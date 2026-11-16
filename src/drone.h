// A bagpipe's drone: a cylindrical pipe (cane or blackwood, a tuning slide)
// sounded by a single beating reed (an idioglot cane reed: a tongue cut in
// the tube's wall), fed by the bag's steady pressure, one fixed pitch.
//
// Closed at the reed, open at the top: a quarter-wave pipe, its odd
// harmonics strong (the drone's buzzing, clarinet-like tone). As a digital
// waveguide (the reed as a pressure-controlled reflection, the classic
// McIntyre-Schumacher-Woodhouse clarinet form): one delay line for the
// round trip, the open end's reflection (inverting, losing the highs), the
// reed's reflection from the pressure across it, clipped as it closes. The
// tuning slide sets the length (a fractional delay), so a drone can be
// tuned to any pitch below the chanter's.
#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace pajupilli {

class DronePipe {
 public:
  void start(double fs, double hz, unsigned seed) {
    fs_ = fs;
    // The top's losses: brighter for a short (tenor) pipe than a long (bass).
    lpA_ = float(std::clamp(0.35 + 0.0004 * hz, 0.35, 0.75));
    // The round trip of a quarter-wave pipe is half the period, less the
    // low-pass's own delay at the pitch (the slide set so it sounds in tune).
    const double w = 2 * M_PI * hz / fs, b = 1 - lpA_;
    const double lpDelay = std::atan2(b * std::sin(w), 1 - b * std::cos(w)) / w;
    const double d = fs / hz / 2 - lpDelay;
    n_ = std::max(4, int(std::ceil(d)) + 2);
    line_.assign(n_, 0.0f);
    frac_ = float(d);
    w_ = 0;
    lp_ = 0;
    on_ = true;
    level_ = 0;
    rng_ = seed * 2654435761u + 7u;
  }
  void stop() { on_ = false; }
  bool sounding() const { return on_ || level_ > 1e-4f; }
  // One sample: the bag's pressure (0 .. 1, the reed's own scale).
  float sample(float bag) {
    if (n_ == 0) return 0;
    // The bag's air comes on (the reed starts, ~40 ms) and goes (~25 ms).
    level_ += (on_ ? 1.0f / (0.04f * float(fs_)) : -1.0f / (0.025f * float(fs_)));
    level_ = std::clamp(level_, 0.0f, 1.0f);
    // Read the round trip (linear interpolation of the fractional delay).
    float rp = float(w_) - frac_;
    while (rp < 0) rp += float(n_);
    const int i0 = int(rp), i1 = (i0 + 1) % n_;
    const float a = rp - float(i0);
    const float back = line_[i0] * (1 - a) + line_[i1] * a;
    // The open top: inverting, the highs lost (a one-pole low-pass).
    lp_ += lpA_ * (back - lp_);
    const float refl = -0.96f * lp_;
    // The reed: its reflection from the pressure across it (the bag minus
    // the bore's), clipped as the tongue closes onto the tube.
    rng_ = rng_ * 1664525u + 1013904223u;
    const float noise = 0.004f * ((rng_ >> 8) * (2.0f / 16777216.0f) - 1);
    const float pm = bag * level_ * (1 + noise);
    const float dp = refl - pm;
    const float r = std::clamp(0.7f - 0.3f * dp, -1.0f, 1.0f);
    const float in = pm + dp * r;
    line_[w_] = in;
    w_ = (w_ + 1) % n_;
    return back;
  }

 private:
  double fs_ = 48000;
  std::vector<float> line_;
  int n_ = 0, w_ = 0;
  float frac_ = 0, lp_ = 0, lpA_ = 0.5f, level_ = 0;
  bool on_ = false;
  unsigned rng_ = 1;
};

}  // namespace pajupilli
