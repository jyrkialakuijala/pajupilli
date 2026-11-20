# The steel-string acoustic (GM 26) and its 12-string variation: build supplement

Status: research only (2026-11-19). Nothing here is implemented. This note fills the gaps in
`research/steel_guitar.md` (cited below as **SG** §n) and turns it into a buildable
`steelGuitar()` spec plus guitarist settings. It does not repeat SG: the string chart (SG §2.1),
the D-28 and Fylde modal data (SG §3), the user's GM 26 file statistics (SG §9) and the slide
instruments (SG §10) stay there.

New here:
- FluidR3's steel guitar measured partial by partial (§1).
- Strum timing measured on six real players on a steel-string acoustic, from GuitarSet's
  per-string annotations (§3).
- A body table whose upper modes hit the measured mean bridge mobility (§2).
- The 12-string: strings, courses, beating, FluidR3's and GM2's versions, and how the model must
  change for it (§6).
- What to build (§8).

Marks as in SG: **[T]** read in the primary text or measured here (said which); **[S]**
secondary (abstract, retailer, snippet, machine summary); **[L]** player or engineer lore;
**[E]** my estimate or arithmetic; **[M]** general knowledge, not looked up. References: §10.

---

## 0. What the code has, and what this note assumes

- **`Guitar` (src/guitar.h/.cc).**
  - A `GuitarSpec` holds six `GuitarString`s {f0, tension, B, loss0, loss1}. Each mode's
    amplitude loss is `(loss0 + loss1 k^1.2) × end × ring × (k = 2–3: ringOvertones; k = 4–5:
    ringOvertones45)`.
  - The body is a list of `Mode {f, q, mass, rad, side, along}`.
  - Modes per string: `modes` (at most `kMaxModes` = 128), none above `topHz`.
  - The two polarisations can be set askew (`tilt`, `tiltJitter`) and detuned (`alongDetune`,
    `alongJitter`); `towardsLoss` adds loss to the direction towards the top.
- **The radiated sound.** It is `radiate × Σ rad_i × (the body mode's acceleration)`, and `side`
  is the same sum with the `side` weights. Strings do not radiate directly. Body mode *i*
  is driven by `fy + along_i × fz`, the strings' pull towards and along the top.
- **The bridge mobility** the strings see at a body peak is `Q / (2π f m)`. Between peaks, the
  mean conductance of a band of modes is about `n(f) / (4 m)`, with n the modes per Hz **[M]**
  (Skudrzyk's mean value). §2 uses both.
- **`Guitar::pick()` exists.** It is an edge 0.8 mm wide, a rise of 0.3–4 ms, a slip of 0.15 ms
  and a hold of 40 /s during the rise. The electric guitarist (`guitarinst.cc`) uses it:
  - depth (0.1 + 2.9 v^1.5) mm;
  - 0.11 m × (1.1 − 0.25 v) / 1.2 from the saddle;
  - up strokes × 0.85;
  - a hand speed of 1.75 m/s (6 ms a string) for strums. The nylon strum uses 1.4 m/s
    (7.5 ms).
- **No pick click** is modelled. `kStrings` = 6 is fixed at compile time, and arrays in `Guitar`,
  `GuitarSpec` (`str[6]`, `steel[6]`, `Pickup::at[6]`) and `GuitarPlayer` are sized by it.
- **MIDI routing.** `src/midifile.cc` maps "steel-guitar" to the banjo, then the koto. It does not
  read bank select (CC0/CC32) at all.

## 1. FluidR3's "Steel String Guitar" as a target

Measured for this note: parsed `FluidR3_GM.sf2` directly, and rendered single notes with
`timidity -c /etc/timidity/fluidr3_gm.cfg -EFreverb=0 -EFchorus=0` (scratch scripts `sf2.py`,
`mkmid.py`, `ana.py`, `ana2.py`). All **[T, measured here]** unless marked.

### 1.1 What the preset is

- **Preset "Steel String Guitar"** (bank 0, program 25 counting from 0) has nine velocity layers
  over one instrument. Each layer changes only the *release* time: 0 tc at velocity 121–127
  down to −2786 tc (×0.2) at velocity 0–64. Reverb send is 10 %.
  - The velocity layers do not filter. Under FluidSynth, SF2's default velocity→filter
    modulator would darken soft notes **[M]**; timidity's render shows none (centroid the same
    at 40, 80 and 120).
- **Samples:** nine recordings, one every 4–5 semitones, rooted at E2, A2, D3, G3, B3, E4, A4,
  B4 and E5, plus a short "Extension" above C6.
  - Recorded lengths are 5.0, 3.0, 4.25, 2.57, 2.69, 1.49, 1.56, 1.63 and 1.24 s. Each ends in
    a loop of 7–50 ms whose further fall is set by the volume envelope.
  - The envelope's decay to −100 dB runs 45 s on E2 (6590 tc), 32 s on E4 and 8 s on E5. So
    after a sample ends, the note falls 2–3 dB/s on the bass and 12 dB/s on E5.
  - Only the decays measured *inside* a recording (below) describe a guitar.

### 1.2 Decay per partial (inside the recorded part; velocity 120)

- **Method.** STFT with 8192 or 4096 points; the peak level of each harmonic band; a linear fit
  in dB from 0.2 s to the end of the recording, down to 50 dB below the harmonic's maximum.
- **Fitted to all harmonics of each note** (dB/s = a + b × f/kHz, and the model's own form):

| Note (open string) | dB/s fit | Up to | Fundamental (dB/s) | T60 at 1 kHz | T60 at 3 kHz | loss0, loss1 fit (1/s) |
|---|---|---|---|---|---|---|
| E2 82 Hz | 3.9 + 9.1 f | 1.6 kHz | −3 | 4.6 s | 1.9 s | 0.57, 0.045 |
| A2 110 | 9.0 + 3.6 f | 2.6 kHz | −7 | 4.8 s | 3.1 s | 1.10, 0.023 |
| D3 147 | 4.9 + 4.8 f | 3.5 kHz | −5 | 6.2 s | 3.1 s | 0.67, 0.042 |
| G3 196 | 5.6 + 5.2 f | 4.7 kHz | −6 | 5.5 s | 2.8 s | 0.81, 0.061 |
| B3 247 | 6.3 + 5.9 f | 5.9 kHz | −6 | 4.9 s | 2.5 s | 0.99, 0.085 |
| E4 330 | 8.0 + 6.0 f | 6.6 kHz | −11 | 4.3 s | 2.3 s | 1.22, 0.120 |

- **Above the fit range:** A4 decays at −21 to −28 dB/s in harmonics 1–7 and −40 to −50 above
  3.5 kHz. E5 decays at −29 to −32 dB/s in harmonics 1–3 and −45 to −90 above 2.5 kHz. The
  latter is a short sample, so the numbers are poor.
- **Reading.** A steel-string's partials at 1 kHz ring about 5 s, at 3 kHz 2–3 s and at 6 kHz
  about 1–1.5 s (extrapolated). This confirms SG §2.2's estimate, which was not measured. The
  built nylon gives 0.2–0.5 s above 2 kHz (`research/guitars.md` §1.2). The ratio of 5–10×
  is the single most important target.
- **The fundamentals** decay at −3 to −7 dB/s on the four lowest strings (T60 9–20 s) and −11 on
  E4 (5.5 s).

### 1.3 Spectrum and attack

- **Initial level of each harmonic, relative to the strongest** (first ~0.2 s), first four
  harmonics:
  - E2: −23, 0, −3, −13;
  - A2: −5, 0, −7, −5;
  - D3: −2, 0, −2, −1;
  - G3: 0, −9, −8, −19;
  - E4: −1, 0, −2, −6.
- **E2's fundamental is 23 dB under its octave.** E2 (82 Hz) lies below the breathing mode
  (~98 Hz), where the radiation falls steeply. This is a cheap, sharp check of the body: a
  dreadnought's low E is "all octave" on a microphone **[E]**.
- **Time to the envelope's peak** (1 ms RMS frames): E2 69 ms, A2 31, D3 40, G3 55, B3 14,
  E4 8, A4 16, E5 9. On the wound strings the sound *blooms* for 30–70 ms after the pick. The
  string's energy goes into the 100–200 Hz body modes, which take tens of milliseconds to grow
  (Q 20–35 at 100–180 Hz is a time constant of 30–110 ms) **[E]**.
- **Share of energy above 5 kHz (the pick).** For each note, the energy above 5 kHz against the
  note's total in three windows:

| Note | 0–5 ms | 50–100 ms | 0.5–0.6 s |
|---|---|---|---|
| E2 | −8 dB | −27 | −55 |
| A2 | −10 | −24 | −45 |
| D3 | −7 | −14 | −32 |
| G3 | −11 | −20 | −34 |
| B3 | −7 | −14 | −26 |
| E4 | −11 | −11 | −21 |
| A4 | −11 | −17 | −29 |
| E5 | −7 | −13 | −29 |

  The first 5 ms carry 7–19 dB more of their energy above 5 kHz than the note does 50 ms later.
  That excess is the pick's click and the release **[E]**, and it is a target for the click of
  §4.1.
- **Velocity under timidity:** +10 dB from velocity 40 to 80, +6 dB from 80 to 120 (16 dB in
  all), with no change of timbre.
- **After note-off** (a 0.25 s note): the level falls at −18, −22, −10, −17, −26, −26, −38 and
  −35 dB/s (E2 … E5). FluidR3 lets released notes ring with a T60 of 2–6 s. That is not what a
  guitarist's fretting hand does (§3.5): FluidR3's note-off is *not* a target.

## 2. The body as modes under the bridge

### 2.1 What is measured, and how it maps to the mode names

- **The naming.** Richardson's group calls the first three coupled modes **T(1,1)₁**,
  **T(1,1)₂** and **T(1,1)₃**:
  - T(1,1)₁, about 100 Hz: "breathing", top and back opposite, air out of the soundhole;
  - T(1,1)₂, about 200 Hz: "anti-breathing", top and back in phase, the ribs moving against
    them;
  - T(1,1)₃: the third member of the triad, air opposite again.
  
  These are **[S]** [FMSS14 snippet; Richardson et al., via search]. FR's D-28 shows exactly
  this triad at **102 / 193 / 204 Hz**, and clamping the ribs moves 193 to 169 Hz **[T]**
  [FR via SG §3.2; SoSI ch. 3].
- **Beyond the triad:**
  - The cross-grain dipole ("see-saw", FR's (1,0), Richardson's T(2,1)): 377 Hz on the D-28.
    The Fylde guitars' third peak at 335–368 Hz is "a transverse bending mode of the top plate"
    **[T]** [CBWFP18].
  - A long-grain or quadrupole mode: 436 Hz on the D-28 **[T]**.
  - On folk guitars the (2,0)-type mode near 550 Hz is "much less prominent" than on classical
    guitars **[T]** [SoSI ch. 3].
- **Which modes radiate:**
  - 102 and 204 Hz radiate "quite efficiently in all directions". 376 Hz is a dipole and 436 Hz
    a quadrupole; neither radiates much to the front **[T]** [SoSI Fig. 3.16, Popp & Rossing
    1986].
  - Held by a player, a Fender CD-60 (a dreadnought, steel strings) radiates nearly
    omnidirectionally up to ~200 Hz with its peak gain near **180 Hz**. Near 500 Hz the beam
    turns ~30° off axis; from 1.5 kHz it splits into lobes at about −60°, 0° and +60°;
    2.5–5 kHz is weak in all directions; 7–8 kHz beams strongly to one side **[T]** [AAU,
    1.7 m arc of 31 microphones, anechoic].
  - The initial level and the decay rate of played notes both follow the body's response
    curve. At strong resonances the initial levels are a little lower and the decays faster
    than the curve predicts **[T, via SoSI]** (Caldersmith & Jansson 1980).
- **Mobility above 500 Hz:** a mean of about −53 dB re 1 m/(s N) with ±8 dB peaks, one peak per
  30–60 Hz at 500–1 000 Hz, falling to −55 … −58 dB at 2–5 kHz **[T, read from the figure]**
  [CBWFP18 via SG §3.3].
- **Not obtainable:** the Cardiff PhD theses that compare classical, steel-string and banjo
  effective masses mode by mode (Perry 2014/15; Roberts 2015) refused automated download
  (HTTP 403). The same went for Woodhouse et al. 2012 (HAL bot wall). They are the next sources
  for effective masses (§9).

### 2.2 The table

- **The first five modes.** Masses come from m = Q / (2π f |Y|peak) with CBWFP18's peak
  mobilities (SG §3.3). For T(1,1)₃ and the 436 Hz mode only f is measured. Q, mass and the
  radiation shares are **[E]**, chosen so that the dipoles radiate little and the 183 Hz mode is
  the loudest, as AAU's 180 Hz peak gain says.
- **The plates (450 Hz–3.6 kHz).** The masses are set to hit the measured *mean* mobility, not
  copied from the classical:
  - Spacing Δf = 7.5 % (geometric), so the mean conductance is G ≈ 1 / (4 m Δf), that is
    m(f) ≈ 1 / (0.3 f G(f)).
  - G(f) = 0.0022 m/(s N) from 450 Hz to 2 kHz, sliding to 0.0015 at 3.5 kHz (CBWFP18's level).
  - Hence m ≈ 3.0 kg at 500 Hz, 1.5 kg at 1 kHz, 0.76 kg at 2 kHz and 0.63 kg at 3.5 kHz, each
    × (0.7 + 0.6 u) for a ±8 dB spread **[E]**.
  - At Q 40 a peak stands about 6 dB over the mean.
  - **This differs from the built classical.** Its plate masses *rise* from 0.56 to 2.4 kg
    (mean ≈ −39 dB near 500 Hz, ≈ −68 dB at 3 kHz **[E]**). The steel body is therefore about
    14 dB less mobile at 500 Hz and about 12 dB more at 3 kHz than the nylon model.
  - Check after building: the summed |Y| (§7 check 6).

| # | Mode | f (Hz) | Q | mass (kg) | rad | side | along | Basis |
|---|---|---|---|---|---|---|---|---|
| 1 | T(1,1)₁ breathing | 98 | 35 | 1.10 | 0.75 | 0.05 | 0.05 | f, Q [T: Fylde 96–101, Q 33–42; D-28 102]; m from \|Y\| 0.05 [T→E] |
| 2 | T(1,1)₂ top + air | 183 | 23 | 0.22 | 1.00 | −0.08 | 0.08 | f, Q [T: 175–188, Q 17–28]; \|Y\| 0.09 [T→E] |
| 3 | T(1,1)₃ back | 204 | 30 | 1.0 | 0.55 | 0.10 | −0.15 | f [T: D-28]; Q, m [E] (not a separate peak at the Fylde's bridge) |
| 4 | Cross dipole T(2,1) | 350 | 40 | 0.60 | 0.20 | 0.25 | 0.45 | f, Q [T: 335–368, Q 24–58; D-28 377]; \|Y\| 0.03 [T→E]; dipole [T] |
| 5 | Long dipole / quadrupole | 436 | 40 | 0.80 | 0.15 | −0.25 | −0.30 | f [T: D-28]; rest [E] |
| 6–33 | plates | 470 × 1.075^i (to ~3.6 kHz) | 30–55 | 1 / (0.3 f G(f)) × (0.7–1.3) | ± (0.5 − 0.3 t)(0.5 + u) | ± 0.3 u | ± (0.1–0.6) | mean \|Y\| [T: CBWFP18]; rest as `classicalGuitar()` [E] |

- **The `along` shares** stay as on the nylon: no steel-string in-plane mobility was found
  (SG §14).
- **Variants:**
  - **OM/000:** modes 1–5 × 1.08 in frequency and × 0.85 in mass (SG §7.2, lore only).
  - **12-string:** §6.4.
- **The 55 Hz bending mode** (D-28) is left out: below every string and radiating nothing
  **[T: "little or no musical importance"]**.

### 2.3 The strings against this body

- **The bridge takes little.** With G ≈ 0.002, a string loses α ≈ 2 f₁ Z₀ G (SG §2.2) through
  the bridge between body peaks: low E 0.35 /s, high E 0.35 /s (T60 ≈ 20 s) **[E]**.
- **So the string's own loss is most of the measured decay** above 500 Hz (§1.2). It is air
  drag on the plain strings and the wrap's friction on the wound ones. Kodama et al. found for a
  phosphor-bronze set on a Yamaha acoustic that the tone's attenuation is "caused mainly by the
  damping of air and the damping of the guitar body", with no rise with mode number like
  nylon's **[T, qualitative; their values only in figures]** [Kod23].
- **The string values in §8.1 are therefore §1.2's fits minus the body's share**, about 0.3 /s
  on loss0 **[E]**. With `ring = ringOvertones = ringOvertones45 = 1` (the nylon's by-ear
  factors do not belong here).

## 3. The pick and the strum, measured

### 3.1 The pick

- **Thickness.** Carral & Paset plucked an open E with a machine, using 0.46, 0.96 and 2.0 mm
  picks:
  - the thin pick bends more and displaces the string less;
  - the thick pick gives "considerably larger" sound;
  - the spectral centroid starts higher with the thick pick, but after ~1 s the thin pick's is
    higher; they attribute this to the sharper string shape at release, when only the thin
    pick's tip holds it **[T, ASA lay paper; no dB given]** [CP08];
  - a secondary summary reports the thin pick "initially 10 dB quieter" at the same striking
    force **[S]**.
- **Depth of the stroke.** Pluta et al. drove picks of six materials (nylon 0.67–1.14 mm,
  polycarbonate 1.30, steel 0.38, rubber, felt) into a 1.42 mm string in 192 µm steps **[T]**
  [Plu25]:
  - Below a material-dependent threshold the string is "not fully excited": the sound is weak,
    the low harmonics missing (the 5th dominant at the lightest depth).
  - Past it, more depth means louder, fuller, darker (centroid falls) and less noisy. From the
    third step on, the ratios of the first nine harmonics stay about fixed and only the level
    grows.
  - Every pluck from the third step on starts with "a spectrally wide impulse" that turns
    quickly into a "stable harmonic sound dominated by low frequencies": the pick's click,
    measured in kind.
- **For the model** **[E]**:
  - Velocity is depth, and at moderate and loud velocities brightness should *not* rise much
    with velocity (Pluta: the harmonic ratios stay fixed). It does at the lightest strokes,
    where the pick hardly catches the string (a quiet, thin sound).
  - A thin pick: less depth for the same force (×0.5–0.7), a faster slip.
- **Contact time, force, the pick's own ring:** still not measured anywhere found (§9). Keep
  SG's 0.1–0.3 ms slip.

### 3.2 Strum timing on a real steel-string acoustic (GuitarSet)

- **The data.** GuitarSet [XBP18] is six players on one acoustic guitar with a hexaphonic
  pickup:
  - annotated note onsets per string;
  - 60 "comp" excerpts: Rock, Singer-Songwriter, Funk, Bossa Nova, Jazz;
  - a Neumann U87 reference microphone ~30 cm in front of the 18th fret **[S]**.
- **Method.** Onsets on different strings within 40 ms of each other (whole chord under 120 ms)
  are one stroke. A stroke needs at least three strings, each once. Its direction comes from the
  correlation of string number with onset time (> 0.5: down, low string first; < −0.5: up).
  Script `gset.py`; 8 484 strokes; all **[T, measured here in GuitarSet's annotations]**.

| Style (median tempo) | Down / up / mixed | Down: span p25/50/75 (ms) | Down: step p50 (ms) | Up: span p25/50/75 | Up: step p50 | Strings, down / up |
|---|---|---|---|---|---|---|
| Rock (117) | 71 / 22 / 7 % | 16 / 23 / 31 | 7.8 | 18 / 26 / 34 | 7.7 | 4.0 / 4.2 |
| Singer-songwriter (98) | 56 / 35 / 9 % | 17 / 26 / 37 | 10.4 | 10 / 18 / 31 | 7.0 | 3.6 / 3.3 |
| Funk (108) | 67 / 28 / 5 % | 13 / 18 / 26 | 7.0 | 15 / 21 / 31 | 7.9 | 3.7 / 3.6 |
| Jazz (137) | 56 / 30 / 15 % | 12 / 20 / 26 | 6.9 | 10 / 25 / 36 | 9.4 | 3.7 / 3.6 |
| Bossa nova (131; fingers, pinched) | 31 / 52 / 18 % | 9 / 14 / 21 | 5.9 | 7 / 10 / 19 | 4.6 | 3.5 / 3.3 |
| All | 59 / 32 / 10 % | 14 / 21 / 29 | 7.5 | 10 / 19 / 31 | 7.0 | 3.8 / 3.6 |

- **The step does not depend on tempo.** Down strokes in Rock, Singer-songwriter and Funk have a
  median step of 8.0, 8.3, 7.3, 8.9 and 8.3 ms at 60, 80, 100, 120 and 140 bpm. The hand
  crosses the strings at its own speed, as the built guitarist assumes.
- **It falls with the number of strings:** median step 8.6, 8.1, 6.6 and 5.5 ms for 3, 4, 5 and
  6 strings. The hand that sweeps all six is moving faster: 1.9 m/s against 1.2 m/s at 10.5 mm
  spacing **[E]**.
- **Players differ by 2×:** their medians are 5.6, 6.5, 7.9, 8.0, 8.4 and 12.3 ms.
- **Which strings:**
  - Down strokes start on the A string on average (mean lowest string 0.9–1.0, with 0 = low E)
    and end on the B (3.7–4.0).
  - Up strokes sit about half a string higher: lowest 1.1–1.8, highest 4.3–4.5. In
    singer-songwriter playing the up stroke's lowest string is the D or above on average (1.78).
  - Full six-string strokes are rare in the annotations: 1–7 % of downs, 2–12 % of ups.
    (Muted or unchanged strings may not be annotated, so this undercounts.)
- **Where strokes fall:**
  - Down strokes begin on a beat in 53–73 % of cases; up strokes in 2–29 %.
  - In Rock, up strokes are rarely on the eighth "and" either (16 %): they fill the sixteenths.
  - In Bossa nova, 68 % of "up" groups are on the "and": the thumb-and-fingers pinch, not a
    stroke.
- **Against the user's files and the built guitarist.**
  - SG §9.2 measured a median step of 8.3 ms in the user's spread chords; GuitarSet gives
    7.5 ms. The two sources agree.
  - The nylon guitarist's 1.4 m/s (7.5 ms) is the median player. The electric's 1.75 m/s
    (6.0 ms) is a fast one.

### 3.3 Down against up: level and colour

- **No measurement of level found.** GuitarSet's annotations carry no amplitude. Its microphone
  audio (657 MB) would answer this, but could not be stored in this session (disk quota).
  It is the next measurement to make (§9).
- **Lore** (SG §4.3): up strokes are 3–6 dB lighter and brighter, and miss the bass strings
  **[L]**. The last part is now measured: the up stroke's lowest string is 0.3–0.8 strings
  higher (§3.2) **[T]**.

### 3.4 The chuck and the hand between strokes

- **The chuck:** the strumming hand's palm or the fretting hand slaps the strings on 2 and 4.
  It is a percussive click of 5–20 ms with the strings' pitches killed **[L]**. GM files write
  it at most as a very short chord (SG §4.3).
- **Model** **[E]**: a pick stroke at a quarter of the normal depth, followed within 3–8 ms by
  `damp(s, 200–400)` on every string the stroke crossed. Trigger: a chord of ≥ 4 notes shorter
  than 60 ms and quieter than the chords around it. Off by default.
- **Between strokes:** the strumming hand does not damp; the fretting hand does, by lifting
  (§3.5).

### 3.5 Note-off on steel strings

- **A real player** damps fretted notes when the fretting finger lifts (in tens of ms) and lets
  open strings ring unless palm or hand touches them **[M]** (SG §5.3).
- **FluidR3 lets everything ring** at 10–38 dB/s after note-off (§1.3).
- **The built guitarist** damps at 45 /s amplitude, that is 390 dB/s: right for a deliberate
  stop, too hard for a strummer's rhythm. GM 26 chords have a median length of 228 ms (SG §9.1):
  each strum would be cut dead before the next.
- **Proposal** **[E]**:
  - while the guitarist is strumming, a fretted note's note-off damps at 15–25 /s (130–220 dB/s:
    gone in ~0.3 s, not a click);
  - an open string's note-off does nothing until the next stroke (the next pick passes over it;
    the hand may touch it);
  - picked single notes and arpeggios keep 45 /s for fretted notes and let open strings ring.

## 4. Fingerstyle on steel (Travis picking)

- **The pattern** [Wiki-Travis; L]:
  - **The thumb** keeps a steady alternating bass on the lower three strings: on the four beats,
    alternating between the chord root's string (5 or 6) and the 4th string (or 3rd).
  - **The index and middle** pick syncopated melody and fill notes on the "ands", "most often
    the second and first" strings.
  - Beat 1 is often a pinch (thumb and middle together).
  - Merle Travis used "a banjo thumb pick" and bare fingers. Players named with a thumbpick and
    (acrylic) nails include Ani DiFranco, Doyle Dykes, Don Ross and Richard Smith **[S]**.
  - The thumb's bass notes are usually lightly palm-muted (the heel of the hand near the saddle
    on strings 4–6), which makes them thump while the trebles ring **[L, M]**.
- **What the model needs** **[E]**:
  - **Detection:** within a bar, single notes on strings 3–6 on the beats, alternating between
    two strings, and notes on strings 1–3 between them. The overlapping notes of SG §9.4 are the
    clue.
  - **Thumb:** `pick()` with the thumb-pick's rounder tip: a contact of about 1.5 mm (needs a
    width argument; `pick()` now fixes 0.8 mm), rise 1–2 ms, `angle` 0.3 (down and a little into
    the top), 0.12–0.14 m from the saddle.
  - **Thumb's palm:** `mute(s, 1.5–3 /s, 0.012–0.018 m)` on strings 4–6.
  - **Fingers:** `pluck()` with flesh and nail (nail 0.3–0.6), depth 0.5–1.5 mm (steel's tension
    is twice nylon's: the same finger moves it half as far), 0.12 m from the saddle.
  - **Pinches:** 0–3 ms apart, thumb first (as the nylon guitarist).
  - **Absent the pattern:** an arpeggio that lets notes ring (SG §9.4) gets the flatpick, played
    as cross-picking (alternating down and up, as the electric guitarist does with
    `pickWay_`).

## 5. Microphone and recording

- **Practice** [L: DPA, Recording Magazine, engineers]:
  - One small-diaphragm condenser at the 12th–14th fret, 15–30 cm away, pointed where the neck
    meets the body.
  - Moving it towards the soundhole adds "woof" (the 98 Hz mode through the hole). Towards the
    bridge, more body and less string.
  - A stereo pair: a second microphone at the lower bout behind the bridge, or a spaced or XY
    pair at 30–50 cm.
  - GuitarSet's reference microphone was a U87 ~30 cm before the 18th fret **[S]**.
- **What makes a recorded steel-string bright and "shimmery"** **[E]**, in order:
  1. The 2–8 kHz partials ringing for seconds (§1.2).
  2. Their slow beating:
     - the two polarisations detuned by about 0.05–0.1 % and askew (as the electric:
       `alongDetune` 0.0007, `alongJitter`, `tilt`);
     - stiff strings in a chord, whose stretched partials lie near each other;
     - sympathetic open strings.
  3. The pick's click (§1.3: the first 5 ms).
  4. Close microphones: the AAU measurement shows the 7–8 kHz radiation beaming to the sides and
     front lobes from 1.5 kHz **[T]**. A microphone 20 cm away sits in one lobe, a listener at
     2 m hears the average.
  5. Studio EQ: cuts at 200–400 Hz, "air" boosts at 8–12 kHz **[L]**; and compression, which
     lifts the tails.
- **For the model:** the two-microphone `side` channel already carries differences per mode.
  Give the plate modes above 1.5 kHz larger `side` weights (|side| up to 0.5) for the lobes
  **[E]**. Brightness comes from the strings, not from raising `rad` at high frequencies.

## 6. The 12-string (GM2 program 26, variation 1)

### 6.1 The instrument

- **Six courses.** The lower four are octave pairs, the top two unison pairs. Some players put a
  unison G, but "most prefer the distinctive high-pitched, bell-like quality an octave string
  makes in this position" **[S]** [Wiki-12].
- **Which string the pick meets first:** "the higher string of each pair is struck first on a
  downward strum" (the octave string, or on the unison courses the thinner-placed one).
  Rickenbacker electrics reverse the order **[S]** [Wiki-12]. On an up stroke the main string
  comes first **[E]**.
- **Building and tuning:**
  - Heavier build and bracing, a larger bridge plate, a wider neck **[S]**.
  - Older instruments had shorter scales or were tuned down "to reduce the stresses". Lead Belly
    tuned to low C (and B, A) on a 26.5–27 in. scale **[S]**.
  - Many players still tune a 12-string a whole tone down (D standard, total tension × 0.79)
    **[L]**.
  - Typical bodies: dreadnought (Martin D12-28) or jumbo (Guild F-512, Taylor 355) **[M]**.
- **D'Addario EJ38 (light 12-string, phosphor bronze)** tensions in lb **[S, retailer copy of
  the maker's chart]** [DAD-EJ38]; B is my arithmetic **[E]** (cores guessed as in SG §2.1):

| Course | Main string | T (lb / N) | B | Partner | T (lb / N) | B |
|---|---|---|---|---|---|---|
| 1 E4 | PL010 | 16.22 / 72.2 | 1.3e-5 | PL010 (unison) | 16.22 / 72.2 | 1.3e-5 |
| 2 B3 | PL014 | 17.85 / 79.4 | 4.6e-5 | PL014 (unison) | 17.85 / 79.4 | 4.6e-5 |
| 3 G3 | PB023 | 27.90 / 124.1 | 0.9e-5 | PL008 G4 | 14.68 / 65.3 | 0.6e-5 |
| 4 D3 | PB030 | 26.71 / 118.8 | 2.3e-5 | PL012 D4 | 18.54 / 82.5 | 2.4e-5 |
| 5 A2 | PB039 | 24.66 / 109.7 | 4.4e-5 | PL018 A3 | 23.41 / 104.1 | 9.7e-5 |
| 6 E2 | PB047 | 19.91 / 88.6 | 1.1e-4 | PB027 E3 (wound) | 26.91 / 119.7 | 1.4e-5 |

- **Totals and the octave G:**
  - The set totals **250.9 lb, 1 116 N**: 56 % more than the six-string's 714 N, on a bridge of
    the same size. Hence the heavier build.
  - The G course's octave string (.008, 392 Hz) runs at about 2.0 GPa, near the breaking
    stress of music wire (2.4–2.8 GPa **[M]**). It is the string that breaks **[E]**.
- **Course spacing:** the two strings of a course lie about 2–3 mm apart, the courses about 10 mm
  **[M]**. A stroke at 1.3 m/s reaches the second string of a course ~2 ms after the first
  **[E]**.

### 6.2 The shimmer: how far apart a course's strings are

- **Measured cents for 12-string courses: none found.** The nearest measurement is the
  doubled-string mandolin: high-speed video of both strings of a course shows energy exchange,
  anti-correlated motion and Weinreich's double decay. The coupled-oscillator model needs "a
  slight detuning" to reproduce it; the abstract gives no size **[S]** [CR16]. Engineers fake a
  12-string with a copy detuned 5–10 cents **[L]** [SOS].
- **Estimates of the actual detuning** **[E]**:
  - **Tuning by ear:** an octave or unison tuned against its partner is within 1–3 cents
    (unisons beat audibly at 1 cent: 0.2 Hz on a B3, 1 Hz at the 5th partial).
  - **Fretting stretches the pair unequally.** Pressing the string down raises its tension by
    (EA/T) × strain, about 2e-5 strain near the nut for 1 mm of action. EA/T is ~410 for the
    wound low E (core .018) but ~100 for the plain .008. So the wound main string goes about
    8 cents sharp at the first frets against about 2 cents for its thin octave partner: about
    6 cents apart. The gap shrinks up the neck.
  - **Inharmonicity pulls the octave pairs' partials apart even when the fundamentals agree.**
    The A course's plain octave string (B ≈ 9.7e-5) against the wound main (4.4e-5): the octave
    string's k-th partial lies below the main's 2k-th by 866 × (B_oct − 4 B_main) × k², that is
    about −1.7 cents at k = 5 and −7 cents at k = 10.
  - **So the beats speed up with frequency.** At the fundamental they are ~0.2–1 Hz; at 1–3 kHz,
    1–5 Hz. That is the "shimmer".
  - **The bridge couples the pair** (one saddle): unison courses show Weinreich's double decay,
    a fast first stage and a slow, ringing second **[T for piano; S for mandolin]**.
- **For the model:** a per-course detune of the partner string of +/−(1–3) cents (random,
  fixed per song), plus the fret-dependent difference above, plus each string's own B. The
  coupling through the one bridge does the rest; the `Guitar` already shares one bridge.

### 6.3 How players use it

- **Mostly strummed rhythm and ringing arpeggios** **[L/M]**. Examples: the Eagles' *Hotel
  California* intro, the user's largest GM 26 part (SG §9.1); Tom Petty's *Free Fallin'*; Bowie's
  *Space Oddity*; Pink Floyd's *Wish You Were Here*; Lead Belly; Leo Kottke's finger-style.
- **Single-note lines** are rarer: the octave strings make every bass-string melody sound in two
  octaves.
- **Bends** are small: two strings per course, and the thin octave string goes out of tune
  against its partner **[L]**.

### 6.4 The body and bridge for the model

- **Proposal** **[E]**: §2.2's table with modes 1–5 masses × 1.25 (heavier top, bracing,
  bridge and bridge plate) and frequencies × 1.0 (the stiffness gained offsets the mass). The
  plate masses also × 1.25. A jumbo body: modes 1–3 × 0.93 in frequency.
- **Not measured** anywhere found.

### 6.5 What FluidR3 and the GM2 and GS sets do

- **FluidR3 has "12 String Guitar" at GS bank 8, program 25** (bank 16 holds "Mandolin"). It
  layers the "Steel/GS" instrument (the same E2–E5 samples) twice **[T, sf2 parsed here]**:
  - layer 1: attenuation 4.5 dB, fine tune +1 cent;
  - layer 2: coarse +12 semitones (the same sample played an octave up), attenuation 6.3 dB,
    fine −1 cent, decay time × 2^(−600/1200) = × 0.71.
- **So FluidR3's 12-string has:**
  - every note doubled at the octave, including the two top courses, which on a real 12-string
    are unisons;
  - the octave voice 1.8 dB under the main and dying faster;
  - a fixed 2-cent offset, so the beating is the same on every note and every chord, a chorus
    rather than a 12-string;
  - an octave voice that is the recorded sample sped up: its body resonances and pick noise move
    up an octave too.
  - It is a target for nothing but "what a GM file expects": a brighter, doubled guitar a little
    louder than bank 0.
- **GM2:** melodic bank MSB 121. Program 26 has LSB 0 "Acoustic Guitar (steel)", LSB 1
  "12-Strings Guitar", LSB 2 "Mandolin" and LSB 3 "Steel Guitar with Body Sound" **[S]**
  [GM2-list; Yamaha voice lists]. So a GM2 file selects the 12-string with **CC0 = 121, CC32 = 1,
  then program 25 (counting from 0)**, not CC0 = 1.
  - GS (Roland) uses CC0 = 8 for "12-str.Gt" and CC0 = 16 for "Mandolin" on the same program,
    as FluidR3 does **[T: FluidR3's cfg; M: Roland]**.
  - XG uses MSB 0 with LSB 35 **[M]**.
- **In the user's files** (5 643 distinct files; script `banks.py`; **[T, measured in the
  files]**):
  - 2 045 program changes to program 26 have no bank select (about 90 % of them).
  - CC0 = 8 (GS 12-string) comes 52 times, in **36 distinct files** ("Dancing in the Streets",
    "Down at the Twist & Shout", "Blue Eyes Crying in the Rain", "Don't Take Away My Heart" …).
  - The rest: CC0 = 24 (32×), 9 (32×), 56 (9× with LSB 0, 11× with LSB 1), 8 with LSB 2 (10×,
    Celine Dion's *My Heart Will Go On*), XG LSB 35 (1 file).
  - **No file uses GM2's MSB 121.** About 30 more tracks are *named* "12 string" (sometimes
    without any bank select).
  - Read: the GS bank 8 select is the one that matters. Track names are a second clue.

## 7. What a listener would notice (additions to SG §8)

1. **The decay ratio.** Partials at 3 kHz must last 2–3 s and at 1 kHz ~5 s (§1.2). If the
   ratio to nylon is not 5–10×, it is a nylon.
2. **A low E that is all fundamental.** A dreadnought on a microphone gives E2 almost no
   fundamental (−23 dB under its octave in FluidR3), and A2 a weak one. A full, round 82 Hz is
   a different instrument (or a pickup).
3. **No bloom.** The wound strings' sound swells for 30–70 ms after the pick as the body modes
   fill (§1.3). An attack that peaks at once is a sampler or a piano.
4. **The click without the note's change.** From moderate to loud, a pick's harder stroke makes
   the same spectrum louder (Pluta). Only the lightest strokes are thin and different. A model
   that brightens steadily with velocity sounds synthetic.
5. **Strums at one fixed speed.** Real strums vary 2× between players and get faster when all
   six strings are swept (§3.2). The up stroke misses the low strings.
6. **Chords cut dead at note-off** (§3.5).
7. **12-string:**
   - a constant chorus beat (FluidR3's) instead of beats that differ course to course and speed
     up with frequency;
   - unison top courses played as octaves;
   - octave strings that come *after* the main ones on a down stroke.

## 8. What to build

### 8.1 `steelGuitar()` (`GuitarSpec`, name "steel-guitar")

- **Geometry:** scale 0.645 m, 20 frets, 6 strings.
- **Strings** (SG §2.1 tensions **[T]**; B **[E]**; loss from §1.2's fits minus ~0.3 /s for the
  body **[E: to refit]**):

| | E2 | A2 | D3 | G3 | B3 | E4 |
|---|---|---|---|---|---|---|
| f0 | 82.41 | 110.0 | 146.83 | 196.0 | 246.94 | 329.63 |
| tension (N) | 111.0 | 128.7 | 133.1 | 133.7 | 103.7 | 103.9 |
| B | 1.1e-4 | 4.9e-5 | 2.1e-5 | 0.9e-5 | 6.1e-5 | 1.9e-5 |
| loss0 (1/s) | 0.30 | 0.40 | 0.40 | 0.50 | 0.65 | 0.85 |
| loss1 (1/s) | 0.040 | 0.030 | 0.038 | 0.055 | 0.075 | 0.10 |

- **Losses and modes:**
  - `ring = ringOvertones = ringOvertones45 = 1`.
  - `modes = 96`, `topHz = 10000`. The low E's 96th partial lies at about 11 kHz, so the wound
    strings carry partials to 8–10 kHz.
  - `towardsLoss = 1`: the body takes the towards direction's energy itself.
- **Polarisations as the electric:** `tilt` 0.2, `tiltJitter` 0.3, `alongDetune` 0.0007,
  `alongJitter` 0.5 **[E]**.
- **Body:** §2.2's table: 5 named modes plus 28 plates from 470 Hz × 1.075^i, with masses
  1 / (0.3 f G(f)) × (0.7 + 0.6 u).
- **`radiate`:** start at the classical's 9.6e-3 and set the level against the nylon by
  `tools/levels.py` (the steel 3–6 dB louder at the same effort **[M]**).
- **Knobs** in the `PJ_GUITAR_*` style: `PJ_STEEL_LOSS`, `PJ_STEEL_LOSSHIGH`, `PJ_STEEL_BODYMASS`,
  `PJ_STEEL_BODY` (0 dreadnought, 1 OM).
- **Model additions** (in `guitar.cc`):
  1. **A width argument for `pick()`**, default 0.8 mm; the thumb-pick uses 1.5 mm.
  2. **A pick click.** A band-passed noise burst (2–8 kHz, 0.3–1 ms) added to the radiated
     output at the slip, proportional to the stroke's force:
     - on wound strings, a burst of 3–8 pulses at (pick speed / wrap pitch) (SG §4.1);
     - level set so that §1.3's 0–5 ms excess above 5 kHz (+7 … +19 dB over the note's share at
       50–100 ms) comes out;
     - larger for a thin pick.
- **Routing:** `-inst steel-guitar`, and `kGm` program 26 to it instead of the banjo.

### 8.2 The guitarist on steel (`GuitarPlayer` with `steel_ = true`)

- **The default right hand is a flatpick**, the electric guitarist's path (`pick()`), not the
  nylon's fingers. Fingerstyle only when §4's pattern is detected (SG §12.2 asks the user).
- **Picked notes:**
  - depth (0.1 + 2.6 v^1.5) mm, × 0.85 on up strokes;
  - 0.10–0.13 m from the saddle (the soundhole's lower edge), 0.08 m on the hardest strokes;
  - rise 1.2 ms falling to 0.6 ms with velocity;
  - alternate picking under 0.3 s, as now.
- **Strums:**
  - **Hand speed:** 1.4 m/s at the median (7.5 ms a string), × a per-performance factor drawn
    once from 0.7–1.5 (GuitarSet's players: 5.6–12.3 ms). Then × (1 + 0.12 (n − 3)) for an
    n-string stroke (8.6 ms at 3 strings, 5.5 at 6).
  - **Where:** 0.14–0.17 m from the saddle, over the soundhole.
  - **Depth:** the chord's evened velocity, as the nylon's strum (0.3 + 0.5 v), mapped to
    (0.2 + 2.0 vs^1.3) mm; up strokes × 0.8 (−2 dB) **[L, until measured]**.
  - **Which strings:** a down stroke takes every string of the voicing. An up stroke drops the
    lowest string with p = 0.5 and the second lowest with p = 0.2 when the voicing has ≥ 5
    strings (GuitarSet: half a string higher).
  - **The pendulum and bar memory** stay as they are (GuitarSet: downs on the beat 53–73 %, ups
    off it).
- **A spread chord in the file** (SG §9.2: half the files) keeps its written order and timing.
  Only simultaneous chords get the strum timing above.
- **Note-off** (§3.5): fretted notes at 20 /s while strumming, 45 /s otherwise; open strings ring.
- **Fingerstyle** (§4): the thumb is `pick(width 1.5 mm, rise 1.5 ms)` with a light palm
  (`mute` 1.5–3 /s at 15 mm) on strings 4–6; the fingers are `pluck(nail 0.3–0.6, depth
  0.5–1.5 mm)`.
- **Chuck:** off by default (§3.4).
- **Voicing completion and capo:** SG §9.3, not repeated.

### 8.3 The 12-string variation

- **The model** **[E]**:
  - **Twelve strings on one bridge.** Make `kStrings` 12 (or a template parameter), with
    `GuitarSpec::strings = 12` and a `course[12]` table giving each string's course (0–5) and
    its octave (0 or 1).
  - **Arrays to resize:**
    - `GuitarSpec::str[6]`, `steel[6]` and `Pickup::at/gap[6]` become [12];
    - `Guitar::str_[kStrings]` and `fieldSlope_[2][kStrings]` follow `kStrings`;
    - `GuitarPlayer`'s per-string arrays (`note_on_`, `held_`, `vib_`, `mend_`, `fretOf_`, …)
      stay per *course* (6). So the player needs its own `kCourses = 6`, not `Guitar::kStrings`.
  - **Model calls by course** (`fret`, `pick`, `pluck`, `damp`, `bend`, `mute`) fan out to the
    course's strings:
    - On a down stroke the partner (octave or upper unison) first, the main string
      (spacing / hand speed) ≈ 1.5–2.5 ms later; on an up stroke the reverse.
    - The same depth for both, the partner's × 0.9 (thinner, a little further from the pick's
      path) **[E]**.
  - **The rest of the code** (`for (Str& st : str_)`, the wake-up loop over `s_.strings`) already
    iterates and works unchanged.
  - **Cost:** about twice the six-string's: 12 strings at ~80 modes average with two directions.
    Perhaps 2.5–3.5 % of a core against the nylon's 0.7 % **[E]**.
- **`twelveString()` spec:**
  - main strings as §6.1's EJ38 table; octave strings at 2 f0 with their own T and B;
  - modes 96 on the wound strings, 64 on the plain octave strings, `topHz` 10000;
  - detune per partner, fixed per song: N(0, 1.5) cents, clipped at ±4 **[E]**;
  - the fret-dependent difference falls out of nothing in the model (it does not model fretting
    stretch), so add `+ (6 cents × (1 − fret/12))` to wound mains against plain partners on
    courses 4–6 **[E]**;
  - body §6.4;
  - knob `PJ_TWELVE_TUNE` (0 standard, −2 whole tone down: tensions × 0.79).
- **The guitarist:** the six-string's, with fewer single-note bends (bend × 0.5) and no
  vibrato on the octave courses.
- **Selecting it from MIDI:**
  - `midifile.cc` must start tracking CC0 and CC32 per channel. A program change to 25 (from 0)
    then chooses:
    - "twelve-string" when (CC0 = 121 and CC32 = 1) [GM2], CC0 = 8 [GS] or (CC0 = 0 and
      CC32 = 35) [XG];
    - "mandolin" (CC0 = 121 / CC32 = 2, or CC0 = 16) when one exists, else the six-string;
    - otherwise the six-string.
  - A track name matching `12[ -]?str` is a second trigger (≈ 30 tracks in the user's files).

### 8.4 Checks (measurable targets)

1. **Pitch:** every string within 1 cent open and at the 12th fret.
2. **Decay per partial** on single open-string notes, picked at velocity 100 (§1.2):
   - dB/s within ±30 % of a + b f/kHz: E2 3.9 + 9.1, D3 4.9 + 4.8, G3 5.6 + 5.2, E4 8.0 + 6.0;
   - fundamentals −3 to −7 dB/s on E2–G3, about −11 on E4;
   - T60 at 3 kHz 2–3 s on every string.
3. **Low E's spectrum** in the first 200 ms: the fundamental 15–25 dB under the 2nd harmonic;
   A2's 3–8 dB under.
4. **Attack:** the envelope peaks at 30–70 ms on E2–G3 and 8–16 ms on B3 and above.
5. **The pick:** energy above 5 kHz in the first 5 ms at −7 to −11 dB of the total; at 50–100 ms
   at −11 to −27 dB (falling from treble to bass); at 0.5 s at −21 to −55 dB.
6. **Bridge mobility** (sum of the body modes, 1/(m(jω + ω₀/Q + ω₀²/(jω))) per mode):
   - peaks at 98 Hz −26 dB, 183 Hz −21 dB, 350 Hz −30 dB (±2);
   - mean −53 ± 3 dB over 500 Hz–2 kHz, −56 ± 3 dB at 2–4 kHz.
7. **Long-term spectrum of a strummed G–C–D sequence:** peaks in 1/6-octave bands near 98 and
   183 Hz, the 183 Hz band the highest (AAU: peak gain ~180 Hz).
8. **Strum timing:**
   - six simultaneous notes give a first-to-last onset span of 25–35 ms (median performance) and
     15–55 ms over the range of the per-performance factor;
   - a three-note chord 15–20 ms;
   - up strokes skip the lowest string about half the time on ≥ 5-string voicings.
9. **Velocity:** 40 → 80 → 120 adds about 10 and 6 dB (timidity's FluidR3) or the project's own
   curve. From velocity 60 up, the spectral centroid of the first 100 ms changes by less than
   20 %.
10. **Note-off while strumming:** a released fretted note 20–30 dB down 0.3 s later; open strings
    within 3 dB of an unreleased one.
11. **12-string:**
    - an open D course (D3 + D4) shows beats of 0.2–1 Hz in its 294 Hz band and 1–5 Hz in its
      bands at 1–3 kHz;
    - a down stroke's onset on each octave string 1.5–2.5 ms before its main string;
    - the top two courses with no energy at the octave above their fundamentals beyond the
      strings' own 2nd harmonic;
    - CPU under 4 % of a core;
    - a GS file with CC0 = 8 on program 26 plays it.

### 8.5 Order

1. The `steelGuitar()` spec and its routing; checks 1–7.
2. Pick width and click; the guitarist's steel path (strum timing, up-stroke strings,
   note-off); checks 5, 8–10.
3. Travis-pattern detection.
4. Bank-select reading in `midifile.cc`.
5. The 12-string (the `kStrings` change first, verified to leave the six-string guitars
   bit-identical); check 11.

## 9. Not found, or uncertain

- **Effective masses mode by mode for a steel-string guitar** (T(1,1)₁₋₃, dipoles). The Cardiff
  theses (Perry 2014/15, Roberts 2015) and Woodhouse et al. 2012 hold them, but refused automated
  download (403, bot wall). §2.2's masses come from peak mobility and Q alone.
- **In-plane (along-the-top) bridge mobility** of a steel-string: still none. The `along` shares
  are the nylon's guesses.
- **An OM/000 modal table:** still lore only.
- **Pick contact time, force, the pick's own ring, and the click's level** relative to the note:
  - Carral & Paset give no dB in the lay paper;
  - Pluta et al. show it only in figures;
  - §1.3's FluidR3 numbers are the only quantitative target.
- **Down against up stroke level and spectrum on real players:** not measured. GuitarSet's
  microphone audio would answer it (download blocked by the session's disk quota).
- **GuitarSet's guitar model** (make, string gauge) was not confirmed; it is "an acoustic guitar"
  with a clip-on hexaphonic pickup **[S]**. Its onset annotations are semi-automatic, so steps
  under ~3 ms are uncertain.
- **12-string course detuning in cents and beat rates:** no measurement. The mandolin paper's
  detuning value is in the full text only. §6.2 is estimated.
- **12-string body and bridge measurements:** none.
- **Kodama et al.'s phosphor-bronze attenuation coefficients** are only in figures.
- **Allen (1976) on aging steel strings:** the PDF was not reachable. His later summary says the
  damping of the high partials, not B, changes with age **[S]**.
- **GM2 bank numbers** are from Yamaha voice lists and a GM2 list **[S]**. The GM2 specification
  itself was not opened. XG's LSB 35 is from memory **[M]**.
- **The steel guitar's level against the nylon** (3–6 dB): unmeasured **[M]**.

## 10. References

- **[SG]** `research/steel_guitar.md` (this project), and the references there: [FR], [CBWFP18],
  [FMSS14], [DAD], [W04a], [PPV08] …
- **[FluidR3]** `/usr/share/sounds/sf2/FluidR3_GM.sf2` and `/etc/timidity/fluidr3_gm.cfg`,
  parsed and rendered for this note.
- **[SoSI]** T. D. Rossing (ed.), *The Science of String Instruments*, Springer 2010, ch. 3
  (Rossing & Caldersmith, "Guitars and Lutes"): D-28 modes and radiation, Caldersmith & Jansson
  1980, Popp & Rossing 1986. https://logosfoundation.org/kursus/The%20Science%20of%20String%20Instruments.pdf
- **[AAU]** "Compact Loudspeaker Array for Synthesizing the Radiation Pattern of an Acoustic
  Guitar", Aalborg University master's thesis, ch. 4 (Fender CD-60 CE, anechoic, 31-microphone
  arc). https://projekter.aau.dk/projekter/files/421577399/Compact_Loudspeaker_Array_for_Synthezing_the_Radiation_Pattern_of_an_Acoustic_Guitar.pdf
- **[CP08]** S. Carral, M. Paset, "The influence of plectrum thickness on the radiated sound of
  the guitar", ASA 2008, lay-language paper. https://acoustics.org/pressroom/httpdocs/160th/carral.html
- **[Plu25]** M. Pluta, J. Jasiński, D. Tokarczyk, J. Grygiel, "The effect of micro-changes in
  the pluck trajectory on the sound of an acoustic guitar", *Vibrations in Physical Systems*
  36(2) 2025205 (2025); arXiv:2606.24356. https://arxiv.org/abs/2606.24356
- **[Kod23]** H. Kodama, M. Sato, S. Hoshika, S. Yokoyama, "Attenuation characteristics of tones
  and vibrations in guitars with nylon, fluorocarbon, and phosphor bronze strings pressed down
  against fret", *Acoust. Sci. & Tech.* 44(3), 218–229 (2023).
  https://www.jstage.jst.go.jp/article/ast/44/3/44_E2244/_article
- **[XBP18]** Q. Xi, R. Bittner, J. Pauwels, X. Ye, J. P. Bello, "GuitarSet: a dataset for guitar
  transcription", ISMIR 2018; data v1.1.0 (annotations used).
  https://ismir2018.ismir.net/doc/pdfs/188_Paper.pdf ; https://zenodo.org/record/3371780
- **[Wiki-12]** Wikipedia, "Twelve-string guitar". https://en.wikipedia.org/wiki/Twelve-string_guitar
- **[Wiki-Travis]** Wikipedia, "Travis picking". https://en.wikipedia.org/wiki/Travis_picking
- **[DAD-EJ38]** D'Addario EJ38 tension chart (retailer copy).
  https://www.yamahamusiclondon.com/EJ38-Light-12-String-Acoustic-Guitar-strings/pidDAD-EJ38-STR
- **[CR16]** "Investigations of the coupling of doubled mandolin strings using high-speed
  video", JASA 140 (2016), abstract, doi:10.1121/1.4970809.
  https://bvbvf1.bib-bvb.de/vufind/PrimoRecord/cdi_crossref_primary_10_1121_1_4970809
- **[SOS]** Sound On Sound, "How can I simulate a 12-string guitar sound in the studio?".
  https://www.soundonsound.com/sound-advice/q-how-can-simulate-12-string-guitar-sound-studio
- **[GM2-list]** GM2 voice list (MSB-LSB-program).
  https://psrtutorial.com/music/articles/VoiceHistory/03GM2%20Voice%20List_files/sheet001.htm ;
  Yamaha manuals, e.g. https://de.yamaha.com/files/download/other_assets/9/318079/MU100E2.pdf
- **[DPA]** DPA Mic University, "How to mic an acoustic guitar".
  https://www.dpamicrophones.com/mic-university/how-to-mic-an-acoustic-guitar ; Recording
  Magazine, "Miking the acoustic guitar".
  https://www.recordingmag.com/resources/recording-info/mics-miking/miking-the-acoustic-guitar/
- **Not reachable (§9):** I. A. Perry, PhD thesis, Cardiff 2015,
  https://orca.cardiff.ac.uk/id/eprint/70916/ ; W. H. Roberts, "Physics and psychoacoustics of
  plucked-string instruments", PhD, Cardiff 2015, https://orca.cardiff.ac.uk/id/eprint/80117/ ;
  J. Woodhouse, E. K. Y. Manuel, L. A. Smith, A. J. C. Wheble, "Perceptual thresholds for
  acoustical guitar models", Acta Acustica 98, 475 (2012), https://hal.archives-ouvertes.fr/hal-01461755 ;
  J. B. Allen, "On the aging of steel guitar strings", Catgut Acoust. Soc. Newsl. 26 (1976).
- **Scripts** (scratch, not in the repo): `sf2.py` (preset dump), `mkmid.py` + `ana.py` +
  `ana2.py` (FluidR3 renders and per-partial fits), `gset.py` (GuitarSet strums), `banks.py`
  (bank selects in the user's files), in
  `/tmp/claude-1000/-home-jyrki-pajupilli/f3f9b7e6-c8bc-4bf3-a7a3-c720f3e9b3d1/scratchpad/scripts/`.
