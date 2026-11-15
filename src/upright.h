// The upright piano (-inst upright-piano; research/piano_variants.md 3.1, 3.4).
//
// The grand's strings, hammers and dampers (piano.h) on a 121 cm upright's
// scale and board (Yamaha U1 class):
//
//   - the bass shorter: wound strings 1.19 m at A0 to 0.98 m at B2 (keys
//     1-27: singles to key 8, pairs to 27), plain from C3 at 1.03 m to the
//     grand's own lengths from C5 up; the grand's wire gauge on the shorter
//     plain strings, so the tenor is stiffer (B ~ 1 / L^4): its partials run
//     sharp sooner, the octaves stretch wider, and the wound-to-plain break
//     is a step in the timbre: the upright's sound;
//   - the board 1.35 m^2 (the grand's 2 m^2), standing upright, its two
//     bridges the bass's low on the bass side and the long one diagonal
//     (tools/uprightboard.py, src/uprightboard_data.h);
//   - the microphones behind the board (as an upright is often recorded:
//     the back open), a crossed pair 60 cm from it.
#pragma once

#include "piano.h"

namespace pajupilli {

PianoDesign uprightDesign();

}  // namespace pajupilli
