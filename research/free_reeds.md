# Free reeds: accordion, harmonica, tango accordion (bandoneón)

Status: 2026-11-16. The accordion (GM 22) and the bandoneón (GM 24) are built (`src/freereed.h`,
`src/accordion.cc`; §12); the harmonica (23) and the reed organ (21) are not, and the program table
in `src/midifile.cc` still sends those two to the `lead-square` synth.

Marks: **[paper]** a source below; **[web]** a web source below (forums, makers, dealers: weaker);
**[calc]** worked out here; **[memory]** general knowledge, to be checked; **[design]** a choice of
this plan; **[file]** measured in the user's MIDI files; **[rec]** measured in the user's recordings
(Demucs stems in the scratchpad; numbers only here).

## 1. Short answer

- **What the user's files ask for:** only the **harmonica**, in four bushgrafts files (1 119 notes),
  as single melodic lines with **downward pitch bends** (up to two semitones, median about half a
  semitone) **[file]**. Accordion, tango accordion and reed organ do not occur. Build order: the
  harmonica first; the accordion and the bandoneón after, for completeness and live play (§9).
- **The jazz harmonica is the chromatic one** (Toots Thielemans: *Waltz for Debby*, which is in the
  files as "Waltse For Dave", with 319 bends). The parts span MIDI 50-88, chromatically: a 12-16 hole
  chromatic harmonica (C3/C4-C7), not a diatonic one **[file, calc]**.
- **A free reed is not a beating reed.** A metal tongue swings *through* a slot cut a little wider than
  itself (clearance ~50 µm) **[paper, MB07]**. It moves almost sinusoidally on its first cantilever mode
  **[paper, RCM05 via MB07]**; the air it lets through comes in two puffs per cycle (one on each side of
  the plate), so the radiated pressure is close to a square wave, rich in harmonics **[paper, C11]**.
  The pitch is the reed's own, very slightly lowered by pressure (blown-closed reeds sound a little
  below their natural frequency) **[paper, C11]**.
- **The excitation** is the unsteady flow's inertia through the gap (Ricot, Caussé, Misdariis; St
  Hilaire's theory): a lumped model needs the reed mode, the slot's opening as a function of the tip
  position, the Bernoulli flow, and the inertia of the air upstream and downstream **[paper, RCM05,
  MB07]**. Millot & Baumann give a minimal model with all its numbers for a harmonica reed (§2.2).
- **The project's reed engine does not fit as is:** `src/reed.*` is a beating reed (it closes on a lay)
  driving a bore waveguide. A free reed needs its own small model: a reed mode, a slot-area law, a
  reed cell, no bore (the harmonica and accordion have only small cavities; the player's vocal tract
  is the harmonica's resonator for bends). It is cheap: ~20-40 ns per reed per sample **[calc]**.
- **Accordion and bandoneón are one instrument each, not a voice per note:** one bellows (one supply
  pressure, its own dynamics) feeds every sounding reed. That matters for the player model, not the
  reed model.

## 2. The free reed

### 2.1 What happens

- **Geometry:** a tongue of brass or steel, riveted at one end over a slot in a plate; the slot is a
  little longer and wider than the tongue. At rest the tip stands a little out of the slot on the
  upstream side (the "set", "Δh₀") **[paper, MB07; memory]**.
- **Blown-closed:** the pressure first pushes the tongue *into* the slot (closing it), it swings through
  and out the other side (opening it on the far side), and back. Western free reeds (accordion,
  harmonica, harmonium) are blown-closed and sound only for one flow direction; hence two reeds per
  note in accordions and harmonicas, one per direction, with a valve over the slot of the reed that is
  not meant to speak **[paper, C11]**.
- **Motion:** the tip swings about **15 % of the tongue length** **[paper, C11]** (~2 mm on a 13 mm
  harmonica reed **[paper, MB07]**), almost sinusoidally, on the first bending mode of a clamped-free
  beam **[paper, RCM05 via MB07]**.
- **Flow and sound:** the flow is pulsed, opening twice a cycle (once each side of the plate); the
  pressure waveform is nearly square, so the tone has abundant harmonics: "rich or harsh" **[paper,
  C11]**. On an accordion A4, 8' outside the cassotto, harmonics reach ~10 kHz, falling with
  frequency, above that negligible **[paper, EMS21]**.
- **Airflow:** a harmonica chamber with a small reed passes hundreds of mL/s, comparable to a clarinet
  **[paper, C11]**.

### 2.2 A minimal model, with numbers (Millot & Baumann)

A harmonica reed (G diatonic, hole 4; the reed below the plate) **[paper, MB07]**:

| Quantity | Value |
|---|---|
| Reed length × width × thickness | 12.95 × 2.1 × 0.110 mm |
| Plate (support) thickness | 0.9 mm |
| Clearance gap h_min | 50 µm |
| Rest set of the tip | 0.528 mm |
| Natural frequency f_r | 444 Hz |
| Equivalent stiffness K | 47.9 N/m |
| Quality factor Q | 95 |
| Tip amplitude in play | ~2 mm |
| Over-pressure across the reed in play | ~570-800 Pa |
| Playing frequency vs supply velocity | changes of 0.4-2 Hz over its range |
| Playing frequency vs the upstream volume | changes of 2.6-22 Hz (the cell matters) |

- **Equations (the minimal model):**
  - the reed: one mode in the tip displacement ζ, driven by the pressure difference Δp, damping from
    Q, stiffness K;
  - the flow out: the reed's own sweeping (S_r dζ/dt) plus the jet through the gap (α S_u v_j), α a
    vena-contracta coefficient;
  - the jet: Bernoulli, Δp = ½ ρ v_j²;
  - the upstream volume V₁ (mass conservation with the steady feed u₀) and the inertia of the air in
    the feeding duct, ρ L₂/S₂ du/dt;
  - the useful escape area S_u(ζ): the gap's front and sides, near the rest opening ≈ S_u0 + a₁ ζ²
    (small when the tip is in the slot, the clearance only; large when it is out).
- **Behaviour:** an inverse (hysteretic) bifurcation at the onset; the playing frequency lies below
  f_r for one reed type and above for the other, depending on the cell **[paper, MB07]**. Its
  quasi-stationary Strouhal number was 0.024-0.027 (the flow quasi-steady), Mach < 0.1.
- **Natural frequency, a check [calc]:** a uniform clamped-free beam: f = 0.1615 (t / L²) √(E/ρ). For
  brass (E 100 GPa, ρ 8500: √(E/ρ) ≈ 3430 m/s), t 0.11 mm, L 12.95 mm: f ≈ 363 Hz; steel (√(E/ρ) ≈
  5050 m/s): 534 Hz. The measured 444 Hz sits between, as a tapered, ground tongue does. Accordion and
  bandoneón bass reeds are long and carry a tip weight to reach their low pitch **[memory]**.

### 2.3 Pitch, pressure, damping

- **Pressure:** in the normal playing range the frequency falls roughly linearly as the pressure rises;
  anomalies at very low and very high pressure; some hysteresis. The reed-organ measurements span
  ~0.5-3.5 kPa **[paper, CRB; CH]**.
- **A reed cell (a cavity behind the reed) lowers the frequency** at the same pressure (reed organ
  reeds) **[paper, CRB]**.
- **Accordion pitch bending** (more bellows pressure, the pallet only partly open): **10-35 cents down,
  ~25 with bellows and finger, ~15 with the finger alone**, all harmonics alike (harmonicity kept),
  the level falling; impossible above C6; the 4' register bends badly; register, cassotto and bellows
  direction make no difference **[paper, EMS21]**. Upward never.
- **Dynamics:** a mezzo-forte accordion note ~70 dB(A) at 50 cm **[paper, EMS21]**.
- **Damping:** Q ~95 for the harmonica reed in air **[paper, MB07]**: alone it rings ~0.07 s at 444 Hz
  (T60 = Q·2.2/f) **[calc]**; in play the losses are the flow's.

## 3. The accordion (GM 22)

### 3.1 The instrument [memory unless marked]

- **Reeds:** steel tongues riveted on aluminium (or zinc) plates, **two per plate**: one sounds on
  push, the other on pull, each covered on the outside by a leather or plastic valve flap against the
  reverse flow **[paper, C11; memory]**. Plates are waxed onto wooden reed blocks; the wax gives a
  warmer sound than the bandoneón's nailed plates **[web, AI1]**.
- **Registers (right hand):** reed sets at 16' (an octave low), 8' (one to three sets), 4' (an octave
  high), combined by register switches. A "musette" combines two or three 8' sets tuned apart so they
  beat **[web; memory]**.
- **Musette tuning:** the French musette's three middle reeds at **A 435 / 440 / 445 Hz** (±5 Hz:
  beats of 5 per second each against the middle, ±~20 cents) **[web, dealer]**; a modern "dry-ish"
  musette with one reed at 438.5 against 440 (~1.5 beats/s, ~6 cents) **[web, forum]**. Tuning is
  usually *constant in beats per second* rather than in cents over the range **[memory]**, so the low
  notes are detuned by more cents.
- **Cassotto:** a tone chamber in which one 8' and the 16' block sit, the sound leaving through a turn
  in the chamber: the higher harmonics lowered, a darker, rounder voice **[paper, EMS21]**.
- **Bellows:** the left arm moves the bellows; pressure (and so loudness) comes from how hard it is
  pushed or pulled; direction changes are articulations (a small gap, a different reed of each pair).
  Bellows shake: rapid direction changes, a tremolo of the whole sound. Typical playing pressures a few
  hundred Pa to ~2 kPa **[memory, to check]**.
- **Left hand:** the Stradella bass: rows of buttons for single bass notes (two rows: fundamental and
  counter-bass, a third above) and for whole chords (major, minor, seventh, diminished), each from a
  few octave-doubled reed sets so that the bass notes and chords stay within one octave whatever the
  key ("folded" range) **[memory]**. Free-bass systems give single notes over the whole range (the
  Pigini Sirius in EMS21 is free-bass).

### 3.2 What a model needs

- Per sounding note: 1-3 reeds (registers × musette sets), each its own free reed (§2.2) with its own
  slight tuning and tip set, in its plate's cell; for the push and pull directions the other reed of
  each pair (very slightly different).
- The radiated sound: each reed's flow puffs into the shared box; the cassotto as a filter (a low-pass
  of the higher harmonics, ~-6 to -12 dB above 2 kHz [design, to fit]); the box and the grille's small
  resonances.
- The bellows: one supply pressure for all, with its own volume (the bag), the arm's force and the
  sum of all reeds' flows: chords drop it a little (playing more notes takes more air) **[design]**.

## 4. The tango accordion: the bandoneón (GM 24)

- **The instrument:** a square concertina-type instrument, buttons on both ends; **bisonoric**: each
  button gives a different note on push and on pull (the Rheinische system); a 142-voice instrument
  has 71 buttons **[web, OC; memory]**.
- **Reeds:** steel tongues on **zinc plates** (aluminium on some), each note sounding **two reeds an
  octave apart** (8' + 4', no beating when well tuned) **[web, OC; HOR]**. Zinc plates are prized: "a
  large, deep sound", "sweet and somewhat dull" **[web, BB; BNI]**. Plates are nailed with leather
  gaskets onto the wooden reed blocks, which contributes a sharper, brighter attack than the waxed
  accordion plates **[web, AI1]**.
- **No cassotto, no musette, no registers** **[memory]**: one fixed voice, the octave pair; dynamic and
  articulate through the bellows (tango's hard *marcato* attacks, the sharp bellows reversals).
- **What GM players do with 24:** sample sets play an accordion or bandoneón-like timbre **[memory]**.
  In tango the bandoneón is the instrument meant; **recommend modelling the bandoneón** for 24, the
  accordion (with musette) for 22.

## 5. The harmonica (GM 23)

### 5.1 Diatonic and chromatic [memory unless marked]

- **Diatonic (Richter):** 10 holes, a blow and a draw reed in each cell (one on each side of the comb's
  plate pair); in C, blow C E G C E G C E G C, draw D G B D F A B D F A. Bends: draw bends in holes
  1-6, blow bends in 7-10 **[paper, BAB98]**: the higher note of the hole can be bent down to a
  semitone above the lower one (hole 3 draw: three semitones, B♭, A, A♭ on a C harp; holes 2 and 10
  two). Both reeds take part: the higher reed bends down, the lower one up; the sound passes from one
  to the other **[paper, BAB98]**.
- **Overblows and overdraws:** a third mode, a single reed sounding in an *opening* mode, its pitch
  outside the hole's two notes; they give the diatonic's missing notes **[paper, BAB98; C11]**.
- **The vocal tract is the resonator:** bending is done with the tongue and throat (arching or
  thickening the tongue, constricting the throat), tuning the tract's resonance near the reed so the
  reed's playing frequency is pulled **[paper, BAB98; C11; MRI]**. 3D-printed vocal tracts from MRI
  scans reproduce a player's bends **[web, VT15]**.
- **Chromatic:** 12, 14 or 16 holes; each hole has a blow and a draw reed for the natural and, with the
  slide pressed (a spring button), the holes' other plate for the sharpened notes; valves (windsavers)
  cover the reeds against the reverse flow, so bends are shallower and less like a diatonic's; a 16-hole
  chromatic in C covers C3-C7 (MIDI 48-96) **[memory]**. Toots Thielemans' jazz harmonica is the
  chromatic **[memory]**.
- **Hand effects:** the hands cupped round the harmonica make a variable resonator: muted, wah
  (opening), tremolo (fluttering hands) **[memory]**.
- **Cover plates:** the sound radiates from the holes and the back of the covers; covers add a little
  brilliance and project forward **[memory]**.

### 5.2 What the user's files ask (§8): chromatic, melodic, bends downward

So: a chromatic harmonica in C (MIDI 48-96 or 60-96), one note at a time (chords at most rare: 0-1 %
of onsets), the slide implied by the note, blow or draw by the note's position in the hole layout,
bends as the player's tract pulling the speaking reed down by up to ~2 semitones (the files' maximum,
at the default bend range) with a softer, choked sound, as on the accordion **[file, design]**.

## 6. Synthesis models and costs

- **The free reed** (§2.2) as a lumped model: one mode (two states), the slot area S_u(ζ) as a table or a
  smooth function, Bernoulli flow (a square root), the reed's sweeping, a small upstream volume
  (pressure state) and the downstream inertance; radiation as the flow's derivative (a small source).
  ~40-80 flops a sample **[calc]**: ~20-40 ns per reed at 48 kHz on one core. Stability: the stiff
  flow/volume loop wants the upstream volume and the reed solved together (implicitly, as the
  project's `Reed` does for the mouthpiece) or 2-4× oversampling for the slot's fast opening.
- **Reuse from the project:**
  - `src/reed.cc`: the trapezoidal solve of reed + mouthpiece pressure + Bernoulli flow in closed form;
    the free reed is the same structure with a different opening law (a slot, not a lay) and no bore.
  - `src/drone.h`: a cheap waveguide reed (the bagpipe's drones), not right for free reeds (it has a
    pipe), but its "one object, many reeds" form is.
  - The vocal tract for harmonica bends: a single resonator (a Helmholtz-like mode, its frequency set
    by the bend amount, its Q a few) between the player's lungs and the reed's upstream side, as the
    "upstream volume + duct" of MB07 with the duct's inertance changed **[design]**.
- **The accordion's load** **[calc]**: right hand 4 notes × 3 reeds (musette) = 12; left hand bass 2
  notes × 2-3 sets + a chord of 3 notes × 2-3 sets ≈ 15: ~30 reeds, ~1 µs per sample, ~5 % of a core.
  The bandoneón: up to ~10 notes × 2 reeds. The harmonica: 1-2 reeds (the speaking one, and in bends
  its partner).

## 7. From MIDI to playing [design]

- **One instrument, one bellows (accordion, bandoneón):** every held key's reeds draw on one supply
  pressure: the bellows' pressure p_b follows a target from the velocity (the push's force) and
  CC11/CC2 (expression/breath) as the bellows' dynamics, with the air the reeds take: a chord droops
  it a little, a single note after a chord rises.
- **Push and pull:** the accordion's direction changes on rests or long notes (the bellows run out:
  every ~2-4 s of continuous playing), with a short articulation (the other reed of each pair, a few ms
  of silence) **[design]**; the bandoneón's direction is *chosen by the note* when the button's push
  and pull notes differ (bisonoric): in practice for a file, choose the direction that keeps the
  bellows moving the same way where possible, changing between phrases **[design]**.
- **Velocity:** the attack's push (how fast the pressure rises: a hard key a sharp start) and the
  level; **CC11/CC2:** the bellows' pressure (swells), as a held note's dynamics.
- **Registers by program:** 22: musette (8' × 3, ±15-20 c, or two sets at ~+6 c dry-ish) [to ask];
  24: the bandoneón's octave pair; 21 (reed organ): a harmonium (8' with a 4', pedal-pumped, a
  softer, steadier supply) if wanted later.
- **Left-hand Stradella from a file:** a GM accordion part is usually written as notes, not chord
  buttons; notes below ~C3 can go to the bass reeds (16'+8'), a block of 3-4 notes in C3-C4 to the
  chord reeds (8'+4', folded to one octave) **[design]**.
- **Harmonica:** each note to its hole and direction (blow/draw) and slide state; a new note on the
  other reed of the same hole within a phrase is just a breath change; legato lines without breaks;
  **pitch bend down** = the tract tuned below the reed: the speaking reed pulled down, its level and
  brightness falling (as measured on the accordion: 10-35 cents give a few dB drop) **[paper, EMS21;
  design]**; bends deeper than ~1 semitone on a valved chromatic are not physical **[memory]** but the
  file asks up to 2: let them through, the sound choked; CC1 (Waltz for Dave: 61 events): the hand
  vibrato/tremolo (cupped hands fluttering) **[file, design]**.

## 8. In the user's files and recordings

### 8.1 MIDI (~/Music/midi) [file]

| File | Program | Notes | Range | Note length (median, beats) | Chords | Velocity (median) | Bends | CC |
|---|---|---|---|---|---|---|---|---|
| Baby its Cold outside | 23 harmonica | 201 | 58-72 | 0.57 | 0 % | 95 | 0 | CC7 ×1 |
| Once upon a summertime | 23 | 187 | 50-81 | 0.27 | 0 % | 49 | 102 (down only, to -4992/8192 ≈ -1.2 st, median -0.56 st) | - |
| Waltse For Dave (Waltz for Debby) | 23 | 155 | 59-80 | 0.47 | 1 % | 68 | 319 (down only, to -8192 ≈ -2 st, median -0.5 st) | CC1 ×61, CC7, CC64 |
| soon | 23 | 576 | 53-88 | 0.09 | 1 % | 56 | 0 | - |

- No GM 21, 22 or 24 in any file. Bend amounts assume the default ±2 semitone range (no RPN 0 sent).
- *soon*'s 0.09-beat notes are very short (staccato, or a sequenced ornament): a harmonica tongue-
  articulation (a breath stop) rather than held notes.

### 8.2 Recordings (~/Music/wavefield) [rec]

Demucs `htdemucs_6s` has no accordion or harmonica stem; those go to "other" (with violins, winds),
sometimes to "piano" or "guitar". On the tango and French-café tracks:

| Track | "other" share | "other" alone (2 s windows over 70 %) | Where (s) | Likely in it |
|---|---|---|---|---|
| Rough Guide to Tango 03, *Escualo* (Piazzolla) | 73 % | 47 % | 60-68, 168-188 | bandoneón with violin (Piazzolla's quintet) |
| Rough Guide to Tango 08, *Balbón* | 22 % | 7 % | 48-64 | bandoneón / strings |
| Le Violon Vagabond, track 3 | 37 % | 20 % | 40, 116-124, 176-184 | violin (the title), to check |
| Putumayo Vintage Italia, track 4 | 11 % | 6 % | 168-184 | accordion or mandolin? (centroid 1.6 kHz) |
| Silje N, *The Waltz* | 9 % | 10 % | 0-24 | to check |
| Putumayo French Café 03-06, 10, 11; Tango 01 *Niebla del riachuelo* | 0-20 % | ~0 | - | accordion, if present, not separated from the vocals/guitar |

- A monophonic pitch track of the "other" stem was not usable (the passages are polyphonic). Which of
  these hold a bandoneón or an accordion alone needs listening; *Escualo* 60-68 s and 168-188 s are the
  first candidates for the bandoneón. **No harmonica recording was found.**

## 9. Plan [design]

1. **Free reed core** (`src/freereed.h/.cc`): §2.2's minimal model, implicit in the reed + upstream
   volume, a slot-area law with the clearance and the plate's thickness, two-sided puffs; checked
   against MB07's numbers (f_play vs f_r, amplitude ~2 mm, Δp ~600-800 Pa), the near-square pressure
   waveform and its harmonics to ~10 kHz (EMS21), the onset hysteresis, pitch falling slightly with
   pressure; cost under 50 ns per reed.
2. **Chromatic harmonica (GM 23), first:** C chromatic, 16 holes (MIDI 48-96); a player: hole, blow or
   draw, slide; legato phrasing; the vocal tract as an upstream resonator driven by pitch bend (down
   only) with the level and brightness drop; CC1 as the hands' tremolo; tongue stops for short notes.
   Fit by ear (no recording found), and to BAB98 / C11 behaviour. Check on the four files.
3. **Accordion (GM 22):** one instrument object with a bellows; right-hand 8' × 3 musette (or the user's
   choice), 16' and 4' registers as options; the cassotto as an option (a filter); left-hand bass and
   chords for low notes; push/pull with direction changes; EMS21's pitch-bend behaviour on a pressure
   rise with a partly open pallet (for pitch bend events).
4. **Bandoneón (GM 24):** the octave pair (8' + 4'), zinc-plate voicing (brighter attack, no cassotto),
   bisonoric direction choice, marcato bellows attacks; fit to *Escualo* if its bandoneón can be
   isolated.
5. **Reed organ (GM 21), optional:** a harmonium from the same reeds: a steady pumped supply, 8' + 4'.
6. **Tests:** tuning of every reed set (cents), the musette beat rates, bend depth vs pitch-bend value,
   onset threshold and hysteresis, fuzz, real time (accordion with 30 reeds under 6 % of a core).

## 10. Open questions for the user

1. **Harmonica:** chromatic (Toots) as recommended, or should the diatonic (blues) also exist?
2. **Accordion voice for GM 22:** a French musette (three 8' sets, wide beating), a dry or "Italian"
   tuning (two sets, slow beats), or a single 8' set? With cassotto or without?
3. **GM 24:** the bandoneón (recommended) or a second accordion voice?
4. **Recordings:** are there harmonica, accordion or bandoneón recordings to fit to? (*Escualo* is the
   only likely bandoneón found.)
5. **Reed organ (21):** wanted?

## 11. Sources

- **[MB07]** L. Millot, C. Baumann, "A proposal for a minimal model of free reeds", Acta Acustica
  united with Acustica (2007); arXiv:2401.01606. https://arxiv.org/html/2401.01606v1 (harmonica reed
  dimensions, f_r 444 Hz, K 47.9 N/m, Q 95, gap 50 µm; the model's equations; Δp ~570-800 Pa).
- **[RCM05]** D. Ricot, R. Caussé, N. Misdariis, "Aerodynamic excitation and sound production of
  blown-closed free reeds without acoustic coupling: the example of the accordion reed", JASA
  117(4):2279 (2005) (unsteady-flow inertia as the excitation; sinusoidal reed motion, first mode;
  abstract only read).
- **[C11]** J. Cottingham, "Acoustics of free-reed instruments", Physics Today 64(3), 44 (2011).
  https://physicstoday.aip.org/features/acoustics-of-free-reed-instruments
- **[EMS21]** M. J. Elejalde-García, E. Macho-Stadler, R. Llanos-Vázquez, "Accordion acoustics: a study on
  pitch bending", Acoustics in Practice AiP-2021-02.
  https://www.euracoustics.org/documents/14/AiP_2021_02_Accordion_acoustics.pdf
- **[BAB98]** H. T. Bahnson, J. F. Antaki, Q. C. Beery, "Acoustical and physical dynamics of the diatonic
  harmonica", JASA 103(4):2134-2144 (1998). https://www.cs.bu.edu/fac/snyder/TablatureWeb/Bahnson%20JASA%201998.pdf
- **[CRB] [CH]** J. Cottingham and co-workers (Reed, Busha; Hanzelka): reed-organ and free-reed frequency
  vs blowing pressure, ASA meeting abstracts. https://auditory.org/asamtgs/asa97pen/4aMU/4aMU1.html ;
  https://auditory.org/asamtgs/asa96haw/4aMUb/4aMUb8.html
- **[FA23]** Forum Acusticum 2023 paper on free-reed excitation (St Hilaire's inertial theory, Tarnopolsky's
  and Ricot's experiments). https://dael.euracoustics.org/confs/fa2023/data/articles/000853.pdf
- **[MRI] [VT15]** P. Egbert et al., real-time MRI of the vocal tract in harmonica bends (Stanford CCRMA);
  Vienna Talk 2015 abstract: 3D-printed vocal tracts reproduce bends.
  https://viennatalk2015.mdw.ac.at/abstracts/ViennaTalk2015_submission_84.pdf
- **[OC]** O. Caccia, "Understanding the bandoneon: a guide for composers".
  https://www.omarcaccia.com/understanding-the-bandoneon-a-guide-for-composers/
- **[BB] [BNI] [HOR] [AI1]** bandoneón reeds and plates: https://www.bando-bando.de/bandoneon/230720/ELA-Zink.html ;
  https://bandoneonist.ch/bf/bf_node2.html ; https://www.horniman.ac.uk/object/M458a-1996 ;
  https://www.accordionists.info/threads/bandoneon-reeds-and-tuning.8004/post-84544
- **[web, dealer / forum]** musette tunings: https://www.accordionists.info/goto/post?id=95233 ;
  https://www.libertybellows.com/general-accordion-questions.htm

## 12. Built (2026-11-16)

- **The reed** (`src/freereed.h`): §2.2's minimal model (MB07), with the reed's mode exact over
  each sample, the duct's flow solved implicitly (a quadratic in closed form: the clearance's stiff flow
  stays stable), and a cell filled through the pallet. The gap: the front and ~0.78 of the length on the
  sides, `alpha` (W + 0.78 L) sqrt(d^2 + h_min^2), d the tip's distance out of the slot on either side.
  It runs at 96 kHz: at 48 kHz the sharp puffs alias to about -30 dB on the high reeds, at 96 kHz
  -42..-52 dB.
- **Checked against MB07's harmonica reed** (444 Hz, upstream 6.4e-5 m^3, a 20 mm x 25 mm^2 duct):
  it plays at 438 Hz, the tip swinging +-1.8..3.6 mm from 300 Pa to 2 kPa, the pitch falling slowly
  with pressure (439.0 to 438.0 Hz) **[calc]**, as MB07 and CRB report. It starts at ~100-150 Pa. The
  duct's inertance, not the volume, is what makes it speak: a 1-3 cm^3 cell (an accordion's chamber)
  works as well as MB07's large one, but a duct of a tenth of the inertance does not speak.
- **By pitch [design]:** the tongue 13 mm x (444 / f)^0.42, its width 2.1 mm x (444 / f)^0.2, the set
  4 % of the length, the plate 7 % (0.8-2.5 mm), the stiffness from a closing pressure of 1.5 kPa, Q
  100 x (f / 440)^0.35, the duct's inertance 960 x 444 / f (kg/m^4: the pitch pulled down ~25 c alike
  over the range, then tuned in). Onset thresholds: ~50-100 Pa in the middle, ~200 at C1 and C8.
- **The instruments:** §3.2 and §4, with §10's open questions answered by default (to revisit):
  the accordion a three-set musette (3.5 beats/s at A4, `PJ_ACC_BEAT`), the middle set and the bass in
  the cassotto; GM 24 the bandoneón (8' + 4'). The bellows: one pressure, the arm's target from
  velocity, the chord's sag, direction changes every 2.5-5 s (between notes when possible). The
  bend: the duct's air x (1 + |bend|), the pallet up to 70 % shut, the pressure x (1 + 0.6 |bend|):
  -13 / -29 / -46 c at a quarter / half / full bend, as EMS21's 10-35 c.
- **Not yet:** a recording to fit the radiation to (the cassotto, the grille and the pull side are
  one-pole filters set by hand from §3.1's description); the bass's Stradella chords; registers chosen
  by the player; the bandoneón's bisonoric direction (it turns like the accordion); the harmonica;
  the reed organ.

