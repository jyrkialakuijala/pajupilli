// The string ensembles (General MIDI 49 "String Ensemble 1" and 50 "String Ensemble 2"): a string
// orchestra's section, played, not a sample or a pad.
//
// 24 players, each one bowed instrument of its own (src/bass.h): 12 violins, 6 violas, 3 cellos,
// 3 double basses, seated across the stage, the violins on the left, the basses on the right.
// A key is its sounding note. Every section plays the held keys that lie in its register, and
// divides between them as a section does (divisi): the players of a section are shared out
// evenly over its keys, at most so many keys a section (the rest are left to the other
// sections). One key in everyone's register is a unison of the sections that reach it; a chord
// is spread over the orchestra, the basses and cellos on its low notes, the violins on top.
//   - When the keys change, as few players move as will do: a player whose key is still held
//     stays on it; one whose key went up moves to a key that needs players, slurring in its bow
//     (its instrument's own legato), or lifts its bow if nothing is held.
//   - No two players start together: each acts a moment after the key, its own moment each time
//     (about 20 ms give or take 15, the desks further back a little later; PJ_STRINGS_SPREAD x).
//   - No two are alike (Performer::vary, Tube::vary; fixed for the seat): each stops its notes a
//     few cents off, has its own vibrato (rate, depth, when it begins), its own bow speed and
//     pressure, is somewhere else in its bow (the bow changes never fall together), and plays
//     its own instrument: plates, air, losses, bridge and upper resonances a little different,
//     as recorded violins differ (17 of them: 4 to 5 dB apart per third of an octave).
//     That is the section's sound: nothing is chorused.
//   - No two sections are alike either: each has its own moment at every change of the keys and
//     its habits of vibrato, bow speed and pressure; within it every stroke begins somewhere
//     else in each player's bow, so the bow changes are staggered (BowPlayer::vary).
//
// The two ensembles differ as the sampled ones do (the Sound Canvas's "Strings" and "Slow
// Strings"), by how the players play:
//   - String Ensemble 1: together within some 40 ms, a firm start of the stroke, the bow a little
//     nearer the bridge (brighter), a short release;
//   - String Ensemble 2: the players come in one after another over 300 ms and start their
//     strokes softly (the slow attack), the bow towards the fingerboard (darker), and they leave
//     one after another over 350 ms (the long release).
//
// The violins, violas and cellos act on a key 100 ms late (their pizzicato's wait: a key let go
// within it is plucked, which the section then does too); the basses are held back as long.
// Played from a file the keys come that much early (noteLead).
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "ensemble.h"
#include "bassinst.h"
#include "knob.h"

namespace pajupilli {

namespace {

// A section: its instrument, its players, the keys it takes (sounding), how many keys it divides
// into at most, and which it takes when more are held: the highest (+1), the lowest (-1), or
// those nearest its middle (0).
struct StringSection {
  const char* inst;
  int players;
  int lo, hi;
  int maxDiv;
  int prefer;
};
const StringSection kStringSections[] = {
    {"violin", 12, 55, 103, 4, +1},
    {"viola", 6, 48, 72, 3, 0},
    {"cello", 3, 36, 60, 3, -1},
    {"contrabass", 3, 28, 43, 1, -1},
};

}  // namespace

void Ensemble::initStrings(const Instrument& inst, const RoomSpec* room) {
  lo_ = inst.lo;
  hi_ = inst.hi;
  slow_ = std::strcmp(inst.name, "slow-strings") == 0;
  tremolo_ = std::strcmp(inst.name, "tremolo-strings") == 0;
  pizzOnly_ = std::strcmp(inst.name, "pizzicato-strings") == 0;
  std::vector<Member> members;
  for (const StringSection& d : kStringSections) {
    const Instrument* in = findInstrument(d.inst);
    if (!in) continue;
    kidCals_.emplace_back(new Calibration(std::getenv("PJ_NOCAL") ? Calibration() : calibrate(*in, 440, nullptr)));
    const int sec = int(&d - kStringSections);
    for (int i = 0; i < d.players; ++i) {
      // (its seat: the desks further back answer a little later, 4 ms a desk, and everyone has
      // a habit of its own, up to 5 ms early or late)
      srng_ = srng_ * 1664525u + 1013904223u;
      const double habit = 0.004 * (i / 2) + 0.005 * ((srng_ >> 8) / 8388608.0 - 1);
      sp_.push_back({int(members.size()), sec, -1, -1, 0.0, 64, std::strcmp(d.inst, "contrabass") == 0, habit});
      members.push_back(Member{in, kidCals_.back().get()});
    }
  }
  Section s;
  s.ens.reset(new Ensemble(members, false, room));
  s.ens->quiet_ = true;
  s.lo = lo_;
  s.hi = hi_;
  s.gain = float(knob("PJ_STRINGS_GAIN", 2.0));  // (against one of its players alone: measured on a chord)
  // (the pizzicato section ~3 dB under the bowed one, its players' plucks as their own instruments'
  // balance them; the tremolo's bows, at 3/4 of the speed, brought up to the bowed section's level)
  static const double kPizzDb = knob("PJ_STRINGS_PIZZ_DB", -3.0), kTremDb = knob("PJ_STRINGS_TREM_DB", 2.5);
  if (pizzOnly_) s.gain *= float(std::pow(10.0, kPizzDb / 20));
  if (tremolo_) s.gain *= float(std::pow(10.0, kTremDb / 20));
  s.stops = true;
  kids_.push_back(std::move(s));
  strings_ = true;
  // How the players play (above). (The controls' knobs and the director's cues go to every
  // player after this, as to any ensemble.)
  unsigned seat = 0;
  for (Slot& f : kids_[0].ens->f_) {
    Performer& p = f.v->player();
    // (each its own player on its own instrument: Performer::vary, Tube::vary)
    ++seat;
    p.vary(seat);
    f.v->model().vary(seat);
    const bool small = std::strcmp(f.v->instrument().name, "violin") == 0 || std::strcmp(f.v->instrument().name, "viola") == 0;
    if (slow_) {
      p.setHardness(small ? 0.08 : 0.35);  // (the stroke eases in over ~0.2 s: BowPlayer's attack)
      p.setBrightness(0.7);
      // (the release is the players leaving one after another; a slowly lifted bow whistles)
    } else {
      p.setHardness(1.3);
      p.setBrightness(1.25);
    }
    // (tremolo: each bow shaking, ~13 strokes a second on its own clock; pizzicato: every key plucked)
    if (tremolo_) p.setFlutter(0.8);
    if (pizzOnly_)
      if (auto* bp = dynamic_cast<BowPlayer*>(&p)) bp->setPizzicatoOnly(true);
  }
}

// A player's key, at once (the section's own dealing: not the ensemble's, which gives a key to
// one voice).
void Ensemble::drive(int slot, int pitch, int velocity, bool on) {
  Slot& s = f_[slot];
  if (!on) {
    s.held = std::max(0, s.held - 1);
    s.lastOff = now_;
    s.v->player().noteOff(pitch);
    return;
  }
  const bool silent = s.held == 0 && !s.v->player().busy() && now_ - s.lastOff > 1.0;
  if (velGain_ && velocityAsGain(s.v->instrument())) {
    s.vgTo = std::pow(10.0, velocityGainDb(s.v->instrument(), velocity, pitch) / 20);
    s.vgTau = s.held ? 0.06 : 0.005;
    if (silent) s.vg = s.vgTo;
  } else if (velGain_) {
    velocity = velocityRemap(s.v->instrument(), velocity, pitch);
  }
  ++s.held;
  s.lastNote = pitch;
  s.active = true;
  s.tunedNote = -1;
  s.ageNote = pitch;
  s.noteAt = now_;
  s.v->player().noteOn(pitch, velocity);
  if (silent) s.v->model().startFresh();
}

// The held keys dealt to the players (see the top).
void Ensemble::sectAssign() {
  static const double kSpread = knob("PJ_STRINGS_SPREAD", 1.0);
  static const double kLate = knob("PJ_PIZZ_MS", 100) / 1000;
  auto rnd = [&]() {  // 0 .. 1
    srng_ = srng_ * 1664525u + 1013904223u;
    return (srng_ >> 8) / 16777216.0;
  };
  auto gauss = [&]() { return (rnd() + rnd() + rnd() + rnd() - 2) * 1.73; };  // (about a unit normal)
  // A start: 20 ms after the key give or take 15 ms (its standard deviation), anew for every
  // note, on top of the seat's own habit; the slow ensemble's players one after another instead.
  auto enter = [&]() { return slow_ ? 0.30 * rnd() : std::max(0.0, 0.020 + 0.015 * gauss()); };
  const double off = kSpread * (slow_ ? 0.35 : 0.06), move = kSpread * (slow_ ? 0.10 : 0.03);
  const int nsec = int(sizeof kStringSections / sizeof kStringSections[0]);
  for (int sec = 0; sec < nsec; ++sec) {
    const StringSection& d = kStringSections[sec];
    // The section's own moment, for all its players alike and anew every time: the sections do
    // not move as one (8 ms give or take; the violas and cellos answer the violins, 4 and 8 ms).
    const double secAt = 0.004 * std::min(sec, 2) + 0.008 * gauss();
    std::vector<int> who;  // this section's players
    for (int i = 0; i < int(sp_.size()); ++i)
      if (sp_[i].sec == sec) who.push_back(i);
    if (who.empty()) continue;
    // Its keys: the held ones in its register, at most maxDiv (no more than it has players).
    std::vector<int> keys;
    for (int k = d.lo; k <= d.hi; ++k)
      if (sheld_[k] > 0) keys.push_back(k);
    const size_t most = size_t(std::min(d.maxDiv, int(who.size())));
    if (keys.size() > most) {
      if (d.prefer > 0) keys.erase(keys.begin(), keys.end() - most);
      else if (d.prefer < 0) keys.resize(most);
      else {
        const double mid = 0.5 * (d.lo + d.hi);
        std::stable_sort(keys.begin(), keys.end(), [&](int a, int b) { return std::fabs(a - mid) < std::fabs(b - mid); });
        keys.resize(most);
        std::sort(keys.begin(), keys.end());
      }
    }
    // How many players a key: shared evenly, the odd ones to the section's side (the violins'
    // top notes, the others' low ones).
    std::vector<int> want(keys.size(), keys.empty() ? 0 : int(who.size() / keys.size()));
    for (size_t r = 0; !keys.empty() && r < who.size() % keys.size(); ++r) want[d.prefer > 0 ? keys.size() - 1 - r : r] += 1;
    std::vector<int> have(keys.size(), 0), spare;
    for (int i : who) {
      const auto it = std::find(keys.begin(), keys.end(), sp_[i].want);
      if (it == keys.end()) spare.push_back(i);       // its key is gone (or it rests)
      else if (++have[it - keys.begin()] > want[it - keys.begin()]) { --have[it - keys.begin()]; spare.push_back(i); }  // one too many there
    }
    size_t next = 0;
    for (size_t k = 0; k < keys.size(); ++k)
      for (; have[k] < want[k] && next < spare.size(); ++have[k]) {
        SectPlayer& p = sp_[spare[next++]];
        const bool fresh = p.want < 0;
        p.want = keys[k];
        p.vel = std::clamp(sheld_[keys[k]] + int(13 * rnd()) - 6, 1, 127);
        p.at = now_ + std::max(0.0, kSpread * ((fresh ? enter() : move * rnd()) + p.habit + secAt)) + (p.late ? kLate : 0);
      }
    for (; next < spare.size(); ++next) {  // nothing for it: it lifts its bow
      SectPlayer& p = sp_[spare[next]];
      if (p.want < 0) continue;
      p.want = -1;
      p.at = now_ + off * rnd() + (p.late ? kLate : 0);
    }
  }
}

void Ensemble::sectOn(int key, int velocity) {
  key = std::clamp(key, lo_, hi_);
  sheld_[key] = std::clamp(velocity, 1, 127);
  sectAssign();
}

void Ensemble::sectOff(int key) {
  key = std::clamp(key, lo_, hi_);
  if (sheld_[key] == 0) return;
  sheld_[key] = 0;
  sectAssign();
}

void Ensemble::sectRender(float* left, float* right, int n) {
  Ensemble& kid = *kids_[0].ens;
  const float g = kids_[0].gain;
  hitL_.resize(n);
  hitR_.resize(n);
  for (int done = 0; done < n;) {
    for (SectPlayer& p : sp_) {
      if (p.want == p.cur || now_ < p.at) continue;
      // To its new key before it leaves the old one: a slur in the same bow.
      if (p.want >= 0) kid.drive(p.slot, p.want, p.vel, true);
      if (p.cur >= 0) kid.drive(p.slot, p.cur, 0, false);
      p.cur = p.want;
    }
    const int m = std::min(n - done, 128);
    kid.render(hitL_.data(), hitR_.data(), m);
    for (int i = 0; i < m; ++i) {
      left[done + i] = g * hitL_[i];
      right[done + i] = g * hitR_[i];
    }
    now_ += m / 48000.0;
    done += m;
  }
}

}  // namespace pajupilli
