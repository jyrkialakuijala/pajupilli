#include "brightness.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"
#include "oversample_data.h"
#include "vmath.h"

namespace pajupilli {

bool fxBrightness(const Instrument& inst) { return std::strcmp(inst.name, "fx-brightness") == 0; }

namespace {

constexpr double kTwoPi = 6.283185307179586;
// The circuit, in units of the arc's operating point (its current and voltage: its resistance
// 1) and of the tank's period (L C = 1):
// - the ballast resistor (20 x the arc's resistance: nearly a constant current; the supply
//   E = 21 sets the operating point);
// - the tank's impedance sqrt(L / C) (1: the arc's resistance) and its Q (30: the coil's
//   resistance);
// - the arc's thermal time constant (0.5 / the tank's angular frequency: the arc's column is
//   sized to its note, so that the arc's falling characteristic still holds at the note);
// - the residual conductance of the hot electrodes (relights the arc if it goes out).
constexpr double kBallast = 20, kZ = 1, kQ = 30, kTau = 0.5, kResidual = 0.02;
// The column's share of the arc's voltage (its length) over which the arc sings: below ~0.27
// the falling characteristic cannot pay for the tank's losses. The tuning table spans it.
constexpr double kPhiLo = 0.27, kPhiHi = 0.67;
constexpr int kPhiN = 21;
constexpr double kSteps[] = {24, 32, 48, 64, 96, 128, 192, 256};  // steps per tank period
constexpr int kStepsN = sizeof(kSteps) / sizeof(kSteps[0]);
constexpr double kLevel = 0.2;
constexpr int kMaxNotes = 24;

// One step of an arc and its tank: phi the column's share, fl the plasma's flicker (a relative
// swing of the column's loss), s the key's slide-wire (the share of the arc's voltage it hands
// the tank: 0 .. 1 as the key goes down). Returns the rate of change of the arc's power (the
// pressure it radiates), per radian.
inline double arcStep(SingingArcModel::Arc& a, double phi, double fl, double s, double ek) {
  const double L = kZ, R = kZ / kQ, E = 1 + kBallast;
  const double G = a.g + 1 / kBallast;  // (the arc and the ballast, seen from the tank)
  // The tank (the arc's conductance held over the step) by the trapezoid: the tank sees s times
  // the arc's voltage, the arc in parallel with the ballast fed from the supply, E / (Rb G) -
  // s i / G; L di/dt = s v - R i - vc, dvc/dt = i / C (C = 1 / kZ).
  const double d = (s * s / G + R) / 2 + a.h * kZ / 4;
  const double itn = ((L / a.h - d) * a.it + s * E / (kBallast * G) - a.vc) / (L / a.h + d);
  a.vc += a.h * 0.5 * (a.it + itn) * kZ;
  a.it = itn;
  const double v = (E / kBallast - s * a.it) / G;  // the arc's voltage
  const double ia = a.g * v;                     // its current
  // Its loss (Mayr): the column's (its share of the operating power, flickering) and the
  // electrode falls' (a voltage times the current).
  const double loss = phi * (1 + fl) + (1 - phi) * std::fabs(ia) + 1e-9;
  const double target = ia * ia / loss + kResidual;
  a.g = target + (a.g - target) * ek;
  const double P = v * ia, dp = (P - a.p) / a.h;
  a.p = P;
  return dp;
}

// The arc burning steadily (silent) and its tank switched across it, uncharged: the arc's
// voltage steps onto the tank, which rings at once.
void startArc(SingingArcModel::Arc& a, double phi) {
  a.it = a.vc = 0;
  // (the operating point: the conductance its own power holds, by bisection)
  auto power = [&](double g, double& p) {
    const double v = (1 + kBallast) / kBallast / (g + 1 / kBallast), ia = g * v;
    p = v * ia;
    return ia * ia / (phi + (1 - phi) * ia) + kResidual - g;
  };
  double lo = 0.05, hi = 20, p = 1;
  for (int k = 0; k < 60; ++k) {
    const double mid = 0.5 * (lo + hi);
    (power(mid, p) > 0 ? lo : hi) = mid;
  }
  a.g = 0.5 * (lo + hi);
  power(a.g, p);
  a.p = p;
}

}  // namespace

SingingArcModel::SingingArcModel(const Instrument& inst) : Tube(inst) {
  // (the knobs' starting places; live's K1-K4 and -seq's "knob K X" move them)
  length_ = std::clamp(knob("PJ_ARC_LENGTH", 0.5), 0.0, 1.0);
  chorus_ = std::clamp(knob("PJ_ARC_CHORUS", 0.5), 0.0, 1.0);
  hiss_ = std::clamp(knob("PJ_ARC_HISS", 0.5), 0.0, 1.0);
  release_ = std::clamp(knob("PJ_ARC_RELEASE", 0.5), 0.0, 1.0);
  // The tuning table: run the arc (no flicker) at each length and step, and count the tank's
  // cycles once it has settled.
  rho_.assign(kPhiN * kStepsN, 1.0);
  for (int i = 0; i < kPhiN; ++i) {
    const double phi = kPhiLo + (kPhiHi - kPhiLo) * i / (kPhiN - 1);
    for (int j = 0; j < kStepsN; ++j) {
      Arc a;
      startArc(a, phi);
      a.h = kTwoPi / kSteps[j];
      const double ek = std::exp(-a.h / kTau);
      const int settle = int(30 * kSteps[j]), count = int(40 * kSteps[j]);
      double first = -1, last = -1;
      int cycles = 0;
      for (int k = 0; k < settle + count; ++k) {
        const double before = a.it;
        arcStep(a, phi, 0, 1, ek);
        if (k >= settle && before < 0 && a.it >= 0) {  // (an upward crossing, interpolated)
          const double x = k + before / (before - a.it);
          if (first < 0) first = x;
          else ++cycles;
          last = x;
        }
      }
      if (cycles >= 10) rho_[i * kStepsN + j] = cycles * kSteps[j] / (last - first);
    }
  }
}

// The arc's pitch over its tank's, by the column's share and the steps in the tank's period
// (linear in phi and in 1 / steps; beyond the table the integrator's first-order error is
// carried on to none).
double SingingArcModel::tuneRatio(double phi, double steps) const {
  const double x = std::clamp((phi - kPhiLo) / (kPhiHi - kPhiLo) * (kPhiN - 1), 0.0, kPhiN - 1.000001);
  const int i = int(x);
  const double fx = x - i;
  auto at = [&](int j) { return rho_[i * kStepsN + j] * (1 - fx) + rho_[(i + 1) * kStepsN + j] * fx; };
  if (steps >= kSteps[kStepsN - 1]) {
    const double r1 = at(kStepsN - 1), r0 = at(kStepsN - 2);
    const double a = (r0 - r1) / (1 / kSteps[kStepsN - 2] - 1 / kSteps[kStepsN - 1]);
    return r1 + a * (1 / steps - 1 / kSteps[kStepsN - 1]);
  }
  if (steps <= kSteps[0]) return at(0);
  int j = 0;
  while (steps > kSteps[j + 1]) ++j;
  const double u = (1 / steps - 1 / kSteps[j]) / (1 / kSteps[j + 1] - 1 / kSteps[j]);
  return at(j) * (1 - u) + at(j + 1) * u;
}

void SingingArcModel::resetAir() {
  Tube::resetAir();
  notes_.clear();
  side_.clear();
  std::memset(z1_, 0, sizeof(z1_));
}

const float* SingingArcModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

// Each arc's step: its tank tuned (bent) so that the arc sings on its pitch.
void SingingArcModel::tune(Note& n) {
  const double fos = instrument().fs * kOver;
  for (Arc& a : n.arc) {
    const double f = std::min(n.f0 * a.ratio * bk_, 0.2 * fos);  // (the arc's pitch)
    double rho = tuneRatio(n.phi, fos / f);
    rho = tuneRatio(n.phi, fos * rho / f);  // (the tank's steps a period, then its pitch again)
    a.h = kTwoPi * f / (rho * fos);
    a.ek = std::exp(-a.h / kTau);
  }
}

void SingingArcModel::setBend(double semitones) {
  if (semitones == bend_) return;
  bend_ = semitones;
  bk_ = std::exp2(semitones / 12);
  for (Note& n : notes_) tune(n);
}

void SingingArcModel::keyOn(int midi, int velocity) {
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  for (Note& n : notes_)
    if (n.midi == midi && n.down) keyOff(midi);
  if (int(notes_.size()) >= kMaxNotes) {  // the oldest released note, else the oldest, gives way
    auto it = std::find_if(notes_.begin(), notes_.end(), [](const Note& n) { return !n.down; });
    notes_.erase(it != notes_.end() ? it : notes_.begin());
  }
  Note n;
  n.midi = midi;
  n.f0 = 440 * std::exp2((midi - 69) / 12.0);
  // The key sets the arc's length (the column's share of the voltage: a harder key, a longer
  // arc; K1 the stroke) and, by its rheostat, its current.
  n.phi = std::clamp(0.30 + 0.1 * v + 0.3 * (length_ - 0.5), 0.28, kPhiHi);
  n.amp = 0.04 + 0.96 * fastPow(v, 1.6);
  n.travel = 0.0015 - 0.0005 * v;  // (the slide-wire's travel: a harder key, quicker)
  n.rise = 0.007 - 0.004 * v;      // (the rheostat's: the current comes up)
  // The two arcs: a few cents apart (K2: 0 .. 12 c), panned apart about the key's place; the
  // second is smaller (its sound 0.35 of the first's: the pair beats by up to ~6 dB, not to
  // silence), on the left or the right by the key.
  const double d = 12 * chorus_ * (0.6 + 0.4 * urand());
  const double centre = std::clamp((midi - 64) / 40.0, -0.4, 0.4);
  const double side = (midi & 1) ? 0.3 : -0.3;
  for (int k = 0; k < kArcs; ++k) {
    Arc& a = n.arc[k];
    startArc(a, n.phi);
    a.ratio = std::exp2((k ? (side > 0 ? d : -d) : 0.0) / 1200);  // (the large arc on the note)
    a.pan = std::clamp(centre + (k ? -side : side), -1.0, 1.0);
    a.level = k ? 0.35 : 1.0;
  }
  tune(n);
  notes_.push_back(n);
}

void SingingArcModel::keyOff(int midi) {
  for (Note& n : notes_)
    if (n.midi == midi && n.down) n.down = false;
}

float SingingArcModel::sample() {
  const double fs = instrument().fs, h = 1 / fs;
  // The current's run-down at the release (K4: 0.5 .. 8 s to -60 dB).
  const double fall = std::exp(-6.908 / (0.5 * fastPow(16.0, release_) * fs));
  // The plasma's flicker: noise low-passed at ~3 kHz, a relative swing of the column's loss
  // (K3: 0 .. 4 % RMS, as its square; 1 % at the middle).
  const double kf = 1 - std::exp(-kTwoPi * 3000 / fs), sig = 0.04 * hiss_ * hiss_ * std::sqrt(3 * (2 - kf) / kf);
  double mid[kOver] = {}, side[kOver] = {};
  for (Note& n : notes_) {
    if (!n.down) n.fade *= fall;
    const double ru = std::min(1.0, n.t / n.rise);
    const double amp = n.amp * n.fade * (0.5 - 0.5 * std::cos(M_PI * ru));
    // The key's slide-wire hands the arc's voltage to the tank as the key goes down (a raised
    // cosine over its travel, at least a radian and a half of the tank): the tank, struck by
    // it, rings at once.
    const double th = std::min(1.0, n.t / std::max(n.travel, 1.5 * 0.62 / (kTwoPi * n.f0)));
    const double sw = 0.5 - 0.5 * std::cos(M_PI * th);
    for (Arc& a : n.arc) {
      a.flick += kf * (sig * (2 * urand() - 1) - a.flick);
      for (int k = 0; k < kOver; ++k) {
        const double o = amp * a.level * arcStep(a, n.phi, a.flick, sw, a.ek);
        mid[k] += o;
        side[k] += a.pan * o;
      }
    }
    n.t += h;
    n.quiet = n.fade < 1e-4 ? n.quiet + 1 : 0;
  }
  notes_.erase(std::remove_if(notes_.begin(), notes_.end(), [](const Note& n) { return n.quiet > 256; }), notes_.end());
  // Back down to the sample rate: the 8th-order elliptic low-pass (0.1 dB to 19 kHz, -80 dB
  // from 26 kHz at 4 x 48 kHz), then every fourth sample.
  double y[2] = {};
  for (int k = 0; k < kOver; ++k) {
    double x[2] = {mid[k], side[k]};
    for (int c = 0; c < 2; ++c)
      for (int s = 0; s < oversample::kSections; ++s) {
        const double* b = oversample::kSos[s];
        double* z = z1_[c][s];
        const double o = b[0] * x[c] + z[0];
        z[0] = b[1] * x[c] - b[3] * o + z[1];
        z[1] = b[2] * x[c] - b[4] * o;
        x[c] = o;
      }
    y[0] = x[0];
    y[1] = x[1];
  }
  side_.push_back(float(outScale() * kLevel * y[1]));
  last_ = float(kLevel * y[0]);
  return finish(last_);
}

// --- The player.

SingingArcPlayer::SingingArcPlayer(SingingArcModel& m) : Performer(m), m_(m) {}

void SingingArcPlayer::noteOn(int midi, int velocity) {
  if (midi < 0 || midi > 127) return;
  articulate(midi, velocity, false);
  keyDown_[midi] = sounding_[midi] = true;
  note_ = midi;
  blowing_ = true;
  // (the director's accent: a harder touch)
  m_.keyOn(midi, std::clamp(int(std::lround(velocity * hardness_)), 1, 127));
}

void SingingArcPlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  keyDown_[midi] = false;
  if (!hold_) {
    m_.keyOff(midi);
    sounding_[midi] = false;
  }
}

void SingingArcPlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int k = 0; k < 128; ++k)
      if (sounding_[k] && !keyDown_[k]) {
        m_.keyOff(k);
        sounding_[k] = false;
      }
}

bool SingingArcPlayer::setKnob(int k, double x) {
  x = std::clamp(x, 0.0, 1.0);
  switch (k) {
    case 0: m_.setLength(x); return true;   // K1: the arc's length (the brightness)
    case 1: m_.setChorus(x); return true;   // K2: the chorus
    case 2: m_.setHiss(x); return true;     // K3: the hiss
    case 3: m_.setRelease(x); return true;  // K4: the release
    default: return false;
  }
}

void SingingArcPlayer::render(float* out, int n) {
  constexpr int kBlock = 64;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.setBend(2.0 * roll_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.asleep();
}

}  // namespace pajupilli
