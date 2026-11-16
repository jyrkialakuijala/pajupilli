// One organ flue pipe (research/church_organ.md §2, §7c-d): an air jet across
// the mouth driving the air column's resonances, which in turn deflect the jet.
//
//   - The jet leaves the flue at a speed set by the foot's wind (v ~
//     sqrt(p)), crosses the mouth in tau (~ cut-up / v: a fraction of the
//     period) and is deflected by the acoustic flow there tau ago; the share
//     of it that goes into the pipe is a tanh of that deflection (it swings
//     in and out over the upper lip), its bias the lip's offset.
//   - The air column as its modes, each a damped rotation (the coupled form,
//     exact in float32 at 48 kHz: §7c): a stopped pipe (Gedackt, Bourdon,
//     Subbass) the odd ones only (its closed end: 1, 3, 5 ... x the
//     fundamental, the upper ones a little off), an open pipe all of them.
//     The jet's flow drives them; their sum is the mouth's acoustic flow.
//   - The pallet opens over a few ms (the key's travel), the foot's pressure
//     rises behind it, the jet's turbulence adds its breath; closed, the
//     wind falls and the column rings out.
//   - The sound: the mouth radiates (the flow's rate of change) and, for an
//     open pipe, its top too.
// float32, one pipe per object; a rank is many.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "rng.h"

namespace pajupilli {

struct PipeVoicing {
  bool stopped = true;      // a stopped pipe: odd modes only
  int maxModes = 12;        // modes kept (below 0.4 fs)
  double q1 = 30;           // the fundamental's Q (radiation and wall losses)
  double qRise = 0.6;       // Q ~ q1 * k^qRise, until the mouth's losses cap it
  double qCap = 120;
  double stretch = -0.0015; // each mode k off k x f by stretch x (k - 1) (end correction)
  // The loop (scanned: research/church_organ.md §8 step 2): the jet's flow
  // drives the modes' real parts, their imaginary parts are the mouth's flow
  // that deflects it, inverted; the jet's travel a quarter period puts the
  // loop's phase on the fundamental (on pitch within ~2 c, before the
  // calibration). A higher gain speaks faster (65 Hz: 214 ms at 16, 344 at 8;
  // 262 Hz: 74 ms; 1 kHz: 24 ms) and a little brighter (the 3rd -20 dB).
  double tauFrac = 0.25;    // the jet's travel across the mouth, a share of the period
  double gain = 16.0;       // the jet's deflection per unit mouth flow
  double bias = 0.3;        // the lip's offset (asymmetry: the even harmonics, the drive)
  double drive = 1.0;       // the jet's flow into the column
  double noise = 0.01;      // the jet's turbulence (relative)
  double modeTilt = 1.0;    // mode k driven ~ k^-modeTilt (the jet's spectrum)
  double jetRad = 0.02;     // the jet's own flow radiated at the mouth (its asymmetric swing: the even
                            // harmonics), low-passed (4 poles) at jetLp x the pitch (0: not: its saturated,
                            // square-ish edge then radiates every harmonic, a buzz)
  double jetLp = 0;
  double breath = 0;        // the jet's turbulence radiated (a soft hiss above ~1 kHz), ~ its speed
  double puff = 0;          // the onset's puff: the wind rushing through the opening pallet and the jet
  double puffTime = 0.06;   // turbulent until the tone locks (its level, its decay in s)
  // A chimney (Rohrflöte, Rohrgedackt): the stopper's tube half-opens the
  // top, so the even modes speak too (weakly: evenLevel x their weight), and
  // its own resonance lifts the harmonics around chimneyK (by chimneyBoost x,
  // over +-chimneyWidth harmonics).
  double evenLevel = 0;
  double chimneyK = 5, chimneyBoost = 0, chimneyWidth = 1.5;
  double rollK = 0;         // and ~ exp(-((k - 1) / rollK)^2) on top (0: none): the upper harmonics'
                            // steep fall (a real Bourdon: the 5th ~-40, the 7th ~-60, the 9th ~-70 dB)
  double valve = 0.005;     // the pallet's opening (s)
  double footRise = 0.006;  // the foot's pressure rising behind it (s)
  double release = 0.015;   // the wind falling when it closes (s)
};

class FluePipe {
 public:
  void setup(double fs, double hz, const PipeVoicing& v, uint32_t seed) {
    fs_ = fs;
    v_ = v;
    hz_ = hz;
    rng_.reseed(seed);
    k_.clear();
    for (int k = 1; int(k_.size()) < v.maxModes; k += v.stopped && v.evenLevel <= 0 ? 2 : 1) {
      const double f = hz * k * (1 + v.stretch * (k - 1));
      if (f > 0.4 * fs) break;
      k_.push_back(k);
    }
    const int n = int(k_.size());
    re_.assign(n, 0.0f);
    im_.assign(n, 0.0f);
    c_.resize(n);
    s_.resize(n);
    in_.resize(n);
    ratio_ = 1;
    tune(1.0);
    // The jet's delay line: the mouth flow's history.
    int len = 1;
    while (len < int(v.tauFrac * fs / hz * 2.5) + 8) len <<= 1;
    hist_.assign(len, 0.0f);
    mask_ = len - 1;
    w_ = 0;
  }
  // The speaking pitch's ratio to the design (a calibration's correction, a
  // just tuning's retune, the temperature): every mode turned by it.
  void tune(double ratio) {
    ratio_ = ratio;
    for (size_t i = 0; i < k_.size(); ++i) {
      const int k = k_[i];
      const double f = hz_ * ratio * k * (1 + v_.stretch * (k - 1));
      const double q = std::min(v_.qCap, v_.q1 * std::pow(double(k), v_.qRise));
      const double r = std::exp(-M_PI * f / (q * fs_)), w = 2 * M_PI * f / fs_;
      c_[i] = float(r * std::cos(w));
      s_[i] = float(r * std::sin(w));
      const double roll = v_.rollK > 0 ? std::exp(-((k - 1) / v_.rollK) * ((k - 1) / v_.rollK)) : 1.0;
      const double even = v_.stopped && k % 2 == 0 ? v_.evenLevel : 1.0;
      const double cz = (k - v_.chimneyK) / v_.chimneyWidth;
      const double chimney = 1 + v_.chimneyBoost * std::exp(-cz * cz);
      in_[i] = float((1 - r) * std::pow(double(k), -v_.modeTilt) * roll * even * chimney);  // (each mode's resonance gain ~ its weight)
    }
    tau_ = v_.tauFrac * fs_ / (hz_ * ratio);
    puffDecay_ = float(std::exp(-1 / (v_.puffTime * fs_)));
    jetA_ = v_.jetLp > 0 ? float(1 - std::exp(-2 * M_PI * std::min(v_.jetLp * hz_ * ratio, 0.4 * fs_) / fs_)) : 1.0f;
  }
  double ratio() const { return ratio_; }
  void open() {
    if (!on_) puffEnv_ = 1;
    on_ = true;
    awake_ = true;
  }
  void close() { on_ = false; }
  bool awake() const { return awake_; }
  // One sample: the wind's pressure at the chest (1 = the voicing's), the
  // room's pressure at the mouth (acting on the jet), out: the sound at 1 m.
  float sample(float wind, float room = 0) {
    if (!awake_) return 0;
    // The pallet and the foot: the key opens the valve over valve s; the
    // foot's pressure follows it (footRise); closed, it falls (release).
    valvePos_ += float(((on_ ? 1.0 : 0.0) - valvePos_) * (on_ ? 1 / (v_.valve * fs_) : 1 / (v_.release * fs_)));
    valvePos_ = std::clamp(valvePos_, 0.0f, 1.0f);
    foot_ += float((valvePos_ * wind - foot_) / (v_.footRise * fs_));
    const float p = std::max(foot_, 0.0f);
    const float v = std::sqrt(p);  // the jet's speed
    // The mouth flow tau ago (tau ~ 1 / v: a slower jet takes longer: the
    // pipe goes flat as the wind sags); linear interpolation.
    const float tau = float(tau_) / std::max(v, 0.35f);
    float rp = float(w_) - tau;
    while (rp < 0) rp += float(mask_ + 1);
    const int i0 = int(rp) & mask_, i1 = (i0 + 1) & mask_;
    const float a = rp - std::floor(rp);
    const float u = hist_[i0] * (1 - a) + hist_[i1] * a;
    // The jet: its deflection by the flow (and the room's pressure there),
    // its turbulence; the share that enters the pipe.
    float white, hiss;  // (two independent noises from one step: the jet's, the radiated)
    rng_.pair(white, hiss);
    const float n = float(v_.noise) * white;
    const float j = std::tanh(float(v_.gain) * (u + 0.05f * room) + float(v_.bias) + n);
    const float d = -float(v_.drive) * v * j;
    float mouth = 0;
    const int K = int(k_.size());
    for (int i = 0; i < K; ++i) {
      const float r = c_[i] * re_[i] - s_[i] * im_[i] + in_[i] * d, im = s_[i] * re_[i] + c_[i] * im_[i];
      re_[i] = r;
      im_[i] = im;
      mouth += im;
    }
    hist_[w_] = mouth;
    w_ = (w_ + 1) & mask_;
    // The mouth radiates the flow's rate of change (a small source); the
    // jet's own flow adds its breath and its edge tone a little.
    jl1_ += jetA_ * (d - jl1_);
    jl2_ += jetA_ * (jl1_ - jl2_);
    jl3_ += jetA_ * (jl2_ - jl3_);
    jl4_ += jetA_ * (jl3_ - jl4_);
    // (differenced, then low-passed twice at ~4 kHz: a hiss around 1-5 kHz,
    // not a fizz rising to the Nyquist: the recorded Bourdon has its energy
    // above 8 kHz ~60 dB down)
    bl1_ += 0.41f * ((hiss - prevHiss_) - bl1_);
    bl2_ += 0.41f * (bl1_ - bl2_);
    const float b = float(v_.breath) * v * bl2_;
    prevHiss_ = hiss;
    // The puff: the same turbulence, broader (low-passed at ~3 kHz, not
    // differenced), fading as the tone takes over.
    puffLp_ += 0.33f * (hiss - puffLp_);
    const float pf = float(v_.puff) * puffEnv_ * v * puffLp_;
    puffEnv_ *= puffDecay_;
    const float y = (mouth - prev_) * float(fs_) * 1e-3f + float(v_.jetRad) * (jl4_ - prevD_) * float(fs_) * 1e-3f + b + pf;
    prev_ = mouth;
    prevD_ = jl4_;
    // Asleep when closed and quiet.
    if (!on_ && p < 1e-5f && std::fabs(mouth) < 1e-6f) {
      if (++quiet_ > int(0.05 * fs_)) { awake_ = false; reset(); }
    } else {
      quiet_ = 0;
    }
    return y;
  }
  void reset() {
    std::fill(re_.begin(), re_.end(), 0.0f);
    std::fill(im_.begin(), im_.end(), 0.0f);
    std::fill(hist_.begin(), hist_.end(), 0.0f);
    valvePos_ = foot_ = prev_ = prevD_ = jl1_ = jl2_ = jl3_ = jl4_ = prevHiss_ = puffLp_ = puffEnv_ = bl1_ = bl2_ = 0;
  }
  double hz() const { return hz_; }

 private:
  double fs_ = 48000, hz_ = 440, tau_ = 0, ratio_ = 1;
  PipeVoicing v_;
  std::vector<int> k_;
  std::vector<float> re_, im_, c_, s_, in_, hist_;
  int mask_ = 0, w_ = 0, quiet_ = 0;
  float valvePos_ = 0, foot_ = 0, prev_ = 0, prevD_ = 0, jl1_ = 0, jl2_ = 0, jl3_ = 0, jl4_ = 0, prevHiss_ = 0, jetA_ = 1, puffLp_ = 0, puffEnv_ = 0, puffDecay_ = 0, bl1_ = 0, bl2_ = 0;
  bool on_ = false, awake_ = false;
  Rng rng_;
};

}  // namespace pajupilli
