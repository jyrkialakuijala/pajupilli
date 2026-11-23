# The celesta: research and model plan

Status: 2026-11-16. Built as `-inst celesta` (`celestaDesign()` in `src/bars.cc`, section 12); what follows was the
research before it (2026-11-15). GM program 9 "Celesta". The plan is to model
it as the grand was modelled (`research/piano.md`): a felt hammer on a vibrator, a resonator,
dampers and a pedal, and two microphones, all physical, no samples.

The instruments: the modern **Schiedmayer** celesta (Wendlingen, Germany): 4, 5 and 5½ octaves,
the 5½-octave "Compact" and "Studio" models. Its ancestor is the **Mustel** celesta (Paris,
patented 1886, the instrument Tchaikovsky bought in 1891). One maker and Jenco instruments appear
where they add a number.

Where each number comes from: **[web]** from a source in section 11; **[calc]** computed here
from those numbers (scripts in the session scratchpad, not in the repository); **[memory]**
remembered from the general literature, not looked up, to be checked before it is relied on;
**[est]** a working guess, to be fitted to recordings. SI units throughout. Pitch names are
sounding pitch (C4 = 261.6 Hz). The celesta is written an octave below where it sounds.

## 1. Short answer

- **No acoustics paper measures a celesta.** Searches found no measured modes, decays or
  spectra of celesta plates, and no physical model of one in the literature. One commercial
  physical model exists (a commercial piano model's "Celeste" pack, after a five-octave German instrument); its
  workings are not published. The construction below comes from makers' descriptions (Schiedmayer,
  one maker's guide and patents), restorers and museum records. The vibration numbers are computed
  from that construction **[calc]**.
- **A plate:** flat high-carbon steel, about 2–2.5 mm thick and 25 mm wide, 50 mm (C8) to about
  280 mm (C3) long. It rests on felt at its two mode-1 nodes, 22.4 % of the length from each end,
  held by a screw through a hole at each node **[web, Yamaha]**. The bass plates carry weights at
  both ends to shorten them **[web, Yamaha]**. There is no undercut and no overtone tuning: the
  plate is tuned for its fundamental only.
- **Modes:** a free-free beam, 1 : 2.757 : 5.404 : 8.933 : 13.34 **[calc]**. The hammer strikes the
  centre, where modes 2 and 4 have nodes, so the sound is mode 1 plus a little of 5.40 and 13.3.
  The felt supports sit on mode 1's nodes and on no other mode's: they spare the fundamental and
  damp modes 2–4. That is the "quick decay of the upper partials". The top-octave plates are only
  2–2.5 times as long as they are wide. They act as plates, with a twisting mode at 1.4–2.5 × f1
  that an off-centre blow excites **[calc]**.
- **Resonator:** one wooden box per plate in a Schiedmayer (aged beech), several plates to one box
  in a Mustel **[web, Schiedmayer]**. The bass boxes are Helmholtz "upside-down bottles" with a
  neck and a mouth facing the plate **[web, Yamaha]**: 2.3 L (C3), 0.64 L (C4) and 0.16 L (C5) for a
  5–6 cm² mouth **[calc]**. Higher up they are closed tubes, λ/4 = 82 mm at C6 and 20 mm at C8
  **[calc]**. The box draws energy from the plate's fundamental only. A coupling of κ ≈ 0.003–0.007
  (a fraction of a percent; the box tuned to the plate) shortens a plate that would ring 30–40 s
  by itself to the heard 3–8 s, and radiates 70–80 % of the fundamental's energy **[calc, est]**.
- **Hammer:** felt on a birch (one maker) or pear-wood (Schiedmayer) head. The shank is thin
  vertically and wide sideways, springy like a mallet **[web]**. Mustel and Schiedmayer strike from
  above; one maker and Kolberg strike from below with a grand action **[web]**. No hammer masses are
  published. With piano-treble felt (K ≈ 10⁹, p ≈ 2.5) and 3–8 g heads, contact lasts 1–2.5 ms
  in the middle **[calc]**. That is too long for the top octave: C8 needs under about 0.25 ms (felt
  ten times harder) or its fundamental falls by 20 dB or more **[calc]**.
- **Dampers:** felt resting on the top face of each plate's rear end **[web, Yamaha patent]**, at a
  free end, where every mode moves. One pedal lifts all of them, like a piano's sustain pedal
  **[web]**.
- **Model:** a dedicated `Celesta` class built from the grand's parts: the implicit Hertz hammer,
  the damper's engage ramp, and the two microphones with per-source delays. Each key gets about 12
  plate modes from an offline plate computation (a `tools/celestabar.py`, the pendant of
  `pianoboard.py`) and 1–2 air modes of its box, coupled linearly (as `ModalSet`'s `Lin`, built for
  the vibraphone). 66 keys × 16 oscillators ≈ 1 100: about 3 % of the piano's strings, a few
  percent of a core.

## 2. The instruments

| Instrument | Compass (sounding) | Plates | Case W × H × D, mass | Notes |
|---|---|---|---|---|
| Mustel, patent 1886 (no. 176 530, INPI) | first model c1–c5 = C4–C8 **[web, Schiedmayer]**; VSL instead says the 1886 instrument had C3–C8, its lowest octave was found unsatisfactory, and the second generation started at C4 **[web, VSL]** | 49 for C4–C8 **[web, UBC wiki]** | — | "One resonator for multiple sound plates" **[web, Schiedmayer]**. A Wikipedia talk-page editor reads the patent as two plates per note **[web, unverified]**. Struck from above |
| Mustel "Celesta No. 1", Paris Exposition 1889 | — | — | — | Grand Prix **[web, Schiedmayer]** |
| Mustel, 4 octaves (various, c. 1898–1960) | C4–C8 | 49 | 0.89 × 0.94 × 0.41 m (one in a dealer's listing) **[web, antiquitymusic]**; 887 × 683 mm (Musée de la musique E.985.10.1) **[web, MIMO]** | Plates in two rows, semitones alternating between them (C, C♯ up; D, D♯ down) **[web, Yamaha]**; short keys, heavy touch, pedal in the centre **[web, Schiedmayer]**; a Victor Mustel of 1906 tuned to 442 Hz **[web, Preissler]** |
| Schiedmayer 4 octaves | c1–c5 = C4–C8 | 49 **[calc]** | — | **[web, Schiedmayer brochure 2018]** |
| Schiedmayer 5 octaves | c–c5 = C3–C8 | 61 **[calc]** | 1.23 × 1.08 × 0.59 m, 121 kg (dealer) **[web]** | Rental instruments from 1959 and 1967 tuned to 443 Hz **[web, Preissler]** |
| Schiedmayer 5½ octaves, Compact and Studio | c–f5 = C3–F8 | 66 **[calc]** | Studio 1.37 × 1.01 × 0.82 m, 93 / 121 kg (a dealer's gross/net, contradictory) **[web]** | The Studio is the loudest, "for large concert halls" **[web, bonedo]**; the brochure says its plates sit "side by side in one larger resonator" **[web, Schiedmayer site]** |
| Schiedmayer, built into a pipe organ | c–c5 = C3–C8 | 61 | 1.17 × 1.00 × 0.35 m, 70 kg | Plates on 3 levels, oak frame, mechanical or 24 V magnet action **[web]** |
| one maker CEL-53 / CEL-56P (from 1992) | 53 / 56 keys | — | 1.01 / 1.05 × 1.08 × 0.65 m, 70 / 72 kg **[web]** | Struck from below with grand-piano action parts, boxes above the plates; 56P has a soft pedal **[web]** |
| Jenco (USA) | — | 3 racks | — | Plates hung on cord (like vibraphone bars), metal tube resonators, felt hammers from above **[web, Schiedmayer; PTG]** |

- Standard pitch 442 Hz, 442–446 depending on the orchestra **[web, VSL]**. Cases weigh 50–130 kg
  and measure about 1.10 × 1.00 × 0.60 m **[web, VSL]**.
- One Schiedmayer build takes about three months. The plates (60 of them in the instrument
  described) are tuned over about a week by heating, cooling and measuring **[web, Deutsche
  Handwerks Zeitung]**. Its "steel and brass alloy" for the plates is a journalist's phrase; one maker
  says high-carbon spring steel **[web]**. The same article says the plates are struck "from
  below", against Schiedmayer's own brochure ("from above"); the brochure is trusted here.
- History: Victor Mustel's tuning-fork *typophone* came first (1860, 1865 or 1866 by source).
  Schiedmayer's brochure names Victor Mustel (1815–1890) as the inventor; Britannica, one maker and
  VSL name his son Auguste. Tchaikovsky ordered one in Paris in 1891, for about 1 200 francs
  **[web, Yamaha]**. Mustel stopped making them in about 1975 **[web, Schiedmayer]**. Schiedmayer has
  made them since 1890, full-size piano keys since 1996, and moved the pedal to right of centre
  **[web]**.
- GM: a 5½-octave Schiedmayer is C3–F8, MIDI 48–101. Whether MIDI files write the celesta at
  sounding or written pitch is an open choice for the player (section 10).

## 3. The plates

### 3.1 Material and size

- **Steel:** "high-carbon steel, a very hard metal also used in automobile suspensions"
  **[web, Yamaha]**. Taken here as E = 206 GPa, ρ = 7850 kg/m³, ν = 0.29, bending wave speed
  √(E/ρ) = 5120 m/s **[memory]**.
- **Thickness:** "around 2 mm" low and "around 2.5 mm" high in a one maker; a metallophone's bars are
  7.8–9 mm **[web, Yamaha]**. Thicker plates sound "more metallic" **[web, Yamaha]**. A student wiki
  says the opposite way round (2 mm high, 2.5 mm low) **[web, UBC, unreliable]**.
- **One measured instrument** (Grinnell College, 49 plates): lengths from 8 in to 2 in (203 →
  51 mm), width 1 in (25.4 mm), the lowest five 1.25 in (31.8 mm), thickness 0.13 in (3.3 mm)
  **[web, quoted via search; the page refused the fetch]**. A 4 : 1 range of lengths over four
  octaves is exactly what a uniform plate needs (f ∝ t / L²). But a 3.3 mm plate 203 mm long
  sounds G♯4, not C4; with 2 mm, the same lengths give C4–C8 **[calc]**. Either that thickness is
  misreported, or the bass plates are weighted.
- Width 20–30 mm **[web, UBC]**; one maker's patent uses three plate widths, widest in the bass
  **[web, US 7 541 530]**.

**Length for a uniform free-free plate, f1 = 1.0279 · t · √(E/ρ) / L² [calc]:**

| Note | f1 (Hz) | L at t = 2.0 mm | 2.5 mm | 3.3 mm | Mass (25 mm wide, 2.0 / 2.5 mm) |
|---|---|---|---|---|---|
| C3 | 130.8 | 284 mm | 317 mm | 365 mm | 111 g (unweighted) |
| C4 | 261.6 | 201 | 224 | 258 | 79 g |
| C5 | 523.3 | 142 | 159 | 182 | 56 g |
| C6 | 1046.5 | 100 | 112 | 129 | 39 g |
| C7 | 2093 | 71 | 79 | 91 | 39 g (2.5 mm) |
| C8 | 4186 | 50 | 56 | 64 | 27 g (2.5 mm) |
| F8 | 5588 | 43 | 49 | 56 | 24 g (2.5 mm) |

### 3.2 Mounting

- A hole at each mode-1 node, 22.4 % of the length from each end. The plate rests on a felt pad
  and is screwed down, with felt above and below. "If the node is fixed too tightly in place it
  will stop the sound" **[web, Yamaha]**. The node of a free-free beam: 0.2242 L **[calc]**.
- Jenco hangs its plates on a continuous cord through holes, like a vibraphone **[web, PTG]**.
- What the supports damp, by each mode's amplitude at 0.224 L relative to its largest **[calc]**:

| Mode | f / f1 | At the supports | At the centre (hammer) | At the end (damper) |
|---|---|---|---|---|
| 1 | 1 | 0.001 | 0.61 | 1 |
| 2 | 2.757 | 0.50 | 0 | 1 |
| 3 | 5.404 | 0.66 | 0.71 | 1 |
| 4 | 8.933 | 0.47 | 0 | 1 |
| 5 | 13.34 | 0.03 | 0.71 | 1 |

  The felt takes energy from modes 2, 3 and 4 at a quarter to almost half of the square of their
  amplitude, and from mode 1 at nearly none. A modelled plate therefore decays as fundamental
  plus a quickly dying 5.4 partial. Mode 5 (13.3 f1: 3.5 kHz at C4) is spared too, and could leave
  a faint high ring.

### 3.3 Bass end weights

"Weights are attached to both ends of bass register sound bars … a clever way to shorten the
sound bars without changing their pitches" **[web, Yamaha]**. One maker's patent does the same with
"mass-concentrating" thicker ends **[web, US 7 541 530]**. A beam with point masses at both ends,
tuned to C3 (t = 2 mm, w = 31.75 mm) **[calc, FE beam]**:

| Length | End masses (each) | Plate mass | Mode ratios | Node at x/L |
|---|---|---|---|---|
| 284 mm | 0 | 142 g | 1 : 2.76 : 5.40 : 8.93 | 0.217 |
| 250 mm | 12 g | 125 g | 1 : 2.89 : 5.85 : 9.91 | 0.175 |
| 220 mm | 41 g | 110 g | 1 : 3.25 : 6.95 : 12.1 | 0.117 |
| 200 mm | 141 g | 100 g | 1 : 3.67 : 8.11 : 14.3 | 0.050 |

- Weighting spreads the overtones (the bass becomes more inharmonic, more "bell") and moves the
  nodes, and with them the support holes, towards the ends. With the weights the lowest octave
  needs no more than about 250 mm, the C4 plate's length plus a quarter.
- VSL: "it is more difficult to tune the lower bars acoustically, since bigger bars carry a
  larger share of inharmonic partials" **[web, VSL]**. This agrees with the table.

### 3.4 Modes of the real plate shape

The top plates are not beams: at C8, 56 × 25 mm. A free rectangular Kirchhoff plate solved by
Rayleigh–Ritz (Legendre polynomials, 12 × 12; the supports left out) gives the first nine elastic
modes as f / f1. The label is the symmetry along the length, then across the width (S symmetric,
A antisymmetric) **[calc]**:

| Note | Size (mm) | Modes / f1 |
|---|---|---|
| C4 | 200 × 25 × 2.0 | 1.00 SS, 2.76 AS, 4.87 AA, 5.43 SS, 9.01 AS, 9.85 SA, 13.7 SS |
| C6 | 100 × 25 × 2.0 | 1.00 SS, 2.45 AA, 2.77 AS, 5.08 SA, 5.46 SS, 8.05 AA, 9.06 AS |
| C7 | 79 × 25 × 2.5 | 1.00 SS, 1.94 AA, 2.78 AS, 4.08 SA, 5.47 SS, 6.61 AA, 9.02 AS |
| C8 | 56 × 25 × 2.5 | 1.00 SS, 1.39 AA, 2.78 AS, 3.01 SA, 5.04 SS, 5.11 AA, 5.63 SS |
| F8 | 48 × 25 × 2.5 | 1.00 SS, 1.21 AA, 2.66 SA, 2.77 AS, 3.86 SS, 4.60 AS, 4.65 AA |

- A blow at the exact centre excites only the SS modes. A blow off the centre along the length
  adds AS (2.76). A blow off the centre line across the width adds SA and the twisting AA modes.
  At C8 a twisting mode lies at 1.39 f1, close above the fundamental: a hammer a few millimetres
  off centre gives the top notes a rough, bell-like inharmonic pair.
- Supports and holes will move these by a few percent; the offline tool (section 9) should mesh
  the actual outline with the holes and end weights.
- **No overtone tuning:** marimba and xylophone bars are undercut to tune 1 : 4 or 1 : 3. Celesta
  plates are flat strips tuned by length and thickness for the fundamental. No source mentions
  tuning their overtones. One maker's patent cites a prior art aiming at 1 : 4 : 8 for keyboard tone
  plates **[web]**, but that is not the classic celesta.

## 4. The resonators

### 4.1 What they are

- **Bottles.** "Although resonator boxes may look like simple square boxes, they actually have
  necks and mouths on the bottom … They look quite like a bottle of liquor, just turned upside
  down … the volume of the box and the cross-sectional area of its mouth are adjusted" **[web,
  one maker]**. A Helmholtz resonator, its mouth facing the plate.
- **By register (one maker's patent):** the low range is Helmholtz, one chamber per plate; the middle
  range is closed tubes, one per plate, in blocks of four to six; the high range is one chamber
  shared by several plates. Each chamber is at least twice the plate's width. Tuning is by the
  opening's area and depth, chamfering, or partly covering the hole **[web, US 7 750 221]**.
- **Schiedmayer:** one resonator per plate, of aged German beech, "tuned to the fundamental pitch
  of its plate"; Mustel had one resonator for several plates **[web, Schiedmayer]**. A c. 1988
  Schiedmayer has its boxes on four levels: bass on top, middle at lower right, treble in the
  centre and lower left (seen from the rear) **[web, Yamaha]**.
- **Mustel:** rectangular boxes under the bars, two rows **[web, restorer; Yamaha]**.
- **Effect:** "merely striking a sound bar does very little: the sound will be very quiet and
  won't reverberate at all" **[web, Yamaha]**. Restorers report that boxes "too far off" their
  plate give weak notes **[web, PTG]**, and that the box is tuned with the bar in place, to the
  fundamental **[web, Piano World]**.

### 4.2 Sizes [calc]

Helmholtz, f = (c / 2π) √(A / (V L_eff)), c = 343 m/s, L_eff = neck + 1.5 × mouth radius:

| Note | Mouth A | Neck | L_eff | Volume V |
|---|---|---|---|---|
| C3 | 6 cm² | 25 mm | 46 mm | 2.3 L |
| C4 | 6 cm² | 20 mm | 41 mm | 0.64 L |
| C5 | 5 cm² | 15 mm | 34 mm | 0.16 L |
| C6 | 4 cm² | 10 mm | 27 mm | 0.04 L: a bottle no longer makes sense |

Closed tube, λ/4 less the end correction: 82 mm (C6), 41 mm (C7), 20 mm (C8). The volumes fit
the case: a 2.3 L box is 280 × 60 × 140 mm. The mouths and necks are guesses at what fits under
a 25–32 mm plate; the real values are not published.

### 4.3 Coupling to the plate, and what it does to the decay [calc]

The plate's fundamental and the box's air are two oscillators sharing the air under the plate.
Soares, Debut and Antunes model exactly this for marimba and vibraphone tubes: bar modes
inertially coupled to pipe modes, the coupling set by the plate's mode shape over the mouth and
by the gap d over the mouth's radius a. Measured on a test bar, a tuned pipe makes the bar louder
and shortens its decay in proportion; the coupling vanishes when the mouth lies under a node
**[web, Soares et al. 2022]**. Their aluminium bar's modes have damping ratios 0.01–0.04 %, their
pipe's 0.7–0.9 % **[web]**.

- **Upper bound, plate sealing the mouth:** the box adds a stiffness k_cav = ρc² S² / V on the
  plate, and the normalised coupling is κ = √(k_cav / k_plate). For C4 (modal mass 53 g at the
  centre, S = 150 × 20 mm, V = 1.2 L): k_plate = 1.1·10⁵ N/m, k_cav = 1.0·10³ N/m, κ ≤ 0.10. For
  C6: κ ≤ 0.05.
- **Two tuned oscillators,** damping ratios ζ_b (plate) and ζ_r (box), κ much below ζ_r: the
  plate's mode decays with ζ_eff = ζ_b + κ² / (4 ζ_r). With κ above roughly ζ_r the pair splits
  into two tones κ·f apart, each decaying at the mean rate: a doublet that beats.
- **What the instrument must have.** A plate alone on felt is taken as ζ_b ≈ 10⁻⁴ (T60 = 42 s at
  C4) **[est]**. A box's ζ_r is 0.01–0.03, the ballpark of a Helmholtz resonator with wall loss and
  radiation **[memory]**. For heard decays of 8 / 5 / 3 / 1.5 s at C4 / C5 / C6 / C7 **[est,
  section 7]**, κ = 0.003–0.007, and 70–80 % of the fundamental's energy leaves through the box.
  That is 3–7 % of the sealed bound: a gap of some millimetres, partial cover of the mouth, and
  no audible doublet (κ far below ζ_r).
- **Consequences for the model:** (a) the box radiates the fundamental, the plate itself very
  little (a small dipole far below its coincidence frequency); (b) detuning the box by more than
  ζ_r (1–3 %, 17–50 cents) lets the fundamental ring longer but quieter: the "weak note" of a
  badly tuned box; (c) a closed tube's next mode is at 3 f_r and a Helmholtz box's far higher, so
  no box helps the 2.76 or 5.40 partials. They are heard only from the plate's direct radiation:
  bright at the attack, then gone. This is the resonator's "suppression of inharmonic partials"
  (VSL, Schiedmayer) in physical terms: not suppression, but amplification of the fundamental
  alone.
- Mustel's shared boxes: one cavity under several plates cannot be tuned to each, so each plate
  sees a smaller and detuned κ: quieter, a longer and more uneven ring **[est]**. One maker's guide:
  "Celestas from Schiedmayer … are louder than those from Mustel Company, and feature a larger
  bass range" **[web]**.

## 5. The hammer and the action

- **Direction:** Mustel's patent claims the hammer striking the plate from above, distinct from
  the upright's sideways and the grand's upward blow **[web, Schiedmayer]**. Schiedmayer keeps this.
  A c. 1988 Schiedmayer transmits the key's motion by "trackers and rollers (normally seen in
  pipe-organs)" **[web, Yamaha]**. One maker (1992) and Kolberg use grand-piano actions striking
  from below, the boxes above the plates; Schiedmayer calls these "keyboard glockenspiels"
  **[web]**.
- **Dynamics:** of the older action one maker says that celestas "would not make any noise unless
  the keys were pressed strongly, and it was impossible to vary the intensity"; glissandi hurt the
  fingers **[web, Yamaha]** (a competitor's claim). Schiedmayer since 1996: full-length keys, a
  light touch, "a very nuanced tone" **[web]**. Model consequence: a narrow range of hammer
  speeds for the Mustel (est. 0.8–2 m/s), wider for the modern instrument (0.4–3 m/s) **[est]**.
- **The hammer:** a wooden head (birch at one maker; pear wood and felt in Schiedmayer's
  workshop) with a felt tip **[web]**. The shank is "thin vertically but thick horizontally, and
  supple", like a soft mallet's, unlike a piano's stiff shank **[web, Yamaha]**. Bass hammers are
  larger with thicker felt **[web, VSL]**. The felt hardens with use and the sound flattens; it is
  sanded as piano hammers are **[web, Yamaha]**. It strikes the plate's centre, the antinode
  **[web, Yamaha patent; UBC]**.
- **One regulated action (Jenco):** key dip 11.5 mm, hammer blow 50 mm, no let-off (the hammer is
  thrown and must not block against the plate at fff); lead in the hammer heads; strong return
  springs **[web, PTG]**. A piano: dip 10–11.5 mm, blow about 45 mm, hammers 11 g (bass) to
  3.5 g (treble) **[web]**.
- **Masses: none published.** Estimate: 4–8 g heads, like a piano's treble **[est]**. The
  supple shank adds some flex during contact; leave it out at first.

**Contact, F = K δ^p, p = 2.5, the plate's modal mass at the centre as the target [calc]:**

| Case | 0.5 m/s | 1.5 m/s | 3 m/s |
|---|---|---|---|
| Piano C4 as fitted (7.5 g, K = 1.7·10⁹) | 2.4 ms | 1.5 ms | 1.1 ms |
| 5 g, K = 10⁹, on the C4 plate (40–53 g modal) | 2.4 ms, 4 N | 1.5 ms, 21 N | 1.1 ms, 56 N |
| 8 g, K = 3·10⁸, on C3 | 3.9 ms | 2.5 ms | 1.8 ms |
| 3 g, K = 10⁹, on C8 (12–19 g modal) | 2.0 ms | 1.3 ms | 0.95 ms |
| 3 g, K = 10¹⁰, on C8 | 1.1 ms | 0.66 ms | 0.49 ms |

- The plate's mass at the hammer is 6–10 times the head's: unlike a string, the plate hardly
  yields during contact. The felt alone sets the contact time.
- Spectrum of a half-sine blow of duration τ, relative to its low-frequency level: −2 dB at
  f τ = 0.5, −9.5 dB at 1, −23.5 dB at 2, −36 dB at 4 **[calc]**. So:
  - at C4 with 1.5 ms, f1 τ = 0.4 (full), and the 5.40 partial (1.41 kHz, f τ = 2.1) is about
    24 dB down from the blow alone, before the box adds to the fundamental: the middle register is
    nearly a sine with a short metallic edge;
  - at C8, 0.5 ms puts f1 τ = 2.1, 23 dB down: the top octave needs τ ≲ 0.12–0.25 ms, felt
    K ≈ 10¹¹ or a 1–2 g head **[calc]**. The piano model's top octave was weak for this very reason
    (piano.md §10).
- Contact over a patch: a felt tip of 10–15 mm on a 25 mm-wide plate averages the width-wise
  modes. Weight the SA and AA modes by the patch, as piano.md does for the hammer width.
- Loud blows harden the felt (p > 1), so they are brighter, as on the piano. Over the narrow speed
  range the colour changes little: a celesta's dynamics are mostly level **[est]**.

## 6. Dampers and pedal

- **Where:** "damper felts rest on the upper face of each plate's rear end" **[web, Yamaha patent]**.
  In an old Mustel, rust had glued plates to the damper felts, which tore when the plates came out:
  the felt rests directly on the steel **[web, restorer]**. The damper is lifted by its key through
  wires and flanges **[web, restorer]**. Its regulation (lift) is a standard item **[web, PTG]**.
- **Effect:** at the free end every mode moves fully (table 3.2), so a damper stops all of them
  together. Like the piano's: a loss per mode by its amplitude under the felt, ramped on over the
  felt's settling.
- **Damping time: not measured anywhere found.** Estimate: once the damper is down, 60 dB in
  0.1–0.3 s **[est]**, like the piano's slowed dampers (0.15 s for a middle note, piano.md §11). The
  coupled box keeps a little air ringing, then follows.
- **Pedal:** one, lifting all dampers like a piano's sustain **[web]**. Old instruments had it in the
  centre, Schiedmayer right of centre **[web]**. One maker's CEL-56P adds a soft pedal **[web]**.
  Three-octave table models have no dampers **[web, Wikipedia]**. No source mentions undamped top
  plates, as the piano's top 18 keys are. Model all keys damped **[est]**.
- With the pedal down, the plates hardly ring in sympathy: no bridge joins them. The only paths
  are the frame and a shared box (Mustel; one maker's top range; the Studio's "one larger
  resonator"). Ignore at first; a shared-box option for the Mustel voice is cheap (section 9).

## 7. The sound

- **Character [web]:** "the attack and a very long resonance"; decay "similar to a piano's in the
  same register, though a little shorter", the low notes ringing longer; quiet, of limited
  dynamic range; fewer overtones than a glockenspiel, warmer (VSL). One maker: soft but "able to
  carry over quite a distance"; heard better at the back of a hall than in the front row. The
  Mustel restorer's instrument has screens (grilles) front and back; the sound leaves through
  them and the open back.
- **Spectrum, from the physics [calc]:** the fundamental, boosted and radiated by the box, is
  almost all of the steady sound. At the attack: the 5.40 partial (the strongest overtone a
  centre blow gives, about 24 dB under the fundamental in the middle; louder in the top octave,
  where the felt is relatively harder), the plate modes AA and SA in the top octave, 13.3 f1
  faintly, and the knock of the action. The upper partials die in tenths of a second through the
  supports' felt (§3.2), and are radiated only by the plate itself.
- **Decay, heard (60 dB), a working table [est]:** C3 10 s, C4 8 s, C5 5 s, C6 3 s, C7 1.5 s, C8
  0.6 s. The middle values are "a little shorter than the piano" set against piano.md's measured
  model and recordings. To be measured (section 9). The model sets the box coupling κ per key to
  reach these.
- **Mustel against Schiedmayer [web + est]:**

| | Mustel (to c. 1975) | Schiedmayer (modern) |
|---|---|---|
| Compass | C4–C8 (first and usual model) | C3–C8, C3–F8 |
| Resonators | one for several plates, rectangular | one per plate, aged beech, tuned to it; bottle-shaped where large |
| Loudness | quieter **[web, Yamaha]** | louder, the Studio the loudest |
| Action | short keys, heavier touch, pedal in the centre | full-size keys since 1996, lighter, more nuanced |
| Sound (model terms) | weaker, unevenly tuned κ: quieter, longer and more uneven sustain, more plate (overtones) in the mix **[est]** | stronger fundamental, shorter sustain, more even |

  Possibly two plates per note in the 1886 patent **[web, unverified]**. If so: two plates a few
  cents apart, beating, like a piano unison. The patent itself is the source to read.
- **A first look at a recording:** an onset-and-partials pass over the Sugar Plum Fairy track in
  `~/Music/m3` was inconclusive. The orchestra's pizzicati and bass clarinet mask the soft
  celesta, and onset gating alone did not isolate its notes. A note-aware tool is needed (§9).

## 8. Published work

- **Celesta-specific:** none found in JASA, Acta Acustica, DAFx or ICMC searches. Wolfram has a
  "celesta struck-bar estimate" notebook with placeholder inputs ("reference_only"). a commercial piano model's
  "Celeste" pack has a celesta physically modelled after a five-octave German instrument
  (unnamed; probably a Schiedmayer) **[web]**.
- **Bars and resonators in general:** Soares, Debut & Antunes 2022/2023 (the coupled bar–pipe modal
  model, measured: in this file's §4.3); Rucz et al. 2021, a coupled 3-D finite element model of
  mallet instruments: louder with a tuned resonator, shorter bar decay **[web]**; Bork 1995, the
  tuning of xylophone bars and resonators; Chaigne & Doutaut 1997, the time-domain xylophone with a
  nonlinear mallet **[memory]**; Henrique & Antunes 2003, Petrolito & Legge 1997, Beaton & Scavone
  2021 (undercut and 3-D tuning; not needed for flat plates). Marimba bar decays: about 2 s in the
  bass to 1/50 s at the top; vibraphone 30 s down to 6 s **[web, US patent 4 411 187]**.
- **Hammer felt:** the piano literature in piano.md (Hall & Askenfelt, Stulov, Chaigne & Askenfelt)
  is the only measured source; celesta felt is not characterised.

## 9. Model plan

### 9.1 What to reuse

| From | What | For |
|---|---|---|
| `Piano` (`src/piano.cc`) | Implicit hammer solve (mass, felt K δ^p with hysteresis, the modes' displacement under the hammer one sample on: `hk_`, `hc_`, `hs_`); damper `engage` ramp and loss per mode; activity census and sleeping; two microphones with per-source delays | Hammer, dampers, pedal, output |
| `ModalSet` (`src/modal.h`) | Exact two-pole mode step; `Lin` coupling (x_a'' += −g x_b), made for a vibraphone bar and its tube; `damp()` | Plate modes, box air modes, their coupling. As it stands: 32 vibrators, 64 modes, 2 couplings each: too few vibrators for 66 keys with the pedal down, and the hammer is explicit |
| `tools/pianoboard.py` | The offline-eigenproblem pattern (Python → `*_data.h`), the radiation patches | New `tools/celestabar.py` |

Recommended: a `Celesta` class in the `Piano`'s pattern, with all 66 keys present and asleep
until struck, sharing the piano's hammer and damper code. `ModalSet` would need a pool and voice
stealing, which the pedal and repeated notes make audible. The grand's soundboard is not needed:
nothing couples the plates.

### 9.2 Per key

- **Plate:** 8–12 modes from `celestabar.py`: SS 1, 5.4, 13.3; AS 2.76, 8.9 (an off-centre
  blow); AA and SA in the top two octaves. Each mode carries its shape at the hammer (patch-
  averaged), under the damper, and over the box's mouth, and its loss: steel (η ≈ 10⁻⁴) plus
  the support felt by the shape at 0.224 L, fitted.
- **Box:** one air mode (Helmholtz or λ/4), two for the bass bottles (the neck's next mode),
  ζ_r ≈ 0.01–0.03. Coupled to the plate's modes by g = κ ω² × the shape over the mouth. κ per key
  is set for the target decay (§4.3), never above about 0.01.
- **Hammer:** 3–8 g, K from 3·10⁸ (C3) to about 10¹¹ (F8), p = 2.5, the piano's hysteresis term;
  strike point 0.5 L ± a per-key offset of 1–3 mm along and across, which sets the AS and AA
  share.
- **Damper:** a loss rate on all modes, 60 dB in about 0.15 s, ramped on over 20–40 ms; the pedal
  lifts all.
- **Radiation:** the box mouth as a monopole (its volume velocity, differentiated); the plate as a
  weak dipole for its own modes (velocity × area × ka, small). Two microphones in front of the
  case's back screen. Through the case a fixed colouring: the open back, the cabinet's cavity, the
  screens. Start with one broad low-pass and a cabinet resonance near 150–250 Hz **[est]**; compute
  a case later only if listening asks for it.
- **Action noise:** the felt knock and the key bed are large relative to so quiet an instrument,
  especially in the top octave. Second stage.
- **Mustel voice:** C4–C8 only; shared boxes (one air mode coupled to 3–4 neighbouring plates,
  detuned against each); a narrower speed range; optional two plates per note.

### 9.3 `tools/celestabar.py` (offline)

- Input per key: length, width, thickness, end-weight mass and size, hole positions, felt
  stiffness; the box type and size.
- Plate: a Mindlin or Kirchhoff FE plate (as the soundboard's grid) with the holes, the end
  weights, and the supports as soft felt springs (with loss) at the holes. Output: frequencies,
  modal masses, shapes at the hammer, damper and mouth, support losses. The Ritz code of §3.4 is
  the check.
- Lengths: solve for each key's length (t, w fixed per register; bass weights fixed) to its
  equal-tempered fundamental at 442 Hz.
- Box: Helmholtz or tube frequency from the geometry; κ from the gap and cover. The parallel-disk
  fit of Soares et al. is the reference; scale it once to the target decays.
- Output: `src/celesta_data.h`.

### 9.4 Cost

66 keys × (12 plate + 2 air + 2 spare) = 1 056 oscillators, against the piano's 24 600. At the
piano's measured 8 % of a core for one note and 29 % with everything ringing, the celesta with the
pedal down and all keys awake: about 2–3 % of a core. The top octave's hammer, with 0.12–0.25 ms
contact (6–12 samples at 48 kHz), needs the implicit solve (as the piano's has) or 2×
oversampling during contact.

### 9.5 Validation

1. **Plate frequencies:** `celestabar.py` against the beam ratios (2.757, 5.404) for a long plate,
   against §3.4 for the short ones, and the end-weight table §3.3.
2. **Box coupling:** a two-oscillator check of ζ_eff = ζ_b + κ²/4ζ_r and of the split at large κ;
   then the detuned box gives a longer, quieter fundamental.
3. **Recordings:** the Sugar Plum Fairy track (Mustel-type part, written for a 4-octave
   instrument); celesta passages elsewhere in the user's collection; any solo celesta recording.
   A tool in `pianofit.py`'s manner. Find onsets; for each, look for a peak and a second one at
   5.40 × it (and at 2.76, and the top octave's 1.2–1.5 × twist mode), which no string or wind
   instrument produces. Measure per register: the fundamental's early decay, the 5.40 partial's
   level relative to the fundamental at the attack and its decay rate, the share above 2 kHz.
   This checks §3.2's claim that the supports kill modes 2–4, and fixes κ, the felt and the
   support loss.
4. **Listening:** the Sugar Plum melody, chords with the pedal, a glissando, repeated notes (the
   felt meeting a moving plate).

## 10. Open questions

- **Actual plate dimensions** of a Schiedmayer, with thickness and end weights per register:
  none published. Grinnell's 3.3 mm conflicts with one maker's 2–2.5 mm. A technician's measurement
  of one instrument would settle §3 at once.
- **Hammer masses and felt hardness:** none published. The top octave's contact time decides
  whether C7–F8 can sound at all.
- **Decay times** with and without the box, and the damper's time: no measurements found; §7's
  table is a guess.
- **The Mustel patent** (FR 176 530, 1886): one plate or two per note, and the resonator layout.
  Schiedmayer's website shows copies.
- **The MIDI mapping:** a GM file's celesta notes at sounding pitch (C3–F8, 48–101) or at
  written pitch (an octave lower). Proposal: sounding pitch, and notes outside the compass folded
  in by octaves.
- **The case:** how much the cabinet's cavity, the screens and the open back colour the sound.
  Unknown. Fixed filter first.

## 11. Sources

- Schiedmayer Celesta GmbH, brochure "The Celesta – The Celeste" (2018): Mustel patent no.
  176.530, models and compasses, one resonator per plate, pedal, keys, built-in celesta data.
  https://www.celesta-schiedmayer.de/wp-content/uploads/2016/04/Schiedmayer-Celesta-EN.pdf
- Schiedmayer, "All about the celesta" (beech resonators, Mustel shared resonators, first Mustel
  model c1–c5). https://www.celesta-schiedmayer.de/en/celesta/all-about-the-celesta/
- Yamaha Musical Instrument Guide, "The Structure of the Celesta", pages 1–5 (plates 2–2.5 mm,
  high-carbon steel, holes at 22.4 %, bass weights, hammer shanks, birch, Schiedmayer trackers and
  rollers, resonator bottles, 1992 bottom-strike design); origins and trivia pages.
  https://www.yamaha.com/en/musical_instrument_guide/celesta/mechanism/ (and mechanism002–005.html,
  /celesta/structure/, /celesta/trivia/trivia002.html)
- Yamaha, US 7 541 530 B2, "Tone plate for keyboard-type tone plate percussion instrument …"
  (mass-concentrating ends, support holes at nodes, damper felt on the plate's rear end, hammer
  at the antinode). https://patents.google.com/patent/US7541530B2/en
- Yamaha, US 7 750 221 B2, "Keyboard-type tone plate percussion instrument and resonance tube and
  resonance box …" (Helmholtz low, closed tubes middle, shared chamber high; tuning by opening).
  https://patents.google.com/patent/US7750221B2/en
- Yamaha CEL-53 / CEL-56P specifications.
  https://europe.yamaha.com/en/products/musical_instruments/keyboards/other_keyboard_instruments/cel-53/specs.html ;
  https://nl.yamaha.com/nl/products/musical_instruments/keyboards/other_keyboard_instruments/cel-56p/specs.html
- Vienna Symphonic Library, "Celesta" (compasses, case, pitch 442–446 Hz, resonators, decay
  "similar to a piano's … a little shorter"). https://www.vsl.co.at/academy/percussion/celesta
- Wikipedia, "Celesta", and its talk page (two plates per note in the patent, unverified).
  https://en.wikipedia.org/wiki/Celesta ; https://en.wikipedia.org/wiki/Talk:Celesta
- Grinnell College Musical Instrument Collection, celesta (49 plates, 8–2 in, 1 / 1.25 in wide,
  0.13 in thick; quoted via search, the page refused direct access).
  https://omeka-s.grinnell.edu/s/MusicalInstruments/item/971
- UBC PHYS 341 student wiki, "Celeste" (nodes at 22.4 %, 1 : 2.76 : 5.40, Helmholtz boxes; cites
  Rossing, Springer Handbook of Acoustics 2007, pp. 649–650). https://wiki.ubc.ca/Course:Phys341_2020/Celeste
- LA Piano Tuning, "Restoring a Mustel Celesta" (2023: 30 bars, rectangular boxes, damper felts,
  flanges, screens). https://www.lapianotuning.com/wordpress/?p=298
- PTG forum, "My first Celesta repair" (Jenco: cord suspension, key dip 11.5 mm, blow 50 mm, no
  let-off, lead-weighted hammers, boxes off tune give weak notes).
  https://my.ptg.org/communities/community-home/digestviewer/viewthread?MessageKey=d92f65df-1cfc-46be-ba01-d9deb7f07a94&CommunityKey=6265a40b-9fd2-4152-a628-bd7c7d770cbf&tab=digestviewer
- Preissler Music, celesta rentals (Schiedmayer 1959/1967/1995, Mustel 1906; pitches).
  https://www.preissler-music.com/en/rental/keyboard-instruments/celesta/
- Deutsche Handwerks Zeitung, on Schiedmayer's workshop (60 plates, tuning by heating over a
  week, pear wood and felt, three months per instrument). https://www.deutsche-handwerks-zeitung.de/?p=155582
- bonedo.de, "Die Celesta – himmlisches Glockenklavier neu entdeckt" (Studio model).
  https://bonedo.de/artikel/die-celesta-himmlisches-glockenklavier-neu-entdeckt
- Antiquity Music, Mustel 4-octave celesta (case 35 × 37 × 16 in).
  https://antiquitymusic.com/keyboards-electronic/mustel-celeste-celesta
- MIMO, Musée de la musique, Mustel & Cie celesta 1898, E.985.10.1 (case dimensions; via search).
  https://mimo-international.com/MIMO/doc/IFD/OAI_CIMU_ALOES_0160182/orgue-celesta
- F. Soares, V. Debut, J. Antunes, "The vibroacoustic interaction in mallet percussion
  instruments: modelling and experiments", 53º Congreso Español de Acústica, Elche 2022
  (read in full); JSV 2023 version "The bar-resonator interaction in mallet percussion
  instruments: a multi-modal model and experimental validation".
  https://www.sea-acustica.es/archivo/congresos/Elche22/ID-187.pdf
- P. Rucz, M. Á. Ulveczki, J. Angster, A. Miklós, "Simulation of mallet percussion instruments by
  a coupled modal vibroacoustic finite element method", JASA 149(5), 3200–3212, 2021 (abstract
  only). https://real.mtak.hu/129617
- I. Bork, "Practical tuning of xylophone bars and resonators", Applied Acoustics 46, 103–127,
  1995 (cited via Soares et al.; not read).
- Modartt, Pianoteq "Celeste" pack (celesta after a five-octave German instrument).
  https://www.modartt.com/celeste
- Wolfram Cloud, "celesta interactive" (placeholder estimate model, no data).
  https://www.wolframcloud.com/obj/wrfcoin/Musical_Instruments/idiophones/celesta/celesta-interactive
- US patent 4 411 187, "Composite marimba bars" (marimba and vibraphone decay ranges).
  https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/4411187
- research/piano.md (hammer felt values, contact times, damper timing, cost figures).

Not obtained: the text of Mustel's patent; any measured plate dimensions of a Schiedmayer;
hammer masses; measured decay times or spectra of any celesta.

## 12. As built (2026-11-16)

`celestaDesign()` in `src/bars.cc` (on the struck-bar engine shared with the rest of the chromatic
percussion), `src/barsinst.h/.cc` (the instrument and its player),
`-inst celesta`, GM program 9 in `-play`; 66 keys C3–F8 at sounding pitch (notes outside are folded
in by octaves), A4 = 440 Hz (`PJ_CEL_A4`).

- **Per key:** five bending modes of the free strip and its first two twisting modes; one air mode
  of the box, coupled to the plate by κ set from the wanted ring time (§4.3); a felt hammer (mass
  and stiffness by register, p = 2.5, a little hysteresis) solved implicitly against the plate's
  give; a damper on every mode, ramped; the pedal lifts all. No offline tool: the strip's modes are
  the closed-form ones, the twisting ones from the strip's torsion (they land on §3.4's ratios:
  4.86 at C4, 1.92 at C7, 1.36 at C8).
- **Checks (`tools/test.py` celesta()):** in tune within 3 cents; the damper stops a note; 10 s of
  random playing stable; 0.6 % of a core.
- **Fitted to one recording** (a short phrase supplied by the user; only numbers kept). Measured
  there, on notes at 664 and 996 Hz, in the first 80 ms and relative to the fundamental: a partial
  at **5.33–5.35 ×, −33 to −36 dB** (the third bending mode: the celesta's metallic edge, gone
  within tenths of a second); 2.72–2.73 × at −44 to −53 dB; a **knock** below the note (near 70,
  150 and 370–570 Hz) at about −35 dB; the fundamental falling 5–8 dB/s (a ring of 8–11 s). The
  recording is 14 cents sharp of A 440 (about 443.5 Hz, as §2 says orchestras tune them).
  - The model's 2nd to 4th bending modes are set 1 % under the plain strip's (2.73, 5.35, 8.84).
  - The plates' own radiation of their overtones is 11 × the bare strip's (the frame and case give
    it more to work against): the 5.35 partial comes out at −32 to −33 dB.
  - The ring table (§7) is doubled in the middle: 12, 11, 9, 6, 3, 1 s at C3 .. C8.
  - A case knock was added: four short cabinet resonances (72, 150, 370, 560 Hz) driven by the
    hammers' force, at −33 to −37 dB.
- **Not done:** end weights in the bass (plain strips at their full length); the top octave's
  shared boxes; the Mustel voice; the case's colouring of the sound beyond the knock; the offline
  plate tool with holes. Levels across the compass (the boxes' tilt of 0.5 halvings an octave) are
  an estimate.
