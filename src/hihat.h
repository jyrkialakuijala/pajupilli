// A hi-hat (research/jazz_drums.md 4): two cymbals face to face on a stand,
// the lower on its seat, the upper on a rod the foot pulls down.
//
//   - Two plates (plate.h), each also a body on felt: it rides up and down
//     and tilts. The lower sits on the seat a little tilted (the tilter
//     screw); the upper hangs in the clutch and wobbles there, slowly.
//   - The rims: touching points all round. Where the upper rim comes down on
//     the lower they press on each other (and lose much: metal rubbing on
//     metal), each touch a knock into both plates' modes and into the two
//     bodies. Closed, they are held pressed by the foot.
//   - How it closes is how the two happen to lie: when the upper plate comes
//     down level with the lower's tilt the rims meet all round at once (flat:
//     a clap, on a cushion of air); when not, one side meets first and the
//     contact rolls round the rim as the plate rocks down onto it (edge
//     first: a clank and a run of knocks). The wobble left by the last
//     stroke and the foot's own lean make every closing its own.
//   - The air between: closing squeezes it out through the gap at the rims.
//     Its pressure cushions the landing; the rush through the last
//     millimetre hisses (the "ch" of the chick). A tilted pair lets it out.
//   - A stick (a mass on a hard contact) or a brush (brush.h) on the upper
//     plate.
//   - The sound: two microphones over the pair hear every mode by its shape.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "brush.h"
#include "plate.h"

namespace pajupilli {

struct HiHatSpec {
  PlateSpec top, bottom;
  BrushSpec brush;
  int patches = 16;            // touching points round the rims
  double rimAt = 1.0;          // ... at this share of the radius (the very edge: every mode moves there)
  double rimK = 1.5e5;         // N/m a point
  double rimLoss = 20;         // its loss: the force x (1 + this x the closing speed), s/m
  // Pressed together, the rims rub all round (between the points too):
  // each mode loses by how much it moves at the rim, rimRub (1/s, for a mode
  // of middling rim motion) under a press of 30 N, in proportion to the press.
  double rimRub = 60;
  double rimTop = 10000;       // the rims feel the plates' modes up to here (Hz)
  double clutchK = 2e4, clutchLoss = 0.5;   // the upper plate on the rod: N/m, share of critical
  double wobbleK = 5, wobbleLoss = 0.06;    // its tilt in the clutch: N m/rad
  double seatK = 5e4, seatLoss = 0.5;       // the lower plate on its seat
  double seatTiltK = 60, seatTiltLoss = 0.3;
  double tilt = 0.010;         // the lower plate's set tilt (rad: 1.8 mm at the rim)
  double openGap = 6e-3;       // the foot up (m)
  double cavity = 2.5e-3;      // the air between the plates (m^3)
  double leak = 0.3e-3;        // the rims never seal: this gap is left (m)
  double jet = 1e-3;           // the rush's share turned to sound
  double micX = 0.20, micZ = 0.40;
  double under = 0.5;          // the lower plate's share at the microphones
  double gain = 1;
};
HiHatSpec hiHatSpec();  // 14" jazz hats, with the PJ_HAT_* knobs

class HiHat {
 public:
  explicit HiHat(double fs, const HiHatSpec& spec = hiHatSpec());
  // The foot: open 0 (down: the plates pressed together with `force` N) .. 1
  // (up: the open gap); the rod moves there at up to `speed` m/s. lean: where
  // the rod's wobble leaves the upper plate tilted as it comes (rad, two axes).
  void pedal(double open, double speed, double force, double leanX = 0, double leanY = 0);
  double gap() const;          // the rims' narrowest gap now (m; <= 0: touching)
  bool closed() const { return target_ <= 0; }
  // A stick on the upper plate: its tip (or, shoulder: the shaft on the
  // edge) at `speed` m/s, r from the centre (m), theta.
  void stick(double speed, double r, double theta, bool shoulder);
  Brush& brush() { return *brush_; }
  Plate& top() { return top_; }
  Plate& bottom() { return bot_; }
  float sample();
  float side() const { return side_; }
  void reset();
  bool asleep() const { return asleep_; }
  void wake() { asleep_ = false; quiet_ = 0; }
  // For tests and tuning: how many rim points touch now, the air's pressure
  // (Pa) and its speed through the gap (m/s).
  int touching() const { return touching_; }
  double pressure() const { return p_; }
  double rush() const { return u_; }
  double tiltOfTop() const;    // rad, against the lower plate's

 private:
  struct Body { double z = 0, vz = 0, ax = 0, ay = 0, wx = 0, wy = 0; };
  HiHatSpec s_;
  double fs_, dt_;
  Plate top_, bot_;
  std::unique_ptr<Brush> brush_;
  Body t_, b_;
  double rod_ = 0, rodV_ = 0, target_ = 0, speed_ = 0.5, force_ = 25, leanX_ = 0, leanY_ = 0;
  int np_, nt_, nb_;
  std::vector<float> phT_, phB_, px_, py_;   // [patch][mode]; the patches' places
  std::vector<float> fRim_, rimG_, rubT_, rubB_;
  float press_ = 0;
  int touching_ = 0;
  double p_ = 0, u_ = 0;
  // The stick.
  bool stickOn_ = false;
  double sz_ = 0, sv_ = 0, sm_ = 0.012, sK_ = 1e9, sG_ = 0, sx_ = 0, sy_ = 0;
  int sAge_ = 0;
  std::vector<float> sPhi_;
  // The microphones.
  std::vector<float> micT_[2], micB_[2];
  float prev_[2] = {}, hp_[2] = {}, hpIn_[2] = {};
  uint32_t rng_ = 12345;
  float side_ = 0;
  bool asleep_ = true;
  int quiet_ = 0;
};

}  // namespace pajupilli
