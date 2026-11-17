#include "leslie.h"

#include <algorithm>
#include <cmath>

#include "knob.h"

namespace pajupilli {

namespace {

constexpr double kC = 343;

// A 2nd-order Butterworth section (bilinear): b, a with a[0] = 1 dropped.
void butter(double fs, double fc, bool high, double* b, double* a) {
  const double k = std::tan(M_PI * fc / fs), q = M_SQRT1_2, n = 1 / (1 + k / q + k * k);
  if (high) { b[0] = n; b[1] = -2 * n; b[2] = n; }
  else { b[0] = k * k * n; b[1] = 2 * b[0]; b[2] = b[0]; }
  a[0] = 1;
  a[1] = 2 * (k * k - 1) * n;
  a[2] = (1 - k / q + k * k) * n;
}

double biquad(double x, const double* b, const double* a, double* s) {
  // (transposed direct form II; s[0], s[1])
  const double y = b[0] * x + s[0];
  s[0] = b[1] * x - a[1] * y + s[1];
  s[1] = b[2] * x - a[2] * y;
  return y;
}

}  // namespace

float Leslie::Ring::read(double delay) const {
  const double rp = double(w) - delay;
  const int i = int(std::floor(rp));
  const double t = rp - i;
  const float y0 = b[(i - 1) & mask], y1 = b[i & mask], y2 = b[(i + 1) & mask], y3 = b[(i + 2) & mask];
  // (4-point Lagrange)
  const double c0 = -t * (t - 1) * (t - 2) / 6, c1 = (t + 1) * (t - 1) * (t - 2) / 2;
  const double c2 = -(t + 1) * t * (t - 2) / 2, c3 = (t + 1) * t * (t - 1) / 6;
  return float(c0 * y0 + c1 * y1 + c2 * y2 + c3 * y3);
}

void Leslie::setup(double fs, uint32_t seed) {
  fs_ = fs;
  dt_ = 1 / fs;
  rng_ = seed * 2654435761u + 99u;
  auto urand = [this] { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; };
  // Each cabinet its own pulleys and belts: the speeds ±3 %.
  hornSlow_ = knob("PJ_LESLIE_HORNSLOW", 0.80) * (1 + 0.03 * urand());
  hornFast_ = knob("PJ_LESLIE_HORNFAST", 6.67) * (1 + 0.03 * urand());
  drumSlow_ = knob("PJ_LESLIE_DRUMSLOW", 0.67) * (1 + 0.03 * urand());
  drumFast_ = knob("PJ_LESLIE_DRUMFAST", 5.90) * (1 + 0.03 * urand());
  hornW_ = hornSlow_;
  drumW_ = drumSlow_;
  hornPh_ = M_PI * (1 + urand());
  drumPh_ = M_PI * (1 + urand());
  butter(fs, 800, false, xa_, xb_);  // (the low-pass: xa_; both share the denominator xb_)
  double a[3];
  butter(fs, 800, true, xhb_, a);
  aLo_ = 1 - std::exp(-2 * M_PI * 1500 / fs);
  aMid_ = 1 - std::exp(-2 * M_PI * 4000 / fs);
  aDrum_ = 1 - std::exp(-2 * M_PI * 200 / fs);
  for (Ring& r : horn_) r.init(int(0.01 * fs) + 8);
  drum_.init(int(0.01 * fs) + 8);
  static const int kLen[4] = {223, 293, 367, 449};
  for (int i = 0; i < 4; ++i) {
    fdnLen_[i] = int(kLen[i] * fs / 48000);
    fdn_[i].assign(fdnLen_[i], 0.0f);
    fdnG_[i] = std::pow(10.0, -3.0 * fdnLen_[i] / (fs * knob("PJ_LESLIE_CABT60", 0.04)));
  }
}

void Leslie::process(float in, float& left, float& right) {
  static const double kHornR = knob("PJ_LESLIE_HORNR", 0.17), kDrumR = 0.20, kMic = knob("PJ_LESLIE_MIC", 0.4);
  static const double kWall = 0.37, kRefl = 0.35;
  static const double kBack[3] = {-4, -12, -18};  // the mouth's loss behind it, low / mid / high (dB)
  // The rotors: their motors pull them towards the set speed through belts and their own inertia.
  {
    const double hT = brake_ ? 0 : fast_ ? hornFast_ : hornSlow_, dT = brake_ ? 0 : fast_ ? drumFast_ : drumSlow_;
    static const double hUp = knob("PJ_LESLIE_HORNUP", 0.5), hDown = knob("PJ_LESLIE_HORNDOWN", 0.8);
    static const double dUp = knob("PJ_LESLIE_DRUMUP", 3.5), dDown = knob("PJ_LESLIE_DRUMDOWN", 2.5);
    hornW_ += (hT - hornW_) * dt_ / (hT > hornW_ ? hUp : brake_ ? 1.0 : hDown);
    drumW_ += (dT - drumW_) * dt_ / (dT > drumW_ ? dUp : brake_ ? 4.0 : dDown);
    hornPh_ += 2 * M_PI * hornW_ * dt_;
    drumPh_ += 2 * M_PI * drumW_ * dt_;
    if (hornPh_ > 2 * M_PI) hornPh_ -= 2 * M_PI;
    if (drumPh_ > 2 * M_PI) drumPh_ -= 2 * M_PI;
  }
  // The amp: a soft clip, a little asymmetric (the push-pull pair a little off balance), its
  // drive easing as the supply sags under a loud passage.
  double x = in * drive_ / (1 + 0.6 * sag_);
  const double y = (std::tanh(x + 0.12) - std::tanh(0.12)) / (1 - std::tanh(0.12) * std::tanh(0.12));
  sag_ += (std::fabs(y) - sag_) * dt_ / 0.1;
  const double amp = y / std::max(drive_, 1e-3);  // (unity at a low level: the drive is in the clipping)
  // The crossover.
  const double lo = biquad(amp, xa_, xb_, lp1_);
  const double hi = biquad(amp, xhb_, xb_, hp1_);
  // The horn's bands, into their delay lines.
  hornLo_ += aLo_ * (hi - hornLo_);
  const double rest = hi - hornLo_;
  hornMid_ += aMid_ * (rest - hornMid_);
  horn_[0].push(float(hornLo_));
  horn_[1].push(float(hornMid_));
  horn_[2].push(float(rest - hornMid_));
  // The drum: its fixed low part, its turning upper part.
  drumLo_ += aDrum_ * (lo - drumLo_);
  drum_.push(float(lo - drumLo_));
  const double hc = std::cos(hornPh_), hs = std::sin(hornPh_), dc = std::cos(drumPh_), ds = std::sin(drumPh_);
  const double hx = kHornR * hc, hy = kHornR * hs;
  float mic[2];
  for (int m = 0; m < 2; ++m) {
    const double mx = kMic * 0.5, my = (m == 0 ? 1 : -1) * kMic * 0.866;
    double sum = 0;
    // The horn: the direct path and the two side walls' images (mirrored in y, turning the
    // other way), each with its Doppler and the mouth's beam.
    for (int p = 0; p < 3; ++p) {
      const double sy = p == 0 ? hy : p == 1 ? 2 * kWall - hy : -2 * kWall - hy;
      const double dirY = p == 0 ? hs : -hs;
      const double vx = mx - hx, vy = my - sy, dist = std::sqrt(vx * vx + vy * vy);
      const double cosPsi = (hc * vx + dirY * vy) / dist;
      const double delay = dist / kC * fs_ + 2;
      const double g = (p == 0 ? 1.0 : kRefl) * 0.4 / std::max(dist, 0.1);
      for (int b = 0; b < 3; ++b) sum += g * std::pow(10.0, kBack[b] * (1 - cosPsi) / 40) * horn_[b].read(delay);
    }
    // The base's leakage, not turning.
    sum += 0.25 * (horn_[0].read(60) + horn_[1].read(60) + horn_[2].read(60));
    // The drum's scoop.
    {
      const double dx = kDrumR * dc, dy = kDrumR * ds;
      const double vx = mx - dx, vy = my - dy, dist = std::sqrt(vx * vx + vy * vy);
      const double cosPsi = (dc * vx + ds * vy) / dist;
      sum += (0.3 + 0.7 * 0.5 * (1 + cosPsi)) * 1.2 * drum_.read(dist / kC * fs_ + 2);
    }
    mic[m] = float(sum);
  }
  // The drum's fixed low part, a little later (the lower compartment), to both.
  const float fixedLo = drumFixed_[(dfw_ - 60) & 2047];
  drumFixed_[dfw_] = float(drumLo_);
  dfw_ = (dfw_ + 1) & 2047;
  // The cabinet's tail (Householder FDN).
  double o[4], s = 0;
  for (int i = 0; i < 4; ++i) {
    o[i] = fdn_[i][fdnPos_[i]];
    s += o[i];
  }
  const double feed = 0.15 * (mic[0] + mic[1]);
  for (int i = 0; i < 4; ++i) {
    fdn_[i][fdnPos_[i]] = float(fdnG_[i] * (o[i] - 0.5 * s) + feed);
    fdnPos_[i] = (fdnPos_[i] + 1) % fdnLen_[i];
  }
  left = float(mic[0] + fixedLo + 0.5 * (o[0] - o[2]));
  right = float(mic[1] + fixedLo + 0.5 * (o[1] - o[3]));
}

}  // namespace pajupilli
