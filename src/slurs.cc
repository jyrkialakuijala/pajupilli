#include "slurs.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "knob.h"

namespace pajupilli {

namespace {

struct SlurRow {
  const char* inst;
  int from;      // the key left (sounding MIDI); -1: the general row for the interval
  int interval;  // semitones, signed
  float dyn;
  float pre;     // s the note left had sounded when practised: kSlow settled, kQuick only just spoken
  SlurGesture g;
};
constexpr float kSlow = 0.6f, kQuick = 0.1f;
constexpr float kTrill = 0.0f;  // (pre of a trill's rows: `from` the lower key, the interval up)
struct AttackRow {
  const char* inst;
  int key;
  float dyn;
  AttackGesture g;
};
struct TrillRow {
  const char* inst;
  int lo, interval;
  float rate;  // notes per second
};

#include "slur_data.h"

SlurGesture mix(const SlurGesture& a, const SlurGesture& b, float t) {
  SlurGesture g;
  auto l = [t](float x, float y) { return x + (y - x) * t; };
  g.fingers = l(a.fingers, b.fingers);
  g.lead = l(a.lead, b.lead);
  g.air = l(a.air, b.air);
  g.airT = l(a.airT, b.airT);
  g.lip = l(a.lip, b.lip);
  g.lipT = l(a.lipT, b.lipT);
  g.dip = l(a.dip, b.dip);
  g.dipT = l(a.dipT, b.dipT);
  g.aperture = l(a.aperture, b.aperture);
  g.firm = l(a.firm, b.firm);
  g.focus = l(a.focus, b.focus);
  g.vowel = l(a.vowel, b.vowel);
  g.vowelAt = l(a.vowelAt, b.vowelAt);
  g.setT = l(a.setT, b.setT);
  // (the tongue touches or it does not: the nearer dynamics' way)
  const SlurGesture& n = t < 0.5f ? a : b;
  g.tongue = n.tongue;
  g.floor = n.floor;
  g.fails = n.fails;
  return g;
}

// Among the rows of one slur (the same `from`, interval and speed), the gesture at dyn: between
// the two dynamics around it, the end ones beyond.
bool atDyn(const char* inst, int from, int interval, float pre, double dyn, SlurGesture* g) {
  const SlurRow *below = nullptr, *above = nullptr;
  for (const SlurRow& r : kSlurRows) {
    if (r.from != from || r.interval != interval || r.pre != pre || std::strcmp(r.inst, inst) != 0) continue;
    if (r.dyn <= dyn && (!below || r.dyn > below->dyn)) below = &r;
    if (r.dyn > dyn && (!above || r.dyn < above->dyn)) above = &r;
  }
  if (!below && !above) return false;
  if (!below) *g = above->g;
  else if (!above) *g = below->g;
  else *g = mix(below->g, above->g, float((dyn - below->dyn) / (above->dyn - below->dyn)));
  return true;
}

bool tryGesture(SlurGesture* g) {
  static const bool kTry = knob("PJ_SLURTRY", 0) > 0;
  if (!kTry) return false;
  static const SlurGesture kG = [] {
    SlurGesture t;
    t.fingers = float(knob("PJ_SLURTRY_FINGERS", t.fingers));
    t.lead = float(knob("PJ_SLURTRY_LEAD", t.lead));
    t.air = float(knob("PJ_SLURTRY_AIR", t.air));
    t.airT = float(knob("PJ_SLURTRY_AIRT", t.airT));
    t.lip = float(knob("PJ_SLURTRY_LIP", t.lip));
    t.lipT = float(knob("PJ_SLURTRY_LIPT", t.lipT));
    t.dip = float(knob("PJ_SLURTRY_DIP", t.dip));
    t.dipT = float(knob("PJ_SLURTRY_DIPT", t.dipT));
    t.tongue = float(knob("PJ_SLURTRY_TONGUE", t.tongue));
    t.floor = float(knob("PJ_SLURTRY_FLOOR", t.floor));
    t.aperture = float(knob("PJ_SLURTRY_APERTURE", t.aperture));
    t.firm = float(knob("PJ_SLURTRY_FIRM", t.firm));
    t.focus = float(knob("PJ_SLURTRY_FOCUS", t.focus));
    t.vowel = float(knob("PJ_SLURTRY_VOWEL", t.vowel));
    t.vowelAt = float(knob("PJ_SLURTRY_VOWELAT", t.vowelAt));
    t.setT = float(knob("PJ_SLURTRY_SETT", t.setT));
    return t;
  }();
  *g = kG;
  return true;
}

// The slur as practised at one speed. At each practised dynamics its own row there, else the
// general one of the nearest practised interval the same way; between the two dynamics around
// dyn, the end ones beyond. (A slur with its own row at one dynamics only is hard there alone:
// the practice found the general way good at the others. Its one row used at every dynamics
// played the trumpet's F3 to A#3 at soft dynamics the forte's way, ending 47 c flat.)
bool atSpeed(const char* inst, int from, int iv, float pre, double dyn, SlurGesture* g) {
  int best = 0;
  for (const SlurRow& r : kSlurRows)
    if (r.from < 0 && r.pre == pre && (r.interval > 0) == (iv > 0) && std::strcmp(r.inst, inst) == 0 &&
        (best == 0 || std::abs(r.interval - iv) < std::abs(best - iv)))
      best = r.interval;
  // (the general rows' dynamics are the grid; with none, the slur's own rows alone)
  if (best == 0) return atDyn(inst, from, iv, pre, dyn, g);
  const SlurRow *below = nullptr, *above = nullptr;
  for (const SlurRow& r : kSlurRows) {
    if (r.from >= 0 || r.interval != best || r.pre != pre || std::strcmp(r.inst, inst) != 0) continue;
    if (r.dyn <= dyn && (!below || r.dyn > below->dyn)) below = &r;
    if (r.dyn > dyn && (!above || r.dyn < above->dyn)) above = &r;
  }
  auto at = [&](const SlurRow* gen) -> const SlurGesture& {
    for (const SlurRow& r : kSlurRows)
      if (r.from == from && r.interval == iv && r.pre == pre && r.dyn == gen->dyn && std::strcmp(r.inst, inst) == 0) return r.g;
    return gen->g;
  };
  if (!below && !above) return atDyn(inst, from, iv, pre, dyn, g);
  if (!below) *g = at(above);
  else if (!above) *g = at(below);
  else *g = mix(at(below), at(above), float((dyn - below->dyn) / (above->dyn - below->dyn)));
  return true;
}

}  // namespace

bool slurGesture(const char* inst, int from, int to, double dyn, SlurGesture* g, double sounded, bool trill) {
  static const bool kOn = knob("PJ_SLURS", 1) > 0;
  // (PJ_SLURSPEED=0: every slur as from a settled note, the practice before the quick rows)
  static const bool kSpeed = knob("PJ_SLURSPEED", 1) > 0;
  if (tryGesture(g)) return true;
  if (!kOn || from == to) return false;
  const int iv = to - from;
  static const bool kTrills = knob("PJ_TRILLWAYS", 1) > 0;
  if (trill && kTrills && atSpeed(inst, std::min(from, to), std::abs(iv), kTrill, dyn, g)) return true;
  SlurGesture slow, quick;
  const bool hs = atSpeed(inst, from, iv, kSlow, dyn, &slow);
  const bool hq = kSpeed && atSpeed(inst, from, iv, kQuick, dyn, &quick);
  if (!hs && !hq) return false;
  if (!hq) *g = slow;
  else if (!hs) *g = quick;
  else {
    // Between the two by how long the note left has sounded (its logarithm: 0.1 .. 0.4 s).
    const float t = std::clamp(float(std::log(std::max(sounded, 1e-3) / kQuick) / std::log(0.4 / kQuick)), 0.0f, 1.0f);
    *g = t <= 0 ? quick : t >= 1 ? slow : mix(quick, slow, t);
  }
  return true;
}

namespace {
std::atomic<bool> gAttacks{true};
}
void practisedAttacks(bool on) { gAttacks = on; }

bool attackGesture(const char* inst, int key, double dyn, AttackGesture* g) {
  if (!gAttacks) return false;
  static const bool kOn = knob("PJ_ATTACKS", 1) > 0;
  static const bool kTry = knob("PJ_ATTACKTRY", 0) > 0;
  if (kTry) {
    static const AttackGesture kG = [] {
      AttackGesture t;
      t.release = float(knob("PJ_ATTACKTRY_RELEASE", t.release));
      t.rise = float(knob("PJ_ATTACKTRY_RISE", t.rise));
      t.push = float(knob("PJ_ATTACKTRY_PUSH", t.push));
      t.pushT = float(knob("PJ_ATTACKTRY_PUSHT", t.pushT));
      t.lip = float(knob("PJ_ATTACKTRY_LIP", t.lip));
      t.lipT = float(knob("PJ_ATTACKTRY_LIPT", t.lipT));
      t.aperture = float(knob("PJ_ATTACKTRY_APERTURE", t.aperture));
      t.firm = float(knob("PJ_ATTACKTRY_FIRM", t.firm));
      t.vowel = float(knob("PJ_ATTACKTRY_VOWEL", t.vowel));
      t.vowelAt = float(knob("PJ_ATTACKTRY_VOWELAT", t.vowelAt));
      t.setT = float(knob("PJ_ATTACKTRY_SETT", t.setT));
      return t;
    }();
    *g = kG;
    return true;
  }
  if (!kOn) return false;
  // The key's rows (the nearest practised key within a tone), between the dynamics around dyn.
  int best = -1;
  for (const AttackRow& r : kAttackRows)
    if (*r.inst && std::strcmp(r.inst, inst) == 0 && std::abs(r.key - key) <= 2 &&
        (best < 0 || std::abs(r.key - key) < std::abs(best - key)))
      best = r.key;
  if (best < 0) return false;
  const AttackRow *below = nullptr, *above = nullptr;
  for (const AttackRow& r : kAttackRows) {
    if (r.key != best || std::strcmp(r.inst, inst) != 0) continue;
    if (r.dyn <= dyn && (!below || r.dyn > below->dyn)) below = &r;
    if (r.dyn > dyn && (!above || r.dyn < above->dyn)) above = &r;
  }
  if (!below) *g = above->g;
  else if (!above) *g = below->g;
  else {
    const float t = float((dyn - below->dyn) / (above->dyn - below->dyn));
    const AttackGesture &a = below->g, &b = above->g;
    auto l = [t](float x, float y) { return x + (y - x) * t; };
    *g = {l(a.release, b.release), l(a.rise, b.rise), l(a.push, b.push), l(a.pushT, b.pushT), l(a.lip, b.lip), l(a.lipT, b.lipT),
          l(a.aperture, b.aperture), l(a.firm, b.firm), l(a.vowel, b.vowel), l(a.vowelAt, b.vowelAt), l(a.setT, b.setT)};
  }
  return true;
}

bool slurPlayable(const char* inst, int from, int to, double dyn, double sounded) {
  SlurGesture g;
  return !slurGesture(inst, from, to, dyn, &g, sounded) || !g.fails;
}

double trillLimit(const char* inst, int lo, int interval) {
  const TrillRow* best = nullptr;
  for (const TrillRow& r : kTrillRows) {
    if (std::strcmp(r.inst, inst) != 0) continue;
    // (the interval first, then the nearest note)
    auto far = [&](const TrillRow& x) { return 100 * std::abs(x.interval - interval) + std::abs(x.lo - lo); };
    if (!best || far(r) < far(*best)) best = &r;
  }
  return best ? best->rate : 0;
}

int trillDivisor(const char* inst, int lo, int interval, double rate) {
  const double lim = trillLimit(inst, lo, interval);
  if (lim <= 0 || rate <= lim) return 1;
  if (rate <= 2 * lim) return 2;
  if (rate <= 4 * lim) return 4;
  return 0;
}

}  // namespace pajupilli
