// A drum head: a round membrane under tension, as its modes.
//
//   - The modes are the ideal membrane's: (m, n), m nodal diameters and n
//     nodal circles, at j_mn c / (2 pi R) (j_mn: the n-th zero of the Bessel
//     function J_m; c = sqrt(tension / areal density)), shaped
//     J_m(j_mn r / R) cos or sin (m theta). Every m > 0 mode is a pair, a
//     little apart (the tension is never even round the lugs), each pair
//     turned to its own angle.
//   - The air outside rides on the modes that move it as a whole (m = 0):
//     a mass that lowers them.
//   - A hard stroke stretches the head: the tension rises with the motion's
//     stretch energy, and every mode with it (the pitch starts sharp and
//     falls as the sound dies).
//   - Anything may touch it anywhere: shape() gives each mode's share at a
//     point; with it, the head's height and speed there, and a force into it.
//
// Two heads on one shell share its air (DrumAir): the cavity is a spring
// between the modes that change its volume.
#pragma once

#include <cstdint>
#include <vector>

#include "surface.h"

namespace pajupilli {

struct HeadSpec {
  double radius = 0.178;    // m (a 14" head)
  double sigma = 0.26;      // areal density, kg/m^2 (a coated 10 mil head)
  double tension = 3000;    // N/m
  double top = 4000;        // modes up to here (Hz)
  // Loss, as a share of critical damping: the (0,1) (it moves the air most),
  // the other m = 0 modes, the rest; and each mode's own loss rising with
  // frequency (x (1 + f / lossKnee)).
  double zeta01 = 0.03, zeta0 = 0.012, zeta = 0.008, lossKnee = 3000;
  double airMass = 1.0;     // the outside air on the m = 0 modes (x the piston's 8/3 rho R^3 by their volume)
  double split = 0.004;     // a pair's two members this far apart (a share of the frequency, at random up to it)
  double stretch = 0;       // E h / (1 - nu^2) of the film (N/m; 0: no glide)
  uint32_t seed = 1;
};

class Membrane : public Surface {
 public:
  Membrane(double fs, const HeadSpec& spec);
  int modes() const override { return n_; }
  double radius() const override { return spec_.radius; }
  double frequency(int k) const { return freq_[k]; }  // Hz, as loaded by the air
  int diameters(int k) const { return m_[k]; }
  int circles(int k) const { return nc_[k]; }  // its nodal circles, the rim counted (1: (m, 1))
  // Re-voice mode k (a timpani's air and kettle, research/timpani.md 12.1): its family's frequency
  // (Hz: a pair's two members keep their split about it), its decay (T60, s), its mass x `mass`
  // (the air riding on it); then update() once for all.
  void setMode(int k, double hz, double t60, double mass = 1);
  void update() { coefs(); }
  double split(int k) const { return split_[k]; }  // (a pair member's frequency x its family's)
  // Each mode's share at the point (r from the centre, m; theta, rad) into
  // phi[modes()]: the head's height there is the sum of phi[k] q[k].
  void shape(double r, double theta, float* phi) const override;
  // The modes' own coordinates, for what couples to all of them at once.
  const float* q() const override { return q_.data(); }
  const float* v() const override { return v_.data(); }
  float* force() override { return f_.data(); }            // (generalised force, per mode)
  const float* volume() const { return vol_.data(); }  // the volume a mode sweeps per unit q (m^3)
  // One sample: the forces pushed since the last, then the modes' own swing.
  void step();
  void reset();
  double energy() const;  // J
  // The modes' loss, all times `x` (a hand or a brush lying on the head).
  void setDamping(double x);

 private:
  HeadSpec spec_;
  double fs_, dt_;
  int n_ = 0;
  std::vector<double> freq_;
  std::vector<int> m_, fam_, kind_, nc_;   // diameters; the radial table's row; 0: m = 0, 1: cos, 2: sin; circles
  std::vector<double> split_, massX_;      // a pair member's frequency x; its mass x so far
  std::vector<float> norm_, c0_, s0_;      // 1 / sqrt(mass); the pair's own angle
  std::vector<float> tab_;                 // [family][kRadii + 1]: J_m(j r / R)
  std::vector<float> q_, v_, f_, vol_, w2_;
  std::vector<float> a11_, a12_, a21_, a22_, alpha_;
  int maxM_ = 0;
  float stretchK_ = 0;
  double damp_ = 1;
  void coefs();
  static constexpr int kRadii = 96;
};

// The air two heads share inside the shell: a spring on the cavity's volume
// (stiffness rho c^2 / V), let out slowly by the vent hole.
struct DrumAir {
  double volume = 0.0139;   // m^3 (14" x 5.5")
  double vent = 12;         // the vent's corner (Hz): below it the air gets out
  double leak = 0;          // (state)
  // The cavity's pressure (Pa) from the two heads as they are, pushed into
  // both (top: its upward motion widens the cavity; bottom: narrows it).
  float couple(Membrane& top, Membrane& bottom, double dt);
};

}  // namespace pajupilli
