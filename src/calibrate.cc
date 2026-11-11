#include "calibrate.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "analysis.h"
#include "flute.h"
#include "player.h"

namespace pajupilli {
namespace {

constexpr int kFs = 48000;

// Plays the note from silence; returns the error in cents (NAN if it sounds in
// the wrong register or not at all).
double tryNote(int midi, double cover, double press, double a4) {
  Flute fl(kFs);
  Player pl(fl);
  pl.coverTrim[midi] = cover;
  pl.pressTrim[midi] = press;
  const int n = int(0.35 * kFs);
  std::vector<float> x(n);
  pl.noteOn(midi, 64);
  pl.render(x.data(), n);
  const int a = n / 2;
  const double f0 = noteFrequency(midi, a4);
  if (rms(x.data() + a, n - a) < 1e-3) return NAN;
  double clarity = 0;
  const double f = estimatePitch(x.data() + a, n - a, kFs, f0 / 2.2, f0 * 2.2, &clarity);
  const double c = 1200 * std::log2(f / f0);
  return std::fabs(c) < 150 && clarity > 0.6 ? c : NAN;
}

}  // namespace

Calibration calibrate(double a4, FILE* log, int lo, int hi) {
  Calibration cal;
  for (int m = lo; m <= hi; ++m) {
    // Uncovered width stays within 2.5 .. 10 mm of the 10.3 mm hole.
    const double f = noteFrequency(m);
    const double t = std::clamp(std::log(f / 261.6) / std::log(1760 / 261.6), 0.0, 1.2);
    const double base = 6.0 - 2.5 * t;
    const double cmin = 2.0 / base, cmax = 10.0 / base;
    // For each breath pressure that sounds the right register, bisect the
    // coverage (more coverage = flatter); keep the best. Gentler breath also
    // flattens, so pressure is the second tuning control.
    double bestAbs = 1e9, bestCover = 1, bestPress = 1, bestC = NAN;
    for (double p : {1.0, 1.2, 0.85, 1.45, 0.7, 1.7, 0.6, 2.0}) {
      double xg = 0, c = tryNote(m, 1, p, a4);
      if (std::isnan(c)) continue;
      double a = std::log(cmin), b = std::log(cmax), x = 0, bx = 0, bc = c;
      for (int it = 0; it < 10 && std::fabs(bc) > 2; ++it) {
        if (c > 0) b = xg; else a = xg;
        x = 0.5 * (a + b);
        const double cx = tryNote(m, std::exp(x), p, a4);
        if (std::isnan(cx)) {  // lost the register: x is a limit
          if (x < xg) a = x; else b = x;
          continue;
        }
        xg = x;
        c = cx;
        if (std::fabs(c) < std::fabs(bc)) { bx = x; bc = c; }
      }
      if (std::fabs(bc) < bestAbs) { bestAbs = std::fabs(bc); bestCover = std::exp(bx); bestPress = p; bestC = bc; }
      if (bestAbs < 3) break;
    }
    if (std::isnan(bestC)) {
      if (log) std::fprintf(log, "calibrate: MIDI %d does not sound in its register\n", m);
      continue;
    }
    const double best = std::log(bestCover), press = bestPress, bestCents = bestC;
    cal.cover[m] = std::exp(best);
    cal.press[m] = press;
    cal.cents[m] = float(bestCents);
    cal.ok[m] = true;
    if (log)
      std::fprintf(log, "calibrate: MIDI %d uncovered %.1f mm, pressure x%.2f, %+.0f cents\n", m,
                   base * cal.cover[m], press, bestCents);
  }
  return cal;
}

}  // namespace pajupilli
