// Standalone test of the hum filter (src/hum.h) on its own and on a Singer.
//   g++ -O3 -march=native -ffast-math -std=c++17 -Isrc tools/humtest.cc -o humtest && ./humtest
// Checks: amount 0 is the identity; at amount 1 the murmur's shape (the pole
// up, the 1 kHz zero down, the highs down); a sung ooh -> hum against an aah
// (levels, band shares); a glide 0 -> 1 -> 0 stays finite and click-free; the
// cost per sample. Exit status 1 on a failed check.
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

#include "../src/hum.cc"
#include "../src/singer.cc"

using namespace pajupilli;

namespace {
constexpr double kFs = 48000;
int fails = 0;
void check(bool ok, const char* what) {
  std::printf("%s  %s\n", ok ? "ok  " : "FAIL", what);
  if (!ok) ++fails;
}
double dbAt(const std::vector<double>& h, double f) {
  double re = 0, im = 0;
  for (size_t i = 0; i < h.size(); ++i) re += h[i] * std::cos(2 * M_PI * f * i / kFs), im -= h[i] * std::sin(2 * M_PI * f * i / kFs);
  return 10 * std::log10(re * re + im * im + 1e-30);
}
// Energy (dB) of x in [f0, f1) by a direct DFT on a 1-s block (1 Hz bins).
double bandDb(const std::vector<float>& x, double f0, double f1) {
  double e = 0;
  const int n = int(x.size());
  for (int f = int(f0); f < int(f1); f += 5) {
    double re = 0, im = 0;
    for (int i = 0; i < n; i += 2) {  // (decimated by 2 for speed; < 12 kHz is all we ask)
      const double w = 0.5 - 0.5 * std::cos(2 * M_PI * i / n);
      re += w * x[i] * std::cos(2 * M_PI * f * i / kFs), im += w * x[i] * std::sin(2 * M_PI * f * i / kFs);
    }
    e += re * re + im * im;
  }
  return 10 * std::log10(e + 1e-30);
}
double rmsDb(const std::vector<float>& x) {
  double e = 0;
  for (float v : x) e += double(v) * v;
  return 10 * std::log10(e / x.size() + 1e-30);
}
// A Singer at f0 on a vowel (0 u .. 2 a), through a hum at amount, 1.5 s; the last second.
std::vector<float> sing(double vowel, double amount, double f0 = 220) {
  Singer s;
  s.init(kFs, 3, 1);
  Hum h;
  h.init(kFs);
  const int blk = 16, n = int(1.5 * kFs);
  std::vector<float> y;
  for (int i = 0; i < n; i += blk) {
    s.control(f0, vowel, 800, 0.2, 0, 5, 55, blk / kFs);
    h.control(amount, blk / kFs);
    for (int k = 0; k < blk; ++k) {
      const float v = h.process(float(s.sample()));
      if (i >= int(0.5 * kFs)) y.push_back(v);
    }
  }
  return y;
}
}  // namespace

int main() {
  // 1. Identity at amount 0.
  {
    Hum h;
    h.init(kFs);
    h.control(0, 0.001);
    double err = 0;
    for (int i = 0; i < 4800; ++i) {
      const float x = std::sin(0.37 * i) + (i == 10 ? 1 : 0);
      err = std::max(err, double(std::fabs(h.process(x) - x)));
    }
    std::printf("amount 0: max |y - x| %.2e\n", err);
    check(err < 1e-5, "amount 0 passes the input unchanged");
  }
  // 2. The murmur's response at amount 1.
  {
    Hum h;
    h.init(kFs);
    for (int i = 0; i < 100; ++i) h.control(1, 0.01);
    std::vector<double> ir(9600);
    for (size_t i = 0; i < ir.size(); ++i) ir[i] = h.process(i == 0 ? 1.0f : 0.0f);
    const double f[] = {100, 270, 500, 800, 1000, 1500, 2000, 3000, 5000};
    std::printf("amount 1 response (dB):");
    for (double v : f) std::printf("  %.0f:%+.1f", v, dbAt(ir, v));
    std::printf("\n");
    check(dbAt(ir, 270) - dbAt(ir, 1000) > 12, "the zero: 1 kHz > 12 dB under the pole");
    check(dbAt(ir, 270) - dbAt(ir, 3000) > 15, "the highs: 3 kHz > 15 dB under the pole");
  }
  // 3. On a Singer (the current kVowels rows: 0 u, 2 a), 220 Hz, 800 Pa.
  {
    const std::vector<float> aah = sing(2, 0), ooh = sing(0, 0), hum = sing(0, 1);
    const double ra = rmsDb(aah);
    auto row = [&](const char* name, const std::vector<float>& x) {
      const double r = rmsDb(x), lo = bandDb(x, 100, 500), mid = bandDb(x, 500, 1500), hi = bandDb(x, 1500, 5000);
      std::printf("%-4s rms %+.1f dB vs aah;  bands <500 / 500-1500 / 1.5-5k: %+.1f / %+.1f / %+.1f dB (re <500)\n", name,
                  r - ra, lo - lo, mid - lo, hi - lo);
      return r - ra;
    };
    row("aah", aah);
    row("ooh", ooh);
    const double dh = row("hum", hum);
    check(dh < -8 && dh > -20, "the hum 8-20 dB under the aah (research: 10-15)");
    check(bandDb(hum, 1500, 5000) - bandDb(hum, 100, 500) < bandDb(ooh, 1500, 5000) - bandDb(ooh, 100, 500) - 6,
          "the hum's highs at least 6 dB further down than the ooh's");
  }
  // 4. A glide 0 -> 1 -> 0 on a sine: finite, no jump larger than the signal's own steps.
  {
    Hum h;
    h.init(kFs);
    double maxStep = 0, prev = 0;
    bool finite = true;
    for (int i = 0; i < int(kFs); ++i) {
      if (i % 16 == 0) h.control(i < kFs / 3 || i > 2 * kFs / 3 ? 0 : 1, 16 / kFs);
      const double y = h.process(float(0.5 * std::sin(2 * M_PI * 220 * i / kFs)));
      finite = finite && std::isfinite(y);
      if (i > 0) maxStep = std::max(maxStep, std::fabs(y - prev));
      prev = y;
    }
    std::printf("glide: max step %.4f (a 220 Hz sine of 0.5: 0.0144; x up to ~3 for the hum's gain)\n", maxStep);
    check(finite && maxStep < 0.06, "glide 0 -> 1 -> 0 finite, click-free");
  }
  // 5. The cost.
  {
    Hum h;
    h.init(kFs);
    h.control(1, 0.1);
    const int n = 20000000;
    float x = 0.1f, acc = 0;
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < n; ++i) {
      if ((i & 15) == 0) h.control((i >> 20) & 1, 16 / kFs);
      acc += h.process(x);
      x = -x * 0.999f + 1e-3f;
    }
    const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count() / n;
    std::printf("cost: %.2f ns a sample (with control every 16), %.4f %% of a core at 48 kHz  (%g)\n", ns,
                ns * 48000 * 1e-9 * 100, acc);
  }
  std::printf(fails ? "%d FAILED\n" : "all passed\n", fails);
  return fails ? 1 : 0;
}
