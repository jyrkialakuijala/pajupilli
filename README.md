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
    ./pajupilli -live -room church               # dry, studio, hall (default), concert, church
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
- GM 62 "Brass Section" is a section of these players, voiced: see the next section.

### Brass section

`-inst brass-section` (GM 62; `src/brasssect.h`, `src/brasssect.cc`; `research/brass_section.md`)
is a brass section of the physical players above, voiced as an arranger would voice it. Nothing
is sampled or chorused: each seat is its own trumpet, trombone, horn or tuba with its own player.

**The make-ups** (research 4, 11.1). `-play` reads the part once with the rest of the song
(research 4.3: its note lengths, how many keys at once, how wide, how low; the director's
style; which instruments play beside it; swing) and picks the section; `PJ_BRASS_MODE=pop |
bigband | orchestral` forces one. `-v` (or `PJ_BRASS_SHOW=1`) prints the plan and every voicing.

| Section | Seats, top to bottom | Chosen for |
|---|---|---|
| pop / soul / funk | trumpet ×3, trombone ×2 | the default: short notes, a band with drums and electric bass |
| big band | trumpet ×4, trombone ×4 (the 4th a bass trombone) | 4–5-key close chords, or swing with saxes, piano and upright bass |
| orchestral | trumpet ×2, horn ×4, trombone ×3, tuba | a classical style, long notes and wide chords, strings or timpani, no drums |

**The voicing rules** (`BrassVoicer`; `-brassvoice "SPEC"` prints them without sound):

- The keys are the harmony, and the top key is the lead: trumpet 1 plays it whenever it can.
- Written chords (two keys or more) are dealt, never re-harmonised: every key sounded, none
  moved by an octave. Spare players double the lead first, then the bottom, never a third under
  C4; at most about P/K players a key, so a chord's notes are shared out. The trumpets take the
  top keys, the horns the middle, the trombones and tuba the bottom; the tuba only plays the
  bottom key, and only from F3 down. The big band's trombones double a close chord of two to four
  keys an octave down, where that is no mud.
- A lone key is a unison of those at ease on it (pop: trumpets from C4 up, trombones to B♭4;
  orchestral: trumpets from D4, horns C3–D5, trombones to D4, tuba to D3). In a unison line
  (a run of single keys) at G4 and above, the low brass doubles it an octave down: the soul
  line, the fanfare. Not if the part writes its own octaves.
- A sparse part (single keys, a third or sixth) is harmonised only where the song says what the
  chord is (the other channels' notes over the onset, the bass weighted) and the part fits it
  (75 % of its notes chord tones or the tensions an arranger keeps: 9, 11 on minor, 13). Pop:
  only held notes (0.3 s or longer) bloom into three-part close harmony under the lead (on
  seventh chords the 3, 5 and 7, the root left to the bass), the lead doubled an octave down;
  the short notes stay in unison. Big band: 4-way close from a D5 lead up, drop-2 below it
  (a triad as a 6th chord, a major 7th chord as a 6th unless the lead is the 7th; a 9 in the
  lead replaces the root, a 13 the fifth), the trombones on the same an octave down, or on its
  inner parts, or doubling the lead; a passing note between voiced ones moves the whole
  voicing with it (planing). Orchestral parts are never harmonised.
- Practical limits on every voicing it makes: each seat in its range, its comfortable register
  weighed; no close interval under its low interval limit (a minor second under G3, a major
  second or a minor third under F3, a major third under E3, a fourth under B2, a fifth under
  B♭1); no minor second or ninth between any two voices; the least motion from the last
  voicing (a common tone held, a seat moving to the nearest note).

Worked examples (`-brassvoice`):

```
C major triad, written (pop):     keys C4 E4 G4     -> dealt: tp1 G4, tp2 G4, tp3 G4, tb1 E4, tb2 C4
... with the octave on top:       keys C4 E4 G4 C5  -> dealt: tp1 C5, tp2 C5, tp3 G4, tb1 E4, tb2 C4
... orchestral, open:             keys C3 G3 E4 C5  -> tp1 C5, tp2 C5, hn1 C5, hn3 E4, hn2 E4, hn4 E4,
                                                       tb1 G3, tb2 G3, tb3 G3, tuba C3
E5 held over C7 (pop):            3-part close, the lead 8vb: tp1 E5, tp2 E5, tp3 Bb4, tb1 G4, tb2 E4
E5 over C7 (big band):            4-way close, the trombones 8vb: tp1 E5, tp2 C5, tp3 Bb4, tp4 G4,
                                                       tb1 E4, tb2 C4, tb3 Bb3, tb4 G3
Bb4 over C7 (big band, lower):    drop-2, the lead 8vb: tp1-3 Bb4, tp4 E4, tb1 C4, tb2 Bb3, tb3 G3
ii-V-I under F5 F5 E5 (big band): Dm7 F5 D5 C5 A4 / F4 D4 C4 A3; G7 F5 D5 B4 G4 (tb G4 F4);
                                  Cmaj7 E5 C5 A4 G4 (tb G4 E4): each seat moves 1-2 semitones
Unison fanfare (pop):             G4 -> tp1-3 G4, tb1-2 G3;  C5 -> tp1-3 C5, tb1-2 C4
... orchestral:                   C5 -> tp1-2, hn1, hn3 C5; hn2, hn4, tb1-3 C4
```

**The playing** (research 6–8):

- Onsets: the lead on the key; on a stab the others 4 ± 6 ms after it, each seat's habit (±3 ms)
  and its section's moment (±4 ms) on top; a soft long entry 10 ± 15 ms. Played from a file the
  keys come 90 ms early (`PJ_BRASS_LEAD`) and each player starts its instrument's speaking time
  before the beat (the trumpet's ~30 ms, a trombone's 30–55, the horn's up to 85), so the section
  lands together: six C5 stabs land within 14 ms, sd 5 ms, the lead never behind. Releases: a
  stab's within ~5 ms, a long note's over ~25 ms.
- Pitch: each seat spread over ±4 c (`PJ_BRASS_CENTS`; the lips aim there, so the players beat
  as a section does: each harmonic's amplitude moves 5–6 dB at 0.5–2 Hz, as in the GM section
  sample, against 0.3–0.8 dB for one trumpet); a section player lips a forte note up to 2 % of
  its lips' tension (`PJ_BRASS_EAR`; a soloist 0.8 %). Held chords move towards just intervals
  by the pajupilli intonation at half strength (`PJ_BRASS_JI` 0.5: brass players lean to equal
  temperament).
- Each player and instrument its own (`HornPlayer::vary`, `Horn::vary`): the attack's push
  (±15 %) and its decay (±20 %); the cup's volume (±5 %), the lips' Q (±10 %), the brassiness
  (±12 %).
- Vibrato: none under 0.6 s; then the lead a little (half the director's depth, `PJ_BRASS_VIB`
  without one), the other trumpets less and later; horns, trombones and tuba none; none in a
  unison.
- Balance: the lead 1.5 dB on top (`PJ_BRASS_LEAD_DB`); every seat's velocity its own (±4); the
  harmony's added notes softer. A unison key is 2 dB over one trumpet in `-play`
  (`PJ_BRASS_GAIN`), four keys 8.6 dB over one (FluidR3's section: +7 to +9). Velocity is
  brightness: a section player's breath spans 1.6× a soloist's over velocity (`PJ_BRASS_VELDYN`),
  so the 4 kHz octave rises 17 dB against the 1 kHz one from velocity 64 to 112.
- Falls and doits only from the file's pitch bend (General MIDI's ±2 semitones,
  `PJ_BRASS_BEND_RANGE`): each player follows a moment later (3–20 ms) at its own speed, gliding
  down its valves or slide a semitone at a time with the lips bending the rest; the players'
  half-way times lie ~50 ms apart, a smear, not one bend for all. Nothing is invented.
- A CC 11 ride inside a held note (an fp<, a swell) moves the players' breath as well as the
  fader: the brass darkens and brightens with it.
- Seating (in the room): pop, the trumpets left of centre and the trombones right; big band, two
  rows across, the leads in the middle; orchestral, horns left, trumpets centre, trombones and tuba
  right.

Cost (one thread): the pop section 0.66 µs a sample (3.2 % of a core), the orchestral one
1.2 µs (5.9 %). `make test` checks the voicer (no crossing, ranges, the lead on top, every key,
no mud, the motion over a ii-V-I), the dealing, the beating, the tuning, the stabs, the
brightness, the balance, the falls, fuzz and real time.

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

`-inst cello` (GM 43) is the same construction at a cello's size (`research/cello.md`, its
last section): C2 to A5, harmonics to C7, pure-fifth open strings, its own vibrato, the endpin on
a stage floor; a key let go within 100 ms is a pizzicato. `-inst orchestra-hit` (GM 56) scores
each key for the orchestra's sections (`src/orchhit.cc`) and holds as long as the key. With
`-play`, the director (`Director.md`, `-director on|off|auto`) gives a quantized file one
reading for all parts.

`-inst viola` (GM 42) is the cello's construction with a viola's own parts (`research/viola.md`,
its last two sections): C3 to E6, harmonics to E7, on 37 cm strings (the C short and heavy,
nearly a cello string), a violin-type bridge (39 mm, one hill near 2 kHz), and a 41 cm body in
the violin family's outline (`tools/bassbody.py -viola`). Its two main resonances lie between its
open strings, not on them: the air at 226 Hz (2–4 semitones above the open G), the main wood
resonance at 365 Hz (F♯4), each about a semitone wide (Q 19 and 24). The lowest fifth has no body
under it, the viola's veiled colour. The back is twice as stiff as the top, so the main resonance
rocks the bridge as the bow drives it (with the back as soft as the top the notes D♯4–F♯4 lost
their fundamental to a mode at 685 Hz). Held under the chin: no floor. The body steps twice a
sample (a small body's grid rings to 16 kHz); about 19 % of a core (30 % on the grid). The strings and the bow take two steps a sample, the bow reaches higher overtones the harder it presses, the f-holes open from three cells of the air, and a bank of resonances stands for the body above the grid's reach (`Viola.md`, `src/bodybank.h`). A pizzicato, there and on the
cello, is the nail's: velocity sets only how far it pulls the string (`PJ_PIZZ_FLESH`: below this
velocity the fingertip's flesh instead).

`-inst violin` (GM 41) is the viola's construction at a violin's size: G3 to C7, harmonics to A7,
on 32.8 cm strings (gut-core G, D, A and a steel E, in pure fifths from A 440), a violin bridge
(33 mm, its hill near 2.8 kHz) and a 35.6 cm body. The body is fitted to the sound of 17 recorded
violins (`tools/bassbody.py -violin`). Each band's level is the median across them, read from the
harmonics of their notes. The simulation comes within 5 dB of it from 196 Hz to 1.4 kHz: A0 at
268 Hz (Q 27), then 354, 369, 549 and 553 Hz, f-holes 11 cm². The body's grid steps four times a sample;
about 8 % of a core as modes (27 % on the grid). Its notes start in tens of milliseconds, and its stopped notes are tuned to
ring with the open strings, as on the viola (`PJ_RING_TUNE`).

**The body as modes** (`src/bodymodes.h`). The grid (plates, air cells, slugs, plugs, the post,
the losses, each instrument's own build) is linear and fixed once the instrument is made, so it
runs as its modes. At start-up its equations become matrices; the undamped modes come from a
symmetric eigenproblem; the losses, which are not proportional (the plates, the air slugs and the
plugs each lose at their own rate, the wood's and the air's friction act on their own stiffness),
couple modes near each other, and 2 x 2 similarity transforms remove those couplings (the
weaker ones to first order); the grid's substeps become one complex pole a sample per mode, and
the sides' and directions' low-passes partial fractions of them. The modes that add nothing above
-70 dB to any output, from either foot, are dropped: about 190 to 215 remain, run 16 at a time.
Each output, from each foot, is the grid's within -58 dB (each design, a section's seats too), the
resonances at the feet at the same frequencies and Q; a bowed or plucked note differs from the
grid's by -66 to -84 dB, a section's chords by -69 dB, and the bow starts the same. A body costs
about 0.14 us a sample instead of 1.8 (bass, cello), 6 (viola) or 8 us (violin). Finding the
modes takes 25 to 50 ms a body, once for each build (a section's seats share them across its
channels). `PJ_BODY_GRID=1` runs the grid itself (the definition; A/B); `PJ_BODY_TOL` (3e-3) is
the coupling left to first order, `PJ_BODY_PRUNE` (1e-7) a dropped mode's largest share.

**Where each resonance sends its sound** (bass, cello, viola; `src/bassbody.h`, `Room::setAxes`).
A body resonance that moves the whole top out and in, or pumps the air out of both f-holes
together, is heard alike all round. One that moves the two halves of the top against each other,
or draws the air in at one f-hole while it leaves the other, displaces no air on balance: it is
heard to the sides with opposite signs and not straight ahead. So the air inside is in two halves
side by side, each with its own f-hole and free to cross to the other (`PJ_AIR_SIDES=0`: one
column and one plug for both, as before), and besides the whole displaced volume the body gives
its three moments: across it, along it, and top against back. In a room every path to the
listener, direct or off a wall, leaves the instrument in its own direction and takes its own mix
of the four; the reverberant field takes their power. Each overtone of a note excites the
resonances differently, so each goes out in its own directions and reaches the two ears by a
different balance (Jyrki: string instruments mix up their overtone structure for the space they
radiate to, so nothing is heard as a comb filter between the ears). `PJ_DIPOLE` scales the
directional parts (0: none). Limits: the moments stand for the lobes only up to where the body is
as large as the wavelength; and the close parts of the sound (the top by the bridge, the f-hole,
the pickup), a large share on the viola, still go out alike all round.

`-inst strings` and `-inst slow-strings` (GM 49 and 50, String Ensemble 1 and 2;
`src/stringsect.cc`) are a string orchestra of 24 of these instruments: 12 violins, 6 violas,
3 cellos, 3 double basses, seated across the stage, violins left, basses right. A key is its
sounding note (E1 to G7). Each section plays the held keys in its register and divides between
them as a section does; when the keys change, as few players move as will do, and a player
moving slurs in its bow. Nothing is chorused: the section's sound is 24 players, no two alike
(fixed for each seat: its stops a few cents off, its own vibrato rate, depth and delay, its own bow
speed and pressure and place in the bow, and its own instrument, built a little differently, as
recorded violins differ), each starting at its own moment (about 20 ms give or take 15 after the
key, the back desks a little later). Ensemble 1 starts together within 40 ms, firmly,
the bow nearer the bridge; ensemble 2 ("slow strings") comes in player by player over 300 ms
with soft strokes, the bow towards the fingerboard, and leaves over 350 ms. All 24 are computed
all the time in a room: about real time on a 16-core machine (`PJ_THREADS`).

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

### Choir (Choir Aahs)

`-inst choir` (GM 53, Choir Aahs; `src/choir.h`, `src/choir.cc`; `research/choir.md`) is a choir
of 24 singers, each a vocal tract as in puhuja (`src/singer.h`, the pad-choir's singer) on a glottis
at its own pitch. They stand in six parts across the stage, sopranos on the left, basses on the
right:

| part | range | at ease | tract | vowel rows |
|---|---|---|---|---|
| bass | G1–E4 (oktavists below D2) | E2–A3 | 17.8 cm | male |
| baritone | A2–A4 | A2–D4 | 17.3 cm | male |
| tenor | C3–C5 | D3–G4 | 16.8 cm | male |
| alto | F3–F5 | G3–C5 | 15.5 cm | female |
| mezzo-soprano | A3–A5 | B3–E5 | 15.0 cm | female |
| soprano | C4–C6 | E4–A5 | 14.5 cm | female |

- **Keys:** a key is its sounding note, G1 to C7. Below D2 only the oktavists reach (two basses in
  four, down to G1); the other basses double them an octave up, softer. Above C6 the sopranos sing
  an octave down.
- **Tuned by the spectra** (the pajupilli intonation, as the organ's; `PJ_CHOIR_JI=0` or `PJ_JI=0`
  turns it off): each sung note's spectrum is its voice's harmonics, falling ~6 dB an octave, with
  the band of the first two resonances (~300–1500 Hz) stronger. Every two notes' partials within
  36 c of each other pull their interval to where they meet, by the weaker one's level over the higher
  harmonic number squared (the ear tunes by the low, resolved partials). A chord struck together
  is placed together, its mean on equal temperament; a note already sounding (150 ms) moves at most
  3 c a change; at most ±25 c (`PJ_CHOIR_JI_LIM`). Each singer follows in ~80 ms. C major: the third
  10.6 c under its root, the fifth +1.8 c (`PJ_CHOIR_TUNEDBG=1` prints each chord's tuning).
- **A key alone** is sung in unison by the parts at ease on it: C3 by the basses and baritones, C4 by
  the baritones, tenors, altos and mezzos, A4 by the altos, mezzos and sopranos.
- **A chord** is spread over the parts by pitch, the basses on its lowest key and the sopranos on its
  highest. A key no part chose is taken by the nearest part, which divides (at most three keys a
  part).
- **Moving between chords:** as few singers move as will do, and a moving singer slurs. Near the top
  of a part's range only some of its singers reach, so the part thins out (one soprano sings C6).
- **Each singer is its own**, fixed for its seat: tract length (±4 %), vowel (R1 ±8 %, R2 ±6 %),
  pitch (sd 6 c), vibrato rate (5.3 ± 0.35 Hz), extent (±35 c ± 12; basses 0.7×, sopranos 1.2×) and
  delay (0.3 ± 0.1 s), level (sd 2.5 dB, one in six a strong voice at f) and breath.
- **Each note:** a fresh offset (sd 4 c), a start a little flat gliding in, and an entry ~25 ms after
  the key, give or take 20 (plus the singer's habit and the part's moment). The release is by
  breath, 1.5 times as scattered. In a long note each singer breathes on its own clock, every
  8–15 s.
- **Nothing is chorused.** Measured on a six-part chord: the singers' mean pitches 4–7 c apart within
  a part, 17–32 c of dispersion with the vibratos, and the onsets spread over 80 ms.
- **The vowel** is a dark choral [ɑ], from rows refitted on the singer's own line
  (`src/singer_vowels_data.h`, `tools/choirvowels.cc`): R1/R2 720/1185 Hz for the basses and
  840/1320 Hz for the sopranos.
- **Resonance tuning:** the women open the jaw to keep R1 just above f0 once f0 nears it (sopranos
  from ~A5: R1 905 Hz at A5, 1050 Hz at C6). The men nudge R1 onto 2 f0 when it is near.
- **The lungs** run from ~0.4 kPa (pp) to 3.5 kPa (ff), 1.4× an octave above a part's middle (to
  5 kPa). The glottal pulse closes faster as the voice grows: pp to ff is ~18 dB, and the 3 kHz
  region rises ~6 dB more than the whole.
- **Levels:** each singer's level is evened by its note, so a lone key's choir rises only gently
  from E2 to C6 (`PJ_CHOIR_EVEN`; the physics alone rose 35 dB).
- **Room:** it sings in the church unless `-room` says otherwise (in a MIDI file, the file's room).
- **Knobs** (live, or `-seq`'s `knob K X`), the same for every choir:
  - K2 is the vowel, from dark to bright: u, ʊ, ɔ, ɑ (at 0.6), e, i;
  - K3 is the openness: closed and breathy to the left, the jaw open and the tone clear to the
    right.

  Both glide in ~40 ms, and each singer keeps its own vowel on top.
- **Cost:** about 0.8 % of a core per singing singer; 24 singing take ~12–19 % of a core dry, ~25 %
  in the church. Silent singers are not computed. `PJ_CHOIR_N` sets the singers per part (4).
- **Variants:** another choir instrument (Voice Oohs) is a row of `kChoirVariants` (`src/choir.h`).
  It sets the vowel per part, the attack, the softness, which parts sing, a soft "d" onset and a
  hum. The parts, singers, dealing and room are shared.

### Voice Oohs

`-inst voice-oohs` (GM 54, Voice Oohs) is the same choir of 24 singers on "ooh", as a variant of
`choir` (`src/choir.cc`):

- **The women lead.** The men sing only keys below C4: a lone key or chord tone from C4 up goes to
  the altos, mezzos and sopranos.
- **The vowel** is a rounded choral [u] for the sopranos and mezzos, moving toward [ʊ] through the
  altos, tenors and baritones to the basses. The rows were refitted on the singer's own line
  (`src/singer_vowels_data.h`, `tools/choirvowels.cc`): R1/R2 ~300/800 Hz for men, ~360/900 for
  women; the old singer's /u/ was a fronted [ʉ] with F2 ~1370. The women's top opens toward [ɔ]
  from D5 to G5, as sopranos do when a rounded [u] cannot keep R1 up with f0.
- **Softer** than the aahs and a little slower: lower pressures, the lungs rising with τ ≈ 75 ms at
  mf, a soft onset (half level at ~75 ms against the aahs' ~65, within 2–3 dB of full in ~0.2 s),
  breathier (×1.3), less vibrato (×0.8). Firmer keys start with a soft "d", as Roland's "dooh" does.
- **Dark:** the band 1.5–5 kHz is ~28 dB under the lows on A4, against ~10 dB for the aahs. Before
  its level trim it is ~6 dB quieter than the aahs.
- **Hum:** `src/hum.h` closes the lips into a hum ("mmm", GM2 Humming). It is a nasal pole (270 Hz)
  and a zero gliding up to 1 kHz on a singer's output, ~15 multiply-adds a sample instead of a
  nasal line (`tools/humtest.cc`). It is off in Voice Oohs: on the new [u] even a little of it
  dulls the voice further.

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
  The dampers land as forces: each head (12-20 g) gives its strings its momentum in an 8 ms
  felt push, and its grip comes on as it touches; a pedal release lands them all, a soft
  thump and "chuff" ~30 dB under a mezzo-forte chord (no click), the lift rail's knock under
  it through the rim. The release's speed follows CC 64 when it moves through values between
  (an on/off pedal: 0.15 m/s, each release its own). `PJ_PIANO_PEDALNOISE` (1; 0: off, the
  old piano byte for byte).
- **The strings stretch:** a loud blow lengthens a bass string by the square of its swing; its
  lengthwise modes (~16 x the fundamental) ring with the sums of its partials and push the
  bridge along the string (Bank's phantom partials): at ff in the bass 0.6-2.8 dB more 2-5 kHz
  at the attack, dying with the partials; nothing at mf (`PJ_PIANO_LONG`, 0.7; 0: off).
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
- **Cost:** about 9 % of a core for a single note, 32 % with the pedal down (the stretching
  adds ~0.75 % of a core per loud bass key while it lasts; the dampers' landing only its 8 ms).

### Bright piano (Shigeru Kawai SK-7)

`-inst bright-piano` (GM 2, Bright Acoustic Piano; `src/sk7.cc`, research/shigeru_kawai_sk7.md),
A0 to C8: the grand's strings, hammers and dampers built as a Shigeru Kawai SK-7, Kawai's 229 cm
semi-concert grand, and miked close as a studio or pop grand is. The brightness comes from the
instrument, not from an EQ:

- **Scale:** a 229 cm case: wound strings 1.66 m at A0 to 1.16 m at E2, plain strings from
  1.44 m at F2 reaching the grand's own lengths at C5. The shorter bass and low tenor are stiffer
  (B 1.0e-4 at C1, 2.3e-4 at C3, against the grand's 7e-5 and 1.1e-4): their upper partials stand
  sharper and stronger, and the tuning stretches further (C1 about 15 c flat, the grand's 7).
- **Hammers:** Kawai presses its felt dense and needles only the shoulders, with no hardeners:
  the felt twice as stiff as the grand's at C4 and 2.5 times from C6 (`PJ_SK7_HARD`, `_HARDTOP`),
  its loose outer nap half as thick (`PJ_SK7_NAP`). A middling blow is already bright (above
  2 kHz C4 at velocity 64: −35 dB, the grand's −61); the softest notes stay round (under −50 dB).
- **Board:** the grand's board drawn in at the tail to the shorter case, 1.77 m² (`tools/sk7board.py`
  → `src/sk7board_data.h`), its own loss ×0.7 (`PJ_SK7_ETA`: the aged spruce and hard rim the
  makers credit with a long sustain). Middle C falls 40 dB in 9.6 s (the grand 5.7), C5 in 3.6 s
  (4.7); the bass a little shorter than the concert grand's, as a smaller grand's is.
- **Sound:** the grand's crossed figure-of-eight pair 30 cm over the strings and 40 cm from the
  belly rail (`PJ_SK7_MICZ`, `_MICX`), placed for evenness from key to key (as even as the
  grand: 0.8 / 1.1 / 2.7 dB rms off the neighbours, bass / middle / top); the room hears it
  without the grand's extra reverb send.
- **Against the grand,** on a solo piano file (the first 75 s of a jazz ballad): 7.8 dB more
  above 2 kHz, 3.7 dB more at 4-8 kHz, 1.3 dB above 8 kHz: presence, not hiss.
- **Against FluidR3's Bright Acoustic Piano** (TiMidity++): its velocity curve within 1.5 dB of
  FluidR3's after the velocity gain; FluidR3's is the same sample as its grand, 3 dB louder, its
  brightness fixed at about −15 dB above 2 kHz at every velocity; here the share grows with the
  blow (C4: −53 dB at velocity 40, −24 at 127), and the notes sustain longer.
- **Cost:** as the grand's: about 16 % of a core for a single note, 55-60 % with the pedal down
  (both measured on a loaded machine, where the grand's read 15 % and 56 %).

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

### Electric piano 1 (the Fender Rhodes)

`-inst electric-piano-1` (GM 5), A0 to C8 (`src/rhodes.*`, research/rhodes.md): the Rhodes as its
parts, key by key, then its electronics.

- **Tines:** lengths from the factory's cutting sheet, the swaged tine's mode ratios with its
  spring (5.8 .. 9 and 17.6 .. 28 x f0; modes 2 and 3 gone in tens of ms), the fundamental's Q from
  Shear's measurements (T60 ~11 s at C4, ~4 s at C6), the factory's stretch tuning (-20 c at A0 to
  +30 c at C8), a weak sideways polarisation 0.02 .. 0.3 Hz apart (the slow swirl); the tonebar a
  lossy mode far under the note, moving the tine (Muenster & Pfeifle).
- **Hammers:** a Hunt-Crossley contact with the tip of its zone (Shore 30/50/70/90, the wrapped
  maple core at the top) against the tine's place and speed as they are: 5-7 ms in the middle (6.4
  measured); a key struck again meets the moving tine. Each key's action is calibrated so that a
  mezzo-forte leaves the tip at 3 m/s from C3 up (the even touch: C3 swings ~3 mm, C6 ~0.5 mm), the
  bass's swing levelling at ~3 mm.
- **Pickup:** the flux a bell of the tip's height over the pole's edge, its derivative on the full
  tip motion every sample: soft notes nearly sines, harder ones barking (the bass first: E2's 2nd
  harmonic -19 dB under the fundamental at velocity 30, +9 dB over it at 120), the fundamental
  folding back and blooming as a hard bass note decays. Gaps 1.6 / 0.8 mm, the offsets as the
  factory voiced them; every key's gap, offset, strike, tuning and Q a little its own.
- **Electronics:** the harp (1.4 kOhm, 0.65 H, the cable) into its load; the Peterson Suitcase by
  default (preamp, the stereo vibrato's lamps and CdS cells, the power amplifier's sagging rails,
  the 4 x 12 in cabinet), or the Stage straight out (`PJ_RHODES_SUITCASE=0`); a trace of hum.
- **Knobs:** `PJ_RHODES_VIB` (0.5; 0: off), `PJ_RHODES_VIB_HZ` (4.5), `PJ_RHODES_GAP`,
  `PJ_RHODES_VOICING` (x the offsets: smaller is more bell), `PJ_RHODES_WEAR` (1; 0: as the tables, 3:
  road-worn), `PJ_RHODES_HUM`, `PJ_MAINS_HZ` (50). Live: K3 the vibrato's speed, K4 its intensity.
- **Not yet:** sympathetic ringing through the rail, the pickups' crosstalk, the tine against the
  pole or the damper at ff, the Mark I/II damper snap-back, a near-tuned tonebar (Moog's picture),
  the Stage's amplifiers. About 1.3 % of a core for a held note.

### Electric piano 2 (the DX7's E.PIANO 1)

`-inst electric-piano-2` (GM 6), A0 to C8 (`src/dx7.*`, research/electric_pianos.md section 6):
the Yamaha DX7's factory voice E.PIANO 1 (ROM1A voice 11), its 128 bytes from the factory bank,
played by the reverse-engineered DX7 engine's arithmetic (MSFA, as in Dexed): six operators in
algorithm 5 (1 <- 2 at 14:1, the tine's clank; 3 <- 4 and 5 <- 6 at 1:1, the bark, 6 feeding
back), the integer envelopes stepping every 64 samples, the output-level, velocity, rate- and
level-scaling tables, the measured detune, free-running phases (key sync off). Velocity works as
on a DX7: mostly the timbre (a soft key nearly a sine, a hard one barking), the level only a few dB.

Unlike a DX7, each key is a piano's unison: three whole DX7 voices from C3 up, two from C2, one
below (that one exactly the DX7's), 1/7 Hz apart, each with its own output levels (+-0.4 dB) and
envelope speeds (+-3 %). Knobs: `PJ_DX_ONE`/`PJ_DX_TWO` (36, 48: the lowest key with two, three),
`PJ_DX_SPREAD` (1: x 1/7 Hz), `PJ_DX_VARY` (1; 0: the generators alike), `PJ_DX_POLY` (32 notes).
The sustain pedal holds; pitch bend +-2 semitones.

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

### Electric guitars (clean, muted, overdriven, distortion)

`-inst steel-guitar` (GM 26; `research/steel_guitar.md`, `research/steel_guitar_build.md`,
`src/guitar.cc` `steelGuitar()`): the steel-string acoustic, a dreadnought with phosphor-bronze
12-53 strings, flatpicked. The same strings-on-a-bridge model as the nylon guitar with:

- **The strings:** twice the nylon's tension and far less loss, 96 modes each to 10 kHz. Measured
  on open strings: the overtones' loss rises about linearly with frequency (3 to 8 dB/s per kHz),
  and at 3 kHz a note still takes 2 to 3.5 s to fall 60 dB.
- **The body:** the air and top resonances at 98, 183 and 204 Hz, two dipoles that radiate little,
  then plate resonances to 10 kHz with masses set to the measured mean bridge mobility. Nothing
  is measured above 3.6 kHz; the plates simply go on.
- **The pick:** the electric guitars' pick, 10 to 13 cm from the saddle, a strum over the
  soundhole, with its own click (a burst of noise as the string leaves the edge, a rattle over
  the wrap on the four wound strings; `PJ_STEEL_CLICK`).
- **The hands:** a strum takes 5.5 ms a string over six strings and 8.6 ms over three; the key's
  release stops the string, open or fretted; while strumming it is only eased off.

Not built yet: fingerstyle, the up stroke leaving out the lowest strings, the 12-string. Knobs:
`PJ_STEEL_LOSS`, `PJ_STEEL_LOSSHIGH`, `PJ_STEEL_BODYMASS`, `PJ_STEEL_BODY` (1: an OM),
`PJ_STEEL_RADHIGH`, `PJ_STEEL_OUT`.

`-inst clean-guitar` (GM 28), `-inst muted-guitar` (GM 29), `-inst overdriven-guitar` (GM 30) and
`-inst distortion-guitar` (GM 31; `research/electric_guitars.md`, `research/distortion_guitar.md`,
`src/guitar.cc`, `src/guitaramp.cc`): a Stratocaster played with a pick into a clean Fender Twin;
the muted one is the same guitar with the picking hand's palm on the strings and a compressor in
front of the amp; the overdriven one a Les Paul's bridge humbucker through a compressor into a
Marshall JCM800 at crunch; the distortion one the same Les Paul into a Soldano SLO-100's
overdrive channel, gated, and a closed 4×12 of Vintage 30s. The same six strings and guitarist as the
classical guitar, in steel, heard through pickups; the amp is a module of its own after the jack.
- **The strings:** 10–46 on 648 mm, tensions from the maker's chart, the plain G the most
  inharmonic; up to 96 modes (8 kHz on the low E). They ring 14 s on the low E, 5 s on the high
  one (not yet fitted to recordings: `PJ_EGTR_LOSS`, `PJ_EGTR_LOSSHIGH`). Each string's two
  directions of swing are a little askew (`PJ_EGTR_TILT`, 0.35 rad, each mode its own) and a
  little apart in pitch (`PJ_EGTR_SPLIT`): a pick throws the string mostly along the top, the
  pickup hears what lies towards it, and the two beat slowly: the note blooms and breathes.
- **The neck:** two resonances under the strings' ends (`PJ_EGTR_DEADHZ`, 212 Hz; `PJ_EGTR_NECK`):
  a dead spot, G#3 losing ~10 dB more in its first 0.4 s than its neighbours. The body is still
  and adds nothing of its own.
- **The pick:** a 0.8 mm edge pushing the string 0.1–3 mm aside (the velocity) over a millisecond
  and letting it slip off in 0.15 ms, ~10 cm from the saddle (a hard stroke nearer the bridge;
  the brightness knob moves it); it passes, so what the string was sounding goes on. Single notes
  are picked down and up in turn while they come within 0.3 s, down after a rest; an up stroke a
  little lighter, the other way across the string. Chords are one sweep (6 ms a string,
  `PJ_EGTR_STRUMSPEED`), down or up as the strumming hand swings.
- **The pickups:** measured positions on a Stratocaster (the bridge pickup slanted, 38–49 mm from
  the saddle; middle ~100; neck ~158): each hears the string where it stands, so each has its
  comb (on the open low E the neck pickup is deaf to the 4th harmonic, the bridge one to the
  13th), over the field's width (`PJ_EGTR_WIDTH`, a 6 mm Gaussian), through the magnet's field
  (the on-axis field of its pole piece: a string swinging close grows an even harmonic), each
  string in proportion to the steel in its section (otherwise the thin plain strings would be
  loudest: `PJ_EGTR_STEELEXP`). The five-way selector (`PJ_EGTR_PICKUP` 1 bridge … 5 neck; 4,
  middle + neck, the in-between "quack", for GM 28; 2, bridge + middle, for GM 29): two pickups
  together are two sources in parallel. 0.15–0.25 V peaks per note, ~0.5 V for a hard strum. `PJ_EGTR_DI=1`: the pickups' EMF
  itself, without the amp.
- **The palm** (GM 29): the side of the picking hand lies across all six strings 8–16 mm from the
  saddle as each stroke is picked, a lossy patch 12 mm wide: each mode loses in proportion to how
  much it moves under the hand, up to 30 times what the fundamental loses (soft flesh takes only
  so much; `PJ_EGTR_PALMSAT`). Matched to FluidR3's recorded palm-muted guitar (the first
  version, from an estimate, was 3–5 times too muted): the fundamental to −30 dB in about 0.25 s
  (`PJ_EGTR_PALM`, 5/s on an open string's fundamental), the 2nd harmonic in 0.1–0.16 s, the 4th
  in 50–60 ms, the 8th in 12–25 ms. The harder the stroke the lighter the palm lies and the nearer
  the saddle (1.4 times at the softest, 0.35 at the hardest: at velocity 127 the fundamental rings
  0.5–0.75 s, at 40 about 0.25 s), and one stroke in six it rests lighter by itself; never twice the same;
  the pick nearer the bridge (~6 cm); the strings not played do not ring along; a released note
  let go more gently than on the clean guitar (`PJ_EGTR_MUTEDRELEASE`, 25/s), the palm already
  on it. Its level is set so its strokes are as loud as the clean guitar's over 100 ms (the usual
  one-second measure would push short notes up by 8 dB more).
- **The amp** (`src/guitaramp.h`): the pickup's coil (2.2 H, 6.5 kΩ, 110 pF; two in parallel: half
  of it) with the cable (`PJ_EGTR_CABLE`, 470 pF) and the pots and the amp's 1 MΩ, solved as
  one: the single coil's resonance at 4–6 kHz. For GM 29 a compressor pedal next (the Dyna Comp's
  kind: ratio 4 above 50 mV, 3 ms to clamp, 0.2 s to let go; `PJ_AMP_COMP`, `PJ_AMP_COMPAT`).
  Then a triode (nearly straight until its grid
  conducts at +1.4 V, cut-off softer at −2.8 V, a slight even curve), the Fender blackface tone
  stack, exact from its parts (Yeh and Smith's closed form, checked against a nodal solve to 0.01
  dB; `PJ_AMP_TREBLE` 0.6, `PJ_AMP_MID` 0.5, `PJ_AMP_BASS` 0.4), the volume (`PJ_AMP_VOLUME`,
  0.2), the second triode, a push-pull power stage whose supply sags under a loud passage
  (`PJ_AMP_CLIP`), and the 12" speakers in an open-back cabinet as a filter (resonance, a lift at
  2–4 kHz, breakup, nothing above ~7 kHz; `PJ_AMP_CAB`). Clean: a hard strum compresses about 1 %.
  The overdriven and high-gain amps (GM 30, 31) are these blocks driven harder, oversampled
  (below).
- **The Les Paul** (GM 30): 628 mm, 22 frets (the same strings 6 % slacker, 13 % more
  inharmonic); two humbuckers, each two coils 18 mm apart in series (twice a coil's signal, and
  the pair's own comb), the bridge one hot (8 H, 13.5 kΩ: its resonance near 2 kHz), 500 k pots;
  the three-way switch (`PJ_EGTR_PICKUP` 1 bridge, 2 both, 3 neck; GM 30: the bridge). A single
  note gives ~0.5 V, a hard power chord 1.3 V. Its overtones ring twice as long as the Strat's
  (`PJ_EGTR_LPRING`, 0.5 of the loss). The pick over the bridge pickup (~6 cm).
- **Feedback** (GM 30): loud and driven, a held note sings as long as it is held. The player
  stands where the amp's sound comes back in phase with the note: from 0.12 s every mode of the
  string below 2.5 kHz is given back what it loses (`PJ_EGTR_HOLDAT`, `PJ_EGTR_HOLDHZ`), so the
  note keeps its level instead of falling out of the amp's clipping after a second; at 1.2–2 s
  it blooms: a low note grows its octave or twelfth and turns into it, a middle one swells or
  goes to its octave, a high one swells (`PJ_EGTR_BLOOM`, 2). Single notes, dyads and power
  chords played at velocity 50 or more; let go or picked again, it stops at once. The hands keep
  the strings not played muted (else they would ring up). `PJ_EGTR_FEEDBACK=0`: none (the note
  then falls 20-30 dB in 2-4 s). An acoustic loop through the air (the speaker's sound shaking
  the body, 2 m away) was tried first: at most distances it squealed at the speaker's 2 kHz or
  fought the note, as it does for a player who has not found the spot.
- **The Marshall** (GM 30; `jcm800` in `src/guitaramp.cc`): a compressor pedal first (ratio 2.5
  above 0.2 V: a held note keeps driving the amp, 5 dB stronger 2 s on; `PJ_AMP_COMP=1`: none),
  then V1A (gain 50), the preamp gain with its 470 pF bright capacitor (`PJ_AMP_GAIN`, 0.08: the
  lower the gain, the more of the treble passes it), V1B biased cold (it clips its lower half
  first; the Miller roll-off from 7 kHz), the Marshall tone stack after the clipping
  (`PJ_AMP_TREBLE` 0.7, `PJ_AMP_MID` 0.6, `PJ_AMP_BASS` 0.5), the master (`PJ_AMP_MASTER`, 0.6),
  the EL34s, a closed 4×12 of Greenbacks (the closed box's thump, forward upper mids, nothing
  above ~6 kHz). A stage driven past its grid's conduction draws grid current and its bias
  slides cold for ~20 ms (the bloom of a cranked amp). Crunch: soft picking comes through
  nearly clean, a hard stroke drives the cold stage well past its knee; from velocity 30 to 120
  the pickup's 17 dB come out as 6 dB, the rest goes into distortion. The stages run 4×
  oversampled (an 8th-order elliptic up and down, `tools/oversample.py`; `PJ_AMP_OS=1` to
  compare: at crunch the soft knees alias little). 2.4–3.7 % of a core.
- **The high-gain amp** (GM 31; `slo` in `src/guitaramp.cc`): a Soldano SLO-100's overdrive
  channel, four triode stages in a row (the stages are a list of up to five, each with its
  coupling capacitor, Miller roll-off, a partly bypassed cathode as a low shelf, a divider into
  the next, a cathode follower after). V1A (gain 60, its bass 3 dB under the treble below
  ~170 Hz), the gain pot behind a 470 k with 2000 pF across it (the bass a further 5.6 dB down;
  `PJ_AMP_GAIN`, 0.1: about 4 of 10), V1B (40, its bass 4.5 dB under), V2A the cold clipper
  (biased near cut-off, gain 2.4, its plate rolled off from 1.6 kHz), V2B (75, its coupling
  capacitor small: 72 Hz, the bass kept out) into the DC-coupled cathode follower (it squashes
  V2B's cut-off swing: `PJ_AMP_FOLLOWER`), then the Marshall stack after all of it
  (`PJ_AMP_TREBLE` 0.6, `PJ_AMP_MID` 0.5, `PJ_AMP_BASS` 0.6), the master low (`PJ_AMP_MASTER`,
  0.4), 6L6s with a softer knee than the EL34s, resonance (+4 dB at 100 Hz, `PJ_AMP_RESONANCE`)
  and presence (+3 dB from 3 kHz, `PJ_AMP_PRESENCE`). A humbucker's note is some 35 dB past the
  last stage's knee, a soft one 20: everything distorts fully; from velocity 40 to 127 the level
  grows 2 dB and the attack brightens (its first 30 ms' centroid +14 to +73 % on E2, A2, A3).
  Grid current charges the coupling capacitors five times slower than in the Marshall (the
  Soldano's grid stoppers; `PJ_AMP_GRID`, 0.2): faster, every hard attack pushed the stages cold,
  thin pulses and a dark, blocked attack. A power chord comes out as loud as one note (+0.4 dB;
  FluidR3's samples add up, +3.8), a triad with more lines between the harmonics than a power
  chord. A held note stays within 1 dB for 2 s (without feedback −3.5 dB at 4 s).
- **Antialiasing:** the stages run 2× oversampled (a second elliptic table in
  `tools/oversample.py`) and each stage's curve is averaged over the sample (first-order
  antiderivative antialiasing, the curve's integral tabulated and in closed form beyond;
  `PJ_AMP_ADAA`, `PJ_AMP_OS` 1, 2 or 4): a held A5's energy off its harmonic grid is −79 dB (2×
  alone −48, neither −29). 3.3 % of a core with the guitar (4× would be 4.3 %).
- **Noise and the gate** (GM 31): the pickup and the first stage hiss (`PJ_AMP_HISS`, 8 µV rms)
  and the coils and the amp's heaters hum (`PJ_AMP_HUM`, 250 µV peak at the input, its odd
  harmonics to 1 kHz, buzzy; `PJ_AMP_HUMHZ`, 50): ungated, a rest sits 45 dB under a held note.
  The gate is the ISP Decimator's kind: it listens to the guitar before the amp (`PJ_AMP_GATE`,
  2 mV) and opens and closes the preamp's output, closing 6 dB lower after a hold
  (`PJ_AMP_GATEHOLD`, 15 ms) over `PJ_AMP_GATEREL` (80 ms to −80 dB). Its release adapts: a note
  that falls quickly 20 dB under its attack (a palm-muted chug, a released string;
  `PJ_AMP_GATEDROP`) closes it at once, four times as fast, and only a new attack opens it again;
  a string decaying by itself keeps it open (a 6 s note: one close, at its release).
  `PJ_AMP_GATEDEBUG=1` prints each open and close.
- **A Tube Screamer** in front, if wanted (`PJ_AMP_TS`, its level; `PJ_AMP_TSDRIVE`,
  `PJ_AMP_TSTONE`): the clean signal and, added to it, its part above 720 Hz amplified and clipped
  by two diodes. Off: with the gate it did not tighten the chugs further.
- **The V30 cabinet:** a closed 4×12 of Vintage 30s, an SM57 close: nine filters, started from
  Celestion's plot and fitted to FluidR3's distortion-guitar samples (their pooled harmonic
  envelope within 5.5 dB from 200 Hz to 8 kHz: the presence pair at 2.6 and 3.25 kHz, +5 dB over
  1 kHz; −9 dB at 5 kHz, −25 at 8, −39 at 10). The strong low mids at 350 Hz and the wide hollow at
  900 Hz are the recording's, not the bare speaker's (`PJ_AMP_CAB`, `PJ_AMP_BREAKUP`).
- **The guitarist on GM 31:** he picks over the bridge pickup (~4 cm). A root and its fifth low
  on the neck become a power chord with the octave on the next string (`PJ_EGTR_OCTAVE=0`: as
  written). Chugs: with no marks in a MIDI file, a low note (below A3) struck again on the same
  root within 0.4 s, when the last stroke was let go early (under 0.25 s, or 0.6 of the gap),
  gets the palm, at the saddle (3–8 mm), 0.9 of GM 29's weight, heavier as the run goes on; not on
  an accent (15 steps louder), not while bending, not on a full chord; the first stroke of a run
  rings open (`PJ_EGTR_CHUG`, its weight; 0: never). Sixteenths at 160 bpm: 46 dB between the
  strokes (the gate), the chug's centroid near 590 Hz (without the palm 1.7 kHz). A single fretted
  note from E3 up, held, gets the rock player's wide vibrato, 40–100 cents up and back
  (`PJ_EGTR_WIDE`). Feedback as on GM 30.
- **The left hand** as on the classical guitar (frets, vibrato, the mended thirds); the fretting
  hand mutes a released note at once. A pitch bend (`-play`; `-seq`'s `bend X`) pushes the
  highest fretted string of what was last picked, up to two semitones; the rest of the chord stays.
- Not yet: the dead strokes of funk (the fretting hand's scratch), the archtop (27), pinch
  harmonics, drop tunings (notes under E2 are folded up), the other high-gain
  presets (a Rectifier, a boosted JCM800, distortion pedals), fret buzz, string squeak, slides
  over frets, chorus and spring reverb, hum on the clean amps; a bass line on GM 29 (louisiana's channel 6) is folded up into the guitar's range, where
  an electric bass belongs. 2.6 % of a core.

### Guitar harmonics

`-inst guitar-harmonics` (GM 32; `research/electric_guitars.md` 2.6, 9; `src/guitarinst.cc`
`HarmonicsPlayer`, `Guitar::touch`): the clean Stratocaster and Twin of GM 28, every note played as
a harmonic, at the pitch written (E3 .. G#7; below an octave up, above an octave down).
- **The pickup:** the middle one alone. The neck pickup lies on the open strings' 4th-harmonic
  node (the 5th fret's harmonic would be nearly gone); the middle one has no node near harmonics
  2–5 of an open string.
- **The touch:** a fingertip (4 mm, ±0.7 mm off the node) lies on the string at 1/n of its
  sounding length from just before the pick until 30–60 ms after; each mode loses 300/s times its
  shape squared there (as `Bass::touch`), so every mode not a multiple of n dies, and n, 2n, 3n …
  ring on. It lifts gradually (its hold halved every 6 ms: a sudden lift sets free what the bridge
  forced the touched modes to). The string's fundamental ends 45–80 dB under the harmonic, every
  other non-multiple 40–60 dB.
- **Where:** a natural harmonic if there is one (the 12th fret, n = 2; the 7th, n = 3; the 5th,
  n = 4), a free string first; else a touched ("harp") harmonic, n = 2: the left hand stops the
  string 12 frets under the note, near where it already is, the right hand touches 12 frets
  higher; past the 21st fret's octave n = 3, then n = 4. Not n = 5 (the 4th fret's, 14 c flat).
  The harmonic is the string's n-th mode, as it is on a guitar: stiffness makes it 0–3 c sharp of n
  times the fundamental, n = 3 is a pure twelfth (+2 c), and near the neck's dead spot (212 Hz) the
  neck pulls the A string's octave and the low E's twelfth up 4–6 c more; left so.
- **The pick:** on the harmonic's antinode nearest the bridge (len/2n, 3.5–16 cm: never its node;
  there mode 2n has its node too), a rounder stroke than GM 28's (pushed over 2–3.5 ms) so that
  the harmonic is not drowned in its own 3rd partial; velocity is its depth (30 to 120: 14–15 dB).
  The player evens the harmonics out by ear (digging in on open strings and where the pickup is
  near the node, light high up the neck: fitted to all 53 notes, 1.5 dB rms). Notes together are
  one sweep from the low strings, 8 ms a string.
- **Ringing:** a harmonic rings past its key's release; a string needed again at another fret is
  damped 4 ms first (every note waits those 4 ms); after 1.5 s with no key down the hand lies
  lightly on the strings. Natural harmonics ring for seconds (the low E's octave 2 s to −40 dB);
  FluidR3's samples are cut at 0.2–1.5 s.
- **Against FluidR3:** its Guitar Harmonics sounds two octaves above the key (E3 plays E5):
  `PJ_GHARM_SHIFT=24` plays it so. Level even over the range in both (FluidR3 ±0.8 dB; here
  1.5 dB rms, ±4 at worst). Knobs: `PJ_GHARM_TOUCH` (300/s), `PJ_GHARM_LIFT` (×30–60 ms),
  `PJ_GHARM_RISE`, `PJ_GHARM_EVEN=0` (no evening out), `PJ_GHARM_DEBUG=1` (string, fret, n).
- Not yet: pinch and tapped harmonics, the tremolo bar's dives, chorus; the pitch wheel does
  nothing. 2.4 % of a core.

### Jazz guitar

`-inst jazz-guitar` (GM 27; `research/jazz_guitar.md` 9–12; `src/guitar.cc` `jazzGuitar()`,
`src/jazzguitar.cc` `JazzGuitarPlayer`, `src/guitaramp.cc` `jazz()`): a Gibson ES-175 played by a
jazz guitarist into a clean jazz amp.
- **The guitar:** 628 mm, 20 frets, flatwounds 12–52 (95–103 N each, the G wound: its 10th
  partial +0.6 c). The wound strings lose above the fundamental as k² (the ribbon's turns
  rubbing): the open low E through the pickup has its harmonics 1–5 down 15 dB in 5.7, 3.4, 0.5,
  1.2, 1.0 s (Rau17's measured flatwound hollow body: 5.45, 4.49, 0.86, 0.87, 1.12). A laminated
  archtop body (the neck's modes at 85/175/332 Hz, the f-hole air at 125, the top at 240, the
  bridge rocking on its feet at 420, plates to 2.2 kHz); the floating bridge drains the swing
  towards the top twice as fast as the one along it. The neck humbucker 169 mm from the saddle
  (4.4 H; its comb's first notch at an open string's 3.7th harmonic), mounted in the top: it moves
  with the top and hears the body through every string's steel. The archtop's own sound is mixed
  20 dB under the amp (A-weighted, on a chord).
- **The tone control at "3":** the tone pot (45 kΩ, `PJ_JAZZ_TONE` in kΩ) in series with 22 nF
  across the pickup, a 600 pF cable: the amp's input as an exact third-order network (35 kΩ:
  −3 dB at 1.38 kHz, no peak; 0: the "woman tone", +3.8 dB at 467 Hz).
- **The amp ("jazz"):** a Polytone-like clean solid-state amp: no tube stage driven, an active EQ
  (+3 dB under 150 Hz, +2 at 500, −2 above 2.5 kHz) instead of the Fender stack's scoop, a closed
  1×15 (85 Hz, −3 dB at 2.5 kHz, 24 dB an octave above 3.5).
- **The touch speaks.** Every stroke is a heavy, round-edged pick held by a hand
  (`Guitar::pickContact`, now with the tip's width): 1.5 mm (17 kN/m), a 0.25 ms edge, 2.5 mm of
  contact. Velocity is how far (0.1–2.1 mm) and how fast (a 10 ms push soft, 2.5 ms hard) the hand
  carries the tip, and the string slips off the edge the faster the faster the hand goes (0.5 ms
  soft: rounded, few partials over 1.5 kHz; 0.2 ms hard). The hand moves with the phrase:
  0.19 m from the saddle on soft, even comping (round, "oo"), toward 0.10–0.13 m on accents and
  loud phrases (nasal, "ah"); harder strokes dig into the top. Against the body's resonances and
  the pickup's comb, that is the vowel; the flatwounds' k² losses make it change in time (G3 at
  v120: the 2–5 kHz share falls 26 dB from the first 50 ms to 200–400 ms, into the round tail).
  Measured on G3, D4, A4 at velocity 30/60/90/120: the centroid rises 236→275, 315→396, 469→628 Hz
  and the 2–5 kHz share −63→−31, −51→−23, −47→−20 dB, every step up; the 5–10 kHz share at 127
  stays 52–90 dB under the attack.
- **The speaking strip** (the "compressor kludge"; `guitaramp.h` `Preset::presDb …`, after the
  speaker, as an engineer's channel): a presence peak at 3.4 kHz (Q 0.8), +2 dB, and an upward
  expander on that band alone: it opens 0.8 dB for each dB the band stands above a firm mf attack
  (−64 dB of the output) and closes as much under it, within −4 … +8 dB, quick up (1 ms), back
  over 80 ms as the note's highs die. Then a 2:1 compressor (above −56 dB rms, 10 dB knee, 10 ms
  attack so the pick's first transient passes, 250 ms release) whose ear is the signal *before*
  the lift: it hears the body, not the presence it is helping. A soft phrase passes untouched (as
  dark as without the strip), a hard one gains presence while its level is held back. On one
  phrase (a line and a chord) from velocity 35 to 110: centroid ×1.24 against ×1.01 without, the
  2–5 kHz share +7.7 dB against −1.5, loudness +10.4 dB against +15.8. Knobs `PJ_JAZZ_PRESENCE` (dB),
  `PJ_JAZZ_PRESHZ`, `PJ_JAZZ_SPEAK` (dB/dB; 0: static), `PJ_JAZZ_PRESREF`, `PJ_JAZZ_PRESMAX`,
  `PJ_JAZZ_COMP` (ratio; 1: none), `PJ_JAZZ_COMPAT`, `PJ_JAZZ_STRIP=0` (none).
- **The guitarist:** chords voiced as a hand can hold them, never dropping the top note: the range
  folded (a chord over C6 down an octave as a whole), one string a note, the fretted notes within
  4 frets (5 from the 5th fret), no open strings in comping, known grips (shell, drop 2, drop 3)
  preferred, the hand near where it was; if none fits, reduced (doubled pitch classes, the fifth,
  the bass under a wide gap, inner notes sparing the 3rd and 7th). On the user's louisiana.mid
  channel 7: 492 chords, 4.9 % of the notes dropped, no top note, none unplayable, no open
  string, the hand moving 0.99 frets on average. A quick down rake (2 ms a string loud, 5 soft), up
  on quick off-beats; strings skipped inside a grip a dead click, the strings outside it muted by
  the fretting hand; a key let go chokes its string (a chord −40 dB 18–41 ms after its keys,
  FluidR3 30–60) with a soft "chk"; a note let go before it sounded is a ghost stroke;
  four-to-the-bar comping (Freddie Green) all down with the inner line ringing; a soft chord after a
  rest plucked with the fingers. Keys within 8 ms are one gesture (every note 8 ms late). Single
  lines: down on the beat, up off it, slurs (hammer-on, pull-off) onto the beat on one string about
  half the time, ghost notes, late shallow vibrato. Octaves (two notes 12 apart) on strings s and
  s+2 with the middle one muted, with Wes's thumb (`PJ_JAZZ_THUMB`: 0 never, 1 octaves and the
  soft notes among them, 2 every single line). `PJ_JAZZ_PLACE`, `PJ_JAZZ_MOVE`, `PJ_JAZZ_PICKMM`,
  `PJ_JAZZ_EDGE`, `PJ_JAZZ_TIPW`, `PJ_JAZZ_PICKUP` (1 bridge, 2 both, 3 neck), `PJ_JAZZ_DEBUG=1`,
  `PJ_JAZZ_STATS=1`.
- **Against FluidR3's Jazz Guitar** (louisiana ch7, 43 s): FluidR3 is much brighter (centroid 901
  Hz against 348; 0.8–2 kHz −4 dB against −17), its velocity a two-layer switch; here the colour
  moves continuously with the touch. Even with the tone full up the centroid is 419 Hz: the neck
  humbucker's comb and the closed 15" make the archtop dark.
- Not yet: the afterlength's bell (the tailpiece), feedback, reverb; the chord's length is not
  known at its key, so "long soft chords" are guessed from the rest before them. 2.7 % of a core
  on a five-note chord.

### Electric bass

`-inst electric-bass` (GM 34, Electric Bass (finger); `research/bass_section.md`, `src/ebass.cc`):
a Fender Jazz Bass played fingerstyle, on the electric guitar's string and pickup model with four
strings.
- **The strings:** 34 in. (864 mm), 20 frets, D'Addario EXL170 (45-65-80-100) roundwounds, 155-216
  N; stiffness from the measured G (B 3.5e-5) to ~2e-4 on the E. Each partial's loss from the
  string's internal damping (Fleischer's fitted values) and the neck's support: the open strings'
  fundamentals ring 17-27 s, the partials die about in proportion to their frequency
  (`PJ_EBASS_LOSS`, `PJ_EBASS_LOSSHIGH`); modes to 5 kHz (`PJ_EBASS_TOP`), ~120 on the open E. The
  neck's second and third bending modes (147 and 290 Hz; `PJ_EBASS_DEADHZ`, `PJ_EBASS_NECK`): D3
  on the G string rings about half as long as its neighbours.
- **The pickups:** two J single coils, 156 and 55 mm from the saddle, both full up (GM 34; the
  "both up" scoop: the open E's 7th-8th harmonics ~10 dB down against the neck pickup alone),
  `PJ_EBASS_PICKUP` 1 bridge, 2 both, 3 neck; each string heard in proportion to its steel; the
  coils (3.3 H, 7.4 kΩ, 90 pF; two in parallel), the pots (two volumes, one tone) and the cable
  (`PJ_EBASS_CABLE`, 700 pF) into the input: a resonance near 4.4 kHz.
- **The amp, the DI and the compressor** (`research/bass_amps.md` 10.1, the amp module's preset
  "b15-di" in `src/guitaramp.cc`), as 1970s-90s records have a fingered bass: the pickups on a
  splitter into an active DI and an Ampeg B-15N (0.9 MΩ between them).
  - The B-15N's 1966 channel from Piazza's drawing: two 6SL7 stages (~30x each, their cathodes
    unbypassed: a grid swing four times a 12AX7's), the volume, the **James (passive Baxandall)
    stack** exact from its parts (an admittance matrix solved at eight points of s, its fourth-order
    numerator and denominator read off and taken to z root by root; bass 1 MΩ with 220 k / 22 k and
    1 / 10 nF, treble 1 MΩ with 470 pF / 4.7 nF; flat at noon, 23 dB down; bass full +15 dB at
    40 Hz, treble full +18.9 dB at 5 kHz: the research's nodal solve to 0.2 dB), the phase splitter,
    two 6L6s on a sagging 5AR4 supply. Coupling corners of 3-16 Hz and the output's at 8 Hz: E1's
    fundamental loses 0.8 dB to them. Clean: what its curves add to a v100 open E is -43 dB; at v127
    no stage passes 0.4 of its knee (`PJ_AMP_PEAKS=1` prints them; `PJ_AMP_LINEAR` 1 straightens
    every curve, 2 the power stage's).
  - Its 1x15 miked with an RE20 at ~15 cm: -4 dB at 40 Hz, +7 dB near 1.9 kHz, -23 dB at 5 kHz.
  - The DI with its top 2 dB down above 5 kHz, blended half and half with the mic
    (`PJ_BAMP_BLEND`), level-matched at 200 Hz and lined up as an engineer does it: the polarity
    and a second-order allpass on the DI that turn its phase to the mic's at the low end (the
    cabinet's high-pass turns the mic's by up to half a turn near 40 Hz: unaligned, the blend
    would lose 13 dB there; aligned it is nowhere under 30 Hz-2 kHz more than 0.7 dB below the
    louder of the two alone; `PJ_BAMP_ALIGN`, `PJ_BAMP_POLARITY`, `PJ_BAMP_MICDELAY` in samples).
  - An LA-2A after the blend: RMS over two 10 ms poles (it does not follow a 41 Hz wave: a held E1
    sine gains 0.013 % THD), 3:1, a 6 dB soft knee, attack 10 ms, half the gain reduction let go in
    60 ms and half over a second (the opto cell's tail after a long hold); about 3.5 dB of gain
    reduction on a mf line, 6 on a ff one (`PJ_BAMP_THRESHOLD` 0.023 V rms, `PJ_BAMP_RATIO`,
    `PJ_BAMP_MAKEUP` 4 dB; `PJ_BAMP_STUDIO=0` none). Its tone: `PJ_BAMP_TREBLE` 0.45,
    `PJ_BAMP_BASS` 0.6, `PJ_BAMP_VOLUME` 0.15.
  - `PJ_EBASS_DI=1`: the bare DI as before (the coils into 1 MΩ, nothing after).
  - Against FluidR3's Fingered Bass (E1 at v120, the first 100 ms in octave bands, dB under the
    strongest): ours -16 -5 0 -9 -12 -37 -50 -92 from 20-60 Hz up to 4-8 kHz, FluidR3's -4 -1 0
    -12 -28 -36 -55 -65. Above 1 kHz both are dark (here the cabinet and the strings' own losses,
    modes to 5 kHz); ours has the J's growl at 250 Hz-1 kHz (both pickups up, roundwounds) 10-16 dB
    stronger and the open E's fundamental 11 dB weaker against its 2nd-4th harmonics (the pickup
    hears the string's velocity); FluidR3's is a boomier, P-like or tone-rolled-off sample. Ours
    brightens with velocity (the first 100 ms's centroid 141 / 154 / 173 Hz at v40 / 80 / 120),
    FluidR3's does not move.
  - Prepared in the module, not yet played ("svt" is GM 35's: Pick bass, below): "slap-di"
    (GM 37: the DI 75 % with the VPF scoop, a clean solid-state head's EQ into a 4x10 with a horn,
    an 1176) and "slap-squash" (GM 38: a stomp compressor, a SansAmp-like drive on the DI, the 1176
    at 8:1). `tools/bassamp.cc` checks the module's bass blocks on their own.
- **The bassist:** each note to a string and fret near the hand, low on the neck, open strings
  welcome; index and middle fingers in turn while notes come within 0.4 s (the middle a little
  deeper, a few mm further along), rest strokes ~10 cm from the saddle between the pickups (a hard
  one nearer the bridge; the brightness knob moves the hand), the calloused pad 10 mm wide slipping
  off in about a millisecond, a hard stroke snapping off the fingertip's firm edge (at v127 a
  0.6 ms slip and a 1.7 ms rise: +5 to +10 dB above 1 kHz in the attack; `PJ_EBASS_SNAP`, 0.4,
  0 as before); velocity the depth (0.4-5 mm). A line is one voice: a new note on
  another string stops the last even if its key is still down (MIDI's legato overlaps), unless they
  came together; a released note is stopped; the strings not played are kept muted. Pitch bend
  bends the last fretted note. Notes below E1 or above D#4 are moved in by octaves.
- Not yet: slap (37, 38; the pick is GM 35's, below), fret buzz, slides, hammer-ons, ghost notes
  as their own stroke; bass lines written on guitar programs (louisiana's channel 6) still go to the
  guitar. The bassist's velocity is narrower than a sample set's (E1 v40 to v120: 10 dB at the DI,
  6 after the LA-2A; FluidR3 16): `-play`'s velocity gain makes up the difference. 2.2 % of a core
  with the amp (the bare DI 1.5 %).

### Pick bass

`-inst pick-bass` (GM 35, Electric Bass (pick); `research/pick_bass.md`, `src/ebass.cc`,
`src/bassspec.cc`): a Fender Precision played with a pick into an Ampeg SVT, as 1970s rock records
have it. The finger bass's strings, neck and fretting hand; a new pickup, a new right hand and the
amp module's "svt".
- **The Precision** (`precisionBass()`): one split coil, the E/A half 149 mm from the saddle and the
  D/G half 124 mm, each string under one half; 6.4 H, 10.5 kΩ, 30 pF, wound hotter than a J coil
  (`PJ_PBASS_VOLTS`); its field along the string a Gaussian of 5 mm (`PJ_PBASS_WIDTH`: what a pick's
  top keeps above 3 kHz); one 250 k volume and a 250 k tone with 0.047 µF (`PJ_PBASS_TONE`, 1 open:
  the R-C branch as a parallel R and C at 1.2 kHz); with the 700 pF cable a resonance near 2.3 kHz.
  Picked over the E/A coil the open E loses its 6th harmonic (the pick's comb and the coil's at
  once). `PJ_PBASS_FLATS=1`: flatwounds (the overtones' loss x2.5, `PJ_PBASS_FLATLOSS`; B x1.2;
  10 % more tension): the open E's decay from 4.5 to 6 dB/s, the attack unchanged.
- **The pick** (`Guitar::pickContact`, new): not a force but a place. The hand carries the pick's
  tip aside along a raised cosine; the tip is a spring (6 kN/m for a 1 mm pick, `PJ_PICK_MM`: ~t^2.6)
  against the string where it is now, pushing only; at the end of the push (or when a thin pick has
  bent 1.5 mm) the string slips off the edge in 0.1-0.3 ms. A slow push and a quick slip: the
  string leaves near its static shape (its fundamental as strong as a finger pulling as far, where
  `Guitar::pick`'s 1 ms force pulse gave 15 dB less) and leaves it sharply (+19 dB above 2 kHz over
  GM 34's finger). The slip is the brightness, the push the level: halving the slip adds 4 dB above
  2 kHz and moves 40-100 Hz by 0.1 dB. It passes: a string still ringing keeps what it had, and a
  pick meeting it mid-swing catches it early or late. Stable over 1000 random picks (2-20 kN/m,
  0.3-4 mm, 4-27 cm).
- **The bassist** (`PickBassPlayer`, on the finger bass's `BassLinePlayer`: the same string and fret
  choice, one voice, double stops, note-off damping, bends):
  - velocity the depth, 0.2 + 3.2 mm·(v/127)^1.8 (v40 0.6, v80 1.6, v120 3.1 mm; `PJ_PICK_VELEXP`),
    and the speed: a push of 12 ms at pp to 3 ms at ff (`PJ_PICK_PUSHPP`, `PJ_PICK_PUSHFF`), the
    slip with the hand's speed (0.3 ms at pp, under 0.1 at ff). At the DI E1 v40 to v120 is 16 dB
    and its attack 24 % brighter;
  - 10 cm from the saddle, over the coil's bridge end, ±6 mm a stroke (`PJ_PICK_PLACE`; the
    brightness knob moves it);
  - down strokes (along the body and a little into it), and up strokes between them while notes
    come within 0.3 s (along it and a little out: 30 % deeper, `PJ_PICK_UP`, and still 2.4 dB
    weaker); `PJ_PICK_DOWN=1` downstrokes only;
  - the palm (`PJ_PICK_MUTE` 0..1: up to depth 8 at 12 mm, lifted a little on accents): the open
    E's 4th harmonic gone in 0.11 s, its fundamental thudding on to -30 dB at 0.46 s. Carol Kaye's
    felt (`PJ_PICK_FELT=1`, depth 2 at 10 mm under all four strings): 20 dB/s;
  - `PJ_PICK_CHORDS=1`: strings ring into each other (drones, Lemmy's chords).
- **The amp:** "svt" (`research/bass_amps.md` 10.2; `src/guitaramp.cc`): the SVT's 12AU7-like and
  12AX7 stages with its James stack, +3 dB at 800 Hz, six 6550s at 4x into an 8x10 with a D112,
  the DI 35 % lined up, an 1176 at 4:1 (its threshold now 0.04 V: a mf line hardly touched, a ff one
  ~2.5 dB; E1 v40 to v120 11 dB after it). The hardest attack (E1 v127) is at 4 % distortion in its
  first 100 ms, a mf note under 1 %: the preamp's triodes, not the power stage.
- **Against FluidR3's Picked Bass** (a real P-like bass sampled string by string): E1 v120, the first
  100 ms in octave bands from 20-60 Hz to 4-8 kHz, ours -19 -2 0 -10 -19 -21 -30 -66, FluidR3's -9
  -4 0 -14 -12 -21 -29 -42. Alike from 120 Hz to 4 kHz (the pick's bark at 1-4 kHz within 1 dB);
  above 4 kHz ours is far darker (the P's coil resonance into the cable, the 8x10); A2's centroid
  344 Hz (FluidR3 373). Decays 1.3-4.1 dB/s on E1-A3 (FluidR3 1.9-5.4; the open A1 rings longest, its fall slowed by the 1176). FluidR3's samples change
  only in level with velocity; ours brightens.
- Not yet: fret clank on hard strokes (the string against the frets: much of a real pick bass's
  growl), the Rickenbacker and the StingRay, the "svt-flat" session chain, XG's muted pick bass on
  bank 28 (the files' bank select is not passed on), a click of the pick itself (through a pickup
  there is none to speak of). 0.6 % of a core for the strings and the DI, 2.4 % with the SVT at 4x.

`-inst slap-bass-1` (GM 37, Slap Bass 1) and `-inst slap-bass-2` (GM 38, Slap Bass 2;
`research/slap_bass_1.md`, `research/slap_bass_2.md`, `src/ebass.cc` `SlapBassPlayer`, `src/guitar.cc`
the frets and the thumb, `tools/slapfret.cc`): the finger bass's Jazz Bass slapped and popped, its
frets struck. 37 thumb-led and passive, 38 pop-forward, active, on stainless strings.
- **The frets as obstacles** (`GuitarSpec::fretContact`, off for every other guitar): 20 crowns on a
  plane with 0.3 mm of relief (a parabola, `PJ_SLAP_RELIEF`), each its own ±0.03 mm; the open string
  0.5 mm over the first fret and 2.4 mm over the 17th on the E (a little less towards the G,
  `PJ_SLAP_ACTION`); a fretted string runs from its fret's crown to the saddle (~0.2 mm over the
  next fret). Each sample the string's displacement towards the board at each crown over the
  sounding part (each mode's shape there through its askew axes); one that has passed a crown has
  its speed into it reflected, 0.6 of it given back (`PJ_SLAP_BOUNCE`: the modes' masses are equal,
  so the reflection keeps or takes energy, never gives it; stopping it dead instead drained a pop of
  70 % of its energy in 50 ms in 128 modes, and 1.0 came out three times too bright against
  FluidR3's recorded slaps), and what is deeper than 0.01 mm is moved back by the least stretch
  (along the shape over ω²). Two sweeps. The gate: while a stroke is on, or just after a contact,
  every crown every sample; otherwise only the crowns each mode's swing could reach together, found
  every 16 samples, the modes above the 32nd first bounded. The modes run to 10 kHz
  (`PJ_SLAP_TOP`), a stopped string's losing as the open one's at the same frequency.
  `PJ_SLAP_FRETS=0`: no frets. Measured: a thumb on the open E meets the 20th fret 2 ms in and is
  pinned there for most of its contact; a popped open G (3.6 mm at 22 cm, Issanchou's case) meets
  the frets 3.9 ms after its release, at 10 frets, dense until 43 ms (Issanchou: 0-90); a 1.2 mm
  pop touches nothing; with the frets the thumbed E1's 30-100 ms centroid is 316 Hz against 88
  without, 2-4 kHz +18 dB. 2000 random strokes stay finite; with the losses off the energy never
  rises at a contact.
- **The thumb** (`Guitar::thump`, new): a 20 g mass on a 3·10⁴ N/m flesh spring over 15 mm, 2 mm
  off the string at 0.265 m from the saddle (`PJ_SLAP_THUMBAT`; the brightness knob moves it, ±8 mm
  a stroke), at 1.3 rad into the body (`PJ_SLAP_THUMBANGLE`); from 2 ms after it touches the
  forearm pulls it back at 30 m/s² (a rebound stroke; a slow heavy note on the E/A/D a rest stroke
  that ends on the next string and damps it). Speed 0.4 + 2.9·(v/127)^1.4 m/s (`PJ_SLAP_THUMBV`;
  ±7 %), a little more on the thinner strings. Thumbed notes under 110 ms apart alternate down and
  up (double thumbing: the nail's side, 8 mm, 65 % of the speed, out of the body).
- **The pop** (`Guitar::pop`, new: `pluckPad` with a hook up to 30 ms): the finger hooks under the
  D or G string 18-23 cm from the saddle (6 mm wide; 38 3 mm and nearer the bridge), pulls it out
  of the body 0.5 + 6·(v/127)^1.6 mm (38: 0.6 + 6.5·(v/127)^1.6; up to 6.5-7 mm), over 12-25 ms
  (slower when soft), and lets it slip off in 0.15-0.35 ms; it slams into the frets. A double stop:
  the second finger 3-8 ms later. From a file every slap-bass key comes 90 ms early (`noteLead`,
  `PJ_SLAP_LEAD`): the hook begins early and the snap lands on the beat (measured -2 ms against
  the thumbs; live 12-13 ms late, as on a bass).
- **The slapper** (`SlapBassPlayer`, on `BassLinePlayer`'s fretting hand, one voice): each key is
  decided 30 ms before its stroke may begin, having seen its own release and the next key when the
  file sent them early:
  - a ghost: velocity under 40 (`PJ_SLAP_GHOST`), or under 55 and shorter than 90 ms (38: only the
    soft ones). The fretting hand lies on the string unpressed (a mute of 200/s where it would have
    fretted, `PJ_SLAP_GHOSTMUTE`) and the stroke falls all the same: a pitchless click off the frets;
  - a hammer-on or pull-off: 1-3 semitones on the string still held, under 0.2 s after, not
    accented: no stroke; the finger comes down (a 5 g knock) or leaves (a small sideways pluck);
  - a pop (only on the D and G strings; the string choice is pushed there): 37 where the line leaps
    9-16 semitones over its last thumbed note within 1 s, on an accented note an octave over the
    lowest of late, the notes of a double stop on D and G, and from E4 up; 38 everything from D2 up
    (the hand up the neck to keep it on D and G), except a note an octave or tenth under the pop
    that follows it;
  - otherwise the thumb.
  Counted on the user's files: louisiana.mid (37) 13 % popped, 12 % ghosts; Gaviota trio 2 (37,
  velocities flat at ~110) 6 % popped; grelots.mid on 38 66 % popped, every thumbed note under D2
  (on 37: 9 %). A released note is damped at 60/s (`PJ_SLAP_OFFDAMP`): -63 dB 0.12 s after a short
  note's release. `PJ_SLAP_COUNT=1` prints the counts, `PJ_SLAP_LOG=1` every stroke and every fret
  contact; `PJ_SLAP_FORCE=1/2` thumbs or pops everything (for measuring).
- **GM 38's bass**: active: a buffer at the coils (the two volumes' 125 k across them, the wiring's
  ~20 pF: the J pair's resonance from 4.4 kHz to ~8.8, broad; `PJ_SLAP2_POTS`, `PJ_SLAP2_WIRING`;
  `PJ_SLAP2_ACTIVE=0` passive), its two shelves +4 dB at 4 kHz and +4 dB at 50 Hz
  (`PJ_SLAP2_TREBLE`, `PJ_SLAP2_BASS`; 37 has the same knobs at 0, `PJ_SLAP_TREBLE`/`_BASS`);
  stainless strings losing 15 % less above the fundamental (`PJ_SLAP2_LOSS1` 0.85; the research's
  0.65 and +6 dB came out 3-6 dB brighter at 0.3-0.8 s than every reference set).
- **The chain**: 37 "slap-di" (the DI 75 % with the VPF at noon, a GK-like head into a 4x10 and
  horn, an 1176 at 4:1, 0.8 ms / 150 ms: 3-4 dB of gain reduction on a v100 line); 38
  "slap-squash", revised after `research/slap_bass_2.md` 4: every SoundFont's 38 is brighter than
  its 37 and rings longer but is no more squashed, so the same chain with the DI at 80 % and the
  1176's attack at 1.5 ms (the snap kept), release 120 ms; the stomp compressor (`PJ_BAMP_STOMP`)
  and the drive (`PJ_BAMP_DRIVEMIX`) off unless asked. In -play the velocity is a gain on the voice
  as well (a GM curve; the frets cap how loud a slap rings: E1 v40 to v120 +14 dB at the DI, +10
  after the chain; a pop's own rise is ~5 dB).
- **Against the references:** thumb against pop on the same note (D2 G2 C3), the share above 2 kHz
  at 30-100 ms +4 / +10 / +5 dB for the pop (FluidR3's two recordings: +4..+9), the two within
  4.4 dB after the 1176 (K-weighted); 38 against 37 from E2 up: the first 100 ms 2.4-4.4 times as
  bright (the references 1.2-5.1), above 2 kHz at 0.3-0.8 s +4..+12 dB (refs +7..+15), more than
  in the first 30 ms (not an EQ), decaying as fast or faster, its E3 above 2 kHz +23 dB from v32 to
  v127 (Jnsgm2 +24). The thumb's first 10 ms are its loudest, but its crest factor is ~19-22 dB
  against the samples' 11-16.
- GM 34 is untouched (bit for bit); `PJ_EBASS_FRETS=1` gives the finger bass the same frets: no
  contact at v40, a light buzz from v80 (2-4 kHz up 15-30 dB from a very low floor, the first
  100 ms's centroid +3 to +50 %).
- Not yet: a left-hand slap, slides, the StingRay (`PJ_SLAP2_BASS=stingray` of the research), the
  thumb's measured mass and speed (assumed); fretted notes buzz on the next fret for 0.2-0.3 s
  after a hard stroke. ~2 % of a core for the strings and the frets (2.1× the finger bass), 3 %
  with the chain.

### Fretless bass

`-inst fretless-bass` (GM 36, Fretless Bass; `research/fretless_bass.md`, `src/fretless.cc`,
`src/fretlessspec.cc`, `src/band.h`, `tools/fretlessboard.cc`): the finger bass's Jazz Bass with its
frets pulled, played by a fretless player who tunes by ear to the band.
- **The board under the finger** (`GuitarSpec::board`, off for every other guitar): behind the
  stopping finger the string rises from the board to 2.5 mm over its end (27 cm from the saddle;
  `PJ_FRETLESS_ACTION` x): 7-20 mrad. Five points 3-36 mm behind the finger, towards the board only
  (each mode's shape there through its askew axes), each a one-sided spring and damper (the double
  bass's 3e4 N/m and 1.5 N s/m, `PJ_FRETLESS_BOARDK`, `_BOARDC`, its step limiter), awake for 1.5 s
  after a stroke or a contact and, within that, only where the modes' swings could reach. A hard
  stroke lays the string on the board next to the finger for part of every period: energy moves
  from the low partials into 0.3-1 kHz. A stopped A1 at v100: partials 10-18 peak 80-400 ms after
  the pluck (the fretted J's all at the attack), that band 4-8 dB over the fretted note's at
  0.15-0.35 s; the board touched in ~5 % of the samples, the last at 0.85 s. At v30 it never
  touches; open strings never (they end on the nut); action x0.6 grazes to 1.15 s, x1.4 to 0.7 s.
  48 and 96 kHz agree within 0.3 dB.
- **The stroke is round** (the fingertip's pad 18 mm, rolling off in 3-5 ms, no snap:
  `PJ_FRETLESS_PAD`, `_SLIP`, `_SNAP`, `_ANGLE` 0.65 rad into the board): the stroke gives the low
  partials and the board the upper ones a moment later. A bright, snapping stroke (the J player's)
  leaves the board nothing to add: measured, the mwah then only takes energy away.
- **The finger** (`Guitar::stop`, any guitar's; `fret()` is unchanged, bit for bit): the stopping
  place in fractional semitones, each control block; small moves turn the modes' frequencies (a
  rotation by a small angle), every 4 c everything is placed anew. A slow change of length keeps
  each mode's action (E / ω, amplitude constant: a lossless slide of 5 semitones keeps it to
  0.00 dB). The fingertip's loss: half the double bass's (`PJ_FRETLESS_FINGER`) instead of a fret's
  x1.15: above 1 kHz a stopped note dies ~3x as fast as fretted, the fundamental 4.8 dB/s against
  3.4 (FluidR3's recorded fretless -4 to -7).
- **The pickups and the chain:** the bridge pickup full, the neck at half (`PJ_FRETLESS_PICKUP`); the
  amp module's "fretless-di": the DI 70 % and the B-15 30 % (`PJ_FRETLESS_BLEND`), both with the
  Acoustic 360's mid push (+3 dB at 800 Hz, `PJ_FRETLESS_MIDDB`), the LA-2A at 3:1.
  `PJ_FRETLESS_CHORUS` 0..1: a bucket-brigade chorus (7 ± 2 ms at 0.6 Hz, the lows under 150 Hz
  dry, stereo by the inverted sweep), off as on a Sound Canvas. `PJ_FRETLESS_FLATS=1` flatwounds.
- **The player** (`FretlessBassPlayer`): every stopped note is the stopping finger's place in cents:
  - the landing: a human error (sd 8 c, `PJ_FRETLESS_ERR`; the player's bias 0 ± 3 c) and, on
    slower notes at v80 and up, now and then a scoop from 15-35 c flat (`PJ_FRETLESS_SCOOP`: 0
    none, 2 every stopped note): the finger lands flat, crosses the note at ~55 ms, overshoots
    ~12 c and is within 2 c by 300 ms (the Recardo file's sequenced scoop); no scoops of its own
    once the file bends;
  - the correction, a second-order approach; a residue of ± (2 + 0.3 sd) c left (halved, and 2 c
    lower, under the vibrato);
  - the vibrato from 0.2-0.35 s on a held stopped note, in over 0.2 s: 4.6-5.4 Hz, ± 10 c, wider
    up the neck (`PJ_FRETLESS_VIB`); the mod wheel adds up to ± 25 c, the director's cue scales it;
  - slides: a new note while the last one's key is held over it for 40 ms or more, at most 4
    semitones away on the same string: no pluck, the finger glides 60 ms + 35 ms a semitone (a
    raised cosine; 1-2 semitones 60 % of the time, 3-4 30 %; `PJ_FRETLESS_SLIDES` 0 never, 2
    always; never in a jazz trio's file). The keys are acted on 60 ms after they come
    (`PJ_FRETLESS_LEAD`; `-play` moves them that much earlier) so the player sees the overlap;
  - the pitch wheel moves the finger (± 2 semitones); open strings only in quick lines (they cannot
    sing), never once the wheel is in use; notes above the board (D#4 up) as natural harmonics
    where one fits within 20 c, else an octave down (`PJ_FRETLESS_HARMONICS=1`: from C4 up wherever
    they fit). `PJ_FRETLESS_LOG` 1: each note's decisions and the board's contacts; 2: the finger.
- **Tuned by ear to the band** (`-play`): `src/band.h`'s registry of what the other channels sound
  (kept by the event loop between blocks: a note-on adds, a note-off takes away unless the
  channel's sustain pedal keeps it falling at its family's rate; a harp's or a bell's rings on;
  each family a spectrum template; `PJ_BAND_LOG=1` prints it). Each block the fretless solves its
  one note against it by the choir's spectral rule (its own partials, its string's stiffness
  included, against each note's template; pairs within 36 c pulling by the weaker level over the
  higher harmonic number squared), followed by its weight w as w / (w + 0.0005), within ± 15 c
  (`PJ_FRETLESS_JI_LIM`; `PJ_FRETLESS_JI=0` off). C2 under a piano's E3: +10 c, its 5th partial
  beating 0.2 Hz against the piano's 2nd; under C3 E3 G3 -0.9 c; G1 under B2 F3 +9 c. A chord
  changing over a held note is followed at most 1 c/ms (no change under 25 ms), changes under 2 c
  let be; with nothing to hear, back to equal temperament in ~3 s. The fretless writes its tuning
  back (`Performer::tuning`); the winds (brass and reeds) tune by the same rule and hear it
  (MIDI files, below).
- **GM 36 in a file:** the fretless, except in a jazz trio's file (a piano and GM 33 or 36: the
  user's bushgrafts files, where GM 36 is the upright): there the double bass, as before.
  `-reinstrument double-bass=fretless-bass` forces it (then without slides).
- Not yet: the mwah high on the neck (from the 12th position the steeper angle keeps a v120 G3 off
  the board, where FluidR3's G3 blooms most), the slide's squeak, finger buzz on a light touch, the
  second polarisation's friction on the board, a velocity table for `-play`, the organ, choir and
  flutes tuning to the fretless. About 1.2x the finger bass's CPU with the chorus.

### Concert harp

`-inst harp` (GM 47, Orchestral Harp; `research/harp.md`, `src/harp.cc`, `src/harpinst.cc`,
`tools/harpboard.py`): a pedal harp of 47 strings, C1–G7; notes outside are moved in by octaves.

- **Strings:** the Elysian's lengths and gut gauges, Camac's wires and Galli's tensions: nylon at
  the top (F6–G7), gut in the middle (A2–E6), steel wound with silvered copper below (C1–G2), all
  tuned flat on their full length (the pedals up: C-flat major) but the three without discs (C1,
  D1, G7). Each string is its modes up to 8 kHz (the wires 6 kHz) in two directions: in the
  strings' plane, where it pulls on the board at 35°, and across it, which only the board's
  turning under the eyelet takes (10–15 dB less). Across, each partial lies 0.05–0.3 % above the
  other, so a string plucked between the two swings in an ellipse that collapses and swells. It
  loses to the air (Stokes) and in its material as it bends (gut and nylon: some ten partials
  live), to its ends (`PJ_HARP_NECKR`, 700 kg/s: fitted, with the board, to the one string
  measured, C-flat 3, 10 s), and to the board. A swinging string is tighter: a loud note starts a
  few cents sharp and settles, and the tension's swing pulls on the board along the string, at
  twice each partial and their sums (the "phantom partials" that fill in what a pluck in the
  middle leaves out). Gut is alive: each partial a few cents off, its loss ±25 %.
- **Pedals:** seven, one for each letter in every octave; the natural disc shortens a string by a
  semitone, the sharp disc by another, retuning its modes where they stand (a pedal moved under a
  sounding string bends it).
- **Board:** spruce, 1.42 m, 50 to 10 cm wide, 11.5 to 2.25 mm thick, the grain across, a string
  bar down its middle, on a grid of 1 cm, coupled with the 29 litres of air in the box breathing
  through its five back holes. Fitted to Le Carrou's measurements: T1 153.5 Hz and A0 172 Hz, the
  air two thirds of A0, 200 g on the board lowering them 6.3 and 4.0 Hz (measured 5 and 3.5);
  broad resonances above 2.4 kHz. Every string meets the board at its own point on the centre
  line: what one string gives the board every other feels, and those whose partials meet ring
  along (the halo). The sound: the board in 5 cm patches and the holes' air, each heard after its
  own distance by two microphones 2 m in front. The strings at T1 and A0 (D3–F3) are the loud,
  short ones; the lowest wires are heard by their overtones.
- **The pluck:** the finger pad (2 cm) takes the string in the middle (0.37–0.42 of its length
  from the board), draws it 0.5 mm × 18^(velocity/127) aside (a string shorter than 17.6 cm less),
  mostly in the strings' plane (the thumb 17° off it, the fingers 20–26°, each pluck its own),
  and lets it slip in 2–3.5 ms, carrying it 0.3–1.5 mm further: it leaves with both displacement
  and speed. A finger landing on a sounding string stops it first. The brightness control from
  1.6 plucks près de la table (0.06–0.12 of the length: brittle, guitar-like).
- **The harpist:** each pitch on the string that already gives it, else on the letter whose
  pedal moving disturbs least (not one whose strings ring or were just played): E sharp or F, B
  sharp or C, as the pedals stand; the pedals set to the song's key first (`PJ_HARP_PEDALS`, C to
  B as `b`, `n` or `#`: the harpist's own setting). A ringing string whose pedal must move is
  damped first. The left hand takes the bass, the right the treble, four fingers each (the thumb on
  the highest note). Chords of three or more notes are rolled up from the bass, 18–30 ms from
  string to string (louder, quicker; `PJ_HARP_ROLL`), unless they come under 0.25 s apart; two
  notes together. Single notes under 60 ms apart, each 1–4 semitones on, are a glissando from the
  third: the finger slides across without stopping, every string between two notes of the run
  sounds, and the strings it passes over are set, if their pedals are free, to pitches of the
  run (a diminished-seventh glissando sounds four pitches to the octave on every string). A lone
  note at velocity 20 or less, between E3 and A6, is an octave harmonic of the string below (the
  side of the hand at its middle, the thumb plucking; `PJ_HARP_HARMVEL`).
- **Damping:** the strings ring on when keys are let go. The palms damp the strings below C5 at a
  rest (no key held for a beat; `PJ_HARP_REST`) and every string at the song's end; the left hand
  stops the bass strings it played that are not in the new harmony when the bass moves; a note let
  go within 0.1 s of its pluck, not in a quick passage, is étouffé. The sustain pedal: laissez
  vibrer, no damping. `PJ_HARP_VERBOSE=1` prints every pluck (and its pedal), every glissando.
- Measured (`tools/test.py`): in tune within 4 cents C1–G7 once a loud pluck has settled; decays
  C2 11 s, A2 9, C4 7, A4 4, C6 2.5, C7 and G7 0.9 s (research's 10–20, 10–15, 7–9, 5–6, 2–3,
  0.5–1); 33 dB from velocity 16 to 127 (FluidR3 30). Against FluidR3's Orchestral Harp (a sample
  set, each zone levelled): as even from E1 to B5 and as much quieter at the top (FluidR3 15–18 dB
  under its middle from B5 up; here 8–24 dB from A6 up), but the T1/A0 strings (D3–F3) up to 14 dB
  over the middle and C1–D1 10–18 dB under it (FluidR3: within 5 dB); brighter in the middle
  register (C4–G5: the second to fourth partials 8–37 dB under the fundamental, FluidR3's 21–64);
  longer in the bass and middle (A2 10 s, C4 8 s against 5 and 3.4: the research has 10–15 and
  7–9) and shorter at the top (C7 0.9 s against 3.4: the research has 0.5–1).
  Not yet: pedal glissandi, xylophonic sounds, nails, bisbigliando as a written tremolo, disc
  buzz, the box's standing air waves above 250 Hz, measured directivity. About 17 % of a core with
  every string ringing (the strings are always computed while the harp sounds: the halo).

      ./pajupilli -inst harp -seq "on 36 80 on 48 80 on 55 80 on 64 80 on 72 80 wait 2 on 72 15 wait 1.5"

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
    regulated to an even line (it rose ~27 dB to C7). The bass's wind nearer the tongue's closing,
    0.95 of it below 100 Hz (0.9 from 200 Hz; `PJ_REED_BASSP`, 0.05): at 0.9 a 16′ reed's heavy
    tongue grew from rest over 150–300 ms, a short pedal note over before the Posaune spoke; now it
    reaches −6 dB within ~40–100 ms, its narrower gap's lesser wind made up in its level (0.97 and
    more choked it on a long note). The Clairon breaks back at c‴ (key 84): its top
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

- **Bent round the edges, not cut off:** a path past the gallery's front edge, another case's
  corner or top, or out of its own pedal tower through the side is diffracted. It is not dropped.
  The model is Kurze-Anderson's form of Maekawa's chart at the pipe's own fundamental:
  - N = 2 δ / λ, where δ is the detour over the edge;
  - in the shadow, 5 + 20 log10(x / tanh x) dB, with x = √(2πN);
  - on the open side within N = 0.2, the lit side's fading loss (one edge's per leg);
  - its harmonics darker with the loss; beyond 30 dB, blocked.

  A cut path made the pedal erratic: a Posaune 16' resonator or a Principalbass 16' top above
  the gallery's line of sight had a full direct path, one just below it none. So the long pipes
  came out loud and dry, the shorter ones buried in the reverberation (the early sound's level
  over a pedal scale spread 27 dB, now 9; the path gains glide from key to key, steps under
  2 dB). `PJ_ORGAN_TAPLOG=1` prints each pedal pipe's path gains.
- **The direct sound 4 dB over the physical balance** (`PJ_ORGAN_DIRECT_DB`, 4): clearer against
  the church, less muffled. Overall it is only ~0.2 dB louder, the reflections carrying most of
  the energy.

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
it: on the Hauptwerk 0.6, elsewhere 0.35). That phrase's last note is also lifted early, the
organist's breath before the next phrase: a fifth of the note, 80 .. 250 ms (the romantic's legato
0.7 of it; `PJ_ORGAN_BREATH` scales it). The line's last note (the piece's end) is held to its
length, and so is a note before a new part (a section of the plan, at a tempo change or a general
pause, begins before the line's next note). The bass is played ahead, as organists anticipate their big pipes'
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
- **Bandoneón:** two reeds an octave apart on each note (8' + 4', the 4' tuned a pure octave to its
  8' at ~1 kPa), no cassotto or musette, thicker plates, stiffer tongues (they still speak at
  fortissimo), and a harder arm (tango's marcato). Its reeds sound through the wooden reed blocks and
  the end-plate: one pole at 500 Hz (`PJ_BN_BLOCK`), a darker voice than the accordion's.
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
    3 dB a step. At most ~6 dB (`PJ_HAMMOND_BALANCEGAP`): an accompaniment written much
    softer (say 11 dB) would be left a whisper under the tune, so past that it is set apart by
    colour instead: the hollow 8' (008060400's shape: the 8' with its odd harmonics, the
    clarinet) at 6 dB under, its vibrato off under the tune's C3, as a trio organist sets two lines
    apart. E.g. a quiet line in the tune's register under a 4' melody: 005030100 instead of
    102300000. The expression pedal follows only the tune's channels (CC11), the
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
- **Live, the MPK's knobs (K1-K6):**
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
  - **K5 is the overdrive:** the Leslie 122's amplifier driven harder, from as registered (left)
    to 8 x (right): its valves' grit, the level kept near (the amp's output falls as its drive^0.3).
  - **K6 is the Leslie's half-moon:** stop (brake) to the left, slow in the middle, fast to the
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

### FX 1 (rain)

`-inst fx-rain` (GM 97), A0 to C8 (`src/rain.*`; research in `research/fx_sounds.md`): rain
falling on tuned glass bowls. The classic patch ("Ice Rain") is a pad under a glassy ping
re-struck from a short sample loop; here the rain itself strikes the glass.

- **The bowls:** each key sets out four thin glass bowls, on its note, the octave, the twelfth
  and two octaves (K4: one to five). Each rings in its bending modes n = 2..5 (1, 2.83, 5.42,
  8.77 × its pitch; the wall thinned, as a handbell's, to put n = 3 on 3.0, so it does not lie
  a semitone under the 3× bowl). Each mode is a doublet split a little by the glass's flaws,
  so it beats slowly. T60 is ~4 s at middle C, shorter up the keyboard. A smaller bowl catches
  fewer drops.
- **The rain:** drops arrive at random. Their sizes follow Marshall and Palmer
  (N(D) ~ exp(−4.1 R^−0.21 D)), weighted by their fall speed, the largest growing with the
  rate. Velocity is the rain rate, 0.3 to 90 mm/h: a drizzle of small drops to a downpour with
  large ones. The wind's gusts (±35 %, over seconds) move the rate for every key alike.
- **A drop on the glass** gives its momentum (D³v) to the modes, through a pulse lasting D/v
  (0.25–0.5 ms, so large drops are duller). Where it lands decides which modes ring: the angle
  splits each doublet, the height on the wall favours the low modes. The impact also ticks.
- **A drop into the water** pushes the bowl weakly. A 0.8–1.2 mm drop always traps a bubble
  (13–21 kHz); a drop over 2 mm often traps a larger one (1–10 kHz). Each bubble is a van den
  Doel bubble (Minnaert's pitch, damped, chirping up), as in `seashore`.

The large drops are the plinks; the many small ones keep the bowls ringing between them, a
soft bed of the chord. On the key, a single large drip strikes the note's bowl near its rim,
sized by the touch. On release the rain on those bowls stops and a hand damps them (≤ 1.5 s).
Knobs (live's K1–K4, `-seq`'s `knob K X`; their starting places `PJ_RAIN_DENSITY`,
`PJ_RAIN_WATER`, `PJ_RAIN_RING`, `PJ_RAIN_BOWLS`):

| Knob | Sets |
|---|---|
| K1 | the drops' density on the note's bowl, 25 to 800 a second (default 140) |
| K2 | the water: bubbles and ticks, 0 to 2× (default 1×) |
| K3 | the bowls' ring, × 0.25 to 4 (default 1) |
| K4 | bowls per key, 1 to 5 (default 4) |

The sustain pedal holds; pitch bend ±2 semitones.

Against FluidR3's Ice Rain (`tools/synthcompare.py profile`, with TimGM6mb and Jnsgm2 as the
references): the droplets here are on the key and random, where FluidR3's are a periodic ping
train (~14–24 Hz) that barely follows the key (20 c per key). The held level falls alike (~6 dB/s
at first). Here is noisier above 6 kHz (the bubbles), with a much sharper onset (a drop on
glass), 4 dB of key-to-key level scatter (each note its own rain, against their ~0.5 dB), and a
little darker when played harder (large drops strike longer).

### FX 2 (soundtrack)

`-inst fx-soundtrack` (GM 98), A0 to C8 (`src/soundtrack.*`; research in
`research/fx_sounds.md`): a keyed aeolian harp, strings sung by the wind. The classic patch is
Roland's D-50 "Soundtrack": a dark, chorused pad of the note and its fifth. Every GM set keeps
it as a string pad layered with a copy a fifth up.

- **The strings:** each key has a course of four nylon strings. Three are on the note, a few
  cents apart (they beat: the chorus), and one is on its fifth. Each string rings in up to six
  modes (T60 ~3.5 s at middle C, longer down the keyboard).
- **The wind:** a blower gives a steady wind, shared by every key (gusts ±0.3 %, over
  seconds). The key slides a shutter that lets the wind across its course: as far and as quickly
  as the touch, the whole travel in ~0.8 s at the hardest.
- **The wake:** vortices shed alternately off each string at the Strouhal frequency, which is
  tuned to the string's first mode. Each stretch of string has a wake oscillator (Facchinetti,
  de Langre and Biolley's van der Pol model: ε = 0.3, A = 12; its lift from the string's mass
  ratio, μ ≈ 750 for nylon in air), coupled both ways with the string. The wake locks onto the
  string's own mode (vortex-induced vibration), so the note is the string's pitch. The string
  blooms over ~1/(ζω), half a second or so, behind the shutter.
- **Dark to bright:** as the shutter slides it first uncovers the main stretch (the first
  mode). Then, one after another, it uncovers short side slots where the wind is 2 to 5 times
  faster, each across the antinode of the mode it locks. The tension's swing (∝ the swing²)
  also reaches the bridge along the string: the octave. A soft key sounds the fundamental and
  its fifth; a hard one adds the harmonics. The note opens over its first second and then
  settles a little. The slots hiss faintly.
- **Release:** the shutter closes, and its felt edge rests on the strings (T60 ≤ ~2.5 s at
  middle C).

Knobs (live's K1–K4, `-seq`'s `knob K X`; their starting places `PJ_AEOLIAN_GUSTS`,
`PJ_AEOLIAN_SWELL`, `PJ_AEOLIAN_FIFTH`, `PJ_AEOLIAN_RING`):

| Knob | Sets |
|---|---|
| K1 | the gusts, 0.04 to 2.4 % (default 0.3 %; a wake catches its lock only within ~±2 %) |
| K2 | the swell (the shutter's travel time), × 0.25 to 4 |
| K3 | the fifth's string, 0 to 2× (default 1×, ~6 dB under the note) |
| K4 | the strings' ring, × 0.25 to 4 |

The sustain pedal holds the keys. Pitch bend is ±2 semitones.

**Against FluidR3's Soundtrack** (`tools/synthcompare.py profile`, with TimGM6mb and Jnsgm2
as the references):

- **What is alike:** the partials up to the fifth harmonic at velocity 100, within ~5 dB. The
  fifth sits ~6 dB under the note (FluidR3's is 10 dB under; TimGM6mb and Jnsgm2 put it 4 dB
  under). The mean spectrum is within ±5 dB up to 6 kHz. The release is 0.85 s against 1.2 s.
- **Where it differs:**
  - It speaks later: an onset delay of ~0.8 s against 0.1–0.24 s, because the strings bloom.
  - It is purer and smoother (Plomp–Levelt).
  - Its partials move together.
  - Its velocity range is narrower on 2 s notes (it has not bloomed fully).
  - Key-to-key levels scatter by ~3 dB (each note's beating).
  - It brightens with the touch, where the samples do not.

### FX 4 (atmosphere)

`-inst fx-atmosphere` (GM 100), A0 to C8 (`src/atmosphere.*`; research in
`research/fx_sounds.md`): a keyed psaltery with magnetic resonators. The classic patch goes back
to the D-50's "Nylon Atmosphere": a plucked string with a pad behind it. FluidR3, TimGM6mb and
Jnsgm2 all layer a nylon guitar over synth strings. Here one physical system is both layers: a
plucked string that feedback keeps singing, as an EBow or McPherson's Magnetic Resonator Piano
does.

- **The pluck:** each key plucks a course of three steel strings, a few cents apart, with a soft
  leather plectrum about a quarter of the way along: a round, nylon-like pluck. The plectrum
  presses the string aside (3–8 ms, shorter when harder) and slips off it (0.1–0.4 ms; the
  quicker, the brighter). It meets the three strings a few milliseconds apart. Each string rings
  in up to 28 slightly stiff modes. Their losses grow as ω², so the upper partials die within a
  second (T60 ~9 s for the fundamental at middle C).
- **The magnets:** over each string a pickup senses its velocity. An amplifier, tuned to the note
  by a band-pass (Q 6), drives an electromagnet a little further along. The amplifier saturates
  (tanh), and the magnet's current lags it (its inductance). In phase with the velocity, the pull
  is a negative damping: the fundamental grows, or decays, until the saturated drive just pays
  for its losses (a limit cycle). The amplifier's limit, set by the touch, holds it ~7–9 dB
  under the pluck's peak. The saturated, square-ish current also feeds the third partial. The
  magnet pulls but never pushes: its force goes as the flux squared, which adds a little octave.
- **The pad:** the pluck's upper partials fade and the held fundamental stays. The magnet sits
  over the middle string; the side strings feel it weaker. So the course sustains on the middle
  string, and the others beat with it slowly: a chorus that breathes by 1–2 dB.
- **The octave (K2):** a second amplifier channel, tuned to the second partial, holds the octave
  too. It needs its own channel: through one saturating amplifier the fundamental starves it.
- **Release:** the magnets let go (20 ms), and a felt damper rests on the course (T60 ~2.4 s at
  middle C).
- **Voicing:** the top octave is lifted (its few, short modes). The bass's magnets are set a
  little weaker, since its plucked string rings long by itself.

Knobs (live's K1–K4, `-seq`'s `knob K X`; their starting places `PJ_ATMO_SUSTAIN`,
`PJ_ATMO_HARMONIC`, `PJ_ATMO_PLUCK`, `PJ_ATMO_CHORUS`). K1 and K2 also act on held notes.

| Knob | Sets |
|---|---|
| K1 | the sustain: the amplifiers' limit, 0 to 1.2× the pluck's fundamental (default 0.6×). At 0 it is a plain psaltery; turned up, the note swells after the pluck |
| K2 | the octave held, 0 to a quarter of the note (default an eighth) |
| K3 | the plectrum's hardness: its slip ×3 to ×⅓ (darker to brighter) |
| K4 | the chorus: the course's spread, 0 to 2× (default ±0.5–1.5 c) |

The sustain pedal holds the keys and their magnets. Pitch bend is ±2 semitones.

**Against FluidR3's Atmosphere** (`tools/synthcompare.py profile`, with TimGM6mb and Jnsgm2 as
the references):

- **What is alike:**
  - It speaks at once: an onset delay of 15 ms against 8–23 ms.
  - The level 2 s into a held note is −14 dB re the peak, against −17, −11 and −10 dB.
  - The release is 0.78 s against 0.63–1.04 s.
  - The velocity curve is within ~2 dB.
  - The overall brightness is within the references' range.
- **Where it differs:**
  - Its attack is sharper (17 dB/ms against 6.5–11). Its pluck is duller than its body by
    1.2 ERB, where FluidR3's nylon is brighter.
  - It is purer: the held note is a few locked partials, the samples' pads a detuned cluster.
    Its noisiness is −61 dB against −36 to −50, and its partials are more jagged.
  - It is wider in stereo (coherence 0.29 against 1). This comes mostly from the room.
  - Key-to-key levels scatter by ~3 dB (each course beats in its own way).
  - It brightens a little with the touch, where the samples do not.
  - It has no vibrato (4 c of wobble from the beating, against 14–30 c of LFO).
  - FluidR3's patch decays to silence (~7 dB/s); this one holds while the key is held, as
    TimGM6mb's and Jnsgm2's do. K1 at 0 gives FluidR3's decay.

### FX 5 (brightness)

`-inst fx-brightness` (GM 101), A0 to C8 (`src/brightness.*`; research in
`research/fx_sounds.md`): a keyboard of singing arcs. The classic patch (Roland's "Brightness",
"a Fairlight type of sound") is a bright, held tone whose octave is as strong as the note and
whose partials fall steadily, speaking at once. FluidR3 layers a chorused "Bright" sample over
synth strings; TimGM6mb and Jnsgm2 a "Solo Vox" over synth strings. Here the tone comes from
William Duddell's singing arc (1900), which Duddell played from a keyboard.

- **The arc:** a carbon arc fed from a DC supply through a ballast resistor (nearly a constant
  current), with a tuned circuit (a coil and a capacitor in series: the tank) across it. The
  arc's voltage falls as its current rises (a negative resistance), so it feeds the tank, which
  oscillates. The arc is Mayr's model: its conductance follows, with a thermal lag, what its own
  power would hold; its loss is a column (the share set by its length) plus the electrode falls.
- **The sound:** the arc is its own loudspeaker. Its power swings with the current, the hot
  column swells and shrinks, and the pressure goes as the rate of change of the power. Power is
  current times voltage, so its swing carries the note and, nearly as strongly, the octave. The
  arc's lag and the ballast round the swing into a limit cycle whose partials fall steadily: at
  the default touch the octave ~−1 dB, the third and fourth −10 to −14 dB, the tenth ~−30 dB.
- **Even over the keyboard:** every arc is the same circuit scaled to its note (the tank, the
  arc's time constant and its power go with the period), so the spectrum over f0 is the same
  from A0 to C8 (centroid 1.88–1.90 × f0 at C3, C4, C6).
- **Tuning:** the arc's own lag is a reactance in the loop: it sings at ~0.62 of its tank's
  frequency, depending on its length. Each tank is tuned so that its arc sings on the note, from
  a table made at start by running the circuit. Measured: within ±1.2 c from A0 to C8.
- **Two arcs a key:** a large one on the note and a small one (0.35 of its sound) a few cents off
  (K2), panned apart. They beat by a dB or so over seconds: the chorus of the sets' L/R samples.
- **The touch:** each arc burns silently while its key is up (a steady arc makes no sound). The
  key hands the arc's voltage to the tank through a slide-wire (~1 ms), so the tank rings at
  once, and a rheostat brings the current up (3–7 ms). Velocity is the current (the loudness)
  and the arc's length: a longer arc has a steeper characteristic and a fiercer, brighter limit
  cycle. Soft notes near the arc's threshold take ~20 ms to bloom in the treble.
- **Release:** the rheostat runs the current down (to −60 dB in 2 s by default), the arc keeping
  its shape, so the pitch and timbre hold as it fades.
- **The plasma flickers** (K3): the column's loss swings by a few per cent, low-passed at
  ~3 kHz: a faint hiss and roughness.
- **Integration:** at 4× the sample rate (the tank by the trapezoid, the arc's conductance
  relaxed exactly over a step), then the guitar amp's elliptic low-pass back down. What lies off
  the harmonics is 66–93 dB under the whole from C6 to C8: no aliasing.

Knobs (live's K1–K4, `-seq`'s `knob K X`; their starting places `PJ_ARC_LENGTH`,
`PJ_ARC_CHORUS`, `PJ_ARC_HISS`, `PJ_ARC_RELEASE`). K1 and K2 act on new notes.

| Knob | Sets |
|---|---|
| K1 | the arc's length (the brightness): the column's share ±0.15. At C4, velocity 100: the centroid 1.58 × f0 at 0, 1.88 at the default, 2.42 at 1 |
| K2 | the chorus: the small arc 0 to 12 c off the note (default ~4–6 c) |
| K3 | the flicker: 0 to 4 % RMS of the column's loss (default 1 %) |
| K4 | the release: 0.5 to 8 s to −60 dB (default 2 s) |

The sustain pedal holds the keys. Pitch bend is ±2 semitones. Eight notes cost ~1.3 µs a sample
(6 % of a core).

**Against FluidR3's Brightness** (`tools/synthcompare.py profile`, with TimGM6mb and Jnsgm2 as
the references):

- **What is alike:**
  - The brightness: the centroid at velocity 96 is 10 ERB (FluidR3 10, TimGM6mb 10, Jnsgm2
    11); the mean spectrum is within 2 dB of FluidR3's up to 6 kHz.
  - The partials: the fundamental's share 0.48 (FluidR3 0.52); odd against even −5.8 dB
    (−7 to −12).
  - It holds as FluidR3's does (−5.7 dB re the peak after 2 s, against −8.4; TimGM6mb and
    Jnsgm2 decay to −28).
  - The velocity curve (within 0.5 dB), the loudness range (16 dB against 16–17), the release
    (0.88 s for 30 dB, against 0.79–1.11 s) and the noisiness (−40 dB, as FluidR3).
- **Where it differs:**
  - It is smoother (Plomp-Levelt −4.3 against −1.8 to −2.8): its partials are exact
    harmonics, where the samples are detuned clusters with a 25–54 c LFO wobble (here 5 c).
  - Its onset is sharper (11 dB/ms against 5–8) and a little later (18 ms against 4–8, with
    the room): the tank is struck at once, and soft treble notes bloom.
  - It brightens a little with the touch (0.7 ERB from velocity 40 to 127; the samples do not).
  - Keys scatter by ~1.7 dB in level and ~1.2 ERB in brightness: the two arcs beat, each key in
    its own phase.
  - Above 6 kHz it is 4 dB darker than FluidR3.

### FX 6 (goblins)

`-inst fx-goblins` (GM 102), A0 to C8 (`src/goblins.*`; research in `research/fx_sounds.md`): a
pyrophone, a keyboard of singing flames. The classic patch ("Goblin" in GS and the three
SoundFonts, "Goblins" in XG) is a dark synth voice that swells in over most of a second and then
opens slowly: a low-pass driven by a modulation envelope with an ~9 s attack (from ~200 Hz in
FluidR3 and ~100–200 Hz in TimGM6mb and Jnsgm2, resonant there), with a 5–9 Hz warble in the upper
partials (TimGM6mb's and Jnsgm2's sitar layer has a six-semitone vibrato at 8.5 Hz, FluidR3's vox
layer a ±1-semitone growl at 33 Hz) and some wind. Here the tone comes from Frédéric Kastner's
pyrophone (1873): a glass pipe a key with a gas flame inside, which the player's keys controlled.

- **The pipe sings thermoacoustically:** each pipe is open at both ends, with its flame a little
  under a quarter of the way up. The flame's heat release follows the air's motion past it with a
  lag. When the lag is near a quarter period, the heat swells as the pressure does (Rayleigh's
  criterion), and the pipe sings.
- **The model:** the pipe's modes (its harmonics, damped by the walls and the radiation) are
  driven by the rate of the flame's heat release at its place. The flame sees the air's velocity
  there, a lag ago. The heat release follows Heckl's law for the Rijke tube, √|u0/3 + u|: it
  saturates where the flow past the flame reverses, so the oscillation settles into a limit cycle.
  The flame's response is a low-pass whose corner is set by the flame's length over the flow.
- **It swells in:** the gas lighting (a soft thump over 4 ms) and the flame's roar set the pipe
  ringing, and the oscillation grows out of them. At C4 it is within 12 dB of the held level by
  ~0.8 s, the bass by ~1.5 s.
- **It opens as the pipe warms** (K1, ~6 s by default), which drives three changes at once:
  - the draft grows, so the flame's lag shrinks to the quarter period (from 1.25 × it): the pitch
    rises a few cents into tune, most in the bass (A0 −4 c to +2 c);
  - the flame shortens in the faster flow, so its response opens from the note to 2.5–7 × it (by
    the flame's size), and the upper partials brighten: at C4 the centroid is 1.16 × f0 over
    0.5–1.5 s and 1.70 × f0 over 6–10 s, with the fifth partial up ~17 dB;
  - the walls stop quenching the flame's edges, so the gain grows and the limit cycle saturates
    further; the flow grows too, so the level swells ~6 dB from 1 s to 8 s.
- **It bubbles:** a buoyant flame puffs at a few Hz. Here that is a weakly self-excited oscillator
  kicked by the turbulence, 5–9 Hz over the keyboard (K2 its depth). It swings the heat and the
  flame's length, and with it the lag. The upper partials, whose phase through the lag moves
  most, warble the most: at C4 93–98 % of the level's modulation lies in 3–15 Hz, peaking at
  ~6.7 Hz.
- **The touch:** velocity is the gas valve, which sets the flame's size: its gain, its flow, its
  response's corner and its roar. A C-major chord goes from −28 dB at velocity 20 to −14 dB at
  127. Once warm, soft notes are darker: at 6–10 s the centroid is 1.44 × f0 at velocity 20,
  against 1.70 at 96.
- **Release:** the key closes the valve (K4), and the pipe falls silent once the flame's gain
  drops below its losses: 30 dB down in ~0.9 s at the default.
- **Tuning:** the lag pulls a pipe flat by cot(phase)/2Q. Each pipe is cut to sing in tune at
  40 % of its warmth. Warm, the pipes are within ±2.5 c from A0 to C8.

Knobs (live's K1–K4, `-seq`'s `knob K X`; their starting places `PJ_PYRO_WARMTH`,
`PJ_PYRO_PUFF`, `PJ_PYRO_ROAR`, `PJ_PYRO_VALVE`):

| Knob | Sets |
|---|---|
| K1 | the pipe's warming, 2 to 16 s (the swell's length; ~6 s by default). At C4, 2–4 s: the centroid 1.71 × f0 at 0, 1.20 at 1 |
| K2 | the flame's puffing (the bubbling's depth): 0 to 0.6 of the heat's swing (0.3 by default) |
| K3 | the flame's roar (combustion noise low-passed at ~700 Hz, through the pipe) |
| K4 | the valve's closing at the release, 0.1 to 3 s (~0.55 s by default) |

The sustain pedal holds the keys. Pitch bend is ±2 semitones. Eight notes cost ~0.8 µs a sample
(4 % of a core).

**Against FluidR3's Goblin** (`tools/synthcompare.py profile`, with TimGM6mb and Jnsgm2 as the
references; 2 s notes). TiMidity++ does not render the patches' slow filter envelope, so the
references here are the unswept samples.

- **What is alike:**
  - The velocity curve (within 1 dB) and the loudness range (16–17 dB, as all three).
  - The attack: −20 to −1 dB in ~0.5 s against 0.73–0.79 s.
  - The release: 0.91–0.93 s for 30 dB, against 0.61–0.88 s.
  - The pitch wobble: 14–16 c peak to peak, against 21–40 c.
  - The fundamental's share: 0.87–0.94, as TimGM6mb's and Jnsgm2's 0.93–0.94; FluidR3's is 0.37.
- **Where it differs:**
  - It speaks later: the onset delay is ~0.9 s against 0.54–0.61 s, as the oscillation grows.
  - Its attack is duller than the held note (−1.7 to −4.5 ERB, against −0.1 to +1.1): it opens
    over seconds.
  - It swells where the samples sag: −2 to −2.5 dB re the peak after 2 s, against −7.5 to −9.2.
  - It is brighter at 2.5–6 kHz (+9–10 dB re FluidR3, level removed): the pipe's upper partials.
    The centroid is 11–13 ERB against 7.4–9.4.
  - Odd and even partials are about even (−1 dB), where the vox samples favour the even ones
    (−17 to −23 dB).
  - It is noisier and rougher: spectral flatness −25 dB against −34 to −49, Plomp-Levelt −1.2 to
    −1.8 against −2.7 to −3.9. This comes from the bubbling's sidebands and the roar.
  - Keys scatter by ~3.8 dB in level and ~1.8–2.5 ERB in brightness. Each flame puffs in its own
    phase, and the bass is still swelling at 2 s.
  - In the first 2 s soft notes come out brighter than loud ones (−1.3 ERB from velocity 40 to
    127, against none in the samples): the lighting and the roar weigh more while a soft pipe is
    still growing.

### FX 7 (echoes)

`-inst fx-echoes` (GM 103), A0 to C8 (`src/echoes.*`; research in `research/fx_sounds.md`): a
dulcitone (a keyboard of tuning forks) played into a tape echo. The classic patch (Roland's "Echo
Drops", Yamaha's "Echoes") turns out not to echo at all: in the three SoundFonts it is a choir
"aah" and a near-sine synth voice held as a pad, whose looped samples swell and dip every
0.5–1.2 s (the loop's period, which moves with the key), with a 5 s release and a stereo pair in
FluidR3. Roland's is described as sweeping chorus-like tones that burst into "drops" at the key-up.
Here the echoes are real, made by the machine that made echoes in the 1970s (the Echoplex,
Roland's RE-201 Space Echo).

- **The forks:** each key strikes a tuning fork with a felt hammer; the fork's stem stands on a
  resonance box tuned to it, and a microphone picks it up. Three bending modes (1, 6.267, 17.55 ×
  the note), the felt stiffening as it is squeezed (Hertz, δ^2.5): a harder blow is a shorter
  contact, which brings out the fork's metallic "clang" (−44, −19, −17 dB re the note at C4 at
  velocities 40, 100, 127) and its octave (a loud fork's stem). The note rings for ~20 s at C4
  (T60) and the damper stops it in ~0.2 s at the key-up.
- **The tape:** one machine for all the keys. A tape loop runs past a record head and two
  playback heads 6 and 12 cm downstream; the second head is fed back to the record head. A short
  note comes back every 0.335 s at the default speed (head 1 to the left, head 2 to the right,
  so the repeats alternate sides), each pair of repeats ~8 dB under the one before (6 within
  30 dB at C5).
  - **Each pass is darker:** the playback head reads the tape through its spacing, a Poisson
    kernel along the tape (Wallace's exp(−2π d f / v)): the octave of C5 loses 1.5 dB a pass and
    the clang 7.7 dB. A slower or a worn tape is darker still.
  - **The tape saturates** softly (the AC bias's anhysteretic curve, a Langevin function): loud
    chords and piled-up repeats compress, and past unity loop gain (K2 near its top) the machine
    runs away into its own oscillation, held by the saturation.
  - **Wow and flutter:** the capstan, the pinch roller, the loop's splice and scrape flutter
    wobble the speed, so the repeats come back a few cents off and a held note beating against
    its own echo sweeps slowly, like a chorus. The motor has inertia: turning the rate glides
    the echoes already on the tape.
- **The touch:** velocity is the hammer's speed. A C-major triad goes from −43 dB at velocity 20
  to −21 dB at 127, brighter as it gets louder.
- **Tuning:** the forks are exact (within ±1.5 c from A0 to C8); the repeats wander by a few
  cents with the wow.

Knobs (live's K1–K4, `-seq`'s `knob K X`; their starting places `PJ_ECHO_RATE`,
`PJ_ECHO_INTENSITY`, `PJ_ECHO_VOLUME`, `PJ_ECHO_WEAR`):

| Knob | Sets |
|---|---|
| K1 | the tape's speed, 8 to 40 cm/s: the echo interval 0.75 to 0.15 s (0.335 s by default) |
| K2 | the intensity (the feedback, 1.2 × K2^1.5): at 0 only the two heads' repeats; 0.8 gives ~19 repeats within 30 dB; near 1 it runs away |
| K3 | the echo's volume (0 to 1.2 × the heads; 0.6 by default) |
| K4 | the tape's wear: the head spacing (3 to 11 µm) and the wow and flutter (0.2 to 3.4 × their default depth) |

The sustain pedal holds the dampers off. Pitch bend is ±2 semitones (the forks; what is on the
tape keeps its pitch). Eight notes cost ~0.35 µs a sample (1.7 % of a core).

**Against FluidR3's Echo Drops** (`tools/synthcompare.py profile`, with TimGM6mb and Jnsgm2 as
the references; 2 s notes, so the profile sees little of the echoes).

- **What is alike:** the velocity curve (within 1 dB) and the loudness range (15–16 dB against
  17), the brightness over the keyboard and at velocity 96, the pitch, the level after 2 s held
  (−9 to −10 dB re the peak against −6 to −10).
- **Where it differs:**
  - It is a near-sine where they are voices: the fundamental's share 0.99 against 0.76–0.94, no
    breath (spectral flatness −81 dB against −29 to −38), smoother (Plomp-Levelt −6.6 to −9
    against −2 to −3.6), dark above 2.5 kHz (−11 to −16 dB re FluidR3).
  - It decays (3 dB/s, a struck fork) where the samples hold (~0.4 dB/s).
  - It speaks at once (−20 to −1 dB in 80–130 ms against 0.2–0.5 s).
  - Its release is a train of repeats, 30 dB down after ~1.2–1.4 s, against FluidR3's 5 s fade.
  - In the hall a near-sine and its repeats cancel or add key by key: key-to-key level scatter
    3.9 dB (0.5 dry).
  - Its pitch wobble is smaller: 4–6 c (the wow) against 16–31 c.

### FX 8 (sci-fi)

`-inst fx-sci-fi` (GM 104), A0 to C8 (`src/scifi.*`): two voices on every key.

- **The discharge:** a sine falling onto the note from up to three octaves above (0.7 to 3 by
  the touch; 12 to 120 ms, slower in the bass), pushed into a sine wavefolder, sin(z sin φ + bias)
  (the bias, each note's own, gives even harmonics). The drive z decays, so the folded harmonics
  collapse inward onto the note until a plain sine dies away: an energy weapon's shot, a shield's
  impact. The folder's spectrum is FM's (J_m(z) at m × the sine), so it is band-limited by holding
  z under what the instantaneous frequency leaves below 20 kHz: no oversampling, no aliasing.
- **The ping:** a resonant state-variable band-pass at the note, at the edge of self-oscillation
  (it rings for seconds while held, 0.35 s at most once released), struck by a 1 ms Hann impulse
  (shorter above 450 Hz; scaled by its spectrum at the note, so every key rings alike). Its
  cutoff is swung at audio rate by a sine at a ratio of the note. A low ratio is a sonar's pure
  ping, a ratio of 1 a pitched telemetry beep, a little off an integer crystalline. The ratio
  drifts 6 % per three octaves away from middle C: the middle beeps, the ends glitter.

Velocity sets the fold's drive (1 up to K1's maximum), the drop, the ping's strength and its FM
index. The bass is mostly discharge, the treble mostly ping. Knobs (live's K1–K4, `-seq`'s
`knob K X`; their starting places `PJ_SCIFI_FOLD`, `PJ_SCIFI_RATIO`, `PJ_SCIFI_BALANCE`,
`PJ_SCIFI_RING`):

| Knob | Sets |
|---|---|
| K1 | the fold: the hardest key's drive, 1.5 to 30 (default 6.7) |
| K2 | the modulator's ratio, 0.05 to 4 (exponential; 0.68 is 1, the default) |
| K3 | the balance, discharge to ping (default: by the key) |
| K4 | the ping's ring when held, T60 0.25 to 64 s at middle C (default 2.3 s) |

The sustain pedal holds; pitch bend ±2 semitones.

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
    ./pajupilli -pianist -play song.mid      # the piano's sustain pedal planned anew

- **`-pianist`** (`src/pianoplan.h`, `research/pedaling.md`): each channel played by a piano with
  dampers (grand, bright, upright, honky-tonk) is pedalled by a plan made from its notes, the
  file's own CC 64 there set aside. Legato pedalling: up at the new harmony's onset, down
  40 ms + a quarter of the time to the next onset after it (MAESTRO's pianists), changed at the
  bass's and the harmony's changes and at revoiced chords, not at a melody's notes; up through a
  low stepwise run; sparing in counterpoint (Bach: the pedal down 0–4 % of the time); lifted at
  rests and at the end. It prints the texture it found, the changes and the time pressed
  (`PJ_PIANIST_LOG=1`: every decision). On ten performed files it finds 72 % of the pianists'
  own changes within 150 ms (Don't Explain: 80 %), 63 % of its changes theirs.

`-transpose N` with `-play` moves every note but the drums N semitones (12: an octave up, -12 down)
as the file is read, so the organ's and the Hammond's plans are made for the transposed piece.

`-reinstrument FROM=TO[,FROM=TO]` plays a channel on another instrument: FROM is the instrument its
program picks (`grand-piano`, `electric-grand`), its family (`piano`) or `*` (every channel but the
drums); the first rule that matches wins; `organ` is the church organ by its plan (pedal, manuals,
registration, articulation). A channel remapped onto any Hammond (`hammond`, `drawbar-organ`, ...) joins the
file's Hammond, one instrument and one plan.

**The winds tune by ear to the band** (`src/windear.cc`, `src/band.h`; `research/fretless_bass.md`
7.6): every brass part (trumpet, muted trumpet, trombone, horn, tuba, the brass section) and every
blown reed (the saxes, oboe, English horn, bassoon, clarinet; duduk, shawm, shehnai; not the free
reeds, which are fixed, nor synth brass) tunes each held note to what the other channels sound, by
the fretless bass's spectral rule: its own partials (brass 1/√k to 2 kHz, the reeds 1/k, the
clarinet's even partials weak below 1.5 kHz) against each other note's template, within ±15 c
(`PJ_WIND_EAR_LIM`). Over a piano's or the strings' C3 G3, an E4 leans to the just third (-13.3 c):
the alto sax, oboe, clarinet and bassoon measured -12.7 to -13.6 c, the horn -13.2, the tuba -12.7;
the trumpet and trombone move by -12 c, to -7.5 (their own +5 c on these notes stays, as without the
band); a G4 over C3 E3 +1.7 c (the reeds); a C4 over C3 E3 G3 stays (±0.5 c). A chord changing
under a held note is followed by the lips in 0.1 s or more (at most 0.2 c/ms; changes under 3 c let
be); with nothing to hear, back to equal temperament in ~3 s. In a part playing chords the
section's own just intervals are solved together with the band's pull (each voice's pull is its
anchor in the pajupilli intonation); the brass section leans to equal temperament (half of it).
Each wind's tuning goes back into the registry, so the others hear it as played; a note of a part
tuning by ear is heard only by the notes after it (struck together: the lower by the higher), so two
winds never chase each other, and where the winds sound alone their mean heals to equal temperament
(1 s, `PJ_WIND_EAR_HEAL`): four winds through the comma pump (C Am Dm G C, common tones held) keep
the C chord within 3 c of equal temperament round after round. `PJ_WIND_EAR=0`: as before;
`PJ_WIND_EAR_LOG=1`: each note's pull, 2: every change of its tuning.

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

The timpani (`-inst timpani`, General MIDI program 48; `src/timpani.cc`, `research/timpani.md`):
five kettledrums (32, 29, 26, 23 and 20 in.) for one timpanist, C2–G♯2, A2–C♯3, D3–F♯3,
G3–B♭3 and B3–E4 (outside, the outer drums). Each head is a membrane heard as its modes, not
a grid: what makes the drum pitched is its partials at 1 : 1.50 : 1.98 : 2.44 : 2.90 on the
(m, 1) modes, which the air on the head and the kettle's air make out of the ideal membrane's
1 : 1.34 : 1.66 …; each mode is placed where it was measured (the (0,1) held near 65 Hz + 0.46
of the pitch by the kettle's air spring; the rest stretched by an air load falling with
frequency), with that air's mass, and decays at its own measured rate: the main tone 3–4 s on
the low drums and ~1 s at the top, the fifth and the octave longer (D2 to 10 s), the thump of
the axisymmetric modes 0.3 s. A 28 g felt mallet (F = K x^2.54, the felt's loss) is thrown at
the head where it is now, a quarter of the way in from the rim, a little further in when louder,
and bounces off; a hard stroke stretches the film (fff on D2 starts ~+55 c sharp). The head's
far field is heard towards the hall, each mode's own pattern (the (1,1) a dipole along the line
from the player through the centre), and the timpani play in the concert hall. A key on a drum
tuned elsewhere retunes it; a key held 0.22 s or more is damped by the hand 0.15 s after its
release (unless struck again first), shorter ones ring on. Rolls: strokes under 140 ms apart on
one drum are a roll, the hands 12 cm apart round the ring (each its own mix of modes), evened
to ~±0.5 dB and a few ms late at random, each landing on the moving head and adding to its
swing or taking from it (the main tone moves ~3 dB stroke to stroke); the mod wheel (live: pad 1)
rolls a held key at the natural rate, 10 (f / 110 Hz)^0.4 strokes a second (8.5 on D2, 13 on
A3), faster as it gets louder, its depth the roll's level, following the expression;
`PJ_TIMP_ROLL=1` rolls any key held over 0.6 s. About 1.3 % of a core per ringing drum. `-seq`
takes `wheel X` and `expr X` (the mod wheel and the expression, 0..1) for such tests.

    ./pajupilli -inst timpani -room concert -seq "on 45 100 wait 1 on 52 100 wait 1.5 wheel 0.5 on 45 60 wait 3 off 45 wheel 0 wait 2"

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
