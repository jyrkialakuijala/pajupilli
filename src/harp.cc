#include "harp.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "harpboard_data.h"
#include "knob.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRhoAir = 1.2, kNuAir = 1.5e-5, kC = 343.0;
double urand(uint32_t& s) {
  s = s * 1664525u + 1013904223u;
  return (s >> 8) * (1.0 / 16777216.0);
}
// The mean of sin^2(k pi z / L) over a contact `len` long centred at z = at L (eight points).
double under(int k, double at, double len, double L) {
  double m = 0;
  for (int j = 0; j < 8; ++j) {
    const double z = std::clamp(at * L + len * ((j + 0.5) / 8 - 0.5), 0.0, L);
    const double s = std::sin(k * kPi * z / L);
    m += s * s / 8;
  }
  return m;
}
}  // namespace

int Harp::natural(int s) {
  static const int kSemi[7] = {0, 2, 4, 5, 7, 9, 11};
  return 12 * (s / 7 + 2) + kSemi[s % 7];
}

int Harp::sounds(int s) const { return natural(s) + (hasDiscs(s) ? str_[s].pos : 0); }

int Harp::totalModes() const {
  int n = 0;
  for (const Str& st : str_) n += st.n;
  return n;
}

Harp::Harp(double fs) : fs_(fs), dt_(1 / fs) {
  namespace hb = harpboard;
  static_assert(hb::kStrings == kStrings, "the board's data is for another number of strings");
  kOut_ = float(knob("PJ_HARP_OUT", 1.0));
  kCouple_ = float(knob("PJ_HARP_BOARD", 1.0));  // (x the strings' coupling to the board)
  sinT_ = float(std::sin(hb::kTheta));
  // The board.
  nB_ = hb::kModes;
  for (auto* v : {&bx_, &bv_, &bcs_, &bsn_, &binv_, &bf_, &bd_, &bacc_}) v->assign(nB_, 0.0f);
  const double boardLoss = knob("PJ_HARP_BOARDLOSS", 1.0);
  for (int m = 0; m < nB_; ++m) {
    const double w = 2 * kPi * hb::kFreq[m], g = std::exp(-0.5 * boardLoss * hb::kLoss[m] * w * dt_);
    bcs_[m] = float(g * std::cos(w * dt_));
    bsn_[m] = float(g * std::sin(w * dt_));
    binv_[m] = float(1 / w);
  }
  phi_.assign(size_t(kStrings) * nB_, 0.0f);
  side_phi_.assign(size_t(kStrings) * nB_, 0.0f);
  for (int i = 0; i < kStrings * nB_; ++i) {
    phi_[i] = kCouple_ * hb::kPhi[i];
    side_phi_[i] = kCouple_ * hb::kSide[i];
  }
  // The air: the patches (and holes) as small sources, heard by two microphones 2 m in front of
  // the board's face, 60 cm apart, 1.2 m up (scaled to 1 m: x their distance). The harp as it
  // stands: the board's foot 20 cm up, the board rising leaning back (55 degrees from the floor:
  // the strings, at 35 degrees to it, near upright), its face towards the microphones and up,
  // the holes 25 cm behind it.
  nP_ = hb::kPatches;
  pvol_.assign(hb::kPatchVol, hb::kPatchVol + size_t(nP_) * nB_);
  ring_.assign(size_t(nP_) * kRing, 0.0f);
  for (int c = 0; c < 2; ++c) {
    tapI_[c].assign(nP_, 0);
    tapF_[c].assign(nP_, 0.0f);
    tapG_[c].assign(nP_, 0.0f);
    const double mx = c ? 0.3 : -0.3, my = 2.0, mz = 1.2;
    for (int p = 0; p < nP_; ++p) {
      const double u = hb::kPatchPos[3 * p], a = hb::kPatchPos[3 * p + 1], nn = hb::kPatchPos[3 * p + 2];
      const double px = a, py = -0.574 * u + 0.819 * nn, pz = 0.2 + 0.819 * u + 0.574 * nn;
      const double r = std::sqrt((px - mx) * (px - mx) + (py - my) * (py - my) + (pz - mz) * (pz - mz));
      const double d = std::min(r / kC * fs_, double(kRing - 2));
      tapI_[c][p] = int(d);
      tapF_[c][p] = float(d - int(d));
      tapG_[c][p] = float(kRhoAir / (4 * kPi * r) * 2.0);
    }
  }
  // The strings.
  const double topGut = knob("PJ_HARP_TOP", 8000), topWire = knob("PJ_HARP_TOPWIRE", 6000);
  uint32_t rng = 4747;
  for (int s = 0; s < kStrings; ++s) {
    Str& st = str_[s];
    st.full = hb::kLength[s];
    st.mu = hb::kMu[s];
    st.T = hb::kTension[s];
    st.EA = hb::kEA[s];
    st.B = hb::kB[s];
    st.pos = hasDiscs(s) ? -1 : 0;
    const bool wire = hb::kMaterial[s] == 0;
    const double top = std::min(0.45 * fs_, wire ? topWire : topGut);
    const double f0 = hb::kFull[s] / std::sqrt(1 + st.B);
    int n = 0;
    while (n < kMaxModes && (n + 1) * f0 * std::sqrt(1 + st.B * (n + 1) * (n + 1)) < top) ++n;
    st.n = std::min(kMaxModes, (std::max(n, 1) + 15) / 16 * 16);
    st.top = top;
    // Gut is alive (each partial a few cents off, its loss +-25 %); nylon less, the wires little.
    const double cents = wire ? 0.7 : hb::kMaterial[s] == 1 ? 3.0 : 1.5, lossJit = wire ? 0.1 : hb::kMaterial[s] == 1 ? 0.25 : 0.15;
    for (int i = 0; i < kMaxModes; ++i) {
      st.cents[i] = float(i == 0 ? 0 : cents * (2 * urand(rng) - 1));
      st.alive[i] = float(1 + lossJit * (2 * urand(rng) - 1));
      // (across the plane each partial its own 0.05-0.3 % sharper)
      st.split[i] = float(0.0005 * std::pow(6.0, urand(rng)));
    }
    tune(s);
  }
  reset();
}

void Harp::tune(int s) {
  namespace hb = harpboard;
  Str& st = str_[s];
  const double r = hasDiscs(s) ? std::exp2(-(st.pos + 1) / 12.0) : 1.0;
  const double L = st.full * r, B = st.B / (r * r);
  const double f0 = hb::kFull[s] / std::sqrt(1 + st.B) / r;
  st.len = L;
  st.f1 = f0 * std::sqrt(1 + B);
  st.invMass = float(2 / (st.mu * L));
  const int mat = hb::kMaterial[s];
  const double d = hb::kDiameter[s], rhoEff = hb::kRhoEff[s], etaE = hb::kEtaE[s];
  static const double kRing = knob("PJ_HARP_RING", 1.0), kDisc = knob("PJ_HARP_DISCLOSS", 1.1);
  static const double kWire = knob("PJ_HARP_WIRELOSS", 1.1e-4);  // (the piano's wound strings' own loss factor)
  // The ends' give: the neck (its pin, the discs) and the eyelet as a resistance R (kg/s): every
  // mode, both directions, decays at T / (L R) by it. Fitted: 700 kg/s brings the one string
  // measured (C-flat 3, 123.6 Hz) to its 10 s, 4-8 s after the pluck, in the strings' plane (the
  // air and the board alone: 19 s); the wires then ring 12-20 s (research/harp.md 6.1).
  static const double kNeckR = knob("PJ_HARP_NECKR", 700);
  const double ends = kNeckR > 0 ? st.T / (L * kNeckR) : 0.0;
  const double disc = hasDiscs(s) && st.pos >= 0 ? kDisc : 1.0;
  for (int i = 0; i < st.n; ++i) {
    const int k = i + 1;
    const double f = k * f0 * std::sqrt(1 + B * k * k) * std::exp2(st.cents[i] / 1200);
    const double w = 2 * kPi * f;
    if (f > st.top) {
      for (int p = 0; p < 2; ++p) st.x[p][i] = st.v[p][i] = st.cs[p][i] = st.sn[p][i] = 0;
      st.in[i] = st.out[i] = st.stretch[i] = st.w[i] = 0;
      continue;
    }
    // What it loses by itself (2.4): to the air (Stokes), and in bending.
    const double M = d / 4 * std::sqrt(w / kNuAir);
    const double etaAir = kRhoAir / rhoEff * (2 * std::sqrt(2.0) * M + 1) / (M * M);
    const double lambda = B * k * k / (1 + B * k * k);
    const double eta = (etaAir + (mat == 0 ? kWire : etaE * lambda)) * st.alive[i] * disc * kRing;
    const double g = std::exp(-(0.5 * eta * w + ends) * dt_);
    const double wOld = st.w[i];
    for (int p = 0; p < 2; ++p) {
      // (the two directions either side of the note: in the plane half the split flat, across it
      // half sharp)
      const double wp = w * (1 + (p ? 0.5 : -0.5) * st.split[i]);
      st.cs[p][i] = float(g * std::cos(wp * dt_));
      st.sn[p][i] = float(g * std::sin(wp * dt_));
      if (wOld > 0) st.x[p][i] *= float(w / wOld);  // (the swing kept, at the new pitch)
    }
    st.w[i] = float(w);
    st.in[i] = float(-2 / (k * kPi) * w);
    st.out[i] = float(st.mu * L / (k * kPi));
    st.stretch[i] = float(st.EA * kPi * kPi * k * k / (4 * st.T * L * L * w * w));
  }
  hands(s);
}

void Harp::hands(int s) {
  Str& st = str_[s];
  st.handOn = st.dampRate > 0 || st.touching;
  if (!st.handOn) return;
  const double L = st.len;
  const double ref = under(1, st.dampAt, st.dampLen, L);
  for (int i = 0; i < st.n; ++i) {
    double rate = 0;
    if (st.dampRate > 0) {
      // (soft flesh takes only so much: at most 30 times what it takes of the fundamental)
      const double r = under(i + 1, st.dampAt, st.dampLen, L) / std::max(ref, 1e-6);
      rate += st.dampRate * r / (1 + r / 30);
    }
    // A harmonic: the side of the hand at the middle, 1 cm, pressing hard (the odd modes gone in
    // a few periods; the even ones, with their node there, hardly touched).
    if (st.touching) rate += 3000 * under(i + 1, 0.5, 0.01, L);
    st.hand[i] = float(std::exp(-rate * dt_));
  }
}

void Harp::pushShape(Str& st, double at, double width, double angle) {
  const double L = st.len;
  for (int i = 0; i < kMaxModes; ++i) {
    const int k = i + 1;
    const double a = k * kPi * width / (2 * L);
    const double shape = i < st.n ? std::sin(k * kPi * at) * (a > 1e-6 ? std::sin(a) / a : 1.0) : 0.0;
    st.push[0][i] = float(shape * std::cos(angle) * st.invMass * dt_);
    st.push[1][i] = float(shape * std::sin(angle) * st.invMass * dt_);
  }
}

void Harp::pluck(int s, const Pluck& p) {
  Str& st = str_[s];
  const double at = std::clamp(p.at, 0.02, 0.98), z = at * st.len;
  st.pull = std::max(p.pull, 0.0005);
  st.slip = std::max(p.slip, 0.0002);
  // (the string leaves the pad's edge as its own swing takes it away: within a fifth of its period)
  st.off = std::clamp(std::min(p.off, 0.2 / st.f1), 0.00005, 0.01);
  st.carry = std::max(p.carry, 0.0) / std::max(p.depth, 1e-5);
  st.force = st.T * p.depth * st.len / (z * (st.len - z));
  // (the pad stops what the string sounded: about critically for its fundamental)
  st.holdRate = p.stop * std::clamp(1.5 * 2 * kPi * st.f1, 100.0, 1500.0);
  st.harmonic = std::max(p.harmonic, 0.0);
  pushShape(st, at, p.width, p.angle);
  st.dampRate = 0;
  st.touching = st.harmonic > 0;
  hands(s);
  st.clock = 0;
  asleep_ = false;
  quiet_ = 0;
}

void Harp::damp(int s, double rate, double at, double len) {
  Str& st = str_[s];
  rate = std::max(rate, 0.0);
  if (rate == st.dampRate && at == st.dampAt && len == st.dampLen) return;
  st.dampRate = rate;
  st.dampAt = at;
  st.dampLen = std::min(len, 0.9 * st.len);
  hands(s);
}

void Harp::pedal(int letter, int pos) {
  letter = std::clamp(letter, 0, 6);
  pos = std::clamp(pos, -1, 1);
  pedal_[letter] = pos;
  for (int s = letter; s < kStrings; s += 7)
    if (hasDiscs(s) && str_[s].pos != pos) {
      str_[s].pos = pos;
      tune(s);
    }
}

void Harp::reset() {
  for (Str& st : str_) {
    for (int p = 0; p < 2; ++p) {
      std::fill(st.x[p], st.x[p] + kMaxModes, 0.0f);
      std::fill(st.v[p], st.v[p] + kMaxModes, 0.0f);
    }
    st.clock = 1e9;
    st.eps = 0;
    st.energy = 0;
    st.touching = false;
    st.dampRate = 0;
    st.handOn = false;
  }
  std::fill(bx_.begin(), bx_.end(), 0.0f);
  std::fill(bv_.begin(), bv_.end(), 0.0f);
  std::fill(accT_, accT_ + kStrings, 0.0f);
  std::fill(accS_, accS_ + kStrings, 0.0f);
  std::fill(ring_.begin(), ring_.end(), 0.0f);
  side_ = 0;
  asleep_ = true;
}

float Harp::sample() {
  if (asleep_) return 0;
  const float dt = float(dt_);
  const float cosT = std::sqrt(1 - sinT_ * sinT_);
  const int nB = nB_;
  float* bf = bf_.data();
  std::fill(bf, bf + nB, 0.0f);
  const bool count = (++clock_ & 255) == 0;
  bool fingers = false;
  for (int s = 0; s < kStrings; ++s) {
    Str& st = str_[s];
    // The finger: taking the string and drawing it out, sliding with it, leaving the pad.
    float F = 0, hold = 1;
    if (st.clock < 1e8) {
      fingers = true;
      const double t = st.clock, t1 = st.pull, t2 = t1 + st.slip, t3 = t2 + st.off;
      st.clock += dt_;
      if (t < t1) {
        F = float(st.force * (0.5 - 0.5 * std::cos(kPi * t / t1)));
        hold = float(std::exp(-st.holdRate * dt_));
      } else if (t < t2) {
        F = float(st.force * (1 + st.carry * (0.5 - 0.5 * std::cos(kPi * (t - t1) / st.slip))));
      } else if (t < t3) {
        F = float(st.force * (1 + st.carry) * (0.5 + 0.5 * std::cos(kPi * (t - t2) / st.off)));
      } else if (t >= t3 + st.harmonic) {
        st.clock = 1e9;
        if (st.touching) {
          st.touching = false;
          hands(s);
        }
      }
    }
    const float dyT = accT_[s], dyS = accS_[s];
    const float nl = std::min(st.eps, 0.5f) * dt;
    const int n = st.n;
    float sumT = 0, sumS = 0, eps = 0;
    {
      float* x = st.x[0];
      float* v = st.v[0];
      const float *cs = st.cs[0], *sn = st.sn[0], *in = st.in, *out = st.out, *pu = st.push[0], *w = st.w, *sc = st.stretch;
      for (int i = 0; i < n; ++i) {
        const float xx = x[i] + in[i] * dyT;
        const float vv = v[i] + F * pu[i] - nl * w[i] * xx;
        const float xn = xx * cs[i] + vv * sn[i];
        const float vn = vv * cs[i] - xx * sn[i];
        sumT += out[i] * (vn - vv);
        x[i] = xn;
        v[i] = vn;
        eps += sc[i] * xn * xn;
      }
    }
    {
      float* x = st.x[1];
      float* v = st.v[1];
      const float *cs = st.cs[1], *sn = st.sn[1], *in = st.in, *out = st.out, *pu = st.push[1], *w = st.w, *sc = st.stretch;
      for (int i = 0; i < n; ++i) {
        const float xx = x[i] + in[i] * dyS;
        const float vv = v[i] + F * pu[i] - nl * w[i] * xx;
        const float xn = xx * cs[i] + vv * sn[i];
        const float vn = vv * cs[i] - xx * sn[i];
        sumS += out[i] * (vn - vv);
        x[i] = xn;
        v[i] = vn;
        eps += sc[i] * xn * xn;
      }
    }
    // A hand on it, the finger's pad stopping it: taken from the string, not given to the board.
    if (st.handOn || hold < 1) {
      for (int p = 0; p < 2; ++p) {
        float* x = st.x[p];
        float* v = st.v[p];
        if (st.handOn) {
          const float* h = st.hand;
          for (int i = 0; i < n; ++i) {
            x[i] *= h[i] * hold;
            v[i] *= h[i] * hold;
          }
        } else {
          for (int i = 0; i < n; ++i) {
            x[i] *= hold;
            v[i] *= hold;
          }
        }
      }
    }
    st.eps = eps;
    // Its ends' pull on the board: across the string in its plane (cos 35 of it towards the
    // board's normal), the tension's rise along it (sin 35), and across the plane (the eyelet).
    const float fn = -cosT * sumT + sinT_ * float(st.T) * eps * dt, fs = -sumS;
    const float* ph = &phi_[size_t(s) * nB];
    const float* sd = &side_phi_[size_t(s) * nB];
    for (int m = 0; m < nB; ++m) bf[m] += ph[m] * fn + sd[m] * fs;
    if (count) {
      double e[2] = {0, 0};
      for (int p = 0; p < 2; ++p)
        for (int i = 0; i < n; ++i) e[p] += double(st.x[p][i]) * st.x[p][i] + double(st.v[p][i]) * st.v[p][i];
      st.energy = 0.25 * st.mu * st.len * (e[0] + e[1]);
      st.energyPlane = 0.25 * st.mu * st.len * e[0];
    }
  }
  // The board.
  float* bx = bx_.data();
  float* bv = bv_.data();
  float* bd = bd_.data();
  float* ba = bacc_.data();
  const float* bcs = bcs_.data();
  const float* bsn = bsn_.data();
  const float* binv = binv_.data();
  const float fsf = float(fs_);
  for (int m = 0; m < nB; ++m) {
    const float vv = bv[m] + bf[m];
    const float xn = bx[m] * bcs[m] + vv * bsn[m];
    const float vn = vv * bcs[m] - bx[m] * bsn[m];
    ba[m] = (vn - bv[m]) * fsf;
    bd[m] = (xn - bx[m]) * binv[m];
    bx[m] = xn;
    bv[m] = vn;
  }
  for (int s = 0; s < kStrings; ++s) {
    const float* ph = &phi_[size_t(s) * nB];
    const float* sd = &side_phi_[size_t(s) * nB];
    float a = 0, b = 0;
    for (int m = 0; m < nB; ++m) {
      a += ph[m] * bd[m];
      b += sd[m] * bd[m];
    }
    accT_[s] = cosT * a;
    accS_[s] = b;
  }
  // The air each patch moves, and what of it reaches the two microphones now.
  ringPos_ = (ringPos_ + 1) & (kRing - 1);
  float l = 0, r = 0;
  for (int p = 0; p < nP_; ++p) {
    const float* pv = &pvol_[size_t(p) * nB];
    float q = 0;
    for (int m = 0; m < nB; ++m) q += pv[m] * ba[m];
    float* ring = &ring_[size_t(p) * kRing];
    ring[ringPos_] = q;
    const int i0 = (ringPos_ - tapI_[0][p]) & (kRing - 1), i1 = (ringPos_ - tapI_[1][p]) & (kRing - 1);
    l += tapG_[0][p] * (ring[i0] + tapF_[0][p] * (ring[(i0 - 1) & (kRing - 1)] - ring[i0]));
    r += tapG_[1][p] * (ring[i1] + tapF_[1][p] * (ring[(i1 - 1) & (kRing - 1)] - ring[i1]));
  }
  const float p = 0.5f * (l + r);
  side_ = kOut_ * 0.5f * (l - r);
  const float out = kOut_ * p;
  if (!std::isfinite(out)) {
    reset();
    return 0;
  }
  if (count) {
    double e = 0, eb = 0;
    for (const Str& st : str_) e = std::max(e, st.energy);
    for (int m = 0; m < nB; ++m) eb += double(bv[m]) * bv[m];
    if (!fingers && e < 1e-13 && eb < 1e-16) {
      if (++quiet_ > int(0.25 * fs_ / 256)) {
        reset();
        return 0;
      }
    } else {
      quiet_ = 0;
    }
  }
  return out;
}

}  // namespace pajupilli
