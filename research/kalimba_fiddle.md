# Kalimba (GM 109) and fiddle (GM 111) as modal models — research notes

Status: 2026-11-14, research only. Nothing here is implemented. SI units.
**[G]** marks my estimates or guesses, not published values. **[D]** marks values I derived from published
formulas, tables or figures (the derivation is given). **[U]** marks a claim from a secondary source, an abstract
or a search summary that I could not check in the primary text. Numbers with none of these tags come from the cited
primary source as read. References are listed at the end (keys in brackets).

Short answer.
- **Kalimba.** One clamped-free steel strip per key, with 3–4 bending modes. The first overtone is **inharmonic: about
  5.3–6.8 × f0 as measured, against 6.27 for an ideal cantilever**. Each strip sits on a compliant bridge over a box with
  one Helmholtz resonance. A thumbnail pushes the tip down and lets it slip off. The overtone dies within a few tenths of
  a second and the fundamental rings for seconds. For the mbira, add a rattle (buzzers) and a gourd. All of this fits
  into the `Bass` machinery: a modal string becomes a modal beam, and the pluck pad becomes a nail.
- **Fiddle.** The bowed `Bass` scaled down: four strings of 0.325 m, Z0 0.18–0.36 kg/s, with the same Friedlander
  friction solve. The body changes most. Its bridge admittance has an A0 / B1− / B1+ cluster (275 / 470 / 540 Hz,
  peaks ~0.02 / ~0.15 / ~0.15 m/s/N) and a broad **bridge hill at 2–3 kHz**, and it falls 12 dB/oct above ~3–4 kHz.
  The *fiddle* (folk) style is a player model on top: steel strings, a flatter bridge, double stops and drones, short
  strokes, cuts and rolls, and little vibrato. That part is mostly qualitative in the literature.

---

## Part A. Kalimba and mbira

### A1. What the instrument is

| Item | Value | Source |
|---|---|---|
| Classification | Lamellophone, plucked idiophone (Hornbostel–Sachs 122.1) | [WPK] |
| Modern 17-key "kalimba in C" | C major diatonic, **C4–E6** (MIDI 60–88) | Sela, Stagg/Kali-Pro specs [SELA, KALIPRO] |
| Layout (17-key, left→right) | D6 B5 G5 E5 C5 A4 F4 D4 **C4** E4 G4 B4 D5 F5 A5 C6 E6 (longest in the middle, alternating sides) | common maker layout [U] |
| Hugh Tracey kalimbas | alto 15 notes, treble and celeste variants, tuned diatonically in G | [WPK, KM-ALTO] [U] |
| Traditional mbira dzavadzimu | 22–28 keys in 3 registers, on a hardwood board (gwariva), played in a gourd resonator (deze); nyunga nyunga 15 keys; njari 30–32 | [WPM] |
| Mbira tunings | Not equal tempered; the most common is nyamaropa (≈ Mixolydian). Seven-note sets are often "stretched" over more than an octave | [WPM] |
| Sustain | "about 5 seconds" (kalimbamagic) | [KM-SUS] [U] |
| GM program | 109 Kalimba (1-based) | GM standard |

### A2. The tine: geometry and the mode frequencies

**Mounting.** Each key is a thin strip. A pressure bar clamps it onto the board. In front of the bar it crosses a
**bridge** (fulcrum), and its far end (the tip, where the thumb strikes) is free. Chapman measured that the short
length **b between the fulcrum and the bar is not dead**: pressing on it while striking damps the note audibly. He
treated the length beyond the bar as fully clamped [C06].

**Measured tines (Chapman 2006, a small pentatonic kalimba) [C06, Table 1]:**

| Note (as named in [C06]) | f0 (Hz) | a = tip–fulcrum (cm) | b = fulcrum–bar (cm) | a/b | f1 (Hz) | f1/f0 |
|---|---|---|---|---|---|---|
| "D" | 304 | 5.4 | 1.4 | 3.9 | 1780 | 5.86 |
| "E" | 336 | 5.0 | 1.3 | 3.8 | 1890 | 5.63 |
| "G#" | 415 | 4.6 | 1.4 | 3.3 | 2390 | 5.76 |
| "A#" | 462 | 4.2 | 1.3 | 3.2 | 2590 | 5.61 |
| "D" | 581 | 3.7 | 1.3 | 2.8 | 3080 | 5.30 |

(The paper labels these D3…D4, but its frequencies are D4…D5.) The spectrum of the 304 Hz note shows f0, f1 = 1780 Hz, and a
"highly attenuated" peak at 3560 Hz, which is **exactly 2·f1** and not the third bending mode (≈ 17.5·f0 ≈ 5.3 kHz)
[C06, Fig. 3]. f1/f0 **falls as a/b falls** (G# is an anomaly). Chapman's earlier study of a Hugh Tracey pentatonic
kalimba gave **f1/f0 ≈ 5.9–6.8** [C08]. Other observations: "The overtone structure … was sensitive to the details of
the mounting geometry" [C06]. Ludwigsen's high-speed video of the longest treble tine (B3, 247 Hz) found **rapidly
decaying harmonic content** (integer multiples of f0) in the attack, which a thin Euler–Bernoulli beam does not predict.
He attributes it to the clamp/bridge boundary ("offset" boundary, bridge impedance) [L11, L20, L23] [U, abstracts only].

**Uniform beam, for reference [D].** Clamped-free modes: βL = 1.8751, 4.6941, 7.8548, 10.996, so the ratios are
**1 : 6.267 : 17.55 : 34.39**. In a numerical check (Hermite beam FE, my script) each of the following moves the
ratios:

| Variant of the tine (all [D], FE) | f1/f0 | f2/f0 | f3/f0 | f0 relative to cantilever of length a |
|---|---|---|---|---|
| Cantilever, length a | 6.27 | 17.55 | 34.39 | 1 |
| + back length b pinned at fulcrum, clamped at bar: b/a = 0.1 / 0.2 / 0.3 | 6.29 / 6.35 / 6.42 | 17.7 / 17.9 / 18.2 | 34.7 / 35.4 / 35.9 | 0.955 / 0.912 / 0.875 |
| + tip mass 5 % / 10 % / 20 % of the tine's free mass (the curled or thickened thumb end) | 6.35 / 6.52 / 6.97 | 17.9 / 18.7 / 20.5 | 35.4 / 37.3 / 41.4 | 0.91 / 0.84 / 0.74 |
| b/a = 0.3 with a **compliant fulcrum** (spring k = 10⁴ / 10³ / 300 · EI/a³) | 6.39 / 6.16 / 5.95 | 18.0 / 16.5 / 15.5 | 35.4 / 29.0 / 28.1 | 0.87 / 0.83 / 0.76 |

So a tip mass raises f1/f0 (up to ~7, which covers the 6.8 of [C08]). A compliant bridge lowers it (towards 5.9). Neither
alone reaches Chapman's 5.3. Fitting one bridge spring to all five tines of [C06] gave 5.8–6.0 against the measured 5.3–5.9.
So something else lowers it as well: the tine's non-uniform cross-section, its curvature, or the board's motion [G].
**For a model, make f1/f0 a per-tine parameter in 5.3–6.8 (default ~5.9) and f2/f0 ≈ 15–18 [G]**, rather than
deriving them from a beam.

**Thickness and length [D].** For a steel strip (E ≈ 200 GPa, ρ = 7850 kg/m³) the cantilever formula is
f0 = (1.8751²/2π)·(h/a²)·√(E/12ρ) ≈ **815 · h/a²** (h and a in m). With the back length lowering f0 by ~0.875, Chapman's
304 Hz tine at a = 54 mm needs **h ≈ 1.2 mm**. At that thickness a 17-key C4 tine has a ≈ 57 mm and an E6 tine
a ≈ 25 mm. That matches a retailer's "vibrating length about 2–7 cm, total length 7–12 cm" [IDM] [U]. One DIY builder
used 5 × 0.7 mm strip [ITR] [U], and Hokema treble tines are 4 mm wide [KM-B17] [U]. **Use w ≈ 4–6 mm and h ≈ 1.0–1.3 mm
[G]**. Tuning is by length (the player slides the tine through the bar). Over the 17 keys the length ratio is √(f_E6/f_C4) = 2.24.

**Other motions of the strip [D]**, for w = 4 mm, h = 1 mm, a = 55 mm:
- The in-plane (sideways) bending modes sit at **(w/h)·f0 ≈ 4·f0**. A thumb that pushes a little sideways excites them.
  They are probably part of the "integer-ish" attack content [G].
- The first torsion mode is at ≈ 7 kHz (≈ 26·f0), which is unimportant.

**Mass and stiffness, for the pluck [D].** w = 5 mm, h = 1.2 mm: EI = 0.144 N·m². The C4 tine (a = 57 mm) weighs about
2.7 g in its free part, and its first mode's effective mass at the tip is ≈ 0.25 × that ≈ 0.7 g. Its tip stiffness
3EI/a³ is ≈ 2.3 kN/m, so a 2 mm push takes ~5 N. The E6 tine (25 mm) has a tip stiffness of ≈ 28 kN/m, so the same
thumb force gives only ~0.2–0.3 mm. **The pluck depth therefore scales roughly as a³ for a constant thumb force.**

### A3. Damping per mode

Nobody seems to have published measured decay times. Chapman synthesised the notes as **two damped sinusoids** and set
the overtone's decay "by guesswork and listening": "the overtone would decay quicker than the fundamental" [C06]. The
"anharmonic overtones die away rapidly … heard only at the instant the note starts" [C08].

| Quantity | Value | Basis |
|---|---|---|
| Fundamental T60, mid tines | **2–4 s** | [D] from [C06] Fig. 2: the 304 Hz envelope falls ~14 dB from 0.2 to 1.0 s, which gives T60 ≈ 3.4 s. Q = πf·T60/6.91 ≈ 470 |
| Fundamental T60, outer (short, high) tines | shorter, ~1–2 s | [G]. Players report that the outer tines are "drier" [RI] [U] |
| f1 (first overtone) T60 | **0.1–0.4 s**, i.e. Q ≈ 100–300 | [G], consistent with [C06]/[C08] |
| f2, f3 | < 0.1 s | [G] |
| Steel's own loss | η ~ 10⁻⁴ (spring steel is chosen for low internal damping) | [VT] [U]. So the bridge, bar, board and air set the decay |
| Attack transients: harmonics of f0 and 2·f1 | decay within tens of ms | [L11, C06]. Size: [G] −20…−30 dB under f0 |

A simple loss law for a modal tine: **σ_k = σ0 + c·f_k^p with p ≈ 1.5–2 [G]**. That makes the overtones' decay grow
roughly with frequency squared, the same form as the string law rn = B1 + B2(n−1)² in [P13].

### A4. The box and the board

| Variant | What it adds | Numbers |
|---|---|---|
| **Board-mounted** (Hugh Tracey "board resonator", nyunga nyunga) | Plate modes of a ~1 cm board. No air resonance. Weak low end | none measured. The board's first modes are probably 300–800 Hz [G] |
| **Box** (most 17-key kalimbas; Tracey alto and treble boxes) | A Helmholtz resonance through the front hole, plus the top's plate modes. Two back holes that the fingers open and close give the "wah" | Tracey boxes: **alto ~440 Hz, treble ~490–520 Hz** [UBC] [U]. An attack phase of ~0.05 s is mentioned [UBC] |
| **Deze** (gourd, mbira) | A large calabash, ~40 cm. It "amplifies the sound, adds some buzz and distortion, and shortens the notes' duration"; some notes or overtones become more pronounced | [MBO, WPM] [U]; no frequencies found |

**Helmholtz estimate for a 17-key box [D]:** f_H = (c/2π)·√(A/(V·L_eff)). Interior 16 × 11 × 2.5 cm, so V ≈ 0.44 L
[G]. Front hole 30 mm, so A = 7.1 cm² [G]. Plate 5 mm, so L_eff = t + 1.7r ≈ 30 mm. Result: **f_H ≈ 400 Hz**, in the
range of the Tracey values. Covering the back holes (two ~12 mm holes [G]) lowers f_H. Opening them raises it by up
to ~20 %. The wah is the player sweeping f_H with the fingers, a few Hz. The Q of a small wooden box's Helmholtz mode is
**~10–30 [G]**. Wall losses and the compliance of the thin walls keep it low.

**Sympathetic tines.** A plucked tine sets its neighbours ringing ("adjacent tines also create secondary vibrations")
[WPK]. Tines whose f0 or f1 matches another tine's mode ring audibly. Model it the way `Bass` does with its strings: all
tines are driven by the bridge's motion.

### A5. Buzzers ("mbira buzz")

- **Hardware:** bottle caps loosely fixed to a metal plate or rod on the board, or on the deze rim (historically snail or
  cowrie shells), the *machachara* [WPM, GRIN] [U]. Also loose rings around the tines, or wire wrapped on the bridge.
- **Physics [D]:** each buzzer is a small mass held by gravity or a weak preload against the board. It **lifts off when
  the board's acceleration at that point exceeds g** (or preload/mass). It then flies and re-impacts, a one-sided contact
  much like `Bass`'s fingerboard. The rattle is broadband, with a level that rises steeply above the threshold. It follows
  the note's envelope and stops when the note has decayed below the threshold. Nothing measured was found on buzzer masses,
  contact stiffness or spectra.
- **Model [G]:** 2–6 rattlers, each 0.5–3 g, on a board point with a contact stiffness of 10⁵–10⁶ N/m and loss
  (Hunt–Crossley). Thresholds are a few m/s² of board acceleration, so loud notes buzz and soft notes do not.

### A6. The pluck

- **Gesture:** the thumbnail (or the thumb's pad, for a softer tone) lands on the tip and pushes it down towards the
  board, by **~1–3 mm on long tines and ~0.2–0.5 mm on short ones** (equal force, §A2) [G/D]. The tine then slips off
  the edge of the nail. The push lasts about 10–50 ms [G]. Release by slip-off is fast: **0.05–0.3 ms for the nail and
  ~1 ms for the pad** [G]. (A hobby firmware uses 0.05–1.2 ms [FIMBA] [U].)
- **Where:** at or near the tip, so all bending modes are excited (the tip is an antinode for every mode). The overtone
  share comes from the release time (a fast slip excites f1 at 6·f0 strongly) and from the nail's contact area. Gesture
  sets the f1:f0 balance, and Chapman's spectra show f1 only ~10–20 dB below f0 at the attack [C06] [D from Fig. 3].
- **Nail noise:** a click when the nail strikes and when it slips off [G]. The nail also stops a ringing tine when the
  thumb returns to it (as in `Bass::pluck`).
- **Pitch glide:** a hard pluck shows little pitch drop, because the tine is stiff and its amplitude is small compared
  with its length. Any glide comes from the bridge contact opening and not from tension [G].

### A7. Proposed model (fits the `Bass` pattern)

- Per tine: **4 modes** (f0, f1 = r1·f0, f2 = r2·f0, plus a "2·f1-ish" or in-plane mode at ~4·f0 for the attack), each a
  2-state resonator. Each has its own decay (§A3) and its shape value at the tip (all ±1) and at the bridge (the force
  into the board).
- The bridge and board as a few modes (box: Helmholtz + 2–3 plate modes, §A4), driven by the sum of the tine forces and
  feeding back to all tines (sympathetic ringing).
- The nail as a stiff spring that pushes the tip to a depth and lets go over the release time, reusing the finger logic
  of `Bass::pluck`.
- Buzzers as one-sided contacts on the board (mbira).
- Cost: 17 tines × 4 modes plus ~5 board modes is trivially cheap, so all tines can always run.

---

## Part B. Fiddle (folk violin)

### B1. Strings

| String | f0 (Hz), MIDI | μ (g/m) | T (N) [D] | Z0 = μc (kg/s) [D] | Inharmonicity B (Percival, 5 violins) | Decay r1 / r10 (1/s) (Percival, plucked, on the instrument) |
|---|---|---|---|---|---|---|
| G3 | 196.0, 55 | 2.8 (range 2.1–3.1) | 45 | 0.36 | 1.9–3.7 × 10⁻⁴ | 0.4–1.0 / 9–25 |
| D4 | 293.7, 62 | 1.29 (steel core, Schoonderwaldt) | 47 | 0.25 (meas. 0.25) | 2.3–4.6 × 10⁻⁴ | 1.0–1.7 / 18–46 |
| A4 | 440.0, 69 | 0.72 | 59 | 0.21 | 1.6–2.5 × 10⁻⁴ | 1.7–2.5 / 25–39 |
| E5 | 659.3, 76 | 0.41 (plain steel 0.26 mm) | 75 | 0.18 (meas. 0.18) | 4.5–5.1 × 10⁻⁵ | 2.8–3.7 / 9.5–15 |

- **Length.** Scale 328 mm is the luthiers' standard [U]. The measured speaking lengths of 5 violins were **319–326 mm**
  [P13, Table 3.2]. The derived columns use L = 0.325 m: c = 2Lf0, T = μc², Z0 = μc.
- **Tension.** Bounds from Rossing (via [P13, Table 3.6]): E 71–91 N, A 48–63 N, D 34–61 N, G 35–51 N. Total ≈ 220 N [D].
- **μ.** E and D are measured [SGA08]. G and A are within the bounds of [P13].
- **Damping law.** Demoucron's law, used in [P13]: **r_n = B1 + B2·(n−1)²** (1/s; amplitude decays as e^(−r_n t)).
  Fitted values: violin G B1 ≈ 1.0–1.9, B2 ≈ 0.11–0.29; A B1 ≈ 3–4.6, B2 ≈ 0.27–0.44; E B1 ≈ 5.3–7.6, B2 ≈ 0.02–0.08
  [P13, Table 3.5]. Woodhouse's physical alternative has loss factors for internal friction, bending and air [P13 eq. 3.11].
- **Torsion.** The torsional impedance Z_R is **2–4 × Z0** [SGA08, citing Schumacher]. Torsional modes are much more
  damped than transverse ones [W14 §2.3.1]. The existing bass needed torsion for Helmholtz motion to appear at all
  (research/double_bass.md §7e), so keep it.
- **Fiddle strings.** Fiddlers often use **steel strings** for a brighter sound [WPF]. All-steel strings have lower B2,
  so their upper partials ring longer, and a smaller μ on the G and D strings for the same pitch [G].

### B2. The body as the string sees it (bridge admittance, bowing direction)

From Woodhouse's review [W14] (Fig. 1: two violins, bridge drive-point admittance in dB re 1 m/s/N; Fig. 16/18 mode
frequencies), Bissinger [B12 via search, B03] and Gough [G16]. Effective masses are [D] from m = Q/(|Y|_peak·ω).

| Feature | f (Hz) | Q / damping | Peak \|Y\| at bridge | Effective mass at bridge [D] | Radiation |
|---|---|---|---|---|---|
| **A0** (f-hole Helmholtz, coupled to the breathing mode) | **~275** (243–290; Bissinger ≈ 275 [B12] [U]; ≈ 286 [B03] [U]) | ζ = **2.4–2.7 %**, so Q ≈ 19–21 [B03] [U]. A model in [B12] uses 2.5 % [U] | ≈ −33 dB ≈ **0.02 m/s/N** [D, W14 Fig. 1] | ~0.5 kg | strong monopole, the bottom of the sound |
| **CBR** (centre-bout rotation) | 380–440 (436 [W14 Fig. 16]; 376–394 without neck [W14 Fig. 18]) | ~30–50 [G] | small (a weak drive point) | — | weak radiator |
| **B1−** (corpus bending/breathing) | **430–480** (≈ 470 [B12] [U]; 454 [W14]) | **25–50** (typical shell-mode Q) [G16] | **−16…−20 dB ≈ 0.1–0.16 m/s/N** [D, W14 Fig. 1] | ~70–120 g | strong |
| **B1+** | **530–560** (≈ 540 [B12] [U]; 538 [W14]) | 25–50 [G16] | **≈ −16 dB ≈ 0.15 m/s/N** [D] | ~70 g | strong |
| "Signature" range | up to ~1 kHz: A0, B1−, B1+ dominate | — | — | — | — |
| Transitional cluster | 800–1500 Hz: many strong overlapping modes, a "hump" near 1 kHz | — | −25…−35 dB | — | — |
| **Bridge hill** | **2–3 kHz**, broad. Coincidence frequency 3–4 kHz | hill Q ≈ 2–4 [D] | peaks −15…−22 dB (0.08–0.18 m/s/N); the averaged Y_dir is ≈ −25 dB [D, W14 Figs. 1, 22b] | — | above it the response **falls ~12 dB/oct** [G16 p. 29] |
| Free bridge (on a rigid base) | in-plane **rocking ~3 kHz** and bouncing ~6 kHz; strongly damped once on the top | — | — | — | — |

- **Mechanism of the hill.** The plate near the bridge behaves like a dashpot (an infinite plate's point admittance is
  real and flat). Driving it through a local mass–spring (bridge bending plus the "island" between the f-holes twisting)
  gives the hill [W14 §3.3.3, W05].
- **Lumped model [D].** From W14 Fig. 22b, Y_dir ≈ −35 dB ≈ 0.018 m/s/N at low frequency, so the plate dashpot is
  **R ≈ 55 kg/s**. (Check: an infinite spruce plate with h = 3 mm has Z = 8√(Dρh) ≈ 40 kg/s.) Take a hill at f_b ≈ 2.5 kHz
  with Q ≈ 3.5. Then **m_b = R/(Q·ω_b) ≈ 1 g** and k = m_b·ω_b² ≈ 2.5 × 10⁵ N/m.
- **Mute.** A mute is added bridge mass: it moves the hill down and cuts the highs [W14, G16]. f_b scales as
  √(m_b/(m_b + m_mute)). A 2 g mute moves 2.5 kHz to ~1.4 kHz [D].
- **How it shapes the sound.** The sawtooth bridge force (harmonics ∝ 1/n) is multiplied by the admittance and by the
  body's radiation. Below ~1 kHz the body radiates as a monopole (the Helmholtz number passes 1 near 1 kHz). The modal
  overlap exceeds 1 at 1–2 kHz, so above that a smooth response (Y_dir) is a fair description [W14 §3.1–3.2]. The
  coincidence frequency is 3–4 kHz [B08 via search] [U]. So G-string notes get their fundamental mostly from A0/B1, mid notes get the B1 peaks
  directly, and the hill gives every note its 2–4 kHz "brilliance". Above ~4 kHz there is roll-off [G16].
- **Listener sensitivity.** The best listeners detect a **1 % shift in body-mode frequency, 20 % in body-mode damping,
  and a factor 3 in string damping** [P13 §1, citing perceptual studies]. So body mode frequencies matter more than their Q.
- **Wolf notes.** These come from strong string–body coupling near a body peak. They are rare on the violin and worse on
  cellos [W14 §2.2.4]. A modal body coupled at the bridge gives them for free.

### B3. Bowing parameters

**Helmholtz motion and its limits.** β = bow–bridge distance / string length. The Schelleng limits are [SGA08 eqs. 1–2]:

```
F_max = 2 Z0 v_B / (Δμ β)        F_min = Z0² v_B / (2 R Δμ β²)       Δμ = μs − μd
```

The **measured violin D string (Z0 = 0.25 kg/s)** on a bowing machine, with 10 mm of hair, carbon bow and Pirastro rosin
[SGA08 Table I], fits F_max = c_up/β and F_min = c_low/β²:

| v_B (m/s) | c_up (mN) | c_low (mN) | Δμ from c_up |
|---|---|---|---|
| 0.05 | 66 | 0.43 | 0.38 |
| 0.10 | 86 | 0.47 | 0.58 |
| 0.15 | 125 | 0.38 | 0.60 |
| 0.20 | 166 | 0.38 | 0.60 |

- **Example [D].** At 0.1 m/s and β = 1/10 the window is 0.047–0.86 N. At β = 1/20 it is 0.19–1.7 N. At β = 1/30 it is
  0.42–2.6 N.
- **Torsion.** With Z_tot = Z0·Z_R/(Z0+Z_R) ≈ 0.17 kg/s, Δμ ≈ 0.4 [SGA08].
- **Slopes.** The upper limit's slope was only **−0.5** (not −1) at 5–10 cm/s. The lower limit's slope matched −2.
- **Implied R [D].** R = Z0²v/(2Δμ·c_low) ≈ 11 kg/s.
- **Compare the existing bass [D].** The bass model's measured window, (0.01+0.68β)Zv/β² … (0.5+8β)Zv/β
  (double_bass.md §7e), gives 0.20–0.33 N for these D-string values, much narrower than the real 0.05–0.86 N. Worth
  checking whether the bass's torsion or band limit narrows it.

**Players' ranges (Schoonderwaldt 2009, experts, via [P13 §2.1]):**
- bow–bridge distance **5–60 mm** (β ≈ 0.015–0.18)
- bow speed **0.05–1.0 m/s**
- bow force **0.1–2 N**

Beginners (Percival's estimate): 20–70 mm and ≤ 0.5 m/s. Dynamics: a crescendo comes mainly from a smaller bow–bridge
distance and/or a higher speed, and the force rises with them [SSW07] [U]. Timbre is controlled mainly by force, with
distance second [DAC] [U].

**Sizes of the motion [D]:**
- Helmholtz amplitude at mid-string: v_B/(8 f0 β). For A4 at 0.3 m/s and β = 0.1 that is 0.85 mm; for G3 at 0.5 m/s it is 3.2 mm.
- The bridge force sawtooth's jump is Z0·v_B/β. For A4 that is 0.62 N peak-to-peak.

**Friction curves (choose one):**

| Model | Formula and parameters | Source |
|---|---|---|
| Hyperbolic (McIntyre–Woodhouse) | μ = μd + K/(z + z0) with z = slip speed. K = 0.08 m/s, μd = 0.4, z0 = 0.2 m/s, so μs = 0.8 | [SGA08 §3.5] |
| Hyperbolic (Percival, from Demoucron) | μs = 0.8, μd = 0.3, slope μc ≈ 0.2 (×U(0.95,1) noise each sample). Fitted per string: μs 0.65–0.91, μd 0.24–0.35, μc 0.08–0.20 | [P13 Fig. 2.7, Tables 3.7, 3.10] |
| Exponential (Smith & Woodhouse 2000 steady sliding) | μ(v) = 0.4·e^(−\|v\|/0.01) + 0.45·e^(−\|v\|/0.1) + 0.35, so μs = 1.2 and μ(∞) = 0.35 | used by [DB15] |
| Thermal (Woodhouse 2003) | Friction set by the contact temperature: rosin melts during slip, so there is hysteresis in the f–v plane and the steady-sliding curve is not followed dynamically | [W14 §2.4] |
| The existing bass | 0.9 static, falling to 0.35 (half-way at 8 cm/s) | double_bass.md §7e |

The bass's curve is already in the range of [SGA08]/[P13], so **reuse it for the violin**. Δμ ≈ 0.4–0.6.

**Bow hair.** The ribbon is ~10 mm wide [SGA08] (the bass uses 16 mm). Bow mass 58 g [SGA08]. A finite width causes
differential slipping, a noise component that players control with tilt [W14 §2.2.3].

**Flattening.** At high force the note plays flat because of the friction hysteresis (strongest high up on thick
strings) [W14 §2.2.2]. The bass model already shows it.

### B4. Fiddle (folk) style compared with classical

The acoustics literature has **no measured study of fiddle bowing**, as far as I found. The descriptions are from
general sources [WPF, VL, YAM] and are tagged [U]; the numbers are [G].

| Aspect | Classical | Fiddle (Irish / Scottish / old-time / bluegrass) | Model parameter [G] |
|---|---|---|---|
| Strings, setup | synthetic or gut core | often **steel**, brighter [WPF]. **Flatter bridge** for double stops and the double shuffle [WPF] | steel losses; string crossing angle per stroke smaller (~10° rather than ~15°) [G] |
| Strokes | long, sustained | **short, rhythmic strokes**: one stroke per note or slurred pairs or triplets, shuffle patterns, syncopation (Appalachian), staccato (Donegal) [WPF, VL] [U] | stroke 0.1–0.3 s; speed 0.3–1 m/s; frequent reversals |
| Force and place | full Schelleng range | Drive is often described as "digging in": higher force for the speed, nearer the edge of the Helmholtz window, more crunch at the starts. Not measured [G] | pressure 0.6–0.9 of the window; β 0.06–0.12 |
| Double stops and drones | occasional | **heavy use of drones and double stops** (Appalachian, Sliabh Luachra) [WPF]. An open string under the melody | bow on two strings: split the force ~50/50 by bow tilt; keep the drone string sounding |
| Vibrato | almost continuous, ~5.5–7 Hz | **sparse**: "classical … tends to contain more vibrato and sustained notes" [WPF]. Used in slow airs and Texas/swing styles | off by default; on long notes 5–6 Hz, ±10–20 cents, delayed onset ~0.3 s |
| Ornaments | trills, grace notes | **cuts and strikes** (one very short upper or lower grace note), **rolls** (note, cut, note, strike, note), **bowed trebles** (three quick bow reversals on one note), slides into notes. Irish flute cuts and strikes last **mean 63 ms (95 %: 32–95 ms)** against 209 ms for notes [ISMIR15] [U]. A fiddle onset study calls ornaments "at most a fifth" of a note [KEL05] [U] | cut = left-hand finger lifted for 30–60 ms; treble = three strokes of ~40–60 ms; slide = pitch glide 50–150 ms from −100…−200 cents |
| Percussive effects | — | the bluegrass "chop": a heavy, short bow drop near the frog, mostly noise [G] | force far above F_max for ~50 ms, low speed |

**Vibrato data (violin, classical) for reference.** Four soloists averaged **6.63 Hz with a width of 63 cents**
[GER] [U]. Typical extents are 20–35 cents [MMM03] [U]; Fletcher & Sanders give ~30 cents (10–60) [U]. One
concertmaster measured 5.7 Hz in 1st position and 6.3 Hz in 6th [U].

### B5. Existing real-time bowed-string models

| Model | Structure | Parameters worth taking |
|---|---|---|
| **STK `Bowed`** (Smith 1986 waveguide after McIntyre–Schumacher–Woodhouse; Cook & Scavone; body filter by Maestre 2011) [STK] | Two delay lines (bridge side β, neck side 1−β), with base delay fs/f − 4. The bow is a memoryless table: out = (\|(in+offset)·slope\| + 0.75)^−4 clipped to [0.01, 0.98]. slope = 5 − 4·pressure (default 3), offset 0.001. Bridge reflection: one-pole lowpass with gain 0.95 and pole 0.75 − 0.2·22050/fs. Default β = 0.127. Vibrato 6.13 Hz, gain up to 0.4 | **body: 6 biquads in cascade** (coefficients in Bowed.cpp: a fitted violin body response). A cheap output filter if the modal body is too costly |
| **Demoucron (KTH thesis 2008)**, extended by Percival 2013 ("Artifastring") [DEM, P13] | **Modal string** (N = 40 modes) with exact per-mode updates (X1..X3, Y1..Y3 coefficients). The bow is a point with a hyperbolic friction curve. The finger is a damped spring (or two or three), and plucks are springs too. Bridge force F = Σ a_n G_n. Body as **measured impulse-response convolution** at 22.05 kHz. A bow-force controller learned from timbre | per-string T, L, d, μ, E, μs, μd, μc (Table 3.10); measured B and r_n (Tables 3.3, 3.5). It is the same architecture as `Bass`, so this is the closest relative |
| **Desvages & Bilbao 2015/16** [DB15, DB16] | Finite differences, **two polarisations**. The bow hair, fingers and fingerboard are nonlinear (Hunt–Crossley) contacts in the normal direction, with friction in the tangential one. Energy-conserving. 44.1 kHz. Uses Percival's string table | the exponential friction curve above; the bow as a mass with its own normal and tangential forces, driven by gesture breakpoints. The fingerboard contact gives slap and buzz |
| **Woodhouse and colleagues** [W14] | Green's-function (waveguide-like) string with measured body admittance, finite bow width, torsion, thermal friction | Schelleng and Guettler diagrams for transients; thermal friction for realism at the starts |

### B6. Proposed fiddle model (from `Bass`)

- **Strings.** `Bass` with 4 strings re-parameterised (§B1): L = 0.325 m; 32–48 modes per direction. At E5 the mode
  frequencies climb faster than on the bass, so fewer modes fit under the 3.8 kHz bow band limit: the E string has only
  ~5 harmonics below it. **The bow band limit must rise to ~8–10 kHz for the violin** [G]. Bowing can probably be done
  at 2× oversampling for the E and A strings only, as Percival does with per-string rates [P13 §2.1].
- **Body.** Either the `BassBody` plate-and-air grid shrunk to violin size (0.355 m body, 2 L of air, f-holes) and
  calibrated to §B2, or a **modal body**: ~10 low modes (A0, CBR, B1−, B1+, A1 ~470–500 Hz, a few at 0.7–1.5 kHz) with
  the Q and effective mass of §B2, plus a high-frequency band as a dashpot behind the bridge mass and spring (the hill).
  The modal body is far cheaper and easier to fit to the numbers here.
- **Bridge.** 2 g with a rocking spring set for 2.5–3 kHz. Add a mute as extra mass.
- **Player.** The bass's `BowPlayer` logic, with the fiddle defaults of §B4. Drones are a second string bowed with a
  share of the force. Ornaments are left-hand events of 30–60 ms.
- **Range.** G3–E6 (MIDI 55–88) covers fiddle repertoire (first to third position, mostly below B5) [G]. Classical
  violin reaches ~A7.

---

## What is guessed (most important first)

1. Kalimba damping: the T60 of each mode, especially the overtones and the outer tines (no measurements found).
2. Kalimba tine thickness and width for a commercial 17-key instrument (derived from one small kalimba's f0 and length).
3. Box Helmholtz frequency and Q for a 17-key box (estimated from guessed dimensions). The Tracey values are [U].
4. Buzzer masses, thresholds and contact stiffness. Gourd (deze) acoustics.
5. Thumbnail release time and push depth.
6. Violin body Q per mode (only A0 is sourced) and the CBR level. Effective masses are derived from figure readings.
7. Everything numeric about fiddle-style bowing (force, β, stroke lengths, vibrato use). Ornament durations are from
   Irish flute, not fiddle.

## References

[B03] G. Bissinger, "Modal analysis of a violin octet", JASA 113(4), 2105 (2003). A0 damping 2.4–2.7 % via search summary [U]. https://www.researchgate.net/publication/10796782_Modal_analysis_of_a_violin_octet
[B08] G. Bissinger, "Structural acoustics of good and bad violins", JASA 124(3), 1764–1773 (2008). Not read; the host strad3d.org is suspended. Via [G16], search summaries [U]. http://strad3d.org/files/bissinger/JASA%202008%20Struct%20Acoust%20good%20bad%20violins.pdf
[B12] G. Bissinger, "Parametric plate-bridge dynamic filter model of violin radiativity", JASA 132 (2012). Values via search summary [U]. https://strad3d.org/files/bissinger/JASA%202012%20dynamic%20filter%20model.pdf
[C06] D. M. F. Chapman, "Investigating the sound of an African thumb piano (kalimba)", Canadian Acoustics 34(3), 26–27 (2006). Read in full (Table 1, Figs. 2–3). https://jcaa.caa-aca.ca/index.php/jcaa/article/view/1805
[C08] D. M. F. Chapman, "The sound of the African thumb piano (kalimba)", ASA 155th meeting / Acoustics'08 Paris, popular version. https://acoustics.org/pressroom/httpdocs/155th/chapman.htm
[DAC] M. Demoucron, A. Askenfelt, R. Caussé, "Measuring bow force in bowed string performance" (sensor paper). https://hal.archives-ouvertes.fr/hal-00444903/file/DemBowForce.pdf [U]
[DB15] C. Desvages, S. Bilbao, "Two-polarisation finite difference model of bowed strings with nonlinear contact and friction forces", Proc. DAFx-15, Trondheim (2015). Read (friction curve, setup). https://www.ntnu.edu/documents/1001201110/1266017954/DAFx-15_submission_71.pdf/680773a8-e63b-4bf8-8e15-e83f8b75f96b
[DB16] C. Desvages, S. Bilbao, "Two-polarisation physical model of bowed strings with nonlinear contact and friction forces, and application to gesture-based sound synthesis", Applied Sciences 6(5), 135 (2016). Not read (403). https://www.mdpi.com/2076-3417/6/5/135
[DEM] M. Demoucron, "On the control of virtual violins: physical modelling and control of bowed string instruments", PhD thesis, KTH / UPMC (2008). Via [P13]. https://www.avhandlingar.se/avhandling/fc2579831a/
[FIMBA] FiMba-1 kalimba firmware (hobby; its mode ratios and pluck times are design assumptions). https://github.com/jadamsowers/fm1-fimba [U]
[G16] C. Gough, "Violin acoustics", Acoustics Today (Summer 2016; volume/pages unverified). Pages 27–30 read (HTML). https://acousticstoday.org/issues/2016AT/2016Summer/files/basic-html/page27.html
[GER] "A pilot study of relationships between pitch register and dynamic level and vibrato" (Bell, Midori, Mutter, Perlman), via search summary [U]. https://experts.illinois.edu/en/publications/a-pilot-study-of-relationships-between-pitch-register-and-dynamic/
[GRIN] Grinnell College Musical Instrument Collection, mbira entries (buzzers). https://omeka-s.grinnell.edu/s/MusicalInstruments/item/1828 [U]
[IDM] Instruments du Monde, "Kalimba notes" (tine lengths 2–7 cm vibrating, 7–12 cm total). https://instruments-du-monde.com/en/blogs/kalimba/kalimba-notes [U]
[ISMIR15] ISMIR 2015 paper 106 on Irish flute ornaments (cut and strike durations). https://www.ismir2015.uma.es/articles/106_Paper.pdf [U, via search summary]
[ITR] Instructables, "Trash Kalimba Musical Instrument" (5 × 0.7 mm tines). https://instructables.com/Trash-Kalimba-Musical-Instrument [U]
[KALIPRO] Stagg/Kali-Pro 17 spec sheet (C4–E6). https://www.bax-shop.nl/downloads/products/9000-0153-7301/kalipro17oce.pdf [U]
[KEL05] Kelleher et al., onset detection for Irish fiddle ornaments (2005). https://mural.maynoothuniversity.ie/id/eprint/9762/1/BL-Onset-2005.pdf [U]
[KM-ALTO] Kalimba Magic, "What is the difference between Alto Kalimba, Treble Kalimba, 17-Note Kalimba in C?". https://www.kalimbamagic.com/blog/item/what-is-the-difference-between-alto-kalimba-treble-kalimba-17-note-kalimba-in-c [U]
[KM-B17] Kalimba Magic, "Hokema B17" (4 mm tines, 8 mm spacing). https://www.kalimbamagic.com/blog/item/hokema-b17-top-notch-17-note-kalimba-in-c [U]
[KM-SUS] Kalimba Magic (sustain ~5 s). https://kalimbamagic.com/?p=164 [U]
[L11] D. Ludwigsen, "Video analysis and modeling of the kalimba tine", JASA 130(4) Suppl., 2509 (2011), abstract. doi:10.1121/1.3654995 [U]
[L20] D. Ludwigsen, J. Huizar, "Free vibration of a kalimba tine model beam with offset boundary condition", JASA 148, 2642 (2020), abstract. https://digitalcommons.kettering.edu/naturalsci_facultypubs/5 [U]
[L23] D. Ludwigsen, "Kalimba tine boundary condition models", JASA 153(3) Suppl., A227 (2023), abstract. doi:10.1121/10.0018731 [U]
[MBO] mbira.org, "Deze (mbira resonator)". https://mbira.org/?p=8770 [U]
[MMM03] "Vibrato: questions and answers from musicians and science" (MMM 2003). https://www.mcg.uva.nl/mmm-2003/papers/mmm-35/mmm-35.html [U]
[P13] G. K. Percival, "Physical modelling meets machine learning: performing music with a virtual string ensemble", PhD thesis, Univ. of Glasgow (2013). Read: Ch. 2 (modal algorithm), Ch. 3 (Tables 3.2, 3.3, 3.5, 3.6, 3.7, 3.10). https://theses.gla.ac.uk/4253/1/2013percivalphd.pdf
[RI] rareinstrument.com kalimba guide (outer tines drier). https://rareinstrument.com/kalimba/ [U]
[SELA] Thomann, Sela SE 246 Kalimba 17 (c'–e''', C4–E6). https://www.thomannmusic.com/sela_se_246_kalimba_black_17.htm [U]
[SGA08] E. Schoonderwaldt, K. Guettler, A. Askenfelt, "An empirical investigation of bow-force limits in the Schelleng diagram", Acta Acustica u. A. 94, 604–622 (2008), doi:10.3813/AAA.918070. Read (§2–3, Tables I–II). https://knutsacoustics.com/files/schoonderwaldt-et-al.-empirical-schelleng.pdf
[SSW07] E. Schoonderwaldt, S. Sinclair, M. Wanderley, "Why do we need 5-DOF force feedback?" (Enactive 2007; bowing measurements). https://idmil.org/wp-content/uploads/2022/06/Schoondw2007_enactive.pdf [U]
[STK] P. R. Cook, G. P. Scavone, STK `Bowed.cpp`, `BowTable.h`. Read. https://github.com/thestk/stk/blob/master/src/Bowed.cpp
[UBC] UBC PHYS 341 wiki, "Kalimba resonator" (Tracey alto ~440 Hz, treble ~490–520 Hz). https://wiki.ubc.ca/Course:PHYS341/KalimbaResonator [U]
[VL] Violin Lounge, "Fiddle vs violin: different music, same instrument". https://violinlounge.com/blog/fiddle-vs-violin-different-music-same-instrument [U]
[VT] "Vibrating tine metallurgy" (spring steel, low internal damping). https://vectree.io/pdf/c/vibrating-tine-metallurgy [U]
[W05] J. Woodhouse, "On the 'bridge hill' of the violin", Acta Acustica u. A. 91, 155–165 (2005). Via [W14].
[W14] J. Woodhouse, "The acoustics of the violin: a review", Rep. Prog. Phys. 77, 115901 (2014). Read (author's version, 13/8/14): §2 (bowed string, friction), §3.3 (admittance, bridge hill), Figs. 1, 16, 18, 22. https://www.repository.cam.ac.uk/handle/1810/245817
[WPF] Wikipedia, "Fiddle". https://en.wikipedia.org/wiki/Fiddle
[WPK] Wikipedia, "Kalimba". https://en.wikipedia.org/wiki/Kalimba
[WPM] Wikipedia, "Mbira". https://en.wikipedia.org/wiki/Mbira
[YAM] Yamaha Hub, "What's the difference between fiddle and violin". https://hub.yamaha.com/strings/s-acoustic/whats-the-difference-between-fiddle-and-violin/ [U]

Not obtained:
- T. D. Rossing, *Science of Percussion Instruments* (World Scientific, 2000). Its lamellophone section is not online.
- Any "Hilarides" kalimba measurement (none found).
- Chapman's 2012 JASA abstract and full Acoustics'08 paper (tine a/b model).
- Bissinger's full papers (strad3d.org is suspended and the Wayback Machine is blocked to this client).
- Askenfelt 1989 (JASA 86, 503) bowing ranges by dynamic level.
- Demoucron's thesis text.
- Any measured study of fiddle (folk) bowing.
