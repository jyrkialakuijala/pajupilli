#include "membrane.h"

#include <algorithm>
#include <cmath>

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRho = 1.2, kC = 343.0;

uint32_t next(uint32_t& s) {
  s = s * 1664525u + 1013904223u;
  return s;
}
double urand(uint32_t& s) { return (next(s) >> 8) * (1.0 / 16777216.0); }  // 0 .. 1
}  // namespace

Membrane::Membrane(double fs, const HeadSpec& spec) : spec_(spec), fs_(fs), dt_(1 / fs) {
  const double R = spec.radius, c = std::sqrt(spec.tension / spec.sigma);
  const double jmax = 2 * kPi * spec.top * R / c;
  struct Mode { double f; int m, fam, kind; double norm, vol, c0, s0; };
  std::vector<Mode> modes;
  uint32_t rng = 9001u + 7919u * spec.seed;
  int fam = 0;
  for (int m = 0;; ++m) {
    // The zeros of J_m up to jmax (the first lies above m).
    std::vector<double> zeros;
    double x0 = std::max(m + 0.5, 0.5), y0 = std::cyl_bessel_j(double(m), x0);
    for (double x = x0 + 0.05; x < jmax + 0.05; x += 0.05) {
      const double y = std::cyl_bessel_j(double(m), x);
      if ((y0 < 0) != (y < 0)) {
        double a = x - 0.05, b = x, ya = y0;
        for (int it = 0; it < 50; ++it) {
          const double mid = 0.5 * (a + b), ym = std::cyl_bessel_j(double(m), mid);
          if ((ya < 0) != (ym < 0)) b = mid; else { a = mid; ya = ym; }
        }
        if (0.5 * (a + b) <= jmax) zeros.push_back(0.5 * (a + b));
      }
      y0 = y;
    }
    if (zeros.empty()) {
      if (m > jmax) break;
      continue;
    }
    maxM_ = m;
    for (double j : zeros) {
      const double f0 = j * c / (2 * kPi * R);
      const double j1 = std::cyl_bessel_j(double(m + 1), j);
      double mass = spec.sigma * kPi * R * R * j1 * j1 * (m == 0 ? 1.0 : 0.5);
      double vol = 0, f = f0;
      if (m == 0) {
        vol = 2 * kPi * R * R * std::cyl_bessel_j(1.0, j) / j;  // the shape's integral over the head
        const double share = vol / (kPi * R * R);
        const double air = spec.airMass * (8.0 / 3) * kRho * R * R * R * share * share;
        f = f0 * std::sqrt(mass / (mass + air));
        mass += air;
      }
      tab_.resize(size_t(fam + 1) * (kRadii + 1));
      for (int i = 0; i <= kRadii; ++i) tab_[size_t(fam) * (kRadii + 1) + i] = float(std::cyl_bessel_j(double(m), j * i / kRadii));
      const double nrm = 1 / std::sqrt(mass);
      if (m == 0) {
        modes.push_back({f, 0, fam, 0, nrm, vol * nrm, 1, 0});
      } else {
        const double th0 = 2 * kPi * urand(rng), d = spec.split * urand(rng);
        modes.push_back({f * (1 - 0.5 * d), m, fam, 1, nrm, 0, std::cos(m * th0), std::sin(m * th0)});
        modes.push_back({f * (1 + 0.5 * d), m, fam, 2, nrm, 0, std::cos(m * th0), std::sin(m * th0)});
      }
      ++fam;
    }
  }
  std::sort(modes.begin(), modes.end(), [](const Mode& a, const Mode& b) { return a.f < b.f; });
  n_ = int(modes.size());
  const int pad = (n_ + 15) / 16 * 16;
  freq_.resize(n_);
  m_.resize(n_); fam_.resize(n_); kind_.resize(n_);
  norm_.assign(pad, 0); c0_.assign(pad, 0); s0_.assign(pad, 0);
  q_.assign(pad, 0); v_.assign(pad, 0); f_.assign(pad, 0); vol_.assign(pad, 0); w2_.assign(pad, 0);
  a11_.assign(pad, 0); a12_.assign(pad, 0); a21_.assign(pad, 0); a22_.assign(pad, 0); alpha_.assign(pad, 0);
  for (int k = 0; k < n_; ++k) {
    const Mode& md = modes[k];
    freq_[k] = md.f;
    m_[k] = md.m; fam_[k] = md.fam; kind_[k] = md.kind;
    norm_[k] = float(md.norm); c0_[k] = float(md.c0); s0_[k] = float(md.s0);
    vol_[k] = float(md.vol);
    const double w = 2 * kPi * md.f;
    w2_[k] = float(w * w);
    const bool first = md.m == 0 && k == 0;
    const double z = md.m == 0 ? (first ? spec.zeta01 : spec.zeta0) : spec.zeta;
    alpha_[k] = float(z * w * (1 + md.f / spec.lossKnee));
  }
  // Berger's mean tension: T rises by E h / (2 pi R^2 (1 - nu^2)) times the
  // integral of the slope squared, which is the sum of w^2 q^2 / T.
  stretchK_ = float(spec.stretch / (2 * kPi * R * R * spec.tension * spec.tension));
  coefs();
}

void Membrane::coefs() {
  for (int k = 0; k < n_; ++k) {
    const double a = alpha_[k] * damp_, w0 = std::sqrt(double(w2_[k]));
    const double w = std::sqrt(std::max(w0 * w0 - a * a, 1.0));
    const double g = std::exp(-a * dt_), c = std::cos(w * dt_), s = std::sin(w * dt_);
    // (q, v) one sample on, exactly, for the damped swing.
    a11_[k] = float(g * (c + a / w * s));
    a12_[k] = float(g * s / w);
    a21_[k] = float(-g * (w0 * w0 / w) * s);
    a22_[k] = float(g * (c - a / w * s));
  }
}

void Membrane::setDamping(double x) {
  if (x == damp_) return;
  damp_ = x;
  coefs();
}

void Membrane::shape(double r, double theta, float* phi) const {
  const double u = std::clamp(r / spec_.radius, 0.0, 1.0) * kRadii;
  const int i = std::min(int(u), kRadii - 1);
  const float fr = float(u - i);
  float cm[128], sm[128];
  const int mm = std::min(maxM_, 127);
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

void Membrane::step() {
  const float dt = float(dt_);
  if (stretchK_ > 0) {
    float e = 0;
    for (int k = 0; k < n_; ++k) e += w2_[k] * q_[k] * q_[k];
    const float g = stretchK_ * e;  // the tension's rise, as a share
    for (int k = 0; k < n_; ++k) f_[k] -= g * w2_[k] * q_[k];
  }
  for (int k = 0; k < n_; ++k) {
    const float v = v_[k] + dt * f_[k], q = q_[k];
    q_[k] = a11_[k] * q + a12_[k] * v;
    v_[k] = a21_[k] * q + a22_[k] * v;
    f_[k] = 0;
  }
}

void Membrane::reset() {
  std::fill(q_.begin(), q_.end(), 0.0f);
  std::fill(v_.begin(), v_.end(), 0.0f);
  std::fill(f_.begin(), f_.end(), 0.0f);
}

double Membrane::energy() const {
  double e = 0;
  for (int k = 0; k < n_; ++k) e += 0.5 * (double(v_[k]) * v_[k] + double(w2_[k]) * q_[k] * q_[k]);
  return e;
}

float DrumAir::couple(Membrane& top, Membrane& bottom, double dt) {
  double dv = 0;
  const float *qt = top.q(), *vt = top.volume(), *qb = bottom.q(), *vb = bottom.volume();
  for (int k = 0; k < top.modes(); ++k) dv += double(vt[k]) * qt[k];
  for (int k = 0; k < bottom.modes(); ++k) dv -= double(vb[k]) * qb[k];
  leak += (dv - leak) * (1 - std::exp(-2 * kPi * vent * dt));  // the vent lets the slow part out
  const float p = float(-kRho * kC * kC / volume * (dv - leak));
  float *ft = top.force(), *fb = bottom.force();
  for (int k = 0; k < top.modes(); ++k) ft[k] += p * vt[k];
  for (int k = 0; k < bottom.modes(); ++k) fb[k] -= p * vb[k];
  return p;
}

}  // namespace pajupilli
