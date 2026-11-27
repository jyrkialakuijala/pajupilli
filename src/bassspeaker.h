// The bass amp's output transformer and its speakers in their box, as a physical system (research/
// bass_amps.md 13): what turns the head's voltage into sound. The fixed filters before this had
// the speaker's measured curve, but not what the curve comes from, and so none of what changes
// with level or with the amp in front of it:
//   - The head as a source: its open-circuit voltage behind its output impedance (a tube head's
//     damping factor of a few: the speaker's own impedance curve shapes what it gets: more at the
//     box's resonance and in the treble, where the coil's inductance raises it; a solid-state
//     head's ~200: none of that).
//   - A tube head's output transformer: its core's flux the voltage's integral, its magnetizing
//     current through the tubes' own (open-loop) resistance; a 3 Hz high-pass while small, and
//     when a loud low note drives the core toward saturation the magnetizing current grows
//     steeply and the low note's peaks are pulled down (a soft, bass-only compression).
//   - Each driver (Thiele-Small: the coil's R and L, the motor's Bl, the cone's mass, the
//     suspension's stiffness and loss) on the box's air as a spring (sealed), or on the box and a
//     port (ported: the port's air a mass on the same spring, tuned to fb). Bl falls as the coil
//     leaves the gap (its rest a little off centre: even harmonics) and the suspension stiffens
//     toward Xmax: the loudest low notes squashed and growling, as a driven 8x10 is.
//   - The drivers' wiring (in series and in parallel) gives the cabinet's impedance, each driver
//     its share of the voltage.
//   - Out: the cone's acceleration (a close mic's pressure, near field or far, at the low end),
//     and in a ported box some of the port's (the mic hears it from farther away). What the cone's
//     break-up and the mic add above a few hundred Hz is the caller's (GuitarAmp's fixed filters).
// It runs at the base rate: its nonlinearities are low-order and below a few hundred Hz.
#pragma once

#include <complex>

namespace pajupilli {

class BassSpeaker {
 public:
  struct Spec {
    // One driver: coil (Ohm, H), Fs (Hz), Qms, Qes, Vas (m^3), Sd (m^2), Xmax (m).
    double re = 5.6, le = 0.7e-3, fs = 35, qms = 5, qes = 0.47, vas = 0.091, sd = 0.035, xmax = 6.2e-3;
    double offset = 0.5e-3;  // the coil's rest off the gap's centre (m)
    double blAtX = 0.7;      // Bl at Xmax, against its rest value
    double kAtX = 1.5;       // the suspension's stiffness at Xmax, against its rest value
    // The box, per driver: its air (m^3), the port's tuning (Hz; 0: sealed), the port's area (m^2),
    // its loss (Q), the share of its sound the mic hears against the cone's.
    double vb = 0.03, fb = 0, sp = 0.01, qp = 10, portHeard = 0.3;
    int series = 1, parallel = 1;  // the drivers' wiring
    // The head: the voltage at the power stage's limit (V peak), its damping factor against the
    // cabinet's nominal impedance (Ohm).
    double volts = 50, damping = 6, nominal = 4;
    // The output transformer (0: none, a solid-state head): its small-signal corner (Hz), the
    // source's open-loop resistance at the secondary (Ohm), the flux where the core saturates (V s).
    double otHz = 0, otR = 8, otSat = 0.25;
  };
  BassSpeaker(double fs, const Spec& s);
  // u: the power stage's output against its limit (1 at the limit); out: the cone's acceleration
  // (m/s^2, the port's heard share added).
  double run(double u);
  // The small-signal response (rest position), from u to the output, at f (Hz).
  std::complex<double> response(double f) const;
  void reset();
  double excursion() const { return xPeak_; }  // (the farthest the cone went, m)

 private:
  static constexpr int kN = 5;  // i, x, v, xp, vp
  void matrix(double x, double A[kN][kN]) const;
  double fs_, h_;
  Spec s_;
  int n_;  // 3 sealed, 5 ported
  double bl0_, mms_, rms_, kms_, kb_, mp_, rp_, rsrc_, xbl_, xk_, lm_ = 0;
  double st_[kN] = {}, uLast_ = 0, phi_ = 0, xPeak_ = 0;
};

}  // namespace pajupilli
