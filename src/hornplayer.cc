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
  kSlurLip_ = knob("PJ_HORNSLURLIP", 0.3);  // the lips' overshoot on a slur across partials, x the gap to the next one
  kSlurAir_ = knob("PJ_HORNSLURAIR", 0.5);   // and the air's push (lifts the new partial off the old one)
  kSlurDip_ = knob("PJ_HORNSLURDIP", 0.18);  // then the lips come up from below the note (x the gap) ...
  kSlurAirDown_ = knob("PJ_HORNSLURAIRDOWN", 0.3);  // ... on a downward slur
  kSlurDipUp_ = knob("PJ_HORNSLURDIPUP", 0);  // ... on an upward slur
  kSlurDipT_ = knob("PJ_HORNSLURDIPT", 0.08);  // ... over this (s)
  kSlurTongue_ = knob("PJ_HORNSLURTONGUE", 0.002);  // a slur across partials: the tongue's touch (s)
  // The attack: a push of breath that settles (the lips' oscillation grows far
  // faster above the steady pressure: C4 ~120 ms to speak without it, ~55 with).
  kAtkP_ = knob("PJ_HORNATKP", 1.5);    // the push, x the note's pressure
  kAtkT_ = knob("PJ_HORNATKT", 0.08);   // its decay (s)
  // ... only to the low partials (higher up it is a hole in the legato, and
  // the slots there hold the note without it).
  kSlurTongueMax_ = knob("PJ_HORNSLURTONGUEMAX", 99);
  // ... and how far it lets the air through (0: it stops it; partly open, the
  // old note is not cut off).
  kSlurTongueFloor_ = knob("PJ_HORNSLURTONGUEFLOOR", 0);
  kLegatoGap_ = knob("PJ_HORNLEGATO", 0.04);  // a key let go: the air stays this long for a slur (s)
}

void HornPlayer::seed(unsigned s) {
  rng_ = 4242u + 7907u * s;
  vib_.seed(s);
  if (s == 0) return;
  for (int i = 0; i < 4; ++i) urand();
  vib_.rateMul = 1 + 0.08 * urand();
  vib_.depthMul = 1 + 0.15 * urand();
  flutterRate_ = 27 + 4 * urand();
  vib_.setPhase(M_PI * (1 + urand()));
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
  // A key let go a moment ago, the air still on: a keyboard's legato (the
  // fingers seldom overlap the keys), slurred from the old note.
  const bool caught = pendingOff_ > 0;
  pendingOff_ = 0;
  const bool legato = note_ >= 0 && blowing_ && (stackTop_ > 1 || hold_ || caught);
  const Articulation art = articulate(midi, velocity, legato);
  note_ = midi;
  keyVel_ = velocity;
  const Attack a = art == kArtSlur || art == kArtGlide ? kSlur
                   : art == kArtTongueLegato || art == kArtDu ? kDa
                   : art == kArtBreath ? kBreath
                   : art == kArtAccent ? kAccent : kTa;
  // (A glide's soft key is its sign, not a diminuendo.)
  if (art != kArtGlide) velDynTarget_ = kVelDyn_ * (velocity - 64) / 63.0;
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
    // A held note: the air stays on a moment for a legato next key; a short
    // one stops at once (staccato).
    if (kLegatoGap_ > 0 && sinceOn_ > 0.15) pendingOff_ = kLegatoGap_;
    else stopBlowing();
  }
}

void HornPlayer::stopBlowing() {
  pendingOff_ = 0;
  blowing_ = false;
  sinceOff_ = 0;
  released();
}

void HornPlayer::applyNote(Attack a) {
  // PJ_HOLES: any valve combination (experiments).
  const char* ov = std::getenv("PJ_HOLES");
  const bool glide = gliding();
  horn_.setGlide(takeHoleGlide());
  horn_.setHoles(ov && *ov ? ov : played(note_)->holes, std::exp2((keyVel_ - 64) / 63.0));
  blowing_ = true;
  if (!gliding()) earCorr_ = 0;
  earClock_ = 0;
  heard_ = 0;
  lastCross_ = -1;
  if (a == kSlur && !glide && prevNote_ >= 0 && prevNote_ != note_) {
    // Across partials, the lips lead into the new note: an overshoot, a share
    // of the gap to the next partial beyond it (high up the partials lie a
    // semitone apart: a fixed overshoot lands on the neighbour). On the same
    // partial (valves only) the lips just follow the note.
    const int pn = played(note_)->harmonic, pp = prevPartial_ >= 0 ? prevPartial_ : pn;
    const double dir = note_ > prevNote_ ? 1 : -1;
    if (pn != pp) {
      const double gap = dir > 0 ? std::log((pn + 1.0) / pn) : std::log(pn / std::max(1.0, pn - 1.0));
      slurLip_ = kSlurLip_ * gap * dir;
      // Down, the lips come into the note from below (from above it can stay
      // on the sharp side of a jump in the slot); up, from below they can stay
      // on the old partial, bent flat: there they settle from the overshoot.
      slurDip_ = (dir < 0 ? kSlurDip_ : kSlurDipUp_) * gap;
    } else {
      slurLip_ = slurDip_ = 0;
    }
    // Up, the air lifts the note off the old partial; down, a full push can
    // drive a low note into a flat regime of its slot: the player eases off
    // (but without any push the legato has holes).
    slurAir_ = dir > 0 ? kSlurAir_ : kSlurAirDown_;
    sinceSlur_ = 0;
  }
  prevNote_ = note_;
  // A slur to a low partial: a light touch of the tongue ("legato tongue";
  // without it the new note can settle on the sharp side of a jump in its
  // slot, a stable state 30..50 cents sharp, kept from the old oscillation).
  if (a == kSlur && !glide && kSlurTongue_ > 0 && prevPartial_ >= 0 && played(note_)->harmonic != prevPartial_ &&
      played(note_)->harmonic <= kSlurTongueMax_) {
    tongueClosed_ = kSlurTongue_;
    tongueRelease_ = 0.003;
    tongueFloor_ = kSlurTongueFloor_;
    tongueClock_ = 0;
  }
  prevPartial_ = played(note_)->harmonic;
  if (a != kSlur) tongueFloor_ = 0;
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
  // The push: less on a breath attack, more on an accent.
  atkPush_ = kAtkP_ * (a == kBreath ? 0.5 : a == kDa ? 0.8 : 1.0);
  tongueClock_ = 0;
  sinceOn_ = 0;
}

void HornPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  const LipParams& lp = *inst_.design->lip;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double bdt = m / fs_;
    if (pendingOff_ > 0 && (pendingOff_ -= bdt) <= 0) stopBlowing();
    tick(bdt);
    if (gliding()) glideEar();
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
    // (performer.h: Vibrato, a rotating complex number, breathing.)
    const double sv = vib_.step(bdt, vibRate_ * (1 + 0.05 * wander_ * w2b_), sinceOn_,
                                std::clamp(dyn_ + breathCtl_, 0.0, 1.0), wander_);
    const double vib = vibDepth_ * vib_.depthMul * vib_.env();
    if (note_ >= 0) {
      const double dyn = std::clamp(dyn_ + breathCtl_ + velDyn_, dynLo[note_], std::max(dynLo[note_], dynHi[note_]));
      const double f = glideFrequency();  // (a glide: the lips slide through the partials)
      // ~ f^1.45 (the calibration's choice: x0.65 at F2, x1.4 at A4 of ~ f).
      const double p0 = std::clamp(lp.k * f * std::pow(f / 261.6, 0.45), lp.pressLo, lp.pressHi);
      // The push brings the breath up to a speaking pressure: a loud note's
      // own is most of the way there (pushed as hard, it overblows).
      const double push = std::clamp((1 + atkPush_) * std::exp(-kDynP_ * (dyn - 0.5)) - 1, 0.0, atkPush_);
      const double accent = 1 + (0.25 * velAccent_ + push) * std::exp(-sinceOn_ / kAtkT_);
      const double slur = std::exp(-sinceSlur_ / 0.03);
      if (blowing_)
        lungTarget = p0 * std::exp(kDynP_ * (dyn - 0.5)) * pressTrim[note_] * kP_ * pressScale_ * accent *
                     (1 + slurAir_ * slur);
      // The lips: calibrated per note and dynamics; the bend; the ear.
      double fl = f * lp.fl * kFl_ * coverAt(note_, dyn) * glideCover(note_, dyn) * std::exp(-kDynLip_ * (dyn - 0.5) + kRollLip_ * roll_);
      if (ear_) fl *= std::exp(earCorr_);
      fl *= 1 + 0.004 * wander_ * w2b_;
      // ... then they come into the note from a little below: above, the
      // oscillation can stay on the sharp side of a jump in the slot.
      fl *= std::exp(slurLip_ * slur - slurDip_ * (std::exp(-sinceSlur_ / kSlurDipT_) - slur) * (sinceSlur_ < 1e8));
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
    e.tongue = tongueFloor_ + (1 - tongueFloor_) * e.tongue;
    e.tongue *= tongueStop();
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
