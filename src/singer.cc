#include "singer.h"

#include <algorithm>
#include <cmath>

#include "knob.h"

namespace pajupilli {
namespace {

constexpr double kRho = 1.14, kC = 350;  // warm, humid air in the tract
constexpr double kPi = M_PI;

// puhuja's vowel rows (puhuja.cc kLineVowels, fitted by its linefit to F1-F3):
// the tongue hump's place (share of the length), area (cm^2) and half-width
// (cm); the lips' area; the pharynx's and the mouth's areas; the lip tube's
// length (cm); the larynx tube's area; the length (cm).
struct VowelRow { double xc, ac, wc, aLip, aPh, aOr, lLip, aLar, len; };
// In the order of the vowel axis: u, o, a, e, i (dark to bright).
const VowelRow kVowels[5] = {
    {0.3234, 1.631, 1.648, 1.55, 11.93, 2.527, 1.654, 0.7042, 17.53},  // u
    {0.488, 2.762, 3.873, 2.743, 1.022, 7.843, 0.5993, 2.784, 17.55},  // o
    {0.5347, 1.995, 4.296, 8.94, 0.7533, 6.264, 1.582, 0.3, 17.38},    // a
    {0.1426, 3.41, 1.775, 3.617, 4.766, 3.455, 3.721, 0.9144, 17.4787},  // e
    {0.5287, 2.003, 2.45, 3.575, 10.86, 0.6748, 1.759, 0.6139, 17.44},  // i
};

double ramp(double x, double x0, double x1) {
  const double t = std::clamp((x - x0) / std::max(x1 - x0, 1e-3), 0.0, 1.0);
  return t * t * (3 - 2 * t);
}

// puhuja's area function (tract.cc, without the optional hypopharynx and
// front spaces): log area (cm^2) at x cm from the glottis.
double lnArea(const VowelRow& v, double x) {
  const double len = v.len, lLar = 2.0;
  double lnA = std::log(v.aLar) + (std::log(v.aPh) - std::log(v.aLar)) * ramp(x, lLar - 0.5, lLar + 0.5);
  const double xv = 0.55 * len;
  lnA += (std::log(v.aOr) - std::log(v.aPh)) * ramp(x, xv - 1, xv + 1);
  const double d = (x - v.xc * len) / std::max(v.wc, 0.2);
  if (std::fabs(d) < 1) {
    const double g = std::cos(0.5 * kPi * d);
    lnA += (std::log(std::max(v.ac, 0.05)) - lnA) * g * g;
  }
  lnA += (std::log(v.aLip) - lnA) * ramp(x, len - v.lLip - 0.5, len - v.lLip + 0.5);
  return lnA;
}

// Each vowel's sections (log m^2), the mean of 8 points per section, over its
// own length: a shorter tract is the same shape scaled.
struct Table {
  float ln[5][Singer::kN];
  Table() {
    for (int k = 0; k < 5; ++k) {
      const VowelRow& v = kVowels[k];
      const double dx = v.len / Singer::kN;
      for (int i = 0; i < Singer::kN; ++i) {
        double s = 0;
        for (int j = 0; j < 8; ++j) s += std::exp(lnArea(v, (i + (j + 0.5) / 8) * dx));
        ln[k][i] = float(std::log(s / 8 * 1e-4));
      }
    }
  }
};
const Table& table() {
  static const Table t;
  return t;
}

}  // namespace

void Singer::init(double fs, unsigned seed, int index) {
  fs_ = fs;
  dts_ = 1 / (fs * kSub);
  rng_ = 2654435761u * (seed + 1) + 97u * index + 1;
  for (int i = 0; i < 8; ++i) urand();
  // This singer's own: a tract a few % longer or shorter, a pitch a few cents
  // off, a vibrato a little faster or slower, starting anywhere.
  lenMul_ = 1 + 0.03 * urand();
  scatter_ = index == 0 ? 0 : 6 * urand();
  vibRateMul_ = 1 + 0.08 * urand();
  vibPh_ = kPi * urand();
  reset();
}

void Singer::reset() {
  std::fill(p_, p_ + kN, 0.0f);
  std::fill(u_, u_ + kN + 1, 0.0f);
  iL_ = uLipPrev_ = 0;
  nLp_ = nHp_ = 0;
  fLp_ = fHp_ = 0;
  cArea_ = cTarget_ = 0;
  cBound_ = -1;
  phase_ = 0;
  lung_ = 0;
  drift_ = 0;
  shaped_ = false;
}

void Singer::shape(double vowel, double len) {
  const Table& t = table();
  const double v = std::clamp(vowel, 0.0, 4.0);
  const int k = std::min(3, int(v));
  const double f = v - k;
  for (int i = 0; i < kN; ++i) lnT_[i] = float(t.ln[k][i] + f * (t.ln[k + 1][i] - t.ln[k][i]));
  lenTarget_ = len;
}

void Singer::coefs() {
  dx_ = len_ / kN;
  // The constriction: a cos^2 hump in log area down to its area, 1 cm
  // either side of its place (puhuja's), on the vowel's shape.
  cBound_ = -1;
  float hump[kN] = {};
  if (cArea_ > 0) {
    const double xc = std::min(cPlace_ * len_, len_ - 0.5 * dx_), w = 0.01, lnc = std::log(std::max(cArea_, 0.002) * 1e-4);
    for (int i = 0; i < kN; ++i) {
      const double d = ((i + 0.5) * dx_ - xc) / w;
      if (std::fabs(d) < 1) {
        const double g = std::cos(0.5 * kPi * d);
        hump[i] = float(std::min(0.0, (lnc - lnA_[i]) * g * g));
      }
    }
    cBound_ = std::min(kN, int(xc / dx_) + 1);
  }
  for (int i = 0; i < kN; ++i) {
    A_[i] = std::exp(lnA_[i] + hump[i]);
    kp_[i] = float(dts_ * kRho * kC * kC / (A_[i] * dx_));
  }
  // The boundaries: the mean area; losses (a viscous part growing in the
  // narrow places, and a constant for the walls' and the boundary layer's):
  // formant bandwidths ~50-100 Hz.
  for (int i = 1; i < kN; ++i) {
    // (At most 1.7 x the narrower side: a boundary much wider than a
    // section next to it, as at a closure, breaks the leapfrog's bound,
    // (c dt / dx)^2 ab / A < 1.)
    const double ab = std::min(0.5 * (A_[i - 1] + A_[i]), 1.7 * std::min(A_[i - 1], A_[i]));
    ku_[i] = float(dts_ * ab / (kRho * dx_));
    damp_[i] = float(std::exp(-(260 + 0.035 / ab) * dts_));
  }
  // The lips: the last half section into Flanagan's piston load.
  const double al = A_[kN - 1];
  ku_[kN] = float(dts_ * al / (kRho * 0.5 * dx_));
  damp_[kN] = float(std::exp(-(260 + 0.035 / al) * dts_));
  radR_ = 128 * kRho * kC / (9 * kPi * kPi * al);
  radL_ = 8 * kRho / (3 * kPi * std::sqrt(kPi * al));
}

void Singer::control(double f0, double vowel, double lung, double breath, double vibDepth, double vibRate,
                     double note, double dt) {
  // The voice part: a bass's tract (18 cm) for low notes, a soprano's (14.5 cm) for high.
  const double len = lenMul_ * 0.01 * (18.0 - 3.5 * std::clamp((note - 45) / 31, 0.0, 1.0));
  if (!shaped_ || std::fabs(vowel - vowel_) > 1e-4 || std::fabs(len - lenTarget_) > 1e-6) {
    shape(vowel, len);
    vowel_ = vowel;
  }
  if (!shaped_) {  // a fresh singer: in place at once
    std::copy(lnT_, lnT_ + kN, lnA_);
    len_ = lenTarget_;
    shaped_ = true;
  } else {  // the tongue, the jaw and the lips move in ~50 ms; the larynx rises slower
    const double a = 1 - std::exp(-dt / 0.05);
    for (int i = 0; i < kN; ++i) lnA_[i] += float(a * (lnT_[i] - lnA_[i]));
    len_ += (1 - std::exp(-dt / 0.15)) * (lenTarget_ - len_);
  }
  // The constriction closes in ~3 ms, opens in ~6 (in log area); gone when wide.
  if (cTarget_ > 0 || cArea_ > 0) {
    const double to = cTarget_ > 0 ? cTarget_ : 20.0, from = cArea_ > 0 ? cArea_ : 20.0;
    const double tau = to < from ? 0.003 : 0.006;
    cArea_ = std::exp(std::log(from) + (1 - std::exp(-dt / tau)) * (std::log(to) - std::log(from)));
    if (cTarget_ <= 0 && cArea_ > 10) cArea_ = 0;
  }
  coefs();
  // Its jet's noise: puhuja's fade (full below 0.12 cm^2, none above 0.2156,
  // in log area).
  cFric_ = 0;
  if (cBound_ >= 0) {
    const double a0 = 0.12, a1 = 0.2156;
    cFric_ = std::clamp(std::log(a1 / std::max(cArea_, 1e-3)) / std::log(a1 / a0), 0.0, 1.0);
  }
  lung_ += (1 - std::exp(-dt / 0.005)) * (lung - lung_);
  breath_ = breath;
  // The pitch: the note, this singer's offset and slow drift (a few cents),
  // its own vibrato.
  drift_ += std::sqrt(dt) * 3 * urand() - dt / 1.5 * drift_;
  vibPh_ += 2 * kPi * vibRate * vibRateMul_ * dt;
  if (vibPh_ > kPi) vibPh_ -= 2 * kPi;
  const double cents = scatter_ + drift_ + 100 * vibDepth * std::sin(vibPh_);
  f0_ = f0 * std::exp2(cents / 1200);
  // Louder: more of the folds' length opens, the open phase shortens and
  // the closing gets quicker (more high harmonics).
  const double loud = std::clamp(lung_ / 1000, 0.0, 1.5);
  aMax_ = (0.10 + 0.08 * loud) * 1e-4 * (1 - whisper_);
  oq_ = 0.68 - 0.18 * std::min(loud, 1.0);
  cq_ = 0.42 - 0.17 * std::min(loud, 1.0);
}

double Singer::sample() {
  static const double kAsp = knob("PJ_CHOIRASP", 1);  // the glottis's turbulence, x
  const double cd2 = 0.92 * 0.92, thick = 0.003, wFold = 0.012, nu = 1.6e-5;
  double uSum = 0;
  for (int s = 0; s < kSub; ++s) {
    // --- The folds' opening at this instant: a smooth pulse, a chink always open.
    phase_ += f0_ * (1 + jit_) * dts_;
    if (phase_ >= 1) {
      phase_ -= 1;
      jit_ = 0.003 * urand();          // jitter: a few tenths of a percent per period
      shim_ = 1 + 0.03 * urand();      // shimmer
    }
    const double op = oq_ * (1 - cq_);
    double g = 0;
    if (phase_ < op) g = 0.5 * (1 - std::cos(kPi * phase_ / op));
    else if (phase_ < oq_) g = std::cos(0.5 * kPi * (phase_ - op) / (oq_ - op));
    const double a = aMax_ * shim_ * g + (0.015 + 0.09 * breath_ + 0.5 * whisper_) * 1e-4;
    // --- Turbulence: past a critical Reynolds number (of the slit), noise
    // ~ (Re^2 - Rc^2) / Rc^2, band-passed (~300 Hz .. 6 kHz).
    const double wn = urand();
    nLp_ += 0.23 * (wn - nLp_);
    nHp_ += 0.013 * (nLp_ - nHp_);
    const double re = std::fabs(u_[0]) / (wFold * nu), rc = 600;
    const double turb = re > rc ? (re * re - rc * rc) / (rc * rc) : 0;
    const double pn = kAsp * (2 + 10 * breath_) * std::min(turb, 30.0) * (nLp_ - nHp_);
    // --- The glottal flow: Lg du/dt = dp - rho u|u| / (2 Cd^2 a^2), implicit.
    const double dp = lung_ - p_[0] + pn;
    const double b = kRho * thick / a / dts_, k = kRho / (2 * cd2 * a * a);
    const double c = b * u_[0] + dp;
    u_[0] = float(2 * c / (b + std::sqrt(b * b + 4 * k * std::fabs(c))));
    // --- The constriction's jet: past a critical Reynolds number (its own,
    // through a slit of its area), noise ~ (Re^2 - Rc^2) / Rc^2 (capped),
    // band-passed (~1 .. 8 kHz), a pressure where the jet leaves it.
    double pf = 0;
    if (cFric_ > 0) {
      const double ac = std::max(cArea_, 0.002) * 1e-4, dh = std::sqrt(4 * ac / kPi);
      const double rec = std::fabs(u_[std::min(cBound_, kN)]) * dh / (ac * nu), rcc = 1800;
      if (rec > rcc) {
        const double fn = urand();
        fLp_ += 0.65 * (fn - fLp_);
        fHp_ += 0.12 * (fLp_ - fHp_);
        pf = kAsp * 25 * cFric_ * std::min((rec * rec - rcc * rcc) / (rcc * rcc), 30.0) * (fLp_ - fHp_);
      }
    }
    // --- The tract: the flows at the inner boundaries, then the pressures.
    for (int i = 1; i < kN; ++i) u_[i] = damp_[i] * u_[i] + ku_[i] * (p_[i - 1] - p_[i]);
    if (pf != 0 && cBound_ < kN) u_[cBound_] += float(ku_[cBound_] * pf);
    {  // the lips: the half section into R parallel L (implicit in R)
      const double kl = ku_[kN];
      const double du = kl * (p_[kN - 1] + (cBound_ == kN ? pf : 0) - radR_ * (u_[kN] - iL_)) / (1 + kl * radR_);
      u_[kN] = float(damp_[kN] * (u_[kN] + du));
      // (The inductance's current: explicitly while that is stable (every
      // open vowel); implicitly for lips nearly closed, whose R / L is so large
      // that an explicit step, dt R / L > 2, rings up.)
      const double g = dts_ * radR_ / radL_;
      if (g < 1) iL_ += g * (u_[kN] - iL_);
      else iL_ = (iL_ + g * u_[kN]) / (1 + g);
    }
    for (int i = 0; i < kN; ++i) p_[i] += kp_[i] * (u_[i] - u_[i + 1]);
    uSum += u_[kN];
  }
  // The radiated sound: the lip flow's derivative (its mean over the
  // substeps: a box filter before the 48 kHz rate).
  const double u = uSum / kSub, d = (u - uLipPrev_) * fs_;
  uLipPrev_ = u;
  return d;
}

}  // namespace pajupilli
