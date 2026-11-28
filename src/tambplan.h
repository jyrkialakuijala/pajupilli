// The tambourine player (research/tambourine.md 1.1, 1.2): a key 54 in a file is
// "something should happen with the tambourine here", a hint of about when and
// about how much, not "the tambourine sound". Before the song the player reads
// the tambourine's notes against the meter, the band and the director's reading
// of the piece and chooses each gesture: what makes the jingles move (a hit
// against the other hand or the thigh, a turn in the air, a twist of the wrist,
// a stroke on the head with fingertips, fingers, fist or palm, where on it), how
// high it is held, its tilt, how strong, the lift before it, what follows (ring,
// hold, choke); or a roll for a run of fast notes; or nothing.
//
//   - Velocities, where they say something (several levels, apart): the intent.
//     Flat (a transcription's 100): the band's loudness and the meter decide.
//   - A transcription (flat, off the file's grid): an isolated note is likely
//     misheard (skipped, or soft on a strong beat); one with a shaker's note
//     is likely the shaker's (softer).
//   - In a groove: the down strokes on the beats and the ands, the up strokes
//     between, weaker; the backbeat with the snare a stopped hit against the
//     other hand's heel, on one of 2 and 4 (the quieter for the tune), both
//     where the band is loud; a crash covers it (softer).
//   - A run of fast notes: a roll (a shake roll, long or loud; a thumb roll,
//     short and soft), its start and end an attack.
//   - Alone: a stroke on the head, from the fingertips at the rim (soft) to the
//     fist or palm in the centre (loud).
//   - The director: a classical reading plays the concert tambourine's ways
//     (the head, the fingers, thumb rolls, held at the chest); jazz plays
//     little and softly; rock or funk harder, held higher.
//   - Where a run begins after a rest and the band grows louder there, a short
//     shake into it (at most twice in a song).
//   - The last note of the song: choked when the band stops with it.
// PJ_TAMB_PLAN=0: every note the old single stroke at its velocity.
#pragma once

#include <string>
#include <vector>

#include "drumkit.h"
#include "tambgesture.h"

namespace pajupilli {

// One planned gesture: when (the song's time, s, as the key was written), what.
struct TambPlanned {
  double t = 0;
  TambGesture g;
  int velocity = 0;
  bool skip = false;   // the note not played
  bool added = false;  // a gesture the file did not write (a lead-in shake)
};

// What the director read of the piece, for the tambourine.
struct TambDirection {
  bool classical = false;  // a classical reading (Baroque, Classical, Romantic): the concert tambourine
  bool jazz = false;
  bool amped = false;      // rock, funk or blues
};

// For every drum-channel key 54 among `notes` (time order): its gesture, and
// after those the gestures the player adds of its own (added = true).
std::vector<TambPlanned> planTambourine(const std::vector<DrumPlanNote>& notes, const std::vector<DrumTempo>& tempo,
                                        const TambDirection& dir, std::string* summary);

// The song's plan, for the player as it plays (set by -play; empty live).
std::vector<TambPlanned>& tambourinePlan();
// The planned gesture for a key at song time t (s; within 30 ms), or null.
const TambPlanned* plannedTambourine(double t);

}  // namespace pajupilli
