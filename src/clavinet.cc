#include "clavinet.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>

#include "clavtone_data.h"
#include "knob.h"

namespace pajupilli {

bool clavinet(const Instrument& inst) { return std::strncmp(inst.name, "clavinet", 8) == 0; }

namespace {

// Log-linear through (key, value) points (keys 1..60).
double through(const double (*pts)[2], int n, double key) {
  if (key <= pts[0][0]) return pts[0][1];
  for (int i = 1; i < n; ++i)
    if (key <= pts[i][0]) {
      const double t = (key - pts[i - 1][0]) / (pts[i][0] - pts[i - 1][0]);
      return std::exp(std::log(pts[i - 1][1]) + t * (std::log(pts[i][1]) - std::log(pts[i - 1][1])));
    }
  return pts[n - 1][1];
}

// The scale (research/clavinet.md 4.2): lengths estimated (only E3's, 0.678 m,
// is published). Stiffness: in the bass and at the top as measured by
// Gabrielli et al. (2013); from E2 to D#4 as measured on a recorded D6 (21
// notes, tools/pianofit.py: 3.5 .. 8e-5, half Gabrielli's there, and what
// the plain wire's gauge on E3's length gives, 5.3e-5); between D#4 and F#5
// rising as the plain wire's 1 / (L^4 f^2) does.
const double kLen[][2] = {{1, 0.82}, {18, 0.74}, {24, 0.678}, {36, 0.43}, {41, 0.36}, {50, 0.254}, {60, 0.18}};
const double kStiff[][2] = {{1, 5e-4}, {6, 2e-4}, {12, 6.6e-5}, {23, 4.7e-5}, {30, 5.4e-5}, {35, 4e-5}, {50, 9e-5}, {60, 8e-5}};

// The held string's decay (research 4.3): T60 20 s at 100 Hz, 8 s at 1 kHz,
// 1.5 s at 5 kHz (only the ends take energy: the tailpiece and the rubber).
double heldDecay(double f) {
  static const double kScale = knob("PJ_CLAV_SUSTAIN", 1.0);
  const double T60 = f < 1000 ? 20 * std::pow(std::max(f, 30.0) / 100, -0.398) : 8 * std::pow(f / 1000, -1.04);
  return 6.91 / (T60 * kScale);
}

// The mute bar's felt near the tailpiece: every mode, the highs more.
double muteDecay(double f, double mute) { return mute * (10 + 0.01 * f); }

constexpr double kTop = 12000;  // modes up to here (Hz)

// The frame's knock (research 3.3: below 1.2 kHz; a guess at its resonances).
const double kKnockHz[] = {180, 330, 560, 820, 1150}, kKnockQ[] = {8, 12, 10, 15, 12};
const double kKnockW[] = {1, 0.7, 0.5, 0.35, 0.25};

}  // namespace

ClavinetModel::ClavinetModel(const Instrument& inst)
    : Tube(inst),
      model_(std::strcmp(inst.name, "clavinet-e7") == 0  ? Model::kE7
             : std::strcmp(inst.name, "clavinet-c") == 0 ? Model::kC
                                                          : Model::kD6),
      dt_(1.0 / inst.fs) {
  static const double kYarn = knob("PJ_CLAV_YARN", 1.0);  // x the yarn's damping (worn yarn: less)
  static const double kSide = knob("PJ_CLAV_SIDE", 0.6);  // the share of keys below E4 that beat
  std::mt19937 rng(4242);
  std::uniform_real_distribution<double> u01(0, 1);
  str_.resize(kKeys);
  for (int k = 0; k < kKeys; ++k) {
    Str& s = str_[k];
    const int key = k + 1;
    s.f0 = 440 * std::exp2((kLowest + k - 69) / 12.0);
    s.B = through(kStiff, 8, key);
    s.L = through(kLen, 7, key);
    s.Lt = 1.19 * s.L;  // the anvil at a constant share: three semitones
    s.dC = 0.185 + (0.065 - 0.185) * k / (kKeys - 1.0);
    // The modes (on L, from the tailpiece end): partial 1 at f1.
    auto build = [&](Bank& b, double f1, double B, double L, bool speaking) {
      const double base = f1 / std::sqrt(1 + B);
      for (int n = 1;; ++n) {
        const double f = base * n * std::sqrt(1 + B * n * n);
        if (f > kTop || f > 0.45 / dt_) break;
        b.w.push_back(float(2 * M_PI * f));
        b.wc.push_back(float(std::sin(n * M_PI * s.dC / L)));
        b.wb.push_back(float(std::sin(n * M_PI * s.dB / L)));
        // The moving end (at x = L) drives mode n by -c_n u'' with c_n = 2 (-1)^(n+1) / (n pi):
        // a += i c_n u'' dt / w per sample.
        b.drive.push_back(speaking ? float((n % 2 ? 2 : -2) / (n * M_PI) * dt_ / (2 * M_PI * f)) : 0.0f);
        b.sig.push_back(heldDecay(f));
      }
      b.n = int(b.w.size());
      b.re.assign(b.n, 0.0f);
      b.im.assign(b.n, 0.0f);
      b.pr.resize(b.n);
      b.pi.resize(b.n);
    };
    build(s.s, s.f0, s.B, s.L, true);
    turns(s.s, 1);
    // The sideways swing (below E4, a share of the keys): tuned 0.5 .. 2 Hz
    // apart at the fundamental, coupled to the up-and-down at the rubber tip
    // (0.4 Hz): the energy goes to and fro and the note beats, up to ~15 dB
    // (research 4.3). The blow is nearly straight down (a sixth of it
    // sideways); the pickups hear the sideways swing 25 dB down.
    // Each beating key: its rate R (0.5 .. 2 Hz) and how much of the energy
    // goes across, D (0.3 .. 0.95: up to ~15 dB p-p), as two swings apart by
    // R sqrt(1 - D) coupled by R sqrt(D) / 2.
    const double beatHz = 0.5 + 1.5 * u01(rng), depth = 0.3 + 0.65 * u01(rng);
    if (kLowest + k < 64 && u01(rng) < kSide) {
      s.detune = beatHz * std::sqrt(1 - depth) / s.f0;
      s.side.re.assign(s.s.n, 0.0f);
      s.side.im.assign(s.s.n, 0.0f);
      s.side.pr.resize(s.s.n);
      s.side.pi.resize(s.s.n);
      s.side.g = float(2 * M_PI * beatHz * std::sqrt(depth) / 2 * dt_);
      s.side.tilt = 0.17f;
      sideTurns(s, 1);
    }
    const double fj = s.f0 * s.L / s.Lt, Bj = s.B * (s.L / s.Lt) * (s.L / s.Lt);
    build(s.j, fj, Bj, s.Lt, false);
    // The yarn on the joined string's end part (a = Lt - L): mode m loses by
    // r w_m / (2 mu), w_m = a / Lt - sin(2 pi m a / Lt) / (2 pi m): the
    // fundamental of the dropped note ~6 times more slowly than the highs
    // (T60 ~30 ms for the highs: research 5.1).
    const double a = (s.Lt - s.L) / s.Lt;
    s.yarn.resize(s.j.n);
    for (int m = 1; m <= s.j.n; ++m) {
      s.yarn[m - 1] = float(kYarn * 1440 * (a - std::sin(2 * M_PI * m * a) / (2 * M_PI * m)));
      s.j.sig[m - 1] += s.yarn[m - 1];
    }
    turns(s.j, 1);
    // The speaking modes as the joined string's: P_mn = (2 / Lt) int_0^L
    // sin(n pi x / L) sin(m pi x / Lt) dx; the released bend (h at x = L, a
    // triangle to the two ends) as a pluck.
    s.P.resize(size_t(s.j.n) * s.s.n);
    s.pluck.resize(s.j.n);
    for (int m = 1; m <= s.j.n; ++m) {
      const double b = m * M_PI / s.Lt;
      for (int n = 1; n <= s.s.n; ++n) {
        const double c = n * M_PI / s.L;
        const double v = std::fabs(c - b) < 1e-9 ? s.L / 2 - std::sin(2 * c * s.L) / (4 * c)
                                                 : std::sin((c - b) * s.L) / (2 * (c - b)) - std::sin((c + b) * s.L) / (2 * (c + b));
        s.P[size_t(m - 1) * s.s.n + n - 1] = float(2 / s.Lt * v);
      }
      s.pluck[m - 1] = float(2 * s.Lt * s.Lt * std::sin(b * s.L) / (m * m * M_PI * M_PI * s.L * (s.Lt - s.L)));
    }
  }
  for (int r = 0; r < kKnock; ++r) {
    kw_[r] = 2 * M_PI * kKnockHz[r];
    const double d = std::exp(-kw_[r] / (2 * kKnockQ[r]) * dt_);
    kpr_[r] = d * std::cos(kw_[r] * dt_);
    kpi_[r] = d * std::sin(kw_[r] * dt_);
  }
}

// A bank's per-sample turns from its decays, its tuning x ratio.
void ClavinetModel::turns(Bank& b, double ratio) {
  for (int i = 0; i < b.n; ++i) {
    const double r = std::exp(-b.sig[i] * dt_), ph = b.w[i] * ratio * dt_;
    b.pr[i] = float(r * std::cos(ph));
    b.pi[i] = float(r * std::sin(ph));
  }
}

void ClavinetModel::sideTurns(Str& s, double ratio) {
  if (s.side.re.empty()) return;
  for (int i = 0; i < s.s.n; ++i) {
    const double r = std::exp(-s.s.sig[i] * dt_), ph = s.s.w[i] * ratio * (1 + s.detune) * dt_;
    s.side.pr[i] = float(r * std::cos(ph));
    s.side.pi[i] = float(r * std::sin(ph));
  }
}

void ClavinetModel::setTone(int bits) {
  bits &= 15;
  if (bits == tone_) return;
  // A rocker's change: the old setting fades out over 5 ms under the new.
  std::memcpy(sos_[1], sos_[0], sizeof sos_[0]);
  toneOld_ = tone_;
  tone_ = bits;
  fade_ = int(0.005 / dt_);
  std::memset(sos_[0], 0, sizeof sos_[0]);
}

void ClavinetModel::setPressure(double cents) {
  if (std::fabs(cents - press_) < 0.05) return;
  press_ = cents;
  const double ratio = std::exp2(cents / 1200);
  for (Str& s : str_)
    if (s.down && std::fabs(s.stretch - ratio) > 1e-6) {
      s.stretch = ratio;
      turns(s.s, ratio);
      sideTurns(s, ratio);
    }
}

void ClavinetModel::strike(int key, double speed) {
  if (key < 0 || key >= kKeys) return;
  Str& s = str_[key];
  if (s.down) return;
  // A string still ringing after its last release: the tangent clamps it
  // again. What swings on the speaking length goes on swinging there, under
  // the new stroke (the old note decays as it would, not cut off); the part
  // beyond the anvil is lost in the yarn (quickly). Speaking amplitudes from
  // the whole string's: b_n = (2 / L) int_0^L y sin(n pi x / L) dx =
  // (Lt / L) sum_m P_mn a_m.
  if (s.j.on) {
    const double k = s.Lt / s.L;
    std::vector<double> qs(s.s.n, 0.0), vs(s.s.n, 0.0);
    for (int m = 0; m < s.j.n; ++m) {
      const double q = s.j.re[m], v = -s.j.w[m] * s.j.im[m];
      const float* row = &s.P[size_t(m) * s.s.n];
      for (int n = 0; n < s.s.n; ++n) {
        qs[n] += k * row[n] * q;
        vs[n] += k * row[n] * v;
      }
    }
    for (int n = 0; n < s.s.n; ++n) {
      s.s.re[n] += float(qs[n]);
      s.s.im[n] += float(-vs[n] / s.s.w[n]);
    }
    // ... and the whole string keeps only what is beyond the anvil.
    for (int m = 0; m < s.j.n; ++m) {
      const float* row = &s.P[size_t(m) * s.s.n];
      double q = 0, v = 0;
      for (int n = 0; n < s.s.n; ++n) {
        q += row[n] * qs[n];
        v += row[n] * vs[n];
      }
      s.j.re[m] -= float(q);
      s.j.im[m] -= float(-v / s.j.w[m]);
    }
    for (int m = 0; m < s.j.n; ++m) s.j.sig[m] += 300;
    turns(s.j, 1);
    for (int m = 0; m < s.j.n; ++m) s.j.sig[m] -= 300;
  }
  const double ratio = std::exp2(press_ / 1200);
  if (std::fabs(s.stretch - ratio) > 1e-6) {
    s.stretch = ratio;
    turns(s.s, ratio);
    sideTurns(s, ratio);
  }
  s.down = true;
  s.s.on = true;
  s.knocked = false;
  s.t = 0;
  s.h = 1e-3;
  s.speed = std::max(speed, 0.05);
  s.tau = s.h / s.speed;
  s.quiet = 0;
}

void ClavinetModel::release(int key) {
  if (key < 0 || key >= kKeys) return;
  Str& s = str_[key];
  if (!s.down) return;
  s.down = false;
  // The whole string, freed: its modes from the speaking ones and the bend.
  // (The sideways swing, a twentieth of it, goes into the yarn.)
  const double h = s.t >= s.tau ? s.h : s.h * 0.5 * (1 - std::cos(M_PI * s.t / s.tau));
  for (int m = 0; m < s.j.n; ++m) {
    double q = h * s.pluck[m], v = 0;
    const float* row = &s.P[size_t(m) * s.s.n];
    for (int n = 0; n < s.s.n; ++n) {
      q += row[n] * s.s.re[n];
      v += row[n] * (-s.s.w[n] * s.s.im[n]);
    }
    s.j.re[m] += float(q);
    s.j.im[m] += float(-v / s.j.w[m]);
  }
  turns(s.j, 1);  // (as tuned: a clamp's quick decay undone)
  s.j.on = true;
  std::fill(s.s.re.begin(), s.s.re.end(), 0.0f);
  std::fill(s.s.im.begin(), s.s.im.end(), 0.0f);
  std::fill(s.side.re.begin(), s.side.re.end(), 0.0f);
  std::fill(s.side.im.begin(), s.side.im.end(), 0.0f);
  s.s.on = false;
  s.quiet = 0;
}

bool ClavinetModel::asleep() const {
  for (const Str& s : str_)
    if (s.s.on || s.j.on) return false;
  return true;
}

void ClavinetModel::resetAir() {
  Tube::resetAir();
  for (Str& s : str_) {
    for (Bank* b : {&s.s, &s.j}) {
      std::fill(b->re.begin(), b->re.end(), 0.0f);
      std::fill(b->im.begin(), b->im.end(), 0.0f);
      b->on = false;
    }
    std::fill(s.side.re.begin(), s.side.re.end(), 0.0f);
    std::fill(s.side.im.begin(), s.side.im.end(), 0.0f);
    // (A key still down stays down: its string at rest on the anvil, so a
    // release still lets the bend go.)
    s.s.on = s.down;
    s.t = s.tau;
    s.knocked = true;
  }
  std::fill(kre_, kre_ + kKnock, 0.0);
  std::fill(kim_, kim_ + kKnock, 0.0);
  hpX_ = hpY_ = lp1_ = lp2_ = lp3_ = env_ = ic1_ = ic2_ = 0;
  std::memset(sos_, 0, sizeof sos_);
}

float ClavinetModel::sample() {
  static const double kDrive = knob("PJ_CLAV_DRIVE", 1.5);    // pickup EMF (per m/s) at stage 2's input (V)
  static const double kNl = knob("PJ_CLAV_NL", 0.15);         // the magnet's field over the string's swing
  static const double kGc = knob("PJ_CLAV_GC", 1.0), kGb = knob("PJ_CLAV_GB", 1.0);
  static const double kKnockAmt = knob("PJ_CLAV_KNOCK", 1.0);  // x the frame's knock
  if (mute_ != muteSet_) {
    muteSet_ = mute_;
    for (Str& s : str_) {
      for (int n = 0; n < s.s.n; ++n) {
        const double f = s.s.w[n] / (2 * M_PI);
        s.s.sig[n] = heldDecay(f) + muteDecay(f, mute_);
      }
      turns(s.s, s.stretch);
      sideTurns(s, s.stretch);
    }
  }
  // The frame: its knock moves every sounding string's end, and so the whole
  // string with it (below the string's own modes it follows: u x d / L at the
  // pickups), and drives the modes by its acceleration.
  double kacc = 0, kvel = 0;
  for (int r = 0; r < kKnock; ++r) {
    const double re = kpr_[r] * kre_[r] - kpi_[r] * kim_[r];
    kim_[r] = kpr_[r] * kim_[r] + kpi_[r] * kre_[r];
    kre_[r] = re;
    kacc -= kw_[r] * kw_[r] * re;
    kvel -= kw_[r] * kim_[r];
  }
  double emfC = 0, emfB = 0;
  for (Str& s : str_) {
    if (!s.s.on && !s.j.on) continue;
    double yC = 0, vC = 0, yB = 0, vB = 0;
    if (s.s.on) {
      // The tangent's travel: a raised-cosine ramp of h over tau; the end's
      // own place and speed reach the pickups too (u x d / L).
      double u = s.h, ud = 0, udd = 0;
      if (s.t < s.tau) {
        const double ph = M_PI * s.t / s.tau;
        u = s.h * 0.5 * (1 - std::cos(ph));
        ud = s.h * 0.5 * M_PI / s.tau * std::sin(ph);
        udd = s.h * 0.5 * (M_PI / s.tau) * (M_PI / s.tau) * std::cos(ph);
      } else if (!s.knocked) {
        // The tip lands on the anvil: the frame is struck (by the key's speed).
        s.knocked = true;
        for (int r = 0; r < kKnock; ++r) kim_[r] -= kKnockAmt * 5e-5 * kKnockW[r] * s.speed;  // (~10 dB under a loud E6 in its first 60 ms)
      }
      s.t += dt_;
      const float drv = float(udd + kacc);
      Bank& b = s.s;
      const bool side = !s.side.re.empty();
      double hc = 0, hb = 0;
      for (int i = 0; i < b.n; ++i) {
        const float re = b.re[i], im = b.im[i] + b.drive[i] * drv;
        float nr = b.pr[i] * re - b.pi[i] * im, ni = b.pr[i] * im + b.pi[i] * re;
        if (side) {
          Side& h = s.side;
          const float sre = h.re[i], sim = h.im[i] + h.tilt * b.drive[i] * drv;
          float hr = h.pr[i] * sre - h.pi[i] * sim, hi = h.pr[i] * sim + h.pi[i] * sre;
          // Coupled at the tip: each swing pushes the other (i g dt x the other).
          const float vr = nr - h.g * hi, vi = ni + h.g * hr;
          hr -= h.g * ni;
          hi += h.g * nr;
          nr = vr;
          ni = vi;
          h.re[i] = hr;
          h.im[i] = hi;
          const float hv = -b.w[i] * hi;
          hc += b.wc[i] * hv;
          hb += b.wb[i] * hv;
        }
        b.re[i] = nr;
        b.im[i] = ni;
        const float v = -b.w[i] * ni;
        yC += b.wc[i] * nr;
        vC += b.wc[i] * v;
        yB += b.wb[i] * nr;
        vB += b.wb[i] * v;
      }
      vC += 0.056 * hc;  // (the sideways swing, 25 dB down at the pickups)
      vB += 0.056 * hb;
      yC += u * s.dC / s.L;
      vC += (ud + kvel) * s.dC / s.L;
      yB += u * s.dB / s.L;
      vB += (ud + kvel) * s.dB / s.L;
    }
    if (s.j.on) {
      Bank& b = s.j;
      double jv = 0, jw = 0;
      for (int i = 0; i < b.n; ++i) {
        const float re = b.re[i], im = b.im[i];
        const float nr = b.pr[i] * re - b.pi[i] * im, ni = b.pr[i] * im + b.pi[i] * re;
        b.re[i] = nr;
        b.im[i] = ni;
        const float v = -b.w[i] * ni;
        yC += b.wc[i] * nr;
        jv += b.wc[i] * v;
        yB += b.wb[i] * nr;
        jw += b.wb[i] * v;
      }
      // (On the joined string the pickups stand at the same distances from the tailpiece.)
      vC += jv;
      vB += jw;
      if (std::fabs(jv) + std::fabs(jw) < 1e-7) {
        if (++s.quiet > 2400) {
          b.on = false;
          std::fill(b.re.begin(), b.re.end(), 0.0f);
          std::fill(b.im.begin(), b.im.end(), 0.0f);
        }
      } else {
        s.quiet = 0;
      }
    }
    // The pickup: EMF ~ the field's slope where the string is, times its
    // velocity (the chain rule: p'(y) y'); the field rises towards the magnet.
    emfC += (1 + kNl * yC / 1e-3) * vC;
    emfB += (1 + kNl * yB / 1e-3) * vB;
  }
  emfC *= kGc;
  emfB *= kGb;
  double x = pickup_ == 0 ? emfC : pickup_ == 1 ? 0.5 * (emfC + emfB) : pickup_ == 2 ? emfB : 0.5 * (emfC - emfB);
  x = x * kDrive + testDp_;
  testDp_ = 0;
  double y;
  if (model_ == Model::kC) {
    // The C: no transformer, no tone switches. Two germanium stages: a
    // coupling high-pass, an uneven soft clip (the negative swing sooner),
    // a roll-off near 14 kHz (3 nF).
    const double aHp = std::exp(-2 * M_PI * 30 * dt_);
    const double hp = aHp * (hpY_ + x - hpX_);
    hpX_ = x;
    hpY_ = hp;
    const double c = 0.8;
    const double cl = hp >= 0 ? c * std::tanh(hp / c) : 0.6 * c * std::tanh(hp / (0.6 * c));
    lp1_ += (1 - std::exp(-2 * M_PI * 14000 * dt_)) * (cl - lp1_);
    lp3_ += (1 - std::exp(-2 * M_PI * 20 * dt_)) * (lp1_ - lp3_);  // (the clip's offset, blocked)
    y = lp1_ - lp3_;
  } else {
    // The transformer (a gentle band: 40 Hz .. 15 kHz) and stage 1 (its
    // feedback's 8.2 pF: a roll-off from 5 kHz).
    const double aHp = std::exp(-2 * M_PI * 40 * dt_);
    const double hp = aHp * (hpY_ + x - hpX_);
    hpX_ = x;
    hpY_ = hp;
    lp1_ += (1 - std::exp(-2 * M_PI * 15000 * dt_)) * (hp - lp1_);
    lp2_ += (1 - std::exp(-2 * M_PI * 5000 * dt_)) * (lp1_ - lp2_);
    // The tone switches (and the old setting fading out after a change).
    const auto& sos = model_ == Model::kE7 ? clavtone::kSosE7 : clavtone::kSos;
    auto run = [&](int bits, double (*st)[2], double in) {
      if (bits == 0) return 0.0;
      double v = in;
      for (int i = 0; i < clavtone::kSections; ++i) {
        const double* c = sos[bits][i];
        const double o = c[0] * v + st[i][0];
        st[i][0] = c[1] * v - c[3] * o + st[i][1];
        st[i][1] = c[2] * v - c[4] * o;
        v = o;
      }
      return v;
    };
    y = run(tone_, sos_[0], lp2_);
    if (fade_ > 0) {
      const double g = fade_ * dt_ / 0.005;
      y = (1 - g) * y + g * run(toneOld_, sos_[1], lp2_);
      --fade_;
    }
    // Stage 2, its swing on 9 V soft at the top: the D6 (x11) 1 % distortion
    // at 400 mV in, c tanh(x / c) with c = 1.14 V; the E7's stage 2 has less
    // gain (x4.5): it is driven less hard (an assumption: their levels at
    // the tone node are not known together).
    if (model_ == Model::kE7) y *= 0.6;
    const double c = 1.14;
    y = c * std::tanh(y / c);
  }
  // The pedals. A wah: a resonant band-pass swept by the pedal (heel 450 Hz,
  // toe 1.6 kHz, Q 5), a little of the low end left. An envelope filter: the
  // band-pass follows the playing's loudness (300 Hz .. 2.5 kHz, Q 3.5).
  {
    const double a = std::fabs(y);
    env_ += (a > env_ ? 1 - std::exp(-dt_ / 0.003) : 1 - std::exp(-dt_ / 0.08)) * (a - env_);
    double target = 0, fc = fc_, q = 4;
    if (wah_ >= 0) {
      target = 1;
      fc = 450 * std::pow(1600 / 450.0, std::clamp(wah_, 0.0, 1.0));
      q = 5;
    } else if (envAmt_ > 0) {
      target = 1;
      fc = 300 * std::pow(2500 / 300.0, std::clamp(env_ * envAmt_ * 2, 0.0, 1.0));
      q = 3.5;
    }
    fxMix_ += (1 - std::exp(-dt_ / 0.02)) * (target - fxMix_);
    fc_ += (1 - std::exp(-dt_ / 0.005)) * (fc - fc_);
    if (fxMix_ > 1e-4) {
      const double g = std::tan(M_PI * std::min(fc_, 0.4 / dt_) * dt_), k = 1 / q;
      const double a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2;
      const double v3 = y - ic2_, v1 = a1 * ic1_ + a2 * v3, v2 = ic2_ + a2 * ic1_ + a3 * v3;
      ic1_ = 2 * v1 - ic1_;
      ic2_ = 2 * v2 - ic2_;
      const double wet = 1.8 * k * v1 + 0.1 * v2;
      y = (1 - fxMix_) * y + fxMix_ * wet;
    } else {
      ic1_ = ic2_ = 0;
    }
  }
  last_ = float(y);
  return finish(last_);
}

ClavinetPlayer::ClavinetPlayer(ClavinetModel& m) : Performer(m), m_(m) {}

void ClavinetPlayer::noteOn(int midi, int velocity) {
  const int key = midi - ClavinetModel::kLowest;
  if (key < 0 || key >= ClavinetModel::kKeys) return;
  articulate(midi, velocity, false);
  keyDown_[key] = true;
  note_ = midi;
  blowing_ = true;
  // The key's speed at the tip: 0.3 (pp) .. 1.5 m/s (ff), a harder touch faster.
  const double v = (0.3 + 1.2 * (velocity - 1) / 126.0) * hardness_ * (0.5 + dyn_);
  m_.strike(key, v);
}

void ClavinetPlayer::noteOff(int midi) {
  const int key = midi - ClavinetModel::kLowest;
  if (key < 0 || key >= ClavinetModel::kKeys) return;
  keyDown_[key] = false;
  if (!hold_) m_.release(key);
}

void ClavinetPlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int k = 0; k < ClavinetModel::kKeys; ++k)
      if (!keyDown_[k]) m_.release(k);
}

void ClavinetPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  // (Tests: PJ_CLAV_TONE the switches' bits, PJ_CLAV_PICKUP the selector, PJ_CLAV_MUTE the bar,
  // PJ_CLAV_WAH the pedal's place, PJ_CLAV_ENV the envelope filter, PJ_CLAV_PRESS cents.)
  static const double kTone = knob("PJ_CLAV_TONE", -1), kPick = knob("PJ_CLAV_PICKUP", -1);
  static const double kMute = knob("PJ_CLAV_MUTE", 0), kWah = knob("PJ_CLAV_WAH", -1);
  static const double kEnv = knob("PJ_CLAV_ENV", -1), kPress = knob("PJ_CLAV_PRESS", -1);
  static const double kBend = knob("PJ_CLAV_BEND", 10);  // the pressing's most (cents)
  // The brightness knob, dark to bright: Soft, Medium, Medium+Treble (the
  // usual), Treble, Medium+Treble+Brilliant, Treble+Brilliant, Brilliant.
  static const int kOrder[] = {1, 2, 6, 4, 14, 12, 8};
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    const double x = std::clamp((brightness_ - 0.3) / 2.2, 0.0, 0.999);
    m_.setTone(kTone >= 0 ? int(kTone) : kOrder[int(x * 7)]);
    m_.setPickup(kPick >= 0 ? int(kPick) : std::min(3, int(std::clamp(air_, 0.0, 0.999) * 4)));
    m_.setMute(kMute);
    m_.setPressure(kPress >= 0 ? kPress : kBend * std::clamp(2 * breathCtl_, 0.0, 1.0));
    m_.setWah(kWah >= 0 ? kWah : std::fabs(roll_) > 0.05 ? 0.5 * (roll_ + 1) : -1);
    m_.setEnvelope(kEnv >= 0 ? kEnv : std::clamp((vibDepth_ - 0.06) / 0.09, 0.0, 1.0));
    m_.process(out + i, m);
  }
  blowing_ = !m_.asleep();
}

}  // namespace pajupilli
