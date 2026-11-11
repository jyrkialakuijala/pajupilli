// pajupilli: a physical model of a Boehm concert flute.
//
//   ./flute -note 69 -o a4.wav        play one note (MIDI), print its pitch
//   ./flute -scale                    play C4..C7, print a tuning table
//   ./flute -imp 69                   passive resonances of a fingering
//   ./flute -slur 71 72 -dyn 0.2      slur into a note; print its pitch and level wobble
//   ./flute -live                     play it from the MPK mini mk3
//       [-dev hw:0] [-midi MPK] [-period 64] [-periods 3] [-transpose 12] [-v] [-selftest]
#include <cmath>
#include <random>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "analysis.h"
#include "calibrate.h"
#include "flute.h"
#include "live.h"
#include "player.h"

using namespace pajupilli;

namespace {

constexpr int kFs = 48000;

struct Result { double f, clarity, rms; };

Calibration gCal;

Result playNote(int midi, double secs, double dyn, std::vector<float>* keep) {
  Flute fl(kFs);
  Player pl(fl);
  pl.setCalibration(gCal);
  pl.setDynamics(dyn);
  const int n = int(secs * kFs);
  std::vector<float> x(n);
  pl.noteOn(midi, 90);
  pl.render(x.data(), n);
  const int a = n * 2 / 3, len = n - a;
  const double f0 = noteFrequency(midi);
  Result r;
  r.f = estimatePitch(x.data() + a, len, kFs, f0 / 2.2, f0 * 2.2, &r.clarity);
  r.rms = rms(x.data() + a, len);
  if (keep) keep->insert(keep->end(), x.begin(), x.end());
  return r;
}

double cents(double f, double ref) { return 1200 * std::log2(f / ref); }

const char* noteName(int m) {
  static const char* n[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B"};
  static char buf[16];
  std::snprintf(buf, sizeof buf, "%s%d", n[m % 12], m / 12 - 1);
  return buf;
}

// Passive resonances seen by the jet: a pressure impulse in the window branch,
// the window flow's spectrum; peaks of |Y|.
void impulse(int midi) {
  Flute fl(kFs);
  fl.setHoles(noteInfo(midi)->holes);
  std::vector<float> tmp(kFs / 10);
  fl.process(tmp.data(), int(tmp.size()));  // let the pads settle
  const int n = 1 << 16;
  std::vector<double> q(n);
  for (int i = 0; i < n; ++i) {
    fl.testDrive(i == 0 ? 1.0 : 0.0);
    float y;
    fl.process(&y, 1);
    q[i] = fl.windowFlow();
  }
  // DFT magnitude on a fine grid, peaks below 4 kHz.
  std::printf("%s fingering %s, admittance peaks (Hz):", noteName(midi), noteInfo(midi)->holes);
  double prev2 = 0, prev1 = 0;
  const double df = 0.5;
  for (double f = 100; f < 4000; f += df) {
    double re = 0, im = 0;
    const double w = 2 * M_PI * f / kFs;
    // Goertzel
    double s1 = 0, s2 = 0;
    const double c = 2 * std::cos(w);
    for (int i = 0; i < n; ++i) { const double s0 = q[i] + c * s1 - s2; s2 = s1; s1 = s0; }
    re = s1 - s2 * std::cos(w);
    im = s2 * std::sin(w);
    const double mag = re * re + im * im;
    if (prev1 > prev2 && prev1 > mag) std::printf(" %.1f", f - df);
    prev2 = prev1;
    prev1 = mag;
  }
  std::printf("  (target %.1f)\n", noteFrequency(midi));
}

}  // namespace

// Fuzz: random notes, velocities and controls for `secs`; reports NaN resets
// and the peak. Exit 1 on any non-finite sample or reset.
int fuzz(double secs, unsigned seed) {
  Flute fl(kFs);
  Player pl(fl);
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> u(0, 1);
  std::vector<float> x(64);
  double peak = 0;
  bool bad = false;
  int held = -1;
  for (long n = 0; n < long(secs * kFs); n += 64) {
    if (u(rng) < 0.02) {
      const int m = 60 + int(u(rng) * 37);
      pl.noteOn(m, 1 + int(u(rng) * 126));
      if (held >= 0 && u(rng) < 0.7) pl.noteOff(held);
      held = m;
    }
    if (u(rng) < 0.005 && held >= 0) { pl.noteOff(held); held = -1; }
    if (u(rng) < 0.01) pl.setDynamics(u(rng));
    if (u(rng) < 0.01) pl.setLipRoll(2 * u(rng) - 1);
    if (u(rng) < 0.01) pl.setBreath(u(rng) - 0.5);
    if (u(rng) < 0.005) pl.setFlutter(u(rng) < 0.5 ? 0 : u(rng));
    if (u(rng) < 0.005) pl.setVibrato(0.15 * u(rng));
    if (u(rng) < 0.003) pl.setHold(u(rng) < 0.5);
    pl.render(x.data(), 64);
    for (float v : x) { bad |= !std::isfinite(v); peak = std::max(peak, double(std::fabs(v))); }
  }
  std::printf("fuzz %.0f s seed %u: peak %.3f, resets %d, %s\n", secs, seed, peak, fl.resets(),
              bad || fl.resets() ? "FAIL" : "ok");
  return bad || fl.resets() ? 1 : 0;
}

// A NaN injected into the window must not silence the flute for good.
int nanTest() {
  Flute fl(kFs);
  Player pl(fl);
  pl.noteOn(69, 80);
  std::vector<float> x(kFs / 2);
  pl.render(x.data(), int(x.size()));
  fl.testDrive(NAN);
  pl.render(x.data(), int(x.size()));
  const double r = rms(x.data() + x.size() / 2, int(x.size() / 2));
  bool finite = true;
  for (float v : x) finite &= std::isfinite(v);
  std::printf("nan recovery: resets %d, finite %d, rms after %.4f: %s\n", fl.resets(), finite, r,
              fl.resets() == 1 && finite && r > 1e-3 ? "ok" : "FAIL");
  return fl.resets() == 1 && finite && r > 1e-3 ? 0 : 1;
}

// Slur from `from` into `to` (as on the keyboard: overlap, then release) and
// measure the settled note in 20 ms frames: mean pitch, pitch wobble (std of
// cents), level wobble (peak-to-peak / mean) and frames in another register.
int slurTest(int from, int to, double dyn, double secs, std::vector<float>* keep) {
  Flute fl(kFs);
  Player pl(fl);
  pl.setCalibration(gCal);
  pl.setDynamics(dyn);
  std::vector<float> a(int(0.4 * kFs)), x(int(secs * kFs));
  pl.noteOn(from, 70);
  pl.render(a.data(), int(a.size()));
  pl.noteOn(to, 70);
  pl.noteOff(from);
  pl.render(x.data(), int(x.size()));
  if (keep) { keep->insert(keep->end(), a.begin(), a.end()); keep->insert(keep->end(), x.begin(), x.end()); }
  const double f0 = noteFrequency(to);
  const int n = 960;  // 20 ms
  std::vector<double> cs, ls;
  int other = 0;
  for (int i = int(0.15 * kFs); i + n <= int(x.size()); i += n) {
    const double f = estimatePitch(x.data() + i, n, kFs, 150, 3000);
    const double c = f > 0 ? cents(f, f0) : 1e4;
    if (std::fabs(c) > 300) { ++other; continue; }
    cs.push_back(c);
    ls.push_back(rms(x.data() + i, n));
  }
  double m = 0, v = 0, lm = 0, lmin = 1e9, lmax = 0;
  for (double c : cs) m += c;
  m /= std::max<size_t>(1, cs.size());
  for (double c : cs) v += (c - m) * (c - m);
  v = std::sqrt(v / std::max<size_t>(1, cs.size()));
  for (double l : ls) { lm += l; lmin = std::min(lmin, l); lmax = std::max(lmax, l); }
  lm /= std::max<size_t>(1, ls.size());
  const std::string fromName = noteName(from), toName = noteName(to);
  std::printf("slur %s -> %s dyn %.2f: mean %+.0f cents, wobble %.1f cents, level %.4f wobble %.0f %%, "
              "other register %d/%d frames\n", fromName.c_str(), toName.c_str(), dyn, m, v, lm,
              ls.empty() ? 0.0 : 100 * (lmax - lmin) / lm, other, other + int(cs.size()));
  return 0;
}

int main(int argc, char** argv) {
#if defined(__SSE__)
  // Flush denormals to zero: decaying states must not slow the real-time path
  // (fast-math links this in too, the portable build does not).
  _mm_setcsr(_mm_getcsr() | 0x8040);
#endif
  int note = -1, imp = -1, slurFrom = -1, slurTo = -1;
  bool scale = false, demo = false, live = false;
  LiveOptions lo;
  double secs = 1.0, dyn = 0.5;
  const char* out = nullptr;
  const char* dump = nullptr;  // window pressure, raw float32
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto next = [&]() { return i + 1 < argc ? argv[++i] : (char*)""; };
    if (a == "-note") note = std::atoi(next());
    else if (a == "-scale") scale = true;
    else if (a == "-cal") { calibrate(440, stdout); return 0; }
    else if (a == "-imp") imp = std::atoi(next());
    else if (a == "-slur") { slurFrom = std::atoi(next()); slurTo = std::atoi(next()); }
    else if (a == "-secs") secs = std::atof(next());
    else if (a == "-dyn") dyn = std::atof(next());
    else if (a == "-demo") demo = true;
    else if (a == "-live") live = true;
    else if (a == "-selftest") { live = true; lo.selfTest = true; }
    else if (a == "-dev") lo.device = next();
    else if (a == "-midi") lo.midiName = next();
    else if (a == "-period") lo.period = std::atoi(next());
    else if (a == "-periods") lo.periods = std::atoi(next());
    else if (a == "-transpose") lo.transpose = std::atoi(next());
    else if (a == "-v") lo.verbose = true;
    else if (a == "-fuzz") { const double fs = std::atof(next()); return fuzz(fs, unsigned(std::atoi(next()))); }
    else if (a == "-nantest") return nanTest();
    else if (a == "-o") out = next();
    else if (a == "-dump") dump = next();
    else { std::fprintf(stderr, "unknown option %s\n", a.c_str()); return 1; }
  }
  if (imp >= 0) {
    if (!noteInfo(imp)) { std::fprintf(stderr, "-imp: MIDI note must be 60..96\n"); return 1; }
    impulse(imp);
    return 0;
  }
  if (note >= 0 && !noteInfo(note)) { std::fprintf(stderr, "-note: MIDI note must be 60..96\n"); return 1; }
  if (slurFrom >= 0 && (!noteInfo(slurFrom) || !noteInfo(slurTo))) {
    std::fprintf(stderr, "-slur: MIDI notes must be 60..96\n");
    return 1;
  }
  if (!std::getenv("PJ_NOCAL")) {
    // One note: learn only that one (fast); otherwise the whole range.
    if (note >= 60 && !scale && !demo && !live) gCal = calibrate(440, nullptr, note, note);
    else gCal = calibrate(440, nullptr);
  }
  if (live) return runLive(lo, gCal);
  std::vector<float> slurOut;
  if (slurFrom >= 0) {
    slurTest(slurFrom, slurTo, dyn, secs, out ? &slurOut : nullptr);
    if (out && !writeWav(out, slurOut, kFs)) { std::fprintf(stderr, "cannot write %s\n", out); return 1; }
    return 0;
  }
  if (std::getenv("PJ_TRACE") && note >= 0) {
    Flute fl(kFs);
    Player pl(fl);
    pl.noteOn(note, 90);
    for (int i = 0; i < int(secs * kFs); ++i) {
      float y;
      pl.render(&y, 1);
      std::printf("%d %g %g %g %g %g %g\n", i, y, fl.windowFlow(), fl.jetSplit(),
                  fl.windowPressure(), fl.mouthPressure(), fl.jetVelocity());
    }
    return 0;
  }
  std::vector<float> all;
  if (note >= 0 && dump) {
    Flute fl(kFs);
    Player pl(fl);
    pl.setDynamics(dyn);
    pl.noteOn(note, 90);
    std::vector<float> pw(int(secs * kFs));
    for (float& v : pw) { float y; pl.render(&y, 1); v = float(fl.windowPressure()); }
    FILE* f = std::fopen(dump, "wb");
    if (!f) { std::fprintf(stderr, "cannot write %s\n", dump); return 1; }
    std::fwrite(pw.data(), 4, pw.size(), f);
    std::fclose(f);
    return 0;
  }
  if (note >= 0) {
    const Result r = playNote(note, secs, dyn, &all);
    std::printf("%s target %.1f Hz: played %.1f Hz (%+.0f cents) clarity %.2f rms %.4f\n",
                noteName(note), noteFrequency(note), r.f, cents(r.f, noteFrequency(note)),
                r.clarity, r.rms);
  }
  if (scale) {
    for (int m = 60; m <= 96; ++m) {
      const Result r = playNote(m, secs, dyn, &all);
      std::printf("%-4s %7.1f %7.1f %+6.0f  clarity %.2f rms %.4f\n", noteName(m),
                  noteFrequency(m), r.f, cents(r.f, noteFrequency(m)), r.clarity, r.rms);
    }
  }
  if (demo) {
    // A scale up and down (slurred), then a tongued phrase with vibrato.
    Flute fl(kFs);
    Player pl(fl);
    pl.setCalibration(gCal);
    pl.setDynamics(dyn);
    auto play = [&](double s) { std::vector<float> x(int(s * kFs)); pl.render(x.data(), int(x.size())); all.insert(all.end(), x.begin(), x.end()); };
    const int up[] = {60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84};
    int prev = -1;
    for (int m : up) { pl.noteOn(m, 70); if (prev >= 0) pl.noteOff(prev); prev = m; play(0.22); }
    for (int i = 13; i >= 0; --i) { pl.noteOn(up[i], 70); pl.noteOff(prev); prev = up[i]; play(0.22); }
    pl.noteOff(prev); play(0.5);
    pl.setVibrato(0.08);
    const int tune[][2] = {{72, 4}, {76, 2}, {79, 2}, {84, 6}, {83, 2}, {81, 2}, {79, 2}, {77, 4}, {76, 4}, {74, 2}, {72, 6}};
    for (auto& t : tune) { pl.noteOn(t[0], 90); play(t[1] * 0.16); pl.noteOff(t[0]); play(0.03); }
    play(0.8);
  }
  if (out && !all.empty() && !writeWav(out, all, kFs)) {
    std::fprintf(stderr, "cannot write %s\n", out);
    return 1;
  }
  return 0;
}
