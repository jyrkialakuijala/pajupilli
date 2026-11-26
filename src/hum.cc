#include "hum.h"
#include "vmath.h"

#include <algorithm>
#include <cmath>

namespace pajupilli {

void Hum::init(double fs, const Params& p) {
  fs_ = fs;
  p_ = p;
  // The murmur's low-pass (RBJ, Q 0.707): fixed.
  const double w0 = 2 * M_PI * std::min(p_.lpHz, 0.45 * fs_) / fs_, alpha = std::sin(w0) / (2 * M_SQRT1_2);
  const double cw = std::cos(w0), a0 = 1 + alpha;
  lb0_ = (1 - cw) / 2 / a0;
  la1_ = -2 * cw / a0;
  la2_ = (1 - alpha) / a0;
  amt_ = 0;
  set_ = -1;
  reset();
  coefs();
}

void Hum::reset() {
  zx1_ = zx2_ = py1_ = py2_ = 0;
  ly1x_ = ly2x_ = ly1_ = ly2_ = 0;
}

void Hum::control(double amount, double dt) {
  const double a = std::clamp(amount, 0.0, 1.0);
  amt_ += (1 - std::exp(-dt / (a > amt_ ? 0.02 : 0.04))) * (a - amt_);
  if (std::fabs(amt_ - a) < 1e-4) amt_ = a;
  if (std::fabs(amt_ - set_) > 1e-4) coefs();
}

void Hum::coefs() {
  set_ = amt_;
  // Klatt's resonator, unity at DC: y = a x + b y1 + c y2; the zero its inverse.
  auto res = [&](double f, double& a, double& b, double& c) {
    const double r = std::exp(-M_PI * p_.bwHz / fs_);
    c = -r * r;
    b = 2 * r * std::cos(2 * M_PI * f / fs_);
    a = 1 - b - c;
  };
  res(p_.poleHz, pa_, pb_, pc_);
  const double fz = p_.poleHz + amt_ * (p_.zeroHz - p_.poleHz);
  double a, b, c;
  res(fz, a, b, c);
  za_ = 1 / a, zb_ = -b / a, zc_ = -c / a;
  lpMix_ = amt_;
  g_ = fastPow(10.0, amt_ * p_.levelDb / 20);
}

}  // namespace pajupilli
