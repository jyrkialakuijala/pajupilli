// The chromatic percussion as instruments (-inst celesta, glockenspiel,
// marimba, xylophone, tubular-bells, music-box): the engine (bars.h) behind
// the Tube and Performer interfaces, as the piano is. One voice is the whole
// instrument.
#pragma once

#include <algorithm>
#include <vector>

#include "bars.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool barInstrument(const Instrument& inst);

class BarsModel : public Tube {
 public:
  explicit BarsModel(const Instrument& inst);
  Bars& bars() { return bars_; }
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
  static BarDesign designOf(const Instrument& inst);
  void controlStep() override {}
  float sample() override;
  Bars bars_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

// The player. A key is a stroke: its velocity the mallet's (or hammer's)
// speed, between the design's softest and hardest. Where the instrument has
// dampers (the celesta), the key lifts its damper and letting it go drops it,
// unless the pedal is down, which lifts them all; the others ring on. The
// keys are at sounding pitch; notes outside the instrument's compass are
// folded in by octaves.
class BarsPlayer : public Performer {
 public:
  explicit BarsPlayer(BarsModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}

 private:
  int keyOf(int midi) const;
  BarsModel& m_;
  bool down_[Bars::kMaxKeys] = {};
};

}  // namespace pajupilli
