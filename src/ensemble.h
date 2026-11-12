// Polyphony: a pool of flutes (each its own model and player) that the keys are
// dealt to. A heuristic guesses how many lines are being played:
//   - keys struck together (within kChord) are a chord: one flute each;
//   - a key pressed while another is held is a slur on that flute (as with one
//     flute), unless more lines are expected than flutes are holding keys;
//   - keys held together on one flute longer than kSplit (100 ms) are separate
//     lines: each older note moves to a free flute from the pool, which takes
//     it up afresh;
//   - the number of lines expected is the most seen lately, and drops back
//     after kForget seconds with fewer.
// A new line goes to the free flute that played nearest in pitch and most
// recently, so lines keep to their flutes. With one flute this is exactly the
// single-flute behaviour.
#pragma once

#include <memory>
#include <vector>

#include "calibrate.h"
#include "voice.h"

namespace pajupilli {

class Ensemble {
 public:
  Ensemble(const Instrument& inst, const Calibration& cal, int maxFlutes, bool verbose);
  void noteOn(int midi, int velocity);
  void noteOff(int midi);
  // A player setting, for every flute.
  void all(void (Player::*set)(double), double x) {
    for (Slot& s : f_) (s.v->player().*set)(x);
  }
  void setHold(bool h) {
    for (Slot& s : f_) s.v->player().setHold(h);
  }
  void render(float* out, int n);  // 48 kHz, mixed
  int resets() const;
  int lines() const { return lines_; }
  int size() const { return int(f_.size()); }

 private:
  struct Slot {
    std::unique_ptr<Voice> v;
    int held = 0;            // keys down on this flute
    int lastNote = -1;
    double lastOff = -1e9;   // when its last key went up (s)
    bool active = false;     // being rendered
  };
  int pickFree(int midi) const;   // a flute with no keys down, or -1
  int nearestHeld(int midi) const;
  int holding() const;            // flutes with keys down
  void split();
  void take(int flute, int midi, int velocity);

  static constexpr double kChord = 0.035, kSplit = 0.10, kForget = 2.0, kIdle = 1.0;
  std::vector<Slot> f_;
  int owner_[128];
  double pressT_[128] = {};
  int vel_[128] = {};
  double now_ = 0, calm_ = 0, mix_ = 1;
  int lines_ = 1;
  bool verbose_;
  std::vector<float> buf_;
};

}  // namespace pajupilli
