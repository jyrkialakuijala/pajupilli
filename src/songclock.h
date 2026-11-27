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
  // The song's key: its tonic as a pitch class (0 = C .. 11 = B; -1: not
  // known), from the file's key signature or, without one, from which notes
  // it uses most. For what is tuned to a key before playing (a harpsichord).
  int tonic = -1;
  bool minor = false;
  // What the band is playing, for players who listen: the song's time now
  // (s), and when each pitch (MIDI key, any part but the drums) was last
  // begun (s; far in the past: never).
  double now = 0;
  double start = 0;      // (-start_time: where in the song the rendering began, s)
  bool playing = false;  // (a file is being rendered: -play; messages carry the song's time, src/heard.h)
  double lastOn[128];
  // The ending: from this moment of the song (s) no brush circles on the snare of its own accord or because
  // the file holds a swirl (two bars before the final note; the file player sets it). Strokes still sound.
  double brushEnd = 1e18;
  SongClock() { for (double& t : lastOn) t = -1e9; }
  static SongClock& get() {
    static SongClock c;
    return c;
  }
};

}  // namespace pajupilli
