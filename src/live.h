// Real-time playing: ALSA audio out, MIDI in from the Akai MPK mini mk3.
#pragma once

#include <string>

#include "calibrate.h"
#include "drumkit.h"
#include "ensemble.h"

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
  int poly = 8;                    // flutes in the pool (1: a single flute)
  // The drum kit (src/drumkit.h; null: none). Its pads (channel 10: the
  // MPK's send General MIDI's drum keys, bank A 36 .. 43, bank B 44 .. 51)
  // then play the kit instead of the breath and the flutter; kitOnKeys: the
  // keys play it too (-inst drums), the MPK's lowest C (48) the bass drum
  // (36) unless -transpose says otherwise.
  DrumKit* kit = nullptr;
  bool kitOnKeys = false;
};

// inst: the keyboard's instrument (its transposition); ens: who plays.
int runLive(const Instrument& inst, const LiveOptions& o, Ensemble& ens);

}  // namespace pajupilli
