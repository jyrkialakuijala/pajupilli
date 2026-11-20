// An organ part's registration plan, made from its notes (a MIDI file's
// tracks): which track plays on which division (the Pedal, the Hauptwerk,
// the Schwellwerk, the Positiv) and how the registration builds through the
// piece.
//
//   - The pedal: the lowest track staying within the pedal's compass (to
//     G4), mostly one voice at a time, clearly under the others.
//   - A trio (two one-voice upper tracks crossing in one range, a pedal
//     under them): the upper voices on two manuals of contrasting colour as
//     strong (the right hand the Hauptwerk, the left the Positiv), one
//     registration throughout.
//   - Otherwise the lines on their own manuals (the highest the Hauptwerk, the
//     next the swell, a third the Positiv: each its colour); the piece in
//     sections (the tempo's changes and the general pauses: a passacaglia and
//     its fugue, a sonata's movements), each a little more than the last, a
//     clearly slower one (an adagio) less; within a section phrases of 8 bars,
//     building from its start to its end,
//     nudged by its texture (voices, notes a second), changing only at a
//     phrase's start and a step at a time upwards; where the pedal rests for
//     a phrase (manualiter) the hands drop to the Positiv or the swell, two
//     steps softer, for contrast; the last phrase at the part's peak.
//   - Each level a registration on every division (the pedal balanced to the
//     manuals, the swell coupled from f, the reeds from ff).
#pragma once

#include <map>
#include <string>
#include <vector>

namespace pajupilli {

struct OrganNote {
  int track;
  long on, off;  // ticks
  int pitch;
};

struct OrganPlanChange {
  long tick;
  std::string registration;     // for OrganModel::setRegistration
  double swell;                 // the swell boxes' opening, 0..1
  std::map<int, int> manuals;   // track -> division (0 HW, 1 SW, 2 Pos, 3 Ped) from here on
  std::string what;             // (a line for a listing: the level, the manuals)
};

struct OrganPlan {
  std::vector<OrganPlanChange> changes;  // in time order; the first at tick 0
  std::string summary;
  // The pedal's articulation: detached (Baroque: a quarter of each note
  // given to silence, 60 .. 180 ms, more before a strong beat: the low notes
  // clear in the church, the strong beat accented) or held (a romantic
  // pedal point, a meditative piece: its full length).
  bool pedalDetached = true;
  // The ticks where its sections begin (the tempo's changes, the general
  // pauses): a note before one ends a part, held to its length.
  std::vector<long> partStarts;
};

// notes: the organ part's notes, by track; tpq: ticks a quarter; barTicks:
// a bar's length; partStarts: ticks where a large part begins (the tempo's
// big changes; 0 implied).
// tempo: (tick, beats a minute) from each tempo change on (a slower section
// registered softer). General pauses (every line silent a beat or more) also
// begin sections.
OrganPlan planOrgan(const std::vector<OrganNote>& notes, int tpq, long barTicks, const std::vector<long>& partStarts,
                    const std::vector<std::pair<long, double>>& tempo = {});

// The registration of a level (0 ppp .. 7 fff) with the hands on a division,
// the pedal's under them if it plays.
std::string organRegistration(int level, int handsOn, bool pedal);

// The keyboard in zones (one 88-key keyboard for the organ): the pedal's,
// the left hand's (the Schwellwerk) and the right hand's (the Hauptwerk, at
// ppp .. p the Positiv). The registration of a level on all three, and the
// division the right hand plays; the swell box's opening.
struct ZoneRegistration {
  std::string stops;
  int rightHand;  // 0 HW, 2 Pos
  double swell;
};
ZoneRegistration organZoneRegistration(int level);

}  // namespace pajupilli
