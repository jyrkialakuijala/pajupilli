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
  // Keys to sounding pitch; default: the MPK's lowest C (C3) plays the
  // instrument's lowest octave (flute +12, piccolo +24, alto +12, bass 0).
  int transpose = kAutoTranspose;
  static constexpr int kAutoTranspose = -1000;
  bool verbose = false;
  bool selfTest = false;           // play an arpeggio without a keyboard
  int poly = 1;                    // flutes: more than one deals the keys to a pool
};

int runLive(const Instrument& inst, const LiveOptions& o, const Calibration& cal);

}  // namespace pajupilli
