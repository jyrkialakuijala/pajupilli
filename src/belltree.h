// The bell tree (-inst bell-tree; General MIDI 2's 84): research/
// clap_claves_surdo.md 5. Sixteen small brass cup bells nested on a rod, the
// largest at the bottom; a brass beater drawn down the stack strikes each in
// turn, top to bottom: a falling glissando that rings on together. Each bell
// its first three cup modes; a harder key is a faster, louder sweep. On the
// modal engine (modal.h).
#pragma once

#include <cstdint>
#include <vector>

#include "modal.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

struct Instrument;
bool bellTree(const Instrument& inst);  // -inst bell-tree

class BellTreeModel : public Tube {
 public:
  static constexpr int kBells = 16;
  explicit BellTreeModel(const Instrument& inst);
  // A sweep down the stack: the beater's speed (m/s), the run's length (s).
  void sweep(double speed, double dur);
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  ModalSet set_;
  double gain_ = 1;
  // The sweep under way: the next bell (from the top), when (samples), each
  // bell's speed.
  int next_ = kBells;
  double at_ = 0, step_ = 0, speed_ = 0;
  uint32_t rng_ = 77;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

class BellTreePlayer : public Performer {
 public:
  explicit BellTreePlayer(BellTreeModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int) override {}
  void render(float* out, int n) override;
  void setHold(bool) override {}
  void seed(unsigned) override {}

 private:
  BellTreeModel& m_;
};

}  // namespace pajupilli
