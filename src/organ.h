// The church organ (research/church_organ.md), its first ranks, each key its
// own pipe (src/organpipe.h: the jet and the column's modes), voiced for its
// pitch and calibrated when built, at its place in its division
// (src/organbox.h), every division on a divided chest: the C side on the
// left, the C# side on the right.
//
//   -inst gedackt-8       Schwellwerk Gedackt 8': stopped wood, in two swell
//                         boxes high on the gallery, ~5 m apart (the C side's
//                         box left, the C#'s right), the bass octave offset
//                         along each box's back wall; the swell pedal (the
//                         dynamics knob, the breath) opens both boxes' shutters.
//   -inst rohrgedackt-8   Positiv Rohrgedackt 8': metal with chimneys, in the
//                         Rückpositiv's open case at the gallery rail (its two
//                         halves C left, C# right), nearer the church.
//   -inst gedackt-rohrgedackt  both drawn, the manuals coupled: each key sounds
//                         both, from the boxes and from the rail; the swell box
//                         a little warmer, so the two flutes beat gently.
// Steady wind for now (the blower and the wind's sag: §4a, later). The keys
// have no velocity (a pallet opens the same however the key is struck).
#pragma once

#include <memory>
#include <vector>

#include "organbox.h"
#include "organpipe.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool organRank(const Instrument& inst);

class OrganModel : public Tube {
 public:
  explicit OrganModel(const Instrument& inst);
  static constexpr int kLo = 36, kHi = 96;
  void open(int midi);
  void close(int midi);
  void setSwell(double s);  // 0 closed .. 1 open
  bool asleep() const;
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  struct Pipe {
    FluePipe p;
    int enc = 0, src = 0;
    int near[4] = {-1, -1, -1, -1};  // the neighbours on its chest (a whole tone, two, away)
    float nearG[4] = {};
    float y = 0;     // its last sample
    float hear = 1;  // how strongly its jet hears the field at its mouth
  };
  void controlStep() override {}
  float sample() override;
  void addRank(int rank);
  std::vector<Pipe> pipes_;
  std::vector<int> byKey_[128];
  std::vector<Enclosure> enc_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

class OrganPlayer : public Performer {
 public:
  explicit OrganPlayer(OrganModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}

 private:
  OrganModel& m_;
};

}  // namespace pajupilli
