// The electric bass (-inst electric-bass; General MIDI program 34, "Electric Bass (finger)";
// research/bass_section.md): a Fender Jazz Bass played fingerstyle. The electric guitar's model
// (guitar.h) with four strings and the bass's numbers:
//   - 34 in. (864 mm), 20 frets, D'Addario EXL170 (45-65-80-100) roundwounds: tensions 155-216 N,
//     B from 3.5e-5 (G, measured) to ~2e-4 (E) (2.1-2.2). Each partial's loss from the string's
//     internal damping and the neck's support (Fleischer): the open E's fundamental rings ~20 s,
//     the partials die about in proportion to their frequency, 0.4-1 s at 8 kHz (Kemp). Modes to
//     5 kHz: 120 on the open E.
//   - The neck: its second bending mode near 150 Hz and the third near 290 (Fleischer): the
//     dead spot on the G string at the 5th-7th fret.
//   - Two J pickups (3.1): the neck one 156 mm from the saddle, the bridge one 55 mm ('70s
//     spacing); both full up: two coils in parallel, the scooped "both up" sound (the notch
//     near the open E's 8th harmonic). A coil 3.3 H, 7.4 kOhm, ~90 pF; 250 k pots (two volumes,
//     one tone), the cable 700 pF, into a 1 MOhm input: a resonance near 4.4 kHz, Q ~1.7 (3.3).
//   - What comes out (research/bass_amps.md 10.1, src/guitaramp.h's "b15-di"): the pickups on a
//     splitter into an active DI and an Ampeg B-15N (0.9 MOhm between them), the B-15's 1x15
//     miked with an RE20, the two blended half and half (lined up), then an LA-2A at 3:1.
//     PJ_EBASS_DI=1: the DI alone, as it was (the coils into 1 MOhm, nothing after).
// The bassist: each note to a string and fret (low on the neck, open strings welcome, near the
// hand); the index and middle fingers in turn while notes come quickly, rest strokes between the
// pickups (~10 cm from the saddle, the brightness knob moves it), the string pushed along the
// body and a little into it; velocity the depth (0.4-5 mm). Every note is stopped when its key
// is let go (the fretting hand lifts, the plucking hand lies on it), and a new note on another
// string stops the last one even if its key is still down (a line is one voice), unless the two
// came together (a double stop); the strings not played are kept muted (no open string rings
// along). Pitch bend bends the last note's string.
//
// The pick bass (-inst pick-bass; GM 35, "Electric Bass (pick)"; research/pick_bass.md): the
// same strings and neck on a Fender Precision (src/bassspec.cc precisionBass(): one split coil,
// the E/A half 149 mm from the saddle, the D/G half 124 mm, 6.4 H, 10.5 kOhm; one volume, one
// tone; PJ_PBASS_FLATS=1 flatwounds), the same fretting hand (BassLinePlayer), a pick held by a
// hand instead of the fingers (PickBassPlayer, Guitar::pickContact), into an Ampeg SVT and its
// 8x10 with the DI, an 1176 (guitaramp.h's "svt").
#pragma once

#include <memory>
#include <vector>

#include "guitar.h"
#include "guitaramp.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool electricBassInstrument(const Instrument& inst);  // the finger bass, the pick bass, the slap basses
bool pickBassInstrument(const Instrument& inst);
int slapBassInstrument(const Instrument& inst);  // 1: GM 37, 2: GM 38, 0: neither
GuitarSpec jazzBass();
GuitarSpec precisionBass();

class ElectricBassModel : public Tube {
 public:
  explicit ElectricBassModel(const Instrument& inst);
  Guitar& bass() { return g_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override {
    sideOut_.assign(n, 0.0f);
    return sideOut_.data();
  }
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  Guitar g_;
  int nanSeen_ = 0;  // g_.nanResets() already answered with a resetAir()
  float strings();   // g_.sample(), with the tests' kick in and a caught NaN answered
  // The amp, the DI and the compressor; or (PJ_EBASS_DI) the bare DI: the coils with their pots
  // and cable into 1 MOhm (a biquad). The level.
  std::unique_ptr<GuitarAmp> amp_;
  // The slap basses' onboard preamp (GM 38's active J: research/slap_bass_2.md 2.3): a bass and a
  // treble shelf on the buffered coils' EMF (0 dB: none).
  struct Shelf {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    double run(double x) {
      const double y = b0 * x + z1;
      z1 = b1 * x - a1 * y + z2;
      z2 = b2 * x - a2 * y;
      return y;
    }
  };
  Shelf shelf_[2];
  bool shelves_ = false;
  double b0_ = 1, b1_ = 0, b2_ = 0, a1_ = 0, a2_ = 0, z1_ = 0, z2_ = 0, out_ = 1;
  float last_ = 0;
  std::vector<float> sideOut_;
};

// The bassist's fretting hand and the rules of a bass line, the same whatever plucks: each note
// to a string and fret (low on the neck, open strings welcome, near the hand); a released note
// stopped; a new note on another string stops the last (one voice) unless the two came together
// (a double stop); the strings not played kept muted; pitch bend on the last note's string. What
// plucks is the player's own (strike()).
class BassLinePlayer : public Performer {
 public:
  explicit BassLinePlayer(ElectricBassModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override { hold_ = h; }
  void seed(unsigned s) override { rng_ = 4242u + 7919u * s; }

 protected:
  static constexpr int kStrings = 4;
  struct Asked { int midi, velocity; };
  double ur();
  // Before the notes of one moment are struck (how many), and each one: string s at `fret`.
  virtual void begin(int /*count*/) {}
  virtual void strike(int s, int fret, const Asked& a) = 0;
  // What the player adds to the cost of taking note a on string s at fret (the slap player's
  // pops want the D and G strings).
  virtual double cost(int /*s*/, int /*fret*/, const Asked& /*a*/) { return 0; }
  ElectricBassModel& m_;
  double sinceNote_ = 10;  // s since the last notes were struck
  bool ringOn_ = false;    // the notes let ring into each other (drones, chords): no one-voice rule
  double offDamp_ = 40;    // a released note's damping (1/s)
  int noteOn_[kStrings] = {-1, -1, -1, -1};
  bool held_[kStrings] = {};
  double hand_ = 3;
  int last_ = -1, lastFret_ = 0;  // the string plucked last, its fret

 private:
  void play();
  uint32_t rng_ = 4242u;
  std::vector<Asked> asked_;
};

// GM 34: fingerstyle (research/bass_section.md 6.1).
class ElectricBassPlayer : public BassLinePlayer {
 public:
  explicit ElectricBassPlayer(ElectricBassModel& m) : BassLinePlayer(m) {}

 private:
  void strike(int s, int fret, const Asked& a) override;
  int finger_ = 0;  // the finger that plucked last (0 index, 1 middle)
};

// GM 35: a pick (research/pick_bass.md 9.2c). Guitar::pickContact: the hand carries the pick's
// tip aside, the string slips off it. Velocity: how far (0.6-3.2 mm) and how quickly (a push of
// 12 ms at pp, 3 at ff: the string leaves its edge faster, too). Down strokes, and up strokes
// between them while notes come within 0.3 s (along and a little out of the body: a little
// weaker); PJ_PICK_DOWN=1 downstrokes only. Over the pickup's end, 10 cm from the saddle (the
// brightness knob and PJ_PICK_PLACE move it), +-6 mm. The pick (PJ_PICK_MM, 1.0): its stiffness
// and slip. The palm (PJ_PICK_MUTE 0..1) on the string near the saddle, lifted a little on
// accents; Carol Kaye's felt (PJ_PICK_FELT=1) under all four, always; PJ_PICK_CHORDS=1 lets
// strings ring together (drones).
class PickBassPlayer : public BassLinePlayer {
 public:
  explicit PickBassPlayer(ElectricBassModel& m);

 private:
  void begin(int count) override;
  void strike(int s, int fret, const Asked& a) override;
  bool up_ = false, downOnly_ = false;
  double place_ = 0.10, kp_ = 6000, slip_ = 0.00015, mute_ = 0, felt_ = 0, upGain_ = 1.3, pushPp_ = 0.012, pushFf_ = 0.003, velExp_ = 1.8;
};

// GM 37 and 38, the slap basses (research/slap_bass_1.md 5.3-5.5, slap_bass_2.md 5.2, 6): the
// same Jazz Bass with its frets as obstacles (Guitar's fretContact), slapped and popped. Each note:
//   - a ghost (low velocity, short): the fretting hand lies on the string unpressed (a deep mute
//     where it lies), and the right hand strikes all the same: a pitchless click off the frets;
//   - a hammer-on or pull-off (a step of 1-3 semitones on the string still held, soon after,
//     not accented): no stroke, the fretting finger comes down (a small knock) or leaves (a small
//     pluck);
//   - a pop (the D and G strings): GM 37 where a line leaps up 9-16 semitones from its last
//     thumbed note, accented high notes, the top of a double stop; GM 38 nearly everything from
//     D2 up, the hand up the neck to keep it on the D and G strings. The finger hooks the string
//     12-25 ms before it lets go (the hook early by that much when the keys come early: noteLead);
//   - otherwise the thumb (Guitar::thump): bouncing off the string at the end of the neck, driving
//     it onto the last frets; quick repeated thumbs alternate down and up (double thumbing); slow
//     heavy ones are rest strokes that come to lie on the next string.
// Played from a file, every key is acted on noteLead later (90 ms): the player sees a short
// note's release, and the next note, before it strikes. Live, nothing is early: the pops' hooks
// make them late, as on a bass.
class SlapBassPlayer : public BassLinePlayer {
 public:
  SlapBassPlayer(ElectricBassModel& m, int program);
  ~SlapBassPlayer() override;
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;

 private:
  enum Tech { kThumb, kPop, kGhost, kHammer, kPull };
  struct Pend {
    int midi, vel;
    double t;          // when the key came (the player's clock)
    bool on;
    bool decided = false, prevHeld = false;
    int tech = kThumb;
    double pre = 0;    // how long before its time it starts (the pop's hook)
  };
  void begin(int count) override;
  double cost(int s, int fret, const Asked& a) override;
  void strike(int s, int fret, const Asked& a) override;
  void decide(size_t i);
  double ur2();
  int prog_;
  double wait_ = 0, now_ = 0;
  std::vector<Pend> pend_;
  int tech_[128];          // the technique decided for each key about to be struck
  double rise_[128];       // a pop's hook (s)
  double delay_[128];      // how much later than it could (struck with the keys that came with it)
  bool keyDown_[128] = {};
  double offAt_[128];
  // The line so far: the last note (key, when, velocity), the last thumbed one, the velocity's
  // running mean, the lowest key of late; the last thumb's time and stroke (up: double thumbing).
  int lastArr_ = -1;        // the last key to come (for legato)
  int lastMidi_ = -1, lastVel_ = 64, thumbMidi_ = -1, lowMidi_ = 127, chord_ = 1, chordIdx_ = 0;
  double lastT_ = -10, thumbT_ = -10, meanVel_ = 80, lowT_ = -10, lastThumbAt_ = -10;
  bool up_ = false;
  int count_[6] = {};      // thumbs, pops, ghosts, hammers, pull-offs, upstrokes (PJ_SLAP_COUNT)
  // Knobs.
  double popBias_ = 1, thumbV_ = 1, thumbAt_ = 0.265, ghostV_ = 40;
  bool log_ = false;
  int force_ = 0;
  double flesh_ = 0;  // the thumb's flesh: its damping (s/m)
  double thumbAngle_ = 1.3, thumbLight_ = 1;
};

}  // namespace pajupilli
