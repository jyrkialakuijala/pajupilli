# pajupilli

A real-time physical model of a Boehm concert flute (B foot), played from an
Akai MPK mini mk3. Also the piccolo, alto flute and bass flute, and other woodwinds
on the same air column: recorder, ocarina, shakuhachi, pan flute, clarinet, the
saxophones (soprano, alto, tenor, baritone), oboe, bassoon, duduk and shawm, and the
first brass instrument, the horn in F (`-inst`).

- The bore is a 1-D acoustic waveguide: 96 pressure points at 48 kHz (7.5 mm apart),
  stored as 6 × 16 float32 so the loops compile to AVX-512. Geometry from a measured
  Pearl PF-661 (UNSW); it agrees with a transfer-matrix model of the same flute
  (`tools/tmm.py`) within 1.4 cents rms over the first three resonances of every fingering.
- 16 tone holes with pads that move in 10–16 ms, real fingerings C4–C7.
- An air jet driven by the sound in the embouchure hole, from the player's
  lungs, mouth, tongue and lips.
- About 1-2 % of one core per instrument (the ensemble in a hall: ~7 %).

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
`playing: 3 x flute` (in an ensemble `playing: bass, alto, 2 x flute`) as the instruments
in use change (an instrument counts for 2 s after its last key). A new line takes the free flute that played nearest in pitch and
most recently; silent flutes are not computed. The mix is scaled by 1 / sqrt(flutes
playing). Controls (breath, vibrato, knobs) go to every flute, but
each player has its own vibrato (rate ±8 %, depth ±15 %, its own phase, drifting with the
breath) and its own flutter tongue (23-31 Hz), so the flutes never pulse together. `./flute -polydemo -o
poly.wav` plays a line, a held note under a melody, and chords.

Flutes sounding together tune to each other (adaptive just intonation, `PJ_JI=0` turns it
off): every pair asks for its interval's nearest simple ratio (fifth +2 c, major third
-14 c, minor third +16 c, ...), simpler and closer intervals weighing more; a least-squares
solve gives each flute its offset (within ±30 c), weakly pulled to equal temperament, and
a note already sounding stays where it is so a new note fits the chord and progressions do
not drift. Each flute gets there with its lips, by ear. It prints `tune: C4 +1 E4 -12 G4 +3`.
In the demo the flutes reach their targets within 1-2 cents and the chords' measured
roughness (Sethares, from the partials) drops by 21 %.

### Ensemble (`-ensemble`) and the room (`-room`)

    ./flute -live -ensemble                  # bass, alto, two flutes, piccolo
    ./flute -live -ensemble alto,flute,flute # any list
    ./flute -live -room church               # dry, studio, hall (default), church
    ./flute -ensemble -polydemo -o choir.wav # with a five-voice chorale (stereo)

A flute choir instead of a pool of one instrument: the keys are at sounding pitch over the
whole ensemble (C3 to B7; keys beyond it fold in by octaves). Each instrument plays only its own
range: a note never goes to the piccolo or a flute an octave up; when no free instrument reaches
it, the nearest busy one that does moves to it (`move: E3 to bass (from C3)`; the old key stays
held under it, as in a slur). A note goes to the instrument it suits
(each has a comfortable range; the flutes lead when several would do), a chord is dealt as a
whole, low notes to low instruments, without voices crossing (the keys of a chord struck
within 35 ms may move to a better instrument as the chord arrives; it prints `deal: C3 bass,
G3 alto, E4 flute 1, C5 flute 2, G5 piccolo`), and a line keeps to its instrument. The
just intonation works across the ensemble.

The instruments play in a room, a reverberant space of its own: a shoebox of the room's
volume, the players on a line across the stage (high instruments on the left), the listener a
pair of microphones further in, aimed out to the sides (stereo). The early sound is the direct
sound and the reflections off the walls, floor and ceiling (to second order; hard side walls,
an absorbing audience floor), each picked up by the microphone facing it: a player on the left
comes back off the right wall in the right channel, 30-40 ms later in the hall, the stage
mirrored (about 11 dB between the channels); the reverberant field is a
16-line feedback delay network sized from the room, its decay from the RT60 and its level
from diffuse-field theory (in the hall at the listener about as loud as the direct sound).
Each player feeds it through diffusers along its own random direction and each listener takes
its own: after 50 ms the room is even left and right and uncorrelated, wherever the player
sits (`./flute -roomimp hall` shows it). Every instrument hears the room's sound and the others' direct
sound in its bore: the pressure acts on the embouchure, the open holes and the foot, in series
with each opening's radiation load, so an instrument takes in only as much as it would
radiate. The silent flutes of the pool are computed too, and ring in sympathy. No energy is
made in the loop: a silent flute gives back 0.1-1 % of the room's sound (`./flute -scatter`
measures it), and the walls and the air take the rest as heat at the room's RT60. `-room dry`
is the old behaviour (no room, silent flutes not computed).

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
the bore keeps its 96 points, resampled to 48 kHz. Offline commands take `-inst` too, with
sounding MIDI notes (`./flute -inst bass -note 48`).

### Other woodwinds

| `-inst` | What | Range (written) | Model rate |
|---|---|---|---|
| `recorder` | alto recorder in F, Baroque fingering: duct, thumb half-holed for the 2nd register | F4–G6 | 48 kHz |
| `ocarina` | alto C, 12 holes: a vessel (Helmholtz), duct | A4–F6 | 48 kHz |
| `shakuhachi` | 1.8 (in D), end-blown: the lips cover the rim (meri / kari) | D4–D6 | 48 kHz |
| `panflute` | chromatic nai, one stopped pipe per note | G4–C7 | 96 kHz |
| `clarinet` | in Bb (sounds a tone lower), register key | E3–C6 | 48 kHz |
| `soprano-sax` | in Bb (a tone lower) | Bb3–F#6 | 48 kHz |
| `alto-sax` | in Eb (a sixth lower) | Bb3–F#6 | 48 kHz |
| `tenor-sax` | in Bb (a ninth lower) | Bb3–F#6 | 36 kHz |
| `baritone-sax` | in Eb (an octave and a sixth lower) | Bb3–F#6 | 24 kHz |
| `oboe` | octave keys | Bb3–F6 | 48 kHz |
| `bassoon` | flick-key vents for the 2nd register | Bb1–F4 | 24 kHz |
| `duduk` | in A: a large soft double reed on a cylindrical bore | A3–C#5 | 48 kHz |
| `shawm` | treble in D: a wide cone, a reed behind a pirouette | D4–A5 | 48 kHz |

`./flute -inst NAME -cal` lists, note by note, how the player tunes and which notes do not
sound in their register (see Status). They play live, in `-poly` pools and in ensembles
(`-ensemble bassoon,clarinet,oboe,flute`).

What they share with the flute: the waveguide (now up to 512 cells, so a bassoon's 2.5 m fit
at 24 kHz; the bore's cells take the mean of their faces' areas, the interfaces the area there,
the stable scheme for cones and bells), the tone holes with pads or fingers (a finger slides off,
or half-holes, a hole), key clicks, the radiation and the room. New:

- **The reed** (`src/reed.*`): a damped mass-spring per unit area driven by the pressure across
  it, beating on the lay; Bernoulli flow through the slit (a double reed adds the head loss of
  its duct), the flow it sweeps as it moves; solved with the mouthpiece cell each sample. The lips
  set its rest opening, stiffness and damping; the tongue on the reed stops and damps it.
- **Nonlinear tone-hole losses**: at playing levels the flow in a small open hole separates and
  sheds vortices (a resistance ~ rho |u| / 2 Cd^2 S^2 at each end of the chimney, linearised per
  control block). It is what makes a register vent work: it damps the low mode, whose flow
  through the vent is large, and spares the mode with a pressure node there.
- **The reed player** (`src/reedplayer.*`): breath by the note and the dynamics, the lips tune
  (calibrated per note, soft / mezzo-forte / loud), "tu", "du", breath attacks, slurs; vibrato
  from the breath (oboe, bassoon, shawm), the jaw (saxophones) or both (duduk), none on the
  clarinet; the player listens and tunes each held note with the lips.
- **A duct** (recorder, ocarina): the windway and window fixed, the breath alone sets the pitch,
  so louder is sharper; the calibration learns each note's breath. **End-blown** (shakuhachi,
  pan flute): the flute's lips on the open top, the coverage bending the pitch.
- **The calibration** also learns each note's loudest dynamics that still holds its register
  (a soft reed or a vessel breaks above it); the players stay below it.
- **The geometry** (`tools/design.py`, `tools/instruments.py` -> `src/designs.cc`): bores, hole
  and reed sizes typical of the real instruments; the hole positions (and, for the recorder and
  shakuhachi, diameters; for the ocarina only the diameters), the bore's length and the volume at
  the top of a cone are fitted by least squares so every fingering's resonance lands on its note,
  as an instrument maker would, with a transfer-matrix model that mirrors the waveguide (the same
  holes, pads, reed and window; `tools/design.py -grid NAME` compares the two: the clarinet
  within ~5 c). Cross fingerings and half-holes are found by search. The keywork is simplified:
  a keyed instrument has a hole per semitone of its first register and vents for the upper ones.

### Brass

| `-inst` | What | Range (written) | Model rate |
|---|---|---|---|
| `horn` | single horn in F (sounds a fifth lower), three valves | C3–C6 (partials 2–16) | 32 kHz |

The same air column and room, with these new parts (`research/horns.md` has the physics):

- **The bore** (`src/brass.cc`, designed with `tools/brass.py`, a transfer-matrix model that
  mirrors the waveguide): throat, backbore, conical leadpipe, the cylindrical valve section, a
  long cone and a Bessel flare to a 305 mm bell, 342 cells at 32 kHz (the waveguide now holds
  up to 512). The flare and the cone are fitted for even partials: 2–16 within 10 cents of F1's
  series. The leadpipe and cone meet the cylinder smoothly, because a kink reflects and made every
  other partial 2–3 dB weaker. The cup is a volume in the first cell and the throat's mass is
  in the first interfaces. The waveguide matches `tools/brass.py` within 4 cents, except
  partials 3–6 (up to 13 c), which turn back in the flare.
- **The valves** (`src/horn.*`): each loop is a bore of its own, linked to the two cells at the
  valve's port. Pressing a valve shuts the port while the links open over a ~7 ms stroke, so
  half-valves are real (a slower stroke left a hole in valve slurs). The loops are fitted so partials 4–12 drop by their semitones;
  combinations come out sharp, as on a real horn (1-2-3 by ~55 cents).
- **The lips**: an outward-striking valve (the mouth pressure opens them into the cup) at about
  0.85 of the note, plus a weaker inward-striking second mode 1.3× higher. A one-mass lip alone
  played 50–200 cents sharp of the bore and the upper register did not speak. The lips are heavier
  in the low register. Their Q is 7.5, rising with pitch above 300 Hz: a sharper lip splits each
  slot into two regimes some 50 cents apart, and a slur can leave the note in the sharp one, while
  the top notes need the sharper lip to speak. Bernoulli flow through the opening plus the lips'
  swept flow, solved with the cup each sample.
- **The horn player** (`src/hornplayer.*`): valves and partial from a fingering table, with the
  standard alternates (another partial, other valves; up high, partial 15 instead of a sharp
  valved 16). Breath rises as f^1.45 with the note. "ta" / "da" / breath attacks, each with a push
  of breath that settles over ~80 ms (less when already loud): without it a note took 120–250 ms to
  speak, with it 30–60 ms (A2 ~100 ms). A slur across partials overshoots the lips towards the new
  note by a share of the gap to the next partial (high up the partials lie a semitone apart, so a
  fixed overshoot landed on the neighbour), with a push of air, stronger going up; a valve-only
  slur just moves the valves. A key let go is caught for 40 ms: the next key in that time slurs
  from the old note instead of restarting it, so a keyboard legato need not overlap the keys. The ear trims a held note only
  slowly and slightly (errors over 5 cents, at most 0.8 % of the lips' tension), because a fast or
  wide one pushed notes over the jump in their slot and made them waver.
- **The calibration** for brass scans each note's lips across its slot and keeps the steady
  ranges between jumps. It picks the setting nearest in tune that stays 2 % clear of a jump. It
  checks the note arrived at by a slur from a fourth below and above, and chooses the fingering
  and breath that land in tune both ways. It retunes at each dynamic as far as the note holds.
  `-note` prints each held note's pitch and level wobble; `make test` checks them.

## Offline

    ./flute -note 69 -o a4.wav   # one note, prints its pitch
    ./flute -demo -o demo.wav    # a scale and a phrase
    ./flute -cal                 # what the player learns at startup (cached in ~/.cache/pajupilli)
    ./flute -imp 69              # passive resonances of a fingering (PJ_IMPHOLES: any fingering)
    ./flute -slur 71 72 -dyn 0.2 # slur into a note; pitch and level wobble
    ./flute -ramp 60             # register map: the breath swept x0.25 .. x3 and back
    tools/tmm.py -inst alto 69   # the transfer-matrix reference
    tools/fitgrid.py             # the waveguide against it, every instrument and fingering
    make test                    # ~30 s of checks

The player: lungs → trachea → glottis (throat valve) → mouth → tongue → lips → jet.
Vibrato comes from the lungs (chosen by ear); `PJ_VIBTHROAT=0..1` moves it to the throat;
a soft key gives an "h" breath attack, the glottis opening from a slit.

Blowing technique without a knob: `PJ_WANDER` (0 machine-steady, 1 natural, 2 unsteady:
slow drift of breath support, jet aim and lips, and attacks that vary), `PJ_AIR` (the
Air knob's start value), `PJ_EDGE` (edge hiss level).

Every model idea is an environment variable (`PJ_*`, see `src/flute.cc` and
`src/player.cc`), e.g. `PJ_BREATH=0.03 ./flute -live`.

## Status

- Geometry and air as measured (PLAN step 2): air at 30 °C, humid, exhaled (c = 349.5 m/s);
  Dubos' negative series length on the closed holes; the grid placed on the embouchure; the
  open foot a lumped unflanged end.
- The jet (PLAN step 3) is dispersive (short disturbances convect faster) and limits its
  own growth instead of clipping, so the registers behave like a flute's: the fundamental
  from the threshold up to ~x2.5 of the normal breath, the octave only above that, back down
  at a lower breath; soft breath no longer jumps up an octave. The face and lips load the
  window more as the pitch rises (Dickens 2007).
- Calibration stays inside a player's ranges (Coltman's lip coverage ±1.5 mm, breath
  ×0.85–1.25, jet length +20 / −30 %) and logs the rest as the model's error: C4–C#6 within
  ±2 cents on every instrument with normal breath; the third octave within ~±10 cents except
  C7 (+35–40) and the alto's D#6 (+21); a few third-octave notes need more breath (flagged).
- Known: the dynamic range is still small (C4 pp → ff: 3 dB) and the low register's odd
  harmonics are now weak (PLAN step 4); faint octave islands remain at ~x0.3 of the breath.
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
- Other woodwinds (`./flute -inst NAME -cal` per note; `make test` checks the shares):

  | Instrument | Notes that sound in tune (calibrated, within ~10 c) | Off or silent |
  |---|---|---|
  | pan flute | all 30 | — |
  | ocarina | 20 / 21 | F5 -22 c |
  | recorder | 21 / 24 | A#5, C#6, D6 (second register: the jet barely leaves the first mode) |
  | shakuhachi | 21 / 24 | D#4 -76 c, B5 +130 c (meri notes), D#5 silent |
  | clarinet | 32 / 33 (E3–C6) | one note silent; the altissimo needs the player's vocal tract |
  | saxophones | soprano 26 / 29, alto 28 / 30, tenor 30 / 30, baritone 30 / 30 (the alto scaled x2) | a few top notes; palm keys above D#6 left out (the low mode wins) |
  | oboe | 22 / 28 | Bb3, G#5 silent; B3 +70 c; a few 14–22 c |
  | bassoon | 28 / 30 | B3 -58 c, Eb4 -32 c (second register) |
  | duduk | 16 / 17 | C4 silent |
  | shawm | 10 / 13 (D4–D5) | D#4 -118 c, F#4 -38 c, D4 -10 c; the second register is open |

  | horn | 37 / 37 (C3–F5 within ~10 c on `-scale`; held notes steady within 2 c and a few % in level; notes speak in 30–60 ms; legato scale ~10 ms per change more than 6 dB down) | F#2–G#2 22–28 c off; C#4, D4 ~15 c sharp; many notes hold only a narrow dynamic range (loudest often 0.5); a slur down to G3 from C4 or D4 lands ~55 c flat; slurs crossing partials with valves in the low register (F3→G3, A3↔Bb3) dip for 20–65 ms |

  The designs' passive resonances are within a few cents of their notes (rms 0–10 c on the
  core fingerings; the saxophones and oboe 16–48 c, their octaves stretched: the mouthpiece's
  volume is not a lumped volume at high frequencies); the players' lips take up the rest.
  whistle tones.
