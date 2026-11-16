// The cymbals of a jazz kit (research/cymbal_plan.md steps 2, 3, 5;
// research/cymbals_built.md): -inst ride-cymbal (a 22" thin jazz ride),
// ride-cymbal-2 (a 20" drier, heavier ride), crash-cymbal (16" thin),
// crash-cymbal-2 (18" thin), splash-cymbal (10"), china-cymbal (18").
//
//   - The plate (plate.h): a thin bronze plate with a free edge, as its modes
//     to 16 kHz (a 22" ride: ~1500), the dome lifting the stretching modes,
//     each pair of a hammered cymbal a little apart. One plate lives through
//     the whole song: strokes pile up in modes that ring for seconds (the
//     ride's wash), and each stroke meets the plate where it is.
//   - The whole cymbal also moves as a body on its stand: up and down on the
//     felt, and rocking on the tilter (~1.5 Hz). That is the give behind the
//     stick, not a sound.
//   - The bell: the plate here is uniform; the cup in the middle, thick and
//     stiff, rings as its own few modes (a clear, pitched tone), driven by
//     what strikes inside its rim.
//   - What touches it:
//       * a stick: its tip (12 g behind the blow) or its shoulder (30 g, on
//         the edge), a hard Hertz contact solved against the plate's own give
//         each sample (as the hi-hat's); the stick's own three bending modes
//         ring a moment (the "wood" in the ping);
//       * a brush (brush.h): the hand, grip, fan and wire ends on the plate;
//       * the hand: choking, a loss on the modes that move at the edge.
//   - The sound: two microphones over the cymbal, every mode by its shape
//     (a Rayleigh sum over the plate; below the coincidence pitch only its
//     edge sounds).
//
// Linear: the plate's nonlinearity (the cascade that makes a crash bloom,
// cymbal_plan.md step 4) is not built. A hard crash here is a loud linear
// plate: bright at once, no build-up over 0.1-0.3 s.
#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

#include "brush.h"
#include "performer.h"
#include "plate.h"
#include "tube.h"

namespace pajupilli {

bool cymbal(const Instrument& inst);

struct CymbalSpec {
  PlateSpec plate;
  BrushSpec brush;
  bool ride = true;            // played with the tip on the bow (a ride), or the shoulder on the edge (a crash)
  double bowAt = 0.62;         // the ride's bow, as a share of the radius
  // The bell: its rim (m), its modes (Hz, T60 s), how much of a stroke inside
  // it goes into them (their mass, kg) and how loud they are heard.
  double bellR = 0.065;
  int bellN = 6;
  double bellF[6] = {820, 1255, 1730, 2205, 2790, 3440};
  double bellT60[6] = {4.5, 4.0, 3.5, 3.0, 2.5, 2.0};
  double bellMass = 0.15, bellGain = 1.0;
  // The body on the stand.
  double seatK = 2e4, seatLoss = 0.3;   // N/m; share of critical
  double tiltK = 5, tiltLoss = 0.05;    // N m/rad
  // The hand on the edge: the loss (1/s, for a mode of middling edge motion)
  // when fully choked.
  double choke = 300;
  // The stick's own ring (its bending modes, damped by the hand), by how
  // much of the blow (a share of the plate's own sound).
  double wood = 0.03;
  double micX = 0.25, micZ = 0.5;
  double gain = 1;
  // Each mode's loss times a factor by its pitch (log-interpolated between
  // these points), fitted to measured band decays.
  int nFix = 0;
  double fixF[8] = {}, fixMul[8] = {};
};
// The six: 0 ride 22", 1 ride 20", 2 crash 16", 3 crash 18", 4 splash 10", 5 china 18".
CymbalSpec cymbalSpec(int which);

class Cymbal {
 public:
  // What the brush touches: the plate as it is, its pushes kept apart (the
  // cymbal hands them on, scaled: the brushed level).
  struct Feel : public Surface {
    explicit Feel(Plate& p) : p_(p), f((p.modes() + 15) / 16 * 16, 0.0f) {}
    int modes() const override { return p_.modes(); }
    double radius() const override { return p_.radius(); }
    void shape(double r, double theta, float* phi) const override { p_.shape(r, theta, phi); }
    const float* q() const override { return p_.q(); }
    const float* v() const override { return p_.v(); }
    float* force() override { dirty = true; return f.data(); }
    Plate& p_;
    std::vector<float> f;
    bool dirty = false;
  };
  Cymbal(double fs, const CymbalSpec& spec, uint32_t seed);
  // A stick: the tip (or the shoulder) at `speed` m/s, r from the centre (m), theta.
  void stick(double speed, double r, double theta, bool shoulder);
  Brush& brush() { return *brush_; }
  Plate& plate() { return plate_; }
  // The hand on the edge: 0 off .. 1 a full grip.
  void choke(double amount) { choke_ = amount; if (amount > 0) wake(); }
  float sample();
  float side() const { return side_; }
  void reset();
  bool asleep() const { return asleep_; }
  void wake() { asleep_ = false; quiet_ = 0; }
  const CymbalSpec& spec() const { return s_; }

 private:
  CymbalSpec s_;
  double fs_, dt_;
  FILE* trace_ = nullptr;  // PJ_CYM_TRACE: the brush's force, float32 per sample
  Plate plate_;
  std::unique_ptr<Feel> feel_;
  std::unique_ptr<Brush> brush_;
  float brushGain_ = 1;
  // The body: height and two tilts.
  double z_ = 0, vz_ = 0, ax_ = 0, wx_ = 0, ay_ = 0, wy_ = 0;
  // The stick.
  bool stickOn_ = false;
  double sz_ = 0, sv_ = 0, sm_ = 0.012, sK_ = 1e9, sG_ = 0, sx_ = 0, sy_ = 0, sBell_ = 0;
  int sAge_ = 0;
  std::vector<float> sPhi_;
  // The bell's and the stick's own modes: complex amplitudes, turns.
  float bRe_[6] = {}, bIm_[6] = {}, bPr_[6] = {}, bPi_[6] = {}, bW_[6] = {};
  float wRe_[3] = {}, wIm_[3] = {}, wPr_[3] = {}, wPi_[3] = {};
  // The choke.
  std::vector<float> edge_;
  double choke_ = 0;
  // The microphones.
  std::vector<float> mic_[2];
  float prev_[2] = {}, bPrev_ = 0, side_ = 0;
  bool asleep_ = true;
  int quiet_ = 0;
};

class CymbalModel : public Tube {
 public:
  explicit CymbalModel(const Instrument& inst);
  Cymbal& cymbal() { return cym_; }
  int which() const { return which_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override {
    sideOut_.assign(n, 0.0f);
    const int have = int(side_.size()), m = std::min(n, have);
    std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
    side_.clear();
    return sideOut_.data();
  }
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  int which_;
  Cymbal cym_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

// The drummer's hand at a cymbal. Keys (General MIDI's; a brush stroke is
// the same key 24 higher, as the kit sends the Brush kit's cymbals):
//   51, 59  the ride's bow with the tip (an accent, velocity >= 112: the
//           shoulder a little further out); the place drifts over the bow by
//           phrases (research/stroke_placement.md 7.2): held for a while,
//           then moved, two times in three to the other side;
//   53      the bell with the tip;
//   49, 57, 55, 52  a crash: the shoulder on the edge (soft, velocity < 48:
//           the tip near the edge, a swell's stroke);
//   other keys: the tip, from the edge (low) to the bell (high).
// With a brush: a tap that lands and is drawn on a little (a crash: a
// sweep across the edge), quiet, no ping.
// The choke: the breath (channel pressure, pad pressure) is the hand
// grabbing the edge. Note-offs do nothing (GM's drum keys), unless
// PJ_CYM_NOTEOFF=1 (then a note-off chokes, as an e-kit's cymbal pad does).
class CymbalPlayer : public Performer {
 public:
  explicit CymbalPlayer(CymbalModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override { hold_ = h; }
  void seed(unsigned s) override { rng_ = 52361u + 7919u * s; }
  static double stickSpeed(int velocity, double dynamics);  // m/s

 private:
  double ur();
  double bowPlace();   // the ride's bow, drifting by phrases (share of the radius)
  double angle();      // the side facing the drummer, wandering (rad)
  double lean(int velocity);  // how much a brush stroke is leaned on, 0 .. 1
  CymbalModel& m_;
  uint32_t rng_ = 52361u;
  // The drift: where the hand is, where it goes, strokes left at this place.
  double bow_ = 0, bowTo_ = 0, home_ = 0, wander_ = 0, thWander_ = 0;
  int held_ = 0, side_ = 1;
  double choked_ = 0, noteChoke_ = 0;
  // Where the bar is, learnt from the ride's own strokes (lean()).
  double phaseCount_[4] = {}, beatCount_[2] = {}, skipCount_[2] = {}, lastBeat_ = -100;
};

}  // namespace pajupilli
