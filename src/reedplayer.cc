#include "reedplayer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "knob.h"
#include "slurs.h"

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
  kEarStart_ = knob("PJ_REEDEARSTART", inst_.design ? inst_.design->reed.earStart : 0);  // s on the note before the ear listens
  kRingT_ = knob("PJ_REEDSLURDOWNT", 0.008);   // s: the tongue on the reed in a slur down onto a subharmonic
  kEarRange_ = knob("PJ_REEDEARRANGE", 120);    // cents: a larger error is another regime
  // A bagpipe's chanter reed sits in the bag: no lips to tune or bend with,
  // the bag's pressure steady.
  bag_ = std::strcmp(inst_.family, "bagpipe") == 0;
  if (bag_) ear_ = false;
}

void ReedPlayer::seed(unsigned s) {
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

void ReedPlayer::setHold(bool h) {
  hold_ = h;
  if (!h && stackTop_ == 0) blowing_ = false;
}

void ReedPlayer::noteOn(int midi, int velocity) {
  // A bagpipe's low keys: its drones, each pipe tuned to its key (and
  // sounding as long as it is held, from the bag).
  if (bag_ && inst_.droneLo >= 0 && midi < inst_.lo + inst_.transpose && midi >= inst_.droneLo + inst_.transpose) {
    Drone* d = nullptr;
    for (Drone& x : drones_)
      if (x.midi == midi) d = &x;
    for (Drone& x : drones_)
      if (!d && !x.pipe.sounding()) d = &x;
    if (!d) { d = &drones_[droneNext_]; droneNext_ = (droneNext_ + 1) % 4; }
    d->midi = midi;
    d->pipe.start(fs_, 440.0 * std::exp2((midi - 69) / 12.0) * (1 + 0.0015 * urand()), unsigned(midi * 7 + droneNext_));
    blowing_ = true;
    (void)velocity;
    return;
  }
  if (!info(midi)) return;
  held_[midi] = 1;
  if (stackTop_ < 16) stack_[stackTop_++] = midi;
  const bool legato = note_ >= 0 && blowing_ && (stackTop_ > 1 || hold_);
  const Articulation art = articulate(midi, velocity, legato);
  const int from = note_;
  note_ = midi;
  // A key harder (or softer) than the note can be played: the dynamics stop
  // at the note's calibrated range, and the attack with them (the tongue, the
  // accent, the fingers as for the dynamics it gets). A reed has more states
  // than the note, and the attack decides which it falls into: a forte attack
  // on a mezzo-forte breath is not a way the calibration has played the note
  // (the English horn's G#3 spoke an octave up from any key over 64).
  if (!bag_ && art != kArtGlide && kVelDyn_ > 0) {
    const double want = dyn_ + breathCtl_ + kVelDyn_ * (velocity - 64) / 63.0;
    const double got = std::clamp(want, dynLo[midi], std::max(dynLo[midi], dynHi[midi]));
    const int v = int(std::lround(velocity - 63 * (want - got) / kVelDyn_));
    velocity = want > got ? std::clamp(v, std::min(velocity, 64), velocity)
                          : std::clamp(v, velocity, std::max(velocity, 64));
  }
  keyVel_ = velocity;
  // (... and a breath attack is a soft key's: a key brought up to the note's softest dynamics
  // is tongued, as the calibration played it there. The alto saxophone's E3, playable at
  // mezzo-forte only, spoke an octave up from a key of 30: a breath attack at that breath.)
  const bool breath = art == kArtBreath && (bag_ || velocity < 35 * hardness_);
  Attack a = art == kArtSlur || art == kArtGlide ? kSlur
             : art == kArtTongueLegato || art == kArtDu ? kDu
             : breath ? kBreath
             : art == kArtAccent ? kAccent : kTu;
  // (A glide's soft key is its sign, not a diminuendo.)
  if (art != kArtGlide) velDynTarget_ = kVelDyn_ * (velocity - 64) / 63.0;
  a = slurPlan(from, a, art == kArtSlur);
  if (a != kSlur) velDyn_ = velDynTarget_;
  const double over = std::max(0.0, (velocity - 50) / 77.0);
  velAccent_ = a == kAccent ? 1.2 * over : a == kTu ? 0.5 * over : 0;
  if (bag_) {
    // The bag: the same pressure whatever the key; the piper never tongues. A
    // note struck again (or tongued) is split from the last by a grace note:
    // high G, or E on high G and A; from silence the bag's air starts the reed.
    velDynTarget_ = velDyn_ = velAccent_ = 0;
    // (The bag keeps the reed going between keys: a key soon after a release is
    // split by a grace note, not started again.)
    if (a != kSlur && a != kBreath && (blowing_ || sinceOff_ < 0.08)) {
      const int g = note_ < 79 + inst_.transpose ? 79 : 76;
      if (const NoteInfo* gi = info(g + inst_.transpose)) {
        reed_.setHoles(gi->holes, 2);
        graceLeft_ = 0.030;
      }
      blowing_ = true;
      return;
    }
    applyNote(a == kSlur ? kSlur : kBreath);
    return;
  }
  applyNote(a);
}

void ReedPlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  for (Drone& d : drones_)
    if (d.midi == midi) { d.pipe.stop(); d.midi = -1; return; }
  held_[midi] = 0;
  int w = 0;
  for (int r = 0; r < stackTop_; ++r)
    if (stack_[r] != midi) stack_[w++] = stack_[r];
  stackTop_ = w;
  if (midi != note_) return;
  if (stackTop_ > 0) {
    const int from = note_;
    note_ = stack_[stackTop_ - 1];
    keyVel_ = 64;
    applyNote(slurPlan(from, kSlur, true));
  } else if (!hold_) {
    blowing_ = false;
    sinceOff_ = 0;
    released();
  }
}

// A slur down an octave, a twelfth or two octaves: the note left is a harmonic of the new one
// and its air column goes on ringing in the new fingering, which holds it as its own second or
// third register (the oboe's C5 slurred to C4 stayed on C5; the bassoon's, the saxophones' low
// notes likewise). A player touches the reed with the tongue there, or the slur fails for him
// too. PJ_REEDSLURDOWN=0: a plain slur.
bool ReedPlayer::rings(int from, int to) const {
  static const bool kOn = knob("PJ_REEDSLURDOWN", 1) > 0;
  const int d = from - to;
  return kOn && from >= 0 && (d == 12 || d == 19 || d == 24);
}

// How a slur from `from` to the note now is played: as practised (src/slurs.h: the gesture kept
// for applyNote), else the old way. A slur found unplayable is tongued lightly ("du").
ReedPlayer::Attack ReedPlayer::slurPlan(int from, Attack a, bool plain) {
  ringing_ = practised_ = false;
  ringT_ = kRingT_;
  if (a != kSlur || bag_ || !plain || from < 0 || from == note_) return a;
  if (slurGesture(inst_.name, from, note_, std::clamp(dyn_ + breathCtl_ + velDynTarget_, 0.0, 1.0), &gesture_)) {
    if (gesture_.fails) return kDu;
    practised_ = true;
    if (gesture_.tongue <= 0) return kSlur;
    ringing_ = true;
    ringT_ = gesture_.tongue;
    return kDu;
  }
  ringing_ = rings(from, note_);
  return ringing_ ? kDu : kSlur;
}

void ReedPlayer::applyNote(Attack a) {
  // PJ_HOLES: any fingering (experiments).
  const char* ov = std::getenv("PJ_HOLES");
  reed_.setGlide(takeHoleGlide());
  const char* holes = ov && *ov ? ov : info(note_)->holes;
  const double fingerSpeed = std::exp2((keyVel_ - 64) / 63.0) * (practised_ ? gesture_.fingers : 1);
  leadLeft_ = 0;
  tongueFloor_ = 0;
  if (practised_) {
    slurAir_ = gesture_.air;
    slurLip_ = gesture_.lip;
    slurDip_ = gesture_.dip;
    slurAirT_ = std::max(1e-3f, gesture_.airT);
    slurLipT_ = std::max(1e-3f, gesture_.lipT);
    slurDipT_ = std::max(1e-3f, gesture_.dipT);
    sinceSlur_ = 0;
    if (gesture_.tongue > 0) tongueFloor_ = std::clamp(double(gesture_.floor), 0.0, 1.0);
  }
  if (practised_ && gesture_.lead > 0) {  // the lips and the breath first: the fingers follow
    leadLeft_ = gesture_.lead;
    leadHoles_ = holes;
    leadSpeed_ = fingerSpeed;
  } else {
    reed_.setHoles(holes, fingerSpeed);
  }
  practised_ = false;
  blowing_ = true;
  if (!gliding()) earCorr_ = 0;
  earClock_ = 0;
  lastCross_ = -1;
  switch (a) {
    case kSlur: return;
    case kBreath:  // the reed starts on the breath alone: a slow push, no tongue
      tongueClosed_ = 0;
      tongueRelease_ = 0.001;
      lungAttack_ = bag_ ? 0.015 : 0.040;  // (the bag is full: the arm's squeeze)
      break;
    case kDu:  // a light touch of the tongue on the reed
      tongueClosed_ = ringing_ ? ringT_ : 0.002;
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
  primePending_ = inst_.design->reed.prime > 0;
}

void ReedPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  const ReedParams& rp = inst_.design->reed;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double bdt = m / fs_;
    tick(bdt);
    if (leadLeft_ > 0 && (leadLeft_ -= bdt) <= 0 && note_ >= 0) reed_.setHoles(leadHoles_, leadSpeed_);
    sinceSlur_ += bdt;
    if (graceLeft_ > 0 && (graceLeft_ -= bdt) <= 0 && note_ >= 0) reed_.setHoles(info(note_)->holes, 2);
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
    Embouchure e = reed_.embouchure();
    double lungTarget = 0;
    // Vibrato, fading in after 300 ms: the breath, or the jaw (rp.vib = 1).
    // (performer.h: Vibrato, a rotating complex number, breathing.)
    const double sv = vib_.step(bdt, vibRate_ * (1 + 0.05 * wander_ * w2b_), sinceOn_,
                                std::clamp(dyn_ + breathCtl_, 0.0, 1.0), wander_);
    // (A clarinet, rp.vib < 0, plays straight: classical clarinettists use none.)
    const double vib = rp.vib < 0 ? 0 : vibDepth_ * vib_.depthMul * vib_.env();
    const double jaw = std::max(0.0, rp.vib);
    if (note_ >= 0) {
      const double dyn = bag_ ? std::clamp(0.5, dynLo[note_], std::max(dynLo[note_], dynHi[note_]))
                              : std::clamp(dyn_ + breathCtl_ + velDyn_, dynLo[note_], std::max(dynLo[note_], dynHi[note_]));
      const double lo = noteFrequency(inst_.lo), hi = noteFrequency(inst_.hi);
      const double fw = glideFrequency() * std::exp2(-inst_.transpose / 12.0);
      const double u = std::clamp(std::log(fw / lo) / std::log(hi / lo), 0.0, 1.0);
      const double p0 = rp.press * std::pow(rp.pressHi / rp.press, u);
      // (The attack's accent no higher than the note's loudest calibrated
      // pressure: past it, a low oboe note breaks into its octave and stays.)
      const double hiP = std::exp(kDynP_ * (std::max(dynLo[note_], dynHi[note_]) - dyn));
      const double accent = std::min(1 + 0.2 * velAccent_ * std::exp(-sinceOn_ / 0.05), std::max(1.0, hiP));
      if (blowing_)
        lungTarget = p0 * std::exp(kDynP_ * (dyn - 0.5)) * pressTrim[note_] * kP_ * pressScale_ * accent *
                     std::max(0.1, 1 + slurAir_ * std::exp(-sinceSlur_ / slurAirT_));
      // The lips: calibrated per note and dynamics; a little looser when loud;
      // the bend; the ear; the jaw vibrato.
      double lip = coverAt(note_, dyn) * glideCover(note_, dyn) * std::exp(-kDynLip_ * (dyn - 0.5) + kRollLip_ * roll_);
      if (ear_) lip *= std::exp(earCorr_);
      lip *= 1 + 0.04 * wander_ * w2b_;
      if (sinceSlur_ < 2) {  // a practised slur's lips (src/slurs.h)
        const double el = std::exp(-sinceSlur_ / slurLipT_);
        lip *= std::exp(slurLip_ * el - slurDip_ * (std::exp(-sinceSlur_ / slurDipT_) - el));
      }
      lip *= 1 + 2 * jaw * vib * sv;
      e.reed = lip;
      // The top notes: the lips rolled out (less lip on the blades).
      const int top = inst_.hi + inst_.transpose;
      e.reedDamp = std::pow(rp.roll, std::clamp((note_ - (top - 2)) / 2.0, 0.0, 1.0));
    }
    // Lungs: a fast push, a slower release.
    const double tc = lungTarget > lung_ ? lungAttack_ : release_;
    lung_ += (1 - std::exp(-bdt / tc)) * (lungTarget - lung_);
    tongueClock_ += bdt;
    sinceOn_ += bdt;
    sinceOff_ += bdt;
    e.tongue = tongueClock_ < tongueClosed_ ? 0 : std::min(1.0, (tongueClock_ - tongueClosed_) / tongueRelease_);
    e.tongue = tongueFloor_ + (1 - tongueFloor_) * e.tongue;
    e.tongue *= tongueStop();
    if (primePending_ && note_ >= 0 && tongueClock_ >= tongueClosed_) {
      // The skilled start: the column set ringing at the note as the tongue
      // leaves (scaled with the breath: a soft start rings it less).
      static const double kPrime = knob("PJ_PRIME", 1.0), kCycles = knob("PJ_PRIMECYC", 4);
      const double hz = glideFrequency() * std::exp2(-inst_.transpose / 12.0);
      const double p0 = rp.press * std::pow(rp.pressHi / rp.press, 0.5);
      reed_.prime(kPrime * rp.prime * std::sqrt(std::max(lungTarget, 0.0) / p0), hz, kCycles);
      primePending_ = false;
    }
    // The release: as the breath falls, the lips (and the bag's piper, the
    // fingers' grip on the bag) let the reed close with it. (On the breath
    // alone a reed played near its closing pressure passed its loudest point
    // on the way down: up to 3-4 dB louder after the key was up.)
    if (blowing_) lungHeld_ = lung_;
    else if (lungHeld_ > 0) e.tongue *= std::clamp(lung_ / lungHeld_, 0.0, 1.0);
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
    // The drones, from the same bag (their level against the chanter's:
    // PJ_DRONE_GAIN).
    if (bag_) {
      static const float kDroneGain = float(knob("PJ_DRONE_GAIN", 0.1));
      static const float kBag = float(knob("PJ_DRONE_BAG", 0.85));
      bool any = false;
      for (Drone& d : drones_)
        if (d.pipe.sounding()) {
          any = true;
          for (int j = 0; j < m; ++j) out[i + j] += kDroneGain * d.pipe.sample(kBag);
        }
      if (any && note_ < 0) blowing_ = true;
      else if (!any && droneOn_ && note_ < 0) blowing_ = false;  // (the last drone gone quiet)
      droneOn_ = any;
    }
    // (An error over 120 cents is another regime, not a matter for the lips:
    // chasing the attack's flat start pushed the duduk's A4 into G4.)
    if (ear_ && note_ >= 0 && earClock_ > kEarStart_) listen(out + i, m, earCorr_, earCents_, earClock_, 150, 0.05, 0, kEarRange_);
  }
}

}  // namespace pajupilli
