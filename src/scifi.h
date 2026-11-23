// General MIDI 104, "FX 8 (sci-fi)" (-inst fx-sci-fi): two voices on every key.
//
// The discharge: a sine whose pitch falls from up to three octaves above the note onto it, pushed
// into a sine wavefolder (sin(z sin(phi) + bias), the bias giving the even harmonics). The drive z
// decays: a hard key folds the sine many times over (a dense, buzzing spectrum), and as z falls
// the folded harmonics collapse inward onto the note until it is a plain sine, dying away (an
// energy weapon's shot, a shield's impact). The folder is band-limited by construction: its
// output's spectrum is that of FM (Bessel J_n(z) at n x the sine's frequency), and z is held
// under what the instantaneous frequency leaves below 20 kHz.
//
// The ping: a resonant state-variable band-pass at the note, its damping at the edge of
// self-oscillation (it rings for seconds while the key is held), struck by a 1 ms Hann impulse
// (shorter high up, so the pulse's spectrum has no null at the note; scaled so every key rings
// alike), its cutoff frequency-modulated at audio rate by a sine at `ratio` x the note. A low
// ratio is a sonar's ping (a slow, slight wobble), a ratio near 1 a pitched telemetry beep, a
// little off an integer ratio crystalline and inharmonic. The ratio drifts a little over the
// keyboard (+-6 % per three octaves from the middle): the middle beeps, the ends glitter.
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool sciFi(const Instrument& inst);  // -inst fx-sci-fi

class SciFiModel : public Tube {
 public:
  explicit SciFiModel(const Instrument& inst);
  void keyOn(int midi, int velocity);
  void keyOff(int midi);
  void setBend(double semitones) { bend_ = semitones; }
  // The knobs, 0..1 (SciFiPlayer::setKnob).
  void setFold(double x) { fold_ = x; }      // K1: the folder's greatest drive (1.5 .. 30)
  void setRatio(double x) { ratio_ = x; }    // K2: the modulator's ratio (0.05 .. 4; 0.68: 1)
  void setBalance(double x) { balance_ = x; }  // K3: discharge .. ping (< 0: by the key)
  void setRing(double x) { ring_ = x; }      // K4: how long the ping rings held (0.25 .. 64 s)
  bool asleep() const { return notes_.empty(); }
  double testResponse() const override { return last_; }
  void resetAir() override { notes_.clear(); }

  struct Note {
    int midi = 0;
    bool down = true;
    double f0 = 0;             // the note's frequency (Hz)
    // The discharge.
    double phi = 0;            // the sine's phase (cycles)
    double drop = 0;           // octaves above the note at the start
    double pitchEnv = 1, pitchDecay = 1;  // the drop's envelope (x drop) and its step
    double z = 0, zDecay = 1;  // the folder's drive and its step
    double bias = 0, dc = 0;   // the folder's bias; the DC it makes (sin(bias) J0(z))
    double foldAmp = 0;
    double rel = 1, relDecay = 1;  // after the key: the discharge's fade
    // The ping.
    double ic1 = 0, ic2 = 0;   // the state-variable filter's states
    double psi = 0;            // the modulator's phase (cycles)
    double ratio = 1, dev = 0; // modulator ratio, cutoff deviation (fraction of the note)
    double kHeld = 0, kRel = 0, k = 0;  // damping (1 / Q) held, released, now
    double pingAmp = 0;        // the impulse's scale
    int pulseLen = 0, pulsePos = 0;
    double mix = 0.5;          // the ping's share
    int quiet = 0, age = 0;
  };

 private:
  void controlStep() override {}
  float sample() override;
  std::vector<Note> notes_;
  double bend_ = 0;
  double fold_ = 0.5, ratio_ = 0.684, balance_ = -1, ring_ = 0.4;
  uint32_t rng_ = 12345;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (1.0 / 16777216.0); }
  float last_ = 0;
};

// Velocity is the fold's drive and the ping's strength (and its FM index); the sustain pedal
// holds the keys; the pitch bend is +-2 semitones. Knobs (the controller's K1-K4, -seq's "knob K X"):
// K1 the fold, K2 the modulator's ratio, K3 the balance, K4 the ping's ring.
class SciFiPlayer : public Performer {
 public:
  explicit SciFiPlayer(SciFiModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}
  bool setKnob(int k, double x) override;

 private:
  SciFiModel& m_;
  bool keyDown_[128] = {}, sounding_[128] = {};
};

}  // namespace pajupilli
