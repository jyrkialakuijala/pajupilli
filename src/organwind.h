// The organ's wind (research/church_organ.md §4a): a small lumped network
// from the blower to each division's chest, solved every sample; every
// sounding pipe draws its flow from its chest, and reads the chest's pressure
// back (its jet's speed, its pitch and level then follow the wind by
// themselves: §4a.3).
//
//   - The blower: a centrifugal fan, Q = Qmax sqrt(1 - P / p0) (its pressure
//     falling as the flow rises; shut off at p0 = 1.25 x the working
//     pressure), into a plenum (the main reservoir and the trunks).
//   - A regulator per division: a valve fed from the plenum (its flow ~
//     opening x sqrt(the pressure across it)), its opening following the
//     chest's pressure (a stiff spring: ~1 % droop for a hymn's plenum) with
//     the regulator's own mass and damping (a fast second-order valve, the
//     pressure loop around it crossing over at ~4 Hz: on a big chord the chest
//     dips, recovers, and overshoots a little; modern, well damped).
//   - The chests: a compliance each (the regulator's top, the chest's air).
// Pressures are relative to the working pressure (1); flows in L/s.
#pragma once

#include <algorithm>
#include <cmath>

namespace pajupilli {

class OrganWind {
 public:
  static constexpr int kDivs = 4;
  struct Params {
    double qMax = 570;      // the blower's free-flow capacity (L/s; ~34 m³/min, a size over the
                            // research's 21-28 m³/min class for 37 stops: the full organ, all 53 stops,
                            // ~290 L/s for a 10-note chord held, sags ~8-10 %, as the user chose; a
                            // plenum or 6 notes ~1-2 %)
    double p0 = 1.25;       // its shut-off pressure
    double plenumC = 120;   // the plenum's compliance (L per unit pressure)
    double chestC = 100;    // a chest's (with its regulator's top)
    double valveG = 320;    // a regulator's valve, fully open, across the full headroom (L/s)
    double stiffness = 17;  // the valve's opening per unit of the chest's droop (the loop then crosses
                            // over at ~4 Hz: the dip's recovery, the wind's slight wobble)
    double valveHz = 15;    // the valve's own response (its top's mass on the springs)
    double damping = 0.7;   // its damping ratio
    double spillG = 4000;   // the relief valve's flow per unit of overpressure (L/s)
  };
  void setup(double fs, const Params& pr) {
    fs_ = fs;
    p_ = pr;
    plenum_ = 1.2;
    for (Div& d : div_) d = Div();
  }
  float pressure(int d) const { return float(div_[d].p); }
  double demand(int d) const { return div_[d].last; }
  double plenum() const { return plenum_; }
  double lowest() const { return minP_; }  // the lowest any chest has been
  // The flow drawn from division d this sample (L/s).
  void draw(int d, double q) { div_[d].demand += q; }
  // A tremulant on division d (research §4a.4), a "retained-wind" one: a
  // beater working the regulator's top at rateHz, the chest's volume swung
  // by +-litres (a few millimetres on a ~1.2 m² top): the wind compressed and
  // let expand; the regulator answers as its valve and loop allow, the pipes
  // follow the pressure (pitch, level) by themselves.
  void setTremulant(int d, bool on, double rateHz = 5.5, double litres = 6) {
    div_[d].trem = on;
    div_[d].tremRate = rateHz;
    div_[d].tremDepth = litres;
  }
  void step() {
    const double dt = 1 / fs_;
    const double w = 2 * M_PI * p_.valveHz;
    double into = 0;
    for (Div& d : div_) {
      double squeeze = 0;  // (L/s: the tremulant's beater pressing the top down, then letting it rise)
      if (d.trem) {
        d.tremPhase += d.tremRate * dt;
        d.tremPhase -= std::floor(d.tremPhase);
        squeeze = d.tremDepth * 2 * M_PI * d.tremRate * std::cos(2 * M_PI * d.tremPhase);
      }
      // The regulator: its valve's opening chases the chest's droop.
      const double target = std::clamp(d.x0 + p_.stiffness * (1 - d.p), 0.0, 1.0);
      d.xv += dt * (w * w * (target - d.x) - 2 * p_.damping * w * d.xv);
      d.x = std::clamp(d.x + dt * d.xv, 0.0, 1.0);
      const double head = std::max(0.0, plenum_ - d.p);
      const double q = d.x * p_.valveG * std::sqrt(head / (p_.p0 - 1));
      // The relief valve: above its set pressure (+1 %) the reservoir spills
      // (else what the valve lets in, or a tremulant's beater presses, could
      // only leave through the pipes).
      const double spill = d.p > 1.01 ? p_.spillG * (d.p - 1.01) : 0.0;
      d.p += dt * (q - d.demand + squeeze - spill) / p_.chestC;
      d.p = std::max(d.p, 0.0);
      minP_ = std::min(minP_, d.p);
      into += q;
      d.last = d.demand;
      d.demand = 0;
    }
    const double qb = p_.qMax * std::sqrt(std::max(0.0, 1 - plenum_ / p_.p0));
    plenum_ += dt * (qb - into) / p_.plenumC;
    plenum_ = std::clamp(plenum_, 0.0, p_.p0);
  }

 private:
  struct Div {
    double p = 1, x = 0.02, xv = 0, x0 = 0.02, demand = 0, last = 0;
    bool trem = false;
    double tremRate = 5.5, tremDepth = 6, tremPhase = 0;
  };
  double fs_ = 48000;
  Params p_;
  double plenum_ = 1.2, minP_ = 1;
  Div div_[kDivs];
};

}  // namespace pajupilli
