// The free-reed instruments with a bellows (research/free_reeds.md §3, §4,
// §7): the accordion (General MIDI 22, -inst accordion) and the tango
// accordion, the bandoneón (GM 24, -inst bandoneon).
//
// Every key opens a pallet over its reeds' chamber; each reed set has two
// reeds on one plate (src/freereed.h), one speaking on the push, the other on
// the pull, each a little different. One bellows feeds them all: its
// pressure follows the arm (the key's velocity: how hard the player pushes;
// CC2 / channel pressure on top), sags a little under the air a chord takes,
// and reverses now and then as the bellows run out, every few seconds,
// between notes if it can (the push reeds hand over to the pull reeds
// through a short gap as the pressure passes zero).
//
//   - Accordion: the right hand a French-style musette, three 8' sets, the
//     middle one in the cassotto (the tone chamber: its highs lowered), the
//     two outside it tuned apart by a few beats a second (PJ_ACC_BEAT);
//     the keys below C3 the bass, 16' + 8' in the cassotto.
//   - Bandoneón: two reeds an octave apart on each note (8' + 4'), no
//     cassotto, no musette; zinc plates nailed on (a quicker, brighter
//     attack) and a harder arm (tango's marcato).
//   - Pitch bend down: the player pulls the bellows harder against a
//     pallet half shut, as on the accordion (EMS21: 10-35 cents down, the
//     level falling; never up): the duct's air heavier, the pressure higher.
//
// The reeds run at twice the output rate (their puffs are sharp), then a
// half-band filter brings them to 48 kHz. Bass reeds sound from the left
// (the bass box), the treble from the right.
#pragma once

#include <vector>

#include "freereed.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool accordionInstrument(const Instrument& inst);  // -inst accordion, bandoneon

class AccordionModel : public Tube {
 public:
  explicit AccordionModel(const Instrument& inst);
  static constexpr int kLo = 24, kHi = 108;
  void press(int midi, int velocity);
  void release(int midi);
  // The arm: the bellows' pressure (Pa) it aims for, and how fast it gets
  // there (s).
  void setArm(double pa, double tau) { armP_ = pa; armTau_ = tau; }
  void setArmPressure(double pa) { armP_ = pa; }
  // Pitch bend (-1 .. 0: how hard the pallet is choked and the bellows pushed).
  void setBend(double b) { bend_ = b; }
  bool asleep() const;
  bool anyKey() const;
  double bellows() const { return pb_; }
  int direction() const { return dir_; }
  void reverse() { dir_ = -dir_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  struct Key {
    std::vector<FreeReed> reeds;  // per set: push, pull
    std::vector<uint8_t> group;   // per reed: the cassotto (1) or not (0)
    double open = 0;              // the pallet, 0 shut .. 1
    double speed = 1 / 0.004;     // its stroke (1/s)
    bool down = false;
    float pan = 0;
  };
  void controlStep() override {}
  float sample() override;
  float half(float* hist, int& pos, float x0, float x1);
  std::vector<Key> keys_;
  std::vector<int> active_;
  bool bandoneon_;
  double fs2_;
  int dir_ = 1;
  double pb_ = 0, armP_ = 0, armTau_ = 0.03, bend_ = 0, flow_ = 0;
  // The radiation's filters (at 2x): the cassotto's low-pass, the grille's,
  // the pull side's (the reeds sounding into the bellows).
  double cass1_ = 0, cass2_ = 0, grille_ = 0, pull_ = 0, pullS_ = 0;
  double aCass_, aGrille_, aPull_;
  static constexpr int kHalf = 48;
  float histM_[2 * kHalf] = {}, histS_[2 * kHalf] = {};
  int posM_ = 0, posS_ = 0;
  float hb_[kHalf];
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

class AccordionPlayer : public Performer {
 public:
  explicit AccordionPlayer(AccordionModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool) override {}  // (no sustain pedal)
  void seed(unsigned s) override { rng_ = 4242u + 7919u * s; }

 private:
  double pressure(int velocity) const;
  AccordionModel& m_;
  bool bandoneon_;
  bool down_[128] = {};
  int held_ = 0;
  int vel_ = 80;
  double travel_ = 0, travelMax_ = 3.5, lastOff_ = -1e9;
  uint32_t rng_ = 4242;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (1.0 / 16777216.0); }
};

}  // namespace pajupilli
