// Real-time playing: ALSA audio out, MIDI in from the Akai MPK mini mk3.
#pragma once

#include <string>

#include "calibrate.h"

namespace pajupilli {

struct LiveOptions {
  std::string device = "default";  // ALSA PCM ("default" goes through PipeWire)
  std::string midiName = "MPK";    // connect to MIDI clients with this in the name
  int period = 64;                 // frames per period (1.3 ms)
  int periods = 3;                 // periods in the buffer
  int transpose = 12;              // the MPK's 25 keys start at C3 by default
  bool verbose = false;
  bool selfTest = false;           // play an arpeggio without a keyboard
};

int runLive(const LiveOptions& o, const Calibration& cal);

}  // namespace pajupilli
