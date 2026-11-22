// A snare drum under brushes (research/jazz_drums.md 5, 8.4): two heads on a
// shell of air, the wires on the lower one, and a brush in each hand.
//
//   - The batter head and the snare head (membrane.h), each with its own
//     tension; the air between them a spring on the volume (the two (0,1)s
//     become a pair, one below and one well above).
//   - The wires: strings lying across the snare head, pressed lightly into
//     it. They ride the head while it moves gently; when it drops away
//     faster than they can follow they come off and slap back: the rattle,
//     coherent at first, then every wire for itself.
//   - Two brushes (brush.h) on the batter head: hands 0 (left) and 1 (right).
//   - The sound: two microphones over the batter head hear every mode of it
//     by its shape (and the snare head round the shell, weaker); the wires'
//     and the brushes' own contact noise, above what the modes carry.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "brush.h"
#include "membrane.h"

namespace pajupilli {

struct SnareSpec {
  HeadSpec batter, snareHead;
  DrumAir air;
  BrushSpec brush;
  int wires = 10, wireModes = 8, wirePoints = 8;
  double wireSpacing = 0.011;  // m between them
  double wireMu = 1.6e-3;      // kg/m
  double wireTension = 8;      // N
  double wireLoss = 0.05;      // share of critical
  double wirePress = 0.25e-3;  // how far the head bulges into the wires' line at its middle (m)
  double wireContact = 6e4;    // N/m
  double wireContactLoss = 3;  // the contact's loss (s/m: its force x (1 + this x the closing speed))
  double wireTop = 2500;       // the wires feel the snare head's modes up to here (Hz)
  double micX = 0.22, micZ = 0.45;  // the two microphones: +- x from the centre, over the batter (m)
  double under = 0.45;         // the snare head's share at them
  double wireDirect = 1, tipDirect = 1;  // the wires' and the wire ends' own sound (x)
  double aim = 0.65;           // the sound's directional parts (dipole(): x)
  double tipAim = 0.6;         // the wire ends' own sound: its share that goes out to the brush's side, at the rim
  double gain = 1;
};
SnareSpec snareSpec();  // the jazz snare, with the PJ_SNARE_* knobs

class Snare {
 public:
  explicit Snare(double fs, const SnareSpec& spec = snareSpec());
  Brush& hand(int i) { return *brush_[i]; }
  Membrane& batter() { return bat_; }
  Membrane& snareHead() { return sn_; }
  // One sample: the two microphones' mean; side(): half their difference.
  // A cross-stick (the side stick): the stick lies across the head, its tip
  // end held on the head by the palm, and its butt comes down on the hoop at
  // `speed` m/s: the knock of the stick's own bending and the hoop's ring,
  // the head only nudged, and damped by the stick lying on it.
  void crossStick(double speed);
  float sample();
  float side() const { return side_; }
  // What goes out differently in different directions, level with the head (k = 0: across, the
  // drummer's left to right; 1: from the drummer away): heard that way with this sign, the other
  // way with the opposite, on top of sample(). The modes with an odd number of nodal diameters
  // send their halves' difference sideways, each pair along its own two lines; which of a pair
  // sounds is where the head is touched, so a brush going round turns the drum's sound with it.
  // The wire ends' hiss goes out more to the side of the head the brush is on.
  float dipole(int k) const { return dip_[k]; }
  void reset();
  bool asleep() const { return asleep_; }
  void wake() { asleep_ = false; quiet_ = 0; }
  void setWires(bool on) { wiresOn_ = on; }  // the strainer: off lets them hang clear
  double wireForce() const { return wireF_; }
  int wiresOff() const { int n = 0; for (float f : cf_) n += f == 0; return n; }  // contact points clear of the head now
  int wirePoints() const { return nc_; }

 private:
  void settle();
  SnareSpec s_;
  double fs_, dt_;
  Membrane bat_, sn_;
  DrumAir air_;
  std::unique_ptr<Brush> brush_[2];
  // The wires: [wire][mode] coordinates; [contact][snare-head mode] shapes.
  int nw_, nm_, np_, nc_, nh_;
  std::vector<float> wq_, wv_, ww2_, wa_, wShape_, cPhi_, cz_, cv_, cf_;
  float wMass_ = 1;
  std::vector<float> press_;
  bool wiresOn_ = true;
  // The microphones.
  std::vector<float> micB_[2], micS_[2];
  float prev_[2] = {}, hpW_ = 0, hpT_ = 0, lastW_ = 0, lastT_ = 0;
  std::vector<float> dipB_[2], dipS_[2];
  float dprev_[2] = {}, dip_[2] = {}, hpH_[2] = {}, lastH_[2] = {};
  // The cross-stick's knock: the stick's and the hoop's modes (each a swing:
  // x, v, and what is fixed), and its push on the head.
  struct Knock { float x = 0, v = 0, a11, a12, a21, a22, gain, pan; };
  std::vector<Knock> knock_;
  std::vector<float> xPhi_;
  double xForce_ = 0, xT_ = -1, xLie_ = 0;
  float side_ = 0;
  double wireF_ = 0;
  bool asleep_ = true;
  int quiet_ = 0;
};

}  // namespace pajupilli
