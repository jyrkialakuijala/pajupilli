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

class BassBody {
 public:
  explicit BassBody(double fs);
  void reset();
  // One sample: the bridge feet's forces on the top (N, outward positive).
  // Returns the body's radiated sound: the acceleration of the volume it
  // displaces (top + back + f-holes: a monopole).
  float step(float bassFoot, float trebleFoot);
  // The feet's motion (for the bridge): displacement and velocity.
  float bassFootW() const { return w_[0][bassdata::kBassJ + kPad][bassdata::kBridgeI + kPad]; }
  float trebleFootW() const { return w_[0][bassdata::kTrebleJ + kPad][bassdata::kBridgeI + kPad]; }
  float bassFootV() const { return v_[0][bassdata::kBassJ + kPad][bassdata::kBridgeI + kPad]; }
  float trebleFootV() const { return v_[0][bassdata::kTrebleJ + kPad][bassdata::kBridgeI + kPad]; }
  // What a close microphone at an f-hole hears: the air plugs' own flow's
  // acceleration (not cancelled by the plates, as it is far away).
  float fholeSound() const { return fhole_; }
  // What differs between the two sides of the bass (add to one ear, take
  // from the other): see step().
  float sideSound() const { return side_; }
  // Losses: a decay rate for all motion (1/s), and one that grows with
  // frequency (the wood's internal friction: a share of the stiffness force,
  // as its rate of change; s).
  void setDamping(double rate, double stiff, double air = 14) {
    rate_ = float(rate); stiff_ = float(stiff); airRate_ = float(air);
  }

 private:
  static constexpr int kPad = 2;
  static constexpr int kRows = bassdata::kNY + 2 * kPad, kCols = bassdata::kNX + 2 * kPad;
  // [plate: 0 top, 1 back][row][column], padded with masses at rest.
  alignas(64) float w_[2][kRows][kCols] = {};
  alignas(64) float v_[2][kRows][kCols] = {};
  alignas(64) float f_[2][bassdata::kNY][bassdata::kNX] = {};
  alignas(64) float fPrev_[2][bassdata::kNY][bassdata::kNX] = {};
  float plugW_ = 0, plugV_ = 0, fhole_ = 0, side_ = 0, side1_ = 0, dipPrev_ = 0, sideA_ = 0, sideB_ = 0;
  // The air slugs between the cells: [i + 1] between cells i and i + 1 ([0] and [16]: walls).
  float slugW_[bassdata::kNX + 1] = {}, slugV_[bassdata::kNX + 1] = {};
  float dt_, rate_ = 18, stiff_ = 3e-6f, airRate_ = 14;
};

}  // namespace pajupilli
