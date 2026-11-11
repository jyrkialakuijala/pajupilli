# pajupilli — improvement plan

From the review of 2026-11-11 (commit 701a84a). Five reviewers worked independently, each with
their own experiments on a scratch copy of the code. Their reports are in `review/`:

| Report | Area |
|---|---|
| `numerics.md` | The waveguide: stability, accuracy, losses, horn, grid |
| `jet.md` | Jet, excitation, timbre, dynamics, noise, attacks |
| `geometry.md` | Bore, tone holes, fingerings, intonation, cross-check with Dickens' code of the same flute |
| `player.md` | Articulation, slurs, expressivity, MPK mapping, live loop |
| `code.md` | Vectorisation, performance, real-time safety, robustness, tests |

## Where we stand

**What holds up (checked, keep):**
- **Waveguide numerics.** The scheme is passive and was stable over 600 s of random playing.
  Dispersion is under 0.4 cent below 4 kHz, and the loss fit is within ±5 %. With the same geometry,
  the time-domain model matches tools/tmm.py within 1 cent.
- **AVX-512.** Both bore loops compile to clean 512-bit FMA code. The model costs 0.8 % of a core,
  12× inside the budget.
- **Geometry and fingerings.** The bore and holes agree with Dickens' model of this Pearl flute
  within about 7 cents of slope. All 37 fingerings match the standard charts.
- **Jet parameters.** Convection at 0.175 Uj without Fletcher's 1.8 mm dead length gives his
  half-period jet delay on every note.

**What is wrong, in order of how much it hurts the sound:**

1. **Registers aren't chosen the way a flute chooses them.** The jet gain is flat in frequency,
   so modes 3, 5 and 7 meet the same phase condition as mode 1, with sharper resonances. Effects:
   - G4 is bistable with mode 5 across its normal blowing range.
   - At low pressure C4 and G4 sound mode 2 or 3 before the fundamental, and softer breath makes
     C4 jump *up* to C5. This is backwards.
   - Upward octave slurs take 100–240 ms, and D4→D5 never arrives.
   - A5 drops an octave at +10 % W or −20 % pressure.
   - G6 never sounds.
   - The second octave is 14–22 dB quieter than A4. (jet F1, player F1–F2)
2. **The low register sounds like a clarinet.** The jet swings 4× past saturation, so the flow
   split is a 50 %-duty square wave: odd harmonics at about 0 dB, H2 at −26 to −42 dB, even at pp.
   The same saturation makes the dynamic range only 2–8 dB in the low register. (jet F2)
3. **Breath-noise bug.** Turbulence enters after the jet low-pass and is differentiated twice, so
   the hiss climbs to 24 kHz. I re-measured this: in the first 60 ms of a C4 attack most of the
   energy is above 12 kHz. (jet F3)
4. **Tuning comes from a non-physical embouchure.**
   - Calibration leaves 9.7 mm of C4 uncovered, more than the jet length; Coltman measured 6 mm.
   - It uses pressures up to 2×.
   - Played notes sit 21–34 cents *below* the passive resonance, where Fletcher et al. 1982 put
     them slightly *above*.
   - E6–C7 are sharp: their passive resonances are already 16–29 cents high, and pitch climbs
     65 cents per pressure doubling. (geometry F1–F3, jet F4)
5. **Three grid shortcuts add tuning errors that the calibration now hides:** (numerics F1–F3)
   - The horn's end correction is 0.94a instead of 0.61a, and it over-radiates above 1 kHz.
   - The foot is 2.8 mm too long.
   - The hole-position compensation corrects only about 40 % of the snapping error.
6. **Attacks and articulation.**
   - Onsets take 50–117 ms to reach 90 %, against 10–30 ms on a real flute.
   - G5 and C6 attacks squeal 15–18 dB above the steady tone.
   - Accents are inaudible: their 40 ms push is over before the tone builds.
   - "tu" and "du" are nearly identical.
   - The MPK's velocity range (mostly < 64, max about 100) barely reaches the accent (> 90) and
     pedal re-tongue (≥ 100) thresholds, and under the pedal a hard hit gives a soft "du".
   (player F4–F5, jet)
7. **Live robustness.** (code, player)
   - A single NaN silences the flute until restart. I confirmed: the output guard zeroes it, but
     nothing resets the model.
   - If the ALSA device goes away, the real-time thread spins at 100 %.
   - The buffer isn't refilled after an underrun.
   - The MPK doesn't reconnect after an unplug.
   - Port rescans and `fprintf` run in the audio thread.
   - Startup calibration takes 1.7 s.
   - The first touch of a knob jumps, and volume changes aren't ramped.
8. **Smaller items.**
   - Octaves are about 36 cents wide passively. This weakens H2 and the slurs. It isn't numerical,
     and it isn't settled whether a real flute stretches that much.
   - A closing pad rings at 24 kHz for about 1 ms.
   - FTZ/DAZ (flush-to-zero) are only on by accident of `-ffast-math`.
   - `-imp 30` and `-dump` to a bad path crash.
   - The listener position cancels even harmonics, costing 8 dB of H2 at C4.
   - Vibrato restarts on every tongued note.
   - The joystick bends only ±10–25 cents.

**Where the reviewers disagreed:**
- **Jet convection speed.** The geometry reviewer suspects 0.175 Uj, since the literature has
  0.3–0.5, because played notes sit below resonance. The jet reviewer finds 0.175 consistent with
  Fletcher's slowed jet. Both point at the same place: the jet's phase response. Step 3.2 settles
  it with a frequency-dependent convection speed, judged on two tests: the register map, and
  played pitch slightly above the passive resonance.
- **Fixing the tuning slope.** A larger constant embouchure correction (the current 2 mm) flattens
  the slope only by making octaves wider and everything flat. The geometry reviewer, backed by
  Dickens, says part of the slope is real and players fix it with a per-note lip/face load.

## The plan

Each step lands as knobs, off by default, then becomes the default after a listening A/B
(the puhuja process). Every step is checked with `make test` (step 1) before and after.

### Step 0 — Robustness (½ day, low risk) — DONE 2026-11-11, except the calibration cache
- NaN/Inf latch: if a block isn't finite, reset the flute state (and the player's) and continue.
- Live loop:
  - stop on an unrecoverable PCM error instead of spinning;
  - refill the buffer after an underrun;
  - move MIDI port scans and verbose printing to a helper thread with a lock-free queue;
  - hot-replug the MPK by watching ALSA announce events.
- Set FTZ/DAZ explicitly. Bounds-check the CLI (`-imp`, `-dump`, `-o`).
- Startup: run the per-note calibration in parallel threads (1.7 → 0.3 s) and cache it in a file
  keyed by a hash of the knobs.
- Radiate (q⁺ + q)/2 for hole flows, to remove the 24 kHz click when a pad closes.

### Step 1 — `make test` (½ day) — DONE 2026-11-11 (tools/test.py, ~10 s)
Before more model work, lock the current behaviour into checks that run in about 10 s:
- the bore loops are vectorised (zmm FMA present in `Bore::step` specifically);
- 3 × 60 s fuzz: no NaN, peak bounded;
- recovery after an injected NaN;
- tuning table against a golden file;
- register selection: each note's dominant frequency matches its harmonic;
- scalar vs AVX build agree on pitch and level;
- real-time margin under 400 ns/sample;
- time-domain model vs tools/tmm.py, first three resonances within 3 cents for C4–C5;
- no allocation while rendering;
- a sanitizer run.

Tests encode decisions; when a change is intended, update the golden file and say why.

As built: the tuning check allows 5 cents for C4–D#6 for now (F5 sits at +4 after the radiation
change; step 3.5 tightens it to 3). The waveguide-vs-tmm.py comparison is reported as "info" until
step 2 lands (now: C4 −13.5, G4 −6.7, A4 +5.9, C5 +13.0 cents). Not yet automated: the
no-allocation check and the sanitizer run.

### Fix 2026-11-11 (evening) — the 7 Hz pumping (reported by ear: "C5 not very stable")
After a soft slur into C5 the tone warbled: ±7 cents and ±60 % level at 7.3 Hz, for as long as
the note was held. Mouth pressure and jet speed were steady (0.2 %); the loop was inside the jet:
the window flow's slow part went through the 15 Hz leaky integral into the jet displacement, moved
the jet's mean aim, which changed the mean flow split and the window flow again. A real jet is
deflected by a slow cross-flow only in proportion to its speed (~ v W / Uj), so the displacement
drive is now high-passed at 100 Hz (`PJ_XIHP`, 0 = old behaviour).
- Soft slur B4→C5: wobble 6.8 → 0.1 cents, level 123 → 3 %.
- Across 59 live-like scenarios × 21 notes: unstable 296 → 92.
- Default dynamics: F#5…D6 had been stuck in the pumping state at 1/10 of the level; all sound now.
- F#5 now uses the middle-finger fingering (a standard alternate): the ring-finger F# holds its
  second register only in a sliver of breath × coverage in this model.
- New: `./flute -slur A B -dyn d` and a `slur stability` check in `make test`.
- Still failing (step 3, register selection): soft G5 (dyn ≤ 0.3) and loud F5 (dyn 0.8) jump to
  the 3rd/6th resonance; B5→C6 keeps a 4–5 cent, 35–50 % wobble. The soft G5 and F5 cases were
  quiet pumping notes before; they are now wrong-register notes, which is more audible.
  Lowering the jet low-pass (`PJ_JETLP` 1.5, 1.2) makes registers worse.

### Step 2 — Exact geometry (½ day, low risk) — numerics F1–F3, geometry F4
- Make the foot length exact: Courant 0.99446 instead of 0.99 (dispersion stays negligible).
- Fit the horn's throat radius so the foot end correction is 0.61a. The horn keeps all 128 points
  in use (see decision D1).
- Per-hole position compensation fitted against tmm.py, plus the embouchure cell offset.
- Add the omitted negative series term on holes (+6…+15 cents, slope −4 cents). Set
  `tools/tmm.py`'s default extra length to match the time-domain model's.
- Air at playing temperature (Dickens: 30 °C, humid, 2.5 % CO₂ → c ≈ 349.5 m/s, +12 cents),
  rather than 27 °C.
- **Target:** all first-octave resonances within 3 cents of tmm.py. The test from step 1 enforces it.

### Step 3 — The jet (2–3 days, the core of the sound) — jet F1–F5
1. **Noise path.** Inject turbulence *before* the jet low-pass, with 3–5× more gain to keep the
   hiss level. Add a direct edge-hiss source that isn't shaped by the bore, and raise the relative
   noise at pp. Small effort; removes the 12–24 kHz fizz.
2. **Frequency-dependent jet.** Make the jet's convection speed and growth depend on the Strouhal
   number, cp(St) and αi(St) (fit to de la Cuadra 2007). Build `tools/ramp.py`, the register map
   from a slow pressure ramp, as the acceptance test:
   - low pressure sounds the fundamental;
   - overblowing comes at higher pressure, with 15–30 % hysteresis;
   - no mode-5 islands;
   - played pitch at or slightly above the passive resonance;
   - onsets in 10–30 ms with no squeal.
3. **Growth-limited jet.** Replace the hard 1.5 mm clip of the jet swing with an amplitude-limited
   growth, so the jet is not driven 4× into saturation. Then the jet aim can produce even
   harmonics (aim at about 0.3–0.6 × swing).
   - **Targets:** C4 mf H2 within 6 dB of H1, H3 −8…−15 dB; a pp→ff range of at least 15 dB in
     the low register; spectra richer when loud.
4. **Listener.** Move it toward the embouchure, as the player hears it, or add a second, offset
   listening point. This gives back the 8 dB of H2.
5. **Physical calibration.** Search coverage within Coltman ±1.5 mm, jet length within Fletcher
   ±20 % and pressure within ×0.85–1.25. Aim for the *middle* of each note's stable range, not the
   first point that works. Reject quasi-periodic states, and log what remains as model error
   instead of hiding it.
- **Expected:** registers behave, the low register sounds like a flute rather than a clarinet,
  real dynamics, the third octave reachable with in-range lips.

### Step 4 — Player and articulation (1–2 days) — player F1–F8
- **Register kick.** When a slur or attack goes up a register, briefly add about +30 % pressure and
  −20 % jet length (tested: median settle about 170 → 50 ms). After step 3 it may need less.
- **MPK velocity curve.** Thresholds relative to the keyboard's real range, and the pedal re-tongue
  logic fixed.
- **Attacks.** Time the accent push to the tone build-up (60–70 ms), and give "tu" and "du"
  different seals and release times. Vibrato keeps its phase across tongued notes.
- **Dynamics and the ear.** Make jet length and aim depend on dynamics. Add an intonation reflex:
  the player corrects pitch drift with the lips over about 150 ms, within ±40 cents, using a
  learned per-note table rather than an FFT.
- **Controls.**
  - wider joystick bend (roll plus partial venting);
  - knob defaults at the knob's centre, with pick-up;
  - ramped volume;
  - the breath pad able to reach zero breath, as a proper breath-controller mode.

### Step 5 — Expressive look-ahead mode (1–2 days) — R16b, player F9
An optional 30–60 ms delay that lets the player:
- prepare register jumps 10–20 ms before the keys move;
- repair ragged legato (an overlap or a short gap becomes a slur);
- choose t-k double tonguing for fast repeated notes;
- taper phrase endings before rests.
Off by default; the normal mode stays at about 10–15 ms.

### Step 6 — Instrument refinements (as needed)
- An embouchure riser modelled as a cone, plus a per-note face/lip load as in Dickens. The riser
  shape moves the tuning slope by ±25 cents, so this is the most sensitive part of the model.
- Losses in the hole chimneys and nonlinear losses at the hole edges, for realistic resonance Q.
- Check the head-taper profile, the largest contributor (+14 cents) to the octave stretch.
- Alternative fingerings: split E, 1-and-1 B♭, trill keys, harmonics (overblowing a held
  fingering), whistle tones. Fix or remove the unused `harmonic` column (D#6 is the 4th mode).
- Code clean-up: dead knobs and paths (`PJ_JETMODEL=0` plays A4 at +755 cents, `PJ_REL`, the sign
  knobs, `held_`), rebuild only the cells whose keys moved (9× cheaper key motion).

## Decisions for Jyrki

- **D1 — 128 points.** Replacing the radiation horn with a lumped open end would be within 1 cent
  of theory and leave 43 cells unused. Options:
  - (a) keep the horn with a fitted throat (recommended: it uses all 128 points, radiates
    physically, and gives the listening point);
  - (b) a lumped end with 96 cells;
  - (c) a lumped end, using the spare cells for something else (e.g. a mouth/tract line later).
- **D2 — How much tuning should the player do?** A real flute plus a fixed embouchure has a
  45–70 cent slope over the first octave. Options:
  - (a) model the player's per-note lip/face adjustment explicitly, as Dickens does (recommended);
  - (b) keep correcting it inside calibration.
- **D3 — Spectrum targets.** `tools/timbre.py`'s targets are my estimates. Digitising Fletcher
  1975 (Figs 3–5) or using UNSW recordings would give measured targets. Your ear stays the final
  judge.

## Order and effort

| Step | Effort | Why this order |
|---|---|---|
| 0 Robustness | ½ day | Live use must not die or spin |
| 1 Tests | ½ day | Protects everything after it |
| 2 Geometry | ½ day | Cheap, and stops the calibration hiding errors before the jet is retuned |
| 3 Jet | 2–3 days | Biggest gain in sound and playability |
| 4 Player | 1–2 days | Builds on a jet that selects registers properly |
| 5 Look-ahead | 1–2 days | R16b |
| 6 Refinements | ongoing | Driven by listening |

Steps 0–2 are mechanical and I can do them without supervision. From step 3 on, every change ends
with listening files for an A/B, as before.
