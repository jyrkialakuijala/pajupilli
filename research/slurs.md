# Practised slurs and trills

The wind players practise each slur in advance (owner: "the instrumentalists could 'practice' how
to do each transition most effectively and know the 'tricks' for stable transitions in advance").
The result is a table the players read at each slurred key (src/slurs.h, src/slur_data.h), and the
fastest speed each trill still speaks at. Horn, clarinet and oboe so far.

## 1. The pieces

| What | Where |
|---|---|
| The gesture and the lookup | `src/slurs.h`, `src/slurs.cc` |
| The table (generated, never edited by hand) | `src/slur_data.h` |
| The practice's results (the table's source, kept) | `tools/slurs/INST.json` |
| The practice, the trills, the table writer, the check | `tools/slurs.py` |
| The whole recalibration | `make slurs` (one instrument: `make slurs I=oboe`) |
| The players that use it | `src/hornplayer.cc`, `src/reedplayer.cc` |

`PJ_SLURS=0` gives the players' ways before the practice (the horn's fixed lip lead and air push,
the reeds' plain slur with a tongue touch on a slur down an octave, a twelfth or two octaves).

## 2. The gesture

Ten numbers a slur (`SlurGesture`):

| Number | What the player does | Range searched |
|---|---|---|
| `fingers` | the fingers' (valves') speed, x (1: about 10 ms) | 0.4 .. 2.5 |
| `lead` | the lips and the breath go to the new note this long before the fingers | 0 .. 40 ms |
| `air`, `airT` | the breath x (1 + air) at the change, dying away over airT (negative: eased) | -0.7 .. 1; 10 .. 150 ms |
| `lip`, `lipT` | the lips beyond (or short of) the new note at the change, dying away | -0.5 .. 0.6; 10 .. 120 ms |
| `dip`, `dipT` | then an approach from below (negative: from above), slower | -0.3 .. 0.5; 30 .. 250 ms |
| `tongue`, `floor` | a touch of the tongue at the change, and how much air it leaves through | 0 .. 12 ms; 0 .. 0.6 |

A reed's `lip` and `dip` are of the lip force on the reed (log). A horn's are of the lips'
resonance, in units of the gap to the next partial (a slur on one partial: of the interval), `lip`
signed with the slur's direction: the horn's old fixed way is lip 0.3, air 0.5 up; lip 0.3, air
0.3, dip 0.18 down.

The table has, for each instrument:

- a general row per interval (1, 2, 3, 4, 5, 7, 9, 12 semitones up and down) and dynamics (0.3,
  0.5, 0.75): the instrument's way with that slur wherever it starts;
- rows of their own for the slurs the general way fails or is slow on;
- `fails` rows: no gesture found. The player tongues those lightly ("du", "da") instead of
  slurring, and `slurPlayable()` lets -play's director know in advance.

Lookup (`slurGesture`): the slur's own rows if it has any, else the general rows of the nearest
practised interval in the same direction; between the two dynamics around the player's, the
numbers are interpolated (the tongue's touch: the nearer dynamics').

## 3. How a slur is judged

`tools/slurs.py` plays `on A, wait, on B, off A, wait` through `-seq` (the player steady:
`PJ_WANDER=0`), tracks the pitch (McLeod's normalized square difference, 2 ms hop) and the level,
and reads:

| Number | Meaning | Its weight in the cost (ms) |
|---|---|---|
| settle | ms from the key until pitch and level stay at the new note's | x 1 |
| over | cents the pitch goes outside the two notes (up to 150) | x 1.5 above 3 c |
| junk | ms on something else (another partial, more than 150 c outside) | x 2 |
| dip | dB the level falls below the softer of the two notes | x 6 above 3 dB |
| bump | dB above the louder of the two | x 6 above 2 dB |
| hole | ms more than 6 dB under the softer note | x 1 |
| fail | not at the new note at the end, or never steady | 600 |

"At the new note" is by the note's own behaviour when held: its steady pitch (not the tempered
one), within twice its own unrest (at least 10 c), its level within 2 dB plus its own unrest. Some
notes sway by themselves (section 7), and a slur is not blamed for that. A slur from a note that is
not steady, or to a note that does not speak when simply tongued at that dynamics, is left out:
not a matter for the slur.

A gesture found for one slur must work on two occasions (the worse counts): the notes held 0.6 s
at the dynamics with a key of 64, and held 0.4 s a little louder (+0.04) with a key of 70.

## 4. The search

1. The general way: for each interval and direction, at mezzo-forte, over a start note every minor
   third; a coordinate search (each number stepped up and down, the best single step or all the
   helpful steps together, the steps then halved), starting from the interval before. Then soft
   and loud from the mezzo-forte result. The cost of a group is 0.7 x its mean + 0.3 x its worst
   slur: a way that leaves one slur failing is not learned.
2. Every slur from every note with its general way, on both occasions. Those that fail, or cost
   more than 1.5 x (or 40 ms more than) the group's typical cost, practise alone.
3. Each of those: the same search on its own, from the general way or its neighbour's own way
   (the table stays smooth). Kept if a fifth better than the general way; `fails` if still not
   there.

During the search the table is out of the way: `PJ_SLURTRY=1` with `PJ_SLURTRY_FINGERS`,
`_LEAD`, `_AIR`, `_AIRT`, `_LIP`, `_LIPT`, `_DIP`, `_DIPT`, `_TONGUE`, `_FLOOR` plays one gesture
for every slur. These do not touch the calibration's cache (the practice plays the calibrated
instrument; a new table changes the binary, the instrument calibrates again with it, and
`verify` plays that).

## 5. Recalibration

`make slurs` (or `python3 -I tools/slurs.py recalibrate [INST ...]`):

1. `practice` each instrument: writes `tools/slurs/INST.json`;
2. `emit`: rewrites `src/slur_data.h`; rebuild;
3. `trills` with the practised slurs built in: adds them to the json; `emit` again; rebuild;
4. `verify`: every slur from every note on both occasions with the table built in.

Needed after a change to an instrument's design, its calibration or its player. At most 12
worker processes (`-j`). The first run after a rebuild goes alone: the instrument calibrates
then, on many threads (`PJ_THREADS` limits them).

Single steps: `survey INST` (a quick look with the built-in table), `one INST FROM TO DYN
[PJ_SLURTRY_...=V] [-w FILE.wav]` (one slur's numbers and its sound).

Adding an instrument: a line in `INSTRUMENTS` and in `START` in `tools/slurs.py`; its player must
read `slurGesture` (the flute's player does not yet).

## 6. Results (2026-11-24)

Every slur of 1, 2, 3, 4, 5, 7, 9, 12 semitones up and down from every note at dynamics 0.3, 0.5,
0.75, on both occasions, the worse counted (`verify`); old: `PJ_SLURS=0`.

| | Horn old | Horn practised | Clarinet old | Clarinet practised | Oboe old | Oboe practised |
|---|---|---|---|---|---|---|
| slurs played | 1297 | 1305 | 1323 | 1323 | 902 | 880 |
| fail | 52 | 15 | 13 | 4 | 30 | 21 |
| settle, median (ms) | 152 | 126 | 43 | 39 | 27 | 30 |
| settle, nine in ten (ms) | 244 | 222 | 113 | 86 | 139 | 76 |
| outside the two notes, median (c) | 25 | 15 | 0 | 0 | 0 | 0 |
| ... nine in ten (c) | 51 | 38 | 24 | 12 | 20 | 14 |
| level dip, median (dB) | 7.1 | 5.1 | 1.5 | 1.5 | 2.1 | 1.9 |
| ... nine in ten (dB) | 14.9 | 13.2 | 10.6 | 6.8 | 12.4 | 9.2 |
| hole, nine in ten (ms) | 38 | 26 | 18 | 4 | 10 | 6 |

The table: 48 general rows an instrument; rows of their own: horn 179 (39 `fails`), clarinet 238
(2), oboe 188 (17). About 60 000 slurs played per instrument, 2.5 minutes each on 12 workers.

What the players found (the general rows, tools/slurs/*.json):

- **Oboe.** Up: faster fingers, the lips eased at the change (lip -0.15 .. -0.4 for 10 .. 40 ms),
  a little more air. Down: the lips eased more (-0.4 .. -0.5 for 10 ms) and an approach from
  below. The slurs that landed an octave high (C#4 -> C4, E5 -> E4, anything down onto C4 .. F4)
  are cured by easing the lips or by the lips going 15 ms before the fingers; the tongue is
  needed on few (29 rows).
- **Clarinet.** Up: fast fingers (x 1.8 .. 2.1), the lips eased. Down: the lips firmer for 10 ms,
  then an approach from above over 150 .. 200 ms; a slur of a sixth or more down: the lips 15 ms
  ahead and more air.
- **Horn.** Fast valves (x 2.5) everywhere. Up: the lips 15 ms ahead for a third to a fifth, the
  air pushed (0.5) for 20 .. 40 ms, a light tongue (2.5 ms, a little air through) from a fourth
  up. Down: a strong push of air (0.8 .. 1.0) for 10 .. 20 ms and the old approach from below.
  The horn still arrives 15 c outside on a typical slur and takes 125 ms: the least improved.
- **Not playable** (`fails`, tongued instead): the horn's lowest notes (F2 .. A2 by step, the
  octave down to F#2 / G2), a few soft slurs in its low register; the oboe's slurs from A#3 and
  C4 upward by a third to a fifth, the octave up from E4.

Not yet: nobody has listened. listen/slurs-{horn,clarinet,oboe}-{old,practised}-listen.wav is the
same slurred line both ways.

## 7. Trills

`trills` plays each trill (the lower note, then 14 alternations) at 3, 4, 6, 8, 10, 12 and 14
notes per second, mezzo-forte, with the practised slurs. A trill speaks at a speed if 85 % of its
notes arrive: within 30 c of the note for 60 % of the note's last 60 %, clear, and no more than
8 dB under the first note. The limit is the fastest speed with every slower one speaking too. 14
notes per second (7 Hz) is the top tried: about what fingers do; the model is not asked for more.

| | Horn | Clarinet | Oboe |
|---|---|---|---|
| semitone: at 14 / 8-12 / 3-6 / none | 14 / 11 / 5 / 6 of 36 | 30 / 0 / 0 / 2 of 32 | 24 / 0 / 1 / 2 of 27 |
| whole tone | 7 / 11 / 14 / 3 of 35 | 30 / 1 / 0 / 0 of 31 | 20 / 0 / 2 / 4 of 26 |
| minor third (tremolo) | 5 / 16 / 10 / 3 of 34 | 30 / 0 / 0 / 0 of 30 | 21 / 0 / 1 / 3 of 25 |
| major third | 1 / 13 / 17 / 2 of 33 | 27 / 2 / 0 / 0 of 29 | 19 / 1 / 1 / 3 of 24 |

- The reeds trill at the fingers' speed nearly everywhere. Not at all: the clarinet's B3 - C4 and
  G4 - G#4; the oboe's around C4 .. D#4 and its top (A5, B5, C6).
- The horn is slow: from C#3 to E3 and from G#3 up most semitone trills reach 8 .. 14; below
  C#3 3 .. 6 or none; whole tones mostly 4 .. 12. (A real horn's lip trill is a whole tone in the
  upper register at 5 .. 6 Hz, 10 .. 12 notes per second; lower down players do not trill.)

`trillLimit(inst, lo, interval)` gives the limit, and `trillDivisor(inst, lo, interval, rate)`
what to do with a written trill: 1 as written; 2 or 4: at a half or a quarter of the speed (the
owner: "2x/4x slower trills can be an option, too"); 0: none of these, an ornament instead. The
owner's ornaments for a trill that fails ("-" the upper note, "_" the main one):

    --_-___     a short turn, then the main note held
    __-___      one touch of the upper note
    --_----     the upper note held after a turn

Not built: nothing in -play finds trills in a file yet or replaces them (src/director.cc has no
trill detection; it is the other session's file just now). The limits and the divisor are ready
for it.

## 8. Seen on the way, not changed

- Some notes sway by themselves when held, with the player steady (`PJ_WANDER=0`) and the ear
  off: the oboe's B4 +-17 c and 2 dB about 4 times a second; the clarinet's G4 (the throat)
  +-20 c every 0.4 s. Not a slur's matter; the measure allows each note its own unrest.
- Notes that do not speak at some dynamics when simply tongued (left out of the practice): the
  oboe's B3, C4 and G#4 (soft or loud), E5 at times an octave low, C#6 (the top note) a ninth low.
- The clarinet's G#4 is 10 dB under G4 at mezzo-forte.
- After a rebuild every first run calibrates on all cores; twelve started together made a load of
  160. tools/slurs.py warms the cache with one run first, and the calibration now keeps to
  `PJ_THREADS`.

## 9. Next

- The flutes (the owner: "open hole flutes may have faster and more complete transitions, as the
  valve leaks more completely"): the flutist (src/player.cc) does not read the table yet; an
  open-hole flute would be its own instrument row, practised and compared with the plateau keys.
- The other reeds and brass (bassoon, English horn, saxophones, trumpet, trombone, tuba): a line
  each in tools/slurs.py; their players already read the table.
- The horn: 125 ms and 15 c outside on a typical slur. The gesture has no number for the lips'
  setting on arrival (a slurred note sits sharper than the tongued one it was calibrated as).
- -play: legato decided by the director (kCueLegato is computed and nobody reads it), trills
  found and replaced, phrase ends.

## 10. The horn's soft playing (2026-11-24)

The owner, on the practised slurs: "horn still has a note that doesn't light up -- is that a
problem in the horn itself or how it is played". Taken as C4 (five times in the listening line,
6 .. 9 dB under the E4 and G4 around it).

- How it is played, mostly. A ramp of the breath with the lips fixed (`-ramp 60`): C4 holds its
  register up to 3 x the breath. With 1.4 x the breath it is 6.7 dB louder and its third harmonic
  6 dB stronger, but 5 .. 11 c sharp, the lips at 0.90 not bringing it down. The calibration
  takes the in-tune setting: 0.7 x the nominal breath, just over the pressure where the note
  starts to speak, and finds nothing louder it accepts (loudest dynamics 0.55).
- Most notes below G#4 are capped at mezzo-forte the same way (25 of 37 under 0.85); C#4 and D4
  are 12 .. 15 c sharp at any breath and have no dynamics at all (the horn's own intonation on
  those valves, it seems). The same at HEAD. Not changed: the loud side is still open.

Then the owner's horn teacher's notes on playing softly ("keep air speed high, reduce aperture
size"; "bring the lips together gently"; the air pressed before the tongue lets go; a light
"da", breath attacks). The player went soft by the breath alone (x e^(1.2 (dyn - 0.5))), the
lips' opening as at mezzo-forte. Now, below mezzo-forte: the rest opening x e^(-s (0.5 - dyn))
(`PJ_HORNSOFTLIPS`, s), and the breath's fall held back by a share (`PJ_HORNSOFTAIR`).
Tongued notes, every second one of the range, with each setting's own calibration:

| breath kept | lips s | notes that play softly (dynLo <= 0.25) | pp under mf | off pitch at pp | onset at pp |
|---|---|---|---|---|---|
| 0 | 0 (before) | 12 of 37 | 1.3 dB | 6.6 c | 135 ms |
| 0 | 1 | 22 | 3.2 | 5.2 | 132 |
| 0 | 2 (now, the horn) | 27 | 5.1 | 4.4 | 123 |
| 0 | 3 | 25 | 5.9 | 3.8 | 115 |
| 0.5 | 1.5 | 28 | 2.9 | 5.6 | 109 |
| 0.5 | 3.5 | 18 | 3.5 | 4.5 | 107 |
| 0.7 | 3 | 17 | 2.7 | 5.0 | 103 |

The lips together is what lets the soft notes speak; the breath kept up starts them sooner but
leaves little of the dynamics (in the model a narrower opening at the same breath is not much
softer). The horn alone has it (lips 2, the breath as before); the other brass: 0, not tried.
The attack's part of the notes (the air before the tongue, "da", the breath attack) the player
already did.

The horn's slurs were practised again on the new calibration: 10 of 1244 fail (15 of 1305
before), nine in ten settle within 222 ms. Open: `brass` "horn: notes sound and tune" counts 8
notes over 10 c (limit 7), with `PJ_HORNSOFTLIPS=0` too (7 this morning: the low F#2 .. B2 and
C#4, D4). listen/horn-soft-{slurred,tongued}-{blown-less,lips-closer}-listen.wav: the line at
dynamics 0.15 both ways.

## 11. The horn's bell turned with the dynamics (2026-11-24, off by default)

The owner: "what about turning the horn down when playing pianissimo and flaring it towards the
listener when playing forte?" A bell beams above ka ~ 1 (research/horns.md 5.3; the horn's from
~400 Hz): on its axis the upper partials are up to 6 .. 10 dB over the all-round mean, off it
10 .. 20 dB under. The model radiates the all-round mean and had no direction.

`PJ_BELLTURN=1` (src/hornplayer.cc): the sound above `PJ_BELLTURN_HZ` (400) by
`PJ_BELLTURN_SOFT` (-8) dB at pp, 0 at mezzo-forte, `PJ_BELLTURN_LOUD` (+8) dB at ff, the
instrument moving over 0.15 s; a first-order shelf on the player's output. By the dynamics
asked for, not the note's calibrated range: a note whose lips are capped at mezzo-forte
(section 10) still has its bell.

Tongued notes A2 .. F5 every major third, A-weighted:

| | pp (0.1) | mf (0.5) | ff (0.9) | pp to ff | above 1.5 kHz, pp / mf / ff (dB re the whole) |
|---|---|---|---|---|---|
| bell still | 40.1 | 43.6 | 45.5 | 5.4 dB | -26.6 / -25.8 / -23.6 |
| turned -8 .. +8 | 37.3 | 43.6 | 50.1 | 12.8 dB | -29.8 / -25.8 / -21.9 |
| turned -12 .. +12 | 36.7 | 43.6 | 52.9 | 16.2 dB | -31.5 / -25.8 / -21.5 |

Off until heard: it changes the horn's loudness against velocity, which src/velgain_data.h
was fitted to. listen/horn-crescendo-{bell-still,bell-turned,bell-turned-more}-listen.wav: a
slurred line, the expression from 0.05 to 1 and back. Not done: the room (the bell's direction
for the direct sound and each reflection differently; here one shelf for all of it), the hand.

### 11.1 The owner's verdict: the wide turn is the default (2026-11-24)

"horn-crescendo-bell-turned-more-listen.wav is best from the dynamics point of view": the horn's
bell now turns by default, -12 dB at pp .. +12 dB at ff above 400 Hz (PJ_BELLTURN=0: still). Two
things followed:

- The bell follows the key as asked (bellVel_), not the velocity clamped to the note's calibrated
  range: the clamp had left a hard key on C4 (lips capped at mf) with +0.7 dB of the +8 it asked
  for. C4, A-weighted, keys 8 / 32 / 64 / 96 / 127: bell still 34.3 / 36.6 / 39.7 / 40.2 / 40.2 dB,
  turned 31.4 / 34.6 / 39.7 / 43.1 / 46.5 (6 dB -> 15 dB: the note "lights up").
- The horn's row in src/velgain_data.h refitted by tools/velcurve.py's measure (program 61 alone,
  research/gm_compare/work/slur/velrow.py; not a full -emit): the raw curve now covers 16 dB of
  General MIDI's 40 between keys 8 and 127, the row the rest.

Also the owner, on the soft-playing pair: "lips-closer is better than old way" (already the default).

## 12. Key velocities: a line of expansion, or a note strong from its start (2026-11-24)

The owner: "when there is a phrase and the midi file request higher velocity notes for a
wind/brass/string instrument, then it needs to be interpreted if it is a stronger note from the
very beginning or if it is belongs to a line of expansion".

Before, a wind part's key velocity did both things at once on every note: a louder key was a
harder attack and a louder note, from its start, so a written crescendo came out as steps.

Now (src/velline.h: readVelocities; wired into the winds' own reading, src/director.cc, used
when the director stays out of a file with its own performance; PJ_VELLINE=0: as before):

- per phrase (a beat's rest ends one) the line at a note is the middle of its velocity and its
  neighbours'; what the note has over or under it is its accent (under 6 steps: none);
- inside a run of three or more notes rising or falling, where the notes are bound, the line
  glides through each note to the next one's level; a single step that stays is a terrace;
- the line goes into the part's level curve (the breath, and the player's loudness: a unit of
  level ~20 dB, General MIDI's velocity curve 33.3 dB a decade); the key is played at the
  part's median line velocity plus the note's accent: an ordinary attack on a line note, a
  strong one from its beginning on an accent.

The horn's bell turn (11.1) already gives ~20 dB per unit of level, so the director's gain on
top (PJ_DIRECTOR_WINDDB 16) is not applied to the horn; with it the line came out twice as wide.

A horn test (research/gm_compare/work/velline/mk.py: a slurred scale 38 .. 112 .. 40, accents of
110 on a line of 63, a terrace 51 -> 97), each note's level at its start, dB re the peak:

  crescendo, as written keys:  -61 -24 -18 -18 -17 -17  -9 -12 | back -14 -11 -15 -19 -14 -19 -26
  crescendo, read as a line:   -33 -24 -18 -19 -17 -17 -16 -11 | back -14 -17 -16 -21 -16 -22 -30

(the first note now speaks from its start; the jump of 8 dB into the seventh note is gone). The
accents and the terrace keep their levels within 1-2 dB.

Not done: bowed strings (the director's reading covers wind lines only; a DirInput::bowed and
the string players' level would be needed); the accent's decay back to the line inside the note
(an accented note is louder throughout); the flat remaining lumps are the horn's own notes.

Tests: tools/test.py velocity_reading.

## 13. The practice rebuilt: speed, starts, trills, more of the embouchure (2026-11-25)

The practice of sections 2-7 knew one situation: a slur between two settled notes, on a steady
player. Music asks for more, and each of these is now rehearsed on its own.

### 13.1 More of the embouchure (`SlurGesture`: aperture .. setT)

Six more numbers, the embouchure's set through the change, x e^(-(t / setT)^2):

| Number | What the player does | Range searched |
|---|---|---|
| `aperture` | brass: the lips' opening x e^aperture (narrower: faster air) | -0.6 .. 0.4 |
| `firm` | firmer or looser lips: a brass player's lips' Q, a reed's lips' damping, x e^firm | -0.8 .. 0.8 |
| `focus` | the breath focused: its turbulence x e^-focus | 0 .. 2 |
| `vowel`, `vowelAt` | the tongue shapes the mouth into a resonance, vowelAt octaves above the new note | 0 .. 1; -0.5 .. 2 |
| `setT` | how long the set is held | 20 .. 250 ms |

The vowel is new physics in `src/horn.cc` and `src/reed.cc`: the throat's inertance on the mouth's
volume, one resonance (Q 6, `PJ_TRACTQ`) the lips or the reed see upstream, as the harmonica's
tract. With `tract` 0 (the players outside a slur or a start) the mouth is the plain volume it
was, sample for sample.

### 13.2 The slur's speed (`SlurRow::pre`, `slurGesture(..., sounded)`)

Every slur is practised twice: out of a settled note (held 0.6 s), and out of a note that has
only just spoken (about ten periods, 0.09 .. 0.2 s: a run's notes), where the lips and the breath
of the note before are still arriving. The quick search starts from the settled way. The player
knows how long the note it leaves has sounded and plays between the two (the logarithm of the
time, 0.1 .. 0.4 s). `PJ_SLURSPEED=0`: the settled way always.

A quick slur's note left is too short to measure in place: its level is the same note's tongued
alone.

### 13.3 The occasions, the level, the effort

- Three occasions a slur (the worst counts): as before the two lengths and key velocities, and
  one on the player as it is in music, the breath and the lips wandering (`PJ_WANDER=1`).
- The new note as loud, once settled, as tongued alone: 5 ms of cost per dB of step past 1.5 dB
  (it was the flutes' check only).
- Effort: a few ms of cost per unit of breath push, opening, firmness, vowel: among equally clean
  ways the plainest, and the table stays smooth.
- A slur that needs its own way: one costing more than 1.6 x (or 50 ms more than) the median of
  the same interval, dynamics and speed across the instrument, on the same occasions.
- One that still fails with its general way and its neighbours' is also tried with other
  techniques before the search (seeds): firmer, looser, a narrower opening, a vowel tuned half an
  octave and an octave and a half up, a touch of the tongue, no lip lead at all. One step at a
  time does not find what only helps together.

### 13.4 Starts (`AttackGesture`, `tools/slurs.py attacks`)

Per note and dynamics, the tongued start: the tongue's release and the breath's rise (x the
player's own), a push of the breath, the lips off the note at first, and the set (opening,
firmness, vowel). Judged like a slur: ms until the note is there, the scoop (cents), a crack (ms
on another partial, x 4), a bark (dB above 4 over the settled level), on four occasions: a key of
64, a hard key (100), the wandering player, and the note tongued again 0.12 s after its own end.
Up the instrument, each note from the best of the plain way, the note below's and seeds. A start
that gains less than a tenth keeps the plain way.

The calibration plays plain starts whatever the table says (`practisedAttacks(false)`), and its
cache does not depend on the practice's knobs (`PJ_ATTACKTRY*`, `PJ_SLURTRY*`, `PJ_WANDER`, ...):
before, every trial start calibrated the instrument again.

### 13.5 Trills (`TrillSense`, `tools/slurs.py trillways`)

A trill is not a row of slurs: the embouchure is held between the two notes and the fingers
alone move. A player senses a trill (a slur back, within 0.25 s, to the note left at the change
before) and plays its own way: one gesture for both directions, the table's rows with `pre` 0.
Each kind of trill is rehearsed apart:

| Kind | What is played |
|---|---|
| steady | 16 even notes |
| accel | steady at 1.5 x the note length, then quickening to 0.75 x |
| cresc, decresc | the expression rising or falling through the trill |
| swell50, swell75 | an emphasis half-way, or three quarters of the way in |
| linked | the turn under the main note and on into the next note, which must speak |

Judged by: the share of notes that do not arrive, how much of each note goes to arriving, how
unequal the two notes are (dB), the pitch (cents), and the linked note's arrival. The general way
per interval (1 .. 4 semitones) is searched on steady, accel and swell50, a step faster than the
instrument's middle manages as quick slurs; then every semitone and whole-tone trill plays every
kind, and those far worse than the others rehearse alone on all of them. Last, how fast each kind
goes on each note (`trills` in the json, by kind).

The director (src/director.cc, `PJ_TRILL_SHAPE`) chooses the kind: a short trill steady; a long
one steady at first and then quickening, growing into a higher note it is linked to, with a late
emphasis when linked otherwise, dying away at a phrase's end, else an emphasis half-way.

### 13.6 The process

`python3 -I tools/slurs.py recalibrate [INST ...]`: practice, attacks, trillways for each, emit,
rebuild, verify and verify-attacks. `audition INST -o FILE.wav`: the slurs, starts and trills that
changed most, before and after, for the ear. The workers play a private copy of the binary (its
path through the environment: they are processes of their own, and a peer's build took the shared
one away mid-practice).

Not done: releases; brass slurs indexed by partial instead of interval; alternate fingerings for
the slurs that still fail; the director slowing a trill written faster than `trills` says.
