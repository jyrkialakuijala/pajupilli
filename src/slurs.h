// What the players have practised (tools/slurs.py): how each slur is best played, and how fast
// each trill can go. A wind player knows in advance what a slur needs: where the lips lead the
// fingers, where the air pushes or eases, where the tongue touches. The practice plays every
// slur on the model, by interval, register and dynamics, searches the gesture that gets to the
// new note soonest and cleanest (no other partial on the way, no hole, no overshoot), and writes
// the table (src/slur_data.h; never edited by hand: tools/slurs.py emit).
//   - A general row per interval, direction and dynamics: the instrument's way with that slur.
//   - Rows of their own for the slurs the general way fails or is slow on (a register break, a
//     slur down onto a note the old one is a harmonic of, a low partial).
//   - A slur nobody found a way to play: `fails`; the player tongues it lightly instead, and
//     -play's director can know in advance (slurPlayable).
//   - The trills: the fastest notes per second each still speaks at, both notes arriving.
// PJ_SLURS=0: the players' ways before the practice. PJ_SLURTRY=1 with PJ_SLURTRY_FINGERS,
// _LEAD, _AIR, _AIRT, _LIP, _LIPT, _DIP, _DIPT, _TONGUE, _FLOOR: one gesture for every slur
// (the practice's own trials; these do not touch the calibration's cache).
#pragma once

namespace pajupilli {

struct SlurGesture {
  float fingers = 1;   // the fingers' (valves') speed, x (1: ~10 ms)
  float lead = 0;      // s: the lips and the breath go to the new note this long before the fingers
  float air = 0;       // the breath at the change: x (1 + air), dying away over airT (< 0: eased)
  float airT = 0.03f;  // s
  // The lips at the change: x exp(lip e^(-t / lipT) - dip (e^(-t / dipT) - e^(-t / lipT))): a lead
  // beyond the new note, then an approach from below. A reed: of the lip force. A horn: of the
  // lips' resonance, in units of the gap to the next partial (same partial: of the interval), lip
  // signed with the slur's direction.
  float lip = 0, lipT = 0.03f, dip = 0, dipT = 0.08f;
  float tongue = 0;    // s: a touch of the tongue at the change (0: none)
  float floor = 0;     // ... how far it leaves the air through (0 .. 1)
  bool fails = false;  // no gesture found: not a slur this player can play
};

// The gesture for a slur from key `from` to key `to` (sounding MIDI) at dynamics dyn (0..1).
// False: the instrument has not practised (the player's old way).
bool slurGesture(const char* inst, int from, int to, double dyn, SlurGesture* g);
// Can the slur be played (true too for an instrument that has not practised)?
bool slurPlayable(const char* inst, int from, int to, double dyn);
// The fastest trill between `lo` and `lo + interval` (notes per second: a trill at 6 Hz is 12);
// 0: not measured. Wider than the widest measured: that one's.
double trillLimit(const char* inst, int lo, int interval);
// What to do with a trill written at `rate` notes per second: 1 as written, 2 or 4 at a half or
// a quarter of the speed (the fastest of those the player can do), 0: none of them, an ornament
// instead (a turn at the start and the main note held).
int trillDivisor(const char* inst, int lo, int interval, double rate);

}  // namespace pajupilli
