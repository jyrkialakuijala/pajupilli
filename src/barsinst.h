// The chromatic percussion as instruments (-inst celesta, glockenspiel,
// marimba, xylophone, tubular-bells, music-box): the engine (bars.h) behind
// the Tube and Performer interfaces, as the piano is. One voice is the whole
// instrument.
#pragma once

#include <algorithm>
#include <vector>

#include "band.h"
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
// A mallet with two faces (the tubular bells' rawhide and wood, their cast bells' padded
// face and iron): the loud strokes with the hard one, the soft with the soft, turned only
// when there is time between the strokes (research/tubular_bells.md 4.3). The tubular bells
// tune each stroke by ear (jyrki-spectral, 9): a key not ringing is set where its own
// partials meet the band's, within PJ_BELLS_TUNE_LIM cents (15); PJ_BELLS_TUNING=tet12: not.
class BarsPlayer : public Performer {
 public:
  explicit BarsPlayer(BarsModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}
  void setBand(const SoundingSet* band, int self) override { band_ = band; self_ = self; }
  double tuning(int key) const override { return cents_[key & 127]; }

 private:
  int keyOf(int midi) const;
  int faceFor(int velocity);
  double pull(int key, int midi, double* weight) const;
  BarsModel& m_;
  bool down_[Bars::kMaxKeys] = {};
  const SoundingSet* band_ = nullptr;
  int self_ = -1;
  double cents_[128] = {};
  double clock_ = 0, lastStroke_ = -1e9;
  int face_ = 0;
};

}  // namespace pajupilli
