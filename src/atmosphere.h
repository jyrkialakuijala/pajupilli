// General MIDI 100, "FX 4 (atmosphere)" (-inst fx-atmosphere): a keyed psaltery with magnetic
// resonators (research/fx_sounds.md).
//
// The classic patch (a classic digital synth's "Nylon Atmosphere", kept by every General MIDI set) is a
// plucked string with a pad behind it: FluidR3, TimGM6mb and Jnsgm2 all layer a nylon guitar
// over synth strings. Here the pad is the plucked string itself, kept singing by feedback. Each
// key plucks a course of three steel strings, a few cents apart, with a soft leather plectrum
// (the harpsichord's "peau de buffle": a round, dark pluck). Over each string sits an
// electromagnet, under it a pickup: the electromagnetically sustained string of the EBow
// (Heet 1978) and of McPherson's Magnetic Resonator Piano (2010).
//
// The loop: the pickup senses the string's velocity at its place; an amplifier, tuned to the
// key by a band-pass (and, by the "harmonic" knob, to its octave), drives the magnet; the
// amplifier saturates (tanh), and the magnet pulls the string at its own place. In phase with
// the velocity the pull is a negative damping: the modes the filter passes, and the places
// favour, grow until the saturated drive just pays for their losses (a limit cycle). The pluck's
// upper partials die away as a plucked string's do; the fundamental (and the octave, by the
// knob) holds where the touch set the amplifier's limit: a pluck turning into a held, pure,
// chorused tone (each string has its own loop and its own pitch: they beat). The magnet pulls,
// never pushes: its force goes as the square of its flux, and its permanent bias makes that the
// current plus a little of its square (the octave); the saturated current is square-ish (the
// odd partials, which the near-harmonic modes take up).
//
// Released, the magnets let go and a felt damper rests on the course.
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool fxAtmosphere(const Instrument& inst);  // -inst fx-atmosphere

class AtmosphereModel : public Tube {
 public:
  explicit AtmosphereModel(const Instrument& inst);
  void keyOn(int midi, int velocity);
  void keyOff(int midi);
  void setBend(double semitones);
  // The knobs, 0..1 (AtmospherePlayer::setKnob).
  void setSustain(double x) { sustain_ = x; }    // K1: the amplifiers' limit (0 none .. 1.2x the pluck's fundamental; 0.5: 0.6x)
  void setHarmonic(double x) { harmonic_ = x; }  // K2: the octave channel (0 .. 1/4 of the note; 0.5: 1/8)
  void setPluck(double x) { pluck_ = x; }        // K3: the plectrum's hardness (its slip x 3 .. 1/3; new notes)
  void setChorus(double x) { chorus_ = x; }      // K4: the course's spread (0 .. 2x; new notes)
  bool asleep() const { return notes_.empty(); }
  double testResponse() const override { return last_; }
  void resetAir() override;
  const float* takeSide(int n) override;

  static constexpr int kModes = 28, kStrings = 3;
  struct Mode {
    double re = 0, im = 0;  // y - i y'/w
    double c = 1, s = 0;    // the rotation a sample (bent)
    double w = 0;           // angular frequency (rad/s, bent)
    double f = 0;           // unbent frequency (Hz)
    double sigma = 0;       // the string's own decay rate (1/s)
    double r = 1;           // the decay a sample (felt included)
    double pl = 0, pu = 0, dr = 0;  // the mode's shape at the plectrum, the pickup, the magnet
  };
  struct Biquad {
    double b0 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    double run(double x) {
      const double y = b0 * x + z1;
      z1 = -a1 * y + z2;
      z2 = b2 * x - a2 * y;
      return y;
    }
  };
  struct String {
    double f = 0;           // first mode (Hz, unbent)
    double pan = 0;
    int modes = 0;
    Mode mode[kModes];
    Biquad bp1, bp2;        // the amplifier's tuning: the note, its octave
    double g1 = 0, g2 = 0;  // the amplifier's gains through them
    double lim1 = 0, lim2 = 0;  // their limits (the touch), per unit of K1's sustain, of K2's octave
    double delay = 0, share = 1;  // when the plectrum meets it, how hard
    double magnet = 1;      // its magnet's strength (the side strings' sit further off)
    double coil = 0;        // the magnet's current (lags the amplifier: its inductance)
    double drive = 0;       // the force on the next sample
  };
  struct Note {
    int midi = 0;
    bool down = true;
    double voice = 1;       // the key's level (the voicing)
    double t = 0;           // seconds since the key
    double push = 0, slip = 0;  // the plectrum's push and slip times (s)
    double fpk = 0;         // its peak force (per unit modal mass)
    double amp = 0;         // the amplifiers' on-ramp (0 .. 1)
    double bass = 1;        // the magnets' setting down the keyboard
    String str[kStrings];
    int quiet = 0;
  };

 private:
  void controlStep() override {}
  float sample() override;
  void tune(String& s, double f0);
  void damp(Note& n);
  std::vector<Note> notes_;
  std::vector<float> side_, sideOut_;
  double bend_ = 0, bk_ = 1;
  double sustain_ = 0.5, harmonic_ = 0.5, pluck_ = 0.5, chorus_ = 0.5;
  double dcIn_ = 0, dcOut_ = 0, dcSideIn_ = 0, dcSideOut_ = 0;
  uint32_t rng_ = 9127;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (1.0 / 16777216.0); }
  float last_ = 0;
};

// Velocity is the pluck and the amplifiers' limit; the sustain pedal holds the keys (and their
// magnets); the pitch bend is +-2 semitones. Knobs (the controller's K1-K4, -seq's "knob K X"): K1 the
// sustain (the magnets' strength: 0 a plain psaltery), K2 the harmonic (the octave held too), K3
// the plectrum's hardness, K4 the chorus (the course's spread).
class AtmospherePlayer : public Performer {
 public:
  explicit AtmospherePlayer(AtmosphereModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}
  bool setKnob(int k, double x) override;

 private:
  AtmosphereModel& m_;
  bool keyDown_[128] = {}, sounding_[128] = {};
};

}  // namespace pajupilli
