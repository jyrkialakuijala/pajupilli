// The tonewheel organ (research/tonewheel_organ.md §13, research/tonewheel_character.md) through its Rotary
// 122 (src/rotary.h): General MIDI 17 (-inst drawbar-organ), 18 (percussive-organ), 19
// (rock-organ); -inst tonewheel is the first.
//
//   - The tonewheel generator: all 91 wheels turn all the time, every one geared to one shaft,
//     so their phases are locked (a chord beats the same every time it is played). Each wheel is
//     a table of one revolution of its own pickup's voltage: its teeth (2 .. 192; the 12 gear
//     ratios' famous near-equal temperament), its few harmonics (the bottom octave's "complex"
//     wheels rich in odd ones), the tuned filter of the upper wheels, and its flaws: the wheel's
//     eccentricity (its tone swelling and fading once a revolution: sidebands at the rotation
//     rate, a sub-octave line under the 2-tooth wheels), the teeth's uneven spacing and depth, its
//     own level. The organ's serial number (PJ_TONEWHEEL_SERIAL) draws them; PJ_TONEWHEEL_AGE scales
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
//   - The preamp and the expression pedal (CC11), the swell feeding the Rotary's amp: louder is
//     dirtier, and more keys and more drawbars dirtier still.
//   - The Rotary: slow / fast by the mod wheel (CC1) or the sustain pedal (CC64).
//   - When a MIDI file sends none of those, the player rides them itself: the expression by the
//     notes' velocities, the Rotary by the music's intensity (fast as a phrase builds, slow again
//     after it), never too often.
#pragma once

#include <array>
#include <cstdlib>
#include <string>
#include <vector>

#include "rotary.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool tonewheelInstrument(const Instrument& inst);  // family "tonewheel"

struct TonewheelRegistration {
  std::array<int, 9> upper{8, 8, 8, 0, 0, 0, 0, 0, 0};
  std::array<int, 9> lower{8, 3, 8, 0, 0, 0, 0, 0, 0};
  std::array<int, 2> pedal{8, 3};  // 16', 8'
  bool perc = false, percThird = true, percFast = true, percSoft = false;
  int vibrato = 6;                  // 0 off, 1-3 V1-V3, 4-6 C1-C3
  bool vibUpper = true, vibLower = true;
  bool rotaryFast = false;
  bool rotary = true;               // false: the organ's own output, dry (a tone cabinet's line)
  double drive = 1.2;               // the 122's amp (1: clean at mezzo-forte)
  std::string text() const;         // "upper 888000000, lower 838000000, pedal 83, perc 3rd fast, C3, ..."
};
// The lower manual for an upper (the accompaniment: softer, darker, of the same family).
// extraSofter: steps more (a file whose accompaniment is written quieter than its tune).
std::array<double, 9> tonewheelLowerFor(const std::array<double, 9>& upper, double extraSofter = 0, double clarity = 0);
// The lower manual on a shape at low's level: kind 1 the percussion's stand-in (008880080: the
// percussion on, the left hand comping above the pedals' bass); 2 the hollow 8' (008060400: a line
// of another colour from the upper's).
std::array<double, 9> tonewheelLowerShaped(const std::array<double, 9>& low, int kind);
// The pedals' two drawbars for an upper (16' by its loudness, 8' by its brightness).
std::array<double, 2> tonewheelPedalFor(const std::array<double, 9>& upper, double clarity = 0);
// The General MIDI program's registration (by the instrument's name).
TonewheelRegistration tonewheelPreset(const std::string& name);
// The PJ_TONEWHEEL_* overrides (tests, by hand), over a preset or a plan's choice.
void tonewheelOverrides(TonewheelRegistration& r);

class TonewheelModel : public Tube {
 public:
  explicit TonewheelModel(const Instrument& inst);
  static constexpr int kWheels = 91;
  enum Division { kUpper = 0, kLower = 1, kPedal = 2 };
  void press(int div, int midi, int velocity);
  void release(int div, int midi);
  void setRegistration(const TonewheelRegistration& r);
  // The upper drawbars' goals, between the steps too (the knobs); the sliders glide there.
  void setUpperGoal(const std::array<double, 9>& bars);
  void setLowerGoal(const std::array<double, 9>& bars);
  void setPedalGoal(const std::array<double, 2>& bars);
  void placeDrawbars();  // (the sliders at their goals at once: a registration set before playing)
  void snap(int div);    // (one division's sliders at their goals at once: a preset key)
  // The run switch: off, the generator coasts down (the pitch sinking, the sound fading with it).
  void setRun(bool on) { run_ = on; }
  double speed() const { return speed_; }
  double drawbar(int div, int d) const { return pos_[div][d]; }  // (where a slider stands now)
  const TonewheelRegistration& registration() const { return reg_; }
  void setExpression(double e) { exprTarget_ = e; }  // 0 .. 1 (the pedal)
  void setRotaryFast(bool f) { for (Cab& c : cab_) c.les.setFast(f); }
  void setRotaryBrake() { for (Cab& c : cab_) c.les.setBrake(true); }
  void setRotaryBoost(double b) { for (Cab& c : cab_) c.les.setBoost(b); }
  void setThin(double t) { thin_ = t; }  // (a trill: the manuals toward their 8' and 4' alone, 0..1)
  bool rotaryFast() const { return cab_[0].les.fast(); }
  double hornHz() const { return cab_[0].les.hornHz(); }
  bool asleep() const { return false; }  // (the wheels never stop)
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  // In a room (the default; PJ_ROTARY_MICS=close for the close microphones instead): the Rotary
  // brings its own early sound to the room's microphones (its rays round the cabinet: the front
  // directly, the side walls, the walls behind), and the room's late field is fed what it sprays
  // all round (the rays' mean: the swirl spread over every direction, its level steady).
  bool ownEarly() const override { return roomMics_; }
  void takeEarly(int n, float* l, float* r, float g) override;
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
    double since = 0, rampClose = 1.5e-4;  // (the contacts' resistance falling as they close, s: the key's speed)
    int velocity = 64;
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
  int shaftOf_[kWheels + 1] = {};          // (each wheel's assembly: the lower wheel on its shaft)
  uint32_t shaftOff_[kWheels + 1] = {};    // (each shaft's angle when the motor started)
  int ctl_ = 0;
  double mains_ = 0;  // the grid's drift (relative speed)
  // The generator's loading: each wheel's voltage sags a little with the contacts it feeds.
  double load_[kWheels + 1] = {}, loadNext_[kWheels + 1] = {};
  uint32_t flRng_ = 1, dirtRng_ = 12345;
  std::vector<Key> keys_;
  TonewheelRegistration reg_;
  double gainU_[9] = {}, gainL_[9] = {}, gainP_[2] = {};
  // The drawbars as sliders: where each is going, where it is, its control's integral.
  double goal_[3][9] = {}, pos_[3][9] = {}, integ_[3][9] = {};
  double mud_ = 0;  // (the low clusters held now, 0..1: the player easing the 16' and 5 1/3' back)
  double mudLevel() const;
  // (the lower manual crowding the pedals, 0..1, smoothed over seconds: its own 16' and 5 1/3'
  // eased back a little, gradually)
  double crowd_ = 0, crowdT_ = 0, thin_ = 0, thinNow_ = 0;
  // (the lower manual carrying the bass, the pedals resting: its low presses a second (decaying),
  // when the pedals last played, and how far its overtones are drawn out for definition, 0..1)
  double lowRate_ = 0, lastPedal_ = -1e9, bass_ = 0;
  double crowdLevel() const;
  double upCrowd_ = 0, upCrowdT_ = 0;  // (the upper manual low over the others, the same way)
  double upperCrowdLevel() const;
  bool placed_ = false;
  void drawbarGains();
  void moveDrawbars(double dt);
  double background_[kWheels + 1] = {};
  // Percussion.
  double percCharge_ = 1, percEnv_ = 0, percTouch_ = 1;
  bool upperHeld_ = false;
  // The scanner.
  std::vector<float> line_;
  int linePos_ = 0;
  double scanPh_ = 0, lineLp_ = 0, lineLpL_ = 0;
  std::vector<float> lineL_;  // (the lower manual's own line under the scanner)
  // (each Rotary's channel of the console's amplification: the expression's bass shelf, the tone,
  // the valve's bias)
  struct Chan { double shelf = 0, tone = 0, dc = 0; };
  Chan chan_[4];
  // The preamp and the pedal.
  double expr_ = 0.8, exprTarget_ = 0.8, shelf_ = 0, tone_ = 0;
  double valveDc_ = 0, microZ_[4] = {};  // (the valve stage's bias drift; the microphonics' resonance)
  std::vector<float> microRing_;
  int microPos_ = 0;
  double t_ = 0;
  bool run_ = true;
  double speed_ = 1;  // (the generator's speed, relative)
  // The Rotary cabinets (four by default, PJ_ROTARY_COUNT): each its own parts (~7 % off nominal),
  // its own place and facing in the room, its own paths from its rays to the listener's pair.
  struct Cab {
    Rotary les;
    double x = 0, y = 0, rot = 0;            // (its place, m: x toward the listener; its facing, rad)
    std::vector<float> ring[Rotary::kRays];  // (its rays, delayed to the paths)
    // Per path (direct, left wall, right wall, back wall, back-left, back-right corners, the floor)
    // and microphone: the delay (samples), and the two rays leaving toward it with their gains.
    int dly[7][2] = {}, ray[7][2][2] = {};
    double g[7][2][2] = {};
    double mix[3] = {1, 1, 1};  // (the upper's, the lower's and the pedals' gains into it)
  };
  std::vector<Cab> cab_;
  void placeCabinets();
  uint32_t rng_ = 1;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
  bool roomMics_ = true, inRoom_ = false;
  int rayPos_ = 0;
  std::vector<float> earlyL_, earlyR_;
};

class TonewheelPlayer : public Performer {
 public:
  explicit TonewheelPlayer(TonewheelModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  // A plan's note (src/tonewheelplan.h): on its division (TonewheelModel::Division).
  void noteOnManual(int div, int midi, int velocity);
  void noteOffManual(int div, int midi);
  void setRegistration(const TonewheelRegistration& r) { m_.setRegistration(r); rotaryFast_ = r.rotaryFast; m_.setRotaryFast(r.rotaryFast); }
  void render(float* out, int n) override;
  void setHold(bool h) override;  // the sustain pedal: the Rotary fast while it is down
  // The Rotary set from outside (a MIDI file's CC82: 0 stop, 1 slow, 2 fast; or a plan): the
  // player's own riding of it stops.
  void setRotary(int state);
  void setRotaryExternal() { rotarySeen_ = true; }
  // The lower manual's vibrato tab (a plan's switch).
  void setLowerVibrato(bool on) {
    TonewheelRegistration r = m_.registration();
    r.vibLower = on;
    r.rotaryFast = rotaryFast_;
    m_.setRegistration(r);
  }
  // A plan's nudge of the slow Rotary on a long note (the slow speed x b; 1 back to it).
  void setRotaryBoost(double b) { m_.setRotaryBoost(b); }
  void setThin(double t) { m_.setThin(t); }
  // (live) K5: the overdrive (the 122's amp driven 1 x .. 8 x); K6: the Rotary (stop / slow /
  // fast, as the half-moon switch). K1-K4: the upper
  // manual's drawbars along the tradition's own paths: K1 soft to hard through the classic
  // registrations (each a detent: the whisper of the 8' alone, the ballad 838, the jazz 888, the
  // gospel 8888, full organ; the 122 driven harder at the end), K2 hollow (the odd harmonics) to
  // full (a chorus), K3 dark to bright (the whistle), K4 light to deep (the 16'); the middles of
  // K2-K4 leave K1's registration as it is.
  bool setKnob(int k, double x) override;
  // The drawbars of a point of the knobs (k 0..1 each); K1's station's name when exactly on it.
  static std::array<double, 9> drawbarsAt(const double k[4], const char** classic);
  // A plan's re-registration: the upper's drawbars (the sliders glide there); the lower manual and
  // pedals follow (lowerExtra: the lower's extra softness, steps).
  // instant: by a preset key (the B), not the drawbars' glide: the upper manual's sound changes at
  // once (the lower manual and pedals still glide after it).
  // lowerShape: the lower manual on a shape of its own (tonewheelLowerShaped; 0: derived).
  void setUpper(const std::array<int, 9>& upper, double lowerExtra, double clarity = 0, bool instant = false, int lowerShape = 0) {
    TonewheelRegistration r = m_.registration();
    std::array<double, 9> up{};
    for (int d = 0; d < 9; ++d) { up[d] = upper[d]; r.upper[d] = upper[d]; }
    std::array<double, 9> low = tonewheelLowerFor(up, lowerExtra, clarity);
    if (lowerShape) low = tonewheelLowerShaped(low, lowerShape);
    std::array<double, 2> ped = tonewheelPedalFor(up, clarity);
    // (set by hand, PJ_TONEWHEEL_LOWER / _PEDAL: kept through the re-registrations)
    if (std::getenv("PJ_TONEWHEEL_LOWER"))
      for (int d = 0; d < 9; ++d) low[d] = r.lower[d];
    if (std::getenv("PJ_TONEWHEEL_PEDAL")) ped = {double(r.pedal[0]), double(r.pedal[1])};
    r.rotaryFast = rotaryFast_;
    m_.setRegistration(r);
    m_.setUpperGoal(up);
    m_.setLowerGoal(low);
    m_.setPedalGoal(ped);
    if (instant) m_.snap(TonewheelModel::kUpper);
  }
  // A plan's shape of the expression pedal over its own riding (x it: 1 as ridden; the final
  // chord's swell or closing).
  void setExprShape(double f) { exprShape_ = f; }
  void setRun(bool on) { m_.setRun(on); }
  // The harmonic percussion's tab (a plan's switch during the piece).
  void setPercussion(bool on) {
    TonewheelRegistration r = m_.registration();
    r.perc = on;
    r.rotaryFast = rotaryFast_;  // (the Rotary stays as it is turning)
    m_.setRegistration(r);
  }
  void seed(unsigned s) override { rng_ = 777u + 7919u * s; }

 private:
  void ride(double dt);
  TonewheelModel& m_;
  int heldDiv_[128];
  int held_ = 0;
  // The riders: the expression from the velocities, the Rotary from the music's intensity, until
  // the file (or the player) sends its own.
  bool exprSeen_ = false, rotarySeen_ = false;
  double lastBreath_ = 0, lastFlutter_ = 0;
  int lastRotaryKnob_ = -1;
  bool laterDone_ = false;
  double baseDrive_ = 1;
  double overdrive_ = 1;     // (live) K5's push of the 122's amp
  int lastOverdrive_ = 10;
  double knobs_[4] = {-1, -1, -1, -1};  // (the drawbar knobs' last positions; -1: not yet moved)
  double printed_[9] = {-9, -9, -9, -9, -9, -9, -9, -9, -9};
  const char* lastName_ = "";
  double exprShape_ = 1;
  double velSmooth_ = 90, rate_ = 0, sinceSwitch_ = 1e9, sinceNote_ = 1e9, intensity_ = 0, usual_ = 2;
  bool rotaryFast_ = false;
  uint32_t rng_ = 777;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (1.0 / 16777216.0); }
};

}  // namespace pajupilli
