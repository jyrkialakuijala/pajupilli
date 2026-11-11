// The flutist learning the instrument: for each note, find the lip coverage
// (rolling in / out) that puts it in tune, and a breath pressure that makes it
// sound in the right register. Runs the full model, ~0.5 s at startup.
#pragma once

#include <cstdio>

namespace pajupilli {

struct Calibration {
  double cover[128];  // uncovered-width multiplier
  double press[128];  // lung pressure multiplier
  float cents[128];   // remaining error after calibration
  bool ok[128];
  Calibration() {
    for (int i = 0; i < 128; ++i) { cover[i] = press[i] = 1; cents[i] = 0; ok[i] = false; }
  }
};

Calibration calibrate(double a4, FILE* log, int lo = 60, int hi = 96);

}  // namespace pajupilli
