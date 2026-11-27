#include "player.h"
#include "heard.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

#include <algorithm>
#include <cmath>

#include "knob.h"
#include "rng.h"
#include "vmath.h"

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
  kPipeLips_ = knob("PJ_PIPELIPS", 0.7);
  kP_ = knob("PJ_P", 1);
  kVibThroat_ = knob("PJ_VIBTHROAT", 0);
  vibThroat_ = kVibThroat_;
  kAim_ = knob("PJ_AIM", 1);
  pipes_ = inst_.design && inst_.design->pipes;  // (a pan flute; the blown bottles, a pan flute of glass)
  bottles_ = pipes_ && inst_.design->pipes[0].neck > 0;
  kJugTrim_ = knob("PJ_JUGTRIM", kJugTrim_);
  kBend_ = knob("PJ_PAN_BENDK", kBend_);
  kBendProb_ = knob("PJ_PAN_BEND", kBendProb_);
  kVialTrim_ = knob("PJ_VIALTRIM", kVialTrim_);
  kPanVib_ = knob("PJ_PAN_VIB", 1.6);       // its vibrato's depth x a flute's (the nai sings with it)
  kPanHand_ = knob("PJ_PAN_HAND", 0.35);    // the hand's usual share
  kPanHandK_ = knob("PJ_PAN_HANDK", 3);     // the hand: log uncovered width per unit of vibrato
  kPanEndDyn_ = knob("PJ_PAN_ENDDYN", 0.12);  // a phrase's last note: this much softer (dynamics) ...
  kPanEndAir_ = knob("PJ_PAN_ENDAIR", 0.3);   // ... and airier (a wider, less focused lip opening)
  kPanEndTune_ = knob("PJ_PAN_ENDTUNE", 0.17);
  kPanEndP_ = knob("PJ_PAN_ENDP", 0.4);      // ... the breath this much lower  // ... the lips uncovering that much more (log) to keep it in tune
  breath_ = knob("PJ_BREATH", 0.01);
  kKickP_ = knob("PJ_KICKP", 0.3);
  // The register kick's jet shape (besides PJ_KICKP's pressure and PJ_KICKW's shorter jet): the
  // opening narrowed (PJ_KICKH) and the aim moved in (PJ_KICKY), x the kick. 0: as before.
  kKickH_ = knob("PJ_KICKH", 0);
  kKickY_ = knob("PJ_KICKY", 0);
  kGeoH_ = knob("PJ_SLURGEO_H", 0.5);
  kGeoY_ = knob("PJ_SLURGEO_Y", 1);
  kLipClose_ = knob("PJ_FLUTE_LIPCLOSE", 0.012);
  kLipCloseAll_ = knob("PJ_FLUTE_LIPCLOSE_ALL", 1) > 0;  // (the first register's notes too)
  kRelTune_ = knob("PJ_FLUTE_RELTUNE", 0.08);
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
  const bool legato = note_ >= 0 && blowing_ && (stackTop_ > 1 || hold_);
  const Articulation art = articulate(midi, velocity, legato);
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
  if (trill_) {
    regKick_ = trill_->keepLips && midi != trillBase_ ? slurKick(midi) : 0;
    cancelGlide();  // a trill is never a glide
  }
  // (the aim: a phrase begins after a breath or after the director's phrase end; this note ends one)
  if (!legato && (sinceOff_ > 0.35 || aimEnd_)) aimNew_ = true;
  aimEnd_ = cued_ && cue_.v[kCuePhraseEnd] > 0.5f;
  aimSince_ = 0;
  keyClock_ = 0;
  lastKey_ = midi;
  const int from = note_;
  note_ = midi;
  keyVel_ = velocity;
  // research/playing_techniques.md 5.1: overlap = slur; a short gap = "du";
  // from silence, velocity picks a breath attack, "tu" or an accented "t".
  // (Performer::articulate: overlap and velocity also choose tongued legato
  // and a glide.)
  const Attack a = art == kArtSlur || art == kArtGlide ? kSlur
                   : art == kArtTongueLegato || art == kArtDu ? kDu
                   : art == kArtBreath ? kBreath
                   : art == kArtAccent ? kAccent : kTu;
  // Velocity is also how much air: +-0.35 dynamics around the knob (64 = none).
  // A slurred note moves there over ~60 ms; a new attack starts there. (A
  // glide's soft key is its sign, not a diminuendo.)
  if (art != kArtGlide) velDynTarget_ = kVelDyn_ * (velocity - 64) / 63.0;
  // A slur this player has practised (src/slurs.h): its gesture; one found unplayable is tongued
  // lightly ("du"), as is one practised with a touch of the tongue (a soft "d" on a wide leap).
  // None practised (the modern flutes): the old way, the register kick alone.
  practised_ = false;
  Attack a1 = a;
  if (a == kSlur && art == kArtSlur && !trill_ && from >= 0 && from != midi &&
      slurGesture(inst_.name, from, midi, std::clamp(dyn_ + breathCtl_ + velDynTarget_, 0.0, 1.0), &gesture_, sinceOn_)) {
    if (gesture_.fails) {
      a1 = kDu;
    } else {
      practised_ = true;
      slurDir_ = midi > from ? 1 : -1;
      if (gesture_.tongue > 0) a1 = kDu;
    }
  }
  // A pan flute's (a bottle's) fast run, tongued through: its syllables from the director's
  // groups, the front "t" / "d" and the back "g" / "k". Its tongued legato elsewhere: the tip's "l".
  // (Not a trill on its keys, not a glide.)
  if (pipes_ && tongueCue_ > 0 && art != kArtGlide && !trill_) {
    const bool back = tongueCue_ == 2 || tongueCue_ == 5 || (tongueCue_ == 4 && tkk_);
    // (the user's syllables: a pair "ta-ga", a three "da-da-ga" / "da-ga-ga"; a very soft key's
    // pair "da-ga", an accented back syllable a "k")
    const bool three = tongueCue_ >= 3;
    a1 = back ? (a == kAccent ? kKu : kGu) : a == kAccent ? kAccent : !three && velocity >= 50 ? kTu : kDu;
    practised_ = false;
  } else if (pipes_ && art == kArtTongueLegato) {
    a1 = kLu;
  }
  tongueCue_ = 0;
  // A slur down a semitone or two: bent on the pipe at the lips (a player's choice, a semitone
  // the likelier), and once bent, slurs within its two semitones stay on it (back up unbent).
  {
    const int pipe = pipeNote_;
    pipeNote_ = midi;
    if (pipes_ && a == kSlur && art == kArtSlur && !trill_ && pipe >= 0 && from >= 0) {
      const int down = pipe - midi;
      const bool bent = pipe != from;  // (the last note already bent on this pipe)
      const double p = kBendProb_ * (down == 1 ? 0.65 : down == 2 ? 0.35 : 0);
      if (down >= 0 && down <= 2 && (bent || 0.5 * (bendRng_.uniform() + 1) < p)) {
        pipeNote_ = pipe;
        a1 = kSlur;
        practised_ = false;
      }
    }
  }
  if (a1 != kSlur) velDyn_ = velDynTarget_;
  // Accent: a pressure overshoot growing with velocity above ~50 (tongued only).
  const double over = std::max(0.0, (velocity - 50) / 77.0);
  velAccent_ = a1 == kAccent ? 1.2 * over : a1 == kTu ? over : a1 == kKu ? 0.6 * over : 0;
  applyNote(a1);
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
    pipeNote_ = note_;
    keyVel_ = 64;  // lifting fingers: the springs do most of the work
    applyNote(kSlur);
  } else if (!hold_) {
    blowing_ = false;  // stop blowing; the fingering stays
    sinceOff_ = 0;
    released();
  }
}

// How hard to kick an upward register slur: with little air the jet needs a
// firm push, with a lot of air a full kick overblows past the note (piccolo D7).
void Player::seed(unsigned s) {
  rng_ = 4242u + 7907u * s;
  aimRng_ = 9191u + 104723u * s;
  aimSidePh_ = 2 * M_PI * aimRand();
  vib_.seed(s);
  tkk_ = knob("PJ_PAN_TKK", 0) > 0;
  bendRng_.reseed(randomSeed(3, int(s)));
  if (s == 0) return;  // the default player keeps its sequence (tests, golden files)
  for (int i = 0; i < 4; ++i) urand();
  // Players differ: vibrato rate +-8 %, depth +-15 %, flutter 23..31 Hz, all
  // starting at a random point of their cycles.
  vib_.rateMul = 1 + 0.08 * urand();
  vib_.depthMul = 1 + 0.15 * urand();
  flutterRate_ = 27 + 4 * urand();
  tkk_ = knob("PJ_PAN_TKK", -1) >= 0 ? knob("PJ_PAN_TKK", -1) > 0 : Rng(randomSeed(2, int(s))).next() % 3 == 0;
  vib_.setPhase(M_PI * (1 + urand()));
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
  trillCover_ = fastPow(trill_->cover, kTrillCover_);
  say(stderr, "%s\n", trill_->name);
  return trillHoles_;
}

void Player::applyNote(Attack a) {
  // Key velocity = finger speed: 64 moves a key in 10 ms, 127 in 5 ms, 1 in 20 ms;
  // a faster finger lands the pad harder (a louder key click).
  flute_.selectPipe(pipes_ && pipeNote_ >= 0 ? pipeNote_ : note_);
  const char* ov = std::getenv("PJ_HOLES");  // any fingering (experiments)
  flute_.setGlide(takeHoleGlide());
  const char* holes = ov && *ov ? ov : holesFor(note_);
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
  if (practised_ && gesture_.lead > 0) {  // the lips and the air first: the fingers follow
    leadLeft_ = gesture_.lead;
    leadHoles_ = holes;
    leadSpeed_ = fingerSpeed;
  } else {
    flute_.setHoles(holes, fingerSpeed);
  }
  const bool soft = practised_ && gesture_.tongue > 0;
  practised_ = false;
  blowing_ = true;
  // A new note: the lips start from its calibrated place; the ear takes over.
  if (!gliding()) earCorr_ = 0;
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
    case kDu:  // soft tongue: short seal, gentle release (a practised slur's "d": its own length)
      tongueClosed_ = soft ? gesture_.tongue : 0.002;
      tongueRelease_ = 0.008;
      lungAttack_ = 0.005;
      break;
    case kLu:  // the tip flicked against the ridge: the air barely broken
      tongueClosed_ = 0.0008;
      tongueRelease_ = 0.006;
      lungAttack_ = 0.005;
      tongueFloor_ = 0.35;
      break;
    case kGu:  // the back of the tongue against the soft palate, gently: a rounder, later start
      tongueClosed_ = 0.003;
      tongueRelease_ = 0.014;
      lungAttack_ = 0.006;
      break;
    case kKu: {  // ... firmly: the seal further back, its release slower and less sharp than a "t"
      const double v = keyVel_ / 127.0;
      tongueClosed_ = 0.004;
      tongueRelease_ = 0.0036 + 0.02 * (1 - v);
      lungAttack_ = 0.005 * std::exp2((64 - keyVel_) / 40.0);
      break;
    }
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

// The director's vibrato for the next note (Performer::direct: its depth and speed). A pan
// flutist chooses how to make it (research/panflute.md 5b): mostly the diaphragm, the hand
// rocking the instrument for a note the director leans on (a deeper cue: more of the hand, as a
// nai player's singing vibrato), the throat for a quicker, urgent one (a faster cue); high notes
// take less of the hand (a small rim: a little rocking bends them far).
void Player::direct(const Cue& c) {
  Performer::direct(c);
  if (!pipes_) return;
  tongueCue_ = int(std::lround(c.v[kCueTongue]));
  endSoft_ = c.v[kCuePhraseEnd] > 0.5f && c.v[kCuePhraseEnd] < 1.5f;  // (the piece's own end: as written)
  vibDepth_ *= kPanVib_;
  const double deep = std::clamp(double(c.v[kCueVibDepth]) - 0.8, -0.8, 0.7);
  const double quick = std::clamp((double(c.v[kCueVibRate]) - 1) / 0.06, 0.0, 1.0);
  vibHand_ = std::clamp(kPanHand_ * (1 + 1.2 * deep) * (1 + 0.3 * wander_ * urand()), 0.0, 0.7);
  vibThroat_ = std::clamp(kVibThroat_ + 0.1 + 0.3 * quick, 0.0, 0.6);
  if (vibHand_ + vibThroat_ > 0.9) vibHand_ = 0.9 - vibThroat_;
}

// How the instrument is held into the room (Tube::setPose). A shakuhachi player projects with
// the instrument and the upper body: the foot comes up (towards the hall) as the playing grows
// loud, on an accent and in the high register, and sinks for a phrase's end (over its last note)
// and between phrases; each new phrase turns to a side, 4 .. 16 degrees, mostly the other side
// from the last, with a slow sway (+-2.5 degrees, ~15 s) inside it; the face turns with the
// instrument by 0.6 of its swing. The lift follows in ~0.3 s twice over (a body, not a hand), the
// side in ~0.6 s.
void Player::aim(double dt) {
  const double deg = M_PI / 180;
  aimSince_ += dt;
  if (aimNew_) {
    aimNew_ = false;
    if (aimRand() < 0.75) aimSign_ = -aimSign_;
    aimSideTo_ = aimSign_ * (4 + 12 * aimRand()) * deg;
  }
  const bool on = note_ >= 0 && blowing_;
  double down = 55 * deg, side = aimSideTo_;
  if (on) {
    const double loud = std::clamp(dyn_ + breathCtl_ + velDyn_, 0.0, 1.0);
    const double high = std::clamp((note_ - 69) / 17.0, 0.0, 1.0);
    const double accent = cued_ ? std::max(0.0, double(cue_.v[kCueAccent])) : 0.0;
    down = 50 * deg - 18 * deg * (loud - 0.5) - 6 * deg * high - 5 * deg * accent;
    if (aimEnd_) down += 10 * deg * std::min(1.0, aimSince_ / 0.8);  // (bowing into the phrase's end)
  } else if (sinceOff_ > 1.5) {
    side *= 0.3;  // (a long rest: back towards the front)
  }
  aimSidePh_ += dt * 2 * M_PI / 15;
  side += 2.5 * deg * std::sin(aimSidePh_);
  down = 50 * deg + kAim_ * (std::clamp(down, 28 * deg, 72 * deg) - 50 * deg);
  side *= kAim_;
  const double ad = 1 - std::exp(-dt / 0.3), as = 1 - std::exp(-dt / 0.6);
  aimDown_[0] += ad * (down - aimDown_[0]);
  aimDown_[1] += ad * (aimDown_[0] - aimDown_[1]);
  aimSide_[0] += as * (side - aimSide_[0]);
  aimSide_[1] += as * (aimSide_[0] - aimSide_[1]);
  // (the room hears a new pose once it has moved by a millimetre at the foot)
  if (std::fabs(aimDown_[1] - aimSet_[0]) + std::fabs(aimSide_[1] - aimSet_[1]) > 0.002) {
    aimSet_[0] = aimDown_[1];
    aimSet_[1] = aimSide_[1];
    flute_.setPose(aimDown_[1], aimSide_[1], 0.6 * aimSide_[1]);
  }
}

void Player::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double bdt = m / fs_;
    keyClock_ += bdt;
    tick(bdt);
    if (flute_.aimed()) aim(bdt);
    {  // (a phrase's last note: the softer, airier breath grows in over ~0.15 s, goes in 50 ms)
      // (not on the top pipes, C#7 and up from G#6: there the jet barely holds as it is)
      const double t = endSoft_ && note_ >= 0 ? std::clamp((96 - note_) / 4.0, 0.0, 1.0) : 0.0;
      endAir_ += (1 - std::exp(-bdt / (t > endAir_ ? 0.15 : 0.05))) * (t - endAir_);
    }
    {  // (a bend: the lip moves over the rim in ~50 ms)
      const double to = pipes_ && note_ >= 0 && pipeNote_ >= 0 ? pipeNote_ - note_ : 0;
      bend_ += (1 - std::exp(-bdt / 0.05)) * (to - bend_);
    }
    sinceSlur_ += bdt;
    if (leadLeft_ > 0 && (leadLeft_ -= bdt) <= 0) flute_.setHoles(leadHoles_, leadSpeed_);
    if (gliding()) glideEar();
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
        say(stderr, "%s: held, lips %+.0f c\n", trill_->name,
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
      // (A bend: the pipe at the lips, its own calibration.)
      const int en = pipes_ && pipeNote_ >= 0 && pipeNote_ != note_ ? pipeNote_
                     : onTrillKey_ && trill_ && trill_->keepLips ? trillBase_ : note_;
      // (A glide: the pitch the fingers pass.)
      const double f = glideFrequency();
      const double fw = f * std::exp2(-inst_.transpose / 12.0);
      // (A pan flute's bass pipes, wider than the nai's 15 mm, and a bottle's wide neck: a bigger,
      // looser embouchure, the lips and the uncovered rim x (rim / 15 mm)^PJ_PIPELIPS (0.7), as a toyo player's.)
      double ke = inst_.emb;
      if (inst_.design && inst_.design->pipes) {
        const Design& d = *inst_.design;
        for (int k = 0; k < d.nPipes; ++k)
          if (d.pipes[k].midi == en - inst_.transpose) {  // (a bottle: its neck is the rim)
            const double rim = d.pipes[k].neck > 0 ? d.pipes[k].neckD : d.pipes[k].d;
            if (rim > 15) ke *= fastPow(rim / 15, kPipeLips_);
          }
      }
      // Dynamics: knob + breath controls + key velocity, never below the softest
      // the note holds its register at (learned by calibration).
      // (a bend's air goes down into the pipe: at a semitone barely changed, at two darker,
      // airier and weaker)
      const double dyn = std::clamp(dyn_ + breathCtl_ + velDyn_ - kPanEndDyn_ * endAir_ - 0.03 * bend_ * bend_, dynLo[en], std::max(dynLo[en], dynHi[en]));
      const double air = air_ + kPanEndAir_ * endAir_ + 0.03 * bend_ * bend_;
      // Fletcher 1975: p ~ 0.8 f Pa, lip to edge l ~ 1.8 + 100 / sqrt(f) mm, of
      // which the first 1.8 mm is where the jet forms; the jet's convective length
      // is the rest, which makes W f / Uj ~ 0.087 for every note.
      e.jetLen = 100 / std::sqrt(f) * 1e-3 * kW_ * jetTrim[en] * (1 - kKickW_ * regKick_);
      // (a phrase's end on a lower breath: the lips come forward with it, the jet's travel time kept)
      if (endAir_ > 0) e.jetLen *= std::sqrt(std::max(0.2, 1 - kPanEndP_ * endAir_));
      // (a practised slur's lips, the flutist's way into another register: not harder blowing but
      // the jet's shape. Going up the lips come forward (a shorter jet, it reaches the edge
      // sooner), the opening narrows (a thinner, faster jet for the same breath, and quieter) and
      // the air is aimed deeper into the hole; coming down the reverse. geo in log units, signed
      // with the slur: leading, then settling from the other side. The opening by
      // PJ_SLURGEO_H of it (0.5), the aim by PJ_SLURGEO_Y (1).)
      double geo = 0;
      if (sinceSlur_ < 2) {
        const double el = std::exp(-sinceSlur_ / slurLipT_);
        geo = slurDir_ * (slurLip_ * el - slurDip_ * (std::exp(-sinceSlur_ / slurDipT_) - el));
        e.jetLen *= std::exp(-geo);
      }
      // Coltman: uncovered width 6 mm at C4 .. 3.5 mm at A6; the calibration
      // trims it per note; rolling the flute (pitch bend) covers more or less.
      // (a bend: the pipe's own width, the lip's cover on top)
      const double fp = en == pipeNote_ && pipes_ ? fw * std::exp2((pipeNote_ - note_) / 12.0) : fw;
      const double t = std::clamp(std::log(fp / 261.6) / std::log(1760 / 261.6), 0.0, 1.2);
      // The lips follow the dynamics: coverage learned soft, mezzo-forte and loud.
      double ct;
      {
        const double lo = std::max(dynLo[en], 0.15);
        const double cl = std::log(coverLoTrim[en]), cm = std::log(coverTrim[en]),
                     ch = std::log(coverHiTrim[en]);
        if (dyn < 0.5) ct = cl + (cm - cl) * std::clamp((dyn - lo) / std::max(0.5 - lo, 1e-3), 0.0, 1.0);
        else ct = cm + (ch - cm) * std::min(1.0, (dyn - 0.5) / (kCalDynHi - 0.5));
        ct = std::exp(ct) * glideCover(en, dyn);
      }
      e.uncovered = (6.0 - 2.5 * t) * 1e-3 * ke * ct * trillLip_ * std::exp(0.3 * roll_ - kBend_ * bend_);
      // (the airier phrase end sags flat: the lips uncover a little more, by ear)
      // (a small rim bends far: x (261.6 / f)^0.5, at most 0.35; fitted C3..G6)
      if (endAir_ > 0) e.uncovered *= std::exp(std::min(0.35, kPanEndTune_ * std::sqrt(261.6 / fw)) * endAir_ * kPanEndAir_ / 0.3);
      // As the breath dies at a release the jet slows and the note sags flat; the lips follow it
      // out (uncovering more, as on a diminuendo) so the end stays within ~15 c. PJ_FLUTE_RELTUNE:
      // the uncovered width x (held breath / breath now)^this (0.08; 0: the lips stay).
      if (!blowing_ && kRelTune_ > 0 && lungHeld_ > 0 && lung_ > 0)
        e.uncovered *= fastPow(std::clamp(lungHeld_ / lung_, 1.0, 20.0), kRelTune_);
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
      const double airy = 1 + 0.5 * (air - 0.35);
      e.lipH = 0.75e-3 * ke * std::exp(kDynH_ * (dyn - 0.5)) * airy * (1 + 0.1 * wander_ * w3b_) *
               (1 - kKickH_ * regKick_) * std::exp(-kGeoH_ * geo);
      // At the release of an overblown note (its second register or higher) the lips close as the
      // breath stops: the jet ends with the air. Left open while the breath fades, the slowing jet
      // falls into the lower register, the note dropping an octave as it dies (no flutist's
      // ending). PJ_FLUTE_LIPCLOSE: their time constant (s; 0: they stay as they were).
      // (Not a pan pipe's, a bottle's: they have no lower register to fall into, and closed on the
      // offset jet they clicked, the slowing jet flapped wholly in and out by the ringing pipe:
      // a jug's C2 +49 dB of highs at the release. Their player only stops the breath.)
      if (!blowing_ && kLipClose_ > 0 && !pipes_ && note_ >= 0 && played(note_) && (kLipCloseAll_ || played(note_)->harmonic > 1))
        e.lipH *= std::exp(-sinceOff_ / kLipClose_);
      e.lipWidth = (7 + 4 * dyn) * 1e-3 * ke * airy;
      e.air = air;
      // Jet offset at the edge: players aim further off-centre on low notes,
      // where the jet swings further (even harmonics need offset ~ swing).
      e.y0 = kY0_ * ke * fastPow(261.6 / fw, kY0Exp_) * brightness_ * 1e-3 * (1 + 0.3 * wander_ * w2b_) *
             (1 - kKickY_ * regKick_) * std::exp(-kGeoY_ * geo);
      e.noise = breath_ * noiseCtl_ * inst_.noise * (0.4 + 1.7 * air);
      if (blowing_) lungTarget = 0.8 * f * std::exp(kDynP_ * (dyn - 0.5)) * pressTrim[en] * kP_ * pressScale_ * (1 + kKickP_ * regKick_);
      if (blowing_ && sinceSlur_ < 2) lungTarget *= std::max(0.1, 1 + slurAir_ * std::exp(-sinceSlur_ / slurAirT_));
      lungTarget *= 1 - kPanEndP_ * endAir_;  // (a phrase's end: a lower breath through the wider lips)
      if (flute_.duct()) {
        // A duct: the windway and the window are fixed; the player has only the
        // breath. It sets the pitch (the calibration's "cover" is the breath
        // here: no lips to tune with) and the loudness, which on a recorder
        // cannot be separated: louder is sharper, as on the real one.
        const JetParams& j = inst_.design->jet;
        const double lo = noteFrequency(inst_.lo), hi = noteFrequency(inst_.hi);
        const double u = std::clamp(std::log(fw / lo) / std::log(hi / lo), 0.0, 1.0);
        const double p0 = j.press * fastPow(j.pressHi / j.press, u);
        e.y0 = j.y0 + 0.1e-3 * (brightness_ - 1) * (1 + 0.3 * wander_ * w2b_);
        // The top notes: ocarina players tuck the chin (head down) to steady
        // the hard breath and cut the wind noise: half the breath noise in the
        // top sixth of the range.
        e.noise *= 1 - 0.5 * std::clamp((u - 0.83) / 0.17, 0.0, 1.0);
        // The breath tunes by ear on the note (the ear's correction), the
        // roll (pitch bend) blows harder or softer.
        lungTarget = blowing_ ? p0 * coverTrim[en] * glideCover(en, dyn) * trillLip_ * std::exp(0.35 * (dyn - 0.5) + 0.4 * roll_) *
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
    if (blowing_) lungHeld_ = lung_;  // (the breath the note was held with: its release measured against it)
    // Tongue: closed for 4 ms while the mouth fills, then opens.
    tongueClock_ += bdt;
    sinceOn_ += bdt;
    sinceOff_ += bdt;
    if (tongueClock_ < tongueClosed_) e.tongue = 0;
    else e.tongue = std::min(1.0, (tongueClock_ - tongueClosed_) / tongueRelease_);
    if (tongueFloor_ > 0 && tongueClock_ < tongueClosed_ + tongueRelease_) e.tongue = std::max(e.tongue, tongueFloor_);
    e.tongue *= tongueStop();  // a staccato note ends with the tongue
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
    // Each player's own rotating vibrato (performer.h: Vibrato), its rate
    // drifting a little with the breath support (~1 Hz wander), breathing.
    const double sv = vib_.step(bdt, vibRate_ * (1 + 0.05 * wander_ * w2b_), sinceOn_,
                                std::clamp(dyn_ + breathCtl_, 0.0, 1.0), wander_);
    const double vib = vibDepth_ * vib_.depthMul * vib_.env();
    // Accented attack: a short pressure overshoot.
    const double accent = 1 + 0.25 * velAccent_ * std::exp(-sinceOn_ / 0.04);
    const double vt = vibThroat_ * vib, vh = vibHand_ * vib, vl = std::max(0.0, 1 - vibThroat_ - vibHand_) * vib;
    // (the hand: the rim covered a little more and less as the instrument rocks; covered, flatter)
    if (vh > 0 && note_ >= 0) e.uncovered *= std::exp(kPanHandK_ * vh * sv * (1 - 0.5 * std::clamp((note_ - 79) / 12.0, 0.0, 1.0)));
    // Throat: a glottal drop of a fraction d of the lung pressure needs an
    // area Ag = Aj sqrt((1 - d) / d) (Bernoulli, same flow as the lip jet).
    const double d = std::clamp(0.01 + 0.65 * vt * (1 - sv), 0.01, 0.6);
    e.lung = lung_ * accent * (1 + vl * sv) * (1 + 0.06 * wander_ * w1b_) / (1 - (0.01 + 0.65 * vt));
    const double aj = 0.8 * e.lipH * e.lipWidth;
    double ag = std::min(1.5e-4, aj * std::sqrt((1 - d) / d));
    // "h" attack: the glottis opens from a slit (3 mm^2) over ~40 ms.
    glottisClock_ += bdt;
    if (glottisClock_ < 0.040) ag = std::min(ag, 3e-6 * fastPow(ag / 3e-6, glottisClock_ / 0.040));
    e.glottis = ag;
    flute_.setEmbouchure(e);
    flute_.process(out + i, m);
    if (bottles_) {  // (the jugs up, the vials down; glided, ~40 ms: a vial's last ring not lifted with a jug's gain)
      const int en = note_ >= 0 ? note_ - inst_.transpose : -1;
      if (en >= 0) pipeTarget_ = fastPow(10.0, (kJugTrim_ * std::max(0, 60 - en) - kVialTrim_ * std::max(0, en - 84)) / 240.0);
      const double a = 1 - std::exp(-1.0 / (0.040 * fs_));
      for (int k = 0; k < m; ++k) out[i + k] *= float(pipeGain_ += a * (pipeTarget_ - pipeGain_));
    }
    if (kTrillTune_ > 0 && note_ >= 0) {
      if (trillEar) listen(out + i, m, trillCorr_[trill_ - kTrills], 0, trillClock_, 116, kTrillTune_);
      // (an instrument's own note: tuned from its own pitch, not pulled to the tempered one by
      // the lips; pulled, the shakuhachi's kan F5 wavered as its jet was pushed off its place)
      else if (ear_) listen(out + i, m, earCorr_, earCents_ + ownCents[note_], earClock_, flute_.duct() ? 150 : 116, kTrillTune_);
    }
  }
}

}  // namespace pajupilli
