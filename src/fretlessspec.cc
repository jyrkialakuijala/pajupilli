// The fretless bass's instrument (src/fretless.h): the Jazz Bass defretted. Only guitar.h, the Jazz
// Bass's spec and the knobs: the unit checks (tools/fretlessboard.cc) build it without the rest.
#include <cmath>

#include "ebass.h"
#include "fretless.h"
#include "knob.h"

namespace pajupilli {

// The Jazz Bass defretted (research/fretless_bass.md 1, 2, 8.2): the board (an epoxied or ebony
// one: the double bass's contact), the action over the board's end 2.5 mm (PJ_FRETLESS_ACTION x),
// no relief (PJ_FRETLESS_RELIEF, 1/m), the fingertip's loss half the double bass's
// (PJ_FRETLESS_FINGER). PJ_FRETLESS_FLATS=1: flatwounds (1.4: the partials lose 2.5 times as much,
// a little stiffer: less mwah, no squeak). The bridge pickup favoured.
GuitarSpec fretlessBass() {
  GuitarSpec g = jazzBass();
  g.name = "fretless-bass";
  g.board = knob("PJ_FRETLESS_BOARD", 1) > 0.5;
  g.boardK = knob("PJ_FRETLESS_BOARDK", 3e4);
  g.boardC = knob("PJ_FRETLESS_BOARDC", 1.5);
  g.boardEnd = g.scale * std::exp2(-20 / 12.0);
  g.boardAction = 2.5e-3 * knob("PJ_FRETLESS_ACTION", 1.0);
  g.boardRelief = knob("PJ_FRETLESS_RELIEF", 0.0);
  g.fingerLoss = knob("PJ_FRETLESS_FINGER", 0.5);
  g.tilt *= knob("PJ_FRETLESS_TILT", 1.0);
  g.tiltJitter *= knob("PJ_FRETLESS_TILT", 1.0);
  if (knob("PJ_FRETLESS_FLATS", 0) > 0.5)
    for (int i = 0; i < 4; ++i) {
      g.str[i].loss1 *= 2.5;
      g.str[i].B *= 1.3;
    }
  // The pickups: the bridge full up, the neck at half (PJ_FRETLESS_PICKUP 1 bridge, 2 both full,
  // 3 neck; 0: the default).
  const int pos = int(knob("PJ_FRETLESS_PICKUP", 0));
  if (pos == 1) { g.select[0] = 1; g.select[1] = -1; }
  else if (pos == 3) { g.select[0] = 0; g.select[1] = -1; }
  else {
    g.select[0] = 1;
    g.select[1] = 0;
    if (pos != 2) g.pickups[0].volts *= 0.5;
  }
  return g;
}

}  // namespace pajupilli
