#include "player.h"

#include <cstdio>
#include <cstring>

#include <algorithm>
#include <cmath>

#include "knob.h"

namespace pajupilli {
namespace {

// Standard Boehm fingerings, C foot (research/keywork.md section 5).
// Holes 1..16: D#tr Dtr C# C B Bb A G# G#' G F# F E D# D C#.
constexpr NoteInfo kNotes[37] = {
    {"xxxxxxxxxxxxxxxx", 1},  // 60 C4
    {"xxxxxxxxxxxxxxxo", 1},  // 61 C#4
    {"xxxxxxxxxxxxxxoo", 1},  // 62 D4
    {"xxxxxxxxxxxxxooo", 1},  // 63 D#4
    {"xxxxxxxxxxxxoooo", 1},  // 64 E4
    {"xxxxxxxxxxxooooo", 1},  // 65 F4
    {"xxxxxxxxxxooxooo", 1},  // 66 F#4
    {"xxxxxxxxxooooooo", 1},  // 67 G4
    {"xxxxxxxoxooooooo", 1},  // 68 G#4
    {"xxxxxxoxoooooooo", 1},  // 69 A4
    {"xxxxxooxoooooooo", 1},  // 70 Bb4 (thumb)
    {"xxxxoooxoooooooo", 1},  // 71 B4
    {"xxxooooxoooooooo", 1},  // 72 C5
    {"xxoooooxoooooooo", 1},  // 73 C#5
    {"xxoxxxxxxxxxxxoo", 2},  // 74 D5 (C# hole vents)
    {"xxoxxxxxxxxxxooo", 2},  // 75 D#5
    {"xxxxxxxxxxxxoooo", 2},  // 76 E5
    {"xxxxxxxxxxxooooo", 2},  // 77 F5
    {"xxxxxxxxxxoxoooo", 2},  // 78 F#5 (middle finger: the ring-finger F# barely holds its register)
    {"xxxxxxxxxooooooo", 2},  // 79 G5
    {"xxxxxxxoxooooooo", 2},  // 80 G#5
    {"xxxxxxoxoooooooo", 2},  // 81 A5
    {"xxxxxooxoooooooo", 2},  // 82 Bb5
    {"xxxxoooxoooooooo", 2},  // 83 B5
    {"xxxooooxoooooooo", 2},  // 84 C6
    {"xxoooooxoooooooo", 2},  // 85 C#6
    {"xxoxxxxxxooooooo", 3},  // 86 D6 (G4 fingering, vented)
    {"xxxxxxxoxxxxxooo", 3},  // 87 D#6
    {"xxxxxxoxoxxxoooo", 3},  // 88 E6
    {"xxxxxoxxxxxooooo", 3},  // 89 F6
    {"xxxxooxxxxooxooo", 3},  // 90 F#6
    {"xxxoxxxxxooooooo", 3},  // 91 G6
    {"xxooxxxoxooooooo", 3},  // 92 G#6
    {"xxoxxxoxoxxooooo", 3},  // 93 A6
    {"xooxxooxoxxooxoo", 3},  // 94 Bb6
    {"oxxxooxxxooooxoo", 3},  // 95 B6
    {"xxxoxxxoxxxooxoo", 4},  // 96 C7
};

}  // namespace

const NoteInfo* noteInfo(int midi) {
  return midi >= 60 && midi <= 96 ? &kNotes[midi - 60] : nullptr;
}

const NoteInfo* noteInfo(const Instrument& inst, int midi) {
  const int w = midi - inst.transpose;
  return w >= inst.lo && w <= inst.hi ? noteInfo(w) : nullptr;
}

double noteFrequency(int midi, double a4) { return a4 * std::pow(2.0, (midi - 69) / 12.0); }

// Standard trill-key trills (written MIDI): hold the lower note's fingering,
// trill the key(s). cover: the lips' coverage for the upper note, where the
// trill fingering is out of tune (measured on the flute).
struct Player::Trill {
  int lo, hi;
  const char* keys;  // "1" D trill key, "2" D# trill key
  const char* name;
  double cover;
  bool settle;             // a held upper note eases over to its own fingering
  const char* notOn = "";  // an instrument where it falls out of its register
};
const Player::Trill Player::kTrills[] = {
    {72, 74, "1", "trill-c5-d5", 1.0, true},
    {73, 74, "1", "trill-cis5-d5", 0.8, true},     // +28 c without the lips
    {73, 75, "2", "trill-cis5-dis5", 0.88, true},  // +16 c without the lips
    {83, 85, "1", "trill-b5-cis6", 1.0, false},
    {84, 86, "2", "trill-c6-d6", 1.0, false},
    {85, 86, "1", "trill-cis6-d6", 1.0, false},
    {85, 87, "12", "trill-cis6-dis6", 1.0, false, "piccolo"},
};

Player::Player(Flute& f) : flute_(f), inst_(f.instrument()), fs_(f.fs()) {
  for (int i = 0; i < 128; ++i) coverTrim[i] = coverLoTrim[i] = coverHiTrim[i] = pressTrim[i] = 1;
  for (double& d : dynLo) d = 0;
  for (double& j : jetTrim) j = 1;
  kW_ = knob("PJ_W", 1);
  kY0_ = knob("PJ_Y0", 0.8);
  kY0Exp_ = knob("PJ_Y0EXP", 1);
  kP_ = knob("PJ_P", 1);
  kVibThroat_ = knob("PJ_VIBTHROAT", 0);
  breath_ = knob("PJ_BREATH", 0.01);
  kKickP_ = knob("PJ_KICKP", 0.3);
  kTrillCover_ = knob("PJ_TRILLCOVER", 1);  // 0: no lip correction on trill notes
  kTrillSettle_ = knob("PJ_TRILLSETTLE", 0.3);  // s
  kSettleGap_ = knob("PJ_SETTLEGAP", 0.03);     // s, other keys -> trill key
  kDynH_ = knob("PJ_DYNH", 0.7);
  kDynP_ = knob("PJ_DYNP", 0.6);
  kVelDyn_ = knob("PJ_VELDYN", 0.35);
  air_ = knob("PJ_AIR", 0.35);
  wander_ = knob("PJ_WANDER", 1);
  kAttackKick_ = knob("PJ_ATTACKKICK", 1);
  kKickW_ = knob("PJ_KICKW", 0.2);
  kKickT_ = knob("PJ_KICKT", 0.06);
  kKickDyn_ = knob("PJ_KICKDYN", 0.9);
}

void Player::noteOn(int midi, int velocity) {
  if (!info(midi)) return;
  held_[midi] = 1;
  if (stackTop_ < 16) stack_[stackTop_++] = midi;
  const bool legato = note_ >= 0 && blowing_ && (stackTop_ > 1 || (hold_ && velocity < 100));
  // Slurring up into a higher register: the flutist kicks the jet over
  // (faster air, lips closer to the edge for a moment), or the old mode stays.
  if (legato && info(note_) && info(midi)->harmonic > info(note_)->harmonic) regKick_ = slurKick();
  {  // One note of a trill pair slurred over the other (either held): the
     // fingers stay on the lower note and the trill key(s) give the upper.
    const int w = midi - inst_.transpose, tr = inst_.transpose;
    const Trill* t = nullptr;
    if (legato)
      for (const Trill& k : kTrills) {
        if (std::strcmp(k.notOn, inst_.name) == 0) continue;
        if ((k.lo == w && held_[k.hi + tr]) || (k.hi == w && held_[k.lo + tr])) {
          t = &k;
          break;
        }
      }
    if (t) { trill_ = t; trillBase_ = t->lo + tr; }
    else if (!trill_ || (w != trill_->hi && w != trill_->lo)) { trill_ = nullptr; trillBase_ = -1; }
  }
  if (trill_) regKick_ = 0;  // the upper note is on the same fingering, no kick
  note_ = midi;
  keyVel_ = velocity;
  // research/playing_techniques.md 5.1: overlap = slur; a short gap = "du";
  // from silence, velocity picks a breath attack, "tu" or an accented "t".
  Attack a;
  if (legato) a = kSlur;
  else if (blowing_ || sinceOff_ < 0.08) a = kDu;
  else if (velocity < 35 * hardness_) a = kBreath;
  else if (velocity > 90 / std::max(0.3, hardness_)) a = kAccent;
  else a = kTu;
  // Velocity is also how much air: +-0.35 dynamics around the knob (64 = none).
  // A slurred note moves there over ~60 ms; a new attack starts there.
  velDynTarget_ = kVelDyn_ * (velocity - 64) / 63.0;
  if (a != kSlur) velDyn_ = velDynTarget_;
  // Accent: a pressure overshoot growing with velocity above ~50 (tongued only).
  const double over = std::max(0.0, (velocity - 50) / 77.0);
  velAccent_ = a == kAccent ? 1.2 * over : a == kTu ? over : 0;
  applyNote(a);
}

void Player::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  held_[midi] = 0;
  if (trill_) {  // the trill ends when neither of its notes is held
    const int lo = trill_->lo + inst_.transpose, hi = trill_->hi + inst_.transpose;
    if (!held_[lo] && !held_[hi]) { trill_ = nullptr; trillBase_ = -1; }
  }
  int w = 0;
  for (int r = 0; r < stackTop_; ++r)
    if (stack_[r] != midi) stack_[w++] = stack_[r];
  stackTop_ = w;
  if (midi != note_) return;
  if (stackTop_ > 0) {  // fall back to the last held key, slurred
    if (info(stack_[stackTop_ - 1])->harmonic > info(note_)->harmonic && !trill_) regKick_ = slurKick();
    note_ = stack_[stackTop_ - 1];
    keyVel_ = 64;  // lifting fingers: the springs do most of the work
    applyNote(kSlur);
  } else if (!hold_) {
    blowing_ = false;  // stop blowing; the fingering stays
    sinceOff_ = 0;
  }
}

// How hard to kick an upward register slur: with little air the jet needs a
// firm push, with a lot of air a full kick overblows past the note (piccolo D7).
double Player::slurKick() const {
  const double dyn = dyn_ + breathCtl_ + velDyn_;
  return std::clamp(1 + kKickDyn_ * (0.5 - dyn), 0.5, 1.5);
}

void Player::setHold(bool h) {
  hold_ = h;
  if (!h && stackTop_ == 0) blowing_ = false;
}

const char* Player::holesFor(int midi) {
  trillCover_ = 1;
  onTrillKey_ = false;
  trillLipTau_ = 0.002;
  if (!trill_ || midi == trillBase_) return info(midi)->holes;
  onTrillKey_ = true;
  trillClock_ = 0;
  // The upper note: the lower note's fingering with the trill key(s) open.
  // Key 1 = the D trill key (hole 2), key 2 = the D# trill key (hole 1).
  std::memcpy(trillHoles_, info(trillBase_)->holes, 16);
  for (const char* k = trill_->keys; *k; ++k) trillHoles_[*k == '1' ? 1 : 0] = 'o';
  trillCover_ = std::pow(trill_->cover, kTrillCover_);
  std::fprintf(stderr, "%s\n", trill_->name);
  return trillHoles_;
}

void Player::applyNote(Attack a) {
  // Key velocity = finger speed: 64 moves a key in 10 ms, 127 in 5 ms, 1 in 20 ms;
  // a faster finger lands the pad harder (a louder key click).
  flute_.setHoles(holesFor(note_), std::exp2((keyVel_ - 64) / 63.0));
  blowing_ = true;
  switch (a) {
    case kSlur: return;
    case kBreath:  // "h": no tongue; the glottis opens from a slit over ~40 ms
      tongueClosed_ = 0;
      tongueRelease_ = 0.001;
      lungAttack_ = 0.005;
      glottisClock_ = 0;
      break;
    case kDu:  // soft tongue: short seal, gentle release
      tongueClosed_ = 0.002;
      tongueRelease_ = 0.008;
      lungAttack_ = 0.005;
      break;
    case kTu:
    case kAccent: {  // the tongue seals, the mouth fills, the tongue lets go
      // Soft keys: a slow push and a lazy tongue (16 ms); hard keys: a fast
      // push and a sharp release (2 ms).
      const double v = keyVel_ / 127.0;
      tongueClosed_ = 0.004;
      tongueRelease_ = 0.002 + 0.014 * (1 - v);
      lungAttack_ = 0.005 * std::exp2((64 - keyVel_) / 40.0);
      break;
    }
  }
  // Notes that sound only a little above their softest playable breath (the
  // third octave) start slowly: the flutist gives them a firmer start, more
  // air and the lips closer to the edge for a moment (the same kick as an
  // upward register slur).
  // Only near that floor: with more air the note speaks anyway, and a kick on
  // top would overblow it.
  if (a != kSlur) {
    const double dynNow = std::clamp(dyn_ + breathCtl_ + velDyn_, dynLo[note_], 1.0);
    const double needs = std::clamp((dynLo[note_] - 0.15) / 0.3, 0.0, 1.0) *
                         std::clamp(1 - (dynNow - dynLo[note_]) / 0.35, 0.0, 1.0);
    regKick_ = std::max(regKick_, kAttackKick_ * needs);
  }
  // No two attacks alike: the tongue and the push vary by ~+-15 %.
  tongueRelease_ *= 1 + 0.15 * wander_ * urand();
  lungAttack_ *= 1 + 0.15 * wander_ * urand();
  velAccent_ *= 1 + 0.2 * wander_ * urand();
  tongueClock_ = 0;
  sinceOn_ = 0;
}

void Player::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double bdt = m / fs_;
    // A trill that stops on its upper note (the lower key stays up): after a
    // while the fingers ease over to the note's own fingering, unheard.
    // Only the middle-octave D / D# (trill_->settle): into D6 / D#6 (3rd
    // resonance) the bass fell an octave, and C#6 swept ~45 c on the way.
    if (onTrillKey_ && trill_ && trill_->settle && note_ >= 0 &&
        !held_[trill_->lo + inst_.transpose]) {
      trillClock_ += bdt;
      if (trillClock_ > kTrillSettle_) {
        std::fprintf(stderr, "%s: settled\n", trill_->name);
        trillLipTau_ = 0.02;  // with the keys (~20 ms)
        // First the keys below the trill and C# holes to the note's own
        // fingering (the trill key stays open and keeps the note); then the
        // trill key closes (the note overblows on the long tube); then the C#
        // vent opens. Opening it with the trill key still open goes 30 c sharp.
        std::memcpy(trillHoles_ + 3, info(note_)->holes + 3, 13);
        flute_.setHoles(trillHoles_, 0.5, true);
        settleKeys_ = trill_->keys;
        settleClock_ = 0;
        trill_ = nullptr;
        trillBase_ = -1;
        onTrillKey_ = false;  // the lips stay until the trill key closes
      }
    }
    if (settleKeys_) {  // then the trill key closes, then the C# vent opens
      settleClock_ += bdt;
      if (note_ < 0 || trill_) {
        settleKeys_ = nullptr;
        if (!trill_) trillCover_ = 1;
      } else if (settleClock_ > 2 * kSettleGap_) {
        flute_.setHoles(info(note_)->holes, 0.5, true);
        settleKeys_ = nullptr;
      } else if (settleClock_ > kSettleGap_ && settleClock_ - bdt <= kSettleGap_) {
        const char* own = info(note_)->holes;
        for (int k = 0; k < 3; ++k)
          if (own[k] == 'x') trillHoles_[k] = 'x';
        flute_.setHoles(trillHoles_, 0.5, true);
        trillCover_ = 1;  // the lips back with the trill key (~20 ms)
      }
    }
    trillLip_ += (1 - std::exp(-bdt / trillLipTau_)) * (trillCover_ - trillLip_);
    Embouchure e = flute_.embouchure();
    double lungTarget = 0;
    if (note_ >= 0) {
      // Breath and jet length follow the sounding pitch (Fletcher's rules keep the
      // jet's travel time a fixed share of the period on any flute); the lips and
      // the embouchure hole are the instrument's size, set by the written pitch.
      const double f = noteFrequency(note_);
      const double fw = noteFrequency(note_ - inst_.transpose), ke = inst_.emb;
      // Dynamics: knob + breath controls + key velocity, never below the softest
      // the note holds its register at (learned by calibration).
      const double dyn = std::clamp(dyn_ + breathCtl_ + velDyn_, dynLo[note_], 1.0);
      // Fletcher 1975: p ~ 0.8 f Pa, lip to edge l ~ 1.8 + 100 / sqrt(f) mm, of
      // which the first 1.8 mm is where the jet forms; the jet's convective length
      // is the rest, which makes W f / Uj ~ 0.087 for every note.
      e.jetLen = 100 / std::sqrt(f) * 1e-3 * kW_ * jetTrim[note_] * (1 - kKickW_ * regKick_);
      // Coltman: uncovered width 6 mm at C4 .. 3.5 mm at A6; the calibration
      // trims it per note; rolling the flute (pitch bend) covers more or less.
      const double t = std::clamp(std::log(fw / 261.6) / std::log(1760 / 261.6), 0.0, 1.2);
      // The lips follow the dynamics: coverage learned soft, mezzo-forte and loud.
      double ct;
      {
        const double lo = std::max(dynLo[note_], 0.15);
        const double cl = std::log(coverLoTrim[note_]), cm = std::log(coverTrim[note_]),
                     ch = std::log(coverHiTrim[note_]);
        if (dyn < 0.5) ct = cl + (cm - cl) * std::clamp((dyn - lo) / std::max(0.5 - lo, 1e-3), 0.0, 1.0);
        else ct = cm + (ch - cm) * std::min(1.0, (dyn - 0.5) / (kCalDynHi - 0.5));
        ct = std::exp(ct);
      }
      e.uncovered = (6.0 - 2.5 * t) * 1e-3 * ke * ct * trillLip_ * std::exp(0.3 * roll_);
      // Louder: more breath and a bigger lip opening (Fletcher: all players open the lips).
      // Fletcher 1975: louder = a thicker and faster jet; the lip opening
      // changes most. Both scale around their mezzo-forte values (dyn 0.5).
      // Airy: a wider, less focused lip opening for the same breath (more
      // flow, turbulence and edge hiss; the jet spreads before the edge).
      const double airy = 1 + 0.5 * (air_ - 0.35);
      e.lipH = 0.75e-3 * ke * std::exp(kDynH_ * (dyn - 0.5)) * airy * (1 + 0.1 * wander_ * w3b_);
      e.lipWidth = (7 + 4 * dyn) * 1e-3 * ke * airy;
      e.air = air_;
      // Jet offset at the edge: players aim further off-centre on low notes,
      // where the jet swings further (even harmonics need offset ~ swing).
      e.y0 = kY0_ * ke * std::pow(261.6 / fw, kY0Exp_) * brightness_ * 1e-3 * (1 + 0.3 * wander_ * w2b_);
      e.noise = breath_ * noiseCtl_ * (0.4 + 1.7 * air_);
      if (blowing_) lungTarget = 0.8 * f * std::exp(kDynP_ * (dyn - 0.5)) * pressTrim[note_] * kP_ * (1 + kKickP_ * regKick_);
    }
    regKick_ *= std::exp(-bdt / kKickT_);
    {  // wander: two-pole low-passed noise, ~1 Hz (support), ~2 Hz (aim), ~3 Hz (lips)
      auto lp2 = [&](double& a, double& b, double hz) {
        const double k = 1 - std::exp(-2 * M_PI * hz * bdt);
        a += k * (urand() * std::sqrt(1 / k) * 0.7 - a);
        b += k * (a - b);
      };
      lp2(w1a_, w1b_, 1.0);
      lp2(w2a_, w2b_, 2.0);
      lp2(w3a_, w3b_, 3.0);
    }
    velDyn_ += (1 - std::exp(-bdt / 0.06)) * (velDynTarget_ - velDyn_);
    // Lungs: fast push (5 ms), slower release (30 ms).
    const double tc = lungTarget > lung_ ? lungAttack_ : release_;
    lung_ += (1 - std::exp(-bdt / tc)) * (lungTarget - lung_);
    // Tongue: closed for 4 ms while the mouth fills, then opens.
    tongueClock_ += bdt;
    sinceOn_ += bdt;
    sinceOff_ += bdt;
    if (tongueClock_ < tongueClosed_) e.tongue = 0;
    else e.tongue = std::min(1.0, (tongueClock_ - tongueClosed_) / tongueRelease_);
    // Flutter tongue: the tongue tip trills against the ridge (~27 Hz).
    if (flutter_ > 0) {
      flutterPhase_ += 2 * M_PI * 27 * bdt;
      if (flutterPhase_ > 2 * M_PI) flutterPhase_ -= 2 * M_PI;
      e.tongue *= 1 - flutter_ * 0.5 * (1 + std::sin(flutterPhase_));
    }
    // Vibrato: pressure modulation in the mouth, fading in after 300 ms. A
    // share comes from the lungs (abdomen), the rest from the throat: the
    // glottis is held a little narrowed and pulses (Hwang & Thorpe 2000).
    vibPhase_ += 2 * M_PI * vibRate_ * bdt;
    if (vibPhase_ > 2 * M_PI) vibPhase_ -= 2 * M_PI;
    const double vib = vibDepth_ * std::min(1.0, std::max(0.0, sinceOn_ - 0.3) / 0.4);
    const double sv = std::sin(vibPhase_);
    // Accented attack: a short pressure overshoot.
    const double accent = 1 + 0.25 * velAccent_ * std::exp(-sinceOn_ / 0.04);
    const double vt = kVibThroat_ * vib, vl = (1 - kVibThroat_) * vib;
    // Throat: a glottal drop of a fraction d of the lung pressure needs an
    // area Ag = Aj sqrt((1 - d) / d) (Bernoulli, same flow as the lip jet).
    const double d = std::clamp(0.01 + 0.65 * vt * (1 - sv), 0.01, 0.6);
    e.lung = lung_ * accent * (1 + vl * sv) * (1 + 0.06 * wander_ * w1b_) / (1 - (0.01 + 0.65 * vt));
    const double aj = 0.8 * e.lipH * e.lipWidth;
    double ag = std::min(1.5e-4, aj * std::sqrt((1 - d) / d));
    // "h" attack: the glottis opens from a slit (3 mm^2) over ~40 ms.
    glottisClock_ += bdt;
    if (glottisClock_ < 0.040) ag = std::min(ag, 3e-6 * std::pow(ag / 3e-6, glottisClock_ / 0.040));
    e.glottis = ag;
    flute_.setEmbouchure(e);
    flute_.process(out + i, m);
  }
}

}  // namespace pajupilli
