// The double bass as an instrument of the program (-inst double-bass): the
// model (bass.h) behind the Tube and Performer interfaces, as the synth leads
// are (lead.h). One voice is one bass: its player takes up to four keys, one
// per string (the ensemble's pool is one voice).
#pragma once

#include <algorithm>
#include <vector>

#include "bass.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

class BassModel : public Tube {
 public:
  explicit BassModel(const Instrument& inst);
  Bass& bass() { return bass_; }
  double testResponse() const override { return last_; }
  // The last n samples' side signal (left = out + side, right = out - side);
  // taken once: cleared for the next block.
  const float* takeSide(int n) {
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
  Bass bass_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

// The bassist. A key is a string and a place on it: the highest string whose
// open note is not above the key (the low positions), or, if that string is
// held by another key, the next one down, higher up its neck. Velocity is
// the pull's depth and the right hand's way: soft keys (up to 45) the thumb,
// then (to 72) the flat finger, the index's side along the string, then the
// stacked pull (index and middle as one), hard ones (from 100)
// the pull turned outward, the string snapping back onto the fingerboard
// (fully a slap at 127). Fingertips in turn for fast notes. The hardness
// knob down: always the thumb; the air knob up: the outward pull at any
// velocity; the brightness knob is the place along the string. The joystick
// slides the stopping finger (sideways) and rocks it (forward: vibrato). The
// mute pad (pad 2) held: ghost notes; struck: a rake over three muted
// strings. A short note is stopped dry, a long one let go slowly. Keys above
// G4, to C6, are
// harmonics: natural ones of the open strings where there is one, otherwise
// a stopped note touched at its half, third or quarter. A soft key under a
// held one (the glide gesture of the other instruments: 45 or softer, and 30
// softer than the last) is played by the left hand alone on that string: a
// hammer-on or pull-off (up to two semitones) or a slide. Releasing a key
// stops its string with the left hand. Vibrato (its knob): the fingertip
// rocks on a stopped string, after a moment.
class BassPlayer : public Performer {
 public:
  explicit BassPlayer(BassModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned s) override;
  static double pluckPlace(double brightness);          // share from the bridge; -1: behind it
  static const char* pluckPlaceName(double brightness);

 private:
  int pickString(int midi) const;
  BassModel& m_;
  int key_[4] = {-1, -1, -1, -1};      // the key on each string
  double since_[4] = {1e9, 1e9, 1e9, 1e9};  // s since its note began
  int lastVel_ = 64, lastString_ = -1;
  bool harm_[4] = {};  // a harmonic sounds on the string
  double sinceAny_ = 1e9;  // s since the last pluck
  int hand_ = -1;          // where the left hand is (semitones above the open strings)
  double scoop_[4] = {};   // each note's slide into pitch (cents)
  bool rakeDown_ = false;
  int rakeNext_ = 0;
  double rakeClock_ = 0, rakeDepth_ = 0;
  double vibPhase_ = 0;
  uint32_t rng_ = 777;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; }
};

}  // namespace pajupilli
