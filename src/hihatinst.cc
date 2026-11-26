#include "hihatinst.h"

#include <cmath>
#include <cstring>

#include "knob.h"
#include "vmath.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

bool hiHat(const Instrument& inst) { return std::strcmp(inst.name, "hi-hat") == 0; }

HiHatModel::HiHatModel(const Instrument& inst) : Tube(inst), hat_(inst.fs) {}

void HiHatModel::resetAir() {
  Tube::resetAir();
  hat_.reset();
}

float HiHatModel::sample() {
  static const float g = float(knob("PJ_HAT_OUT", 1.0 / 3));
  // The highs 3 dB down, as the cymbals' (src/cymbal.cc: heard from a little further off).
  static const float kHi = float(fastPow(10.0, knob("PJ_CYM_HIGHS", -4.3) / 20) - 1);
  static const float kA = float(1 - std::exp(-2 * M_PI * 3000 / fs_));
  const float x = hat_.sample(), sd = hat_.side();
  shelfM_ += kA * (x - shelfM_);
  shelfS_ += kA * (sd - shelfS_);
  last_ = g * (x + kHi * (x - shelfM_));
  side_.push_back(g * float(outScale()) * (sd + kHi * (sd - shelfS_)));
  return finish(last_);
}

HiHatPlayer::HiHatPlayer(HiHatModel& m) : Performer(m), m_(m) {
  brush_ = knob("PJ_HAT_BRUSH", 0) > 0;   // strokes with a brush, not a stick
  flat_ = knob("PJ_HAT_FLAT", 0.35);      // the share of closings that land flat
}

double HiHatPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

void HiHatPlayer::lean() {
  const double tilt = hiHatSpec().tilt;
  if (ur() < flat_) {  // near the lower plate's own tilt: it will land flat
    leanX_ = tilt * (1 + 0.15 * (ur() - 0.5));
    leanY_ = tilt * 0.15 * (ur() - 0.5);
  } else {             // anywhere: one edge will meet first
    const double a = 2 * kPi * ur(), r = tilt * (0.4 + 1.0 * ur());
    leanX_ = r * std::cos(a);
    leanY_ = r * std::sin(a);
  }
}

void HiHatPlayer::stroke(int velocity) {
  HiHat& h = m_.hat();
  const double v = std::clamp(velocity, 1, 127) / 127.0, R = h.top().radius();
  const double speed = (0.25 + 4.5 * fastPow(v, 1.6)) * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0));
  const double th = 0.4 + 0.5 * (ur() - 0.5);
  if (brush_ || brushKey_) {
    const double r = (0.75 + 0.1 * (ur() - 0.5)) * R;
    h.brush().jump(r * std::cos(th), r * std::sin(th), th + 0.3);
    // (The fan comes down at a slant and is let through: its wire ends
    // arrive one after another over several milliseconds, a scatter of
    // small touches, not one blow.)
    h.brush().strike(0.6 * speed, (6 + 3 * ur()) * 1e-3, (4 + 2 * v) * 1e-3, 0.0);
    h.wake();
  } else if (velocity >= 100) {
    h.stick(0.8 * speed, R, th, true);                          // an accent: the shoulder on the edge
  } else {
    h.stick(speed, (0.68 + 0.12 * (ur() - 0.5)) * R, th, false);  // the tip on the bow
  }
}

void HiHatPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  HiHat& h = m_.hat();
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  // (The Brush kit's hi-hat: 43 the closed, 47 the open, struck with the brush; the foot's 44 is the same.)
  if (midi == 42 || midi == 46) brushKey_ = false;
  if (midi == 43 || midi == 47) { brushKey_ = true; --midi; }
  if (midi == 46) {
    if (foot_ != kOpen) { lean(); h.pedal(1, 0.5, closeForce_, leanX_, leanY_); }
    strokeIn_ = foot_ == kDown ? 0.004 : 0;  // (from closed: as the rims part)
    strokeVel_ = velocity;
    foot_ = kOpen;
    liftIn_ = closeIn_ = -1;
  } else if (midi == 42) {
    closeForce_ = 25 + 35 * v;
    if (foot_ != kDown) {
      const double gap = std::max(h.gap(), 0.0);
      h.pedal(0, 1.2, closeForce_, leanX_, leanY_);
      strokeIn_ = std::min(gap / 1.2, 0.008) + 0.002;
    } else {
      h.pedal(0, 1.2, closeForce_, leanX_, leanY_);
      strokeIn_ = 0;
    }
    strokeVel_ = velocity;
    foot_ = kDown;
    since42_ = 0;
    liftIn_ = closeIn_ = -1;
  } else if (midi == 44) {
    closeSpeed_ = 0.25 + 1.0 * v;
    closeForce_ = 20 + 60 * v;
    if (foot_ == kDown) {  // the foot was resting: up a little first, then down
      lean();
      h.pedal(0.5, 0.6, closeForce_, leanX_, leanY_);
      closeIn_ = 0.012;
    } else {
      h.pedal(0, closeSpeed_, closeForce_, leanX_, leanY_);
      closeIn_ = -1;
    }
    foot_ = kDown;
    strokeIn_ = -1;
    // Keeping time with the foot: it comes up again, ready for the next.
    liftIn_ = since42_ > 1.5 ? 0.18 : -1;
  }
}

void HiHatPlayer::control(double dt) {
  HiHat& h = m_.hat();
  since42_ += dt;
  if (closeIn_ >= 0 && (closeIn_ -= dt) < 0) h.pedal(0, closeSpeed_, closeForce_, leanX_, leanY_);
  if (strokeIn_ >= 0 && (strokeIn_ -= dt) < 0) stroke(strokeVel_);
  if (liftIn_ >= 0 && (liftIn_ -= dt) < 0 && foot_ == kDown) {
    lean();
    h.pedal(0.6, 0.08, closeForce_, leanX_, leanY_);
    foot_ = kReady;
  }
}

void HiHatPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    control(m / fs_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.hat().asleep();
}

}  // namespace pajupilli
