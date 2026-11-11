// The flute's air column as a 1-D acoustic waveguide (transmission line).
//
// 128 cells at the audio rate (48 kHz), one leapfrog step per sample:
//   p[i]  pressure at the middle of cell i (cell i spans [i dx, (i+1) dx] from the cork)
//   u[i]  volume flow through the interface between cell i and cell i+1
//   (rho / S') du/dt = -dp/dx - kappa sqrt(d/dt) u       (momentum, viscothermal loss)
//   C dp/dt          = u[i-1] - u[i] - q                  (continuity, q = side-hole flow)
// The cork is a rigid wall before cell 0 (u[-1] = 0). Cells 0..84 are the flute
// (cork to foot end), the rest a spherical-wave horn: the radiation from the foot
// into the room, ending in a non-reflecting spherical termination.
//
// Side branches (tone holes, the embouchure) are lumped at a cell: a series
// mass (chimney + pad slot) and resistance, then the radiation load, a resistance
// in parallel with an inertance. The cell pressure and the branch are solved
// together with the trapezoidal rule (implicit, so a branch never limits dt).
//
// The viscothermal loss sqrt(jw) is a constant plus three first-order sections
// (fitted 50 Hz .. 12 kHz within 10 %); each section is one state per interface.
//
// Layout for AVX-512: every per-cell array is alignas(64) float[128] (8 vectors
// of 16 floats). The per-sample loops run over all 128 cells with no branches;
// cells without a hole carry identity coefficients. Coefficients change only
// when the geometry or the keys move, once per block.
#pragma once

#include <cstdint>

namespace pajupilli {

constexpr int kCells = 128;
constexpr int kVec = 16;          // floats per AVX-512 register
constexpr int kLossSections = 3;  // sqrt(jw) fit
constexpr int kPad = 16;          // zero padding on both sides of p and u

struct alignas(64) BoreState {
  // Padded so p[i + 1] and u[i - 1] are plain loads (no branches at the ends).
  alignas(64) float pbuf[kPad + kCells + kPad];
  alignas(64) float ubuf[kPad + kCells + kPad];
  alignas(64) float psi[kLossSections][kCells];  // loss section states, per interface
  alignas(64) float q[kCells];                   // branch flow out of the cell
  alignas(64) float l[kCells];                   // flow through the radiation inertance
  float* p() { return pbuf + kPad; }
  float* u() { return ubuf + kPad; }
  const float* p() const { return pbuf + kPad; }
  const float* u() const { return ubuf + kPad; }
};

// Per-cell coefficients of one leapfrog step.
struct alignas(64) BoreCoefs {
  // u update:  u+ = au u - bu (p[i+1] - p[i]) - sum_k ck psi_k ;  psi_k+ = a_k psi_k + g_k (u+ - u)
  alignas(64) float au[kCells], bu[kCells], ck[kLossSections][kCells];
  float a[kLossSections], g[kLossSections];
  // node update, x = (p, q, l):  x+ = M x + v J,  J = u[i-1] - u[i]
  alignas(64) float m[3][3][kCells];
  alignas(64) float v[3][kCells];
};

// What a side branch at a cell looks like, all in SI units (m, kg, s).
struct Branch {
  double g = 0;     // 1 / series inertance (1 / (kg m^-4)); 0 = closed / no branch
  double rs = 0;    // series resistance (Pa s / m^3)
  double rr = 0;    // radiation resistance (Pa s / m^3), in parallel with ...
  double hl = 0;    // ... 1 / radiation inertance
  double vol = 0;   // extra cell volume (m^3): a closed hole's chimney
};

class Bore {
 public:
  static constexpr double kRho = 1.18;      // air in the flute, ~27 C, humid (kg/m^3)
  static constexpr double kC = 347.0;       // speed of sound (m/s)
  static constexpr double kNu = 1.56e-5;    // kinematic viscosity (m^2/s)
  static constexpr double kCourant = 0.99;  // c dt / dx

  explicit Bore(double fs);
  double dt() const { return dt_; }
  double dx() const { return dx_; }
  int footCell() const { return foot_; }  // the first horn cell
  double hornRadius() const { return termR_ - horn0_; }  // throat to far end (m)

  // Geometry: diameter (m) as a function of distance from the cork face; the
  // foot end at footX (m). Called once.
  void setBore(double (*diameter)(double x), double footX);
  // A branch at a cell (the coefficients are rebuilt by update()).
  void setBranch(int cell, const Branch& b) { branch_[cell] = b; dirty_ = true; }
  const Branch& branch(int cell) const { return branch_[cell]; }
  double area(int cell) const { return area_[cell]; }
  double compliance(int cell) const;  // C of the cell incl. a closed chimney (m^3/Pa)
  void update();  // rebuild coefficients if something changed

  // One time step. The embouchure cell `ec` is excluded from the vector node
  // update (its branch is solved by the caller): pOld is p[ec] before the step,
  // J the net bore inflow into it. Returns the flow out of the horn's far end.
  float step(int ec, float* pOld, float* J);

  BoreState s;
  BoreCoefs k;

 private:
  void buildU();
  void buildNode(int cell);
  double dt_, dx_, fs_;
  int foot_ = 0;
  double area_[kCells] = {};    // cell areas (m^2)
  double kappa_[kCells] = {};   // sqrt(jw) loss per length at each interface
  double horn0_ = 0, omega_ = 0;  // horn: throat radius (m), solid angle (sr)
  double termS_ = 0, termR_ = 0;  // far-end area, radius
  float termRes_ = 0, termInd_ = 0, termL_ = 0;  // spherical termination
  Branch branch_[kCells];
  bool dirty_ = true;
};

}  // namespace pajupilli
