#include "slurs.h"

#include <algorithm>
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
  SlurGesture g;
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
  // (the tongue touches or it does not: the nearer dynamics' way)
  const SlurGesture& n = t < 0.5f ? a : b;
  g.tongue = n.tongue;
  g.floor = n.floor;
  g.fails = n.fails;
  return g;
}

// Among the rows of one slur (the same `from` and interval), the gesture at dyn: between the two
// dynamics around it, the end ones beyond.
bool atDyn(const char* inst, int from, int interval, double dyn, SlurGesture* g) {
  const SlurRow *below = nullptr, *above = nullptr;
  for (const SlurRow& r : kSlurRows) {
    if (r.from != from || r.interval != interval || std::strcmp(r.inst, inst) != 0) continue;
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
    return t;
  }();
  *g = kG;
  return true;
}

}  // namespace

bool slurGesture(const char* inst, int from, int to, double dyn, SlurGesture* g) {
  static const bool kOn = knob("PJ_SLURS", 1) > 0;
  if (tryGesture(g)) return true;
  if (!kOn || from == to) return false;
  const int iv = to - from;
  if (atDyn(inst, from, iv, dyn, g)) return true;
  // The general row: the nearest practised interval the same way.
  int best = 0;
  for (const SlurRow& r : kSlurRows)
    if (r.from < 0 && (r.interval > 0) == (iv > 0) && std::strcmp(r.inst, inst) == 0 &&
        (best == 0 || std::abs(r.interval - iv) < std::abs(best - iv)))
      best = r.interval;
  return best != 0 && atDyn(inst, -1, best, dyn, g);
}

bool slurPlayable(const char* inst, int from, int to, double dyn) {
  SlurGesture g;
  return !slurGesture(inst, from, to, dyn, &g) || !g.fails;
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
