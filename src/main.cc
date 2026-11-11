// pajupilli: a physical model of a Boehm concert flute.
//
//   ./flute -note 69 -o a4.wav        play one note (MIDI), print its pitch
//   ./flute -scale                    play C4..C7, print a tuning table
//   ./flute -imp 69                   passive resonances of a fingering
//   ./flute -live                     play it from the MPK mini mk3
//       [-dev hw:0] [-midi MPK] [-period 64] [-periods 3] [-transpose 12] [-v] [-selftest]
#include <cmath>
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
  static char buf[8];
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

int main(int argc, char** argv) {
  int note = -1, imp = -1;
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
    else if (a == "-o") out = next();
    else if (a == "-dump") dump = next();
    else { std::fprintf(stderr, "unknown option %s\n", a.c_str()); return 1; }
  }
  if (imp >= 0) { impulse(imp); return 0; }
  if (!std::getenv("PJ_NOCAL")) gCal = calibrate(440, nullptr);
  if (live) return runLive(lo, gCal);
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
  if (out && !all.empty()) writeWav(out, all, kFs);
  return 0;
}
