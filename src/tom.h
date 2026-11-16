// The jazz toms (-inst tom-10, tom-12, tom-13, tom-14, tom-16, tom-16-low;
// research/toms.md): two-headed drums tuned high and open, played with a
// stick or a wire brush.
//
//   - Two heads (membrane.h) on a shell of air (DrumAir): a coated batter and
//     a clear resonant head at the same lug pitch (the most sustain). The air
//     couples their m = 0 modes: the drum's note is the pair's lower member,
//     both heads moving together, well below either head's own (0,1).
//   - The tension is solved for: at construction the pair is struck and
//     listened to until its note is the drum's.
//   - A stick: a mass on a stiff contact, thrown from just over the head where
//     the head is now (a stroke meets the ringing). The contact is solved
//     for the end of each sample, as the head gives under it; the stick
//     rebounds and is gone. One key, one stroke.
//   - A brush (brush.h): the pull-out stroke. The hand turns up as the wires
//     land, the contact is over in a few ms, and the drum rings free. Struck
//     very softly, the brush is set down and drawn a little way instead: a
//     short hiss with the drum's pitch in it.
//   - A hard stick stroke stretches the heads: the note starts sharp and
//     falls. A brushed one hardly moves them, and keeps its pitch.
//   - The sound: two microphones over the batter (the resonant head heard
//     round the shell), and the contact's own click and the wire ends' "tss"
//     above what the modes carry.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "brush.h"
#include "membrane.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool jazzTom(const Instrument& inst);

struct TomSpec {
  HeadSpec batter, reso;
  DrumAir air;
  BrushSpec brush;
  double note = 110;             // the drum's note (Hz): the in-phase (0,1) pair
  double depth = 0.356;          // shell, m
  double stickMass = 0.015;      // kg: what of the stick is behind the blow
  double stickK = 2e8;           // N/m^1.5: the tip on the coated film (Hertz)
  double micX = 0.2, micZ = 0.4; // the microphones: +- x, over the batter (m)
  double under = 0.5;            // the resonant head's share at them (round the shell)
  double click = 1, tss = 1;     // the stick's and the wire ends' own sound (x)
  double gain = 1;
};
TomSpec tomSpec(const char* name);  // with the PJ_TOM_* knobs

class Tom {
 public:
  Tom(double fs, const TomSpec& spec);
  // A stick stroke: speed (m/s) at r (share of the radius), theta (rad).
  void stick(double speed, double r, double theta);
  // A brushed pull-out stroke: the hand's speed (m/s), at r, theta.
  void brush(double speed, double r, double theta);
  // The brush set down with `load` N and drawn from (r, theta) by `dist` m over
  // `dur` s, then lifted (the softest brushed strokes).
  void drag(double load, double r, double theta, double dist, double dur);
  void setOutside(float pBatter, float pReso);  // the room's sound at the heads (Pa)
  float sample();
  float side() const { return side_; }
  void reset();
  bool asleep() const { return asleep_; }
  const TomSpec& spec() const { return s_; }
  Membrane& batter() { return bat_; }
  Membrane& reso() { return res_; }
  Brush& hand() { return *brush_; }
  bool touching() const { return stickOn_ && lastF_ > 0; }
  double note() const { return s_.note; }

 private:
  TomSpec s_;
  double fs_, dt_;
  Membrane bat_, res_;
  DrumAir air_;
  std::unique_ptr<Brush> brush_;
  std::vector<float> phi_;       // the stick tip's place, each mode's share
  double zs_ = 1, vs_ = 0;       // the stick tip: height over the head's rest (m), speed
  double sG_ = 0;                // what the head gives there in one sample, per unit force (m/N)
  int sAge_ = 0;
  bool stickOn_ = false;
  double lastF_ = 0;
  // The drag: its path and time.
  double dT_ = -1, dDur_ = 0, dX_ = 0, dY_ = 0, dDx_ = 0, dDy_ = 0, dAng_ = 0;
  std::vector<float> micB_[2], micR_[2];
  float prev_[2] = {}, hpC_ = 0, lastC_ = 0, side_ = 0;
  bool asleep_ = true;
  int quiet_ = 0;
  float pOutB_ = 0, pOutR_ = 0;
};

class TomModel : public Tube {
 public:
  explicit TomModel(const Instrument& inst);
  Tom& tom() { return tom_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  Tom tom_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
  std::vector<float> delay_;  // the room's pressure, for the far head
  int delayPos_ = 0;
};

// The drummer's hand on one tom. Keys: General MIDI's toms (41, 43, 45, 47,
// 48, 50: struck with a stick) and the same 24 up (65 .. 74: with a brush,
// the Brush kit's); any other key is a stick stroke. Velocity is the speed
// (stick 0.1 .. 4 m/s; brush as the snare's). Each stroke lands a little
// elsewhere, between the middle and halfway out.
class TomPlayer : public Performer {
 public:
  explicit TomPlayer(TomModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int) override {}
  void render(float* out, int n) override;
  void setHold(bool h) override { hold_ = h; }
  void seed(unsigned s) override { rng_ = 6131u + 7919u * s; }
  static double stickSpeed(int velocity, double dynamics);  // m/s
  static double brushSpeed(int velocity, double dynamics);  // m/s

 private:
  double ur();
  TomModel& m_;
  uint32_t rng_ = 6131u;
};

}  // namespace pajupilli
