# A viola, derived from the cello: research

Status: research only (2026-11-18). Nothing here is implemented. The viola is to be built from the
cello (`research/cello.md`), which is built on the bowed double bass (`src/bass.*`,
`src/bassbody.*`, `Bass::bow`, `BowPlayer`). The violin comes later on the same code. So wherever
the sources allow, this note puts the violin, viola, cello and bass side by side.

How each number is marked (as in `cello.md`):
- **[T]** read in the primary text. **(wf)** after it: read through a summarising web fetch, not
  the raw text.
- **[S]** from a secondary source, an abstract, a forum or a search snippet.
- **[E]** my estimate or arithmetic from cited values; not a published value.
- **[M]** from general knowledge, not looked up: check before relying on it.

References are short keys in brackets; the list with URLs is section 9.

**Short answer.** The viola is the member of the family furthest from scaling. It is tuned a fifth
below the violin (x1.50), but its body is only x1.17 as long and its resonances lie only x1.33
lower **[T]** [FR91 Table 10.1]. A fully scaled viola would have a 53 cm body. Its strings carry
violin-like tensions (47–80 N), so the low strings are heavy: the C string's wave speed is 98 m/s,
close to the cello C's 90 m/s, on a body barely larger than a violin's. Its air and main body
resonances fall *between* the open strings rather than on them:
- A0 (208–244 Hz) is 2–4 semitones above the open G;
- B1− (340–385 Hz) is between D4 and A4;
- B1+ (440–455 Hz) is at A4.

The lowest fifth (C3–G3) has no body support. That is the viola's veiled, slightly nasal,
"hollow" colour. From the cello, the modes move up x2.2–2.3 while the tuning moves x2, so against
the strings they sit about 2–4 semitones higher. The viola is less directional than the violin
(omnidirectional up to the 630 Hz band, not 1 kHz) and 2–3 dB weaker as a section.

---

## 1. The instrument

### 1.1 Strings

- **Tuning:** C3 130.81 Hz, G3 196.00 Hz, D4 293.66 Hz, A4 440.00 Hz, in fifths. A fifth below the
  violin, an octave above the cello **[T]** [FR91].
- **Sounding length:**
  - Guettler's reference length: 37.5 cm **[T]** [KnutStr]. Fletcher & Rossing: 37–38 cm **[T]**
    [FR91 Table 10.1].
  - Larsen's reference length: 370 mm, with "Extra Long" sets at 420 mm **[T]** [Larsen].
  - A luthier's table: 37 / 38 / 38–39 cm for small, medium and large violas (bodies 39 / 41 /
    42–43 cm) **[T, wf]** [Johannsson].
  - Pirastro's strings are rated for 37 cm; small "ergonomic" 15.75-inch violas use 35.6 cm
    **[S]** [Heath].
  - D'Addario's size classes by body length: XS 13–14 in, S 14–15, M 15–16, L 16–17, XL 17 and
    over **[T]** [DAdd].
  - Model **[E]**: 35.5 cm for a 15-inch body, 37 cm for 16 inches, 38–39 cm for 16.5–17 inches.
    One viola, unlike the violin and the cello, has no standard size: the size is a parameter.
- **Range for the model:** C3 (MIDI 48) to about E6 (88) in orchestral writing, A6–C7 in solo
  writing **[M]**. GM program 42 counted from 1 (41 from 0) **[M]**. This repository's GM table
  (`src/midifile.cc` around line 192) currently maps "viola" to the fiddle.

**Ranges over brands** (L = 37.5 cm, a = 440; Guettler, data from D'Addario and Corelli) **[T]**
[KnutStr]:

| | A4 | D4 | G3 | C3 |
|---|---|---|---|---|
| μ (g/m) | 0.56–0.92 | 0.98–1.25 | 2.20–2.81 | 4.95–6.31 |
| Wave speed 2Lf (m/s) | 330 | 220.3 | 147.0 | 98.1 |
| Z = √(Tμ) (kg/s) | 0.18–0.30 | 0.22–0.28 | 0.32–0.41 | 0.49–0.62 |
| Tension T (N) | 60.6–100.2 | 47.6–60.7 | 47.6–60.7 | 47.6–60.7 |

The four tensions sum to 203–282 N. The violin's sum to 189–265 N and the cello's to 493–601 N.

**Makers' medium sets.** The kg values are makers' figures compiled on a forum ("not guaranteed")
**[S]** [Heath]. Larsen's are confirmed in the maker's own PDF **[T]** [Larsen], Dominant's D, G
and C by a retailer **[S]** [L&M]. μ = T/(2Lf)² and Z = T/(2Lf) are computed at L = 0.37 m **[E]**:

| Set (medium) | T A / D / G / C (N) | μ A / D / G / C (g/m) | Z A / D / G / C (kg/s) | ΣT (N) |
|---|---|---|---|---|
| Dominant (nylon) | 59 / 49 / 47 / 48 | 0.56 / 1.04 / 2.24 / 5.13 | 0.18 / 0.23 / 0.32 / 0.50 | 203 |
| Larsen Original | 80 / 48 / 49 / 49 | 0.76 / 1.02 / 2.33 / 5.23 | 0.25 / 0.22 / 0.34 / 0.51 | 227 |
| Evah Pirazzi (synthetic A) | 69 / 61 / 53 / 57 | 0.65 / 1.29 / 2.52 / 6.07 | 0.21 / 0.28 / 0.37 / 0.59 | 239 |
| Evah Pirazzi (steel A) | 80 / 61 / 53 / 57 | 0.76 / 1.29 / 2.52 / 6.07 | 0.25 / 0.28 / 0.37 / 0.59 | 251 |
| Obligato | 76 / 58 / 52 / 55 | 0.72 / 1.23 / 2.47 / 5.86 | 0.23 / 0.27 / 0.36 / 0.57 | 241 |
| Jargar Superior (steel A, C) | 80 / 65 / 55 / 56 | 0.76 / 1.37 / 2.61 / 5.97 | 0.25 / 0.30 / 0.38 / 0.58 | 256 |
| Spirocore (all steel) | 75 / 70 / 69 / 65 | 0.70 / 1.47 / 3.26 / 6.91 | 0.23 / 0.32 / 0.47 / 0.67 | 278 |
| Pirastro Chorda (plain gut) | 64 / 48 / 49 / 43 | 0.60 / 1.02 / 2.33 / 4.60 | 0.20 / 0.22 / 0.34 / 0.45 | 204 |

- Larsen soft and strong: A 7.7 / 8.4 kg, strong D 5.6, G 5.3, C 5.2 kg. Larsen Virtuoso Extra
  Long (420 mm): A 10.5, D 8.0, G 6.5, C 6.4 kg **[T]** [Larsen].
- **Gut** (L about 36 cm, a = 415) **[T, wf]** [GamutVa]:
  - plain A 0.72–0.82 mm, 4.8–6.2 kg;
  - D 1.00–1.10 mm, 4.2–5.0 kg;
  - plain G 1.48–1.60 mm, 4.0–4.8 kg;
  - plain C 2.10–2.42 mm, 3.6–4.8 kg;
  - silver-wound G 0.78–0.88 mm and C 1.10–1.20 mm.

  Medium gauges give μ ≈ 0.59 / 1.11 / 2.44 / 4.97 g/m and Z ≈ 0.18 / 0.22 / 0.32 / 0.44 kg/s **[E]**.

**The short, heavy C string** ([KnutStr] **[T]** unless marked):
- Wave speed 98 m/s on the viola C, 129 m/s on the violin G, 90 m/s on the cello C. **The viola's
  C behaves almost like a cello string, on a violin-scale body.**
- A viola scaled fully from the violin would have L = 49 cm **[E]** and keep the violin G's
  129 m/s. At the real 37.5 cm and the same tension, μ must rise by (49.2/37.5)² = 1.72 **[E]**.
  The real viola C has 4.95–6.31 g/m, 2.0–2.3 times the violin G's 2.12–3.09 g/m, at only 1.0–1.2
  times its tension.
- Ratio of wave resistance, lowest string to highest: viola C/A about 2.0–2.7, violin G/E about
  1.6–1.9, cello C/A about 2.6–2.8 **[E]**. In how much heavier the low string is under the bow,
  the viola is close to the cello.
- A plain gut C would be 2.1–2.4 mm thick, hence the universal wound C (1.1–1.2 mm over silver)
  **[T, wf]** [GamutVa]. Most sets taper in tension from A down to C. Obligato offers a heavy C;
  Jargar Superior has a steel C **[S]** [Heath].
- What a longer string buys **[E]**: at fixed μ = 5.6 g/m the C's tension is 47 N at L = 35 cm,
  52.5 N at 37 cm and 68 N at 42 cm. This is the case for the 42–43 cm strings of Hutchins' alto.
  Long-scale sets (Larsen Extra Long) are 25–30 % tenser.

**Not found for viola strings:** measured bending stiffness, inharmonicity, the damping
coefficients (η_F, η_B, η_A, Q) and torsion. There is no viola counterpart of Mansour's or
Lampis' cello tables. For the model **[E]**:
- **Inharmonicity:** B = π²EI/(T L²) ≈ 1.5·EI at T ≈ 48 N, L = 0.37 m (EI in N m²). A wound C
  with EI 0.3–1 x 10⁻⁴ N m² (well under the cello C's 6.2 x 10⁻⁴) gives B ≈ 50–150 x 10⁻⁶: order
  of magnitude only.
- **Damping and torsion:** reuse the cello's values per core material (torsional Q 34 steel,
  45 nylon, 20 gut; the three-term loss model, η_F tripled under a finger). The loss mechanisms
  belong to the material, not to the instrument.

### 1.2 The body's dimensions

| | Viola (small / medium / large) | Violin | Cello | Source |
|---|---|---|---|---|
| Body length | 390 / 405–415 / 420–430 mm | 355 mm | 750–760 mm | **[T, wf]** [Johannsson]; [FR91] |
| Body length (table) | 43 cm (x1.17 the violin) | 35.5 cm | | **[T]** [FR91 Table 10.1] |
| Overall length | 70–71 cm | 59–60 cm | 124 cm | **[T]** [FR91] |
| Upper / C / lower bout | 188/127/228; 195–198/130–138/235–238; 200/140/250 mm | 160/105/200 mm | 340/240/430 mm | **[T, wf]** [Johannsson]; [SSI14] |
| Rib height | 36 / 38 / 40 mm (1.1–1.2 thick) | 30–35 mm | 111–121 mm | **[T, wf]** [Johannsson]; [SSI14] |
| Arching, back / top | 15/16, 16/17, 17/18 mm | about 15–16 mm **[M]** | 28 / 29 mm | **[T, wf]** [Johannsson] |
| f-hole length | 78 / 78 / 82 mm; 46–50 mm between the upper holes | 72–75 mm **[M]** | 130 mm | **[T, wf]** [Johannsson] |
| Stop (top edge to bridge) | 219 / 225 / 225–230 mm | about 195 mm **[M]** | 405 mm | **[T, wf]** [Johannsson] |

Johannsson's is a luthier's web table marked "being modified and expanded".

- **Plate thicknesses** (mm, small / medium / large) **[T, wf]** [Johannsson]:
  - Top: 3.1 / 3.2 / 3.3 at the soundpost; 2.5 / 2.6 / 3.0 in the flanks and the bouts;
    3.9 / 4.0 / 4.1 at the edge.
  - Back: 4.0 / 4.5 / 5.0 at the centre; 2.4–3.0 in the flanks and the bouts; 3.9–4.1 at the edge.
- **Bass bar and soundpost:** bar 5.5 / 6 / 7 mm thick and 13 / 14 / 15 mm high at the middle;
  soundpost 6 / 6.5 / 7 mm in diameter **[T, wf]** [Johannsson].
- **Free plates** (Hutchins, 16–17-inch violas): mode 2 at 115–125 Hz, mode 5 at 230–250 Hz.
  Violin 165–185 / 330–370 Hz, cello 60–65 / 120–130 Hz **[T]** [FR91]. The viola's are about
  0.68–0.70 of the violin's **[E]**.
- **Mass:** 500–700 g, a guide figure (chin rest not stated) **[S]** [Violinspiration]. No weighed
  luthier's figure found.
- **Air volume: about 3.0–3.6 litres for a 39–43 cm viola [E].**
  - Method: plan area ≈ 0.7 · length · lower bout, times (rib + 0.4 x the two arch heights).
  - The same recipe gives 2.1 litres for the violin, against the commonly quoted ~2 litres **[M]**.
  - The viola is about 1.6 times the violin's volume **[E]**.
  - No published figure was found.
- **f-hole area: about 10–11 cm² [E]**, the violin's 9.25 cm² [SSI14] scaled by (78/73)².

**"Too small for its tuning":**
- Relative to the violin: **body length x1.17, resonance placement x1.33, string tuning x1.50**
  **[T]** [FR91 Table 10.1, after Hutchins 1980].
- "Its dimensions are only about 15 % greater than those of the violin, and the principal
  resonances lie from 20 to 40 % below those of the violin." The air resonance and the main body
  resonance "tend to lie between the open string frequencies instead of on them, as in the violin.
  Thus the viola is not a scaled-up violin" **[T]** [FR91 §10.11]. And: "the similarity
  requirements are even more difficult to fulfill in the viola (Cremer, 1984)" **[T]**.
- Full scaling would give a body of 1.5 x 35.5 = **53 cm** and strings of 49 cm **[E]**.
- **Hutchins' octet "alto"** (the vertical viola), scaled by ergonomics:
  - body 50.2 cm, strings 43 cm, overall 82–83 cm;
  - length x1.44 and resonance placement x1.50 **[T]** [FR91 Table 10.1];
  - the Met's 1982 alto: body 51.7 cm, strings 42.2 cm **[S]** [Met].

  The octet aimed A0 at about 1.5 times the lowest string. The alto's bout widths over body length
  are 46 % / 59 % (treble 43 / 54 %). Its A string was among the octet's "more problematic
  strings" **[T]** [SSI18].
- **Schelleng's flat-plate scaling** **[T]** [SSI18]: H₂ = H₁(f₂/f₁)(L₂/L₁)²,
  M₂ = M₁(f₂/f₁)(L₂/L₁)⁴. A 41 cm viola with the violin's mode placement times 2/3 would need plates
  0.89 times the violin's thickness **[E]**. Real viola tops (2.5–3.3 mm) are at or above the
  violin's: the real viola does not follow this scaling either.
- **Wolves:** the viola and the cello suffer them more than the violin because their bodies are not
  scaled with 1/f, which leaves a smaller impedance mismatch between string and body **[T]** [FR91,
  after Schelleng].

### 1.3 The bridge

- **Dimensions:**
  - Feet 45 / 46 / 47 mm wide, 5 mm thick, about 38 / 39 / 40 mm high **[T, wf]** [Johannsson].
  - Blanks at 46–50 mm (52 mm in one model); 4.7 mm finished thickness in a fitting tutorial
    **[S]** [blank listings].
  - Violin: 41 mm feet **[S]**, about 33 mm high **[M]**. Cello: 90 mm feet, 92 mm high.
- **Mass:** not found. Violin bridges finish at 2.0–2.1 g **[S]**. Scaled by width x height x
  thickness, (46/41)(39/33)(4.7–5/4.2) ≈ 1.5–1.6 times the violin's: **about 3.0–3.5 g [E]**.
  The cello's is about 19 g.
- **Its own resonances:**
  - Violin, rigid support: about 3 and 6 kHz (Reinicke). Cello: about 1–1.5 and 2.2 kHz **[T]**
    [FR91].
  - Professionally cut violin bridges are tuned to 2.8–3.4 kHz **[S]** [VSA].
  - Mounted on a violin, the rocking and bounce modes fall to about 2.5 and 3 kHz **[S]**
    [McLennan].
  - **No viola value was found.** Scaled by 1.15 in size at the same wood and shape: first
    (rocking) mode about 2.6 kHz clamped, 2.0–2.3 kHz mounted, the bridge hill about 2–2.5 kHz
    **[E]**.
- **A violin-type bridge, not a cello-type one.** The viola bridge has short legs and a rocking
  top. Its first mode is the violin's rocking of the top, not the cello's bending of long legs
  **[M]**. **For the model this is the main structural change from the cello**: the cello's two
  bridge hills (about 1 and 2–2.3 kHz) become one violin-like hill, about 1.9–2.2 kHz by the two
  estimates compared in section 2.2.
- **Measured hill:** Pérez Carrillo et al. measured a viola's bridge admittance. Above about 1 kHz
  there is a broad hill, strongest in the horizontal admittance, and a second broad peak in the
  vertical. Shown only in figures, without numbers **[T]** [UPF13].
- **Lever**, (height / foot spacing)²: viola (39/46)² = 0.72, violin (33/41)² = 0.65, cello
  (92/90)² = 1.04 **[E]**. The viola is about 1.1 times the violin's lever. (The 2.4 that
  `cello.md` cites for cello/violin from [FR91] must use another definition: this formula gives
  1.6.)

---

## 2. Body resonances

Published viola acoustics is thin. "One must search diligently to find 1 or 2 % as much published
material" as for the violin **[T]** [FR91 §10.11]; Pätynen & Lokki say the same **[T]**
[Patynen10]. Meyer's book could not be opened, so every Meyer number here is second-hand.

### 2.1 Signature modes

Names as in `cello.md`. Hutchins' older "main wood" is B1 (B1− and B1+ together), "main air" A0.

| Mode | Coffey 2013 **[T]** | Forum, 3 violas **[S]** | Powell 2012, back NAH **[T]** | Pätynen LTAS peaks **[E from T]** | Violin | Viola / violin **[E]** |
|---|---|---|---|---|---|---|
| A0 | 244 (15½"), 220 (16¼"), 228 (16½"), 208 (Tertis model) | 231 (40.2 cm), 210 (41.6), 228 (40.7) | 224 (the back "breathing") | 223 | 270–290 [S]; 271 measured [T, Coffey] | 0.77–0.90 |
| CBR | | 321, 296 (the third merged with B1−) | 328? (the sides in antiphase) | | 380–420 [S] | about 0.75–0.8 |
| B1− | | 374/385 (split), 342, 385 | | 366 | 430–480 [S] | about 0.8 |
| A1 | 390 (16½"); also 455, 950, 995 | | | | 466–474 [T, Coffey] | 0.83 |
| B1+ | | 453 (weak), 442 | | | 510–580 [S] | about 0.8 |
| Higher | | | 560, 1078, 1504 | 975 (weak), 1876 | | |

- **The sources:**
  - Forum [S] [Maestronet]: maker Don Noon's tap spectra of two violas and a recording of a third:
    anecdotal, but the only multi-viola B1 data found.
  - Powell [T] [Powell12]: one Eastman VA200, an undergraduate report; its two major peaks about
    220 Hz and 1 kHz.
  - Pätynen [T] [Patynen10]: the violin's long-time-average peaks at 290, 530, 1500 and 3350 Hz.
    The viola's lie 23, 31, 35 and 44 % lower, and its third is "somewhat absent". The Hz values
    in the table are arithmetic.
- **Against the strings** [T] [Hutchins88]:
  - On the violin, A0 and A1 lie within a semitone of the open D and A.
  - On the viola they lie at about B–B♭–A on the G string (220–247 Hz) and F–F♯ on the D string
    (349–370 Hz). Hutchins found this placement "the most critical factor in tone quality between
    the violin and other members".
  - The combined A1–B1 peak lies at F–F♯ on the D string on small violas, E–F on large ones.
  - Hutchins 1962: in the viola and the cello "the two resonances fall 3 or 4 semitones higher than
    the middle two open strings… The resonances are too far above the lower notes" **[T, as cited]**
    [Coffey13].

| Ratios **[E]** | Violin | Viola | Cello |
|---|---|---|---|
| A0 / lowest open string | 1.4 | 1.59–1.86 | 1.25–1.6 |
| B1− / lowest open string | 2.2–2.45 | 2.6–2.9 | 2.0–2.8 |
| A0 against the 2nd string | about a semitone below D4 | 2–4 semitones above G3 | at G2 |
| B1− against the 3rd string | at A4 | 2.5–4.5 semitones above D4 | near D3–A3 |

- **A1/A0:** violins 1.71 ± 0.05 (15 violins) **[T]** [SSI18]; Coffey's 16½" viola 390/224 = 1.74
  **[E]**. Hutchins: about a fifth apart "as in the violin and normal violas" **[T]**.
- **Size dependence** **[T]** [Coffey13, Hutchins88]:
  - A0 falls with size: 244 → 208 Hz from 15½" to the Tertis model.
  - One f-hole covered drops A0 to about 75 % (16½": 163–174 Hz; Tertis 153 Hz).
  - A1 is set by the body's length (a half wave in about 44 cm at 390 Hz). Deeper ribs lower A0;
    a shorter body raises A1.
  - Tertis-type violas (short, deep ribs) push A0 and A1 nearly an octave apart: "two very strong
    tone ranges… with a relatively weak area in between".
- **A0 is weaker on the viola:** against the strong peak near 440 Hz, A0 is 90–110 % on a violin,
  only 50–60 % on a large viola, and 20–25 % with one f-hole covered **[T]** [Coffey13].
- **The octet's alto** (a 50 cm, 20-inch viola, not standard) **[T]** [SSI18]: scaled so that
  A0 = 1.5 and B1 = 2.25 times its lowest string. Its two B1 modes radiated "approximately the
  same". "From the alto onward, A1 was readily apparent"; A1 radiates more relative to B1 as size
  grows.
- **Q and damping:** none found for a viola mode. The violin's A0, measured by an internal
  microphone, is 28 Hz wide at 263 Hz: Q ≈ 9–10 **[E from T]** [Coffey13]. (Powell's η =
  0.27 ± 0.19 % from string decays is methodologically doubtful: do not use it as a mode Q.)
- **For the model [E]:** the cello's modes x2.2–2.3 give the viola's (A0 97 → about 220 Hz,
  T1 about 173 → about 380 Hz). Better, place them directly: **A0 225, CBR 300–320,
  B1− 360–385, A1 390, B1+ 440–455 Hz**. Body size as a parameter moves A0 from 244 Hz (15½") to
  208 Hz (17"+).

### 2.2 Violin, viola, cello: what scales and what does not

| | Violin | Viola | Cello |
|---|---|---|---|
| Lowest string | G3 196 Hz | C3 130.8 Hz | C2 65.4 Hz |
| Body length | 35.5 cm | 43 cm in the tables; 38–47 in use, mostly 40.5–43 [S, WikiDE] | 75 cm |
| String length | 32.7–32.8 cm | 37.5 cm [SSI16] | 69 cm |
| A0 | 270–290 Hz | 208–244 Hz | 82–104 Hz |
| B1− | 430–480 Hz | 342–385 Hz [S] | 132–185 Hz |
| Mode ratio to the violin | 1 | about 0.75–0.85 | 0.3–0.4 |
| Tuning ratio to the violin | 1 | 0.667 | 0.333 |
| Bridge hill | 2–3 kHz | about 1.5–2.5 kHz **[E]** | 1–2.5 kHz (hills at 1 and 2–2.3) |
| Omnidirectional up to | about 1–1.25 kHz band | 630 Hz band | 200–300 Hz |
| Sum of string tensions | 189–265 N | 203–282 N | 493–601 N |

- **"Too small":** Bessaraboff called the viola "the ailing member of the violin family — too small
  for its tone range — too large to play comfortably under the chin" **[T, as quoted]**
  [Hutchins88]. A body scaled to the tuning would be about 53–54 cm **[E, S]**.
- **Radiating the lowest note:** λ/π at C3 is 0.83 m, against a 0.43 m body; the violin's is
  0.56 m against 0.36 m **[E, after Jansson ch. 3]**. Askenfelt: the violin's body is about λ/4 of
  its lowest note; the bass's λ/4 is twice its body; the viola and cello lie between **[T]**
  [SSI15].
- **Played loudness:** the same notes gave more power played on a higher string (D, A) than on a
  lower one (G, D) **[T]** [FR91 fig. 10.40, Barnes et al. 1983; no numbers].
- **Wave resistance (g/s)** **[T]** [SSI16 Table 16.1]: viola lowest to highest string 485–619,
  324–413, 220–276, 184–304; violin 272–397, 178–193, 167–217, 165–210. The viola's strings are
  not much heavier than the violin's: the coupling grows much less than on the cello.
- **The bridge hill:** no viola bridge-admittance number was found (section 1.3). Two estimates:
  - the violin bridge scaled by size: 2.0–2.5 kHz **[E]**;
  - the violin's hill scaled by the viola's measured 0.65–0.75 for its higher peaks: 1.5–2.3 kHz
    **[E]**. This fits Pätynen's viola peak near 1.9 kHz (violin 3.35 kHz) and the brightness
    formant at 2.0–2.6 kHz (2.4).

  **Take 1.9–2.2 kHz** as the starting point.

### 2.3 The wolf

- **Where [T]** [Hutchins88]: "Some violas are plagued with a wolf-note in this E–F♯ range,
  particularly when these notes are fingered on the mid-G string or upper C string": E4–F♯4,
  330–370 Hz, on the combined A1–B1 peak. It moves with temperature and humidity, because A1 and
  B1 respond differently to them.
- **Anecdotes [S]:** "mild wolf on F♯4, most noticeable on the G string"; "horrid wolf on low B♭"
  (B♭3 near A0, or B♭4 near B1+: not clear).
- **Why more than on the violin and less than on the cello [T]** [FR91]: a smaller mismatch of
  string and body impedance (the body not grown with 1/f), and a taller bridge. Euphonics gives
  cello numbers only **[T]** [Euph9.4].
- **For the model [E]:** the wolf sits on the B1− target (360–385 Hz, about F♯4), on the G string
  or high on the C. Weaker than the cello's (lighter strings relative to the body: wave resistance
  0.5 against the cello's 1.46 on the lowest string).

### 2.4 The spectrum of the sound

**Formant regions [S]:**

| Source | Regions |
|---|---|
| Syrový, *Hudební akustika* 2013 p. 294, cited in [Smola23] | 220–240 Hz (Helmholtz: "sonority"); about 350 Hz; 600–1200 Hz ("nasality"); 2000–2600 Hz (brightness); 3000–3500 Hz (sharpness, "typical of quality instruments") |
| Sengpiel's table, source not given (probably Meyer) [Sengpiel] | main 220 / 350 Hz; adjacent 600 / 1600 Hz (violin 400 / 1000; cello 250–400 / 600–900) |
| Syrový's violin, for comparison | 400 (O); 800–1200 nasal (A); 1200–1600; 2000–2600 (E); 3000–4000 (I) |

- **Long-time average [T]** [Patynen10]: the violin's curve shape shifted down: peaks 23–44 % lower,
  the violin's third peak (1.5 kHz → about 1 kHz) nearly missing. Both fall about −15 dB/octave
  above 3 kHz. Pätynen: "its distinctive, somewhat hollow timbre".
- **Open strings:** harmonics 2–5 stronger relative to the fundamental than on the violin **[T]**
  [Powell12, one viola].
- **Nasality:** a violin with one f-hole covered (A0 271 → about 200 Hz) sounded "more nasal,
  rather like a viola" **[T]** [Coffey13, informal]. Mathews & Kohut's electronic violin: evenly
  spaced resonances made low G-string notes sound "more like a viola"; widely spaced peaks sounded
  like a violin **[T]** [SSI22].
- **Vowels:** the literature assigns U (low), O (middle), Å (high) to the viola. In a 64-listener
  test the high register sounded like "I", as the violin's does **[T]** [Reuter24].

---

## 3. Where the sound comes from

### 3.1 Which part radiates

- **Below A0** (C3 to about A3): the sum-rule cancellation of the cello. The lowest fifth's
  fundamentals are weak; the body is well under λ/π **[E]**.
- **A0 (about 225 Hz):** the f-holes. On violins more than half the radiation at B1− and B1+ also
  comes through the f-holes **[S]** [Strad3D]; no viola split found.
- **Powell's holography (back only) [T]:** at 224 Hz the whole back moves in phase (breathing),
  with diffraction at the edges; at 328 Hz the two sides move in antiphase.
- **Not found:** radiation efficiency by band, the share of f-holes against plates, holography of
  the top.

### 3.2 Directivity

Pätynen & Lokki: 22 microphones, anechoic, the same tones as the violin **[T]** [Patynen10]:
- "Overall the viola exhibits less directivity than the violin."
- Even below the **630 Hz** third-octave band (violin 1000–1250 Hz), front-concentrated above it.
- 500–800 Hz: strong variation with direction, and from tone to tone.
- About 2 kHz: a second, weaker fluctuation towards −36° azimuth, −11° elevation. Constant above
  2 kHz.
- Lateral plane: the 1 kHz octave goes front-right, the 2 kHz octave front-left. The median and
  transverse planes are like the violin's.
- Directional tone colour as on the violin: harmonics of different notes at one frequency radiate
  alike, most clearly an octave apart.
- Dynamics hardly change the pattern. Louder playing sharpens the peaks without moving them.
- **Meyer, as cited by Pätynen:** near 600 Hz the radiation is strongly concentrated towards the
  back plate in the horizontal plane (heard at the player's far left). Low frequencies are mostly
  omnidirectional.
- The upper limit of omnidirectional radiation follows the tuning better than the resonances
  (Pätynen's fig. 46).
- **TU Berlin** recorded a modern viola C3–F7 and a classical viola C3–C6 on a 32-microphone
  sphere **[T]** [Shabtai17]. The directivity data are in that database, not extracted here.

**For the model [E]:** the cello's band pattern moved up. Omnidirectional to about 600 Hz; a
per-mode dipole ("two sides") term from about 300 Hz (B1's antiphase sides); front-weighted above
630 Hz, 10 dB front–back by 1–2 kHz; the back-plate lobe near 600 Hz; constant above 2 kHz. Being
under the chin, the source is about 1.4–1.6 m above the floor **[E]**, not the cello's 0.4–0.6 m,
and points up and to the player's left.

### 3.3 Level and dynamics

Sound power level, dB re 1 pW:

| Source | pp | ff | Range | Note |
|---|---|---|---|---|
| Weinzierl 2018, modern viola, extremes over the range **[T]** | 53 | 97 | 44 | single notes |
| Same, weighted by Beethoven's pitch use **[T]** | 62 | 93 | 31 | |
| Same, classical viola, extremes (weighted) **[T]** | 57 (62) | 94 (91) | 37 (29) | |
| Meyer via Rindel **[T in Rindel; S for Meyer]** | 68 | 93 | 25 | forte 87; power factor k = 0.5 |
| Same, violin / cello | 65 / 67 | 97 / 97 | 32 / 30 | forte 89 / 90; k = 0.8 / 1.0 |
| Weinzierl, modern violin / cello: extremes (weighted) | 52 / 63 (56 / 70) | 95 / 97 (91 / 93) | | |

- In Meyer's figures the viola is the quietest upper string: 93 dB at ff (against 97), and 2 dB
  under the violin and 3 dB under the cello at forte. Its range is the narrowest.
- In Weinzierl's measurements the modern viola reaches the violin's ff (97 dB), and its weighted
  range (31 dB) is about the violin's (35 dB) **[E]**. **The sources disagree.** Weinzierl
  measured one instrument and one player.
- Free field **[E]**: L_p at 1 m = L_W − 11 dB, about 76 dB at forte by Meyer.

---

## 4. Projection and blend

- **Blend over projection** [T] [SSI13, Curtin & Rossing]: "a viola that projects over an orchestra
  with violin-like force and brilliance may not sound quite like a viola, nor is it likely to blend
  into a viola section or round out the sound of a string quartet… Research in this area would be
  useful." None was found.
- **Repertoire** [T, anecdotal] [Hutchins88]: on violas with A0 and A1 moved, Mozart's viola
  quintets "did not sound well". Hutchins suggests Mozart wrote the first viola around the A1 tones
  and the second around A0. A loudspeaker-filtered viola (Gorrill 1975) was insensitive to
  adjustment played solo, but in the quartet "the right amount of peakiness" mattered **[T]**
  [Jansson ch. 8].
- **Section size [T]** [Odeon]: 12 violas in a large orchestra (16/14/12/10/8), 8 for Mozart and
  Beethoven, 10 for Brahms.
- **Section level at forte [E]** (Meyer's forte + 10 log N): 12 violas 97.8 dB; 16 first violins
  101; 14 seconds 100.5; 10 cellos 100. The violas are the weakest strings by 2–3 dB, which fits
  the orchestrators' habit of doubling them with clarinets in tutti **[S]** [WikiDE].
- **Seating [S]** [WikiDE, unsourced]: violas front right are said to sound best; behind the first
  violins on the left their tone moves towards the violin's.
- **Which bands carry [E]:** the formants at 600–1200 Hz and 2–2.6 kHz lie inside the orchestra's
  strong region, and the viola has no peak near 3 kHz (−15 dB/octave above it). Hence the inner
  voice.
- **Section scatter:** none measured for violas. Use the cello's: bow changes with a standard
  deviation of 40–50 ms, ±10 c between players, independent vibratos, +10 log N.
- **Not found:** viola-against-violin balance measured in an orchestra; old-against-new viola
  listening tests.

---

## 5. Holding: chin, collarbone, left hand (the cello's end pin and floor)

**Short answer.** Most playing data come from violinists. The one primary study that measured
violists [Schoond09T] found that they use the same strategies as violinists, with somewhat more
force: up to 3 N against 2.5 N, most of it on the C string, and with more tilt of the hair. No
measurement exists of how much the player's chin and shoulder damp a viola.

### 5.1 What is measured (violin only)

- **Marshall 1985** (modal analysis of one violin, free and held by five players): the player acts
  on the **neck and the lower bout** as "a strong viscous damper acting in parallel with a weak
  spring". "Resonant frequencies and mode shapes are only slightly affected, but the amplitude of
  the response … is significantly lowered." No dB or Q values in the abstract **[S]** [Marshall85].
- **Bissinger & Mores 2015** (auralised structural-acoustic model): "holding the violin greatly
  diminishes its low frequency response"; the hall's reverberation compensates only weakly **[S]**
  [Bissinger15].
- **Shoulder rest:** one informal forum test found the low resonances (CBR, B1−) "a bit weaker and
  very slightly lower" with a rest fitted **[S, anecdotal]** [VnComShoulder]. Many violists play
  without one **[S]** [Chang18].
- **Not found:** Q or dB of A0, CBR, B1− or B1+ on any violin or viola with and without a player;
  the mechanical impedance of a jaw or collarbone.

### 5.2 Viola against cello

- **No floor path.** No end pin. The viola's lowest note (C3, 131 Hz) lies below its own A0
  (section 2), so a floor would not help it anyway **[E]**.
- **The player is the only support,** at three patches:
  1. the jaw on the chin rest, on the top's lower bout, over or left of the tailpiece;
  2. the collarbone and shoulder on the back's lower edge, directly or through shoulder-rest feet
     gripping the ribs;
  3. the left hand at the neck (the thumb and the base of the index).

  These are Marshall's "neck and lower bout". The cello's are the knees on the ribs, the chest on
  the upper back, and the pin. Both are damped mainly in their low corpus modes. On the viola the
  chin acts on the **top** plate, which has no cello counterpart **[E]**.
- **For the model [E]:** a parallel dashpot and weak spring at the lower-bout edge and at the
  neck. Size them to lower the low-mode peaks (A0, CBR, B1) by a few dB without moving their
  frequencies; that is all the literature supports. The cello model's "knees and chest" damping
  becomes this. Drop the floor path. The player's head and torso shadow the sound to the player's
  left and back **[M]**.

---

## 6. Bowing and articulation

### 6.1 The bow

| | Violin | **Viola** | Cello |
|---|---|---|---|
| Mass | 58–62 g [S, AdA]; 60 g [T, KnutBow] | **68–72 g [S, AdA]; 70–74 g [S, WikiVa]** | 78–82 g [S, AdA] |
| Weight at the tip | 16.5–18 g | **20–22 g** | 24–26 g [S, AdA] |
| Length | 74–75 cm [T, KnutBow] | **72–74 cm; about ½ inch shorter than the violin's [S]** | 69–72 cm |
| Hair length | 65 cm [T, KnutBow] | **not found; about 63–65 cm [E]** | about 59 cm |
| Hairs | 150–200 [T, KnutBow] | **about 180–250 [E]** | 200–290 |
| Ribbon / open string | 2.4–3.7 % | **2.4–3.5 % [T, BowNotes]: 9–13 mm on 375 mm [E]** | 1.5–2.6 % |
| Hair tension | 60 N; 50–65 N [T, KnutBow] | **not found; about 60–70 N [E]** | 70 N [T, model, Mansour2] |
| Balance from the frog | 19 cm [T, KnutBow] | **about 24 cm (9.5 in) [S, dealer]** | |
| Stick modes | 60, 160, 300, 500, 750 Hz [T, KnutBow] | **not found;** +17 % mass at the same stiffness: about 56, 150, 280 … Hz [E] | low hair–stick mode 65 Hz [T, FR91] |
| Frog | square corner | **outer corner rounded [S]** | |

- **Bounce:** the violin bow on a string at the spiccato point bounces at 13–14.7 Hz [T,
  GuettlerThesis]. The viola bow is 17 % heavier at the same hair stiffness: **about 12–13.5 Hz
  [E]**. Players say the viola bow "sits in the string" **[M]**.
- **Tilt:** in Schoonderwaldt's study the viola bow was "stiffer than normal". Violists tilted the
  hair up to **60°** (violinists −10° to 45°) **[T]** [Schoond09T]. On violin and viola the tilt
  grows towards the frog and at pp. On the cello and bass it grows towards the tip, because of
  how they are held **[T, citing Trendelenburg]**.

### 6.2 Force, speed, place

**Violists, measured** (motion capture plus a bow-force sensor; 4 players, 6 sessions, 3 on the
viola) **[T]** [Schoond09T, the thesis summary of Paper IV; the full paper Schoond09J not read]:
- Tasks: détaché on all strings, third finger in first position, notes of 4, 2 and 0.2 s at f, mf
  and pp.
- Bow speed about 15 cm/s in whole notes and 30 cm/s in half notes, up to 2 m/s in sixteenths
  (distributions for violinists). β 0.03–0.3.
- **Viola: bow force did not exceed 3 N** (violin 2.5 N; accents in musical fragments 3–4 N on the
  violin). **"Higher bow forces on the lowest strings, especially the viola C string."**
- Speed rose from the lower to the higher strings in fast forte notes, on both instruments. Long
  notes vary their dynamics mostly by β, short ones mostly by speed.
- Players have signatures: one violist varied mainly force, with consistently low speeds; another
  varied mainly β, with low force.
- **Attacks (preliminary):** on the viola, β was higher on stopped strings than on open ones. Open
  strings got more force and acceleration. The highest attack forces and accelerations were on
  the **viola C**.
- Level follows 20 log(v/β), R² = 0.90 **[T, violin]**. Brightness rises with force, falls with
  speed and rises with β **[T]**.

**Violin for scale [S]** [Askenfelt89 via Chudy]: 10–50 mm from the bridge (sul ponticello 1–4 mm,
sul tasto 55–60 mm), 0.2–1 m/s, 0.5–2 N (0.15–3 N). The same β on a 375 mm string against the
violin's 328 mm: x1.14, so about 11–57 mm, sul tasto 65–70 mm **[E]**.

**Schelleng's limit with the viola's strings.** Z from [KnutStr] **[T]**; the arithmetic is
**[E]**: F_max = 2Zv/((μs − μd)β) with μs − μd = 0.5, v = 0.2 m/s, β = 0.1, which is 8Z.

| String | f (Hz) | c = 2Lf (m/s) | Z (kg/s) | Tension (N) | F_max (N) |
|---|---|---|---|---|---|
| A4 | 440 | 330 | 0.18–0.30 | 61–100 | 1.5–2.4 |
| D4 | 293.7 | 220 | 0.22–0.28 | 48–61 | 1.8–2.2 |
| G3 | 196 | 147 | 0.32–0.41 | 48–61 | 2.6–3.3 |
| **C3** | 130.8 | **98** | **0.49–0.62** | 48–61 | **3.9–5.0** |
| violin G3 | 196 | 129 | 0.27–0.40 | 35–51 | 2.2–3.2 |
| cello C2 | 65.4 | 90 | 1.29–1.53 | 117–138 | 10–12 |

- **Why the viola's C is hard to speak [E]:**
  - Z = T/c, and the short string forces a low wave speed. At the same tension the viola C has
    1.3 times the violin G's Z per newton, and 1.6–1.8 times its Z overall.
  - F_max grows as Z, F_min as Z². The arm must give about 1.5–2 times the violin G's force just to
    keep the Helmholtz motion. Schoonderwaldt's violists did just that.
  - The heavy, low-tension wound string also has more internal damping and inharmonicity **[M]**,
    and its fundamental lies below the body's A0. Hence its slow, "covered" speech.
- **Amplitude cap [T, violin]:** at the E string's bow speed, the violin's G would swing 5 mm and
  hit the D string, with +33 cents of pitch rise. The viola's C must cap its forte speed in fast
  notes in the same way **[E]**.
- **Force window for the model [E]:** the bass and cello window (`cello.md` 6.2) with the viola's
  Z: usable forces of roughly 0.3–2 N on the A and 0.8–3.5 N on the C. That agrees with the
  measured 3 N maximum.

### 6.3 Attacks

- **Guettler's diagram** applies unchanged. Violinists' attacks filled its predicted triangles:
  small β with high force, large β with high acceleration **[T]** [Schoond09T].
- **The transient scales with the period [T, KnutSculpt].** Listeners accept up to 50 ms of
  prolonged periods or 90 ms of multiple slips on the violin's open G: about 10 and 18 periods
  **[T]** [GuettlerAskenfelt].

| Viola open string | Period | 10 periods | 18 periods |
|---|---|---|---|
| C3 130.8 Hz | 7.6 ms | 76 ms | 138 ms |
| G3 196 Hz | 5.1 ms | 51 ms | 92 ms |
| D4 293.7 Hz | 3.4 ms | 34 ms | 61 ms |
| A4 440 Hz | 2.3 ms | 23 ms | 41 ms |

All **[E]**. The viola's C3 is the cello's C3 played on its G string. Cello G strings settled in
6.7–10.3 periods on average [T, Lampis24]. Expect the viola's C to settle in 50–80 ms in a good
attack, and over 130 ms in a poor one **[E]**. The body adds its own build-up: about 18 ms on the
violin [T, KnutOnset], slightly longer on the viola **[E]**.

- **Habits:**
  - "The viola responds to changes in bowing more slowly": violists start their bow movements
    earlier than violinists **[S, WikiVa]**.
  - Violists, like violinists, often land a moving bow; cellists start "from the string" **[T,
    GuettlerCreation]**. On the C, violists commonly start from the string, weight first **[M]**.
  - Strong attacks used 5–15° of tilt, soft ones 20–25°. Open strings got flatter hair **[T,
    violin and viola pooled]**.

### 6.4 Strokes

Viola-specific numbers are scarce; most are scaled from the violin.

| Stroke | Viola difference | Numbers |
|---|---|---|
| Détaché | As on the violin; speed rises from C to A; more force on C | 15–30 cm/s in long notes, up to 2 m/s in sixteenths; F ≤ 3 N **[T, Schoond09T]** |
| Legato, bow change | Earlier preparation; a heavier bow | No duration found |
| Martelé | Pinch then release; more initial force on C and G | Irregular first periods accepted under 20–30 ms **[T, violin]**: 3–4 periods on C3 **[E]** |
| Spiccato | Bow thrown near its balance; slightly slower bounce | 12–13.5 Hz **[E]** from the violin's 13–14.7 Hz **[T]** |
| Sautillé | The bow's own bounce | About 9–13 notes/s **[E]** |
| Ricochet | As on the violin | No data |
| Tremolo | Small strokes at the tip | 10–16 per second, as for strings generally **[M]** |
| Sul ponticello | β < 0.04: 0–15 mm from the bridge **[E]** | Multiple slips; the n ≈ 1/β harmonic dominant **[T, violin simulation]** |
| Sul tasto | β 0.15–0.25: 55–95 mm **[E]**, limited by the end of the fingerboard | Darker; near the lower force limit **[M]** |
| Harmonics | Natural ones speak readily on A and D; slow on C | The bass model's rule (the bow drives every n-th mode) carries over |
| Pizzicato | **This project's choice: plucked with the nail, the velocity only setting the displacement** (as the cello now does, `PJ_PIZZ_FLESH`), over the end of the fingerboard, about 1/5–1/4 of the string **[M]** | **Decay times not found.** Smaller instruments and open strings decay faster; the upper partials die first **[S, WikiVnAc]**. The C's fundamental (below A0) should decay faster than its 2nd and 3rd partials **[E]**. Estimate: A about 1 s or less, C about 1–1.5 s **[E]** |

### 6.5 Vibrato

| Source | Players | Rate | Width | Marker |
|---|---|---|---|---|
| **MacLeod 2008** | 40 high-school and university **violinists and violists** | +0.32 Hz in the high register | +26 c high against low; +4 c at f against p | [S, abstract] |
| MacLeod 2008 | | **violinists "slightly faster and wider than violists"** | violinists 13 c wider in the high register | [S] |
| Geringer & Allen 2004 | 40 violinists and cellists | about 5.5 Hz, no difference | violin wider than cello | [S] |
| Allen, Geringer & MacLeod 2009 | 1 artist violinist | 5.7 Hz (1st position), 6.3 Hz (6th) | | [S] |
| A violin study (4 professionals) | | 6.63 Hz | 63 c | [S] |
| Timmers & Desain 2003 | strings generally | | 0.2–0.35 semitone | [T, citing] |
| Hu 2025 (cello) | | 6.5 Hz (soloists) | finger travel 0.5 % of L | [T] |

- **The claims conflict.** Wikipedia says violists use "a wider and more intense vibrato … with the
  fleshier part of the finger" **[S]**. The only measurement (MacLeod) says slightly *narrower and
  slower* than violinists. Trust the measurement. Whether its widths are peak-to-peak or ± is not
  stated in the abstract.
- **Finger travel [E]:** a semitone near the nut is 21 mm on the viola, 18 mm on the violin. The
  same rocking of the finger gives about 12 % fewer cents in first position.
- **For the model [E]:**
  - solo: 5.8–6.4 Hz, ±12–25 c, wider and about 0.3 Hz faster in high positions, +4 c at forte;
  - section: 5.3–5.8 Hz, narrower;
  - the amplitude modulation is not added by hand: it comes from the partials sweeping the body's
    resonances.

  The viola lies between the cello (5.3–6.5 Hz, ±10–35 c by position) and the violin.
- **Not found:** the onset delay after the note's start; a study of professional violists in Hz and
  cents; the pitch centre's direction (on the violin it is flat by about 2–5 c **[S]**).

### 6.6 The left hand and the player's choices

- **Stretch [E]** (L = 375 mm against the violin's 328 mm):
  - a semitone in first position is 21.0 mm (violin 18.4);
  - a whole tone 40.9 mm (violin 35.7);
  - a fourth from the first to the fourth finger 94 mm (violin 82; the cello needs a shift for it).

  Violists use wider fingerings **[S, WikiVa]**, extend more, and use half position and
  extensions instead of shifts **[M]**. The elbow swings further under the instrument for the C
  string **[S]**.
- **Range [M]:** C3 to about E6 orchestrally (the A string's fourth finger in about 7th position),
  A6 and above in solo writing. Written in the alto clef, the treble clef for long high passages
  **[S]**.
- **Which string [M]:** each covers about 1.5–2 octaves in ordinary use.
  - C: dark, rich, slow to speak, the "viola colour".
  - G: warm, veiled.
  - D: soft, covered.
  - A: bright, nasal, penetrating; the solo voice above D5.

  The same pitch high on a lower string is darker (sul G, sul C). High positions on the C are
  avoided for clarity.
- **Portamento and shifts:** no viola data. Use the cello's (slides of 45–300 ms) and the
  violin's **[E]**.
- **Double stops:** a fifth lies across one finger and is harder to play in tune on a longer
  string **[M]**. Chords are broken two plus two; the cello's 60 ms chord-spread rule carries over
  **[E]**.
- **Section against solo [M]:**
  - Section: lower positions, less vibrato, on the string, an inner voice (tremolo, offbeats,
    divisi).
  - Solo: nearer the bridge, more force, wider vibrato, more of the C and A strings.

---

## 7. Main considerations for a model

In order of expected gain. The model is the cello's code with the viola's data. Items 1–3 are
what makes it a viola and not a small cello.

1. **The body's modes placed against the strings, not scaled with them:** A0 225 Hz (208–244 by
   size), CBR 300–320, B1− 360–385, A1 390, B1+ 440–455 Hz, all 2–4 semitones above the
   corresponding open strings. A0 weaker relative to B1 than on the cello or violin (50–60 % of the
   peak near 440 Hz). This gives the veiled lowest fifth and the nasal middle. Body size should be
   one parameter (15½"–17"), moving A0 and A1 together.
2. **A violin-type bridge:** about 3–3.5 g, 39 mm high, 46 mm feet, rocking about its top. One hill
   at about 1.9–2.2 kHz instead of the cello's two (about 1 and 2–2.3 kHz). The structural change
   from the cello is its shape, not just a smaller size.
3. **The spectrum's shape:** formants near 220–240 and 350 Hz; nasality 600–1200 Hz; brightness
   2–2.6 kHz; −15 dB/octave above 3 kHz. A weak third LTAS peak (about 1 kHz). If items 1–2 are
   right this should follow; it is the test.
4. **The C string as a heavy, low-tension string:** wave speed 98 m/s, Z 0.49–0.62 kg/s (about 2–2.7
   times the A's), slow to speak; the cello's string loss model per core material; inharmonicity
   unmeasured (order 50–150 x 10⁻⁶ on the C).
5. **The wolf** near F♯4 (360–385 Hz), weaker than the cello's, on the G string or high on the C:
   the check that the bridge admittance's level is right.
6. **Attacks that scale with the period:** 10 periods are 76 ms on the C, 23 ms on the A. Violists
   land a moving bow on the upper strings and start from the string, weight first, on the C.
7. **Bow and forces:** a 70 g bow, hair 9–13 mm, bounce about 12–13.5 Hz; forces 0.3–2 N (A) to
   0.8–3.5 N (C), at most about 3 N; speeds 15–30 cm/s in long notes, to 2 m/s; β 0.03–0.3; more
   tilt (to 60°).
8. **Vibrato:** solo 5.8–6.4 Hz, ±12–25 c, about 0.3 Hz faster and wider high up; section 5.3–5.8 Hz
   and narrower. Slightly slower and narrower than the violin's (MacLeod), against the folklore.
9. **Holding instead of the floor:** no end pin and no floor path. Damping by the chin (top, lower
   bout), the collarbone (back edge) and the left hand (neck), lowering the low-mode peaks by a few
   dB without moving them. The source sits 1.4–1.6 m up, pointing up and to the player's left.
10. **Radiation by band:** omnidirectional to about 600 Hz; the back-plate lobe near 600 Hz; front
    above 630 Hz; constant above 2 kHz.
11. **Level:** forte about 87 dB sound power (Meyer) to violin-like (Weinzierl); pp 53–68 dB.
    Dynamics as spectrum: brightness from force, level from v/β.
12. **The left hand:** a 21 mm semitone, extensions rather than shifts; the same pitch on a lower
    string for colour; the C string's dark, the A string's nasal colours.
13. **Pizzicato with the nail** (this project's choice), velocity setting the displacement; decays
    estimated at about 1 s (A) to 1–1.5 s (C).
14. **A section:** 8–12 players, ±10 c, 40–50 ms spread, independent vibratos; 2–3 dB under the
    other string sections.

### Cello → viola

| Parameter | Cello (`cello.md`) | Viola | Factor / change |
|---|---|---|---|
| Tuning | C2 G2 D3 A3 | C3 G3 D4 A4 | x2, one octave |
| String length | 0.69 m | 0.37–0.375 m (0.355–0.39 by size) | x0.54 |
| Tension per string | 117–180 N | 47–100 N (A highest) | x0.4–0.55 |
| Mass per length, lowest / highest | 14.3–17 / 1.5–1.9 g/m | 4.95–6.31 / 0.56–0.92 g/m | x0.35 / x0.4 |
| Wave resistance, lowest / highest | 1.29–1.53 / 0.46–0.58 kg/s | 0.49–0.62 / 0.18–0.30 kg/s | x0.4 |
| Wave speed, lowest string | 90 m/s | 98 m/s | about the same |
| Bending stiffness | 1.9–6.2 x 10⁻⁴ N m² | not measured; about 0.3–1 x 10⁻⁴ on the C **[E]** | lower |
| A0 | 95–100 Hz | 208–244 Hz (225 typical) | x2.2–2.3 |
| Main top mode B1− | 145–175 Hz | 342–385 Hz | x2.2–2.4 |
| CBR / B1+ | 163–186 / 185–220 Hz | 296–328 / 440–455 Hz | x1.8 / x2.1–2.4 |
| A1 | 200 Hz | 390 Hz | x1.95 |
| A0 strength against B1 | strong | 50–60 % of the peak near 440 Hz | weaker |
| Body length / ribs | 75.5 / 12 cm | 39–43 / 3.6–4 cm | x0.55 / x0.32 |
| Top thickness | 3.9–4.7 mm (5.5 edge) | 2.5–3.3 mm (4 edge) | x0.65–0.7 |
| Air volume | about 30 l **[E]** | about 3.0–3.6 l **[E]** | x0.11 |
| f-hole area | 42.5 cm² | about 10–11 cm² **[E]** | x0.25 |
| Bridge | 92 mm, 19 g, long legs; sway 1.2–1.5 kHz, second 2–2.3 kHz | 39 mm, about 3–3.5 g, violin type; rocking, one hill about 1.9–2.2 kHz | a different shape |
| Wolf | E3–F♯3, strong | about E4–F♯4, milder | x2 in pitch; weaker |
| Omnidirectional up to | 200–300 Hz | about 600 Hz (630 Hz band) | x2–3 |
| Support | end pin, floor (−20 dB; +5 dB at 64–80 Hz on a riser), knees and chest | chin, collarbone, left hand; no floor | the floor path off |
| Source height | 0.4–0.6 m | 1.4–1.6 m | |
| Bow | 78–82 g, hair 59 cm, 70 N | 68–74 g, hair 63–65 cm, 60–70 N **[E]** | lighter, longer hair |
| Bow force | 0.5–6 N | 0.3–3.5 N (at most about 3 N measured) | x0.5–0.6 |
| Bow speed, β | 6–90 cm/s; β 0.035–0.16 | 15 cm/s to 2 m/s; β 0.03–0.3 | faster |
| Attack (10 periods), lowest / highest string | 153 / 45 ms | 76 / 23 ms | x0.5 |
| Bow habit | starts from the string | lands a moving bow, except on the C | |
| Vibrato | 5.3–6.5 Hz, ±10–35 c | 5.3–6.4 Hz, ±12–25 c | about the same; narrower high up |
| Left hand | thumb position, shifts for a fourth | extensions, a fourth under the hand | |
| Sound power, forte | 90 dB (pp 63–70, ff 93–97) | 87 dB (pp 53–68, ff 93–97) | about −3 dB |
| Formants | 230–250, 300–500, 600–900 Hz; 2–3 kHz | 220–240, 350, 600–1200 Hz; 2–2.6 kHz; 3–3.5 kHz | the nasal band widened |

### What the first build taught (2026-11-18)

The first viola (`-inst viola`, `kViolaSpec` in `src/bass.cc`, `tools/bassbody.py -viola`) runs
on the cello's code. Five things did not carry over by scaling, and the violin will meet all of
them:

1. **The plates' stiffness against the air.** With the cello's proportion (back 4 x the top's
   arching) a 41 cm body's plates give way so readily that A0 falls to 0.6 of the rigid box's and
   no f-hole lifts it to 225 Hz.
2. **Which modes rock the bridge.** With the back as stiff as the top, A0 and the main resonance
   land on target, but the main resonance no longer rocks the bridge: both feet rise together. A
   mode at 685 Hz rocked it 65 x as much. The bow drives the bridge sideways, so the notes whose
   second harmonic fell there (D♯4–F♯4) lost their fundamental, by up to 30 dB. With the back at
   2 x the top the main resonance rocks it (the bass foot 1.7 x the treble's) and nothing above
   rocks it more than 3.7 x. The check is `tools/bassbody.py`'s foot columns: the main wood
   resonance must move the two feet unequally. The cello's moves them 2.13 and 0.54.
3. **The microphones' levels.** The top by the bridge, the far sound, the f-hole and the pickup
   scale differently with size: the far sound with the top's area, the top's motion not at all.
   At the cello's levels the viola's top microphone made nearly all of its sound. Each path is
   measured alone on played notes (`PJ_ARCO_FAR`, `_HOLE`, `_PICKUP`, `PJ_MIC_TOP` = 0) and
   brought to the cello's balance (f-hole +8.5, pickup −6.5, top +9.7 dB against the far sound).
4. **The fingerboard's contact on a light string.** On the viola's A (0.7 g/m) it blew up within
   one step; the board's force is now held to what a step can take (the cello's strings stay
   under the limit).
5. **The body's grid at 48 kHz.** It rings to 16 kHz, past the explicit scheme's 15.3 kHz: two
   steps a sample.

The fitted f-holes are 44 cm² for a cut area of about 11 cm² (the cello's: 208 against 42.5). As
heard, A0 is 226 Hz at Q 19 and the main resonance 365 Hz at Q 24, A0 15 dB under it (real
violas 4–6 dB; the cello's model 22). Open points: the slurs into D3–F♯3 (fundamentals below A0)
leave the fundamental more than 25 dB under its harmonics, as the cello's into A2 do; C4–D♯4 keep
a weak fundamental (12–16 dB under the second harmonic) from the dip between A0 and the main
resonance.

**The C string, heard as weak and bimodal (second pass).** Two separate causes:

- *The bow's turns.* The bow kept its full weight while its speed went through zero, so the
  string stuck to the hair, its fundamental fell 15–30 dB, and it came back over 0.3–1 s through
  a double slip ("buaaa" at every turn). It came from the shared code, so the cello and bass had it
  too. Now the weight eases with the speed through a turn, down to 0.3 of the full weight
  (`PJ_ARCO_TURN_FLOOR`), and the fundamental dips about 5 dB for about 60 ms. The weight must
  stay light while the bow speeds up again: full weight on the new side is worse than none at
  all. A faster turn (`PJ_ARCO_TURN`, 15 ms) does not help.
- *The body's balance.* As heard, the 480–590 Hz group (A1 at 483, a bass-side top mode at 575
  that rocks the bridge 2 x as much as the main resonance and pumps the f-holes) stands 6 dB over
  the main resonance and 20 dB over A0. The C string's fourth harmonic lands there (C3: h4
  27–30 dB over h1, h2 and h3 only 12–16), and h2 falls in the A0–main-resonance dip (−27 dB).
  The cello's strongest region is its main resonance itself. A radiation estimate in
  `bassbody.py`'s terms (rocking drive × net volume per mode, Q 20) matches the simulation within
  3 dB. Varying the back's arching (7.8–13), the post's place, the bar's height, the f-holes'
  depth, the top's thickness and the bridge's place by one node each moves A0 against that group
  by no more than 2 dB. A0 stays at −19 to −22 dB. A richer C string needs a body whose A0 and
  main resonance carry more net volume under the rocking bridge: a design change, not a knob.

**Against recordings (third pass, 2026-11-19).** Twelve real violas (~/Music/viola*.wav; the
player prefers viola4) agree: 247–440 Hz is a plateau within a few dB of their strongest band,
and 494–700 Hz sits 5–8 dB lower. Each recording's response comes from the harmonic levels of
its notes, with the bowed string's 1/n slope undone. The first two bodies were the reverse.
- *Body.* Fitted to viola4 (`tools/bassbody.py` VIOLA_P, three body steps a sample). 220–392 Hz
  is now within 1–7 dB of it, where the first body was 10–16 dB short.
- *Shine.* The bow drove nothing above 3.8 kHz (`PJ_BOW_WMAX` 0.5), so the model sat 30–45 dB
  under the recordings at 8–16 kHz. The viola now drives to 6.9 kHz (`BassSpec::bowWmax` 0.9),
  back to 0.5 for fundamentals of 400–800 Hz: driven further, those play 15–35 c flat.
  Above 4 kHz the fingertip takes more (`PJ_FINGER_HI`), so open strings shine more than stopped
  ones. The C string's 64 modes end at 8.4 kHz.
- *Onset.* Notes reach full sound in tens of ms, not 100–200 (`BassSpec::attackScale` 0.2,
  `PJ_ARCO_ATTACK`). It is also cleaner: less non-harmonic sound in the first 120 ms.
- *Spectral tuning.* A stopped note goes where one of its first six overtones meets an open
  string's, within 15 c of equal temperament (`PJ_RING_TUNE`). The open strings stay in pure
  fifths.
- *Behind the bridge.* Those string lengths rang with Q up to ~900. Now Q ~40 (`PJ_AFTER_Q`):
  passive, taking energy from the bridge and giving it back within a few dozen cycles. That
  removed a 3.9 kHz whistle on E5.
- *Still open.* Fixed narrow peaks at ~2960, 4120 and 4930 Hz ring at onsets (E♭5, B♭4, F5).
  They survive switching off the open strings, the parts behind the bridge, the tailpiece, the
  bridge's own resonance, the fingerboard and the string's twist.

### Toward the violin

The same table, one more step, needs little new research. The violin's numbers already appear
above for comparison:
- G3 D4 A4 E5 on 32.8 cm;
- A0 270–290, CBR 380–420, B1− 430–480, B1+ 510–580 Hz, sitting *on* the open D and A;
- a 2 g bridge, hill 2–3 kHz;
- a 60 g bow, hair 65 cm, bounce 13–14.7 Hz;
- vibrato 5.5–6.6 Hz, up to ±30 c;
- forte 89 dB.

So the code should take body size, bridge type and the hold as parameters, not as cello or viola
special cases.

---

## 8. Not found (no reliable source)

- **Strings:** measured stiffness, inharmonicity, damping coefficients and torsion of any viola
  string; Thomastik's and Pirastro's own tension sheets (Thomastik returned 403; several sets come
  from a forum compilation).
- **Body:** a peer-reviewed table of signature modes across many violas (VIOCADEAS covers violins;
  the octet paper covers the 20" alto only); Q, damping or effective mass of any viola mode; air
  volume, f-hole area, plate and total masses from a citable source.
- **Bridge:** viola bridge mass; its resonances clamped or mounted; a viola bridge-hill frequency
  as a number (Pérez Carrillo et al. show it only in figures).
- **Meyer's text:** his viola formants, directivity figures and "nasal" band, all second-hand
  here. Langhoff 1995 (modal analysis of violin, viola and cello): cited, not obtained. Dünnwald,
  Meinel, Schleske, Fritz and Woodhouse on violas specifically.
- **Sound:** the viola's long-time-average slope as a number beyond "−15 dB/octave above 3 kHz";
  the CBR beyond forum identifications; a systematic study of viola wolves.
- **Player:** the newton figures in Schoonderwaldt's JASA paper (only the thesis summary was read);
  viola bow hair length, hair count, tension, stick modes and bounce (dealer figures and
  estimates); any viola Guettler diagram or measured attack; pizzicato decay times; tremolo and
  sautillé rates; shift and portamento durations; a vibrato study of professional violists in Hz
  and cents, its onset delay (MacLeod's means: the paper was blocked).
- **Holding:** damping of viola or violin modes by chin, collarbone, shoulder rest or hand in dB or
  Q.
- **Ensemble:** viola-against-violin balance in an orchestra; section timing or pitch scatter;
  old-against-new viola listening tests.

Worth a manual download:
- Schoonderwaldt, JASA 126, 2709 (2009): the violists' force tables.
- MacLeod, JRME 56, 43 (2008): vibrato means.
- Langhoff 1995.
- The TU Berlin directivity database for the viola [Shabtai17].
- Hutchins' JAVS viola article, which is downloadable (read here: [Hutchins88]).
- Meyer's book.

---

## 9. References

Keys shared with `cello.md` (FR91, SSI13–SSI18, Euph9.4, KnutStr, KnutBow, KnutSculpt, KnutOnset,
BowNotes, GuettlerThesis, GuettlerCreation, GuettlerAskenfelt, Lampis24, Askenfelt89, Mansour2,
Hu25, Timmers03, Patynen10, Weinzierl18, Rindel14, Shabtai17, Odeon) are listed there. Their URLs:
FR91 https://csclub.uwaterloo.ca/~pbarfuss/The_Physics_of_Musical_Instruments.pdf , SSI
https://logosfoundation.org/kursus/The%20Science%20of%20String%20Instruments.pdf , KnutStr
https://knutsacoustics.com/files/Typical-string-properties.pdf , Patynen10
https://users.aalto.fi/~ktlokki/Publs/patynen_aaua_2010.pdf .

Viola-specific:
- **[Coffey13]** J. Coffey, "The air cavity, f-holes and Helmholtz resonance of a violin or viola",
  2013. https://www.lisafea.com/pdf/JMC-Helmholtz_Resonance--Re-ct2.pdf
- **[Hutchins88]** C. M. Hutchins, "The acoustics of the viola", J. American Viola Society 4(2),
  1988. https://www.americanviolasociety.org/wp-content/uploads/2022/02/JAVS-4_2.pdf
- **[Powell12]** M. Powell, "Acoustic analysis of the viola", UIUC REU report, 2012.
  https://courses.physics.illinois.edu/phys406/sp2017/NSF_REU_Reports/2012_reu/Meredith_Powell/Meredith_Powell_Final_Paper.pdf
- **[Maestronet]** "Frequency analysis violas", forum thread, 2018.
  https://maestronet.com/forum/index.php?/topic/341662-frequency-analysis-violas/
- **[Strad3D]** https://strad3d.org/st_4.html (violin ranges; snippet)
- **[Smola23]** O. Smola, thesis, Brno University of Technology, 2023 (cites Syrový 2013, p. 294).
  https://dspace.vut.cz/handle/11012/210886
- **[Sengpiel]** https://sengpielaudio.com/VowelDiagram.htm
- **[Reuter24]** Reuter et al., DAGA 2024. https://comma.eecs.qmul.ac.uk/assets/pdf/Reuter_DAGA_2024.pdf
- **[Jansson]** E. Jansson, *Acoustics for violin and guitar makers*, KTH, ch. 3, 8, 9.
  https://www.speech.kth.se/music/acviguit4/
- **[UPF13]** Pérez Carrillo et al., "Digital modeling of bridge driving-point admittances", SMAC
  2013. http://mtg.upf.edu/system/files/publications/smac_adm_2013_v3.pdf
- **[Larsen]** Larsen string tension charts 2018/19.
  https://www.gewamusicusa.com/catalog/documents/Larsen-String-Tension-Charts-18.pdf
- **[Heath]** "Viola string tensions compared", Violinist.com, 2022 (forum compilation).
  https://www.violinist.com/discussion/thread.cfm?page=6131
- **[L&M]** Long & McQuade, Dominant viola set (retailer).
  https://www.long-mcquade.com/932/Band-Orchestral/Orchestral-Strings/Thomastik/Dominant-Viola-String-Set-4-4.htm
- **[GamutVa]** Gamut Music, viola gut string tensions. https://www.gamutmusic.com/viola-tensions
- **[DAdd]** D'Addario orchestral strings comparison chart.
  https://integration.daddario.com/globalassets/pdfs/orchestral/orchestral-strings-comparison-chart.pdf
- **[Johannsson]** H. Johannsson, viola measurements. https://www.hansjohannsson.com/resources/mesviola.html
- **[Met]** Metropolitan Museum, Hutchins alto violin (snippet).
  https://www.metmuseum.org/art/collection/search/503563
- **[Violinspiration]** https://violinspiration.com/how-much-does-a-violin-weigh/
- **[McLennan, VSA]** snippets: https://newt.phys.unsw.edu.au/music/people/mclennan/McLennanThesisComplete.pdf ,
  https://engweb.swan.ac.uk/~adhikaris/BmUSD06/Presentations/JWoodhouse.pdf
- **[Schoond09T]** E. Schoonderwaldt, *Mechanics and acoustics of violin bowing*, KTH 2009 (ch. 2.6
  and 4 read). https://kth.diva-portal.org/smash/get/diva2:133385/FULLTEXT01.pdf
- **[Schoond09J]** E. Schoonderwaldt, "The player and the bowed string: coordination of bowing
  parameters in violin and viola performance", JASA 126, 2709 (2009). Abstract only.
  https://doi.org/10.1121/1.3203209
- **[MacLeod08]** R. MacLeod, JRME 56(1), 43–54 (2008). Abstract only.
  https://doi.org/10.1177/0022429408323070
- **[Geringer04]** Geringer & Allen, JRME 52, 167 (2004). Abstract only. https://eric.ed.gov/?id=EJ705511
- **[Allen09]** Allen, Geringer & MacLeod, artist-level violin vibrato case study. Snippet.
  https://www.researchgate.net/publication/283122059
- **[Marshall85]** K. D. Marshall, "The influence of the musician on the vibrational behavior of a
  violin", JASA 77 (S1) (1985). Abstract only. https://doi.org/10.1121/1.2022340
- **[Bissinger15]** Bissinger & Mores, JASA 137, EL293 (2015). Abstract only.
  https://doi.org/10.1121/1.4915062
- **[VnComShoulder]** violinist.com, "Science of shoulder rests" (anecdotal).
  https://www.violinist.com/discussion/archive/27298/
- **[Chang18]** Chang, *A guide for playing the viola without a shoulder rest*, DMA, U. South
  Carolina, 2018. Not read. https://scholarcommons.sc.edu/etd/4725
- **[AdA]** Atelier d'Archeterie, "Curve and balance".
  https://www.atelierdarcheterie.com/blog_eng/Articoli/Curveandbalance.html
- **[SoundPost]** https://www.thesoundpost.com/store/bows/viola
- **[WikiVa, WikiVert, WikiVnAc, WikiDE]** Wikipedia: "Viola", "Vertical viola", "Violin acoustics";
  German "Bratsche" (mostly unsourced).
- **[violinist.com wolf]** https://www.violinist.com/discussion/archive/11062
