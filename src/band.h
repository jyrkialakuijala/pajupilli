// What the band is sounding (research/fretless_bass.md 7.4, 8.5): a registry of the notes the
// other channels hold or let ring, kept by -play's event loop (src/midifile.cc) between blocks and
// read by a player at the start of its block (no locking: the two never overlap). Each note with
// its channel, its pitch (as played), its tuning (cents; a fixed-pitch part's 0), its level (from
// its velocity, falling at its family's rate while it rings) and its family's spectrum template.
//   - On: a note-on adds it. Off: a note-off takes it away, unless the channel's sustain pedal is
//     down (then it rings on, falling, until the pedal lifts) or the family rings by itself (a
//     harp's, a bell's). Below -40 dB it leaves.
//   - The templates (bandPartial): a piano's, a plucked string's 1/k; an organ's, the bowed
//     strings' 1/k, held; the choir's as Ensemble::choirTune has it; a flute's first three
//     partials strong; brass 1/sqrt(k) to 2 kHz; the reeds 1/k, the clarinet's even partials
//     weak below its bore's cutoff (~1.5 kHz: -20 dB, Benade's lattice cutoff; above it the
//     evens back at about 0.6 of the odd ones'); mallets their fundamental; the fretless's own
//     from FluidR3's recorded GM 36.
// The parts that tune by ear (the fretless bass, src/fretless.cc; the brass and the reeds,
// Ensemble::windEar) hear it through bandPull (below), and write back where they have each note
// (setCents), so the others hear them as played.
// The order of hearing (research/fretless_bass.md 7.6): a note of a part that tunes by ear (byEar)
// is heard only by the notes that came after it, more than 35 ms later; of notes begun within
// 35 ms of each other (a chord struck together) the lower is heard by the higher (a unison: the
// lower channel by the higher). The newcomer fits in, as within a section (Ensemble::retune: the
// young fit the old). Who hears whom is then a fixed order, never a loop: two winds cannot chase
// each other round (A to B's -14 c, B to A's, ...), the first of them hears only the fixed-pitch
// parts (or nothing, and heals to 0 c). A chain of them, though, a common tone held over each
// change (the comma pump: C Am Dm G C, 21.5 c a round if every interval is just), would carry
// its offset on to the clamp; the winds' common healing (drift) brings their mean back to equal
// temperament, as far as they are alone (with a piano sounding, the piano holds them).
#pragma once

#include <vector>

namespace pajupilli {

enum BandFamily {
  kBandPiano, kBandPluck, kBandMallet, kBandBell, kBandOrgan, kBandChoir, kBandFlute,
  kBandStrings, kBandBrass, kBandReed, kBandClarinet, kBandBass, kBandChime, kBandOther, kBandNone
};

struct BandNote {
  int ch, key;        // the channel, the key as it came (the entry's name)
  int pitch;          // as played (the part's transposition included)
  double cents = 0;   // its tuning against equal temperament
  double level = 1;   // amplitude (1: velocity 127, new)
  double fall = 0;    // dB/s while it rings (0: held level)
  int fam = kBandOther;
  bool down = true;   // its key still down
  bool kept = false;  // let go under the sustain pedal (or a family that rings on)
  bool rings = false; // rings on after its key, whatever its family (SoundingSet::on)
  double at = 0;      // when it began (s, the set's clock)
  bool byEar = false; // its part tunes it by ear (heard in order: above)
  bool wind = false;  // ... a wind's (its cents in the winds' common healing: drift())
  double earShare = 0;  // (a wind's) how much of its pull comes from other parts tuning by ear
};

class SoundingSet {
 public:
  // ear: 0 a fixed pitch, 1 tuned by ear (the fretless), 2 a wind by ear.
  // rings: the note rings on after its key whatever its family (a koto's strings: nothing stops them).
  void on(int ch, int key, int pitch, int velocity, int fam, int ear = 0, bool rings = false);
  void off(int ch, int key);
  void pedal(int ch, bool down);
  void step(double dt);              // the ringing notes fall; the faint leave
  void setCents(int ch, int key, double cents);
  const std::vector<BandNote>& notes() const { return n_; }
  unsigned version() const { return version_; }  // changes whenever a note comes or goes
  bool pedalDown(int ch) const { return ch >= 0 && ch < 16 && pedal_[ch]; }
  double now() const { return t_; }  // (the clock: step's dt summed)
  // The winds' common healing (c/s; research/fretless_bass.md 7.6): the wind notes' tuning, each
  // as far as its pull comes from other winds (earShare: what the fixed-pitch parts ask for is
  // theirs to keep), summed by level over all that sounds (the fixed-pitch notes too), back to
  // 0 c with a 1 s time constant (PJ_WIND_EAR_HEAL; 0: none). Every held wind note moves by it
  // at once, times its own earShare, so the winds keep their intervals (a few c/s: no glide).
  double drift() const { return drift_; }
  void setEarShare(int ch, int key, double share);
  // A part's own note (channel ch, sounding at pitch; else an octave of it), or nullptr.
  const BandNote* find(int ch, int pitch) const;

 private:
  std::vector<BandNote> n_;
  double t_ = 0, drift_ = 0;
  bool pedal_[16] = {};
  unsigned version_ = 0;
};

// A part's family (Instrument::family, name) as a template; kBandNone: not heard (drums, effects).
int bandFamily(const char* family, const char* name);
// Partial k's amplitude (k >= 1) in family fam's template at fundamental f0 (Hz); 0: none. Not
// normalised.
double bandPartial(int fam, double f0, int k);
// Where partial k lies, x f0: k, but for the inharmonic (kBandChime: the tubular bells' tubes,
// from C4 to G5, and their church and carillon bells below and above; research/tubular_bells.md 9).
double bandRatio(int fam, double f0, int k);
// The band's pull on a listener's note (research/fretless_bass.md 7.5, 7.6): its own partials
// against each other note's template, each pair within 36 c pulling by the weaker one's level x
// exp(-(delta/20 c)^2) / (the higher harmonic number)^2, a faded note the less (its level x4, at
// most 1). The listener: its fundamental f0 (Hz), its partials (log2 of each over f0, as its
// strings' stiffness has them; their amplitudes, normalised here; their harmonic numbers), its
// channel (left out: its own part tunes itself), its note's onset and pitch (the order of hearing
// above; at < 0: hears every note). Returns the pull (c, the mean of the pairs' deltas) and its
// weight (the sum of theirs; 0: nothing to hear).
struct BandListener {
  int self = -1;
  double f0 = 0;
  const double* pl = nullptr;  // log2(f_k / f0)
  const double* pw = nullptr;  // amplitudes
  const double* pk = nullptr;  // harmonic numbers
  int np = 0;                  // (at most 32)
  double at = -1;              // its onset (the set's clock)
  int pitch = 0;
  // Its own part's other notes heard too (a koto's ringing strings, as an organ's pipes): those
  // before it in the order of hearing; the note itself (same pitch, begun within 35 ms) left out.
  bool hearOwn = false;
};
// (earWeight: the part of the weight from notes tuned by ear, if asked.)
double bandPull(const SoundingSet& band, const BandListener& me, double* weight, double* earWeight = nullptr);
// (-play) The file is a jazz trio's (a piano and an upright bass: src/midifile.cc's rule): its GM
// 36 is an upright, and a fretless forced onto it plays its keyboard legato without slides.
bool& jazzBandFile();

}  // namespace pajupilli
