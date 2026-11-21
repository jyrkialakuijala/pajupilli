// The guitars as instruments (-inst nylon-guitar; clean-guitar, GM 28; muted-guitar, GM 29;
// overdriven-guitar, GM 30): the model (guitar.h) behind the Tube and Performer interfaces, and
// the guitarist. An electric's pickups go into an
// amp (guitaramp.h): what is heard is the amp's speaker.
#pragma once

#include <algorithm>
#include <vector>

#include <memory>

#include "guitar.h"
#include "guitaramp.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool guitarInstrument(const Instrument& inst);
bool guitarHarmonics(const Instrument& inst);  // GM 32: the clean Strat played in harmonics
bool jazzGuitarInstrument(const Instrument& inst);  // GM 27: the ES-175 (src/jazzguitar.h's player)

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
  int nanSeen_ = 0;  // g_.nanResets() already answered with a resetAir()
  float strings();   // g_.sample(), with the tests' kick in and a caught NaN answered
  std::unique_ptr<GuitarAmp> amp_;
  float kAmpOut_ = 1;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
  // (GM 27: the archtop's radiated sound mixed under the amp; its scale: Pa to the amp's units,
  // ~20 dB under it on a chord, A-weighted: research/jazz_guitar.md 2.5.)
  float kAcoustic_ = 0;
  static constexpr double kAcousticCal_ = 0.083;
 public:
  GuitarAmp* amp() { return amp_.get(); }
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
// On an electric (research/electric_guitars.md 7) the same left hand, and a pick in the right:
//   - Single notes are picked down and up in turn while they come quickly (under 0.3 s apart),
//     down again after a rest; an up stroke a little lighter and the other way across the
//     string. Notes that come together are one sweep of the pick across the strings (6 ms a
//     string), down or up as the strumming hand swings (above).
//   - Velocity is how far the pick pushes the string (0.1-3 mm), at 11 cm from the saddle (the
//     brightness knob moves it: towards the bridge, brighter); a hard stroke a little nearer
//     the bridge and quicker off the pick.
//   - On the muted guitar (GM 29) the side of the picking hand lies on the strings 8-16 mm from
//     the saddle as each one is picked (8.1): the note thuds, its overtones soon gone,
//     its fundamental to -30 dB in about a quarter of a second; the harder the stroke the lighter
//     and nearer the saddle (an accent rings), now and then lighter by itself, never twice the
//     same. The pick strikes nearer the bridge (~6 cm).
//   - On the overdriven guitar the hands keep the strings not played muted, and a loud note held
//     alone (or in a dyad or power chord) sings: the player stands where the amp's sound comes
//     back in phase with it (feedback). From 0.12 s it holds (its modes below 2.5 kHz); at
//     1.2-2 s it blooms: a low note
//     into its octave or twelfth, a middle one swells or goes to its octave, a high one swells;
//     as long as the key is down (PJ_EGTR_FEEDBACK: 0 none, x the bloom; PJ_EGTR_BLOOM: 2,
//     how far it swells).
//   - Pitch bend is the left hand pushing a string: the last note picked bends, the rest of a
//     chord stays where it is. The fretting hand lifts off a released note and lies on the
//     string at once (an electric player mutes what is not playing).
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
  struct Due { double in; int string, fret; double depth, angle, nail, where; int strum; double clack, palm = 0, palmAt = 0.012; };
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
  // The electric's pick: which way it goes next (+1 down), and the string a pitch bend takes.
  bool electric_ = false, muted_ = false, driven_ = false, distorted_ = false;  // (distorted_: GM 31, driven_ too)
  // The steel-string acoustic (GM 26; research/steel_guitar_build.md 8.2): a flatpick as the
  // electric's, over the soundhole; its strums as measured; open strings left to ring.
  bool steel_ = false;
  bool idleMuted_[Guitar::kStrings] = {};  // (steel: an unplayed string the hand lies on after the last key's release)
  // GM 31's palm on chugs: the last stroke's lowest note, its velocity, how long its key was
  // held (s; -1: still), how many strokes on that note so far; the knobs (palm rule, octave on a
  // power chord, the lead vibrato's width).
  int pmRoot_ = -1, pmVel_ = 0, pmRun_ = 0;
  double pmHeld_ = -1, chug_ = 1, octave_ = 1, wide_ = 1;
  bool octaveAdded_[Guitar::kStrings] = {};  // (that string's note the added octave: let go with its root)
  int pickWay_ = 1, bendString_ = -1;
  double kPalm_ = 5, kRelease_ = 25;
  // Feedback: whether each held string is being fed, its key's velocity, when it blooms (s).
  double feedback_ = 1, holdAt_ = 0.12, holdHz_ = 2500, bloomGain_ = 2, bloomAt_[Guitar::kStrings] = {};
  int fed_[Guitar::kStrings] = {};  // (0 not yet, 1 held, 2 bloomed, 3 the fundamental let go)
  int bloomK_[Guitar::kStrings] = {};
  int velOf_[Guitar::kStrings] = {};
};

// GM 32, Guitar Harmonics (research/electric_guitars.md 2.6, 9): the clean Stratocaster on its
// middle pickup (the neck pickup sits on the open strings' 4th-harmonic node), into the Twin.
// Every note is a harmonic, at the pitch written:
//   - Where: a natural harmonic if there is one (the open string touched at the 12th fret,
//     n = 2; the 7th, n = 3; the 5th, n = 4), the free strings first; else a touched
//     ("harp") harmonic, n = 2: the left hand stops the string 12 frets under the note (near
//     where it already is) and the right hand's index finger touches 12 frets higher; above
//     the 21st fret's octave n = 3, then n = 4; below E3 an octave up, above C8 an octave down.
//     The harmonic sounds what the string's n-th mode sounds: its stiffness makes it 0.1-3
//     cents sharp of n times the fundamental, and that is left as it is (as on a guitar).
//   - How: the fingertip on the node from just before the pick to 30-60 ms after (it kills what
//     is not a multiple of n), then lifted; the pick near the bridge, at the harmonic's own
//     antinode nearest it (never on its node), velocity its depth. A harmonic is weaker the
//     higher n is (the pluck's share of mode n, and the pickup's ear for it): the player digs
//     in for the 5th fret's.
//   - They ring past the key's release, as harmonics do. A string needed again at another fret
//     is damped first (4 ms; every note waits those 4 ms, so the timing stays even); after a
//     rest (1.5 s with no key down) the hand lies lightly on the released strings.
//   - Notes that come together are one sweep of the pick from the low strings up, 8 ms apart.
class HarmonicsPlayer : public Performer {
 public:
  explicit HarmonicsPlayer(GuitarModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override { hold_ = h; }
  void seed(unsigned s) override { rng_ = 919u + 7919u * s; }
  // Where pitch `midi` is played: string, fret, harmonic (false: nowhere; `taken`: strings in use).
  bool place(int midi, const bool* taken, int* string, int* fret, int* n) const;

 private:
  double ur();
  void play();
  GuitarModel& m_;
  uint32_t rng_ = 919u;
  struct Asked { int midi, velocity; };
  std::vector<Asked> asked_;
  struct Due { double in; int string, fret, n; double depth, where, angle, rise, touch, off; };
  std::vector<Due> due_;
  int note_on_[Guitar::kStrings];      // the MIDI note each string sounds (-1: none)
  int fretOf_[Guitar::kStrings] = {}, nOf_[Guitar::kStrings] = {};
  bool held_[Guitar::kStrings] = {}, rested_[Guitar::kStrings] = {};
  double age_[Guitar::kStrings] = {};
  double hand_ = 5, sinceNote_ = 10, gap_ = 10, rate_ = 300, lift_ = 1;
  int pickWay_ = 1, shift_ = 0;
};

}  // namespace pajupilli
