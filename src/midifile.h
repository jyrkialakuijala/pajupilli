// A Standard MIDI File played by the program's instruments (-play FILE.mid).
//
// Each channel's General MIDI program picks its instrument: the one built for
// that program, or for those not built yet the nearest that is (the same
// family first). Channel 10's drums go to the percussion that exists (taiko,
// toms, woodblock, agogo, synth drum); cymbals and hi-hats are not built and
// stay silent. Each channel is a pool of its instrument (a piano or a plucked
// instrument: one voice that takes every key), in the chosen room; the
// sustain pedal (CC 64), volume (7), expression (11), channel pressure (the
// breath) and pitch bend (the lips; a Clavi: the wah) are followed. The
// song is rendered on its own thread and played (ALSA) as it comes, a second
// behind; should the rendering be slower than real time, the player waits
// once, long enough to play on to the end. With -o: written to a file.
#pragma once

#include <string>

#include "room.h"

namespace pajupilli {

// Reinstrumenting (-reinstrument): rules FROM=TO, comma-separated, applied to
// each channel's instrument as its program picks it: FROM an instrument's name
// (grand-piano), its family (piano) or * (every channel but the drums); TO an
// instrument ("organ": the church organ by its plan, organ-plan). The first
// rule that matches wins. Returns false (and says why) if a rule is wrong.
bool setReinstrument(const std::string& spec);

// Only some instruments heard (-only LIST with -play): comma-separated instrument names
// (grand-piano), families (choir), channels (11 or ch11) or drums; the rest's notes left out after
// the director has read the whole piece, so the tempo and phrasing stay as with every part.
// skip (-onlyskip LIST): and the song from just before the first of their notes.
void setOnly(const std::string& list, bool skip = false);

// Transposing (-transpose N with -play): every note but the drums' N
// semitones (12: an octave up), as the file is read, before any planning.
void setPlayTranspose(int semitones);

// A pianist's pedalling (-pianist with -play): each piano part's sustain pedal planned from its
// notes (src/pianoplan.h), the file's own CC 64 on its channel set aside.
void setPianist(bool on);
void setPlayVolume(double dB);  // -volume: the whole mix, before the output's limit
void setPlayStart(double seconds);  // -start_time: begin there in the song

// -inst's list of the instruments: in General MIDI's order and groups, each with its program
// number; then those no program reaches, by family.
std::string instrumentList();

// Returns the process exit code.
int playMidiFile(const std::string& path, const char* outWav, const RoomSpec* room, const std::string& device,
                 bool verbose, bool roomGiven = true);

}  // namespace pajupilli
