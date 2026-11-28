// The cowbell player (research/triangle_cowbell.md 7): a key 56 in a file is
// "the cowbell plays here", not "the one cowbell sound". Before the song the
// player reads the bell's notes against the meter and the band and chooses
// each stroke: where on the bell (the shaft on the mouth: the open, low,
// accented clang; the tip toward the crown: the dry, high ping of the
// in-between notes), how much the hand mutes it, how hard.
//
//   - The file's velocities, where they say something (two levels or more,
//     apart), are the accent: the louder notes on the mouth. Where they are
//     flat (a transcription's 100), the meter and the band decide.
//   - The meter: the bar's 1 and 3 on the mouth, 2 and 4 nearer the body, the
//     off-beats and sixteenths toward the crown (ranked within each bar, so a
//     bell on quarters and one on sixteenths both get their two voices).
//   - Listening: a stroke with the clave (75), or the bass drum, is an accent;
//     one with the snare's backbeat in a rock or funk groove locks with it,
//     softer and nearer the crown.
//   - The hand: a Latin band's bell (the file has bongos, congas, timbales,
//     claves or a guiro) is held, so its crown strokes are muted by a finger and
//     its mouth strokes open; a kit's bell is mounted, tape on the inside, a
//     little muted always. A last stroke before a rest rings.
//   - A transcription (flat velocities, off the file's grid): an isolated
//     note off the beat is skipped (likely misheard), one on the beat played
//     soft; a doubled note (two within 25 ms) played once.
// PJ_COWBELL_PLAN=0: every note the open mouth stroke at its velocity, as before.
#pragma once

#include <string>
#include <vector>

#include "drumkit.h"

namespace pajupilli {

// The drum keys -play sends for a chosen stroke (not General MIDI's): 16 for
// a held bell from kKeyCowbellStroke0 (to the cowbell's kCowbellStroke0 + the
// same offset: it turns as it is struck), 16 for a mounted one from
// kKeyCowbellMounted0 (to kCowbellMounted0 + it: still).
constexpr int kKeyCowbellStroke0 = 104, kKeyCowbellMounted0 = 88;

struct CowbellStroke {
  int key = 56;       // the drum key to send (56, or kKeyCowbell{Stroke,Mounted}0 + 4 place + mute)
  int velocity = 0;
  bool skip = false;  // not played
};

// For every drum-channel key 56 among `notes` (in their order): its stroke.
// `notes` is the song's note-ons in time order (drumkit.h); summary: what the
// player read and chose, to print.
std::vector<CowbellStroke> planCowbell(const std::vector<DrumPlanNote>& notes, const std::vector<DrumTempo>& tempo, std::string* summary);

}  // namespace pajupilli
