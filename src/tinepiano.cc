#include "tinepiano.h"
#include "heard.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "knob.h"
#include "vmath.h"

namespace pajupilli {

bool tinepiano(const Instrument& inst) { return std::strcmp(inst.name, "electric-piano-1") == 0; }

namespace {

// Piecewise linear through (midi, value) points.
double through(const double (*pts)[2], int n, double key) {
  if (key <= pts[0][0]) return pts[0][1];
  for (int i = 1; i < n; ++i)
    if (key <= pts[i][0]) return pts[i - 1][1] + (key - pts[i - 1][0]) / (pts[i][0] - pts[i - 1][0]) * (pts[i][1] - pts[i - 1][1]);
  return pts[n - 1][1];
}

// The tine lengths (mm): the factory's cutting sheet for the 73's keys E1 .. E7 (research/tinepiano.md
// 3.2; its two misprints, keys 33 and 48-50, smoothed), the 88's A0 .. D#1 on E1's tine, F7 .. C8
// from the manual's chart less its 1 mm offset.
const double kLen73[73] = {156.4, 152.4, 148.4, 143.7, 140.5, 135.7, 131.4, 128.6, 125.8, 121.4, 117.9, 113.1,
                           110.7, 108.3, 105.2, 103.2, 100.8, 98.4,  95.2,  92.9,  89.7,  87.3,  84.9,  81.8,
                           79.8,  77.8,  76.2,  74.6,  71.4,  70.2,  67.9,  66.3,  64.9,  63.5,  61.9,  59.9,
                           57.9,  56.4,  54.0,  53.2,  52.0,  50.8,  50.0,  48.4,  46.4,  45.2,  44.1,  43.0,
                           42.0,  41.2,  40.5,  39.7,  38.1,  37.3,  36.1,  34.9,  34.1,  34.1,  32.9,  32.1,
                           31.0,  29.8,  29.4,  28.6,  27.8,  27.0,  27.0,  25.8,  25.0,  24.6,  24.2,  23.8,
                           23.0};
const double kLenTop[8] = {21.3, 20.6, 20.1, 19.3, 18.6, 18.3, 17.6, 17.0};  // F7 .. C8

double tineLength(int midi) {
  if (midi < 28) return 156.4;
  if (midi <= 100) return kLen73[midi - 28];
  return kLenTop[std::min(midi, 108) - 101];
}

// The swaged tine's mode ratios with its spring (3.3), the spring's mass (g).
const double kF2[][2] = {{21, 9.2}, {24, 7.9}, {28, 6.74}, {36, 6.79}, {40, 7.05}, {48, 7.01}, {60, 7.01}, {64, 7.40},
                         {72, 7.69}, {76, 7.92}, {84, 7.02}, {88, 6.94}, {96, 6.17}, {100, 5.81}};
const double kF3[][2] = {{21, 28}, {24, 24}, {28, 19.5}, {36, 19.6}, {40, 20.5}, {48, 20.1}, {60, 19.1}, {64, 19.6},
                         {72, 18.8}, {76, 19.0}, {84, 17.6}, {88, 19.4}, {96, 20.2}, {100, 19.2}};
const double kSpring[][2] = {{21, 1.3}, {24, 0.75}, {28, 0.27}, {36, 0.24}, {40, 0.30}, {48, 0.24}, {60, 0.18}, {64, 0.23},
                             {72, 0.26}, {76, 0.33}, {84, 0.28}, {88, 0.35}, {96, 0.38}, {100, 0.31}};
// The fundamental's Q (Shear; log-frequency interpolation; 4.2), and its T60 at most 30 s.
const double kQ[][2] = {{39, 949}, {51, 731}, {61, 1300}, {75, 2175}, {87, 1761}, {100, 1500}};
// The strike point as a share of the tine's length (3.6).
const double kStrike[][2] = {{21, 0.375}, {28, 0.375}, {40, 0.45}, {48, 0.49}, {60, 0.54}, {72, 0.565}, {84, 0.495},
                             {96, 0.33}, {100, 0.23}, {108, 0.2}};
// The factory's stretch (cents, 3.4).
const double kStretch[][2] = {{21, -20}, {34, -7}, {35, -6}, {36, -6}, {37, -5}, {39, -5}, {40, -4}, {44, -4}, {45, -3},
                              {53, -3}, {54, -2.5}, {55, -2.5}, {56, -2}, {62, -2}, {63, -1.5}, {64, -1}, {68, -1},
                              {69, 0}, {74, 0}, {75, 1}, {80, 1}, {81, 2}, {84, 2}, {85, 3}, {87, 3}, {88, 4}, {89, 4},
                              {90, 5}, {91, 5}, {92, 6}, {93, 6}, {100, 13}, {101, 15}, {108, 30}};
// The tonebar's own frequency (Muenster & Pfeifle's Mark I, 3.5).
const double kBar[][2] = {{39, 51}, {46, 69}, {53, 79}, {60, 105}, {67, 138}, {74, 183}, {81, 140}, {88, 145}, {95, 222}};
// The damper's release (T60 of a damped note, s): the bass's long tine rings through the felt.
const double kRelease[][2] = {{21, 0.4}, {40, 0.3}, {60, 0.15}, {84, 0.08}, {108, 0.06}};

// The uniform cantilever's modes, normalised to 1 at the tip.
const double kBeta[3] = {1.87510, 4.69409, 7.85476}, kSigma[3] = {0.734096, 1.018467, 0.999224};
double shape(int n, double xi) {
  const double b = kBeta[n] * xi, s = kSigma[n];
  const double v = std::cosh(b) - std::cos(b) - s * (std::sinh(b) - std::sin(b));
  const double bt = kBeta[n];
  const double tip = std::cosh(bt) - std::cos(bt) - s * (std::sinh(bt) - std::sin(bt));
  return v / tip;
}

// The hammer's tip by zone (3.7): Young's modulus (MPa) of its neoprene, or the wrapped maple core.
double tipE(int midi) {
  if (midi <= 50) return 1.1;   // Shore 30, A0 .. D3
  if (midi <= 60) return 2.5;   // Shore 50
  if (midi <= 70) return 5.5;   // Shore 70
  if (midi <= 84) return 21;    // Shore 90
  return 120;                   // maple under shrink tubing
}

unsigned hashKey(unsigned a, unsigned b) {
  unsigned h = 2166136261u ^ (a * 2654435761u) ^ (b * 40503u);
  h ^= h >> 15, h *= 2246822519u, h ^= h >> 13, h *= 3266489917u, h ^= h >> 16;
  return h;
}
double urand(unsigned& h) {  // -1 .. 1
  h ^= h << 13, h ^= h >> 17, h ^= h << 5;
  return (h & 0xffffff) / double(0x800000) - 1;
}

void setRot(double w, double sigma, double dt, double& c, double& s, double& d) {
  c = std::cos(w * dt), s = std::sin(w * dt), d = std::exp(-sigma * dt);
}

// RBJ biquads.
void rbj(double* b, double* a, int type, double f, double q, double gainDb, double fs) {
  const double w = 2 * M_PI * f / fs, cw = std::cos(w), sw = std::sin(w), al = sw / (2 * q);
  const double A = fastPow(10, gainDb / 40);
  double b0, b1, b2, a0, a1, a2;
  if (type == 0) {  // low-pass
    b0 = (1 - cw) / 2, b1 = 1 - cw, b2 = (1 - cw) / 2, a0 = 1 + al, a1 = -2 * cw, a2 = 1 - al;
  } else if (type == 1) {  // high-pass
    b0 = (1 + cw) / 2, b1 = -(1 + cw), b2 = (1 + cw) / 2, a0 = 1 + al, a1 = -2 * cw, a2 = 1 - al;
  } else {  // peak
    b0 = 1 + al * A, b1 = -2 * cw, b2 = 1 - al * A, a0 = 1 + al / A, a1 = -2 * cw, a2 = 1 - al / A;
  }
  b[0] = b0 / a0, b[1] = b1 / a0, b[2] = b2 / a0, a[0] = a1 / a0, a[1] = a2 / a0;
}

constexpr int kSub = 8;              // contact substeps per sample
constexpr double kRhoA = 7850 * M_PI * 0.762e-3 * 0.762e-3;  // the swaged tine's 1.524 mm: kg/m

}  // namespace

TinePianoModel::TinePianoModel(const Instrument& inst) : Tube(inst), dt_(1.0 / inst.fs) {
  static const double kWear = knob("PJ_TINEPIANO_WEAR", 1.0);     // the keys' scatter (0: all as the tables)
  static const double kGap = knob("PJ_TINEPIANO_GAP", 1.0);       // x every gap (smaller: more bark, louder)
  static const double kDelta = knob("PJ_TINEPIANO_VOICING", 1.0); // x the offset over the pole's edge (smaller: bell / overtones)
  static const double kP = knob("PJ_TINEPIANO_P", 1.25);          // the bell's exponent
  static const double kBarQ = knob("PJ_TINEPIANO_BAR_Q", 40);
  static const double kSideDb = knob("PJ_TINEPIANO_SIDE_DB", -20);  // the sideways polarisation's share
  suitcase_ = knob("PJ_TINEPIANO_SUITCASE", 1) > 0;
  const double fs = inst.fs;
  keys_.resize(kKeys);
  for (int k = 0; k < kKeys; ++k) {
    Key& K = keys_[k];
    const int midi = kLowest + k;
    unsigned h = hashKey(unsigned(midi), 7u);
    const double wear = kWear;
    const double cents = through(kStretch, int(sizeof kStretch / sizeof kStretch[0]), midi) + wear * 2.0 * urand(h);
    const double f0 = 440 * std::exp2((midi - 69 + cents / 100) / 12.0);
    const double L = tineLength(midi) * 1e-3;
    const double ms = through(kSpring, 14, midi) * 1e-3;
    double xs = through(kStrike, 10, midi) + wear * 0.0015 / L * urand(h);
    xs = std::clamp(xs, 0.12, 0.8);
    double Q = std::exp(through(kQ, 6, midi)) ;
    {  // (interpolated in log Q over the keys)
      double lq[6][2];
      for (int i = 0; i < 6; ++i) lq[i][0] = kQ[i][0], lq[i][1] = std::log(kQ[i][1]);
      Q = std::exp(through(lq, 6, midi));
    }
    Q *= 1 + 0.15 * wear * urand(h);
    Q = std::min(Q, 30 * M_PI * f0 / 6.91);  // (T60 at most 30 s)
    const double ratios[3] = {1, through(kF2, 14, midi), through(kF3, 14, midi)};
    // Modes 2 and 3: T60 30 .. 150 ms and 10 .. 50 ms, shortest in the bass.
    const double t = std::clamp((midi - 21) / 87.0, 0.0, 1.0);
    const double t60[3] = {6.91 * Q / (M_PI * f0), 0.03 + 0.12 * t, 0.01 + 0.04 * t};
    K.nm = 0;
    for (int n = 0; n < 3; ++n) {
      const double f = f0 * ratios[n];
      if (f > 0.42 * fs || f > 19000) break;
      Mode& m = K.m[n];
      m.w = 2 * M_PI * f;
      m.phiS = shape(n, xs);
      m.mass = kRhoA * L / 4 + ms;  // (tip-normalised: the beam's quarter, the spring at the tip)
      setRot(m.w, 6.91 / t60[n], dt_, m.c, m.s, m.d);
      setRot(m.w, 6.91 / t60[n], dt_ / kSub, m.cs, m.ss, m.ds);
      ++K.nm;
    }
    // The sideways polarisation: mode 1 a fraction of a Hz apart (0.02 .. 0.3 Hz).
    {
      Mode& s = K.side;
      const double split = 0.02 + 0.28 * (0.5 + 0.5 * urand(h));
      s.w = 2 * M_PI * (f0 + split);
      s.phiS = K.m[0].phiS;
      s.mass = K.m[0].mass;
      setRot(s.w, 6.91 / t60[0], dt_, s.c, s.s, s.d);
      setRot(s.w, 6.91 / t60[0], dt_ / kSub, s.cs, s.ss, s.ds);
      K.sideShare = fastPow(10, kSideDb / 20) * (urand(h) > 0 ? 1 : -1) * (0.6 + 0.4 * std::fabs(urand(h)));
    }
    // The tonebar: its own mode, 10 .. 25 x the tine's mass, driven by the tine's root (the
    // inertial shear of mode 1: w^2 rhoA L sigma/beta per m of tip displacement).
    {
      Mode& b = K.bar;
      const double fb = midi < 39 ? 51 * std::exp2((midi - 39) / 24.0) : through(kBar, 9, midi);
      b.w = 2 * M_PI * fb;
      b.mass = 15 * (kRhoA * L + ms);
      setRot(b.w, M_PI * fb / kBarQ, dt_, b.c, b.s, b.d);
      K.barDrive = K.m[0].w * K.m[0].w * kRhoA * L * kSigma[0] / kBeta[0];
    }
    // The pickup: the gap (horizontal, 1.6 mm bass and wood-tip zone, 0.8 mm middle), the bell's
    // width the gap + 1 mm, the tine's rest over the pole's edge (delta / w 0.8 where the factory
    // set "mostly fundamental", 0.45 elsewhere), each +- 0.15 mm (wear 1).
    const bool fund = midi < 43 || midi >= 85;
    K.gap = std::max(0.3e-3, (fund ? 1.6e-3 : 0.8e-3) * kGap + wear * 0.15e-3 * urand(h));
    K.w = K.gap + 1e-3;
    K.delta = std::max(0.0, (fund ? 0.8 : 0.45) * K.w * kDelta + wear * 0.15e-3 * urand(h));
    K.p = kP;
    K.sens = fastPow(10, wear * 1.5 * urand(h) / 20);  // the magnet's strength
    K.damp = 6.91 / through(kRelease, 5, midi);
    // The hammer: Hunt-Crossley (alpha 2.8, Sonderbo's k scaled by the tip's modulus), mass 8 g.
    K.alpha = 2.8;
    K.k = 1.5e11 * tipE(midi) / 5.5;
    K.mu = 0.6;
    K.M = knob("PJ_TINEPIANO_HAMMER_G", 11) * 1e-3;
  }
  // The action: each key's hammer speed scaled so that a blow at 1 m/s leaves its tine's tip
  // moving at PJ_TINEPIANO_TIP m/s (the TinePiano is built for an even touch: the same tip speed, so
  // an amplitude ~ 1 / f, the bass barking first; research/tinepiano.md 4.1): a test blow on each
  // key at rest.
  {
    static const double kTip = knob("PJ_TINEPIANO_TIP", 3.0);
    for (Key& K : keys_) {
      Key T = K;
      T.awake = true;
      T.hammer = true, T.xh = -1e-4, T.vh = 1.0;
      const double dts = dt_ / kSub;
      for (int i = 0; i < int(0.03 / dts) && T.hammer; ++i) {
        double ys = 0, vs = 0;
        for (int n = 0; n < T.nm; ++n) ys += T.m[n].phiS * T.m[n].re, vs -= T.m[n].phiS * T.m[n].w * T.m[n].im;
        const double c = T.xh - ys;
        if (c > 0) {
          const double F = std::max(0.0, T.k * fastPow(c, T.alpha) * (1 + T.mu * (T.vh - vs)));
          for (int n = 0; n < T.nm; ++n) T.m[n].im -= F * T.m[n].phiS / T.m[n].mass * dts / T.m[n].w;
          T.vh -= F / T.M * dts;
        } else if (T.vh < 0) {
          T.hammer = false;
        }
        T.xh += T.vh * dts;
        for (int n = 0; n < T.nm; ++n) rotate(T.m[n], true);
      }
      const double tipV = T.m[0].w * std::hypot(T.m[0].re, T.m[0].im);
      // (below C3 the tip's speed falls with the pitch: the bass's swing stays ~3 mm at mezzo-forte,
      // its A / w ~1 there, 2 .. 3 at fortissimo: research/tinepiano.md 12.1)
      const double f0 = K.m[0].w / (2 * M_PI);
      const double target = kTip * std::min(1.0, f0 / 131.0);
      K.action = tipV > 0 ? std::clamp(target / tipV, 0.05, 5.0) : 1.0;
    }
  }
  // The harp into its load (6.3): V/e = R_L / ((R_h + sL)(1 + s R_L C) + R_L), bilinear.
  {
    const double Rh = 1400, Lh = 0.65;
    const double RL = suitcase_ ? 27e3 : 10e3, C = suitcase_ ? 1.0e-9 : 0.5e-9;
    const double a2 = Lh * C * RL, a1 = Lh + Rh * RL * C, a0 = Rh + RL;
    const double K2 = 2 * fs;
    const double A0 = a2 * K2 * K2 + a1 * K2 + a0, A1 = 2 * a0 - 2 * a2 * K2 * K2, A2 = a2 * K2 * K2 - a1 * K2 + a0;
    // (normalised so that the low frequencies pass at 1: the source's divider is folded into the gain)
    const double g = (Rh + RL) / RL;
    harpB_[0] = g * RL / A0, harpB_[1] = 2 * g * RL / A0, harpB_[2] = g * RL / A0;
    harpA_[1] = A1 / A0, harpA_[2] = A2 / A0;
  }
  // The cabinet (4 x 12 in, closed): -3 dB near 85 Hz, the cones' breakup 2.2 kHz, rolling off
  // over 5 kHz.
  for (int c = 0; c < 2; ++c) {
    double b[3], a[2];
    rbj(b, a, 1, 85, 0.75, 0, fs);
    cab_[c][0] = {b[0], b[1], b[2], a[0], a[1]};
    rbj(b, a, 2, 2200, 1.2, 3, fs);
    cab_[c][1] = {b[0], b[1], b[2], a[0], a[1]};
    rbj(b, a, 0, 5000, 0.7, 0, fs);
    cab_[c][2] = {b[0], b[1], b[2], a[0], a[1]};
  }
  unsigned h = 99;
  for (double& e : humEps_) e = 0.05 * urand(h);
}

void TinePianoModel::resetAir() {
  Tube::resetAir();
  for (Key& K : keys_) {
    for (Mode& m : K.m) m.re = m.im = 0;
    K.side.re = K.side.im = K.bar.re = K.bar.im = 0;
    K.hammer = K.awake = false;
  }
  awake_ = 0;
  hx_[0] = hx_[1] = hy_[0] = hy_[1] = 0;
  side_.clear();
}

const float* TinePianoModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

void TinePianoModel::strike(int key, double speed) {
  if (key < 0 || key >= kKeys) return;
  Key& K = keys_[key];
  // The hammer leaves its rest under the tine's swing: it meets the tine where the tine is.
  double amp = 0;
  for (int n = 0; n < K.nm; ++n) amp += std::fabs(K.m[n].phiS) * std::hypot(K.m[n].re, K.m[n].im);
  K.hammer = true;
  K.xh = -(amp + 1.0e-3);
  K.vh = speed * K.action;
  K.hammerT = 0;
  K.contact = 0;
  K.dampTarget = 0;
  if (!K.awake) {
    K.awake = true;
    ++awake_;
  }
  K.quiet = 0;
}

void TinePianoModel::damp(int key, bool on) {
  if (key < 0 || key >= kKeys) return;
  keys_[key].dampTarget = on ? 1 : 0;
}

inline void TinePianoModel::rotate(Mode& m, bool sub) {
  const double c = sub ? m.cs : m.c, s = sub ? m.ss : m.s, d = sub ? m.ds : m.d;
  const double re = m.re, im = m.im;
  m.re = d * (c * re - s * im);
  m.im = d * (s * re + c * im);
}

float TinePianoModel::sample() {
  static const double kRetreat = knob("PJ_TINEPIANO_RETREAT", 1.0);  // the hammer's fall after a blow (m/s)
  static const double kEmf = knob("PJ_TINEPIANO_GAIN", 2.0e-4);
  static const double kHum = knob("PJ_TINEPIANO_HUM", 1.0), kMains = knob("PJ_MAINS_HZ", 50);
  const double dts = dt_ / kSub;
  double emf = 0;
  for (Key& K : keys_) {
    if (!K.awake) continue;
    // The hammer: substeps while it flies or touches.
    if (K.hammer) {
      for (int s = 0; s < kSub; ++s) {
        double ys = 0, vs = 0;
        for (int n = 0; n < K.nm; ++n) {
          ys += K.m[n].phiS * K.m[n].re;
          vs -= K.m[n].phiS * K.m[n].w * K.m[n].im;
        }
        const double c = K.xh - ys;
        if (c > 0) {
          double F = K.k * fastPow(c, K.alpha) * (1 + K.mu * (K.vh - vs));
          F = std::max(F, 0.0);
          for (int n = 0; n < K.nm; ++n) {
            Mode& m = K.m[n];
            m.im -= F * m.phiS / m.mass * dts / m.w;
          }
          K.side.im -= K.sideShare * F * K.side.phiS / K.side.mass * dts / K.side.w;
          K.vh -= F / K.M * dts;
          K.contact += dts;
        } else if (K.vh < 0) {
          K.vh = std::min(K.vh, -kRetreat);  // (off the tine, the key falls away: escapement)
        }
        K.xh += K.vh * dts;
        for (int n = 0; n < K.nm; ++n) rotate(K.m[n], true);
        rotate(K.side, true);
      }
      if (++K.hammerT > int(0.1 / dt_) || (K.vh < 0 && K.xh < -6e-3)) {
        K.hammer = false;
        static const bool kDebug = knob("PJ_TINEPIANO_DEBUG", 0) > 0;
        if (kDebug) {
          double a = 0;
          for (int n = 0; n < K.nm; ++n) a += std::hypot(K.m[n].re, K.m[n].im);
          say(stderr, "tinepiano key %d: contact %.2f ms, tip %.2f mm (A/w %.2f)\n", int(&K - &keys_[0]) + kLowest,
                       K.contact * 1e3, a * 1e3, a / K.w);
        }
      }
    } else {
      for (int n = 0; n < K.nm; ++n) rotate(K.m[n], false);
      rotate(K.side, false);
    }
    // The damper (engaging and lifting over ~4 ms).
    K.dampOn += (K.dampTarget - K.dampOn) * (1 - std::exp(-dt_ / 0.004));
    if (K.dampOn > 1e-3) {
      const double g = std::exp(-K.damp * K.dampOn * dt_);
      for (int n = 0; n < K.nm; ++n) K.m[n].re *= g, K.m[n].im *= g;
      K.side.re *= g, K.side.im *= g;
    }
    // The tine at its tip; the tonebar driven by its root, moving it.
    double x = 0, v = 0;
    for (int n = 0; n < K.nm; ++n) {
      x += K.m[n].re;
      v -= K.m[n].w * K.m[n].im;
    }
    {
      Mode& b = K.bar;
      b.im -= (-K.barDrive * K.m[0].re) / b.mass * dt_ / b.w;
      rotate(b, false);
      x += b.re;
      v -= b.w * b.im;
    }
    const double y = K.side.re, vy = -K.side.w * K.side.im;
    // The pickup: Phi = (1 + u^2)^-p, u = (x + delta) / w, w = the gap's bell less the sideways
    // motion toward the pole.
    const double w = std::max(0.25e-3, K.w - y);
    const double u = (x + K.delta) / w;
    const double q = 1 + u * u;
    const double dPdu = -2 * K.p * u * fastPow(q, -K.p - 1);
    const double dPdt = dPdu * (v / w + u / w * vy);  // (du/dt = v / w - (x + delta) / w^2 * dw/dt, dw/dt = -vy)
    emf -= K.sens * dPdt;
    // asleep?
    const double e = std::fabs(K.m[0].re) + std::fabs(K.m[0].im);
    if (!K.hammer && e < 1e-8) {
      if (++K.quiet > 4800) {
        K.awake = false;
        --awake_;
        for (Mode& m : K.m) m.re = m.im = 0;
        K.side.re = K.side.im = K.bar.re = K.bar.im = 0;
      }
    } else {
      K.quiet = 0;
    }
  }
  // The harp: one coil a key in groups of three (its EMF a third at the output), the groups in
  // series; mains hum through the groups' imperfect cancelling (odd harmonics).
  double e = kEmf * emf / 3;
  if (kHum > 0) {
    humPh_ += 2 * M_PI * kMains * dt_;
    if (humPh_ > 2 * M_PI) humPh_ -= 2 * M_PI;
    e += kHum * 2.5e-5 * (humEps_[0] * std::sin(humPh_) + humEps_[1] * std::sin(3 * humPh_) + humEps_[2] * std::sin(5 * humPh_));
  }
  const double hv = harpB_[0] * e + harpB_[1] * hx_[0] + harpB_[2] * hx_[1] - harpA_[1] * hy_[0] - harpA_[2] * hy_[1];
  hx_[1] = hx_[0], hx_[0] = e, hy_[1] = hy_[0], hy_[0] = hv;
  double l, r;
  if (!suitcase_) {
    l = r = hv;  // the Stage: straight out (a DI)
  } else {
    // The Peterson preamp: 0.22 uF in (~24 Hz), gain ~8, its tone at the centre (a gentle treble
    // roll-off).
    const double aHp = std::exp(-2 * M_PI * 24 * dt_);
    const double hp = aHp * (preHp_ + hv - preHpX_);
    preHpX_ = hv, preHp_ = hp;
    treb_ += (1 - std::exp(-2 * M_PI * 7000 * dt_)) * (8 * hp - treb_);
    const double pre = treb_;
    // The vibrato: a square wave lights the lamps in antiphase; each filament heats (30 ms) and
    // cools (70 ms), its light steep in its heat; each CdS cell follows the light fast (10 ms) and
    // lets go slowly (200 ms); the cell shunts its channel.
    double gch[2] = {1, 1};
    if (vibDepth_ > 1e-3) {
      vibPh_ += vibRate_ * dt_;
      if (vibPh_ >= 1) vibPh_ -= 1;
      for (int c = 0; c < 2; ++c) {
        const bool lit = (vibPh_ < 0.5) == (c == 0);
        const double drive = lit ? std::min(1.0, vibDepth_) : 0;
        const double tau = drive > lampT_[c] ? 0.03 : 0.07;
        lampT_[c] += (drive - lampT_[c]) * (1 - std::exp(-dt_ / tau));
        const double light = lampT_[c] * lampT_[c] * lampT_[c];
        const double tc = light > cell_[c] ? 0.01 : 0.2;
        cell_[c] += (light - cell_[c]) * (1 - std::exp(-dt_ / tc));
        gch[c] = 1 / (1 + 9 * cell_[c]);
      }
    }
    // The power amplifier: its unregulated rails sag under a heavy signal (~50 ms) and recover
    // (~300 ms); it clips softly against them.
    const double drive = std::fabs(pre);
    const double target = 1 - 0.25 * std::min(1.0, drive);
    rail_ += (target - rail_) * (1 - std::exp(-dt_ / (target < rail_ ? 0.05 : 0.3)));
    l = rail_ * std::tanh(pre * gch[0] / rail_);
    r = rail_ * std::tanh(pre * gch[1] / rail_);
    for (int i = 0; i < 3; ++i) l = cab_[0][i].run(l), r = cab_[1][i].run(r);
  }
  side_.push_back(float(outScale() * 0.5 * (l - r)));
  last_ = float(0.5 * (l + r));
  return finish(last_);
}

// --- The player.

TinePianoPlayer::TinePianoPlayer(TinePianoModel& m) : Performer(m), m_(m) {}

void TinePianoPlayer::noteOn(int midi, int velocity) {
  if (midi < TinePianoModel::kLowest || midi >= TinePianoModel::kLowest + TinePianoModel::kKeys) return;
  articulate(midi, velocity, false);
  keyDown_[midi] = true;
  note_ = midi;
  blowing_ = true;
  // The hammer's speed: 0.1 .. 1.6 m/s (the mezzo-forte C3's tine swinging ~3 mm at its tip).
  static const double kVmax = knob("PJ_TINEPIANO_VMAX", 1.6);
  const double v = kVmax * fastPow(std::clamp(velocity, 1, 127) / 127.0, 1.3) * hardness_;
  m_.strike(midi - TinePianoModel::kLowest, std::max(0.03, v));
}

void TinePianoPlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  keyDown_[midi] = false;
  if (!hold_) m_.damp(midi - TinePianoModel::kLowest, true);
}

void TinePianoPlayer::setHold(bool h) {
  hold_ = h;
  for (int k = TinePianoModel::kLowest; k < TinePianoModel::kLowest + TinePianoModel::kKeys; ++k)
    if (!keyDown_[k]) m_.damp(k - TinePianoModel::kLowest, !h);
}

bool TinePianoPlayer::setKnob(int k, double x) {
  if (k == 2) {  // K3: the vibrato's speed (2.0 .. 6.6 Hz, the Peterson's range)
    rate_ = 2.0 * fastPow(3.3, x);
    return true;
  }
  if (k == 3) {  // K4: its intensity
    depth_ = x;
    return true;
  }
  return false;
}

void TinePianoPlayer::render(float* out, int n) {
  static const double kRate = knob("PJ_TINEPIANO_VIB_HZ", 4.5), kDepth = knob("PJ_TINEPIANO_VIB", 0.5);
  constexpr int kBlock = 32;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    const double depth = (depth_ >= 0 ? depth_ : kDepth) + 0.5 * std::clamp(breathCtl_, 0.0, 1.0);
    m_.setVibrato(rate_ > 0 ? rate_ : kRate, std::min(1.0, depth));
    m_.process(out + i, m);
  }
  blowing_ = !m_.asleep();
}

}  // namespace pajupilli
