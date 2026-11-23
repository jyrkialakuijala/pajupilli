# tine electric piano: per-key data, measurements, electronics and flaws for the physical model

Status: 2026-11-19, research only; nothing here is built. SI units. This note sits on top of
`research/electric_pianos.md` (§3 tine piano, §6 FM piano, §7 model plan) and does not repeat its
derivations: the tip-loaded cantilever table, the bell-shaped flux function and its harmonic table,
the Hunt–Crossley hammer numbers, the Shore → modulus conversion. It fills the gaps a modeller needs
as numbers (per key, per era, per circuit stage), corrects that note where primary sources disagree
(§2), and updates its model plan (§12). The aim is the depth of `research/tonewheel_organ.md`: per-part
physics, tolerances, flaws, leakage, circuit stages with values, noise and ageing.

Marks as in the other notes: **[web, KEY]** from the sources in §15 (primary texts, patents,
schematics, papers, makers' pages); **[web, snippet]** known only from a search-engine summary or a
forum: weakest; **[calc]** worked out here (one-off Python in the session scratchpad; inputs given);
**[memory]** general knowledge, to check; **[design]** a modelling choice.

## 1. Short answer

- **The per-key geometry is now a table, not a guess.** The factory kit sheet "Cutting instructions
  for tone generators" (P/N 024297, in the Vintage Vibe scan of the 1979 manual) gives all 73 tine
  lengths in inches and the spring type per key (#069658 yellow on keys 1–33, #069666 blue on 34–73)
  **[web, RSM-AF]**; the manual's to-scale fig. 6-2, digitised, agrees within ±1.6 mm and adds the
  88's top octave (§3.2). The manual's figures give the swaged tine's profile: 2.54 mm for the first
  12.7 mm, then 1.524 mm. With that profile every key needs a spring of only ~0.2–0.4 g (two part
  numbers, as listed); f2/f1 runs 5.8–7.9 and f3/f1 17.6–20.5 **[calc]** (§3.3). The 88-key piano's
  seven lowest keys (A0–D#1) use the *same* 156.4 mm tine and an identical tonebar as E1, tuned down
  by heavier springs (~0.4–1.3 g) **[web, RSM ch. 6; calc]**.
- **The tonebar is not tuned to the tine in the bass and middle.** tine piano's own patents say the bar
  has a low Q and is set "about 2.5 piano tones" (≈ 500 cents) away from the tine in the low and mid
  range, within half a tone only at the top, and that low notes are deliberately put off the
  resonance peak to cut their dwell to about 17 s (treble 3–5 s) **[web, P656, P922]**. Muenster &
  Pfeifle measured the bars' own lowest frequencies "several hundred to more than 1400 cents" away from the note (their table: 760–3800 cents below), with the
  bar forced into exact in-phase or anti-phase motion at the tine's frequency **[web, MP14]**. So the
  tonebar is a heavy, lossy, *detuned* reaction mass; the tine is the oscillator. This corrects
  `electric_pianos.md` §1/§3.3 (§2).
- **Amplitude scales as 1/f, so the tip velocity is the same everywhere.** A CBS pickup patent gives
  the normal steady-state tip amplitude as ~1/8 in (3.2 mm) at C3 and ~1/64 in (0.40 mm) at C6
  **[web, P321]**: in both cases the peak tip velocity is 2.6 m/s **[calc]**. Against pickup gaps of
  0.5–3.2 mm this puts the bass and low middle at A/w ≈ 1–2 (deep in the nonlinear bell: bark) and
  the top at A/w < 0.5 (clean bell) at the *same* touch, which is why the growl lives in the lower
  half **[calc; web, MP14]**.
- **The fundamental saturates and then falls with amplitude.** For the bell-shaped flux at the usual
  voicing (δ/w ≈ 0.5), the output's fundamental peaks at A/w ≈ 0.8 and drops by 5 dB by A/w = 2 while
  the total level keeps rising ~5 dB per doubling **[calc]** (§4.4). So a hard bass note *gains*
  fundamental while it decays (a "bloom" over the first 1.5–3.5 s) and each pickup harmonic n decays
  n times faster (in dB/s) than the tine once A/w < 0.5 **[calc]**. These are cheap, strong
  validation targets.
- **Hammer contact is long: 6.4 ms average** (high-speed camera, Mark I), the waveform becomes
  sinusoidal within 10–14 ms, and after that the tine moves as a single sine in both planes
  **[web, MP14]**. In the bass and middle the contact lasts most of a period of the fundamental
  (C3: 7.6 ms) and many periods of mode 2, so the hammer itself filters the inharmonic modes; the
  bell transient comes mainly from the treble, from the hard wood-cored tips, and from the tonebar
  **[web, PM17, MP14; calc]**.
- **Production changed the sound more than the model name.** Tines: Raymac (to 1971), Torrington
  (1971–76), Singer (1976–77), Schaller (late 1977 on), each with its own taper and strike point;
  tips: felt (to ~1970), cube neoprene (1971–early 1976), graduated/triangular (1976 on); hammers:
  wood-cored (to mid-1975), all-plastic after; Mark II (1979) is a late Mark I in a new case; Mark V
  (1984) changed hammers and dampers for "more bell" **[web, CEP-T, CEP-M, RL-T, FR-M5]** (§5).
- **Electronics** (§6): 73 coils of 170–190 Ω, ~70–110 mH each, wired as 24 series groups of
  ~3 in parallel, alternating in polarity: a 1.4–2.1 kΩ, 0.6–0.85 H source **[web, FRS-PU, MORRIN;
  calc]**. Into the Stage's own 10 kΩ volume pot (or a Haigler Suitcase's 10 kΩ input) that is a
  first-order low-pass at ~2.2 kHz, which integrates the pickup's d/dt above it; the Peterson
  Suitcase's ~27 kΩ input puts it near 5 kHz; a harp into 1 MΩ resonates with the cable at 5–10 kHz
  **[calc]**. The Peterson "vibrato" is a square-wave astable (2.0–6.6 Hz) lighting #19 lamps that
  face ORP61 CdS cells, antiphase per channel: an auto-pan smoothed by lamp and cell lag; the 1977
  Haigler/Janus uses an op-amp oscillator and LED/LDR couplers (sharper) **[web, RSM figs 11-1,
  11-8; calc]**. Power amps are solid state (Peterson ±35 V unregulated, ~34 W/ch into 16 Ω; Haigler
  2 × 50 W) into 4 × 12 in Eminence 32 Ω speakers **[web, RSM; SG]**.

## 2. Corrections and refinements to `electric_pianos.md`

| There | Here | Source |
|---|---|---|
| "The tonebar is a heavy second prong tuned to the same f0" (§1, §3.3) | Detuned by ~2.5 tones (≈500 cents) in bass/mid by design, low Q; within ½ tone only in the top group; measured "several hundred to more than 1400 cents" (table: 760–3800 c below the note) | [P656, P922, MP14] |
| "The lowest seven tines of the 88 have no tonebars" (§3.1, from Shear) | They have identical tonebars and the same 156.4 mm tine as E1; numbered "0" | [RSM ch. 6, fig. 6-2] |
| A0 tine length unverified (§3.2) | 156.4 mm (same as E1); the spring carries ~1.3 g (swaged profile) | [RSM fig. 6-2; calc] |
| Tine modelled as a uniform 1.905 mm rod tuned by a tip mass (μ 0.19 at E1 → 0 above C7) (§3.2) | 1975+ tines are swaged: 2.54 mm for 12.7 mm, then 1.524 mm; springs ~0.2–0.4 g on every key (two part numbers, yellow keys 1–33, blue 34–73); f2/f1 5.8–7.9 | [RSM-AF figs 7-6, p. 97; calc] |
| Stretch "−2 … +2 cents" from the ch. 5 text | Factory chart: −20 c at A0 … +30 c at C8 | [RSM-AF fig. 5-4] |
| Tine Ø 1.905 mm vs Shear's 1.5 mm (§3.1) | Both right: 0.075 in is the original uniform wire and the clamped end; the later tapered/swaged tines are ~0.060 in (1.52 mm) at the free end | [RSM ch. 7, P321] |
| Tine-to-tonebar coupling to be fitted to equal-f0 Q | Model as a detuned reaction mass; the sustain comes from the tine's own low loss plus base leakage, which is least when the fork is balanced (§4.2) | [P922, MP14; calc] |
| Tremolo "triangle-like LFO", rate 1–8 Hz | Peterson: square astable 2.0–6.6 Hz driving lamps → CdS cells (rounded, asymmetric); Haigler: op-amp square → LED/LDR (sharper), ~0.3–30 Hz; Jordan: mono, phase-shift sine into a JFET | [RSM fig. 11-1, 11-8, 11-17; §6.4] |
| Pickup coil "a few hundred mH", RLC 500 Hz example | 170–190 Ω, ~70–110 mH per coil; 73 coils in 24 series groups of ~3; harp ≈ 1.4–2.1 kΩ, 0.6–0.85 H; into the 10 k Stage pot a ~2.2 kHz first-order low-pass | [FRS-PU, MORRIN, RSM ch. 10; calc] |
| Pickup p, w0 free | Pole geometry from the CBS patent; tip amplitudes from the same patent fix A/w per register (§3.9, §4.1) | [P321] |
| Tines break "in the treble" (folk view) | The manual's "heavy traffic" fatigue zone is the middle; the 111 mm kit tine covers it | [RSM ch. 5] |

## 3. The harp, key by key

### 3.1 Numbering

- 88-key pianos number tone bars 1–88 (A0–C8); 73-key pianos 8–80 (E1–E7) on later instruments,
  1–73 on earlier ones; early 88s labelled the seven bars below the first as "0" because they were
  identical **[web, RSM ch. 6]**. The cutting chart labels each bar "n73–n88" (1-8 = E1 … 73-80 = E7,
  then 74-81 … 81-88 for the 88's top octave).
- The hammer-tip table uses the 88 numbering (§3.7); the tine piano tip page uses the chart's 73-based
  numbering 0–81 **[web, RSM ch. 4; RHT]**. They agree: n88 = n73 + 7.

### 3.2 Tine lengths (factory cutting sheet, and the manual's chart)

Two primary tables exist. (a) The kit sheet "Cutting instructions for tone generators", P/N 024297
(kit of 6 tone generators #120063 with 6 tuning springs #069658 and 6 #069666), printed in inches,
"measurement is from the inside of the square rod in which the tyne is mounted"; "slight errors in
length can be compensated by the tuning springs" **[web, RSM-AF p. 97]**. (b) The manual's fig. 6-2
bar chart, drawn to scale, digitised here at 0.253 mm/pixel with a −0.4 mm zero offset **[web, RSM
fig. 6-2; calc]**. Table (a) in mm, 73-key numbers (88-key note = key + 7; key 1 = E1):

| Keys | E | F | F# | G | G# | A | A# | B | C | C# | D | D# |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1–12 (E1–D#2) | 156.4 | 152.4 | 148.4 | 143.7 | 140.5 | 135.7 | 131.4 | 128.6 | 125.8 | 121.4 | 117.9 | 113.1 |
| 13–24 (E2–D#3) | 110.7 | 108.3 | 105.2 | 103.2 | 100.8 | 98.4 | 95.2 | 92.9 | 89.7 | 87.3 | 84.9 | 81.8 |
| 25–36 (E3–D#4) | 79.8 | 77.8 | 76.2 | 74.6 | 71.4 | 70.2 | 67.9 | 66.3 | 66.7* | 63.5 | 61.9 | 59.9 |
| 37–48 (E4–D#5) | 57.9 | 56.4 | 54.0 | 53.2 | 53.2 | 50.8 | 50.0 | 48.4 | 46.4 | 45.2 | 44.1 | 40.5* |
| 49–60 (E5–D#6) | 40.5 | 40.5 | 39.7 | 38.1 | 37.3 | 36.1 | 34.9 | 34.1 | 34.1 | 32.9 | 32.1 | 31.0 |
| 61–72 (E6–D#7) | 29.8 | 29.4 | 28.6 | 27.8 | 27.0 | 27.0 | 25.8 | 25.0 | 24.6 | 24.2 | 23.8 | 23.0 |
| 73 (E7) | 22.6 | | | | | | | | | | | |

In inches as printed: key 1 6-5/32, 13 4-23/64, 25 3-9/64, 33 2-5/8, 37 2-9/32, 49 1-19/32,
61 1-11/64, 73 57/64. Springs: **#069658 "Yellow" on keys 1–33, #069666 "Blue" on keys 34–73**
**[web, RSM-AF p. 97]**: only two spring types for the whole keyboard (§3.3).

- (*) Probable misprints or scan misreads: key 33 (2-5/8 in) is longer than key 32; keys 48–50 all
  read 1-19/32 in after 47 = 1-47/64 (a 3.6 mm step where fig. 6-2 is smooth: 44.1/42.6/42.3 mm,
  minus its ~1 mm right-comb offset). Where (a) and (b) disagree, prefer a smooth curve through both.
- (b) agrees with (a) within ±1.6 mm for keys 1–46; its right-hand comb (keys 47–81) reads on
  average +1.1 mm long. Fig. 6-2 also gives the 88's top octave (keys 74–81 = F7–C8): 22.3, 21.6,
  21.1, 20.3, 19.6, 19.3, 18.6, 18.0 mm (subtract ~1 mm), and bar "0" for the 88's A0–D#1 = 156.4 mm,
  the same as E1 **[web, RSM fig. 6-2; calc]**.
- Lengths are production cutting lengths; the spring takes up the difference. Replacement tines come
  long (kit tine 4-3/8 in = 111.1 mm for the middle) and are cut to this chart **[web, RSM ch. 5–6]**.

### 3.3 Tine section, taper and the spring mass per key

- Three generations, with the dimensions from the manual's figures **[web, RSM-AF figs 7-4…7-6;
  RSM ch. 7]**:
  1. original: uniform piano wire 0.075 in (1.905 mm); 40 000 blows to failure in tine piano's test;
  2. tapered by centerless grinding: 0.100 in (2.54 mm) at the root, tapering over 1 in (25.4 mm) to
     0.070 in (1.78 mm), then 0.060 in (1.524 mm) to the tip; 1 500 000 blows;
  3. **swaged (current, from ~1975): 0.100 in (2.54 mm) for the first 1/2 in (12.7 mm), then a short
     transition to 0.060 in (1.524 mm) for the rest**; > 6 000 000 blows.
  This matches the 1975 CBS patent's 0.060 in free end **[web, P321]** and Shear's 1.5 mm "with
  widening at the base" **[web, Sh11]**. Taper lengths differ by maker (Torrington's swage shorter
  than Raymac's taper **[web, RL-T]**): mode ratios and strike points differ by maker.
- The cutting sheet's two spring types decide between models **[calc, FE beam, E = 200 GPa,
  ρ = 7850 kg/m³, spring as a point mass 3 mm in from the tip]**:
  - a uniform 1.905 mm tine would need a 0.67 g spring at E1 falling to *zero* above ~C7 (the chart
    length equals the bare length at E7), which contradicts the blue springs listed up to key 73;
  - the **swaged geometry** (12.7 mm × 2.54 mm root, then 1.524 mm) needs an almost constant spring
    mass, **0.18–0.31 g on keys 1–33 and 0.19–0.39 g on keys 34–73**, matching two spring part
    numbers. So the chart is cut for swaged tines with springs of ~0.2–0.4 g everywhere; the stiff
    root is outside the block (measurement "from the inside of the square rod").
- Spring mass and mode ratios with the swaged geometry, chart (a) lengths **[calc]**:

| Key | f0 (Hz) | L (mm) | spring (g) | f2/f1 | f3/f1 | modes < 20 kHz |
|---|---|---|---|---|---|---|
| A0 (88) | 27.5 | 156.4 | ~1.3 | ~9.2 | ~28 | 3+ |
| C1 (88) | 32.7 | 156.4 | ~0.75 | ~7.9 | ~24 | 3+ |
| E1 | 41.2 | 156.4 | 0.27 | 6.74 | 19.5 | 3+ |
| C2 | 65.4 | 125.8 | 0.24 | 6.79 | 19.6 | 3+ |
| E2 | 82.4 | 110.7 | 0.30 | 7.05 | 20.5 | 3+ |
| C3 | 130.8 | 89.7 | 0.24 | 7.01 | 20.1 | 3+ |
| C4 | 261.6 | 66.7 | 0.18 | 7.01 | 19.1 | 3 |
| E4 | 329.6 | 57.9 | 0.23 | 7.40 | 19.6 | 3 |
| C5 | 523.3 | 46.4 | 0.26 | 7.69 | 18.8 | 3 |
| E5 | 659.3 | 40.5 | 0.33 | 7.92 | 19.0 | 3 |
| C6 | 1046.5 | 34.1 | 0.28 | 7.02 | 17.6 | 3 |
| E6 | 1318.5 | 29.8 | 0.35 | 6.94 | 19.4 | 2 |
| C7 | 2093 | 24.6 | 0.38 | 6.17 | 20.2 | 2 |
| E7 | 2637 | 22.6 | 0.31 | 5.81 | 19.2 | 2 |

- Gabrielli et al. measured 7.11 and 20.25 × f0 on F1 **[web, Ga20 via So24]**; the swaged model gives
  6.7–6.8 and 19.5–19.6 at E1–F1: within 5 %, the residual being the spring's position and the
  swage's transition. The 88's lowest keys need much heavier springs (~0.4–1.3 g on the same
  156.4 mm tine) and have f2/f1 up to ~9: the bottom octave of an 88 is acoustically a different tine.
- Superseded: the uniform-1.905 mm μ table of the first version of this note (and `electric_pianos.md`
  §3.2) applies only to the original 1960s wire; use the swaged table for 1975+ instruments and the
  tapered (stage 2) profile for 1970–75 **[design]**.
- The spring: a crimped helical coil, friction fit, "moved by hand with some effort"; low-pitch springs
  heavier with more turns, high-pitch ones "only a few turns"; range "as much as 1-1/2 steps above or
  below optimum" **[web, RSM ch. 5; P922]**. Detuning by sliding it also moves the tine against its
  tonebar; tine piano claimed a target G can be retuned to F…A "without appreciable loss of support" from
  the bar **[web, RSM ch. 1]**: the bar's resonance is broad (§3.5).

### 3.4 Tuning practice and stretch

- Factory stretch chart (fig. 5-4, read from the scan **[web, RSM-AF p. 36]**), cents from equal
  temperament, A4 = 440 Hz = 0:

| Range | Offset (cents) |
|---|---|
| A0, A#0, B0, C1, C#1, D1, D#1, E1, F1, F#1, G1, G#1, A1, A#1 | −20, −19, −18, −17, −16, −15, −14, −13, −12, −11, −10, −9, −8, −7 |
| B1–C2 | −6 |
| C#2–D#2 | −5 |
| E2–G#2 | −4 |
| A2–F3 | −3 |
| F#3–G3 | −2.5 |
| G#3–D4 (middle C −2) | −2 |
| D#4 | −1.5 |
| E4–G#4 | −1 |
| A4–D5 | 0 |
| D#5–G#5 | +1 |
| A5–C6 | +2 |
| C#6–D#6 | +3 |
| E6–F6 | +4 |
| F#6–G6 | +5 |
| G#6–A6 | +6 |
| A#6, B6, C7, C#7, D7, D#7, E7 | +7, +8, +9, +10, +11, +12, +13 |
| F7, F#7, G7, G#7, A7, A#7, B7, C8 | +15, +17, +19, +21, +23, +25, +27, +30 |

  A 50-cent total stretch over the 88 keys, much larger at the ends than the text in ch. 5 suggests.
  With zero inharmonicity this stretch is psychoacoustic only, and it is the factory reference for
  the model's tuning table.
- Measured partials of the output are exactly harmonic (F1: 2.0000 × f0 at 0.67 Hz resolution; zero
  inharmonicity in TuneLab) **[web, snippet: EPF08]**: the output's overtones after the attack are
  pickup harmonics of a sinusoidal tine, not tine modes. Model consequence: no stretched partials in
  the sustain.
- Notes drift because of a loose spring or "Tine fatigue under the stress of constant heavy blows"
  **[web, RSM ch. 5]** (§8).

### 3.5 Tonebars

- History **[web, RSM ch. 7; P922; P656]**: (1) one-piece cast iron fork in a channel (1959 patent:
  bar ≥10×, preferably ≥25× the tine's mass; ~1 in deep perpendicular to the plane); (2) removable
  generator; aluminium bars of 3/8 in square were used at some point **[P656 background]**; (3) the
  "twisted steel bar" (patent 3,644,656, filed 1970): flat steel ~1/8 in (3.2 mm) thick, 0.490 in
  (12.4 mm) wide (0.437 in, 11.1 mm, for the higher group), twisted 90° near the base so that the
  *wide* dimension lies in the vibration plane along the working length. Later bars are brass in some
  descriptions **[web, PM17, MP14]**: not resolved here which years.
- Groups in the patent: I-1…I-25 low (twist offset ~1.5 in from the mount; a ~1/4 × 1 in slot adds
  compliance and lowers f without a counterweight; I-1…I-12 also have a ~1/4 in rebate over ~2 in so
  the swinging bass tine cannot hit the bar), I-26…I-53 mid, I-54…I-73 high (no twist: "overtones are
  not a problem") **[web, P656]**. Example overall lengths: I-1 11.12 in (282 mm), I-12 8.62 in
  (219 mm) **[web, P656, via page text]**.
- Tuning relation: the bar "has a low Q", resonates with the tine "over a range"; in low and mid the
  bar's natural frequency "may differ from the tine's by about 2.5 piano tones", at the top "under
  half a tone" **[web, P656]**. The 1959 patent: maximum dwell when tine and bar coincide; low notes
  deliberately off-peak to bring the dwell down to ~17 s; high notes at the peak, 3–5 s **[web,
  P922]**.
- Measured (Mark I, impulse hammer on the bar, piezo on the bar) **[web, MP14 table 1]**:

| Bar | Note (Hz) | Bar's own f (Hz) | Interval | Bar vs tine phase |
|---|---|---|---|---|
| 12 | E♭2 79 | 51 | −755 c | anti |
| 19 | B♭2 118 | 69 | −929 c | anti |
| 26 | F3 176 | 79 | −1387 c | in |
| 33 | C4 263 | 105 | −1590 c | in |
| 40 | G4 393 | 138 | −1811 c | in |
| 47 | D5 588 | 183 | −2020 c | anti |
| 54 | A5 880 | 140 | −3176 c | anti |
| 61 | E6 1316 | 145 | −3815 c | anti |
| 68 | B6 1969 | 222 | −3778 c | in |

  (intervals **[calc]**). These "bar f0" values grow far more slowly than the note and are probably
  the assembly's lowest mode on its soft mount (grommet + two coil springs), not the bar's bending
  mode near the note; the paper does not say. Either way the bar is driven far from its resonance and
  "enslaved" at the tine's frequency, in or anti phase, and its own modes appear only in the
  transient, adding the "glockenspiel" component, more in the upper register **[web, MP14, PM17]**.
- **A conflicting observation (Moog 1980).** Bob Moog, explaining the tine piano in *Contemporary
  Keyboard* (April 1980, reposted on the EP Forum), says the mounting post is not rigid, the
  resonator (tonebar) is "tuned to be very near" the tine's frequency, and energy goes back and forth
  between them "for several seconds"; on a low note with the top off "after a second or so the
  resonator will be vibrating and the tone bar [tine] will be almost motionless"; the exchange gives
  the overtones "complex envelopes" and slightly detunes the fundamental against them **[web,
  EPF2896, forum repost of a magazine text; no measurements]**. This is the near-tuned two-oscillator
  picture (beating exchange, two-stage decay), opposite to MP14's enslaved, far-detuned bar. Both can
  be true on different instruments and keys: the patent's ±2.5-tone design window and a sliding
  tuning spring (±1.5 semitones) allow anything from near coincidence to far detuning. Model: make
  the bar's detuning a per-key parameter; near 0 it gives the classic ladder's slow exchange (a beat of period
  1/Δf of a few seconds in the pickup signal, since the pickup reads only the tine), far it gives
  MP14's slaving **[design]**. The rest of the thread has no measured modes **[web, EPF2896]**.
- Mounting: each tone-bar assembly sits on the tonebar rail on a soft elastomer grommet, with two
  adjustable coil springs setting its height 3/16–1/2 in (factory 3/8 in = 9.5 mm) **[web, RSM ch. 4;
  P418]**. A 1983 patent changed to a single screw per fork through a soft elastomer block "to
  lengthen the dwell of high-pitched forks" **[web, P418]**: the treble's sustain is limited by mount
  leakage. Apple's notes say the Mark II lacks the earlier "treble resonance clamps" and sustains
  less in the treble **[web, APL]** (unverified elsewhere).

### 3.6 Strike line and escapement

- Striking line, leading edge of the hammer tip to leading edge of the tone generator: ~57 mm
  (2-1/4 in) at the extreme bass bar, ~3.2 mm (1/8 in) at the extreme treble **[web, RSM ch. 4]**.
  Interpolating linearly in key position (the harp and the hammer rail are straight) and adding half
  a tip width (0–4 mm), the strike point as a fraction of the tine length is **[calc]**:

| Bar (note) | L (mm) | strike from block (mm) | x = s/L |
|---|---|---|---|
| 1 (E1) | 156.4 | 57–61 | 0.36–0.39 |
| 13 (E2) | 111.1 | 48–52 | 0.43–0.47 |
| 21 (C3) | 89.9 | 42–46 | 0.47–0.51 |
| 33 (C4) | 65.1 | 33–37 | 0.51–0.57 |
| 45 (C5) | 46.4 | 24–28 | 0.52–0.61 |
| 57 (C6) | 34.7 | 15–19 | 0.44–0.55 |
| 69 (C7) | 25.1 | 6–10 | 0.25–0.41 |
| 73 (E7) | 22.6 | 3–7 | 0.14–0.32 |

  So the middle is struck near half the length (close to mode 3's node at 0.50 L), the top near the
  root. The "Percussion Pedal" of Dyno-My-Piano moved the whole harp with a knee lever to change
  this in real time **[web, FR-DY]**.
- Each tine maker's tine has its own optimal strike point; mixed Torrington/Singer harps (1976) have
  notes that "don't sustain or bark" when the harp position suits their neighbours **[web, RL-T]**:
  per-key strike error is a real flaw (§8).
- Escapement (hammer-tip edge to tine with the key held down): ideal 1/32 in (0.8 mm), impossible in
  the bass because of "the whipping action of the Tine"; latest design: extreme bass bar 1/4–3/8 in
  (6.4–9.5 mm), extreme treble 1/32–3/32 in (0.8–2.4 mm); early design: bass 3/16–3/8 in, mid
  1/16–1/8 in, treble 1/32–3/32 in **[web, RSM ch. 4, ch. 10]**. The hammer is in free flight over
  the escapement; the tips' heights are graded (§3.7) to keep touch similar.
- Key dip 3/8 in ± 1/32 (9.5 mm) **[web, RSM ch. 4]**.

### 3.7 Hammers and tips by key and era

| 88 no. (73 no.) | Notes | Shore A | Colour | Tip height | E (Gent) |
|---|---|---|---|---|---|
| 1–30 (0–23) | A0–D3 | 30 | black | 1/4 in, 6.35 mm | 1.1 MPa |
| 31–40 (24–33) | D#3–C4 | 50 | red | 5/16 in, 7.94 mm | 2.5 MPa |
| 41–50 (34–43) | C#4–A#4 | 70 | yellow | 3/8 in, 9.53 mm | 5.5 MPa |
| 51–64 (44–57) | B4–C6 | 90 | black | 7/16 in, 11.11 mm | 21 MPa |
| 65–88 (58–81) | C#6–C8 | maple + 3 layers shrink tube | – | 7/16 in, 11.11 mm | wood core |

**[web, RSM ch. 4; RHT; E calc in electric_pianos.md §3.5]**. Musicians often extended the 90
tips to hammer 67 or beyond "to avoid the sharp change in tone" at the wood-tip boundary **[web,
RSM ch. 4]**: the zone boundaries are audible steps, and a model should reproduce them (an
authentic flaw) with a smoothing option.

- Eras **[web, RHT, CEP-M, CEP-T, WP-R]**: felt, voiced per note like piano hammers (1965 to ~1970);
  3/8 in square ("cube") neoprene glued to a maple head, early 1971 to early 1976 ("deep lows, clear
  highs"); graduated heights, then a "somewhat triangular" profile because only one edge touches the
  tine, from 1976 ("the most dramatic change in tone"); hammer wood/plastic hybrid until mid-1975,
  then one moulded plastic piece. Mark V (1984): redesigned pedestal and hammer, "increasing dynamic
  play" **[web, CEP-T]**.
- Contact time measured on a Mark I: 6.42 ms on average (7532 fps camera) **[web, MP14]**. For the
  hammer model: a 6.4 ms contact with an 11 g hammer needs an effective contact stiffness of
  ~π²·M/τ² ≈ 2.6 kN/m (half-sine) **[calc]**, the order of the bass/mid tine's own stiffness at the
  strike point (~7 kN/m at E1, `electric_pianos.md` §3.5): the contact is tine-limited in the bass
  and middle, as argued there. Sønderbo's Hunt–Crossley tip on a rigid wall gives 0.6–1.5 ms: the
  tip stiffness alone does not set the contact time.
- The hammer is braked against rebound after the blow (a 1980 tine piano patent adds restraint "to
  prevent bouncing, rebounding" **[web, P848]**): double strikes exist on older actions (§7).

### 3.8 Dampers

- Felt on an aluminium leaf-spring arm, pulled off by a bridle strap from the hammer; modules of 12;
  bass arms full width and stiffest with long wide felts, middle medium, treble short narrow felts
  **[web, RSM ch. 2]**. Clearance with the key down 3/8–1/2 in (9.5–12.7 mm) **[web, RSM ch. 4]**.
  The felt "should bear against the Tine sufficiently firm so that the sound will be damped
  immediately following the Hammer blow" **[web, RSM ch. 4]**.
- The Mark V changed the dampers because the original "caused a snap-back that dampened overtones";
  the new dampers let the tines ring more freely ("more bell-like") **[web, FR-M5]**. Reading: on
  Mark I/II the returning damper briefly touches the tine during the attack (or on fast repetition),
  cutting the high partials early; a model needs a per-era "damper bounce" that applies a short
  extra loss in the first tens of ms **[design]**.
- Release: no published T60 of a damped tine piano note. Sønderbo's spring–dashpot damper
  (K1 = 100 N/m, R = 0.5 kg/s) **[web, So24]** and the felt's role suggest 50–200 ms to −60 dB in the
  middle **[memory]**; the bass, with its long tine and large amplitude, rings audibly through the
  damper ("thunk") **[web, snippet: forum reports]**.

### 3.9 Pickup position, voicing and the pole tip

- Volume (gap, horizontal, tine end to pole): 1/16–1/8 in (1.6–3.2 mm); from March 1972, 0.020 in
  (0.51 mm) possible in the middle and treble; "the smaller the gap … the more pronounced the DYNAMIC
  RESPONSE", defined as "percentage of volume increase in response to increased weight of touch"
  **[web, RSM ch. 4]**. tine piano's later tine-settings page: bass 1/16 in, middle 1/32 in (0.8 mm),
  wood tips 1/16 in **[web, RTS]**.
- Timbre (vertical: tine rest height relative to the pole tip's edge): "slightly above dead center",
  by ear **[web, RSM ch. 4]**. tine piano's settings page: lowest 15 notes "mostly fundamental", rest
  "most preferred", wood-tip zone "mostly fundamental" **[web, RTS]**: i.e. bass and top set further
  off the edge (δ/w larger) than the middle.
- The manual's timbre diagram (fig. 4-8) draws the pole tip as a cone and three settings: tine
  well above the cone's point "PURE FUNDAMENTAL (DEEP)", slightly above "MORE OVERTONE (IDEAL)", on the
  axis "PURE OVERTONE (SHALLOW)"; the volume diagram (fig. 4-9): "LOW VOLUME", "IDEAL", "TOO CLOSE"
  **[web, RSM-AF p. 29]**. Read off the drawing, "ideal" is roughly one tine diameter above the axis
  (~1–1.5 mm, δ/w ≈ 0.4–0.7) and "deep" two or more (δ/w ≳ 1) **[calc, rough]**.
- Pole geometry from the 1975 CBS patent (Seth Lover; not certain it went into production, but it
  describes the tine-piano geometry it improves on) **[web, P321]**: mild-steel (SAE 1010) pole
  piece, shank 0.197 in (5.0 mm) Ø, head 0.25 in (6.35 mm) Ø, an Alnico 5 rod magnet 0.5 in long ×
  0.1875 in Ø (12.7 × 4.8 mm) behind it, magnetised lengthwise, "weak enough to avoid excessive
  magnetic drag on the tine"; the production tine piano pole ends in a chisel (wedge) edge; the patent's
  version has two peaks ~0.2 in apart with a 60° groove ~1/8 in deep between them, the tine resting
  ~0.030 in (0.76 mm) from the base line, "roughly 1/8 in from the pole axis … on a steep part of
  the field-strength curve". The production pole's chisel tip with flattened sides is confirmed by
  FEM work **[web, PM17]**; Muenster describes a round ferrite magnet with a wedge-shaped end
  **[web, MP14]** (another era or a misdescription).
- Hence w0 (the bell's half-width) ≈ the gap plus ~1 mm (the edge's radius of field), i.e. 1.5–4 mm
  **[calc]**; amplitudes 0.4–3.2 mm (§4.1) give A/w from ~0.1 (treble, soft) to ~2 (bass, hard).
- Magnetic drag: the magnet pulls the tine (a negative spring) and eddy currents in the pole damp it.
  The patent's "weak magnet" requirement shows it matters; Apple's notes say the magnets "damp" the
  tine **[web, P321, APL]**. The production history confirms it: magnets were 1 in long on early
  pickups and were shortened to 1/2 in in the early 1970s because the long magnet "would influence
  the decay properties of the vibrating tines"; the 1970–71 pickups with a "longer and stronger
  magnet" gave "shorter dwell and pitch shifting" **[web, FRS-PU, VV-HIST]**. Not quantified: model
  as a per-key negative stiffness (pitch falls as the gap closes; size it so that a 1970–71 magnet at
  0.5 mm gap shifts a few cents and halves T60 in the bass) and an eddy loss **[design]**.

## 4. Measured behaviour

### 4.1 Motion and amplitude

- After an "extremely short transient" the tine moves as a pure sine; two tracking points near the
  strike and two near the tip "vibrate completely simultaneous"; the y and z motions are "perfectly the
  same in pitch and phase" (tine on a table, rubber hammer) **[web, MP14]**. The waveform becomes
  steady after 10–14 ms **[web, MP14]**. On the instrument the horizontal polarisation is smaller and
  excited by coupling or tip imperfection **[web, Pf17]**.
- Steady-state tip amplitude "normal" playing **[web, P321]**: ~3.2 mm at C3 (131 Hz), ~0.40 mm at C6
  (1047 Hz). Peak tip velocity 2.6 m/s in both cases **[calc]**: so with equal touch, A ∝ 1/f and
  the pickup voltage (∝ dΦ/dx · v) is roughly level across the keyboard, as intended.
- Implied A at other keys for the same touch: E1 10 mm, E2 5 mm, C4 1.6 mm, C5 0.8 mm, E7 0.16 mm
  **[calc]**. E1 at 10 mm on a 156 mm tine is 6 % of its length: still linear as a beam, but beyond
  the pickup gap; bass tines at ff hit the tonebar rebate, the damper felt and the pickup (§7).

### 4.2 Decay of the fundamental

| Register | Shear Q → T60 (1974 Mk I 88) | tine piano design target | Model T60 (mf, clean voicing) |
|---|---|---|---|
| E♭2 (78 Hz) | 949 → 27 s | low notes ~17 s | 20–27 s |
| E♭3 (156 Hz) | 731 → 10 s | | 10–14 s |
| B3–E♭4 (247–311 Hz) | 1040–1520 → 9–11 s | | 9–11 s |
| E♭5 (622 Hz) | 2175 → 7.7 s | | 6–8 s |
| E♭6 (1245 Hz) | 1761 → 3.1 s | high notes 3–5 s | 2.5–4 s |
| E7 (2637 Hz) | – | | 1–2 s [calc: Q ~1500] |

**[web, Sh11, P922; T60 = 6.91·Q/(π f0), calc]**. Shear's τ is measured on the pickup signal, so it is
the fundamental's envelope; in the bass at forte the pickup's compression (§4.4) lengthens the
*apparent* early decay of the fundamental.

- Mechanism **[calc, standard tuning-fork physics; consistent with P922]**: a balanced fork cancels
  the bending moment at the base, so little energy leaks into the harp rail; an unbalanced one (bar
  detuned) leaks through the base into the mount. The tine's mode loss is therefore
  1/Q = 1/Q_tine + 1/Q_leak(Δf), with Q_leak largest near bar–tine coincidence. tine piano used the
  detuning to *shorten* the bass dwell. The bar's own (lossy) mode, excited by the blow, dies within
  the transient. Model: one tine mode per polarisation with its Q from the table, plus 1–2 bar modes
  at the bar's own frequency (§3.5) with Q ~ 20–100 excited through the base reaction, contributing
  only to the first 10–50 ms **[design]**.
- Two-stage decay: not reported in the measurements found. A tine piano note decays nearly
  exponentially in the fundamental; what looks like a fast initial drop in recordings is the pickup
  harmonics (next point).
- **Harmonics decay faster than the tine.** For A/w < ~0.5 the pickup's n-th harmonic ∝ (A/w)^n, so
  in dB it decays n times faster than the fundamental: at C4 (T60 10 s) H2 has T60 5 s, H3 3.3 s, H4
  2.5 s **[calc]**. This matches the observation that "higher harmonics decay much faster" on a Mark
  I **[web, Woo13]**. At larger A/w (bass, ff) the harmonics first stay level or *rise* relative to
  H1 as the amplitude falls into the bell's steep part (§4.4).

### 4.3 The attack: inharmonic partials and the bell

- Sources of the inharmonic attack: (1) tine modes 2 and 3 (5.8–9.2 × f0 and 17.6–28 × f0, §3.3);
  (2) the tonebar's own modes (inharmonic, "more prominent in the upper register") **[web, PM17,
  Ga20]**; (3) intermodulation of these through the pickup (sum and difference tones with the
  fundamental and its harmonics) **[web, Ga20 abstract]**.
- Duration: "after ca. 10–14 ms the waveform becomes sinusoidal" (bar accelerometer) **[web, MP14]**;
  Shear: the inharmonic partials "dissipate quickly" **[web, Sh11]**. Mode 2 of a tine has Q of
  the order of the fundamental's in a free steel rod, so their fast death must come from contact,
  the clamp, the spring (a lossy coil at the antinode region of mode 2), and the bar **[calc,
  reasoning]**: in the model give mode 2 a T60 of 30–150 ms and mode 3 10–50 ms, shortest in the bass
  **[design; to fit to Ga20's data]**.
- Excitation by register **[calc, §3.6 + contact time]**: mode 2 at C4 is at ~1.7 kHz (period
  0.6 ms) under a ~4–6 ms contact: the smooth contact force has little energy there, so mode 2 is
  weak in the middle; at C6–E7 (contact ≲1 ms with the wood tip, mode 2 at 6.6–16.5 kHz) and with a
  strike near the root (x ≈ 0.15–0.4, where mode 2 and 3 shapes are large) the treble keys get their
  "tink". Sønderbo confirms both the strike-point and stiffness effects in his model, the first
  stronger **[web, So24]**.
- Hammer tip hardness: "too soft a tip gives a thunk, too hard a clank with stronger upper
  harmonics"; owners converted soft tips to hard for brightness **[web, RHT]**.

### 4.4 Pickup level and harmonics vs amplitude (bell function, p = 1)

Using `electric_pianos.md` §3.4's Φ(x) = Φ0 [1 + ((x+δ)/w)²]^(−p) and a sinusoidal tip x = A sin ωt,
levels of V = dΦ/dt relative to the A/w = 0.3 fundamental at the same voicing **[calc]**:

| δ/w | A/w | H1 (dB) | total (dB) | H2−H1 | H3−H1 |
|---|---|---|---|---|---|
| 0.5 | 0.1 | −9.1 | −9.1 | −34 | −43 |
| 0.5 | 0.3 | 0 | 0 | −22 | −24 |
| 0.5 | 0.5 | +3.4 | +3.7 | −15 | −14 |
| 0.5 | 0.7 | +4.8 | +5.9 | −9 | −8 |
| 0.5 | 1.0 | +4.9 | +8.6 | −2 | −2 |
| 0.5 | 1.5 | +2.8 | +11.9 | +5 | +4 |
| 0.5 | 2.0 | −0.1 | +13.9 | +10 | +8 |
| 0.25 | 1.0 | +3.2 | +12.5 | +8 | 0 |
| 1.0 | 1.0 | +9.2 | +9.8 | −13 | −13 |
| 1.0 | 2.0 | +7.4 | +14.8 | −2 | +4 |

- Over a 26 dB range of tine amplitude (A/w 0.1 → 2) the total output rises 23 dB (nearly linear)
  but the fundamental only 9 dB net (14 dB to its peak near A/w ≈ 0.8, then *falling*): at ff in the bass the
  fundamental is weaker than at mf and the energy is in H2–H5. The growl is this, plus intermodulation
  of the attack partials **[calc; consistent with MP14 "the more growl"]**.
- In time: a bass note struck at A/w = 2 decays through A/w = 1 and 0.8, so the fundamental *rises*
  by ~5 dB during the first part of the decay while the harmonics fall: a "bloom" or swell lasting
  ~(ln 2.5)·τ ≈ 0.9 τ, i.e. 1.5–3.5 s in the bass (τ 1.5–3.9 s, §4.2) **[calc]**. a commercial piano model exposes a "Blooming" parameter
  for its electric pianos **[web, MOD]**; whether it models this is not documented. A validation
  target (§13).
- Voicing at the edge (δ/w ≈ 0.25) gives H2 above H1 already at A/w = 1: "the fundamental and all odd
  partials are attenuated, leaving the second partial as the dominant" when centred **[web, Sh11]**;
  Dyno-My-Piano set tines to "emphasize the overtones more than the fundamental" with the pickups
  "as close to the tines as possible" **[web, FR-DY]**: the 1980s "bell" tine piano is δ/w small and
  gap small, with low amplitudes (light touch, hard tips).

### 4.5 Beating and slow modulations

- Design intent: tine and bar vibrate "in one plane, so the plane does not rotate and cause beating"
  **[web, P922]**. Real tines have a second polarisation (smaller, §4.1); a round tine with a spring
  has nearly degenerate polarisations, split by the block's and bar's different stiffness in and out of
  plane. Through the pickup (the horizontal motion modulates the bell width w, `electric_pianos.md`
  §3.4) the split gives a slow amplitude/timbre beat. No measured split was found: model 0.02–0.3 Hz
  per key, random **[design]**.
- Tine vs bar: no steady beating, since the bar is enslaved at the tine's frequency **[web, MP14]**;
  a transient beat between the free bar mode and the tine during the first 10–50 ms is possible but not
  reported.
- the classic ladder's description of a low note's energy moving into the tonebar within ~1 s (tine almost still)
  implies a slow tine-amplitude beat when bar and tine are close in frequency **[web, EPF2896]**; the
  pickup sees it as a dip and recovery of the whole note (all harmonics together, the high ones by
  n× in dB). Not reported in the measured papers; a per-key option (§3.5).
- Shear's electromagnetic drive showed audible beating whenever the driving frequency differed
  slightly from the tine's **[web, Sh11]**: the tine's resonance is sharp (Q ~1000), so any second
  source near f0 (sympathetic coupling from a neighbouring key through the rail, octave unisons under
  the sustain pedal) beats audibly **[calc]**.
- Sympathetic resonance: with the sustain pedal down, tines and bars are coupled through the tonebar
  rail and harp; no measurement found. Model at −40 to −60 dB as an option **[design]**.

### 4.6 Velocity response and dynamic range

- Hammer speed range used in models: 0.5–4 m/s **[web, So24]** (≈18 dB). The tine is linear, so
  tine amplitude ∝ hammer speed over that range; the tine piano action's ratio from key to hammer is not
  published **[open]**.
- Output: in the treble (A/w ≲ 0.5 up to forte) the output level follows the tine almost linearly,
  ~18–25 dB from pp to ff; in the bass the total level still rises ~5 dB per doubling, but the
  fundamental stalls and then falls above A/w ≈ 0.8, so the *perceived* loudness grows less while the
  timbre changes strongly: Muenster & Pfeifle: "velocity sensitivity is to be distinguished by a
  change in volume to lesser extent than in sound. Playing softly the fundamental comes up, playing
  harder the more and more growl appears" **[web, MP14]** **[calc for the dB]**.
- The service manual's "dynamic response" (§3.9) is this: a small gap gives a higher A/w at the same
  touch, so a larger change of level and timbre per unit touch.

## 5. Variants that change the sound

| Era / model | Tines | Tips / hammers | Tonebars, pickups, other | Sound reports |
|---|---|---|---|---|
| 1959–65 Piano Bass, Pre-CBS | uniform 0.075 in wire | felt | cast iron bars (1959 patent) | – |
| 1965–69 tine electric piano ("Sparkle Top") | Raymac | felt, voiced per note | early bars; Suitcase: mono tremolo, Jordan amp | "darker and mellower attack"; Raymac bark "entirely different" [CEP-T] |
| 1969–early 1971 Mark I (early) | Raymac | felt → neoprene | twisted steel bars; stereo vibrato from 1969 | "warm bark" |
| 1971–1976 Mark I | Torrington (swaged, shorter taper) | cube neoprene 3/8 in on maple head | 0.5 mm gap possible from 3/1972; 88-key from 1973 | "deep lows, clear highs" [CEP-M] |
| mid-1975 | – | all-plastic hammer, new contour ("tighter") | – | [CEP-T] |
| 1976–77 | Singer (mixed with Torrington in 1976) | graduated / triangular tips | – | "most dramatic change in tone" [CEP-T]; mixed strike points |
| late 1977–1979 Mark I | Schaller | graduated | Peterson 80 W → Janus 100 W (1977) | toward "bell" |
| 1979–82 Mark II (Stage, Suitcase, 54) | Schaller | graduated; plastic keys optional from 1980 | flat top; highest pickup failure rate 1979–80 | "more bell-like, less deep and barky" [CEP-T] |
| 1984 Mark V | – | new pedestal + hammer (more dynamic, less reliable) | new dampers; wooden keys; Stage 73 + MIDI with 3-band EQ | "more bell-like", "a little more clarity" [FR-M5, CEP-T] |

**[web, CEP-T, CEP-M, RL-T, RHT, FR-M2, FR-M5, WP-R]**. Two caveats from the same sources: "up to
80–90 % of tone depends on maintenance and setup" **[web, CEP-M]**, and Apple: the characteristic
sound "depends more on its adjustment and maintenance than on its model"; many 1980s pianos were
adjusted for more bell **[web, APL]**. So the model's era presets are mostly *setup* presets (gap,
δ, tip hardness, strike point), plus the tine taper (overtone ratio) and the damper bounce.

- **Suitcase vs Stage** is electronics only: the same harp; the Suitcase adds the preamp, vibrato,
  power amp and the 4×12 in cabinet, the Stage feeds an external amp from the passive harp (§6). The
  Suitcase's speaker cabinet is the instrument's sound in the room; the Stage's sound is whatever amp
  it was played through.

## 6. Electronics

Schematics read from the service manual's ch. 11 figures (11-1 Haigler/Janus 100 W preamp, 11-3
100 W power amp, 11-8 Peterson 80 W preamp, 11-9 80 W power module, 11-10 80 W supply, 11-17 Jordan
50 W) and ch. 10 (harp wiring) **[web, RSM ch. 10–11]**; values below are printed on them unless
marked [calc] or tagged otherwise. Pickup and harp data from Steve Woodyard's tine piano service pages
and Vintage Vibe **[web, FRS-PU, VV-PDF, VV-HIST]**.

### 6.1 The pickup as a circuit element

- **Coil:** DC resistance 170–190 Ω "depending on its vintage" **[web, FRS-PU]**; ≈180 Ω
  **[web, VV-PDF]**; wire 0.0045 in = AWG 37 **[web, snippet: EPF-6277]**; turns unpublished
  (a forum guess "< 2500"). The physical pickup changed over the years but "the differences were not
  electrically significant" (Woodyard) **[web, FRS-PU]**. Each coil is wound in two sections in
  opposite sense for hum cancelling **[web, MP14]**.
- **Inductance:** no single-coil measurement found. A whole 1973 Stage 88 harp measured 0.854 H and
  2.1 kΩ **[web, MORRIN]**. With groups of 3 the harp's L ≈ (groups/3)·L_coil ≈ 8 L_coil for a 73,
  so **L_coil ≈ 70–110 mH** **[calc]**; the tine's iron near the pole raises L slightly and
  modulates it with the tine's position (a small parametric effect, neglected).
- **Magnet:** early pickups 1 in long magnet; 1970–71 a longer, stronger magnet with a larger bobbin
  (shorter dwell, pitch shift); from the early 1970s 1/2 in **[web, FRS-PU, VV-HIST]**; 1970–71
  magnets north-facing, all other years south-facing (mixing them needs the magnet and pole flipped)
  **[web, VV-HIST]**. Eras: 1965–69 long-leaded, wired in series on Sparkle Tops and Piano Bass;
  1972–73 short magnet; 1974–79 the common type (3 in bracket); 1980–84 "white" pickups with high
  failure rates from corrosion **[web, VV-HIST, CEP-T]**. Alloy: Alnico 5 in the CBS patent
  **[web, P321]**; "ferrite" in MP14; no primary production spec found.
- **EMF:** V = −N dΦ/dt (electric_pianos.md §3.4). Absolute scale: the Jordan amp's input is marked
  "0.2 V PEAK MAX — KEYBOARD" **[web, RSM fig. 11-17]**; with groups of 3 a single coil's EMF reaches
  the harp output divided by ~3 (its group partners shunt it) **[calc]**, so one coil gives up to
  ~0.6 V peak at ff, and roughly 20–100 mV at mf **[calc: §4.4 levels]**.
- **Faults that change sound:** a magnet loose in its bobbin gives distortion/microphonics (fix:
  glue) **[web, FRS-PU]**; open coils (1979–80); weak magnets **[web, snippet: YT]**.

### 6.2 The harp network

- **Grouping:** early 73s: parallel groups of 7, 6, 6, 6 … in series; from the early 1970s to 1985:
  4, 3, 3, 3 … (4 + 23 × 3 = 73, 24 groups in series) **[web, FRS-PU]**. Ch. 10 shows the conversion
  (two bus wires, breaks alternating top/bottom; originally every 6, after modification every 3):
  "basic voltage output was quadrupled … The output impedance then becomes approximately 2500 ohms"
  **[web, RSM ch. 10]**. For N coils in groups of n, one struck coil gives ~e/n at the output and
  R_harp ≈ N·R/n²: 6 → 3 doubles the voltage and quadruples the impedance (4× the power into a
  matched load) **[calc]**. Groups alternate in polarity "to reduce electromagnetic interference"
  **[web, Sh11]**.
- **Harp source impedance:** R ≈ 1.4 kΩ for 73 coils in threes **[calc]**; measured ~1.6 kΩ at a 73's
  jack with pots **[web, snippet: MEF]**, 2.1 kΩ and 0.854 H on an 88 **[web, MORRIN]**. Coil
  self-capacitance not published; with cable it forms the resonance below.
- **88-key top section:** the notes above 73 are a separate section (groups 3, 2, 2, 2, 1, 1, 1, 1) in
  parallel with the main harp through a series **4.7 nF** capacitor that "removes thumps"; 47 nF there
  sounds "dull, muffled" **[web, FRS-PU; snippet: EPF-9770]**. Into 10 kΩ the 4.7 nF is a high-pass
  at 3.4 kHz **[calc]**: the top octave's fundamental is attenuated and its hammer "thump" (§7)
  removed. The 88 has slightly lower output **[web, FRS-PU]**.
- **One coil, one key:** the network is linear, so the model sums each key's EMF with weight 1/n_g
  (its group size) and one output impedance; the shared network is where all notes meet.
- **Crosstalk:** a dead pickup can still sound "low or quiet" because "the adjacent pickups are
  producing a faint signal" **[web, FRS-PU]**; Shear, driving one tine electromagnetically, saw a
  signal "approximately equal to that of only the pickup nearest" **[web, Sh11]**. With 13.7 mm key
  pitch against a 0.5–3 mm own gap, the neighbour's coupling is ~−40 to −60 dB **[calc: dipole
  field ∝ r⁻³; to be checked]**. Model: each tine also feeds its two neighbours' coils at −50 dB
  (with the bell function evaluated at the neighbour's distance: nearly linear, so it carries the
  fundamental, not the bark) **[design]**.
- **Hum:** alternating groups cancel uniform fields, but with 24 unequal groups "not theoretically
  perfect"; hum at 50/60 Hz and harmonics **[web, snippet: EPF-6237]**; Dyno sold a shielding kit
  against 60 Hz hum from keyboards stacked on the lid **[web, FR-DY]**. Larger groups (6 or 9) are
  reported quieter **[web, snippet: EPF-6237]**. Model: hum = Σ_g ±(1+ε_g)·B_mains(t) with ε_g ~ 5 %,
  B_mains rich in odd harmonics (transformer leakage); level −60 to −45 dB re a mf note, higher in a
  Suitcase (its own transformer below the harp) **[design]**.

### 6.3 Stage piano output

- Harp → RCA → nameboard → 1/4 in jack. 1979–84 Stages (and from late 1973): **Volume 10 kΩ audio,
  Bass Boost 50 kΩ reverse-audio with 0.047 µF** **[web, snippet: EPF-9793, JVP]**. Topology: the
  capacitor in series with the 10 k load and the pot across the capacitor, so the "boost" is a bass
  *cut* that runs from cut to flat (a shelf of ~−6 dB part-way) **[web, snippet: EPF-9793]**. Before
  late 1973: a 10k/10k pair with 1 µF (or 0.1 µF) as a treble roll-off **[web, snippet: VV-PASSIVE,
  MORRIN]**.
- **The harp's own low-pass:** the harp is a series R–L source; into the 10 kΩ pot the corner is
  (R_s + R_L)/(2πL) ≈ **2.2 kHz** (88, 0.854 H) to 3 kHz (a 73 at ~0.6 H), first order **[calc;
  web, MORRIN gives ~1.8 kHz]**. Since the EMF ∝ dΦ/dt rises 6 dB/oct for a given tine
  displacement, above the corner the output follows the *displacement* (flat), below it the
  velocity: the harp integrates the pickup's differentiation above ~2 kHz **[calc]**. This shapes the
  bark's upper harmonics and the treble's level; it is a central part of the "tine piano sound" and absent
  from models that take V = dΦ/dt straight to the output.
- With the 47 nF bass cut engaged: series resonance with L at f = 1/(2π√(LC)) ≈ 790 Hz, Q ≈ 0.36 with
  ~12 kΩ: a broad mid band-pass **[calc; MORRIN qualitatively]**; a wrong 50 k volume pot moves the
  high-pass from 340 to 68 Hz **[web, MORRIN]**.
- **Cable and amp:** the harp L with cable + coil capacitance C resonates at 1/(2π√(LC)):
  300 pF → 9.9 kHz, 500 pF → 7.7 kHz, 1 nF → 5.4 kHz (L = 0.854 H) **[calc]**. Loaded by the 10 kΩ pot
  it is overdamped (Q ≈ 0.2–0.35, no peak); unloaded (pot removed, harp into a 1 MΩ amp or DI) it
  would peak with Q ~10–20 (limited by coil R and eddy losses) in the 5–10 kHz range **[calc]**: a
  "modded" bright tine piano. Cable ~100 pF/m **[memory]**.
- **clean 2x12 combo (AB763)**: Input 1 1 MΩ, Input 2 136 kΩ (−6 dB); tone stack slope 100 k,
  treble 250 pF, bass 0.1 µF, mid 0.047 µF **[web, ROB]**: the usual American amp mid scoop; guitar-amp
  presence around 3–4 kHz "can sound too piercing" **[web, FR-FX]**. The Stage's 10 k pot dominates
  the load, so the amp input impedance hardly matters **[web, snippet: EPF-6237; MORRIN]**.

### 6.4 Suitcase systems

Three generations; the power amp and supply sit in the speaker cabinet, the preamp in the piano
**[web, RAMP]**:

| | Jordan 50 W (1966–69) | Peterson 80 W (1969–77) | Haigler 100 W (1977–, also Janus I) |
|---|---|---|---|
| Preamp | discrete, 13.5 V zener supply | discrete 2N3392, +25 V | MC1458 op-amps, ±15 V |
| Input load on harp | – | ~25–30 kΩ [calc] | 10 kΩ (R1) |
| Tone | bass 100 k, 0.1/0.1 µF; treble 100 k, 3 nF | bass 100 k lin, 0.1/0.1 µF; treble 100 k lin, 4.7 nF; 22 k, 6.8 k ×2, 47 nF; in a 2-transistor feedback stage (390 k/1 M) | Baxandall: bass 50 k lin across 47 nF, treble 50 k lin with 10 nF ×2, 4.7 k, 39 k, 5.6 k |
| Vibrato | mono: RC phase-shift oscillator (2N3397; 0.15/0.15/0.22 µF, 820 k) driving a JFET shunt via 33 k | stereo: 2-transistor astable → 2N3053 drivers → #19 lamps → ORP61 CdS cells, antiphase | stereo: MC1458 Schmitt oscillator → clipped square → LED/LDR optocouplers, antiphase |
| Rate | – | 2.0–6.6 Hz [calc] | ~0.3–30 Hz [calc] |
| Power | 32 V, quasi-complementary, 4 Ω min | transformer phase splitter, two output transistors, 0.5 Ω emitters, ±35 V unregulated (3000 µF) | 2 × 50 W, 2N6254 quasi-complementary, ±30 V (11 000 µF), gain ≈ 31 |
| Speakers | – | 4 × 12 in Eminence, 32 Ω each, a parallel pair per channel = 16 Ω | not found |

**[web, RSM figs 11-1, 11-3, 11-8 … 11-17; RAMP; SG]**. Details for the model:

- **Peterson preamp input** (fig. 11-8): 0.22 µF coupling, common-emitter stage with 390 k/33 k bias,
  12 k collector, unbypassed 1.5 k emitter (gain ≈ 8), 330 pF collector–base, emitter follower into
  4.7 k **[web, RSM fig. 11-8]**. Input high-pass ≈ 24 Hz **[calc]**. With ~27 kΩ load the harp corner
  moves to ~5.3 kHz **[calc]**: the Peterson Suitcase hears a brighter harp than a Stage into its
  10 k pot or a Haigler's 10 k input (~2.2 kHz). The 330 pF Miller capacitance (×~9) is ~3 nF at
  the base, resonating with the harp near 3 kHz **[calc, unverified]**. A SPICE run of the Peterson:
  "pretty flat, with a treble rolloff" at centre settings **[web, snippet: EPF-8834]**.
- **Haigler preamp input** (fig. 11-1): 0.1 µF series, 10 k into an inverting stage, R_f 220 k ∥
  120 pF: gain −22 (26.8 dB), low-pass 6.0 kHz; with the harp: high-pass ~140 Hz, low-pass ~2.1 kHz
  **[calc]**.
- **Peterson vibrato** (fig. 11-8, read here): a symmetric astable with 2.2 k collector loads,
  5 µF NP cross-coupling and 22 k + 22 k base resistors returning through the Speed pot (R1B, 50 k
  reverse log): half-period 0.693·(22 k + R_speed)·5 µF → **2.0–6.6 Hz** **[calc]**; 2N3053 drivers,
  each with a 150 Ω 2 W resistor and a 50 µF capacitor, light one #19 lamp per channel; each lamp
  faces an ORP61 CdS cell that shunts its channel's signal (47 k / 10 k network); Intensity is R1A
  (50 k reverse log) in the lamp drive **[web, RSM fig. 11-8]**. The drive is square; the lamp's
  thermal lag and the CdS cell's slow, asymmetric response round it: the Peterson pan is described as
  "silky", smoother than the Haigler/Janus **[web, CEP-M; snippet: EPF-8834]**. Model: square LFO →
  lamp filament first-order thermal model (τ_heat ≈ 20–50 ms, τ_cool ≈ 50–100 ms, light ∝ T⁴-like
  steep curve) → CdS conductance with fast attack (~10 ms) and slow release (~100–300 ms), history-
  dependent → shunt attenuator per channel **[memory for the time constants; design]**. Depth at full
  intensity: not published; from the network a deep cut (> 20 dB) is possible **[calc, rough]**.
- **Haigler vibrato** (fig. 11-1): A2A relaxation oscillator (hysteresis 1 M / 470 k, β ≈ 0.68; timing
  1 k + 100 k reverse-audio Speed into 10 µF): T ≈ 3.3RC → ~0.3–30 Hz **[calc]**; A2B amplifies ×11
  and clips → near-square; LEDs in antiphase drive LDRs in resistive networks (2.2 k, 3.3 k, 10 k;
  Intensity 25 k reverse-audio) feeding A3A/B (gain −2.2) **[web, RSM fig. 11-1]**. LEDs switch
  faster than lamps: "on/off sharpness", "more synthetic" **[web, snippet: EPF-8834, EPF-1425]**.
- **Jordan tremolo:** a sine-like phase-shift oscillator into a JFET shunt (mono) **[web, RSM fig.
  11-17]**; the effects history calls the first effect a square-pattern tremolo **[web, FR-FX]**:
  the schematic wins for the circuit; perceived shape depends on the JFET's nonlinearity.
- **Power amps:** the Peterson's unregulated ±35 V on 3000 µF sags under bass-heavy chords (→
  compression, then clipping); ~34 W sine per channel into 16 Ω **[calc]**; germanium outputs
  **[web, snippet: SG]**. The Haigler 100 W: ±30 V, ~45 W into 8 Ω **[calc]**; feedback 100 k ∥ 10 pF
  over 3.3 k + 1 µF (LF corner ~48 Hz) **[web, RSM fig. 11-3; calc]**. Solid-state clipping: hard,
  symmetric, with rail sag; model with the project's solid-state clip plus a supply-sag state
  **[design]**.
- **Speakers/cabinet:** 4 × 12 in Eminence, 32 Ω each, wired as two stereo pairs **[web, SG;
  snippet: RVB listing]**; cabinet construction (sealed or open) and response not found. Alnico
  speakers in the Peterson era, "warm" **[web, CEP-M]**. Model: a 12 in driver in a closed box,
  −3 dB ~80–100 Hz, cone breakup peaks 1–3 kHz, roll-off above ~4–5 kHz, two speakers per side
  **[memory; to measure]**. The room hears both sides from one box: the pan is spatial only within
  ~1 m of the cabinet; at a distance it becomes partly a tremolo (the two sides sum) **[calc]**.
- **Headphones** fed through 330 Ω from the power amp **[web, RSM fig. 11-10]**.

### 6.5 Later electronics

- Mark V: passive Stage (plus the Stage 73 + MIDI with 3-band EQ); a planned 200 W Suitcase with
  chorus never shipped **[web, FR-M5]**. Mark 7 (2007–08): passive or active (preamp, EQ, auto-pan)
  **[web, snippet]**. Vintage Vibe pickups copy the 1974–84 spec with anti-microphonic bobbins
  **[web, VV-HIST]**. Modern "Dyno-like" preamps: high input impedance (330 kΩ in one) and a mid EQ
  ±7 dB at Q ≈ 0.7; a repairer's rule: "a pretty flat response, but with a dip around 300 Hz"
  **[web, snippet: EPF-8834]**.

### 6.6 Electronics: model chain

1. Per key: EMF_k(t) from the pickup function (both polarisations), scaled by the per-key coil
   sensitivity (magnet strength scatter ±1.5 dB **[design]**) and its group weight 1/n_g.
2. Harp: sum + crosstalk + hum, then the source impedance R_h + sL_h (R_h 1.4–2.1 kΩ, L_h 0.6–0.85 H)
   and coil/cable capacitance (0.3–1 nF) into the load: 10 k pot (Stage), 27 k (Peterson), 10 k
   (Haigler), 1 MΩ (modded/DI). A 2nd-order section per load is exact for this lumped circuit.
3. Stage: bass cut network (47 nF, 50 k) → cable → amp model (Twin: AB763 stack, tube clip, 2 × 12).
4. Suitcase: preamp input stage (soft clip only far beyond 0.2 V peak), tone stack (exact component
   values above, bilinear-transformed), vibrato (square LFO → lamp/LED → LDR dynamics → per-channel
   shunt gain), power amp with rail sag and clip, cabinet filter per side, stereo out.
5. The direct-out path (post step 2) for DI presets and validation.

Open electronics questions: single-coil L and self-capacitance; the magnet alloy and strength per
era; measured harp response; the Suitcase vibrato's actual rate range and depth; ORP61 and lamp time
constants; 100 W speaker impedance and cabinet type; output level of a coil in mV at given tine
amplitude (to calibrate N·Φ0/w).

## 7. Mechanical noises and contacts

What reaches the output is (a) electrical: anything that moves the tine or changes the magnetic
circuit near the pole, and (b) acoustic: in a Suitcase or with a mic, the action itself.

- **Hammer contact (electrical).** During the 6.4 ms contact the hammer pushes the tine up through the
  field: the pickup sees a low-frequency pulse ("thump") before the oscillation, polarity and size
  set by δ and the push; part of the "attack" of a direct-out tine piano **[calc from MP14 contact time;
  design]**. Generated by the model for free if the pickup reads the full tine displacement.
- **Damper (electrical).** On release the felt presses the tine off its rest position for ~10–50 ms:
  a small step through the pickup plus the fast decay; on Mk I/II the damper's snap-back touches the
  tine during the attack (§3.8). The bass dampers are the stiffest and longest **[web, RSM ch. 2]**.
- **Tine hitting things at ff.** Bass tines at 5–10 mm amplitude (§4.1) can reach (1) the pickup pole
  when the gap is small: the manual's gaps and the tine-settings page warn of this **[web, RSM ch. 4,
  RTS]**; a click/buzz with a sharp flux change; (2) the tonebar (the patent's rebate on bars
  I-1…I-12 exists to prevent it **[web, P656]**); (3) the damper felt. Model as one-sided stiff
  contacts at the tip (`ModalSet` obstacle) with per-key clearances **[design]**.
- **Hammer bounce / double strike** on worn or misregulated actions; a 1980 patent adds a brake
  against rebound **[web, P848]**.
  The manual (early design): bass escapement "could be 3/8 in (9.525 mm). Insufficient Escapement
  in this area invites 'double stroking' with an accompanying disturbing sound", because of "the
  wide arc of Tine movement" **[web, RSM ch. 10; RSM-AF p. 68]**: the long bass tine swings back into
  the hammer before it has fallen away.
- **Acoustic action noise.** Key bottoming on the felt, hammer return, damper return, pedal push rod
  (a wooden dowel with felt ends **[web, RSM ch. 2]**): audible in the room, absent from a DI. Players
  and sample-library users ask for separate keybed/release/pedal layers **[web, snippet: NUF]**; the
  Lounge Lizard model has a damper-noise layer with attack/release balance and a mallet-noise layer
  with pitch and decay **[web, AAS]**. Level: no measurement; −40 to −30 dB re a mf note at the
  player, mixed only for "Suitcase in the room" presets **[memory/design]**.
- **The direct tine sound.** An unamplified tine piano is audible at a quiet "tink" level from the tines
  and bars (a few dB SPL above the room at a metre) **[memory]**: irrelevant to the line signal,
  relevant to "in the room" presets.

## 8. Flaws, tolerances and ageing (per instrument)

From the service manual and repairers; each is a per-key random parameter in the model **[design]**,
with the source of the effect:

- **Uneven voicing** (δ and gap differ key to key): the manual sets both by ear key by key, with
  wide factory ranges (gap 0.5–3.2 mm; "slightly above dead center") **[web, RSM ch. 4]**. Typical
  scatter on a real harp: ±0.3 mm in each **[memory]** → ±3–6 dB level and ±10 dB in H2 between
  neighbours **[calc from §4.4]**.
- **Tip zone steps** at hammer 30/31, 40/41, 50/51, 64/65 (88 numbering) **[web, RSM ch. 4]**; worn
  tips get grooved and harden (neoprene ages), mixed tip types after repairs **[web, RHT]**.
- **Mixed tines** (makers, strike points) and replacements cut by eye → per-key strike-point and
  taper error; notes that "don't sustain or bark" **[web, RL-T]**.
- **Tuning drift:** springs slip; tine fatigue under heavy blows lowers the pitch; the "heavy
  traffic" middle fatigues and breaks most **[web, RSM ch. 5]**. Breakage concentrates where the
  bending stress at the clamp is highest for a given touch: the middle register gets most blows
  **[web, RSM ch. 1, ch. 5]**. Model: per-key detune ±2–5 cents, a few keys ±10–20 cents on an
  "old" instrument **[design]**.
- **Damper misadjustment:** felts not bearing firmly → notes ring after release; felts touching →
  short sustain; damper module side shift on early instruments (the reason for modules of 12)
  **[web, RSM ch. 2, ch. 4]**.
- **Escapement** too large: weak, uneven touch; too small: blocking/double strikes **[web, RSM ch.
  4]**.
- **Pickups:** open or weak coils (1979–80 pickups notorious) **[web, CEP-T]**; weak magnets lower
  the level of single notes; flux and resistance values in §6.
- **Tonebar mount:** grommets harden and springs settle, changing the leak and hence the sustain per
  key (§3.5); a loose tone-generator screw kills sustain **[calc from P418's single-screw rationale]**.
- **Key-to-key action variation:** felt compression, warped wooden keys (the reason for plastic keys
  in 1980) **[web, FR-M2]**.

## 9. Recording chains and effects

- **Stage → guitar amp**: the recommended amp was the clean 2x12 combo; tube overdrive "became a
  normal part of the tone", with tremolo and spring reverb; guitar-amp EQ peaks around 3–4 kHz "can
  sound too piercing on a tine piano"; one maker JC-120 (stereo chorus, 2×12 in) popular **[web, FR-FX]**.
  Herbie Hancock's *Head Hunters* is the reference for overdriven tine piano **[web, FR-FX]**.
- **Wah** (Hancock, Corea, Zawinul; often parked as a static band-pass, or with fuzz in the early
  1970s); **phaser** (late 1970s–80s ballads: Small Stone, one pedal maker Phase 90; Billy Joel "Just the Way You
  Are", Paul Simon "Still Crazy After All These Years"); **one pedal maker CE-1** chorus (stereo, the JC-120
  circuit); **Univibe**; **ring modulator** (Jan Hammer) **[web, FR-FX]**. Richard Tee: Small Stone
  **[web, snippet: RVB]**.
- **Dyno-My-Piano** (1974–mid 1980s): pickups as close as possible, tines set for overtones, an EQ
  preamp in the Stage namerail (bass boost/"overtone"), a tri-stereo tremolo (left–centre–right;
  square, sawtooth, sine), stereo chorus, shielding against 60 Hz hum **[web, FR-DY]**: the 1980s
  bright tine piano. Model it as a voicing preset + preamp EQ + stereo chorus.
- **Mark V Stage 73**: 3-band EQ and MIDI out **[web, FR-M5]**.
- **Direct inject**: the passive harp into a DI or console (high-impedance input), common in studios
  **[memory]**: the cleanest reference for validation.

## 10. Existing models: what they got right and wrong

| Model | Approach | Right | Wrong / missing |
|---|---|---|---|
| Pfeifle 2017 (FPGA) [Pf17] | FD beam, Hunt–Crossley hammer, point-charge field integrated over an idealised pole | pickup as the primary timbre source; velocity series | Kirchhoff-type nonlinearity in a cantilever; no tonebar; one polarisation |
| Muenster & Pfeifle 2014 [MP14] | measurement; a sine through a filter-like pickup function | tine-bar slaving; 6.4 ms contact; growl from the field | no transient in the model |
| Falaize & Hélie 2015/17 [FH15, FH17] | port-Hamiltonian (passive): hammer, beam, magnetic pickup, RLC load | energy-consistent pickup with gap and offset; audible voicing examples | no tonebar; pickup field model 1-D (2-D in JSV) |
| Gabrielli et al. 2020 [Ga20] | laser vibrometry; modal model + pickup intermodulation; keyboard-wide mode map | inharmonic attack explained by tine/bar modes + pickup intermodulation | full text not obtained here |
| Sønderbo 2024 [So24] | FDTD tine (uniform 1 mm), Hunt–Crossley, filter + tanh pickup | strike point and stiffness effects | wrong diameter (all notes flat, overtones at 6.37/35.3); pickup non-physical, harsh; overtones too long |
| jstolberg/tinepiano (GitHub) [JS] | modes + force pulse + magnetic pickup, parameters fitted to Pianobook samples (p, mp, mf, f) in 4 stages | per-key fitted pickup (p_d, p_o, κ) and modes from recordings | no published values; force-pulse hammer |
| AAS Lounge Lizard EP-4 [AAS] | commercial physical model: mallet (stiffness, force, noise), fork (tine level/colour/decay, tone bar level/decay), pickup (symmetry, distance, R/W type, input drive), damper noise, tremolo (triangle or soft square, stereo 180°) | exposes exactly the voicing controls | treats tine and tone bar as two additive layers; key/vel mapping by knobs |
| the piano-modelling forum a commercial piano model Vintage Tines MKI/MKII (2016) [MOD] | commercial physical model | "Bark" from pickup distance + symmetry; tine noise; "Blooming" | internals unpublished |
| Apple EVP88 / Vintage Electric Piano [APL] | modelled/sample hybrid | era list, "bell vs bark by tine–pickup proximity" | – |
| a sample-library maker Keyscape, Wonder tine piano (Pianobook) etc. | samples (4 velocity layers for Wonder tine piano) | real instruments; reference recordings | fixed voicing; layer crossfades |

**[web, sources as tagged]**. Common failure modes: treating the tonebar as a second additive
"layer" with its own pitch (it is enslaved); pickup as a static tanh on a filtered signal (wrong
symmetry and no H1 fold-back); overtones that sustain too long; the same timbre curve on all keys
(the bark belongs to the lower half at the same touch).

## 11. Parameter table (per key, for the model)

All per-key arrays over the 88 keys (73-key presets use 8–80). **Bold** = from a primary table here;
the rest are fits or design values.

| Parameter | Value / law | Source |
|---|---|---|
| f0 | equal temperament, A4 = 440, factory stretch −20 c (A0) … +30 c (C8) (**§3.4 table**) | RSM-AF fig. 5-4 |
| Tine length L | **§3.2 table** (kit sheet; fig. 6-2 for F7–C8); 88 keys 1–7 = 156.4 mm | RSM-AF p. 97, RSM fig. 6-2 |
| Tine profile | swaged: 2.54 mm × 12.7 mm root, then 1.524 mm (1975+); tapered 2.54→1.78 mm over 25.4 mm then 1.524 (1970–75); uniform 1.905 (1960s) | RSM-AF figs 7-4…7-6 |
| Spring mass | **§3.3 table**: ~0.2–0.4 g (yellow keys 1–33, blue 34–73); 88 bass up to ~1.3 g | RSM-AF p. 97, calc |
| Mode ratios f2/f1, f3/f1 | swaged FE: ~9.2/28 (A0) … 6.7/19.5 (E1) … 7.0/19 (C4) … 7.9/19 (E5) … 5.8/19 (E7) | calc, Ga20 check |
| Modes kept | 3 (≤B5), 2 (C6–G#7), 1 (≥A7) below 20 kHz | calc |
| Fundamental Q | 950 (E♭2) … 730 (E♭3) … 1100–1500 (C4–E♭4) … 2200 (E♭5) … 1760 (E♭6), interpolate in log f | Sh11 |
| Mode 2/3 T60 | 30–150 ms / 10–50 ms | design, to fit |
| Polarisation split | 0.02–0.3 Hz random; horizontal level −15 to −25 dB | design |
| Tonebar mode | f_bar per MP14 table (51…222 Hz) or 2.5 tones off in bass/mid, Q 20–100, mass 10–25× tine | P656, P922, MP14 |
| Strike point x = s/L | **§3.6 table** | RSM ch. 4, calc |
| Hammer mass | 5–11 g effective (to fit to 6.4 ms contact) | FH15, So24, MP14 |
| Tip stiffness | Shore 30/50/70/90/wood zones (**§3.7**), E 1.1–21 MPa, wood ≫ | RSM, RHT |
| Hammer speed | 0.5–4 m/s from velocity 1–127 | So24 |
| Gap g | 1.6 mm bass, 0.8 mm mid, 1.6 mm wood-tip zone (post-1972); early 1.6–3.2 mm | RSM, RTS |
| Bell width w | g + ~1 mm | calc |
| Voicing δ/w | ~0.7–1 lowest 15 keys and wood zone ("fundamental"); 0.3–0.6 elsewhere; preset knob | RTS, design |
| Bell exponent p | 1–1.5 | electric_pianos.md |
| Amplitude scale | A = 3.2 mm at C3 for "normal" touch, A ∝ 1/f | P321 |
| Damper | bass/mid/treble stiffness classes, release T60 50–200 ms, Mk I/II snap-back flag | RSM ch. 2, FR-M5 |
| Contacts | tip vs pole (gap), vs bar rebate, vs damper felt | design |
| Scatter (old instrument) | δ, g ±0.3 mm; detune ±3 c; Q ±20 %; strike ±2 mm | design |
| Electronics | §6 | §6 |

## 12. Model plan update (against `electric_pianos.md` §7)

1. **Per-key tables** in a generated header (like `levels_data.h`): L, μ, f2/f1, f3/f1, mode shapes at
   the strike point, Q, strike x, tip zone, gap, δ. Generated by a small tool from §3.2/§3.3 so the
   chart stays the source of truth.
2. **Resonator:** tine = 2–3 modes × 2 polarisations (`ModalSet`-style exact steps), plus *one* bar
   mode per key at the bar's own frequency, coupled through the base reaction (detuned, lossy). Drop
   the equal-f0 fork and the bar-as-second-layer idea.
3. **Hammer:** Hunt–Crossley with mass, stiffness per zone and the tine's compliance; check the
   contact time against 6.4 ms in the middle and that mode 2 is weak there.
4. **Pickup:** the bell function on the *full* tip displacement (so the hammer push and damper
   steps come out), both polarisations, analytic derivative; per-key g and δ; 2× oversampling below
   ~C4 where A/w is large and harmonics up to H8 matter, direct elsewhere (the band-limit is set by
   n·f0 with n up to ~8 at A/w = 2) **[calc]**. Optional small magnetic stiffness/loss per key.
5. **Electronics** per §6: harp network (per-key coil L, R; group wiring; hum pickup; crosstalk),
   then either Stage (passive volume/bass boost → cable → amp model) or Suitcase (preamp, EQ, lamp/
   LDR pan, power amp, 4×12 cabinet), then the effects of §9 as optional stages.
6. **Era/setup presets:** Raymac/Torrington/Schaller taper (mode ratios ±5 %), felt/cube/graduated
   tips, damper snap-back, voicing (bark vs bell), "old instrument" scatter.
7. **Cost:** unchanged from `electric_pianos.md` §7 (~1 % of a core for 20 notes) plus the
   electronics chain, which is per instrument, not per note.

### 12.1 What the model must do that samples cannot

A user comparison of real and sampled tine piano lists four things samples miss. Each becomes a
requirement with the numbers found above:

1. **Continuous velocity, no layers.** Hammer speed 0.5–4 m/s maps linearly to tine amplitude
   (§4.6). Tip amplitude at "normal" touch is 3.2 mm at C3 and A ∝ 1/f (§4.1). Against w = g + ~1 mm
   (g 0.5–3.2 mm, §3.9), the bass sweeps A/w from ~0.2 (pp) to ~2–3 (ff). Over that sweep the pickup
   function morphs the spectrum continuously: H2−H1 goes from −34 dB through 0 to +10 dB, and H1 folds
   back above A/w ≈ 0.8 (§4.4). The treble stays at A/w ≲ 0.5 and only gets louder. The bloom (H1
   rising during the decay) and n×-faster harmonic decay follow from the same function and cannot be
   had from a fixed sample with an envelope. Requirement: the pickup is evaluated on the simulated
   displacement every sample. No crossfades, no per-velocity filters.
2. **Mechanical coupling.**
   - *Through the rail and mount.* Every tonebar sits on a soft grommet and two springs on one rail
     (§3.5). classic ladder says the mounting post is "not rigid" **[web, EPF2896]**, and the 1983 patent
     changed the mount because it leaked treble energy **[web, P418]**. No measured rail-coupling
     coefficient or sympathetic level was found. Requirement: a shared rail mode (or a weak all-to-all
     coupling through one rail velocity) that every tonebar's base reaction drives and feels. Start
     at −50 dB transfer between keys at unison and fit by ear and to a pedal-down recording **[design]**.
   - *Sympathetic ringing with the pedal down.* Undamped tines at or near harmonics of struck notes
     pick up energy. Since a tine has Q ~1000, only partners within ~f0/Q (a fraction of a Hz to a
     few Hz) respond strongly. In practice that means octave and unison partners, and the tine's own
     f0 against a struck note's pickup harmonic does *not* couple, because the harmonics are
     electrical, not mechanical **[calc]**.
   - *Thump into the pickups.* There are two paths. The hammer pushes the tine through the field
     during the 6.4 ms contact (§7). The keybed and harp shake the pickups relative to the tines, and
     a loose magnet is microphonic **[web, FRS-PU]**. The factory's 4.7 nF series capacitor on the 88's
     top section exists to "remove thumps" (a high-pass at ~3.4 kHz into 10 kΩ) **[web, FRS-PU; calc]**:
     evidence that the low-frequency thump is in the signal and large relative to short tines. No
     spectrum or level was found. Model: the push and damper steps come out of the pickup function on
     the full displacement, plus an optional rail-acceleration term per pickup **[design]**.
3. **Re-striking a vibrating tine.** The contact is a Hunt–Crossley spring-damper between the hammer
   and the tine's *current* position and velocity at the strike point, so the new blow adds
   vectorially to the old motion:
   - At mf C3 the tine's strike-point velocity is about 0.35 × 2.6 ≈ 0.9 m/s peak (mode shape at
     x ≈ 0.5) **[calc]**. That is comparable to the hammer's 0.5–4 m/s.
   - Meeting the tine as it comes down (moving toward the hammer), the relative speed is higher: a
     harder, shorter contact and a sharper transient. The new motion is roughly in phase with the old,
     so the amplitudes add.
   - Meeting it as it moves away, the contact is softer and longer and the blow partly cancels the
     old motion: a choked thud, with amplitude down to |A_new − A_old|.
   - The effect is strongest where the 6.4 ms contact is shorter than the period: the bass (E1 period
     24 ms). It averages out above ~C4, where contact ≳ period.
   - Dampers between strikes: the damper is pulled off by the bridle strap as the hammer rises (§3.8).
     On a fast repeat the key may not return far enough for the felt to touch the tine, but a partial
     return lets it brush the tine. Mk I/II dampers also "snap back" (§3.8). No timing data was found;
     model the damper as engaged by key position with a few-ms ramp and a light-contact (partial)
     state **[design]**.
   - The bass needs ≥ 3/8 in escapement or the tine's own return swing double-strikes the hammer
     (§7): the model gets this for free if hammer and tine share the contact.
4. **Per-key voicing irregularity.** Gap and offset are set by hand and by ear on every key (§3.9).
   - With a ±0.3 mm scatter in each **[memory]** on w ≈ 2 mm: level ±1.5–3 dB (the flux gradient
     goes roughly as w⁻²), H2 relative to H1 ±5–10 dB at forte (§4.4), and the velocity at which the
     bark begins shifted by ±15–20 % **[calc]**.
   - Add the hammer-tip zone steps (§3.7), mixed tine makers with wrong strike points (§3.6), spring
     drift of ±2–5 cents, and a few keys at ±10–20 cents on an old instrument (§8).
   - The Suitcase adds its own unruliness:
     - the vibrato's lamp and CdS lag make the pan depth depend on rate and history (§6.4);
     - the square-wave lamp current (2N3053 into #19 lamps, 150 Ω / 50 µF per side) can couple a tick
       at the LFO rate into the audio **[memory: common in lamp tremolos]**;
     - the unregulated ±35 V supply adds ripple hum at 2× mains and sags under bass chords;
     - harp hum comes from imperfect group cancellation (§6.2).

   Requirement: every per-key parameter in §11 carries a seeded random offset, and the instrument
   preset chooses the spread ("factory fresh" ±0.1 mm, "road-worn" ±0.5 mm).

## 13. Validation targets

1. Tuning from the chart: every key within ±50 cents before spring trim using L and μ (sanity of
   the tables); overtone ratios at F1 7.11/20.25 **[Ga20]**.
2. Fundamental T60 per register against §4.2 (±20 %).
3. Waveform sinusoidal within 10–14 ms of the strike (bar/tine signals) **[MP14]**; contact 6.4 ms.
4. Pickup: H1 fold-back and H2/H3 rise with velocity in the bass (§4.4); the harmonics' decay n×
   faster than H1 at soft levels; Shear's G3 voicing series **[Sh11 figs 2.4–2.5]**; Falaize–Hélie
   gap/offset audio examples **[FH17 page]**.
5. Same touch across the keyboard: bark in the lower half, clean bell above ~C5 (§4.1).
6. Velocity series against recorded samples (Pianobook "Matt's tine electric piano" p/mp/mf/f used by
   [JS]; Wonder tine piano 1976 Mk I, 4 layers): spectra of H1–H8 vs time, attack partials.
7. Electronics: Suitcase pan rate and depth range, preamp EQ curves (§6) against schematic
   simulation and recordings; Stage into a 1 MΩ/cable load: resonance frequency and Q.
8. Listening: DI, Suitcase in the room, clean 2x12 combo, with and without phaser/chorus.

## 14. Open questions

- Gabrielli et al.'s keyboard-wide mode map (tine + bar modes, levels and decays): the main missing
  table; JASA paywalled, not obtained (the EP-Forum thread on the classic ladder's article has no measured modes,
  so it does not close this).
- Is the tonebar near-tuned (Moog 1980: slow energy exchange) or far-detuned (MP14: slaved)? Probably
  both, per key and instrument; a recording of a held low note on a DI showing (or not) a slow dip
  would decide it per instrument.
- Whether MP14's "bar f0" (51–222 Hz) is the bar's bending mode or its mount mode; the bar's
  bending frequencies per key (P656 lengths and the 1/8 × 1/2 in section give a check).
- The polarisation split and its audibility; magnetic drag size.
- Hammer effective mass and key-to-hammer ratio; velocity ↔ hammer speed curve (the 1979 manual,
  checked in full in the Vintage Vibe scan, gives key dip, escapement and tip heights but no lever
  ratios or masses: still open).
- Damped-note release T60 per register; damper snap-back timing on Mk I/II.
- Tine taper profiles per maker (Raymac, Torrington, Singer, Schaller): the factory profiles of the
  three generations are now known (§3.3, closed for the generic case); per-maker differences remain.
- Closed by the scanned manual [RSM-AF]: per-key tine lengths in inches and spring type per key
  (§3.2), the swaged tine profile (§3.3), the full stretch chart (§3.4), the timbre/volume diagrams
  (§3.9). Not in it: coil inductance, magnet data, vibrato rates, speaker data (§6 open list stands).
- Electronics questions listed at the end of §6.

## 15. Sources

Primary documents and papers:

- [RSM-AF] The same 1979 service manual (CBS P/N 34.0119.000) as a 97-page scan with Vintage Vibe's
  stamp, including all figures and the loose kit sheet "Cutting instructions for tone generators"
  (P/N 024297) as the last page: https://medias.audiofanzine.com/files/rhodes-keyboard-instruments-service-manual-470656.pdf
  (read as images: figs 4-8, 4-9, 5-4, 7-4…7-6, 10-3…10-5, p. 97). Same edition as [RSM]; it adds
  the figures' contents, which the web transcription lacks.

- [RSM] Rhodes Keyboard Instruments Service Manual (CBS, 1979), online:
  https://fenderrhodes.com/org/manual/ (toc), ch. 1 https://fenderrhodes.com/org/manual/ch1.html,
  ch. 2, ch. 4 https://fenderrhodes.com/org/manual/ch4.html, ch. 5, ch. 6 (fig. 6-2 tine cutting chart
  https://fenderrhodes.com/org/manual/fig6-2.gif, digitised here), ch. 7, ch. 10, ch. 11 figures
  fig11-1/-3/-8/-9/-10/-17.jpg; OCR'd PDF https://www.fenderrhodes.com/org/manual2.pdf (all read).
- [P922] H. B. Rhodes, "Electrical musical instrument in the nature of a piano", US 2,972,922 (1961)
  https://patents.google.com/patent/US2972922A/en (read: fork, mass ratios, dwell targets).
- [P656] H. B. Rhodes, "Tone generator with vibratory bars", US 3,644,656 (1972)
  https://patents.google.com/patent/US3644656A/en (read: twisted bar, detuning, groups).
- [P321] S. E. Lover (CBS), "Electromagnetic pickup and method for tine-type electric piano",
  US 4,040,321 (1977) https://patents.google.com/patent/US4040321A/en (read: pole geometry, tine
  free-end Ø, tip amplitudes).
- [P418] H. B. Rhodes, S. J. Woodyard, "Tuning fork mounting assembly in electromechanical pianos",
  US 4,373,418 (1983) https://patents.google.com/patent/US4373418 (via search summary).
- [P848] H. B. Rhodes, "Piano action", US 4,338,848 (1982)
  https://patentimages.storage.googleapis.com/af/02/4a/6db9239e5a0d15/US4338848.pdf (via search summary).
- [MP14] M. Muenster, F. Pfeifle, "Non-linear behaviour in sound production of the Rhodes piano",
  ISMA 2014, Le Mans, 247–252. http://www.conforg.fr/isma2014/cdrom/data/articles/000062.pdf (read).
- [PM17] F. Pfeifle, M. Münster, "Tone production of the Wurlitzer and Rhodes e-pianos", DAGA 2017.
  https://pub.dega-akustik.de/DAGA_2017/data/articles/000210.pdf (read).
- [Pf17] F. Pfeifle, DAFx-17 https://dafx.de/paper-archive/2017/papers/DAFx17_paper_79.pdf (read).
- [Sh11] G. Shear, MS thesis, UCSB (2011)
  https://www.mat.ucsb.edu/Masters/GregShearMasters2011_12_5.pdf (read); Shear & Wright, NIME 2011
  https://www.nime.org/proceedings/2011/nime2011_014.pdf.
- [Ga20] L. Gabrielli et al., JASA 148(5), 3052–3064 (2020), doi:10.1121/10.0002002 (abstract only).
- [FH15], [FH17] Falaize & Hélie, see `electric_pianos.md` §9.
- [So24] T. Sønderbo, MSc thesis, Aalborg (2024)
  https://projekter.aau.dk/projekter/files/719175589/Master_Thesis_Real_time_physical_model_of_the_Rhodes_Electric_Piano_Tobias_Sonderbo.pdf (read).
- [Woo13] B. Wooley, "Comparative analysis of acoustic/electro-mechanical … instruments", UIUC
  Physics 406 report (2013)
  https://courses.physics.illinois.edu/phys406/sp2017/Student_Projects/Spring13/Ben_Wooley_P406_Final_Project_Report1_Sp13.pdf (read; qualitative).

Makers, repairers, histories:

- [RHT] https://www.fenderrhodes.com/service/hammer-tips.html (read).
- [RTS] https://www.fenderrhodes.com/service/tine-settings.html (read; figure values not in text).
- [FRS-PU] S. Woodyard, Rhodes pickups https://fenderrhodes.com/service/pickups.html (read by the
  electronics sub-search).
- [RAMP] Suitcase amp differences https://www.fenderrhodes.com/service/amp-diffs.html (read).
- [FR-FX] Effects history https://www.fenderrhodes.com/history/effects.html (read).
- [FR-DY] Dyno-My-Piano history https://www.fenderrhodes.com/history/dyno.html (read).
- [FR-M2] https://www.fenderrhodes.com/models/mark2.php; [FR-M5] https://www.fenderrhodes.com/models/mark5.php (read).
- [VV-HIST] Vintage Vibe, history of Rhodes pickups https://www.vintagevibe.com/blogs/news/history-of-rhodes-pickups (read).
- [VV-PDF] Vintage Vibe, Rhodes pickup measurement/troubleshooting (2020)
  https://cdn.shopify.com/s/files/1/0666/2821/files/Rhodes_Pickup_Measurement_Troubleshooting_-_REV1_27FEB2020.pdf (read).
- [VV-PASSIVE] https://vintagevibe.com/products/fender-rhodes-stage-passive-electronics (snippet).
- [RL-T] Retrolinear, Mk I Singer tines https://retrolinear.com/mk-i-singer-tines.html (read).
- [CEP-T] Chicago Electric Piano, Rhodes timeline
  https://chicagoelectricpiano.com/rhodes/the-ultimate-fender-rhodes-timeline/ (read).
- [CEP-M] Chicago Electric Piano, Mark I vs Mark II
  https://chicagoelectricpiano.com/blogs/news/fender-rhodes-mark-i-vs-rhodes-mark-ii (read).
- [MORRIN] D. Morrin, Rhodes troubleshooting
  https://sites.google.com/site/davidmorrinoldsite/home/trouble/trouble-keyboards/rhodes (read).
- [SG] SoundGirls, "The Fender Rhodes" https://soundgirls.org/the-fender-rhodes/ (snippet/read).
- [ROB] R. Robinette, AB763 https://robrobinette.com/How_The_AB763_Deluxe_Reverb_Works.htm.
- [JVP] Jupiter Vintage Pianos, Stage pots
  https://www.jupitervintagepianos.com/shop/rhodes/rhodes-electronics/rhodes-stage-pots/ (snippet).
- [WP-R] https://en.wikipedia.org/wiki/Rhodes_piano.

Forums (weakest; second-hand summaries):

- [EPF08] https://ep-forum.com/smf/index.php?topic=4580.0 (partials, inharmonicity).
- [EPF-6237] …?topic=6237.0 (harp wiring, hum); [EPF-6277] …?topic=6277.0 (wire gauge);
  [EPF-9770] …?topic=9770.0 (88 top section); [EPF-9793] …?topic=9793.0 (Stage pots);
  [EPF-8834] …?topic=8834.0 (Peterson vs Haigler, SPICE); [EPF-1425] …?topic=1425.0;
  [EPF-516] …?topic=516.0 (Dyno). All at https://ep-forum.com/smf/index.php.
- [MEF] https://music-electronics-forum.com/forum/instrumentation/pickup-makers/31339-fender-rhodes-pickups-experiment.
- [EPF2896] EP Forum, "The Rhodes Piano by Bob Moog" (repost of B. Moog, *Contemporary Keyboard*,
  April 1980) https://ep-forum.com/smf/index.php?topic=2896.0 (read; one page, qualitative).
- [NUF] Nord user forum, Rhodes key clicks https://www.norduserforum.com/viewtopic.php?p=182454.
- [RVB] Reverb, "6 iconic keyboard and effect pedal combos"
  https://reverb.com/news/6-iconic-keyboard-and-effect-pedal-combos.

Models and products:

- [AAS] Lounge Lizard EP-4 manual https://applied-acoustics.com/lounge-lizard-ep-4/manual (read).
- [MOD] Modartt, electric pianos https://modartt.com/electric; press release
  https://modartt.com/data/download/press/press_release_2016_06_08.pdf (snippet).
- [APL] Apple Logic Pro, Rhodes models
  https://help.apple.com/logicpro-instruments/mac/10.1/en.lproj/lgsifc1ba3ec.html (read).
- [JS] J. Stolberg, https://github.com/jstolberg/rhodes (read README).

Not obtained: Gabrielli et al.'s full text and mode map; Wendland (2009); the Pfeifle–Münster
Springer chapter; the Rhodes tine-settings diagram values; a measured harp response or coil
inductance; Suitcase cabinet data.
