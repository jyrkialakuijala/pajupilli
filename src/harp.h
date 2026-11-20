// The concert (pedal) harp (research/harp.md): 47 strings, C1 to G7, on one tapered soundboard
// over a box with five holes in its back. The harpist is src/harpinst.h.
//
//   - Each string (tools/harpboard.py: the Elysian's lengths and gut gauges, Camac's wires,
//     Galli's tensions): nylon at the top (F6-G7), gut in the middle (A2-E6), steel wound with
//     silvered copper in the bass (C1-G2); tuned flat on its full length (all pedals up: C-flat
//     major), the three without discs (C1, D1, G7) by hand to their naturals. Its modes up to
//     8 kHz (the wires: 6 kHz; at most 128) in two directions: in the strings' plane, where the
//     string pulls the board (at 35 degrees to it: cos 35 of its swing moves the board, and the
//     board moves its end as much), and across the plane, which only the board's turning under
//     the eyelet takes (10-15 dB less). Across, each partial its own 0.05-0.3 % sharper than in
//     the plane (the note between them): the two directions drift apart, and a string plucked
//     between them swings in an ellipse that collapses and swells. (A loud pluck on a short
//     string, its tension swinging, hands energy from one direction to the other: it whirls.)
//   - What a string loses by itself (2.4): to the air (Stokes: the thinner and faster, the more)
//     and inside its material as it bends (gut and nylon: the bending share of each partial's
//     energy times the material's loss factor, rising as n^2: some ten partials are left);
//     the wires lose little themselves. Gut is alive: each partial a few cents off and its loss
//     +-25 %. Its ends give (the neck's pin and discs, the eyelet: a resistance, every mode alike,
//     fitted to the one string measured). The rest of a string's decay is what the board takes.
//   - The discs: a pedal turns the discs of its letter in every octave: the natural disc
//     shortens the string by a semitone (5.6 %), the sharp disc by another; the string's modes
//     retuned where they stand (a pedal moved under a sounding string bends it a semitone). A disc
//     pinches: a little more loss than the bare pin (x1.1, estimated).
//   - The stretch: a swinging string is longer, so tighter: the tension rises by EA/L of the
//     stretch, every sample. That raises every mode's pitch a little while the swing is large
//     (a loud note starts sharp and settles), and the tension's swing, at twice each partial and
//     at their sums, pulls on the board along the string (sin 35 of it): the "phantom partials"
//     that fill in what a pluck in the middle leaves out, more the harder the pluck.
//   - The board (tools/harpboard.py): its resonances computed on a grid (spruce, 1.42 m, 50 to 10
//     cm wide, 11.5 to 2.25 mm, the grain across; the string bar; the strings' own stiffness at
//     their points), coupled with the box's air through the five holes and fitted to what was
//     measured (T1 153.5 Hz, A0 172 Hz, the air's share of A0, an added mass's effect); broad ones
//     above 2.4 kHz. Each string meets it at its own point on the centre line: every
//     string feels what every other gives the board, and those whose partials meet ring along:
//     the halo. Nothing is damped unless a hand does it.
//   - The finger (5.1-5.2): its pad (2 cm wide) takes the string where it is (stopping what it
//     sounded), pulls it aside `depth` at `at` of its length from the board, mostly in the
//     strings' plane (`angle` off it), and lets it slip: the string slides on with the finger a
//     little further, then off the pad (within a fifth of its period): it leaves with both
//     displacement and speed (here 10-60 % of its energy kinetic as it leaves, by string and
//     velocity; Chadefaux measured 15-37 %).
//   - Hands lying on strings (6.4): a palm (a long contact: every mode) or a finger put back
//     (a short one at the pluck); a touch at the middle for a harmonic, lifted as the thumb lets go.
//   - The sound: the board in patches (5 cm up it, each side of the bar) and the five holes' air,
//     each a small source heard after its own distance by two microphones 2 m in front of the
//     board (scaled to 1 m): each partial goes out in its own pattern.
#pragma once

#include <cstdint>
#include <vector>

namespace pajupilli {

class Harp {
 public:
  static constexpr int kStrings = 47, kMaxModes = 128;
  explicit Harp(double fs);

  // A pluck: the finger's pad on string s.
  struct Pluck {
    double depth = 0.004;   // m: how far it holds the string aside before it slips
    double at = 0.40;       // where: the share of the sounding length from the board
    double angle = 0.4;     // rad off the strings' plane (0: in it, towards the board's normal)
    double width = 0.02;    // m: the pad on the string
    double pull = 0.006;    // s: taking the string and drawing it out
    double slip = 0.003;    // s: sliding with the finger
    double off = 0.0005;    // s: leaving the pad's edge
    double carry = 0.001;   // m: how much further the finger carries it while it slips
    double stop = 1;        // how hard the pad stops what the string sounded (x)
    double harmonic = 0;    // a touch at the middle, lifted this long after the release (s; 0: none)
  };
  void pluck(int s, const Pluck& p);
  // A hand on string s: its fundamental dies at `rate` (1/s), each mode by its shape under the
  // contact (`len` m long, its middle `at` of the length from the board); 0: lifted.
  void damp(int s, double rate, double at = 0.42, double len = 0.08);
  bool damped(int s) const { return str_[s].dampRate > 0; }
  // The pedal of a letter (0 C .. 6 B): -1 flat, 0 natural, 1 sharp. Every string of that letter
  // but the three without discs.
  void pedal(int letter, int pos);
  int pedalOf(int letter) const { return pedal_[letter]; }
  // Which MIDI pitch string s sounds now; its natural pitch; whether it follows the pedals.
  int sounds(int s) const;
  static int natural(int s);
  static bool hasDiscs(int s) { return s != 0 && s != 1 && s != kStrings - 1; }
  double frequency(int s) const { return str_[s].f1; }  // its fundamental as stopped now
  double length(int s) const { return str_[s].len; }
  int modes(int s) const { return str_[s].n; }
  int totalModes() const;
  // The string's energy now (J), as last counted (every 256 samples).
  double energy(int s) const { return str_[s].energy; }
  double energyInPlane(int s) const { return str_[s].energyPlane; }  // (of it, the swing in the strings' plane)
  bool fingerOn(int s) const { return str_[s].clock < 1e8; }

  // The two microphones' mean (Pa, as at 1 m); side(): half their difference.
  float sample();
  float side() const { return side_; }
  void reset();
  bool asleep() const { return asleep_; }

 private:
  struct Str {
    int n = 0;                  // its modes (a multiple of 16)
    int pos = -1;               // its disc: -1 flat (none), 0 natural, 1 sharp
    double full = 1, len = 1, f1 = 100, B = 0, top = 8000;
    double mu = 0, T = 0, EA = 0;
    float invMass = 0;          // 2 / (mu L): the force on a mode, per unit of its shape
    alignas(64) float x[2][kMaxModes] = {}, v[2][kMaxModes] = {};   // x: displacement x omega
    alignas(64) float cs[2][kMaxModes] = {}, sn[2][kMaxModes] = {}; // the turn per sample, the own loss in
    alignas(64) float w[kMaxModes] = {};                            // omega (in the plane)
    alignas(64) float in[kMaxModes] = {}, out[kMaxModes] = {};     // the end's motion into x; momentum to the board
    alignas(64) float push[2][kMaxModes] = {};                      // the finger's force into v (x dt)
    alignas(64) float stretch[kMaxModes] = {};                      // the tension's rise per x^2
    alignas(64) float hand[kMaxModes] = {};                         // a hand's or a touch's keep per sample
    float split[kMaxModes] = {}, alive[kMaxModes] = {}, cents[kMaxModes] = {};
    // The finger.
    double clock = 1e9, force = 0, pull = 0.006, slip = 0.003, off = 0.0005, carry = 0, stop = 0, harmonic = 0;
    double holdRate = 0;
    // A hand.
    double dampRate = 0, dampAt = 0.42, dampLen = 0.08;
    bool touching = false;      // a harmonic's touch on it
    bool handOn = false;        // hand[] is in force
    double energy = 0, energyPlane = 0;
    float eps = 0;              // the tension's rise now (share of T)
    bool loud = false;          // its stretch is worth computing
  };
  void tune(int s);
  void hands(int s);
  void pushShape(Str& st, double at, double width, double angle);

  double fs_, dt_;
  Str str_[kStrings];
  int pedal_[7] = {-1, -1, -1, -1, -1, -1, -1};
  // The board: per mode, its state (x: displacement x omega), turn, coupling, sound.
  int nB_ = 0;
  std::vector<float> bx_, bv_, bcs_, bsn_, binv_, bf_, bd_, bacc_;
  std::vector<float> phi_, side_phi_;   // [string][mode]: x cos 35 (the plane's swing); across
  // The air: how much each patch of the board (and each hole) moves it per mode, the patches'
  // last 10 ms, and each microphone's delay and gain from each.
  static constexpr int kRing = 512;
  int nP_ = 0, ringPos_ = 0;
  std::vector<float> pvol_, ring_;      // [patch][mode], [patch][kRing]
  std::vector<int> tapI_[2];
  std::vector<float> tapF_[2], tapG_[2];
  float accT_[kStrings] = {}, accS_[kStrings] = {};  // how far the board moved each string's end, last sample
  float sinT_ = 0;
  float side_ = 0;
  float kOut_ = 1, kCouple_ = 1;
  bool asleep_ = true;
  int quiet_ = 0, clock_ = 0;
};

}  // namespace pajupilli
