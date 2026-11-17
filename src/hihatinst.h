// The hi-hat as an instrument (-inst hi-hat): the pair of cymbals (hihat.h)
// behind the Tube and Performer interfaces, with the drummer's foot and hand.
#pragma once

#include <algorithm>
#include <vector>

#include "hihat.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool hiHat(const Instrument& inst);

class HiHatModel : public Tube {
 public:
  explicit HiHatModel(const Instrument& inst);
  HiHat& hat() { return hat_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override {
    sideOut_.assign(n, 0.0f);
    const int have = int(side_.size()), m = std::min(n, have);
    std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
    side_.clear();
    return sideOut_.data();
  }
  void resetAir() override;

 private:
  float shelfM_ = 0, shelfS_ = 0;  // (the highs' shelf)
  void controlStep() override {}
  float sample() override;
  HiHat hat_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

// The drummer at the hi-hat. The keys are General MIDI's:
//   42  closed: the foot down (it closes first if it was up), a stroke;
//   44  the foot alone: the pair closes, the "chick";
//   46  open: the foot up, a stroke; it rings until a 42 or 44 closes it.
// Velocity: the stroke's speed (hard ones with the stick's shoulder on the
// edge); for the foot, how fast and how hard it comes down (a heavy foot: a
// tighter, shorter closed sound).
// The foot keeps time as a drummer's does: after a chick it comes up again
// a little, quietly, ready for the next (unless closed strokes are being
// played). Each time it comes up the rod leans its own way, so the upper
// cymbal hangs differently for the next closing: sometimes level with the
// lower one (it lands flat: a clap on a cushion of air), more often not (one
// edge first, the rest rolling round after it).
class HiHatPlayer : public Performer {
 public:
  explicit HiHatPlayer(HiHatModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int) override {}
  void render(float* out, int n) override;
  void setHold(bool h) override { hold_ = h; }
  void seed(unsigned s) override { rng_ = 31337u + 7919u * s; }

 private:
  double ur();
  void lean();          // where the rod leaves the upper plate hanging next
  void stroke(int velocity);
  void control(double dt);
  HiHatModel& m_;
  uint32_t rng_ = 31337u;
  enum Foot { kDown, kReady, kOpen } foot_ = kDown;
  double leanX_ = 0, leanY_ = 0;
  double strokeIn_ = -1, closeIn_ = -1, liftIn_ = -1, since42_ = 100;
  int strokeVel_ = 0;
  double closeSpeed_ = 0.6, closeForce_ = 30;
  bool brush_ = false;
  double flat_ = 0.35;
};

}  // namespace pajupilli
