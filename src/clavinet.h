// The Hohner Clavinet (-inst clavinet: the D6, GM 8; clavinet-e7; clavinet-c;
// research/clavinet.md).
//
// 60 strings, F1 .. E6, one a key. A key's rubber tangent strikes its string
// against an anvil and holds it there: the string's end moves by h (1 mm) in
// h / v (the key's speed) and stays; the part beyond the anvil, wound with
// yarn, is silent. The string is held as its modes (the speaking length, from
// the tailpiece to the anvil); the moving end drives mode n by -2/(n pi) of
// its acceleration: below 1/(pi tau) every key speed gives the same partials
// (a sawtooth), above it a faster key is brighter. The tip's stop on the
// anvil knocks the frame (a few resonances, 180 .. 1150 Hz), and the frame
// moves the sounding strings' ends: the knock, heard through their modes.
// Below E4 a string also swings sideways, tuned a little apart and coupled to
// the up-and-down swing at the rubber tip: the two trade energy and the note
// beats (0.5 .. 2 Hz). Pressing a held key harder stretches its string (a few
// cents up). Let go, the tangent frees the whole string (1.19 x longer: three
// semitones down): its vibration and the released bend (a pluck at the anvil)
// ring on it, the yarn damping them: the release thunk.
//
// Two electromagnetic pickups read each string's velocity where they stand
// (the bridge pickup 4 cm from the tailpiece, bright; the centre one 18.5 ..
// 6.5 cm, warm), with a little of the magnet's nonlinearity; the selector
// takes one, both in phase, or both out of phase (thin). Then the model's
// electronics:
//   - the D6: the 1:15 transformer, stage 1, the tone switches (Soft, Medium,
//     Treble, Brilliant: four elements in series, each shorted until
//     pressed: tools/clavtone.py), stage 2's soft clip;
//   - the E7: the same network from a lower source (12 k), drier, stage 2's
//     gain lower;
//   - the C: no transformer and no tone switches: two germanium stages
//     (an uneven clip) and a roll-off near 14 kHz.
// After it, the pedals it was played through: a wah (a resonant band-pass
// swept 450 Hz .. 1.6 kHz) and an envelope filter (Mu-Tron style: the
// band-pass follows the playing's loudness).
#pragma once

#include <vector>

#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool clavinet(const Instrument& inst);  // -inst clavinet, clavinet-e7, clavinet-c

class ClavinetModel : public Tube {
 public:
  static constexpr int kKeys = 60, kLowest = 29;  // F1 .. E6
  enum class Model { kD6, kE7, kC };
  explicit ClavinetModel(const Instrument& inst);
  // The tangent: key speed (m/s at the tip).
  void strike(int key, double speed);
  void release(int key);
  bool asleep() const;
  Model model() const { return model_; }
  // Pickup selector: 0 centre (rhythm), 1 both in phase, 2 bridge, 3 both out
  // of phase. Tone: bits 1 Soft, 2 Medium, 4 Treble, 8 Brilliant (0: silent;
  // the C has none).
  void setPickup(int p) { pickup_ = p; }
  void setTone(int bits);
  void setMute(double m) { mute_ = m; }       // the mute slider, 0 .. 1
  void setPressure(double cents);             // held keys pressed harder: their strings' stretch
  void setWah(double pedal) { wah_ = pedal; } // 0 .. 1 heel to toe; < 0: out
  void setEnvelope(double s) { envAmt_ = s; } // the envelope filter's sensitivity, 0 (out) .. 1
  double testResponse() const override { return last_; }
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  struct Bank {  // a string's modes: complex amplitude (displacement = re, velocity = -w im)
    int n = 0;
    std::vector<float> re, im, pr, pi, w, wc, wb, drive;
    std::vector<double> sig;  // the decays (1/s)
    bool on = false;
  };
  struct Side {  // the sideways swing, coupled at the tip (a share of the keys below E4)
    std::vector<float> re, im, pr, pi;
    float g = 0, tilt = 0;  // coupling x dt; the blow's share sideways
  };
  struct Str {
    double f0, B, L, Lt, dC, dB = 0.04;
    Bank s, j;                 // speaking (key down), joined (key up)
    Side side;                 // (empty: none)
    double detune = 0;         // the sideways swing's tuning (ratio - 1)
    std::vector<float> P;      // [j.n][s.n]: the speaking modes on the joined string
    std::vector<float> pluck;  // [j.n]: the released bend (per m of h)
    std::vector<float> yarn;   // per joined mode: the yarn's decay (1/s)
    bool down = false, knocked = false;
    double t = 0, tau = 1e-3, h = 1e-3, speed = 1;  // the tangent's travel
    double stretch = 1;        // the pressing's tuning (ratio) now on the speaking modes
    int quiet = 0;
  };
  void turns(Bank& b, double ratio);
  void sideTurns(Str& s, double ratio);
  std::vector<Str> str_;
  Model model_;
  int pickup_ = 1, tone_ = 6, toneOld_ = 6, fade_ = 0;
  double mute_ = 0, muteSet_ = -1, press_ = 0;
  // The knock: the frame's resonances.
  static constexpr int kKnock = 5;
  double kre_[kKnock] = {}, kim_[kKnock] = {}, kpr_[kKnock], kpi_[kKnock], kw_[kKnock];
  // The electronics' and pedals' states.
  double hpX_ = 0, hpY_ = 0, lp1_ = 0, lp2_ = 0, lp3_ = 0;
  double sos_[2][8][2] = {};
  double wah_ = -1, envAmt_ = 0, env_ = 0, fxMix_ = 0, ic1_ = 0, ic2_ = 0, fc_ = 800;
  float last_ = 0;
  double dt_;
};

// The player: velocity is the key's speed; the brightness knob (K2) walks the
// tone switches from Soft to Brilliant, the air knob (K3) the pickup
// selector, hardness the keys' speed (a harder touch). The breath (pad
// pressure, joystick up) presses the held keys harder; the joystick across
// rocks the wah (centred: out); K4 above its usual is the envelope filter.
// No sustain pedal on a Clavinet: the hold keeps the tangents down.
class ClavinetPlayer : public Performer {
 public:
  explicit ClavinetPlayer(ClavinetModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}

 private:
  ClavinetModel& m_;
  bool keyDown_[ClavinetModel::kKeys] = {};
};

}  // namespace pajupilli
