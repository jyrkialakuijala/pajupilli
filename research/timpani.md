# Timpani (orchestral kettledrums): research

Status: built 2026-11-19 as `src/timpani.cc` (-inst timpani): sections 12.1-12.7 and 12.8 in
part (the far field toward the hall; no classical-pair detector, no stick changes by passage, no
sympathetic ringing between the drums, the pedal retuning at the stroke). GM "Timpani" is
program 47 counting from 0 (48 counting from 1). Before, `src/midifile.cc` sent "timpani" to
the taiko. The existing head model is `src/membrane.h`
(ideal-membrane modes, air mass on the m = 0 modes only, pair splitting, tension stretch,
`DrumAir` for two heads on a shell).

How each number is marked:
- **[T]** read in the primary text (downloaded and read for this document). "Measured" is said
  in words where the text reports a measurement and not a calculation.
- **[S]** from a secondary source, an abstract, a search snippet or a machine summary of a page:
  the primary text was not read.
- **[L]** player lore: a teaching text, clinic handout or article by a timpanist, read in full
  unless also marked [S]. Not a measurement.
- **[E]** my estimate, fit or arithmetic from cited values; not a published value.
- **[M]** from general knowledge, not looked up: check before relying on it.

References are short keys in brackets; the list with URLs is section 15. What could not be found
is in section 14. None of the classic method books (Goodman, Hinger, Friese–Lepak, Hochrainer,
Tafoya, Woud, Haas, Herbert) could be opened: what is said about those schools is [M] or comes
through pupils and articles, and is marked so.

Short answer. A timpani is one Mylar or calf head, 0.19 mm thick, on a 50–82 cm copper bowl of
about 140 litres (26–29 in. drum). The air outside and inside moves the modes with one to five
nodal diameters from the ideal membrane's 1 : 1.34 : 1.66 : 1.98 : 2.29 to 1 : 1.50 : 1.98 :
2.44 : 2.90, a harmonic series 2 : 3 : 4 : 5 : 6 whose fundamental is missing; the pitch heard is
the (1,1) mode. The axisymmetric modes ((0,1), (0,2), (0,3)) are the thump: they radiate as
monopoles and are gone in 0.1–0.4 s. The pitched modes last 1–12 s (T60), shorter the higher the
drum is tuned, and each a different time. The stroke lands a quarter to a third of the way from
the rim to the centre; the centre gives only thump. Everything else is the player: one spot for
nearly all dynamics, a stick chosen per passage and not per note, single-stroke rolls at 8–16
strokes per second that go slower on low slack heads, and fingers on the head when the note ends.

---

## 1. The instrument

### 1.1 Sizes and ranges

- **Standard set of four:** 32, 29, 26, 23 in. (81, 74, 66, 58 cm bowl diameter); a fifth,
  "piccolo", drum of 20 in. (51 cm). The set covers roughly D2 to A3; the 20 in. reaches C4;
  Stravinsky's *Rite* asks for B3 on a piccolo drum **[T]** [Wiki].
- **Ranges by source.** All agree that a drum is good over about a fifth and usable over a sixth
  to an octave:

| Drum | [Gottry] **[L]** | [PASIC03] top note **[L]** | [FPL] **[L]** | Adams via retailer **[S]** | [VSL] **[S]** |
|---|---|---|---|---|---|
| 32 in. | D2–A2 | A2 | C2–A2 (Bb2) | D2–Bb2 | "D drum" 75–80 cm: C2/D2–B2/C3 |
| 29 in. | F2–C3 | C3 | F2–D3 | F2–D3 | "G drum" 65–70 cm: E2/F2–D3/E3 |
| 26 in. | Bb2–F3 | F3 | Bb2–F3 | A2–F3 | "C drum" 60–65 cm: Ab2/Bb2–G3/Ab3 |
| 23 in. | D3–A3 | A3 | C3–A3 | C3–A3 | "A drum" 50–60 cm: C3/D3–Bb3/C#4 |
| 20 in. | F3–C4 | C4 | — | D3–B3 | "B drum" 45–50 cm: G3–C4 |

- **Where a drum sounds best:** the middle to upper part of its range. "The best sound for each
  drum is in the mid-high portion of the range" **[L]** [Gottry]; "help the student choose the
  drum that places the note in the higher part of its range" **[L]** [Jones]; "purest tone in the
  middle of each drum's range; very high notes thin and strained, very low notes woolly and weak"
  **[S]** [VSL]. A tight head is also more articulate and clearer in pitch **[L]** [Tactical].
- **Pedal range:** tension can be changed by at least 3 : 1, more than a sixth in pitch **[T]**
  [FR]. The 73 cm Kolberg drum of [Fleischer08] is usable over an octave according to its maker
  **[T]**.
- **Head and bowl sizes differ:** the head is about 2 in. wider than the bowl (a 23 in. drum
  takes a 25 in. head), standard since 1978 **[T]** [Wiki]. The vibrating radius is the bowl's.
- **Number of drums by period** (this decides which drum plays which key, 12.2):
  one pair until about 1800, tuned a fourth or a fifth apart to tonic and dominant, written
  range F2–F3, parts written transposed as C–G **[S]** [VSL], **[T]** [Wiki]. A pair averaged
  about 28 and 25 in. **[L]** [Kettledrummer]. Beethoven first used other intervals (octave F2–F3
  in the Ninth's scherzo) **[T]** [Wiki]; Berlioz several players (16 drums, 10 players in the
  Requiem) **[T]** [FR, Wiki]. "In Romantic and modern works four timpani are usual; in the
  Classical period one pair was standard" **[S]** [VSL]. Three drums in much of the 19th century
  **[M]**. Six or seven is the rare maximum **[S]** [VSL].

### 1.2 The kettle

- **Material:** copper preferred; fibreglass and aluminium on school drums **[T]** [FR, Wiki].
  Wall 0.5 mm **[S]** [VSL]; three 73 cm Kolberg copper kettles measured: mass 12.0, 8.3,
  11.2 kg, height 50, 53, 47 cm, volume 142, 143, 133 litres, one polished and two hammered
  **[T]** [Fleischer08].
- **Volume:** 0.14 m³ for the 26 in. (a = 0.328 m) one drum maker drum of Christian et al. **[T]** [FR].
  For other sizes, scaling with a³: 32 in. 0.26, 29 in. 0.20, 23 in. 0.10, 20 in. 0.065 m³
  **[E]**. The Kolberg 73 cm drum has the same 0.14 m³ as the 66 cm one drum maker, so makers do not
  scale strictly; ±30 % is plausible.
- **Does the volume matter?** Yes, it is close to an optimum. Measured on the 26 in. drum with
  the kettle partly filled with water (ratios to the (1,1) mode) **[T]** [FR, Table 18.3]:

| Mode | V₀ = 0.14 m³ | 0.75 V₀ | 0.5 V₀ | 0.25 V₀ |
|---|---|---|---|---|
| (0,1) | 0.83 | 0.94 | 1.12 | — |
| (2,1) | 1.50 | 1.52 | 1.57 | 1.67 |
| (0,2) | 1.65 | 1.73 | 1.82 | — |
| (3,1) | 1.98 | 2.02 | 2.10 | 2.28 |
| (4,1) | 2.45 | 2.49 | 2.59 | 2.86 |
| (5,1) | 2.91 | 2.96 | 3.08 | 3.43 |

  A smaller kettle raises the axisymmetric modes (stiffer air spring) and lowers the others (the
  inside air must move faster, more mass); the harmonic tuning survives −25 % and is lost at
  −50 %.
- **Shape:** "the effect of kettle shape does not appear to be very significant" (Tubis & Davis
  1986, Davis 1989) **[T]** [FR]. Players and makers say hemispherical bowls sound brighter and
  parabolic ones darker **[T]** [Wiki, citing a maker]; [Fleischer08] found differences between
  three kettles of equal diameter in the decay times but could not explain them systematically
  **[T]**. Treat shape as second order.
- **Does the kettle radiate?** No. Intensity measurements and FE/BE computation: the sound comes
  from the head alone. Kettle-rim resonances (ovalling, 117–151 Hz on the big drum) and stand
  resonances can only take energy: when the octave partial sat on a rim resonance near 147.5 Hz
  its T60 fell from 11 s to 6 s, measured **[T]** [Fleischer08]. So "copper against fibreglass"
  is at most a difference in losses.
- **Kettle air modes** (26 in. one drum maker kettle, rigidly capped, measured): 337, 385, 537, 566,
  655, 747, 816 Hz; all lie above the membrane modes they couple to **[T]** [FR, Fig. 18.3].
- **The vent hole:** 2.6 cm across in the big Kolberg kettle; with rigid walls a Helmholtz
  resonance at 22 Hz, with the yielding head 10 Hz (tuned E2), 13 Hz (A2), 15 Hz (D3), computed
  **[T]** [Fleischer08]. Plugging it: (0,1) from 135.4 to 134.9 Hz, decay 0.29 s both ways,
  measured; timpanists heard no consistent difference **[T]** [FR]. Players do feel it in the
  rebound **[T]** [Fleischer08]. For sound: ignore it. ([VSL] says a closed bowl "would dampen
  the vibration": the measurements say it would not **[S]**.)

### 1.3 The head

- **Mylar (PET):** 0.19 mm (0.0075 in., "7.5 mil") is standard **[T]** [FR]; one head maker's timpani
  heads are one ply of 7.5 mil film, Kolberg lists 190 µm **[S]**. Areal density 0.262 kg/m²
  (0.26 in [Bilbao12]) **[T]**; 0.19 mm × 1390 kg/m³ = 0.264 **[E]**.
- **Tension** (uniform membrane tension, N/m), measured or fitted to measured modes **[T]**:

| Drum | (1,1) pitch | Tension | Source |
|---|---|---|---|
| 26 in., a = 0.328 m | 145 Hz (D3) | 3710 | [FR] Table 18.4 |
| same | 150 Hz | 3990 (calc.) | [FR] Table 18.1 |
| same | 172 Hz (F3) | 5360 | [FR] Table 18.2 |
| 73 cm, a = 0.365 m | 82 Hz (E2) | 1465 | [Fleischer08] Tab. IX |
| same | 110 Hz (A2) | 2668 | [Fleischer08] Tab. VIII |
| same | 147 Hz (D3) | 4873 | [Fleischer08] Tab. X |
| simulation, "typical" | — | 3325 | [Bilbao12] |

- **Tension against pitch [E]:** the five measured pairs fit
  **T = σ_eff (2π a f₁₁ / 3.832)²** with **σ_eff = 0.26 + 1.05 a** kg/m² (a in m) to within 6 %:
  the head's own 0.26 kg/m² plus an air load that is larger than the head itself. Per drum:

| Drum (a) | lowest | T (N/m) | highest | T (N/m) |
|---|---|---|---|---|
| 32 in. (0.406 m) | D2 73.4 Hz | 1640 | A2 110 Hz | 3680 |
| 29 in. (0.368 m) | F2 87.3 Hz | 1790 | C3 130.8 Hz | 4030 |
| 26 in. (0.330 m) | Bb2 116.5 Hz | 2410 | F3 174.6 Hz | 5410 |
| 23 in. (0.292 m) | D3 146.8 Hz | 2800 | A3 220 Hz | 6290 |
| 20 in. (0.254 m) | F3 174.6 Hz | 2790 | C4 261.6 Hz | 6250 |

  A fifth is a tension ratio of 2.25, an octave 4. The total pull on the hoop at 5000 N/m on a
  26 in. drum is 2π a T ≈ 10 kN **[E]**.
- **Calf and goat:** calf was universal until the mid-20th century; one head maker led the change to
  plastic **[T]** [Wiki]. Thickness 0.12–0.17 mm according to [VSL] **[S]**; I found no second
  source and no areal density. Vienna still uses goat on all its timpani **[T]** [mdw]. Calf
  needs less tension than goat for the same pitch **[T]** [Nagl]. Calf heads decay faster overall
  but more evenly across the partials and depend less on the tuning; plastic rings longer and
  unevenly; "calf seems better suited to staccato" (measured, three kettles, D2, A2, D3) **[T]**
  [Fleischer08]. On goat-headed Viennese drums the (1,1) is measurably stronger in the spectrum
  and the sound has fewer inharmonic partials and beats than a plastic-headed pedal drum (laser
  vibrometry) **[T]** [mdw]. A comparison paper by Fleischer & Fastl reports the same direction
  (plastic decays more slowly than calf) **[S]**.
- **Humidity and temperature:** Mylar "is insensitive to humidity" **[T]** [FR]. Skin is not: a
  goat head mounted wet rose from 25 Hz to 96 Hz (two octaves) in 17 h of drying and then held;
  humidity changes act on the pitch at once; calf reacts more strongly than goat in every test
  **[T]** [Nagl]. [VSL] stresses that "the volume and pitch of the timpani are far more heavily
  influenced by the prevailing atmospheric conditions than other instruments" **[S]**: true of
  skin heads; for plastic heads only the residual effects remain (gauges drift with room
  temperature and after transport **[T]** [Wiki]). Humid skin is slack: lower pitch, shorter
  sustain, less level **[M]**. For a model: a plastic-headed drum needs no weather; a calf option
  would be "a few per cent of tension drift over minutes, corrected by the player".
- **Bending stiffness:** raises the modes by about 0.5 % (0.05 % at the (6,1) for 2668 N/m);
  negligible for the modes that matter **[T]** [FR, Fleischer08].
- **Collar:** the part of the head pulled down over the bowl edge. An "extended collar" drum
  takes a head 2 in. over the bowl; the usable range depends on the collar and on the hoop
  profile, and outside it a balanced pedal slips **[S]**. No acoustic measurement found.

### 1.4 Clearing the head

- **What it is:** matching the pitch heard when tapping in front of each of the 6–8 tension rods
  ("lugs"). A felt in the centre isolates each lug's region; opposite lugs are checked in pairs
  **[L]** [Buonviri].
- **What an uncleared head does:** "a single stroke in the normal playing spot will produce beats
  in the sound just like two flutes slightly out of tune" **[L]** [Buonviri]; "if one rod is
  lower the head has a flat spot: when the drum is hit loudly the pitch dips low briefly" **[L]**
  [Adams]. A clear head "maintains a pure pitch as it sustains, with no bending upward or
  downward after the initial attack" **[L]** [Buonviri].
- **Physics:** every m > 0 mode is a degenerate pair (cos mθ, sin mθ). Uneven tension splits the
  pair; a stroke that excites both members beats at their difference (Worland 2010, abstract
  only **[S]**). Measured on a professional drum in good order: the fifth ((2,1)) beat at
  "4 to 5 Hz" at A2 (165 Hz: a split of 2.5–3 %) and more slowly at D2; main tone and octave
  decayed smoothly or beat very slowly **[T]** [Fleischer08]. So some beating in the fifth is
  normal and part of the sound.

---

## 2. Modes

Mode (m, n): m nodal diameters, n nodal circles (the rim counts). Ideal membrane:
f_mn = j_mn / (2π a) · √(T/σ), j₀₁ = 2.405, j₁₁ = 3.832, j₂₁ = 5.136, j₀₂ = 5.520, j₃₁ = 6.380,
j₁₂ = 7.016, j₄₁ = 7.588, j₂₂ = 8.417, j₀₃ = 8.654, j₅₁ = 8.771, j₃₂ = 9.761, j₆₁ = 9.936,
j₁₃ = 10.173.

### 2.1 Ideal against real

26 in. drum, T = 3990 N/m, calculated by the Green-function method, which matches the
measurements of 2.2 within 1–3 % **[T]** [FR, Table 18.1]:

| Mode | Ideal (Hz) | ideal / (1,1) | With kettle (Hz) | ratio | No kettle (Hz) | ratio | σ_eff (kg/m²) **[E]** |
|---|---|---|---|---|---|---|---|
| (0,1) | 143 | 0.63 | 131 | 0.87 | 89 | 0.54 | 0.31 |
| (1,1) | 228 | 1.00 | 150 | 1.00 | 165 | 1.00 | 0.61 |
| (2,1) | 306 | 1.34 | 227 | 1.51 | 237 | 1.44 | 0.48 |
| (0,2) | 328 | 1.44 | 253 | 1.68 | 257 | 1.55 | 0.44 |
| (3,1) | 380 | 1.66 | 299 | 1.99 | 308 | 1.92 | 0.42 |
| (1,2) | 417 | 1.83 | 352 | 2.34 | 343 | 2.08 | 0.37 |
| (4,1) | 452 | 1.98 | 370 | 2.46 | 377 | 2.28 | 0.39 |
| (2,2) | 501 | 2.20 | 411 | 2.74 | 424 | 2.57 | 0.39 |
| (5,1) | 522 | 2.29 | 434 | 2.93 | 445 | 2.69 | 0.38 |
| (3,2) | 581 | 2.55 | 492 | 3.28 | 501 | 3.04 | 0.37 |
| (6,1) | 591 | 2.61 | 507 | 3.38 | 512 | 3.10 | 0.36 |
| (1,3) | 605 | 2.66 | 507 | 3.38 | 525 | 3.18 | 0.37 |
| (4,2) | 658 | 2.89 | 570 | 3.80 | 578 | 3.50 | 0.35 |

The last column is the areal density that would give the "with kettle" frequency in an ideal
membrane, 0.262 × (f_ideal/f)²: the head plus the air riding on that mode. It falls from 0.61 on
the (1,1) to 0.36 on the high modes: this falling air load is what stretches 1.34 to 1.5.

### 2.2 Measured

26 in. one drum maker, Mylar, Christian et al. 1984 **[T]** [FR, Tables 18.2 and 18.4; also Packer]:

| Mode | T = 5360 N/m (Hz) | ratio | T = 3710 N/m (Hz) | ratio | no kettle, 4415 N/m (Hz) | ratio |
|---|---|---|---|---|---|---|
| (0,1) | 140 | 0.81 | 128 | 0.88 | 92 | 0.54 |
| (1,1) | 172 | 1.00 | 145 | 1.00 | 173 | 1.00 |
| (2,1) | 258 | 1.50 | 218 | 1.50 | 253 | 1.47 |
| (0,2) | 284 | 1.65 | 235 | 1.62 | 266 | 1.54 |
| (3,1) | 340 | 1.97 | 287 | 1.98 | 330 | 1.91 |
| (1,2) | 344 | 2.00 | 303 | 2.09 | 365 | 2.12 |
| (4,1) | 420 | 2.44 | 354 | 2.44 | 408 | 2.36 |
| (2,2) | 493 | 2.86 | 394 | 2.72 | 443 | 2.63 |
| (0,3) | 467 | 2.71 | 383 | 2.64 | 459 | 2.66 |
| (5,1) | 501 | 2.91 | 421 | 2.90 | 485 | 2.81 |

Table 18.3 adds (3,2) at 3.27 and (6,1) at 3.37 (556 and 573 Hz at f₁₁ = 170 Hz) **[T]**.

73 cm Kolberg, Mylar, three tunings (measured; computed by FEM/BEM in brackets) **[T]**
[Fleischer08, Tab. VII–X]:

| Mode | E2 (Hz) | ratio | A2 (Hz) | ratio | D3 (Hz) | ratio |
|---|---|---|---|---|---|---|
| (0,1) | 104 (104) | 1.26 | 118 (119) | 1.07 | 135 (134) | 0.92 |
| (1,1) | 82 (82) | 1 | 110 (110) | 1 | 147 (146) | 1 |
| (2,1) | 125 (124) | 1.51 | 165 (167) | 1.50 | 224 (223) | 1.52 |
| (3,1) | 164 (164) | 1.99 | 216 (221) | 1.87* | 296 (296) | 2.02 |
| (4,1) | 204 (204) | 2.47 | 266 (274) | 2.42 | 366 (367) | 2.51 |

*so printed; 216/110 = 1.96. Same head without kettle, 2668 N/m: (0,1) 65 Hz, (1,1) 121 Hz,
(2,1) 174 Hz (1.44), (3,1) 227 Hz (1.88), measured **[T]**.

### 2.3 What the tables say

- **The pitched family (m,1), m = 1..5: 1 : 1.50 : 1.98 : 2.44 : 2.90** on both drums and at
  every tuning, within about 1 % for the first three **[T]**. Against exact 2 : 3 : 4 : 5 : 6
  the (4,1) is 42 cents flat and the (5,1) 59 cents flat **[E]** ("within about half a semitone",
  [FR]). Rayleigh heard fifth, major seventh (1.88) and octave above the principal note **[T]**
  [FR].
- **Missing fundamental:** the series 2 : 3 : 4 : 5 : 6 implies a fundamental an octave below the
  (1,1), but the pitch heard is the (1,1): the upper partials are too weak and short to carry the
  virtual pitch. Some timpanists report a faint tone an octave lower from a gentle stroke with a
  soft stick (Brindle 1970) **[T]** [FR].
- **Why:** (1) air mass, the main cause, lowering the low modes most (up to 40 % by the piston
  estimate); (2) the kettle's air: its stiffness raises the (0,n) modes, its modes fine-tune the
  others; (3) bending stiffness, about +0.5 %; (4) shear stiffness of the film. "Air loading is
  mainly responsible for the harmonic relationship; the other effects only fine tune the
  frequencies but may have considerable effect on the rate of decay" **[T]** [FR].
- **Without a kettle** the head alone already gives 1 : 1.47 : 1.91 : 2.36: "a fairly definite
  sense of pitch". The kettle adds the last few per cent and, as a baffle, lengthens the decay
  **[T]** [FR].
- **Piston estimate of the air mass:** m = (8/3) ρ a³ below f = c/(4π a), a disc of air of the
  head's radius and thickness 8a/3π; falls as 1/f² above, negligible beyond about 500 Hz. For
  a = 0.328 m: 0.113 kg of air against 0.089 kg of film **[T formula, E arithmetic]** [FR]. It
  overestimates below 350 Hz and underestimates above: the kettle is needed in the calculation.
- **The (0,1) mode does not follow the tuning.** Its ratio to the (1,1) goes from 1.26 (E2) to
  0.92 (D3) on the 73 cm drum and 0.88 to 0.81 on the 26 in.: at low tunings it lies *above* the
  principal tone. In Hz it is nearly fixed by the kettle's air spring: all five measured points
  fit **f₀₁ ≈ 65 + 0.46 f₁₁** within 2 Hz **[E, fit to T]**. Fleischer warns that at high tunings
  it can drop into the audible neighbourhood of the main tone and disturb **[T]**.
- **The (0,2) and (0,3)** sit near 1.65 and 2.7; the (1,2) near 2.0–2.1 (it can coincide with
  the octave), the (2,2) near 2.7–2.9 **[T]**.
- **Above 600 Hz** no measured table was found. The membrane has many more modes (a 4 kHz limit
  means some hundreds); they are what a hard stick's "contact sound" is made of.

---

## 3. Decay, radiation, level

### 3.1 Decay times

T60, the time for 60 dB. Amplitude time constant τ = T60/6.91; decay rate 60/T60 dB/s.

26 in. one drum maker, Mylar, measured (Christian et al. 1984) **[T]** [FR, Table 18.4]:

| Mode | radiates as | with kettle, 172 Hz tuning | with kettle, 145 Hz tuning | no kettle, 173 Hz | no kettle, 139 Hz |
|---|---|---|---|---|---|
| (0,1) | monopole | < 0.3 s | 0.4 s | 0.8 s | 1.5 s |
| (1,1) | dipole | 0.8 | 2.3 | 2.5 | 3.4 |
| (2,1) | quadrupole | 1.7 | 3.7 | 3.3 | 3.4 |
| (0,2) | | 0.4 | 0.3 | 0.4 | < 0.3 |
| (3,1) | | 2.7 | 4.6 | 2.6 | 4.6 |
| (1,2) | | 0.5 | 2.5 | < 0.3 | 1.3 |
| (4,1) | | 1.7 | 4.3 | 2.8 | 4.2 |
| (2,2) | | 0.5 | 0.9 | 0.7 | 1.2 |
| (0,3) | | < 0.3 | 0.5 | < 0.3 | < 0.3 |
| (5,1) | | 2.6 | 4.1 | 2.1 | 4.2 |

Calculated from radiation alone at 5360 N/m: (0,1) 0.06 s, (1,1) 0.8, (2,1) 1.8, (0,2) 0.2,
(3,1) 4.6, (1,2) 0.2, (4,1) 10.5, (2,2) 0.8, (0,3) 0.1, (5,1) 16.2 s **[T]** [Packer, quoting
Christian]. The first three agree with the measurement; from the (3,1) up the measured decay is
faster than radiation allows: other losses (film, kettle, stand) take over there.

73 cm Kolberg, Mylar, kettle A, sound level at 3 m in an anechoic room, measured **[T]**
[Fleischer08, Abb. 80–82]:

| Tuning | (0,1) | (1,1) main tone | (2,1) fifth | (3,1) octave |
|---|---|---|---|---|
| D2, 73 Hz | 0.7 s | 3.9 s | 11.8 s | 11.4 s |
| A2, 110 Hz | — | 3.7 s | 8.5 s | 7.2 s |
| D3, 147 Hz | — | 1.0 s | 2.6 s | 5.7 s |

Other measured values in the same report **[T]**: with a calf head near G2 the main tone 2–3 s,
the fifth about 13 s, the octave about 11 s (6 s on the rim resonance). [mdw] gives round
figures "about 1.5 s" for the pitched modes and "0.2–0.3 s" for the rest **[T]**. [VSL]: a
mezzoforte tone "resounds" 4–5 s on a large drum and 3–4 s on a small one **[S]**; teaching
texts say "sustains for several seconds" **[L]** [Gottry].

Rules **[T]** [FR, Fleischer08]:
1. The (m,1) modes decay much more slowly than the (0,n) and the other n > 1 modes.
2. Monopole fastest, dipole next, quadrupole slower: radiated power for a given motion grows as
   f², f⁴, f⁶. Removing the kettle (no baffle) turns the (0,1) into a dipole and the (1,1) into a
   quadrupole: both then last longer.
3. **Tuning up shortens every decay.** [Fleischer08] finds T60 of the main tone falling as about
   1/f² when radiation dominates; the fifth lasts longer than the main tone and the octave as long
   or longer.
4. Calf: shorter overall, more even between partials, less dependent on tuning (1.3).
5. Radiation "accounts for a major part of the damping", but not all: the film's and the
   kettle's losses show in the high modes and at low tunings.

A fit for interpolation **[E]** (to the eight (1,1) and (2,1) points above, plus the (3,1)), with
k = 2π f_mode / 343 and a the head radius:
- (1,1): 1/T60 = 0.25 + 0.83 (ka)⁸. Gives 3.9, 3.1, 1.0 s for the Kolberg (3.9, 3.7, 1.0
  measured) and 1.9, 0.75 s for the one drum maker (2.3, 0.8). Do not use beyond ka = 1.05; hold 0.6 s
  as the floor there.
- (2,1): 1/T60 = 0.083 + 0.036 (ka)⁶ (within 25 % of the five points).
- (3,1): 1/T60 = 0.085 + 0.0045 (ka)⁶ (within a factor 2; the data scatter that much).
- (4,1), (5,1): 4 s at the middle of a drum's range, 2 s at the top **[T, two tunings only]**.
- (0,1): 0.3 s (0.7 s on the largest drum at its lowest note); (0,2), (0,3): 0.3 s; (1,2):
  0.5–2.5 s; (2,2): 0.5–0.9 s **[T]**.
The exponents are fits, not physics: near ka = 1 the simple multipole laws no longer hold.

### 3.2 The sound of one stroke

- Normal spot, 30 ms after the stroke (65 cm drum at E3): partials at the (1,1), (2,1), (3,1),
  (4,1) dominate; a second later the same ones remain. Struck at the centre: the (0,1), (0,2),
  (0,3) dominate at first, "a dull, thumping sound"; after 1 s even then the (1,1) and (2,1) are
  the strongest left **[T]** [FR, Fig. 18.4, after Rossing & Kvistad 1976].
- A soft broad stick favours the first six, nearly harmonic, partials (darker, pitch more
  definite); a small or wooden head brings out the higher inharmonic ones (brighter, more
  incisive) **[S]** [VSL]. A "brighter" stick gives a brighter but still ordered set of partials
  (spectrograph study quoted in a search summary **[S]**).
- Near the player there is much "contact sound"; it does not carry: "what sounded way too hard to
  me in the performance sounded a little too soft and woolly to the microphone" **[L]**
  [PercClinic]. A model heard "from the hall" should have less stick noise than one at the drum.

### 3.3 Directivity

- Per mode: 2m lobes round the head (40 cm drum in an anechoic room, Fleischer 1988) **[T]**
  [FR, Fig. 18.6]. The (0,1) is a monopole over a baffle; the (1,1) a dipole lying in the plane
  of the head, its axis through the strike point and the centre, with a null straight up and
  along the nodal diameter **[T, E for the geometry]**.
- In an orchestra's playing position (two drums, 73 and 63 cm, A2 and E3, anechoic room,
  22 microphones): "the overall radiation is divided mainly into both sides at different
  elevation levels"; the A2 drum on the player's left radiated most at 100–150 Hz to that side.
  The authors call their large-percussion data approximate (microphones too near) **[T]**
  [Patynen10]. A Chalmers directivity set left the timpani out for want of single notes **[S]**.
- For a model: a per-mode source (monopole, horizontal dipole, quadrupole) is the right
  structure; in a hall the reflections fill the nulls. No measured on-axis/off-axis level
  differences in dB were found.

### 3.4 Level

- Pressure pulses from a timpani stroke at a horn bell placed close by: of the order of 100 Pa
  (130 dB) and "sometimes exceeding 1 kPa (150 dB)"; strokes "up to mf" gave 125 dB at the bell,
  measured, near field **[T]** [Chen14]. These are peak pressures a metre or less from the head,
  not hall levels.
- A level chart (University of Michigan) lists "timpani and bass drum" at 85–114 dB **[S]**.
- **"The loudest of all orchestra instruments"** [VSL] **[S]**: not supported by what I could
  read. [FR] gives the *bass drum* as "capable of radiating the most power of all the instruments
  in the orchestra" (20 W peak, Sivian et al. 1931) **[T]**. Meyer's and Weinzierl's sound-power
  tables, which would settle it, could not be opened. Treat the claim as lore: the timpani is
  among the loudest, and certainly able to cover the orchestra ([Firth]: "the part may say forte,
  but if you play too loudly you will overpower the strings"; "rolls sound louder than single
  beats"; "lower pitched drums sound louder" **[L]** [Peters]).
- Mode amplitude in real playing was not found (the mdw table of (1,1) amplitudes in nanometres
  is a figure I could not read).

---

## 4. Mallet and stroke

### 4.1 The stick

- **Parts:** shaft, core, covering. Shafts of bamboo (tonkin), hickory, maple, ash, ebony,
  cherry, beech, aluminium, carbon fibre; cores of wood, cork, hard felt; covering of piano felt
  in one or more layers ("German" soft felt, "American" harder felt, hard side out for staccato,
  billiard felt for ultra-staccato), flannel, leather, or none (wood) **[T]** [Wiki], **[L]**
  [Araya, Adams], **[S]** [VSL].
- **Sizes:** length 30–38 cm; head 2.5–4.5 cm across (felt), 2.5–5 cm (flannel) **[S]** [VSL];
  catalogue values: head 33–42 mm, shaft 10–12.5 mm (a "French" shaft tapers from 14 to 9 mm),
  whole mallet 31–43 g (Kaufmann 38–43 g, Kolberg 31–38 g, a carbon-shaft series 38–43 g)
  **[S]**, **[L]** [PercClinic]. Two Duff-model sticks: 0.16 lb (73 g; bamboo, wood core,
  cartwheel) and 0.22 lb (100 g; hickory): whether per stick or per pair is not said **[T]**
  [Araya].
- **Types** **[L]** [Peters, Adams, Firth, PercClinic, Companion]: cartwheel or ball (large,
  soft, seamless, for soft rolls with the least stick sound); soft ("roller"); general or medium;
  staccato (hard); ultra-staccato (very hard felt); wood, cork, leather; flannel (French
  repertoire) **[S]** [VSL]; sponge in old scores, now played with felt **[T]** [Wiki]. A school
  should own four pairs: soft, general, staccato, ultra-staccato **[L]** [Adams].
- **Hardness is not loudness:** "you can play any of the mallets fortissimo or pianissimo"
  **[L]** [Companion]. "The faster the rhythm, the harder the stick; the slower the rhythm, the
  larger the stick. Young players use big sticks when a passage is loud: that creates a lot of
  width but not a lot of volume" **[L]** [Adams]. Harder for higher notes, softer for lower
  ("often a harder mallet in the right hand for the higher notes"); a larger head "moves a
  greater amount of timpani head, giving a rounder tone at high volumes"; cork "softens a little
  at loud dynamics but gives a harder, more pingy sound when played softly" **[L]** [PercClinic].
- **Shaft:** a light bamboo shaft "lets the player get a rich tone and better projection"; a
  tapered shaft puts the balance point forward for finger playing **[L]** [Araya, PercClinic].
  No measurement of the shaft's effect was found; mechanically it sets the effective mass at the
  head and how much the hand's grip adds to it during contact **[E]**.

### 4.2 The felt as a spring

- **Simulation values:** mallet mass M = 0.028 kg, force F = K η^α on compression η, with
  K = 1.6 × 10⁸ N/m^α, α = 2.54; head 0.26 kg/m², 3325 N/m **[T]** [Bilbao12]. The law and the
  kettledrum set-up are those of Rhaouti, Chaigne & Joly 1999, which Bilbao cites for the
  mallet–membrane interaction; that these very numbers are Rhaouti's measured values is **[M]**
  (the 1999 paper could not be opened). 28 g is the effective mass at the head: less than the
  whole 31–43 g stick, as expected.
- **Other felts:** no measured force–compression law for a timpani stick was found. Piano hammer
  felt: exponents 2.2–3.5 (used hammers), 1.5–2.8 (new), with hysteresis (Hall & Askenfelt;
  Stulov) **[S]**. For harder timpani felts raise K and lower α towards 2; for bare wood use the
  Hertz exponent 1.5 **[E]**.
- **On a rigid wall** this mallet would give **[E]** (energy balance, η_max =
  [(α+1) M v² / 2K]^(1/(α+1)), contact time 2.70 η_max / v):

| Mallet speed (m/s) | 0.5 | 1 | 2 | 4 | 6 | 8 | 10 |
|---|---|---|---|---|---|---|---|
| Felt compression (mm) | 1.4 | 2.1 | 3.1 | 4.5 | 5.7 | 6.7 | 7.6 |
| Contact time (ms) | 7.5 | 5.6 | 4.1 | 3.0 | 2.6 | 2.3 | 2.0 |
| Peak force (N) | 9 | 24 | 65 | 176 | 315 | 476 | 655 |

- **On the head** the membrane gives way too. Bilbao's simulated force histories (1, 5, 10 m/s)
  last several milliseconds on a 12 ms axis, with peaks up to about 150 N at 10 m/s, and the
  contact shortens as the speed rises **[T, read from the figure's description, not digitised]**.
  So at ff the head, not the felt, limits the force (150 N against 655 N on a wall) **[E]**. The
  first reflection from the nearest rim comes back after 2d/c: with d = 8–10 cm and
  c = 65–120 m/s, 1.5–3 ms **[E]**; the mallet leaves when the head throws it back.
- **Spectrum:** a force pulse of length τ has little energy above about 1.5/τ: 8 ms → 190 Hz,
  4 ms → 375 Hz, 2 ms → 750 Hz **[E]**. Because α > 1 the felt is stiffer in a hard blow, so a
  louder stroke with the same stick is brighter ("the drums tend to sound brighter when you are
  playing louder; to keep a dark tone stay relaxed or change to a softer mallet" **[L]**
  [Tactical]).
- **Grip:** a loose hand lets the stick rebound freely; a tight grip adds the hand's mass and
  damping to the contact: "a tighter grip and a snappy, quick stroke produces a brighter, more
  pointed sound"; "squeezing the mallets too tightly will actually dampen some of the sustain"
  **[L]** [Tactical, Jones]. No measurement found.

### 4.3 Strike position

- **The normal spot,** by source: "about one-fourth of the way from the edge to the centre"
  **[T]** [FR]; "1/3 of the way from the edge of the bowl to the centre" **[L]** [PASIC03,
  Gottry, Graham]; "2 to 4 in. from the rim" [Peters]; "about 3 in." [FPL, KSU, YamahaPed];
  "about 4 in." [Wiki, Tactical, Companion]; "3 to 4.5 in. from the counterhoop" [PAS-ECV];
  "between two and five inches" [Firth]; "10 cm from the rim" on a 73 cm drum [Fleischer08]
  **[T]**; "a hand's width" [VSL] **[S]**. It scales with the drum: a little over 3 in. on the
  32, nearer 2 in. on the 23 **[L]** [FPL]. As a fraction of the radius from the centre:
  r/a = 0.67–0.77, most often 0.72–0.75 **[E]**.
- **Where on the circle:** directly in front of the player, above the pedal; both sticks on the
  same ring and a few inches apart (6 in. says [KSU]) **[L]**. The best spot is found by ear and
  can differ with the head: the "primary playing channel" is the pair of lugs either side of the
  spot and the pair opposite **[L]** [Substack2].
- **Centre:** "dull and thuddy", "like tom-toms or cardboard boxes" [FPL]; "a sharp, dead sound"
  [Buonviri]; "almost completely devoid of tone" (Gershwin, *An American in Paris*) [Wiki]; "no
  clear pitch, sounds like a drum" [VSL] **[S]**. Beyond 6 in. from the rim the tone is already
  "dead and thuddy" **[L]** [YamahaPed]; beyond 5 in. "more percussive, pitch clarity suffers"
  **[L]** [Firth].
- **Edge:** "thin and tinny", "pingy" [FPL, Buonviri, Tactical]; "the timpani's sul ponticello:
  the fundamental is still present, with a sharper character" **[L]** [Holmes]; used by Bartók,
  Kodály, Bernstein **[T]** [Wiki]; Elgar's *Enigma* XIII, snare sticks near the edge, pp **[L]**
  [Kettledrummer].
- **Why (ideal-membrane shapes) [E]:** a point stroke gives mode (m,n) an energy proportional to
  J_m(j_mn r/a)² over its modal mass. In dB relative to the (1,1) struck at r/a = 0.75:

| r/a (from centre) | (0,1) | (1,1) | (2,1) | (0,2) | (3,1) | (1,2) | (4,1) | (2,2) | (0,3) | (5,1) |
|---|---|---|---|---|---|---|---|---|---|---|
| 0 (centre) | +3.1 | −∞ | −∞ | +6.8 | −∞ | −∞ | −∞ | −∞ | +8.7 | −∞ |
| 0.30 | +1.9 | +2.0 | −2.5 | −0.7 | −7.8 | +5.9 | −13.6 | +4.8 | −11.7 | −19.6 |
| 0.50 | −0.4 | +3.6 | +2.9 | −8.7 | +1.6 | −6.6 | −0.2 | +1.5 | −0.2 | −2.3 |
| 0.62 (5 in. on a 26) | −2.6 | +2.7 | +3.5 | −1.9 | +3.5 | −3.7 | +3.2 | −19.3 | −16.8 | +2.5 |
| 0.67 (one third) | −3.8 | +2.0 | +3.2 | −1.2 | +3.7 | −0.4 | +3.7 | −4.7 | −12.9 | +3.5 |
| 0.70 (4 in.) | −4.7 | +1.3 | +2.8 | −1.2 | +3.5 | +0.9 | +3.9 | −1.4 | −6.9 | +3.9 |
| 0.75 (one quarter) | −6.3 | 0 | +1.8 | −1.6 | +2.9 | +1.6 | +3.6 | +1.0 | −3.0 | +4.0 |
| 0.77 (3 in.) | −7.1 | −0.6 | +1.3 | −1.9 | +2.5 | +1.6 | +3.3 | +1.5 | −2.3 | +3.9 |
| 0.85 (2 in.) | −11.0 | −4.2 | −1.9 | −4.6 | −0.3 | −0.1 | +0.9 | +0.8 | −2.2 | +1.9 |
| 0.92 (1 in.) | −16.6 | −9.6 | −7.2 | −9.6 | −5.4 | −4.7 | −4.0 | −3.3 | −6.2 | −2.8 |

  Read: the centre feeds only the (0,n) thumps. From 0.62 to 0.77 the (1,1)..(5,1) are all
  within 4 dB of each other and the (0,1) falls from −3 to −7 dB: this is the "warm" band, and
  moving outward inside it trades thump for clarity. At 0.85 and beyond everything loses level
  but the low modes lose most, so the high partials stand out: thin and pingy. The spot one third
  in is also where the (2,2) and (0,3), both inharmonic, are weakest. The real, air-loaded mode
  shapes differ a little and the stick's 2–3 cm contact patch averages the pattern for modes
  above about 1 kHz **[E]**.
- [Fleischer08] remarks that stepping the point radially could find an optimum where the wanted
  partials are strongest and the unwanted weakest; he did not publish one **[T]**.

### 4.4 Grips and strokes

- **German:** palms down, thumbs at the side, wrist stroke; "provides power, sacrifices the speed
  of the fingers"; "tends to be heavy and boomy". **French:** thumbs up, stick moved by the
  fingers; "less power, softer touch", "a lighter sound", more room to lift. **American:** palms
  at 45°, wrist and fingers; general purpose **[L]** [Peters, PASIC03, Buonviri], **[S]**
  [Instrumentalist14]. "French can be more conducive to legato playing, German to staccato;
  however, you can achieve these styles with both grips" **[L]** [PASIC03]. One handout: French
  for general playing and some rolls, German for sixteenths and syncopated rhythms **[S]**.
- **Schools [M]:** Goodman (New York) and his pupils (Firth, Hinger) played thumbs-up; the
  Leipzig/Dresden tradition and Duff's teachers German; Vienna (Hochrainer) a German grip on
  goat-skin hand-lever drums with flannel and felt sticks. Not checked against the method books.
- **Lift:** timpani need "substantially more upstroke" than a snare drum: the head rebounds less
  and the felt must leave the head at once ("avoid excess contact between the mallet felt and
  drumhead") **[L]** [Buonviri]. "You don't actually pull the sound out of the drums, but it is a
  helpful image; most of the work is done through the velocity of the stroke" **[L]** [PASIC03].
  Exercise in the Duff line: return the stick to vertical after every stroke, "to ensure a
  consistent lift stroke and avoid inadvertent downstrokes" **[L]** [Instrumentalist20, read in
  part]. Duff's "lifting" stroke as he described it himself: not found.
- **Legato stroke:** wrist and fingers throw the stick, which rebounds freely; "essentially
  dropping the mallet onto the head", warm and round. **Staccato stroke:** more grip pressure,
  "a very fast stroke with a quick rebound", like a snare down-stroke, wrist in control; brighter
  **[L]** [PASIC03], **[S]** [SecondConductor]. Physically: the grip changes the contact time and
  the effective mass (4.2), and a stick held into the head damps what it has just excited **[E]**.

---

## 5. Things the ideal picture leaves out

### 5.1 Pitch glide

- **No measurement found** of a timpani's pitch against time after a hard stroke.
- **Lore:** "when a drum is played loudly at the top of its range the pitch will be flat, and
  fortissimo at the bottom of its range it will go sharp"; "loud → generally sharper"; hence
  "floating the pedal": keeping the pedal unlocked and correcting by foot while playing **[L]**
  [Substack2, Substack3]. "If one rod is lower, when the drum is hit loudly the pitch dips low
  briefly" **[L]** [Adams]. A clear head shows "no bending upward or downward after the attack"
  **[L]** [Buonviri].
- **Estimate [E]:** the film stretches when the head is deflected. For the (1,1) shape with peak
  deflection w, the area strain averaged over the head is 16 w² (w in m, a = 0.33 m). With
  E h ≈ 0.19 mm × 4.5 GPa = 8.5 × 10⁵ N/m and ν = 0.3, the tension rises by
  E h / (2 (1−ν)) × strain: w = 5 mm → 240 N/m at the peak, about 120 N/m averaged over a cycle,
  3 % of 3700 N/m, +1.6 % in frequency: **about +25 cents at the start of a very hard stroke,
  falling with the square of the amplitude** (so twice as fast as the level). At 1 mm: 1 cent.
  At a low tuning (1500 N/m) the same deflection gives +60 cents: the lore's "sharp at the bottom
  when loud". The "flat at the top when loud" is then not the film: it must be the mechanism
  (hoop, pedal linkage or a slipping balanced pedal giving way under the stroke) **[E]**.
  `HeadSpec::stretch` (E h / (1−ν²)) already models the film part.

### 5.2 Sympathetic ringing

- A struck drum makes the others ring; players muffle unused drums ("sympathetic vibration
  occurs when the heads are not muffled and any sound makes the drum vibrate"; bean bags or sand
  bags on drums not in use) **[L]** [Peters]; "you may need to dampen more than one drum, even
  drums that you haven't been playing" **[L]** [Tactical]. Carter's *Eight Pieces* uses the
  effect **[T]** [Wiki]. Tuning by humming into the head is the same thing (9.2).
- **No measurement of the coupling between two drums was found.** Coincidences to expect **[E]**:
  two drums a fifth apart share the lower one's (2,1) with the upper one's (1,1); a fourth apart
  the lower's (3,1) is the upper's (2,1) (2 f = 1.5 × 1.33 f); an octave apart the lower's (3,1)
  is the upper's (1,1). So the usual tonic–dominant tunings are exactly the ones that ring.
- The whole orchestra also drives the heads (and a horn bell nearby is driven by the drum: gains
  of 16–26 dB from bell to mouthpiece, enough to upset the player's lips **[T]** [Chen14]).

---

## 6. How dynamics are made

### 6.1 What the sources say

- **Height and speed:** "most of the work is done through the velocity of the stroke" **[L]**
  [PASIC03]; aspects of a stroke are "velocity, height, weight" **[L]** [PAS-ECV]; a full stroke
  starts with the stick about 12 in. up and is tossed and released **[L]** [YamahaPed]; soft
  playing keeps "stick heights to a minimum" **[L]**. The arm "may assist but not govern" at loud
  levels; rolls are finger strokes, with wrists added when loud **[L]** [PASIC03].
- **Loud is not hard:** see 4.1. For loud tutti rhythm a general stick; for a loud exposed fast
  figure a hard stick; for the loudest smooth rolls (Berlioz Requiem) a medium felt with an extra
  large head, "to produce as loud a sound as possible without too much contact sound" **[L]**
  [Adams, PercClinic]. For fortissimo single notes, two sticks in unison on one drum (Mahler)
  **[S]** [VSL].
- **Written dynamics are not literal:** "you cannot take all dynamics literally or you will be
  playing too loudly", above all in Classical works; long loud rolls: "I play the entrance as
  written, then immediately drop to piano, and crescendo near the end" **[L]** [Firth]. "Rolls
  sound louder than single beats, so play rolls softer; lower pitched drums sound louder, so
  compensate" **[L]** [Peters].
- **fp and sfz:** fortepiano roll = one loud stroke, let it ring "a second", then enter with a
  soft roll and grow; sforzando = stay loud slightly longer, perhaps three loud strokes **[L]**
  [Peters, Tactical].

### 6.2 Does the playing spot move with the dynamic?

This was asked for explicitly; the evidence is thin and I report it as it is.

- **Mainstream teaching: no.** "It is easy to produce a great tone throughout the majority of
  dynamic ranges in this single playing area" (one third in) **[L]** [Graham]. "All playing and
  tuning should take place in the same spot" **[L]** [Gottry]. The rule "softer → nearer the
  edge" that turns up in searches is written about the *snare drum* (Graham's snare paragraph; a
  2022 audition sheet's snare solo): I checked both texts **[T]**.
- **one stick maker (Boston SO), the one first-hand statement found [L] [Firth]:** the warm band is
  2 to 5 in. from the rim; "a hard stick will have more penetration playing deeper on the head,
  and a soft stick will reveal fewer strokes when playing closer to the rim"; for "very soft
  sostenuto rolls, use a cartwheel type stick and move around the head looking for the least
  amount of stick sound".
- **Timbre, not level, is what the position is used for:** 3 in. "rings and has a bright timbre
  that projects well"; 4 in. is "slightly darker" **[L]** [YamahaPed]. A handout quoted in a
  search summary says moving toward the centre adds articulation and toward the edge a thinner,
  smoother sound **[S]**. Composers ask for "centre" and "edge" as effects (4.3).
- **What follows [E]:** within the band (r/a 0.62–0.80) a player shifts by a centimetre or two:
  outward for soft sustained playing and soft rolls (less attack, fewer audible strokes), inward
  for loud or hard-stick articulate playing (more of the low modes, more "weight", a shorter
  thicker note). By the table of 4.3 a move from 0.75 to 0.80 costs the (1,1) and the (0,1)
  about 2 dB each while the (5,1) loses under 1 dB; a move from 0.75 to 0.68 gains the (1,1)
  1.7 dB and the (0,1) 2.1 dB with the high modes unchanged. These are small, audible as colour, and a model should not exaggerate them.
- **Not found:** any statement by Goodman, Hinger, Duff, Friese, Hochrainer, Tafoya, Woud, Haas
  or Herbert on moving the spot with the dynamic. That schools disagree on this cannot be shown
  from what I could read.

### 6.3 Dynamic by dynamic (lore where marked, otherwise my reading)

| | Stroke | Stick | Spot | Sound |
|---|---|---|---|---|
| pp | fingers, 2–5 cm lift **[E]** | soft or cartwheel; hard felt or wood only for a dry effect | outer part of the band, 2–3 in. **[L]** [Firth] | (1,1) and (2,1) only; no audible attack |
| p | fingers and wrist | soft to general | normal | round; little above 300 Hz |
| mp–mf | wrist, 10–20 cm | general | normal, 3–4 in. | the "standard" tone, rings 3–5 s **[S]** [VSL] |
| f | wrist and forearm, about 30 cm (12 in. full stroke **[L]**) | general; staccato if fast | normal to slightly in | brighter (felt stiffens); thump audible |
| ff | forearm added; relaxed grip to stay dark **[L]** [Tactical] | general or larger head, not softer **[L]** [Adams] | normal to 4–5 in. **[L]** [Firth] | pitch starts sharp (5.1); head-limited force |
| fff | arm; two sticks together on single notes **[S]** [VSL] | large medium felt **[L]** [PercClinic] | as ff | the head, not the player, is the limit |

---

## 7. Rolls

- **Single strokes only,** hand to hand. "Because of the sustain of the drum we don't require
  the speed of double strokes"; felt sticks do not bounce; doubles are uneven **[L]**
  [PercClinic, Buonviri, Tactical, Peters], **[T]** [Wiki]. A buzz roll is a rare special effect.
- **Speed against pitch:** slower on large, slack heads, faster on small, tight ones; and on one
  drum faster as it is tuned up. Reasons given: "the heads vibrate slower" [Peters]; "matching
  the vibration pattern of each drum" [Buonviri]; "a tight head doesn't vibrate as long, so you
  need to play faster to keep the roll resonating; playing too fast on a loose head will disrupt
  the resonating of the head" [FPL]; "higher notes on smaller drums don't ring on for as long"
  [PercClinic] **[L]**. The physics agrees with the last two: T60 of the (1,1) falls from about
  4 s to under 1 s across a drum's range (3.1).
- **Speed against dynamic:** faster when louder. "An extremely loud roll requires a fast hand
  speed (this may cancel out the head tension rule)" [PASIC03]; "the louder the roll, the faster,
  particularly on lower notes", because the early decay after a loud stroke is steeper and the
  next stroke must come before the level has dropped [PercClinic]; "soft rolls are generally
  slower than louder rolls" [FPL] **[L]**. It is also expressive: faster for tension, slower for
  calm ("a soft, low final bass note might be played rather slowly and calmly") **[L]**
  [PercClinic, Buonviri].
- **Too fast, too slow:** too fast "chokes" the sound: "you are in essence dampening one stroke
  with the next", "too much contact sound", "the sound will growl"; too slow is heard as a
  rhythm. "Don't try to fill in all of the spaces; let the ring of the drum fill them" **[L]**
  [FPL, PercClinic, Tactical].
- **Numbers.** No measurement of professional roll rates was found. Teaching figures **[L]**:
  thirty-seconds at ♩ = 72 (9.6 strokes/s) as the basic roll, a little faster if single strokes
  are heard [FPL]; thirty-seconds at ♩ = 80 (10.7/s) [FPL]; an exercise that works up to
  thirty-seconds at ♩ = 100 (13.3/s), after which "the roll becomes sextuplets, but the actual
  roll speed will be the same" [YamahaPed]. A forum count: 13 left–right pairs in three beats at
  ♩ = 108, 15.6 strokes/s **[S, anecdote]**. So **about 8 to 16 strokes per second**; far below
  the rate of a snare roll and unrelated to the head's own frequency (73–260 Hz).
- **Phase:** "the smoothest sound is achieved if the mallet strikes the skin when it is already
  moving downwards; only applicable for lower notes, and only for the advanced player" **[L]**
  [PercClinic]. "The vibration patterns are like the rolling sea: any given stroke may be met by
  an unpredictable wave" **[L]** [Buonviri]. A modal model has this for free: a stroke adds to or
  takes from each mode according to its phase, so even perfectly even strokes sound uneven.
- **Where:** the two sticks on the same ring of the head, "a few inches apart", which "activates
  more resonance" [SecondConductor, Jones, Wiki]; unmatched spots are "one of the first things to
  check when rolls sound poor" [Buonviri] **[L]**. Two spots some 10–15 cm apart on the ring are
  20–25° apart in angle: the two hands excite the (m,1) pairs at different orientations, so
  successive strokes do not hit the same nodal pattern **[E]**.
- **Starting:** with a slight accent, "to create an attack" [Jones]; "play your first attack
  louder than the second; the second immediately after, not overpowering the first" [KSU];
  either hand may start; moving to a lower drum the left hand starts, to a higher the right (on
  an American set-up) [Jones, Peters] **[L]**.
- **Ending:** a tied roll ends on a final stroke on the beat; an untied one stops and rings or is
  damped; a "cut-off stroke" is an accented last stroke damped by hand **[S]** [VSL].
- **Measured and unmeasured:** measured rolls are written as sixteenths, unmeasured with three
  slashes (thirty-seconds) or, before the 20th century, "tr" **[S]** [VSL], **[T]** [Wiki]. An
  unmeasured roll is "not rhythmically symmetrical: out of time and in time all at once" **[L]**
  [FPL]. In slow tempi three slashes may be meant literally; the player decides **[M]**.
- **Crescendo rolls:** speed rises with the level; practise eight bars from the softest to the
  loudest and back [FPL] **[L]**.
- **On two drums:** a roll alternating between two pitches (one hand on each), a "double roll"
  with one player rolling on two drums, and a "transition roll" that moves to the next drum
  without a break **[S]** [VSL]; intervals under a third blur into a boom **[T]** [Timbre].
- **Longest:** Bruckner 7 has one roll of over two and a half minutes **[T]** [Wiki].
- **Glissando roll:** pedal moved during the roll, up or down at any level; an unrolled
  ("resonance") glissando works rising and fades falling, because the slackening head loses the
  sound **[S]** [VSL], **[T]** [Timbre].

---

## 8. Damping

- **Why:** "timpani are so resonant that percussionists rarely allow their sound to decay by
  itself" **[L]** [Buonviri2]. "One muffles for duration and articulation": duration is rests,
  caesuras and note ends; articulation is notes bleeding into each other (clashing pitches on
  two drums, harmony changes, cadences) **[L]** [PASIC03, Jones].
- **With what:** the last three fingers (middle, ring, little), spread, while thumb and index
  keep the stick; pads of the fingers, some add the outer palm; "lightly in the exact playing
  spot" **[L]** [PASIC03, PAS-ECV, PercClinic, Buonviri2, Tactical]. "Very lightly with the
  fingertips, not with the full hand" **[L]** [SecondConductor]. Roll or slide the fingers on so
  that the landing makes no slap; a woollen glove with thumb and index cut off for the quietest
  cases; forearm, elbow, even a knee when three to five drums must stop at once **[L]** [FPL,
  PercClinic].
- **Why the playing spot:** it is where the (m,1) modes just struck have their antinode (the
  stroke put them there), so a damper there takes energy from exactly those modes. A hand at the
  centre would touch only the (0,n) modes **[E]**. ([FPL] makes the same point for mutes: a pad
  in the centre lowers the volume but leaves "a fair degree of resonance".)
- **When:** at the written end of the note: "a half note is dampened on the third beat" **[L]**
  [FPL]; "stop the drum from ringing when the note value ends, but don't shorten the note values"
  **[L]** [Peters]. But "dampening is not always necessary when there is a rest; sometimes it
  sounds good to ring through a rest": the player listens to what the others hold and marks the
  part **[L]** [Tactical]; where a trumpet fanfare ends on a whole note and the timpani part
  shows a quarter and rests, the timpanist lets the note ring to match **[L]** [Jones, read in
  part]. So: damp to the length of the harmony, not mechanically to the printed note.
- **How much:** "the volume and speed determine how much we muffle: the slower and louder we
  play, the more we can and should muffle; faster and softer = less muffling" **[L]** [PASIC03].
  Hand muffling "need not always be complete"; a light touch with one finger only shortens the
  sound **[S]** [PAS]. *Secco*: a dry stroke damped at once **[S]** [VSL].
- **Which hand, which drum:** the hand that is free; "simultaneous dampening": one hand strikes
  the new drum as the other stops the old one, practised on two drums a second apart, where the
  clash is obvious; after a passage over all drums, "silence the last drums played first"
  **[L]** [FPL, Jones, Tactical].
- **How fast the sound dies:** not measured anywhere I could read. A hand pressed on is audibly
  complete in about a tenth of a second **[M]**; the residue is the click of the landing and
  whatever the other drums still ring.
- **Mutes** (*coperto*, *con sordino*): a felt or leather pad on the head. A folded felt 6 × 5
  in., a magnet pad at the rim, or gel pads. On the playing ring at 3 or 9 o'clock (the stroke at
  6): "cuts most of the resonance, the pitch is heard well and the rhythms very clearly"; in the
  centre: softer overall, still resonant; opposite the stroke at 12: "very muted, not desirable".
  Used for clear fast rhythms on low notes, for Baroque and Classical works with small
  orchestras, and where the score says so (Berlioz, *Marche au supplice*: muted, wooden sticks)
  **[L]** [FPL]. Unused drums get a pad against sympathetic ringing **[L]** [Peters].
- **Tradition [M]:** on calf heads and with harder sticks the sound was shorter and less damping
  was needed; plastic heads ring longer (3.1), which is one reason modern players damp so much
  and mute for early music.

---

## 9. Tuning

### 9.1 Mechanisms

- **Hand screws (T-handles)** until the late 19th century; a single-handle "machine" drum 1812
  (Cramer, Munich); the first pedal drums in Dresden in the 1870s **[T]** [Wiki], **[L]**
  [Araya].
- **Dresden pedal:** on the player's side, moved by the ankle, held by a ratchet and pawl; the
  clutch is released to move and re-engaged; since the ratchet has fixed steps, a hand
  fine-tuner at the rim does the rest. **Berlin pedal:** a long arm to the far side, moved by the
  whole leg, with a post-and-friction clutch. **Balanced action:** a spring (or hydraulic
  cylinder) balances the head's pull so the pedal stays where it is put ("floating", "like a gas
  pedal: toe down, pitch up"); most common on school drums, pedal in the middle. It slips when
  the head is tuned outside the range the spring was set for **[T]** [Wiki], **[L]** [Peters].
- **Viennese (Hochrainer/Schnellar) drums:** a hand lever, and the kettle is pressed up into the
  head where a pedal drum pulls the hoop down. This changes the handling, "but contributes in no
  way to the very special sound" (which is the goat skin) **[T]** [mdw].
- **Gauges:** a pointer linked to the hoop or the pedal; accurate until the drum is moved or the
  room changes; needed when there is no time to listen **[T]** [Wiki]. "Gauges are a good aid,
  but do not take the place of a good ear" **[L]** [PASIC03].
- **Speed:** a scale step every quarter note up to ♩ = 150 **[S]** [VSL]; composers should leave
  "at least five seconds" for a silent retuning **[T]** [Timbre].

### 9.2 By ear

- Take the pitch from a fork, the orchestra, a pitch pipe (blown very softly, and disliked);
  sing it; set the pedal low; tap once lightly with a finger; glide up to the pitch. "Always
  move the pedal from below the pitch": no slack is left and the drum does not settle flat. (One
  text, [Peters], says the opposite: tune down to the pitch.) Then hum into the head an inch or
  two above the playing spot: "it should sing right back; if it sings back flat or sharp it is
  tuned low or high". Quiet humming is also how pitch is checked in a concert **[L]** [Gottry,
  Buonviri, Adams, Peters].
- Tuning happens during the piece, silently, often for a key the orchestra has not reached:
  "trying to tune an E-flat while the orchestra is in E-flat major is tough; doing it while the
  group is in D major is asking for trouble" **[L]** [Substack2].
- **With the orchestra:** play softer, hear the others, move the pedal until it matches; "an
  ensemble's intonation typically rises at the end of pieces as the volume goes up; keep a foot
  on the pedal" **[L]** [Adams]. Duff, through his pupil: "It is not a sin to play out of tune
  but a sin to stay out of tune" **[L]** [Adams]. "Theoretically, timpani are just-intonation
  instruments": the player places tonic and dominant where the chord is, not on a tempered grid;
  some intervals between drums sound better slightly narrow **[S]** [Substack1]. For a model: the
  timpani should follow the tuning of whatever it plays with; a pure fifth is 2 cents wider than
  tempered, a pure major third 14 cents narrower: small beside the instrument's own ±10 cents of
  human error **[E]**.
- **Historical:** two drums a fourth or a fifth apart, tonic and dominant, the dominant usually
  below; range F2–F3; sometimes the tuning was reversed to fit a key **[S]** [VSL].

### 9.3 Layout

- **American** (also British, French): lowest drum on the player's left, like a keyboard.
  **German** (also Austrian): lowest on the right; the drums in an arc; pedals on the matching
  side **[T]** [Wiki], **[S]** [VSL]. Seen from the hall the order is mirrored, and the player
  sits behind the drums: with the German layout the low drum is on the audience's left **[E]**.
- Standing or on a high swivel stool; seated when there is much pedalling **[L]** [PASIC03].
- The drums are placed so that the sticks fall on each drum's spot without reaching; pulling
  them too close puts the strokes too near the centre **[L]** [YamahaPed, SecondConductor].

---

## 10. Sticking, articulation, repertoire

- **Sticking:** alternate hands wherever possible; shift rather than cross; doubles only when
  the movement between drums needs them, since "doubling can cause an unevenness in sound";
  cross-sticking is avoided ("now avoided unless necessary", the double cross beat was "mainly
  for show") **[L]** [PASIC03, Peters], **[S]** [VSL]. Lead with the hand nearest the next drum
  **[L]** [Peters]. Two hands never strike exactly the same spot: "it is essentially impossible
  to play with both mallets in exactly the same spot; choose symmetrical spots" **[L]**
  [Buonviri]: every other stroke differs a little.
- **Grace notes:** single strokes, "wider than on a snare drum"; flams often with the grace note
  on the beat, as wind players play them; a grace note at another pitch is played on another
  drum, lightly **[L]** [SecondConductor], **[S]** [VSL].
- **Agility:** "it is possible to play the instrument with the same agility as a snare drum; it
  should not have a slow, lethargic feel"; the notes must reach the hall on time, so the player
  plays "aggressively", meaning actively, not loudly **[L]** [Adams]. Rapid scales are uncommon
  because of the resonance **[S]** [VSL].
- **Articulation** is stick, spot and stroke together **[L]** [Firth]: legato (soft stick,
  relaxed lifted stroke, long ring), "common" (the default, between the two), staccato (hard
  stick, firm quick stroke, perhaps a mute or a finger left on the head). A tight head
  articulates more than a slack one: another reason to choose the drum by where the note lies
  **[L]** [Tactical].
- **By repertoire** **[L]** [Firth, Araya, FPL, Wiki], partly **[M]**:
  - Baroque (Bach, Handel) and early Classical: wooden or leather-covered sticks, small drums,
    calf; today often hard felt, with mutes when the orchestra is small.
  - Haydn, Mozart, Beethoven: a pair; hard to medium sticks; clear rhythm. Beethoven's dynamics
    "if played as written will cover important string or woodwind writing" [Firth].
  - Berlioz: the first to name sticks (wood, leather, sponge); chords from several players.
  - Brahms, Bruckner: "a softer, round stick; a full, dark sound; a more legato stroke" [Firth].
  - Mahler, Strauss: four or more drums, two players, extremes of dynamic, two sticks at once.
  - Bartók, Nielsen: pedal glissandi, chromatic bass lines; edge and centre as colours.
  - Stravinsky: "sharp, angular, percussive, like bites of steel": ultra-staccato sticks, an
    "abrupt, stiff, quick" stroke [Firth]; the piccolo drum at B3.
- **Effects:** centre, edge, with the hand or fingers, coins on the head, a cymbal or bowl on
  the head with a pedal glissando, wind instruments blown into the head **[T]** [Wiki, Timbre].

---

## 11. Notation and MIDI

- Bass clef at pitch (parts before about 1800 often written C–G whatever the key) **[S]** [VSL].
- **Range to expect:** D2–A3 (MIDI 38–57) from a set of four; F2–F3 (41–53) in Classical works;
  down to C2 (36) and up to C4 (60) with the outer drums; B3 in the *Rite* **[T]** [Wiki].
  General MIDI gives no range; files do stray outside (bass lines given to the patch) **[M]**.
- **Rolls:** "tr" with a wavy line (older scores) or three tremolo slashes; both mean the same
  **[T]** [Wiki]. Tied rolls run into each other without a new attack; an untied new note under a
  roll is re-attacked **[M]**.
- **What a MIDI file holds for a roll [M] (no source; from how notation programs and sequencers
  behave):** either (a) repeated notes, usually exact thirty-seconds or sixteenth-triplets with
  equal or alternating velocities, because the GM patch decays and the roll had to be written
  out, or because a notation program's playback expanded the tremolo; or (b) one long note where
  "tr" was lost on export and the patch was expected to ring. A long note is equally often a
  single stroke meant to ring. The file alone cannot say which: see 12.6.
- **Note length:** many files give timpani notes the printed length (so a quarter-note stroke
  ends after one beat), others a short fixed length; sustain-pedal messages are rare **[M]**.

---

## 12. What a model needs

Everything here is a proposal. Measured inputs are marked [T]; the rest is [E] unless marked.

### 12.1 Structure

One `Surface` per drum, with the membrane's machinery (mode shapes for the strike and the damping
hand, pairs with a split, stretch) but **not** the ideal membrane's frequencies and not `airMass`
on the m = 0 modes alone. The simplest way that fits `membrane.h`: keep the Bessel shapes, and
give every mode its own effective areal density (head plus air) and its own T60.

- **Effective density per mode** (26 in. drum, from Table 2.1): (0,1) 0.31, (1,1) 0.61,
  (2,1) 0.48, (0,2) 0.44, (3,1) 0.42, (1,2) 0.37, (4,1) 0.39, (2,2) 0.39, (5,1) 0.38,
  (3,2) 0.37, (6,1) 0.36, (1,3) 0.37, (4,2) 0.35 kg/m²; above these 0.26 + 0.09·(600/f)²,
  falling to the bare film. For another radius scale the air part (the excess over 0.26) by
  a / 0.328.
- **Then pin the pitched family** to the measured ratios 1 : 1.50 : 1.98 : 2.44 : 2.90 : 3.37
  (m = 1..6) [T]: the densities above give them to 2 %, and a listener hears 2 % in the fifth.
- **(0,1):** f₀₁ = 65 + 0.46 f₁₁ Hz for the 26 and 29 in. drums [fit to T]; for the others scale
  the 65 Hz by 0.35 / a. (0,2) at 1.65 f₁₁, (0,3) at 2.7 f₁₁ [T].
- **Kettle:** no separate state is needed if the frequencies are pinned. If it is simulated
  (`DrumAir` with one head): volume 0.14 m³ for 26–29 in. [T], spring ρc²/V on the volume swept
  by the m = 0 modes; vent corner 10–15 Hz [T]; the vent itself is inaudible [T].
- **Modes up to** 4 kHz for hard sticks; for soft sticks 1.5 kHz is enough.
- **Pairs:** split 0.2–0.5 % at random for a well-cleared head, up to 3 % on the (2,1) [T: beats
  of 4–5 Hz at 165 Hz]. `HeadSpec::split = 0.004` is in range. Orientation: random per drum,
  fixed for the drum's life.
- **Stretch:** E h / (1 − ν²) = 0.19 mm × 4.5 GPa / 0.91 ≈ 9 × 10⁵ N/m (5.1).

### 12.2 Drums and keys

| Drum | a (m) | Kettle (m³) | Keys (MIDI) | f₁₁ (Hz) | Tension (N/m) |
|---|---|---|---|---|---|
| 32 in. | 0.406 | 0.26 | 36–44 (C2–G#2) | 65–104 | 1300–3290 |
| 29 in. | 0.368 | 0.20 | 45–49 (A2–C#3) | 110–139 | 2850–4520 |
| 26 in. | 0.330 | 0.14 [T] | 50–54 (D3–F#3) | 147–185 | 3830–6080 |
| 23 in. | 0.292 | 0.10 | 55–58 (G3–Bb3) | 196–233 | 4990–7060 |
| 20 in. | 0.254 | 0.065 | 59–64 (B3–E4) | 247–330 | 5570–9950 |

- This puts every key in the middle to upper part of a drum's range (1.1) and never at its slack
  bottom. Below 36 and above 64 keep the outer drums and accept a poor sound, as a real one would
  be.
- **A Classical pair:** if a passage uses only two pitches a fourth or fifth apart inside F2–F3,
  a player has two drums: lower note on the 29 (or 28) in., upper on the 26 (or 25) in., whatever
  the table says. A detector: at most two distinct pitches in the last 30 s. The lower drum then
  plays as low as F2 (slack, woolly, long) and that is authentic.
- **One voice per drum, not per note.** A new stroke on a ringing head adds to its state (it may
  cancel part of it: 7, phase). A different key on the same drum means the pedal moved: glide
  the tension over 150–300 ms *before* the stroke if there was time, or during the ring if the
  file overlaps the notes (a pedal glissando). Pitch bend maps to the pedal: the modes all move
  together, with f₀₁ lagging (2.3).
- **Tuning error:** ±5 cents per retuning, corrected toward the target over the next second or
  two ("floating the pedal") [E].

### 12.3 Decay

T60 per mode from 3.1: the fits for (1,1), (2,1), (3,1); 4 → 2 s across the range for (4,1),
(5,1), (6,1); 0.3 s for (0,n) (0.7 s for the (0,1) on the 32 in. below F2); 0.5–1 s for (1,2),
(2,2); above 600 Hz T60 = 1.0 s·(600/f) down to 0.15 s [E, no data]. Examples from the fits:

| Drum, key | f₁₁ (Hz) | T60 (1,1) | (2,1) | (3,1) |
|---|---|---|---|---|
| 32 in., D2 | 73 | 3.9 s | 10.7 s | 10.8 s |
| 32 in., G#2 | 104 | 2.8 | 5.9 | 6.9 |
| 29 in., A2 | 110 | 3.1 | 6.6 | 7.5 |
| 29 in., C#3 | 139 | 1.3 | 2.7 | 3.5 |
| 26 in., D3 | 147 | 1.8 | 3.5 | 4.4 |
| 26 in., F#3 | 185 | 0.6 (floor) | 1.1 | 1.6 |
| 23 in., G3 | 196 | 0.7 | 1.6 | 2.1 |
| 23 in., Bb3 | 233 | 0.6 (floor) | 0.7 | 1.0 |
| 20 in., B3 | 247 | 0.6 (floor) | 0.9 | 1.3 |

For comparison [T]: Kolberg 73 cm at D2 3.9 / 11.8 / 11.4 s, at D3 1.0 / 2.6 / 5.7 s; one drum maker
26 in. at 145 Hz 2.3 / 3.7 / 4.6 s, at 172 Hz 0.8 / 1.7 / 2.7 s. The small drums are
extrapolated: no decay data exist for the 23 and 20 in. A calf-head option: all pitched modes
2–3 s whatever the tuning [T, one drum].

### 12.4 Mallet

- Mass 0.028 kg, F = K η^α, a contact patch 2.5 cm across (shape() averaged over it).

| Stick | K (N/m^α) | α | Notes |
|---|---|---|---|
| Cartwheel / soft | 4 × 10⁷ | 3.0 | [E]; mass 0.032 |
| General | 1.6 × 10⁸ | 2.54 | [T] [Bilbao12] |
| Staccato | 6 × 10⁸ | 2.3 | [E] |
| Ultra-staccato | 2 × 10⁹ | 2.1 | [E] |
| Wood | 5 × 10⁹ | 1.5 | [E]; mass 0.025; no felt loss |

- Felt hysteresis: a loss of 10–20 % of the stored energy per stroke (piano felt behaves so
  [S]); it shortens the tail of the force pulse.
- Grip: free stick for legato; for staccato add 20–50 g of hand mass and a damper of a few N s/m
  between stick and "hand" [E].
- **Which stick:** by passage, not by note (4.1). Default general. Harder when the same drum is
  struck faster than about 6 times a second or the passage is marked short; softer for rolls and
  for notes that are long and soft; one step harder on the 23 and 20 in. than on the 32 [L].

### 12.5 Velocity

| Dynamic | MIDI velocity | Mallet speed (m/s) [E] | Stick shift | Spot r/a (26 in.: from rim) |
|---|---|---|---|---|
| pp | 1–24 | 0.3–0.7 | one softer if sustained | 0.80 (6.6 cm) |
| p | 25–44 | 0.7–1.3 | — | 0.78 (7.3 cm) |
| mp | 45–62 | 1.3–2.2 | — | 0.76 (7.9 cm) |
| mf | 63–80 | 2.2–3.5 | — | 0.75 (8.3 cm) |
| f | 81–100 | 3.5–5.5 | — | 0.73 (8.9 cm) |
| ff | 101–118 | 5.5–8 | larger head, not softer | 0.71 (9.6 cm) |
| fff | 119–127 | 8–11 | as ff; two sticks on isolated notes | 0.69 (10.2 cm) |

- The speeds are guesses bracketed by the 1–10 m/s of [Bilbao12]'s simulation and a 30 cm full
  stroke; no measurement of timpani stick speed was found.
- The spot column is the mild reading of 6.2 (Firth: soft nearer the rim, hard deeper). It must
  stay inside 0.62–0.80. Add ±1 cm of scatter per stroke and a fixed offset of a few cm round the
  ring between the two hands. "Centre" and "edge" are effects for a controller, not for
  velocity.
- Level compensation: lower drums sound louder for the same stroke [L]; `velgain_data.h` style
  per-key evening should leave some of that in.

### 12.6 Rolls

- **Recognise written-out rolls:** strokes on one drum less than 110 ms apart for more than half
  a second. Play them as a roll: hands alternating between two spots 12 cm apart on the ring,
  legato strokes, soft or general stick, velocities evened to ±1 dB, timing loosened by 5 % so
  it is not a machine gun; keep the file's count of strokes if it lies within 8–16 per second,
  otherwise re-time to the natural rate below.
- **A held key:** by default a single stroke that rings until note-off (11). Offer a switch (a
  controller or a command-line flag) that turns a key held longer than about 0.6 s into a roll;
  with it on, the first stroke comes at note-on at the key's velocity and the roll continues
  5–15 % softer per stroke [L: "first attack louder than the second"].
- **Natural rate [E, inside the lore's 8–16/s]:** rate = 10 · (f₁₁ / 110)^0.4 strokes/s at mf;
  × 0.85 at pp, × 1.2 at ff. D2: 8.5; A2: 10; D3: 11.2; A3: 13.2; C4: 14.1; at ff up to 17.
  A crescendo roll speeds up with the level.
- **fp:** one loud stroke, 0.5–1 s of ring, then the roll from pp [L].
- **End:** last stroke on the next beat if tied to a note; otherwise stop and let ring or damp.

### 12.6b The two hands (Jyrki's guidance, 2026-11-19)

"If timpani is hit repeatedly fast, it is rolled with two hands, the hammers hit in slightly
different positions, with slightly different excitation and tone, making it like pupopupopupo
instead of pupupupupupu." And: "every second hit is a bit different from the every second, if the
same drum (midi key) is hit twice or more in a row."

In the model (src/timpani.cc): a key struck again takes the other hand; another key begins with
the leading hand. The leading hand strikes 2 % of the radius further in, 4 % faster, with the
harder mallet (stiffness x1.3 against x0.77). `PJ_TIMP_HANDS` scales the differences (0: alike).
Measured on A2 at 0.5 s spacing: alternate strokes about 1 dB apart in level and 2 to 4 dB apart
in the share above 400 Hz.

### 12.7 Note-off and damping

- **The hand:** three fingertips on the playing spot. Simplest: `setDamping(x)` ramped from 1 to
  about 30 over 40 ms, so the (1,1) falls 60 dB in roughly 0.1–0.15 s; better: a soft
  spring-damper pad at the spot through `shape()`, which damps the struck modes most and leaves a
  little of the others, as a real hand does. A faint click (a pp stroke of a very soft "mallet")
  at the landing; inaudible in a hall.
- **When:** at note-off, if the key was held at least 200–250 ms (a hand needs that long to get
  there) and no stroke follows on the same drum within 150 ms. Shorter notes ring on: fast
  passages are not damped stroke by stroke ("faster and softer = less muffling") [L]. After the
  last note before a silence longer than about 0.4 s, damp every drum that still rings, the last
  played first, 100–150 ms apart [L].
- **Partial:** a note-off velocity or a controller could choose a light touch (loss × 3–5).
- **Mutes:** a pad on the playing ring 90° from the stroke: loss × 5–10 on the (m,1) modes, the
  pitch still clear [L]. An option for Baroque and Classical files.
- **Sympathetic:** feed each drum's radiated pressure to the other heads at about −35 dB [E, no
  data]; damped drums do not answer. It matters only at the coincidences of 5.2.

### 12.8 Radiation

Per mode: (0,n) as a monopole, (1,1) as a horizontal dipole along the line from the strike point
through the centre (so pointing from the player toward the hall), (2,1) as a quadrupole; higher
modes as omnidirectional with a level set by their radiation efficiency. In a room model the
dipole's two lobes have opposite sign. Less contact sound at the listener than at the drum (3.2).

### 12.8b The hall (Jyrki's guidance, 2026-11-18)

Jyrki: how the timpani connect to the space of the concert hall is very important; they
probably only work in a classical concert hall with proper sound. A studio, a small hall or a
church do not feel right for them. So the timpani are to be designed and judged in a concert
hall, and the room is part of the instrument, not an effect added after.

Why this fits the physics (reasoning added when recording the guidance; the numbers are
computed from the room presets in `src/room.cc`, not measured) **[E]**:

- The principal tones lie at 73–250 Hz. A room has an even, diffuse response only above its
  Schroeder frequency, 2000·sqrt(RT60 / V):

  | Room | Volume | RT60 | Schroeder frequency |
  |---|---|---|---|
  | `studio` | 150 m³ | 0.5 s | 115 Hz |
  | `hall` (a chamber-music hall) | 2500 m³ | 1.8 s | 54 Hz |
  | `church` | 15000 m³ | 4.0 s | 33 Hz |
  | a symphony hall (not a preset yet) | about 18000 m³ | about 2.0 s | 21 Hz |

  In the studio the low drums' principal tones sit at or below that limit: single room modes
  decide how loud each pitch is, so one note booms and its neighbour is weak.
- The (1,1) is a dipole with nulls (3.3, 12.8). Only a room's reflections fill them; a dry
  room leaves the tone dependent on where the listener sits, and close up the stick is heard
  more than the tone (3.2).
- The drum's own decays (1–4 s on the (1,1), up to 12 s on the (2,1), section 3) are of the
  same order as a concert hall's reverberation, so drum and hall ring as one. In the church
  (4 s) the room outlasts the drum: the 8–16 strokes/s of a roll and the timpanist's damping
  are covered. In the studio (0.5 s) the drum is left alone with its thump.
- The existing `hall` preset is a chamber-music hall of 2500 m³. A symphony hall is about
  seven times that volume, with a later first side reflection and usually a bass rise in the
  reverberation time. Built: the `concert` preset in `src/room.cc` (18000 m³, 2.0 s, the
  listener 8 m away; no bass rise yet, the room has one decay time with shorter highs).
- **With -play** (Jyrki, the same day): if a piece uses timpani and an organ, bring the organ
  to a usual concert hall rather than the timpani to a church. Built in `src/midifile.cc`: a
  part on General MIDI program 48 (timpani) puts the piece in `concert`, organ or not; an organ
  without timpani still plays in `church`; `-room` overrides both.

### 12.8c The sound to aim for (pasted by Jyrki, 2026-11-18; source not given) **[L]**

| | A high-quality timpani sound | A normal one |
|---|---|---|
| Pitch | a clear, singing pitch centre: the overtones align into one focused note | clashing overtones: tubby, wobbly or uncertain pitch |
| Bowl and head | thick copper bowl; evenly tensioned calf or synthetic head; warm, long sustain | lighter fibreglass or aluminium shell; uneven head tension; less sustain |
| Technique | struck 1 to 3 inches from the edge, a quick rebound, matched mallets: the head rings freely | struck too near the centre, or with stiff strokes and one pair of mallets for everything: a harsh attack or a flat thud |

What it means for the model, against the sections above:

- **Pitch clarity is the first target:** the (1,1)..(3,1) at 1 : 1.50 : 2.00 within about 1 %
  (section 2), and a cleared head, so no split pairs beating slowly on the principal tone
  ("wobbly"; 5.1). A little beat high up is normal on a good drum (the measured 4–5 Hz on the
  fifth partial).
- **"Tubby"** is the (0,1) thump too strong or too long against the (1,1): strike spot too far
  in, or the axisymmetric modes not damped fast enough (their T60 is under 0.3–0.7 s).
- **Warm, long sustain:** the measured decays of section 3 are the target, on the long side.
  The bowl does not radiate (1, "Does the kettle radiate?"), so "thick copper" can only mean
  fewer losses to rim and stand resonances: model the good drum as the one that takes no
  energy from the head, and do not give the kettle a sound of its own.
- **Spot:** 1–3 in. from the edge is nearer the rim than most texts found (2–5 in., most often
  3–4 in.; section 4 and 12). On a 26 in. drum 1–3 in. is r/a 0.91–0.77: its inner end agrees
  with the measured-mode table, its outer end is where the low modes already fall away. Keep
  the default near r/a 0.75 and let the ear decide whether to go further out.
- **Quick rebound:** the stick must leave before the head comes back, so a contact time well
  under half a period of the (1,1) and no second touch; a stiff stroke that stays on the head
  damps the high modes it has just excited (4).
- **Matched mallets, more than one pair:** stick hardness is a choice per passage (section 6),
  so the model should have several sticks and not derive hardness from velocity alone.

### 12.9 What a listener would notice if it were wrong

1. **Partials at ideal-membrane ratios** (1.34, 1.66): no pitch, a tom or a taiko. The fifth
   and the octave must be within about 1 %.
2. **A strong fundamental an octave below, or a strong (0,1):** a bass drum under the note. The
   (0,1) must be a 0.3 s thump, not a pitch.
3. **All partials decaying together:** a sampled-and-faded sound. The main tone must go first on
   low notes, leaving the fifth and octave humming; high notes must be short and dry.
4. **The same ring time at every pitch:** real drums ring 4–5 s low and 1 s at the top of a
   drum's range; the tone changes when the part moves to the next drum.
5. **No beat in the fifth:** too clean; or beats of several Hz in the main tone: an uncleared
   head.
6. **Rolls as machine-gun repeats, or as a sustained organ tone:** each stroke must be audible
   as a soft swell at 8–16 per second, slower on low notes, with uneven strokes because of the
   head's phase.
7. **Roll rate that does not change with pitch or level.**
8. **Notes that stop dead at note-off with no hand, or never stop:** timpani parts are full of
   rests that must be silent, and of last notes that must ring.
9. **Loud strokes that are only louder:** they must be brighter, start slightly sharp and have
   more thump.
10. **Hard, clicky attacks at the listener's distance:** the stick noise does not carry.
11. **Every stroke identical:** two hands, two spots, a head already moving.
12. **Pitch steps without a glide** when one drum is retuned while ringing (and glides where
    another drum would have been used).

---

## 13. The Vienna Symphonic Library pages

The instrumentology page (history, construction, notation, range, sound production, playing
techniques, sound characteristics, sound combinations) is one page with anchors, drawn by a
script; I could read it only through a summarising fetch, so everything from it is **[S]** and
the wording is not verbatim, except the introduction quoted in the task. Used above for: drum
names and ranges, mallet sizes, the "hand's width" spot, coperto and secco, roll notation,
glissando types, the ring times (4–5 s large drum, 3–4 s small, at mf), the German and American
layouts, retuning speed, and the drums per period.

What it stresses:
- **Weather:** pitch and volume depend on temperature and humidity far more than on other
  instruments. Supported for skin heads (1.3, [Nagl]); for plastic heads [FR] says the opposite.
  The page's own construction section agrees that plastic heads "are unaffected by atmospheric
  changes".
- **Drums by period:** one pair in the Classical period, four in Romantic and modern works: used
  in 1.1 and 12.2.
- **A specialist's instrument:** the part is "generally only entrusted to trained timpanists"
  (also [Timbre]).

Where it differs from the acoustics literature:
- "The loudest of all orchestra instruments": unverified; [FR] names the bass drum (3.4).
- "A closed bowl would dampen the vibration" (the vent): measured otherwise (1.2).
- Bowl "height approx. 80 cm": the measured kettles are 47–53 cm deep [Fleischer08]; 80 cm must
  be the drum on its stand.
- "Vellum 0.12–0.17 mm": no second source; Mylar is 0.19 mm.
- The bowl "is the resonator": it is a baffle and an air load; it does not resonate at the
  played pitches and radiates nothing (1.2).
- "The first six partials are relatively harmonic": matches 2 : 3 : 4 : 5 : 6 with the top two
  half a semitone flat (2.3).
- Sound combinations (not used in the model): unison with bassoon, bass clarinet, cello, bass,
  bass trombone, tuba blends; trumpets doubling tonic and dominant one and two octaves above;
  soft rolls under long trombone notes; with harp and pizzicato basses in piano; bass drum added
  to a stroke for power [Timbre].

---

## 14. Not found, or uncertain

- **Rhaouti, Chaigne & Joly 1999** in the original (mallet measurements, simulated and measured
  waveforms, energy balance). The mallet numbers come from [Bilbao12].
- **Christian et al. 1984** in the original; its tables are read from [FR] and [Packer], which
  agree with each other.
- **Any measured force–compression law, contact time or stick speed for a timpani mallet**, and
  how they vary from pp to ff. The contact times here are computed.
- **Decay data for the 23 and 20 in. drums and for the 32 in.**; modes and decays above 600 Hz.
- **Pitch glide**, measured. The +25 cents is an estimate; the lore about going flat at the top
  of the range is unexplained.
- **Sympathetic coupling between drums**, measured.
- **Sound power** at pp and ff (Meyer; Weinzierl et al. 2018) and Sivian's 1931 figure for the
  timpani; whether the timpani or the bass drum is the loudest.
- **Calf head:** areal density, thickness (one source), loss factor.
- **Kettle depth and volume per size** from makers; the effect of the collar.
- **The method books:** Goodman, Hinger, Friese–Lepak, Hochrainer, Tafoya, Woud, Haas, Herbert.
  Nothing they say about the playing spot against dynamic, roll speed in numbers, or damping
  could be read. The claim that the spot moves with the dynamic rests on **one** first-hand
  source (one stick maker); the mainstream handouts say one spot.
- **Roll rates measured from recordings.** The 8–16 per second is from three teaching texts and
  one forum post.
- **How fast a hand damps the head**, measured.
- **Fleischer's figures** (T60 against tuning for three kettles and two heads, the mdw amplitude
  table): present in the PDFs as plots only; I used the numbers given in the captions and text.
- **MIDI practice** (11, 12.6) is from general knowledge, not from a survey of files. Checking a
  few dozen GM files for how timpani rolls are written would settle the default.

---

## 15. References

- **[FR]** Fletcher & Rossing, *The Physics of Musical Instruments*, 2nd ed., Springer 1998,
  ch. 18 (18.1–18.8). Read in full for these sections.
  https://csclub.uwaterloo.ca/~pbarfuss/The_Physics_of_Musical_Instruments.pdf
  It quotes: Christian, Davis, Tubis, Anderson, Mills & Rossing, "Effects of air loading on
  timpani membrane vibrations", JASA 76, 1336–1345 (1984); Rossing & Kvistad 1976; Rossing,
  "The physics of kettledrums", Sci. Am. 247(5), 172–178 (1982); Benade 1976; Rayleigh 1894;
  Tubis & Davis 1986; Davis 1989 (Purdue thesis, https://docs.lib.purdue.edu/dissertations/AAI8825522);
  Fleischer 1988; Brindle 1970; Sivian, Dunn & White 1931. None of these read in the original.
- **[Rossing92]** Rossing, "Acoustics of drums", Physics Today 45(3), 40 (1992). Same text as
  [FR] on kettledrums. https://aip.brightspotcdn.com/PTO.v45.i3.40_1.online.pdf
- **[Packer]** Packer, *Acoustics of the orchestral kettledrum*, MSc thesis, Durham 1993
  (reproduces Christian's calculated and measured tables).
  https://etheses.durham.ac.uk/id/eprint/5633/1/5633_3049.PDF
- **[Fleischer08]** Fleischer, *Fell, Kessel und Gestell der Orchesterpauke*, Beiträge zur
  Vibro- und Psychoakustik 1/08, Univ. der Bundeswehr München 2008 (German; read in the parts
  cited). https://www.unibw.de/lrt4/veroeffentlichungen/pauke_1_08_endversion.pdf/@@download/file/Pauke_1_08_Endversion.pdf
  Related, not opened: Fleischer 2005 (heads), https://www.rz.unibw-muenchen.de/lrt4/veroeffentlichungen/2005_1_pauke.pdf ;
  Fleischer & Fastl, https://mediatum.ub.tum.de/doc/821241/document.pdf
- **[Bilbao12]** Bilbao, Torin & Chatziioannou, "Numerical modeling of collisions in musical
  instruments", Acta Acustica united with Acustica (2012 preprint), section 7 and Fig. 12.
  https://www.ness.music.ed.ac.uk/wp-content/uploads/2015/01/bilbao_acustica_r2.pdf
- **[Rhaouti99]** Rhaouti, Chaigne & Joly, "Time-domain modeling and numerical simulation of a
  kettledrum", JASA 105, 3545–3562 (1999). Abstract only.
  https://researchportal.ip-paris.fr/en/publications/time-domain-modeling-and-numerical-simulation-of-a-kettledrum/
- **[mdw]** Institut für musikalische Akustik – Wiener Klangstil, "Wiener Pauke".
  https://www.mdw.ac.at/iwk/wiener-pauke/?l=en
- **[Nagl]** Nagl, "Humidity influence on natural drumheads" (extract of a 2020 master's thesis,
  mdw). https://www.mdw.ac.at/iwk/?PageId=187
- **[Patynen10]** Pätynen & Lokki, "Directivities of symphony orchestra instruments", Acta
  Acustica 96, 138–167 (2010), section 6.1.
  https://users.aalto.fi/~ktlokki/Publs/patynen_aaua_2010.pdf
- **[Chen14]** Chen, Smith & Wolfe, "The effect of nearby timpani strokes on horn playing",
  JASA 135, 472–478 (2014). https://www.phys.unsw.edu.au/jw/reprints/timpani.pdf ;
  https://newt.phys.unsw.edu.au/jw/timpani-horn/timpani-horn.html
- **[Worland10]** Worland, "Normal modes of a musical drumhead under non-uniform tension", JASA
  127, 525 (2010). Abstract only.
- **[Papiotis10]** Papiotis & Papaioannou, "Kettle: a real-time model for orchestral timpani",
  SMC 2010 (a sample-based instrument; five drums, three mallets, hit positions).
  https://zenodo.org/records/849781/files/smc_2010_040.pdf
- **[VSL]** Vienna Symphonic Library, Academy, "Timpani". Read through a summarising fetch only.
  https://www.vsl.co.at/en/academy/percussion/timpani
- **[Wiki]** Wikipedia, "Timpani". https://en.wikipedia.org/wiki/Timpani
- **[Timbre]** Timbre and Orchestration Resource, "Percussion: Drums".
  https://timbreandorchestration.org/isfee/extreme-orchestration/percussion/drums
- **[Firth]** Vic Firth, "Developing an Artistic Sound", Percussive Arts Society.
  https://pas.org/?p=30106
- **[Adams]** Timothy Adams Jr. (a Duff pupil), "The Three Aspects of Timpani Playing", The
  Instrumentalist, April 2015. https://theinstrumentalist.com/april-2015/the-three-aspects-of-timpani-playing
- **[Buonviri]** Buonviri, "Five Ways to Improve Timpani Sound", The Instrumentalist, June 2017.
  https://theinstrumentalist.com/june-2017/five-ways-to-improve-timpani-sound/
- **[Buonviri2]** Buonviri, "Muffling Percussion Instruments", The Instrumentalist, Dec. 2018.
  https://theinstrumentalist.com/december-2018/muffling-percussion-instruments
- **[Graham]** Graham, "Proper Playing Areas", The Instrumentalist, Sept. 2017.
  https://music.ubc.ca/wp-content/uploads/sites/47/2022/05/2017-09-01ProperPlayingAreas-AaronGraham-TheInstrumentalist.pdf
- **[Instrumentalist20]** "Consistent Timpani Tone and Articulations", March 2020 (read in part).
  https://theinstrumentalist.com/march-2020/consistent-timpani-tone-and-articulations/
- **[Instrumentalist14]** "3 Timpani Grips", Dec. 2014 (snippet).
  https://theinstrumentalist.com/december-2014/3-timpani-grips/
- **[PASIC03]** McCutchen & Broadway, "Timpani Fundamentals", PASIC 2003 handout.
  https://core-docs.s3.us-east-1.amazonaws.com/documents/asset/uploaded_file/3707/SWHS/3585299/Timpani_Fundamentals.pdf
- **[PAS-ECV]** Percussive Arts Society, timpani warm-up sheet.
  https://pas.org/wp-content/uploads/2024/04/ECV0212-15.pdf
- **[Peters]** Peters, "Timpani", Bandworld 9(5), 1994. https://bandworld.org/pdfs/TimpPeters.pdf
- **[Companion]** "Timpani Companion", Bandworld. https://bandworld.org/pdfs/TimpCompanion.pdf
- **[Gottry]** Gottry, "Timpani Technique".
  https://cachepe2.samedaymusic.com/media/gotry_learn-59cff78b3b50899e6a98bc0c9f3d6c43.pdf
- **[Jones]** Jones, "Percussion Performance: Timpani", SBO Magazine (read in part).
  https://sbomagazine.com/percussion-performance-timpani/
- **[PercClinic]** Percussion Clinic Adelaide, "Timpani technique".
  https://www.percussionclinic.com/art_timptech.htm
- **[FPL]** Proctor, Free Percussion Lessons: playing spot, rolls on low and high notes, rolls
  soft to loud, performing a roll, dampening, mutes.
  https://freepercussionlessons.com/where-is-the-best-playing-spot-on-timpani ;
  https://freepercussionlessons.com/how-to-play-timpani-rolls-on-low-and-high-notes ;
  https://freepercussionlessons.com/how-to-play-timpani-rolls-from-soft-to-loud-and-back-to-soft ;
  https://freepercussionlessons.com/how-to-properly-perform-a-timpani-roll/ ;
  https://freepercussionlessons.com/how-to-dampen-timpani ;
  https://freepercussionlessons.com/how-to-mute-the-timpani
- **[YamahaPed]** Yamaha, "Timpani Teaching Tips".
  https://hub.yamaha.com/music-educators/instruments/perc/timpani-pedagogy
- **[Tactical]** "Tactical Timpani" (school site): striking area, dynamics, articulation, rolls,
  dampening. https://sites.google.com/salineschools.org/bands/tactical-timpani-3/dynamics
- **[SecondConductor]** "The Second Conductor: a clinic on improving timpani performance" (read
  in part). https://pdfcoffee.com/the-second-conductor-a-clinic-on-improving-timpani-performance-pdf-free.html
- **[KSU]** Kennesaw State University, timpani masterclass handout.
  https://kennesaw.edu/arts/academics/music/docs/ksu-masterclass-timpani-techniques.pdf
- **[Substack1, 2, 3]** "Learning Percussion": timpani tuning parts I and II, floating the pedal.
  https://learningpercussion.substack.com/p/a-timpani-tuning-journey-part-i ;
  https://learningpercussion.substack.com/p/timpani-tuning-part-ii-a-clear-well ;
  https://learningpercussion.substack.com/p/timpani-exercises-part-iii-floating
- **[Holmes]** Holmes, "Playing spots on the timpani".
  https://adamholmesmusic.com/blog-playing-spots-on-the-timpani/
- **[Kettledrummer]** "The Timpani". https://www.kettledrummer.com/?p=17
- **[Araya]** Araya, *Timpani mallet choices and how they affect sound and interpretation*, DMA
  document, Univ. of Georgia (read in part).
  https://openscholar.uga.edu/record/2363/files/ArayaSebastian%20pdf.pdf
- **Roll count (forum, snippet only):**
  https://forums.musicplayer.com/topic/53957-question-about-timpani-rolls
- **Mallet and head catalogue values (snippets only):** Kaufmann and Kolberg mallets, Remo
  timpani heads, e.g. https://kolberg.com/en/Wool-felt-timpani-mallets-Rossmann-24/24K3 ;
  https://remo.com/product/ti-series-hazy
- **Not opened, for later:** Weinzierl et al., JASA 144, 1347 (2018),
  https://depositonce.tu-berlin.de/handle/11303/9646 ; Schweizer, *Timpani Tone and the
  Interpretation of Baroque and Classical Music*, Oxford 2010; Goodman, *Modern Method for
  Tympani*; Hinger, *Technique for the Virtuoso Tympanist*; Friese & Lepak, *Timpani Method*;
  Tafoya, *The Working Timpanist's Survival Guide*; Deane, "Pitch Control for Timpani",
  Percussive Notes 42(3), 2004.
