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
//     partials strong; brass 1/sqrt(k) to 2 kHz; mallets their fundamental; the fretless's own
//     from FluidR3's recorded GM 36.
// Its first user is the fretless bass (src/fretless.cc): it tunes its note by ear to these.
#pragma once

#include <vector>

namespace pajupilli {

enum BandFamily {
  kBandPiano, kBandPluck, kBandMallet, kBandBell, kBandOrgan, kBandChoir, kBandFlute,
  kBandStrings, kBandBrass, kBandReed, kBandBass, kBandOther, kBandNone
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
};

class SoundingSet {
 public:
  void on(int ch, int key, int pitch, int velocity, int fam);
  void off(int ch, int key);
  void pedal(int ch, bool down);
  void step(double dt);              // the ringing notes fall; the faint leave
  void setCents(int ch, int key, double cents);
  const std::vector<BandNote>& notes() const { return n_; }
  unsigned version() const { return version_; }  // changes whenever a note comes or goes
  bool pedalDown(int ch) const { return ch >= 0 && ch < 16 && pedal_[ch]; }

 private:
  std::vector<BandNote> n_;
  bool pedal_[16] = {};
  unsigned version_ = 0;
};

// A part's family (Instrument::family, name) as a template; kBandNone: not heard (drums, effects).
int bandFamily(const char* family, const char* name);
// Partial k's amplitude (k >= 1) in family fam's template at fundamental f0 (Hz); 0: none. Not
// normalised.
double bandPartial(int fam, double f0, int k);
// (-play) The file is a jazz trio's (a piano and an upright bass: src/midifile.cc's rule): its GM
// 36 is an upright, and a fretless forced onto it plays its keyboard legato without slides.
bool& jazzBandFile();

}  // namespace pajupilli
