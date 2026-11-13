#include "bassinst.h"

#include <algorithm>
#include <cmath>

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
};
extern const int kNumPlucked = 1;

BassModel::BassModel(const Instrument& inst) : Tube(inst), bass_(inst.fs) {}

void BassModel::resetAir() {
  Tube::resetAir();
  bass_.reset();
}

float BassModel::sample() {
  last_ = bass_.sample(float(testDp_));  // (tests: a force on the bridge's bass foot)
  testDp_ = 0;
  side_.push_back(float(0.7 * outScale()) * bass_.side());
  return finish(0.7f * last_);  // (a mezzo-forte pluck peaks near 0.1)
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

}  // namespace pajupilli
