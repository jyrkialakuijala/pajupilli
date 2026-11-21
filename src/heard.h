// Messages timed to the listening. Under -play the rendering runs up to 10 s (PJ_PLAY_LEAD) ahead
// of what is heard, so whatever a player or the band says about a moment (the piano's "sustain
// pedal down", a section's "tune:", a Hammond's "Leslie: fast") is stamped with the song's time
// and queued, and the playback loop prints it when that moment is heard, beside its m:ss ticks.
// Writing a file (-o), it is printed at once, stamped. Outside -play (-inst, -seq, the tests'
// renders) it is printed as it always was, unstamped.
#pragma once

#include <cstdio>
#include <string>

namespace pajupilli {

// printf into a string.
std::string strf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
// Say it (printf; the text may end with a newline or not): under -play at the song's time now
// (SongClock::now, the block being rendered), otherwise at once as it is.
void say(std::FILE* f, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
// Say `text` (one line, no newline) on stdout when the song's time t (s) is heard.
void sayAt(double t, const std::string& text, std::FILE* f = stdout);
// (the playback loop) heardLive(true) before the rendering starts, so the sayings queue;
// heardUntil(t): everything due by the heard time t printed; heardLive(false) at the end prints
// whatever is left.
void heardLive(bool on);
void heardUntil(double t);

}  // namespace pajupilli
