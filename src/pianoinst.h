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
  float treble_ = 0, trebleA_ = 0, trebleM_ = 0, trebleS_ = 0;  // (the honky-tonk's lifted highs)
  float air_ = 0, airA_ = 0, airM_ = 0, airS_ = 0;              // (... and its top)
  float lpB_[3] = {}, lpA_[2] = {}, lpZm_[2] = {}, lpZs_[2] = {};  // (... rolled off: no hiss)
  bool lpOn_ = false;
  struct Box {  // (the closed case's air: three round trips)
    static constexpr int kMax = 2048;
    float line[3][kMax] = {};
    int pos[3] = {};
    float lp[3] = {};
  };
  Box boxM_, boxS_;
  float box_ = 0, boxFb_ = 0, boxLp_ = 0;
  int boxLen_[3] = {2, 2, 2};
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
  void sustainPedal(int cc) override;
  // (-play) The next release's speed at the dampers (m/s), as the player chose it for the music
  // (an on/off pedal: how hard the foot lets go there); taken by that release.
  void planRelease(double speed) { plannedUp_ = speed; }
  void seed(unsigned) override {}
  static double hammerSpeed(int velocity, double dynamics);  // m/s

 private:
  int fold(int midi) const;
  double pedalMoved(bool h);
  PianoModel& m_;
  bool down_[Piano::kKeys] = {};
  // (the pedal: its controller's last value and when it came (s), the time now, the speed it
  // gave, whether it is down, the draws for each release's own speed)
  int pedalCC_ = 0;
  double pedalT_ = -1, now_ = 0, nextSpeed_ = -1, plannedUp_ = -1;
  bool pedalDown_ = false;
  uint32_t pedalRng_ = 9157;
  int lo_ = 0, hi_ = 127;
  bool held_ = false;  // the sustain pedal, as last told (printed when it changes)
  double t_ = 0;       // the player's clock (s)
};

}  // namespace pajupilli
