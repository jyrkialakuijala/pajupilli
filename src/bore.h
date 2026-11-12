// The flute's air column as a 1-D acoustic waveguide (transmission line).
//
// 96 cells at the model's rate (48 kHz for the flute), one leapfrog step per sample:
//   p[i]  pressure at the middle of cell i (cell i spans [i dx, (i+1) dx] from the cork)
//   u[i]  volume flow through the interface between cell i and cell i+1
//   (rho / S') du/dt = -dp/dx - kappa sqrt(d/dt) u       (momentum, viscothermal loss)
//   C dp/dt          = u[i-1] - u[i] - q                  (continuity, q = side-hole flow)
// The cork is a rigid wall before cell 0 (u[-1] = 0). dx is at the Courant
// limit and the grid placed so the embouchure is at a cell centre; the cork
// cavity's last few mm before cell 0 are a lumped volume in cell 0, and the
// foot ends inside the last cell (its end branch takes the rest of the tube). The open foot is a side branch
// at the last cell: half a cell of tube, then the unflanged radiation load
// (Levine-Schwinger at low ka: end correction 0.6133 a). Cells past it are idle.
//
// Side branches (tone holes, the embouchure) are lumped at a cell: a series
// mass (chimney + pad slot) and resistance, then the radiation load, a resistance
// in parallel with an inertance. The cell pressure and the branch are solved
// together with the trapezoidal rule (implicit, so a branch never limits dt).
//
// The viscothermal loss sqrt(jw) is a constant plus three first-order sections
// (fitted 50 Hz .. 12 kHz within 10 %); each section is one state per interface.
//
// Layout for AVX-512: every per-cell array is alignas(64) float[96] (6 vectors
// of 16 floats). The per-sample loops run over all 128 cells with no branches;
// cells without a hole carry identity coefficients. Coefficients change only
// when the geometry or the keys move, once per block.
#pragma once

#include <cstdint>
#include <functional>

namespace pajupilli {

constexpr int kCells = 96;
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
  alignas(64) float e[3][kCells];  // x+ += e pRoom: the room's pressure at the opening
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
  static constexpr double kRho = 1.18;      // air in the flute (kg/m^3)
  // Speed of sound in the played flute: ~30 C at the embouchure, saturated,
  // 2.5 % CO2 (exhaled), Dickens 2007 (Cramer's formula).
  static constexpr double kC = 349.5;
  static constexpr double kNu = 1.56e-5;    // kinematic viscosity (m^2/s)
  // Courant number c dt / dx: leaves room for the closed holes' negative
  // series inertance (setSeries) without losing stability.
  static constexpr double kCourantMax = 0.975;

  explicit Bore(double fs);
  double dt() const { return dt_; }
  double dx() const { return dx_; }
  int footCell() const { return foot_ - 1; }  // the last cell: the open end's branch
  int embCell() const { return ec_; }     // the cell centred on the embouchure
  double x0() const { return x0_; }       // cell 0 starts this far from the cork face (m)
  double courant() const { return kC * dt_ / dx_; }

  // Geometry: diameter (m) as a function of distance from the cork face; the
  // embouchure centre at embX, the foot end at footX (m). Sets dx. Called once.
  void setBore(const std::function<double(double)>& diameter, double embX, double footX);
  // A series length (m, in units of the bore at the cell; negative for a
  // hole's t_a) split over the two interfaces around a cell.
  void setSeries(int cell, double len);
  // A branch at a cell (the coefficients are rebuilt by update()).
  void setBranch(int cell, const Branch& b) { branch_[cell] = b; dirty_ = true; }
  const Branch& branch(int cell) const { return branch_[cell]; }
  double area(int cell) const { return area_[cell]; }
  double compliance(int cell) const;  // C of the cell incl. a closed chimney (m^3/Pa)
  void update();  // rebuild coefficients if something changed

  // One time step. The embouchure cell `ec` is excluded from the vector node
  // update (its branch is solved by the caller): pOld is p[ec] before the step,
  // J the net bore inflow into it.
  // pRoom: the room's sound pressure outside (Pa), acting on every open branch
  // (the holes and the foot), in series with each opening's radiation load: the
  // flute takes in as much as it would radiate, no more (reciprocity).
  void step(int ec, float* pOld, float* J, float pRoom = 0);

  BoreState s;
  BoreCoefs k;

 private:
  void buildU();
  void buildNode(int cell);
  double dt_, dx_, fs_, x0_ = 0;
  int foot_ = 0, ec_ = 0;
  double cellVol_[kCells] = {};  // lumped extra volume (cork cavity) (m^3)
  double series_[kCells] = {};   // series length per cell (m)
  bool dirtyU_ = false;
  double area_[kCells] = {};    // cell areas (m^2)
  double kappa_[kCells] = {};   // sqrt(jw) loss per length at each interface
  Branch branch_[kCells];
  bool dirty_ = true;
};

}  // namespace pajupilli
