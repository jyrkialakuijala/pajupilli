# The triangle, the cowbell and the agogo bells

Status: 2026-11-16. Researched, planned and built: `-inst triangle` (General MIDI's 80 mute, 81 open)
and `-inst cowbell` (56) in `src/metalperc.h/.cc`; the agogo's partials (67/68, `src/modalinst.cc`)
corrected. They appear in the user's MIDI files: triangle 3100 notes in 9 files, cowbell 2100 in 16,
agogo 340 in 6.

Marks: **[paper]**, **[web]** a source below; **[meas]** measured here on FluidR3's recorded samples
(TiMidity++; a recording of a real instrument, but cut short: its decays are lower bounds); **[calc]**
worked out here; **[memory]** general knowledge, to be checked; **[design]** a choice of this note.

## 1. The triangle

### 1.1 What it is

- **The bar:** a round steel rod (8–12 mm), bent into a triangle with one corner left open. Sides are
  15–25 cm (6–10"), so the rod is 0.45–0.75 m long **[memory]**.
- **Playing:** it hangs from a loop of string (nearly free) and is struck with a steel beater of
  3–6 mm. Strokes land on the inside of the bottom side, or on the outside of a closed corner.
- **The muted triangle (GM 80):** the hand holding the clip clasps the bar and damps it, so the
  stroke is a short "tink". Open (81), it rings for seconds.
- **The samba and baião pattern** alternates the two: the hand closes and opens on every beat
  **[memory]**.

### 1.2 Its modes

- **The kinks matter little to the bending modes.** Rossing found the transverse modes "similar to
  the vibration modes of a straight bar" of the same total length **[paper, R76, as cited in S22]**.
  The free bar's series is f_n = β_n² κ / (2π L²), with β_n L = 4.730, 7.853, 10.996, … ≈ (n + ½)π and
  κ = (d/4) √(E/ρ) for a round rod.
- **Two families.** One bends in the triangle's plane, one across it, where the corners twist and
  stiffen it.
  - The finite-element and measured first modes of a small triangle: **156 Hz in the plane, 162 Hz
    across it ("torsion"), 269 Hz longitudinal** **[paper, S22]**.
  - The modes couple at the kinks; that is the non-linear mode coupling in kinked bars studied by
    Legge **[paper, L87, cited in S22]**.
- **The longitudinal modes:** f = n c / 2L, with c = √(E/ρ) ≈ 5050 m/s for steel. The corners turn
  a stroke across one side into a push along the next.
- **What is heard:**
  - The low bending modes radiate poorly: a thin rod is a weak line dipole (ka ≪ 1) **[calc]**.
  - The sound is the dense, inharmonic shimmer from about 1 to 15 kHz. The recorded triangle has no
    usable partial under 1.2 kHz **[meas]**.
- **The recorded open triangle (key 81)** **[meas]**. Peaks in Hz (level in dB against the strongest,
  T60 in s):

| Peak | 1206 | 1853 | 2166 | 3943 | 4423 | 4753 | 5357 | 5958 | 7853 | 9757 | 10523 | 11208 | 15532 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| dB | −8 | −21 | −2 | −3 | −11 | 0 | −20 | −3 | −17 | −26 | −16 | −4 | −23 |
| T60 | 2.6 | 3.0 | 1.8 | 2.0 | 2.3 | 2.4 | 2.3 | 2.3 | 1.6 | 0.8 | 0.5 | 1.0 | 0.5 |

- **The free bar fits it [calc].** A steel rod 0.57 m long and 10 mm thick (an 8" triangle) gives:

| | | | | | | | |
|---|---|---|---|---|---|---|---|
| Bar mode n | 4 | 5 | 8 | 9 | 11 | 12 | 13 |
| Bar (Hz) | 1232 | 1840 | 4400 | 5490 | 8050 | 9500 | 11 070 |
| Recorded (Hz) | 1206 | 1853 | 4423 | 5357 | 7853 | 9757 | 11 208 |

  - The bar's first longitudinal mode, c/2L = 4428 Hz, matches 4423 too.
  - The peaks between them (2166, 3943, 4753, 5958, 10 523) lie **8–18 % above** their in-plane
    neighbours: the family across the plane, stiffened by the corners.
- **Decay:**
  - Open, about 2–3 s through 1–6 kHz and 0.5–1 s above **[meas; the sample is cut, so a real
    triangle rings longer]**. Steel's own loss is tiny; the air and the string take the energy.
  - Muted, 0.36–1.0 s by band in the recording **[meas]**: the hand takes 2.5–3.5× more.

### 1.3 The beater

- **Mass and contact:** a steel rod, about 20 g effective at its tip. Steel on steel, crossed
  cylinders: Hertz K = (4/3) E* √R ≈ 9·10⁹ N/m^1.5 for R ≈ 4 mm, a contact of tens of µs **[calc]**.
  Its force spectrum is flat to beyond 15 kHz, so a hard stroke hardly changes colour: the level
  carries the dynamics.
- **Where and how:**
  - The middle of the bottom side excites the symmetric modes most. A stroke near a corner brings in
    the others and the longitudinal ones.
  - Players vary the place and the angle a little on every stroke **[memory]**.

## 2. The cowbell

### 2.1 What it is

- **Shape:** a box of sheet iron (1–1.5 mm), tapered, closed at the top and open at the mouth; Latin
  bells are 15–25 cm long **[memory]**.
- **Playing:** struck with a stick's shaft on the mouth's edge (open, ringing) or with the tip on the
  body near the closed end (drier, "cha" against "bong"). It is mounted, or held in the hand, which
  damps it.
- **In GM (56)** it is one sound: the open stroke.

### 2.2 Its modes

- **Literature:** no measured mode table for a cowbell was found (two searches) **[web]**. Rossing's
  holographic work covers bells, gongs and drums, not the cowbell **[paper, R01]**.
- **By analogy** with bells: the lowest modes are the mouth's breathing, its broad walls moving in
  and out as a bell's (2,0), with neighbours split by the rectangular shape. Above lie the walls' own
  plate modes **[memory]**.
- **The recorded cowbell (key 56)** **[meas]**: the strongest partial at **472 Hz**, then partials
  relative to it:

| Ratio to 472 Hz | 1.072 | 1.142 | 1.229 | 4.20 | 4.74 | 5.28 | 5.35 | 5.42 | 5.52 | 5.61 | 6.60 | 7.14 | 7.79 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| dB | −13 | −24 | −30 | −27 | −30 | −20 | −31 | −28 | −32 | −29 | −29 | −28 | −32 |

  - The decays are 0.3–0.75 s.
  - The 472–580 Hz cluster gives the cowbell its rough "clank".
  - The 2–3.7 kHz group gives its edge. (The TR-808 imitated a cowbell with two square waves at 540
    and 800 Hz **[memory]**: a rough pair, as here, but not these ratios.)

## 3. The agogo bells

- **What they are:** two conical iron bells on one handle, struck with a stick (Brazilian; GM's 67
  high, 68 low).
- **The model before this work** used Rayleigh's ring law, 1 : 2.83 : 5.42 : 8.77 : 12.9.
  - In practice it sounded nearly a sine: its partials were 44 dB and more under the note at C5.
  - The stick (20 g) was as heavy as the model's bell (5 modes, modal mass 20 g), so it rode the
    fundamental for half a cycle and filtered the partials away.
- **The recorded pair** **[meas]**: high 1499 Hz, low 1000 Hz. **Both have the same ratios:** 2.0,
  2.25, 2.55, **3.73 (−17 to −23 dB)**, **5.12 (−15 dB)**, 6.46, 7.45, 7.94 (−25 to −34 dB). T60s are
  0.3–0.8 s whatever the pitch.

## 4. Plan (2026-11-16)

1. **The triangle:** two vibrators on the modal engine (`ModalSet`, one polarization each).
   - **In the plane:** the free bar's bending modes to 17 kHz (16) and three longitudinal modes.
   - **Across it:** the same, each 4 % (first mode) to 12 % (fourth up) higher, with ±1.5 % scatter
     (fitted to the recording's second family).
   - **Strokes:** shapes of mean square 1 and the rod's mass as modal mass. The engine's mallet (20 g,
     K 9·10⁹, α 1.5) strikes both vibrators at 25° from the plane, at 0.42–0.54 of the rod, a little
     different each time.
   - **Radiation:** ∝ √(f / 2 kHz), capped at 2 (the thin rod).
   - **T60:** 3 s at 2 kHz (× (2 kHz/f)^0.25 below 6 kHz), falling as f^−1.3 above.
   - **The hand:** an extra damping on every mode (T60 0.4 s). Key 80 or the hold clasps, any other
     key lets go. So a muted key on a ringing triangle chokes it.
2. **The cowbell:** one vibrator with the recorded partials and their decays.
   - **Radiation:** set so the levels come out as recorded.
   - **Stroke:** a hickory shaft (20 g, K 2·10⁹) on the mouth's edge, its place random along the edge
     (the walls' modes change with it, the mouth's breathing not).
3. **The agogo:**
   - the recorded ratios and decays;
   - a heavier bell (modal mass 80 g) and a lighter stick tip (6 g), so the contact is short and the
     partials come through;
   - amplitudes fitted on the model's own stroke.
4. **Velocity:** speed ∝ velocity^1.65. On a contact this stiff the colour hardly changes, so the
   level alone must carry General MIDI's ~30 dB from 16 to 127.
5. **Levels:** the common loudness (−28.2 LUFS at velocity 90), held down where velocity 127 would
   peak over full scale (as the other struck instruments).
6. **Checks:** partials and decays against §1–3, the mute and the choke, the velocity range, fuzz,
   cost. Test `metal_perc()` in `tools/test.py`.

## 5. What was built and measured

- **Files:**
  - `src/metalperc.h/.cc`: `MetalPercModel` and `MetalPercPlayer`;
  - registration in `src/voice.cc` and `src/bassinst.cc` (kPlucked rows `triangle` and `cowbell`);
  - trims in `src/levels_data.h`: triangle +0.9 dB, cowbell −3.9 dB;
  - the agogo in `src/modalinst.cc`: its table, the bell's mass and stick tip
    (`PJ_AGOGO_MASS`, `PJ_AGOGO_TIP`), and its level trim (20.6 → 30.8 dB).
- **The triangle (open, velocity 100):**
  - partials at 1236, 1379, 2577, 2915, 3431, 4408, 4892, 5506, 6138, 6726, 8068, 9094, 11 119 Hz,
    within 11 dB of each other;
  - T60 3.4 s at 1.2 kHz, 2.5 s at 4.4 kHz, 1.0 s at 11 kHz;
  - muted (80): T60 0.34 s;
  - a muted key 1 s into an open ring takes it 65 dB under ringing free within 0.4 s.
- **The cowbell (velocity 100):** 472 Hz; 506 at −16 dB (recorded −13), 539 at −27 (−24), 1983 at −28
  (−27), 2492 at −18 (−20), 2605 at −25 (−32), 3115 at −31 (−29). T60 0.4–0.8 s.
- **The agogo, at 523, 699, 988 and 1480 Hz:** 3.73× at −20 to −21 dB (recorded −17 to −23), 5.12× at
  −14 to −17 (−15), 2.0× at −29 (−28 to −31), 7.94× at −25 to −33 (−25 to −26).
- **Velocity range, 16 to 127 (unweighted fast max):** triangle 32.9 dB, cowbell 30.2 dB.
- **Peaks at velocity 127:** triangle 0.48, cowbell 1.00, agogo 1.00.
- **Cost:** triangle 46 ns/sample, cowbell 39 ns/sample (0.2 % of a core each); fuzz stable.
- **The GM keys** (to be routed in `src/drumkit.cc`, which belongs to the other session):
  - 80, 81 → triangle (the key as played);
  - 56 → cowbell;
  - 67, 68 → agogo. The recorded pair sits at 1499 and 1000 Hz. The present mapping (keys 77 and 72:
    698 and 523 Hz) is a fifth to an octave lower; keys 90 and 83 (1480 and 988 Hz) match the
    recording.

## 6. Not done, and open

- **No real measurement:** no recording of a real triangle with its full ring. The FluidR3 sample is
  cut at about 1.7 s, so the long T60s are a design.
- **Triangle details:** the triangle's suspension (its string) and its slow swinging are not
  modelled. The kink coupling is folded into the second family's offset, not computed.
- **Cowbell strokes:** only the mouth stroke. A body or tip stroke (drier) and a hand-held,
  hand-damped cowbell are not separate keys in GM; they could follow velocity or a knob.
- **Sizes:** one size each (8" triangle, a 472 Hz cowbell). Cowbells vary a lot; `PJ_COWBELL_HZ`
  moves the whole bell.

## 7. Sources

- **[S22]** M. D. Stanciu et al., "Mechanical and Acoustic Properties of Alloys Used for Musical
  Instruments", Materials 15 (2022) 5192, https://doi.org/10.3390/ma15155192 (triangles' modal
  analysis: 156 Hz in plane, 162 Hz "torsion" across it, 269 Hz longitudinal; dominant peaks
  1.6–1.9 kHz).
- **[S22b]** M. D. Stanciu et al., "Dynamic Analysis of the Musical Triangles—Experimental and
  Numerical Approaches", Applied Sciences 12 (2022) 6275, https://www.mdpi.com/2076-3417/12/12/6275
  (experiment and FEM within 2.2 %; not read beyond the abstract: the site refused the fetch).
- **[R76]** T. D. Rossing, "Acoustics of percussion instruments", The Physics Teacher 14 (1976)
  546–556 (cited in S22 for the triangle's transverse modes as a straight bar's).
- **[L87]** K. A. Legge, "Non-linear mode coupling in symmetrically kinked bars", J. Sound Vib. 118
  (1987) 23–34 (cited in S22).
- **[R01]** T. D. Rossing, "Acoustics of percussion instruments: Recent progress", Acoust. Sci. &
  Tech. 22 (2001) 177, https://www.jstage.jst.go.jp/article/ast/22/3/22_3_177/_article
- **[meas]** FluidR3_GM.sf2 (TiMidity++), keys 56, 67, 68, 80, 81, analysed with
  `scratchpad/drums/triangle/ref.py` (peaks; per-peak band decays).
