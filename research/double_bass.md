# A plucked double bass (jazz pizzicato): research and model plan

Status: research only (2026-11-13). Nothing here is implemented yet.

The instrument: an acoustic double bass plucked by hand, as in jazz. Four strings, a resonant
body with a wood resonance and an air resonance at a non-integer ratio taken from the literature. It is to be a
physical model like the rest of the project: strings, a bridge, a body and a finger, no samples.

Where each number comes from is marked: **[web]** found in the sources listed in section 8,
**[user]** from the user's notes, **[memory]** from the general literature as remembered, not
looked up for this document, to be checked against measurements before it is relied on.

## 1. What the instrument is

- **Strings:** E1 41.20 Hz, A1 55.00 Hz, D2 73.42 Hz, G2 98.00 Hz, tuned in fourths **[web]**.
  Scale length 105–110 cm **[web]**; 106 cm is a common 3/4 bass.
- **Range for the model:** E1 to about G4 (the thumb position of a jazz solo), MIDI 28–67. GM
  program 33, Acoustic Bass.
- **String data [memory]:** steel-core orchestral strings carry about 270–300 N each, gut and
  jazz hybrids less (about 200–260 N). The mass per length follows from T = μ (2 L f)²: at 280 N
  and 1.06 m, E1 has μ ≈ 37 g/m, A1 21 g/m, D2 11.6 g/m, G2 6.5 g/m. Characteristic impedance
  Z = √(T μ) ≈ 3.2, 2.4, 1.8, 1.35 kg/s.
- **String sets and their sound:** gut decays fastest and thumps (old-school jazz); steel sustains
  longest ("mwah", growl); hybrids and tungsten-wound sit between **[web, players' reports, no
  measured decay times found]**.

## 2. The body's resonances

From the user's notes **[user]** and the literature:

| Resonance | Range | What moves |
|---|---|---|
| A0, the air (Helmholtz) | 41–60 Hz **[user]**; about 60 Hz in Guettler & Askenfelt **[web]** | The air breathing through the f-holes, against the plates |
| T1, the main wood resonance | 82–117 Hz **[user]** | The belly, strongest on the bass-bar side; grouped with body modes C3, C4 |
| Plate modes | 150–400 Hz **[user]** | A dense series of bending modes: the peaks of the admittance curve |
| Bridge and upper modes | above 400 Hz; a massive bridge resonance near 300 Hz **[user]** | The bridge, tailpiece, plate edges: the punch |

**The ratio of wood to air, from the literature and a recording.** The first idea was 1.62–1.68;
other ratios are fine, ideally not an integer. What is measured:

| Source | A0 (air) | T1 (wood) | Ratio |
|---|---|---|---|
| Brown 2004, two full-size basses (flat and round back), admittance, radiation and laser vibrometer | 65–67 Hz | 115 Hz | 1.72–1.77 |
| Wall 1985, a small test bass (response curves; Brown finds its low values surprising) | 51 Hz | 100 Hz | 1.96 |
| Askenfelt 1982, five basses (as cited: T1 on average 82–114 Hz); Guettler et al.: A0 about 60 Hz | ~60 Hz | 82–114 Hz | 1.4–1.9 |
| For comparison (Brown's table 6.1): violin | 275 Hz | 460 Hz | 1.67 |
| For comparison: cello | 102 Hz | 203 Hz | 1.99 |
| Recording: Till Brönner, *Ku'damm 1:30 A.M.* (section 5.1) | about 69 Hz (weak evidence) | about 104 Hz (clear) | about 1.5 |

Brown also gives the bass's next modes: C4 at 130 Hz and C3 at 155 Hz (round back), C2 at 145 Hz
(flat back), A2 (the second air mode) at 150 Hz, the top's strong band from 125 to 160 Hz, and
brace resonances of a flat back at 240, 335 and 405 Hz.

Proposed: **A0 = 66 Hz, T1 = 111.86 Hz, a ratio of 1.69478**. The user's preference: more than a
fifth, like good violins (the violin figures above: 1.67), and as far from any simple ratio as
possible: about 1.68 to 1.7, not 1.75.
- 5/3 (1.667) and 12/7 (1.714) are Farey neighbours (12·3 − 5·7 = 1): no simpler fraction lies
  between them. Their mediant is 17/10 = 1.7, itself a fraction.
- The numbers hardest to approximate by fractions between them are the noble numbers, built with
  the golden ratio φ: (5φ + 12)/(3φ + 7) = 1.69478 and (12φ + 5)/(7φ + 3) = 1.70431.
- The model takes the first, 1.69478: the two resonances then share no low harmonics. The second
  (T1 = 112.48 Hz) is an alternative.
- Both values are inside what is measured on basses: A0 is Brown's (65–67 Hz), and 111.9 Hz lies
  between Askenfelt's range for T1 (82–114 Hz) and Brown's 115 Hz.
- The ratio is a parameter of the model: Brown's basses (1.74) and the recorded one (about 1.5)
  can be set with it and compared by ear. A0 sits at C2, T1 just above A2.

**Why the ratio matters to the sound:**
- Notes near A0 and T1 are loud and die fast (the body takes the string's energy quickly); notes
  between and below them are weaker and ring longer. That unevenness from note to note is much of
  an acoustic bass's character.
- Under A0 the air and the wood radiate in opposite phase and cancel: E1's fundamental is weak,
  and the ear rebuilds the pitch from its harmonics.
- A note that sits on a strong resonance can beat or stutter (the wolf); in pizzicato it shows as a
  short, two-stage decay.

**The two are one coupled system.** A0 and T1 are not independent: the belly's motion pumps the air,
and the air loads the belly. The standard model is two coupled oscillators (the guitar's
"two-mass model": Christensen & Vistisen 1980; Caldersmith 1978) **[memory]**: a belly piston on
its stiffness, and the air plug in the f-holes on the cavity's air spring. The coupling pushes the
two apart: the measured A0 lies under the rigid-body Helmholtz frequency, T1 above the belly's
own. A three-oscillator version adds the back plate. The model here should keep this structure
and fit the uncoupled values so the coupled pair lands on the chosen A0 and T1.

## 3. The pluck

Jazz pizzicato **[memory]**:
- The right hand's index or middle finger (or both together) pulls the string sideways, across the
  end of the fingerboard, with the side of the fingertip: a wide, soft contact (1–2 cm of flesh),
  not a point. The finger leaves by slipping off: the string's release is not instantaneous.
- Plucking point: about 12–20 cm from the fingerboard's end towards the bridge is rare; mostly
  over the last part of the fingerboard, roughly 1/6 to 1/4 of the string from the bridge. Nearer
  the bridge is brighter and tighter; over the board is rounder.
- The pull is mostly parallel to the belly; the string then also vibrates perpendicular to it, and
  the two polarisations decay at different rates (the bridge moves the belly far more for one
  than the other): a two-stage decay, a quick "thump" then a longer tail.
- A hard pluck makes the string slap the fingerboard (the string's amplitude exceeds its height
  over the board): the percussive click of slap playing; a little of it is in every strong note.
- The left hand: a stopped note is damped by the fingertip's flesh (shorter, darker than an open
  string); the finger can slide (glissando), rock (vibrato, slow and wide in jazz), and lift to
  stop the note (the "dead" ghost notes). Hammer-ons and pull-offs sound without a new pluck.
- The other three strings ring in sympathy unless damped: part of the instrument's bloom.

## 4. Proposed model

1. **Four strings**, each a waveguide (a delay line pair) with:
   - two polarisations (in the plane of the pull and across it), coupled at the bridge;
   - stiffness (dispersion: a low wound string is slightly inharmonic) and frequency-dependent
     loss (air, internal friction; more for gut than steel), fitted to decay times per harmonic;
   - the stopping finger as a lossy termination that can move (slides, vibrato);
   - a collision with the fingerboard (a one-sided spring along the board) for slap and buzz.
2. **The finger's pluck:** a mass on a damped spring (the fingertip's flesh) with a finite width,
   pulling the string and slipping off at a force limit (friction). Velocity sets the pull's
   depth and speed; the plucking point and direction are player settings.
3. **The bridge:** where the four strings meet the body: a 2-D admittance (how far the bridge
   moves per force, along and across the belly). The strings exchange energy through it
   (sympathetic ringing) and drive the body with it.
4. **The body:** modal.
   - A0 and T1 as the coupled two-oscillator system of section 2, at 66 and 111.86 Hz (1.69478, a parameter).
   - The plate modes from 150 to 400 Hz and above as a bank of some dozens of resonators, with
     frequencies, dampings and strengths set from published admittance curves **[to be found:
     Askenfelt 1982, Brown 2004]**, and a broad bridge resonance near 300 Hz.
   - Radiation: the f-holes' air flow and the belly's motion summed with their phases (the
     cancellation under A0), plus directivity kept simple.
5. **The player:** a `Performer` as for the winds: note on = stop the string (choose the string
   and position as a bassist would: the lowest position that reaches, open strings when asked),
   pluck after the finger lands; legato overlap = hammer-on or slide; velocity = pluck strength;
   release = damp with the left hand. The ensemble's small "booth" room suits it.

## 5. What to measure and from where

- **Body modes:** published admittance and radiation curves of double basses (Askenfelt 1982,
  "Eigenmodes and tone quality of the double bass"; Brown 2004, the dissertation on the back
  plate; Bissinger's work on the violin octet) **[cited in the sources found, not yet read]**.
- **Decay per note and harmonic, pizzicato:** from the jazz recordings in `~/Music` (the bass
  stem of the same separation used for the drums: Demucs gives a bass stem), as measurements only.
  They also show the note-to-note unevenness around A0 and T1, and so which values the recorded
  basses have.
- **String damping:** the Bundeswehr report "Basses II: Diagnosis" has decay time constants for
  bass strings **[web: found, not read]**; Woodhouse's and Valette's string-loss models
  **[memory]**.

### 5.1 First measurement from a recording (2026-11-13)

Till Brönner, *Ku'damm 1:30 A.M.* (a walking pizzicato bass through the track). The bass stem
was separated (Demucs), each note's pitch found, and each partial's decay rate fitted (1185
notes, 4263 partial decays; `scratchpad/bass/decay.py`, measurements only). A partial near a body
resonance loses its energy to the body and dies faster:

| Note | Hz | Notes | Fundamental's decay | Level |
|---|---|---|---|---|
| B1 | 61.7 | 80 | 10 dB/s | −22 dB |
| C2 | 65.4 | 85 | 13 dB/s | −19 dB |
| **C♯2** | **69.3** | 36 | **33 dB/s** | −19 dB |
| D2 (open string) | 73.4 | 83 | 6 dB/s | −20 dB |
| F♯2 | 92.5 | 35 | 32 dB/s | −13 dB |
| G2 | 98.0 | 72 | 28 dB/s | −12 dB |
| **G♯2** | **103.8** | 23 | **102 dB/s** | −11 dB |
| A2 | 110.0 | 42 | 22 dB/s | −15 dB |
| F3, F♯3 | 175–185 | 37 | 76–97 dB/s | −17 dB |

- **The wood resonance is near 104 Hz:** G♯2's fundamental dies four times faster than its
  neighbours', while its own 2nd and 3rd partials do not (55 dB/s, like the neighbours'). The
  level also peaks there.
- **The air resonance is probably near 69 Hz:** C♯2 dies 2.5–6 times faster than C2 and D2, but its
  2nd partial is not slower than its fundamental, so a short-played note cannot be ruled out.
- **Another strong resonance near 180 Hz.**
- The resolution is a semitone (the notes played): 67–71 Hz and 101–107 Hz, a ratio of 1.4–1.6.
- Four more tracks are being separated to see how far basses differ.

### 5.2 A second passage (2026-11-13)

*Some Cats Know* (Love Ballads), 1:01–1:10: upright bass with brushes, a slow ballad. The clip
was separated into bass, drums, vocals and the rest. The bass plays about one note per 0.7 s:
D2, G1, C3, A1, D2, C3, F3, A1, C2, D2, D3, C3, A1, C2, D2. Fifteen notes are too few for decay
statistics, but the passage is a clean reference for the pluck's attack and a note's shape
(0.3–0.65 s notes, the open D and A ringing longest). The whole track is to be measured next.

## 6. Checks

- A0 and T1 of the model's bridge admittance at their set values (66 and 111.86 Hz), within 0.5 Hz.
- Each open string in tune within 2 c; stopped notes within 5 c over E1–G4.
- Decay times per note against the measured ones; the two-stage decay present.
- Sympathetic ringing: plucking A1 excites the open E1's third harmonic, and the others likewise.
- Real time, NaN recovery, fuzz, as for every instrument.

## 7. Open questions

- The exact A0 and T1 of real basses: the user's ranges are wide (41–60 and 82–117 Hz). The first
  measurement task is to read them from recordings and the literature.
- **Decided:** gut strings first (the 1950s thump: quick decay, a strong fundamental), with a
  hybrid set (a gut-like core, metal-wound: more sustain) as a second option. They differ in the
  loss fit, the mass and the stiffness; the model is the same.
- How many body modes are needed before it stops sounding like a filter? To be judged by ear.

## 7a. Ten strategies to make the simulation sound better and more authentic (2026-11-13)

The first model plays (`-inst double-bass`): one string in two directions, a rigid bridge without
mass, top and back as 16 x 8 grids, a sound post spring, one air pressure. What it lacks, in the
order of the expected gain:

1. **Four strings on one body, all the time.** One bass, not a bass per key: the four strings end
   on the same bridge, and the three not played ring along (the open strings answer notes that
   share their overtones), unless the hands damp them. The bassist's choice of string and position
   then matters: the same pitch on a thicker string, higher up, is darker and shorter.
2. **The strings behind the bridge, and the tailpiece.** Each string goes on from the bridge to
   the tailpiece (about 1/6 of the sounding length: it sounds two octaves and a fifth above the
   open string). These short lengths ring, and the tailpiece hangs on them and on its cord from the
   end pin: a heavy pendulum with its own resonances (rocking, swinging), usually near or under
   the lowest notes. Together they take energy at particular notes (the wolf's neighbours) and
   add a faint metallic ring after each pluck.
3. **The bridge as a part.** A bass bridge is tall (about 16 cm), with a mass of about 100 g and
   legs with a waist. It rocks on its feet, and its top sways on the waist: a resonance of its own
   that passes the strings' overtones to the body below it and holds them back above it (it shapes
   everything over a few hundred Hz). The adjusters under the feet and the strings' angle over it
   (their downward force, some 300–400 N in all) belong to it. Eight motions, as planned.
4. **The sound post as a part.** Now a spring. A real post is a spruce rod with mass, wedged
   between the plates: it carries force along its length (wave travel time about 0.04 ms), can rock,
   and stands a little behind the treble foot, not under it. Its place and fit are what a luthier
   adjusts to voice the bass. It makes the treble foot a pivot, so the bridge's rocking pumps
   the bass-bar side: the asymmetry behind the strongest low resonances.
5. **The bass bar and the top's real build.** The bar is now extra mass and stiffness on one row.
   Better: its height tapering to the ends and its spring (it is glued in under tension), the
   top's arching and thickness changing from the middle to the edges (graduation), the f-holes
   as cuts that free the middle "island" between them, where the bridge stands. A finer grid there
   (32 x 16, or a patch around the bridge) lets the island move as it does.
6. **The air inside as a space, not one pressure.** The body is over a metre long: besides the
   Helmholtz resonance there are standing waves along it (the first near 150 Hz: Brown's "A2")
   and across it. They couple to the plates and give the response its peaks between 150 and
   400 Hz. A coarse grid of air cells between the plates, with the f-holes at their real places.
   The f-holes' own losses (the air's friction at their edges) set how sharp the air resonance is.
7. **Ribs, neck, fingerboard, and the body as a whole.** The edges are now held still. Really the
   ribs give a little, the whole body bends and twists (the lowest "body modes", under 100 Hz, in
   which the neck and scroll swing against the body), and the long fingerboard, glued on only at
   the neck, has a resonance of its own. The end pin stands on the floor and passes vibration
   into it (a stage floor radiates it: Guettler, Buen & Askenfelt). These move the low
   resonances and add small ones a player knows as dead and live notes.
8. **The string itself, more truly.** Gut strings: damping that depends on the overtone as
   measured (fitted to recordings), each string's own stiffness, and its twisting. A hard pluck
   stretches the string: its pitch starts a little sharp and falls, and the two directions of
   motion couple (the string's path turns: a whirling that gives the tone its slow beating). The
   string against the fingerboard: at a strong pluck it touches the board along its length (the
   growl and, harder, the slap); a stopped string ends on a soft fingertip and a wooden board,
   not a fixed point.
9. **The hands.** Right: the finger's pad as a soft body with friction, so the release depends on
   the pull's speed and direction; where it lands on a string still sounding (it stops it first:
   a tiny silence before each note); the thumb resting on the fingerboard's edge. Left: fingers
   landing and lifting (hammer-ons, pull-offs, ghost notes), sliding into notes, a slow wide
   vibrato, and damping the neighbours. The timing between the left finger landing and the pluck.
10. **What is heard, and proof by measurement.** The microphones: real positions (in front of the
    f-hole, by the bridge, a pickup under the bridge's foot), the body's radiation by direction
    from all the masses' motion, the room. And each step checked against measurement, not only by
    ear: the model's response at the bridge against published curves (Askenfelt, Brown), its
    notes' decay and overtones against the recordings in `~/Music` (as begun in section 5.1), so
    that parameters are fitted, not guessed.

A first order of work: 1, 2 and 3 (they change what every note sounds like and how notes ring
together), then 4 to 6 (the body's response), with 10 running alongside as the check.

## 7b. What was built from the strategies, and what each did (2026-11-13)

`-inst double-bass`: `src/bass.*` (strings, fingers, bridge), `src/bassbody.*` with
`src/bassbody_data.h` from `tools/bassbody.py -emit` (the body), `src/bassinst.*` (the player).
`PJ_BASS_SYMP`, `_AFTER`, `_TAIL`, `_BRIDGE`, `_TMOD`, `_BOARD` = 0 switch a part off to compare.

| # | Part | Measured effect |
|---|---|---|
| 1 | One bass, four strings on one bridge: the others ring along | After A2 on the G string is stopped, the ring left is −41 dB re the note (−74 dB without); it is the open A string's octave |
| 2 | The bridge as a 70 g mass on its legs: stiff up and down, swaying on its waist (420 Hz); each string at its notch | Open D: +3 dB at 250–500 Hz, −10 dB at 1–2 kHz, −16 dB at 2–5 kHz against a rigid bridge |
| 3 | The sound post: with mass, 2.4 cm behind the treble foot | At the wood resonance the bass foot moves 1.4 times the treble foot (they moved alike) |
| 4 | The bass bar tapered (40 to 12 mm), the plates graduated (thicker in the middle), the f-holes softening the top beside the bridge | The 111.86 Hz resonance is the top's (72 % of its energy; it was 39 %, mostly the back) |
| 5 | The air as 16 cells along the body | Air standing waves appear: 139.6 Hz (Brown measured 150 Hz), 175 Hz (the recording's bass: 175 Hz). The air resonance is 8 dB stronger in the far sound |
| 6 | The string stretches at a hard pluck (its tension rises) | Small: a 12 mm pull is +3 c still at 0.3 s; more in the attack (capped at +50 c) |
| 7 | The fingerboard under the string | A hard pluck towards the board: +29 dB at 1–5 kHz in the first 0.25 s (the growl and slap) |
| 8 | Two calloused fingers taking turns; both for an accent | Against a soft fingertip: the 8th overtone +19 dB; consecutive notes differ (the middle finger 12 mm further, 10 % deeper) |
| 9 | The left hand: slides, hammer-ons and pull-offs without a pluck (a soft key under a held one), vibrato on stopped strings, stopping the string at the key's release, then leaving it open | E2 slides to G2 in 80 ms and hammers to A2 in 12 ms: 82.2, 98.0, 109.6 Hz. Key to sound: 15 ms (was 40) |
| 10 | The microphones as on a jazz record (a bridge pickup, an f-hole microphone, a little from far), and the string losses, checked against the recording | See the table below |

Overtones 1 to 8 of notes between 50 and 100 Hz, level at the note's start (dB re the note's
strongest) / decay (dB/s):

| | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|
| Recording (*Ku'damm 1:30 A.M.*) | −2 / 17 | −3 / 36 | −16 / 46 | −23 / 50 | −29 / 69 | −28 / 80 | −28 / 75 | −31 / 79 |
| First version, heard far away | −19 / 11 | 0 / 33 | −9 / 33 | −2 / 77 | −14 / 70 | −2 / 93 | −19 / 110 | −9 / 68 |
| Now | −5 / 26 | 0 / 41 | −10 / 47 | −22 / 59 | −25 / 76 | −17 / 76 | −25 / 70 | −25 / 102 |

Built with almost no effect (kept, switchable):
- **The strings behind the bridge** (6 modes each): under 0.1 dB. Sideways to the bridge they are
  only a soft spring (T / length = 1.3 kN/m against the bridge's 4 MN/m).
- **The tailpiece** (250 g, 48 Hz): no measurable change at G1. Its real effect comes through the
  strings' pull along their length and the bridge's rocking fore and aft, which the model does
  not have.

Not done: ribs, neck, fingerboard and end pin as parts (strategy 7); a finer grid around the
bridge; the plucking finger as a soft body with friction. Unchecked guesses remain: the gut
strings' tension, stiffness and losses, the bridge's and tailpiece's resonances, the f-holes
(the fit asks 234 cm², large), the plates' arching factors (top x8, back x20).

Cost: 1.2–2.0 µs per sample (6–10 % of a core). Tests: `tools/test.py strings()`.

## 7c. Later additions (2026-11-13)

- **Longer, livelier resonance.** String losses lowered (the fundamental now dies at 17 dB/s, as
  on the recording; open strings, ending on the nut, 30 % less again), the body's resonances ring
  longer. Each overtone has two slightly different pitches (the two directions of the string's
  swing) and its plane turns slowly, and a gut string's overtones are each a little off and
  damped differently: overtones 2–6 swell and fade by 2–4 dB around their decay
  (`PJ_BASS_ALIVE`). Open strings have a third of it.
- **The sound post is rigid:** its ends are held together exactly each sample (a constraint force),
  not a spring.
- **The strings behind the bridge** are exactly 1/6 of the string (they sound its 6th overtone).
- **Two sides.** Left and right hear the body's sound plus and minus the rate of change of the
  top's motion's moment across (and partly along) it, over the speed of sound: the first term of
  the top's directivity, low-passed at 250 and 700 Hz where that term stops holding
  (`PJ_BASS_WIDTH`, default 0.7). Under 160 Hz the two sides are alike (correlation 0.99);
  above, 0.55–0.68, and each overtone has its own balance (a ringing F1 + C2: −4.6 to +5.2 dB).
- **The right hand's ways:** the jazz pull (index and middle as one, a 22 mm pad) by default;
  fingertips in turn and the hand by the bridge when notes come under ~210 ms apart (200–500 Hz
  +10 dB, 500–1500 Hz +17 dB against slow notes); the thumb (hardness knob down: 400–1200 Hz
  −33 dB); the pull turned outward by the air knob, so the string swings back onto the
  fingerboard (growl, slap); the brightness knob moves the place from halfway up the fingerboard
  to the bridge. The skin's rasp as it slides off.
- **Harmonics** on the keys above G4, to C6: a light touch at 1/n of the string after the pluck
  damps every overtone that moves there. Natural ones of the open strings where there is one,
  otherwise the note an octave (twelfth, two octaves) lower touched at its half (third,
  quarter). In tune within 2–8 c; clear up to A5, weak above (the bridge passes little there).
- **One bass:** up to four notes, one per string; a fifth takes over a string.
- **Cost:** up to 3.7 µs per sample with all four strings ringing (18 % of a core).
- **Open:** almost nothing above 1.2 kHz reaches the output (−73 dB re the low band): the bridge
  model and the microphones pass too little there.

## 7d. Expressive playing (2026-11-13)

- **The right hand by velocity:** up to 45 the thumb; to 72 the flat finger (the index's side along
  the string); to 99 the stacked pull; from 100 the pull turned outward (snap, then slap).
  Fast notes: fingertips in turn, by the bridge. K2: the plucking place, from the middle of the
  string to behind the bridge.
- **The stopping finger on the joystick:** sideways slides it up to two semitones either way;
  forward rocks it (vibrato on top of the knob's). Stopped notes are slid into from 5–20 c below.
- **Ghost notes and the rake:** the mute pad (pad 2) held makes plucks dead (11 dB quieter, gone by
  a further 15 dB within 0.2 s); struck, it rakes three muted strings 28 ms apart.
- **Note endings:** a short note (under 0.15 s) is stopped dry, the strings that rang along too
  (−23 dB at 150 ms); a note held over 0.6 s is let go slowly and its neighbours ring on.
- **Which string:** by the line: staying on the last note's string and near the hand's place
  cost less than crossing or shifting; an open string is taken when it is clearly nearer.
- **The strings' height (K3):** turned up, the action goes down to 0.35 of its height and the
  attack growls more (1–4 kHz in the first 0.3 s from −36 to −15 dB re the lows). The growl does
  not yet last through the note.
- **The pluck's definition:** the string leaves the pad's edge at once at the end of its slide,
  the bridge's top is 60 g (its sway 440 Hz, well damped, up-down 1500 Hz), and a microphone
  close to the top by the bridge hears the top's acceleration. 1.2–3.5 kHz is now about −44 dB
  re the lows (it was −73 dB).
- **Along the strings:** the bridge's top tips towards the neck or the tail, each string is a
  spring along its length, the tailpiece a mass between the short lengths and its cord (about
  207 Hz). Measured effect on the notes' levels and decays around it: under 1 dB. A stronger
  path than the strings' angle over the bridge would be needed for it to matter.
- **Strings from older records.** Fundamental's decay and the 2nd and 3rd overtones' levels at
  the start: Ella Fitzgerald, *Body and Soul*: 20 dB/s, −5, −18 dB; Louis Armstrong, *On the
  Sunny Side of the Street*: 34 dB/s, −19, −32 dB (darker and shorter than *Ku'damm*: 17 dB/s,
  −3, −16 dB: probably plain gut). The model's default rings longer (14 dB/s, as asked);
  `PJ_BASS_OLDGUT=1` gives the old records' decay.
- **Loudness:** all instruments are trimmed to a common loudness (`tools/levels.py`,
  `src/levels_data.h`): the saxes were up to 9.5 dB over the median, the bass 3 dB under.

## 7e. Under the bow (`-inst contrabass`)

The same strings, bridge and body, with a bow instead of the fingers (`Bass::bow`, `BowPlayer`).

**The contact.** The hair grips the string by friction: up to 0.9 × the bow's force while they move
together (sticking), falling with the slipping speed to 0.35 × (half-way at 8 cm/s). Each sample
the string's velocity under the hair without the bow is known from its modes; the bow's force adds
to it in proportion (the modes' shapes under the 16 mm of hair); the friction curve and that line
are intersected (Friedlander's construction, with its hysteresis: a sticking string sticks while it
can, a slipping one slips while it can). Each string is bowed along the bridge's crown at its notch
(the outer strings 24° towards the top).

What had to be added for the string to be caught once a period (Helmholtz's motion):
- **The string's twist.** Without it every string slipped 6 to 25 times a period at any force. The
  string rolls under the hair: a well-damped spring (at once it gives 1 / (2 · 2 Z) per newton, as
  torsional waves running off; held, it springs back at 4 × the string's frequency).
- **A band limit.** The bow acts on the modes below about 3.8 kHz (above, a force held for one
  sample is too coarse: with all 64 modes a heavy bow froze the short strings).
- **Enough force.** Measured over strings, positions, speeds and bow places (the playable window,
  β the bow's place as a share of the sounding length, Z the wave resistance, v the bow's speed):
  least ≈ (0.01 + 0.68 β) Z v / β², most ≈ (0.5 + 8 β) Z v / β. The player presses between the two
  (the air knob). At the bridge (β 0.05) they almost meet. The low strings need much force: the open
  E at 40 cm/s and β 0.11 holds only from 14 N (a real bow gives perhaps 5 to 10 N); the player's
  speeds stay lower.
- **The force before the speed.** With both rising together some notes took up to 580 ms to speak
  (an octave or a whistle first); with the force there in a quarter of the time, 40 to 180 ms.

**Findings.**
- All stopped notes E1 to G4 play within 6 c, harmonics to C6 within 2 c after the hand stops the
  artificial ones 7 c lower (a stiff string's harmonics run sharp).
- A bowed harmonic did not speak with a touching finger modelled only as damping; the bow now
  drives only every n-th mode under such a finger.
- A note on an open string's overtone (A2 with the A string's second) lost 9 dB of its fundamental:
  the open string, ringing at exactly that pitch, holds the bridge still there (with the other
  strings damped: no dip). Real, but real pitches are never that exact: the stopping finger now
  trembles by ~3 c.
- Notes still differ: 15 dB between the weakest and the strongest (3.5 dB std against their
  neighbours), almost unchanged by the body's damping or the microphones. The player evens them by
  ear (`tools/bowlevels.py`, `src/bowlevels_data.h`: 70 % of each note's difference in bow speed):
  1.3 dB std, the worst 4.7 dB.
- Cost: 2.5 µs a sample (12 % of a core), as the plucked bass.

**Not done / open.** Not compared with a recording of a bowed bass yet (none looked for in
`~/Music`). No pizzicato, col legno or bouncing bow (spiccato is only a short stroke). Sustained
double stops only for keys pressed within 60 ms (later keys are a slur). Random playing peaks at
1.13 (clipping at the extreme).

## 8. Sources found

- Guettler, Buen & Askenfelt, "An in-depth analysis of the double bass–stage floor contact" (the
  Helmholtz resonance at about 60 Hz):
  https://www.ioa.org.uk/system/files/proceedings/k_guettler_a_buen_a_askenfelt_an_in-depth_analysis_of_the_double_bass-stage_floor_contact.pdf
- Guettler et al., JASA, "tuning fork or not" (double bass, about 60 Hz):
  https://knutsacoustics.com/files/guettler-et-al.-jasa-tuning-fork-or-not----with-errata.pdf
- Brown 2004, "Acoustical Studies on the Flat-backed and Round-backed Double Bass" (dissertation;
  A0, T1 and the mode table 6.1; read for this document):
  https://awbmusic.com/wp-content/uploads/2021/06/2004_Acoustical_Studies_on_the_Flat_backed_and_Round_backed_Double_Bass.pdf
- Brown 2004, "Documentation of Double Bass Plate Modes Using the Scanning Laser Vibrometer":
  https://awbmusic.com/wp-content/uploads/2021/06/2004_Documentation_of_Double_Bass_Plate_Modes_Using_the_Scanning_Laser_Vibrometer.pdf
- Askenfelt 1982, "Eigenmodes and Tone Quality of the Double Bass", STL-QPSR 4/1982, 149–174
  (five basses; not obtained, cited through Brown and a Finnish thesis).
- UNSW, the violin's A0 and body modes: https://newt.phys.unsw.edu.au/music/people/mclennan_monopole.html
- Le Carrou et al., identification of coupled A0 and T1 modes (harp):
  https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A2_LeCarrou_IdentificationA0T1modes.pdf
- HyperPhysics, the double bass (tuning): https://hyperphysics.gsu.edu/hbase/Music/bass.html
- Liutaio Mottola, bass string length data: https://liutaiomottola.com/formulae/bassString.htm
- Knut's Acoustics, typical string properties: https://knutsacoustics.com/files/Typical-string-properties.pdf
- Bundeswehr University, "Basses II: Diagnosis" (string decay time constants):
  https://www.unibw.de/lrt4/veroeffentlichungen/2000_1_basses-_ii-diagnosis.pdf
- UNSW, torsional modes of steel and gut double bass strings:
  https://www.phys.unsw.edu.au/jw/reprints/WollmanISMA2010.pdf
