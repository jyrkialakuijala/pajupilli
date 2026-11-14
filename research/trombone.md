# The tenor (and bass) trombone on the pajupilli waveguide — research notes

Status: 2026-11-14. SI units, mm for geometry. Extends `research/horns.md` (§3.1 table, §4.2 slide, §6, §7);
read that first. Tags as there: **[G]** my estimate or guess, **[D]** derived by me from published data or
formulas (here mostly: digitised from a vector figure, or computed with `tools/brass.py`'s transfer-matrix code on
the digitised bore), **[U]** taken from a secondary source or an abstract that I could not check in the primary text.
References are at the end.

Short answer:
1. **A real, validated bore exists:** the Conn 8H (Bilbao & Chick 2013 [BC13], Fig. 7) is stored as vector
   paths in their preprint. I extracted the 55 vertices exactly (§1.1). The extracted mouthpiece matches the maker's
   Denis Wick 5AL figures (rim 25.6 vs 25.7 mm, throat 7.23 vs 7.30 mm). The transfer-matrix impedance of the extracted bore lands within
   ~1 % of the **measured** peaks of the same instrument (§3.1). Use it as is.
2. **The slide** (fitted on that bore so partials 3–8 move by whole semitones from 1st position): travel from 1st
   = **0, 85, 174, 269, 370, 474, 583 mm** (tube added = 2×). Gaps grow from 85 to 109 mm, matching players' "3 in to
   just over 4 in" (§2.2). F rotor loop **≈ 966 mm** of tube. Bass trombone G♭ loop **≈ 770 mm**; F + G♭ together give a D
   about 15 cents flat (§2.4).
3. **Impedance (1st position, Conn 8H, 19 °C, measured):** 38.3 (false), 113.2, 174.0, 233.2, 296.0, 350.9, 410.1, 471, 529,
   594, 655, 709 Hz… The UNSW data set (Yamaha, 26 °C) adds magnitudes: **24, 20, 21, 23, 19, 13, 12, 10, 6 MΩ** for
   peaks 2–10 (§3).
4. **New physics vs. the horn:** the slide (a smoothly variable-length cylinder, §2.5), the highest brassiness
   (B ≈ 0.71 computed for the 8H, §6), a big cup (≈ 9.6 cm³ from the rim plane, popping ≈ 520 Hz, §5), and the
   **legato-tongue** articulation that hides the slide's glissando (§7).

---

## 1. The bore

### 1.1 Conn 8H tenor trombone, 1st position, with a Denis Wick 5AL mouthpiece (Bilbao & Chick 2013)

**Source and method [D].** [BC13] Fig. 7 (middle panel) plots the measured bore radius against the axial coordinate,
with a log radius axis. The preprint's PDF has the curve as a 55-vertex polyline, and I converted it with the panel's
own tick marks: x 0…2500 mm at 286.0 PDF units / 500 mm, log₁₀ r at 234.8 units/decade (the 10², 90…20 mm ticks).
The vertices are the authors' own polyline, so this is a readout of their data rather than a hand digitisation. The
readout error is under 0.1 mm in x and about 0.3 % in diameter. The measurement itself is quoted as **±5 %** in
diameter: external callipers minus a nominal wall, mid-line lengths in the bends [BC13]. The instrument is at 19–20 °C, overall
≈ 2.74 m. Only the x = 2643.8 vertex, a 0.2 mm dip that is a plotting artefact, should be dropped.

| x (mm) | d (mm) | Part (my reading **[G]**) |
|---|---|---|
| 0.0 | 25.59 | rim plane (cup Ø; 5AL spec 25.73 [DW]) |
| 4.4 | 25.59 | |
| 9.3 | 24.98 | cup |
| 14.4 | 22.48 | |
| 19.4 | 17.45 | |
| 24.4 | 11.29 | |
| 26.9 | 8.00 | throat entry |
| 29.4 | 7.23 | **throat** (5AL spec 7.30 [DW]) |
| 44.4 | 7.69 | backbore |
| 49.4 | 8.08 | |
| 54.4 | 8.37 | |
| 59.4 | 8.89 | |
| 69.4 | 9.87 | |
| 74.4 | 10.63 | |
| 79.5 | 12.72 | shank end → receiver/leadpipe (12.7 mm = minimum bore D₀) |
| 84.5 | 12.76 | |
| 89.5 | 13.60 | start of inner slide (8H: 0.547 in = 13.9 mm nominal [U]) |
| 689.4 | 13.60 | end of the down-going slide leg (600 mm) |
| 764.6 | 14.79 | slide crook (bow), slightly wider |
| 919.2 | 13.60 | |
| 1594.3 | 13.60 | end of the up-going slide leg (675 mm) |
| 1666.3 | 13.84 | bell-section entry / tuning slide |
| 1694.6 | 14.23 | |
| 1794.7 | 14.43 | |
| 1829.8 | 14.48 | |
| 1931.3 | 15.16 | bell section: slow cone |
| 2031.4 | 17.26 | |
| 2131.7 | 19.38 | |
| 2138.5 | 19.38 | |
| 2240.6 | 21.46 | |
| 2328.3 | 24.44 | |
| 2340.8 | 25.50 | |
| 2427.8 | 29.98 | flare |
| 2440.4 | 31.04 | |
| 2473.5 | 33.07 | |
| 2513.6 | 36.62 | |
| 2540.5 | 39.00 | |
| 2551.8 | 40.40 | |
| 2583.8 | 44.75 | |
| 2608.1 | 49.36 | |
| 2627.6 | 54.47 | |
| 2640.7 | 60.53 | |
| ~~2643.8~~ | ~~60.32~~ | (artefact; drop) |
| 2657.0 | 66.79 | |
| 2668.3 | 73.69 | |
| 2677.7 | 81.31 | |
| 2685.8 | 90.03 | |
| 2694.6 | 99.34 | |
| 2700.8 | 109.61 | |
| 2707.1 | 121.36 | |
| 2713.3 | 133.91 | |
| 2723.3 | 164.17 | |
| 2729.0 | 181.15 | |
| 2733.3 | 199.87 | |
| 2735.2 | 213.74 | **bell mouth** (8H: 8.5 in = 216 mm nominal [U]) |

Ready to paste, in the `BorePoint` format of `src/brass.cc` (x from the rim plane). The cup (0–26.9 mm) is lumped as in the
horn, so the list starts at the throat entry:

```
// Conn 8H tenor trombone, 1st position, Denis Wick 5AL (Bilbao & Chick 2013, Fig. 7, read from the
// preprint's vector paths). Cup 0..26.9 mm lumped: 9.56 cm^3 from the rim plane.
{26.9, 8.00}, {29.4, 7.23}, {44.4, 7.69}, {49.4, 8.08}, {54.4, 8.37}, {59.4, 8.89}, {69.4, 9.87},
{74.4, 10.63}, {79.5, 12.72}, {84.5, 12.76}, {89.5, 13.60}, {689.4, 13.60}, {764.6, 14.79},
{919.2, 13.60}, {1594.3, 13.60}, {1666.3, 13.84}, {1694.6, 14.23}, {1794.7, 14.43}, {1829.8, 14.48},
{1931.3, 15.16}, {2031.4, 17.26}, {2131.7, 19.38}, {2138.5, 19.38}, {2240.6, 21.46}, {2328.3, 24.44},
{2340.8, 25.50}, {2427.8, 29.98}, {2440.4, 31.04}, {2473.5, 33.07}, {2513.6, 36.62}, {2540.5, 39.00},
{2551.8, 40.40}, {2583.8, 44.75}, {2608.1, 49.36}, {2627.6, 54.47}, {2640.7, 60.53}, {2657.0, 66.79},
{2668.3, 73.69}, {2677.7, 81.31}, {2685.8, 90.03}, {2694.6, 99.34}, {2700.8, 109.61}, {2707.1, 121.36},
{2713.3, 133.91}, {2723.3, 164.17}, {2729.0, 181.15}, {2733.3, 199.87}, {2735.2, 213.74}
```

Notes on this profile:
- **The leadpipe is missing.** On a real 8H a conical leadpipe (~ 230–300 mm **[G]**) sits *inside* the upper inner
  slide tube, and an external measurement cannot see it. [BC13]'s profile goes 12.7 → 13.6 mm within 10 mm. The impedance
  still matches to ~1 % (§3.1), so leave it. If a leadpipe is wanted, a cone 11.0 → 13.6 mm over 80–330 mm is a
  plausible tenor shape **[G]**. Retune the length afterwards.
- **The outer slide tube is not resolved.** The outer tube's inside diameter is the inner tube's outside diameter, i.e.
  ≈ 0.5–1 mm wider **[G]**. That step is acoustically minor and absent from the data. The 14.8 mm crook is in the data.
- The slide region is 89.5–689.4 (down leg) + crook + 919.2–1594.3 (up leg) = **1275 mm of straight cylinder**. The
  extension in the model goes there (§2.5).
- The bell is a Bessel-like flare. A fit of r = B(x₀−x)^(−ε) through the last 300 mm **[D]** gives ε ≈ 0.7–0.8 and x₀ a few mm
  past the mouth. Use the points, not a fit.

### 1.2 Cross-check: a measured historic tenor (Huschauer, Vienna 1794) [CCM14]

This is a published table (37 points, distance from the **mouthpiece receiver**, slide closed). It is a sackbut-era instrument:
11.0 mm slide bore, 115 mm bell, B = 0.81. Useful for a "sackbut/classical trombone" variant. Points (x mm, d mm):
(0, 11.9) (22.5, 11.0) (684, 11.0) (684, 11.6) (900, 11.6) (900, 11.0) (1534, 11.0) (1626, 11.2) (1626, 12.8) (1680, 11.3)
(1871, 11.0) (1908, 11.4) (2077, 11.6) (2109, 12.2) (2204, 13.5) (2239, 13.8) (2365, 16.7) (2403, 18.2) (2431, 20.1)
(2457, 22.2) (2484, 24.5) (2512, 27.1) (2536, 30.0) (2563, 33.1) (2587, 36.6) (2607, 40.5) (2624, 44.7) (2637, 49.4)
(2649.5, 54.6) (2660, 60.4) (2670, 66.7) (2679, 73.7) (2686, 81.5) (2694, 90.2) (2701.5, 99.5) (2708.5, 110.0) (2711, 115.0).
The 684–900 section (11.6 mm) is the outer slide and crook. Note the same overall shape: the flare starts ~ 300 mm before the mouth.

### 1.3 Bass trombone **[G]** (no published profile obtained)

Published facts: Courtois AC502 ("Legend") bass trombone, impedance in [MGV22] Fig. 2 (peaks in §3.3), B = 0.67 [CCM14].
Bass trombones typically have a **14.3 mm (0.562 in) slide** and a **241–254 mm (9.5–10 in) bell** **[U]** (makers'
specifications; Vienna Symphonic Library gives 12.6–13.9 mm / 205–220 mm for tenors [VSL]). Same pitch (B♭), so the same
length.

**Recipe [G]:** take §1.1. Keep x and the mouthpiece region up to 79.5 mm (or a bass mouthpiece: rim 27.5–28.5 mm, throat
7.5–8.0 mm, cup ~11–13 cm³ **[G]**). Scale every diameter in 79.5–1594.3 by 14.3/13.6. In the bell section scale by a factor
that moves log-linearly in log d from 14.3/13.6 at d = 13.6 to 241.5/213.74 at the mouth. Result for x ≥ 1594.3:

```
{1594.3, 14.30}, {1666.3, 14.56}, {1694.6, 14.98}, {1794.7, 15.20}, {1829.8, 15.25}, {1931.3, 15.99},
{2031.4, 18.26}, {2131.7, 20.57}, {2138.5, 20.57}, {2240.6, 22.83}, {2328.3, 26.09}, {2340.8, 27.26},
{2427.8, 32.18}, {2440.4, 33.35}, {2473.5, 35.59}, {2513.6, 39.51}, {2540.5, 42.15}, {2551.8, 43.70},
{2583.8, 48.54}, {2608.1, 53.68}, {2627.6, 59.39}, {2640.7, 66.18}, {2657.0, 73.21}, {2668.3, 80.98},
{2677.7, 89.58}, {2685.8, 99.45}, {2694.6, 110.02}, {2700.8, 121.71}, {2707.1, 135.11}, {2713.3, 149.47},
{2723.3, 184.22}, {2729.0, 203.80}, {2733.3, 225.43}, {2735.2, 241.50}
```

TMM peaks of this guess (19 °C, tenor mouthpiece) **[D]**: 38.2, 112.8, 173.6, 232.3, 296.4, 356.9, 412.7, 472.5, 533.8 Hz,
vs. the AC502's measured 39.5, 115.5, 173.3, 233.4, 294.1, 346.1, 407.2, 464.8, 521.4 Hz (temperature not stated). This is close
enough to start. Fit the bell to [MGV22] if the bass trombone matters.

---

## 2. The slide and the triggers

### 2.1 Geometry

Two fixed parallel **inner tubes** (the leadpipe sits in the upper one), an **outer U-tube** (two legs + the crook) that
slides over them, then the bell section with the tuning slide (and, on an F trombone, the rotor) [BC13, §1.1]. Extending the slide by
a travel s adds **2s** of cylinder, s in each leg. On the 8H the straight legs are 600 + 675 mm (§1.1). Maximum useful
travel ≈ 0.58–0.61 m (7th position), quoted by players as "about 23 in" [IEM]. Above that the outer tube still needs
~ 50–80 mm of overlap with the stockings on the inner tubes **[G]**.

### 2.2 Positions (B♭ side), fitted on the 8H bore

Method **[D]**: with `tools/brass.py`'s TMM (Webster + visco-thermal + LS load, c = 343.4 m/s), I inserted extra cylinder
in the slide and bisected it until peaks 3–8 *on average* moved by exactly k semitones from 1st position (relative, so the
instrument's own tuning offsets stay). Partial 2 drops a little faster (bell effect), and the residuals per partial are listed.

| Position | Note (partial 2 / 4) | Semitones | Travel from 1st (mm) | Gap (mm) | Tube added (mm) | Residuals, partials 3/4/5/6/7/8 (cents) |
|---|---|---|---|---|---|---|
| 1 | B♭2 / B♭3 | 0 | 0 | — | 0 | (reference) |
| 2 | A2 / A3 | 1 | **85** | 85 | 170 | −4 / +7 / −5 / −14 / +15 / +1 |
| 3 | A♭2 / A♭3 | 2 | **174** | 89 | 348 | −4 / +14 / −18 / −11 / +22 / −4 |
| 4 | G2 / G3 | 3 | **269** | 95 | 538 | −1 / +15 / −27 / −4 / +11 / +7 |
| 5 | G♭2 / G♭3 | 4 | **370** | 101 | 739 | +1 / +11 / −27 / −6 / +3 / +18 |
| 6 | F2 / F3 | 5 | **474** | 104 | 948 | +2 / +7 / −19 / −14 / +9 / +14 |
| 7 | E2 / E3 | 6 | **583** | 109 | 1167 | +1 / +7 / −11 / −20 / +17 / +7 |

Checks: an ideal equal-tempered extension of an equivalent cylinder L_eq gives travel_k = L_eq(2^{k/12}−1)/2. These numbers fit
**L_eq ≈ 2.74–2.83 m**, i.e. about the *physical* length, not L_ecl = 2.94–2.97 m [D]. Pedagogy: gaps "from 3 in for 1st to 2nd to just over 4 in for
6th to 7th" [PB] (76 → ~105 mm). The 7th is "about 23 in" (584 mm) from closed [IEM]. **Discrepancy:** Boutin et al.
[BFSW15] extended a Yamaha YBL-321's slide by **22.7 cm** for "4th position" (G2 series), vs. 269 mm here. Players do not
pull a full nominal 4th, the YBL-321 is a smaller student bore, and their notes were ±lipped. The model needs the
position → length map anyway; make it a per-instrument table and let the calibration trim it.

**What this means for intonation:** a position is a compromise. Within ±20–27 cents the partials drift: in this bore the
**5th resonance goes flat as the slide extends** (−27 c at 4th–5th), and the 6th goes flat by ~15–20 c at 6th–7th. Players shade positions per
note ("raised"/"lowered" positions) and lip the rest (±½ semitone up, 1 tone down [BSW20]). The keyboard model should
store a **per-note slide length** (calibrated), not per-position constants.

### 2.3 The F attachment (tenor) and its positions

- The thumb trigger (rotor) adds a loop that lowers by a **perfect fourth** (5 semitones) [PB]. Fitted on the 8H
  bore (loop at x ≈ 1700 mm, bore 13.9 mm; partials 3–8 on the F series) **[D]**: **loop ≈ 966 mm** of tube. With it, the open
  peaks are (cents vs the F series) 3: −23, 4: −16, 5: −14, 6: +4, 7: +25, 8: +25. Real loops are longer, because the rotor's passages
  and a tuning slide are inside them. Fit the length, not the drawing.
- **F-side positions [D]** (same relative fit, partials 3–8): travel from 1st **0, 108, 225, 348, 476, 613, 759 mm** for F2, E2, E♭2, D2,
  D♭2, C2, B1 (partial 2 of the F series). With the trigger the positions are ×(1+966/2830) ≈ 1.3 wider. **T6 = C2 lies past the B♭ 7th (583 mm)**,
  at or beyond the end of the slide, and **B1 is unreachable**. This is the well-known gap of the F tenor. Players pull the F tuning slide to "E" to get B1/C2 **[U]**.
  The F side's 3rd partial goes flat as the slide extends (−25 … −58 c). The mixed bore of the attachment makes the F-side series less even.
- Common trigger use **[U, standard practice]**: C3 = T1 instead of 6th, B2 = T2 instead of 7th (partial 3 of the F series), F2 = T1
  and E2 = T2 (partial 2), and the low notes E♭2–C2 that the B♭ side lacks. Kagarice [K91]: with the trigger, **every note above low E
  has at least two positions except the bottom-space F♯** (F♯2: 5th only).

### 2.4 Bass trombone triggers

| System | Valve 1 | Valve 2 alone | Both | Loops (tube, fitted on the §1.3 bore) **[D]** |
|---|---|---|---|---|
| Independent F/G♭ | F (−5 st) | **G♭ (−4 st)** | **D** (−8 st): the loops in series add 0.335 + 0.260 of L = −8.08 st **[D]**; the TMM gives −8.1…−8.3 st on partials 3–8 (D ≈ 10–25 c flat) | F ≈ **981 mm**, G♭ ≈ **770 mm** |
| Independent F/G | F | G (−3 st) | E♭ (−7.3 st → tuned to E♭) | G loop ≈ 0.19 L ≈ 540 mm **[G]** |
| Dependent F/D (older) | F | — (only through valve 1) | D (or E♭ by its slide) | 2nd loop ≈ L(2^{8/12}−1) − F loop ≈ 1660 − 980 ≈ **700 mm [G]** |

Practice: the combination is tuned to D (or E♭) with the second valve's slide **[U]**. In D the positions are ~1.6× wider than on the
B♭ side, and the short positions give D2, D♭2, C2, B1 [D]. This fills the F tenor's B1/C2 gap down to the pedal B♭1 **[U]**.

### 2.5 How to model the moving slide

`research/horns.md` §4.2 recommends **stretching** a fixed number of slide cells. Numbers for the 8H **[D]**:
- The stretched region is the two straight legs, **1275 mm** in 1st. At 32 kHz (dx = 11.2 mm) that is N_s = 114 cells at
  Courant 0.975. In 7th, 1275 + 1167 = 2442 mm, dx_s = 21.4 mm, λ_s = 0.51. The leapfrog phase error (1−λ²)(k dx)²/24 is then
  ~0.5 % at 1 kHz in that region, ~0.3 % averaged over the bore (≈ 5 cents at 1 kHz, < 1 cent at 300 Hz). Acceptable.
  If not: let N_s grow by one cell whenever λ_s < 0.75. Insert the new cell at the crook, where the profile is cylindrical and
  p, u are interpolated, during a zero-crossing of the slide velocity **[G]**.
- Per block, update only the stretched cells' compliance (∝ dx_s) and inertance (∝ dx_s); the losses (∝ dx_s) follow the same way.
  The crook (764.6 mm, 14.8 mm) moves *with* the outer slide. Keep it as fixed cells between the two stretched legs.
- **Continuity is free.** A linear map position → travel (§2.2 table, interpolated) gives a smooth glissando with no
  cell re-meshing. The partials glide together, except near the 2nd partial, which glides slightly faster.
- **Slide speeds [G]:** adjacent positions in 30–60 ms; 1st→7th in 100–200 ms; peak 3–6 m/s. dL/dt = 2v ≤ 12 m/s is ≤ 3.5 % of c,
  so the moving boundary is quasi-static. The Doppler-like transient is real but small. No extra modelling needed.
- **Cells.** Open 2735 mm → 244 cells at 32 kHz; 7th + F loop: 2735 + 1167 + 966 = 4868 mm → 435 cells. Bass with both loops in
  7th: 2735 + 1167 + 981 + 770 → ~500 cells [D] (fits the 512 proposal in horns.md §3.4).

---

## 3. Input impedance (1st position)

### 3.1 Conn 8H [BC13]: measured, and our TMM of the extracted bore

Measured frequencies are from [BC13] Table I (19 °C); peaks 8–16 and heights are read from the vector curve of Fig. 9 (middle panel) **[D]**. The height
axis there reads "dB" without an unambiguous reference: the plotted *differences* are almost exactly half of 20·log₁₀ ratios
from the TMM (peaks 1–6) and the UNSW data, so the axis looks like **10·log₁₀|Z|** **[D, unverified]**. Use the relative column.

| Peak n | Measured f (Hz) [BC13] | f/n | TMM of §1.1 bore, c = 343.4 (Hz) **[D]** | TMM \|Z\| (MΩ = 10⁶ Pa·s/m³) **[D]** | Measured height as plotted (relative to n = 2) |
|---|---|---|---|---|---|
| 1 ("false") | 38.3 | — | 37.9 | 55.0 | +2.5 |
| 2 | 113.2 | 56.6 | 111.9 | 31.0 | 0 |
| 3 | 174.0 | 58.0 | 172.4 | 25.1 | −0.9 |
| 4 | 233.2 | 58.3 | 231.1 | 21.0 | −1.6 |
| 5 | 296.0 | 59.2 | 295.0 | 21.2 | −1.0 |
| 6 | 350.9 | 58.5 | 355.2 | 25.4 | −0.9 |
| 7 | 410.1 | 58.6 | 411.2 | 20.1 | −2.8 |
| 8 | 471.0 [D] | 58.9 | 471.4 | 18.6 | −2.7 |
| 9 | 529 [D] | 58.8 | 533.0 | 21.5 | −3.1 |
| 10 | 594 [D] | 59.4 | 591.0 | 14.2 | −5.7 |
| 11 | 655 [D] | 59.5 | 652.3 | 9.8 | −6.5 |
| 12 | 709 [D] | 59.1 | 713.6 | 8.8 | −6.5 |
| 13 | 772 [D] | 59.4 | 772.0 | 6.7 | −8.1 |
| 14 | 843 [D] | 60.2 | 835.8 | 4.9 | −9.3 |
| 15 | 896 [D] | 59.7 | 900.8 | 4.0 | −9.6 |
| 16 | 955 [D] | 59.7 | 961.7 | 3.7 | −10.3 |

Minima (measured [BC13]): 71.8, 140.0, 201.5, 259.9, 322.9, 388.8, 446.4 Hz.
**The extracted bore + the existing TMM is within −1.2 … +1.2 % of the measured peaks 1–16** [D]. That is the validation target for the
waveguide (horns.md §9.3), and the profile needs no fitting. At pajupilli's c = 349.5 m/s all frequencies scale ×1.018 (peak 2 → 113.9 Hz).
The B♭ series is f₀ = 58.27 Hz; peaks 3–8 sit at ≈ 58–59 Hz per n, so the instrument is in tune with warm breath plus a
slightly pushed-in tuning slide.

### 3.2 UNSW data set (Henri Boutin): Yamaha trombone, dry air 26.3 °C [UNSWZ]

The spreadsheet gives |Z|, phase, 50–1800 Hz, 0.67 Hz steps. It is the impedance of the trombone in the lipping/warming
studies (probably the YBL-321 of [BFSW15] **[U]**). My peak-picking **[D]** (no data below 50 Hz, so no first peak):

| n | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| f (Hz) | 113.1 | 171.0 | 228.3 | 288.2 | 344.1 | 402.2 | 455.1 | 515.8 | 583.1 | 644.2 | 695.0 | 756.0 |
| \|Z\| (MΩ) | 24.0 | 20.1 | 21.1 | 23.3 | 19.4 | 12.7 | 12.4 | 10.1 | 6.3 | 4.5 | 4.8 | 3.7 |

Minima (MΩ): 0.39 (72.7 Hz), 0.62 (139), 0.77 (197), 1.11 (256), 1.55 (320), 1.69 (384), 1.71 (436). Above ~800 Hz the peaks are
< 3 MΩ and ragged: the bell cut-off (UNSW: the trombone "functions as a megaphone rather than a resonator above about 700 or 800 Hz" [UNSW]).
Peak/trough ratio: ~60 at n = 2, ~11 at n = 7. This instrument is flatter per n (f/n 56.6–58.3) than the 8H. Smaller
bore, a different mouthpiece.

### 3.3 Bass trombone, Courtois AC502 [MGV22] (vector figure read-out [D])

| n | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| f (Hz) | 39.5 | 115.5 | 173.3 | 233.4 | 294.1 | 346.1 | 407.2 | 464.8 | 521.4 | 586.7 | 645 | 703 |
| \|Z\| (MΩ) | 48.9 | 34.6 | 30.2 | 29.2 | 34.6 | 33.1 | 19.1 | 18.7 | 14.7 | 7.2 | 7.1 | 6.1 |

(y axis 10⁵…10⁸ kg m⁻⁴ s⁻¹, log; readout ±2 %.) Lower losses than the tenors (wider bore) shows up as higher peaks.

### 3.4 The pedal and the "false" first resonance

- Peak 1 (38.3 Hz, ~ E♭1 − 25 cents) is **~ −740 cents** from B♭1 (58.27 Hz) [D]. It is almost unplayable as a note: its harmonics hit no
  resonances [BSW20].
- The **pedal B♭1** (58.3 Hz) has no resonance of its own. Its harmonics 2, 3, 4… lock to peaks 2, 3, 4 (113–233 Hz)
  [UNSW, BSW20]. In the one-mass model the pedal comes out **about a semitone lower** than linear stability predicts, at
  60.6 Hz in their bass trombone model (B♭2 regime at 120.4 Hz) [MGV22]. Trombones have **no ghost note** (the euphonium does) [MGV22].
- Pedals exist in every position (B♭1 … E1 in 7th), and with the F trigger lower still. The waveguide plays them if the lip's
  f_l is set near the pedal and the pressure is low. Expect a fragile, buzzy tone; it is a calibration case of its own.

---

## 4. Positions chart, E2–F5 (concert pitch; tenor trombone without trigger)

Harmonic partial n of the position's series (position p lowers by p−1 semitones). "♯" = raised (shortened), "♭" = lowered.
The ET offsets of the *ideal* harmonic series are n = 5: −14 c, n = 7: −31 c, n = 9: +4, n = 10: −14, n = 11: −49 [D]. On real
instruments the bore shifts them (§2.2, §3.1). Standard and alternates follow common teaching charts and [K91] **[U]**.
T = with F trigger.

| Note | Hz (A440) | Standard pos. / partial | Alternates (pos./partial) | Remarks |
|---|---|---|---|---|
| E2 | 82.4 | 7 / 2 | T2 / 2 | 7th low E; T2 on F tenors |
| F2 | 87.3 | 6 / 2 | T1 / 2 | |
| F♯2 | 92.5 | 5 / 2 | — | the one note with no alternate [K91] |
| G2 | 98.0 | 4 / 2 | — | |
| A♭2 | 103.8 | 3 / 2 | — | |
| A2 | 110.0 | 2 / 2 | — | |
| B♭2 | 116.5 | 1 / 2 | — | played ~1 % above peak 2 [BFSW15] |
| B2 | 123.5 | 7 / 3 | T2 / 3 (= ♯T2) | |
| C3 | 130.8 | 6 / 3 | T1 / 3 | |
| C♯3 | 138.6 | 5 / 3 | — | |
| D3 | 146.8 | 4 / 3 | — | |
| E♭3 | 155.6 | 3 / 3 | — | |
| E3 | 164.8 | 2 / 3 | 7 / 4 | |
| F3 | 174.6 | 1 / 3 | 6 / 4 | |
| F♯3 | 185.0 | 5 / 4 | — | |
| G3 | 196.0 | 4 / 4 | — | |
| A♭3 | 207.7 | 3 / 4 | 7 / 5 (♯7) | |
| A3 | 220.0 | 2 / 4 | 6 / 5 (♯6) | |
| B♭3 | 233.1 | 1 / 4 | 5 / 5 (♯5) | ♯5 used in E♭ arpeggios [K91] |
| B3 | 246.9 | 4 / 5 | 7 / 6 | |
| C4 | 261.6 | 3 / 5 | 6 / 6 | |
| C♯4 | 277.2 | 2 / 5 | 5 / 6 | |
| D4 | 293.7 | 1 / 5 | 4 / 6 | 1st is the standard; 4th for slide economy ([UNSW] uses it as an exercise) |
| E♭4 | 311.1 | 3 / 6 | 6 / 7 (♯6) | |
| E4 | 329.6 | 2 / 6 | 5 / 7 (♯5), 7 / 8 | |
| F4 | 349.2 | 1 / 6 | 4 / 7 (♯4), 6 / 8 | ♯4 for endurance [K91] |
| F♯4 | 370.0 | 5 / 8 | 3 / 7 (♯3) | |
| G4 | 392.0 | 4 / 8 | 2 / 7 (♯2) | many charts give ♯2 as standard |
| A♭4 | 415.3 | 3 / 8 | — | 7th partial in 1st is ~30 c flat: "A♭4 naturally a bit flat" [MGV22] |
| A4 | 440.0 | 2 / 8 | 6 / 10, 4 / 9 | |
| B♭4 | 466.2 | 1 / 8 | 5 / 10, 3 / 9 | |
| B4 | 493.9 | 4 / 10 | 2 / 9 | |
| C5 | 523.3 | 3 / 10 | 1 / 9 (slightly sharp) | |
| C♯5 | 554.4 | 2 / 10 | 5 / 12 | |
| D5 | 587.3 | 1 / 10 | 4 / 12 | |
| E♭5 | 622.3 | 3 / 12 | — | (n = 11 is ~ ½ semitone off; avoided) |
| E5 | 659.3 | 2 / 12 | — | |
| F5 | 698.5 | 1 / 12 | 4 / 14 (♯4) | above ~700 Hz the resonances are weak (§3.2); range ends F5–B♭5 for most players |

Below E2: the F trigger (E♭2–C2) or pedals (B♭1 … E1, plus "false tones" between them by lipping). For the model: the table maps
key → (position, partial, trigger). The calibration then finds the per-note **slide length** and f_l, as for the horn's valves/partials.

---

## 5. Lips and mouthpiece

### 5.1 Mouthpiece (from §1.1 and the maker's figures)

| Quantity | Value | Status |
|---|---|---|
| Rim inner Ø / rim width | 25.6–25.7 mm / 6.6 mm (DW 5AL) | [BC13 D], [DW] |
| Cup depth (rim plane → throat) | ≈ 27–29 mm | [D] |
| Cup volume, rim plane → throat entry (26.9 mm) | **9.6 cm³** | [D] (integrated from §1.1) |
| Volume displaced by the lips in the cup | 0.04–0.21 cm³ *oscillating amplitude* (F4 / B♭2); the static intrusion is larger, ~1–2 cm³ **[G]** | [BFSW15] |
| Throat Ø / length | 7.2–7.3 mm / ~5 mm (DW 5AL); Bach 6½AL throat 6.6 mm (0.261 in) | [D], [DW], [Bach] |
| Backbore | 7.7 → 10.6 mm over 45 mm, then the shank (12.7 mm at 79.5 mm) | [D] |
| **Popping frequency** (cup + throat + backbore on a non-reflecting 12.7 mm pipe, TMM) | **≈ 520 Hz** (full 9.6 cm³); 600 Hz with 8 cm³, 700 Hz with 6 cm³ | [D] |

So horns.md's guess (500–600 Hz) holds. In the lumped-cup scheme put **V ≈ 8.5–9.5 cm³** in cell 0 (the lips take some) and the throat
as `setSeries` (7.2 mm over ~5 mm, then the backbore over ~5 cells).

### 5.2 Lips (trombone-specific measurements)

| Quantity | Value | Source |
|---|---|---|
| Model set used for trombone (one-mass, outward) | h₀ = 0.5 mm (or H = k/f_l, k = 0.074 m·Hz), w = 12 mm, Q = 7, 1/μ = 0.11 m²/kg | [MGV22] Table 1 after [VVG17] |
| Effective lip width (from the closed-phase flow fit) | **13 mm** (= 0.56 × inner mouthpiece width) | [BSW20] |
| Aperture width / vertical opening ratio | 11–15 mm (B♭2), 11–13 mm (F4) | [BFSW15] |
| Sweeping flow | 180 cm³/s p-p of a total 400 cm³/s p-p at B♭2. Much smaller at F4 | [BFSW15] |
| Lip motion | B♭2: longitudinal ≈ 0.4–0.6 × vertical (swinging-door). F4: mostly vertical (0.26–0.35) | [BFSW15] |
| Playing frequency vs bore peak | **+1.1 %** on average. Range ≈ +½ semitone / −1 tone, limits where \|Z\| ≈ 15 % of the peak | [BFSW15, BSW20] |
| Upper lip leads | in 7 of 9 subjects (B♭2), 68 % of notes | [BFSW15, BSW20] |
| Vocal-tract impedance | peaks at ~650 Hz and ~900 Hz (two groups, σ ≈ 50 Hz); upstream \|Z\| < 20 % of the bore's at F3/B♭3 in normal play, up to 40 % when lipping far | [BFSW15, BSW20] |
| Breath | peaks drop 0.48–0.62 % in 3–10 s of play (CO₂); +1.2–2.6 c/°C | [BFSW13, BSW-warm] |

Two-mode lips: [BFSW15] concludes the motion fits **two-degree-of-freedom** models with varying effective length. At B♭3 and up
the motion is mainly transverse ([BFSW13], horns.md §1.1). The horn model's weak inward second mode is therefore well motivated for the trombone above ~F3.

### 5.3 Mouth pressure

| Note / dynamic | Value | Status |
|---|---|---|
| B♭2, normal (mf) | **1.4 kPa ± 10 %** [BFSW15]; 1.3 kPa [BSW20] | measured |
| Threshold (K f, K = 4–8 Pa/Hz, trumpet value) | B♭2: 0.5–0.9 kPa; F3: 0.7–1.4; B♭3: 0.9–1.9; F4: 1.4–2.8; B♭4: 1.9–3.7 kPa | [D from FT99] |
| pp … ff | ≈ 1.1 … 3.5 × threshold; ff low register 3–5 kPa, ff high register 8–12 kPa | **[G]** |
| Ceiling | ~ 10–12 kPa (trumpeters reach 25 kPa; trombone not found) | **[G]** |

---

## 6. Brassiness

What horns.md §6 already covers (physics, B, the N1 out-of-loop warp) is not repeated. New here:
- **B for the 8H [D]:** with §1.1 from the receiver (79.5 mm) to the mouth, D₀ = 12.72 mm and L_ecl = 2969 mm (B♭ at 346 m/s, as in
  [CCM14]), B = Σ l·2D₀/(D_n+D_{n+1}) / L_ecl = **0.71**. That is the published "tenor and bass trombones 0.70" [MPGC10]. Other
  measured: Huschauer 1794 tenor 0.81, Courtois bass 2000 **0.67**; sackbuts/trombones span 0.6–0.82 [CCM14]. **The slide
  dominates**: in [CCM14]'s worked example the two inner-slide sections contribute 118 of the 219 total of Σ 2l/(D_n+D_{n+1}) (54 %).
- **The slide position changes B [D]:** in 7th, 1167 mm more cylinder (13.6 mm) adds 1167·12.72/13.6 ≈ 1090 mm to the 2101 mm sum.
  L_ecl grows to 2969·2^{6/12} = 4199 mm, so **B(7th) ≈ 0.76** vs 0.71 in 1st. Long positions are a little brassier. N1 should take
  L_nl from the current cylindrical length.
- **Shock waves** at ff in the slide, seen by schlieren and pressure measurements [HGMW96] (dp/dt numbers not obtained **[U]**).
  Msallam & Dubois [MD10] model the slide as a ~1.5 m, 7 mm-radius pipe and get the **4th harmonic +5 dB** after propagation.
  The CFA 2018 review [GH18] stresses the radiated pressure ∝ d(U_bell)/dt. Steepened bore pressure → near-Dirac spikes in the radiated
  sound. That is why N1 must differentiate (horns.md §6.2).
- The full model (Msallam, Dequidt, Caussé, Tassart 2000 [MDCT00]) is a trombone with nonlinear propagation for loud tones (not read).

Cheap model, specific to the trombone **[G]:** N1 with L_nl = (cylindrical length from the throat to x ≈ 1930 mm, incl. slide extension) and
β = 1.2. Apply on the forward wave at x ≈ 1930 mm (end of cylinder, start of the bell cone), output (warped − unwarped) differentiated
plus the linear bell output. Scale by (p⁺/ρc)²-ish automatically via the warp. pp–mf: nil; ff: the brass "blat".

---

## 7. Playing practice for a keyboard model

### 7.1 Legato

- **Natural (lip) slurs** across partials in one position, no tongue: F3 ↔ B♭3 ↔ D4 ↔ F4 in 1st [MGV22 Fig. 5 lists the natural notes].
  The model: change f_l only (horns.md §7.3).
- **Slide legato needs the tongue** whenever the slide moves *within one partial*: B♭3 (1) → A3 (2), C4 (3) → D4 (1), etc. Without the tongue it is an
  audible glissando. Players use a **soft "legato tongue" (da/la/du)**: "the tongue merely interrupts the air flow as lightly as possible
  while the slide moves as quickly as possible" [PB].
- **Cross-grain slurs** (the pitch moves opposite to the slide, which forces a partial change): e.g. B♭3 (1) → C4 (3), F3 (1) → G3 (4).
  The lips break cleanly to the new partial while the slide moves, and many players slur these without the tongue **[U, standard pedagogy]**.
  Kagarice: choose positions as close as possible, and avoid direction changes of more than one position in legato [K91].
- **Rule for the model [G]:**
  1. Same position: lip slur (f_l ramp, 5–20 ms).
  2. Partial changes and the slide moves opposite to the pitch: slur, but move the slide fast (≤ 40 ms) and switch f_l at mid-move.
  3. Same partial (or slide and pitch move the same way): **legato tongue**. Dip the mouth pressure to ~30–50 % (or close the
     tongue valve partially) for **15–30 ms**, centred on a **30–60 ms** slide move. Unless the player asks for a glissando
     (a control, e.g. portamento switch).
  4. Choose the alternate position that minimises slide travel and direction changes (§4 table), as players do [K91].

### 7.2 Glissando

- A true glissando spans at most the **tritone** between 1st and 7th within one partial (B♭3 → E3 down, E3 → B♭3 up). Longer ones
  (the "Bolero"/"Firebird"/jazz smears) break across partials. Players either hide the break (re-tongue softly in mid-slide) or accept it **[U]**.
- The model: the slide length ramps continuously (§2.5). Keep f_l tracking the current target resonance n·f(L)·(1 − ~3 %)
  (horns.md §1.5), so the regime does not jump.

### 7.3 Slide vibrato

No measured trombone slide-vibrato rate/depth found (an Illinois student report has one trombone example without numbers [ILL]).
**[G]:** rate 5–6.5 Hz; depth ±10–25 cents (classical British/band, ballad jazz), ±30–50 cents in "sweet" dance-band style.
With §2.2's ~90 mm per semitone, ±20 cents ≈ **±15–20 mm** of slide travel. Lip/jaw vibrato modulates f_l (smaller depth, ±5–15 cents). Many
orchestral players use little or none **[U]**. Implement it as slide modulation (a sine on the travel), with the same calibrated map, so
higher partials vibrate by the same cents.

### 7.4 Other

- **Tonguing** and attacks: as in horns.md §7.3. Trombone attacks at ff are spat out, with the shock-wave brightening [BLVC17 via horns.md].
- **Tuning:** the tuning slide is "18 mm" out in the UNSW studies [BSW20]. A model's global pitch offset is a tuning-slide length.
- **Mutes:** straight, cup, Harmon, plunger. Same bell-end machinery as the horn's hand (horns.md §4.3).

---

## 8. Starting values (one table)

| Parameter | Tenor (8H-like) | Bass (AC502-like) | Status |
|---|---|---|---|
| Bore | §1.1 list | §1.3 list | [D] / [G] |
| Slide straight legs (1st) | 600 + 675 mm | same | [D] |
| Travel per position (from 1st) | 0, 85, 174, 269, 370, 474, 583 mm | same **[G]** | [D] |
| F loop / G♭ loop | 966 mm / — | 981 / 770 mm | [D] (TMM fit) |
| fs, cells | 32 kHz: 244 open, 435 with 7th + F | 32 kHz: ~500 worst case | [D] |
| Cup V / throat | 9.6 cm³ (use 8.5–9.5) / 7.2 mm | 11–13 cm³ / 7.5–8 mm | [D] / [G] |
| Popping | ≈ 520 Hz | ≈ 400–450 Hz | [D] / [G] |
| Lips μ, Q, w, H | 9 kg/m² (1/0.11), 7 (humans lower: 5), 12–13 mm, 0.074/f_l | same, w 14 mm | [MGV22, BSW20], bass [G] |
| p_m mf at B♭2 | 1.3–1.4 kPa | ~1.2 kPa | [BFSW15, BSW20] / [G] |
| Brassiness B | 0.71 | 0.67 | [D] / [CCM14] |
| Validation | peaks 1–16 within ~1 % of §3.1 | §3.3 | — |

---

## References

[Bach] Retail listing of the Bach 6½AL mouthpiece (throat 0.261 in). https://haggertysmusic.com/products/bach-trombone-mouthpiece-6-5al **[U]**
[BC13] S. Bilbao, J. Chick, "Finite difference time domain simulation for the brass instrument bore", JASA 134(5), 3860–3871 (2013). Preprint (read: §IV instruments, Table I, Figs. 7 and 9 as vector data): https://www.ness.music.ed.ac.uk/wp-content/uploads/2013/08/bilbao_jasa_13_r2_double.pdf
[BFSW13] H. Boutin, N. Fletcher, J. Smith, J. Wolfe, SMAC 2013 (see horns.md). https://newt.phys.unsw.edu.au/jw/reprints/SMACtrombone.pdf
[BFSW15] H. Boutin, N. Fletcher, J. Smith, J. Wolfe, "Relationships between pressure, flow, lip motion, and upstream and downstream impedances for the trombone", JASA 137(3), 1195–1209 (2015), doi:10.1121/1.4908236. https://newt.phys.unsw.edu.au/jw/reprints/Trombone1.pdf
[BSW20] H. Boutin, J. Smith, J. Wolfe, "Trombone lip mechanics with inertive and compliant loads ('lipping up and down')", JASA 147(6), 4133–4144 (2020). https://newt.phys.unsw.edu.au/jw/reprints/lipping.pdf
[BSW-warm] H. Boutin, J. Smith, J. Wolfe, "Warming up a trombone" (UNSW reprint, 2020). https://newt.phys.unsw.edu.au/jw/reprints/warming.pdf
[CCM14] D. M. Campbell, J. Chick, A. Myers, "Is the sackbut merely a narrow-bore trombone?" (title per the paper's footnote), Historic Brass Society Journal 26 (2014), 63–78; appendix: Huschauer tenor bore table, B = 0.81; table of B incl. Courtois bass trombone 0.67. https://www.historicbrass.org/images/hbj/hbj-2014/HBSJ_2014_JL01_005_Myers.pdf
[DW] Denis Wick 5AL mouthpiece specifications (cup 25.73 mm, rim 6.64 mm, throat 7.30 mm), retailer listings. https://www.musiciansfriend.com/brass-instruments/denis-wick-dw5880-classic-series-trombone-mouthpiece-in-silver/469700000000221 ; https://www.long-mcquade.com/53292/Band/Brass_Mouthpieces/Denis_Wick/5AL_Heritage_Trombone_Mouthpiece.htm **[U]**
[FT99] N. H. Fletcher, A. Tarnopolsky, JASA 105(2), 874–881 (1999) (see horns.md). https://www.phys.unsw.edu.au/music/people/publications/Fletcheretal1999.pdf
[GH18] J. Gilbert, A. Hirschberg et al., review in honour of Bram Wijnands, Proc. CFA 2018, Le Havre, §3.2 "From shock waves to brassy sounds". https://www.conforg.fr/cfa2018/output_directory2/data/articles/000044.pdf
[HGMW96] A. Hirschberg, J. Gilbert, R. Msallam, A. P. J. Wijnands, "Shock waves in trombones", JASA 99(3), 1754–1758 (1996). (abstract/secondary only)
[IEM] "It's Easy To Make Music" (older British text; 7th position ≈ 23 in from closed; text partly garbled). https://www.traditionalmusic.co.uk/its-easy-to-make-music/make-music-0128.htm **[U]**
[ILL] A. DiCristofano, A. Manaster, Physics 406 report (UIUC, 2016), trombone mouth and slide vibrato plots. https://courses.physics.illinois.edu/phys406/sp2017/Student_Projects/Spring16/A_DiCristofano_A_Manaster_Physics_406_Final_Report_Sp16.pdf
[K91] V. Kagarice, "Advantages of alternate slide positions", Bandworld 6(3), 34 (1991). https://bandworld.org/pdfs/AlternateTbnKagarice.pdf
[MD10] R. Msallam, F. Dubois, "Mathematical model for coupling a quasi-unidimensional perfect flow with an acoustic boundary layer", arXiv:1006.2991 (2010), §8.4 trombone. https://arxiv.org/pdf/1006.2991
[MDCT00] R. Msallam, S. Dequidt, R. Caussé, S. Tassart, Acta Acustica u. A. 86, 725–736 (2000). (not read)
[MGV22] R. Mattéoli et al., arXiv:2112.08751 (see horns.md). Fig. 2 (Courtois AC502 impedance, read from the vector figure), Table 1 (lips), §4 (pedal note, natural notes). https://arxiv.org/pdf/2112.08751
[MPGC10] A. Myers, R. W. Pyle, J. Gilbert, D. M. Campbell, Proc. Second Vienna Talk (2010) (see horns.md). https://viennatalk.mdw.ac.at/papers/Pap_01_85_Myers.pdf
[PB] "The Trombone", in *Brass Techniques and Pedagogy* (PALNI Pressbooks). https://pressbooks.palni.org/brasstechniquesandpedagogy/chapter/the-trombone/
[UNSW] J. Wolfe et al., "Brass instrument (lip reed) acoustics: an introduction". https://newt.phys.unsw.edu.au/jw/brassacoustics.html
[UNSWZ] H. Boutin, trombone input impedance, dry air, 26.3 °C (spreadsheet and plot), linked from [UNSW]. https://www.phys.unsw.edu.au/music/brass/ZTbn_dry_26_3degC.xls ; https://www.phys.unsw.edu.au/music/brass/ZTbn_Dry_26_3degC.pdf
[VSL] Vienna Symphonic Library, "Tenor trombone" (bore 12.6–13.9 mm, bell 20.5–22 cm). https://www.vsl.co.at/academy/brass/tenor-trombone **[U]**
[VVG17] L. Velut et al., Acta Acustica u. A. 103(1), 132–148 (2017) (see horns.md).

Not obtained: Braden's thesis (Edinburgh 2006, "Bore optimisation and impedance modelling of brass musical instruments") and
Braden, Newton & Campbell, JASA 125, 2404–2412 (2009): the CORE copy was blocked. It would give more trombone bores. Also not
obtained: Hirschberg et al. 1996 full text (dp/dt), Msallam et al. 2000, Pyle's/Kausel's trombone data, measured slide-vibrato
statistics, and a bass trombone bore profile.

How the [D] numbers were made (reproducible): the preprint's page 7/8 (and arXiv 2112.08751 page 4) were converted with `pdftocairo -svg`, and
the curve polylines were mapped through the axis tick paths. The impedance, slide and loop fits imported `tools/brass.py` (`Zin`, `peaks`,
`fit_loops`) with c = 343.4 m/s, the §1.1 profile from the throat entry (26.9 mm) and the cup 0–26.9 mm lumped. The slide extension
was inserted at x = 389.5 mm (mid down-leg). The scripts are scratch files, not part of the repo.
