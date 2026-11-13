// The reed player: fingerings, breath, the lips on the reed and the tongue.
//   - Breath: a pressure in the mouth set by the note (higher notes need more)
//     and the dynamics; the calibration trims it per note.
//   - Lips: the force on the reed (the embouchure) tunes the note: more force
//     closes the reed and raises the pitch. Calibrated per note at soft,
//     mezzo-forte and loud; the ear corrects it while a note is held. The lip
//     roll (pitch bend) relaxes or tightens it.
//   - Tongue: "tu" lifts the tongue off the reed (it was resting on it), "du"
//     is a shorter touch; slurs keep the reed going; a soft key from silence is
//     a breath attack (the reed starts on the breath alone).
//   - Vibrato: from the breath (oboe, bassoon, duduk, shawm) or the jaw
//     (saxophones: the lip force pulses); clarinets play it straight unless
//     the knob asks.
#pragma once

#include <cstdint>

#include "performer.h"
#include "reed.h"

namespace pajupilli {

class ReedPlayer : public Performer {
 public:
  explicit ReedPlayer(Reed& r);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void setHold(bool h) override;
  void render(float* out, int n) override;
  void seed(unsigned s) override;

 private:
  enum Attack { kSlur, kBreath, kDu, kTu, kAccent };
  void applyNote(Attack a);

  Reed& reed_;
  int held_[128] = {};
  int stackTop_ = 0, stack_[16];
  int keyVel_ = 64;
  double lung_ = 0, lungAttack_ = 0.008, sinceOff_ = 1e9, sinceOn_ = 0;
  // A bagpipe: the bag's steady pressure, no lips, no tongue; notes split by grace notes.
  bool bag_ = false;
  double lungHeld_ = 0;  // the breath while the note was blown (for the release)
  double graceLeft_ = 0;
  double tongueClock_ = 1e9, tongueClosed_ = 0.004, tongueRelease_ = 0.004;
  double velAccent_ = 0, velDyn_ = 0, velDynTarget_ = 0;
  double vibPhase_ = 0, vibRateMul_ = 1, vibDepthMul_ = 1;
  double flutterPhase_ = 0, flutterRate_ = 27;
  double kEarStart_ = 0, kEarRange_ = 120, kDynP_, kDynLip_, kVelDyn_, kRollLip_, kP_;
  uint32_t rng_ = 4242;
  double w1a_ = 0, w1b_ = 0, w2a_ = 0, w2b_ = 0;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; }
};

}  // namespace pajupilli
