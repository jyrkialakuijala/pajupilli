// The frets as obstacles in Guitar (GuitarSpec::fretContact; research/slap_bass_1.md 5.2, 5.8;
// slap_bass_2.md 7), on the two-pickup bass's strings: tools/test.py builds and runs this with
// src/guitar.cc and src/bassspec.cc and reads its "ok  " / "FAIL" lines.
//   - stable: 2000 random thumbs, pops and plucks (every string, frets 0-17, soft to hard),
//     every sample finite;
//   - the contacts take energy or keep it, never give it: with the strings' losses and the neck
//     off, the energy never rises at a sample with a contact (once the stroke is over);
//   - when and where: a thumb drives the open E onto the last frets within 1-4 ms; a popped open G
//     (3.6 mm at 22 cm, Issanchou's case) meets the frets within 6 ms of its release, at five
//     frets or more, its dense contacts over within 0.12 s; a gentle pop (1.2 mm) hardly touches;
//   - off (GM 34's default) nothing touches.
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "../src/ebass.h"
#include "../src/guitar.h"

using namespace pajupilli;

namespace {
int fails = 0;
void check(bool ok, const char* name, const char* detail) {
  std::printf("%s  %s: %s\n", ok ? "ok  " : "FAIL", name, detail);
  if (!ok) ++fails;
}
uint32_t rng = 12345;
double ur() {
  rng = rng * 1664525u + 1013904223u;
  return (rng >> 8) * (1.0 / 16777216.0);
}
GuitarSpec slapSpec(bool lossless) {
  GuitarSpec g = twoPickupBass();
  g.fretContact = true;
  g.topHz = 10000;
  g.lossByFrequency = true;
  if (lossless) {
    for (auto& s : g.str) s.loss0 = s.loss1 = 0;
    g.body.clear();
  }
  return g;
}
}  // namespace

int main() {
  const double fs = 48000;
  char d[256];
  {
    Guitar b(fs, slapSpec(false));
    bool finite = true;
    int contacts = 0;
    for (int k = 0; k < 2000 && finite; ++k) {
      const int s = int(ur() * 4) % 4, fret = int(ur() * 18);
      const double v = 0.1 + 0.9 * ur();
      b.fret(s, fret);
      const double kind = ur();
      if (kind < 0.45) b.thump(s, 0.7 + 3.2 * v, 0.24 + 0.05 * ur(), ur() < 0.8 ? 1.3 : -1.3);
      else if (kind < 0.9) b.pop(s, (1.5 + 5 * v) * 1e-3, 0.18 + 0.05 * ur(), -1.45, 0.003 + 0.003 * ur(), 0.01 + 0.015 * ur(), 0.0003);
      else b.pluckPad(s, (0.5 + 4 * v) * 1e-3, 0.1, 0.4, 0.01, 0.003, 0.001);
      const int n = int((0.02 + 0.15 * ur()) * fs);
      for (int i = 0; i < n; ++i) finite = finite && std::isfinite(b.sample());
      for (int t = 0; t < 4; ++t) contacts += b.fretHits(t);
    }
    std::snprintf(d, sizeof d, "2000 random strokes, %d contact samples, every sample finite", contacts);
    check(finite && contacts > 1000, "frets: stable", d);
  }
  {
    // (Energy: a thumb on the open E, a pop on the open G and on the G at the 5th fret.)
    int rises = 0, hits = 0;
    double worst = 0;
    for (int c = 0; c < 3; ++c) {
      Guitar b(fs, slapSpec(true));
      const int s = c == 0 ? 0 : 3;
      b.fret(s, c == 2 ? 5 : 0);
      if (c == 0) b.thump(0, 3.0, 0.265, 1.3);
      else b.pop(s, 4.5e-3, 0.215, -1.45, 0.006, 0.015, 0.0003);
      double prev = 0;
      for (int i = 0; i < int(0.8 * fs); ++i) {
        b.sample();
        const double e = b.energy(s);
        const int h = b.fretHits(s);
        if (i > int(0.03 * fs) && h) {
          ++hits;
          if (e > prev * (1 + 1e-6)) { ++rises; worst = std::max(worst, e / prev - 1); }
        }
        prev = e;
      }
    }
    std::snprintf(d, sizeof d, "%d contact samples after the strokes, the energy rose at %d (most %.2g)", hits, rises, worst);
    check(hits > 50 && rises == 0, "frets: the contacts never give energy", d);
  }
  {
    // When and where.
    Guitar b(fs, slapSpec(false));
    b.thump(0, 2.6, 0.265, 1.3);
    double first = -1;
    uint32_t which = 0;
    for (int i = 0; i < int(0.02 * fs); ++i) {
      b.sample();
      uint32_t w = 0;
      if (b.fretHits(0, &w) && first < 0) first = i / fs;
      which |= w;
    }
    std::snprintf(d, sizeof d, "thumb, open E, 2.6 m/s: first contact %.1f ms, the last frets struck: %s", first * 1e3,
                  (which >> 18) ? "yes" : "no");
    check(first >= 0.001 && first <= 0.004 && (which >> 18), "frets: the thumb's clank", d);
  }
  {
    Guitar b(fs, slapSpec(false));
    const double rise = 0.015, slip = 0.0003;
    b.pop(3, 3.6e-3, 0.22, -1.45, 0.006, rise, slip);
    double first = -1, last = -1;
    uint32_t which = 0;
    int n = 0;
    for (int i = 0; i < int(0.4 * fs); ++i) {
      b.sample();
      uint32_t w = 0;
      const int h = b.fretHits(3, &w);
      const double t = i / fs - rise - slip;
      if (h) {
        if (first < 0) first = t;
        if (last < 0 || t - last < 0.02) last = t;  // (dense: no gap over 20 ms)
        which |= w;
        n += h;
      }
    }
    int frets = 0;
    for (int q = 0; q < 32; ++q) frets += which >> q & 1;
    std::snprintf(d, sizeof d, "open G, 3.6 mm at 22 cm: first contact %.1f ms after the release, %d frets, dense until %.0f ms (Issanchou: 0-90)",
                  first * 1e3, frets, last * 1e3);
    check(first >= 0 && first < 0.006 && frets >= 5 && last < 0.12, "frets: the pop's slap", d);
  }
  {
    Guitar b(fs, slapSpec(false));
    b.pop(3, 1.2e-3, 0.22, -1.45, 0.006, 0.02, 0.0004);
    int n = 0;
    for (int i = 0; i < int(0.4 * fs); ++i) { b.sample(); n += b.fretHits(3); }
    std::snprintf(d, sizeof d, "open G, 1.2 mm: %d contact samples", n);
    check(n < 30, "frets: a gentle pop hardly touches", d);
  }
  {
    GuitarSpec g = twoPickupBass();
    Guitar b(fs, g);
    b.thump(0, 4.0, 0.265, 1.3);
    int n = 0;
    for (int i = 0; i < int(0.3 * fs); ++i) { b.sample(); n += b.fretHits(0); }
    std::snprintf(d, sizeof d, "a hard thumb with fretContact off: %d contact samples", n);
    check(n == 0, "frets: off by default", d);
  }
  return fails ? 1 : 0;
}
