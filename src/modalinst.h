// The GM ethnic and percussive families on the modal engine (modal.h), as
// instruments of the program (-inst sitar, banjo, shamisen, koto, kalimba,
// fiddle, tinkle-bell, agogo, steel-drums, woodblock, taiko, reverse-cymbal).
// One voice is one instrument: its player gives the keys to its strings,
// tines, note areas or bells (the ensemble's pool is one voice).
#pragma once

#include <vector>

#include <memory>

#include "modal.h"
#include "sfx.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

class ModalModel : public Tube {
 public:
  enum Type { kSitar, kBanjo, kShamisen, kKoto, kKalimba, kFiddle, kTinkle, kAgogo, kSteelpan, kWoodblock, kTaiko,
              kRevCymbal,
              // The sound effects (GM 121-128: sfx.h).
              kFretNoise, kBreathNoise, kSeashore, kBirdTweet, kTelephone, kHelicopter, kApplause, kGunshot };
  explicit ModalModel(const Instrument& inst);
  Type type() const { return type_; }
  ModalSet& set() { return set_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

  // The instrument's own data (set by its builder, used by its player).
  struct Str {
    double f0 = 110, L = 0.65, T = 60, mu = 1e-3;  // open frequency, length (m), tension (N), mass/length (kg/m)
    double pluckAt = 0.15;                          // share of the length from the bridge
    int vib = 0;                                    // its vibrator
    // How it was made (to make it again, stopped shorter): inharmonicity,
    // modes, losses, the two directions' split, y only; its obstacles (from
    // the bridge: x, width, height, stiffness).
    double B = 0, t0 = 1, fc = 1, split = 0;
    int n = 0;
    bool one = false;
    struct Contact { double x, w, h, K; };
    std::vector<Contact> contacts;
    double fStop = 0;  // the stopped note it sounds now (0: open)
  };
  // String si stopped to sound f (a finger on a fret or the neck): a shorter
  // string at the same tension, its modes, mass, bridge force and obstacles
  // made again. (An obstacle at the nut's end moves with the finger: only the
  // open string reaches it.)
  void stopString(int si, double f);
  std::vector<Str> strings;  // the strings (or tines, note areas, bells) in order
  int pool = 0;              // struck things: how many vibrators take turns
  // The reverse cymbal: a crash rendered once (the physical cymbal, src/modal.cc
  // with its cascade), played backwards.
  std::vector<float> rev;
  double revPos = -1, revRate = 1, revGain = 0;
  double gain = 1;
  std::unique_ptr<Sfx> sfx;  // the sound effects
  // The fiddle's strings: bowed waveguides (Smith; STK's Bowed): the string
  // split at the bow into two delay lines (bow to nut and back, bow to
  // bridge and back), the bow's friction where they meet (a stick-slip
  // table), the bridge's lossy reflection, whose force drives the body.
  struct Bowed {
    static constexpr int kN = 2048;
    float neck[kN] = {}, bridge[kN] = {};
    int pos = 0;
    double f = 440, beta = 0.08, fNow = 440;
    double vel = 0, velTo = 0, press = 0, pressTo = 0;  // bow speed (STK units) and pressure 0..1
    float lp = 0, damp = 0.995f;                         // the bridge's reflection filter; the reflections' loss
    bool active = false;
    float step(float* bridgeForce);
  };
  Bowed bowed[4];

 private:
  void controlStep() override {}
  float sample() override;
  void build();
  void build1();
  Type type_;
  ModalSet set_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

class ModalPlayer : public Performer {
 public:
  explicit ModalPlayer(ModalModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned s) override;

 private:
  int pickString(int midi) const;    // fretted/stopped strings: the string for a key
  int nextInPool();                  // struck things: the next vibrator
  ModalModel& m_;
  ModalSet& s_;
  int keyOf_[ModalSet::kMaxVib];     // the key each vibrator sounds (-1: none)
  int vibOf_[128];                   // the vibrator a key sounds (-1: none)
  double sinceOn_[ModalSet::kMaxVib] = {};
  double lastCents_[ModalSet::kMaxVib] = {};  // the bend last applied to each
  int next_ = 0;
  bool hold_ = false;
  bool held_[128] = {};
  double bend_ = 0, vibPh_ = 0, clock_ = 0;
  int bowKey_ = -1;   // the fiddle: the key under the bow
  double bowDir_ = 1, bowClock_ = 0, bowTarget_ = 0;
  uint32_t rng_ = 4711;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; }
};

}  // namespace pajupilli
