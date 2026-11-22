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
              // GM 12 (the chromatic percussion).
              kVibraphone,
              // The sound effects (GM 121-128: sfx.h).
              kFretNoise, kBreathNoise, kSeashore, kBirdTweet, kTelephone, kHelicopter, kApplause, kGunshot };
  explicit ModalModel(const Instrument& inst);
  Type type() const { return type_; }
  ModalSet& set() { return set_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;
  // The vibraphone's bars at their places on the frame (vibPlace()), and the rest of the sound (the
  // DC blocker's share, the key ticks) at its centre: the last part.
  int parts() const override { return partPlace_.empty() ? 0 : int(partPlace_.size()) / 3; }
  const float* partPlaces() const override { return partPlace_.empty() ? nullptr : partPlace_.data(); }
  const float* takePart(int n, int j) override;
  // Bar i's place (m from the frame's centre: x to the player's right, y away from the player,
  // towards the audience; z 0: one plane, the damper pressing both rows).
  static void vibPlace(int i, double* x, double* y);

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
  // The vibraphone: bar i (MIDI 53 + i) is vibrator i; its fundamental is
  // mode 0, the air in its resonator mode kVibAir, coupled by a spring the
  // fan's discs open and close.
  static constexpr int kVibLo = 53, kVibBars = 37, kVibAir = 6, kVibHigh = 8;

 private:
  void controlStep() override {}
  float sample() override;
  void build();
  void build1();
  Type type_;
  ModalSet set_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
  std::vector<float> partPlace_;
  std::vector<std::vector<float>> part_;
  std::vector<float> partOut_;
};

class ModalPlayer : public Performer {
 public:
  explicit ModalPlayer(ModalModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  // The vibraphone's damper bar by the pedal's depth (half pedalling: the bar resting lightly on
  // the bars no longer held, they ring shorter); the rest as on/off.
  void sustainPedal(int cc) override;
  void seed(unsigned s) override;
  // The vibraphone's motor live: K4 how far the discs turn the tubes off (0: left open), K5 its speed.
  bool setKnob(int k, double x) override;
  // (-play) The motor as the vibist sets it through the piece: from each step's time (s, the
  // song's clock) its tremolo hz, gliding there over ~1 s (the knob, the motor's inertia); a step
  // locked to the beat brings its openings onto that grid (an opening at the step's start) within
  // the glide, else the shaft runs on from where it was. Empty: back to the knobs.
  struct MotorStep { double t, hz; bool locked; };
  void setMotorPlan(const std::vector<MotorStep>& plan);

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
  double fanPh_ = 0;  // the vibraphone's fan shaft (rad)
  double fanDepth_ = 1, fanRate_ = 0.7;  // (its motor: running by default, the slowest tremolo)
  // (-play: the motor's plan; each step's disc angle at its start, and the correction the glide
  // takes away (to the grid, a locked step))
  // (the vibraphone's damper bar: the pedal's depth 0 up .. 1 down, each bar's key let go)
  double pedalFree_ = 0;
  bool released_[ModalSet::kMaxVib] = {};
  double barDamp() const;
  static constexpr double kMotorGlide = 1.0;
  std::vector<MotorStep> motor_;
  std::vector<double> motorTh_, motorCorr_;
  size_t motorAt_ = 0;
  double motorAngle(double t);
  int bowKey_ = -1;   // the fiddle: the key under the bow
  double bowDir_ = 1, bowClock_ = 0, bowTarget_ = 0;
  uint32_t rng_ = 4711;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; }
};

}  // namespace pajupilli
