// A cymbal: a thin round bronze plate with a free edge, as its modes.
//
//   - The modes are the free round plate's: (m, n), m nodal diameters and n
//     nodal circles, shaped [J_m(k r) + C I_m(k r)] cos or sin (m theta),
//     their frequencies rising as the square of k (a plate is stiff, not
//     stretched: the high modes lie far closer together than a drum head's).
//   - The dome: every mode that stretches the plate's middle surface (all
//     but the (m, 0)s, which only bend it) is lifted by the dome's own
//     stiffness, sqrt(E / rho) / (2 pi Rs) for a dome of radius Rs. Below
//     that pitch only the (m, 0) modes are left, as on a real cymbal.
//   - Every m > 0 mode is a pair a little apart (the hammering), each turned
//     to its own angle.
//   - The whole plate also moves as a body (on its felts): its height and
//     its two tilts are whoever holds it's to move (hihat.h).
//
// (A first form: the flat plate's shapes with the dome as a correction. The
// plate's true profile and taper, ring by ring, is research/cymbal_plan.md
// step 1.)
#pragma once

#include <cstdint>
#include <vector>

#include "surface.h"

namespace pajupilli {

struct PlateSpec {
  double radius = 0.178;      // m (14")
  double thickness = 1.2e-3;  // m, the mean
  double E = 110e9, rho = 8600, nu = 0.33;  // B20 bronze
  double domeRadius = 0.8;    // m (0: flat)
  // A cymbal is not a ball's cap: it is most curved at the bell and nearly
  // flat towards the edge. domeTaper: the curvature falls from (1 + taper/2)
  // of the mean at the middle to (1 - taper/2) at the edge (0: the same all
  // over, which piles every stretching mode up just above one pitch); each
  // mode is lifted by the curvature where it moves.
  double domeTaper = 0;
  double top = 16000;         // modes up to here (Hz)
  // Loss (1/s): lossA w^0.6 + lossB (w: rad/s); the felt at the middle takes
  // more from the modes that move there (x (1 + felt x their share)).
  // (Left to ring, a cymbal dies slowly: about 5 s to -60 dB at 1 kHz, 3 s at
  // 10 kHz with these. What stops it sooner is what touches it.)
  double lossA = 0.0025, lossB = 0.6, felt = 3;
  double split = 0.006;       // a pair's members this far apart (a share, at random up to it)
  uint32_t seed = 1;
};

class Plate : public Surface {
 public:
  Plate(double fs, const PlateSpec& spec);
  int modes() const override { return n_; }
  double radius() const override { return spec_.radius; }
  void shape(double r, double theta, float* phi) const override;
  const float* q() const override { return q_.data(); }
  const float* v() const override { return v_.data(); }
  float* force() override { return f_.data(); }
  double frequency(int k) const { return freq_[k]; }
  int diameters(int k) const { return m_[k]; }
  int circles(int k) const { return nc_[k]; }
  // How much each mode moves on the circle at r, the mean square round it
  // (into share[modes()]); and a loss there: each mode loses rate x its
  // share, for this sample (something rubbing on the plate all round).
  void ringShare(double r, float* share) const;
  void rub(const float* share, float rate) {
    for (int k = 0; k < n_; ++k) f_[k] -= rate * share[k] * v_[k];
  }
  // A mode's own loss (1/s), to fit measured decays; retune() after setting.
  double loss(int k) const { return alpha_[k]; }
  void setLoss(int k, double a) { alpha_[k] = float(a); }
  void retune() { coefs(); }
  double mass() const;        // kg, the whole plate
  void step();                // one sample: the forces pushed, then the modes' swing
  void reset();
  double energy() const;      // J
  // All losses times x (a hand on it: the choke).
  void setDamping(double x);

 private:
  void coefs();
  PlateSpec spec_;
  double fs_, dt_, damp_ = 1;
  int n_ = 0, maxM_ = 0;
  std::vector<double> freq_;
  std::vector<int> m_, nc_, fam_, kind_;
  std::vector<float> norm_, c0_, s0_, tab_;
  std::vector<float> q_, v_, f_, w2_, alpha_, a11_, a12_, a21_, a22_;
  static constexpr int kRadii = 128;
};

}  // namespace pajupilli
