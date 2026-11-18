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
//   - The harmonic percussion where the tune is single, detached and moving (the jazz line), not
//     in legato, chordal or slow playing: one choice for the piece, switched for one long contrasting
//     stretch at most (two changes), the percussive organ always on.
#pragma once

#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "hammond.h"

namespace pajupilli {

struct HammondNote {
  int line;          // channel * 1000 + track
  long on, off;      // ticks
  int pitch, vel, ch;
  int expr = 127;    // its channel's expression (CC11) when it began
  int forced = -1;   // a division its instrument routes it to (hammond-upper-manual ...), or -1
};

struct HammondPlan {
  // (channel * 128 + pitch, on-tick) -> HammondModel::Division
  std::map<std::pair<int, long>, int> division;
  HammondRegistration reg;
  // The Leslie's planned speed changes (tick, fast); empty when the file sets it itself.
  std::vector<std::pair<long, bool>> leslie;
  // Nudges of the slow Leslie on long held notes (tick, the slow speed x this; 1 back to slow).
  std::vector<std::pair<long, double>> leslieBoost;
  // The harmonic percussion's planned switches (tick, on): at most two, mostly none (the piece's
  // choice is in reg).
  std::vector<std::pair<long, bool>> percussion;
  // The re-registrations at the general pauses (tick, the upper's drawbars from there): a drawbar
  // pulled out at each, toward the end; the lower manual and pedals follow (lowerExtra: its
  // balance's extra softness, steps).
  std::vector<std::pair<long, std::array<int, 9>>> upperChanges;
  double lowerExtra = 0;
  // The channels whose expression (CC11) is the organ's pedal: the tune's (mostly upper notes).
  std::set<int> exprChannels;
  std::string summary;
};

// fileLeslie: the file sends its own Leslie controls (CC1, CC64, CC82 on the part's channels).
HammondPlan planHammond(const std::vector<HammondNote>& notes, int tpq, double bpm, const std::string& instrument,
                        bool fileLeslie);

}  // namespace pajupilli
