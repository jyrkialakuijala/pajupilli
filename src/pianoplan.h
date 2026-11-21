// A pianist's sustain pedal, planned from a piano part's notes (-pianist with -play): the file's
// own pedalling (CC 64) on the piano's channel is set aside and this plan's put in its place, at
// the times a pianist would press and lift it (research/pedaling.md; its sections below, its
// rules R1-R11 in §7, the validation in §8).
//
//   - Legato (syncopated) pedalling (§2.2): up as the new harmony is struck, 10 ms before the
//     onset ±25 ms (ten performed files here release a median -8 .. +24 ms from it, §4.7), down
//     again 40 ms + a quarter of the time to the next onset after it, 50 .. 450 ms (MAESTRO's
//     1276 competition performances, §4.3: a median 146 ms; Repp 1997: earlier at a quicker tempo,
//     by less than the tempo), spread lognormally; before the bass's key comes up where it can
//     (caught, P10); up at least 60 ms, 120 ms over a bass below C3 (P3).
//   - A change where the harmony changes (P1, §4.3's bass-motion table): a new bass (a step always,
//     a leap to a new pitch class once the pedal has stood 0.4 s: a broken chord's notes are not
//     each a bass); a chord of three bringing two new pitch classes, or revoiced 0.8 beat on; the
//     bass struck again 0.6 s on (the ballad pianists here re-pedal at most left-hand attacks,
//     §4.7); a low note a second from one under the pedal (the mud of §5.2: the bass's partials
//     within a quarter of a critical band, its decay of tens of seconds); eight pitch classes
//     gathered (a wash). Not a melody's note over the harmony (§5.3: the treble clears itself).
//   - Up through a stepwise run in the low register (P6); not again within 0.16 s (§4.3: fast
//     notes go under one pedal).
//   - Direct pedal (P4): with the note (+23 ms, MAESTRO) at the start and after a lift; lifted
//     where every key is up over a second and over a beat (the pianists hold it through shorter
//     silences, §2.7, §4.7); at the end 50 ms after the last release.
//   - The texture by four-bar sections (§3, P7, P8): by harmony (chords, broken chords, a line
//     over a held bass: a ballad, Chopin); linear (few chords, lines moving by step, no held bass:
//     Bach's counterpoint, finger legato, only the long chords pedalled; §3.1); detached (staccato
//     in a quick rhythm: dry). The director's Baroque reading of a quantized file leans linear.
//   - The model's damper is on/off (src/piano.cc: the felt falls in 50 ms), so no half pedal; a
//     melody's semitones gathering over a long-held harmony get a quick change (up ~50 ms) in the
//     half change's place (§2.4, P9).
//   - Human timing from a seeded generator: the same file, the same pedalling.
//
// PJ_PIANIST_LOG=1 prints every decision (=2 every pedal event too); PJ_PIANIST_LAG (x 1) scales
// the press lag; PJ_PIANIST_UPSHIFT=1 releases as MAESTRO's pianists do (later in slow music).
#pragma once

#include <string>
#include <vector>

namespace pajupilli {

struct PianoNote {
  double on, off;      // seconds
  double beat;         // the onset in quarter notes (the file's ticks / its division)
  long tick;
  int pitch, vel;
};

struct PedalChange {
  double t;            // seconds
  bool down;
  long tick;           // (the tick of the onset it belongs to)
  int kind;            // 0 a legato change, 1 a direct pedal, 2 a lift (a rest, a run, the end), 3 a quick change
  std::string why;     // (for the log)
};

struct PianoPedalPlan {
  std::vector<PedalChange> changes;  // in time order, alternating down/up, starting with a down
  std::string summary;               // what was decided, a few lines
  std::vector<std::string> log;      // every decision, one line each (PJ_PIANIST_LOG)
  int legato = 0, direct = 0, quick = 0, lifts = 0;
  double downShare = 0;              // the share of the part's time with the pedal down
};

// notes: the part's notes (any order); quartersPerBar: a bar's length (4 if unknown); baroque:
// the director read the piece as Baroque; seed: the human timing's; who: the part's name.
PianoPedalPlan planPedal(const std::vector<PianoNote>& notes, double quartersPerBar, bool baroque, unsigned seed,
                         const std::string& who);

}  // namespace pajupilli
