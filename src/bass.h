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
#include "bodybank.h"

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

// An instrument of this construction: the double bass, or the cello (research/cello.md), the
// same parts at its size.
struct BassSpec {
  BassString strings[4];     // low to high
  int open[4];               // their open notes (MIDI)
  int top;                   // the top of the stopped notes (above: harmonics)
  double notch;              // the strings' spacing on the bridge (m)
  double crown;              // the bow's angle from one string to the next (rad)
  double bridgeM;            // the bridge's top, above the waist (kg)
  double bridgeFy, bridgeQy; // its top against the feet, up and down (Hz, 1 / Q)
  double bridgeFz, bridgeQz; // ... sideways on the waist: the bridge's own resonance
  double tailM, tailF, tailQ;  // the tailpiece on its cord (kg, Hz, Q)
  double bridgeKx, bridgeCx, cordK, cordC;  // along the strings (N/m, N s/m)
  double bridgeH, feet;      // bridge height, foot spacing (m)
  double afterLen;           // bridge to tailpiece (m)
  double boardEnd;           // the fingerboard's end to the bridge (m)
  double hair;               // the bow hair's width on the string (m)
  double bodyRate, bodyStiff, bodyAir;  // the body's losses (BassBody::setDamping)
  const BodyData* body;
  // What is heard: the levels that bring the far sound, the f-hole, the bridge pickup and the
  // top by the bridge to a like level, and the two sides' difference.
  double farLevel, holeLevel, pickupLevel, topLevel;
  double bowLength;          // the hair's length used in a stroke (m)
  double vibRate;            // the vibrato's speed x the knob's (a bassist's is slower)
  const float* bowLevels;    // each bowed note against its neighbours (dB; tools/bowlevels.py), from bowLevelLo
  int bowLevelLo, bowLevelN;
  double bowSpeed;           // the bow's speed at ff less its 5 cm/s at pp (m/s)
  double vibNat;             // the player's own vibrato on a stopped note under the bow, with no knob
                             // (cents each way in the first position; wider up the neck: the same
                             // finger travel on a shorter string)
  // The endpin on the stage floor: the bridge's force on the body, as far as the pin passes it,
  // sets the floor going around its own resonance (Hz, Q); floorGain: its sound against the
  // body's far sound (0: none).
  double floorGain, floorF, floorQ;
  // The open strings below equal temperament (semitones): a cello is tuned in pure fifths down
  // from its A (C 5.9, G 3.9, D 2.0 cents flat), so its open strings' overtones meet and do not
  // beat; the stopping finger makes up the difference.
  double openFlat[4];
  double pizzWait;           // (s) a key let go within this is plucked; keys sound this much late (0: none)
  double muteM;              // the mute's mass on the bridge's top (kg; con sordino)
  double attackScale = 1;    // the stroke's ease-in against the bass's (a light string speaks sooner)
  double bowWmax = 0.5;      // the bow drives the modes turning up to about this much a sample (rad;
                             // 0.5: 3.8 kHz at 48 kHz), the higher ones less and less
  // (Viola.md, findings 1 to 3.) The strings, the bow and the bridge are stepped this many times
  // a sample (the body once, with the feet's mean force): at one step a sample the bowed string
  // had not converged (the same note at twice the rate played 6 to 9 cents sharper, its overtones
  // up to 10 dB otherwise), and the bow could not be let drive a high note's upper modes. bowWmax
  // stays what it is at 48 kHz (a frequency: x 7.6 kHz).
  int oversample = 1;
  // The bow's force sharpens the string's corner as it passes under the hair: the bow drives the
  // modes up to bowWmax x (its force against the force of a forte stroke, 40 cm/s at the same
  // place) to this power (0: always bowWmax; 0.75: pp 0.3 x, mf 0.76 x, f and above 1.1 x), no
  // further than a step can follow (0.5 rad a step). Louder is brighter, as under a real bow.
  double bowBright = 0;
  // The body above its grid (src/bodybank.h): the level of a bank of resonances from about 1 kHz
  // up, against the far sound (0: none).
  double bankGain = 0;
};
extern const BassSpec kDoubleBassSpec, kCelloSpec, kViolaSpec, kViolinSpec;

class Bass {
 public:
  static constexpr int kStrings = 4, kModes = 64, kAfter = 6, kBoard = 5;
  explicit Bass(double fs, const BassSpec& spec = kDoubleBassSpec);
  const BassSpec& spec() const { return spec_; }
  void setStringDef(int s, const BassString& def);
  // Pluck string s, stopped at a sounding length (m; 0: open): pull depth (m),
  // where (share of the sounding length from the bridge), the pull's angle
  // from the top's plane (rad; outward positive: the string then swings back
  // onto the fingerboard). finger: 0 index tip, 1 middle tip, 2 the jazz pull
  // (the two as one), 3 the thumb, 4 the flat finger (the index's side along
  // the string), 5 the nail, -1: the tips take turns.
  void pluck(int s, double stoppedLength, double depth, double where = 0.2, double angle = 0.3, int finger = -1);
  // The left hand moves on a sounding string: a new length over `glide` s
  // (a slide; short: a hammer-on or pull-off), without a pluck.
  void setLength(int s, double stoppedLength, double glide);
  // A harmonic (flageolet): a finger lies lightly on the string at 1/n of its
  // sounding length for a moment after the pluck: every overtone that moves
  // there dies under it, and the n-th and its multiples ring on. Call after
  // pluck().
  void touch(int s, int n, double secs = 0.09, double rate = 900);
  // Pluck string s on the wrong side of the bridge, between it and the
  // tailpiece: a short, high ping (the string's 6th overtone), whatever the
  // left hand does.
  void pluckBehind(int s, double depth);
  // The bow on string s (arco): its speed along its stroke (m/s; the sign is
  // the stroke's direction: down-bow positive), the force pressing it on the
  // string (N; 0: it leaves the string), where (m from the bridge), and how
  // fast speed and force get there (s; riseForce: the force's own time, if
  // not the speed's). Call again whenever they change.
  //   The hair grips the string by its rosin: while they move together the
  // string sticks to it (friction up to 0.9 x the force); pulled off, it
  // slips back under a weaker friction that falls with the slipping speed
  // (0.35 x at high speed), until it is caught again: once a period, the
  // string's kink running round (Helmholtz's motion). Solved every sample
  // with the string's own answer to the force (Friedlander's construction,
  // with its hysteresis: a sticking string sticks while it can, a slipping
  // one slips while it can: so a heavy bow plays flat, then crushes the
  // note; a light fast one whistles on the surface). Each string is bowed
  // along the bridge's crown at its notch: the outer strings partly towards
  // the top.
  void bow(int s, double speed, double force, double where, double rise = 0.02, double riseForce = 0);
  // The force for that speed and place: pressure 0 (the least that holds the
  // note) .. 1 (the most the string bears).
  // (harmonic n: the string swings in n parts, each as a string of its own.)
  double bowForce(int s, double speed, double where, double pressure, int harmonic = 1) const;
  // The left hand alone: string s stopped at this length (0: open), at once.
  void finger(int s, double stoppedLength);
  double length(int s) const { return str_[s].len; }
  bool bowing(int s) const { return str_[s].bowOn; }
  bool slipping(int s) const { return str_[s].bowOn && str_[s].slipping; }  // (tests)
  void bend(int s, double cents);           // vibrato: the fingertip rocks
  // The left hand lies on the string: it dies at `rate` (1/s; a firm stop ~70,
  // a light touch ~10 lets it ring out).
  void damp(int s, double rate = 40);
  // The mute (con sordino): its mass on the bridge's top, which lowers the bridge's own
  // resonances and holds the upper overtones back from the body (veiled, softer).
  void setMute(bool on) { kBridgeM = float(spec_.bridgeM + (on ? spec_.muteM : 0.0)); }
  void setSkin(double toughness) { skin_ = toughness; }  // 0 soft fingertip .. 1 calloused
  // One of a section's instruments (seat 1, 2, ...): its own build, as instruments of one
  // design differ (17 recorded violins: their levels per third of an octave 4 to 5 dB apart,
  // standard deviation, recording and player included): the plates up to 12 % stiffer or softer
  // (the wood resonances up to 6 % off), the air 8 %, the losses 25 % and the bridge's mass
  // 8 %, its upper resonances its own (BodyBank), and where it is heard from.
  void vary(unsigned seat);
  // The strings' height over the fingerboard, x the instrument's (1): low (0.4)
  // and the strings graze the board (the growl); high (1.4): clean.
  void setAction(double scale);
  // The microphones' shares: far away, close to an f-hole, a bridge pickup.
  void setMics(double far, double fhole, double pickup) {
    micFar_ = float(far); micHole_ = float(fhole); micPickup_ = float(pickup);
  }
  float sample(float testForce = 0);
  float side() const { return side_; }
  // The body's sound along its three axes (BassBody::dipole), on the far sound's scale.
  float dipole(int k) const { return rest_ ? 0.0f : dipScale_ * (body_.dipole(k) + bankG_ * bank_.dipole(k)) / farLevel_; }
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
    int touchN = 1;
    alignas(64) float turn[kModes];  // the plane of each mode's swing turns (rad per sample)
    alignas(64) float bridgeIn[kModes], slope[kModes], finger[kModes], stretch[kModes];
    alignas(64) float board[kBoard][kModes];
    float clear[kBoard] = {};
    float invModalMass = 0, notch = 0;
    // The bow: each mode's shape under the hair, their squares' sum, the
    // bridge's motion as seen there; speed, force (and where they are going).
    alignas(64) float bowShape[kModes] = {};
    float bowSum = 0, bowBr = 0, bowV = 0, bowF = 0, bowVTo = 0, bowFTo = 0, bowK = 0.01f, bowKF = 0.01f, bowSin = 0, bowCos = 1;
    float bowNoise = 0, bowTors = 0, bowTw = 0, bowTwR = 0;
    float bowBright = 1, bowBrightAt = 1;  // the bow's reach against bowWmax now; as the shape was made
    float bowTurn = 0;  // (a change of direction under way: the speed it came from)
    double bowDist = 0;
    bool bowOn = false, slipping = true;
    // The finger: a force along its pull (N), rising, held, then slipping off.
    double fingerTarget = 0, fingerClock = 1e9, fingerAngle = 0.3, slip = 0.0012, cut = 0.00015, holdDamp = 12;
    double boardClock = 1e9, dampClock = 0, dampRate = 40, behindClock = 1e9;
    // Behind the bridge (y only).
    float aq[kAfter] = {}, av[kAfter] = {}, acs[kAfter], asn[kAfter], aw[kAfter], adecay[kAfter];
    float aInB[kAfter], aInT[kAfter], aSlopeB[kAfter], aSlopeT[kAfter];
    float aInvModalMass = 0;
  };
  void tune(Str& s);     // the modes' coefficients at the present length
  void bowShape(Str& s);
  void stepStrings();    // one step of the strings, the bow, the tailpiece and the bridge
  void control();        // every 16 samples: slides, vibrato, which strings sound
  const BassSpec& spec_;
  // (the instrument's constants, from its spec: named as when they were the bass's alone)
  float kBridgeM, kKy, kCy, kKz, kCz, kTailM, kTailK, kTailC, kBridgeKx, kBridgeCx, kCordK, kCordC;
  double kHair, kBridgeH_, kFeet_, kAfterLen_, kBoardEnd_;
  float farLevel_, holeLevel_, pickupLevel_, topLevel_;
  float floorG_ = 0, floorW_ = 0, floorD_ = 0, floorY_ = 0, floorV_ = 0, floorLp_ = 0;  // the stage floor under the endpin
  double fs_, dt_;        // the strings' rate and step (the sample rate x oversample)
  double fsOut_, dtOut_;  // the sample rate: the body, what is heard
  int os_ = 1;
  float feetYSum_ = 0, footBSum_ = 0, footTSum_ = 0;  // (the steps of one sample, summed)
  float noiseK_ = 0.3f;
  BassBody body_;
  BodyBank bank_;      // the body's upper resonances (BassSpec::bankGain)
  float bankG_ = 0;
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
  bool rest_ = false;  // nothing sounds: not computed (Bass::sample)
  int calm_ = 0;       // samples of stillness so far
  float micFar_ = 0.25f, micHole_ = 0.42f, micPickup_ = 0.33f, micTop_ = 0.08f, width_ = 0.7f, side_ = 0;
  bool kSymp_, kAfter_, kTail_, kBridge_, kTmod_, kBoard_;
  float dipScale_ = 1;  // PJ_DIPOLE: the directional parts against the physical (1; 0: none)
  double kAlive_ = 1;  // PJ_BASS_ALIVE: the two directions' differences and the string's irregularities
  static constexpr double kRise_ = 0.012, kHold_ = 0.003;
};

}  // namespace pajupilli
