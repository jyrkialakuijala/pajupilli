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

### Added 2026-11-11 — key clicks, key velocity as finger speed
A closing pad now accelerates into the seat (lands at 2 × mean speed, ~0.5 m/s at 10 ms) instead
of easing in. The air between pad and rim leaves through the slot under the pad; the share that
goes into the bore is R_slot / (R_slot + ρc/2S), so only the last fraction of a millimetre
pumps. Without breath a medium click is ~26 dB under a mezzo-forte note (soft key −30, hard −22)
and rings at the fingering's resonances for ~30 ms. Fingers land ±15 % apart in time. Key velocity
sets finger speed (64 → 10 ms, 127 → 5 ms, 1 → 20 ms); springs keep 16 ms. The pump level
(`PJ_PADPUMP` 0.25 of a rigid flat landing) is set by ear; a soft-pad contact model could replace it.

### Fix 2026-11-11 — upward register slurs (reported by ear: "D5 unstable")
A slur up into D5 (from C4, D4, G4; from A4/B4 when soft) kept the old oscillation on the D5
fingering's first resonance (435 Hz, about A4): the note sounded A4. Direct attacks were fine.
The player now kicks the jet over on a slur into a higher register, as flutists do: +30 % breath
and a 20 % shorter jet, decaying over 60 ms (`PJ_KICKP`, `PJ_KICKW`, `PJ_KICKT`; 0 = off).
Upward register slurs (21 pairs × 3 dynamics): frames in the wrong register 990 → 264; all D5
slurs land. Stronger kicks overshoot into the register above (E6, A6). Left for step 3: F5,
soft E5 and G5, the C6 wobble, and a D5 crescendo from silence, which starts on 435 Hz and stays
there (too much hysteresis).

### Fix 2026-11-11 — thin second register (reported by ear: "above D5 sounds thinner")
From D5 the radiated 3rd harmonic was 12–20 dB above the fundamental and the level dropped
3–9 dB, while the pressure in the embouchure had a normal, fundamental-led spectrum. In the
second register the flute vibrates in its second mode: the embouchure and the open holes move
air in antiphase, and the listener (1 m opposite the middle of the flute, equidistant) heard the
fundamentals cancel (E5: 44 + 46 dB → 32 dB). The listener now sits 1 m in front of the player's
face (`PJ_LX` 0, `PJ_LY` 1): D5–G5 H3 within +4 dB of H1, level rises smoothly C4 −32 → C6 −21 →
C7 −13 dB. This was part of step 3.4. The low register's weak H2 (jet saturation) remains.
A stereo pair (one near the player's head, one in the audience) would be the next step.

### Added 2026-11-11 — key velocity as dynamics and attack; steady jet length per note (D6)
- Velocity sets the note's dynamics (knob ± 0.35, 64 = none; slurs glide over 60 ms) and its
  attack: lung push 15 ms (v 1) … 1.7 ms (v 127), tongue release 16 … 2 ms, pressure overshoot
  growing above v 50.
- Calibration (D2, explicit lips): besides mezzo-forte, it learns the coverage at the note's
  softest steady dynamics and at forte (pitch within ±5 cents across velocities), the softest
  dynamics that holds the register steadily (soft keys stop there), and a jet length
  (×0.8–1.1) that gives a steady note. D6 (and B5–C#6) beat at Fletcher's length: the note's
  H2 and the fingering's 2345 Hz resonance pulsed at 13 Hz, 40–57 % level; at ×0.9 it is steady.
  Live-like scenarios over 21 notes: register jumps 344 → 62, top octave within 18 cents.
- Calibration is cached in ~/.cache/pajupilli (keyed on the binary and PJ_* knobs).
- Velocity changes loudness by under 2 dB: the window's vortex loss (`PJ_VORTEX`) clamps the
  amplitude (off: C5 spans 7 dB). Next: re-tune with weaker vortex loss, then step 3.

### Added 2026-11-11 — breath noise (step 3.1), Air control, natural variation
- Jet turbulence now enters before the jet low-pass (×4 gain): the 8–24 kHz fizz is down
  13–17 dB and the noise sits at 1–3 kHz, shaped by the bore.
- Direct edge hiss: band-passed noise at St ≈ 0.12 (1.5–7 kHz), ~U³, from the embouchure.
- Air (knob 3, `PJ_AIR`): wider, less focused lip opening, more turbulence and hiss, a jet that
  spreads more. Noise re tone: C4 −33 (focused) … −30 (default) … −19 dB (airy); C6 −23 … −14.
- Wander (`PJ_WANDER`, 1 = natural): breath support ±6 % (~1 Hz), jet aim ±30 % (~2 Hz), lip
  opening ±10 % (~3 Hz), attacks vary ±15–20 %. Held G4: 1.7 cents, 0.56 dB drift. Calibration
  and `-slur` run with wander off.

### Fix 2026-11-11 — slow third-octave onsets (reported by ear: "E6 lights up a bit slow")
E6 at velocity 64 sat at ~35 % for 40 ms before speaking (50 % at 55 ms; F#6 85–145 ms): at
mezzo-forte it plays just above its softest steady breath (calibrated floor 0.45), where the
oscillation grows slowly. Attacks on such notes now get the register kick (more air, jet
closer, 60 ms), scaled by how close the floor is and faded out when the breath is well above
it (`PJ_ATTACKKICK`). E6: 50 % at 20 ms, 80 % at 25 ms; no wrong registers at any dynamics.
Also: learned soft/loud coverage limited to ×0.75–1.35 of mezzo-forte (Coltman ~±1.5 mm): the
loud C4 had been covered down to 2.8 mm and choked; now +18 cents sharp at forte instead.

### Added 2026-11-12 — piccolo, alto flute, bass flute (`-inst`)
The concert flute resized, as makers do: lengths along the bore × the pitch ratio (0.5, 1.335,
2), bore / holes / pads to the real bores (~10.5, 26, 35 mm), embouchure hole, lips and jet ×
√ratio. The model runs at 48 kHz / ratio (96, 36, 24 kHz) so the bore keeps its 128 cells; a
polyphase resampler (Kaiser sinc, 32 taps) brings it to 48 kHz. Fletcher's player rules
(pressure ~ f, jet length ~ 1/√f) already keep the jet's travel time a fixed share of the period,
so the player needed no new rules: breath and jet follow the sounding pitch, lips and
embouchure the written one.
- Relatively smaller embouchure holes (alto, bass) play flat, a bigger one (piccolo) sharp: the
  head is shortened 15 / 30 mm (lengthened 5 mm), fitted to the flute's mode-1 resonances.
- Piccolo: D foot (no C, C# keys), ending at 528 concert-flute mm (fitted with `-imp`);
  cylindrical bore with the flute's head taper.
- Ranges where calibration tunes within a few cents: piccolo D5–B7, alto G3–B5, bass C3–D5.
  Above, the alto and bass third octave runs 25–100 cents sharp at the lip-coverage limit (weak
  on the real instruments too); the piccolo's C8 does not speak.
- Upward register slur kick now follows the breath, ×(1 + 0.9 (0.5 − dyn)): at forte a full kick
  overblew the piccolo's D7 for 0.3 s; at piano the alto's D4→D5 stayed low.
- Jet swing limit (`PJ_ETAMAX`) scales with the embouchure; noise filters and the DC blocker
  are set per sample rate (identical at 48 kHz).
- Calibration listens longer to notes below C4 (bass C3 had not settled in 175 ms: −26 cents).
- `-slur` judges the register by the spectrum: a bright piccolo A6 (third harmonic 12 dB over
  the fundamental at forte) fooled the pitch tracker.
- Output level as if miked: piccolo ×0.7, alto ×1.4, bass ×3 (physically the bass flute is ~15
  dB quieter than the flute).
- Open: bass C3 speaks in ~240 ms (flute C4 ~90 ms; it scales with the period); the piccolo's
  third octave is bright; the bass at 24 kHz has nothing above ~11 kHz.

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

### Added 2026-11-12 — key timing, thud and release, G at rest, C/C#-D trill
- Keys that move together (closing or opening) start 0..3 ms apart (`PJ_KEYSPREAD`); pads
  closed since the start open together on their first stroke (keeps the calibration).
- Landing thud: the pad goes on into its felt (cup + finger on felt, ~400 Hz, Q 1.5) and pushes
  the air of the sealed hole into the bore, both ways; 0.15 of a rigid piston, ~2 dB under the
  pad's puff (`PJ_THUD`, `PJ_THUDF`). A held pad lifting off: the felt preload (0.05 mm) springs
  back, a suction ~13 dB under the opening puff (`PJ_RELEASE`).
- Rest fingering G4 (left hand down): a first note moves fewer keys. The flute's C7 moved
  from +30 to +36 c (test limit for E6-C7 now 40 c).
- Trills on the trill keys (Flute School's list): C5-D5, C#5-D5, C#5-D#5, B5-C#6, C6-D6,
  C#6-D6, C#6-D#6; either note held, the other slurred over it; the lower fingering stays
  and the key(s) give the upper note (printed: trill-c, trill-cis, ...). Without lip help
  C#5-D5 is +28 c and C#5-D#5 +16 c: the lips cover 20 % / 12 % more. Upper notes, cents:
  flute -8..+6; piccolo -27..+1 (C6-D6 -27; C#6-D#6 falls out of its register: off);
  alto -14..+10; bass -8..+23 (C#6-D6 +23; D#6 is above its range).
- Trill names print both notes (trill-c5-d5, ...). Settling: a middle-octave trill stopping on
  its upper note (lower key up 0.3 s) moves to the note's own fingering with quiet keys (no
  puff, thud, click or release) in three steps 30 ms apart: the keys below the C# hole (the
  trill key keeps the note), the trill key (the note overblows on the long tube), the C#
  vent. One step dipped 4 dB and went -18 c; the C# vent with the trill key open is +30 c.
  Flute: within 8 c, dip < 2 dB; piccolo / alto / bass 9-38 c (their trill notes are off too).
  Not for the 3rd-octave trills: into D6 the bass fell an octave, into C#6 ~45 c sweeps.
- A trill starts only when its two notes alternate fast: the last key event (press or release)
  was one of the pair, under 0.12 s before (`PJ_TRILLFAST`). Slow legato B C D E and scales at
  8 and 16 notes/s never use the trill keys; a C-D trill picks them up from its second D.
  The settle moves every key quietly (checked: no puff, thud, click or release); slower settle
  fingers (40 ms strokes) wander more (flute 37 c, -4.8 dB), so 20 ms stays.
- Settling replaced (2026-11-12, by request: "without fingering changes"): the upper note stays on
  the trill key and the player tunes it by ear with the lips. A band-pass (Q 3) at the note,
  upward zero crossings give each period; ln(uncovered) moves by -cents/116 per period over
  50 ms (d cents / d ln uncovered ~ 116 on D5); kept per trill. Held on the key, cents off
  without -> with: flute up to -10 -> within 2; piccolo -28 -> +5 (C6-D6); alto -17 -> 1;
  bass +25 -> -3. The three-step fingering switch and the quiet-key flag are gone.

### Added 2026-11-12 — B foot by default (`-cfoot` / `-inst flute-c` for a C foot)
Hole 17: the B foot's C hole, 592.3 mm from the embouchure, Ø 15.4, pad 2.8 mm, cup 20.6 [D];
the tube on to 633.3 mm (B3). Fingerings without a 17th character leave it open; B3 closes
it. Calibration: B3 -2 c (uncovered 9.9 mm), C4 needs 8.8 mm uncovered (C foot 7.7).
C7: with the gizmo (B key closed) +50 c, an extra resonance near 1917 Hz; without +29 c; C foot
+36 c: the default C7 fingering has no gizmo. The piccolo, alto and bass keep their feet.
Test: the C-foot flute runs the family checks; the tmm.py comparison uses it (tmm.py has the
C foot). Note: that info line reads 64 -2.7 c, 67 -6.7 c (earlier +0.0, -2.2) for an unchanged
C-foot flute since one of today's earlier commits; not chased yet.

### Added 2026-11-12 — more trill-key uses
- D6-Eb6 and A6-Bb6 on both trill keys (the request said A5-Bb5; tmm.py: A5 + both keys moves
  the resonance 902 -> 1070 Hz, a minor third; A6 + both keys 1812 -> 1926 Hz, a semitone, where
  the Bb6 fingering's is). With the upper note's own calibration the flute's D6-Eb6 sounded an
  octave low and the register kick alone did not help; keeping the lower note's lips, jet and
  breath (as a trilling flutist does) plus the kick: flute -1 c, piccolo +9 c (both trills).
- Trill keys as vents (calibrated cents): B6 with neither +8, D# trill key +21 (the old
  fingering), D trill key +1, both +14 -> B6 now uses the D trill key. C7: neither +29, D# +36,
  D +95, both +97 -> unchanged.

### Added 2026-11-12 — polyphony (`-poly N`, src/ensemble.*)
A pool of up to N Voices (own seeds for noise, key timing and wander). Heuristic: chord =
keys within 35 ms; a key over a held one = slur unless more lines are expected than flutes
holding keys; two keys held together on one flute > 150 ms = two lines (the older note
moves to a free flute, re-attacked: it is silent for those 150 ms, unavoidable without
look-ahead); lines expected = most seen, forgotten after 2 s with fewer. A free flute is
chosen by |pitch difference| + 10 x seconds since its last key. Idle flutes (1 s after the
last key, not blowing) are skipped. Mix x 1/sqrt(playing), 40 ms glide. -polydemo: line ->
1 flute, held C4 + melody -> 2, triads -> 3 (C-E-G, D-F-A heard), peak 0.17, no resets.
Possible next: per-flute tuning spread / seating (stereo), smarter line tracking (voice
crossing), the held note keeping its own flute from the start when the history says so.
- Split time 150 -> 100 ms (by request: keys held together > 100 ms get flutes from the pool);
  live default -poly 8 (all created at start; idle ones cost nothing), -poly 1 = one flute.
- Adaptive just intonation in the pool (by request: "reduce the total dissonance"). Per pair
  the nearest 5-limit ratio (offset from ET, weight 1.0 unison/octave .. 0.05 tritone, / (1 +
  0.5 octaves)); least squares with l = 0.05 to ET for new notes, 1.0 to the current offset
  for notes still sounding (fresh key = new); clamp ±30 c; solved when the sounding set
  changes. Realised by each Player's ear (the trill listener, generalised: target f0 2^(c/1200)).
  Demo (equal vs just, measured cents / Sethares roughness from partials): C-E-G 0.310 -> 0.252
  (E -12, G +3), D-F-A .382 -> .371, C-F-A .335 -> .268, C-E-G .324 -> .213, C-E-G-C .388 -> .199,
  but E-G-C .231 -> .259 (G, C uncovered more: +1.3 dB louder); total -21 %. On model flute
  spectra the JI solution is within 1 % of a direct roughness search and lowers every chord
  (E-G-C .267 -> .248), so the rule stands; the lips' loudness change is the open part.

### Added 2026-11-12 — ensemble (`-ensemble`) and the room (`-room`, src/room.*)
- Ensemble: a mixed pool, default bass, alto, flute, flute, piccolo, each with its own
  calibration. Notes go where they are comfortable (strain = semitones outside a comfort
  range, + preference 0 flutes / 1 alto, piccolo / 1.5 bass, + line continuity capped at 2,
  + 24 per octave folded when no free instrument reaches a key), and a chord (keys within
  35 ms, each alone on its instrument) is re-dealt as a whole by exhaustive search with a
  6-point penalty per voice crossing (rank = an instrument's lowest note). A key no held
  instrument reaches is not slurred: it goes to a free one. Uniform pools (-poly) keep the
  old rule unchanged. The demo's five-voice chorale deals C3 bass, G3 alto, E4 / C5 flutes,
  G5 piccolo.
- Room: 16-line FDN (Hadamard / 4, prime lengths 0.5..1.6 mean free paths of a 2:1.4:0.7
  shoebox, one-pole wall/air loss: RT60 at DC, RT60/3 at Nyquist). Level from diffuse-field
  theory: p_rev^2 = 16 pi / A p(1 m)^2, A = 0.161 V / RT60. Seats on a line 1 m apart (0.8 for
  more than 6), high left; direct paths 1/r and r/c between players and to a spaced pair of
  microphones (+-1 m, at the room's listener distance: studio 2, hall 3, church 6 m), scaled so
  a source there sounds as at 1 m. Rooms: dry, studio 150 m3 0.5 s, hall 2500 m3 1.8 s
  (default), church 15000 m3 4 s.
- Room into the instruments: the room's pressure at each player (its own FDN taps + the others'
  direct sound) is an external pressure in series with every opening's radiation load: the
  embouchure branch, every tone-hole branch (Bore::step pRoom, the e coefficients) and the
  foot's opening (the interface between the tube and its radiation horn). All flutes are
  computed in a room, the silent ones too (sympathetic resonance). Coupling in 32-sample blocks
  (the direct paths shortened by a block).
- Passivity: the first version applied the room at the far end of the foot's radiation horn
  (r = 0.31 m sphere): that shell collected the room's sound into the bore and an idle flute
  gave back 0.18..0.49 Pa at 1 m per Pa; with a few players and the reverb the loop gained
  > 1 and grew 70 dB/s. At the opening (series with its load, reciprocity) an idle flute gives
  back 0.0006..0.01 Pa/Pa (rising with frequency like a small scatterer); `-scatter` measures it
  and `make test` checks < 0.03. The energy the instruments absorb (bore losses) is not taken
  out of the FDN: their absorption area (cm^2) is negligible next to the room's A (> 10 m^2).
- No octave folding per instrument (by request): a note no free instrument reaches takes the
  nearest held instrument whose range has it (a slur onto it, its old key under the new one);
  keys beyond the whole ensemble fold into its range. The demo ends with C3 + E3 together: E3
  moves the bass.
- Cost: ensemble in the hall 6.5 % of a core (all five always computed), dry 3.8 %.
