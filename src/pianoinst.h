// The concert grand as an instrument of the program (-inst grand-piano): the
// model (piano.h) behind the Tube and Performer interfaces, as the double
// bass is (bassinst.h). One voice is the whole piano: its player takes every
// key (the ensemble's pool is one voice).
#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "performer.h"
#include "piano.h"
#include "tube.h"

namespace pajupilli {

// The grand's tuning (-tuning grand-piano;NAME): grand-piano-stretch (default, Precision Strobe's table),
// stretch-variable, railsback-4-2 (a piano-modelling forum study's curves), octaves-by-register, partials
// (the model's earlier 2:1 / 6:3), tet12 (no stretch), or meantone, kirnberger3, neidhardt, lehman
// (over the partials' stretch). False: no such tuning.
bool setPianoTuning(const std::string& name);
const char* pianoTuningName();
// -hammer_pos X: where the grand's hammers strike, a share of the string's length from its far end
// from the bridge (default 0.125).
void setPianoHammerPos(double share);

bool grandPiano(const Instrument& inst);  // -inst grand-piano, upright-piano: a PianoModel (among the string instruments)

class PianoModel : public Tube {
 public:
  explicit PianoModel(const Instrument& inst);
  Piano& piano() { return piano_; }
  double testResponse() const override { return last_; }
  // The last n samples' side signal (left = out + side, right = out - side:
  // the microphones over the bass and the treble strings); taken once.
  const float* takeSide(int n) override {
    sideOut_.assign(n, 0.0f);
    const int have = int(side_.size()), m = std::min(n, have);
    std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
    side_.clear();
    return sideOut_.data();
  }
  void resetAir() override;
  // With an orchestra (-play): no microphones on it, the board's regions heard through the hall.
  // (The hammers and the action speak above the soundboard, which stands between them and much of
  // the hall: only part of their direct sound reaches it, PJ_PIANO_HALLACT, 0.5; the close
  // microphones, over the strings, take it whole.)
  void concertHall(bool on);
  float actionShare() const { return hallAct_; }
  bool inConcertHall() const { return static_cast<const Piano&>(piano_).hallSound(); }
  // The grand's board as regions at their places (Piano::keepRegions; across: the bass at the
  // listener's left, as the player sees it; along the strings: the tail away), the rest of its
  // sound at the middle: the last part. PJ_PIANO_PARTS=0: one point.
  int parts() const override { return partPlace_.empty() ? 0 : int(partPlace_.size()) / 3; }
  const float* partPlaces() const override { return partPlace_.empty() ? nullptr : partPlace_.data(); }
  const float* takePart(int n, int j) override;
  // The action's own knock at a stroke, heard through the air (the key's front on its felt
  // punching, the hammer's shank, the wood of the action): a few wooden resonances (~250 Hz ..
  // 2 kHz, gone in 10-25 ms), ~ the hammer's speed^1.5, from the quarter of the keyboard the key
  // is in (the front region of the board over it, for the room). Under the strings in the middle
  // of the keyboard; in the top octave much of the note: its thunk. (PJ_PIANO_ACTION: how loud.)
  void actionKnock(int key, double hammerSpeed);

 private:
  static constexpr int kActBands = 4, kActModes = 4;
  float actRe_[kActBands][kActModes] = {}, actIm_[kActBands][kActModes] = {};
  float actC_[kActBands][kActModes] = {}, actS_[kActBands][kActModes] = {}, actD_[kActBands][kActModes] = {};
  bool actOn_ = false;
  float actGain_ = 0, hallAct_ = 1;
  // The top keys' hammer tick: the hard felt on a short, thin string, a faint hiss of high,
  // inharmonic overtones gone in a few ms (above ~4 kHz; from D6, fully from E7).
  float tick_[kActBands] = {}, tickD_ = 0, tickHp_[kActBands][2] = {}, tickA_ = 0;
  uint32_t tickRng_ = 12345;
  // The top keys' thump as pearls: resonances on the note's subharmonics (an octave, two and
  // three below), round and short, from C6, fully from C7, in place of the wooden knock's highs.
  static constexpr int kPearls = 8, kPearlModes = 3;
  struct Pearl { float re[kPearlModes] = {}, im[kPearlModes] = {}, c[kPearlModes] = {}, s[kPearlModes] = {}; int band = 0; bool on = false; };
  Pearl pearl_[kPearls];
  int pearlNext_ = 0;
  float pearlGain_ = 0;
  std::vector<float> partPlace_, partOut_;
  std::vector<std::vector<float>> part_;
  float treble_ = 0, trebleA_ = 0, trebleM_ = 0, trebleS_ = 0;  // (the honky-tonk's lifted highs)
  float air_ = 0, airA_ = 0, airM_ = 0, airS_ = 0;              // (... and its top)
  float lpB_[3] = {}, lpA_[2] = {}, lpZm_[2] = {}, lpZs_[2] = {};  // (... rolled off: no hiss)
  bool lpOn_ = false;
  struct Box {  // (the closed case's air: three round trips)
    static constexpr int kMax = 2048;
    float line[3][kMax] = {};
    int pos[3] = {};
    float lp[3] = {};
  };
  Box boxM_, boxS_;
  float box_ = 0, boxFb_ = 0, boxLp_ = 0;
  int boxLen_[3] = {2, 2, 2};
  void controlStep() override {}
  float sample() override;
  Piano piano_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

// The pianist. A key throws its hammer: the velocity is the hammer's speed
// (0.35 m/s at the softest to over 5 m/s; the dynamics control scales it),
// and lifts its damper; releasing it lets the damper down, unless the
// sustain pedal is held: the pedal lifts every damper, so all the strings
// ring along with what is played. The brightness knob is the hammers'
// voicing (down: soft felt, mellow; up: hard felt, bright); the hardness
// knob adds to it.
class PianoPlayer : public Performer {
 public:
  explicit PianoPlayer(PianoModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void sustainPedal(int cc) override;
  // (-play) The next release's speed at the dampers (m/s), as the player chose it for the music
  // (an on/off pedal: how hard the foot lets go there); taken by that release.
  void planRelease(double speed) { plannedUp_ = speed; }
  // (-play) The next release is the song's last, or goes into a silence: the foot comes up as gently
  // as it can, the dampers laid on over PJ_PIANO_LASTPEDAL seconds (1.5; 0: as any other).
  void planGentleRelease() { lastUp_ = true; }
  void seed(unsigned) override {}
  static double hammerSpeed(int velocity, double dynamics);  // m/s

 private:
  int fold(int midi) const;
  double pedalMoved(bool h);
  PianoModel& m_;
  bool down_[Piano::kKeys] = {};
  // (the pedal: its controller's last value and when it came (s), the time now, the speed it
  // gave, whether it is down, the draws for each release's own speed)
  int pedalCC_ = 0;
  double pedalT_ = -1, now_ = 0, nextSpeed_ = -1, plannedUp_ = -1;
  bool pedalDown_ = false, lastUp_ = false;
  uint32_t pedalRng_ = 9157;
  int lo_ = 0, hi_ = 127;
  bool held_ = false;  // the sustain pedal, as last told (printed when it changes)
  double t_ = 0;       // the player's clock (s)
};

}  // namespace pajupilli
