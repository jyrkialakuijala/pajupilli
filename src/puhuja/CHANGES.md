# puhuja in pajupilli: the record of changes

These files are a copy of puhuja's sources (the speech synthesizer: `puhuja.h`), brought in
so that pajupilli's lead singer can be built on it. The copy lives here and may be changed here
for singing. Every change is to be carried back into the puhuja project later, natively, so:

**Every change to a file in this directory gets an entry below, with its motivation**: what
singing needed that the speech synthesizer did not do, what was changed and where, and how it
was judged (a measurement, or by ear). An entry is written in the same commit as the change.
A change with no entry here is a mistake.

## Where the copy came from

- Project: puhuja (Apache License 2.0: `LICENSE`; the English pronunciation table derives from
  the CMU Pronouncing Dictionary: `LICENSE-CMUDICT`). Both projects have the same sole author.
- Commit: `b723192576763eb89e9b860ea5275bf335954ff7` (2026-11-25), "The chirp fix on:
  LINE_X_ARTIC_CLOSE_WAIT 2, LINE_X_ARTIC_DORSUM 0.76, ...". The working tree's sources were
  at that commit (its uncommitted changes were in the Makefile and two tools, not copied).
- Files: every source of the `puhuja` binary but `main.cc` (its command line): `puhuja.cc/.h`,
  `prosody.cc/.h`, `larynx.cc/.h`, `grid.cc/.h`, `block.cc/.h`, `string.cc/.h`, `folds.cc/.h`,
  `line.cc/.h`, `tract.cc/.h`, `english.cc/.h`, `english_data.inc`, `english_shape.inc`,
  `filters.h`, `params.h`, `tuned_defaults.h`, `fastmath.h`.
- Unchanged at the copy: byte for byte the upstream files.

## How it is built

Apart from pajupilli's own sources, into `build/libpuhuja.a` (the Makefile's `PUHUJA_FLAGS`:
puhuja's own flags, without `-ffast-math`, which its Makefile rules out: it lets glibc swap
exp/log/pow in vectorized loops). Everything is in `namespace puhuja`.

## Changes

### 2026-11-25: a pitch controller a singer can tighten
- Motivation: the lead singer (pajupilli's "Lead 6 (voice)") must land each note's pitch. Sung through the
  default voice (string folds, line tract), notes came out 25 to 240 cents flat and drifted inside the vowel.
  The folds' pitch controller learns only where the flow fluctuation exceeds kPitchLearnVib (98 cm^3/s); at
  sung pitches (110 to 260 Hz, male voice) the fluctuation measured 5 to 35, so the controller never engaged.
  The knobs are read once at static initialisation from the environment, so a host program cannot set them
  for one voice.
- Change: `LarynxControl` has `pitchControl` and `pitchLearnVib` (-1: the speaker's constants, as before);
  `FoldLarynx::step` (larynx.cc) uses them in the controller's relax, its learning gate and its gain.
  `Synth::setPitchControl(share, learnVib)` (puhuja.h) sets them; `Synth::tick` passes them on (puhuja.cc).
  Speech is unchanged: nothing calls the setter, and with -1 the code path computes the same values.
- Judged by: a measurement, not yet by ear. "dah doo dee", 500 ms a note, each key D2 to A4, male voice,
  share 0.6 and learnVib 3: the middle of every vowel within 13 cents of the key from D2 to C4 (it was 25 to
  240 cents flat on a test phrase). Above C4 the folds leave the pitch asked (D4 and up: 80 to 300 cents
  off, an octave low in places) and the level falls 10 to 15 dB; the female preset the same from E4. The
  first 150 ms of a note still scoop by 50 to 300 cents while the controller pulls in.
- For puhuja: a per-voice setting as here, or a singing mode that also sets the pitch anchors of a vowel
  (below). The range above C4 / D4 is the folds' own and needs work there, not in the controller.

### 2026-11-25: the tension a singer knows for a pitch
- Motivation: the author, on the singer's pitch: "probably a control system to maintain the pitch is needed --
  one control system with listening, like spectral matching, and another for knowing the approximate tension
  needed for the vocal cords". With the controller alone (the listening half), every note began 50 to 300
  cents off and was pulled in over its first 150 ms: the folds' tension for a pitch was learnt anew on each
  note, from the speaker's mapping (tension = pitch / kFoldNaturalF0).
- Change: `LarynxControl::tensionPrior` multiplies the folds' tension beside the controller's correction
  (larynx.cc, `FoldLarynx::step`). `Synth::setTensionMap(loHz, x)` gives it by pitch, a semitone an entry
  (puhuja.h; looked up in `Synth::tick`, puhuja.cc). `Synth::pitchCorrection()` (and
  `Larynx::pitchCorrection()`) reads the controller's correction, which is what a host learns the map from:
  it sings each pitch once and keeps the settled correction. Speech is unchanged: the map is empty, the prior 1.
- Judged by: see pajupilli's `src/jazzsinger.cc` (the practice at start-up) and its measurements; not yet by ear.
- For puhuja: the map belongs to the voice (learnt once per voice and fold model, stored with the preset),
  and should also depend on the vowel and the lung pressure, which move the pitch too.

### 2026-11-25: a low, still larynx
- Motivation: the author: "one needs to maintain the larynx in a relatively low position to have good voice
  -- if larynx is jumping up and down, it is difficult to sing in a controlled manner and voice quality will
  be bad". The line tract's larynx rises 1.5 to 2 mm in s, k, g and r (LINE_RAISE_*), right for speech.
- Change: `Synth::setLarynx(lowerCm, raiseScale)` (puhuja.h): the whole tract `lowerCm` longer, the consonants'
  rise x `raiseScale`, where the shape's length is set for the line (puhuja.cc, three places beside
  `lineExtra_`). Speech is unchanged (0 and 1).
- Judged by: not yet judged; pajupilli sings with 0.6 cm and 0 for now, values to be set by ear.
- For puhuja: a voice setting (a singer's posture), as here.

### 2026-11-25: a sung vowel holds its pitch from its beginning to its end
- Motivation: speech anchors a vowel's pitch at its centre (or, with a second pitch, at 0.25 and 0.9 of it:
  kFallAnchorAt, kFallEndAt) and glides between anchors through everything else. Sung with both pitches set to
  the note's, a 900 ms note still spent its first 225 ms gliding up from the last note: measured 120 to 190
  cents off over the first 100 ms of the long notes of a test line, with the tension map already in.
- Change: `Synth::Shape::sungAt` and `sungEndAt` (puhuja.h): with a second pitch set, the anchors' places as
  shares of the vowel, instead of the two constants (puhuja.cc, the vowel's pitch line in `emit`). Speech is
  unchanged (both -1). pajupilli sets them to 35 ms after the vowel's beginning and 35 ms before its end.
- Judged by: pajupilli's measurement of the same test line (src/jazzsinger.cc); not yet by ear.
- For puhuja: part of a sung-note call (pitch, length, the two anchors in ms), not fields of the speech shape.

## The form of an entry

    ### YYYY-MM-DD: a short title
    - Motivation: what the lead singer (or another use here) needed, and why speech did not.
    - Change: files, functions, knobs; the default behaviour for speech (unchanged, or how not).
    - Judged by: the measurement or the listening that settled it.
    - For puhuja: how it would go in natively (a knob, an API call, a mode), if not as it is.
