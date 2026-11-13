// The flutist: turns notes into fingerings, breath, tongue and lip settings,
// with the timing of tonguing, slurs and releases.
#pragma once

#include "flute.h"
#include "performer.h"

#include <cstdint>

namespace pajupilli {

class Player : public Performer {
 public:
  explicit Player(Flute& f);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void setHold(bool h) override;
  void render(float* out, int n) override;
  void seed(unsigned s) override;

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
  bool heldTold_ = false;
  // A trill starts only when its two notes alternate fast: the last key event
  // (press or release) was one of them, under kTrillFast_ seconds before (not a
  // slow B C D E, nor a fast scale).
  double keyClock_ = 1e9, kTrillFast_;
  int lastKey_ = -1;
  Flute& flute_;
  int held_[128] = {};
  int stackTop_ = 0, stack_[16];
  double kFace_ = 1;
  double kW_, kY0_, kY0Exp_, kP_, kVibThroat_, breath_;
  double glottisClock_ = 1e9;
  double vibPhase_ = 0;
  double lung_ = 0;
  double tongueClock_ = 1e9, tongueRelease_ = 0.005, tongueClosed_ = 0.004;
  double lungAttack_ = 0.005, sinceOff_ = 1e9;
  double flutterPhase_ = 0;
  double vibRateMul_ = 1, vibDepthMul_ = 1, flutterRate_ = 27;  // this player's own
  double sinceOn_ = 0;
  double velAccent_ = 0;
  int keyVel_ = 64;
  double regKick_ = 0, kKickP_, kKickW_, kKickT_, kKickDyn_, kSlurKick_ = 1.9, kVentKickDyn_ = 0,
         kVentKickT_ = 0.04, kickT_ = 0.06;
  double slurKick(int to);  // the register kick for a slur up to `to` (sets its duration)
  double kDynH_, kDynP_, kVelDyn_;
  double kAttackKick_;
  // Slow random wander of breath support, jet aim and lip opening (~0.3-4 Hz).
  uint32_t rng_ = 4242;
  double w1a_ = 0, w1b_ = 0, w2a_ = 0, w2b_ = 0, w3a_ = 0, w3b_ = 0;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; }
  double velDyn_ = 0, velDynTarget_ = 0;  // dynamics offset from key velocity
};

}  // namespace pajupilli
