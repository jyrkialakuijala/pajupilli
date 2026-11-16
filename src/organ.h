// The church organ (research/church_organ.md), its first rank: Gedackt 8'
// (-inst gedackt-8), a stopped wooden flute, C2..C7 (MIDI 36..96; 8' C at
// 65.4 Hz). Each key its own pipe (src/organpipe.h: the jet and the stopped
// column's odd modes), voiced for its pitch and calibrated when built so it
// speaks in tune. The chest: C side and C# side, the pipes alternating left
// and right by semitone, the big ones outside. Steady wind for now (the
// blower, reservoirs and the wind's sag: §4a, later). The keys have no
// velocity: an organ's pallet opens the same however the key is struck.
#pragma once

#include <vector>

#include "organpipe.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool organRank(const Instrument& inst);  // -inst gedackt-8

class OrganModel : public Tube {
 public:
  explicit OrganModel(const Instrument& inst);
  static constexpr int kLo = 36, kHi = 96;
  void open(int midi);
  void close(int midi);
  bool asleep() const;
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  std::vector<FluePipe> pipes_;
  std::vector<float> pan_;  // each pipe's place across the chest, -1 .. 1
  float last_ = 0;
  std::vector<float> side_, sideOut_;
  float sideNow_ = 0;
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
  bool down_[128] = {};
};

}  // namespace pajupilli
