// The small wooden percussion of the drum kit, on the modal engine (modal.h):
// research/clap_claves_surdo.md 2-3.
//
//   - The claves (-inst claves; General MIDI's 75): two rosewood bars. One
//     lies in the cupped hand, held near its free-free nodes, the cupped hand
//     under it a resonator for its first mode; the other, held loosely at an
//     end, strikes it at its middle and rings too, lower and short (the grip
//     damps it). Wood on wood: a contact of ~0.1 ms.
//   - The wood blocks (-inst wood-blocks; 76 high, 77 low): a hardwood block
//     hollowed behind a slot, its thin lip over the cavity: a few strong,
//     nearly harmonic partials (the lip's bending with the cavity's air),
//     the rest gone at once; as measured on recorded ones (746 and 502 Hz).
//     Struck with the tip of a wooden stick (a small, hard contact, ~0.1 ms)
//     on top, over the cavity: roughly the same place each time, within
//     +-10 % of the block's width sideways and +-10 % of its length along
//     it; each mode by its shape there.
#pragma once

#include <cstdint>
#include <vector>

#include "modal.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

struct Instrument;
bool woodPerc(const Instrument& inst);  // -inst claves, wood-blocks

class WoodPercModel : public Tube {
 public:
  enum Kind { kClaves, kBlocks };
  explicit WoodPercModel(const Instrument& inst);
  Kind kind() const { return kind_; }
  // A stroke: speed (m/s); which block (0 high, 1 low; the claves: 0); where:
  // along (the claves: 0 .. 1 along the bar; a block: the offset from the
  // middle of its top, a share of its length) and across (a block: a share
  // of its width, from the middle over the slot).
  void strike(int which, double speed, double along, double across);
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  void define(int vib, double f0, double detune);
  Kind kind_;
  ModalSet set_;
  double gain_ = 1;
  double tuned_[2] = {-1, -1};  // each vibrator's detune now (redefined when it changes much)
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

class WoodPercPlayer : public Performer {
 public:
  explicit WoodPercPlayer(WoodPercModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int) override {}
  void render(float* out, int n) override;
  void setHold(bool) override {}
  void seed(unsigned s) override { rng_ = s * 2654435761u + 1; }
  static double speed(int velocity, double dynamics);  // m/s

 private:
  double ur();
  WoodPercModel& m_;
  uint32_t rng_ = 4242;
};

}  // namespace pajupilli
