// A long note shaped by its player where the director gives it no shape (research/panflute.md
// 7c): from what the player hears around it (the cue's kCueOwn*, filled by src/midifile.cc,
// where the considerate choices are made: not a chord's or a pad's note, not swelling under a
// melody, less on each of a run of long notes). Over a busy band it expands (grows, then holds);
// over a quiet one it sinks a little, in its later part less once the vibrato has come in; leading
// up into a higher, harder note it grows towards its end. The pan flute plays it itself
// (src/player.cc); the other winds and the strings through their part's level (src/midifile.cc).
#pragma once

#include <algorithm>

#include "director.h"

namespace pajupilli {

struct LongShape {
  bool on = false;
  double len = 0, exp = 0, cresc = 0, decr = 0;  // s; dynamics (the level's units)
};

// The shape for a note's cue, with the player's own amounts (dynamics); tapered: a phrase's last
// note that tapers already (it sinks no further); scatter: -1 .. 1, this note's own.
inline LongShape longShape(const Cue& c, double kExp, double kCresc, double kDecr, double kLong, bool tapered, double scatter) {
  LongShape s;
  // (the cue gives none: a cue of the vibrato alone, or no swell and the level hardly moving)
  const bool flat = c.v[kCueOnly] > 0.5f || (c.v[kCueSwell] < 0.02f && std::abs(c.v[kCueLevelEnd] - c.v[kCueLevel]) < 0.05f);
  s.len = c.v[kCueOwnLen];
  s.on = flat && s.len >= 0.6;
  if (!s.on) return s;
  // (from 0.6 s it comes in, full from kLong + 0.3 s, deeper for the longest; never the same twice)
  const double size = std::clamp((s.len - 0.6) / std::max(0.05, kLong - 0.3), 0.0, 1.0) * std::clamp(0.5 + s.len / 3, 0.8, 1.3) * (1 + 0.2 * scatter);
  // (busy: a note-on a second or fewer is quiet, 3 or more plenty)
  const double busy = std::clamp((double(c.v[kCueOwnBusy]) - 1) / 2, 0.0, 1.0), ramp = std::clamp(double(c.v[kCueOwnRamp]), 0.0, 1.0);
  const double grow = std::clamp(double(c.v[kCueOwnSize]), 0.0, 1.0);  // (how far it may grow, considered)
  s.cresc = kCresc * size * ramp * grow;
  s.exp = kExp * size * busy * (1 - ramp) * grow;
  s.decr = tapered ? 0 : kDecr * size * (1 - busy) * (1 - ramp);
  return s;
}

inline double longSmooth(double a, double b, double x) {
  const double t = std::clamp((x - a) / (b - a), 0.0, 1.0);
  return t * t * (3 - 2 * t);
}

// One step of it, t s into the note (dt s since the last): the dynamics to add now. sink: its
// running decrescendo (start at 0); vib: the vibrato's presence now, 0 .. 1.
inline double longShapeAt(const LongShape& s, double t, double dt, double vib, double& sink) {
  if (!s.on) return 0;
  const double u = std::clamp(t / std::max(0.1, s.len), 0.0, 1.0);
  // (the vibrato eases the sinking only in the note's later part: 40 .. 70 % of it on)
  if (u < 1) sink += s.decr / std::max(0.1, s.len) * dt * (1 - 0.75 * vib * longSmooth(0.4, 0.7, u));
  // (expansion: a grow to a hold by 60 %, easing a little over the last 15 %; the crescendo from 40 %)
  return s.exp * longSmooth(0, 0.6, u) * (1 - 0.3 * longSmooth(0.85, 1, u)) + s.cresc * longSmooth(0.4, 1, u) - sink;
}

}  // namespace pajupilli
