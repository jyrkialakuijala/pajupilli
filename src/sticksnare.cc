#include "sticksnare.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "knob.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRho = 1.2;
}  // namespace

bool stickSnare(const Instrument& inst) { return std::strcmp(inst.name, "stick-snare") == 0; }

StickSnare::StickSnare(double fs) : snare_(fs), fs_(fs), dt_(1 / fs) {
  R_ = snare_.batter().radius();
  K_ = knob("PJ_SSNARE_K", 2e8);  // hickory on the coated film (N/m^1.5), as the toms'
  for (Stick& s : st_) s.phi.assign(snare_.batter().modes(), 0.0f);
  auto make = [&](double f, double t60, double g, double pan) {
    const double w = 2 * kPi * f, a = 6.91 / t60, gd = std::exp(-a * dt_), c = std::cos(w * dt_), sn = std::sin(w * dt_);
    Ring r;
    r.a11 = float(gd * c); r.a12 = float(gd * sn / w); r.a21 = float(-gd * w * sn); r.a22 = float(gd * c);
    r.gain = float(g);
    r.pan = float(pan);
    return r;
  };
  // The stick, free at both ends (40 cm hickory): 324, 892, 1749 Hz, the
  // hand on it (dead in 40 .. 30 ms).
  const double sf[3] = {324, 892, 1749}, st[3] = {0.040, 0.035, 0.030}, sg[3] = {0.5, 1.0, 0.8};
  for (int i = 0; i < 3; ++i) stickRing_.push_back(make(sf[i], st[i], sg[i], 0.1));
  // The hoop, a steel ring bending out of its plane (n = 2 .. 6), held by
  // the lugs and the head.
  // n (n^2 - 1) / sqrt(n^2 + 1) x 137 Hz for n = 2 .. 10: the steel's crack
  // reaches far up (the shaft on the hoop: a contact of a few tens of us).
  const double hf[9] = {368, 1040, 1995, 3226, 4730, 6512, 8566, 10887, 13496},
               ht[9] = {0.060, 0.055, 0.045, 0.035, 0.025, 0.020, 0.016, 0.012, 0.010},
               hg[9] = {0.35, 0.8, 1.0, 0.9, 0.8, 0.7, 0.6, 0.5, 0.4};
  for (int i = 0; i < 9; ++i) hoop_.push_back(make(hf[i], ht[i], hg[i], i % 2 ? 0.25 : -0.25));
}

void StickSnare::stick(int hand, double speed, double r, double theta, double mass) {
  static const double kMass = knob("PJ_SSNARE_MASS", 0.6);  // (x every stroke's mass: 9 g at the tip)
  mass *= kMass;
  Stick& s = st_[hand & 1];
  Membrane& b = snare_.batter();
  b.shape(std::min(r, 0.95) * R_, theta, s.phi.data());
  double g = 0;
  for (int k = 0; k < b.modes(); ++k) g += double(s.phi[k]) * s.phi[k];
  s.G = dt_ * dt_ * g;  // (unit-mass modes: one sample's give per unit force)
  // Thrown from just over the head where it is now: the stroke meets its swing.
  s.z = b.height(s.phi.data()) + 0.05e-3;
  s.v = -std::fabs(speed);
  s.m = mass;
  s.on = true;
  s.age = 0;
  snare_.wake();
  quiet_ = 0;
}

void StickSnare::rimShot(int hand, double speed, double theta) {
  // The tip 2-3 cm inside the rim, with more of the stick behind it (the
  // shaft is stopped by the hoop: the wrist's weight in it), and the hoop
  // and the stick struck at once by the shaft.
  static const double kHoop = knob("PJ_SSNARE_HOOP", 10.0);
  stick(hand, speed, 0.86, theta, 0.03);
  // Each mode set going at its own phase (the shaft lands along the hoop, not
  // at one point: the modes do not all start together, and the crack has no
  // needle of a peak), the same energy as a single kick.
  auto kick = [&](Ring& r, double a) {
    rng_ = rng_ * 1664525u + 1013904223u;
    const double ph = 2 * kPi * ((rng_ >> 8) * (1.0 / 16777216.0));
    const double w = std::sqrt(std::max(1e-9, -double(r.a21) / std::max(1e-9, double(r.a12))));  // its pulsatance
    r.v += float(a * std::cos(ph));
    r.x += float(a * std::sin(ph) / w);
  };
  for (Ring& h : hoop_) kick(h, kHoop * speed);
  for (Ring& s : stickRing_) kick(s, kHoop * 0.6 * speed);
}

void StickSnare::reset() {
  snare_.reset();
  for (Stick& s : st_) s.on = false;
  for (Ring& r : stickRing_) r.x = r.v = 0;
  for (Ring& r : hoop_) r.x = r.v = 0;
  hpC_ = lastC_ = 0;
  quiet_ = 0;
}

bool StickSnare::asleep() const { return snare_.asleep() && !st_[0].on && !st_[1].on && quiet_ > int(0.3 * fs_); }

float StickSnare::sample() {
  Membrane& b = snare_.batter();
  // The sticks: each tip against the batter where it is, the force found for
  // the end of the sample as the head gives under it (as the hi-hat's and
  // the toms'); pushed into the head before the drum's own sample takes it.
  double F = 0;
  for (Stick& s : st_) {
    if (!s.on) continue;
    const double h = b.height(s.phi.data()), hv = b.speed(s.phi.data());
    const double free = (h - s.z) + dt_ * (hv - s.v), G = s.G + dt_ * dt_ / s.m;
    double f = 0;
    if (free > 0) {
      double lo = 0, hi = free;  // the dent left once the force has pushed both apart
      for (int it = 0; it < 30; ++it) {
        const double x = 0.5 * (lo + hi);
        if (x + G * K_ * x * std::sqrt(x) > free) hi = x; else lo = x;
      }
      const double x = 0.5 * (lo + hi);
      f = K_ * x * std::sqrt(x);
    }
    s.v += dt_ * f / s.m;
    s.z += dt_ * s.v;
    if (f > 0) b.push(s.phi.data(), float(-f));
    F += f;
    // Off the head and on its way up (or after 30 ms in any case): the
    // stroke is over; the hand takes the stick away.
    if (++s.age > int(0.03 * fs_) || (f == 0 && s.v > 0)) s.on = false;
    snare_.wake();
  }
  lastF_ = F;
  static FILE* trace = std::getenv("PJ_SSNARE_TRACE") ? std::fopen(std::getenv("PJ_SSNARE_TRACE"), "wb") : nullptr;
  if (trace) { const float ff = float(F); std::fwrite(&ff, 4, 1, trace); }
  float y = snare_.sample();
  float sd = snare_.side();
  // The stick's own ring, driven by the contact force (N -> its swing).
  static const float kRing = float(knob("PJ_SSNARE_RING", 1.0));
  const float push = float(F * dt_ * 8.0);
  float rn = 0, rnSide = 0;
  for (Ring& r : stickRing_) {
    r.v += push;
    const float x = r.a11 * r.x + r.a12 * r.v, v = r.a21 * r.x + r.a22 * r.v;
    r.x = x; r.v = v;
    rn += kRing * 0.3f * r.gain * v;
  }
  for (Ring& r : hoop_) {
    const float x = r.a11 * r.x + r.a12 * r.v, v = r.a21 * r.x + r.a22 * r.v;
    r.x = x; r.v = v;
    rn += 0.3f * r.gain * v;
    rnSide += 0.3f * r.gain * r.pan * v;
  }
  // The contact's click above the batter's top: a force on a light film
  // moves the air at once (rho F / (2 pi sigma r)), heard at the microphones.
  static const float kClick = float(knob("PJ_SSNARE_CLICK", 1.0));
  const double top = 4500;
  const float aC = float(std::exp(-2 * kPi * top * dt_));
  hpC_ = aC * (hpC_ + float(F) - lastC_);
  lastC_ = float(F);
  const float click = kClick * float(kRho / (2 * kPi * 0.26 * 0.5)) * hpC_;
  y += rn + click;
  side_ = sd + rnSide;
  if (!std::isfinite(y)) { reset(); return 0; }
  if (std::fabs(y) < 1e-6f && !touching()) ++quiet_; else quiet_ = 0;
  return y;
}

StickSnareModel::StickSnareModel(const Instrument& inst) : Tube(inst), d_(inst.fs) {}

void StickSnareModel::resetAir() {
  Tube::resetAir();
  d_.reset();
}

const float* StickSnareModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

float StickSnareModel::sample() {
  // (As the brushed snare's: the microphones' pressure, scaled.)
  static const float g = float(knob("PJ_SSNARE_OUT", 1.0 / 8));
  last_ = g * d_.sample();
  side_.push_back(g * float(outScale()) * d_.side());
  return finish(last_);
}

StickSnarePlayer::StickSnarePlayer(StickSnareModel& m) : Performer(m), m_(m) {}

double StickSnarePlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

double StickSnarePlayer::stickSpeed(int velocity, double dynamics) {
  // ~ velocity^1.75 (General MIDI's ~30 dB from 16 to 127; the drum gives a
  // little less than linear): ghost notes (velocity 20 .. 40) 0.1 .. 0.6
  // m/s, accents 3 .. 6.
  static const double top = knob("PJ_SSNARE_VMAX", 6.0), ex = knob("PJ_SSNARE_VEXP", 1.75);
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  return top * std::pow(v, ex) * (0.75 + 0.5 * std::clamp(dynamics, 0.0, 1.0));
}

void StickSnarePlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  StickSnare& d = m_.drum();
  const double speed = stickSpeed(velocity, dyn_) * (1 + 0.05 * (ur() - 0.5));
  // The hands in turn; each stroke a little elsewhere (the right hand's
  // side, the left's).
  hand_ ^= 1;
  const double th = (hand_ ? -0.5 : 0.4) + 0.4 * (ur() - 0.5);
  switch (midi) {
    case 36:
      rolling_ = true;
      rollVel_ = velocity;
      rollT_ = 0;
      nextStroke_ = 0;
      bounces_.clear();
      break;
    case 37:
      d.crossStick(speed);
      break;
    case 39:
    case 41:
      d.rimShot(hand_, std::min(1.25 * speed, 8.0), th);
      break;
    case 40:
      d.stick(hand_, speed, 0.5 + 0.06 * (ur() - 0.5), th);
      break;
    case 42:
      d.stick(hand_, speed, 0.75 + 0.06 * (ur() - 0.5), th);
      break;
    default:
      d.stick(hand_, speed, 0.25 + 0.12 * ur(), th);
      break;
  }
}

void StickSnarePlayer::noteOff(int midi) {
  if (midi == 36 && !hold_) rolling_ = false;
}

// The buzz roll: the hands in turn, 6 .. 9 strokes a second each (faster
// when louder); each stroke pressed into the head, bouncing 3 .. 5 times,
// each bounce ~0.75 x sooner and ~0.55 x weaker than the last.
void StickSnarePlayer::rollStep(double dt) {
  StickSnare& d = m_.drum();
  if (rolling_) {
    rollT_ += dt;
    if (rollT_ >= nextStroke_) {
      const double w = std::clamp(rollVel_ / 127.0, 0.0, 1.0);
      const double rate = 2 * (6 + 3 * w);  // both hands
      const int h = (hand_ ^= 1);
      const double v0 = 0.7 * stickSpeed(rollVel_, dyn_) * (1 + 0.08 * (ur() - 0.5));
      const int n = 3 + int(2 * w + ur());
      double t = 0, gap = (0.030 - 0.008 * w) * (1 + 0.1 * (ur() - 0.5)), v = v0;
      bounces_.push_back({rollT_, h, v});
      for (int i = 1; i <= n; ++i) {
        t += gap;
        v *= 0.55 * (1 + 0.1 * (ur() - 0.5));
        bounces_.push_back({rollT_ + t, h, v});
        gap *= 0.75;
      }
      nextStroke_ = rollT_ + (1 / rate) * (1 + 0.06 * (ur() - 0.5));
    }
  } else if (!bounces_.empty()) {
    rollT_ += dt;  // (the last stroke's bounces finish)
  }
  for (size_t i = 0; i < bounces_.size();) {
    if (bounces_[i].t <= rollT_) {
      const double th = (bounces_[i].hand ? -0.5 : 0.4) + 0.3 * (ur() - 0.5);
      d.stick(bounces_[i].hand, bounces_[i].speed, 0.28 + 0.08 * ur(), th);
      bounces_.erase(bounces_.begin() + long(i));
    } else {
      ++i;
    }
  }
}

void StickSnarePlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    rollStep(m / fs_);
    m_.process(out + i, m);
  }
  blowing_ = rolling_ || !bounces_.empty() || !m_.drum().asleep();
}

}  // namespace pajupilli
