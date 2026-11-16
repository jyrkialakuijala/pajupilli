// The hand clap (-inst hand-clap; General MIDI's 39 in the Jazz and Standard
// kits): research/clap_claves_surdo.md 1.
//
// A clap is a Helmholtz resonator struck shut: the air cupped between the
// palms (the cavity, a spring) is squeezed out through the gaps between the
// fingers and at the heels of the hands (the openings, a mass) as the hands
// meet in about a millisecond; it rings at the cavity's own pitch (cupped
// hands low, ~0.8 kHz; flat ones high, ~2 kHz), and the hands' soft tissue
// stops it within a few cycles (Fu et al. 2025). Each clap is two contacts:
// the fingers, then the palms 1-3 ms later. Under it, the escaping air's
// turbulence (a hiss, 2-6 kHz, while the hands close) and the palms' soft
// thud. The General MIDI clap is a group: four people, each their own hands,
// spread over ~20 ms; every stroke drawn afresh, no two alike.
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "tube.h"

namespace pajupilli {

struct Instrument;
bool handClap(const Instrument& inst);  // -inst hand-clap

class ClapModel : public Tube {
 public:
  explicit ClapModel(const Instrument& inst);
  // A clap of the group: the hands' closing speed (m/s) of the loudest
  // clapper; the others a little softer, later, each their own hands.
  void clap(double speed);
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  struct Hit {        // one contact of one clapper's hands
    int wait = 0;     // samples until it starts
    int t = 0, len = 0;    // samples since it started; the squeeze's length
    double amp = 0;   // the volume squeezed out, per unit (scaled)
    double g = 0, k = 0;   // the cavity's SVF (tan(pi f / fs), 1 / Q)
    double ic1 = 0, ic2 = 0, prev = 0;  // its state; the last neck flow
    double ng = 0, nk = 0, n1 = 0, n2 = 0, noise = 0;  // the hiss's band, its level
    double thudW = 0, thudR = 0, tre = 0, tim = 0, thud = 0;  // the palms' thud (a decaying sine)
    double pan = 0;
    bool on = false;
  };
  double ur();
  double un();
  std::vector<Hit> hits_;
  double gain_ = 1;
  uint32_t rng_ = 9001;
  float last_ = 0, sd_ = 0;
  std::vector<float> side_, sideOut_;
};

class ClapPlayer : public Performer {
 public:
  explicit ClapPlayer(ClapModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int) override {}
  void render(float* out, int n) override;
  void setHold(bool) override {}
  void seed(unsigned) override {}
  static double speed(int velocity, double dynamics);  // m/s

 private:
  ClapModel& m_;
};

}  // namespace pajupilli
