#include "plate.h"

#include <algorithm>
#include <cmath>

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
double urand(uint32_t& s) {
  s = s * 1664525u + 1013904223u;
  return (s >> 8) * (1.0 / 16777216.0);
}
double jd(int m, double x) {  // J_m'
  return m == 0 ? -std::cyl_bessel_j(1.0, x) : std::cyl_bessel_j(double(m - 1), x) - m / x * std::cyl_bessel_j(double(m), x);
}
double idr(int m, double x) {  // I_m' / I_m
  return m == 0 ? std::cyl_bessel_i(1.0, x) / std::cyl_bessel_i(0.0, x)
                : std::cyl_bessel_i(double(m - 1), x) / std::cyl_bessel_i(double(m), x) - m / x;
}
// The free edge: no bending moment and no shear there. For the shape
// J_m(l r / R) + C I_m(l r / R): the two conditions' terms (the I ones per
// unit I_m(l)), and their determinant, zero at a mode.
struct Edge { double aJ, aI, bJ, bI; };
Edge edge(int m, double l, double nu) {
  const double J = std::cyl_bessel_j(double(m), l), Jp = jd(m, l), Ir = idr(m, l), m2 = double(m) * m;
  Edge e;
  e.aJ = -(l * l * J + (1 - nu) * (l * Jp - m2 * J));
  e.aI = l * l - (1 - nu) * (l * Ir - m2);
  e.bJ = -(l * l * l * Jp + (1 - nu) * m2 * (l * Jp - J));
  e.bI = l * l * l * Ir - (1 - nu) * m2 * (l * Ir - 1);
  return e;
}
double det(int m, double l, double nu) {
  const Edge e = edge(m, l, nu);
  return e.aJ * e.bI - e.aI * e.bJ;
}
}  // namespace

Plate::Plate(double fs, const PlateSpec& spec) : spec_(spec), fs_(fs), dt_(1 / fs) {
  const double R = spec.radius, h = spec.thickness;
  const double kappa = h * std::sqrt(spec.E / (12 * spec.rho * (1 - spec.nu * spec.nu)));  // sqrt(D / rho h)
  const double lmax = std::sqrt(2 * kPi * spec.top * R * R / kappa);
  const double fDome = spec.domeRadius > 0 ? std::sqrt(spec.E / spec.rho) / (2 * kPi * spec.domeRadius) : 0;
  struct Mode { double f; int m, nc, fam, kind; double norm, c0, s0, mid; };
  std::vector<Mode> modes;
  uint32_t rng = 60013u + 7919u * spec.seed;
  int fam = 0;
  for (int m = 0; m < 200; ++m) {
    std::vector<double> roots;
    // (m = 0 and 1 have a root at 0: the plate moving and tilting as a body.)
    double x0 = m < 2 ? 1.0 : std::max(0.6 * m, 1.0), y0 = det(m, x0, spec.nu);
    for (double x = x0 + 0.04; x < lmax + 0.04; x += 0.04) {
      const double y = det(m, x, spec.nu);
      if ((y0 < 0) != (y < 0)) {
        double a = x - 0.04, b = x, ya = y0;
        for (int it = 0; it < 50; ++it) {
          const double mid = 0.5 * (a + b), ym = det(m, mid, spec.nu);
          if ((ya < 0) != (ym < 0)) b = mid; else { a = mid; ya = ym; }
        }
        if (0.5 * (a + b) <= lmax) roots.push_back(0.5 * (a + b));
      }
      y0 = y;
    }
    if (roots.empty()) {
      if (m > 3) break;
      continue;
    }
    maxM_ = m;
    for (size_t i = 0; i < roots.size(); ++i) {
      const double l = roots[i];
      const int nc = int(i) + (m < 2 ? 1 : 0);  // nodal circles
      const Edge e = edge(m, l, spec.nu);
      const double C = -e.aJ / e.aI, Il = std::cyl_bessel_i(double(m), l);
      tab_.resize(size_t(fam + 1) * (kRadii + 1));
      float* t = &tab_[size_t(fam) * (kRadii + 1)];
      double sum = 0, curv = 0;
      for (int j = 0; j <= kRadii; ++j) {
        const double x = l * j / kRadii;
        const double w = std::cyl_bessel_j(double(m), x) + C * std::cyl_bessel_i(double(m), x) / Il;
        t[j] = float(w);
        const double r = R * j / kRadii;
        const double e = w * w * r * (R / kRadii) * (j == 0 || j == kRadii ? 0.5 : 1.0);
        sum += e;
        const double c = 1 + spec.domeTaper * (0.5 - double(j) / kRadii);  // the curvature there, against the mean
        curv += e * c * c;
      }
      const double mass = spec.rho * h * sum * (m == 0 ? 2 * kPi : kPi);
      double f = l * l * kappa / (2 * kPi * R * R);
      if (nc > 0 || m == 0) f = std::sqrt(f * f + fDome * fDome * curv / sum);
      const double nrm = 1 / std::sqrt(mass), mid = m == 0 ? t[0] * t[0] * nrm * nrm : 0;
      if (m == 0) {
        modes.push_back({f, 0, nc, fam, 0, nrm, 1, 0, mid});
      } else {
        const double th0 = 2 * kPi * urand(rng), d = spec.split * urand(rng);
        modes.push_back({f * (1 - 0.5 * d), m, nc, fam, 1, nrm, std::cos(m * th0), std::sin(m * th0), 0});
        modes.push_back({f * (1 + 0.5 * d), m, nc, fam, 2, nrm, std::cos(m * th0), std::sin(m * th0), 0});
      }
      ++fam;
    }
  }
  std::sort(modes.begin(), modes.end(), [](const Mode& a, const Mode& b) { return a.f < b.f; });
  while (!modes.empty() && modes.back().f > spec.top) modes.pop_back();
  n_ = int(modes.size());
  const int pad = (n_ + 15) / 16 * 16;
  freq_.resize(n_); m_.resize(n_); nc_.resize(n_); fam_.resize(n_); kind_.resize(n_);
  for (auto* a : {&norm_, &c0_, &s0_, &q_, &v_, &f_, &w2_, &alpha_, &a11_, &a12_, &a21_, &a22_}) a->assign(pad, 0.0f);
  double midMean = 0;
  for (const Mode& md : modes) midMean += md.mid / std::max(n_, 1);
  for (int k = 0; k < n_; ++k) {
    const Mode& md = modes[k];
    freq_[k] = md.f; m_[k] = md.m; nc_[k] = md.nc; fam_[k] = md.fam; kind_[k] = md.kind;
    norm_[k] = float(md.norm); c0_[k] = float(md.c0); s0_[k] = float(md.s0);
    const double w = 2 * kPi * md.f;
    w2_[k] = float(w * w);
    alpha_[k] = float((spec.lossA * std::pow(w, 0.6) + spec.lossB) * (1 + spec.felt * md.mid / std::max(midMean * n_ / 8, 1e-12)));
  }
  coefs();
}

void Plate::coefs() {
  for (int k = 0; k < n_; ++k) {
    const double a = alpha_[k] * damp_, w0 = std::sqrt(double(w2_[k]));
    const double w = std::sqrt(std::max(w0 * w0 - a * a, 1.0));
    const double g = std::exp(-a * dt_), c = std::cos(w * dt_), s = std::sin(w * dt_);
    a11_[k] = float(g * (c + a / w * s));
    a12_[k] = float(g * s / w);
    a21_[k] = float(-g * (w0 * w0 / w) * s);
    a22_[k] = float(g * (c - a / w * s));
  }
}

void Plate::setDamping(double x) {
  if (x == damp_) return;
  damp_ = x;
  coefs();
}

double Plate::mass() const { return spec_.rho * spec_.thickness * kPi * spec_.radius * spec_.radius; }

void Plate::shape(double r, double theta, float* phi) const {
  const double u = std::clamp(r / spec_.radius, 0.0, 1.0) * kRadii;
  const int i = std::min(int(u), kRadii - 1);
  const float fr = float(u - i);
  float cm[256], sm[256];
  const int mm = std::min(maxM_, 255);
  const float c1 = float(std::cos(theta)), s1 = float(std::sin(theta));
  cm[0] = 1; sm[0] = 0;
  for (int m = 1; m <= mm; ++m) {
    cm[m] = cm[m - 1] * c1 - sm[m - 1] * s1;
    sm[m] = sm[m - 1] * c1 + cm[m - 1] * s1;
  }
  for (int k = 0; k < n_; ++k) {
    const float* t = &tab_[size_t(fam_[k]) * (kRadii + 1) + i];
    const float rad = t[0] + fr * (t[1] - t[0]);
    const int m = std::min(m_[k], mm);
    const float c = cm[m] * c0_[k] + sm[m] * s0_[k], s = sm[m] * c0_[k] - cm[m] * s0_[k];
    phi[k] = norm_[k] * rad * (kind_[k] == 2 ? s : c);
  }
}

void Plate::ringShare(double r, float* share) const {
  const double u = std::clamp(r / spec_.radius, 0.0, 1.0) * kRadii;
  const int i = std::min(int(u), kRadii - 1);
  const float fr = float(u - i);
  for (int k = 0; k < n_; ++k) {
    const float* t = &tab_[size_t(fam_[k]) * (kRadii + 1) + i];
    const float w = norm_[k] * (t[0] + fr * (t[1] - t[0]));
    share[k] = w * w * (m_[k] == 0 ? 1.0f : 0.5f);
  }
}

void Plate::step() {
  const float dt = float(dt_);
  for (int k = 0; k < n_; ++k) {
    const float v = v_[k] + dt * f_[k], q = q_[k];
    q_[k] = a11_[k] * q + a12_[k] * v;
    v_[k] = a21_[k] * q + a22_[k] * v;
    f_[k] = 0;
  }
}

void Plate::reset() {
  std::fill(q_.begin(), q_.end(), 0.0f);
  std::fill(v_.begin(), v_.end(), 0.0f);
  std::fill(f_.begin(), f_.end(), 0.0f);
}

double Plate::energy() const {
  double e = 0;
  for (int k = 0; k < n_; ++k) e += 0.5 * (double(v_[k]) * v_[k] + double(w2_[k]) * q_[k] * q_[k]);
  return e;
}

}  // namespace pajupilli
