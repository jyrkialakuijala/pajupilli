// The horn player: valves, breath, the lips' tension and the tongue.
//   - Valves: the note's fingering (written pitch: valves and partial).
//   - Lips: the tension sets their resonance a little below the note (the
//     note sounds above it, the bore's resonance pulls it in). Calibrated per
//     note at soft, mezzo-forte and loud; the ear corrects it while a note is
//     held; the lip bend (pitch bend) moves it.
//   - Breath: a mouth pressure that grows with the note (~ 12 Pa per Hz at
//     mezzo-forte, Fletcher & Tarnopolsky's threshold x2) and the dynamics.
//   - Tongue: "ta" holds the air behind the teeth while the mouth fills, then
//     lets it go; "da" is a lighter touch; a slur changes the lips (and the
//     valves) on the same breath; a soft key from silence is a breath attack.
#pragma once

#include <cstdint>

#include "horn.h"
#include "performer.h"

namespace pajupilli {

class HornPlayer : public Performer {
 public:
  explicit HornPlayer(Horn& h);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void setHold(bool h) override;
  void render(float* out, int n) override;
  void seed(unsigned s) override;
  // One of a brass section (src/brasssect.cc): its own pitch a few cents off (the lips aim
  // there: the players beat as a section's do), its own attack.
  void vary(unsigned seat) override;
  double seatCents() const { return seatCents_; }

 private:
  enum Attack { kSlur, kBreath, kDa, kTa, kAccent };
  void applyNote(Attack a);
  void stopBlowing();

  Horn& horn_;
  int held_[128] = {};
  int stackTop_ = 0, stack_[16];
  int keyVel_ = 64;
  double lung_ = 0, lungAttack_ = 0.008, sinceOff_ = 1e9, sinceOn_ = 0;
  double tongueClock_ = 1e9, tongueClosed_ = 0.004, tongueRelease_ = 0.004;
  double velAccent_ = 0, velDyn_ = 0, velDynTarget_ = 0;
  Vibrato vib_;  // its own vibrato (performer.h)
  double flutterPhase_ = 0, flutterRate_ = 27;
  double kDynP_, kDynH_, kDynLip_, kVelDyn_, kRollLip_, kP_, kFl_, kSlurLip_, kSlurAir_, kSlurDip_, kSlurDipT_, kSlurTongue_, kEarTau_, kAtkP_, kAtkT_, kSlurDipUp_, kLegatoGap_, kSoftAir_, kSoftLips_, kSlurTongueMax_, kSlurTongueFloor_, kSlurAirDown_;
  double tongueFloor_ = 0;
  double pendingOff_ = 0;  // a key let go: the air stops when this runs out
  double slurDip_ = 0;
  // The bell turned with the dynamics (hornplayer.cc): the sound above its beaming, dB now.
  bool kBellTurn_ = false;
  double kBellSoft_ = -12, kBellLoud_ = 12, bellK_ = 0.05, bellDb_ = 0, bellVel_ = 0;
  float bellLp_ = 0;
  double atkPush_ = 0;
  // A slur to another partial: the lips overshoot towards it and the air
  // pushes, briefly (log of the lips' factor; the breath's extra share).
  double slurLip_ = 0, slurAir_ = 0, sinceSlur_ = 1e9;
  double slurLipT_ = 0.03, slurAirT_ = 0.03, slurDipT_ = 0.08;  // (their times: src/slurs.h)
  // A practised slur's lead: the valves go when this runs out (the lips went at the key).
  double leadLeft_ = 0, leadSpeed_ = 1;
  const char* leadHoles_ = nullptr;
  int prevNote_ = -1;
  int prevSlideNote_ = -1;  // (the last note, for its slide position)
  double kSlideTongue_;
  int prevPartial_ = -1;
  uint32_t rng_ = 4242;
  bool section_ = false;   // (vary: a section's player)
  double seatCents_ = 0;   // its own offset from the note (c)
  double w1a_ = 0, w1b_ = 0, w2a_ = 0, w2b_ = 0;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; }
};

}  // namespace pajupilli
