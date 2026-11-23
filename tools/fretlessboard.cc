// The fretless's board and finger in Guitar (GuitarSpec::board, Guitar::stop; research/
// fretless_bass.md 2, 8.2, 8.6): tools/test.py builds and runs this with src/guitar.cc,
// src/bassspec.cc and src/fretlessspec.cc and reads its "ok  " / "FAIL" lines.
//   - stable: 1000 random plucks, slides and vibratos (every string, positions 0-24, soft to
//     hard), every sample finite;
//   - an open string never touches the board, at any depth;
//   - convergence: a hard stopped A1 at 48 and 96 kHz: its harmonics at 0.1, 0.3 and 0.6 s agree
//     within 1.5 dB (test 6);
//   - the glide keeps each mode's action: a lossless string slid 5 semitones up keeps E / w within
//     1 dB (its amplitude; test 8), and so does the string's sound at the pickup, as predicted;
//   - off (the fretted basses' default) nothing touches, stop() at whole frets is fret().
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "../src/ebass.h"
#include "../src/fretless.h"
#include "../src/guitar.h"

using namespace pajupilli;

namespace {
int fails = 0;
void check(bool ok, const char* name, const char* detail) {
  std::printf("%s  %s: %s\n", ok ? "ok  " : "FAIL", name, detail);
  if (!ok) ++fails;
}
uint32_t rng = 4711;
double ur() {
  rng = rng * 1664525u + 1013904223u;
  return (rng >> 8) * (1.0 / 16777216.0);
}
// Harmonic k's level (dB) around time t: a 4-period Hann window, as the research's analysis.
double harm(const std::vector<float>& x, double fs, double f1, int k, double t) {
  const int win = int(4 * fs / f1), at = int(t * fs) - win / 2;
  std::complex<double> s = 0;
  double wsum = 0;
  for (int i = 0; i < win; ++i) {
    const double w = 0.5 - 0.5 * std::cos(2 * M_PI * i / (win - 1));
    s += w * x[at + i] * std::polar(1.0, -2 * M_PI * k * f1 * i / fs);
    wsum += w;
  }
  return 20 * std::log10(2 * std::abs(s) / wsum + 1e-15);
}
std::vector<float> hardA1(double fs, int* hits) {
  Guitar g(fs, fretlessBass());
  for (int s = 1; s < 4; ++s) g.damp(s, 40);
  g.stop(0, 5);
  g.pluckPad(0, 4.5e-3, 0.085, 0.65, 0.018, 0.0036, 0.0028);
  std::vector<float> x(size_t(0.8 * fs));
  *hits = 0;
  for (float& y : x) {
    y = g.sample();
    *hits += g.boardHits(0) > 0;
  }
  return x;
}
}  // namespace

int main() {
  char d[300];
  {
    // Stable.
    Guitar g(48000, fretlessBass());
    bool fine = true;
    double pos[4] = {};
    for (int n = 0; n < 1000 && fine; ++n) {
      const int s = int(ur() * 4);
      pos[s] = ur() < 0.15 ? 0 : 24 * ur();
      g.stop(s, pos[s]);
      g.pluckPad(s, 0.2e-3 + 6e-3 * ur(), 0.04 + 0.2 * ur(), 1.4 * ur(), 0.018, 0.002 + 0.004 * ur(), 0.001 + 0.004 * ur());
      const int len = 200 + int(ur() * 4000);
      const double to = pos[s] > 0 ? std::max(0.3, pos[s] + 8 * (ur() - 0.5)) : 0;
      for (int i = 0; i < len; ++i) {
        if (i % 16 == 0 && pos[s] > 0) g.stop(s, pos[s] + (to - pos[s]) * i / len + 0.2 * std::sin(i * 0.002));
        if (!std::isfinite(g.sample())) { fine = false; break; }
      }
      pos[s] = to;
    }
    check(fine, "fretless board: stable", "1000 random plucks with slides and vibrato, every sample finite");
  }
  {
    // The open strings never touch.
    int touched = 0;
    for (int s = 0; s < 4; ++s)
      for (double depth : {1e-3, 4e-3, 8e-3}) {
        Guitar g(48000, fretlessBass());
        g.stop(s, 0);
        g.pluckPad(s, depth, 0.085, 1.2, 0.018, 0.003, 0.002);
        for (int i = 0; i < 24000; ++i) {
          g.sample();
          touched += g.boardHits(s);
        }
      }
    std::snprintf(d, sizeof d, "four strings at 1, 4, 8 mm: %d samples touching", touched);
    check(touched == 0, "fretless board: open strings never touch", d);
  }
  {
    // Convergence: 48 against 96 kHz.
    int h48, h96;
    const std::vector<float> a = hardA1(48000, &h48), b = hardA1(96000, &h96);
    double worst = 0;
    std::string where;
    const double f1 = 55.0;
    for (double t : {0.1, 0.3, 0.6})
      for (int k = 1; k <= 12; ++k) {
        const double la = harm(a, 48000, f1 * std::sqrt(1 + 2e-4 * k * k), k, t), lb = harm(b, 96000, f1 * std::sqrt(1 + 2e-4 * k * k), k, t);
        const double ref = harm(a, 48000, f1, 1, 0.1);
        if (la - ref < -40 && lb - ref < -40) continue;  // (both far down: noise)
        if (std::fabs(la - lb) > worst) {
          worst = std::fabs(la - lb);
          char w[64];
          std::snprintf(w, sizeof w, "h%d at %.1f s", k, t);
          where = w;
        }
      }
    std::snprintf(d, sizeof d, "a hard A1 on the E string, h1-h12 at 0.1/0.3/0.6 s: at most %.2f dB apart (%s); touching %.2f %% / %.2f %% of the samples",
                  worst, where.c_str(), 100.0 * h48 / a.size(), 100.0 * h96 / b.size());
    check(worst < 1.5 && h48 > 0, "fretless board: 48 kHz as 96 kHz", d);
  }
  {
    // The glide keeps the action: lossless, no board, no finger loss, no neck.
    GuitarSpec sp = fretlessBass();
    for (auto& s : sp.str) s.loss0 = s.loss1 = 0;
    sp.body.clear();
    sp.board = false;
    sp.fingerLoss = 0;
    Guitar g(48000, sp);
    g.stop(1, 3);
    g.pluckPad(1, 2e-3, 0.085, 0.6, 0.018, 0.003, 0.002);
    for (int i = 0; i < 9600; ++i) g.sample();
    const double e0 = g.energy(1), w0 = g.frequency(1);
    const int n = 6720;  // 140 ms
    for (int i = 0; i < n; ++i) {
      if (i % 16 == 0) g.stop(1, 3 + 5 * (0.5 - 0.5 * std::cos(M_PI * double(i) / n)));
      g.sample();
    }
    g.stop(1, 8);
    for (int i = 0; i < 4800; ++i) g.sample();
    const double e1 = g.energy(1), w1 = g.frequency(1);
    const double db = 10 * std::log10((e1 / w1) / (e0 / w0));
    std::snprintf(d, sizeof d, "C2 to F2 on the A string in 140 ms: the energy x%.3f, the frequency x%.3f: E / w %+.2f dB", e1 / e0, w1 / w0, db);
    check(std::fabs(db) < 1, "fretless glide: the action kept", d);
  }
  {
    // Off: the fretted bass's Guitar untouched; stop() at whole frets as fret().
    Guitar a(48000, twoPickupBass()), b(48000, twoPickupBass());
    a.fret(1, 5);
    b.stop(1, 5);
    a.pluckPad(1, 2e-3, 0.1, 0.4, 0.010, 0.002, 0.001);
    b.pluckPad(1, 2e-3, 0.1, 0.4, 0.010, 0.002, 0.001);
    double diff = 0;
    for (int i = 0; i < 24000; ++i) diff = std::max(diff, double(std::fabs(a.sample() - b.sample())));
    std::snprintf(d, sizeof d, "the two-pickup bass's D2 by fret() and by stop(): largest difference %.3g", diff);
    check(diff == 0, "fretless glide: stop() at a fret is fret()", d);
  }
  return fails ? 1 : 0;
}
