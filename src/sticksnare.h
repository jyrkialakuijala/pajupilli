// The snare drum with sticks (-inst stick-snare; research/stick_snare.md):
// the brushed snare's drum (snare.h: two heads on a shell of air, the wires
// on the lower one, the microphones), wrapped, played with a pair of sticks.
//
//   - A stick: a mass on a stiff contact, thrown from just over the batter
//     where it is now (a stroke meets the ringing), the force found for the
//     end of each sample as the head gives under it; it rebounds and is gone.
//     The wires answer the snare head as it moves: a ghost note a short thin
//     buzz, an accent a long rattle.
//   - The stick rings too (its three bending modes, driven by the contact
//     force, the hand damping them), and the contact's own click above what
//     the head's modes carry.
//   - The open rim shot: a heavier stroke near the rim, the shaft striking
//     the hoop at once: the hoop's ring and the stick's, a loud crack.
//   - The cross-stick: the snare's own (the butt on the hoop, the tip end on
//     the head).
//   - The buzz roll: the two hands in turn, each stroke pressed into the
//     head and bouncing on it, a little sooner and weaker each time.
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "snare.h"
#include "tube.h"

namespace pajupilli {

bool stickSnare(const Instrument& inst);  // -inst stick-snare

class StickSnare {
 public:
  explicit StickSnare(double fs);
  // A stroke of hand 0 or 1: speed (m/s), at r (share of the radius), theta
  // (rad); mass: what of the stick is behind it (kg).
  void stick(int hand, double speed, double r, double theta, double mass = 0.015);
  // The open rim shot: the tip near the rim, the shaft on the hoop.
  void rimShot(int hand, double speed, double theta);
  void crossStick(double speed) { snare_.crossStick(speed); }
  float sample();
  float side() const { return side_; }
  void reset();
  bool asleep() const;
  Snare& snare() { return snare_; }
  bool touching() const { return st_[0].on || st_[1].on; }
  double force() const { return lastF_; }

 private:
  struct Stick {
    std::vector<float> phi;  // its place on the batter: each mode's share
    double z = 1, v = 0, m = 0.015, G = 0;
    int age = 0;
    bool on = false;
  };
  struct Ring { float x = 0, v = 0, a11, a12, a21, a22, gain, pan; };
  Snare snare_;
  double fs_, dt_, R_;
  Stick st_[2];
  std::vector<Ring> stickRing_, hoop_;
  float hpC_ = 0, lastC_ = 0, side_ = 0;
  double lastF_ = 0, K_ = 2e8;
  uint32_t rng_ = 12345u;
  int quiet_ = 0;
};

class StickSnareModel : public Tube {
 public:
  explicit StickSnareModel(const Instrument& inst);
  StickSnare& drum() { return d_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  StickSnare d_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

// The drummer with a pair of sticks. Keys:
//   36  the buzz roll, for as long as the key is held
//   37  the cross-stick (side stick)
//   38  a stroke, between the middle and a third of the way out
//   39, 41  the open rim shot
//   40  a stroke halfway out (more ring)
//   42  a stroke near the edge (thin)
//   43, 44  strokes, as 38
// Velocity is the stick's speed (ghost notes 0.2 .. 0.5 m/s, accents 3 .. 6);
// the hands alternate; each stroke lands a little elsewhere.
class StickSnarePlayer : public Performer {
 public:
  explicit StickSnarePlayer(StickSnareModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override { hold_ = h; }
  void seed(unsigned s) override { rng_ = 4241u + 7919u * s; }
  static double stickSpeed(int velocity, double dynamics);  // m/s

 private:
  double ur();
  void rollStep(double dt);
  StickSnareModel& m_;
  uint32_t rng_ = 4241u;
  int hand_ = 0;
  // The roll: held, its velocity; the next hand stroke's time; the bounces
  // to come (time, hand, speed).
  bool rolling_ = false;
  int rollVel_ = 0;
  double rollT_ = 0, nextStroke_ = 0;
  struct Bounce { double t; int hand; double speed; };
  std::vector<Bounce> bounces_;
};

}  // namespace pajupilli
