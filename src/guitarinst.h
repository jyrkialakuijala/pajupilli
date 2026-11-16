// The guitars as instruments (-inst nylon-guitar): the model (guitar.h)
// behind the Tube and Performer interfaces, and the guitarist.
#pragma once

#include <algorithm>
#include <vector>

#include "guitar.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool guitarInstrument(const Instrument& inst);

class GuitarModel : public Tube {
 public:
  explicit GuitarModel(const Instrument& inst);
  Guitar& guitar() { return g_; }
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
  Guitar g_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

// The guitarist (research/guitars.md 7). MIDI gives notes; a guitar wants a
// string and a fret for each, and a hand to pluck them.
//   - Which string: every note that lies on a string (open or within the
//     frets) may go there; the one chosen is free, near where the left hand
//     already is (a hand spans four or five frets), low on the neck and open
//     where it can be. A chord's notes go to different strings, the low
//     notes to the low strings. A note outside the guitar's range is moved
//     in by octaves. A new note on a sounding string ends the old one.
//   - How plucked: notes that come together are one gesture: the thumb
//     takes the lowest (on the bass strings: more flesh, into the top), the
//     fingers the rest, a few milliseconds one after another from the bass
//     up (wider when soft and slow, nearly together when loud). Picking is
//     the guitar's own voice, and velocity is its colour as much as its
//     loudness: soft, the flesh of the fingertip alone (dark, round,
//     intimate); from the middle up a little nail and then more (crisp,
//     woody); at the hardest the nail knocks on the string as it takes it
//     (the clack: more of the upper middle, never metallic). The brightness
//     knob moves the hand along the string (towards the bridge: brighter).
//   - Strumming. When three or more strings are sounded within 60 ms,
//     one after another across the strings (or five or six at once, more
//     than thumb and fingers can pinch), the right hand is strumming, and
//     goes on strumming while such chords keep coming. Then the strings are
//     not plucked but caught by a hand passing over them, further from the
//     bridge, over the soundhole: it does not take a string first, so what
//     the string was still sounding rings on into the new stroke. The hand
//     crosses the strings at a nearly constant speed however loud the chord
//     is meant (7.5 ms from string to string): velocity is how deep the
//     fingers reach into the strings, the chord's as a whole (its notes'
//     mean) and much evened out: every string gets much the same of the one
//     stroke; going down the bass strings a little more, coming up the
//     middle strings, the highest spared. Flesh and nail together are broad and
//     soft on the string: a strum is warm, thick and velvety where a plucked
//     note is clear. Down (bass to treble) is the fingers' backs, the bass
//     strings struck fullest. Up (treble to bass) is the fingertip's pad:
//     rounder and darker still, lighter, and lighter again on the bass
//     strings it reaches last.
//     Chords that come all at once have no direction of their own: the
//     hand swings down and up like a pendulum whether it touches the strings
//     or not, so a chord one swing after the last goes the other way, two
//     swings after the same way again; with the song's beat known, a chord
//     on a beat is most often down, on the bar's first beat nearly always;
//     and where the last bar's pattern was clear, a small pull towards
//     playing this bar the same way.
//     A down stroke hardly disturbs what the strings still sound; an up
//     stroke's pad lies on them a moment and takes some of it. A strum never has the hard nail's sound nor its
//     knock, however loud: a nail is curved, and where picking meets its
//     stiff hollow side and edge, strumming meets its rounded back, which
//     flexes.
//   - The same note played again and again is never plucked the same way
//     twice. He hears how the repeats are meant (their spacing and how their
//     velocities change) and answers with the right hand: a note louder
//     than the last is a rest stroke, into the top and nearer the bridge
//     (weight and bite, without pulling harder); a softer one is taken
//     nearer the fingerboard on more flesh (round, flute-like); even ones
//     alternate fingers (a little more nail, a little less), and every third
//     has more flesh; the hand wanders between fingerboard and bridge from
//     one to the next, further when they come slowly, hardly in a fast run.
//   - When nothing else is happening, the left hand may warm a held note
//     with vibrato. It comes in once the note has sounded for 0.6 s with
//     nothing new played since (other notes within 150 ms of its start are
//     the same gesture); a little sooner (0.45 s) when the music has been
//     busy; later (0.9 s) and slower when it has been slow. In a MIDI
//     file the guitarist hears the band as well: another part's note near
//     this one (within an octave surely, an octave and a half as often as
//     not, two octaves never) is something happening too. How: the finger
//     pushes the string up along its fret and lets it back, five or six
//     times a second, so the pitch rises a few cents and returns (never
//     below the note). Mostly on the B and G strings, where the hand has
//     room; less on the high E (near the neck's edge) and the D; hardly on
//     the two lowest; more the higher up the neck, from about the fifth
//     fret (the string gives more there); never on an open string. Not
//     every note gets it, and each its own depth and pace. A new note ends it.
//     A note that comes by itself after a pause, with no other string held,
//     is one to make beautiful while waiting: the hand takes it where
//     vibrato is easy (the B or G string between the fifth and tenth fret,
//     never open), and it always gets it.
//   - With nothing else to do he also mends minor thirds: equal
//     temperament's is 16 cents narrow of the pure 6 : 5, and a finger can
//     only raise a note, so the upper note of a minor third (against another
//     string, or against what the band has just played under it) is pulled
//     up about 13 cents and held; not on an open string. So is the minor
//     seventh of a minor chord (its root and minor third sounding): it
//     belongs a pure fifth over that third, 18 cents above where the frets
//     put it, and is pulled up 17; a minor chord's major seventh 11 and
//     its major thirteenth 18; a major chord's ninth 4.
//   - A key let go: the finger lifts off its fret, or the hand lies on the
//     open string: the note is damped. The sustain pedal lets everything ring.
class GuitarPlayer : public Performer {
 public:
  explicit GuitarPlayer(GuitarModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned s) override { rng_ = 777u + 7919u * s; }

 private:
  double ur();
  void play();  // the notes that came together: strings, frets, the hand
  GuitarModel& m_;
  uint32_t rng_ = 777u;
  struct Asked { int midi, velocity; };
  std::vector<Asked> asked_;
  struct Due { double in; int string, fret; double depth, angle, nail, where; int strum; double clack; };
  std::vector<Due> due_;
  int note_on_[Guitar::kStrings];   // the MIDI note each string sounds (-1: none)
  bool held_[Guitar::kStrings] = {};  // its key still down
  double age_[Guitar::kStrings] = {};
  double hand_ = 2;                 // where the left hand is (fret)
  // The left hand's vibrato: how far it is in on each string (0 .. 1), its
  // phase, this note's own depth (cents) and rate (Hz); since the last note
  // was asked for.
  double vib_[Guitar::kStrings] = {}, vibPh_[Guitar::kStrings] = {}, vibCents_[Guitar::kStrings] = {}, vibHz_[Guitar::kStrings] = {};
  int fretOf_[Guitar::kStrings] = {};
  double sinceNote_ = 10, gapBefore_ = 10, vibrato_ = 1, pace_ = 0.6;
  int vibTick_ = 0;
  int lastMidi_ = -1, lastVel_ = 0, repeats_ = 0;  // the same note again and again
  // Strumming: the last strokes (how long ago, on which string), the run of
  // strings in one direction they make, and whether the hand is strumming
  // (and which way it went last: +1 down, from the bass strings to the treble).
  double strokeAgo_[3] = {10, 10, 10};
  int strokeString_[3] = {-1, -1, -1};
  double strumming_ = 0;
  int strumWay_ = 1;
  bool inRun_ = false;
  int barSlot(double beat, int* bar = nullptr) const;
  void noteStrum(int way);
  int thisBar_[16] = {}, lastBar_[16] = {}, bar_ = -100;  // the strums' ways by sixteenth, this bar and the last
  bool lastBarClear_ = false;
  double sameAsLastBar_ = 0.3, strumVel_ = 0.6;  // (the strums' velocity, evened over the last strings)
  double swing_ = 0.3, strumFree_ = 0.05;  // the strumming hand's swing from down to up (s); how often it goes against it
  double handSpeed_ = 1.4;  // across the strings (m/s)
  double mend_[Guitar::kStrings] = {}, pull_ = 1;  // a minor third's upper note pulled up (cents now)
  bool bent_[Guitar::kStrings] = {};
};

}  // namespace pajupilli
