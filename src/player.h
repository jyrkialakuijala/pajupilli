// The flutist: turns notes into fingerings, breath, tongue and lip settings,
// with the timing of tonguing, slurs and releases.
#pragma once

#include "calibrate.h"
#include "flute.h"

#include <cstdint>

namespace pajupilli {

struct NoteInfo {
  const char* holes;  // hole 1 .. 16 (17: the B foot's C key, open if left out), 'o' open
  int harmonic;       // which resonance of the fingering sounds (1, 2, 3, 4)
};

// Written MIDI 59 (B3, B foot) .. 96 (C7); nullptr outside.
const NoteInfo* noteInfo(int midi);
// A sounding note on an instrument: its fingering, or nullptr outside its range.
const NoteInfo* noteInfo(const Instrument& inst, int midi);
double noteFrequency(int midi, double a4 = 440.0);

class Player {
 public:
  explicit Player(Flute& f);
  void noteOn(int midi, int velocity);
  void noteOff(int midi);
  void setDynamics(double d) { dyn_ = d; }      // 0 pp .. 1 ff (breath, lip opening)
  void setBreath(double b) { breathCtl_ = b; }   // added to dynamics (joystick, pad pressure)
  void setVibrato(double depth) { vibDepth_ = depth; }  // fraction of lung pressure
  void setVibratoRate(double hz) { vibRate_ = hz; }
  void setLipRoll(double r) { roll_ = r; }       // -1 .. 1: covers more / less (pitch)
  void setNoise(double n) { noiseCtl_ = n; }     // breath noise multiplier
  void setAir(double a) { air_ = a; }            // 0 focused .. 1 airy (lip opening, turbulence, hiss)
  void setWander(double w) { wander_ = w; }      // 0 machine-steady .. 1 natural .. 3 unsteady
  void setBrightness(double b) { brightness_ = b; }  // jet offset multiplier
  void setHold(bool h);                          // keep blowing after the keys are up
  void setFlutter(double depth) { flutter_ = depth; }  // 0 .. 1
  void setHardness(double h) { hardness_ = h; }  // attack: 0.5 soft .. 2 hard
  void setRelease(double s) { release_ = s; }    // breath release time (s)
  // Per-note corrections from calibration: uncovered width scale, pressure scale.
  double coverTrim[128], coverLoTrim[128], coverHiTrim[128], dynLo[128], pressTrim[128], jetTrim[128];
  void setCalibration(const Calibration& c) {
    for (int i = 0; i < 128; ++i) {
      coverTrim[i] = c.cover[i];
      coverLoTrim[i] = c.coverLo[i];
      coverHiTrim[i] = c.coverHi[i];
      dynLo[i] = c.dynLo[i];
      pressTrim[i] = c.press[i];
      jetTrim[i] = c.jet[i];
    }
  }
  void render(float* out, int n);
  int sounding() const { return note_; }
  bool busy() const { return blowing_; }
  // Play this note `cents` off its tempered pitch, tuned by ear with the lips.
  void setTuning(double cents) { ear_ = true; earCents_ = cents; }
  // Its own breath wander, and its own vibrato and flutter: rate, phase and
  // depth differ from player to player, so an ensemble never pulses together.
  void seed(unsigned s);
  const NoteInfo* info(int midi) const { return noteInfo(inst_, midi); }

 private:
  enum Attack { kSlur, kBreath, kDu, kTu, kAccent };
  void applyNote(Attack a);
  // Trills on the trill keys: one note of a pair held, the other slurred over
  // it. The fingers stay on the lower note; the upper sounds through the trill
  // key(s). trill_: the trill in progress (or null), trillBase_: its sounding
  // lower note.
  struct Trill;
  static const Trill kTrills[9];
  const Trill* trill_ = nullptr;
  int trillBase_ = -1;
  const char* holesFor(int midi);
  char trillHoles_[kHoles + 1] = {};
  double trillCover_ = 1, kTrillCover_;  // lip coverage for the upper note
  // On a trill key the player listens and tunes with the lips (no fingering
  // change): a band-pass at the note, its zero crossings give the pitch, and
  // the coverage moves to bring it to the note (time constant kTrillTune_).
  // The correction is remembered per trill: the next time it starts closer.
  bool onTrillKey_ = false;
  double trillClock_ = 0, trillLip_ = 1, kTrillTune_, kTrillHeld_;
  double trillCorr_[9] = {};  // log coverage, per trill
  double bp1_ = 0, bp2_ = 0, lastCross_ = -1, prevBp_ = 0;
  bool heldTold_ = false;
  void listen(const float* x, int m, double& corr, double cents, double clock);
  // Tuning by ear to a target (an ensemble's just intonation): cents from the
  // note, reached with the lips like a trill note (earCorr_: log coverage).
  bool ear_ = false;
  double earCents_ = 0, earCorr_ = 0, earClock_ = 0;
  // A trill starts only when its two notes alternate fast: the last key event
  // (press or release) was one of them, under kTrillFast_ seconds before (not a
  // slow B C D E, nor a fast scale).
  double keyClock_ = 1e9, kTrillFast_;
  int lastKey_ = -1;
  Flute& flute_;
  const Instrument& inst_;
  double fs_;
  int note_ = -1, held_[128] = {};
  int stackTop_ = 0, stack_[16];
  double dyn_ = 0.5, breathCtl_ = 0, vibDepth_ = 0.0, vibRate_ = 5.0, roll_ = 0;
  double noiseCtl_ = 1, brightness_ = 1;
  bool blowing_ = false, hold_ = false;
  double kW_, kY0_, kY0Exp_, kP_, kVibThroat_, breath_;
  double glottisClock_ = 1e9;
  double vibPhase_ = 0;
  double lung_ = 0;
  double tongueClock_ = 1e9, tongueRelease_ = 0.005, tongueClosed_ = 0.004;
  double lungAttack_ = 0.005, release_ = 0.030, sinceOff_ = 1e9;
  double flutter_ = 0, flutterPhase_ = 0, hardness_ = 1;
  double vibRateMul_ = 1, vibDepthMul_ = 1, flutterRate_ = 27;  // this player's own
  double sinceOn_ = 0;
  double velAccent_ = 0;
  int keyVel_ = 64;
  double regKick_ = 0, kKickP_, kKickW_, kKickT_, kKickDyn_;
  double slurKick() const;
  double kDynH_, kDynP_, kVelDyn_;
  double air_, wander_, kAttackKick_;
  // Slow random wander of breath support, jet aim and lip opening (~0.3-4 Hz).
  uint32_t rng_ = 4242;
  double w1a_ = 0, w1b_ = 0, w2a_ = 0, w2b_ = 0, w3a_ = 0, w3b_ = 0;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; }
  double velDyn_ = 0, velDynTarget_ = 0;  // dynamics offset from key velocity
};

}  // namespace pajupilli
