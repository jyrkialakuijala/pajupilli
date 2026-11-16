# Building the cymbal: a plan

Status: 2026-11-15. A plan; nothing is built. It rests on `research/jazz_drums.md` (§3 cymbals,
§8.2 model, §8.5 order, §8.6 cost) and narrows it to one object: a cymbal that can be struck, rung,
choked and heard, first a 22" thin jazz ride, then the crash, splash and china from the same code.
The hi-hat (two cymbals in contact) and the kit around it are separate plans.

Marks: **[jd]** taken from jazz_drums.md; **[calc]** worked out here; **[design]** a choice of this
plan, to be proven by the step's own check.

## 1. What is built

- **One cymbal = one object that lives through the whole song.** Every stroke on it (bow, bell,
  edge, any key) goes into the same ringing plate. The ride's wash is then what it is on a real
  one: many strokes piling up in modes that live for seconds **[jd §3.6]**. A voice per note
  cannot do this, so the cymbal is not played through the note pool.
- **Its shape is data.** Diameter, bell radius and height, bow profile, thickness from centre to
  edge, alloy, hammering, rivets, the felt it sits on. A tool turns the shape into modes
  (`tools/cymbal.py` → `src/cymbal_data.h`), as `tools/pianoboard.py` does for the soundboard.
  Ride, crash, splash and china are four shapes, not four models.
- **Three layers, built and heard in this order:**
  1. the linear plate: all its modes to 16–20 kHz, struck by a stick;
  2. what touches it: the stick's own ring, rivets, the hand (choke), the felt;
  3. the nonlinearity: the low modes coupled as a thin shell's are, and the cascade they feed.
  A jazz ride played with the tip is almost wholly layer 1 **[jd §1]**; layer 3 is what makes a
  crash a crash.

## 2. Where this plan differs from jazz_drums.md §8.2

| | jazz_drums.md | here | why |
|---|---|---|---|
| Mode frequencies and shapes | drawn from the flat free plate's series, stretched for the dome, scattered at random | computed from the cymbal's own profile and taper: a ring-by-ring shell model, one small eigenproblem per number of nodal diameters | Bell, taper and dome decide what a cymbal is **[jd §3.5: "shape matters as much as taper"]**. The cymbal is round, so the exact problem is one-dimensional per angular order and costs seconds offline **[calc]**. Strike points and microphones then get true shapes, not Bessel guesses. |
| Radiation | a weight per mode | each mode's sound at two overhead microphones, computed from its shape (both faces, opposite sign) | Below about 14 kHz a 1.2 mm bronze plate's bending waves are shorter than the sound's **[calc: coincidence at c²√3/(π c_L h)]**: front and back cancel except near the edge. Which modes are heard, and how loud, is set by this, and it tilts the whole spectrum. |
| The nonlinear force | coupling tensors of a flat plate, scaled | first an offline reference that works the stretching out on a grid over the real shape; the real-time form (sparse tensors or the grid itself) is chosen after measuring both | A flat plate's tensors are not a domed, tapered shell's (jazz_drums.md §10 lists this as open). The reference settles it before any real-time code is written. |
| Stroke-to-stroke variety | — | each stroke lands a little elsewhere (a few mm, a few degrees) | The modes' shares at the strike point change with place; a drummer never hits the same point twice. It is the physical answer to the machine-gun effect. |

Everything else (mode count, damping law, core + cascade sink, rivets as contact masses, order)
is as jazz_drums.md has it.

## 3. Steps

Each step ends in something to hear (`listen/cymbal_*.wav`) and a check that can fail.

### Step 0. Targets and cost (a day)

- **Cost per mode.** Time the piano's string bank (the same rotate-and-decay step, AVX2):
  ns per mode-sample. jazz_drums.md assumes 0.2–0.4 ns; 1900 modes would then be 2–4 % of a
  core. If it is twice that, the top band is thinned (§5).
- **What a ride is, in numbers**, from the drum stems of the jazz tracks in `~/Music` (as for the
  bass and the piano: separated in the scratchpad, numbers only into the repo) **[jd §9]**:
  - band decay times (third octaves, 100 Hz–16 kHz) after the last stroke of a phrase;
  - the steady wash spectrum under ride time;
  - ping against wash: each band's level in the 60 ms after a stroke against just before it;
  - spectral centre against time for a crash at a phrase end (rise 0.1–0.3 s **[jd §3.5]**).
- Check: the tool run on a synthetic cymbal (known modes and decays, mixed with noise) returns
  them within 10 %.

### Step 1. The shape tool: `tools/cymbal.py`

- **Input:** radius, bell radius and height, bow height, thickness at centre and edge and how it
  thins, E, ρ, ν, the hole, hammering (depth, count, seed).
- **Model:** a thin shell of revolution cut into ~200 rings. For each count of nodal diameters
  m = 0 … ~80: bending and in-plane motion together (the dome couples them: this is what stiffens
  the low modes), free edge, a generalized eigenproblem of a few hundred unknowns.
- **Hammering:** small dents break the roundness. To first order each pair (cos mθ, sin mθ)
  splits by a fraction of a percent to a few percent and turns to its own angle; the dents'
  angular spectrum gives each pair's split **[design]**. Heavier hammering: more beating, less
  pitch **[jd §3.1]**.
- **Damping per mode:** the law μ = a f^0.6 + b **[jd §3.3]** fitted to step 0, plus the felt at
  the centre (by how much the mode moves there) and the air (by how well the mode radiates).
- **Output per mode:** frequency, loss, modal mass, its shape at the strike rings (bell, bow at
  three radii, edge) and at the rivet holes, its sound at each microphone (a gain and a delay).
- **Checks:**
  - a flat uniform disc gives the free plate's textbook series;
  - the count below 20 kHz is within 10 % of the density formula (1930 for the 22" ride) **[jd §3.2]**;
  - an 18" crash's shape gives Rossing's low modes: 64, 99, 145, 188 Hz for 3–6 diameters
    **[jd §3.2]**, within 10 %;
  - bell modes exist and are nearly deaf to the edge (their shape there is small).

### Step 2. The linear ride: `src/plate.h/.cc`, `-inst ride-cymbal`

- **`Plate`:** up to 2048 modes in aligned arrays, the piano strings' exact per-sample step,
  modes asleep below −100 dB.
- **The stick:** a mass (12 g at the tip **[jd §3.4]**) on a Hertz spring against the plate's own
  give at that point, solved each sample as the piano hammer is. Tip (hickory or nylon) or
  shoulder; MIDI velocity is its speed, 0.3–8 m/s.
- **Played live** from the MPK to judge it by ear: keys across the plate from edge to bell,
  velocity the stroke; two pads for shoulder and choke. In a MIDI file: 51 bow, 53 bell, 59 the
  second ride.
- **Sound:** the two overhead microphones of step 1, into the room as every instrument.
- **Checks:**
  - band decay times within 20 % of step 0;
  - wash builds: eight strokes a second for four seconds settle 6–12 dB over one stroke's tail
    in 300 Hz–2 kHz, and the 3–6 kHz band falls between strokes (the gap the ping lands in)
    **[jd §3.6]**;
  - the bell is pitched and clear, the edge is dark and wide: bell against bow spectral centre;
  - energy: with losses off, a struck plate's energy holds to 1e-4 over 10 s;
  - real time: ride time at 240 bpm under 6 % of a core.
- **Hear:** `listen/cymbal_ride_time.wav` (spang-a-lang, soft to loud), `cymbal_bell.wav`,
  `cymbal_places.wav` (edge to bell). This is the first point where the user's ear decides; the
  shape (thickness, bell size, hammering) is retuned here before anything is added.

### Step 3. What touches it

- **The stick's ring:** its three bending modes (324, 892, 1749 Hz for a 40 cm hickory stick,
  damped by the hand in 20–50 ms) driven by the blow: the wood in the ping **[jd §3.4, §8.2]**.
- **Rivets** (`-inst sizzle-ride`): 6–8 loose masses of 0.2–0.5 g at holes near the edge, each
  under gravity on a one-sided contact, hearing the modes below 6 kHz **[jd §3.8]**.
  Check: they rattle after the softest stroke and stop abruptly when the plate's acceleration at
  the hole falls under g.
- **Choke:** the hand on the edge: every mode damped by how much it moves under the hand, 50–100 ms
  to silence. Aftertouch on the key, or a pad.
- **The stand:** the cymbal rocks on its felt after a stroke (a slow tilt, ~1 Hz, by the stroke's
  momentum and place). The tilt turns the plate against the microphones: the slow breathing of a
  rung cymbal **[design]**. Cheap (two rigid-body states); dropped if it cannot be heard.

### Step 4. The nonlinearity

- **4a. Offline reference** (`tools/cymbalnl.py`, numpy): the low N modes (N to 400) of step 1,
  the stretching of the middle surface worked out on a rings × angles grid over the real shape
  (dome and taper included, so both the cubic term and the dome's quadratic term come out of one
  calculation), energy-conserving time step. Slow; it is the truth the real-time form is judged by.
  - Checks against the literature: on the published flat test plate, 100 N gives pitch bends and
    300 N a broadband spectrum that clears again **[jd §3.5]**; on the tapered, domed 16" crash,
    the spectral centre against time follows the published trends for edge thickness 1, 0.7,
    0.4 mm, and a bell strike stays linear at 90 N.
  - It also answers how many modes the core needs: raise N until the 0–3 kHz result stops
    changing.
- **4b. Real-time core** in `Plate`, on the lowest 100–200 modes at 24 kHz, off whenever the
  motion is under a tenth of the thickness (most of a ride's life) **[jd §8.2]**. Two forms,
  both measured against 4a for accuracy and time, the cheaper kept:
  - sparse coupling tables computed by the reference (about N_Ψ N² × 10 % operations a step
    **[jd §3.9]**);
  - the grid itself in C++: modes → grid by rings (one small matrix per angular order) and a
    64-point FFT round each ring, the stretching there, and back. Its cost grows with N, not N²
    **[calc, rough: ~10⁵ operations a step at N = 160, 24 rings]**, so it wins if the core has
    to be large.
- **4c. The cascade above the core:** the core's top band loses energy by an extra damping that
  grows with its own energy; exactly that power is handed to the linear modes above as random
  forcing with a falling spectrum **[jd §8.2, a design, not a published method]**. Fitted to the
  crash targets of step 0 and to 4a run with a large N.
- **Checks:** energy conserved to 1e-3 with losses off (core + sink + upper modes); no blow-up at
  velocity 127 on the edge for 60 s of random playing; the crash's 3–8 kHz band peaks 0.1–0.3 s
  after the stroke; a soft ride stroke is unchanged from step 2 (within 0.5 dB per band).
- **Hear:** `cymbal_crash.wav` (soft to hard, edge), `cymbal_ride_accent.wav` (the ride crashed
  with the shoulder).

### Step 5. The family, and the MIDI drum keys

- **Shapes:** 16" and 18" thin crash (49, 57), 10" splash (55), 18" china (52: the turned-up edge
  is in the profile; no special code), a second, drier ride (59). Each fitted to its targets where
  the recordings have one, else to the maker-type data in jazz_drums.md §3.1.
- **In `-play`:** the cymbal keys of channel 10 routed to these objects (now "not built" in
  `drumFor`), a level trim each, velocity gain as for the other struck instruments.
- **Cost guard:** at most two nonlinear cores at once; the older falls back to linear **[jd §8.6]**.
- **Tests** in `tools/test.py`: modes and decays against the stored data, energy, stability,
  real time with ride time plus a crash every bar.

## 4. Files

| File | What |
|---|---|
| `tools/cymbal.py` | shape → modes, shapes, losses, microphone gains; writes `src/cymbal_data.h` |
| `tools/cymbalnl.py` | the offline nonlinear reference; writes the core's tables if that form is kept |
| `src/plate.h/.cc` | the mode bank, stick, contacts (rivets, hand), core, cascade |
| `src/cymbal.h/.cc` | the cymbals as data (`CymbalDesign`), the instrument (live keys, drum keys) |
| touched: `src/instruments.cc`, `src/midifile.cc` (`drumFor`), `src/levels_data.h`, `tools/test.py`, README | registration, routing, trims, tests |

The touched files are ones the other session edits; those changes are agreed with it first, and
`levels_data.h` is edited by hand, not re-emitted.

## 5. Risks

- **The mode count is too dear.** Above ~8 kHz single modes are not heard as such (20–40 in every
  critical band). Fallback: keep every mode below 8 kHz, every second or third above with its
  mass adjusted to hold the band's energy. Decided by ear at step 2.
- **The shell tool is wrong where it matters** (the bell's join, a china's flange are not
  "shallow"). The ring model is written for a general shell of revolution, not the shallow
  approximation; the checks of step 1 catch the rest.
- **The cascade closure does not sound like a crash.** Then the core grows (the grid form makes
  400 modes thinkable) and the closure carries less.
- **Separated drum stems are poor above 10 kHz** (the separator's own artefacts). Targets up there
  come from passages where the cymbal rings alone (endings), and from the published decay laws.
- **Hammering as a first-order split** may be too tame for a heavily hammered, "trashy" ride. A
  second-order mix between neighbouring angular orders is the next term.

## 6. Open, for the user

- Which cymbal is the first target: the 22" thin, dark jazz ride (assumed here), or one heard on a
  particular recording in `~/Music`?
- The other session wrote jazz_drums.md and edits the drum routing: which session builds this?
