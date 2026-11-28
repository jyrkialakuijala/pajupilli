#include "tambplan.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "knob.h"

namespace pajupilli {

namespace {

// Is there a time in the sorted v within [t - before, t + after]?
bool near(const std::vector<double>& v, double t, double before, double after) {
  auto it = std::lower_bound(v.begin(), v.end(), t - before);
  return it != v.end() && *it <= t + after;
}
int countIn(const std::vector<double>& v, double t0, double t1) {
  return int(std::lower_bound(v.begin(), v.end(), t1) - std::lower_bound(v.begin(), v.end(), t0));
}

}  // namespace

std::vector<TambPlanned>& tambourinePlan() {
  static std::vector<TambPlanned> plan;
  return plan;
}

const TambPlanned* plannedTambourine(double t) {
  const std::vector<TambPlanned>& p = tambourinePlan();
  const TambPlanned* best = nullptr;
  double bd = 0.03;
  auto it = std::lower_bound(p.begin(), p.end(), t - 0.03, [](const TambPlanned& a, double x) { return a.t < x; });
  for (; it != p.end() && it->t <= t + 0.03; ++it)
    if (std::fabs(it->t - t) <= bd) { bd = std::fabs(it->t - t); best = &*it; }
  return best;
}

std::vector<TambPlanned> planTambourine(const std::vector<DrumPlanNote>& notes, const std::vector<DrumTempo>& tempo,
                                        const TambDirection& dir, std::string* summary) {
  std::vector<int> idx;
  for (int i = 0; i < int(notes.size()); ++i)
    if (notes[i].drum && notes[i].key == 54) idx.push_back(i);
  const int n = int(idx.size());
  std::vector<TambPlanned> out(n);
  for (int j = 0; j < n; ++j) {
    out[j].t = notes[idx[j]].t;
    out[j].velocity = notes[idx[j]].velocity;
  }
  if (n == 0) return out;

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

  // Listening: what the band plays.
  std::vector<double> hats, ride, crash, snare, shakers, upper, pitchedT;
  std::vector<int> pitchedV;
  for (const DrumPlanNote& x : notes) {
    if (x.drum) {
      if (x.key == 42 || x.key == 44 || x.key == 46) hats.push_back(x.t);
      if (x.key == 51 || x.key == 53 || x.key == 59) ride.push_back(x.t);
      if (x.key == 49 || x.key == 57 || x.key == 52 || x.key == 55) crash.push_back(x.t);
      if (x.key == 38 || x.key == 40) snare.push_back(x.t);
      if (x.key == 69 || x.key == 70 || x.key == 82) shakers.push_back(x.t);
    } else {
      pitchedT.push_back(x.t);
      pitchedV.push_back(x.velocity);
      if (x.key >= 60) upper.push_back(x.t);  // (the tune's register: the voice, the lead)
    }
  }
  double songBand = 64;
  if (!pitchedV.empty()) {
    double s = 0;
    for (int v : pitchedV) s += v;
    songBand = s / pitchedV.size();
  }
  // How hard the band plays around a moment (its keys' mean velocity, a second either side).
  auto bandVel = [&](double t) {
    auto a = std::lower_bound(pitchedT.begin(), pitchedT.end(), t - 1.0) - pitchedT.begin();
    auto b = std::lower_bound(pitchedT.begin(), pitchedT.end(), t + 1.0) - pitchedT.begin();
    if (b - a < 3) return songBand;
    double s = 0;
    for (auto k = a; k < b; ++k) s += pitchedV[k];
    return s / double(b - a);
  };
  const double lastBand = pitchedT.empty() ? -1 : pitchedT.back();

  int plan = int(knob("PJ_TAMB_PLAN", 1));
  // The velocities: intent (several levels, apart) or nothing (one value).
  int vlo = 127, vhi = 0;
  std::vector<int> seen;
  for (int j = 0; j < n; ++j) {
    vlo = std::min(vlo, out[j].velocity);
    vhi = std::max(vhi, out[j].velocity);
    if (std::find(seen.begin(), seen.end(), out[j].velocity) == seen.end()) seen.push_back(out[j].velocity);
  }
  const bool intent = seen.size() >= 3 && vhi - vlo >= 20;
  // On the file's grid (a sixteenth's 0.06 beat) or a transcription's.
  int onGrid = 0;
  for (int j = 0; j < n; ++j) {
    const double q = beatAt(out[j].t) * 4;
    onGrid += std::fabs(q - std::round(q)) < 0.25;
  }
  const bool gridded = onGrid >= 0.7 * n;
  const bool transcribed = !intent && !gridded;

  int nSkipIso = 0, nDouble = 0, nShakerLike = 0, nShakerSoft = 0, nThin = 0, nShakeRoll = 0, nThumbRoll = 0, nDown = 0, nUp = 0,
      nBack = 0, nHead = 0, nLeg = 0, nTwist = 0, nChoke = 0, nLeadIn = 0;
  if (plan <= 0) {
    for (TambPlanned& p : out) {  // (the old single stroke at the file's velocity)
      p.g.drive = TambGesture::kHead;
      p.g.striker = TambGesture::kFingers;
    }
    if (summary) *summary = "  (the tambourine: PJ_TAMB_PLAN=0, every note one stroke)";
    return out;
  }

  // Doubled notes (two within 25 ms): one.
  for (int j = 1; j < n; ++j)
    if (out[j].t - out[j - 1].t < 0.025 && !out[j - 1].skip) { out[j].skip = true; ++nDouble; }

  auto live = [&](int j) { return !out[j].skip; };
  auto prevT = [&](int j) {
    for (int k = j - 1; k >= 0; --k)
      if (live(k)) return out[k].t;
    return -1e9;
  };
  auto nextT = [&](int j) {
    for (int k = j + 1; k < n; ++k)
      if (live(k)) return out[k].t;
    return 1e9;
  };
  std::vector<double> tt;
  for (int j = 0; j < n; ++j) tt.push_back(out[j].t);

  // A transcription's stray notes: alone within two bars either side, a misheard sound, skipped
  // (on a strong beat with the band playing out: a soft stroke, it may be real).
  for (int j = 0; j < n; ++j) {
    if (!transcribed || !live(j)) continue;
    const double b = beatLen(out[j].t);
    const int around = countIn(tt, out[j].t - 8 * b, out[j].t + 8 * b);
    if (around < 4) {
      const double q = beatAt(out[j].t);
      const bool strong = std::fabs(q - std::round(q)) < 0.12;
      if (strong && bandVel(out[j].t) >= 60 && !dir.jazz) {
        out[j].velocity = 45;
      } else {
        out[j].skip = true;
        ++nSkipIso;
      }
    }
  }

  // Runs of fast notes: a roll (5 or more, each within 85 ms or a third of a beat of the last).
  for (int j = 0; j < n;) {
    if (!live(j)) { ++j; continue; }
    int e = j;
    for (int k = j + 1; k < n; ++k) {
      if (!live(k)) continue;
      const double gap = out[k].t - out[e].t;
      if (gap > std::min(0.085, beatLen(out[k].t) / 3)) break;
      e = k;
    }
    int cnt = 0;
    for (int k = j; k <= e; ++k) cnt += live(k);
    if (cnt >= 5) {
      const double b = beatLen(out[j].t);
      const double dur = out[e].t - out[j].t + (out[e].t - out[j].t) / (cnt - 1);
      TambGesture& g = out[j].g;
      const bool loud = (intent ? out[j].velocity : bandVel(out[j].t)) >= 80;
      g.drive = (dur > 1.5 * b || loud) && !(dir.classical && dur <= 1.5 * b) ? TambGesture::kShakeRoll : TambGesture::kThumbRoll;
      g.dur = float(dur);
      g.tilt = 0.9f;
      g.height = loud ? 0.6f : 0.45f;
      g.speed = 1;
      g.speed2 = intent ? float(std::clamp(double(out[e].velocity) / std::max(1, out[j].velocity), 0.5, 2.0))
                        : bandVel(out[e].t + 1) > bandVel(out[j].t) + 5 ? 1.3f : 1.0f;
      if (!intent) out[j].velocity = int(std::clamp(bandVel(out[j].t) - 10, 35.0, 100.0));
      (g.drive == TambGesture::kShakeRoll ? nShakeRoll : nThumbRoll)++;
      for (int k = j + 1; k <= e; ++k) out[k].skip = true;
      j = e + 1;
      continue;
    }
    j = e + 1;
  }
  // A transcription whose tambourine sounds with a shaker's notes again and again (30 % or more):
  // the transcriber heard one sound twice; those notes are the shaker's, not played.
  int withShaker = 0;
  for (int j = 0; j < n; ++j) withShaker += live(j) && transcribed && near(shakers, out[j].t, 0.02, 0.02);
  const bool shakersHeard = transcribed && withShaker >= 0.3 * n;
  if (shakersHeard)
    for (int j = 0; j < n; ++j)
      if (live(j) && near(shakers, out[j].t, 0.02, 0.02)) { out[j].skip = true; ++nShakerLike; }
  std::vector<bool> isRoll(n, false);
  for (int j = 0; j < n; ++j)
    isRoll[j] = live(j) && (out[j].g.drive == TambGesture::kShakeRoll || out[j].g.drive == TambGesture::kThumbRoll);

  // Which backbeat in each bar: the one with less of the tune around it (both where the band plays out).
  auto tuneAround = [&](double t, double b) { return countIn(upper, t - 0.5 * b, t + 0.5 * b); };

  for (int j = 0; j < n; ++j) {
    if (!live(j) || isRoll[j]) continue;
    TambPlanned& p = out[j];
    TambGesture& g = p.g;
    const double t = p.t, b = beatLen(t);
    const double q = beatAt(t), q16 = q * 4;
    const int pos16 = ((int(std::lround(q16)) % 16) + 16) % 16;
    const double off = std::fabs(q16 - std::round(q16));
    const bool onBeat = pos16 % 4 == 0 && off < 0.35, odd = pos16 % 2 == 1, backbeat = (pos16 == 4 || pos16 == 12) && off < 0.35;
    const double gp = (t - prevT(j)) / b, gn = (nextT(j) - t) / b;
    const double band = bandVel(t);
    const bool bandLoud = band >= std::max(songBand + 8, 80.0);
    const bool crashHere = near(crash, t, 0.03, 0.03);
    const bool shakerHere = transcribed && near(shakers, t, 0.02, 0.02);
    const bool groove = gp <= 0.6 || gn <= 0.6;
    g.speed = g.speed2 = 1;
    // How hard: the file's where it means something; else the band's loudness and the meter.
    double v = intent ? p.velocity : std::clamp(band - 12, 35.0, 100.0);
    if (dir.jazz) v -= 10;
    if (dir.amped) v += 6;
    if (shakerHere) { v *= 0.6; ++nShakerSoft; }
    g.height = float(std::clamp(0.35 + 0.4 * (v - 40) / 70, 0.2, 0.85)) + (dir.amped ? 0.1f : 0.0f);
    g.tilt = 0.5f;
    if (groove) {
      const double hatsNow = countIn(hats, t - 0.5 * b, t + 0.5 * b);
      // (the hi-hat already on sixteenths and the tambourine too: the in-between ones left to the hats)
      if (odd && hatsNow >= 4 && gn <= 0.3 && !intent) { p.skip = true; ++nThin; continue; }
      // (jazz: the ride and the hats hold that band; the tambourine only on the beats and the ands)
      if (dir.jazz && odd && !intent) { p.skip = true; ++nThin; continue; }
      if (dir.classical) {
        // The concert tambourine's fast strokes: loud, the fist and the knee in turn; soft, both
        // hands' fingertips with the instrument on the thigh.
        if (v >= 85) {
          if (pos16 % 2 == 0) { g.drive = TambGesture::kHead; g.striker = TambGesture::kFist; g.place = 0.8f; ++nHead; }
          else { g.drive = TambGesture::kAgainst; g.striker = TambGesture::kKnee; ++nLeg; }
          g.height = 0.3f;
        } else {
          g.drive = TambGesture::kHead;
          g.striker = TambGesture::kFingertips;
          g.fingers = 2;
          g.place = 0.15f;
          g.height = 0.2f;
          g.tilt = 0.4f;
          ++nHead;
        }
      } else {
        // The pop player's motion: down on the beats and the ands, up between; the backbeat with
        // the snare struck against the other hand's heel.
        const bool withSnare = backbeat && near(snare, t, 0.035, 0.035);
        bool stopped = false;
        if (withSnare) {
          // (one of 2 and 4 unless the band plays out: the one with less of the tune around it)
          const double other = t + (pos16 == 4 ? 2 : -2) * b;
          const bool otherToo = backbeat && countIn(tt, other - 0.1 * b, other + 0.1 * b) > 0 && near(snare, other, 0.035, 0.035);
          stopped = bandLoud || dir.amped || !otherToo || tuneAround(t, b) < tuneAround(other, b) ||
                    (tuneAround(t, b) == tuneAround(other, b) && pos16 == 12);
        }
        if (stopped) {
          g.drive = TambGesture::kAgainst;
          g.striker = TambGesture::kHeel;
          v += intent ? 4 : 10;
          g.height = std::min(0.95f, g.height + 0.15f);
          ++nBack;
        } else {
          g.drive = TambGesture::kAir;
          g.dir = odd ? -1 : 1;
          g.tilt = 0.65f;
          if (!intent) v += onBeat ? 4 : odd ? -6 : 0;
          if (odd && tuneAround(t, b) >= 3) v -= 6;  // (the tune busy: the in-between ones recede)
          (odd ? nUp : nDown)++;
        }
        g.lift = gp * b >= 0.2 && !odd ? 0.5f : 0.0f;
      }
    } else {
      // Alone: a stroke chosen by its strength; on the head for the concert tambourine and soft
      // strokes, a stopped hit or a twist for the pop player's accents.
      if (!intent) v += onBeat && pos16 == 0 ? 8 : backbeat ? 4 : off > 0.35 || odd ? -6 : 0;
      g.drive = TambGesture::kHead;
      if (v <= 45) { g.striker = TambGesture::kFingertips; g.fingers = v <= 30 ? 1 : 2; g.place = 0.1f; g.tilt = 0.45f; }
      else if (v <= 75) { g.striker = TambGesture::kFingers; g.place = 0.5f; }
      else if (v <= 100 || dir.classical) { g.striker = v <= 100 ? TambGesture::kFist : TambGesture::kPalm; g.place = v <= 100 ? 0.8f : 0.9f; }
      else { g.drive = TambGesture::kAgainst; g.striker = TambGesture::kHeel; g.lift = 0.6f; }
      if (crashHere && !dir.classical) { g.drive = TambGesture::kTwist; g.lift = 0.6f; }
      (g.drive == TambGesture::kHead ? nHead : g.drive == TambGesture::kTwist ? nTwist : nBack)++;
    }
    if (crashHere) { v -= 8; g.lift = 0; }  // (the crash covers it)
    // What follows: the song's last note choked when the band stops with it; before the band's
    // general rest (nothing for two beats) held still.
    if (lastBand > 0 && t >= lastBand - 0.1 && nextT(j) > 1e8) {
      g.follow = TambGesture::kStop;
      g.stopAfter = float(0.6 * b);
      if (g.drive == TambGesture::kAir) { g.drive = TambGesture::kHead; g.striker = TambGesture::kPalm; g.place = 0.8f; }
      ++nChoke;
    } else if (!near(pitchedT, t, -0.05, 2 * b) && gn > 2) {
      g.follow = TambGesture::kStop;
      g.stopAfter = float(0.5 * b);
      ++nChoke;
    }
    p.velocity = int(std::clamp(std::lround(v), 1L, 127L));
  }

  // Lead-ins: where a run begins after two bars' rest and the band grows louder there (or a
  // crash marks it), a shake of a beat into it, the first stroke struck. At most twice; not in
  // a classical reading or jazz.
  std::vector<TambPlanned> added;
  if (!dir.classical && !dir.jazz) {
    for (int j = 0; j < n && nLeadIn < int(knob("PJ_TAMB_LEADINS", 2)); ++j) {
      if (!live(j) || isRoll[j]) continue;
      const double t = out[j].t, b = beatLen(t);
      if (t - prevT(j) < 8 * b || countIn(tt, t, t + 8 * b) < 8) continue;
      if (!(bandVel(t + 2) >= bandVel(t - 2) + 6 || near(crash, t, 0.03, 0.03))) continue;
      TambPlanned a;
      a.t = t - b;
      a.added = true;
      a.velocity = std::max(30, out[j].velocity - 10);
      a.g.drive = TambGesture::kShakeRoll;
      a.g.dur = float(b - 0.03);
      a.g.speed = 0.6f;
      a.g.speed2 = 1.1f;
      a.g.tilt = 0.9f;
      a.g.height = 0.6f;
      a.g.follow = TambGesture::kHold;  // (no ending stroke: the run's first is it)
      added.push_back(a);
      TambGesture& g = out[j].g;
      g.drive = TambGesture::kAgainst;
      g.striker = TambGesture::kHeel;
      g.lift = 0;
      out[j].velocity = std::min(127, out[j].velocity + 6);
      ++nLeadIn;
    }
  }
  for (const TambPlanned& a : added) out.push_back(a);

  if (summary) {
    char buf[700];
    int kept = 0;
    for (int j = 0; j < n; ++j) kept += live(j);
    std::snprintf(buf, sizeof buf,
                  "  (the tambourine: %d notes, read as %s%s; played %d: skipped %d stray, %d doubled, %d in between left to the hats (or jazz's ride); "
                  "%d left to the shaker (heard twice), %d softer with its note; rolls %d shaken, %d with the thumb; in the groove %d down, %d up, %d backbeats "
                  "against the hand; strokes on the head %d, against the leg %d, twists %d; choked %d; lead-in shakes added %d)",
                  n, intent ? "its velocities meant" : transcribed ? "a transcription (flat, off the grid)" : "flat velocities on the grid",
                  dir.classical ? ", the concert tambourine" : dir.jazz ? ", jazz" : dir.amped ? ", rock or funk" : "", kept, nSkipIso, nDouble, nThin,
                  nShakerLike, nShakerSoft, nShakeRoll, nThumbRoll, nDown, nUp, nBack, nHead, nLeg, nTwist, nChoke, nLeadIn);
    *summary = buf;
  }
  return out;
}

}  // namespace pajupilli
