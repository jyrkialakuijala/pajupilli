#include "woodperc.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "instrument.h"
#include "knob.h"
#include "vmath.h"

namespace pajupilli {

namespace {

constexpr double kPi = 3.14159265358979323846;

// The free-free beam's roots (cos b cosh b = 1), then (n + 1/2) pi.
double freeRoot(int n) {
  static const double r[] = {4.7300407449, 7.8532046241, 10.9956078380, 14.1371654913, 17.2787596574};
  return n <= 5 ? r[n - 1] : (n + 0.5) * kPi;
}

// Its shape at u = x / L (0 .. 1), written so that it does not cancel for
// large b (as in metalperc.cc).
double freeShape(double b, double u) {
  const double den = 1 - std::exp(-2 * b) - 2 * std::sin(b) * std::exp(-b);
  const double oneMinusS = (std::cos(b) - std::sin(b) - std::exp(-b)) * 2 * std::exp(-b) / den;
  const double s = 1 - oneMinusS;
  const double grow = (std::cos(b) - std::sin(b) - std::exp(-b)) * std::exp(b * (u - 1)) / den;
  return grow + std::exp(-b * u) * (1 + s) / 2 + std::cos(b * u) - s * std::sin(b * u);
}

// The wood blocks, as measured on recorded ones (FluidR3's General MIDI 76
// and 77: research/clap_claves_surdo.md 3): the partials against the main one,
// their levels (dB) and T60s (s). The lip's main mode with its near
// neighbours (the cluster), the second near 2.2 x, a third near 3.6 x.
struct Block { double f0; double ratio[7]; double db[7]; double t60[7]; };
const Block kBlockData[2] = {
    {746, {1, 0.900, 0.937, 1.016, 2.196, 2.165, 3.6}, {0, -4, -8, -6, -3, -10, -18}, {0.55, 0.40, 0.30, 0.30, 0.20, 0.15, 0.05}},
    {502, {1, 1.118, 1.028, 1.044, 2.207, 2.450, 3.6}, {0, -5, -9, -8, -13, -12, -20}, {0.65, 0.40, 0.35, 0.35, 0.20, 0.18, 0.06}},
};

}  // namespace

bool woodPerc(const Instrument& inst) {
  return std::strcmp(inst.name, "claves") == 0 || std::strcmp(inst.name, "wood-blocks") == 0;
}

WoodPercModel::WoodPercModel(const Instrument& inst)
    : Tube(inst), kind_(std::strcmp(inst.name, "claves") == 0 ? kClaves : kBlocks), set_(inst.fs) {
  if (kind_ == kClaves) {
    // The held clave (0) and the striker (1): rosewood, 20 cm x 3 cm, 160 g
    // (research 2: 2540 - 2570 Hz measured on such a pair; the recorded GM
    // claves 2156 Hz). The held one 2.45 kHz, the striker 6 % lower.
    define(0, knob("PJ_CLAVES_HZ", 2450), 0);
    define(1, knob("PJ_CLAVES_HZ", 2450) * 0.94, 0);
    gain_ = knob("PJ_CLAVES_GAIN", 0.03);  // (a peak ~0.3 at velocity 100; the trim then small)
  } else {
    define(0, kBlockData[0].f0, 0);
    define(1, kBlockData[1].f0, 0);
    gain_ = knob("PJ_BLOCKS_GAIN", 0.18);  // (by the peak: ~0.8 at velocity 100)
  }
}

void WoodPercModel::define(int vib, double f0, double detune) {
  ModalSet::Def def;
  def.one = true;
  def.zShare = 0;
  uint32_t h = 2654435761u * (vib + 7);
  auto rnd = [&]() { h = h * 1664525u + 1013904223u; return ((h >> 8) & 0xffffff) / 16777216.0; };
  if (kind_ == kClaves) {
    // Free-free bending modes to 20 kHz (four: 1, 2.756, 5.404, 8.933 x).
    // The held clave rings longest in its first mode (held near its nodes),
    // the cupped hand under it radiating that mode about twice (a resonance
    // tuned to it); its upper modes short (the fingers on them). The striker,
    // held at an end, rings short throughout.
    def.modalMass = 0.16f;
    const double b1 = freeRoot(1);
    int n = 0;
    for (int m = 1; m <= 4; ++m) {
      const double f = f0 * (1 + detune) * freeRoot(m) * freeRoot(m) / (b1 * b1);
      if (f > 20000) break;
      def.freq[n] = float(f);
      const double t = vib == 0 ? (m == 1 ? 0.30 : 0.09 / m) : (m == 1 ? 0.12 : 0.04 / m);
      def.t60[n] = float(t * knob("PJ_CLAVES_T60", 1.0));
      def.rad[n] = float((vib == 0 && m == 1 ? 2.0 : 1.0) * (vib == 0 ? 1.0 : 2.5) / std::sqrt(double(m)));
      def.side[n] = float(0.3 * def.rad[n] * (rnd() < 0.5 ? 1 : -1));
      ++n;
    }
    def.n = n;
  } else {
    const Block& b = kBlockData[vib];
    def.modalMass = 0.05f;
    def.n = 7;
    for (int k = 0; k < 7; ++k) {
      def.freq[k] = float(b.f0 * (1 + detune) * b.ratio[k]);
      def.t60[k] = float(b.t60[k] * knob("PJ_BLOCKS_T60", 1.0));
      // The sound ~ f x rad x the shape at the stroke: rad to the recorded levels.
      def.rad[k] = float(fastPow(10.0, b.db[k] / 20) / b.ratio[k]);
      def.side[k] = float(0.3 * def.rad[k] * (rnd() < 0.5 ? 1 : -1));
    }
  }
  set_.setVib(vib, def);
  tuned_[vib] = detune;
}

void WoodPercModel::strike(int which, double speed, double where, double across) {
  if (kind_ == kClaves) {
    // The striker's middle on the held clave's middle (where: a little off,
    // so its second mode is heard a little): each bar by its shape there.
    for (int vib = 0; vib < 2; ++vib) {
      float shape[ModalSet::kModes] = {};
      const double u = vib == 0 ? 0.5 + 0.12 * (where - 0.5) : 0.62 + 0.1 * (where - 0.5);
      const ModalSet::Def& d = set_.def(vib);
      for (int k = 0; k < d.n; ++k) shape[k] = float(freeShape(freeRoot(k + 1), u));
      set_.setExcitation(vib, shape);
      // Wood on wood: a hard, short contact (~0.1 ms), half the other bar's
      // mass behind it.
      set_.strike(vib, 0.08, speed, knob("PJ_CLAVES_K", 5e9), 1.5);
    }
  } else {
    // The stick's tip on the top over the cavity, at (u along, v across) from
    // its middle (shares of the length and the width): each mode by its
    // shape there. The main mode has its antinode at the middle (a stroke
    // anywhere near it sounds it alike); its neighbours have nodal lines
    // running close by, one along the block just off its middle (the twist),
    // one across it (the lip's second bending), so a few mm sideways or
    // along change them: the same block, never quite the same stroke. The
    // upper modes, more nodal lines, change more.
    const int vib = std::clamp(which, 0, 1);
    const double u = where, v = across;
    const double sh[7] = {
        std::cos(kPi * u) * std::cos(0.3 * kPi * v),             // the main mode
        1.6 * std::cos(kPi * u) * std::sin(kPi * (v + 0.18)),    // its neighbour: a nodal line along, off the middle
        1.4 * std::sin(2 * kPi * (u + 0.12)),                    // a nodal line across
        std::cos(kPi * u) * std::cos(kPi * v),
        std::cos(3 * kPi * u),                                   // the second bending
        1.3 * std::sin(3 * kPi * (u + 0.06)) * std::cos(kPi * v),
        std::cos(5 * kPi * u),
    };
    float shape[ModalSet::kModes] = {};
    const ModalSet::Def& d = set_.def(vib);
    for (int k = 0; k < d.n && k < 7; ++k) shape[k] = float(sh[k]);
    set_.setExcitation(vib, shape);
    // The tip of a wooden stick (12 g behind it; hard on hard: ~0.1 ms).
    set_.strike(vib, knob("PJ_BLOCKS_TIP", 0.012), speed, knob("PJ_BLOCKS_K", 2e9), 1.5);
  }
}

float WoodPercModel::sample() {
  const float y = float(gain_) * set_.sample();
  side_.push_back(float(gain_ * outScale()) * set_.side());
  last_ = y;
  return finish(y);
}

void WoodPercModel::resetAir() {
  Tube::resetAir();
  set_.reset();
}

const float* WoodPercModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

WoodPercPlayer::WoodPercPlayer(WoodPercModel& m) : Performer(m), m_(m) {}

double WoodPercPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

double WoodPercPlayer::speed(int velocity, double dynamics) {
  // ~ velocity^1.6: General MIDI's ~30 dB from 16 to 127 (a contact this
  // stiff hardly brightens with speed).
  static const double top = knob("PJ_WOOD_VMAX", 3.0), ex = knob("PJ_WOOD_VEXP", 1.6);
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  return top * fastPow(v, ex) * (0.75 + 0.5 * std::clamp(dynamics, 0.0, 1.0));
}

void WoodPercPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  // 77 (and below 76): the low block; 76 and up: the high one. The claves: any key.
  const int which = m_.kind() == WoodPercModel::kBlocks && midi == 77 ? 1 : 0;
  if (m_.kind() == WoodPercModel::kBlocks) {
    // Roughly the same place on top, over the cavity: +-10 % of the length
    // along, +-10 % of the width across.
    static const double scatter = knob("PJ_BLOCKS_SCATTER", 0.1);
    m_.strike(which, speed(velocity, dyn_) * (1 + 0.05 * (ur() - 0.5)), scatter * (2 * ur() - 1), scatter * (2 * ur() - 1));
  } else {
    m_.strike(which, speed(velocity, dyn_) * (1 + 0.05 * (ur() - 0.5)), ur(), 0);
  }
}

void WoodPercPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
}

}  // namespace pajupilli
