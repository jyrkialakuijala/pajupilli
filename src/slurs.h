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
// _LEAD, _AIR, _AIRT, _LIP, _LIPT, _DIP, _DIPT, _TONGUE, _FLOOR,
// _APERTURE, _FIRM, _FOCUS, _VOWEL, _VOWELAT, _SETT: one gesture for every slur
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
  // The embouchure's set through the change, x e^(-(t / setT)^2) (held, then let go):
  float aperture = 0;  // brass: the lips' opening, x e^aperture (< 0: narrower, the air faster)
  float firm = 0;      // the lips firmer: a brass player's lips' Q, a reed's lips' damping, x e^firm
  float focus = 0;     // the breath focused: its turbulence (noise, airiness) x e^-focus
  float vowel = 0;     // the tongue shapes the mouth (0 .. 1: none .. a tuned tract, src/horn.cc)
  float vowelAt = 0;   // ... its resonance, octaves above the new note
  float setT = 0.06f;  // s
  bool fails = false;  // no gesture found: not a slur this player can play
};

// The gesture for a slur from key `from` to key `to` (sounding MIDI) at dynamics dyn (0..1).
// False: the instrument has not practised (the player's old way).
// sounded: how long (s) the note left has sounded: a slur out of a note that has only just
// spoken is played its own way (the quick rows: practised from a note 0.1 s old; a settled one's
// from 0.4 s on, between the two in between).
// trill: the slur is a trill's (TrillSense): its own way, one gesture for both directions (the
// embouchure held between the two notes, the fingers alone moving), practised as trills are
// played: steady, quickening, swelling, growing and dying away, and into the note after
// (tools/slurs.py trillways). None practised: the quick slur's.
bool slurGesture(const char* inst, int from, int to, double dyn, SlurGesture* g, double sounded = 1, bool trill = false);
// A player's sense of being in a trill (or a tremolo): the slur returns, quickly, to the note
// left at the change before.
struct TrillSense {
  int before = -1;
  bool at(int from, int to, double sounded) {
    const bool t = to == before && from != to && from >= 0 && sounded < 0.25 && (to > from ? to - from : from - to) <= 5;
    before = from;
    return t;
  }
};
// Can the slur be played (true too for an instrument that has not practised)?
bool slurPlayable(const char* inst, int from, int to, double dyn, double sounded = 1);
// A tongued start as practised (tools/slurs.py attacks): per note and dynamics, how the note
// speaks soonest and cleanest from silence (no crack onto another partial, no scoop, no bark).
// On top of the attack's kind (a "ta", a soft "da", an accent: the players' own).
// PJ_ATTACKS=0: the players' ways before the practice. PJ_ATTACKTRY=1 with PJ_ATTACKTRY_RELEASE,
// _RISE, _PUSH, _PUSHT, _LIP, _LIPT, _APERTURE, _FIRM, _VOWEL, _VOWELAT, _SETT: one for every note.
struct AttackGesture {
  float release = 1;   // the tongue's release time, x
  float rise = 1;      // the breath's rise time, x
  float push = 0;      // the breath at the start: x (1 + push), dying away over pushT
  float pushT = 0.05f; // s
  float lip = 0;       // the lips at the start: x e^lip (a reed: the lip force; a horn: the lips'
  float lipT = 0.04f;  // resonance), dying away over lipT
  float aperture = 0;  // as a slur's set (SlurGesture): the opening, the lips' firmness, the vowel
  float firm = 0;
  float vowel = 0;
  float vowelAt = 0;
  float setT = 0.06f;
};
// The start of key `key` (sounding MIDI) at dynamics dyn. False: not practised.
bool attackGesture(const char* inst, int key, double dyn, AttackGesture* g);
// The calibration plays plain starts (the practised ones are learned on the calibrated instrument).
void practisedAttacks(bool on);

// The fastest trill between `lo` and `lo + interval` (notes per second: a trill at 6 Hz is 12);
// 0: not measured. Wider than the widest measured: that one's.
double trillLimit(const char* inst, int lo, int interval);
// What to do with a trill written at `rate` notes per second: 1 as written, 2 or 4 at a half or
// a quarter of the speed (the fastest of those the player can do), 0: none of them, an ornament
// instead (a turn at the start and the main note held).
int trillDivisor(const char* inst, int lo, int interval, double rate);

}  // namespace pajupilli
