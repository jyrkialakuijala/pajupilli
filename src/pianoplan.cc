// A pianist's sustain pedal planned from the notes: src/pianoplan.h, research/pedaling.md.
#include "pianoplan.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace pajupilli {

namespace {

double knobOr(const char* name, double dflt) {
  const char* s = std::getenv(name);
  return s ? std::atof(s) : dflt;
}

// (a small seeded generator: the human timing's)
struct Rng {
  unsigned s;
  double uni() {
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return (s & 0xffffff) / double(0x1000000);
  }
  double gauss() {
    const double u = std::max(1e-9, uni()), v = uni();
    return std::sqrt(-2 * std::log(u)) * std::cos(6.283185307 * v);
  }
};

// One attack: the notes struck together (a chord as played, its spread up to 35 ms).
struct Attack {
  double t;
  double beat;
  long tick;
  std::vector<int> idx;   // into the notes, low to high
  int lo = 127, hi = 0;
  double loOff = 0;       // the lowest note's release
  bool lowRun = false;    // a note of a stepwise run in the low register
};

const char* kNames[12] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
std::string noteName(int p) { return std::string(kNames[p % 12]) + std::to_string(p / 12 - 1); }

enum Mode { kHarmony, kLinear, kDetached };
const char* kModeName[3] = {"by harmony", "linear (finger legato, the long chords touched)", "detached (dry)"};

}  // namespace

PianoPedalPlan planPedal(const std::vector<PianoNote>& in, double qpb, bool baroque, unsigned seed, const std::string& who) {
  PianoPedalPlan plan;
  if (in.empty()) return plan;
  std::vector<PianoNote> nt(in);
  std::sort(nt.begin(), nt.end(), [](const PianoNote& a, const PianoNote& b) { return a.on != b.on ? a.on < b.on : a.pitch < b.pitch; });
  if (qpb <= 0) qpb = 4;
  static const double kLag = knobOr("PJ_PIANIST_LAG", 1.0);  // (x the measured lag)
  static const double kBassAgain = knobOr("PJ_PIANIST_BASSAGAIN", 0.6);  // (s)
  static const double kUpShift = knobOr("PJ_PIANIST_UPSHIFT", 0.0);  // (x MAESTRO's release offsets)
  static const double kMinLeap = knobOr("PJ_PIANIST_MINLEAP", 0.4);  // (s)
  static const double kLagSpread = knobOr("PJ_PIANIST_LAGSPREAD", 0.45);
  static const double kRestrike = knobOr("PJ_PIANIST_RESTRIKE", 0.8);  // (beats)
  static const double kRestrikeS = knobOr("PJ_PIANIST_RESTRIKE_S", 0.0);  // (s)
  Rng rng{seed * 2654435761u + 12345u};
  for (int i = 0; i < 8; ++i) rng.uni();

  // --- The attacks.
  std::vector<Attack> at;
  for (int i = 0; i < int(nt.size()); ++i) {
    if (at.empty() || nt[i].on - at.back().t > 0.035) at.push_back({nt[i].on, nt[i].beat, nt[i].tick, {}});
    Attack& a = at.back();
    a.idx.push_back(i);
    if (nt[i].pitch < a.lo) { a.lo = nt[i].pitch; a.loOff = nt[i].off; }
    a.hi = std::max(a.hi, nt[i].pitch);
  }
  const int na = int(at.size());
  // The local beat (s a quarter), from the onsets' times against their beats.
  auto beatLen = [&](int k) {
    const int a = std::max(0, k - 8), b = std::min(na - 1, k + 8);
    const double db = at[b].beat - at[a].beat;
    return db > 0.5 ? std::clamp((at[b].t - at[a].t) / db, 0.15, 2.0) : 0.5;
  };
  // The fingers' state: every key up from `freeFrom[k]` on before attack k (the running latest release).
  std::vector<double> freeFrom(na, 0);
  {
    double last = -1e9;
    for (int k = 0; k < na; ++k) {
      freeFrom[k] = last;
      for (int i : at[k].idx) last = std::max(last, nt[i].off);
    }
  }
  // Low stepwise runs: four or more single notes below middle C, a second apart, under 0.3 s each.
  for (int k = 0; k < na;) {
    int e = k;
    while (e + 1 < na && at[e].idx.size() == 1 && at[e + 1].idx.size() == 1 && at[e].lo < 60 && at[e + 1].lo < 60 &&
           std::abs(at[e + 1].lo - at[e].lo) <= 2 && at[e + 1].lo != at[e].lo && at[e + 1].t - at[e].t < 0.3)
      ++e;
    if (e - k >= 3)
      for (int j = k; j <= e; ++j) at[j].lowRun = true;
    k = e + 1;
  }

  // --- The texture, by four-bar sections (§7 R2; §3.1, P7, P8).
  const double secQ = 4 * qpb;
  const int nsec = int(at.back().beat / secQ) + 1;
  std::vector<Mode> mode(nsec, kHarmony);
  int chordalN = 0, linearN = 0, detachedN = 0;
  {
    // (chordal: three notes or more struck together; the lines' motion: a single note's step or
    // leap from the last single note near it, a line moving by steps, a broken chord by leaps)
    std::vector<int> attacks(nsec), chordal(nsec), notes(nsec), detached(nsec), steps(nsec), leaps(nsec), underHeld(nsec);
    for (int k = 0; k < na; ++k) {
      const int s = std::min(nsec - 1, int(at[k].beat / secQ));
      ++attacks[s];
      chordal[s] += at[k].idx.size() >= 3;
      // (a line over a held low note or chord, held 0.8 s by now: Peace Piece's ostinato, a
      // nocturne's: pedalled by its harmony; counterpoint's bass moves)
      for (int j = std::max(0, k - 32), hit = 0; j < k && !hit; ++j)
        for (int i : at[j].idx)
          if (nt[i].pitch < 60 && at[k].t - nt[i].on >= 0.8 && nt[i].off > at[k].t + 0.03) { ++underHeld[s]; hit = 1; break; }
      if (at[k].idx.size() == 1)
        for (int j = k - 1; j >= 0 && j >= k - 6 && at[k].t - at[j].t < 1.0; --j) {
          if (at[j].idx.size() != 1 || std::abs(at[j].lo - at[k].lo) > 9) continue;
          const int d = std::abs(at[j].lo - at[k].lo);
          steps[s] += d >= 1 && d <= 2;
          leaps[s] += d >= 3;
          break;
        }
      const double next = k + 1 < na ? at[k + 1].t - at[k].t : 1.0;
      for (int i : at[k].idx) {
        ++notes[s];
        const double dur = nt[i].off - nt[i].on;
        // (a staccato's rhythm, onsets under half a second apart: a slow chord let go early was
        // left to a pedal, in a performed file, as Peace Piece's are)
        detached[s] += next < 0.5 && dur < 0.55 * next && next - dur > 0.08;
      }
    }
    for (int s = 0; s < nsec; ++s) {
      if (!attacks[s]) { mode[s] = s ? mode[s - 1] : kHarmony; continue; }
      const double cs = double(chordal[s]) / attacks[s];
      const double stepShare = steps[s] + leaps[s] ? double(steps[s]) / (steps[s] + leaps[s]) : 0;
      const double det = double(detached[s]) / notes[s], held = double(underHeld[s]) / attacks[s];
      mode[s] = det > 0.6 && cs < 0.5 ? kDetached
              : (cs < 0.12 && stepShare >= 0.5 && held < 0.4) || (baroque && cs < 0.3) ? kLinear : kHarmony;
    }
    for (int s = 0; s < nsec; ++s) (mode[s] == kHarmony ? chordalN : mode[s] == kLinear ? linearN : detachedN) += attacks[s] > 0;
  }

  // --- The pedal through the attacks.
  bool down = false;          // (as planned: pressed now)
  double lastDown = -1e9;     // when it last went down
  int bass = 0;               // the lowest note since the last change
  unsigned pcs = 0;           // the pitch classes gathered since
  int pcCount[12] = {};
  std::vector<std::pair<int, double>> low;  // the notes below A3 caught since (pitch, onset)
  std::vector<int> highSteps;               // the melody's notes over C5 since (pitch)
  auto log = [&](double t, double beat, const std::string& s) {
    char b[64];
    std::snprintf(b, sizeof b, "%8.3f s  bar %3d  ", t, int(beat / qpb) + 1);
    plan.log.push_back(b + s);
  };
  auto push = [&](double t, bool d, long tick, int kind, const std::string& why) {
    // (never before the last change, never out of order)
    if (!plan.changes.empty()) t = std::max(t, plan.changes.back().t + 0.02);
    plan.changes.push_back({t, d, tick, kind, why});
    down = d;
    if (d) lastDown = t;
  };
  auto reset = [&](const Attack& a) {
    bass = a.lo;
    pcs = 0;
    std::fill(pcCount, pcCount + 12, 0);
    low.clear();
    highSteps.clear();
  };
  auto gather = [&](const Attack& a) {
    bass = std::min(bass, a.lo);
    for (int i : a.idx) {
      const int p = nt[i].pitch;
      if (!(pcs >> (p % 12) & 1)) ++pcCount[p % 12];
      pcs |= 1u << (p % 12);
      if (p < 57) low.push_back({p, a.t});
      if (p >= 72 && a.idx.size() == 1) highSteps.push_back(p);
    }
  };
  auto bits = [](unsigned x) { int c = 0; for (; x; x &= x - 1) ++c; return c; };
  // The lag after the onset (§4.3): lognormal about its median (in seconds: the tempo moves it
  // little, Repp 1997; a performed file's beats are not its tempo's anyway), before the next onset
  // that may change the pedal again, and mostly before the bass's release (caught: P10, §2.6; the
  // performed files catch it in 67-85 % of the changes, a key let go early under the pedal).
  auto lagFor = [&](int k) {
    const Attack& a = at[k];
    // (the median: 40 ms + a quarter of the time to the next onset, 50 .. 450 ms: MAESTRO's
    // 1276 performances, research/pedaling.md §4.3)
    const double gap = k + 1 < na ? at[k + 1].t - a.t : 1.0;
    double lag = kLag * std::clamp(0.040 + 0.25 * gap, 0.05, 0.45) * std::exp(kLagSpread * std::clamp(rng.gauss(), -2.2, 2.2));
    // (before the next onset that may itself change the pedal: a bass, a chord)
    for (int j = k + 1; j < na && j < k + 16; ++j)
      if (at[j].lo <= a.lo + 2 || at[j].idx.size() >= 3) { lag = std::min(lag, 0.8 * (at[j].t - a.t)); break; }
    lag = std::min(lag, std::max(a.loOff - a.t - 0.015, 0.6 * lag));  // (the bass caught while its key is down)
    return std::max(lag, 0.025);
  };
  // The release against the onset, by the time to the next onset (MAESTRO's medians, §4.3).
  auto releaseAfter = [](double gap) {
    static const double kG[] = {0.10, 0.275, 0.425, 0.625, 0.875, 1.25, 2.25};
    static const double kU[] = {-0.055, -0.014, 0.002, 0.025, 0.049, 0.076, 0.104};
    if (gap <= kG[0]) return kU[0];
    for (int i = 1; i < 7; ++i)
      if (gap <= kG[i]) return kU[i - 1] + (kU[i] - kU[i - 1]) * (gap - kG[i - 1]) / (kG[i] - kG[i - 1]);
    return kU[6];
  };
  auto voicing = [&](const Attack& a) {
    std::vector<int> v;
    for (int i : a.idx) v.push_back(nt[i].pitch);
    return v;
  };
  std::vector<int> lastChord;
  int changesInSec = 0, curSec = -1;
  std::vector<std::pair<int, int>> secChanges;  // (section, changes)
  for (int k = 0; k < na; ++k) {
    const Attack& a = at[k];
    const int s = std::min(nsec - 1, int(a.beat / secQ));
    if (s != curSec) {
      if (curSec >= 0) secChanges.push_back({curSec, changesInSec});
      curSec = s;
      changesInSec = 0;
    }
    const Mode m = mode[s];
    const double bl = beatLen(k);
    // (the last chord's voicing kept: a chord repeated as it was is the same harmony)
    bool revoiced = false;
    if (a.idx.size() >= 3) {
      const std::vector<int> v = voicing(a);
      revoiced = v != lastChord;
      lastChord = v;
    }
    // A rest: every key up over a second and over a beat: the pedal lifted a little after (P5, §4.7).
    if (down && a.t - freeFrom[k] > std::max(1.0, bl)) {
      push(freeFrom[k] + std::min(0.25, 0.3 * (a.t - freeFrom[k])), false, a.tick, 2, "lift: a rest");
      ++plan.lifts;
      log(plan.changes.back().t, a.beat, "lift (a rest of " + std::to_string(int((a.t - freeFrom[k]) * 1000)) + " ms)");
    }
    // A low run: up through it (P6, §5.2).
    if (a.lowRun) {
      if (down) {
        push(a.t - 0.01, false, a.tick, 2, "lift: a low run");
        ++plan.lifts;
        log(a.t, a.beat, "lift: a stepwise run in the bass from " + noteName(a.lo));
      }
      continue;
    }
    // Does this attack want the pedal at all?
    bool wants = m == kHarmony;
    if (m != kHarmony) {
      // (linear or detached: only a long chord, three notes or more held over a beat and a half,
      // or the final chord; detached: a long note held two beats)
      int longHeld = 0;
      for (int i : a.idx) longHeld += nt[i].off - nt[i].on > 1.5 * bl;
      wants = (m == kLinear && longHeld >= 3) || (m == kDetached && longHeld >= 2) || (k == na - 1 && a.idx.size() >= 3);
    }
    if (!wants) {
      if (down) {
        push(a.t - 0.01, false, a.tick, 2, "lift: the texture");
        ++plan.lifts;
        log(a.t, a.beat, std::string("lift: ") + kModeName[m]);
      }
      continue;
    }
    if (!down) {
      // Direct pedal: down with the note (MAESTRO: +23 ms the median after the pedal has been up
      // a second, IQR -27 .. +82 ms; §2.1), not before it (the planner keeps it simple), and
      // before the bass's key comes up.
      const double lag = std::clamp(0.023 + 0.04 * rng.gauss(), 0.005, std::max(0.02, std::min(0.12, a.loOff - a.t - 0.015)));
      push(a.t + lag, true, a.tick, 1, "direct");
      ++plan.direct;
      ++changesInSec;
      reset(a);
      gather(a);
      log(a.t + lag, a.beat, "down (direct) under " + noteName(a.lo) + (m != kHarmony ? ": a long chord" : ""));
      continue;
    }
    // A change?
    std::string why;

    const int pc = a.lo % 12;
    // (a bass: at or near the lowest note since the change, in the lower half of the keyboard)
    const bool isBass = a.lo < 62 && a.lo <= bass + 4;
    if (isBass && pc != bass % 12) {
      const bool chordTone = pcs >> pc & 1;
      // (a step always; a leap to a new pitch class only once the harmony has stood a while: a
      // broken chord's notes, coming one by one, are not each a new bass)
      const int iv = std::abs(a.lo - bass);
      if (iv <= 2 || (!chordTone && a.t - lastDown >= kMinLeap))
        why = "new bass " + noteName(a.lo) + (iv <= 2 ? " (a step)" : "");
    }
    if (why.empty() && m != kHarmony) why = "a long chord";
    if (why.empty()) {
      int fresh = 0, notesBelow = 0;
      for (int i : a.idx) {
        const int p = nt[i].pitch;
        notesBelow += p < 79;
        fresh += !(pcs >> (p % 12) & 1) && p < 79;
      }
      if (a.idx.size() >= 3 && notesBelow >= 2 && fresh >= 2) why = "a new chord (" + std::to_string(fresh) + " new pitch classes)";
      // A chord struck again (revoiced, or the same) a beat or more after the last change: the
      // old voicing's tail cleared (the jazz ballad's pedal by the chord, §3.7, §4.7).
      else if (a.idx.size() >= 3 && notesBelow >= 3 && a.t - lastDown > std::max(kRestrike * bl, kRestrikeS) && revoiced)
        why = "a chord revoiced";
    }
    // The bass struck again, the harmony having stood a while: the pedal renewed with it (the
    // performed files change at most left-hand onsets, §4.7, §8).
    if (why.empty() && isBass && a.t - lastDown >= kBassAgain) why = "the bass again";
    if (why.empty())
      for (int i : a.idx) {
        const int p = nt[i].pitch;
        if (p >= 57) continue;
        for (auto& [q, tq] : low)
          if (std::abs(p - q) >= 1 && std::abs(p - q) <= 2 && a.t - tq < 6) { why = "mud: " + noteName(p) + " a second from " + noteName(q); break; }
        if (!why.empty()) break;
      }
    if (why.empty() && bits(pcs) >= 8 && (isBass || a.idx.size() >= 2)) why = "a wash (" + std::to_string(bits(pcs)) + " pitch classes)";
    bool quick = false;
    if (why.empty() && a.idx.size() == 1 && a.lo >= 72) {
      // A melody's chromatic steps gathering over a long held harmony: a quick change (§2.4's
      // half-change, the damper being on or off here).
      int steps = 0;
      for (size_t j = 1; j < highSteps.size(); ++j) steps += std::abs(highSteps[j] - highSteps[j - 1]) == 1;
      if (steps >= 3 && a.t - lastDown > 2 * bl) { why = "the melody's semitones (a quick change)"; quick = true; }
    }
    if (why.empty()) { gather(a); continue; }
    if (a.t - lastDown < 0.16) { gather(a); continue; }  // (too soon after the last: the foot cannot)
    // The change: up about the onset (before it in a quick passage, after it in a slow one: the
    // two sounds overlap a moment, Hofmann's "simultaneously with the striking of the next tone";
    // MAESTRO's medians by the time to the next onset, §4.3), down after it.
    const double gap = k + 1 < na ? at[k + 1].t - a.t : 1.0;
    // (the performed files here release a median 5-24 ms before the onset whatever the tempo:
    // 10 ms before by default; PJ_PIANIST_UPSHIFT=1 MAESTRO's competition pianists' instead)
    double up = a.t - 0.010 + kUpShift * releaseAfter(gap) + std::clamp(0.025 * rng.gauss(), -0.05, 0.05);
    up = std::max(up, lastDown + 0.06);
    double dn;
    if (quick) {
      dn = up + 0.045 + 0.01 * rng.uni();
    } else {
      dn = a.t + lagFor(k);
      // (the dampers on the strings long enough to stop them: 60 ms, MAESTRO's shortest tenth;
      // 120 when a bass under C3 is to be silenced, its energy the largest, §6 P3)
      const double gmin = bass < 48 ? 0.12 : 0.06;
      if (dn - up < gmin) up = std::max(lastDown + 0.06, dn - gmin);
      dn = std::max(dn, up + gmin);
    }
    push(up, false, a.tick, quick ? 3 : 0, why);
    push(dn, true, a.tick, quick ? 3 : 0, why);
    ++(quick ? plan.quick : plan.legato);
    ++changesInSec;
    char b[48];
    std::snprintf(b, sizeof b, " (up %+.0f ms, down %+.0f ms)", (up - a.t) * 1e3, (dn - a.t) * 1e3);
    log(a.t, a.beat, "change: " + why + b);
    if (quick) gather(a);
    else { reset(a); gather(a); }
    if (quick) highSteps.clear();
  }
  if (curSec >= 0) secChanges.push_back({curSec, changesInSec});
  // The end: up with the last release (a fermata's length as played).
  double lastOff = 0;
  for (const PianoNote& n : nt) lastOff = std::max(lastOff, n.off);
  if (down) {
    push(lastOff + 0.05, false, at.back().tick, 2, "the end");
    ++plan.lifts;
    log(lastOff + 0.05, at.back().beat, "lift: the end");
  }
  // The share of the time pressed.
  {
    double d = 0, from = 0;
    for (const PedalChange& c : plan.changes)
      if (c.down) from = c.t;
      else d += c.t - from;
    plan.downShare = d / std::max(1e-3, lastOff - nt.front().on);
  }

  // --- The summary.
  const int total = chordalN + linearN + detachedN;
  const char* style = !total ? "nothing to pedal"
                    : chordalN >= 0.7 * total ? (baroque ? "chordal" : "legato pedalling by harmony")
                    : linearN >= 0.6 * total ? "a linear texture: finger legato, the pedal sparing"
                    : detachedN >= 0.6 * total ? "detached: dry"
                    : "mixed textures";
  std::vector<double> lags;
  {
    size_t j = 0;
    for (const PedalChange& c : plan.changes)
      if (c.down && c.kind == 0) {
        while (j + 1 < at.size() && at[j + 1].t <= c.t) ++j;
        lags.push_back(c.t - at[j].t);
      }
    std::sort(lags.begin(), lags.end());
  }
  char buf[512];
  std::snprintf(buf, sizeof buf,
                "pianist: %s: %s%s; %d changes (%d legato, %d direct, %d quick), %d lifts; pedal down %.0f %% of the time%s\n",
                who.c_str(), style, baroque ? " (the director: Baroque)" : "", plan.legato + plan.direct + plan.quick, plan.legato,
                plan.direct, plan.quick, plan.lifts, 100 * plan.downShare,
                lags.empty() ? "" : (", down " + std::to_string(int(lags[lags.size() / 2] * 1000 + 0.5)) + " ms after the onset (median)").c_str());
  plan.summary = buf;
  // The sections, those alike joined: "bars 1-8: by harmony, 24 changes".
  for (size_t i = 0; i < secChanges.size();) {
    size_t j = i;
    int ch = 0;
    while (j < secChanges.size() && mode[secChanges[j].first] == mode[secChanges[i].first]) ch += secChanges[j++].second;
    const int b0 = int(secChanges[i].first * 4) + 1, b1 = int(secChanges[j - 1].first * 4) + 4;
    std::snprintf(buf, sizeof buf, "  bars %d-%d: %s, %d pedals\n", b0, b1, kModeName[mode[secChanges[i].first]], ch);
    plan.summary += buf;
    i = j;
    if (plan.summary.size() > 4000) { plan.summary += "  ...\n"; break; }
  }
  return plan;
}

}  // namespace pajupilli
