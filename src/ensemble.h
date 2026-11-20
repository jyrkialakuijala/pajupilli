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

struct ChoirState;  // (the choir's dealing: src/choir.cc)
struct BrassState;  // (the brass section's: src/brasssect.cc)
struct BrassHint;

class Ensemble {
 public:
  struct Member {
    const Instrument* inst;
    const Calibration* cal;
    double x = 1e9;  // its place across the stage (m; 1e9: by its range, high on the left)
  };
  Ensemble(const std::vector<Member>& members, bool verbose, const RoomSpec* room = nullptr);
  // maxFlutes of one instrument.
  Ensemble(const Instrument& inst, const Calibration& cal, int maxFlutes, bool verbose,
           const RoomSpec* room = nullptr);
  void noteOn(int midi, int velocity);
  void noteOff(int midi);
  // A player setting, for every flute.
  void all(void (Performer::*set)(double), double x) {
    // (the brass section: a bend is each player's own fall, the dynamics its swell's base)
    if (brass_ && set == &Performer::setLipRoll) { brassBend(x); return; }
    if (brass_ && set == &Performer::setDynamics) { brassSetDynamics(x); return; }
    for (Slot& s : f_) (s.v->player().*set)(x);
    for (Section& k : kids_) k.ens->all(set, x);
  }
  // An instrument's own knob (Performer::setKnob): true if its players took it.
  bool instrumentKnob(int k, double x) {
    bool took = false;
    for (Slot& s : f_) took |= s.v->player().setKnob(k, x);
    for (Section& c : kids_) took |= c.ens->instrumentKnob(k, x);
    return took;
  }
  // (-play) Let a released note ring out: a new note takes the voice released
  // longest ago; a wind's the one released last if within a minor third (its
  // line keeps to its player). Live keeps its own choice.
  void setKeepTails(double secs) {
    keepTails_ = secs;
    for (Section& k : kids_) k.ens->setKeepTails(secs);
  }  // (how long a released note is taken to ring)
  // (-play) Velocity is loudness, as a General MIDI player's: each note's
  // voice is turned up or down on its instrument's curve (velocityGainDb), as
  // the listener's gain (the room's air is not). Live keeps the physics alone.
  void setVelocityGain(bool on) {
    velGain_ = on;
    for (Section& k : kids_) k.ens->setVelocityGain(on && !brass_);  // (the brass section applies its own)
  }
  // (-play) The director's cue for the key that follows (Performer::direct).
  void direct(const Cue& c) {
    if (brass_) brassCue(c);
    for (Slot& s : f_) s.v->player().direct(c);
    for (Section& k : kids_) k.ens->direct(c);
  }
  void setHold(bool h) {
    for (Slot& s : f_) s.v->player().setHold(h);
    for (Section& k : kids_) k.ens->setHold(h);
  }
  void render(float* left, float* right, int n);  // 48 kHz, mixed, stereo
  void render(float* out, int n);                 // ... mono
  int resets() const;
  int lines() const { return lines_; }
  int size() const { return int(f_.size()); }
  Performer& player(int i) { return f_[i].v->player(); }  // (an organ's plan talks to its player)
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
    double vg = 1, vgTo = 1, vgTau = 0.005;  // velocity's gain: now, its goal, how fast (s)
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
  void retune();  // the pajupilli intonation among the sounding flutes (adaptive just)
  void block(float* left, float* right, int n);

  static constexpr double kChord = 0.035, kSplit = 0.10, kForget = 2.0, kIdle = 1.0;
  static constexpr double kFresh = 0.25;  // silent this long after a release: a fresh start
  std::vector<Slot> f_;
  // The orchestra hit (src/orchhit.cc): no voices of its own, but sections (each an ensemble of
  // one instrument's players) that every key is scored for.
  struct Section {
    std::unique_ptr<Ensemble> ens;
    std::vector<int> offs;  // its notes: semitones from the key
    int lo, hi;             // ... folded by octaves into this range
    float gain;
    bool stops;             // its players stop with the eighth (not the piano, the drum)
    bool run = false, roll = false;  // a grace run up to its note; a rolled chord
    double lead = 0;        // its players act this long after a key (noteLead)
  };
  struct HitEvent { double t; int section, pitch, vel; bool on; };
  struct Hit {
    int key;
    double tuttiAt;         // when the chord lands
    double offAt = -1;      // the key let go early: when the hit ends
    bool stopped = false;   // the eighth is over
    std::vector<HitEvent> ev;  // by time
    size_t next = 0;        // the first not yet played
  };
  void initOrchestra(const RoomSpec* room);
  void hitOn(int key, int velocity);
  void hitOff(int key, bool all, bool force);
  void hitRender(float* left, float* right, int n);
  bool quiet_ = false;  // (a section of the hit: says nothing of its tuning or who plays)
  // The string ensembles (src/stringsect.cc): one ensemble of 24 players (kids_[0]), each
  // player's key dealt by the section.
  struct SectPlayer {
    int slot, sec;
    int cur, want;   // the key it plays; the key it is to play (-1: none)
    double at;       // ... from when
    int vel;
    bool late;       // (a double bass: held back by the others' pizzicato wait)
    double habit;    // its seat's own lateness (s)
  };
  void initStrings(const Instrument& inst, const RoomSpec* room);
  void drive(int slot, int pitch, int velocity, bool on);
  void sectAssign();
  void sectOn(int key, int velocity);
  void sectOff(int key);
  void sectRender(float* left, float* right, int n);
  bool strings_ = false, slow_ = false, tremolo_ = false, pizzOnly_ = false;
  std::vector<SectPlayer> sp_;
  // The choir (src/choir.cc, src/choir.h): one ensemble of singers (kids_[0]), each singer's key
  // dealt by its voice part.
  void initChoir(const Instrument& inst, const RoomSpec* room);
  void choirAssign();
  void choirTune();  // (the chord tuned by the spectra: src/choir.cc)
  void choirOn(int key, int velocity);
  void choirOff(int key);
  void choirRender(float* left, float* right, int n);
  std::shared_ptr<ChoirState> choir_;
  // The brass section (src/brasssect.cc, src/brasssect.h): one ensemble of players (kids_[0]), the
  // held keys voiced and dealt to its seats.
 public:
  void setBrassMode(int mode);           // (-play: the section the part stands for, before it plays)
  void brassHint(const BrassHint& h);    // (-play: what the song says about the next key)
  void brassExpression(double x);        // (-play: CC 11, 0..1)
 private:
  void initBrass(const Instrument& inst, const RoomSpec* room);
  void brassBuild(int mode);
  void brassCue(const Cue& c);
  void brassBend(double x);
  void brassSetDynamics(double d);
  void brassOn(int key, int velocity);
  void brassOff(int key);
  void brassAssign(bool onset);
  void brassRender(float* left, float* right, int n);
  std::shared_ptr<BrassState> brass_;
  bool mixFixed_ = false;  // (a section's players: their sum not evened to one player's level)
  double jiScale_ = 1;     // (the pajupilli intonation's share: the brass section's players lean to equal temperament)
  int sheld_[128] = {};  // the held keys' velocities (0: up)
  unsigned srng_ = 2026;
  std::vector<Section> kids_;
  std::vector<std::unique_ptr<Calibration>> kidCals_;
  std::vector<Hit> hits_;
  std::vector<float> hitL_, hitR_;
  bool mixed_ = false;
  double keepTails_ = 0;
  bool velGain_ = false;
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
  bool ji_;                 // PJ_JI (1): the pajupilli intonation (adaptive just)
  long long tuneSig_ = -1;  // the sounding set last tuned for
  std::vector<std::vector<float>> buf_;
  // Three reverb channels, each a room of its own: small (the synth
  // basses), main (the room chosen; everyone else), gated (a big live room,
  // its ambience compressed and gated: the toms and drums, a snare).
  // A source sounds into its channel's room and hears that room back.
  std::unique_ptr<Room> rooms_[3];
  std::vector<int> roomSlots_[3];
  bool room_ = false;  // in rooms at all (not dry)
  static int reverbChannel(const Instrument& in);
  float roomFb_ = std::getenv("PJ_ROOMFB") ? float(std::atof(std::getenv("PJ_ROOMFB"))) : 1.0f;  // room into the bores
  std::vector<float> srcBuf_;  // physical pressures for the room
  std::vector<float> dipBuf_;  // ... and the sources' directional parts
  std::vector<float> qL_, qR_; // rendered, not yet handed out
  size_t qPos_ = 0;
};

}  // namespace pajupilli
