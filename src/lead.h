// Synth leads from analog circuits (research/synth_leads.md). The first: the
// sawtooth lead (GM 82), its core as simulated in ngspice by tools/circuit.py:
//
//   - An expo converter: a transistor's collector current, exponential in the
//     control voltage, I = Iref exp(Vcv / VT), VT = kT/q: ~18 mV per octave at
//     25 C, and the scale drifts with the voice's temperature.
//   - An op-amp integrator: the current charges C, the output ramps up.
//   - Parasitics that move the pitch (measured in SPICE): the expo
//     transistor's emitter resistance (the top flat), leakage at the summing
//     node (the bottom sharp, more so when warm). Stray capacitance, the op-
//     amp's bandwidth and the reset loop's inductance only shift it.
//   - A comparator with hysteresis (a Schmitt trigger): at the upper threshold
//     it closes a MOSFET across C; the integrator's output slews down (the
//     op-amp's slew rate) to the lower threshold, where the switch opens again.
//     The thresholds come from its resistor divider and the comparator's swing.
//
// Real time (48 kHz; the ladder 2x oversampled inside): the ramp is exact between events; a crossing is found between
// samples, the reset's dead time (the slew) is kept, and the jump is band-
// limited with a polyBLEP (two samples, one sample of delay). Against the
// SPICE netlist (tools/circuit.py saw): the same frequency law, the same
// thresholds, the dead time (0.36 us; -2 c at A7).
//
// Then a 4-pole transistor ladder low-pass (Moog 1969; the nonlinear model of
// Huovilainen 2004: each stage a differential pair, tanh at 2 VT), its cutoff
// an exponential current (keyboard, envelope, knob), resonance by feedback;
// an OTA amplifier (CA3080-like: tanh of the input, gain by its bias current);
// envelopes as RC charges (an analog ADSR: attack charges towards 1.3 and
// switches to decay at 1).
//
// Each voice has its own parts: C within 5 %, the divider within 1 %, its own
// temperature (a few tenths of a degree from 25 C, drifting slowly). At power-
// on it tunes itself (as polyphonic analog synths do): it measures its own
// period at every C and trims the control voltage.
//
// The voices share a board (PJ_ANALOG scales all of this; 0: ideal):
//   - Parallel wires: each voice's ramp output runs past the others' summing
//     nodes (~0.5 pF to its neighbours, ~0.1 pF to the rest). A reset's 4.6 V
//     drop pushes a charge into every other integrator (a 2 mV step in its
//     ramp: pulse-coupled oscillators, which pull together near unison, as
//     Mirollo & Strogatz 1990), and the ramps' slopes a small current.
//   - The supply rail (0.3 ohm of regulator and wiring): each sounding voice
//     draws ~5 mA more; the comparator's swing and reference follow the rail
//     (a chord goes a little sharp), with the rectifier's 100 Hz ripple.
//   - The ground (0.5 mohm shared): the outputs' currents lift it under the
//     expo transistors' bases: the voices frequency-modulate each other by
//     a fraction of a cent.
//   - Heat: playing warms the board (~0.6 C per voice at full level, over
//     ~40 s); the expo converters are compensated by a tempco resistor, each
//     within 10 %, so voices drift apart a little after the autotune; the
//     leakage doubles every 10 C.
//   - The parts' own: the expo transistor's 1/f noise (a slow wander of a few
//     hundredths of a cent), the ladder pairs' offsets (~1 mV: even
//     harmonics), the OTA's control feedthrough (a thump on a fast attack)
//     and its bleed when closed (the oscillator never stops: ~-75 dB), mains
//     hum picked up at ~-90 dB.
#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

#include "knob.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

// What lies between the core (the vibrator: the expo converter and the
// ramp) and the ladder (the resonator) in each lead: its sound circuit, and
// how the player drives it. Levels 0 = the part is absent.
struct LeadPatch {
  const char* name;
  // Sources off the core: the ramp; a comparator on it (a pulse; its
  // threshold swept by its own triangle, depth of the width); the ramp
  // rectified into a triangle and shaped by a differential pair (a sine);
  // flip-flops dividing the resets by 2 and by 4 (exact octaves below).
  double saw = 1, pulse = 0, pw = 0.5, pwm = 0, sine = 0, sub = 0, sub2 = 0;
  // A second core: its level, its interval (semitones, the CV's pre-set),
  // and the window of soft sync from the first core's reset (share of the
  // ramp: 0 = free).
  double second = 0, secondSemis = 0, syncWin = 0;
  double detune = 0;  // the two cores this far apart (semitones), split around the note
  // Noise from a reverse-biased junction: breath (band-passed, then a delay-
  // line comb at the note's period: pitched breath), its comb feedback; a
  // chiff (a resonant filter pinged by a burst at each attack).
  double breath = 0, comb = 0, chiff = 0;
  // Shapers: a ring modulator (the ramp times the /2 square), an asymmetric
  // diode overdrive, a mid-scoop notch after it, a sine wavefolder.
  double ring = 0, drive = 0, notch = 0, fold = 0;
  // A formant bank (4 band-passes, "oo" to "ah" with the key's speed), a
  // bucket-brigade chorus after the amplifier, a slow pitch wobble (cents).
  double formants = 0, chorus = 0, wobble = 0;
  // Scat (the formant bank sings syllables): each attack a voiced stop (d or
  // b: a closure, a burst, the formants gliding in from the stop's locus) and
  // a vowel chosen by the key's speed: u, a, i, ei.
  double scat = 0;
  // The player: the amplifier's envelope (attack, decay s; sustain), the
  // cutoff's offset (semitones), the filter envelope's and the resonance's share.
  double ampA = 0.004, ampD = 0.35, ampS = 0.75, cutoff = 0, fltEnv = 1, resScale = 1;
  // ... the release (x the release knob's), the filter envelope's attack, decay
  // and sustain, a pitch envelope (semitones at a full-velocity strike, falling
  // with its time constant, s: a drum's skin).
  double relMul = 1, fltA = 0.006, fltD = 0.25, fltS = 0.25, pitchEnv = 0, pitchTau = 0.05;
};

class Lead : public Tube {
 public:
  explicit Lead(const Instrument& inst);
  ~Lead() override;
  // Controls, per block: the control voltage as a note (fractional MIDI,
  // sounding), cutoff (as a note, fractional), resonance 0..1, the amplifier's
  // gain 0..1 (from the envelope), the gate (envelopes run here).
  struct Controls {
    double note = 69, cutoff = 100, res = 0.3, level = 0;
    double vibDepth = 0, vibRate = 5;  // the LFO's depth (semitones, peak) and nominal rate (Hz)
    int strike = 0;    // counts the attacks (a chiff pings on each)
    double vel = 0.5;  // the attacking key's speed 0..1
  };
  const LeadPatch& patch() const { return *patch_; }
  void setControls(const Controls& c) { ctl_ = c; }
  void seed(unsigned s) override;
  double testResponse() const override { return v_; }
  void resetAir() override;
  // The core's frequency at a control voltage (as a note, no trim), measured
  // by running the core alone for a few periods (the autotune; tests).
  double measure(double note) { return measureC(note, C_); }
  double measureC(double note, double C);
  // The per-note trim from the autotune (cents added to the control voltage).
  double trim(double note) const;
  double temperature() const { return tempC_; }
  // The ladder's Q at a resonance r (feedback 4r around four one-pole
  // stages, linear: the poles at -1 + (4r)^(1/4) e^(+-i pi/4)); infinite at
  // r = 1, where it starts to oscillate. There is no separate Q: in a ladder
  // the resonance is the Q.
  static double ladderQ(double r) {
    if (r >= 1) return std::numeric_limits<double>::infinity();
    const double a = std::pow(4 * std::max(0.0, r), 0.25) * std::sqrt(0.5), re = 1 - a;
    return std::hypot(re, a) / (2 * re);
  }

 private:
  void controlStep() override;
  float sample() override;
  double coreStep(double& out);  // one sample of the core: the band-limited ramp
  void autotune();

  Controls ctl_;
  // The core's parts.
  double C_ = 1e-9, r3_ = 100e3, r4_ = 574.5e3, vref_ = 2.257, vsw_ = 13.32, slew_ = 13e6;
  double vhi_ = 5, vlo_ = 0.3, iref_ = 0;  // thresholds, current at A4 and 25 C
  double tempC_ = 25, tempDrift_ = 0;
  double re_ = 1.5, leak_ = 1e-9;  // the expo transistor's emitter resistance (ohm), leakage (A)
  double coreCurrent(double note, double tempC) const;
  // The board (shared by the voices) and this voice's own imperfections.
  int id_ = -1;
  double kAnalog_ = 1;
  double tcErr_ = 0;      // the expo's tempco compensation error (share of VT's drift left)
  double vos_[5] = {};    // the ladder pairs' offsets (V)
  double ft_ = 0, bleed_ = 0;  // the OTA's control feedthrough (V at full level), closed bleed
  double cvNoise_ = 0;    // 1/f-like wander of the CV (semitones)
  // The filter's own wander (cutoff in semitones, resonance in its units): not
  // a random walk but kicks of Laplace-distributed size at random moments (a
  // few a second), each pulled back by a PID loop, so the setting is right on
  // average (the integral term: no lasting bias) and is never left drifting.
  struct Wander {
    double x = 0, integ = 0;
    // One control block: a kick (0 most of the time), then the PID's pull;
    // the derivative term, taken implicitly, resists fast change.
    void step(double kick, double dt) {
      constexpr double kp = 4, ki = 4, kd = 0.5;  // critically damped, ~0.5 s
      integ += x * dt;
      x += (kick - dt * (kp * x + ki * integ)) / (1 + kd);
    }
  } fcWander_, resWander_;
  double kWander_ = 1;
  // The voice's own vibrato LFO, free-running from power-on: a quadrature
  // oscillator (two integrators in a loop), i.e. a rotating complex number
  // z, turned by its angle each block; an amplitude loop holds |z| at 1. Each
  // note gets its own rate (uniform within 15 % of the setting), and the angle
  // and the depth wander slowly (~10 % and ~20 %, smooth, over seconds: the integrators' leakage
  // and temperature), so the voices' vibratos drift in and out of phase.
  double lfoRe_ = 1, lfoIm_ = 0, lfoRateMul_ = 1, lfoWander_ = 0;
  double lfoN1_ = 0, lfoD1_ = 0, lfoDepthNow_ = 0;  // the speed's and depth's slow noise (unit std)
  double lfoDepthMul_ = 1;  // its depth trimmer (the amount's resistor, within 25 %)
  double lfoStep(double rate, double dt);  // the output, Re z, -1..1
  double wanderKick(double rate, double dt);  // a Laplace kick (unit std over time) or 0
  double vhiNow_ = 5, vloNow_ = 0.3;  // thresholds on the present rail
  double xtalk_ = 0;      // the others' slopes through the wires (dV/dt on this ramp)
  // The sound circuit (LeadPatch): simple_ = the plain sawtooth (lead-saw).
  const LeadPatch* patch_;
  bool simple_ = true;
  double sound(double saw);           // the core's outputs through the patch's circuit, volts around 0
  double post(double y);              // after the ladder (volts): the chiff, the chorus
  double events(double slope, double vStart, double vPre, double d, bool reset);  // pulse, flip-flops
  // Core events this sample: the ramp at the start, before any reset, the reset's share after.
  double vStart_ = 0, vPre_ = 0, resetD_ = -1;
  // Pulse and flip-flops: naive level (+-1), band-limited output, next sample's correction.
  double pw_ = 0.5, pwmPh_ = 0, pulseHi_ = -1, pulseOut_ = 0, pulseNext_ = 0;
  double ff1_ = 1, ff2_ = 1, ff1Out_ = 0, ff1Next_ = 0, ff2Out_ = 0, ff2Next_ = 0;
  // The second core (its own capacitor and trim), soft-synced.
  double C2_ = 1e-9, v2_ = 0.3, dead2_ = 0, prev2_ = 0, slope2_ = 0, out2_ = 0, tune2_[11] = {};
  // Noise: breath band, the comb (pitched breath), the chiff's burst and resonator.
  double brLp_ = 0, brHp_ = 0, chiffEnv_ = 0, chiffA_ = 0, chiffB_ = 0, chiffG_ = 0, combLen_ = 100;
  int lastStrike_ = 0;
  std::vector<float> comb_;
  int combPos_ = 0;
  double wob1_ = 0, wob2_ = 0;
  // Formants: 4 state-variable band-passes (trapezoidal), their g and 1/Q.
  double fmA_[4] = {}, fmB_[4] = {}, fmG_[4] = {}, fmK_[4] = {}, fmGain_[4] = {1.0, 0.7, 0.35, 0.2};
  // Scat: the syllable (vowel, its glide target, the stop: 0 d, 1 b), the
  // time since its strike, the closure's length, the source's gate, the burst.
  int scV_ = 0, scV2_ = 0, scC_ = 0;
  double scT_ = 1, scClose_ = 0.015, srcGain_ = 1, burst_ = 0;
  void scatFormants(double dt, double f0, double* F, double* G);
  // The notch (a biquad, fixed), the chorus's delay line and its two LFOs.
  double nb_[5] = {}, nz_[2] = {};
  std::vector<float> bbd_;
  int bbdPos_ = 0;
  double chPh1_ = 0, chPh2_ = 0.37, bbdLp_ = 0;
  // The core's state.
  double v_ = 0.3, dead_ = 0, blepNext_ = 0, prevOut_ = 0;
  double slope_ = 0;  // dV/dt this block
  // Autotune: cents per C (MIDI 0, 12, ... 120), interpolated.
  double tune_[11] = {};
  // Ladder (4 stages, 2x oversampled), amplifier.
  double y_[4] = {}, w_[4] = {}, y4Prev_ = 0, g_ = 0.1, res_ = 0.3, vt2_ = 0.0514;
  double level_ = 0;
  uint32_t rng_ = 1;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; }
};

class LeadPlayer : public Performer {
 public:
  explicit LeadPlayer(Lead& l);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned s) override;
  // The resonance knob (K3, 0..1) to the ladder's resonance (0..1.05).
  static double resonance(double knob) { return 1.05 * std::pow(std::clamp(knob, 0.0, 1.0), 0.6); }

 private:
  void gate(bool on, bool retrigger);
  Lead& lead_;
  int held_[128] = {};
  int stackTop_ = 0, stack_[16];
  int keyVel_ = 64;
  bool gate_ = false;
  // The control voltage's glide (an RC lag), as a note.
  double cv_ = 69, glideTau_ = 0;
  double tuneNow_ = 0;  // the just-intonation offset applied (cents), slewed
  int strike_ = 0;
  // The filter envelope's reach (semitones at a full-velocity key; ~ velocity^2,
  // so soft keys barely open it): PJ_FLTENV.
  const double kFltEnv_ = knob("PJ_FLTENV", 24);
  double formantNow_ = 0;  // this key's filter offset (semitones), slewed between keys
  // Envelopes: amplifier and filter (RC: attack towards 1.3, decay to sustain, release to 0).
  struct Env {
    double v = 0;
    bool attack = false;
    void step(bool gate, double a, double d, double s, double r, double dt);
  } amp_, flt_;
  double velAmt_ = 0.5, sinceOnClock_ = 0;
  double noteVibDepth_ = 1, noteVibRate_ = 1, noteVibDelay_ = 0.35, noteVibFade_ = 0.6,
         noteVibCurve_ = 1, noteVibStart_ = 1;  // this note's vibrato (a new one each attack)
  uint32_t rng_ = 4242;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; }
};

}  // namespace pajupilli
