#include "barsinst.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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

// The face for a stroke at this velocity: the hard one from PJ_BELLS_HARD_VEL (100) up, the
// soft one below PJ_BELLS_SOFT_VEL (80), between them the one in the hand; turned only after a
// gap of PJ_BELLS_TURN s (0.4). PJ_BELLS_FACE: 0 always the soft, 1 always the hard, -1 (the
// default) by the velocity.
int BarsPlayer::faceFor(int velocity) {
  static const double hardV = knob("PJ_BELLS_HARD_VEL", 100), softV = knob("PJ_BELLS_SOFT_VEL", 80);
  static const double turn = knob("PJ_BELLS_TURN", 0.4);
  static const int fixed = int(knob("PJ_BELLS_FACE", -1));
  if (fixed == 0 || fixed == 1) return fixed;
  const bool time = clock_ - lastStroke_ >= turn;
  if (time && velocity >= hardV) face_ = 1;
  else if (time && velocity < softV) face_ = 0;
  return face_;
}

// The band's pull on a stroke of key (research/tubular_bells.md 9): its own partials (the first
// of each doublet), their levels as the design has them under the soft face, the bells' tierce
// and the tubes' P3 lightly (their colour, not a thing to tune by).
double BarsPlayer::pull(int key, int midi, double* weight) const {
  *weight = 0;
  if (!band_) return 0;
  const BarKey& b = m_.bars().design().key[key];
  double pl[32], pw[32], pk[32];
  int np = 0;
  for (int m = 0; m < b.modes && np < 32; ++m) {
    if (b.rad[m] <= 0 || b.ratio[m] * b.f1 > 5000) continue;
    bool second = false;
    for (int j = 0; j < m; ++j) second |= b.pair[j] == m;
    if (second) continue;
    const double r = b.ratio[m], w = 2 * 3.14159265358979 * b.f1 * r;
    const bool colour = (r > 1.15 && r < 1.3);
    pl[np] = std::log2(r);
    pw[np] = b.rad[m] * b.hit[m] * w * (colour ? 0.3 : 1.0);
    pk[np] = std::max(1.0, r);
    ++np;
  }
  BandListener me;
  me.self = self_;
  me.f0 = 440 * std::exp2((midi - 69) / 12.0);
  me.pl = pl;
  me.pw = pw;
  me.pk = pk;
  me.np = np;
  me.hearOwn = true;  // (its own bells ringing: a melody's notes fit each other too)
  if (const BandNote* own = band_->find(self_, midi)) {
    me.at = own->at;
    me.pitch = own->pitch;
  } else {
    me.at = band_->now();
    me.pitch = midi;
  }
  return bandPull(*band_, me, weight);
}

void BarsPlayer::noteOn(int midi, int velocity) {
  const int key = keyOf(midi);
  const bool tubular = std::strcmp(m_.bars().design().name, "tubular-bells") == 0;
  if (tubular) {
    static const bool spectral = [] {
      const char* t = std::getenv("PJ_BELLS_TUNING");
      return !t || std::strcmp(t, "tet12") != 0;
    }();
    static const double lim = knob("PJ_BELLS_TUNE_LIM", 15);
    const int pm = m_.bars().lowMidi() + key;
    if (spectral && band_ && !m_.bars().ringing(key)) {
      double w = 0;
      const double c = pull(key, pm, &w);
      m_.bars().retune(key, w > 0 ? std::clamp(c, -lim, lim) : 0.0);
      static const bool log = std::getenv("PJ_BELLS_LOG") != nullptr;
      if (log) std::fprintf(stderr, "bells: key %d pull %+.1f c (weight %.3g) -> %+.1f c\n", pm, c, w, m_.bars().cents(key));
    }
    cents_[midi & 127] = m_.bars().cents(key);
  }
  // (the bells: a note let go dies out on the half pedal; the next stroke clears them all, the
  // dampers fully down on every key let go: the player moving on, not a smear)
  if (tubular && !hold_)
    for (int k = 0; k < m_.bars().keys(); ++k)
      if (!down_[k] && k != key && m_.bars().damperDepth(k) < 1) m_.bars().damper(k, true);
  articulate(midi, velocity, false);
  down_[key] = true;
  note_ = midi;
  blowing_ = true;
  const BarDesign& d = m_.bars().design();
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  const double speed = (d.speedLo + (d.speedHi - d.speedLo) * std::pow(v, 1.5)) * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0));
  m_.bars().damper(key, false);
  // (With the hard face the player strikes a little lighter: the change of face is heard as a
  // brighter, harder stroke more than a louder one.)
  static const double ease = knob("PJ_BELLS_HARD_EASE", 0.75);
  const int face = faceFor(velocity);
  m_.bars().strike(key, face == 1 && m_.bars().design().key[key].mallet2 > 0 ? speed * ease : speed, face);
  lastStroke_ = clock_;
}

void BarsPlayer::noteOff(int midi) {
  const int key = keyOf(midi);
  down_[key] = false;
  if (hold_) return;
  // The tubular bells: the player half-pedals a note let go, the felt only touching the tube, so
  // it dies out (PJ_BELLS_HALFPEDAL, 0.2 of the full damper: ~1.7 s to -60 dB on a tube's strike
  // note) unless the next stroke comes, which clears it (noteOn). 1: the full damper at once.
  static const float kHalf = float(knob("PJ_BELLS_HALFPEDAL", 0.2));
  const bool tubular = std::strcmp(m_.bars().design().name, "tubular-bells") == 0;
  m_.bars().damper(key, tubular ? kHalf : 1.0f);
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
  clock_ += n / fs_;
  blowing_ = !m_.bars().asleep();
}

}  // namespace pajupilli
