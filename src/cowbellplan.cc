#include "cowbellplan.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>

#include "knob.h"

namespace pajupilli {

namespace {

// Is there a time in the sorted v within w (s) of t?
bool near(const std::vector<double>& v, double t, double w) {
  auto it = std::lower_bound(v.begin(), v.end(), t - w);
  return it != v.end() && *it <= t + w;
}

}  // namespace

std::vector<CowbellStroke> planCowbell(const std::vector<DrumPlanNote>& notes, const std::vector<DrumTempo>& tempo, std::string* summary) {
  std::vector<int> idx;
  for (int i = 0; i < int(notes.size()); ++i)
    if (notes[i].drum && notes[i].key == 56) idx.push_back(i);
  const int n = int(idx.size());
  std::vector<CowbellStroke> out(n);
  for (int j = 0; j < n; ++j) out[j].velocity = notes[idx[j]].velocity;
  if (n == 0 || knob("PJ_COWBELL_PLAN", 1) <= 0) return out;

  // The tempo map: the quarter notes at a time, and how long one lasts there.
  auto seg = [&](double t) -> const DrumTempo* {
    const DrumTempo* s = nullptr;
    for (const DrumTempo& x : tempo)
      if (x.t <= t) s = &x;
    return s ? s : tempo.empty() ? nullptr : &tempo[0];
  };
  auto beatAt = [&](double t) {
    const DrumTempo* s = seg(t);
    return s ? s->beat + (t - s->t) / s->len : t / 0.5;
  };
  auto beatLen = [&](double t) {
    const DrumTempo* s = seg(t);
    return s ? s->len : 0.5;
  };

  // What the band plays, for listening: the clave, the bass drum, the snare, the bass's notes.
  std::vector<double> clave, kick, snare, bass, pitched;
  std::vector<std::pair<double, int>> pitchedVel;
  int latinN = 0;
  for (const DrumPlanNote& x : notes) {
    if (x.drum) {
      if (x.key == 75) clave.push_back(x.t);
      if (x.key == 35 || x.key == 36) kick.push_back(x.t);
      if (x.key == 38 || x.key == 40) snare.push_back(x.t);
      latinN += (x.key >= 60 && x.key <= 66) || x.key == 75 || x.key == 73 || x.key == 74;
    } else {
      if (x.key < 48) bass.push_back(x.t);
      pitchedVel.push_back({x.t, x.velocity});
    }
  }
  const bool latin = latinN >= 16;           // a Latin band's percussion: the bell is held in the hand
  const bool backbeat = snare.size() >= 16;  // a kit's groove with a snare

  // The file's velocities: do they say something (two levels or more, apart), or are they flat?
  std::map<int, int> hist;
  double mean = 0;
  for (int j = 0; j < n; ++j) { ++hist[out[j].velocity]; mean += out[j].velocity; }
  mean /= n;
  int most = 0;
  for (const auto& [v, c] : hist) most = std::max(most, c);
  double sd = 0;
  for (int j = 0; j < n; ++j) sd += (out[j].velocity - mean) * (out[j].velocity - mean);
  sd = std::sqrt(sd / n);
  const bool flat = most >= 0.85 * n || sd < 3;

  // Where the notes lie on the file's grid: sixteenths, triplets, or neither (a transcription).
  std::vector<double> beat(n), ph(n);
  int offGrid = 0, trip = 0;
  for (int j = 0; j < n; ++j) {
    beat[j] = beatAt(notes[idx[j]].t);
    ph[j] = beat[j] - std::floor(beat[j]);
    const double d16 = std::fabs(ph[j] * 4 - std::round(ph[j] * 4)) / 4, d3 = std::fabs(ph[j] * 3 - std::round(ph[j] * 3)) / 3;
    offGrid += std::min(d16, d3) > 0.05;
    trip += d3 < 0.03 && d16 > 0.06;
  }
  const bool triplets = trip > 0.3 * n;
  const bool transcription = flat && offGrid > 0.35 * n;

  // Each note's weight as an accent (S): its place in the bar (4/4 from the
  // song's start), what it lands with, the file's velocity.
  std::vector<double> S(n);
  std::vector<int> beatInBar(n), sub(n);  // sub: 0 the beat, 2 the "&", 1 / 3 the sixteenths (triplets: 1, 2)
  for (int j = 0; j < n; ++j) {
    const double t = notes[idx[j]].t, bl = beatLen(t);
    const int div = triplets ? 3 : 4;
    int g = int(std::lround(ph[j] * div));
    const long b = long(std::floor(beat[j])) + (g == div);
    g %= div;
    beatInBar[j] = int(((b % 4) + 4) % 4);
    sub[j] = triplets ? (g == 0 ? 0 : g == 2 ? 2 : 1) : g;
    static const double kBeat[4] = {1.0, 0.6, 0.85, 0.6};
    double w = sub[j] == 0 ? kBeat[beatInBar[j]] : sub[j] == 2 ? 0.4 : 0.2;
    if (transcription) w = 0.5;  // (the file's grid is not the music's)
    if (latin && near(clave, t, 0.03)) w += 0.35;
    if (near(kick, t, 0.03)) w += 0.2;
    if (j == 0 || notes[idx[j]].t - notes[idx[j - 1]].t > 0.9 * bl) w += 0.15;  // the first after a gap
    if (!flat) w += 0.35 * std::clamp((out[j].velocity - mean) / std::max(8.0, sd), -2.0, 2.0);
    S[j] = w;
  }

  // The bandsman's level: what the pitched parts play around a moment (a second either side).
  auto bandVel = [&](double t) {
    double sum = 0;
    int c = 0;
    auto it = std::lower_bound(pitchedVel.begin(), pitchedVel.end(), std::make_pair(t - 1, -1));
    for (; it != pitchedVel.end() && it->first <= t + 1; ++it) { sum += it->second; ++c; }
    return c >= 3 ? sum / c : 90.0;
  };

  int count[4] = {}, muted = 0, skipped = 0, rings = 0;
  for (int j = 0; j < n; ++j) {
    const double t = notes[idx[j]].t, bl = beatLen(t);
    CowbellStroke& o = out[j];
    // (a doubled note: played once)
    if (j > 0 && t - notes[idx[j - 1]].t < 0.025) { o.skip = true; ++skipped; continue; }
    // Ranked among the notes of its bar (a transcription: of two beats either side).
    double lower = 0;
    int others = 0;
    const double b0 = transcription ? beat[j] - 2 : std::floor((beat[j] + 0.125) / 4) * 4, b1 = transcription ? beat[j] + 2 : b0 + 4;
    for (int k = 0; k < n; ++k) {
      if (k == j || beat[k] < b0 - 0.125 || beat[k] >= b1 - 0.125) continue;
      ++others;
      lower += S[k] < S[j] - 1e-6 ? 1 : S[k] <= S[j] + 1e-6 ? 0.5 : 0;
    }
    const double q = others ? lower / others : std::clamp(S[j], 0.0, 1.0);
    const double score = 0.5 * q + 0.5 * std::clamp(S[j], 0.0, 1.0);
    int place = score >= 0.72 ? 0 : score >= 0.55 ? 1 : score >= 0.35 ? 2 : 3;
    double vel = o.velocity;
    // A rock or funk groove: on the snare's backbeat the bell locks with it, softer and nearer the crown.
    const bool onSnare = backbeat && !latin && near(snare, t, 0.03);
    if (onSnare) place = std::min(3, place + 1);
    // A transcription's isolated note (fewer than three others within four bars): off the beat
    // it is likely misheard, skipped; on it, played soft on the body.
    if (transcription) {
      int around = 0;
      for (int k = 0; k < n; ++k) around += k != j && std::fabs(beat[k] - beat[j]) < 16;
      if (around < 3) {
        const double d = std::fabs(ph[j] - std::round(ph[j]));
        if (d > 0.08) { o.skip = true; ++skipped; continue; }
        place = 2;
        vel *= 0.7;
      }
    }
    // The hand: on a held bell the lower fingers press the face or lift off, a choice of its own
    // beside mouth or top (four tones: open or muted, mouth or top): open for an accent, or a
    // note with room to ring (a beat or more to the next); pressed for the in-between notes
    // that come quick (half a beat or less), so they do not smear; lightly between. A kit's
    // bell (mounted, taped inside) has no hand: a little muted off the mouth. The last
    // stroke before a rest rings.
    const double gap = j + 1 < n ? (notes[idx[j + 1]].t - t) / bl : 99;
    int mute;
    if (latin) mute = score >= 0.72 || gap >= 1 ? 0 : gap <= 0.55 && score < 0.55 ? 2 : 1;
    else mute = place >= 2 ? 1 : 0;
    if (gap > 2) { mute = 0; ++rings; }
    // The level: the file's own where it says something; else the stroke's role, near the band's.
    if (flat) {
      static const double kRole[4] = {1.0, 0.92, 0.85, 0.78};
      vel = std::min(vel, bandVel(t) + 25) * kRole[place];
      if (place == 0 && sub[j] == 0 && beatInBar[j] == 0 && !transcription) vel *= 1.06;
      if (onSnare) vel *= 0.92;
    }
    o.velocity = std::clamp(int(std::lround(vel)), 1, 127);
    o.key = (latin ? kKeyCowbellStroke0 : kKeyCowbellMounted0) + 4 * place + mute;
    ++count[place];
    muted += mute > 0;
  }
  if (summary) {
    char buf[400];
    std::snprintf(buf, sizeof buf,
                  "  (the cowbell: %d notes, %s, %s%s%s; mouth %d, lip %d, body %d, crown %d; %d muted (%s), %d left to ring, %d skipped)",
                  n, latin ? "held in the hand (a Latin band)" : "mounted on the kit", flat ? "flat velocities" : "the file's velocities as its accents",
                  transcription ? ", a transcription (off the grid)" : "", triplets ? ", on triplets" : "", count[0], count[1], count[2], count[3],
                  muted, latin ? "the hand" : "the tape inside", rings, skipped);
    *summary = buf;
  }
  return out;
}

}  // namespace pajupilli
