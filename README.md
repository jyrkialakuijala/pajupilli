# pajupilli

A real-time physical model of a Boehm concert flute (C foot), played from an
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

### The flute family

| `-inst` | Sounds | Range (sounding) | Model rate |
|---|---|---|---|
| `flute` | as written | C4–C7 | 48 kHz |
| `piccolo` | an octave higher | D5–B7 | 96 kHz |
| `alto` | a fourth lower (in G) | G3–B5 | 36 kHz |
| `bass` | an octave lower | C3–D5 | 24 kHz |

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
  plus a small mechanical tick. `PJ_PADPUMP` (0.25) and `PJ_KEYCLICK` (1) set the levels.
- Not yet: the look-ahead expressive mode, trills, harmonics, multiphonics,
  whistle tones.
