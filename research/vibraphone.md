# The vibraphone (Musser M55 Pro-Vibe) on the pajupilli modal engine — research notes

Status: 2026-11-15. Research only: nothing is built yet. It covers GM program 12 "Vibraphone", MIDI 53–89 (F3–F6).
SI units. c = 344 m/s (air at 20 °C). Aluminium 2024-T4: E = 73.1 GPa, ρ = 2780 kg/m³, ν = 0.33, so √(E/ρ) =
5128 m/s **[memory]** (typical handbook values).

Tags: **[web]** taken from a source in the reference list (the source is named in the line); **[calc]** computed
here from the numbers given (scripts are re-runnable from the formulas in the text); **[memory]** from the general
literature as I remember it, not checked for this file. Check these before relying on them. Engine terms
(`ModalSet::Def`, `strike`, `damp`, `quad`, `rad`, `side`) refer to `src/modal.h`.

---

## 0. Short answer

- **Bars:** 37 bars of 2024-T4 aluminium, 12.7 mm thick. Width is graduated from 57 mm (F3) to 38 mm (F6), and
  length runs from about 381 mm to 175 mm. The bars are silver anodised (M55), gold (M55G) or black (M55B).
- **Modes:** a deep arch tunes the first three vertical bending modes to **1 : 4 : 10** (the double octave, then
  three octaves and a major third) in the low register. That holds up to about D4. The third mode then drops to a
  minor third (≈ 1 : 4 : 9.6). Mode 2 is still tuned to 4 up to about G5. Above that, only the fundamental is
  tuned. A struck bar also rings a torsional mode, at about 7 f₁ low and about 3 f₁ high **[calc]**, and higher
  untuned bending modes.
- **Decay of the bar alone (damper off):** T60 ≈ 20–30 s at F3 falling to ≈ 5–6 s at F6 **[web]**. A tuned
  resonator with the fan open drains the fundamental: **20 s → 3–4 s** in the low register **[web]**. The damper
  felt (pedal up) stops a note in a few tenths of a second **[memory/calc]**.
- **Resonators:** closed aluminium tubes, a quarter wavelength long at the fundamental minus the end correction.
  They run from ≈ 47 cm long and 76 mm in diameter (F3) to ≈ 5 cm and 38 mm (F6) **[calc from web anchors]**. Their
  Q is ≈ 90 at F3 and ≈ 12 at F6 **[calc]** (a measured lab tube: ζ = 0.73 %, Q ≈ 68 **[web]**). Only odd
  harmonics fit a closed tube, so the resonator **reinforces only the fundamental**.
- **Coupling:** the low bars are designed near **critical coupling**: κ ≈ 3 rad/s against the tube's σ_r ≈ 6–8 /s
  at F3. About 80 % of the fundamental's energy then leaves through the tube **[calc]**. The resonance half-width
  is ≈ 10 cents at F3 and ≈ 70 cents at F6 **[calc]**. This explains why the sound and the sustain depend so much on
  temperature: the air in the tube shifts ≈ +3 cents/K, while the aluminium shifts −0.4 cents/K **[calc]**.
- **Fan:** one disc per tube, all on shafts turned by one motor. Each disc opens and closes its tube twice per
  revolution. Modern motors turn **30–145 rpm** (Yamaha), 25–150 or 40–140 rpm (Adams) and 20–160 rpm (Adams
  Nomad) **[web]**. That is a 0.5–2.4 Hz shaft, so the **tremolo runs at ≈ 1–5 Hz** **[calc]**. Wikipedia's
  "1–12 Hz" (citing Meyer 1973) is the outer bound. I found no published rpm for the M55's own motor, an Oriental
  Motor speed-control unit at 1/140 HP **[web]**. The effect is mostly **amplitude modulation of the fundamental**:
  85–90 % → 10–15 % of "resonance" from open to closed **[web, patent]**, or about −15 dB on the fundamental
  **[calc]**. It also adds about ±1 cent of FM **[calc]**. The upper partials are not modulated, because they do not
  go through the tube.
- **Mallets:** a rubber core wound with yarn or cord, 20–30 g at the head **[memory]**. Hertz contact with
  α ≈ 2.5 (wound, stiffening) and contact times of ≈ 3 ms (soft, pp) down to 0.7 ms (hard, ff) give **K ≈ 2·10⁹
  (soft) … 4·10¹¹ N/m^2.5 (hard)** for the engine's `strike` **[calc]**. Equivalent values with α = 1.5 are
  1·10⁶ … 4·10⁷ N/m^1.5. Rucz's xylophone mallet, as a sanity check: 35 g, K = 5·10⁷ N/m^1.5, 0.7 ms **[web]**.
- **Plan:** model each bar as one `Def` with about 6 bar modes plus the resonator as a 7th "mode" (an air
  oscillator). The resonator is tied to bending mode 1 by a **linear** coupling term. That term is the one engine
  addition needed: `quad` is quadratic only. The fan modulates the coupling and the tube's tuning per block. The
  pedal is `damp()` on every bar. The cost is about 600 mode-steps per sample even with all 37 bars ringing. One
  limit must be raised: `kMaxVib = 32 < 37`.

---

## 1. The instrument: Musser M55 Pro-Vibe

| Item | Value | Tag / source |
|---|---|---|
| Range | F3–F6, 3 octaves, 37 bars (MIDI 53–89) | [web] retailers, Conn-Selmer [CS], Wikipedia [WV] |
| Tuning | A = 442 Hz standard since 1 Jan 1994 (A = 440 before; 440/445 to order) | [web] Ludwig/Musser parts list [M55P], Ozwinds |
| Bar width | graduated 2¼" → 1½" (57 → 38 mm) | [web] French retailer "57–38 mm"; same as Yamaha YVT-2700 [YVT] |
| Bar thickness | ½" (12.7 mm); a 37-bar instrument at Grinnell measured 0.52" (13.2 mm) | [web] [YVT], [GRN] |
| Bar length | ≈ 15" → 6.9" (381 → 175 mm) on a 37-bar F3–F6 vibraphone (Grinnell; the maker is not confirmed as Musser) | [web] [GRN] |
| Alloy | 2024-T4 aluminium (a vibraphone patent; a Musser forum statement says the same). Some makers use 6061-T6 | [web] [P749], [VW]; 6061: [memory] |
| Finish | anodised: silver (M55), gold (M55G), black (M55B) | [web] [CS] |
| Suspension | "node wire, plain/sharp" through holes at the nodes; 40 saddle posts, 40 hook posts, 88 post insulators | [web] [M55P] |
| Node holes | 0.244 L from each end (undercut bar; a uniform bar has 0.224 L) | [web] [P749]; [WV] |
| Resonators | "resonator and pulsator assembly, plain / sharp": two banks with two pulsator shafts and one motor belt (5¾" loop) | [web] [M55P] |
| Motor | variable speed; Bodine with a rheostat until ≈ 1980, then an Oriental Motor speed-control set; "115 V 60 Hz, 1/140 HP" | [web] forum + listing [VW2] (unverified) |
| Damper | one felt strip ½" × 1½" × 51" (12.7 × 38 × 1295 mm) on a spring-loaded bar, lifted by the pedal | [web] [M55P] |
| Frame | 56" long, 30"/15.5" wide at low/high end, 32–36" high, 137 lb | [web] retailers |

### 1.1 Bar geometry and undercut (computed table)

Interpolation: length and width are taken geometric between the end values above. A uniform free–free bar has
f₁ = (4.730²/2πL²)·h·√(E/12ρ). The "arch factor" is f₁,uniform / f_note: how much the undercut lowers the
fundamental below that of an un-arched bar of the same length **[calc]**.

| Note | f (A442) Hz | L mm | w mm | f₁ uniform Hz | arch factor | lateral₁ / f₁ | torsion₁ / f₁ (est.) |
|---|---|---|---|---|---|---|---|
| F3 | 175.4 | 381 | 57.0 | 461 | 2.63 | 11.8 | ≈ 7.1 |
| A3 | 221.0 | 349 | 54.5 | 548 | 2.48 | 10.6 | ≈ 6.4 |
| C4 | 262.8 | 328 | 52.7 | 624 | 2.37 | 9.8 | ≈ 5.9 |
| F4 | 350.8 | 294 | 49.8 | 775 | 2.21 | 8.7 | ≈ 5.2 |
| A4 | 442.0 | 270 | 47.6 | 921 | 2.08 | 7.8 | ≈ 4.7 |
| C5 | 525.6 | 253 | 46.0 | 1048 | 1.99 | 7.2 | ≈ 4.3 |
| F5 | 701.6 | 227 | 43.5 | 1301 | 1.85 | 6.4 | ≈ 3.8 |
| A5 | 884.0 | 208 | 41.6 | 1547 | 1.75 | 5.7 | ≈ 3.4 |
| C6 | 1051.3 | 195 | 40.2 | 1761 | 1.68 | 5.3 | ≈ 3.1 |
| F6 | 1403.3 | 175 | 38.0 | 2186 | 1.56 | 4.7 | ≈ 2.7 |

- The arch lowers the low bars by a factor of 2.6, more than an octave. This is the "deep arch" that keeps them
  short and puts mode 2 at 4:1 (a uniform bar has 2.756:1) **[calc]**. The high bars are lowered by only 1.6×.
  This is consistent with makers saying the top bars have no room for overtone tuning **[web, VW-Nico]**.
- **Lateral (in-plane) bending:** f ∝ w/L² and does not depend on h. A uniform-thickness cut changes the ratio
  only weakly, so the uniform-bar value is a fair estimate **[calc]**. It is 12 f₁ low and 4.7 f₁ high. A vertical
  strike barely excites it, and it radiates poorly (edge-on). Leave it out or add one quiet mode.
- **Torsion:** the uniform-bar value is f_t1 = (1/2L)·√(G·βwh³/(ρ·wh(w²+h²)/12)), with G = 27.5 GPa and
  β ≈ 0.29. The ratios in the table use an extra factor of 0.75 to allow for the thinned centre (my guess). Off
  the centre line, a strike excites t1 strongly. In the top octave t1 falls near the untuned mode 2, which is
  where torsional "pollution" is reported **[web, BS21, SAD21]**. Beaton & Scavone and Soares et al. tune torsion
  with 3-D cut-aways **[web]**. Production bars generally do not tune it **[memory]**.
- Nobody publishes the M55 arch profile. Bar-tuning optimisation papers (Petrolito & Legge 1997; Orduña-Bustamante
  1991; Henrique & Antunes 2003; Soares et al. 2020/21; Beaton & Scavone 2021) give typical profiles: a parabolic
  or flat-bottomed arch over the middle 50–60 % of the length **[web titles, profiles memory]**. Bork 1995:
  removing material at a mode's antinode lowers that mode most **[web, via secondary source]**.

### 1.2 Mode tuning per register

| Register | Mode 1 | Mode 2 | Mode 3 | Source |
|---|---|---|---|---|
| F3 – ~D4 | 1 | 4.00 (double octave) | 10.0 (3 oct + major 3rd: F3 → A6) | [web] Wikipedia citing Beaton & Scavone 2021; [VW-Nico] |
| ~D♯4 – ~G4 | 1 | 4.00 | ≈ 9.6 (3 oct + minor 3rd) | [web] [VW-Nico] ("change … to a minor 3rd at D♯4", "up to F♯4 or G4"; the text is ambiguous) |
| ~G♯4 – G5 | 1 | 4.00 | untuned (between 5.4 for a uniform bar and ≈ 9) | [web] [VW-Nico], range [calc] |
| G♯5 – F6 | 1 | untuned, ≈ 3.0–3.8 | untuned, ≈ 6–8 | [calc/guess]: shallow arch, between the uniform 2.756/5.404 and the tuned values |

- Measured on a C4 bar: 262.5 Hz and 1050 Hz, exactly 4.00 **[web, ICSV12]**. B4: 496.88 Hz and 1984.38 Hz
  (3.994). That paper also notes that mode 2 is excited "only about 4.4 dB" under the fundamental for an
  edge strike **[web, ICSV12]**.
- Builders' residuals on a CNC-cut C3 bar after near-final tuning: +15, +5, +2 cents on the three modes
  **[web, VW-Nico]**. In the model, a few cents of random mistuning per mode and bar is realistic.
- Uniform free–free ratios, for the untuned upper modes: 1 : 2.756 : 5.404 : 8.933 : 13.34 **[memory]** (the
  standard (βL)² series 4.730², 7.853², 10.996², 14.137², 17.279²).
- **Symmetry:** modes 1 and 3 are symmetric (antinode at the centre). Mode 2 is antisymmetric, with a **node at the
  centre**. A centre strike gives almost no 4:1 partial. A strike towards the end, as players do on the
  accidentals' near ends, brings it in **[calc from beam theory]**. This is a free tone-colour control.

---

## 2. Decay

### 2.1 The bar without a resonator, damper off

| Source | Value |
|---|---|
| Composite-marimba patent US4411187 (as common knowledge) | vibraphone decay "30 seconds in the low end to about 6 seconds in the high end" **[web, P187]** |
| Vibraphone builder (Nico, VibesWorkshop V-Book 2008) | low register ≈ 20 s with an unmatched resonator, 3–4 s with an exactly matched one **[web, VW-Nico]** |
| Soares, Debut, Antunes 2022: undercut Al bar on elastic strings, 169 Hz | ζ₁ = 0.0426 % → T60 = 6.91/(ζω) = **15.3 s**; mode 2 (505 Hz) ζ = 0.0243 % → 9.0 s; mode 3 (848 Hz) 0.0215 % → 6.0 s; mode 4 (1187 Hz) 0.0096 % → 9.7 s **[web, SDA22]; T60 [calc]** |
| ICSV 2012, a vibraphone in playing condition (resonators fitted; motor state not stated) | envelope decay rate σ = 0.4–1.4 /s over all notes **[web, ICSV12]** → T60 = 4.9–17 s **[calc]**; C4 alone σ = 1.204 → 5.7 s |

A fit for the bar alone (cords, no resonator), as one law for every mode of every bar **[calc]**:

    T60_bar(f) ≈ 25 s · (175 Hz / f)^0.75

This gives 25 s at F3, 8.9 s for F3's mode 2 (700 Hz), 4.4 s for its mode 3 (1750 Hz) and 5.9 s at F6. That is
within the patent's 30/6 s and close to Soares' 9 s and 6 s for modes 2 and 3. Use the same law for the torsional
and higher modes, ×0.7 for modes not centred on the cord nodes (more loss into the cords) **[guess]**.

### 2.2 With the resonator

- Rucz et al. 2021 (FE, rosewood xylophone F4): bar alone 3.03 s; with a perfectly tuned resonator **0.67 s**. A
  well-tuned resonator raises the level by **> 10 dB**. Neighbouring bars detune the resonator by **up to 15 %**
  **[web, RUAM21]**.
- Vibraphone, low register: 20 s → 3–4 s when bar and tube are matched exactly. Makers therefore tune the tube
  **"just below"** the bar. A tube more than half a semitone off gives no audible resonance **[web, VW-Nico]**.
- Resonators are "usually tuned slightly off-pitch to balance loudness and sustain" (Rossing 2000, p. 66, via
  [WV]) **[web]**.
- Stopped fan: horizontal (closed) gives more sustain and less volume; vertical (open) gives the most volume
  **[web, MJ]**.

### 2.3 Damper, half pedal, mallet dampening

- The M55 has **one** felt bar (12.7 × 38 mm) that a spring presses up against the underside of all 37 bars. The
  pedal pulls it away. There are no per-key dampers: **pedal up = every bar damped**, including the one just
  struck. A design patent gives one damper geometry: felt touching **8–12 % of the bar's underside**, trapezoidal,
  35–45 mm → 90–110 mm wide, crowned with an 80–130 mm radius **[web, P749]**.
- Damped decay: no measurement found. Played pedal-up, a note sounds as a short "dead" tone of roughly 0.2–0.5 s
  **[memory]**. Model: an extra decay rate **σ_d ≈ 15–30 /s** (T60 ≈ 0.25–0.45 s) at F3, rising ∝ f^0.5 to ≈ 50 /s
  at F6 **[guess, to fit]**. The felt touches near the bar end, where every bending mode has an antinode, so a
  uniform rate over all modes is a fair approximation. This is what `damp()` does.
- **Half pedal:** the felt grazes the bars and the rate falls continuously. Model σ = σ_d·(1 − p)² with p ∈ [0, 1]
  from CC64/127 **[guess]**. Real half-pedalling also has a buzz from intermittent contact. This could be done with
  the engine's contact patches, but it is optional.
- **Dead strokes and mallet dampening** (Burton technique): the mallet head pressed onto one ringing bar. A mallet
  left on the bar after the stroke is a soft one-sided spring and dashpot at the strike point. The Hertz contact in
  `strike` already allows a long contact (`mT` limit 50 ms). A dead stroke needs a "stay" flag plus a dashpot of
  ≈ 5–20 /s on that bar's modes **[guess]**. Mallet dampening by itself is `damp(i, ≈ 30 /s)` on one bar.

---

## 3. Resonators

### 3.1 Geometry

- Thin-walled aluminium tubes, closed at the bottom, open at the top just under the bar. The diameter is "slightly
  wider than" the bar. The length to the closure is a quarter wavelength of the fundamental **[web, P749]**.
- Diameters: "high register ≈ 1.5", low register ≈ 3"" **[web, P691]**. VSL gives 5–6 cm and C4 = 32.5 cm long,
  and says the tube is to be shortened by an end correction of "5:3 of the tube diameter" **[web, VSL]**. VSL's
  32.5 cm is c/4f itself (32.7 cm **[calc]**), and its 5/3·d end correction looks far too large. A Wikipedia
  mirror gives A3 ≈ 15" (38 cm) **[web]**. The M55's actual tube sizes are not published. On the M55 the four
  lowest tubes carry "resonator plates/rubbers", which suggests their bottoms are adjusted with plates or end caps
  **[web, M55P; interpretation]**.

Quarter-wave length, end correction δ = 0.61a (unflanged open end, no bar) **[memory]**, diameter interpolated
geometrically from 76 mm (F3) to 38 mm (F6). Q is split into radiation and boundary-layer parts: Q_rad = π/(ka)²
from |R| ≈ 1 − (ka)²/2, and Q_wall = k/2α with α = 3.0·10⁻⁵ √f / a Np/m **[memory for both forms; values calc]**:

| Note | c/4f cm | d mm | L_tube = c/4f − 0.61a, cm | Q_rad | Q_wall | Q | σ_r = ω/2Q, 1/s |
|---|---|---|---|---|---|---|---|
| F3 | 49.0 | 76 | 46.7 | 212 | 153 | 89 | 6.2 |
| C4 | 32.7 | 66 | 30.7 | 124 | 164 | 70 | 11.8 |
| F4 | 24.5 | 60 | 22.7 | 84 | 172 | 56 | 19.7 |
| A4 | 19.5 | 56 | 17.8 | 62 | 179 | 46 | 30 |
| C5 | 16.4 | 53 | 14.8 | 49 | 184 | 39 | 42 |
| F5 | 12.3 | 48 | 10.8 | 33 | 193 | 28 | 79 |
| C6 | 8.2 | 42 | 6.9 | 19 | 206 | 18 | 183 |
| F6 | 6.1 | 38 | 5.0 | 13 | 217 | 12 | 367 |

- Check: Soares et al. measured a lab tube at 174 Hz: **ζ = 0.73 % (Q = 68)**, rising to ζ = 0.94 % at the 4th
  mode **[web, SDA22]**. Rucz's xylophone Helmholtz resonator: ζ = 0.88 % **[web, RUAM21]**. This agrees with
  Q ≈ 70–90 low **[calc]**.
- The bar sits a few mm to cm above the mouth. It adds mass loading, so the effective end correction is larger
  than 0.61a, and the fan disc and shaft in the mouth add more. A player's report says a tube with its shaft and
  disc in place, fully open, sounds "around 3 semitones" lower **[web, forum, VW]**. That claim is unverified,
  and 3 semitones would be δ ≈ 0.19 L. Builders tune the assembled tube by ear, end cap or plate, "just below" the
  bar. Reinstalling the bars lowers the tube pitch "just a bit" **[web, VW-Nico]**.
- **Closed tube = odd harmonics** (1 : 3 : 5 : …, slightly flattened by the end correction). Bar modes 2 (4 f) and
  3 (10 f) land between tube modes and are not reinforced. The tube amplifies "the fundamental, but not the upper
  partials" **[web, P749]**.

### 3.2 Bar–resonator coupling: numbers for a two-oscillator model

Soares, Debut & Antunes reduce one bar mode and one tube mode to **two inertially coupled oscillators**. They
measured an undercut Al bar (1 : 3 : 5 : 7 tuned) over a tube **[web, SDA22]**:

- Bar modal masses (from their model): 0.045, 0.031, 0.024, 0.037 kg. Tube modal mass 0.61 g.
- Decay falls as the tube is tuned onto the bar mode, with a matching rise in the share radiated by the tube.
  Coupling weakens with the bar–mouth gap. It vanishes when the tube sits under a node of that mode. Their
  disc-shaped coupling model **under**-estimates the measured coupling.
- Rucz et al. (3-D FE) find the same trade-off and > 10 dB of gain **[web, RUAM21]**.

As modal envelope equations (rotating-wave form, as two unit-mass modes):
ȧ_b = −σ_b a_b − iκ a_r and ȧ_r = −(σ_r + iΔ) a_r − iκ a_b. With σ_b ≪ σ_r and κ < σ_r/2, the bar-like mode decays
at σ_b + κ²σ_r/(σ_r² + Δ²) **[calc]**. Fitting κ to "T60 bar → T60 with a tuned tube" **[calc]**:

| f | T60 bar | T60 with tube | σ_r | κ, rad/s | κ/σ_r | share radiated by tube | resonance half-width |
|---|---|---|---|---|---|---|---|
| 175 | 20–25 s | 3.5–4 s | 6.2–7.9 | 3.2–3.4 | 0.43–0.51 | 82–84 % | ±1.0–1.3 Hz (±10–12 c) |
| 351 | 14 s | 3 s | 19.7 | 6.0 | 0.30 | 79 % | ±3.1 Hz (±15 c) |
| 701 | 8 s | 3 s | 79 | 10.6 | 0.14 | 62 % | ±12.5 Hz (±31 c) |
| 1403 | 5 s | 3.5 s | 367 | 14.8 | 0.04 | 30 % | ±58 Hz (±71 c) |

(The T60 targets above F3 are my guesses, interpolated between the patent's numbers and Nico's.)

- **Low bars sit at the exceptional point, κ ≈ σ_r/2.** A little more coupling would split the pair into two
  beating modes, an audible "wah". This is the maximum-radiation design, and it is the reason the low register is
  so sensitive to resonator tuning **[calc]**.
- In the engine, as a stiffness coupling between unit-mass modes, x_b″ = … − k x_r and x_r″ = … − k x_b. The
  splitting is κ = k/2ω, so **k = 2ωκ ≈ 7·10³ s⁻² (F3) … 2.6·10⁵ s⁻² (F6)** **[calc]**. Then k·dt² ≪ 1, so an
  explicit term is stable.
- **Frequency pull:** a tube tuned below the bar by Δ pushes the bar up by κ²Δ/(σ_r² + Δ²), at most κ²/2σ_r. At F3
  that is 0.8 rad/s = 0.13 Hz ≈ **1.3 cents** **[calc]**. The fan therefore makes a vibrato of only about ±1 cent.
- **Bloom:** the tube builds up in ≈ 1/σ_r = 0.16 s at F3 and 3 ms at F6 **[calc]**. In the low register the
  resonator "swells" in after the attack.
- **Temperature:** the bar shifts by (1/2)(dE/E)/dT ≈ −0.38 cents/K, plus expansion −0.04 cents/K (dE/dT/E ≈
  −4.5·10⁻⁴ /K **[memory]**). The tube shifts ∝ c ∝ √T: +2.9 cents/K **[calc]**. A 5 K change is 16 cents of
  relative detuning, more than the F3 half-width. Builders note that it is audible at 3–5 °C, especially C3–C4
  **[web, VW-Nico]**. This is optional "temperature" realism for the model.

---

## 4. The fan (pulsator) and the "vibrato"

- One thin disc per tube, fixed across the tube's mouth on a shaft along the row. The M55 has two pulsator shafts
  (naturals, accidentals) with a pulley and one belt to the motor **[web, M55P]**. All discs on a shaft are set at
  the same angle, so **every tube on a row is modulated in phase**. The real tremolo is essentially mono, not a
  left–right pan **[web, forum, Pianoteq]**. The phase between the two rows is not documented. I would take it
  as 0 **[guess]**.
- A disc closes the mouth when face-on and opens it edge-on: **two openings per revolution**, so the tremolo rate
  is 2 × the shaft rate **[calc/standard]**. One builder says a tube tuned too high peaks at 45°, giving four
  maxima per revolution, "doubling the vibrato frequency" **[web, VW-Nico]**. In the coupled model this is
  detuning that moves through zero twice per opening.
- **Speeds:** Yamaha YVD10/YVD20 drivers **30–145 rpm**; Adams Concert 25–150 rpm (another listing says 40–140);
  Adams Nomad 20–160 rpm **[web, YVD, AD]**. That is a 0.33–2.7 Hz shaft and a **≈ 0.7–5.3 Hz tremolo** **[calc]**.
  Wikipedia: motor "1–12 Hz" (Meyer 1973) **[web, WV]**. A Deagan owner runs a retrofitted Yamaha motor at 3.5–6
  rpm (a slow "breathing") **[web, VW2]**. M55: an Oriental Motor speed-control set behind a belt reduction, ratio
  not published. Assume ≈ 20–160 rpm like its competitors **[guess]**. Yamaha's "pause memory" stops the shaft at
  a preset angle, open for volume or closed for sustain **[web, YVD]**.
- **Depth:** open "≈ 85–90 % of maximum resonance", closed "≈ 10–15 %": leakage around the disc means the tube never
  fully closes **[web, P691]**. Read as amplitude of the tube-radiated fundamental, that is 20·log(0.12/0.88) ≈
  **−17 dB** of swing. Read as power it is ≈ −9 dB **[calc]**. The perceived depth is less, because:
  (a) the bar radiates the fundamental directly; this matters little at F3 (dipole, ka ≈ 0.09) and a lot at F6
  (ka ≈ 0.7), so **the tremolo is deepest in the low register** **[calc]**;
  (b) the resonator cannot follow faster than σ_r: at F3, 1/σ_r = 0.16 s against a 0.2 s period at 5 Hz, so fast
  tremolo is shallower and smoother low, while the top follows instantly **[calc]**;
  (c) modes 2, 3 and torsion are not modulated at all.
- A **running** fan also lengthens the sustain compared with fan open. Take κ(θ) = κ₀(m + (1 − m)·o(θ)), with an
  open fraction o(θ) = 1 − |sin θ| and m ≈ 0.3. Then ⟨κ²⟩ ≈ 0.35 κ₀², and F3 goes from 3.5 s (open) to ≈ 7–9 s
  (running) and ≈ 14 s (closed) **[calc]**. This is a testable prediction (§7).
- Closing the mouth also **lowers the tube's frequency** (more end loading) **[web, VW-Nico]**. With the tube already
  tuned just below the bar, closing detunes it further and weakens the coupling twice over: once through the area
  and once through the detuning. Model the tube frequency as f_r(θ) = f_r0·(1 − d·(1 − o(θ))) with d ≈ 0.03–0.08
  (½–1½ semitones) **[guess, fit]**.

---

## 5. Mallets and other excitation

- Construction: a rubber ball core wound with yarn or cord, on rattan, birch or fibreglass handles **[web, WV]**.
  Cord keeps the articulation with a softer core, and yarn softens the contact sound **[web, Yamaha hub]**.
  Typical vibe mallet: core Ø 25–32 mm, head 12–20 g, handle 15–20 g, total 25–35 g each **[memory]**. Effective
  striking mass is about the head plus a third of the handle, **m ≈ 20–25 g** **[memory/calc]**.
- Contact time: no vibe-mallet measurement found. Rucz used a xylophone mallet of 35 g, K = 5·10⁷ N/m^1.5,
  T_int = 0.7 ms, and 0.4–1.5 ms over their runs **[web, RUAM21]**. My Hertz formula reproduces their 0.55–0.64 ms
  **[calc]**. Bork measured mallet shock spectra and found the upper frequency limit rises with blow strength
  **[web, abstract, Bo90]**. Chaigne & Doutaut 1997 used Hertz contact fitted to real xylophone mallets **[web]**;
  their numbers are not checked here.
- Hertz contact for the engine (mallet 25 g on a bar effective mass of 0.15 kg at F3 or 0.04 kg at F6; contact time
  τ at a strike speed of 0.5 m/s; τ ∝ v^(−(α−1)/(α+1))) **[calc]**:

| Grade | τ at 0.5 m/s | α = 2.5: K (N/m^2.5) | τ at 3 m/s | α = 1.5: K (N/m^1.5) | τ at 3 m/s | peak force 0.5 → 3 m/s |
|---|---|---|---|---|---|---|
| soft yarn | 3.0 ms | 2.3·10⁹ | 1.4 ms | 1.1·10⁶ | 2.1 ms | 13–17 → 110–220 N |
| medium cord | 1.5 ms | 2.6·10¹⁰ | 0.70 ms | 6.5·10⁶ | 1.05 ms | 26–34 → 230–440 N |
| hard | 0.7 ms | 3.8·10¹¹ | 0.32 ms | 4.3·10⁷ | 0.49 ms | 56–72 → 480–940 N |

  K is almost independent of the note, because the mallet is much lighter than the bar. Wound heads stiffen as
  they compress, so **α ≈ 2.5 is preferred**: velocity then brightens the tone by itself (τ halves from pp to ff).
  The engine's tinkle-bell felt hammer already uses α = 2.5.
- **Mallet filtering** (half-sine force spectrum, dB relative to DC) **[calc]**:

| τ | F3 f₁ 175 | F3 mode 2 700 | F3 mode 3 1754 | F4 f₁ 351 | F6 f₁ 1403 | F6 mode 2 ≈ 4–5 kHz |
|---|---|---|---|---|---|---|
| 3.0 ms | −2 | −25 | −44 | −11 | −39 | −60 or less |
| 1.5 ms | −1 | −11 | −36 | −2 | −25 | ≈ −45…−60 |
| 0.7 ms | 0 | −2 | −16 | 0 | −9 | ≈ −36…−44 |

  A soft mallet makes a near-pure fundamental in the low register. It also **muffles the top octave**, which is why
  players choose harder mallets or play harder up there. The F3 mode 2 is controlled jointly by mallet hardness and
  strike position, since a centre strike sits on mode 2's node.
- Strike speeds: pp ≈ 0.3–0.5 m/s, ff ≈ 3–4 m/s **[memory, as for the marimba]**. Map v = 0.3 + 3.2·(vel/127)^1.3.
- **Bowing:** a double-bass bow drawn vertically across the end of the bar. The end is an antinode of every
  bending mode, and stick-slip locks onto mode 1. The result is a pure, swelling sine with no attack, and
  harmonics are possible by damping a node **[web, WV; memory]**. The engine's `bow()` with `exc` = mode shapes at
  the bar end (a uniform-beam end value of 2 for every mode) covers it. Offer it as a keyswitch or alternate patch.
- **Pitch bend:** a hard rubber mallet pressed at a node and slid towards the centre lowers the pitch by about a
  semitone **[web, WV via Dimpker]**. This is optional: `retune(i, r, glide)` with r down to 0.94.

---

## 6. Radiation

- **Bar:** the top and bottom faces move in antiphase, so the bar is a dipole (a quadrupole along its length for
  mode 1, whose ends and middle move in opposite directions). At F3, k·w/2 ≈ 0.09 and the radiation efficiency is
  tiny, ∝ (kw)² for the dipole part **[calc/memory]**. At F6, kw/2 ≈ 0.49, which is respectable. This is why a
  vibraphone without resonators (Deagan's 1970 ElectraVibe, pickups instead **[web, WV]**) is thin in the bass.
- **Resonator mouth:** a monopole with volume velocity U, omnidirectional below ka ≈ 1, which holds for every
  vibraphone tube at its fundamental. At the low end it dominates the fundamental by ~20 dB (§2.2, Rucz's > 10 dB on
  a xylophone F4) **[calc/web]**. The underside of the bar drives the tube, while the top face radiates in
  antiphase. The tube's output and the bar's top-face output add with a phase that depends on direction, but at
  these wavelengths this is a few dB of up/down directivity, not more **[memory]**.
- **Overall level:** a vibraphone at ff reaches roughly 90–100 dB SPL at 1 m in the low/mid register **[memory]**.
  The player's ear is ~0.5 m above the bars.
- **Stereo:** the keyboard is ≈ 1.2 m long (frame 56"), low notes at the player's left. Pan each bar's `side`
  weight by its position, −0.5 … +0.5, for both bar and tube modes. The tremolo is in phase everywhere, so the side
  signal is modulated exactly like the mid signal.

---

## 7. Model plan in this engine

### 7.1 One bar = one `ModalSet::Def` (y only: `one = true`)

| k | Mode | freq | t60 (bar alone) | exc at strike | rad |
|---|---|---|---|---|---|
| 0 | bending 1 | f | 25·(175/f)^0.75 | φ₁(x_s) | dipole: r₀·(kw/2)² clamp ≤ 1 |
| 1 | bending 2 | 4f (≤ G5), else 3.0–3.8 f | law at its f | φ₂(x_s) (0 at centre) | same law |
| 2 | bending 3 | 10f / 9.6f / untuned (§1.2) | law at its f | φ₃(x_s) | same law |
| 3 | bending 4 | ≈ 17–20 f low (untuned; uniform 8.93 is only a floor) **[guess]** | law ×0.7 | φ₄(x_s) | same law |
| 4 | torsion 1 | 7.1 f (F3) → 2.7 f (F6), §1.1 | law ×0.7 | ∝ lateral offset of strike (random ±5 mm) | low, ×0.3 |
| 5 | torsion 2 / lateral 1 | 2 × t1 or §1.1 lateral | law ×0.7 | small | small |
| 6 | **resonator (air)** | f·(1 − 0.3 %) (tuned just below, ≈ −5 cents) **[guess]** | 6.91/σ_r from §3.1 (Q) | 0 (driven only through coupling) | r_res, fitted |

- **Mode shapes:** uniform free–free shapes with nodes moved out to 0.244 L for mode 1 are close enough
  **[calc/approx]**. `cantileverShape` exists, but a free–free helper `freeBarShape(k, x, L)` is needed (cosh/cos
  form, βL = 4.730, 7.853, 10.996, 14.137).
- **modalMass:** ≈ 0.15 kg (F3) → 0.04 kg (F6) at the centre **[guess from bar mass ≈ 0.5 kg (F3) after undercut;
  Soares' modal 0.045 kg for a smaller bar]**. This scales the mallet's bounce, not the pitch.
- **Mistuning:** ±3 cents random per mode and bar, from a seed. The tube gets ±5 cents.

### 7.2 What the engine lacks, and the smallest additions

1. **Linear mode coupling:** `Def::Lin { int p, i; float g; }` adds f0[p] −= dt·g·x[i]. It sits next to the `quad`
   loop. It provides the bar↔tube coupling (two terms, k = 2ωκ, §3.2) and the tube's **self-detune** (p = i = 6,
   g = ω_r² − ω_r0²; small, explicit, k·dt² ≲ 10⁻⁴). The fan rewrites these g's once per 16-sample block.
   Everything else (exact 2×2 stepping, damping) stays.
2. **Optional per-mode loss modulation** (the disc adds loss when closed: +14 % open, +91 % face-on in one
   simulation, unverified **[web, JDT]**). This can be done as a velocity term in `Lin`, or by recomputing that one
   mode's `c` matrix per block. Skip it at first, since the coupling and detune carry the effect.
3. **kMaxVib = 32 → ≥ 37** (or 40). With the pedal down, all 37 bars ring. At 16 floats per vibrator per array,
   the state grows by about 5 KB.
4. **Dead stroke:** a "stay" flag in `strike`, so the mallet rests on the bar with a dashpot (§2.3). This is
   optional.

### 7.3 Controls

- **Notes:** each key owns its bar; there is no pool, because the vibraphone has fixed bars. Note-on calls
  `strike(bar, m = 0.022, v(vel), K, α = 2.5)` with K chosen by the mallet grade (an instrument variant, or CC71
  "timbre"). Strike position: naturals x_s ≈ 0.5 L ± 0.05 L, accidentals ≈ 0.62 L (near end) **[guess]**, plus a
  little randomness. This gives the 4:1 partial for free.
- **Damper:** CC64 (continuous) sets p, and every bar gets `damp(i, σ_d(f)·(1 − p)²)`. Two modes:
  (a) *realistic*: the pedal is the only damper, key-off does nothing. (b) *GM-friendly* (default for files that
  never send CC64): with the pedal up, a held key is free and key-off damps that bar. This is what players do with
  mallet dampening.
- **Motor:** CC1 (mod wheel) sets the shaft rate, 0 = stopped, 1 → 20 … 160 rpm (exponential). The disc angle θ(t)
  is integrated so a speed change never jumps phase. Rest angle when stopped: open by default, CC to choose. The
  motor ramps over ≈ 0.5–1 s (inertia) **[guess]**.
- **Per block (16 samples):** o = 1 − |sin θ|; for each awake bar, κ = κ₀(f)·(0.3 + 0.7 o) and Δf_r = −d·(1 − o)·f.
  Write the three `Lin` g's. That is 37 × 3 writes per block.

### 7.4 Calibration targets (initial values)

- κ₀(f): log-interpolate 3.3 rad/s (F3), 6 (F4), 10.6 (F5), 15 (F6) (§3.2).
- σ_r from the Q table (§3.1). Resonator rad r_res: set so that at F3 with fan open the fundamental is ≈ 20 dB
  above the bar's own radiation, and ≈ 3–5 dB above it at F6 **[guess, fit to recordings]**.
- Damper σ_d: 20 /s (F3) → 55 /s (F6). Then check that a pedal-up staccato note lasts ≈ 0.3 s.

### 7.5 Cost

37 bars × 7 modes, rounded to 16 per vector, one direction: 592 mode-steps per sample when everything rings, plus
3 coupling terms per bar and one Hertz solve while a mallet is on. The engine already runs the 31-note steel pan and
the 32-voice pool in this budget. Sleeping bars (the existing `quiet` counter) bring the typical load down to a few
bars. Expect well under 1 % of a core at 48 kHz **[calc, by comparison with existing patches]**.

---

### 7.5b The motor as played

The motor runs by default at the slowest tremolo, 0.7 Hz, the discs turning the tubes off fully (open 0.15 +
0.85·|cos θ|, a rectified sine). The mod wheel speeds it up to 5 Hz; live, K5 sets its speed and K4 its depth
(0: the discs left open). It no longer follows the voice's vibrato cue: a vibist sets the motor and leaves it.
The tubes are in step but not exactly: the two shafts (naturals, accidentals) on one belt are set ~11° of disc
angle apart (`PJ_FAN_ROWS`, 0.2 rad; ~23° of the tremolo cycle) and sway ±15 % of that once a turn (the belt's
give), and each disc is up to ±3.4° off its shaft (`PJ_FAN_DISC`, fixed per seat). Measured: F3, F#3 and C5 held
8 s swing 10–15 dB peak to peak at 0.69 Hz, F#3's cycle 28° ahead of F3's. Against the SoundFonts (whose
vibraphones swing less or not at all) the GM distance went 2.03 → 2.15.

### 7.5c The motor in -play, and the pedal

In `-play` the vibist sets the motor for each section (src/midifile.cc), bar by bar, with changes at least four bars
apart, unless the file rides the mod wheel on the channel. A **slow shimmer** (0.7–1.3 Hz, slower for a gentler
part) goes with a solo piece, a solo passage in a slow song (under 100 bpm), a very gentle part, or rich harmony
(≥ 4.5 pitch classes sounding together at an onset) with little rhythm under it (under one drum hit a beat).
Otherwise the openings fall **on the eighth notes**, phase-locked to the song's clock: quarters when eighths would
pass 5 Hz, triplets or sixteenths when the motor cannot go that slowly. A change glides over 1 s, and a locked step
brings its openings onto the grid within that glide. The sound peaks ~30 ms after each eighth, the resonators'
build-up. The bushgrafts files with a vibraphone (couldwrite, desire, loveishere to stay, manteca, shinystockings)
are all drum-driven and lock to the eighths, or to the quarters for manteca at 180 bpm.

The **sustain pedal** works as the damper bar, by its depth. Fully up, a let-go bar is gripped (25 /s: down 40 dB
in 0.15 s). Fully down (from CC 100), it rings free. Between, the grip goes as the press squared: CC 64 gives
0.8 s, CC 90 2.7 s. Moving the pedal changes the grip on bars already let go.

### 7.6 The bars' places in the room

The bars lie in one plane, the naturals on the player's side of the damper felt and the accidentals beyond it,
towards the audience. Each bar's middle (over its resonator's mouth) is half its length plus half the ~18 mm gap
the felt presses into from the felt's line, so the rows close in from ~39 cm apart (centre to centre) at F3 to
~19 cm at F6. Along the frame the naturals stand in order, ~12 mm apart (22 bars of 57 → 38 mm under the 1295 mm
felt); each accidental is over the gap between its two neighbours. Left to right runs low to high (the player's
view, as the piano's), the accidentals nearer the listener.

`ModalModel::vibPlace` gives each bar's place; the model offers the bars as parts (`Tube::parts`), plus the rest
of its sound (the DC blocker's share) at the centre. The room (`Room::setParts`) gives every early path, direct
or off the walls, each bar's own length: the path's direction as it leaves the instrument, along the bar's
offset. Measured in the hall: the direct sound of F3 reaches the left microphone 1.1 ms before the right, D6's
the right 0.9 ms before the left; an accidental arrives ~1.1 ms (F#3) to ~0.6 ms (D#6) before its natural
neighbour. The late field still takes the whole sound (diffuse). Without a room the bars' sides place them
(by the same layout). `PJ_VIB_BARS=0`: one point; `PJ_VIB_SPREAD` scales the places (0: all at the centre, the
sum then the same as one point's, -132 dB).

## 8. Validation against recordings

- **Recordings:** the University of Iowa Electronic Music Studios "Musical Instrument Samples" have a vibraphone,
  note by note, with motor on/off, dampened and sustained, hard/soft mallets and **bowed** notes **[memory: check
  the exact sets]** <https://theremin.music.uiowa.edu/MIS.html>. Many sample libraries record "fans open" and "fans
  closed" separately (Soniccouture crossfades the two with an LFO) **[web, SC]**. That is exactly the
  coupled/decoupled pair needed to fit κ.
- **Measurements to fit, per note (F3, C4, F4, C5, F5, C6, F6):**
  1. Partial frequencies: ratios 1 : 4 : 10 and the torsional line (§1.2, §1.1). Measure them on a long FFT of the
     motor-off sustain.
  2. T60 per partial, motor off, from a Hilbert envelope of each band-passed partial. Fundamental with fan stopped
     open vs closed → κ₀ (§3.2); mode 2/3 decay → the bar's loss law (§2.1).
  3. Tremolo: depth (dB peak to trough of the fundamental's envelope) and waveform vs rate and register. Predicted:
     deeper low, shallower at fast rates in the low register, upper partials flat (§4).
  4. FM of the fundamental under tremolo: predicted ≤ ±1–2 cents (§3.2).
  5. Attack: the resonator bloom (0.1–0.2 s low) and the level of mode 2 vs strike position and mallet.
  6. Pedal-up staccato length and half-pedal decay.
- **Engine tests** (as `tools/test.py` does for the others): the energy decays at the target σ with the fan fixed;
  open/closed T60 ratio at F3 ≈ 3–4; the tremolo rate in the output envelope = 2 × the shaft rate; no click when
  CC1 changes; damping with all 37 bars ringing stays within budget.

---

## 9. Main gaps

- No published M55 bar profiles, tube lengths or motor rpm. The geometry here is interpolated from generic 37-bar
  data and patents. A tape measure on one instrument (5 bars and 5 tubes) would settle it.
- No measured vibraphone mallet contact times; the K values are inferred from τ targets.
- Rossing's *Science of Percussion Instruments* (ch. on keyboard percussion) and Fletcher & Rossing §19 almost
  certainly contain measured vibraphone decay curves with discs open/closed and modulation spectra. I could not
  read them online; they are cited here only through Wikipedia.
- Damper felt rates and half-pedal behaviour are guesses.
- The phase between the naturals' and accidentals' pulsator shafts is unknown.

---

## References

[AD] Adams Concert / Soloist / Nomad vibraphones, motor 25–150 / 40–140 / 20–160 rpm. https://www.adams-music.com/en/percussion/vibraphone/nomad ; https://www.steveweissmusic.com/product/33795/vibraphone
[Bo90] I. Bork, "Measuring the acoustical properties of mallets", Applied Acoustics 30, 207–218 (1990). (Abstract only, via) https://publica.fraunhofer.de/handle/publica/269354
[Bo95] I. Bork, "Practical tuning of xylophone bars and resonators", Applied Acoustics 46, 103–127 (1995), doi:10.1016/0003-682X(95)93953-F. (Not read; cited in [SDA22], [RUAM21].)
[BS21] D. Beaton, G. Scavone, "Three-dimensional tuning of idiophone bar modes via finite element analysis", JASA 149(6), 3758–3768 (2021), doi:10.1121/10.0005062. (Abstract.)
[CD97] A. Chaigne, V. Doutaut, "Numerical simulations of xylophones. I. Time-domain modeling of the vibrating bars", JASA 101, 539–557 (1997), doi:10.1121/1.418117; Part II (resonator, radiation): V. Doutaut, D. Matignon, A. Chaigne, JASA 104, 1633 (1998). (Not read.)
[CS] Conn-Selmer, Musser M55B Pro-Vibe. https://connselmer.com/instruments/mallet-instruments/musser-pro-vibe-vibraphone-m55b
[FR98] N. H. Fletcher, T. D. Rossing, *The Physics of Musical Instruments*, 2nd ed., Springer 1998, ch. 19. (Not read.)
[GRN] Grinnell College Musical Instrument Collection, vibraphone (37 bars, 6.9–15" long, 1.5–2.25" wide, 0.52" thick). https://omeka-s.grinnell.edu/s/MusicalInstruments/item/2395 (via search summary; the page itself returned 403)
[ICSV12] "Discussions of sound characteristics of vibraphone by different playing techniques", ICSV19, Vilnius 2012, paper 259. https://val.npust.edu.tw/wp-content/uploads/2021/11/259-ICSV-Discussions-of-Sound-Characteristics-of-Vibraphone-by-Different-Playing-Techniques.pdf
[JDT] jdthaler/forte PR #79, a vibraphone simulation's disc-loss model (numbers from simulation, not measurement). https://github.com/jdthaler/forte/pull/79
[M55P] Ludwig/Musser M55 Pro-Vibraphone parts list (motor E36191, control E3634, belt 5¾", damper felt ½"×1½"×51", node wire, A=442 since 1/1/94). https://www.ludwig-drums.com/application/files/7514/6531/8474/M55.pdf
[MJ] malletjazz.com, "Tone production on mallet instruments" (fan vertical = volume, horizontal = sustain; resonators shorten sustain). https://www.malletjazz.com/lessons/tone_les.html
[P187] US 4,411,187, "Composite marimba bars" (vibraphone decay 30 s low to 6 s high). https://patents.google.com/patent/US4411187
[P691] US 7,732,691 B2, "Methods and apparatus for vibrato effects in keyboard percussion musical instruments" (85–90 % / 10–15 %, tube Ø 1.5"/3", 37 disks). https://patents.google.com/patent/US7732691
[P749] US 2010/0192749 A1, "Vibraphone" (2024-T4 bars, nodes at 0.244 L, quarter-wave tubes slightly wider than the bar, damper geometry). https://patents.google.com/patent/US20100192749
[PL] J. Petrolito, K. A. Legge, "Optimal undercuts for the tuning of percussive beams", JASA 102(4), 2432–2436 (1997). (Not read.)
[RUAM21] P. Rucz, M. Á. Ulveczki, J. Angster, A. Miklós, "Simulation of mallet percussion instruments by a coupled modal vibroacoustic finite element method", JASA 149(5), 3200–3212 (2021), doi:10.1121/10.0004216. http://real.mtak.hu/129618/1/Rucz_JASA_2021_paper.pdf
[SAD21] F. Soares, J. Antunes, V. Debut, "Tuning of bending and torsional modes of bars used in mallet percussion instruments", JASA 150(4), 2757–2769 (2021). (Abstract.)
[SAD20] F. Soares, J. Antunes, V. Debut, "Multi-modal tuning optimization of vibrating bars with simplified undercuts using an evolutionary algorithm", TecniAcústica/Faro 2020; Applied Acoustics 173 (2021). https://www.sea-acustica.es/archivo/congresos/Faro20/ID02.pdf
[SDA22] F. Soares, V. Debut, J. Antunes, "The vibroacoustic interaction in mallet percussion instruments: modelling and experiments", 53º Congreso Español de Acústica, Elche 2022 (Table 1: bar and tube modal parameters). https://www.sea-acustica.es/archivo/congresos/Elche22/ID-187.pdf ; the multi-modal model in J. Sound Vib. (2023).
[SC] Soniccouture Vibraphone (fans open / closed sampled separately). https://www.soundonsound.com/reviews/soniccouture-vibraphone
[VSL] Vienna Symphonic Library, Vibraphone (tubes 5–6 cm, C4 tube 32.5 cm). https://www.vsl.co.at/academy/percussion/vibraphone
[VW] VibesWorkshop forum threads (2024-T4; ~3 semitones with shaft and disc). https://www.vibesworkshop.com/node/9468 ; https://vibesworkshop.com/node/1243
[VW-Nico] Nico, "Bars, resonators. Their behaviour and how to act with it", VibesWorkshop V-Book, 6 Dec 2008. https://www.vibesworkshop.com/story/bars-resonators-their-behaviour-and-how-act-it/nico/050316
[VW2] VibesWorkshop motor threads (Bodine → Oriental Motor; Yamaha motor at 3.5–6 rpm). https://vibesworkshop.com/node/17628 ; https://vibesworkshop.com/node/7602
[WV] Wikipedia, "Vibraphone" (fan 1–12 Hz citing Meyer 1973; F3 overtones F5, A6 citing Beaton & Scavone; nodes 22.4 %; pitch bend; ElectraVibe). https://en.wikipedia.org/wiki/Vibraphone
[YVD] Yamaha YVD10 / YVD20 vibraphone variable-speed drivers, 30–145 rpm, pause memory. https://fr.yamaha.com/fr/products/musical_instruments/percussion/vibraphones/yvd10/specs.html ; https://europe.yamaha.com/en/musical-instruments/percussion/products/vibraphones/yvd20/
[YVT] Yamaha YVT-2700 specs (F33–F69, bars 1½–2¼" × ½"). https://usa.yamaha.com/products/musical_instruments/percussion/vibraphones/yvt-2700/specs.html
