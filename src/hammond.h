// The Hammond B-3 (research/hammond_b3.md §13, research/hammond_character.md) through its Leslie
// 122 (src/leslie.h): General MIDI 17 (-inst drawbar-organ), 18 (percussive-organ), 19
// (rock-organ); -inst hammond is the first.
//
//   - The tonewheel generator: all 91 wheels turn all the time, every one geared to one shaft,
//     so their phases are locked (a chord beats the same every time it is played). Each wheel is
//     a table of one revolution of its own pickup's voltage: its teeth (2 .. 192; the 12 gear
//     ratios' famous near-equal temperament), its few harmonics (the bottom octave's "complex"
//     wheels rich in odd ones), the tuned filter of the upper wheels, and its flaws: the wheel's
//     eccentricity (its tone swelling and fading once a revolution: sidebands at the rotation
//     rate, a sub-octave line under the 2-tooth wheels), the teeth's uneven spacing and depth, its
//     own level. The organ's serial number (PJ_HAMMOND_SERIAL) draws them; PJ_HAMMOND_AGE scales
//     them (1: a used organ, sidebands ~-30 dB). The shaft's own wobble (20 Hz) and the motor's
//     pulses (120 Hz) ripple every wheel at once.
//   - Leakage: each wheel's shaft partner faintly with it; the whole generator's whisper behind
//     everything (an organ is never silent).
//   - Three divisions: the upper and lower manuals (61 keys, C2..C7, nine drawbars each with
//     the foldback), the pedals (25 keys, C1..C3, 16' and 8'). A key closes nine contacts onto
//     the busbars one after another, each bouncing: the key click, never the same twice; a hard
//     key closes them within ~0.1 ms, a soft one over ~3 ms, the softest one by one (10-100 ms).
//   - Percussion (upper manual): the 2nd or 3rd harmonic's ping, slow or fast, soft or normal,
//     single-trigger (legato does not re-strike; a short gap re-arms it only part way: softer,
//     shorter); the 1' drawbar silent while on.
//   - The scanner vibrato and chorus (V1-V3, C1-C3), 6.87 Hz.
//   - The preamp and the expression pedal (CC11), the swell feeding the Leslie's amp: louder is
//     dirtier, and more keys and more drawbars dirtier still.
//   - The Leslie: slow / fast by the mod wheel (CC1) or the sustain pedal (CC64).
//   - When a MIDI file sends none of those, the player rides them itself: the expression by the
//     notes' velocities, the Leslie by the music's intensity (fast as a phrase builds, slow again
//     after it), never too often.
#pragma once

#include <array>
#include <string>
#include <vector>

#include "leslie.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool hammondInstrument(const Instrument& inst);  // family "hammond"

struct HammondRegistration {
  std::array<int, 9> upper{8, 8, 8, 0, 0, 0, 0, 0, 0};
  std::array<int, 9> lower{8, 3, 8, 0, 0, 0, 0, 0, 0};
  std::array<int, 2> pedal{8, 3};  // 16', 8'
  bool perc = false, percThird = true, percFast = true, percSoft = false;
  int vibrato = 6;                  // 0 off, 1-3 V1-V3, 4-6 C1-C3
  bool vibUpper = true, vibLower = true;
  bool leslieFast = false;
  bool leslie = true;               // false: the organ's own output, dry (a tone cabinet's line)
  double drive = 1.2;               // the 122's amp (1: clean at mezzo-forte)
  std::string text() const;         // "upper 888000000, lower 838000000, pedal 83, perc 3rd fast, C3, ..."
};
// The General MIDI program's registration (by the instrument's name).
HammondRegistration hammondPreset(const std::string& name);
// The PJ_HAMMOND_* overrides (tests, by hand), over a preset or a plan's choice.
void hammondOverrides(HammondRegistration& r);

class HammondModel : public Tube {
 public:
  explicit HammondModel(const Instrument& inst);
  static constexpr int kWheels = 91;
  enum Division { kUpper = 0, kLower = 1, kPedal = 2 };
  void press(int div, int midi, int velocity);
  void release(int div, int midi);
  void setRegistration(const HammondRegistration& r);
  const HammondRegistration& registration() const { return reg_; }
  void setExpression(double e) { exprTarget_ = e; }  // 0 .. 1 (the pedal)
  void setLeslieFast(bool f) { leslie_.setFast(f); }
  void setLeslieBrake() { leslie_.setBrake(true); }
  bool leslieFast() const { return leslie_.fast(); }
  double hornHz() const { return leslie_.hornHz(); }
  bool asleep() const { return false; }  // (the wheels never stop)
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;
  // The pitch of wheel n (1..91), as geared (Hz).
  static double wheelHz(int n);

 private:
  struct Contact {
    double t[8];    // its toggle times after the key's event (s): close, open, close, ...
    int n = 0, next = 0;
    double gain = 0;
    bool closed = false;
    double brk = 0;  // a dirty contact's momentary break (s left)
  };
  struct Key {
    int div = 0, midi = 0;
    bool down = false;
    double since = 0;
    Contact c[9];
    int wheel[9];   // per drawbar (pedal: [0] 16', [1] 8')
  };
  void controlStep() override {}
  float sample() override;
  void schedule(Key& k, int velocity, bool closing);
  std::vector<std::vector<float>> table_;  // per wheel, one revolution
  int gearOf_[kWheels + 1] = {}, partner_[kWheels + 1] = {};
  double turnsPerSec_[12];
  uint32_t phase_[12] = {}, inc_[12] = {};
  float wheel_[kWheels + 1] = {};
  // Each wheel's torsional flutter on its spring coupling (a resonance of a few Hz excited by the
  // motor's pulses and its bearing): its phase (revolutions) and the resonator's state, at a
  // control rate.
  double flX_[kWheels + 1] = {}, flV_[kWheels + 1] = {}, flW_[kWheels + 1] = {}, flQ_[kWheels + 1] = {}, flG_[kWheels + 1] = {};
  int32_t flOff_[kWheels + 1] = {};
  int ctl_ = 0;
  double mains_ = 0;  // the grid's drift (relative speed)
  // The generator's loading: each wheel's voltage sags a little with the contacts it feeds.
  double load_[kWheels + 1] = {}, loadNext_[kWheels + 1] = {};
  uint32_t flRng_ = 1, dirtRng_ = 12345;
  std::vector<Key> keys_;
  HammondRegistration reg_;
  double gainU_[9] = {}, gainL_[9] = {}, gainP_[2] = {};
  double background_[kWheels + 1] = {};
  // Percussion.
  double percCharge_ = 1, percEnv_ = 0;
  bool upperHeld_ = false;
  // The scanner.
  std::vector<float> line_;
  int linePos_ = 0;
  double scanPh_ = 0, lineLp_ = 0;
  // The preamp and the pedal.
  double expr_ = 0.8, exprTarget_ = 0.8, shelf_ = 0;
  double t_ = 0;
  Leslie leslie_;
  uint32_t rng_ = 1;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

class HammondPlayer : public Performer {
 public:
  explicit HammondPlayer(HammondModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  // A plan's note (src/hammondplan.h): on its division (HammondModel::Division).
  void noteOnManual(int div, int midi, int velocity);
  void noteOffManual(int div, int midi);
  void setRegistration(const HammondRegistration& r) { m_.setRegistration(r); leslieFast_ = r.leslieFast; m_.setLeslieFast(r.leslieFast); }
  void render(float* out, int n) override;
  void setHold(bool h) override;  // the sustain pedal: the Leslie fast while it is down
  // The Leslie set from outside (a MIDI file's CC82: 0 stop, 1 slow, 2 fast; or a plan): the
  // player's own riding of it stops.
  void setLeslie(int state);
  void setLeslieExternal() { leslieSeen_ = true; }
  void seed(unsigned s) override { rng_ = 777u + 7919u * s; }

 private:
  void ride(double dt);
  HammondModel& m_;
  int heldDiv_[128];
  int held_ = 0;
  // The riders: the expression from the velocities, the Leslie from the music's intensity, until
  // the file (or the player) sends its own.
  bool exprSeen_ = false, leslieSeen_ = false;
  double lastBreath_ = 0, lastFlutter_ = 0;
  double velSmooth_ = 90, rate_ = 0, sinceSwitch_ = 1e9, sinceNote_ = 1e9, intensity_ = 0, usual_ = 2;
  bool leslieFast_ = false;
  uint32_t rng_ = 777;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (1.0 / 16777216.0); }
};

}  // namespace pajupilli
