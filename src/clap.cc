#include "clap.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "instrument.h"
#include "knob.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

bool handClap(const Instrument& inst) { return std::strcmp(inst.name, "hand-clap") == 0; }

ClapModel::ClapModel(const Instrument& inst) : Tube(inst) {
  gain_ = knob("PJ_CLAP_GAIN", 10.0);  // (by its peak: ~0.6 at velocity 100; a clap is short, its loudness over a second low)
  hits_.reserve(32);
}

double ClapModel::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

double ClapModel::un() {  // about normal: the sum of four uniforms
  return (ur() + ur() + ur() + ur() - 2) * 1.7320508;
}

void ClapModel::clap(double speed) {
  static const int people = int(knob("PJ_CLAP_PEOPLE", 4));
  static const double spread = knob("PJ_CLAP_SPREAD", 0.020);
  const double fs = fs_;
  // (Hits long done make room.)
  hits_.erase(std::remove_if(hits_.begin(), hits_.end(), [](const Hit& h) { return !h.on; }), hits_.end());
  for (int p = 0; p < people; ++p) {
    // Each person: their own hands (the cavity's pitch, how well damped),
    // how hard (the first is the loudest), when (spread over ~20 ms), where.
    const double cup = ur();                                  // 0 flat .. 1 cupped
    const double f = 2200 * std::pow(0.33, cup) * (1 + 0.06 * un());  // 2.2 kHz flat .. 0.75 kHz cupped
    const double Q = 3 + 3 * ur();
    const double v = speed * (p == 0 ? 1.0 : 0.55 + 0.4 * ur());
    const int at = p == 0 ? 0 : int(spread * fs * ur());
    const double pan = 0.8 * (ur() - 0.5);
    // Two contacts: the fingers (weaker, the cavity not yet shut), then the
    // palms 1-3 ms later.
    for (int c = 0; c < 2; ++c) {
      Hit h;
      h.on = true;
      h.wait = at + (c == 0 ? 0 : int((0.001 + 0.002 * ur()) * fs));
      // The squeeze: the hands closing the last few mm at v (0.5 .. 1.5 ms;
      // a faster clap is quicker).
      const double tau = std::clamp(1.2e-3 * std::sqrt(1.5 / std::max(v, 0.2)), 0.4e-3, 2.0e-3) * (0.8 + 0.4 * ur());
      h.len = std::max(4, int(tau * fs));
      // The volume pushed out: ~ the speed (the cavity's few cm^3 in the
      // closing time); its pressure at the listener ~ its rate.
      h.amp = v * (c == 0 ? 0.35 + 0.3 * ur() : 1.0) / tau;
      const double fc = f * (c == 0 ? 1.25 : 1.0);        // (the fingers' pocket a little smaller)
      h.g = std::tan(kPi * std::min(fc, 0.45 * fs) / fs);
      h.k = 1 / Q;
      // The hiss: the air jetting out of the gaps (2 - 6 kHz, broad).
      const double fn = 2500 + 3000 * ur();
      h.ng = std::tan(kPi * fn / fs);
      h.nk = 1.4;
      h.noise = 0.5 + 0.4 * ur();
      // The palms' thud: 100 - 300 Hz, gone in a few ms (the soft tissue).
      const double ft = 100 + 200 * ur();
      h.thudW = 2 * kPi * ft / fs;
      h.thudR = std::exp(-1 / (0.004 * fs));
      h.thud = (c == 1 ? 0.25 : 0.08) * (0.6 + 0.8 * ur());
      h.pan = pan;
      hits_.push_back(h);
    }
  }
}

float ClapModel::sample() {
  double y = 0, s = 0;
  for (Hit& h : hits_) {
    if (!h.on) continue;
    if (h.wait > 0) { --h.wait; continue; }
    // The squeeze: a half-sine of volume flow out of the cavity, into the
    // resonator (its band-pass: the neck's flow).
    double in = 0;
    if (h.t < h.len) {
      in = h.amp * std::sin(kPi * (h.t + 0.5) / h.len);
      // The hiss, while the air rushes out.
    }
    const double a1 = 1 / (1 + h.g * (h.g + h.k)), a2 = h.g * a1, a3 = h.g * a2;
    const double v3 = in - h.ic2, v1 = a1 * h.ic1 + a2 * v3, v2 = h.ic2 + a2 * h.ic1 + a3 * v3;
    h.ic1 = 2 * v1 - h.ic1;
    h.ic2 = 2 * v2 - h.ic2;
    const double flow = v1 * h.k;  // (the band-pass, unit gain at its peak)
    // The sound: the flow's rate (a small source in the open: p ~ dq/dt).
    double out = (flow - h.prev) * 1e-3;
    h.prev = flow;
    if (h.t < 2 * h.len) {
      const double env = std::sin(kPi * std::min(1.0, (h.t + 0.5) / (2.0 * h.len)));
      const double w = un() * env * h.noise * h.amp * 5e-5;
      const double b1 = 1 / (1 + h.ng * (h.ng + h.nk)), b2 = h.ng * b1, b3 = h.ng * b2;
      const double u3 = w - h.n2, u1 = b1 * h.n1 + b2 * u3, u2 = h.n2 + b2 * h.n1 + b3 * u3;
      h.n1 = 2 * u1 - h.n1;
      h.n2 = 2 * u2 - h.n2;
      out += u1 * h.nk;
    }
    // The thud: a decaying sine set going by the squeeze.
    if (h.t == 0) h.tim += h.thud * h.amp * 1e-4;
    {
      const double c = std::cos(h.thudW) * h.thudR, sn = std::sin(h.thudW) * h.thudR;
      const double re = h.tre * c - h.tim * sn, im = h.tre * sn + h.tim * c;
      h.tre = re;
      h.tim = im;
      out += im;
    }
    ++h.t;
    if (h.t > 3 * h.len + int(0.06 * fs_)) h.on = false;
    y += out;
    s += out * h.pan;
  }
  const float g = float(gain_);
  last_ = g * float(y);
  sd_ = g * float(s);
  side_.push_back(float(outScale()) * sd_);
  return finish(last_);
}

void ClapModel::resetAir() {
  Tube::resetAir();
  hits_.clear();
}

const float* ClapModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

ClapPlayer::ClapPlayer(ClapModel& m) : Performer(m), m_(m) {}

double ClapPlayer::speed(int velocity, double dynamics) {
  // The hands' closing speed: ~ velocity^1.4 (the clap's level ~ its speed
  // over the squeeze's time, ~ speed^1.5: about General MIDI's 30 dB, 16 to 127).
  static const double top = knob("PJ_CLAP_VMAX", 3.0), ex = knob("PJ_CLAP_VEXP", 1.4);
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  return top * std::pow(v, ex) * (0.75 + 0.5 * std::clamp(dynamics, 0.0, 1.0));
}

void ClapPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  m_.clap(speed(velocity, dyn_));
}

void ClapPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
}

}  // namespace pajupilli
