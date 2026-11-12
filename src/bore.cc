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
  dx_ = kC * dt_ / kCourant;
  std::memset(&s, 0, sizeof(s));
  std::memset(&k, 0, sizeof(k));
  for (int j = 0; j < kLossSections; ++j) {
    const double h = 0.5 * kFitW[j] * dt_;
    k.a[j] = float((1 - h) / (1 + h));
    k.g[j] = float(1 / (1 + h));
  }
}

void Bore::setBore(const std::function<double(double)>& diameter, double footX) {
  foot_ = int(std::lround(footX / dx_));
  for (int i = 0; i < foot_; ++i) {
    double a = 0;  // mean area over the cell
    for (int j = 0; j < 4; ++j) {
      const double d = diameter((i + (j + 0.5) / 4) * dx_);
      a += 0.25 * M_PI * d * d / 4;
    }
    area_[i] = a;
  }
  // The horn: a spherical sector of solid angle omega whose throat (radius r0)
  // has the area of the bore end. Its mass load is that of an unflanged end:
  // r0 = 0.6133 a gives the end correction 0.6133 a.
  const double aEnd = std::sqrt(area_[foot_ - 1] / M_PI);
  horn0_ = 0.6133 * aEnd;
  omega_ = area_[foot_ - 1] / (horn0_ * horn0_);
  for (int i = foot_; i < kCells; ++i) {
    const double r = horn0_ + (i - foot_ + 0.5) * dx_;
    area_[i] = omega_ * r * r;
  }
  termR_ = horn0_ + (kCells - foot_) * dx_;
  termS_ = omega_ * termR_ * termR_;
  // Spherical-wave termination: Z = rho c / S * jkr / (1 + jkr), i.e. a
  // resistance rho c / S in parallel with an inertance rho r / S.
  termRes_ = float(termS_ / (kRho * kC));
  termInd_ = float(dt_ * termS_ / (kRho * termR_));
  // Viscothermal loss per length, thermal part lumped into the series term:
  // Re Z = 2 rho / (S a) * 1.47 * sqrt(nu w / 2)  =>  kappa = 2.94 rho sqrt(nu) / (S a).
  for (int i = 0; i < kCells; ++i) {
    if (i + 1 >= foot_) { kappa_[i] = 0; continue; }  // free air past the foot
    const double sa = std::min(area_[i], area_[i + 1]);
    kappa_[i] = 2.94 * kRho * std::sqrt(kNu) / (sa * std::sqrt(sa / M_PI));
  }
  buildU();
  dirty_ = true;
  update();
}

void Bore::buildU() {
  double sumCG = 0;
  for (int j = 0; j < kLossSections; ++j) sumCG += kFitC[j] * k.g[j];
  for (int i = 0; i < kCells; ++i) {
    // Interface area: the smaller neighbour keeps the scheme's energy bounded.
    const double sa = i + 1 < kCells ? std::min(area_[i], area_[i + 1]) : area_[i];
    const double l = kRho / (sa * dt_), kap = kappa_[i];
    const double d = l + 0.5 * kap * (kFitC0 + sumCG);
    k.au[i] = float((l - 0.5 * kap * (kFitC0 - sumCG)) / d);
    k.bu[i] = float(1.0 / (dx_ * d));
    k.fr[i] = i == foot_ - 1 ? k.bu[i] : 0.0f;
    for (int j = 0; j < kLossSections; ++j)
      k.ck[j][i] = float(0.5 * kap * kFitC[j] * (1 + k.a[j]) / d);
  }
}

double Bore::compliance(int cell) const {
  return (area_[cell] * dx_ + branch_[cell].vol) / (kRho * kC * kC);
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

void Bore::update() {
  if (!dirty_) return;
  for (int i = 0; i < kCells; ++i) buildNode(i);
  dirty_ = false;
}

float Bore::step(int ec, float* pOld, float* J, float pRoom) {
  float* __restrict p = s.p();
  float* __restrict u = s.u();
  // Momentum: interfaces 0..127 (p[128] is the zero pad).
  for (int i = 0; i < kCells; ++i) {
    const float un = k.au[i] * u[i] - k.bu[i] * (p[i + 1] - p[i]) - k.fr[i] * pRoom -
                     k.ck[0][i] * s.psi[0][i] - k.ck[1][i] * s.psi[1][i] -
                     k.ck[2][i] * s.psi[2][i];
    const float du = un - u[i];
    s.psi[0][i] = k.a[0] * s.psi[0][i] + k.g[0] * du;
    s.psi[1][i] = k.a[1] * s.psi[1][i] + k.g[1] * du;
    s.psi[2][i] = k.a[2] * s.psi[2][i] + k.g[2] * du;
    u[i] = un;
  }
  // The far end of the horn: the spherical-wave load.
  const float pe = p[kCells - 1];
  const float uOut = pe * termRes_ + termL_;
  termL_ += termInd_ * pe;
  u[kCells - 1] = uOut;
  *pOld = p[ec];
  *J = u[ec - 1] - u[ec];
  // Continuity with the side branches (u[-1] is the zero pad: the cork).
  float* __restrict q = s.q;
  float* __restrict l = s.l;
  for (int i = 0; i < kCells; ++i) {
    const float j = u[i - 1] - u[i];
    const float p0 = p[i], q0 = q[i], l0 = l[i];
    p[i] = k.m[0][0][i] * p0 + k.m[0][1][i] * q0 + k.m[0][2][i] * l0 + k.v[0][i] * j + k.e[0][i] * pRoom;
    q[i] = k.m[1][0][i] * p0 + k.m[1][1][i] * q0 + k.m[1][2][i] * l0 + k.v[1][i] * j + k.e[1][i] * pRoom;
    l[i] = k.m[2][0][i] * p0 + k.m[2][1][i] * q0 + k.m[2][2][i] * l0 + k.v[2][i] * j + k.e[2][i] * pRoom;
  }
  return uOut;
}

}  // namespace pajupilli
