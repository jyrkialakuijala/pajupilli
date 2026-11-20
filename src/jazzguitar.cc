#include "jazzguitar.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "knob.h"
#include "songclock.h"

namespace pajupilli {

namespace {
constexpr int kOpen[Guitar::kStrings] = {40, 45, 50, 55, 59, 64};
// Where the song's beat is (0 .. 1 within the beat; -1: not known).
double beatFrac() {
  const SongClock& sc = SongClock::get();
  return sc.valid ? sc.beat - std::floor(sc.beat) : -1;
}
double beatLen() {
  const SongClock& sc = SongClock::get();
  return sc.valid ? sc.beatLen : 0.5;
}
}  // namespace

JazzGuitarPlayer::JazzGuitarPlayer(GuitarModel& m) : Performer(m), m_(m) {
  std::fill(strNote_, strNote_ + kS, -1);
  place_ = knob("PJ_JAZZ_PLACE", 0.165);
  move_ = knob("PJ_JAZZ_MOVE", 1.0);
  const double mm = std::clamp(knob("PJ_JAZZ_PICKMM", 1.5), 0.4, 3.0);
  stiff_ = std::clamp(6000 * std::pow(mm, 2.6), 1500.0, 50000.0);
  edge_ = std::clamp(knob("PJ_JAZZ_EDGE", 0.25), 0.05, 2.0) * 1e-3;
  tipW_ = std::clamp(knob("PJ_JAZZ_TIPW", 2.5), 0.5, 15.0) * 1e-3;
  thumb_ = int(std::lround(knob("PJ_JAZZ_THUMB", 1)));
  release_ = knob("PJ_JAZZ_RELEASE", 200);
  chk_ = knob("PJ_JAZZ_CHK", 1.0);
  gather_ = knob("PJ_JAZZ_GATHER", 0.008);
  debug_ = knob("PJ_JAZZ_DEBUG", 0) > 0;
  stats_ = knob("PJ_JAZZ_STATS", 0) > 0;
}

JazzGuitarPlayer::~JazzGuitarPlayer() {
  if (!stats_) return;
  std::fprintf(stderr, "jazz-guitar: %ld chords, %ld notes asked, %ld dropped (%.1f %%), %ld top notes dropped, %ld unplayable, "
               "%ld open strings in chords of 3+, the hand's mean move %.2f frets\n",
               nChords_, nNotes_, nDropped_, 100.0 * nDropped_ / std::max(1L, nNotes_), nTopDropped_, nFailed_, nOpen_,
               moveSum_ / std::max(1L, moves_));
}

double JazzGuitarPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

void JazzGuitarPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  if (asked_.empty()) { gapBefore_ = sinceNote_; askAge_ = 0; }
  asked_.push_back({midi, velocity, false});
  sinceNote_ = 0;
}

void JazzGuitarPlayer::noteOff(int midi) {
  // (Let go before it sounded: a ghost stroke.)
  for (Asked& a : asked_)
    if (a.midi == midi && !a.ghost) { a.ghost = true; return; }
  // A chord's length, for four to the bar: when its first note is let go.
  for (int s = 0; s < kS; ++s)
    if (chordMidi_[s] == midi) {
      if (!chordLen_.empty() && chordLen_.back() < 0) chordLen_.back() = clock_ - chordOn_.back();
      std::fill(chordMidi_, chordMidi_ + kS, -1);
      break;
    }
  for (int s = 0; s < kS; ++s) {
    if (strNote_[s] != midi || !held_[s]) continue;
    held_[s] = false;
    if (hold_) continue;
    // The left hand eases off: the string leaves the fret and lies on the finger (5 ms), a soft
    // "chk" as it lifts, then dies in ~30 ms.
    Touch t{};
    t.depth = 0.03e-3 * chk_ * (0.5 + vel_[s] / 127.0);
    due_.push_back({0.005, kRelease, s, fret_[s], t, 0, release_});
  }
}

void JazzGuitarPlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int s = 0; s < kS; ++s)
      if (!held_[s] && strNote_[s] >= 0) m_.guitar().damp(s, release_);
}

// The touch for velocity v (0..1): where, how deep, how fast, the angle (12.1).
JazzGuitarPlayer::Touch JazzGuitarPlayer::touch(double v, bool up, int string) {
  Touch t;
  // The phrase's intensity: how loud it has been (the running mean), and this note against it
  // (an accent). Loud and accented: towards the bridge; soft and even: towards the neck.
  const double vel = v * 127;
  const double phrase = std::clamp((velMean_ - 30) / 70, 0.0, 1.0), accent = std::clamp((vel - velMean_) / 40, -0.5, 1.0);
  const double I = std::clamp(0.55 * phrase + 0.45 * (0.5 + accent) + 0.2 * (v - 0.5), 0.0, 1.0);
  t.from = std::clamp(place_ * std::pow(2.0, 1 - brightness_) * (1.15 - 0.54 * move_ * I) * (0.97 + 0.06 * ur()), 0.06, 0.24);
  const double dyn = 0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0);
  t.depth = (0.1e-3 + 2.0e-3 * std::pow(v, 1.25)) * dyn * (0.93 + 0.14 * ur()) * (up ? 1.15 : 1.0);
  // (The low strings are heavier: the same hand moves them a little less far.)
  t.depth *= 1.0 - 0.04 * (2.5 - string) / 2.5;
  t.push = (0.010 - 0.0075 * v) * (0.85 + 0.3 * ur());
  const double speed = std::max(t.depth, 1e-5) / t.push;
  t.slip = std::clamp(edge_ * std::sqrt(0.3 / speed), 0.00008, 0.002);
  t.angle = up ? M_PI + 0.12 : 0.22 + 0.35 * v;
  return t;
}

// The cheapest way a hand holds `notes` (low to high): each note its own string, the strings
// rising with the notes; the fretted notes within reach; costs as 6.3.
bool JazzGuitarPlayer::finger(const std::vector<int>& notes, Grip& best, bool comp) const {
  const Guitar& g = m_.guitar();
  const int n = int(notes.size());
  best.cost = 1e18;
  if (n == 0 || n > kS) return false;
  Grip cur{};
  // (Depth-first over strings, at most C(6, n) choices.)
  struct Rec {
    const JazzGuitarPlayer* p;
    const std::vector<int>& notes;
    int n, frets;
    bool comp;
    Grip& cur;
    Grip& best;
    void go(int i, int from) {
      if (i >= kS || i == n) {
        int lo = 99, hi = -1, open = 0, fretted = 0;
        double sum = 0;
        for (int k = 0; k < n; ++k) {
          const int f = cur.fret[k];
          if (f == 0) { ++open; continue; }
          lo = std::min(lo, f);
          hi = std::max(hi, f);
          sum += f;
          ++fretted;
        }
        if (fretted && hi - lo > (lo >= 5 ? 5 : 4)) return;
        if (comp && open) return;  // (comping: no open string, it cannot be choked)
        double c = 0;
        c += open * (comp ? 4.0 : -0.3);
        if (fretted) {
          const double mid = sum / fretted, mv = std::fabs(mid - 1 - p->hand_);
          c += 0.4 * mv + 1.0 * std::max(0.0, mv - 2);
          if (comp) c += 0.3 * (std::max(0.0, 3 - mid) + std::max(0.0, mid - 10));
          c += 0.1 * (hi - lo);
        }
        // Strings skipped inside the grip (muted by a finger): one each is a known grip's, two
        // in a row are awkward.
        int skipped = 0;
        for (int k = 1; k < n; ++k) {
          const int gap = cur.string[k] - cur.string[k - 1] - 1;
          skipped += gap;
          if (gap > 1) c += 1.5 * (gap - 1);
        }
        c += 0.3 * skipped;
        // Known grips (6.2): shell 6-4-3 / 5-4-3, drop 2 (four adjacent), drop 3 (6-4-3-2, 5-3-2-1).
        const int s0 = cur.string[0];
        if (n == 3 && cur.string[1] == s0 + 2 && cur.string[2] == s0 + 3 && s0 <= 1) c -= 0.8;
        if (n == 4 && cur.string[3] == s0 + 3) c -= 0.8;
        if (n == 4 && cur.string[1] == s0 + 2 && cur.string[2] == s0 + 3 && cur.string[3] == s0 + 4) c -= 0.6;
        if (c < best.cost) { best = cur; best.cost = c; }
        return;
      }
      for (int s = from; s <= kS - (n - i); ++s) {
        const int f = notes[i] - kOpen[s];
        if (f < 0 || f > frets) continue;
        cur.string[i] = s;
        cur.fret[i] = f;
        go(i + 1, s + 1);
      }
    }
  } rec{this, notes, n, g.frets(), comp, cur, best};
  rec.go(0, 0);
  return best.cost < 1e17;
}

// The voicing (6.3): fold the range, then reduce until a hand can hold it, never the top note.
bool JazzGuitarPlayer::voice(std::vector<Asked>& notes, Grip& grip) {
  const int top = kOpen[5] + m_.guitar().frets();
  std::sort(notes.begin(), notes.end(), [](const Asked& a, const Asked& b) { return a.midi < b.midi; });
  // (A chord over the top fret: the whole voicing an octave down if its bass allows, so that its
  // top note stays on top.)
  while (notes.size() > 1 && notes.back().midi > top && notes.front().midi - 12 >= kOpen[0])
    for (Asked& a : notes) a.midi -= 12;
  for (size_t i = 0; i < notes.size(); ++i) {
    while (notes[i].midi > top) notes[i].midi -= 12;
    if (notes[i].midi < kOpen[0]) {
      const int up = notes[i].midi + 12 * ((kOpen[0] - notes[i].midi + 11) / 12);
      // (Up an octave if that does not cross the next note; else the bassist has it.)
      if (i + 1 < notes.size() && up > notes[i + 1].midi && notes[i + 1].midi <= top) { notes.erase(notes.begin() + i); --i; ++nDropped_; continue; }
      notes[i].midi = up;
    }
  }
  std::sort(notes.begin(), notes.end(), [](const Asked& a, const Asked& b) { return a.midi < b.midi; });
  for (size_t i = 1; i < notes.size(); ++i)
    if (notes[i].midi == notes[i - 1].midi) { notes[i].velocity = std::max(notes[i].velocity, notes[i - 1].velocity); notes.erase(notes.begin() + i - 1); --i; ++nDropped_; }
  const bool comp = notes.size() >= 3;
  const auto mids = [&]() { std::vector<int> v; for (const Asked& a : notes) v.push_back(a.midi); return v; };
  const auto playable = [&]() { return finger(mids(), grip, comp); };
  // One note dropped by the first rule that applies (false: none does).
  const auto reduce = [&]() {
    const int n = int(notes.size());
    if (n <= 1) return false;
    const int bass = notes[0].midi, topN = n - 1;
    const auto drop = [&](int i) { notes.erase(notes.begin() + i); ++nDropped_; return true; };
    // (a) a doubled pitch class: an inner instance (the top and the bottom kept).
    for (int i = 1; i < topN; ++i)
      for (int j = 0; j < n; ++j)
        if (j != i && notes[j].midi % 12 == notes[i].midi % 12) return drop(i);
    // (b) the perfect fifth over the bass.
    for (int i = 1; i < topN; ++i)
      if ((notes[i].midi - bass) % 12 == 7) return drop(i);
    // (c) the bass, under a wide gap: the bassist plays it.
    if (n >= 3 && notes[1].midi - bass >= 7) return drop(0);
    // (e) inner notes from the bottom, sparing the 3rd and the 7th over the bass.
    for (int i = 1; i < topN; ++i) {
      const int iv = (notes[i].midi - bass) % 12;
      if (iv != 3 && iv != 4 && iv != 10 && iv != 11) return drop(i);
    }
    if (topN > 1) return drop(1);
    // (two notes: the lower one)
    return drop(0);
  };
  // More than four that will not fit (or more than six): to four first.
  if (notes.size() > size_t(kS) || (notes.size() >= 5 && !playable()))
    while (notes.size() > 4) reduce();
  while (!playable())
    if (!reduce()) break;
  return grip.cost < 1e17;
}

// A note now sounding on string s.
void JazzGuitarPlayer::sounding(int s, int midi, int fret, int vel, bool ghost) {
  strNote_[s] = ghost ? -1 : midi;
  held_[s] = !ghost;
  fret_[s] = fret;
  vel_[s] = vel;
  age_[s] = 0;
  vib_[s] = 0;
}

void JazzGuitarPlayer::chord(std::vector<Asked>& notes) {
  Guitar& g = m_.guitar();
  const size_t asked = notes.size();
  const int topAsked = notes.back().midi;
  Grip grip{};
  std::fill(chordMidi_, chordMidi_ + kS, -1);
  ++nChords_;
  nNotes_ += long(asked);
  const long droppedBefore = nDropped_;
  if (!voice(notes, grip)) { ++nFailed_; return; }
  {
    // (Was the top note kept? Folded, it is the same pitch class at the top.)
    if (notes.back().midi % 12 != topAsked % 12) { ++nTopDropped_; if (debug_) std::fprintf(stderr, "top dropped: asked top %d\n", topAsked); }
  }
  (void)droppedBefore;
  const int n = int(notes.size());
  if (n >= 3)
    for (int i = 0; i < n; ++i) nOpen_ += grip.fret[i] == 0;
  double fsum = 0;
  int fn = 0;
  for (int i = 0; i < n; ++i)
    if (grip.fret[i] > 0) { fsum += grip.fret[i]; ++fn; }
  if (fn) {
    const double pos = fsum / fn - 1;
    if (nChords_ > 1) { moveSum_ += std::fabs(pos - hand_); ++moves_; }
    hand_ = std::clamp(pos, 1.0, 15.0);
  }
  double vmean = 0;
  bool allGhost = true;
  for (const Asked& a : notes) { vmean += a.velocity / double(n); allGhost = allGhost && a.ghost; }
  // Four to the bar (6.4): the last eight chords about a beat apart, each held under 0.6 beat.
  const double bl = beatLen();
  chordOn_.push_back(clock_);
  chordLen_.push_back(-1);
  if (chordOn_.size() > 9) { chordOn_.erase(chordOn_.begin()); chordLen_.erase(chordLen_.begin()); }
  fourBar_ = false;
  if (chordOn_.size() == 9 && SongClock::get().valid) {
    fourBar_ = true;
    for (size_t i = 1; i < chordOn_.size(); ++i) {
      const double d = (chordOn_[i] - chordOn_[i - 1]) / bl;
      if (d < 0.8 || d > 1.2 || (i + 1 < chordOn_.size() && (chordLen_[i] < 0 || chordLen_[i] > 0.6 * bl))) fourBar_ = false;
    }
  }
  const double frac = beatFrac();
  const bool onBeat = frac < 0 || frac < 0.15 || frac > 0.85;
  // The fingers: a soft chord after a long rest (a ballad's).
  const bool fingers = !fourBar_ && !allGhost && vmean < 50 && gapBefore_ >= 1.5 * bl;
  // The rake's way: down on the beat and on anticipations; up on a quick off-beat.
  const bool up = !fourBar_ && !fingers && !onBeat && gapBefore_ < 0.45 && pickWay_ > 0;
  pickWay_ = up ? -1 : 1;
  if (fourBar_) {
    const SongClock& sc = SongClock::get();
    const int beatInBar = int(std::floor(std::fmod(sc.beat + 0.1, 4.0)));
    if (beatInBar == 1 || beatInBar == 3) vmean += 3;
  }
  // The strings outside the grip, not holding a note: the fretting hand's fingers lie on them
  // (every unfretted string is muted, 6.2): they do not ring along.
  for (int s = 0; s < kS; ++s) {
    bool in = false;
    for (int i = 0; i < n; ++i) in = in || grip.string[i] == s;
    if (!in && !held_[s] && !hold_) g.damp(s, 200);
  }
  // Strings: those of the grip, and the ones skipped inside it (dead).
  const int sLo = grip.string[0], sHi = grip.string[n - 1];
  std::vector<int> order;
  for (int s = sLo; s <= sHi; ++s) order.push_back(s);
  if (up) std::reverse(order.begin(), order.end());
  const double vv = std::clamp(vmean / 127, 0.0, 1.0);
  const double perString = (0.002 + 0.003 * (1 - vv)) * (fourBar_ ? 0.8 : 1.0);
  double at = 0;
  // (Each chord its own: the hand a little nearer or further.)
  for (int s : order) {
    int idx = -1;
    for (int i = 0; i < n; ++i)
      if (grip.string[i] == s) idx = i;
    const double delay = fingers ? 0.003 * ur() : at;
    at += perString * (0.85 + 0.3 * ur());
    if (idx < 0) {
      // The skipped string: the fretting hand's finger lies on it; the pick crosses it: a click.
      Touch t = touch(0.6 * vv, up, s);
      due_.push_back({delay, kDead, s, std::max(0, fret_[s]), t, 0, 600});
      continue;
    }
    const Asked& a = notes[idx];
    const double v = std::clamp((fourBar_ ? vmean + 0.4 * (a.velocity - vmean) : a.velocity) / 127.0, 0.0, 1.0);
    Touch t = touch(v, up, s);
    Kind kind = fingers ? kFingers : kPick;
    double dampAfter = 0, rate = 0;
    if (a.ghost) {
      // A ghost: struck, then the hand that only touched the strings lets them die.
      if (a.velocity < 25) { kind = kDead; rate = 600; }
      else { dampAfter = 0.015 + 0.01 * ur(); rate = release_; }
    } else if (fourBar_ && s != 2 && n >= 3) {
      // Freddie Green: the outer voices of the shell damped, the 4th string's line sings.
      dampAfter = 0.03;
      rate = 80;
    }
    due_.push_back({delay, kind, s, grip.fret[idx], t, dampAfter, rate});
    sounding(s, a.midi, grip.fret[idx], a.velocity, a.ghost);
    if (!a.ghost) chordMidi_[s] = a.midi;
  }
  if (debug_) {
    std::fprintf(stderr, "chord %s%s%s v%.0f:", fingers ? "fingers" : up ? "up" : "down", fourBar_ ? " (four to the bar)" : "", allGhost ? " ghost" : "", vmean);
    for (int i = 0; i < n; ++i) std::fprintf(stderr, " %d@s%d/f%d", notes[i].midi, 6 - grip.string[i], grip.fret[i]);
    std::fprintf(stderr, " (asked %zu)\n", asked);
  }
}

void JazzGuitarPlayer::octave(const Asked& lo, const Asked& hi) {
  // Strings s and s + 2, the middle one muted; the thumb sweeps all three (6.6).
  int best = -1, bf = 0, bh = 0;
  double cost = 1e9;
  for (int s = 0; s + 2 < kS; ++s) {
    const int f = lo.midi - kOpen[s], fh = hi.midi - kOpen[s + 2];
    if (f < 1 || fh < 1 || f > m_.guitar().frets() || fh > m_.guitar().frets()) continue;
    const double c = std::fabs((f + fh) * 0.5 - 1 - hand_) + (s == 0 ? 1.0 : 0.0) + (held_[s] || held_[s + 2] ? 4 : 0);
    if (c < cost) { cost = c; best = s; bf = f; bh = fh; }
  }
  if (best < 0) { single(lo); single(hi); return; }
  lastOctave_ = clock_;
  const bool thumb = thumb_ >= 1;
  const double frac = beatFrac();
  const bool up = thumb && frac >= 0 && frac > 0.3 && frac < 0.75 && gapBefore_ < 0.4 && pickWay_ > 0;
  pickWay_ = up ? -1 : 1;
  const double v = std::clamp(0.5 * (lo.velocity + hi.velocity) / 127.0, 0.0, 1.0);
  const int order[3] = {up ? best + 2 : best, best + 1, up ? best : best + 2};
  double at = 0;
  for (int k = 0; k < 3; ++k) {
    const int s = order[k];
    Touch t = touch(v, up, s);
    if (thumb) { t.from = std::clamp(0.18 * std::pow(2.0, 1 - brightness_), 0.08, 0.24); t.depth *= 1.25; }
    if (k == 1) {
      due_.push_back({at, kDead, s, std::max(1, fret_[s]), t, 0, 600});
    } else {
      const bool low = s == best;
      due_.push_back({at, thumb ? kThumb : kPick, s, low ? bf : bh, t, lo.ghost ? 0.015 : 0.0, lo.ghost ? release_ : 0.0});
      sounding(s, low ? lo.midi : hi.midi, low ? bf : bh, low ? lo.velocity : hi.velocity, lo.ghost);
    }
    at += (thumb ? 0.006 : 0.003) * (0.85 + 0.3 * ur());
  }
  hand_ += 0.6 * ((bf + bh) * 0.5 - 1 - hand_);
  bendString_ = best + 2;
  if (debug_) std::fprintf(stderr, "octave %d/%d strings %d,%d frets %d,%d %s%s\n", lo.midi, hi.midi, 6 - best, 4 - best, bf, bh, thumb ? "thumb" : "pick", up ? " up" : "");
}

void JazzGuitarPlayer::single(const Asked& a) {
  Guitar& g = m_.guitar();
  const int top = kOpen[5] + g.frets();
  int midi = a.midi;
  while (midi > top) midi -= 12;
  while (midi < kOpen[0]) midi += 12;
  // A slur: a step of 1-2 semitones from the note before, quickly, on the same string, onto the
  // beat (or anywhere when the beat is not known), about half the time.
  const double frac = beatFrac();
  const bool onBeat = frac < 0 || frac < 0.15 || frac > 0.85;
  const int step = lastMidi_ >= 0 ? midi - lastMidi_ : 99;
  bool slur = false;
  if (lastString_ >= 0 && std::abs(step) >= 1 && std::abs(step) <= 2 && gapBefore_ < 0.3 && onBeat && strNote_[lastString_] == lastMidi_) {
    const int f = midi - kOpen[lastString_];
    slur = f >= 1 && f <= g.frets() && ur() < 0.5 && !a.ghost;
  }
  int best = -1;
  if (slur) best = lastString_;
  else {
    double cost = 1e9;
    for (int s = 0; s < kS; ++s) {
      const int f = midi - kOpen[s];
      if (f < 0 || f > g.frets()) continue;
      double c = f == 0 ? 1.5 : std::fabs(f - 1 - hand_) * 0.5 + (std::fabs(f - 1 - hand_) > 4 ? 3 : 0);
      if (f > 0 && (f < 2 || f > 13)) c += 0.5;
      if (held_[s] && strNote_[s] != midi) c += 6;
      if (c < cost) { cost = c; best = s; }
    }
  }
  if (best < 0) return;
  const int fret = midi - kOpen[best];
  // A ghost note in a line: much softer than its neighbours.
  const bool ghost = a.ghost || (a.velocity <= 0.6 * lastVel_ && gapBefore_ < 0.4);
  // The pick's way: down on the beat, up off it when quick; else down and up in turn.
  bool up;
  if (frac >= 0) up = !onBeat && gapBefore_ < 0.35 && pickWay_ > 0;
  else up = gapBefore_ < 0.3 && pickWay_ > 0;
  if (gapBefore_ < 0.3 && lastString_ >= 0 && best != lastString_) {
    // (Economy: crossing towards the next string, the pick goes on the way it was going.)
    up = best < lastString_ ? true : up;
  }
  const bool thumb = thumb_ >= 2 || (thumb_ == 1 && a.velocity < 50 && clock_ - lastOctave_ < 3);
  const double v = std::clamp(a.velocity / 127.0, 0.0, 1.0);
  Touch t = touch(v, up && !thumb, best);
  if (thumb) { t.from = std::clamp(0.18 * std::pow(2.0, 1 - brightness_), 0.08, 0.24); t.depth *= 1.25; up = false; }
  if (!slur) pickWay_ = up ? -1 : 1;
  if (ghost) t.depth *= 0.6;
  const Kind kind = slur ? kHammer : thumb ? kThumb : kPick;
  due_.push_back({0, kind, best, fret, t, ghost ? 0.015 + 0.01 * ur() : 0.0, ghost ? 150.0 : 0.0});
  sounding(best, a.midi, fret, a.velocity, a.ghost);
  if (fret > 0) hand_ += 0.4 * (fret - 1 - hand_);
  // (The other strings, not holding a note, under the hands: they do not ring along.)
  for (int s = 0; s < kS; ++s)
    if (s != best && !held_[s] && !hold_) g.damp(s, 40);
  hand_ = std::clamp(hand_, 1.0, 15.0);
  lastString_ = best;
  lastMidi_ = midi;
  lastVel_ = a.velocity;
  bendString_ = fret > 0 ? best : -1;
  if (debug_) std::fprintf(stderr, "note %d s%d f%d %s%s%s v%d: %.0f mm from the saddle, %.2f mm, push %.1f ms, slip %.2f ms\n", a.midi, 6 - best, fret,
                           slur ? (step > 0 ? "hammer" : "pull") : thumb ? "thumb" : up ? "up" : "down", ghost ? " ghost" : "", "", a.velocity,
                           t.from * 1000, t.depth * 1000, t.push * 1000, t.slip * 1000);
}

void JazzGuitarPlayer::play() {
  std::vector<Asked> notes = asked_;
  asked_.clear();
  std::sort(notes.begin(), notes.end(), [](const Asked& a, const Asked& b) { return a.midi < b.midi; });
  double vm = 0;
  for (const Asked& a : notes) vm += a.velocity / double(notes.size());
  if (notes.size() == 1) single(notes[0]);
  else if (notes.size() == 2 && notes[1].midi - notes[0].midi == 12) octave(notes[0], notes[1]);
  else chord(notes);
  velMean_ += 0.15 * (vm - velMean_);
  if (notes.size() > 1) { lastString_ = -1; lastMidi_ = -1; }
}

void JazzGuitarPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  Guitar& g = m_.guitar();
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double dt = m / fs_;
    // (What comes within a few ms is one gesture: a chord whose keys were not struck quite together.)
    if (!asked_.empty() && askAge_ >= gather_) play();
    askAge_ += dt;
    tick(dt);
    clock_ += dt;
    sinceNote_ += dt;
    int heldNow = 0;
    for (int s = 0; s < kS; ++s) { age_[s] += dt; heldNow += held_[s]; }
    // The left hand's vibrato: a single held note, nothing new for 0.7 s; shallow (6 cents),
    // in over 0.4 s. Pitch bend: the last single note's string.
    for (int s = 0; s < kS; ++s) {
      const bool on = held_[s] && heldNow == 1 && fret_[s] > 0 && age_[s] > 0.7 && sinceNote_ > 0.7;
      vib_[s] = on ? std::min(1.0, vib_[s] + dt / 0.4) : std::max(0.0, vib_[s] - dt / 0.05);
      const double pushed = s == bendString_ ? 200 * roll_ : 0.0;
      if (vib_[s] <= 0 && std::fabs(pushed) < 0.5) { if (vibPh_[s] != 0) { g.bend(s, 0); vibPh_[s] = 0; } continue; }
      vibPh_[s] += 2 * M_PI * 5.2 * dt;
      g.bend(s, pushed + vib_[s] * 6 * (0.5 - 0.5 * std::cos(vibPh_[s])));
    }
    for (size_t j = 0; j < due_.size();) {
      if ((due_[j].in -= dt) > 0) { ++j; continue; }
      const Due d = due_[j];
      due_.erase(due_.begin() + j);
      const Touch& t = d.t;
      switch (d.kind) {
        case kPick:
          g.fret(d.string, d.fret);
          g.pickContact(d.string, t.depth, t.from, t.angle, t.push, t.slip, stiff_, 0, tipW_);
          break;
        case kThumb:
          // The thumb's pad (6.6): 14 mm of flesh, slow on and slow off.
          g.fret(d.string, d.fret);
          g.pluckPad(d.string, t.depth, t.from, 0.4, 0.014, 0.004, 0.0018);
          break;
        case kFingers:
          g.fret(d.string, d.fret);
          g.pluckPad(d.string, t.depth * 1.1, t.from, 0.7, 0.009, 0.003, 0.0012 - 0.0004 * std::min(1.0, t.depth / 2e-3));
          break;
        case kHammer:
          // The finger comes down on the fret (or pulls off it): the string, still sounding, now
          // this long; the finger's tap a light stroke at its end.
          g.fret(d.string, d.fret);
          g.pick(d.string, 0.25 * t.depth, 0.025, 1.4, 0.0012, 0.01);
          break;
        case kDead:
          g.fret(d.string, d.fret);
          g.pickContact(d.string, 0.5 * t.depth, t.from, t.angle, t.push, t.slip, stiff_, 0, tipW_);
          g.damp(d.string, d.dampRate);
          break;
        case kRelease:
          if (held_[d.string]) break;  // (struck again meanwhile)
          if (t.depth > 0) g.pick(d.string, t.depth, 0.02, 1.2, 0.0008, 0.008);
          g.damp(d.string, d.dampRate);
          break;
        case kDamp:
          if (age_[d.string] >= 0.8 * d.t.push) g.damp(d.string, d.dampRate);  // (unless struck again since)
          break;
      }
      if (d.dampAfter > 0 && d.kind != kDead && d.kind != kRelease) {
        // (A ghost: the damping comes after the stroke.)
        Touch t2{};
        t2.push = d.dampAfter;  // (its age by then)
        due_.push_back({d.dampAfter, kDamp, d.string, d.fret, t2, 0, d.dampRate});
      }
    }
    m_.process(out + i, m);
  }
  blowing_ = !g.asleep() || !due_.empty() || !asked_.empty();
}

}  // namespace pajupilli
