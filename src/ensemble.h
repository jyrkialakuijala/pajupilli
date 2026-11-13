// Polyphony: a pool of flutes (each its own model and player) that the keys are
// dealt to. A heuristic guesses how many lines are being played:
//   - keys struck together (within kChord) are a chord: one flute each;
//   - a key pressed while another is held is a slur on that flute (as with one
//     flute), unless more lines are expected than flutes are holding keys;
//   - keys held together on one flute longer than kSplit (100 ms) are separate
//     lines: each older note moves to a free flute from the pool, which takes
//     it up afresh;
//   - the number of lines expected is the most seen lately, and drops back
//     after kForget seconds with fewer.
// A new line goes to the free flute that played nearest in pitch and most
// recently, so lines keep to their flutes. With one flute this is exactly the
// single-flute behaviour.
//
// An ensemble (a mixed pool: by default bass, alto, two flutes and piccolo):
// each instrument plays only its own range: a note no free instrument
// reaches takes the nearest busy one that does (never another an octave off).
// A note goes where it sits comfortably (the piccolo's top fifth and the others' strained tops cost
// more; the flutes lead when several fit); a chord is dealt as a whole, low
// notes to low instruments, without voices crossing (keys struck together
// within kChord may move to a better instrument as the chord arrives); a line
// keeps to its instrument.
//
// Tuning: flutes sounding together move a few cents off equal temperament
// towards just intervals (their overtones meet instead of beating), each by
// ear with its lips. Every pair asks for its interval's nearest simple ratio
// (fifth 3/2 +2 c, major third 5/4 -14 c, minor third 6/5 +16 c, ...), the
// simpler and closer the harder; a least-squares solve gives each flute its
// offset. Notes younger than 150 ms are still being placed: if they are fewer
// than the notes already sounding, those stay where they are and the young fit
// them (the chord does not slide, a progression does not drift); if they are
// as many or more (a chord struck or rolled, a new chord over one held note),
// they are placed together with their mean on equal temperament, and the
// older minority moves to fit them. Solved when the sounding set changes, and
// every 100 ms while it does not: the notes heal towards 0 c (time constant
// 3 s, PJ_JIHEAL), as far as their intervals let them; a note alone, or with
// no harmonic reason to be off, ends at 0 c.
//
// The room (room.h): the instruments sit in it, sound into it and hear it in
// their bores, the silent ones too. Without a room the silent flutes are not
// computed.
#pragma once

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "calibrate.h"
#include "room.h"
#include "voice.h"

namespace pajupilli {

class Ensemble {
 public:
  struct Member {
    const Instrument* inst;
    const Calibration* cal;
  };
  Ensemble(const std::vector<Member>& members, bool verbose, const RoomSpec* room = nullptr);
  // maxFlutes of one instrument.
  Ensemble(const Instrument& inst, const Calibration& cal, int maxFlutes, bool verbose,
           const RoomSpec* room = nullptr);
  void noteOn(int midi, int velocity);
  void noteOff(int midi);
  // A player setting, for every flute.
  void all(void (Performer::*set)(double), double x) {
    for (Slot& s : f_) (s.v->player().*set)(x);
  }
  void setHold(bool h) {
    for (Slot& s : f_) s.v->player().setHold(h);
  }
  void render(float* left, float* right, int n);  // 48 kHz, mixed, stereo
  void render(float* out, int n);                 // ... mono
  int resets() const;
  int lines() const { return lines_; }
  int size() const { return int(f_.size()); }
  int lo() const { return lo_; }  // sounding range of the whole ensemble
  int hi() const { return hi_; }
  std::string describe() const;

 private:
  struct Slot {
    std::unique_ptr<Voice> v;
    std::string label;       // "flute 2"
    int lo, hi;              // sounding range
    int easyLo, easyHi;      // comfortable range
    int rank;                // lowest note: lower instruments play lower voices
    double pref;             // cost of choosing it (the flutes lead)
    int held = 0;            // keys down on this flute
    int lastNote = -1;
    double lastOff = -1e9;   // when its last key went up (s)
    bool active = false;     // being rendered
    int tunedNote = -1;      // the note its tuning was set for
    int ageNote = -1;        // the note sounding, and since when (s): its age
    double noteAt = -1e9;
    double cents = 0;        // its offset from equal temperament
    double gl = 1, gr = 1;   // its pan (no room): gains left and right
    std::vector<float> room; // the room's pressure here, next block
  };
  void init(const std::vector<Member>& members, const RoomSpec* room);
  int fold(int f, int key, int* octaves) const;  // the key in flute f's range (a pool), -1 (an ensemble)
  int reach(int key) const;
  double cost(int f, int pitch, int octaves) const;
  double crossing(int f, int pitch, int g, int q) const;
  int pickFree(int key, int* pitch) const;  // a flute with no keys down, or -1
  int nearestHeld(int key) const;
  int holding() const;  // flutes with keys down
  bool deal(int key, int velocity);  // a mixed pool: deal a chord as a whole
  void split();
  void take(int flute, int key, int pitch, int velocity);
  void release(int key);
  void retune();  // just intonation among the sounding flutes
  void block(float* left, float* right, int n);

  static constexpr double kChord = 0.035, kSplit = 0.10, kForget = 2.0, kIdle = 1.0;
  static constexpr double kFresh = 0.25;  // silent this long after a release: a fresh start
  std::vector<Slot> f_;
  bool mixed_ = false;
  bool panned_ = false;  // a stereo mix by each voice's pan (no room)
  double healAt_ = 0;    // when the tuning last healed (s)
  int lo_ = 127, hi_ = 0;
  int owner_[128];
  int pitch_[128];
  double pressT_[128] = {};
  int vel_[128] = {};
  double now_ = 0, calm_ = 0, mix_ = 1;
  int lines_ = 1;
  std::string playing_ = "none";  // last printed: "2 x flute, piccolo"
  bool verbose_;
  bool ji_;                 // PJ_JI (1): adaptive just intonation
  long long tuneSig_ = -1;  // the sounding set last tuned for
  std::vector<std::vector<float>> buf_;
  // Three reverb channels, each a room of its own: small (the drums, the
  // synth basses), main (the room chosen; everyone else), gated (a big room
  // behind a gate: for the snare).
  // A source sounds into its channel's room and hears that room back.
  std::unique_ptr<Room> rooms_[3];
  std::vector<int> roomSlots_[3];
  bool room_ = false;  // in rooms at all (not dry)
  static int reverbChannel(const Instrument& in);
  float roomFb_ = std::getenv("PJ_ROOMFB") ? float(std::atof(std::getenv("PJ_ROOMFB"))) : 1.0f;  // room into the bores
  std::vector<float> srcBuf_;  // physical pressures for the room
  std::vector<float> qL_, qR_; // rendered, not yet handed out
  size_t qPos_ = 0;
};

}  // namespace pajupilli
