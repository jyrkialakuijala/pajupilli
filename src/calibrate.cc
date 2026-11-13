#include "calibrate.h"

#include <algorithm>
#include <atomic>
#include <thread>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

extern char** environ;

#include "analysis.h"
#include "voice.h"

namespace pajupilli {
namespace {

constexpr int kFs = 48000;  // Voice output

// Plays the note from silence; returns the error in cents (NAN if it sounds in
// the wrong register or not at all). wobble: level peak-to-peak / mean over the
// settled part (a pulsing, two-frequency state is not a usable note).
double tryNote(const Instrument& inst, int midi, double cover, double press, double a4, double dyn = 0.5,
               double jet = 1, double* wobble = nullptr) {
  Voice v(inst);
  Performer& pl = v.player();
  pl.coverTrim[midi] = pl.coverLoTrim[midi] = pl.coverHiTrim[midi] = cover;
  pl.pressTrim[midi] = press;
  pl.jetTrim[midi] = jet;
  pl.setDynamics(dyn);
  pl.setWander(0);  // learn on a steady player
  // Brass: learn the lips' own setting, not one the ear keeps correcting (the
  // ear only trims a held note, slowly).
  if (inst.lip()) pl.setEar(false);
  // Low notes build up over a number of periods: listen longer (bass C3: 2x).
  const double f0 = noteFrequency(midi, a4);
  // A brass note builds up over more periods still (the long bore).
  const double slow = std::max(1.0, 261.6 / f0) * (inst.lip() ? 1.6 : 1.0);
  const int n = int((wobble ? 0.5 : 0.35) * slow * kFs);
  std::vector<float> x(n);
  pl.noteOn(midi, 64);
  v.render(x.data(), n);
  const int a = wobble ? int(0.2 * slow * kFs) : n / 2;
  if (rms(x.data() + a, n - a) < 1e-3) return NAN;
  double clarity = 0;
  const double f = estimatePitch(x.data() + a, n - a, kFs, f0 / 2.2, f0 * 2.2, &clarity);
  const double c = 1200 * std::log2(f / f0);
  if (wobble) {
    double lmin = 1e9, lmax = 0, lm = 0;
    int k = 0;
    const int fr = int(std::max(0.02, 4 / f0) * kFs);  // >= 4 periods (bass)
    for (int i = a; i + fr <= n; i += fr, ++k) {
      const double r = rms(x.data() + i, fr);
      lmin = std::min(lmin, r);
      lmax = std::max(lmax, r);
      lm += r;
    }
    *wobble = k ? (lmax - lmin) / (lm / k) : 1;
  }
  return std::fabs(c) < 150 && clarity > 0.6 ? c : NAN;
}

constexpr double kSteady = 0.2;  // level wobble above this: not a usable state
const double kCoverMm = std::atof(std::getenv("PJ_CALCOVER") ? std::getenv("PJ_CALCOVER") : "1.5");

}  // namespace

// One note: for each breath pressure that sounds the right register, bisect
// the coverage (more coverage = flatter); keep the best. Gentler breath also
// flattens, so pressure is the second tuning control.
// Bisect the coverage (log scale, more coverage = flatter) for one pressure and
// dynamics, starting from x0. Returns the best |cents| found (NAN: never sounded)
// and its coverage.
// The range of the pitch control ("cover") a player has, as multipliers.
static void coverRange(const Instrument& inst, int m, double* cmin, double* cmax) {
  switch (inst.kind) {
    case Kind::kDuct:  // the breath: a player's range for one note (an ocarina's is wide)
      *cmin = 0.5;
      *cmax = 2.5;
      return;
    case Kind::kReed:
    case Kind::kDoubleReed:  // the lips' force on the reed
      *cmin = 0.5;
      *cmax = 2.0;
      return;
    case Kind::kLip:  // brass: the lips' resonance, within the note's slot
      *cmin = 0.82;
      *cmax = 1.18;
      return;
    default: break;
  }
  const double f = noteFrequency(m - inst.transpose);  // the lips go by the written note
  const double t = std::clamp(std::log(f / 261.6) / std::log(1760 / 261.6), 0.0, 1.2);
  const double base = 6.0 - 2.5 * t;  // Coltman's uncovered width (concert-flute mm)
  // A player's lips: Coltman's schedule +-1.5 mm (wider than that is not a
  // flute embouchure; the rest of the error is the model's, and is logged).
  // An end-blown flute's lips cover the rim much further (meri): half a hole.
  const double mm = inst.kind == Kind::kEndBlown ? 3.5 : kCoverMm;
  *cmin = std::max(inst.kind == Kind::kEndBlown ? 0.8 : 1.5, base - mm) / base;
  *cmax = (base + mm) / base;
}

static double bisectCover(const Instrument& inst, int m, double p, double dyn, double jet, double a4, double x0, double* cover) {
  double cmin, cmax;
  coverRange(inst, m, &cmin, &cmax);
  double xg = x0, c = tryNote(inst, m, std::exp(x0), p, a4, dyn, jet);
  // Not in its register from there: try the rest of the player's range
  // (a duct's pitch climbs steeply with the breath; a reed's lips).
  // (The flute family keeps its calibration: they start in their register.)
  for (int k = 0; k < 6 && std::isnan(c) && inst.design; ++k) {
    const double u = (k + 0.5) / 6;
    xg = std::log(cmin) + u * (std::log(cmax) - std::log(cmin));
    c = tryNote(inst, m, std::exp(xg), p, a4, dyn, jet);
  }
  if (std::isnan(c)) return NAN;
  x0 = xg;
  double a = std::log(cmin), b = std::log(cmax), bx = x0, bc = c;
  for (int it = 0; it < 10 && std::fabs(bc) > 2; ++it) {
    if (c > 0) b = xg; else a = xg;
    const double x = 0.5 * (a + b);
    const double cx = tryNote(inst, m, std::exp(x), p, a4, dyn, jet);
    if (std::isnan(cx)) {  // lost the register: x is a limit
      if (x < xg) a = x; else b = x;
      continue;
    }
    xg = x;
    c = cx;
    if (std::fabs(c) < std::fabs(bc)) { bx = x; bc = c; }
  }
  *cover = std::exp(bx);
  return bc;
}

// Brass: one note at a lip setting, breath and dynamics, played from silence:
// the pitch's error (median over 50 ms frames, cents) and how unsteady it is:
// the pitch's spread (cents) plus the level's wobble (x30). NAN: not in its
// partial (or silent, or not periodic). Measured late (the long bore builds up
// slowly) and long (a quasi-periodic note beats over ~0.1..0.5 s).
struct LipTrial {
  double cents = NAN, spread = 1e9;
  bool steady() const { return !std::isnan(cents) && spread < kLipSpread; }
  static constexpr double kLipSpread = 8;
};

static LipTrial tryLip(const Instrument& inst, int midi, double cover, double press, double a4, double dyn,
                       int fing) {
  Voice v(inst);
  Performer& pl = v.player();
  pl.fingering[midi] = fing;
  pl.coverTrim[midi] = pl.coverLoTrim[midi] = pl.coverHiTrim[midi] = cover;
  pl.pressTrim[midi] = press;
  pl.setDynamics(dyn);
  pl.setWander(0);
  pl.setEar(false);
  const double f0 = noteFrequency(midi, a4);
  const double t0 = f0 < 150 ? 0.7 : 0.5, t1 = f0 < 150 ? 1.6 : 1.2;
  const int n = int(t1 * kFs);
  std::vector<float> x(n);
  pl.noteOn(midi, 64);
  v.render(x.data(), n);
  const int fr = int(std::max(0.05, 6 / f0) * kFs);
  std::vector<double> cs, lv;
  for (int i = int(t0 * kFs); i + fr <= n; i += fr) {
    const double r = rms(x.data() + i, fr);
    double clarity = 0;
    const double f = estimatePitch(x.data() + i, fr, kFs, f0 / 1.5, f0 * 1.5, &clarity);
    if (r < 3e-4 || clarity < 0.5 || f <= 0) return LipTrial();
    const double c = 1200 * std::log2(f / f0);
    if (std::fabs(c) > 100) return LipTrial();
    cs.push_back(c);
    lv.push_back(r);
  }
  LipTrial t;
  if (cs.size() < 3) return t;
  std::vector<double> sorted = cs;
  std::sort(sorted.begin(), sorted.end());
  t.cents = sorted[sorted.size() / 2];
  double sd = 0, lmin = 1e9, lmax = 0, lm = 0;
  for (double c : cs) sd += (c - t.cents) * (c - t.cents);
  for (double l : lv) { lmin = std::min(lmin, l); lmax = std::max(lmax, l); lm += l; }
  t.spread = std::sqrt(sd / cs.size()) + 30 * (lmax - lmin) / (lm / lv.size());
  return t;
}

// Brass: the note reached by a slur from `from` (its standard fingering, the
// lips and breath at their design values), the median error over 0.4..1.2 s
// after the slur (NAN: another partial, or unsteady). A slot can hold two
// states, and a running oscillation can carry the note into the sharp one.
static double tryLipSlur(const Instrument& inst, int from, int midi, int fing, double cover, double press,
                         double a4) {
  Voice v(inst);
  Performer& pl = v.player();
  pl.coverTrim[midi] = pl.coverLoTrim[midi] = pl.coverHiTrim[midi] = cover;
  pl.pressTrim[midi] = press;
  pl.fingering[midi] = fing;
  pl.setDynamics(0.5);
  pl.setWander(0);
  pl.setEar(false);
  std::vector<float> a(int(0.5 * kFs)), x(int(1.2 * kFs));
  pl.noteOn(from, 64);
  v.render(a.data(), int(a.size()));
  pl.noteOn(midi, 64);
  pl.noteOff(from);
  v.render(x.data(), int(x.size()));
  const double f0 = noteFrequency(midi, a4);
  const int fr = int(std::max(0.05, 6 / f0) * kFs);
  std::vector<double> cs;
  for (int i = int(0.4 * kFs); i + fr <= int(x.size()); i += fr) {
    double clarity = 0;
    const double f = estimatePitch(x.data() + i, fr, kFs, f0 / 1.5, f0 * 1.5, &clarity);
    if (rms(x.data() + i, fr) < 3e-4 || clarity < 0.5 || f <= 0) return NAN;
    cs.push_back(1200 * std::log2(f / f0));
  }
  std::sort(cs.begin(), cs.end());
  if (cs.empty() || cs.back() - cs.front() > 30) return NAN;
  return cs[cs.size() / 2];
}

// Brass: the lips' setting for one breath and dynamics, the way a player
// finds the middle of the note's slot. Scan the lips' resonance over [lo, hi]
// (log, k steps). Inside a slot the pitch rises smoothly with the lips but
// can also jump (by 40..100 cents: the oscillation changes how it sits on the
// bore's resonances); there, and at the slot's edges, the note beats, cracks
// or flips between the two. So the usable settings are runs of steady steps
// without a jump; the chosen setting keeps >= 2 % of the lips' tension from a
// run's ends (the ear and the player's wander stay inside that), as near in
// tune as that allows (the pitch interpolated between steps). Returns false
// if there is no such setting.
static bool centreLips(const Instrument& inst, int m, int fing, double press, double a4, double dyn, double lo,
                       double hi, int k, double* cover, LipTrial* out) {
  constexpr double kJump = 25, kMargin = 0.02;
  std::vector<double> xs(k);
  std::vector<LipTrial> tr(k);
  for (int i = 0; i < k; ++i) {
    xs[i] = std::log(lo) + (std::log(hi) - std::log(lo)) * i / (k - 1);
    tr[i] = tryLip(inst, m, std::exp(xs[i]), press, a4, dyn, fing);
  }
  if (std::getenv("PJ_CALDEBUG")) {
    std::fprintf(stderr, "  %d/%d press %.2f dyn %.2f:", m, fing, press, dyn);
    for (int i = 0; i < k; ++i)
      std::fprintf(stderr, " %.2f:%+.0f/%.0f", std::exp(xs[i]), tr[i].cents, std::min(99.0, tr[i].spread));
    std::fprintf(stderr, "\n");
  }
  const double step = (std::log(hi) - std::log(lo)) / (k - 1);
  double bestScore = 1e9;
  bool found = false;
  for (int i = 0; i < k;) {
    if (!tr[i].steady()) { ++i; continue; }
    int j = i;
    while (j + 1 < k && tr[j + 1].steady() && std::fabs(tr[j + 1].cents - tr[j].cents) < kJump) ++j;
    // The run xs[i]..xs[j]; usable: kMargin inside both ends (a run of one
    // step stands for at most a step either side of it).
    const double a = xs[i] - 0.5 * step + kMargin, b = xs[j] + 0.5 * step - kMargin;
    if (b >= a) {
      // The pitch along the run (linear between steps), clamped to [a, b]:
      // the setting where it crosses zero, or the end nearer to it.
      auto pitchAt = [&](double x) {
        if (x <= xs[i]) return tr[i].cents;
        for (int q = i; q < j; ++q)
          if (x <= xs[q + 1]) return tr[q].cents + (tr[q + 1].cents - tr[q].cents) * (x - xs[q]) / step;
        return tr[j].cents;
      };
      double x = std::fabs(pitchAt(a)) < std::fabs(pitchAt(b)) ? a : b;
      for (int q = i; q < j; ++q)
        if ((tr[q].cents <= 0) != (tr[q + 1].cents <= 0)) {
          const double u = tr[q].cents / (tr[q].cents - tr[q + 1].cents);
          x = std::clamp(xs[q] + u * step, a, b);
        }
      LipTrial t;
      t.cents = pitchAt(x);
      t.spread = 0;
      for (int q = i; q <= j; ++q) t.spread = std::max(t.spread, tr[q].spread);
      // Prefer in tune, then a wide run, then steady.
      const double score = std::fabs(t.cents) + 0.02 / (b - a + 0.005) + 0.3 * t.spread;
      if (score < bestScore) { bestScore = score; *cover = std::exp(x); *out = t; found = true; }
    }
    i = j + 1;
  }
  return found;
}

// Brass, one note: for each breath (a player's range first), centre the lips
// in the slot at mezzo-forte; then walk the dynamics down and up from there in
// steps of 0.1, re-centring the lips near the last setting at each (louder
// pulls the note sharp), as far as the note stays steady within 15 cents.
static void calibrateLipNote(const Instrument& inst, int m, double a4, Calibration* cal) {
  double cmin, cmax;
  coverRange(inst, m, &cmin, &cmax);
  static const double kPress[] = {1.0, 0.85, 1.2, 0.7, 1.45};
  struct Cand { int fing; double press, cover, score; LipTrial t; };
  std::vector<Cand> cands;
  // The standard fingering, and the alternate if it has one (a player takes
  // the alternate when it is steadier or better in tune: a small penalty).
  for (int fing = 0; fing < 2; ++fing) {
    if (!noteInfo(inst, m, fing)) continue;
    for (int ip = 0; ip < 5; ++ip) {
      double cv;
      LipTrial t;
      if (!centreLips(inst, m, fing, kPress[ip], a4, 0.5, cmin, cmax, 17, &cv, &t)) continue;
      // Refine around it at a finer step.
      double cv2;
      LipTrial t2;
      if (centreLips(inst, m, fing, kPress[ip], a4, 0.5, cv * 0.96, cv * 1.04, 9, &cv2, &t2)) { cv = cv2; t = t2; }
      cands.push_back({fing, kPress[ip], cv,
                       std::fabs(t.cents) + 0.3 * t.spread + 10 * std::fabs(std::log(kPress[ip])) + 3 * fing, t});
    }
  }
  if (cands.empty()) return;
  std::sort(cands.begin(), cands.end(), [](const Cand& a, const Cand& b) { return a.score < b.score; });
  // The best that a slur from a fourth below and from a fourth above also
  // brings in tune (within 15 cents); if none does, the one that misses least.
  const int first = inst.lo + inst.transpose, last = inst.hi + inst.transpose;
  double bestScore = 1e9;
  const Cand* pick = &cands[0];
  for (const Cand& c : cands) {
    double miss = 0;
    for (int from : {m - 5, m + 5}) {
      if (from < first || from > last) continue;
      const double e = tryLipSlur(inst, from, m, c.fing, c.cover, c.press, a4);
      miss += std::isnan(e) ? 60 : std::max(0.0, std::fabs(e) - 15);
    }
    if (std::getenv("PJ_CALDEBUG"))
      std::fprintf(stderr, "  %d fingering %d press %.2f lips %.3f: %+.0f c, slurs miss %.0f\n", m, c.fing, c.press,
                   c.cover, c.t.cents, miss);
    if (c.score + miss < bestScore) { bestScore = c.score + miss; pick = &c; }
    if (miss == 0) break;
  }
  const double bestCover = pick->cover, bestPress = pick->press;
  const int bestFing = pick->fing;
  const LipTrial best = pick->t;
  if (std::isnan(best.cents)) return;
  cal->fingering[m] = bestFing;
  cal->cover[m] = cal->coverLo[m] = cal->coverHi[m] = bestCover;
  cal->press[m] = bestPress;
  cal->cents[m] = float(best.cents);
  cal->ok[m] = true;
  auto walk = [&](double step, double* edge, double* cvEdge) {
    double cv = bestCover;
    *edge = 0.5;
    *cvEdge = cv;
    for (int k = 1; k <= 5; ++k) {
      const double d = 0.5 + k * step;
      double c2;
      LipTrial t;
      if (!centreLips(inst, m, bestFing, bestPress, a4, d, std::max(cmin, cv * 0.93), std::min(cmax, cv * 1.07), 9,
                      &c2, &t) ||
          std::fabs(t.cents) > 15)
        break;
      cv = c2;
      *edge = d;
      *cvEdge = cv;
    }
  };
  double lo, hi, cvLo, cvHi;
  walk(-0.1, &lo, &cvLo);
  walk(0.1, &hi, &cvHi);
  cal->dynLo[m] = lo > 1e-9 ? std::min(0.5, lo + 0.05) : 0.0;
  cal->dynHi[m] = hi >= 1 - 1e-9 ? 1.0 : std::max(0.5, hi - 0.05);
  // coverAt: log-linear from coverLo at max(dynLo, 0.15) to cover at 0.5 to
  // coverHi at kCalDynHi; put the edges' settings on that line.
  const double cm = std::log(bestCover);
  const double l0 = std::max(cal->dynLo[m], 0.15);
  cal->coverLo[m] = lo < 0.5 ? std::exp(cm + (std::log(cvLo) - cm) * (0.5 - l0) / (0.5 - lo)) : bestCover;
  cal->coverHi[m] = hi > 0.5 ? std::exp(cm + (std::log(cvHi) - cm) * (kCalDynHi - 0.5) / (hi - 0.5)) : bestCover;
}

// One note: for each breath pressure that sounds the right register, bisect
// the coverage; keep the best. Gentler breath also flattens, so pressure is the
// second tuning control. Then: the softest dynamics that still holds the
// register, and the coverage at soft and loud.
static void calibrateNote(const Instrument& inst, int m, double a4, Calibration* cal) {
  if (inst.lip()) {
    calibrateLipNote(inst, m, a4, cal);
    return;
  }
  // Jet length: the player moves the lips closer to or further from the edge
  // until the note is steady (a note can sit where its harmonics beat against
  // a nearby resonance, e.g. D6 at Fletcher's length).
  double bestAbs = 1e9, bestCover = 1, bestPress = 1, bestJet = 1, bestC = NAN, bestWob = 1e9;
  // Fletcher's jet length +-20 % (shorter as a last resort: players shorten it
  // in the third octave), breath x0.85 .. 1.25: a player tunes with the lips,
  // not by blowing harder. Only if that finds no steady note within 10 cents,
  // harder breath (logged as outside a player's range: the model's error).
  static const double kJets[] = {1.0, 0.9, 1.1, 0.8, 1.2, 0.7, 0.85, 0.75};
  static const double kPress[] = {1.0, 1.15, 0.9, 1.25, 0.85}, kHard[] = {1.45, 1.7, 2.0};
  // Reeds: the breath within a player's range (more closes the reed); a duct's
  // breath is its pitch control already; only the flutes move the lips to the edge.
  static const double kReedPress[] = {1.0, 0.85, 1.15, 0.7, 1.3}, kReedHard[] = {0.6, 1.45};
  const bool lips = inst.kind == Kind::kFlute || inst.kind == Kind::kEndBlown;
  const int nJets = lips ? 8 : 1;
  auto search = [&](const double* ps, int np) {
    for (int ij = 0; ij < nJets; ++ij) {
      const double jet = kJets[ij];
      double jAbs = 1e9, jCover = 1, jPress = 1, jC = NAN;
      for (int k = 0; k < np; ++k) {
        const double p = ps[k];
        double cv = 1;
        const double bc = bisectCover(inst, m, p, 0.5, jet, a4, 0, &cv);
        if (std::getenv("PJ_CALDEBUG")) std::fprintf(stderr, "  %d jet %.2f press %.2f: %+.0f c cover %.2f\n", m, jet, p, bc, cv);
        if (std::isnan(bc)) continue;
        if (std::fabs(bc) < jAbs) { jAbs = std::fabs(bc); jCover = cv; jPress = p; jC = bc; }
        if (jAbs < 3) break;
      }
      if (std::isnan(jC)) continue;
      double wob = 1;
      const double cc = tryNote(inst, m, jCover, jPress, a4, 0.5, jet, &wob);
      if (std::isnan(cc)) continue;
      // Prefer steady, then in tune.
      const bool better = (wob < kSteady) != (bestWob < kSteady) ? wob < kSteady
                          : wob < kSteady ? jAbs < bestAbs : wob < bestWob;
      if (better) { bestAbs = jAbs; bestCover = jCover; bestPress = jPress; bestJet = jet; bestC = jC; bestWob = wob; }
      if (bestWob < kSteady && bestAbs < 3) break;
    }
  };
  if (inst.kind == Kind::kDuct) {
    search(kPress, 1);
  } else if (inst.reed() || inst.lip()) {
    search(kReedPress, 5);
    if (!(bestWob < kSteady && bestAbs <= 10)) search(kReedHard, 2);
  } else {
    search(kPress, 5);
    if (!(bestWob < kSteady && bestAbs <= 10)) search(kHard, 3);
  }
  if (std::isnan(bestC)) return;
  cal->cover[m] = cal->coverLo[m] = cal->coverHi[m] = bestCover;
  cal->press[m] = bestPress;
  cal->jet[m] = bestJet;
  cal->cents[m] = float(bestC);
  cal->ok[m] = true;
  // Softest dynamics: every step from there up to mezzo-forte must hold the
  // register and be steady; a little margin for slurs.
  double floor = 0.5;
  for (double d = 0.4; d >= -1e-9; d -= 0.1) {
    double wob = 1;
    if (std::isnan(tryNote(inst, m, bestCover, bestPress, a4, std::max(0.0, d), bestJet, &wob)) ||
        wob > kSteady) break;
    floor = std::max(0.0, d);
  }
  cal->dynLo[m] = floor > 0 ? std::min(0.5, floor + 0.05) : 0.0;
  // Loudest: likewise up from mezzo-forte (a soft reed, a vessel can only take
  // so much breath before the note breaks). The flutes keep their whole range.
  if (inst.design) {
    double ceil = 0.5;
    for (double d = 0.6; d <= 1.0 + 1e-9; d += 0.1) {
      double wob = 1;
      if (std::isnan(tryNote(inst, m, bestCover, bestPress, a4, d, bestJet, &wob)) || wob > kSteady) break;
      ceil = d;
    }
    cal->dynHi[m] = ceil >= 1 ? 1.0 : std::max(0.5, ceil - 0.05);
  }
  const double lo = std::max(cal->dynLo[m], 0.15);
  // Lips move only so far (Coltman: about +-1.5 mm); the rest of the pitch
  // change with breath stays, as on a real flute (loud a little sharp).
  auto limit = [bestCover](double cv) { return std::clamp(cv, 0.75 * bestCover, 1.35 * bestCover); };
  if (inst.kind == Kind::kDuct) return;  // the breath is the pitch: louder stays sharper
  double cv = bestCover;
  if (!std::isnan(bisectCover(inst, m, bestPress, lo, bestJet, a4, std::log(bestCover), &cv))) cal->coverLo[m] = limit(cv);
  cv = bestCover;
  if (!std::isnan(bisectCover(inst, m, bestPress, std::min(kCalDynHi, cal->dynHi[m]), bestJet, a4, std::log(bestCover), &cv)))
    cal->coverHi[m] = limit(cv);
}

static Calibration learn(const Instrument& inst, double a4, int lo, int hi) {
  Calibration cal;
  // Notes are independent: learn them in parallel.
  std::atomic<int> next{lo};
  const int nThreads = std::max(1, std::min(int(std::thread::hardware_concurrency()), hi - lo + 1));
  std::vector<std::thread> pool;
  for (int t = 0; t < nThreads; ++t)
    pool.emplace_back([&] {
      for (int m; (m = next.fetch_add(1)) <= hi;) calibrateNote(inst, m, a4, &cal);
    });
  for (std::thread& th : pool) th.join();
  return cal;
}

static void logCalibration(const Instrument& inst, const Calibration& cal, FILE* log, int lo, int hi) {
  for (int m = lo; m <= hi; ++m) {
    if (!cal.ok[m]) {
      std::fprintf(log, "calibrate: MIDI %d does not sound in its register\n", m);
      continue;
    }
    if (inst.kind != Kind::kFlute && inst.kind != Kind::kEndBlown) {
      char fing[64] = "";
      if (inst.lip())
        if (const NoteInfo* ni = noteInfo(inst, m, cal.fingering[m]))
          std::snprintf(fing, sizeof fing, "; valves %s partial %d%s", ni->holes, ni->harmonic,
                        cal.fingering[m] ? " (alternate)" : "");
      std::fprintf(log, "calibrate: MIDI %d %s x%.2f, pressure x%.2f, %+.0f cents; softest dynamics %.2f; "
                   "%s soft x%.2f, loud x%.2f; loudest dynamics %.2f%s\n", m, inst.reed() || inst.lip() ? "lips" : "breath", cal.cover[m],
                   cal.press[m], cal.cents[m], cal.dynLo[m], inst.reed() || inst.lip() ? "lips" : "breath",
                   cal.coverLo[m], cal.coverHi[m], cal.dynHi[m], fing);
      continue;
    }
    const double f = noteFrequency(m - inst.transpose);
    const double t = std::clamp(std::log(f / 261.6) / std::log(1760 / 261.6), 0.0, 1.2);
    const double mm = (6.0 - 2.5 * t) * inst.emb;
    std::fprintf(log, "calibrate: MIDI %d uncovered %.1f mm, pressure x%.2f, %+.0f cents; "
                 "jet x%.2f; softest dynamics %.2f; uncovered soft %.1f, loud %.1f mm%s\n", m,
                 mm * cal.cover[m], cal.press[m], cal.cents[m], cal.jet[m], cal.dynLo[m],
                 mm * cal.coverLo[m], mm * cal.coverHi[m],
                 cal.press[m] > 1.3 ? " (breath outside a player's range)" : "");
  }
}

// A calibration depends only on this binary and the PJ_* knobs: keep it in
// ~/.cache/pajupilli keyed on both, so live startup and make test skip the ~1 s
// of learning. PJ_NOCALCACHE=1 bypasses it.
static std::string cachePath(const Instrument& inst, double a4, int lo, int hi) {
  const char* home = std::getenv("HOME");
  struct stat st;
  if (std::getenv("PJ_NOCALCACHE") || !home || stat("/proc/self/exe", &st) != 0) return "";
  uint64_t h = 1469598103934665603ull;  // FNV-1a
  auto mix = [&h](const void* d, size_t n) {
    for (size_t i = 0; i < n; ++i) h = (h ^ static_cast<const unsigned char*>(d)[i]) * 1099511628211ull;
  };
  mix(&st.st_size, sizeof st.st_size);
  mix(&st.st_mtime, sizeof st.st_mtime);
  std::vector<std::string> env;
  for (char** e = environ; *e; ++e)
    if (std::strncmp(*e, "PJ_", 3) == 0) env.emplace_back(*e);
  std::sort(env.begin(), env.end());
  for (const std::string& e : env) mix(e.data(), e.size() + 1);
  mix(inst.name, std::strlen(inst.name) + 1);
  mix(&a4, sizeof a4);
  mix(&lo, sizeof lo);
  mix(&hi, sizeof hi);
  const std::string cache = std::string(home) + "/.cache", dir = cache + "/pajupilli";
  mkdir(cache.c_str(), 0755);
  mkdir(dir.c_str(), 0755);
  char name[40];
  std::snprintf(name, sizeof name, "/cal-%016llx.bin", static_cast<unsigned long long>(h));
  return dir + name;
}

Calibration calibrate(const Instrument& inst, double a4, FILE* log, int lo, int hi) {
  if (lo < 0) lo = inst.lo + inst.transpose;
  if (hi < 0) hi = inst.hi + inst.transpose;
  if (inst.plucked()) {  // strings are tuned by their lengths: nothing to find
    Calibration cal;
    for (int m = lo; m <= hi; ++m) cal.ok[m] = true;
    return cal;
  }
  if (inst.circuit()) {  // a circuit tunes itself (its autotune, src/lead.cc)
    Calibration cal;
    for (int m = lo; m <= hi; ++m) cal.ok[m] = true;
    if (log) std::fprintf(log, "calibrate: the %s tunes itself at power-on (autotune)\n", inst.name);
    return cal;
  }
  const std::string path = cachePath(inst, a4, lo, hi);
  Calibration cal;
  bool cached = false;
  if (!path.empty())
    if (FILE* f = std::fopen(path.c_str(), "rb")) {
      cached = std::fread(&cal, sizeof cal, 1, f) == 1;
      std::fclose(f);
    }
  if (!cached) {
    cal = learn(inst, a4, lo, hi);
    if (!path.empty()) {
      const std::string tmp = path + ".tmp" + std::to_string(getpid());
      if (FILE* f = std::fopen(tmp.c_str(), "wb")) {
        const bool ok = std::fwrite(&cal, sizeof cal, 1, f) == 1;
        std::fclose(f);
        if (ok) std::rename(tmp.c_str(), path.c_str());
        else std::remove(tmp.c_str());
      }
    }
  }
  if (log) logCalibration(inst, cal, log, lo, hi);
  return cal;
}

}  // namespace pajupilli
