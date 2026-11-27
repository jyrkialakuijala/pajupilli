#include "director.h"
#include "velline.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>

#include "knob.h"

namespace pajupilli {

namespace {

int gMode = 0;  // -1 off, 0 auto, 1 on
bool gFrench = false;  // -style french: Baroque with notes inégales
int gStyle = -1;  // -style: the reading's style forced (a Direction::Style; -1: read from the rhythm)

double median(std::vector<double> v) {
  if (v.empty()) return 0;
  std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
  return v[v.size() / 2];
}

// Seconds a quarter lasts at a beat (the file's own tempo map).
double lenAt(const std::vector<DirTempo>& tm, double beat) {
  double len = 0.5;
  for (const DirTempo& x : tm) {
    if (x.beat > beat + 1e-9) break;
    len = x.len;
  }
  return len;
}

}  // namespace

bool setDirectorMode(const std::string& mode) {
  if (mode == "on") gMode = 1;
  else if (mode == "off") gMode = -1;
  else if (mode == "auto") gMode = 0;
  else return false;
  return true;
}

bool setDirectorStyle(const std::string& style) {
  gFrench = style == "french";
  if (style == "baroque" || style == "french") gStyle = Direction::kBaroque;
  else if (style == "classical") gStyle = Direction::kClassical;
  else if (style == "romantic") gStyle = Direction::kRomantic;
  else if (style == "auto") gStyle = -1;
  else return false;
  if (gStyle >= 0 && gMode == 0) gMode = 1;  // (a style given: the director acts, unless -director off)
  return true;
}

float levelAt(const std::vector<std::pair<long, float>>& c, long tick) {
  if (c.empty()) return 0.5f;
  if (tick <= c.front().first) return c.front().second;
  if (tick >= c.back().first) return c.back().second;
  auto hi = std::upper_bound(c.begin(), c.end(), std::make_pair(tick, 1e9f));
  auto lo = hi - 1;
  const double a = hi->first > lo->first ? double(tick - lo->first) / double(hi->first - lo->first) : 0.0;
  return float(lo->second + a * (hi->second - lo->second));
}

bool& jazzPiece() {
  static bool jazz = false;
  return jazz;
}

// Jazz, from what a listener hears first (research/bass_amps.md 13.6): swung eighths (the off-beat
// at two thirds of the beat, not at half: in a file played or written with its swing), the time
// kept on the ride cymbal more than on the hi-hat, a walking bass (a line of quarter notes), the
// jazz band's instruments (a jazz guitar, saxophones, a vibraphone, a muted trumpet). Two of
// those, the swing counting twice, or src/midifile.cc's trio (a piano and an upright bass).
// PJ_DIRECTOR_JAZZ: 1 always, 0 never (-1: read).
static void readJazz(const DirInput& in, Direction& d) {
  const long div = std::max(1, in.div);
  int half = 0, twoThirds = 0;
  const auto offbeat = [&](long tick) {
    const double f = double(tick % div) / double(div);
    half += std::fabs(f - 0.5) < 0.06;
    twoThirds += std::fabs(f - 2.0 / 3) < 0.06;
  };
  for (const DirNote& x : in.notes) offbeat(x.tick);
  int ride = 0, hat = 0;
  for (const auto& [t, k] : in.drums) {
    offbeat(t);
    ride += k == 51 || k == 59 || k == 53;
    hat += k == 42 || k == 46;
  }
  const int offs = half + twoThirds;
  const double swing = offs ? double(twoThirds) / offs : 0;
  // The bass: the lowest channel with a line of its own; walking, if most of its steps are a beat.
  int chN[16] = {};
  double chKey[16] = {};
  for (const DirNote& x : in.notes) { ++chN[x.ch]; chKey[x.ch] += x.key; }
  int bass = -1;
  for (int c = 0; c < 16; ++c)
    if (chN[c] >= 16 && chKey[c] / chN[c] < 52 && (bass < 0 || chKey[c] / chN[c] < chKey[bass] / chN[bass])) bass = c;
  double walk = 0, eighths = 0;
  if (bass >= 0) {
    long last = -1;
    int steps = 0, beats = 0, halves = 0;
    for (const DirNote& x : in.notes) {
      if (x.ch != bass || x.tick == last) continue;
      if (last >= 0) {
        ++steps;
        beats += std::labs(x.tick - last - div) <= div / 8;
        halves += x.tick - last < div * 7 / 8;
      }
      last = x.tick;
    }
    walk = steps ? double(beats) / steps : 0;
    eighths = steps ? double(halves) / steps : 0;
  }
  const bool swung = offs >= 16 && swing >= 0.6, rides = ride >= 16 && ride >= 1.5 * hat, walks = walk >= 0.5;
  const bool band = in.program[26] || in.program[64] || in.program[65] || in.program[66] || in.program[67] || in.program[11] || in.program[59];
  const int score = 2 * swung + rides + walks + band;
  d.jazz = in.jazzBand || score >= 2;
  static const int kForce = int(knob("PJ_DIRECTOR_JAZZ", -1));
  if (kForce >= 0) d.jazz = kForce > 0;
  char why[240];
  std::snprintf(why, sizeof why, "%s: off-beats swung %.0f %% (of %d), ride %d against hi-hat %d, the bass (channel %d) in beats %.0f %%, quicker %.0f %%%s%s%s",
                d.jazz ? "jazz" : "not jazz", 100 * swing, offs, ride, hat, bass + 1, 100 * walk, 100 * eighths, band ? ", a jazz band's instruments" : "",
                in.jazzBand ? ", a piano and an upright bass" : "", kForce >= 0 ? " (PJ_DIRECTOR_JAZZ)" : "");
  d.jazzWhy = why;
}

Direction direct(const DirInput& in) {
  Direction d;
  readJazz(in, d);
  const std::vector<DirNote>& nt = in.notes;
  const int n = int(nt.size());
  d.cues.assign(n, Cue{{0.5f, 0.5f, 0, 0.5f, 0, 1, 0, 1, 1, 0, 0, 0}});
  static const double kAll = knob("PJ_DIRECTOR", 1.0);
  static const double kDyn = kAll * knob("PJ_DIRECTOR_DYN", 1.0), kAcc = kAll * knob("PJ_DIRECTOR_ACCENT", 1.0);
  static const double kVib = knob("PJ_DIRECTOR_VIB", 1.0), kRit = kAll * knob("PJ_DIRECTOR_RIT", 1.0);
  static const double kHold = knob("PJ_DIRECTOR_FERMATA", 1.6), kQuant = knob("PJ_DIRECTOR_QUANT", 0.9);
  if (n < 8) {
    d.summary = "director: too few notes to read";
    return d;
  }
  const long div = std::max(1, in.div), bar = in.barTicks > 0 ? in.barTicks : 4 * div;
  long lastOff = 0, lastOn = 0;
  for (const DirNote& x : nt) { lastOff = std::max(lastOff, x.tickOff); lastOn = std::max(lastOn, x.tick); }
  const double bars = double(lastOff) / double(bar);

  // --- Does it act: a file with no performance of its own.
  int onGrid = 0;
  const long tol = std::max(1L, div / 50);
  for (const DirNote& x : nt) {
    bool g = false;
    for (long q : {div / 4, div / 3, div / 8})
      if (q > 0) { const long r = x.tick % q; g |= r <= tol || q - r <= tol; }
    onGrid += g;
  }
  const double quant = double(onGrid) / n;
  bool flat = true;
  int chNotes[16] = {};
  double chKey[16] = {};
  {
    std::map<int, int> vel[16];
    for (const DirNote& x : nt) { ++vel[x.ch][x.vel]; ++chNotes[x.ch]; chKey[x.ch] += x.key; }
    for (int c = 0; c < 16; ++c) {
      if (chNotes[c] < 8) continue;
      std::vector<int> cnt;
      for (auto& [v, k] : vel[c]) cnt.push_back(k);
      std::sort(cnt.rbegin(), cnt.rend());
      int top = 0;
      for (size_t i = 0; i < cnt.size() && i < 3; ++i) top += cnt[i];
      flat &= top >= 0.9 * chNotes[c];
    }
  }
  const bool fewTempo = double(in.tempo.size()) <= 1 + bars / 4;
  d.acts = gMode > 0 || (gMode == 0 && quant >= kQuant && flat && fewTempo);
  d.gridded = quant >= kQuant;
  char why[200];
  std::snprintf(why, sizeof why, "%.0f %% of the notes on the grid, velocities %s, %d tempo events in %.0f bars",
                100 * quant, flat ? "flat" : "varied", int(in.tempo.size()), bars);
  // Without the director (off, or out of a file with its own performance), the wind players still
  // read their own lines (PJ_WIND_SELF, 1): a wind part with no breath or expression of its own
  // (no CC 11, 1, 2 or pressure) is not blown like a machine. Its players say their notes, shape
  // their phrases to the line's high point and play with the piece's drama (below); the tempo,
  // the other parts and every other decision stay the file's (d.self, not d.acts).
  // The vibrato is planned for every part, whatever the file has of its own: a played file (its
  // velocities, its tempo, a channel's expression rides) keeps all of that, and its players
  // still get the director's vibrato for each note (its depth, delay and speed; each player
  // chooses how). Not with the director off. PJ_DIRECTOR_VIBALL (1; 0: as before, only where it
  // reads the part).
  d.vib = gMode >= 0 && knob("PJ_DIRECTOR_VIBALL", 1.0) > 0;
  const std::string outWhy = gMode < 0 ? "director: off" : std::string("director: stays out (the file has its own performance: ") + why + ")";
  if (!d.acts) {
    bool any = false;
    for (int c = 0; c < 16; ++c) any |= in.wind[c] && !in.ridden[c] && chNotes[c] > 0;
    if (!any || knob("PJ_WIND_SELF", 1.0) <= 0) {
      d.summary = outWhy;
      if (!d.vib) return d;
      d.vibOnly = true;  // (read on: only its vibrato is played)
    } else {
      d.self = true;
    }
  }
  const bool self = d.self || d.vibOnly;

  // --- The parts: which carries the melody (the highest), which the bass (the lowest).
  int melody = -1, bass = -1, channels = 0;
  for (int c = 0; c < 16; ++c) {
    if (chNotes[c] == 0) continue;
    ++channels;
    chKey[c] /= chNotes[c];
    if (chNotes[c] < 0.05 * n) continue;
    if (melody < 0 || chKey[c] > chKey[melody]) melody = c;
    if (bass < 0 || chKey[c] < chKey[bass]) bass = c;
  }
  if (melody < 0) melody = nt[0].ch;
  if (channels < 2) bass = -1;

  // --- The style, from the rhythm: a motor of even short notes with few long ones (Baroque);
  // long melodic notes (Romantic); else Classical.
  std::vector<long> onsets;
  for (const DirNote& x : nt)
    if (onsets.empty() || x.tick != onsets.back()) onsets.push_back(x.tick);
  std::map<long, int> ioi;
  for (size_t i = 1; i < onsets.size(); ++i) ++ioi[onsets[i] - onsets[i - 1]];
  long modeIoi = div;
  int modeN = 0;
  for (auto& [k, c] : ioi)
    if (c > modeN) { modeN = c; modeIoi = k; }
  const double motor = onsets.size() > 1 ? double(modeN) / double(onsets.size() - 1) : 0;
  int longN = 0, melN = 0, melLong = 0;
  std::vector<double> melDur;
  for (const DirNote& x : nt) {
    const long dur = x.tickOff - x.tick;
    longN += dur >= 2 * div;
    if (x.ch == melody) { ++melN; melLong += dur >= 3 * div / 2; melDur.push_back(double(dur)); }
  }
  const double longShare = double(longN) / n, melLongShare = melN ? double(melLong) / melN : 0;
  d.style = motor >= 0.55 && longShare < 0.08 && modeIoi <= div / 2 ? Direction::kBaroque
          : melLongShare >= 0.3 ? Direction::kRomantic : Direction::kClassical;
  if (gStyle >= 0) d.style = Direction::Style(gStyle);  // (-style)
  const bool baroque = d.style == Direction::kBaroque, romantic = d.style == Direction::kRomantic;

  // --- Phrases: everyone rests together; or the melody rests on a long note; none longer than
  // eight bars (split in fours), none shorter than a bar (joined to the one before).
  std::set<long> cut;
  {
    long maxOff = 0;
    for (const DirNote& x : nt) {
      if (maxOff > 0 && x.tick - maxOff >= div / 2) cut.insert(x.tick);
      maxOff = std::max(maxOff, x.tickOff);
    }
    const double md = median(melDur);
    long prevOff = -1;
    bool prevLong = false;
    for (const DirNote& x : nt) {
      if (x.ch != melody) continue;
      if (prevLong && x.tick >= prevOff - div / 8) cut.insert(x.tick);
      const long dur = x.tickOff - x.tick;
      prevLong = dur >= std::max(2.0 * md, 1.5 * div) && dur >= 2 * div;
      prevOff = x.tickOff;
    }
  }
  {
    std::vector<long> b = {nt[0].tick};
    for (long c : cut)
      if (c - b.back() >= bar) b.push_back(c);
    b.push_back(lastOff);
    for (size_t i = 0; i + 1 < b.size(); ++i) {
      long s = b[i];
      const long e = b[i + 1];
      while (e - s > 8 * bar) {
        d.phrases.push_back({s, s + 4 * bar});
        s += 4 * bar;
      }
      if (e > s) d.phrases.push_back({s, e});
    }
  }
  const int np = int(d.phrases.size());
  auto phraseOf = [&](long tick) {
    int lo = 0, hi = np - 1;
    while (lo < hi) {
      const int m = (lo + hi + 1) / 2;
      if (d.phrases[m].first <= tick) lo = m; else hi = m - 1;
    }
    return lo;
  };

  // --- Each phrase: how many parts play (the texture), its climax (the melody's highest note,
  // the one nearest two thirds of the way if several), whether it repeats the phrase before.
  std::vector<int> climax(np, -1);
  std::vector<double> texture(np, 0);
  std::vector<std::vector<int>> sig(np);
  {
    std::vector<std::set<int>> who(np);
    for (int i = 0; i < n; ++i) {
      const int p = phraseOf(nt[i].tick);
      who[p].insert(nt[i].ch);
      if (nt[i].ch != melody) continue;
      sig[p].push_back(nt[i].key);
      const long want = d.phrases[p].first + 2 * (d.phrases[p].second - d.phrases[p].first) / 3;
      const int c = climax[p];
      if (c < 0 || nt[i].key > nt[c].key ||
          (nt[i].key == nt[c].key && std::labs(nt[i].tick - want) < std::labs(nt[c].tick - want)))
        climax[p] = i;
    }
    for (int p = 0; p < np; ++p) texture[p] = double(who[p].size()) / std::max(1, channels);
  }
  int top = -1;  // the piece's climax
  for (int p = 0; p < np; ++p)
    if (climax[p] >= 0 && (top < 0 || nt[climax[p]].key > nt[climax[top]].key ||
                           (nt[climax[p]].key == nt[climax[top]].key && texture[p] >= texture[top])))
      top = p;
  auto echo = [&](int p) {
    if (p == 0 || sig[p].size() < 4 || sig[p].size() != sig[p - 1].size()) return false;
    for (size_t i = 1; i < sig[p].size(); ++i)
      if (sig[p][i] - sig[p][i - 1] != sig[p - 1][i] - sig[p - 1][i - 1]) return false;
    return true;
  };
  // A phrase said again: its keying a little different each time, for interest (the player's
  // touch, not the dynamics). Twice: long, then shorter; three times: mid, short, long; more:
  // long first and last, short and mid between. Found by bars, not by the phrases above (their
  // ends rarely fall on a repeat): four, two or one bars of the melody said again at once, the
  // same rhythm and the same steps within a semitone (a sequence a step higher or lower counts:
  // Bach's melodies are rarely said again exactly, but often so), at least four notes, the
  // longest first. The short notes (up to two beats) of each statement, not the
  // phrase's last (the breath), in every part; PJ_DIRECTOR_REPEAT (1) scales it.
  const long nbars = lastOff / bar + 1;
  std::vector<std::vector<std::pair<long, int>>> bsig(static_cast<size_t>(nbars));
  for (const DirNote& x : nt)
    if (x.ch == melody && x.tick / bar < nbars) bsig[size_t(x.tick / bar)].push_back({x.tick % bar, x.key});
  auto same = [&](long a, long b, int w) {
    if (b + w > nbars) return false;
    int count = 0, pa = -1, pb = -1;
    for (int k = 0; k < w; ++k) {
      const auto& A = bsig[size_t(a + k)];
      const auto& B = bsig[size_t(b + k)];
      if (A.size() != B.size()) return false;
      for (size_t q = 0; q < A.size(); ++q) {
        if (A[q].first != B[q].first) return false;
        // (each step within a semitone of the first statement's: a diatonic sequence's steps
        // turn major into minor and back)
        if (pa >= 0 && std::abs((B[q].second - pb) - (A[q].second - pa)) > 1) return false;
        pa = A[q].second;
        pb = B[q].second;
        ++count;
      }
    }
    return count >= 4;
  };
  std::vector<int> rep(static_cast<size_t>(nbars), 0), run(static_cast<size_t>(nbars), 1);
  int repeated = 0;
  for (int w : {4, 2, 1})
    for (long b = 0; b + w < nbars;) {
      long e = b + w;
      while (same(b, e, w)) e += w;
      bool free = e - b >= 2 * w;
      for (long k = b; k < e && free; ++k) free = run[size_t(k)] == 1;
      if (!free) { ++b; continue; }
      const int L = int((e - b) / w);
      for (long k = b; k < e; ++k) {
        rep[size_t(k)] = int((k - b) / w);
        run[size_t(k)] = L;
      }
      repeated += L - 1;
      b = e;
    }
  static const double kRep = kAll * knob("PJ_DIRECTOR_REPEAT", 1.0);
  auto keying = [&](long tick) {
    const size_t k = size_t(std::clamp(tick / bar, 0L, nbars - 1));
    const int L = run[k], r = rep[k];
    constexpr double kShort = 0.86, kMid = 0.93;
    const double f = L < 2 ? 1 : L == 2 ? (r == 0 ? 1 : kShort) : L == 3 ? (r == 0 ? kMid : r == 1 ? kShort : 1)
                   : (r == 0 || r == L - 1) ? 1 : (r % 2 ? kShort : kMid);
    return 1 + (f - 1) * kRep;
  };

  // --- The arc. Baroque: terraces (the full texture forte, the thin one piano, an echo softer,
  // a small swell into the cadence). Otherwise hairpins: up to each phrase's climax and down
  // again, the piece's climax the highest.
  std::vector<std::pair<long, float>> arc;  // (none for the winds' own reading: there is no conductor)
  double meanTex = 0;
  for (double t : texture) meanTex += t / np;
  int echoes = 0;
  for (int p = 0; p < np && !self; ++p) {
    const long s = d.phrases[p].first, e = d.phrases[p].second;
    if (baroque) {
      const bool ec = echo(p);
      echoes += ec;
      const double l = 0.5 + kDyn * (0.25 * (texture[p] - meanTex) - (ec ? 0.10 : 0.0));
      arc.push_back({s, float(l)});
      arc.push_back({std::max(s, e - bar / 2), float(l)});
      arc.push_back({std::max(s + 1, e - 1), float(l + 0.03 * kDyn)});
    } else {
      const double a = (romantic ? 0.20 : 0.12) + (p == top ? 0.06 : 0.0);
      const long c = climax[p] >= 0 ? std::clamp(nt[climax[p]].tick, s + (e - s) / 4, e - (e - s) / 8) : s + 2 * (e - s) / 3;
      arc.push_back({s, float(0.5 - 0.04 * kDyn)});
      arc.push_back({c, float(0.5 + a * kDyn)});
      arc.push_back({std::max(c + 1, e - 1), float(0.5 - 0.08 * kDyn)});
    }
  }
  std::sort(arc.begin(), arc.end());
  // --- Mutes: on for the thin or echoing phrases of a slow piece (up to 76 quarters a minute),
  // not in the Baroque.
  static const double kMute = knob("PJ_DIRECTOR_MUTE", 1.0);
  std::vector<bool> muted(np, false);
  int mutedN = 0;
  {
    const bool slow = lenAt(in.tempo, 0) >= 60.0 / 76;
    bool was = false;
    for (int p = 0; p < np; ++p) {
      muted[p] = kMute >= 2 || (kMute > 0 && !baroque && slow && (texture[p] < meanTex - 0.01 || echo(p)));
      mutedN += muted[p];
      if (muted[p] != was) d.mutes.push_back({d.phrases[p].first, bool(muted[p])});
      was = muted[p];
    }
  }

  // --- Meter: each place in the bar has its weight.
  // A compound metre (6/8, 9/8, 12/8: the beat a dotted quarter): the onsets fall on the dotted
  // quarters far more than on the plain quarters between them (the file says only the bar's length).
  bool compound = false;
  if (div % 2 == 0 && bar % (3 * div / 2) == 0 && bar != 3 * div / 2) {
    int onDot = 0, onQuarter = 0;
    const long dq = 3 * div / 2;
    for (const DirNote& x : nt) {
      const long pos = x.tick % bar;
      if (pos == 0) continue;
      onDot += pos % dq == 0 && pos % div != 0;
      onQuarter += pos % div == 0 && pos % dq != 0;
    }
    compound = onDot >= n / 30 && onDot > 2 * onQuarter;
  }
  // The dance (PJ_DIRECTOR_DANCE, 1; Baroque only): its bar has its own weights (Mattheson 1739;
  // Little and Jenne, Dance and the Music of J. S. Bach):
  //   - sarabande (3/4, slow; long notes begun on the second beat): the second beat leaned on;
  //   - minuet (3/4, moving): felt in one, the second and third beats light;
  //   - gavotte (duple, its phrases from the half bar): the half bar nearly a downbeat;
  //   - bourrée (duple, quick, a quarter's upbeat): in two, the half bar strong;
  //   - gigue (compound, quick): the lilt of each dotted beat's first note.
  enum Dance { kNoDance, kSarabande, kMinuet, kGavotte, kBourree, kGigue };
  static const char* const kDanceName[] = {"", "sarabande", "minuet", "gavotte", "bourrée", "gigue"};
  Dance dance = kNoDance;
  if (baroque && knob("PJ_DIRECTOR_DANCE", 1.0) > 0 && bars >= 8) {
    const double q = lenAt(in.tempo, 0);  // (a quarter, s)
    if (compound) {
      if (1.5 * q < 0.6) dance = kGigue;
    } else if (bar == 3 * div) {
      int beat2Long = 0, beat1Long = 0;
      for (const DirNote& x : nt) {
        if (x.tickOff - x.tick < 3 * div / 2) continue;
        beat2Long += x.tick % bar == div;
        beat1Long += x.tick % bar == 0;
      }
      if (q >= 0.5 && beat2Long >= bars / 5 && beat2Long * 3 >= beat1Long) dance = kSarabande;
      else if (q < 0.5 && q >= 0.25) dance = kMinuet;
    } else if (bar == 4 * div) {
      // (the melody's first note, and its notes after each general rest: where its phrases begin)
      std::map<long, int> startAt;
      long prevOff = -1;
      for (const DirNote& x : nt) {
        if (x.ch == melody && (prevOff < 0 || x.tick - prevOff >= div)) ++startAt[x.tick % bar];
        if (x.ch == melody) prevOff = std::max(prevOff, x.tickOff);
      }
      int total = 0;
      for (auto& [p, c] : startAt) total += c;
      if (total >= 3 && startAt[bar / 2] * 2 >= total) dance = kGavotte;
      else if (total >= 3 && startAt[3 * div] * 2 >= total && q < 0.4) dance = kBourree;
    }
  }
  auto weight = [&](long tick) {
    const long pos = tick % bar;
    if (pos == 0) return 1.0;
    if (compound) {
      const long dq = 3 * div / 2;
      if (pos % dq == 0) return bar == 4 * dq && pos == 2 * dq ? 0.6 : dance == kGigue ? 0.45 : 0.35;
      if (div % 2 == 0 && pos % (div / 2) == 0) return dance == kGigue ? 0.05 : 0.1;
      return 0.0;
    }
    if (dance == kSarabande && pos == div) return 0.75;
    if (dance == kMinuet && pos % div == 0) return 0.2;
    if ((dance == kGavotte || dance == kBourree) && pos == bar / 2) return 0.8;
    if ((bar / div) % 2 == 0 && pos == bar / 2) return 0.6;
    if (pos % div == 0) return 0.35;
    if (div % 2 == 0 && pos % (div / 2) == 0) return 0.1;
    return 0.0;
  };

  // --- Each part's notes in order: what follows each, what came before.
  std::vector<int> nextIn(n, -1), prevIn(n, -1);
  {
    std::vector<int> of[16];
    for (int i = 0; i < n; ++i) of[nt[i].ch].push_back(i);
    for (const std::vector<int>& v : of)
      for (size_t a = 0; a < v.size();) {
        size_t b = a;  // (a chord's notes share what follows and what came before)
        while (b < v.size() && nt[v[b]].tick == nt[v[a]].tick) ++b;
        for (size_t q = a; q < b; ++q) {
          nextIn[v[q]] = b < v.size() ? v[b] : -1;
          prevIn[v[q]] = a > 0 ? v[a - 1] : -1;
        }
        a = b;
      }
  }

  // --- The cues, and each part's swells (long notes of a line: not the bass's pedal notes, not
  // a chord's).
  struct Swell { long on, peak, off; float depth; };
  std::vector<Swell> swells[16];
  // A Baroque wind line speaks (PJ_DIRECTOR_SPEECH, 1; Quantz, Versuch 1752, VII, VIII, XI and
  // XIV 41; Hotteterre, Principes 1707): not every note alike, as an organ's, but each said as
  // a syllable of a sentence.
  //   - Good and bad notes: the note on the beat a little louder and tongued harder ("ti"), the
  //     ones between softer and lighter ("di"): the bar's own stress, note by note.
  //   - Each note relaxes once it has spoken (a breath-borne tone falls a little after its
  //     start); a long note is begun softly, swells and fades (messa di voce), deeper than a
  //     string's.
  //   - The sigh: a stressed note that falls by a step onto a weaker one (an appoggiatura or a
  //     Seufzer) is leaned on and slurred into its resolution, which is softer and lifted.
  //   - Breaths: a line running on without a rest breathes after PJ_DIRECTOR_BREATHE seconds
  //     (5) where a flutist would, after a note on the beat before a leap or after a longer
  //     note, the note shortened by up to 0.15 s; past 1.8 x that, at the next note on a beat.
  // Points on the channel's level curve (tick, offset), added to it below.
  // (A Classical reading too, at 0.6: Quantz's rules are the galant style's, and a Bach movement of
  // long lines is read as Classical by its rhythm alone.)
  const double kSpeech = kAll * knob("PJ_DIRECTOR_SPEECH", 1.0) * (baroque ? 1.0 : romantic ? 0.0 : 0.6);
  static const double kBreathe = knob("PJ_DIRECTOR_BREATHE", 5.0);
  struct Spoken { long on, off; float at, end; };
  std::vector<Spoken> spoken[16];
  std::vector<char> sighFrom(n, 0), sighTo(n, 0);
  auto speaks = [&](int ch) { return in.wind[ch] && !in.ridden[ch]; };  // (its breaths; the rest x kSpeech)
  // A flute's register (PJ_WIND_LEAP, 1; with or without the speech): the higher notes speak with
  // faster air and come out louder by themselves; a flutist does not blow the highest note the
  // most, least of all in an inner or accompanying part. Under the part's median key, its notes
  // a little fuller; above, a little held back: -0.08 a unit of level an octave in a part under
  // the melody, -0.04 in the melody (to +-0.1). A note reached by a leap up of a third or more
  // is tongued softly and begun up to 0.06 below its level, grown into over the note.
  const double kLeap = kAll * knob("PJ_WIND_LEAP", 1.0);
  auto flutes = [&](int ch) { return kLeap > 0 && in.flute[ch] && !in.ridden[ch]; };
  int midKey[16];
  for (int ch = 0; ch < 16; ++ch) {
    std::vector<int> k;
    for (const DirNote& x : nt) if (x.ch == ch) k.push_back(x.key);
    midKey[ch] = k.empty() ? 60 : (std::nth_element(k.begin(), k.begin() + k.size() / 2, k.end()), k[k.size() / 2]);
  }
  auto register_ = [&](int ch, int key) {
    const double perOct = ch == melody ? 0.04 : 0.08;
    return kLeap * std::clamp(-perOct * (key - midKey[ch]) / 12.0, -0.1, 0.1);
  };
  for (int i = 0; i < n; ++i) {  // (the sighs, first: a note needs to know it resolves one)
    const DirNote& x = nt[i];
    const int nx = nextIn[i];
    if (kSpeech <= 0 || !speaks(x.ch) || nx < 0 || nt[nx].tick > x.tickOff + div / 16) continue;
    const int step = x.key - nt[nx].key;
    const long dur = x.tickOff - x.tick, ndur = nt[nx].tickOff - nt[nx].tick;
    if (step >= 1 && step <= 2 && weight(x.tick) >= 0.35 && weight(nt[nx].tick) < weight(x.tick) && ndur <= dur &&
        dur >= div && phraseOf(nt[nx].tick) == phraseOf(x.tick)) {  // (a quarter at least: not passagework)
      sighFrom[i] = 1;
      sighTo[nx] = 1;
    }
  }
  // A wind's trill: two keys a step apart, alternating at least four notes under 0.18 s each.
  //   - A flute's (PJ_FLUTE_TAKA, 1) double-tongued "ta-ka-ta-ka": every note tongued (not
  //     slurred), the "ta"s a little firmer, the "ka"s (the back of the tongue) softer and a
  //     shade shorter.
  //   - Any wind's played alive (PJ_TRILL_QUICKEN, 0.2; Baroque, a quantized file): begun slower
  //     and quickening to its end, its start and end kept (the rate from 1 - 0.2 to 1 + 0.2), as
  //     the organ's (PJ_ORGAN_TRILL). Begun on the upper note (the Baroque trill, from above, on
  //     the beat): that note leaned on, an appoggiatura's start.
  // (taka[i]: 1 a "ta", 2 a "ka", 0 not; trillAt[i]: its place in the trill from 1, 0 not in one)
  static const double kTaka = knob("PJ_FLUTE_TAKA", 1.0);
  const double kQuicken = kAll * knob("PJ_TRILL_QUICKEN", 0.2) * (baroque ? 1.0 : 0.5);
  std::vector<char> taka(n, 0);
  std::vector<int> trillAt(n, 0);
  std::vector<std::vector<int>> trillRuns;
  int takaTrills = 0;
  for (int i = 0; i < n; ++i) {
    if (trillAt[i] || !(speaks(nt[i].ch) || flutes(nt[i].ch) || (d.acts && in.pluck[nt[i].ch])) || (prevIn[i] >= 0 && trillAt[prevIn[i]])) continue;
    auto quick = [&](int q) {
      const long d = nt[q].tickOff - nt[q].tick;
      return d <= div / 2 && double(d) / div * lenAt(in.tempo, double(nt[q].tick) / div) < 0.18;
    };
    std::vector<int> run{i};
    for (int q = i; quick(q);) {
      const int nx = nextIn[q];
      if (nx < 0 || nt[nx].tick > nt[q].tickOff + div / 16) break;
      const int step = std::abs(nt[nx].key - nt[q].key);
      if (step < 1 || step > 2 || (run.size() >= 2 && nt[nx].key != nt[run[run.size() - 2]].key)) break;
      run.push_back(nx);
      q = nx;
    }
    if (run.size() >= 2 && !quick(run.back())) run.pop_back();  // (the trill's end note held: not tongued in it)
    if (run.size() < 4) continue;
    for (size_t k = 0; k < run.size(); ++k) trillAt[run[k]] = int(k) + 1;
    trillRuns.push_back(run);
    if (kTaka > 0 && flutes(nt[i].ch)) {
      ++takaTrills;
      for (size_t k = 0; k < run.size(); ++k) taka[run[k]] = k % 2 ? 2 : 1;
    }
  }
  // A flute's fast run (PJ_DIRECTOR_TONGUE, 1): three notes or more, each under 0.2 s and
  // joined (a gap under a 32nd), tongued double or triple: the note's place in its group of two
  // (ta-ka) or of three (a triplet's, a compound beat's eighths), counted from the beat's grid
  // (the run's own note value), so a pair's back note falls off the beat. The trills' ta-ka too.
  // (tongue[i]: kCueTongue's value)
  static const double kTongue = knob("PJ_DIRECTOR_TONGUE", 1.0);
  std::vector<char> tongue(n, 0);
  int tongueRuns = 0;
  if (kTongue > 0)
    for (int i = 0; i < n; ++i) {
      const int ch = nt[i].ch;
      if (!in.flute[ch] || tongue[i]) continue;
      auto ioi = [&](int q) { const int nx = nextIn[q]; return nx < 0 ? -1L : nt[nx].tick - nt[q].tick; };
      auto quick = [&](int q) {
        const long g = ioi(q);
        return g > 0 && g <= div / 2 && nt[nextIn[q]].tick <= nt[q].tickOff + div / 8 &&
               double(g) / div * lenAt(in.tempo, double(nt[q].tick) / div) < 0.2;
      };
      std::vector<int> run{i};
      for (int q = i; quick(q) && run.size() < 4096;) run.push_back(q = nextIn[q]);
      if (run.size() < 3) continue;
      std::vector<long> gs;
      for (size_t k = 0; k + 1 < run.size(); ++k) gs.push_back(ioi(run[k]));
      std::nth_element(gs.begin(), gs.begin() + gs.size() / 2, gs.end());
      const long g = std::max(1L, gs[gs.size() / 2]);
      const double per = double(div) / g;  // (notes a quarter)
      auto near = [&](double x) { return std::abs(per - x) < 0.12 * x; };
      const bool three = near(3) || near(6) || near(1.5) || (compound && (near(2) || near(4)));
      const long group = three ? 3 * g : 2 * g;
      ++tongueRuns;
      for (int q : run) {
        const int pos = int(std::lround(double(nt[q].tick % group) / g)) % (three ? 3 : 2);
        tongue[q] = char(three ? 3 + pos : 1 + pos);
      }
    }
  for (int i = 0; i < n; ++i)
    if (taka[i]) tongue[i] = taka[i];
  // (the time each trill's notes are moved by, onShift/offShift, filled in below with the
  // inequality's: per input note, in ticks)
  d.onShift.assign(n, 0);
  d.offShift.assign(n, 0);
  int quickened = 0;
  // The kinds of trill (PJ_TRILL_SHAPE, 1; the players rehearse each: tools/slurs.py trillways):
  //   - a short one (under 8 notes): steady in level, quickening from its start as above;
  //   - a long one: steady at first, then quickening towards its end; and in level
  //       linked on into a higher note: growing into it (a crescendo);
  //       linked on otherwise (a cadence's trill): an emphasis three quarters of the way in;
  //       the last of its phrase: dying away (a decrescendo);
  //       else: an emphasis half-way.
  // (trillDyn[i]: the note's level offset; the shapes ~ +-0.05 of the dynamics' 0..1)
  static const double kTrillShape = knob("PJ_TRILL_SHAPE", 1.0);
  std::vector<float> trillDyn(n, 0.0f);
  int trillKinds[5] = {};  // steady, growing, late emphasis, dying away, emphasis half-way
  for (const std::vector<int>& run : trillRuns) {
    const long t0 = nt[run.front()].tick, D = nt[run.back()].tickOff - t0;
    if (D <= 0) continue;
    const bool isLong = run.size() >= 8 && kTrillShape > 0;
    const int after = nextIn[run.back()];
    const bool linked = after >= 0 && nt[after].tick <= nt[run.back()].tickOff + div / 8;
    // (past the trill's held end note, if it has one: the note the phrase goes on to)
    const int main = std::min(nt[run[0]].key, nt[run[1]].key);
    int kind = 0;
    if (isLong) kind = linked ? (nt[after].key > main + 2 ? 1 : 2) : 3;
    if (isLong && !linked && after >= 0 && nt[after].tick <= nt[run.back()].tickOff + 2 * div) kind = 4;
    ++trillKinds[kind];
    if (kind > 0) {
      const double depth = 0.05 * kTrillShape * kAll;
      for (int q : run) {
        const double u = std::clamp(double(nt[q].tick - t0) / double(D), 0.0, 1.0);
        auto peak = [u](double at) { return std::max(0.0, 1 - std::fabs(u - at) / (u < at ? at : 1 - at)); };
        const double v = kind == 1 ? 2 * u - 1 : kind == 2 ? 1.5 * peak(0.75) - 0.5 : kind == 3 ? 0.6 - 1.6 * u : 1.5 * peak(0.5) - 0.5;
        trillDyn[q] = float(depth * v);
      }
    }
    if (!(kQuicken > 0 && (d.acts || d.gridded))) continue;
    // (a long one: the first 40 % steady, the slower rate; then quickening, more so)
    const double u0 = isLong ? 0.4 : 0.0, k = isLong ? 1.6 * kQuicken : kQuicken;
    auto g = [&](long t) {
      const double u = std::clamp(double(t - t0) / double(D), 0.0, 1.0);
      const double c1 = 1 + k * (1 - u0) * (1 - u0), v = std::max(0.0, u - u0);
      return t0 + long(std::lround(double(D) * (c1 * u - k * v * v)));
    };
    for (int q : run) {
      d.onShift[q] = g(nt[q].tick) - nt[q].tick;
      d.offShift[q] = g(nt[q].tickOff) - nt[q].tickOff;
    }
    ++quickened;
  }
  // --- Notes inégales (-style french, or PJ_INEGALES 1 in any Baroque reading; Hotteterre 1719,
  // Couperin, Quantz XI.12): in French music the running eighths by step are played unequal,
  // long-short, the first of each pair taking PJ_INEGALES_RATIO of the two (0.6; up to 2:1), every
  // part alike. Not in a compound metre, not across a leap, not a pair broken by a rest; the pair
  // on the beat, the second note the eighth after it.
  const bool inegal = baroque && !compound && (gFrench || knob("PJ_INEGALES", 0.0) > 0) && div % 2 == 0;
  int unequal = 0;
  if (inegal && (d.acts || d.gridded)) {
    const double r = std::clamp(knob("PJ_INEGALES_RATIO", 0.6), 0.5, 0.67);
    const long u = div / 2, sh = long(std::lround((2 * r - 1) * u));
    for (int i = 0; i < n; ++i) {
      const DirNote& x = nt[i];
      const int nx = nextIn[i];
      if (x.tick % div != 0 || nx < 0 || trillAt[i] || trillAt[nx]) continue;
      if (std::abs(x.tickOff - x.tick - u) > u / 8 || nt[nx].tick != x.tick + u || std::abs(nt[nx].tickOff - nt[nx].tick - u) > u / 8) continue;
      if (std::abs(nt[nx].key - x.key) > 2 || x.tickOff < nt[nx].tick - u / 8) continue;
      d.offShift[i] += sh;
      d.onShift[nx] += sh;
      ++unequal;
    }
  }
  // A flute's three repeated notes beginning an accompanying passage (PJ_FLUTE_REPEAT, 1; Baroque
  // x1, Classical x0.7, Romantic x0.3; Quantz's good and bad notes, the soft "di-ri" tonguing): a
  // part under the melody, after a rest or at a phrase's start, the same key three times, the
  // fourth another or none. Not three alike, but shaped by where the beat falls, portato (a hint
  // of air between them), the accompaniment kept under the soloist:
  //   - the first on the beat: DA-ra-da, leaned on, then lighter and lighter;
  //   - the third on the beat: ri-ti-DA, a pickup: light, warmer, arriving;
  //   - the second on the beat: da-DA-da;
  //   - none on a beat: ti-RI-ti, sneaking in feather-light (no new downbeat), a little warmth
  //     in the middle, released.
  // (repShape[i]: the note's accent, level and length; repAt[i]: its place in the three, 1..3)
  const double kRepN = kAll * knob("PJ_FLUTE_REPEAT", 1.0) * (baroque ? 1.0 : romantic ? 0.3 : 0.7);
  struct RepShape { float acc, lvl, len; };
  std::vector<RepShape> repShape(n, RepShape{0, 0, 1});
  std::vector<char> repAt(n, 0);
  int repeats = 0;
  if (kRepN > 0)
    for (int i = 0; i < n; ++i) {
      const int ch = nt[i].ch, pv = prevIn[i];
      if (!flutes(ch) || ch == melody || taka[i] || repAt[i]) continue;
      const bool starts = pv < 0 || nt[i].tick - nt[pv].tickOff >= div / 2 || phraseOf(nt[pv].tick) != phraseOf(nt[i].tick);
      if (!starts) continue;
      int r[3] = {i, nextIn[i], -1};
      if (r[1] < 0) continue;
      r[2] = nextIn[r[1]];
      if (r[2] < 0) continue;
      bool ok = true;
      for (int k = 1; k < 3 && ok; ++k)
        ok = nt[r[k]].key == nt[i].key && nt[r[k]].tick - nt[r[k - 1]].tickOff < div / 2 && nt[r[k]].tick - nt[r[k - 1]].tick <= 2 * div;
      const int r4 = nextIn[r[2]];
      if (!ok || (r4 >= 0 && nt[r4].key == nt[i].key && nt[r4].tick - nt[r[2]].tickOff < div / 2)) continue;
      const double w1 = weight(nt[r[0]].tick), w2 = weight(nt[r[1]].tick), w3 = weight(nt[r[2]].tick);
      static const RepShape kDown[3] = {{0.15f, 0.02f, 0.88f}, {-0.1f, -0.01f, 0.82f}, {-0.2f, -0.03f, 0.78f}};
      static const RepShape kUp[3] = {{-0.15f, -0.03f, 0.78f}, {-0.05f, -0.01f, 0.82f}, {0.15f, 0.02f, 0.9f}};
      static const RepShape kMid[3] = {{-0.1f, -0.02f, 0.8f}, {0.15f, 0.015f, 0.86f}, {-0.1f, -0.02f, 0.78f}};
      static const RepShape kOff[3] = {{-0.25f, -0.035f, 0.8f}, {0.05f, 0.0f, 0.85f}, {-0.12f, -0.02f, 0.76f}};
      const RepShape* sh = w1 >= 0.35 && w1 >= w2 && w1 >= w3 ? kDown
                           : w3 >= 0.35 && w3 > w1 && w3 >= w2 ? kUp
                           : w2 >= 0.35 && w2 > w1 && w2 > w3 ? kMid : kOff;
      ++repeats;
      for (int k = 0; k < 3; ++k) {
        repAt[r[k]] = char(k + 1);
        repShape[r[k]] = {float(kRepN * sh[k].acc), float(kRepN * sh[k].lvl), float(1 + kRepN * (sh[k].len - 1))};
      }
    }
  // --- Dissonance and resolution (PJ_DIRECTOR_HARMONY, 1; Quantz XVII.vi.12-14, C. P. E. Bach):
  // a dissonance struck on a beat (an appoggiatura, a suspension, a seventh against the bass) is
  // played a little louder and leaned on, the sharper the dissonance the more; its resolution, a
  // step down onto a consonance, is softer and lifted. Each note against the notes of the other
  // parts sounding when it begins (and against the lowest of them, the bass): a minor second or
  // major seventh 1, a tritone 0.8, a major second or minor seventh 0.6, a fourth over the bass
  // 0.4. Off the beat (passing notes) nothing. All the parts' accents; the speaking winds' level too.
  const double kHarm = kAll * knob("PJ_DIRECTOR_HARMONY", 1.0) * (baroque ? 1.0 : romantic ? 0.6 : 0.8);
  std::vector<float> diss(n, 0.0f);
  std::vector<char> resolves(n, 0);
  int dissonant = 0, resolved = 0;
  if (kHarm > 0) {
    long longest = 0;
    for (const DirNote& x : nt) longest = std::max(longest, x.tickOff - x.tick);
    auto score = [&](int i) {
      const DirNote& x = nt[i];
      int low = x.key;
      double sc = 0;
      std::vector<int> other;
      for (int j = i - 1; j >= 0 && x.tick - nt[j].tick <= longest; --j)
        if (nt[j].ch != x.ch && nt[j].tickOff > x.tick) other.push_back(nt[j].key);
      for (int j = i + 1; j < n && nt[j].tick == x.tick; ++j)
        if (nt[j].ch != x.ch) other.push_back(nt[j].key);
      for (int k : other) low = std::min(low, k);
      for (int k : other) {
        const int iv = ((x.key - k) % 12 + 12) % 12;
        sc = std::max(sc, iv == 1 || iv == 11 ? 1.0 : iv == 6 ? 0.8 : iv == 2 || iv == 10 ? 0.6 : 0.0);
      }
      if (low < x.key && (x.key - low) % 12 == 5) sc = std::max(sc, 0.4);
      return sc;
    };
    for (int i = 0; i < n; ++i) {
      if (weight(nt[i].tick) < 0.35) continue;
      const double sc = score(i);
      const int nx = nextIn[i];
      // (a dissonance that resolves: it falls by a step, soon, to something clearly less
      // dissonant; the note it clashes with, which does not, is not the dissonance)
      const bool res = nx >= 0 && nt[i].key - nt[nx].key >= 1 && nt[i].key - nt[nx].key <= 2 &&
                       nt[nx].tick - nt[i].tickOff < div / 2 && score(nx) < sc - 0.3;
      if (sc < 0.4 || !res) continue;
      diss[i] = float(kHarm * sc);
      ++dissonant;
      if (res) { resolves[nx] = 1; ++resolved; }
    }
  }
  // --- The harpsichord's chords rolled (PJ_HPSI_ROLL, 1; the director acting): a chord struck at
  // once on a plucked keyboard is a thud. Three notes or more begun together on its channels,
  // from the bass up:
  //   - in the music's flow, a quick roll (8 ms a note, to 40 ms) ending on the beat: the top note,
  //     the melody's, where it is written, the others just before it;
  //   - a chord that opens or closes a phrase, or is held two beats or more, broadly, from the
  //     beat (35 ms a note, to 180 ms; slower in a slow piece): the pillar arpeggio.
  static const double kRoll = knob("PJ_HPSI_ROLL", 1.0);
  int rolled = 0, broad = 0;
  if (d.acts && kRoll > 0 && baroque)
    for (int i = 0; i < n;) {
      int j = i;
      while (j < n && nt[j].tick == nt[i].tick) ++j;
      std::vector<int> ch;
      for (int q = i; q < j; ++q)
        if (in.pluck[nt[q].ch] && !trillAt[q]) ch.push_back(q);
      if (ch.size() >= 3) {
        std::sort(ch.begin(), ch.end(), [&](int a, int b) { return nt[a].key < nt[b].key; });
        const long t = nt[i].tick;
        long longest = 0;
        for (int q : ch) longest = std::max(longest, nt[q].tickOff - t);
        const double q1 = lenAt(in.tempo, double(t) / div);  // (a quarter, s)
        const int p = phraseOf(t);
        const bool pillar = t == d.phrases[p].first || longest >= 2 * div || t + longest >= d.phrases[p].second - div / 4;
        const double per = kRoll * (pillar ? std::min(0.035 * (q1 > 0.75 ? 1.4 : 1.0), 0.18 / (ch.size() - 1)) : std::min(0.008, 0.04 / (ch.size() - 1)));
        const long step = long(std::lround(per / q1 * div));
        for (size_t k = 0; k < ch.size(); ++k)
          d.onShift[ch[k]] += pillar ? long(k) * step : -long(ch.size() - 1 - k) * step;
        ++rolled;
        broad += pillar;
      }
      i = j;
    }
  int breaths = 0, leapsUp = 0, hpsiHeld = 0;
  long sinceBreath[16];
  std::fill(sinceBreath, sinceBreath + 16, -1L);
  const double vibBase = baroque ? 0.5 : romantic ? 1.0 : 0.8;
  for (int i = 0; i < n; ++i) {
    const DirNote& x = nt[i];
    Cue& c = d.cues[i];
    const long dur = std::max(1L, x.tickOff - x.tick);
    const double secs = double(dur) / div * lenAt(in.tempo, double(x.tick) / div);
    const int p = phraseOf(x.tick), nx = nextIn[i], pv = prevIn[i];
    const double w = weight(x.tick);
    bool chord = false;
    for (int q = i - 1; q >= 0 && nt[q].tick == x.tick && !chord; --q) chord = nt[q].ch == x.ch;
    for (int q = i + 1; q < n && nt[q].tick == x.tick && !chord; ++q) chord = nt[q].ch == x.ch;
    const bool gapless = nx >= 0 && nt[nx].tick <= x.tickOff + div / 16;
    const bool lastOfPhrase = nx < 0 || phraseOf(nt[nx].tick) != p;
    // Emphasis.
    double acc = (w - 0.3) * (baroque ? 0.25 : 0.15);
    const long strong = (x.tick / div + 1) * div;
    if (w < 0.35 && strong < x.tickOff - div / 8 && weight(strong) > w + 0.2) acc += 0.3;  // a syncopation
    if (w >= 0.6 && (pv < 0 || x.tick - nt[pv].tickOff >= div / 2)) acc += 0.15;           // after a rest
    if (i == climax[p]) acc += baroque ? 0.3 : 0.5;
    c.v[kCueAccent] = float(std::clamp(acc * kAcc, -1.0, 1.0));
    // Articulation.
    double len = 1;
    if (baroque && dur <= div && gapless && !lastOfPhrase)
      len = std::abs(nt[nx].key - x.key) > 2 || weight(nt[nx].tick) >= 0.6 ? 0.80 : 0.93;
    if (lastOfPhrase && gapless) {  // the breath: ~0.12 s, at most 17 % of the note
      const double breath = 0.12 / lenAt(in.tempo, double(x.tickOff) / div) * div;
      len = 1 - std::min(0.17 * dur, breath) / dur;
    }
    if (!lastOfPhrase && dur <= 2 * div) len *= keying(x.tick);
    // (a wind line's speech: its sighs and its breaths)
    const bool speech = speaks(x.ch) && !chord;
    bool breathe = false;
    if (speech) {
      if (sighFrom[i]) { len = 1; acc += 0.25; }  // (leaned on, slurred into its resolution)
      if (sighTo[i]) { len = std::min(len, 0.75); acc -= 0.2; }  // (the resolution: softer, lifted)
      // The breath: the time since the line last had air (a rest, a phrase's end, a breath).
      if (sinceBreath[x.ch] < 0 || pv < 0 || x.tick - nt[pv].tickOff >= div / 8) sinceBreath[x.ch] = x.tick;
      const double ran = double(x.tickOff - sinceBreath[x.ch]) / div * lenAt(in.tempo, double(x.tick) / div);
      if (lastOfPhrase || !gapless) {
        sinceBreath[x.ch] = -1;
      } else if (ran >= kBreathe && !sighFrom[i]) {
        const long ndur = nt[nx].tickOff - nt[nx].tick;
        const bool good = w >= 0.35 && (std::abs(nt[nx].key - x.key) > 2 || dur > ndur);
        breathe = good || (ran >= 1.8 * kBreathe && weight(nt[nx].tick) >= 0.35);
        if (breathe) {
          ++breaths;
          const double b = 0.15 / lenAt(in.tempo, double(x.tickOff) / div) * div;
          len = std::min(len, 1 - std::min(0.4 * dur, b) / dur);
          sinceBreath[x.ch] = -1;
        }
      }
    }
    // (a flute's leap up: tongued softly, "dah", not "tah": the top note eased into)
    const int leapUp = flutes(x.ch) && !chord && pv >= 0 && x.tick - nt[pv].tickOff < div / 2 ? x.key - nt[pv].key : 0;
    leapsUp += leapUp >= 3;
    if (leapUp >= 3) acc -= kLeap * std::min(0.25, 0.04 * (leapUp - 2));
    if (trillAt[i] == 1 && nextIn[i] >= 0 && nt[nextIn[i]].key < x.key) acc += 0.15 * kQuicken / 0.2;  // (from above: leaned on)
    if (taka[i]) {  // (ta-ka: tongued through, the ka softer and a shade shorter)
      acc += kTaka * (taka[i] == 1 ? 0.08 : -0.1);
      len = std::min(len, taka[i] == 1 ? 0.9 : 0.82);
    }
    // A repeated note ends the legato (PJ_DIRECTOR_REPEATLIFT, 1): a line that comes back to the
    // key it is on asks the slur to stop there. The note before the repeat is let go, not cut at
    // the last instant: a lift of 15 % of it, at most 70 ms, and never slurred into the next.
    static const double kRepLift = knob("PJ_DIRECTOR_REPEATLIFT", 1.0);
    const bool repeatNext = !chord && nx >= 0 && nt[nx].key == x.key && gapless && !trillAt[i];
    if (repeatNext && kRepLift > 0) {
      const double lift = std::min(0.15 * dur, 0.07 / std::max(1e-3, lenAt(in.tempo, double(x.tickOff) / div)) * div);
      len = std::min(len, 1 - kRepLift * lift / dur);
    }
    // The harpsichord's touch (PJ_HPSI_TOUCH, 1; Baroque, the director acting): its loudness is
    // time. A little daylight between notes (speech's consonants), the strong beats fuller and
    // held, the weak ones lighter and more detached; a dissonance held its whole value, its
    // resolution let go early; a broken chord's tones held on past their value (over-legato: the
    // harmony blooming in the case, the fingers its pedal), to the half bar, never past the same
    // key's next note.
    static const double kTouch = knob("PJ_HPSI_TOUCH", 1.0);
    if (d.acts && baroque && in.pluck[x.ch] && kTouch > 0 && !chord && !trillAt[i]) {
      if (gapless && !lastOfPhrase && dur <= 2 * div)
        len = std::min(len, 1 - kTouch * (w >= 0.6 ? 0.03 : w >= 0.35 ? 0.07 : 0.13));
      if (diss[i] > 0) len = std::max(len, 1.0);
      if (resolves[i]) len = std::min(len, 1 - kTouch * 0.15);
      const int nx2 = nextIn[i];
      if (nx2 >= 0 && dur <= div && gapless) {
        const int leap = std::abs(nt[nx2].key - x.key);
        if (leap >= 3 && leap <= 9 && leap != 6) {
          long until = (x.tick / (bar / 2) + 1) * (bar / 2);  // (to the half bar)
          for (int q = nx2; q >= 0 && nt[q].tick < until; q = nextIn[q])
            if (nt[q].key == x.key) { until = nt[q].tick - div / 32; break; }
          const double over = double(until - x.tick) / dur;
          if (over > 1.05) { len = 1 + kTouch * (std::min(over, 4.0) - 1); ++hpsiHeld; }
        }
      }
    }
    const bool harm = diss[i] > 0 || resolves[i];
    if (diss[i] > 0) acc += 0.3 * diss[i];  // (the dissonance leaned on; its resolution softer)
    if (resolves[i]) acc -= 0.2 * kHarm;
    if (repAt[i]) { acc += repShape[i].acc; len = std::min(len, double(repShape[i].len)); }  // (portato, shaped)
    if (speech || leapUp >= 3 || trillAt[i] || repAt[i] || harm) c.v[kCueAccent] = float(std::clamp(acc * kAcc, -1.0, 1.0));
    c.v[kCueLength] = float(1 + (len - 1) * std::min(1.0, kAll));
    c.v[kCueLegato] = gapless && !lastOfPhrase && !breathe && !taka[i] && !repAt[i] && !repeatNext && len > 0.9 ? 1.0f : 0.0f;
    c.v[kCuePhraseEnd] = lastOfPhrase ? (p == np - 1 ? 2.0f : 1.0f) : 0.0f;
    c.v[kCueTongue] = float(tongue[i]);
    c.v[kCueMute] = muted[p] ? 1.0f : 0.0f;
    // Vibrato: none on Baroque passagework, an ornament on its long notes; light in the
    // Classical; always there in the Romantic; widest at the climax, wider when louder.
    double vib = 0, delay = 0.2;
    if (baroque) { vib = dur >= div && secs >= 0.5 ? vibBase : 0; delay = 0.35; }
    else if (romantic) { vib = secs >= 0.25 ? vibBase : 0.3; delay = 0.12; }
    else { vib = secs >= 0.3 ? vibBase : 0; delay = 0.2; }
    if (i == climax[p]) vib *= 1.3;
    if (lastOfPhrase) vib *= 0.7;
    c.v[kCueVibDepth] = float(1 + (vib - 1) * std::min(1.0, kAll * kVib));
    c.v[kCueVibDelay] = float(delay);
    c.v[kCueVibRate] = float(1 + (romantic ? 0.04 : 0.0) + 0.03 * c.v[kCueAccent]);
    // A long note of a line swells and fades.
    // (a wind line's notes: the bar's stress and each note's own shape)
    if (speech || flutes(x.ch)) {
      const double g = w >= 1 ? 0.05 : w >= 0.6 ? 0.035 : w >= 0.35 ? 0.01 : w >= 0.1 ? -0.03 : -0.045;
      const double lean = sighFrom[i] ? 0.05 : sighTo[i] ? -0.05 : 0.0;
      const double at = speech ? kSpeech * (g + lean) : 0.0;
      const double relax = speech && secs >= 0.2 ? kSpeech * std::min(0.05, 0.03 + 0.02 * secs) : 0.0;
      // (a flute's high notes not its loudest: blown under the register's own rise; the leap's
      // top note begun a little below and grown into)
      const double reg = flutes(x.ch) ? register_(x.ch, x.key) : 0.0;
      const double tk = taka[i] ? kTaka * (taka[i] == 1 ? 0.01 : -0.01) : 0.0;
      const double rp = repShape[i].lvl + 0.04 * diss[i] - (resolves[i] ? 0.03 * kHarm : 0.0);
      const double dip = leapUp >= 3 ? kLeap * std::min(0.06, 0.012 * (leapUp - 2)) : 0.0;
      spoken[x.ch].push_back({x.tick, x.tickOff, float(at + reg + tk + rp - dip), float(at - relax + reg + tk + rp)});
    }
    const bool longSpoken = speech && dur >= div && secs >= 0.6;
    if (!chord && x.ch != bass && (longSpoken || (dur >= 3 * div / 2 && secs >= 0.8)) && !in.ridden[x.ch]) {
      const float depth = float(kDyn * (longSpoken ? 0.12 * kSpeech : baroque ? 0.06 : romantic ? 0.16 : 0.10));
      const float peak = lastOfPhrase ? 0.35f : 0.55f;
      c.v[kCueSwell] = depth;
      c.v[kCueSwellPeak] = peak;
      swells[x.ch].push_back({x.tick, x.tick + long(peak * dur), x.tickOff, depth});
    }
  }

  // --- A wind line's own shape (PJ_WIND_ARC, 1; Baroque x0.5, Classical x0.8, Romantic x1): each
  // phrase rises to the line's highest note in it and falls after (the melody's own charge), and
  // the whole follows the piece's drama (PJ_WIND_DRAMA, 1): how busy all the parts are just now
  // (onsets a second, over a bar each side) against the piece's median: busier, fuller and more
  // urgent (up to +0.07); calmer, more intimate (down to -0.07).
  const double kArc = kAll * knob("PJ_WIND_ARC", 1.0) * (baroque ? 0.5 : romantic ? 1.0 : 0.8);
  const double kDrama = kAll * knob("PJ_WIND_DRAMA", 1.0);
  std::vector<long> allOn;
  for (const DirNote& x : nt) allOn.push_back(x.tick);  // (sorted: nt is)
  auto activity = [&](long t) {
    const long lo = t - bar, hi = t + bar;
    const long c = long(std::upper_bound(allOn.begin(), allOn.end(), hi) - std::lower_bound(allOn.begin(), allOn.end(), lo));
    return double(c) / std::max(1e-3, double(hi - lo) / div * lenAt(in.tempo, double(t) / div));
  };
  double medAct = 1;
  {
    std::vector<double> a;
    for (long b = 0; b < lastOff; b += bar) a.push_back(activity(b));
    if (!a.empty()) { std::nth_element(a.begin(), a.begin() + a.size() / 2, a.end()); medAct = std::max(0.1, a[a.size() / 2]); }
  }
  auto drama = [&](long t) {
    return float(kDrama * 0.07 * std::clamp(std::log2(std::max(1e-3, activity(t)) / medAct), -1.5, 1.5) / 1.5);
  };
  std::vector<long> peakOf[16];  // [ch][phrase]: its highest note's tick (-1: silent there)
  for (int ch = 0; ch < 16; ++ch) {
    if (!speaks(ch)) continue;
    peakOf[ch].assign(np, -1);
    std::vector<int> top(np, -1);
    for (int i = 0; i < n; ++i)
      if (nt[i].ch == ch) {
        const int p = phraseOf(nt[i].tick);
        if (top[p] < 0 || nt[i].key > top[p]) { top[p] = nt[i].key; peakOf[ch][p] = nt[i].tick; }
      }
  }
  auto lineArc = [&](int ch, long t) {
    if (peakOf[ch].empty()) return 0.0f;
    const int p = phraseOf(t);
    const long s = d.phrases[p].first, e = d.phrases[p].second, pk = peakOf[ch][p];
    if (pk < 0) return 0.0f;
    double v;
    if (t <= pk) v = -0.03 + 0.09 * double(t - s) / double(std::max(1L, pk - s));
    else v = 0.06 - 0.11 * double(t - pk) / double(std::max(1L, e - pk));
    // (an accompanying flute part does not climb to its top note: a third of the arc)
    return float(kArc * (in.flute[ch] && ch != melody && kLeap > 0 ? 0.33 : 1.0) * std::clamp(v, -0.05, 0.06));
  };

  // --- A wind part's own velocities (a played file's; the owner: a higher velocity is "a stronger
  // note from the very beginning" or "belongs to a line of expansion"): read as a line and accents
  // (src/velline.h). The line goes into the part's level, gliding through the bound notes of a
  // crescendo (the breath, not a step and a harder tongue at each note); the key is played at
  // the part's usual velocity plus the note's accent. PJ_VELLINE (1; 0: the keys as written).
  // A unit of level is ~20 dB (the players' loudness, PJ_DIRECTOR_WINDDB 16, and their breath);
  // General MIDI's velocity curve is 33.3 dB a decade (tools/velcurve.py).
  struct LineNote { long on, off; float at, end; };
  std::vector<LineNote> velLine[16];
  static const double kVelLine = knob("PJ_VELLINE", 1.0);
  int lineParts = 0, lineGlides = 0, lineAccents = 0;
  if (self && kVelLine > 0) {
    d.keyVel.assign(n, 0);
    for (int ch = 0; ch < 16; ++ch) {
      if (!speaks(ch) || chNotes[ch] < 3) continue;
      std::vector<VelNote> part;
      std::vector<int> idx;
      for (int i = 0; i < n; ++i)
        if (nt[i].ch == ch) part.push_back({nt[i].tick, nt[i].tickOff, nt[i].vel}), idx.push_back(i);
      const std::vector<VelReading> rd = readVelocities(part, div);
      std::vector<float> lines;
      bool varied = false;
      for (const VelReading& r : rd) lines.push_back(r.line), varied |= r.accent != 0 || r.line != rd[0].line;
      if (!varied) continue;
      std::nth_element(lines.begin(), lines.begin() + lines.size() / 2, lines.end());
      const float ref = lines[lines.size() / 2];
      auto levelOf = [&](float v) { return float(kVelLine * std::clamp(33.3 * std::log10(std::max(1.0f, v) / ref) / 20, -0.4, 0.4)); };
      ++lineParts;
      for (size_t k = 0; k < rd.size(); ++k) {
        velLine[ch].push_back({part[k].tick, part[k].tickOff, levelOf(rd[k].line), levelOf(rd[k].lineEnd)});
        d.keyVel[idx[k]] = std::clamp(int(std::lround(ref + rd[k].accent)), 1, 127);
        lineGlides += rd[k].lineEnd != rd[k].line;
        lineAccents += rd[k].accent > 0;
      }
    }
  }

  // --- Each part's level: the arc, the melody a little over the others, its swells.
  for (int ch = 0; ch < 16; ++ch) {
    if (chNotes[ch] == 0 || in.ridden[ch] || (self && !speaks(ch))) continue;
    const float off = float(kDyn * (channels < 2 ? 0.0 : ch == melody ? 0.05 : -0.02));
    std::vector<long> at;
    for (auto& a : arc) at.push_back(a.first);
    for (const Swell& s : swells[ch]) { at.push_back(s.on); at.push_back(s.peak); at.push_back(s.off); }
    for (const Spoken& s : spoken[ch]) { at.push_back(s.on); at.push_back(std::max(s.on + 1, s.off - 1)); }
    for (const LineNote& s : velLine[ch]) { at.push_back(s.on); at.push_back(std::max(s.on + 1, s.off - 1)); }
    std::sort(at.begin(), at.end());
    at.erase(std::unique(at.begin(), at.end()), at.end());
    size_t k = 0, q = 0;
    // (a spoken note's offset at t: from its start to its end, held over the gap after it)
    auto said = [&](long t) {
      const std::vector<Spoken>& v = spoken[ch];
      while (q + 1 < v.size() && v[q + 1].on <= t) ++q;
      if (v.empty() || v[q].on > t) return 0.0f;
      const Spoken& s = v[q];
      const long e = std::max(s.on + 1, s.off - 1);
      return t >= e ? s.end : s.at + (s.end - s.at) * float(t - s.on) / float(e - s.on);
    };
    // (the file's own line at t: through the note sounding, held over the gap after it)
    size_t lq = 0;
    auto fileLine = [&](long t) {
      const std::vector<LineNote>& v = velLine[ch];
      while (lq + 1 < v.size() && v[lq + 1].on <= t) ++lq;
      if (v.empty() || v[lq].on > t) return v.empty() ? 0.0f : v[0].at;
      const LineNote& s = v[lq];
      const long e = std::max(s.on + 1, s.off - 1);
      return t >= e ? s.end : s.at + (s.end - s.at) * float(t - s.on) / float(e - s.on);
    };
    for (long t : at) {
      while (k < swells[ch].size() && swells[ch][k].off < t) ++k;
      float bump = 0;
      if (k < swells[ch].size() && swells[ch][k].on <= t) {
        const Swell& s = swells[ch][k];
        bump = t <= s.peak ? s.depth * float(t - s.on) / float(std::max(1L, s.peak - s.on))
                           : s.depth * (1 - 1.3f * float(t - s.peak) / float(std::max(1L, s.off - s.peak)));
      }
      // (a spoken long note begins below its level: the swell rises from softer)
      float soft = 0;
      if (!spoken[ch].empty() && k < swells[ch].size() && swells[ch][k].on == t) soft = -0.04f * float(kSpeech);
      const float own = speaks(ch) ? lineArc(ch, t) + drama(t) + fileLine(t) : 0.0f;
      d.level[ch].push_back({t, std::clamp(levelAt(arc, t) + off + bump + soft + said(t) + own, 0.05f, 0.95f)});
    }
  }
  for (int i = 0; i < n; ++i) {
    d.cues[i].v[kCueLevel] = std::clamp(levelAt(d.level[nt[i].ch], nt[i].tick) + trillDyn[i], 0.05f, 0.95f);
    d.cues[i].v[kCueLevelEnd] = std::clamp(levelAt(d.level[nt[i].ch], nt[i].tickOff) + trillDyn[i], 0.05f, 0.95f);
  }

  // (the winds' own reading ends here: the time, the mutes and the other parts are the file's)
  if (d.vibOnly) {
    d.mutes.clear();
    d.summary = outWhy + "\n  the vibrato planned for every part (only that: the rest is the file's)";
    return d;
  }
  if (self) {
    d.mutes.clear();
    int lines = 0, notes = 0, sighs = 0;
    for (int ch = 0; ch < 16; ++ch) { lines += !spoken[ch].empty(); notes += int(spoken[ch].size()); }
    for (char c : sighFrom) sighs += c;
    static const char* const kStyleS[] = {"unknown", "Baroque", "Classical", "Romantic"};
    char b2[300];
    std::snprintf(b2, sizeof b2, "\n  the winds read their own lines (%s%s): %d notes of %d lines, %d sighs, %d breaths, "
                  "each phrase to its high point, with the piece's drama",
                  kStyleS[d.style], gStyle >= 0 ? ", -style" : "", notes, lines, sighs, breaths);
    d.summary = outWhy + b2;
    if (quickened || dissonant || dance != kNoDance) {
      std::snprintf(b2, sizeof b2, "\n  %d trills played alive, %d dissonances leaned on, %d resolutions eased%s%s",
                    quickened, dissonant, resolved, dance != kNoDance ? ", a " : "", kDanceName[dance]);
      d.summary += b2;
    }
    if (trillKinds[1] + trillKinds[2] + trillKinds[3] + trillKinds[4] > 0) {
      std::snprintf(b2, sizeof b2, "\n  long trills: %d growing into the next note, %d with a late emphasis, %d dying away, %d with an "
                    "emphasis half-way (%d short ones steady)", trillKinds[1], trillKinds[2], trillKinds[3], trillKinds[4], trillKinds[0]);
      d.summary += b2;
    }
    if (takaTrills + leapsUp + repeats + tongueRuns > 0) {
      std::snprintf(b2, sizeof b2, "\n  flutes: %d leaps up eased into, %d trills tongued ta-ka, %d fast runs double- or triple-tongued, %d accompanying repeated-note starts shaped", leapsUp, takaTrills, tongueRuns, repeats);
      d.summary += b2;
    }
    if (lineParts > 0) {
      std::snprintf(b2, sizeof b2, "\n  their key velocities read as lines and accents (%d parts): %d notes growing or fading "
                    "through to the next, %d accents", lineParts, lineGlides, lineAccents);
      d.summary += b2;
    }
    return d;
  }

  // --- The time: fermatas (a long chord everyone begins together, then a general rest), the
  // breath before the piece's climax, the final ritardando and the last chord held.
  struct Stretch { double from, to, factor; };  // in quarters
  std::vector<Stretch> st;
  const double endBeat = double(lastOff) / div, lastBeat = double(lastOn) / div;
  {
    size_t i = 0;
    while (i < nt.size()) {
      size_t j = i;
      std::set<int> chs;
      long minOff = 1L << 60, maxOffC = 0;
      while (j < nt.size() && nt[j].tick == nt[i].tick) {
        chs.insert(nt[j].ch);
        minOff = std::min(minOff, nt[j].tickOff);
        maxOffC = std::max(maxOffC, nt[j].tickOff);
        ++j;
      }
      const long T = nt[i].tick;
      const bool together = int(chs.size()) >= std::max(1, (channels + 1) / 2) && (channels > 1 || j - i >= 2);
      if (together && T != lastOn && minOff - T >= 2 * div && minOff - T >= 2 * modeIoi && j < nt.size() &&
          nt[j].tick >= maxOffC + div / 2) {
        // (nothing else may sound through it)
        bool alone = true;
        for (size_t q = 0; q < i && alone; ++q) alone = nt[q].tickOff <= T + div / 8;
        if (alone) {
          const double b0 = double(T) / div, b1 = double(maxOffC) / div, b2 = double(nt[j].tick) / div;
          const double hold = 1 + (kHold * (baroque ? 0.94 : romantic ? 1.2 : 1.0) - 1) * kRit;
          st.push_back({b0 - 2, b0, 1 + 0.08 * kRit});
          st.push_back({b0, b1, hold});
          const double restSecs = (b2 - b1) * lenAt(in.tempo, b1);
          st.push_back({b1, b2, 1 + 0.35 * kRit / std::max(0.1, restSecs)});
          d.fermatas.push_back({T, hold});
        }
      }
      i = j;
    }
  }
  if (top >= 0 && climax[top] >= 0) {
    const double cb = double(nt[climax[top]].tick) / div;
    if (cb > 1 && cb < lastBeat - 1) st.push_back({cb - 0.5, cb, 1 + 0.08 * kRit});
  }
  const double barBeats = double(bar) / div;
  const double ritLen = std::max(2.0, std::min(lastBeat, bars >= 16 ? 2 * barBeats : barBeats));
  const double ritDepth = (baroque ? 0.14 : 0.22) * kRit;
  std::vector<double> edge = {0.0, endBeat + 8};
  for (const DirTempo& x : in.tempo) edge.push_back(x.beat);
  for (const Stretch& s : st) { edge.push_back(std::max(0.0, s.from)); edge.push_back(s.to); }
  for (double b = lastBeat - ritLen; b < lastBeat; b += 0.5) edge.push_back(std::max(0.0, b));
  edge.push_back(lastBeat);
  std::sort(edge.begin(), edge.end());
  double t = 0;
  for (size_t i = 0; i + 1 < edge.size(); ++i) {
    const double b0 = edge[i], b1 = edge[i + 1];
    if (b1 - b0 < 1e-9) continue;
    const double mid = 0.5 * (b0 + b1);
    double f = 1;
    for (const Stretch& s : st)
      if (mid >= s.from && mid < s.to) f *= s.factor;
    if (mid >= lastBeat - ritLen && mid < lastBeat) {
      const double u = (mid - (lastBeat - ritLen)) / ritLen;
      f *= 1 + ritDepth * u * u;
    } else if (mid >= lastBeat) {
      f *= 1 + (0.3 + ritDepth) * std::min(1.0, kRit);
    }
    const double len = lenAt(in.tempo, b0) * f;
    if (d.tempo.empty() || std::fabs(d.tempo.back().len - len) > 1e-12) d.tempo.push_back({t, b0, len});
    t += (b1 - b0) * len;
  }

  // --- What it decided.
  static const char* const kStyle[] = {"unknown", "Baroque", "Classical", "Romantic"};
  char buf[800];
  int nb = std::snprintf(buf, sizeof buf, "director: %s%s, %s; %.0f bars of %g quarters; %d phrases%s; melody on channel %d%s\n",
                         kStyle[d.style], gStyle >= 0 ? " (-style)" : "", baroque ? "terraced dynamics" : "hairpins to each phrase's climax", bars, barBeats, np,
                         echoes ? (", " + std::to_string(echoes) + " echoes").c_str() : "", melody + 1,
                         bass >= 0 ? (", bass on " + std::to_string(bass + 1)).c_str() : "");
  if (rolled || hpsiHeld)
    nb += std::snprintf(buf + nb, sizeof buf - nb, "  harpsichord: %d chords rolled (%d broadly, as pillars), %d broken-chord tones held on\n",
                        rolled, broad, hpsiHeld);
  if (quickened || unequal)
    nb += std::snprintf(buf + nb, sizeof buf - nb, "  %d trills played alive (slower to quicker), %d eighth pairs unequal (notes inegales)\n",
                        quickened, unequal);
  if (trillKinds[1] + trillKinds[2] + trillKinds[3] + trillKinds[4] > 0)
    nb += std::snprintf(buf + nb, sizeof buf - nb, "  long trills: %d growing into the next note, %d with a late emphasis, %d dying away, "
                        "%d with an emphasis half-way (%d short ones steady)\n",
                        trillKinds[1], trillKinds[2], trillKinds[3], trillKinds[4], trillKinds[0]);
  if (compound || dance != kNoDance || dissonant)
    nb += std::snprintf(buf + nb, sizeof buf - nb, "  %s%s%s%s; %d dissonances leaned on, %d resolutions eased\n",
                        compound ? "compound metre" : "simple metre", dance != kNoDance ? ", a " : "", kDanceName[dance],
                        dance != kNoDance ? "'s weights" : "", dissonant, resolved);
  if (top >= 0 && climax[top] >= 0)
    nb += std::snprintf(buf + nb, sizeof buf - nb, "  climax bar %ld; ", nt[climax[top]].tick / bar + 1);
  else
    nb += std::snprintf(buf + nb, sizeof buf - nb, "  ");
  for (auto& [tk, hold] : d.fermatas)
    if (nb < int(sizeof buf) - 80) nb += std::snprintf(buf + nb, sizeof buf - nb, "fermata bar %ld (x%.2f); ", tk / bar + 1, hold);
  nb += std::snprintf(buf + nb, sizeof buf - nb, "final rit. over %g quarters (-%.0f %%), the last chord held\n", ritLen,
                      100 * ritDepth / (1 + ritDepth));
  if (mutedN) nb += std::snprintf(buf + nb, sizeof buf - nb, "  mutes on in %d of the %d phrases\n", mutedN, np);
  {
    int lines = 0, notes = 0, sighs = 0;
    for (int ch = 0; ch < 16; ++ch) { lines += !spoken[ch].empty(); notes += int(spoken[ch].size()); }
    for (char c : sighFrom) sighs += c;
    if (notes)
      nb += std::snprintf(buf + nb, sizeof buf - nb, "  winds speak: %d notes of %d lines, good and bad notes, %d sighs, %d breaths\n",
                          notes, lines, sighs, breaths);
    if (takaTrills + leapsUp + repeats + tongueRuns > 0)
      nb += std::snprintf(buf + nb, sizeof buf - nb, "  flutes: %d leaps up eased into, %d trills tongued ta-ka, %d fast runs double- or triple-tongued, %d accompanying repeated-note starts shaped\n", leapsUp, takaTrills, tongueRuns, repeats);
  }
  if (repeated) nb += std::snprintf(buf + nb, sizeof buf - nb, "  %d repeats (a bar or more said again at once): keyed long / mid / short in turn\n", repeated);
  std::snprintf(buf + nb, sizeof buf - nb, "  vibrato: %s; articulation: %s  (%s)",
                baroque ? "an ornament, long notes only" : romantic ? "always, widest at the climaxes" : "light, on the longer notes",
                baroque ? "detached, leaps and before strong beats more" : "legato, a breath at each phrase end", why);
  d.summary = buf;
  return d;
}

}  // namespace pajupilli
