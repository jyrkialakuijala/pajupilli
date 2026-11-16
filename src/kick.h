// The jazz bass drum (-inst jazz-kick, jazz-kick-20; research/kick.md): the
// bebop kick, 18" x 14" (or 20" x 14"), both heads on, no port, tuned high
// and open, a felt beater.
//
//   - Two heads (membrane.h) on a shell of air (DrumAir): the batter,
//     coated, and the resonant head, unported. The air couples their m = 0
//     modes: the drum's note is the pair's lower member (both heads moving
//     the same way, carrying the air), its upper member a little above.
//   - The beater: a felt ball on the pedal's shaft, a mass with a soft
//     contact (Hunt-Crossley, felt's steep law) on a patch of the batter
//     below its middle. It is thrown from just over the head where the head
//     is now, so a stroke meets the drum's ringing: in step with it, it adds;
//     against it, it takes some away. Then it springs back off the head
//     (the pedal's spring) and stays off: one key, one stroke.
//   - Buried: with the foot kept on the pedal (the hold), the beater stays
//     pressed into the head after the stroke, damping it: the short, dead
//     "thud".
//   - A hard stroke stretches the heads: the note starts sharp and falls.
//   - The sound: a microphone in front of the resonant head (and one a little
//     to the side) hears both heads by each mode's shape, the batter round the
//     shell; and the felt's own thump above the modes' top.
#pragma once

#include <cstdint>
#include <vector>

#include "membrane.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool jazzKick(const Instrument& inst);

struct KickSpec {
  HeadSpec batter, reso;
  DrumAir air;
  double depth = 0.356;          // shell, m (14")
  double beaterMass = 0.07;      // kg, at the felt (the ball, the shaft, the pedal)
  double feltK = 1.5e8;          // N/m^alpha
  double feltAlpha = 2.3;
  double feltLoss = 0.6;         // Hunt-Crossley, s/m
  double feltRadius = 0.025;     // m: the patch the felt presses
  double beaterR = 0.12;         // where it lands, as a share of the radius (below the middle)
  double returnHz = 6;           // the pedal's spring with the beater on it
  double buryForce = 12;         // N: the foot left on the pedal
  double footDamp = 60;          // N s/m: the leg holding it there
  double micZ = 0.6, micX = 0.25;  // the microphones in front of the resonant head (m)
  double click = 1;              // the felt's own thump above the modes (x)
  double gain = 1;
};
KickSpec kickSpec(bool twenty);  // with the PJ_KICK_* knobs

class Kick {
 public:
  Kick(double fs, const KickSpec& spec);
  // A stroke: the beater's speed at the head (m/s), where (share of the
  // radius); bury: the foot stays on.
  void strike(double speed, double r, bool bury);
  void setBury(bool b) { bury_ = b; }  // the foot on the pedal, or lifted
  // The room's sound outside each head this sample (Pa): it presses on them
  // (the band's bass notes ring the drum along). Loud enough, it wakes it.
  void setOutside(float pBatter, float pReso);
  float sample();
  float side() const { return side_; }
  void reset();
  bool asleep() const { return asleep_; }
  const KickSpec& spec() const { return s_; }
  Membrane& batter() { return bat_; }
  Membrane& reso() { return res_; }
  bool touching() const { return touch_; }
  double force() const { return lastF_; }

 private:
  void place(double r);
  KickSpec s_;
  double fs_, dt_;
  Membrane bat_, res_;
  DrumAir air_;
  std::vector<float> phi_;       // the felt's patch, averaged shape
  double zb_ = 1, vb_ = 0, rest_ = 0;  // the beater: height over the head's rest (m), speed
  bool active_ = false, bury_ = false, touch_ = false, left_ = false, hit_ = false;
  double lastF_ = 0;
  std::vector<float> micB_[2], micR_[2];
  float prev_[2] = {}, hpC_ = 0, lastC_ = 0, side_ = 0;
  bool asleep_ = true;
  int quiet_ = 0;
  float pOutB_ = 0, pOutR_ = 0;  // the room outside the batter and the resonant head (Pa)
};

class KickModel : public Tube {
 public:
  explicit KickModel(const Instrument& inst);
  Kick& kick() { return kick_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  Kick kick_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
  std::vector<float> delay_;  // the room's pressure, for the far head (the shell's depth late)
  int delayPos_ = 0;
};

// The drummer's right foot. Keys: General MIDI's 35 (Acoustic Bass Drum: the
// beater a little lower, more of the drum's ring) and 36 (Bass Drum 1: nearer
// the middle, rounder). Velocity is the beater's speed: feathered quarters at
// 20 .. 70 (0.2 .. 1 m/s), accents and "bombs" above. The hold (the sustain
// pedal; on the drum channel CC 64) is the heel down, the beater buried;
// PJ_KICK_BURY=1 buries every stroke.
class KickPlayer : public Performer {
 public:
  explicit KickPlayer(KickModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int) override {}
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned s) override { rng_ = 4241u + 7919u * s; }
  static double beaterSpeed(int velocity, double dynamics);  // m/s

 private:
  double ur();
  KickModel& m_;
  uint32_t rng_ = 4241u;
};

}  // namespace pajupilli
