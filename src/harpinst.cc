#include "harpinst.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "knob.h"
#include "songclock.h"
#include "vmath.h"

namespace pajupilli {

bool harpInstrument(const Instrument& inst) { return std::strcmp(inst.name, "harp") == 0; }

HarpModel::HarpModel(const Instrument& inst) : Tube(inst), h_(inst.fs) {}

void HarpModel::resetAir() {
  Tube::resetAir();
  h_.reset();
}

const float* HarpModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

float HarpModel::sample() {
  last_ = h_.sample();
  if (side_.size() > 4096) side_.clear();  // (nobody takes them: no room)
  side_.push_back(float(outScale()) * h_.side());
  return finish(last_);
}

HarpPlayer::HarpPlayer(HarpModel& m) : Performer(m), m_(m) {
  std::fill(keyString_, keyString_ + 128, -1);
  std::fill(heldBy_, heldBy_ + Harp::kStrings, -1);
  std::fill(pluckedAt_, pluckedAt_ + Harp::kStrings, -1e9);
  std::fill(pluckedPitch_, pluckedPitch_ + Harp::kStrings, -1);
  rollStep_ = knob("PJ_HARP_ROLL", 1.0);      // (x the rolled chord's step; 0: never rolled)
  harmVel_ = knob("PJ_HARP_HARMVEL", 20.0);   // (a lone key this soft: a harmonic; 0: never)
  rest_ = knob("PJ_HARP_REST", 1.0);          // (x the rest before the hands damp; 0: never)
  verbose_ = knob("PJ_HARP_VERBOSE", 0) > 0;  // (each pluck printed)
  // (tests: the strings not played damped by paper woven through them, as Le Carrou measured)
  if (knob("PJ_HARP_PAPER", 0) > 0)
    for (int s = 0; s < Harp::kStrings; ++s) m.harp().damp(s, 20, 0.5, 0.02);
  // (C major until the song's key is known; PJ_HARP_PEDALS: the harpist's own setting, C to B,
  // each b, n or #, kept whatever the song's key)
  for (int l = 0; l < 7; ++l) m.harp().pedal(l, 0);
  if (const char* p = std::getenv("PJ_HARP_PEDALS"))
    for (int l = 0; l < 7 && p[l]; ++l) {
      m.harp().pedal(l, p[l] == 'b' ? -1 : p[l] == '#' ? 1 : 0);
      keyed_ = true;
    }
}

double HarpPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

void HarpPlayer::noteOn(int midi, int velocity) {
  asked_.push_back({midi, velocity});
  shortKey_[midi & 127] = false;
  note_ = midi;
  lastEvent_ = now_;
}

void HarpPlayer::noteOff(int midi) {
  midi &= 127;
  for (const Asked& a : asked_)
    if (a.midi == midi) { shortKey_[midi] = true; return; }  // (let go before it was played: étouffé if it may be)
  lastEvent_ = now_;
  const int s = keyString_[midi];
  if (s < 0) return;
  keyString_[midi] = -1;
  if (heldBy_[s] == midi) heldBy_[s] = -1;
  // Étouffé: a note let go within 0.1 s of its pluck, in no quick passage: the finger put back on
  // the string (its own pluck's place, 1.5 cm), and lifted again.
  if (!hold_ && glissDir_ == 0 && now_ - pluckedAt_[s] < 0.10 && rest_ > 0) {
    // (not before the string has left the finger)
    const double at = std::max(now_, pluckedAt_[s] + 0.03);
    lifts_.push_back({-at, s});  // (negative: a damping, not a lift)
    lifts_.push_back({at + 0.3, s});
  }
}

void HarpPlayer::setHold(bool h) {
  hold_ = h;
  if (h)
    for (int s = 0; s < Harp::kStrings; ++s) m_.harp().damp(s, 0);
}

// The pedal of a letter to pos: its strings that ring loud damped first (but `except`, about to be
// plucked).
void HarpPlayer::setPedal(int letter, int pos, int except) {
  Harp& h = m_.harp();
  if (h.pedalOf(letter) == pos) return;
  for (int q = letter; q < Harp::kStrings; q += 7)
    if (q != except && Harp::hasDiscs(q) && h.energy(q) > 3e-6 && !hold_) {
      h.damp(q, 120, 0.42, 0.03);
      lifts_.push_back({now_ + 0.2, q});
    }
  h.pedal(letter, pos);
  ++pedalsMoved_;
}

// Palms on the strings below `below` that sound over minEnergy (J).
void HarpPlayer::dampStrings(int below, double rate, double minEnergy, int except) {
  Harp& h = m_.harp();
  for (int s = 0; s < std::min(below, int(Harp::kStrings)); ++s)
    if (s != except && h.energy(s) > minEnergy && !h.fingerOn(s)) {
      h.damp(s, rate, 0.42, 0.08);
      lifts_.push_back({now_ + 0.4, s});
    }
}

// The string for a pitch, and its pedal: the one that already gives it, else the one whose pedal
// moving disturbs least.
int HarpPlayer::choose(int midi, const std::vector<int>& taken, int* posOut) {
  Harp& h = m_.harp();
  const SongClock& sc = SongClock::get();
  const int tonic = sc.valid && sc.tonic >= 0 ? (sc.minor ? (sc.tonic + 3) % 12 : sc.tonic) : 0;
  const bool flatKey = tonic == 5 || tonic == 10 || tonic == 3 || tonic == 8 || tonic == 1;
  int best = -1, bestPos = 0;
  double bestCost = 1e18;
  for (int s = 0; s < Harp::kStrings; ++s) {
    const int acc = midi - Harp::natural(s);
    if (acc < -1 || acc > 1) continue;
    if (!Harp::hasDiscs(s) && acc != 0) continue;
    if (std::find(taken.begin(), taken.end(), s) != taken.end()) continue;
    const int letter = s % 7;
    double c = 0;
    if (Harp::hasDiscs(s) && h.pedalOf(letter) != acc) {
      c += 10;
      for (int q = letter; q < Harp::kStrings; q += 7) {
        if (q == s || !Harp::hasDiscs(q)) continue;
        if (h.energy(q) > 1e-6) c += 20;       // (it would bend a string that rings)
        if (heldBy_[q] >= 0) c += 50;          // (... or change a held note)
        if (std::find(taken.begin(), taken.end(), q) != taken.end()) c += 1000;  // (... or one of this chord's)
      }
      if (pedalsMoved_ >= 2) c += 30;          // (two feet)
      if (now_ - letterAt_[letter] < 2.0) c += 8;  // (its letter just sounded as it is: keep it so)
    }
    if (heldBy_[s] >= 0) c += 15;
    if (now_ - pluckedAt_[s] < 0.3 && h.energy(s) > 1e-6) c += 6;  // (a twin string free: the repeats alternate)
    if (acc != 0) c += 0.5;
    if ((acc == 1 && flatKey) || (acc == -1 && !flatKey)) c += 0.3;
    if (c < bestCost) {
      bestCost = c;
      best = s;
      bestPos = acc;
    }
  }
  *posOut = bestPos;
  return best;
}

void HarpPlayer::play() {
  Harp& h = m_.harp();
  const SongClock& sc = SongClock::get();
  if (!keyed_) {
    // The pedals set for the song's key before the first note (the harp silent: nothing bends).
    keyed_ = true;
    if (sc.valid && sc.tonic >= 0) {
      const int tonic = sc.minor ? (sc.tonic + 3) % 12 : sc.tonic;
      static const int kMajor[7] = {0, 2, 4, 5, 7, 9, 11};
      bool in[12] = {};
      for (int d : kMajor) in[(tonic + d) % 12] = true;
      const bool flats = tonic == 5 || tonic == 10 || tonic == 3 || tonic == 8 || tonic == 1;
      for (int l = 0; l < 7; ++l) {
        const int nat = kMajor[l];
        int pos = 0;
        if (!in[nat]) pos = flats ? -1 : 1;
        if (!in[(nat + pos + 12) % 12]) pos = -pos;
        if (!in[(nat + pos + 12) % 12]) pos = 0;
        h.pedal(l, pos);
      }
    }
  }
  std::sort(asked_.begin(), asked_.end(), [](const Asked& a, const Asked& b) { return a.midi < b.midi; });
  const double gap = now_ - lastGesture_;
  pedalsMoved_ = 0;
  restDamped_ = endDamped_ = false;
  struct Note { int midi, vel, string; bool harm, shortK; };
  std::vector<Note> notes;
  std::vector<int> taken;
  int vsum = 0;
  for (const Asked& a : asked_) {
    int midi = a.midi;
    while (midi < 24) midi += 12;
    while (midi > 103) midi -= 12;
    // A lone soft note in the harmonics' range: the octave harmonic of the string below.
    const bool harm = harmVel_ > 0 && asked_.size() <= 2 && a.velocity <= harmVel_ && midi >= 52 && midi <= 93 && gap > 0.12;
    int pos = 0;
    const int s = choose(harm ? midi - 12 : midi, taken, &pos);
    if (s < 0) continue;
    if (Harp::hasDiscs(s)) setPedal(s % 7, pos, s);
    taken.push_back(s);
    notes.push_back({a.midi, a.velocity, s, harm, shortKey_[a.midi & 127]});
    vsum += a.velocity;
  }
  asked_.clear();
  if (notes.empty()) return;
  const int n = int(notes.size());
  // The glissando: single notes under 60 ms apart, 1-4 semitones on in one direction.
  int fillFrom = -1;
  double fillGap = 0;
  if (n == 1 && !notes[0].harm) {
    const Recent r{now_, notes[0].midi, notes[0].string};
    if (glissDir_ != 0) {
      const Recent& p = recent_.back();
      const int d = r.midi - p.midi;
      if (r.t - p.t < 0.08 && d * glissDir_ > 0 && std::abs(d) <= 5) {
        fillFrom = p.string;
        fillGap = r.t - p.t;
      } else {
        glissDir_ = 0;
      }
    }
    recent_.push_back(r);
    if (recent_.size() > 4) recent_.erase(recent_.begin());
    if (glissDir_ == 0 && recent_.size() >= 3) {
      const Recent &a = recent_[recent_.size() - 3], &b = recent_[recent_.size() - 2], &c = recent_.back();
      const int d1 = b.midi - a.midi, d2 = c.midi - b.midi;
      if (c.t - b.t < 0.06 && b.t - a.t < 0.06 && d1 * d2 > 0 && std::abs(d1) <= 4 && std::abs(d2) <= 4) {
        glissDir_ = d2 > 0 ? 1 : -1;
        fillFrom = b.string;  // (the finger already sliding from the note before)
        fillGap = c.t - b.t;
        std::fill(glissPc_, glissPc_ + 12, false);
        for (const Recent* q : {&a, &b, &c}) glissPc_[q->midi % 12] = true;
        if (verbose_) std::printf("  (glissando %s)\n", glissDir_ > 0 ? "up" : "down");
      }
    }
    if (glissDir_ != 0) glissPc_[notes[0].midi % 12] = true;
  } else {
    glissDir_ = 0;
    recent_.clear();
  }
  // The hands: the left the bass (below C4), the right the treble, four notes each at most.
  std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) { return a.string < b.string; });
  int split = 0;
  while (split < n && notes[split].string < 21) ++split;
  if (split > 4 && n - split < 4) split = std::max(4, n - 4);
  if (n - split > 4 && split < 4) split = std::min(n - 4, 4);
  // The bass moving to a new harmony: the left hand stops the bass strings it played that are
  // not in it (not those whose keys are still held).
  if (split > 0 && !hold_ && rest_ > 0) {
    bool pc[12] = {};
    for (const Note& q : notes) pc[h.sounds(q.string) % 12] = true;
    for (int k = 0; k < 128; ++k)
      if (keyString_[k] >= 0) pc[k % 12] = true;
    for (int s = 0; s < 21; ++s) {
      if (std::find(taken.begin(), taken.end(), s) != taken.end() || heldBy_[s] >= 0 || !leftHand_[s]) continue;
      if (now_ - pluckedAt_[s] > 10 || pc[h.sounds(s) % 12] || h.energy(s) < 3e-7) continue;
      h.damp(s, 60, 0.42, 0.06);
      lifts_.push_back({now_ + 0.25, s});
    }
  }
  // Rolled (three or more notes, not in a quick succession of chords) or together.
  const double vmean = double(vsum) / n;
  const bool rolled = n >= 3 && rollStep_ > 0 && gap > 0.25;
  const double step = rollStep_ * (0.030 - 0.012 * vmean / 127);
  const double bright = brightness_;
  auto make = [&](const Note& q, int finger, bool gliss) {
    Harp::Pluck p;
    const double v = std::clamp(q.vel, 1, 127) / 127.0;
    const int s = q.string;
    // (how far: Chadefaux's pulls were measured down to a 17.6 cm string; a shorter one is drawn
    // out less, as much of its length as that one)
    p.depth = 0.0005 * fastPow(18.0, v) * std::min(1.0, h.length(s) / 0.176);
    // (the middle of the string, a little higher up in the treble; near the board when asked)
    double at = 0.37 + 0.05 * s / 46.0 + 0.02 * (ur() - 0.5);
    if (bright >= 1.6) at = std::clamp(0.12 - 0.05 * (bright - 1.6) / 0.9, 0.06, 0.12);
    else at *= std::clamp(1.25 - 0.25 * bright, 0.85, 1.2);
    p.at = at;
    const bool thumb = finger == 0;
    p.width = thumb ? 0.025 : 0.02;
    static const double kAngle[4] = {0.30, 0.45, 0.40, 0.35};  // (rad off the strings' plane: thumb .. ring finger)
    static const double kAngleX = knob("PJ_HARP_ANGLE", 1.0);  // (x the release's angle off the strings' plane; 0: in it)
    p.angle = kAngleX * (kAngle[std::clamp(finger, 0, 3)] + 0.25 * (ur() - 0.5));
    p.pull = std::clamp(0.8 / h.frequency(s), 0.004, 0.025);
    p.slip = (0.0035 - 0.0015 * v) * (0.8 + 0.4 * ur());
    p.carry = 0.0003 + 0.0012 * v;
    p.off = 0.0005;
    if (gliss) {
      // (the finger sliding across: in the plane, hardly stopping, lighter)
      p.depth *= 0.7;
      p.angle = 0.05 + 0.1 * ur();
      p.pull = 0.0025;
      p.slip = 0.0012;
      p.stop = 0.3;
      p.width = 0.015;
      p.carry = 0.0005;
    }
    if (q.harm) {
      // (the thumb in the lower half, the hand's side at the middle; mf and softer)
      p.at = 0.25;
      p.width = 0.025;
      p.angle = 0.3;
      p.harmonic = 0.004;
      // (a harmonic is played mezzo-forte or softer whatever the key's softness, which only marked
      // it: "one at f is hardly louder than one at p")
      p.depth = 0.0005 * fastPow(18.0, 0.45 + 0.3 * v) * std::min(1.0, h.length(s) / 0.176);
    }
    return p;
  };
  // Which finger: in each hand the thumb the highest, the fingers below it.
  std::vector<int> finger(n);
  for (int i = 0; i < split; ++i) finger[i] = std::min(3, split - 1 - i);
  for (int i = split; i < n; ++i) finger[i] = std::min(3, n - 1 - i);
  double delay = 0;
  if (fillFrom >= 0) {
    // The strings the run passes over: set to the run's pitches if their pedals are free, and
    // swept on the way.
    const int to = notes[0].string, dir = to > fillFrom ? 1 : -1;
    const int k = std::abs(to - fillFrom) - 1;
    // (at the finger's speed: the run's notes as far apart as they came, the strings between
    // evenly among them, all a step late)
    const double dd = std::min(fillGap / (k + 1), 0.025);
    for (int j = 1; j <= k; ++j) {
      const int q = fillFrom + dir * j;
      if (Harp::hasDiscs(q) && !glissPc_[h.sounds(q) % 12]) {
        bool used = false;
        for (const Recent& r : recent_) used = used || r.string % 7 == q % 7;
        if (!used)
          for (int pos = -1; pos <= 1; ++pos)
            if (glissPc_[(Harp::natural(q) + pos + 120) % 12]) { setPedal(q % 7, pos, q); break; }
      }
      due_.push_back({now_ + delay, q, make({notes[0].midi, notes[0].vel, q, false, false}, 1, true)});
      pluckedAt_[q] = now_ + delay;
      leftHand_[q] = q < 21;
      delay += dd;
    }
  }
  for (int i = 0; i < n; ++i) {
    const Note& q = notes[i];
    const int s = q.string;
    // (a finger put back on this string, still to come, is for the note before: not this one)
    lifts_.erase(std::remove_if(lifts_.begin(), lifts_.end(), [s](const Lift& l) { return l.at < 0 && l.string == s; }), lifts_.end());
    double at = now_ + delay + (rolled ? step * i : 0.006 * ur());
    if (rolled) at += 0.004 * (ur() - 0.5);
    Harp::Pluck p = make(q, finger[i], glissDir_ != 0);
    due_.push_back({at, s, p});
    // (who holds what)
    if (heldBy_[s] >= 0 && heldBy_[s] != q.midi) keyString_[heldBy_[s]] = -1;
    heldBy_[s] = q.midi;
    keyString_[q.midi & 127] = s;
    pluckedAt_[s] = at;
    pluckedPitch_[s] = q.midi;
    letterAt_[s % 7] = now_;
    leftHand_[s] = i < split;
    // A key let go before it was played: étouffé at once after the pluck, if it may be.
    if (q.shortK) {
      keyString_[q.midi & 127] = -1;
      heldBy_[s] = -1;
      if (!hold_ && glissDir_ == 0 && gap > 0.15 && rest_ > 0) {
        lifts_.push_back({-(at + p.pull + p.slip + 0.03), s});
        lifts_.push_back({at + 0.35, s});
      }
    }
  }
  lastGesture_ = now_;
}

void HarpPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  Harp& h = m_.harp();
  if (!asked_.empty()) play();
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    now_ += m / fs_;
    for (size_t j = 0; j < due_.size();) {
      if (due_[j].at <= now_) {
        h.pluck(due_[j].string, due_[j].p);
        if (verbose_) {
          static const char* const kNames[7] = {"C", "D", "E", "F", "G", "A", "B"};
          const int st = due_[j].string, pos = Harp::hasDiscs(st) ? h.pedalOf(st % 7) : 0;
          std::printf("  %.3f s  pluck %s%s%d (string %d)%s%s\n", now_, kNames[st % 7], pos < 0 ? "b" : pos > 0 ? "#" : "",
                      st / 7 + 1, st, due_[j].p.harmonic > 0 ? " harmonic" : "", due_[j].p.stop < 1 ? " gliss" : "");
        }
        due_.erase(due_.begin() + j);
      } else {
        ++j;
      }
    }
    // The hands: put on (a negative time: the finger back on a string, étouffé) and lifted.
    for (size_t j = 0; j < lifts_.size();) {
      const Lift& l = lifts_[j];
      if (l.at < 0 && -l.at <= now_) {
        if (!hold_ && !h.fingerOn(l.string)) h.damp(l.string, 80, 0.4, 0.015);
        lifts_.erase(lifts_.begin() + j);
      } else if (l.at >= 0 && l.at <= now_) {
        // (not a hand still needed: a newer damping on it keeps its own lift)
        bool later = false;
        for (const Lift& o : lifts_)
          if (&o != &l && o.string == l.string && std::fabs(o.at) > l.at) later = true;
        if (!later && !h.fingerOn(l.string)) h.damp(l.string, 0);
        lifts_.erase(lifts_.begin() + j);
      } else {
        ++j;
      }
    }
    // At a rest (no key held, nothing played for a beat) the palms damp the strings below C5;
    // at a general pause, all of them.
    if (!hold_ && rest_ > 0 && due_.empty()) {
      bool held = false;
      for (int s = 0; s < Harp::kStrings; ++s) held = held || heldBy_[s] >= 0;
      const SongClock& sc = SongClock::get();
      const double beat = sc.valid ? std::max(0.35, sc.beatLen) : 0.6;
      if (!held && !restDamped_ && now_ - lastEvent_ > rest_ * beat) {
        restDamped_ = true;
        dampStrings(28, 60, 1e-7);
      }
      if (sc.valid && sc.pause && !endDamped_ && !held && now_ - lastEvent_ > 0.2) {
        endDamped_ = true;
        dampStrings(Harp::kStrings, 60, 1e-9);
      }
    }
    m_.process(out + i, m);
    static const int kDbg = int(knob("PJ_HARP_ENERGY", -1));
    if (kDbg >= 0 && (int(now_ * fs_) / m) % int(0.1 * fs_ / m) == 0) std::printf("  energy %.2f s %.4e J, in the plane %.4e J\n", now_, h.energy(kDbg), h.energyInPlane(kDbg));
  }
  blowing_ = !h.asleep() || !due_.empty();
}

}  // namespace pajupilli
