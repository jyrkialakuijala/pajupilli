// The jazz guitarist (GM 27, -inst jazz-guitar; research/jazz_guitar.md 6, 9.3, 12): the ES-175
// (guitar.h jazzGuitar()) into the jazz amp (guitaramp.h "jazz"), played as a jazz player plays it.
//
// The touch "speaks" (12.1): each stroke is a heavy, round-edged pick held by a hand
// (Guitar::pickContact), and the player's intent sets four things that each colour the tone:
//   - where along the string: 0.165 m from the saddle (over the neck pickup's end) at mf; a
//     ballad's soft comping drifts towards the neck (~0.19 m: the pluck's comb puts its first
//     notch at the 3rd harmonic: round, "oo"), an accent or a loud phrase towards the bridge
//     (0.10-0.13 m: the notch past the 5th, the 2nd-4th strong: "ah", nasal). The hand follows the
//     phrase (the running mean of the velocities) and each note's accent against it.
//   - how deep and how fast: velocity is how far the hand carries the tip (0.1-2.1 mm) and how
//     quickly (a push of 10 ms soft, 2.5 ms hard); the string runs off the edge the faster, the
//     faster the hand goes (the slip as 1/sqrt of its speed): a soft stroke leaves the string
//     rounded (few partials over 1.5 kHz), a hard one sharply (the 2nd-12th all there);
//   - the pick: its thickness (PJ_JAZZ_PICKMM 1.5: its tip's stiffness, 6 kN/m x mm^2.6), its
//     edge's roundness (PJ_JAZZ_EDGE 0.25 ms: the slip of a rounded bevel against a sharp pick's
//     0.15), its contact's width (PJ_JAZZ_TIPW 2.5 mm); Wes's thumb (PJ_JAZZ_THUMB) the softest
//     and widest of all: 14 mm of flesh, a 4 ms rise, a 1.8 ms slip;
//   - the angle: a harder stroke digs more into the top (the swing the floating bridge drains
//     quickly: the attack's punch, then the bloom).
//   Against the body's fixed resonances and the pickup's own comb, that is the vowel. The
//   flatwounds' steep losses above the 3rd harmonic (k^2) make it change in time: what a hard
//   stroke adds above 1.5 kHz dies in 50-300 ms into the warm, long fundamental.
//
// Chords (6.3-6.4): a voicing that a hand can hold, never dropping the top note: range folded
// (a chord over C6 an octave down as a whole where its bass allows, else the notes over it; under
// E2 an octave up); one string per note, low notes on low strings;
// the fretted notes within 4 frets (5 from the 5th fret up); no open strings in comping (they cannot be choked); known grips
// (shell, drop 2, drop 3) win ties, and the one nearest the hand (it moves 0-2 frets). If none
// fits, reduce: doubled pitch classes, the fifth over the bass, the bass under a wide gap, the
// root doubled above, then inner notes from the bottom sparing the 3rd and 7th; 5-7 notes that
// will not fit go to four.
//   - The stroke: a down rake (2 ms a string loud, 5 soft), up on quick off-beats; the strings
//     between the grip's skipped: muted by the fretting hand, the pick's dead click.
//     The strings outside the grip, not holding a note, lie under the fretting hand's fingers
//     (200 /s; beside a single line 40 /s): they do not ring along through the body.
//   - Let go: the left hand eases off (5 ms), the string dies in ~30 ms (200 /s) with a soft "chk";
//     each string as its key is let go.
//   - A note let go before it sounded (zero length in the file): a ghost stroke, the strings
//     touched not pressed: a thud of 20 ms; under velocity 25 the dead strings alone.
//   - Four to the bar (8+ chords about a beat apart, each under 0.6 beat): all down, 2 and 4 a
//     little harder, the outer voices damped so the inner line sings (Freddie Green).
//   - A soft chord after a long rest (1.5 beats): the fingers, all together, flesh.
// Single lines (6.5): down on the beat, up off it when they come quickly; a step of 1-2 semitones
// onto the beat on the same string, about half the time, a hammer-on or pull-off; a note much
// softer than its neighbours a ghost (half-fretted). Vibrato late and shallow (6 cents).
// Octaves (6.6): two notes 12 apart, strings s and s+2, the one between muted and swept: with the
// thumb (PJ_JAZZ_THUMB: 0 never, 1 octaves and the soft notes among them (default), 2 every
// single line). Pitch bend: the last single note's string.
// The keys of one gesture: whatever comes within 8 ms (PJ_JAZZ_GATHER) of the first is played
// together (every note 8 ms late: a constant). PJ_JAZZ_PLACE (0.165 m), PJ_JAZZ_MOVE (1: how far the hand follows the phrase), PJ_JAZZ_RELEASE
// (200 /s), PJ_JAZZ_DEBUG=1 (each stroke on stderr), PJ_JAZZ_STATS=1 (the voicings' tally at the end).
#pragma once

#include <vector>

#include "guitarinst.h"

namespace pajupilli {

class JazzGuitarPlayer : public Performer {
 public:
  explicit JazzGuitarPlayer(GuitarModel& m);
  ~JazzGuitarPlayer() override;
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned s) override { rng_ = 2707u + 7919u * s; }

 private:
  static constexpr int kS = Guitar::kStrings;
  struct Asked { int midi, velocity; bool ghost; };
  // What the right hand will do, and when: a pick, the thumb, the fingers, a hammer, a dead
  // stroke (the string damped as it is struck), a let-go.
  enum Kind { kPick, kThumb, kFingers, kHammer, kDead, kRelease, kDamp };
  struct Touch { double depth, from, push, slip, angle; };
  struct Due { double in; Kind kind; int string, fret; Touch t; double dampAfter, dampRate; };
  struct Grip { int string[kS], fret[kS]; double cost; };
  double ur();
  Touch touch(double v, bool up, int string);
  bool finger(const std::vector<int>& notes, Grip& g, bool comp) const;
  bool voice(std::vector<Asked>& notes, Grip& g);
  void play();
  void chord(std::vector<Asked>& notes);
  void octave(const Asked& lo, const Asked& hi);
  void single(const Asked& a);
  void sounding(int s, int midi, int fret, int vel, bool ghost);
  GuitarModel& m_;
  uint32_t rng_ = 2707u;
  std::vector<Asked> asked_;
  std::vector<Due> due_;
  int strNote_[kS], fret_[kS] = {}, vel_[kS] = {};
  bool held_[kS] = {};
  double age_[kS] = {};
  double hand_ = 5, clock_ = 0, sinceNote_ = 10, gapBefore_ = 10, velMean_ = 60, lastVel_ = 60;
  int pickWay_ = 1, lastString_ = -1, lastMidi_ = -1, bendString_ = -1;
  double lastOctave_ = -10;
  // The chords' rhythm (four to the bar): their onsets (s) and how long each was held (s).
  std::vector<double> chordOn_, chordLen_;
  int chordMidi_[kS] = {-1, -1, -1, -1, -1, -1};
  bool fourBar_ = false;
  double vib_[kS] = {}, vibPh_[kS] = {};
  // The knobs.
  double askAge_ = 0, gather_ = 0.008;  // (the keys gathered for this long before they are played)
  double place_ = 0.165, move_ = 1, stiff_ = 17000, edge_ = 0.0003, tipW_ = 0.0025, release_ = 200, chk_ = 1;
  int thumb_ = 1;
  bool debug_ = false, stats_ = false;
  // The tally (PJ_JAZZ_STATS): chords, their notes asked, dropped, top notes dropped, unplayable,
  // open strings in chords of 3+, the hand's moves.
  long nChords_ = 0, nNotes_ = 0, nDropped_ = 0, nTopDropped_ = 0, nFailed_ = 0, nOpen_ = 0;
  double moveSum_ = 0;
  long moves_ = 0;
};

}  // namespace pajupilli
