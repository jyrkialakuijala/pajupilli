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

With a synth lead (`-inst lead-saw`), the first three knobs work the filter instead:

| Knob | Synth lead |
|---|---|
| Knob 1 (CC70) | Formant spread: each key its own fixed filter offset, within ±12 semitones at full turn (the level is knob 8) |
| Knob 2 (CC71) | Cutoff: two octaves under the note to five over it (up to ~19 kHz); an octave over at rest |
| Knob 3 (CC72) | Resonance, which in a ladder is the Q: about 0.5 at 0, 5 at the middle, 19 at 0.85, self-oscillating from about 0.92. Live prints the Q as it turns |

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
not drift. The highest note is tuned less: it is pulled to equal temperament, the more the
less clearly harmonic its sound (shimmer, vibrato, an inharmonic spectrum: the pads' bell,
pipes, glass; `PJ_TOPPULL`), and the notes under it find the chord's justness. Such sounds
are also corrected at most 11 c (the pads and effects always; others by how unclear: a clean
tone keeps the full ±35 c); a note at its limit is held there and the others fit it. Each flute gets
there with its lips, by ear. It prints `tune: C4 +1 E4 -12 G4 +3`.
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

### Ethnic and percussive (modes)

The GM ethnic (105–112) and percussive (113–120) families, each researched first
(`research/sitar_shamisen.md`, `banjo_koto.md`, `kalimba_fiddle.md`, `steelpan_taiko.md`,
`bells_blocks.md`), on one modal engine (`src/modal.*`, `src/modalinst.*`): `-inst sitar`, `banjo`,
`shamisen`, `koto`, `kalimba`, `fiddle`, `tinkle-bell`, `agogo`, `steel-drums`, `woodblock`, `taiko`,
`reverse-cymbal` (the bagpipe and shehnai are woodwinds; the melodic tom and synth drum are circuits).
One voice is one instrument, its player giving the keys to strings, tines, note areas or bells.

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
  per octave, as polyphonic analog synths do. The ensemble's just intonation trims each voice's
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

  | trumpet | 31 / 31 within 1 c after calibration; held notes steady | soft notes stop near mf (softer, they fall into a flat regime); low notes ~20 dB softer than high |
  | muted-trumpet | 31 / 31 within 1 c | (as the trumpet) |
  | trombone | 42 / 42 within 2 c, wide dynamics on most | the shaded high notes (A♭4–E5) hold a narrower range |
  | tuba | 38 / 38, 3 over 10 c (A♭1–B♭1 −17..−47 c) | E1–E♭2 hold only near mf |
  | horn | 37 / 37 (C3–F5 within ~10 c on `-scale`; held notes steady within 2 c and a few % in level; notes speak in 30–60 ms; legato scale ~10 ms per change more than 6 dB down) | F#2–G#2 22–28 c off; C#4, D4 ~15 c sharp; many notes hold only a narrow dynamic range (loudest often 0.5); a slur down to G3 from C4 or D4 lands ~55 c flat; slurs crossing partials with valves in the low register (F3→G3, A3↔Bb3) dip for 20–65 ms |

  The designs' passive resonances are within a few cents of their notes (rms 0–10 c on the
  core fingerings; the saxophones and oboe 16–48 c, their octaves stretched: the mouthpiece's
  volume is not a lumped volume at high frequencies); the players' lips take up the rest.
  whistle tones.
