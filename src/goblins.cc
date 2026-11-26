#include "goblins.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"
#include "vmath.h"

namespace pajupilli {

bool fxGoblins(const Instrument& inst) { return std::strcmp(inst.name, "fx-goblins") == 0; }

namespace {

constexpr double kTwoPi = 6.283185307179586;
// The pipe: open at both ends, the flame at kPlace of its length from the bottom (a little under
// a quarter: a quarter would sit on the fourth mode's node); its fundamental's Q kQ at C4 (as
// the root of the frequency over the keyboard: wider pipes in the treble lose less a cycle), the
// modes' losses the walls' (as the root of the frequency) and the radiation's (as its square);
// modes up to kTop Hz.
constexpr double kPlace = 0.23, kQ = 60, kTop = 6000;
constexpr double kWall = 0.8, kRadiation = 0.1;
// The draft: the lag at the start is kLagCold x the hot one (the draft grows as the root of the
// warmth: lag ~ sqrt((1 + c2) / (warm + c2)), c2 = 1 / (kLagCold^2 - 1)).
constexpr double kLagCold = 1.25, kC2 = 1 / (kLagCold * kLagCold - 1), kTuned = 0.4;
// The heat release's law (Heckl): sqrt(|1/3 + u/u0|), the corner at the flow's reversal rounded
// by kRound.
constexpr double kRound = 0.03;
constexpr double kLevel = 0.4;
// The gas lighting: the heat's rise time and its size (re the loop's: a soft thump that sets the
// pipe ringing, so that the oscillation grows from it rather than from the roar alone).
constexpr double kLight = 0.004, kThump = 0.3;
constexpr int kMaxNotes = 24;

inline double heat(double x) {
  const double a = 1.0 / 3 + x;
  return std::sqrt(std::sqrt(a * a + kRound * kRound));
}
const double kHeat0 = heat(0);
const double kSlope = (heat(1e-5) - heat(-1e-5)) / 2e-5;  // (its slope at rest)

}  // namespace

PyrophoneModel::PyrophoneModel(const Instrument& inst) : Tube(inst) {
  // (the knobs' starting places; live's K1-K4 and -seq's "knob K X" move them)
  warmth_ = std::clamp(knob("PJ_PYRO_WARMTH", 0.5), 0.0, 1.0);
  puff_ = std::clamp(knob("PJ_PYRO_PUFF", 0.5), 0.0, 1.0);
  roar_ = std::clamp(knob("PJ_PYRO_ROAR", 0.5), 0.0, 1.0);
  valve_ = std::clamp(knob("PJ_PYRO_VALVE", 0.5), 0.0, 1.0);
}

void PyrophoneModel::resetAir() {
  Tube::resetAir();
  notes_.clear();
  side_.clear();
}

const float* PyrophoneModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

// The pipe's modes at its pitch (bent).
void PyrophoneModel::tune(Note& n) {
  const double fs = instrument().fs, h = 1 / fs;
  const double fn = n.f0 * bk_;
  const double q1 = std::clamp(kQ * std::sqrt(fn / 261.63), 15.0, 300.0);
  // The flame's lag, past the quarter period, pulls the pipe flat by cot(phase) / 2Q: the pipe is
  // cut so that it sings in tune at kTuned of its warmth.
  const double phase = M_PI / 2 * std::sqrt((1 + kC2) / (kTuned + kC2));
  const double f1 = fn / (1 + 1 / std::tan(phase) / (2 * q1));
  n.sigma1 = M_PI * f1 / q1;
  n.modes = std::clamp(int(std::min(kTop, 0.42 * fs) / f1), 1, kModes);
  for (int k = 0; k < n.modes; ++k) {
    const int m = k + 1;
    const double w = kTwoPi * f1 * m, s = n.sigma1 * (kWall * std::sqrt(double(m)) + kRadiation * m * m);
    const double r = std::exp(-s * h);
    n.cr[k] = float(r * std::cos(w * h));
    n.ci[k] = float(r * std::sin(w * h));
    n.drive[k] = float(std::sin(M_PI * m * kPlace) / w);  // (its share at the flame, over its omega)
    n.look[k] = float(-std::cos(M_PI * m * kPlace));
    n.rad[k] = float((m & 1 ? 1 : -1) * m);  // (the top end: the volume velocity's rate)
  }
}

void PyrophoneModel::setBend(double semitones) {
  if (semitones == bend_) return;
  bend_ = semitones;
  bk_ = std::exp2(semitones / 12);
  for (Note& n : notes_) tune(n);
}

void PyrophoneModel::keyOn(int midi, int velocity) {
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  for (Note& n : notes_)
    if (n.midi == midi && n.down) keyOff(midi);
  if (int(notes_.size()) >= kMaxNotes) {  // the oldest released note, else the oldest, gives way
    auto it = std::find_if(notes_.begin(), notes_.end(), [](const Note& n) { return !n.down; });
    notes_.erase(it != notes_.end() ? it : notes_.begin());
  }
  notes_.emplace_back();
  Note& n = notes_.back();
  n.midi = midi;
  n.f0 = 440 * std::exp2((midi - 69) / 12.0);
  // The key opens the gas valve: a harder key, a larger flame (its gain over the pipe's losses,
  // cold and hot; its flow; its response's corner when hot) opened faster.
  n.size = 0.15 + 0.85 * std::pow(v, 1.5);
  n.gainCold = 1.3 + 0.1 * v;
  n.gainHot = 1.8 + 1.4 * v;
  n.cornerHot = 2.5 + 4.5 * v;
  n.open = 0.25 - 0.12 * v;
  // The flame's puffing: a few Hz, slower under the wider (bass) pipes' larger burners.
  n.pw = kTwoPi * 6.5 * fastPow(n.f0 / 261.63, 0.1) * (0.94 + 0.12 * urand());
  n.px = 2 * urand() - 1;
  n.pv = 0;
  n.pan = std::clamp((midi - 64) / 40.0, -0.5, 0.5);
  std::memset(n.re, 0, sizeof(n.re));
  std::memset(n.im, 0, sizeof(n.im));
  std::memset(n.hist, 0, sizeof(n.hist));
  tune(n);
}

void PyrophoneModel::keyOff(int midi) {
  for (Note& n : notes_)
    if (n.midi == midi && n.down) n.down = false;
}

float PyrophoneModel::sample() {
  const double fs = instrument().fs, h = 1 / fs;
  const double tauWarm = 2 * fastPow(8.0, warmth_);           // K1: 2 .. 16 s
  const double close = std::exp(-h / (0.1 * fastPow(30.0, valve_)));  // K4: 0.1 .. 3 s
  const double depth = 0.6 * puff_;                            // K2: the puffing's swing of the heat
  const double eta = 0.06 * roar_ * roar_;                     // K3: the roar, re the heat's swing
  const double c2 = kC2;
  const double kn = 1 - std::exp(-kTwoPi * 700 / fs), nsig = std::sqrt(3 * (2 - kn) / kn);
  double out = 0, side = 0;
  for (Note& n : notes_) {
    // The valve.
    if (n.down) {
      const double th = std::min(1.0, n.t / n.open);
      n.valve = n.size * (0.5 - 0.5 * std::cos(M_PI * th));
    } else {
      n.valve *= close;
    }
    const double open = n.valve / n.size;
    // The pipe warms towards what the flame gives it.
    n.warm += (open - n.warm) * h / tauWarm;
    // The flame's puffing (a weakly self-excited oscillator, its amplitude ~2, kicked by the
    // turbulence).
    n.pv += h * (0.3 * n.pw * (1 - n.px * n.px) * n.pv - n.pw * n.pw * n.px) + 0.4 * n.pw * std::sqrt(h) * (2 * urand() - 1);
    n.px += h * n.pv;
    const double puff = depth * 0.5 * n.px;
    // The flame's gain over the pipe's losses (the cold walls quench its edges; its puffing swings
    // it) and the flow past it (the draft: the saturation's scale).
    const double gain = (n.gainCold + (n.gainHot - n.gainCold) * n.warm) * open * (1 + puff);
    const double u0 = std::pow(n.size, 0.75) * std::sqrt((n.warm + c2) / (1 + c2)) + 1e-6;
    // The flame's response is a low-pass in the Strouhal number (its length over the flow): as the
    // draft grows and the burner warms, the flame shortens and its corner rises from the note to
    // several times it (the brightening). Its lag is part of the flame's: the pure delay is the
    // rest.
    const double f1 = n.f0 * bk_, corner = f1 * (1 + (n.cornerHot - 1) * n.warm * n.warm);
    const double lp = 1 - std::exp(-kTwoPi * corner * h);
    const double lag = fs * ((0.25 * std::sqrt((1 + c2) / (n.warm + c2)) * (1 + 0.4 * puff) - std::atan(f1 / corner) / kTwoPi) / f1) - 0.5;
    // The air's velocity at the flame, and what the flame saw a lag ago.
    double uf = 0;
    for (int k = 0; k < n.modes; ++k) uf += n.look[k] * n.im[k];
    n.hist[n.w] = float(uf);
    const double rd = n.w - std::clamp(lag, 1.0, kDelay - 2.0) + kDelay;
    const int i0 = int(rd);
    const double fr = rd - i0;
    const double ud = n.hist[i0 & (kDelay - 1)] * (1 - fr) + n.hist[(i0 + 1) & (kDelay - 1)] * fr;
    n.w = (n.w + 1) & (kDelay - 1);
    // The heat release (Heckl's law) and the roar; its rate drives the modes.
    n.nz += kn * (nsig * (2 * urand() - 1) - n.nz);
    const double p1 = std::sin(2 * M_PI * kPlace) / 2;
    const double a = gain * u0 * 2 * n.sigma1 / (p1 * kSlope) * std::hypot(1.0, f1 / corner);
    // (the gas lighting: the flame's heat comes up over a few ms)
    const double lit = 0.5 - 0.5 * std::cos(M_PI * std::min(1.0, n.t / kLight));
    n.flame += lp * ((heat(ud / u0) - kHeat0) - n.flame);
    const double q = a * (n.flame + eta * std::sqrt(n.size) * open * n.nz) + kThump * 2 * n.sigma1 / (p1 * kSlope) * std::sqrt(n.size) * lit;
    const double dq = q - n.qPrev;
    n.qPrev = q;
    double o = 0, e = 0;
    for (int k = 0; k < n.modes; ++k) {
      const float r = n.re[k], i = n.im[k];
      n.re[k] = n.cr[k] * r - n.ci[k] * i;
      n.im[k] = n.cr[k] * i + n.ci[k] * r - n.drive[k] * float(dq);
      o += n.rad[k] * n.re[k];
      e += n.re[k] * n.re[k];
    }
    out += o;
    side += n.pan * o;
    n.t += h;
    n.quiet = (!n.down && n.valve < 1e-4 && e < 1e-12) ? n.quiet + 1 : 0;
  }
  notes_.erase(std::remove_if(notes_.begin(), notes_.end(), [](const Note& n) { return n.quiet > 256; }), notes_.end());
  side_.push_back(float(outScale() * kLevel * side));
  last_ = float(kLevel * out);
  return finish(last_);
}

// --- The player.

PyrophonePlayer::PyrophonePlayer(PyrophoneModel& m) : Performer(m), m_(m) {}

void PyrophonePlayer::noteOn(int midi, int velocity) {
  if (midi < 0 || midi > 127) return;
  articulate(midi, velocity, false);
  keyDown_[midi] = sounding_[midi] = true;
  note_ = midi;
  blowing_ = true;
  // (the director's accent: a harder touch)
  m_.keyOn(midi, std::clamp(int(std::lround(velocity * hardness_)), 1, 127));
}

void PyrophonePlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  keyDown_[midi] = false;
  if (!hold_) {
    m_.keyOff(midi);
    sounding_[midi] = false;
  }
}

void PyrophonePlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int k = 0; k < 128; ++k)
      if (sounding_[k] && !keyDown_[k]) {
        m_.keyOff(k);
        sounding_[k] = false;
      }
}

bool PyrophonePlayer::setKnob(int k, double x) {
  x = std::clamp(x, 0.0, 1.0);
  switch (k) {
    case 0: m_.setWarmth(x); return true;  // K1: the warming (the swell)
    case 1: m_.setPuff(x); return true;    // K2: the puffing (the bubbling)
    case 2: m_.setRoar(x); return true;    // K3: the roar
    case 3: m_.setValve(x); return true;   // K4: the valve's closing (the release)
    default: return false;
  }
}

void PyrophonePlayer::render(float* out, int n) {
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
