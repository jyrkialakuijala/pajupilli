// General MIDI 97, "FX 1 (rain)" (-inst fx-rain): rain falling on tuned glass bowls
// (research/fx_sounds.md).
//
// Each key sets out a cluster of thin hemispherical glass bowls, partly filled with water, under
// the rain: the note's bowl and smaller ones at 2x, 3x and 4x (K4: one to five, 1x .. 5x), each
// tuned so that its lowest bending mode (n = 2) is on its pitch. A bowl rings in its modes
// n = 2 .. 5 (frequencies n (n^2 - 1) / sqrt(n^2 + 1): 1, 2.83, 5.42, 8.77 x; its wall thinned,
// as a handbell's, to put n = 3 on the twelfth, 3 x), each a doublet split a little by the
// glass's flaws (cos n theta and sin n theta: they beat slowly).
//
// The rain: drops arrive at random (Poisson), their diameters drawn from Marshall and Palmer's
// distribution, N(D) ~ exp(-Lambda D), Lambda = 4.1 R^-0.21 mm^-1, weighted by the flux (each
// size falling at its terminal speed, v(D) = 9.65 - 10.3 exp(-0.6 D) m/s, Atlas et al.). The
// key's velocity is the rain rate R (0.3 .. 90 mm/h): a soft key a drizzle of small drops, a
// hard one a downpour with large ones. The rate wanders with gusts (shared by every key).
//   - A drop on the glass: a force pulse (sin^2, its length the drop's diameter over its speed,
//     ~0.25 .. 0.4 ms) carrying its momentum (D^3 v). Each mode takes it by the pulse's spectrum
//     at the mode's frequency, the impact's angle (cos / sin n theta: the doublet) and height on
//     the wall. The impact itself radiates a short tick (the force's derivative).
//   - A drop into the water: a weaker push through the water and, by its size (the regimes of
//     Pumphrey and Elmore, Medwin and Nystuen): 0.8 .. 1.2 mm drops entrain a bubble every time
//     (Minnaert: f = 3.26 / R, 13 .. 21 kHz), drops over 2 mm often a larger one (1 .. 10 kHz).
//     A bubble rings damped and chirping upward (van den Doel 2005: d = 0.043 f + 0.0014 f^1.5,
//     f(t) = f0 (1 + 0.1 d t)).
// A large drop is a plink on its bowl; the many small ones keep the bowls ringing between them,
// a soft bed of the cluster's chord. At the key a single large drop (a drip from the eaves,
// sized by the touch) strikes the note's bowl. Released keys: the rain on them stops and a hand
// damps the bowls (T60 at most ~2 s).
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool fxRain(const Instrument& inst);  // -inst fx-rain

class RainModel : public Tube {
 public:
  explicit RainModel(const Instrument& inst);
  void keyOn(int midi, int velocity);
  void keyOff(int midi);
  void setBend(double semitones);
  // The knobs, 0..1 (RainPlayer::setKnob).
  void setDensity(double x) { density_ = x; }  // K1: drops a second per key (25 .. 800; 0.5: 140)
  void setWater(double x) { water_ = x; }      // K2: the bubbles' and ticks' share (0 .. 2x)
  void setRing(double x) { ring_ = x; }        // K3: the bowls' ring (x 0.25 .. 4)
  void setBowls(double x) { bowls_ = x; }      // K4: bowls per key (1 .. 5; 0.75: 4)
  bool asleep() const { return notes_.empty() && bubbles_.empty() && ticks_.empty(); }
  double testResponse() const override { return last_; }
  void resetAir() override;
  const float* takeSide(int n) override;

  struct Mode {
    double re = 0, im = 0;  // the complex state (out: im)
    double c = 1, s = 0;    // the rotation at the note (bent)
    double f = 0;           // unbent frequency (Hz)
    double rHeld = 1, rRel = 1;  // decay a sample, held and released
    int n = 2;              // the bowl's mode number
    bool sine = false;      // the doublet's sin n theta member
  };
  struct Bowl {
    double f = 0;                // its first mode (Hz, unbent)
    double rate = 0;             // drops a second at the gust's mean
    double next = 0;             // samples to the next drop
    double pan = 0;              // -1 .. 1
    double gain = 1;
    std::vector<Mode> modes;
  };
  struct Note {
    int midi = 0;
    bool down = true;
    double R = 5;           // rain rate (mm/h)
    double lambda = 3;      // Marshall-Palmer's slope (1/mm)
    double dmax = 3;        // the largest drops (mm)
    std::vector<Bowl> bowls;
    int quiet = 0;
  };
  struct Bubble { double re, im, c, s, decay, chirp, amp, pan; };
  struct Tick { int pos, len; double amp, pan; };

 private:
  void controlStep() override {}
  float sample() override;
  void drop(Note& n, Bowl& b, double D, bool glass, double height = -1);  // (height on the wall, 0..1; < 0: at random)
  void tune(Mode& m, double bk) const;
  double drawDiameter(double lambda, double dmax);
  std::vector<Note> notes_;
  std::vector<Bubble> bubbles_;
  std::vector<Tick> ticks_;
  std::vector<float> side_, sideOut_;
  double bend_ = 0, bk_ = 1;
  double density_ = 0.5, water_ = 0.5, ring_ = 0.5, bowls_ = 0.75;
  double gust_ = 0, gustF_ = 1;  // the wind: an Ornstein-Uhlenbeck walk, its factor on the rate
  int gustCount_ = 0;
  uint32_t rng_ = 977;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (1.0 / 16777216.0); }
  float last_ = 0;
};

// Velocity is the rain's rate (and the first drop's size); the sustain pedal holds the keys; the
// pitch bend is +-2 semitones. Knobs (the controller's K1-K4, -seq's "knob K X"): K1 the drops' density,
// K2 the water (bubbles and ticks), K3 the bowls' ring, K4 bowls per key.
class RainPlayer : public Performer {
 public:
  explicit RainPlayer(RainModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}
  bool setKnob(int k, double x) override;

 private:
  RainModel& m_;
  bool keyDown_[128] = {}, sounding_[128] = {};
};

}  // namespace pajupilli
