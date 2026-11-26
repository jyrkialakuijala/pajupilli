#include "rain.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"
#include "vmath.h"

namespace pajupilli {

bool fxRain(const Instrument& inst) { return std::strcmp(inst.name, "fx-rain") == 0; }

namespace {

constexpr double kTwoPi = 6.283185307179586;
// A thin hemispherical bowl's bending modes n = 2 .. 5 (Rayleigh: f_n ~ n (n^2 - 1) / sqrt(n^2 + 1):
// 1, 2.83, 5.42, 8.77), over the first, but the wall's profile thinned so that n = 3 sits on the
// twelfth (3.0), as handbell founders tune their bells' second partial: the untuned 2.83 would lie
// a semitone under the 3x bowl beside it. And what each gives the air (the higher ones radiate
// less, fewer of them reach the rim's full swing).
constexpr int kModes = 4;
constexpr double kRatio[kModes] = {1.0, 3.0, 5.4232, 8.7710};
constexpr double kWeight[kModes] = {1.0, 0.35, 0.3, 0.2};
constexpr double kDropGain = 0.5;   // a 2 mm drop square on the glass: the rim's swing
constexpr double kTickGain = 0.15;  // the impact's own tick
constexpr double kBubbleGain = 2.0;
constexpr int kMaxNotes = 24, kMaxBubbles = 96, kMaxTicks = 64;

// A drop's terminal speed (m/s) by its diameter (mm): Atlas, Srivastava and Sekhon (1973).
double terminal(double D) { return std::max(0.1, 9.65 - 10.3 * std::exp(-0.6 * D)); }

// The spectrum of a sin^2 pulse of length tau at f, over its value at 0 (x = f tau).
double pulseSpectrum(double x) {
  const double d = 1 - x * x;
  if (std::fabs(d) < 1e-6) return 0.5;
  if (x < 1e-9) return 1;
  return std::fabs(std::sin(M_PI * x) / (M_PI * x) / d);
}

}  // namespace

RainModel::RainModel(const Instrument& inst) : Tube(inst) {
  // (the knobs' starting places; live's K1-K4 and -seq's "knob K X" move them)
  density_ = std::clamp(knob("PJ_RAIN_DENSITY", 0.5), 0.0, 1.0);
  water_ = std::clamp(knob("PJ_RAIN_WATER", 0.5), 0.0, 1.0);
  ring_ = std::clamp(knob("PJ_RAIN_RING", 0.5), 0.0, 1.0);
  bowls_ = std::clamp(knob("PJ_RAIN_BOWLS", 0.75), 0.0, 1.0);
}

void RainModel::resetAir() {
  Tube::resetAir();
  notes_.clear();
  bubbles_.clear();
  ticks_.clear();
  side_.clear();
}

const float* RainModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

void RainModel::tune(Mode& m, double bk) const {
  const double w = kTwoPi * std::min(m.f * bk, 0.47 * instrument().fs) / instrument().fs;
  m.c = std::cos(w);
  m.s = std::sin(w);
}

void RainModel::setBend(double semitones) {
  if (semitones == bend_) return;
  bend_ = semitones;
  bk_ = std::exp2(semitones / 12);
  for (Note& n : notes_)
    for (Bowl& b : n.bowls)
      for (Mode& m : b.modes) tune(m, bk_);
}

// Marshall-Palmer's diameters (0.3 mm .. dmax), weighted by the flux: each size by its speed.
double RainModel::drawDiameter(double lambda, double dmax) {
  constexpr double kMin = 0.3;
  const double span = 1 - std::exp(-lambda * (dmax - kMin));
  double D = kMin;
  for (int i = 0; i < 16; ++i) {
    D = kMin - std::log(1 - urand() * span) / lambda;
    if (urand() * terminal(dmax) < terminal(D)) break;
  }
  return D;
}

void RainModel::drop(Note& n, Bowl& b, double D, bool glass, double height) {
  (void)n;
  const double fs = instrument().fs;
  const double v = terminal(D);
  const double p = D * D * D * v / (8 * terminal(2.0));  // momentum, 1 for a 2 mm drop
  const double tau = 1e-3 * D / v;                       // the impact's length (s)
  const double theta = kTwoPi * urand(), z = height > 0 ? height : 0.25 + 0.75 * urand();
  const double push = (glass ? 1.0 : 0.12) * kDropGain * b.gain * p;
  for (Mode& m : b.modes) {
    const double at = m.sine ? std::sin(m.n * theta) : std::cos(m.n * theta);
    m.re += push * pulseSpectrum(m.f * bk_ * tau) * at * fastPow(z, m.n - 1) * kWeight[m.n - 2];
  }
  const double water = 2 * water_;
  if (water <= 0) return;
  // The impact's tick: the force's derivative, its size the momentum over the contact time.
  if (int(ticks_.size()) < kMaxTicks) {
    Tick t;
    t.pos = 0;
    t.len = std::max(3, int(std::lround(tau * fs)));
    t.amp = water * kTickGain * p * (2.5e-4 / tau) * (glass ? 1.0 : 0.5) * (urand() < 0.5 ? 1 : -1);
    t.pan = b.pan;
    ticks_.push_back(t);
  }
  if (glass) return;
  // Into the water: a bubble, by the drop's size.
  double f = 0, amp = 0;
  if (D >= 0.8 && D <= 1.2) {  // regular entrainment, every time: 21 .. 13 kHz
    f = 21000 - 8000 * (D - 0.8) / 0.4;
    amp = 0.6;
  } else if (D > 2 && urand() < 0.5) {  // irregular, large drops: 1 .. 10 kHz, the larger louder
    f = 1000 * fastPow(10.0, urand());
    amp = (D / 2) * std::sqrt(1000 / f);
  }
  if (f <= 0 || f > 0.45 * fs) return;
  if (int(bubbles_.size()) >= kMaxBubbles) bubbles_.erase(bubbles_.begin());
  const double d = 0.043 * f + 0.0014 * std::pow(f, 1.5);
  Bubble u;
  u.re = 0;
  u.im = 1;  // (out: re, from zero)
  u.c = std::cos(kTwoPi * f / fs);
  u.s = std::sin(kTwoPi * f / fs);
  u.decay = std::exp(-d / fs);
  u.chirp = 1 + 0.1 * d / fs;  // the bubble's radius shrinking: the pitch rises
  u.amp = water * kBubbleGain * amp * (0.6 + 0.8 * urand());
  u.pan = std::clamp(b.pan + 0.4 * (urand() - 0.5), -1.0, 1.0);
  bubbles_.push_back(u);
}

void RainModel::keyOn(int midi, int velocity) {
  const double fs = instrument().fs;
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  for (Note& n : notes_)
    if (n.midi == midi && n.down) keyOff(midi);
  if (int(notes_.size()) >= kMaxNotes) {  // the oldest released note, else the oldest, gives way
    auto it = std::find_if(notes_.begin(), notes_.end(), [](const Note& n) { return !n.down; });
    notes_.erase(it != notes_.end() ? it : notes_.begin());
  }
  Note n;
  n.midi = midi;
  // The rain rate: 0.3 mm/h (a drizzle) .. 90 mm/h (a cloudburst), by the touch.
  n.R = 0.3 * fastPow(300.0, v);
  n.lambda = 4.1 * fastPow(n.R, -0.21);
  // The largest drops: the tail cut by breakup, at a size growing with the rate (~1.2 mm in a
  // drizzle, ~4.6 mm in a cloudburst).
  n.dmax = std::clamp(1.5 * std::pow(n.R, 0.25), 0.8, 5.5);
  const double f0 = 440 * std::exp2((midi - 69) / 12.0);
  const int count = 1 + int(std::lround(4 * bowls_));
  const double dens = 25 * fastPow(32.0, density_);  // drops a second on the note's bowl
  const double centre = std::clamp((midi - 64) / 40.0, -0.6, 0.6);
  // The ring: T60 of the first mode, ~4 s at middle C, shorter up the keyboard (K3 x 0.25 .. 4).
  const double ringMul = fastPow(4.0, 2 * ring_ - 1);
  for (int k = 1; k <= count; ++k) {
    const double f = k * f0;
    if (k > 1 && f > 9000) break;
    Bowl b;
    b.f = f;
    b.rate = dens / k;  // (a smaller bowl catches fewer drops)
    b.pan = std::clamp(centre + (k == 1 ? 0.0 : 0.5 * (urand() - 0.5)), -1.0, 1.0);
    // (lighter, a smaller bowl swings more under the same drop (~k) but moves less air with its
    // smaller wall: ~sqrt(k) between the two)
    b.gain = std::sqrt(double(k));
    const double t1 = std::clamp(4 * fastPow(f / 261.6, -0.4), 1.0, 10.0) * ringMul;
    for (int j = 0; j < kModes; ++j) {
      const double fm = f * kRatio[j];
      if (fm * 1.003 > 0.45 * fs) break;
      const double t60 = t1 * fastPow(kRatio[j], -0.6);
      const double t60r = std::max(0.15, std::min(t60, 1.5 * fastPow(f / 261.6, -0.2)));
      const double split = 0.0005 + 0.002 * urand();  // the flaws' split of the doublet
      for (int s = 0; s < 2; ++s) {
        Mode m;
        m.n = j + 2;
        m.sine = s == 1;
        m.f = fm * (s ? 1 + 0.5 * split : 1 - 0.5 * split);
        m.rHeld = std::exp(-6.908 / (t60 * fs));
        m.rRel = std::exp(-6.908 / (t60r * fs));
        tune(m, bk_);
        b.modes.push_back(m);
      }
    }
    b.next = -std::log(1 - urand()) * fs / b.rate;
    n.bowls.push_back(b);
  }
  notes_.push_back(n);
  // The first drop: a drip from the eaves, its size by the touch (larger than the rain's own),
  // on the note's bowl, always near its rim (the eaves drip in one place).
  Note& nn = notes_.back();
  drop(nn, nn.bowls[0], 1.2 + 3.8 * v, true, 0.9);
}

void RainModel::keyOff(int midi) {
  for (Note& n : notes_)
    if (n.midi == midi && n.down) {
      n.down = false;
      for (Bowl& b : n.bowls)
        for (Mode& m : b.modes) m.rHeld = m.rRel;  // (a hand on the bowls)
    }
}

float RainModel::sample() {
  const double fs = instrument().fs;
  // The wind: the rate's gusts (log-normal, an Ornstein-Uhlenbeck walk: 3 s, +-35 %).
  if (++gustCount_ >= 64) {
    gustCount_ = 0;
    const double dt = 64 / fs, tau = 3;
    const double g = 2 * (urand() + urand() + urand() - 1.5);  // (~unit variance)
    gust_ += -gust_ * dt / tau + 0.35 * std::sqrt(2 * dt / tau) * g;
    gustF_ = std::exp(gust_);
  }
  double out = 0, side = 0;
  for (Note& n : notes_) {
    double e = 0;
    for (Bowl& b : n.bowls) {
      if (n.down) {
        b.next -= gustF_;
        while (b.next <= 0) {
          drop(n, b, drawDiameter(n.lambda, n.dmax), urand() < 0.6);
          b.next += -std::log(1 - urand()) * fs / b.rate;
        }
      }
      double o = 0;
      for (Mode& m : b.modes) {
        const double re = m.rHeld * (m.re * m.c - m.im * m.s);
        const double im = m.rHeld * (m.re * m.s + m.im * m.c);
        m.re = re;
        m.im = im;
        o += im;
        e += re * re + im * im;
      }
      out += o;
      side += b.pan * o;
    }
    n.quiet = (!n.down && e < 1e-12) ? n.quiet + 1 : 0;
  }
  notes_.erase(std::remove_if(notes_.begin(), notes_.end(), [](const Note& n) { return n.quiet > 256; }), notes_.end());
  for (Bubble& b : bubbles_) {
    const double re = b.decay * (b.re * b.c - b.im * b.s), im = b.decay * (b.re * b.s + b.im * b.c);
    b.re = re;
    b.im = im;
    const double s2 = std::min(b.s * b.chirp, 0.95);  // (the chirp: the rotation's angle grows)
    b.c = std::sqrt(1 - s2 * s2);
    b.s = s2;
    out += b.amp * re;
    side += b.pan * b.amp * re;
  }
  bubbles_.erase(std::remove_if(bubbles_.begin(), bubbles_.end(),
                                [](const Bubble& b) { return b.re * b.re + b.im * b.im < 1e-8; }),
                 bubbles_.end());
  for (Tick& t : ticks_) {
    const double y = t.amp * std::sin(kTwoPi * (t.pos + 0.5) / t.len);
    out += y;
    side += t.pan * y;
    ++t.pos;
  }
  ticks_.erase(std::remove_if(ticks_.begin(), ticks_.end(), [](const Tick& t) { return t.pos >= t.len; }), ticks_.end());
  // (a NaN: Tube::process clears the notes, resetAir)
  constexpr double kLevel = 0.06;  // (the cluster's pressure, as the other instruments')
  side_.push_back(float(outScale() * kLevel * side));
  last_ = float(kLevel * out);
  return finish(last_);
}

// --- The player.

RainPlayer::RainPlayer(RainModel& m) : Performer(m), m_(m) {}

void RainPlayer::noteOn(int midi, int velocity) {
  if (midi < 0 || midi > 127) return;
  articulate(midi, velocity, false);
  keyDown_[midi] = sounding_[midi] = true;
  note_ = midi;
  blowing_ = true;
  // (the director's accent: a harder touch)
  m_.keyOn(midi, std::clamp(int(std::lround(velocity * hardness_)), 1, 127));
}

void RainPlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  keyDown_[midi] = false;
  if (!hold_) {
    m_.keyOff(midi);
    sounding_[midi] = false;
  }
}

void RainPlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int k = 0; k < 128; ++k)
      if (sounding_[k] && !keyDown_[k]) {
        m_.keyOff(k);
        sounding_[k] = false;
      }
}

bool RainPlayer::setKnob(int k, double x) {
  x = std::clamp(x, 0.0, 1.0);
  switch (k) {
    case 0: m_.setDensity(x); return true;  // K1: the drops' density
    case 1: m_.setWater(x); return true;    // K2: bubbles and ticks
    case 2: m_.setRing(x); return true;     // K3: the bowls' ring
    case 3: m_.setBowls(x); return true;    // K4: bowls per key
    default: return false;
  }
}

void RainPlayer::render(float* out, int n) {
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
