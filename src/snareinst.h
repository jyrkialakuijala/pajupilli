// The brushed snare as an instrument (-inst brush-snare): the drum (snare.h)
// behind the Tube and Performer interfaces, as the piano is. One voice is the
// drum and its drummer's two hands.
#pragma once

#include <algorithm>
#include <vector>

#include "performer.h"
#include "snare.h"
#include "tube.h"

namespace pajupilli {

bool brushSnare(const Instrument& inst);

class SnareModel : public Tube {
 public:
  explicit SnareModel(const Instrument& inst);
  Snare& snare() { return snare_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override {
    sideOut_.assign(n, 0.0f);
    const int have = int(side_.size()), m = std::min(n, have);
    std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
    side_.clear();
    return sideOut_.data();
  }
  // The drum's sideways parts (Snare::dipole): across the stage, and towards the listener.
  const float* takeDipole(int n, int k) override {
    dipOut_[k].assign(n, 0.0f);
    const int have = int(dip_[k].size()), m = std::min(n, have);
    std::copy(dip_[k].end() - m, dip_[k].end(), dipOut_[k].end() - m);
    dip_[k].clear();
    return dipOut_[k].data();
  }
  const float* dipoleAxes() const override { return aim_ ? kAxes : nullptr; }
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  Snare snare_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
  // (the drummer faces the listener: the drum's x is the listener's left, its y away from them)
  static constexpr float kAxes[9] = {-1, 0, 0, 0, -1, 0, 0, 0, 1};
  std::vector<float> dip_[3], dipOut_[3];
  bool aim_ = true;
  float trim_ = 1;
};

// The drummer, with a brush in each hand. The keys are General MIDI's (the
// Brush kit's):
//   38  a tap of the right hand, which is the start of a sweep: the brush
//       lands moving and is drawn on across the head ("t-shh");
//   39  a slap: the fan flat, pressed in for a moment;
//   40  the swirl, for as long as the key is held: both brushes on the head;
//   37  the cross-stick (side stick): the stick's butt on the hoop;
//   36, 41  taps at the middle of the head and near its edge;
//   42  (held) the right hand is away, swirling on a cymbal (-play sends it
//       with the cymbal's swirl, drumkit.h): only the left hand circles here.
// Brushing is two-handed, and the drummer does not wait to be asked for it:
// whenever brush strokes are played there is one whoosh under them that
// never stops.
//   - The left hand goes round in smooth circles that never stop, a turn
//     every two beats (one a beat when slow, one a bar when very fast). It
//     takes its tempo from the song (a MIDI file's own beat; alone, the
//     pulse heard in the strokes) and keeps it: it does not jump to double
//     or half. Each turn has its pull, where the hand speeds up and bears
//     down, on the second and fourth beat.
//   - The right hand's stroke is drawn on across the head and lifts, on top
//     of that.
//   - No two rounds are alike: each turn has its own place on the head
//     (nearer the rim for a few turns, then nearer the middle), its own
//     width, slant, weight and pull, and now and then, at a phrase's end,
//     one turn is a flat sweep across and back.
//   - With no strokes for a while (or the swirl held): the right hand
//     circles too, on its own side, the other way round and half a turn
//     behind.
//   - Once begun it does not stop for bars without a stroke: only for a
//     general pause (the whole band silent) or the song's end, and it begins
//     again with the next brush stroke. (Alone, with no song to go by: after
//     twelve seconds without a stroke.)
// (PJ_SNARE_LOOP=0: nothing unasked. PJ_SNARE_TAPSWEEP=0: taps lift at once.
// PJ_SNARE_PULL: how much faster the hand is at the pull, 0: even.
// PJ_SNARE_TAPSWOOSH: how much of a sweep a quarter-note stroke is, 0 .. 2.
// PJ_SNARE_VARY: how much one round differs from the next, 0: all alike.
// PJ_SNARE_MEAT: the quarter-note strokes harder by this, dB. PJ_SNARE_FLAT:
// the share of them that land flatter, a fuller slap.)
// Velocity: the stroke's speed; the swirl's weight on the head. Each tap
// lands a little elsewhere; the brightness knob moves the right hand between
// the middle (dark, fat) and the edge (ringing).
class SnarePlayer : public Performer {
 public:
  explicit SnarePlayer(SnareModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override { hold_ = h; }
  void seed(unsigned s) override { rng_ = 99991u + 7919u * s; }
  // The beat (the swirl's turns follow it): beats per minute, and where in
  // the bar it is now (in beats, 0 .. 4).
  void setBeat(double bpm, double inBar) { bpm_ = bpm; beat_ = inBar; }
  static double strokeSpeed(int velocity, double dynamics);  // m/s

 private:
  double ur();
  void control(double dt);
  SnareModel& m_;
  uint32_t rng_ = 99991u;
  bool swirl_ = false;
  double load_ = 0, bpm_ = 120, beat_ = 0, phase_ = 0;
  bool loop_ = true;
  double sinceTap_ = 100, gap_ = 0.5, ownLoad_ = 0.4, gaps_[4] = {};
  int nGaps_ = 0;
  bool sweeps_ = true;
  static constexpr int kWarp = 256;
  double pull_ = 0.8, pullWide_ = 1.0, pullAt_ = 0.75, warpW_[kWarp], warpT_[kWarp], warpN_ = 1;
  double pulse_ = 0.5, swoosh_ = 0.28;
  int misses_ = 0, turnBeats_ = 0;
  bool brushing_ = false;
  // One round's own way: where its middle is and how wide it is (shares of
  // the usual), the brush's slant, weight and pull, and how much of it is a
  // flat sweep; what the hand aims for this turn, and where it is on the way.
  struct Round { double cx = 0, cy = 0, rx = 1, ry = 1, ang = 0, load = 1, pull = 1, wipe = 0; };
  Round aim_[2], now_[2];
  double vary_ = 1.0, lastPhase_ = 0, zone_ = 1;
  int turn_ = 0, zoneHold_ = 2;
  double tapSwoosh_ = 0.7, held_ = 0.42, meat_ = 1.0, flatShare_ = 0.22, flatHard_ = 0.8;
  bool lastFlat_ = false, rhAway_ = false;
  double rhT_ = -1, rhDur_ = 0, rhV0_ = 0, rhX_ = 0, rhY_ = 0, rhDx_ = 0, rhDy_ = 0, rhAng_ = 1.2, phaseR_ = 3.1;
};

}  // namespace pajupilli
