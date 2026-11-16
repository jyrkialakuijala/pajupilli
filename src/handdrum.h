// The hand drums (-inst conga, bongo, timbale; research/hand_drums.md):
// General MIDI's 60 .. 66.
//
//   - One head (membrane.h) over an open-bottomed shell: the shell is a pipe
//     closed by the head and open at its foot. Its quarter-wave modes are
//     driven by the volume the head sweeps and press back on it; the foot
//     radiates what the pipe does not take back. A conga's 76 cm barrel puts
//     its first one near 100 Hz (the bass tone); a bongo's short shell near
//     its own note.
//   - The hand: a soft, heavy, lossy mallet (flesh on bone, Hunt-Crossley),
//     thrown from where the head is now, so a stroke meets the ring. An open
//     tone bounces off; a closed (mute) tone stays pressed a moment and damps
//     the head where it lies.
//   - The timbale: a thin plastic head, a stick, and its steel shell, a few
//     high ring modes that every stroke near the edge sets going.
//   - The sound: a microphone above the head (each mode by its shape, summed
//     over the head) and the pipe's foot below, as a point source, later.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "membrane.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool handDrum(const Instrument& inst);  // -inst conga, bongo, timbale

struct HandDrumSpec {
  HeadSpec head;
  double pipeL = 0;        // the shell's length (m); 0: no pipe
  double footA = 0.1;      // the foot's radius (m): the end correction 0.6 a
  double pipeQ = 12;       // its modes' Q (wall and radiation losses)
  int pipeModes = 3;
  // The striker: a hand (fingers) or a stick.
  double mass = 0.03;      // kg
  double K = 1e6;          // N/m^alpha
  double alpha = 1.5;
  double loss = 2.0;       // Hunt-Crossley, s/m
  double where = 0.85;     // the stroke's place, a share of the radius
  double band = 0.035;     // the fingers' half width across the rim (m); 0: a point
  double returnHz = 12;    // the arm taking the hand (or the stick) back up
  double muteForce = 5;    // N: a closed tone's fingers left on the head
  double muteTime = 0.2;   // s
  double muteDamp = 150;   // N s/m: the flesh lying there
  double vmax = 6;         // m/s at velocity 127
  // The steel shell (timbale): its ring modes, set going through the rim.
  int shellN = 0;
  double shellF[4] = {}, shellT60[4] = {};
  double shellMass = 0.4;  // kg (modal)
  double shellRad = 0.01;  // its radiating area (m^2)
  double rimShare = 0.0;   // of the stroke's force into the rim at the edge
  double micZ = 0.5, micX = 0.2;  // the microphone over the head (m)
  double click = 1;        // the contact's own sound above the head's top
  double gain = 1;
};

class HandDrum {
 public:
  HandDrum(double fs, const HandDrumSpec& spec);
  // A stroke: the striker's speed (m/s) at r (share of the radius), angle
  // theta; mute: the fingers stay a moment.
  void strike(double speed, double r, double theta, bool mute);
  // A hand's stroke as played this time: the striker's mass, its stiffness
  // and the fingers' width (a flat open hand, cupped closed fingers, the
  // palm), how quickly the arm lifts it (the rebound), and, after the blow,
  // fingers left on the head (where, how hard, how long, how much they damp;
  // a light touch at the edge, or a closed tone's press). Factors of the
  // spec's; mute force 0: none.
  struct Gesture {
    double mass = 1, K = 1, band = 1, lift = 1;
    double muteForce = 0, muteTime = 0, muteDamp = 1, muteR = -1;
  };
  void strike(double speed, double r, double theta, const Gesture& g);
  float sample();
  void reset();
  bool asleep() const { return asleep_; }
  const HandDrumSpec& spec() const { return s_; }
  Membrane& head() { return head_; }

 private:
  void place(double r, double theta, double band = -1);
  void shapeAt(double r, double theta, double band, std::vector<float>& phi, double& g);
  HandDrumSpec s_;
  double fs_, dt_;
  Membrane head_;
  std::vector<float> phi_;
  // The striker.
  double zb_ = 1, vb_ = 0;
  bool active_ = false, hit_ = false, mute_ = false;
  double muteLeft_ = 0, hitAt_ = 0, gPhi_ = 0;
  // This stroke's hand (from its Gesture): mass, stiffness, the arm's lift,
  // the fingers left on the head and where they lie (their own shape).
  double mass_ = 0.03, K_ = 1e6, lift_ = 12, muteForce_ = 0, muteDamp_ = 0;
  std::vector<float> phiMute_;
  double gMute_ = 0;
  // The pipe: modal pressures, their speeds; the swept volume's history.
  int np_ = 0;
  double pw_[4] = {}, pg_[4] = {}, pp_[4] = {}, pv_[4] = {}, pmean_[4] = {};
  double drive_ = 0, pbar1_ = 0;
  // The shell.
  double sx_[4] = {}, sv_[4] = {}, sw_[4] = {}, sg_[4] = {};
  // The microphone.
  std::vector<float> mic_;
  float prev_ = 0, hpC_ = 0, lastC_ = 0;
  double footPrev_ = 0, shellPrev_ = 0;
  std::vector<float> footDelay_;
  int footPos_ = 0;
  double footK_ = 0, shellK_ = 0, clickK_ = 0;
  bool asleep_ = true;
  int quiet_ = 0;
};

class HandDrumModel : public Tube {
 public:
  explicit HandDrumModel(const Instrument& inst);
  HandDrum& drum(int i) { return *drums_[i]; }
  int kind() const { return kind_; }  // 0 conga, 1 bongo, 2 timbale
  double testResponse() const override { return last_; }
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  std::vector<std::unique_ptr<HandDrum>> drums_;  // [0] the high, [1] the low
  int kind_ = 0;
  float last_ = 0;
};

// The player. Keys: General MIDI's 60 Hi Bongo, 61 Low Bongo, 62 Mute Hi
// Conga, 63 Open Hi Conga, 64 Low Conga, 65 High Timbale, 66 Low Timbale;
// other keys alternate high and low. Velocity is the hand's (the stick's)
// speed; each stroke lands a little elsewhere.
class HandDrumPlayer : public Performer {
 public:
  explicit HandDrumPlayer(HandDrumModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int) override {}
  void render(float* out, int n) override;
  void setHold(bool h) override { hold_ = h; }
  void seed(unsigned s) override { rng_ = 6007u + 7919u * s; }

 private:
  double ur();
  HandDrumModel& m_;
  uint32_t rng_ = 6007u;
};

}  // namespace pajupilli
