// The concert grand as an instrument of the program (-inst grand-piano): the
// model (piano.h) behind the Tube and Performer interfaces, as the double
// bass is (bassinst.h). One voice is the whole piano: its player takes every
// key (the ensemble's pool is one voice).
#pragma once

#include <algorithm>
#include <vector>

#include "performer.h"
#include "piano.h"
#include "tube.h"

namespace pajupilli {

bool grandPiano(const Instrument& inst);  // -inst grand-piano, upright-piano: a PianoModel (among the string instruments)

class PianoModel : public Tube {
 public:
  explicit PianoModel(const Instrument& inst);
  Piano& piano() { return piano_; }
  double testResponse() const override { return last_; }
  // The last n samples' side signal (left = out + side, right = out - side:
  // the microphones over the bass and the treble strings); taken once.
  const float* takeSide(int n) override {
    sideOut_.assign(n, 0.0f);
    const int have = int(side_.size()), m = std::min(n, have);
    std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
    side_.clear();
    return sideOut_.data();
  }
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  Piano piano_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

// The pianist. A key throws its hammer: the velocity is the hammer's speed
// (0.35 m/s at the softest to over 5 m/s; the dynamics control scales it),
// and lifts its damper; releasing it lets the damper down, unless the
// sustain pedal is held: the pedal lifts every damper, so all the strings
// ring along with what is played. The brightness knob is the hammers'
// voicing (down: soft felt, mellow; up: hard felt, bright); the hardness
// knob adds to it.
class PianoPlayer : public Performer {
 public:
  explicit PianoPlayer(PianoModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}
  static double hammerSpeed(int velocity, double dynamics);  // m/s

 private:
  int fold(int midi) const;
  PianoModel& m_;
  bool down_[Piano::kKeys] = {};
  int lo_ = 0, hi_ = 127;
};

}  // namespace pajupilli
