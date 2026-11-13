// Brass on the bore waveguide: the lips buzzing in a cup mouthpiece at the top
// of the air column, the valves switching loops of tubing in.
//
//   - The lips: an outward-striking valve (Elliott & Bowsher 1982; Velut et
//     al. 2017; research/horns.md 1.2), one mass per area:
//       x'' + (wl / Q) x' + wl^2 x = (pm - p) / mu,   opening h = H + x,
//     the mouth pressure pushes them open into the cup, the cup's pressure
//     pushes them shut. The player sets the resonance fl (tension) per note,
//     a little below the partial; the rest opening H = hk / fl. They may
//     overlap a little when shut (no flow); the opening is smoothed near zero
//     (Vergez & Rodet 2001) so the flow's derivative stays continuous.
//   - The flow: Bernoulli through w h (the jet's energy is lost in the cup),
//     plus the lips' swept flow sr dx/dt.
//   - The cup is a volume in the first cell, the throat's mass is in the
//     first interfaces (the exact integral of dx / S over each).
//   - A valve: its loop is a bore of its own, linked to the two cells around
//     the valve's port in the main bore. Pressing it shuts the port and opens
//     the links over the stroke: half-way both paths are part open (a
//     half-valve). The links are leapfrog interfaces too (a mass and, part
//     open, a resistance), solved before the bores step, their flows applied
//     after.
#pragma once

#include <memory>

#include "flute.h"  // Embouchure
#include "tube.h"

namespace pajupilli {

class Horn : public Tube {
 public:
  explicit Horn(const Instrument& inst);
  // e.reed: the lip resonance (Hz); e.lung, e.tongue, e.glottis, e.noise, e.air.
  void setEmbouchure(const Embouchure& e) { target_ = e; }
  const Embouchure& embouchure() const { return target_; }
  // The valves: 'x' pressed, valve 1 first. fingerSpeed as for keys.
  void setHoles(const char* pattern, double fingerSpeed = 1) override;

  double testResponse() const override { return bore_.s.p()[ec_]; }
  void resetAir() override;
  // Introspection for tests.
  double mouthPressure() const { return pm_; }
  double cupPressure() const { return bore_.s.p()[ec_]; }
  double lipOpening() const { return h_; }
  double lipFlow() const { return u_; }
  double valve(int i) const { return valve_[i].s; }

 private:
  void controlStep() override;
  void settle() override;
  float sample() override;

  // A damped mass-spring, trapezoidal: x'' + g x' + w^2 x = F.
  struct Mode {
    double x = 0, v = 0;
    double t00 = 1, t01 = 0, t10 = 0, t11 = 1, b0 = 0, b1 = 0;
    void set(double w, double g, double dt);
    void step(double f) {
      const double xn = t00 * x + t01 * v + b0 * f;
      v = t10 * x + t11 * v + b1 * f;
      x = xn;
    }
  };

  struct Valve {
    std::unique_ptr<Bore> loop;
    int a = 0;          // the port: the interface between main cells a and a + 1
    int n = 0;          // the loop's cells
    double link = 0;    // each link's length (m)
    double area = 0;    // the loop's area (m^2)
    double s = 0;       // 0 up .. 1 down (the loop's open share)
    double t = 0;       // stroke time 0 .. 1
    int target = 0;
    double stroke = 0.012;
    float cw = 0, cp = 0;   // link update: w+ = cw w + cp (p_from - p_to)
    float w1 = 0, w2 = 0;   // into the loop at a, out of it into a + 1
    bool awake = false;     // the loop's air moves: stepped (always while linked)
  };
  // A valve up: its loop, shut off, rings down to still air.
  static bool still(const Bore& b, double area);
  void setValveCoefs(Valve& v);

  const LipParams& lp_;
  Embouchure target_, cur_;
  double tongue_ = 0;
  // The lips: the outward mode (displacement into the cup) and the inward
  // one (towards the mouth); the opening and the force on them.
  Mode out_, in_;
  double h_ = 0, dp_ = 0;
  double H_ = 0, invMu_ = 0;
  // Mouth.
  double pm_ = 0, u_ = 0;
  double glotLp_ = 0, noiseLp_ = 0, noiseHp_ = 0, glotA_, noiseA_, noiseHpA_;
  double kNoise_, kSweep_, kGlotNoise_, kQ_, kEps_, kPress_ = 1, kRelease_, kStroke_, kMu_, kHk_, kW_, kIn2_, kInQ_, kInA_, kQExp_;
  Valve valve_[4];
  int nValves_ = 0;
};

}  // namespace pajupilli
