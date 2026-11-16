#include "percextras.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"

namespace pajupilli {

bool percExtras(const Instrument& inst) { return std::strcmp(inst.name, "perc-extras") == 0; }

namespace {

constexpr double kPi = 3.14159265358979323846;

enum Sound {
  kNone, kClickNoise, kCastanets, kSeqLow, kSeqHigh, kSnap, kHighQ, kWhip, kScratchPush, kScratchPull, kSticks,
  kSquare, kMetroClick, kMetroBell
};

Sound kindOf(int key) {
  switch (key) {
    case 20: return kClickNoise;
    case 21: return kCastanets;
    case 23: return kSeqLow;
    case 24: return kSeqHigh;
    case 26: return kSnap;
    case 27: return kHighQ;
    case 28: return kWhip;
    case 29: return kScratchPush;
    case 30: return kScratchPull;
    case 31: return kSticks;
    case 32: return kSquare;
    case 33: return kMetroClick;
    case 34: return kMetroBell;
    default: return kNone;
  }
}

// Each sound's level at the output: they are set to sound about equally loud
// at the same velocity (tools/test.py gm2Extras checks the spread).
float gainOf(int kind) {
  static const float g[] = {0, 1.504f, 5.614f, 0.4613f, 0.4018f, 7.484f, 0.2524f, 0.7067f, 0.2033f, 0.1964f, 0.6683f, 0.4655f, 4.231f, 0.9755f};
  return g[kind];
}

// A free-free beam's mode shape at x (m from a free end), as the end's value 2
// (unit mean square over the length).
double beamShape(int n, double x, double L) {
  static const double bl[] = {4.7300408, 7.8532046, 10.9956078, 14.1371655, 17.2787597, 20.4203522};
  const double b = bl[n] / L, bL = bl[n];
  const double sg = (std::cosh(bL) - std::cos(bL)) / (std::sinh(bL) - std::sin(bL));
  return std::cosh(b * x) + std::cos(b * x) - sg * (std::sinh(b * x) + std::sin(b * x));
}

}  // namespace

PercExtrasModel::PercExtrasModel(const Instrument& inst) : Tube(inst), dt_(1.0 / inst.fs) {}

void PercExtrasModel::resetAir() {
  Tube::resetAir();
  ev_.clear();
}

void PercExtrasModel::addMode(Event& e, double hz, double t60, double w, double inP, double inN) {
  if (e.nm >= kModes || hz >= 0.45 / dt_) return;
  const int k = e.nm++;
  const double r = std::exp(-6.9078 / t60 * dt_), ph = 2 * kPi * hz * dt_;
  e.pr[k] = float(r * std::cos(ph));
  e.pi[k] = float(r * std::sin(ph));
  e.w[k] = float(w);
  e.inP[k] = float(inP);
  e.inN[k] = float(inN);
}

void PercExtrasModel::hit(int key, double v) {
  const Sound kind = kindOf(key);
  if (kind == kNone) return;
  v = std::clamp(v, 0.0, 1.0);
  auto ur = [&] {  // 0 .. 1
    rng_ = rng_ * 1664525u + 1013904223u;
    return (rng_ >> 8) * (1.0 / 16777216.0);
  };
  Event e;
  e.kind = kind;
  e.rng = rng_ ^ 0x9e3779b9u;
  const double fs = 1 / dt_;
  // A struck thing: the stroke's speed ~ velocity^1.6 (m/s, up to 3; about 30 dB from velocity 16 to
  // 127, as General MIDI's curve and the modal percussion); its
  // force a half-sine as long as the contact, its impulse in proportion to
  // the speed (so a shorter contact is brighter, not louder).
  const double speed = 3.0 * std::pow(std::max(v, 1e-3), 1.6);
  auto contact = [&](double ms) {  // Hertz: a faster blow, a little shorter
    e.pulse = float(std::max(1.0, ms * 1e-3 * fs * std::pow(speed, -0.2)));
    e.amp = float(speed * kPi / (2 * e.pulse));
  };
  // An electronic one: its level as a sample player's, v^(5/3).
  const float elec = float(std::pow(v, 5.0 / 3.0));
  switch (kind) {
    case kSticks: {
      // Two 5A hickory sticks (406 mm, 14.4 mm): free-free modes, each stick
      // a little its own; the click 6 cm from one's tip and 12 cm from the
      // other's; the hands damp them in 35-55 ms (research 3.1).
      static const double f[] = {330, 910, 1780, 2950, 4400, 6140};
      static const double t60[] = {0.045, 0.05, 0.055, 0.05, 0.04, 0.035};
      const double L = 0.406;
      for (int s = 0; s < 2; ++s) {
        const double var = 1 + 0.03 * (ur() - 0.5) + (s ? 0.012 : -0.012), xc = s ? 0.12 : 0.06;
        for (int n = 0; n < 6; ++n)
          addMode(e, f[n] * var, t60[n] * (0.9 + 0.2 * ur()), beamShape(n, xc, L) / 2 * std::sqrt(f[n] / f[0]));
      }
      contact(0.15);
      e.crack = 0.15f;
      e.life = int(0.25 * fs);
      break;
    }
    case kWhip: {
      // Two hinged maple boards slapped together: the air squeezed out (a
      // short broadband burst), then the boards' modes (research 3.2).
      static const double f[] = {200, 560, 1100, 1820, 2720, 3800};
      for (int s = 0; s < 2; ++s)
        for (int n = 0; n < 6; ++n)
          addMode(e, f[n] * (1 + 0.02 * (s - 0.5) + 0.01 * (ur() - 0.5)), 0.08 - 0.008 * n, 0.4 * std::sqrt(f[n] / f[0]));
      addMode(e, 1800, 0.004, 1.0, 0, 1);  // the air's burst, broad
      addMode(e, 4200, 0.003, 0.8, 0, 1);
      contact(0.3);
      e.noise = float(speed * 0.8);
      e.noiseTau = float(0.0006 * fs);
      e.crack = 0.25f;
      e.life = int(0.3 * fs);
      break;
    }
    case kSnap:
      // The finger on the palm (soft: ~0.8 ms) and the cupped hand's air
      // (research 3.3).
      // (A 0.4 ms contact: a longer one's first spectral null would fall on
      // the hand's ring near 2 kHz.)
      addMode(e, 600, 0.008, 0.15);
      addMode(e, 1400 * (1 + 0.04 * (ur() - 0.5)), 0.0063, 1.0);
      addMode(e, 2800 * (1 + 0.04 * (ur() - 0.5)), 0.0047, 0.8);
      contact(0.4);
      e.crack = 0.05f;
      e.life = int(0.08 * fs);
      break;
    case kCastanets:
      // The air between the two shells as they close (~3 kHz, broad) and the
      // shells' own modes (research 3.4).
      addMode(e, 3000 * (1 + 0.05 * (ur() - 0.5)), 0.004, 1.0);
      for (double hz : {2300.0, 3700.0, 5100.0, 6800.0}) addMode(e, hz * (1 + 0.02 * (ur() - 0.5)), 0.03, 0.3);
      contact(0.2);
      e.crack = 0.1f;
      e.life = int(0.12 * fs);
      break;
    case kMetroClick:
      // The tick: a small case knocked (two resonances, ~10 ms).
      addMode(e, 1800, 0.015, 1.0);
      addMode(e, 3400, 0.010, 0.7);
      contact(0.1);
      e.amp *= float(elec / std::max(speed / 3.0, 1e-3));  // (the electronic level law, v^(5/3))
      e.life = int(0.06 * fs);
      break;
    case kMetroBell:
      // The metronome's little bell: three partials.
      addMode(e, 1600, 0.45, 1.0);
      addMode(e, 4350, 0.30, 0.6);
      addMode(e, 8300, 0.18, 0.3);
      contact(0.15);
      e.amp *= float(elec / std::max(speed / 3.0, 1e-3));
      e.life = int(0.6 * fs);
      break;
    case kHighQ:
      e.amp = elec;
      e.life = int(0.06 * fs);
      break;
    case kSquare:
      e.amp = elec;
      e.life = int(0.004 * fs);
      break;
    case kClickNoise:
      e.amp = elec;
      e.life = int(0.002 * fs);
      break;
    case kSeqLow:
    case kSeqHigh:
      e.amp = elec;
      e.life = int(0.005 * fs);
      break;
    case kScratchPush:
    case kScratchPull:
      e.amp = elec;
      e.life = int((kind == kScratchPush ? 0.12 : 0.15) * fs);
      break;
    default:
      return;
  }
  if (e.crack > 0) e.hpPole = float(std::exp(-2 * kPi * 2000 * dt_));
  if (ev_.size() >= 8) ev_.erase(ev_.begin());  // (the oldest gives way)
  ev_.push_back(e);
}

float PercExtrasModel::step(Event& e) {
  const double t = e.t * dt_;
  float y = 0;
  auto white = [&] {
    e.rng = e.rng * 1664525u + 1013904223u;
    return float(int32_t(e.rng)) * (1.0f / 2147483648.0f);
  };
  switch (e.kind) {
    case kHighQ: {
      // A resonant zap: the pitch falls 3 kHz -> 700 Hz in ~25 ms.
      const double f = 700 + 2300 * std::exp(-t / 0.008);
      e.ph += f * dt_;
      y = float(std::sin(2 * kPi * e.ph) * std::exp(-t / 0.012) * (1 - std::exp(-t / 0.0003)));
      break;
    }
    case kSquare: {
      const double f = 1600;
      e.ph += f * dt_;
      const float sq = e.ph < 1.5 ? (std::fmod(e.ph, 1.0) < 0.5 ? 1.0f : -1.0f) : 0.0f;
      e.hp += 0.65f * (sq - e.hp);  // (a gentle low-pass: ~9 kHz)
      y = e.hp;
      break;
    }
    case kClickNoise:
      y = white() * float(0.5 - 0.5 * std::cos(2 * kPi * e.t / std::max(1, e.life)));
      break;
    case kSeqLow:
    case kSeqHigh: {
      const double f = e.kind == kSeqLow ? 1000 : 2000;
      y = float(std::sin(2 * kPi * f * t) * (0.5 - 0.5 * std::cos(2 * kPi * e.t / std::max(1, e.life))));
      break;
    }
    case kScratchPush:
    case kScratchPull: {
      // The record read at a speed r: a push sweeps forward (to 3x), a pull
      // backward (to -2.5x); a "voice" on it (a sawtooth at 140 Hz through
      // three formants), all its frequencies times |r|, its loudness with |r|.
      const double T = e.kind == kScratchPush ? 0.12 : 0.15, R = e.kind == kScratchPush ? 3.0 : -2.5;
      const double r = R * std::sin(kPi * std::min(t / T, 1.0));
      e.ph += 140 * r * dt_;
      const double saw = 2 * (e.ph - std::floor(e.ph)) - 1;
      static const double F[3] = {700, 1220, 2600}, B[3] = {90, 110, 160}, G[3] = {1.0, 0.6, 0.35};
      const double a = std::fabs(r);
      float out = 0;
      for (int i = 0; i < 3; ++i) {
        const double fc = std::min(F[i] * std::max(a, 0.05), 20000.0), bw = B[i] * std::max(a, 0.3);
        const double rr = std::exp(-kPi * bw * dt_), th = 2 * kPi * fc * dt_;
        const float pr = float(rr * std::cos(th)), pi = float(rr * std::sin(th));
        const float re = e.fr[i] * pr - e.fi[i] * pi + float(saw), im = e.fr[i] * pi + e.fi[i] * pr;
        e.fr[i] = re;
        e.fi[i] = im;
        out += float(G[i] * bw * dt_ * 12) * im;
      }
      y = float(a / std::fabs(R)) * (out + 0.01f * white());
      break;
    }
    default: {
      // A modal bank under the contact's force (and an air burst).
      const float F = e.t < e.pulse ? e.amp * float(std::sin(kPi * (e.t + 0.5) / e.pulse)) : 0.0f;
      const float N = e.noise > 0 ? e.noise * float(std::exp(-e.t / double(e.noiseTau))) * white() : 0.0f;
      const float inF = F * float(dt_) * 2e3f, inN = N * float(dt_) * 2e3f;
      for (int k = 0; k < e.nm; ++k) {
        const float re = e.re[k] * e.pr[k] - e.im[k] * e.pi[k] + e.inP[k] * inF + e.inN[k] * inN;
        const float im = e.re[k] * e.pi[k] + e.im[k] * e.pr[k];
        e.re[k] = re;
        e.im[k] = im;
        y += e.w[k] * im;
      }
      if (e.crack > 0) {  // the blow's own jolt, above the modes
        e.hp = e.hpPole * (e.hp + F - e.lastF);
        e.lastF = F;
        y += e.crack * e.hp;
      }
      return y * gainOf(e.kind);
    }
  }
  return y * e.amp * gainOf(e.kind);
}

float PercExtrasModel::sample() {
  float y = 0;
  for (Event& e : ev_) {
    y += step(e);
    ++e.t;
  }
  if (!ev_.empty())
    ev_.erase(std::remove_if(ev_.begin(), ev_.end(), [](const Event& e) { return e.t >= e.life; }), ev_.end());
  static const float g = float(knob("PJ_PERCX_OUT", 1.0));
  last_ = g * y;
  return finish(last_);
}

PercExtrasPlayer::PercExtrasPlayer(PercExtrasModel& m) : Performer(m), m_(m) {}

void PercExtrasPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  m_.hit(midi, velocity / 127.0);
}

void PercExtrasPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.asleep();
}

}  // namespace pajupilli
