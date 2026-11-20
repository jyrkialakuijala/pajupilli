#include "soundtrack.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"

namespace pajupilli {

bool fxSoundtrack(const Instrument& inst) { return std::strcmp(inst.name, "fx-soundtrack") == 0; }

namespace {

constexpr double kTwoPi = 6.283185307179586;
// The wake oscillator (Facchinetti, de Langre and Biolley 2004): its nonlinearity, its coupling
// to the structure's acceleration, the fluid's added damping (gamma = C_D / (4 pi St)).
constexpr double kEps = 0.3, kA = 12, kGamma = 0.8;
// The string's mass ratio (nylon in air, its added mass included): mu = (rho_s + C_M rho_air)
// (pi / 4) / rho_air ~ 750; the lift's coupling M = C_L0 / (16 pi^2 St^2 mu), C_L0 = 0.3, St = 0.2.
constexpr double kMu = 750;
const double kM = 0.3 / (16 * M_PI * M_PI * 0.2 * 0.2 * kMu);
// The bridge: its push along the string (the tension's swing) over its push across (the slope),
// per (swing in diameters)^2. The longitudinal coupling a soundbox's bridge has through the
// strings' down-bearing angle; set so that a string swinging fully gives the octave ~-10 dB.
constexpr double kAlong = 10;
constexpr double kLevel = 4.0;  // (the course's pressure, as the other instruments')
constexpr int kMaxNotes = 24;
constexpr double kHiss = 0.0025;  // (the slots' turbulence, ~40 dB under the strings)

}  // namespace

// How much of a cell the shutter has uncovered.
double AeolianModel::exposure(const Cell& z, double open) {
  return std::clamp((open - z.lo) / (z.hi - z.lo), 0.0, 1.0);
}

AeolianModel::AeolianModel(const Instrument& inst) : Tube(inst) {
  // (the knobs' starting places; live's K1-K4 and -seq's "knob K X" move them)
  gusts_ = std::clamp(knob("PJ_AEOLIAN_GUSTS", 0.5), 0.0, 1.0);
  swell_ = std::clamp(knob("PJ_AEOLIAN_SWELL", 0.5), 0.0, 1.0);
  fifth_ = std::clamp(knob("PJ_AEOLIAN_FIFTH", 0.5), 0.0, 1.0);
  ring_ = std::clamp(knob("PJ_AEOLIAN_RING", 0.5), 0.0, 1.0);
}

void AeolianModel::resetAir() {
  Tube::resetAir();
  notes_.clear();
  side_.clear();
  dcIn_ = dcOut_ = dcSideIn_ = dcSideOut_ = hissLo_ = hissBand_ = 0;
}

const float* AeolianModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

void AeolianModel::tune(String& s) {
  const double fs = instrument().fs;
  for (int j = 0; j < s.modes; ++j) {
    Mode& m = s.mode[j];
    m.w = kTwoPi * std::min(m.f * bk_, 0.47 * fs);
    m.c = std::cos(m.w / fs);
    m.s = std::sin(m.w / fs);
  }
}

void AeolianModel::setBend(double semitones) {
  if (semitones == bend_) return;
  bend_ = semitones;
  bk_ = std::exp2(semitones / 12);
  for (Note& n : notes_) {
    for (String& s : n.str) tune(s);
    damp(n);
  }
}

// Each mode's decay a sample: its own, and the air's while the wind blows across the string
// (gamma W / (2 mu) on the uncovered share of its length).
void AeolianModel::damp(Note& n) {
  const double fs = instrument().fs, U = gustF_;
  for (String& s : n.str) {
    double bare = 0;
    for (int c = 0; c < s.cells; ++c) bare += s.cell[c].span * exposure(s.cell[c], n.open);
    const double W = kTwoPi * s.f * bk_ * U;
    const double air = s.gain * bare * kGamma * W / (2 * kMu);
    for (int j = 0; j < s.modes; ++j) {
      Mode& m = s.mode[j];
      // (released, the shutter's felt edge rests on the strings: T60 at most ~2.5 s)
      const double felt = n.down ? 0.0 : 6.908 / std::clamp(2.5 * std::pow(s.f / 261.6, -0.2), 0.5, 3.5);
      m.r = std::exp(-(std::max(m.zeta * m.w, felt) + (j + 1) * air) / fs);
    }
    for (int c = 0; c < s.cells; ++c) {
      Cell& z = s.cell[c];
      z.W = W * z.speed;
      // (Strouhal's law gives the shedding's observed frequency; the van der Pol limit cycle
      // runs ~eps^2 / 16 below its linear one. High up, the wake takes several steps a sample,
      // each under 0.4 rad.)
      const double Wl = z.W / (1 - kEps * kEps / 16);
      z.steps = 1 + int(Wl / fs / 0.4);
      z.hs = 1 / (fs * z.steps);
      z.We2 = 2 * (1 - std::cos(Wl * z.hs)) / (z.hs * z.hs);  // (the step's exact rotation)
    }
  }
}

void AeolianModel::keyOn(int midi, int velocity) {
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
  // The shutter: it slides as far as the touch, the quicker the harder (the whole travel in
  // ~0.8 s at the hardest; K2 x 0.25 .. 4).
  n.target = 0.03 + 0.97 * std::pow(v, 1.5);
  n.rate = (0.5 + 0.5 * v) / (0.8 * std::pow(4.0, 2 * swell_ - 1) * fs);
  const double f0 = 440 * std::exp2((midi - 69) / 12.0);
  const double ringMul = std::pow(4.0, 2 * ring_ - 1);
  // The voicing: a string's swing (in its diameters) grows up the keyboard (~f^0.55: its Q
  // grows); the harp's maker evens the keys out (the strings' gauges and the box's radiation,
  // not modelled: a gain).
  n.voice = std::pow(f0 / 261.6, -0.55);
  // Three strings on the note, their tensions a few cents apart, and one on the fifth.
  const double d = 1.5 + 2.5 * urand();
  const double fr[kStrings] = {f0 * std::exp2(-d / 1200), f0 * std::exp2(d / 1200),
                               f0 * std::exp2(0.4 * d * (urand() - 0.5) / 1200),
                               1.5 * f0 * std::exp2((urand() - 0.5) / 1200)};
  const double gain[kStrings] = {1.0, 0.8, 0.62, 1.1 * fifth_};
  const double pan[kStrings] = {-0.3, 0.3, 0.0, 0.1};
  const double centre = std::clamp((midi - 64) / 40.0, -0.6, 0.6);
  for (int k = 0; k < kStrings; ++k) {
    String& s = n.str[k];
    s.f = fr[k];
    s.gain = gain[k];
    s.pan = std::clamp(centre + pan[k], -1.0, 1.0);
    // The ring: T60 of the first mode, ~3.5 s at middle C, longer down the keyboard; the
    // higher modes shorter (K4 x 0.25 .. 4).
    const double t1 = std::clamp(3.5 * std::pow(s.f / 261.6, -0.45), 1.0, 5.0) * ringMul;
    s.modes = 0;
    for (int j = 1; j <= kModes; ++j) {
      const double f = j * s.f * (1 + 1e-5 * j * j);  // (a thin string: barely stiff)
      if (j > 1 && f > 0.42 * fs) break;
      Mode& m = s.mode[s.modes++];
      m.f = f;
      const double t60 = t1 / std::sqrt(double(j));
      m.zeta = 6.908 / (t60 * kTwoPi * f);
    }
    tune(s);
    // The wake cells. The shutter uncovers the string's main stretch (two cells, a third and
    // three fifths along it) as it slides, the wind there at the first mode's lock speed; on its
    // way it also uncovers, one after another, short side slots where the wind comes through
    // faster, at two .. five times it, each across its mode's antinode (a quarter, a half, three
    // eighths, three tenths along), where it locks that mode. The fifth's string has only the first four
    // (up to its third mode). The wake starts from the free stream's turbulence.
    struct Plan { double speed, x, span, lo, hi; };
    static const Plan kPlan[kCells] = {{1, 0.29, 0.25, 0.0, 1.0},     {1, 0.61, 0.25, 0.0, 1.0},
                                       {2, 0.25, 0.05, 0.35, 0.65},   {3, 0.5, 0.025, 0.55, 0.8},
                                       {4, 0.375, 0.008, 0.7, 0.9},   {5, 0.3, 0.004, 0.8, 1.0}};
    s.cells = 0;
    for (int c = 0; c < kCells; ++c) {
      const Plan& p = kPlan[c];
      if (p.speed > s.modes || (k == 3 && c > 3)) break;
      Cell& z = s.cell[s.cells++];
      z.x = p.x + 0.03 * (urand() - 0.5);
      z.speed = p.speed * (1 + (c == 0 ? -0.004 : c == 1 ? 0.004 : 0.0) + 0.001 * (urand() - 0.5));
      z.span = p.span;
      z.lo = p.lo;
      z.hi = p.hi;
      for (int j = 0; j < kModes; ++j) z.phi[j] = std::sin((j + 1) * M_PI * z.x);
      z.q = 0.05 * (urand() - 0.5);
      z.qd = 0;
    }
  }
  notes_.push_back(n);
  damp(notes_.back());
}

void AeolianModel::keyOff(int midi) {
  for (Note& n : notes_)
    if (n.midi == midi && n.down) {
      n.down = false;
      n.rate = 1 / (0.25 * instrument().fs);  // (the shutter closes, in a quarter second)
    }
}

float AeolianModel::sample() {
  const double fs = instrument().fs, h = 1 / fs;
  if (++count_ >= 64) {
    count_ = 0;
    // The blower's gusts: an Ornstein-Uhlenbeck walk on the speed's log, 2.5 s, +-0.3 % (a
    // reservoir steadies it; K1: 0.04 .. 2.4 %. A wake catches its lock only within ~+-2 %,
    // and holds it wider).
    const double dt = 64 / fs, tau = 2.5;
    const double g = 2 * (urand() + urand() + urand() - 1.5);
    gust_ += -gust_ * dt / tau + 0.003 * std::pow(64.0, gusts_ - 0.5) * std::sqrt(2 * dt / tau) * g;
    gustF_ = std::exp(gust_);
    for (Note& n : notes_) damp(n);
  }
  const double inertia = 1 - std::exp(-1 / (0.05 * fs));  // (the shutter's: 50 ms)
  double out = 0, side = 0, hiss = 0;
  for (Note& n : notes_) {
    const double goal = n.down ? n.target : 0.0;
    n.travel = goal > n.travel ? std::min(goal, n.travel + n.rate) : std::max(goal, n.travel - n.rate);
    n.open += (n.travel - n.open) * inertia;
    hiss += n.voice * n.open;
    double e2 = 0;
    for (String& s : n.str) {
      double F[kModes] = {};
      for (int c = 0; c < s.cells; ++c) {
        Cell& z = s.cell[c];
        const double expo = s.gain * exposure(z, n.open);
        if (expo <= 1e-7) {
          z.q *= 0.999, z.qd *= 0.999;  // (the wake washes away)
          continue;
        }
        const double W = z.W, We2 = z.We2, hs = z.hs;
        const int steps = z.steps;
        double acc = 0;  // the string's acceleration at the cell (diameters / s^2)
        for (int j = 0; j < s.modes; ++j) acc -= z.phi[j] * s.mode[j].w * s.mode[j].w * s.mode[j].re;
        const double push = kA * acc + 0.02 * We2 * (urand() - 0.5);  // (and the free stream's turbulence)
        for (int k = 0; k < steps; ++k) {
          // (the wake's damping taken half before, half after the step: the trapezoid, which
          // leaves its frequency where it is)
          const double dq = 0.5 * hs * kEps * W * (z.q * z.q - 1);
          z.qd = (z.qd * (1 - dq) + hs * (push - We2 * z.q)) / (1 + dq);
          z.q += hs * z.qd;
        }
        // The lift on the cell's stretch, onto each mode (a sine mode's mass: half the string's).
        const double lift = 2 * z.span * kM * W * W * expo * z.q;
        for (int j = 0; j < s.modes; ++j) F[j] += lift * z.phi[j];
      }
      double across = 0, swing2 = 0;
      for (int j = 0; j < s.modes; ++j) {
        Mode& m = s.mode[j];
        const double re = m.r * (m.re * m.c - m.im * m.s);
        const double im = m.r * (m.re * m.s + m.im * m.c) - h * F[j] / m.w;
        m.re = re;
        m.im = im;
        across += (j + 1) * re;
        swing2 += (j + 1) * (j + 1) * re * re;
        e2 += re * re + im * im;
      }
      // The bridge: the tension (1 + kappa sum j^2 y_j^2) times the slope, and the tension's
      // swing along the string.
      const double o = across * (1 + kAlong * 0.25 * swing2) + kAlong * swing2;
      out += n.voice * o;
      side += n.voice * s.pan * o;
    }
    n.quiet = (!n.down && n.open < 1e-4 && e2 < 1e-12) ? n.quiet + 1 : 0;
  }
  notes_.erase(std::remove_if(notes_.begin(), notes_.end(), [](const Note& n) { return n.quiet > 256; }), notes_.end());
  // The slots' hiss: the wind's turbulence through the open shutters, a broad band around 3 kHz.
  {
    const double k1 = 1 - std::exp(-kTwoPi * 6000 / fs), k2 = 1 - std::exp(-kTwoPi * 1500 / fs);
    hissLo_ += (kHiss * hiss * (urand() - 0.5) - hissLo_) * k1;  // (below 6 kHz)
    hissBand_ += (hissLo_ - hissBand_) * k2;                      // (less what is below 1.5 kHz)
    out += hissLo_ - hissBand_;
  }
  // (the tension's mean pushes the bridge steadily: no sound; a blocker at ~8 Hz)
  const double a = 1 - kTwoPi * 8 / fs;
  dcOut_ = a * dcOut_ + out - dcIn_;
  dcIn_ = out;
  dcSideOut_ = a * dcSideOut_ + side - dcSideIn_;
  dcSideIn_ = side;
  side_.push_back(float(outScale() * kLevel * dcSideOut_));
  last_ = float(kLevel * dcOut_);
  return finish(last_);
}

// --- The player.

AeolianPlayer::AeolianPlayer(AeolianModel& m) : Performer(m), m_(m) {}

void AeolianPlayer::noteOn(int midi, int velocity) {
  if (midi < 0 || midi > 127) return;
  articulate(midi, velocity, false);
  keyDown_[midi] = sounding_[midi] = true;
  note_ = midi;
  blowing_ = true;
  // (the director's accent: a harder touch)
  m_.keyOn(midi, std::clamp(int(std::lround(velocity * hardness_)), 1, 127));
}

void AeolianPlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  keyDown_[midi] = false;
  if (!hold_) {
    m_.keyOff(midi);
    sounding_[midi] = false;
  }
}

void AeolianPlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int k = 0; k < 128; ++k)
      if (sounding_[k] && !keyDown_[k]) {
        m_.keyOff(k);
        sounding_[k] = false;
      }
}

bool AeolianPlayer::setKnob(int k, double x) {
  x = std::clamp(x, 0.0, 1.0);
  switch (k) {
    case 0: m_.setGusts(x); return true;  // K1: the gusts
    case 1: m_.setSwell(x); return true;  // K2: the shutter's time
    case 2: m_.setFifth(x); return true;  // K3: the fifth
    case 3: m_.setRing(x); return true;   // K4: the ring
    default: return false;
  }
}

void AeolianPlayer::render(float* out, int n) {
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
