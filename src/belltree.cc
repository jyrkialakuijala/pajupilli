#include "belltree.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "instrument.h"
#include "knob.h"
#include "vmath.h"

namespace pajupilli {

bool bellTree(const Instrument& inst) { return std::strcmp(inst.name, "bell-tree") == 0; }

BellTreeModel::BellTreeModel(const Instrument& inst) : Tube(inst), set_(inst.fs) {
  // Sixteen bells, from the top (the smallest: 11 kHz) to the bottom (the
  // largest: 4 kHz), evenly in log; each its first three cup modes (1, 1.62,
  // 2.43 x), the higher ones a little shorter; brass rings long (T60 2.5 s at
  // the bottom, 1.4 s at the top).
  static const double lo = knob("PJ_BELLTREE_LO", 4000), hi = knob("PJ_BELLTREE_HI", 11000);
  const double ratio[3] = {1, 1.62, 2.43}, level[3] = {1, 0.6, 0.35};
  uint32_t h = 4242;
  for (int b = 0; b < kBells; ++b) {
    ModalSet::Def def;
    def.one = true;
    def.zShare = 0;
    def.modalMass = float(0.004 * fastPow(hi / lo, -1.0 + b / double(kBells - 1)));  // (smaller bells: lighter)
    const double f0 = hi * fastPow(lo / hi, b / double(kBells - 1));  // b = 0: the top
    int n = 0;
    for (int k = 0; k < 3; ++k) {
      h = h * 1664525u + 1013904223u;
      const double f = f0 * ratio[k] * (1 + 0.01 * (((h >> 8) & 0xffff) / 65536.0 - 0.5));
      if (f > 19500) break;
      def.freq[n] = float(f);
      def.t60[n] = float((1.4 + 1.1 * b / double(kBells - 1)) / (1 + 0.3 * k) * knob("PJ_BELLTREE_T60", 1.0));
      def.exc[n] = 1;
      def.rad[n] = float(level[k] / ratio[k]);
      def.side[n] = float(0.6 * def.rad[n] * (((h >> 9) & 1) ? 1 : -1) * (b % 2 ? 1 : -1));
      ++n;
    }
    def.n = n;
    set_.setVib(b, def);
  }
  gain_ = knob("PJ_BELLTREE_GAIN", 0.005);  // (a peak ~0.3 at velocity 100)
}

void BellTreeModel::sweep(double speed, double dur) {
  next_ = 0;
  at_ = 0;
  step_ = std::max(dur, 0.05) * fs_ / kBells;
  speed_ = speed;
}

float BellTreeModel::sample() {
  // The beater reaching the next bell: struck (brass on brass, ~0.05 ms),
  // each a little harder or softer as the hand slides.
  while (next_ < kBells && at_ >= next_ * step_) {
    rng_ = rng_ * 1664525u + 1013904223u;
    const double jit = 0.8 + 0.4 * ((rng_ >> 8) * (1.0 / 16777216.0));
    set_.strike(next_, 0.003, speed_ * jit, 1e10, 1.5);  // (the beater lighter than the bells)
    ++next_;
  }
  at_ += 1;
  const float y = float(gain_) * set_.sample();
  side_.push_back(float(gain_ * outScale()) * set_.side());
  last_ = y;
  return finish(y);
}

void BellTreeModel::resetAir() {
  Tube::resetAir();
  set_.reset();
  next_ = kBells;
}

const float* BellTreeModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

BellTreePlayer::BellTreePlayer(BellTreeModel& m) : Performer(m), m_(m) {}

void BellTreePlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  // The beater's speed ~ velocity^1.6 (General MIDI's ~30 dB); the sweep
  // quicker the harder it is played: 0.5 s soft, 0.25 s hard.
  static const double top = knob("PJ_BELLTREE_VMAX", 2.0), ex = knob("PJ_BELLTREE_VEXP", 1.6);
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  const double speed = top * fastPow(v, ex) * (0.75 + 0.5 * std::clamp(dyn_, 0.0, 1.0));
  m_.sweep(speed, 0.5 - 0.25 * v);
}

void BellTreePlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
}

}  // namespace pajupilli
