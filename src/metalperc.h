// The small metal percussion of the drum kit, on the modal engine (modal.h):
// research/triangle_cowbell.md.
//
//   - The triangle (-inst triangle; General MIDI's 80 mute, 81 open): a steel
//     rod (10 mm, 0.57 m: an 8" triangle) bent in three, free. Its bending
//     modes are, as Rossing found, nearly the straight free bar's: one family
//     moving in the triangle's plane, one across it (the corners stiffen it:
//     about 12 % higher), and the rod's first longitudinal modes (c / 2L). A
//     steel beater, a near-instant Hertz contact, wherever on the bottom side
//     (a little different each time). The low modes barely radiate (a thin
//     rod): what is heard is the shimmer from 1 to 15 kHz. The mute: the hand
//     clasps the triangle (a key 80, or the hold); an open stroke lets go.
//   - The cowbell (-inst cowbell; 56): a sheet-iron box, open at the mouth,
//     its modes as measured on a recorded cowbell (the mouth's breathing near
//     the note and its neighbours, a cluster at 4-8 x). Two voices on one
//     bell: the stick's shaft on the mouth's edge (open, low, the clank) or
//     its tip on the body toward the closed crown (dry, high, the ping), and
//     the hand's finger on the face (open ... muted). Key 56 is the open mouth
//     stroke; keys 60 .. 75 are a stroke the player chose (src/cowbellplan.h).
#pragma once

#include <cstdint>

#include "modal.h"
#include "performer.h"
#include "rng.h"
#include "tube.h"

namespace pajupilli {

struct Instrument;
bool metalPerc(const Instrument& inst);  // -inst triangle, cowbell
// The cowbell's chosen strokes: keys kCowbellStroke0 + 4 x place + mute,
// place 0, 1 (the mouth, the side of the stick) .. 2, 3 (the top, its tip),
// mute 0 (open) .. 3 (the lower fingers pressed). Held in the hand, turned as
// it is struck (from 60); mounted on the kit, still (from 40).
constexpr int kCowbellStroke0 = 60, kCowbellMounted0 = 40;

class MetalPercModel : public Tube {
 public:
  enum Kind { kTriangle, kCowbell };
  explicit MetalPercModel(const Instrument& inst);
  Kind kind() const { return kind_; }
  // A stroke: the beater's speed (m/s); where along the rod (share of its
  // length; the triangle) or of the mouth's edge (the cowbell).
  void strike(double speed, double where);
  // The cowbell's stroke: place 0 (the shaft on the mouth's edge) .. 1 (the
  // tip on the crown); mute 0 (open) .. 1 (a finger pressed on the face),
  // set as the stroke lands and kept until the next.
  void strikeBell(double speed, double where, double place, double mute);
  // Where the held bell's mouth turns to, from the listener (degrees: 0
  // toward them, 60 tipped up), and how fast (s); held false: mounted, still,
  // toward the listener.
  void turn(double degrees, double seconds, bool held);
  // The hand on the triangle (0: off, 1: clasped).
  void setHand(bool on);
  bool hand() const { return hand_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  void excite(double where, double place = 0);
  Kind kind_;
  ModalSet set_;
  int nVib_ = 1;
  double L_ = 0.57;            // the rod's length (m)
  int nBend_[2] = {0, 0};      // bending modes per family (then the longitudinal ones)
  double beta_[2][48] = {};    // their roots (beta L)
  bool hand_ = false;
  double gain_ = 1;
  float last_ = 0, sd_ = 0;
  // The held bell's turning: its walls' partials (over ~1.1 kHz) beam along
  // the mouth's axis, its mouth's breathing goes everywhere. A crossover on
  // the sound, the upper band's level and side share from the angle.
  double ang_ = 0, angTo_ = 0, angA_ = 0, sway_ = 0, swayW_ = 0, swayAmp_ = 0, sideDir_ = 1;
  double lp1_ = 0, lp2_ = 0, xb_[3] = {}, xa_[2] = {};
  std::vector<float> side_, sideOut_;
};

class MetalPercPlayer : public Performer {
 public:
  explicit MetalPercPlayer(MetalPercModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned s) override { rng_.reseed(s); }
  // The beater's speed for a key's velocity (m/s).
  static double speed(int velocity, double dynamics);

 private:
  double ur();
  MetalPercModel& m_;
  bool hold_ = false;
  Rng rng_{12345};
};

}  // namespace pajupilli
