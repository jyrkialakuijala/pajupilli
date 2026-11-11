# pajupilli — requirements

A real-time physical model of a modern concert flute (Boehm system, C foot), played live
from an Akai MPK mini mk3. Status: draft, 2026-11-11. Open questions are marked **(Q)**.

## 1. Goals

1. Sound comes from physics: an air jet driving a waveguide bore with tone holes, not samples
   and not additive synthesis. Pitch, timbre, attack, overblowing and breath noise should come
   out of the model, not be painted on.
2. Interactive and streaming: play it live from the MPK mini mk3 with low latency.
3. Fast and simple: the bore is 128 pressure points at 48 kHz, stored as 8 × 16 float32 so the
   inner loops compile to AVX-512 (one zmm register per block of 16).

## 2. Acoustic model

### 2.1 Formalism
- R1. The bore is a 1-D acoustic waveguide (transmission line): pressure p and volume flow u on a
  staggered grid, leapfrog in time (the formalism of puhuja's `line.h`). Area varies along the
  bore (Webster horn equation).
- R2. 128 pressure points, 48 kHz audio rate. Courant number c·dt/dx just below 1 (≈ 0.99):
  stable, nearly dispersion-free. At c ≈ 347 m/s that is dx ≈ 7.3 mm, so 128 points span ≈ 0.93 m.
  If substeps are needed for stability or accuracy, the output rate stays 48 kHz. **(Q)**
- R3. The flute bore (cork to foot end, 618 mm measured on a Pearl PF-661) takes ≈ 85 points. The remaining ≈ 40 points
  model radiation beyond the foot: an expanding (spherical-wave) horn with a non-reflecting end,
  which gives the right radiation impedance and a listening point.

### 2.2 Geometry (values to be fixed by the research, §6)
- R4. Cylindrical body bore 19.0 mm; head joint taper from 16.8 mm at the cork to 18.8 mm at 113.5 mm
  below the embouchure centre (research/keywork.md).
- R5. Cork 17.5 mm above the embouchure hole centre (the closed end).
- R6. Embouchure hole 11.6 × 10.3 mm, undercut, chimney 4.3 mm; lip coverage (≈ 40–65 %)
  changes its effective area and is the per-note tuning knob.
- R7. Tone holes at real Boehm-flute positions and diameters, with real chimney heights and
  open-key (pad) heights. Holes are lumped side branches (inertance + loss) at their nearest grid
  point. A closed hole still adds its chimney volume.
- R8. Keys move: each hole has an openness 0…1 that slews over a few milliseconds (pad travel), so
  fingering changes are continuous. Half-holing and venting work.

### 2.3 Excitation (the player)
- R9. Jet-drive model (Verge / Fabre / de la Cuadra): jet velocity from blowing pressure, convective
  delay over the lip-to-edge distance W, jet deflection by the acoustic flow in the window, a
  saturating (tanh) flow split at the labium, the dipole pressure source, nonlinear vortex losses.
- R10. Player controls: blowing pressure, W, lip coverage, jet offset y0, breath noise
  (turbulence). Overblowing to the 2nd and 3rd registers by raising pressure and shortening W, as
  flutists do.
- R11. The player's airway (decided from research/player_and_controller.md): lungs as a slewed
  pressure set-point with vibrato; the lip slit as nonlinear resistance + inertance whose area is
  the main dynamics control; the tongue as a fast valve (2–10 ms; flutter at ≈ 25–30 Hz); the mouth
  as one 50–100 cm³ compliance; Reynolds-number breath noise. A glottal valve (added 2026-11-11):
  throat vibrato, "h" breath attacks, glottal turbulence. Optional later: a weak bore→mouth feedback. Not modelled: articulated vocal tract, nasal tract, trachea, vocal folds
  (the folds only for a later sing/growl effect, from puhuja).
- R12. Articulation: tonguing (fast pressure gate on note start), slurs (fingering change only),
  vibrato (pressure modulation ≈ 5 Hz).

### 2.4 Output
- R13. Radiated sound = sum of d(flow)/dt of the open holes, the embouchure and the foot/horn
  outlet; DC-blocked; mono float32 at 48 kHz.
- R13a. Bore losses frequency dependent (≈ √f) with their dispersion (Bilbao & Harrison 2016), since
  they shift C4 by ≈ 18 cents.
- R14. Range: C4–C7 (the flute's written range). First octave by fingering; second by
  overblowing (D5, D♯5 with the C♯ key as vent); third octave by real cross fingerings. The third
  octave may come in a later milestone.
- R15. Tuning to A4 = 440 Hz (adjustable) within ±10 cents with the nominal player settings.

## 3. Real-time / interaction

- R16. Streams audio to ALSA (works through PipeWire's ALSA plugin). No allocation, locks or I/O
  in the audio path.
- R16a. Latency, key press to first sound at the output: < 20 ms (plan: 64-frame periods × 2–3,
  ≈ 3–4 ms buffering; MIDI read every period; the remaining budget for USB MIDI and the audio
  server). "First sound" is the breath/tongue transient; the tone itself then builds up at the
  speed a real flute does (tens of ms), which is physics, not latency.
- R16b. An optional "expressive" mode with more delay (e.g. 30–60 ms look-ahead): the player model
  sees what comes next and can choose slur vs. tongued attack, prepare the fingering before the
  breath, shape the release, and pre-set the embouchure (W, pressure) for an upcoming register
  jump, as a flutist anticipates.
- R17. Reads MIDI from the MPK mini mk3 (ALSA sequencer, auto-connect by name). Proposed map
  **(Q)**:
  - Keys: note → fingering. Velocity → attack (tonguing strength, initial pressure overshoot).
    Held legato (overlapping) notes → slur without re-tonguing. Monophonic, last-note priority.
  - Joystick: Y → breath pressure / dynamics (expression); X → pitch bend as the flutist's lip
    roll (coverage / jet angle), not a frequency fudge.
  - 8 knobs: breath pressure, W, lip coverage, breath noise, vibrato depth, vibrato rate,
    tuning (A4), output gain.
  - Pads: register/overblow selection, harmonics (overblow without changing fingering),
    flutter tongue, key clicks (Q).
  - Sustain pedal: keep blowing after key release.
- R18. Also runs offline: render a note list or MIDI file to WAV, for tests and listening A/B.

## 4. Code

- R19. C++17, one small project, `make`. The Makefile builds for speed: `-O3 -march=native
  -mprefer-vector-width=512 -ffast-math -fno-finite-math-only` (AVX-512 with full 512-bit vectors;
  fast-math for vectorised reductions, but NaN/Inf checks still work for the output guard).
  `make scalar` builds a portable reference without AVX-512 for R22.
- R20. Bore state and per-point coefficients are `alignas(64) float[128]` arrays; inner loops run
  over the whole 128 with no branches; boundary and special points use zero/identity
  coefficients rather than ifs. Scalar code only for the jet and the boundaries, once per sample.
- R21. No intrinsics required; check the generated assembly (zmm) to confirm vectorisation.
- R22. A plain scalar build (`-O2` without AVX-512) gives the same audio within float rounding.
- R23. Every new model idea is a knob (environment variable), off by default until tested (as in
  puhuja).

## 5. Verification

- R24. Stability: 60 s of random fingerings and pressures, no blow-up, no NaN.
- R25. Tuning table: played pitch for each fingering against equal temperament, in cents.
- R26. Passive input impedance of each fingering compared to published measurements (UNSW flute
  acoustics data) for the first few resonances.
- R27. Real-time margin: the model must run at well under 10 % of one core.
- R28. Listening files for each change; Jyrki's ear decides between close variants.

## 6. Research (done 2026-11-11; reports in research/)

1. Keywork: positions, diameters and chimney heights of every tone hole; pad heights (how far each
   key opens); which keys close which holes; ring keys (French model) vs plateau; the standard
   fingering chart for C4–C7.
2. The jet: typical W, jet thickness h, blowing pressure per register, lip coverage, Strouhal
   ranges, jet-drive model equations and parameter values.
3. The player's airway: does the vocal tract / mouth / lungs / glottis audibly affect a flute?
   What do the published measurements (e.g. Coltman; Wolfe and colleagues at UNSW) say? Decide what
   to model.
4. Losses: viscothermal losses in a 19 mm bore; radiation from open holes and the foot.
5. MPK mini mk3 default MIDI map (knob CCs, joystick, pads, aftertouch).
