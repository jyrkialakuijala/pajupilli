# A cello, derived from the bowed double bass: research

Status: research only (2026-11-18). Nothing here is implemented. The cello is to be built from the
existing bass (`src/bass.*`, `src/bassbody.*`, `Bass::bow`, `BowPlayer`; `double_bass.md` 7e).

How each number is marked:
- **[T]** read in the primary text (downloaded and read for this document).
- **[S]** from a secondary source, an abstract or a search snippet: the primary text was not read.
- **[E]** my estimate or arithmetic from cited values; not a published value.
- **[M]** from general knowledge, not looked up: check before relying on it.

References are short keys in brackets; the list with URLs is section 9. What could not be found is
collected in section 8. Meyer's book (*Acoustics and the Performance of Music*) could not be opened
anywhere: every Meyer number here is second-hand and marked [S].

Short answer. The cello is not a bass scaled by one factor. Strings and tuning scale one way
(length x0.65, pitch x1.59 to x2.25 per string, wave resistance x0.4), the body another (modes at
about x1.55 of the bass's), the bridge a third (a different shape: long legs, a resonance at
1–1.5 kHz and a second hill near 2–2.3 kHz), and the string couples to the body more strongly than
on either the violin or the bass (the wolf at E3–F#3). The player differs most: fifths, thumb
position, a faster and narrower vibrato, far more agile bowing.

---

## 1. The instrument

### 1.1 Strings

- **Tuning:** C2 65.41 Hz, G2 98.00 Hz, D3 146.83 Hz, A3 220.00 Hz, in fifths. Sounding length
  690 mm (Pirastro's reference), 695 mm [Johannsson], 700 mm (Thomastik's reference) **[T]**
  [KnutStr, Aitchison, Johannsson].
- **Range for the model:** C2 to about A5–C6 in ordinary playing **[M]**; the TU Berlin database
  recorded a modern cello from C2 to G6 **[T]** [Shabtai17]. MIDI 36–84 (to 91 with harmonics).
  GM program 43 (Cello).
- **One measured modern set**, standard Kaplan Solutions, L = 690 mm **[T]** [Mansour1]:

| | A3 | D3 | G2 | C2 |
|---|---|---|---|---|
| Tension T (N) | 171 | 135.9 | 135.5 | 131.5 |
| Mass per length (g/m) | 1.85 | 3.31 | 7.40 | 16.14 |
| Wave resistance Z = √(Tμ) (kg/s) | 0.56 | 0.67 | 1.00 | 1.46 |
| Bending stiffness EI (10⁻⁴ N m²) | 3.26 | 2.48 | 1.88 | 6.20 |
| Inharmonicity B (10⁻⁶), f_n = n f₀ √(1 + B n²) | 39.5 | 37.9 | 28.7 | 97.8 |
| η_F (10⁻⁵) | 22 | 23 | 20 | 12 |
| η_B (10⁻²) | 11.4 | 12.5 | 13 | 4.7 |
| η_A (1/s) | 0.12 | 0.11 | 0.04 | 0.07 |

- **Ranges over brands** (69.0 cm) **[T]** [KnutStr]: A3 1.50–1.92 g/m, 138–177 N, Z 0.46–0.58;
  D3 2.94–3.57 g/m, 121–147 N, Z 0.60–0.73; G2 6.38–7.56 g/m, 117–138 N, Z 0.86–1.02;
  C2 14.33–16.98 g/m, 117–138 N, Z 1.29–1.53 kg/s. Sum of the four tensions 493–601 N.
- **Four G strings measured on a monochord** (L = 70 cm) **[T]** [Lampis24]: T 116–154 N,
  μ 6.2–8.2 g/m, Z 0.85–1.13 kg/s, EI 3.0–6.9 x 10⁻⁴ N m², diameter 0.88–1.15 mm, mean Q over the
  first 30 modes 419–1064. EI of one string fell 29 % after remounting: stiffness is not a constant.
- **Makers' tensions:** Larsen Magnacore medium A 180 N, D 140, G 137, C 134 N **[T]** [Larsen];
  Dominant A 137 / D 111 / G 102 / C 100 N, Jargar A 166, D 141 N, Spirocore G 137, C 132 N **[S]**
  [Aitchison]. Baroque gut at a = 415, 68 cm: A 113–133 N, D 81–93 N, G and C 78–85 N; plain gut
  1.2 / 1.55 / 2.25 / 3.35 mm thick **[S]** [Gamut].
- **Inharmonicity in cents [E]** (865.6 · B · n² cents for small B n²): A, D, G strings: partial 10
  about +3 c, partial 20 about +12–14 c; C string: partial 10 +8 c, partial 20 +34 c. Bowed, the
  partials are locked harmonic (the stick-slip resets their phases every period); the stiffness
  shows only in pizzicato and as a rounder Helmholtz corner **[T]** [KnutStiff].
- **Damping** **[T]** [Mansour1, Mansour2]: Q_n = [T + EI (nπ/L)²] / [T (η_F + η_A/ω_n) +
  EI η_B (nπ/L)²]. Q is highest at the 2nd or 3rd mode (1200–3000), falls towards 1/η_B (about
  8–20) at very high modes; the pattern against mode number is nearly the same for all four
  strings. A simulation floor of Q = 150 is not reached before mode 25. **A stopped string: triple
  η_F** and leave the other two (the finger's damping).
- **Torsion** **[T]** [Mansour2, Galluzzo03, Serafin99]: open D3: torsional fundamental 758 Hz
  (wave speed about 1046 m/s **[E]**, 5.2 times the transverse speed), torsional wave resistance
  1.8 kg/s, Q 34 (steel core), 45 (nylon), 20 (gut). The share of the bow's force that goes into
  twist: Z_T/(Z_T + Z_R) = 0.234 for a Dominant D.
- **Not found:** measured decay of a stopped against an open cello string in dB/s; Woodhouse &
  Loach 1999 in the original; Pickering's cello data.

### 1.2 The body's dimensions

| | Cello | Violin | Source |
|---|---|---|---|
| Body (cavity) length | 75–76 cm (Amati 79, Brescian 71) | 35 cm | **[T]** [SSI14] |
| Upper / C / lower bout | 34 / 24 / 43 cm | 16 / 10.5 / 20 cm | **[T]** [SSI14] |
| Rib height | 12 cm (111–121 mm on instruments) | 3.5 cm | **[T]** [SSI14], **[S]** |
| f-hole area (table 14.1; whether one or both is not stated) | 42.5 cm² | 9.25 cm² | **[T]** [SSI14] |
| Overall length | 124 cm | 59–60 cm | **[T]** [FR91] |

Length and width are about 2.1 times the violin's, the ribs 3.4 times, the tuning 3 times lower: a
violin scaled to the cello's tuning would be 3 times as long and 27 times as heavy **[T]** [FR91].

A luthier's table for a 4/4 cello, mm **[T, a web table marked "being revised"]** [Johannsson]:
- Arching: back 28, top 29. Ribs 1.7 thick.
- Top thickness 4.7 at the post, 3.9 in the flanks, 4.0 in the bouts, 5.5 at the edge. Back 7.0 at
  the centre, 3.5 flanks, 3.6 bouts, 5.0 edge. (4.4–4.5 mm tops in [FR91] and [Blanford18] **[T]**.)
- Bass bar about 580 x 11 x 24 (height at the middle). Soundpost 11 mm in diameter.
- f-holes 130 long, 98 between the upper holes. Stop 405, neck 280, fingerboard 580 long.
- String spacing 47 at the bridge, 23 at the nut. String angle over the bridge 153°.

Estimates and gaps:
- **Air volume: about 30 litres [E]** (plan area about 0.22 m² x 0.12 m ribs plus arching). With
  the rigid-wall Helmholtz frequency of 104 Hz [SSI14] this is consistent with 42.5–85 cm² of
  f-hole and an effective neck of 3–5 cm. No published litre figure was found.
- **Total mass:** about 2.5–3.5 kg **[M]**; not found in a citable source. Plate masses, soundpost
  length and string afterlength: not found.
- **Free plates:** mode 2 at 60–65 Hz and mode 5 at 120–130 Hz for cello plates (violin 165–185
  and 330–370 Hz) **[T]** [FR91, after Hutchins].

### 1.3 The bridge

- Mass about 19 g. Belgian model: 90 mm across the feet, 95 mm high. French: 92 mm wide, 91 mm
  high, legs about half the height **[T]** [Zhang16]. Feet 90 mm, thickness 11 mm, height about
  92 mm **[T]** [Johannsson]. (The bass bridge: about 160 mm and 100 g or more; violin 2 g.)
- **Its own resonances, feet clamped:** "sway" on the long legs about 1.5 kHz (Reinicke's rigid-
  support value about 1 kHz), "bend" about 2.2 kHz, "bounce" about 3.1 kHz **[T]** [Euph5.3, FR91].
  Steinkopf: a major resonance at 1000–1500 Hz, spring-like below it **[T]** [FR91]. The violin's
  two are near 3 and 6 kHz: its first is a rocking of the top, the cello's a bending of the legs
  ("a Dachshund and a Great Dane", Cremer).
- **On the instrument:** first bridge resonance 1200 Hz (Belgian, lowered by built-in sensors),
  1350 Hz (Belgian), 1500 Hz (French) on three cellos **[T]** [Zhang16]. The bridge admittance of
  two cellos shows **two hills, about 1 kHz and 2–2.3 kHz** **[T]** [Euph5.3]; a broad hump
  1–2.5 kHz (violin 2–3 kHz, bass 500–1000 Hz) **[T]** [SSI15]; one nineteenth-century cello peaks
  just above 2 kHz, higher than any signature mode **[T]** [SSI17].
- **Below the hill** the bridge is nearly rigid in its plane and rocks: about the soundpost-side
  foot for 65–205 Hz, about a point towards the bass-bar side for 205–466 Hz, about mid-bridge for
  466–1200 Hz **[T]** [Zhang16]. Force to the two feet is equal and opposite up to about 1 kHz
  **[T]** [FR91, Reinicke]. Bladier: a gain of about 2 (6 dB) from 66 to 660 Hz, 1 or less above
  **[T]** [FR91].
- The string sees the top through a lever: mobility at the notch scales as (bridge height / foot
  spacing)² **[T]** [SSI13]. The cello bridge is 2.4 times the violin's lever; with the smaller
  impedance mismatch Cremer estimates wolves 3.4 times easier than on the violin **[T]** [FR91].
- **Not found:** how the hill moves with bridge mass; a controlled French–Belgian comparison.

---

## 2. Body resonances

### 2.1 Signature modes

Names differ by author: C2 = T1 = B1−; C3 = B1+; C1 = CBR-like torsion; B1 of Bynum = whole-body
bending (others' B0/B−1).

| Mode | What moves | Bynum 1997 (school cello) | Other cellos | Ratio to violin |
|---|---|---|---|---|
| Body bending | Neck and body bend like a beam | 57 Hz | | 0.33 |
| **A0** | Air through the f-holes against the plates | 102 Hz | 82 (Eggers), 85 (Langhoff), 90 (Askenfelt), 90 / 104 (Firth), 97 (Zhang) | 0.36 |
| **C2 / T1 / B1−** | Top moves strongly; strong radiator | 144 Hz | 132, 146, 168, 165 / 185 | 0.32 |
| C1 | Torsion | 170 Hz | 186, 163 | 0.42 |
| C4 | Symmetric; prominent at the bridge, radiates poorly | 195 Hz | 202 | 0.29 |
| **A1** | Air sloshing between the bouts; little from the f-holes | 203 Hz | | 0.43 |
| **C3 / B1+** | Strong radiator | 219 Hz | 218, 185 | 0.39 |
| A3, C5, A2, C6 | | 277, 297, 302, 336 Hz | A3 260, A2 312 | 0.25–0.37 |

All **[T]** [SSI14, tables 14.2 and 14.3].

- **Typical values** **[T]** [SSI14]: A0 90–100 Hz, close to the open G (98 Hz); C2 about 140 Hz;
  C4 about 195; A1 about 200; C3 about 220; A2 about 300 Hz. **C2, C3 and C4 cluster near 200 Hz
  in the cello; in the violin they spread over nearly an octave.** Caldersmith: C2 and C3 often
  blend into one broad peak at 140–190 Hz; higher arching (32 against 26–28 mm), a lighter bass bar
  and thinner edges spread them to 165–200 Hz and weaken the wolf.
- **One played cello** (student instrument) **[T]** [Zhang16]: A0 about 97 Hz; the strong "main
  body resonance" at 173 Hz (its wolf); small peaks at 199 and 209 Hz; a prominent peak at 281 Hz.
- **Old Italians [S]** [Schleske]: Tecchler 1719: 156 and 173 Hz; Stradivari "Lott": 204 and
  223 Hz. Brown's table: cello A0 102, T1 203 Hz **[T]** (`double_bass.md`).
- **Air modes of the rigid cavity** **[T]** [SSI14]: Helmholtz 104 Hz; (0,1) 226; (1,0) 496; (0,2)
  547; (1,1) 609; (0,3) 742; (1,2) 820; (2,0) 893; (1,3) 980; (0,4) 1016 Hz. A0 and A1 are about an
  octave apart in at least three cellos.
- **Q, damping and effective mass of cello modes: not found** in any text that could be opened.
  One indirect figure **[S]**: a 196 Hz mode with damping ratio 0.7 % (Q about 70) against about
  2 % (Q about 25) for other peaks [Neubauer18, snippet]. Mode density and modal overlap: not found.

### 2.2 Violin, cello, bass: what scales and what does not

| | Violin | Cello | Double bass | Cello / bass |
|---|---|---|---|---|
| Lowest string | G3 196 Hz | C2 65.4 Hz | E1 41.2 Hz | 1.59 |
| A0 | 260–290 Hz [Viala] | 82–104 Hz | 58–68 Hz [SSI15] | about 1.5 |
| A0 / lowest string | 1.4 | 1.25–1.6 | 1.4–1.65 | |
| Main top mode (B1− / T1) | 430–480 Hz | 132–185 Hz | 82–114 Hz | about 1.6 |
| B1+ / C3 | 510–580 Hz | 185–220 Hz | 155 Hz (Brown, one bass) | about 1.3 |
| B1− / lowest string | 2.2–2.45 | 2.0–2.8 | 2.0–2.8 | |
| Bridge hill | 2–3 kHz | 1–2.5 kHz (hills at 1 and 2–2.3) | 0.5–1 kHz | about 2 |
| Bridge, clamped | 3, 6 kHz | 1–1.5, 2.2, 3.1 kHz | 0.7, 1.1, 1.5 kHz [SSI15] | 1.4–2 |
| Critical frequency, top, along the grain | 4.87 kHz (2.5 mm) | 2.8 kHz (4.4 mm) [FR91] | not found | |

- Askenfelt: on a mobility plot the cello sits one octave below the violin and the bass two
  octaves below **[T]** [SSI15]. Pätynen & Lokki: cello modes at about 40 % of the violin's, bass
  modes at about 60 % of the cello's **[T]** [Patynen10].
- **What scales with size:** the air mode (volume and f-holes), the low plate modes, the bridge's
  own resonances, the critical frequency (falls as the plate thickens), the band where the
  radiation stops being uniform.
- **What does not:** the strings (they are heavier and shorter than scaling asks), so the
  string-to-body coupling grows with size up to the cello. The cello's body is smaller than a scaled
  violin's, its plates thinner, its strings heavier: "heavier strings on a lighter body", louder and
  wolf-prone **[T]** [Euph9.4]. In a real cello the main body resonance lies 4–5 semitones higher,
  relative to the strings, than in the violin **[T]** [FR91].
- **Against the tuning:** A0 lies below the second-lowest string on the violin, at it on the cello
  (G2), well above it on the bass (A1 55 Hz). The main top mode is near the top string's octave
  region on the violin and near the top open string on the bass, the cello between **[T]** [SSI15].
- **For the model [E]:** multiply the bass body's mode frequencies by about 1.5 (A0: 66 → 97 Hz)
  to 1.55 (T1: 111.86 → about 173 Hz), then set the cluster by hand: B1− 145–175, C4 195, A1 200,
  B1+ 205–220 Hz. With the bass's noble ratio 1.69478 and A0 = 97 Hz the top mode lands at 164.4 Hz,
  almost exactly E3 (164.8 Hz): a wolf on a named note. A0 = 100 Hz gives 169.5 Hz, between E3 and
  F3, which is where most cellos have it.

### 2.3 The wolf

- **Where:** on or between E3 and F#3 (165–185 Hz) on most cellos, about G# on the bass **[S]**
  [WikiWolf]. Measured: F3, 173–174.6 Hz, played high on the C string (two cellos) **[T]**
  [Zhang14, Euph9.4]; about 155 Hz on a test cello, with a double peak in the admittance **[T]**
  [Tschesche14]. Firth's two cellos: body resonances 165 / 185 Hz **[T]** [SSI14].
- **Why:** at a strong, lightly damped body resonance the bridge is "soft" (its impedance falls by
  about the mode's Q): the string's reflection is too weak to hold the Helmholtz motion. String and
  body then trade energy: the note warbles at about 5 Hz **[T]** [FR91]; 4–10 Hz **[S]**
  [Neubauer18]. The motion alternates between Helmholtz and double slipping; the pitch wavers
  **[T]** [Euph9.4]. The fundamental splits into a pair separated by the warble rate **[S]**
  [Firth73].
- **Which string:** the minimum bow force curve is the same for all strings times the string's wave
  resistance: worst on the C string (Z 1.46), then G; a player takes that F on the G string
  **[T]** [Euph9.4]. Predicted peak of minimum bow force on a bare cello about 11 N at the wolf
  (at the test's speed and β) **[T]** [Zhang14].
- **Cures, as model checks:** a tuned mass on the afterlength (a narrow absorber: only that mode);
  6.2 g on the bridge clearly helps, 1.6 and 4.3 g do not **[T]** [Zhang14]; tuned absorbers cut
  the admittance peak by more than 10 dB **[T]** [Tschesche14]; lighter strings; more bow force, a
  bow nearer the bridge, or squeezing the cello between the knees **[T]** [FR91], **[S]**.
- **Not found:** Schelleng's wolf criterion in numbers for a cello; the body-mode parameters of
  Zhang & Woodhouse 2018 (abstract only).

### 2.4 The spectrum of the sound

- **Formant regions, attributed to Meyer [S]** [WikiDE, Leonhardt, REMIC]: about 230–250 Hz;
  300–500 Hz; 600–900 Hz; a dip at 1000–1200 Hz; 2000–3000 Hz (brightness). The fundamentals of the
  lowest notes lie about 15 dB below the strongest partials; partials are weak above 3 kHz and
  reach about 8 kHz. Meyer's own text was not read.
- A room-averaged spectrum shows a formant at 800–1000 Hz, weaker than the violin's **[T]** [SSI14].
- Between two old cellos **[S]** [Schleske]: up to 6 dB difference at 600–900 Hz and up to 10 dB
  at 2–3 kHz (the Stradivari stronger).
- A preferred cello had the least level variation from note to note: 1.4 dBA mean deviation from
  the median against 1.6 and 2.4; 1.67 dBA between adjacent semitones against 2.37 and 2.54 **[T]**
  [JCAA].
- **Roll-off [E]:** the bowed string's force falls 6 dB/octave up to the corner-rounding
  frequency; the bridge passes little above its second hill (2–2.3 kHz), and the top's critical
  frequency is 2.8 kHz. So the radiated spectrum should fall steeply from 2.5–3 kHz. A measured
  slope in dB/octave from a long-time-average spectrum of cellos was **not found**.

---

## 3. Where the sound comes from

### 3.1 Which part radiates

- **Below A0** (C2 to about F#2): f-holes and plates move in opposite phase and cancel (Weinreich's
  sum rule) **[S]** [Gough16]. What is left is a dipole from the whole body's rigid back-and-forth
  motion, growing as 1/ω² towards low frequency: bowed instruments do not fall 12 dB/octave like a
  monopole below the air resonance **[T]** [Guettler12]. C2's fundamental is weak: about 15 dB under
  the strongest partials **[S]**.
- **At A0 (90–100 Hz):** the f-holes; strong enough that the cello can be driven by a sound field
  **[T]** [SSI14].
- **140–220 Hz:** the plates: C2/T1 (about 140 Hz) and C3 (about 220 Hz) radiate strongly; C4
  (195 Hz) and A1 (200 Hz) show at the bridge but radiate little **[T]** [SSI14]. One resonance can
  be omnidirectional while its neighbour is not: a Montagnana's 217 Hz mode is nearly uniform, its
  194 Hz mode has 105 dB towards the top (bass-bar side) and 77 dB at 150° towards the back **[S]**
  [Schleske].
- **250–500 Hz:** plate modes with side lobes (below).
- **Above about 1 kHz (violin)** the pattern changes from semitone to semitone ("directional tone
  colour") and calms again above about 4 kHz **[T]** [Weinreich97], **[S]**. The cello's transition
  should lie at about 0.4 of that, near 400–500 Hz **[E]**, which agrees with the measured loss of
  omnidirectionality at 300–500 Hz.
- **Above the critical frequency (2.8 kHz along the grain)** the plates radiate efficiently and in
  two lobes symmetric about the top's normal: Meyer's cello data show this above 2 kHz **[T]** [FR91].
- **Not found:** a measured split of radiated power between f-holes, top, back and ribs by band;
  the back's share; radiation efficiency by band; cello near-field holography.

### 3.2 Directivity

Pätynen & Lokki, 22 microphones around a seated cellist, anechoic **[T]** [Patynen10]:
- Below 300 Hz: within 6 dB in all directions; octave bands below 500 Hz nearly omnidirectional.
- 315 Hz third-octave: most sound to both sides, low.
- 500 Hz octave: strongly to the front in the median plane; the back of the player is attenuated
  from here up.
- 1000 Hz: uneven in the lateral plane; 1250 Hz third-octave concentrated to the front, slightly
  downward (−11° elevation).
- 2 kHz octave: even over the front half-space ("cardioid"); the bass shows the same one octave
  lower.
- With louder playing the pattern hardly changes; the spectrum does (more above 2 kHz).
- Harmonics of different notes that share a frequency radiate alike, less cleanly than on the violin.

Meyer, second-hand:
- Omnidirectional up to about 200 Hz; plate resonances at 250–300 Hz to the sides; the front–back
  ratio first exceeds 10 dB at about 500 Hz **[T, as cited in Patynen10]**.
- Around 1000 Hz largely upward; 2000–5000 Hz two zones, one towards the floor in front and one
  steeply upward **[S]** [Dickreiter via Struchholz]; two preferred angles symmetric about the
  top's normal above 2 kHz **[T]** [FR91, fig. 10.27–10.28, five cellos].
- Statistical directivity factor of the strings in their main directions at high frequencies: of
  the order of 2–3 (3–5 dB) **[T]** [Meyer96]. Several cello bands are poorly radiated to the
  player's left; the European seating (cellos facing the hall) is best for them **[S]** [Meyer72].
- Strings as a class have a low directivity index but the largest change of it from tone to tone
  **[T]** [Corcuera23].

For the model: omnidirectional to 200–300 Hz; a per-mode dipole term from 250 Hz (the bass's
"two sides" term, with its corner frequencies raised by 1.5–2); front-weighted above 500 Hz with
10 dB front–back; the floor reflection from a source 0.4–0.6 m above the floor **[E]** is left to
the room model.

### 3.3 Level and dynamics

Sound power level, dB re 1 pW:

| Source | pp | ff | Range | Note |
|---|---|---|---|---|
| Weinzierl et al. 2018, modern cello, extremes over the range **[T]** | 63 | 97 | 34 dB | single notes |
| Same, weighted by Beethoven's pitch use **[T]** | 70 | 93 | 23 dB | |
| Same, classical (gut-strung) cello **[T]** | 57 (avg 62) | 102 (avg 96) | 45 (34) dB | |
| Meyer via Rindel **[S]** | 67 | 97 | 30 dB | forte 90 dB |
| For comparison, Meyer via Rindel: violin / viola / bass | 65 / 68 / 75 | 97 / 93 / 97 | 32 / 25 / 22 | forte 89 / 87 / 92 |
| Weinzierl, modern violin / bass (ff max, pp min) | 52 / 56 | 95 / 100 | | |

- **Sound pressure [E]:** in a free field L_p at 1 m = L_W − 11 dB: about 52–56 dB (pp), 79 dB
  (forte), 86 dB (ff) at 1 m, before directivity (+3–5 dB on axis at high frequencies). A measured
  solo-cello level at a stated distance was not found. At the cellist's ear in the orchestra:
  L_Aeq 80.2 ± 3.2 dB, L_AS,max 94.8 dB, L_C,peak 113.5 dB **[T]** [Pawlaczyk10]: mostly the orchestra.
- **Spectrum with dynamics:** over all instruments sound power explains 69 % of the intended
  dynamic, spectral shape (skewness, flatness) and attack slope most of the rest **[T]**
  [Weinzierl18]. For the strings the levels above 2 kHz rise with loudness **[T]** [Patynen10].
  Mechanism **[T]** [KnutTable]: more bow force sharpens the Helmholtz corner (brighter); more bow
  speed alone rounds it (duller); the bow's place changes the level (∝ v/β) but not the slope.
  A dB-of-overtone per dB-of-level figure for the cello: not found.
- String amplitude and bridge force ∝ v_bow/β **[T]** [FR91]: halving β at the same speed gives
  +6 dB; the 23–34 dB range needs speed x(5–10) and β x(1/3–1/5) together.

---

## 4. Projection

- **Old against new, violins:** 137 listeners in two halls: new violins were judged to project
  better than Stradivari in all pairings but one; the result was the same with and without an
  orchestra; violins louder under the ear projected better **[T]** [Fritz17]. Projection is, to a
  first order, loudness in the bands the ear weights.
- **Old against new, cellos:** no peer-reviewed study found. A 1975 BBC blind test (6 old, 6 new
  cellos, about 140 musicians: best one old, second to fifth new) is reported third-hand **[S]**:
  unverified.
- **Which bands [E, from the cited pieces]:** an orchestra at forte peaks near 250 Hz and is about
  30 dB down at 3 kHz **[S, an essay citing Sundberg]**. The cello's fundamentals (65–660 Hz) lie
  in the orchestra's strongest band and are masked; what carries a soloist is 600–900 Hz (the
  "a" formant, where good cellos differ by up to 6 dB) and 2–3 kHz (the second bridge hill, up to
  10 dB between instruments) **[S]** [Schleske]. So projection is set by the bridge hills and the
  roll-off above them, and by evenness from note to note [JCAA].
- **Dünnwald's violin bands [S]:** 190–650 Hz, 650–1300 Hz "nasal", 1300–4200 Hz "brilliant",
  4200–6400 Hz "harsh". Scaled by the cello's 0.4: 75–260, 260–520, 520–1700, 1700–2600 Hz **[E]**;
  no cello version was found, and the cello's formants (section 2.4) do not follow this scaling
  exactly.
- **Player means [M]:** soloists play nearer the bridge with more force (more partials above
  1 kHz), with wider and faster vibrato (the partials sweep across body resonances, which makes
  the tone audible in a mass of steady tones), and higher on the A string.
- **Section against solo:**
  - Level: N players add 10 log N: 6 cellos +7.8 dB, 8 +9 dB, 10 +10 dB, 12 +10.8 dB **[E]**; ten
    cellos at forte about 100 dB sound power **[S]** [Rindel14]. Sections: 10 cellos in a large
    orchestra (16/14/12/10/8), 6 in a classical one **[T]** [Odeon].
  - Timing: bow changes within a section are normally distributed with a standard deviation of
    40–50 ms (first violins and cellos, video tracked); about half fall within 60 ms **[T]**
    [PatynenThesis].
  - Pitch: section simulations used ±10 cents between players and 1 dB amplitude modulation at
    5 Hz; the 3 dB width of an ensemble's spectral peaks is up to ±20 cents **[T, as cited in
    PatynenThesis]**. A measured pitch scatter inside a real cello section: not found.
  - The chorus effect follows from these: independent vibratos (rate and phase), ±10 c, 45 ms.
- **Not found:** any acoustics paper on balancing a solo cello against an orchestra.

---

## 5. The endpin, the floor and the player

### 5.1 What is measured for the cello

- **Cremer (1930s, in *Physik der Geige*):** the floor radiates, through the end pin, about 5 % of
  the directly radiated energy for a bass and about **1 % (−20 dB) for a cello**; he later called
  both probable underestimates **[T, as quoted in Askenfelt86]**.
- **Fuchs 1964** (bowing machine, anechoic room, a real orchestra riser, 1.5 m): **gain up to
  about 5 dB at 64–80 Hz; otherwise a loss, especially at 130–180 Hz**; the cello's air resonance
  "about 95 Hz" **[T, as quoted in Askenfelt86]**.
- **Beranek et al. 1964** (Philharmonic Hall): cellos gained less than basses on risers, and at
  100–150 Hz the cello was weaker on risers than on concrete (cancellation, or the riser absorbing
  the downward sound). Basses: about +5 dB at 40–100 Hz and above 200 Hz, +3 dB at 100–200 Hz
  **[T, as quoted in Askenfelt86]**.
- **Askenfelt 1986** measured basses only: a 1 x 2 m plywood riser on a rigid floor: +3 dB around
  50 Hz and about +5 dB at 150–1000 Hz in the far field; less around 100 Hz. Bridge acceleration
  exceeds the support's by about 30 dB on the riser and more than 50 dB on the rigid floor
  **[T]** [Askenfelt86].
- **End pin materials:** iron, tungsten, ebony pins (8 mm, 0.47 m): "slight differences" in the
  sound at 200–300 and 700–1000 Hz, none visible in the FEM/BEM prediction; no dB values **[T]**
  [Sakai23]. Seven pins of 18–238 g: no significant change at the top plate; the largest effect was
  on the floor sensor and the microphone (spectral centroid spread 30–100 Hz) **[T]** [Gorazd23].
  Listeners rated aluminium and tungsten "more powerful" **[T]** [Yamauchi01]. A rubber tip damped
  the contact vibration by about 3.5 dB in some bands **[S]** [Leiby10].
- **A bowed cello on a joist floor** (C3–F4): floor vibration is higher with the pin over a joist
  than between; repeatability worse than 5 dB **[T]** [Vigran13].
- The old Leipzig Gewandhaus was famous for its cello and bass sound, attributed to the wooden
  stage **[T]** [Guettler12].

### 5.2 The bass, measured in full (for scaling)

All **[T]** [Guettler12]:
- The bass at its end pin is a mass up to about 100 Hz: 1000 kg/s at 20 Hz to 10 000 kg/s at
  100 Hz (about 8 kg effective **[E]**), spring-like above.
- Stage floors: 4000 kg/s (a pliant hall floor) to 79 000 kg/s at 60 Hz in rigid sections,
  500–12 600 kg/s between joists, 12.6 x 10⁶ kg/s for parquet on bedrock. Loss factors 0.3–0.6
  below 60 Hz, 0.07–0.3 above.
- Power into the floor at f–ff: 3–6 mW over 31–100 Hz: 36–40 % of the bridge's power at 20–31 Hz,
  10 % at 40 Hz, 3 % at 50–63 Hz, **1 % at 80–100 Hz**. Above the impedance match (44 Hz in the
  best hall) the transfer falls about 9 dB/octave.
- In the hall: the best floor adds about 5 dB at 40–60 Hz and more than 10 dB at 30–40 Hz against
  a rigid support; another hall's lift adds nothing below 100 Hz and has a 5 dB dip at 40 Hz
  (floor and bass out of phase).
- The floor radiates below its coincidence frequency (585 Hz along the parquet's grain) only
  because it is driven at a point: like a piston a quarter of a bending wavelength in radius.
- A bass hung free radiated about 5 dB more at 30–50 Hz than on a rigid support: the pin holds
  back the whole-body (dipole) motion.

### 5.3 What it means for a cello

- The cello's lowest note (65 Hz) is already where the bass's floor path has fallen to 3 % of the
  bridge power; the cello is lighter (about 3 kg against 10 kg), so its end-pin impedance is about
  a third of the bass's **[E]** and meets a pliant floor's impedance at a higher frequency. The
  measurements agree: **the floor helps a cello only in 64–80 Hz (up to 5 dB on a riser), costs up
  to a few dB at 100–180 Hz, and is about −20 dB of the direct sound overall.** No source states
  an upper limit; from the above it is about 100 Hz **[E]**.
- The larger effect is on the **player's side**: the support under the pin changes the bridge
  mobility (a compliant floor raises it; a 3 dB change is felt as playability) **[T]** [Guettler12],
  and the pin restrains the body's rigid motion, the source of the dipole radiation below A0.
- **The pin itself [E, beam formulas, not measured]:** a steel pin 8 mm thick: first bending
  resonance about 100–160 Hz at 40 cm free length, 180–280 Hz at 30 cm (pinned or clamped at the
  cello); axial stiffness about 3.5 x 10⁷ N/m, an axial bounce of a 3 kg cello near 540 Hz. The
  bending resonance falls among the signature modes, which may be why pin material is audible to
  players at all. Pins: 8 or 10 mm, steel, carbon fibre, tungsten, titanium **[S]**.
- **The player's body:**
  - Sound spectra of one cello hung on rubber bands and held by a cellist differ (figure only, no
    dB in the text) **[T]** [SSI14].
  - For the violin the player is "a strong viscous damper in parallel with a weak spring" at the
    contact points: frequencies and shapes change slightly, amplitudes drop **[S]** [Marshall];
    holding reduces the bass response **[S]** [Bissinger15].
  - Squeezing the cello between the knees suppresses the wolf **[S]**: the ribs' and back's motion
    in the 150–220 Hz modes is damped by the legs. The chest touches the back's upper edge.
  - The player's body shadows the sound behind the cello above 500 Hz **[T]** [Patynen10].
  - **Not found:** Q or dB of A0 and B1 with and without a player; a cello end-pin impedance;
    measured gain of resonance boxes or podiums; baroque (no pin) against modern holding.

---

## 6. Bowing and articulation

### 6.1 The bow

- **Cello bow:** about 78–83 g, 69–72 cm long, 200–290 hairs **[S, dealers]**; hair bundle about
  59 cm, tension 70 N, about 290 hairs of 0.16–0.25 mm with about 50 touching the string; first
  transverse hair resonance 75 Hz; hair wave resistance 0.79 kg/s (transverse), about 10 kg/s
  (lengthwise, 50 hairs); hair Q 20 and 10 **[T, model values]** [Mansour2]. Low transverse
  oscillation of the stick with the hair: typically 65 Hz in a cello bow **[T]** [FR91].
- **Ribbon width** 1.5–2.6 % of the open string (violin 2.4–3.7 %, bass 1.4–1.7 %) **[T]**
  [BowNotes]: 10–18 mm on 690 mm **[E]**. The bass model's 16 mm of hair is in range.
- **Violin bow for comparison [T]** [KnutBow]: 60 g, 74–75 cm, 65 cm of hair, 150–200 hairs,
  tension 60 N, balance point 19 cm from the frog, stick modes 60, 160, 300, 500, 750 ... Hz.
  Bass bow: French about 125–140 g **[M]**.
- **Bounce:** a violin bow on a rigid support bounces at 6–35 Hz from 8 to 60 cm from the frog
  (Q 30–50); on a string at the spiccato point (35 cm from the frog) 13–14.7 Hz **[T]**
  [GuettlerThesis, KnutBow]. A measured cello-bow bounce rate was not found; heavier and shorter, it
  should be somewhat lower at the same hair tension **[E]**.

### 6.2 Force, speed, place

- **Cellists, motion capture** (6 players, 2 cellos) **[T]** [Chudy13]:
  - Slow scale: bow–bridge distance mean 5.9 cm (2.4–11.2), β 0.085 (0.035–0.16); bow speed mean
    16.6 cm/s (6–32).
  - Fast scale: 7.6 cm (3.2–11.9), β 0.11; 40.5 cm/s (12–90).
  - On higher strings the bow moves nearer the bridge and faster (D to A: up to +20 cm/s).
  - Bow force only in arbitrary units.
- **Violin, for scale [S]** [Askenfelt89 via Chudy]: distance 10–50 mm (sul ponticello 1–4 mm, sul
  tasto 55–60 mm); speed 0.2–1 m/s (0.1–1.5); force typically 0.5–2 N (0.15–3 N).
- **Bow force on the cello:** no player measurement in newtons was found. Bounds:
  - Schelleng's diagram for a cello A string at 20 cm/s spans β 0.01–0.2 and 0.1–10 N on its axes
    **[T, axes only]** [Gough07, FR91 fig. 10.6].
  - Bowing-machine tests on a cello D string used 0.4–3 N **[T]** [Galluzzo03]; a cello pendulum
    0–5 N at up to 30 cm/s **[T]** [Mores15].
  - Schelleng's maximum, F_max = 2 Z v / ((μ_s − μ_d) β) **[T]** [Euph9.3.1]: with μ_s − μ_d =
    0.5, v = 0.2 m/s, β = 0.1: A 4.5 N, D 5.4 N, G 8 N, C 11.7 N **[E]**. Usable forces are a
    fraction of this: roughly 0.5–3 N on the A string, 1–6 N on the C **[E]**.
  - F_max ∝ 1/β, F_min ∝ 1/β² (measured on the cello D string); below β = 0.036 no Helmholtz
    motion at all; starting from rest needs several times the force that sustains it **[T]**
    [Galluzzo03].
  - The bass's fitted window (`double_bass.md` 7e: least ≈ (0.01 + 0.68 β) Z v/β², most ≈
    (0.5 + 8 β) Z v/β) carries over with the cello's Z (0.56–1.46 against 1.35–3.9 kg/s): forces
    0.4 times the bass's at the same v and β, and the low-string problem (14 N needed) goes away.
- **Tone colour** **[T]** [KnutTable]: brighter with more force, duller with more speed, slightly
  brighter with tilted hair, duller with a soft or wide stopping finger; the bow's place changes
  only the level and local dips (partials near n = 1/β weaken).

### 6.3 Attacks

- **Guettler's diagram** (constant force, constant acceleration from rest): a wedge of force
  against acceleration gives a "perfect" start; outside it the first periods are prolonged
  (too much force: creaky) or split (too little: multiple slips, loose) **[T]** [Euph9.5].
  Measured on the cello D string (period 6.8 ms): force to 3 N, acceleration 0.24–3 m/s², eight
  bow places β 0.036–0.18; counted playable up to 20 periods (136 ms) **[T]** [Galluzzo03].
  G strings at β = 0.079: 71–85 % of the tested points settle within 20 periods; mean transient
  6.7–10.3 periods (69–105 ms at 98 Hz) depending on the brand **[T]** [Lampis24].
- **What listeners accept** (violin open G): at most 50 ms of prolonged periods or 90 ms of
  multiple slips, 10 and 18 periods **[T]** [GuettlerAskenfelt]. Professionals' attacks: 44 %
  "perfect" (under 5 ms to regular triggering).
- **Scaling:** the transient scales with the period: an octave lower takes twice as long **[T]**
  [KnutSculpt]. Ten periods: C2 153 ms, G2 102 ms, D3 68 ms, A3 45 ms, A4 23 ms **[E]**. The body
  adds its own build-up (violin about 18 ms; cello's low modes at Q 25–70 about 40–130 ms **[E]**).
- **Reported attack times [S, unverified, probably Meyer]:** cello 60–100 ms, up to 300 ms in soft
  styles; a 10–90 % rise of 50 ms sounds direct, 25 ms and under percussive **[T]** [KnutOnset];
  pizzicato rises in under 4 ms **[T]** [KnutOnset, after Melka].
- **Habits** **[T]** [GuettlerCreation, KnutSculpt]: cellists and bassists start "from the
  string" (the bow resting with force before it moves), where violinists often land a moving bow.
  Open strings are harder to start than stopped ones (no finger damping): players often pluck the
  open string lightly with the left hand as the bow starts. Force first, then speed: as found for
  the bass model.

### 6.4 Strokes

| Stroke | What the bow does | Numbers |
|---|---|---|
| Détaché | One note per stroke, on the string, even force | Acceleration at each change inside the Guettler wedge; β 0.07–0.12, 15–90 cm/s [Chudy13] |
| Legato | Several notes per stroke; the finger change, not the bow, makes the note | A new pitch settles in a few periods **[M]**; string crossings inside a slur risk multiple slips, masked by letting the old string ring **[T]** [GuettlerCreation] |
| Bow change | Speed through zero; force dipped by good players | No duration found; section scatter 40–50 ms [PatynenThesis] |
| Martelé | Bow "pinched" before the start, released into speed | Irregular first periods accepted under 20–30 ms; then a diminuendo by force release at constant speed **[T]** [KnutSculpt] |
| Staccato / slurred staccato | Short on-string strokes stopped by the bow | Stopped string rings on in the body: note end is the body's decay **[E]** |
| Spiccato | Bow thrown for each note, near its balance or middle | The frog moves at half the bounce rate; first slip near the force maximum; the string is damped between notes; flat hair **[T]** [GuettlerThesis] |
| Sautillé | The stick bounces by itself, hair hardly leaves | At the bow's own bounce rate: order 8–15 notes/s **[E]** from 6–35 Hz bounce rates |
| Ricochet | One throw, several rebounds in one direction | Faster than spiccato; the string rings between notes **[T]** [KnutSculpt] |
| Tremolo | Fast small strokes at the tip | Ceiling about 12 notes/s for one cellist **[S]**; unmeasured sections typically 10–16 per second **[M]** |
| Sul ponticello | β under about 0.04 with too little force | Multiple slips per period (7 at β = 1/24), the n ≈ 1/β harmonic dominant, level −11 dB **[T, violin simulation]** [KnutSculpt] |
| Sul tasto / flautando | β 0.15–0.25, light and fast | Fewer high partials, partials near 1/β missing; near the lower force limit **[M]** |
| Col legno battuto | The stick strikes | A click plus a short pizzicato-like tone; no data found |
| Col legno tratto | The stick is drawn | Weak, noisy; no data found |
| Harmonics | A finger touches at L/n | Natural on each open string (n = 2–6 usable); artificial: stop + touch a fourth above = 2 octaves above the stopped note. The bass model's rule (the bow drives only every n-th mode) carries over |

- **Pizzicato:** a strongly plucked open A rings for about 2 s: Q of 100 or more with the body's
  loading **[T]** [Gough07]. The string alone would ring far longer (Q 1200–3000): the decay is set
  by the bridge's mobility at each partial and, stopped, by the finger (η_F x3). Decay times per
  string: **not found**; expect them between the violin's (well under 1 s) and the bass's
  (`double_bass.md` 5.1: 6–30 dB/s for fundamentals) and to dip sharply near A0 and the 140–220 Hz
  cluster **[E]**. Plucking point: over the end of the fingerboard, about 1/5–1/4 of the string
  **[M]**. Bartók pizzicato: the bass model's fingerboard collision.

### 6.5 Vibrato

- **Rate:** soloists on commercial recordings 6.5 Hz (sd 0.42, 54 excerpts); sample-library tones
  5.3 Hz (sd 0.28, 40 excerpts) **[T]** [Hu25]. Students: about 5.5 Hz, no significant difference
  between violin and cello **[S]** [Geringer04]. Double bass: 5.2 Hz, 19 cents **[S]** [Mick24].
  Violin soloists: 6.6 Hz, 63 cents **[S]**.
- **Width:** string players generally 0.2–0.35 semitone **[T, citing]** [Timmers03]; the violin's
  is wider than the cello's **[S]** [Geringer04]. On the cello the width in cents grows towards the
  bridge (ρ = 0.69) while the finger's travel shrinks (ρ = −0.64): players compensate only partly.
  Mean finger travel is 0.5 % of the string length **[T]** [Hu25]. From that, if it is the full
  travel **[E]**: about 11 c in first position (a quarter up the string), 17 c at mid-string, 34 c
  at three quarters. The paper does not state mean cents.
- **Rate against register:** rises only weakly with position **[T]** [Hu25].
- **For the model [E]:** solo 5.8–6.5 Hz, ±10–20 c low on the string growing to ±25–35 c in thumb
  position; section players 5–5.5 Hz and narrower; the bass's 5.2 Hz and ±10 c is the slow end.
  The amplitude modulation is not put in by hand: it comes from partials sweeping the body's
  resonances.
- **Not found:** onset delay of vibrato after the note start, its direction relative to the
  pitch for cello, dependence on dynamics, amplitude-modulation depth in dB.

### 6.6 The left hand and the player's choices

- **Stopped against open:** the fingertip damps (η_F x3 [Mansour1]) and rounds the corner (wider
  pads on cello and bass than on violin) **[T]** [KnutTable]. Open strings are brighter, ring on
  after the bow leaves, cannot take vibrato: avoided in melodic playing except for colour **[M]**.
- **Portamento:** slide rates from about 600 Hz/s (recent recordings) to 4000–4500 Hz/s (Casals,
  1930s); slides last 45 ms (fast) to 300 ms **[T]** [Portamento]; used less since the 1930s **[S]**.
- **Shifts and crossings:** durations not found. A fifth between strings means fewer crossings than
  on the bass but more shifts: an octave span needs a shift or the thumb.
- **Which string [M]:** each string covers about two octaves in ordinary use. A string: bright,
  penetrating, the solo voice (A3 up to A5 and beyond); D: warm, covered; G: dark, full; C: heavy,
  slow to speak. The same pitch high on a lower string is darker and more veiled (the stopped
  length is short against the string's stiffness and the finger's damping; the higher harmonic
  number of the rounded corner [KnutTable]): players choose it for colour (sul D, sul G). Thumb
  position from about A4 on the A string.
- **Wolf avoidance:** F3–F#3 is played on the G or D string, not high on the C **[T]** [Euph9.4].
- **Double stops:** fifths lie across one finger; the bass model's 60 ms rule for chords carries
  over. Three- and four-note chords are broken (two strings at a time).
- **Section against solo:** section players: less vibrato, on-the-string strokes, lower positions,
  40–50 ms timing spread, ±10 c; soloist: nearer the bridge, more force, wider vibrato, more
  portamento, high positions.

---

## 7. Main considerations for a model

In order of expected gain:

1. **The body's low modes at cello values:** A0 95–100 Hz; a strong top mode at 145–175 Hz; the
   cluster C4 195 / A1 200 / B1+ 205–220 Hz; next peaks near 280 and 300 Hz. These set the note-to-
   note unevenness, the tone's "wood", and the wolf.
2. **The bridge as a cello bridge:** about 19 g, 92 mm high, 90 mm feet, legs that sway at
   1.2–1.5 kHz and a second resonance at 2–2.3 kHz; nearly rigid rocking below 1 kHz with the pivot
   moving from the post side (under 205 Hz) to the bar side. The two hills are the cello's
   brightness and its projection.
3. **String-to-body coupling strong enough for a wolf** near F3 on the C and G strings, with a
   4–10 Hz warble when force is low, removable by a tuned absorber. If the model can wolf, its
   bridge admittance is right in level, not only in frequency.
4. **Four strings with measured data** (section 1.1 table): Z 0.56–1.46 kg/s, EI, the three-term
   loss model, η_F tripled under a finger; torsion at 5.2 times the transverse speed, Z_R 1.8 kg/s,
   Q 20–45.
5. **Attack transients that scale with the period** (ten periods: 153 ms on C2, 45 ms on A3) and
   a player who starts from the string, force before speed, inside the Guettler wedge.
6. **Bow parameters from cellists:** β 0.035–0.16 (mean 0.085–0.11), speed 6–90 cm/s (means 17 and
   40), force about 0.5–6 N by string; nearer the bridge and faster on the upper strings.
7. **Vibrato:** 5.3–6.5 Hz, width growing up the string; amplitude modulation left to the body.
8. **Radiation by band:** dipole below A0, f-holes at A0, plates at 140–220 Hz, side lobes at
   250–315 Hz, front from 500 Hz (10 dB front–back), cardioid at 2 kHz, two lobes above 2.8 kHz;
   roll-off above 2.5–3 kHz.
9. **Dynamics as spectrum, not gain:** 23–34 dB of level from speed and β; brightness from force.
10. **String choice and the left hand:** fifths, thumb position, the same pitch on a lower string
    for colour, portamento at 600–4500 Hz/s, open strings avoided, the wolf note re-fingered.
11. **The strokes:** détaché, martelé, spiccato/sautillé (the bow's bounce, 6–35 Hz by place),
    tremolo, ponticello and tasto as regions of the Schelleng diagram, harmonics, pizzicato with
    its two-stage decay.
12. **The air inside** with its (0,1) mode at 226 Hz (the A1 near 200 Hz) and the next at 496 and
    547 Hz: the bass's 16 cells along the body, rescaled.
13. **The player's knees and chest** as damping on ribs and back (the 150–220 Hz modes), and the
    body's shadow behind.
14. **The end pin and floor:** small: about −20 dB overall; up to +5 dB at 64–80 Hz on a hollow
    riser, a loss at 100–180 Hz. Worth one switchable low-frequency path, after everything else.
15. **A section:** N players with ±10 c, 45 ms, independent vibratos; +10 log N.

### Double bass → cello

| Parameter | Bass (model or literature) | Cello | Factor / change |
|---|---|---|---|
| Tuning | E1 A1 D2 G2, fourths | C2 G2 D3 A3, fifths | x1.59, 1.78, 2.0, 2.25 per string |
| String length | 1.06 m | 0.69 m | x0.65 |
| Tension per string | 258–337 N (steel) | 117–180 N | x0.45–0.5 |
| Mass per length, lowest / highest | 37–44 / 6–7 g/m | 14.3–17 / 1.5–1.9 g/m | x0.4 / x0.27 |
| Wave resistance, lowest / highest | 3.3–3.9 / 1.24–1.5 kg/s | 1.3–1.5 / 0.46–0.58 kg/s | x0.4 |
| Bending stiffness | gut/steel bass values (unchecked) | 1.9–6.2 x 10⁻⁴ N m² | use the table |
| A0 | 66 Hz | 95–100 Hz | x1.45–1.5 |
| Main top mode T1 / B1− | 111.86 Hz | 145–175 Hz | x1.3–1.55 |
| B1+ and cluster | C4 130, C3 155 Hz | 195–220 Hz | x1.4–1.5; closer together |
| Air standing waves | 140, 175 Hz | 226, 496, 547 Hz (rigid) | x1.4–1.6 by length 76/105–110 cm |
| Body length / ribs | about 110 / 20 cm | 75.5 / 12 cm | x0.68 / x0.6 |
| Top thickness | bass grid values | 3.9–4.7 mm (5.5 edge) | set from the table |
| Bridge | 160 mm, 60–100 g, sway 420–440 Hz | 92 mm, 19 g, sway 1.2–1.5 kHz, second 2–2.3 kHz | height x0.58, mass x0.2–0.3, resonances x3 |
| Bridge hill | 0.5–1 kHz | 1–2.5 kHz | x2 |
| String coupling (wolf) | wolf about G#2 | wolf E3–F#3, stronger | raise bridge mobility until it wolfs |
| Radiation corners (the "two sides" term) | 250 and 700 Hz | about 400–500 Hz and 1.2–1.4 kHz **[E]** | x1.6–2 |
| Omnidirectional up to | about 100–200 Hz | 200–300 Hz | |
| Floor path | 3–40 % of bridge power under 60 Hz | about 1 %; +5 dB at 64–80 Hz only | nearly off |
| Bow | 125–140 g, hair 16 mm | 78–83 g, hair 10–18 mm, 70 N, 59 cm | lighter, faster |
| Bow force | 5–10 N (14 N needed on E1) | 0.5–6 N | x0.4 at equal v and β |
| Bow speed, β | slow, β about 0.1 | 6–90 cm/s; β 0.035–0.16 | wider range |
| Bow band limit in the model | 3.8 kHz | at least 6–8 kHz **[E]** | partials reach 8 kHz |
| Attack | 40–180 ms | 45–150 ms by string (10 periods) | x0.5 at equal string rank |
| Vibrato | 5.2 Hz, about ±10 c | 5.3–6.5 Hz, ±10–35 c by position | faster; wider up the string |
| Player | fourths, half-step fingering, few high positions | fifths, thumb position, portamento, string choice for colour | new `Performer` rules |
| Sound power, forte | 92 dB | 90 dB (pp 63–70, ff 93–97) | range 23–34 dB against 22 |

---

## 8. Not found (no reliable source)

- Meyer's primary text: his cello formant list, attack-time and decay tables, directivity angles
  per band, podium remarks, dynamic spectral factor. All Meyer numbers here are second-hand.
- Q, damping and effective mass of the cello's signature modes; mode density and modal overlap.
- Internal air volume, total mass, plate masses, afterlength, soundpost length from a citable source.
- Bridge-hill frequency against bridge mass; French against Belgian bridge, controlled.
- A long-time-average spectrum of cellos with its roll-off in dB/octave; old against new cello
  spectra or blind tests (peer-reviewed); any paper on balancing a solo cello with an orchestra.
- The share of radiated power from f-holes, top, back and ribs by band; radiation efficiency.
- Sound pressure of a solo cello at a stated distance; overtone growth per dB of level.
- Cello end-pin impedance; end-pin force against bridge force; measured pin resonances; gains of
  resonance podiums; any cello measurement by Askenfelt himself (his are basses); Q of body modes
  with and without the player.
- Bow force of cellists in newtons; Schelleng's 1973 cello curve values; Mores's measured maxima.
- A cello bow's mass, balance point, bounce rate and stick modes from a measurement.
- Per-string attack times of real cello playing in ms; bow-change and finger-change transients.
- Pizzicato decay times per string; col legno, flautando, tremolo rates measured.
- Vibrato: mean width in cents for cellists from a primary text, onset delay, dynamics dependence.
- Shift and string-crossing durations; pitch scatter within a real cello section.
- Schelleng's wolf criterion evaluated for a cello; Zhang & Woodhouse 2018 body parameters.

Worth a manual download: Fleischer's cello reports (UniBw München, `cello_2009_2.pdf`,
`cello_2010_netzversion.pdf`: probably eigenfrequency and bridge-conductance tables), Meyer's
book, Zhang & Woodhouse JASA 144, 2852 (2018), Askenfelt 1982 (STL-QPSR 4/1982).

## 9. References

Fetched and read unless marked otherwise.

- **[SSI14]** Bynum & Rossing, "Cello", ch. 14 of Rossing (ed.), *The Science of String
  Instruments*, Springer 2010. **[SSI13, SSI15, SSI16, SSI17, SSI18]**: chapters by Gough/Curtin,
  Askenfelt (double bass), Guettler (bows, strings, bowing), Campbell (viols), Bissinger (octet).
  https://logosfoundation.org/kursus/The%20Science%20of%20String%20Instruments.pdf
- **[FR91]** Fletcher & Rossing, *The Physics of Musical Instruments*, Springer 1991, ch. 10.
  https://csclub.uwaterloo.ca/~pbarfuss/The_Physics_of_Musical_Instruments.pdf
- **[Zhang16]** Zhang, Woodhouse & Stoppani, "Motion of the cello bridge", JASA 140, 2636 (2016).
  https://www.repository.cam.ac.uk/handle/1810/262677
- **[Zhang14]** Zhang & Woodhouse, "On the playability of wolf note", ISMA 2014.
  http://www.conforg.fr/isma2014/cdrom/data/articles/000010.pdf
- **[Euph5.3, Euph9.3.1, Euph9.4, Euph9.5]** Woodhouse, *Euphonics*, sections 5.3, 9.3, 9.3.1,
  9.3.2, 9.4, 9.5, 9.8. https://euphonics.org/5-3-signature-modes-and-formants/ ,
  https://euphonics.org/9-4-chasing-the-wolf/ ,
  https://euphonics.org/9-5-getting-that-perfect-start-guettlers-diagram/
- **[Tschesche14]** Tschesche et al., DAGA 2014 (wolf absorbers on a cello).
  https://pub.dega-akustik.de/DAGA_2014/data/articles/000095.pdf
- **[Neubauer18]** Neubauer et al., "An active-system approach for eliminating the wolf note on a
  cello", JASA 143, 2965 (2018). Snippets only. https://tuprints.ulb.tu-darmstadt.de/28668/
- **[Firth73]** Firth & Buchanan, "The wolf in the cello", JASA 53 (1973). Abstract/snippet only.
- **[WikiWolf, WikiDE]** Wikipedia, "Wolf tone"; German Wikipedia, "Violoncello". Secondary.
- **[Viala]** Viala et al., violin signature modes table (Sci. Rep.).
  https://publiweb.femto-st.fr/tntnet/entries/21285/documents/author/data
- **[Blanford18]** Blanford, Shepherd & Jerome, Proc. Mtgs. Acoust. 33, 035002 (2018).
  https://physics.byu.edu/docs/publication/8777
- **[Johannsson]** H. Johannsson, cello measurements table.
  https://www.hansjohannsson.com/resources/mescello.html
- **[Schleske]** M. Schleske, sound analysis pages (Tecchler, Stradivari "Lott", Montagnana).
  Search snippets only. https://www.schleske.de/en/research/introduction-violin-acoustics/sound-analysis/sound-analysis-examples.html
- **[Leonhardt, REMIC]** https://violin-leonhardt.de/en/service/dictionary/cello.html ,
  https://www.remic.dk/academy/article/how-to-mic-a-cello/ . Secondary, crediting Meyer.
- **[JCAA]** "Sound quality in the cello", Canadian Acoustics.
  https://jcaa.caa-aca.ca/index.php/jcaa/article/download/551/233/623
- **[KnutStr]** Guettler, "Typical string properties". https://knutsacoustics.com/files/Typical-string-properties.pdf
- **[KnutStiff]** Guettler, "String stiffness". https://knutsacoustics.com/files/String-stiffness.pdf
- **[KnutTable]** Guettler, table 15.2 "Parameters that affect the string's spectrum".
  https://knutsacoustics.com/files/table-for-rossing.pdf
- **[BowNotes]** Guettler & Askenfelt, "Bow notes", Proc. IOA 19(5), 1997.
  https://knutsacoustics.com/files/bow-notes--i.o.a-1997-.pdf
- **[KnutBow]** Guettler, "Some physical properties of the modern violin bow".
  https://knutsacoustics.com/files/some-bow-properties-cism.pdf
- **[KnutSculpt]** Guettler, "The sound-sculpturing bow".
  https://knutsacoustics.com/files/The-Sound-Sculpturing-bow-CISM_7opl67i1.pdf
- **[KnutOnset]** Guettler, "Onset-transient times".
  https://knutsacoustics.com/files/Onset-transient-times-Rev.2.pdf
- **[GuettlerCreation, GuettlerAskenfelt]** Guettler, "On the creation of the Helmholtz motion in
  bowed strings", Acta Acustica 88 (2002) (cites Guettler & Askenfelt 1997 for the 50/90 ms
  limits). https://knutsacoustics.com/files/actaacustica-on-the-creation.pdf
- **[GuettlerThesis]** Guettler, *The bowed string*, KTH 2002.
  https://knutsacoustics.com/files/thesis-guettler-020607.pdf
- **[Mansour1, Mansour2]** Mansour, Woodhouse & Scavone, "Enhanced wave-based modelling of musical
  strings", Parts 1 and 2, Acta Acustica 102 (2016).
  https://www.repository.cam.ac.uk/handle/1810/261228 , https://www.repository.cam.ac.uk/handle/1810/261227
- **[Lampis24]** Lampis, Mayer & Chatziioannou, "An experimental approach for comparing the
  influence of cello string type on bowed attack response", JASA Express Lett. 4, 113201 (2024).
  https://pub.mdw.ac.at/pubmdw/publication/50841b67-c7f9-43fa-b05d-8314c1edd2c1/pdf
- **[Galluzzo03]** Galluzzo, *On the playability of stringed instruments*, PhD, Cambridge 2003.
  https://www.repository.cam.ac.uk/handle/1810/244810
- **[Serafin99]** Serafin, Smith & Woodhouse 1999. https://ccrma.stanford.edu/~serafin/SerafinEtAl.pdf
- **[Gough07]** Gough, "Musical acoustics", Springer Handbook of Acoustics, ch. 15.
  https://www.math.ucdavis.edu/~saito/data/auditory/gough_musical-acoustics.pdf
- **[Gough16]** Gough, Acoustics Today, Summer 2016 (sum rule). Snippet only.
- **[Chudy13]** Chudy, Dixon & Pérez Carrillo, ICA 2013 (cello bowing, motion capture).
  https://webspace.eecs.qmul.ac.uk/s.e.dixon/pub/2013/Chudy-Dixon-Perez-ICA2013.pdf
  **[Askenfelt89 via Chudy]**: Chudy's PhD thesis, p. 112.
  https://webspace.eecs.qmul.ac.uk/s.e.dixon/phd/MagdalenaChudy-PhD-Thesis.pdf
- **[Mores15]** Mores, cello pendulum, Vienna Talk 2015.
  https://viennatalk2015.mdw.ac.at/proceedings/ViennaTalk2015_submission_66.pdf
- **[Larsen, Aitchison, Gamut]** String tension charts.
  https://www.gewamusicusa.com/catalog/documents/Larsen-String-Tension-Charts-18.pdf ,
  https://www.aitchisoncellos.com/string-tensions-charts/ , https://www.gamutmusic.com/cello-tensions/
- **[Hu25]** Hu et al., "Influence of string register locations on vibratos among
  violoncellists", arXiv 2512.18162. https://arxiv.org/pdf/2512.18162
- **[Geringer04]** Geringer & Allen, J. Res. Music Educ. 52, 167 (2004). Abstract only.
- **[Mick24]** Mick 2024, double-bass vibrato. Snippet only.
- **[Timmers03]** Timmers & Desain, "Vibrato: questions and answers from musicians and science".
  https://www.mcg.uva.nl/mmm-2003/papers/mmm-35/mmm-35.pdf
- **[Portamento]** arXiv 2604.22037 (cello portamento in Beethoven sonata recordings).
  https://arxiv.org/pdf/2604.22037
- **[Patynen10]** Pätynen & Lokki, "Directivities of symphony orchestra instruments", Acta
  Acustica 96, 138–167 (2010). https://users.aalto.fi/~ktlokki/Publs/patynen_aaua_2010.pdf
- **[PatynenThesis]** Pätynen, *A virtual symphony orchestra for studies on concert hall
  acoustics*, Aalto 2011. https://aaltodoc.aalto.fi/handle/123456789/5055
- **[Weinzierl18]** Weinzierl et al., "Sound power and timbre as cues for the dynamic strength of
  orchestral instruments", JASA 144, 1347 (2018). https://depositonce.tu-berlin.de/handle/11303/9646
- **[Shabtai17]** Shabtai et al., JASA 141, 1246 (2017). https://doi.org/10.14279/depositonce-9009
- **[Corcuera23]** Corcuera, Chatziioannou & Ahrens, JAES 71, 293 (2023).
  https://research.chalmers.se/publication/536676/file/536676_Fulltext.pdf
- **[Rindel14]** Rindel, Forum Acusticum 2014 (Meyer's sound power values).
  https://www.odeon.dk/pdf/RehearsalRooms_ForumAcusticum2014_Rindel.pdf
- **[Odeon]** Odeon application note, orchestra auralisation.
  https://odeon.dk/pdf/Application_note_Orchestra_auralisation.pdf
- **[Pawlaczyk10]** Pawlaczyk-Łuszczyńska et al., Arch. Acoustics 35, 579 (2010).
  https://journals.pan.pl//Content/107566/PDF/101.pdf
- **[Meyer72]** Meyer, JASA 51, 1994 (1972). Abstract only. **[Meyer96]** Meyer, ASA 131st
  meeting abstract. https://www.auditory.org/asamtgs/asa96ind/3pAA/3pAA1.html
- **[Dickreiter via Struchholz]** https://opus4.kobv.de/opus4-hs-duesseldorf/files/92/diplom-struchholz.pdf
- **[Weinreich97]** Weinreich, "Directional tone color", JASA 101, 2338 (1997).
  https://knutsacoustics.com/files/weinreich,-g.-directional-tone-color-jasa-1997.pdf
- **[Fritz17]** Fritz, Curtin, Poitevineau & Tao, "Listener evaluations of new and Old Italian
  violins", PNAS 114, 5395 (2017). https://hal.sorbonne-universite.fr/hal-01521152v1/document
- **[Guettler12]** Guettler, Askenfelt & Buen, "Double basses on the stage floor: tuning
  fork–tabletop effect, or not?", JASA 131, 795 (2012).
  https://knutsacoustics.com/files/guettler-et-al.-jasa-tuning-fork-or-not----with-errata.pdf
- **[Askenfelt86]** Askenfelt, "Stage floors and risers: supporting resonant bodies or sound
  traps?", in *Acoustics for Choir and Orchestra*, KMA publ. 52 (1986), 43–61 (quotes Cremer,
  Fuchs 1964, Beranek et al. 1964). https://www.speech.kth.se/music/publications/kma/papers/kma52-ocr.pdf
- **[Sakai23]** Sakai & Samejima, Acoust. Sci. & Tech. 44(3), 259 (2023).
  https://www.jstage.jst.go.jp/article/ast/44/3/44_E2255/_pdf
- **[Gorazd23]** Gorazd & Pochel, Vibrations in Physical Systems 34(2), 2023201.
  https://vibsys.put.poznan.pl/_journal/2023-34-2/articles/vps_2023201.pdf
- **[Yamauchi01]** Yamauchi, Kai & Iwamiya, Acoust. Sci. & Tech. 22(1), 47 (2001).
  https://www.jstage.jst.go.jp/article/ast/22/1/22_1_47/_pdf
- **[Leiby10]** Leiby, RPI MS thesis 2010 (end-pin pads). Abstract snippet only.
- **[Vigran13]** IRF 2013 paper 4746. https://fe.up.pt/irf/Proceedings_IRF2013/data/papers/4746.pdf
- **[Marshall, Bissinger15]** Marshall 1985; Bissinger & Mores, JASA 137, EL293 (2015). Snippets only.
- **[Rossing98]** Rossing et al., "Modal analysis of violins and cellos", ASA lay paper 1998.
  https://acoustics.org/pressroom/httpdocs/135th/rossing.htm

## 10. What was built from this (2026-11-18)

`-inst cello` (GM 43 in `-play`): the bowed double bass model (`src/bass.*`, `src/bassbody.*`,
`src/bassinst.*`) with a cello's numbers (`kCelloSpec` in `src/bass.cc`). What follows the
research above, and what is only an estimate:

- **Strings:** C2 G2 D3 A3 on 0.69 m, tensions 131.5 / 135.5 / 136 / 171 N (section 1, measured
  set), inharmonicity 1.0 / 0.6 / 0.4 / 0.3 e-4. Tuned in **pure fifths** down from A = 220 Hz; the
  stopping finger makes up the 2 to 6 cents, so stopped notes are equal-tempered. (With
  equal-tempered open strings the open C's third overtone beat against the G string's second
  at 0.2 Hz and the note's level swung by 9 dB.) Stretch stiffness 100 x the tension: [E].
- **Body** (`tools/bassbody.py -cello`, `src/cellobody_data.h`): the bass's plate-and-air grid at
  755 x 440 x 117 mm; fitted to A0 = 96 Hz and the main wood resonance = 174 Hz (the wolf's
  place); the next resonances come out at 194 (air), 243, 267, 285 Hz. Top 446 g, back 776 g.
  Plate thicknesses and the stiffer back are [E], chosen to move the upper resonances towards
  the 280 / 300 Hz of section 2.
- **Bridge:** 90 mm high, feet 90 mm apart, top 9 g, rocking at 1 kHz, up and down at 2.1 kHz.
  Tailpiece 80 g near 85 Hz: [E].
- **Bow:** hair 12 mm wide, 55 cm of it in a stroke, 5 to 60 cm/s (cellists reach 90; faster the
  open C string became unsteady in the model), the bass's friction and force rules unchanged.
- **Vibrato:** 6 Hz, +-10 cents in the first position growing to +-35 up the neck, on every held
  stopped note.
- **Endpin and floor:** a 72 Hz, Q 2.5 resonance driven by the bridge's force, added to the far
  sound; calibrated to lift the fundamentals of C2 .. E2 by about 5 dB and nothing above the air
  resonance (section 5: Fuchs's riser). `PJ_FLOOR=0` removes it. No loss at 130 .. 180 Hz is
  modelled, and nothing for the bass.
- **Evenness:** `tools/bowlevels.py -cello` -> `src/cellobowlevels_data.h`; 1.0 dB std between
  neighbouring notes after the player's evening (3.6 before).
- **Pizzicato:** a key let go within 100 ms (`PJ_PIZZ_MS`) is plucked; for that every key of the
  cello sounds 100 ms late (in `-play` the notes are sent that much early). Velocity is how far
  the fingertip pulls the string aside (0.8 to 6 mm); from velocity 105 the nail.
- **Mute:** 6 g on the bridge's top (`Bass::setMute`), set by the director's cue.

Not done: no comparison with a recording; the directivity of section 3 (the bass's simple
two-sides difference, scaled); the wolf does not warble; no sul ponticello / col legno beyond
what the bass's bow player has; the velocity curve is the contrabass's; a section of cellos is
one player.

The body runs as its modes since 2026-11-20 (`src/bodymodes.h`; `research/viola.md`, "The body
as modes"): 212 modes, every output within -67 dB of the grid's (`PJ_BODY_GRID=1`), the
resonances at the feet at the same frequencies and Q (173, 194, 267, 370 Hz ...), a bowed phrase
-80 dB; 0.14 us a sample instead of 1.8.
