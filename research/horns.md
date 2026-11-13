# Brass ("lip-reed") instruments on the pajupilli waveguide — research notes

Status: 2026-11-12. SI units. c = 349.5 m/s, ρ = 1.18 kg/m³ (the bore's air, `Bore::kC`, `Bore::kRho`).
**[G]** marks my estimates or guesses, not published values. **[D]** marks values I derived from published
formulas or data. **[U]** marks a claim taken from a secondary source or an abstract, which I could not check
in the primary text. References are numbered; the list is at the end.

The French horn comes first. The trumpet, trombone, tuba, flugelhorn, natural horn, alphorn and cornett/serpent
follow where they differ. Short answer: the bore, the losses and the branch machinery carry over almost as they are.
The new parts are, in order of importance:
(1) an **outward-striking lip valve** whose resonance the player retunes for every note;
(2) a **lumped mouthpiece** (the cup and throat are smaller than one cell);
(3) **longer bores**: a horn in F is 3.75 m plus its valve loops, which does not fit 256 cells at a useful rate;
(4) **valve junctions** (a cell with three neighbours) and a **variable-length slide**;
(5) a **wide-band bell radiation load** plus the horn player's **hand**;
(6) optionally, **nonlinear steepening** ("brassiness") at ff, done outside the feedback loop.

---

## 0. What carries over from the woodwind model

| Woodwind part (exists) | Brass use | Change needed |
|---|---|---|
| Leapfrog bore, per-cell area, `setSeries`, lumped `cellVol_` | Mouthpiece backbore, leadpipe, valve section, bell | Mouthpiece cup as a volume in cell 0 and the throat as a series inertance (§2). The bell's last cells may need spherical-wave areas (§3.5). |
| sqrt(jω) loss fit, 3 sections | Same physics. Brass bores are 4–20 mm in the narrow parts, so losses set the impedance peak heights | Nothing. The fit range 50 Hz–12 kHz covers brass. A tuba's pedal (~29 Hz) is below it; the error there is small **[G]**. |
| Implicit side branches (`Branch`) | The horn's open bell end, the hand, mutes, the cornett's/serpent's finger holes | The bell needs a wider-band radiation network (§5): one more state at that cell. |
| `Reed` (inward-striking, beating) | Template for the lip | Flip the sign (outward-striking). Retune f_l per note. The opening scales as H ∝ 1/f_l. Contact instead of beating. Sweep flow (§1). |
| Calibration (`calibrate.cc`) per note | Find per-note f_l and pressure so that the intended **partial** sounds in tune | New search variable: f_l rather than lip coverage (§7, §9). |
| Room coupling through every opening | The bell | The bell is large and beams sound, so a directional source helps (§5.3). |

---

## 1. The lip reed

### 1.1 Valve types (Fletcher's classification)

Fletcher [F93] classifies pressure-controlled valves by the sign of their response to the upstream (mouth) and
downstream (mouthpiece) pressures, written (σ1, σ2):

| Type | (σ1, σ2) | Picture | Plays relative to the valve's own resonance f_l | Examples |
|---|---|---|---|---|
| Inward-striking ("blown closed") | (−, +) | reed pushed shut by the mouth pressure | **below** f_l | clarinet, oboe (what `Reed` does now) |
| Outward-striking ("swinging door", "blown open") | (+, −) | lips pushed open, into the cup | **above** f_l | the brass lips in the 1-DOF models [EB82, VVG17, MGV22] |
| Sideways / transverse ("sliding door") | (+, +) | lips pulled together by Bernoulli suction, moving across the flow | near f_l | the high-register lip motion, vocal folds |

Measurements on players:
- Newton, Campbell & Gilbert 2008 [NCG08] used high-speed video on 4 trombonists and 3 notes. **Every measurement showed
  a strong outward-striking mechanical resonance, and the played note was always above it.** A weaker second
  resonance, above the played frequency, behaved as inward-striking. The Q of human lips was **lower than that of
  artificial lips**. (Abstract verified. I did not get the numbers from the full text.)
- Boutin, Fletcher, Smith & Wolfe 2013 [BFSW13] measured trombone Bb2 (bore resonance 113–114 Hz) and Bb3 (227.7–229.4 Hz) with six
  players. The played frequency was **always above** the bore resonance for Bb2 and **always below** it for Bb3. The
  results fit a (+1,−1) valve for Bb2 and a (−1,+1) one for Bb3. At Bb3 the lip motion is mainly transverse.
- Boutin, Smith & Wolfe 2020 [BSW20]: players normally play **a little above the bore impedance peak**. They can **lip up
  about half a semitone and down about a whole tone**. The range is roughly symmetric about the impedance peak,
  and its limits sit where |Z| ≈ 15 % of the peak and the phase is −69° to +75°. The flow into the bore has two parts: the
  aperture flow, and the **sweeping flow** of the moving lips (S_r dx/dt). The sweeping work is ~20× smaller than the
  acoustic work but similar to the lips' kinetic energy, so it can carry the oscillation.
- Yoshikawa 1995 [Y95] (abstract only): outward striking is favoured when the mouth–cup coupling is negligible.

**Consequence for the model.** A one-mass outward-striking valve plays the low and middle registers correctly.
Higher up it plays sharp, by tens of cents **[G]**, unless the calibration absorbs this through f_l. The
two-mode (outward + inward) model fixes this physically (§1.4). Start with 1-DOF.

### 1.2 The one-mass outward-striking model (the "elementary" brass model)

From Elliott & Bowsher 1982 [EB82], as written by Velut et al. 2017 [VVG17] and Mattéoli et al. 2022 [MGV22]:

```
h'' + (ω_l/Q_l) h' + ω_l² (h − H) = (p_m − p) / μ          (μ: lip mass per unit area, kg/m²)
U_lip = w · max(h, 0) · sgn(p_m − p) · sqrt(2 |p_m − p| / ρ)    (Bernoulli; the jet's energy is lost in the cup)
U     = U_lip + S_r · h'                                   (sweeping flow, [AS96, HBPW15, BSW20])
```

Compare `Reed`: `x'' + q_r ω_r x' + ω_r² x = (p_m − p)/μ` with `h = H − x`. The lip is the same oscillator with
**h = H + x**. The mouth pressure pushes it open, and the mouthpiece pressure pushes it shut. The lips do not beat
against a lay. They close (h = 0) once per cycle in normal playing, and in the models they may overlap (h < 0) with zero flow
[MGV22]. Vergez & Rodet [VR01] smooth the closure with an effective lip length l·tanh(h/ε) so that dU/dt stays
continuous. They report that this removes "disturbing high frequencies". **Do this** (the reed already uses a soft contact).

### 1.3 Parameter sets from the literature

| Source | Instrument | f_l | Q_l | μ | H (rest opening) | w (width) | S_r | Notes |
|---|---|---|---|---|---|---|---|---|
| Velut et al. 2017 [VVG17] via Mattéoli et al. 2022 [MGV22, Table 1] | trombone | swept 50–500 Hz (the control) | **7.0** | 1/μ = 0.11 m²/kg → **μ ≈ 9.1 kg/m²** [D] | **0.5 mm** (constant), or **H = k/f_l, k = 0.074 m·Hz** (fit to Elliott & Bowsher's measurements) | **12 mm** | — | ρ = 1.2, c = 340 |
| Harrison, Bilbao, Perry, Wishart 2015 [HBPW15] score file (they cite Adachi & Sato) | trumpet | 550 Hz (example) | σ = 5 s⁻¹ (as printed; this would be Q ≈ 700, which is suspicious; **[U]** units/meaning) | m = 5.37·10⁻⁵ kg, S_r = 1.46·10⁻⁵ m² → **m/S_r ≈ 3.7 kg/m²** [D] | **0.29 mm** | **10 mm** | 1.46·10⁻⁵ m² | p_m = 5 kPa |
| Berjamin et al. 2017 [BLVC17] | trumpet-length cylinder (R = 7 mm, 1.4 m) | from k | r = √(mk)/4 | Table 2 (garbled in my text extraction; **[U]**) | — | — | — | p_m up to 20 kPa |
| Fletcher & Tarnopolsky 1999 [FT99] | trumpet (fit to players) | — | effective Q of the bore resonance ≈ 10 | — | opening saturates at ~1 mm | — | — | area S ≈ C x^{3/2}, **C ≈ 0.05 m^{1/2}**; peak bore impedance R ~ 10⁸ Pa·s/m³ |

Derived stiffness per area and "opening pressure" **[D]** (μ = 9.1 kg/m², H = 0.074/f_l):

| f_l (Hz) | k = μ ω_l² (Pa/m) | H (mm) | k·H (Pa) — pressure that would double the opening |
|---|---|---|---|
| 60 (tuba/trombone pedal) | 1.3·10⁶ | 1.23 | 1 600 |
| 175 (horn F3, trombone F3) | 1.1·10⁷ | 0.42 | 4 600 |
| 350 (horn F4, trumpet F4) | 4.4·10⁷ | 0.21 | 9 300 |
| 700 (horn F5, trumpet F5) | 1.8·10⁸ | 0.11 | 18 600 |
| 1050 (trumpet C6) | 4.0·10⁸ | 0.07 | 28 000 |

The last column grows ∝ f_l. This matches Fletcher & Tarnopolsky's finding [FT99] that the **threshold pressure
∝ playing frequency**, p_0 = K f with **K ≈ 4–8 Pa/Hz** (three players: 7, 8, 4 Pa/Hz). They argue that the lip's
vibrating mass falls as its tension rises (m ∝ 1/T, after [EB82]), so p_0 ∝ f, not f².

**Recommended start [G]:** μ = 5 kg/m² (between the two sets), Q_l = 5 (human lips are less sharp than artificial ones [NCG08]),
w = 10 mm (trumpet, horn) to 14 mm (trombone, tuba), H = 0.074/f_l m (clamped to 0.05–1.5 mm), S_r = w × 1.5 mm
**[G]**, and f_l from the per-note calibration (§9). Keep μ and Q per instrument.

### 1.4 Two-mass / two-mode models

- **Adachi & Sato 1996** [AS96]: a 2-D lip. It combines the swinging-door motion (a hinged mass rotating outward) with a
  transverse (stretching) motion. The low modes have a positive phase between lip motion and mouthpiece pressure,
  and the higher modes a negative one. It reproduced trumpet tones and the lowest notes. Parameters in the JASA paper
  (I could not extract them; the ATR preprint TR-H-0177 [AS95] is a scan).
- **Cullen, Gilbert & Campbell 2000** [CGC00]: an artificial mouth plus linear stability analysis. Two lip resonances (one
  outward, one inward-striking) are needed to explain the measured thresholds and playing frequencies. Cullen's PhD
  thesis (Edinburgh 2000) has the one-mass model and the stability analysis in detail.
- **Newton/Campbell/Gilbert 2008** [NCG08]: the pair of resonances is seen in human players too (the second one weaker).
- **Velut et al. 2017** [VVG17]: 1-DOF outward valve plus a modal trombone. Linear stability analysis predicts the threshold
  and the frequency well. Above threshold the simulation also shows over-blowing, quasi-periodicity and period doubling.
- **Mattéoli et al. 2022** [MGV22]: continuation of the same model. Each regime n has an optimal f_l,n that minimises the
  threshold (a "U-shaped" threshold curve vs f_l, one U per natural note). With **H ∝ 1/f_l** the thresholds agree
  better with measurement. The **pedal note** of the trombone comes out about **a semitone lower** than linear stability
  predicts. A euphonium (mostly expanding bore) also has a **"ghost note"** between the pedal and the 2nd regime, ≈ a
  minor third to a fourth above the pedal. A model of a strongly conical instrument should reproduce it.

**Two-mode recipe [G]** (if 1-DOF intonation in the upper register is not good enough): add a second mode with
f_2 ≈ 1.3–1.6 f_l, Q ≈ 3, a smaller effective area (×0.3), and opposite sign (inward-striking). The opening is
h = H + x_1 − x_2. Cost: 2 more states, negligible.

### 1.5 How the lip resonance tracks the note

- The player sets f_l by lip tension and mass (embouchure: muscle tension, how much lip is in the cup, mouthpiece
  pressure on the lips). Vergez & Rodet [VR01] measured that lip "contraction" increases monotonically with the force of
  the mouthpiece on the lips, and used that force as a control.
- With an outward-striking valve, the playing frequency is **above f_l** and close to the selected bore resonance
  [NCG08, MGV22]. In the models f_l,opt sits a few % below the resonance it selects. **[G]**: f_l ≈ 0.90–0.97 × f_n.
  Calibrate per note (§9) rather than fixing it.
- The bore pulls the frequency: in measurements the playing frequency follows the resonance frequency. **"For most
  regimes, variations of the resonance frequency lead to same-order variations of the playing frequency"**, and the
  dynamics change the pitch little (Eveno et al. 2014 [EPGKC14], abstract). Moving f_l within a slot bends the pitch
  by about **+½ semitone / −1 tone** before the note breaks or jumps [BSW20].
- Breath and CO₂: the bore resonances of a trombone **drop 0.48–0.62 %** after 3–10 s of playing, because exhaled CO₂
  outweighs the warming [BFSW13]. pajupilli's exhaled-air c = 349.5 m/s already assumes warm, CO₂-rich air. Real brass
  is colder than a flute (long tube, metal), so **[G]** the brass c should be ~345 m/s (≈ 25 °C). Set it per instrument
  only if the calibration shows a global offset.

---

## 2. The mouthpiece

### 2.1 Geometry and role

Rim → **cup** (a volume, a compliance) → **throat** (the narrowest point, an inertance) → **backbore** (a ~cone,
widening to the leadpipe). Cup plus throat form a Helmholtz resonator, the **popping frequency** (heard when you slap the
rim on the palm). It raises the impedance peaks around itself and lowers them far above it. It is the
"formant" of the instrument's response [M16, UNSW].

| Mouthpiece | Popping frequency | Source |
|---|---|---|
| Trumpet (Bach 1¼C) | ≈ 800 Hz | maker, quoted in [RH12] |
| Trumpet (typical) | 750–850 Hz | [RH12] and course notes **[U]** |
| Trumpet / cornet / flugelhorn (FE model, input pressure peak) | ≈ 790 / 660 / 1150 Hz | Rose & Holloway 2012 [RH12] |
| Trombone, horn, tuba | not found in a primary source. **[G]**: trombone ~500–600 Hz, horn ~600–750 Hz (deep funnel cup), tuba ~300–400 Hz | — |

Typical dimensions **[G, from common maker specifications; check against a drawing of the chosen mouthpiece]**:

| | Cup inner rim Ø | Cup volume | Throat Ø / length | Backbore length | Shank → leadpipe Ø |
|---|---|---|---|---|---|
| Trumpet | 16–17 mm | 0.8–1.2 cm³ | 3.6–3.8 mm / 5–8 mm | 60–70 mm | ~9 mm |
| Horn | 17–18 mm | 1.2–1.8 cm³ (deep, V-shaped) | 4.2–4.6 mm / 5–10 mm | 50–60 mm | ~8.5 mm |
| Tenor trombone | 25–26 mm | 3–4 cm³ | 6.5–7 mm / ~8 mm | ~70 mm | ~11 mm |
| Tuba | 32–33 mm | 8–12 cm³ | 7.5–8.5 mm | ~80 mm | ~13 mm |

Consistency check **[D]**: the Helmholtz frequency f = (c/2π)·√(S_t/(V·L_eff)). For a trumpet with throat Ø 3.66 mm
(S_t = 1.05·10⁻⁵ m²) and V = 1.0 cm³, f = 800 Hz requires L_eff ≈ 50 mm. That is the throat plus part of the backbore,
which is plausible.

### 2.2 In the waveguide

At the rates in §3.4, dx = 11–15 mm. **The cup and the throat are each smaller than one cell.**
- **Cup** → a lumped volume in cell 0, like the flute's cork cavity (`cellVol_[0]`). The lips are the boundary of cell 0
  (the reed is already solved this way at `ec_`).
- **Throat** → a series length via `setSeries` at the first interface: Δℓ = S_cell·∫dx/S(x) − dx over the throat
  region. **Important:** the throat's inertance is set by ∫dx/S (a harmonic mean). The grid's mean area per cell (the
  4-point average in `setBore`) **underestimates** the inertance of a constriction that is narrower than its cell.
  Let the bore loader compute ∫S dx (compliance) and ∫dx/S (inertance) per cell separately for the mouthpiece and the
  valve ports, and put the difference into `setSeries` **[G, engineering]**.
- The **backbore** (50–80 mm) spans 4–7 cells. Resolve it.
- Bilbao & Chick 2013 [BC13] warn of exactly this: at lower rates "resolution of the fine features of the bore
  profile (particularly in the mouthpiece section) can be too crudely modelled, particularly if the cup volume is poorly
  approximated". Validate the 3rd–8th peak heights and frequencies (§9.3).
- Losses: the throat (Ø 3.7 mm) and the leadpipe are where the viscothermal loss per length is highest. The existing
  per-interface κ ∝ 1/(S·a) handles this. Make sure κ uses the true throat radius, not the cell average.

---

## 3. The bore

### 3.1 Profiles

| Instrument | Tube length (no valves) | Character | Bell Ø | Pedal / nominal |
|---|---|---|---|---|
| Horn in F (double horn, F side) | **≈ 3.75 m** [PL10] | narrow (main bore ~11.5–12 mm), long conical leadpipe, cylindrical valve section, long gradual **conical-then-Bessel** taper into a wide bell | 300–310 mm **[G]** | F1 43.7 Hz |
| Horn in B♭ (B♭ side) | ≈ 2.75 m **[G]** (= 3.75 × 2^(−5/12) ≈ 2.81 m) | as above | shared bell | B♭1 58.3 Hz |
| Natural horn crooked in D | **≈ 4.51 m** [BC13] | crooks, no valves | — | D1 36.7 Hz |
| B♭ trumpet | **≈ 1.38 m** [BC13]; "≈ 1.4 m" [PL10] | ~1/3 cylindrical (valves, tuning slide), leadpipe conical; bore 11.1–11.7 mm [MPGC10] | ~123 mm **[G]** | B♭2 116.5 Hz |
| Tenor trombone (1st pos.) | **≈ 2.74 m** [BC13] | cylindrical slide (~13.3–13.9 mm **[G]**), conical/Bessel bell | **~180 mm** [PL10] (bass trombone ~1/3 larger) | B♭1 58.3 Hz |
| Flugelhorn B♭ | ≈ 1.3–1.4 m **[G]** | mostly conical, wide | ~150 mm **[G]** | B♭2 |
| Euphonium B♭ | ≈ 2.7 m **[G]** | mostly conical | ~300 mm **[G]** | B♭1, + a "ghost note" [MGV22] |
| Tuba (F / E♭ / C / B♭B♭) | ≈ 3.7 / 4.1 / 4.9 / 5.5 m **[G]** | conical | 370–500 mm **[G]** | F1 … B♭0 29 Hz |
| Alphorn | ≈ 3.4–3.6 m **[G]** | long, almost conical wooden tube, small upturned bell; no valves, no holes | ~150–200 mm **[G]** | partials 2–16 |
| Cornett / serpent | ≈ 0.6 m / ≈ 2.3 m **[G]** | conical, **finger holes** (6–7), tiny cup mouthpiece | — | — |

The cornett and serpent are **lip-reed instruments with tone holes**. They would reuse pajupilli's hole and fingering
machinery directly, which makes them the cheapest brass to add after the lip. The cornett is about the length of a
tenor recorder (UNSW [UNSW]).

### 3.2 Why the resonances end up nearly harmonic

A closed–open cylinder resonates at odd multiples, 1:3:5:7. In brass, two corrections bend this series into a
harmonic one [M16, UNSW]:
1. **The bell** reflects low frequencies early. The low waves "see" a shorter tube because they turn back where the
   horn function (§3.3) exceeds k². This raises the low resonances more than the high ones.
2. **The mouthpiece** (a volume plus a constriction) adds an effective length that grows with frequency up to the popping
   frequency. This lowers the upper resonances.

The design target is a series 2:3:4:5:6:… of a **missing fundamental**. The real first resonance is far flat and seldom
used. Players can still play a **pedal note** at the missing fundamental, sustained by the 2nd, 3rd, … resonances
locking onto its harmonics (mode locking) [UNSW, MGV22].

Measured impedance maxima (Bilbao & Chick 2013 [BC13], Table I, measured column; the model columns agree to ≲ 1 %):

| Peak n | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| Trumpet B♭ (20 °C), Hz | ~83.5 | 234.2 | 353.1 | 469.1 | 591.6 | 702.7 | 812.5 |
| f_n / n | — | 117.1 | 117.7 | 117.3 | 118.3 | 117.1 | 116.1 |
| Trombone (1st pos., 19 °C), Hz | 38.3 | 113.2 | 174.0 | 233.2 | 296.0 | 350.9 | 410.1 |
| f_n / n | — | 56.6 | 58.0 | 58.3 | 59.2 | 58.5 | 58.6 |
| Natural horn in D (4.51 m, 20 °C), Hz | 23.2 | 64.8 | 103.7 | 144.7 | 181.9 | 220.3 | 256.2 |
| f_n / (n+?) | — | 32.4 (n=2) | 34.6 (3) | 36.2 (4) | 36.4 (5) | 36.7 (6) | 36.6 (7) |

[D]: the horn's low peaks are flat against the D1 = 36.7 Hz series, by ~−210 cents at n = 2 and −100 cents at n = 3. They
reach it only from n ≈ 5. The horn is played mostly on partials 3–16 (F horn: written C3–C6 ≈ partials 2–16), where the
series is good and dense. Neighbouring partials 8–16 are only a whole tone to a semitone apart, which is why horn
players "crack" notes (§7.4).

### 3.3 The bell, the horn function and the cut-off

For a slowly varying axisymmetric bore with radius r(x), write the plane-wave (Webster) equation for ψ = r·p:

```
ψ'' + (k² − U(x)) ψ = 0,     U(x) = r''(x) / r(x)        ("horn function", Benade)
```

Where k² < U the wave is evanescent: the **barrier** that reflects low frequencies before the bell mouth. For a
**Bessel horn** r(x) = B·(x₀ − x)^(−ε) (x₀ the virtual apex past the mouth, y = x₀ − x) the barrier is
U = ε(ε+1)/y². Its local cut-off is **f_c(y) = c·√(ε(ε+1)) / (2π y)** [D]. A wave of frequency f turns back at
y ≈ c√(ε(ε+1))/(2πf). The higher the frequency, the nearer to the mouth it reflects, so the acoustic length grows with f.
Above the barrier's maximum (near the mouth) almost nothing reflects: this is the **bell cut-off**. Above it the
impedance peaks fade.

| Instrument | Cut-off (from impedance / radiation) | Source |
|---|---|---|
| Horn (no hand) | **≈ 500 Hz**. Peaks "greatly diminished above 450 Hz, gone above 750 Hz" | Dell et al. 2010 [DJDW10] (citing earlier work) |
| Horn (hand in bell) | peaks persist to ≈ 1500 Hz **[U]** (HyperPhysics) | §4.3 |
| Trumpet | "typically about 1000 Hz" (Luce & Clark, quoted in [FT99]); ~1200 Hz for one trumpet **[U]** (Acoustics Today 2021) | [FT99] |
| Trombone | radiated level falls "rather rapidly after 630 Hz, suggesting the instrument cut-off" | Pätynen & Lokki 2010 [PL10] |

For comparison, ka = 1 at the mouth is reached at **c/(2πa) ≈ 360 Hz (horn, a = 0.155 m), 515 Hz (trombone),
900 Hz (trumpet)** [D]. The bell cut-off is of the same order but set by the flare, not by ka alone.

**The leapfrog already contains the barrier.** Cell areas varying along x make the scheme solve Webster's equation.
The only requirement is **enough cells in the flare**. A horn bell goes from r ≈ 20 mm to 155 mm over the last ~0.3 m
**[G]**, i.e. 20–27 cells at dx = 11–15 mm. Bilbao & Chick [BC13] used the same plane-wave Webster model with a
measured bore (at 88.2 kHz). They matched the first seven maxima and minima of trumpet, trombone and natural horn to
**under a percent in most cases and within a dB**. For the horn the calculated peaks were slightly low in frequency
and too high in amplitude. The first horn peak was the hardest.

### 3.4 Lengths, cells and the sample rate

Cells N = L / dx, dx = c/(0.975 fs) **[D]** (c = 349.5 m/s):

| Instrument / configuration | L (m) | N at 48 kHz | fs for N = 256 | fs for N = 192 | ka = 1 at mouth (Hz) |
|---|---|---|---|---|---|
| Trumpet B♭, open | 1.48 | 198 | 62.0 kHz | 46.5 kHz | ~900 |
| Trumpet B♭, 1+2+3 | 2.09 | 280 | 43.8 kHz | 32.9 kHz | ~900 |
| Flugelhorn / cornet | 1.40 | 187 | 65.5 kHz | 49.2 kHz | 700–930 |
| Horn, B♭ side, open | 2.75 | 368 | 33.4 kHz | 25.0 kHz | ~360 |
| **Horn, F side, open** | **3.75** | **502** | **24.5 kHz** | 18.4 kHz | ~360 |
| Horn F, 1+2+3 (path length) | 5.30 | 710 | 17.3 kHz | 13.0 kHz | ~360 |
| Natural horn in D | 4.51 | 604 | 20.3 kHz | 15.3 kHz | ~400 |
| Tenor trombone, 1st position | 2.74 | 367 | 33.5 kHz | 25.1 kHz | ~515 |
| Tenor trombone, 7th position | 3.87 | 519 | 23.7 kHz | 17.8 kHz | ~515 |
| Bass trombone, both triggers, 7th | 5.81 | 777 | 15.8 kHz | 11.9 kHz | ~430 |
| Euphonium | 2.75 | 368 | 33.4 kHz | 25.0 kHz | ~370 |
| F tuba | 3.75 | 502 | 24.5 kHz | 18.4 kHz | ~265 |
| B♭B♭ tuba, open | 5.50 | 736 | 16.7 kHz | 12.5 kHz | ~240 |
| Alphorn | 3.50 | 469 | 26.2 kHz | 19.7 kHz | ~700 |

The table counts **path length**. A valved instrument with partial-valve capability must hold **both** branches
of every valve in memory (§4.1), so its cell count is the open length plus all valve loops. A single F horn with three valves
holds ≈ 3.75 + 0.46 + 0.22 + 0.71 ≈ **5.1 m** of tube [D] (loops sized for 2, 1 and 3 semitones; real loops are a little longer
for tuning slides).

**Bandwidth needed.** The horn's radiated spectrum falls **−21 dB/oct (p), −13 (f), −9 (ff) above 800 Hz** [PL10]
(Luce & Clark: −15 dB/oct). The trumpet's brightens at ff up to a transition of 2–3 kHz and beyond [FT99]. **[G]**: a horn
modelled to 10–12 kHz (fs ≈ 24–32 kHz) is not audibly band-limited. Trumpet and trombone ff need ≥ 16 kHz
(fs ≥ 40 kHz). Above the model's band, the nonlinear stage (§6) can extend the spectrum in the output, outside the loop.

**Proposal [G]:** raise `kCells` to **512** (or make it a template parameter). Run: horn F (single) at 32 kHz
(dx = 11.2 mm, ≈ 460 cells including valve loops); double horn at 24–26 kHz (both sides ≈ 512 cells); trumpet with valves at 48 kHz
(≈ 300 cells); trombone at 32 kHz (7th position ≈ 360 cells); tuba at 22–24 kHz. Cost: 512 cells × 32 kHz = 16 M
cell-steps/s, ~3.5× the flute's 96 × 48 kHz. That is still a few % of a core with AVX-512 **[D from the README's 1–2 %]**.
Harrison et al. [HBPW15] report an AVX FDTD horn (4.5 m, 3 valves, 44.1 kHz) at 3.6 s per second of sound on a 2014 laptop CPU;
it was not optimised as pajupilli is. A trumpet was faster than real time.

### 3.5 Is 1-D plane-wave propagation adequate in the bell?

- **Plane vs spherical wavefronts.** In a fast flare the wavefront is a curved cap, not a plane section. Using the plane
  area overestimates the effective length near the mouth. Benade & Jansson 1974 [BJ74] showed that spherical-cap
  (equal-volume) wavefronts give better resonance predictions. Bilbao & Chick [BC13]: the residual high-frequency
  deviations are "a feature of 1D models which can be partly rectified using spherical (rather than planar) models of
  wave propagation, but can ultimately only be fully resolved using a multimodal approach". Spherical generalisations
  "do not pose any major difficulties" in FDTD.
  **Practical correction [G]:** for the cells where the wall angle θ exceeds ~10°, replace the area by the cap area
  S_cap = 2πR²(1−cos θ) (R the radius of a sphere centred on the axis and normal to the wall), and use the arc length along the cap normals as the
  axial coordinate. This is one geometry-preprocessing step, at no run-time cost. Validate against the measured
  impedance (§9.3).
- **Higher modes.** The first axisymmetric non-plane mode cuts on at f = 3.832·c/(2πa). At the mouth that is ≈ 1.4 kHz
  (horn), 2.4 kHz (trombone), 3.4 kHz (trumpet) [D], all above the bell cut-off. Only the last few cm of the bell are affected, and
  only above cut-off, where the reflection is already small. Ignore them **[G]**.
- **Webster–Lokshin** (Hélie; Mignot, Hélie & Matignon [MHM10]) is the 1-D equation with a curvature term and fractional
  losses, built for waveguides of flared pipes. It needs an axisymmetric bore, so slide and tubing bends are
  "virtually straightened". It is equivalent to what pajupilli's FDTD does, plus the curvilinear-area correction above.
- **Bends**: their effect is frequency-dependent and "mostly, though not always, small". It is negligible for horn and
  trombone, and less so for the trumpet's tight valve bends [BC13]. Ignore them.
- **Wall vibration**: bell vibrations do change the sound measurably (Moore et al. 2005; Kausel et al. 2015, cited in [M16]).
  This is a second-order effect. Ignore it.

---

## 4. Valves, the slide and the hand

### 4.1 Valves

**Physics.** Pressing a valve diverts the air from a short "default" path into a longer "bypass" loop. On a
trumpet (and on each horn side), valves 2, 1 and 3 lower by **1, 2 and 3 semitones** (UNSW [UNSW]). Combinations are sharp
because each loop is sized for the open tube (1+3 and 1+2+3 need tuning slides on trumpet, or alternate fingerings).
The double horn's **thumb valve** switches the whole F extension (≈ 1 m) in or out. Half-pressed valves split the flow
between both paths. This gives "a set of inharmonically spaced resonances, leading to the production of multiphonic
tonal timbres" [HBPW15]: the muffled, unstable half-valve sound used in jazz trumpet and for horn glissandi.

**Model (Bilbao 2011; Harrison et al. 2015 [B11, HBPW15]).** At the junction the pressures in all three tubes are equal and the
volume flows sum to zero:

```
p_main(J) = p_default(0) = p_bypass(0),     U_main(J) = U_default(0) + U_bypass(0)
```

The valve opening scales the entrance and exit areas of the two branches, S_d = q_d·S, S_b = q_b·S, with q_d + q_b ≈ 1
along the stroke [HBPW15, DAFx-15]. The constricted ports are as long as the default tube. Each tube gets its own grid spacing.

**In pajupilli [G, design]:**
- Lay the tubes out as consecutive cell ranges in the same arrays: `[mouthpiece+leadpipe | valve-1 default | valve-1
  loop | valve-2 default | … | bell]`. One idle cell (u = 0) separates them. The vector loops stay branch-free.
- Each junction is a pressure node with three interfaces. Like the embouchure cell `ec`, junction nodes leave the vector
  node update and are solved in a small scalar step: C·dp/dt = Σ u_in − Σ u_out, plus a trapezoidal momentum update for the
  three port interfaces.
- The valve opening goes into the **port interfaces' area**: bu ∝ q (inertance ρ dx/(qS)). q → 0 closes a port
  (u → 0) and stays stable (a smaller coefficient). A real valve moves in **10–30 ms [G]**. Ramp q per block and
  re-solve only the port coefficients.
- Grid spacing: the loops can be any number of cells. A **fractional last cell** (endLen trick) or a series length
  (`setSeries`) at the port fixes each loop's exact length, so every tube shares dx and fs.
- Cost with both paths always computed (needed for partial valves and the transient): the cells of all loops (see
  §3.4). The idle path does nothing audible; fully closed ports keep it silent.

### 4.2 The trombone slide

The slide adds up to ≈ 2 × 0.57 m **[G]** of uniform cylinder (7th position: 6 semitones lower, 2.74 → 3.87 m, [D]). The grid
cannot move, so two options:
1. **Stretch the slide cells [G, recommended]:** let the slide section be a fixed number of cells N_s whose dx_s
   changes with position. dx_s is smallest in 1st position (Courant 0.975 there). In 7th position the local Courant number
   falls to ≈ 0.975 × L_min/L_max. The cost is mild numerical dispersion: with leapfrog the phase error at Courant λ is
   ≈ (1 − λ²)(k dx)²/24. At λ = 0.6, dx = 13 mm: ~2–3 cents at 1 kHz, more above **[D]**. Per block, only bu (∝ 1/dx_s) and the
   compliance (∝ dx_s) of those cells change. Moving the slide changes the stored energy slightly. At slide speeds of a few m/s
   (≪ c) this is the physical Doppler/parametric effect.
2. **Interpolated end** (Harrison/Bilbao): fixed dx, a moving fractional cell. It needs re-meshing when crossing a cell,
   which produces clicks unless handled carefully. Not recommended.

The trombone's F trigger (bass trombone: F and G♭/D triggers) are valves (§4.1).

### 4.3 The hand in the horn bell

**What it does** (Dell, James, Davidson & Wolfe 2010 [DJDW10]; UNSW [UNSW]):
- The hand raises the reflection at high frequency. The impedance peaks above ~450 Hz become stronger, and there are more of them, so
  high notes become playable and stable ("horns are occasionally required to play pitches above 500 Hz").
- It **lowers the frequencies of the resonances** and makes the sound darker and less loud ("bells up",
  *pavillons en l'air*, is loud and bright).
- Players' hands differ measurably. Peak frequencies changed by up to 0.6 Hz rms and amplitudes by 0.8 dB rms between replica hands. Players
  heard these differences as intonation differences.
- **Stopping** (closing the bell with the hand) changes pitch "by about a semitone or so" [UNSW] and the timbre to a buzzy,
  distant, nasal sound. In practice stopped horn (F side) sounds a semitone higher, and players finger a semitone lower.
  Physically the hand closure adds acoustic mass. Each resonance moves **down**, close to a semitone above the next lower
  resonance (HyperPhysics, Acoustics Today 2021 **[U]**; Backus 1976 not obtained). Hornists stop on the F side for intonation
  (Schuller, quoted on hornmatters.com **[U]**).
- Before valves (natural horn), partial hand-stopping was **the** way to get notes between partials.

**Model [G]:**
- **Open hand (normal):** reduce the area of the bell's last ~80–120 mm by the hand's cross-section, which grows to
  ~40–50 % of the bell area at the mouth. Reduce the radiating aperture area at the end accordingly. One geometry
  preset, applied once per block like a key change.
- **Hand position control (0 = bells up … 1 = normal … 2 = stopped):** interpolate the area profile. Towards 2 the
  aperture becomes a slit (~1–2 cm² **[G]**) with a series inertance. This is exactly the end `Branch` (g, rs, rr, hl). The
  "Branch" machinery already handles openings that close continuously (tone-hole pads).
- Validate against Dell et al.'s curves [DJDW10]: hand vs no hand, peak heights near 500 Hz.
- Mutes (straight, cup, wah-wah/Harmon, practice) are also bell-end geometry plus branch changes. The wah-wah is a hand over an
  open mute, moving the instrument's "formant" [UNSW].

---

## 5. Radiation from a large bell

### 5.1 Radiation impedance

The present open end is a **parallel R‖L** (resistance 1.505 ρc/S, end correction 0.6133 a). It fits Levine–Schwinger only
for ka ≪ 1. At the horn's mouth ka = 1 at ~360 Hz, inside the playing range, so a wide-band load is needed. Two time-domain
forms:
- **Silva et al. 2009 rational fit** (already in `jet_and_losses.md` §4.3; valid to ka ≈ 2–3).
- **Bilbao & Chick's network** as used by Harrison et al. [HBPW15], normalised by the mouth area S (a = mouth radius):

```
Z_rad = (L_r (R1+R2) jω + L_r R1 R2 C (jω)²) / (R1 + R2 + (L_r + R1 R2 C) jω + L_r R2 C (jω)²)
R1 = ρc,  R2 = 0.505 ρc,  L_r = 0.613 a,  C = 1.111 a / (ρ c²)      (as printed in [HBPW15])
```

(As printed the units are loose: read L_r as an inductance ρ·0.613a and the whole network as a specific impedance, then
divide by S for the acoustic impedance. **Check the low-ka limits** before use: Re Z → (ka)²/4·ρc/S, Im Z → 0.613 ka·ρc/S.)
This is two resistors, one inductor and one capacitor: **one more state** than the current R‖L. In `buildNode` the end
cell's x = (p, q, l) becomes (p, q, l, v_C), a 4×4 trapezoidal solve, only at the bell cell. Solve that cell scalar, like
the embouchure.

Unflanged vs flanged: a bell rim is not a flange, but the rapid flare acts partly like one. Unflanged is the usual
choice [BC13, HBPW15]. In the bell the spherical-cap area (§3.5) also changes S and a at the mouth. Use the cap's area
and its base radius consistently **[G]**.

### 5.2 The radiated signal

The far-field pressure of a small source is a monopole, p(r,t) = ρ/(4πr)·dU_bell/dt [BLVC17 eq. 56]. This is what pajupilli
already does at the foot. Above ka ≈ 1 the bell beams: on axis the level rises up to ~+6–10 dB relative to a monopole **[G]**, and off axis it
falls.

### 5.3 Directivity

Pätynen & Lokki 2010 [PL10] (anechoic, musicians playing):
- **Horn**: almost omnidirectional in the median plane up to 500 Hz. The bell dominates above 500 Hz, and above 1 kHz the
  directivity is "limited in the bell direction". The bell points **backwards-right and down**, so the audience hears mostly
  reflections at high frequency. The hand reduces radiation to higher elevations.
- **Trumpet**: symmetric about the axis, directivity increases from 1 kHz, "only within −6 dB" all round at C#5.
  Dynamics do not change the directivity, but loud playing emphasises > 2 kHz in front.
- **Trombone**: omnidirectional at 125–250 Hz, front-emphasised from 500 Hz, highly concentrated in the bell direction at ≥ 1 kHz.

**Model [G]:** in the room code, give the bell a direction. Feed the direct path and each image source through a
first-order shelving filter whose high-frequency gain follows the angle from the bell axis: 0 dB on axis, −10 to −15 dB
at 90°, −20 dB behind, corner frequency ≈ c/(2πa). For the horn, aim the bell at the back wall. This is the main reason a horn
sounds like a horn in a hall. In a dry mix use the off-axis (lowpassed) signal.

---

## 6. Nonlinear propagation ("brassiness")

### 6.1 Physics

- Hirschberg, Gilbert, Msallam & Wijnands 1996 [HGMW96]: at fortissimo the trombone's waves steepen into **shock waves**
  inside the slide, seen with schlieren photography (they travel ~1 % faster than c). This nonlinear propagation, not the lip,
  makes the loud tone bright. It "can be ignored" when modelling the oscillation, so the steepening can be added
  to existing models as a modest change.
- Each point of a simple wave travels at c + βu, with β = (γ+1)/2 ≈ 1.2. The shock-formation distance for a plane wave is
  **x_s = ρc³ / (β·(∂p/∂t)_max) = 2γP₀c / ((γ+1)(∂p/∂t)_max)** [D; the form used in [HGMW96]].
  With ρc³ = 5.04·10⁷ kg/s³: (∂p/∂t)_max = 10⁷ Pa/s → x_s ≈ 4.2 m; 3·10⁷ → 1.4 m; 10⁸ → 0.42 m.
  For a sinusoid, (∂p/∂t)_max = 2πfP: horn F4 at P = 5 kPa gives 1.1·10⁷ → 3.8 m; trumpet C5 at 10 kPa gives 3.3·10⁷ → 1.3 m [D].
  Real mouthpiece waveforms are steeper than sinusoids, because the lips close abruptly.
- Steepening accumulates **mostly in the narrow, cylindrical part**. It slows where the bore widens, because the
  amplitude drops as 1/√S. Myers, Pyle, Gilbert & Campbell [MPGC10, MPGCCL12] capture this in the **brassiness potential**

```
B = (1 / L_ecl) ∫₀^L (D₀ / D(x)) dx        (D₀ the minimum bore, L_ecl the equivalent cone length)
```

  Measured B: tenor sackbut 0.76, tenor and bass trombones 0.70, saxhorn basse 0.48, cornophones 0.36–0.43, Wagner tuba and
  Kaiserbaryton 0.37, C tuba 0.39, ophicleide 0.30 [MPGC10]. Values for trumpet and horn were not in what I read **[U]**.
  Narrow bores are brighter for the same output level. "Narrowing the bore by 25 % has an effect on brassiness potential
  equivalent to increasing B by 10 %" [MPGC10]. A high-B instrument brightens faster in a crescendo.
- Myers et al. 2012 [MPGCCL12] (abstract): B correlates with the spectral enrichment measured as spectral centroid.
- Vergez & Rodet [VR01]: the enrichment with level is much stronger at the bell than in the mouthpiece (trumpet C5). Their
  real-time model therefore used a **linear** reflection function for the loop and the **nonlinear** algorithm only on the way to the
  bell. They described this as "perceptively convincing and less demanding". Berjamin et al. 2017 [BLVC17] put the full
  nonlinearity in the loop (TVD scheme, 18 min CPU per second of sound). Nonlinear propagation **strongly affects the timbre**.
  It also affected the **attack**: transiently up to +157 cents in pitch, and a ~2× shorter attack. After 0.35 s the pitch difference was ≤ 3 cents.

### 6.2 Cheap options for pajupilli

| Option | How | Cost | Verdict |
|---|---|---|---|
| N0: none | — | 0 | The horn at p–mf is fine. ff trumpet/trombone sound "too pure" |
| **N1: out-of-loop time warp [recommended, G]** | At a cell near the end of the cylindrical part (or the bell throat), split the outgoing simple wave p⁺ = (p + Z_c U)/2. Delay each sample by τ_n = L_nl/c − L_nl·β p⁺_n/(ρc³) (a pressure-dependent time warp, Burgers without dispersion). Enforce monotonic arrival times (a running max; overtaken samples merge, which forms the shock, cf. Vergez's "physically based constraint" [VR01]). Add (warped − unwarped) p⁺, differentiated (the bell passes high frequencies almost fully), to the radiated output. L_nl ≈ B·L_ecl from the bore (§6.1) | ~1 fractional-delay read + a few ops per **sample**, not per cell. Warp span at p = 10 kPa, L = 1.5 m: ≈ 0.36 ms (12 samples at 32 kHz) [D] | Consistent with [HGMW96, VR01]. No stability risk. Misses the (small) effect on the attack |
| N2: in-loop, per cell | Make the local wave speed c(1 + βp/(ρc²)) in the leapfrog (bu per sample) | +1–2 FMA per cell, and a lower Courant number (u/c ≈ 7 % at 10 kPa → ≤ 0.9) plus artificial viscosity/flux limiting, because the leapfrog rings at shocks | Not worth it before N1 is tried |
| N3: post-filter by level | Brighten the output with a level-dependent shelving filter | ~0 | Fake. No waveform steepening, no phase coherence |

The upsampling question: steepening creates harmonics above the model's Nyquist frequency (12–16 kHz for horn/trombone rates). Run N1
**after** the resampler to 48 kHz. Pass p⁺ (band-limited) to the 48 kHz domain and warp there.

---

## 7. The player

### 7.1 Mouth pressure

| Quantity | Value | Source |
|---|---|---|
| Threshold pressure | p₀ = K·f, **K ≈ 4–8 Pa/Hz** (trumpet; the same for cornet and piccolo trumpet in one player) | [FT99] |
| Trumpet maximum | most players 7–15 kPa; one player **25 kPa** on high notes (above systolic blood pressure, ~18 kPa) | [FT99] |
| Trumpet (Bouhuys, quoted in [FT99]) | 4 kPa (pp) to 13 kPa (ff) for high notes | [FT99] |
| Trumpet high-note saturation | output rises only ~3 dB per doubling of pressure | [FT99] |
| Trombone B♭2, normal playing | ΔP across the lips ≈ **1.3 kPa** | [BSW20] |
| Trumpet model example | 5 kPa | [HBPW15] |
| Model sweeps | 0–20 kPa | [BLVC17] |
| Efficiency (Bouhuys) | 0.01–0.03 % low notes, up to 1 % high ff | [FT99] |
| Horn | not found in a primary source. **[G]**: threshold from K f (F3 175 Hz → 0.7–1.4 kPa; F5 698 Hz → 2.8–5.6 kPa), ff 2–4× threshold, ceiling ~12–15 kPa | — |
| Tuba | **[G]**: 0.3–3 kPa in the low register | — |

Mapping for pajupilli **[G]**: p_m = p₀(f)·(1.2 … 3.5) from pp to ff, where p₀ = 6 Pa/Hz × f. Clamp at the instrument's ceiling
(trumpet 20 kPa, horn 12 kPa, trombone 10 kPa, tuba 5 kPa). These numbers belong in the instrument table, like `ReedParams::press/pressHi`.

### 7.2 The embouchure as the control

The player's continuous controls, in order of importance:
1. **f_l (lip tension)**: selects the partial and bends the pitch within it (+½ / −1 semitone [BSW20]).
2. **p_m (breath)**: dynamics. Brightness rises with level because the lips close more abruptly and through §6.
3. **H, w (aperture)**: tighter or looser, dark or bright. Fletcher's x^{3/2} area law [FT99] couples them.
4. **Vocal tract**: matters mostly in the high trumpet register and for pitch bends. Ignore it at first. The upstream impedance is
   several times smaller than the bore's near B♭2 on trombone, but reaches 40 % of the series impedance when lipping far [BSW20].

### 7.3 Articulation, slurs, trills, glissandi

- **Tonguing:** the tongue seals behind the teeth/lips while the mouth pressure builds, then releases. The model is the same as
  `Reed`'s tongue: a series flow restriction in the mouth with its own time constant (t ≈ 2–5 ms soft "d", < 1 ms hard "t" **[G]**).
  Brass attacks are faster and more "spat" than flute attacks. The lips start from rest (h ≈ H), so the first cycles
  depend on the pressure rise time. Berjamin et al. [BLVC17] found the nonlinear propagation halves the attack time.
- **Lip slur:** change f_l with the valves/slide fixed. The note jumps when the old regime loses stability. There is
  **hysteresis**, from coexisting stable regimes [BLVC17, MGV22]. Slurring down needs a lower f_l than slurring up. The model gets this for free.
- **Lip trill:** alternate f_l between two adjacent partials at ~6–10 Hz **[G]**. Only possible where partials are a
  tone apart: horn written above ~G5 (partials 8–12), trumpet partials 6–10.
- **Glissando / horn "rip":** sweep f_l fast over several partials. The instrument steps through its resonances
  (a "lip glissando" can also be smooth over a large range for good players [BSW20]). Trombone glissando: slide motion (§4.2).
  Half-valve glissando: partial valve (§4.1).
- **Vibrato:** jaw/lip vibrato modulates f_l (and so the pitch, within the slot). Trombone slide vibrato modulates the length.
  Horn players mostly use little or none **[G]**. Typical 5–6 Hz **[G]**.

### 7.4 How players choose the partial (and what a keyboard needs)

Each written note on a valved instrument is reachable by a (valve combination, partial) pair. Players pick the one that
is in tune and stable. They then "hear" the pitch and set f_l (embouchure) for that partial. The bore pulls the
frequency into the nearest stable regime. **For the MIDI front end:** a per-instrument table, key →
(valves, partial n, f_l,n, p_m range), like the flute's fingering table. On the horn, partials 8–16 lie a semitone to a
whole tone apart, so an f_l error of a few % selects the neighbour: a **cracked note**. A real hornist cracks
occasionally. The calibration must find the centre of each "slot". Expose an "accuracy" control (the f_l error, per note, from
`PJ_WANDER`) rather than making cracks impossible.

---

## 8. Existing real-time and near-real-time brass models

| Model | Bore | Lip | What it simplifies | Ref |
|---|---|---|---|---|
| **Cook, TBone / HosePlayer → STK `Brass`** | one delay line (length = 2 × period + 3 samples), reflection −0.85, slide = delay length | **biquad resonator** at f_l (pole radius 0.997, gain 0.03) driven by p_m − p_bore; **squared** (a position→area map), clipped at 1; output mixes mouth pressure and bore pressure by that "area"; DC blocker | No mouthpiece, no bell, no losses beyond the scalar; "lip tension" CC scales f_l by 4^{±1} | [STK, C91] |
| **Vergez & Rodet (IRCAM), real-time trumpet** | measured reflection function (linear) in the loop, nonlinear propagation to the bell | 1-mass, tanh-smoothed closure; Poiseuille flow correction at small openings | 1-D lip; MIDI wind controller and a sensor-equipped "meta-trumpet" (valve FSRs, cup pressure, mouthpiece force → lip tension) | [VR01] |
| D'haes & Rodet 2002 | (as above) | inversion: estimate the controls from recordings | — | [DR02] |
| **Harrison, Bilbao, Perry, Wishart (NESS)** | FDTD Webster, viscothermal (Bilbao–Chick), valves as 3-tube junctions with partial opening, Silva/LS radiation network | 1-mass outward valve with sweep flow (Adachi & Sato form) | Plane waves, no nonlinear propagation; near-real-time (trumpet faster than real time; 4.5 m horn ~3.6× slower on a 2014 laptop, AVX, 44.1 kHz) | [HBPW15, DAFx-15, BC13] |
| Berjamin, Lombard, Vergez, Cottanceau 2017 | Menguy–Gilbert weakly nonlinear, TVD, diffusive losses | 1-mass, Newmark | Not real-time (18 min/s) | [BLVC17] |
| Msallam, Dequidt, Caussé, Tassart 2000 | trombone with nonlinear propagation for loud tones | — | — (not read) | [MDCT00] |
| **Audio Modeling SWAM-B** (Trumpets, Horns & Tubas, Trombones) | the vendor says "built only on Physical Modeling … based on Digital Waveguides Synthesis, as developed by Prof. Julius O. Smith" | not documented | Half-valve transitions, growl, flutter, breath noise; needs continuous breath control. No technical paper found **[U]** | [AM] |
| Sample Modeling "The Trumpet" (older), Yamaha VL1 (1994) | — | — | Commercial, undocumented. Treat as reference sounds only **[U]** | — |
| Mignot, Hélie, Matignon | waveguides from the Webster–Lokshin model (flare, curvature, fractional losses) | — | Axisymmetric, bends straightened | [MHM10] |

What they all keep: an outward-ish lip oscillator whose resonance is a **control**, Bernoulli flow, and a bore with a
bell. The cheap ones (STK) drop the mouthpiece and bell entirely and still "sound brass". The difference pajupilli can make is
the measured bore, the partial structure, the hand and valve transients, and playing in the room.

---

## 9. Recommendation

### 9.1 The minimal model that sounds like a horn

1. **Bore:** a single F horn (3.75 m) with **three valves** (loops 2, 1, 3 semitones) and the **hand** in the bell. The
   bore comes from a published profile; Bilbao & Chick's natural horn [BC13] is ready-made for validation. Run at **32 kHz**
   with `kCells` = 512 (≈ 460 cells used). Mouthpiece: cup volume in cell 0, throat as `setSeries`, backbore resolved. Bell:
   the plane-wave profile first, with spherical-cap correction as a switch (`PJ_CAPBELL`).
2. **Bell radiation:** Silva / Bilbao–Chick 2-state network at the end cell (§5.1). The radiated output is dU/dt.
3. **Lip:** a new `Lip : Tube` cloned from `Reed`, with the sign flipped (outward). Parameters μ = 5 kg/m², Q_l = 5, w = 10 mm,
   H = 0.074/f_l, S_r = 1.5·10⁻⁵ m², soft closure (tanh, ε ≈ 0.02 mm), flow U = w·h₊·√(2|Δp|/ρ)·sgn + S_r ḣ.
   Lips, cup cell and flow are solved together each sample, as the reed is. f_l is slewed (≈ 5–20 ms) on note changes.
4. **Player:** p_m = K f (1.2 … 3.5), K = 6 Pa/Hz, with the existing lungs/glottis/tongue chain. Tongue attack as in the reed. Hand
   position, f_l bend (joystick: ±1 semitone of f_l, which yields ~+½/−1 semitone of pitch) and valve speed as controls.
5. **Calibration:** per note, search (valves fixed by the table) f_l ∈ [0.8, 1.05]·f_target for (a) the right partial,
   (b) minimum threshold, (c) pitch within ±5 cents at mf. Then set the pitch with f_l **within the slot** (the player's ±½ semitone), as the
   flute calibration does with lip coverage. Store f_l and the slot edges.
6. **Not in v1:** nonlinear propagation (add N1, §6.2, in v2: horns need it least), two-mode lips, vocal tract, mutes,
   the double horn's thumb valve (v2: same junction code, ~512 cells at 26 kHz).

For the **trumpet** (v2) the same code runs at 48 kHz (≈ 300 cells with loops). It needs N1 for ff and the trumpet table. The
**trombone** needs the stretched slide (§4.2) plus N1 (B = 0.70, the brassiest). **Tuba/euphonium**: the low rates are fine. Check
the euphonium's ghost note [MGV22]. **Cornett/serpent**: lip + the existing hole machinery. **Alphorn/natural horn**:
lip + bore only, the best first smoke test (no valves).

### 9.2 Starting values (one table)

| Parameter | Horn (F) | Trumpet (B♭) | Trombone | Tuba (B♭B♭/C) | Source / status |
|---|---|---|---|---|---|
| Path length, open (m) | 3.75 | 1.38–1.48 | 2.74 | 4.9–5.5 | [PL10, BC13], tuba [G] |
| fs (kHz), cells | 32, ~460 | 48, ~300 | 32, ~360 | 22, ~420 | [D], §3.4 |
| Mouth-end bore (mm) | ~11.5–12 | 11.1–11.7 | ~13.3–13.9 | ~18–20 | [MPGC10], others [G] |
| Bell Ø (mm) | 300–310 | ~123 | ~180–220 | 370–500 | [PL10], others [G] |
| Cup volume (cm³) / throat Ø (mm) | 1.5 / 4.4 | 1.0 / 3.7 | 3.5 / 6.8 | 10 / 8 | [G], §2.1 |
| Popping frequency target (Hz) | 650–750 | ~800 | 500–600 | 300–400 | trumpet [RH12]; rest [G] |
| μ (kg/m²) | 5 | 4 | 7 | 9 | [VVG17]=9.1, [HBPW15]≈3.7; per-instrument [G] |
| Q_l | 5 | 5 | 5–7 | 5 | [VVG17]=7, humans lower [NCG08] |
| w (mm) | 10 | 10 | 12 | 14 | [HBPW15]=10, [VVG17]=12 |
| H (mm) | 0.074/f_l | 0.074/f_l | 0.074/f_l | 0.074/f_l | [MGV22] after [EB82] |
| K = p₀/f (Pa/Hz) | 6 | 6 | 6 | 6 | [FT99] 4–8 |
| p_m ceiling (kPa) | 12 | 20 | 10 | 5 | [FT99] trumpet; rest [G] |
| Brassiness L_nl | small | ≈ 0.6–0.7 L **[G]** | 0.70 L_ecl | 0.39 L_ecl | [MPGC10] |

### 9.3 What to validate

1. **Passive input impedance**, `-imp`-style (impulse response at the cup, FFT), against published curves:
   - **Bilbao & Chick 2013 Table I** [BC13]: the first 7 maxima/minima of a Smith-Watkins B♭ trumpet (1.38 m), Conn 8H trombone
     (2.74 m), and a natural horn in D (4.51 m, PHC 22 mouthpiece), with **measured frequencies and their FDTD agreement (≲ 1 %)**.
     Their bore profiles (Fig. 7) can be digitised. Correct for temperature: they measured at 19–20 °C (c ≈ 343 m/s), so scale
     frequencies by 349.5/343 or run at their c.
     **Targets:** peaks within 1 % (17 cents) and heights within 1 dB, as their FDTD achieved. Better: ±5 cents on peaks 2–8.
   - **Dell et al. 2010** [DJDW10]: Yamaha 668 double horn with and without a hand, for the hand model.
   - Mattéoli et al. [MGV22] Fig. 2: impedance of a bass trombone (Courtois AC502).
   - UNSW's online brass impedance database (Wolfe's group) for horn, trumpet and trombone curves. Use the curves as given on
     the site; the accompanying text was secondary for me.
2. **The tool chain:** extend `tools/tmm.py` (the transfer-matrix reference) with conical/Bessel segments, the mouthpiece
   lumps and the radiation network, the same way it checks the flute. The waveguide must match TMM within a few cents, and
   TMM must match the measured peaks (validating geometry and losses separately).
3. **Oscillation:** thresholds vs f_l (the U-shaped curves of [MGV22], including the trombone pedal about a semitone below
   the linear prediction). Played frequency slightly **above** the bore peak in the low register [BFSW13, BSW20]. Lip-up/down range ≈
   +½ / −1 semitone [BSW20]. Threshold ∝ f with K ≈ 4–8 Pa/Hz [FT99].
4. **Sound:** spectrum slopes above 800 Hz: horn −21 / −13 / −9 dB/oct for p / f / ff [PL10]. Trumpet: rise to ~1 kHz, then falling
   [FT99]. Spectral centroid vs mouthpiece rms pressure for the brassiness stage [MPGC10].
5. **Valves:** combination sharpness (1+3, 1+2+3 sharp by roughly 15–30 cents on trumpet **[G]**). Half-valve gives inharmonic,
   unstable tone [HBPW15]. No clicks when pressing.

### 9.4 Main risks

1. **Cell budget.** Horns, trombones and tubas exceed 256 cells at useful rates (§3.4). `kCells` must grow to 512 (memory ~
   doubles, and the vector loops already run over the used vectors only). The alternative, running the horn at 17–24 kHz, risks the mouthpiece
   resolution and the ff top end.
2. **Mouthpiece sub-cell accuracy.** The cup and throat are smaller than dx. Peak heights 3–8 and the playing "feel" depend on them [BC13].
   Mitigation: separate inertance/compliance integration (§2.2) and validation against TMM.
3. **Partial selection by f_l is knife-edge in the horn's high register.** The calibration must find slot centres. The 1-DOF lip
   plays sharp, or fails to play, in the upper register, where real lips move transversely [BFSW13]. Fallback: the two-mode lip (§1.4).
4. **Junction stability and energy.** Three-way nodes and moving valve ports are new to the code. Keep the trapezoidal/implicit form
   and test passivity with the scatter test (`-scatter`), as for the room coupling.
5. **The bell end.** The plane-wave bell plus an unflanged lumped load gives peaks slightly low and too strong above cut-off
   (horn, [BC13]). The spherical-cap correction must be validated, not assumed.
6. **Brassiness.** Without §6 a ff trumpet/trombone sounds like a loud flugelhorn. With N1 the attack transient effect [BLVC17] is lost.
   This is acceptable **[G]**.
7. **Unverified numbers:** horn/trombone/tuba mouthpiece data, horn mouth pressures, the trumpet/horn B values, Adachi & Sato's
   parameters and Berjamin's lip table (garbled in my copy), and Harrison's σ = 5. Check each in the primary text before
   building on it.

---

## References

[AS95] S. Adachi, M. Sato, ATR Technical Report TR-H-0177 (1995), preprint of [AS96]. https://results.atr.jp/35th/techreport/TR-H-0177/TR-H-0177.pdf (scanned; parameters not extracted)
[AS96] S. Adachi, M. Sato, "Trumpet sound simulation using a two-dimensional lip vibration model", JASA 99(2), 1200–1209 (1996). (not read; via citations)
[AM] Audio Modeling, "Exploring SWAM-B technology and its transformative power" (vendor blog). https://audiomodeling.com/blog/exploring-swam-b-technology-and-its-transformative-power **[U]**
[B11] S. Bilbao, "Modelling of brass instrument valves", Proc. DAFx-11, Paris (2011). https://dafx.de/paper-archive/details/L_6rPtwCFNIgElNK7o_sNg (not read)
[BC13] S. Bilbao, J. Chick, "Finite difference time domain simulation for the brass instrument bore", JASA 134(5), 3860–3871 (2013) (volume/pages from memory, unverified). Preprint: https://www.ness.music.ed.ac.uk/wp-content/uploads/2013/08/bilbao_jasa_13_r2_double.pdf (read: instruments, Table I, conclusions)
[BFSW13] H. Boutin, N. Fletcher, J. Smith, J. Wolfe, "Lip motion, the playing frequency of the trombone and the upstream and downstream impedances", SMAC 2013, Stockholm. https://newt.phys.unsw.edu.au/jw/reprints/SMACtrombone.pdf
[BJ74] A. H. Benade, E. V. Jansson, "On plane and spherical waves in horns with nonuniform flare", Acustica 31, 79–98 (1974). (not read; via [BC13])
[BLVC17] H. Berjamin, B. Lombard, C. Vergez, E. Cottanceau, "Time-domain numerical modeling of brass instruments including nonlinear wave propagation, viscothermal losses, and lips vibration", Acta Acustica u. A. 103(1) (2017), doi:10.3813/AAA.919038 **[U]**. arXiv:1511.04247 https://arxiv.org/pdf/1511.04247
[BSW20] H. Boutin, J. Smith, J. Wolfe, "Trombone lip mechanics with inertive and compliant loads ('lipping up and down')", JASA 147(6), 4133–4144 (2020), doi:10.1121/10.0001466. https://newt.phys.unsw.edu.au/jw/reprints/lipping.pdf
[C91] P. R. Cook, "TBone: an interactive waveguide brass instrument synthesis workbench for the NeXT machine", Proc. ICMC 1991. (not read)
[CGC00] J. S. Cullen, J. Gilbert, D. M. Campbell, "Brass instruments: linear stability analysis and experiments with an artificial mouth", Acta Acustica u. A. 86, 704–724 (2000). https://hal.archives-ouvertes.fr/hal-00474993 (abstract); J. S. Cullen, PhD thesis, Univ. of Edinburgh (2000).
[DAFx-15] R. L. Harrison, S. Bilbao, J. Perry, "An algorithm for a valved brass instrument synthesis environment using finite-difference time-domain methods with performance optimisation", Proc. DAFx-15, Trondheim (2015). https://www.ntnu.edu/documents/1001201110/1266017954/DAFx-15_submission_55.pdf/053b889b-1a15-4232-823b-13e3b3b4490f
[DJDW10] N. Dell, R. James, J. Davidson, J. Wolfe, "The effect of hand and mute on the impedance spectra of the horn", Proc. ISMA 2010, Sydney. https://www.phys.unsw.edu.au/jw/reprints/DellISMA2010.pdf
[DR02] W. D'haes, X. Rodet, "Physical constraints for the control of a physical model of a trumpet", Proc. DAFx-02, Hamburg (2002). https://www.dafx.de/paper-archive/2002/DAFX02_DHaes_Rodet_trumpet_model.pdf
[EB82] S. J. Elliott, J. M. Bowsher, "Regeneration in brass wind instruments", J. Sound Vib. 83(2), 181–217 (1982). (not read; via [MGV22, FT99])
[EPGKC14] P. Eveno, J.-P. Petiot, J. Gilbert, B. Kieffer, R. Caussé, "The relationship between bore resonance frequencies and playing frequencies in trumpets", Acta Acustica u. A. 100(2) (2014). (abstract only; pages unverified)
[F93] N. H. Fletcher, "Autonomous vibration of simple pressure-controlled valves in gas flows", JASA 93(4), 2172–2180 (1993). (via [BFSW13])
[FT99] N. H. Fletcher, A. Tarnopolsky, "Blowing pressure, power, and spectrum in trumpet playing", JASA 105(2), 874–881 (1999). https://www.phys.unsw.edu.au/music/people/publications/Fletcheretal1999.pdf
[HBPW15] R. L. Harrison, S. Bilbao, J. Perry, T. Wishart, "An environment for physical modeling of articulated brass instruments", Computer Music Journal 39 (2015/16; issue given as 4 or 5 by different sources), 80–95. https://www.pure.ed.ac.uk/ws/files/21585566/Harrison_etal_CMJ_2015_An_Articulated_Brass_Instrument_Synthesis.pdf
[HGMW96] A. Hirschberg, J. Gilbert, R. Msallam, A. P. J. Wijnands, "Shock waves in trombones", JASA 99(3), 1754–1758 (1996), doi:10.1121/1.414698. (abstract and secondary descriptions only)
[M16] T. R. Moore, "The acoustics of brass musical instruments", Acoustics Today 12(4), 30–37 (Winter 2016). https://acousticstoday.org/wp-content/uploads/2021/07/Brass.pdf
[MDCT00] R. Msallam, S. Dequidt, R. Caussé, S. Tassart, "Physical model of the trombone including nonlinear effects. Application to the sound synthesis of loud tones", Acta Acustica u. A. 86, 725–736 (2000). (not read)
[MGV22] R. Mattéoli, J. Gilbert, C. Vergez, J.-P. Dalmont, S. Maugeais, S. Terrien, F. Ablitzer, "Minimal blowing pressure allowing periodic oscillations in a model of bass brass instruments", Acta Acustica (2022, venue/volume unverified); arXiv:2112.08751. https://arxiv.org/pdf/2112.08751
[MHM10] R. Mignot, T. Hélie, D. Matignon, "Digital waveguide modeling for wind instruments: building a state-space representation based on the Webster–Lokshin model", IEEE Trans. Audio Speech Lang. Proc. 18(4), 843–854 (2010). (abstract/intro via https://core.ac.uk/download/12042576.pdf)
[MPGC10] A. Myers, R. W. Pyle, J. Gilbert, D. M. Campbell, "The influence of bore size on brassiness potential", Proc. Second Vienna Talk (2010). https://viennatalk.mdw.ac.at/papers/Pap_01_85_Myers.pdf
[MPGCCL12] A. Myers, R. W. Pyle, J. Gilbert, D. M. Campbell, J. P. Chick, S. Logie, "Effects of nonlinear sound propagation on the characteristic timbres of brass instruments", JASA 131(1), 678–688 (2012). (abstract only)
[NCG08] M. J. Newton, D. M. Campbell, J. Gilbert, "Mechanical response measurements of real and artificial brass players lips", JASA 123(1), 14–20 (2008). https://hal.archives-ouvertes.fr/hal-00474652 (abstract only)
[PL10] J. Pätynen, T. Lokki, "Directivities of symphony orchestra instruments", Acta Acustica u. A. 96, 138–167 (2010). https://users.aalto.fi/~ktlokki/Publs/patynen_aaua_2010.pdf
[RH12] N. Rose, D. Holloway, "Finite element modeling of brass musical instruments", Proc. Acoustics 2012, Fremantle. https://logosfoundation.org/instrum_gwr/bug/p60.pdf
[STK] P. R. Cook, G. P. Scavone, STK `Brass.cpp` / `Brass.h`. https://github.com/thestk/stk/blob/master/src/Brass.cpp
[UNSW] J. Wolfe et al., "Brass instrument (lip reed) acoustics: an introduction", UNSW Music Acoustics. https://newt.phys.unsw.edu.au/jw/brassacoustics.html
[VR01] C. Vergez, X. Rodet, "Trumpet and trumpet player: model and simulation in a musical context", Proc. ICMC 2001. https://dept-info.labri.fr/~mrobine/TMJV/4.Synthese_Realiste/biblio/ICMC01VergezRodet.pdf ; see also C. Vergez, X. Rodet, "New algorithm for nonlinear propagation of a sound wave. Application to a physical model of a trumpet", J. Signal Processing 4 (2000), https://hal.archives-ouvertes.fr/hal-01105576
[VVG17] L. Velut, C. Vergez, J. Gilbert, M. Djahanbani, "How well can linear stability analysis predict the behaviour of an outward-striking valve brass instrument model?", Acta Acustica u. A. 103(1), 132–148 (2017). https://hal.archives-ouvertes.fr/hal-01245846 (parameters via [MGV22] Table 1)
[Y95] S. Yoshikawa, "Acoustical behavior of brass player's lips", JASA 97(3), 1929–1939 (1995), doi:10.1121/1.412066. (abstract only)

Not obtained: Elliott & Bowsher 1982 and Adachi & Sato 1996 full texts (the lip parameter tables); Cullen's thesis; Msallam et al. 2000;
Backus 1976 (horn impedance with hand); D. M. Campbell, J. Gilbert, A. Myers, *The Science of Brass Instruments* (Springer/ASA Press, 2021),
the obvious single reference for most of §2–§7 and worth buying; Hirschberg et al. 1996 full text (its mouthpiece dp/dt values).
