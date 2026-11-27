// The bass presets of the amp module (src/guitaramp.cc; research/bass_amps.md 10.6) on their own:
// the pickup's network under each load, the James stacks, the low end of the amp's path, the
// cabinets, the DI and the mic blended, the studio compressor on a held E1, the cost.
//   g++ -O3 -march=native -ffast-math -std=c++17 -Isrc tools/bassamp.cc src/guitaramp.cc src/bassspeaker.cc src/heard.cc -o bassamp && ./bassamp
// Each line ok/FAIL (as tools/test.py prints them); exit status 1 on a failed check.
#include <chrono>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "guitaramp.h"

using namespace pajupilli;

namespace {
constexpr double kFs = 48000;
int fails = 0;
void check(bool ok, const char* what, const char* detail) {
  std::printf("%s  bass amp: %s: %s\n", ok ? "ok  " : "FAIL", what, detail);
  if (!ok) ++fails;
}
double db(std::complex<double> h) { return 20 * std::log10(std::abs(h) + 1e-30); }
// The response of one part of an amp at f against 200 Hz (dB).
double rel(const GuitarAmp& a, int what, double f, double ref = 200) { return db(a.response(what, f)) - db(a.response(what, ref)); }
// The peak of the pickup's network above 500 Hz: its frequency and height against 200 Hz.
void peak(const GuitarAmp& a, double& f0, double& h0) {
  h0 = -1e9;
  for (double f = 500; f < 15000; f *= 1.002) {
    const double h = rel(a, 0, f);
    if (h > h0) { h0 = h; f0 = f; }
  }
}
// A sine's harmonics 2..10 against its fundamental (rms sum, %): a whole number of periods.
double thd(const std::vector<double>& y, size_t from, size_t n, double f) {
  double fund = 0, rest = 0;
  for (int k = 1; k <= 10; ++k) {
    double re = 0, im = 0;
    for (size_t i = from; i < from + n; ++i) {
      const double ph = 2 * M_PI * k * f * double(i) / kFs;
      re += y[i] * std::cos(ph);
      im += y[i] * std::sin(ph);
    }
    const double p = re * re + im * im;
    if (k == 1) fund = p; else rest += p;
  }
  return 100 * std::sqrt(rest / fund);
}
}  // namespace

int main() {
  char d[512];
  const GuitarAmp::Source P{6.4, 10.5e3, 30e-12, 125e3}, J{1.65, 3.7e3, 90e-12, 83e3};
  // 1. The pickup's network (5.2): P and J into an active DI's 10 MOhm, an amp's 1 MOhm, a
  // passive DI's 140 k.
  {
    double fP[3], hP[3], fJ[3], hJ[3], at2k[3], worst = 0;
    const double loads[3] = {10e6, 1e6, 140e3};
    std::vector<double> rp[3];
    for (int i = 0; i < 3; ++i) {
      GuitarAmp::Preset p = GuitarAmp::fliptopDi();
      p.inputR = loads[i];
      GuitarAmp a(kFs, p);
      a.connect(P, 700e-12);
      peak(a, fP[i], hP[i]);
      at2k[i] = rel(a, 0, 2000);
      for (double f = 30; f < 16000; f *= 1.05) rp[i].push_back(rel(a, 0, f));
      a.connect(J, 700e-12);
      peak(a, fJ[i], hJ[i]);
    }
    for (size_t k = 0; k < rp[0].size(); ++k) worst = std::max(worst, std::fabs(rp[0][k] - rp[1][k]));
    std::snprintf(d, sizeof d, "P into 10 MOhm peaks at %.2f kHz %+.1f dB (research 1.96, +2.4); J both %.2f kHz %+.1f dB (4.05, +4.5)", fP[0] / 1e3, hP[0], fJ[0] / 1e3, hJ[0]);
    check(fP[0] > 1850 && fP[0] < 2050 && std::fabs(hP[0] - 2.4) < 0.5 && std::fabs(fJ[0] - 4000) < 150 && std::fabs(hJ[0] - 4.5) < 0.5, "the pickup's resonance", d);
    std::snprintf(d, sizeof d, "1 MOhm against 10 MOhm within %.2f dB (30 Hz-16 kHz; P: the research's table 0.6 at the peak)", worst);
    check(worst < 0.8, "the input's load", d);
    std::snprintf(d, sizeof d, "P into 140 k: its highest point above 500 Hz %+.1f dB, 2 kHz %+.1f dB (research: no peak, -1.5)", hP[2], at2k[2]);
    check(hP[2] < 0.3 && std::fabs(at2k[2] + 1.5) < 0.7, "a passive DI flattens the P", d);
  }
  // 2. The low end of the amp's path, cabinet off, the stack at noon: E1's 41 Hz against 200 Hz.
  {
    GuitarAmp::Preset p = GuitarAmp::fliptopDi();
    p.treble = p.bass = 0.5;
    p.cabinet = -1;
    GuitarAmp a(kFs, p);
    // (The fliptop's stack itself is 2.9 dB down at 40 Hz at noon (3.1's table: the 270 k from the
    // bass wiper): what is asked of the rest, the couplings, is to keep the E1.)
    const double e1 = rel(a, 3, 41.2) - rel(a, 1, 41.2), c30 = rel(a, 3, 30) - rel(a, 1, 30), st = rel(a, 1, 41.2);
    GuitarAmp::Preset q = GuitarAmp::bassStack();
    q.treble = q.bass = 0.5;
    q.cabinet = -1;
    q.voices = 0;
    GuitarAmp b(kFs, q);
    const double s1 = rel(b, 3, 41.2) - rel(b, 1, 41.2);
    std::snprintf(d, sizeof d, "fliptop-di's couplings %+.2f dB at 41 Hz (%+.2f at 30), bassStack's %+.2f, against 200 Hz (the fliptop's stack at noon itself %+.1f)", e1, c30, s1, st);
    check(e1 > -1 && s1 > -1, "the amp keeps E1's fundamental", d);
  }
  // 3. The James stacks (3.1, 3.2), against noon at the same frequency.
  {
    const auto stack = [](GuitarAmp::Preset p, double bass, double treble) {
      p.bass = bass;
      p.treble = treble;
      return GuitarAmp(kFs, p);
    };
    const GuitarAmp sn = stack(GuitarAmp::bassStack(), 0.5, 0.5), sb = stack(GuitarAmp::bassStack(), 1, 0.5), st = stack(GuitarAmp::bassStack(), 0.5, 1);
    double lo = 1e9, hi = -1e9;
    for (double f = 30; f <= 12000; f *= 1.05) { lo = std::min(lo, db(sn.response(1, f))); hi = std::max(hi, db(sn.response(1, f))); }
    const auto up = [](const GuitarAmp& a, const GuitarAmp& n, double f) { return db(a.response(1, f)) - db(n.response(1, f)); };
    std::snprintf(d, sizeof d, "noon %.1f .. %.1f dB from 30 Hz to 12 kHz (research -22.6 .. -21.5)", lo, hi);
    check(hi - lo < 2 && lo > -23.5 && hi < -20.5, "bass stack stack at noon", d);
    const double b30 = up(sb, sn, 30), b100 = up(sb, sn, 100), t8 = up(st, sn, 8000), t12 = up(st, sn, 12000);
    std::snprintf(d, sizeof d, "bass full %+.1f dB at 30 Hz, %+.1f at 100 (+18, +14.8); treble full %+.1f at 8 kHz, %+.1f at 12 (+9.5, +12.5)", b30, b100, t8, t12);
    check(std::fabs(b30 - 18) < 1 && std::fabs(b100 - 14.8) < 1 && std::fabs(t8 - 9.5) < 1 && std::fabs(t12 - 12.5) < 1.5, "bass stack stack's range", d);
    const GuitarAmp bn = stack(GuitarAmp::fliptopDi(), 0.5, 0.5), bb = stack(GuitarAmp::fliptopDi(), 1, 0.5), bt = stack(GuitarAmp::fliptopDi(), 0.5, 1),
                    bz = stack(GuitarAmp::fliptopDi(), 0.5, 0);
    const double c40 = up(bb, bn, 40), c5 = up(bt, bn, 5000), z5 = up(bz, bn, 5000), ins = db(bn.response(1, 1000));
    std::snprintf(d, sizeof d, "insertion %.1f dB at 1 kHz (-23.1); bass full %+.1f at 40 Hz (+14.9); treble full %+.1f at 5 kHz (+18.8), zero %+.1f (-17.4)", ins, c40, c5, z5);
    check(std::fabs(ins + 23.1) < 1 && std::fabs(c40 - 14.9) < 1 && std::fabs(c5 - 18.8) < 1 && std::fabs(z5 + 17.4) < 1, "fliptop stack", d);
  }
  // 4. The cabinets (4.3), against 200 Hz.
  {
    // (Each with its own head: the physical speakers know what drives them.)
    const GuitarAmp c3(kFs, GuitarAmp::bassStack()), c4(kFs, GuitarAmp::fliptopDi()), c5(kFs, GuitarAmp::slapDi());
    if (std::getenv("PJ_BASSAMP_CURVES"))
      for (double f : {30., 35., 40., 45., 50., 58., 70., 80., 100., 150., 300., 500., 1000., 2000., 3000., 4000., 5000., 8000.})
        std::printf("  %5.0f Hz: 8x10 %+6.1f  1x15 %+6.1f  4x10 %+6.1f\n", f, rel(c3, 2, f), rel(c4, 2, f), rel(c5, 2, f));
    const double a = rel(c3, 2, 40), b = rel(c3, 2, 58), c = rel(c3, 2, 5000), e = rel(c4, 2, 40), f = rel(c4, 2, 5000), g = rel(c5, 2, 40), h = rel(c5, 2, 5000);
    std::snprintf(d, sizeof d, "8x10 %+.1f / %+.1f / %+.1f dB at 40 Hz / 58 / 5 kHz (the 810E's -10, -3; -20); 1x15 %+.1f / %+.1f at 40 / 5 kHz (-4, -23); 4x10+horn %+.1f / %+.1f (-5.5, -1.6)", a, b, c, e, f, g, h);
    check(std::fabs(a + 10) < 1 && std::fabs(b + 3) < 1 && std::fabs(c + 20) < 1 && std::fabs(e + 4) < 1 && std::fabs(f + 23) < 2 && std::fabs(g + 5.5) < 1 &&
              std::fabs(h + 1.6) < 2,  // (the physical speakers: the 1x15 on its tube head a little brighter, the 4x10 a little duller, than the fitted filters)
          "the cabinets", d);
  }
  // 6. The blend: the DI and the mic (fliptop-di; bassStack) no weaker than the louder alone at 40-200 Hz,
  // no notch deeper than 3 dB under 2 kHz.
  {
    for (GuitarAmp::Preset p : {GuitarAmp::fliptopDi(), GuitarAmp::bassStack()}) {
      GuitarAmp a(kFs, p);
      double low = 1e9, all = 1e9, lowF = 0, allF = 0;
      for (double f = 30; f <= 2000; f *= 1.02) {
        const double louder = std::max(db(p.blend * a.response(4, f)), db((1 - p.blend) * a.response(3, f))), v = db(a.response(5, f)) - louder;
        if (f >= 40 && f <= 200 && v < low) { low = v; lowF = f; }
        if (v < all) { all = v; allF = f; }
      }
      GuitarAmp::Preset q = p;
      q.diAlign = false;
      GuitarAmp b(kFs, q);
      double raw = 1e9;
      for (double f = 30; f <= 2000; f *= 1.02)
        raw = std::min(raw, db(b.response(5, f)) - std::max(db(q.blend * b.response(4, f)), db((1 - q.blend) * b.response(3, f))));
      std::snprintf(d, sizeof d, "%s: the blend against the louder alone %+.1f dB at worst at 40-200 Hz (%.0f Hz), %+.1f under 2 kHz (%.0f Hz); with the polarity alone, not lined up, %+.1f", p.name, low, lowF, all, allF, raw);
      check(low > -0.5 && all > -3, "the DI and the mic lined up", d);
    }
  }
  // 7. The studio compressor on a held E1 (a sine, the DI alone): its gain reduction settled by
  // 0.3 s; what it adds to the distortion.
  {
    const double f = 41.2;
    const int n = int(2.0 * kFs);
    std::vector<double> y[2];
    for (int k = 0; k < 2; ++k) {
      GuitarAmp::Preset p = GuitarAmp::fliptopDi();
      p.blend = 1;
      p.dis = 0;
      p.studio = k ? 1 : 0;
      GuitarAmp a(kFs, p);
      a.connect(J, 700e-12);
      for (int i = 0; i < n; ++i) y[k].push_back(a.process(float(0.25 * std::sin(2 * M_PI * f * i / kFs))));
    }
    const auto lvl = [&](const std::vector<double>& v, double t0, double t1) {
      double s = 0;
      for (int i = int(t0 * kFs); i < int(t1 * kFs); ++i) s += v[i] * v[i];
      return 10 * std::log10(s / (int(t1 * kFs) - int(t0 * kFs)) + 1e-30);
    };
    const GuitarAmp::Preset p = GuitarAmp::fliptopDi();
    const double grEnd = p.stMakeup - (lvl(y[1], 1.5, 2.0) - lvl(y[0], 1.5, 2.0)), gr3 = p.stMakeup - (lvl(y[1], 0.3, 0.4) - lvl(y[0], 0.3, 0.4));
    const size_t per = size_t(std::lround(kFs / f * 41));  // (41 periods: 0.995 s)
    const double t0 = thd(y[0], size_t(kFs), per, f), t1 = thd(y[1], size_t(kFs), per, f);
    std::snprintf(d, sizeof d, "a 0.25 V E1: %.1f dB of gain reduction at 1.5-2 s, %.1f at 0.3-0.4 s", grEnd, gr3);
    check(std::fabs(gr3 - grEnd) < 1 && grEnd > 1, "the optical compressor settles", d);
    std::snprintf(d, sizeof d, "THD %.3f %% with it, %.3f %% without", t1, t0);
    check(t1 - t0 < 0.5, "the optical compressor leaves a held E1 clean", d);
  }
  // Every preset runs (a plucked-like burst, then silence): finite; the cost of each.
  {
    for (GuitarAmp::Preset p : {GuitarAmp::fliptopDi(), GuitarAmp::bassStack(), GuitarAmp::slapDi(), GuitarAmp::slapSquash()}) {
      GuitarAmp a(kFs, p);
      a.connect(J, 700e-12);
      const int n = int(4 * kFs);
      bool finite = true;
      double peakOut = 0, sink = 0;
      const auto t0 = std::chrono::steady_clock::now();
      for (int i = 0; i < n; ++i) {
        const double t = std::fmod(i / kFs, 1.0);
        double v = 0;
        for (int k = 1; k < 20; ++k) v += 0.6 / k * std::exp(-t * (1 + 0.5 * k)) * std::sin(2 * M_PI * 41.2 * k * t);
        const float o = a.process(float(i < 3 * kFs ? v : 0));
        finite = finite && std::isfinite(o);
        peakOut = std::max(peakOut, double(std::fabs(o)));
        sink += o;
      }
      const double ns = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() / n * 1e9;
      std::snprintf(d, sizeof d, "%s: peak %.2f, %.0f ns a sample (%.2f %% of a core)%s", p.name, peakOut, ns, ns / 20833 * 100, sink == 12345 ? "" : "");
      check(finite && peakOut < 20, "runs", d);
    }
  }
  return fails ? 1 : 0;
}
