# The 5-string banjo (GM 106) and the koto (GM 108): research notes for modal models

Status: 2026-11-14, research only; nothing is implemented. SI units. A4 = 440 Hz unless noted.

Both are to be built like the plucked double bass (`src/bass.h`, `research/double_bass.md`): each string a
sum of modes in two polarisations, ending on a bridge that is a mass on the body's modes, plus an excitation
(pick, nail) that pulls, holds and lets go.

**How numbers are marked.** A number followed by a reference key (e.g. [WPM21a]) was read in that source's
primary text. **[D]** marks a number I derived from published data or formulas (the derivation is given or
obvious). **[G]** marks my estimate or guess. **[U]** marks a number taken from an abstract, a secondary source,
a vendor or a blog, which I could not check in the primary text. References are at the end.

Short answer:
- **Banjo:** a light (2.2 g) bridge on a thin, tight Mylar membrane (0.18 mm, 5.33 kN/m, 0.30 kg/m²). The
  bridge admittance is 20–30 dB above a guitar's over 300 Hz, so the strings lose most of their energy into the
  head. String modes decay in 100–200 ms (time constant) instead of 300–400 ms on a guitar, and much of that
  energy is radiated. The result is a loud, bright note that dies fast. The colour comes from three broad
  formants: a mass–spring one at 500–800 Hz (the bridge mass against the head's and the strings' stiffness) and
  two "bridge hills" near 3 kHz and 5 kHz. Fine detail of single head modes matters less [WPM21a, WPM21b].
- **Koto:** thirteen identical polyester (or silk) strings at about 147 N, about 1.8 m between the end bridges.
  Each string sounds between the fixed bridge (ryūkaku) and its own movable bridge (ji); its sounding length
  runs from 0.16 m to 1.3 m [CS11]. The body is a heavy, thick paulownia box with its main resonances at
  about 85 Hz (air) and 100 Hz (top) [C19], below most played notes. Little is published on its damping or on
  string decay; most body and decay numbers below are guesses.

---

## Part I: the 5-string banjo

### 1. Strings

#### 1.1 Tuning and geometry

Open G tuning (gDGBD) [SM08 Table I, WPM21a Table 2]. The 5th string is the short drone. It starts at a peg at
the 5th fret, so its sounding length is 5/12 of an octave shorter [D].

| String | Note | MIDI | f0 (Hz) | Length (m) | Diameter (mm) | Tension (kgf) [WPM21a] | Tension (N) [D] | μ (g/m) [D] | Z = √(Tμ) (Ns/m) [WPM21a] | B, inharmonicity [D] |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | D4 | 62 | 293.7 | 0.67 | 0.25 plain | 6.19 | 60.7 | 0.392 | 0.154 | 1.4e-5 |
| 2 | B3 | 59 | 246.9 | 0.67 | 0.28 plain | 5.29 | 51.9 | 0.474 | 0.157 | 2.6e-5 |
| 3 | G3 | 55 | 196.0 | 0.67 | 0.33 plain | 4.66 | 45.7 | 0.663 | 0.174 | 5.6e-5 |
| 4 | D3 | 50 | 146.8 | 0.67 | 0.53 wound | 5.76 | 56.5 | 1.46 | 0.287 | ~1.6e-5 (core 0.25 mm [G]) |
| 5 | G4 | 67 | 392.0 | 0.50 | 0.24 plain | 5.56 | 54.5 | 0.355 | 0.139 | 2.4e-5 |

- Instrument: Deering Eagle II, scale 0.67 m (26.4 in) [WPM21a]. The common scale is 26–26.4 in [G].
- μ = T/(2Lf)². B = π³ E d⁴/(64 T L²), with E = 200 GPa for steel [P21] and the plain diameter (for the
  wound 4th string, the core). The μ values reproduce the published Z within 1 % [D].
- A typical "medium" set is 10-12-14-22w-10 thousandths of an inch (0.25-0.30-0.36-0.56w-0.25 mm) [P19]. The
  light D'Addario EJ60 is 9-10-12-20w-9 [U, retailers], with about 11 lbf (49 N) on the 0.009 in D string [U].
- Total string tension is about 270 N [P21].
- Measured inharmonicity: the standard deviation of the first 15 partials from integer ratios is 0.10 % on a
  banjo, against 0.09 % on a guitar [P14]. B ≈ 2.5e-5 gives a shift of B n²/2 = 0.28 % at n = 15, so the
  two agree [D].
- Fretting at fret k multiplies B by 2^(2k/12): ×4 at the 12th fret [D].
- Afterlength (bridge to tailpiece): about 5 cm [SM08]; 4 cm "as an example" [P14].
- Break angle over the bridge: 5–15° on current instruments [P14, PWM21]; 12–13° on the Deering [P21, WPM21b].

#### 1.2 String decay

- Plectrum-plucked notes on the top string (fundamentals 294–588 Hz) [WPM21a Fig. 4/11]: up to about 1 kHz,
  banjo string modes have decay time constants (ωη)⁻¹ of **100–200 ms**, against **300–400 ms** for the same
  plain steel string on a guitar. That gives T60 = 6.9 τ ≈ **0.7–1.4 s** for the banjo [D].
- Loss to the body follows from the bridge admittance Y_b and the string's characteristic admittance
  Y0 = 1/Z [WPM21a eqs. 1–3]: η_b,n ≈ 2 Re(Y_b)/(π n Y0). On a banjo this term dominates; on a guitar the
  string's own losses dominate. Above about 1 kHz the banjo's string modes follow the body-loss curve closely.
  Where a string overtone sits on a head resonance it can lose so fast that it merges with the body modes: such a
  note "falls flat" [WPM21a].
- The string's intrinsic damping as fitted for synthesis [WPM21a eqs. 4–7]:
  η_n = η_background + η_air + η_bend, with η_background = 3e-5;
  η_air ≈ (ρ_a/ρ)(2√2 M + 1)/M², where M = (d/4)√(2π f_n/ν_a), ρ_a = 1.2 kg/m³ and ν_a = 1.5e-5 m²/s
  (the PDF text extraction is garbled; the exact form is [U], see Fletcher & Rossing);
  η_bend = E π² d² n² η_E / (64 ρ L⁴ f1² + E π² d² n²), with η_E = 1e-3 ("does not matter very much").
  Example, string 1 at 294 Hz: M = 0.69, η_air ≈ 9e-4, so τ_air ≈ 0.6 s [D]. For a plain steel string the air
  is the larger intrinsic loss, but it is still smaller than the loss into the head.
- Two polarisations [SM08]:
  - Horizontal (parallel to the head) plucks decay more slowly than vertical ones.
  - Individual partials decay in one of three ways: a single exponential, two exponentials (the two
    polarisations with different rates), or with periodic beating.
  - A pluck aimed in one plane soon turns elliptical: the two planes are weakly coupled.
  - The vertical pluck of the 1st string (D4, 294 Hz) puts its fundamental next to the head's lowest mode
    (277 Hz there), and that fundamental dies very fast.
  - Spectrograms of the 1st and 3rd overtones show beating, read as a frequency split between the polarisations
    caused by the body [WPM21a].
- **Tension modulation** [P14, P19]: the bridge floats on the head. When it moves by x, the string length
  changes to first order through the break angle, by ≈ x sin β [D]. This modulates the tension, and so the pitch,
  at audio rate: frequency-modulation sidebands, which Politzer proposes as the source of the metallic "ring".
  A bigger break angle gives more of it.
  - With 0° break angle the spectra below 1.5 kHz hardly change, and the differences sit in the first
    0.2–0.3 s of a note [P19].
  - Per string, ∂T/∂x ≈ EA sin β / L_total = 9.8 kN × 0.21 / 0.72 m ≈ 2.8 kN/m for string 1 [D]. This is the
    same mechanism as the string-axial stiffness term in §2.3 (≈ 5 kN/m for all five) [WPM21b].
  - In the model: tension = T0 + k_ax (x_bridge sin β + the stretch from the string's own amplitude), as the
    bass already does for amplitude.

### 2. The head, bridge and pot

#### 2.1 The membrane

| Quantity | Deering Eagle II [WPM21a/b Table 1] | Gold Tone MC-150R [SM08] | Notes |
|---|---|---|---|
| Diameter | 268 mm (nominal 11 in) | 270 mm (radius 135 mm) | |
| Thickness | 0.18 mm | 0.21 mm, frosted Mylar, ρ ≈ 1200 kg/m³ | Mylar: E ≈ 5.3 GPa [P21, "educated guess"] |
| Mass per area σ | 0.30 kg/m² | 0.25 kg/m² [D] | the whole head is about 11 g [PWM21] |
| Tension T | **5.33 kN/m** (inferred from FE fit, checked by static compliance) | n/a; head tap ≈ 275 Hz | Politzer: same value [P21] |
| Wave speed c = √(T/σ) | 133 m/s [D] | | subsonic: a Mylar head is always below the speed of sound in air [WPM21a] |
| Lowest mode, in vacuo | 380 Hz [WPM21b]; 2.405 c/(2π a) = 380 Hz [D] | | |
| Lowest mode, air-loaded | **290 Hz** [WPM21b] | 277 Hz [SM08] | the air's added mass roughly equals the membrane's mass for this mode |

Measured head modes on the Gold Tone, resonator off, head tuned to about 275 Hz, strings damped, driven
acoustically [SM08 Table II]. SM08 labels the modes (nodal diameters, nodal circles), starting from (0,0):

| Mode | (0,0) | (1,0) | (2,0) | (0,1) | (3,0) | (4,0) | (5,0)* | (6,0)* | (7,0)* |
|---|---|---|---|---|---|---|---|---|---|
| f (Hz), ±10 | 277 | 550, 580 (doublet) | 831, 842 | 978 | 1084 | 1320 | 1549 | 1771 | 1991 |
| Ratio to (0,0) [D] | 1 | 1.99/2.09 | 3.00 | 3.53 | 3.91 | 4.77 | 5.59 | 6.39 | 7.19 |
| Ideal membrane ratio (Bessel) | 1 | 1.594 | 2.136 | 2.296 | 2.653 | 3.156 | 3.600 | 4.06 | 4.50 |

- Air loading lowers the low modes most, so the measured series is stretched compared with the ideal membrane.
  The added mass is about 0.15 × (the mass of air within one wavelength) for the lower modes
  [WPM21b Fig. 5].
- Widths [SM08]: "in general less than 10 Hz", so Q ≳ 50–200 [D]. The exception is (0,0), which "could be
  easily excited over a range of almost 100 Hz", so Q ≈ 3 [D].
- Tuned heads have symmetric mode shapes despite the off-centre bridge [SM08]. The ideal circular-membrane modes
  are a good modal basis.
- Tap tuning: "bluegrass tight is G# or A" on an 11 in Mylar head [PWM21]. A tap spectrum in [PWM21] has a
  spacing of about 238 Hz (A#3). Politzer's own Goodtime reads 91 on a DrumDial [P19]. Head tension in practice
  spans roughly ±30 % around the Deering value [G].

#### 2.2 Damping of the head: radiation dominates

- Radiation damping of a membrane is **highest at low frequency and falls with frequency**, the opposite of a
  plate, because a membrane has no coincidence (critical) frequency [WPM21b].
- For many head modes radiation damping exceeds structural damping, so a large share of the energy the strings
  give the head becomes sound [WPM21b]. This is the "loud" part.
- Lowest-mode loss factor, measured on a Nechville with eight head tensions: about 1e-2 to 1e-1 (log axis), rising
  with tension [WPM21a Fig. 9]. A tighter head radiates more efficiently. Read from the plot, so [U].
- Radiation loss factor of the modes, FE/BE, from about 1e-1 near 300 Hz down to about 1e-3 at 2–3 kHz
  [WPM21b Fig. 7; plot read, U].
- Background structural loss factor added to fit the measurement: **η = 4e-3** [WPM21b].
- The simple "square banjo" model in [WPM21b] uses Q = 50 (η = 0.02) for every mode.
- Proposed for pajupilli [G]: η_k = 4e-3 + η_rad(f_k), with η_rad ≈ 0.05·(300 Hz / f_k)^1.5 (gives 0.05 at
  300 Hz and 2e-3 at 2.5 kHz). Q ≈ 15 for the lowest mode, rising to 150 or more above 2 kHz.

#### 2.3 The bridge on the head

| Quantity | Value | Source |
|---|---|---|
| Bridge mass | **2.2 g** (Deering, three-foot maple, ebony top); 2.5 ± 0.1 g (Gold Tone, 80.4 mm long); 2.6 g with screws (Politzer's zero-break-angle bridge) | [WPM21a], [SM08], [P19] |
| Footprint | 78 × 5 mm | [WPM21a] |
| Static down-force | strings ≈ 55 N, head ≈ 71 N; equilibrium head dip ≈ 2.9 mm, head angle at the foot ≈ 4.5° | [P21] |
| Stiffness at the bridge | Politzer: string stretch 5.7 + break angle 2.6 + head 20 + head stretch 2.9 ≈ **31 kN/m**. Woodhouse: strings axially ≈ 5 kN/m, afterlength ≈ 2.4 kN/m; **20 kN/m** fits the measurement best | [P21], [WPM21b] |
| Dashpot | 1 Ns/m (in the fitted model) | [WPM21b] |
| Effective mass in the fit | 1.5 g matched the high-frequency trend better than 2.2 g | [WPM21b] |
| Static compliance of the head | C0 ≈ 1e-4 m/N (from the admittance); ideal membrane C = −ln(r/a)/(2πT) | [WPM21b] |
| Main formant | **500–800 Hz** measured; √(k_s/m_b)/2π = 480 Hz at 20 kN/m and 2.2 g [D], about 600 Hz [P21] | [WPM21a] |
| Bridge hills | ≈ **3 kHz**: rotation of the three legs, rocking of the bridge top; strongest in the lateral admittance. ≈ **5 kHz**: vertical at the middle (3rd string) notch | [WPM21a Fig. 7, WPM21b] |

- **Mobility.** Above 300 Hz the banjo's bridge admittance is **20–30 dB higher** than that of any of seven
  guitars [WPM21a Fig. 5].
  - Measured with damped strings, the bridge without its resonator, at the 1st string notch.
  - The resonator splits the lowest head peak (≈ 300 Hz without it) into two by coupling it to the back's
    Helmholtz mode [WPM21a Fig. 6].
  - Order of magnitude, from the decay rates in §1.2: Re(Y_b) ≈ η π Y0/2 ≈ 0.03 m/s/N (−31 dB) near 400 Hz
    for the fundamental of string 1 [D].
  - SM08's impedance/admittance plot had a ×10 (20 dB) calibration error and undamped strings [WPM21a]. Do not
    use its absolute values; its statement that the impedance is "below 25 kg/s at most frequencies" is
    therefore [U].
  - SM08's "string impedance of several hundred kg/s" is wrong: Z ≈ 0.14–0.29 Ns/m (table above).
- **Motion** [SM08, high-speed speckle interferometry]: the head's motion after a pluck is dominated by **rocking
  of the bridge**. It pivots about the foot far from the plucked string. The two outer feet drive the head; the
  middle foot hardly matters, so a solid rectangular bridge is an adequate first model. The 3rd (middle) string
  plucked vertically does not rock the bridge.
- What changes the sound [WPM21a/b, PWM21, Dickey 2003]:
  - **Bridge mass** acts as a low-pass filter. More mass lowers the main formant: mellower and quieter.
  - **Head tension**: tighter raises the main formant, mainly by suppressing low-frequency motion, and raises
    radiation damping. Looser is warmer and slightly less loud; decay hardly changes (Dickey).
  - Changes that move individual head modes but not the formant are subtle. Changes that move the formant are
    clearly audible.

Model for pajupilli [G]:
- **Bridge.** Three degrees of freedom, like the bass's bridge: vertical translation, rocking about the long
  axis (the outer feet in antiphase), and lateral sway.
  - Mass 2.2 g; a vertical spring of 20 kN/m (string-axial plus head-local stiffness) in parallel with the
    head's modes, driven at the two outer feet (x ≈ ±39 mm from the centre).
  - Rocking: give the bridge top an internal rocking resonance near 5 kHz.
  - Lateral: a lateral spring putting the sway resonance near 3 kHz.
- **Head.** Ideal-membrane modes, Bessel shapes evaluated at the feet, frequencies from the SM08 table (or the
  ideal ratios with the air-loading correction), and Q from §2.2. 20–30 modes up to about 3 kHz would be enough
  for this [G].

#### 2.4 Pot, tone ring, resonator

- Pot inner diameter about 9.8 in (250 mm); internal height for the axial cavity modes 2.97 in (75 mm), which
  first appear near **2.3 kHz**. Annulus modes (between the rim and the resonator wall) start near **500 Hz**.
  The standard resonator wall stops 3/8 in (9.5 mm) below the head [P16, Bacon internal-resonator study].
- The head's lowest mode couples to the back's Helmholtz mode, giving the two lowest body modes [P16, WPM21a].
  The pot's height (standard 2¼ in) has no measurable effect on the Helmholtz frequency [P16]. No published
  Helmholtz frequency was found. A guess: the resonator splits the ≈ 290 Hz head mode into ≈ 200 and ≈ 380 Hz
  [G]. An open-back banjo held against the player's body behaves partly like a vented enclosure too [WPM21b].
- Tone ring: a metal ring under the head's edge makes a harder, stiffer edge, with less low-frequency loss. Its
  flanges ring at high frequency and damp the high overtones: a faster-decaying "finish" [P16, U].
- The body modes ("clonk") are excited by every pluck. They decay much faster than the string modes but, at the
  note rate of banjo music, they ring into the next note. This punctuation is part of the banjo sound; the body
  modes extend up to about 2 kHz [WPM21a].

### 3. Excitation

- **Scruggs (three-finger) style:**
  - A plastic thumbpick, plus metal (nickel-silver or stainless) fingerpicks on the index and middle fingers.
  - Picks are about 0.018–0.025 in (0.46–0.64 mm) thick [U, vendors].
  - The ring and little fingers often rest on the head near the bridge [U, players' practice]. In a model that
    is extra head damping, or simply ignored [G].
  - A metal pick is hard and narrow: the string slips off in well under 0.1 ms [G], against about 1 ms for the
    bass's calloused finger. The spectrum is limited by the pluck position more than by the pick.
- **Pluck point:** about 7 cm above the bridge, "approximately the position that a player normally plucks"
  [SM08]. That is 0.10 of 0.67 m, so harmonics near 10, 20 are weak [D]. WPM21a plucked at 130 mm (≈ 1/5) with
  a plectrum.
  - Bluegrass players pick 2–4 in (5–10 cm) from the bridge [G]; closer is brighter and more "plunky".
  - Dickey: the pluck position has a complex effect on brightness.
- **Pluck direction:** mostly parallel to the head. The fingers pull slightly up, away from the head; the thumb
  pushes slightly down, toward it [G]. Horizontal plucks sustain longer [SM08], and the lateral bridge admittance carries the 3 kHz
  hill [WPM21a]. In a two-polarisation synthesis [WPM21a], a 90° (parallel) pluck sounded "most distinctive".
  Suggested angle: 20–40° into the head from the head's plane [G].
- **Clawhammer (frailing):**
  - No picks. A downstroke with the back of the index or middle fingernail, with the thumb catching the 5th
    string.
  - Played further from the bridge, over or near the end of the fingerboard, 15–20 cm from the bridge [G; the
    22nd fret of a 0.67 m scale is 19 cm from the bridge [D]].
  - The nail is hard but the stroke is a strike-and-brush rather than a pull. Model it as a pluck with a short
    pull (small depth, fast release) and a vertical (into-the-head) component [G].
- Hard plucks: the tension rises with amplitude, as in the bass (§1.2, tension modulation).
- Dickey: string displacement in a normal pluck is "about one-thousandth of its length", i.e. ≈ 0.7 mm [D].

### 4. Playing the banjo from a keyboard

- **Range:** open D3 (MIDI 50) up to about fret 22 on string 1, C6 (MIDI 84) [D]. GM synths usually map Banjo
  (program 106) over a similar range [G]. Below D3, fold up an octave, or retune string 4 to C3 (the "C tuning",
  cGCBD) [G].
- **Note to string:**
  - g4 (MIDI 67) goes to the open 5th string whenever it is free; it is the drone.
  - Otherwise use the lowest fret on the highest string that can play the note within a 4–5 fret hand position.
    Prefer open strings (D3, G3, B3, D4), as banjo players do.
  - The 5th string can be fretted only from fret 5 up (its first sounding fret is the 6th, A4), and rarely is
    [G].
- **Rolls** (Scruggs, 8 notes per cycle; T = thumb, I = index, M = middle):
  - Forward roll: T I M T I M T M.
  - Backward roll: M I T M I T M I.
  - Forward-reverse: T I M T M I T M.
  - Alternating thumb: T I T M T I T M.
  - Pinch: T + M together [U, standard method books, e.g. Scruggs 1968 cited in SM08].
  - The thumb takes strings 3–5, the index 2–3, the middle 1.
  - Notes come at about 8–10 per second at bluegrass tempos [G]; [WPM21a]'s synthesis example used 4 per
    second.
  - Keyboard idea: a held chord becomes a roll. The chord tones are voiced onto strings 1–4 (fretted shapes), the
    drone g4 goes to string 5, and the roll pattern plays at the tempo. Each finger keeps its own pick's
    character (thumbpick plastic and slightly softer [G]).
  - Each note keeps ringing until its string is replucked or refretted, so the roll builds up a sustained chord
    [G].
- Left hand: hammer-ons, pull-offs and slides (the bass's `setLength` glide), and bends. Bends raise the tension;
  they are mostly on strings 2–3, up to about a semitone [G].

---

## Part II: the koto (13-string, sō)

### 5. Strings

#### 5.1 Geometry and material

- Body (sōban) length about **1.82 m** (modern standard; historically 1.52–1.94 m) [Wiki]. 1.83 m in [C19];
  186 × 25 × 7 cm in Coaldrake's poster [C12].
- Each string runs from the fixed bridge at the player's right end (ryūkaku) to the fixed bridge at the far left
  end (unkaku/kashiwaba), about 1.7–1.75 m [G]. A movable bridge (ji) under each string divides it into the
  sounding part (ryūkaku to ji) and a silent rear part. The sounding length is **0.16–1.3 m** [CS11].
- Material: traditionally silk. Now polyester ("tetron"), either multifilament (4 twisted plies) or 4 twisted
  monofilaments [CS11, IY96]. Nylon too [SS]. All 13 strings of a 13-string koto have the same gauge
  (e.g. "17 monme") [U]. I found no diameter table: the Bachido compilation exists only as an image.

| Quantity | Silk | Multifilament polyester | Mono-polyester | Source |
|---|---|---|---|---|
| Linear density μ | 1.38 g/m (1380 tex) | 1.08 g/m | 1.05 g/m | [CS11 Table 1] |
| Tension on the koto | **≈ 147 N** (calculated by CS11 from length, μ and f) | same | same | [CS11] |
| Wave speed √(T/μ) | 326 m/s [D] | 369 m/s [D] | 374 m/s [D] | |
| Equivalent solid diameter | 1.15 mm (ρ ≈ 1340 [G]) [D] | 1.00 mm (ρ = 1380) [D] | 0.98 mm [D] | the real twisted string is a little thicker |
| Z = √(Tμ) | 0.45 Ns/m [D] | 0.40 Ns/m [D] | 0.39 Ns/m [D] | ≈ 2.6 × the banjo's |
| EA | ? | ≈ 8 kN (E ≈ 10 GPa for drawn PET, less when twisted) [G] | | |
| Inharmonicity B | — | ≈ 2e-5 at L = 0.63 m, ≈ 2e-4 at L = 0.21 m (4-ply bundle, I ≈ I_solid/4) [D from G] | | |

- Silk strings give fewer high-frequency components and cleaner integer partials than polyester (FFT of a
  40 cm string at 147 N cut loose with scissors) [IY96]. Players still prefer silk; polyester lasts 2.3–3.5×
  longer under the pick [CS11].

#### 5.2 Tunings

Hirajōshi in D [SS]: D G A A# D D# G A A# D D# G A for strings 1–13. String 1 is unison with string 5 in
classical pieces and an octave below it in modern pieces [SS]. Octaves and lengths at 147 N, polyester
(L = c/2f, c = 369 m/s) [D]:

| String | 1 (modern / classical) | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Note [U octave] | D3 / D4 | G3 | A3 | B♭3 | D4 | E♭4 | G4 | A4 | B♭4 | D5 | E♭5 | G5 | A5 |
| MIDI | 50 / 62 | 55 | 57 | 58 | 62 | 63 | 67 | 69 | 70 | 74 | 75 | 79 | 81 |
| Sounding length (m) | 1.26 / 0.63 | 0.94 | 0.84 | 0.79 | 0.63 | 0.59 | 0.47 | 0.42 | 0.40 | 0.31 | 0.30 | 0.24 | 0.21 |

- The extremes (1.26 m and 0.21 m) match CS11's 0.16–1.3 m, which supports both the 147 N figure and the
  octave placement [D].
- Other tunings [SS]:
  - Kokin-jōshi: D G A C D D# G A C D D# G A.
  - Kumoi-jōshi: D G G# C D D# G G# C D D# G A.
- The key is moved by moving all the ji. E is a common base as well as D [U]. Gagaku (gakusō) koto use
  A4 = 430 Hz [Stanford].

#### 5.3 String decay

- I found no published koto decay times. A Japanese patent (JPS62232559A) describes measuring a koto string's
  loss factor from its decay, without values [U].
- Air damping from the WPM21a formula, polyester d = 1 mm at 294 Hz: M ≈ 2.8, η_air ≈ 1.0e-3, so
  τ ≈ 0.55 s, T60 ≈ 3.7 s [D]. Add the polymer's internal loss, η ≈ 1–3e-3 [G], and the loss into the body.
- Proposal [G]:
  - T60 about 3–5 s for the fundamentals of the low strings and 1.5–2.5 s around A4.
  - Upper partials decay faster: loss rising about ∝ f^0.5–1.
  - Silk: 1.5–2× the high-partial loss of polyester (it has "fewer high components" [IY96]).
  - The body takes little energy (it is heavy and thick; see §6), so the decay is set mainly by the string, as
    in the guitar [G by analogy with WPM21a].
- The rear part of each string (ji to the far end, 0.5–1.5 m [D]) is untuned and is not plucked. It rings
  sympathetically through the ji, and it is where the left hand presses (§7.2). Model it like the bass's
  afterlength: about 6 modes, one polarisation [G].

### 6. The body

- **Construction:** a single hollowed paulownia (kiri, *Paulownia tomentosa*) log forms the top and sides.
  The interior is hollowed with an adze and is rough. A separate bottom plate has two sound holes. Internal
  struts and a partition; end plates [C19].
- **Wood:** density 250–280 kg/m³, highly anisotropic. The top's grain runs along the length. FE model elastic
  data from Waltham (unpublished, 2015). Wrong grain orientation moves the first FE eigenmode from 100 Hz to
  35 Hz or 24 Hz [C19].
- **Measured and modelled resonances:**

| Source | Frequencies (Hz) | Notes |
|---|---|---|
| Coaldrake CT-scan FE model + Chladni + acoustic camera + laser vibrometer [C19, C20] | **(0,0) / air ≈ 84–85**, **(0,1) top ≈ 99–100**, plus a strong unexplained ≈ 50 Hz component | the two dominant body resonances |
| Ando 1986 Chladni patterns, as reproduced on Coaldrake's poster [C12, A86] | 50, 80, 103, 132, 181, 211 | [U]: read from the poster labels |
| Coaldrake heuristic block model [C12] | 78, 89, 92, 148, 213, 232 | |
| Longitudinal air mode of the cavity, c/2L_in, L_in ≈ 1.7 m [G] | ≈ 100, 200, 300 … [D] | the cavity is a 1.7 m closed tube with two holes in the bottom |

- **Q / damping:** none published that I found. Woodhouse gives Q 30–100 for wooden instruments generally
  [WPM21a]. Proposal: Q ≈ 30 for the lowest top/air modes and ≈ 50 above [G].
- The body is massive compared with a guitar top: the top is thick (≈ 2–3 cm [G]) and the whole koto weighs
  about 5–7 kg [G]. Bridge admittance is likely well below a guitar's at the ji [G], hence the long, string-
  dominated decay. The played fundamentals (147–880 Hz) lie above the main body modes, which colour the attack
  and the low strings mostly.
- Coaldrake describes the played spectrum as rich in non-harmonic peaks beyond the string overtones; their origin
  is not understood [C12, C19]. Body modes excited by the pick transient, as in the banjo, are a likely part of it
  [G].
- **The ji:** historically ivory, now usually plastic, occasionally wood [Wiki]. Height about 5–6 cm and mass
  5–10 g [G]. It stands on two feet on the curved top. The string crosses its notch with a break angle on both
  sides. Treat it as a mass on a spring (the ji's own bending, a resonance in the low kHz [G]) standing on the body
  modes. The ryūkaku is a fixed bridge glued to the body.

### 7. Excitation and left-hand techniques

#### 7.1 Tsume (picks) and the pluck

- **Picks:** three, on the thumb, index and middle of the right hand [Wiki, SS]. Ivory (also bone or plastic) on
  a leather ring.
  - Ikuta-ryū: square-tipped picks (kaku-zume); CS11's commercial ivory Ikuta pick is 17 mm wide × 25 mm long.
    The player sits at an angle and uses the pick's corner [U].
  - Yamada-ryū: round-tipped picks (maru-zume); the player sits square to the instrument [U].
- **Where:** on the sounding part just to the left of the ryūkaku: "near the ryūkaku" [CS11]. The textbook
  value is 2–4 cm from it [G]. On a 0.63 m D4 string that is ≈ 5 % of the length, very bright, with the
  20th-harmonic notch above most of the audible spectrum [D].
- **How:**
  - The pick meets the string at about 45° to the string axis [CS11].
  - Standard stroke: index and middle pull toward the player, the thumb pushes away. Each stroke comes to rest on
    the next string [CS11: "from the near (or far) side, stopping at the next string"; directions U].
  - A rest stroke damps the next string. In a model, the landing pick stops that string, as a bass finger landing
    on a sounding string does.
  - The pull is mostly parallel to the top, with some push down into the string by the 45° corner [G].
- **Pick contact:** ivory is hard. The string is about 1 mm thick and slips off the pick edge in a fraction of a
  millisecond [G]. Depth of pull at mf: 2–5 mm [G] (147 N × 4 mm × (1/0.03 + 1/0.6) ≈ 20 N pluck force [D]).
- **Other right-hand techniques** [CS11, Kawai]:
  - Kaki-zume: two adjacent strings in one middle-finger stroke.
  - Awase-zume: an octave pair, thumb plus middle.
  - Sukui-zume: a thumb back-stroke (scoop).
  - Waren: a glissando across the strings.
  - Chirashi-zume and suri-zume: scraping along a string with the pick edges, mostly noise.
  - A string pressed onto the ji gives a muted, muffled tone.

#### 7.2 Left hand: oshide, yuri, hiki-iro

- **Oshide:** press the rear part of the string, about **15 cm left of the ji**, with the left index or middle
  finger [Kawai]. This raises the whole string's tension, so the pitch rises.
  - **Yowa-oshi: a semitone. Tsuyo-oshi: a whole tone.** Some classical pieces ask for a minor third, which is
    hard on tight tetron strings [Kawai].
  - Tension needed: a semitone is ×1.122 (+18 N); a whole tone is ×1.26 (+38 N) [D].
  - Strain for the whole tone: 38 N / EA (≈ 8 kN [G]) ≈ 5e-3, i.e. ≈ 8 mm over 1.75 m [D from G].
  - Pressing at a = 0.15 m from the ji, on a rear part of b ≈ 1 m: ΔL ≈ δ²/2 (1/a + 1/b) gives a depth of
    δ ≈ 45 mm and a finger force of T δ (1/a + 1/b) ≈ 65 N [D from G].
  - This assumes the tension equalises across the ji (no friction lock) [G]. Players' reports that oshide hurts
    and leaves marks [Kawai] fit a force of this size.
- **Ato-oshi:** press after the pluck, a rising pitch glide on the ringing note. **Oshi-hanashi:** pluck while
  pressed, then release: a falling glide. **Tsuki-iro:** a quick push and release [Kawai].
- **Yuri (yuri-iro):** vibrato. Wiggle the rear part right after the pluck, either holding or pushing. No rate or
  depth is published [Kawai]. Proposal: 4–6 Hz, ±20–50 cents, starting 0.1–0.3 s into the note [G].
- **Hiki-iro:** pinch the string left of the ji just after the pluck and slide toward the ji, loosening the
  tension: the pitch falls [Kawai].
- **Model:** the string's tension T(t) = T0 (1 + p(t)), where p is driven by the left hand. All mode frequencies
  scale by √(1+p). The rear part's modes scale too, and the ji stays put. The finger on the rear part adds
  damping to the rear modes only [G].

### 8. Playing the koto from a keyboard

- Default: hirajōshi in D (modern, string 1 = D3). The strings give MIDI 50, 55, 57, 58, 62, 63, 67, 69, 70, 74,
  75, 79, 81. With up to a whole tone of oshide the reach is 50–83 [D].
- Pitch classes on open strings: D, E♭, G, A, B♭. With oshide (in the order players prefer) [D]:
  - E = E♭ + semitone (or D + whole tone).
  - F = E♭ + whole tone.
  - G♯ = G + semitone.
  - B = B♭ + semitone (or A + whole tone).
  - C = B♭ + whole tone.
  - C♯ and F♯ need a minor third (B♭ + 3, E♭ + 3): rare and hard on polyester [Kawai]. Play them as a strong
    push, or silently move a ji (retune) between phrases, or switch tuning (kumoi-jōshi has G♯ and C) [G].
- **Mapping:** each key finds the string whose open pitch is the key or 1–2 semitones below it. Prefer the open
  string. Otherwise press first (oshide, with the press preceding the pluck by about 50–100 ms [G]), then pluck.
  - The pitch-bend wheel drives ato-oshi or hiki-iro on the last string, ±2 semitones.
  - Modulation drives yuri.
  - Velocity sets the pull depth.
  - Two keys an octave apart within about 30 ms play awase-zume. Adjacent strings struck together play kaki-zume
    [G].
- The finger choice follows the direction: rising lines with the thumb (pushing away), falling or repeated notes
  with index and middle [U/G].

---

## 9. Published synthesis and models with parameters

| Work | What | Parameters given |
|---|---|---|
| Woodhouse, Politzer, Mansour 2021 [WPM21a] | Banjo: frequency-domain (inverse FFT of the coupled string + measured admittance) and modal synthesis, one and two polarisations; separate "string modes" and "body modes" | strings (Table 2), head (Table 1), string-loss model (eqs. 4–7), Y-based loss (eqs. 1–3); sounds at euphonics.org §5.5 |
| Woodhouse, Politzer, Mansour 2021 [WPM21b] | Banjo theory: "square banjo" rectangular membrane (closed-form modes, radiation damping, added mass 0.15), FE/BE, point coupling 1/Y_b1 = 1/Y_1 + iω m_b + k_s/(iω) + λ | m_b 1.5–2.2 g, k_s 20 kN/m, λ 1 Ns/m, η_struct 4e-3, Q 50, a footprint of 5 mm half-width to avoid the log singularity of a point-forced membrane |
| Politzer, Woodhouse, Mansour 2021 [PWM21] | Pickers' guide; synthesis variations (bridge mass, break angle) | model bridge 1.5 g; post-filter ×f^0.4 to boost the highs |
| Politzer 2014, 2019, 2021 [P14, P19, P21] | Tension modulation via the break angle; bridge spring arithmetic | §1.2, §2.3 |
| Dickey 2003 (JASA 114, 2958–2966) [D03, U] | Time-domain coupled model: 5 strings + 1 membrane | not obtained; press summaries only |
| Stephey & Moore 2008 [SM08] | Measurements; says the circular-membrane modes are a good modal basis | §2.1 |
| Lasickas (Aalborg MSc) [L] | Shamisen physical model with a banjo section | not read [U] |
| Coaldrake 2018–2020 [C19, C20] | Koto FE (COMSOL, 966k elements, 34 M dof with air); strings as beam/truss models (ISMA 2019) and as waveforms injected at the bridges in hirajōshi | body modes only; no synthesis parameters found |
| Penttinen et al. 2006 [PPV06] | Guqin (a fretless Chinese zither, a relative): digital waveguide with a ripple filter for flageolets; body filter in cascade | the nearest published zither synthesis |

No published real-time koto physical model with parameters was found.

## 10. Summary for implementation (both on the `Bass` pattern)

| | Banjo | Koto |
|---|---|---|
| Strings | 5 steel, 0.67 m (5th: 0.50 m), 46–61 N, Z 0.14–0.29 Ns/m, B 1–6e-5 | 13 polyester, sounding 0.21–1.26 m, all ≈ 147 N, Z ≈ 0.40 Ns/m, B ≈ 2e-5–2e-4 [D/G] |
| Modes per string | 64 × 2 polarisations (up to ≈ 10 kHz needed: the bright attack) | 48–64 × 2 |
| Rear segment | 5 cm afterlength to the tailpiece (stiffens the bridge, k ≈ 2.4 kN/m total) | 0.5–1.5 m rear part, ~6 modes; the left hand presses here |
| Bridge | 2.2 g; 3 DOF (vertical, rock, lateral); k_s 20 kN/m; hills at 3 and 5 kHz | 13 separate ji, each 5–10 g [G], on the body; ryūkaku fixed |
| Body | Membrane modes (SM08 list), η = 4e-3 + radiation (0.05 at 300 Hz falling as f^-1.5 [G]); resonator: Helmholtz split [G] | Modes at 50, 85 (air), 100 (top), 130–230 Hz …, Q 30–50 [G] |
| Decay of string modes | τ 100–200 ms below 1 kHz (T60 0.7–1.4 s), set by the head | T60 ≈ 2–5 s [G], set by the string |
| Excitation | Metal fingerpicks, 7 cm from the bridge; clawhammer 15–20 cm | Ivory tsume, 2–4 cm from the ryūkaku [G], 45° corner, rest stroke |
| Pitch nonlinearity | Tension modulation via the break angle (the "ring") | Oshide: tension +12 % / +26 % from the left hand |

---

## References

[A86] Y. Ando, "Acoustics of sohs ('koto's)", Proc. 12th ICA, Toronto (1986), vol. 3, K1-5. Y. Ando, *Gakki no Onkyōgaku*, 2nd ed., Ongaku no Tomo Sha (1996). (not obtained; via [C12, C19])
[C12] K. Coaldrake, "Towards a new method for the evaluation of the tonal colouring of the Japanese koto using COMSOL Multiphysics", COMSOL Conference 2012, poster. https://www.comsol.com/paper/download/159347/coaldrake_poster.pdf ; page: https://www.comsol.com/paper/toward-an-evaluation-of-the-tonal-colouring-of-the-japanese-koto-using-comsol-multiphysics-and-acoustics-module-14676
[C19] K. Coaldrake, "Measurement and modelling of the Japanese koto: problems and solutions", Proc. 23rd ICA, Aachen (2019), 7549–7556. https://www.pub.dega-akustik.de/ICA2019/data/articles/001444.pdf
[C20] K. Coaldrake, "A finite element model of the Japanese koto constructed from computed tomography scans", JASA (Nov. 2020), doi:10.1121/10.0002427. (abstract via secondary sources only: https://eres.library.adelaide.edu.au/dspace/handle/2440/136206 ; [U]) Also Coaldrake, "Finite element modelling of Japanese koto strings", ISMA 2019, Detmold (not read).
[CS11] T. Chinen, A. Sakaguchi, H. Kimura, Y. Matsumoto, H. Morooka, H. Morooka, "A measuring method for the durability of So-strings" (箏絃の撥絃耐久性の評価法), J. Textile Eng. 57(3), 69–74 (2011). https://www.jstage.jst.go.jp/article/jte/57/3/57_3_69/_article/-char/ja
[D03] J. W. Dickey, "The structural dynamics of the American five-string banjo", JASA 114(5), 2958–2966 (2003). (not obtained.) Press summaries: https://acoustics.org/pressroom/httpdocs/145th/Dickey.htm , https://acoustics.org/pressroom/httpdocs/159th/dickey.htm
[IY96] H. Ishikura, S. Yamashita, H. Torigai, Y. Taniyama, K. Kamizawa, M. Nakajima, "Trial makings of string for 'koto' using silk-like polyester fiber", Sen'i Kikai Gakkaishi 49(7), T175–T182 (1996). https://www.jstage.jst.go.jp/article/transjtmsj1972/49/7/49_7_T175/_article/-char/en
[Kawai] Kawai Shamisen & Koto shop blog (informal): "Oshide" https://kawai-samisen-koto-en.blogspot.com/2024/02/blog-post.html ; "Yuri-iro, mute, oshi-hanashi" https://kawai-samisen-koto-en.blogspot.com/2024/05/koto-tips-techniquesyuri-iro-mute-oshi.html ; "Sukui-zume and hiki-iro" https://kawai-samisen-koto-en.blogspot.com/2024/05/koto-tips-techniquesukui-zume-and-hiki.html
[L] T. Lasickas, MSc thesis, Aalborg University (shamisen, banjo section). https://projekter.aau.dk/projekter/files/402526672/SMC10_Masters_Thesis_TitasLasickas.pdf (not read)
[P14] D. Politzer, "Banjo timbre from string stretching and frequency modulation", arXiv:1406.4907 (2014). https://arxiv.org/abs/1406.4907
[P16] D. Politzer, "Air modes of the Bacon internal resonator banjo", arXiv:1610.02355 (2016). https://ar5iv.arxiv.org/html/1610.02355
[P19] D. Politzer, "Banjo ring from stretching string: a zero break angle demo", arXiv:1903.03186 (2019). https://arxiv.org/abs/1903.03186
[P21] D. Politzer, "Arithmetic of spring forces on the banjo bridge", arXiv:2104.06616 (2021). https://ar5iv.labs.arxiv.org/html/2104.06616
[PPV06] H. Penttinen, J. Pakarinen, V. Välimäki, M. Laurson, H. Li, M. Leman, "Model-based sound synthesis of the guqin", JASA 120(6), 4052–4063 (2006). https://research.aalto.fi/en/publications/model-based-sound-synthesis-of-the-guqin/ (abstract only)
[PWM21] D. Politzer, J. Woodhouse, H. Mansour, "Pickers' guide to acoustics of the banjo, parts I and II", arXiv:2104.02480 (2021). https://arxiv.org/abs/2104.02480
[SM08] L. A. Stephey, T. R. Moore, "Experimental investigation of an American five-string banjo", JASA 124(5), 3276–3283 (2008), doi:10.1121/1.2982371. https://www.csc.kth.se/utbildning/kth/kurser/DT2212/Banjo%20Moore%20et%20al%20JASA.pdf
[SS] European Shakuhachi Society, "Koto" (tunings). https://shakuhachisociety.eu/shakuhachi/koto/
[Stanford] Stanford Gagaku, "Koto". https://gagaku.stanford.edu/en/strings/koto
[Wiki] "Koto (instrument)", Wikipedia. https://en.wikipedia.org/wiki/Koto_(instrument)
[WPM21a] J. Woodhouse, D. Politzer, H. Mansour, "Acoustics of the banjo: measurements and sound synthesis", Acta Acustica 5, 15 (2021), doi:10.1051/aacus/2021009. https://authors.library.caltech.edu/records/w37w3-6r253
[WPM21b] J. Woodhouse, D. Politzer, H. Mansour, "Acoustics of the banjo: theoretical and numerical modelling", Acta Acustica 5, 16 (2021), doi:10.1051/aacus/2021008. https://authors.library.caltech.edu/records/eaegy-pw942

Not obtained: Dickey 2003 full text; Rae & Rossing, "The acoustics of the banjo", ISMA 2004 (Nara), and JASA 117, 2590 (2005) abstract;
Ando 1986/1996 (the only koto Chladni measurements); Coaldrake JASA 2020 full text; Sanadzadeh's 2019 Adelaide PhD thesis on koto models
(link dead); Shirasuna, "Differences between silk, nylon and tetron koto strings" (ASJ meeting, 1965–69); any measured koto string decay
or ji admittance; a koto-string diameter/monme table.
