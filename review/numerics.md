# Numerical acoustics of the waveguide: review

Scope: `src/bore.{h,cc}` and how `src/flute.cc` uses them, with `tools/tmm.py` as the reference.
Reviewer: numerics. Date: 2026-11-11.

All experiments ran on a copy of the repo (scratchpad `review_numerics/`, not committed). The copy had
extra env knobs in bore.cc/flute.cc: `PJ_LAMBDA` (Courant), `PJ_KLOSS` (loss scale), `PJ_IFACE`
(interface-area rule), `PJ_TERM` (1 = p=0 end, 2 = lumped Levine–Schwinger end instead of the horn),
`PJ_HORNR0` and `PJ_HCOMP` (hole-offset compensation factor). There were also small harnesses: a
cylinder impulse response `exp/cyl.cc`, the window admittance with Q `exp/imp.cc`+`exp/t3.py`,
a quantised-hole TMM `exp/t4.py`, and stability runs `exp/stab.cc`, `exp/nyq.cc`, `exp/decay.cc`.
Peaks and Q come from |Y| or |Z| with parabolic interpolation and the −3 dB width.
Throughout, "TMM" means `tools/tmm.py` run with `extra=2 mm` (the FDTD default `PJ_EMB_LEN=2`). The plain
`tmm.py` with no argument uses 0 mm and gives a different answer.

## Summary

* **The scheme is sound.** It is stable in theory and in long runs. Dispersion at Courant 0.99
  is negligible (<0.4 cent below 4 kHz). The loss fit is passive and accurate to ±5 % up to 4 kHz.
  Lumping the thermal loss into the series term costs ≤0.25 cent and ≤2.5 % in Q.
  With the same geometry, the FDTD matches the TMM within about 1 cent once the three spatial
  approximations below are removed.
* **All the passive tuning error comes from three spatial approximations.** None is time-stepping error:
  1. The spherical "horn" end correction is 0.94a instead of 0.61a, i.e. 3 mm too long. Its radiation loss
     above 1 kHz is about 2× too high.
  2. Rounding the foot to 85 cells makes it 2.8 mm too long.
  3. The hole-offset compensation `d·S_h/S` corrects only about 40 % of the quantisation error.
  Together these explain the C4 −14.5 cent offset, the C5 +11.7 cent offset, and the ±5 cent errors on
  G4/A4/B4. The cheap fixes below bring all first resonances within 3 cents of the TMM.
* **The octave stretch is not numerical.** The passive stretch of about +36 cents (mode 2 vs 2× mode 1)
  is the same in FDTD and TMM (±1 cent for C4–F4; FDTD up to 5 cents less for G4–C5).
  Decomposed in the TMM for C4: cylinder + cork ≈ +5 cents, closed holes ≈ +9 cents, head taper ≈ +14 cents,
  window inertance ≈ +5–8 cents, dispersion ≈ +5 cents.
  Whether a real PF-661 has this much stretch is not settled by the published data I could reach (see Open questions).
* **"Q 47–77" is not numerical either.** It is the passive model without the lip-slit loss (`PJ_EMBR=0`),
  and the FDTD and TMM Q agree within ±5 %. With the default `PJ_EMBR=0.01` the window admittance Q is
  23–44 for C4–C5 (mode 1 23–29). The bore wall loss is correct against Keefe. The Q is high because some
  physics is missing: tone-hole chimney and edge losses, wall roughness, closed-hole cavities. Numerics is not the cause.

## Findings, ranked by impact

### F1. The spherical horn termination: +3 mm end correction and too much radiation loss above 1 kHz (high impact, cheap fix)
**Evidence.** Test case: a 0.6 m × 19 mm cylinder, closed at the cork, driven by a flow impulse in
cell 0, and compared with an exact lossy TL + Levine–Schwinger (LS) end (`exp/t2.py`).

| mode | f FDTD (horn) | f LS theory | Δ | end corr FDTD / LS (mm) | Q FDTD / theory |
|---|---|---|---|---|---|
| 1 | 140.73 | 141.43 | −8.6 c | 17.6 / 14.6 | 34.5 / 34.5 |
| 4 | 993.7 | 999.0 | −9.2 c | 12.3 / 9.1 | 77.8 / 82.6 |
| 7 | 1848.6 | 1858.2 | −9.0 c | 11.3 / 8.1 | 77.4 / 97.0 |
| 10 | 2705.0 | 2718.3 | −8.5 c | 10.6 / 7.6 | 72.0 / 101.4 |

* Lossless: the end correction is 8.9 mm, i.e. 0.94a against LS 0.61a.
* With a p=0 end the same code matches the ideal within 0.4 cent and the Keefe Q within 5 % (`PJ_TERM=1`).
  So the error is the horn alone.
* Cause: the throat radius r0 = 5.8 mm is smaller than dx = 7.3 mm, so the near field (where the
  radiation mass lives) is under-resolved. In addition, the min-area interface rule shifts every horn
  interface area half a cell inward, which narrows the throat and adds mass.
* The mean-area rule in the horn (`PJ_IFACE=3`) is unstable and overflows in 180 samples. This matches the
  Gershgorin bound λ²(S₋+S₊)/(2Sᵢ) ≤ 1, which fails when r < 3.5 dx. The min rule is therefore necessary
  for the horn as built.
* The sector is 8.35 sr with r0 = 0.6133a. Its continuous radiation resistance is already 1.5× LS at low ka,
  and the discretisation adds to that.
* In the flute, the horn makes C4 −8 cents of the earlier −14.5 cent offset (−6.3 cents left after
  replacing it with a lumped end). It also sets the foot's high-frequency radiated level and the Q of
  the upper modes in low fingerings.

**Fix (pick one).**
* (a) **Lumped LS end** (`PJ_TERM=2` in my copy): close the bore after cell 84 and put a `Branch` there.
  The branch has series mass ρ(dx/2)/S, `rr = 1.505ρc/S` and `hl = S/(ρ·0.6133a)`. It is the same implicit
  trapezoidal node as the holes, so it is unconditionally stable.
  * Result for the cylinder: within 0.85 cent of LS for modes 1–10, Q within 3 % (Q 34.3/34.5 at mode 1, 98.9/101.4 at mode 10).
  * Result for C4 in the flute: −14.5 → −6.3 cents. The rest is F2.
  * The foot then radiates as d(q)/dt like every hole, so the "p(R)·R/d" special case goes.
  * Cost: the 43 horn cells become unused. The array can shrink to 96 cells (6 vectors), so about 25 % less bore work.
  * Effort S–M, risk low. Re-check the foot's share of the radiated timbre (commit 6a3d8d6) by ear.
* (b) **Keep the horn, retune r0** (`PJ_HORNR0=0.35`): the cylinder comes within −0.6 to −2.3 cents and Q within ±5 %.
  This is a one-line change and also corrects the excess radiation loss. It is empirical, though: it is only
  valid at this dx and bore radius. Effort S.
* (c) True interface areas in the horn (`PJ_IFACE=4`) give −3 cents. They are stable in runs but not provably so. Not recommended.

### F2. The foot length is rounded by +2.8 mm (and the embouchure by +0.76 mm) (medium impact, trivial fix)
**Evidence.**
* `footX/dx` = 84.62 → 85 cells, so the foot is 2.8 mm too long.
* TMM with the foot moved by the same 2.8 mm reproduces the remaining C4 offset: with the lumped end,
  FDTD vs TMM changes from −6.3 to +1.3 cents.
* `ec_ = int(0.0175/dx) = 2` puts the embouchure centre 0.76 mm downstream of its true place (18.26 vs 17.5 mm).
  The cork cavity is 0.76 mm long and the air column 0.76 mm short. This is the residual +1.3 cents.

**Fix.**
* Choose λ so the foot falls on an interface: λ = c·dt·85/0.6179 = 0.99446 (`dx = 7.269 mm`).
  It is still <1, so stable, and dispersion stays negligible.
* Compensate the embouchure offset with a negative `vol` on cell 0 (−0.76 mm·S) plus 0.76 mm·S/S_w of
  extra window mass. Alternatively, let the calibration absorb it.

With the lumped end and λ = 0.99446, the first resonances of C4/D4 come within +0.6 cents of TMM. Effort S.
Risk: all hole offsets change, which F3 handles.

### F3. The hole-position quantisation compensation `d·S_h/S` is too weak (medium impact)
**Evidence.** Hole offsets from the cell centre reach ±3.6 mm: hole 4 (C5 thumb) +3.27 mm, hole 10 (G4) −3.57 mm,
hole 15 (D4) −2.2 mm. In TMM with the holes moved to cell centres (`exp/t4.py`), mode-1 shift in cents:

| compensation | D4 | E4 | G4 | A4 | B4 | C5 | C#5 |
|---|---|---|---|---|---|---|---|
| none | −4.9 | −5.4 | −11.0 | +6.3 | +4.4 | +17.8 | +7.7 |
| ×1 (current) | −1.8 | −1.9 | −5.2 | +4.4 | +3.5 | +11.3 | +5.2 |
| ×2 | +1.6 | +1.8 | +1.3 | +2.5 | +2.6 | +5.4 | +3.0 |
| split over 2 cells (admittance weights) | +4.4 | +5.0 | +6.1 | +4.4 | +3.6 | +7.1 | +3.8 |

* The ×1 row reproduces the measured FDTD−TMM differences (G4 −4.9, A4 +4.6, C5 +11.7). Quantisation is
  therefore the whole story for the hole notes.
* `d·S_h/S` is the leading-order result for an isolated hole. With a lattice of open holes below, the
  sensitivity of the end correction to the hole's mass is smaller, so a larger chimney change is needed.
* Splitting a hole between two cells has a systematic sharp bias of about +5 cents. It is not better.

**Fix.** Fit a per-hole extra chimney offline against the TMM, so that each note whose first open hole it is
keeps its TMM frequency. That is 16 numbers, computed by a 20-line script in `tools/` and stored in `kSpec`.
* With a global factor ×2, the lumped end and λ = 0.99446, the FDTD is within −0.3 to +3.3 cents of TMM
  for C4–C5 (A4 +2.8, B4 +3.3).
* A per-hole fit should bring this under 1 cent.

Note that calibration retunes each note anyway. The benefit is correct cross-fingerings,
harmonics/overblowing and trill fingerings, not first-octave pitch. Effort S–M, risk low.

### F4. The coupled explicit/implicit scheme is provably stable for fixed coefficients, and stable in long runs (no action)
**Argument.**
* The leapfrog with interface inertance L = ρdx/S_int and cell compliance C is a standard staggered
  (symplectic) scheme. Its energy is ½Σ C p² + ½Σ L u^{n+½}u^{n−½}, which stays positive when
  λ²(S_{i−½}+S_{i+½})/(2S_i) ≤ 1 for every cell.
* The min-area rule makes S_int ≤ S_i, so the condition holds whenever λ ≤ 1. This is why min-area is needed; F1 shows the alternative failing.
* The branches enter continuity through the averaged flow (q^{n+1}+q^n)/2, which is centred at n+½ like J.
* The trapezoidal rule maps a passive RL network to a passive discrete one. The loss sections are trapezoidal
  R‖L with c_k > 0, plus the centred c0 term, so they are discretely positive-real: min Re of the discrete fit is 9.05 > 0.
* The scheme is therefore passive for frozen coefficients. The only energy sources are the jet and the
  coefficient changes (see F6).

**Experiments.**
* 600 s with random fingerings every 12–300 ms, random lung pressure (160–1440 Pa), coverage and lip
  height: no NaN, peak bore pressure 2.1 kPa, output peak 1.24.
* Also run: 120 s at 3 kPa with fingerings every 5 ms, λ = 0.99999, and the F1/F2/F3 fixes. All stable.
* Decay after release: Σp² falls from 2.7e−10 at 1 s to 1e−16 at 2 s and keeps falling. No DC drift; the mean bore p goes to 0.
  The DC path is the radiation inertances (the horn termination's `termL_`, the hole and window `hl`), which short DC to ground.
* Dispersion (lossless, p=0 end): the error is −0.01 cent at 720 Hz and −0.39 cent at 3.9 kHz, matching
  sin(ωdt/2) = λ sin(k dx/2). λ is irrelevant for accuracy; set it by F2.

### F5. Trapezoidal stiffness: Nyquist ringing in a closing pad's flow (low impact)
**Evidence.**
* As a pad closes, `rs = 6μ ln(R/b)/(π hp³)` grows without bound. The branch mode's trapezoidal amplification
  tends to −1, because the rule is A-stable but not L-stable.
* In the last ~1 ms before the s<1e-4 cutoff, q at hole 13 alternates at exactly fs/2: +4.9e−7, −4.9e−7, +4.8e−7, …
  (`exp/nyq.py`). Above 20 kHz, q's energy fraction rises from 1.5e−9 to 1.1e−5 during the closing.
* The bore pressure does not see it, because it uses (q⁺+q)/2 and the alternation cancels.
* The radiated d(q)/dt does see it: about 10 % of that hole's radiated sample-to-sample change, for ~1 ms, at 24 kHz.
* At the cutoff, q and l are reset to 0, a small discontinuity.

It is inaudible on a normal DAC but would alias under resampling or nonlinear post-processing.

**Fix.** Any one of:
* Radiate (q⁺+q)/2 instead of q.
* Use a θ-scheme (θ ≈ 0.55) or backward Euler for the q/l rows when g·rs·dt > 10.
* Clamp `rs·g·dt` (the closing then happens at hp ~ μm anyway).

Effort S.

### F6. Coefficient switching is not conservative (low impact, physically justified fix)
* Every 16 samples the hole coefficients change while p, q and l are held. A closing pad changes the closed-chimney
  `vol` and with it C_i by up to ~15 % at a hole cell (V_chimney ≈ 0.3 cm³ vs ≈ 2 cm³ cell volume).
* Holding p while C changes adds or removes ½ΔC p² of energy. Holding q while g changes alters the branch kinetic energy.
* No instability was seen (F4), but it is a parametric source. Conserving mass and momentum
  (rescale p by C_old/C_new, q by g_new/g_old) is the physically correct update.

Effort S, benefit small: cleaner key transitions and no tiny clicks.

### F7. Viscothermal loss: the fit, the 1.47 lumping and dispersion are all fine (no action)
* The sqrt(jω) fit is passive. Its error is ±5 % from 100 Hz to 4 kHz in both real and imaginary parts
  (Re +3.3 % at 262 Hz, +4.6 % at 2 kHz) and −4.4 % / −11 % at 50 Hz.
* Bilinear warping makes the discrete loss +35 % at 12 kHz and +97 % at 20 kHz. That only adds high-frequency damping.
  Optionally refit in the warped domain.
* Lumping thermal into series (×1.47) gives α and the phase-velocity correction to leading order
  (coefficient 1.039 vs Keefe 1.043). Only Z_c is slightly off.
  Measured effect against the full Keefe model: +0.24 cent and +2.6 % Q at 141 Hz, +0.05 cent at 2.7 kHz.
* FDTD Q vs Keefe theory (p=0 end): 34.5/34.2, 94/91, 150/152.

### F8. The min-area interface rule in the head taper: +1.5 cents (no action)
* True interface diameters inside the bore (`PJ_IFACE=5`) change mode 1 by +1.5 cents and the stretch by about −1 cent.
* The min rule biases the taper by half a cell (3.6 mm) and buys a stability proof (F4). Keep it.

### F9. Smaller notes
* `research/jet_and_losses.md` §4.3 gives Silva's Z_r for the e^{−iωt} convention. With e^{jωt} its
  real part is *negative* (Re Z = −0.0025 at ka = 0.1). The R(ω) form there is fine with j → −j.
  Anyone who uses that Z_r as written gets a Q that is too high. `tools/tmm.py` does not use it.
* No explicit FTZ/DAZ: the bore decays into denormals (|p| ~1e−38 after 20 s of silence) in non-fast-math builds,
  e.g. `make scalar`. The cost measured here is ~20 %. The fast-math build sets FTZ via crtfastmath.
  Set the MXCSR bits explicitly in the audio thread.
* `tools/tmm.py` with no argument uses extra = 0 mm while the FDTD uses 2 mm. Make 2 mm the default so the
  cross-check compares like with like.
* There is no automated FDTD-vs-TMM regression. Add the `-imp` vs `tmm.py` comparison as a test (tolerance 3 cents, Q 10 %).

## Prioritised fix list

| # | Fix | Benefit | Effort | Risk |
|---|---|---|---|---|
| 1 | Lumped LS end at the foot (F1a), or r0 = 0.35a (F1b) | C4 +8 cents correct; correct foot Q and HF radiation; −25 % bore cost (a) | S–M | low; re-check timbre |
| 2 | λ = 0.99446 so the foot is exact; compensate the embouchure offset (F2) | C4–E4 +6 cents correct | S | low |
| 3 | Per-hole fitted chimney compensation, or ×2 interim (F3) | hole notes ±12 cents → ≤3 (≤1 with the fit); better cross-fingerings and harmonics | S–M | low |
| 4 | FDTD vs TMM regression test; tmm default extra = 2 mm (F9) | keeps 1–3 correct | S | none |
| 5 | Radiate (q⁺+q)/2 or θ-scheme for stiff pads (F5) | removes Nyquist bursts at key closings | S | none |
| 6 | Mass/momentum-conserving coefficient switching (F6) | cleaner key transitions | S | low |
| 7 | Physics, not numerics: add hole-chimney viscothermal loss and a hole-edge nonlinear loss; reconsider the head-taper and window values if the stretch is wrong (see Open questions) | realistic passive Q | M | medium (retuning) |

The expected outcome of fixes 1–3, measured in my copy: FDTD mode 1 within −0.3 to +3.3 cents of TMM
for C4–C5, Q unchanged, all stability runs still clean.

## Open questions
* **Is a ~36-cent passive stretch real?** The UNSW page for C4 (newt.phys.unsw.edu.au/music/flute/modernB/C4.html)
  calls the minima "rather regular and harmonic" but publishes no numbers; the data files would settle it.
  UNSW measures with the embouchure open and no lip, so compare against `uncovered ≈ 12 mm`, `extra = 0`.
  In TMM that setting still stretches by +33 cents, so the window is not the cause.
  The two largest contributions in the model are the head taper (+14 cents) and the closed holes (+9 cents).
  The head-taper profile (linear 17.07 → 18.8 mm over 113.5 mm) is worth checking against the measured PF-661 profile.
  *Speculation:* real flutes are played with a jet whose effective end correction flattens upper registers (Coltman),
  so some passive stretch is expected.
* **The target Q for the passive bore.** No published Q numbers were found. The boundary-layer-only Q (46 at 262 Hz)
  is a known underestimate for real instruments, but by how much for this flute is unknown.
* **Is the min-area rule stable for the head socket step (19.8 mm over 4 mm)?** It is proven for any geometry,
  but true-diameter interfaces (F8) were only shown stable empirically.
