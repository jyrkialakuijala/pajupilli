#include "modal.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace pajupilli {

namespace {
constexpr double kLn1000 = 6.907755278982137;  // t60: e^(-s t) falls by 60 dB
}

double stringShape(int k, double x, double L) { return std::sin(k * M_PI * x / L); }

// The clamped-free beam's roots: cos b cosh b = -1.
double cantileverRoot(int k) {
  static const double r[] = {1.8751040687, 4.6940911330, 7.8547574382, 10.9955407349, 14.1371683910};
  return k <= 5 ? r[k - 1] : (k - 0.5) * M_PI;
}

double cantileverRatio(int k) {
  const double b = cantileverRoot(k), b1 = cantileverRoot(1);
  return b * b / (b1 * b1);
}

double cantileverShape(int k, double x, double L) {
  const double b = cantileverRoot(k), u = b * x / L;
  const double s = (std::cosh(b) + std::cos(b)) / (std::sinh(b) + std::sin(b));
  // (cosh u - cos u) - s (sinh u - sin u), computed without cancelling: for
  // large u, cosh - s sinh is small.
  const double ch_sh = std::exp(-u) * 0.5 * (1 + s) + std::exp(u) * 0.5 * (1 - s);
  return ch_sh - std::cos(u) + s * std::sin(u);
}

ModalSet::ModalSet(double fs) : fs_(fs), dt_(1 / fs) {}

void ModalSet::reset() {
  for (int i = 0; i < nVib_; ++i) {
    Vib& v = vib_[i];
    std::memset(v.x, 0, sizeof v.x);
    std::memset(v.v, 0, sizeof v.v);
    v.pT = 1e9;
    v.mOn = false;
    v.bowOn = false;
    v.bowF = v.bowV = 0;
    v.awake = false;
  }
  std::fill(bx_, bx_ + kBody, 0.0f);
  std::fill(bv_, bv_ + kBody, 0.0f);
  bridgeA_ = 0;
  prevOut_ = prevSide_ = 0;
}

void ModalSet::setVib(int i, const Def& d) {
  Vib& v = vib_[i];
  v.def = d;
  std::copy(d.exc, d.exc + kModes, v.exc);
  v.ratio = v.ratioFrom = v.ratioTo = 1;
  v.glideT = 1;
  v.glideNow = 1;
  nVib_ = std::max(nVib_, i + 1);
  coefs(v);
}

void ModalSet::setExcitation(int i, const float* shape) { std::copy(shape, shape + kModes, vib_[i].exc); }

void ModalSet::coefs(Vib& v) {
  const Def& d = v.def;
  v.nv = std::min(int(kModes), (d.n + 15) & ~15);
  const double r = v.ratio * v.glideNow;
  for (int p = 0; p < 2; ++p)
    for (int k = 0; k < kModes; ++k) {
      double f = k < d.n ? d.freq[k] * r : 0;
      // z: each mode a hair off y's, its sign from the mode's number.
      if (p == 1) f *= 1 + d.split * (((k * 2654435761u) >> 7) & 1 ? 1 : -1);
      if (f <= 0 || f > 0.45 * fs_ || d.t60[k] <= 0) {
        v.c00[p][k] = v.c01[p][k] = v.c10[p][k] = v.c11[p][k] = 0;
        continue;
      }
      const double w = 2 * M_PI * f, s = kLn1000 / d.t60[k] + v.dampRate;
      const double wd = std::sqrt(std::max(w * w - s * s, 1e-6)), e = std::exp(-s * dt_);
      const double c = std::cos(wd * dt_), sn = std::sin(wd * dt_);
      v.c00[p][k] = float(e * (c + s / wd * sn));
      v.c01[p][k] = float(e * sn / wd);
      v.c10[p][k] = float(-e * w * w * sn / wd);
      v.c11[p][k] = float(e * (c - s / wd * sn));
    }
  // The contact patches' heights per unit force through one step.
  for (int a = 0; a < d.nc; ++a)
    for (int b = 0; b < d.nc; ++b) {
      double g = 0;
      for (int k = 0; k < kModes; ++k) g += d.cShape[a][k] * d.cShape[b][k] * v.c01[0][k];
      v.cG[a][b] = float(g * dt_ / d.modalMass);
    }
}

// Every mode's frequency x g: a slow change of tension keeps each mode's
// energy / frequency (an adiabatic invariant), so w x and v scale by sqrt(g):
// a bend neither pumps energy in nor takes it out beyond that.
static void adiabatic(float (&x)[2][ModalSet::kModes], float (&v)[2][ModalSet::kModes], int n, double g) {
  const float sv = float(std::sqrt(g)), sx = float(1 / std::sqrt(g));
  for (int p = 0; p < 2; ++p)
    for (int k = 0; k < n; ++k) {
      x[p][k] *= sx;
      v[p][k] *= sv;
    }
}

void ModalSet::silence(int i) {
  Vib& v = vib_[i];
  std::memset(v.x, 0, sizeof v.x);
  std::memset(v.v, 0, sizeof v.v);
  v.mOn = false;
  v.pT = 1e9;
}

void ModalSet::retune(int i, double ratio, double glide) {
  Vib& v = vib_[i];
  if (glide <= 0) {
    adiabatic(v.x, v.v, v.nv, ratio / v.ratio);
    v.ratio = v.ratioFrom = v.ratioTo = ratio;
    v.glideT = 1;
    coefs(v);
    return;
  }
  v.ratioFrom = v.ratio;
  v.ratioTo = ratio;
  v.glideT = 0;
  v.glideDur = glide;
}

void ModalSet::setBody(int n, const float* freq, const float* t60, const float* mass, const float* rad,
                       const float* side) {
  nb_ = std::min(n, int(kBody));
  for (int j = 0; j < kBody; ++j) {
    if (j >= nb_) {
      b00_[j] = b01_[j] = b10_[j] = b11_[j] = bInvM_[j] = bRad_[j] = bSide_[j] = 0;
      continue;
    }
    const double w = 2 * M_PI * freq[j], s = kLn1000 / t60[j];
    const double wd = std::sqrt(std::max(w * w - s * s, 1e-6)), e = std::exp(-s * dt_);
    const double c = std::cos(wd * dt_), sn = std::sin(wd * dt_);
    b00_[j] = float(e * (c + s / wd * sn));
    b01_[j] = float(e * sn / wd);
    b10_[j] = float(-e * w * w * sn / wd);
    b11_[j] = float(e * (c - s / wd * sn));
    bInvM_[j] = float(1 / mass[j]);
    bRad_[j] = rad[j];
    bSide_[j] = side ? side[j] : 0;
  }
}

void ModalSet::pluck(int i, double force, double rise, double hold, double slip, double angle) {
  Vib& v = vib_[i];
  v.pF = force;
  v.pRise = std::max(rise, dt_);
  v.pHold = hold;
  v.pSlip = std::max(slip, dt_);
  v.pT = 0;
  v.pAngle = angle;
  v.awake = true;
}

void ModalSet::strike(int i, double mass, double speed, double K, double alpha) {
  Vib& v = vib_[i];
  // The mallet comes down onto the surface (y), from just above it.
  double y = 0;
  for (int k = 0; k < v.def.n; ++k) y += v.exc[k] * v.x[0][k];
  v.mOn = true;
  v.mMass = mass;
  v.mZ = y + 1e-6;
  v.mV = -std::fabs(speed);
  v.mK = K;
  v.mAlpha = alpha;
  v.mT = 0;
  v.awake = true;
}

void ModalSet::bow(int i, double speed, double force, double rise) {
  Vib& v = vib_[i];
  v.bowVTo = float(speed);
  v.bowFTo = float(force);
  v.bowK = float(1 - std::exp(-dt_ / std::max(rise, 1e-3)));
  if (force > 0 && !v.bowOn) {
    v.bowOn = true;
    v.slipping = false;
    float s = 0;
    for (int k = 0; k < v.def.n; ++k) s += v.exc[k] * v.exc[k];
    v.bowSum = s;
  }
  if (force > 0) v.awake = true;
}

void ModalSet::knock(double force, double dur, double delay) {
  kickF_ = force;
  kickDur_ = std::max(dur, dt_);
  kickT_ = -delay;
}

void ModalSet::damp(int i, double rate) {
  Vib& v = vib_[i];
  v.dampRate = rate;
  v.dampMul = float(std::exp(-rate * dt_));
}

double ModalSet::energy(int i) const {
  const Vib& v = vib_[i];
  double e = 0;
  for (int p = 0; p < 2; ++p)
    for (int k = 0; k < v.def.n; ++k) {
      const double w = 2 * M_PI * v.def.freq[k] * v.ratio;
      e += 0.5 * (v.v[p][k] * v.v[p][k] + w * w * v.x[p][k] * v.x[p][k]);
    }
  return e;
}

float ModalSet::sample() {
  // --- Every 16 samples: glides, the tension's stretch, who is awake.
  if (++ctl_ >= 16) {
    ctl_ = 0;
    const double bdt = 16 * dt_;
    for (int i = 0; i < nVib_; ++i) {
      Vib& v = vib_[i];
      bool changed = false;
      if (v.glideT < 1) {
        v.glideT = std::min(1.0, v.glideT + bdt / v.glideDur);
        const double u = v.glideT * v.glideT * (3 - 2 * v.glideT);
        const double r = v.ratioFrom * std::pow(v.ratioTo / v.ratioFrom, u);
        adiabatic(v.x, v.v, v.nv, r / v.ratio);
        v.ratio = r;
        changed = true;
      }
      if (v.def.glide > 0 && v.awake) {
        // The amplitude at the excitation point, squared, over the reference.
        double a2 = 0;
        for (int k = 0; k < v.def.n; ++k) a2 += v.x[0][k] * v.x[0][k];
        const double g = std::min(1.2, std::sqrt(1 + v.def.glide * a2 / (v.def.glideRef * v.def.glideRef)));
        if (std::fabs(g - v.glideNow) > 2e-4) {
          adiabatic(v.x, v.v, v.nv, g / v.glideNow);
          v.glideNow = g;
          changed = true;
        }
      }
      if (changed) coefs(v);
      if (v.awake && v.pT > v.pRise + v.pHold + v.pSlip && !v.mOn && !v.bowOn) {
        const double e = energy(i);
        v.quiet = e < 1e-13 ? v.quiet + 1 : 0;
        if (v.quiet > 8) {  // still for ~3 ms: asleep, until excited or the bridge moves
          v.awake = false;
          std::memset(v.x, 0, sizeof v.x);
          std::memset(v.v, 0, sizeof v.v);
        }
      }
    }
  }
  // --- The bridge: the vibrators' push, the body's answer.
  float Fb = 0;
  for (int i = 0; i < nVib_; ++i) {
    Vib& v = vib_[i];
    if (!v.awake) continue;
    const float zs = v.def.zShare;
    float f = 0;
    for (int k = 0; k < v.nv; ++k) f += v.def.bridge[k] * (v.x[0][k] + zs * v.x[1][k]);
    Fb += f;
  }
  Fb += extF_;
  extF_ = 0;
  if (kickT_ < 0) {  // a knock on the body itself (a bachi on the skin), after its delay
    kickT_ += dt_;
  } else if (kickT_ < kickDur_) {
    Fb += float(kickF_ * std::sin(M_PI * kickT_ / kickDur_));
    kickT_ += dt_;
  }
  float before = 0, after = 0;
  for (int j = 0; j < kBody; ++j) before += bv_[j];
  for (int j = 0; j < kBody; ++j) {
    const float vv = bv_[j] + dt_ * Fb * bInvM_[j];
    const float xn = b00_[j] * bx_[j] + b01_[j] * vv;
    bv_[j] = b10_[j] * bx_[j] + b11_[j] * vv;
    bx_[j] = xn;
  }
  for (int j = 0; j < kBody; ++j) after += bv_[j];
  bridgeA_ = (after - before) * float(fs_);
  const bool shaking = std::fabs(bridgeA_) > 1e-4f;
  // --- Each vibrator.
  double out = 0, side = 0;
  for (int j = 0; j < kBody; ++j) {
    out += bRad_[j] * bv_[j];
    side += bSide_[j] * bv_[j];
  }
  for (int i = 0; i < nVib_; ++i) {
    Vib& v = vib_[i];
    if (!v.awake) {
      if (!shaking || v.def.back[0] == 0) continue;
      v.awake = true;
      v.quiet = 0;
    }
    const Def& d = v.def;
    const int nv = v.nv;
    const float im = 1 / d.modalMass, dt = float(dt_);
    alignas(64) float f0[kModes], f1[kModes];
    const float ab = bridgeA_ * dt;
    for (int k = 0; k < nv; ++k) {
      f0[k] = d.back[k] * ab;
      f1[k] = d.back[k] * ab * d.zShare;
    }
    // The pluck: a force rising, held, let go.
    if (v.pT < v.pRise + v.pHold + v.pSlip) {
      double F;
      if (v.pT < v.pRise) {
        const double u = v.pT / v.pRise;
        F = v.pF * u * u * (3 - 2 * u);
      } else if (v.pT < v.pRise + v.pHold) {
        F = v.pF;
      } else {
        F = v.pF * (1 - (v.pT - v.pRise - v.pHold) / v.pSlip);
      }
      v.pT += dt_;
      const float fy = float(F * std::sin(v.pAngle)) * im * dt, fz = float(F * std::cos(v.pAngle)) * im * dt;
      for (int k = 0; k < nv; ++k) {
        f0[k] += fy * v.exc[k];
        f1[k] += fz * v.exc[k];
      }
    } else {
      v.pT += dt_;
    }
    // The mallet: Hertz contact while it presses into the surface, solved
    // with the step (a stiff stick on a light bell would ring the explicit
    // contact up): the surface's and the mallet's places after this sample
    // without the force, how far a force moves them apart, F = K c^alpha by
    // Newton.
    if (v.mOn) {
      float yf = 0, g = 0;
      for (int k = 0; k < nv; ++k) {
        yf += v.exc[k] * (v.c00[0][k] * v.x[0][k] + v.c01[0][k] * (v.v[0][k] + f0[k]));
        g += v.exc[k] * v.exc[k] * v.c01[0][k];
      }
      const double G = g * dt_ / d.modalMass + dt_ * dt_ / v.mMass;
      const double zf = v.mZ + v.mV * dt_;
      const double c0 = yf - zf;
      double F = 0;
      if (c0 > 0) {
        // F = K (c0 - G F)^alpha: monotone; Newton from the bracket's middle.
        double lo = 0, hi = c0 / G;
        F = 0.5 * hi;
        for (int it = 0; it < 30; ++it) {
          const double c = std::max(c0 - G * F, 0.0);
          const double r = F - v.mK * std::pow(c, v.mAlpha);
          if (r > 0) hi = F; else lo = F;
          const double dr = 1 + v.mK * v.mAlpha * std::pow(std::max(c, 1e-12), v.mAlpha - 1) * G;
          double Fn = F - r / dr;
          if (Fn <= lo || Fn >= hi) Fn = 0.5 * (lo + hi);
          if (std::fabs(Fn - F) < 1e-9 * (1 + F)) { F = Fn; break; }
          F = Fn;
        }
      }
      v.mV += F / v.mMass * dt_;
      v.mZ += v.mV * dt_;
      v.mT += dt_;
      const float fm = float(-F) * im * dt;
      for (int k = 0; k < nv; ++k) f0[k] += fm * v.exc[k];
      if ((c0 < 0 && v.mV > 0 && v.mT > 1e-3) || v.mT > 0.05) v.mOn = false;
    }
    // The bow (along the top: z), stick-slip as the double bass's.
    if (v.bowOn) {
      v.bowV += v.bowK * (v.bowVTo - v.bowV);
      v.bowF += v.bowK * (v.bowFTo - v.bowF);
      if (v.bowFTo <= 0 && v.bowF < 2e-3f) {
        v.bowOn = false;
        v.bowF = 0;
      } else {
        constexpr float kMuS = 0.9f, kMuD = 0.35f, kMuV = 0.08f;
        static const float kBowNoise = float(std::getenv("PJ_MBOWNOISE") ? std::atof(std::getenv("PJ_MBOWNOISE")) : 0.1);
        // The string's velocity under the hair after this sample's step
        // without the bow's force, and how much a force moves it (the step
        // turns each mode: the high ones a good way round in one sample, so
        // their share counts as c11, not 1).
        float vh = 0, ys = 0;
        for (int k = 0; k < nv; ++k) {
          vh += v.exc[k] * (v.c10[1][k] * v.x[1][k] + v.c11[1][k] * (v.v[1][k] + f1[k]));
          ys += v.exc[k] * v.exc[k] * v.c11[1][k];
        }
        const float Y = std::max(dt * im * ys, 1e-6f);
        v.bowNoise += 0.3f * (urand() - v.bowNoise);
        const float Fn = v.bowF * (1 + kBowNoise * v.bowNoise);
        const float a = vh - v.bowV, A = std::fabs(a), sg = a > 0 ? 1.0f : -1.0f;
        const bool canStick = A <= Y * Fn * kMuS;
        const float a1 = A - Y * Fn * kMuD;
        const float disc = (a1 + kMuV) * (a1 + kMuV) - 4 * kMuV * Y * Fn * (kMuS - kMuD);
        const float u = disc >= 0 ? 0.5f * (a1 - kMuV + std::sqrt(disc)) : -1.0f;
        v.slipping = v.slipping ? u > 0 || !canStick : !canStick;
        const float f = v.slipping ? -sg * Fn * (kMuD + (kMuS - kMuD) / (1 + std::max(u, 0.0f) / kMuV)) : -a / Y;
        const float fb = f * im * dt;
        for (int k = 0; k < nv; ++k) f1[k] += fb * v.exc[k];
      }
    }
    // Contact with the obstacle (y): a one-sided linear spring on each patch,
    // solved with the step (van Walstijn, Bridges & Mehes 2016's tanpura):
    // the patch's height after this sample without the contact, plus what a
    // contact force would move it, F = K (h - y) where positive. Two
    // Gauss-Seidel passes over the patches (they share the modes).
    if (d.nc > 0) {
      float yf[kContacts], F[kContacts] = {};
      for (int c = 0; c < d.nc; ++c) {
        float y = 0;
        for (int k = 0; k < nv; ++k) y += d.cShape[c][k] * (v.c00[0][k] * v.x[0][k] + v.c01[0][k] * (v.v[0][k] + f0[k]));
        yf[c] = y;
      }
      for (int pass = 0; pass < 2; ++pass)
        for (int c = 0; c < d.nc; ++c) {
          float y = yf[c];
          for (int e = 0; e < d.nc; ++e)
            if (e != c) y += v.cG[c][e] * F[e];
          F[c] = std::max(0.0f, d.cK * (d.cHeight[c] - y) / (1 + d.cK * v.cG[c][c]));
        }
      for (int c = 0; c < d.nc; ++c) {
        if (F[c] <= 0) continue;
        const float Fi = F[c] * im * dt;
        for (int k = 0; k < nv; ++k) f0[k] += Fi * d.cShape[c][k];
      }
    }
    // The shell's quadratic coupling.
    for (int t = 0; t < d.nq; ++t) {
      const Def::Quad& q = d.quad[t];
      f0[q.p] -= dt * q.g * v.x[0][q.i] * v.x[0][q.j];
    }
    // Step every mode: the impulse, the damper, the free motion over a sample.
    const float dm = v.dampMul;
    float o = 0, s = 0;
    for (int p = 0; p < (d.one ? 1 : 2); ++p) {
      float* x = v.x[p];
      float* vv = v.v[p];
      const float* f = p == 0 ? f0 : f1;
      for (int k = 0; k < nv; ++k) {
        const float vk = (vv[k] + f[k]) * dm, xk = x[k] * dm;
        x[k] = v.c00[p][k] * xk + v.c01[p][k] * vk;
        vv[k] = v.c10[p][k] * xk + v.c11[p][k] * vk;
      }
    }
    for (int k = 0; k < nv; ++k) {
      const float vk = v.v[0][k] + d.zShare * v.v[1][k];
      o += d.rad[k] * vk;
      s += d.side[k] * vk;
    }
    out += o;
    side += s;
  }
  // The pressure ~ the volume acceleration.
  const float y = float((out - prevOut_) * fs_ * 1e-3);
  side_ = float((side - prevSide_) * fs_ * 1e-3);
  prevOut_ = out;
  prevSide_ = side;
  return y;
}

}  // namespace pajupilli
