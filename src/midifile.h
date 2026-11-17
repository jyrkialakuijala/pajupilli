// A Standard MIDI File played by the program's instruments (-play FILE.mid).
//
// Each channel's General MIDI program picks its instrument: the one built for
// that program, or for those not built yet the nearest that is (the same
// family first). Channel 10's drums go to the percussion that exists (taiko,
// toms, woodblock, agogo, synth drum); cymbals and hi-hats are not built and
// stay silent. Each channel is a pool of its instrument (a piano or a plucked
// instrument: one voice that takes every key), in the chosen room; the
// sustain pedal (CC 64), volume (7), expression (11), channel pressure (the
// breath) and pitch bend (the lips; a Clavinet: the wah) are followed. The
// song is rendered on its own thread and played (ALSA) as it comes, a second
// behind; should the rendering be slower than real time, the player waits
// once, long enough to play on to the end. With -o: written to a file.
#pragma once

#include <string>

#include "room.h"

namespace pajupilli {

// Returns the process exit code.
int playMidiFile(const std::string& path, const char* outWav, const RoomSpec* room, const std::string& device,
                 bool verbose, bool roomGiven = true);

}  // namespace pajupilli
