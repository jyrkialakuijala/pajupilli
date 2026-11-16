// Struck bars, tubes and teeth: the chromatic percussion (General MIDI 9 ..
// 15) on one engine. An instrument here is a row of sounding pieces, one per
// key, each with its own modes, what strikes it, and (some) a resonator and
// a damper; the designs say what each is:
//
//   celesta        steel plates on felt, a tuned wooden box under each, felt
//                  hammers, dampers and a pedal (research/celesta.md);
//   glockenspiel   thick steel bars, bare, hard mallets: every overtone of
//                  the free bar rings on (1 : 2.76 : 5.40 : 8.93);
//   marimba        rosewood bars carved thin in the middle so that the
//                  second mode sounds two octaves over the first, a tuned
//                  tube under each, yarn mallets;
//   xylophone      rosewood bars carved for a twelfth (1 : 3), hard mallets,
//                  short tubes: a bright clack and a short note;
//   tubular-bells  brass tubes hung by one end and struck on the other with
//                  a rawhide hammer: no mode sounds at the pitch heard; the
//                  fourth, fifth and sixth lie as 2 : 3 : 4.2 and the ear
//                  supplies the note an octave under the fourth;
//   music-box      a steel comb's teeth, clamped at one end (1 : 6.27 :
//                  17.5), not struck but plucked: a pin bends the tooth and
//                  lets it go; the box it is screwed to is what sounds.
//
// (research/chromatic_percussion.md; the vibraphone is src/modalinst.cc's.)
//
// Per key, in the engine: up to nine modes, each with its share under the
// mallet (and how that share changes as the mallet lands a little
// elsewhere, stroke by stroke), its loss, its share under the damper, its
// push on the resonator's air and its sound at the microphones; the
// resonator's one air mode; the mallet, a mass on a felt, yarn or hard
// contact, its force found against the piece's own give (it meets the piece
// where it is: a second blow on a ringing bar may add to its swing or take
// it away); or a pluck.
#pragma once

#include <cstdint>
#include <vector>

namespace pajupilli {

struct BarKey {
  static constexpr int kModes = 9;
  double f1 = 440;             // the pitch (Hz): the first mode's, or what the ratios are against
  int modes = 0;
  double ratio[kModes] = {};   // each mode's frequency / f1
  double hit[kModes] = {};     // its share under the mallet (mass-normalised, 1/sqrt(kg))
  double hitVary[kModes] = {}; // ... and what is added x a number in -1 .. 1, new for each stroke
  double loss[kModes] = {};    // its own loss (1/s)
  double damp[kModes] = {};    // the damper's on it, resting (1/s)
  double couple[kModes] = {};  // its push on the resonator's air and the air's on it (1/s^2)
  double rad[kModes] = {};     // its sound at a microphone, Pa per unit acceleration
  bool resonator = false;
  double resF = 440, resLoss = 30, resRad = 0;  // the air: Hz, 1/s, Pa per unit acceleration
  double mallet = 0.02, K = 1e8, p = 1.5, hyst = 0;  // kg; N/m^p; more force pressing than parting (s/m)
  bool pluck = false;          // not struck: bent by `K` N per m/s of the key's speed, and let go
  double x = 0;                // where it stands, m from the middle (the bass to the left)
};

struct BarDesign {
  static constexpr int kMaxKeys = 72;
  const char* name = "celesta";
  int keys = 0, lowMidi = 48;
  BarKey key[kMaxKeys];
  bool dampers = false;        // a damper on each key, lifted by the key and by the pedal
  double damperTime = 0.03;    // ... coming to rest over this (s)
  // What the blows do to the case or frame: up to four short resonances
  // (Hz, s to -60 dB, strength), driven by the mallets' force.
  double knockF[4] = {}, knockT[4] = {}, knockG[4] = {};
  double micDist = 1.0, micApart = 0.6;
  double speedLo = 0.4, speedHi = 3.0;  // the mallet's speed at the softest and the hardest key (m/s)
};

BarDesign celestaDesign();
BarDesign glockenspielDesign();
BarDesign marimbaDesign();
BarDesign xylophoneDesign();
BarDesign tubularBellsDesign();
BarDesign musicBoxDesign();
// By the instrument's name; false if it is none of these.
bool barDesignFor(const char* name, BarDesign* d);

class Bars {
 public:
  static constexpr int kMaxKeys = BarDesign::kMaxKeys, kModes = BarKey::kModes;
  Bars(double fs, const BarDesign& design);
  int keys() const { return n_; }
  int lowMidi() const { return d_.lowMidi; }
  const BarDesign& design() const { return d_; }
  // Key 0 .. keys() - 1. The mallet arrives at `speed` (m/s).
  void strike(int key, double speed);
  void damper(int key, bool down);
  float sample();              // the two microphones' mean
  float side() const { return side_; }
  void reset();
  bool asleep() const { return nActive_ == 0 && !knockOn_; }
  double frequency(int key) const { return d_.key[key].f1; }

 private:
  struct Key {
    int nm = 0;
    float q[kModes + 1] = {}, v[kModes + 1] = {};
    float w2[kModes + 1] = {}, alpha[kModes + 1] = {};
    float a11[kModes + 1], a12[kModes + 1], a21[kModes + 1], a22[kModes + 1];
    float hit[kModes] = {}, damp[kModes] = {}, couple[kModes] = {}, rad[kModes + 1] = {};
    float gainL = 1, gainR = 1;
    float G = 0;
    double hz = 0, hv = 0;
    bool hammerOn = false;
    float dPrev = 0;
    float engage = 0;
    bool damperDown = false, active = false;
    int quiet = 0, tick = 0;
  };
  void coefs(Key& k, const BarKey& b);
  BarDesign d_;
  double fs_, dt_;
  int n_ = 0;
  Key key_[kMaxKeys];
  int active_[kMaxKeys], nActive_ = 0;
  float prevL_ = 0, prevR_ = 0, side_ = 0;
  struct Case { float x = 0, v = 0, a11, a12, a21, a22, gain; };
  std::vector<Case> case_;
  float knockIn_ = 0;
  bool knockOn_ = false;
  uint32_t rng_ = 2024;
};

}  // namespace pajupilli
