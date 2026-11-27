#include "bassspeaker.h"

#include <algorithm>
#include <cmath>

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRho = 1.18, kC = 343;  // air at 20 C
// x = A^-1 y for an n x n system (n <= 5), A destroyed (partial pivoting).
template <typename T>
void solve(int n, T A[5][5], T y[5]) {
  for (int c = 0; c < n; ++c) {
    int p = c;
    for (int r = c + 1; r < n; ++r) if (std::abs(A[r][c]) > std::abs(A[p][c])) p = r;
    if (p != c) { for (int q = 0; q < n; ++q) std::swap(A[c][q], A[p][q]); std::swap(y[c], y[p]); }
    for (int r = c + 1; r < n; ++r) {
      const T f = A[r][c] / A[c][c];
      for (int q = c; q < n; ++q) A[r][q] -= f * A[c][q];
      y[r] -= f * y[c];
    }
  }
  for (int r = n - 1; r >= 0; --r) {
    T a = y[r];
    for (int q = r + 1; q < n; ++q) a -= A[r][q] * y[q];
    y[r] = a / A[r][r];
  }
}
}  // namespace

// The driver's parts from its Thiele-Small numbers: Cms = Vas / (rho c^2 Sd^2), Mms from Fs,
// Rms from Qms, Bl^2 = ws Mms Re / Qes. The box's air on the cone: kb = rho c^2 Sd^2 / Vb; the
// port's air a mass on the box's spring (rho c^2 Sp^2 / Vb), tuned to fb, its loss from Qp.
BassSpeaker::BassSpeaker(double fs, const Spec& s) : fs_(fs), h_(1 / fs), s_(s) {
  const double rc2 = kRho * kC * kC, ws = 2 * kPi * s.fs;
  const double cms = s.vas / (rc2 * s.sd * s.sd);
  kms_ = 1 / cms;
  mms_ = 1 / (ws * ws * cms);
  rms_ = ws * mms_ / s.qms;
  const double bl = std::sqrt(ws * mms_ * s.re / s.qes);
  xbl_ = s.xmax / std::sqrt(std::max(1e-3, 1 / std::clamp(s.blAtX, 0.05, 0.999) - 1));
  xk_ = s.xmax / std::sqrt(std::max(1e-3, s.kAtX - 1));
  bl0_ = bl * (1 + (s.offset / xbl_) * (s.offset / xbl_));
  kb_ = rc2 * s.sd * s.sd / s.vb;
  n_ = s.fb > 0 ? 5 : 3;
  if (n_ == 5) {
    const double wb = 2 * kPi * s.fb, kp = rc2 * s.sp * s.sp / s.vb;
    mp_ = kp / (wb * wb);
    rp_ = wb * mp_ / s.qp;
  }
  rsrc_ = s.nominal / std::max(0.1, s.damping) * s.parallel / s.series;
  if (s.otHz > 0) lm_ = s.otR / (2 * kPi * s.otHz);
}

void BassSpeaker::reset() {
  std::fill(st_, st_ + kN, 0.0);
  uLast_ = phi_ = xPeak_ = 0;
}

// ds/dt = A s + b e (e the driver's share of the voltage, b = (1/Le, 0, ...)); A with the motor
// and the suspension as they are at x.
void BassSpeaker::matrix(double x, double A[kN][kN]) const {
  for (int r = 0; r < kN; ++r) for (int c = 0; c < kN; ++c) A[r][c] = 0;
  const double q = (x - s_.offset) / xbl_, bl = bl0_ / (1 + q * q);
  const double k = kms_ * (1 + (x / xk_) * (x / xk_)) + kb_;
  A[0][0] = -(s_.re + rsrc_) / s_.le;
  A[0][2] = -bl / s_.le;
  A[1][2] = 1;
  A[2][0] = bl / mms_;
  A[2][1] = -k / mms_;
  A[2][2] = -rms_ / mms_;
  if (n_ == 5) {
    const double r = s_.sp / s_.sd;
    A[2][3] = -kb_ * r / mms_;
    A[3][4] = 1;
    A[4][1] = -kb_ * r / mp_;
    A[4][3] = -kb_ * r * r / mp_;
    A[4][4] = -rp_ / mp_;
  }
}

double BassSpeaker::run(double u) {
  // The head's voltage; through the transformer (its magnetizing current, explicit: the core is
  // slow against the sample).
  double vt = u * s_.volts;
  if (lm_ > 0) {
    const double p = phi_ / s_.otSat, p2 = p * p;
    vt -= s_.otR * phi_ / lm_ * (1 + p2 * p2);
    phi_ += h_ * vt;
  }
  const double e = vt / s_.series;
  // The trapezoid, the motor and suspension frozen where the cone is.
  double A[kN][kN], M[5][5], y[5];
  matrix(st_[1], A);
  const int n = n_;
  for (int r = 0; r < n; ++r) {
    double a = 0;
    for (int c = 0; c < n; ++c) {
      a += A[r][c] * st_[c];
      M[r][c] = (r == c ? 1.0 : 0.0) - 0.5 * h_ * A[r][c];
    }
    y[r] = st_[r] + 0.5 * h_ * a;
  }
  y[0] += 0.5 * h_ * (uLast_ + e) / s_.le;
  uLast_ = e;
  solve(n, M, y);
  const double dv = y[2] - st_[2], dvp = n == 5 ? y[4] - st_[4] : 0;
  for (int r = 0; r < n; ++r) st_[r] = std::isfinite(y[r]) ? y[r] : 0;
  xPeak_ = std::max(xPeak_, std::fabs(st_[1]));
  return (dv + s_.portHeard * (s_.sp / s_.sd) * dvp) * fs_;
}

std::complex<double> BassSpeaker::response(double f) const {
  using C = std::complex<double>;
  const C z1 = std::polar(1.0, -2 * kPi * f / fs_);
  const C sb = 2 * fs_ * (1.0 - z1) / (1.0 + z1);
  C vt = s_.volts;
  if (lm_ > 0) vt /= 1.0 + (s_.otR / lm_) * h_ * z1 / (1.0 - z1);
  double A[kN][kN];
  matrix(0, A);
  C M[5][5], y[5] = {};
  for (int r = 0; r < n_; ++r)
    for (int c = 0; c < n_; ++c) M[r][c] = (r == c ? sb : C(0)) - A[r][c];
  y[0] = vt / double(s_.series) / s_.le;
  solve(n_, M, y);
  const C d = (1.0 - z1) * fs_;
  return d * (y[2] + (n_ == 5 ? s_.portHeard * (s_.sp / s_.sd) * y[4] : C(0)));
}

}  // namespace pajupilli
