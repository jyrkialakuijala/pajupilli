// Flue instruments on the bore waveguide: the air jet across a window drives
// the air column.
//   - The Boehm concert flute (B foot) and its family, resized from it.
//     Geometry: Pearl PF-661 measured by Dickens (UNSW), see research/keywork.md.
//   - Designs (src/designs.cc): a duct forms the jet (recorder, ocarina), or
//     the lips blow across the open top of a pipe (shakuhachi, pan flute).
// Jet: Verge / Fabre / de la Cuadra jet-drive model, see research/jet_and_losses.md.
// Player: lungs as a pressure set-point, the mouth one compliance, the tongue a
// valve, the lip slit (or the windway) a Bernoulli orifice with jet inertia
// (research/player_and_controller.md).
#pragma once

#include <cstdint>
#include <string>

#include "tube.h"

namespace pajupilli {

// What the player does with the lips and breath; set any time, smoothed inside.
// A duct instrument keeps its own jet geometry (the lip settings are ignored).
struct Embouchure {
  double lung = 0;          // lung (blowing) pressure set-point (Pa)
  double tongue = 0;        // tongue valve: 0 closed .. 1 open
  double lipH = 0.8e-3;     // lip opening height = jet thickness h (m)
  double lipWidth = 9e-3;   // lip opening width = jet width H (m)
  double jetLen = 8e-3;     // lip to edge W (m)
  double uncovered = 6e-3;  // embouchure hole width left open by the lower lip (m)
  double y0 = 0.1e-3;       // jet offset from the labium edge (m)
  double noise = 0.01;      // turbulence, in units of the jet velocity
  double air = 0.35;        // 0 focused .. 1 airy: unfocused jet, edge hiss
  double glottis = 1.5e-4;  // glottal opening (m^2): 1.5 cm^2 open .. ~3 mm^2 a slit
  double face = 1;          // the face and lip load on the window: extra window
                            // length in units of 2 mm (the instrument's size)
  double reed = 1;          // (reeds) lip force on the reed, x mezzo-forte
  double lipPress = 1;      // (brass) the lips pressed together: their rest opening, x
};

class Flute : public Tube {
 public:
  explicit Flute(const Instrument& inst = concertFlute());

  void setEmbouchure(const Embouchure& e) { target_ = e; }
  const Embouchure& embouchure() const { return target_; }
  void selectPipe(int midi) override;
  bool duct() const { return inst_.kind == Kind::kDuct; }

  double testResponse() const override { return eq_; }
  void resetAir() override;
  // Introspection for tests.
  double mouthPressure() const { return pm_; }
  double jetVelocity() const { return uj_; }
  double windowFlow() const { return eq_; }
  double jetSplit() const { return ef_; }
  double windowPressure() const { return bore_.s.p()[ec_]; }

 private:
  void embouchureGeometry();
  void jetCoefs();
  void controlStep() override;
  void settle() override;
  float sample() override;
  double boreDiameter(double x) const;  // at x (m) from the cork face (the flute family)
  void buildDesign();
  void buildPipe(int pipe);

  // The instrument's dimensions (m).
  double cork_, head_, foot_, embLong_, embRin_, embChimney_, embA_;
  double channel_;  // lip channel or windway: the jet's inertia (m)
  int pipe_ = -1;   // a pan flute's pipe in use
  double noiseA_, noiseHpA_, glotA_;  // noise filters at this rate

  Embouchure target_, cur_;
  // Embouchure window branch (scalar): flow out of the bore Q, radiation flow l.
  double eq_ = 0, el_ = 0, ef_ = 0;
  double eg_ = 0, ers_ = 0, err_ = 0, ehl_ = 0, esw_ = 1e-4, ecnl_ = 0;
  // Mouth and jet.
  double pm_ = 0, uj_ = 0, tongue_ = 0;
  static constexpr int kHist = 2048;
  float vHist_[kHist] = {};
  int hPos_ = 0;
  double noiseLp_ = 0, noiseHp_ = 0, glotLp_ = 0, kGlotNoise_;
  // Knobs.
  int kJetModel_;
  double kRel_, kSpread_, kJetLp_, kEtaMax_, kEmbR_, xi_ = 0, jl1_ = 0, jl2_ = 0;
  double kXiHp_, xiLow_ = 0, kVortex_ = 1;
  int kJetDisp_ = 1;
  double kCpHi_ = 0.45, kDispT_ = 0, dl1_ = 0, dl2_ = 0;
  double kAgc_ = 0, agcA_ = 0, agcEnv_ = 0;
  double kTurbIn_, kEdge_, edgeLow_ = 0, edgeBand_ = 0;
  double kJetSign_, kEtaSign_, kEmbLen_, kAlpha_, kConv_, kNoiseGain_;
  // Per block (jetCoefs): the jet's width at the edge, its growth, the edge
  // force's scale; the jet lowpass's exp(-x) and the edge hiss's tan(a) at the
  // block's start, followed through the block by their series in uj.
  double jB_ = 1e-3, jGain_ = 1, jCg_ = 1, jDd_ = 0;
  double lpX0_ = 0, lpE0_ = 1, hissA0_ = 0, hissT0_ = 0;
};

}  // namespace pajupilli
