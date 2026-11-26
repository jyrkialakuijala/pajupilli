#include "guitarinst.h"
#include "heard.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "knob.h"
#include "songclock.h"
#include "vmath.h"

namespace pajupilli {

namespace {
constexpr double kPiHalf = 1.5707963267948966;
}

bool guitarInstrument(const Instrument& inst) {
  return std::strcmp(inst.name, "nylon-guitar") == 0 || std::strcmp(inst.name, "clean-guitar") == 0 ||
         std::strcmp(inst.name, "muted-guitar") == 0 || std::strcmp(inst.name, "overdriven-guitar") == 0 ||
         std::strcmp(inst.name, "distortion-guitar") == 0 || guitarHarmonics(inst) ||
         std::strcmp(inst.name, "steel-guitar") == 0 || jazzGuitarInstrument(inst);
}

bool jazzGuitarInstrument(const Instrument& inst) { return std::strcmp(inst.name, "jazz-guitar") == 0; }

bool guitarHarmonics(const Instrument& inst) { return std::strcmp(inst.name, "guitar-harmonics") == 0; }

namespace {
GuitarSpec specFor(const Instrument& inst) {
  if (std::strcmp(inst.name, "steel-guitar") == 0) return steelGuitar();
  if (jazzGuitarInstrument(inst)) return jazzGuitar();
  return std::strcmp(inst.name, "nylon-guitar") == 0 ? classicalGuitar() : electricGuitar(inst.name);
}
}  // namespace

GuitarModel::GuitarModel(const Instrument& inst) : Tube(inst), g_(inst.fs, specFor(inst)) {
  // (PJ_EGTR_DI=1: the pickups' EMF itself, no amp: for looking at the guitar alone.)
  if (!g_.electric() || knob("PJ_EGTR_DI", 0) > 0) return;
  if (jazzGuitarInstrument(inst)) {
    // GM 27 (research/jazz_guitar.md 9.1-9.2): the jazz amp; the humbucker with the guitar's tone
    // control rolled back (the tone pot's resistance PJ_JAZZ_TONE kOhm in series with 22 nF across
    // the pickup), a 600 pF cable. And the archtop's own sound in the room, ~20 dB under the amp
    // (PJ_JAZZ_ACOUSTIC, dB re that; -60: none).
    amp_.reset(new GuitarAmp(inst.fs, GuitarAmp::jazz()));
    const GuitarSpec& s = g_.spec();
    const GuitarSpec::Pickup& p = s.pickups[s.select[0]];
    const int n = s.select[1] >= 0 ? 2 : 1;
    GuitarAmp::Source src{p.L / n, p.R / n, p.C * n, s.pots};
    src.Rt = 1e3 * knob("PJ_JAZZ_TONE", 45);
    src.Ct = 22e-9;
    amp_->connect(src, knob("PJ_JAZZ_CABLE", 600e-12));
    // (The jazz amp's output is ~37 dB under the Twin's at its settings: brought up here, past the
    // strip, whose levels are the amp's own, to the common loudness, the rest in levels_data.h.)
    constexpr double kJazzLevel = 74.0;
    kAmpOut_ = float(kJazzLevel * knob("PJ_EGTR_OUT", 1.0));
    kAcoustic_ = float(kJazzLevel * kAcousticCal_ * fastPow(10.0, knob("PJ_JAZZ_ACOUSTIC", 0) / 20));
    return;
  }
  // The amp: a clean clean 2x12 combo for GM 28; with a compressor in front for GM 29; a British stack
  // British crunch stack at crunch for GM 30; a high-gain amp SLO into a 60 W British 4x12, gated, for GM 31. What is plugged
  // in: the selected pickups' coils (two in parallel: half the inductance and resistance, twice
  // the capacitance), the pots.
  amp_.reset(new GuitarAmp(inst.fs, std::strcmp(inst.name, "muted-guitar") == 0        ? GuitarAmp::cleanComboComp()
                                    : std::strcmp(inst.name, "overdriven-guitar") == 0 ? GuitarAmp::crunchStack()
                                    : std::strcmp(inst.name, "distortion-guitar") == 0 ? GuitarAmp::slo()
                                                                                        : GuitarAmp::cleanCombo()));
  const GuitarSpec& s = g_.spec();
  const GuitarSpec::Pickup& p = s.pickups[s.select[0]];
  const int n = s.select[1] >= 0 ? 2 : 1;
  amp_->connect({p.L / n, p.R / n, p.C * n, s.pots}, knob("PJ_EGTR_CABLE", 470e-12));
  kAmpOut_ = float(knob("PJ_EGTR_OUT", 1.0));
}

void GuitarModel::resetAir() {
  Tube::resetAir();
  g_.reset();
  if (amp_) amp_->reset();
}

// The strings' sample: the tests' kick (testDrive) goes into them; a non-finite state they caught
// and cleared clears the rest too (the amp, the filters), counted as a reset.
float GuitarModel::strings() {
  if (testDp_ != 0) { g_.kick(float(testDp_)); testDp_ = 0; }
  const float x = g_.sample();
  if (g_.nanResets() != nanSeen_) [[unlikely]] {
    nanSeen_ = g_.nanResets();
    resetAir();
    ++resets_;
  }
  return x;
}

float GuitarModel::sample() {
  if (amp_ && kAcoustic_ > 0) {
    // (The jazz archtop: the amp, and the guitar's own sound under it.)
    last_ = amp_->process(strings()) * kAmpOut_ + kAcoustic_ * g_.acoustic();
    side_.push_back(0.0f);
    return finish(last_);
  }
  if (amp_) {
    last_ = amp_->process(strings()) * kAmpOut_;
    side_.push_back(0.0f);
    return finish(last_);
  }
  last_ = strings();
  side_.push_back(float(outScale()) * g_.side());
  return finish(last_);
}

GuitarPlayer::GuitarPlayer(GuitarModel& m) : Performer(m), m_(m) {
  std::fill(note_on_, note_on_ + Guitar::kStrings, -1);
  vibrato_ = knob("PJ_GUITAR_VIBRATO", 1.0);  // the left hand's vibrato (x; 0: none)
  pull_ = knob("PJ_GUITAR_PULL", 1.0);        // mending minor thirds (x 13 cents; 0: never)
  handSpeed_ = knob("PJ_GUITAR_STRUMSPEED", 1.4);  // the strumming hand across the strings (m/s)
  strumFree_ = knob("PJ_GUITAR_STRUMFREE", 0.05);  // how often a strum goes against the hand's swing
  sameAsLastBar_ = knob("PJ_GUITAR_STRUMBAR", 0.3);  // how often the last bar's way at this place wins over a different reading
  electric_ = m.guitar().electric();
  steel_ = std::strcmp(m.instrument().name, "steel-guitar") == 0;
  muted_ = std::strcmp(m.instrument().name, "muted-guitar") == 0;
  distorted_ = std::strcmp(m.instrument().name, "distortion-guitar") == 0;
  driven_ = distorted_ || std::strcmp(m.instrument().name, "overdriven-guitar") == 0;
  chug_ = knob("PJ_EGTR_CHUG", 1.0);      // (GM 31: the palm on chugs, x its weight; 0: never)
  octave_ = knob("PJ_EGTR_OCTAVE", 1.0);  // (GM 31: the octave added to a power chord's root and fifth; 0: not)
  wide_ = knob("PJ_EGTR_WIDE", 1.0);      // (GM 31: the lead's wide vibrato, x 40-100 cents)
  // (Loud and driven, a string left free would ring up from the amp: the hands keep the strings
  // not being played muted.)
  if (driven_)
    for (int s = 0; s < Guitar::kStrings; ++s) m.guitar().damp(s, 60);
  if (electric_) handSpeed_ = knob("PJ_EGTR_STRUMSPEED", 1.75);  // a pick across the strings: 6 ms a string
  kPalm_ = knob("PJ_EGTR_PALM", 5.0);  // the palm's hold on an open string's fundamental (1/s)
  kRelease_ = knob("PJ_EGTR_MUTEDRELEASE", 25.0);
  feedback_ = knob("PJ_EGTR_FEEDBACK", 1.0);  // (the overdriven guitar's: 0 none, 1 as found)
  bloomGain_ = knob("PJ_EGTR_BLOOM", 2.0);
  holdAt_ = knob("PJ_EGTR_HOLDAT", 0.12);  // (when the note starts to be held, s)
  holdHz_ = knob("PJ_EGTR_HOLDHZ", 2500.0);  // (its modes held below this)
}

double GuitarPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

void GuitarPlayer::noteOn(int midi, int velocity) {
  if (steel_)
    for (int s = 0; s < Guitar::kStrings; ++s)
      if (!held_[s] && (idleMuted_[s] || note_on_[s] < 0)) {
        // (The hand lifts, but the strings not played are still touched lightly by the hands:
        // free, an open string answers a fretted note of its pitch nearly as loud as the note,
        // and stays where it is under a bend. PJ_STEEL_IDLE: its damping, 1/s.)
        static const double kIdle = knob("PJ_STEEL_IDLE", 3.0);
        idleMuted_[s] = false;
        m_.guitar().damp(s, kIdle);
      }
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  if (asked_.empty()) {
    gapBefore_ = sinceNote_;
    // How busy the music has been: the gaps between strokes, the last few weighing most.
    pace_ += 0.3 * (std::min(gapBefore_, 3.0) - pace_);
  }
  asked_.push_back({midi, velocity});
  sinceNote_ = 0;
}

void GuitarPlayer::noteOff(int midi) {
  for (size_t i = 0; i < asked_.size(); ++i)
    if (asked_[i].midi == midi) { asked_.erase(asked_.begin() + i); return; }  // (let go before it was played)
  if (distorted_ && midi == pmRoot_ && pmHeld_ < 0) pmHeld_ = sinceNote_;  // (GM 31: how short it was)
  for (int s = 0; s < Guitar::kStrings; ++s)
    if ((note_on_[s] == midi || (octaveAdded_[s] && note_on_[s] == midi + 12)) && held_[s]) {
      held_[s] = false;
      // (On the muted guitar the palm is already on the string: the lifting finger lets it go
      // more gently.)
      // (Steel: while strumming the hands only ease off, the chord rings into the next. An open
      // string is stopped too, by the side of the picking hand: Jyrki, "G3 doesn't damp when I
      // release the key".)
      if (steel_) { if (!hold_) m_.guitar().damp(s, strumming_ > 0 ? 20 : 45); continue; }
      if (!hold_) m_.guitar().damp(s, muted_ ? kRelease_ : electric_ ? 60 : 45);
    }
  if (steel_ && !hold_ && strumming_ <= 0) {
    // The last key let go: the picking hand's side comes down on all the strings, also those
    // that only rang along (a lone G3 is fretted on the D string, and the open G string answers
    // it for seconds on steel; Jyrki: "G3 doesn't damp when I release the key").
    bool any = false;
    for (int s = 0; s < Guitar::kStrings; ++s) any = any || held_[s];
    if (!any && asked_.empty())
      for (int s = 0; s < Guitar::kStrings; ++s) { m_.guitar().damp(s, 45); idleMuted_[s] = true; }
  }
}

void GuitarPlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int s = 0; s < Guitar::kStrings; ++s)
      if (!held_[s] && note_on_[s] >= 0) m_.guitar().damp(s, 45);
}

// The bar's sixteen sixteenths: which one the song is at (the bar taken as
// four beats from the song's start), and the bar's number.
int GuitarPlayer::barSlot(double beat, int* bar) const {
  const double b = beat + 1.0 / 32;
  const int n = int(std::floor(b / 4));
  if (bar) *bar = n;
  return std::clamp(int((b - 4 * n) * 4), 0, 15);
}

// A strum, into this bar's pattern; a new bar makes this one the last.
void GuitarPlayer::noteStrum(int way) {
  const SongClock& sc = SongClock::get();
  if (!sc.valid) return;
  int bar;
  const int slot = barSlot(sc.beat, &bar);
  if (bar != bar_) {
    int n = 0;
    for (int w : thisBar_) n += w != 0;
    lastBarClear_ = bar == bar_ + 1 && n >= 3;
    std::copy(thisBar_, thisBar_ + 16, lastBar_);
    std::fill(thisBar_, thisBar_ + 16, 0);
    bar_ = bar;
  }
  thisBar_[slot] = way;
}

void GuitarPlayer::play() {
  Guitar& g = m_.guitar();
  static const int kOpen[Guitar::kStrings] = {40, 45, 50, 55, 59, 64};
  const int top = kOpen[5] + g.frets();
  std::sort(asked_.begin(), asked_.end(), [](const Asked& a, const Asked& b) { return a.midi < b.midi; });
  // More notes than strings: keep the bass and the top (the melody), drop inner voices, a note
  // whose pitch class sounds elsewhere in the chord first.
  while (asked_.size() > size_t(Guitar::kStrings)) {
    size_t drop = 1;
    for (size_t i = 1; i + 1 < asked_.size(); ++i) {
      bool doubled = false;
      for (size_t j = 0; j < asked_.size(); ++j)
        if (j != i && asked_[j].midi % 12 == asked_[i].midi % 12) doubled = true;
      if (doubled) { drop = i; break; }
    }
    asked_.erase(asked_.begin() + drop);
  }
  // The distortion guitar (GM 31: research/distortion_guitar.md 8.1-8.2). A root and its fifth
  // low on the neck are a power chord: he adds the octave on the next string. And the palm on
  // the chugs: with no marks to read, he hears them from the notes. A low note (below A3) struck
  // again on the same root within 0.4 s, when the last stroke was let go early (held under
  // 0.25 s, or under 0.6 of the gap): the palm on the strings; not on an accent (15 steps louder
  // than the last), not while bending, not on a full chord. The first stroke of a run rings open.
  bool palm31 = false;
  int added = -1;  // (the octave he added)
  if (distorted_ && !asked_.empty()) {
    if (octave_ > 0 && asked_.size() == 2 && asked_[1].midi - asked_[0].midi == 7 && asked_[0].midi < 57) {
      added = asked_[0].midi + 12;
      asked_.push_back({added, asked_[1].velocity});
    }
    int vmax = 0;
    for (const Asked& a : asked_) vmax = std::max(vmax, a.velocity);
    const int root = asked_[0].midi;
    const bool again = root == pmRoot_ && gapBefore_ < 0.4;
    const bool clipped = pmHeld_ >= 0 && (pmHeld_ < 0.25 || pmHeld_ < 0.6 * gapBefore_);
    pmRun_ = again ? pmRun_ + 1 : 0;
    palm31 = chug_ > 0 && root < 57 && again && clipped && vmax < pmVel_ + 15 && std::fabs(roll_) < 0.02 && asked_.size() <= 3;
    pmRoot_ = root;
    pmVel_ = vmax;
    pmHeld_ = -1;
  }
  // Strumming? Many strings at once is a strum; so is a chord of three or
  // more while the hand is already strumming. (Strings that come one after
  // another within 60 ms are found stroke by stroke, below.)
  const bool together = asked_.size() >= 5 || (asked_.size() >= 3 && strumming_ > 0);
  if (together) {
    // Which way. The strumming hand swings like a pendulum, down and up,
    // whether or not it touches the strings: a chord that comes one swing
    // after the last goes the other way, two swings after the same way
    // again (the hand passed over in between), and so on; the swing's
    // length is the shortest gap the chords have come at. After a longer
    // silence: down. When the song's beat is known, a chord on a beat is
    // most often down whatever the pendulum says, the first beat of the bar
    // nearly always. (Most often, not always: he is not a machine.)
    int way = 1;
    if (strumming_ > 0 && gapBefore_ < 1.6) {
      const int swings = std::max(1, int(std::lround(gapBefore_ / std::max(swing_, 0.08))));
      if (swings == 1) swing_ += 0.3 * (gapBefore_ - swing_);
      else if (gapBefore_ < 0.8 * swing_) swing_ = gapBefore_;
      way = (swings & 1) ? -strumWay_ : strumWay_;
      if (ur() < strumFree_) way = -way;
    } else {
      swing_ = 0.3;
    }
    const SongClock& sc = SongClock::get();
    if (sc.valid) {
      const double frac = sc.beat - std::floor(sc.beat), inBar = std::fmod(sc.beat, 4.0);
      const bool onBeat = frac < 0.12 || frac > 0.88, first = inBar < 0.12 || inBar > 3.88;
      if (onBeat && ur() < (first ? 0.97 : 0.85)) way = 1;
      // What the last bar did at this place, if its pattern was clear (three
      // strums or more): a small pull towards doing the same again.
      const int slot = barSlot(sc.beat);
      if (lastBarClear_ && lastBar_[slot] != 0 && lastBar_[slot] != way && ur() < sameAsLastBar_) way = lastBar_[slot];
    }
    strumWay_ = way;
    strumming_ = 1.5;
    noteStrum(way);
    say(stderr, "%s", strumWay_ > 0 ? "down\n" : "up\n");
    if (std::getenv("PJ_GUITAR_STRUMDEBUG")) say(stderr, "  (beat %.2f, %zu notes, gap %.3f s, swing %.3f s)\n", SongClock::get().beat, asked_.size(), gapBefore_, swing_);
  }
  double chordVel = 0;
  for (const Asked& a : asked_) chordVel += std::clamp(a.velocity, 1, 127) / 127.0 / asked_.size();
  strumVel_ += (asked_.size() >= 3 ? 1.0 : 0.35) * (chordVel - strumVel_);
  bool taken[Guitar::kStrings] = {};
  int lowest = Guitar::kStrings, order = 0, minString = 0;
  double fretSum = 0;
  int fretted = 0;
  const size_t count = asked_.size();
  bool idle = true;
  for (int s = 0; s < Guitar::kStrings; ++s) idle = idle && !held_[s];
  const bool lone = count == 1 && idle && gapBefore_ > 0.25;
  for (const Asked& a : asked_) {
    int midi = a.midi;
    while (midi < kOpen[0]) midi += 12;
    while (midi > top) midi -= 12;
    // The string: free, near the hand, low on the neck, open where it can
    // be; above the string the chord's note below this one took.
    int best = -1;
    double cost = 1e9;
    for (int s = 0; s < Guitar::kStrings; ++s) {
      const int fret = midi - kOpen[s];
      if (fret < 0 || fret > g.frets() || taken[s]) continue;
      double c = fret == 0 ? -0.5 : std::fabs(fret - hand_) + 0.15 * fret;
      if (fret > 0 && std::fabs(fret - hand_) > 4) c += 3;       // out of the hand's reach: a shift
      if (lone) {
        // A note by itself with time around it will be made to sing: not on
        // an open string (it cannot be), and where vibrato is easy and
        // sounds best: the B and G strings between the fifth and tenth
        // fret, then the D and the high E.
        static const double kSing[Guitar::kStrings] = {4.0, 3.0, 1.2, 0.0, 0.0, 0.8};
        c = fret == 0 ? 6.0 : kSing[s] + (fret < 5 ? 0.8 * (5 - fret) : fret > 10 ? 0.5 * (fret - 10) : 0.0) + 0.1 * std::fabs(fret - hand_);
      }
      if (held_[s]) c += 6;                                      // a held note would be cut
      else if (note_on_[s] >= 0 && age_[s] < 0.4) c += 1.0;      // still ringing: rather let it
      if (s < minString) c += 4;                                 // the strings in the notes' order
      if (c < cost) { cost = c; best = s; }
    }
    if (best < 0) continue;
    const int fret = midi - kOpen[best];
    taken[best] = true;
    minString = best + 1;
    lowest = std::min(lowest, best);
    if (fret > 0) { fretSum += fret; ++fretted; }
    // The right hand. The thumb takes a chord's lowest note or any note on
    // the three bass strings: more flesh, pushed into the top (a rest
    // stroke); the fingers pluck more along it. A soft, single note's
    // fingers come one after another; a loud chord nearly together.
    const double v = std::clamp(a.velocity, 1, 127) / 127.0;
    const bool thumb = (order == 0 && count > 1) || best < 3;
    const double depth = (0.25e-3 + 2.6e-3 * fastPow(v, 1.4)) * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0)) * (0.9 + 0.2 * ur());
    // Flesh alone when soft; a little nail from the middle; much, and its knock, at the hardest.
    const double e = std::clamp((v - 0.32) / 0.5, 0.0, 1.0), ease = e * e * (3 - 2 * e);
    const double nail = std::clamp((thumb ? 0.5 : 0.8) * ease + 0.2 * (hardness_ - 1), 0.0, 1.0);
    const double clack = (thumb ? 0.5 : 1.0) * std::clamp((v - 0.84) / 0.16, 0.0, 1.0);
    const double angle = thumb ? 0.9 + 0.3 * ur() : 0.6 + 0.3 * ur();
    const double spread = count > 1 ? (0.009 - 0.006 * v) * (0.8 + 0.4 * ur()) : 0;
    double nailNow = nail, angleNow = angle, where = 1, depthNow = depth;
    if (count == 1) {
      // The same note again: each stroke its own.
      repeats_ = a.midi == lastMidi_ ? repeats_ + 1 : 0;
      if (repeats_ > 0) {
        const double slow = std::clamp((gapBefore_ - 0.15) / 0.5, 0.15, 1.0);  // how much room there is to change the hand
        const int dv = a.velocity - lastVel_;
        if (dv > 8) {            // an accent: a rest stroke, nearer the bridge
          angleNow = 1.15 + 0.2 * ur();
          where = 1 - 0.25 * slow;
          nailNow = std::min(1.0, nail + 0.15);
          depthNow *= 1.08;
        } else if (dv < -8) {    // falling away: towards the fingerboard, on the flesh
          where = 1 + 0.45 * slow;
          nailNow = std::max(0.0, nail - 0.3);
          angleNow = 0.45 + 0.2 * ur();
        } else {                 // even: the fingers take turns, and the hand wanders
          nailNow = std::clamp(nail + ((repeats_ & 1) ? 0.14 : -0.10) - (repeats_ % 3 == 2 ? 0.2 : 0.0), 0.0, 1.0);
          angleNow = (repeats_ & 1) ? angle + 0.05 : angle - 0.05;  // (evenly loud: only the colour changes)
          static const double kWander[4] = {1.0, 1.3, 0.95, 0.78};
          where = 1 + (kWander[repeats_ & 3] - 1) * slow;
        }
      }
      // (He means them equally loud unless the velocities say otherwise:
      // further from the bridge or on the flesh the string is pulled deeper
      // for the same loudness.)
      if (repeats_ > 0) depthNow *= fastPow(where, 0.9) * (1 + 1.1 * std::max(0.0, nail - nailNow));
      lastMidi_ = a.midi;
      lastVel_ = a.velocity;
    } else {
      lastMidi_ = -1;
    }
    // A run of strings one after another across the guitar: this one the
    // third or later within 60 ms of the one before, the same way.
    bool strum = together;
    if (!together && count <= 2) {
      const int way = strokeString_[0] >= 0 ? (best > strokeString_[0] ? 1 : best < strokeString_[0] ? -1 : 0) : 0;
      const int wayBefore = strokeString_[1] >= 0 && strokeString_[0] >= 0 ? (strokeString_[0] > strokeString_[1] ? 1 : strokeString_[0] < strokeString_[1] ? -1 : 0) : 0;
      const bool run = way != 0 && way == wayBefore && strokeAgo_[0] < 0.06 && strokeAgo_[1] < 0.12;
      if (run) {
        strumming_ = 1.5;
        strumWay_ = way;
        if (!inRun_) {  // (once a stroke: at its third string)
          say(stderr, "%s", way > 0 ? "down\n" : "up\n");
          noteStrum(way);
        }
      }
      inRun_ = run || (inRun_ && way == strumWay_ && strokeAgo_[0] < 0.06);
      // (While strumming, a string that follows its neighbour that closely is the hand's too.)
      strum = run || (strumming_ > 0 && way != 0 && strokeAgo_[0] < 0.06);
    }
    double delay = order * spread;
    if (strum) {
      // The hand passing over: over the soundhole, flat to the strings; the
      // nails' backs going down, the fingertip coming up.
      const bool down = strumWay_ > 0;
      const int place = together ? (down ? order : int(count) - 1 - order) : 0;  // how far into the stroke this string is
      // (Flesh and nail; coming up nearly all flesh. Whatever the velocity:
      // across the strings a nail gives, it is soft that way; its hard edge
      // and its knock belong to picking alone.)
      nailNow = down ? 0.40 + 0.1 * ur() : 0.15 + 0.1 * ur();
      angleNow = down ? 0.28 + 0.1 * ur() : kPiHalf * 2 - (0.22 + 0.1 * ur());   // along the top, the way the hand goes
      where = 1.45;
      // One hand, one stroke: every string gets much the same of it. How
      // deep the hand reaches is the chord's velocity as a whole (its notes'
      // mean, or for strings that come one by one the mean of the last few),
      // and much evened out: a strumming hand's loud and soft lie close
      // together. Going down the bass strings get a little more; coming up
      // the middle strings do, and the hand is gentle with the highest.
      const double vs = 0.3 + 0.5 * (together ? chordVel : strumVel_);
      depthNow = (0.2e-3 + 2.0e-3 * fastPow(vs, 1.3)) * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0)) * (0.95 + 0.1 * ur());
      const double mid = 1 - std::fabs(best - 2.5) / 2.5;
      depthNow *= down ? 1.0 + 0.15 * (5 - best) / 5.0 : 1.05 * (1 + 0.12 * mid) * (best == 5 ? 0.82 : 1.0);  // (the soft pad needs more of the string for its sound)
      if (together) delay = place * (0.0105 / handSpeed_) * (0.9 + 0.2 * ur());   // the strings 10.5 mm apart
    }
    if (electric_ || steel_) {
      // The pick. Notes together: one sweep, the strumming hand's way; a single note: down and
      // up in turn while they come quickly, down after a rest.
      int way;
      if (count > 1 || strum) {
        way = strum ? strumWay_ : 1;
        if (!together && !strum) delay = order * (0.0105 / handSpeed_) * (0.9 + 0.2 * ur());
      } else {
        if (order == 0) pickWay_ = gapBefore_ < 0.3 ? -pickWay_ : 1;
        way = pickWay_;
      }
      // How far: 0.1-3 mm, a strum's much evened; up strokes lighter. Where: 11 cm from the
      // saddle, a hard stroke nearer the bridge. Down: along the top and a little into it.
      const double vv = strum ? 0.3 + 0.5 * (together ? chordVel : strumVel_) : v;
      depthNow = (0.1e-3 + 2.9e-3 * std::pow(vv, 1.5)) * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0)) * (0.92 + 0.16 * ur()) * (way > 0 ? 1.0 : 0.85);
      where = (1.1 - 0.25 * v) * (0.93 + 0.14 * ur()) / 1.2;
      angleNow = way > 0 ? 0.25 + 0.1 * ur() : kPiHalf * 2 + 0.08 + 0.08 * ur();
      nailNow = std::clamp(0.0012 - 0.0006 * v, 0.0004, 0.002);  // (here: the pick's rise, s)
      if (steel_) {
        // The flatpick on steel: 0.1-2.7 mm, up strokes lighter; 10-13 cm from the saddle (the
        // soundhole's lower edge), the hardest nearer; a strum over the soundhole (15-16 cm),
        // its depth the chord's evened velocity, up strokes 2 dB under.
        const double dynK = (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0)) * (0.92 + 0.16 * ur());
        if (strum) {
          depthNow = (0.2e-3 + 2.0e-3 * fastPow(vv, 1.3)) * dynK * (way > 0 ? 1.0 : 0.8);
          where = 1.3 * (0.95 + 0.1 * ur());
          // (a hand taking more strings moves faster: 8.6 ms a string at three, 5.5 at six)
          if (together) delay = (strumWay_ > 0 ? order : int(count) - 1 - order) * (0.0105 / (handSpeed_ * (1 + 0.12 * (double(count) - 3)))) * (0.9 + 0.2 * ur());
        } else {
          depthNow = (0.1e-3 + 2.6e-3 * std::pow(v, 1.5)) * dynK * (way > 0 ? 1.0 : 0.85);
          where = (1.08 - 0.25 * v - 0.17 * std::clamp((v - 0.85) / 0.15, 0.0, 1.0)) * (0.95 + 0.1 * ur());
        }
      }
      if (order == 0) bendString_ = -1;
      if (fret > 0) bendString_ = best;  // (an open string cannot be pushed: the highest fretted note bends)
    }
    double palm = 0, palmAt = 0.012;
    if (muted_) {
      // The palm, laid on as the string is picked: the harder the stroke the lighter it lies and
      // the nearer the saddle (the hand lifts and draws back for an accent: from 1.4x at the
      // softest to 0.35x at the hardest); now and then (one stroke in six) it rests lighter by
      // itself; its place and weight a little different each time. The pick nearer the bridge.
      palm = kPalm_ * (1.6 - 1.25 * v) * (0.8 + 0.4 * ur()) * (ur() < 1.0 / 6 ? 0.45 + 0.2 * ur() : 1.0);
      palmAt = std::max(0.004, 0.009 + 0.008 * ur() - 0.005 * v);
      where *= 0.6;
    } else if (driven_) {
      where *= distorted_ ? 0.55 : 0.65;  // (a rock player picks over the bridge pickup: ~6 cm; a metal one ~4)
      if (palm31) {
        // GM 31's palm: right at the saddle (3-8 mm: lighter than the funk player's, the amp must
        // still hear a pitch), 0.9 of GM 29's weight; heavier as the run goes on, lighter when
        // picked hard; never twice the same.
        palm = chug_ * kPalm_ * 0.9 * (1.6 - 1.25 * v) * (0.85 + 0.3 * ur()) * (pmRun_ >= 2 ? 1.15 : 1.0);
        palmAt = 0.003 + 0.005 * ur();
      }
    }
    due_.push_back({delay, best, fret, depthNow, angleNow, nailNow, where, strum ? (strumWay_ > 0 ? 1 : 2) : 0, electric_ || steel_ ? 0.0 : clack, palm, palmAt});
    strokeAgo_[2] = strokeAgo_[1]; strokeString_[2] = strokeString_[1];
    strokeAgo_[1] = strokeAgo_[0]; strokeString_[1] = strokeString_[0];
    strokeAgo_[0] = 0; strokeString_[0] = best;
    note_on_[best] = a.midi;
    octaveAdded_[best] = a.midi == added;
    fed_[best] = 0;
    velOf_[best] = a.velocity;
    bloomAt_[best] = 1.2 + 0.8 * ur();
    held_[best] = true;
    age_[best] = 0;
    fretOf_[best] = fret;
    // Whether this note will get the left hand's vibrato, how deep and how fast.
    static const double kString[Guitar::kStrings] = {0.0, 0.15, 0.7, 1.0, 1.0, 0.6};
    const double neck = std::clamp((fret - 2) / 5.0, 0.15, 1.0) * (fret > 0 ? 1 : 0);
    const bool will = lone ? kString[best] * neck > 0.1 : ur() < 0.35 + 0.55 * kString[best] * neck;
    vibCents_[best] = will ? vibrato_ * 14 * kString[best] * neck * (0.6 + 0.8 * ur()) : 0;
    // (GM 31's lead: a single fretted note from E3 up, held, gets the rock player's wide vibrato,
    // the string bent up and back by a quarter to a whole tone: distortion_guitar.md 8.5.)
    if (distorted_ && count == 1 && a.midi >= 52 && fret > 0)
      vibCents_[best] = vibrato_ * wide_ * (40 + 60 * ur()) * (best < 2 ? 0.6 : 1.0);
    vibHz_[best] = 5.0 + 1.2 * ur();
    vibPh_[best] = 0;
    ++order;
  }
  if (fretted) hand_ += 0.6 * (fretSum / fretted - 1 - hand_);  // (the first finger a fret under the notes' middle)
  hand_ = std::clamp(hand_, 1.0, 14.0);
  asked_.clear();
}

void GuitarPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  Guitar& g = m_.guitar();
  if (!asked_.empty()) play();
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    for (int s = 0; s < Guitar::kStrings; ++s) age_[s] += m / fs_;
    sinceNote_ += m / fs_;
    for (double& a : strokeAgo_) a += m / fs_;
    strumming_ = std::max(0.0, strumming_ - m / fs_);
    // The left hand's vibrato, on the notes that are held with nothing new
    // being played: in over 0.4 s, out at once when a note comes or the key
    // is let go.
    if ((++vibTick_ & 3) == 0) {
      const double step = 4 * m / fs_;
      static const double kPull[Guitar::kStrings] = {0.0, 0.3, 0.8, 1.0, 1.0, 0.8};  // how well each string can be pulled
      int heldNow = 0;
      for (int s = 0; s < Guitar::kStrings; ++s) heldNow += held_[s] ? 1 : 0;
      for (int s = 0; s < Guitar::kStrings; ++s) {
        // Feedback (the overdriven guitar): a loud note held alone (or in a dyad or power chord),
        // the player standing where it sings. From 0.12 s it holds (its modes below 2.5 kHz given
        // back what they lose); at its bloom time (1.2-2 s) it blooms: a low note into its octave or
        // twelfth, a middle one swells or goes to its octave, a high one swells.
        if (driven_ && feedback_ > 0 && held_[s] && heldNow <= 3 && velOf_[s] >= 50) {
          if (fed_[s] == 0 && age_[s] > holdAt_) {
            fed_[s] = 1;
            g.hold(s, holdHz_, 1.0, 4.0);
          } else if (fed_[s] == 1 && age_[s] > bloomAt_[s]) {
            fed_[s] = 2;
            const double f = g.frequency(s);
            int k = 1;
            if (f < 150) k = ur() < 0.55 ? 2 : 3;
            else if (f < 300) k = ur() < 0.6 ? 1 : 2;
            // (Grown towards where the note began: bloomGain_ times what the fundamental has
            // now; a harmonic to 0.8 of that, the rest still held while it grows, then let go a
            // second and a half later: the note turns into its harmonic.)
            if (k == 1) g.sustain(s, 1, 1, bloomGain_ * feedback_, 1.5, 0);
            else g.sustain(s, k, 1, 0.8 * bloomGain_ * feedback_, 2.5, 1);
            bloomK_[s] = k;
          } else if (fed_[s] == 2 && bloomK_[s] > 1 && age_[s] > bloomAt_[s] + 1.5) {
            fed_[s] = 3;
            g.hold(s, 0, 0, 0);
          }
        }
        // It comes in once the note has been sounding a while with nothing
        // else played since (what came within 150 ms of its start belongs to
        // the same gesture): after 0.6 s; sooner, 0.45 s, when the music has
        // been busy; later, 0.9 s, and slower, when it has been slow.
        const double slow = std::clamp((pace_ - 0.6) / 0.9, 0.0, 1.0), busy = std::clamp((0.6 - pace_) / 0.35, 0.0, 1.0);
        const double after = 0.6 + 0.3 * slow - 0.15 * busy;
        // The guitarist listens to the band too: another part's note begun
        // since then counts as something happening if it lies near this
        // note: surely within an octave, not at all two octaves off, and
        // between them the less the further (an octave and a half: as
        // often as not).
        double since = sinceNote_;
        const SongClock& clock = SongClock::get();
        if (clock.valid && note_on_[s] >= 0) {
          for (int q = std::max(0, note_on_[s] - 24); q <= std::min(127, note_on_[s] + 24); ++q) {
            const double ago = clock.now - clock.lastOn[q];
            if (ago < 0 || ago >= since) continue;
            const double w = std::clamp((24 - std::abs(q - note_on_[s])) / 12.0, 0.0, 1.0);
            uint32_t h = uint32_t(q * 2654435761u) ^ uint32_t(int64_t(clock.lastOn[q] * 1000) * 40503u);
            h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
            if ((h >> 8) * (1.0 / 16777216.0) < w) since = ago;
          }
        }
        // (After such a note the hand waits again, as long as it waited at first.)
        const bool on = held_[s] && vibCents_[s] > 0 && age_[s] > after && (since > age_[s] - 0.15 || since > after);
        if (on && vib_[s] <= 0) vibHz_[s] = (5.3 - 0.8 * slow + 0.5 * busy) * (0.94 + 0.12 * ur());
        vib_[s] = on ? std::min(1.0, vib_[s] + step / 0.4) : std::max(0.0, vib_[s] - step / 0.05);
        // With nothing else to do, the hand also mends a minor third: equal
        // temperament's is 16 cents narrower than the pure 6 : 5, and a
        // finger can only raise a note: so the upper note of a minor third
        // (against another string of the guitar, or a note the band has
        // just played under it) is pulled up most of the way, and held there.
        double want = 0;
        const bool calm = held_[s] && fretOf_[s] > 0 && kPull[s] > 0 && age_[s] > 0.7 * after && (since > age_[s] - 0.15 || since > after);
        if (calm && pull_ > 0) {
          // The chord, as it sounds on the guitar's strings and in what the
          // band has just played: a minor chord is a root with its minor
          // third (and not one that is only the upper part of a major chord:
          // its third and fifth, or its fifth and seventh, are a minor third
          // apart too, and those are right as the frets have them). In it,
          // above equal temperament: the minor third 13 cents, the minor
          // seventh 17 (a pure fifth over that third), the major seventh 11,
          // the major thirteenth (the sixth) 18.
          bool pc[12] = {};
          for (int o = 0; o < Guitar::kStrings; ++o)
            if (held_[o] && note_on_[o] >= 0) pc[note_on_[o] % 12] = true;
          if (clock.valid)
            for (int q = 24; q < 108; ++q)
              if (clock.now - clock.lastOn[q] < 2.0) pc[q % 12] = true;
          const int p = note_on_[s] % 12;
          for (int r = 0; r < 12; ++r) {
            if (!pc[r] || !pc[(r + 3) % 12] || pc[(r + 4) % 12]) continue;
            if (pc[(r + 8) % 12]) continue;                           // r is the major third of a chord sounding
            if (pc[(r + 5) % 12] && pc[(r + 9) % 12]) continue;       // r is the fifth of a major chord sounding
            const int up = ((p - r) % 12 + 12) % 12;
            want = std::max(want, pull_ * kPull[s] * (up == 3 ? 13.0 : up == 10 ? 17.0 : up == 11 ? 11.0 : up == 9 ? 18.0 : 0.0));
          }
          // In a major chord (a root with its major third): the ninth, 4
          // cents (two pure fifths up from the root).
          for (int r = 0; r < 12; ++r)
            if (pc[r] && pc[(r + 4) % 12] && ((p - r) % 12 + 12) % 12 == 2) want = std::max(want, pull_ * kPull[s] * 4.0);
        }
        mend_[s] += (want - mend_[s]) * std::min(1.0, step / (want > mend_[s] ? 0.25 : 0.06));
        // (A pitch bend: the left hand pushing the last string picked; up to two semitones.)
        // (Steel: only up, there is no bar: a wheel rocked both ways is a finger vibrato that
        // returns to the note and never goes under it. Jyrki: "only up the pitch, never down".)
        const double pushed = (electric_ || steel_) && s == bendString_ ? 200 * (steel_ ? std::max(0.0, roll_) : roll_) : 0.0;
        if (vib_[s] <= 0 && mend_[s] < 0.05 && std::fabs(pushed) < 0.5) { if (vibPh_[s] != 0 || bent_[s]) { g.bend(s, 0); vibPh_[s] = 0; bent_[s] = false; } continue; }
        vibPh_[s] += 2 * M_PI * vibHz_[s] * step;
        // (Pushed up and let back: never flat. On a mended note the vibrato is shallower.)
        g.bend(s, pushed + mend_[s] + vib_[s] * vibCents_[s] * (mend_[s] > 1 ? 0.6 : 1.0) * (0.5 - 0.5 * std::cos(vibPh_[s])));
        bent_[s] = true;
      }
    }
    for (size_t j = 0; j < due_.size();) {
      if ((due_[j].in -= m / fs_) <= 0) {
        const Due& d = due_[j];
        g.fret(d.string, d.fret);
        // Where the hand plucks: 12 cm from the bridge, nearer for a brighter sound.
        const double from = std::clamp(0.12 * std::exp2(1 - brightness_) * d.where, 0.03, 0.24);
        if (steel_) g.pick(d.string, d.depth, from, d.angle, d.nail);
        else if (electric_) {
          // (The palm lies across all six: the strings not played do not ring along either.)
          for (int s = 0; s < Guitar::kStrings; ++s) g.mute(s, d.palm, d.palmAt);
          g.pick(d.string, d.depth, from, d.angle, d.nail);
        }
        else g.pluck(d.string, d.depth, from, d.angle, d.nail, d.strum, d.clack);
        due_.erase(due_.begin() + j);
      } else {
        ++j;
      }
    }
    m_.process(out + i, m);
  }
  blowing_ = !g.asleep() || !due_.empty();
}

// GM 32 (guitarinst.h): every note a harmonic.
HarmonicsPlayer::HarmonicsPlayer(GuitarModel& m) : Performer(m), m_(m) {
  std::fill(note_on_, note_on_ + Guitar::kStrings, -1);
  rate_ = knob("PJ_GHARM_TOUCH", 300.0);  // the fingertip's hold on a mode moving under it (1/s)
  lift_ = knob("PJ_GHARM_LIFT", 1.0);     // how long it stays (x 30-60 ms)
  shift_ = int(std::lround(knob("PJ_GHARM_SHIFT", 0.0)));
}

double HarmonicsPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

void HarmonicsPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  if (asked_.empty()) gap_ = sinceNote_;
  asked_.push_back({midi, velocity});
  sinceNote_ = 0;
}

void HarmonicsPlayer::noteOff(int midi) {
  for (size_t i = 0; i < asked_.size(); ++i)
    if (asked_[i].midi == midi) { asked_.erase(asked_.begin() + i); return; }
  // (A harmonic rings on: the key's release lifts nothing.)
  for (int s = 0; s < Guitar::kStrings; ++s)
    if (note_on_[s] == midi) held_[s] = false;
}

bool HarmonicsPlayer::place(int midi, const bool* taken, int* string, int* fret, int* n) const {
  static const int kOpen[Guitar::kStrings] = {40, 45, 50, 55, 59, 64};
  static const int kUp[5] = {0, 0, 12, 19, 24};  // n-th harmonic: semitones over the stopped note
  const int frets = m_.guitar().frets();
  double best = 1e9;
  for (int s = 0; s < Guitar::kStrings; ++s) {
    if (taken && taken[s]) continue;
    // (A string still sounding: a held note is cut only if nothing else will do; a released one
    // that has rung a while is free.)
    const double busy = held_[s] ? 6.0 : note_on_[s] >= 0 && age_[s] < 0.6 ? 1.0 : 0.0;
    for (int k = 2; k <= 4; ++k) {
      const int f = midi - kOpen[s] - kUp[k];
      if (f < 0 || f > frets) continue;
      double c;
      if (f == 0) c = 0.5 * (k - 2);  // natural: the 12th fret's, the 7th's, the 5th's
      else {
        // Touched: n = 2 anywhere, then n = 3 (past the 21st fret's octave), then n = 4. The
        // left hand near where it is, not too high up the neck (the right hand needs room).
        c = (k == 2 ? 3.5 : k == 3 ? 8.0 : 10.0) + 0.12 * std::fabs(f - hand_) + (f > 12 ? 0.1 * (f - 12) : 0.0);
      }
      // (Where the pickup lies near the harmonic's node, the harmonic is thin: elsewhere if can be.)
      const GuitarSpec& sp = m_.guitar().spec();
      const double len = sp.scale * std::exp2(-f / 12.0);
      c += busy + 8 * std::max(0.0, 0.4 - std::fabs(std::sin(k * M_PI * sp.pickups[sp.select[0]].at[s] / len)));
      if (c < best) { best = c; *string = s; *fret = f; *n = k; }
    }
  }
  return best < 1e8;
}

void HarmonicsPlayer::play() {
  Guitar& g = m_.guitar();
  const GuitarSpec& spec = g.spec();
  std::sort(asked_.begin(), asked_.end(), [](const Asked& a, const Asked& b) { return a.midi < b.midi; });
  if (asked_.size() > size_t(Guitar::kStrings)) asked_.erase(asked_.begin() + 1, asked_.end() - (Guitar::kStrings - 1));
  bool taken[Guitar::kStrings] = {};
  const size_t count = asked_.size();
  int order = 0;
  for (const Asked& a : asked_) {
    // (Below E3 an octave up; above C8 an octave down. PJ_GHARM_SHIFT: semitones up, as some sets
    // play it: FluidR3's sounds two octaves over the key, 24.)
    int midi = a.midi + shift_;
    while (midi < 52) midi += 12;
    while (midi > 108) midi -= 12;
    int s = -1, fret = 0, n = 2;
    while (!place(midi, taken, &s, &fret, &n) && midi > 52) midi -= 12;
    if (s < 0) continue;
    taken[s] = true;
    if (fret > 0) hand_ += 0.5 * (fret - hand_);
    const double v = std::clamp(a.velocity, 1, 127) / 127.0;
    const double len = spec.scale * std::exp2(-fret / 12.0);
    // The pick on the harmonic's antinode nearest the bridge (len / 2n; at most 16 cm: over the
    // neck pickup), nowhere near its node; there mode 2n has its node too, and mode n is heard
    // above its octave.
    const double where = std::clamp(len / (2 * n), 0.035, 0.16) * (0.95 + 0.1 * ur());
    // How much of mode n the pick and the pickup give (the selected pickup's place on the
    // stopped string); the player evens the harmonics out by ear: he digs in where they are weak
    // (an open string, the pickup near the node) and goes lightly high up the neck (the shorter
    // the string, the more of the stroke goes into mode n). (Fitted to the levels of all 53
    // notes played alike: about 1 dB a fret, the ear 0.73 of its dB; within 1.5 dB rms.)
    const GuitarSpec::Pickup& pu = spec.pickups[spec.select[0]];
    const double ear = std::fabs(std::sin(n * M_PI * pu.at[s] / len)) * std::fabs(std::sin(n * M_PI * where / len));
    const double weak = 0.967 * fret - 6 - 0.046 * (midi - 76) + 0.73 * 20 * std::log10(std::max(ear, 0.05)) +
                        (n == 3 ? 1.98 : n == 4 ? 4.03 : fret == 0 ? 1.76 : 0.0);
    const double dig = knob("PJ_GHARM_EVEN", 1.0) > 0 ? std::clamp(fastPow(10.0, -weak / 20), 0.12, 2.2) : 1.0;
    // Single notes down and up in turn while they come quickly; a chord one sweep down.
    int way = 1;
    if (count == 1) { pickWay_ = gap_ < 0.3 ? -pickWay_ : 1; way = pickWay_; }
    const double depth = std::min(5e-3, (0.12e-3 + 2.4e-3 * std::pow(v, 1.5)) * dig * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0)) *
                                            (0.93 + 0.14 * ur()) * (way > 0 ? 1.0 : 0.88));
    const double angle = way > 0 ? 0.25 + 0.1 * ur() : M_PI + 0.08 + 0.08 * ur();
    // A rounder stroke than the clean guitar's (the pick pushed over 2-3.5 ms, not 0.6-1.2): on the
    // antinode, the stroke's quick edge would ring the harmonic's 3rd partial as loud as itself.
    const double rise = std::clamp((0.0035 - 0.0015 * v) * knob("PJ_GHARM_RISE", 1.0), 0.0005, 0.004);
    // The fingertip: on the node from the pick to 30-60 ms after; a little off it (+-0.7 mm).
    const double touch = (0.03 + 0.03 * ur()) * lift_, off = (ur() - 0.5) * 0.0014;
    // A string sounding at another fret is stopped first (the pick comes 4 ms after the asking,
    // always, so the timing stays even).
    if (note_on_[s] >= 0 && fretOf_[s] != fret) g.damp(s, 1500);
    due_.push_back({0.004 + (count > 1 ? order * 0.008 * (0.9 + 0.2 * ur()) : 0.0), s, fret, n, depth, where, angle, rise, touch, off});
    note_on_[s] = a.midi;
    fretOf_[s] = fret;
    nOf_[s] = n;
    held_[s] = true;
    rested_[s] = false;
    age_[s] = 0;
    if (std::getenv("PJ_GHARM_DEBUG"))
      say(stderr, "harmonic %d: string %d fret %d n %d (%s), pick %.0f mm, depth %.2f mm, ear %.3f\n", a.midi, s + 1, fret, n,
                   fret ? "touched" : "natural", where * 1000, depth * 1000, ear);
    ++order;
  }
  hand_ = std::clamp(hand_, 1.0, 14.0);
  asked_.clear();
}

void HarmonicsPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  Guitar& g = m_.guitar();
  if (!asked_.empty()) play();
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double dt = m / fs_;
    tick(dt);
    sinceNote_ += dt;
    bool anyHeld = false;
    for (int s = 0; s < Guitar::kStrings; ++s) { age_[s] += dt; anyHeld = anyHeld || held_[s]; }
    // A rest: the hand lies lightly on the strings let go (they fade in a second or so).
    if (!anyHeld && !hold_ && sinceNote_ > 1.5)
      for (int s = 0; s < Guitar::kStrings; ++s)
        if (note_on_[s] >= 0 && !rested_[s]) { g.damp(s, 5); rested_[s] = true; }
    for (size_t j = 0; j < due_.size();) {
      if ((due_[j].in -= dt) <= 0) {
        const Due& d = due_[j];
        g.fret(d.string, d.fret);
        g.touch(d.string, d.n, d.touch, rate_, 0.004, d.off);
        g.pick(d.string, d.depth, d.where, d.angle, d.rise);
        due_.erase(due_.begin() + j);
      } else {
        ++j;
      }
    }
    m_.process(out + i, m);
  }
  blowing_ = !g.asleep() || !due_.empty();
}

}  // namespace pajupilli
