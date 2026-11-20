// The concert harp as an instrument (-inst harp; General MIDI program 47, Orchestral Harp;
// research/harp.md): the model (harp.h) behind the Tube and Performer interfaces, and the harpist.
#pragma once

#include <vector>

#include "harp.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool harpInstrument(const Instrument& inst);

class HarpModel : public Tube {
 public:
  explicit HarpModel(const Instrument& inst);
  Harp& harp() { return h_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  Harp h_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

// The harpist (research/harp.md 4-7). MIDI gives pitches; a harp wants a string and its pedal
// for each, and two hands of four fingers (no little finger) to play them.
//   - The pedals: seven, one for each letter in every octave (C1, D1 and G7 have no discs: tuned
//     by hand to their naturals). Set to the song's key when it is known (C major otherwise), they
//     move when a note needs it: a pitch is played on the string that already gives it if there is
//     one (E sharp on the E string or F natural on the F, whichever is set); if none does, on the
//     letter whose pedal moving disturbs least (not one whose strings are ringing, else the
//     natural spelling). A key struck again soon after on a string still sounding goes to its
//     enharmonic twin if the pedals give one (repeated notes, trills, bisbigliando); otherwise the
//     finger lands on the sounding string, stops it and plucks it again. A ringing string whose
//     pedal must move is damped first.
//   - The hands: the left takes the bass, the right the treble, each at most four notes; in a hand
//     the thumb takes the highest note (on a harp the thumbs are on top), the fingers those below.
//   - The pluck (5.1): the finger pad in the middle of the string (0.37 of its length from the
//     board in the bass to 0.42 at the top), 0.5 mm x 18^(velocity / 127) aside, mostly in
//     the strings' plane (the thumb 17 degrees off it, the fingers 20-26, each pluck its own), held
//     and let slip in 2-3.5 ms (quicker when loud) carrying the string 0.3-1.5 mm further (a string
//     shorter than 17.6 cm, the shortest Chadefaux measured, drawn out as much of its length as
//     that one). Near the board (près de la table: 0.06-0.12 of the length, brittle and
//     guitar-like) when the brightness control asks for it (from 1.6).
//   - A glissando's strings, plucked by a finger sliding across: in the plane, hardly stopping,
//     lighter (x0.7).
//   - Chords of three or more notes are rolled, upwards from the bass, 18 ms (loud) to 30 ms
//     (soft) from string to string, unless they come quickly one after another (under 0.25 s
//     apart: played together, as an accompaniment's chords are); two notes are played together.
//   - Glissando: single notes coming under 60 ms apart, each 1-4 semitones on in one direction,
//     are a glissando from the third on: the finger slides across the strings without stopping
//     on them, every string between two notes of the run sounds (the strings the run passes over
//     set, if their pedals are free, to sound notes of the run: a glissando over the run's
//     pitches, 4-7 to the octave).
//   - Harmonics: a lone note at velocity 20 or less, sounding between E3 and A6, is played as the
//     octave harmonic of the string an octave below (the side of the hand at its middle, the thumb
//     plucking, the hand lifted as it lets go).
//   - Damping (6.4): the strings ring on when the keys are let go; the hands damp them at rests
//     (no key held for a beat: the palms on the strings below C5), when the bass moves to a new
//     harmony (the left hand stops the bass strings it played that are not in it), and a note let
//     go within 0.1 s of its pluck, not in a quick passage, is étouffé (the finger put back on it).
//     A string a pedal must move is damped first if it rings loud. The sustain pedal: laissez
//     vibrer (no damping at all). At a general pause (the song's end) every string is damped.
class HarpPlayer : public Performer {
 public:
  explicit HarpPlayer(HarpModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned s) override { rng_ = 4703u + 7919u * s; }

  // For tests: which string a key went to and its pedal; whether the last gesture was a gliss.
  int stringOf(int midi) const { return keyString_[midi]; }

 private:
  double ur();
  void play();
  int choose(int midi, const std::vector<int>& taken, int* pos);
  void setPedal(int letter, int pos, int except);
  void dampStrings(int below, double rate, double minEnergy, int except = -1);
  HarpModel& m_;
  uint32_t rng_ = 4703u;
  double now_ = 0;
  struct Asked { int midi, velocity; };
  std::vector<Asked> asked_;
  struct Due { double at; int string; Harp::Pluck p; };
  std::vector<Due> due_;
  struct Lift { double at; int string; };
  std::vector<Lift> lifts_;
  int keyString_[128];
  int heldBy_[Harp::kStrings];       // the key holding each string (-1: none)
  double pluckedAt_[Harp::kStrings];
  int pluckedPitch_[Harp::kStrings];
  bool leftHand_[Harp::kStrings] = {};
  double letterAt_[7] = {-10, -10, -10, -10, -10, -10, -10};  // when a string of each letter was last plucked
  bool shortKey_[128] = {};          // a key let go before it was played
  double lastGesture_ = -10, lastEvent_ = -10;
  bool restDamped_ = true, endDamped_ = true;
  // The glissando: the recent single notes (time, key, string), the run's direction and pitches.
  struct Recent { double t; int midi, string; };
  std::vector<Recent> recent_;
  int glissDir_ = 0;
  bool glissPc_[12] = {};
  int pedalsMoved_ = 0;  // (this gesture's)
  double rollStep_ = 1, harmVel_ = 20, rest_ = 1;
  bool keyed_ = false;   // the pedals set to the song's key
  bool verbose_ = false;
};

}  // namespace pajupilli
