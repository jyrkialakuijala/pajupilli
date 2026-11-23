// The bright piano (-inst bright-piano, General MIDI 2): a semi-concert grand, the 229 cm
// semi-concert grand (research/bright_grand.md).
//
// The concert grand's strings, hammers and dampers (piano.h) on the semi-concert grand's own scale, hammers
// and board, miked as a studio grand is:
//
//   - the scale of a 229 cm case: wound strings 1.66 m at A0 to 1.16 m at E2 (keys 1-20), plain
//     from 1.44 m at F2 growing to the grand's own lengths from C5 up; the grand's wire on
//     shorter strings: the bass ~1.5x and the low tenor ~2-3x stiffer, their upper partials
//     sharper and stronger, the tuning stretched further;
//   - its maker's hammers, pressed to a high felt density and needled down at the shoulders (no
//     hardeners): the felt stiffer from the tenor up (x2 at C4, x2.5 from C6), its loose nap
//     half as thick (the dense pressing): a middling blow already bright, the softest still round;
//   - the board of the shorter case (tools/brightboard.py, src/brightboard_data.h: 1.77 m^2, its
//     lowest resonance 78 Hz), a little less lossy (the long-sustaining spruce, the hard rim);
//   - the crossed pair of microphones 30 cm over the strings, nearer the hammers' end of the
//     board: close, as a pop or studio grand is recorded.
#pragma once

#include "piano.h"

namespace pajupilli {

PianoDesign brightGrandDesign();

}  // namespace pajupilli
