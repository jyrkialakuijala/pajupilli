// Reading a part's key velocities (the owner: "when there is a phrase and the midi file request
// higher velocity notes for a wind/brass/string instrument, then it needs to be interpreted if it
// is a stronger note from the very beginning or if it belongs to a line of expansion").
//
// A piano's velocity is one hammer blow. A sustained instrument has two things to say with it:
//   - the line: the phrase's dynamics, a crescendo or a diminuendo carried through the notes
//     (the breath, the bow's speed and weight growing inside each note towards the next), with
//     ordinary attacks: no note of it starts with a bump;
//   - the accent: one note stronger from its very beginning (the tongue, the bow's bite), then
//     back on the line; or one lighter than the line (a passing note).
//
// The reading, per phrase (a rest of a beat or more ends one):
//   - the line at a note is the middle of its velocity and its two neighbours': a rising or
//     falling run passes unchanged, a single note standing out does not move it;
//   - what a note has over (or under) the line is its accent (under kDead: none, the line takes it);
//   - inside a run of three notes or more rising (or falling) the line glides through each note
//     to the next one's level, where they are bound (the next begins within a quarter beat of
//     this one's end); a single step that then stays is a terrace: the new level from the new
//     note's start, the note before it holding its own;
//   - a phrase's first note has nothing to grow from: its level is there from its beginning; at a
//     phrase's ends the line is continued from inside (a ramp's first note is on the line, a loud
//     first note before a level line is an accent). Two notes alone: each its own level.
// Notes struck together (a chord, divisi) are one moment: its loudest key reads for all.
#pragma once

#include <vector>

namespace pajupilli {

struct VelNote {
  long tick, tickOff;
  int vel;
};
struct VelReading {
  float line;     // the line's level at the note's start, as a key velocity (1 .. 127)
  float lineEnd;  // ... and at its end (a crescendo or diminuendo through the note)
  float accent;   // the note's own emphasis over the line, in velocity steps (negative: lighter)
};

// part: one part's notes, sorted by tick. tpq: ticks a quarter.
std::vector<VelReading> readVelocities(const std::vector<VelNote>& part, int tpq);

}  // namespace pajupilli
