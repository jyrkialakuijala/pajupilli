// A Tonewheel part's plan from a MIDI file (-play), made by rules from its notes: which notes the
// pedals take, which the lower manual, which the upper, and the drawbars for the piece.
//
//   - Lines: the part's tracks. A low, mostly one-note-at-a-time line (under ~D3 on average) is the
//     pedal line; the highest line the upper manual; the others the lower.
//   - A line holding both hands (one track, chords under a tune): each moment's notes (begun
//     together) split: the top note the upper manual with the notes close under it (a right hand's
//     voicing within a sixth), the rest the lower; with no pedal line, a low note well under the
//     rest (a fifth or more, below C3) the pedals. A tune's single notes keep their manual while
//     they move by small steps (no hopping between manuals at a split point).
//   - The drawbars from the piece's character: its program (17, 18, 19), its tempo, how busy the
//     tune is, how chordal: a quick, linear tune the jazz organ's 888 with the 3rd percussion; a
//     slow piece a softer 838 with the chorus; a chordal, hymn-like one fuller, 888800000; the
//     rock organ 888800000 driven; the lower manual a comping 838 (808 under a soft upper), the
//     pedals 8 / 3 (8 / 5 when there is no pedal line to carry, the bass then on the lower).
//   - Three lines (an organist's three staves): the lowest the pedals, the other two the manuals.
//   - The Rotary: the file's own controls if it has them; else planned from the notes (seeded by
//     them: the same file, the same plan): most pieces slow throughout, about one in four fast in
//     its strongest phrase (or two).
//   - The harmonic percussion where the tune is single, detached and moving (the jazz line), not
//     in legato, chordal or slow playing: one choice for the piece, switched for one long contrasting
//     stretch at most (two changes), the percussive organ always on. The jazz setting soft (Jimmy
//     Smith's 888, 3rd, fast, soft); a ballad's single, spaced tune the 2nd, slow, soft (a bell).
//     Where it is on, a legato line's accented notes (on the beat, or louder) are detached before,
//     so they ping.
//   - The vibrato tabs: the chorus on both manuals, off for counterpoint (the lines straight), off
//     on the lower manual when it carries the bass.
//   - Preset keys: a re-registration at a short pause at once (the B key), not pulled.
//   - The ending: on a long final chord the Rotary up and winding down (or down, if fast), the
//     expression closing on a ballad or swelling otherwise, two more drawbars out (not a ballad);
//     the rock organ's sometimes ending with the run switch off: the pitch sinking.
//   - A glissando up the white keys into the first note of a climax (where the Rotary goes fast).
#pragma once

#include <array>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "tonewheel.h"

namespace pajupilli {

struct TonewheelNote {
  int line;          // channel * 1000 + track
  long on, off;      // ticks
  int pitch, vel, ch;
  int expr = 127;    // its channel's expression (CC11) when it began
  int forced = -1;   // a division its instrument routes it to (tonewheel-upper-manual ...), or -1
};

struct TonewheelPlan {
  // (channel * 128 + pitch, on-tick) -> TonewheelModel::Division
  std::map<std::pair<int, long>, int> division;
  TonewheelRegistration reg;
  // The Rotary's planned speed changes (tick, fast); empty when the file sets it itself.
  std::vector<std::pair<long, bool>> rotary;
  // Nudges of the slow Rotary on long held notes (tick, the slow speed x this; 1 back to slow).
  std::vector<std::pair<long, double>> rotaryBoost;
  // The harmonic percussion's planned switches (tick, on): at most two, mostly none (the piece's
  // choice is in reg).
  std::vector<std::pair<long, bool>> percussion;
  // The re-registrations at the general pauses (tick, the upper's drawbars from there): a drawbar
  // pulled out at each, toward the end; the lower manual and pedals follow (lowerExtra: its
  // balance's extra softness, steps).
  std::vector<std::pair<long, std::array<int, 9>>> upperChanges;
  // (per change: by the preset key, at once, where the pause is too short to pull a drawbar)
  std::vector<bool> upperInstant;
  double lowerExtra = 0;
  // The lower manual on a shape of its own (tonewheelLowerShaped): 1 008880080, the percussion on, the
  // left hand comping above the pedals' bass, an extra octave of percussion-like sound; 2 the hollow
  // 8', a soft line set apart from the tune by colour, not left a whisper. 0: derived.
  int lowerShape = 0;
  // The lower manual's vibrato tab switched (tick, on): off through the planned fast phrases and
  // back after (C3 under a fast Rotary muddies the low accompaniment; the upper keeps it).
  std::vector<std::pair<long, bool>> lowerVib;
  // (the lower manual's and pedals' clarity, 0..1: less 16' (and 5 1/3') under a moving pedal line
  // or rich harmony; see tonewheelLowerFor, tonewheelPedalFor)
  double clarity = 0;
  // The channels whose expression (CC11) is the organ's pedal: the tune's (mostly upper notes).
  std::set<int> exprChannels;
  // Velocities the plan plays instead of the file's ((channel * 128 + pitch, on-tick) -> velocity):
  // a trill's, rising through it (its key clicks sharpening).
  std::map<std::pair<int, long>, int> velocity;
  int trills = 0, trillFrom = 0, trillTo = 0;
  // The registration thinned for the trills ((tick, 0..1): from a trill's start the drawbars glide
  // toward the 8' and 4' alone, back after it): a trill on a thick registration is a blur.
  std::vector<std::pair<long, double>> thin;  // (how many; the first one's touch, first and last note)
  // Notes let go early so the next strikes the single-trigger percussion ((channel * 128 + pitch,
  // on-tick) -> the tick it is released): a legato line's accented notes detached, as jazz organists
  // play to choose which notes ping.
  std::map<std::pair<int, long>, long> release;
  // The expression pedal's planned shape over its riding (tick, x it), linear between the points:
  // the final chord swelled or closed.
  std::vector<std::pair<long, double>> swell;
  // The run switch (tick, on): off on a rock organ's final chord, the generator coasting down.
  std::vector<std::pair<long, bool>> run;
  // Notes the plan adds: a glissando's white keys swept up into a climax's first note.
  struct Extra { long on, off; int pitch, vel, div; };
  std::vector<Extra> extra;
  std::string summary;
};

// fileRotary: the file sends its own Rotary controls (CC1, CC64, CC82 on the part's channels);
// fileExpr: its own expression pedal (CC11). secondsAt: a tick's time by the file's tempo map (the
// ending's length, a ritardando's included); empty: by bpm.
TonewheelPlan planTonewheel(const std::vector<TonewheelNote>& notes, int tpq, double bpm, const std::string& instrument,
                        bool fileRotary, bool fileExpr = false, std::function<double(long)> secondsAt = nullptr);

}  // namespace pajupilli
