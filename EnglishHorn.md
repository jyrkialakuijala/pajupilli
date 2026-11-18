# English horn: recalibration and improvements (2026-11-18)

Nothing is committed and `./pajupilli` is not rebuilt (another session is using it). `make` picks
everything up; `src/instrument.h` changed, so it is a full rebuild.

## What was wrong

The English horn was the *first* oboe fit scaled x1.5, with a reed that was not scaled with it.

| | before | after |
|---|---|---|
| Calibration (`-cal`), 28 notes | 1 silent, 4 over 10 c (up to +64 c), lips at their limit (x0.50) on 7 notes | all 28 within 2 c |
| Held notes (`-note`) at soft / medium / loud, 84 | 33 off by more than 15 c (mostly an octave up or down) | 0 |
| Phrases: 6 key velocities and slurs from neighbours, 316 notes (`tools/reedcheck.py`) | 102 off by more than 20 c | 2 |
| `-scale`, 28 notes | 10 in the wrong register or over 15 c | all within 1 c |
| Level spread over the notes at velocity 90 | 20.5 dB | 9.4 dB |
| Passive fit of the fingerings (`tools/design.py`) | top notes up to +93 c (the present scheme with the old reed: rms 23 c, second register 20..105 c sharp) | rms 4.2 c, worst 14 c |

The two phrase notes still wrong: E3 at velocity 20 (octave up), G3 slurred down from D4 (octave up).

## What was changed

1. **Its own bore fit, in the oboe's present scheme** (`tools/instruments.py`, `tools/designs/english-horn.json`,
   the English horn block of `src/designs.cc`). Staple and reed-end diameter are fitted, the octave
   vents sit where the oboe's do. The fit lands on almost the oboe's own values (top 1.308 against
   1.312, staple 102.4 against 100.6 mm at oboe scale), length 938 mm.
2. **The reed scaled with the bore.** Resonance 2000 Hz (the oboe's / 1.5), the oboe's damping (1/Q 0.9)
   and blades meeting over the last quarter of the closure, lips rolled out at the top. The
   compliance `sr / k` is 6.3e-12 m^3/Pa, the oboe's x 1.5^3: it is part of the cone's missing apex,
   and with the old 3.1e-12 (or 4.2e-12) no bore in the family brings the registers in line. Slit
   8 x 0.4 mm, closing pressure 9.6 kPa, breath 4.0..5.5 kPa as before.
3. **The player waits 0.15 s before tuning a note by ear** (`ReedParams::earStart`, `PJ_REEDEARSTART`;
   English horn and oboe). A reed's attack starts 50..100 c flat and settles by itself in ~0.1 s. The
   ear used to start 20 ms in, tightened the lips after the flat start, and pushed the reed past the
   note into its octave (written D#4, F5). Other reeds keep the old behaviour: with the delay on
   everything the shawm and the baritone sax got worse.
4. **The skilled start** the bassoon has (`prime` 1e-4): the column set ringing at the note as the
   tongue leaves.
5. **Reed calibration** (`src/calibrate.cc`, all reed instruments):
   - A reed has a state about a semitone flat of the note (and that state's octave). The calibration
     accepted anything within 150 c as "the note"; for a reed's dynamics it is now 25 c.
   - Each dynamic step is played four ways: from the breath control and from the key (a harder key
     is a harder attack, and the attack decides the state), each with the lips 5 % tighter and
     looser (the player wanders by 4 %).
   - The lips follow one line over the dynamics, tested in steps of 0.05 the way the player
     interpolates it. Before, only the end points were tested and the note fell between them.
   - Calibrating the English horn takes 1.1 s instead of 0.6 s (cached as before).
6. **The attack stops where the dynamics stop** (`src/reedplayer.cc`, all reeds): a key harder than
   the note's calibrated range used to get the limited breath but the full forte attack, a
   combination nobody had tested. Now the tongue, accent and fingers match the dynamics it gets.
7. **Level and velocity tables**: English horn trim -1.2 -> -3.1 dB, velocity gain row re-measured.
   The calibration change moved two other reeds: shawm -4.2 -> -1.7 dB, soprano sax -9.5 -> -9.1 dB.
   Only these rows were edited; the files were not regenerated.
8. `tools/design.py` writes `roll`, `prime`, `earStart`; the oboe's reed in `tools/instruments.py`
   now has the values `src/designs.cc` had by hand (1/Q 0.9, meetAt 0.75, roll 0.5). The generator
   reproduces the oboe and English horn rows. It still does not reproduce the bassoon's `prime`,
   the bagpipe's drone note or the oboe's comment, so `src/designs.cc` was spliced, not regenerated.

## Effect on the other reeds

Held notes off by more than 15 c, of (notes x 3 dynamics), before -> after (5 and 6 without the ear delay):

| oboe | bassoon | clarinet | soprano | alto | tenor | baritone | duduk | shawm | shehnai |
|---|---|---|---|---|---|---|---|---|---|
| 2 -> 1 | 4 -> 3 | 5 -> 5 | 9 -> 6 | 14 -> 5 | 10 -> 2 | 14 -> 18 | 11 -> 5 | 17 -> 11 | 24 -> 21 |

The baritone's extra ones are 16..27 c mistunings; its register errors went from 6 to 3.
`tools/test.py` woodwinds, played notes, brass and flute tuning pass (run on the HEAD copy of the
test file; the working copy has another session's changes).

## Not done, and what I would do next

- **Low notes have little dynamic range.** Sounding E3..A3 hold only between dynamics 0.45 and
  0.50..0.65: louder they fall into the flat state. Velocity still sets the loudness (the gain
  table), but the tone does not brighten. The cause is in the reed model, not the calibration.
- **Level spread 9.4 dB over the notes** (the oboe has 9.3). There is no per-note level trim.
- **The flat state itself** is not understood. It needs the reed's own air displacement (it goes
  away at 70 % of it), but smaller displacement cannot be fitted (see 2). It also exists on the oboe
  (F4 loud). A cavity volume at the reed, separate from the blade compliance, is the thing to try.
- **Bulb bell** not modelled (a plain flare). Long-term spectrum against the old one: 400..630 Hz
  4..6 dB lower, 2 kHz 3 dB higher, above 6 kHz 3..7 dB lower; the 1 kHz band leads (oboe: 1.6 kHz).
- Nobody has listened to it.

## Listening

    mpv /home/jyrki/pajupilli/listen/english_horn_scale_before.wav /home/jyrki/pajupilli/listen/english_horn_scale_after.wav
    mpv /home/jyrki/pajupilli/listen/english_horn_demo_before.wav /home/jyrki/pajupilli/listen/english_horn_demo_after.wav

## Checking

    tools/reedcheck.py ./pajupilli english-horn 51 78     # velocities and slurs, sounding MIDI range
    ./pajupilli -inst english-horn -cal ; ./pajupilli -inst english-horn -scale
