#include "pianoinst.h"

#include <cmath>
#include <cstring>

#include "knob.h"

namespace pajupilli {

namespace {
constexpr int kLowest = 21;  // A0
}

// The upright (src/upright.cc, when it is built in).
PianoDesign uprightDesign() __attribute__((weak));

static bool upright(const Instrument& inst) { return std::strcmp(inst.name, "upright-piano") == 0; }

bool grandPiano(const Instrument& inst) { return std::strcmp(inst.name, "grand-piano") == 0 || upright(inst); }

static PianoDesign designFor(const Instrument& inst) {
  return upright(inst) && uprightDesign ? uprightDesign() : grandDesign();
}

PianoModel::PianoModel(const Instrument& inst) : Tube(inst), piano_(inst.fs, designFor(inst)) {}

void PianoModel::resetAir() {
  Tube::resetAir();
  piano_.reset();
}

float PianoModel::sample() {
  // (The microphones' pressure, Pa at 0.65 m over the strings: a mezzo-forte
  // middle C peaks near 1 Pa.)
  static const float g = float(knob("PJ_PIANO_GAIN", 1.0 / 6));
  last_ = g * piano_.sample(float(testDp_));  // (tests: a force on the bridge at middle C)
  testDp_ = 0;
  side_.push_back(g * float(outScale()) * piano_.side());
  return finish(last_);
}

PianoPlayer::PianoPlayer(PianoModel& m) : Performer(m), m_(m) {
  // (PJ_PIANO_PEDAL: the sustain pedal held from the start: every string free, the most work.)
  if (knob("PJ_PIANO_PEDAL", 0) > 0) setHold(true);
}

double PianoPlayer::hammerSpeed(int velocity, double dynamics) {
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  return (0.35 + 5.0 * std::pow(v, 1.7)) * (0.7 + 0.6 * std::clamp(dynamics, 0.0, 1.0));
}

void PianoPlayer::noteOn(int midi, int velocity) {
  const int key = midi - kLowest;
  if (key < 0 || key >= Piano::kKeys) return;
  articulate(midi, velocity, false);
  down_[key] = true;
  note_ = midi;
  blowing_ = true;
  m_.piano().damper(key, false);
  m_.piano().strike(key, hammerSpeed(velocity, dyn_));
}

void PianoPlayer::noteOff(int midi) {
  const int key = midi - kLowest;
  if (key < 0 || key >= Piano::kKeys) return;
  down_[key] = false;
  if (!hold_) m_.piano().damper(key, true);
}

void PianoPlayer::setHold(bool h) {
  hold_ = h;
  for (int k = 0; k < Piano::kKeys; ++k)
    if (h) m_.piano().damper(k, false);
    else if (!down_[k]) m_.piano().damper(k, true);
}

void PianoPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.piano().setVoicing(std::pow(4.0, brightness_ - 1) * hardness_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.piano().asleep();
}

}  // namespace pajupilli
