// The flutist: turns notes into fingerings, breath, tongue and lip settings,
// with the timing of tonguing, slurs and releases.
#pragma once

#include "flute.h"
#include "performer.h"
#include "rng.h"
#include "slurs.h"

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
  void direct(const Cue& c) override;

 private:
  // (kGu, kKu: the back of the tongue, "ga", "ka": a fast run's second syllable; kLu: the tip
  // flicked, "la", a tongued legato barely breaking the air: a pan flute's, a bottle's)
  enum Attack { kSlur, kBreath, kDu, kTu, kAccent, kGu, kKu, kLu };
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
  double kW_, kY0_, kY0Exp_, kPipeLips_, kP_, kVibThroat_, breath_;
  // A pan flute's vibrato (direct): the director plans it, the player chooses its kind per note:
  // the throat's share (vibThroat_), the hand's (vibHand_: the instrument rocked under the lips,
  // the rim covered and uncovered: pitch and colour, little level), the rest the diaphragm.
  double vibThroat_ = 0, vibHand_ = 0, kPanVib_ = 1.6, kPanHand_ = 0.35, kPanHandK_ = 3;
  bool pipes_ = false;
  // A fast run's tonguing (the director's kCueTongue): the note's place in its pair or three;
  // the player's three is da-da-ga (TTK) or, by habit, da-ga-ga (TKK; PJ_PAN_TKK, a seeded
  // player's own choice: one in three).
  int tongueCue_ = 0;
  bool tkk_ = false;
  // A slur down a semitone or two on the same pipe (bottle): the lip covers more of the rim and
  // the air goes down into it, the pitch sinks (at two semitones darker and weaker, at one barely
  // changed). pipeNote_: the pipe under the lips (a note's own, or the bent one's); bend_ the
  // semitones bent now, moving to the target over ~50 ms. kBend_: log uncovered width per
  // semitone; kBendProb_: how often a player bends rather than moves (PJ_PAN_BEND, x; 0 never).
  int pipeNote_ = -1;
  double bend_ = 0, kBend_ = 0.5, kBendProb_ = 1;
  Rng bendRng_{randomSeed(3, 0)};
  // A pan flute's phrase end (the director's cue, not the piece's last): blown softer and more
  // diffuse, a lower, airier breath, growing over the note (endAir_ 0 .. 1).
  bool endSoft_ = false;
  double endAir_ = 0, kPanEndDyn_ = 0.12, kPanEndAir_ = 0.3, kPanEndTune_ = 0.17, kPanEndP_ = 0.4;
  // The bottles' jugs and vials (C2..C4, C6..C8): radiation grows with pitch, ~9 dB an octave
  // out there against ~5 dB in the middle; a player picks a jug a size up to be heard, and blows
  // a vial gently. Trimmed (dB an octave, below C4 and above C6) towards ~3 dB an octave.
  bool bottles_ = false;
  double pipeGain_ = 1, pipeTarget_ = 1, kJugTrim_ = 6.5, kVialTrim_ = 5;
  // The instrument aimed into the room (Tube::aimOn: the shakuhachi held in the hands): how far
  // its foot is lowered below the horizontal and swung to a side, each moving as a body moves
  // (two lags). Lifted for loud playing, an accent and the high notes, bowed for a phrase's end
  // and while resting; each phrase turns to a side of its own, mostly the other one, with a slow
  // sway inside it (aim()). PJ_AIM scales the motion (0: held still, at 50 degrees).
  void aim(double dt);
  double aimDown_[2] = {0.873, 0.873}, aimSide_[2] = {0, 0}, aimSideTo_ = 0, aimSidePh_ = 0;
  double aimSet_[2] = {-1, 0}, kAim_ = 1, aimSince_ = 0;
  int aimSign_ = 1;
  bool aimNew_ = false, aimEnd_ = false;
  uint32_t aimRng_ = 9191;
  double aimRand() { aimRng_ = aimRng_ * 1664525u + 1013904223u; return (aimRng_ >> 8) * (1.0 / 16777216.0); }
  double glottisClock_ = 1e9;
  Vibrato vib_;  // its own vibrato (performer.h)
  double lung_ = 0;
  double tongueClock_ = 1e9, tongueRelease_ = 0.005, tongueClosed_ = 0.004;
  double lungAttack_ = 0.005, sinceOff_ = 1e9;
  double flutterPhase_ = 0;
  double flutterRate_ = 27;  // this player's own
  double sinceOn_ = 0;
  double velAccent_ = 0;
  int keyVel_ = 64;
  bool kLipCloseAll_ = true;
  double regKick_ = 0, kLipClose_ = 0.012, kRelTune_ = 0, lungHeld_ = 0, kKickH_ = 0, kKickY_ = 0, kGeoH_ = 0.5, kGeoY_ = 1, kKickP_, kKickW_, kKickT_, kKickDyn_, kSlurKick_ = 1.9, kVentKickDyn_ = 0,
         kVentKickT_ = 0.04, kickT_ = 0.06;
  double slurKick(int to);  // the register kick for a slur up to `to` (sets its duration)
  double kDynH_, kDynP_, kVelDyn_;
  double kAttackKick_;
  // Slow random wander of breath support, jet aim and lip opening (~0.3-4 Hz).
  uint32_t rng_ = 4242;
  double w1a_ = 0, w1b_ = 0, w2a_ = 0, w2b_ = 0, w3a_ = 0, w3b_ = 0;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; }
  double velDyn_ = 0, velDynTarget_ = 0;  // dynamics offset from key velocity
  // A practised slur (src/slurs.h; tools/slurs.py, kind 'jet'): on top of the register kick, the
  // air pushed or eased at the change, the lips' jet shortened (up) or lengthened (down) and
  // settling, the fingers' speed, the lips and air going first (lead), or a soft tongue ("d").
  SlurGesture gesture_;
  bool practised_ = false;
  int slurDir_ = 0;
  double slurAir_ = 0, slurLip_ = 0, slurDip_ = 0, slurAirT_ = 0.03, slurLipT_ = 0.03, slurDipT_ = 0.08, sinceSlur_ = 1e9;
  double leadLeft_ = 0, leadSpeed_ = 1, tongueFloor_ = 0;
  const char* leadHoles_ = nullptr;
};

}  // namespace pajupilli
