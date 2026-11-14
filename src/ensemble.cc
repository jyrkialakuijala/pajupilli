#include "ensemble.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "knob.h"
#include "lead.h"

namespace pajupilli {

namespace {

// Interval classes (semitones mod 12): the nearest simple ratio's offset from
// equal temperament (cents) and how much it matters (simpler = more).
struct Just { double cents, weight; };
constexpr Just kJust[12] = {
    {0.0, 1.0},     // 1/1, 2/1
    {11.7, 0.1},    // 16/15
    {3.9, 0.15},    // 9/8
    {15.6, 0.4},    // 6/5
    {-13.7, 0.5},   // 5/4
    {-2.0, 0.6},    // 4/3
    {-9.8, 0.05},   // 45/32
    {2.0, 0.8},     // 3/2
    {13.7, 0.35},   // 8/5
    {-15.6, 0.45},  // 5/3
    {-31.2, 0.3},   // 7/4 (the harmonic seventh: 1.75 x the root)
    {-11.7, 0.1},   // 15/8
};
constexpr double kToTempered = 0.05;  // pull of a new note to equal temperament
constexpr double kStay = 1.0;         // pull of a sounding note to where it is
constexpr double kYoung = 0.150;      // a note this young (s) is still being placed
constexpr double kMean = 4.0;         // pull of the young notes' mean to equal temperament
const double kMaxMove_ = knob("PJ_JIMOVE", 3);  // the most a sounding note's tuning moves (c)
const double kHeal_ = knob("PJ_JIHEAL", 3);     // healing towards 0 c: time constant (s; 0: none)
constexpr double kHealEvery = 0.1;              // ... re-solved this often
constexpr double kMaxCents = 35;  // (a 7/4 above a root held at 0 c: -31.2 c)

const char* name(int midi) {
  static const char* n[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  return n[midi % 12];
}

// Where each instrument is at ease (sounding MIDI), and how readily it is
// chosen when several would do: the flutes lead.
struct Comfort { const char* name; int lo, hi; double pref; };
constexpr Comfort kComfort[] = {
    {"flute", 60, 91, 0}, {"flute-c", 60, 91, 0}, {"piccolo", 79, 103, 1},
    {"alto", 55, 79, 1}, {"bass", 48, 69, 1.5},
};
constexpr double kCross = 6;    // two voices crossing (a low instrument above a higher one)

}  // namespace

Ensemble::Ensemble(const std::vector<Member>& members, bool verbose, const RoomSpec* room)
    : verbose_(verbose), ji_(std::getenv("PJ_JI") ? std::atof(std::getenv("PJ_JI")) != 0 : true) {
  init(members, room);
}

Ensemble::Ensemble(const Instrument& inst, const Calibration& cal, int maxFlutes, bool verbose,
                   const RoomSpec* room)
    : verbose_(verbose), ji_(std::getenv("PJ_JI") ? std::atof(std::getenv("PJ_JI")) != 0 : true) {
  // (A plucked instrument is one voice: its player takes the keys to its strings.)
  init(std::vector<Member>(inst.oneVoice() ? 1 : std::max(1, maxFlutes), Member{&inst, &cal}), room);
}

void Ensemble::init(const std::vector<Member>& members, const RoomSpec* room) {
  std::fill(owner_, owner_ + 128, -1);
  std::fill(pitch_, pitch_ + 128, -1);
  f_.resize(members.size());
  int same = 0;
  for (int i = 0; i < int(f_.size()); ++i) {
    const Instrument& in = *members[i].inst;
    Slot& s = f_[i];
    s.v.reset(new Voice(in));
    s.v->seed(unsigned(i));  // each flute its own breath and wander
    s.v->player().setCalibration(*members[i].cal);
    s.lo = in.lo + in.transpose;
    s.hi = in.hi + in.transpose;
    s.rank = s.lo;
    s.easyLo = s.lo;
    s.easyHi = s.hi - 5;
    s.pref = 1;
    for (const Comfort& c : kComfort)
      if (std::strcmp(c.name, in.name) == 0) { s.easyLo = c.lo; s.easyHi = c.hi; s.pref = c.pref; }
    lo_ = std::min(lo_, s.lo);
    hi_ = std::max(hi_, s.hi);
    mixed_ |= members[i].inst != members[0].inst;
    int nth = 0, of = 0;
    for (int j = 0; j < int(members.size()); ++j)
      if (members[j].inst == members[i].inst) { ++of; if (j <= i) ++nth; }
    s.label = std::string(std::strncmp(in.name, "flute", 5) == 0 ? "flute" : in.name) +
              (of > 1 ? " " + std::to_string(nth) : "");
    same += of > 1;
  }
  (void)same;
  {
    // Without a room, the synth voices each get a place in the stereo
    // field (an analog polysynth's voice pan): spread by the golden ratio,
    // so the first few voices are already far apart, within +-70 %.
    // Equal power, centre at unity (as the mono mix). PJ_PANWIDTH sets it.
    static const double width = knob("PJ_PANWIDTH", 0.7);
    int k = 0;
    for (Slot& s : f_) {
      if (!s.v->instrument().circuit() || f_.size() < 2) continue;
      double u = 0.5 + 0.6180339887 * k++;
      u -= std::floor(u);
      const double p = width * (2 * u - 1), a = (p + 1) * M_PI / 4;
      s.gl = std::sqrt(2.0) * std::cos(a);
      s.gr = std::sqrt(2.0) * std::sin(a);
      panned_ = true;
    }
  }
  if (room && room->volume > 0) {
    // Seats across the stage, high instruments on the left, 1 m apart.
    const int n = int(f_.size());
    std::vector<int> order(n);
    for (int i = 0; i < n; ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return f_[a].rank > f_[b].rank; });
    std::vector<double> x(n);
    const double gap = n > 6 ? 0.8 : 1.0;
    for (int k = 0; k < n; ++k) x[order[k]] = (k - (n - 1) / 2.0) * gap;
    // The channels' rooms: small is smaller than the main one, large larger
    // (or the same where the main one already is the smallest or largest).
    const RoomSpec* spec[3] = {findRoom(room->volume > 150 ? "studio" : "booth"), room, findRoom("gated")};
    for (int c = 0; c < 3; ++c) {
      std::vector<double> xc;
      roomSlots_[c].clear();
      for (int i = 0; i < n; ++i)
        if (reverbChannel(f_[i].v->instrument()) == c) {
          roomSlots_[c].push_back(i);
          xc.push_back(x[i]);
        }
      if (!xc.empty()) rooms_[c].reset(new Room(*spec[c], xc));
    }
    room_ = true;
    for (Slot& s : f_) s.room.assign(Room::kBlock, 0.0f);
  }
}

// Which reverb channel an instrument plays into: the synth basses a small
// room (their low end stays clear); the toms and drums the gated reverb (a
// big live room, its ambience compressed and gated: Phil Collins); the rest
// the main one.
int Ensemble::reverbChannel(const Instrument& in) {
  if (std::strcmp(in.family, "drum") == 0 || std::strcmp(in.family, "tom") == 0) return 2;  // gated
  if (std::strcmp(in.family, "bass") == 0) return 0;
  return 1;
}

std::string Ensemble::describe() const {
  std::string d;
  for (const Slot& s : f_) d += (d.empty() ? "" : ", ") + s.label;
  if (!mixed_ && f_.size() > 1) d = std::to_string(f_.size()) + " x " + f_[0].label.substr(0, f_[0].label.find(' '));
  static const char* const kChannel[3] = {"small", "main", "gated"};
  for (int c = 0; c < 3; ++c) {
    if (!rooms_[c]) continue;
    char b[128];
    std::snprintf(b, sizeof b, "; %s room: %s (%.0f m3, RT60 %.1f s)", kChannel[c], rooms_[c]->spec().name,
                  rooms_[c]->spec().volume, rooms_[c]->spec().rt60);
    d += b;
  }
  return d;
}

int Ensemble::holding() const {
  int h = 0;
  for (const Slot& s : f_) h += s.held > 0;
  return h;
}

int Ensemble::fold(int f, int key, int* octaves) const {
  const Slot& s = f_[f];
  if (octaves) *octaves = 0;
  // An ensemble never moves a note by an octave: only instruments that reach it.
  if (mixed_) return key >= s.lo && key <= s.hi ? key : -1;
  int p = key, o = 0;
  while (p < s.lo) { p += 12; ++o; }
  while (p > s.hi) { p -= 12; ++o; }
  if (octaves) *octaves = o;
  return p >= s.lo ? p : -1;
}

double Ensemble::cost(int f, int p, int octaves) const {
  const Slot& s = f_[f];
  if (!mixed_)  // one semitone weighs as much as 0.1 s since the flute's last key went up
    return s.lastNote < 0 ? 1e9 + f : std::abs(s.lastNote - p) + 10 * (now_ - s.lastOff);
  const double strain = std::max(0, s.easyLo - p) + std::max(0, p - s.easyHi);
  const double line = s.lastNote < 0 ? 2 : std::min(2.0, 0.2 * std::abs(s.lastNote - p) + 2 * (now_ - s.lastOff));
  return strain + s.pref + line + 24 * octaves;
}

double Ensemble::crossing(int f, int p, int g, int q) const {
  if (!mixed_) return 0;
  const int a = f_[f].rank, b = f_[g].rank;
  return (a < b && p > q) || (a > b && p < q) ? kCross : 0;
}

int Ensemble::pickFree(int key, int* pitch) const {
  // Near in pitch and recently played: a line keeps to its flute; in an
  // ensemble also the instrument the note suits, not crossing the others.
  int best = -1;
  double bestCost = 1e300;
  for (int i = 0; i < int(f_.size()); ++i) {
    if (f_[i].held) continue;
    int o;
    const int p = fold(i, key, &o);
    if (p < 0) continue;
    double c = cost(i, p, o);
    for (int g = 0; g < int(f_.size()); ++g)
      if (f_[g].held) c += crossing(i, p, g, f_[g].v->player().sounding());
    if (c < bestCost) { bestCost = c; best = i; *pitch = p; }
  }
  return best;
}

int Ensemble::nearestHeld(int key) const {
  int best = -1, bestD = 1 << 30;
  for (int i = 0; i < int(f_.size()); ++i) {
    const Slot& s = f_[i];
    if (!s.held || (mixed_ && (key < s.lo || key > s.hi))) continue;
    const int d = std::abs(s.v->player().sounding() - key);
    if (d < bestD) { bestD = d; best = i; }
  }
  return best;
}

void Ensemble::take(int flute, int key, int pitch, int velocity) {
  Slot& s = f_[flute];
  owner_[key] = flute;
  pitch_[key] = pitch;
  pressT_[key] = now_;
  vel_[key] = velocity;
  // A fresh note: it fits the chord, not its old place. Even on the same
  // voice and pitch it is young again (placed from 0 c, free to move), and
  // the voice takes the solved tuning at once (the player snaps to it on the
  // attack): nothing of the old tuning is remembered.
  s.tunedNote = -1;
  s.ageNote = pitch;
  s.noteAt = now_;
  s.cents = 0;
  tuneSig_ = -1;  // solve again even if the sounding set looks the same
  // Taken from silence (no key, no breath, its last note gone a while, or
  // never played): it starts as a fresh instrument would, its keys and lips
  // already at this note, nothing carried over from where it stood.
  const bool silent = s.held == 0 && !s.v->player().busy() && now_ - s.lastOff > kFresh;
  ++s.held;
  s.lastNote = pitch;
  s.active = true;
  s.v->player().noteOn(pitch, velocity);
  if (silent) s.v->model().startFresh();
}

void Ensemble::release(int key) {
  Slot& s = f_[owner_[key]];
  owner_[key] = -1;
  --s.held;
  s.lastOff = now_;
  s.v->player().noteOff(pitch_[key]);
}

// A mixed pool: the new key and the keys struck just before it (still alone on
// their instruments) are dealt together over those instruments and the free
// ones, at the least total cost (comfort, lines, no crossings with each other
// or with the voices already sounding).
bool Ensemble::deal(int key, int velocity) {
  int keys[8], nk = 0, cand[16], nc = 0;
  for (int k = 0; k < 128 && nk < 5; ++k)
    if (owner_[k] >= 0 && now_ - pressT_[k] < kChord && f_[owner_[k]].held == 1) keys[nk++] = k;
  keys[nk++] = key;
  for (int i = 0; i < int(f_.size()) && nc < 16; ++i) {
    bool movable = false;
    for (int j = 0; j + 1 < nk; ++j) movable |= owner_[keys[j]] == i;
    if (!f_[i].held || movable) cand[nc++] = i;
  }
  if (nc < nk) return false;
  int fixed[16], fixedP[16], nf = 0;
  for (int i = 0; i < int(f_.size()) && nf < 16; ++i) {
    bool isCand = false;
    for (int j = 0; j < nc; ++j) isCand |= cand[j] == i;
    if (f_[i].held && !isCand) { fixed[nf] = i; fixedP[nf++] = f_[i].v->player().sounding(); }
  }
  // Exhaustive: at most 6 keys over at most 16 instruments, pruned by cost.
  int pick[8], pickP[8], best[8], bestP[8];
  bool used[16] = {};
  double bestCost = 1e300;
  auto search = [&](auto&& self, int j, double c) -> void {
    if (c >= bestCost) return;
    if (j == nk) {
      bestCost = c;
      std::copy(pick, pick + nk, best);
      std::copy(pickP, pickP + nk, bestP);
      return;
    }
    for (int ci = 0; ci < nc; ++ci) {
      if (used[ci]) continue;
      const int f = cand[ci];
      int o;
      const int p = fold(f, keys[j], &o);
      if (p < 0) continue;
      double add = cost(f, p, o);
      for (int m = 0; m < nf; ++m) add += crossing(f, p, fixed[m], fixedP[m]);
      for (int m = 0; m < j; ++m) add += crossing(f, p, pick[m], pickP[m]);
      used[ci] = true;
      pick[j] = f;
      pickP[j] = p;
      self(self, j + 1, c + add);
      used[ci] = false;
    }
  };
  search(search, 0, 0.0);
  if (bestCost >= 1e300) return false;
  // Move the keys that are better elsewhere (struck within kChord: their
  // attack has hardly begun), then add the new one.
  bool moved = false;
  for (int j = 0; j + 1 < nk; ++j) {
    const int k = keys[j];
    if (owner_[k] == best[j] && pitch_[k] == bestP[j]) continue;
    const double t = pressT_[k];
    release(k);
    take(best[j], k, bestP[j], vel_[k]);
    pressT_[k] = t;
    moved = true;
  }
  take(best[nk - 1], key, bestP[nk - 1], velocity);
  if (verbose_ && (nk > 1 || moved)) {
    std::fprintf(stderr, "deal:");
    for (int j = 0; j < nk; ++j)
      std::fprintf(stderr, " %s%d %s%s", name(bestP[j]), bestP[j] / 12 - 1, f_[best[j]].label.c_str(), j + 1 < nk ? "," : "");
    std::fprintf(stderr, "\n");
  }
  return true;
}

// A key beyond every instrument of an ensemble: into its range by octaves (the
// keyboard's folding does this already when live).
int Ensemble::reach(int key) const {
  if (!mixed_ || key < 0 || key > 127) return key;
  while (key < lo_) key += 12;
  while (key > hi_) key -= 12;
  return key;
}

void Ensemble::noteOn(int key, int velocity) {
  key = reach(key);
  if (key < 0 || key > 127) return;
  if (owner_[key] >= 0) noteOff(key);
  bool chord = false;
  for (int k = 0; k < 128; ++k)
    if (owner_[k] >= 0 && now_ - pressT_[k] < kChord) chord = true;
  const int h = holding();
  // Silence, a chord, or more lines expected than flutes holding: a flute of
  // its own; otherwise a slur on the nearest held flute.
  bool own = h == 0 || chord || h < lines_;
  // A synth (a pool of circuits) is a polysynth: every key a voice of its
  // own, so a held note is never taken over and restarted elsewhere. Only
  // the portamento gesture slurs on the held voice: a soft key, softer than
  // the last one held (Performer::articulate's glide thresholds).
  if (!own && !mixed_ && (f_[0].v->instrument().circuit() || f_[0].v->instrument().plucked())) {
    static const double glideVel = knob("PJ_GLIDEVEL", 45), dvGlide = knob("PJ_DVGLIDE", 30);
    int lastVel = 64;
    double lastT = -1e9;
    for (int k = 0; k < 128; ++k)
      if (owner_[k] >= 0 && pressT_[k] > lastT) { lastT = pressT_[k]; lastVel = vel_[k]; }
    own = !(velocity <= glideVel && velocity - lastVel <= -dvGlide);
    // The pads have no portamento: every key its own voice, always.
    if (std::strcmp(f_[0].v->instrument().family, "pad") == 0 || std::strcmp(f_[0].v->instrument().family, "fx") == 0)
      own = true;  // (nor the effects: a struck plate does not slide)
  }
  if (mixed_) {
    if (own && deal(key, velocity)) return;
    // A slur, or no free instrument reaches the note: the nearest instrument
    // whose range has it moves to it (its old key stays held under the new one).
    int f = nearestHeld(key), p = key;
    if (f < 0) f = pickFree(key, &p);
    if (f < 0) return;
    if (verbose_ && own && f_[f].held)
      std::fprintf(stderr, "move: %s%d to %s (from %s%d)\n", name(key), key / 12 - 1, f_[f].label.c_str(),
                   name(f_[f].v->player().sounding()), f_[f].v->player().sounding() / 12 - 1);
    take(f, key, p, velocity);
    return;
  }
  int f = -1, p = key;
  if (own) f = pickFree(key, &p);
  if (f < 0) { f = nearestHeld(key); p = key; }
  if (f < 0) { f = 0; p = fold(0, key, nullptr); }
  if (p < 0) return;
  take(f, key, p, velocity);
}

void Ensemble::noteOff(int key) {
  key = reach(key);
  if (key < 0 || key > 127 || owner_[key] < 0) return;
  release(key);
}

// Two keys held together on one flute for longer than a slur's overlap: the
// older note is another line; give it a flute of its own.
void Ensemble::split() {
  for (int i = 0; i < int(f_.size()); ++i) {
    Slot& s = f_[i];
    if (s.held < 2) continue;
    const int cur = s.v->player().sounding();
    int curKey = -1;
    for (int k = 0; k < 128; ++k)
      if (owner_[k] == i && pitch_[k] == cur) curKey = k;
    if (curKey < 0 || now_ - pressT_[curKey] < kSplit) continue;
    for (int k = 0; k < 128; ++k) {
      if (owner_[k] != i || k == curKey) continue;
      int p;
      const int nf = pickFree(k, &p);
      if (nf < 0) return;
      --s.held;
      s.v->player().noteOff(pitch_[k]);  // not sounding there: only leaves its key stack
      const double t = pressT_[k];
      take(nf, k, p, vel_[k]);
      pressT_[k] = t;
      if (verbose_) std::fprintf(stderr, "poly: note %d moves to %s\n", p, mixed_ ? f_[nf].label.c_str() : ("flute " + std::to_string(nf + 1)).c_str());
    }
  }
}

void Ensemble::retune() {
  int idx[64], note[64], n = 0;
  long long sig = 0;
  for (int i = 0; i < int(f_.size()) && n < 64; ++i) {
    const Performer& p = f_[i].v->player();
    if (!f_[i].held || !p.busy() || p.sounding() < 0) continue;
    idx[n] = i;
    note[n] = p.sounding();
    if (f_[i].ageNote != note[n]) {  // a new note on this voice: its age starts
      f_[i].ageNote = note[n];
      f_[i].noteAt = now_;
    }
    sig = sig * 1000003 + i * 131 + note[n] + 1;
    ++n;
  }
  // Healing: while the sounding set stays the same (and none is young), the
  // notes slowly let go of where they are: every kHealEvery each anchor is
  // what it was times exp(-dt / kHeal), and the chord is solved again. The
  // intervals still ask for their ratios, so the chord stays just while its
  // common offset heals to equal temperament (its weighted mean to 0 c); a
  // note left alone, or with no harmonic reason to be off, goes back to 0 c.
  bool heal = false;
  if (sig == tuneSig_) {
    if (kHeal_ <= 0 || n == 0 || now_ - healAt_ < kHealEvery) return;
    for (int i = 0; i < n; ++i)
      if (now_ - f_[idx[i]].noteAt < kYoung) return;
    heal = true;
  }
  healAt_ = now_;
  tuneSig_ = sig;
  const double decay = heal ? std::exp(-kHealEvery / kHeal_) : 1.0;
  // Least squares: sum_pairs w (c_j - c_i - d_ij)^2 + sum_i l_i (c_i - a_i)^2
  // (+ the young notes' mean, below).
  // Who moves: the notes younger than kYoung are still being placed. If they
  // are fewer than the notes already sounding, those stay where they are and
  // the young fit them; if they are as many or more (a chord struck at once, a
  // rolled one, a new chord over one held note), they are placed together,
  // their mean on equal temperament, and the older minority moves to fit them.
  bool young[64];
  int ny = 0;
  for (int i = 0; i < n; ++i) ny += young[i] = now_ - f_[idx[i]].noteAt < kYoung;
  const bool oldStay = n - ny > ny;
  // A note already sounding (tuned before, past kYoung) moves at most
  // kMaxMove from where it is: solved freely, then any that would move further
  // pinned at its limit and the rest solved again around it.
  // A seventh over a sounding root, and the root's chord tones (its octaves,
  // third and fifth) tuned to it as one chord:
  //   - a minor seventh, major third (or none): the harmonic seventh, 4:5:6:7
  //     (1.75 x the root; 7/5 from the third, 7/6 from the fifth), or the
  //     5-limit 45/32 and 6/5 would pull it back up;
  //   - a minor seventh, minor third (no major one): 1.8 x the root,
  //     1 : 1.2 : 1.5 : 1.8 (a pure fifth from the third, 6/5 from the fifth;
  //     1.75 would put the third's fifth 47 c flat);
  //   - a major seventh, major third: 15/8 (1 + 7/8), 8:10:12:15, as the table;
  //   - a major seventh, minor third: 1.9 x the root, 10:12:15:19 (19/12 from
  //     the third, within 4 c of tempered; 15/8 would put it 27 c flat).
  // root7[i]: the root of note i's seventh; sev7[i]: its offset from the root (c).
  auto pcOf = [&](int a, int k) { return ((note[a] - note[k]) % 12 + 12) % 12; };
  int root7[64];
  bool minor7[64];
  double sev7[64];
  for (int i = 0; i < n; ++i) {
    root7[i] = -1;
    minor7[i] = false;
    for (int want : {10, 11})
      for (int k = 0; k < n && root7[i] < 0; ++k)
        if (pcOf(i, k) == want) root7[i] = k;
    if (root7[i] < 0) continue;
    bool m3 = false, M3 = false;
    for (int o = 0; o < n; ++o) {
      m3 |= pcOf(o, root7[i]) == 3;
      M3 |= pcOf(o, root7[i]) == 4;
    }
    minor7[i] = m3 && !M3;
    const bool major7 = pcOf(i, root7[i]) == 11;
    sev7[i] = major7 ? (minor7[i] ? 1200 * std::log2(1.9) - 1100 : kJust[11].cents)
                     : (minor7[i] ? 1200 * std::log2(1.8) - 1000 : kJust[10].cents);
  }
  // The offset of x from y (c_x - c_y) the seventh asks for, if one of them is
  // the seventh of a root the other is a chord tone of.
  auto seventh = [&](int x, int y, double* d) {
    for (int pass = 0; pass < 2; ++pass) {
      const int sv = pass ? y : x, o = pass ? x : y, k = root7[sv];
      if (k < 0) continue;
      const int pc = pcOf(o, k);
      if (pc != 0 && pc != 7 && pc != (minor7[sv] ? 3 : 4)) continue;
      const double off = sev7[sv] - kJust[pc].cents;  // c_sv - c_o
      *d = pass ? -off : off;
      return true;
    }
    return false;
  };
  // A virtual pitch: the chord heard as harmonics h_i of one low fundamental
  // F (a note's nominal frequency / N, N = 1..32); each note to its nearest
  // harmonic. Cost: the harmonics' size (log2: 4:5:6 before 8:10:12; a
  // seventh harmonic a little more, so a cluster is 8:9:10, not 7:8:9) and the
  // squared deviations (20 c units); none further than kMaxCents. Favoured, as a
  // chord heard over its root r: a fifth (or a bare minor seventh: 7/4) with
  // h_r a power of two (1 : 1.5); a
  // major triad, h_r = 4 x 2^k (1 : 1.25 : 1.5: a seventh then 14 = 1.75 x
  // the root, a major seventh 15); a minor triad, h_r = 10 x 2^k (1 : 1.2 :
  // 1.5: a seventh 18, a major seventh 19), whatever the voicing; both
  // thirds an octave apart: the higher one the overtone (below). A cluster
  // takes any N. The pairs then ask for these intervals (vpCents: each note's
  // place over F, from equal temperament); none fits: the interval table.
  static const bool kVirtual = knob("PJ_JIVIRTUAL", 1) > 0;
  double vpCents[64];
  int vpH[64], vpRoot = -1, vpN = 0;
  bool vp = false;
  if (kVirtual && n > 1) {
    double best = 1e300;
    int h[64];
    auto pc = [&](int a, int r) { return ((note[a] - note[r]) % 12 + 12) % 12; };
    for (int k = 0; k < n; ++k)
      for (int N = 1; N <= 32; ++N) {
        const double fk = note[k] - 12 * std::log2(double(N));  // F, as a (fractional) MIDI note
        double cost = 0;
        bool ok = true;
        for (int j = 0; j < n && ok; ++j) {
          const double ratio = std::exp2((note[j] - fk) / 12);
          h[j] = std::max(1, int(std::lround(ratio)));
          const double dev = 1200 * std::log2(h[j] / ratio);
          ok = h[j] <= 64 && std::fabs(dev) <= kMaxCents;
          cost += (dev / 20) * (dev / 20) + std::log2(double(h[j])) + (h[j] % 7 == 0 ? 1.0 : 0.0);
        }
        if (!ok) continue;
        // Chords over a root r among the notes.
        double bonus = 0;
        for (int r = 0; r < n; ++r) {
          bool p5 = false, M3 = false, m3 = false, m7 = false;
          for (int o = 0; o < n; ++o) {
            p5 |= pc(o, r) == 7;
            M3 |= pc(o, r) == 4;
            m3 |= pc(o, r) == 3;
            if (pc(o, r) == 10) {  // the seventh on a harmonic 7 x 2^k
              int ho = h[o];
              while (ho % 2 == 0) ho /= 2;
              m7 |= ho == 7;
            }
          }
          int hr = h[r];
          while (hr % 2 == 0 && hr > 1) hr /= 2;
          const int odd = hr;  // h_r = odd x 2^k
          // Both thirds, an octave or more apart: the higher one is heard as
          // the overtone. The major above: a minor chord, 10 : 12 : 25 (r / 10);
          // the minor above: a major one, 8 : 10 : 19 (r / 8; 2.375 for 2.4).
          bool majorAbove = false, minorAbove = false;
          for (int o1 = 0; o1 < n; ++o1)
            for (int o2 = 0; o2 < n; ++o2)
              if (pc(o1, r) == 4 && pc(o2, r) == 3) {
                majorAbove |= note[o1] - note[o2] >= 13;
                minorAbove |= note[o2] - note[o1] >= 11;
              }
          double b = 0;
          if (majorAbove && !minorAbove) b = odd == 5 && h[r] >= 10 ? 4 : 0;
          else if (minorAbove && !majorAbove) b = odd == 1 && h[r] >= 8 ? 4 : 0;
          else if (p5 && M3 && odd == 1 && h[r] >= 4) b = 4;
          else if (p5 && m3 && !M3 && odd == 5 && h[r] >= 10) b = 4;
          else if (m7 && !m3 && odd == 1 && h[r] >= 4) b = 2.5;  // a bare seventh as 7/4
          else if (p5 && odd == 1 && h[r] >= 2) b = 1.5;
          bonus = std::max(bonus, b);
        }
        cost -= bonus;
        if (cost < best) {
          best = cost;
          vp = true;
          vpRoot = k;
          vpN = N;
          for (int j = 0; j < n; ++j) {
            vpH[j] = h[j];
            vpCents[j] = 1200 * std::log2(h[j] / std::exp2((note[j] - fk) / 12));
          }
        }
      }
  }
  // How clear each note's pitch is (its sound's harmonicity, blurred by its
  // vibrato: the usual live 0.05 x0.67), and how far it may be corrected:
  // the pads and the effects (a shimmer, an inharmonic spectrum) and any
  // sound this unclear at most 11 c; a clean tone the full kMaxCents (from
  // clarity 0.95; 0.75 and under: 11).
  double clar[64], capC[64];
  for (int i = 0; i < n; ++i) {
    const Performer& p = f_[idx[i]].v->player();
    const Instrument& in = f_[idx[i]].v->instrument();
    double h = in.circuit() ? leadClarity(in) : in.oneVoice() ? 0.9 : 1.0;
    h /= 1 + p.vibrato() / 0.1;
    clar[i] = h;
    const bool pad = std::strcmp(in.family, "pad") == 0 || std::strcmp(in.family, "fx") == 0;
    capC[i] = pad ? 11.0 : 11 + (kMaxCents - 11) * std::clamp((h - 0.75) / 0.2, 0.0, 1.0);
  }
  // The highest note and its pull to equal temperament (above, in solve).
  int top = -1;
  double topPull = 0;
  {
    static const double kTopPull = knob("PJ_TOPPULL", 1);
    for (int i = 0; i < n; ++i)
      if (top < 0 || note[i] > note[top]) top = i;
    if (n < 2 || kTopPull <= 0) top = -1;
    if (top >= 0) topPull = kTopPull * (0.4 + 2.5 * (1 - clar[top]));
  }
  bool pin[64] = {};
  double pinTo[64] = {}, c[64];
  bool meanOn = true;  // the young's mean held at 0 (dropped once an old note is at its limit)
  auto solve = [&]() {
    double A[64][64] = {}, b[64] = {};
    for (int i = 0; i < n; ++i) {
      Slot& s = f_[idx[i]];
      const bool known = s.tunedNote == note[i];
      double l, a;
      if (pin[i]) { l = 1e6; a = pinTo[i]; }
      else if (young[i]) { l = kToTempered; a = known ? s.cents : 0; }
      else if (oldStay) { l = kStay; a = s.cents * decay; }
      else { l = kToTempered; a = s.cents; }  // the old minority: free, near where it is
      A[i][i] += l;
      b[i] += l * a;
    }
    // The highest note tuned less: pulled to equal temperament (the chord's
    // justness found more by the notes under it), the more so the less
    // clearly harmonic its sound (a shimmer, a vibrato, an inharmonic
    // spectrum: there is less of a pitch to lock): 0.4 for a clean tone up to
    // 2.9 (the pairs' weights are 0.05..1). PJ_TOPPULL scales it (0: none).
    if (top >= 0) {
      A[top][top] += topPull;
      // (its target: 0 c)
    }
    if (ny > 0 && !oldStay && meanOn)  // kMean (sum of the young's c / ny)^2: their mean at 0
      for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
          if (young[i] && young[j]) A[i][j] += kMean / (double(ny) * ny);
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j) {
        if (note[j] <= note[i] && !(note[j] == note[i] && j > i)) continue;
        const int semis = note[j] - note[i];
        const Just& J = kJust[semis % 12];
        double d = J.cents, wJ = J.weight;
        if (vp) {
          d = vpCents[j] - vpCents[i];
          wJ = std::max(wJ, 0.3);
        } else if (seventh(j, i, &d)) {
          wJ = std::max(wJ, kJust[10].weight);
        }
        const double w = wJ / (1 + 0.5 * (semis / 12));
        // c_j - c_i = d
        A[i][i] += w; A[j][j] += w; A[i][j] -= w; A[j][i] -= w;
        b[j] += w * d; b[i] -= w * d;
      }
    // Gaussian elimination (A is symmetric positive definite).
    for (int k = 0; k < n; ++k)
      for (int r = k + 1; r < n; ++r) {
        const double f = A[r][k] / A[k][k];
        for (int cc = k; cc < n; ++cc) A[r][cc] -= f * A[k][cc];
        b[r] -= f * b[k];
      }
    for (int k = n - 1; k >= 0; --k) {
      double v = b[k];
      for (int jj = k + 1; jj < n; ++jj) v -= A[k][jj] * c[jj];
      c[k] = std::clamp(v / A[k][k], -kMaxCents, kMaxCents);
    }
  };
  solve();
  bool again = false;
  for (int i = 0; i < n; ++i) {
    const Slot& s = f_[idx[i]];
    if (young[i] || s.tunedNote != note[i]) continue;
    const double lim = std::clamp(c[i], s.cents - kMaxMove_, s.cents + kMaxMove_);
    if (lim != c[i]) { pin[i] = true; pinTo[i] = lim; again = true; }
  }
  // A note past its correction limit (capC: the unclear sounds' 11 c): pinned
  // there, and the others solved again around it.
  for (int i = 0; i < n; ++i)
    if (!pin[i] && std::fabs(c[i]) > capC[i]) {
      pin[i] = true;
      pinTo[i] = std::clamp(c[i], -capC[i], capC[i]);
      again = true;
    }
  // An old note at its limit: it cannot meet the young halfway, so the
  // young fit it (the intervals before their mean on 0 c; the healing brings
  // the chord back to 0 c afterwards).
  if (again) {
    meanOn = false;
    solve();
    // (Solved around the pins, another may now pass its limit: pinned too, at
    // most once each.)
    for (int pass = 0; pass < n; ++pass) {
      bool more = false;
      for (int i = 0; i < n; ++i)
        if (!pin[i] && std::fabs(c[i]) > capC[i]) {
          pin[i] = true;
          pinTo[i] = std::clamp(c[i], -capC[i], capC[i]);
          more = true;
        }
      if (!more) break;
      solve();
    }
  }
  for (int i = 0; i < n; ++i) {
    Slot& s = f_[idx[i]];
    s.cents = c[i];
    s.tunedNote = note[i];
    s.v->player().setTuning(c[i]);
  }
  if (n > 1 && !heal) {
    std::fprintf(stderr, "tune:");
    for (int i = 0; i < n; ++i) std::fprintf(stderr, " %s%d %+.0f", name(note[i]), note[i] / 12 - 1, c[i]);
    if (vp) {  // (its own line: tools/test.py reads the notes on the tune line)
      std::fprintf(stderr, "\nvirtual pitch: %s%d / %d, harmonics", name(note[vpRoot]), note[vpRoot] / 12 - 1, vpN);
      for (int i = 0; i < n; ++i) std::fprintf(stderr, " %d", vpH[i]);
    }
    std::fprintf(stderr, "\n");
  }
}

void Ensemble::render(float* left, float* right, int n) {
  // In blocks of Room::kBlock (the room's coupling step), queued.
  for (int done = 0; done < n;) {
    if (qPos_ >= qL_.size()) {
      qL_.assign(Room::kBlock, 0.0f);
      qR_.assign(Room::kBlock, 0.0f);
      qPos_ = 0;
      block(qL_.data(), qR_.data(), Room::kBlock);
    }
    const int m = std::min(n - done, int(qL_.size() - qPos_));
    std::copy(qL_.begin() + qPos_, qL_.begin() + qPos_ + m, left + done);
    std::copy(qR_.begin() + qPos_, qR_.begin() + qPos_ + m, right + done);
    qPos_ += m;
    done += m;
  }
}

void Ensemble::render(float* out, int n) {
  std::vector<float> r(n);
  render(out, r.data(), n);
  for (int i = 0; i < n; ++i) out[i] = 0.5f * (out[i] + r[i]);
}

void Ensemble::block(float* left, float* right, int n) {
  if (f_.size() > 1) split();
  if (f_.size() > 1 && ji_) retune();
  // How many lines: the most seen lately, back down after kForget with fewer.
  const int h = holding();
  if (h >= lines_) {
    calm_ = 0;
    if (h > lines_) lines_ = h;
  } else if ((calm_ += n / 48000.0) > kForget) {
    calm_ = 0;
    lines_ = std::max(1, h);
  }
  {  // Who is playing: holding a key, or did within kForget. Print changes.
    std::string who;
    std::vector<std::pair<int, std::string>> ins;  // (rank, name), low to high
    for (const Slot& s : f_)
      if (s.held || now_ - s.lastOff < kForget) {
        const char* nm = s.v->instrument().name;
        ins.push_back({s.rank, std::strncmp(nm, "flute", 5) == 0 ? "flute" : nm});
      }
    std::stable_sort(ins.begin(), ins.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    for (size_t i = 0; i < ins.size();) {
      size_t j = i;
      while (j < ins.size() && ins[j].second == ins[i].second) ++j;
      who += (who.empty() ? "" : ", ") + (j - i > 1 ? std::to_string(j - i) + " x " : std::string()) + ins[i].second;
      i = j;
    }
    if (who.empty()) who = "none";
    if (who != playing_) {
      if (who != "none") std::fprintf(stderr, "playing: %s\n", who.c_str());
      playing_ = who;
    }
  }
  const int nf = int(f_.size());
  buf_.resize(nf);
  int playing = 0;
  std::vector<int> go;
  bool circuits = true;
  for (int i = 0; i < nf; ++i) {
    Slot& s = f_[i];
    buf_[i].assign(n, 0.0f);
    circuits &= s.v->instrument().circuit();
    // Without a room, a flute silent for a while is not computed (its tail
    // has died away); in a room every flute listens. (Circuits always run:
    // their oscillators never stop, and they share a board.)
    if (s.active && !s.held && !s.v->player().busy() && now_ - s.lastOff > kIdle) s.active = false;
    if (!room_ && !s.active && nf > 1 && !s.v->instrument().circuit()) continue;
    go.push_back(i);
  }
  // Circuits on one board couple sample by sample (wires, rail, ground):
  // render them interleaved, 4 samples at a time.
  const int step = circuits ? 4 : n;
  for (int j = 0; j < n; j += step) {
    const int m = std::min(step, n - j);
    for (int i : go) {
      Slot& s = f_[i];
      s.v->render(buf_[i].data() + j, m, room_ ? s.room.data() + j : nullptr);
    }
  }
  for (int i : go) playing += f_[i].v->player().busy();
  if (room_) {
    // To pressure at 1 m (Pa) for the room; the listener's microphones keep
    // each instrument's balance (its gain).
    const float k = float(f_[0].v->model().outScale());
    srcBuf_.resize(size_t(nf) * n);
    std::vector<const float*> src(nf);
    std::vector<float*> at(nf);
    std::vector<float> lg(nf);
    for (int i = 0; i < nf; ++i) {
      const float g = float(f_[i].v->instrument().gain);
      for (int j = 0; j < n; ++j) srcBuf_[size_t(i) * n + j] = buf_[i][j] / (g * k);
      src[i] = &srcBuf_[size_t(i) * n];
      at[i] = f_[i].room.data();
      lg[i] = g;
    }
    for (int c = 0; c < 3; ++c) {
      if (!rooms_[c]) continue;
      std::vector<const float*> sc;
      std::vector<float*> ac;
      std::vector<float> gc;
      for (int i : roomSlots_[c]) {
        sc.push_back(src[i]);
        ac.push_back(at[i]);
        gc.push_back(lg[i]);
      }
      rooms_[c]->process(sc.data(), n, ac.data(), left, right, gc.data());
    }
    for (int i = 0; i < nf; ++i)
      for (int j = 0; j < n; ++j) at[i][j] *= roomFb_;
    for (int j = 0; j < n; ++j) { left[j] *= k; right[j] *= k; }
  } else if (panned_) {
    for (int i = 0; i < nf; ++i) {
      const float gl = float(f_[i].gl), gr = float(f_[i].gr);
      for (int j = 0; j < n; ++j) {
        left[j] += gl * buf_[i][j];
        right[j] += gr * buf_[i][j];
      }
    }
  } else {
    for (int i = 0; i < nf; ++i)
      for (int j = 0; j < n; ++j) left[j] += buf_[i][j];
    std::copy(left, left + n, right);
  }
  // An instrument that sounds differently to its two sides (the double bass):
  // its direct sound's difference, to the left and from the right.
  for (int i : go)
    if (const float* sd = f_[i].v->side(n))
      for (int j = 0; j < n; ++j) {
        left[j] += sd[j];
        right[j] -= sd[j];
      }
  // Several flutes add up; keep the sum near one flute's level (1 / sqrt(n)).
  const double target = 1 / std::sqrt(std::max(1, playing));
  // Down at once when voices join (~40 ms: no clipping), back up slowly when
  // they leave (~2 s, as a compressor's release): a released note still
  // rings, and turning the others up under its tail is an "umpf" on every
  // key up.
  for (int i = 0; i < n; ++i) {
    mix_ += (target < mix_ ? 0.0005 : 1e-5) * (target - mix_);
    left[i] *= float(mix_);
    right[i] *= float(mix_);
  }
  now_ += n / 48000.0;
}

int Ensemble::resets() const {
  int r = 0;
  for (const Slot& s : f_) r += s.v->model().resets();
  return r;
}

}  // namespace pajupilli
