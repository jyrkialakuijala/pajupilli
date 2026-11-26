// A free reed (research/free_reeds.md §2): a steel tongue riveted over a slot
// in a plate, swinging through the slot with a ~50 um clearance. The minimal
// model of Millot & Baumann (2007, "MB07" in the research):
//
//   - the reed: one mode in the tip's displacement z (into the slot: +),
//       z'' + (w / Q) z' + w^2 z = mu p2,   mu = 0.39 W L w^2 / K
//     (0.39: the clamped-free beam's first mode, its force and swept area);
//   - the flow out: the reed's own sweeping (Sr z') and the jet through the
//     gap (alpha S(z) vj), with p2 = rho vj^2 / 2 (Bernoulli);
//   - upstream of the reed: the air in the duct that feeds it (its inertance
//     M: p1 - p2 = M du/dt) and, behind that, the reed's cell (a volume V1,
//     p1, filled from the bellows through the pallet's opening).
//   The gap: the tip stands `set` out of the slot on the upstream side at
//   rest; while it is in the slot (a plate `es` thick) only the clearance
//   passes air, out of it on either side the gap opens along its front and
//   its sides: two puffs a cycle, a nearly square pressure (rich, to ~10 kHz).
//   The duct's inertance is what makes it speak (the flow lags the opening);
//   the pitch is the reed's own, a little lower the harder it is blown.
//
// The flow is solved implicitly each sample (backward Euler on u: a quadratic
// in closed form), so the clearance's stiff flow stays stable at any rate;
// the reed's mode is exact (a damped rotation with the force held over the
// sample). The source radiates as its flow's rate of change.
//
// Blown-closed: it speaks for one direction of the air only; an accordion
// pairs two on each plate (push and pull), each with its valve.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "vmath.h"

namespace pajupilli {

// One reed's build: sizes in m, its tuning in Hz.
struct FreeReedBuild {
  double hz = 440;
  double len = 13e-3, width = 2.1e-3;  // the tongue
  double set = 0.5e-3;                 // the tip's rest height out of the slot
  double plate = 1.0e-3;               // the plate's thickness (the slot's depth)
  double clear = 45e-6;                // the clearance round the tongue
  double closeP = 2000;                // the pressure that would hold the tip at the plate (Pa): its stiffness
  double q = 100;                      // its resonance's Q in air
  double cell = 2e-6;                  // the cell's volume (m^3)
  double duct = 960;                   // the feeding duct's inertance (rho l / S, kg/m^4)
  double alpha = 0.6;                  // the jet's vena contracta
};

// An accordion reed of a given pitch: the tongue longer and wider the lower
// it is (bass reeds also carry a tip weight: the length grows slower than a
// plain tongue's would), its set and the plate scaled with it [design;
// MB07's harmonica reed at 444 Hz: 12.95 x 2.1 mm, set 0.53 mm].
inline FreeReedBuild accordionReed(double hz) {
  FreeReedBuild b;
  b.hz = hz;
  b.len = std::clamp(13e-3 * fastPow(444 / hz, 0.42), 6e-3, 70e-3);
  b.width = std::clamp(2.1e-3 * fastPow(444 / hz, 0.2), 1.4e-3, 4.5e-3);
  b.set = 0.04 * b.len;
  b.plate = std::clamp(0.07 * b.len, 0.8e-3, 2.5e-3);
  b.cell = 2e-6 * std::pow(b.len / 13e-3, 2.0);
  // The high reeds ring longer (short, stiff steel): they still start at a
  // few hundred Pa.
  b.q = std::clamp(100 * fastPow(hz / 440, 0.35), 80.0, 250.0);
  b.closeP = 1500;
  // The air that feeds it (the pallet's hole and the chamber's throat): its
  // mass lowers the pitch alike over the range (~25 c) and lets it speak.
  b.duct = 960 * 444 / hz;
  return b;
}

class FreeReed {
 public:
  void setup(double fs, const FreeReedBuild& b) {
    b_ = b;
    fs_ = fs;
    dt_ = 1 / fs;
    sr_ = 0.39 * b.width * b.len;
    perim_ = b.alpha * (b.width + 0.78 * b.len);
    k_ = kRho * kC * kC / b.cell;
    ductNow_ = b.duct;
    tune(1);
    reset();
  }
  // The tongue scraped: its own frequency x r (the calibration).
  void tune(double r) {
    ratio_ = r;
    const double w = 2 * M_PI * b_.hz * r;
    w_ = w;
    const double stiff = b_.closeP * 0.39 * b_.width * b_.len / b_.set;  // K (N/m)
    mu_ = 0.39 * b_.width * b_.len * w * w / stiff;
    // The exact step of z'' + 2 g z' + w^2 z = f (f held over the sample).
    const double g = w / (2 * b_.q), wd = std::sqrt(w * w - g * g), e = std::exp(-g * dt_);
    const double c = std::cos(wd * dt_), s = std::sin(wd * dt_);
    a11_ = e * (c + g / wd * s);
    a12_ = e * s / wd;
    a21_ = -e * w * w / wd * s;
    a22_ = e * (c - g / wd * s);
    b1_ = (1 - a11_) / (w * w);
    b2_ = a12_;  // the velocity's response to a held force
  }
  // The duct's air x s (a pallet half shut: the player's bend).
  void setDuct(double s) { ductNow_ = b_.duct * s; }
  double ratio() const { return ratio_; }
  double hz() const { return b_.hz * ratio_; }
  void reset() { z_ = v_ = p1_ = u_ = uPrev_ = up_ = upPrev_ = 0; }
  // The harmonium's arrangement (research/reed_organ.md §4.4, §8.2): the wind chest's pressure
  // on the tongue, the reed's cell *downstream* of it, the cell emptying into the room through the
  // key's pallet (its open area, m^2; 0: shut) with that opening's air (ductLen, m) and its
  // Bernoulli loss. Uses the reed's own `cell` volume. Returns d(pallet flow)/dt (m^3/s^2).
  // palletQ: the cell's resonance damped by the pallet's felt and leather (its Q; 0: undamped).
  float sampleDown(double chest, double palletArea, double ductLen = 5e-3, double palletQ = 2) {
    const double y = b_.set - z_;
    const double e = b_.clear;
    const double d = 0.5 * (y + std::sqrt(y * y + e * e)) + 0.5 * (-b_.plate - y + std::sqrt((b_.plate + y) * (b_.plate + y) + e * e));
    const double area = perim_ * std::sqrt(d * d + e * e);
    // The jet through the gap, against the cell's pressure (implicit, as sample()).
    const double a = sr_ * v_, cc = kRho / (2 * area * area);
    const double drive = chest - p1_;
    const double bb = u_ - a + dt_ * drive / ductNow_, dd = dt_ * cc / ductNow_;
    const double x = std::copysign((std::sqrt(1 + 4 * dd * std::fabs(bb)) - 1) / (2 * dd), bb);
    u_ = x + a;
    const double p2 = cc * x * std::fabs(x);
    const double f = mu_ * p2;
    const double z = a11_ * z_ + a12_ * v_ + b1_ * f;
    v_ = a21_ * z_ + a22_ * v_ + b2_ * f;
    z_ = z;
    // The pallet: the cell's air out through the key's opening (implicit in its own loss).
    if (palletArea > 1e-9) {
      const double mp = kRho * ductLen / palletArea, cp = kRho / (2 * palletArea * palletArea);
      const double rl = palletQ > 0 ? std::sqrt(mp * k_) / palletQ : 0;  // the felt's loss (Z0 / Q)
      const double den = 1 + dt_ * rl / mp;
      const double bp = (up_ + dt_ * p1_ / mp) / den, dp = dt_ * cp / mp / den;
      up_ = std::copysign((std::sqrt(1 + 4 * dp * std::fabs(bp)) - 1) / (2 * dp), bp);
    } else {
      up_ = 0;
    }
    // The cell between them.
    p1_ += k_ * dt_ * (u_ - up_);
    const double out = (up_ - upPrev_) * fs_;
    upPrev_ = up_;
    return float(out);
  }
  // A hammer on the tongue (the harmonium's Percussion): its tip set moving (m/s, into the slot).
  void kick(double v) { v_ += v; }
  double palletFlow() const { return up_; }
  // How far the tongue swings (m), roughly: for tests.
  double tip() const { return z_; }
  double cellPressure() const { return p1_; }
  double flow() const { return u_; }
  // Still: no pressure in the cell, the tongue swinging less than ~2 um.
  bool quiet() const { return std::fabs(p1_) < 0.05 && std::fabs(z_) < 2e-6 && std::fabs(v_) < 2e-6 * w_; }

  // One sample: the supply's pressure (Pa, the bellows: positive is this
  // reed's speaking direction) behind the pallet, the pallet's resistance
  // (Pa s/m^3; large: shut). Returns the radiated source, d(flow)/dt (m^3/s^2).
  float sample(double supply, double pallet) {
    // The gap.
    const double y = b_.set - z_;  // the tip's height above the plate (upstream side)
    const double e = b_.clear;
    const double d = 0.5 * (y + std::sqrt(y * y + e * e)) + 0.5 * (-b_.plate - y + std::sqrt((b_.plate + y) * (b_.plate + y) + e * e));
    const double area = perim_ * std::sqrt(d * d + e * e);
    // The duct's flow, implicit: M (u' - u) / dt = p1 - c (u' - a) |u' - a|.
    const double a = sr_ * v_, cc = kRho / (2 * area * area);
    const double bb = u_ - a + dt_ * p1_ / ductNow_, dd = dt_ * cc / ductNow_;
    const double x = std::copysign((std::sqrt(1 + 4 * dd * std::fabs(bb)) - 1) / (2 * dd), bb);
    u_ = x + a;
    const double p2 = cc * x * std::fabs(x);
    // The tongue.
    const double f = mu_ * p2;
    const double z = a11_ * z_ + a12_ * v_ + b1_ * f;
    v_ = a21_ * z_ + a22_ * v_ + b2_ * f;
    z_ = z;
    // The cell: filled through the pallet, emptied by the reed (implicit).
    const double kd = k_ * dt_;
    p1_ = (p1_ + kd * (supply / pallet - u_)) / (1 + kd / pallet);
    const double out = (u_ - uPrev_) * fs_;
    uPrev_ = u_;
    return float(out);
  }

 private:
  static constexpr double kRho = 1.2, kC = 343;
  FreeReedBuild b_;
  double fs_ = 48000, dt_ = 1 / 48000.0, ratio_ = 1;
  double sr_ = 0, perim_ = 0, k_ = 0, mu_ = 0, ductNow_ = 960, w_ = 1;
  double a11_ = 1, a12_ = 0, a21_ = 0, a22_ = 1, b1_ = 0, b2_ = 0;
  double z_ = 0, v_ = 0, p1_ = 0, u_ = 0, uPrev_ = 0, up_ = 0, upPrev_ = 0;
};

// Two samples in, one out: a Kaiser-windowed sinc cut at 21 kHz (the reeds' 96 kHz to 48 kHz).
class HalfBand {
 public:
  HalfBand() {
    const double fc = 21000 / 96000.0, beta = 8, c = 0.5 * (kN - 1);
    auto i0 = [](double x) {
      double s = 1, t = 1;
      for (int k = 1; k < 30; ++k) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; }
      return s;
    };
    double sum = 0;
    for (int i = 0; i < kN; ++i) {
      const double t = i - c, r = t / c;
      const double sinc = t == 0 ? 2 * fc : std::sin(2 * M_PI * fc * t) / (M_PI * t);
      h_[i] = float(sinc * i0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / i0(beta));
      sum += h_[i];
    }
    for (float& h : h_) h = float(h / sum);
  }
  float process(float x0, float x1) {
    for (float x : {x0, x1}) {
      pos_ = (pos_ + 1) % kN;
      hist_[pos_] = hist_[pos_ + kN] = x;
    }
    const float* h = hist_ + pos_ + 1;  // the last kN inputs (the filter is symmetric)
    float s = 0;
    for (int i = 0; i < kN; ++i) s += h_[i] * h[i];
    return s;
  }
  void reset() { std::fill(hist_, hist_ + 2 * kN, 0.0f); }

 private:
  static constexpr int kN = 48;
  float h_[kN], hist_[2 * kN] = {};
  int pos_ = 0;
};

}  // namespace pajupilli
