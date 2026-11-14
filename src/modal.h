// Struck and plucked things as their modes (the GM ethnic and percussive
// families: research/sitar_shamisen.md, banjo_koto.md, kalimba_fiddle.md,
// steelpan_taiko.md, bells_blocks.md).
//
// An instrument is a set of vibrators (strings, tines, a pan's note areas,
// bells, heads) and, for the stringed ones, a body under a bridge:
//
//   - A vibrator: its lowest modes, each a damped oscillator of unit modal
//     mass, x'' + 2 s x' + w^2 x = f, stepped exactly (its free motion over a
//     sample is a matrix of four numbers; a force enters as an impulse on the
//     velocity first). Two directions: y (towards the bridge's top: what the
//     bridge feels, what a jawari bridge stops) and z (along the top), each
//     mode a hair apart between them, so the plane of its swing turns.
//   - The bridge and the body: the vibrators' y modes push the bridge (a
//     string's tension times its slope there; a tine's root force), the body
//     (its modes at the bridge, each with its own mass) moves, and its
//     acceleration drives every vibrator back: the strings not played ring
//     along (a sitar's sympathetic strings), the body's resonances colour
//     everything and take the energy away at their own rates.
//   - Contact: an obstacle under the string near its end (a sitar's jawari,
//     a shamisen's sawari): at a few points close to the termination the
//     string's height (from its modes) is compared with the obstacle's
//     surface; where it is below, a stiff one-sided spring pushes back
//     (Hertz-like, force ~ penetration^1.5) into the modes. The string wraps
//     on and off the curved bridge once per period: the buzz.
//   - Quadratic coupling (a steel pan's note: a shallow dome): the energy of
//     the fundamental flows into the octave and the twelfth after the hit
//     (forces from the potential a x1^2 x2 + b x1 x2 x3).
//   - Tension glide: a hard hit or pluck stretches the vibrator; every mode
//     runs a little faster while the amplitude is high (a taiko's thump).
//   - The excitation: a pluck (a force at a point that rises and lets go:
//     finger, plectrum, mizrab, tsume, thumbnail), a mallet (a mass with a
//     Hertz contact spring: stick, bachi, rubber beater, clapper), or a bow
//     (stick-slip friction solved each sample, as the double bass's).
//
// The sound: the body's (and each vibrator's own) radiating velocity,
// differentiated (a monopole's pressure ~ the volume acceleration); two
// projections for left and right.
#pragma once

#include <cstdint>

namespace pajupilli {

class ModalSet {
 public:
  static constexpr int kMaxVib = 32, kModes = 64, kBody = 24, kContacts = 4;
  explicit ModalSet(double fs);
  double fs() const { return fs_; }

  // A vibrator's modes. freq: Hz; t60: s (each mode's own); n <= kModes.
  // exc: each mode's shape at the excitation point (unit modal mass);
  // bridge: the force on the bridge per unit of the mode's y (N/m: a
  // string's tension x its slope there, k pi T / L); back: how the bridge's acceleration drives the mode; rad: its
  // own radiation (for things with no body); side: left minus right.
  struct Def {
    int n = 0;
    float freq[kModes] = {}, t60[kModes] = {}, exc[kModes] = {}, bridge[kModes] = {}, back[kModes] = {},
          rad[kModes] = {}, side[kModes] = {};
    float modalMass = 1e-3f;  // kg: forces are divided by it
    float split = 0;          // z's modes: their frequency x (1 + split * a +-1 per mode)
    float zShare = 0.3f;      // the bridge feels z this much (its rocking)
    bool one = false;         // y only (a sympathetic string, a drone: z left out, half the work)
    // Contact: up to kContacts patches, each mode's shape there (a patch: a
    // short cosine-weighted stretch), the obstacle's height (m, the string at
    // rest 0, the obstacle below: negative), its stiffness (N/m, linear).
    int nc = 0;
    float cShape[kContacts][kModes] = {}, cHeight[kContacts] = {};
    float cK = 0;
    // Quadratic coupling (a steel pan's note, its shell's curvature: the
    // resonant terms of 1 : 2 : 3 or 1 : 2 : 4, tools/steelpan.py): up to
    // kQuad terms x_p'' += -g x_i x_j (per unit modal mass).
    static constexpr int kQuad = 6;
    int nq = 0;
    struct Quad { int p, i, j; float g; } quad[kQuad] = {};
    // Tension glide: every mode x sqrt(1 + glide * (amplitude / glideRef)^2).
    float glide = 0, glideRef = 1e-3f;
  };
  void setVib(int i, const Def& d);
  const Def& def(int i) const { return vib_[i].def; }
  int vibs() const { return nVib_; }
  // Every mode of vibrator i x ratio (a string pressed behind the bridge, a
  // meend, a stopped length; a retuned pan note), at once or over `glide` s.
  void retune(int i, double ratio, double glide = 0);
  double ratio(int i) const { return vib_[i].ratio; }
  // Vibrator i at rest at once (a pool's bell taken for another note).
  void silence(int i);
  // Its excitation point's shape (a different place to pluck or strike).
  void setExcitation(int i, const float* shape);

  // The body at the bridge: its modes (Hz, t60 s), each one's mass at the
  // bridge (kg), radiation and side weights.
  void setBody(int n, const float* freq, const float* t60, const float* mass, const float* rad, const float* side);

  // Pluck vibrator i: a force (N) rising over `rise` s, held `hold` s, let go
  // over `slip` s; angle: the pull's direction from z towards y (rad).
  void pluck(int i, double force, double rise, double hold, double slip, double angle = 0.6);
  // A mallet: mass (kg), speed (m/s), contact stiffness K (N/m^alpha), alpha.
  void strike(int i, double mass, double speed, double K, double alpha = 1.5);
  // The bow on vibrator i: its speed (m/s), force (N); exc is the shape under
  // the hair (set it with setExcitation). force 0: off the string.
  void bow(int i, double speed, double force, double rise = 0.03);
  bool bowing(int i) const { return vib_[i].bowOn; }
  // A force on the bridge from outside the set this sample (N; a bowed
  // waveguide string's): added to the vibrators' push.
  void push(float F) { extF_ += F; }
  // A knock on the body at the bridge: a half-sine force (N) lasting dur s,
  // after delay s (a shamisen's bachi meeting the skin after the string).
  void knock(double force, double dur, double delay = 0);
  // A damper (a finger, the hand, a felt): every mode of i dies at `rate` 1/s (0: off).
  void damp(int i, double rate);
  // The vibrator's energy (per unit modal mass; tests).
  double energy(int i) const;

  // One sample: the sound (and its side signal, left = out + side).
  float sample();
  float side() const { return side_; }
  void reset();

 private:
  struct Vib {
    Def def;
    alignas(64) float x[2][kModes] = {}, v[2][kModes] = {};
    alignas(64) float c00[2][kModes] = {}, c01[2][kModes] = {}, c10[2][kModes] = {}, c11[2][kModes] = {};
    alignas(64) float exc[kModes] = {};
    float cG[kContacts][kContacts] = {};  // contact patches: height per unit force over one step
    double ratio = 1, ratioFrom = 1, ratioTo = 1, glideT = 1, glideDur = 0, glideNow = 1;
    double dampRate = 0;
    float dampMul = 1;
    // Pluck.
    double pF = 0, pRise = 0, pHold = 0, pSlip = 0, pT = 1e9, pAngle = 0.6;
    // Mallet.
    bool mOn = false;
    double mMass = 0, mZ = 0, mV = 0, mK = 0, mAlpha = 1.5, mT = 0;
    // Bow.
    bool bowOn = false, slipping = true;
    float bowV = 0, bowF = 0, bowVTo = 0, bowFTo = 0, bowK = 0.01f, bowSum = 0, bowNoise = 0;
    bool awake = false;
    int quiet = 0;
    int nv = 16;  // the modes stepped: n rounded up to a vector
  };
  void coefs(Vib& v);  // the modes' step matrices at the present tuning and damping
  double fs_, dt_;
  Vib vib_[kMaxVib];
  int nVib_ = 0;
  // Body.
  int nb_ = 0;
  alignas(64) float bx_[kBody] = {}, bv_[kBody] = {}, b00_[kBody] = {}, b01_[kBody] = {}, b10_[kBody] = {},
                    b11_[kBody] = {}, bInvM_[kBody] = {}, bRad_[kBody] = {}, bSide_[kBody] = {};
  float bridgeA_ = 0;  // the bridge's acceleration (last sample)
  double kickF_ = 0, kickDur_ = 0, kickT_ = 1e9;
  float extF_ = 0;
  double prevOut_ = 0, prevSide_ = 0;
  float side_ = 0;
  int ctl_ = 0;
  uint32_t rng_ = 99991;
  float urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0f / 16777216.0f) - 1; }
};

// Mode shapes for the usual vibrators (helpers for the patches).
// A string of length L fixed at both ends (bridge at x = 0): mode k (1-based) at x.
double stringShape(int k, double x, double L);
// A cantilever (clamped at 0, free at L): the k-th mode's shape at x (1-based
// k, normalised to +-2 at the tip), and its frequency ratio to the first.
double cantileverShape(int k, double x, double L);
double cantileverRatio(int k);

}  // namespace pajupilli
