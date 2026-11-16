// A wire brush in a hand, on a drum head (research/toms.md 4.5,
// research/jazz_drums.md 5.6). Not a force pulse: a small mechanism that
// meets the head where the head is.
//
//   - The hand moves a target: down and away again (a tap), down and into
//     the head (a slap, a press), or lying on the head and drawn along it
//     (a sweep).
//   - The handle (a mass) follows the hand through the grip (a spring).
//   - The fan: the wires, a soft spring with much friction, from the handle
//     to the wire ends.
//   - The wire ends, in groups (each a small mass), spread over the fan's
//     footprint and, by the fan's tilt, in height: they land one after
//     another, each on its own spot of the head through a stiff contact.
//   - Drawn along the head, each end rides the coating's grain: its height
//     under the end is a rough profile read at the speed it is drawn. Faster
//     is brighter and louder; pushed against the wires' lie it bites more
//     than pulled along it.
//
// What the head gets is the contact forces at the ends' places; force() is
// their sum (the wires themselves are heard too: the "tss").
#pragma once

#include <cstdint>
#include <vector>

#include "surface.h"

namespace pajupilli {

struct BrushSpec {
  int tips = 24;              // groups of wire ends
  double handle = 0.020;      // kg
  double grip = 2000;         // N/m (a firm hand; 300: a loose one)
  double gripLoss = 1.0;      // share of critical
  double fan = 400;           // N/m, all wires together
  double fanLoss = 2.0;       // share of critical, against the handle
  // The fan's friction end by end (1: every end tied to the handle by it, the
  // brush lands as one soft thing) or on the fan as a whole (0: the ends
  // free to bounce each on its own, a scatter of small touches); and each
  // end's own small loss (share of critical, on its own spring).
  double together = 1.0, tipLoss = 0.0;
  // The wire ends feel only the surface's lowest `modes` modes (0: all; a
  // big cymbal's highest are not theirs to move, and are most of the cost).
  int modes = 0;
  double tipMass = 5e-5;      // kg a group (the ends of half a dozen wires)
  // A group on the film: the film gives under each end (what the head's
  // modes above their top would have given), N/m; its loss: the force
  // x (1 + this x the closing speed), s/m.
  double contact = 2e4;
  double contactLoss = 0.6;
  double length = 0.08, width = 0.04;  // the footprint: across the fan, along the wires (m)
  double grain = 0.18e-3;     // the coating's grain (m)
  double rough = 8e-6;        // its height, rms (m)
  double drag = 0.5;          // the press's own rise and fall on the grain (a share of it, rms)
  double bite = 0.5;          // rougher by this share pushed against the wires, smoother pulled along them
};

class Brush {
 public:
  Brush(double fs, Surface& head, const BrushSpec& spec, uint32_t seed);
  // The fan's place on the head (its middle: x, y from the centre, m) and
  // which way the wires point (rad, from +x); moved there over the next
  // samples as the hand moves (a place held still: no sweep).
  void place(double x, double y, double angle);
  void jump(double x, double y, double angle);  // (lifted: there at once)
  // A stroke from the air: the hand comes down at `speed` (m/s) with the fan
  // tilted so its ends land over `spread` m of height; from the first touch
  // it goes `follow` m further, stays `dwell` s, and lifts.
  // share: how much of the fan gets to the head, 1 all of it .. 0.2 its tip
  // alone. A hand that slows as it arrives (not thrown through) brings only
  // the fan's tip down: fewer wire ends, on a smaller spot, and with follow
  // 0 they are off again at once: little of the brush lies on what it has
  // struck, so that rings on brighter and longer.
  void strike(double speed, double spread, double follow, double dwell, double share = 1);
  // Lying on the head with `load` N (0: lift). The ends ride the grain as
  // place() draws them along.
  void rest(double load);
  bool up() const { return mode_ == kUp; }
  bool touching() const { return touching_ > 0; }
  // One sample, before the head's step(): the ends against the head.
  void step();
  float force() const { return force_; }      // the ends' contact forces, summed (N)
  float scrape() const { return scrape_; }    // ... the part of it that is the grain (N)
  void seed(uint32_t s) { rng_ = 4711u + 104729u * s; }

 private:
  void layout(double share = 1);  // the ends' own places in the footprint, new for each stroke
  void shapes(int i);     // end i's share of each mode where it is now
  double heightAt(int i) const;
  Surface& head_;
  BrushSpec s_;
  double fs_, dt_;
  enum Mode { kUp, kDown, kFollow, kDwell, kLift, kRest } mode_ = kUp;
  // The hand: height and speed; the handle; the path.
  double zHand_ = 0, vHand_ = 0, zHandle_ = 0, vHandle_ = 0;
  double speed_ = 0, follow_ = 0, dwell_ = 0, load_ = 0, t_ = 0, zTouch_ = 0;
  double x_ = 0, y_ = 0, ang_ = 0, xTo_ = 0, yTo_ = 0, angTo_ = 0;
  struct Tip {
    double u, w;        // its place in the footprint (across, along; m)
    double h;           // how far below the handle's line it hangs (m; the tilt)
    double z, v;        // its height and speed
    double travel;      // how far it has been drawn over the grain (in grains)
    double px, py;      // where it was on the head a sample ago
    bool on;
  };
  std::vector<Tip> tip_;
  std::vector<float> phi_, full_;   // [tip][mode]; one shape of all the surface's modes
  std::vector<float> tex_;   // the grain's profile
  int n_, next_ = 0, touching_ = 0, active_ = 0;
  float force_ = 0, scrape_ = 0;
  uint32_t rng_;
};

}  // namespace pajupilli
