# The B♭ trumpet on the pajupilli brass model — research notes

Status: 2026-11-14. This file extends `research/horns.md`. That file already covers the lip valve (§1), the mouthpiece in
general (§2), the plane-wave bell (§3), valve junctions (§4.1), radiation (§5), nonlinear propagation (§6) and the
player (§7). Here I give only what is specific to the trumpet: a **measured bore you can type in**, valve loops,
impedance targets, the fingering table, and the trumpet's lip, pressure, timbre and playing data.
SI units unless marked. Tags: **[G]** guess, **[D]** derived by me from published data or formulas, **[U]** taken
from a secondary source or a snippet, not checked in the primary text. Untagged numbers are quoted from the cited
primary source.

**Three results:**
1. **Bore.** Bilbao & Chick 2013 [BC13] plot their measured Smith-Watkins B♭ trumpet (Fig. 7) as a vector graphic.
   I extracted the polyline from the PDF (§1.1). A transfer-matrix model of this bore reproduces their **measured**
   impedance peaks 1–7 to within −0.6 … +0.6 % (§3.1), so the digitisation is good enough to use as the profile.
2. **Valve loops**, fitted on that bore [D]: **2nd 88 mm, 1st 180 mm, 3rd 277 mm** added length. These match
   HyperPhysics' 86 / 179 / 278 mm [HP]. The fit gives the combinations' sharpness: 1+3 is +27 c and 1+2+3 is +48 c,
   which the 3rd slide fixes when pulled 30 and 56 mm (§2).
3. **Impedance targets**: the measured maxima and minima of [BC13] (§3). The derived peak magnitudes are
   70–170 MPa·s/m³, and the bell cut-off is ≈ 1 kHz (peaks fade above ~1.2 kHz).

---

## 1. The bore

### 1.1 Source and extraction

[BC13] measured a **Smith-Watkins B♭ trumpet with a Kelly "Screamer" mouthpiece**. The Screamer is "a specialist high
register combination with a very small mouthpiece cup volume". Its open length is "approximately 1.38 m". The bell
was measured with graded rod gauges, the narrow parts with internal dial calipers, and the rest from the outside diameter
minus a nominal wall thickness: "accurate to within ±5 %". Bends were measured along the mid-line and treated as
straight. Their Fig. 7 plots the radius (log scale) against the axial coordinate.

**Extraction [D].** I converted page 7 of the preprint to SVG (`pdftocairo -svg`) and took the 56-vertex trumpet polyline.
I calibrated it on the axis ticks. x: 0 and 1200 mm tick marks; y: the log-decade minor ticks 3…9, 10, 20…60 mm,
which give a decade height of 27.705 pt and fit the ticks to < 0.1 pt. The polyline is the plotted data. Line-width and
plotting decimation are the only losses. Validation is in §3.1.

### 1.2 The profile (x from the rim plane, diameters)

| x (mm) | d (mm) | Part | | x (mm) | d (mm) | Part |
|---|---|---|---|---|---|---|
| 0.0 | 17.34 | rim (inner) | | 285.2 | 11.71 | end of leadpipe taper |
| 1.9 | 15.25 | cup | | 318.1 | 11.75 | cylinder (tuning slide, valves) |
| 2.8 | 13.80 | cup | | 726.3 | 11.75 | |
| 3.8 | 12.31 | cup | | 741.1 | 11.79 | (step +0.04 mm) |
| 4.7 | 9.58 | cup | | 841.4 | 11.79 | **bell taper starts** |
| 5.7 | 7.38 | cup | | 941.3 | 12.89 | |
| 7.0 | 5.53 | cup → throat | | 981.1 | 13.80 | |
| 7.9 | 4.95 | | | 1031.1 | 14.87 | |
| 8.9 | 4.45 | | | 1041.2 | 15.30 | |
| 9.8 | 4.16 | throat entry | | 1081.4 | 16.49 | |
| 11.7 | 4.07 | throat | | 1131.3 | 18.29 | |
| 15.8 | 4.07 | throat end | | 1141.1 | 18.76 | |
| 52.2 | 5.79 | backbore | | 1181.3 | 20.51 | |
| 57.2 | 6.20 | | | 1242.3 | 24.52 | |
| 62.3 | 6.49 | | | 1288.2 | 29.94 | |
| 67.0 | 6.80 | | | 1303.7 | 33.09 | |
| 72.1 | 7.10 | | | 1317.3 | 36.57 | |
| 77.1 | 7.49 | | | 1327.4 | 40.42 | |
| 87.2 | 8.10 | shank end | | 1343.2 | 49.37 | |
| 87.2 | 9.69 | receiver gap | | 1348.2 | 54.58 | |
| 88.2 | 9.58 | | | 1354.2 | 60.29 | |
| 88.2 | 8.70 | leadpipe entry | | 1358.7 | 66.64 | |
| 91.0 | 8.70 | | | 1363.7 | 73.67 | |
| 96.1 | 8.80 | | | 1367.8 | 81.41 | |
| 98.0 | 8.70 | | | 1371.3 | 89.98 | |
| 160.3 | 10.08 | leadpipe | | 1375.7 | 99.44 | |
| 210.2 | 10.78 | | | 1378.3 | 109.90 | |
| 260.2 | 11.38 | | | **1381.4** | **126.77** | **bell mouth** |

All values **[D]** (digitised from [BC13] Fig. 7; the instrument data themselves carry ±5 %).

Section summary **[D]**:

| Section | From – to (mm) | Length | Diameters | Remarks |
|---|---|---|---|---|
| Cup | 0 – 9.8 | 9.8 mm | 17.3 → 4.2 | **V ≈ 0.92 cm³** (0–9.8 mm; 0.94 cm³ to 11.7 mm). Small cup, as [BC13] say |
| Throat | 9.8 – 15.8 | 6 mm | 4.07 | Bach "27" throat = 3.66 mm [MPE] (Screamer's is wider) |
| Backbore | 15.8 – 87.2 | 71 mm | 4.07 → 8.10 | 2.05 cm³ |
| Receiver gap | 87.2 – 88.2 | 1 mm | 9.7 | Step down to the leadpipe: the "gap" |
| Leadpipe | 88 – 285 | ≈ 197 mm | 8.70 → 11.71 | Conical, ~0.9° half-angle overall, steeper first |
| Cylinder | 285 – 841 | **556 mm** | 11.75–11.79 | Main tuning slide + valve block + first bell-tail bow. ML bore (Bach 37: 0.459" = 11.66 mm [BACH]) |
| Bell section | 841 – 1381 | **540 mm** | 11.8 → 126.8 | Taper, then flare; total open length **1381 mm** |

Total air volume (open) ≈ 442 cm³ **[D]**.

**Bessel fit of the bell [D]:** over 941–1381 mm, d(x) = 2·b·(x₀ − x)^(−ε) with **x₀ = 1391.5 mm, b = 269.8 (mm units),
ε = 0.61**. RMS error 3 % in log d. The fit runs ~6 % fat at 1290–1370 mm and 3 % thin at the rim. Use the points, not the fit.
For comparison: Bach Stradivarius 37: 4.8" (122 mm) bell, #25 leadpipe, 0.459" bore [BACH]. A "37-like" variant
would scale the last 40 mm to 122 mm **[G]**.

### 1.3 The same bore for `kTrumpetBore` (cup lumped, x from the throat entry)

`kHornBore` starts at the throat, with the cup in `LipParams::cup`. Below is the same data, shifted by −9.8 mm. The two
zero-length steps at the receiver are spread over 0.5 mm so that x is strictly increasing **[D]**. Cup: **0.92e-6 m³**.

```
// Smith-Watkins B♭ trumpet, Kelly Screamer mouthpiece (Bilbao & Chick 2013, Fig. 7,
// digitised; research/trumpet.md). x from the throat entry (rim + 9.8 mm).
const BorePoint kTrumpetBore[] = {
    {0.0, 4.16}, {1.9, 4.07}, {6.0, 4.07}, {42.4, 5.79}, {47.4, 6.20}, {52.5, 6.49}, {57.2, 6.80},
    {62.3, 7.10}, {67.3, 7.49}, {77.4, 8.10}, {77.9, 9.69}, {78.4, 9.58}, {78.9, 8.70}, {81.2, 8.70},
    {86.3, 8.80}, {88.2, 8.70}, {150.5, 10.08}, {200.4, 10.78}, {250.4, 11.38}, {275.4, 11.71},
    {308.3, 11.75}, {716.5, 11.75}, {731.3, 11.79}, {831.6, 11.79}, {931.5, 12.89}, {971.3, 13.80},
    {1021.3, 14.87}, {1031.4, 15.30}, {1071.6, 16.49}, {1121.5, 18.29}, {1131.3, 18.76}, {1171.5, 20.51},
    {1232.5, 24.52}, {1278.4, 29.94}, {1293.9, 33.09}, {1307.5, 36.57}, {1317.6, 40.42}, {1333.4, 49.37},
    {1338.4, 54.58}, {1344.4, 60.29}, {1348.9, 66.64}, {1353.9, 73.67}, {1358.0, 81.41}, {1361.5, 89.98},
    {1365.9, 99.44}, {1368.5, 109.90}, {1371.6, 126.77}
};
```

Grid **[D]**: at 48 kHz, dx = 349.5/(0.975·48000) = 7.47 mm. That gives 184 cells for the open bore plus ~73 for the
three loops ≈ **257 cells**. The cup and throat are sub-cell. The receiver gap (1–2 mm) is sub-cell, and the loader's
∫dx/S inertance handles it. The rim lies 9.8 mm before x = 0. If the lumped cup should include the throat taper, use
0.94e-6 and start at 11.7 mm.

### 1.4 Mouthpiece: the Screamer vs an ordinary one

| | Kelly Screamer (this bore) | Bach 3C / 1½C (typical "legit") | Source |
|---|---|---|---|
| Inner rim Ø | 17.3 mm (plot; the rim curvature blurs it) | 16.30 mm (3C) | [D]; [MPE] |
| Cup volume | 0.92 cm³ | ~1.0–1.2 cm³ **[G]** | [D]; horns.md §2.1 |
| Throat | 4.07 mm × ~6 mm | 3.66 mm (#27) | [D]; [MPE] |
| Backbore | 71 mm, 4.1 → 8.1 mm | — | [D] |
| Popping frequency | **≈ 815 Hz** (TMM of 0–87.2 mm, open shank) **[D]** | ≈ 800 Hz (1¼C, maker figure) | [D]; [RH12] |

The popping frequency, not the cup volume, sets the "formant" of the impedance envelope. It also sets FT99's envelope peak
(Q ≈ 5 at 500–1000 Hz [FT99]). Both mouthpieces sit near 800 Hz, so the Screamer bore is a fair default.

---

## 2. Valves

### 2.1 Order and position

- **Air path: leadpipe → main tuning slide → 3rd valve → 2nd → 1st → bell** [TIP, US1724632, U]. The main tuning slide
  "fits into the leadpipe at one end, and into the tube leading to the third valve at the other".
- Position on the measured bore **[G]**: the cylinder runs from 285 to 841 mm. The tuning-slide crook takes ~250–300 mm of it,
  and the bell tail's first bow ~100 mm. Suggested ports (rim coordinates): **3rd 620, 2nd 675, 1st 730 mm** (throat
  coordinates: 610 / 665 / 720). The +0.04 mm step at 726–741 mm may be the valve-block/bell-tail joint, which supports
  the 1st valve near 730 mm. In the TMM the fitted loop lengths change by < 0.2 mm when a port moves anywhere from 400 to 800 mm
  **[D]**, so the exact position does not matter much.

### 2.2 Loop lengths

| Valve | Interval | Fitted added length **[D]** (TMM on §1.2, partials 3–8 on their semitones) | HyperPhysics [HP] (L = 1.48 m) | NTBIS [NTBIS] (L ≈ 1.40 m) | Yamaha [YAM, U] |
|---|---|---|---|---|---|
| 2 | 1 semitone | **87.7 mm** | 86 mm | 84 mm | 70 mm |
| 1 | 2 semitones | **180.1 mm** | 179 mm | 173 mm | 160 mm |
| 3 | 3 semitones | **277.3 mm** | 278 mm | 267 mm | 270 mm |

Yamaha's figures (from a search snippet; the page refused my fetch) are probably physical slide-tube lengths, not added
acoustic length **[U]**. Use the fitted values in `ValveDef::len` (the added length, as for the horn). Loop bore =
the main bore, 11.75 mm **[G]** (some trumpets have a larger 3rd slide).

Per-partial error of a single valve after the fit (cents, partials 2…10) **[D]**:
- valve 2: −17, −1, −6, −2, +1, +3, +5, −1, −6
- valve 1: −36, −1, −11, −5, +2, +4, +10, +2, −10
- valve 3: −56, −1, −14, −8, +3, +6, +13, +7, −9

Partial 2 is flat by tens of cents with any valve, because the low resonances are not a harmonic series (§3.2). Players lip
it up.

**Design practice:** "most brass instruments are designed so that the tubing inserted by depressing either of the first
two valves is slightly longer than it should be", which puts the combinations closer to pitch [M16]. **[G]**: make
valves 1 and 2 about +1.5 % long (89 / 183 mm) to copy this.

### 2.3 Combinations and slides

Sharpness of the combinations with the fitted loops, mean over partials 3–8 **[D]**:

| Valves | Target | Mean error | Note |
|---|---|---|---|
| 1+2 | −3 st | **+10 c** | written A4, E4, A3 (alt: 3) |
| 2+3 | −4 st | **+14 c** | G#4, Eb4, G#3 |
| 1+3 | −5 st | **+27 c** | D4, G3: **3rd slide out ≈ 30 mm** (= +60 mm path) |
| 1+2+3 | −6 st | **+48 c** | C#4, F#3: **3rd slide out ≈ 56 mm** |

"Depressing the first and second valves together" needs 26.5 cm of added length [HP]; the loops give 26.8 cm, close.
- The **3rd-valve slide** has a ring for the ring finger and "capable of rapid movement". The **1st-valve slide** has a thumb
  saddle or trigger [M16]. Pedagogy says low **C#4 and D4 are "very sharp"** and low E4 is sharp. Use the 1st/3rd slides,
  "particularly for a5, d4, and c#4" [UTC, WEID, U].
- **Model [G]**: treat the 3rd slide as a per-note **loop-length offset**: +30 mm (1+3), +56 mm (1+2+3). Also give +10 mm on
  the 1st slide for 1+2 notes if the calibration wants it. The calibration can choose the offset. It is a fixed number per
  fingering (the player sets the slide before the note speaks), so it needs no moving geometry. The stretched-cell method of
  horns.md §4.2 also works, but a static offset per note is enough.
- **Main tuning slide**: the bore is measured at 20 °C. Playing air (c = 349.5 m/s in pajupilli) raises everything by
  **+31 c** **[D]**. A player pulls the main slide **≈ 13 mm** (26 mm of path, 1.8 % of 1.48 m) **[D]**. In the model,
  lengthen the cylinder at ~400 mm by 26 mm, or let the global tuning calibrate it.

---

## 3. Input impedance

### 3.1 Measured peaks: the validation target

[BC13] Table I, Smith-Watkins + Kelly Screamer, measured with BIAS in an anechoic room at 20 °C:

| n | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|
| Max (Hz) | 83.5 | 234.2 | 353.1 | 469.1 | 591.6 | 702.7 | 812.5 |
| Min after it (Hz) | 122.5 | 261.1 | 379.1 | 505.6 | 631.1 | 754.9 | 878.8 |
| FDTD of [BC13] | 83.0 | 233.0 | 351.5 | 469.7 | 588.3 | 699.6 | 809.8 |
| **TMM on the digitised bore [D]** | 83.0 | 233.3 | 352.5 | 472.0 | 591.2 | 705.7 | 818.8 |
| TMM − measured | −0.6 % | −0.4 % | −0.2 % | +0.6 % | −0.1 % | +0.4 % | +0.8 % |
| f_n / n (measured) | — | 117.1 | 117.7 | 117.3 | 118.3 | 117.1 | 116.1 |
| cents vs n·B♭2 (116.54 Hz, A440), 20 °C [D] | −577 | +8 | +17 | +11 | +26 | +8 | −7 |
| TMM magnitude (MPa·s/m³) [D] | 89 | 67 | 81 | 117 | 140 | 160 | 169 |

TMM minima [D]: 123.8, 259.5, 379.8, 508.2, 634.0, 757.2, 883.0 Hz (measured: 122.5 … 878.8, within 1 %).
The [BC13] FDTD matched amplitudes "within a dB or less". Their plot is in dB re 1 MΩ, but its absolute calibration is
not readable from the text.

TMM continuation (no measured counterpart) **[D]**: peak 8 928.8 Hz (133 MPa·s/m³), 9 1043 (86), 10 1166 (53), 11 1289 (37),
12 1415 (28), 13 1541 (22), 14 1667 (19).

TMM used **[D]**: 1 mm lossy-cylinder steps, Zwikker–Kosten boundary-layer loss to first order, an unflanged
Silva-type radiation load at the 126.8 mm mouth, c = 343.4 m/s, 20 °C. Magnitudes from this simple loss model are
probably **10–20 % high** (no wall roughness, no junction losses) **[G]**. FT99 gives "R … typically of order
10⁸ Pa·m⁻³·s" [FT99], which agrees. Script logic: `scratchpad/scripts/tmm.py, fit.py` (not in the repo). Port it to
`tools/brass.py` with the bore above.

**Validation targets for pajupilli [G]:** peaks 2–7 within ±1 % (17 c) of the measured row, after scaling by c ratio
349.5/343.4 (+1.78 %, +31 c). Peak heights rise from n = 2 to a maximum at n = 7–8 (~800–900 Hz, the popping-frequency
envelope), then fall by ~10 dB by n = 11.

### 3.2 Shape of the resonance series

- The sounding series (written C4, G4, C5, E5, G5 …) is n = 2…8 of a **missing fundamental** B♭2 = 116.5 Hz. Peak 1
  (83.5 Hz, ~6 semitones flat of B♭2) is "weak and hard to play" and not in the series [UNSW].
- On this trumpet, relative to peak 4 (C5, which the tuning slide sets): **p2 −3 c, p3 +6 c, p5 +15 c, p6 −2 c, p7 −18 c**
  **[D]** (measured). The TMM extension gives **p8 ≈ −28 c, p9 ≈ −30 c** **[D]**, since the mouthpiece pulls the top down.
- Combined with the just/ET differences of the harmonic series (n = 3: +2 c, 5: **−14 c**, 6: +2 c, 7: **−31 c**,
  9: +4 c, 10: −14 c, 11: −49 c, 13: +41 c; standard arithmetic), this gives, relative to a slide-tuned C5:
  **E5 (p5) ≈ +1 c, G5 (p6) ≈ 0 c, B♭5 on p7 ≈ −49 c (unusable), C6 (p8) ≈ −28 c** **[D, peaks only]**. Played
  pitch is not the peak: see §3.4.
- Pedagogy disagrees in detail and is instrument-dependent: "the third, sixth, and twelfth will be sharp, and the fifth and
  tenth will be flat" [UTC, U]. Take it as a description of what players lip, not as data for this bore.

### 3.3 Cut-off and the top

- Bell cut-off: "typically about 1000 Hz" [FT99]. In the TMM, peaks drop by > 6 dB from n = 8 (929 Hz) to n = 10 (1166 Hz)
  **[D]**. HyperPhysics: peak heights grow up to the 7th, the 10th sits at ~1170 Hz, and peaks above it diminish
  rapidly [HP].
- Players nevertheless play to ~1.4–1.5 kHz (sounding F6, written G6) **without** tuning a vocal-tract resonance. The weak
  peaks plus a strongly driven lip suffice [CSW12]. High notes have lip frequencies > 1000 Hz [UNSW]. For the model this
  means the upper register (partials 9–12) relies on weak bore peaks. Expect the 1-mass lip to struggle there.
  The second (inward) lip mode in `LipParams` exists for exactly this.

### 3.4 Playing frequency vs resonance

- Eveno et al. 2014 [EKGPC14] (three leadpipes on one trumpet, several players): "for most of the regimes, variations of
  the resonance frequency lead to same-order variations of the playing frequency". Players have a "limited influence"
  on overall intonation, and dynamics change pitch little. The "sum function" predicts no better than the peak frequencies
  (abstract-level **[U]**).
- So the calibration targets are: the bore peaks (§3.1) plus a small, register-dependent lip offset. The 1-mass lip plays
  above the peak (horns.md §1.1). Low register: played slightly **above** the peak. Upper register: lip motion becomes
  partly transverse (horns.md §1.1, [BFSW13]).

### 3.5 Pedal notes and false tones

- **Pedal** (written C3, sounding B♭2, 116.5 Hz; or with valves down to written ~F#2): not on a resonance. Peak 1 is at
  83.5 Hz. The lips sustain the missing fundamental by **mode locking**: "the lips establish a nonlinear vibration at the
  frequency of the missing fundamental", sustained by resonances 2f, 3f, 4f. "The spectrum of a pedal note has hardly
  any power at the fundamental" [UNSW]. It is "harder to play than on the trombone and … not considered as useful
  musically" [HP]. The model gets this behaviour (Mattéoli's pedal) if f_l is set at ~B♭2 and the pressure is low
  **[G]**.
- **False tones** (written ~F3–B2, between pedal and F#3) are played by lipping the 2nd partial down with valves. They are
  weak and unstable. Expose them only through the pitch-bend control **[G]**.

---

## 4. Fingerings

### 4.1 Standard chart, written F#3 … C6 (sounding E3 … B♭5; transposition −2)

Format as `NoteInfo` in `src/brass.cc` ("123", x = pressed; partial of the valved tube). Standard school chart [HP, UNSW;
universal practice]:

```
const NoteInfo kTrumpetNotes[] = {
    {"xxx", 2},   // F#3   (3rd slide out ~56 mm)
    {"xox", 2},   // G3    (3rd slide out ~30 mm)
    {"oxx", 2},   // G#3
    {"xxo", 2},   // A3
    {"xoo", 2},   // Bb3
    {"oxo", 2},   // B3
    {"ooo", 2},   // C4
    {"xxx", 3},   // C#4   (3rd slide out)
    {"xox", 3},   // D4    (3rd slide out)
    {"oxx", 3},   // Eb4
    {"xxo", 3},   // E4    (sharp; 1st slide)
    {"xoo", 3},   // F4
    {"oxo", 3},   // F#4
    {"ooo", 3},   // G4
    {"oxx", 4},   // G#4
    {"xxo", 4},   // A4    (alt 3)
    {"xoo", 4},   // Bb4
    {"oxo", 4},   // B4
    {"ooo", 4},   // C5
    {"xxo", 5},   // C#5
    {"xoo", 5},   // D5
    {"oxo", 5},   // Eb5
    {"ooo", 5},   // E5
    {"xoo", 6},   // F5
    {"oxo", 6},   // F#5
    {"ooo", 6},   // G5
    {"oxx", 8},   // G#5   (p7 is skipped: ~30-50 c flat)
    {"xxo", 8},   // A5    (alt 3)
    {"xoo", 8},   // Bb5
    {"oxo", 8},   // B5
    {"ooo", 8},   // C6
};
```

Above C6 (professional; not in the table yet): C#6 `xxo`/10 or `oxo`/9, D6 `xoo`/10 or `ooo`/9, Eb6 `oxo`/10,
E6 `ooo`/10, F6 `xoo`/12 (or `ooo`/11, sharp of F6 by ~+50 c: an alternative used in lead playing), F#6 `oxo`/12, G6 `ooo`/12.
Partial assignments above 8 are standard practice **[U]**.

**Notes on partial 7** [D]: the open 7th peak is −18 c relative to p4 on this bore, and the harmonic is −31 c from ET.
Written B♭5 open would be ~−49 c. Players use valve 1 on p8. A5/A♭5 on p7 (`oxo`/7, `xoo`/7, both ~50 c flat) are also not used in the
standard chart.

### 4.2 Alternates (the `kTrumpetAlt` table)

| Written | Standard | Alternate | Why |
|---|---|---|---|
| A4 | 12/p4 | **3/p4** | 1+2 is sharp (+10 c) |
| D5 | 1/p5 | **13/p6** | tuning, trills |
| C#5 | 12/p5 | **123/p6** | tuning (123 needs the slide) |
| Eb5 | 2/p5 | **23/p6** | trills |
| E5 | 0/p5 | **12/p6** | 5th-partial flatness on many horns |
| F#5 | 2/p6 | **123/p8** | trills, intonation |
| G5 | 0/p6 | **13/p8** | trills, C6–G5 slurs |
| G#5 | 23/p8 | **123/p9** | rare; high-register trills |
| A5 | 12/p8 | **3/p8** | 1+2 sharp; very common |
| B♭5 | 1/p8 | **0/p7** | lip-trill/jazz only; flat ~50 c |

The alternates follow from the arithmetic of partial × valve offset. Their use is from [UTC, WEID, U] and common pedagogy
**[U]**. Mark the 0/p7 B♭5 as "sounds flat" if exposed.

```
const NoteInfo kTrumpetAlt[] = {   // written F#3 .. C6
    {nullptr, 0}, {nullptr, 0}, {nullptr, 0}, {"oox", 2}, {nullptr, 0}, {nullptr, 0}, {nullptr, 0},  // F#3..C4
    {nullptr, 0}, {nullptr, 0}, {nullptr, 0}, {"oox", 3}, {nullptr, 0}, {nullptr, 0}, {nullptr, 0},  // C#4..G4
    {nullptr, 0}, {"oox", 4}, {"xxx", 5}, {"xox", 5}, {"oxx", 5},                                 // G#4..C5
    {"xxx", 6}, {"xox", 6}, {"oxx", 6}, {"xxo", 6},                                                 // C#5..E5
    {nullptr, 0}, {"xxx", 8}, {"xox", 8},                                                           // F5..G5
    {"xxx", 9}, {"oox", 8}, {"ooo", 7}, {"xxo", 9}, {"xoo", 9}                                     // G#5..C6
};
```

(A3 `oox`/2 and E4 `oox`/3 are the same "3 for 1+2" substitution. B♭4 `xxx`/5, B4 `xox`/5 and C5 `oxx`/5 are the
higher-partial alternates of the 4th-partial notes (lip-trill/tuning use). B5 `xxo`/9 and C6 `xoo`/9 are the high
alternates on the 9th partial. All
alternates are **[D]** arithmetic; whether players like them is **[U]**.)

---

## 5. Lips, cup and breath: trumpet numbers

### 5.1 What the literature gives

| Quantity | Value | Source |
|---|---|---|
| Lip mass, area, width, rest gap (model) | m = 5.37·10⁻⁵ kg, S_r = 1.46·10⁻⁵ m² (→ μ ≈ 3.7 kg/m²), w = 10 mm, H = 0.29 mm at f_l = 550 Hz, damping "σ = 5", p_m = 5 kPa | Harrison et al. score example [HBPW15] (after Adachi & Sato) |
| Lip opening area law | S ≈ C x^{3/2}, **C ≈ 0.05 m^{1/2}**; opening saturates at ~**1 mm** | [FT99] |
| Lip vibration ≈ sinusoidal, closing once per cycle; amplitude ≈ rest opening | — | [FT99] after Martin |
| Threshold | p₀ = K f, **K = 7 (GC, professional), 8 (NH), 4 (KB) Pa/Hz** | [FT99] Fig. 4 |
| Range of pressures | GC up to **25 kPa** (high notes); NH to 15 kPa; KB to 7 kPa | [FT99] |
| Bouhuys (in [FT99]) | **4 kPa (pp) – 13 kPa (ff)**, high notes | [FT99] |
| Output vs pressure | +15 dB per doubling above threshold; saturates to +3 dB per doubling | [FT99] |
| Efficiency | ~0.1 % (very soft) to ~3 % (loud), lip-generator level; radiated 0.01–1 % | [FT99] |
| Max SPL | ~110 dB(A) at 1 m, high notes | [FT99] |
| Lip Q (humans < artificial lips) | trombone data; trumpet not found | [NCG08] (horns.md) |
| Vocal tract | peaks usually "rather smaller" than the bore's; **no systematic tuning** by 7 trumpeters, up to ~1.5 kHz; f₂ 500–1400 Hz, f₃ > 1500 Hz | [CSW12] |
| Pitch bend | players bend with lips/pressure, not the tract: f₂ stays above f in bends | [CSW12] |

### 5.2 Pressure map for the keyboard [D/G]

p₀ = K·f_sounding, K = 6 Pa/Hz (mean of FT99's 4–8):

| Written | Sounding f (Hz) | p₀ (kPa) | pp ≈ 1.3 p₀ | ff ≈ 3–4 p₀ (clamped) |
|---|---|---|---|---|
| F#3 | 165 | 1.0 | 1.3 | 3.5 |
| C4 | 233 | 1.4 | 1.8 | 5 |
| C5 | 466 | 2.8 | 3.6 | 10 |
| C6 | 932 | 5.6 | 7.3 | 18–20 |
| (G6) | 1397 | 8.4 | 11 | 25 (ceiling) |

The C6 row matches Bouhuys' 4–13 kPa for high notes **[D]**. **pressHi = 20 kPa** (25 kPa possible but exceptional).

### 5.3 Suggested `LipParams` for the trumpet [G]

Same layout as `kHornLips` (cup, μ, Q, w, hk, S_r, k, pressLo, pressHi, fl, r2, q2, a2):

```
const LipParams kTrumpetLips = {0.92e-6, 3.7, 7.5, 0.010, 0.074, 1.46e-5, 7.0, 600, 20000, 0.85,
                                1.3, 3.0, 0.7, kTrumpetValves, 3};
```

- cup 0.92 cm³ [D, §1.4]; μ = 3.7 kg/m² [HBPW15, D]; w = 10 mm [HBPW15]; S_r = 1.46·10⁻⁵ m² [HBPW15];
  hk = 0.074 m·Hz [MGV22] gives H = 0.32 mm at 233 Hz and 0.08 mm at 932 Hz, consistent with HBPW's 0.29 mm at 550 Hz
  only within a factor ~2 **[D]**. If high notes do not speak, try a larger hk or an H floor of 0.1 mm.
- Q 7.5, fl 0.85, r2/q2/a2: copied from the horn (calibrated there). Trumpet-specific values are not found **[G]**.
- k = 7 Pa/Hz: FT99's professional (GC). pressLo 600 Pa so the pedal and F#3 can speak **[G]**.

```
const ValveDef kTrumpetValves[] = {   // throat coordinates (rim − 9.8 mm); air meets 3, 2, 1
    {720.0, 180.1, 11.75, 2},          // valve 1
    {665.0, 87.7, 11.75, 1},           // valve 2
    {610.0, 277.3, 11.75, 3},          // valve 3 (slide: +30 mm for 1+3, +56 mm for 1+2+3)
};
```

---

## 6. Timbre

### 6.1 Spectral envelope (Fletcher & Tarnopolsky [FT99], player GC, anechoic)

- C4 soft/loud: **+6 dB/oct below ~1 kHz**, a nearly constant slope from 1 kHz to an **upper transition at 2–3 kHz**
  that rises with level, then **−30 dB/oct (soft)** or **−10 dB/oct (loud)** above it.
- Basic low-level envelope: rises to a **maximum near 1000 Hz**, then falls. On top of it a level-dependent
  mechanism extends the envelope upward. FT99 identify this as nonlinear propagation "particularly in the long
  cylindrical part" (Hirschberg et al.).
- The frequency where the envelope is 20 dB below its peak rises with blowing pressure (Fig. 9) and reaches ≥ 16 kHz in
  loud high notes (microphone-limited). Scaling: note ×a with pressure ×a^{3/4} gives the same spectrum shape.
- Loud playing gives an "apparent power gain at high frequencies" relative to the mouthpiece spectrum: energy moves from
  low to high harmonics.

### 6.2 Brassiness / nonlinear propagation

- Brassiness potential (Pyle & Myers): **B = L_cyl-equivalent / L_ecl** with u ∝ D₀/D(x). L_ecl is the length of a complete
  cone with the instrument's nominal fundamental, "generally slightly longer than the physical length" [PM10].
- **Trumpets in Myers' data set: B ≈ 0.68–0.77**, at minimum bore 9.2–11 mm. Flugelhorns: 0.50–0.59 [D: digitised from
  [PM10] Fig. 6, adapted from Myers et al.]. The trombone's 0.70 (horns.md) is in the same range.
- This bore, computed **[D]**: D₀ = 8.70 mm (leadpipe entry), L_ecl = c/(2·116.54 Hz) = 1.473 m: **B = 0.57**. With D₀ = 9.69
  (receiver) B = 0.63. The difference from Myers' 0.68–0.77 is the D₀ convention or the Screamer's long narrow leadpipe
  **[G]**.
- **For N1 (horns.md §6.2)**: **L_nl ≈ 0.6–0.7 × 1.47 m ≈ 0.9–1.0 m** of equivalent cylinder at Ø 11.75 mm **[D]**. Take
  the split point at the end of the cylinder (x ≈ 841 mm, rim coordinates). FT99: a 2 kPa wave has ~5 m/s particle velocity.
  Over ~1 m of cylinder this gives ~0.05 ms of convective gain, enough to steepen.
- Vergez & Rodet [VR01] (trumpet C5): enrichment with level is much stronger at the bell than in the mouthpiece. A
  real-time trumpet used a linear loop plus nonlinear propagation on the way out. See horns.md §6.
- The shock-formation distance at the C5 ff example, 1.3 m (horns.md §6.1), is about the trumpet's length. ff trumpet is
  therefore close to shocks, and N1 is not optional for ff **[D]**.

### 6.3 Directivity (brief)

Pätynen & Lokki [PL10]: symmetric about the bell axis, directivity increasing from 1 kHz, "within −6 dB" all round at
C#5. Dynamics do not change the pattern, but loud playing emphasises > 2 kHz in front. ka = 1 at the 126.8 mm mouth:
**861 Hz** [D]. The first non-planar mode cuts on at ~3.4 kHz (horns.md §3.5). **Model [G]**: the bell-direction shelf
of horns.md §5.3 with a corner at ~900 Hz, −10 dB at 90°, −20 dB behind, bell pointing at the audience.

### 6.4 Mutes (for later)

Straight, cup, Harmon and practice mutes add formants in the 1–3 kHz range [UNSW]. "Closed" mutes (practice/silent) keep
strong high impedance peaks, while open mutes lose the peaks above ~1.4 kHz [FORM, a student report, U].

---

## 7. Playing facts for a keyboard model

| Item | Value | Source |
|---|---|---|
| Written range, standard | **F#3 – C6** (sounding E3 – B♭5) | [HP, UNSW]. HP: "about three octaves", low end "F3 sharp = 185 Hz". That is concert F#3; written F#3 sounds E3 = 165 Hz, so HP's label is probably concert pitch **[U]** |
| Professional / lead | to F6–G6 (sounding E♭6–F6 = 1245–1397 Hz); players in [CSW12] reached written G6 "a very high note for the trumpet"; lead players higher **[U]** | [CSW12] |
| Pedal | written C3 and below (§3.5) | [UNSW, HP] |
| Pitch bend | ~+½ / −1 semitone (trombone data, [BSW20]); trumpeters bend without vocal-tract tuning | horns.md §1.1, [CSW12] |
| Vibrato type | jaw vibrato is the most common on brass; hand (valve-shaking) vibrato on trumpet, "particularly in jazz" | [WEID] |
| Vibrato rate / depth | **5–6.5 Hz, ±10–25 c [G]**. General musical vibrato 4–7 Hz, 5.5–6 Hz common [SAGE, U]. Herrick's dissertation measured 10 trumpeters' jaw and hand vibrato, numbers not obtained [HER, U]. Orchestral/legit trumpet: little or delayed vibrato; jazz/commercial: wider, often late **[G]** | — |
| Vibrato in the model | modulate f_l (as in NESS: f_l(1 + A sin 2πf_v t)) for jaw vibrato; pressure "tremolo" separately; valve modulation for hand vibrato [HBPW15] | [HBPW15] |
| Valve speed | 10–30 ms (horns.md §4.1) **[G]**; half-valve effects as in [HBPW15] | — |
| Attack | tongued, fast. Nonlinear propagation shortens the attack ~2× and makes a transient pitch excursion [BLVC17] (horns.md §6.1). Higher harmonics build up later than lower ones in trumpet attacks (Risset 1966, via citations **[U]**). **[G]**: 20–40 ms to full spectrum at mf, < 15 ms at ff "t" | — |
| Lip slurs / lip trills | possible on partials 6–10 (a whole tone apart and closer) | horns.md §7.3 |
| Main tuning slide | ~13 mm out for warm air at A440 [D, §2.3] | — |

---

## 8. Open questions and next steps

1. **Port the TMM** (`tools/brass.py -inst trumpet`) with the §1.3 bore and check the waveguide against §3.1 at 48 kHz.
   Peaks 2–7 should land within 1 % after the c scaling.
2. **Bach 37 vs Smith-Watkins**: the only real profile found with numbers is [BC13]'s. Braden's thesis, Kausel's
   optimisation papers and Eveno's thesis plot trumpet bores, but I found no digitisable vector figure or table in
   reach. For a "standard" trumpet, keep this bore and swap in a 3C-like cup (§1.4) **[G]**.
3. Trumpet-specific lip Q, the second-mode ratio and transverse motion at high notes: not found. Calibrate.
4. Vibrato measurements (Herrick dissertation [HER]) not read.
5. Magnitude calibration: the [BC13] Fig. 9 trumpet impedance curve might also be extractable from the PDF. I did not find
   it among the vector paths (possibly raster). The TMM magnitudes are the best available **[D]**.

---

## References

[BACH] Bach Stradivarius 180S37 specification (0.459" ML bore, #37 bell 4.8", #25 leadpipe), e.g. https://bachbrass.com/print/bach-stradivarius-trumpet-in-bb-180s37 and dealer pages (via search; **[U]** for the exact bell Ø)
[BC13] S. Bilbao, J. Chick, "Finite difference time domain simulation for the brass instrument bore", JASA 134(5), 3860–3871 (2013). Preprint: https://www.ness.music.ed.ac.uk/wp-content/uploads/2013/08/bilbao_jasa_13_r2_double.pdf (read: §IV, Fig. 7 digitised, Table I)
[BFSW13] H. Boutin, N. Fletcher, J. Smith, J. Wolfe, SMAC 2013. https://newt.phys.unsw.edu.au/jw/reprints/SMACtrombone.pdf (see horns.md)
[BLVC17] H. Berjamin et al., arXiv:1511.04247 (see horns.md)
[BSW20] H. Boutin, J. Smith, J. Wolfe, JASA 147(6) (2020). https://newt.phys.unsw.edu.au/jw/reprints/lipping.pdf (see horns.md)
[CSW12] J.-M. Chen, J. Smith, J. Wolfe, "Do trumpet players tune resonances of the vocal tract?", JASA 131(1), 722–727 (2012). https://www.phys.unsw.edu.au/jw/reprints/TrumpetTract.pdf (read)
[EKGPC14] P. Eveno, B. Kieffer, J. Gilbert, J.-F. Petiot, R. Caussé, "The relationship between bore resonance frequencies and playing frequencies in trumpets", Acta Acustica u. A. 100(2), 362–374 (2014) (pages **[U]**). https://www.ingentaconnect.com/contentone/dav/aaua/2014/00000100/00000002/art00020 (abstract only)
[FORM] G. Formosa, "Acoustic harmonicity and input impedance for various B♭ trumpet mutes", UIUC Physics 406 student report (no URL kept; via search) **[U]**
[FT99] N. H. Fletcher, A. Tarnopolsky, "Blowing pressure, power, and spectrum in trumpet playing", JASA 105(2), 874–881 (1999). https://www.phys.unsw.edu.au/music/people/publications/Fletcheretal1999.pdf (read)
[HBPW15] R. L. Harrison, S. Bilbao, J. Perry, T. Wishart, "An environment for physical modeling of articulated brass instruments", CMJ 39 (2015). https://www.pure.ed.ac.uk/ws/files/21585566/Harrison_etal_CMJ_2015_An_Articulated_Brass_Instrument_Synthesis.pdf (read: Tables 2–3. Their example instrument, vpos 600/630/660, vbl 130.7/57.5/199.8 mm, Leg = 1000 mm, is a generic demo, **not** a trumpet: do not use its loops.) Also https://github.com/tommmmudd/Ness-Brass-Interface (same defaults)
[HER] "An investigation of the frequency modulations and intensity modulations of the vibrato on selected brass instruments", Univ. of North Texas dissertation (10 trumpeters, 10 trombonists). https://openaccess.library.uitm.edu.my/Record/ndltd-unt.edu-info-ark-67531-metadc331150 (abstract only, **[U]**)
[HP] HyperPhysics, "The Trumpet". https://hyperphysics.gsu.edu/hbase/Music/trumpet.html (read)
[M16] T. R. Moore, "The acoustics of brass musical instruments", Acoustics Today 12(4) (2016), pp. 35–36 online: https://acousticstoday.org/issues/2016AT/2016Winter/files/basic-html/page38.html (read)
[MGV22] R. Mattéoli et al., arXiv:2112.08751 (see horns.md)
[MPE] Mouthpiece Express, Bach throat/backbore and 3C specifications. https://mouthpieceexpress.com/specshub/specs/specs_bach_throats_backbores.html ; https://mouthpieceexpress.com/products/bach-symphonic-trumpet-mouthpiece-3c (via search, **[U]**)
[NCG08] M. J. Newton, D. M. Campbell, J. Gilbert, JASA 123(1) (2008) (see horns.md)
[NTBIS] NTBIS, "How valves are used to change the note played". https://ntbis.org.uk/brass-music-intro/how-do-brass-instruments-work/how-valves-are-used-to-change-the-note-played/ (via search **[U]**)
[PL10] J. Pätynen, T. Lokki, Acta Acustica u. A. 96 (2010). https://users.aalto.fi/~ktlokki/Publs/patynen_aaua_2010.pdf (see horns.md)
[PM10] R. W. Pyle, "The effect of lip motion and mouthpiece on brassiness" / brassiness potential overview, Proc. Second Vienna Talk (2010), pp. 117–120. https://viennatalk.mdw.ac.at/papers/Pap_02_15_Pyle.pdf (read; Fig. 6 digitised)
[RH12] N. Rose, D. Holloway, Proc. Acoustics 2012. https://logosfoundation.org/instrum_gwr/bug/p60.pdf (see horns.md)
[SAGE] "Vibrato", in *Music in the Social and Behavioral Sciences* (SAGE). https://sk.sagepub.com/ency/edvol/embed/music-in-the-social-and-behavioral-sciences/chpt/vibrato (snippet **[U]**)
[TIP] "Anatomy of the Trumpet", https://www.theinstrumentplace.com/parts-of-the-trumpet/ ; US Patent 1,724,632 "Trumpet", https://patents.google.com/patent/US1724632 (air path 3→2→1; via search **[U]**)
[UNSW] J. Wolfe et al., "Brass instrument (lip reed) acoustics: an introduction". https://newt.phys.unsw.edu.au/jw/brassacoustics.html (read)
[UTC] E. Schafer, "Trumpet tuning tendencies relating to the overtone series with solutions". https://blog.utc.edu/erika-schafer/?p=76 (via search **[U]**)
[VR01] C. Vergez, X. Rodet, ICMC 2001 (see horns.md)
[WEID] B. Weidner, *Brass Techniques and Pedagogy*, ch. "Advanced techniques" and "Pitch and intonation". https://pressbooks.palni.org/brasstechniquesandpedagogy/?p=352 ; https://pressbooks.palni.org/brasstechniquesandpedagogy/chapter/pitch-and-intonation/ (via search **[U]**)
[YAM] Yamaha, "The structure of the trumpet". https://www.yamaha.com/en/musical_instrument_guide/trumpet/mechanism/mechanism002.html (fetch refused; numbers from a search snippet **[U]**)

Not obtained: Braden's thesis (Edinburgh 2006), Kausel's and Noreland's trumpet bell optimisation data, Eveno's thesis
(trumpet impedances with fingerings), Backus 1976 (trumpet impedance curves), Adachi & Sato 1996 parameter tables,
Luce & Clark 1967 / Risset 1966 (trumpet attacks and spectra).
