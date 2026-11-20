// The held pick (Guitar::pickContact; research/pick_bass.md 9.6 items 2-3), on its own: the
// Precision's open E (src/bassspec.cc), the other strings muted, the pickup's EMF (no DI, no amp:
// what is compared is ratios). Writes each case as float32 at 48 kHz into OUTDIR for tools/
// test.py to measure; runs 1000 random picks over ringing strings and prints what came out.
//   pickcontact OUTDIR
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "../src/ebass.h"
#include "../src/guitar.h"

using namespace pajupilli;

namespace {
constexpr double kFs = 48000;

void write(const std::string& path, const std::vector<float>& x) {
  FILE* f = std::fopen(path.c_str(), "wb");
  if (!f) return;
  std::fwrite(x.data(), sizeof(float), x.size(), f);
  std::fclose(f);
}

template <class Strike>
std::vector<float> note(Strike strike, double secs = 1.0) {
  Guitar g(kFs, precisionBass());
  for (int s = 1; s < 4; ++s) g.damp(s, 40);
  strike(g);
  std::vector<float> x(size_t(secs * kFs));
  for (float& y : x) y = g.sample();
  return x;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) { std::fprintf(stderr, "usage: pickcontact OUTDIR\n"); return 1; }
  const std::string dir = argv[1];
  const double x = 0.10, d = 3.7e-3, a = 0.42;
  // A finger pulling the string as far, slowly (a static release: the reference for the level),
  // and as GM 34's player plucks at velocity 100 (rise 2.2 ms, a 0.7 ms slip off the pad).
  write(dir + "/finger_slow.f32", note([&](Guitar& g) { g.pluckPad(0, d, x, a, 0.010, 0.010, 0.0007); }));
  write(dir + "/finger.f32", note([&](Guitar& g) { g.pluckPad(0, d, x, a, 0.010, 0.0022, 0.0007); }));
  // The old pick (a force pulse of 1 ms) and the held pick (pushed over 6 ms, a 1 mm pick).
  write(dir + "/pulse.f32", note([&](Guitar& g) { g.pick(0, d, x, a, 0.001); }));
  write(dir + "/contact.f32", note([&](Guitar& g) { g.pickContact(0, d, x, a, 0.006, 0.00015, 6000); }));
  write(dir + "/contact_slip30.f32", note([&](Guitar& g) { g.pickContact(0, d, x, a, 0.006, 0.0003, 6000); }));
  // Over the E/A coil (149 mm): the pick's comb and the coil's on the same notch (the 6th).
  write(dir + "/contact_149.f32", note([&](Guitar& g) { g.pickContact(0, 2.5e-3, 0.149, 0.25, 0.006, 0.00015, 6000); }));
  // Stability: 1000 picks, each string, fret, place (4-27 cm), depth (0.3-4 mm), stiffness
  // (2-20 kN/m), push, slip and direction at random, over strings left ringing.
  Guitar g(kFs, precisionBass());
  uint32_t r = 2026;
  auto ur = [&]() { r = r * 1664525u + 1013904223u; return (r >> 8) * (1.0 / 16777216.0); };
  double peak[4] = {}, worst = 0;
  long bad = 0;
  for (int n = 0; n < 1000; ++n) {
    const int s = int(ur() * 4) % 4;
    g.fret(s, int(ur() * 21));
    const double place = 0.04 + 0.23 * ur(), depth = (0.3 + 3.7 * ur()) * 1e-3, kp = 2000 + 18000 * ur();
    const double angle = ur() < 0.5 ? 0.25 + 0.1 * (ur() - 0.5) : M_PI + 0.18 + 0.1 * (ur() - 0.5);
    const double push = 0.002 + 0.018 * ur(), slip = 0.0001 + 0.0004 * ur(), bend = ur() < 0.3 ? 0.0015 : 0;
    g.pickContact(s, depth, place, angle, push, slip, kp, bend);
    const int len = int(kFs * (0.01 + 0.2 * ur()));
    for (int i = 0; i < len; ++i) {
      const float y = g.sample();
      if (!std::isfinite(y)) ++bad;
      peak[n / 250] = std::max(peak[n / 250], double(std::fabs(y)));
    }
  }
  for (double p : peak) worst = std::max(worst, p);
  // (Against the hardest single stroke the fuzz can make, on each string: 4 mm, 20 kN/m, a 2 ms push.)
  double one = 0;
  for (int s = 0; s < 4; ++s)
    for (double place : {0.04, 0.27}) {
      Guitar h(kFs, precisionBass());
      for (int o = 0; o < 4; ++o) if (o != s) h.damp(o, 40);
      h.fret(s, 20);
      h.pickContact(s, 4e-3, place, 0.25, 0.002, 0.0001, 20000);
      for (int i = 0; i < int(0.2 * kFs); ++i) one = std::max(one, double(std::fabs(h.sample())));
    }
  std::printf("fuzz nonfinite %ld peaks %.4f %.4f %.4f %.4f worst %.4f one %.4f\n", bad, peak[0], peak[1], peak[2], peak[3], worst, one);
  return 0;
}
