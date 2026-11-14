# A concert grand: research and model plan

Status: research only (2026-11-14). Nothing here is implemented yet.

The instrument: a concert grand (the Steinway D is the one the literature measures), GM
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

- **Strings [memory]:** 88 keys, 243 strings on a Steinway D: keys 1–8 one wound string, 9–13
  two wound, 14–20 three wound, 21–88 three plain steel. Speaking lengths about 2.0 m (A0) to
  5 cm (C8). Tension 600–1300 N per string, about 20 tonnes in all.
- **One measured string [web, Chabassier RR-8181]:** D#1: L = 1.945 m, steel area 1.31 mm²,
  T = 1328 N, E = 202 GPa, ρ = 7850 kg/m³.
- **String impedance [web, Askenfelt via Ege]:** about 10 kg/s (bass) to 5 kg/s (treble).
- **Bridge impedance [web, Ege & Boutillon; Conklin; Giordano]:** about 1000 kg/s from 100 Hz to
  1 kHz (near a rib 1000–2000, between ribs 600–700), falling to 200–300 kg/s by 2.5–10 kHz.
  A hundred times the string's: the string gives its energy slowly, hence the long sustain.
- **Inharmonicity [web, range only]:** partial n at n·f0·√(1 + B n²), B from about 10⁻⁴ in the
  low-middle to 10⁻² at the top. A Steinway D's tuning stretch: about −22 c at A0 to +46 c at C8
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
  25 % of his cost. Second stage.
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
3. All 88 keys always running; dampers and pedals; `-inst grand-piano` on the MPK.
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
