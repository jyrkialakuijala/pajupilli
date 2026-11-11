# pajupilli

A real-time physical model of a Boehm concert flute (C foot), played from an
Akai MPK mini mk3.

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

Options: `-dev hw:1` (another ALSA device), `-period 64 -periods 3` (buffer),
`-transpose 12` (the MPK's keys start at C3; notes fold into C4–C7),
`-midi NAME` (another MIDI controller).

Latency, key to first sound: ≈ 10–15 ms (1.3 ms periods; PipeWire is asked
for the same quantum).

### Controls (MPK mini mk3, default program)

| Control | Does |
|---|---|
| Keys | Notes. Overlapping keys **slur**. A new key soon after release is a soft "du"; from silence, velocity chooses a breath attack (soft), "tu", or an accented "t" (hard). |
| Joystick X | Roll the flute in / out: pitch bend through lip coverage |
| Joystick Y | Breath push |
| Sustain pedal | Keep blowing between keys (every key slurs unless hit hard) |
| Pad 1 (hold, press harder) | Breath controller: pad pressure is the breath |
| Pad 2 (hold) | Flutter tongue; pressure = depth |
| Knob 1 (CC70) | Dynamics: breath and lip opening |
| Knob 2 (CC71) | Brightness: jet offset at the edge |
| Knob 3 (CC72) | Breath noise |
| Knob 4 (CC73) | Vibrato depth |
| Knob 5 (CC74) | Vibrato rate 4–7 Hz |
| Knob 6 (CC75) | Attack hardness |
| Knob 7 (CC76) | Release time |
| Knob 8 (CC77) | Volume |

The knobs start from their defaults until you turn them.

## Offline

    ./flute -note 69 -o a4.wav   # one note, prints its pitch
    ./flute -demo -o demo.wav    # a scale and a phrase
    ./flute -cal                 # the per-note tuning the player learns at startup
    ./flute -imp 69              # passive resonances of a fingering

Every model idea is an environment variable (`PJ_*`, see `src/flute.cc` and
`src/player.cc`), e.g. `PJ_BREATH=0.03 ./flute -live`.

## Status

- C4–B5 in tune within ±4 cents after calibration; C6–C7 are 10–35 cents sharp.
- The tone is still brighter (stronger odd harmonics) than a real flute.
- Not yet: the look-ahead expressive mode, trills, harmonics, multiphonics,
  whistle tones, key clicks.
