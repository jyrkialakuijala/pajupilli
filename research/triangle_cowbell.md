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
  - The 2–3.7 kHz group gives its edge. (The classic drum machine imitated a cowbell with two square waves at 540
    and 800 Hz **[memory]**: a rough pair, as here, but not these ratios.)

### 2.4 Two voices, the hand and the turning (2026-11-27) **[design; the owner; web via the owner]**

- **What players do** (the owner, and the material the owner brought):
  - **The four main tones:** the lower and side fingers press the bell or lift off, apart from where it
    is struck: open mouth, muted mouth, open top, muted top. Within each, where exactly on the mouth or
    the top still varies.
  - **The stick:** the mouth with the side (shoulder) of the drumstick: the open, low, accented clang;
    the top, toward the closed end, with its tip: a dry, high ping for ghost notes and subdivisions.
  - **The turning** (the owner): the bell turned toward the listener when the top is struck, tipped up
    about 60° when the mouth is. As it turns, its partials project differently into the room: complexity
    from the handling.
  - **The kit's bell** is mounted (an L-rod on the kick's hoop or a stand): no hand, no turning, a strip
    of tape or a gel inside to take the excess ring.
- **The model** (`src/metalperc.cc`, `MetalPercModel::strikeBell`, `turn`):
  - **Place** 0 … 1 in two zones. The mouth (< 0.5): the side of the stick (20 g, K 2·10⁹); the breathing
    modes full, a little less further in. The top (≥ 0.5): the tip (7 g, K 4·10⁹, ×1.4 faster: further out
    on the stick). The breathing modes at 0.4 on the body and 0.2 (−14 dB) at the crown: they move most
    at the open end. Four high wall modes (8.9–13.6×, T60 0.2–0.35 s) come in.
  - *Found while building:* a larger excitation shape is also a lighter bell under the stick, which
    then takes less energy. Raising the crown's wall shapes made it 11 dB *quieter*. So the shapes stay
    near 1 and the level comes from the stroke.
  - **The hand:** extra loss per mode, the lowest four (the face's breathing) to T60 0.1 s, the rest to
    0.25 s, when fully pressed; growing as pressure^1.5. It is set as the stroke lands, kept until the next.
  - **The turning:** the crossover at 1.1 kHz on the bell's sound. The mouth's breathing (472–580 Hz,
    ka ≈ 0.4) goes everywhere; the walls' 2–4 kHz partials (ka 3–6) beam along the mouth's axis: off it
    by −12 dB × (1 − cos θ), −6 dB at 60°, and a share of them to the side. The bell is 90 % of the way to
    its angle as the stick lands and turns on through the ring (~70 ms), with a 0.6 Hz sway of ±4°.
- **Keys:** cowbell 56: the open mouth stroke, still, as before. Mounted 40 + 4 place + mute, held
  (turning) 60 + 4 place + mute: place 0, 1 the mouth (its middle, further along), 2, 3 the top (the
  body, the crown); mute 0 … 3. The hand scatters place (±0.06, within its zone) and pressure (±0.05).
- **Measured** (velocity 100, mounted, first 100 ms; `scratchpad/cb/strokes.py`):

| stroke | peak dB | 400–650 Hz | 1.8–4 kHz | centroid (Hz) | to −30 dB (s) |
|---|---|---|---|---|---|
| 56 (as before) | −4.6 | −23.4 | −35.7 | 573 | 0.33 |
| mouth, open | −4.5 | −23.4 | −35.4 | 576 | 0.33 |
| mouth, muted 1 / 2 / 3 | −4.0 | −25.7 / −28.2 / −29.7 | −36.2 / −37.6 / −38.6 | 649 / 874 / 1224 | 0.16 / 0.08 / 0.05 |
| mouth, further along | −5.1 | −25.2 | −35.3 | 644 | 0.33 |
| top (body), open / muted 1 | −6.5 / −6.4 | −29.6 / −31.4 | −33.7 / −34.2 | 1110 / 1507 | 0.33 / 0.19 |
| crown, open / muted 1 / 2 / 3 | −8.6 … −8.7 | −32.4 / −34.2 / −35.5 / −36.1 | −33.8 / −34.2 / −35.9 / −37.3 | 1457 / 1886 / 2213 / 2374 | 0.33 / 0.19 / 0.13 / 0.07 |

  - Held and tipped up, the mouth stroke's 1.8–4 kHz is 4–8 dB under the mounted one's (the angle and
    the sway); the top stroke, facing the listener, is unchanged.

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
- **Cowbell strokes:** built 2026-11-27 (§2.4, §7).
- **Sizes:** one size each (8" triangle, a 472 Hz cowbell). Cowbells vary a lot; `PJ_COWBELL_HZ`
  moves the whole bell.

## 7. The player (2026-11-27): a key 56 is "the cowbell plays here" **[design]**

As with the tambourine (`research/tambourine.md` §1.1), the note is a request. The player chooses the
stroke before the song (`src/cowbellplan.h/.cc`, called from `src/midifile.cc`, sent as keys 88–119).

- **The user's files** (`research/gm_compare/work/cowbell/cb.py`, `pat.py`): about 3100 notes of key 56.
  - **Hand-made Latin files** with velocities that mean something: *Manteca* (beats 1 and 3 at 70, the
    anticipations 2& and 4& at 48: the campana's open and closed strokes written as velocities),
    *Peanut Vendor* (quarters, humanized 59–72), *Cubano Chant*, *Gaviota* (quarters at 127).
  - **Transcriptions:** every note at 100, spread evenly over the beat (Jammo, So Flute, El Shing-A-Ling,
    Say My Name, Paul Simon).
- **What it reads:**
  - the velocities: flat (one value for 85 % of the notes) or carrying accents;
  - the grid: sixteenths, triplets, or neither (a transcription);
  - a Latin band (16 or more notes on bongos, congas, timbales, claves, guiro): the bell is held;
  - else it is mounted on the kit.
- **Each note's accent weight:**
  - The meter (4/4 from the song's start): 1 → 1.0, 3 → 0.85, 2 and 4 → 0.6, the "&" → 0.4, the
    sixteenths → 0.2; in a transcription 0.5, since its grid is not the music's.
  - Listening: +0.35 with the clave (Latin), +0.2 with the bass drum, +0.15 for the first stroke after
    a gap.
  - The file's velocity, where it means something: ±0.35 per standard deviation.
- **Ranked within its bar** (half the rank, half the weight), so that a bell on quarters and one on
  sixteenths both get two voices. Then: ≥ 0.72 the mouth, ≥ 0.55 the mouth further along, ≥ 0.35 the
  body, else the crown.
  - Quarters come out as the mouth on 1 and 3 and the body on 2 and 4, as the pasted advice has it.
  - In rock or funk, a stroke on the snare's backbeat moves one step toward the crown and 8 % softer:
    it locks with the snare rather than fighting it.
- **The hand** (held only): open for an accent (≥ 0.72) or a note with a beat or more of room; pressed
  (2) for quick in-between notes (half a beat or less to the next); lightly (1) between. A mounted
  bell: tape, mute 1 on the top. The last stroke before a rest of two beats rings open.
- **The level:** the file's own velocity where it means something. Flat: the band's level + 25 at
  most, × 1.0 / 0.92 / 0.85 / 0.78 by place, the bar's downbeat × 1.06.
- **A transcription:** a doubled note (two within 25 ms) is played once. An isolated note (fewer than
  three others within four bars) is skipped off the beat; on the beat it is played soft on the body.
- **What it read in the files:**

| file | read as | mouth / lip / body / crown | muted | skipped |
|---|---|---|---|---|
| Manteca | held, the file's accents | 475 / 0 / 235 / 238 | 470 | 0 |
| Peanut Vendor | held, the file's accents | 363 / 97 / 128 / 139 | 23 | 0 |
| Cubano Chant 2 | held, the file's accents | 46 / 13 / 23 / 37 | 13 | 0 |
| Jammo | held, a transcription | 63 / 26 / 171 / 13 | 186 | 1 |
| El Shing-A-Ling | mounted (no Latin percussion in the file), a transcription | 15 / 25 / 52 / 94 | 128 (tape) | 3 |
| Say My Name | mounted, a transcription | 5 / 12 / 20 / 10 | 25 (tape) | 0 |

- **Switches:** `PJ_COWBELL_PLAN=0` (every note key 56, as before), `PJ_COWBELL_BEAM` (12 dB; 0: no
  turning), `PJ_COWBELL_SWAY` (4°), `PJ_COWBELL_TURN_AHEAD` (0.9), `PJ_COWBELL_TIP_SPEED` (1.4),
  `PJ_COWBELL_TIP` (7 g), `PJ_COWBELL_BODY_LOW` / `CROWN_LOW` (0.4 / 0.2), `PJ_COWBELL_TOP_HI` (1),
  `PJ_COWBELL_MUTE_LOW_T60` / `MUTE_HI_T60` (0.1 / 0.25 s).
- **Listening** (`research/gm_compare/work/cowbell/`, 45 s each, `-base` = `PJ_COWBELL_PLAN=0`):
  `manteca`, `peanut`, `cubano`, `jammo`, `shingaling`, `saymyname`; and the bell alone: `tones-held`
  (the four tones, 4 each, then the mambo bell's 2-3 pattern, M open mouth, c muted top),
  `tones-held-noturn` (the same, not turning), `tones-mounted`.
- **Not done:**
  - the bell's size by style (a large bongo bell, a small cha-cha or mambo bell, a deep rock bell):
    one bell, 472 Hz;
  - no sections and no restraint across the form: the plan does not lay out in verses or bring the bell
    in for a chorus;
  - no vocal awareness;
  - no swing reading beyond triplets;
  - the bar assumed to be 4/4 from the song's start;
  - the timbalero's two bells;
  - with the Latin band read only from its percussion keys, a salsa transcription that uses kit sounds
    gets a mounted bell.

## 8. Sources

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
