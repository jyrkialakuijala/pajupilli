#include "hihat.h"

#include <algorithm>
#include <cmath>
#include <complex>

#include "knob.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRho = 1.2, kC = 343.0;
}  // namespace

HiHatSpec hiHatSpec() {
  HiHatSpec s;
  // 14" jazz hats: a thin top, a heavier bottom.
  s.top.thickness = knob("PJ_HAT_THICK", 1.15e-3);
  s.bottom.thickness = knob("PJ_HAT_THICK2", 1.5e-3);
  s.top.top = s.bottom.top = knob("PJ_HAT_TOP", 16000);
  s.top.domeRadius = s.bottom.domeRadius = knob("PJ_HAT_DOME", 0.8);
  s.top.lossA = s.bottom.lossA = knob("PJ_HAT_LOSSA", 0.0025);
  s.top.lossB = s.bottom.lossB = knob("PJ_HAT_LOSSB", 0.7);
  s.top.seed = 11;
  s.bottom.seed = 12;
  s.rimK = knob("PJ_HAT_RIMK", 1.5e5);
  s.rimLoss = knob("PJ_HAT_RIMLOSS", 20);
  s.rimRub = knob("PJ_HAT_RIMRUB", 60);
  s.tilt = knob("PJ_HAT_TILT", 0.010);
  s.wobbleK = knob("PJ_HAT_WOBBLEK", 5);
  s.wobbleLoss = knob("PJ_HAT_WOBBLELOSS", 0.06);
  s.openGap = knob("PJ_HAT_GAP", 6e-3);
  s.cavity = knob("PJ_HAT_CAVITY", 2.5e-3);
  s.jet = knob("PJ_HAT_JET", 1e-3);
  s.leak = knob("PJ_HAT_LEAK", 0.3e-3);
  s.under = knob("PJ_HAT_UNDER", 0.5);
  s.gain = knob("PJ_HAT_GAIN", 1);
  s.brush = BrushSpec();
  // On bronze nothing gives but the wire itself, which bends: a soft, light
  // touch for each end, and many of them, each on its own (a scatter, not
  // one blow).
  s.brush.tips = int(knob("PJ_HAT_BRUSHTIPS", 48));
  s.brush.tipMass = knob("PJ_HAT_BRUSHTIPMASS", 2.5e-5);
  s.brush.contact = knob("PJ_HAT_BRUSHK", 1.2e4);
  s.brush.together = knob("PJ_HAT_BRUSHTOGETHER", 0.1);
  s.brush.tipLoss = knob("PJ_HAT_BRUSHTIPLOSS", 0.15);
  s.brush.fanLoss = knob("PJ_HAT_BRUSHFANLOSS", 0.4);
  s.brush.rough = knob("PJ_HAT_ROUGH", 3e-6);    // (lathing grooves)
  s.brush.grain = 0.3e-3;
  return s;
}

HiHat::HiHat(double fs, const HiHatSpec& spec)
    : s_(spec), fs_(fs), dt_(1 / fs), top_(fs, spec.top), bot_(fs, spec.bottom) {
  brush_.reset(new Brush(fs, top_, spec.brush, 7));
  np_ = s_.patches;
  auto upTo = [&](Plate& p) {
    int n = 0;
    while (n < p.modes() && p.frequency(n) < s_.rimTop) ++n;
    return std::min((n + 15) / 16 * 16, p.modes());
  };
  nt_ = upTo(top_);
  nb_ = upTo(bot_);
  phT_.assign(size_t(np_) * nt_, 0);
  phB_.assign(size_t(np_) * nb_, 0);
  px_.resize(np_); py_.resize(np_); fRim_.assign(np_, 0);
  std::vector<float> ph(std::max(top_.modes(), bot_.modes()));
  const double rc = s_.rimAt * top_.radius();
  for (int p = 0; p < np_; ++p) {
    const double th = 2 * kPi * (p + 0.37) / np_;
    px_[p] = float(rc * std::cos(th)); py_[p] = float(rc * std::sin(th));
    top_.shape(rc, th, ph.data());
    std::copy(ph.begin(), ph.begin() + nt_, phT_.begin() + size_t(p) * nt_);
    bot_.shape(rc, th, ph.data());
    std::copy(ph.begin(), ph.begin() + nb_, phB_.begin() + size_t(p) * nb_);
  }
  // What a rim point's two sides give in speed, in one sample, per unit force.
  rimG_.resize(np_);
  {
    const double mT = top_.mass(), mB = bot_.mass(), R = top_.radius();
    for (int p = 0; p < np_; ++p) {
      double g = 1 / mT + 1 / mB + rc * rc * (4 / (mT * R * R) + 4 / (mB * R * R));
      for (int k = 0; k < nt_; ++k) g += double(phT_[size_t(p) * nt_ + k]) * phT_[size_t(p) * nt_ + k];
      for (int k = 0; k < nb_; ++k) g += double(phB_[size_t(p) * nb_ + k]) * phB_[size_t(p) * nb_ + k];
      rimG_[p] = float(dt_ * g);
    }
  }
  sPhi_.assign(top_.modes(), 0);
  for (int which = 0; which < 2; ++which) {
    Plate& h = which ? bot_ : top_;
    std::vector<float>& rub = which ? rubB_ : rubT_;
    rub.assign((h.modes() + 15) / 16 * 16, 0.0f);
    h.ringShare(rc, rub.data());
    // (Against the middle mode's share: the modes that live at the rim
    // alone, many diameters and no circle, have many times that and go first.)
    std::vector<float> sorted(rub.begin(), rub.begin() + h.modes());
    std::nth_element(sorted.begin(), sorted.begin() + sorted.size() / 2, sorted.end());
    const double mid = sorted[sorted.size() / 2];
    for (float& x : rub) x = float(std::min(x / mid, 20.0) * s_.rimRub / 30);
  }
  // The microphones: each mode by its shape, summed over the plate with
  // every patch's distance and delay at the mode's pitch. A plate in the
  // open sounds from both faces, opposite: below about c / (2 pi R) they
  // cancel, the more the lower the pitch.
  const int kR = 20, kT = 64;
  double fCoin[2];
  for (int which = 0; which < 2; ++which) {
    const PlateSpec& ps = which ? s_.bottom : s_.top;
    fCoin[which] = kC * kC * std::sqrt(12.0) / (2 * kPi * ps.thickness * std::sqrt(ps.E / (ps.rho * (1 - ps.nu * ps.nu))));
  }
  for (int which = 0; which < 2; ++which) {
    Plate& h = which ? bot_ : top_;
    std::vector<float> pm(h.modes());
    for (int mic = 0; mic < 2; ++mic) {
      std::vector<std::complex<double>> g(h.modes());
      const double mx = (mic ? 1 : -1) * s_.micX, mz = s_.micZ + (which ? 0.03 : 0.0);
      const double d0 = std::hypot(mx, mz);
      for (int ir = 0; ir < kR; ++ir)
        for (int it = 0; it < kT; ++it) {
          const double r = h.radius() * (ir + 0.5) / kR, th = 2 * kPi * (it + 0.5) / kT;
          const double dA = 2 * kPi * r * (h.radius() / kR) / kT;
          h.shape(r, th, pm.data());
          const double ddx = r * std::cos(th) - mx, ddy = r * std::sin(th);
          const double d = std::sqrt(ddx * ddx + ddy * ddy + mz * mz);
          for (int k = 0; k < h.modes(); ++k) {
            const double kk = 2 * kPi * h.frequency(k) / kC;
            g[k] += double(pm[k]) * dA * std::exp(std::complex<double>(0, -kk * (d - d0))) / d;
          }
        }
      std::vector<float>& out = which ? micB_[mic] : micT_[mic];
      out.assign((h.modes() + 15) / 16 * 16, 0.0f);
      for (int k = 0; k < h.modes(); ++k) {
        // (Below the pitch where the plate's bending waves outrun sound,
        // only its edge sounds: the share falls as sqrt(f / that pitch).)
        const double eff = std::min(1.0, std::sqrt(h.frequency(k) / fCoin[which]));
        out[k] = float((g[k].real() < 0 ? -1 : 1) * std::abs(g[k]) * kRho / (2 * kPi) * eff * (which ? s_.under : 1.0));
      }
    }
  }
  reset();
}

void HiHat::reset() {
  top_.reset();
  bot_.reset();
  t_ = Body();
  b_ = Body();
  b_.ax = s_.tilt;
  // Closed, the foot resting on the pedal.
  target_ = 0;
  rod_ = -force_ / s_.clutchK;
  rodV_ = 0;
  p_ = u_ = 0;
  press_ = 0;
  stickOn_ = false;
  prev_[0] = prev_[1] = hp_[0] = hp_[1] = hpIn_[0] = hpIn_[1] = 0;
  // Let it settle onto the lower plate.
  asleep_ = false;
  t_.ax = s_.tilt;
  for (int i = 0; i < int(0.2 * fs_); ++i) sample();
  prev_[0] = prev_[1] = hp_[0] = hp_[1] = 0;
  asleep_ = true;
  quiet_ = 0;
}

void HiHat::pedal(double open, double speed, double force, double leanX, double leanY) {
  target_ = std::clamp(open, 0.0, 1.0);
  speed_ = std::max(speed, 0.01);
  force_ = std::max(force, 1.0);
  leanX_ = leanX; leanY_ = leanY;
  wake();
}

double HiHat::gap() const {
  double g = 1;
  for (int p = 0; p < np_; ++p)
    g = std::min(g, (t_.z + t_.ax * px_[p] + t_.ay * py_[p]) - (b_.z + b_.ax * px_[p] + b_.ay * py_[p]));
  return g;
}

double HiHat::tiltOfTop() const { return std::hypot(t_.ax - b_.ax, t_.ay - b_.ay); }

void HiHat::stick(double speed, double r, double theta, bool shoulder) {
  wake();
  sx_ = r * std::cos(theta); sy_ = r * std::sin(theta);
  top_.shape(r, theta, sPhi_.data());
  double g = 0;
  for (int k = 0; k < top_.modes(); ++k) g += double(sPhi_[k]) * sPhi_[k];
  sm_ = shoulder ? 0.030 : 0.012;       // what of the stick is behind the blow (kg)
  sK_ = shoulder ? 4e8 : 1.2e9;         // hickory on bronze: 4/3 E sqrt(R) (N/m^1.5)
  // What the plate gives at that place in one sample, per unit force: its
  // modes, and the plate as a body (its mass, and its tilt by the lever).
  const double m = top_.mass(), I = m * top_.radius() * top_.radius() / 4;
  sG_ = dt_ * dt_ * (g + 1 / m + r * r / I);
  sz_ = t_.z + t_.ax * sx_ + t_.ay * sy_ + top_.height(sPhi_.data()) + 0.05e-3;
  sv_ = -speed;
  stickOn_ = true;
  sAge_ = 0;
}

float HiHat::sample() {
  if (asleep_) {
    if (!stickOn_ && brush_->up()) { side_ = 0; return 0; }
    asleep_ = false;
    quiet_ = 0;
  }
  // The rod: to where the foot wants it.
  const double want = target_ > 0 ? target_ * s_.openGap : -force_ / s_.clutchK;
  rodV_ = std::clamp((want - rod_) / 0.003, -speed_, speed_);
  rod_ += rodV_ * dt_;
  const double mT = top_.mass(), mB = bot_.mass();
  const double iT = mT * top_.radius() * top_.radius() / 4, iB = mB * bot_.radius() * bot_.radius() / 4;
  double fzT = s_.clutchK * (rod_ - t_.z) + 2 * s_.clutchLoss * std::sqrt(s_.clutchK * mT) * (rodV_ - t_.vz);
  double txT = -s_.wobbleK * (t_.ax - leanX_) - 2 * s_.wobbleLoss * std::sqrt(s_.wobbleK * iT) * t_.wx;
  double tyT = -s_.wobbleK * (t_.ay - leanY_) - 2 * s_.wobbleLoss * std::sqrt(s_.wobbleK * iT) * t_.wy;
  double fzB = -s_.seatK * b_.z - 2 * s_.seatLoss * std::sqrt(s_.seatK * mB) * b_.vz;
  double txB = -s_.seatTiltK * (b_.ax - s_.tilt) - 2 * s_.seatTiltLoss * std::sqrt(s_.seatTiltK * iB) * b_.wx;
  double tyB = -s_.seatTiltK * b_.ay - 2 * s_.seatTiltLoss * std::sqrt(s_.seatTiltK * iB) * b_.wy;
  // The rims.
  const float *qT = top_.q(), *vT = top_.v(), *qB = bot_.q(), *vB = bot_.v();
  float *fT = top_.force(), *fB = bot_.force();
  touching_ = 0;
  double area = 0;
  const double ds = 2 * kPi * s_.rimAt * top_.radius() / np_;
  const double rigidGap = gap();
  const bool near = rigidGap < 1.5e-3;
  for (int p = 0; p < np_; ++p) {
    double g = (t_.z + t_.ax * px_[p] + t_.ay * py_[p]) - (b_.z + b_.ax * px_[p] + b_.ay * py_[p]);
    double gv = (t_.vz + t_.wx * px_[p] + t_.wy * py_[p]) - (b_.vz + b_.wx * px_[p] + b_.wy * py_[p]);
    float f = 0;
    if (near) {
      const float *a = &phT_[size_t(p) * nt_], *b = &phB_[size_t(p) * nb_];
      float zt = 0, wt = 0, zb = 0, wb = 0;
      for (int k = 0; k < nt_; ++k) { zt += a[k] * qT[k]; wt += a[k] * vT[k]; }
      for (int k = 0; k < nb_; ++k) { zb += b[k] * qB[k]; wb += b[k] * vB[k]; }
      g += zt - zb;
      gv += wt - wb;
      if (g < 0) {
        // Pressed together the rims rub: a loss that grows with the press
        // (the foot's weight sets how fast the closed pair dies). The force
        // is found for the end of the sample, as both rims give under it.
        const double c = s_.rimLoss * s_.rimK * -g;
        f = float(std::max(0.0, (s_.rimK * -g - c * gv) / (1 + (c + s_.rimK * dt_) * rimG_[p])));
        ++touching_;
        for (int k = 0; k < nt_; ++k) fT[k] += a[k] * f;
        for (int k = 0; k < nb_; ++k) fB[k] -= b[k] * f;
        fzT += f; txT += f * px_[p]; tyT += f * py_[p];
        fzB -= f; txB -= f * px_[p]; tyB -= f * py_[p];
      }
    }
    fRim_[p] = f;
    area += std::max(g, s_.leak) * ds;
  }
  {
    double sum = 0;
    for (int p = 0; p < np_; ++p) sum += fRim_[p];
    press_ += 0.01f * (float(sum) - press_);
    if (press_ > 0.01f) {
      top_.rub(rubT_.data(), press_);
      bot_.rub(rubB_.data(), press_);
    }
  }
  // The air between: squeezed out through the rims' gap (the faster the
  // narrower), its pressure on both plates.
  {
    const double S = 0.9 * kPi * top_.radius() * top_.radius();
    const double a = kRho * kC * kC / s_.cavity * dt_, dV = S * (t_.vz - b_.vz), u0 = 0.5;
    double p = p_;
    for (int it = 0; it < 6; ++it) {
      const double root = std::sqrt(2 * std::fabs(p) / kRho + u0 * u0);
      const double Q = area * p / (kRho * root), dQ = area / (kRho * root) * (1 - std::fabs(p) / (kRho * root * root));
      const double F = p - p_ - a * (-dV - Q);
      p -= F / (1 + a * dQ);
    }
    p_ = p;
    u_ = std::sqrt(2 * std::fabs(p) / kRho);
    fzT += p * S;
    fzB -= p * S;
  }
  // The stick: its tip against the plate where it is, the force found for
  // the end of the sample (a hard contact: the plate gives under it).
  if (stickOn_) {
    const double h = t_.z + t_.ax * sx_ + t_.ay * sy_ + top_.height(sPhi_.data());
    const double hv = t_.vz + t_.wx * sx_ + t_.wy * sy_ + top_.speed(sPhi_.data());
    const double free = (h - sz_) + dt_ * (hv - sv_), G = sG_ + dt_ * dt_ / sm_;
    double f = 0;
    if (free > 0) {
      double lo = 0, hi = free;  // the dent left once the force has pushed both apart
      for (int it = 0; it < 30; ++it) {
        const double x = 0.5 * (lo + hi);
        if (x + G * sK_ * x * std::sqrt(x) > free) hi = x; else lo = x;
      }
      const double x = 0.5 * (lo + hi);
      f = sK_ * x * std::sqrt(x);
    }
    sv_ += dt_ * f / sm_;
    sz_ += dt_ * sv_;
    if (f > 0) {
      top_.push(sPhi_.data(), float(-f));
      fzT -= f; txT -= f * sx_; tyT -= f * sy_;
    }
    if (++sAge_ > int(0.02 * fs_) || (f == 0 && sv_ > 0)) stickOn_ = false;
  }
  brush_->step();
  // The two bodies.
  t_.vz += dt_ * fzT / mT; t_.z += dt_ * t_.vz;
  t_.wx += dt_ * txT / iT; t_.ax += dt_ * t_.wx;
  t_.wy += dt_ * tyT / iT; t_.ay += dt_ * t_.wy;
  b_.vz += dt_ * fzB / mB; b_.z += dt_ * b_.vz;
  b_.wx += dt_ * txB / iB; b_.ax += dt_ * b_.wx;
  b_.wy += dt_ * tyB / iB; b_.ay += dt_ * b_.wy;
  top_.step();
  bot_.step();
  // The microphones; the rush of air as its own hiss (its strength as the
  // cube of its speed, by the gap it comes through).
  const float rMic = float(std::hypot(s_.micX, s_.micZ));
  const float jet = float(kRho * u_ * u_ * u_ * std::sqrt(s_.jet * area / (2 * kPi)) / (kC * rMic));
  float y[2];
  for (int mic = 0; mic < 2; ++mic) {
    float s = 0;
    const float *mt = micT_[mic].data(), *mb = micB_[mic].data();
    for (int k = 0; k < top_.modes(); ++k) s += mt[k] * vT[k];
    for (int k = 0; k < bot_.modes(); ++k) s += mb[k] * vB[k];
    y[mic] = (s - prev_[mic]) * float(fs_);
    prev_[mic] = s;
    rng_ = rng_ * 1664525u + 1013904223u;
    const float w = ((rng_ >> 8) * (1.0f / 8388608.0f) - 1.0f) * 1.73f;  // unit rms
    const float a = float(std::exp(-2 * kPi * 1500 * dt_));
    hp_[mic] = a * (hp_[mic] + w - hpIn_[mic]);
    hpIn_[mic] = w;
    y[mic] += jet * hp_[mic];
  }
  const float g = float(s_.gain);
  const float l = g * y[0], r = g * y[1];
  side_ = 0.5f * (l - r);
  const float mono = 0.5f * (l + r);
  if (!std::isfinite(mono) || !std::isfinite(t_.z)) { reset(); return 0; }
  const bool still = std::fabs(want - rod_) < 1e-6 && std::fabs(t_.vz) < 1e-4;
  if (std::fabs(mono) < 1e-6f && still && !stickOn_ && brush_->up()) {
    if (++quiet_ > int(0.3 * fs_)) asleep_ = true;
  } else {
    quiet_ = 0;
  }
  return mono;
}

}  // namespace pajupilli
