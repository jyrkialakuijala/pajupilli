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
//     the note and its neighbours, a cluster at 4-8 x), the stick on the
//     mouth's edge.
#pragma once

#include <cstdint>

#include "modal.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

struct Instrument;
bool metalPerc(const Instrument& inst);  // -inst triangle, cowbell

class MetalPercModel : public Tube {
 public:
  enum Kind { kTriangle, kCowbell };
  explicit MetalPercModel(const Instrument& inst);
  Kind kind() const { return kind_; }
  // A stroke: the beater's speed (m/s); where along the rod (share of its
  // length; the triangle) or of the mouth's edge (the cowbell).
  void strike(double speed, double where);
  // The hand on the triangle (0: off, 1: clasped).
  void setHand(bool on);
  bool hand() const { return hand_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  void excite(double where);
  Kind kind_;
  ModalSet set_;
  int nVib_ = 1;
  double L_ = 0.57;            // the rod's length (m)
  int nBend_[2] = {0, 0};      // bending modes per family (then the longitudinal ones)
  double beta_[2][48] = {};    // their roots (beta L)
  bool hand_ = false;
  double gain_ = 1;
  float last_ = 0, sd_ = 0;
  std::vector<float> side_, sideOut_;
};

class MetalPercPlayer : public Performer {
 public:
  explicit MetalPercPlayer(MetalPercModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned s) override { rng_ = s * 2654435761u + 1; }
  // The beater's speed for a key's velocity (m/s).
  static double speed(int velocity, double dynamics);

 private:
  double ur();
  MetalPercModel& m_;
  bool hold_ = false;
  uint32_t rng_ = 12345;
};

}  // namespace pajupilli
