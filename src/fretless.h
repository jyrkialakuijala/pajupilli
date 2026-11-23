// The fretless bass (-inst fretless-bass; General MIDI program 36, "Fretless Bass";
// research/fretless_bass.md): the two-pickup bass (src/bassspec.cc twoPickupBass()) with its frets pulled and
// the slots filled, played by a fretless player.
//   - The string ends under a fingertip pressing it onto a hard board, not on a fret. Behind the
//     finger the string rises from the board at a small angle (the action over the board's end,
//     2.5 mm, over the stopped length less the board's 27 cm: 4-20 mrad); a string plucked hard
//     swings at its end by more than that, and lies down on the board next to the finger for part
//     of every period: a jawari laid along the neck (2.1). Its upper partials (5-9) grow late,
//     peaking 100-300 ms after the pluck, the pluck-position notches fill in, and the contact
//     dies out after 0.3-0.5 s: the mwah. Guitar's board (GuitarSpec::board): five points, 3 to
//     36 mm behind the finger, the double bass's penalty (3e4 N/m, 1.5 N s/m). Soft notes hardly
//     touch; open strings never (they end on the nut); high on the neck the angle is steeper and
//     it touches less.
//   - The fingertip's loss: half the double bass's (2.4, 8.2.3), instead of the fret's x1.15.
//   - The bridge pickup favoured (the neck one at half its volume; PJ_FRETLESS_PICKUP 1: the bridge
//     alone, 2: both full, 3: the neck alone), the J network; into "fretless-di" (src/guitaramp.cc:
//     an active DI 70 % with the Variamp's mid push, +3 dB at 800 Hz, the fliptop 30 %, an optical compressor at 3:1)
//     and, if asked (PJ_FRETLESS_CHORUS 0..1, off as on a Sound Canvas), a bucket-brigade chorus:
//     7 +- 2 ms at 0.6 Hz, the lows under 150 Hz dry, the right side's sweep inverted.
// The player (3, 7.5): every stopped note is one quantity, the stopping finger's place in cents
// (Guitar::stop): its landing (a human error, sd 8 c, PJ_FRETLESS_ERR; a deliberate scoop from
// 15-35 c flat on some slower, harder notes, the finger sliding up into it, overshooting and
// settling: the sequenced "fretless scoop" of the user's files), its correction (a second-order
// approach, ~100 ms), the band's pull (below), a residue (+-(2 + 0.3 sd) c, the player's bias
// 0 +- 3 c), the late vibrato (from 0.2-0.35 s on a held stopped note, +-10 c at ~5 Hz, wider up
// the neck; the mod wheel adds up to +-25 c), slides, and the pitch wheel (+-2 semitones). A new
// note while the last one's key is still down, at most 4 semitones away on the same string, held
// over it for 40 ms or more: a slide, no new pluck (1-2 semitones 60 % of the time, 3-4 30 %;
// PJ_FRETLESS_SLIDES 0 never, 2 always; never in a jazz trio's file), 60 ms + 35 ms a semitone,
// a raised cosine. Notes above the board (D#4 up) as natural harmonics where one fits within
// 20 c, else an octave down; PJ_FRETLESS_HARMONICS=1: from C4 up, harmonics wherever they fit.
// The keys are acted on 60 ms after they come (PJ_FRETLESS_LEAD; -play moves them that much
// earlier): the player sees whether the last key is held on over the next before it decides.
// The band (src/band.h, -play): each stopped note is tuned by ear against what the other channels
// sound, within +-15 c: the choir's spectral rule (Ensemble::choirTune), the bass's own partials
// (with its strings' stiffness) against each other note's family template, each pair of partials
// within 36 c pulling by the weaker one's level over the higher harmonic number squared. Over a
// chord with its root above it hardly moves; under a bare third or a shell voicing it leans in
// (+10-15 c). A weak pull is followed only partly; a chord changing over a held note is followed
// at most 1 c/ms, changes under 2 c let be; with nothing to hear, back to 0 in ~3 s. PJ_FRETLESS_JI=0:
// equal temperament. PJ_FRETLESS_LOG=1: each note's decisions, the board's contacts on stderr.
#pragma once

#include <memory>
#include <vector>

#include "guitar.h"
#include "guitaramp.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool fretlessBassInstrument(const Instrument& inst);
GuitarSpec fretlessBass();  // with the PJ_FRETLESS_* knobs

class FretlessBassModel : public Tube {
 public:
  explicit FretlessBassModel(const Instrument& inst);
  Guitar& bass() { return g_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  Guitar g_;
  int nanSeen_ = 0;  // g_.nanResets() already answered with a resetAir()
  float strings();   // g_.sample(), with the tests' kick in and a caught NaN answered
  std::unique_ptr<GuitarAmp> amp_;
  double b0_ = 1, b1_ = 0, b2_ = 0, a1_ = 0, a2_ = 0, z1_ = 0, z2_ = 0, out_ = 1;
  // The chorus (PJ_FRETLESS_CHORUS): a delay line, the wet path's high-pass, the LFO's phase.
  double chorus_ = 0, lfo_ = 0, lfoStep_ = 0, hpA_ = 0, hpX_ = 0, hpY_ = 0;
  std::vector<float> line_;
  int pos_ = 0;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

class SoundingSet;

class FretlessBassPlayer : public Performer {
 public:
  explicit FretlessBassPlayer(FretlessBassModel& m);
  ~FretlessBassPlayer() override;
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override { hold_ = h; }
  void seed(unsigned s) override;
  void setBand(const SoundingSet* band, int self) override { band_ = band; self_ = self; }
  double tuning(int key) const override;

 private:
  static constexpr int kStrings = 4;
  struct Pend { int midi, vel; double t; bool on; };
  // What each string is doing.
  struct Line {
    int midi = -1, key = -1;  // the key it plays, the pitch it sounds (folded)
    bool held = false;
    int harmonic = 0;         // a natural harmonic's n (0: none)
    int fret = 0;             // the note's place (semitones above the open string; 0: open)
    double x = 0, xv = 0;     // the finger's cents against it, and their speed (c/s)
    double zeta = 0.6, wn = 28;
    double band = 0, bandTo = 0, bandMix = 0;  // the band's pull (c): now, where it goes; how firmly
    double residue = 0;       // this note's own error left after the correction (c)
    double age = 0, vibDelay = 0.3, vibPhase = 0, vibRate = 5;
    bool slide = false;
    double slFrom = 0, slT = 0, slDur = 0, slErr = 0;
    double pos = 0;           // where the finger stops the string now (semitones)
    double lastSolve = -1;
    unsigned bandVersion = ~0u;
  };
  double ur();
  double gauss();
  void start(size_t k);
  void release(int midi);
  int harmonicFor(int midi, int* n) const;
  bool overlapped(size_t k, int prevMidi) const;
  // The band's pull on a note on string s at `key` (c), and its weight.
  double bandPull(int s, int key, double* weight) const;
  double aim(const Line& l) const;
  void steer(int s, double dt);

  FretlessBassModel& m_;
  Line line_[kStrings];
  std::vector<Pend> pend_;
  double t_ = 0, wait_ = 0, sinceNote_ = 10, hand_ = 3;
  int last_ = -1, finger_ = 0;
  uint64_t rng_ = 9001;
  const SoundingSet* band_ = nullptr;
  int self_ = -1;
  bool bendSeen_ = false;
  // The player: his bias (c), his landing error's sd (c), vibrato rate and depth (x), knobs.
  double bias_ = 0, errSd_ = 8, vibRate_ = 5, vibDepth_ = 1;
  double slides_ = 1, scoop_ = 1, ji_ = 1, lim_ = 15;
  bool harmonics_ = false;
  int log_ = 0, string_ = 0;
  double pad_ = 0.010, snap_ = 0.4, rise_ = 1, slip_ = 1, angle_ = 0.6, place_ = 0.09;
  long contact_ = 0, samples_ = 0, slid_ = 0, struck_ = 0;
  double lastContact_ = -1;
};

}  // namespace pajupilli
