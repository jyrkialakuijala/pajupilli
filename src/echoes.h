// General MIDI 103, "FX 7 (echoes)" (-inst fx-echoes): a tuning-fork keyboard played into a
// tape echo (research/fx_sounds.md).
//
// The classic patch (Roland's "Echo Drops", Yamaha's "Echoes") is not a delay effect: in the
// three SoundFonts it is a choir "aah" and a near-sine synth voice whose looped samples swell and
// dip every 0.5 .. 1 s (the loops' own period, following the key), panned apart in FluidR3, with a
// 5 s release in FluidR3; Roland's is described as two chorus-like sweeping tones that burst into
// "drops" at the key-up. Here the echoes are real, and made by the machine that made echoes in
// the 1970s: a tape loop running past an erase head, a record head and two playback heads
// (Roland's RE-201 Space Echo and the Echoplex are this machine).
//
// The source is a dulcitone (Machell, 1880s): a tuning fork a key, struck by a felt hammer, its
// stem on a resonance box tuned to it, picked up by a microphone. A fork is the nearest a struck
// thing comes to a sine (the classic's near-sine voice), with a short metallic "clang" (its second
// bending mode, 6.27 x the note) that a harder blow (a shorter contact on the felt, Hertz's law)
// brings out: the "drop". A loud fork also sounds its octave (its stem's motion is not quite
// linear in the prongs').
//
// The tape echo (one machine for all the keys, so chords and their echoes mix on the tape):
// - The record head writes the magnetisation the AC bias gives: the anhysteretic curve (a Langevin
//   function), linear when quiet, saturating softly when loud or when the echoes pile up.
// - Each playback head reads the tape a head spacing later: the delay is spacing / speed (6 and 12
//   cm; ~0.34 and 0.67 s at the default speed). The head sees the tape through its spacing d: the
//   field it reads is the recorded one smoothed along the tape by a Poisson kernel of width d,
//   exp(-2 pi d f / v) in frequency (Wallace's spacing loss): every pass is darker, and a slower
//   tape (longer echoes) is darker still.
// - The second head's output goes back to the record head (the "intensity"), so the repeats come
//   every head-1 interval: head 1 (to the left) gives the odd ones, head 2 (to the right) the
//   even ones, each pass a little quieter, darker and (when loud) more saturated. Above unity
//   loop gain the machine runs away into its own oscillation, held by the tape's saturation.
// - The tape's speed wobbles: the capstan's and the pinch roller's eccentricity (flutter, a few
//   Hz), the loop's splice and tension (wow, a fraction of a Hz), and random scrape flutter. The
//   echoes come back slightly off pitch, and a held fork beating against its own echo sweeps
//   like a chorus (the classic's "sweeping tones").
// - The motor has inertia: turning the rate knob glides the speed, and the echoes already on the
//   tape slide in pitch.
// Velocity is the hammer's speed; the key-up lands the damper on the fork, but the tape keeps
// repeating it. Knobs: K1 the tape's speed (the echo interval, 0.15 .. 0.75 s), K2 the intensity
// (the feedback; near its top, ~0.9, it runs away), K3 the echo's volume, K4 the tape's wear
// (head spacing, wow and flutter).
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool fxEchoes(const Instrument& inst);  // -inst fx-echoes

class EchoesModel : public Tube {
 public:
  explicit EchoesModel(const Instrument& inst);
  void keyOn(int midi, int velocity);
  void keyOff(int midi);
  void setBend(double semitones);
  // The knobs, 0..1 (EchoesPlayer::setKnob).
  void setRate(double x) { rate_ = x; }            // K1: the tape's speed (the echo interval)
  void setIntensity(double x) { intensity_ = x; }  // K2: the feedback (the repeats)
  void setVolume(double x) { volume_ = x; }        // K3: the echo's volume
  void setWear(double x) { wear_ = x; }            // K4: the tape's wear (spacing, wow, flutter)
  bool asleep() const { return notes_.empty() && idle_ > idleLimit_; }
  double testResponse() const override { return last_; }
  void resetAir() override;
  const float* takeSide(int n) override;

  static constexpr int kModes = 3;
  struct Note {
    int midi = 0;
    bool down = true;
    double f0 = 0;          // the fork's note (Hz)
    double t = 0;           // seconds since the blow
    double damper = 0;      // the damper's contact (0 off .. 1 down)
    int modes = 0;
    double w[kModes];       // the modes' angular frequencies (bent)
    double sFree[kModes];   // their own damping (1/s)
    double sDamp[kModes];   // the damper's
    float re[kModes], im[kModes];  // the modes (re: velocity / omega, im: displacement at the tip)
    float cr[kModes], ci[kModes];  // their rotation and decay a sample
    // The hammer (felt, Hertz's law): its place, speed, mass, stiffness (linear at the reference
    // compression), the reference compression; in contact or gone.
    double yh = 0, vh = 0, mh = 0.25, kh = 0, dref = 1;
    bool hammer = true;
    // The resonance box: a band-pass on the stem's velocity (biquad state and coefficients).
    double bx1 = 0, bx2 = 0, by1 = 0, by2 = 0, b0 = 0, a1 = 0, a2 = 0;
    double boxDetune = 0;
    double pan = 0;
    int quiet = 0;
  };

 private:
  void controlStep() override {}
  float sample() override;
  void tune(Note& n);
  void decay(Note& n);
  double readHead(int k, double target, double gamma);
  std::vector<Note> notes_;
  std::vector<float> side_, sideOut_;
  // The tape: what the record head wrote each sample, and where on the tape it wrote it (m).
  std::vector<float> tapeM_;
  std::vector<double> tapePos_;
  int64_t n_ = 0;        // samples recorded
  double pos_ = 0;       // the tape's travel past the record head (m)
  double speed_ = 0.18;  // the motor's speed (m/s)
  double rd_[2] = {0, 0};  // each playback head's last read index (samples, absolute)
  double hp_[2][2] = {};   // each head's coupling high-pass (x, y)
  double head_[2] = {};    // each head's last output
  double phCap_ = 0, phPinch_ = 0, phLoop_ = 0, scrape_ = 0, scrape2_ = 0;
  double bend_ = 0, bk_ = 1;
  double rate_ = 0.5, intensity_ = 0.5, volume_ = 0.5, wear_ = 0.5;
  int64_t idle_ = 0, idleLimit_ = 0;
  uint32_t rng_ = 4721;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (1.0 / 16777216.0); }
  float last_ = 0;
};

// Velocity is the hammer's speed; the sustain pedal holds the dampers off; the pitch bend is
// +-2 semitones (the forks; the tape's echoes keep what was recorded). Knobs (the MPK's K1-K4,
// -seq's "knob K X"): K1 the tape's speed (the echo interval), K2 the intensity (the repeats), K3
// the echo's volume, K4 the tape's wear.
class EchoesPlayer : public Performer {
 public:
  explicit EchoesPlayer(EchoesModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}
  bool setKnob(int k, double x) override;

 private:
  EchoesModel& m_;
  bool keyDown_[128] = {}, sounding_[128] = {};
};

}  // namespace pajupilli
