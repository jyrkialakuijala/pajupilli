// Reed instruments on the bore waveguide: a reed valve at the top of the air
// column (the mouthpiece or the staple), driven by the player's mouth pressure.
//
//   - The reed: a damped mass-spring per unit area (Wilson & Beavers 1974;
//     Chaigne & Kergomard 2016, ch. 9): x'' + qr wr x' + wr^2 x = (pm - p) / mu,
//     x the displacement towards the lay (or the other blade), the opening
//     h = H - x; it beats (closes) when x reaches H. The lips set H, add
//     stiffness and damping (the player's "lip force"); the tongue on the
//     reed closes and damps it.
//   - The flow through the slit: Bernoulli, U = w h sqrt(2 |pm - p| / rho)
//     (the jet's kinetic energy is lost in the mouthpiece). A double reed's
//     narrow duct adds a head loss: (pm - p) = rho U^2 / 2 (1 / (w h)^2 + psi / sd^2)
//     (Vergez et al. 2003, Almeida et al. 2007): the flow saturates sooner.
//   - The reed sweeps air as it moves: sr dx/dt into the mouthpiece.
// The mouthpiece cell's pressure and the reed flow are solved together each
// sample (trapezoidal, the flow's square root in closed form).
#pragma once

#include "flute.h"  // Embouchure
#include "tube.h"

namespace pajupilli {

class Reed : public Tube {
 public:
  explicit Reed(const Instrument& inst);
  void setEmbouchure(const Embouchure& e) { target_ = e; }
  const Embouchure& embouchure() const { return target_; }

  double testResponse() const override { return bore_.s.p()[ec_]; }
  void resetAir() override;
  // Introspection for tests.
  double mouthPressure() const { return pm_; }
  double mouthpiecePressure() const { return bore_.s.p()[ec_]; }
  double reedOpening() const { return h_; }
  double reedFlow() const { return u_; }
  // Closing pressure at mezzo-forte lips: the reed's own scale of pressure (Pa).
  double closingPressure() const { return inst_.design->reed.k * inst_.design->reed.h0; }

 private:
  void controlStep() override;
  void settle() override;
  float sample() override;

  const ReedParams& rp_;
  Embouchure target_, cur_;
  double tongue_ = 0;
  bool played_ = false;  // blown at least once (the tongue may rest on the reed)
  // The reed: displacement towards closing and its velocity; the opening.
  double x_ = 0, v_ = 0, h_ = 0, dp_ = 0;
  // Per control block: rest opening, wr^2, damping, 1 / mu, trapezoid matrix.
  double H_ = 0, w2_ = 0, g_ = 0, invMu_ = 0;
  double t00_ = 1, t01_ = 0, t10_ = 0, t11_ = 1, b0_ = 0, b1_ = 0;
  double aFlow_ = 0;  // sqrt(2 / rho) / sqrt(1 / (w h)^2 + psi / sd^2), without h's part
  // Mouth.
  double pm_ = 0, u_ = 0;
  double glotLp_ = 0, noiseLp_ = 0, noiseHp_ = 0, glotA_, noiseA_, noiseHpA_;
  double kNoise_, kSweep_, kLipH_, kGlotNoise_, kFr_ = 1, kQr_ = 1;
  double psi_ = 0;  // the double reed's duct loss (the design's, or PJ_REEDPSI)
  // (Experiments, off by default.) The blades' progressive closing: a contact
  // spring past contactAt (PJ_REEDCONTACT*), and its trapezoid matrix; the
  // slit narrowing from the sides past meetAt (PJ_REEDMEET*).
  double contact_ = 0, contactAt_ = 0.5, contactD_ = 0.5;
  double c00_ = 1, c01_ = 0, c10_ = 0, c11_ = 1, cb0_ = 0, cb1_ = 0;
  double meetAt_ = 1, meet_ = 1;
};

}  // namespace pajupilli
