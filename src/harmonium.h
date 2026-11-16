// The reed organ (General MIDI 21, -inst reed-organ; research/reed_organ.md §8): a pressure
// harmonium of the Mustel / Schiedmayer Kunstharmonium kind, the reeds blown (not sucked).
//
//   - The wind: two feeders trodden in turn fill the air space. With Expression (the default) the
//     reservoir is shut off and the feet's pressure is the reeds' pressure: the dynamics. The
//     feet aim at a pressure from the key's velocity (and CC2 / channel pressure), each stroke's
//     changeover a small dip; a heavy chord at ff takes more than the feeders give and sags it.
//     Expression off (PJ_HARM_EXPR=0): the reservoir's steady ~800 Pa.
//   - The reeds: brass tongues, each in its cell *downstream* (src/freereed.h, sampleDown): the
//     chest's wind through the reed into the cell, the cell out through the key's pallet. Rows
//     (stops), both halves of the keyboard (the split at E4/F4): 1 Cor anglais / Flûte 8' (front,
//     broad, with the Percussion hammer), 2 Bourdon / Clarinette 16' (front), 3 Clairon / Fifre 4'
//     (back), 4 Basson / Hautbois 8' (back, narrow); the treble's Voix céleste 8' (back, ~1.5
//     beats/s at A4). PJ_HARM_REG: default (1 + 4, Percussion), flute (1), celeste (1 + céleste),
//     grandjeu (1 2 3 4).
//   - The sound: the pallets' flow; the front rows through a gentle low-pass, the back rows under
//     the closed forte lids (darker, quieter); the bass a little left, the treble right.
//   - The sustain pedal holds the keys down. No pitch bend (fixed reeds).
#pragma once

#include <vector>

#include "freereed.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool harmoniumInstrument(const Instrument& inst);  // -inst reed-organ

class HarmoniumModel : public Tube {
 public:
  explicit HarmoniumModel(const Instrument& inst);
  static constexpr int kLo = 36, kHi = 96;
  void press(int midi, int velocity);
  void release(int midi);
  void setFeet(double pa, double tau) { feetP_ = pa; feetTau_ = tau; }
  void setFeetPressure(double pa) { feetP_ = pa; }
  double wind() const { return pL_; }
  bool asleep() const { return active_.empty(); }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  struct Key {
    std::vector<FreeReed> reeds;
    std::vector<uint8_t> back;    // per reed: a back row (under the lids)
    std::vector<double> q;        // per reed: its pallet's damping (the cell's Q)
    std::vector<double> kick;     // per reed: the hammer's start (m/s at full velocity; 0: none)
    double open = 0, speed = 250, area = 30e-6;
    bool down = false;
    float pan = 0;
  };
  void controlStep() override {}
  float sample() override;
  std::vector<Key> keys_;
  std::vector<int> active_;
  double fs2_;
  bool expression_;
  double feetP_ = 0, feetTau_ = 0.05, pL_ = 0, use_ = 0, tread_ = 0;
  double front_ = 0, front2_ = 0, back1_ = 0, back2_ = 0, aFront_, aBack_;
  HalfBand downM_, downS_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

class HarmoniumPlayer : public Performer {
 public:
  explicit HarmoniumPlayer(HarmoniumModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}

 private:
  double pressure(int velocity) const;
  HarmoniumModel& m_;
  bool down_[128] = {}, sustained_[128] = {};
  int held_ = 0, vel_ = 80;
};

}  // namespace pajupilli
