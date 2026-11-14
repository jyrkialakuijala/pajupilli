# Steelpan (GM 115) and taiko (GM 117): modal models — research notes

Status: 2026-11-14. Research only, nothing implemented. Extends `research/drums.md` §4.4 (modal banks struck
by a Hunt–Crossley contact, tension modulation, two heads coupled through air). SI units unless marked.

Tags on every number: **[D]** data: quoted from the cited primary source I read (the key says which), or
derived by me from such data or a formula (marked "derived"); **[U]** from a secondary source, an abstract or a
snippet, not checked in a primary text; **[G]** my guess or estimate.

**Main results**
1. **Steelpan nonlinearity, measured.** Monteil, Thomas & Touzé [MTT15] fitted a 3-mode **1:2:4 quadratic
   model** to forced-vibration measurements of a G3 note. The coefficients are **α5 = 0.24, α6 = 0.34,
   α7 = 0.88, α8 = 0.53** [D], in units where the displacement is in mm and the time in fundamental radians. The
   same model with an initial displacement of 0.15 mm (mezzo-piano) or 0.65 mm (forte) reproduces the measured
   envelopes of a stick-struck note. My re-simulation of it (§1.4) shows the octave blooming 10–50 ms after
   the hit and overtaking the fundamental [D].
2. **Steelpan modes and damping.** On two double-second notes [MTT15], modes sit at **1 : 1.97 & 2.01 : 3.99 &
   4.05 : 4.8 : 5.85 : 7.7** [D], with damping ratios **ξ = 0.001–0.006** [D]. That is Q ≈ 100–550 and T60 ≈ 0.3–1.5 s.
   Maloney measured Q = 50–800 across a soprano (tenor) pan, mostly 100–300 [D]. The stick contact time is
   **1–2 ms** on a tenor [D].
3. **Taiko.** A real 48 cm nagado-daiko with one head has modes at **127 Hz × 1 : 1.567 : 2.138 : 2.266 : 2.611
   : 2.734 : 2.985 : 3.295** [D] (practically an ideal membrane). With two heads, the (0,1) mode splits around
   the uncoupled frequency f0: the in-phase mode moves to **0.975 f0** and the out-of-phase mode to **1.059 f0** [D].
   Measured decays: the (0,1) mode falls at **−44 dB/s** (T60 ≈ 1.35 s), the (1,1) at **−22 dB/s** (T60 ≈ 2.8 s) [D].
   Nothing published on the bachi's contact time, the strike force or the pitch glide was found: §2.3–2.4 estimate
   them from the physics [G].

---

## 1. Steelpan (tenor / lead pan)

### 1.1 The instrument and the note areas

| Quantity | Value | Source |
|---|---|---|
| Tenor range | D4–F6, 28 notes (Mayers' 1990 tenor); low-C tenor C4–E6 or C4–E♭6 | [Kr-A] [D]; [U] retailers |
| Tenor bowl depth, skirt length | 200 mm, 140 mm | [Kr-A] [D], [Mal10 Table 1.1] [D] |
| Whole pan diameter | ≈ 570–600 mm (an oil-drum head; 22.5–23.5 in) | [U] retailers; [Mon12] blank D = 567 mm [D] |
| Sheet thickness | 1 mm (Monteil's pan), ≈ 0.5 mm at a note's centre after sinking; sinking thins a tenor by ≈ 30 % in the middle | [MTT12] [D], [MTT15] [D], [Kr-21] [D] |
| Rossing's model shell | h = 0.77 mm, aspect ratio L/S = 1.5–2.5 | [RHH96] [D] |
| Bending-wave speed in the notes | 68–199 m/s (below the speed of sound: the notes radiate below coincidence) | [RHH96] [D] |

**Tenor note sizes**, Mayers 1990 [Kr-A] [D], in mm (radial × tangential):

| Ring | Notes | Size |
|---|---|---|
| Outer (12) | D4 … C#5 | D4 150 × 155 (180 along the rim); A4 140 × 120; C5 135 × 101 |
| Middle (12) | D5 … C6 | D5 100 × 70; G5 82 × 63; C6 60 × 47 |
| Inner (5) | C#6 … F6 | C#6 59 × 48; F6 55 × 41 |

The layout follows the circle of fifths. The middle ring is an octave above the outer ring, and the inner ring a
further octave up [HR05] [D]. A low-C tenor gets C4 and C#4 by enlarging those two outer notes and shrinking the
others a little [Kr-A] [D].

**Tuning.** The tuner tunes the mode with a nodal line parallel to the rim (or along the diameter, depending on
the ring) to 2f1. A third mode goes to 3f1 (a twelfth) or 4f1 (two octaves) [RHH96] [D], [R01] [D]. The third mode
is "often tuned to three times the fundamental, at least in the lowest notes" [R01] [D]. Examples [HR05] [D]:
G3 on a tenor at 198.9, 391.9, 583.8 Hz (1 : 1.970 : 2.935) [D]. Ping C4 at 262.2, 519.2, 773.4 Hz. Alexis tenor
higher modes at 4.72, 5.09, 6.71, 8.77 × f1 [U, snippet of RHH00]. Mode pairs appear because elliptical notes
lift the degeneracy [HR05] [D].

### 1.2 Measured modes and damping (Monteil et al., double second, laser vibrometer) [MTT15]

| Mode | G3: f (Hz), ratio, ξ | T60 (s) [D derived: 6.91/(ξ ω)] | F3: f (Hz), ratio, ξ | T60 (s) [D derived] |
|---|---|---|---|---|
| 1 (0,0) | 197.5, 1, 0.0049 | 1.14 | 172.2, 1, 0.0045 | 1.42 |
| 2a (note's 2nd + octave note in phase) | 389.7, 1.973, 0.0021 | 1.34 | 341.2, 1.981, 0.0031 | 1.04 |
| 2b (… out of phase) | 397.5, 2.013, 0.0053 | 0.52 | 357.5, 2.076, 0.0058 | 0.53 |
| 4a | 788.8, 3.994, 0.0009 | 1.55 | 686.2, 3.985, 0.0032 | 0.50 |
| 4b | 799.1, 4.046, 0.0012 | 1.15 | 705, 4.094, 0.0038 | 0.41 |
| ≈5f1 | 954.1, 4.831, — | — | 868.8 / 878.8, 5.05 / 5.10, 0.006 / 0.0024 | 0.21 / 0.52 |
| ≈6f1 | 1155, 5.848, 0.0013 | 0.73 | 1115 / 1144, 6.48 / 6.64, 0.01 / 0.0035 | 0.10 / 0.27 |
| ≈8f1 (9f1 on F3) | 1523 / 1530, 7.71 / 7.75, 0.0024 / 0.0017 | 0.30 / 0.42 | 1581, 9.18, 0.003 | 0.23 |

All values [D]. The 2f1 and 4f1 modes are degenerate pairs: each is the note's own mode combined with the
fundamental of the octave note next to it (G4) in phase or out of phase [MTT12] [D]. All the numbers come from a
suspended double-second pan in the lab.

**Tenor notes** [D derived / G]: no tenor-specific damping table was found. Maloney measured a paint-coated
soprano pan (24 notes, impact tests): Q = 50–800, mostly 100–300 [Mal10 §4] [D]. At constant Q this gives
T60 = 6.91·Q/(π f) [D formula]:

| f1 | Q = 100 | Q = 250 |
|---|---|---|
| D4 294 Hz | 0.75 s | 1.9 s |
| C5 523 Hz | 0.42 s | 1.05 s |
| C6 1047 Hz | 0.21 s | 0.53 s |

High tenor notes are therefore short [D derived]. The perceived sustain is longer because sympathetic notes and
the skirt keep ringing [G].

### 1.3 The nonlinearity

- The steel is about 1 mm thick, and quadratic (shell-curvature) coupling starts at a fundamental amplitude of
  **0.04 mm ≈ thickness/25** [MTT12] [D]. Cubic (softening) terms matter at forte: the peak moves from 197.6 to
  192 Hz [MTT12] [D].
- In playing, energy reaches "up to a dozen" modes [TMT14] [D]. Kronman's handbook: the fundamental peaks within
  a few ms, the octave by 15 ms, higher partials after 20–30 ms; a good tone has 5–6 strong partials [Kr-20] [D].
- Measured struck G3 [MTT15] [D]: at mezzo-piano most of the energy stays in H1 and H2, and H2 peaks a little
  after H1. At forte, **H2 reaches 4× H1** and H4 is comparable to H1 (accelerations). The envelopes are
  amplitude-modulated, and the modulation gets faster when the strike is harder.
- **Model equations, 1:2:4** [MTT15 Eq. 4] [D]. Dimensionless: w̄ = w/w0 with w0 = 1 mm, t̄ = 2π f0 t with
  f0 = 197 Hz (G3) or 171.7 Hz (F3), ω̄k = fk/f0:

```
q1'' + 2 ξ1 ω̄1 q1' + ω̄1² q1 + α5 q1 q2           = F1 cos(Ω̄ t̄)
q2'' + 2 ξ2 ω̄2 q2' + ω̄2² q2 + α6 q1² + α7 q2 q3 = 0
q3'' + 2 ξ3 ω̄3 q3' + ω̄3² q3 + α8 q2²           = 0
w(t) = U1 q1 + U2 q2 + U3 q3   (w in mm at the measurement point)
```

| Fit | f1, f2, f3 (Hz) | ξ1, ξ2, ξ3 | U1, U2, U3 | α5, α6, α7, α8 |
|---|---|---|---|---|
| G3, 1:2:4 [Table 4] | 197.1, 389.2, 788.6 | 0.0045, 0.001, 0.001 | 0.30, 0.34, 0.06 | **0.24, 0.34, 0.88, 0.53** |
| F3, 1:2:4 [Table 5] | 171.7, 344.8, 689.7 | 0.003, 0.0006, 0.0013 | 1.002, 1.06, 0.95 | **0.48, 0.57, 0.68, 0.43** |

All values [D]. The 1:2:2 model fitted to G3 [MTT15 Eq. 1, Table 3] [D] has modes 197.0 / 389.2 / 397.2 Hz,
ξ = 0.0051 / 0.0005 / 0.0023, and

```
q1'' + 2ξ1ω̄1 q1' + ω̄1² q1 + α1 q1 q2 + α2 q1 q3 = F
q2'' + 2ξ2ω̄2 q2' + ω̄2² q2 + α3 q1² = 0
q3'' + 2ξ3ω̄3 q3' + ω̄3² q3 + α4 q1² = 0
```

with **α1 = 0.42, α2 = 0.46, α3 = 0.46, α4 = 0.39** [D].
- Impact simulation in [MTT15 §5] [D]: initial displacement w1(0) = 0.15 mm (mp) or 0.65 mm (f), with q2 = q3 = 0.
  The paper reports a "perfect agreement for the first four harmonics".

### 1.4 What the fitted 1:2:4 model does after a hit (my re-simulation)

I integrated G3's model with DOP853 from q1(0) = w10/U1 (script in the scratchpad, not in the repo). The table
gives the displacement amplitude per mode at the measurement point, U·q in mm, max over 10 ms windows [D derived]:

| t | w10 = 0.04 mm (pp) H1 / H2 | w10 = 0.15 mm (mp) H1 / H2 / H4 | w10 = 0.65 mm (f) H1 / H2 / H4 |
|---|---|---|---|
| 0–10 ms | 0.040 / 0.003 | 0.150 / 0.038 / 0.0001 | 0.65 / 0.47 / 0.026 |
| 20 ms | 0.035 / 0.007 | 0.114 / 0.083 / 0.002 | 0.27 / 0.19 / 0.047 |
| 50 ms | 0.029 / 0.010 | 0.077 / 0.075 / 0.005 | 0.30 / 0.46 / 0.026 |
| 100 ms | 0.022 / 0.009 | 0.083 / 0.016 / 0.004 | 0.38 / 0.13 / 0.015 |
| 200 ms | 0.013 / 0.004 | 0.040 / 0.035 / 0.003 | 0.07 / 0.23 / 0.010 |
| 400 ms | 0.004 / 0.003 | 0.014 / 0.018 / 0.001 | 0.07 / 0.05 / 0.007 |

- In acceleration (× ω²), H2 overtakes H1 by 20 ms at mp and from the start at f. H2/H1 reaches ≈ 3–6 at forte,
  and H4 is comparable to H1 [D derived]. That is the octave bloom and the paper's "4×".
- Energy swings back and forth between H1 and H2. The modulation period is ≈ 150 ms at mp and ≈ 50–150 ms at f,
  getting shorter with amplitude [D derived].
- Because ξ2 = 0.001 < ξ1, the octave outlives the fundamental [D].
- Caveat: my absolute accelerations come out several times larger than the axes of their Fig. 23 (≈ 70 m/s² at mp,
  ≈ 700 at f). Their scaling of U or of the initial condition may differ from my reading, so trust the ratios and
  timings, not the levels [U].

### 1.5 Sympathetic notes, the skirt, radiation

- **Sympathetic notes.** Striking B3 excites B4 and B5; B4 radiates "a very strong second harmonic", and damping
  B4 changes B3's timbre noticeably [RHH96] [D]. With a 1:2:3 tuning in the fifths layout, the fundamental of the
  middle-ring note (831 Hz in their double second) equals the 2nd partial of the outer note an octave below and
  the 3rd partial of the note a twelfth below. A skirt mode sits at 830 Hz too [HR05] [D]. As the drive grows, more
  and more of the pan joins in [RHH96] [D], [R01] [D]. A time-reversal study found that for some notes the
  strongest radiation comes from an adjacent note [U].
- **Linear note–note coupling strength.** The octave pair splits by 389.7 → 397.5 Hz, which is 2.0 % (G3), and by
  4.8 % on F3 [D derived]. For two equal oscillators coupled by a spring, the split Δf/f ≈ κ, so the coupling
  is **κ ≈ 0.02–0.05** of the modal stiffness [D derived].
- **Skirt.** On a tenor, the skirt's vibration is **19 ± 9 dB** below the note's (same side) and **23 ± 11 dB**
  below it (opposite side), whether the note is played loud or soft [RHH96] [D]. On lower pans it is weaker: double
  second 27–37 dB, cello 41–46 dB, bass 40–41 dB [D]. Its large area makes it radiate substantially [R01] [D].
- **Front–back cancellation.** A 15 cm tenor skirt gives a ≈ 80 cm path around the pan, so notes below ≈ 277 Hz
  are weakened [Kr-26] [D]. A tenor's lowest note (D4, 294 Hz; C4 on low tenors) is right at that edge
  [D derived].

### 1.6 Excitation: the stick

| Quantity | Value | Source |
|---|---|---|
| Tenor stick | length 150–180 mm, tip Ø 15–17 mm, rubber length 15–20 mm | [Kr-C] [D] |
| Construction | ½-inch aluminium tube or wooden dowel; tip wrapped in surgical rubber | [RHH96] [D] |
| Contact time | **1–2 ms on a tenor** | [RHH96] [D] |
| Kronman's rule | best contact time ≈ one period of the fundamental (3.4 ms at D4, 0.7 ms at F6); small notes want a hard stick, big notes a soft one | [Kr-27] [D] |
| Stick mass | 20–35 g | [G] (½-in aluminium tube, 18 cm, plus rubber) |
| Note area's modal mass | ≈ 15–40 g for an outer note | [G] (ρ h × area × 0.25: 7850 × 0.8e-3 × 0.02 × 0.25 ≈ 30 g) |
| Hertz stiffness of the rubber tip | **k ≈ 5 × 10⁶ N/m^1.5**, exponent 1.5, Hunt–Crossley loss λ ≈ 0.5 s/m | [D derived] (t_c = 3.22 (m/k)^0.4 v^−0.2 = 1.2 ms for m = 30 g, v = 3 m/s); λ is [G] |
| Strike speed | 1–5 m/s, pp → ff | [G] |
| Peak force | 20–150 N | [G] |

The stick's mass is comparable to the note's modal mass, so the contact must be simulated against the moving note
(as `drums.md` plans), not as a fixed force pulse [G]. A contact of 1–2 ms has its first spectral zero near
0.75–1.5 kHz, so it shapes which tuned modes get direct energy. Everything above that comes from the nonlinear
coupling [D derived].

### 1.7 Modal recipe for one tenor note (implementation)

Per note n with fundamental f1 (key → f1). Ratios and damping are from §1.2, scaled to the tenor [D/G as marked].

| # | Mode | Ratio to f1 | ξ (Q) | Strike amp. (velocity, centre of note) | Notes |
|---|---|---|---|---|---|
| 1 | fundamental | 1.000 | 0.0045 (110) [D] | 1.0 [G] | mass-like contact partner |
| 2 | octave, own | 1.975 (detune ±0.5 %) [D] | 0.002 (250) [D] | 0.3 [G] (antinode is off-centre) | quadratic-coupled to 1 |
| 3 | octave, shared with the octave note | 2.01 [D] | 0.005 (95) [D] | 0.1 [G] | linear coupling κ = 0.02 to the octave note's mode 1 [D derived] |
| 4 | 3rd (outer/low notes) **or** 4th | 2.94–3.0 or 3.99 [D] | 0.0015 (330) [G] | 0.2 [G] | quadratic-coupled to 2 (1:2:4) or to 1+2 (1:2:3) |
| 5 | ≈5f1, inharmonic | 4.8–5.1 [D] | 0.004 (125) [D] | 0.15 [G] | linear |
| 6 | ≈6f1 | 5.85–6.6 [D] | 0.002 (250) [D] | 0.1 [G] | linear |
| 7, 8 | ≈8f1 pair | 7.7 / 7.75 [D] | 0.002 (250) [D] | 0.05 [G] | linear |

- **Nonlinear core** [D, from §1.3]: modes 1, 2, 4 follow Monteil's 1:2:4 equations above, with the G3 α's, in
  dimensionless time t̄ = 2π f1 t and displacement in mm. For a 1:2:3 note (mode 4 at 3f1), use the same
  structure, with α(q1 q2) in mode 4's equation and α(q1 q4) in mode 2's [G]. The general quadratic normal-form
  terms resonant for ω4 = ω1 + ω2 are [D derived]:

```
q1'' + … + α5 q1 q2 + β1 q2 q4  = F/m
q2'' + … + α6 q1² + β2 q1 q4   = 0
q4'' + … + β3 q1 q2            = 0      (1:2:3 case; β1..β3 ≈ 0.3–0.6 [G])
```

- **Contact force** goes into mode k scaled by its shape at the strike point (the table's column) [G].
- **Amplitude scale.** Choose the stick velocity so that the initial w1 is 0.04 mm (pp), 0.15 mm (mp) or 0.65 mm
  (f) [D]. Keep the α's the same for every note, since the thickness, which sets the nonlinear scale, is about the
  same everywhere [G]. Higher notes then become less nonlinear by themselves: for the same stick energy,
  w ∝ 1/(ω√M) [D derived].
- **Optional cubic softening** of mode 1: γ q1³ with γ ≈ 0.05 (dimensionless), which gives ≈ −2 % at
  w1 = 0.5 mm [G] (measured −2.8 % at forte drive [MTT12] [D]).
- **Sympathetic notes:** connect each note's mode 3 to the fundamental of its octave neighbour, and a 1:2:3 note's
  mode 4 to its twelfth neighbour, with κ = 0.02 [D derived]. Then ringing notes and damping behave as on a pan.
  Cost: one extra oscillator per note [G].
- **Skirt:** 6–10 linear modes in 250–2000 Hz, with ξ ≈ 0.003, driven by the sum of the note velocities at −20 dB
  [D level, G modes]. Radiation: a first-order high-pass near 250 Hz for front–back cancellation [D derived from
  Kronman's path length].
- **Cost:** about 8 modes + 3 nonlinear terms per sounding note, plus a shared skirt. That is cheap [G].

---

## 2. Taiko (nagado-daiko, GM 117)

### 2.1 Geometry and heads

| Quantity | Value | Source |
|---|---|---|
| Sizes | 1 shaku = 30.3 cm: 1.4 shaku = 42.4 cm, 1.5 = 45.5 cm, 1.6 = 48.5 cm, 2 = 60.6 cm, 3 = 90.9 cm | [D] (Suzuki's model A/B drums are 42.4 and 48.5 cm [SM12]) |
| Typical diameter | 0.3–1 m | [SYI09] [D] |
| Body | hollow barrel, zelkova (keyaki); 48 cm drum: 50 cm long; o-daiko ≈ 1 m long | [SYI09] [SH08] [D], [R01] [D] |
| Cowhide | ρ ≈ 2000 kg/m³, h ≈ 1.5–2 mm (σ = 3–4 kg/m²), E ≈ 3.5 GPa (≈ 30 % of zelkova), ν ≈ 0.3 | [SH08] [SYI09] [D] |
| Tension | 48 cm drum, f(0,1) = 127 Hz single head: T ≈ 20,800 N/m (clamped-plate fit); T a²/D ≈ 450, so it acts as a membrane | [SYI09] [D] |
| Two heads | front tuned slightly lower than rear "at assembly" | [SM12] [D] |
| Climate | −0.3 octave for +50 % humidity (0.006 oct/%, a semitone per 15 %); +0.1 octave from 5 to 35 °C | [SM12] [D] |

### 2.2 Modes

**Single head, 48 cm drum, other head removed** [SYI09 Table 2] [D]. My notation is (diameters, circles);
Suzuki's (circles, diameters) is in brackets.

| Mode | (0,1) [0,0] | (1,1) [0,1] | (2,1) [0,2] | (0,2) [1,0] | (3,1) [0,3] | (1,2) [1,1] | (4,1) [0,4] | (2,2) [1,2] |
|---|---|---|---|---|---|---|---|---|
| f (Hz) | 126.9 | 198.1 | 271.3 | 287.5 | 331.3 | 346.9 | 378.8 | 418.1 |
| Ratio | 1 | 1.567 | 2.138 | 2.266 | 2.611 | 2.734 | 2.985 | 3.295 |
| Ideal membrane | 1 | 1.593 | 2.136 | 2.295 | 2.653 | 2.917 | 3.156 | 3.500 |

All values [D]. The lowest five modes match the ideal membrane within 1.6 %. The higher modes sit low, not high: the
stiffness does not show [SYI09] [D]. A resonance below 100 Hz appears only when the open-bottomed drum is lifted
off the floor (an air/body mode) [SYI09] [D].

**Two heads coupled through the enclosed air** (48 cm × 50 cm, rigid body, ideal membranes) [SH08] [D]:
- Equal heads at T = 14,000 N/m (uncoupled f0 = 113.3 Hz) give an in-phase mode at **110.5 Hz** (×0.975) and an
  out-of-phase mode at **120.0 Hz** (×1.059). In the in-phase mode the heads move the same way, carrying the
  internal air: added mass, a dipole. In the out-of-phase mode both bulge out or in together: the air acts as a
  spring, a monopole.
- With the rear head tighter, the pair widens: at T2/T1 = 1.2 the upper mode is about a whole tone above the
  lower. Above T2/T1 ≈ 1.15 even the lower mode rises above f0, and f2/f1 grows almost linearly beyond T2/T1 ≈ 1.35.
- Real drums of this size have their lowest resonance at **120 ± 20 Hz** [SH08] [D].
- O-daiko (Obata & Tesima, via Rossing): (0,1) pairs at 168/193 Hz (humid) and 202/227 Hz (dry), a ratio of
  1.15 / 1.12. Then (1,1) at 333 Hz and (2,1) at 468 Hz [R01] [D] (ratios 1.98 and 2.79 to the lower (0,1): air
  loading pulls the (0,1) pair down [D derived]).
- Only the (0,n) modes change the volume, so only they are strongly air-coupled. The (1,1) modes of the two heads
  couple weakly through the shell: Nishio saw a ≈ 10 Hz beat in the (1,1) mode of a miya-daiko [Ni99] [D].

**Decays** (Nishio et al. 1999, a miya-daiko with a wood (keyaki) body; its size is given as ≈ 60–90 cm in a
garbled text extraction [U]; sound analysed per mode) [Ni99] [D]:

| Mode | Initial rate | Later rate | T20 of the whole sound |
|---|---|---|---|
| (0,1) | −44.4 dB/s (T60 ≈ 1.35 s) | −6.0 dB/s | 0.47 s (wood body); 0.31 s (aluminium body) |
| (1,1) | −21.7 dB/s (T60 ≈ 2.8 s), ≈ 10 Hz beating | −15.0 dB/s after ≈ 0.3 s (T60 ≈ 4 s) | — |

The (0,1) mode carries the energy, but players prefer the "good sound" of a strike ≈ 10 % off centre, which brings in
the (1,1) mode [Ni99] [D].

**Shell** (zelkova barrel without heads, measured) [HS16] [D]: 137 (0,2), 192 (1,2), 383 (0,3), 452 (1,3), 680
(0,4), 761 (1,4), 800 (1,1) Hz. The drum's size was not stated in the text I read. Barrels of keyaki are preferred
to plastic shells, which have higher internal friction [WP09] [D]. The shell is heavy and stiff, so it mainly
supplies the boundary. Its own modes are weak in the sound [G].

### 2.3 Excitation: the bachi

| Quantity | Value | Source |
|---|---|---|
| Nagado bachi | length 33–42 cm, Ø 2.1–2.5 cm; woods kashi (oak), kaede, tabu, ho (magnolia), hinoki | [U] taiko-shop.com |
| Mass | 145 g (330 × 21 mm beech), 190–200 g (360–390 × 24 mm beech), ≈ 150 g (41 cm ho) | [U] retailer specs |
| Stick speed, maximum effort | ≈ 4.4 m/s | [U] (secondary, original study not found) |
| Contact time, centre "don" | **3–6 ms** | [G], [D derived]: the stick rides the head for half a cycle of the (0,1) mode loaded by its mass, f_c = f0 √(M/(M+m)) ≈ 85 Hz for M = 0.2 kg modal (σπR²·0.27), m = 0.17 kg, so t_c ≈ 6 ms; higher modes shorten it |
| Peak force, ff | ≈ 300–800 N | [G] (π m v (1+e) / (2 t_c) = 3.14·0.17·5·1.5/(2·0.004) ≈ 500 N) |
| Head displacement, ff, centre | ≈ 5 mm | [G] (half of 2.5 J stick energy into the (0,1) mode: v ≈ 3.6 m/s, a = v/ω) |
| Grip | relaxing the grip just before impact shortens the contact and lets the head ring | [U] (KAKEN report, Matsuda) |
| Rim hit (kara/fuchi, "ka") | wood on the wooden rim: contact ≈ 0.3–0.5 ms, excites the shell modes above and head modes weakly | [G] |

Hunt–Crossley for the bachi: wood on hide, stiffness k ≈ 2 × 10⁷ N/m^1.5 (exponent 1.5), λ ≈ 0.3 s/m. The membrane's
compliance then dominates the contact time [G]. Velocity maps to 0.5–5 m/s [G].

### 2.4 Pitch glide (tension modulation) [D derived from the head data; no measurement found]

The (0,1) shape w = a J0(2.405 r/R) has ∫|∇w|² dA = 1.559 π a². With the Berger-type average tension

  T = T0 + (E h / (2 A (1−ν²))) ∫|∇w|² dA = T0 (1 + β a²),  β = 0.857 E h / (R² T0)

For the 48 cm drum (E h = 7 × 10⁶ N/m, T0 = 20,800 N/m, R = 0.24 m), **β ≈ 5000 m⁻²**. Averaged over a cycle
(factor ½), f ≈ f0 √(1 + β a²/2):

| Centre amplitude a | Δf/f0 | Cents |
|---|---|---|
| 2 mm (mf) | +1.0 % | +17 |
| 5 mm (ff, the estimate in §2.3) | +3.1 % | +54 |
| 10 mm (extreme) | +11.8 % | +193 |

The glide decays with a² ∝ e^(−2t/τ), at twice the amplitude decay rate: for the (0,1) mode with τ ≈ 0.2 s
(−44 dB/s), the glide's time constant is ≈ 0.1 s [D derived]. So a hard taiko hit starts **≈ 0.5–1 semitone sharp**
and settles within ≈ 0.2–0.3 s [G overall]. The high tension (T a²/D ≈ 450) keeps the glide smaller than on a
loose bass drum [G]. Related data: the ko-tsuzumi's deliberate glide (the player releasing the cords) is 16 % in
pitch [R01] [D]. That is not tension modulation.

### 2.5 In a room [G unless marked]

- Hard hits are reported to exceed 130 dB [U].
- The lower (0,1) mode is a dipole (the heads move together). It cancels sideways, and near a floor or wall it
  depends strongly on placement. The upper mode is a monopole and fills the room. The single-head drum's sub-100 Hz
  resonance vanishes when the drum sits on the floor [SYI09] [D]. Drums are usually played on stands (beta: on its
  side, low; naname: slanted), which puts a floor reflection 1–3 ms after the direct sound [G].
- 60–130 Hz lies below the Schroeder frequency of practice rooms: room modes colour the boom and lengthen it. In a
  hall the low-frequency T60 is ≈ 2–2.5 s, which a 3-shaku drum excites fully [G]. For the project's reverb: send
  the taiko at a high level to the hall channel, with the low band's T60 1.3–1.5 × the mid band's [G].

### 2.6 Modal recipe (one drum, struck front head)

Size → f0 = 127 Hz × (48.5 cm / D) [D for 48.5 cm; 1/D scaling at equal T/σ is a G]: 1.5 shaku ≈ 135 Hz,
2 shaku ≈ 100 Hz, 3 shaku ≈ 68 Hz. The air coupling grows with the body volume relative to the head; for o-daiko
use the Obata ratios [G].

| # | Mode | Ratio to f0 | T60 (s) | Centre ("don") | 10 % off centre (r = 0.2 R) |
|---|---|---|---|---|---|
| 1 | (0,1) in-phase (dipole) | 0.975 [D] | 2.5 [G] (bounded by the −6…−44 dB/s slopes) | 0.6 [G] | 0.6 |
| 2 | (0,1) out-of-phase (monopole) | 1.06 equal tension, up to 1.12 with a tighter rear head [D] | 1.35 [D] | 0.6 [G] | 0.6 |
| 3, 4 | (1,1) front / rear | 1.567 / 1.567 + 10 Hz [D] | 2.8 [D] | 0 | 0.5 / 0.15 [G] |
| 5 | (2,1) | 2.138 [D] | 1.8 [G] | 0 | 0.2 [G] |
| 6 | (0,2) | 2.266 [D] | 1.2 [G] | 0.35 [G] | 0.3 |
| 7 | (3,1) | 2.611 [D] | 1.5 [G] | 0 | 0.06 [G] |
| 8 | (1,2) | 2.734 [D] | 1.5 [G] | 0 | 0.3 [G] |
| 9 | (4,1) | 2.985 [D] | 1.2 [G] | 0 | 0.02 [G] |
| 10 | (2,2) | 3.295 [D] | 1.0 [G] | 0 | 0.15 [G] |
| 11 | (0,3) | ≈ 3.55 [G] (ideal 3.598 less ≈ 1.5 %) | 0.8 [G] | 0.15 [G] | 0.1 |
| 12–14 | shell | 137 / 192 / 383 Hz × (size scale) [D] | 0.5 [G] | −30 dB [G] | −30 dB |

Amplitudes are relative sound levels. They come from head velocities [D derived: φ(r0)/M_n × the 3 ms half-sine
contact spectrum: centre (0,1) : (0,2) : (0,3) = 1 : 1.44 : 0.73] reduced by the poor net volume velocity of
(0,2) and (0,3) (0.29× and 0.15× that of (0,1)) [D derived]. The final numbers are [G].

**Air coupling of the two (0,1) modes** [D: structure from SH08; coefficients derived]. With x1, x2 the
outward centre displacements of the front and rear head:

```
M ẍ1 + μ (ẍ1 − ẍ2) + R1 ẋ1 + K1 (1 + β x1²/2…) x1 + κ (x1 + x2) = F(t)
M ẍ2 + μ (ẍ2 − ẍ1) + R2 ẋ2 + K2 x2                 + κ (x1 + x2) = 0
```

- M = 0.2695 σ π R² (≈ 0.15–0.2 kg at 48 cm); K_i = M (2π f0,i)² [D].
- κ = ρ c² S² / V, with modal area S = 0.432 π R² (J0 mode, centre-normalised) and V the body volume [D derived].
- Calibrated to Suzuki's equal-tension case: **κ/K = 0.061, μ/M = 0.026** [D derived from 1.059 and 0.975].
  Scale them as κ ∝ R⁴/(V K) and μ ∝ R³ when changing size [G].
- R1, R2: the radiation damping is larger on the monopole combination (x1 + x2) [G]. Implement it as
  R_mono (ẋ1 + ẋ2) on both, sized to give the 1.35 s and 2.5 s T60s above [G].
- Tension modulation (§2.4) acts on K1 through x1² [D derived]. The glide then appears automatically in both
  coupled modes.

---

## 3. Gaps (what was not found)

- Steelpan: no tenor-specific damping table or decay measurement; no measured stick force or contact model
  beyond "1–2 ms"; Achong's oscillator coefficients (only abstracts seen [Ach97] [U]).
- Taiko: no measured bachi contact time, force or pitch glide; no mode/decay table per shaku size; decays come
  from one 1999 study [Ni99]. For all three, fitting to recordings (the `drums.md` method) is the way forward. The
  glide estimate in §2.4 is the first thing to check.

## References

- [MTT15] M. Monteil, O. Thomas, C. Touzé, "Identification of mode couplings in nonlinear vibrations of the
  steelpan", *Applied Acoustics* 89 (2015) 1–15. https://hal.science/hal-01084240 (read via
  https://web.archive.org/web/2023id_/https://hal.science/hal-01084240/document)
- [MTT12] M. Monteil, C. Touzé, O. Thomas, "Complicated dynamics exhibited by thin shells displaying numerous internal
  resonances: application to the steelpan", ICSV19, Vilnius 2012.
  https://perso.ensta-paris.fr/~touze/PDF/ICSV2012Monteil.pdf (archived copy on web.archive.org)
- [TMT14] O. Thomas, M. Monteil, C. Touzé, same title, ENOC 2014.
  https://perso.ensta-paris.fr/~touze/PDF/ENOC2014_ThomasMonteilTouze.pdf
- [Mon12] M. Monteil et al., Acoustics 2012 Nantes (pan forming). https://perso.ensta-paris.fr/~touze/PDF/Acoustics2012_Monteil.pdf
- [HR05] U. J. Hansen, T. D. Rossing, "The Caribbean steelpan, and some offsprings", Forum Acusticum 2005.
  https://www.hangblog.org/pdf/hansen_rossing_the-caribbean-steelpan-and-some-offsprings.pdf
- [RHH96] T. D. Rossing, D. S. Hampton, U. J. Hansen, "Music from oil drums: the acoustics of the steel pan",
  *Physics Today* 49(3) 24 (1996). https://aip.brightspotcdn.com/PTO.v49.i3.24_1.online.pdf
- [RHH00] Rossing, Hansen, Hampton, "Vibrational mode shapes in Caribbean steelpans. I", *JASA* 108, 803 (2000)
  (snippets only). https://www.researchgate.net/publication/12364667
- [R01] T. D. Rossing, "Acoustics of percussion instruments: recent progress", *Acoust. Sci. Tech.* 22, 177 (2001).
  https://www.jstage.jst.go.jp/article/ast/22/3/22_3_177/_pdf
- [Mal10] S. E. Maloney, PhD thesis, Cambridge 2010 (steelpan forming, mode confinement, damping).
  https://www.repository.cam.ac.uk/bitstream/1810/238665/1/Soren%20E%20Maloney%202010%20PhD%20thesis.pdf
- [Kr] U. Kronman, *Steel Pan Tuning* (Musikmuseet, Stockholm 1991), online at https://stockholmsteelband.se/pan/tuning/ :
  [Kr-A] app_a_soprano.php (tenor measures), [Kr-C] app_c.php (sticks), [Kr-20] theory20_tone_generation.php,
  [Kr-21] theory21_material.php, [Kr-26] theory26_skirt.php, [Kr-27] theory27_sticks.php.
- [Ach97] A. Achong, K. A. Sinanan-Singh, "The steelpan as a system of non-linear mode-localized oscillators, Part II",
  *J. Sound Vib.* 203(4) 547–561 (1997) (abstract only). Also A. Achong, "Mode locking on the non-linear notes of
  the steelpan", *JSV* 266, 193 (2003) (not read).
- [CRM05] B. Copeland, T. D. Rossing, A. Morrison, "Sound radiation from Caribbean steelpans", *JASA* 117, 375 (2005)
  (abstract). https://pubs.aip.org/asa/jasa/article-abstract/117/1/375/544414
- [SH08] H. Suzuki, Y.-F. Hwang, "Coupling between two membranes of a Japanese drum", *Acoust. Sci. Tech.* 29, 215 (2008).
  https://www.jstage.jst.go.jp/article/ast/29/3/29_3_215/_pdf
- [SYI09] H. Suzuki, T. Yamaguchi, S. Izumi, "Theoretical and experimental studies on the resonance frequencies of a
  stretched circular plate: application to Japanese drum diaphragms", *Acoust. Sci. Tech.* 30, 348 (2009).
  https://www.jstage.jst.go.jp/article/ast/30/5/30_5_348/_pdf
- [SM12] H. Suzuki, Y. Miyamoto, "Resonance frequency changes of Japanese drum (nagado daiko) diaphragms due to
  temperature, humidity, and aging", *Acoust. Sci. Tech.* 33, 277 (2012). https://www.jstage.jst.go.jp/article/ast/33/4/33_E1209/_pdf
- [HS16] Y.-F. Hwang, H. Suzuki, "A finite-element analysis on the free vibration of Japanese drum wood barrels under
  material property uncertainty", *Acoust. Sci. Tech.* 37, 117 (2016). https://www.jstage.jst.go.jp/article/ast/37/3/37_E1526/_pdf
- [Ni99] 西尾雅明 他, 「和太鼓の振動分析」(Vibration analysis of Japanese drum), 愛知工業大学研究報告 34B, 161–168 (1999).
  http://repository.aitech.ac.jp/dspace/bitstream/11133/1069/1/%E7%B4%80%E8%A6%8134%E5%8F%B7B(P161-168).pdf
- [WP09] "Keyaki wood is the most suitable material for the Wadaiko…" (wood-plastic wadaiko), *Acoust. Sci. Tech.* 30(6)
  410 (2009). https://www.jstage.jst.go.jp/article/ast/30/6/30_6_410/_pdf
- [U] sources: bachi sizes https://taiko-shop.com/blogs/learn/types-material-of-bachi-sticks ; bachi masses
  https://m.thomann.de/be/suzuki_taiko_bachi_sticks_420x24.htm ; stick speed https://grokipedia.com/page/Taiko ;
  grip/contact (Matsuda, KAKEN 26350280) https://kaken.nii.ac.jp/ja/report/KAKENHI-PROJECT-26350280/263502802016jisseki ;
  130 dB https://web-japan.org/kidsweb/cool/12-12-25/index.html ; tenor retail sizes
  https://mittcostg.paradoxstudiostt.com/product/tenor-chrome-plated/ ; steelpan time-reversal radiation
  https://www.researchgate.net/publication/225102568
