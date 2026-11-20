// The Fender Rhodes (-inst electric-piano-1, General MIDI 5; research/rhodes.md, the model plan
// in its section 12; research/electric_pianos.md section 3).
//
// Per key (A0 .. C8, the 88's harp; the 73's keys are E1 .. E7):
//   - The tine: a cantilever with its tuning spring, held as its modes: the first in two
//     polarisations (the hammer's, and a weak sideways one a fraction of a Hz apart: the slow
//     swirl), the second and third (5.8 .. 9 and 17.6 .. 28 x f0, the swaged tine's ratios by key)
//     dying in tens of ms. Lengths from the factory's cutting sheet, the strike point from the
//     manual's striking line, the fundamental's Q from Shear's measurements, the factory stretch.
//   - The tonebar: one mode at the bar's own frequency (Muenster & Pfeifle: far under the note),
//     lossy, driven by the tine's root and moving the whole tine with it: enslaved at the tine's
//     frequency, its own ring only in the attack.
//   - The hammer: a mass with a neoprene tip (Shore 30 / 50 / 70 / 90 by zone, a wrapped maple
//     core at the top), a Hunt-Crossley contact against the tine's place and speed at the strike
//     point as they are: a key struck again meets a moving tine (a choked or a sharper blow, by the
//     phase), and the bass tine's swing back can strike it twice.
//   - The damper: felt from below, engaged by the key's return (a few ms), its loss by register.
//   - The pickup: the flux through the coil a bell of the tine tip's height over the pole's edge,
//     Phi = [1 + ((x + delta) / w)^2]^-p, w the gap (+ ~1 mm), the sideways motion changing the gap;
//     the EMF its time derivative, analytic, on the full tip motion: soft notes nearly sines, hard
//     ones (the bass first) barking, the fundamental folding back, the bloom as a note decays.
//   - Each key its own: gap, offset, strike point, tuning, Q (seeded; PJ_RHODES_WEAR scales the
//     spread from "factory fresh" to "road-worn").
//
// The electronics (one for the instrument): the harp (the coils in groups of three, 24 groups in
// series: R 1.4 kOhm, L 0.65 H, with the cable's capacitance) into its load; then
//   - a Suitcase (default): the Peterson preamp's input (27 kOhm: the harp's corner near 5 kHz),
//     its tone at the centre, the stereo vibrato (a square-wave oscillator lighting two lamps in
//     antiphase, each lamp's filament heating and cooling, each CdS cell following its light fast
//     and letting go slowly, shunting its channel), the power amplifier on its unregulated supply
//     (sagging under bass chords, clipping), and the 4 x 12 in cabinet; or
//   - a Stage (PJ_RHODES_SUITCASE=0): the harp into its 10 kOhm volume pot, straight out (DI).
// A trace of mains hum through the harp's groups (PJ_RHODES_HUM).
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool rhodes(const Instrument& inst);  // -inst electric-piano-1

class RhodesModel : public Tube {
 public:
  static constexpr int kLowest = 21, kKeys = 88;  // A0 .. C8
  explicit RhodesModel(const Instrument& inst);
  // The hammer's speed at the tip (m/s).
  void strike(int key, double speed);
  void damp(int key, bool on);  // the damper on (key up, no pedal) or off
  void setVibrato(double rateHz, double depth) { vibRate_ = rateHz, vibDepth_ = depth; }
  bool asleep() const { return awake_ == 0; }
  const float* takeSide(int n) override;
  double testResponse() const override { return last_; }
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  struct Mode {      // displacement at the tip = re; velocity at the tip = -w im
    double re = 0, im = 0;
    double c = 1, s = 0, d = 1;      // the step's rotation and decay
    double cs = 1, ss = 0, ds = 1;   // a substep's (the hammer's contact)
    double w = 0, phiS = 0, mass = 0;  // rad/s; the shape at the strike point (tip = 1); modal mass (kg)
  };
  struct Key {
    int nm = 0;                 // modes kept (1 .. 3), vertical
    Mode m[3], side;            // side: mode 1's sideways polarisation
    Mode bar;                   // the tonebar's mode (its displacement moves the tine)
    double barDrive = 0;        // the tine root's force into the bar per m of tip displacement
    double sideShare = 0;       // the blow's share sideways
    double gap = 1e-3, w = 2e-3, delta = 1e-3, p = 1, sens = 1;
    double damp = 0, dampOn = 0;  // the damper's loss (1/s) when on; its engagement 0..1 now
    double dampTarget = 1;
    // the hammer
    bool hammer = false;
    double xh = 0, vh = 0, k = 0, alpha = 2.8, mu = 0.6, M = 0.008;
    int hammerT = 0;
    double contact = 0;  // (the blow's contact time, s)
    double action = 1;   // the key's lever: hammer speed per key speed
    bool awake = false;
    int quiet = 0;
  };
  void rotate(Mode& m, bool sub);
  std::vector<Key> keys_;
  int awake_ = 0;
  double dt_;
  // electronics
  bool suitcase_ = true;
  double harpB_[3] = {}, harpA_[3] = {}, hx_[2] = {}, hy_[2] = {};
  double hum_ = 0, humPh_ = 0, humEps_[4] = {};
  double vibRate_ = 4.5, vibDepth_ = 0.5, vibPh_ = 0;
  double lampT_[2] = {}, cell_[2] = {};
  double rail_ = 1, pre_ = 0, preHp_ = 0, preHpX_ = 0, treb_ = 0;
  struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    double run(double x) {
      const double y = b0 * x + z1;
      z1 = b1 * x - a1 * y + z2;
      z2 = b2 * x - a2 * y;
      return y;
    }
  };
  Biquad cab_[2][3];
  std::vector<float> side_, sideOut_;
  float last_ = 0;
};

// Velocity is the hammer's speed; the sustain pedal lifts the dampers. The vibrato: K3 its speed,
// K4 its intensity (live); the breath / mod wheel deepens it.
class RhodesPlayer : public Performer {
 public:
  explicit RhodesPlayer(RhodesModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}
  bool setKnob(int k, double x) override;

 private:
  RhodesModel& m_;
  bool keyDown_[128] = {};
  double rate_ = -1, depth_ = -1;
};

}  // namespace pajupilli
