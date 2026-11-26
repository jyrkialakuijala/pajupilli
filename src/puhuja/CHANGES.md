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

### 2026-11-26: an expression line: breath pressure and vibrato over time
- Motivation: the author: "the director should be in charge of drama, vibrato and hairpins of the singer, too".
  pajupilli's director gives each note a level at its start and its end, a swell with its peak, and a vibrato
  (depth, rate, when it begins). Speech has one level a sound and no vibrato: a crescendo through a held vowel
  or a vibrato coming in late had no way in.
- Change: `Synth::setExpression(std::vector<Expr>)` (puhuja.h): points over the output's time (s), each a lung
  pressure factor and a vibrato (cents either way, Hz), straight lines between them. In the sample loop
  (puhuja.cc, beside the tension map): the pitch target x the vibrato before the pitch control and the tension
  map read it, the lung pressure x the factor after the line's level. Empty by default: speech is unchanged.
  The caller has to know when each sound falls in the output; pajupilli takes that from a first rendering's
  labels.
  Later the same day a fourth value a point, `cents`: the pitch target itself moved (default 0). The singer's
  practice needed it (the author: "practice the voice similarly to wind instruments, both for slurred and direct
  intervals"): one of the things tried on arriving at a note is aiming a little past it for some tens of ms, as a
  singer's folds are set for a leap; a vowel's two pitch anchors can only draw a straight line through the note.
- Judged by: pajupilli's measurement of a sung line (level and pitch tracks against the line asked); not by ear.
- For puhuja: part of a sung-note call, the contour given over the note's own length, so no first rendering is
  needed to place it.

### 2026-11-26: the mouth opened for high notes, and the folds brought together
- Motivation: a soprano sings above her vowels' first resonance, where the air column stops helping the folds
  (the author's notes on inertive reactance: she drops the jaw and flares the lips so that the first resonance
  stays at or just above her pitch, and all vowels move towards "ah" up there). pajupilli first tried this with
  `Shape::f1`: on the line tract the formant targets of a shape do nothing (the sound was the same bit for bit,
  as it was for `Shape::press`), the tube's shape comes from the phoneme. And the soprano's overtones are weak
  (on G5 the 2nd to 5th partials 27 to 45 dB under the fundamental): a way to bring the folds together was
  wanted to try a firmer closure.
- Change: `Synth::Expr::open` (puhuja.h; 0 by default): in the sample loop, where the line's shape for the
  moment is final (`lineShapePlanned`), the shape mixed 0.7 x open towards the table's "ah", the lip area
  x (1 + 1.5 open), the lip tube x (1 - 0.5 open), the oral area x (1 + 0.5 open).
  `Synth::setAdduction(rest)` / `LarynxControl::restX` (larynx.cc, after the voice-quality rest factors): the
  folds' rest gap and chink while voicing x rest. Speech is unchanged (0 and 1).
  And `Synth::Expr::neutral` (0 by default): the shape mixed towards the table's neutral vowel, at the same
  place as `open` (the author's notes on scat: at tempo the articulators cannot reach the vowels, which
  centralize to a schwa-like carrier between the consonants; and an unstressed syllable's vowel is reduced).
  Later the same day `Synth::Expr::adduct` (1 by default): the same factor over time, for stress in scat
  singing (the author's notes: a stressed syllable has the folds pressed together, a longer closed phase and a
  flatter spectral tilt, without more breath; a ghost note has them let apart, breathy).
- Judged by: see pajupilli's measurements of held soprano notes (src/jazzsinger.h); the closure moved the
  overtones by 1 to 3 dB at most and upset the pitch of single notes, and is not used.
- For puhuja: a jaw and lip gesture belongs in the articulation (a sung vowel's shape by its pitch), not laid
  over the finished shape; the folds' closure in the larynx control beside the glottal area.

### 2026-11-26: the vocal folds' mass by voice, and following the pitch
- Motivation: the string folds have one mass (0.15 g on 1.4 cm) and their stiffness grows as the square of the
  pitch asked. Sung, they held their pitch only to about D4; at a flute's pitches the voice gave almost no
  sound (the author: "it is producing no or almost no output in the flute range"). The author: "idealize the
  singer by changing the mass on the vocal cords to get higher -- if mass is changed, then it needs to be a
  controlled (PD-system) change that changes relatively quickly (like in ~3 ms to reach 98 % of the goal mass)
  since then the tension needs to be suitable fitted as well -- or otherwise, choose singers vocal coord mass
  initially based on their wished range". Both are in: a voice's own mass (a soprano's folds are shorter and
  thinner than a baritone's), and less of the fold swinging above the voice's passaggio.
- Change: `VocalFolds::Input::mass` (folds.h; 1 by default), used by the string folds alone (string.cc: every
  lane's mass and every stiffness derived from it x this, so the pitch is still the tension's);
  `LarynxControl::foldMass` (larynx.h), followed in `FoldLarynx::step` by a critically damped follower, 98 % of
  a change in 3 ms (larynx.cc); `Synth::setFoldMass(scale, kneeHz, power)` (puhuja.h): the mass x scale, and
  above kneeHz x (kneeHz / pitch)^power, set each sample from the pitch target (puhuja.cc). Speech is unchanged
  (1, 0: the mass table itself, bit for bit).
- Judged by: a sung scale C3 to E6 on "dah" (pajupilli, 2026-11-26). Mass 1: on pitch to D4, then 70 to 200
  cents off. Mass 0.5: to G#4. Mass 0.25: to C5. Mass 1 falling as (150 Hz / pitch)^1.5: within 7 cents from C3
  to E5 and from A#5 to D6, with a hole at F#5 and G#5 (240 and 95 cents flat; the pitch crossing the vowel's
  first formant, probably) and at E6. Lighter folds are far louder on the same breath (mass 0.5: +10 dB; the
  falling mass: +20 dB and more at the top); pajupilli levels that after the fact. Not judged by ear.
- For puhuja: the fold's length is a constant (1.4 cm), so a shorter fold is only stood in for by its mass; a
  voice preset would set length, mass and the chink together. The level should follow from the physics (a
  shorter fold passes less air), not be levelled afterwards.

### 2026-11-26: a voice between or beyond the male and the female preset
- Motivation: pajupilli's singers are nine voices, basso profondo to coloratura soprano. The author: "if it is a
  woman, the whole head is smaller, including nasal cavities, brains are 1.2 liters vs. man brain 1.4 liters, so
  probably in the same ratio other things, too", and: "the amount of female and male hormones can be a bit of a
  continuum so perhaps soprano could be slightly more feminine than the usual female head -- like the parameters
  could be 1.1 * female - 0.1 * male, for mezzo the defaults, and 0.9 * female + 0.1 * male for alto, and
  similarly for male voices". puhuja has the two presets and nothing between.
- Change: `Synth::setVoiceBlend(woman)` (puhuja.h, puhuja.cc): `setVoice` of the nearer preset (the pulse, the
  per-voice knobs and filters are one or the other), then every value of the `Voice` struct mixed
  male + woman x (female - male), and what `setVoice` derives from them set again. 0 and 1 return right after
  `setVoice`: the presets themselves, speech unchanged.
- Judged by: not judged; pajupilli uses -0.15 (basso profondo), -0.1 (bass), 0 (baritone), 0.1 (tenor,
  countertenor), 0.9 (contralto), 1 (mezzo), 1.1 (soprano), 1.15 (coloratura).
- For puhuja: the per-voice parameters chosen by `-female` at start-up ("@f" values) are not mixed and cannot be
  chosen in-process at all; a voice as a value from 0 to 1 would need them read per Synth.

## The form of an entry

    ### YYYY-MM-DD: a short title
    - Motivation: what the lead singer (or another use here) needed, and why speech did not.
    - Change: files, functions, knobs; the default behaviour for speech (unchanged, or how not).
    - Judged by: the measurement or the listening that settled it.
    - For puhuja: how it would go in natively (a knob, an API call, a mode), if not as it is.
