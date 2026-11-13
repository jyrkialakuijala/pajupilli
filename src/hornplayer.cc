#include "hornplayer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "knob.h"

namespace pajupilli {

HornPlayer::HornPlayer(Horn& h) : Performer(h), horn_(h) {
  kDynP_ = knob("PJ_HORNDYNP", 1.2);     // breath: x3.3 from pp to ff
  kDynLip_ = knob("PJ_HORNDYNLIP", 0);    // louder: the lips' tension (the calibration sets it)
  kVelDyn_ = knob("PJ_VELDYN", 0.35);
  kRollLip_ = knob("PJ_ROLLLIP", 0.06);   // lip bend: the lips' resonance x e^(+-0.06)
  kP_ = knob("PJ_P", 1);
  air_ = knob("PJ_AIR", 0.35);
  wander_ = knob("PJ_WANDER", 1);
  ear_ = knob("PJ_LIPEAR", 1) > 0;
  kFl_ = knob("PJ_LIPFL", 1);  // the lips' resonance, x the design's ratio to the note
  vibRate_ = 5.0;
  kEarTau_ = knob("PJ_LIPEARTAU", 0.4);  // the ear's time constant (s)
  kSlurLip_ = knob("PJ_HORNSLURLIP", 0.06);  // the lips' overshoot on a slur across partials
  kSlurAir_ = knob("PJ_HORNSLURAIR", 0.2);   // and the air's push
  kSlurDip_ = knob("PJ_HORNSLURDIP", 0.04);  // then the lips come up from below the note ...
  kSlurDipT_ = knob("PJ_HORNSLURDIPT", 0.08);  // ... over this (s)
  kSlurTongue_ = knob("PJ_HORNSLURTONGUE", 0.002);  // a slur across partials: the tongue's touch (s)  // a brass player tunes every held note with the lips
}

void HornPlayer::seed(unsigned s) {
  rng_ = 4242u + 7907u * s;
  if (s == 0) return;
  for (int i = 0; i < 4; ++i) urand();
  vibRateMul_ = 1 + 0.08 * urand();
  vibDepthMul_ = 1 + 0.15 * urand();
  flutterRate_ = 27 + 4 * urand();
  vibPhase_ = M_PI * (1 + urand());
  flutterPhase_ = M_PI * (1 + urand());
}

void HornPlayer::setHold(bool h) {
  hold_ = h;
  if (!h && stackTop_ == 0) blowing_ = false;
}

void HornPlayer::noteOn(int midi, int velocity) {
  if (!info(midi)) return;
  held_[midi] = 1;
  if (stackTop_ < 16) stack_[stackTop_++] = midi;
  const bool legato = note_ >= 0 && blowing_ && (stackTop_ > 1 || (hold_ && velocity < 100));
  note_ = midi;
  keyVel_ = velocity;
  Attack a;
  if (legato) a = kSlur;
  else if (blowing_ || sinceOff_ < 0.08) a = kDa;
  else if (velocity < 35 * hardness_) a = kBreath;
  else if (velocity > 90 / std::max(0.3, hardness_)) a = kAccent;
  else a = kTa;
  velDynTarget_ = kVelDyn_ * (velocity - 64) / 63.0;
  if (a != kSlur) velDyn_ = velDynTarget_;
  const double over = std::max(0.0, (velocity - 50) / 77.0);
  velAccent_ = a == kAccent ? 1.2 * over : a == kTa ? 0.5 * over : 0;
  applyNote(a);
}

void HornPlayer::noteOff(int midi) {
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

void HornPlayer::applyNote(Attack a) {
  // PJ_HOLES: any valve combination (experiments).
  const char* ov = std::getenv("PJ_HOLES");
  horn_.setHoles(ov && *ov ? ov : played(note_)->holes, std::exp2((keyVel_ - 64) / 63.0));
  blowing_ = true;
  earCorr_ = 0;
  earClock_ = 0;
  heard_ = 0;
  lastCross_ = -1;
  if (a == kSlur && prevNote_ >= 0 && prevNote_ != note_) {
    // Across partials (or valves), the lips lead into the new note.
    const double dir = note_ > prevNote_ ? 1 : -1;
    slurLip_ = kSlurLip_ * dir;
    slurAir_ = kSlurAir_;
    sinceSlur_ = 0;
  }
  prevNote_ = note_;
  // A slur across partials: a light touch of the tongue ("legato tongue";
  // without it the new note can settle on the sharp side of a jump in its
  // slot, a stable state 30..50 cents sharp, kept from the old oscillation).
  if (a == kSlur && kSlurTongue_ > 0 && prevPartial_ >= 0 && played(note_)->harmonic != prevPartial_) {
    tongueClosed_ = kSlurTongue_;
    tongueRelease_ = 0.003;
    tongueClock_ = 0;
  }
  prevPartial_ = played(note_)->harmonic;
  switch (a) {
    case kSlur: return;
    case kBreath:  // the lips start on the breath alone
      tongueClosed_ = 0;
      tongueRelease_ = 0.001;
      lungAttack_ = 0.040;
      break;
    case kDa:  // a light touch of the tongue
      tongueClosed_ = 0.003;
      tongueRelease_ = 0.004;
      lungAttack_ = 0.008;
      break;
    case kTa:
    case kAccent: {  // the tongue holds the air while the mouth fills, then lets go
      const double v = keyVel_ / 127.0;
      tongueClosed_ = 0.010;
      tongueRelease_ = 0.002 + 0.006 * (1 - v);
      lungAttack_ = 0.008 * std::exp2((64 - keyVel_) / 40.0);
      break;
    }
  }
  tongueRelease_ *= 1 + 0.15 * wander_ * urand();
  lungAttack_ *= 1 + 0.15 * wander_ * urand();
  tongueClock_ = 0;
  sinceOn_ = 0;
}

void HornPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  const LipParams& lp = *inst_.design->lip;
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
    Embouchure e = horn_.embouchure();
    double lungTarget = 0;
    // Vibrato (from the breath), fading in after 300 ms; a horn player seldom uses it.
    vibPhase_ += 2 * M_PI * vibRate_ * vibRateMul_ * (1 + 0.05 * wander_ * w2b_) * bdt;
    if (vibPhase_ > 2 * M_PI) vibPhase_ -= 2 * M_PI;
    const double vib = vibDepth_ * vibDepthMul_ * std::clamp((sinceOn_ - 0.3) / 0.4, 0.0, 1.0);
    const double sv = std::sin(vibPhase_);
    if (note_ >= 0) {
      const double dyn = std::clamp(dyn_ + breathCtl_ + velDyn_, dynLo[note_], std::max(dynLo[note_], dynHi[note_]));
      const double f = noteFrequency(note_);
      // ~ f^1.45 (the calibration's choice: x0.65 at F2, x1.4 at A4 of ~ f).
      const double p0 = std::clamp(lp.k * f * std::pow(f / 261.6, 0.45), lp.pressLo, lp.pressHi);
      const double accent = 1 + 0.25 * velAccent_ * std::exp(-sinceOn_ / 0.05);
      const double slur = std::exp(-sinceSlur_ / 0.03);
      if (blowing_)
        lungTarget = p0 * std::exp(kDynP_ * (dyn - 0.5)) * pressTrim[note_] * kP_ * pressScale_ * accent *
                     (1 + slurAir_ * slur);
      // The lips: calibrated per note and dynamics; the bend; the ear.
      double fl = f * lp.fl * kFl_ * coverAt(note_, dyn) * std::exp(-kDynLip_ * (dyn - 0.5) + kRollLip_ * roll_);
      if (ear_) fl *= std::exp(earCorr_);
      fl *= 1 + 0.004 * wander_ * w2b_;
      // ... then they come into the note from a little below: above, the
      // oscillation can stay on the sharp side of a jump in the slot.
      fl *= std::exp(slurLip_ * slur - kSlurDip_ * (std::exp(-sinceSlur_ / kSlurDipT_) - slur) * (sinceSlur_ < 1e8));
      e.reed = fl;
    }
    // Lungs: a fast push, a slower release.
    const double tc = lungTarget > lung_ ? lungAttack_ : release_;
    lung_ += (1 - std::exp(-bdt / tc)) * (lungTarget - lung_);
    tongueClock_ += bdt;
    sinceOn_ += bdt;
    sinceOff_ += bdt;
    sinceSlur_ += bdt;
    e.tongue = tongueClock_ < tongueClosed_ ? 0 : std::min(1.0, (tongueClock_ - tongueClosed_) / tongueRelease_);
    if (flutter_ > 0) {  // flutter tongue (or a growl)
      flutterPhase_ += 2 * M_PI * flutterRate_ * bdt;
      if (flutterPhase_ > 2 * M_PI) flutterPhase_ -= 2 * M_PI;
      e.tongue *= 1 - flutter_ * 0.5 * (1 + std::sin(flutterPhase_));
    }
    e.lung = lung_ * (1 + vib * sv) * (1 + 0.03 * wander_ * w1b_);
    e.glottis = 1.5e-4;
    e.noise = 0.01 * noiseCtl_;
    e.air = air_;
    horn_.setEmbouchure(e);
    horn_.process(out + i, m);
    // The ear: once the note has spoken, a slow trim (a fast one hunts: the
    // horn's pitch answers the lips only after the long bore has settled),
    // errors over 5 cents only, and by 0.8 % of the lips' tension at most: the
    // calibration keeps 2 % from where the note would flip or crack (and
    // sustained, a note flips sooner than from silence).
    if (ear_ && note_ >= 0 && earClock_ > 0.2) {
      listen(out + i, m, earCorr_, earCents_, earClock_, 600, kEarTau_, 5);
      earCorr_ = std::clamp(earCorr_, -0.008, 0.008);
    }
  }
}

}  // namespace pajupilli
