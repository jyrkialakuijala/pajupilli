// The Yamaha CP-80 electric grand (-inst electric-grand, GM 3; research/cp80.md).
//
// A piano without a soundboard: the grand's strings, hammers and dampers
// (piano.h) on a short scale, standing on a heavy, nearly rigid bridge, read
// by a piezo element under each key's strings, the 88 elements in parallel
// on one bus; then the instrument's own circuit, as its overall circuit
// diagram (1978) draws it (research/cp80.md 3.3):
//
//   - the bus: every key alike (a passive sum), the top six keys through a
//     0.022 uF mixing capacitor (a few dB down);
//   - the input's high-pass (0.1 uF into 470 k: 3.4 Hz);
//   - the tone controls (bass, middle, treble: flat at the centre detent) and
//     the brilliance switch, at the corners the diagram's parts give (the
//     network's wiring is not legible on the scan: research/cp80.md 3.3);
//   - the tremolo: a Wien-bridge sine (2.2 uF, 5.6 k + the 100 k speed pot:
//     0.69 .. 12.9 Hz), its peaks held by two Zeners; per channel a
//     transistor lights an LED on one half of each cycle, the LED a
//     light-dependent resistor, which shunts the signal to ground after 22 k
//     (in series with the depth pot): each channel dips once a cycle, the
//     right one driven by the inverted sine: the "180 degree" auto-pan.
#pragma once

#include <vector>

#include "performer.h"
#include "piano.h"
#include "tube.h"

namespace pajupilli {

bool electricGrand(const Instrument& inst);  // -inst electric-grand
PianoDesign cp80Design();

// The circuit after the pickup bus: one input (the bus), two outputs.
class Cp80Circuit {
 public:
  explicit Cp80Circuit(double fs);
  // Bass, middle, treble: -1 .. +1 (the panel's knobs; 0 the centre detent,
  // flat). Brilliance: 0 low, 1 medium, 2 high (flat). Tremolo: depth 0
  // (off) .. 1 (the deepest), speed 0 .. 1 (the pot's travel).
  void setTreble(double t);
  void setBass(double b);
  void setMiddle(double m);
  void setBrilliance(int b);
  void setTremolo(double depth, double speed) { depth_ = depth; speed_ = speed; }
  void process(float in, float* left, float* right);
  void reset();

 private:
  double fs_;
  double hpA_ = 0, hpX_ = 0, hpY_ = 0;           // the input's high-pass
  // The tone section: biquads in series (direct form I), each (b0 b1 b2 a1 a2).
  struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    double run(double x) {
      const double y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
      x2 = x1; x1 = x; y2 = y1; y1 = y;
      return y;
    }
  };
  void shelf(Biquad* q, double fc, double db, bool high);
  Biquad bass_, middle_, treble_, brill_;
  double bassSet_ = 2, middleSet_ = 2, trebleSet_ = 2;
  int brillSet_ = -1;
  double depth_ = 0, speed_ = 0.33, ph_ = 0;     // the oscillator
  double ldrG_[2] = {0, 0};                      // the LDRs' conductance (1/ohm), lagging the light
};

class Cp80Model : public Tube {
 public:
  explicit Cp80Model(const Instrument& inst);
  Piano& piano() { return piano_; }
  Cp80Circuit& circuit() { return circuit_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  Piano piano_;
  Cp80Circuit circuit_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

// The player: the grand's (velocity = hammer speed, the sustain pedal lifts
// every damper). The brightness knob is the treble control, hardness the
// hammers' voicing; the vibrato knobs are the tremolo (K4 depth: 0 off, K5
// speed).
class Cp80Player : public Performer {
 public:
  explicit Cp80Player(Cp80Model& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}

 private:
  Cp80Model& m_;
  bool down_[Piano::kKeys] = {};
};

}  // namespace pajupilli
