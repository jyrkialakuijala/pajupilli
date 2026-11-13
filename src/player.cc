#include "player.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <algorithm>
#include <cmath>

#include "knob.h"

namespace pajupilli {
namespace {

}  // namespace

// Standard trill-key trills (written MIDI): hold the lower note's fingering,
// trill the key(s). cover: the lips' coverage for the upper note, where the
// trill fingering is out of tune (measured on the flute).
struct Player::Trill {
  int lo, hi;
  const char* keys;  // "1" D trill key, "2" D# trill key
  const char* name;
  double cover;
  bool keepLips = false;   // the upper note keeps the lower note's embouchure, kicked up
  const char* notOn = "";  // an instrument where it falls out of its register
};
const Player::Trill Player::kTrills[] = {
    {72, 74, "1", "trill-c5-d5", 1.0},
    {73, 74, "1", "trill-cis5-d5", 0.8},     // +28 c without the lips
    {73, 75, "2", "trill-cis5-dis5", 0.88},  // +16 c without the lips
    {83, 85, "1", "trill-b5-cis6", 1.0},
    {84, 86, "2", "trill-c6-d6", 1.0},
    {85, 86, "1", "trill-cis6-d6", 1.0},
    {85, 87, "12", "trill-cis6-dis6", 1.0, false, "piccolo"},
    {86, 87, "12", "trill-d6-dis6", 1.0, true},  // D6-Eb6
    {93, 94, "12", "trill-a6-ais6", 1.0, true},  // A6-Bb6 (A5 + both keys is a minor third up)
};

Player::Player(Flute& f) : Performer(f), flute_(f) {
  kW_ = knob("PJ_W", 1);
  kFace_ = knob("PJ_FACE", 1);
  kY0_ = knob("PJ_Y0", 0.8);
  kY0Exp_ = knob("PJ_Y0EXP", 1);
  kP_ = knob("PJ_P", 1);
  kVibThroat_ = knob("PJ_VIBTHROAT", 0);
  breath_ = knob("PJ_BREATH", 0.01);
  kKickP_ = knob("PJ_KICKP", 0.3);
  kTrillCover_ = knob("PJ_TRILLCOVER", 1);  // 0: no lip correction on trill notes
  kTrillTune_ = knob("PJ_TRILLTUNE", 0.05);  // s; 0: the lips do not listen
  kTrillHeld_ = 0.3;                          // s on the trill key: report the tuning
  kTrillFast_ = knob("PJ_TRILLFAST", 0.12);  // s     // s, other keys -> trill key
  kDynH_ = knob("PJ_DYNH", 0.7);
  kDynP_ = knob("PJ_DYNP", 0.6);
  kVelDyn_ = knob("PJ_VELDYN", 0.35);
  air_ = knob("PJ_AIR", 0.35);
  wander_ = knob("PJ_WANDER", 1);
  kAttackKick_ = knob("PJ_ATTACKKICK", 1);
  kKickW_ = knob("PJ_KICKW", 0.2);
  kSlurKick_ = knob("PJ_SLURKICK", 1.9);
  kVentKickT_ = knob("PJ_VENTKICKT", 0.04);
  kVentKickDyn_ = knob("PJ_VENTKICKDYN", 0);
  kKickT_ = knob("PJ_KICKT", 0.06);
  kickT_ = kKickT_;
  kKickDyn_ = knob("PJ_KICKDYN", 0.9);
}

void Player::noteOn(int midi, int velocity) {
  if (!info(midi)) return;
  held_[midi] = 1;
  if (stackTop_ < 16) stack_[stackTop_++] = midi;
  const bool legato = note_ >= 0 && blowing_ && (stackTop_ > 1 || (hold_ && velocity < 100));
  // Slurring up into a higher register: the flutist kicks the jet over
  // (faster air, lips closer to the edge for a moment), or the old mode stays.
  if (legato && info(note_) && info(midi)->harmonic > info(note_)->harmonic) regKick_ = slurKick(midi);
  {  // One note of a trill pair slurred over the other (either held): the
     // fingers stay on the lower note and the trill key(s) give the upper.
    const int w = midi - inst_.transpose, tr = inst_.transpose;
    const Trill* t = nullptr;
    if (legato && !inst_.design)  // the trill keys: the Boehm flutes
      for (const Trill& k : kTrills) {
        if (std::strcmp(k.notOn, inst_.name) == 0) continue;
        const bool fast = keyClock_ < kTrillFast_ &&
                          (lastKey_ == k.lo + tr || lastKey_ == k.hi + tr);
        if (!fast && trill_ != &k) continue;
        if ((k.lo == w && held_[k.hi + tr]) || (k.hi == w && held_[k.lo + tr])) {
          t = &k;
          break;
        }
      }
    if (t) { trill_ = t; trillBase_ = t->lo + tr; }
    else if (!trill_ || (w != trill_->hi && w != trill_->lo)) { trill_ = nullptr; trillBase_ = -1; }
  }
  if (trill_) regKick_ = trill_->keepLips && midi != trillBase_ ? slurKick(midi) : 0;
  keyClock_ = 0;
  lastKey_ = midi;
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
  keyClock_ = 0;
  lastKey_ = midi;
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
    if (info(stack_[stackTop_ - 1])->harmonic > info(note_)->harmonic && !trill_) regKick_ = slurKick(stack_[stackTop_ - 1]);
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
void Player::seed(unsigned s) {
  rng_ = 4242u + 7907u * s;
  if (s == 0) return;  // the default player keeps its sequence (tests, golden files)
  for (int i = 0; i < 4; ++i) urand();
  // Players differ: vibrato rate +-8 %, depth +-15 %, flutter 23..31 Hz, all
  // starting at a random point of their cycles.
  vibRateMul_ = 1 + 0.08 * urand();
  vibDepthMul_ = 1 + 0.15 * urand();
  flutterRate_ = 27 + 4 * urand();
  vibPhase_ = M_PI * (1 + urand());
  flutterPhase_ = M_PI * (1 + urand());
}

double Player::slurKick(int to) {
  const double dyn = dyn_ + breathCtl_ + velDyn_;
  // Up into a vented fingering (the C# hole open as a vent: D5, D#5, D6 and
  // most of the third octave) the player pushes harder and moves the lips
  // closer, at any dynamics: with the dispersive jet, G5 -> D6 stayed on the
  // vented fingering's second mode with the octave slurs' kick.
  // A short, strong kick (40 ms): a long one overshot the pitch by 60 cents.
  const char* h = info(to)->holes;
  if (!inst_.design && h[2] == 'o' && h[3] == 'x') {
    kickT_ = kVentKickT_;
    return kSlurKick_ * std::clamp(1 + kVentKickDyn_ * (0.5 - dyn), 0.5, 1.5);
  }
  kickT_ = kKickT_;
  return std::clamp(1 + kKickDyn_ * (0.5 - dyn), 0.5, 1.5);
}

void Player::setHold(bool h) {
  hold_ = h;
  if (!h && stackTop_ == 0) blowing_ = false;
}

const char* Player::holesFor(int midi) {
  trillCover_ = 1;
  onTrillKey_ = false;
  if (!trill_ || midi == trillBase_) return info(midi)->holes;
  onTrillKey_ = true;
  trillClock_ = 0;
  heldTold_ = false;
  lastCross_ = -1;
  // The upper note: the lower note's fingering with the trill key(s) open.
  // Key 1 = the D trill key (hole 2), key 2 = the D# trill key (hole 1).
  std::strncpy(trillHoles_, info(trillBase_)->holes, kHoles);
  for (const char* k = trill_->keys; *k; ++k) trillHoles_[*k == '1' ? 1 : 0] = 'o';
  trillCover_ = std::pow(trill_->cover, kTrillCover_);
  std::fprintf(stderr, "%s\n", trill_->name);
  return trillHoles_;
}

void Player::applyNote(Attack a) {
  // Key velocity = finger speed: 64 moves a key in 10 ms, 127 in 5 ms, 1 in 20 ms;
  // a faster finger lands the pad harder (a louder key click).
  flute_.selectPipe(note_);
  const char* ov = std::getenv("PJ_HOLES");  // any fingering (experiments)
  flute_.setHoles(ov && *ov ? ov : holesFor(note_), std::exp2((keyVel_ - 64) / 63.0));
  blowing_ = true;
  // A new note: the lips start from its calibrated place; the ear takes over.
  earCorr_ = 0;
  earClock_ = 0;
  if (!onTrillKey_) lastCross_ = -1;
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
    if (kAttackKick_ * needs > regKick_) {
      regKick_ = kAttackKick_ * needs;
      kickT_ = kKickT_;
    }
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
    keyClock_ += bdt;
    double lipTarget = trillCover_;
    const bool trillEar = onTrillKey_ && trill_;
    if (!trillEar && ear_ && note_ >= 0) {
      earClock_ += bdt;
      lipTarget *= std::exp(earCorr_);
    }
    if (trillEar) {
      trillClock_ += bdt;
      lipTarget *= std::exp(trillCorr_[trill_ - kTrills]);
      if (!heldTold_ && trillClock_ > kTrillHeld_) {  // held on the key: say how the lips sit
        heldTold_ = true;
        std::fprintf(stderr, "%s: held, lips %+.0f c\n", trill_->name,
                     116 * trillCorr_[trill_ - kTrills]);
      }
    }
    trillLip_ += (1 - std::exp(-bdt / 0.002)) * (lipTarget - trillLip_);
    Embouchure e = flute_.embouchure();
    double lungTarget = 0;
    if (note_ >= 0) {
      // Breath and jet length follow the sounding pitch (Fletcher's rules keep the
      // jet's travel time a fixed share of the period on any flute); the lips and
      // the embouchure hole are the instrument's size, set by the written pitch.
      // The calibrated lips, jet and breath: the note's own, or on a third-octave
      // trill key the lower note's (the flutist keeps that embouchure; with the
      // upper note's own the flute's D6-Eb6 fell an octave).
      const int en = onTrillKey_ && trill_ && trill_->keepLips ? trillBase_ : note_;
      const double f = noteFrequency(note_);
      const double fw = noteFrequency(note_ - inst_.transpose), ke = inst_.emb;
      // Dynamics: knob + breath controls + key velocity, never below the softest
      // the note holds its register at (learned by calibration).
      const double dyn = std::clamp(dyn_ + breathCtl_ + velDyn_, dynLo[en], std::max(dynLo[en], dynHi[en]));
      // Fletcher 1975: p ~ 0.8 f Pa, lip to edge l ~ 1.8 + 100 / sqrt(f) mm, of
      // which the first 1.8 mm is where the jet forms; the jet's convective length
      // is the rest, which makes W f / Uj ~ 0.087 for every note.
      e.jetLen = 100 / std::sqrt(f) * 1e-3 * kW_ * jetTrim[en] * (1 - kKickW_ * regKick_);
      // Coltman: uncovered width 6 mm at C4 .. 3.5 mm at A6; the calibration
      // trims it per note; rolling the flute (pitch bend) covers more or less.
      const double t = std::clamp(std::log(fw / 261.6) / std::log(1760 / 261.6), 0.0, 1.2);
      // The lips follow the dynamics: coverage learned soft, mezzo-forte and loud.
      double ct;
      {
        const double lo = std::max(dynLo[en], 0.15);
        const double cl = std::log(coverLoTrim[en]), cm = std::log(coverTrim[en]),
                     ch = std::log(coverHiTrim[en]);
        if (dyn < 0.5) ct = cl + (cm - cl) * std::clamp((dyn - lo) / std::max(0.5 - lo, 1e-3), 0.0, 1.0);
        else ct = cm + (ch - cm) * std::min(1.0, (dyn - 0.5) / (kCalDynHi - 0.5));
        ct = std::exp(ct);
      }
      e.uncovered = (6.0 - 2.5 * t) * 1e-3 * ke * ct * trillLip_ * std::exp(0.3 * roll_);
      // The face and lips load the window more as the pitch rises (Dickens
      // 2007, fitted to played flutes: ~ 2.937 ln m - 11.63, x4.5 from C4 to
      // C7); A4 keeps the constant 2 mm. Without it the third octave ran sharp
      // with the window nearly closed.
      {
        const double mw = 69 + 12 * std::log2(fw / 440);
        const double sh = std::max(0.1, (2.937 * std::log(mw) - 11.63) / (2.937 * std::log(69.0) - 11.63));
        // (A transverse flute's: an end-blown rim keeps its load.)
        e.face = inst_.kind == Kind::kFlute ? 1 + kFace_ * (sh - 1) : 1;
      }
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
      if (blowing_) lungTarget = 0.8 * f * std::exp(kDynP_ * (dyn - 0.5)) * pressTrim[en] * kP_ * pressScale_ * (1 + kKickP_ * regKick_);
      if (flute_.duct()) {
        // A duct: the windway and the window are fixed; the player has only the
        // breath. It sets the pitch (the calibration's "cover" is the breath
        // here: no lips to tune with) and the loudness, which on a recorder
        // cannot be separated: louder is sharper, as on the real one.
        const JetParams& j = inst_.design->jet;
        const double lo = noteFrequency(inst_.lo), hi = noteFrequency(inst_.hi);
        const double u = std::clamp(std::log(fw / lo) / std::log(hi / lo), 0.0, 1.0);
        const double p0 = j.press * std::pow(j.pressHi / j.press, u);
        e.y0 = j.y0 + 0.1e-3 * (brightness_ - 1) * (1 + 0.3 * wander_ * w2b_);
        // The top notes: ocarina players tuck the chin (head down) to steady
        // the hard breath and cut the wind noise: half the breath noise in the
        // top sixth of the range.
        e.noise *= 1 - 0.5 * std::clamp((u - 0.83) / 0.17, 0.0, 1.0);
        // The breath tunes by ear on the note (the ear's correction), the
        // roll (pitch bend) blows harder or softer.
        lungTarget = blowing_ ? p0 * coverTrim[en] * trillLip_ * std::exp(0.35 * (dyn - 0.5) + 0.4 * roll_) *
                                    pressScale_ * (1 + 0.5 * kKickP_ * regKick_)
                              : 0;
      }
    }
    regKick_ *= std::exp(-bdt / kickT_);
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
      // Never a machine: the rate drifts with the player's lips (~3 Hz wander).
      flutterPhase_ += 2 * M_PI * flutterRate_ * (1 + 0.1 * wander_ * w3b_) * bdt;
      if (flutterPhase_ > 2 * M_PI) flutterPhase_ -= 2 * M_PI;
      e.tongue *= 1 - flutter_ * 0.5 * (1 + std::sin(flutterPhase_));
    }
    // Vibrato: pressure modulation in the mouth, fading in after 300 ms. A
    // share comes from the lungs (abdomen), the rest from the throat: the
    // glottis is held a little narrowed and pulses (Hwang & Thorpe 2000).
    // Each player's own rate, drifting a little with the breath support (~1 Hz wander).
    vibPhase_ += 2 * M_PI * vibRate_ * vibRateMul_ * (1 + 0.05 * wander_ * w2b_) * bdt;
    if (vibPhase_ > 2 * M_PI) vibPhase_ -= 2 * M_PI;
    const double vib = vibDepth_ * vibDepthMul_ * std::min(1.0, std::max(0.0, sinceOn_ - 0.3) / 0.4);
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
    if (kTrillTune_ > 0 && note_ >= 0) {
      if (trillEar) listen(out + i, m, trillCorr_[trill_ - kTrills], 0, trillClock_, 116, kTrillTune_);
      else if (ear_) listen(out + i, m, earCorr_, earCents_, earClock_, flute_.duct() ? 150 : 116, kTrillTune_);
    }
  }
}

}  // namespace pajupilli
