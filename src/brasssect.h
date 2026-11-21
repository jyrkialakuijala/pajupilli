// The brass section (General MIDI 62 "Brass Section", -inst brass-section;
// research/brass_section.md): a section of brass players, voiced as an arranger would, played
// by the physical trumpets, trombones, horns and tuba (src/horn.cc, src/hornplayer.cc). Nothing
// is sampled or chorused.
//
// Three make-ups (research 4, 11.1), chosen from the part and the song (planBrassSection) or by
// PJ_BRASS_MODE (pop, bigband, orchestral):
//   pop / soul / funk   3 trumpets, 2 trombones                       (Jerry Hey's, trumpet-heavy)
//   big band            4 trumpets, 4 trombones (the 4th a bass trombone)
//   orchestral          2 trumpets, 4 horns, 3 trombones, tuba
// The seats are ordered from the top (tp1, the lead) down; a voicing never crosses them.
//
// The voicer (BrassVoicer) turns the held keys into one note per seat:
//   - The keys are the harmony; the top key is the lead and trumpet 1 plays it whenever it can.
//   - Three or more keys: every key is sounded, no key moved by an octave; the spare players
//     double the lead first, then the bottom, never a third low down (under C4); the trumpets
//     take the top keys, the horns the middle, the trombones and the tuba the bottom.
//   - One key: a unison of those at ease on it (pop: the trumpets from C4 up, the trombones to
//     B-flat 4; the tuba never above F3). In a unison line (a run of single notes) whose lead is
//     at G4 or above, the trombones double it an octave down (the fanfare and the soul line),
//     unless the part writes its own octaves.
//   - A sparse part (single notes, a third or a sixth) is harmonised only where the song says
//     what the chord is (the other channels' notes, planBrassSection) and the line fits it: its
//     notes chord tones or the tensions an arranger would keep (9, 11 on minor, 13). Pop: a held
//     note (0.3 s or more) blooms into three-part close harmony under the lead (on seventh chords
//     the 3, 5 and 7, the root left to the bass), the lead doubled an octave down; the short notes
//     stay in unison. Big band: four-way close (the lead at D5 and above; a triad sounds as a 6th
//     chord, a 9 or 13 in the lead replaces the root or the fifth) or drop-2 (lower), the trumpets
//     on the four parts and the trombones on the same an octave down; a passing note between
//     two chord notes moves the whole voicing with it (planing). Orchestral parts are never
//     harmonised.
//   - Practical limits: each seat in its range, its comfortable register weighed; no close
//     interval below its low interval limit (a minor second under G3, a major second under F3,
//     a minor third under F3, a major third under E3, a fourth under B2 ...); no minor ninth
//     against the lead; the smallest motion from the last voicing (a common tone held).
//   - With -v (or PJ_BRASS_SHOW=1) every voicing is printed ("brass: ..."), as the strings deal.
//
// The playing (Ensemble::initBrass, brassAssign, brassRender in src/brasssect.cc):
//   - Onsets (research 6.1): the lead on the key; on a stab the others 3 +- 6 ms after it, each
//     seat's habit (+-3 ms) and its section's moment (+-4 ms) on top; a soft, long entry 10 +- 15
//     ms. Releases: a stab's within ~5 ms (the tongue), a long note's over ~25 ms.
//   - Pitch (6.2): each seat a few cents off (uniform +-4 c, HornPlayer::vary; the lips aim
//     there, so the players beat as a section does); the held chord tuned by its partials as the
//     organ's keys are (Ensemble::retune, spectral_): those that nearly meet pull the interval
//     to where they do, so the overtones lock. Each player's ear holds its aim within a cent.
//   - Each player and instrument its own (HornPlayer::vary, Horn::vary): the attack's push and
//     its decay, the lips' Q, the cup's volume, the brassiness.
//   - Vibrato (6.3): none under 0.6 s; then the lead a little (x0.5 of the director's depth), the
//     other trumpets less (x0.2) and later, the horns, trombones and tuba none; none in a unison.
//   - Balance (6.4): the lead 1.5 dB on top, every seat's velocity a little its own (+-4); a key
//     sounds about as one trumpet does (+2 dB), the harmony's added voices softer, so a voiced
//     chord adds up as a sampled section's keys do.
//   - Falls and doits only from the file's pitch bend: each player follows it a moment later
//     (0-30 ms) and at its own speed: a smear, not one bend for all. Nothing is invented.
//   - A swell (fp<, a CC 11 ride inside a held note) moves the players' breath as well as the
//     level: the brass gets darker and brighter with it, not only softer and louder.
//   - Seating (8): pop, trumpets left of centre and trombones right; big band, two rows across;
//     orchestral, horns left of centre, trumpets in the middle, trombones and tuba to the right.
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "instrument.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

class Ensemble;

enum BrassMode { kBrassPop = 0, kBrassBigBand = 1, kBrassOrchestral = 2 };
enum BrassKind { kBrTrumpet, kBrHorn, kBrTrombone, kBrTuba };

// A seat: its instrument, label, kind, range and comfortable range (sounding MIDI), its place
// across the stage (m, left negative).
struct BrassSeatDef {
  const char* inst;
  const char* label;
  BrassKind kind;
  int lo, hi, easyLo, easyHi;
  double x;
};
const BrassSeatDef* brassSeats(int mode, int* n);
const char* brassModeName(int mode);

// What the song says about a key (planBrassSection; or nothing: the keys alone).
struct BrassHint {
  int root = -1;          // the harmony's root (pitch class; -1: not known)
  int quality = -1;       // its kind (kBrassChords)
  bool harmonise = false; // a sparse onset here may be voiced
  bool line = true;       // a unison line (the octave below may double it)
  bool octaves = true;    // the part does not write its own octaves
  double len = -1;        // the note's length (s; -1: not known)
};
// A chord symbol ("Dm7", "G7", "Cmaj7", "F#m7b5", "Bb6", "C"): root and quality; false if not one.
bool parseBrassChord(const std::string& s, int* root, int* quality);
std::string brassChordName(int root, int quality);

// The voicer: the held keys to one note per seat (-1: rests).
class BrassVoicer {
 public:
  explicit BrassVoicer(int mode);
  // keys: the held keys (any order); prev: each seat's note before (-1: resting).
  std::vector<int> voice(const std::vector<int>& keys, const BrassHint& h, const std::vector<int>& prev,
                         std::string* how = nullptr) const;
  int seats() const { return n_; }
  const BrassSeatDef& seat(int i) const { return s_[i]; }
  int mode() const { return mode_; }

 private:
  // A note to deal: its key; not one of the given keys (the voicing's); an octave under the lead
  // (the low brass's doubling, never the trumpets'); a third low down (not doubled).
  struct Key { int k; bool added; bool lowThird; bool octave = false; };
  double deal(const std::vector<Key>& keys, const std::vector<int>& prev, const std::vector<bool>* unison,
              std::vector<int>* out) const;
  bool harmonise(int lead, int other, const BrassHint& h, const std::vector<int>& prev, std::vector<int>* out,
                 std::string* how) const;
  std::vector<int> compact(std::vector<int> o) const;
  int mode_, n_;
  const BrassSeatDef* s_;
};
// The low interval limits: the lowest note of an interval of d semitones (1..11) that is not mud.
int brassLowLimit(int d);

// -play: the part read once with the rest of the song (research 4.3): which section it stands
// for, and per onset what the song's harmony is there and whether it may be voiced.
struct BrassNoteIn {
  double on, off;  // s
  long tick;
  int ch, key, vel, prog;
  bool self;       // this part's own note
};
struct BrassPlan {
  int mode = kBrassPop;
  bool harmonise = false, octaves = true;
  std::string summary;
  std::vector<std::pair<std::pair<int, long long>, BrassHint>> hints;  // ((key, on in us), hint), sorted
  const BrassHint* find(int key, double on) const;
};
BrassPlan planBrassSection(const std::vector<BrassNoteIn>& notes, int div, int style);

// -brassvoice "SPEC": the voicer alone, for tests and listening: steps separated by ';', each
// "[mode pop|bigband|orchestral] [chord SYM] [line|noline] KEY ..." (MIDI keys). Prints each voicing.
int brassVoiceDemo(const std::string& spec);

bool brassSectionInstrument(const Instrument& inst);

// The section as one voice (-inst brass-section -note, -seq, -fuzz: a bare Voice): the whole
// section inside, dry. (-play and live make an Ensemble of it, which deals the players itself in
// its room.)
class BrassSectionModel : public Tube {
 public:
  explicit BrassSectionModel(const Instrument& inst);
  ~BrassSectionModel() override;
  Ensemble& ensemble() { return *ens_; }
  double testResponse() const override { return 0; }

 private:
  void controlStep() override {}
  float sample() override { return 0; }
  std::unique_ptr<Ensemble> ens_;
};

class BrassSectionPlayer : public Performer {
 public:
  explicit BrassSectionPlayer(BrassSectionModel& m) : Performer(m), m_(m) {}
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}

 private:
  BrassSectionModel& m_;
};

}  // namespace pajupilli
