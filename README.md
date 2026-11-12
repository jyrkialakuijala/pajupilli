# pajupilli

A real-time physical model of a Boehm concert flute (B foot), played from an
Akai MPK mini mk3. Also the piccolo, alto flute and bass flute (`-inst`).

- The bore is a 1-D acoustic waveguide: 128 pressure points at 48 kHz, stored
  as 8 × 16 float32 so the loops compile to AVX-512. Geometry from a measured
  Pearl PF-661 (UNSW).
- 16 tone holes with pads that move in 10–16 ms, real fingerings C4–C7.
- An air jet driven by the sound in the embouchure hole, from the player's
  lungs, mouth, tongue and lips.
- About 0.5 % of one core.

See `REQUIREMENTS.md` and `research/` for the physics and the numbers.

## Build

    make            # AVX-512, fast-math (needs libasound2-dev)
    make scalar     # portable reference build

## Play

    ./flute -live           # finds the MPK mini, plays through PipeWire/ALSA
    ./flute -live -v        # also prints the MIDI events
    ./flute -selftest       # a short arpeggio without a keyboard
    ./flute -live -inst piccolo   # or alto, bass (default: flute)

Options: `-dev hw:1` (another ALSA device), `-period 64 -periods 3` (buffer),
`-transpose 12` (keys to sounding pitch; default: the MPK's lowest C plays the
instrument's lowest octave, notes outside its range fold in by octaves),
`-midi NAME` (another MIDI controller).

Latency, key to first sound: ≈ 10–15 ms (1.3 ms periods; PipeWire is asked
for the same quantum).

### Controls (MPK mini mk3, default program)

| Control | Does |
|---|---|
| Keys | Notes. Velocity sets the breath (soft: gentler, slower onset; hard: more pressure, a sharper attack) and finger speed (harder keys land the pads faster: louder key clicks). Overlapping keys **slur**. A new key soon after release is a soft "du"; from silence, velocity chooses a breath attack (soft), "tu", or an accented "t" (hard). |
| Joystick X | Roll the flute in / out: pitch bend through lip coverage |
| Joystick Y | Breath push |
| Sustain pedal | Keep blowing between keys (every key slurs unless hit hard) |
| Pad 1 (hold, press harder) | Breath controller: pad pressure is the breath |
| Pad 2 (hold) | Flutter tongue; pressure = depth |
| Knob 1 (CC70) | Dynamics: breath and lip opening |
| Knob 2 (CC71) | Brightness: jet offset at the edge |
| Knob 3 (CC72) | Air: focused → airy (lip opening, breath noise, edge hiss) |
| Knob 4 (CC73) | Vibrato depth |
| Knob 5 (CC74) | Vibrato rate 4–7 Hz |
| Knob 6 (CC75) | Attack hardness |
| Knob 7 (CC76) | Release time |
| Knob 8 (CC77) | Volume |

The knobs start from their defaults until you turn them.

### Polyphony (`-poly N`, default 8)

    ./flute -live            # a pool of 8 flutes
    ./flute -live -poly 1    # one flute only

The keys are dealt to a pool of flutes, each its own model and player (with its own
breath wander). How many play is guessed from the playing: keys struck together (within
35 ms) are a chord, one flute each; a key pressed while another is held is a slur on that
flute, as with one flute; keys held together for more than 100 ms are separate lines and
each older note moves to a flute from the pool (it re-enters with a fresh attack). The
number of lines is the most seen lately and drops back after 2 s with fewer; it prints
`flutes: N` as it changes. A new line takes the free flute that played nearest in pitch and
most recently; silent flutes are not computed. The mix is scaled by 1 / sqrt(flutes
playing). Controls (breath, vibrato, knobs) go to every flute. `./flute -polydemo -o
poly.wav` plays a line, a held note under a melody, and chords.

Flutes sounding together tune to each other (adaptive just intonation, `PJ_JI=0` turns it
off): every pair asks for its interval's nearest simple ratio (fifth +2 c, major third
-14 c, minor third +16 c, ...), simpler and closer intervals weighing more; a least-squares
solve gives each flute its offset (within ±30 c), weakly pulled to equal temperament, and
a note already sounding stays where it is so a new note fits the chord and progressions do
not drift. Each flute gets there with its lips, by ear. It prints `tune: C4 +1 E4 -12 G4 +3`.
In the demo the flutes reach their targets within 1-2 cents and the chords' measured
roughness (Sethares, from the partials) drops by 21 %.

### The flute family

| `-inst` | Sounds | Range (sounding) | Model rate |
|---|---|---|---|
| `flute` | as written, B foot | B3–C7 | 48 kHz |
| `flute-c` (or `-cfoot`) | as written, C foot | C4–C7 | 48 kHz |
| `piccolo` | an octave higher | D5–B7 | 96 kHz |
| `alto` | a fourth lower (in G) | G3–B5 | 36 kHz |
| `bass` | an octave lower | C3–D5 | 24 kHz |

The flute has a B foot by default (the "H-Fuß"): one more key, the C hole at 592 mm, and
the tube on to 633 mm from the embouchure; it adds low B3 and changes the high resonances.
Players drop it for the weight; here it costs nothing. The MPK's octave-down button reaches
the B. C7 is played without the gizmo (in the model the gizmo makes it 50 cents sharp, 29
without; the C foot's C7 is 36). `-cfoot` gives the C-foot flute.

Each is the concert flute resized: tube lengths by the pitch ratio, bores to the real
instruments' (piccolo ~10.5 mm, alto 26 mm, bass 35 mm), embouchure, lips and jet by about
the square root. Same Boehm fingerings; the piccolo has a D foot. Each runs at its own rate so
the bore keeps its 128 points, resampled to 48 kHz. Offline commands take `-inst` too, with
sounding MIDI notes (`./flute -inst bass -note 48`).

## Offline

    ./flute -note 69 -o a4.wav   # one note, prints its pitch
    ./flute -demo -o demo.wav    # a scale and a phrase
    ./flute -cal                 # what the player learns at startup (cached in ~/.cache/pajupilli)
    ./flute -imp 69              # passive resonances of a fingering
    ./flute -slur 71 72 -dyn 0.2 # slur into a note; pitch and level wobble
    make test                    # ~10 s of checks

The player: lungs → trachea → glottis (throat valve) → mouth → tongue → lips → jet.
Vibrato comes from the lungs (chosen by ear); `PJ_VIBTHROAT=0..1` moves it to the throat;
a soft key gives an "h" breath attack, the glottis opening from a slit.

Blowing technique without a knob: `PJ_WANDER` (0 machine-steady, 1 natural, 2 unsteady:
slow drift of breath support, jet aim and lips, and attacks that vary), `PJ_AIR` (the
Air knob's start value), `PJ_EDGE` (edge hiss level).

Every model idea is an environment variable (`PJ_*`, see `src/flute.cc` and
`src/player.cc`), e.g. `PJ_BREATH=0.03 ./flute -live`.

## Status

- C4–D#6 in tune within ±2 cents after calibration; E6–C7 up to ~30 cents. Piccolo, alto
  and bass within ±3 cents over their ranges (see PLAN.md, 2026-11-12).
- Known: register selection is still weak at the extremes (soft G5 and loud F5 jump up,
  G6 does not sound); see PLAN.md step 3.
- Tone: variant D (jet swing saturation, pitch-dependent jet aim, lip loss at the window) chosen by ear on 2026-11-11; still being refined.
- Key clicks: a closing pad accelerates into its seat; in the last fraction of a
  millimetre the trapped air goes into the bore (a pop pitched by the tube),
  plus a small mechanical tick. The pad then sinks into its felt and springs back: a
  low thud (~400 Hz) into the tube, both ways; a held pad lifting off gives a small suction.
  Keys moving together start 0..3 ms apart. At rest the player holds G (left hand down).
  `PJ_PADPUMP` (0.25), `PJ_KEYCLICK` (1), `PJ_THUD` (1), `PJ_RELEASE` (1) and
  `PJ_KEYSPREAD` (3 ms) set them.
- Trills on the D and D# trill keys (1st and 2nd): hold one note of a pair and alternate
  the other over it fast, either way round (each key event within 0.12 s of the last one of
  the pair, `PJ_TRILLFAST`; the first alternation is still played with real fingerings, so
  slow passages and scales never use the trill keys). The fingers stay on the lower note
  and the upper one sounds through the key(s), printed as it happens:

  | Trill (written) | Keys | Printed |
  |---|---|---|
  | C5-D5 | D | `trill-c5-d5` |
  | C#5-D5 | D | `trill-cis5-d5` |
  | C#5-D#5 | D# | `trill-cis5-dis5` |
  | B5-C#6 | D | `trill-b5-cis6` |
  | C6-D6 | D# | `trill-c6-d6` |
  | C#6-D6 | D | `trill-cis6-d6` |
  | C#6-D#6 | both (not on the piccolo) | `trill-cis6-dis6` |
  | D6-Eb6 | both | `trill-d6-dis6` |
  | A6-Bb6 | both | `trill-a6-ais6` |

  In the D6-Eb6 and A6-Bb6 trills the upper note keeps the lower note's embouchure (with
  its own, the flute's Eb6 fell an octave). B6 uses the D trill key as a vent (+1 cent;
  with the D# trill key +21); C7 is best with neither. On the flute the upper notes are within 10 cents; C#-D and C#-D# are sharp on their keys,
  so the player covers more for them (`PJ_TRILLCOVER` 0 turns that off).
  On a trill key the player listens and tunes with the lips, not the fingers: the coverage
  moves until the note is in tune (time constant 50 ms, `PJ_TRILLTUNE`; 0 = off), and the
  correction is kept for the next time. A note held on the key for 0.3 s prints
  `trill-c5-d5: held, lips +x c`. Held trill notes end within 3 cents (piccolo top: 7).
- Not yet: the look-ahead expressive mode, other trills, harmonics, multiphonics,
  whistle tones.
