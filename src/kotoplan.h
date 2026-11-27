// The koto player's reading of its part before playing (research/koto_expressive.md 2, 6.1): where
// the thirteen ji go. A koto is tuned for the piece: its bridges set so the part's notes fall on
// open strings, a note between them pressed behind its ji (a semitone or a whole tone up), and the
// ji moved again where the music leaves that set, at a breath, as few as the music needs. A fixed
// hirajoshi under a pop song's melody would press half its notes and move a bridge for the rest.
//
// The plan: from each step's time (s, the song's clock) the thirteen open pitches, low to high (a
// string sounds a key at its pitch or one or two semitones above it). A key outside the koto's reach
// (open strings C3..C#6, sounding lengths 1.37..0.16 m at 147 N) is played an octave in.
// The fingering (kotoFinger, kotoplan.cc): a chord's top note the thumb's, its lowest the middle
// finger's (awase-zume: they pinch), between them the index; a low bass note under three or more,
// or under the right hand's line, the left hand's when no press needs it. A line: the thumb; a
// repeated note thumb and index in turn; a fast run up the index alone pulling through the
// strings, down the thumb; a broken chord up middle, index, thumb (down: thumb, index, middle).
// PJ_KOTO_PLAN=0: no plan, the hirajoshi on D throughout (as live).
#pragma once

#include <string>
#include <vector>

namespace pajupilli {

struct KotoNote { double t, off; int key, vel; };  // the part's notes (sounding pitches), by onset
struct KotoTuning { double t; int open[13]; };     // from t on, the strings' open pitches

// The hands (research/koto_expressive.md 4): the three tsume of the right hand (ivory corners on the
// thumb, index and middle finger: the thumb plucks furthest from the fixed bridge and pushes the
// string away, the index and middle nearer and pull it toward the player), and the left hand's bare
// finger, softer, on a bass string while it is free of pressing.
enum KotoFinger { kKotoThumb, kKotoIndex, kKotoMiddle, kKotoLeft };
struct KotoStroke { double t; int key, finger; };  // a note's finger, by onset (the part's keys)

struct KotoPlan {
  std::vector<KotoTuning> steps;
  std::vector<KotoStroke> strokes;
  std::string summary;  // what it decided, to print
};

// The lowest and highest open string, and a key folded by octaves into what the strings reach.
constexpr int kKotoOpenLo = 48, kKotoOpenHi = 85;
int kotoFold(int key);
KotoPlan planKoto(const std::vector<KotoNote>& notes);

}  // namespace pajupilli
