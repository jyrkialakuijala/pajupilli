#include "hornplayer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "knob.h"
#include "slurs.h"
#include "vmath.h"

namespace pajupilli {

HornPlayer::HornPlayer(Horn& h) : Performer(h), horn_(h) {
  kDynP_ = knob("PJ_HORNDYNP", 1.2);     // breath: x3.3 from pp to ff (or the design's dynP)
  if (inst_.design && inst_.design->lip && inst_.design->lip->dynP > 0 && !std::getenv("PJ_HORNDYNP")) kDynP_ = inst_.design->lip->dynP;
  kDynLip_ = knob("PJ_HORNDYNLIP", 0);
  kDynH_ = knob("PJ_LIPDYNH", 1);       // the lips pressed together when loud, x the design's    // louder: the lips' tension (the calibration sets it)
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
  // Soft playing (a horn teacher's way: "keep air speed high, reduce aperture size"; "bring the
  // lips together gently"): below mezzo-forte the breath falls by less (this share of its fall
  // kept back) and the lips come together instead (the rest opening x e^(-this (0.5 - dyn))).
  // The horn (research/slurs.md 10): lips 2, the breath as before: 27 of its 37 notes then play
  // softly (12 without), pianissimo 5 dB under mezzo-forte (1.3 without). Keeping the breath up
  // too starts the soft notes sooner (109 ms for 135) but leaves half the range. The other
  // brass: not tried yet (0).
  kSoftAir_ = knob("PJ_HORNSOFTAIR", 0);
  kSoftLips_ = knob("PJ_HORNSOFTLIPS", std::strcmp(inst_.name, "horn") == 0 ? 2 : 0);
  // The bell turned with the dynamics (the owner: "turning the horn down when playing pianissimo
  // and flaring it towards the listener when playing forte"; research/horns.md 5.3: above
  // ka ~ 1 the bell beams, a horn's from ~400 Hz). The sound above PJ_BELLTURN_HZ by
  // PJ_BELLTURN_SOFT dB at pp .. 0 at mezzo-forte .. PJ_BELLTURN_LOUD dB at ff, the instrument
  // moving over ~0.15 s. By the dynamics asked for, not the note's calibrated range: a note
  // the lips cannot play louder still has its bell. PJ_BELLTURN=0: the bell stays.
  kBellTurn_ = knob("PJ_BELLTURN", std::strcmp(inst_.name, "horn") == 0 ? 1 : 0) > 0;
  kBellSoft_ = knob("PJ_BELLTURN_SOFT", -12);
  kBellLoud_ = knob("PJ_BELLTURN_LOUD", 12);
  bellK_ = 1 - std::exp(-2 * M_PI * knob("PJ_BELLTURN_HZ", 400) / fs_);
  kLegatoGap_ = knob("PJ_HORNLEGATO", 0.04);
  kSlideTongue_ = knob("PJ_SLIDETONGUE", 0.02);  // a trombone's legato tongue: the air dipped this long (s)  // a key let go: the air stays this long for a slur (s)
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

// A section's player (research/brass_section.md 11.3), fixed for its seat: its mean pitch spread
// over +-4 c (sd 2.4 c) (the GM section sample's players 2-4 c apart; PJ_BRASS_CENTS), the attack's
// push +-15 % and its decay +-20 %.
void HornPlayer::vary(unsigned seat) {
  static const double kCents = knob("PJ_BRASS_CENTS", 4);
  uint64_t r = 0x9e3779b97f4a7c15ull * (seat + 1) + 0x1234567ull;
  auto u = [&]() {  // (splitmix64: the seats' draws independent)
    uint64_t z = (r += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return double((z ^ (z >> 31)) >> 11) * (2.0 / 9007199254740992.0) - 1;
  };
  section_ = true;
  // (stratified, not drawn: five players drawn at random can all land within a cent or two)
  static const double kSpread[10] = {0.7, -0.8, 0.2, -0.35, 0.95, -0.55, 0.45, -0.95, 0.05, -0.15};
  seatCents_ = kCents * kSpread[(seat + 9) % 10];
  kAtkP_ *= 1 + 0.15 * u();
  kAtkT_ *= 1 + 0.2 * u();
  // (a section's stab is played from the chest: velocity spans more of the breath, so the loud
  // keys reach the brassy ff the sampled sections have; tools/test.py checks the 4 kHz band)
  static const double kVelDyn = knob("PJ_BRASS_VELDYN", 1.6);
  kVelDyn_ *= kVelDyn;
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
  // A key softer (or harder) than the note can be played: the dynamics stop at the note's
  // calibrated range, and the attack with them (reedplayer.cc). A soft key's slow breath on a
  // low note that only speaks from mezzo-forte let a higher partial start first, and it stayed
  // (the tuba's F#1..B1 from a key of 30: a twelfth and more up).
  if (art != kArtGlide && kVelDyn_ > 0) {
    bellVel_ = kVelDyn_ * (velocity - 64) / 63.0;  // (the key as asked: the bell is not bound by the lips' range)
    const double want = dyn_ + breathCtl_ + kVelDyn_ * (velocity - 64) / 63.0;
    const double got = std::clamp(want, dynLo[midi], std::max(dynLo[midi], dynHi[midi]));
    const int v = int(std::lround(velocity - 63 * (want - got) / kVelDyn_));
    velocity = want > got ? std::clamp(v, std::min(velocity, 64), velocity)
                          : std::clamp(v, velocity, std::max(velocity, 64));
  }
  keyVel_ = velocity;
  // (... and a breath attack is a soft key's: a key brought up to the note's softest dynamics
  // is tongued, as the calibration played it there.)
  const bool breath = art == kArtBreath && velocity < 35 * hardness_;
  const Attack a = art == kArtSlur || art == kArtGlide ? kSlur
                   : art == kArtTongueLegato || art == kArtDu ? kDa
                   : breath ? kBreath
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
  const bool rested = !blowing_;
  // What the player has practised for this slur (src/slurs.h): none (an instrument that has
  // not practised): the old way below. A slur found unplayable is tongued lightly.
  SlurGesture g;
  const bool slurring = a == kSlur && !glide && prevNote_ >= 0 && prevNote_ != note_;
  const bool trill = trillSense_.at(prevNote_, note_, sinceOn_);
  const bool practised = slurring && slurGesture(inst_.name, prevNote_, note_, std::clamp(dyn_ + breathCtl_ + velDynTarget_, 0.0, 1.0), &g, sinceOn_, trill);
  if (practised && g.fails) a = kDa;
  const char* holes = ov && *ov ? ov : played(note_)->holes;
  // (a trombone: the gesture's number is the slide's own speed: its time x 1 / fingers, where the
  // horn takes the square root of what it is given; slower can be steadier, the note arriving
  // with the tube)
  const double fingerSpeed = std::exp2((keyVel_ - 64) / 63.0) *
                             (practised ? (inst_.design->lip->slide ? g.fingers * g.fingers : g.fingers) : 1);
  leadLeft_ = 0;
  if (practised && !g.fails && g.lead > 0) {  // the lips first: the valves follow
    leadLeft_ = g.lead;
    leadHoles_ = holes;
    leadSpeed_ = fingerSpeed;
  } else {
    horn_.setHoles(holes, fingerSpeed);
  }
  if (rested) horn_.slideAhead(sinceOff_);  // the arm moved in the rest
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
    slurLipT_ = slurAirT_ = 0.03;
    slurDipT_ = kSlurDipT_;
    if (practised) {
      const double gap = pn != pp ? (dir > 0 ? std::log((pn + 1.0) / pn) : std::log(pn / std::max(1.0, pn - 1.0)))
                                  : std::fabs(note_ - prevNote_) * (M_LN2 / 12);
      slurLip_ = g.lip * gap * dir;
      slurDip_ = g.dip * gap;
      slurLipT_ = std::max(1e-3f, g.lipT);
      slurDipT_ = std::max(1e-3f, g.dipT);
      slurAirT_ = std::max(1e-3f, g.airT);
    } else if (pn != pp) {
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
    slurAir_ = practised ? g.air : dir > 0 ? kSlurAir_ : kSlurAirDown_;
    const SlurGesture none;
    const SlurGesture& s = practised ? g : none;
    setAperture_ = s.aperture;
    setFirm_ = s.firm;
    setFocus_ = s.focus;
    setVowel_ = s.vowel;
    setVowelAt_ = s.vowelAt;
    setT_ = std::max(1e-3f, s.setT);
    sinceSlur_ = 0;
  }
  prevNote_ = note_;
  // A slur to a low partial: a light touch of the tongue ("legato tongue";
  // without it the new note can settle on the sharp side of a jump in its
  // slot, a stable state 30..50 cents sharp, kept from the old oscillation).
  if (practised && a == kSlur) {
    if (g.tongue > 0) {
      tongueClosed_ = g.tongue;
      tongueRelease_ = 0.003;
      tongueFloor_ = std::clamp(double(g.floor), 0.0, 1.0);
      tongueClock_ = 0;
    }
  } else if (a == kSlur && !glide && kSlurTongue_ > 0 && prevPartial_ >= 0 && played(note_)->harmonic != prevPartial_ &&
      played(note_)->harmonic <= kSlurTongueMax_) {
    tongueClosed_ = kSlurTongue_;
    tongueRelease_ = 0.003;
    tongueFloor_ = kSlurTongueFloor_;
    tongueClock_ = 0;
  }
  // A trombone's slur on one partial: the slide moves, and the soft "da" of
  // the legato tongue dips the air while it travels (without it, a smear; a
  // glide keeps the smear: the glissando).
  if (a == kSlur && !glide && inst_.design->lip->slide && prevPartial_ == played(note_)->harmonic &&
      prevSlideNote_ >= 0 && played(prevSlideNote_) && *played(note_)->holes != *played(prevSlideNote_)->holes) {
    tongueClosed_ = kSlideTongue_;
    tongueRelease_ = 0.012;
    tongueFloor_ = 0.3;
    tongueClock_ = 0;
  }
  prevSlideNote_ = note_;
  prevPartial_ = played(note_)->harmonic;
  if (a != kSlur) tongueFloor_ = 0;
  switch (a) {
    case kSlur:
      // (a practised start's slow or quick breath is the start's alone: left on, the trumpet's
      // soft E3, begun on a breath 4.8 times slower, slurred to B3 on that slow breath and
      // landed 47 c flat)
      lungAttack_ /= riseX_;
      riseX_ = 1;
      preheard_ = false;
      return;
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
  // The start as practised for this note (src/slurs.h): the tongue, the breath, the lips' set.
  riseX_ = 1;
  preheard_ = true;
  AttackGesture ag;
  if (attackGesture(inst_.name, note_, std::clamp(dyn_ + breathCtl_ + velDynTarget_, 0.0, 1.0), &ag)) {
    tongueRelease_ *= ag.release;
    lungAttack_ *= ag.rise;
    riseX_ = ag.rise;
    slurAir_ = ag.push;
    slurAirT_ = std::max(1e-3f, ag.pushT);
    slurLip_ = ag.lip;
    slurLipT_ = std::max(1e-3f, ag.lipT);
    slurDip_ = 0;
    setAperture_ = ag.aperture;
    setFirm_ = ag.firm;
    setFocus_ = 0;
    setVowel_ = ag.vowel;
    setVowelAt_ = ag.vowelAt;
    setT_ = std::max(1e-3f, ag.setT);
    sinceSlur_ = 0;
  }
  // (the syllable's consonant, Performer::setSyllable: a "d" slower, a "t" quicker)
  tongueRelease_ *= std::exp(-0.6 * sylHard_);
  lungAttack_ *= std::exp(-0.4 * sylHard_);
  tongueRelease_ *= 1 + 0.15 * wander_ * urand();
  lungAttack_ *= 1 + 0.15 * wander_ * urand();
  // The push: less on a breath attack, more on an accent.
  atkPush_ = kAtkP_ * (a == kBreath ? 0.5 : a == kDa ? 0.8 : 1.0);
  tongueClock_ = 0;
  sinceOn_ = 0;
}

void HornPlayer::render(float* out, int n) {
  static const double kSectionEar = knob("PJ_BRASS_EAR", 0.008);
  constexpr int kBlock = 16;
  const LipParams& lp = *inst_.design->lip;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double bdt = m / fs_;
    if (pendingOff_ > 0 && (pendingOff_ -= bdt) <= 0) stopBlowing();
    if (leadLeft_ > 0 && (leadLeft_ -= bdt) <= 0) horn_.setHoles(leadHoles_, leadSpeed_);
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
      const double p0 = std::clamp(lp.k * f * fastPow(f / 261.6, 0.45), lp.pressLo, lp.pressHi);
      // The push brings the breath up to a speaking pressure: a loud note's
      // own is most of the way there (pushed as hard, it overblows).
      const double push = std::clamp((1 + atkPush_) * std::exp(-kDynP_ * (dyn - 0.5)) - 1, 0.0, atkPush_);
      const double accent = 1 + (0.25 * velAccent_ + push) * std::exp(-sinceOn_ / kAtkT_);
      const double slur = std::exp(-sinceSlur_ / slurLipT_), slurA = std::exp(-sinceSlur_ / slurAirT_);
      if (blowing_)
        lungTarget = p0 * std::exp(kDynP_ * (dyn - 0.5) * (dyn < 0.5 ? 1 - kSoftAir_ : 1)) * pressTrim[note_] * kP_ * pressScale_ * accent *
                     std::max(0.1, 1 + slurAir_ * slurA);
      // The lips: calibrated per note and dynamics; the bend; the ear.
      static const double kPrehear = knob("PJ_BRASS_PREHEAR", 1);
      static const bool kPrehearSlur = knob("PJ_BRASS_PREHEAR_SLUR", 0) > 0;
      double fl = f * lp.fl * kFl_ * coverAt(note_, dyn) * glideCover(note_, dyn) * std::exp(-kDynLip_ * (dyn - 0.5) + kRollLip_ * roll_);
      if (ear_) fl *= std::exp(earCorr_);
      // (a section's player: the lips set for its own pitch and the chord's tuning at once, the
      // ear then trimming: ~600 c for the lips' factor's log, as the ear's own slope)
      if (section_) fl *= std::exp((earCents_ + seatCents_) / 600);
      // (a soloist asked a tuning by ear, the same: the note is heard before it is played, and
      // the lips are set for it from the start. The ear alone starts 0.2 s into the note and
      // reaches under 5 c: asked 12 c low, a trumpet's A4 sat 18 c off through a half-second
      // note. The starts are practised with this, 12 c either side: tools/slurs.py AOCC.)
      // (Only a tongued note for now: the slurs have not been practised with it, and a soft slur
      // into the trumpet's G3 asked 12 c low, the lips pre-set, sagged to 68 c flat.
      // PJ_BRASS_PREHEAR_SLUR=1: slurred notes too, for that practice.)
      else if (ear_ && kPrehear && (preheard_ || kPrehearSlur)) fl *= std::exp(kPrehear * earCents_ / 600);
      fl *= 1 + 0.004 * wander_ * w2b_;
      // ... then they come into the note from a little below: above, the
      // oscillation can stay on the sharp side of a jump in the slot.
      fl *= std::exp(slurLip_ * slur - slurDip_ * (std::exp(-sinceSlur_ / slurDipT_) - slur) * (sinceSlur_ < 1e8));
      e.reed = fl;
      e.lipPress = std::exp(-lp.dynH * kDynH_ * std::max(0.0, dyn - 0.5) - kSoftLips_ * std::max(0.0, 0.5 - dyn));
      // The set through a practised slur: the opening, the lips' firmness, the vowel.
      const double set = sinceSlur_ < 8 * setT_ ? std::exp(-(sinceSlur_ / setT_) * (sinceSlur_ / setT_)) : 0;
      e.lipPress *= std::exp(setAperture_ * set);
      e.lipQ = std::exp(setFirm_ * set);
      e.tract = setVowel_ * set;
      e.tractHz = noteFrequency(note_) * std::exp2(setVowelAt_);
      if (vowelAmt_ > e.tract) {  // (the syllable's vowel, held through the note)
        e.tract = vowelAmt_;
        e.tractHz = vowelHz_;
      }
      setNow_ = set;
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
    e.noise = 0.01 * noiseCtl_ * std::exp(-setFocus_ * setNow_);
    e.air = air_ * std::exp(-setFocus_ * setNow_);
    horn_.setEmbouchure(e);
    horn_.process(out + i, m);
    // The ear: once the note has spoken, a slow trim (a fast one hunts: the
    // horn's pitch answers the lips only after the long bore has settled),
    // errors over 5 cents only, and by 0.8 % of the lips' tension at most: the
    // calibration keeps 2 % from where the note would flip or crack (and
    // sustained, a note flips sooner than from silence).
    if (ear_ && note_ >= 0 && earClock_ > 0.2) {
      static const double kSectionDead = knob("PJ_BRASS_EARDEAD", 1);
      listen(out + i, m, earCorr_, earCents_ + seatCents_, earClock_, 600, kEarTau_, section_ ? kSectionDead : 5);
      // (a section's player listens closer, a dead band of 1 c: with 5 c the trombones sat 8 c
      // above their aim; its reach as a soloist's: at 2 % a horn's ear ran away 35 c flat on E4)
      const double reach = section_ ? kSectionEar : 0.008;
      earCorr_ = std::clamp(earCorr_, -reach, reach);
    }
    if (kBellTurn_) {  // (after the ear: the player hears the horn, not the hall)
      const double want = std::clamp(dyn_ + breathCtl_ + bellVel_, 0.0, 1.0);
      const double db = want < 0.5 ? kBellSoft_ * (0.5 - want) * 2 : kBellLoud_ * (want - 0.5) * 2;
      bellDb_ += (1 - std::exp(-bdt / 0.15)) * (db - bellDb_);
      const float g = float(fastPow(10.0, bellDb_ / 20) - 1), k = float(bellK_);
      for (int j = 0; j < m; ++j) {
        bellLp_ += k * (out[i + j] - bellLp_);
        out[i + j] += g * (out[i + j] - bellLp_);
      }
    }
  }
}

}  // namespace pajupilli
