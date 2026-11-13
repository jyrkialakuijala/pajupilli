// A plucked double bass (research/double_bass.md): four strings, the fingers,
// a bridge, the strings behind it and the tailpiece, and the body
// (bassbody.h).
//
// Each string: its 64 lowest modes in each of two directions (y: away from
// the top plate; z: along it, the way the finger pulls), each a displacement
// and a velocity (4 x 64 float32s). Every mode sits at its own frequency (a
// stiff string's overtones run a little sharp) with its own decay.
//   - All four end on the same bridge: the bridge's acceleration drives every
//     string's modes, so the strings not played ring along.
//   - A hard pluck stretches the string: its tension rises with its
//     amplitude, so the pitch starts a little sharp and falls.
//   - The fingerboard lies under the string: where a strong pluck swings it
//     down onto the board, the board pushes back (the growl, the slap).
//   - A stopped string ends on a fingertip: more loss, more so high up.
//
// Behind the bridge each string goes on to the tailpiece (a short length,
// 6 modes, away from the top only), and the tailpiece, a mass on its cord,
// swings on them.
//
// The bridge: a mass on its legs. Its top moves against the feet up and down
// (stiff) and sideways on its waist (softer: the bridge's own resonance,
// which passes the strings' overtones under it and holds them back above
// it). Sideways force rocks the feet: one down, the other up. Each string
// sits at its own notch: the low strings press the bass foot more.
//
// The fingers: a jazz bassist plucks with two, the index and the middle,
// taking turns (or both together for an accent). Each is a pad of some width
// that pulls the string aside (mostly along the top), holds, and lets it slip
// off. Their skin is calloused, tougher than an ordinary fingertip: the pad
// gives less, so the string leaves it faster and cleaner (a slip of about a
// millisecond: more of the upper overtones), and it damps the string less
// while it holds. The middle finger is longer and stronger: it plucks a
// little further from the bridge and a little deeper; the index is lighter.
// A finger landing on a string that still sounds stops it first.
//
// The colour is alive, never one fixed spectrum dying away:
//   - The two directions are not alike. Across the top the string's ends
//     give differently than towards it, so every overtone has two slightly
//     different pitches (up to 0.3 % apart, each overtone its own), and the
//     plane of its swing turns slowly. Heard through the bridge, each
//     overtone swells and fades at its own slow rate.
//   - A gut string is not uniform: each overtone a few cents off its place
//     and with its own damping (up to +-25 %).
//
// PJ_BASS_SYMP, _AFTER, _TAIL, _BRIDGE, _TMOD, _BOARD = 0 switch a part off
// (to hear what it does); PJ_BASS_ALIVE scales the aliveness (0: none).
#pragma once

#include <cstdint>

#include "bassbody.h"

namespace pajupilli {

struct BassString {
  double f0;       // open frequency (Hz)
  double length;   // m
  double tension;  // N
  double stiff;    // inharmonicity B: f_k = k f0 sqrt(1 + B k^2)
  double loss0, loss1;  // decay rates (1/s): loss0 + loss1 k^1.2
  double ea;       // the string's stretch stiffness E A (N)
  double action;   // its height over the fingerboard's end (m)
};

class Bass {
 public:
  static constexpr int kStrings = 4, kModes = 64, kAfter = 6, kBoard = 5;
  explicit Bass(double fs);
  void setStringDef(int s, const BassString& def);
  // Pluck string s, stopped at a sounding length (m; 0: open): pull depth (m),
  // where (share of the sounding length from the bridge), the pull's angle
  // from the top's plane (rad; outward positive: the string then swings back
  // onto the fingerboard). finger: 0 index tip, 1 middle tip, 2 the jazz pull
  // (the two as one), 3 the thumb, 4 the flat finger (the index's side along
  // the string), -1: the tips take turns.
  void pluck(int s, double stoppedLength, double depth, double where = 0.2, double angle = 0.3, int finger = -1);
  // The left hand moves on a sounding string: a new length over `glide` s
  // (a slide; short: a hammer-on or pull-off), without a pluck.
  void setLength(int s, double stoppedLength, double glide);
  // A harmonic (flageolet): a finger lies lightly on the string at 1/n of its
  // sounding length for a moment after the pluck: every overtone that moves
  // there dies under it, and the n-th and its multiples ring on. Call after
  // pluck().
  void touch(int s, int n, double secs = 0.09);
  // Pluck string s on the wrong side of the bridge, between it and the
  // tailpiece: a short, high ping (the string's 6th overtone), whatever the
  // left hand does.
  void pluckBehind(int s, double depth);
  void bend(int s, double cents);           // vibrato: the fingertip rocks
  // The left hand lies on the string: it dies at `rate` (1/s; a firm stop ~70,
  // a light touch ~10 lets it ring out).
  void damp(int s, double rate = 40);
  void setSkin(double toughness) { skin_ = toughness; }  // 0 soft fingertip .. 1 calloused
  // The strings' height over the fingerboard, x the instrument's (1): low (0.4)
  // and the strings graze the board (the growl); high (1.4): clean.
  void setAction(double scale);
  // The microphones' shares: far away, close to an f-hole, a bridge pickup.
  void setMics(double far, double fhole, double pickup) {
    micFar_ = float(far); micHole_ = float(fhole); micPickup_ = float(pickup);
  }
  float sample(float testForce = 0);
  float side() const { return side_; }
  float footForce() const { return footForce_; }  // the bridge's bass foot on the top (N)  // left = sample + side, right = sample - side
  void reset();  // everything at rest (after a NaN)
  BassBody& body() { return body_; }
  double openLength(int s) const { return str_[s].def.length; }

 private:
  struct Str {
    BassString def{};
    double len = 1, lenFrom = 1, lenTo = 1, glideT = 1e9, glideDur = 0, bendCents = 0;
    bool stopped = false, damping = false, active = false, boardOn = false;
    alignas(64) float q[2][kModes] = {}, v[2][kModes] = {};
    // [direction]: the two directions differ a little in every mode.
    alignas(64) float cs[2][kModes], sn[2][kModes], w[2][kModes], wdt[2][kModes], decay[2][kModes];
    alignas(64) float touch[kModes];  // a touching finger's damping of each mode, per sample
    double touchClock = 1e9, touchDur = 0;
    alignas(64) float turn[kModes];  // the plane of each mode's swing turns (rad per sample)
    alignas(64) float bridgeIn[kModes], slope[kModes], finger[kModes], stretch[kModes];
    alignas(64) float board[kBoard][kModes];
    float clear[kBoard] = {};
    float invModalMass = 0, notch = 0;
    // The finger: a force along its pull (N), rising, held, then slipping off.
    double fingerTarget = 0, fingerClock = 1e9, fingerAngle = 0.3, slip = 0.0012, cut = 0.00015, holdDamp = 12;
    double boardClock = 1e9, dampClock = 0, dampRate = 40, behindClock = 1e9;
    // Behind the bridge (y only).
    float aq[kAfter] = {}, av[kAfter] = {}, acs[kAfter], asn[kAfter], aw[kAfter], adecay[kAfter];
    float aInB[kAfter], aInT[kAfter], aSlopeB[kAfter], aSlopeT[kAfter];
    float aInvModalMass = 0;
  };
  void tune(Str& s);     // the modes' coefficients at the present length
  void control();        // every 16 samples: slides, vibrato, which strings sound
  double fs_, dt_;
  BassBody body_;
  Str str_[kStrings];
  int ctl_ = 0;
  // The bridge's top (a mass on the legs) and the tailpiece.
  float uy_ = 0, vy_ = 0, uz_ = 0, vz_ = 0, ay_ = 0, az_ = 0;
  float ut_ = 0, vt_ = 0, at_ = 0;
  // Along the strings: the bridge's top tipping, the tailpiece on its cord.
  float ux_ = 0, vx_ = 0, xt_ = 0, vxt_ = 0, sumEAL_ = 0, sumEALa_ = 0;
  double action_ = 1;
  float prevVy_ = 0, prevVz_ = 0, footForce_ = 0, prevFootV_ = 0;
  double skin_ = 1;
  int nextFinger_ = 0;
  uint32_t rng_ = 12345;
  float micFar_ = 0.25f, micHole_ = 0.42f, micPickup_ = 0.33f, micTop_ = 0.08f, width_ = 0.7f, side_ = 0;
  bool kSymp_, kAfter_, kTail_, kBridge_, kTmod_, kBoard_;
  double kAlive_ = 1;  // PJ_BASS_ALIVE: the two directions' differences and the string's irregularities
  static constexpr double kRise_ = 0.012, kHold_ = 0.003;
  static constexpr double kBridgeH_ = 0.16, kFeet_ = 0.17;  // bridge height, foot spacing (m)
  static constexpr double kAfterLen_ = 1.06 / 6;            // bridge to tailpiece (m): 1/6 of the string
  static constexpr double kBoardEnd_ = 0.20;                // fingerboard's end to the bridge (m)
};

}  // namespace pajupilli
