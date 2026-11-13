#include "bore.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace pajupilli {
namespace {

// sqrt(s) ~ c0 + sum_k c_k s / (s + w_k), s = jw, fitted 50 Hz .. 12 kHz
// (max error 9 %, real part 11 %; tools in the commit log).
constexpr double kFitC0 = 9.04488;
constexpr double kFitC[kLossSections] = {22.7977, 69.8657, 441.289};
constexpr double kFitW[kLossSections] = {831.168, 8811.22, 126729.0};

// Inverse of a 3x3 matrix (row major), double precision.
void invert3(const double a[3][3], double r[3][3]) {
  const double c00 = a[1][1] * a[2][2] - a[1][2] * a[2][1];
  const double c01 = a[1][2] * a[2][0] - a[1][0] * a[2][2];
  const double c02 = a[1][0] * a[2][1] - a[1][1] * a[2][0];
  const double det = a[0][0] * c00 + a[0][1] * c01 + a[0][2] * c02;
  const double id = 1.0 / det;
  r[0][0] = c00 * id;
  r[1][0] = c01 * id;
  r[2][0] = c02 * id;
  r[0][1] = (a[0][2] * a[2][1] - a[0][1] * a[2][2]) * id;
  r[1][1] = (a[0][0] * a[2][2] - a[0][2] * a[2][0]) * id;
  r[2][1] = (a[0][1] * a[2][0] - a[0][0] * a[2][1]) * id;
  r[0][2] = (a[0][1] * a[1][2] - a[0][2] * a[1][1]) * id;
  r[1][2] = (a[0][2] * a[1][0] - a[0][0] * a[1][2]) * id;
  r[2][2] = (a[0][0] * a[1][1] - a[0][1] * a[1][0]) * id;
}

}  // namespace

Bore::Bore(double fs) : fs_(fs) {
  dt_ = 1.0 / fs;
  dx_ = kC * dt_ / kCourantMax;
  std::memset(&s, 0, sizeof(s));
  std::memset(&k, 0, sizeof(k));
  std::fill(gate_, gate_ + kCells, 1.0);
  for (int j = 0; j < kLossSections; ++j) {
    const double h = 0.5 * kFitW[j] * dt_;
    k.a[j] = float((1 - h) / (1 + h));
    k.g[j] = float(1 / (1 + h));
  }
}

void Bore::setBore(const std::function<double(double)>& diameter, double embX, double footX,
                   bool closedEnd, bool webster) {
  // dx at the Courant limit, the embouchure at the centre of cell ec. The cork
  // face is x0 before cell 0 (that piece of the cavity: a volume in cell 0).
  // The foot ends `endLen` past the centre of the last cell (foot - 1): the
  // end branch's tube and the cell's volume take the rest, so the grid does
  // not depend on the foot (a B and a C foot share it, and the hole fit).
  dx_ = kC * dt_ / kCourantMax;
  ec_ = std::max(0, int(std::floor(embX / dx_ - 0.5)));
  x0_ = embX - (ec_ + 0.5) * dx_;
  foot_ = std::min(kCells - 1, int(std::floor((footX - x0_) / dx_ + 0.5)));
  // The used cells, past the foot by at least one idle cell (p[foot] reads 0).
  n_ = std::min(kCells, (foot_ / kVec + 1) * kVec);
  listDirty_ = true;
  std::memset(cellVol_, 0, sizeof cellVol_);
  std::memset(series_, 0, sizeof series_);
  std::memset(ifaceExtra_, 0, sizeof ifaceExtra_);
  std::fill(gate_, gate_ + kCells, 1.0);
  for (Branch& b : branch_) b = Branch();
  const double endLen = footX - (x0_ + (foot_ - 0.5) * dx_);  // 0 .. dx
  for (int i = 0; i < kCells; ++i) {
    double a = 0;  // mean area over the cell (idle cells past the foot: the end's)
    const int ic = std::min(i, foot_ - 1);
    for (int j = 0; j < 4; ++j) {
      const double d = diameter(x0_ + (ic + (j + 0.5) / 4) * dx_);
      a += 0.25 * M_PI * d * d / 4;
    }
    area_[i] = a;
  }
  auto areaAt = [&](double x) { const double d = diameter(x); return 0.25 * M_PI * d * d; };
  for (int i = 0; i < kCells; ++i) {
    if (!webster) {
      areaI_[i] = std::min(area_[i], area_[std::min(i + 1, kCells - 1)]);
      continue;
    }
    areaI_[i] = areaAt(x0_ + (std::min(i, foot_ - 1) + 1) * dx_);
  }
  if (webster)
    for (int i = 0; i < kCells; ++i) {
      const int ic = std::min(i, foot_ - 1);
      area_[i] = 0.5 * (areaAt(x0_ + ic * dx_) + (ic == foot_ - 1 ? areaAt(footX) : areaI_[ic]));
    }
  {
    const double d = diameter(0.5 * x0_);
    cellVol_[0] = 0.25 * M_PI * d * d * x0_;
  }
  cellVol_[foot_ - 1] += area_[foot_ - 1] * (endLen - 0.5 * dx_);  // the last cell ends at the foot
  // Viscothermal loss per length, thermal part lumped into the series term:
  // Re Z = 2 rho / (S a) * 1.47 * sqrt(nu w / 2)  =>  kappa = 2.94 rho sqrt(nu) / (S a).
  for (int i = 0; i < kCells; ++i) {
    const double sa = areaI_[i];
    kappa_[i] = 2.94 * kRho * std::sqrt(kNu) / (sa * std::sqrt(sa / M_PI));
  }
  // The open end: the tube from the last cell's centre to the end in series
  // with the unflanged radiation load, a resistance 1.505 rho c / S in parallel
  // with the mass of 0.6133 a (Levine-Schwinger at low ka: Re Z = rho c / S (ka)^2 / 4).
  if (!closedEnd) {  // (closed: the last cell's volume already reaches the stopper)
    const double s = area_[foot_ - 1], a = std::sqrt(s / M_PI);
    Branch end;
    end.g = s / (kRho * std::max(endLen, 0.02 * dx_));
    end.rr = 1.505 * kRho * kC / s;
    end.hl = s / (kRho * 0.6133 * a);
    branch_[foot_ - 1] = end;
  }
  buildU();
  dirty_ = true;
  update();
}

void Bore::buildU() {
  double sumCG = 0;
  for (int j = 0; j < kLossSections; ++j) sumCG += kFitC[j] * k.g[j];
  for (int i = 0; i < kCells; ++i) {
    if (i >= foot_ - 1 || gate_[i] <= 0) {  // closed: the foot's end and the idle cells past it; a shut valve port
      k.au[i] = k.bu[i] = 0;
      for (int j = 0; j < kLossSections; ++j) k.ck[j][i] = 0;
      continue;
    }
    const double sa = areaI_[i];
    // A hole's series length (t_a) lengthens or shortens the interface's mass.
    // Never by more than the stability margin left by kCourantMax: a cell next
    // to it, without a chimney's extra volume, needs
    // lambda^2 (1 / (1 - e) + 1) / 2 <= 1.
    const double lam2 = courant() * courant(), eMax = 0.9 * (1 - lam2 / (2 - lam2));
    const double extra =
        std::max(-eMax * dx_, 0.5 * (series_[i] + (i + 1 < kCells ? series_[i + 1] : 0)) + ifaceExtra_[i]);
    // A part-open valve port: its area shrinks (more mass) and the flow through
    // it separates, a resistance ~ rho c / S (1 / open - 1) / 2 (per length here).
    const double g = gate_[i];
    const double l = kRho * (1 + extra / dx_) / (sa * g * dt_), kap = kappa_[i];
    const double r = g < 1 ? 0.5 * kRho * kC / sa * (1 / g - 1) / dx_ : 0;
    const double d = l + 0.5 * kap * (kFitC0 + sumCG) + 0.5 * r;
    k.au[i] = float((l - 0.5 * kap * (kFitC0 - sumCG) - 0.5 * r) / d);
    k.bu[i] = float(1.0 / (dx_ * d));
    for (int j = 0; j < kLossSections; ++j)
      k.ck[j][i] = float(0.5 * kap * kFitC[j] * (1 + k.a[j]) / d);
  }
}

double Bore::compliance(int cell) const {
  return (area_[cell] * dx_ + branch_[cell].vol + cellVol_[cell]) / (kRho * kC * kC);
}

void Bore::setSeries(int cell, double len) {
  if (series_[cell] == len) return;
  series_[cell] = len;
  dirtyU_ = true;
}

// Trapezoidal rule for x = (p, q, l) at one cell:
//   C dp/dt = J - q
//   dq/dt   = g (p - pRoom - rs q - rr (q - l))
//   dl/dt   = hl rr (q - l)
void Bore::buildNode(int cell) {
  const Branch& b = branch_[cell];
  const double c = compliance(cell), w = 2.0 / dt_;
  const double a[3][3] = {{0, -1 / c, 0},
                          {b.g, -b.g * (b.rs + b.rr), b.g * b.rr},
                          {0, b.hl * b.rr, -b.hl * b.rr}};
  double lhs[3][3], inv[3][3];
  for (int r = 0; r < 3; ++r)
    for (int col = 0; col < 3; ++col) lhs[r][col] = (r == col ? w : 0) - a[r][col];
  invert3(lhs, inv);
  for (int r = 0; r < 3; ++r) {
    for (int col = 0; col < 3; ++col)
      k.m[r][col][cell] = float(2 * w * inv[r][col] - (r == col ? 1 : 0));
    k.v[r][cell] = float(2 * inv[r][0] / c);
    k.e[r][cell] = float(-2 * inv[r][1] * b.g);
  }
}

// A cell is plain when its node update is exactly p += v0 J: no branch (the
// identity for q and l, nothing from the room) and no branch flow left in q or
// l (a plain cell keeps them as they are, so this holds until the next change).
void Bore::classify(int i) {
  bool plain = s.q[i] == 0 && s.l[i] == 0 && k.m[0][0][i] == 1 && k.m[0][2][i] == 0;
  for (int r = 1; r < 3; ++r)
    for (int c = 0; c < 3; ++c) plain &= k.m[r][c][i] == (r == c ? 1.0f : 0.0f);
  for (int r = 0; r < 3; ++r) plain &= (r == 0 || k.v[r][i] == 0) && k.e[r][i] == 0;
  if (full_[i] == !plain) return;
  full_[i] = !plain;
  listDirty_ = true;
}

// A vector with 2 or more full cells: its full update as a vector costs less.
void Bore::listFull() {
  nFull_ = 0;
  for (int c = 0; c < n_ / kVec; ++c) {
    int m = 0;
    for (int i = c * kVec; i < (c + 1) * kVec; ++i) m += full_[i];
    dense_[c] = m >= 2;
    if (dense_[c]) continue;
    for (int i = c * kVec; i < (c + 1) * kVec; ++i)
      if (full_[i]) fullList_[nFull_++] = i;
  }
  listDirty_ = false;
}

void Bore::update() {
  if (dirtyU_) {
    buildU();
    dirtyU_ = false;
  }
  if (!dirty_) return;
  for (int i = 0; i < n_; ++i) {
    buildNode(i);
    classify(i);
  }
  dirty_ = false;
}

void Bore::step(int ec, float* pOld, float* J, float pRoom) {
  float* __restrict p = s.p();
  float* __restrict u = s.u();
  // Momentum: every interface (p[n] is an idle cell or the zero pad; past the foot all zero).
  const int n = n_;
  for (int i = 0; i < n; ++i) {
    const float un = k.au[i] * u[i] - k.bu[i] * (p[i + 1] - p[i]) - k.ck[0][i] * s.psi[0][i] - k.ck[1][i] * s.psi[1][i] -
                     k.ck[2][i] * s.psi[2][i];
    const float du = un - u[i];
    s.psi[0][i] = k.a[0] * s.psi[0][i] + k.g[0] * du;
    s.psi[1][i] = k.a[1] * s.psi[1][i] + k.g[1] * du;
    s.psi[2][i] = k.a[2] * s.psi[2][i] + k.g[2] * du;
    u[i] = un;
  }
  *pOld = p[ec];
  *J = u[ec - 1] - u[ec];
  // Continuity (u[-1] is the zero pad: the cork). The few cells with a branch
  // first, from the old pressures (their new p set aside), then each vector:
  // a dense one in full, the others as plain cells; then the few cells' p put back.
  if (listDirty_) listFull();
  float* __restrict q = s.q;
  float* __restrict l = s.l;
  float pFull[kCells];
  for (int b = 0; b < nFull_; ++b) {
    const int i = fullList_[b];
    const float j = u[i - 1] - u[i];
    const float p0 = p[i], q0 = q[i], l0 = l[i];
    pFull[b] = k.m[0][0][i] * p0 + k.m[0][1][i] * q0 + k.m[0][2][i] * l0 + k.v[0][i] * j + k.e[0][i] * pRoom;
    q[i] = k.m[1][0][i] * p0 + k.m[1][1][i] * q0 + k.m[1][2][i] * l0 + k.v[1][i] * j + k.e[1][i] * pRoom;
    l[i] = k.m[2][0][i] * p0 + k.m[2][1][i] * q0 + k.m[2][2][i] * l0 + k.v[2][i] * j + k.e[2][i] * pRoom;
  }
  const float* __restrict v0 = k.v[0];
  for (int c = 0; c < n; c += kVec) {
    if (!dense_[c / kVec]) {
      for (int i = c; i < c + kVec; ++i) p[i] += v0[i] * (u[i - 1] - u[i]);
      continue;
    }
    for (int i = c; i < c + kVec; ++i) {
      const float j = u[i - 1] - u[i];
      const float p0 = p[i], q0 = q[i], l0 = l[i];
      p[i] = k.m[0][0][i] * p0 + k.m[0][1][i] * q0 + k.m[0][2][i] * l0 + k.v[0][i] * j + k.e[0][i] * pRoom;
      q[i] = k.m[1][0][i] * p0 + k.m[1][1][i] * q0 + k.m[1][2][i] * l0 + k.v[1][i] * j + k.e[1][i] * pRoom;
      l[i] = k.m[2][0][i] * p0 + k.m[2][1][i] * q0 + k.m[2][2][i] * l0 + k.v[2][i] * j + k.e[2][i] * pRoom;
    }
  }
  for (int b = 0; b < nFull_; ++b) p[fullList_[b]] = pFull[b];
}

}  // namespace pajupilli
