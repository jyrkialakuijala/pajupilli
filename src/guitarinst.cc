#include "guitarinst.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "knob.h"
#include "songclock.h"

namespace pajupilli {

namespace {
constexpr double kPiHalf = 1.5707963267948966;
}

bool guitarInstrument(const Instrument& inst) { return std::strcmp(inst.name, "nylon-guitar") == 0; }

GuitarModel::GuitarModel(const Instrument& inst) : Tube(inst), g_(inst.fs) {}

void GuitarModel::resetAir() {
  Tube::resetAir();
  g_.reset();
}

float GuitarModel::sample() {
  last_ = g_.sample();
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
}

double GuitarPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

void GuitarPlayer::noteOn(int midi, int velocity) {
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
  for (int s = 0; s < Guitar::kStrings; ++s)
    if (note_on_[s] == midi && held_[s]) {
      held_[s] = false;
      if (!hold_) m_.guitar().damp(s, 45);
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
  if (asked_.size() > size_t(Guitar::kStrings)) asked_.resize(Guitar::kStrings);
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
    std::fprintf(stderr, strumWay_ > 0 ? "down\n" : "up\n");
    if (std::getenv("PJ_GUITAR_STRUMDEBUG")) std::fprintf(stderr, "  (beat %.2f, %zu notes, gap %.3f s, swing %.3f s)\n", SongClock::get().beat, asked_.size(), gapBefore_, swing_);
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
    const double depth = (0.25e-3 + 2.6e-3 * std::pow(v, 1.4)) * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0)) * (0.9 + 0.2 * ur());
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
      if (repeats_ > 0) depthNow *= std::pow(where, 0.9) * (1 + 1.1 * std::max(0.0, nail - nailNow));
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
          std::fprintf(stderr, way > 0 ? "down\n" : "up\n");
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
      depthNow = (0.2e-3 + 2.0e-3 * std::pow(vs, 1.3)) * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0)) * (0.95 + 0.1 * ur());
      const double mid = 1 - std::fabs(best - 2.5) / 2.5;
      depthNow *= down ? 1.0 + 0.15 * (5 - best) / 5.0 : 1.05 * (1 + 0.12 * mid) * (best == 5 ? 0.82 : 1.0);  // (the soft pad needs more of the string for its sound)
      if (together) delay = place * (0.0105 / handSpeed_) * (0.9 + 0.2 * ur());   // the strings 10.5 mm apart
    }
    due_.push_back({delay, best, fret, depthNow, angleNow, nailNow, where, strum ? (strumWay_ > 0 ? 1 : 2) : 0, clack});
    strokeAgo_[2] = strokeAgo_[1]; strokeString_[2] = strokeString_[1];
    strokeAgo_[1] = strokeAgo_[0]; strokeString_[1] = strokeString_[0];
    strokeAgo_[0] = 0; strokeString_[0] = best;
    note_on_[best] = a.midi;
    held_[best] = true;
    age_[best] = 0;
    fretOf_[best] = fret;
    // Whether this note will get the left hand's vibrato, how deep and how fast.
    static const double kString[Guitar::kStrings] = {0.0, 0.15, 0.7, 1.0, 1.0, 0.6};
    const double neck = std::clamp((fret - 2) / 5.0, 0.15, 1.0) * (fret > 0 ? 1 : 0);
    const bool will = lone ? kString[best] * neck > 0.1 : ur() < 0.35 + 0.55 * kString[best] * neck;
    vibCents_[best] = will ? vibrato_ * 14 * kString[best] * neck * (0.6 + 0.8 * ur()) : 0;
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
      for (int s = 0; s < Guitar::kStrings; ++s) {
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
        if (vib_[s] <= 0 && mend_[s] < 0.05) { if (vibPh_[s] != 0 || bent_[s]) { g.bend(s, 0); vibPh_[s] = 0; bent_[s] = false; } continue; }
        vibPh_[s] += 2 * M_PI * vibHz_[s] * step;
        // (Pushed up and let back: never flat. On a mended note the vibrato is shallower.)
        g.bend(s, mend_[s] + vib_[s] * vibCents_[s] * (mend_[s] > 1 ? 0.6 : 1.0) * (0.5 - 0.5 * std::cos(vibPh_[s])));
        bent_[s] = true;
      }
    }
    for (size_t j = 0; j < due_.size();) {
      if ((due_[j].in -= m / fs_) <= 0) {
        const Due& d = due_[j];
        g.fret(d.string, d.fret);
        // Where the hand plucks: 12 cm from the bridge, nearer for a brighter sound.
        const double from = std::clamp(0.12 * std::pow(2.0, 1 - brightness_) * d.where, 0.03, 0.24);
        g.pluck(d.string, d.depth, from, d.angle, d.nail, d.strum, d.clack);
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
