// General MIDI 98, "FX 2 (soundtrack)" (-inst fx-soundtrack): a keyed aeolian harp
// (research/fx_sounds.md).
//
// The classic patch (a classic digital synth's "Soundtrack", kept by every General MIDI set) is a dark,
// chorused pad of the note and its fifth. Here it is strings sung by the wind. Each key has a
// course of four nylon strings: three on the note (a few cents apart, by their tensions: they
// beat, a chorus) and one on its fifth (3:2). A blower keeps a steady wind (shared by every key,
// a reservoir steadying it to +-0.3 %: a wake catches its lock only within ~+-2 %); the key
// slides a shutter that lets it across its course, as far and as quickly as the touch.
//
// The wind sheds vortices off each string, alternately from its two sides, at the Strouhal
// frequency (f = St U / D, St = 0.2), the strings' diameters chosen so that it lies at the
// string's first mode. The wake is a van der Pol oscillator (Facchinetti, de Langre and Biolley
// 2004: q'' + eps W (q^2 - 1) q' + W^2 q = A y'', eps = 0.3, A = 12), one in each cell along
// the string; its lift (M W^2 q, M from the string's mass ratio, mu ~ 750 for nylon in air)
// drives the string's modes (each a damped resonator, plus the air's own damping while the wind
// blows), and the string's motion feeds back into the wake: they lock (vortex-induced
// vibration). So the note is the string's own pitch, and it blooms: the string's swing builds
// over ~1/(zeta w), half a second or so, behind the shutter.
//
// Dark to bright: as the shutter slides it uncovers the main stretch (two cells at the first
// mode's lock speed) and, one after another, short side slots where the wind is two .. five
// times faster, each across the antinode of the mode it locks (2 .. 5). And the string's tension
// rises with the square of its swing: the bridge feels it along the string (the octave) and in
// its product with the slope. A soft key: the main stretch partly, the fundamental and its
// fifth; a hard one: the side slots too, the harmonics. The slots hiss a little (turbulence).
// Released, the shutter closes and its felt edge rests on the strings (T60 ~2.5 s at middle C).
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool fxSoundtrack(const Instrument& inst);  // -inst fx-soundtrack

class AeolianModel : public Tube {
 public:
  explicit AeolianModel(const Instrument& inst);
  void keyOn(int midi, int velocity);
  void keyOff(int midi);
  void setBend(double semitones);
  // The knobs, 0..1 (AeolianPlayer::setKnob).
  void setGusts(double x) { gusts_ = x; }  // K1: the wind's gusts (0.04 .. 2.4 %; 0.5: 0.3 %)
  void setSwell(double x) { swell_ = x; }  // K2: the shutter's time (x 0.25 .. 4)
  void setFifth(double x) { fifth_ = x; }  // K3: the fifth string's exposure (0 .. 2 x 0.6)
  void setRing(double x) { ring_ = x; }    // K4: the strings' ring (T60 x 0.25 .. 4)
  bool asleep() const { return notes_.empty(); }
  double testResponse() const override { return last_; }
  void resetAir() override;
  const float* takeSide(int n) override;

  static constexpr int kModes = 6, kCells = 6, kStrings = 4;
  struct Mode {
    double re = 0, im = 0;  // y - i y'/w (y in diameters)
    double c = 1, s = 0;    // the rotation a sample (bent)
    double w = 0;           // angular frequency (rad/s, bent)
    double f = 0;           // unbent frequency (Hz)
    double zeta = 0;        // the string's own damping ratio
    double r = 1;           // the decay a sample (with the air's)
  };
  struct Cell {
    double q = 0, qd = 0;   // the wake variable and its rate
    double x = 0.3;         // where along the string (0..1)
    double speed = 1;       // its local wind over the first mode's lock speed (~ the mode it locks)
    double span = 0.25;     // its share of the string's length
    double lo = 0, hi = 1;  // the shutter's travel over which it is uncovered
    double phi[kModes] = {};  // the modes' shapes there
    double W = 0, We2 = 0, hs = 0;  // the shedding's angular frequency; the step's (damp())
    int steps = 1;
  };
  struct String {
    double f = 0;           // first mode (Hz, unbent)
    double gain = 1;        // exposure share (the fifth less)
    double pan = 0;
    double tension = 0;     // the swing's tension rise (relative, for the bridge)
    int modes = 0, cells = 0;
    Mode mode[kModes];
    Cell cell[kCells];
  };
  struct Note {
    int midi = 0;
    bool down = true;
    double voice = 1;       // the key's level (a voicing)
    double travel = 0;      // the shutter's travel (0 .. 1)
    double open = 0;        // ... smoothed (its inertia)
    double target = 1;      // how far it goes (by the touch)
    double rate = 0;        // its speed (travel a sample)
    String str[kStrings];
    int quiet = 0;
  };

 private:
  void controlStep() override {}
  float sample() override;
  void tune(String& s);
  void damp(Note& n);
  static double exposure(const Cell& z, double open);
  std::vector<Note> notes_;
  std::vector<float> side_, sideOut_;
  double bend_ = 0, bk_ = 1;
  double gusts_ = 0.5, swell_ = 0.5, fifth_ = 0.5, ring_ = 0.5;
  double gust_ = 0, gustF_ = 1;  // the blower's wind: an Ornstein-Uhlenbeck walk, its factor on the speed
  int count_ = 0;
  double dcIn_ = 0, dcOut_ = 0, dcSideIn_ = 0, dcSideOut_ = 0;
  double hissLo_ = 0, hissBand_ = 0;  // the slots' hiss (two one-poles)
  uint32_t rng_ = 4241;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (1.0 / 16777216.0); }
  float last_ = 0;
};

// Velocity is the shutter's width and speed; the sustain pedal holds the keys; the pitch bend is
// +-2 semitones. Knobs (the controller's K1-K4, -seq's "knob K X"): K1 the wind's gusts (turned up,
// the level and the brightness breathe; a note struck in a strong gust may not catch), K2 the
// swell's time, K3 the fifth, K4 the strings' ring.
class AeolianPlayer : public Performer {
 public:
  explicit AeolianPlayer(AeolianModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}
  bool setKnob(int k, double x) override;

 private:
  AeolianModel& m_;
  bool keyDown_[128] = {}, sounding_[128] = {};
};

}  // namespace pajupilli
