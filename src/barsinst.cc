#include "barsinst.h"

#include <cmath>
#include <cstring>

#include "knob.h"

namespace pajupilli {

bool barInstrument(const Instrument& inst) {
  for (const char* n : {"celesta", "glockenspiel", "marimba", "xylophone", "tubular-bells", "music-box"})
    if (std::strcmp(inst.name, n) == 0) return true;
  return false;
}

BarDesign BarsModel::designOf(const Instrument& inst) {
  BarDesign d;
  if (!barDesignFor(inst.name, &d)) d = celestaDesign();
  return d;
}

BarsModel::BarsModel(const Instrument& inst) : Tube(inst), bars_(inst.fs, designOf(inst)) {}

void BarsModel::resetAir() {
  Tube::resetAir();
  bars_.reset();
}

float BarsModel::sample() {
  // (The microphones' pressure, Pa.)
  last_ = bars_.sample();
  side_.push_back(float(outScale()) * bars_.side());
  return finish(last_);
}

BarsPlayer::BarsPlayer(BarsModel& m) : Performer(m), m_(m) {}

int BarsPlayer::keyOf(int midi) const {
  const int lo = m_.bars().lowMidi(), hi = lo + m_.bars().keys() - 1;
  while (midi < lo) midi += 12;
  while (midi > hi) midi -= 12;
  return midi - lo;
}

void BarsPlayer::noteOn(int midi, int velocity) {
  const int key = keyOf(midi);
  articulate(midi, velocity, false);
  down_[key] = true;
  note_ = midi;
  blowing_ = true;
  const BarDesign& d = m_.bars().design();
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  const double speed = (d.speedLo + (d.speedHi - d.speedLo) * std::pow(v, 1.5)) * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0));
  m_.bars().damper(key, false);
  m_.bars().strike(key, speed);
}

void BarsPlayer::noteOff(int midi) {
  const int key = keyOf(midi);
  down_[key] = false;
  if (!hold_) m_.bars().damper(key, true);
}

void BarsPlayer::setHold(bool h) {
  hold_ = h;
  for (int k = 0; k < m_.bars().keys(); ++k)
    if (h) m_.bars().damper(k, false);
    else if (!down_[k]) m_.bars().damper(k, true);
}

void BarsPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.bars().asleep();
}

}  // namespace pajupilli
