// One organ reed pipe (research/church_organ.md §2.2): a brass tongue beating
// against the shallot's opening, closed by the wind's pressure in the boot,
// the resonator above it reinforcing it.
//
//   - The tongue: a damped mass-spring, tuned near the pipe's pitch (an organ
//     reed's tongue is heavy and stiff: it sets the pitch, the resonator
//     pulls it a little; the tuning wire sets both together here). The
//     pressure across it (the boot's against the shallot's) closes it; the
//     gap h = max(0, 1 - y), the tongue's travel past the shallot stopped
//     (it beats: its impact the reed's brassy edge).
//   - The flow through the gap: ~ h sqrt(|dp|), into the resonator.
//   - The resonator: its input impedance as a sum of band-pass modes (a cone
//     - Trompete, Oboe, Fagott, Posaune - every harmonic; a cylinder half as
//     long - Cromorne - the odd ones), their pressure at the shallot acting on
//     the tongue.
//   - The sound: the resonator's open top, its flow's rate of change (the
//     bell a high-pass: the reed's upper harmonics strong).
// Run 2x oversampled (the tongue's impact and the loop without delay are
// coarse at 48 kHz for a bright reed). Same interface as FluePipe.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "rng.h"

namespace pajupilli {

struct ReedVoicing {
  bool cylinder = false;    // a Cromorne's half-length cylinder: odd modes only
  int maxModes = 16;
  double tongueRatio = 1.03;  // the tongue's own frequency against the pitch
  double tongueQ = 12;      // its damping
  double pressure = 1.3;    // the wind's pressure against the tongue's closing pressure (beating above ~1)
  double zeta = 0.35;       // the gap's flow against the resonator's impedance (the coupling)
  double q1 = 25, qRise = 0.3, qCap = 60;
  double modeTilt = 0.3;    // mode k's impedance ~ k^-modeTilt (a cone's peaks fall slowly)
  double bright = 1.0;      // the bell's radiation: mode k ~ k^bright in the sound
  double rollHz = 9000;     // the resonator's top: the highs above it lost
  double formantHz = 0;     // the resonator's radiation concentrated around a band (its flared or capped
  double formantGain = 0;   // top: the trumpet's ~1.5 kHz, the oboe's ~1.3: harmonics there louder than the
  double formantOct = 1;    // fundamental, whatever the pitch), its gain (x) and width (octaves)
  double valve = 0.02, footRise = 0.008, release = 0.035;
  double noise = 0.002;     // the air's turbulence through the gap
  double viscous = 0.1;     // the pressure difference below which the gap's flow turns viscous (linear)
  double level = 1.0;       // its output (the voicer's regulation along the compass: the model's flow is
                            // per unit of the pipe's scale, a treble pipe's shallot and resonator smaller)
};

class ReedPipe {
 public:
  static constexpr int kOs = 2;
  void setup(double fs, double hz, const ReedVoicing& v, uint64_t seed) {
    fs_ = fs * kOs;
    v_ = v;
    hz_ = hz;
    rng_.reseed(seed);
    k_.clear();
    for (int k = 1; int(k_.size()) < v.maxModes; k += v.cylinder ? 2 : 1) {
      if (hz * k > std::min(0.4 * fs, v.rollHz * 1.5)) break;
      k_.push_back(k);
    }
    const int n = int(k_.size());
    a1_.assign(n, 0);
    a2_.assign(n, 0);
    b0_.assign(n, 0);
    z_.assign(n, 0);
    w_.assign(n, 0);
    y1_.assign(n, 0);
    y2_.assign(n, 0);
    tune(1.0);
    reset();
  }
  void tune(double ratio) {
    ratio_ = ratio;
    const double f = hz_ * ratio;
    for (size_t i = 0; i < k_.size(); ++i) {
      const int k = k_[i];
      const double fk = f * k * (1 + 0.002 * (k - 1));
      const double q = std::min(v_.qCap, v_.q1 * std::pow(double(k), v_.qRise));
      const double r = std::exp(-M_PI * fk / (q * fs_)), w = 2 * M_PI * fk / fs_;
      a1_[i] = float(2 * r * std::cos(w));
      a2_[i] = float(-r * r);
      b0_[i] = float((1 - r * r) / 2);  // (a constant-peak band-pass: its gain 1 at its centre)
      z_[i] = float(v_.zeta * std::pow(double(k), -v_.modeTilt));
      const double fo = v_.formantHz > 0 ? std::log2(fk / v_.formantHz) / v_.formantOct : 0;
      const double formant = 1 + v_.formantGain * std::exp(-fo * fo);
      w_[i] = float(std::pow(double(k), v_.bright) * (fk > v_.rollHz ? std::pow(v_.rollHz / fk, 2) : 1.0) * formant);
    }
    // (the topmost modes' radiation faded out, a raised cosine over the last
    // six: cut off hard, the last one rang the harmonic above it, a narrow
    // spike 6-12 dB over its neighbours at ~17 x the pitch: the Posaune's
    // pipes from B1 up penetrating, where it fell in 1-3 kHz)
    const int nk = int(k_.size()), taper = std::min(6, nk - 1);
    for (int j = 0; j < taper; ++j) w_[nk - 1 - j] *= float(0.5 - 0.5 * std::cos(M_PI * (j + 0.5) / (taper + 0.5)));
    inst_ = 0;
    for (size_t i = 0; i < k_.size(); ++i) inst_ += z_[i] * b0_[i];
    const double wt = 2 * M_PI * f * v_.tongueRatio / fs_;
    wt2_ = float(wt * wt);
    damp_ = float(wt / v_.tongueQ);
  }
  double ratio() const { return ratio_; }
  void open(double secs = 0) {
    on_ = true;
    awake_ = true;
    moveTime_ = secs > 0 ? secs : v_.valve;
  }
  void close(double secs = 0) {
    on_ = false;
    moveTime_ = secs > 0 ? secs : v_.release;
  }
  bool awake() const { return awake_; }
  bool openPipe() const { return false; }
  float top() const { return 0; }
  float foot() const { return foot_; }
  double hz() const { return hz_; }
  // One sample at the output rate: the chest's wind, the room's pressure at
  // the resonator's top; out: the sound at 1 m.
  float sample(float wind, float room = 0) {
    if (!awake_) return 0;
    float out = 0;
    for (int s = 0; s < kOs; ++s) out += step(wind, room);
    out *= float(v_.level) / kOs;
    if (!on_ && foot_ < 1e-4f && std::fabs(out) < 1e-6f) {
      if (++quiet_ > int(0.05 * fs_)) { awake_ = false; reset(); }
    } else {
      quiet_ = 0;
    }
    return out;
  }
  void reset() {
    std::fill(y1_.begin(), y1_.end(), 0.0f);
    std::fill(y2_.begin(), y2_.end(), 0.0f);
    valvePos_ = foot_ = y_ = yv_ = u1_ = u2_ = pr_ = prev_ = lp_ = 0;
  }

 private:
  float step(float wind, float room) {
    valvePos_ += float(((on_ ? 1.0 : 0.0) - valvePos_) / (moveTime_ * fs_));
    valvePos_ = std::clamp(valvePos_, 0.0f, 1.0f);
    foot_ += float((valvePos_ * wind - foot_) / (v_.footRise * fs_));
    const float pf = std::max(foot_, 0.0f) * float(v_.pressure);
    // The tongue: closed by the pressure across it (semi-implicit Euler).
    const float dp = pf - pr_;
    yv_ += wt2_ * (dp - y_) - damp_ * yv_;
    y_ += yv_;
    if (y_ > 1) {  // it beats on the shallot: stopped, a little bounce
      y_ = 1;
      yv_ = -0.2f * yv_;
    } else if (y_ < -0.5f) {  // (pushed open by the resonator's pressure: its curve stops it, the gap at
      y_ = -0.5f;             // most 1.5 x its rest; unbounded, the wider gap's flow fed the resonator
      yv_ = 0;                // without limit as the wind fell at the key's release)
    }
    const float h = 1 - y_;
    float white, n2;
    rng_.pair(white, n2);
    // The resonator's pressure at the shallot: its modes' past (all but this
    // sample's flow) plus the flow times their instantaneous impedance; the
    // flow through the gap depends on it: solved together (Newton: the loop
    // without a sample's delay, which at a low reed's many modes ran away).
    // The flow: Bernoulli's sqrt for a real pressure difference, viscous
    // (linear) near none.
    const int K = int(k_.size());
    float rest = -inst_ * u2_;
    for (int i = 0; i < K; ++i) rest += z_[i] * (a1_[i] * y1_[i] + a2_[i] * y2_[i]);
    const float inst = inst_;
    rest += 0.02f * room;
    const float ev = float(v_.viscous), gain = h * (1 + float(v_.noise) * white);
    float u = u1_;
    for (int it = 0; it < 2; ++it) {  // (from the last sample's flow: two steps converge)
      const float d = pf - rest - inst * u;
      const float sq = std::sqrt(std::fabs(d) + ev);
      const float g = d / sq;                                   // the flow law
      const float dg = (std::fabs(d) / 2 + ev) / (sq * sq * sq);  // its slope in d
      const float f = u - gain * g, df = 1 + gain * dg * inst;
      u -= f / df;
    }
    float pr = 0, rad = 0;
    for (int i = 0; i < K; ++i) {
      const float y = b0_[i] * (u - u2_) + a1_[i] * y1_[i] + a2_[i] * y2_[i];
      y2_[i] = y1_[i];
      y1_[i] = y;
      pr += z_[i] * y;
      rad += w_[i] * z_[i] * y;
    }
    u2_ = u1_;
    u1_ = u;
    pr_ = pr + 0.02f * room;
    // The top: its flow's rate of change, the very highs softened.
    const float d = (rad - prev_) * float(fs_) * 1e-3f;
    prev_ = rad;
    lp_ += 0.6f * (d - lp_);
    return lp_;
  }
  double fs_ = 96000, hz_ = 440, ratio_ = 1;
  ReedVoicing v_;
  Rng rng_;
  std::vector<int> k_;
  std::vector<float> a1_, a2_, b0_, z_, w_, y1_, y2_;
  float wt2_ = 0, damp_ = 0, inst_ = 0;
  double moveTime_ = 0.01;
  float valvePos_ = 0, foot_ = 0, y_ = 0, yv_ = 0, u1_ = 0, u2_ = 0, pr_ = 0, prev_ = 0, lp_ = 0;
  int quiet_ = 0;
  bool on_ = false, awake_ = false;
};

}  // namespace pajupilli
