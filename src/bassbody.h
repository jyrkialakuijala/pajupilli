// The double bass's body in real time (research/double_bass.md; designed by
// tools/bassbody.py, which writes bassbody_data.h):
//
//   - the top and the back plate, each a 16 x 8 grid of masses in the outline
//     of a bass (the masses outside it stay at rest: the ribs hold the edge);
//     the bass bar is in the top's masses and stiffness along its line;
//   - the sound post: rigid (its mass in the plates' masses), from the top, a
//     little behind the bridge's treble foot, to the back: its two ends move
//     together, and it passes whatever force that takes;
//   - the air inside: a column of 16 cells along the body, each with its own
//     pressure, the air moving between them (the body's standing waves); the
//     f-holes' plugs open from the bridge's cell.
//
// Each plate is three sets of 8 rows of 16 float32s (one AVX-512 register a
// row): displacement, velocity, and the forces collected each sample (the
// plate's stiffness from 13 neighbours, the feet, the post, the air). Then
// v += F / m dt, w += v dt (leapfrog; the grid's highest resonance, 7.2 kHz,
// is well under the scheme's limit at 48 kHz, fs / pi).
#pragma once

#include "bassbody_data.h"

namespace pajupilli {

// A body's design (tools/bassbody.py -emit): the double bass (bassbody_data.h), the cello
// (cellobody_data.h, tools/bassbody.py -cello) or the viola (violabody_data.h, -viola), the same grid.
struct BodyData {
  const float (*topK)[bassdata::kNY][bassdata::kNX];
  const float (*topInvM)[bassdata::kNX];
  const float (*backK)[bassdata::kNY][bassdata::kNX];
  const float (*backInvM)[bassdata::kNX];
  float nodeArea;
  const float* rowY;
  const float* colX;
  const float* airK;
  const float* slugArea;
  float slugInv, plugArea, plugInvM, postBehind;
  int bridgeI, bassJ, trebleJ;
  // The two sides' difference: up to where the top is small against the wavelength, and where
  // its patches average out (Hz; the bass 250 and 700, a smaller body higher).
  float sideLo, sideHi;
  // Steps per sample: a small body's grid (the viola's) has resonances up to ~15 kHz, at the
  // leapfrog's limit at 48 kHz (fs / pi): two steps of half the time each hold it to 30 kHz.
  int substeps = 1;
  float depth = 0.20f;  // the ribs' height (m): the top's and the back's distance, the air's passage across
  // The air's losses that grow with frequency (s): its motion is driven by the pressure plus
  // this x the pressure's rate of change, which damps a resonance at f to Q = 1 / (2 pi f x this)
  // at most. What the f-holes radiate grows with the frequency squared, and so does what the
  // air loses on the walls of a narrow body. Without it the one slice of air at the f-holes
  // bounces on their plugs with almost no loss (the viola: at 4.9 kHz, Q over 1000): every note
  // with an overtone there (D#4, E4, F4, B4, F5) rang with a metallic shine. 0: none.
  float airHf = 0;
  // The f-holes are long: each opens from this many cells of its half of the air (1: the
  // bridge's cell alone; 3: that and its two neighbours, a quarter, a half and a quarter of the
  // hole: a viola's 78 mm holes over its 26 mm cells). Opening from one thin slice, the hole's
  // air and the slice made a resonance of their own (Viola.md, finding 7).
  int holeCells = 1;
};
extern const BodyData kBassBodyData, kCelloBodyData, kViolaBodyData, kViolinBodyData;

class BassBody {
 public:
  explicit BassBody(double fs, const BodyData& data = kBassBodyData);
  void reset();
  // One sample: the bridge feet's forces on the top (N, outward positive).
  // Returns the body's radiated sound: the acceleration of the volume it
  // displaces (top + back + f-holes: a monopole).
  float step(float bassFoot, float trebleFoot) {
    if (d_.substeps == 1) return stepOne(bassFoot, trebleFoot);
    float a = 0, h = 0;
    for (int k = 0; k < d_.substeps; ++k) {
      a += stepOne(bassFoot, trebleFoot);
      h += fhole_;
    }
    fhole_ = h / float(d_.substeps);
    return a / float(d_.substeps);
  }
  // The feet's motion (for the bridge): displacement and velocity.
  float bassFootW() const { return w_[0][d_.bassJ + kPad][d_.bridgeI + kPad]; }
  float trebleFootW() const { return w_[0][d_.trebleJ + kPad][d_.bridgeI + kPad]; }
  float bassFootV() const { return v_[0][d_.bassJ + kPad][d_.bridgeI + kPad]; }
  float trebleFootV() const { return v_[0][d_.trebleJ + kPad][d_.bridgeI + kPad]; }
  // What a close microphone at an f-hole hears: the air plugs' own flow's
  // acceleration (not cancelled by the plates, as it is far away).
  float fholeSound() const { return fhole_; }
  // What differs between the two sides of the bass (add to one ear, take
  // from the other): see step().
  float sideSound() const { return side_; }
  // How the body's motion lies in space: the moments of the volume it displaces (top, back and
  // f-holes), as the sound each sends along its axis over and above the whole volume's (the
  // return value of step(), heard alike all round): k = 0 across the body (towards the treble
  // side), 1 along it (towards the tail), 2 out of the top (the top and the f-holes against the
  // back). A resonance that moves the two halves of the top against each other, or pumps the air
  // in at one f-hole and out at the other, displaces no volume: it is heard only through these,
  // with opposite signs to its two sides. Each resonance has its own share of the four, so each
  // overtone of a note goes out into the room in its own directions.
  float dipole(int k) const { return dip_[k]; }
  // Losses: a decay rate for all motion (1/s), and one that grows with
  // frequency (the wood's internal friction: a share of the stiffness force,
  // as its rate of change; s).
  void setDamping(double rate, double stiff, double air = 14) {
    rate_ = float(rate); stiff_ = float(stiff); airRate_ = float(air);
  }
  double dampRate() const { return rate_; }
  double dampStiff() const { return stiff_; }
  double dampAir() const { return airRate_; }
  // Another instrument of the same design: its plates this much stiffer (x; its wood
  // resonances move by the square root of it) and its air (the ribs a little higher or lower).
  void setVariation(double plates, double air) { plateK_ = float(plates); airKs_ = float(air); }

 private:
  const BodyData& d_;
  float stepOne(float bassFoot, float trebleFoot);
  static constexpr int kPad = 2;
  static constexpr int kRows = bassdata::kNY + 2 * kPad, kCols = bassdata::kNX + 2 * kPad;
  // [plate: 0 top, 1 back][row][column], padded with masses at rest.
  alignas(64) float w_[2][kRows][kCols] = {};
  alignas(64) float v_[2][kRows][kCols] = {};
  alignas(64) float f_[2][bassdata::kNY][bassdata::kNX] = {};
  alignas(64) float fPrev_[2][bassdata::kNY][bassdata::kNX] = {};
  float fhole_ = 0, side_ = 0, side1_ = 0, dipPrev_ = 0, sideA_ = 0, sideB_ = 0;
  // The air in two halves side by side, the bass side's and the treble side's ([side]): each a
  // column of cells with its own f-hole's plug, and the air free to cross between the halves.
  // The air slugs between the cells: [i + 1] between cells i and i + 1 ([0] and [16]: walls).
  float plugW_[2] = {}, plugV_[2] = {};
  float slugW_[2][bassdata::kNX + 1] = {}, slugV_[2][bassdata::kNX + 1] = {};
  float crossW_[bassdata::kNX] = {}, crossV_[bassdata::kNX] = {}, crossInv_[bassdata::kNX] = {};  // from the bass side to the treble side
  float crossArea_ = 0, holeY_ = 0;
  bool sides_ = true;  // (PJ_AIR_SIDES 0: one column of air and one plug for both f-holes, as before)
  float dip_[3] = {}, dip1_[3] = {}, momPrev_[3] = {};
  float plateK_ = 1, airKs_ = 1;
  float pressPrev_[2][bassdata::kNX + 1] = {};
  float dt_, rate_ = 18, stiff_ = 3e-6f, airRate_ = 14;
};

}  // namespace pajupilli
