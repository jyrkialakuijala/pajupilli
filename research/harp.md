# Concert (pedal) harp: research

Status: built (2026-11-19): `-inst harp`, GM "Orchestral Harp" (program 47 counting from 1, 46
from 0), in `src/harp.cc` (strings, pedals, board), `src/harpinst.cc` (the harpist) and
`tools/harpboard.py` (the string table and the board); README.md "Concert harp" says what was
fitted and what is not done. Section 8 names the existing code it was built from.

How each number is marked (as in `timpani.md`, `viola.md`):
- **[T]** read in the primary text (downloaded and read for this note). "Measured" is said in words
  where the text reports a measurement and not a calculation.
- **[S]** from a secondary source, a search snippet, a retailer page or a machine summary of a page:
  the primary text was not read.
- **[L]** player or maker lore: a teaching text, orchestration guide, maker's manual or forum. Not
  a measurement.
- **[E]** my estimate, fit or arithmetic from cited values; not a published value.
- **[M]** from general knowledge, not looked up: check before relying on it.

References are short keys in brackets; the list with URLs is section 10. What could not be found
is in section 9.

**Note names.** Harp literature uses three octave conventions, and they disagree. This note uses
scientific pitch (A4 = 440 Hz, C4 = middle C) throughout and converts:
- Harpists number strings from the top: 00 G, 0 F, then 1 = the highest E, ... 45 = the lowest C,
  and the "octaves" run E down to F (octave 1 = E7..F6, octave 7 = E1, D1, C1) **[T]** [Camac].
- [Chadefaux12] write "D♭2" for 138.6 Hz (scientific D♭3/C♯3) and give the range as C♭0–G♭6;
  [LeCarrou10] label 46.2 Hz "G6" (harp octave numbering). [LeCarrou09] uses scientific names
  (C♭3 = 123.6 Hz). Every frequency below was checked against its stated Hz.

**Short answer.** A concert grand pedal harp has 47 strings, C1 (32.7 Hz) to G7 (3136 Hz). They
are 1.5 m to 7 cm long and all tuned to C♭ major when the pedals are up. On a Camac, the top 9
(G7–F6) are nylon, the next 26 (E6–A2) gut, and the bottom 12 (G2 down) copper- or silver-wound
steel.
Tensions rise from about 40 N at the top to 250–300 N on the lowest gut strings and 350–620 N on
the wires. The board carries a total of about 10 kN (an estimate; published figures run from 7 to
20 kN).

The strings end along the centre line of a tapered spruce soundboard, 1.4 m long, 0.5 → 0.1 m
wide and 11–12 → 2–2.5 mm thick. The board sits on a semi-conical box of about 29 litres with
five holes in its back. There is no bridge: each string drives the board at its own point. The
board's antinodes climb the board as frequency rises, roughly following the strings. The lowest
radiating pair are T1 (154 Hz) and A0 (172 Hz), with A0 above T1, unlike the guitar or violin.
Below about 140 Hz the box barely radiates, so the lowest dozen strings are heard by their
overtones. Below about 220 Hz the harp radiates evenly all round; above that the lobes change
from one frequency to the next.

The string is plucked with the finger pad a third to two-fifths of the way up from the board.
The finger holds the string for 0.1–0.4 s, then lets it slip off in 1–6 ms. At release the string
is 0.5–7 mm out and moving at 0.2–5 m/s, and it has been rotated by 7–82°. A finger pad 2 cm wide
removes nothing below 7–10 kHz. What does cut the top is the string's own internal loss, which
makes every gut and nylon string a roughly 10-partial instrument.

The plucked string vibrates in an ellipse that collapses and swells. Its two polarizations decay
at different rates (on one B2 string, T60 10 s and much longer). The other strings are undamped,
and those whose partials coincide ring on as "sympathetic modes" with T60 of 25–60 s: the halo.
Because the string meets the board at an angle (about 35°), the string's tension modulation
(at 2× each partial, and sums) drives the board directly. That gives a pitch glide and "phantom
partials" that matter as much as the even partials a mid-string pluck should lack.

Sound power: 54 dB (softest pp) to 91 dB (loudest ff), re 1 pW. It is one of the quiet
instruments.

---

## 1. The instrument

- **Form.** A triangle: a pillar (column) carrying the pedal rods, a curved neck carrying pegs,
  bridge pins and the disc action, and a soundbox whose front face is the soundboard. The strings
  lie in one plane, perpendicular to the board **[T]** [AT24].
- **Size** **[S]** [VSL]: pillar about 183 cm; soundbox about 145 cm long (maple), five sound
  holes; weight about 35 kg. The strings meet the board at about 36° **[S]** [VSL]. The small
  harp of [Wood22] has 35° **[T]**.
- **Strings:** 47 on a concert grand (46 on semi-grands, about 40 on the smallest pedal harps),
  C1 or D1 to G7 **[T]** [Waltham08, Camac]. "Cb1 – G#7" **[S]** [VSL]; the top G♯ is doubtful,
  since the top G has no disc (section 4).
- **Colour code:** all C strings red, all F strings blue or black, the rest white **[T]** [AT24,
  Camac].
- **Range used for the model:** MIDI 24 (C1) to 103 (G7), with C♭1 (MIDI 23) when the lowest C
  is tuned down by hand (it has no disc) **[E]**.

## 2. Strings

### 2.1 Materials by register

| Strings (harp no.) | Notes | Material | Source |
|---|---|---|---|
| 00–7 | G7–F6 | nylon (Camac: "Nylon Classique") | **[T]** [CamacChart] |
| 8–33 | E6–A2 | gut (standard, light, "English" or heavy gauge); nylon optional | **[T]** [CamacChart] |
| 34–45 | G2–C1 | steel core, copper (or silver-plated copper) wound | **[T]** [CamacChart]; [Galli] **[S]** |

- one harp maker's standard stringing for the Chicago 47: Bow Brand pedal nylon at the top, Bow
  Brand pedal gut in the middle, Bow Brand pedal wire (optional silver/copper) in the bass
  **[T]** [HCchart]. Some retailers list nylon in the 0 and 1 octaves and optional nylon in
  octave 2 **[S]**.
- **Why gut in the middle:** for these long, high-tension strings, the damping that rises with
  frequency (§2.4) sets the brightness limit. That limit is higher for gut than for nylon, so gut
  allows a heavier, better-feeling string that still rings brightly **[T]** [WLA19]. A second
  reason: nylon strain-stiffens (its Young's modulus rises while it is tensioned), gut does not.
  Nylon therefore has the higher E in use, and stronger pitch glide and phantom partials (§5.6)
  **[T]** [Wood22].
- **Why wire in the bass:** to keep the string length (the neck's double curve) and the "feel"
  reasonable. A gut or nylon string at those pitches would have to be very thick, very
  inharmonic and too slack, and would touch its neighbours **[T]** [Waltham05]. Players notice an
  "awkward change in feel at the break" **[T]** [Waltham05].

### 2.2 Lengths, diameters, tensions (gut and nylon)

Lengths and gut gauges for the top 35 strings of an Elysian Cecilia 46 pedal harp, with Bow
Brand "Pedal Light" gut and "Pedal Nylon" gauges **[T]** [WLA19 Table III]. The other columns
are mine **[E]**:
- T = ρ (π d²/4)(2 L f)², gut ρ = 1320 kg/m³, f at the natural pitch. In the flat position, which
  uses the full length, the tension is ×0.89.
- c = 2 L f, the transverse wave speed.
- B = π³ E d⁴ / (64 T L²), with E = 6 GPa for gut [WLA19].
- Z0 = √(T μ), the string's wave impedance.
- Feel 4T/L: the force per mm of mid-string displacement [WLA19 eq. 10].

| No. | Note | f (Hz) | L (mm) | gut d (mm) | nylon d (mm) | T (N) | c (m/s) | B | Z0 (N·s/m) | feel (N/mm) |
|---|---|---|---|---|---|---|---|---|---|---|
| 00 | G7 | 3136 | 73 | 0.41 | 0.40 | 37 | 458 | 4.2e-4 | 0.08 | 2.0 |
| 0 | F7 | 2794 | 81 | 0.43 | 0.41 | 39 | 453 | 3.9e-4 | 0.09 | 1.9 |
| 1 | E7 | 2637 | 87 | 0.45 | 0.43 | 44 | 459 | 3.6e-4 | 0.10 | 2.0 |
| 3 | C7 | 2093 | 103 | 0.50 | 0.50 | 48 | 431 | 3.6e-4 | 0.11 | 1.9 |
| 6 | G6 | 1568 | 133 | 0.64 | 0.66 | 74 | 417 | 3.7e-4 | 0.18 | 2.2 |
| 10 | C6 | 1047 | 182 | 0.76 | 0.79 | 87 | 381 | 3.4e-4 | 0.23 | 1.9 |
| 13 | G5 | 784 | 231 | 0.88 | 0.88 | 105 | 362 | 3.1e-4 | 0.29 | 1.8 |
| 17 | C5 | 523 | 315 | 1.05 | 1.10 | 124 | 330 | 2.9e-4 | 0.38 | 1.6 |
| 19 | A4 | 440 | 367 | 1.15 | 1.18 | 143 | 323 | 2.6e-4 | 0.44 | 1.6 |
| 20 | G4 | 392 | 397 | 1.20 | 1.25 | 145 | 311 | 2.6e-4 | 0.46 | 1.5 |
| 24 | C4 | 262 | 550 | 1.45 | 1.47 | 181 | 288 | 2.4e-4 | 0.63 | 1.3 |
| 27 | G3 | 196 | 714 | 1.75 | 1.79 | 249 | 280 | 2.2e-4 | 0.89 | 1.4 |
| 31 | C3 | 131 | 965 | 2.00 | 2.01 | 264 | 252 | 1.9e-4 | 1.05 | 1.1 |
| 33 | A2 | 110 | 1067 | 2.20 | 2.20 | 276 | 235 | 2.2e-4 | 1.18 | 1.0 |

- **Cross-check against a different harp (Camac Atlantide Prestige), measured** **[T]**
  [Chadefaux12 Table I]: 1109 Hz, 86 N, 17.6 cm, 0.70 mm; 554.4 Hz, 129 N, 31.0 cm, 1.02 mm;
  277.2 Hz, 168 N, 54.2 cm, 1.32 mm; 138.6 Hz, 260 N, 97.5 cm, 1.96 mm (all gut). These agree
  with the table within about 10 %.
- **Shortest and longest:** 7 cm and 150 cm **[S]** [VSL]. Grinnell College's catalogue gives
  about 61 in. (155 cm) and 2.9 in. (7.4 cm) **[S]** [Grinnell].
- **Camac gut gauges** (standard gut, 1/100 mm) **[T]** [CamacChart]: E6 0.70, C6 0.78, C5 1.00,
  C4 1.40, C3 2.07, A2 2.29. Nylon top: G7 0.51 to F6 0.71. Heavy gauge is 4–9 % thicker, light
  4–6 % thinner.
- **The design rule seen in the table** **[T]** [WLA19]: tension rises steadily from treble to
  bass, so the feel (4T/L) stays nearly constant, 1–2 N/mm. Impedance, and so loudness, is *not*
  kept constant: Z0 rises 15× from top to A2. The gauges sit close to the damping threshold
  (λ ≈ 0.05, §2.4) across the whole range.
- **Inharmonicity of gut and nylon: small** **[E]**. B is 2–4 × 10⁻⁴, so partial 10 is
  +1.0–2.1 % sharp (17–35 cents). Partial 10 is about where the string's damping ends its useful
  range anyway. [Wood22] measured the reduced frequency f_j/j rising along eq. (3) on a small-harp
  nylon string, up to mode 40 **[T]**.

### 2.3 Wire strings (G2–C1)

- **Tensions** (silver-plated copper wound, G34–C45, from the maker's table) **[S]** [Galli]:
  G2 491 N, F2 463, E2 560, D2 515, C2 475, B1 444, A1 436, G1 410, F1 356, E1 349, D1 368,
  C1 364 N. Galli's nickel-wound set: 365–620 N **[S]**. A harp calculator gives "35–85 lb"
  (156–378 N) for pedal-harp bass wires **[S]**.
- **Overall diameters** (Camac "standard wires", 1/100 mm) **[T]** [CamacChart]: G2 1.60, F2 1.73,
  E2 1.82, D2 1.92, C2 2.00, B1 2.10, A1 2.20, G1 2.30, F1 2.45, E1 2.54, D1 2.70, C1 2.80 mm.
  The Camac chart also lists light wires (1.37–2.60 mm) for B2 and A2 on some models.
- **Lengths:** not found per string. C1 is about 1.50 m **[S]** [VSL, Grinnell]; A2 is 1.07 m on
  the Elysian. Between them: G2 ≈ 1.10, C2 ≈ 1.30, C1 ≈ 1.50 m **[E]** (the neck's curve
  flattens the growth at the bottom).
- **Derived** **[E]**:
  - C1 with T ≈ 364 N and L ≈ 1.50 m: c = 2 L f = 98 m/s, μ = T/c² = 0.038 kg/m.
  - That μ is about 70 % of solid copper of 2.8 mm (0.055 kg/m), which is plausible for a
    wound string.
  - Inharmonicity depends on the steel core, whose diameter was not found. With a 0.6–1.0 mm
    core (E = 200 GPa), B ≈ 2 × 10⁻⁵ (G2) to 5 × 10⁻⁵ … 1 × 10⁻⁴ (C1). That is lower than the gut
    strings: "the steel keeps the inharmonicity low for the lowest strings" **[T]** [Waltham05].
- "Maximum stress (and tension) occurs in the steel strings"; "the highest steel and nylon
  strings feel the tightest to the player" **[T]** [Waltham05, figure captions; one harp maker Aurora data,
  numbers not legible].

### 2.4 String damping (the string alone)

- **Model and constants** **[T]** [Wood22, WLA19]. The loss factor of transverse mode j is
  η_j = η_air + η_bend (+ η_body, §6):
  - η_air = (ρa/ρ)(2√2 M + 1)/M², with M = (d/4)√(ω/νa), ρa = 1.2 kg/m³ and
    νa = 1.5 × 10⁻⁵ m²/s (Stokes; the same as [FR] eq. 2.40).
  - η_bend = ηE · E π⁴ d² j² / (16 ρ L⁴ ω1²): the material's own loss, through the bending. It
    grows as j², so it ends the useful partials.
  - Gut: E = 6.0 GPa, E″ = 0.24 GPa (ηE = 0.04), ρ = 1320 kg/m³.
  - Nylon: E ≈ 4.5 + 39σ GPa (σ the stress in GPa), E″ = 0.25 GPa, ρ = 1070, so ηE falls from
    0.04 to 0.02 as the stress rises.
  - Fluorocarbon: E ≈ 3.2 + 41σ GPa, E″ = 0.18 GPa, ρ = 1800 kg/m³.
- **The brightness limit** **[T]** [WLA19]. A partial n counts as useful while λ = the bending
  share of its energy stays under about 0.05; at that point η_n ≈ 10⁻³. Harp gauges run close to
  this line, so about 10 overtones are left **[T]** [Wood22]. Gut has a wider bandwidth than
  nylon on every string **[T]** [WLA19 Fig. 11].
- **T60 of the string alone** (air plus internal loss, the board's loss *not* included, so an
  upper bound), from the formulas above with the gut gauges of 2.2 **[E]**:

| String | partial 1 | partial 3 | partial 5 | partial 10 |
|---|---|---|---|---|
| G7 (3136 Hz) | 0.9 s | 0.4 s | 0.2 s | — |
| C6 (1047 Hz) | 2.9 s | 1.4 s | 0.7 s | 0.1 s |
| A4 (440 Hz) | 6.9 s | 3.4 s | 1.8 s | 0.4 s |
| C4 (262 Hz) | 11 s | 5.6 s | 3.1 s | 0.7 s |
| C3 (131 Hz) | 22 s | 11 s | 6.6 s | 1.7 s |
| A2 (110 Hz) | 26 s | 13 s | 7.4 s | 1.8 s |

  Air loss is the larger part up to about partial 5 and internal loss above. One measured B2
  string decays faster (T60 about 10 s for its board-driving polarization, §6.1): the board takes
  more than half.
- **Longitudinal modes** **[E]**: f_L1 = √(E/ρ)/(2L) = 2132 m/s / (2L) for gut. That is about
  1.0 kHz on A2 (9.1 × f0), 1.9 kHz on C4 (7.4 ×), 5.9 kHz on C6 (5.6 ×) and 14.6 kHz on G7
  (4.7 ×). On the small harp's nylon E4, measured: 3.3 and 6.6 kHz, more damped than the
  transverse modes, ζ = ηE ≈ 0.035 for every longitudinal mode **[T]** [Wood22].

### 2.5 Total load on the soundboard

- Sum of the 35 gut and nylon strings (table above, natural pitch): 4.9 kN; flat position 4.4 kN
  **[E]**. The 12 Galli wires: 5.2 kN (silver) or 6.0 kN (nickel) **[E]** from [Galli].
  **Total about 10–11 kN** **[E]**.
- Published: "on the order of 12 kN, over a ton" **[T]** [AT24]; "about 12–20 kN" **[T]**
  [Waltham08]; "700–1400 kgf" (7–14 kN) **[S]** [VSL]; "near two tonnes" **[T]** [LeCarrouRNRM];
  "6000 N in this case" for a harp Waltham built **[T]** [Waltham05].
- About 35° from the board (§1), so about 57 % of the load (sin 35°) pulls the board out
  normal to its face and 82 % (cos 35°) pulls along it **[E]**. The board bulges under it, and
  harps "occasionally explode" **[T]** [Waltham05].
- The strings stiffen the board. Their longitudinal (axial) stiffness at each attachment raises
  the board's resonances in a strung harp, more than the tension itself does **[T]** [Wood22],
  observed by [WaltKot08].

## 3. Soundboard and body

### 3.1 The board

- **Shape:** approximately trapezoidal, about 1.4 m long, 0.5 m wide at the base (bass) and
  0.1 m at the top **[T]** [Waltham08].
- **Thickness:** 11–12 mm at the bass end, falling to 2–2.5 mm at the treble **[T]** [Waltham08].
  "10 mm" **[S]** [VSL]. The small lever harp of [Wood22]: 780 mm long, 250 → 80 mm wide,
  5.0 → 2.5 mm thick, plain spruce **[T]**.
- **Construction:**
  - Strips of Sitka or Engelmann spruce, 3–8 cm wide, glued edge to edge with the grain running
    *across* the board (horizontally), so as to take the string pull **[T]** [Waltham08, AT24,
    Waltham05].
  - On the front, a thin veneer with vertical grain, partly for looks and partly against
    cracking; it "degrades the sound slightly" **[T]** [Waltham05, AT24].
  - Camac: a sandwich of several layers (aluminium, carbon, woods), a central aluminium bar and
    two lateral wooden stiffeners **[T]** [LeCarrou09].
- **The central strip (string bar):** a vertical strip of wood down the centre of the inside of
  the board takes the string attachments. Nylon plugs or eyelets reinforce the holes, since "the
  large tension … would rapidly pull the strings into the wood"; each string is knotted below its
  eyelet **[T]** [AT24, LeCarrou09].
- **Equivalent beam** (Camac; fitted, not measured directly) **[T]** [LeCarrou09]:
  - Clamped at both ends, at the pillar level and at the 11th string: the treble part barely
    moves at low frequency because the box edges stiffen it. So the beam is 1 m long and carries
    35 strings.
  - ρ = 553 kg/m³, A_eq = 38.3 cm², J_eq = 38.9 cm⁴, E fitted to 5.9 GPa (first bending mode
    150 Hz with the strings on).
  - As a free beam this gives modes at about 117, 320, 630, 1050 Hz **[E]** (clamped–clamped,
    EJ = 2295 N·m², μ = 2.1 kg/m).

### 3.2 The soundbox and its holes

- Semi-conical shell of hardwood veneer (beech, birch, maple) moulded on a form. Strong U-shaped
  ribs (beech, aluminium or steel) inside the back keep it from flexing **[T]** [Waltham08,
  Waltham05].
- **Camac Atlantide Prestige:** 6 mm thick shell, enclosed air 0.029 m³ **[T]** [LeCarrou07]. Five
  elliptical holes in the back, from top (1) to bottom (5) **[T]** [LeCarrou07 Table I]:

| Hole | major × minor (cm) | area (cm²) **[E]** |
|---|---|---|
| 1 | 16.6 × 4.8 | 63 |
| 2 | 17.2 × 5.6 | 76 |
| 3 | 17.7 × 7.0 | 97 |
| 4 | 18.1 × 8.0 | 114 |
| 5 | 18.5 × 9.3 | 135 |

- Most harps have "four or five soundholes in the back … and one in the base" **[T]**
  [Waltham08]. Their main purpose is access to change strings, but they matter acoustically
  **[T]** [LeCarrou07, Waltham09]. Harps over 200 years old often had small holes in the
  soundboard and none in the back **[T]** [Waltham09].
- **Helmholtz frequency with rigid walls** **[E]**: Σ A_i / L_eff,i ≈ 0.48 m (end correction
  1.7 r_eq + 6 mm) and V = 0.029 m³ give f ≈ (c/2π)√(ΣA/L / V) ≈ 220 Hz. The measured air mode
  sits lower (172 Hz) because the walls yield (§3.3).

### 3.3 Low modes, measured

Camac Atlantide Prestige, strung and tuned, strings damped with paper; shaker at the back of the
board between the A♭2 and B♭2 attachments. Measured **[T]** [LeCarrou07 Table II, Fig. 4]:

| Mode | f (Hz) | What moves |
|---|---|---|
| 1 | 61.5 | whole body on its supports (arm, pillar foot); no volume change |
| 2 | 84.5 | box bending like a simply-supported free beam; weak air coupling |
| 3 | 124.5 | as mode 2, next shape |
| 4 | 153.5 | **T1**: first bending of the board (quasi-clamped plate); the box breathes; air in the holes moves *in phase* with the board |
| 5 | (between) | pitch (twisting) mode: node on the centre line, where the strings attach, so it is hardly excited |
| 6 | 172 | **A0**: same board shape, air in the holes *out of phase* with the board |

- **Identification:** a 200 g mass on the board lowers mode 4 by 5 Hz and mode 6 by only 3.5 Hz.
  A 2 cm chimney on the lowest hole lowers mode 6 by 3 Hz and mode 4 by 1.5 Hz. Closing all holes
  makes mode 6 vanish **[T]** [LeCarrou07]. So mode 4 is T1 and mode 6 is A0.
- **A0 lies above T1** (unlike the guitar and violin): "the concert harp does not take advantage
  of the soundbox resonance to increase its sound radiated in low frequencies" **[T]**
  [LeCarrou07]. The low response can be modelled as a 2-degree-of-freedom oscillator (board plus
  air piston) **[T]** [LeCarrou07]. Damping coefficients were measured (their Fig. 4) but are only
  in the figure.
- **Air in the holes** **[T]** [LeCarrou07]. Hole 1 (top) barely moves; the lower a hole, the
  faster its air. Below f_A = 250 Hz the four lower pistons move in phase: one acoustic mode.
  Below f_B = 160 Hz the board and the pistons move together; between f_B and f_A their phase
  difference goes from 0 to 180°. Another Camac harp gave f_B = 175 Hz.
- **Radiation:** strongest at 140–230 Hz for the same force; the first two pressure peaks are
  T1 and A0 **[T]** [LeCarrou07]. Above 200 Hz modes overlap and cannot be told apart.
- Earlier studies **[T]** (quoted in [LeCarrou07]): a strung Spanish baroque harp had A0/T1-like
  pair at 112 and 146 Hz. An unstrung one harp maker Orchestra showed the pair by holography, and one of
  the two "weakens when the soundboard is stressed by the strings" (Bell 1987).
- **What the box cannot radiate:** "a dozen or so" of the lowest strings have fundamentals below
  what the soundbox radiates; they are heard by their overtones (missing fundamental) **[T]**
  [AT24].

### 3.4 The board above T1: modes that climb with the strings

- In an FEM of a bare trapezoidal board, the first mode moves the wide (bass) end, and each higher
  mode's region of greatest motion lies further up toward the treble **[T]** [AT24, Waltham08
  Fig. 3]. The figure's legend lists modes at about 213, 285 and 571 Hz (partly illegible in
  the copy).
- **Driving-point admittance along the string bar,** measured (one harp maker Aurora, 0.2 g accelerometer,
  impact hammer) **[T]** [Waltham08]. Plotted against position and frequency, the high-admittance
  regions move steadily up the board as frequency rises. The string "trajectory" (each string's
  f0 at its attachment) runs just to the right of the primary antinodes (above them in
  frequency), except past the fundamental antinode. Mode 4 near 500 Hz is split. The pressure at
  the lowest hole shows *double* peaks where the admittance shows one, near 500 and 700 Hz.
  Waltham reads these as T1/A0-like pairs.
- **Uneven strings:** "coincidences between string fundamental frequencies and the anti-nodes of
  the soundboard correlate with excessively loud strings; coincidences … with the nodes … with
  'dead' strings" **[T]** (Waltham & Kotlicki 2008 as quoted in [LeCarrou10]). In a good harp the
  string positions lie just above the anti-node of the lowest mode and below the primary
  anti-nodes of the higher modes **[T]** [LeCarrou10]. Board frequencies rise arithmetically
  (1, 2, 3, 4) and strings geometrically (1, 2, 4, 8), so the match cannot be kept everywhere.
  But a completely dead string is unlikely, because each string has many partials **[T]** [AT24].
- **Above about 1 kHz: not found.** No measured modal density or loss for the harp board. Use the
  general plate estimate. For a 1.4 m² … 0.35 m² board of 2–12 mm spruce the asymptotic modal
  density is n = A/(4π)·√(ρh/D). It is a few modes per 100 Hz in the bass part and fewer in the
  treble part **[E]**, with Q of 20–50 **[M]**.
- **Coupling strength** **[T]** [Wood22]:
  - The string-to-body impedance ratio (bridge admittance × Z0 of the string) of a small harp is
    comparable to a classical guitar's. A violin's is about 10 dB higher, a banjo's about 20 dB
    higher.
  - A concert harp (Elysian) had *lower* board admittance but higher-impedance strings, "so that
    the impedance ratio ends up in a similar range".
  - The board impedance is far above the string's transverse and longitudinal impedances at
    nearly all frequencies, so the string's motion is little changed by the board.
- **Admittance magnitude (estimate)** **[E]**. From the equivalent beam: modal mass about 0.8 kg
  at mode 1, ζ ≈ 0.02 **[M]**, giving a peak admittance of about 1/(2ζωm) ≈ 0.03 m/s/N at
  150 Hz. Between modes it is lower by 10–20 dB. No calibrated harp admittance was readable in
  the papers.

### 3.5 Radiation and directivity

- Measured with a shaker on the centre line and 35 microphones on an arch at 2.35 m, in a
  semi-anechoic room, 100 Hz–1 kHz **[T]** [LeCarrou10]:
  - **Below about 220 Hz:** non-directional, including at T1 (134 Hz) and A0 (148, 166 Hz) on
    this harp.
  - **Above 220 Hz:** lobes in both elevation and azimuth. At 250 and 384 Hz the radiation is
    mostly to the front; at 460–756 Hz front and back; at 688 Hz the back; at 730 and 892 Hz
    front, back and sides. "When a listener moves around the harp … the instrument is nearly
    silent in some locations."
  - A baffled piston of radius 20 cm (the moving part of the board) has its diffraction
    frequency c/2πa ≈ 270 Hz, close to that.
  - The patterns are symmetric about the strings' plane, because the drive is on the centre line.
- **Where the sound comes from** (equivalent monopoles in the symmetry plane) **[T]**
  [LeCarrou10]:
  - Between 100 and 550 Hz the source climbs from the bottom of the box to the top.
  - 250–375 Hz: centred on the 3rd hole, at the back.
  - From 375 Hz: near the 4th hole, plus an out-of-phase source at the 2nd (a dipole).
  - 550–750 Hz: the top of the board.
  - Above 750 Hz: back near the drive point.
- **Bell's earlier picture** (quoted) **[T]** [LeCarrou10]: non-directional to 400 Hz; strong to
  the front and back at 400–2000 Hz; more from the back at 2–4 kHz, with two areas on the front.
- **Directional tone colour:** the patterns change quickly with frequency, so each partial of a
  string goes out in its own direction **[T]** [LeCarrou10]. Waltham 2009 found two adjacent box
  resonances, one radiating mostly from the top and one from the bottom **[T]** [Waltham09].
- **Box knocking:** the soundbox can be tapped, knocked or slapped as a box drum **[T]** [AT24].

## 4. Pedals and discs

- **Seven pedals, three positions each.** Up = flat, middle = natural, down = sharp. With all
  pedals up the harp is in C♭ major (the A string plays A♭) **[T]** [AT24]. Left foot: D, C, B;
  right foot: E, F, G, A, from left to right **[T]** [AT24]. Each pedal moves every string of its
  letter in every octave **[L]** [TOR-intro].
- **The discs (fourchettes)** **[T]** [AT24, Camac]:
  - Two rows of discs below the bridge pins on the neck. Each disc is a rotating disc with two
    prongs that grab the string when turned. The upper (natural) disc shortens the string by a
    semitone; the lower (sharp) disc by another.
  - Moving a pedal from flat to natural turns all natural discs of that letter. From natural to
    sharp only the sharp discs turn (Érard's double action, patented 1810).
  - For one semitone, the length must shorten by 1 − 2^(−1/12) = 5.6 % of the sounding length,
    then by another 5.3 % **[E]**. On A2 (1.07 m) that is 60 mm and 57 mm; on C6 (0.18 m) about
    10 mm per step.
- **No disc on three strings:** the lowest C and D and the highest G. They are tuned by hand to
  whatever the music needs **[L]** [TOR-intro]. Camac's regulation lists discs from E43 to F0
  only **[T]** [Camac].
- **What the disc does to the string** **[T]** [Camac]:
  - The disc pinches the string between its prongs. If it grips "a bit too much" the note goes
    sharp; too little and it is flat or buzzes. Regulation consists of turning the disc on its
    spindle to set the grip.
  - From F7 to F0 (the top) the discs have one prong only, and "a very slight change in grip will
    have a great change on the intonation". New strings of a different gauge need re-regulation.
  - The string nut (top) is set so the string passes through the centre of the disc, "so the two
    prongs … touch the string simultaneously".
- **Buzzes** (maker's troubleshooting, the mechanisms) **[T]** [Camac]:
  - *Open-disc buzz:* in natural or sharp, the disc does not grip enough, and the string buzzes
    on one or both prongs as it swings.
  - *Action-plate buzz:* the disc grips but rattles in its bearing; its spindle springs are loose.
  - *Flat-position buzz:* the string touches the centre or a prong of a disc it passes, for
    example an old wire string that has lost stiffness, or a string off-centre.
  - *String-nut buzz* (flat position only).
  - *Knot buzz:* a knot lying against the board, or "sympathetic" buzzes from music stands.
- **Tone with the discs on: not measured.** The disc end is a pinch between two small metal
  prongs instead of the bridge pin. That is probably lossier and softer than the pin, so a
  natural or sharp may ring slightly shorter than a flat **[E]**; only player talk was found
  (section 9). The sounding length still ends at the board, so the plucking point's share and the
  board coupling barely change **[E]**.
- **Enharmonics:** the pitches A♯/B♭, B♯/C, C♯/D♭, D♯/E♭, E♯/F, F♯/G♭, G♯/A♭, C♭/B, F♭/E each
  exist on two strings. Only D, G and A have one string each **[S, L]** [VSL, TOR-intro].
  - Players use this for repeated notes and trills (two strings take turns), bisbigliando, and
    glissandi that sound only 4–7 pitches per octave, for example a diminished-seventh chord
    with every string tuned to one of four pitches **[L]** [VSL, TOR-scoring].
  - The same pitch on two strings of different length, gauge and attachment point is two
    slightly different tones **[E]**.
- **Pedal slide (pedal glissando):** moving a pedal while a string still sounds bends its pitch
  by a semitone, audibly. It is best in the lower register and on octaves, and both feet can
  slide at once **[L]** [TOR-scoring, TOR-intro]. At most two pedals change at once, one per
  foot **[L]** [VSL, TOR-intro].

## 5. Plucking

### 5.1 The pluck, measured

Ten harpists (six professional, four amateur), Camac Atlantide Prestige. A high-speed camera at
5167 frames/s tracked finger and string in 3-D; mezzo-forte chords (at 60 bpm) and arpeggios (at
120 bpm) with forefinger, ring finger or thumb **[T]** [Chadefaux12]:

- **Phases:** approach, *sticking* (finger and string move together), *slipping* (the string slides
  off the finger the other way), free oscillation.
  - Sticking lasts 56–431 ms (chords about 100–230 ms, arpeggios 150–320 ms).
  - Slipping lasts 1.2–6.1 ms (typically 2–4 ms).
  - A harpsichord plectrum takes about 150 ms and 8 ms; a guitar finger 84–170 ms and ≈ 0.5 ms.
- **Force:** up to 15 N (a plectrum about 1.5 N) **[T]** [Chadefaux12]. "Near 3 kg at the
  fingertip" **[T]** [LeCarrouRNRM]. 74 % of harpists surveyed report pain **[T]**
  [LeCarrouRNRM].
- **The string is rotated about its axis** by the finger during sticking: 7–82° at most, mean
  about 44°. Chords get less rotation (39°) than arpeggios (45°) **[T]** [Chadefaux12].
- **Release (initial conditions):**
  - Displacement 4 mm and velocity 2 m/s on average; extremes 0.5 mm with 0.2 m/s and 7 mm with
    5 m/s.
  - About 1 mJ of potential energy and 0.4 mJ kinetic. The kinetic share is 15–37 %, *not*
    zero, so a plucked harp string starts from a mixture of displacement and velocity.
  - Large displacement goes with large velocity. Each player sits in their own region of the
    (displacement, velocity) plane, and does so repeatably **[T]** [Chadefaux12].
- **Release direction:** the long axis of the first ellipse is 7–43° from the strings' plane,
  i.e. mostly *in* the plane, toward the neighbours. It is slightly larger for the forefinger
  than for the ring finger **[T]** [Chadefaux12]. The amplitude is limited only by the string
  spacing, "about a dozen millimetres" **[T]** [AT24]; on a small harp about 15 mm **[T]** [Wood22].
  A harpist "could easily use an initial displacement of 10 mm or more for a louder note"
  **[T]** [Wood22].
- **Where:**
  - Measured: between about one-third and two-fifths of the length from the board. The forefinger,
    ring finger and thumb differ by under 1 % of the length, and the point rises slightly with
    pitch (for reach) **[T]** [Chadefaux12, Chadefaux13].
  - "Between a third and a half the way up from the soundboard" **[T]** [AT24]. "Plucked in the
    middle" **[T, L]** [Waltham05, VSL]. "Close to their centres with the fleshy part" **[T]**
    [AT24].
  - Model **[E]**: 0.33–0.45 of the length from the board; 0.40 by default.
- **Finger width:** about 2 cm in contact. As a low-pass on the excitation, f_c = c/(2D) =
  9760 Hz (C♯6), 8593 (C♯5), 7512 (C♯4), 6757 Hz (C♯3), i.e. above harmonics 8, 15, 27 and 52.
  So the pluck is effectively a point **[T]** [Chadefaux12 Table V].

### 5.2 A model of the finger, fitted

**[T]** [Chadefaux13]:
- **Sticking:** the finger is a Kelvin–Voigt pad (spring k_f and dashpot c_f), rest thickness
  l_f ≈ 1 cm (forefinger). The string is a quasi-static spring
  k_s = T (1/(y0 − Δl/2) + 1/(L − y0 − Δl/2)).
  - The best fit is a linear stiffness and an exponential damping:
    k_f = k_fa·δu and c_f = c_fa·e^(c_fb·δu), with δu the indentation.
  - Fitted values: k_fa = 294–1785 N/m² (harpist), c_fa = 351–1867 N·s/m, c_fb = 6–216 m⁻¹.
    The silicone robot finger: k_fa 311–350, c_fa 675–1093, c_fb 69–90.
- **Slipping** starts when the tangential force reaches F_max (1–10 N, reference 5 N). The string
  then slides a distance d_s (0.1–2 mm, reference 1 mm) along a direction γ the player chooses,
  under Coulomb friction μ = 0.87–1.00 (fitted 0.99). The normal force is always over 1 N.
- **Effects:**
  - F_max sets the release displacement, 0.03–7.4 mm. d_s and μ set the release velocity,
    0.4–2.7 m/s.
  - Over those ranges the spectral centroid of a D♭3 string moves by 9 Hz (about 6 % of f0) and
    the spectral spread by up to 9×. Velocity alone moves the centroid by 3 % and the spread by
    1.7×.
  - Different players on the same harp sound different mainly through how they release the
    string.

### 5.3 The excitation spectrum

- A point pluck at a share β of the length gives partial n an initial displacement amplitude
  ∝ sin(nπβ)/n² **[M]** (the A_n of [Chadefaux12 eq. 2]). The velocity part adds
  B_n ∝ sin(nπβ)/n³ times V/c **[T]** [Chadefaux12 eq. 3].
  - β = 1/2: the even partials vanish. β = 1/3: every third. β = 0.4: partial 5 vanishes and
    2, 3 are weak.
  - The harp's mellow sound: "a spectrum that is stronger in the fundamental and a few low
    harmonics compared with … a guitar string plucked close to the bridge" **[T]** [AT24].
- **But the nonlinearity fills the gaps** (§5.6). On a measured mid-point pluck the second
  partial was as strong as the 1st and 3rd. With a strong finger pluck "the first few strong
  frequency components all show up with rather similar amplitudes" **[T]** [Wood22].
- **Pad and nail:**
  - Normal playing is the fleshy part of the finger; there is no "nail then flesh" as on the
    guitar **[T]** [AT24, Chadefaux12].
  - Some players sound as if plucking with the fingertip, others "close to their fingernail",
    and brightness varies from player to player **[T]** [Chadefaux12].
  - Plucking with the nails ("près des ongles") is an occasional effect for "a harder spectrum"
    **[T]** [AT24]. It was medieval and Renaissance practice and is a 20th-century effect
    **[S]** [VSL]. Fingernail glissandi work only from ppp to mf and only descending **[L]**
    [TOR-scoring].
  - In the model: a narrow, hard contact with a quick slip (≈ 0.3–0.5 ms instead of 2–4 ms)
    **[E]**.

### 5.4 Près de la table, harmonics, and other effects

- **Près de la table (p.d.l.t.):** plucking close to the board "to give a more extended spectrum"
  **[T]** [AT24]: brighter, more metallic, guitar-like, nasal, brittle **[S, L]** [VSL,
  TOR-scoring]. "In some music … close to the soundboard in order to imitate a guitar" **[T]**
  [Waltham05].
  - Playable at f and softer **[L]** [TOR-scoring].
  - Model **[E]**: β = 0.05–0.12 (at C4, 3–7 cm from the eyelet); the finger's 2 cm width then
    matters more.
- **Harmonics (flageolet)** **[T]** [AT24]: touch the midpoint of the string with the base of the
  thumb and pluck with the thumb; "the sound … is ethereal".
  - The left hand stops with the side of the palm and plucks with the thumb. The right hand
    stops with the knuckle of the 2nd finger and plucks with the thumb **[M]**.
  - It sounds an octave above the written note **[L]** [TOR-scoring]. The octave (partial 2) is
    usual; up to partial 5 is possible **[S]** [VSL].
  - Mf and softer, and only on pitches within the treble and bass staves. "A harmonic at f won't
    sound much louder than one at p." The left hand can play two at once if they are a fourth
    or less apart **[L]** [TOR-scoring].
  - Model **[E]**: a damping contact at L/2 (a hand, not a point), held through the pluck and
    lifted a few ms after release. That removes the odd partials, partial 1 included, and lets
    the even ones ring.
- **Xylophonic sounds (sons xylophoniques)** **[M]**: one hand presses the string lightly near
  the board while the other plucks. The result is a dry, short, pitched click. Not found in the
  texts read (section 9). Model **[E]**: a heavy damping contact near the eyelet with a pluck.
- **Étouffé (sons étouffés), staccato:** pluck and damp at once, with the same finger or the flat
  of the hand. It works only at moderate tempos **[L, S]** [TOR-scoring, VSL]. "Étouffez" means
  damp all strings **[L]** [TOR-scoring].
- **Other effects** **[T, S, L]**:
  - Knocking or slapping the box **[T]** [AT24]; striking with a stick, plectrum, secco, vibrato,
    trills, tremolo **[S]** [VSL].
  - Thunder glissando (a bass-wire gliss with the nails or flat hand) and scraping the strings
    with a coin **[S]** (Hindemith sonata, [bakerartist]).
  - Salzedo catalogued "Aeolian flux" (glissando) through "Xylophonic sounds" and others; his
    *Modern Study of the Harp* was not read.
  - Bray pins: historic harps had them (adjustable pins at the string feet that buzz against the
    string) "to boost the sound volume" **[T]** [AT24]. Not on the concert harp, but the same
    contact model as a disc buzz.

### 5.5 Glissando and bisbigliando

- **Glissando:** the fingertips sweep adjacent strings. Every string sounds, so the pedals set the
  scale, 4–7 pitches per octave **[S, L]** [VSL, TOR-scoring].
  - Up: usually the 2nd finger (or 3rd), leading with the pad. Down: the thumb **[M]**. Two-note
    glissandi in thirds or fourths in one hand; up to four notes with two hands; these are not
    louder than forte **[L]** [TOR-scoring].
  - The loudest sound the harp can make is a glissando **[T]** [AT24].
  - Speed (not measured): a 6½-octave gliss in about 1–2 s, i.e. 25–45 strings per second
    **[M]**, so 22–40 ms between strings **[E]**.
  - Each string is plucked by a finger that does not stop: a short stick, slip, and the next.
    The strings plucked before keep ringing.
- **Bisbigliando ("whispering"):** a rapid, soft tremolo, usually between enharmonic strings or
  between the hands on one or more chord notes. It is played piano: "delicate, crackly". Loud
  and fast, it would produce "an ugly buzz with every new pluck" as the finger lands on a moving
  string **[S, L]** [VSL, TOR-scoring, OrchOnline].
  - Model **[E]**: a finger landing on a sounding string first stops it, then plucks (as `Bass`
    does). The "crackle" is that landing.

### 5.6 The string's nonlinearity

**[T]** [Wood22], measured on a small harp:
- **Phantom partials:**
  - A string at an angle to the board (35°) drives the board with its *axial* force as well as
    its transverse force. The geometric nonlinearity (the tension changing as the string
    stretches) makes an axial force at sums and differences of the transverse partials: at 2× each
    partial, and at the sums of neighbours.
  - These "phantom partials" are as strong as the linear partials. A mid-point pluck in the plane
    of the strings and one perpendicular to it give equal even "harmonics", while the odd ones
    differ by 10–15 dB.
  - The perpendicular pluck sounds "jangly". The first longitudinal resonances (3.3, 6.6 kHz on
    that string) show as broad bumps.
  - The pattern fits a string end *clamped* at the board, not pinned. The dominant nonlinearity
    is quadratic, so it grows with pluck strength: a harpist can change the tone by force alone,
    with the plucking point fixed.
- **Pitch glide:** over the first ≈ 100 ms each partial falls by "a little over 2 %" after a
  strong finger pluck (small-harp nylon E4). That is above the threshold of perception, about
  0.1 ERB ≈ 2 % at 327 Hz **[T]** [Wood22].
  - Estimate for concert gut **[E]**: Δf/f ≈ ½ (EA/T)(a/L)². With a = 5 mm: C4 +0.2 % (4 cents);
    A4 +0.3 %. With a = 10 mm: C4 +0.9 % (16 cents).
  - Gut has a lower E than nylon in use, so the glide is smaller than on nylon **[T]** [Wood22].
- **Linear coupling** of transverse and longitudinal motion through the angle changes little.
  Its main effect is cos θ on how strongly the board is driven **[T]** [Wood22].

## 6. Decay, polarizations, sympathetic strings

### 6.1 Decay measured

- **One string, measured** (string 31 = C♭3 = B2, 123.6 Hz, Camac). An accelerometer on the board
  recorded 8 s at 4096 Hz, and ESPRIT high-resolution analysis was applied to the 4–8 s part,
  first partial only **[T]** [LeCarrou09 Table II]. Damping factor δ is the amplitude ∝ e^(δt);
  T60 = 6.91/|δ| **[E]**:

| Component | f (Hz) | δ (1/s) | T60 **[E]** |
|---|---|---|---|
| the string, polarization coupled to the board | 123.59 | −0.68 | 10 s |
| the string, other polarization | 123.78 | −0.15 ± 0.12 | ≈ 46 s (uncertain) |
| sympathetic mode with string 42 (F♭1 = E1, 41.1 Hz, its 3rd partial) | 123.34 | −0.15 … −0.10 | 46–69 s |
| sympathetic mode with string 38 (C♭2 = B1, 61.5 Hz, its 2nd partial) | 123.08 | −0.26 … −0.21 | 27–33 s |

  The two polarizations of one string are 0.19 Hz apart (1.5 × 10⁻³). With all other strings
  damped by paper, the string's two components were 123.54 Hz (δ −0.57) and 123.71 Hz (−0.54).
- **Lore:**
  - "Low notes ring for four or five seconds; high notes die away quickly"; low notes "need about
    half a second to fully develop" **[S]** [VSL].
  - "Lower strings sustain longer; higher strings are brighter and more percussive"; low passages
    "can become muddy", like a piano with the sustain pedal held **[L]** [TOR-intro].
  - A book quoted on a harp forum gives 25 s for a low F, doubted by the poster **[S]**
    [HCdecay].
- **Model T60, fundamental** **[E]**, from the string-alone bound (§2.4), lowered for the board
  so that B2 lands at the measured 10 s:

| Register | T60 |
|---|---|
| G7–C7 | 0.5–1 s |
| C6 | 2–3 s |
| A4 | 5–6 s |
| C4 | 7–9 s |
| C3 | 10–12 s |
| A2–G2 | 10–15 s |
| wires C2–C1 | 10–20 s **[M]** |

  Steel and copper lose less internally, so the wire strings are limited by air and board.
  Partials decay faster: about T60(f) ∝ 1/f by the air term alone, and much faster above the
  ~10th partial (η_bend ∝ j²).

### 6.2 Two polarizations

- The polarization in the plane of the strings drives the board (normal force through the
  string's angle) and decays fast. The one across the plane (parallel to the board's face)
  barely couples and rings long **[T]** [LeCarrou09, Wood22]. In-plane plucks give board partials
  10–15 dB stronger than perpendicular ones at the odd harmonics **[T]** [Wood22].
- **Ellipses:** "The long strings vibrate in collapsing and expanding ellipses, which gives many
  strings a long pulsed tail to the sound." The large polygon at time zero is the attack
  **[T]** [Waltham05]; a 65 Hz strobe movie of C2.
- The release direction (7–43° from the plane, §5.1) sets the mix: mostly in-plane, so loud
  first, with a quieter long tail from the perpendicular part **[E]**.
- **Beating** **[E]**: the two components of the measured B2 string are 0.19 Hz apart, and a
  sympathetic mode 0.25–0.5 Hz away. Together they give slow beats of 2–5 s period in the tail.
  A plausible split for the model is 0.05–0.3 % per partial, each partial its own (as in
  `bass.h`, up to 0.3 %).

### 6.3 Sympathetic strings: the halo

- "When one string is plucked, a number of normal modes involving the coupled motions of other
  strings are excited which induce a characteristic 'halo of sound'." Makers must keep it
  "reasonable"; musicians and makers are "annoyed by" it **[T]** [LeCarrou09].
- **Mechanism:** the modes of the string–board system include modes in which the plucked string's
  partial and a lower string's matching partial move together, with little board motion. Those
  are string–string modes; their kinetic energy on the beam is near zero. A waveguide model of
  35 strings on an equivalent beam predicts them within 0.04–0.11 % **[T]** [LeCarrou09].
- **Which strings:** those with "overlapping overtones", i.e. octaves, fifths and fourths below;
  "many strings vibrate when only one is plucked" **[T]** [Waltham05, AT24]. On Waltham's lever
  harp, plucking A2 strongly excited C4, "an octave and a minor third away (about 7/3)".
- **Stopping the string does not stop the sound:** the end amplitude stays of the same order
  unless the sympathetic strings are stopped too. Harpists know this **[T]** [LeCarrou09].
- The halo is "particularly noticeable as a harp is being tuned" **[T]** [AT24].

### 6.4 Damping by the hands

- Damping is done "by pressing the strings with fingers or an open hand" **[L]** [TOR-intro]. Low
  notes are damped when time allows, when the score requires it, and at the end of a piece; high
  notes need no damping **[S]** [VSL]. "There are many techniques" **[T]** [AT24]: placing the
  flat of the palm on a group of bass strings, or replacing a finger on the string just played
  **[M]**.
- **How fast a hand stops a string: not measured** (section 9). A palm presses on a long stretch
  of string, so every mode is damped, the low ones within about 50–150 ms **[M]**. Re-placing a
  finger stops the string in a few periods.

## 7. Level and dynamics

- **Sound power,** measured (anechoic, 32-microphone sphere, ISO 3745, F-time-weighted for the
  plucked transient; single notes as soft and as loud as possible over the whole range) **[T]**
  [Weinzierl18 Table I]:

| Instrument | L_W ff max | L_W pp min | range |
|---|---|---|---|
| Harp | 91 dB | 54 dB | 37 dB |
| Guitar | 88 | 59 | 29 |
| Violin (modern) | 95 | 52 | 43 |
| Cello (modern) | 97 | 63 | 34 |

  (dB re 1 pW; pitch-weighted averages were not given for the harp.)
- **At the player's ear:** harpists measured at 89.2/87.1 dB LAeq (left/right) over 65 min of
  20th-century repertoire **[S]** [Zera23].
- **In the orchestra:**
  - "Like any plucked string instrument, the harp is quiet compared with bowed instruments. The
    loudest sound it can make is a glissando … otherwise, to give the harp prominence, the rest
    of the orchestra must quiet down" **[T]** [AT24]. "One of the quiet instruments"; Strauss: it
    "must always be treated as a solo instrument" **[S]** [VSL].
  - With a trombone, the trombone plays two or three dynamic steps lower **[S]** [VSL].
  - The low strings often carry the bass line or replace bells; arpeggios make harmonic
    backdrops, and glissandi add shimmer **[S]** [VSL].
  - Chords of up to eight notes; thick, doubled voicings sound more resonant **[L]**
    [TOR-scoring].
- **Velocity to pluck (model)** **[E]**:
  - The release displacement runs about 0.5 mm (pp) to 7–10 mm (ff), about 23 dB of string
    amplitude.
  - With the release velocity (0.2–5 m/s) and pluck point, the rest of the measured 37 dB comes
    from the spectrum. Louder plucks are brighter through the nonlinearity (§5.6), and more
    phantom partials and glide make the tone change with force.
  - Make the release displacement the main control, roughly D_tr ≈ 0.5 mm × 18^(v/127)
    (0.5 mm at velocity 0, 9 mm at 127).

## 8. What a model needs, and what this codebase already has

### 8.1 The parts

1. **47 strings, each a modal set in two polarizations,** at n f0 √(1 + B n²):
   - B from §2.2–2.3; about 10–12 useful partials on gut and nylon, more on the wires (their B
     is low and their f0 too, so 40–60 partials up to 4 kHz **[E]**).
   - Per-mode loss from air + η_E bending (§2.4), plus what the board takes. The out-of-plane
     polarization barely couples.
   - The two polarizations a hair apart (0.05–0.3 %). Gut aliveness: each partial a few cents off
     and ±25 % in damping (as the double bass's gut).
   - Each string ends at its own point on the board's centre line, at 35° to the board.
2. **The board as modes with a share at each of 47 attachment points,** not one bridge:
   - The low pair T1/A0 (≈ 150/170 Hz) with the air: a 2-degree-of-freedom model (board plus air
     piston) [LeCarrou07]. Then body modes (60, 85, 125 Hz, barely radiating). Then
     board-bending modes climbing the board: antinodes moving from the bass end to the treble as
     frequency rises [Waltham08, AT24].
   - Above ~1 kHz, a dense bank. The string impedance ratio is guitar-like.
   - The strings' axial stiffness raises the board's modes when strung.
3. **The air in the box:** five pistons in the back, in phase below 250 Hz (one Helmholtz-like
   mode), with the bottom holes moving most; above that, standing waves along the box
   [LeCarrou07].
4. **The sound:** monopole-like below ~220 Hz. Above, each partial with its own front/back/side
   lobe. The source climbs the box with frequency (100–550 Hz), then the top of the board
   (550–750 Hz) [LeCarrou10]. Two microphones in front (audience) and, optionally, behind (the
   holes).
5. **The pluck** (§5.1–5.2):
   - A finger pad (Kelvin–Voigt, 1 cm thick, 2 cm wide) grabs the string 0.33–0.45 of the way
     up and pulls it 0.5–10 mm, mostly in the plane of the strings (7–43° off).
   - It holds for 50–400 ms (the score decides), then lets it slip in 1–6 ms with μ ≈ 0.95, so
     the string leaves with both displacement and velocity.
   - The rotation of the string (up to 80°) has no known acoustic effect; leave it out at first.
   - Variants: thumb (larger pad, more depth), nail (narrow, fast slip), p.d.l.t. (β ≈ 0.08),
     harmonic (hand at L/2), xylophonic (hand near the eyelet).
6. **The nonlinearity:** tension modulation from the strings' stretch. It gives the pitch glide
   and drives the board through the axial force (×sin 35°) with the squared slope. That is
   quadratic, and it fills in the gaps of the mid-string pluck [Wood22]. It costs one sum per
   string per sample: the mean of Σ (n π/L)² q_n² over modes, times EA/(2L) **[E]**. It matters
   most on hard plucks and nylon.
7. **Discs:**
   - Three lengths per string (flat, natural, sharp); pedal moves change the length of every
     string of that letter, while sounding (pedal slide).
   - A disc that grips too little buzzes: a one-sided contact on the prongs near the top end.
   - The three disc-less strings (C1, D1, G7) are tuned by hand.
8. **Hands:** damping with the palm (many strings at once, a long contact), a finger replaced on
   a string, étouffé. No string is damped unless the player does it, so all 47 ring
   sympathetically.
9. **Glissando:** the same finger dragged across strings at 25–45 strings/s, each string a short
   stick-slip, with a pedal setting that chooses the scale.

### 8.2 What can be reused

- **`src/piano.cc` / `piano.h`: the closest whole design.**
  - Each string is a bank of modes in two directions at n f0 √(1 + B n²).
  - The soundboard is a bank of resonances (`PianoDesign::Board`: freq, loss, and `phi[keys][n]`,
    each mode's share at every key's bridge point; made by `tools/pianoboard.py`). For the harp,
    each string's attachment point on the centre line takes that role directly. The rising
    antinodes are just a board whose `phi` per string varies with position.
  - Dampers that take energy by the mode shape at a point (`damper`, `damperLen`, `damperRate`)
    serve for the hands, harmonics and xylophonic contacts.
  - The undamped strings (`hasDamper = false`) already ring with every other string through the
    board: the halo comes for free.
  - The output is board patches heard by microphones with angle and distance, for the
    directivity.
  - What the harp adds: a plucking finger instead of a hammer, 47 single strings, the
    35° string angle, and the axial (phantom) force.
- **`src/bass.h` / `bass.cc` (`Bass`) and `bassinst.cc` (`BassPlayer`, `pluckPlace`):**
  - A finger pad that pulls the string aside, holds and slips off, mostly sideways.
  - A finger landing on a sounding string stops it first (bisbigliando, repeated notes).
  - Two polarizations with per-overtone splits (up to 0.3 %), a slowly turning plane, and gut
    non-uniformity (±25 % damping, a few cents).
  - Tension glide (pitch starts sharp).
  - A stopped length (the disc is a stop that does not move: `pluck(s, stoppedLength, …)`).
  - The `BassBody` grid (plates of masses, a column of air cells with holes opening from chosen
    cells) is shaped for a violin-family body. A long trapezoidal board over a semi-conical box
    with five back holes in a column is the same kind of object, but needs its own design
    (`tools/bassbody.py`-like). The modal-bank route of the piano is cheaper for 47 attachment
    points.
- **`src/guitar.h` / `guitarinst.cc`:**
  - Flesh-then-nail release, rest and free strokes (the two polarizations' two-stage decay), and
    damping by a hand or lifting finger.
  - The guitar's air-and-top pair (≈ 100/200 Hz) is the same 2-degree-of-freedom idea as the
    harp's T1/A0, with the order reversed.
- **`src/modal.h` (`ModalSet`, today's koto):**
  - Strings as damped modal oscillators in two directions, a body under a bridge, and a pluck
    that rises and lets go.
  - The jawari/sawari *contact* (one-sided Hertz-like spring near the end) fits a disc that
    grips too little, or a bray pin.
  - The *quadratic coupling* (`Def::Quad`) and *tension glide* are the right forms for the axial
    force and the glide.
  - Its single bridge point is what does not fit: the harp needs a share per string. The quickest
    stopgap is a harp entry in `ModalModel` with 47 strings and a body whose modes fade from bass
    to treble by string index.
- **`src/bodybank.h`:** body resonances drawn at a set density above where the computed body
  reaches, each with its own direction. That fits the harp board above ~1 kHz, where no
  measurements were found.
- **`src/membrane.h`:** nothing for the strings. Its `DrumAir` (a cavity as a spring between the
  modes that change its volume) is the concept for coupling the board's breathing modes to the
  box air (T1/A0). The `Surface` interface (`shape()`, `force()`, `q()`, `v()`: anything touching
  anywhere) is the right interface for a harp board that 47 strings and two hands touch at
  different points.
- **`src/room.h`:** the hall.

## 9. Not found, or uncertain

- **Per-string lengths and core diameters of the wire strings** (G2–C1) for any concert harp,
  and so their inharmonicity. Only overall gauges (Camac) and tensions (Galli, a retailer page)
  were found.
- **A complete string table of a one harp maker or one harp maker 47** (lengths, gauges, tensions); the
  Harp Connection PDF has no numbers, only the stringing types.
- **Calibrated soundboard admittance** of a concert harp in m/s/N. The figures of [LeCarrou07],
  [Waltham08] and [Wood22] give shapes, not readable values. The damping of the six low modes is
  only in [LeCarrou07] Fig. 4. Waltham & Kotlicki's JASA 2008 paper and Daltrop's thesis
  (Aoyama Amphion) were not read; only the 2-page conference version was.
- **Board modes and losses above ~1 kHz.**
- **Measured T60 per register.** Only one string (B2, first partial, 4–8 s after the pluck) was
  found measured. The VSL "4–5 s" and the forum's "25 s" are lore.
- **How the disc changes the tone** (loss at the prongs, the sharp versus flat "ring"): only
  regulation and buzz mechanisms were found. The physics of a disc buzz (amplitude, contact) was
  not found.
- **Xylophonic sounds, Salzedo's effects in his own words** (*Modern Study of the Harp*), and how
  each is played. Described here from general knowledge.
- **Glissando speed and level;** the harp's sound power for a glissando, said to be its loudest.
- **How fast a palm damps a string.**
- **Directivity above 1 kHz** (Bell 1987 is only quoted).
- **Le Carrou's thesis** (2006) and **Firth 1977, Bell 1987/1997** were not opened.
- The top note "G♯7" (VSL) against the disc-less top G (Camac, TOR); maybe some makers fit a
  disc there.
- **Woodhouse's measurements are on a small nylon lever harp.** He states that concert-harp
  impedance ratios are "broadly comparable", but the phantom-partial strength on gut concert
  strings was not measured.

---

## 10. References

- **[AT24]** C. Waltham, "Origins and Acoustics of the Modern Pedal Harp", *Acoustics Today*
  20(2), 59–66 (2024). Read in full.
  https://acousticstoday.org/wp-content/uploads/2024/07/AT-8-Origins_featured_summer2024.pdf
- **[LeCarrou07]** J.-L. Le Carrou, F. Gautier, E. Foltête, "Experimental study of A0 and T1
  modes of the concert harp", JASA 121(1), 559–567 (2007). Read in full.
  https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A2_LeCarrou_IdentificationA0T1modes.pdf
- **[LeCarrou09]** J.-L. Le Carrou, F. Gautier, R. Badeau, "Sympathetic string modes in the
  concert harp", Acta Acustica united with Acustica 95, 744–752 (2009). Read in full.
  https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A3_LeCarrou_SympatheticStringModes.pdf
- **[LeCarrou10]** J.-L. Le Carrou, Q. Leclère, F. Gautier, "Some characteristics of the concert
  harp's acoustic radiation", JASA 127(5), 3203–3211 (2010). Read sections I–IV.
  https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A4_LeCarrou_HarpAcousticRadiation.pdf
- **[Chadefaux12]** D. Chadefaux, J.-L. Le Carrou, B. Fabre, L. Daudet, "Experimentally based
  description of harp plucking", JASA 131(1), 844–855 (2012). Read in full.
  https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A5_Chadefaux_HarpPluckingDescription.pdf
- **[Chadefaux13]** D. Chadefaux, J.-L. Le Carrou, B. Fabre, "A model of harp plucking", JASA
  133(4), 2444–2455 (2013). Read in the parts cited.
  https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A6_Chadefaux_HarpPluckingModeling.pdf
- **[LeCarrouRNRM]** J.-L. Le Carrou, D. Chadefaux, C. Pothrat, "Prévention des fatigues
  musculaires … cas du cordage de la harpe de concert", RNRM 2020 abstract (French).
  https://recenmus.sciencesconf.org/335316/RNRM2020_JL_LECARROU_V5.pdf
- **[Wood22]** J. Woodhouse, "The acoustics of a plucked harp string", J. Sound Vib. (accepted
  version, 2022). Read in full. https://www.repository.cam.ac.uk/handle/1810/353683
- **[WLA19]** J. Woodhouse, N. Lynch-Aird, "Choosing strings for plucked musical instruments",
  Acta Acustica united with Acustica 105, 516–529 (2019). Read §2 and §4.3.
  https://www.repository.cam.ac.uk/handle/1810/294170
- **[WLA21]** J. Woodhouse, N. Lynch-Aird, "Why do harpists still prefer gut strings?" (ASA lay
  paper 2pMU3, 2021). https://acoustics.org/2pmu3-why-do-harpists-still-prefer-gut-strings-jim-woodhouse
- **[Waltham08]** C. Waltham, A. Kotlicki et al., "Vibrational characteristics of harp
  soundboards", Canadian Acoustics 36(4), 52–53 (2008) (conference version). Read in full.
  https://jcaa.caa-aca.ca/index.php/jcaa/article/download/2108/1855/2245
  The JASA paper (124, 1774–1780, 2008, doi 10.1121/1.2956479) was not read.
- **[WaltKot08]** the JASA 2008 paper above, known only as quoted in [LeCarrou10] and [Wood22].
- **[Waltham05]** C. Waltham, "Harp Design and Construction", ASA lay paper 5aMU8 (2005).
  https://acoustics.org/pressroom/httpdocs/149th/waltham.html
- **[Waltham09]** C. Waltham, S. Daltrop, A. Kotlicki, "What makes a good harp?", ASA lay paper
  1pMU1 (2009). https://acoustics.org/pressroom/httpdocs/157th/waltham.html
- **[Weinzierl18]** S. Weinzierl, S. Lepa, F. Schultz, E. Detzner, H. von Coler, G. Behler,
  "Sound power and timbre as cues for the dynamic strength of orchestral instruments", JASA
  144(3), 1347–1355 (2018). Table I read.
  https://depositonce.tu-berlin.de/handle/11303/9646
- **[Camac]** Camac Harps, *Pedal harp technical manual* (English text J. Pratt). Read the parts
  on strings, discs and buzzes.
  https://www.camac-harps.com/wp-content/uploads/2016/10/Pedal-harp-technical-manual-.pdf
- **[CamacChart]** Camac Harps, *String chart for pedal harps, gauges 2022*.
  https://www.camac-harps.com/wp-content/uploads/2022/11/cordage-pedal-eng-2022.pdf
- **[HCchart]** The Harp Connection, *Lyon Healy Chicago 47 standard stringing*.
  https://www.harpconnection.com/PDF%20String%20Charts/String%20Chart%20Pedal%20Chicago%2047.pdf
- **[Galli]** Galli Strings, pedal harp wires (tension tables), via web summary.
  https://gallistrings.com/en/strings/harp/harp-concert-silver ;
  https://www.gallistrings.com/en/strings/harp/harp-concert
- **[VSL]** Vienna Symphonic Library Academy, "Harp", via web summary.
  https://www.vsl.co.at/academy/strings/harp
- **[TOR-intro]**, **[TOR-scoring]** Timbre and Orchestration Resource, "Extreme orchestration:
  harp", introduction and scoring pages, via web summary.
  https://timbreandorchestration.org/isfee/extreme-orchestration/harp/introduction ;
  https://timbreandorchestration.org/isfee/extreme-orchestration/harp/scoring
- **[OrchOnline]** Orchestration Online, "Bisbigliando is vewy vewy quiet" (snippet only).
  https://orchestrationonline.com/orchestration-tip-bisbigliando-is-vewy-vewy-quiet
- **[Grinnell]** Grinnell College Musical Instrument Collection, pedal harp (search snippet only).
  https://omeka-s.grinnell.edu/s/MusicalInstruments/item/2134
- **[HCdecay]** Harp Column forum, "Harp decay times" (snippet only).
  https://harpcolumn.com/forums/topic/harp-decay-times/
- **[Zera23]** Zera, Gołuch, Pietrzak, sound exposure of orchestral musicians, 2023 (snippet only).
  https://yadda.icm.edu.pl/baztech/element/bwmeta1.element.baztech-ac517f44-5745-4a4e-b96d-fc441dae3a56/c/Zera_Goluch_Pietrzak_Musical_3_2023.pdf
- **[bakerartist]** composer's work page on Hindemith's harp sonata (snippet only).
  https://bakerartist.org/node/17989
- **[FR]** N. H. Fletcher, T. D. Rossing, *The Physics of Musical Instruments*, 2nd ed., Springer
  1998, §2.13 (air damping of strings, eq. 2.40).
  https://csclub.uwaterloo.ca/~pbarfuss/The_Physics_of_Musical_Instruments.pdf
