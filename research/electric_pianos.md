# Electric pianos (Rhodes, Wurlitzer, Pianet, the FM piano): research and model plan

Status: 2026-11-15, research only; nothing here is built. SI units.

The targets: GM program 5 "Electric Piano 1" (a Fender Rhodes Mark I/II: tine + tonebar, a
neoprene-tipped hammer, an electromagnetic pickup), the Wurlitzer 200/200A (a steel reed struck by
a felt hammer, an electrostatic pickup), short notes on the Hohner Pianet, and GM program 6
"Electric Piano 2" (in practice the DX7-style FM piano). The Clavinet is left to the harpsichord and
clavinet notes.

Where each number comes from is marked: **[web]** from the sources in section 9 (with the key),
**[calc]** computed here (the scripts were one-off Python; the inputs are given), **[memory]**
from the general literature as remembered, not looked up for this document, to be checked before it
is relied on. A [web] number taken from a search summary or a forum, not read in a primary text,
says so.

## 1. Short answer

- **The mechanics are nearly sinusoidal; the pickups make the sound.** High-speed camera
  measurements of both instruments show the tine tip and the reed swinging almost as sines while
  the voltage behind the pickup is far richer **[web, Pf17, PM17]**. Both papers conclude that
  "the primary mechanical exciters are secondary" and the timbre comes "primarily" from the
  pickup. A model that gets the pickup's transfer curve wrong cannot be rescued by the resonator.
  The one published real-time model with a filter + tanh pickup (Sønderbo 2024) sounded harsh
  and wrong for exactly this reason **[web, So24]**.
- **Rhodes pickup = a static nonlinear function of the tine tip's position, then d/dt.**
  The flux through the coil is a bell-shaped function of the tip's offset from the pole axis.
  Voicing (the tip's rest height relative to the pole, "slightly above dead centre" at the factory
  **[web, RSM ch. 4]**) chooses where on the bell the tine swings: centred gives the second
  harmonic only (the fundamental vanishes), off-centre gives a strong fundamental plus harmonics
  that grow with amplitude: the bark. Section 3.4 gives the formula and a table.
- **Tine = a tip-loaded cantilever; it explains the inharmonic "bell".** With the published tine
  diameter (0.075 in = 1.905 mm **[web, RSM via search]**), a bare steel cantilever tuned to E7
  is 22.6 mm long **[calc]**, which is exactly the shortest tine of the 73-key cutting chart
  (22.6 mm) **[web, So24]**. The longest tine (156.4 mm, E1 41.2 Hz) needs a tip mass of 0.19 of
  its own mass (0.65 g: the tuning spring) **[calc]**, and that tip mass moves the overtones to
  6.9 and 20.3 × f0 **[calc]**, against 7.11 and 20.25 measured by laser vibrometry on F1
  **[web, Ga20 via So24]**. So the modes per key follow from the length chart and one tip mass.
  (Sønderbo used a 1 mm tine and found every note flat and the overtones at 6.37 and 35.3: the
  diameter was wrong **[web, So24]**.)
- **The tonebar is a heavy second prong tuned to the same f0**; it stores the energy and gives
  the long sustain: Q ≈ 700–2200, T60 ≈ 27 s at E♭2, 10 s at E♭4, 3 s at E♭6 **[web Q, Sh11;
  calc T60]**. Its own higher modes are the inharmonic partials of the attack, more audible in the
  upper register **[web, PM17]**.
- **Wurlitzer: one or two reed modes, a capacitive pickup, then a transistor amplifier.** The
  reed's higher modes are "comparably small or non-existent even under extreme playing
  conditions" **[web, Pf17]**. The capacitance is largest near the reed's rest position and falls
  at both extremes, unequally (the reed is not symmetric about the plate) **[web, PM17]**: again a
  bell-shaped static nonlinearity, read as a current i = V0 dC/dt into a high-impedance node.
  The bark at high velocity is that nonlinearity plus the overdriven preamp and power amp;
  published sources do not separate the two shares (section 4.4).
- **Plan:** a small dedicated `EPiano` class: per key 4 + 4 tine modes (two polarisations) and 2–3
  tonebar modes coupled at the block (Rhodes), or 2 reed modes (Wurlitzer), the mallet solve from
  `ModalSet::strike` extended with Hunt–Crossley damping, an analytic pickup function evaluated at
  2× rate in the top two octaves, then the electronics (harp network, preamp, tremolo/pan,
  cabinet). Cost about 1 % of a core for 20 sounding notes **[calc]**: ~100 times cheaper than the
  grand.
- **GM 6 (FM piano)** is the DX7 "E.PIANO 1" voice: three 2-operator stacks (section 6). The same
  physical model covers it as a Rhodes voiced for "bell": tine set far off the pole axis (nearly
  linear pickup), hard tips (a loud, fast-dying high partial), two slightly detuned generators
  per key or a chorus, no tremolo. FM's velocity → modulation index is the waveshaper's
  amplitude → harmonics.

## 2. What can be reused from the project

- **`ModalSet` (src/modal.h):** the exact 2×2 step per mode, two polarisations (y and z, a hair
  apart), `strike` (a mass on a Hertz spring F = K c^α, solved implicitly with Newton per
  sample), `damp` (a per-vibrator damping rate), the body-at-the-bridge coupling and
  `cantileverShape`/`cantileverRatio`. Everything the tine needs except:
  (1) tip-mass mode ratios and shapes (the cantilever helpers are for a uniform beam),
  (2) the hammer's velocity-dependent damping (Hunt–Crossley), (3) a per-vibrator body: in
  `ModalSet` all vibrators share one bridge, while each Rhodes tine has its own tonebar,
  (4) a nonlinear readout of the tip position (ModalSet's output is a linear sum of `rad`).
  Recommendation: a new class that copies the step and the mallet solve (as `Piano` copied the
  string bank), one key = one small fixed block of modes, all keys in a row for vectorisation.
- **`Piano` (src/piano.h):** the felt hammer (power law with hysteresis, solved implicitly), the
  dampers' engagement ramp (`engage` 0..1 over tens of ms), key bookkeeping and sleeping of quiet
  keys. The Wurlitzer's felt hammer is this hammer at a smaller scale.
- **`lead.h` shapers:** the asymmetric diode overdrive and the OTA tanh are the right parts for
  the Wurlitzer's transistor preamp and power-amp clipping. The project's room/reverb for the end.
- **Not needed:** quadratic coupling (pan), tension glide: a clamped-free beam has no stretching,
  so the string's tension nonlinearity does not apply; tip amplitude / length is ~1–2 % **[calc]**,
  so the geometric nonlinearity of the tine is negligible next to the pickup's. (Pfeifle includes
  a Kirchhoff-like term **[web, Pf17]**; for a cantilever with a free end it has no physical basis
  in axial stretching, so this note drops it.)

## 3. Rhodes (GM 5)

### 3.1 Construction and ranges

- 73 keys E1–E7 (41.2–2637 Hz) or 88 keys A0–C8 (27.5–4186 Hz) **[web, Sh11]**; also 54 keys
  (Mk II) **[web, WP-R]**. Shear's measured instrument: a 1974 Mark I 88-key Stage.
- Tine: spring-steel rod pressed into an aluminium block, the block screwed to the tonebar
  (brass, sometimes twisted by π/2): together an asymmetric tuning fork with both prongs at the
  same f0 **[web, Pf17, Sh11]**. Original tine 0.075 in (1.905 mm) piano wire, later tapered, then
  swaged **[web, RSM via search]**. (Shear gives 1.5 mm with "slight widening at the base"
  **[web, Sh11]**: the measured section may be the thinned one; 1.905 mm fits the length chart, 3.2.)
- Tine lengths 157 mm down to 18 mm (88 keys) **[web, Sh11]**; 156.4 to 22.6 mm (73 keys,
  service manual cutting chart) **[web, So24]**. Replacement tines 170 mm full length, kits 111 mm
  for the heavily played middle **[web, RSM ch. 5; Vintage Vibe via search]**.
- Tuning spring: a crimped coil spring on the tine near its end, "a counter-weight"; sliding it
  out lowers the pitch, the range "as much as 1-1/2 steps" either way **[web, RSM ch. 5]**.
- The lowest seven tines of the 88 have no tonebars **[web, Sh11]**.
- One pickup per tine: a wound permanent magnet with a shaped (wedge/frustum) iron tip, the tine
  end in front of it; coils in two hum-cancelling sections **[web, Pf17]**; pickups in parallel
  groups of three, the groups in series, alternating polarity **[web, Sh11]**.
- Hammer: plastic/wood hammer, simple single action, the hammer struck from below; felt tips
  until about 1970, then neoprene **[web, RHT; WP-R gives 1971]**.
- Damper: a felt on a leaf spring, from below; clearance when struck 3/8–1/2 in (9.5–12.7 mm)
  **[web, RSM ch. 4]**.

### 3.2 The tine as a cantilever with a tip mass

Uniform clamped-free beam: f_n = β_n² / (2π L²) · √(EI/ρA), β₁ = 1.8751, ratios 1 : 6.27 : 17.55 :
34.39. With a tip mass M = μ·(ρAL), β solves 1 + cos β cosh β + μβ(cos β sinh β − sin β cosh β) = 0
**[calc, standard]**:

| μ (tip / tine mass) | β₁ | f2/f1 | f3/f1 | f4/f1 |
|---|---|---|---|---|
| 0 | 1.875 | 6.27 | 17.55 | 34.4 |
| 0.1 | 1.723 | 6.52 | 18.7 | 37.3 |
| 0.25 | 1.574 | 7.21 | 21.4 | 43.4 |
| 0.5 | 1.420 | 8.38 | 25.6 | 52.6 |
| 1 | 1.248 | 10.4 | 32.7 | 67.6 |

Bare steel tine (E = 200 GPa, ρ = 7850 kg/m³, d = 1.905 mm) tuned by length alone **[calc]**:
A0 221 mm, E1 181, A1 156, A2 111, C4 72, A4 55, C6 36, E7 22.6, C8 17.9 mm.

- The chart's top tine (22.6 mm at E7) is the bare length; the 88-key top (18 mm at C8) is the
  bare 17.9 mm. So in the treble the spring is a trim.
- E1 at 156.4 mm needs μ = 0.19: tine 3.5 g, tip 0.65 g; ratios 1 : 6.93 : 20.4 : 41.1 **[calc]**.
  Gabrielli et al. measured 7.11 and 20.25 on F1 **[web, Ga20 via So24]**: within 3 %. The spring
  sits a little in from the tip, which lowers the needed mass slightly; the modes stay close.
- A0 with a 157 mm tine (the 88's longest) would need μ = 0.72 (2.5 g): ratios 9.3, 28.9 **[calc]**.
  Unverified: whether the 88's bottom tines are longer instead.
- So per key: μ from (chart length, f0), then the ratios from the table. The mode count below
  20 kHz: 4 in the bass, 2 at C6, 1 at E7 **[calc]**.
- Strike point: lower notes struck nearer the middle of the tine, higher notes nearer the base
  **[web, So24 citing RSM]**; the "striking line" is set by sliding the harp: about 57 mm between
  the leading edges of hammer tip and tone generator at the bass end, 3.2 mm at the treble end
  **[web, RSM ch. 4]**. Striking nearer the base excites more and stronger overtones; very short
  tines need a base-near strike and a hard tip "to produce any output" **[web, So24]**.
- Two polarisations: the tine's motion is non-planar; the vertical (hammer) direction is larger,
  the horizontal excited by coupling or hammer imperfection **[web, Pf17 fig. 4]**.

### 3.3 Tonebar, coupling, decay

- Tonebar and tine are tuned to the same f0; matched prongs give the high Q **[web, Sh11]**.
  Measured Q (Q = π f0 τ, τ the 1/e amplitude time) on the 1974 Mark I, and T60 = 6.91 τ **[calc]**:

| Note | f0 (Hz) | Q | τ (s) | T60 (s) |
|---|---|---|---|---|
| E♭2 | 77.8 | 949 | 3.9 | 27 |
| E♭3 | 155.6 | 731 | 1.5 | 10 |
| B3 / C4 | 247 / 262 | 1101 / 1238 | 1.4 / 1.5 | 10 |
| E♭4 | 311.1 | 1520 | 1.6 | 11 |
| E♭5 | 622.3 | 2175 | 1.1 | 7.7 |
| E♭6 | 1244.5 | 1761 | 0.45 | 3.1 |

- These are the fundamental's decay; the inharmonic partials "dissipate quickly" and the tine
  "settles into simple harmonic motion" **[web, Sh11]**. Sønderbo's model, without the tonebar
  and with too little high-frequency loss, sustained its overtones too long **[web, So24]**.
- The tonebar's size: Shear's custom aluminium tonebar for D4 (293.7 Hz) is 82.1 × 11.1 × 3.4 mm
  carrying a 30 g electromagnet at 26.8 mm **[web, Sh11]**. A bare aluminium bar that size would be
  412 Hz **[calc]**; mass 8.4 g in aluminium, about 26 g in brass **[calc]**. So the tonebar is
  ~10 times the tine's mass: a heavy, low-loss reservoir.
- Coupling model **[calc, standard two-oscillator]**: tine (modal mass m_t) and tonebar (M_b ≫ m_t)
  with equal f0 joined through the block's compliance. The two normal modes split by about
  f0·κ (κ the coupling); the hammer puts the energy in the tine, it flows into the bar and back,
  the pair decays at the mass-weighted mean loss, dominated by the bar. A small mistuning of the
  pair gives the two-stage decay and slow beating; a well-tuned pair a single long decay.
  κ and the bar's loss are not published; fit them to Shear's Q per register.
- The tonebar's own overtones: a cantilever-like bar's second mode near 6.3 f0, plus torsion
  (the twisted bars) **[memory]**; Gabrielli et al. compared modes of the assembled fork with those
  of its parts and published the main modes across the keyboard **[web, Ga20 abstract]**. That
  table (not obtained here) is the one to digitise for the tonebar's modes.

### 3.4 The pickup: the bark and the bell

Physics **[web, Sh11, Pf17, FH15]**: the tine, magnetised by the pole, changes the flux Φ through
the coil; the open-circuit voltage is V = −N dΦ/dt. The field at the pole tip is concentrated in a
narrow bell (the frustum's flattened sides focus it **[web, PM17]**). Only a short length of the
tine's end matters, so the tip is treated as a point **[web, Pf17]**.

Published forms:
- Falaize & Hélie (one polarisation, the tip moving along the pole axis, gap l_p):
  Φ_c = a_c (1 + Δμ a_b / (q + l_p)²) φ0, so V = N h0 · k_p q̇ / (q + l_p)³ with
  k_p = 2 Δμ a_b a_c μ0 **[web, FH15 eq. 4, 6]**. Their example: A4 tine, r = 2 mm, l_b = 78.3 mm,
  l_p = 1 mm; a tip motion of 6 % of l_p is linear, 70 % gives the changed waveform **[web, FH15]**.
  The JSV version adds the lateral offset l_δ: gap 0.5 mm "too close", 1.5 mm "ideal", 3 mm
  "low volume"; offset 0 "pure overtone", 0.5 mm "ideal", 1.5 mm "pure tone" **[web, FH17 page]**.
- Pfeifle: a magnetic point-charge field B_z = B0 Δz/|r|³ integrated over a three-part idealised
  pole profile; the tip on a circular arc about the clamp **[web, Pf17 eq. 6–8]**. Gabrielli et al.
  and Pfeifle & Münster's book chapter use FEM; reported adequate against pickup recordings
  **[web, So24]**.
- Observed: as the tip's rest point approaches the pole axis "the fundamental and all odd partials
  are attenuated, leaving the second partial as the dominant" **[web, Sh11, G3 196 Hz]**; centred
  exactly, the output is twice the tine's frequency **[web, PM17]**.

Proposed form for the model (tip at height x above its rest, rest offset δ from the pole axis,
horizontal gap g, both in metres; y the tine's horizontal motion):

    Φ(x, y) = Φ0 · [1 + ((x + δ) / w(y))²]^(−p),   w(y) = w0 + y,   V = −N (∂Φ/∂x · ẋ + ∂Φ/∂y · ẏ)

w0 ≈ the gap g (bell width ~ the distance from the pole tip), p ≈ 1–1.5 (p = 1.5 is the axial
field of a point pole). The derivatives are analytic and multiply the modal velocities, so no
numerical differentiation. Harmonic levels of V for a pure sine tip motion, dB re the strongest
(p = 1; p = 1.5 within a few dB) **[calc]**:

| δ/w | A/w | H1 | H2 | H3 | H4 | H5 |
|---|---|---|---|---|---|---|
| 0 | any | −∞ | 0 | −∞ | −27 (A/w 0.3) … −9 (1.0) | −∞ |
| 0.25 | 0.1 | 0 | −16 | −38 | −70 | −85 |
| 0.25 | 0.3 | 0 | −6 | −19 | −40 | −47 |
| 0.25 | 1.0 | −8 | 0 | −8 | −12 | −16 |
| 0.5 | 0.1 | 0 | −34 | −43 | −70 | −101 |
| 0.5 | 0.3 | 0 | −22 | −24 | −43 | −61 |
| 0.5 | 0.6 | 0 | −11 | −11 | −28 | −34 |
| 0.5 | 1.0 | 0 | −2 | −2 | −29 | −14 |
| 1.0 | 0.3 | 0 | −17 | −54 | −49 | −64 |
| 1.0 | 1.0 | 0 | −13 | −13 | −15 | −26 |

- Soft notes are nearly sine (bell-like with the inharmonic partials on top); hard notes spread
  over H2–H5 within 10–15 dB: the bark. The harmonics rise much faster than the amplitude, as the
  velocity series of Pfeifle's model and the instrument show **[web, Pf17 fig. 10]**.
- The settings in metres: gap 1/16–1/8 in (1.6–3.2 mm), after March 1972 down to 0.020 in
  (0.5 mm) in the middle and treble; a smaller gap is louder and more dynamic **[web, RSM ch. 4]**;
  another Rhodes page gives 1/32 in (0.8 mm) for the middle, 1/16 in for bass and wood-tip treble,
  and warns the tine hits the pickup if set too close **[web, RTS via search]**. Tip amplitudes
  of a few mm at forte in the bass **[memory]**: A/w reaches 1 and beyond, so the bass barks first.
- The horizontal polarisation modulates the width w: an amplitude modulation at the beat of the
  two polarisations, a slow "swirl" in long notes **[calc, from the form; to be checked]**.
- The tine hitting the pole (too close, ff) is a contact: `ModalSet`'s obstacle (one patch at the
  tip, a stiff one-sided spring) does it.

### 3.5 Hammer tips

- Neoprene durometers by key (73-key numbering): 0–23 Shore 30 (black), 24–33 Shore 50 (red),
  34–43 Shore 70 (yellow), 44–57 Shore 90, 58–81 hard maple core wrapped in three layers of shrink
  tubing **[web, RHT]**. The service manual's (88-key) tip heights: tips 1–30 (30) 6.35 mm, 31–40 (50)
  7.94 mm, 41–50 (70) 9.5 mm, 51–64 (90) 11.1 mm, 65–88 wrapped 11.1 mm **[web, RSM ch. 4]**.
  Tips were originally 3/8 in (9.5 mm) square, later triangular **[web, RHT]**.
- Young's modulus from Shore A (Gent's relation) **[calc]**: 30 → 1.1 MPa, 50 → 2.5, 70 → 5.5,
  90 → 21 MPa. Contact stiffness of a rubber tip scales about with E: a factor ~20 from bass to
  upper treble, before the wood core.
- Published model parameters: Hunt–Crossley F = k c^α (1 + μ_h ċ): Sønderbo uses M_h = 11 g,
  α = 2.8, k = 1.5·10¹¹ N/m^α, λ = 9·10¹⁰ (μ_h = λ/k = 0.6 s/m), speed up to 4 m/s **[web, So24]**;
  Falaize & Hélie m_h = 5 g, β = 2 **[web, FH15]**. On a rigid wall Sønderbo's tip gives contact
  0.79 ms, 0.6 mm compression, 140 N at 2 m/s (1.5 ms at 0.5 m/s, 0.57 ms at 4 m/s) **[calc]**.
- But the tine is soft: the bass tine's first mode, seen at a strike point ~0.3 L, is a spring of
  ~7 kN/m **[calc: tip modal mass ~1.5 g at 41 Hz → 100 N/m at the tip, ÷ shape² 0.015]**;
  an 11 g hammer on it is a half-period of ~4 ms. So in the bass the contact time is set by the
  tine, not the rubber, and the tip hardness mainly sets the overtone excitation; in the treble
  the tip dominates. Hence graded tips (and the wood core where no rubber is hard enough).

### 3.6 Damper, sustain, action noise

- Felt from below, released by the key; a sustain pedal lifts all **[web, RSM; So24]**. Damping
  of a released note in ~0.1–0.3 s **[memory]**; Sønderbo's damper as a spring–dashpot to rest:
  K1 = 100 N/m, K3 = 10⁵ N/m³, R = 0.5 kg/s at one point **[web, So24]**: use `damp` with an
  engage ramp as the piano's dampers.
- The mechanical thump of the key and hammer is acoustic, not in the pickup signal; the direct
  out has none (leave it out, or a very low acoustic layer).

### 3.7 Electronics: Stage and Suitcase

- Passive harp: 73/88 coils in the series–parallel network (3.1) into a cable and an amplifier
  input. Each coil is an inductance with resistance; with the cable capacitance it is a resonant
  low-pass in the low kHz **[memory: Rhodes pickups ~180–190 Ω each, a few hundred mH; not
  checked]**. Falaize & Hélie model the pickup load as RLC: 500 Hz cutoff, C = 330 nF,
  L = 307 mH, r = 1 kΩ (illustrative) **[web, FH15]**.
- Suitcase amps: Jordan 50 W (1966–69, mono tremolo), Peterson 80 W (stereo "vibrato":
  intensity/speed), Haigler 100 W (on/off, speed, intensity; slider EQ) **[web, RAMP]**; the 1969
  stereo change turned the tremolo into a panning effect **[web, WP-R, Rhodes effects page]**. It
  is amplitude panning between two sides, not pitch vibrato **[web, WP-R]**; a triangle-like LFO
  **[web, forum, unverified]**. Rate range roughly 1–8 Hz, an LDR/photocell per side, speakers
  4 × 12 in **[memory]**.
- Model: L = s·(1 + d·m(t))/2, R = s·(1 − d·m(t))/2, m a rounded triangle, each side's gain through
  an LDR-like asymmetric slew (fast on, slow off) **[memory: LDRs ~10 ms / ~50–100 ms]**; then the
  preamp's bass/treble shelves and a cabinet filter. For the Stage: a guitar-amp-style input with
  soft clipping (players' practice: Fender Twin **[memory]**).

## 4. Wurlitzer 200 / 200A

### 4.1 Construction

- 64 keys A1–C7 (55–2093 Hz): an octave in from each end of the 88 **[web, WP-W]**.
- Reeds: hardened spring steel, clamped at one end by a screw through an elongated hole, a blob of
  solder (lead) at the free end filed to tune: more solder flatter **[web, WP-W, Pf17]**; sliding the
  reed forward lowers the pitch 2–5 cents **[web, vendor PDF via search]**.
- Reed blanks, 200A series (thickness × width) **[web, VV-R]**: reeds 1–14 0.020 × 0.155 in
  (0.51 × 3.94 mm), 15–20 0.020 × 0.129 in (0.51 × 3.28), 21–42 0.032–0.034 × 0.120 in
  (0.81–0.86 × 3.05, ground), 43–50 × 0.110 in (2.79), 51–64 × 0.097 in (2.46). The 200's middle and
  treble 0.031 in. The text also says "0.020 in on the tongue of a 200 reed to 0.026 in on the
  200A" (the ground tongue, not the blank) **[web, VV-R]**: the 200A's thicker reeds are
  "smoother, rounder and mellower".
- Reed lengths are not published online. Bare steel lengths for f0 **[calc]**: A1 at 0.51 mm thick
  87 mm, A2 62 mm, A3 at 0.81 mm 55 mm, A4 at 0.84 mm 40 mm, C7 18 mm. Real bass reeds are
  shorter [memory: ~50–65 mm], so the solder is a large tip mass: a 60 mm A1 reed (0.95 g) needs
  0.8 g of solder (μ = 0.83), a 50 mm one 1.6 g (μ = 2) **[calc]**. The second mode then sits at
  10–14 × f0 and the third at 30–43 × **[calc]**, far above, consistent with the measured absence
  of higher modes **[web, Pf17]**.
- Action: a miniature London-style (grand-like) action, maple hammers with felt tips **[web, Pf17]**.
  Hammer masses not found.
- Pickup: one plate at high voltage with a slot cut-out per reed; the grounded reed tip swings in
  its slot, the air gap the dielectric **[web, Pf17]**. 170 V DC in the manual, 130 V measured on
  Pfeifle's instrument **[web, Pf17]**; "around 120 V" **[web, KN]**; a 200A forum report 170 → 130 V
  **[web, GDIY]**. The whole reed bar measured 240 pF **[web, GDIY forum]**: ~4 pF per reed including
  stray **[calc]**.
- Amplifiers: the 200 (1968) 30 W, two speakers facing the player **[web, WP-W]**; 200A (1974): a
  better shield over reeds and pickup, AC wiring in tubing, the preamp on its own board mounted
  on the reed bar **[web, WP-W, CEP]**. Single-speed tremolo ("vibrato") with adjustable depth
  **[web, WP-W]**, an LED/LDR in the 200A **[web, VV-LDR]**; rate about 5.5–6 Hz, 200A amp ~20 W,
  two oval speakers **[memory]**.

### 4.2 Reed and hammer model

- Reed: 2 modes (f0 and the tip-mass second mode at ~10–14 f0, its excitation small and fast
  decaying), one polarisation (Pfeifle omits the second and saw no torsion **[web, Pf17, PM17]**).
  Pfeifle used a non-uniform shear beam to place the fundamental; for a modal model the measured
  f0 is simply the tuning, the ratio follows from μ.
- Decay: not published as numbers; the output shows "non-exponential decay and beating of higher
  partials" **[web, PM17]**, which in this plan comes from the pickup (a decaying amplitude
  sweeps through the nonlinear curve) and the second mode. Sustain of a few seconds in the middle
  **[memory]**: fit.
- Hammer: the piano's felt hammer (power law with hysteresis), mass a few grams **[memory]**,
  stiffness fitted so the contact is a fraction of the period.

### 4.3 The electrostatic pickup

Capacitance: computed by FEM over static reed positions; maximal near the rest position, minimal
at the extremes of the swing, unequal between the two directions "because of the non-symmetric
design of the reed" **[web, PM17]**. With the bias V0 nearly constant the reed's current is
i = V0 dC/dt **[web, Pf17 eq. 10–11]**.

Proposed form (x the reed tip's displacement, x0 the rest offset from the slot's centre plane,
w the slot's effective half-width, a an asymmetry):

    C(x) = Cs + Cr · [1 + ((x − x0)/w)² + a·(x − x0)/w]^(−1)     (bell, peak near rest)

The same family as the Rhodes flux (3.4), so the same harmonic table applies with x0/w for δ/w: a
reed resting near the centre plane gives a strong 2nd harmonic, an offset one a fundamental with
harmonics growing with amplitude. w, x0, a are to be computed once from a 2-D electrostatic
solution of the slot (as `tools/pianoboard.py` computes the board) or fitted to the published
reed-motion-and-voltage traces **[web, Pf17 fig. 5]**.

The node: all reeds' currents flow into one bar node with total capacitance C_b ≈ 240 pF and the
input resistance R (1 MΩ and 470 kΩ in the 200A input **[web, GDIY]**):

    C_b dv/dt = Σ_k V0 dC_k/dt − v/R

One sum, one first-order filter for the whole instrument. Corner f_c = 1/(2πRC_b) = 660 Hz (1 MΩ) to
1.4 kHz (470 kΩ) **[calc]**: below it the output is the current (∝ dC/dt, rising 6 dB/octave),
above it v ≈ V0 ΔC/C_b (charge-like). A tilt of 21–28 dB is thus built in from A1 to the corner
**[calc]**, which a forum poster describes as a useful bass cut since bass reeds swing farthest
**[web, GDIY]**. The bias's sag under the summed charge is a second-order effect.

### 4.4 The bark: what is known

- The tone is "closer to a sawtooth" than the Rhodes; soft playing "sweet, vibraphone-like",
  hard playing "a slightly overdriven tone described as a bark" **[web, WP-W]**.
- Two mechanisms, both in the plan: (1) the pickup curve (4.3: amplitude-dependent harmonics,
  even and odd, with the asymmetry a); (2) the transistor preamp and power amp driven hard: the
  200's amplifier was "prone to" noise and distortion, the 200A cleaner **[web, CEP]**; a GroupDIY
  poster: 10–15 dB of gain reaches line level, the bar's output is "much hotter" than a microphone
  **[web, GDIY]**. Symmetric clipping in a push-pull output stage adds odd harmonics; BJT
  exponential input stages add even ones **[memory, general]**. The published sources give no
  measurement separating (1) from (2); "odd-harmonic growl" is therefore not established and should
  be tested against direct reed-bar recordings vs. the speaker/line output.
- Debris or a reed touching the plate gives bursts of distortion (a short) **[web, WP-W]**: not modelled.

### 4.5 Electronics model

Reed-bar node (4.3) → preamp: gain ~4–6 (12–15 dB), a BJT soft clip (`lead.h` diode/OTA shaper,
asymmetric) → tremolo: the LDR shunts the preamp signal, g(t) = 1 − d·L(t), L the LED drive
(sine-ish ~5.5–6 Hz) through the LDR's asymmetric slew **[memory]** → volume → power amp: class-AB
with mild crossover and hard clip near the rails → two small speakers (a 2-pole band-pass ~100 Hz
to ~6 kHz with a breakup peak) **[memory]**. Line out for the "direct" Wurli: skip the speaker.

## 5. Hohner Pianet (and the Clavinet difference)

- 61 keys F1–F6 (43.6–1397 Hz), later 60 **[web, WP-P]**. A tuned spring-steel reed per key; a pad
  on the key's rear end rests on the reed, lifts it when the key goes down and lets it go: the
  reed is plucked by adhesion, not struck **[web, WP-P]**. Early pads leather-faced over urethane
  foam (they decay with age), from 1977 moulded silicone; the Pianet T's described as "suction
  pads" **[web, WP-P]**.
- Pickups: early models (Pianet, C/CH, L/LB, N, N II, Combo) variable-capacitance like the
  Wurlitzer; the T, M and Duo electromagnetic, passive; T reeds damp at once on release
  **[web, WP-P]**. Sound: "a complex mix of harmonics" at the strike settling into a cleaner tone;
  later models mellower **[web, WP-P]**.
- Model: the reed as the Wurlitzer's (2 modes) with a different excitation: the pad pulls the tip
  up with the key and releases when the reed's elastic force k·x exceeds the adhesion force, so
  the initial displacement is nearly fixed by the pad, not by key speed: little dynamic range
  **[memory]**. `ModalSet::pluck` (rise, hold, slip) is this excitation, with the slip time from
  the peel. Pickup: the Wurlitzer's capacitive curve (early) or the Rhodes-type magnetic one (T).
- The Clavinet (another note covers it) is a string, struck against an anvil by a rubber tip that
  stays on it while the key is held, with magnetic string pickups: a string with a stopping
  contact, not a free reed or tine.

## 6. GM 6 "Electric Piano 2": the FM piano

- In GM/GS sound sets program 6 is usually an FM-style electric piano after the DX7's "E.PIANO 1"
  **[memory]**. The DX7 factory ROM1A voice 11 "E.PIANO 1", read from the bank **[web, DX7 file;
  parsed here]**: algorithm 5 (three carrier ← modulator stacks: 1←2, 3←4, 5←6, feedback on 6),
  feedback 6:

| Op | Role | Ratio | Detune | Out | Vel. sens | EG rates / levels |
|---|---|---|---|---|---|---|
| 2 | mod of 1 | 14.00 | 0 | 58 | 7 | 95 50 35 78 / 99 75 0 0 |
| 1 | carrier | 1.00 | +3 | 99 | 2 | 96 25 25 67 / 99 75 0 0 |
| 4 | mod of 3 | 1.00 | 0 | 89 | 6 | 95 29 20 50 / 99 95 0 0 |
| 3 | carrier | 1.00 | 0 | 99 | 2 | 95 20 20 50 / 99 95 0 0 |
| 6 | mod of 5 (fb) | 1.00 | +7 | 79 | 6 | 95 29 20 50 / 99 95 0 0 |
| 5 | carrier | 1.00 | −7 | 99 | 0 | 95 20 20 50 / 99 95 0 0 |

- Reading: stack 1←2 is the "tine clank": a ratio-14 modulator with full velocity sensitivity and
  a fast decay gives a burst of sidebands around 13, 14, 15 f0 at the attack; stacks 3←4 and 5←6
  are 1:1 (harmonic series whose richness follows velocity: the "bark"), 5/6 detuned against each
  other (±7 of the DX7's detune steps, a few cents **[memory]**) for the chorus-like shimmer.
- Physical coverage: the tine model already has (a) an inharmonic partial excited by a hard strike
  near the base and dying fast (the tine's 2nd/3rd modes and the tonebar's: 6–20 f0, with μ and the
  strike point as the knobs; FM's 14 is an exaggerated clank), (b) a waveshaper on a near-sine whose
  harmonic count grows with velocity: what a 1:1 FM pair with velocity → index produces. So
  "Electric Piano 2" = the Rhodes model with: the tip far off the pole axis (δ/w ≈ 1, nearly
  clean), harder tips and a strike nearer the base (louder high partial), shorter tonebar
  sustain, two generators per key 2–4 cents apart (or a stereo chorus), no tremolo. It will sound
  like a "very clean, glassy Rhodes"; the exact DX timbre (its sidebands are harmonic at 13/15 f0
  where the tine's are not) is not a physical goal.

## 7. Model plan and cost

Per key (one block, all keys in a row; a key sleeps below −100 dB as in `Piano`):

| | Rhodes | Wurlitzer | Pianet |
|---|---|---|---|
| Vibrator modes | tine 4 (bass) .. 1 (top), × 2 polarisations | reed 2, one direction | reed 2 |
| Second resonator | tonebar 2–3 modes, coupled at the block | none | none |
| Excitation | mallet: Hunt–Crossley, α 2.5–2.8, k by Shore grade, strike point by key | felt hammer (piano's) | adhesive pluck |
| Pickup | Φ(x, y) bell, analytic derivative, per key | C(x) bell, current into one shared node | either |
| Electronics | harp RLC, preamp EQ, stereo pan LFO, cabinet | node RC, BJT preamp clip, LDR tremolo, class-AB, speakers | (N: valve amp) |
| Damper | felt ramp (`engage`) | felt ramp | release |

- **Oversampling:** the pickup's harmonics reach H5–H8 at ff (3.4); above ~3 kHz f0·8 passes the
  Nyquist of 48 kHz. Evaluate the pickup at 2× (or with a polynomial antiderivative / ADAA) for
  keys above ~C6; below, direct. The modes themselves are exact at any rate.
- **Cost [calc, from the piano's measured figures]:** 20 sounding notes × (8 tine + 3 bar) = 220
  oscillators, + 20 pickup evaluations (a rational function and its derivative) + one electronics
  chain: about 1 % of one core (the grand runs 20 000 modes at 30–50 %). Polyphony is not a cost
  issue; all 88 keys could stay live.
- **Parameters per key** from tables, not free fits: f0, chart length → μ → mode ratios; Shore
  grade → k; strike line → shape at the strike point; Q table → losses; voicing (δ, gap) per key
  as one global "voicing" knob plus key-to-key scatter (real instruments are uneven).
- **Controls worth exposing:** voicing (δ/w: bell ↔ bark), pickup gap (volume and dynamics), tip
  hardness, tremolo/pan rate and depth, drive (Wurlitzer amp). MPE-style per-key bend is cheap with
  modes (retune).

### Validation

1. **Tuning and modes:** per-key f0 from μ against the chart; overtone ratios against Gabrielli et al.'s
   keyboard-wide mode table (6.9–7.1 and 20.3 at F1 already match, 3.2).
2. **Decay:** T60 of the fundamental against Shear's Q table (3.3) and recordings of held notes;
   the inharmonic partials' decay in the first 50–200 ms.
3. **Pickup curve:** reproduce Shear's G3 (196 Hz) waveforms and harmonic levels vs. tine-to-axis
   separation (his figs. 2.4–2.5, digitise) and Falaize & Hélie's audio examples (gap 0.5/1.5/3 mm,
   offset 0/0.5/1.5 mm); then fix w0, p per register.
4. **Velocity series:** record (or obtain) a direct-out Rhodes and a Wurlitzer at 5 velocities per
   key on a few keys; compare H1..H6 vs time and the attack partials; Pfeifle's figs. 3, 5, 10
   (tip motion vs voltage, 12 ms Wurlitzer traces) as shape checks.
5. **Electronics separately:** Wurlitzer reed-bar signal vs line out vs speaker, to place the bark
   between pickup and amplifier (4.4); tremolo rate/shape from a recording's envelope.
6. **Listening:** A/B against the project's existing recordings workflow (as for the grand),
   single notes then chords, both clean and through the amp/pan.

## 8. Open questions

- Tonebar modes and the tine–bar coupling strength per register (Gabrielli et al. 2020 has them;
  not obtained).
- Real Rhodes pole-tip geometry → w0 and p of the flux bell; the 2-D field around the wedge.
- Wurlitzer reed lengths, solder masses, hammer masses; the slot geometry for C(x); the 200A
  preamp/power-amp schematic values (the service manual [WSM] has them).
- Rhodes coil inductance and resistance, harp network output impedance.
- Suitcase pan rate range and LFO shape; Wurlitzer tremolo rate (measure from recordings).

## 9. Sources

- [Sh11] G. Shear, *The Electromagnetically Sustained Rhodes Piano*, MS thesis, UC Santa Barbara
  (2011): ranges, tine 18–157 mm, Q table, pickup voicing, tonebar model.
  https://www.mat.ucsb.edu/Masters/GregShearMasters2011_12_5.pdf (read). NIME papers: Shear &
  Wright 2011 https://zenodo.org/records/1178161/files/nime2011_014.pdf and 2012
  https://vhosts.eecs.umich.edu/nime2012/Proceedings/papers/284_Final_Manuscript.pdf (not read).
- [Pf17] F. Pfeifle, "Real-time physical model of a Wurlitzer and Rhodes electric piano", DAFx-17,
  Edinburgh (2017). https://dafx.de/paper-archive/2017/papers/DAFx17_paper_79.pdf (read).
- [PM17] F. Pfeifle, M. Münster, "Tone production of the Wurlitzer and Rhodes e-pianos", DAGA 2017,
  Kiel. https://pub.dega-akustik.de/DAGA_2017/data/articles/000210.pdf (read). The longer book
  chapter, same title, in *Studies in Musical Acoustics and Psychoacoustics*, Springer (2017),
  pp. 75–107, doi:10.1007/978-3-319-47292-8_3 (not obtained).
- [MP14] M. Münster, F. Pfeifle, "Non-linear behaviour in sound production of the Rhodes piano",
  ISMA 2014, Le Mans, 247–252; M. Münster, F. Pfeifle, T. Weinrich, M. Keil, "Nonlinearities and
  self-organization in the sound production of the Rhodes piano", JASA 136(4), 2164 (2014),
  abstract (neither obtained).
- [FH15] A. Falaize, T. Hélie, "Guaranteed-passive simulation of an electro-mechanical piano: a
  port-Hamiltonian approach", DAFx-15, Trondheim (2015).
  https://www.ntnu.edu/documents/1001201110/1266017954/DAFx-15_submission_33.pdf/a87ee400-f47c-4938-92e7-4030b1338cb7
  (read: pickup eq. 4–6, parameters).
- [FH17] A. Falaize, T. Hélie, "Passive simulation of the nonlinear port-Hamiltonian modeling of a
  Rhodes piano", J. Sound Vib. 390, 289–309 (2017). https://hal.archives-ouvertes.fr/hal-01470949
  (full text not obtained); project page with the gap/offset examples
  https://afalaize.github.io/posts/rhodes (read).
- [Ga20] L. Gabrielli, M. Cantarini, P. Castellini, S. Squartini, "The Rhodes electric piano:
  analysis and simulation of the inharmonic overtones", JASA 148(5), 3052–3064 (2020),
  doi:10.1121/10.0002002; record https://iris.univpm.it/handle/11566/286030 (abstract via search;
  the F1 ratios 7.11/20.25 as quoted by [So24]).
- [So24] T. Sønderbo, *Real-time physical model of the Rhodes electric piano*, MSc thesis, Aalborg
  University (2024).
  https://projekter.aau.dk/projekter/files/719175589/Master_Thesis_Real_time_physical_model_of_the_Rhodes_Electric_Piano_Tobias_Sonderbo.pdf
  (read: FDTD tine, Hunt–Crossley and damper parameters, chart lengths, its failures).
- [Wen09] T. Wendland, "Klang und Akustik des Fender Rhodes E-Pianos", TU Berlin (2009) (cited by
  [Pf17]; not obtained).
- [HM09] N. G. Horton, T. R. Moore, "Modeling the magnetic pickup of an electric guitar", Am. J. Phys.
  77(2), 144 (2009) (the point-charge pickup model; not read).
- [RSM] Rhodes service manual (1979), online: ch. 4 dimensional settings
  https://fenderrhodes.com/org/manual/ch4.html, ch. 5 tuning
  https://www.fenderrhodes.com/org/manual/ch5.html (read); tine diameter 0.075 in from
  https://fenderrhodes.com/service/manual.html (via search summary).
- [RHT] Rhodes hammer tips https://www.fenderrhodes.com/service/hammer-tips.html (read).
- [RTS] Tine settings https://www.fenderrhodes.com/service/tine-settings.html (via search summary).
- [RAMP] Suitcase amp systems https://fenderrhodes.com/service/amp-diffs.html (read); effects
  history https://www.fenderrhodes.com/history/effects.html (via search summary).
- [WP-R] Wikipedia, "Rhodes piano" https://en.wikipedia.org/wiki/Rhodes_piano (read).
- [WP-W] Wikipedia, "Wurlitzer electronic piano"
  https://en.wikipedia.org/wiki/Wurlitzer_electronic_piano (read).
- [WP-P] Wikipedia, "Pianet" https://en.wikipedia.org/wiki/Pianet (read).
- [VV-R] Vintage Vibe, "Case study: Wurlitzer electric piano reeds"
  https://vintagevibe.com/blogs/news/wurlitzer-electric-piano-reeds-case-study (read).
- [VV-LDR] Vintage Vibe, Wurlitzer 200A LDR https://www.vintagevibe.com/products/wurlitzer-200a-ldr
  (via search).
- [GDIY] GroupDIY forum, "Wurlitzer 200A preamp" https://groupdiy.com/threads/wurlitzer-200a-preamp.44606
  (via search summary: 240 pF, 1 MΩ / 470 kΩ, 170 → 130 V, 10–15 dB gain).
- [KN] Keyboard Notes, "On Wurlitzer input circuits"
  https://keyboardnotes.substack.com/p/on-wurlitzer-input-circuits (free preview read).
- [CEP] Chicago Electric Piano, "Wurlitzer 200 vs 200A"
  https://chicagoelectricpiano.com/blogs/news/wurlitzer-200-vs-200a (read).
- [WSM] Wurlitzer Series 200 and 200A service manual (cited by [Pf17] at
  http://manuals.fdiskc.com/flat/Wurlitzer%20Series%20200%20Service%20Manual.pdf; not obtained).
- [DX7] Yamaha DX7 factory bank ROM1A, https://yamahablackboxes.com/patches/dx7/factory/rom1a.syx
  (downloaded and parsed for voice 11; also listed at
  https://homepages.abdn.ac.uk/d.j.benson/pages/html/dx7.html).

Not obtained: Gabrielli et al.'s full mode table; the Pfeifle & Münster chapter; the JSV paper's
2-D pickup formula; Wurlitzer reed lengths, hammer data and amplifier values; any measured
Suitcase pan rate.
