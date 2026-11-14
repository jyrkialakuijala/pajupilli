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
// Layout for AVX-512: every per-cell array is alignas(64) float[kRow]. The
// per-sample loops run over the used cells in vectors of 16 with no branches.
// Most cells have no hole: their node update is p += v0 J alone; the cells
// with a branch take the full 3x3 update (by vectors where they are many, one
// by one where they are few). Coefficients
// change only when the geometry or the keys move, once per block.
#pragma once

#include <cstdint>
#include <functional>

namespace pajupilli {

constexpr int kCells = 512;        // room for the longest bore (a horn in F at 32 kHz);
                                  // the loops run over the used vectors only
constexpr int kVec = 16;          // floats per AVX-512 register
constexpr int kLossSections = 3;  // sqrt(jw) fit
constexpr int kPad = 16;          // zero padding on both sides of p and u
// Rows of the per-cell arrays: one vector longer than kCells, so that cell i of
// each array falls in a different L1 set (at 2 KB apart, all ~25 of them
// shared a few sets and evicted each other).
constexpr int kRow = kCells + kVec;

struct alignas(64) BoreState {
  // Padded so p[i + 1] and u[i - 1] are plain loads (no branches at the ends).
  alignas(64) float pbuf[kPad + kCells + kPad];
  alignas(64) float ubuf[kPad + kCells + kPad];
  alignas(64) float psi[kLossSections][kRow];  // loss section states, per interface
  alignas(64) float q[kRow];                   // branch flow out of the cell
  alignas(64) float l[kRow];                   // flow through the radiation inertance
  float* p() { return pbuf + kPad; }
  float* u() { return ubuf + kPad; }
  const float* p() const { return pbuf + kPad; }
  const float* u() const { return ubuf + kPad; }
};

// Per-cell coefficients of one leapfrog step.
struct alignas(64) BoreCoefs {
  // u update:  u+ = au u - bu (p[i+1] - p[i]) - sum_k ck psi_k ;  psi_k+ = a_k psi_k + g_k (u+ - u)
  alignas(64) float au[kRow], bu[kRow], ck[kLossSections][kRow];
  float a[kLossSections], g[kLossSections];
  // node update, x = (p, q, l):  x+ = M x + v J,  J = u[i-1] - u[i]
  alignas(64) float m[3][3][kRow];
  alignas(64) float v[3][kRow];
  alignas(64) float e[3][kRow];  // x+ += e pRoom: the room's pressure at the opening
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
  int cells() const { return n_; }            // cells computed per step (a multiple of 16)
  int embCell() const { return ec_; }     // the cell centred on the embouchure
  double x0() const { return x0_; }       // cell 0 starts this far from the cork face (m)
  double courant() const { return kC * dt_ / dx_; }

  // Geometry: diameter (m) as a function of distance from the cork face; the
  // embouchure centre at embX, the foot end at footX (m). Sets dx. A closed
  // end (a stopped pipe, a vessel) has no radiation branch.
  // webster: each interface takes the bore's own area there and each cell the
  // mean of its two faces (Bilbao 2009, the Webster equation's stable scheme:
  // exact for cones and bells, stable at Courant <= 1); otherwise cells take
  // their mean area and interfaces the smaller neighbour (the flute family's
  // nearly cylindrical bores, fitted that way).
  void setBore(const std::function<double(double)>& diameter, double embX, double footX,
               bool closedEnd = false, bool webster = false);
  // A series length (m, in units of the bore at the cell; negative for a
  // hole's t_a) split over the two interfaces around a cell.
  void setSeries(int cell, double len);
  // A branch at a cell (the coefficients are rebuilt by update()).
  void setBranch(int cell, const Branch& b) { branch_[cell] = b; dirty_ = true; }
  const Branch& branch(int cell) const { return branch_[cell]; }
  double area(int cell) const { return area_[cell]; }
  double compliance(int cell) const;  // C of the cell incl. a closed chimney (m^3/Pa)
  void update();  // rebuild coefficients if something changed
  // A branch whose coefficients are rebuilt at once (only this cell).
  void setBranchNow(int cell, const Branch& b) {
    branch_[cell] = b;
    buildNode(cell);
    classify(cell);
  }

  // Brass: lumped and network pieces (after setBore).
  // Extra volume at a cell (m^3): a mouthpiece's cup.
  void addVolume(int cell, double v) { cellVol_[cell] += v; dirty_ = true; }
  // Extra length (m, in units of the interface's own area) of the interface
  // between cell i and i + 1: a constriction narrower than its cells (a throat).
  void setInterfaceLength(int i, double len) { ifaceExtra_[i] = len; dirtyU_ = true; }
  double interfaceArea(int i) const { return areaI_[i]; }
  // A trombone's slide: cell i (and the interface after it) stand for s >= 1
  // times their length, a cylinder's mass, volume and losses all x s (the grid
  // stays; c dt / (s dx) only falls, so it stays stable). Its node at once.
  void setStretch(int i, double s) {
    if (stretch_[i] == s) return;
    stretch_[i] = s;
    dirtyU_ = true;
    buildNode(i);
    classify(i);
  }
  double stretch(int i) const { return stretch_[i]; }
  // A valve port: the interface's open share (1 open .. 0 shut). Part open, it
  // is narrower (more mass) and loses the flow's energy (a resistance).
  void setGate(int i, double open) {
    if (gate_[i] == open) return;
    gate_[i] = open;
    dirtyU_ = true;
  }
  // A flow w (m^3/s) leaving the cell for another tube over the last step
  // (a valve loop's link): applied after step(), as part of its continuity.
  void inject(int cell, float w) {
    s.p()[cell] -= k.v[0][cell] * w;
    s.q[cell] -= k.v[1][cell] * w;
    s.l[cell] -= k.v[2][cell] * w;
  }

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
  void classify(int cell);
  void listFull();
  double dt_, dx_, fs_, x0_ = 0;
  int foot_ = 0, ec_ = 0, n_ = kVec;
  double cellVol_[kCells] = {};  // lumped extra volume (cork cavity) (m^3)
  double series_[kCells] = {};   // series length per cell (m)
  double ifaceExtra_[kCells] = {};  // extra length per interface (m)
  double gate_[kCells];          // valve ports: open share per interface
  double stretch_[kCells];       // a slide's cells: length x this (setStretch)
  bool dirtyU_ = false;
  double area_[kCells] = {};    // cell areas (m^2)
  double areaI_[kCells] = {};   // interface areas (m^2): between cell i and i + 1
  double kappa_[kCells] = {};   // sqrt(jw) loss per length at each interface
  Branch branch_[kCells];
  bool dirty_ = true;
  // Cells whose node update needs the full 3x3 matrix (a branch, or a branch's
  // flow left over); every other cell is p += v0 J (step()'s fast path). A
  // vector of 16 cells with several of them takes the full update as a whole
  // (dense_); the others' few go one by one from fullList_.
  bool full_[kCells] = {};
  bool dense_[kCells / kVec] = {};
  int fullList_[kCells], nFull_ = 0;
  bool listDirty_ = true;
};

}  // namespace pajupilli
