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
#include "vmath.h"

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

bool gShareOn = false;  // (-play: Ensemble::shareStrings)

}  // namespace

// --- The shared orchestra. A played file's string parts (String Ensemble 1 and 2, Tremolo
// Strings, Pizzicato Strings, on however many channels) are one string orchestra, as a score's
// are: 24 players, not 24 a part. Each part is a client: its held keys, its way of playing, its
// fader. Every section divides between all the clients' keys in its register (each client's own
// keys chosen as a part alone chooses them); a player on a client's key plays that client's way
// (bowed, the slow ensemble's eased strokes, tremolo, plucked), at that client's level (the
// file's volume and expression: the player's own gain, into the room too), under that client's
// cues. A player moving between two keys of one client slurs in its bow; to another client's key
// it lifts and starts anew. The first client renders the orchestra; the others' outputs are
// silent. (PJ_STRINGS_SHARED=0: an orchestra a part, as before.)
struct Ensemble::StringClient {
  int held[128] = {};  // the held keys' velocities (0: up)
  bool slow = false, tremolo = false, pizz = false;
  float gain = 1;      // its section's level (initStrings), x
  float fader = 1;     // the file's (setShareGain)
  float comp = 1;      // sharing's: its players as loud together as the orchestra it would have alone
  int self = -1;       // its channel in the band (the tuning by ear, src/windear.cc)
  bool hasCue = false;
  Cue cue;
  std::vector<std::pair<void (Performer::*)(double), double>> sets;  // its players' settings (all())
};

struct Ensemble::StringShare {
  std::unique_ptr<Ensemble> ens;  // the players
  std::vector<std::unique_ptr<Calibration>> cals;
  struct Seat {
    int slot, sec;
    int cur = -1, want = -1;        // the key it plays; the key it is to play (-1: none)
    int curWho = -1, wantWho = -1;  // ... and whose (a client)
    int lastWho = -1;               // whose its sound still is (a tail ringing on)
    double at = 0;
    int vel = 64;
    bool late = false;
    double habit = 0;
  };
  std::vector<Seat> seats;
  std::vector<std::shared_ptr<StringClient>> clients;
  double now = 0;
  unsigned rng = 2026;
};

std::weak_ptr<Ensemble::StringShare>& Ensemble::theShare() {
  static std::weak_ptr<StringShare> w;
  return w;
}

void Ensemble::shareStrings(bool on) {
  static const bool kOn = knob("PJ_STRINGS_SHARED", 1) != 0;
  gShareOn = on && kOn;
}

Ensemble* Ensemble::shareKid() const { return share_ ? share_->ens.get() : nullptr; }

// The players listen (the tuning by ear): a part's own orchestra hears the band but its own
// channel; a shared one's players each but the channel they play for.
void Ensemble::stringsBand(const SoundingSet* band, int self, int fam) {
  if (share_) {
    client_->self = self;
    share_->ens->setBand(band, -1, fam);
    return;
  }
  if (!kids_.empty()) kids_[0].ens->setBand(band, self, fam);
}

void Ensemble::setShareGain(float g) {
  if (client_) client_->fader = g;
  else choirFader(g);
}

void Ensemble::shareAll(void (Performer::*set)(double), double x) {
  bool found = false;
  for (auto& kv : client_->sets)
    if (kv.first == set) { kv.second = x; found = true; }
  if (!found) client_->sets.push_back({set, x});
  for (const StringShare::Seat& t : share_->seats)
    if (t.curWho == clientIdx_ || (t.cur < 0 && t.lastWho == clientIdx_)) (share_->ens->f_[t.slot].v->player().*set)(x);
}

void Ensemble::shareCue(const Cue& c) {
  client_->cue = c;
  client_->hasCue = true;
  for (const StringShare::Seat& t : share_->seats)
    if (t.curWho == clientIdx_) share_->ens->f_[t.slot].v->player().direct(c);
}

void Ensemble::initStrings(const Instrument& inst, const RoomSpec* room) {
  lo_ = inst.lo;
  hi_ = inst.hi;
  slow_ = std::strcmp(inst.name, "slow-strings") == 0;
  tremolo_ = std::strcmp(inst.name, "tremolo-strings") == 0;
  pizzOnly_ = std::strcmp(inst.name, "pizzicato-strings") == 0;
  static const double kPizzDb0 = knob("PJ_STRINGS_PIZZ_DB", -3.0), kTremDb0 = knob("PJ_STRINGS_TREM_DB", 2.5);
  if (gShareOn) {
    share_ = theShare().lock();
    if (!share_) {  // the first string part: the orchestra is built, in this part's room
      share_.reset(new StringShare);
      theShare() = share_;
      std::vector<Member> members;
      for (const StringSection& d : kStringSections) {
        const Instrument* in = findInstrument(d.inst);
        if (!in) continue;
        share_->cals.emplace_back(new Calibration(std::getenv("PJ_NOCAL") ? Calibration() : calibrate(*in, 440, nullptr)));
        const int sec = int(&d - kStringSections);
        for (int i = 0; i < d.players; ++i) {
          share_->rng = share_->rng * 1664525u + 1013904223u;
          StringShare::Seat t;
          t.slot = int(members.size());
          t.sec = sec;
          t.late = std::strcmp(d.inst, "contrabass") == 0;
          t.habit = 0.004 * (i / 2) + 0.005 * ((share_->rng >> 8) / 8388608.0 - 1);
          share_->seats.push_back(t);
          members.push_back(Member{in, share_->cals.back().get()});
        }
      }
      share_->ens.reset(new Ensemble(members, false, room));
      share_->ens->quiet_ = true;
      unsigned seat = 0;
      for (Slot& f : share_->ens->f_) {  // (each its own player on its own instrument)
        ++seat;
        f.v->player().vary(seat);
        f.v->model().vary(seat);
      }
    }
    client_.reset(new StringClient);
    client_->slow = slow_;
    client_->tremolo = tremolo_;
    client_->pizz = pizzOnly_;
    client_->gain = float(knob("PJ_STRINGS_GAIN", 2.0) * (pizzOnly_ ? fastPow(10.0, kPizzDb0 / 20) : tremolo_ ? fastPow(10.0, kTremDb0 / 20) : 1.0));
    clientIdx_ = int(share_->clients.size());
    share_->clients.push_back(client_);
    strings_ = true;
    return;
  }
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
  if (pizzOnly_) s.gain *= float(fastPow(10.0, kPizzDb / 20));
  if (tremolo_) s.gain *= float(fastPow(10.0, kTremDb / 20));
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
    StringClient way;
    way.slow = slow_;
    way.tremolo = tremolo_;
    way.pizz = pizzOnly_;
    stringWay(p, f.v->instrument(), way);
  }
}

// A player takes a client's way of playing (and its cue, its settings).
void Ensemble::stringWay(Performer& p, const Instrument& in, const StringClient& c) {
  const bool small = std::strcmp(in.name, "violin") == 0 || std::strcmp(in.name, "viola") == 0;
  if (c.slow) {
    p.setHardness(small ? 0.08 : 0.35);  // (the stroke eases in over ~0.2 s: BowPlayer's attack)
    p.setBrightness(0.7);
    // (the release is the players leaving one after another; a slowly lifted bow whistles)
  } else {
    p.setHardness(1.3);
    p.setBrightness(1.25);
  }
  // (tremolo: each bow shaking, ~13 strokes a second on its own clock; pizzicato: every key plucked)
  p.setFlutter(c.tremolo ? 0.8 : 0.0);
  if (auto* bp = dynamic_cast<BowPlayer*>(&p)) bp->setPizzicatoOnly(c.pizz);
  if (c.hasCue) p.direct(c.cue);
  for (const auto& kv : c.sets) (p.*kv.first)(kv.second);
}

// The shared orchestra's dealing: as sectAssign below, over every client's keys.
void Ensemble::shareAssign() {
  static const double kSpread = knob("PJ_STRINGS_SPREAD", 1.0);
  static const double kLate = knob("PJ_PIZZ_MS", 100) / 1000;
  StringShare& sh = *share_;
  auto rnd = [&]() {  // 0 .. 1
    sh.rng = sh.rng * 1664525u + 1013904223u;
    return (sh.rng >> 8) / 16777216.0;
  };
  auto gauss = [&]() { return (rnd() + rnd() + rnd() + rnd() - 2) * 1.73; };
  const int nsec = int(sizeof kStringSections / sizeof kStringSections[0]);
  struct Entry { int who, key; };
  for (int sec = 0; sec < nsec; ++sec) {
    const StringSection& d = kStringSections[sec];
    const double secAt = 0.004 * std::min(sec, 2) + 0.008 * gauss();
    std::vector<int> who;  // this section's players
    for (int i = 0; i < int(sh.seats.size()); ++i)
      if (sh.seats[i].sec == sec) who.push_back(i);
    if (who.empty()) continue;
    // Each client's keys in the section's register, at most maxDiv of them (as a part alone).
    std::vector<std::vector<int>> mine(sh.clients.size());
    size_t longest = 0;
    for (size_t c = 0; c < sh.clients.size(); ++c) {
      std::vector<int>& keys = mine[c];
      for (int k = d.lo; k <= d.hi; ++k)
        if (sh.clients[c]->held[k] > 0) keys.push_back(k);
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
      longest = std::max(longest, keys.size());
    }
    // All of them, a key of each client in turn (the section's preferred end first), no more
    // than there are players.
    std::vector<Entry> entries;
    for (size_t r = 0; r < longest && entries.size() < who.size(); ++r)
      for (size_t c = 0; c < mine.size() && entries.size() < who.size(); ++c)
        if (r < mine[c].size()) entries.push_back({int(c), d.prefer > 0 ? mine[c][mine[c].size() - 1 - r] : mine[c][r]});
    // How many players a key: shared evenly, the odd ones to the first.
    std::vector<int> want(entries.size(), entries.empty() ? 0 : int(who.size() / entries.size()));
    for (size_t r = 0; !entries.empty() && r < who.size() % entries.size(); ++r) want[r] += 1;
    std::vector<int> have(entries.size(), 0), spare;
    for (int i : who) {
      const StringShare::Seat& t = sh.seats[i];
      size_t e = 0;
      while (e < entries.size() && !(entries[e].who == t.wantWho && entries[e].key == t.want)) ++e;
      if (e == entries.size()) spare.push_back(i);  // its key is gone (or it rests)
      else if (++have[e] > want[e]) { --have[e]; spare.push_back(i); }  // one too many there
    }
    size_t next = 0;
    for (size_t e = 0; e < entries.size(); ++e)
      for (; have[e] < want[e] && next < spare.size(); ++have[e]) {
        StringShare::Seat& t = sh.seats[spare[next++]];
        const StringClient& c = *sh.clients[entries[e].who];
        const bool fresh = t.want < 0 || t.wantWho != entries[e].who;
        t.want = entries[e].key;
        t.wantWho = entries[e].who;
        t.vel = std::clamp(c.held[t.want] + int(13 * rnd()) - 6, 1, 127);
        const double enter = c.slow ? 0.30 * rnd() : std::max(0.0, 0.020 + 0.015 * gauss());
        const double move = kSpread * (c.slow ? 0.10 : 0.03);
        t.at = sh.now + std::max(0.0, kSpread * ((fresh ? enter : move * rnd()) + t.habit + secAt)) + (t.late ? kLate : 0);
      }
    for (; next < spare.size(); ++next) {  // nothing for it: it lifts its bow
      StringShare::Seat& t = sh.seats[spare[next]];
      if (t.want < 0) continue;
      const bool slow = t.wantWho >= 0 && sh.clients[t.wantWho]->slow;
      t.want = -1;
      t.wantWho = -1;
      t.at = sh.now + kSpread * (slow ? 0.35 : 0.06) * rnd() + (t.late ? kLate : 0);
    }
  }
}

void Ensemble::shareRender(float* left, float* right, int n) {
  std::fill(left, left + n, 0.0f);
  std::fill(right, right + n, 0.0f);
  if (clientIdx_ != 0) return;  // (the first client plays the orchestra)
  StringShare& sh = *share_;
  Ensemble& kid = *sh.ens;
  // A client that shares a section has fewer players than the part alone would (every player
  // of each section it has keys in): the file's balance was written for that part alone, so its
  // players are turned up to make the same power together (PJ_STRINGS_SHARE_COMP, 1; at most
  // 6 dB: a part left with a quarter of its players).
  static const double kComp = knob("PJ_STRINGS_SHARE_COMP", 1.0);
  const int nsec = int(sizeof kStringSections / sizeof kStringSections[0]);
  for (size_t c = 0; c < sh.clients.size(); ++c) {
    int has = 0, alone = 0;
    for (int sec = 0; sec < nsec; ++sec) {
      int mine = 0, all = 0;
      for (const StringShare::Seat& t : sh.seats) {
        all += t.sec == sec;
        mine += t.sec == sec && t.wantWho == int(c);
      }
      bool keys = false;
      for (int k = kStringSections[sec].lo; k <= kStringSections[sec].hi && !keys; ++k) keys = sh.clients[c]->held[k] > 0;
      has += mine;
      if (keys) alone += all;
    }
    if (has > 0 && alone > 0) sh.clients[c]->comp = float(std::pow(std::clamp(double(alone) / has, 1.0, 4.0), 0.5 * kComp));
  }
  for (int done = 0; done < n;) {
    for (StringShare::Seat& t : sh.seats) {
      Slot& f = kid.f_[t.slot];
      // (its level: the client's whose key it plays, or whose note still rings in it)
      if (t.lastWho >= 0) f.sg = sh.clients[t.lastWho]->gain * sh.clients[t.lastWho]->fader * sh.clients[t.lastWho]->comp;
      if ((t.want == t.cur && t.wantWho == t.curWho) || sh.now < t.at) continue;
      if (t.want >= 0 && t.wantWho == t.curWho) {
        // To its new key before it leaves the old one: a slur in the same bow.
        kid.drive(t.slot, t.want, t.vel, true);
        if (t.cur >= 0) kid.drive(t.slot, t.cur, 0, false);
      } else {
        if (t.cur >= 0) kid.drive(t.slot, t.cur, 0, false);
        if (t.want >= 0) {  // another client's key: its way of playing, its level, a new start
          const StringClient& c = *sh.clients[t.wantWho];
          stringWay(f.v->player(), f.v->instrument(), c);
          f.sg = c.gain * c.fader * c.comp;
          f.earSelf = c.self;
          t.lastWho = t.wantWho;
          kid.drive(t.slot, t.want, t.vel, true);
        }
      }
      t.cur = t.want;
      t.curWho = t.wantWho;
    }
    const int m = std::min(n - done, 128);
    kid.render(left + done, right + done, m);
    sh.now += m / 48000.0;
    done += m;
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
    s.vgTo = fastPow(10.0, velocityGainDb(s.v->instrument(), velocity, pitch) / 20);
    s.vgTau = s.held ? 0.06 : 0.005;
    if (silent) s.vg = s.vgTo * s.sg;
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
  if (silent && !velGain_) s.vg = s.vgTo * s.sg;
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
  if (share_) {
    client_->held[key] = std::clamp(velocity, 1, 127);
    shareAssign();
    return;
  }
  sheld_[key] = std::clamp(velocity, 1, 127);
  sectAssign();
}

void Ensemble::sectOff(int key) {
  key = std::clamp(key, lo_, hi_);
  if (share_) {
    if (client_->held[key] == 0) return;
    client_->held[key] = 0;
    shareAssign();
    return;
  }
  if (sheld_[key] == 0) return;
  sheld_[key] = 0;
  sectAssign();
}

void Ensemble::sectRender(float* left, float* right, int n) {
  if (share_) { shareRender(left, right, n); return; }
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
