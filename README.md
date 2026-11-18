# pajupilli

A real-time physical model of a Boehm concert flute (B foot), played from an
Akai MPK mini mk3. Also the piccolo, alto flute and bass flute, and other woodwinds
on the same air column: recorder, ocarina, shakuhachi, pan flute, clarinet, the
saxophones (soprano, alto, tenor, baritone), oboe, bassoon, duduk and shawm, and the
brass: horn in F, trumpet (open and with a Harmon mute), trombone and tuba (`-inst`).

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

    ./pajupilli -live           # finds the MPK mini, plays through PipeWire/ALSA
    ./pajupilli -live -v        # also prints the MIDI events
    ./pajupilli -selftest       # a short arpeggio without a keyboard
    ./pajupilli -live -inst piccolo   # or alto, bass (default: flute)

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

With a synth lead (`-inst lead-saw`), the first three knobs work the filter instead:

| Knob | Synth lead |
|---|---|
| Knob 1 (CC70) | Formant spread: each key its own fixed filter offset, within ±12 semitones at full turn (the level is knob 8) |
| Knob 2 (CC71) | Cutoff: two octaves under the note to five over it (up to ~19 kHz); an octave over at rest |
| Knob 3 (CC72) | Resonance, which in a ladder is the Q: about 0.5 at 0, 5 at the middle, 19 at 0.85, self-oscillating from about 0.92. Live prints the Q as it turns |

The knobs start from their defaults until you turn them.

### Polyphony (`-poly N`, default 8)

    ./pajupilli -live            # a pool of 8 flutes
    ./pajupilli -live -poly 1    # one flute only

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
breath) and its own flutter tongue (23-31 Hz), so the flutes never pulse together. `./pajupilli -polydemo -o
poly.wav` plays a line, a held note under a melody, and chords.

Flutes sounding together tune to each other by **the pajupilli intonation** (an adaptive just
intonation of the project's own design, `PJ_JI=0` turns it off): every pair asks for its interval's nearest simple ratio (fifth +2 c, major third
-14 c, minor third +16 c, ...), simpler and closer intervals weighing more; a least-squares
solve gives each flute its offset (within ±30 c), weakly pulled to equal temperament, and
a note already sounding stays where it is so a new note fits the chord and progressions do
not drift. The highest note is tuned less: it is pulled to equal temperament, the more the
less clearly harmonic its sound (shimmer, vibrato, an inharmonic spectrum: the pads' bell,
pipes, glass; `PJ_TOPPULL`), and the notes under it find the chord's justness. Such sounds
are also corrected at most 11 c (the pads and effects always; others by how unclear: a clean
tone keeps the full ±35 c); a note at its limit is held there and the others fit it. Each flute gets
there with its lips, by ear. It prints `tune: C4 +1 E4 -12 G4 +3`.
In the demo the flutes reach their targets within 1-2 cents and the chords' measured
roughness (Sethares, from the partials) drops by 21 %.

### Ensemble (`-ensemble`) and the room (`-room`)

    ./pajupilli -live -ensemble                  # bass, alto, two flutes, piccolo
    ./pajupilli -live -ensemble alto,flute,flute # any list
    ./pajupilli -live -room church               # dry, studio, hall (default), church
    ./pajupilli -ensemble -polydemo -o choir.wav # with a five-voice chorale (stereo)

A flute choir instead of a pool of one instrument: the keys are at sounding pitch over the
whole ensemble (C3 to B7; keys beyond it fold in by octaves). Each instrument plays only its own
range: a note never goes to the piccolo or a flute an octave up; when no free instrument reaches
it, the nearest busy one that does moves to it (`move: E3 to bass (from C3)`; the old key stays
held under it, as in a slur). A note goes to the instrument it suits
(each has a comfortable range; the flutes lead when several would do), a chord is dealt as a
whole, low notes to low instruments, without voices crossing (the keys of a chord struck
within 35 ms may move to a better instrument as the chord arrives; it prints `deal: C3 bass,
G3 alto, E4 flute 1, C5 flute 2, G5 piccolo`), and a line keeps to its instrument. The
pajupilli intonation works across the ensemble.

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
sits (`./pajupilli -roomimp hall` shows it). Every instrument hears the room's sound and the others' direct
sound in its bore: the pressure acts on the embouchure, the open holes and the foot, in series
with each opening's radiation load, so an instrument takes in only as much as it would
radiate. The silent flutes of the pool are computed too, and ring in sympathy. No energy is
made in the loop: a silent flute gives back 0.1-1 % of the room's sound (`./pajupilli -scatter`
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
sounding MIDI notes (`./pajupilli -inst bass -note 48`).

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

`./pajupilli -inst NAME -cal` lists, note by note, how the player tunes and which notes do not
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
  clarinet; the player listens and tunes each held note with the lips. At the oboe's top
  (C6, C#6) the player rolls the lips out, taking less reed, as oboists do for the sharp
  third register: less lip on the blades, half the damping at C#6. Bitten harder instead,
  C#6 held its register only at the top of the bite, 19 c sharp; rolled out, it plays in tune.
  The bassoonist starts a note as a skilled player does: as the tongue leaves the reed, the
  air column is already ringing at the note (a few cycles of flow at its pitch into the
  mouthpiece, scaled with the breath; `ReedParams::prime`, `PJ_PRIME`). Over 144 random
  tongued notes: none silent (5 before), the rise to full level 20 ms (30), 14 ending over
  20 c off (27). Bb3–Eb4 (on the speaker vents, built flat) still settle slowly: they need
  their fingerings refitted.
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
| `trumpet` | B♭ trumpet (sounds a tone lower), three valves, the 3rd slide kicked out for 1+3, 1+2+3 | F#3–C6 (partials 2–8) | 48 kHz |
| `muted-trumpet` | the trumpet with a Harmon mute, stem out (GM 60) | F#3–C6 | 48 kHz |
| `trombone` | tenor trombone with F attachment: a moving slide | C2–F5 (at pitch) | 32 kHz |
| `tuba` | CC tuba, four valves | E1–F4 (at pitch) | 20 kHz |

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

The trumpet, trombone and tuba were added on the same machinery, each researched first
(`research/trumpet.md`, `trombone.md`, `tuba.md`, `mutes.md`):

- **Trumpet**: the Smith-Watkins trumpet that Bilbao & Chick measured (their published bore,
  digitised; its transfer matrices meet their measured resonances 1–7 within 1 %), the main
  tuning slide pulled 45 mm for warm breath, valve loops fitted on it. 1+3 and 1+2+3 are
  27 and 48 cents sharp: the ring finger kicks the 3rd slide out 31 / 57 mm for them, as
  players do. Lips: Adachi & Sato's trumpet mass and width (via Harrison et al.), the
  resonance at ~0.75 of the note; the inward mode's share 1.5× the horn's (with the horn's,
  each slot was sharp but for its bottom edge, where slurs failed).
- **Muted trumpet** (GM 60's usual sound, the "Miles Davis" Harmon, stem out): the mute's cork
  seals the bell 55 mm in, and its neck (36 → 44 mm), chamber (88 × 85 mm) and 17 mm exit are
  the bore's last cells. That gives a parasitic resonance near 160 Hz below the trumpet's own,
  the low notes pulled sharp and the high ones flat (the lips take it up), a chamber formant
  near 2 kHz and 16–25 dB less below 1 kHz.
- **Trombone**: the Conn 8H Bilbao & Chick measured (peaks 1–16 within ~1 %), with an F
  attachment (966 mm loop). **The slide moves**: its 134 cells (both legs and the crook) stretch
  together. Each cell's mass, volume and losses scale with the extension; the grid stays, and
  the Courant number only falls. Positions 1–7 add 172–1184 mm of tube (travel 86–592 mm),
  fitted so partials 3–8 drop by their semitones; the F side's positions are wider apart.
  The slide takes 30–90 ms between positions; a glide (a soft overlapping key) is a real
  glissando, the partials sliding together. A legato on one partial gets the light "da" of
  the legato tongue while the slide travels. From A♭4 up the bore's partials are 8–35 c sharp,
  so the chart lowers those positions ("+75": mm of tube more), as players shade them.
- **Tuba**: no published tuba bore was found, so the shape (throat, cone to a 19.5 mm valve
  cylinder, two-part cone, Bessel bell to 470 mm; 5.05 m) is fitted to the resonances measured
  on a Besson BE994 (Mattéoli et al. 2022), transposed to C. Four non-compensating valves; the
  combinations and some single valves are 40–120 cents sharp on this bore, so the chart pulls
  the last pressed valve's slide for those notes ("+613" mm of tube for E1 on 2-3-4).
- **Brassiness** (all but the horn): at ff the outgoing wave steepens towards a shock in the
  narrow bore. The wave leaving the cylinder is time-warped (each sample travels at c + βu,
  overtaken samples merge) and the difference from the linear wave radiates from the bell,
  shaped as measured spectra fall above 2–3 kHz. The trumpet's ff centroid rises from ~1 to
  ~1.7 kHz; pp–mf it adds nothing.
- **Dynamics**: louder, the player presses the lips together (the rest opening shrinks), so
  they still shut each cycle under the higher breath, and the breath spans more (×3 pp to ff
  around mezzo-forte): the trumpet's ff is 10–12 dB over its mf.
- GM 61 "brass section": an ensemble, e.g. `-ensemble trumpet,trumpet,horn,trombone,tuba`.

### Double bass (plucked)

`-inst double-bass` is a jazz double bass plucked by hand (GM 33), E1 to G4
(`research/double_bass.md`):

- **Strings:** four gut strings, each 64 modes in two directions, on one bridge: the strings not
  played ring along. A hard pluck stretches the string and lets it touch the fingerboard.
- **Fingers:** two calloused fingers take turns; a very hard key takes both.
- **Left hand:** it stops the string when the key goes up. A soft key under a held one is a slide
  or a hammer-on on the same string.
- **Bridge:** a mass on its legs, swaying on its waist.
- **Body:** top and back plates as 16 x 8 grids of masses in a bass's outline, with the bass bar,
  the sound post, the f-holes and a column of air cells. `tools/bassbody.py` designs it, with the
  air resonance at 66 Hz and the main wood resonance 1.69478 times higher.
- **Knobs:** dynamics scales the pull, brightness moves the plucking point, vibrato rocks the
  stopping finger.

`-inst contrabass` is the same bass under the bow (GM 44), E1 to G4, harmonics to C6:

- **Bow:** the hair grips the string by friction and lets it slip back once a period; the string
  also twists under it. The force follows the bow's speed and place, between the least that holds
  the note and the most the string bears.
- **Strokes:** a key is a stroke, down-bow and up-bow in turn; its velocity is the bow's speed. A
  soft key eases in, a hard one bites. A long note turns the bow when it runs out.
- **Legato:** overlapping keys are one stroke: the left hand moves, or the bow crosses to the next
  string. Releasing the new key with the old one still held goes back to it (a trill). Keys
  pressed together (within 60 ms) are a double stop.
- **Knobs and pads:** brightness is the bow's place (over the fingerboard to the bridge), air its
  pressure, the joystick forward a swell, the mute pad held a tremolo, vibrato the fingertip.
- **By ear:** the stopping finger trembles by a few cents, and the player evens weak and booming
  notes with the bow (`tools/bowlevels.py`).

### Concert grand

`-inst grand-piano` (GM 1), A0 to C8: a physical model of a concert grand, with no samples
(research/piano.md):

- **Strings:** 243 strings on 88 keys (one, two or three per key). Each string is held as its
  bending shapes up to 10 kHz in the bass and 20 kHz in the treble, in two directions of swing,
  with the stiff wire's sharp partials; the piano is tuned as a tuner does, octave by octave
  on those partials (stretched: about 6 cents flat at the bottom, 23 sharp at the top).
- **Hammers:** a felt-covered mass thrown at the strings; the felt hardens as it is pressed,
  so a harder key is brighter as well as louder. The felt is soft at its surface (as fitted
  to recordings for gentle blows) and hardens steeply underneath, so a loud note blazes
  rather than swells. Key velocity is the hammer's speed.
  On the felt lies a loose nap: the softest keys never press through it and give a long,
  gentle push, the fundamental with little else (flute-like); between about velocity 20 and
  50 the nap is pressed flat within the blow and the tone opens quickly, then more gradually.
- **Bridge and soundboard:** every string stands on the bridge and exchanges motion with the
  soundboard each sample: a ribbed, tapered spruce plate computed by `tools/pianoboard.py`
  (256 resonances, 96 broad ones above them). The damped strings resting on the bridge load
  the board; a lifted damper takes its key's load away. A string's sideways swing is tuned
  a little apart from its up-and-down swing and holds the bridge less: a note falls quickly,
  then slowly, the two drifting against each other.
- **Dampers and pedal:** releasing a key lets its damper down (the top 18 keys have none).
  The sustain pedal lifts all dampers: every string rings along with what is played. A key
  struck again before its sound is gone meets its own waves: repeated notes differ.
- **Fitted to recordings:** string stiffness, tuning stretch, string loss, unison spread,
  felt hardness and the bridge's stiffness are fitted to notes found in two solo piano
  recordings (`tools/pianofit.py`, research/piano.md section 11): numbers only, no samples.
- **Sound:** a crossed pair of figure-of-eight microphones over the strings, turned 60° to either side,, one turned to
  the bass side (left), one to the treble side (right), hears each part of the board after
  its own distance and by its angle: each partial sits where the board moves with it.
- **Knobs:** brightness (K2) and hardness voice the hammers' felt, softer or harder.
- **The top two and a half octaves** (from F#5, fully from C6): the second partial dies away ~35 %
  faster (1.31-1.47x measured at C6-G7), so the tone settles onto the fundamental. Its own loss is
  x1.6 (`PJ_PIANO_TOPH2`), the board's share unchanged. `PJ_PIANO_TOPHI` does the same for the
  partials above it (1: off).
- **Cost:** about 9 % of a core for a single note, 32 % with the pedal down.

### Upright piano

`-inst upright-piano`, A0 to C8: the same strings, hammers and dampers on a 121 cm upright's
scale and board (Yamaha U1 class; research/piano_variants.md section 3):

- **Scale:** the bass shorter: wound strings 1.19 m at A0 to 0.98 m at B2, plain strings from
  C3 at 1.03 m reaching the grand's lengths at C5. The grand's wire on shorter strings is
  stiffer, so the tenor's partials run sharp sooner and the bass is stretched wider (C1 about
  21 c flat); the step from wound to plain at B2/C3 is heard as in a real upright.
- **Board:** a rectangle of 1.30 m² (the grand's 2 m²) standing upright, grain along the
  diagonal long bridge, the bass bridge low on the bass side (`tools/uprightboard.py`: the
  grand's board tool on this outline, radiating fully from 500 Hz as the grand's). Its bridge
  gives more than the grand's: the treble dies sooner (C6 about 42 dB/s against 27).
- **Sound:** a crossed pair of microphones behind the board (the back open), 60 cm off it, of
  the grand's pattern (between figure-of-eight and cardioid).
- **Not yet:** fitted to recordings (the wound strings' stiffness is an estimate); the wall
  behind the board; the soft pedal moving the hammer rail.
- **Cost:** about as the grand's with the pedal down.


### Honky-tonk piano

`-inst honky-tonk` (GM 4; `src/pianoinst.cc`, `research/piano_variants.md` §3.3-3.4): the upright,
out of tune as its recipe says.
- **Every string of a key its own:** about 1 Hz off (`PJ_HONKY_HZ`): the outer strings of three
  either side, the middle one a little, both strings of a bichord, each ×0.6-1.4, capped at 20 c
  (`PJ_HONKY_CAP`). The cents recipe, `PJ_HONKY_CENTS`, is there instead if wanted.
- **The treble drifting flat:** each string above middle C a further uniform 0..−1 Hz, above C5
  0..−2 Hz (`PJ_HONKY_TREBLEFLAT` scales it).
- **A neglected piano's bad strings:** 15 % of them about 3 Hz flat (`PJ_HONKY_FLAT`,
  `_FLATHZ`; ×0.7-1.3, capped at 40 c).
- **Tacks (or a mandolin rail's rings):** metal between the hammer and the strings, each key its
  own. On each key one string, or two (40 %), meets the metal first: a short knock (a half sine
  of 0.3 ms, ~5 N per m/s of the hammer, ×0.5-1.5 per string, ×0.5-1.5 per key how well its tacks
  sit; capped at a hard blow; `PJ_HONKY_TACK`, `_TACKTIME`). The felt around the tacks is uneven
  (×0.75-1.25) and the face displaced, each string met up to 0.3 mm later (`PJ_HONKY_TACKGAP`). The
  attacks' 2-6 kHz differs from key to key by −2 to +9 dB, ~+3 on average; nothing more over 6 kHz.
- **Worn hammers:** the strings' grooves flatten and harden the felt: K ×8 (`PJ_HONKY_WORN`), the
  felt law's exponent 0.3 lower (`PJ_HONKY_HARD`; F = K x^p with x far under a metre, so a lower
  p is harder felt; much lower makes the contact unstable), the loose outer nap pressed flat
  (`PJ_HONKY_NAP` 0), each key ±30 % its own.
- **The closed case:** the lid down and the front on, the sound comes out through the cabinet.
  Its air rings between the walls (~1.4 × 1.2 × 0.3 m inside): three round trips of 8.2, 7.0 and
  1.75 ms, each fed back ×0.65 through a 1.5 kHz low-pass (the wood and the felt taking the
  highs), mixed in at 1 (`PJ_HONKY_BOX`, `_BOXFB`, `_BOXLP`). Peaks and dips about 13 dB apart
  over 100 Hz-4 kHz, no overall tilt, a tail of ~0.1 s: boxy. The strings' highs still come
  through, lifted 6 dB from ~2 kHz (`PJ_HONKY_TREBLE`, `_TREBLEHZ`). No hiss above: the strings'
  modes stop at 9 kHz (`PJ_HONKY_TOP`), the top is shelved −6 dB from 6 kHz (`PJ_HONKY_AIR`) and
  rolled off at 12 dB/octave from 7 kHz (`PJ_HONKY_LP`).
- **Measured** (a C major chord at velocity 90), against the 200 Hz-1 kHz band, in the attack:
  2-4 kHz −18 dB, 4-6 kHz −28, 6-10 kHz −36, 10-16 kHz −51.

A held A4 beats at about 2-3 Hz, its level wobbling ~10 dB, at its pitch. The strings meet
at the bridge, so past ~4 c the unisons no longer lock: every note beats and the aftersound
goes.
### Electric grand (Yamaha CP-80)

`-inst electric-grand` (GM 3), A0 to C8: the CP-80 on the grand's machinery (`src/cp80.*`,
research/cp80.md), a different piano as data (`PianoDesign`):

- **Strings:** 154 on a short scale (single wound strings A0–F#2, two a key above), about 0.68 m
  through the bass and tenor: stiff, the partials sharp (B ~ 2e-3 at A0: the clank); unisons
  0.4–2 cents apart (a stiff bridge does not pull them together: they beat). Tuned as a small
  piano is, the bass octaves between 2 : 1 and 4 : 2.
- **No soundboard:** the bridge on a heavy plate over urethane strips (Yamaha's patent: a spring
  and a dashpot: `supportR` 4000 kg/s above a 150 Hz corner): the fundamentals ring long (50 s at
  A0), the treble about as on a grand. Read by a piezo per key: the strings' force on the bridge, the 88 in parallel on one bus
  (the top six through a 0.022 uF mixing capacitor, a few dB down).
- **The circuit** (from its 1978 overall circuit diagram): the input's 3.4 Hz high-pass; the
  tone controls, flat at the centre detent as on the panel, at the corners the diagram's parts
  give (bass a shelf at 100 Hz, middle a peak or dip at 340 Hz, treble a shelf at 8.7 kHz,
  +-10-12 dB; the network's wiring through the brilliance switch is not legible on the scan, so
  the gains are assumed): treble on K2; the brilliance switch (high, as default: flat; medium,
  low: -4, -9 dB above 1.5 kHz); the tremolo (K4 depth, K5 speed): a Wien-bridge oscillator
  (0.69–12.9 Hz) switching an LED on for about half of each cycle, the LED a light-dependent
  resistor shunting the signal, the other channel from the inverted sine: an auto-pan that flips
  rather than swings, as recorded (0.92 Hz, each channel dipping 8.5 dB, nearly square).
- **Fitted to recordings** (about 120 notes from six recordings of CP-80s, `tools/pianofit.py`):
  the low bass far darker than a grand's (A0–D2 nearly all below 1 kHz: a softer bass hammer),
  the wound strings' upper partials dying twice as fast as a grand's (wire loss x 3); the tremolo
  above. Still estimates: string gauges, the treble hammers; the recordings' 2–4 kHz in the
  middle and upper registers sits about 5 dB over the model, which no hammer or strike point
  reaches (perhaps the recordings' EQ, perhaps a resonance of the pickup's mount).
- **Cost:** about 0.5 % of a core for a note; with the pedal down about 0.45 of the grand's work.

### Clavinet (Hohner D6, E7, C)

`-inst clavinet` (the D6, GM 8), `clavinet-e7`, `clavinet-c`, F1 to E6: the 60 strings of a
Hohner Clavinet, its pickups, its electronics and the pedals it was played through
(research/clavinet.md):

- **The tangent:** a key's rubber tip strikes its string against an anvil and holds it there.
  The string's end moves 1 mm in the time the key's speed takes (0.7–3 ms) and stays; each
  speaking mode is driven by that end's motion. The low partials come out the same at any
  velocity (a sawtooth); above about 1/(π·τ) a faster key is brighter: the Clavinet's dynamics
  are mostly its brightness.
- **The knock:** the tip landing on the anvil strikes the frame (five resonances, 180 Hz to
  1.15 kHz); the frame moves the sounding strings' ends, and the knock is heard through them,
  about 10 dB under a loud high note's start.
- **Two directions of swing:** below E4, on most keys, the string also swings sideways, tuned
  a little apart and coupled to the up-and-down swing at the rubber tip. The two trade their
  energy: the note beats, 0.5 to 2 times a second, up to about 15 dB deep (the pickups hear
  the sideways swing 25 dB down).
- **Pressing a held key** harder stretches its string: up to 10 cents up, from the breath
  (pad pressure, joystick up; in a MIDI file, channel pressure).
- **The release:** the tip lets the whole string go (1.19 × the speaking length: three
  semitones lower). The vibration and the released bend, a pluck at the anvil, ring on it while
  the yarn damps them (the highs in about 30 ms, the dropped fundamental in about 0.2 s): the
  release "thunk" with its short downward drop. A key struck again while that rings clamps the
  swinging string: what swings on the speaking length goes on under the new stroke (the old note
  decays as it would, not cut off), and only the part beyond the anvil dies in the yarn.
- **Strings:** lengths estimated (0.82 m at F1 to 0.18 m at E6; only E3's 0.678 m is
  published); stiffness as measured on recordings of a D6 from E2 to D#4 (about 5e-5, half the
  published figure there) and as published in the bass and at the top; held notes ring 20 s at
  100 Hz, 8 s at 1 kHz.
- **Pickups:** the bridge pickup 4 cm from the tailpiece (bright, the bass fundamentals weak)
  and the centre one 18.5 to 6.5 cm (warm, its comb notches every 3–5 partials), each reading
  its strings' velocity, with a little of the magnet's nonlinearity. The air knob (K3) is the
  selector: centre, both in phase (the default), bridge, both out of phase (thin).
- **Tone switches:** the D6's four elements in series from the tone node to ground, each
  shorted until pressed (`tools/clavtone.py`: all 15 settings as exact filters). Their
  combinations make notches a simple cascade of filters would not. The brightness knob (K2) walks
  them: Soft, Medium, Medium+Treble (the default), Treble (the 1.6 kHz twang),
  Medium+Treble+Brilliant, Treble+Brilliant, Brilliant. The settings keep their levels, as on
  the instrument (up to 20 dB apart).
- **Electronics:**
  - the D6: the 1:15 transformer, stage 1's roll-off from 5 kHz, the tone switches, stage 2's
    soft clip (a loud chord colours, a single note hardly);
  - the E7: the same network fed from its lower source (12 k against 51 k): the switches shape
    the sound less (drier), stage 2 driven less hard;
  - the C: no transformer and no tone switches: two germanium stages, an uneven clip, a
    roll-off near 14 kHz.
- **Pedals:** a wah on the joystick across (centred: out; heel 450 Hz to toe 1.6 kHz, Q 5; in a
  MIDI file, pitch bend), and an envelope filter on K4 above its usual (the Mu-Tron's "wow":
  the band-pass opens with the attack and closes as the note dies, 300 Hz to 2.5 kHz).
- **Knobs:** hardness: the touch (key speed). For tests: `PJ_CLAV_TONE` (bits 1 Soft, 2 Medium,
  4 Treble, 8 Brilliant), `PJ_CLAV_PICKUP` (0–3), `PJ_CLAV_MUTE` (the mute slider),
  `PJ_CLAV_WAH` (0–1), `PJ_CLAV_ENV` (0–1), `PJ_CLAV_PRESS` (cents), `PJ_CLAV_KNOCK`,
  `PJ_CLAV_SIDE` (the share of bass keys that beat).
- **Fitted so far** (four recordings, 21 notes, `tools/pianofit.py`): the stiffness from E2 to
  D#4; the recorded D#3 is nearest the model's default setting (both pickups, Medium+Treble:
  8.6 dB rms over its first 16 partials), though its comb's first gap lies two partials lower.
- **Not yet:** the scale's lengths and pickup distances (the comb), the decays (the recordings'
  notes are short or effected), the yarn, the knock's resonances and the E7's level at its tone
  node are estimates; the tension's own pitch glide on a hard strike.
- **Cost:** about 1 % of a core for a note, about 6 % for busy playing.

### Ethnic and percussive (modes)

The GM ethnic (105–112) and percussive (113–120) families, each researched first
(`research/sitar_shamisen.md`, `banjo_koto.md`, `kalimba_fiddle.md`, `steelpan_taiko.md`,
`bells_blocks.md`), on one modal engine (`src/modal.*`, `src/modalinst.*`): `-inst sitar`, `banjo`,
`shamisen`, `koto`, `kalimba`, `fiddle`, `tinkle-bell`, `agogo`, `steel-drums`, `woodblock`, `taiko`,
`reverse-cymbal` (the bagpipe and shehnai are woodwinds; the melodic tom and synth drum are circuits).
One voice is one instrument, its player giving the keys to strings, tines, note areas or bells.

The bagpipe (a woodwind, GM 110) has its drones too: a key below the chanter (G4-A5) down to A1 is a
drone, a cylindrical pipe sounded by a single beating cane reed from the same bag, its tuning slide
set to that key (`src/drone.h`: a quarter-wave waveguide, the reed as a pressure-controlled
reflection, so the odd harmonics: the 3rd -10 dB, the 5th -15, the even ones -60), sounding while the
key is held; a pair (A2, A3) sits ~6 dB under the chanter (`PJ_DRONE_GAIN`).

- **The engine**: each vibrator is its lowest modes, each a damped oscillator stepped exactly
  (its free motion over a sample a matrix of four numbers), in two directions a hair apart. Strings
  push a bridge (tension × slope); the body's modes at the bridge move, and their acceleration
  drives every string back: the strings not played ring along. A stopped note is the string made
  again at its shorter length (frequencies, mass, bridge force, obstacles). A bend or a pressed
  string retunes it adiabatically (energy / frequency kept), so a bend neither pumps energy in nor
  loses it.
- **Contact**: an obstacle under a string's end (a sitar's jawari, a shamisen's sawari) is a few
  linear contact patches solved with each step in closed form (van Walstijn, Bridges & Mehes 2016's
  tanpura scheme), stable at a bone bridge's ~1e8 N/m. The jawari's curve is the measured 0.6 m⁻¹:
  the buzz blooms ~0.3 s after the stroke (the centroid from ~1 to ~2.6 kHz) and fades over seconds.
- **Mallets** (sticks, bachi, clapper, rubber tips) are masses on a Hertz contact (F = K c^1.5),
  solved with the step by Newton (an explicit contact rang up on a light bell).
- **Sitar**: Ma (F#3) and jod as melody strings, drones and two chikari (struck after a loud
  stroke), 13 tarafs ringing in sympathy; a soft key above a held one within a fifth is a meend
  (the string pulled along its fret), the joystick pulls it up to a fourth. **Banjo**: a Deering
  Eagle II as Woodhouse, Politzer & Mansour measured it, the light bridge on the Mylar head (the
  500–800 Hz formant; it is loud and dies fast). **Shamisen**: silk strings, the first buzzing on
  the sawari, the skin's measured modes (Kokubun), the bachi meeting the skin a moment after the
  string. **Koto**: 13 strings at 147 N tuned to hirajōshi by their bridges, notes between pressed
  behind the bridge (oshide, reaching pitch just after the pluck); the kiri body's modes
  (Coaldrake). **Kalimba**: tines with the measured 1 : 5.9 overtone, over a ~400 Hz box,
  pressed slowly and let go by the nail. **Fiddle**: bowed **waveguide** strings (two delay
  lines split at the bow, the stick-slip table where they meet, as STK's), whose bridge force
  drives the violin body's measured modes (A0, CBR, B1−, B1+, the bridge hill); a modal string
  under a bow needs modes that turn too far in one sample for the friction's solve.
- **Tinkle bell**: from C5 up a small English handbell (1 : 3 : 4.8 : 5.4 …, doublets); below C5 (down to C3) a tuned church bell, its partials relative to the strike note 0.5 (hum) : 1 (prime) : 1.2 (the minor-third tierce) : 1.5 : 2 (nominal) : 2.5 : 2.67 : 3 : 4, struck once by a soft felt hammer much lighter than the bell (no click: the strike blooms). **Agogo**: conical
  iron shells (Rayleigh's ring law), the oval mouth splitting each pair. **Steel drums**: one
  note area per key on one pan (they ring in sympathy), modes at 1, 2, 4× coupled as Monteil,
  Thomas & Touzé measured: the octave takes the energy within ~20 ms (the pan's bloom).
  **Woodblock**: the lip and its cavity (1 : 1.15), decaying in tens of ms, struck by a stick in contact ~0.5 ms (a quieter click). **Taiko**: the
  nagado-daiko's measured modes, the lowest split by the two heads through the air, a hard hit
  stretching the head (up to a semitone sharp at first). **Reverse cymbal** (allowed to be
  non-physical): a physically rendered crash (dense modes, the measured damping law) played
  backwards, its swell faster for higher keys.
- Levels: their raw loudness spans 50 dB; each is set to the common loudness, the struck ones
  held down so the hardest hit peaks near full scale. `make test`: no blow-ups, NaN recovery,
  real time (the sitar ~0.6 µs per sample, the rest 0.04–0.35), and the pitched ones in tune
  (within ~7 c on three notes).

### Vibraphone

`-inst vibraphone` (GM 12), F3 to F6: a Musser M55 on the modal engine (research/vibraphone.md).

- **Bars:** 37 aluminium bars, each its bending modes (tuned 1 : 4 : 10 in the low register by the
  arch underneath; only the fundamental tuned from G#5 up), a torsion and a lateral mode. Alone, a
  low bar rings about 25 s.
- **Resonators:** under each bar a closed tube tuned 5 cents below it, its air a mode of its own,
  coupled to the bar's fundamental by a spring (the engine's linear coupling). Open, the tube takes
  most of the fundamental's energy and radiates it: louder, and a low note rings about 4.5 s
  instead of 23.
- **Fan:** discs on a shaft over the tubes' mouths open and close the coupling twice a turn: the
  tremolo is the tube coupling coming and going, not a volume knob (a low tube cannot empty
  quickly: at fast speeds the low notes pulse less). K4 switches the motor on and sets the depth
  (off: the discs left open), K5 its speed (0.7–5 Hz).
- **Mallets and damper:** yarn mallets (22 g) on a stiffening contact, as Mike Balter's Pro Vibe
  series (`PJ_VIB_MALLET`): 24 (red, soft), 23 (blue, the default), 22 (green), 21 (yellow, hard),
  25 (silver "jazz", very hard: firm from the first touch; `-inst vibraphone-25r`); all but the
  softest made hard enough for the "blium", a bright start that turns at once into the tone. Each
  mallet hardens gradually with the velocity (a soft stroke on the yarn alone, a hard one through
  to the core: 0.75x at the softest, 1.35x at 127, `PJ_VIB_VELHARD`); the stroke's speed ~
  velocity^1.2 keeps ~30 dB from 16 to 127 (the default; 24 dB with the #25). The bar hangs on cords
  at its fundamental's nodes, which damp its overtones (x0.2 of a free bar's ring, `PJ_VIB_SUPPORT`):
  the highs fall ~22 dB against the note in the first 0.3 s while the tube carries the fundamental
  on; and its dense high modes (~6-16 kHz, 0.1-0.2 s) add the hardest strokes' sparkle. A released
  key is damped by the felt bar in ~0.3 s unless the pedal is down.
- **Cost:** about 1 % of a core with every bar ringing.

### Classical guitar

`-inst nylon-guitar` (GM 25; `research/guitars.md`, `src/guitar.cc`): the first of the guitars, the
classical guitar with nylon strings. E2–B5; notes outside are moved in by octaves.

- **Strings:** six, each swinging in two directions (towards the top plate and along it), fretted
  by shortening. All six end on one bridge: the strings not played ring along with what is.
- **Body:** the air in the box breathing through the soundhole (near 100 Hz), the top's first
  resonance (near 200), the back's, and some twenty more up to 3 kHz, each a mass under the bridge.
  Towards the top the bridge gives readily (loud, soon over); along it, little (quiet, ringing on):
  a note's two-stage decay, set by how the string is plucked.
- **Pluck:** a finger takes the string, pulls it aside and lets it go, first off the flesh (wide,
  slow: round), then off the nail's edge (narrow, quick: bright). Picking is the guitar's own
  voice, and velocity is its colour as much as its loudness: soft, the flesh alone (dark, round,
  intimate); from the middle up a little nail and then more (crisp, woody); at the hardest the
  nail knocks on the string as it takes it (the clack: more upper middle, never metallic).
- **The guitarist:** chooses a string and a fret for every note (near the left hand, low on the
  neck, open where it can be, a chord's notes on different strings); notes that come together are
  one gesture, the thumb on the bass (a rest stroke) and the fingers after it a few milliseconds
  apart. Velocity is how deep the pull and how much nail; the brightness knob moves the hand along
  the string. A key let go damps its string; the sustain pedal lets everything ring.
- **Vibrato:** when nothing else is happening, the left hand warms a held note: the finger pushes
  the string up along its fret and lets it back, about five times a second, so the pitch rises
  some cents and returns. It comes in after 0.6 s of the note with nothing new played (0.45 s when
  the music has been busy; 0.9 s and slower when it has been slow). Mostly on the B and G strings
  from the fifth fret up, never on an open string; a note that comes alone after a pause is taken
  where vibrato is easy and always gets it. In a MIDI file the guitarist listens to the band as
  well: another part's note near his own (within an octave and a half or so) counts as something
  happening. `PJ_GUITAR_VIBRATO` (0: none).
- **Mending the tuning:** with nothing else to do he pulls notes up towards their pure places
  (a finger can only raise a note, and not on an open string). In a minor chord, known by its
  root and minor third sounding on the guitar or in what the band has just played: the minor
  third 13 cents, the minor seventh 17, the major seventh 11, the major thirteenth 18. In a major
  chord: the ninth 4.
  The thirds, fifths and sevenths of major and dominant chords stay as fretted. `PJ_GUITAR_PULL`.
- **Strumming:** three or more strings sounded within 60 ms one after another across the guitar
  (or five or six at once) put the right hand into strumming, and it stays there while such
  chords keep coming. The strings are then caught by a hand passing over them above the soundhole,
  not taken first, so what they still sounded rings into the new stroke. The hand crosses the
  strings at a nearly constant speed however loud the chord is meant (`PJ_GUITAR_STRUMSPEED`,
  1.4 m/s: 7.5 ms from string to string); velocity is how deep the nails reach. Down is the backs
  of the fingers, the bass strings fullest; up is the fingertip's pad, rounder and darker still.
  Flesh and nail together are broad and soft on the string: a strum is warm, thick and velvety,
  and never has the hard nail's sound or its knock however loud (a nail is curved: picking meets
  its stiff hollow side and edge, strumming its rounded back, which flexes). A down stroke hardly
  disturbs what the strings still sound; an up stroke's pad lies on them a moment and takes some
  of it. Chords that come all at once have no direction of their own: the hand swings down and up
  like a pendulum whether it touches the strings or not (a chord one swing after the last goes
  the other way, two swings after the same way again), and with the song's beat known a chord on
  a beat is most often down, on the bar's first beat nearly always; where the last bar's pattern
  was clear there is a small pull towards playing this bar the same way (`PJ_GUITAR_STRUMBAR`).
  It prints `down` or `up` for every strum it finds.
- **The same note again:** never plucked the same way twice. A note louder than the last is a
  rest stroke nearer the bridge, a softer one is taken nearer the fingerboard on more flesh, even
  ones alternate fingers and wander along the string (further when slow, hardly in a fast run),
  equally loud unless the velocities say otherwise.
- A first form from the literature's numbers: not yet fitted to recordings, no fret buzz, slides
  or harmonics. The ring was lengthened by ear (`PJ_GUITAR_RING`; `PJ_GUITAR_RING23` for the first
  and second overtones, `PJ_GUITAR_RING45` for the third and fourth, lengthened less). 0.9 % of a core.

### Church organ (first ranks)

The church organ's first three ranks (`src/organ.cc`, `src/organpipe.h`, `src/organbox.h`,
`research/church_organ.md`), C2-C7, each key its own pipe: an air jet crossing the mouth (its travel a
quarter period, deflected by the flow there and by the field around the mouth, a tanh of the
deflection entering the pipe, its turbulence) driving the column's modes, whose flow deflects the jet
in turn: a self-sustained loop, float32 at 48 kHz.

- `-inst gedackt-8`: the Schwellwerk's Gedackt 8', stopped wood (odd modes: the twelfth ~-18 dB, the
  octave ~-55), voiced against a recorded Bourdon 8' (GrandOrgue's demo sample set, measured in the
  scratchpad only). More wind below C5 (`PJ_GEDACKT_AIR`: a wider bass mouth; the mouth radiates its
  flow's rate of change, ~6 dB an octave weaker for the same flow), narrower mouths above
  (`PJ_GEDACKT_TREBLE`): an even level. The jet's own flow radiates low-passed (4 poles at 2x the
  pitch: its asymmetry gives the even harmonics without a buzz). A breath (a hiss around 1-5 kHz) and
  each note's puff (the wind rushing through the opening pallet, the jet turbulent until the tone
  locks: ~25 dB over the breath, ~200 ms in the bass, ~40 at the top; `PJ_GEDACKT_PUFF`).
- `-inst rohrgedackt-8`: the Positiv's Rohrgedackt 8', a metal chimney flute: the chimney half-opens
  the stopper (even modes: the octave ~-50 dB in the bass .. -30 at the top) and lifts the 4th-6th
  (the 5th ~-20 dB): brighter, "liquid".
- `-inst principal-8`: the Hauptwerk's Principal 8', open metal at normal scale, voiced against a
  recorded Montre 8' (GrandOrgue's demo set): the octave ~-9 dB (-14 at the top), the 3rd ~-20, the
  4th ~-20, harmonics to the 12th; speaking ~230 ms at 8' C, ~70 at middle C, ~20 at the top. An
  open pipe sounds from its **mouth and its top** (half a wavelength above: the odd harmonics
  breathing out of both ends in phase, the even ones in opposite phase, so a listener below the
  gallery, the top farther than the mouth, hears the octave). Its octave comes from the jet: the
  languid set against the jet's swing (`PJ_PRIN_BFRAC`), the jet in the pipe more than half of each
  period, its flow asymmetric; the voicer's octave (`PJ_PRIN_OCT`) on the even modes. Its bass,
  C2-F3, stands in the **façade** in front of the main case, the rest on the Hauptwerk's chest inside
  (C side left, C# right).
- `-inst organ`: a registration, `PJ_ORGAN_STOPS`, a comma-separated list of the stops built (default
  `principal8,gedackt8,subbass16`), every manual coupled to the keys; the Pedal's stops play the lowest
  key held below G3 (`PJ_ORGAN_SPLIT`, 55: the bass line), or every key when only pedal stops are drawn.
  The 37 stops of the research's selection (`src/organ.cc`'s `kStops`; research/church_organ.md §6),
  each regulated against the Principal 8′ as a voicer does on site (its output only), their pitches pure to the
  harmonic (a mutation tuned to its partial, not to equal temperament):
  - **Hauptwerk:** `principal16` (its bass in the façade too), `principal8`, `fluteharm8` (the Flûte
    harmonique: from G3 up double-length pipes overblowing to the octave, round and singing), `octave4`, `quinte223` (2⅔′), `superoctave2`, `rohrflote8`, `mixtur5`
    (2′: 2′ 1⅓′ 1′ ⅔′ ½′ at C, breaking back an octave at each c), `cymbel3` (½′, breaking each octave);
  - **Schwellwerk** (in its two boxes): `gedackt8`, `bourdon16`, `flute4`, `nasard223`, `waldflote2`,
    `terz135` (1⅗′: with the Gedackt, the flute and the Nasard, the cornet), `gamba8` and `celeste8` (the
    strings: narrow pipes, their upper modes keeping their Q, the jet driving them nearly as hard as the
    fundamental, voiced against a recorded Gamba 8′; the bass octave bridged, its voicing towards the
    Principal's, as real string basses are; from C3 up a frein harmonique before the mouth: its loss
    lowers the narrow pipes' Q (~5 Hz more bandwidth on every mode), brakes the octave's precursor and
    lets them take a string's low cut-up, so the middle speaks promptly and surely, C3 ~140 ms and
    middle C ~110 to 90 % in the box (were ~225 and ~160, the octave leading, the fundamental
    stalled), its level and spectrum as before (`PJ_GAMBA_FREIN`, Hz; 0: none); the céleste from
    tenor C, ~6 c sharp: a slow beat; the Salicional and the Violonbass's top likewise);
  - **Positiv:** `rohrgedackt8`, `posoctave4`, `gedacktflote4`, `sesquialtera` (2⅔′ + 1⅗′);
  - **Pedal** (two towers at the case's sides, behind the case and the façade: its flues' breath and
    puff low-passed at ~800 Hz and 6 dB softer, `PJ_PEDAL_NOISELP`; the towers' direct sound softened
    above ~2.5 kHz, `PJ_PEDAL_LP`; its reeds on their open chest inside the case, their sound shelved
    5 dB down above ~300 Hz, `PJ_PEDREED_CASE_DB`, `PJ_PEDREED_CASE_HZ`; every reed's topmost resonator
modes fade out over the last six, else the last one rang the harmonic above it, a narrow spike
6-12 dB over its neighbours at ~17x the pitch: the Posaune from B1 up penetrating): `subbass16`, `octavbass8`, `choralbass4`, `pedmixtur4`
    (2⅔′ 2′ 1⅓′ 1′), `posaune16`, `untersatz32` (stopped, C 16.35 Hz), `principalbass16`, `violonbass16`
    (a string bass), `pedtrompete8`;
  - **the rest of the user's disposition** (beyond the research's 37; all 53 but the unwanted Vox
    humana): `salicional8`, `blockflote4` (Hauptwerk); `swprincipal8`, `swoctave4`, `pleinjeu4`
    (Schwellwerk); `posprincipal8`, `gemshorn8`, `posoctave2`, `quinte113`, `sifflote1` (the last two
    breaking back in the top octave), `scharf4`, `dulcian16`, `chamade8` (the Trompette en chamade, at
    the rail, the loudest stop) (Positiv); `gedacktbass8`, `contraposaune32`, `klarine4` (Pedal);
  - **the reeds** (`src/organreed.h`): `trompete16`, `trompete8` (Hauptwerk), `fagott16`, `trompette8`, `oboe8`,
    `clairon4` (Schwellwerk), `cromorne8` (Positiv), `posaune16` (Pedal). A brass tongue, tuned near
    the pitch, beating against the shallot as the wind closes it; the flow through its gap (Bernoulli's
    √ of the pressure across it, viscous near none) and the resonator's modes solved together each
    sample (2x oversampled); a cone's every harmonic (trumpets, oboe, bassoon), the Cromorne's
    half-length cylinder its odd ones; the resonator's top radiating through a band around ~1.3-1.5 kHz
    (the recorded Trompette's and Hautbois's harmonics there louder than the fundamental). Voiced against
    GrandOrgue's Trompette 8′ and Hautbois 8′. The treble (C5–C7 at 8′): the band rides on the 2nd
    harmonic once the pitch reaches half of it (a small resonator's top radiates higher), so the top
    keeps the middle's spectrum instead of thinning to a sine, and every pipe from middle C up is
    regulated to an even line (it rose ~27 dB to C7). The Clairon breaks back at c‴ (key 84): its top
    octave a reprise at 8′, no reed above C7. A reed does not hear the room (its boot's pressure is
    hundreds of times the field at its top). Its pitch follows a retuning unevenly (a plateau, then
    steep; a 16′ reed's bistable at places): calibrated by secant steps and, where that fails, the best
    of a fine scan. ~50 ns a pipe-sample (under 2x a flue).

  E.g. the plenum: `PJ_ORGAN_STOPS=principal8,octave4,quinte223,superoctave2,mixtur5,cymbel3,subbass16,octavbass8,choralbass4,pedmixtur4`;
  the cornet: `gedackt8,flute4,nasard223,waldflote2,terz135`. The families: stopped wood (the
  Gedackt's voicing), chimney (the Rohrgedackt's), open principal (the Principal's), open flute (a wide
  scale: the octave ~-20 dB, the upper harmonics falling fast). The smallest pipes (above 2.5 kHz) run
  2x or 4x oversampled and their jets are deflected by the flow / their mouth's width, so even the 1⅗′'s
  top (10.5 kHz) speaks. Unison pipes of a key in neighbouring rows of a chest hear each other (and can
  pull each other's pitch). The pipes' calibration (~1.5 s for every stop on 4 threads,
  `PJ_ORGAN_THREADS`) is cached in ~/.cache/pajupilli by the binary and the `PJ_*` settings. A flue pipe
  costs ~30 ns a sample, a reed ~50: rendered by blocks of 16 samples, each enclosure (its pipes and
  their field) on its own thread (they meet only in the wind: its pressures read at a block's start,
  its flows averaged over the block), on 3 threads (`PJ_ORGAN_RENDER_THREADS`) the full organ (all 53
  stops) with eight keys held renders ~1.4x faster than real time on a busy machine, the 37 ~2x.
- `-inst gedackt-rohrgedackt`: both drawn, the manuals coupled: every key sounds both, from the boxes
  and from the rail; the swell box's air ~0.6 C warmer than the church's (`PJ_SWELL_WARM`), so its
  Gedackt stands ~2 c sharp and the two flutes beat slowly.

**The divided chest, the enclosures:** every division stands on a divided chest, the C side (C, D,
E, F#, G#, A#) on the left, the C# side on the right, the biggest pipes at the outer ends: chords
spread across the gallery (an even interval stays on one side, an odd one crosses: every triad spans
both). The Gedackt stands in **two swell boxes** high on the gallery (~2.6 x 2.4 x 1.6 m, ~5 m apart;
its bass octave offset along each box's back wall); the Rohrgedackt in the **Rückpositiv**'s open
case at the gallery rail (its halves C left, C# right; nearer: ~6 ms ahead of the boxes). Each
enclosure is a small room (four delay lines: the round trips across it, an oblique one; Jot damping):
each pipe's mouth drives its modes as their shapes are at the mouth, hears the field there (by
reciprocity, ~ 1 / its pitch: the jet moved by the acoustic velocity) and its chest neighbours (a whole
tone and two away), and is calibrated in tune where it stands (all within the designed +-0.5 c and
the box's warmth). Out through the opening: each pipe's direct sound (delayed by its distance to the
front) and the box's field. **The swell pedal** (the dynamics, `-dyn`, the knob; the breath
controller) opens both boxes' shutters: 0.5 (the default) ~3/4 open, 2/3 and up fully; closed, ~14 dB
quieter and dark (600 Hz .. 16 kHz), the box ringing longer (0.6 s against 0.3), its walls leaking
the bass. **To the two microphones in the nave:** below ~300 Hz they hear every enclosure alike (the
bass's wavelengths against the gallery's width, the church's field everywhere: 8' C within ~1 dB
left to right), above it a few dB more on the enclosure's side (the swell boxes ~6 dB, the
Hauptwerk and the Positiv ~3: `PJ_ORGAN_WIDTH`); spaced microphones' time difference optionally
(`PJ_ORGAN_ITD`, s: off by default, as it combs the mono sum). No velocity (a pallet opens the same however the key is struck), no sustain pedal. Steady
wind for now; the blower and the wind's sag, the other ranks, the church's acoustics follow the
research's plan. ~15 % of a core for the three ranks together in random playing.

**The wind** (`src/organwind.h`, research/church_organ.md §4a): a small network solved every sample.
A centrifugal blower (Q = Qmax √(1 − P/p0), shut off at 1.25 × the working pressure; Qmax 570 L/s,
~34 m³/min, `PJ_WIND_QMAX`) fills a plenum; a regulator per division (Hauptwerk, Schwellwerk, Positiv, Pedal)
feeds its chest through a valve that opens as the chest's pressure falls (stiff and well damped, a
modern organ: `PJ_WIND_STIFF`, `PJ_WIND_VALVE`, `PJ_WIND_DAMP`). Every sounding pipe draws its flow
(~3 L/s at 8' C, ~0.5 at middle C, a few hundredths for a mixture's smallest, ~ √ its foot's
pressure) and its jet runs on its chest's pressure, so a sagging wind makes the pipes flat and a
little softer by themselves (~0.5 c per 1 %, `PJ_WIND_PITCH`). One note: nothing moves; a hymn's
plenum: a few tenths of a percent; the full organ, all 53 stops, with a 10-note chord held (~290 L/s):
the blower falls behind, the chests sag ~8 % over a second or two
and every flue droops ~4 c (the user's "struggle"), recovering when the keys are let go. A relief valve spills above +1 %. **The tremulants** (draw
`tremsw`, `trempos` among the stops): a beater working the division's regulator, its top's volume
swung by ±8 L at ~5.5 Hz (`PJ_TREM_DEPTH`, `PJ_TREM_RATE`; the Positiv's a little quicker and
lighter): the chest's pressure swings ~13 % peak to peak and every pipe on it follows, in pitch
(~±3 c) and level, by itself. **Registrations by name** in `PJ_ORGAN_STOPS`, alone or with stops:
`foundations`, `plenum`, `plenum16`, `grandjeu`, `cornet`, `sesquialtera`, `strings`, `soft`,
`tutti` (all 53 but the Voix céleste, which beats against the chorus), `tutti37` (the research's
37). `PJ_WIND=0`: steady wind.

**The crescendo roller** (`-inst organ-crescendo`, or `crescendo` in `PJ_ORGAN_STOPS`): every stop
built, drawn by one control (live: K1; `-dyn`; in `-play`, CC11): silence at 0, then ppp (the swell's
Gedackt and the Subbass, the boxes closed), pp (the Gamba, the swell's Flûte 4′, the Salicional), p
(the Positiv's flutes, the Gemshorn, the Gedacktbass), mp (the 8′ flutes, the Bourdon 16′, the swell's
Principal, the Octavbass), mf (the Principals 8′ and the Octaves 4′, the Principalbass, the
Violonbass), f (the Principal 16′, the 2′s and mutations, the Mixtur and the Plein jeu, the pedal's
chorus), ff (the Hauptwerk's Trompete 8′, the Oboe and the Cromorne, the Cymbel and the Scharf, the
Positiv's mutations, the pedal reeds 16′ and 8′, the Untersatz 32′), fff (the swell's reed battery,
Fagott 16′, Trompette 8′, Clairon 4′; the Trompete 16′, the Dulcian, the Klarine, the Contra Posaune
32′, the Trompette en chamade): each stop at its point of the roller, drawn or retired at once on the
keys held; the swell boxes opening from closed at ppp to open at mf; ~20 dB (K-weighted) from ppp to
fff, each step louder: from mf on +1.4, +1.9 and +1.6 dB (a held chord, averaged over three), the reeds
regulated against the Principal 8′ (the Trompete +2 dB, the chamade +5).
`PJ_ORGAN_SAY=1` prints the dynamic as it changes.

**One 88-key keyboard as the whole organ** (`-inst organ-88`): its keys in zones: A0-B2 the Pedal
(an octave up: C1 plays the 16′ C, 32.7 Hz), C3-E4 the left hand on the Schwellwerk (an expressive
colour, its box following the level), F4 up the right hand on the Hauptwerk (at ppp-p the Positiv, the
Hauptwerk too loud there; the top octave folded into the manuals' compass); K1 the level: silence,
then ppp .. fff, each division at its rung (`src/organplan.cc`'s ladders). `PJ_ORGAN_SPLITS=48,65`
moves the zones' borders, `PJ_ORGAN_PEDAL_UP` the pedal's transposition.

**Playing by manuals, a piece's registration plan** (`src/organplan.h`): from an organ part's notes by
track, made by rules (no tables per piece): the pedal is the lowest track staying in the pedal's
compass, mostly one voice, clearly under the rest; a trio (two one-voice upper tracks crossing in one
range; a rare double note, under 2 % of a track's time, still one voice: BWV 530's left hand holds two
once) gets the right hand on the Hauptwerk and the left on the Positiv, contrasting colours as strong,
one registration; otherwise
the hands play the Hauptwerk and each large part (the tempo's big changes: a passacaglia and its
fugue) builds from mp (or f) to fff in 8-bar phrases, a step at a time, nudged by its texture; where
the pedal rests for a phrase the hands go to the Positiv (the thinnest phrases to the swell), two
steps softer. For BWV 582 it finds the pedal, the manualiter variations 11-15 and the fugue's
episodes, and builds the Passacaglia from mp to fff at variation 17 and the fugue from f to fff; BWV
525 it plays as a trio. The model plays a key on a division (`OrganModel::openOn`), with couplers
(`sw-hw`, `pos-hw`, `hw-ped` ...) in `setRegistration`; with no pedal track, the pedal takes the
lowest key held below G3.

**In `-play`** an organ part (General MIDI's church organ: `organ-plan`, every stop built) is played by
its plan: every channel on the organ joins one part, its lines each channel's tracks; the plan is
printed with each change's registration on every division. The planner first recognises the kind of
piece: a trio (the right hand on the Hauptwerk, the left on the Positiv, the left moving to the swell
at each section), a cantus firmus (the melody on the swell's cornet, the accompaniment on the
Positiv's flutes), a meditative piece (slow, long notes: the swell's Gamba and Voix céleste), a chordal
one (the romantic grand chœur: the fonds coupled, the swell opening, the reeds at the climaxes), a
contrapuntal one (the organo pleno: Principal 8′ 4′ 2′ and the Mixtur, the pedal's principals and
Posaune; a step more at each new section), a long contrapuntal one (built up phrase by phrase); the
sections are the tempo's changes and the general pauses (each a little more than the last, a clearly
slower one less). Each division keeps its own stops (no couplers but the grand chœur's), so each line
has its own colour. The pedal is detached in the Baroque styles (a quarter of each note given to
silence, 60-180 ms, more before a strong beat), held in the romantic ones. But a pedal note always
sounds its big pipes' speech and 0.1 s more (~0.26 s at 16′ D; written shorter, all of it but 25 ms):
a cold 16′ Principal's fundamental reaches −10 dB only after ~140 ms and −6 dB after ~200 ms, so a
quick line detached by a quarter (0.14-0.18 s sounding) spoke or not by a few ms, a note struck
cold nearly without its fundamental and one struck while its pipe still rang (its key released
under 0.3 s before: ~+10 dB) with it: some notes penetrating, others missing. Organists detach the
pedal less in quick passages.

**Tuning by the spectra** (on; `PJ_JI_SPECTRAL=0`: the interval table above alone): each
key's spectrum from its sounding pipes' modelled partials (each pipe's harmonics as its model sounds them
alone, read once at calibration and cached; a pedal reed's through its case: a Gedackt's odd ones, a
Nasard's at 3×, a Terz's at 5×, a mixture's ranks), every two keys' partials within 36 c of each other
(`PJ_JI_SPECTRAL_WINDOW`; a key's correction still at most ±30 c) pulling their interval to where they
meet, by the weaker partial's level (the beating's depth); no partials meeting, the table's pull, a
tenth as strong (`PJ_JI_SPECTRAL_W` scales the spectra's). A third whose partials don't meet (a
Gedackt's) stays near equal temperament: it doesn't beat.

**A little life in the tuning:** each pipe ±1/4.5 Hz off its stop (`PJ_ORGAN_DETUNE_HZ`), each stop ±1/4.5 Hz
off the others (`PJ_ORGAN_RANK_HZ`), uniform, fixed; the Rohrgedackt at 0 Hz and the Gedackt +0.15 Hz as a
pair (`PJ_ORGAN_ROHR_HZ`, `PJ_ORGAN_GEDACKT_HZ`; the swell box's warmth on top), each pipe's own offset on top: two pipes beat at most ~0.44 Hz whatever their
pitch, a slow swell of a few seconds (in the bass more in cents: ~7 c at 16′ C); pure intervals and an
exact calibration alone sound clinical.

**The pajupilli intonation** (`PJ_ORGAN_JI`, on; 0: equal temperament), the project's own system:
an adaptive just intonation (relatives: Hermode Tuning, Sethares's adaptive tuning) with its own rules,
and on the organ a deliberate imperfection on top (the ±1/4.5 Hz above, the wind's droop). The held keys'
offsets solved as the flutes' (src/ensemble.cc's rules: each pair pulled to its just interval, a new key a little to equal
temperament, a sounding one near where it is, the chord's offset healing to 0 c), every pipe of a key
gliding to its offset (+-30 c, a pedal key +-15, the highest key +-10: `PJ_JI_RANGE`,
`PJ_JI_RANGE_PEDAL`, `PJ_JI_RANGE_TOP`; no glide quicker than 25 ms, none faster than 1 c a millisecond:
`PJ_JI_GLIDEMS`, `PJ_JI_GLIDERATE`): a triad's third 386 c, its fifth 702 c.

**The pitch log** (`PJ_ORGAN_PITCHLOG=file`): every pipe's frequency as it plays (two band-passes at
its pitch, its zero crossings timed once it has spoken): its mean against its tuning, the spread, the
extremes, how long it sounded; for the per-pipe tuning in context (BWV 537: the flues within +-0.3 c
on average, +-4 c at the extremes; the Posaune 16′ within -6..+1).

**The pallets and the sliders:** a pallet opens over 24 ms and closes over 30, each key's on each
division drawn afresh at every press and release, ±6 ms (`PJ_ORGAN_PALLETVAR`: a finger's, a
tracker's, a spring's unevenness; all of a key's stops on its pallet together), the bass's bigger
pallets up to 4 ms slower, the pedal's 6; a
stop drawn or retired under held keys (a plan's change, the roller) moves as a slider, over 80 ms
(`PJ_ORGAN_SLIDER`); a key struck again before its pallet has closed re-articulates its pipe a little,
not with a fresh attack.

**Each pipe its own way into the church** (`PJ_ORGAN_PATHS`, on; src/organroom.h,
research/organ_room_coupling.md): every sounding pipe's mouth, an open pipe's top and a reed's
resonator top has its own direct path and image sources to second order (the nave's walls, the vault,
the floor, the gallery's floor, its case's back), traced around the cases (an open front, the
impost's diffraction, the Rückpositiv's shadow) to a 2 m spaced pair on a tall stand, 4 m high, 10 m into the nave (as an
organ is recorded; `PJ_ORGAN_LISTENER`, `PJ_ORGAN_LISTENER_X`, `PJ_ORGAN_LISTENER_Y`; their balance
against the church's late field `PJ_ORGAN_EARLY_DB`): its own time and level at each microphone, the rows behind
darker, the pedal's towers too (`PJ_ORGAN_PEDAL_OCCL`); each enclosure's field from its opening, the
swell's through its shutters; below 300 Hz both channels alike. The church adds only its late field
for the organ (the room's own early sound for it off). ~19 % more time on full organ.

**Its own nave** (`PJ_NAVE`, on with the paths; src/organnave.h): the late field behind the pipes'
early sound is the organ's own, a 50 × 20 × 20 m stone nave (20 000 m³, `PJ_NAVE_VOLUME`) ringing 7 s
at 63 Hz, 8 s at 125, 6.5 / 5.8 / 5.3 s at 250 Hz-1 kHz, then 4.2 / 3 / 1.6 s at 2 / 4 / 8 kHz and
0.5 s at 16 kHz, where the air alone stops it (large stone churches and cathedrals as measured: St
Paul's in London, the Papal Basilicas of Rome, Wells, Notre-Dame; the sources in the header; each
octave a knob, `PJ_NAVE_T60_63` .. `PJ_NAVE_T60_16K`, all scaled by `PJ_NAVE_T60`). 32 delay lines
mixed by a Hadamard matrix, each line's loss per pass fitted (a gain, six shelves and a peak) so
that every octave, measured as one measures a church (Schroeder, T30), meets its time: within 3 %
from 63 Hz to 4 kHz, 6 % at 8 kHz. Its level is diffuse-field theory (16π/A against the organ at
1 m), so its balance against the pipes' own paths is the physical one, through the same microphones
(`PJ_NAVE_GAIN`, dB, on top). It fades in under the images: nothing in the first 20 ms after the
direct sound (`PJ_NAVE_ONSET_MS`), a third by 20-30 ms, all of it from ~45 ms. The 2 m pair hears
it as a spaced pair in a diffuse field does (sinc(kd)): one below ~50 Hz, its own above ~100 Hz.
The pipes hear it back at the case (`PJ_NAVE_FB`), as they heard the room: weak, and stable with
the reeds and the full organ. The shared church then hears nothing of the organ: no second late
field. Against the old room the bass blooms (a piece +3 dB at 63 Hz, +2 dB at 125 Hz, as loud
overall, its top 2-4 dB quieter), and a chord's pedal rings on for ~8 s. Its cost: <1 % of a
piece's rendering (one more job beside the paths). `PJ_NAVE_LENS` (samples, off): warm air layered
under the vault as a slow lens, each line's length wandering by that much over
`PJ_NAVE_LENS_SECS` (4 s): the decay unchanged, the tail's fine structure moving above ~1 kHz.

**The church warming:** over the first 10 minutes (`PJ_ORGAN_WARMUP_SECS`) the air 1 °C warmer
(`PJ_ORGAN_WARMUP_C`) and 5 % more humid (`PJ_ORGAN_WARMUP_RH`): the flues ~3.3 c sharper, the reeds
~0.3 c flatter (`PJ_ORGAN_WARMUP_REED`, -0.1 of the flues' rise: their tongues set their pitch,
warming with the wind, softer and a little longer): the reeds slip ~3.6 c flat of the flues, as in a
service; the pitch log measures against it.

**The small pipes' chiff:** from C6 up (the 2′ and smaller ranks, the mutations' and mixtures' upper
ranks, the 4′ top) the chiff is a small pipe's: short (~11 ms at 2 kHz, ~5 at 8, from when the wind
reaches the foot), on its first overtone whatever the stop's steady octave (a flute's too), a stopped
pipe's twelfth as gentle as the Gedackt's, each pipe a little its own and each attack too (a quicker
pallet a little more); struck again before its pallet closed, hardly any. `PJ_SMALL_CHIFF_P` / `_F` /
`_S` (principals, flutes, stopped), `PJ_SMALL_CHIFFTIME`, `PJ_SMALL_CHIFFVAR`, `PJ_SMALL_CHIFFPRESS`.

**Air on the move** (`PJ_ORGAN_AIR`, on): now and then (every ~3 min, `PJ_ORGAN_AIR_EVERY`) a draft or
a warm plume reaches one part of the church (the main case and the Positiv, the swell boxes, the left
side, the right side, or only the nave's air), 0.3 .. 1.5 °C warmer or colder, coming on over 15 ..
45 s, staying 1 .. 4 min, dissipating over 2 .. 5 min: the pipes there drift against the rest (the
flues ~3 c a degree, the reeds a tenth the other way) and some of the nave's paths lengthen or
shorten a sample or so, its reverberation's colour shifted till the air settles; `PJ_ORGAN_SAY=1`
says when.

**The finger:** a softer finger opens a pallet up to 2.5× slower and closes it up to 2× (its puff and
chiff gentler, ~ the square root of the speed; `PJ_ORGAN_SOFTTOUCH` scales it). Played live: after a
long melody note (1 s or more, C4 up, within 7 s), a note rising above the previous onset's top is
softer, the more the leap, the more it stands above the recent high notes (their mean, their spread),
the quieter the playing (`PJ_ORGAN_LIVETOUCH`); a high note held long released softly. In `-play` also
from the score: a long high note (G4 .. C6, 0.5 .. 2 s), a phrase's last note (a rest of 250 ms after
it: on the Hauptwerk 0.6, elsewhere 0.35). The bass is played ahead, as organists anticipate their big pipes'
slow speech (~75 ms at middle C, ~ (262 / f)^0.6 longer below): the pedal's notes half the difference
early, a manual's bass a third of it, at most 60 ms (the pedal's C 60 ms, its G 30; the manuals' C 29;
nothing from middle C up; half of it into silence, nothing held or released within 1.5 s; never
ahead of the key's last release; `PJ_PLAY_BASS_LEAD` scales it); the
pedal's soft registrations (pp, p) have the Gedacktbass 8′ with the Subbass 16′, its prompt speech the
attack the 16′ catches up to. Trills (two keys a step apart alternating, 8 notes or
more) stay legato, unshortened; a perfectly quantized one is played alive, ~25 % slower at its start
quickening to ~25 % faster at its end (`PJ_ORGAN_TRILL`, 0.25).

**A head start from the room** (off for now): a pipe about to speak given what the field at its mouth
already holds at its partials. Set into its modes at the key (`PJ_ORGAN_PRESET=1`) it sped a C4 with C3
sounding from ~56 to ~35 ms, but kinked the sound as the jet came in (clicks in trios and arpeggios);
the field driving its modes at the start instead (`PJ_ORGAN_SYMP=1`) slowed its speech (the field
holds the pipe's own sound too). To be reworked.

### Accordion and bandoneón (free reeds)

`-inst accordion` (GM 22) and `-inst bandoneon` (GM 24, the tango accordion): free reeds and a
bellows (`src/accordion.cc`, `src/freereed.h`, `research/free_reeds.md`). Each reed is the minimal
model of Millot & Baumann: a steel tongue's first mode, swinging through its slot (a 45 µm
clearance, a plate ~1-2.5 mm thick, the tip set ~4 % of its length out at rest); the jet through
the gap (Bernoulli, solved implicitly each sample) and the tongue's own sweeping; the air in the
feeding duct, whose inertia makes it speak; the reed's cell, filled through the key's pallet. The
gap opens on both sides of the plate, so there are two puffs a cycle and a nearly square tone. The tongues are
sized by pitch (13 x 2.1 mm at A4, 39 mm long at C1, 6 mm at the top). They start from ~50-100 Pa in
the middle (~200 at the extremes) and swing ~15-25 % of their length. The pitch sinks a little as
they are blown harder. Each reed is tuned in at 600 Pa when the instrument is built (~0.4 s). The
reeds run at 96 kHz (their puffs are sharp), then a half-band filter takes them to 48 kHz.

- **Accordion:** the treble is a French-style musette, three 8' sets. The middle one sits in the
  cassotto (two poles at 1.4 kHz, `PJ_ACC_CASS`), and the outer two are tuned 3.5 beats a second
  flat and sharp at A4 (`PJ_ACC_BEAT`; 0: one set). The tuning is in beats, so the low notes are wider in
  cents, up to 25. Below C3 the bass plays 16' + 8', both in the cassotto.
- **Bandoneón:** two reeds an octave apart on each note (8' + 4'), no cassotto or musette, thicker
  plates, a brighter grille and a harder arm (tango's marcato).
- **Push and pull:** each set has two reeds per plate, one per bellows direction, each tuned a little apart
  (within ~1 c). Every key draws on one bellows. Its pressure follows the arm, ~220 Pa at velocity 0
  to ~2.2 kPa at 127 (channel pressure adds to it); a harder key pushes it up faster (8-45 ms) and opens
  the pallet faster (2-6 ms). A chord takes more air and sags it a little (`PJ_ACC_SAG`). The bellows
  turn every 2.5-5 s of playing, between notes if they can, under held notes if they must. The
  pressure passes through zero and the pull reeds take over after a ~30 ms gap. The pull side
  sounds a little duller (into the bellows).
- **Pitch bend** works as on the instrument (EMS21): the bellows are pushed harder against a pallet
  half shut, so the duct's air grows heavier. The pitch goes down only: ~-13, -29 and -46 c at a
  quarter, half and full bend, a little quieter. A MIDI bend of 2 semitones is not reached.
- The bass box sounds from the left and the treble from the right. 2.5 % of a core for a bass and a
  four-note musette chord (14 reeds).

    ./pajupilli -inst accordion -seq "on 48 80 on 64 80 on 67 80 wait 1.5 off 48 off 64 off 67 on 43 80 on 62 80 on 65 80 on 71 80 wait 1.5 off 43 off 62 off 65 off 71"

### Chromatic harmonica

`-inst harmonica` (GM 23; `src/harmonica.cc`, `research/harmonica.md`): a 16-hole C chromatic,
C3-D7 (MIDI 48-98, the top hole's slide draw D7). It has solo tuning, 64 free reeds (the
accordion's engine, `src/freereed.h`) in 0.35 cm³ chambers on 1.05 mm plates, and A = 440. Notes
outside the range are moved by octaves. Each reed is tuned in place when the instrument is built
(~0.15 s).

- **One player, one breath:**
  - The lungs' signed pressure feeds the vocal tract, a Helmholtz mode set by the throat's
    inertance and the mouth's 40 cm³.
  - The mouth feeds every hole the tongue leaves open, in parallel (tongue blocking).
  - The chambers sound into the cupped hands, a second Helmholtz cavity of 75 cm³ whose opening
    radiates.
  - So a chord takes more air and each of its reeds a little less pressure, and a bend moves every
    note of a chord.
- **Blow or draw, never both:** only the reeds of the breath's direction are ever driven, whatever
  the MIDI file asks (`PJ_HARP_CHECK=1` counts violations). Notes are placed as follows:
  - **A single line:** each note goes where it keeps the breath, then the slide, then the mouth's
    place. C and F exist both ways, and the lungs' fill nudges the choice: drawing when they run
    empty, blowing when full.
  - **A chord:** it takes the breath whose notes have the higher average velocity, plus a bonus of
    6 per note (a near-tie goes to the side with more notes) and the lungs' leaning. The slide
    position is the one most of those notes want, and the holes must fit four under the lips. The
    other notes do not sound.
- **Transitions:**
  - A breath turned round dies away, pauses and comes back: a ~65 ms gap in a slow line, a few ms
    in a quick one.
  - A hole change in one breath leaves no gap.
  - **The slide** travels (~20 ms in, ~20 out on its spring), and both rows are partly open in the
    middle: the old note fades as the new one comes in, ~3 dB down on the way in.
  - **Attacks:** a breath attack from silence takes 20-70 ms; a firm key or a quick line is tongued
    (the pressure built behind the tongue). Many slower notes are scooped into from 0.5-1.8
    semitones below; quick notes are not.
- **Pitch bend:** down, the player tunes the tract just above the bent pitch and listens, moving it
  until the lead note sits there (to -2 semitones, as the files ask, though a real valved reed
  manages about one). Up, a softer breath: +3 c.
- **The hands:**
  - cupped while playing (a 12 mm opening, a resonance near 1.1 kHz, as in the recordings);
  - opened after a phrase or a rest, and on some long notes, brighter and projected (+3.5 dB
    forward);
  - closed again at the next note.
  - The opening and the habit of fluttering them on long notes depend on how much the other parts
    of a MIDI file play in C3-D7 at the time: more drama over a sparse accompaniment.
  - **CC1:** the hand tremolo (6.5 Hz, wandering ~12 %), ~6.6 dB in level and ~200 Hz in centroid,
    as in the recording.
  - `PJ_HARP_HANDS=0` turns the automatic hand moves off.
- 2 % of a core for a four-note chord.

    ./pajupilli -inst harmonica -seq "on 67 70 wait 0.4 on 69 80 off 67 wait 0.4 on 71 85 off 69 wait 0.4 on 72 90 off 71 wait 1.2 off 72 wait 0.6"

### Reed organ (pressure harmonium)

`-inst reed-organ` (GM 21; `src/harmonium.cc`, `research/reed_organ.md`): a Kunstharmonium of the
Mustel / Schiedmayer kind, C2-C7, A = 440. The reeds are blown by the wind, not sucked.

- **The wind:** the feet tread two feeders in turn, each stroke's changeover a few % dip.
  - With Expression (the default) the reservoir is shut off, so the feet's pressure is the reeds'
    pressure and the dynamics. It runs from ~300 Pa at velocity 0 to ~2.2 kPa at 127, and channel
    pressure or CC2 adds to it. A harder chord gets a quicker push (15-60 ms).
  - A heavy chord takes more air than the feeders give, and the pressure sags a little.
  - `PJ_HARM_EXPR=0` gives the reservoir's steady ~800 Pa: the organ-like, even tone.
- **The reeds:** brass tongues (`src/freereed.h`, `sampleDown`). Each sits over its cell *downstream*:
  the chest's wind passes through the reed into the cell, and out through the key's pallet. The
  pallet's felt damps the cell's resonance (Q ~2, ~6 at the top).
  - They are voiced close and stiff, so the pitch moves only a few cents from soft to loud.
  - The bass and top tongues are softer, so every key speaks at the softest wind.
  - Each is tuned in its cell when the instrument is built (~0.9 s).
- **Stops** (`PJ_HARM_REG`), both halves of the keyboard (split at E4/F4):
  - `default`: 1 Cor anglais / Flûte 8' (front, broad, dark) with 4 Basson / Hautbois 8' (back,
    narrow, under the closed forte lids), and the Percussion on;
  - `flute`: 1 alone;
  - `celeste`: 1 with the treble's Voix céleste, 1.5 beats a second at A4;
  - `grandjeu`: 1, 2 Bourdon / Clarinette 16', 3 Clairon / Fifre 4', and 4.
- **Percussion** (`PJ_HARM_PERC=0` turns it off): a hammer starts the row-1 tongues at ~20 % of
  their swing, harder for a harder key. C3 speaks in ~75 ms instead of ~125.
- **Sound:** the front rows go through the case's cloth (two poles at 2.5 kHz). The back rows go
  under the lids (two poles at 1.4 kHz, -6 dB). The bass is a little left and the treble right.
- **The sustain pedal** holds the keys down. There is no pitch bend (fixed reeds).
- 5.7 % of a core for Grand Jeu with an eight-note chord (32 reeds).

    ./pajupilli -inst reed-organ -seq "on 48 70 on 64 70 on 67 70 on 72 70 wait 2 off 48 off 64 off 67 off 72 wait 0.5"

### Hammond B-3 and Leslie

`-inst hammond` and GM 17-19 (`drawbar-organ`, `percussive-organ`, `rock-organ`): a tonewheel organ
through its Leslie 122 (`src/hammond.cc`, `src/leslie.cc`, `src/hammondplan.cc`;
`research/hammond_b3.md`, `research/leslie.md`, `research/hammond_character.md`).

- **The generator:** all 91 wheels turn all the time, geared to one shaft through the 12 gear
  pairs. A is exactly 440 and C is −0.58 c, as built. The wheels are phase-locked, so a chord sounds
  the same whenever it is begun. Each wheel is a table of one revolution of its own pickup's
  voltage: its teeth (2-192), a few harmonics (the bottom octave's "complex" wheels rich in odd
  ones), the tuned filters of wheels 49-91, and its flaws:
  - eccentricity and ovality, working through the gap: they carry the teeth nearer the pickup's
    pole piece and away once (and twice) a turn, and the coupling falls with the gap, never to
    nothing: the fundamental's as (1 + Δgap)^−2, steeper closing than opening; the overtones'
    finer fields fall off faster, (1 + Δgap)^−(2 + 1.6 (j − 1)), so the colour breathes with the
    level. Sidebands at the rotation rate, ~−30 dB;
  - **its own subharmonics:** the wheel's rotation rate (16.35-30.86 Hz by its gear) and that
    rate's harmonics up to 3 × its teeth (8-48 of them), its own levels (~1/k, ±3 dB) and phases,
    ~−40 dB under the tone together (`PJ_HAMMOND_SUBS`, dB); the tone's own harmonics left out,
    the wheel's filter (49-91) on them as on the rest. C3's wheel (8 teeth): lines at 16.35,
    32.7 ... 114.5 Hz, −38 to −59 dB; the filtered wheels keep little of them. Those next to the
    tone (the tone ± the rotation rate) also read as a slight pitch wobble of the wheel's own;
  - a sub-octave line under the 2- and 4-tooth wheels;
  - each tooth a little off in place and depth, and the wheel's own level.
  - **its machined teeth's overtones:** never a perfect sine, each wheel's tooth profile its own,
    up to the 7th harmonic: the 2nd and 3rd ~−30 dB, the 5th ~−34, the 4th, 6th and 7th lower,
    ±4 dB per wheel (`PJ_HAMMOND_WHEELHARM`, dB). The tuned filters of wheels 49-91 take most of
    them off the high wheels (C5: −46 to −72 dB). Being exact multiples of their wheel, they beat
    against the tempered wheels of the drawbars above: the 1⅗' wheel ~14 c sharp of the 5th, the
    2⅔' ~2 c narrow of the 3rd. Middle C on 008000800 beats at 10.4 Hz (±2.8 % of the 1⅗' band's
    level), C3 at 5.2 Hz: the full registrations' slightly dissonant brilliance. Measured on C3's
    unfiltered wheel: 2nd −34, 3rd −36, 4th −41, 5th −31, 6th −48, 7th −43 dB (were ≤ −68).
  - **the valve preamp:** a single-ended triode's 2nd-order curve (`PJ_HAMMOND_VALVE`): even
    harmonics and difference tones growing with the level (C4 + E4 on the 8': their 68 Hz
    difference tone −34 dB, −50 without). Its **microphonics:** the console's valves hear the
    Leslies (~2 m, 6 ms); the electrodes (a mass on a spring, resonant at ~3.3 kHz, Q 8) shaken by
    the swirling sound modulate the stage's gain (~0.5 %, `PJ_HAMMOND_MICROMOD`) and add a little
    of it (~40 dB under the signal, `PJ_HAMMOND_MICRO`): a faint loop through the turning
    speakers. At these settings the brightness changes little (+0.5 dB at 1-2 kHz on a 888
    chord); `PJ_HAMMOND_VALVE=3` gives +3.5 dB, at the cost of the key clicks on full
    registrations (the distortion's highs mask them);
  - **its shaft's, shared with exactly one other wheel:** the generator's 48 assemblies each carry
    two wheels four octaves apart (1-36 with 49-84, 42-48 with 85-91; 37-41 ride alone). A
    bent shaft swells both wheels' levels together once a turn. Its driven gear's runout and
    ovality sway its speed once and twice a turn (its angle up to ~1e-3 rad off: ~1 c of
    vibrato at the rotation rate, `PJ_HAMMOND_SHAFT`). Its torsional flutter on its coupling and
    its starting angle are shared too, while each wheel keeps its own eccentricity, teeth and
    filter. Measured: C2 and C6 (wheels 13 and 61, one shaft) swell together (correlation 0.93),
    their pitch wobbles 0.30 (each wheel's own overtones breathing too); C2 and C3 (two shafts of
    the C ratio) 0.12 and 0.19.

  The organ's serial number (`PJ_HAMMOND_SERIAL`) draws the flaws, and `PJ_HAMMOND_AGE` scales them
  (1: a used organ). The shaft's 20 Hz wobble and the motor's 120 Hz pulses ripple every wheel
  together. The organic rest, all running in absolute time, so chords stay phase-locked:
  - **Flutter:** each wheel sits on its own spring coupling, a torsional resonance of 3-8 Hz
    excited by the motor's pulses (all wheels alike) and its bearing (its own). Each wheel's speed
    wobbles ~0.4 c rms (`PJ_HAMMOND_FLUTTER`), each its own way, so a chord shimmers.
  - **Mains drift:** the synchronous motor and the scanner follow the grid's slow drift, ±0.5 c over
    tens of seconds (`PJ_HAMMOND_MAINS`).
  - **Breathing colour:** where the off-centre wheel runs closer, its harmonics swell more than its
    fundamental, so its colour breathes once a revolution, not only its level.
  - **Loading:** a wheel's voltage sags ~3 % for each contact it feeds beyond the first, so big
    chords and full drawbars compress a little.
  - **Hum and ripple:** the power supply's 60/120/180 Hz hum (`PJ_HAMMOND_HUM`) and its 120 Hz
    ripple on the preamp's gain. With the leakage, the idle floor is ~−53 dB under a note.
  - **Dirty contacts:** a held contact breaks for 0.1-2 ms now and then (`PJ_HAMMOND_DIRT`, 0.3:
    about one crackle in 10 s under a held full-drawbar chord). Each wheel's shaft partner leaks in at −45 dB, and the whole generator whispers behind
  everything at ~−70 dB per wheel.
- **Keys:** upper and lower manuals (C2-C7, nine drawbars with the foldback) and pedals (C1-C3,
  16' + 8').
  - A key closes nine contacts one after another, each bouncing: the click, never the same twice
    and 10-14 dB over the tone above 5 kHz.
  - A hard key closes them within ~0.1 ms, a soft one over ~3 ms, below velocity 20 one by one
    (10-100 ms).
  - Velocity has no other effect.
- **Velocity is the key's speed:** a hard key slams its nine contacts (each closes within ~0.1 ms,
  its resistance gone in ~0.12 ms, bouncing up to 3 times): a sharp click. A soft key lays them on
  (spread over ~3 ms, the resistance falling over ~1 ms, hardly bouncing): a soft one. With all
  drawbars out the click runs from ~3 dB over the tone (velocity 30) to ~21 dB (127). The
  percussion is touch-sensitive as on the modern Hammonds: its ping from ~−5 dB under a soft key
  to full (`PJ_HAMMOND_PERCTOUCH`). In `-play` the velocities also ride the expression pedal where
  the file has none.
- **Percussion** (upper): the 2nd or 3rd harmonic, fast (−20 dB in ~0.17 s) or slow, normal or
  soft. It is single-trigger: legato does not strike it again, and a short gap re-arms it only
  part way. The 1' drawbar is silent while it is on.
- **Vibrato and chorus:** V1-V3 and C1-C3, a delay line read back and forth at 6.87 Hz.
- **Preamp:** the expression pedal (CC11, ~35 dB, the bass held up as it closes) and the tube's
  soft knee, into the Leslie's amp.
- **The Leslie:**
  - an asymmetric soft clip with supply sag (the drive per program: the rock organ hot), then the
    800 Hz crossover;
  - the horn on a 0.17 m arm: to each of two microphones its direct path and two wall images,
    each a delay line read at the moving distance (Doppler) with the mouth's three-band beam, plus
    the base's leakage;
  - the drum: fixed below 200 Hz, its scoop turning above;
  - a 40 ms cabinet tail.

  Each rotor's speed follows a slow PI control, a motor pulling a mass through a belt:
  - the acceleration is the speed error and its integral, limited to what the motor can give
    (horn 6 Hz a second, drum 1.2), with no windup against the limit;
  - gentler going down (friction and coasting) than up;
  - damped at 1.8 (`PJ_LESLIE_ZETA`): it eases in and barely passes its speed.

  The horn, lighter on its own motor, reacts faster: it reaches its fast speed in ~0.9 s and its
  slow one in ~1 s; the drum takes ~2.8 and ~3.1 s. The speeds are 0.8/6.7 Hz (horn) and the drum
  1.6 × slower, 0.5/4.2 Hz (`PJ_LESLIE_RATIO`; the published 122 measurements give ~1.17 ×,
  0.67/5.7 Hz: `PJ_LESLIE_RATIO=1.17`), each cabinet ±3 %. The two rotors turn opposite ways, the
  horn counter-clockwise seen from above, the drum clockwise, so their Doppler swings and beams
  sweep against each other (measured: the drum's left/right phase flips, +160° → −164°, the
  horn's unchanged; `PJ_LESLIE_SAMEWAY=1` turns both one way).

  **Four Leslies** by default (`PJ_LESLIE_COUNT`, 1-4), each its own:
  - its parts off nominal, each up to ±7 % (`PJ_LESLIE_SPREAD`): the motors' strength, the
    crossover, the horn's and the drum's radii, the cabinet's inner reverberation; its own
    flaws (eccentricities, belts, rattle) from its own serial;
  - its speeds, exactly (the ±3 % of a lone cabinet's pulleys would undo the choice): four
    factors whose every cross ratio stays furthest from the fractions up to N/11
    (`tools/leslie_ratios.py --four --q 11`): none within 1.2 % of a fraction over 1-5 (1.230
    is 1.6 % from 5/4, 1.307 2.0 % from 4/3, 1.418 1.2 % from 7/5), the closest misses
    0.6-0.9 % from 8/7, 10/7, 11/9, 13/10, 12/11; scaled to centre on the usual speeds (the
    cross ratios unchanged) (`PJ_LESLIE_SPEEDS`);
  - its manuals, its place and its speed (`PJ_LESLIE_MIX`, shares of upper, lower, pedals):
    | Leslie | place | upper | lower | pedals | speed |
    |---|---|---|---|---|---|
    | centre | in front | 75 % | 12.5 % | 12.5 % | × 1.172 (the fastest), horn/drum 1.6 |
    | left | 2 m left, turned 25° | 12.5 % | 75 % | 12.5 % | × 1.081 |
    | right | 2 m right, turned −25° | 12.5 % | 75 % | 12.5 % | × 0.953 |
    | back | 5 m behind | 12.5 % | 12.5 % | 75 % | × 0.827 |

    (the three others with the published 122's horn/drum ratios, 1.20 on slow, 1.17 on fast).
    Each manual's shares are scaled so its whole power over the four is as before: the balance
    kept. Each Leslie has its own channel of the console's amplification (the expression's bass,
    the tone, the valve stage, its overload): each amplifier distorts its own mix. The back wall
    stands 1.5 m behind the rearmost cabinet. Measured (each cabinet's input): an upper note
    −19 dB in the centre, −34 in the others; a lower one −20 left and right, −36 in the others;
    a pedal −25 behind, −40 in the others. Each path from a cabinet to the listener (direct, the
    side walls, the back wall, its corners, the floor) is fed by the two rays leaving it toward
    it (60° apart, blended).

  Each is fed down (two: ~5 dB, four: ~8 dB), so that together they are as loud as one
  (measured: −20.1 to −20.2 LUFS for one, two or four). Four cost ~20 % of a core (one: 8 %).
  `tools/leslie_ratios.py` searches speed sets whose every cross ratio stays away from small
  whole-number ratios (up to N/17, the simpler the fraction the wider the berth; the closest miss
  decides). The mod wheel (CC1 ≥ 64) or the
  sustain pedal switches fast/slow, and CC82 (as the Leslie 2101's default) switches stop / slow /
  fast (0-42 / 43-85 / 86-127). Measured at the microphones on a fast C6:
  - the Doppler: −39..+44 c;
  - the tremolo: 15-16 dB;
  - the reflections: a chord's partials each swinging 9-22 dB their own way (correlation 0.30).

  `PJ_LESLIE_*` knobs set the speeds, ramps and mic distance.

  **In a room** (the default; `PJ_LESLIE_MICS=close` for the close microphones), the cabinet
  radiates into it rather than being close-miked:
  - six rays leave it at 0°, ±60°, ±120° and 180°, each with its own Doppler (the horn's distance
    along it) and the horn's beam toward it, plus the dummy bell, the drum's scoop, the fixed bass
    and the cabinet's tail;
  - the room's microphones, a pair 2 m apart and 4 m away (`PJ_LESLIE_LISTENER`), hear each ray
    by its own path: the front ray directly and off the floor, the side rays off the side walls,
    the rear rays off the back wall and corners, each with its delay and 1/r;
  - the room's late field is fed the rays' mean: what the Leslie sprays all round, its level
    steady, its Doppler from every direction.

  Against close microphones in the hall, the tail is 3 dB fuller and the left-right correlation
  0.6 instead of the microphones' fixed pattern. The Hammond's own output is +14 dB (its trims
  lowered 10.9 dB; the rest makes up being heard from 4 m), so the room hears a loud Leslie as
  it is.

  **Inside the horn compartment** (the 122, 41 × 29 × 21 in, height first: 737 × 533 mm; the
  user's look at one):
  - **The walls:** louvres on all four sides. Of their sound ~25 % gets out, ~25 % is turned up and
    down by the slats (to the lid and the motor board), ~50 % goes back into the box.
  - **The posts:** the four corners are solid, thick wood, and the two long sides are narrowed in
    their middle: six openings a turn. A path's way out depends on where it meets the wall (out at
    a louvre, none at a post, its edges softened over 1 cm).
  - **The images:** the horn's sound comes back off each surface as from the horn mirrored in it:
    the side walls, front and back turning the other way (the Doppler opposite), the lid and the
    board the same way, later. Each image is its own delay line read, so the overtones meet their
    own oppositely shifted copies: a comb moving with the turn. Against the horn alone, the
    partials' levels change ~33 % faster.
  - **Off-centre:** the horn's axis is off the middle (3 cm back, 4 cm left: `PJ_LESLIE_OFFX`,
    `_OFFY`), so each side has its own delays and response.
  - **The slots as a grating:** 5 cm apart one over another (`PJ_LESLIE_SLOT`), they send the highs
    out as a vertical row of sources. The close microphones, one a little above the horn and one
    below, hear the row at slightly different times: a comb above 4 kHz, different left and right.
  - **The close microphones** are at the compartment's sides (±80°, 0.65 m), in front of the
    louvres, as players mike it.
  - Knobs: `PJ_LESLIE_INSIDE` (the reflections), `_POST`, `_MIDPOST` (the posts' reach).
  - **The horn's own response** on its axis, a fixed filter (the driver is driven: nothing in its
    bell plays back on the organ, so no wave is simulated there, only its effect): the flare's
    cutoff (~650 Hz, 12 dB an octave, a little hump at it), the mouth's reflection's ripple
    (+3 dB at 1 kHz, −2.5 at 1.45, +2 at 1.95, −1.5 at 2.6 kHz, fading as the mouth radiates
    better), the driver's top from ~7 kHz; each cabinet ±4 % in frequency. Through the slow Leslie
    a B5 +3 dB, an F6 −0.6 dB, a C5 unchanged (the drum's). `PJ_LESLIE_HORNEQ` scales the ripple
    (0: flat).

  **Each cabinet's own flaws** (seeded by the organ's serial; `PJ_LESLIE_AGE` scales them,
  `PJ_LESLIE_NOISE` the machinery's sounds):
  - **Uneven rotation:** an off-centre pulley and an unbalanced horn make each rotor run a few %
    faster on one side of its turn (horn ~3 %, drum ~1 %). Each belt adds its own wobble at a rate
    unrelated to the turn.
  - **Hunting:** the induction motors never hold their speed, the fast ~1 % and the weaker slow
    (chorale) motor ~3 %, wandering over half a second.
  - **The dummy bell:** the twin horn's sealed bell at the rotor's other end leaks ~−16 dB, a second
    source going the other way round, its Doppler opposite.
  - **The horn's wind:** air swished off the spinning horn, a 1.5-5 kHz hiss growing with the square
    of its speed and swirling with it (idle at fast: +9 dB over slow).
  - **A loose panel:** a 150-220 Hz resonance driven by the deep bass, which buzzes past its
    clearance (`PJ_LESLIE_RATTLEGAP`) under a loud bass: ~33 dB under the chord's own highs.
  - **The machinery:** the running motor's 120 Hz hum and its 28.75 Hz rumble, the amp's hum, and
    the speed relay's click when the switch moves (40 dB under its measured level, practically
    silent; `PJ_LESLIE_RELAY`, dB over that, brings it back).
  - **The crossover** a few % off 800 Hz (aged capacitors). **The microphones** never quite
    symmetric (±4°, ±3 cm).
- **Registrations:**
  - GM 17: upper 888000000, C3, Leslie slow.
  - GM 18: with the 3rd percussion, fast.
  - GM 19: 888800000, the Leslie fast, the amp driven.
  - Lower 838000000 and pedals 8/3 throughout.
  - Overrides: `PJ_HAMMOND_UPPER`, `_LOWER`, `_PEDAL` (digits), `_PERC` (`off`, `3f`, `2s`,
    `3fs`), `_VIB` (`off`, `V1`..`C3`), `_LESLIE` (`slow`, `fast`, `off`: dry).
- **`-play` plans the part from the file** (`src/hammondplan.cc`), printed under the channel list:
  - **Pedals and manuals:**
    - a low, one-note-at-a-time track takes the pedals, the highest track the upper manual, the
      others the lower;
    - with three tracks (an organist's three staves), the lowest is the pedals and the other two
      the manuals;
    - every channel on one Hammond program joins one organ and one Leslie, its channels as lines;
    - one track holding both hands is split moment by moment: the top note and a block chord
      moving with it go to the upper manual, held chords under a moving tune to the lower, and a
      low note a fifth or more under the rest to the pedals;
    - a tune keeps its manual while it moves by steps.
  - **The drawbars** from the piece's character:
    - quick and linear: the jazz 888 with the 3rd percussion;
    - slow: a ballad's 838/808 with C1 (clean: a slight movement that blurs neither the line nor
      the attack);
    - chordal: a fuller 888800000;
    - the program sets rock and percussive.
  - **A 4' melody:** a tune within a fourth above its accompaniment (in its range), or alone and low (mean under E3),
    gets the 4' leading an octave up with a little 16' and 8' under it: 302800000.
  - **Played very low, registered higher:** when the upper manual's notes lie low (mean under E3: an
    octave; under F#2: two), every drawbar moves to the one an octave above (16' → 8', 5⅓' →
    2⅔', 8' → 4', 4' → 2', 2⅔' → 1⅓', 2' → 1'; the top three have none). The jazz 888000000
    becomes 008880000 (or 000808080); the 8' alone becomes the 4' alone, 000800000. A low line
    stays clear instead of muddy. A little of the sub-octave stays (a third of the 16' an octave
    up: 308880000), as a low line may be low for its weight. A track that never sounds two notes together is a tune,
    whatever its register: all of it on the upper manual.
  - **The file's own balance:** an accompaniment written quieter than the tune (by velocity and
    its channels' expression, on General MIDI's square law) gets a lower manual that much softer,
    3 dB a step. The expression pedal follows only the tune's channels (CC11), the
    accompaniment's loudness being in its registration.
  - **General pauses re-register:** where every line rests half a beat and a quarter second or
    more, one more drawbar is pulled out (the 4', the 2', the 2⅔', the 1', the 1⅓', the 1⅗').
    At most four; with more pauses, the ones nearest 35, 55, 75 and 90 % of the piece, so the
    organ grows toward the end. The lower manual and pedals follow, the sliders gliding.
  - **Routing:** `hammond-upper-manual`, `hammond-lower-manual` and `hammond-pedal` are the same one
    Hammond and Leslie, their notes kept on that division whatever the plan says, e.g.
    `-reinstrument grand-piano=hammond-upper-manual,double-bass=hammond-pedal`.
  - **The harmonic percussion** (3rd, fast) where the upper manual's line is mostly single notes,
    detached and moving (≥ 60 % single, < 45 % legato, ≥ 1.5 notes a second): the jazz line.
    - Not in legato, chordal, slow or rock playing.
    - One choice for the piece. A single 8-bar-or-longer stretch covering a quarter of the piece
      that clearly asks the other way switches it there and back: at most two changes, mostly none.
    - The percussive organ (GM 18) keeps it throughout.
    - The jazz setting is Jimmy Smith's: 3rd, fast, **soft** (the ping without the normal setting's
      drop of the whole manual). A ballad's tune of single, spaced notes (mostly detached, 0.4-2.5 a
      second, mean over G4) gets the 2nd, slow, soft: a bell under each note.
    - **The touch chooses the ping:** single-trigger, it strikes only after every upper key has
      been up (~35 ms to charge). Where it is on, a legato move into a bar's first note (4/4
      assumed) or a clearly louder note (+12) is detached: the note before let go 45 ms early, at
      most one every 1.5 s, never while another upper key is held. The other legato moves stay as
      written, without the ping (`PJ_HAMMOND_DETACH=0`: none).
    - **The jazz upper with the percussion is 888000008:** the 1' is silent while the percussion
      is on (its busbar taken for the ping) and comes in where the percussion is switched off.
    - **The lower manual as the percussion's stand-in:** an organist's trick: "if you are not
      playing left hand bass and need an extra octave of percussion-like sound, set your lower
      manual drawbars to 008880080". With the percussion on (3rd, fast) for the whole piece and
      the left hand comping (mostly chords) above the bass (the pedals carry it, or the lower's
      notes average above G3), the lower manual gets 008880080's shape at its planned level,
      e.g. 005550050 (`PJ_HAMMOND_LOWERPERC=0`: never). With the left hand's own bass it keeps
      its derived registration (jazz: ~748, the classic 838/848).
    - **The lower's vibrato off under a fast Leslie:** C3 on the lower manual under fast rotors
      smears the accompaniment. Through each planned fast phrase its vibrato tab is off (dry,
      tight comping under the upper's C3) and back on after.
  - **The vibrato tabs:** C3 on both manuals by default. Counterpoint (a moving pedal line, the
    lower manual a single line of its own, plain harmony under 0.3, not chordal) is played straight,
    both tabs off, the lines clear as on a pipe organ (BWV 525, 537, 541). A lower manual carrying the
    bass itself (no pedal line, its single notes under F3) has its tab off, the bass tight, as trio
    organists play the left hand's line. The plan prints `vibrato C3 upper only` / `(tabs off)`.
  - **Preset keys:** a re-registration at a pause shorter than 0.7 s (`PJ_HAMMOND_PRESETKEY`, s) is
    made by the B preset key, the upper manual's new registration at once; at longer pauses the
    drawbars are pulled and glide as before.
  - **The ending,** on a final chord (the notes sounding at the end, from the last of them begun; its
    length by the tempo map, a ritardando included) of 1.5 s or more (`PJ_HAMMOND_ENDING=0`: none):
    - the Leslie (when planned), on four in five such chords of 2 s or more
      (`PJ_HAMMOND_ENDLESLIE`): fast at 12 % of it, slow again at 80 %, the rotors still winding
      down when it is let go; already fast, slow at 35 %;
    - the expression pedal (when the file has no CC11): a ballad's closing from 30 % of the chord to
      the end (to 0.6 of the pedal, ~−12 dB); otherwise the swell, the chord struck, eased back to
      0.85 and opened past where it was (1.2) by 75 %;
    - two more drawbars pulled out through it (the next of the pauses' order), the chord opening up:
      not a ballad, not on harmony thinned to one foundation;
    - the rock organ, on one ending in two (`PJ_HAMMOND_DIVE`): **the run switch off** half way
      through it. The generator coasts down on its inertia (`PJ_HAMMOND_COAST`, 4 s), its pitch
      sinking and its voltage falling with its speed, the scanner slowing with it: a held C4 falls
      ~750 c in 1.6 s.
  - **Glissandos:** the palm swept up the white keys from an octave and a bit under the note into it
    (0.16-0.26 s, each key held into the next, the last let go just before so the percussion strikes
    the note). Into the first note of a planned fast phrase (jazz, rock, full, percussive), and on
    the rock organ or a full chordal piece into the music resuming after a re-registering pause (the
    later ones first) or into the final chord. Only where the upper manual is free for the sweep, not
    on complex harmony (0.3 or more) nor in a ballad; at most two (`PJ_HAMMOND_GLISS`).
  - **The expression:** with no CC11 in the file it follows the velocities (slowly, as a foot).
  - **The Leslie:** the file's own if it sends CC82, or CC1 or CC64 on a channel it wrote for a
    Hammond (a reinstrumented piano's sustain pedal is its damper, not the Leslie's switch: ignored
    on the organ). Otherwise it is planned:
    - most pieces stay slow throughout;
    - about one in four (one in two on the rock organ) goes fast in its strongest phrase (the most,
      loudest notes a second between the band's rests), or in two;
    - the choice is drawn from a hash of the notes, so the same file gets the same plan every run.

    - **A nudge on a long note:** on a note held 1.6 s (and two beats) or more alone, with nothing
      beginning in its middle 60 %, the slow rotors are asked a little faster, 1.6-2.4 x the slow
      speed (drawn from the notes), from ~45 % of it to ~70 %. Through their slow PI control the
      horn swells and settles (0.8 → ~1.8 Hz and back) while the drum barely stirs. Only where
      the plan has the Leslie slow, at most one every 8 s.
    - **Trills with a rising touch:** a line alternating between two neighbouring notes (a
      semitone or a tone), six notes or more each within 0.2 s (or four within 0.1 s), is played
      from about half its velocity up to 1.3 × (at most 127): the contacts struck ever harder, the
      key clicks sharpening through it (`PJ_HAMMOND_TRILL`; BWV 537 and 525 have seven each).
    - **Harmonic complexity:** at every onset, over what sounds then: the pitch classes beyond a
      triad, a semitone or tritone among them, a second or third under C4 (0..1; Bach's chorales
      ~0.2, jazz standards 0.3-0.45). From 0.25 the 16' and 5 1/3' fade, from 0.4 the upper is a
      single foundation, 008300000 (or 000800000 when its tune is low): complex harmony on the
      888's sub-octave and fifth is mud (`PJ_HAMMOND_COMPLEX`).
    - **Clarity for the lower manual and pedals:** a pedal line (more for a busy, walking or
      contrapuntal one) and rich harmony ease the lower's 16' and 5 1/3' back and put the pedals on
      less 16', more 8' (`PJ_HAMMOND_CLARITY`). BWV 537 (`-reinstrument organ=hammond`): lower
      637000000 → 428000000, pedals 82 → 63.
    - **Trills thinned:** from a planned trill's start the manuals' drawbars other than the 8' and
      4' glide down by up to 80 % (888000000 → ~228000000), back after it (`PJ_HAMMOND_TRILLTHIN`):
      a trill on a thick registration is a blur.
    - **The lower manual crowding the pedals:** its held notes low (under C4) and close above the
      pedal note (within an octave and a half), where its 16' doubles its line into the pedals'
      register: its 16' and 5 1/3' eased back (up to 75 / 70 %), followed over ~0.8 s rising,
      ~3 s falling, slight and gradual (`PJ_HAMMOND_CROWD`). BWV 537 at 1:45-1:51: the lower's 16'
      ~4 → ~2.3.
    - **The upper manual low over the others:** its notes under G4 and close above the lower's or
      the pedals' (within an octave), where its 16' doubles its line onto theirs: its 16' and
      5 1/3' eased back (up to 75 / 70 %), its 4' up 1.5 steps, as gradually. BWV 537 at
      1:40-1:53: the upper's 16' and 5 1/3' 8 → ~4.5.
    - **The lower manual carrying the bass line:** the pedals resting over a second and its low
      notes moving (over ~1.5 a second): its 4', 2 2/3' and 2' drawn out (up to 3, 2, 2 steps) for
      the line's definition, over ~0.8 s, back over ~2 s (`PJ_HAMMOND_BASSLINE`). BWV 537
      4:17-4:30. `PJ_HAMMOND_MUDLOG=1` prints all these corrections every second.
    - **Muddiness corrected online:** while playing, close intervals held low on the manuals (a
      second under C4, a third under C3) and a pedal note with manual notes close above it under
      C3 ease the manuals' 16' and 5 1/3' back (up to 70 / 80 %), the pedal's 16' (up to 50 %), and
      give the 4' a little more; the sliders glide there in ~0.3 s and back in ~1.5 s when it
      clears (`PJ_HAMMOND_MUD`). A held C3-D3-E♭3 on 888: its sub-bass −5 → −13 dB of the sound.
    - **The fast switch on a long held chord:** where three notes or more are held unchanged for
      2.5 s or more (nothing begins or ends). Where the chord is struck there, an organist's way:
      "I switch to fast just before I hit the note and switch back to slow in the middle of the
      note". The switch goes to fast 0.1-0.3 s before the chord and back to slow at 40-60 % of it:
      the horn swells up to speed and settles, the drum half way. A chord whose notes began at
      different times gets the short flick inside it instead: fast for 0.35-0.75 s in its first
      half, back before the rotors get there. About two in three such chords
      (`PJ_HAMMOND_FLICK` 0.65), half as many in a slow part (under a moment a second over the
      10 s around it: "sometimes I leave it on slow over a slow part or a long note just to have
      that clear sound"). 10 s apart at least, only where the plan has the Leslie slow; such a
      chord is not nudged too. Measured on a struck 6 s chord: the horn 0.8 → 7.9 Hz and back to
      0.9 Hz before it ends. The plan's speed changes are applied
      by the clock, as the nudges are: a held chord has no events to carry them.
      `PJ_HAMMOND_FASTSHARE` sets the share of pieces with a fast phrase.

    Played live, the player rides it by the music's intensity instead.
  - The `PJ_HAMMOND_*` overrides apply over the plan.
- **Live, the MPK's knobs (K1-K5):**
  - **K1-K4 draw the upper manual** along the tradition's own paths (not a linear map):
    - **K1, soft to hard:** 16 of the most used registrations in the order an organist pulls them as
      a tune grows, piecewise linear between them, in whole steps:
      008000000, 008400000, 808000000, 838000000, 868000000, 688600000, 888000000, 888000008,
      888400000, 888600000, 888800000, 888808000, 888880000, 888888000, 888888880, 888888888,
      continuously between them. Its last third also drives the 122 harder (to 2.5x).
    - **The lower manual follows the upper**, as organists derive the accompaniment's
      registration from the melody's: of the same family, softer and darker. Each drawbar keeps a
      share of the upper's, then a step less (`PJ_HAMMOND_LOWER_SOFTER`, 1 ≈ 3 dB). The shares are
      16' 0.85, 5⅓' 0.45, 8' 1, 4' 0.55, 2⅔' 0.45, and the top 0.3 down to 0.15. So the jazz
      888000000 gets ~637000000 below it, close to the classic 838, and full organ a mellow
      tapering 6.3.7.3.3.1.1.1.0. `-play`'s plans derive their lower manual the same way. Under
      a pedal bass (the plan's clarity) the 5⅓' eases further, toward another organist's
      favourite, 888000000 over 808000000 with pedals 80. A lower or pedal set by hand
      (`PJ_HAMMOND_LOWER`, `_PEDAL`) is kept through the plan's re-registrations.
    - **The pedals follow too** (their two drawbars): the 16' by the upper's loudness, the 8' by
      its brightness (the share of its 4' and above). Round registrations get the simple pedal,
      bright ones a brighter one: whisper 5.7/1.5, ballad 7/1.5, jazz 8/1.5, gospel 8/3.5, full
      8/6.8. `-play`'s plans derive theirs the same way.
    - **K2, hollow to full:** left thins the registration to the 8''s odd harmonics (8', 2⅔', 1⅗':
      the clarinet's hollow); right fills in the octaves and mutations (a chorus).
    - **K3, dark to bright:** left takes the top away; right adds the whistle (1', 1⅓', 2').
    - **K4, light to deep:** left removes the 16' and 5⅓'; right adds the 16''s growl.
    - The middles of K2-K4 change nothing, so K1 alone walks the classics.
    - **The drawbars are continuous**, between the steps too (3 dB a step, below the first fading to
      silence). Each is a slider pulled by a hand: a PI control of its speed toward its goal
      (`PJ_HAMMOND_DRAWBAR_KP` 8, `_KI` 20, top speed `_SPEED` 16 steps a second, stopped at its
      ends, no windup against them). It glides there in a few tenths of a second, a touch past
      sometimes, never in steps: the 16' pulled in from 8 fades 20 dB in ~0.4 s. A plan's
      registration changes glide too.
    - The registration is printed as it changes, with the station's name when it is one.
      `PJ_HAMMOND_KNOBS=k1,k2,k3,k4` sets them offline.
  - **K5 is the Leslie's half-moon:** stop (brake) to the left, slow in the middle, fast to the
    right.
- 2.8 % of a core for a five-note chord with the Leslie.

    ./pajupilli -inst hammond -seq "on 48 100 on 60 100 on 64 100 on 67 100 wait 3 off 48 off 60 off 64 off 67 wait 1"

### Harpsichord

`-inst harpsichord` (GM 7; `research/harpsichord.md`, `src/harpsichord.cc`): a large North German
double of the Hass kind on the grand's string engine: 61 keys F1–F6 and four choirs of strings,
two at pitch (8'), one an octave up (4'), one an octave down (16'). Each string is plucked by a
plectrum on a jack: the jack rises at the key's speed, the plectrum bends under the string and
lets go at a fixed force, so how hard a key is struck hardly changes the sound, as on the
instrument. Each string has its cloth damper.

    ./pajupilli -live -inst harpsichord        # 8' + 8'
    ./pajupilli -live -inst harpsichord-8      # one 8'
    ./pajupilli -live -inst harpsichord-full   # 16' 8' 8' 4'
    PJ_HPSI_STOPS=16,8,4 ./pajupilli -live -inst harpsichord   # any registers

- **Registers:** a register on, its row of jacks plucks; off, its strings keep their dampers and
  only ring along through the bridge. The rows pluck one after another, not together.
- **The pedal** (CC 64) is a register pedal, as revival instruments had: down, it adds the 4' and
  the 16' from the next pluck on.
- **Tuning:** just intonation, fixed before playing as a harpsichord's is: every interval from
  the key's tonic a ratio of small numbers (1, 16/15, 9/8, 6/5, 5/4, 4/3, 45/32, 3/2, 8/5, 5/3,
  9/5, 15/8), every octave alike. In `-play` the tonic is the file's key signature or, without
  one, the key its notes fit best (printed at the start); alone it is C. `PJ_HPSI_TONIC=0..11`
  chooses it, `PJ_HPSI_TUNING=equal` gives equal temperament.
- **Tone:** the rows pluck at different places (the back 8' an eighth of the string from the nut
  in the bass to a third in the treble, the front 8' a little further on, the 16' nearer the nut);
  a string's two directions are tuned 0.8 % apart by the bridge pin: the slow beat in the sound.
- A first stage: the soundboard is still the grand's, and the lengths behind the bridge, the air
  in the case, the buff and lute stops and the pedal instrument of the plan are not built. All
  four choirs under dense random playing: about 36 % of a core (the grand with its pedal down: 29 %).

### Chromatic percussion

General MIDI 9–16 on one struck-bar engine (`src/bars.cc`; `research/chromatic_percussion.md`), with
the vibraphone above and the dulcimer on the piano's strings. Notes outside an instrument's compass
are folded in by octaves. A mallet meets its bar where the bar is, so a repeated note may add to
the ring or take from it, and no two strokes land in quite the same place.

| `-inst` | GM | Compass (sounding) | What sounds |
|---|---|---|---|
| `celesta` | 9 | C3–F8 | steel plates on felt, a tuned box under each, felt hammers, dampers and pedal (below) |
| `glockenspiel` | 10 | G5–C8 | thick steel bars, bare, hard mallets: 1 : 2.76 : 5.40 : 8.93, all ringing on |
| `music-box` | 11 | C4–C8 | a comb's teeth (1 : 6.27 : 17.5), plucked by pins, heard through the box |
| `vibraphone` | 12 | F3–F6 | (above) |
| `marimba` | 13 | C2–C7 | rosewood bars tuned 1 : 4 : 10, a tube under each, yarn mallets |
| `xylophone` | 14 | F4–C8 | rosewood bars tuned 1 : 3 : 6, hard mallets, short notes |
| `tubular-bells` | 15 | C4–F5 | brass tubes struck on the end: the note heard is an octave under the fourth mode |
| `dulcimer` | 16 | G2–A6 | courses of steel strings, light leather-faced hammers, no dampers (`src/dulcimer.cc`) |

The celesta is fitted to a recording. The other six are first forms from general acoustics, not
yet checked against the literature or recordings; the dulcimer still sounds through the grand's
soundboard.

#### Celesta

`-inst celesta` (GM 9; `research/celesta.md`): a 5½-octave instrument, C3–F8 at
sounding pitch, every key its own plate, box, hammer and damper.

    ./pajupilli -live -inst celesta        # the sustain pedal lifts all dampers

- **Plates:** flat strips of spring steel resting on felt at the two points where their lowest
  mode does not move, tuned by their length for that mode alone. Their overtones are a free
  strip's, none of them harmonic (2.73, 5.35, 8.84, 13.3 times the fundamental), and the twisting
  modes, which lie close over the fundamental in the short top plates. The felt takes every mode
  that moves on it within tenths of a second and spares the first.
- **Boxes:** a wooden resonator under each plate, its air tuned to the plate. The air is what is
  heard, and by sounding it takes the fundamental's energy: the box sets how long a note rings
  (about 11 s at middle C, 1 s at C8).
- **Hammers:** felt on a small head, soft and heavy in the bass, hard and light at the top, each a
  millimetre or two off its plate's middle. **Dampers:** felt on the plate's far end.
- **The sound:** nearly a pure tone, with a metallic edge at the attack (the 5.35 partial, about
  33 dB under) and the knock of the action in the case's wood under it. Those three, and the ring
  times, are fitted to a recording.
- About 0.6 % of a core. `PJ_CEL_PLATE` (the metallic edge), `PJ_CEL_KNOCK`, `PJ_CEL_RING`,
  `PJ_CEL_HARD` (the felt), `PJ_CEL_A4`.

### Sound effects

GM 121–128 as physical systems too (`src/sfx.*`, on the modal instrument's frame):
`-inst fret-noise` (a fingertip sliding over a wound string's windings, 0.25 mm apart: a squeak
at the slide's speed, the string and a guitar body ringing with it), `breath-noise` (the choir's
vocal tract whispering: the folds apart, the glottis's turbulence through the tract, the key
choosing the mouth's shape), `seashore` (bubbles: each rings at Minnaert's frequency 3.26 / R,
damped and chirping upward as van den Doel models liquid sounds, their sizes from a power law,
their rate following each wave's crash, wash and backwash), `bird-tweet` (the syrinx's labia as a
van der Pol valve driven by pressure and tension gestures after Gardner et al.: syllables sweeping
down, the beak's resonance following the note), `telephone` (a polarised ringer: the 20 Hz ring
current, 2 s on and 4 s off, turns the armature against its spring, ~18 Hz, and its steel ball
swings into one gong, then the other, through Hertz contacts solved with the step; each gong 16
modes of 4 numbers, frequency, decay, shape at the strike, radiation: a stamped bowl's (m, n)
families as doublets), `helicopter` (the blade-vortex slap once a blade passage, the thickness
noise, the tail rotor, the turbine; spinning up and down), `applause` (an audience of up to 960
clappers in 40 rows, each with their own hands, the air between the palms a resonance as Peltola et
al. measured; most keep their own rhythm, some seek the beat: slower, their phases pulled toward
their nearest fellow seekers' and the hall's, so a rhythm emerges in the wash as Neda et al. found;
the key sets the crowd's size), `gunshot` (the bullet's N-wave, then the muzzle blast as Friedlander's
wave and its ground reflection; lower keys a larger calibre).

### Synth leads (analog circuits)

`-inst lead-saw` is GM's sawtooth lead (82), built from a circuit rather than a stored waveform
(`research/synth_leads.md`; the reference simulations in `tools/circuit.py` need ngspice).

The same core, with a sound circuit of its own between the core and the filter, makes the other
synths:

| Family | Instruments |
|---|---|
| Leads (GM 81–88) | `lead-square`, `lead-saw`, `lead-calliope`, `lead-chiff`, `lead-charang`, `lead-voice`, `lead-fifths`, `lead-bass+lead` |
| Synth bass | `synth-bass-1`, `synth-bass-2` |
| Synth strings | `synth-strings-1`, `synth-strings-2` |
| Synth voice | `synth-voice` |
| Synth brass | `synth-brass-1`, `synth-brass-2` |
| Percussion | `melodic-tom`, `synth-drum` |
| Pads (GM 89–96) | `pad-new-age`, `pad-warm`, `pad-polysynth`, `pad-choir`, `pad-bowed`, `pad-metallic`, `pad-halo`, `pad-sweep` |
| Effects | `fx-crystal` (GM 99) |

Each pad keeps the core, its vibrato and the ladder, and adds a part of its own:

| Pad | Its circuit |
|---|---|
| `pad-new-age` | a bell on each attack: the core's sine ring-modulated by a second core 2.76× above, dying in ~1.6 s |
| `pad-warm` | two cores 8 c apart into a saturating output transformer: low notes compress and thicken |
| `pad-polysynth` | a swept pulse and the octave below through an RC high-pass before the ladder, and the chorus |
| `pad-choir` | not a circuit: three singers per key, each a vocal tract as in puhuja (a transmission line in puhuja's vowel shapes) on a glottis at the note's pitch. They sing aah, eeh, iih, ooh or uuh: K2 moves the vowel from uuh (left) through ooh, aah (middle) and eeh to iih (right), K3 is their breath |
| `pad-bowed` | a slip noise once a period (the bow letting go) and the formant bank at a wooden body's resonances |
| `pad-metallic` | four metal pipes hanging from their tops, joined there, tuned to 1×, 1.2× and 2.5× the note, and the highest one to the key's own ratio between 1 and 3 (mostly a prime over 16, sometimes over 12 or 10: 17/16, 19/16, 29/16, 37/16, 17/12 …). The upper three are quieter: −9, −12 and −12 dB under the note’s pipe. Each rings in its own bending modes (1, 2.76, 5.40×). On each attack two pipes, chosen at random, are struck by soft felt hammers at random places along them and ring free; the other two are bowed (stick-slip; they swell in over about a second), and each bow changes direction every 5–9 s. The pipes give sound to the room and take some back (no more than ~90 % of what they gave), so the room, other instruments and other pipes set them ringing. Two microphones at different heights give left and right; the ramp through the ladder is a quiet layer under them |
| `pad-halo` | soft source and pitched breath through a cascade of five formant resonators, each feeding the next and wandering slowly. Each key has its own formants, and over the keys each spreads evenly across its range in human speech (F1 250–850, F2 800–2300, F3 2000–3200, F4 3000–4200, F5 3800–5000 Hz; a hard key a little higher); their widths (Q) are each key's own, vary a little on every key press, and narrow with velocity (a hard key rings sharper); what each adds is heard from its own place, different on each key (F1 near the middle, the others anywhere left to right). A slow compressor on each resonator (it integrates over 1.5 s), governed by the main oscillator's level, lifts what it adds gently towards that level (1.5:1, within ±6 dB). Each compressor also hears the whole sound, late (each its own delay, 0.2–0.6 s), and turns down when the whole was louder than the source: a slow loop that never quite settles, so the formants take turns (each swinging 2–3 dB, the whole breathing about every 3 s). The room's sound reaches back into all of them, about 25 dB under the sound; it cannot ring by itself, since without the source the compressor turns the resonators down. Then the chorus and a regenerating bucket-brigade echo after the amplifier. Live, its vibrato starts low (K6 at 17/127; the others start at ~42/127) |
| `fx-crystal` | not a circuit: a glass plate of 16 × 16 points whose edges are joined into a torus, its mass varying along x. Two felt hammers strike it at random places on each attack, and a bow keeps it singing (it swells in like a pad and changes direction every 5–9 s). Its 64 lowest modes come from `tools/glass.py`; they are inharmonic (1, 1.022, 1.164, 2.00, 2.34, 3.74 …), the first is on the note. Held keys ring, released ones are damped over ~1.5 s. It gives sound to the room and takes some back (at most ~90 % of what it gave), from the room, other instruments and other crystals. Left is the difference of two random pixels, right of two others (per voice; the note in the middle, the other modes apart) |
| `pad-sweep` | two detuned ramps, a filter opening over seconds, and a six-stage JFET phaser swept at 0.12 Hz |

Lead-saw's circuit:

- **The core:** an expo transistor charges a capacitor through an op-amp integrator; a
  comparator with hysteresis resets it through a MOSFET. In real time it is exact between resets,
  each reset found between samples, with its dead time, and band-limited with a polyBLEP.
- **Its parasitics:** emitter resistance (the top a little flat) and leakage (the bottom a little
  sharp, more so when warm).
- **Each voice its own:** parts and temperature, which drifts slowly. It tunes itself at power-on,
  per octave, as polyphonic analog synths do. The ensemble's pajupilli intonation trims each voice's
  control voltage.
- **Then:** a 4-pole transistor ladder (Huovilainen's nonlinear model), an OTA amplifier and RC
  envelopes.
- **The voices share a board.** Each one's output runs past the others' timing nodes, the
  sounding voices pull the supply rail and lift the shared ground, and playing warms the board.
  Each voice has its own 1/f noise, ladder offsets (even harmonics), amplifier thump and bleed,
  and mains hum. `PJ_ANALOG` scales all of this (0: ideal, 1: as sized, more: exaggerated).
- **Controls:**
  - the knobs: dynamics (level and cutoff), brightness (cutoff), air (resonance), vibrato,
    hardness (attack) and release;
  - the joystick: ±2 semitones;
  - overlapping keys slur, a softer overlapped key glides, and a harder one retriggers the
    envelopes.

## MIDI files

    ./pajupilli -play song.mid               # play it (rendered on a thread, heard a second behind)
    ./pajupilli -play song.mid -o song.wav   # or write it
    ./pajupilli -play song.mid -room church  # in another room
    ./pajupilli -reinstrument grand-piano=organ -play song.mid   # the piano's part on the church organ
    ./pajupilli -reinstrument piano=hammond,*=organ -play song.mid

`-transpose N` with `-play` moves every note but the drums N semitones (12: an octave up, -12 down)
as the file is read, so the organ's and the Hammond's plans are made for the transposed piece.

`-reinstrument FROM=TO[,FROM=TO]` plays a channel on another instrument: FROM is the instrument its
program picks (`grand-piano`, `electric-grand`), its family (`piano`) or `*` (every channel but the
drums); the first rule that matches wins; `organ` is the church organ by its plan (pedal, manuals,
registration, articulation). A channel remapped onto any Hammond (`hammond`, `drawbar-organ`, ...) joins the
file's Hammond, one instrument and one plan.

The drum kit, live (`src/drumkit.h`): the same drum keys from the keyboard.

    ./pajupilli -live -inst drums                    # the kit on the keys and the pads
    ./pajupilli -live -inst grand-piano -pads drums  # the piano on the keys, the kit on the pads

The MPK's pads send General MIDI's drum keys on channel 10 as they come (bank A 36..43: bass
drum, side stick, snare, clap, snare 2, low tom, closed hi-hat, tom; bank B 44..51: pedal
hi-hat, tom, open hi-hat, tom, tom, crash, tom, ride); with `-pads drums` they play the kit
instead of the breath and the flutter. With `-inst drums` the keys are drum keys too, the
MPK's lowest C (48) the bass drum (36; `-transpose N` moves them, the octave buttons reach the
rest). Velocity is the stroke. At the start it lists which key plays what, and which are
silent: the cymbals and hi-hats are not built yet (`research/cymbal_plan.md`), so until then
five of the sixteen pads make no sound. About 12 % of a core, busy.

The jazz bass drum (`-inst jazz-kick`, `jazz-kick-20`; `src/kick.cc`, `research/kick.md`): the
bebop kick, 18" × 14" (or 20"), both heads on, no port, tuned up to a note (70 Hz; the 20": 60
Hz), a felt beater. The two heads share the shell's air (the note is their in-phase pair), a
hard stroke stretches them (a bomb starts most of a semitone sharp; a feathered one not at
all), and the beater is thrown from where the head is now, so a stroke into a ringing drum adds
to it or takes from it by its phase; it springs off and stays off (one key, one stroke). Keys
35 (lower on the head, more ring) and 36; velocity is the beater's speed (feathered quarters at
20–70, about 30 dB from 16 to 127). The hold (CC 64; `PJ_KICK_BURY=1`: always) buries the
beater: a short, higher, dead thud.

    ./pajupilli -inst jazz-kick -seq "on 36 44 wait 0.5 on 36 40 wait 0.5 on 36 105 wait 1"

The cymbals (`-inst ride-cymbal`, `ride-cymbal-2`, `crash-cymbal`, `crash-cymbal-2`,
`splash-cymbal`, `china-cymbal`; `src/cymbal.cc`, `research/cymbals_built.md`): a 22" thin jazz
ride, a 20" drier ride, 16" and 18" thin crashes, a 10" splash, an 18" china. Each is a free
bronze plate as its modes (to 16 kHz: ~1600 on the 22"), living through the whole song, so ride
strokes pile up into the wash and each stroke meets the plate where it is; the cup rings as its
own few modes when struck inside its rim; the cymbal rocks on its stand. A stick (the tip on the
bow, the shoulder on the edge) or a brush (the hand, grip, fan and wire ends of `src/brush.cc`)
strikes it; a hand on the edge chokes it. Keys are General MIDI's: 51 and 59 the bow (the place
drifting over the bow by phrases and wandering a little every stroke), 53 the bell, 49/57/55/52
a crash; the same key 24 higher is the stroke with a brush (`PJ_CYM_BRUSH=1`: all). On the ride
a brush stroke goes continuously from the slowed tip (light: a few wire ends, off at once,
bright) to the thrown fan (leaned on), by velocity and, in a song, by the beat: 1 and 3 leaned
on, 2 and 4 light (the bar learnt from the ride's own pattern: 2 and 4 are where its skip notes lead). The breath (channel pressure) grabs the edge (`PJ_CYM_NOTEOFF=1`: a note-off
does). Linear only: a hard crash is bright at once but does not bloom (no nonlinear cascade
yet). About 5–7 % of a core each, busy.

    ./pajupilli -inst ride-cymbal -seq "on 51 80 wait 0.3 on 51 90 wait 0.2 on 51 60 wait 0.1 on 51 80 wait 2"

The brush swirled on a cymbal (key 84 held on any cymbal; `research/jazz_drums.md` 5.7): the
fan lies on the plate (a quarter of its wire ends, `PJ_CYM_SWIRLFAN`) and is drawn round in long
loops along the lathing, a turn every two beats. The wire ends chatter on the bronze's grain and
ride the lathing's concentric ridges (`PJ_CYM_GROOVE`, `PJ_CYM_RIDGE`: drawn along a groove only
the grain, across them a buzz). The loops' middle wanders between the edge and the bell; the plate
is thicker towards the bell (`PJ_CYM_THICK`), stiffer under the ends, so they spring off sooner:
brighter and glassier there (the cup's own modes join in inside its rim), lower and washier at the
edge (centroid ~8.7 kHz near the bell, ~4.8 kHz at the edge). The brush's press damps the plate
while it lies there (`PJ_CYM_PRESS`, a loss per N): a light swirl takes ~4 dB off a ringing ride
in the first second, a heavy one ~13 dB. Velocity is how hard it is pressed (`PJ_CYM_SWIRLLOAD`,
N at 127); `PJ_CYM_SWIRLDB` its level against the brush strokes; `PJ_CYM_SWIRLAT` holds the loops
at one place (a share of the radius). A stroke while it is held takes the hand for a moment.
~1.6 µs a sample while swirling (~8 % of a core, the plate's own included).

In `-play`, with the brushed snare (a Brush kit, or a piano and an upright bass), the drummer now
and then takes the right hand to a cymbal and swirls there for two to eight bars while the left
hand goes on circling on the snare (`drumkit.h` `planCymbalSwirls`): in quiet, sparse passages and
ballads (low velocity, few notes a beat), ending on a phrase's or a section's end (where the band
changes, or into a crash), one in a short piece, one more each 40 bars, three at most, at least eight bars
apart. It swirls on the ride, or a crash if the file plays the ride then, never on a cymbal the
file strikes then (it lifts before a stroke on it). The choice is seeded from the file's notes:
the same file plays the same way every time. Each swirl is listed as the file is read.
`PJ_SWIRL=0`: none; `PJ_SWIRL_MAX`: at most this many.

    ./pajupilli -inst ride-cymbal -seq "on 84 60 wait 6 off 84"

The shakers and jingles (`-inst maracas`, `shaker`, `cabasa`, `tambourine`, `jingle-bell`;
`src/shaker.cc`, `research/shakers.md`), for General MIDI's 70, 69, 54 and GM2's 82, 83: many
small hard things thrown by the hand, each collision a click ringing the body's few broad
resonances (PhISEM's resonances, but the things themselves simulated, cheaply). In a maraca 60
seeds fly on when the hand stops and hit the far wall together (the chick, at the key's time),
then rattle off each other and settle in ~0.1 s; two maracas take the strokes in turn. The
shaker's tube alternates push and pull. The tambourine's six pairs of jingles slam against their
slots and each other, each disc ringing its own modes (3-8 kHz), over the frame's knock; the
jingle bell's balls ring their bells. Every stroke finds the jingles otherwise arranged: the
frame held at its own tilt and rocking on the blow (one side's pairs thrown harder), the kick
reaching the pairs over a few ms, each disc tilted on its pin (its bounce and free travel its
own), and every collision striking a disc's rim somewhere else (its modes mixed by their shapes
there). The cabasa is a scraper: the bead chains over the ribbed
cylinder, dense clicks at the ridge rate, as long as its key is held. Velocity chooses how it
is played: the softest turned on one way (the chains keep sliding, no reversal; to 45), then
twisted back and forth (each reversal throws the chains over; to 80), then shaken in the air
(the chains thrown, a short scrape, in a 1-2-3-4: 1 strongest, 3 a little under it, 2 under 3,
4 the weakest, from the song's beat or, live, by counting; to 105), and the hardest struck
against the other hand (the palm's thump on the handle and cylinder, every chain thrown, the
beads sliding); `PJ_CABASA_TURN`, `_TWIST`, `_SHAKE` move the borders. One object a song, so a
stroke meets what is still moving; no two strokes alike; velocity is the hand's speed, about
30 dB from 16 to 127. The maracas are played in their ways by velocity: the softest from the
chest (the seeds start to slide as the hand moves off, a small rising woosh, then the slap; in
a song the player gets these notes 0.12 s early, `PJ_MARACA_LEAD`, and the slap falls on the
note; live it is a soft slap), the plain slap, two slaps in one movement (out and back, the
second weaker, a sixteenth later; 100-110), and a flick (the wrist snapped: the seeds thrown in
a tight bunch, a firmer slap; 111 up); a key held past 3/4 of a beat (0.4 s live) turns the
maraca in loops, the seeds rolling round the wall, until it is let go (`PJ_MARACA_CHEST`,
`_SLAP`, `_DOUBLE` move the borders). The maracas and shaker sit 5-6 dB under the common level (a chick's peak
is ~30 dB over its loudness). Under 0.5 % of a core each.

    ./pajupilli -inst maracas -seq "on 70 100 wait 0.23 on 70 55 wait 0.23 on 70 75 wait 0.23 on 70 55 wait 1"

The snare with sticks (`-inst stick-snare`; `src/sticksnare.cc`, `research/stick_snare.md`): the
brushed snare's drum (its two heads on their air, the wires on the snare head, its microphones)
played with a pair of sticks. A stick is a 9 g tip on a stiff contact, thrown from just over the
head where it is now and found against the head's give each sample (3.8-4 ms on the head; the
wires answer, a short buzz under a ghost note, a longer rattle under an accent); the stick's own
three bending modes and the contact's click above the head's modes. Keys: 38 a stroke (between
the middle and a third out), 40 halfway out, 42 near the edge, 37 the cross-stick (the snare's
own), 39/41 the open rim shot (a heavier stroke near the rim, the shaft striking the hoop at once:
its nine ring modes and the stick's, each at its own phase; about 6 dB over a stroke and
brighter), 36 the buzz roll for as long as the key is held (the hands in turn, 6-9 strokes a
second each, each pressed in and bouncing 3-5 times, sooner and weaker: 65-90 impacts a second).
Velocity ~^1.75 gives about 31 dB from 16 to 127; it sits 2 dB under the common level (an accent
peaks near 1.5). About 7 % of a core (the drum's wires the most of it).

    ./pajupilli -inst stick-snare -seq "on 38 30 wait 0.25 on 38 100 wait 0.25 on 39 110 wait 0.4 on 36 80 wait 1.2 off 36 wait 0.5"

The small sounds below the drum map's 35 (`-inst perc-extras`; `src/percextras.cc`,
`research/gm2_extras.md`), by GS/GM2's keys: 26 finger snap, 27 High Q, 28 slap (the whip's two
boards), 29/30 scratch push and pull, 31 sticks, 32 square click, 33/34 the metronome's click and
bell; and Yamaha XG's own on free keys: 20 click noise, 21 castanets, 23/24 sequencer clicks. The
struck ones are physical: two hickory sticks' free-free bending modes (330, 910, 1780, 2950 Hz),
damped by the hands, under the click's own crack; the whip's air burst and boards; the finger's
blow on the palm and the cupped hand's air; the castanets' shells and the air between them. The
electronic ones and the turntable's scratch (the record read at a swept speed) are simple
synthesis. Velocity gives about 30 dB from 16 to 127; they sit 12 dB under the common level (small
sounds). The user's files with these keys are Yamaha XG sequences, where 25-28 are brushes, 31 a
snare, 33 a bass drum and 34 an open rim shot (research/gm2_extras.md §1, §6).

    ./pajupilli -inst perc-extras -seq "on 31 70 wait 0.5 on 31 70 wait 0.5 on 33 60 wait 0.5 on 34 60 wait 1"

The jazz toms (`-inst tom-10`, `tom-12`, `tom-13`, `tom-14`, `tom-16`, `tom-16-low`; `src/tom.cc`,
`research/toms.md`): one two-headed drum for each of General MIDI's six tom keys (50, 48, 47, 45,
43, 41), tuned high and open, both heads at the same lug pitch, on 196, 165, 147, 110, 92.5 and
73.4 Hz (the tension is solved for when the drum is made, so the note stays put as the heads'
other settings change). The air in the shell couples the heads (the note is their in-phase
pair, about half the batter's (1,1)); a hard stick stroke stretches them (the 14" starts about
half a semitone sharp at velocity 127, the low 16" more), a feathered or brushed one keeps its
pitch. A stick is thrown from where the head is now and its contact solved as the head gives
under it; it rebounds and is gone. Keys 24 up (65–74) play the brush: the pull-out stroke, the
hand turning up as the wires land, off the head in about 4 ms, the drum ringing on; below
velocity 24 (`PJ_TOM_DRAGVEL`) the brush is set down and drawn a few cm instead, a short hiss
with the drum's pitch in it. About 30 dB from velocity 16 to 127; brushes 6–13 dB under sticks.
About 1.6 % of a core per ringing drum.

    ./pajupilli -inst tom-14 -seq "on 45 60 wait 0.6 on 45 127 wait 1 on 69 90 wait 1 on 69 12 wait 1"

The hand drums (`-inst conga`, `bongo`, `timbale`; `src/handdrum.cc`, `research/hand_drums.md`):
each a pair, General MIDI's 62 (mute hi conga), 63 (open hi conga), 64 (low conga), 60, 61
(bongos), 65, 66 (timbales). One head over an open-bottomed shell: rawhide on the congas (C4 and
G3) and bongos (C5, G4), thin plastic on the timbales (E4, B3), each tuned on its (0,1) mode as
the air loads it. The conga's 76 cm barrel is a pipe closed by the head: its quarter waves (near
100, 310 and 520 Hz) are driven by the volume the head sweeps and press back on it, and its
open foot radiates (the "bass" under the open tone); the tension is solved so that the note
stays on pitch with the pipe pulling at it. The hand is a soft, heavy, lossy mallet (fingers
across the rim, flesh on bone), thrown from where the head is now and solved against the
head's give; the open tone bounces off, the closed tone (62) stays pressed 0.2 s and damps the
head where it lies (16 dB shorter at 0.1 s). No two strokes are played the same: the hand
lands a little elsewhere, more open (flat, wide, softer) or more closed (cupped fingers,
narrower, harder); on open tones the fingertips sometimes stay a moment at the edge (a touch
of damping), and now and then the hand drops further in, heavier, and is lifted at once, a
rebound off the head for a bigger, rounder sound (more often on accents); a closed tone presses
harder or longer. The timbales are struck with a stick near the
edge, and their steel shells ring a little through the rim. About 30 dB from velocity 16 to
127; 1.5 % of a core.

    ./pajupilli -inst conga -seq "on 62 70 wait 0.33 on 62 50 wait 0.17 on 63 95 wait 0.17 on 62 70 wait 0.33 on 64 100 wait 0.17 on 64 90 wait 1"

The triangle and the cowbell (`-inst triangle`, `cowbell`; `src/metalperc.cc`,
`research/triangle_cowbell.md`): General MIDI's 80 (mute triangle), 81 (open) and 56. The
triangle is a 10 mm steel rod, 0.57 m (an 8" triangle), its bending modes as the straight free
bar's (Rossing): one family in its plane, one across it that the corners stiffen (8-18 % higher,
as on a recorded triangle), and the rod's longitudinal modes; struck by a steel beater (a contact of
tens of us) on the bottom side, a little elsewhere each time. The low modes barely radiate: the
shimmer from 1 to 15 kHz, T60 about 3 s. A key 80 (or the hold) is the hand clasping it (T60
0.35 s), and it chokes a ringing triangle; 81 lets go. The cowbell: the partials and decays of a
recorded one (472 Hz with its rough neighbours, the walls at 2-3.7 kHz), a hickory shaft on the
mouth's edge. (The agogo's partials, `-inst agogo`, now as a recorded pair's too: 3.73 and 5.12 x
the strongest.) Velocity^1.65: about 30 dB from 16 to 127; 0.2 % of a core each.

    ./pajupilli -inst triangle -seq "on 80 70 wait 0.12 on 80 60 wait 0.12 on 81 95 wait 0.25 on 80 70 wait 0.12 on 81 90 wait 1.5"

The scraped, rubbed, rattled and blown ones (`-inst guiro`, `cuica`, `vibraslap`, `pea-whistle`;
`src/scrapers.cc`, `research/scrapers_friction.md`): General MIDI's 73/74 (short, long guiro),
78/79 (mute, open cuica), 58 (vibraslap), 71/72 (short, long whistle). The guiro's stick tip
(2 g, held by a damping hand) is dragged over 20 cm of notches, 5 mm apart at the wide bottom
to 3 mm at the narrow top: it climbs each and drops onto the next, a click train at the
stroke's speed over the spacing, into the gourd's cavity (330 Hz) and shell, and into each
notch's own ring (the wall under it: ~0.9 kHz at the bottom to ~2.4 kHz at the top, each a
little apart), so a stroke glides in pitch. Strokes alternate up and down, in pairs over the
lower notches, then the upper ones (low-low, high-high); soft ones (to 50, `PJ_GUIRO_JOIN`)
are joined (the stick stays on, the next goes on from where it is), harder ones separate: the
stick comes back down out of the air and lands with its own blow (0.1-0.6 m/s) before it
scrapes. A light press skims the ridge tops. The cuica's stick, tied to the head's centre, is rubbed by a damp cloth: stick-slip
against the head's modes there (goat skin, the stick's mass on it; the grip grows with the
cloth's speed, as a bow's force must), the thumb gliding the open note up about five semitones,
the mute an octave up and damped. The vibraslap's wire swings the box (22 Hz, dying in about a
second and a half) and its six loose pins rattle against the walls (trimmed 6 dB under the
common level: its rattle is all peaks). The pea whistle is a self-sustained cavity (3.3 kHz)
the jet drives and its turbulence seeds, the pea trilling it at 25-40 Hz. 23-30 dB from
velocity 16 to 127; under 0.5 % of a core.

    ./pajupilli -inst guiro -seq "on 74 100 wait 0.32 on 73 90 wait 0.16 on 73 90 wait 0.16 on 74 100 wait 0.5"

The hand clap, claves, wood blocks, surdo and bell tree (`-inst hand-clap`, `claves`,
`wood-blocks`, `surdo`, `bell-tree`; `src/clap.cc`, `woodperc.cc`, `surdo.cc`, `belltree.cc`,
`research/clap_claves_surdo.md`): General MIDI's 39 (in the Jazz and Standard kits), 75, 76/77,
GM2's 86/87 (XG's 13/14) and 84.
- The clap: the air cupped between the palms rings as a Helmholtz resonator as the hands close
  (0.75 kHz cupped to 2.2 kHz flat; the soft tissue damps it within a few cycles), each clap the
  fingers' and then the palms' contact, with the escaping air's hiss and the palms' thud; four
  people at once spread over 20 ms, every stroke drawn afresh.
- The claves: two rosewood bars, free-free: the held one at 2.45 kHz, the cupped hand under it
  helping its first mode out; the striker 6 % lower and short.
- The wood blocks: their partials as measured on recorded ones (746, 502 Hz). They are struck
  with a stick's tip on top over the cavity, within ±10 % of the block's width and length of the
  same place, each mode by its shape there: the main one alike every time, its neighbours a few
  dB different stroke to stroke.
- The surdo: a 22" × 55 cm samba drum, built as the toms are, at 62 Hz, struck with a felt
  mallet; the mute (86) is the other hand flat on the head.
- The bell tree: sixteen brass bells from 11 down to 4 kHz, the beater drawn down the stack, a
  harder key a quicker sweep.

26-32 dB from velocity 16 to 127; under 3 % of a core.

    ./pajupilli -inst wood-blocks -seq "on 76 90 wait 0.25 on 76 70 wait 0.25 on 77 90 wait 0.25 on 76 80 wait 0.5"

Each channel's General MIDI program plays on its instrument, or, where that is not built yet,
on the nearest one that is: the harpsichord and clavinet fall back to the upright and the
CP-80, guitars to the koto and banjo, cello to the contrabass, string
sections to the synth strings, choirs to the pads. Channel 10's drums go to the percussion
built (bass drum: the jazz kick; snare and side stick: the brushed snare; hi-hats: the hi-hat; toms, bongos, congas: melodic tom; cowbell,
agogo; claves, wood blocks); cymbals and hi-hats are not built and stay silent. Each channel
is a pool of its instrument as big as the most notes sounding at once, counting 1.5 s of tail
after each release (`PJ_PLAY_TAIL`; at most 32 voices, `PJ_PLAY_VOICES`; a piano: one voice). A
new note takes the voice released longest ago, so tails ring out; a wind's line keeps to its
player (the voice released last, if its note was within a minor third);
the sustain pedal, volume and expression are followed. Struck and plucked instruments (the
modal percussion, the drum synths) take a key's velocity as the stroke's speed, ~velocity^1.6
(^1.2 for hard bars and the taiko, ^1 for bells): about 30 dB from velocity 16 to 127, as a
General MIDI player's curve (`tools/velcurve.py`). The others' loudness comes mostly from the
breath (live: the dynamics knob and the pad), so a file's velocity also sets each note's level:
the median curve of TiMidity++ with FluidR3, 33 log10(v/127) dB, less what the instrument's
physics already gives, measured per instrument (`tools/velcurve.py -emit`,
`src/velgain_data.h`; 0 dB at velocity 90, at most +6 dB; `PJ_NOVELGAIN` turns it off). A
wind or a synth gets it as its voice's gain (a slur moves there in 60 ms); one voice that
sounds every key (the CP-80, the plucks) gets the stroke that sounds that loud instead. A
file played hot throughout (its median velocity over 96: a sequencer's default) is scaled
down together until the median is 96 (`PJ_PLAY_VELMEDIAN`). Playing starts once 10 s are rendered
(`PJ_PLAY_LEAD`, s), and should the rendering fall behind (a song heavier than real time), it
waits for 10 s again, never longer. While playing it prints the time heard every 5 s (`0:05`,
`0:10` ...; what the audio device still holds taken off), so a muddy or otherwise bad moment
can be named (`PJ_PLAY_TICK`, s; 0 for none). A file is scaled to
its peak, a stream passes a soft knee over 0.9. Schubert's piano (8 min) renders at about
3× real time. The parts render side by side on up to 16 threads (`PJ_PLAY_THREADS`; the synth
circuits together, as they share a board) and mix in their usual order, so the result is the
same as on one thread, bit for bit: *Good Bait* (a trio with the brushed kit, 6 min) renders in
124 s instead of 278 s. The heaviest part (most often the piano's pool of voices) still sets
the pace.

## Offline

    ./pajupilli -note 69 -o a4.wav   # one note, prints its pitch
    ./pajupilli -demo -o demo.wav    # a scale and a phrase
    ./pajupilli -cal                 # what the player learns at startup (cached in ~/.cache/pajupilli)
    ./pajupilli -imp 69              # passive resonances of a fingering (PJ_IMPHOLES: any fingering)
    ./pajupilli -slur 71 72 -dyn 0.2 # slur into a note; pitch and level wobble
    ./pajupilli -ramp 60             # register map: the breath swept x0.25 .. x3 and back
    tools/velcurve.py drums          # velocity curves (A-weighted) of every GM drum key, against TiMidity++
    tools/velcurve.py programs       # ... of every GM program
    tools/velcurve.py -emit          # the -play velocity gains (src/velgain_data.h)
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
`src/player.cc`), e.g. `PJ_BREATH=0.03 ./pajupilli -live`.

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
- Other woodwinds (`./pajupilli -inst NAME -cal` per note; `make test` checks the shares):

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

  | trumpet | 31 / 31 within 1 c after calibration; held notes steady | soft notes stop near mf (softer, they fall into a flat regime); low notes ~20 dB softer than high |
  | muted-trumpet | 31 / 31 within 1 c | (as the trumpet) |
  | trombone | 42 / 42 within 2 c, wide dynamics on most | the shaded high notes (A♭4–E5) hold a narrower range |
  | tuba | 38 / 38, 3 over 10 c (A♭1–B♭1 −17..−47 c) | E1–E♭2 hold only near mf |
  | horn | 37 / 37 (C3–F5 within ~10 c on `-scale`; held notes steady within 2 c and a few % in level; notes speak in 30–60 ms; legato scale ~10 ms per change more than 6 dB down) | F#2–G#2 22–28 c off; C#4, D4 ~15 c sharp; many notes hold only a narrow dynamic range (loudest often 0.5); a slur down to G3 from C4 or D4 lands ~55 c flat; slurs crossing partials with valves in the low register (F3→G3, A3↔Bb3) dip for 20–65 ms |

  The designs' passive resonances are within a few cents of their notes (rms 0–10 c on the
  core fingerings; the saxophones and oboe 16–48 c, their octaves stretched: the mouthpiece's
  volume is not a lumped volume at high frequencies); the players' lips take up the rest.
  whistle tones.
