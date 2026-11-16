// The song's beat, for players who keep time with it (the drummer's brush
// circles): set by the MIDI file player as it renders; not valid live.
#pragma once

namespace pajupilli {

struct SongClock {
  bool valid = false;
  double beatLen = 0.5;  // seconds a quarter note, now
  double beat = 0;       // quarter notes since the song's start, now
  // A general pause: the whole band silent (no note held in any part but the
  // drums, none begun for two beats, none to come within the next beat), or
  // the song over.
  bool pause = false;
  static SongClock& get() {
    static SongClock c;
    return c;
  }
};

}  // namespace pajupilli
