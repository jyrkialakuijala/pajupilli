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
- Brought up to date 2026-11-27 (the author: "bring in the recent speech improvements from puhuja"): the
  sources are now those of puhuja's commit `be11668b16c421eba727ac3d7c73668f80582e73`, byte for byte. That
  commit is b723192 with puhuja's work since (the vectorized string folds and line coefficients, ab1ddd3; an
  unstressed k's shorter puff, 7274f9f; the yielding wall behind a voiced closure let go without a pop, cbe0543
  and d7f52f7; a k or g released into s at the s's own place, c9efd86) and every change recorded below, merged
  in there three ways. Checked here before taking it: merging each file of this copy with upstream's (base
  b723192) gave upstream's file exactly, but for the indentation of three lines in folds.h; nine files changed
  (folds.h, larynx.cc/.h, line.cc/.h, puhuja.cc/.h, string.cc/.h). The singers on a song measure as before
  (soprano and baritone: pitch, cleanliness and partials the same to 0.2 dB). The entries below remain the
  record of why each singing change exists; none of them is a difference from upstream any more.

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
  Later the same day two more arguments, `knee2Hz` and `power2`: the author's table of the registers gives each
  voice a chest range, a crossover zone between two pivots (the primo and the secondo passaggio) and a head
  range, "a gradual overlap bounded by two pivot points", not one bend. The mass falls by one law through the
  zone and by another above it; the first pivot is rounded over a semitone either side. 0: one law, as before.
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

### 2026-11-26: the folds shielded from the tract's sound pressure (a singer's setting)
- Motivation: the author on pajupilli's soprano: "soprano is still screatchy", "something strange is happening
  vocal folds ... feels like they need some stabilizing". Held notes showed the folds breaking into a rough swing
  about a semitone flat, the noise louder than the harmonics, wherever the sung pitch meets the vowel's first
  resonance: "ah" at G#5 and A5 (soprano), "ee" at E4, "oo" at F4; a longer tract moves the break down with the
  resonance. Fold mass (0.14 to 0.28), the mass law's pivots, the closure (x 0.7, x 1.3), the breath (x 0.7) and
  the mouth opening did not cure it; nor did the sample rate (48, 96 and 192 kHz sing alike). Speech never holds
  a pitch on a first resonance, so it does not meet this.
- Change: `VocalFolds::Input::shield, shieldHz` (folds.h), read in the string folds' step (string.cc) in place
  of STRING_P1SUP and STRING_P1SUP_HZ when shield >= 0; `LarynxControl::shield, shieldHz` (larynx.h, larynx.cc);
  `Synth::setTractShield(share, hz)` (puhuja.h, puhuja.cc). Default -1: the built constants, speech bit for bit
  as before. pajupilli's singers use 0.5 at 3000 Hz (PJ_SINGER_SHIELD, PJ_SINGER_SHIELD_HZ).
- Judged by: held notes, the pitch found and the harmonics' energy over the rest between 100 Hz and 12 kHz.
  Soprano "ah" G5, G#5, A5, A#5: 10, 8, -8, 14 dB and -112, -109 cents on the middle two, to 32, 18, 18, 21 dB and
  all within 3 cents. Her "ee" at E4: -143 cents and -0.5 dB to +16 cents and 10 dB. A baritone's notes move by
  a few dB either way, none breaks. Shares 0.5 and 1 at 300 and 1000 Hz did not cure all of it; 1 at 3000 Hz
  does but costs 3 to 5 dB of level in the fifth below the break. Not yet judged by ear.
- Later the same day, after the author's listening ("sounds much better, but still a little screechy ... perhaps
  go down to 25 %"): pajupilli's singers use 0.75 at 8000 Hz. Soprano, 27 held notes C4 to D6, notes with the
  harmonics under 15 dB over the rest: unshielded 3 ("ah"), 8 ("ee"), 8 ("oo"); 0.5 at 3000 Hz 0, 4, 7; 0.75 at
  8000 Hz 0, 0, 3 ("oo" at E5 to F#5, which only an opened mouth cures: fold mass, closure, damping, collision
  stiffness and the glottal inertance did not). No change in puhuja for this, only the values pajupilli passes.
- For puhuja: the note at STRING_P1SUP says a quick corner killed the male b d g voice bars; that was with the
  share 1 and other variants. The bars should be checked with 0.5 at 3000 Hz before speech takes this.

### 2026-11-26: the pitch line's own smoothness (a singer's held note)
- Motivation: the author heard the soprano as "still a little screechy". Her long notes in a song were steady
  in the folds but not in pitch: 20 to 30 cents of wander through a held note with no vibrato asked, from the
  motion plan's smoothness term (PLAN_EPS 0.05) rounding the pitch line as it does the formants. Speech wants
  that; a sung note is to hold still.
- Change: `Synth::setPitchSmoothing(eps)` (puhuja.h) and `planPitchEps_`, used in `planSentence` (puhuja.cc) for
  the pitch channel only when >= 0. Default -1: PLAN_EPS for all four channels, speech bit for bit as before.
  pajupilli's singers pass 0.001 (PJ_SINGER_PITCH_EPS).
- Judged by: E5, B5, E5 on "dah", 0.55 s each, the pitch every 40 ms: the middle of each note within 30 cents
  before, within 7 after. (The plan's pitch speed limit and its tolerance changed nothing there.)
- With it `Synth::setPitchDrift(x)` (puhuja.h; `pitchDriftX_` in the per-cycle drift, puhuja.cc): the speaker's slow
  random pitch drift (kPitchDrift, 1.2 %) x this, 1 by default (speech unchanged); pajupilli's singers pass 0.3
  (PJ_SINGER_DRIFT). In a song the middle half of the soprano's long notes wandered 8.3 cents with 1, 6.8 with
  0.5, 6.2 with 0.
- For puhuja: the plan also makes a pitch move three to four times as long as `Shape::sungAt` and `sungEndAt`
  ask (35 ms asked, 120 ms heard, the pitch keyframes being sparse); pajupilli now asks for 8 ms. A sung
  vowel's pitch anchors should be keyframes of their own.

### 2026-11-26: the tongue's body forward or back (a second-resonance control for singing)
- Motivation: the author: "this tuned resonance I like very much, let's build a second resonance for the octave
  partial -- build a control for a plausible second resonance". On the line tract `Shape::f2` has no effect, and
  the expression line could only open the mouth (the first resonance) or go to the neutral vowel. A soprano's
  partials 2 to 4 lay 19 dB under her fundamental in a song.
- Change: `Synth::Expr::front` (puhuja.h), followed like `open`; at `lineShapePlanned:` (puhuja.cc), after the
  neutral and open mixes: the pharynx's area x exp(k front) and the mouth cavity's / exp(k front) (k = ln 1.8,
  LINE_FRONT_LOG), the tongue constriction's place + 0.08 front of the length (LINE_FRONT_X), front limited to
  -1 .. 1. 0, the default: nothing, speech unchanged.
- Why this gesture: it is what a tongue does. Its body is one mass between the pharynx and the mouth: forward, it
  leaves room behind and takes it in front; the two cavities' areas change oppositely and the constriction goes
  with it. In a two-cavity tract that is the classical lever on the second resonance (the front cavity's), with
  the first moved less. No area is invented: the two cavities' product is kept.
- Judged by: held notes. A low voice on G2, the resonances read off the partials: "ee" second resonance about
  1800 Hz at -1, 2070 at -0.5, 2700 and up at +1 (the first 460 down to 200); "oo" 840 to 1400; "ah" only 1140
  to 1250 (its constriction is weak). Soprano, overtones 2 to 4 against the fundamental with front +1 against
  0: "ah" F#5 to D6 +6 to +14 dB; "oo" G#4 to D6 +5 to +21; "ee" G#5 to C6 +2 to +5. With -1 her low "ee" and
  "oo" (C4 to F#4) gain 15 to 22 dB, mostly by the first resonance rising (the vowel darker). Not judged by ear.
- For puhuja: a real tongue model (the body's place and height as two numbers, the areas following) would give
  this and `open` from one mechanism; the resonances should be computed from the area function so that a
  singer can ask for "the second resonance at 2 f0" and not for a tongue position found by trial.

### 2026-11-26: the tract shield, for puhuja to explore in speech
The author asked for the shield to be described for puhuja and for its physical plausibility. This is that note;
no code beyond the entry "the folds shielded from the tract's sound pressure" above.

What the model does. A layer of the string folds is pushed sideways by the pressure on its medial surface times
that surface (STRING_KAPPA). Upstream of the narrowest point that pressure is the subglottal one; downstream of
it, and on the upper layer when the folds are shut, it is p1, the sound pressure in the tract's first section,
taken whole (string.cc, `p = j < jClosed ? in.ps : in.p1`). Nothing presses back. The shield takes
`share x lowpass(p1, hz)` off every layer's pressure. The flow equation is not touched: the air through the
glottis is still driven by `ps - p1` in full.

What it cures. Where the pitch meets the tract's first resonance, p1 at the pitch's own frequency is large and
turns from in step with the flow to against it within a semitone or two. Acting whole on the tissue it throws
the folds into an uneven swing about a semitone flat. Held soprano notes with the harmonics under 15 dB over
the rest, of 27 from C4 to D6 on "ah", "ee", "oo": unshielded 3, 8, 8; share 0.5 at 3 kHz 0, 4, 7; 0.75 at
8 kHz 0, 0, 3; 1 at 8 kHz 3, 3, 5 (too much is worse again). In a song: worst tenth of notes 66 cents off
unshielded, 20 with 0.5 or 0.75.

Why it may be physical, and where it is not.
- A fold is not a wall with air on one side. Above and beside its upper part lies the laryngeal ventricle, an
  air space open to the tract just above the folds. Being small it follows the tract's pressure there up to a
  few kHz. So the same p1 that pushes the fold's upper medial surface outward also presses on its upper and
  outer surface from the ventricle, inward and down. Sideways, the two largely cancel: a fold immersed in p1
  feels the pressure drop across the glottis and little of p1 itself. This is the argument of the existing
  STRING_P1SUP note, there applied only to the slow part (10 Hz, the mouth pressurised in b d g); the ventricle
  does not stop following at 10 Hz, so the argument holds for the sound as well.
- The cancelling cannot be whole. The surfaces differ in size and direction, the ventricle's opening has its own
  inertance, and the lower (body) layer is not reached from above at all. A share between a half and nine
  tenths is what the geometry allows; 0.75 is fitted by ear and by the counts above, not derived.
- Downstream of where the jet leaves the wall the pressure on the medial surface is near p1, but upstream of
  that point it is set by the flow (Bernoulli) and not by p1. The model gives p1 to the whole downstream part;
  that overstates p1's hold on the tissue, in the same direction as the shield corrects.
- The flow's coupling to the tract is kept on purpose. The tract's inertance skewing the flow pulse is what
  gives a voice its power and is well measured; only the tissue's direct push from p1 is reduced. Real voices
  do break where pitch crosses the first resonance (untrained singers, children); trained ones avoid it by
  moving the resonance. So some of the unshielded behaviour is real, and its size here was not.
- Not physical as built: the share is taken off all layers, also the upstream ones that never see p1 (for them
  it is a small wrong force, s x p1 against ps); and the corner frequency stands for the ventricle's own
  resonance without modelling it.

What to try in speech.
1. Shares 0.25, 0.5, 0.75 with the corner at 1, 3 and 8 kHz, against the male b d g voice bars (they died with
   share 1 and a fast fall), the voiceless fricatives' hiss (0.8 dB weaker with the slow share 1) and the
   aspiration after t and s.
2. The female voice on close vowels at accent peaks: her pitch (250 to 350 Hz) meets the first resonance of
   "ee" and "oo" there, the case this cures in singing. Look for rough or flat "ee"/"oo" under accents.
3. The more physical form: take the share off the downstream surfaces only, and give the ventricle its own
   small volume and neck (a Helmholtz side branch at the tract's first section) so that the corner and the
   share follow from two lengths and an area instead of two knobs.
4. Keep the slow share (STRING_P1SUP, 10 Hz) apart from the fast one: they answer different questions (a
   pressurised mouth, a resonance at the pitch) and may want different values.

### 2026-11-27: period doubling throughout (a singer's subharmonic)
- Motivation: the author asked for a faint subharmonic under the soprano ("perhaps a -20 db subharmonic on
  soprano voice would make it more realistic"; after hearing it: "remove the subharmonic or put it at -45 db").
  Speech has period doubling only in a creaky phrase end (CREAK_END_DIPLO).
- Change: `Synth::setDiplo(float x)` (puhuja.h; `diploX_`), which in puhuja.cc sets `lc.diplo` to at least x on
  every sample: alternate glottal cycles' breath pressure +-x and tension +- CREAK_DIPLO_Q x (larynx.cc,
  unchanged). Default 0: speech is unchanged.
- Judged by: the partial at half the pitch against the fundamental on the long notes of a song, soprano:
  -38 dB at 0.03, -32 at 0.06, -26 at 0.12, -21 at 0.24 (6 dB a doubling), a partial at 1.5 times the pitch as
  strong. The pitch falls by the square of x (7 cents at 0.12, 30 at 0.24), which the singer aims above. The
  singer uses 0.0135 (about -45 dB) for the soprano, none for the other voices.
- For puhuja: as it is (a call), or a knob beside CREAK_END_DIPLO; the pitch fall wants a look if it is used
  strongly (the tension share, CREAK_DIPLO_Q, is the likely cause; not tested).

### 2026-11-27: the jaw dropped with the mouth's corners in (a singer's tall mouth)
- Motivation: the author, on the soprano: "the soprano could open mouth more in the high notes, and keep mouth
  sides narrow, so that the backwave goes from the teeth back to the throat, and the sound would be a bit more
  'open' and less stressed". `Expr::open` is the other gesture: towards "ah" with the lips flared and their
  tube shortened.
- Change: `Expr::tall` (puhuja.h, 0 .. 1, the last field, default 0; `exprTall_`). In puhuja.cc the vowel's
  shape keeps its place and the mouth's cavity grows x LINE_TALL_ORAL (1.6), the lips' opening
  x LINE_TALL_LIP (1.25), the lip tube x LINE_TALL_LEN (1.15); and `open`'s lip flare is taken back with it (the
  opening's growth to 0.4 of it, the tube not shortened). Speech sets no expression: unchanged.
- Judged by: a song, soprano, 168 vowels, tall from A4 to A5: the pitch miss the same (median 3.4 -> 3.5
  cents), the harmonic share the same (20.5 -> 20.9 dB), the fundamental stronger against the overtones
  (overtones 2 to 4: -8.2 -> -9.6 dB; partial 3 -4.1 -> -6.7 dB), 2.5 to 6 kHz 0.5 dB less. Not yet heard by the
  author.
- For puhuja: no use in speech as it is.

## The form of an entry

    ### YYYY-MM-DD: a short title
    - Motivation: what the lead singer (or another use here) needed, and why speech did not.
    - Change: files, functions, knobs; the default behaviour for speech (unchanged, or how not).
    - Judged by: the measurement or the listening that settled it.
    - For puhuja: how it would go in natively (a knob, an API call, a mode), if not as it is.
