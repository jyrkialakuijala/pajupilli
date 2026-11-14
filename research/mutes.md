# Brass mutes on the pajupilli waveguide (muted trumpet, GM 60): research notes

Status: 2026-11-14. SI units. c = 349.5 m/s, ρ = 1.18 kg/m³, ρc² = 144 140 Pa (as in `Bore`). Extends
`horns.md` §4.3 (the hand) and §5 (radiation). Tags as there: **[G]** my estimate or guess, **[D]** derived by me
from published formulas or from my own transfer-matrix calculation (§2.1), **[U]** a claim from a secondary source, an
abstract or a vendor/forum page that I could not check in the primary text.

**Short answer.**
- A mute is a **second resonator at the bell**. Below its own resonance it loads the bore like a mass and lowers the
  bore resonances. Above it, it loads the bore like a spring and raises them. Near it, it splits the nearest bore
  resonance in two (Pyle's rule [KP15]).
- Open mutes (straight, cup) leave a narrow **annular leak** between mute and bell. That leak is the Helmholtz neck and
  the mute's inside is the cavity, which gives the "parasitic" impedance peak. For trumpet and trombone it lies below
  the 2nd resonance [CS82, CGM21].
- The **Harmon** seals the bell with a cork ring, so all the sound goes through its chamber and out of one small hole.
  That makes it a different bore end, not a side branch: neck → chamber → exit tube (or stem) → radiation.
- In 1-D, the Harmon is the easy case: just re-profile the bore's last few centimetres. The straight and cup mutes need
  a junction (or a lumped side branch) at the mute's tip.
- For GM program 60, build the **Harmon, stem out** first (§0). §5 gives the starting values.

---

## 0. Which mute is "Muted Trumpet" (GM 60)?

The GM 1 spec names program 60 only "Muted Trumpet" and says nothing about the mute. GM 2 adds "Muted Trumpet 2" in
bank 1 **[U]** (MIDI tables quoted in search results).

| Sound set | Program 60 | Evidence |
|---|---|---|
| MuseScore_General (SF2/SF3, the GM set shipped with MuseScore) | "**Harmon Mute Trumpet**" (preset 000:059) | its changelog [MSG] |
| MuseScore forum (users and developers) | GM has one muted trumpet. "By convention it is taken to be harmon, and harmon is far more commonly used in music created today." Straight, cup and plunger are not in GM | [MSF] **[U]** (forum, via search) |
| Roland Sound Canvas / GS "Mute Trumpet", FluidR3, General User GS | It sounds like a stemless Harmon (thin, buzzy, Miles-Davis-like, no low end) **[G]**, from memory of the sound, not checked | none |

**Decision:** model the **Harmon without stem** ("Miles Davis", 1946 on [WIKI]) as the GM 60 default. Keep **stem in**
and the **straight mute** as variants (GM 2 bank 1, or a controller). The straight mute is what "con sordino" means in
classical scores [WIKI, CGM21].

---

## 1. The mutes: geometry and what they do

### 1.1 Common physics

- **Two-port at the bell.** From inside, a mute (or the hand, or an echo bell) looks like a **Helmholtz resonator**:
  a cavity (the "spring") and a narrow neck (the "mass") [KP15].
  - Below that resonance the load is mass-like and **lowers** the bore resonances.
  - Above it the load is spring-like and **raises** them.
  - Near it the nearest bore resonance **splits into two**.
  - "For the mute to raise the pitch consistently throughout the playing range … the mute resonance frequency [should]
    lie below, or at least in the lower part of, the playing range" [KP15].
- **The parasitic peak** (Sluchin & Caussé) [CS82, CGM21]:
  - The straight mute adds one extra impedance peak and shifts its neighbours. For nearly hyperbolic horns (trumpet,
    trombone) "the parasite normally lies near the second resonance". For nearly conical horns (tuba) it lies near the
    lowest.
  - The parasite **falls as the mute goes deeper** and goes roughly as **V^(−1/2)** of the mute volume.
  - **Cup and Harmon mutes may add two parasitic peaks.**
  - Example, tenor trombone with straight mute: the extra peak is at **77 Hz**. In 1st position the 2nd–4th peaks move
    slightly up. In 7th position the 2nd partial E2 (82.4 Hz) splits and is hard to hold [CGM21]. An active mute that
    cancels the cavity pressure removes the peak and gives back the B♭1 pedal (58 Hz) [MMCSS15, CGM21].
- **Radiation zeros.** The muted instrument's external spectrum, averaged over directions and notes, has "up to six
  prominent minima" at the **impedance minima of the mute itself** [CS82]. When the mute's input impedance (seen from
  its open tip) is low, it short-circuits the bell plane, so the leak flow and the radiation vanish.
- **High-frequency reflection goes up** (as with the hand, `horns.md` §4.3):
  - With the Bryant mute, "the impedance shows small but quite distinct peaks well beyond the 1200 Hz 'cutoff' of the
    open bell" [KP15].
  - Sealed mutes (Harmon, practice) reach a "playability" (peak/valley ratio) of ~10, against < 6 for open (corked)
    mutes [F10].
  - The horn practice mutes in [DJDW10] lowered the low peaks, raised the high ones and made the series less harmonic.
- **Directivity.** Changes are mainly above 1 kHz. More sound goes behind the player. Straight and cup mutes change
  little below the 1.6 kHz third-octave band [ABLG22].

### 1.2 Geometry

Few dimensions are published. A B♭ trumpet bell mouth is ~123 mm. Rows marked **[G]** were sized by me to fit the bell
of my scratch trumpet (§2.1). That bell is narrower inside than a Bach bell, so a real mute sits ~20–30 mm deeper.
What matters is the **bell diameter at the mute's tip and corks**, not the depth.

| | Straight | Cup | Harmon ("wow-wow") | Plunger |
|---|---|---|---|---|
| Body | Truncated cone, open narrow end into the bell, closed wide end [CGM21, WIKI] | Straight mute plus an inverted bowl facing the bell [WIKI, CGM21] | Bulb (aluminium/copper). The cork ring seals the bell. Exit hole in the front face with a sleeve projecting inwards, and a removable stem [CGM21, WIKI, F10] | Rubber cup (sink plunger) held in front of the bell [WIKI] |
| Length / max Ø | ~200 mm / ~96 mm ID at the wide end **[G]** | ~200 mm, bowl Ø ~100–105 mm **[G]** | ~140 × 89 mm (Harmon B, listed as 5.5 × 3.5 in **[U]**, retail) | Ø ~140 mm (H1 metal plunger 127 mm **[U]**) |
| Tip / inlet | Tip ID 24 mm at bell Ø ≈ 39 mm, 80 mm inside the rim **[G]** | as straight | Cork ring at bell Ø ≈ 48 mm (55 mm inside the rim). Neck ID 36 → 44 mm over 45 mm **[G]** | none |
| Seal / leak | Three corks (~1 cm thick [ABLG22 **[U]**], 6–8 mm radial gap **[G]**). Sound leaves through the annulus [CGM21] | Annulus, then the gap between bowl rim and bell rim (adjustable 5–25 mm **[G]**) | **Sealed** (cork strip all round) [CGM21, F10] | Gap h between the rim and the cup, 0–30 mm **[G]** |
| Internal volume | ≈ 0.63 L **[G]** (cone 24 → 96 mm, 200 mm) | + bowl cavity ≈ 0.3 L **[G]** | Chamber ≈ 0.5 L (Ø 88 × 85 mm) **[G]** | cavity ≈ 0.24 L **[G]** |
| Exit | Annulus to the rim | Bowl-rim slot | Hole Ø ≈ 17 mm (fits the stem), sleeve 30 mm into the chamber **[G]**. Stem: tube Ø 15 × 105 mm + small bell to Ø 40 **[G]** | radial slot |
| Cork effect on pitch | 0.5 mm cork steps → ~4 cents each. Six sizes span 19–21 cents. Factory corks +3.6 c (Bach 180S37) and +9.7 c (Yamaha C) [S23] | | | |

### 1.3 What each does (literature)

| Mute | Spectrum | Pitch / response |
|---|---|---|
| Straight | "Acts as a high-pass filter… on trumpet passes frequencies above about 1800 Hz", "shrill, piercing". One of the few mutes that can play forte [WIKI **[U]**]. Played G4 (concert F4): harmonics 1–4 weak, 5+ strong [F10] | Players say it plays sharp. Good metal mutes are in tune or < 20 c sharp. A cheap fibre one was erratic and often flat [F10] |
| Cup | "Band-pass, roughly 800–1200 Hz", darker [WIKI **[U]**]. Trombone cup: **−20 dB in a narrow band near 1000 Hz** from the bowl cavity resonance [S&C via CGM21]. Harmonics 1–3 strong, 4+ weak [F10] | Flat if not in tune [F10] |
| Harmon, stem out | "Band-pass about 1500–2000 Hz", "subdued, distant" [WIKI **[U]**]. "Strong resonances between 1–2 kHz (depending on the model)" (forum **[U]**). One played note: strong low harmonics, a dip at harmonics 4–5, maybe a 2.5–3.5 kHz region [F10] | Never flat. Stemless more out of tune than stem-in, but a good one stays within 20 c [F10] |
| Harmon, stem in | "Penetrating" [WIKI]. Lows down, harmonics 4–6 up, **4th strongest** (≈ 1.4 kHz for concert F4) [F10] | "Nearly perfect intonation" [F10] |
| Plunger | Held still: dull. Moved: wah, speech-like [WIKI] | not measured |
| Practice (sealed) | Very quiet, odd spectrum [F10] | In tune, strong high peaks (easy top register, more resistance) [F10, DJDW10] |

General: "standard trumpet and cornet mutes produce strong formants at various frequencies in the 1–3 kHz range" [UNSW].

---

## 2. Modelling in 1-D

### 2.1 How I checked the networks

I wrote a scratch transfer-matrix model (not in the repo; same lossy segments and radiation as `tools/brass.py`). It has:
- a B♭ trumpet: cup 0.65 cm³, throat Ø 3.66 mm, backbore to 8.9 mm, leadpipe to 11.66 mm, cylinder to 960 mm, then a
  Bessel bell (ε = 0.75) to Ø 123 mm at 1440 mm;
- peaks 231, 353, 463, 584, 701, 815, 929 Hz, within ±20 c of B♭ partials 2–9;
- each mute as distributed geometry (the annulus, the mute's interior as a closed cone, the chambers and stems as
  tubes), then each compared with its lumped network.

Pitch shifts are the shifts of the **impedance peaks**. Levels are radiated power summed over the harmonics of the note,
for the **same lip flow**, from a monopole at the radiating opening. Body radiation and leakage are left out.
Every **[D]** number below comes from this model with **[G]** geometry. Treat it as a consistent starting point, not a
measurement.

### 2.2 Straight mute

**Topology.** At the mute's tip plane (bell Ø 38.6 mm, area 1.17e-3 m²) the bore splits:

```
bore ──┬── annulus (leak): L_g + R_g(√f) ── Z_rad(annulus, ≈ the bell's) ──> radiates
       └── interior (closed cone): L_tip ── C_m ──┤ (closed)          [one section, good to ~600 Hz]
                                    or  L1 ─┬─ L2 ─┬─┤                 [two sections, good to ~1.5 kHz]
                                            C1     C2
```

**Option A, geometry (preferred if the waveguide has 3-neighbour junctions, `horns.md` §4.1).**
- Continue the bore through the annulus: area π(R_bell² − R_mute²) from the tip to the rim. Take away ~30 % of it over
  the 15 mm of the corks.
- Radiate from the rim annulus: area 9.5e-3 m², a_eq ≈ 55 mm, close to the open bell's.
- Hang the mute's interior on the tip cell as a closed side bore: a cone 24 → 96 mm ID, 200 mm long, ≈ 15 cells at
  dx ≈ 13 mm.

**Option B, lumped (the cut moves to the tip plane; implicit 4–6-state network) [D]:**

| Element | Value | Note |
|---|---|---|
| L_g (annulus, ρ∫dx/A) | **86 kg/m⁴** (57 / 135 for tip 70 / 90 mm in) | ∫dx/A = 73 m⁻¹ |
| R_g (viscous) | **8.5e3 Pa·s/m³ at 500 Hz, ∝ √f** | min radial gap 6–8 mm |
| Radiation | as `horns.md` §5.1 with S = 9.5e-3 m² | |
| C_m = V/ρc² | **4.39e-9 m³/Pa** (V = 0.63 L) | |
| L_tip | **106 kg/m⁴** | fitted to the cone's first impedance zero |
| Two-section interior (fit) | L1 = 86, C1 = 4.5e-10, L2 = 67, C2 = 2.9e-9, R ≈ 3e3 | zeros 234, 1262 Hz; pole 992 Hz (exact cone: 234, 1264; 1008) |

**Frequencies [D]:**

| Quantity | Value |
|---|---|
| Interior series zero (radiation minimum; the "blow across the tip" Helmholtz) | 1/(2π√(L_tip C_m)) = **233 Hz** |
| Next interior zeros (the cone's own modes) | 1263, 2155, 3036, 3914 Hz (the "six minima" of [CS82]) |
| Termination antiresonance | 1/(2π√((L_g+L_tip) C_m)) = 174 Hz |
| **In-situ parasitic input-impedance peak** | **165 Hz** (181 / 146 Hz for tip 70 / 90 mm in), about 0.7 × the 2nd resonance. Same ratio as the trombone's 77 / 116 Hz [CGM21] ✓ |

**Lumped vs geometry [D]:**
- Option B reproduces the parasite and peaks 2–6 within ~5 c. Peaks 7–11 come out 10–25 c too high.
- Option B's radiation keeps falling above 1 kHz, because a single-section interior has no 1 kHz pole. Use the
  two-section interior or option A for the bright, "high-pass" timbre.
- Distributed model, radiation relative to the open bell per third octave:

| Band (Hz) | 250 | 315–1000 | 1250 | 1600–2000 | 2500–4000 |
|---|---|---|---|---|---|
| Change | −15 dB | −5…−9 dB | −11 dB | −5 dB | −1…−3 dB |

So the low and middle bands are cut and the treble kept, which is the reported "high-pass" ✓.

### 2.3 Harmon (stem out / stem in)

**Topology: serial.** The cork seals the bell (at Ø ≈ 48 mm, 55 mm inside the rim). The flare beyond the seal is
outside air and drops out. The bore then continues:

```
bore ── L_n (neck) ──┬── L_a + R_a (exit hole / stem) ── Z_rad(exit, flanged by the front face) ──> radiates
                     C_h (chamber)
```

**Option A, geometry (no new machinery: only a different end profile):**
1. Bell profile down to the seal.
2. Neck: Ø 36 → 44 mm, 45 mm.
3. Chamber: Ø 88 mm, 85 mm (6–7 cells).
4. End branch at the chamber's last cell with the exit's area: a hole Ø 17 mm with series length 31 mm plus the inner
   end correction 0.6a, and flanged radiation (0.82a). Stem in: bore cells Ø 15 mm for 105 mm, then a flare to Ø 40.
5. Optional: the dead-end annulus around the sleeve, a closed side branch 30 mm long with a quarter-wave near 2.5 kHz,
   which makes a notch.

The chamber's **half-wave mode c/(2·85 mm) ≈ 2.06 kHz** is the stem-out formant. Only the cells give it; a lumped
compliance does not.

**Option B, lumped [D]:**

| Element | Stem out | Stem in |
|---|---|---|
| L_n (neck) | **48 kg/m⁴** | 48 |
| C_h (chamber, V ≈ 0.51 L) | **3.55e-9 m³/Pa** | 3.55e-9 |
| L_a (exit, without the outer end correction) | **188 kg/m⁴** (hole Ø 17 mm, l = 31 mm + 0.6a) | **781 kg/m⁴** (stem Ø 15 × 105 mm + bell, both end corrections) |
| R_a (viscous) | 1.2e4 Pa·s/m³ at 500 Hz, ∝ √f | 6e4 at 500 Hz, ∝ √f |
| Radiation | flanged R‖L, a = 8.5 mm (L_r = 36 kg/m⁴, R = ρc/S) | unflanged, a = 20 mm |
| Stem's own compliance (if used as a T-section) | none | 2.3e-10 m³/Pa |

**Frequencies [D]:**

| Quantity | Stem out | Stem in |
|---|---|---|
| Chamber Helmholtz 1/(2π√(L_a C_h)) (outer correction added) | 178 Hz | **96 Hz** (distributed: 96 Hz ✓) |
| In-situ parasitic peak | **155 Hz** | **96–100 Hz** |
| Formant(s) of the transmission (bell plane → outside) | **2.07 kHz** (chamber length mode, +10 dB over the floor) | **1.52 kHz** (stem tube, +27 dB) and 2.06 kHz (chamber, +11 dB) |

The lumped network matches the distributed one's impedance peaks within ~10 c up to 900 Hz, so it is good for
**playing**. Its transmission has neither formant, so it is not good for **timbre**. Two ways around that:
- run the chamber and stem as cells (option A);
- keep the lumped end in the loop and put the formant in the **output** as a resonant filter outside the feedback:
  - stem out: band-pass 2.0–2.1 kHz, Q ≈ 4, +10 dB;
  - stem in: 1.5 kHz, Q ≈ 6, +25 dB, plus a weaker 2.06 kHz;
  - the hand moves the stem-in formant (§4).

**Sizing note [D/G].** My first, smaller Harmon guess (chamber Ø 82 × 75 mm, hole Ø 22 mm) put the parasite at
~200–250 Hz. It split the 2nd peak (199/251 Hz) and made B♭3 +140 c sharp, which contradicts [F10]. A parasite **below
~160 Hz** is needed for sane low-register tuning. Hence the larger chamber and the 17 mm hole.

### 2.4 Cup mute

Take the straight mute and put a **cavity C_c with the bowl-rim slot L_c** between the annulus and the radiation:

```
bore ──┬── L_g + R_g ──┬── L_c(h) + R_c ── Z_rad ──>
       │               C_c (bowl cavity)
       └── interior (as the straight mute)
```

- C_c ≈ **2.1e-9 m³/Pa** (≈ 0.3 L) **[G]**.
- Slot of circumference 2π·61.5 mm and height h: **L_c = ρ(15 mm + 0.6h) / (2π·0.0615·h)**.

| Bowl-rim gap h | L_c (kg/m⁴) | Second Helmholtz f_c [D] |
|---|---|---|
| 5 mm | 11.0 | 1.05 kHz |
| 10 mm | 6.4 | 1.38 kHz |
| 15 mm | 4.9 | 1.58 kHz |
| 25 mm | 3.7 | 1.82 kHz |

The result is a straight-mute high-pass followed by a low-pass with a resonance at f_c: a **band-pass ~0.8–1.4 kHz**,
with −15…−20 dB at 1.6–4 kHz relative to the open bell (gap 5–10 mm) [D]. This fits "800–1200 Hz" [WIKI]. The second
parasite splits the peaks near 0.7–0.95 kHz [D]. That is the "two parasitic peaks" of [CS82].

### 2.5 Plunger

There is no insertion: a cavity in front of the bell mouth, then a radial slot:

```
bell end ──┬── L_p(h) + R_p ── Z_rad(a ≈ 70 mm) ──>
           C_p ≈ 1.7e-9 m³/Pa (0.24 L) [G]
```

L_p(h) uses the same slot formula as L_c. f_p = 1/(2π√(L_p C_p)) [D]:

| h | 1 mm | 3 mm | 6 mm | 12 mm | 25 mm |
|---|---|---|---|---|---|
| L_p (kg/m⁴) | 47.6 | 17.1 | 9.5 | 5.6 | 3.7 |
| f_p | 565 Hz | 0.94 kHz | 1.27 kHz | 1.64 kHz | 2.04 kHz |

- Radiation is low-passed above f_p, so the **plunger's "wah" is f_p moving with h**.
- Below f_p the load is mass-like, so notes go **flat**: −4…−42 c (B♭3…D5) at h = 6 mm, worse near f_p.
- Nearly closed (h ≈ 1 mm) it behaves like hand-stopping: peaks move by most of a semitone [D].
- It is the same network as the horn hand's (`horns.md` §4.3) with a capacitor added.

### 2.6 In pajupilli's terms

- The present end `Branch` is a series mass and resistance, then R‖L radiation (`bore.h`). All mute networks above
  are that, plus **one shunt compliance** (Harmon, plunger) or **one parallel arm** (straight: L_tip + C_m).
  - Harmon or plunger: the end cell's state (p, q, l) becomes (p, q, l, v_C). It is a 4 × 4 trapezoidal solve, as
    `horns.md` §5.1 already plans for the Bilbao–Chick radiation. Or use option A and nothing new is needed.
  - Straight: (p, q_g, l_g, q_m, v_m), 5 × 5. Or use a junction plus a closed side bore.
- **Moving the cut.** For the straight and cup mutes the bore's last cells (tip → rim) become the annulus, or option B
  drops them and the end branch moves to the tip cell. For the Harmon the cells past the seal become the neck and
  chamber.
- **Radiators.** Radiate from the exit (`radCell_`/`radGain_`) instead of from the bell. A straight mute's body also
  radiates a little (thin aluminium) and so does the gap at the corks; both are left out here.
- **Nonlinear leak loss at ff [G].** The annular leak and the Harmon's 17 mm hole carry the whole bell flow. At ff the
  hole flow separates. Add ρ|u|/(2 C_d² S²) on R_a/R_g, with the same describing-function treatment as
  `Tube::holeLosses()`. The Harmon's "sizzle" at ff probably comes from this and from wall buzz **[G]**.

---

## 3. The effect on playing

**Pitch of the notes [D]** (impedance peaks, cents vs the open trumpet; same scratch bore):

| Note (concert) | Straight (tip 80 mm) | Cup (gap 10 mm) | Harmon stem out | Harmon stem in | Plunger h = 6 mm | Plunger h = 25 mm |
|---|---|---|---|---|---|---|
| B♭3 (233 Hz) | +15 | +15 | +55 | +32 | −4 | −1 |
| F4 (349) | +6 | +5 | +34 | +24 | −12 | −4 |
| B♭4 (466) | +1 | −2 | +15 | +9 | −21 | −6 |
| D5 (587) | −3 | −8 | 0 | −4 | −42 | −10 |
| F5 (698) | −7 | −21 | −14 | −16 | −99 | −19 |

**Literature:**
- Straight mutes "play high". Good ones are within 20 c, mostly sharp [F10].
- Factory corks give +3.6 / +9.7 c on two trumpets. Cork depth spans ~20 c [S23].
- Players push the main slide in, or file or shim the corks [S23].
- Harmons are sharp if not in tune; the stem helps [F10].
- Baroque sealed mutes: +87 c and +120 c measured, "about a semitone" by players [KP15].

The model agrees on the sign and on stem-in < stem-out. Its low-register Harmon numbers are probably large: the real
parasite may be lower still.

**For the player model.** Apply the shift as the calibration/slide offset per mute, about −10 c on the main slide for a
straight mute and −20 c for a Harmon **[G]**. Leave the residual (sharp low, flat high) to the physics.

**Resistance / response:**
- Sealed mutes raise the impedance peaks above ~800 Hz by +1…+5 dB [D] and more strongly in measurement [F10]. Top
  notes slot more easily and the instrument feels more "resistant" [F10, DJDW10].
- The open mutes change the peaks < 1 dB above 300 Hz [D].
- The parasite takes the 2nd partial (B♭3) in the Harmon and makes it **wobbly** [D] (cf. the trombone E2 [CGM21]).

**Loudness [D]** (radiated power of the note, same lip flow; listed as computed, but see the caveats):

| Straight | Cup | Harmon (either stem) | Plunger h = 6 / 25 mm |
|---|---|---|---|
| −3…−4 dB | −6…−9 dB | **−22…−24 dB** | −5…−8 / −0.5…−2.5 dB |

Caveats:
- The Harmon figure leaves out cork leakage and body radiation, so it **overestimates** the attenuation. My guess for a
  real one is −12…−18 dB **[G]**.
- I found no published dB figures. Players do play straight-mute forte [WIKI].
- A level trim per mute in `levels_data.h` will do the rest.

---

## 4. Wah-wah

- **Harmon with stem: the hand cups the stem's bell** [UNSW, WIKI]. A hand over an orifice adds mass:
  L_hand = ρ(t + 1.2 a_x)/S_x, with S_x = open fraction × the stem-bell area (Ø 40 mm: 1.26e-3 m²) and t ≈ 5 mm
  **[G]**.
- Stem-in formant from the distributed model [D]:

| Open fraction | 100 % | 60 % | 30 % | 15 % | 5 % | 1 % |
|---|---|---|---|---|---|---|
| Stem formant | **1.52 kHz** | 1.41 kHz | 1.34 kHz | 1.24 kHz | 1.02 kHz | **0.72 kHz** |

  - A second peak (the stem's next mode) moves from 3.4 to 1.8–2.2 kHz.
  - The chamber's 2.06 kHz stays put.
  - So **"wah" ≈ formant 0.7 → 1.5 kHz**, about an octave. The "ooo-aaa" vowel change [UNSW] is the same as an F2
    sweep, from /u/ to /a/.
- **Stem out with the hand over the hole.** This mostly lowers the level and the treble. The 2.06 kHz chamber formant
  does not move [D]. That is why the classic wah needs the stem.
- **Plunger** (the open bell or a pixie): f_p ≈ **0.6 → 2.0 kHz** as h goes 1 → 25 mm (§2.5), with the pitch flattening
  as it closes.
- **Control:** one 0–1 "hand" value per block, mapped to S_x (Harmon) or h (plunger). Glide the end branch like a pad
  (`Tube` key-stroke machinery).
  - Players' wah rate: ~2–6 Hz, from listening **[G]**.
  - Mod wheel → hand closure is the obvious MIDI mapping **[G]**.

---

## 5. Recommendation and starting values (B♭ trumpet, bell 123 mm)

| | **Harmon, stem out (GM 60 default)** | Harmon, stem in | **Straight** |
|---|---|---|---|
| Cut / topology | Seal at bell Ø ≈ 48 mm. Bore → L_n → C_h → L_a → rad | same, stem as L_a | Tip at bell Ø ≈ 39 mm. Bore → [L_g + R_g → rad(9.5e-3 m²)] ‖ [L_tip → C_m] |
| Masses (kg/m⁴) | L_n 48, L_a 188 (+ flanged radiation L_r 36, a = 8.5 mm) | L_n 48, L_a 781 | L_g 86, L_tip 106 (or L1 86, L2 67) |
| Compliances (m³/Pa) | C_h 3.55e-9 | C_h 3.55e-9 | C_m 4.39e-9 (or C1 4.5e-10, C2 2.9e-9) |
| Resistances (Pa·s/m³, at 500 Hz, ∝ √f) | R_a 1.2e4 (+ nonlinear at ff) | R_a 6e4 | R_g 8.5e3 |
| Resonances | Helmholtz 178 Hz, in situ 155 Hz. Formant 2.07 kHz (cells or output filter) | 96 Hz; formant 1.52 kHz (hand → 0.72) | Parasite 165 Hz; radiation zeros 233, 1263, 2155 Hz; series zero 233 Hz |
| Pitch / level | +34 c (F4) … −14 c (F5); −22 dB [D], −12…−18 **[G]** | +24 … −16 c | +6 … −7 c; −3…−4 dB |

Order of work:
1. Harmon as geometry (option A). Retune the slide.
2. The hand on the stem as the end branch's area.
3. The straight mute as a lumped parallel arm.
4. Cup and plunger last.

### What to validate
- **Impedance:** `./flute -imp` with the mute against this table and against [F10]'s curves (Appendix C there).
  - Parasite frequency.
  - Peak shifts < 1 kHz.
  - Peak heights above 1 kHz, which should go up for the Harmon.
- **Played spectrum:**
  - Harmon stem out: a 2 kHz bump, little below 1 kHz relative to the open trumpet.
  - Stem in: the strongest harmonic near 1.4–1.5 kHz [F10].
  - Straight: harmonics 1–4 weak, 5+ kept [F10].
- **Feel:** low B♭3 should be unstable with the Harmon (parasite). If it is not playable at all, lower the parasite
  with a bigger C_h or a larger L_a.

---

## References

[ABLG22] J. E. Avila, S. D. Bellows, T. W. Leishman, K. L. Gee, "Directivity analysis of the muted trumpet", Proc. Mtgs. Acoust. 50, 035005 (2022). https://pubs.aip.org/asa/poma/article/50/1/035005/2892916/Directivity-analysis-of-the-muted-trumpet (abstract read; PDF not fetched). BYU page: https://physics.byu.edu/docs/publication/9857
[CGM21] M. Campbell, J. Gilbert, A. Myers, "Put a sock in it! The acoustics of brass mutes" (title as cited by [S23]), Acoustics Today 17(2), Summer 2021, pp. 15–21. https://acousticstoday.org/issues/2021AT/Summer2021/files/basic-html/page19.html (pp. 19–21 read)
[CS82] R. Caussé, B. Sluchin, "Mutes of brass instruments: Experiments and calculations", J. Acoust. Soc. Am. 71 (S1), S91–S92 (1982), doi:10.1121/1.2019634 (abstract via search snippets **[U]**). Full treatment: B. Sluchin, R. Caussé, *Sourdines des cuivres*, Éd. de la Maison des Sciences de l'Homme, Paris (1991) (not obtained).
[DJDW10] N. Dell, R. James, J. Davidson, J. Wolfe, "The effect of hand and mute on the impedance spectra of the horn", Proc. ISMA 2010. https://www.phys.unsw.edu.au/jw/reprints/DellISMA2010.pdf (read; horn practice mutes only)
[F10] G. Formosa, "Acoustic harmonicity and input impedance for various B♭ trumpet mutes", UIUC Physics 498POM report (2010). https://courses.physics.illinois.edu/phys406/sp2017/Student_Projects/Spring10/Greg_Formosa/G_Formosa_P498POM_Final_Report_Sp10.pdf (read; student work, one trumpet, 2.5 Hz resolution)
[KP15] S. K. Klaus, R. W. Pyle, "Trumpet mute pitch: an analysis of three historic mutes", Proc. Third Vienna Talk on Music Acoustics (2015). https://viennatalk2015.mdw.ac.at/proceedings/ViennaTalk2015_submission_14.pdf (read)
[MMCSS15] T. Meurisse, A. Mamou-Mani, R. Caussé, B. Sluchin, D. Sharp, "An active mute for the trombone", JASA 138(6), 3539–3548 (2015) (via [CGM21]; https://oro.open.ac.uk/44760 not fetched)
[MSF] MuseScore forum threads on mutes (e.g. https://musescore.org/en/node/311412), via search snippets **[U]**.
[MSG] MuseScore_General SoundFont changelog ("000:059 Harmon Mute Trumpet"). https://people.debian.org/~tg/sf/MuseScore_General_Full/changelog (read)
[S23] N. Sobieralski, "Problems of muted brass instrument intonation, a novel solution" (2023, vendor white paper, s-mute.com). https://cdn.shopify.com/s/files/1/0609/5968/6893/files/Problems_of_Muted_Brass_Instrument_Intonation_a_Novel_Solution.pdf (read)
[UNSW] J. Wolfe et al., "Brass instrument (lip reed) acoustics: an introduction", UNSW. https://newt.phys.unsw.edu.au/jw/brassacoustics.html (read: mutes, wah-wah)
[WIKI] "Mute (music)", Wikipedia. https://en.wikipedia.org/wiki/Mute_(music) (read; its filter-band numbers are unsourced there **[U]**)
[YN17] S. Yoshikawa, Y. Nobara, "Acoustical modeling of mutes for brass instruments", in A. Schneider (ed.), *Studies in Musical Acoustics and Psychoacoustics*, Springer (2017), pp. 143–186, doi:10.1007/978-3-319-47292-8_5 (not obtained; covers horn straight/stopping mutes and trumpet straight, cup, wah-wah mutes)

Not obtained / not found:
- **Backus 1976**, "Input impedance curves for the brass instruments", JASA 60, 470–480 (doi:10.1121/1.381104). I found
  no separate Backus 1976 paper titled "Acoustic investigations of mutes"; the mute curves, if any, are in this one.
- **Ancell 1960**, "Sound pressure spectra of a muted cornet", JASA 32, 1101–1104 (bibliographic data only).
- **Martínez-Montejo et al.**, ICA 2016 paper 399, "Characterisation of brass instruments with mutes through
  experimental means and finite-element simulations". Search snippet: the Harmon changes trumpet and trombone impedance
  the most and the cup mute the least **[U]**; the PDF host's certificate failed.
- The Puget Sound Harmon-dent study (2016).
- **N. Smith**, *History of the Horn Mute*, DMA, Eastman (1980).
- A "Watts 2009" mute thesis: the only Watts 2009 found is a UIUC report on the horn hand.
- **Kausel**: I found no mute paper. His transmission-line work (Vienna Talk 2010) concerns wall vibration.
- Element values: no lumped-element values for trumpet mutes were found in the literature. Every number in §2–§5 is my
  derivation from guessed geometry and should be refined against measured impedance curves ([F10], [YN17], [CS82]).
