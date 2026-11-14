#include "bassinst.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "bowlevels_data.h"
#include "knob.h"

namespace pajupilli {

namespace {

// E1 .. G4 (MIDI 28 .. 67): every note is a string and a place on it. Above,
// to C6 (MIDI 84): harmonics.
const NoteInfo kBassNotes[64] = {
#define N {"", 1}
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N
#undef N
};
const Design kBassDesign = {nullptr, 0, nullptr, 0, kBassNotes, 0, 0, false, nullptr, 0, {}, {}};
const int kOpen[4] = {28, 33, 38, 43};
const int kTop = 67;  // G4: the top of the stopped notes
const int kThumbVel = 45;  // up to this velocity: the thumb
const int kFlatVel = 72;   // up to this: the flat finger; above: the stacked pull

}  // namespace

extern const Instrument kPlucked[] = {
    {"double-bass", 0, 28, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kBassDesign, "bass"},
    {"contrabass", 0, 28, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kBassDesign, "bass"},
};
extern const int kNumPlucked = 2;

bool bowedBass(const Instrument& inst) { return std::strcmp(inst.name, "contrabass") == 0; }

BassModel::BassModel(const Instrument& inst) : Tube(inst), bass_(inst.fs), bowed_(bowedBass(inst)) {
  // Under the bow the bass is heard from further away (a hall's microphone:
  // the whole body), with a little of the f-hole and the bridge.
  if (bowed_) bass_.setMics(knob("PJ_ARCO_FAR", 0.55), knob("PJ_ARCO_HOLE", 0.3), knob("PJ_ARCO_PICKUP", 0.15));
}

void BassModel::resetAir() {
  Tube::resetAir();
  bass_.reset();
}

float BassModel::sample() {
  last_ = bass_.sample(float(testDp_));  // (tests: a force on the bridge's bass foot)
  testDp_ = 0;
  // (A mezzo-forte pluck peaks near 0.1; the bow gives the string far more.)
  const float g = bowed_ ? 1.0f : 0.7f;
  side_.push_back(g * float(outScale()) * bass_.side());
  return finish(g * last_);
}

// The plucking place from the brightness knob (0.3 .. 2.5, the knob's whole
// turn): the share of the sounding length from the bridge, or -1 for the
// wrong side of the bridge.
//   knob 0     over the fingerboard, the middle of the string (sul tasto):
//              dark, woody, booming: the fundamental, few overtones
//   knob 0.2   up the fingerboard
//   knob 0.32  the usual place, 10 cm above the fingerboard's end (at rest)
//   knob 0.5   at the fingerboard's end
//   knob 0.8   by the bridge (sul ponticello): tight, bright
//   knob 0.9+  behind the bridge
double BassPlayer::pluckPlace(double brightness) {
  const double x = std::clamp((brightness - 0.3) / 2.2, 0.0, 1.0);
  if (x > 0.88) return -1;
  static const double kx[] = {0, 0.2, 0.32, 0.5, 0.8, 0.88}, kw[] = {0.5, 0.36, 0.29, 0.19, 0.07, 0.05};
  for (int i = 0; i < 5; ++i)
    if (x <= kx[i + 1]) return kw[i] + (kw[i + 1] - kw[i]) * (x - kx[i]) / (kx[i + 1] - kx[i]);
  return kw[5];
}

const char* BassPlayer::pluckPlaceName(double brightness) {
  const double w = pluckPlace(brightness);
  return w < 0 ? "behind the bridge" : w > 0.42 ? "over the fingerboard (sul tasto)" : w > 0.33 ? "up the fingerboard"
       : w > 0.24 ? "the usual place" : w > 0.14 ? "at the fingerboard's end" : "by the bridge (sul ponticello)";
}

BassPlayer::BassPlayer(BassModel& m) : Performer(m), m_(m) {}

void BassPlayer::seed(unsigned s) { rng_ = 777u + 7919u * s; }

void BassPlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int s = 0; s < 4; ++s)
      if (key_[s] < 0) m_.bass().damp(s);
}

// Which string: as a bassist chooses, by the line, not note by note. Each
// string that has the note (and is free) costs:
//   - its position: 0.5 a semitone up the neck (the low positions are home);
//   - an open string 0.8 (taken on purpose: it cannot be bent or shaken, and
//     it rings on), unless the hand has rested (then it is as good as any);
//   - leaving the string of the last note: 1.2;
//   - moving the hand: 0.6 a semitone from where it is (an open string needs
//     no hand; the cost fades as the hand rests: gone after about 2 s).
// So a phrase stays on its string while the hand can reach, and crosses to
// the next when that is nearer than a shift.
int BassPlayer::pickString(int midi) const {
  int best = -1;
  double bestCost = 1e9;
  const double fresh = std::exp(-sinceAny_ / 0.8);  // how present the last note still is
  for (int k = 0; k < 4; ++k) {
    const int pos = midi - kOpen[k];
    if (pos < 0 || pos > 26) continue;
    if (key_[k] >= 0 && key_[k] != midi) continue;  // held by another key
    double cost = 0.5 * pos;
    if (pos == 0) cost += 0.8 * fresh;
    else if (hand_ >= 0) cost += 0.6 * std::abs(pos - hand_) * fresh;
    if (lastString_ >= 0 && k != lastString_) cost += 1.2 * fresh;
    if (cost < bestCost) { bestCost = cost; best = k; }
  }
  if (best >= 0) return best;
  // All that have it are held: the lowest position's note gives way.
  for (int k = 3; k >= 0; --k)
    if (midi >= kOpen[k]) return k;
  return 0;
}

void BassPlayer::noteOn(int midi, int velocity) {
  if (midi < inst_.lo || midi > inst_.hi) return;
  static const double glideVel = knob("PJ_GLIDEVEL", 45), dvGlide = knob("PJ_DVGLIDE", 30);
  Bass& b = m_.bass();
  // The left hand alone: a soft key under a held one, on that key's string.
  if (lastString_ >= 0 && key_[lastString_] >= 0 && velocity <= glideVel && velocity - lastVel_ <= -dvGlide &&
      midi >= kOpen[lastString_] && midi - kOpen[lastString_] <= 24 && midi <= kTop && !harm_[lastString_]) {
    const int s = lastString_;
    const int from = key_[s], semis = midi - kOpen[s];
    const double len = semis ? b.openLength(s) / std::exp2(semis / 12.0) : 0;
    b.setLength(s, len, std::abs(midi - from) <= 2 ? 0.012 : 0.04 + 0.012 * std::abs(midi - from));
    key_[s] = midi;
    since_[s] = 0.1;  // (the vibrato goes on)
    note_ = midi;
    return;
  }
  // Above G4: harmonics. A natural one where an open string has it (its
  // 2nd to 6th: 12, 19, 24, 28, 31 semitones over the open string, pure, so
  // the 5th is 14 c flat, as on the instrument), on the lowest such harmonic;
  // otherwise an artificial one: the note an octave (a twelfth, two octaves)
  // lower stopped, and touched at its half (third, quarter).
  int harmonic = 0, stopNote = midi;
  int s = -1;
  if (midi > kTop) {
    static const int kSemis[5] = {12, 19, 24, 28, 31};
    for (int n = 2; n <= 6 && s < 0; ++n)
      for (int k = 3; k >= 0 && s < 0; --k)
        if (midi == kOpen[k] + kSemis[n - 2] && key_[k] < 0) { s = k; harmonic = n; stopNote = kOpen[k]; }
    if (s < 0) {
      harmonic = midi - 12 <= kTop ? 2 : midi - 19 <= kTop ? 3 : 4;
      stopNote = midi - (harmonic == 2 ? 12 : harmonic == 3 ? 19 : 24);
      s = pickString(stopNote);
    }
  } else {
    s = pickString(midi);
  }
  const int semis = stopNote - kOpen[s];
  // The pull: 2 mm (pp) to 9 mm (ff), with the dynamics knob; no two alike.
  const double v = velocity / 127.0;
  const double depth = std::min(0.012, (0.002 + 0.007 * v * v) * (0.6 + 0.8 * dyn_) * (1 + 0.06 * urand()));
  // Where: the brightness knob (pluckPlace); loud notes a little nearer the
  // bridge.
  const double place = pluckPlace(brightness_);
  if (place < 0) {  // the wrong side of the bridge: the short length there pings
    const int sb = pickString(std::min(midi, kTop));
    b.pluckBehind(sb, 0.6 * depth);
    key_[sb] = midi;
    since_[sb] = 0;
    harm_[sb] = true;
    lastVel_ = velocity;
    lastString_ = -1;
    note_ = midi;
    blowing_ = true;
    sinceAny_ = 0;
    return;
  }
  double where = std::clamp(place - 0.03 * (v - 0.5) + 0.01 * urand(), 0.06, 0.5);
  // Fast notes (the last pluck under 250 ms ago, fully by 120 ms): the hand
  // moves to the gap by the bridge (sul ponticello: tight and clear), and
  // the two fingertips take turns.
  const double fast = std::clamp((0.25 - sinceAny_) / 0.13, 0.0, 1.0);
  where += fast * (std::min(where, 0.09) - where);
  // The pull's direction, the air knob: along the top (round, sustaining) at
  // its rest and below; turned up, more and more outward, so that the string
  // swings back onto the fingerboard (the growl, then the slap).
  // A hard key does the same by itself: from velocity 100 the pull turns
  // outward (the snap), fully at 127 (the slap).
  const double snap = std::clamp((velocity - 100) / 27.0, 0.0, 1.0);
  const double angle = std::max(std::clamp(0.05 + 1.25 * (air_ - 0.35) / 0.65, 0.0, 1.3),
                                1.25 * snap * snap * (3 - 2 * snap)) + 0.06 * urand();
  // The hand, by the key's velocity: soft (up to 45) the thumb; then (to
  // 72) the flat finger, the index's side along the string; then the stacked
  // pull (index and middle as one); hard (from 100) the same pull turned
  // outward: snap, then slap. Fingertips taking turns when the
  // notes come fast (under ~210 ms apart); the thumb always with the
  // hardness knob turned down.
  const int finger = hardness_ <= 0.75 || (velocity <= kThumbVel && fast <= 0.3) ? 3
                     : fast > 0.3 ? -1 : velocity <= kFlatVel ? 4 : 2;
  if (harmonic) {  // a fingertip near the bridge; the other hand's finger touches
    b.pluck(s, semis ? b.openLength(s) / std::exp2(semis / 12.0) : 0, 1.3 * depth, 0.085 + 0.01 * urand(), 0.05, 0);
    b.touch(s, harmonic);
  } else {
    b.pluck(s, semis ? b.openLength(s) / std::exp2(semis / 12.0) : 0, depth, where, angle, finger);
  }
  harm_[s] = harmonic != 0;
  sinceAny_ = 0;
  // A ghost note (the mute pad held): the left hand lies on the string, the
  // pluck is a short thud of no clear pitch.
  if (flutter_ > 0 && !harmonic) {
    b.damp(s, 150);
    for (int k = 0; k < 4; ++k)  // (the hand lies across the others too)
      if (k != s && key_[k] < 0) b.damp(k, 60);
  }
  // Stopped notes are slid into a little from below (5-20 c, more when loud;
  // gone in ~50 ms): the finger settles.
  scoop_[s] = semis > 0 && !harmonic && fast < 0.3 ? (5 + 15 * v) * (0.5 + 0.5 * std::fabs(urand())) : 0;
  if (semis > 0) hand_ = semis;
  key_[s] = midi;
  since_[s] = 0;
  lastVel_ = velocity;
  lastString_ = s;
  note_ = midi;
  blowing_ = true;
}

void BassPlayer::noteOff(int midi) {
  bool any = false;
  for (int s = 0; s < 4; ++s) {
    if (key_[s] == midi) {
      key_[s] = -1;
      // The left hand stops the string. A short note is stopped firmly (a
      // clean, dry end); a note held long is let go slowly: the finger lifts
      // and the sound trails off. The release knob scales it (turned up, the
      // note rings on).
      const double held = since_[s];
      const double firm = held < 0.15 ? 1.6 : held > 0.6 ? 0.55 : 1.6 - 1.05 * (held - 0.15) / 0.45;
      if (!hold_) {
        m_.bass().damp(s, std::clamp(firm * 1.2 / std::max(0.01, release_), 4.0, 120.0));
        // A dry stop takes the strings that rang along too (the hand lies on
        // them); after a long note they are left to ring.
        if (firm > 1.2)
          for (int k = 0; k < 4; ++k)
            if (k != s && key_[k] < 0) m_.bass().damp(k, 30);
      }
    }
    any |= key_[s] >= 0;
  }
  if (!any) {
    blowing_ = false;
    released();
  } else if (midi == note_) {
    for (int s = 0; s < 4; ++s)
      if (key_[s] >= 0) note_ = key_[s];
  }
}

void BassPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double bdt = m / fs_;
    tick(bdt);
    sinceAny_ += bdt;
    // Vibrato: slow and wide, on stopped strings, coming in after 250 ms.
    vibPhase_ += 2 * M_PI * 0.85 * vibRate_ * bdt;
    if (vibPhase_ > 2 * M_PI) vibPhase_ -= 2 * M_PI;
    for (int s = 0; s < 4; ++s) {
      since_[s] += bdt;
      if (key_[s] < 0) continue;
      const double in = std::clamp((since_[s] - 0.25) / 0.3, 0.0, 1.0);
      // The stopping finger: the joystick slides it (sideways: up to two
      // semitones either way; pushed forward: it rocks, a vibrato on top of
      // the knob's), and it settles into each note from a little below.
      const double vib = 230 * (vibDepth_ * in + 0.3 * std::max(0.0, breathCtl_)) * std::sin(vibPhase_);
      m_.bass().bend(s, key_[s] > kOpen[s] && !harm_[s] ? vib + 200 * roll_ - scoop_[s] * std::exp(-since_[s] / 0.05) : 0.0);
    }
    // The strings' height over the fingerboard, the air knob: at its rest and
    // below, high and clean; turned up, lower and lower: the strings graze
    // the board through the note (the growl).
    m_.bass().setAction(std::clamp(1.15 - 1.2 * (air_ - 0.35), 0.35, 1.5));
    // The mute pad struck: a rake, the fingers drawn across three muted
    // strings (G, D, A), 28 ms apart, as hard as the pad was hit.
    if (flutter_ > 0 && !rakeDown_) {
      rakeDown_ = true;
      rakeNext_ = 3;
      rakeClock_ = 0;
      rakeDepth_ = 0.002 + 0.006 * flutter_;
    }
    if (flutter_ <= 0) rakeDown_ = false;
    if (rakeNext_ > 0 && (rakeClock_ -= bdt) <= 0) {
      const int rs = rakeNext_;  // strings 3, 2, 1
      if (key_[rs] < 0) {
        m_.bass().pluck(rs, 0, rakeDepth_ * (1 + 0.1 * urand()), 0.2, 0.05, 0);
        m_.bass().damp(rs, 170);
      }
      --rakeNext_;
      rakeClock_ = 0.028;
      blowing_ = true;
    }
    m_.process(out + i, m);
  }
}

// ---------------------------------------------------------------- the bow

// The bow's place from the brightness knob (0.3 .. 2.5), as a share of the
// sounding length from the bridge:
//   knob 0     over the fingerboard's end (sul tasto): 0.20
//   knob 0.32  the usual place (at rest): 0.11
//   knob 1     at the bridge (sul ponticello): 0.03
double BowPlayer::bowPlace(double brightness) {
  const double x = std::clamp((brightness - 0.3) / 2.2, 0.0, 1.0);
  return x < 0.32 ? 0.20 * std::pow(0.11 / 0.20, x / 0.32) : 0.11 * std::pow(0.03 / 0.11, (x - 0.32) / 0.68);
}

const char* BowPlayer::bowPlaceName(double brightness) {
  const double w = bowPlace(brightness);
  return w > 0.16 ? "over the fingerboard (sul tasto)" : w > 0.125 ? "towards the fingerboard" : w > 0.09 ? "the usual place"
       : w > 0.055 ? "towards the bridge" : "at the bridge (sul ponticello)";
}

BowPlayer::BowPlayer(BassModel& m) : BassPlayer(m) {}

void BowPlayer::seed(unsigned s) {
  BassPlayer::seed(s);
  vib_.seed(s);
  vib_.rateMul = 1 + 0.08 * urand();
  vib_.depthMul = 1 + 0.15 * urand();
  vib_.setPhase(M_PI * (1 + urand()));
}

void BowPlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int s = 0; s < 4; ++s)
      if (on_[s] && key_[s] < 0) lift(s);
}

// Which string, stopped where (as the plucked bass chooses), or which
// harmonic.
void BowPlayer::place(int& s, int midi, int& stopNote, int& harmonic) const {
  harmonic = 0;
  stopNote = midi;
  s = -1;
  if (midi > kTop) {
    // Harmonics: an open string's 2nd to 4th where there is one (the bow
    // holds those well); otherwise the note an octave (a twelfth, two
    // octaves) lower, stopped, and touched at its half (third, quarter).
    static const int kSemis[3] = {12, 19, 24};
    for (int n = 2; n <= 4 && s < 0; ++n)
      for (int k = 3; k >= 0 && s < 0; --k)
        if (midi == kOpen[k] + kSemis[n - 2] && key_[k] < 0) { s = k; harmonic = n; stopNote = kOpen[k]; }
    if (s < 0) {
      // (Not stopped in the last fifth of the neck if a lower stop will do:
      // the short string there is hard to hold in two.)
      harmonic = midi - 12 <= kTop - 7 ? 2 : midi - 19 <= kTop - 7 ? 3 : midi - 24 <= kTop - 7 ? 4 : midi - 12 <= kTop ? 2 : 3;
      stopNote = midi - (harmonic == 2 ? 12 : harmonic == 3 ? 19 : 24);
      s = pickString(stopNote);
    }
  } else {
    s = pickString(midi);
  }
}

// The bow comes to string s for this key (newStroke: from the air; else it
// is already moving: a slur across the strings).
void BowPlayer::start(int s, int midi, int stopNote, int harmonic, int velocity, bool newStroke) {
  Bass& b = m_.bass();
  const int semis = stopNote - kOpen[s];
  // (A stiff string's harmonics run sharp: the hand stops an artificial one
  // ~7 c lower, by ear.)
  b.finger(s, semis ? b.openLength(s) / std::exp2((semis - (harmonic ? 0.07 : 0.0)) / 12.0) : 0);
  harmN_[s] = harmonic ? harmonic : 1;
  if (harmonic) b.touch(s, harmonic, 1e9, knob("PJ_ARCO_TOUCH", 6000));
  key_[s] = midi;
  since_[s] = 0;
  harm_[s] = harmonic != 0;
  vel_[s] = velocity;
  on_[s] = true;
  strokeT_[s] = newStroke ? 0 : 0.5;
  if (semis > 0) hand_ = semis;
  lastVel_ = velocity;
  lastString_ = s;
  note_ = midi;
  blowing_ = true;
  sinceAny_ = 0;
}

void BowPlayer::lift(int s) {
  Bass& b = m_.bass();
  on_[s] = false;
  // The bow leaves (the release knob: slower); the string rings on under
  // the finger, which then lets it go.
  b.bow(s, dir_ * speed_[s], 0, dist_[s], std::clamp(0.7 * release_, 0.015, 0.4));
  b.damp(s, std::clamp(0.3 / std::max(0.01, release_), 1.5, 14.0));
}

void BowPlayer::noteOn(int midi, int velocity) {
  if (midi < inst_.lo || midi > inst_.hi) return;
  static const double glideVel = knob("PJ_GLIDEVEL", 45), dvGlide = knob("PJ_DVGLIDE", 30);
  Bass& b = m_.bass();
  down_[midi] = true;
  order_[midi] = ++clock_;
  int sounding = 0;
  for (int s = 0; s < 4; ++s) sounding += on_[s] && key_[s] >= 0;
  const bool chord = sounding > 0 && lastOn_ < 0.06;
  lastOn_ = 0;
  int s, stopNote, harmonic;
  if (sounding > 0 && !chord) {
    // Legato: the same stroke goes on; the note last begun moves.
    int from = lastString_ >= 0 && on_[lastString_] && key_[lastString_] >= 0 ? lastString_ : -1;
    for (int k = 0; k < 4 && from < 0; ++k)
      if (on_[k] && key_[k] >= 0) from = k;
    const int old = key_[from];
    const bool wasHarm = harm_[from];
    key_[from] = -1;
    place(s, midi, stopNote, harmonic);
    if (s == from && !harmonic && !wasHarm) {
      // The left hand alone: a finger down or up, a shift for a wider step;
      // the glide gesture (a soft key under a held one): a slow slide.
      const int semis = stopNote - kOpen[s], step = std::abs(midi - old);
      const bool slide = velocity <= glideVel && velocity - lastVel_ <= -dvGlide;
      b.setLength(s, semis ? b.openLength(s) / std::exp2(semis / 12.0) : 0,
                  slide ? 0.07 + 0.022 * step : step <= 4 ? 0.018 : 0.03 + 0.004 * step);
      key_[s] = midi;
      since_[s] = std::min(since_[s], 0.25);  // (the vibrato eases back in)
      if (!slide) vel_[s] = velocity;
      if (semis > 0) hand_ = semis;
      lastVel_ = velocity;
      note_ = midi;
      sinceAny_ = 0;
      return;
    }
    lift(from);  // the bow crosses to another string, moving on
    m_.bass().damp(from, 25);
    start(s, midi, stopNote, harmonic, velocity, false);
    return;
  }
  if (!chord) {  // a new stroke: the other way (after a rest: a down-bow)
    dir_ = sinceAny_ > 1.0 ? 1 : -dir_;
    travel_ = 0;
  }
  place(s, midi, stopNote, harmonic);
  if (on_[s]) b.damp(s, 60);
  start(s, midi, stopNote, harmonic, velocity, true);
}

void BowPlayer::noteOff(int midi) {
  down_[midi] = false;
  for (int s = 0; s < 4; ++s) {
    if (key_[s] != midi) continue;
    if (hold_) {  // the pedal: the bow stays on
      key_[s] = -1;
      continue;
    }
    // A key still held that no string plays (the note this one was slurred
    // from): back to it, in the same stroke.
    int back = -1;
    for (int k = inst_.lo; k <= inst_.hi; ++k) {
      if (!down_[k] || (back >= 0 && order_[k] < order_[back])) continue;
      bool playing = false;
      for (int t = 0; t < 4; ++t) playing |= key_[t] == k;
      if (!playing) back = k;
    }
    key_[s] = -1;
    if (back >= 0) {
      int s2, stopNote, harmonic;
      place(s2, back, stopNote, harmonic);
      if (s2 == s && !harmonic && !harm_[s]) {
        const int semis = stopNote - kOpen[s], step = std::abs(back - midi);
        m_.bass().setLength(s, semis ? m_.bass().openLength(s) / std::exp2(semis / 12.0) : 0, step <= 4 ? 0.018 : 0.03 + 0.004 * step);
        key_[s] = back;
        note_ = back;
        if (semis > 0) hand_ = semis;
      } else {
        lift(s);
        m_.bass().damp(s, 25);
        start(s2, back, stopNote, harmonic, vel_[s], false);
      }
      continue;
    }
    lift(s);
  }
  bool any = false;
  for (int s = 0; s < 4; ++s) any |= key_[s] >= 0;
  if (!any) {
    blowing_ = false;
    for (int s = 0; s < 4; ++s) blowing_ |= on_[s];
    if (!blowing_) released();
  } else if (midi == note_) {
    for (int s = 0; s < 4; ++s)
      if (key_[s] >= 0) note_ = key_[s];
  }
}

void BowPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  Bass& b = m_.bass();
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double bdt = m / fs_;
    tick(bdt);
    sinceAny_ += bdt;
    lastOn_ += bdt;
    // The arm is not a motor: the bow's speed drifts a little (~1 Hz).
    drift1_ += std::sqrt(6 * bdt / 0.25) * urand() - bdt / 0.25 * drift1_;
    drift_ += (drift1_ - drift_) * bdt / 0.25;
    // Nor is the stopping finger a fret: the pitch trembles by a few cents
    // (the hand's own tremor, a noise under ~4 Hz). So a note never sits
    // exactly on an open string's overtone, which would swallow it (the open
    // string, ringing along at exactly that pitch, holds the bridge still).
    static const double kTremor = knob("PJ_ARCO_TREMOR", 3.0);
    trem1_ += std::sqrt(6 * bdt / 0.04) * urand() - bdt / 0.04 * trem1_;
    tremor_ += (trem1_ - tremor_) * bdt / 0.04;
    // Tremolo (the mute pad held): the bow shakes, ~13 strokes a second.
    const bool trem = flutter_ > 0;
    bool turned = false;
    if (trem && (tremClock_ -= bdt) <= 0) {
      dir_ = -dir_;
      tremClock_ = (1 / 13.0) * (1 + 0.15 * urand());
      turned = true;
    }
    double sinceOn = 1e9, fastest = 0;
    for (int s = 0; s < 4; ++s)
      if (on_[s] && key_[s] > kOpen[s] && !harm_[s]) sinceOn = std::min(sinceOn, since_[s]);
    const double support = std::clamp(dyn_ + breathCtl_, 0.0, 1.0);
    const double vib = vib_.step(bdt, 0.85 * vibRate_, sinceOn, support, wander_) * vib_.env() * vib_.depthMul;
    const double cents = 230 * (vibDepth_ + 0.04 * std::max(0.0, breathCtl_)) * vib;
    for (int s = 0; s < 4; ++s) {
      since_[s] += bdt;
      if (!on_[s]) continue;
      strokeT_[s] += bdt;
      const double v = vel_[s] / 127.0;
      // The bow's speed: 5 cm/s (pp) to ~50 cm/s (ff), with the dynamics knob
      // and the joystick's swell.
      double speed = (0.05 + 0.45 * v * v) * (0.6 + 0.8 * dyn_) * (1 + 1.3 * std::max(0.0, breathCtl_)) *
                     (1 + 0.05 * wander_ * drift_);
      if (trem) speed *= 0.75;
      // By ear: a note the bass answers weakly gets more bow, a booming one
      // less (tools/bowlevels.py: 70 % of the difference, at most 6 dB).
      static const double kEven = knob("PJ_ARCO_EVEN", 0.7);
      const int li = key_[s] - kBowLevelLo;
      if (li >= 0 && li < int(sizeof kBowLevels / sizeof kBowLevels[0]))
        speed *= std::pow(10.0, std::clamp(-kEven * double(kBowLevels[li]), -6.0, 6.0) / 20);
      // Its place: the knob's; a louder note nearer the bridge. Between the
      // bridge and the fingerboard's end.
      const double beta = bowPlace(brightness_) * (1.12 - 0.3 * v);
      const double dist = std::clamp(beta * b.length(s) / harmN_[s], 0.012, 0.19);
      // Its pressure, the air knob; a hard key bites at the start (martele);
      // a harmonic wants a light, fast bow.
      const double bite = std::clamp((vel_[s] - 100) / 27.0, 0.0, 1.0) * std::max(0.0, 1 - strokeT_[s] / 0.07);
      double pressure = std::clamp(0.5 + 1.1 * (air_ - 0.35) + 0.3 * bite, -0.3, 1.2);
      if (harm_[s]) pressure -= 0.15;
      // How fast the stroke starts: a soft key eases in over ~130 ms, a hard
      // one in ~12 ms (the hardness knob scales it); a change of direction
      // takes ~15 ms.
      const double attack = std::clamp((0.14 - 0.13 * v) / hardness_, 0.008, 0.25);
      const double rise = trem ? 0.006 : strokeT_[s] < 3 * attack ? attack : 0.015;
      // (The hair is laid on the string first, then drawn: the force is there
      // before the speed. Drawn before it grips, the string whistles.)
      static const double kLead = knob("PJ_ARCO_LEAD", 0.25);
      b.bow(s, dir_ * speed, b.bowForce(s, speed, dist, pressure, harmN_[s]), dist, rise, std::max(0.004, kLead * rise));
      speed_[s] = speed;
      dist_[s] = dist;
      fastest = std::max(fastest, speed);
      b.bend(s, key_[s] > kOpen[s] && !harm_[s] ? cents + 200 * roll_ + kTremor * wander_ * tremor_ : 0.0);
    }
    // The bow has a length (~55 cm of hair): used up, it turns.
    travel_ += fastest * bdt;
    if (turned) travel_ = 0;
    if (travel_ > 0.5 && !trem) {
      dir_ = -dir_;
      travel_ = 0;
    }
    m_.process(out + i, m);
  }
}

}  // namespace pajupilli
