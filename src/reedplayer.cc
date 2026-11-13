#include "reedplayer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "knob.h"

namespace pajupilli {

ReedPlayer::ReedPlayer(Reed& r) : Performer(r), reed_(r) {
  kDynP_ = knob("PJ_REEDDYNP", 0.7);    // breath: x2 from pp to ff
  kDynLip_ = knob("PJ_REEDDYNLIP", 0.3);  // louder: the lips give a little
  kVelDyn_ = knob("PJ_VELDYN", 0.35);
  kRollLip_ = knob("PJ_ROLLLIP", 0.5);  // pitch bend: lip force x e^(+-0.5)
  kP_ = knob("PJ_P", 1);
  air_ = knob("PJ_AIR", 0.35);
  wander_ = knob("PJ_WANDER", 1);
  // A reed player listens and tunes with the lips on every held note (the
  // reed's pitch moves with the breath and the attack far more than a
  // flute's); an ensemble's just intonation sets the target instead.
  ear_ = knob("PJ_REEDEAR", 1) > 0;
}

void ReedPlayer::seed(unsigned s) {
  rng_ = 4242u + 7907u * s;
  if (s == 0) return;
  for (int i = 0; i < 4; ++i) urand();
  vibRateMul_ = 1 + 0.08 * urand();
  vibDepthMul_ = 1 + 0.15 * urand();
  flutterRate_ = 27 + 4 * urand();
  vibPhase_ = M_PI * (1 + urand());
  flutterPhase_ = M_PI * (1 + urand());
}

void ReedPlayer::setHold(bool h) {
  hold_ = h;
  if (!h && stackTop_ == 0) blowing_ = false;
}

void ReedPlayer::noteOn(int midi, int velocity) {
  if (!info(midi)) return;
  held_[midi] = 1;
  if (stackTop_ < 16) stack_[stackTop_++] = midi;
  const bool legato = note_ >= 0 && blowing_ && (stackTop_ > 1 || (hold_ && velocity < 100));
  note_ = midi;
  keyVel_ = velocity;
  Attack a;
  if (legato) a = kSlur;
  else if (blowing_ || sinceOff_ < 0.08) a = kDu;
  else if (velocity < 35 * hardness_) a = kBreath;
  else if (velocity > 90 / std::max(0.3, hardness_)) a = kAccent;
  else a = kTu;
  velDynTarget_ = kVelDyn_ * (velocity - 64) / 63.0;
  if (a != kSlur) velDyn_ = velDynTarget_;
  const double over = std::max(0.0, (velocity - 50) / 77.0);
  velAccent_ = a == kAccent ? 1.2 * over : a == kTu ? 0.5 * over : 0;
  applyNote(a);
}

void ReedPlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  held_[midi] = 0;
  int w = 0;
  for (int r = 0; r < stackTop_; ++r)
    if (stack_[r] != midi) stack_[w++] = stack_[r];
  stackTop_ = w;
  if (midi != note_) return;
  if (stackTop_ > 0) {
    note_ = stack_[stackTop_ - 1];
    keyVel_ = 64;
    applyNote(kSlur);
  } else if (!hold_) {
    blowing_ = false;
    sinceOff_ = 0;
  }
}

void ReedPlayer::applyNote(Attack a) {
  // PJ_HOLES: any fingering (experiments).
  const char* ov = std::getenv("PJ_HOLES");
  reed_.setHoles(ov && *ov ? ov : info(note_)->holes, std::exp2((keyVel_ - 64) / 63.0));
  blowing_ = true;
  earCorr_ = 0;
  earClock_ = 0;
  lastCross_ = -1;
  switch (a) {
    case kSlur: return;
    case kBreath:  // the reed starts on the breath alone: a slow push, no tongue
      tongueClosed_ = 0;
      tongueRelease_ = 0.001;
      lungAttack_ = 0.040;
      break;
    case kDu:  // a light touch of the tongue on the reed
      tongueClosed_ = 0.002;
      tongueRelease_ = 0.006;
      lungAttack_ = 0.008;
      break;
    case kTu:
    case kAccent: {  // the tongue rests on the reed while the mouth fills, then leaves it
      const double v = keyVel_ / 127.0;
      tongueClosed_ = 0.006;
      tongueRelease_ = 0.002 + 0.010 * (1 - v);
      lungAttack_ = 0.006 * std::exp2((64 - keyVel_) / 40.0);
      break;
    }
  }
  tongueRelease_ *= 1 + 0.15 * wander_ * urand();
  lungAttack_ *= 1 + 0.15 * wander_ * urand();
  tongueClock_ = 0;
  sinceOn_ = 0;
}

void ReedPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  const ReedParams& rp = inst_.design->reed;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double bdt = m / fs_;
    if (ear_ && note_ >= 0) earClock_ += bdt;
    {  // wander: breath support (~1 Hz), lips (~2 Hz)
      auto lp2 = [&](double& a, double& b, double hz) {
        const double k = 1 - std::exp(-2 * M_PI * hz * bdt);
        a += k * (urand() * std::sqrt(1 / k) * 0.7 - a);
        b += k * (a - b);
      };
      lp2(w1a_, w1b_, 1.0);
      lp2(w2a_, w2b_, 2.0);
    }
    velDyn_ += (1 - std::exp(-bdt / 0.06)) * (velDynTarget_ - velDyn_);
    Embouchure e = reed_.embouchure();
    double lungTarget = 0;
    // Vibrato, fading in after 300 ms: the breath, or the jaw (rp.vib = 1).
    vibPhase_ += 2 * M_PI * vibRate_ * vibRateMul_ * (1 + 0.05 * wander_ * w2b_) * bdt;
    if (vibPhase_ > 2 * M_PI) vibPhase_ -= 2 * M_PI;
    // (A clarinet, rp.vib < 0, plays straight: classical clarinettists use none.)
    const double vib = rp.vib < 0 ? 0 : vibDepth_ * vibDepthMul_ * std::clamp((sinceOn_ - 0.3) / 0.4, 0.0, 1.0);
    const double jaw = std::max(0.0, rp.vib);
    const double sv = std::sin(vibPhase_);
    if (note_ >= 0) {
      const double dyn = std::clamp(dyn_ + breathCtl_ + velDyn_, dynLo[note_], std::max(dynLo[note_], dynHi[note_]));
      const double lo = noteFrequency(inst_.lo), hi = noteFrequency(inst_.hi);
      const double fw = noteFrequency(note_ - inst_.transpose);
      const double u = std::clamp(std::log(fw / lo) / std::log(hi / lo), 0.0, 1.0);
      const double p0 = rp.press * std::pow(rp.pressHi / rp.press, u);
      const double accent = 1 + 0.2 * velAccent_ * std::exp(-sinceOn_ / 0.05);
      if (blowing_)
        lungTarget = p0 * std::exp(kDynP_ * (dyn - 0.5)) * pressTrim[note_] * kP_ * pressScale_ * accent;
      // The lips: calibrated per note and dynamics; a little looser when loud;
      // the bend; the ear; the jaw vibrato.
      double lip = coverAt(note_, dyn) * std::exp(-kDynLip_ * (dyn - 0.5) + kRollLip_ * roll_);
      if (ear_) lip *= std::exp(earCorr_);
      lip *= 1 + 0.04 * wander_ * w2b_;
      lip *= 1 + 2 * jaw * vib * sv;
      e.reed = lip;
    }
    // Lungs: a fast push, a slower release.
    const double tc = lungTarget > lung_ ? lungAttack_ : release_;
    lung_ += (1 - std::exp(-bdt / tc)) * (lungTarget - lung_);
    tongueClock_ += bdt;
    sinceOn_ += bdt;
    sinceOff_ += bdt;
    e.tongue = tongueClock_ < tongueClosed_ ? 0 : std::min(1.0, (tongueClock_ - tongueClosed_) / tongueRelease_);
    if (flutter_ > 0) {  // flutter tongue (or a growl)
      flutterPhase_ += 2 * M_PI * flutterRate_ * bdt;
      if (flutterPhase_ > 2 * M_PI) flutterPhase_ -= 2 * M_PI;
      e.tongue *= 1 - flutter_ * 0.5 * (1 + std::sin(flutterPhase_));
    }
    e.lung = lung_ * (1 + (1 - jaw) * vib * sv) * (1 + 0.04 * wander_ * w1b_);
    e.glottis = 1.5e-4;
    e.noise = 0.01 * noiseCtl_;
    e.air = air_;
    reed_.setEmbouchure(e);
    reed_.process(out + i, m);
    if (ear_ && note_ >= 0) listen(out + i, m, earCorr_, earCents_, earClock_, 150, 0.05);
  }
}

}  // namespace pajupilli
