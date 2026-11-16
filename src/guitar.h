// A guitar (research/guitars.md): six strings over frets, a bridge on a
// body, the fingers of the right hand. The first of them: the classical
// guitar, nylon strung (classicalGuitar()).
//
//   - Each string: its lowest 48 modes in each of two directions (towards
//     the top plate and along it). A fretted string ends on its fret: the
//     same string, shorter.
//   - All six end on one bridge, and the bridge stands on the body: its
//     motion moves every string's end (the strings not played ring along),
//     and takes a string's sound into the body. Towards the top the bridge
//     gives readily: that part of a note is loud and soon over. Along the
//     top it gives little: that part rings on quietly. How a string is
//     plucked sets the mix (a rest stroke goes into the top; a free stroke
//     along it): the note's two-stage decay.
//   - The body: the air in the box breathing through the soundhole with the
//     top (near 100 Hz), the top's own first resonance (near 200), the
//     back's, the next air mode, and some twenty more of the plates up to
//     3 kHz, each a mass under the bridge with its own share of the sound.
//   - A pluck: a finger takes the string where it stands (stopping what it
//     still sounded), pulls it aside, and lets it go: first off the flesh
//     (wide and slow: the round part of the tone), then off the nail's edge
//     (narrow and quick: the bright part). Where along the string, how
//     deep, at what angle, how much nail.
//   - Damping: a hand or a lifting finger laid on a string.
#pragma once

#include <cstdint>
#include <vector>

namespace pajupilli {

struct GuitarString {
  double f0;        // open (Hz)
  double tension;   // N
  double B;         // inharmonicity: f_k = k f0 sqrt(1 + B k^2)
  double loss0, loss1;  // 1/s: loss0 + loss1 k^1.2
};

struct GuitarSpec {
  const char* name = "nylon-guitar";
  double scale = 0.650;        // nut to saddle (m)
  int frets = 19;
  GuitarString str[6];         // low E first
  // The body's resonances under the bridge: Hz, quality, the mass the bridge
  // feels (kg), its share of the sound (+-; 1: the top's first resonance),
  // and of what differs between the two microphones.
  // along: how much of a string's motion along the top this resonance takes
  // (and gives back), against its motion towards the top: the breathing
  // modes hardly any, the ones that rock the bridge much, either way.
  struct Mode { double f, q, mass, rad, side, along; };
  std::vector<Mode> body;
  // The strings' own losses: all (x); the 2nd and 3rd partial's (x again); the 4th and 5th's (x again, less).
  double ring = 0.8, ringOvertones = 0.55, ringOvertones45 = 0.78;
  double alongDetune = 0.0015; // along the top a string sounds this much sharper (its ends give less)
  double radiate = 4.8e-3;     // Pa at the microphones per m/s^2 of the top's first resonance
};
GuitarSpec classicalGuitar();  // with the PJ_GUITAR_* knobs

class Guitar {
 public:
  static constexpr int kStrings = 6, kModes = 48;
  explicit Guitar(double fs, const GuitarSpec& spec = classicalGuitar());
  int frets() const { return s_.frets; }
  double open(int s) const { return s_.str[s].f0; }
  // The left hand: string s (0: low E) stopped at a fret (0: open), at once.
  void fret(int s, int fret);
  // The right hand: pull string s aside by `depth` (m) at `fromBridge` (m),
  // `angle` from the top's plane (rad: 0 along the top, 1.57 straight into
  // it), `nail` 0 (flesh alone) .. 1 (nail alone), and let it go.
  // strum (1 down, 2 up; 0: a plucking finger): not a finger that takes the
  // string first, but a hand passing over it: flesh and nail together brush
  // the string as it is (still ringing, if it was). They are broad and
  // soft where a plucking nail's edge is narrow and quick: the string leaves
  // them in a rounded shape, with few of its high overtones: warm, thick,
  // velvety; the fingertip's pad coming up most of all.
  // clack: 0 .. 1, the nail striking the string as the finger takes it, a
  // short knock before the pull (the hardest strokes; flamenco's): it adds
  // to the upper middle of the tone, not to the very top.
  void pluck(int s, double depth, double fromBridge, double angle, double nail, int strum = 0, double clack = 0);
  // The left hand pushes string s sideways along its fret: it is stretched,
  // and sounds this many cents sharper (0: let back).
  void bend(int s, double cents);
  // Something lying on string s: its sound dies at `rate` (1/s); 0: lifted.
  void damp(int s, double rate);
  float sample();
  float side() const { return side_; }
  void reset();
  bool asleep() const { return asleep_; }
  double frequency(int s) const;  // as stopped now

 private:
  struct Str {
    int fret = 0;
    double len = 0.65, f1 = 110, bend = 0;
    alignas(64) float q[2][kModes] = {}, v[2][kModes] = {};
    alignas(64) float cs[2][kModes], sn[2][kModes], w[2][kModes], decay[2][kModes];
    alignas(64) float slope[kModes], bridgeIn[kModes], shape[kModes], shapeN[kModes];  // (the flesh's width on the string, and the nail's)
    float invMass = 0;
    // The finger: planted, pulling, then letting go off the flesh and off the nail.
    double clock = 1e9, force = 0, sinA = 0, cosA = 1, nail = 0.5, fromBridge = 0.12;
    // This stroke's rise, its slips off the flesh and off the nail (s), the
    // finger's damping while it lies on (1/s), the flesh's and the nail's width on the string (m).
    double plant = 0.005, flesh = 0.0022, nailSlip = 0.0005, hold = 300, wFlesh = 0.009, wNail = 0.0015, clack = 0;
    double dampRate = 0;
    bool active = false;
  };
  void tune(Str& st, int index);
  void shapeAt(Str& st, float* shape, double fromBridge, double width, double apart = 0);
  GuitarSpec s_;
  double fs_, dt_;
  Str str_[kStrings];
  struct Body { float x = 0, v = 0, a11, a12, a21, a22, invMass, rad, side, along; };
  std::vector<Body> body_;
  float bridgeV_ = 0, bridgeVz_ = 0, side_ = 0;
  bool asleep_ = true;
  int quiet_ = 0;
  static constexpr double kPlant_ = 0.005, kFlesh_ = 0.0022, kNail_ = 0.0005;  // s: the finger's rise; its two slips
};

}  // namespace pajupilli
