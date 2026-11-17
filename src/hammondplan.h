// A Hammond part's plan from a MIDI file (-play), made by rules from its notes: which notes the
// pedals take, which the lower manual, which the upper, and the drawbars for the piece.
//
//   - Lines: the part's tracks. A low, mostly one-note-at-a-time line (under ~D3 on average) is the
//     pedal line; the highest line the upper manual; the others the lower.
//   - A line holding both hands (one track, chords under a tune): each moment's notes (begun
//     together) split: the top note the upper manual with the notes close under it (a right hand's
//     voicing within a sixth), the rest the lower; with no pedal line, a low note well under the
//     rest (a fifth or more, below C3) the pedals. A tune's single notes keep their manual while
//     they move by small steps (no hopping between manuals at a split point).
//   - The drawbars from the piece's character: its program (17, 18, 19), its tempo, how busy the
//     tune is, how chordal: a quick, linear tune the jazz organ's 888 with the 3rd percussion; a
//     slow piece a softer 838 with the chorus; a chordal, hymn-like one fuller, 888800000; the
//     rock organ 888800000 driven; the lower manual a comping 838 (808 under a soft upper), the
//     pedals 8 / 3 (8 / 5 when there is no pedal line to carry, the bass then on the lower).
//   - Three lines (an organist's three staves): the lowest the pedals, the other two the manuals.
//   - The Leslie: the file's own controls if it has them; else planned from the notes (seeded by
//     them: the same file, the same plan): most pieces slow throughout, about one in four fast in
//     its strongest phrase (or two).
#pragma once

#include <map>
#include <string>
#include <vector>

#include "hammond.h"

namespace pajupilli {

struct HammondNote {
  int line;          // channel * 1000 + track
  long on, off;      // ticks
  int pitch, vel, ch;
};

struct HammondPlan {
  // (channel * 128 + pitch, on-tick) -> HammondModel::Division
  std::map<std::pair<int, long>, int> division;
  HammondRegistration reg;
  // The Leslie's planned speed changes (tick, fast); empty when the file sets it itself.
  std::vector<std::pair<long, bool>> leslie;
  std::string summary;
};

// fileLeslie: the file sends its own Leslie controls (CC1, CC64, CC82 on the part's channels).
HammondPlan planHammond(const std::vector<HammondNote>& notes, int tpq, double bpm, const std::string& instrument,
                        bool fileLeslie);

}  // namespace pajupilli
