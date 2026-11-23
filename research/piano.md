# A concert grand: research and model plan

Status: a first version is built (2026-11-14, `-inst grand-piano`); section 10 says what it has
and what is still open. Sections 1-9 are the research as written before building.

The instrument: a concert grand (the 274 cm concert grand is the one the literature measures), GM
program 1, MIDI 21–108. A physical model like the rest of the project: hammers, strings, bridge,
soundboard, no samples.

Where each number comes from is marked: **[web]** from the sources in section 9, **[calc]**
computed here from those numbers, **[memory]** from the general literature as remembered, not
looked up for this document, to be checked before it is relied on.

## 1. The two questions, answered first

**CPU or GPU: CPU.** The whole piano, every string always ringing, is about 20 000–30 000 modal
oscillators (section 3). Measured figures for exactly this kind of work (banks of modes at 48 kHz)
**[web, Frontiers 2025]**:

| Platform | Modes in real time | Latency |
|---|---|---|
| One CPU thread (Apple M1/M2) | 65 000–70 000 | none added; any buffer from 128 up, unchanged |
| 8 CPU threads | 280 000–420 000 | |
| RTX 4070, 256-sample buffers | 494 000 | 0.2 ms at 48k modes, 2.2 ms at 768k; varies more from call to call |

- The piano needs a third to a half of what one thread does. The GPU starts to pay beyond some
  hundreds of thousands of modes: ten times more than this model has.
- A GPU works in buffers. Earlier measurements **[web, Renney 2020]**: buffers under 32 samples
  cannot keep the sample rate, 256 is the comfortable size. The live loop here runs periods of
  1.3 ms (64 samples); a 256-sample buffer alone is 5.3 ms, before the sound server.
- The hammer on the string and the strings on the bridge are feedback loops closed every sample.
  On the CPU the whole state (about 0.5 MB) stays in the cache of one core.
- This machine: Ryzen 9950X3D with AVX-512 (16 floats per register, as the build already uses)
  and an RTX 5090. **The GPU's place is offline:** fitting the model's parameters to recordings
  (thousands of renderings in parallel), and computing the soundboard's modes from a fine mesh.

**16 × 16 floats for the wood: yes, as 256 modes rather than 256 grid points** (section 4).
**16 floats per string: enough from about C4 up, far too few below** (section 3): 16 per string
is one register; the bass strings need 8–12 registers each.

## 2. What the instrument is

- **Strings [memory]:** 88 keys, 243 strings on a 274 cm concert grand: keys 1–8 one wound string, 9–13
  two wound, 14–20 three wound, 21–88 three plain steel. Speaking lengths about 2.0 m (A0) to
  5 cm (C8). Tension 600–1300 N per string, about 20 tonnes in all.
- **One measured string [web, Chabassier RR-8181]:** D#1: L = 1.945 m, steel area 1.31 mm²,
  T = 1328 N, E = 202 GPa, ρ = 7850 kg/m³.
- **String impedance [web, Askenfelt via Ege]:** about 10 kg/s (bass) to 5 kg/s (treble).
- **Bridge impedance [web, Ege & Boutillon; Conklin; Giordano]:** about 1000 kg/s from 100 Hz to
  1 kHz (near a rib 1000–2000, between ribs 600–700), falling to 200–300 kg/s by 2.5–10 kHz.
  A hundred times the string's: the string gives its energy slowly, hence the long sustain.
- **Inharmonicity [web, range only]:** partial n at n·f0·√(1 + B n²), B from about 10⁻⁴ in the
  low-middle to 10⁻² at the top. A 274 cm concert grand's tuning stretch: about −22 c at A0 to +46 c at C8
  **[web, a users' forum, unverified]**.
- **Hammers [web, KTH lectures]:** 11–12 g in the bass to 3.5–6 g in the treble. Felt as a
  spring that hardens: F = K·δ^p, p = 2.2–3.5 for played hammers, 1.5–2.8 for new ones (Hall &
  Askenfelt). One value: C#5, 7.9 g, K = 2.8·10¹⁰ (Chaigne). The felt also loses energy
  (hysteresis: Stulov's model); a plain power law "did not sound very good", with hysteresis
  "much improved" (the Purdue group).
- **Contact time [web]:** 4–5 ms for soft bass notes to under 1 ms (0.5 ms loud) in the top
  octave; about a tenth of the period in the bass.
- **Strike point [memory]:** about 1/8 of the string in the bass and middle, moving towards
  1/12–1/17 in the top octaves.

## 3. The strings

A string as a bank of modes, as in `src/bass.cc` (64 modes × 2 directions there). Modes needed
below a frequency limit, with inharmonicity, summed over the 243 strings **[calc, with a rough
B curve from memory]**:

| Limit | A0 | C2 | C4 | C6 | C8 | All 243 strings, one direction |
|---|---|---|---|---|---|---|
| 5 kHz | 98 | 61 | 18 | 4 | 1 | 5 400 |
| 10 kHz | 145 | 99 | 32 | 8 | 2 | 9 100 |
| 20 kHz | 209 | 149 | 54 | 15 | 4 | 14 500 |

- **16 per string** reaches 440 Hz on A0, 1 kHz on C2, 4.2 kHz on C4, 16.7 kHz on C6: a dull
  bass. Bank's real-time modal piano uses up to about 140 per note in the bass, falling to a
  handful at the top **[web, Bank 2010, fig. 7]**.
- **Proposed:** modes up to 10 kHz (to 12 kHz where affordable), in whole registers of 16: bass
  strings 8–12 registers, middle 2–4, the top two octaves 1. About 10 000 modes per direction.
- **Two directions per string.** A string swings up-and-down (the hammer's direction: gives its
  energy to the soundboard fast) and sideways (slow). The two together are the piano's double
  decay: a quick first fall, then a long aftersound (Weinreich) **[memory]**. So about 20 000
  oscillators. The bass model already carries both directions and their slow turning.
- **Three strings per key, slightly mistuned** (0.5–2 c): they beat, exchange energy through the
  bridge, and the aftersound grows from it. This comes for free once each string is its own bank
  and the bridge is shared. It matters more to the tone than any detail of the wood.
- **All strings always computed.** With every string a cheap bank, the sustain pedal's wash
  (all 243 strings answering through the bridge) and the undamped top octave and a half are simply
  there. Bank needed separate "secondary resonators" for this (30 % of his cost).
- **Lengthwise motion.** A loud bass note has a ringing "ping" from the string's stretching:
  free longitudinal modes (16–20 times the transverse fundamental **[web, Bank]**) and phantom
  partials at sums of transverse partials. Bank's method: the stretching force from products of
  the transverse modes' amplitudes, a few longitudinal resonators (5–10), one-way coupling.
  25 % of his cost.
  **Built (2026-11-21, `Piano::stretch`, `PJ_PIANO_LONG`):** on the strings whose first
  lengthwise mode is under 2.5 kHz (the wound bass and the low plain strings, A0 to about E3),
  up to 8 lengthwise modes each (the core's E A from B, the whole string's mass: A0 440 Hz, 16 x;
  key 21 1.3 kHz, 15 x), Q 50, driven by the slopes' products (pairs k apart and pairs adding to
  k, both directions of swing); the bridge gets the even tension (the partials' squares) plus
  each mode's motion less what it would hold at rest (the phantoms, rising near the modes).
  That push along the string rocks the bridge: it moves the board as 0.7 of it would at 1 kHz,
  as sqrt(f) (bridge height 3 cm x the board's bending wavenumber under the bass bridge,
  ~24 /m: an estimate; Conklin found the bridge about as mobile along the strings as across).
  One-way, as Bank's. Computed after each blow until the string has lost 20 dB (at most 4 s).
  Measured (dry, A0 A1 C2 G2 C3 at 127 against the old piano): 2-5 kHz over the first 0.2 s
  +2.8 +1.3 +1.8 +2.5 +0.6 dB, over 0.5-1.5 s +1.5 +1.0 +0.3 +1.2 +0.5; level +0.5 .. +0.1 dB;
  at 64 under 0.3 dB anywhere; the added 1-5 kHz sound falls 13 dB in its first second at C2
  and G2 (A0's partials die slowly, and so do its phantoms: 4 dB). In Taking a Chance on Love's
  loudest bass passage (142-165 s, velocities to 99) the 1-5 kHz band +0.4 dB, the level
  unchanged. Cost: ~0.75 % of a core per loud bass key while it is computed.
- **Damping per mode:** a law in frequency (air, internal friction) plus what the bridge takes;
  T60 from tens of seconds in the bass to under a second at the top **[memory]**; to be fitted to
  recordings.
- **Beyond the speaking length:** duplex scales and the lengths behind the bridge, as extra
  short banks on the same bridge (the bass model's after-lengths). Later.

## 4. The soundboard: 16 × 16 floats

A concert grand's board: about 2 m² of spruce, 6–9 mm thick, 17 ribs across the grain, two
bridges, a crown, the rim **[memory]**. Spruce: 380 kg/m³, E along the grain 11 GPa **[web,
Chabassier]**. What is measured on boards **[web, Ege & Boutillon, an upright: 1.39 × 0.91 m,
9 kg]**:
- Up to 1.1 kHz it behaves as a uniform plate: one mode per 19.5 Hz, mean loss factor 2 %.
- Above 1.1–1.2 kHz the waves are trapped between ribs (rib spacing 12.8 cm): the board, seen
  from the bridge, becomes three narrow strips; the impedance falls from 800 to 230 kg/s at
  2.5 kHz. Modes overlap; no single one is heard.
- A grand's board is about 1.6 times the area: about one mode per 12 Hz **[calc]**. A plate
  estimate for 2 m² of 9 mm spruce gives one per 7–14 Hz **[calc]**.

Two ways to spend 256 floats:

| | 16 × 16 grid of masses (as `BassBody`) | 256 modes of a finer board |
|---|---|---|
| Spacing | 12 cm × 9 cm: one point per rib bay | computed offline on e.g. 96 × 64 with ribs, bridges, crown |
| Accurate to | about 300 Hz (bending wavelength along the grain at 1 kHz is 30 cm, across 15 cm **[calc]**: 2–3 points per wave) | every mode exact; 256 modes reach 2–3 kHz on a grand **[calc]** |
| Above that | modes in the wrong places, none between ribs | a statistical tail (below) |
| Damping | one law for the plate | per mode |
| Stability | a step limit (met at 48 kHz for spacing over 3.4 cm **[calc]**) | exact, unconditional |
| Cost per sample | 256 points × 13 neighbours | 256 oscillators + the bridge sums |
| Changing the board at run time | easy (mass, stiffness per point) | recompute the modes |

**Proposed: 256 modes.** Each mode keeps its shape at the bridge points (where each key's
strings sit: 88 numbers, or 16 per bridge segment interpolated) and its radiation weights. The
plate is computed once, offline, in Python (the bass's `tools/` pattern), with ribs and bridges as
added stiffness and mass, the way the bass bar is done.

- **The tail above the last mode:** the bridge there is nearly a plain resistance (200–300
  kg/s). A second set of 64–128 broad resonances, spaced evenly in pitch up to 10 kHz, stands for
  the thousands of overlapping modes; Bank's soundboard filter is 100 such resonances.
- **The bridge in two directions.** The board resists up-and-down motion (1000 kg/s) and motion
  along the string (200–300 kg/s in the treble, comparable **[web, Conklin via Ege]**); sideways it
  is stiffer. This sets the double decay and how the longitudinal "ping" gets out.
- **The full-physics reference** (Chabassier, Chaigne & Joly: nonlinear stiff strings, 2400
  soundboard modes to 10 kHz **[memory]**, 3-D air; hours on a cluster per second of sound) shows
  what matters; it is far from real time, and its extra cost is mostly the air.

## 5. The hammer

- A mass (per key) on a hardening, lossy spring, striking at one point of each string of the key,
  both felt and string free to move: solved every sample, implicitly (an explicit step goes
  unstable for hard blows **[web, Bank]**). The top octave's contact (0.5 ms = 24 samples) needs
  the hammer at 2× the sample rate, or the implicit solve.
- Velocity sets the hammer's speed (about 0.5 m/s pp to 5 m/s ff **[web, Chabassier: 0.5, 1.5,
  3 m/s]**); the hardening spring makes loud notes brighter: no separate "brightness" control.
- The hammer's width (about 1 cm of felt) rounds the high modes, as the bow hair does in the bass.
- Left pedal: the hammers shift, striking two of three strings with fresh felt; the third string
  still rings with the others.
- Noise of the action: the thump of the key bed and the hammer shank, into the soundboard
  directly: audible in the top octaves, where the note itself is short. Second stage.

## 6. Dampers, pedals, sound

- Dampers: felt on the string near the hammer: a damping per mode by its shape there, coming on
  over 30–100 ms (the bass model's `touch`). Half pedalling follows from the felt's pressure.
  Keys 71–88 have none **[memory]**.
- Sustain pedal: all dampers up. Sostenuto: those up at that moment stay.
- **The dampers landing and the trapwork (built 2026-11-21, `PJ_PIANO_PEDALNOISE`, 0 = the old
  piano byte for byte).** No published level found (Askenfelt & Jansson measured the action's
  timing and the key-bottom noise, not the pedal's; Schutz et al. 2008 only "a noise floor" with
  the pedal); numbers are estimates **[calc]**:
  - Each damper (head, wire, the underlever's share: 20 g in the bass to 12 g at key 71; the
    upright's x 0.6, spring-driven) lands at the pedal's speed: 0.15 m/s x 0.75-1.25 for an
    on/off CC 64 (seeded), from the foot's speed (6 mm of damper lift over the CC's travel,
    at most 0.3 m/s) when CC 64 moves through values between; a key let go, 0.12 m/s (the
    upright 0.18). Its momentum m v goes into the strings at the damper's place (the felt's
    length-averaged mode shape) in a half sine of 8 ms (soft felt, ~2.5 kN/m), pushing them
    down (the grand) or towards the board (the upright); each head late by its regulation
    (0-0.5 mm of lift over its speed, fixed per instrument): a "chuff", not one click. Its grip
    starts when it touches: half within the push, the rest over 25 ms (was: 50 ms linear from
    the key's release).
  - The lift rail and the pedal's lever (0.6 kg) land on their felt stop in 8 ms; half the knock
    reaches the board through the rim (entering at the two ends of the bridges); a press knocks
    at 0.3 of it. The dampers leaving the strings give up only their resting weight: left out.
  - Measured (dry): a 5-note mezzo-forte chord's ringing stopped by the pedal: the thump ~32 dB
    under the chord's peak (50 ms windows), 40-500 Hz (none above 4 kHz), -20 dB within 25 ms
    plus the strings' quicker damping; the pedal alone 40-45 dB under; in Don't Explain (61
    releases in 60 s) the difference 31 dB under the music's loudest 50 ms, 22 dB under the
    music around each release. Cost: only during the 8 ms pushes.
- **Sympathetic strings (checked 2026-11-21).** With the dampers up every string is computed
  and driven by the bridge's motion (`key_.acc`); measured as string energies after a C4
  strike, pedal down: C3 (its 2nd partial on C4's fundamental) 6-10 dB under C4, C2 11-15,
  F1/F2 14-18; non-coinciding strings (C#, D, F#, A) 40-60 dB under; C5 struck: F3 (3rd
  partial ~2 c off) 11-14 dB under, C6 18-29, C4 (2nd partial ~2-3 c off) 16-24. That is what
  first-order theory gives: board coupling of a string ~ its board-loaded decay (~0.5 /s at
  C5) against mistunings of 0.3-1 Hz: (kappa / delta omega)^2 ~ -15 .. -25 dB. The board's
  transfer between keys' bridge points (from its 352 resonances) is 0-8 dB under the drive
  point to 1 kHz, 7-19 dB above 2 kHz. A broadband knock into the board (the key bottoming,
  tried at 27-31 dB under the note) changed no string's energy measurably: a struck note's
  halo comes from its partials, not from a knock. The pedal-down output: C3/C4 9-12 dB louder
  after 1-2 s, partials' off-harmonic share -66 -> -36 dB (C4). Left as it is (no tuning
  changed).
- Sound: the modes' radiation weights for two or three microphone positions (player, in the
  curve of the rim, the hall), then the project's room. Lid as a fixed colouring at first.

## 7. Cost, estimated

| Part | State | Per sample |
|---|---|---|
| Strings, 2 directions | 20 000 oscillators (1250 registers of 16) | rotation + bridge sum: the bulk |
| Lengthwise (stage 2) | about 10 per bass and middle string | products of mode pairs |
| Soundboard | 256 modes + 64–128 tail | small |
| Hammers | up to 10–20 in contact at once | one small solve each |

- From the measured 65–70k modes per thread, and the bass model's 2.4 µs per sample for 512
  string oscillators with a far heavier contact model: **about 30–50 % of one core** with every
  string ringing; less when quiet strings (below −100 dB) are skipped. To be measured.
- If more is wanted (20 kHz, 30 000+ oscillators), split by register over two or three cores:
  strings only meet at the bridge, one sample late.

### 7a. Measured: this CPU against this GPU (2026-11-14)

One benchmark, the same float32 arithmetic on both: string oscillators grouped by 88 keys, each
taking the board's velocity at its bridge point and adding to its key's bridge force; 384 board
modes; feedback closed every sample. No hammers or dampers. CPU: one thread, AVX-512. GPU: RTX
5090 through CUDA, three ways of closing the loop: (A) three kernel launches per sample; (B) one
launch per buffer, one block of 1024 threads; (C) one launch per buffer, many blocks meeting at a
barrier every sample. The machine was busy: load about 17, the GPU at 100 % with other jobs, so
the GPU figures are under contention.

Time for one 64-sample period (budget 1.33 ms), median:

| String oscillators | CPU, one thread | GPU A | GPU B | GPU C (best block count) |
|---|---|---|---|---|
| 21 120 (the piano) | **0.45 ms (34 %)** | 2.09 ms (156 %) | 4.28 ms (321 %) | 1.75 ms (131 %) |
| 84 480 | 1.42 ms (107 %) | 2.17 ms (162 %) | 16.6 ms | 4.3 ms (324 %) |
| 337 920 | 3.92 ms (294 %) | 5.5 ms (412 %) | 63 ms | 3.1 ms (232 %) |
| 1 351 680 | 25.6 ms | 6.3 ms (472 %) | 269 ms | 6.6 ms (494 %) |

- The CPU plays the piano's size in a third of a core, steadily (95 % of periods within 0.46 ms).
- The GPU does not reach real time at any size while the loop is closed every sample: each
  sample costs it about 30 µs (the launches, or the barrier) against a budget of 20.8 µs. Buffers
  of 256 change nothing: the cost is per sample.
- The GPU overtakes one CPU thread only near 340 000 oscillators, sixteen times the piano, and is
  still 2–5 times too slow there.
- Not measured: a GPU with the bridge's feedback delayed by a whole buffer (the published
  494 000 modes at 256 samples are of that kind). That is another instrument: the strings would
  hear the board 1–5 ms late.

## 8. Plan

1. One key, three strings, two directions, hammer, a single-resonance bridge: check the attack
   spectrum against velocity, the double decay, the beating.
2. The soundboard's 256 modes from an offline plate model (ribs, bridges, rim), and the tail;
   check the bridge impedance against the measured 1000 kg/s and its fall.
3. All 88 keys always running; dampers and pedals; `-inst grand-piano` on the controller.
4. Fit string damping, hammer hardness per key and unison mistuning to the solo piano recordings
   in `~/Music` (Beethoven, Schubert, Debussy tracks): parameters only, as for the bass.
5. Lengthwise motion, duplex lengths, action noise, lid.

Open choices for the user: the frequency limit of the strings (10 kHz proposed; 16 or 20 kHz
costs 1.3–1.6 times), and whether the board should be changeable at run time (then the grid).

## 9. Sources

- B. Bank, S. Zambon, F. Fontana, "A Modal-Based Real-Time Piano Synthesizer", IEEE TASLP 18(4),
  2010. https://home.mit.bme.hu/~bank/publist/taslp10.pdf
- K. Ege, X. Boutillon, "Synthetic description of the piano soundboard mechanical mobility",
  ISMA 2010. https://arxiv.org/pdf/1210.5688
- X. Boutillon, K. Ege, "Vibroacoustics of the piano soundboard", J. Sound Vib. 332, 2013.
  https://arxiv.org/abs/1305.3057
- J. Chabassier, A. Chaigne, P. Joly, "Modeling and simulation of a grand piano", JASA 134, 2013;
  INRIA report RR-8181. https://inria.hal.science/hal-00768234
- "Evaluating CPU, GPU, and FPGA performance in the context of modal reverberation", Frontiers
  in Signal Processing, 2025.
  https://www.frontiersin.org/journals/signal-processing/articles/10.3389/frsip.2025.1522604/full
- H. Renney, B. Mitchell, T. Mitchell, "There and Back Again: The Practicality of GPU Accelerated
  Digital Audio", NIME 2020. https://nime.org/proc/nime2020_39/
- KTH "Five lectures on the acoustics of the piano" (Askenfelt, Hall, Conklin).
  https://www.speech.kth.se/music/5_lectures/
- D. Russell, "The piano hammer as a nonlinear spring".
  https://www.acs.psu.edu/drussell/piano/nonlinearhammer.html
- Piano physics at Purdue (Giordano). https://www.physics.purdue.edu/piano/piano.html

## 10. As built (first version, 2026-11-14)

Files: `src/piano.h/.cc` (the model), `src/pianoinst.h/.cc` (instrument and player),
`tools/pianoboard.py` -> `src/pianoboard_data.h` (the soundboard).

- **Strings:** 243, modes to 10 kHz (bass) .. 20 kHz (from C5), in registers of 16: 12 288 per
  direction, two directions. Scale (lengths, diameters, tensions) from a rounded concert-grand
  table **[memory]**, B from the wire: 1.2e-4 (A0), 3.2e-4 (C4), 2.0e-2 (C8). Tuned by partials:
  -6 c at A0, +30 c at C8 (a real D: about -22 and +46 **[web, unverified]**: the bass stretch is
  too small).
- **Bridge:** the string's end rides on the board. The exchange is done in two exact halves per
  sample (the bridge's displacement shifts the string's modes; the modes' change of momentum is
  the board's): energy is neither made nor lost by it. A first version with the board's
  acceleration fed back as a force was unstable.
- **Soundboard:** 2.5 cm grid turned to the grain, 1.86 m^2, 10.8 kg with 17 ribs and the two
  bridges as beams along their own lines; lowest resonances 69, 110, 153 Hz; 256 resonances
  reach 2.3 kHz (one per 6.7 Hz: denser than section 4 expected), 96 broad ones to 10 kHz.
  Impedance on the long bridge 1300-3200 kg/s between 300 Hz and 2.5 kHz, 1000-1700 above
  (measured on real bridges: 1000-2000).
- **Hammers:** 10.5 g to 4.5 g; felt K and p from Chaigne & Askenfelt's three notes **[memory]**,
  hysteresis as a rate term (not Stulov's); solved implicitly each sample. Contact: 3.7 ms (C1),
  1.8 ms (C4), 0.5 ms (C8) at 2 m/s (literature: 4 ms to under 1 ms).
- **Sound:** the board in 48 patches of 20 cm, each a source in a wall, heard by two
  microphones 65 cm over the strings after its own delay; above 700 Hz each resonance's
  radiation is brought up to full (above the coincidence the patches are too coarse).
- **Decay at 2 m/s** (time to fall 20 / 60 dB): C1 4.6 s / over 30 s; C3 2.6 / 13 s; C4 2.6 /
  12 s; C5 1.9 / 7 s; C6 0.9 / 4.6 s; C7 0.4 / 2.1 s; C8 0.4 / 1.4 s. A quick first fall, then a
  slow aftersound, on every note.
- **Cost (CPU time):** one note 1.5 us per sample (7 % of a core); pedal down, all 243 strings,
  5.8 us (28 %).

Open:
- Nothing is fitted to a recording yet (step 4 of the plan): string losses, felt hardness per
  key, unison mistuning and the balance between registers are first guesses.
- The top two octaves are weak: C7 and above read about 15 dB under the middle (loudness
  over one second), C6 about 5 dB. Part of it is real (short notes, the hammer on the string
  for two periods), how much is not known.
- The bass is dull (A0's spectral centre near 250 Hz) and its tuning stretch small.
- Not built: lengthwise string motion (the bass's ring), duplex lengths, the air under the lid
  and the lid itself, the frame, the hammer's and key's noise, the left and middle pedals,
  half-pedalling.
- The strings' static pull on the bridge (T / L per string) is left out of the board.

## 11. Fitted to two recorded grands (2026-11-15)

Step 4 of the plan. Two solo piano tracks from the user's own CDs, both judged good pianos
by ear (a Beethoven track: the reference; a Debussy track: "a little darker"). Parameters only:
nothing of the recordings is in the repository or in the sound.

**Method** (`tools/pianofit.py`): no score. The tool finds note onsets, and at each onset
followed by 0.3 s or more without another, takes what the spectrum gained and matches it to a
stiff string's series n f0 sqrt(1 + B n^2). A note is kept if one key (or two) explains 80 % of
the gain with its low partials in a row. Per partial it measures frequency, level and decay
over the clear time. 267 notes (Beethoven) and 107 (Debussy) passed, A0 to G#5; nothing above
(the music's clean notes end there). The clear time is short (median 0.55 s), so the decays are
early decays; the model's notes were measured by the same code over the same 0.55 s. Tables
are medians per register of six keys; they are noisy (other notes sound on), and a quantity
was only fitted where both pianos and neighbouring registers agree. A transcription network
(downloaded) would have given keys and velocities directly; running it was not permitted in
this session, so loudness is only known as each track's own quiet / middle / loud thirds,
set against hammer speeds 0.6 / 1.5 / 3.5 m/s.

**What was fitted, and to what:**

| What | First version | Now | Recorded |
| --- | --- | --- | --- |
| Stiffness B, keys 13-20 (wound) | 9e-5 .. 1e-4 | 4.5e-5 .. 5.5e-5 | 3.7e-5 .. 5e-5 |
| B, keys 21-30 (lowest plain) | 9e-5 .. 1.5e-4 | 6e-5 .. 1.2e-4 | 7e-5 .. 9.5e-5 |
| B, C4 / A4-region / G5 | 3.2e-4 / 8e-4 / 1.3e-3 | 2.9e-4 / 7e-4 / 1.3e-3 | 2.5-3.4e-4 / 6.5e-4-1.2e-3 / 1.3-1.5e-3 |
| Tuning, bass and tenor against the middle | -1 .. -7 c | -3 .. -6 c | -4 .. -6 c |
| Decay of partials at 2-4 kHz, keys 31-48 | 6-13 dB/s (over 1.5 s) | 22-29 dB/s | 23-37 dB/s |
| Decay at 4-8 kHz, keys 43-60 | 15-20 dB/s (over 1.5 s) | 37-45 dB/s | 34-41 dB/s (Debussy 23) |
| Decay below 500 Hz, keys 37-48 | 24-33 dB/s (before the taller bridge) | 9-16 dB/s | 11-23 dB/s |
| 2-4 kHz share of a middle-loud note, keys 49-60 | -19 dB | -25 .. -28 dB | -28 .. -42 dB |

- **Stiffness** follows one law from the lowest plain string up (doubling every 8.2 keys:
  7.5e-5 at key 24); the plain strings' lengths now follow from it and the wire: 1.90 m at
  key 21 (a 274 cm concert grand's is about 1.87 m **[memory]**), 0.695 m at C4, 53 mm at C8. The wound
  bass has the least near key 13 (4.5e-5); below key 7 the recordings give too few partials
  to say, and 8e-5 at A0 is a guess.
- **Tuning:** an equal-tempered octave D#4-D5, 2:1 octaves going up, 6:3 going down (was 4:2,
  6:3 only in the low bass). With the fitted stiffness this gives the recorded pianos' curve.
- **String loss:** a term rising with frequency, 3.5e-4 /s per Hz (a Q of 9000 for the wire),
  and the f^2 term lowered to 3e-8.
- **Unisons:** 0.08 c apart at the bottom falling to 0.03 c at the top (was 0.35 .. 1.1 c).
  Wider unisons make the upper partials beat away within the half second: with the old
  spread the model's 1-4 kHz partials fell at 40-60 dB/s against the recorded 17-30.
- **Hammers:** the felt's stiffness from the middle up three to ten times softer than
  Chaigne & Askenfelt's C4 and C7 (1.3e9 at key 30, 1.7e9 at key 49, 2.5e10 at key 64; theirs
  kept in the bass and at keys 76-88, where there is nothing to fit to). Contact at 2 m/s:
  3.6 ms (C1), 2.0 (C4), 1.7 (C5), 1.2 (C6), 0.7 (C7), 0.5 ms (C8).
- **Bridge above 2.3 kHz:** rising with frequency as a mass (2500 kg/s x f / 2.3 kHz; was
  2000 falling). The upper keys' 4-8 kHz partials decayed twice too fast, and by more the
  higher the key: the mark of the bridge, not of the wire.
- **Bridge and ribs taller** (bridge 36 x 42 mm, ribs 25 mm; were 35 x 35 and 22): the low
  partials of the middle died twice too fast.

**Found on the way** (the model, not numbers):

- **The damped strings load the board.** 200 strings rest damped on the bridges; from the
  bridge each looks like a line that swallows what is sent down it (impedance
  -i Zc cot(kL) with the damper's loss). Each board resonance is damped and raised by them
  (on the first board the lowest went from 69.5 to 79 Hz, and the loss factors of the next
  ones from 0.03 to 0.05-0.085). Without it a key whose
  fundamental met a board resonance lost it at once (A2 did). A lifted damper takes its
  key's load off. Each resonance also has its loss to the air now (0.005-0.02).
- **Bridge placement.** The long bridge stood too far from the front edge in the treble
  (0.04 + 0.93 L): each key from C#5 to C6 then sat half a bending wave from the edge at its
  own fundamental, a dead spot that made that octave 5 dB weak. Now the string starts at the
  agraffe 7 cm before the board's edge; the bar runs 8 cm past its last string; the board is
  2.12 m long (1.98 m^2, 12.8 kg) so that the 1.90 m strings end 29 cm before the tail.
  Key-to-key unevenness (rms off the neighbours' mean, both microphones' power): 2.7 -> 1.7 dB.
- **The broad resonances above 2.3 kHz** are as broad as the seven or so real ones each
  stands for (loss factor 0.08), radiating the same share: single notes of the top octave no
  longer find gaps between them (one key had been 10 dB down).
- **Full radiation from 700 Hz** (rising from 350; was 1400 from 700).
- **Dampers slower** (a released middle note gone in about 0.15 s, was 0.05 s; **[memory]**,
  not measured): see repeated notes below.

**Repeated notes.** A key struck again meets what is left on its strings: the felt is pushed
by the string's own motion, and the new wave adds to some partials and cancels others. C4 at
2 m/s every 0.17 s: with the dampers up, partials 3-8 differ by 2-7 dB between blows; with
the key released 40 ms before each blow, by up to 2 dB (with the old dampers: 0.0-0.2 dB,
every blow the same). `listen/piano_repeat.wav`.

**As it stands** (2 m/s, time to fall 20 / 60 dB): C1 3.5 s / over 14 s; C2 3.2 / over 14;
C3 1.9 / 8.0; C4 1.4 / 4.7; C5 2.1 / 6.3; C6 1.0 / 2.8; C7 0.5 / 1.6; C8 0.15 / 0.85 s. One
recorded C#4 held 4 s fell 20 dB in 1.5 s and 35 dB in 3 s, without a knee: as the model.
Cost unchanged: 8 % of a core for one note, 29 % with the pedal down.

**The felt, soft outside and hard inside** (by ear, after the fit). With the fitted softness
at every force a loud note in the low middle and up kept its sound in the first two partials
(the third 10-17 dB down): hollow, "like a clarinet", and dull where a loud piano should
blaze; the bass was fine. A power law as soft as fitted stays soft however hard it is hit.
Now the felt is as fitted at a gentle blow's force (2 N a string) and its exponent is 2
higher from there (`PJ_PIANO_COLOUR`, `PJ_PIANO_PIVOT`): about twice as stiff at a middling
blow, five times at a hard one. The share of a note's sound above 2 kHz at 0.6 / 1.5 / 3.5 m/s:
keys 19-30 -46 / -21 / -19 dB (was -50 / -29 / -22); keys 43-54 -37 / -20 / -16 dB (was
-46 / -27 / -20). This is brighter than the recordings' middle and loud thirds from A4 up
(2-4 kHz: -15 dB against their -29 .. -36): the ear's choice over the fit there.

**The microphones** (the user's idea): two pressure-gradient (figure-of-eight) microphones
close together 65 cm over the strings, one turned 45 degrees to the bass side, one to the
treble side, instead of two all-round ones 80 cm apart. Each hears a patch of the board by
the cosine of its angle off the microphone's axis (behind 90 degrees: inverted). A partial
now sits in the stereo picture by where the board moves with it (left against right differs
by up to 20 dB from partial to partial of one note, some in opposite sign), and the two
channels together are one such microphone looking down: the old pair's sum had gaps from the
two distances (middle C's third and fifth partials 25-32 dB down in mono). The sound comes
from the board only, patch by patch; the strings do not radiate themselves (in a real piano
hardly either), and the hammer's and key's own knock is still not modelled.

**The sideways swing, heard** (by ear: the tone was "clarinety" in every register, also
the bass). With unisons this pure a note was one steady tone falling in a straight line: no
inner movement, like a wind instrument; the single-string bass always was. The model's
sideways swing did not help: both swings pushed one bridge motion at one pitch, so they
settled at once into the mix that moves the bridge and the mix that does not, and the long
part was silent (raising its strength changed nothing above -80 dB). Now the sideways swing
is tuned 0.5-1.5 c apart from the vertical (`PJ_PIANO_SIDETUNE`: a real bridge gives
differently the two ways), the blow sets the string swinging further off the vertical
(0.4, was 0.055), and the bridge takes 0.4 of the sideways swing (was 0.2). A note now falls
quickly, then slowly, the two parts drifting against each other: middle C's fundamental
bends 30 dB down and wavers from there; 60 dB down after 7.8 s (was 4.7 s; C3 9 s, C5
5.8 s, C6 3.5 s, C7 1.9 s). The first half second, which the recordings fix, is unchanged
(2-4 kHz: 19-30 dB/s; 4-8 kHz: 27-43 dB/s).

**Evenness, and the upper register at a hard blow** (by ear: "could be more beautiful").
The spectra there were in order; what was not: one key 4 dB over its neighbours, another 14 dB
under, some dying three times faster than the next (a note there is nearly all fundamental,
so it takes whatever the board and the microphones do at that one frequency). Three changes:
- the felt's extra hardening tapers from 2 at key 40 to 0.8 from key 64 up (little difference
  to the partials: there a hard blow's contact is set by the hammer's mass);
- the plate's resonances from 250 Hz up are made up to three times broader (at 1 kHz and
  above), radiating the same share: a stand-in for the denser, broader resonances of a real
  board, whose bridge mobility is smooth there; the low ones stay as computed;
- the microphones stand 50 cm from the keyboard's end of the board (were 75): the treble
  bridge is within 20 cm of that end, and from further along the board its sound arrived by
  paths that cancelled at some notes.
Level of the first 0.3 s at 4 m/s, rms off the neighbours' mean: keys 1-44 0.9 dB (was 0.9),
45-66 1.0 dB (was 2.4), 67-88 1.7 dB (was 2.8); the largest step between neighbours 4.3 dB
(was 9.4). The top octave is 6-10 dB under the middle (was 10-20).

**A third recording** (a Schubert track, 8 minutes; added after the fit as a check, nothing
changed for it): 357 notes measured, the most of the three. Stiffness and tuning as the
other two (B 4.0e-5 at keys 7-12, 2.0e-4 at 31-36, 4.7e-4 at 43-48; -5 .. -7 c in the bass,
+1 c at keys 55-60). Decays as fitted (2-4 kHz: 21-33 dB/s; 4-8 kHz in the middle 45-48 dB/s,
faster than the Beethoven's 30-41: the model's 31-37 lies between the three). In brightness
it sits with the Debussy: 2-4 kHz at -38 .. -42 dB of a middle-loud note through the bass and
middle, where the Beethoven has -22 .. -31 and the model -27 .. -35: the model is voiced
like the brightest of the three.

One doubt the third recording raised: at keys 49-54 both the Beethoven and the Schubert read
B = 1.2e-3, nearly three times the 4.2-4.7e-4 six keys lower and off the law (7.4e-4); the
Debussy reads 6.5e-4. Four times the stiffness an octave lower is what a note read an octave
too high would give (its even partials only), so those two readings are probably the
finder's octave mistakes, and the law is kept.

Still open:
- The treble (keys 49-60) is still brighter than both recordings in its 1-4 kHz partials (by
  2-15 dB), and softer felt alone does not close it; nothing is known above G#5.
- The fundamentals of F2-G#2 (the low end of the long bridge) are weak (20 dB under the
  second partial).
- The slow aftersound: with unisons this pure a note falls in one straight line; the
  recordings' clear stretches are too short to show whether theirs do.
- Loudness of single notes in the recordings (hammer speed) is not known; a transcription
  would give it.
- As before: lengthwise string motion, duplex lengths, lid and air under it, frame, action
  noise, the other pedals.

**The second partial in the middle** (the user: "there could be in the middle octaves a bit
more of the 2nd partial"). Measured on keys 35–48 (G3–G#4) against the first partial, the model's
second stood about where the recordings' does (median −3 dB in both), but its third to sixth
stood some 8 dB over the recordings', so the second was outshone. What moves the second alone
is little: the hammer's width, the pivot, the board's loss and the strings' load each under
1 dB. Moving the microphone pair 15 cm towards the bass side gave +2.4 dB but brought back the
upper register's unevenness (1.4 → 2.1 dB, largest step 4.3 → 8.5 dB), so they stay. Kept:
- the board radiates fully from 500 Hz, rising from 250 (was 700 from 350): the band of these
  keys' second partials;
- the microphones three quarters figure-of-eight, a quarter all round (`PJ_PIANO_MICPATTERN`
  0.75, was 1).
Together the second partial gains 1.3–1.7 dB on the first and as much on the third, at every
speed; the evenness is as before (0.9–1.0 and 1.5 dB, largest step 4.8 dB); the level rose
1.5 dB (trim 12.0). Cost 9 % of a core for one note, 34 % with the pedal down. The stronger
lever, left alone, is a softer felt in the middle: it takes the third and higher down by more.

**Later the same day, two more remarks of the user's.**
- "My 183 cm grand rings quite a bit longer ... could it be that it damps energy too quickly?"
  Yes. Held notes at 2 m/s fell 40 dB in 5.2 s at C4 (60 dB in 9 s), 4.3 s at C5, 8.5 s at C3:
  after the first seconds the fall was about 6 dB/s where a grand's aftersound falls 2–3. The
  fit had only the first 0.55 s of the recordings' notes, which the bridge sets; the wire's own
  loss, which sets the aftersound, was never pinned by it. Now `PJ_PIANO_LOSS` 0.05 (was 0.12)
  and `PJ_PIANO_LOSS2` 2e-4 from key 40 up (was 3.5e-4; the wound bass keeps 3.5e-4 at key 16
  and below, `PJ_PIANO_LOSS2BASS`: with less, C1's upper partials outlast its fundamental).
  To −40 dB now: C1 17 s (was 11), C2 10 (9), C3 11 (8.5), C4 12 (5), C5 6 (4), C6 2.6 (2.5);
  to −60 dB: C5 12 s (6), C6 8 (4), C7 4 (2), C4 and below over 20 s. The first half second
  falls 4.9 dB at C4 (was 5.7). `listen/piano_ring.wav`: C2, C4, C6 and a chord held.
- "The microphones could include more 'differential' resonances from the sound board, to make
  it sound more spatial." The pattern is back to a pure figure-of-eight (0.75 had narrowed the
  picture: the difference of the channels 1.5–4 dB lower against their sum in the lower five
  octaves), and the pair is turned 60° to either side (was 45°): the difference signal 1.7 dB
  higher against the sum than at 45°, the evenness as before (0.8 and 1.4 dB, largest step
  4.6 dB). 75° gives 3.5 dB but empties the sum. Of the second partial's gain the board's part
  stays: +1.4 dB at 1.5 m/s on keys 35–48. Trim 13.9. Cost 8 % and 30 %.

**Three more of the user's remarks, the same afternoon.**
- "The grand piano could use a bit more reverb, perhaps 5 dB." The grand's sound goes into the
  room 5 dB stronger than the others' (`PJ_PIANO_REVERB`, `src/ensemble.cc`); its direct sound
  and first reflections are unchanged. In the hall the reverberant field stands 6 dB under the
  rest of the piano's sound (was 11). Later: "perhaps 4 dB less reverb for the piano": the send is now
  1 dB (10 dB under).
- "Perhaps a bit too much odd partials." Found in the octave below middle C, and it was the
  board, not the hammer. Re-measured on the recordings with the transcribed notes (the
  network's keys; `py/tr3.py` in the scratchpad), partials 1–5 of A2–G#3 read −6, 0, −7, −11,
  −14 dB; the model's −14, −1, 0, −14, −16: its first partial 8 dB weak and its third the
  strongest. (The earlier finder had also taken some notes here an octave low, which made the
  recordings look even less odd than they are: −8.9 dB odd against even where the transcribed
  notes give −5.7.) Microphone place, pattern, string load, board loss, felt: each under 2 dB.
  The cause: the patches carry only 0.27 of a full radiation below 300 Hz, and the ramp that
  makes it up began at 250 Hz. Now `RAD_FROM, RAD_FULL = 50, 120`: partials 1–5 there −6, 0,
  −2, −14, −19; odd against even −1.8 dB (was −0.7); an octave higher −7.4 (the recordings
  −5.8 to −8). The second partial above middle C gained another 3 dB by it. The bass and the
  middle are 4–5 dB louder against the top two octaves than before (trim 9.5); C1 and C2 now
  read clear (0.89, 0.97) and at pitch. Still open: the third partial below middle C is 5 dB
  over the recordings', and A1–G#2 has partials 3–7 some 10 dB over them (the recordings are
  soft playing there).
- "At the lowest velocity it should be only the fundamental and perhaps some of the 2nd
  partial ... and this should ramp into the spectral richness through a complex path that
  would have bends." Measured first: from C4 down the low partials hardly moved with velocity
  at all (C4's second from −10 to −6 dB, C3 nothing), since the single power law is a straight
  line in the logarithm. Now the felt has a nap (`PianoKey::nap`, `napK`): a loose outer layer,
  0.9 mm at C4 and below (0.6 at first), a spring of 1200 N/m a string at C4 that stiffens without limit as it
  is pressed flat, lying on the felt proper (two springs in a row, solved for the force). The
  softest blows never get through it: a long gentle push. From about velocity 20 to 50 it is
  flat within the blow and the felt under it takes over: the bend. Thinner and firmer up the
  keyboard (halving twice an octave above C4, stiffness x 5.7 an octave), softer below
  (`PJ_PIANO_NAP`, `NAPK`, `NAPTHIN`, `NAPSLOPE`, `NAPSLOPEBASS`).
  C4, partials 2 and 3 against the first: velocity 1 −27, −26 dB; 20 −23, −30; 40 −12, −16;
  65 −8, −10; 127 −7, −7. C3: −5, −16; −1, −4 (20); +1, +2 (40). C5: −29, −37; −24, −46;
  −15, −27; −10, −17; −6, −13. The softest notes are 6 dB quieter than before. C3 keeps a
  second partial 5 dB under the first at the softest: a contact long enough to take it out
  would be longer than the hammer stays on the string. `listen/piano_velocity.wav`.
  The listening files are now rendered in the hall (the program's usual room), not the studio.
- "The low velocity highest octave or 1.5 octaves can be darker, less of even the 2nd
  partial." From key 76 up the fitted felt is hard (K 1e12 and more) on a light hammer, and
  soft notes there kept a second partial 23–30 dB under the first. A thicker nap does not
  help (a contact longer than the note's period takes the fundamental away as well). Instead,
  rising over keys 68–76 and full above: the felt at a gentle blow 0.3 times as stiff
  (`PJ_PIANO_KHIGH`) and its exponent 2 higher above the pivot (`PJ_PIANO_COLOURHIGH`).
  Second partial against the first at velocities 1, 15, 30, C7: −45, −33, −30 dB (was −26,
  −26, −23); D#7: −50, −39, −28 (−32, −30, −23); C8: −76, −63, −44 (−32, −47, −45); from
  velocity 60 up as before (−21 to −11), and the notes' levels within 2 dB of before at
  every velocity.
