#include "surdo.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "instrument.h"
#include "knob.h"
#include "vmath.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kC = 343.0;
}  // namespace

bool surdo(const Instrument& inst) { return std::strcmp(inst.name, "surdo") == 0; }

TomSpec surdoSpec() {
  TomSpec s;
  const double R = 22 * 0.0254 / 2;
  s.note = knob("PJ_SURDO_HZ", 62);
  s.depth = knob("PJ_SURDO_DEPTH", 0.55);
  s.batter.radius = s.reso.radius = R;
  // Heavy heads (goatskin, or 14 mil plastic): 0.32 kg/m^2 both.
  s.batter.sigma = s.reso.sigma = knob("PJ_SURDO_SIGMA", 0.32);
  // Both at the same lug pitch, 1.75 x the note, as the toms' (the tension
  // is then solved for the note).
  const double lug = 1.75 * s.note;
  const double c = 2 * kPi * R * lug / 3.832;
  s.batter.tension = s.reso.tension = s.batter.sigma * c * c;
  // A big drum rings long (the recorded open surdo: T60 ~2.5 s).
  s.batter.zeta01 = s.reso.zeta01 = knob("PJ_SURDO_ZETA01", 0.006);
  s.batter.zeta0 = s.reso.zeta0 = knob("PJ_SURDO_ZETA0", 0.008);
  s.batter.zeta = s.reso.zeta = knob("PJ_SURDO_ZETA", 0.008);
  s.batter.lossKnee = s.reso.lossKnee = 1500;
  // A hard stroke stretches the heads: the "boom" falling in pitch.
  s.batter.stretch = knob("PJ_SURDO_STRETCH", 1.4e6);
  s.reso.stretch = 1.2e6;
  s.batter.seed = 91;
  s.reso.seed = 93;
  s.air.volume = kPi * R * R * s.depth;
  s.air.vent = knob("PJ_SURDO_VENT", 4);
  // The mallet: a felt head of ~0.15 kg, soft (F ~ K x^1.5, K 1.5e6: a
  // contact of a few ms).
  s.stickMass = knob("PJ_SURDO_MALLET", 0.15);
  s.stickK = knob("PJ_SURDO_K", 1.5e6);
  s.micX = 0.25;
  s.micZ = knob("PJ_SURDO_MICZ", 0.6);
  s.under = 0.6;
  s.click = knob("PJ_SURDO_CLICK", 0.15);
  s.tss = 0;
  s.gain = knob("PJ_SURDO_GAIN", 1);
  return s;
}

SurdoModel::SurdoModel(const Instrument& inst) : Tube(inst), drum_(inst.fs, surdoSpec()) {}

void SurdoModel::setHand(bool on) {
  if (on == hand_) return;
  hand_ = on;
  // The palm flat on the batter: its modes' loss many times over (the drum
  // is dead in ~0.2 s, the stroke a short dull thud).
  static const double mute = knob("PJ_SURDO_MUTE", 12);
  drum_.batter().setDamping(on ? mute : 1.0);
}

void SurdoModel::resetAir() {
  Tube::resetAir();
  drum_.reset();
}

const float* SurdoModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

float SurdoModel::sample() {
  static const float g = float(knob("PJ_SURDO_OUT", 1.0 / 8));
  // The room's sound at the drum, as at the toms (0.3 of it: the room model
  // is not passive at the low end; the far head a shell's depth later).
  static const float kRoom = float(knob("PJ_SURDO_ROOM", 0.3));
  if (roomPos_ < roomN_) {
    pRoom_ = room_[roomPos_++];
    if (delay_.empty()) delay_.assign(std::max(1, int(std::lround(drum_.spec().depth / kC * fs()))), 0.0f);
    const float far = delay_[delayPos_];
    delay_[delayPos_] = pRoom_;
    delayPos_ = (delayPos_ + 1) % int(delay_.size());
    drum_.setOutside(kRoom * pRoom_, kRoom * far);
  }
  last_ = g * drum_.sample();
  side_.push_back(g * float(outScale()) * drum_.side());
  return finish(last_);
}

SurdoPlayer::SurdoPlayer(SurdoModel& m) : Performer(m), m_(m) {}

double SurdoPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

double SurdoPlayer::malletSpeed(int velocity, double dynamics) {
  // ~ velocity^1.3 (the drum about linear in the mallet's speed; the felt
  // softer for a soft stroke: a little less than linear in level).
  static const double top = knob("PJ_SURDO_VMAX", 5.0), ex = knob("PJ_SURDO_VEXP", 1.3);
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  return top * fastPow(v, ex) * (0.75 + 0.5 * std::clamp(dynamics, 0.0, 1.0));
}

void SurdoPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  // 86 (and XG's 13): the hand on the head; 87 (and 14, any other): open.
  m_.setHand(midi == 86 || midi == 13);
  // Near the middle, a little elsewhere each time (a surdo is struck
  // a third of the way out for its fullest tone).
  const double r = 0.3 + 0.1 * (ur() - 0.5), th = 2 * kPi * ur();
  m_.drum().stick(malletSpeed(velocity, dyn_) * (1 + 0.04 * (ur() - 0.5)), r, th);
}

void SurdoPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.drum().asleep();
}

}  // namespace pajupilli
