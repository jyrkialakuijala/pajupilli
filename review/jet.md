# Review: jet, excitation and timbre (pajupilli @ 701a84a)

Reviewer focus: jet aeroacoustics, excitation, timbre, dynamics, breath noise, attacks.
All experiments ran on a copy, not on the repo:
`/tmp/claude-1000/-home-jyrki-pajupilli/b0250b6e-5038-4aaa-b429-7d4c393762a5/scratchpad/review_jet/`.
That copy adds a driver `xp/exp_main.cc` (play / passive impulse / pressure ramp with chosen cover and pressure trims) and
three experimental knobs in `src/flute.cc` / `src/player.cc`: `PJ_TURBPRE`, `PJ_AGC`/`PJ_AGCT` and `PJ_HEXP`.
The Python analysis scripts are in `.../scratchpad/py/`: `t2.py` (timbre pp/ff), `ramp.py` (register map), `grid.py`,
`wscan.py`, `imp.py`, `noise.py`, `onset.py` and `hf.py`.
**[S]** marks speculation or a literature value quoted from memory that I could not re-check here.

## Summary

The jet model is a reasonable *Fletcher-style* displacement drive. Its numbers can be defended: cp = 0.175 Uj with the
1.8 mm dead length removed gives exactly a half-period jet delay (ωτ = π) for every note, which is Fletcher's classical
operating point. It is not yet a credible *register selector*, though. With physical (Coltman) lip coverage, the model
has the following faults:

1. It has spurious stable regimes on the odd modes 3, 5 and 7. G4 is bistable with mode 5 across its normal blowing
   pressure.
2. At low pressure it sounds the 2nd or 3rd mode *before* the fundamental, which is the reverse of a flute. The player
   reviewer saw the same thing: lowering the pressure makes C4 jump up to C5.
3. It reaches its tuning only because calibration moves the embouchure to non-physical settings: C4 is left 9.7 of
   10.3 mm uncovered at 1.7× pressure, where Coltman measured 6 mm.

The timbre faults come mostly from one mechanism. The jet deflection is 4× past its saturation, so the flow split F is
a 50 %-duty square wave. That makes the low register **odd-harmonic dominated, like a clarinet**: H2 is −26 to −42 dB
and H3/H5 are about 0 dB, even at pp. Breath noise has a plumbing bug. The noise bypasses the jet low-pass and is
then differentiated twice, so the hiss rises at +6 dB/oct up to 24 kHz.

The E6–C7 sharpness has two causes:
- **Passive geometry:** the 3rd-octave resonances are +16 to +29 c sharp at Coltman coverage.
- **The jet:** those regimes only win at 1.45–2× pressure, and the pitch rises about 65 c per doubling of pressure.

The calibration then runs into its coverage floor, or the note loses its register as the window closes.

## Measurements (calibrated unless stated otherwise; dyn 0.1 = pp, 0.9 = ff)

**Harmonic levels H2..H5, dB re H1, radiated at 1 m:**

| note | pp | ff | real flute (Fletcher 1975; Fletcher & Rossing §16.11) [S on exact numbers] |
|---|---|---|---|
| C4 | −36 −3 −29 −3 | −20 +1 −16 −4 | pp: H1 dominant, H2 ≈ −5…−10, H3 below that. ff: H2 and H3 ≈ H1 or above, smooth roll-off, **no odd/even pattern** |
| G4 | −27 −6 −14 −5 | −38 −4 −27 +4 | same as C4 |
| C5 | −12 −8 −15 −20 | −17 −2 −5 −15 (16 Hz AM, sidebands at ±16 Hz) | H2 ≈ −5…−10 |
| G5 | +5 +17 −6 −2 (+80 c, wrong regime) | +1 −5 −26 −32 | H1 dominant, H2 ≈ −10…−15 |
| C6 | +10 −12 (+112 c) | 0 −21 −41 | H1 dominant |
| E6 | sounds a 5th low (−655 c) | **+10** −10 −38 | nearly sinusoidal, H2 ≈ −15…−25 |

- **Odd/even balance:** the jet split F itself has H2 −25 dB and H3 −10 dB at C4. F sits at −0.99 for 48 % of the cycle
  and at +0.64 for 42 %.
  - The linear deflection before the clip is 6.2 mm at C4 (b = 0.92 mm, y0 = 0.8 mm). For E4–C5 it is 4.7–5.3 mm.
  - With the edge crossings this steep, y0 hardly moves the duty cycle: (acos(y0'/A)/π ≈ 0.45).
  - Setting `PJ_ETAMAX` to 0, 1.5 or 3, or `PJ_Y0` to 1.6, leaves H2 at −18 to −38 dB at C4/G4 (timbre.py sweep).
- **Listener geometry:** with the all-closed fingering, the embouchure and foot flows of the even modes are in antiphase.
  The listener stands almost equidistant (path difference 3.9 cm), so C4's H2 loses another 8 dB.
  - Embouchure only (`PJ_RADONLY=-1`) gives H2 −11 dB; all sources together give −19 dB.
  - The cancellation is real physics in an anechoic field, but rooms and recordings do not show it this deeply.
- **Resonance alignment:** passive peaks of the C4 fingering at Coltman coverage are −41, −5, +8, +11 c relative to
  n·f(C4). G4: −17, +19, +18 c. C5: +26, +51, +29 c. That is a **~36 c octave stretch** between modes 1 and 2.
  - The harmonics of the played note sit on the flanks of the resonances, which weakens H2.
  - Mode-1 Q is 22–28; modes 2–6 have Q 40–58.

**Dynamics** (pp→ff, calibrated):
- RMS rises by only 2.3 dB at C4, 6 dB at E4 and 7.5 dB at G4. A real flute changes by roughly 10–20 dB in the low
  register, mostly through the upper harmonics [S].
- Steady G5 and C6 are −43 and −39 dB, against −25 dB for G4 and E5. The second octave is weak, as the player reviewer
  also found.
- Pitch rises with blowing in the right direction: C4 −14 → +8 c, C5 −12 → +14 c.
- At fixed coverage the slope is about 65 c per doubling of pressure (C6, E6). That is plausible but large [S].
- The pitch is very sensitive to jet length: scaling W from ×0.6 to ×1.1 moves C4 from −7 to −82 c and C5 from +89 to
  −18 c. Low Q plus jet phase pulls the frequency off the resonance (Δf/f ≈ Δφ/2Q).

**Register map.** Pressure ramp from 117 Pa to 4× and back over 3 s each way, calibrated cover. Changes of
f0/f(note) are listed:
- C4: up the ramp, 3.4/2.8/3.3/**2.0** at 82–151 Pa, mode 1 from 164 Pa, **5.1 at 548 Pa**, back to 1 at 694 Pa. Down the
  ramp, 5.1/10/2.0 at 660–744 Pa, 1 to 117 Pa, then 7/2/3/5.
- G4 (normal pressure is about 314 Pa): up the ramp, 1 at 251 Pa, **5 at 272–476 Pa**, 1 from 476 Pa, 5 at 1135 Pa. Down
  the ramp, 5 at 372–492 Pa, 1 at 259 Pa, then 2/5/3.
- C5 fingering: 1 → 2 at about 1500 Pa up; 2 → 1 at about 1100 Pa down (hysteresis ✓).
- `PJ_REL=1` (Fletcher's full relative form), `PJ_JETLP=1` and `PJ_AGC` all leave the mode-5 and low-pressure
  higher-mode windows in place. `JETLP=1` makes them worse.

**3rd octave.** Grids of coverage 2–5 mm × pressure 0.85–2×, nominal W:
- E6 sounds its 2nd mode below 1.45× pressure. At 1.45× the pitch runs from −13 c (2 mm) to +46 c (5 mm).
- G6 needs ≥ 1.2× pressure at 4 mm (+31 c). With a smaller window it falls to mode 0.77 (1205 Hz). The player
  reviewer's "G6 = 1205 Hz at every dynamic" is this.
- C7 sounds only at ≥ 1.7× pressure and is +35 to +71 c sharp.
- With W ×0.6–0.9, all of them sound at nominal pressure, but +25 to +76 c sharp.
- Passive peaks at nominal coverage: E6 +26 c, G6 +29 c, A6 +16 c.

**Breath noise** (1–8 kHz between the harmonics, dB re total):
- The model gives −30 to −36 dB, and nearly the same at pp and ff.
- Above 12 kHz the attack is −8 to −18 dB re the steady tone and the steady state is −12 to −25 dB. That is a strong
  near-Nyquist hiss.
  - With `PJ_NOISE=0 PJ_GLOTNOISE=0` it falls to about −50 dB.
  - It comes from the embouchure radiator (`PJ_RADONLY=-1`).
  - At C4 the 8–64 ms of the attack are dominated by 18–23 kHz energy.

**Attacks** ("tu", vel 70):
- 10 %→90 % rise is 10→110 ms at C4, 25→117 ms at G4 and 35→52 ms at C5. Real tongued attacks take about 10–30 ms
  (Castellengo 2004; research §3).
- G5 and C6 attacks have a burst 15–18 dB louder than the steady state, lasting about 30 ms, on a high regime (2.34 kHz
  for G5). That is a squeal, not a chiff.

## Findings, ranked by impact

### 1. The jet transfer is frequency-flat, so odd higher modes win (registers, thresholds, fragility). Impact: very high
- **Evidence:** the register map above, and the W-scan. The higher-mode window at low pressure is the player's "lower
  breath → octave up". The mode-5 window inside G4's playing range explains the spontaneous "jumps to 4th–9th modes".
- **Why:** in the displacement form, ξ = v/jω cancels the jω of the dF/dt source. The loop gain is then about
  cosh(μW)·LP(ω)·Y(ω), and the phase condition ωτ = (2n+1)π is met *equally* by mode 1 and by modes 3, 5 and 7 when
  τf₁ = ½. Modes 2–6 have about twice the Q of mode 1. The two-pole "Rayleigh" low-pass costs only about 8 dB at
  mode 5, too little to stop it, and the eta clip flattens the describing-function gains on top of that.
- **What a real jet does** (Fletcher & Thwaites 1979; de la Cuadra 2005/2007; Rayleigh/Bickley stability theory):
  1. **Dispersion.** cp rises with kb. For the sinuous mode of a top-hat jet, the temporal theory gives
     cp/U ≈ tanh(kb)/(1+tanh kb): 0.17 at kb = 0.2, 0.3 at kb = 0.46, toward 0.5 at kb ≥ 1. The Bickley profile is
     lower [S]. High modes therefore see a *shorter* relative delay and leave the ωτ = (2n+1)π lock.
  2. **Growth limit.** The growth rate μ(St) peaks near kb ≈ 0.4–0.5. A jet several wavelengths long breaks up rather
     than amplifying coherently.
  - The present cp = 0.175 Uj is a good *low-St* value: it equals Fletcher's effective jet speed (0.4–0.6 of Bernoulli)
    × 0.4. As a constant, though, it gives every harmonic the same lock.
- **Fix (M):** replace the delay + two-pole low-pass with a frequency-dependent jet transfer, for example:
  1. Split the delay: a constant part τ₀ = W/(0.4 Uj), plus a dispersive all-pass/low-pass section. The section's group
     delay at low St makes the total effective cp ≈ 0.175 Uj (keeping today's tuning point), and it falls toward
     W/(0.45 Uj) for kb ≳ 0.5.
  2. Multiply by a gain that decays with ωτ beyond about 1.5π, for example exp(−(f/f_c)²) with f_c ≈ 1.2–1.5/τ
     (a Gaussian with a matched delay; 4th-order Bessel is fine).
  3. Fit the curves to de la Cuadra 2007's measured convection/amplification against St. That paper is on arXiv
     0811.0333 and is already reference [4] in research/jet_and_losses.md.
  - Acceptance test: `py/ramp.py` must show, for every low-register fingering, one mode-1 window from threshold up to
    about 2–3× nominal pressure, then 2 (or 3 on the 12th fingerings), with hysteresis. There must be no mode ≥ 3
    below the 1→2 threshold.
- **Risk:** medium. This retunes everything, and the calibration must be re-run.

### 2. The jet deflection is far past saturation, so F is a square wave (odd/even balance, pp too rich, small dynamic range). Impact: high
- **Evidence:** linear η of 4.7–6.2 mm against ETAMAX 1.5 mm and b ≈ 0.9 mm; the F spectrum; the y0 sweeps; the
  clarinet-like H2..H5 at both pp and ff.
- **Why:** Fletcher & Douglas 1980 (JASA 68, 767) show that harmonic generation in flutes comes from a jet of
  amplitude ~b swinging across an edge that sits *offset* by y0. In that regime H2 depends strongly on y0/b, and
  overblowing toward ff enriches the spectrum gradually. A clipped η removes both effects: the duty cycle stays at
  0.45 whatever y0 is.
- **Test:** I replaced the instantaneous clip with a slow amplitude control (`PJ_AGC=1.5 PJ_ETAMAX=0`, 4 ms envelope).
  At C4/G4, H3 drops from about 0 to −10/−16 dB, H5 to −20/−30 dB, and H2 rises to −14 dB at F.
  - It is not yet right: the radiated H2 is still −17 to −24 dB.
  - The remaining H2 deficit comes from the 36 c mode stretch (finding 5) and the listener cancellation (finding 6).
- **Fix (M):**
  1. Make the jet saturate through *gain* (vortex roll-up limits growth over a few cycles) and stop clipping η
     instantaneously. Physically, the amplification factor cosh(μW) falls once |ξ| ≳ b. An envelope- or
     energy-based μ(A) is an honest phenomenological form [S].
  2. Re-tune y0 so that y0 ≈ 0.3–0.6 × the swing.
  3. Let the swing grow with blowing, so that pp sits near threshold (nearly sinusoidal) and ff is rich.
- **Risk:** low to medium. The register map interacts with finding 1.

### 3. Breath noise bypasses the jet filter and is differentiated twice. Impact: high (audible), effort S
- **Evidence:** `turb/(2π·1000)` is added *after* `jl1_/jl2_` in `flute.cc`. The chain then has cosh gain, tanh,
  dF/dt (+6 dB/oct) and radiation d(flow)/dt (+6 dB/oct), so a one-pole 6 kHz noise rises to Nyquist.
  - The >12 kHz energy is −8…−25 dB re the tone (see above); 18–23 kHz dominates the first 60 ms of C4.
  - With `PJ_TURBPRE=1` (noise added *before* the jet low-pass), the >12 kHz energy falls 15–28 dB to −35…−46 dB.
    The 1–8 kHz hiss then needs `PJ_NOISE` ×3–5 to stay audible (−20…−40 dB band level).
- **Fix:**
  1. Inject the noise before the jet filter.
  2. Add a *direct* edge-turbulence radiator: a dipole at the embouchure, pressure ∝ ρUj³·n(t), band-passed around
     St = f·h/Uj ≈ 0.1–0.2 (2–6 kHz), with no bore shaping. That gives the "airy" component heard outside the tube.
  3. Make the relative noise level rise at pp/low notes. Edge noise scales about as Uj³ while the tone does not [S].
- Also check the glottal "hiss" path: it adds Uj jitter and HF in the attack.

### 4. Tuning comes from non-physical embouchure settings; the 3rd octave is sharp. Impact: high
- **Evidence:**
  - Without calibration, the low notes are 40–62 c flat and C5–D6 roughly in tune. Calibration fixes this by
    uncovering 8–9.9 mm (Coltman measured 6 mm) and blowing at 1.2–1.7×.
  - At Coltman coverage the passive mode 1 of C4 is −41 c. The jet pull then adds W-dependent offsets of up to tens
    of cents.
  - 3rd octave:
    - The passive peaks are sharp: E6 +26 c, G6 +29 c, A6 +16 c.
    - The 3rd-octave regime only wins at 1.45–2× pressure, and +65 c per doubling of pressure makes it sharper.
    - Closing the window (which would flatten the note) hands the jet to the lower mode.
    - Calibration ends at the 2 mm floor (90, 92, 93, 95) or where the register is lost (91, 96).
  - Scaling lip height with pitch (`PJ_HEXP` 0.3/0.5) did not help.
- **Fix:**
  1. Let calibration (and the player model) also choose W, the lip-to-edge distance. Real players shorten it in the
     3rd octave; it is Fletcher's l(f) with ±20 % scatter. Keep coverage within Coltman ±1.5 mm.
  2. With finding 1, the 3rd-octave regimes should be reachable at about 1–1.3× pressure.
  3. Hand the passive part to the bore reviewer:
     - The cork cavity is only ~2.4 cells (dx 7.3 mm).
     - The embouchure inertance is frequency-independent: no cavity or lip "flange" frequency dependence.
     - The hole lattice of the cross-fingerings needs checking.
     Wolfe et al. (J. Sound Vib. 2001, flute impedance spectra) is the reference for target resonance positions [S].
- **Risk:** medium.

### 5. 36 c octave stretch of the low-register resonances. Impact: medium (timbre, slurs)
- **Evidence:** `imp.py` at Coltman coverage (above).
- **Why:** the harmonics fall off the resonances, so H2/H3 are weak and lock poorly. It also explains:
  - C5's two competing oscillations: 16 Hz sidebands ±3 dB at mf/ff, heard as roughness.
  - The slow, unreliable octave slurs: C4→C5 drops out, and D4→D5 never settles (player reviewer).
- **Fix (bore and embouchure):** resolve the cork cavity and embouchure better (sub-cell or lumped compliance), check the
  2 mm `PJ_EMB_LEN`, and target ≤ 10 c of mode-1/mode-2 inharmonicity. The real head joint and cork position are
  designed for this.
- **Risk:** low.

### 6. The listener sums coherently at an even-mode null. Impact: medium (H2 −8 dB at C4), effort S
- Even modes radiate from the embouchure and the foot in antiphase. At 1 m, "opposite the middle", they cancel.
- **Fix:** move the listener toward the embouchure end, in front of and above the player (a typical mic or audience
  position). Alternatively, give the foot and holes a small decorrelating room tap (a short early reflection) so
  the timbre does not hinge on one null.

### 7. Attacks: slow onsets and high-regime squeals. Impact: medium
- **Evidence:** t90 is 50–117 ms (target 10–30 ms). G5 and C6 attacks have a +15–18 dB burst of about 30 ms on a high
  mode. The player reviewer also found that the tone reaches −6 dB 40–80 ms after onset whatever the tongue does.
- **Why:** the ramp passes through the spurious higher-mode windows (finding 1) on its way to the right regime. Near
  threshold the square-wave limit cycle builds up slowly.
- **Fix:** mostly follows from findings 1–2. Then add the jet-noise burst at tongue release (research §3) as the
  chiff, rather than relying on wrong-regime transients.

### 8. Dynamic range and loudness balance. Impact: medium
- **Evidence:** pp→ff is +2…+8 dB at C4–G4. The steady G5/C6 are 14–18 dB quieter than G4/E5.
- **Why:** a saturated F (finding 2) makes the source amplitude independent of blowing. Only K ∝ Uj and the duty
  change.
- **Fix:** findings 2 and 1. Then check that a pp/ff difference of about 10–15 dB appears, mostly in H2–H5, with H1
  rising only a few dB (Fletcher 1975).

### 9. Smaller points
- **The undelayed relative term** (`PJ_REL=1`, Fletcher's η = cosh·ξ(t−τ) − ξ(t)) is the textbook form. At
  cosh ≈ 6 it is a ~16 % term and barely changed the register map in my test. Keep it as an option; the 0/1 choice is
  not what decides correctness.
- **Source term K:** Verge's ρδd b Uj/W is used consistently. δd(4/π·√(2hW)) was derived for recorder geometry, and a
  flute's edge and window differ, so treat it as a fit parameter [S].
- **Window loss:** 0.01 ρc/S_w (lip slit) plus vortex shedding is acceptable as a phenomenological term. Since the
  resistance is frequency-independent it lowers mode-1 Q (22–28) relative to modes 2–6 (40–58), which favours
  higher modes (finding 1). A √f or slit-viscous resistance would do so less.
- **η noise scaling:** `turb/(2π·1000)` converts velocity to displacement at a fixed 1 kHz. It should follow the
  jet filter (finding 3).
- **The calibration accepts quasi-periodic states** (C5) and wrong-register near misses. Add an inharmonicity or
  sideband check, and one for spurious-mode stability under a ±20 % pressure perturbation.

## Answers to the specific questions
- **Is it defensible?** Yes, as Fletcher-style physics with sensible numbers. As implemented, it lacks the
  frequency dependence (dispersion and growth limit) that makes real jets select registers. The most credible
  formulation that also selects registers well:
  - Fletcher/Verge flow-split source with an offset Bickley jet.
  - A jet transfer with dispersive cp(St) and band-limited growth, fitted to de la Cuadra 2007.
  - Saturation through growth, not through an η clip.
  - Validation against the register map and the thresholds/hysteresis of Auvray 2012/2016 (θ ≈ 0.066 up / 0.076
    down, recorder; research §2.3).
- **cp = 0.175 Uj** is plausible as a low-St effective value: Fletcher's slowed jet (0.4–0.6 of Bernoulli) × 0.4.
- **The 1.8 mm dead length:** dropping it from W makes θ = 0.087 and τf = ½ for every note, i.e. Fletcher's
  half-period condition. Physically it is the jet-forming region at the lips, so it should count in the jet's
  convective length with cp near 0 there [S]. Treat it as part of the tuning of W, not as free length.
- **Third-octave sharpness:** see finding 4. It is passive-sharp, the regime needs high pressure, and calibration
  hits its limits. W is the missing control.

## Prioritised plan

| # | Change | Benefit | Effort | Risk |
|---|---|---|---|---|
| 1 | Noise before the jet filter, ×3–5 gain; direct edge-hiss dipole; relative noise up at pp | Removes 12–24 kHz hiss and attack fizz; realistic airy tone | S | low |
| 2 | Move the listener toward the embouchure / add a decorrelated room tap | +8 dB H2 low register | S | low |
| 3 | Growth-limited jet (envelope/energy μ(A)) instead of the η clip; re-tune y0 ≈ 0.3–0.6 × swing | Flute-like even/odd balance, pp → ff enrichment, dynamic range | M | med |
| 4 | Dispersive cp(St) + band-limited jet gain; acceptance test `ramp.py` | Clean registers, normal thresholds and hysteresis, no squeals or inverted overblowing, faster attacks | M–L | med |
| 5 | Calibration over (coverage within Coltman ±1.5 mm, W within Fletcher ±20 %, pressure); reject quasi-periodic states | Physically plausible embouchure; 3rd-octave tuning | M | med |
| 6 | Bore/embouchure: cork cavity resolution, frequency-dependent embouchure end correction; target ≤ 10 c mode stretch | H2 alignment, slurs, C5 roughness | M | low |

## Open questions
- Published flute spectra by register × dynamic: I quoted Fletcher 1975 / Fletcher & Rossing qualitatively. Digitising
  Fletcher 1975 Figs 3–5 and the UNSW sound files would give hard targets for `tools/timbre.py`, whose targets are
  estimates.
- How fast does pitch rise with blowing on a real flute at fixed embouchure? The model gives about 65 c per pressure
  doubling in the 2nd and 3rd octave; the Coltman 1966 data should settle it.
- Is the remaining H2 deficit after fixes 2–3 resonance alignment or jet asymmetry? Re-measure F and radiated H2
  after the bore fix.
