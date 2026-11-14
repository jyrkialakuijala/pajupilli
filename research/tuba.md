# The orchestral tuba on the pajupilli waveguide — research notes

Status: 2026-11-14. SI units. c = 349.5 m/s, ρ = 1.18 kg/m³ unless stated. This file extends `research/horns.md`
(lip valve, mouthpiece lumping, valve junctions, bell load, brassiness, player). It does not repeat that material.
Tags as there: **[G]** my estimate or guess; **[D]** derived by me from published data or formulas (including my
transfer-matrix fits with `tools/brass.py`); **[U]** taken from a secondary source or abstract and not checked in the
primary text. References are at the end.

Short answer:
- **Model a 4-valve CC tuba** (5th valve optional). It covers GM "Tuba" E1..F4 with standard fingerings: E1 is 2-3-4 on
  partial 2, and F4 is valve 1 on partial 12 (§1, §4).
- **No full tuba bore profile is published online.** I fitted the bore below with the repo's transfer-matrix code. It
  reproduces the measured resonance pattern of a real contrabass tuba (Besson BE994 B♭B♭, Mattéoli et al. 2022 [MGT22])
  transposed to C: rms 4 cents on peaks 2–12. It is anchored at the published end points: throat, shank, valve bore and
  bell (§2).
- **The tuba's first resonance is not far below the nominal fundamental**, unlike the trombone's. It sits ~2 semitones
  *above* the pedal. A one-mass lip model plays a "ghost note" just above it, a minor third to a fourth above the pedal
  [MGT22] (§3).
- **Low-register intonation is the hard part.** With valves the 3rd partial comes out 50–110 cents sharp in my fit.
  This is either physics (the reason tubas have 4–6 valves and compensating systems) or an artefact of the fit.
  Validate it before trusting it (§3.4, §8).

---

## 1. Which tuba

| Type | Open length | Nominal (pedal) | Where it is the orchestral standard | Reaches E1 (41.2 Hz) | F4 (349 Hz) is partial |
|---|---|---|---|---|---|
| B♭B♭ contrabass | "18 ft" ≈ 5.5 m [W] | B♭0 29.1 Hz | Germany, Austria, Russia (rotary valves) [W] | 4 valves, partial 2 | 12 (open) |
| **CC contrabass** | "16 ft" ≈ 4.9 m [W]; my fit 5.05 m [D] | C1 32.7 Hz | "most widely used orchestral tuba outside Germany and Russia"; professional models have 5 non-compensating valves [W]. Pätynen & Lokki's orchestral recordings used a C tuba [PL10] | 2-3-4, partial 2 | 12 (valve 1) |
| E♭ bass | "13 ft" ≈ 4.0 m [W] | E♭1 38.9 Hz | UK (1960s on, often compensating) [W] | needs the pedal register or a compensating 4th valve | 10 (valve 2)… |
| F bass | "12 ft" ≈ 3.7 m [W] | F1 43.7 Hz | much of continental Europe [W] | only as a valved pedal tone (F1 is the open pedal) | 8 (open) |

**Choice: CC, 4 valves (1, 2, 3, 4 = 2, 1, 3, 5 semitones), non-compensating, an optional 5th valve.** Reasons:
1. The GM range E1..F4 sits on partials 2–12 of a C instrument. E1 = C2 − 8 semitones is the bottom of the normal
   valved register (partial 2 with 2-3-4), so it needs no pedal tones. F4 is still under the bell cut-off (§6.2), and
   partials 9–12 are a whole tone apart, not a semitone as on the horn. An F tuba would have to play E1–B1 as pedal or
   "false" tones, and its upper register would need partials up to 16.
2. C is the most common orchestral tuba [W] and the one whose directivity and spectrum were measured [PL10].
3. It is ~0.45 m shorter than B♭B♭, so fewer cells (§7.1), with the same physics. Everything here transposes to B♭B♭ by
   scaling lengths by 2^(2/12) = 1.122 [D]. The measured data I used come from a B♭B♭ [MGT22].

---

## 2. The bore

### 2.1 Published anchor values

| Quantity | Value | Source |
|---|---|---|
| Mouthpiece cup Ø (rim inner) | 32–34 mm [W]; Perantucci PT-50 33 mm, PT-83 32–32.5 mm; Schilke 69C4 32.76 mm; Denis Wick 1–5: 30.0–32.5 mm | retailers [MPX, DW] **[U]** |
| Mouthpiece throat ("bore") | Schilke 69C4 / 62 / 66 / 67: .3281" = **8.33 mm**; Schilke Helleberg: .295–.323" (7.5–8.2 mm); Denis Wick 7.6–8.8 mm; Perantucci PT-83 8.6 mm; Jacobs Heritage 8.5 mm | [MPX, DW] **[U]** |
| Shank tip (American standard) | OD .520–.530" (13.2–13.5 mm); Jarno-like taper 1:20 on diameter | tubaforum [TF1] **[U]** |
| Mouthpipe (leadpipe) start | "under .550"" (14 mm) bore on "fake euro" receivers | [TF1] **[U]** |
| Valve-section bore, CC | Yamaha YCB-822: **19.5 mm (.768")**, bell **500 mm**; Miraphone 1291 CC: conical through the valves **19.6 → 21.2 mm**, bell **470 mm**, 4 piston + 1 rotary valve | Yamaha spec page [Y822]; retailers [MIR] **[U]** |
| Valve bore, B♭B♭ | Adams: 19–20.5 mm, bell 450 mm; Miraphone 495: 19.6 mm; Yamaha YBB-201: .728" (18.5 mm), bell 443 mm | retailer/maker pages [ADM, Y201] **[U]** |
| Shape | "almost entirely conical" over > 5 m [PL10] citing [FR98] | [PL10] |
| Cup volume | not published for tuba mouthpieces that I found. Bowl of Ø 32.5 mm, depth ~22 mm, about paraboloidal: π·r²·d/2 ≈ **9 cm³** [D]. Myers' thesis ranks 435 measured mouthpieces by cup volume [My98], but the scan's OCR did not give the tuba values | [D] |

Euphonium (B♭, 9 ft) for comparison: I digitised the bore from Mattéoli et al. [MGT22, Fig. 8], which took it from Campbell,
Gilbert & Myers [CGM21]. It is mm from the mouthpipe end, ±0.3 mm [D]:
(100, 12.6) (300, 14.1) (500, 16.0) (700, 18.2) (900, 20.9) (1000, 22.5) (1200, 25.7) (1400, 29.6) (1600, 35.4) (1800, 44.6)
(2000, 57.0) (2100, 64.0). There is **no visible cylindrical part**: the curve rises steadily from the leadpipe, and the
valve section is not distinguishable at this resolution. The same figure gives a B♭ baritone saxhorn, cylindrical 13.0 mm
to 650 mm and then conical, and a bass trombone, 12.0 → 13.0 mm to 1800 mm and then the bell.

### 2.2 A CC tuba bore you can type in [D, fitted; shape partly G]

Method: the profile is parametrised as throat, backbore, conical leadpipe, cylindrical valve section, a two-segment
cone and a Bessel bell, with Hermite blends (±120 mm) at the three kinks, as in `kHornBore`. I fitted 7 parameters
(Nelder–Mead on `tools/brass.py`'s transfer matrices, cup 9 cm³ lumped) so that impedance peaks 1–12 match the cents
pattern **measured on a Besson BE994 B♭B♭** [MGT22, Fig. 3a; my digitisation, §3.1], transposed to C1 = 32.703 Hz. The
result has an rms error of **4.3 cents** on peaks 2–12. The end points (8.3 mm throat, 13 mm at the shank, 19.5 mm valves,
470 mm bell) were fixed.

Knots (x mm from the lips, d mm), with linear segments, then smoothed as described:

| Part | x (mm) | d (mm) | Status |
|---|---|---|---|
| Lips / cup (volume 9 cm³ lumped at x = 0) | 0 | 8.3 | cup [D]; throat Ø [U, Schilke 8.33] |
| Throat end | 14 | 8.3 | length [G] |
| Backbore end = shank tip | 90 | 13.0 | [G]/[U] |
| Leadpipe (conical) → valve cylinder | 655 | 19.5 | position fitted [D], Ø [U Yamaha] |
| Valve section, cylinder (valves 2, 1, 3, 4 at 800 / 900 / 1000 / 1100 [G]) | 655–1331 | 19.5 | fitted [D] |
| Long cone, part 1 (bows and branches) | 1331 → 3544 | 19.5 → 54.8 | fitted [D] |
| Long cone, part 2 (bell branch) | 3544 → 4260 | 54.8 → 127.8 | fitted [D] |
| Bessel bell, ε = 0.40 (at the fit's lower bound) | 4260 → 5053 | 127.8 → 470 | fitted [D]; mouth Ø [U Miraphone] |

The same profile as `BorePoint`s (blends and bell expanded, 61 points) [D]:

```
{0.0, 8.30}, {14.0, 8.30}, {90.0, 13.00}, {534.7, 18.12}, {558.7, 18.38}, {582.7, 18.62}, {606.7, 18.82},
{630.7, 19.00}, {654.7, 19.15}, {678.7, 19.28}, {702.7, 19.38}, {726.7, 19.44}, {750.7, 19.49}, {774.7, 19.50},
{1211.0, 19.50}, {1235.0, 19.52}, {1259.0, 19.58}, {1283.0, 19.67}, {1307.0, 19.81}, {1331.0, 19.98},
{1355.0, 20.19}, {1379.0, 20.44}, {1403.0, 20.73}, {1427.0, 21.05}, {1451.0, 21.41}, {3423.6, 52.89},
{3447.6, 53.37}, {3471.6, 54.07}, {3495.6, 54.96}, {3519.6, 56.07}, {3543.6, 57.38}, {3567.6, 58.90},
{3591.6, 60.62}, {3615.6, 62.55}, {3639.6, 64.69}, {3663.6, 67.03}, {4259.9, 127.79}, {4364.6, 134.91},
{4456.0, 142.44}, {4535.8, 150.38}, {4605.4, 158.77}, {4666.3, 167.62}, {4719.4, 176.97}, {4765.8, 186.84},
{4806.3, 197.25}, {4841.7, 208.25}, {4872.5, 219.87}, {4899.5, 232.13}, {4923.0, 245.07}, {4943.6, 258.74},
{4961.5, 273.17}, {4977.2, 288.40}, {4990.9, 304.48}, {5002.8, 321.46}, {5013.3, 339.39}, {5022.4, 358.31},
{5030.3, 378.29}, {5037.3, 399.39}, {5043.3, 421.66}, {5048.6, 445.18}, {5053.2, 470.00}
```

- **Total open length 5.05 m [D]**, against the nominal "16 ft" = 4.88 m [W]. The equivalent cone length c/(2·C1) is
  5.34 m at c = 349.5 [D].
- **Cylindrical: ~0.44–0.52 m of 5.05 m, i.e. 9–10 % open [D]**, all in the valve block. Each pressed valve adds pure
  cylinder (§2.3), so 1-2-3-4 makes the air column ~42 % cylindrical [D]. Real CC tubas may have none: Miraphone's valve
  section is itself conical, 19.6 → 21.2 mm [MIR].
- **Brassiness potential** of this bore, B = (1/L_ecl)∫D₀/D dx with D₀ = 13 mm at the shank and L_ecl = 5.24–5.34 m:
  **B ≈ 0.36 [D]**. Myers et al. report 0.36–0.52 for the tuba family [MGPC07 **U**]. Note: the **"C tuba 0.39"** in
  [MPGC10] (and in horns.md §6.1) is a **"Tuba in 8-ft C (Couesnon)"**, the French tenor C tuba an octave above CC, not
  a contrabass [MPGC10, its table]. I found no published B for a CC contrabass.
- **The fit is not unique.** Seven parameters fitted to 12 frequencies, with ε hitting its bound and the cone split
  where the optimiser chose, cannot identify the geometry. Moving the valve block or straightening the cone moves peaks
  1–4 by 50–150 cents (§3.4). Treat the shape between the valves and the bell as **[G] constrained by [D]**.
- Spherical-cap bell correction (horns.md §3.5): the bell above x ≈ 4.9 m flares at > 20° [D]. Apply it if the TMM–FDTD
  comparison shows the upper peaks sitting low.

### 2.3 Valves [D fitted; positions G]

`ValveDef {x, len, d, semis}`. The loops are fitted with the transfer-matrix model so that the mean cents of partials
2–10 (valve 4: 2–8) sit on the valve's interval. Bore 19.5 mm. Positions in the valve cylinder [G]:

| Valve | Interval | x (mm) | Loop (mm) [D] | Ideal ΔL = L_eff(2^(s/12)−1), L_eff = 5.05 m [D] |
|---|---|---|---|---|
| 2 | 1 semitone | 800 | **301** | 301 |
| 1 | 2 semitones | 900 | **639** | 619 |
| 3 | 3 semitones | 1000 | **999** | 956 |
| 4 | 5 semitones (4th) | 1100 | **1805** | 1691 |
| (5, optional) | "flat whole step" (~2¼ semitones) on many CC tubas [EV]; Wikipedia: usually 2½, sometimes a minor third on C tubas [W] | 1200 | ≈ 700–760 [D, by proportion] | — |

```
const ValveDef kTubaValves[] = {{900.0, 638.6, 19.5, 2}, {800.0, 300.8, 19.5, 1},
                                {1000.0, 999.1, 19.5, 3}, {1100.0, 1804.6, 19.5, 5}};
```

- **Non-compensating** [W]. With loops sized for the open tube, combinations are sharp in pure length terms [D]: 1+3 =
  470 c instead of 500 (−30); 1-2-3 = 571 instead of 600; 1-4 = 652 instead of 700 (+48 sharp); 1-2-4 = 721 instead of
  700; 2-3-4 = 796 instead of 900 nominal, which is exactly E1 (800). This is why the 4th replaces 1-3, 2-4 replaces
  1-2-3, and why E1 is fingered 2-3-4 [EV].
- **Compensating systems** (Blaikley 1878) route the 4th valve's tubing back through valves 1–3. They are used on
  British E♭/B♭ tubas, not on CC tubas [W]. They are not needed for a keyboard model: tune the loops instead.
- **Valve block position:** front-action CC tubas have the leadpipe going into the valves after ~0.5–1 m, then the main
  tuning slide [G]. Acoustically only the position in the cylinder matters, and that only weakly (§3.4).
- Cell cost of the loops: 0.30 + 0.64 + 1.00 + 1.80 = **3.74 m [D]**, on top of the 5.05 m open bore (§7.1).

---

## 3. Input impedance, the pedal and the ghost note

### 3.1 Measured: a B♭B♭ contrabass tuba (Besson BE994, mouthpiece Perantucci PT-50)

Mattéoli et al. 2022 [MGT22] measured it with the Macaluso–Dalmont impedance head on the mouthpiece and fitted 13 modes.
I digitised the peaks of their Fig. 3a (the vector plot rendered at 800 dpi; the frequency axis was read from the grid
lines; ±0.4 Hz, i.e. ±5–20 cents) **[D]**:

| Peak n | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| f (Hz) | 33.0 | 58.7 | 86.1 | 114.3 | 146.2 | 175.3 | 202.6 | 233.3 | 263.6 | 289.3 | 321.6 | 353.1 | 385.1 | 412.4 |
| cents vs n·B♭0 (29.135) | **+216** | +13 | −26 | −34 | +6 | +5 | −11 | +2 | +9 | −12 | +6 | +17 | +29 | +19 |
| \|Z\| (10⁶ Pa·s/m³) | 33 | 28 | 23 | 24 | 23 | 22 | 18 | 17 | 14 | 13 | 5.5 | 4.5 | 3.4 | 3.2 |

- **Peaks 2–12 lie within ±35 cents of the harmonic series.** The 7th is *not* flat (−11 c), but partial 7 is musically flat
  (−31 c against ET) and is avoided [D].
- **Peak 1 is 2.2 semitones above the pedal note** (B♭0), at ≈ C1. On a trombone the first peak is ~7 semitones *below* the
  pedal (38 vs 58 Hz, horns.md §3.2). The euphonium's first peak is ≈ 61 Hz, ~+80 cents above its B♭1 pedal [MGT22, Fig. 8;
  my reading]. [MGV22] note that the first resonance "can vary up to 8 semitones between a trombone and euphonium". This is
  the difference between a mostly expanding bore and a mostly cylindrical one.
- **Peak heights more than halve between n = 10 and 11** (13 → 5.5·10⁶, −7.5 dB), i.e. above ≈ 300 Hz. That is the
  bell cut-off for this B♭B♭ [D]. For CC scale by 1.12: **≈ 340 Hz [D]**, which matches Pätynen & Lokki's ≈ 400 Hz power
  maximum for a C tuba [PL10]. F4 (349 Hz, partial 12) therefore sits right at the cut-off. Expect a thin, less "slotted"
  top note, as tubists report **[G]**.
- My CC fit (§2.2) gives, open, in Hz: 37.0, 65.9, 96.6, 128.4, 164.5, 196.7, 227.3, 261.2, 295.1, 326.9, 360.7, 395.5
  (cents vs n·C1: +216, +13, −27, −33, +10, +4, −13, −3, +5, −1, +5, +14) [D]. Peak levels are even up to 500 Hz in the TMM
  (152 → 137 dB re 1 Pa·s/m³): the R‖L radiation load passes too little at high ka. The waveguide's wide-band load
  (horns.md §5.1) should reproduce the measured fall above ~300 Hz. Validate against the table above.

### 3.2 The pedal note and the ghost note (Mattéoli et al. 2021, 2022)

Lip model: one-mass outward valve, Q_l = 7, 1/μ = 0.11 m²/kg, w = 12 mm, H = k/f_l with k = 0.074 m·Hz, the same for all
seven tubas [MGT22, Table 2]. B♭B♭ tuba, from bifurcation analysis [MGT22 §3.1, Fig. 6–7; numbers from the text, the rest my reading of the figure]:

| Regime | Note (player) | Optimal f_l (Hz) | Oscillation f at threshold (Hz) | Min. threshold p_m (kPa) |
|---|---|---|---|---|
| Pedal (isolated branch, not found by linear stability) | B♭0 29.1 | **28.67** (text) | ≈ 33.5 [D, fig.] | ≈ 0.35 [D, fig.] |
| **Ghost note** (Hopf branch from the first resonance) | D1–D♯1 (recordings: 7–8 semitones under B♭1) | ≈ 37.5 [D] | ≈ 39.4 [D]; linear stability: first U at 37–39 Hz (text) | **≈ 0.20** [D] — the *easiest* note |
| 2nd regime | B♭1 58.3 | ≈ 59 [D]; f_l = 51.35 for Fig. 6b (text) | 60.3–62.6 at its emergence (text); ≈ 65 at the optimum [D] | ≈ 0.39 [D] |

- The **ghost note** "can be played by any tuba player" but is "not commonly known among tuba players". It lies a **minor
  third to a perfect fourth above the pedal**, depending on the tuba [MGV22, MGT22]. On the B♭B♭ it is D1 (a major third);
  on the E♭ tuba a fourth (citing Herbert et al.'s "factitious notes"); on the euphonium the model gives 75.6/60.6 Hz = 5/4,
  and the player a "high major third" [MGV22].
- The model's pedal is ~2 semitones sharp of B♭0, and its 2nd regime sharp of the peak. Players play the pedal in tune:
  the measured pedal–R2 interval is ≈ 12 semitones, while the model gives 10.6 (CC) to ~13 (E♭) semitones [MGT22 §4.2].
- **CC tuba:** in the model the pedal note's U-curve is "very narrow" with an optimal threshold **> 3 kPa**: "very slotted
  … but also hard to obtain" [MGT22, App. A].
- With Q_l above ≈ 21.7 the pedal branch joins the main branch [MGT22].

**For pajupilli [G]:** the pedal register (C1 and below) is outside GM "Tuba". Do not calibrate for it. But **expect the
ghost note to appear** when f_l is set for E1–G1 on long valve combinations (§3.4): their partial-2 resonance is weak and
the first resonance sits only ~a fourth below. The calibration (horns.md §9.1 item 5) must check that the oscillation
locks to partial 2, not to the ghost regime. This is the tuba's version of a "cracked" note.

### 3.3 Which partials are usable (CC)

| n | Open note | f (ET) | Use |
|---|---|---|---|
| 1 | (C1 pedal) | 32.7 | pedal register only; first peak at ~D1 (§3.1) |
| 2 | C2 | 65.4 | E1–C2 with valves: the bottom of GM range |
| 3 | G2 | 98.0 | C♯2–G2 |
| 4 | C3 | 130.8 | A♭2–C3 |
| 5 | E3 | 164.8 | C♯3–E3 |
| 6 | G3 | 196.0 | F3–G3 |
| 7 | B♭3 −31 c | 228.9 | avoided; A♭3–C4 are taken on partial 8 [EV] |
| 8 | C4 | 261.6 | A♭3–C4 |
| 9 | D4 | 294.3 | C♯4–D4 |
| 10 | E4 | 327.0 | E♭4–E4 |
| 11 | ≈ F♯4 −49 c | 359.7 | unusable (between F4 and F♯4) |
| 12 | G4 | 392.4 | F4 (valve 1), F♯4 (2), G4 (0) |

### 3.4 Valved resonances in the fitted bore [D] — a warning

TMM of §2.2 with the fitted loops. Cents of peak n against the valve combination's nominal series:

| Valves | n=2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|
| open | +13 | −27 | −33 | +10 | +4 | −13 | −3 |
| 1 | −15 | **+66** | −40 | −35 | +25 | +5 | −13 |
| 1+3 | +79 | **+108** | −3 | +7 | +5 | +51 | +35 |
| 4 | +51 | **+53** | −30 | −36 | −40 | +8 | −5 |

Valves leave partials ≥ 5 within ±40 c. **Partial 3 goes 50–110 c sharp, and partial 2 sharp on long combinations.**
Two other bore shapes that fit the open horn less well show the same (+35 to +109 c). It may be partly real: the low
register of a valved tuba is notoriously sharp, which is why there are 5th/6th valves and compensating systems [W]. But
I have **no measured valved tuba impedance** to confirm the size. Until then, treat it as a model risk: the calibration
will have to lip these notes down by up to a semitone. Real players can lip down about a whole tone (horns.md §1.1, [BSW20]).

---

## 4. Fingering chart, CC tuba (E1–F4)

Fingerings follow Everett's CC chart (University of Mississippi), "most commonly used, in order of preference" [EV]. It
assumes a flat-whole-step 5th valve; the 4-valve choice is listed first here. "TMM" is the fitted bore's peak (§2.2/2.3),
in cents against equal temperament (A4 = 440 Hz) [D]. 'x' notation for `NoteInfo`: valves "1234".

| Note | f (Hz) | Valves (4-valve) | Partial | TMM cents | Alternates [EV] |
|---|---|---|---|---|---|
| E1 | 41.2 | 2-3-4 | 2 | +123 | — |
| F1 | 43.7 | 1-2-4 | 2 | +90 | 4-5; 1-4 |
| F♯1 | 46.2 | 2-4 | 2 | +101 | 2-3-5; 1-2-3 |
| G1 | 49.0 | 4 | 2 | +51 | 1-2-5; 1-3 (+79) |
| A♭1 | 51.9 | 2-3 | 2 | +39 | |
| A1 | 55.0 | 1-2 | 2 | +12 | 3 (−1) |
| B♭1 | 58.3 | 1 | 2 | −15 | |
| B1 | 61.7 | 2 | 2 | −10 | |
| C2 | 65.4 | 0 | 2 | +13 | 4-5; 1-2-4; 1-4 (partial 3) |
| C♯2 | 69.3 | 2-4 | 3 | +58 | 1-2-3 (+112); 2-3-5 |
| D2 | 73.4 | 4 | 3 | +55 | 1-3 (+110) |
| E♭2 | 77.8 | 2-3 | 3 | +115 | |
| E2 | 82.4 | 1-2 | 3 | +108 | 3 (+94) |
| F2 | 87.3 | 1 | 3 | +68 | |
| F♯2 | 92.5 | 2 | 3 | +25 | 2-4; 1-2-3 (partial 4) |
| G2 | 98.0 | 0 | 3 | −25 | 4; 1-3 (partial 4) |
| A♭2 | 103.8 | 2-3 | 4 | −37 | |
| A2 | 110.0 | 1-2 | 4 | −39 | 3 |
| B♭2 | 116.5 | 1 | 4 | −40 | |
| B2 | 123.5 | 2 | 4 | −26 | |
| C3 | 130.8 | 0 | 4 | −33 | 2-3 (partial 5) |
| C♯3 | 138.6 | 1-2 | 5 | −34 | 2-4; 1-2-3 (partial 6) |
| D3 | 146.8 | 1 | 5 | −49 | 4; 1-3 (partial 6) |
| E♭3 | 155.6 | 2 | 5 | −28 | 2-3 (partial 6) |
| E3 | 164.8 | 0 | 5 | −3 | 1-2 (partial 6) |
| F3 | 174.6 | 1 | 6 | +27 | |
| F♯3 | 185.0 | 2 | 6 | +21 | |
| G3 | 196.0 | 0 | 6 | +6 | |
| A♭3 | 207.7 | 2-3 | 8 | +22 | |
| A3 | 220.0 | 1-2 | 8 | +13 | |
| B♭3 | 233.1 | 1 | 8 | −13 | |
| B3 | 246.9 | 2 | 8 | −12 | |
| C4 | 261.6 | 0 | 8 | −3 | 1 (partial 9) |
| C♯4 | 277.2 | 2 | 9 | +8 | 1-2; 1-2-3 (partial 10) |
| D4 | 293.7 | 0 | 9 | +9 | 1; 1-3 (partial 10) |
| E♭4 | 311.1 | 2 | 10 | −2 | 2-3 |
| E4 | 329.6 | 0 | 10 | −14 | 1-2 (partial 12) |
| F4 | 349.2 | 1 | 12 | +8 | |

- **The 4th valve for intonation** [W, EV]: **4** instead of 1-3 (D2, G1, and D3/G2 as alternates); **2-4** instead of 1-2-3
  (C♯2, F♯1, C♯3); **1-2-4** or **4-5** for F1, where 1-4 is ~50 c sharp [D]. On a 3-valve tuba these notes are 1-3 / 1-2-3
  and must be lipped down.
- Below E1: E♭1 1-3-4 (or 3-4-5), D1 2-3-4-5 / 1-3-4-5 / 1-2-3-4, C♯1 1-2-3-4-5 or a **false tone** (1-2-3-4, or 2-3)
  [EV]. C1 is the open pedal. On 4-valve CC tubas some of these exist only as "privileged" (false) tones [UNT **U**]. All
  are outside GM range.
- The TMM cents are the bore's resonances, not played pitches. The lip plays a little above the peak in the low register
  (horns.md §1.1) and the calibration absorbs the rest. The partial 3–4 errors (§3.4) are the ones to watch.

---

## 5. Lips, mouthpiece and the player

### 5.1 Lip parameters

| Source | f_l | Q_l | μ (kg/m²) | w | H | Notes |
|---|---|---|---|---|---|---|
| Mattéoli et al. 2021/22, **all seven tubas** (same as their trombone) | control, 20–130 Hz in the figures | 7.0 | 9.1 (1/μ = 0.11) | 12 mm | 0.074/f_l (2.5 mm at 30 Hz, 1.5 mm at 50 Hz) | [MGV22, MGT22] |
| Same, B♭B♭: optimal f_l | pedal 28.67 Hz; R2 ≈ 59 Hz (plays ≈ 65 at threshold) | | | | | [MGT22] |

Tuba-specific lip measurements: **none found**. All published high-speed lip studies are on trombone (horns.md §1.1).
**Recommended start [G]:** μ = 12 kg/m² (more lip mass in a 33 mm cup), Q_l = 5, **w = 16 mm** (about half the cup
diameter), H = 0.074/f_l clamped to ≤ 3 mm, S_r = w × 2 mm. Inward second mode as on the horn (1.3 × f_l, Q 3, ×0.3).
f_l ≈ 0.85–0.95 × note (horns.md §1.5). Keep the "heavier below 190 Hz" logic of `src/horn.cc`: the whole tuba range is below it.

### 5.2 Mouthpiece

- Cup 9 cm³ [D] lumped in cell 0. Throat Ø 8.3 mm [U], length ~14 mm [G]. Backbore 8.3 → 13 mm over ~75 mm [G]; resolve it
  (4–5 cells at dx ≈ 16–18 mm).
- **Popping frequency [D]:** f = (c/2π)·√(S_t/(V·L_eff)). With S_t = 54 mm², V = 9 cm³ and L_eff = 40–80 mm (throat plus part
  of the backbore), **f ≈ 480–680 Hz**. horns.md §2.1 guessed 300–400 Hz; this estimate puts it higher. Either way it lies
  above the bell cut-off (~340 Hz), so on a tuba it shapes the radiated "formant" (§6.1) more than it shapes the playable peaks.

### 5.3 Mouth pressure and flow

| Quantity | Value | Source |
|---|---|---|
| Model threshold, B♭B♭ low notes | ghost 0.20, pedal 0.35, B♭1 0.39 kPa | [MGT22, Fig. 7; D] |
| Same as K = p₀/f | 0.39 kPa / 65 Hz = **6 Pa/Hz**, the same as Fletcher & Tarnopolsky's trumpet K = 4–8 Pa/Hz | [D, FT99] |
| CC pedal (model) | > 3 kPa (narrow U) | [MGT22] |
| Bouhuys 1964, all instruments | mouth pressure 2.5–158 mmHg (0.33–21 kPa), rising with pitch and loudness; power p·U < 0.1 to 17 W | [Bo64 abstract **U**] |
| Tuba high notes | "highest oral pressure … the tuba … high notes … 60–70 mmHg" (8–9.3 kPa) | secondary review of Bouhuys **[U]** |
| Bouhuys flow range (all instruments) | < 0.05 to > 1.6 L/s | secondary **[U]** |
| Larger bores | "require less intra-oral compression and produce more airflow" than the higher brass | Kruger et al. 2006 [KMK06] (qualitative) |
| Flow estimate | U ≈ w·h̄·√(2p/ρ): w = 16 mm, h̄ = 0.75 mm (f_l = 50 Hz, half of H), p = 1 kPa → 0.5 L/s while open, **≈ 0.25–0.5 L/s** mean; ff low notes ~1 L/s. A 5–6 L breath lasts 5–20 s | [D] |

**Mapping [G]:** p_m = 6 Pa/Hz × f × (1.2 … 4) from pp to ff, floor 0.25 kPa, ceiling 8 kPa (the low notes reach ff at
1–2 kPa; only above ~C4 does the pressure climb towards 5–8 kPa). This replaces the 5 kPa tuba ceiling of horns.md §7.1.

---

## 6. Timbre and radiation

### 6.1 Spectrum

| Quantity | Value | Source |
|---|---|---|
| Instrument measured | a C tuba, tones A1–A3 (55–220 Hz), anechoic (approximate below 100 Hz) | [PL10] |
| Power maximum ("suggesting the cut-off") | ≈ **400 Hz** | [PL10] |
| Slope above 500 Hz | **−18 dB/oct (p), −12 (f), −10 (ff)** | [PL10] |
| Brassiness | B ≈ 0.36 for the bore of §2.2 [D]; tuba family 0.36–0.52 [MGPC07 **U**]. Lower than trombone (0.70) or saxhorn basse (0.48) [MPGC10] | |
| Shock formation | negligible except ff above ~C3. (∂p/∂t)max = 2πfP: 100 Hz at 5 kPa gives 3.1·10⁶ Pa/s, so x_s = ρc³/(β·dp/dt) ≈ 13 m against an ~1 m effective cylinder [D] | horns.md §6.1 |

[D]: the tuba's 12–18 dB/oct roll-off sets in above ~500 Hz, against ~800 Hz for the horn. The fundamental and partials up
to ~400 Hz carry the timbre. A model band-limited to 6–8 kHz is inaudibly short of the real spectrum at p–f. The
nonlinear stage N1 (horns.md §6.2) matters only for ff high notes and can wait.

### 6.2 The bell and its radiation

- Mouth radius a = 0.235 m (470 mm). **ka = 1 at 237 Hz [D]**, i.e. inside the playing range (B♭3). The R‖L
  low-ka load is wrong over most of the range, so use the wide-band network (horns.md §5.1). The first non-planar mode cuts on at
  3.832c/(2πa) ≈ **900 Hz** at the mouth [D], above the cut-off, so ignore it.
- Directivity [PL10]: below 250 Hz "somewhat omnidirectional", with up to 6 dB less behind the bell. Above that "very
  strongly concentrated on the axis of the bell". At 1 kHz the main lobe spans ~90° (Meyer). Meyer: within −10 dB omni up
  to 400 Hz. The pattern does not change with dynamics or note.
- **Upward bell [G]:** in the room code, point the bell up (slightly forward/right). The audience hears the bell's high band
  only via the ceiling reflection. In a dry mix use the off-axis, low-passed signal: a first-order shelf, corner ≈ 240 Hz,
  −10 to −15 dB above ~1 kHz. Like the horn pointing backwards, this is a large part of why a tuba sounds "round" in a
  hall.
- **Low-frequency end [D]:** E1 = 41 Hz radiates weakly. A monopole's radiated power ∝ ω²U², and at ka = 0.17 the bell
  reflects almost everything. The partials 2–5 (82–205 Hz) carry the loudness of low notes. Do not expect, or add, a
  strong fundamental.

---

## 7. Playing practice for a keyboard model

### 7.1 Cells and rate [D]

Path length with all four loops: 5.05 + 0.30 + 0.64 + 1.00 + 1.80 = **8.79 m**. With dx = c/(0.975 fs):

| fs | dx (mm) | Cells (open + all loops) | Band |
|---|---|---|---|
| 16 kHz | 22.4 | 392 | 8 kHz: enough for p–f [D, §6.1] |
| 20 kHz | 17.9 | 491 | 10 kHz |
| 22.05 kHz | 16.3 | 541 | 11 kHz |

20 kHz fits 512 cells. 16 kHz is defensible for the tuba alone, but the backbore (75 mm) then spans only 3 cells
(horns.md §2.2 sub-cell care applies).

### 7.2 Attack, legato, vibrato

- **Attack.** The round trip is 2L/c = 29 ms open and 50 ms on 1-2-3-4 [D]. Low notes need several periods (24 ms at E1) to
  settle, so a tuba "speaks" in ~80–200 ms at low pitches, against ~30–60 ms for a horn mid-register note **[G]**.
  Players compensate by tonguing slightly ahead of the beat **[G]**. The model gets most of this from the long bore. Tongue
  release ~3–8 ms (softer than trumpet), pressure rise ~30–60 ms [G]. Let note-on start the breath a few ms early only if
  latency allows; otherwise accept the physics.
- **The ghost-note trap:** a slow pressure rise on E1–G1 may lock onto the ghost regime first (§3.2). A faster initial
  pressure overshoot (+30 % for ~50 ms) or f_l starting slightly high favours partial 2 [G].
- **Legato.** Valve slurs: valves move 10–30 ms (horns.md §4.1). Tubas use large piston or rotary valves, so lean towards
  the slow end [G]. Lip slurs within one fingering show the same hysteresis as on the horn. Slur by moving f_l with the
  valves fixed when the table allows. The partials are a whole tone or more apart up to F4, so slurs are less "knife-edge"
  than on the horn [D].
- **Vibrato.** Orchestral tuba: little or none. Solo/lyrical: slight jaw/lip vibrato ~4.5–5.5 Hz, ±10–20 cents, delayed onset
  **[G]**. Default the mod wheel to 0 depth for "Tuba".
- **Dynamics → brightness:** −18 → −10 dB/oct from p to ff [PL10] comes mostly from lip closure (the lip model) at this
  low B. Check it against [PL10] before adding N1.

---

## 8. Validation and open risks

1. **Bore uniqueness (largest risk).** The §2.2 profile is fitted to the open resonances of one B♭B♭ tuba. Valved
   resonances (§3.4) are untested. Obtain a measured CC or B♭B♭ bore: Myers 2019 ("The typology and timbre of the tuba",
   *Vom Serpent zur Tuba*, bore profiles and impedances of several tubas) [My19]; Campbell, Gilbert & Myers 2021, Fig. 7.34
   and the bore appendix [CGM21]; or a measured bore from a reflectometry service. Then refit only the loop lengths.
2. **Validate the waveguide against the TMM** (`tools/brass.py` with this profile and a 9 cm³ cup): peaks within a few cents
   on n = 2–12. Validate the **cut-off fall** (peaks 11–14 at ~0.3–0.4 of peak 10's |Z|) against §3.1.
3. **Ghost note:** with the [MGT22] lip set (Q 7, μ 9.1, w 12 mm), the B♭B♭-scaled bore should give a ghost note ≈ 7–8 semitones
   under partial 2 at ~half its threshold pressure. A good qualitative test of the low-register lip–bore coupling.
4. **Unverified:** all mouthpiece numbers (retailer specs), the CC valve bore and bell (Yamaha/retailers), Bouhuys' tuba
   pressures, the tuba-family B range, and every [G] in §5 and §7.

---

## References

[ADM] Adams Musical Instruments, B♭ tubas (bell 450 mm, bore 19–20.5 mm). https://www.adams-music.com/en/brass/tubas/bb_tubas **[U]**
[Bo64] A. Bouhuys, "Lung volumes and breathing patterns in wind-instrument players", J. Appl. Physiol. 19, 967–975 (1964). Abstract via https://www.flutopedia.com/refs_bpress.htm **[U]**
[BSW20] see horns.md.
[CGM21] M. Campbell, J. Gilbert, A. Myers, *The Science of Brass Instruments*, Springer/ASA Press (2021). https://www.springer.com/book/9783030556846 (not read; via [MGT22])
[DW] Mouthpiece Express, "Denis Wick tuba mouthpiece specs". https://mouthpieceexpress.com/blogs/specs/denis-wick-tuba-mouthpiece-specs **[U]**
[EV] M. Everett, "Chromatic fingering chart for CC tuba (flat whole-step fifth valve)", University of Mississippi. https://olemiss.edu/lowbrass/studio/fingeringcharts/cctubafingering.pdf
[FR98] N. H. Fletcher, T. D. Rossing, *The Physics of Musical Instruments*, 2nd ed., Springer (1998). (via [PL10])
[FT99] N. H. Fletcher, A. Tarnopolsky, JASA 105(2), 874–881 (1999); see horns.md.
[KMK06] J. Kruger, J. McClean, M. Kruger, "A comparative study of air support in the trumpet, horn, trombone and tuba", Proc. ICMPC9, Bologna (2006), 535–536. https://windsongpress.com/jacobs/written/Kruger%20McClean%20Kruger%20A%20Comparitive%20Study%20of%20Air%20Support%20in%20the%20Trumpet,%20Horn,%20Trombone%20and%20Tuba.pdf
[MGPC07] A. Myers, J. Gilbert, R. W. Pyle, D. M. Campbell, "Non-linear propagation characteristics in the evolution of brass musical instrument design", Proc. ICA 2007, Madrid. https://hal.archives-ouvertes.fr/hal-00475592 (B range for the tuba family from a search snippet **[U]**; the site blocked access)
[MGT22] R. Mattéoli, J. Gilbert, S. Terrien, J.-P. Dalmont, C. Vergez, S. Maugeais, E. Brasseur, "Diversity of ghost notes in tubas, euphoniums and saxhorns", Acta Acustica 6, 32 (2022), doi:10.1051/aacus/2022026. arXiv:2207.05395 https://arxiv.org/pdf/2207.05395 (read in full; Fig. 3a, 7, 8 digitised by me)
[MGV22] R. Mattéoli et al., "Minimal blowing pressure allowing periodic oscillations in a model of bass brass instruments", Acta Acustica 5, 57 (2021). arXiv:2112.08751 https://arxiv.org/pdf/2112.08751 (euphonium sections read; = [MGV22] in horns.md)
[MIR] Miraphone 1291 CC retail listings (bore 19.6–21.2 mm conical, bell 470 mm, 4 piston + 1 rotary valve), e.g. https://musicelements.com.sg/products/miraphone-model-1291-cc-tuba ; maker: https://www.miraphone.de/cc-tuba-11-2.html **[U]**
[MPGC10] A. Myers, R. W. Pyle, J. Gilbert, D. M. Campbell, "The influence of bore size on brassiness potential", Proc. Second Vienna Talk (2010). https://viennatalk.mdw.ac.at/papers/Pap_01_85_Myers.pdf (its table: "Tuba in 8-ft C (Couesnon) 0.39")
[MPX] Mouthpiece Express, "Schilke tuba mouthpiece specs". https://mouthpieceexpress.com/blogs/specs/schilke-tuba-mouthpiece-specs ; Perantucci PT-50/PT-83 retailer pages (e.g. https://www.thomann.co.uk/perantucci_tuba_mouthpiece_pt_83_g_591149.htm) **[U]**
[My98] A. Myers, *Characterization and taxonomy of historic brass musical instruments from an acoustical standpoint*, PhD thesis, Univ. of Edinburgh (1998). https://era.ed.ac.uk/items/e701af45-3836-4e8d-ba54-324e7eb1222d (searched; tuba data not extracted)
[My19] A. Myers, "The typology and timbre of the tuba", in *Vom Serpent zur Tuba*, Michaelsteiner Konferenzberichte 83, Wissner (2019), 159–174. https://www.research.ed.ac.uk/en/publications/the-typology-and-timbre-of-the-tuba/ (not obtained)
[PL10] J. Pätynen, T. Lokki, "Directivities of symphony orchestra instruments", Acta Acustica u. A. 96, 138–167 (2010), §5.4 Tuba. https://users.aalto.fi/~ktlokki/Publs/patynen_aaua_2010.pdf
[TF1] TubeNet/tubaforum thread "Mouthpiece shank tapers". https://mail.tubaforum.net/viewtopic.php?p=27797 **[U]**
[UNT] University of North Texas tuba studio, "Low range fingerings CC". https://music.unt.edu/tuba/files/lowrangefingeringscc.pdf (not obtained; quoted via search **[U]**)
[W] Wikipedia, "Tuba" (lengths, valves, regional choice, compensating system). https://en.wikipedia.org/wiki/Tuba **[U]**
[Y201] Yamaha YBB-201 specs. https://usa.yamaha.com/products/musical_instruments/winds/tubas/ybb-201/specs.html **[U]**
[Y822] Yamaha YCB-822 specs (C tuba, bore 19.5 mm / .768", bell 500 mm). https://ca.yamaha.com/fr/products/musical_instruments/winds/tubas/ycb-822/specs.html

Not obtained: a measured CC or B♭B♭ bore profile (Myers 2019; Campbell et al. 2021, Fig. 7.34); measured valved tuba
impedances; tuba lip mechanics; Bouhuys' per-instrument tables.
