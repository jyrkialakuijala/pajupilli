// How a player holding a wind moves it into the room as it plays (Tube::aimOn, setPose): the
// owner, 2026-11-28: "the minimum modulation that these instruments can do": an oboe, a clarinet
// or a recorder a small directional spiral, the sound projected more interestingly; a flute
// player sways a bit; the shakuhachi only up and down (its blowing is delicate), as a trumpet
// raises and lowers its bell.
//
//   - Every kind: the instrument lifts (towards the hall) as the playing grows loud, on an accent
//     and in the high register, and sinks into a phrase's end; between phrases it rests lower.
//   - kUpDown (the shakuhachi): only that.
//   - kFree (clarinet, oboe, recorder): around it the bell circles, a small ellipse (wider than
//     tall), its radius growing over a long note from about 1 to 5 degrees (a spiral outwards),
//     falling back between notes; each phrase its own speed (0.17 .. 0.28 turns a second) and sense.
//   - kTransverse (the flute): the body sways, the flute and the face turning with it, 2 degrees,
//     growing to 5 over a long note, at 0.15 .. 0.25 Hz; the foot a little below the lips.
// The pose follows as a body moves: the lift in ~0.3 s twice over, the side in ~0.6 s. PJ_AIM
// scales the motion (0: held still).
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "knob.h"
#include "rng.h"

namespace pajupilli {

class HeldAim {
 public:
  enum Kind { kUpDown, kFree, kTransverse };
  void setKind(Kind k, uint64_t seed) {
    kind_ = k;
    rng_.reseed(seed);
    scale_ = knob("PJ_AIM", 1);
    down_[0] = down_[1] = rest();
  }
  Kind kind() const { return kind_; }
  // A new phrase begins (a note after a rest, or after a phrase's end).
  void newPhrase() {
    rate_ = kind_ == kTransverse ? 0.15 + 0.1 * u01() : 0.17 + 0.11 * u01();
    if (u01() < 0.5) sense_ = -sense_;
  }
  // dt s on. on: a note sounding; loud 0 .. 1; high 0 .. 1 (the register); accent 0 .. 1;
  // ending: a phrase's last note; sinceOn: s into the note; sinceOff: s since the last ended.
  // The pose (radians: the axis below the horizontal, swung to the listener's right, the face).
  void step(double dt, bool on, double loud, double high, double accent, bool ending, double sinceOn, double sinceOff, double* down, double* side, double* turn) {
    constexpr double deg = M_PI / 180;
    double d = rest(), s = 0;
    if (on) {
      const double lift = kind_ == kTransverse ? 0.25 : 1.0;
      d = rest() - lift * (18 * deg * (loud - 0.5) + 6 * deg * high + 5 * deg * accent);
      if (ending) d += lift * 10 * deg * std::min(1.0, sinceOn / 0.8);  // (bowing into the phrase's end)
    } else if (sinceOff < 1.5) {
      d = down_[0];  // (a short rest: held where it was)
    }
    // (the circling or the sway: wider over a long note, settling between notes)
    const double grow = on ? std::clamp(sinceOn / 3, 0.0, 1.0) : 0.0;
    const double rTo = kind_ == kFree ? (on ? 1 + 4 * grow : 0.5) * deg : kind_ == kTransverse ? (on ? 2 + 3 * grow : 1.5) * deg : 0.0;
    radius_ += (1 - std::exp(-dt / 0.8)) * (rTo - radius_);
    phase_ += 2 * M_PI * rate_ * sense_ * dt;
    if (phase_ > 2 * M_PI) phase_ -= 2 * M_PI;
    if (phase_ < 0) phase_ += 2 * M_PI;
    if (kind_ == kFree) {
      s += radius_ * std::cos(phase_);
      d += 0.7 * radius_ * std::sin(phase_);
    } else if (kind_ == kTransverse) {
      s += radius_ * std::sin(phase_);
    }
    const double lo = kind_ == kTransverse ? 0 : 28 * deg, hi = kind_ == kTransverse ? 20 * deg : 72 * deg;
    d = rest() + scale_ * (std::clamp(d, lo, hi) - rest());
    s *= scale_;
    const double ad = 1 - std::exp(-dt / 0.3), as = 1 - std::exp(-dt / 0.6);
    down_[0] += ad * (d - down_[0]);
    down_[1] += ad * (down_[0] - down_[1]);
    side_[0] += as * (s - side_[0]);
    side_[1] += as * (side_[0] - side_[1]);
    *down = down_[1];
    *side = side_[1];
    *turn = kind_ == kTransverse ? side_[1] : 0.6 * side_[1];
  }

 private:
  double rest() const { return (kind_ == kTransverse ? 8.0 : kind_ == kFree ? 40.0 : 50.0) * M_PI / 180; }
  double u01() { return 0.5 * (rng_.uniform() + 1); }
  Kind kind_ = kUpDown;
  Rng rng_{1};
  double scale_ = 1, rate_ = 0.22, sense_ = 1, phase_ = 0, radius_ = 0;
  double down_[2] = {0.873, 0.873}, side_[2] = {0, 0};
};

}  // namespace pajupilli
