# Synth leads from analog circuits: requirements and research plan

Status: requirements only (2026-11-13). Nothing here is implemented yet.

General MIDI programs 81–88 are the "synth leads": Square, Sawtooth, Calliope, Chiff, Charang, Voice,
Fifths and Bass + Lead. They are to be built as this project builds its winds. Each lead is a physical
system whose sound comes from its dynamics: here an analog circuit, simulated at the component level.
It is not built from stored waveforms or phase accumulators. The intervals inside a patch and between
voices are to be just (pure ratios), not equal-tempered.

This document says what the leads must do and how we find the circuits that oscillate the right way.
It does not choose the final circuits; that choice is the output of the research it describes.

## 1. Goals

1. **Born oscillation.** Each oscillator is a circuit that starts oscillating on its own when powered
   and given a control voltage: a relaxation oscillator (an integrator and a comparator, a capacitor
   and a reset switch), or a harmonic oscillator whose amplitude a nonlinearity limits. The pitch, the
   waveform, the start-up and the drift follow from component values and the circuit's equations.
   They are never set directly.
2. **One circuit per GM lead.** The circuit is an oscillator core, waveshaping, a filter, an amplifier
   and envelopes. It is chosen so the result is what a listener expects of that GM program (section 4).
3. **Just tuning.** The intervals inside a patch are exact ratios: Fifths is 3:2 and Bass + Lead is 2:1.
   Chords between voices are tuned just, as the flute ensemble already does. Where we can, the ratio
   should emerge physically from coupling between oscillators (injection locking, sync), not only from
   arithmetic on the control voltage (section 5).
4. **Playable like the winds.** The leads use the same keyboard, ensemble, room and live loop
   (`src/live.cc`, `src/ensemble.*`, `src/room.*`). They implement the `Performer` interface and are
   calibrated per note the way `src/calibrate.cc` calibrates a wind instrument.
5. **Real time.** Each voice is cheap enough for an 8-voice pool on one core (section 7).

## 2. Non-goals

- Cloning a specific commercial synthesizer's sound or panel. Published circuits are research
  material (topologies, component ranges, measured behaviour). Our circuits are designed for the GM
  roles, not copied.
- Sample playback, wavetables, or additive resynthesis of recordings.
- Digital-era sources such as DCOs with divider chains, FM chips or PCM, except as comparison points.

## 3. Terms

- **Core:** the part of the circuit that oscillates (for example an exponential converter, a
  timing capacitor, a comparator and a reset switch).
- **Expo converter:** a matched transistor pair that turns a control voltage into a current
  exponentially. At 25 °C it gives about 18 mV per octave (V_T ln 2), and it drifts with
  temperature unless compensated.
- **Locking:** an oscillator pulled to an exact ratio of another's frequency by weak coupling,
  within a capture range (Adler 1946; Pikovsky, Rosenblum & Kurths 2001, "Synchronization").
- **Reference simulation:** a SPICE transient analysis of the same netlist. It is the ground truth
  our real-time solver is measured against, as the transfer-matrix model `tools/design.py` is the
  reference for the bore waveguide.

## 4. The eight leads: what each must sound like, and candidate circuits

What each GM name implies is a requirement. The circuit in the third column is a starting candidate
for the research, not a decision.

| GM | Required character | Candidate circuit to study |
|---|---|---|
| 81 Square | Hollow, odd harmonics. Variable pulse width is welcome. Clean attack. | A sawtooth or triangle core into a comparator with a PWM threshold. Or an astable multivibrator (two transistors, cross-coupled RC) whose duty cycle follows the timing-capacitor ratio. |
| 82 Sawtooth | Bright, all harmonics, the classic lead. A resonant low-pass sweep on attack. | A sawtooth core: an expo converter charges a capacitor, and a comparator triggers a fast discharge (reset). Into a 4-pole transistor-ladder low-pass with resonance (Moog 1969 ladder; Stilson & Smith 1996; Huovilainen 2004). |
| 83 Calliope | A breathy, flute-like whistle: near-sine with noise, slight pitch instability. | A sine-shaped core: a triangle through a differential-pair shaper, or a Wien-bridge or phase-shift oscillator whose amplitude a diode or lamp limiter sets. Noise from a reverse-biased junction, gated by the envelope. |
| 84 Chiff | A pipe-organ-like attack transient ("chiff") on a mellow tone. | A narrow band-pass noise burst at onset (filtered avalanche noise and a fast envelope) into a mellow pulse or triangle tone. The burst should come from the circuit's start-up where possible: a resonant filter pinged by the gate. |
| 85 Charang | An overdriven, guitar-like lead with compressed sustain. | A sawtooth or pulse core into an op-amp or diode-clipper overdrive (Yeh, Abel & Smith 2007), then a tone stack or low-pass. Two slightly detuned cores beat, unless locked. |
| 86 Voice | A sung "ah/oo" lead: formants. | A pulse or saw core into parallel resonant band-passes at vowel formants (state-variable or Sallen-Key sections); the formants follow the note slightly. |
| 87 Fifths | Two oscillators a fifth apart sounding as one. | Two cores, the upper coupled to lock at exactly 3:2: soft sync, or injection through a small capacitor from one core's reset pulse into the other's timing node. Shared filter. |
| 88 Bass + Lead | A sub-octave bass under a bright lead. | A saw core and a flip-flop divider (÷2) for the exact octave. Or a second core locked at 1:2. Mixed before a resonant low-pass. |

Requirements common to all eight:

- **Envelopes and amplifier:** an ADSR built from capacitor charge and discharge through
  transistor switches, and a transconductance (OTA) or transistor amplifier. The attack and
  release are shaped by the circuit's RC curves.
- **Glide:** an RC lag on the control voltage, driven by the same articulation rules as the winds
  (`Performer::articulate`: overlap with a softer key gives a glide).
- **Imperfection:** component tolerances (±1 % resistors, ±5 % capacitors), the expo pair's
  temperature, leakage and noise give each voice its own small offsets. This plays the part that
  the player's "wander" plays for the winds, and makes an ensemble of identical leads never sound
  identical.

## 5. Just tuning

1. **Inside a patch:** Fifths sounds 3:2 exactly (+1.96 c above an equal-tempered fifth). Bass +
   Lead sounds 2:1 exactly. Where the ratio comes from locking, it must stay locked across the whole
   playable range, all dynamics and glides. If coupling alone cannot hold it, the control voltage
   pre-sets the ratio within the capture range and the coupling holds it.
2. **Between voices:** reuse the ensemble's just-intonation solver (`src/ensemble.cc`: pairwise
   simple ratios, a least-squares offset per voice, held notes anchored). For an analog lead the
   offset is a control-voltage trim in the expo converter, not a lip correction.
3. **Measured, not assumed:** each voice's pitch against its control voltage is calibrated per note
   (temperature, tracking error, high-frequency droop from the reset time), so a just offset of a few
   cents is actually realised. A requirement: pitch error after calibration is within 2 c over the
   lead's range.
4. **Physical locking as a research question:** locking between voices (two Fifths voices, or a lead
   and a pad) is a possible feature. Pure ratios would then emerge as they do in coupled organ pipes.
   The research measures the capture range against coupling strength (Arnold tongues) before anything
   is designed on it.
5. **Optional modes:** 5-limit just intonation around a tonic chosen by a knob, as well as the
   ensemble's chord-local tuning.

## 6. How we find circuits that oscillate the right way (research plan)

The method mirrors the woodwinds: a slow, exact reference model, a fast real-time model, and a
design tool that searches the reference for good component values, the way `tools/design.py` places
tone holes.

### 6.1 Sources

- **Topologies and component ranges:** published schematics and service manuals of classic monosynths
  and their VCO, VCF and VCA sections. Datasheets and application notes of synth ICs (CEM3340,
  SSM2040/2044, CA3080, LM13700). Analog Devices' and TI's application notes on expo converters and
  relaxation oscillators.
- **Virtual-analog literature:**
  - Stilson & Smith 1996 (BLIT, the Moog filter);
  - Välimäki 2005 (DPW sawtooth);
  - Välimäki & Huovilainen 2007 (antialiasing oscillators);
  - Brandt 2001 (hard sync without aliasing);
  - Huovilainen 2004 (nonlinear Moog ladder);
  - Zavalishin, "The Art of VA Filter Design";
  - Yeh, Abel & Smith 2010 (the DK method: automated nonlinear state-space models from circuits);
  - Holters & Zölzer 2015 (generalised state-space derivation);
  - Werner et al. 2015 (wave digital filters with several nonlinearities);
  - Parker, Zavalishin & Le Bivic 2016, and Bilbao et al. 2017 (antiderivative antialiasing);
  - Esqueda, Välimäki & Bilbao 2016 (BLAMP).
- **Synchronisation:** Adler 1946 and Pikovsky, Rosenblum & Kurths 2001, for locking and capture range.

Each source used is cited in `research/synth_leads.md` with what was taken from it.

### 6.2 Reference simulator

- ngspice (open source; not installed on this machine yet: `apt install ngspice`) runs each
  candidate netlist as a transient analysis. Device models: 2N3904/2N3906 or BC547-class BJTs, 1N4148
  diodes, TL072-class op-amps (macro-models), CA3080/LM13700 OTA models.
- A Python driver (`tools/circuit.py`, to be written) writes netlists from a parametrised topology,
  runs ngspice in batch mode, and reads the waveforms. It runs several simulations in parallel, limited
  to a few at a time (this machine's load matters).
- Outputs per run:
  - the waveform;
  - the start-up time to 90 % amplitude;
  - the steady frequency;
  - the spectrum (harmonic levels, phases);
  - pitch against control voltage over the range;
  - drift with temperature (ngspice `.temp`);
  - sensitivity to each component (±tolerance).

### 6.3 Questions each oscillator must answer

1. **Does it start reliably?** From zero state with the supply ramping up, with ordinary noise as the
   seed. How long does it take (a requirement: under one period plus 5 ms at the lowest note)?
2. **Does the pitch track?** Is it exponential in the control voltage within 2 c over the lead's range
   after a trim (an expo converter's bulk resistance and the reset time bend the top)? Does a
   compensation circuit fix it (a series resistor; a high-frequency tracking trim)?
3. **Is the waveform right for the role?** The required spectrum per lead (section 4): harmonic
   slopes, odd/even balance and noise floor, stated as numbers once the references are measured.
4. **How does it drift and spread?** Pitch per °C, and per-voice spread from tolerances. These set
   the "imperfection" model and the calibration's job.
5. **How strongly can it be coupled?** For Fifths and Bass + Lead: the capture range of the 3:2 and
   2:1 locks against the coupling component's value; whether the lock survives glides and the full
   range; and its side effects on the waveform.
6. **How does it sound through the rest of the patch?** Filter self-oscillation, clipping, envelope
   clicks (a requirement: no click above -60 dB from gates).

### 6.4 Search

- Each lead has a parametrised topology: component values within E-series ranges, plus a few
  structural options (for example the reset by a transistor or a JFET; ladder poles 2 or 4).
- An objective per lead combines the measured spectrum against the required one (section 4),
  tracking error, start-up time, robustness to ±tolerance, and cost in the real-time solver (fewer
  nonlinear nodes is cheaper).
- Optimisation runs over the SPICE reference: Nelder–Mead or CMA-ES over continuous values, a sweep
  over structural options. The best few go on to the real-time model.
- Listening checks happen at each stage; numbers decide only what the ear cannot.

### 6.5 Real-time model

- The netlist is turned into a state-space model by the DK method or a wave digital filter.
- Nonlinearities (diode and BJT exponentials, op-amp saturation) are solved by Newton's method
  per sample, with a fixed small iteration budget and a good initial guess from the last sample.
  Hot spots go into lookup tables precomputed per patch.
- Discontinuities (resets, comparators, clipping) are band-limited by oversampling with polyBLEP or
  BLAMP corrections at the event times, or by antiderivative antialiasing for static waveshapers.
- Agreement with the reference is required:
  - frequency within 1 c;
  - harmonics 1–20 within 1 dB;
  - start-up time within 10 %;
  - over the lead's range and at three temperatures.
  
  This is the analogue of the waveguide-against-TMM test (`tools/test.py`, passive_vs_tmm).

## 7. Integration requirements

- A new base class beside `Tube` (for example `Circuit`), with the same `process()` and `seed()`
  contract and the same NaN recovery (`-nantest`) and fuzz (`-fuzz`) behaviour.
- A `SynthPlayer : Performer`:
  - `noteOn`/`noteOff` and the articulation rules (slur = legato, no retrigger; a softer overlapped
    key = glide; staccato = a short gate);
  - velocity to envelope depth and filter;
  - the ear's role is taken by the per-note calibration.
- Instruments are added to the instrument table with `Kind::kCircuit` and a family name ("lead"),
  so `-inst lead-saw`, `-ensemble` and live work unchanged.
- **MPK knobs:** a per-family knob map (the winds keep theirs). The leads use cutoff, resonance,
  envelope amount, attack, release, glide time, detune or lock strength, and volume.
- **Real time:** each voice runs under 1.5 µs per sample at 48 kHz including oversampling on this
  machine's core (the winds use 0.3–0.8 µs). Eight voices fit in a 64-frame period without xruns.
- **Tests** in `tools/test.py`:
  - start-up and pitch per lead after calibration (within 2 c);
  - the Fifths lock (3:2 within 0.5 c over the range);
  - aliasing (components folded below the fundamental at -60 dB or lower at C7);
  - the real-time margin;
  - fuzz;
  - NaN recovery;
  - real-time model against the SPICE reference on stored waveforms.

## 8. Deliverables, in order

1. `research/synth_leads.md` (this file), extended by the research: per lead the chosen topology,
   sources, measured reference behaviour and the open questions.
2. `tools/circuit.py`: netlist generation, the ngspice driver, measurements, search.
3. Reference results per lead (netlists and measurement tables) under `tools/circuits/`.
4. The real-time solver and the first lead (Sawtooth: the simplest core with a reset and a ladder),
   with tests, before the other seven.
5. The remaining leads; Fifths and Bass + Lead with locking; just tuning between voices.

## 9. Open questions

- Which antialiasing (oversampling with polyBLEP, or AA-IIR) gives the best cost for reset-based
  cores at C7 and above?
- Can 3:2 locking be made robust without audible waveform modulation? If not, the ratio comes from
  the control voltage with a weak coupling only to hold it.
- How much component spread and temperature drift to keep: enough to sound analog, not so much that
  just chords beat?
- Calliope and Chiff are named after pipes. Should they share the jet model instead, with an analog
  front end only for the chiff burst? A listening comparison should decide.

## 10. Findings so far (2026-11-13)

**The sawtooth core in ngspice** (`tools/circuit.py saw`): a 2N3904 expo transistor, an op-amp
integrator on 1 nF, a Schmitt-trigger comparator and a MOSFET reset.

- It starts from rest within one ramp.
- Its harmonics at A4 match an ideal sawtooth (−20 log n) within 0.1 dB.
- After a fix to the op-amp macro-model (its internal node must saturate, not wind up), the pitch
  tracks the control voltage within 2.4 c over A1–A7.
- The reset takes 0.36 µs (the op-amp's slew); that dead time is the −2.4 c at A7.
- The constant +130 c offset comes from the divider's real thresholds (0.335 to 4.965 V).

**Parasitics, each alone** (`tools/circuit.py parasitics`):

| Parasitic | A1 | A4 | A7 | Ramp shape |
|---|---|---|---|---|
| TL072-like op-amp (3 MHz) | +0.6 c | +0.4 c | −0.1 c | straight |
| 20 pF stray at the summing node | +4.8 c | +4.6 c | +4.0 c | straight |
| 2 nA leakage | +12.7 c | +1.6 c | +0.2 c | straight |
| 50 nH in the reset loop | +4.7 c | +4.5 c | +4.6 c | straight |
| 2 Ω emitter resistance | +0.1 c | −0.3 c | −2.4 c | straight |
| Ceramic C, −10 % at 5 V | +55.5 c | +55.3 c | (did not oscillate; probably a solver problem) | bends 0.84 %; harmonics change 0.03 dB |

In a sawtooth core the parasitics move the pitch, not the timbre: the circuit's job is a straight
ramp, and it does that job.
- The ones that change with pitch are kept in the real-time model, per voice: leakage (the
  bottom sharp, doubling every 10 °C) and emitter resistance (the top flat).
- The constant shifts are left to the autotune.
- An analog lead's character must come from elsewhere: voice-to-voice drift and tracking,
  the ladder filter's and the amplifier's nonlinearities, and their parasitics.

**The real-time core** (`src/lead.cc`, 48 kHz):
- It is exact between events, with a sub-sample reset, the dead time and a polyBLEP.
- Before the autotune it matches the SPICE netlist's frequency within 0.1 c at A4 (+130.2 c both)
  and its droop within 0.1 c.
- It costs about 10 ns per sample with the ladder.
- After its power-on autotune (per octave, interpolated) every note C2–C7 is within 2 c.

**Next:**
1. The transistor ladder in ngspice: each pair's mismatch (offsets: even harmonics), base
   currents, the junction capacitances at high cutoff, input overdrive. Measured the same way,
   against the Huovilainen model now in `src/lead.cc`.
2. The OTA amplifier.
3. Then the other leads.

**The shared board** (2026-11-13; `src/lead.cc`; `PJ_ANALOG` scales it, 0 = ideal). The voices
share stray wiring capacitance, a supply rail, a ground and the board's heat. Each voice also has
its own 1/f noise, ladder offsets, amplifier feedthrough and bleed, and mains hum. Measured with
`flute -leadpair DETUNE OTHERS`, two voices on A4, interleaved 4 samples at a time as in the
ensemble:

- **Parallel wires do not lock the voices.** At realistic stray capacitance (0.5 pF to a
  neighbour) a reset moves another voice's ramp by about 2.3 mV, and the ramps' slopes feed a
  current that cancels it over a period. A linear ramp advances by the same amount wherever the
  kick lands. Two voices 0.05–0.3 c apart stay that far apart, even at 3× the coupling. Locking
  in real instruments needs a stronger path than stray wiring (a shared reference, sync); that
  is a question for Fifths and Bass + Lead (section 5).
- **Rail sag:** each extra sounding voice raises the others by about 0.1 c (+0.7 c with 8 voices).
- **After a release:** the always-running oscillator leaks through the closed amplifier, with
  50 Hz hum, about 67 dB under the note (95 dB at `PJ_ANALOG=0`). Control feedthrough gives a
  small thump on each attack.

**Healing of the just tuning** (`src/ensemble.cc`). A chord is solved when the sounding set changes.
While the set stays the same, every 100 ms each note's anchor is decayed towards 0 c (time constant
3 s, `PJ_JIHEAL`) and the chord is solved again. The intervals keep their ratios, and the common
offset heals:
- An E4 left alone after a C–E–G chord went from −9 c to 0 c in about 8 s; before this change it
  stayed at −9 c.
- A held C–E third stayed at −13.5 c (−13.7 c just), its mean near 0.

**The filter's wander** (`PJ_FILTERWANDER` scales it, 0 = none). Each voice's cutoff and resonance
move on their own. The motion is not a random walk: it is kicks of Laplace-distributed size at
random moments (about 8 a second for the cutoff, 5 for the resonance). A PID loop pulls each kick
back, critically damped over about 0.5 s; its derivative term resists fast change. Its integral
term means no offset lasts, so the setting is right on average. The spread is about 0.15 semitone
in the cutoff and 0.02 in the resonance, and the tails are heavier than Gaussian (kurtosis about 5).
In a held A3, the spectral centroid moves 0.9 % from one 0.1 s frame to the next (0.05 % without
the wander), and its mean is unchanged (561 Hz).

**Each voice's own vibrato and place** (2026-11-13).
- **Vibrato.** Every voice has its own free-running vibrato oscillator (LFO): a quadrature
  oscillator, two integrators in a loop. That is a rotating complex number, turned by an angle each
  block, with an amplitude loop that holds it on the unit circle. Each attacked note gets its own
  rotation speed, uniform within ±15 % of the setting (`PJ_VIBSPREAD`), and its own depth, within
  35 %; a slurred note keeps the one before it. Each voice's depth also has its own trim (within
  25 %).
- **Onset.** A note starts plain. After its own delay (about 0.35 s, ±30 % per note), the vibrato
  comes in over about 0.6 s, along a random path:
  - The depth rises on a smooth curve whose steepness differs per note.
  - Most notes (about 90 %) swing at their own rate from the start.
  - About 10 % start slower (55–80 % of the rate) and speed up, a little later than the depth.
  Measured, in an earlier version that always started slower: under 2 c in the first 0.3 s, and full
  depth by 0.8 s.
- **Re-attack on a pooled voice.** A pooled voice that takes a new note starts at that note's just
  tuning, from 0 c. It used to keep its old tuning when it got the same note again: an E4 started at
  −9.9 c and healed slowly; now it starts at −0.4 c.
- **Breathing.** On top, the speed and the depth wander like breathing, not a machine. Each is a
  noise through two low-passes (0.6 s each, so nothing faster than about 0.3 Hz): the speed by about
  10 %, the depth by about 20 % (`PJ_VIBNOISE` scales both). Measured on a held note at 5 Hz:
  - the cycle rates ran 4.1–6.4 Hz, but changed only 2.5 % from one cycle to the next;
  - the depth ran 17–37 c around 30 c.
- **Pan.** Without a room (`-room dry`), the synth voices each get a stereo place: spread by the
  golden ratio within ±70 % (`PJ_PANWIDTH`), equal power, the centre at the mono level. In a room
  they already sit at their own seats.

**Filter knobs, Q and loudness** (2026-11-13).
- **Knobs.** The lead's filter has its own knobs: K1 the formant spread (each key its own fixed
  cutoff offset: a random table over the 128 keys, centred, within ±12 semitones at full turn; slid
  over 15 ms on a slur), K2 the cutoff (−24 to +60 semitones from the
  note, keyboard-tracked by half), K3 the resonance (0 to 1.05). The dynamics knob no longer moves
  the cutoff.
- **Q.** A 4-pole ladder has no separate Q: its resonance is the Q. With feedback 4r around four
  one-pole stages, the linear poles are at −1 + (4r)^¼ e^(±iπ/4), which gives:

  | r | 0.3 | 0.5 | 0.7 | 0.8 | 0.9 | 0.95 | 0.98 | 1.0 |
  |---|---|---|---|---|---|---|---|---|
  | Q | 1.5 | 2.7 | 5.4 | 8.7 | 18.7 | 38.7 | 99 | ∞ (oscillates) |

  `Lead::ladderQ`; live prints it as K3 turns.
- **Loudness under the cutoff.** The ladder's gain under the cutoff is 1/(1 + 4r): −14 dB at
  r = 1. The output gives back (1 + 2r). A held A2 at full resonance went from 0.012 to 0.033 rms
  (0.042 with none).
- **The ensemble's mix.** The mix keeps the sum near one voice's level (1/√n). It used to rise
  within 40 ms when a key went up, while that note still rang: an "umpf" on every key up, for
  every instrument (a two-note chord rose 4 dB when one key went up). It now drops in 40 ms when
  voices join and comes back up over about 2 s when they leave.

**All eight leads, the synth families and two drums** (2026-11-13; `src/lead.cc`, `LeadPatch`).
Every one keeps lead-saw's core (the expo converter and the ramp: the vibrator) and its ladder (the
resonator), with the same board, wander, vibrato and player. Between them each has its own sound
circuit; lead-saw's path is unchanged (its render is bit-identical).

| Instrument | GM | Sound circuit |
|---|---|---|
| lead-square | 81 | a comparator on the ramp, its threshold swept by its own 0.6 Hz triangle (width 35–65 %) |
| lead-calliope | 83 | the ramp full-wave rectified (a triangle) into a differential pair (a near-sine); junction noise band-passed into a delay-line comb at the note's period (pitched breath); a slow pitch wobble |
| lead-chiff | 84 | the sine with a little pulse; at each attack a resonator ~4× the note (Q 6) pinged by a 25 ms noise burst |
| lead-charang | 85 | two cores 7 c apart (split around the note), an asymmetric diode overdrive, a 650 Hz mid scoop |
| lead-voice | 86 | a narrow pulse and the ramp into four formant band-passes, "oo" (soft key) to "ah" (hard); breath; a two-tap bucket-brigade chorus after the amplifier |
| lead-fifths | 87 | a second core pre-set 3 c under 3:2, held there by soft sync (at the first core's reset it fires if within 4 % of its top); a sine wavefolder |
| lead-bass+lead | 88 | flip-flops on the resets (÷2, ÷4: exact octaves); the ramp ring-modulated by the ÷2 square |
| synth-bass-1/2 | 39/40 | the ramp and its ÷2, a fast resonant filter pluck / a narrow pulse ring-modulated by its octave, the sine, a little overdrive |
| synth-strings-1/2 | 51/52 | two cores 12 / 20 c apart (and a swept pulse), a slow bow, long release, the chorus |
| synth-voice | 55 | the formant bank, breath, slow onset, the chorus |
| synth-brass-1/2 | 63/64 | two cores 6 c apart / ramp and wide pulse; the filter opening slower than the amplifier (60 / 110 ms), overshooting |
| melodic-tom | 118 | the sine, its pitch falling 4 semitones (120 ms) from the strike, the stick's noise, decaying while held |
| synth-drum | 119 | the sine swept down an octave in ~45 ms through a resonant filter, a noise burst |

Measured (`tools/test.py leads`): every note within 3 c of its target modulo the octave, wider only
where two cores are detuned on purpose (charang 7 c, strings 6–8 c) or the period is the ÷4's
(bass+lead 11 c). lead-fifths: every note's pair repeats at exactly twice the note's period (the
fifth locked at 3:2 within 1 c, 54/54 notes). 280–400 ns per sample each.
