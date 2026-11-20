// General MIDI 101, "FX 5 (brightness)" (-inst fx-brightness): a keyboard of singing arcs
// (research/fx_sounds.md).
//
// The classic patch is a bright, held, octave-rich tone that speaks at once: FluidR3 layers a
// chorused "Bright" sample (the note and its octave nearly equal, the partials falling ~2 dB
// each) over synth strings; TimGM6mb and Jnsgm2 a "Solo Vox" over synth strings (an octave up
// in Jnsgm2). Here it is William Duddell's singing arc (1900): a carbon arc fed from a DC supply
// through a ballast resistor, shunted by a series L-C circuit (the tank). The arc's falling
// characteristic (its voltage drops as its current rises: a negative resistance) feeds the tank,
// which oscillates; the arc's power swings with it, and the arc column, heating and cooling,
// pushes on the air: the arc is its own loudspeaker (the pressure goes as the rate of change of
// the power, as in a thermophone). The power is current times voltage: its swing carries the
// note and, nearly as strongly, its octave; the arc's thermal lag and the ballast round the swing
// into a limit cycle whose partials fall steadily (at the default touch the octave ~-1 dB, the
// third and fourth -10 to -14, the tenth ~-30).
//
// The arc is Mayr's: its conductance g relaxes to what its power would hold, dg/dt = (i^2 /
// P(i) - g) / tau, its loss P = phi + (1 - phi) |i| in units of the operating point (the column's
// share phi, the electrode falls the rest: the static characteristic V = Va + P0 / i, Ayrton's
// form), a little residual conductance (the hot electrodes) relighting it if it goes out. The
// ballast (20 x the arc's resistance) makes the supply nearly a constant current; the tank's
// impedance is the arc's resistance, its Q 30; the arc's thermal time constant is 0.5 / the
// tank's angular frequency. The arc sings at ~0.62 of its tank's frequency (its own lag is a
// reactance in the loop): each tank is tuned so that its arc sings on the note, from a table made
// at start by running the circuit (by the column's share and the steps per period).
//
// Each key has two arcs, a large and a small (its sound 0.35 of the other's), their tanks a few
// cents apart (K2), panned apart: the chorus of the sets' L/R samples, beating by a few dB. Every
// arc is the same circuit scaled to its note (the tank, the arc's time constant, its power), so
// the timbre is even over the keyboard. The arc burns silently while its key is up (a steady arc
// makes no sound); the key hands the arc's voltage to the tank through a slide-wire (over ~1 ms,
// at least a radian and a half of the tank: the tank, struck, rings at once) and its rheostat
// brings the current up (3 .. 7 ms). Velocity is the current (the loudness) and the arc's length
// (the column's share 0.30 .. 0.40: a longer arc, a steeper characteristic, a fiercer and
// brighter limit cycle; K1 moves it by +-0.15). The plasma flickers (K3: the column's loss
// swinging by a few %, low-passed at ~3 kHz: a faint hiss and roughness). Released, the rheostat
// runs the current down (the feed keeping the arc's shape, so pitch and timbre hold as it fades,
// K4) and the arc goes out.
//
// It is integrated at 4x the sample rate (the tank by the trapezoid, the arc's conductance held
// over the step and then relaxed exactly) and low-passed back down (the guitar amp's elliptic
// filter).
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool fxBrightness(const Instrument& inst);  // -inst fx-brightness

class SingingArcModel : public Tube {
 public:
  explicit SingingArcModel(const Instrument& inst);
  void keyOn(int midi, int velocity);
  void keyOff(int midi);
  void setBend(double semitones);
  // The knobs, 0..1 (SingingArcPlayer::setKnob).
  void setLength(double x) { length_ = x; }    // K1: the arc's length (the brightness; new notes)
  void setChorus(double x) { chorus_ = x; }    // K2: the two arcs' spread (0 .. 12 c; new notes)
  void setHiss(double x) { hiss_ = x; }        // K3: the arc's flicker (the plasma's turbulence: 0 .. 4 %)
  void setRelease(double x) { release_ = x; }  // K4: the current's run-down at the release (0.5 .. 8 s to -60 dB)
  bool asleep() const { return notes_.empty(); }
  double testResponse() const override { return last_; }
  void resetAir() override;
  const float* takeSide(int n) override;

  static constexpr int kArcs = 2, kOver = 4;
  // One arc and its tank, in units of the operating point (the arc's current and voltage) and
  // of the tank's own period (L C = 1).
  struct Arc {
    double it = 0, vc = 0;  // the tank's current, its capacitor's voltage
    double g = 0;           // the arc's conductance
    double p = 0;           // its power a step ago
    double h = 0;           // the step (radians of the tank)
    double ek = 0;          // the arc's thermal relaxation over a step
    double ratio = 1;       // the tank's frequency over the note's (the tuning)
    double pan = 0, level = 1;
    double flick = 0;       // the plasma's flicker (low-passed noise)
  };
  struct Note {
    int midi = 0;
    bool down = true;
    double f0 = 0;          // the note (Hz)
    double phi = 0.4;       // the column's share of the arc's voltage (the arc's length)
    double amp = 1;         // the arc's current (the loudness)
    double t = 0;           // seconds since the key
    double travel = 0.001;  // the key's slide-wire's travel (s)
    double rise = 0.005;    // its rheostat's: the arc's current coming up (s)
    double fade = 1;        // the current's run-down after the release (x amp)
    Arc arc[kArcs];
    int quiet = 0;
  };

 private:
  void controlStep() override {}
  float sample() override;
  void tune(Note& n);
  double tuneRatio(double phi, double stepsPerPeriod) const;
  std::vector<Note> notes_;
  std::vector<float> side_, sideOut_;
  // The tuning table: the arc's pitch over the tank's (rho) by the column's share and by the
  // steps in a period.
  std::vector<double> rho_;
  double bend_ = 0, bk_ = 1;
  double length_ = 0.5, chorus_ = 0.5, hiss_ = 0.5, release_ = 0.5;
  double z1_[2][4][2] = {};  // the down-sampling filters' states (mid, side)
  uint32_t rng_ = 4057;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (1.0 / 16777216.0); }
  float last_ = 0;
};

// Velocity is the arc's length and current; the sustain pedal holds the keys; the pitch bend is
// +-2 semitones. Knobs (the MPK's K1-K4, -seq's "knob K X"): K1 the arc's length (the
// brightness), K2 the chorus (the two arcs' spread), K3 the hiss (the arc's flicker), K4 the
// release (the current's run-down).
class SingingArcPlayer : public Performer {
 public:
  explicit SingingArcPlayer(SingingArcModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}
  bool setKnob(int k, double x) override;

 private:
  SingingArcModel& m_;
  bool keyDown_[128] = {}, sounding_[128] = {};
};

}  // namespace pajupilli
