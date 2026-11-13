// The flutist learning the instrument: for each note, find the lip coverage
// (rolling in / out) that puts it in tune, and a breath pressure that makes it
// sound in the right register. Runs the full model, ~0.5 s at startup.
#pragma once

#include <cstdio>

#include "instrument.h"

namespace pajupilli {

struct Calibration {
  // Learned at mezzo-forte (dynamics 0.5), and the lip coverage again at the
  // note's softest playable dynamics (dynLo) and at forte (0.85): flutists cover
  // more when loud and less when soft to stay in tune.
  double cover[128];    // uncovered-width multiplier at dynamics 0.5
  double coverLo[128];  // ... at dynLo
  double coverHi[128];  // ... at kCalDynHi
  double dynLo[128];    // softest dynamics that holds the register (+ margin)
  double dynHi[128];    // loudest (the designed instruments; the flutes: 1)
  double press[128];    // lung pressure multiplier
  double jet[128];      // jet length (lip-to-edge) multiplier
  float cents[128];     // remaining error after calibration (mezzo-forte)
  int fingering[128];   // which fingering (0 standard, 1 the alternate: brass)
  bool ok[128];
  Calibration() {
    for (int i = 0; i < 128; ++i) {
      cover[i] = coverLo[i] = coverHi[i] = press[i] = jet[i] = 1;
      dynLo[i] = 0;
      dynHi[i] = 1;
      cents[i] = 0;
      fingering[i] = 0;
      ok[i] = false;
    }
  }
};
constexpr double kCalDynHi = 0.85;

// Sounding MIDI notes lo..hi (default: the instrument's whole range).
Calibration calibrate(const Instrument& inst, double a4, FILE* log, int lo = -1, int hi = -1);

}  // namespace pajupilli
