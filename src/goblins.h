// General MIDI 102, "FX 6 (goblins)" (-inst fx-goblins): a pyrophone, a keyboard of singing
// flames (research/fx_sounds.md).
//
// The classic patch is a dark synth voice that swells in over most of a second, then opens
// slowly over ~9 s (a low-pass, resonant in TimGM6mb and Jnsgm2, swept up through the partials
// by a slow modulation envelope), with a 5 .. 9 Hz warble in its upper partials (a sitar layer's
// six-semitone vibrato in TimGM6mb and Jnsgm2; a growling vox layer in FluidR3) and wind. Here it
// is Frédéric Kastner's pyrophone (1873): a glass pipe a key, open at both ends, with a gas flame
// a quarter of the way up. The flame's heat release follows the air's motion past it with a lag
// (the time the flow takes to carry a disturbance through the flame); when the lag is near a
// quarter of the pipe's period the heat swells as the pressure does (Rayleigh's criterion) and
// the pipe sings, thermoacoustically. The gas lighting (a soft thump) and the flame's roar set
// the pipe ringing, and the oscillation grows out of them, so it swells in; it saturates where the
// flow past the flame reverses (Heckl's nonlinear law for the Rijke tube: the heat transfer goes as
// the root of the speed, |u0/3 + u|^1/2); its timbre is a pipe's, the odd partials strong.
//
// The slow evolution is the pipe warming. The flame heats the gas over seconds (the thermal time
// constant, K1: ~6 s by default) and:
// - the draft through the pipe grows, so the flame's lag shrinks to the quarter period (from 1.25
//   x it): the pitch rises a few cents into tune (the bass the most), and the partials the flame
//   reinforces move up (each partial's phase through the lag is n times the note's: a comb moving
//   up the spectrum);
// - the flame shortens in the faster flow: its response (a low-pass in the Strouhal number) opens
//   from the note to several times it (2.5 .. 7 x, by the flame's size), so the heat release, and
//   the pipe's upper partials with it, brighten (the classic's slow filter sweep: ~1.2 x f0 for
//   the centroid at first, ~1.7 x after 6 s);
// - the walls stop quenching the flame's edges: its gain grows, the limit cycle moves away from
//   its threshold (from a near sine to a saturated tone), and the flow, and with it the level of
//   the saturation, grows (~6 dB from 1 s to 8 s).
// The bubbling is the flame's own puffing: a buoyant flame flickers at a few Hz (~1.5 / sqrt(D) Hz,
// Cetegen and Ahmed), here a weakly self-excited oscillator kicked by the turbulence (5 .. 9 Hz
// over the keyboard, K2 its depth); it swings the heat and the flame's length, and so the lag:
// the upper partials, whose phase through the lag is the most sensitive, warble the most.
//
// Each pipe is a set of modes (the open pipe's harmonics, their losses the walls' and the
// radiation's), each driven by the flame's heat release at its place; the flame sees the air's
// velocity there, delayed by its lag; the sound is the pipe's top end radiating. Velocity is the
// gas valve (the flame's size: its gain, its flow, its response's corner, its roar); the key-up
// closes the valve (K4) and the pipe falls silent once the flame's gain is below its losses. K3
// is the flame's roar (the combustion noise, low-passed at ~700 Hz, through the pipe: a breathy
// wind around the partials).
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool fxGoblins(const Instrument& inst);  // -inst fx-goblins

class PyrophoneModel : public Tube {
 public:
  explicit PyrophoneModel(const Instrument& inst);
  void keyOn(int midi, int velocity);
  void keyOff(int midi);
  void setBend(double semitones);
  // The knobs, 0..1 (PyrophonePlayer::setKnob).
  void setWarmth(double x) { warmth_ = x; }  // K1: the pipe's warming (2 .. 16 s; the swell's length)
  void setPuff(double x) { puff_ = x; }      // K2: the flame's puffing (the bubbling's depth)
  void setRoar(double x) { roar_ = x; }      // K3: the flame's roar (the combustion noise)
  void setValve(double x) { valve_ = x; }    // K4: the valve's closing at the release (0.1 .. 3 s)
  bool asleep() const { return notes_.empty(); }
  double testResponse() const override { return last_; }
  void resetAir() override;
  const float* takeSide(int n) override;

  static constexpr int kModes = 32, kDelay = 4096;
  struct Note {
    int midi = 0;
    bool down = true;
    double f0 = 0;        // the note (Hz)
    double size = 1;      // the flame's size (the valve's full opening, by velocity)
    double valve = 0;     // the valve's opening now (0 .. 1)
    double t = 0;         // seconds since the key
    double open = 0.15;   // the valve's opening time (s)
    double warm = 0;      // the pipe's warmth (0 cold .. 1 hot, at this flame)
    double px = 0.5, pv = 0, pw = 40;  // the puffing (its state, its angular frequency)
    double nz = 0;        // the roar (low-passed noise)
    double flame = 0;     // the flame's heat release (its response low-passed)
    double cornerHot = 5; // the flame's response's corner, hot (x the note)
    double qPrev = 0;     // the heat release a sample ago
    double sigma1 = 1;    // the fundamental's damping (1/s)
    double gainCold = 1.2, gainHot = 3;  // the flame's gain over the losses, cold and hot
    double pan = 0;
    int modes = 0, w = 0, quiet = 0;
    float re[kModes], im[kModes];      // the modes (complex amplitudes)
    float cr[kModes], ci[kModes];      // their rotation and decay a sample
    float drive[kModes];               // the flame's push on each (its place's share / omega)
    float look[kModes];                // the velocity each gives at the flame
    float rad[kModes];                 // its radiation from the top end
    float hist[kDelay];                // the velocity at the flame (the lag's line)
  };

 private:
  void controlStep() override {}
  float sample() override;
  void tune(Note& n);
  std::vector<Note> notes_;
  std::vector<float> side_, sideOut_;
  double bend_ = 0, bk_ = 1;
  double warmth_ = 0.5, puff_ = 0.5, roar_ = 0.5, valve_ = 0.5;
  uint32_t rng_ = 9127;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (1.0 / 16777216.0); }
  float last_ = 0;
};

// Velocity is the flame's size; the sustain pedal holds the keys; the pitch bend is +-2
// semitones. Knobs (the MPK's K1-K4, -seq's "knob K X"): K1 the warming (the swell's length), K2
// the puffing (the bubbling), K3 the roar, K4 the valve's closing (the release).
class PyrophonePlayer : public Performer {
 public:
  explicit PyrophonePlayer(PyrophoneModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}
  bool setKnob(int k, double x) override;

 private:
  PyrophoneModel& m_;
  bool keyDown_[128] = {}, sounding_[128] = {};
};

}  // namespace pajupilli
