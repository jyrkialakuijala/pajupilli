// A body's upper resonances, as a bank (Viola.md, findings 5 and 6).
//
// The plates' grid (bassbody.h) carries a body's first dozen resonances rightly: the air, the
// main wood resonances, what lies between the open strings. Its masses are 26 to 30 mm apart on
// a viola, too few for the bending waves above about 1 kHz, and its wood loses more and more
// with frequency: above 2 kHz it answers almost evenly, where a real body has a resonance every
// 50 to 100 Hz with a Q that stays near 30 to 40. Each overtone of a real note meets its own
// peak or dip there and its own direction out of the body; with vibrato it rides over them: the
// instrument's shimmer.
//
// So above the grid's reach the body is not computed but drawn: resonances at a body's density
// (one per `spacing` Hz, each a little off its place), all of one Q, coming in between 800 Hz
// and 1.5 kHz, falling 6 dB an octave from there and faster above 4.5 kHz (the bridge's hill is
// in the feet's forces already). Each is driven by the bridge's feet, by their sum and their difference (the bridge
// rocking) in its own shares, and sends its sound out in its own direction: its own share of
// the sound heard alike all round and of the three directional parts (bassbody.h: dipole).
//
// A stand-in, not physics: the same resonances for every instrument of a kind (a fixed seed),
// and the bridge does not feel them (they take nothing from the strings).
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace pajupilli {

class BodyBank {
 public:
  void init(double fs, double lo = 900, double hi = 9000, double spacing = 80, double q = 35, uint32_t seed = 7) {
    const int n = std::max(0, int((hi - lo) / spacing));
    n_ = n;
    for (auto* v : {&b0_, &a1_, &a2_, &x1_, &x2_, &y1_, &y2_, &ps_, &pd_, &m_, &d0_, &d1_, &d2_}) v->assign(n, 0.0f);
    uint32_t r = seed * 2654435761u + 12345u;
    auto rnd = [&]() {  // -1 .. 1
      r = r * 1664525u + 1013904223u;
      return (r >> 8) * (2.0 / 16777216.0) - 1;
    };
    for (int k = 0; k < n; ++k) {
      const double f = lo + (k + 0.5 + 0.4 * rnd()) * spacing;
      const double w = 2 * M_PI * f / fs, alpha = std::sin(w) / (2 * q);
      // A band-pass of unit gain at its peak.
      b0_[k] = float(alpha / (1 + alpha));
      a1_[k] = float(-2 * std::cos(w) / (1 + alpha));
      a2_[k] = float((1 - alpha) / (1 + alpha));
      // In: between 800 Hz and 1.5 kHz; out: above 4.5 kHz.
      const double u = std::clamp((f - 800) / 700, 0.0, 1.0);
      // (and a body's answer to a force falls as the frequency rises: 6 dB an octave from 1.5 kHz)
      const double level = u * u * (3 - 2 * u) / (1 + std::pow(f / 4500, 2)) * std::min(1.0, 1500 / f);
      // The feet's sum and their difference, in its own shares (the difference the more: the
      // bridge rocks).
      ps_[k] = float(0.5 * rnd() * level);
      pd_[k] = float((rnd() > 0 ? 1 : -1) * (0.5 + 0.5 * std::fabs(rnd())) * level);
      // Its own direction: a share heard all round and three directional ones.
      double g[4], nn = 0;
      for (double& x : g) { x = rnd() + rnd() + rnd(); nn += x * x; }
      nn = std::sqrt(std::max(nn, 1e-9));
      m_[k] = float(g[0] / nn);
      d0_[k] = float(g[1] / nn);
      d1_[k] = float(g[2] / nn);
      d2_[k] = float(g[3] / nn);
    }
  }
  void reset() {
    for (auto* v : {&x1_, &x2_, &y1_, &y2_}) std::fill(v->begin(), v->end(), 0.0f);
    out_ = dip_[0] = dip_[1] = dip_[2] = 0;
  }
  bool on() const { return n_ > 0; }
  // One sample: the two feet's forces on the top (N).
  void step(float bassFoot, float trebleFoot) {
    const float sum = bassFoot + trebleFoot, diff = bassFoot - trebleFoot;
    float o = 0, e0 = 0, e1 = 0, e2 = 0;
    for (int k = 0; k < n_; ++k) {
      const float x = ps_[k] * sum + pd_[k] * diff;
      const float y = b0_[k] * (x - x2_[k]) - a1_[k] * y1_[k] - a2_[k] * y2_[k];
      x2_[k] = x1_[k];
      x1_[k] = x;
      y2_[k] = y1_[k];
      y1_[k] = y;
      o += m_[k] * y;
      e0 += d0_[k] * y;
      e1 += d1_[k] * y;
      e2 += d2_[k] * y;
    }
    out_ = o;
    dip_[0] = e0;
    dip_[1] = e1;
    dip_[2] = e2;
  }
  float out() const { return out_; }          // heard alike all round
  float dipole(int k) const { return dip_[k]; }  // ... and along the body's three axes

 private:
  int n_ = 0;
  std::vector<float> b0_, a1_, a2_, x1_, x2_, y1_, y2_, ps_, pd_, m_, d0_, d1_, d2_;
  float out_ = 0, dip_[3] = {0, 0, 0};
};

}  // namespace pajupilli
