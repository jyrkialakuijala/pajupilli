// The timpani (-inst timpani; General MIDI program 48; research/timpani.md): five kettledrums,
// 32, 29, 26, 23 and 20 in., played by one timpanist.
//
//   - Each head is a membrane (membrane.h) heard as its modes, not a grid: what makes a timpani a
//     pitched drum is its partials' ratios, 1 : 1.50 : 1.98 : 2.44 : 2.90 : 3.37 on the (m, 1)
//     modes within ~1 %, which the air riding on the head and the kettle's air make out of the
//     ideal membrane's 1 : 1.34 : 1.66 ... A grid would place its modes a few per cent off and
//     square; here each mode is placed where it was measured (12.1): the pitched family pinned,
//     the (0,1) near-fixed by the kettle's air spring (65 Hz + 0.46 f11), the (0,2), (0,3), (1,2),
//     (2,2) ... at their measured ratios, the rest by the air's load falling with frequency (the
//     effective density 0.26 + 0.09 (600 / f)^2 kg/m^2); its mass the head's and that air's.
//   - Each mode decays at its own measured rate (3.1, 12.3): the main tone a few seconds on the low
//     drums, ~1 s at a drum's top; the fifth and the octave longer; the axisymmetric thump 0.3 s.
//     Tuning up shortens all of it.
//   - A felt mallet (28 g, F = K x^2.54, a felt's loss: Hunt-Crossley) thrown at the head where it
//     is now, a quarter of the way in from the rim: a little further in for a louder stroke
//     (12.5), the two hands a little apart; the contact solved as the head gives under it. A hard
//     stroke stretches the head: it starts sharp.
//   - The sound: the head's motion radiated far (a baffled head's far field, each mode's own
//     pattern: the (0,n) a monopole, the (1,1) a dipole along the line from the player through
//     the centre), towards the hall, two directions for the two channels. The kettle itself does
//     not sound. The timpani belong in the concert hall (12.8b): -play puts them there.
//   - Rolls (7, 12.6): strokes on one drum under 140 ms apart are played as a roll: the hands
//     alternating 12 cm apart round the playing ring (each its own mix of the modes and of the
//     split pairs' members), evened to about +-0.5 dB, each a few ms late at random (not a
//     machine gun); the two hands are not alike (Jyrki, 2026-11-19: a roll is pu-po-pu-po, not
//     pu-pu-pu-pu): the stronger hand strikes 2 % of the radius further in, 4 % faster, with
//     the harder of the two mallets (x1.3 against x0.77), in every stroke, rolled or not
//     (PJ_TIMP_HANDS x the differences; 0: alike). A key struck again alternates; another
//     key begins with the leading hand, so a key's first stroke is always the same; every stroke lands on the head as it is moving, adding to its swing or
//     taking from it. A held key rolls with the mod wheel up (live: pad 1), its depth the roll's
//     level, or with PJ_TIMP_ROLL=1 when held over 0.6 s: at the natural rate, 10 (f11 / 110 Hz)^0.4
//     strokes a second, faster as it gets louder (x0.85 at pp .. x1.2 at ff), following the
//     expression (a crescendo roll), the first stroke the key's and the rest a little softer.
//   - The pedal: a key on a drum tuned elsewhere retunes it (at once: no look-ahead to do it
//     earlier). The hand: a key held 0.22 s or more is damped 0.15 s after its release, unless the
//     drum is struck again first; shorter notes ring on.
// Knobs: PJ_TIMP_MODES (500: modes a head), PJ_TIMP_DECAY (x every T60), PJ_TIMP_K, PJ_TIMP_ALPHA,
// PJ_TIMP_MASS (the mallet), PJ_TIMP_VMAX (x the speeds), PJ_TIMP_SPOT (x the strike radius),
// PJ_TIMP_THUMP (x the (0,n) modes' radiation), PJ_TIMP_OUT.
#pragma once

#include <memory>
#include <vector>

#include "membrane.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool timpaniInstrument(const Instrument& inst);

class Kettle {
 public:
  Kettle(double fs, int index);
  int lo() const { return lo_; }
  int hi() const { return hi_; }
  void tune(double f11);                                // the pedal: every mode to the new pitch
  double pitch() const { return f11_; }
  // speed (m/s), r (share of the radius), theta; hard: x this mallet's stiffness (the two differ)
  void stroke(double speed, double r, double theta, double hard = 1);
  void damp(bool on);                                   // the hand on the head, or off
  float sample(float& side);
  bool asleep() const { return asleep_; }
  Membrane& head() { return *head_; }
  double t60Main() const { return t11_; }

 private:
  double fs_, dt_, radius_, pan_;
  int lo_, hi_;
  std::unique_ptr<Membrane> head_;
  std::vector<double> ideal_;    // each mode's ideal-membrane ratio to the (1,1)
  std::vector<float> phi_, wRe_[2], wIm_[2];
  double f11_ = 0, t11_ = 1;
  // The mallet.
  double zs_ = 1, vs_ = 0, sG_ = 0, lambda_ = 0, lastX_ = 0, hard_ = 1;
  int sAge_ = 0;
  bool stickOn_ = false;
  // The hand.
  double handX_ = 1, handTo_ = 1;
  int handCount_ = 0;
  float prev_[2] = {};
  bool asleep_ = true;
  int quiet_ = 0;
  void retune();
};

class TimpaniModel : public Tube {
 public:
  explicit TimpaniModel(const Instrument& inst);
  int kettles() const { return int(k_.size()); }
  Kettle& kettle(int i) { return *k_[i]; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;
  bool asleep() const;

 private:
  void controlStep() override {}
  float sample() override;
  std::vector<std::unique_ptr<Kettle>> k_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

// The timpanist: each key to its drum (32 in.: C2-G#2; 29: A2-C#3; 26: D3-F#3; 23: G3-Bb3;
// 20: B3-E4; below and above, the outer drums), the velocity to the mallet's speed and the
// spot (12.5), the hands alternating; the damping hand at the release (above).
class TimpaniPlayer : public Performer {
 public:
  explicit TimpaniPlayer(TimpaniModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override { hold_ = h; }
  void seed(unsigned s) override { rng_ = 7151u + 7919u * s; }
  static double malletSpeed(int velocity);  // m/s
  static int drumFor(int midi);

 private:
  double ur();
  void strike(int d, double speed, double r, double theta, double hard = 1);
  double rollRate(int d) const;  // strokes a second
  TimpaniModel& m_;
  uint32_t rng_ = 7151u;
  double now_ = 0;
  int hand_ = 0;
  int lastKey_ = -1;  // (the key struck last: another key begins with the leading hand again)
  double onAt_[128] = {};
  int drumOf_[128];
  double lastStroke_[8] = {}, dampAt_[8];
  // Rolls: a held key's (its key, the next stroke, its velocity), and a written-out one's
  // strokes (evened, each a little late: the one waiting).
  int rollKey_[8], rollVel_[8] = {};
  double rollNext_[8] = {}, evenSpeed_[8] = {};
  struct Pending { double due = -1, speed = 0, r = 0, th = 0, hard = 1; } pend_[8];
};

}  // namespace pajupilli
