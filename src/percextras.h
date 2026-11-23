// The small sounds of the drum maps below 35 (research/gm2_extras.md):
// -inst perc-extras, one instrument holding them all, keyed by GS/GM2's
// numbers, with XG's extra small sounds on keys GS leaves free.
//
//   20 click noise (XG 20)       27 High Q (GS 27, XG 15)
//   21 castanets (XG 30, GM2 85) 28 slap: the whip (GS 28, XG 16)
//   23 seq click low (XG 23)     29 scratch push (GS 29, XG 17)
//   24 seq click high (XG 24)    30 scratch pull (GS 30, XG 18)
//   26 finger snap (GS 26, XG 19) 31 sticks (GS 31, XG 32)
//   32 square click (GS 32)      33 metronome click (GS 33, XG 21)
//   34 metronome bell (GS 34, XG 22)
//
// Struck things are physical, as small modal banks driven by the contact's
// force: two hickory sticks clicked together (their free-free bending
// modes, the hands' damping, the click's own crack), the whip's two boards
// (the air squeezed out between them, then the boards' modes), the finger's
// blow on the palm and the cupped hand's air, the castanets' shells and the
// air between them. The electronic ones (High Q, the clicks, the metronome)
// and the turntable's scratch are simple synthesis.
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool percExtras(const Instrument& inst);  // -inst perc-extras

class PercExtrasModel : public Tube {
 public:
  explicit PercExtrasModel(const Instrument& inst);
  // A stroke on key (the list above); v: 0 .. 1 (the key's velocity / 127).
  void hit(int key, double v);
  bool asleep() const { return ev_.empty(); }
  double testResponse() const override { return last_; }
  void resetAir() override;

  static constexpr int kModes = 16;
  struct Event {
    int kind = 0;
    int t = 0, life = 0;   // samples since the stroke, and how long it may last
    float amp = 0;         // the stroke's strength
    float pulse = 1;       // the contact's length (samples)
    int nm = 0;
    float re[kModes] = {}, im[kModes] = {}, pr[kModes] = {}, pi[kModes] = {}, w[kModes] = {};
    float inP[kModes] = {}, inN[kModes] = {};  // each mode's share of the contact's force, of the air burst
    float crack = 0, hp = 0, lastF = 0, hpPole = 0;  // the contact's own crack, high-passed
    float noise = 0, noiseTau = 1;                   // an air burst: level, decay (samples)
    double ph = 0;                                   // synth phase (cycles)
    float fr[3] = {}, fi[3] = {};                    // the scratch's formant states
    uint32_t rng = 1;
  };

 private:
  void controlStep() override {}
  float sample() override;
  void addMode(Event& e, double hz, double t60, double w, double inP = 1, double inN = 0);
  float step(Event& e);
  std::vector<Event> ev_;
  uint32_t rng_ = 99991u;
  float last_ = 0;
  double dt_;
};

// The player: velocity is the stroke's speed (a struck thing) or its level
// (an electronic one). Note-offs change nothing: every sound rings out.
class PercExtrasPlayer : public Performer {
 public:
  explicit PercExtrasPlayer(PercExtrasModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int) override {}
  void render(float* out, int n) override;
  void setHold(bool) override {}
  void seed(unsigned) override {}

 private:
  PercExtrasModel& m_;
};

}  // namespace pajupilli
