# Steel guitars: the steel-string acoustic (GM 26) and the lap, Dobro and pedal steel — research

Status: research only (2026-11-19). Nothing here is implemented. GM "Acoustic Guitar (steel)" is
program 26 counting from 1 (25 from 0). Today `src/midifile.cc`'s `kGm` table sends
"steel-guitar" to the banjo and then the koto. The classical guitar is built
(`src/guitar.h/.cc`, `src/guitarinst.h/.cc`, `-inst nylon-guitar`, `research/guitars.md` §13);
this note says what a steel-string guitar must change in it, and what a slide instrument (lap
steel, Dobro, pedal steel) would need later.

**Layout.** Sections 1–15 are the first pass (2026-11-19). `research/steel_guitar_build.md` is
the build supplement (FluidR3 measured, GuitarSet strum timing, the body table, a first pass on
the 12-string). **Part 3, sections 16–25 after the first reference list (2026-11-20)**, adds
construction and woods, the medium set, measured inharmonicity and whole-note decay, air drag,
string ageing, an X-braced modal map and its dead notes, pluck forces and geometry, the playing
culture (flatpicking, bends, vibrato, harmonics, capo, tunings, chord shapes, MIDI reading),
the vocabulary of good and bad sound, the 12-string as a second variant, and the model tables
with every number marked measured / derived / guess. Part 3 uses its own marks ([MEAS], [DER],
[GUESS]); they are defined at its head, which also lists six corrections to parts 1–2.

How each number is marked (the scheme of `research/timpani.md`):
- **[T]** read in the primary text (downloaded and read for this note). "Measured" is said in
  words where the text reports a measurement and not a calculation.
- **[S]** from a secondary source, an abstract, a retailer or forum page, a search snippet or a
  machine summary of a page: the primary text was not read.
- **[L]** player or luthier lore: a teaching text, maker's setup sheet, forum post by a player.
  Not a measurement.
- **[E]** my estimate, fit or arithmetic from cited values; not a published value.
- **[M]** from general knowledge, not looked up: check before relying on it.

References are short keys in brackets; the list with URLs is section 15. What could not be found
is in section 14.

**Short answer.** The steel-string acoustic is the nylon guitar's body with nearly twice the
pull on it, strung with strings that lose almost nothing of their own, and played with a pick.

- **Strings:** a light set (.012–.053) pulls 104, 104, 134, 133, 129 and 111 N on a 645–648 mm
  scale, 714 N in all **[T]** [DAD]. The classical's set pulls about 400 N. The plain strings
  are steel; the wound ones are a steel core wrapped in phosphor bronze (92/8 Cu/Sn) or "80/20"
  brass. The characteristic impedance √(Tμ) is 0.24 Ns/m on the high E and 1.04 on the low E, about
  1.3× and 1.8× the nylon's **[E]**.
- **What the ear gets from that:** steel's own loss is tiny (Q ≈ 2 000–5 000 against nylon's
  few hundred, falling with frequency) **[T]** [Euph, PLC14]. So the upper partials, 2–8 kHz,
  ring for seconds instead of tenths of seconds: the shimmer that names the instrument. The
  fundamentals of the bass strings last 5–15 s **[E]**. The wound strings are stiff:
  B ≈ 1e-4 on the low E puts its 10th partial 10 cents sharp **[E]**.
- **Body:** X-braced. Its first three admittance peaks are measured: Martin D-28 at 102, 193 and
  204 Hz, then see-saw modes at 377 and 436 Hz **[T]** [FR]. Six Fylde "Falstaff" guitars give
  96–101 Hz (Q 33–42), 175–188 Hz (Q 17–28) and 335–368 Hz (Q 24–58) **[T]** [CBWFP18]. Those are
  the classical's frequencies to within 10 % (`guitar.cc` has 100, 200, 236, 290, 400). What
  differs is the order of the cross-grain and long-grain top modes, the back tuned to the top, and
  a lower, smoother mobility above 500 Hz: about −53 dB re 1 m/(s N), 0.002 m/(s N), measured
  **[T, read from the figure]** [CBWFP18].
- **The pick:** a 0.5–1.5 mm sheet of celluloid, acetal or nylon. It holds the string over less
  than 1 mm and lets go in about 0.1–0.3 ms **[M]**: there is no flesh stage. A strummed
  stroke crosses six strings 10.5 mm apart. In the user's MIDI files, half the chords are written
  as one instant; the rest are spread, 8 ms per string at the median (16 ms for three notes, 32 ms
  for six), 53 % of them low to high (§9.2).
- **The rest is noise and setup:** the pick's click and scrape, the squeak of a finger sliding on
  a bronze winding (one pulse per turn of the wrap, measured in kind on slide guitars) **[T]**
  [PPV08], fret buzz at 2.4 mm action on the bass side **[L]** [Martin], and strings that go
  dull within hours of playing **[T]** [FR].
- **GM 26 is the most used guitar in the user's collection:** 1 199 files in
  `~/midifiles` and `~/Music/midi` (with duplicates) carry 1.49 million notes on program 26 (§9).
  `research/guitars.md` §0 put it last because its five jazz files had none. Real pedal-steel
  parts also exist, in country files, on programs 27 and 94 with dense pitch bends (§9.5).
- **Slide instruments (part 2):**
  - A steel bar replaces the frets: the length changes continuously, with vibrato of about 6 Hz
    and 30–70 cents **[L]**.
  - The bar's own noise on wound strings is a train of clicks, one per turn of the winding
    **[T]** [PPV08].
  - Tunings are open: the Dobro in G (G2 B2 D3 G3 B3 D4) **[T]** [RS19]; the lap steel in C6;
    the pedal steel in E9 (10 strings, 3 pedals, 4–5 knee levers that raise or lower chosen
    strings by a semitone or a tone) and C6, or a 12-string universal E9/B6 **[S]** [WikiCop].
  - Electric slide instruments go through a pickup, a volume pedal (attack hidden, swell after
    it) and a clean amp with reverb.
  - The Dobro's cone is a stiff piston of about 40 g on a spring of about 10⁵ N/m, a broad peak
    near 250–300 Hz **[T]** [Pol23], with modes that soften (fall in frequency) at large
    amplitude **[T]** [RS19].

---

## 1. Which instrument

### 1.1 Body styles

| Style | Lower bout | Depth (at the tail) | Scale | Typical sound | Source |
|---|---|---|---|---|---|
| Dreadnought (Martin D-18/D-28) | 15⅝ in. (397 mm) | ~4⅞ in. (124 mm) | 25.4 in. (645 mm) | big bass, "boom" near G2 | [M]; scale [S] |
| Slope-shoulder dreadnought (Gibson J-45) | 16 in. | ~4¾ in. | 24.75 in. (629 mm) | darker, drier | [M] |
| OM / 000 (Martin) | 15 in. (381 mm) | ~4⅛ in. (105 mm) | 25.4 (OM) / 24.9 in. (000) | balanced, quicker, finger-style | [M] |
| Jumbo (Gibson J-200) | 17 in. | ~4½ in. | 25.5 in. | | [M] |

- **For GM 26 take the dreadnought** **[E]**. It is the default "acoustic guitar" of the pop,
  rock and country records that most GM files transcribe (§9: the Eagles, the Beatles, ABBA,
  REM). An OM body is a variant: smaller, its air mode 5–15 % higher (§3.2).
- **Scale for the model:** 645 mm (25.4 in.); D'Addario's charts use 25.5 in. (648 mm) **[T]**
  [DAD]. The 3 mm makes 1 % in tension.
- **Frets:** 20 on a 14-fret dreadnought **[M]**; the top fret is C#6 on the high E (85).

### 1.2 GM2's variations

- GM2 adds bank variations of program 26: a 12-string, a mandolin and a "steel guitar with body
  sound"; and the pedal steel as a variation of program 27, the jazz guitar **[M]** (not read in
  the GM2 specification). `research/gm2_extras.md` does not mention them. The project ignores
  bank select for these today, so this matters only if it starts reading it. The user's country
  files put their pedal steel on program 27 without a bank change (§9.5).

## 2. Strings

### 2.1 Light set (12–53) on 648 mm

| String | Item | Kind | d (in.) | T (N) | μ (g/m) | Z₀ = √(Tμ) (Ns/m) | Core (in., est.) | B |
|---|---|---|---|---|---|---|---|---|
| E4 329.6 Hz | PL012 | plain steel | .012 | 103.9 | 0.57 | 0.243 | — | 1.9e-5 |
| B3 246.9 | PL016 | plain steel | .016 | 103.7 | 1.01 | 0.324 | — | 6.1e-5 |
| G3 196.0 | PB024 | phosphor bronze on hex steel | .024 | 133.7 | 2.07 (2.09 [S]) | 0.527 | .0105 | 0.9e-5 |
| D3 146.8 | PB032 | " | .032 | 133.1 | 3.68 (3.75 [S]) | 0.700 | .013 | 2.1e-5 |
| A2 110.0 | PB042 | " | .042 | 128.7 | 6.34 (6.56 [S]) | 0.903 | .016 | 4.9e-5 |
| E2 82.4 | PB053 | " | .053 | 111.0 | 9.74 (10.17 [S]) | 1.040 | .019 | 1.1e-4 |

- **Tension:** D'Addario's EJ16 chart on the maker's page gives 10.619, 10.595, 13.66, 13.605,
  13.152 and 11.34 kg (E4 → E2) at 25.5 in. **[T]** [DAD]; retailer copies of the same chart in
  pounds: 23.36, 23.31, 30.06, 29.93, 28.93, 24.95 lb **[S]**. Total 160.5 lb = 714 N.
  - Martin's MA540 (92/8 phosphor bronze, .012-.016-.025-.032-.042-.054) totals 168.5 lb, its
    80/20 twin MA140 167.3 lb **[S]** [Martin-str].
  - D'Addario's 80/20 EJ11 (same gauges): 23.3, 23.3, 29.4, 29.5, 28.4 lb for the first five
    **[S]** [DAD-EJ11]. The 80/20 wound strings are 2 % lighter: brass (~8.5 g/cm³) is less dense
    than phosphor bronze (~8.8) **[M]**.
  - Fletcher & Rossing: "steel strings require tensions of 100 to 180 N", nylon 50–80 N **[T]**
    [FR].
- **μ** is backed out of the tension (T = μ (2Lf)²) **[E]**. The bracketed μ is D'Addario's own
  unit weight converted to g/m by a third party **[S]** [WSH]; they differ by 1–4 %, the chart's
  rounding or the scale.
- **Inharmonicity** B = π³ E d⁴ / (64 T L²), E = 200 GPa, d the plain wire or the *core*
  **[E]**. The core diameters are my guesses (D'Addario does not publish them); the wrap adds a
  little stiffness, so the wound strings' B is a lower bound. Partial k is (√(1 + B k²) − 1) ×
  1731 cents sharp: on the low E the 10th partial is 10 cents sharp, the 20th 35 cents; on the
  G string the 36th is 10 cents sharp **[E]**. A banded, metallic low E needs this.
- **Gauges and tunings that matter:**
  - Extra light 10–47 and custom light 11–52: ~20 % and ~10 % less tension **[M]**.
  - Medium 13–56: ~185 lb, 820 N **[M]**.
  - Drop D (low E down to D2, 73.4 Hz): the low string at 79 % of its tension, floppier,
    higher B, buzzier **[E]**.
  - A capo moves the nut; GM files never say so, but voicings in §9 show where one would be.
- **Comparison with nylon (`guitar.cc`):**

| | Nylon (built) | Steel, light |
|---|---|---|
| Tensions (N), low E → high E | 62, 68, 70, 61, 62, 83 | 111, 129, 133, 134, 104, 104 |
| Z₀ low E / high E (Ns/m) | 0.58 / 0.19 **[E]** | 1.04 / 0.24 **[E]** |
| B low E / high E | 1e-5 / 1.6e-5 | 1.1e-4 / 1.9e-5 |
| String's own Q | low, falling with f (a few hundred) **[T, qualitative]** [Euph] | ~3 000, "typical of a steel guitar string" **[T]** [Euph]; Q_disl 5 500 [T, PLC14] |

### 2.2 Losses and decay

- **Three mechanisms** in the string: air drag, internal friction (viscoelastic and
  thermoelastic), and on wound strings friction between the turns of the wrap **[T]** [PLC14,
  C11 abstract]. A fourth is the bridge: the energy the body takes and radiates.
  - PLC14 fit an electric's plain G (.017): δ_VE/TE ≈ 1e-3 (published values 1e-4…1e-3) and
    Q_disl = 5 500 **[T]** (quoted from `research/guitars.md`).
  - Christian's Purdue thesis on bronze-wound acoustic strings: the harmonic decays were "primarily
    sensitive to the aerodynamic damping parameter" **[S]** [C11].
  - Woodhouse: "the internal damping of the kind of steel used for musical strings is very low";
    for his steel string synthesis "all modes … have the same Q-factor … 3000 … typical of a
    steel guitar string" **[S, machine summary of the page]** [Euph].
- **The bridge's share.** A string with one end on a conductance G (the real part of the bridge
  mobility) loses amplitude at α ≈ 2 f₁ Z₀ G per second **[E]** (the reflection coefficient is
  1 − 2 Z₀ G per round trip). With the measured mobility of §3.3:
  - between body peaks, G ≈ 0.002–0.005 m/(s N): low E α ≈ 0.4–0.9 /s (T60 8–17 s); high E
    α ≈ 0.3–0.8 /s (9–20 s);
  - on a body peak, G up to 0.05–0.09: the partial of the low E that lands on the 180 Hz top mode
    (its 2nd, at 165 Hz) loses α ≈ 14 /s, T60 ≈ 0.5 s; the high E's fundamental near the 340 Hz
    mode α ≈ 4 /s, T60 ≈ 1.5 s **[E]**. Near such a coincidence the coupled string and body modes
    veer apart, and the simple formula overstates the loss; Woodhouse's modal method gets it
    right **[T]** [W04a] (`guitar.cc` already couples string and body modes directly, so it does
    the veering itself).
- **Total (estimate, to be fitted)** **[E]**:

| | Fundamental T60 | Partials at 2–4 kHz | Partials at 6–8 kHz |
|---|---|---|---|
| Low E (open) | 8–15 s | 2–4 s | 0.7–1.5 s |
| D, G (wound) | 6–10 s | 1.5–3 s | 0.5–1 s |
| High E, B (plain) | 3–8 s | 2–5 s (air drag small, internal loss small) | 1–2 s |

  Compare the nylon as built: fundamentals 3–8 s (bass) and 1–3 s (trebles), partials above
  2 kHz 0.2–0.5 s (`research/guitars.md` §1.2). The steel's high partials last **5–10× longer**.
  That, more than the attack, is what a listener calls "steel".
- **Wound against plain:** the wound strings' wrap friction adds a loss that rises with frequency
  **[S]** [C11], so above ~3 kHz the plain B and E ring longer than the wound G and D **[E]**.
  This is why the G string, when wound, is the dullest (and why many players hear the wound G as
  the "dead" string) **[L]**.
- **Old strings.** "Guitar strings, especially the lower strings which are wrapped, become 'dead'
  after a short period of playing … the main cause is increased damping due to foreign material
  … imbedded between the windings" **[T]** [FR §9.3]. Players say bronze sounds best in its first
  hours; 80/20 brightest when new and dulls fastest, phosphor bronze holding its sound longer
  **[L]** [UMGF, Cream City]. A model parameter: `age` 0..1 raising the wound strings' loss1
  (the k-dependent term) by up to 3× **[E]**.
- **Phosphor bronze against 80/20:** no measurement found. The lore: 80/20 brighter and "crisper"
  at first, phosphor bronze warmer with more low-mid and a longer life **[L]**. Model it as a few
  per cent in mass (80/20 lighter) and a little less loss at 3–8 kHz for the 80/20 when new
  **[E]**; a listener will not tell them apart in a mix.

### 2.3 Two polarisations

- As on the nylon (`research/guitars.md` §1.3), the motion normal to the top dies fast and the
  motion along it rings on; F&R show the two decays and the compound envelope of a guitar tone
  **[T]** [FR Fig. 9.18].
- **With a pick** the force is mostly along the top (the pick moves across the strings), plus a
  normal part from the pick's angle and from the string riding up the pick's face. A downstroke
  pushes the string slightly towards the top, an upstroke away from it **[E]**. Start at an angle
  of 0.25–0.4 rad from the top's plane for a pick, against the finger's apoyando/tirando range in
  `guitar.cc`.
- A necessary condition for a double decay is a difference between the bridge's admittance in
  the two directions **[S]** [W21]. The steel-string bridge is a pinned bridge on a bridge plate,
  stiffer in the plane than the classical's tied bridge **[M]**; keep `along` (the share of each
  body mode in the in-plane direction) as on the nylon.

## 3. The body

### 3.1 X-bracing against fan bracing

- **The structure.**
  - A steel-string top is X-braced: two braces crossing just below the soundhole, a maple bridge
    plate under the bridge, two or more tone bars below the X, and transverse braces above the
    soundhole **[M]**.
  - The back has a centre strip and four cross braces (three on most classical guitars) **[T]**
    [FR].
  - The X takes the torque of 700 N pulling about 10 mm above the top: the bridge would
    otherwise rotate the top, bulging behind the bridge and sinking in front **[M]**.
- **What it does to the modes:**
  - "the (1,0) mode now occurs at a higher frequency than the (0,1) mode, and the (2,0) mode has
    moved up in frequency and changed its shape because of the crossed bracing" **[T]** [FR].
  - "In the fan-braced classical guitars, the (0,1) mode occurs at a higher frequency than the
    (1,0) mode, while in the cross-braced top plate of the folk guitars and in all the back
    plates, the reverse is true. In the Martin D-28, the fundamental modes of the top plate and
    back plate are tuned to almost the same frequency" **[T]** [FR].
  - The see-saw (1,0) resonance: "around 300 Hz in a classical guitar but closer to 400 Hz in a
    cross-braced folk guitar. In the D-28, this coupling is quite strong at 377 Hz" **[T]** [FR].
  - "A fairly prominent (2,0) top plate resonance is usually observed in classical guitars
    around 550 Hz, but this mode is much less prominent in folk guitars" **[T]** [FR].
- **Components with the rest of the body clamped in sand** (Rossing et al. 1985, in F&R Table
  9.1; all **[T]**, measured):

| Top plate | (0,0) | (0,1) | (1,0) | (0,2) | (1,1) | (0,3) | (2,0) | (1,2) |
|---|---|---|---|---|---|---|---|---|
| Martin D-28 | 163 | 326 | 390 | 431 | 643 | 733 | 756 | |
| Martin D-35 | 135 | 219 | 313 | 397 | 576 | 626 | 648 | 777 |
| Kohno 30 (classical) | 183 | 388 | 296 | 466 | 558 | | 616 | 660 |
| Conrad (classical) | 163 | 261 | 228 | 382 | 474 | | 497 | |

| Back plate | (0,0) | (0,1) | (0,2) | (1,0) | (0,3) | (1,1) | (2,0) | (1,2) |
|---|---|---|---|---|---|---|---|---|
| Martin D-28 | 165 | 257 | 337 | 369 | 480 | 509 | 678 | 693 |
| Martin D-35 | 160 | 231 | 306 | 354 | 467 | 501 | 677 | |
| Kohno 30 | 204 | 285 | 368 | 417 | 537 | 566 | 646 | 856 |
| Conrad | 229 | 277 | 344 | 495 | 481 | 573 | 830 | 611 |

| Air cavity | A0 (Helmholtz) | A1 (0,1) | A2 (1,0) | A3 (1,1) | A4 (0,2) | A5 (2,0) |
|---|---|---|---|---|---|---|
| Martin D-28 | 121 | 383 | 504 | 652 | 722 | 956 |
| Martin D-35 | 118 | 392 | 512 | 666 | 730 | 975 |
| Kohno 30 | 118 | 396 | 560 | 674 | 780 | |
| Conrad | 127 | 391 | 558 | 711 | 772 | 1033 |

  (The A0 here is the cavity with rigid walls; coupled to the top it becomes the ~100 Hz
  "breathing" resonance below.)

### 3.2 The assembled guitar

- **Martin D-28, freely supported, driven at the bridge** **[T]** [FR §9.7]:
  - 55 Hz: a bar-like bending mode of the whole guitar, "probably has little or no musical
    importance because it lies well below the lowest string frequency";
  - **102 Hz**: the breathing mode (top and back in opposite directions, air out of the
    soundhole in phase);
  - **193 and 204 Hz**: the top's and back's (0,0) with the air; at 193 Hz the lower ribs and
    tail block move against the plates; clamping the ribs lowers it to 169 Hz;
  - **377 Hz**: the (1,0) see-saw, strong coupling of top, back and air;
  - **436 Hz**: a (0,1)-type resonance.
  - The two-oscillator rule f₁² + f₂² = f_A² + f_p² holds with ribs clamped (Ross & Rossing,
    Christensen & Vistisen) **[T]** [FR §9.5]; with the back, f₁² + f₂² + f₃² = f_A² + f_p² + f_b²
    **[T]** [FR §9.6].
- **Six Fylde "Falstaff" steel-string guitars** (a relatively large body, Sitka top, 80/20
  strings), bridge admittance normal to the top between the 5th and 6th strings, measured by
  hammer and laser vibrometer **[T]** [CBWFP18 Table I]:

| Back wood | F1 (Hz) | Q1 | F2 (Hz) | Q2 | F3 (Hz) | Q3 |
|---|---|---|---|---|---|---|
| Brazilian rosewood | 97 | 34 | 177 | 18 | 336 | 36 |
| Indian rosewood | 101 | 38 | 188 | 26 | 368 | 58 |
| Maple | 100 | 35 | 187 | 25 | 363 | 53 |
| Mahogany | 99 | 33 | 184 | 25 | 351 | 35 |
| Sapele | 96 | 39 | 175 | 17 | 335 | 44 |
| Walnut | 98 | 42 | 182 | 28 | 347 | 24 |

  - "The first two are part of the coupled triad …, while the third peak is a transverse bending
    mode of the top plate" **[T]**.
  - The differences come from the top, not the back wood: F3 varies as much as the others and the
    three frequencies are correlated; the extreme guitars are half a semitone sharp and flat of
    the mean **[T]**.
  - Listeners discriminate shifts of low body-mode frequencies of 1–2 %; Q factors must change by
    about 20 % to be heard **[T]** (citing Woodhouse et al. 2012) [CBWFP18].
- **A mass-produced dreadnought** (Godin Seagull, 18 nominally identical guitars): the lowest
  "signature" mode T(1,1)₁ near 91–96 Hz; the guitars sorted by their maker as "bassy" had it
  about 5 % lower; the "treble" guitars had lower mean mobility between 600 Hz and 2 kHz (about
  2 dB) **[T]** [FMSS14].
- **OM / 000 against dreadnought** (no measured modal table found for an OM; lore only):
  - air resonance of a dreadnought "around G on the low E string (~98 Hz)"; about 95 Hz for a
    Martin dreadnought, below 90 Hz for a Gibson 16 in. jumbo-shouldered body; OM and Gibson
    L-00 "air resonances ranging between 100 Hz to 115 Hz"; one builder's OMs on G# or A
    **[L]** [PG, UMGF, LF];
  - so the OM's air mode is 5–15 % higher, its top (0,0) mode likewise higher (smaller, lighter
    top) **[E]**; a quicker, less boomy bass and relatively more 300–1 000 Hz.
- **The dreadnought's "boom".** Its breathing mode at 96–102 Hz sits on G2–A♭2, the 3rd and 4th
  frets of the low E: the G chord's root, the most common chord in the files (§9), lands on it
  **[E]**. The 180–200 Hz top mode sits on the G3 / open G string. A model whose air mode is
  20 % off loses this.

### 3.3 Bridge mobility (the admittance the strings see)

- **Measured, six Fylde guitars and a Yamaha FG-403MS** (read off [CBWFP18] Fig. 1A,
  ±2 dB) **[T]**:

| Region | |Y| (dB re 1 m/(s N)) | |Y| (m/(s N)) |
|---|---|---|
| Peak 1 (96–101 Hz) | −26 | 0.05 |
| Anti-resonance (~115 Hz) | −68 | 4e-4 |
| Peak 2 (175–190 Hz) | −21 | 0.09 |
| Peak 3 (335–370 Hz) | −30 | 0.03 |
| 500 Hz–2 kHz, dense modes | −53 ± 8 | 0.0022 |
| 2–5 kHz | −55 … −58 | 0.0015 |

  (The Yamaha's second peak is near 210 Hz and its third is not separate.)
- **As masses for `GuitarSpec::body`** (m = Q / (2π f |Y|) at a peak) **[E]**: 98 Hz, Q 35,
  1.1 kg; 182 Hz, Q 23, 0.22 kg; 350 Hz, Q 40, 0.6 kg. The built nylon guitar has 100 Hz /
  0.9 kg / Q 25 and 200 Hz / 0.30 kg / Q 22: the steel body's low end is the same size of
  mobility. The difference in sound comes from the strings (Z₀ 1.3–1.8× larger: each string talks
  to the body more strongly) and from the body above 500 Hz.
- **Above 500 Hz:** check the nylon model's upper modes against −53 dB. The built table puts
  peaks of about −32 dB at 455 Hz and −61 dB at 3 kHz **[E]** (from Q/(2πfm)); the steel guitar
  wants a mean of about −53 dB from 500 Hz to 2 kHz with peaks of ±8 dB, falling slowly above.
  Mode density: roughly one peak per 30–60 Hz at 500–1 000 Hz in the figure **[T, read from the
  figure]**; above 1.2 kHz the admittance is better modelled statistically (Woodhouse's
  "statistical" region) **[M]**.
- **Directions:** the measurements are all normal to the top. In-plane mobility is lower; no
  steel-string number found (section 14).

### 3.4 Radiation and the soundhole

- **D-28, 1 m in front, anechoic, 0.15 N at the bridge** **[T]** [FR §9.9, Popp & Rossing 1986]:
  - 102 Hz ("air pumping") and 204 Hz radiate "quite efficiently in all directions";
  - 376 Hz is a dipole, 436 Hz a quadrupole: strong mechanical peaks that radiate little to the
    front.
  - "most of the mechanical resonances result in peaks in the radiated sound", except the
    see-saw modes.
- **Model:** as `guitar.cc`, each mode its own radiation weight (`rad`) and side difference
  (`side`). For the steel guitar: rad 0.75 for the breathing mode, 1.0 for the 190 Hz mode,
  0.15–0.25 for the 377 and 436 Hz modes (dipole, quadrupole), then random signs and falling
  weights as now **[E]**.
- **The soundhole** (100 mm diameter on a dreadnought **[M]**) radiates the breathing mode with
  the top in phase, and the other way at the 190–200 Hz mode **[T]** [FR]. A close microphone at
  the soundhole hears the 100 Hz boom 6–10 dB louder than one at the 12th fret **[L/M]**: studio
  practice puts the microphone at the 12th–14th fret, 15–30 cm away. The project's two-microphone
  `side` channel can carry this.
- **Level:** a steel-string acoustic is louder than a nylon of the same body by perhaps 3–6 dB
  at the same playing effort **[M]**; the higher tension stores more energy for the same
  displacement, and the pick drives it further.

## 4. The pick and the hand

### 4.1 The pick against the string

- **Picks:** 0.38–0.6 mm thin, 0.6–0.8 medium, 0.8–1.2 heavy, 1.5+ jazz **[M]**. Materials:
  celluloid (the classic tortoiseshell substitute), acetal (Delrin, "Tortex"), nylon,
  polycarbonate **[M]**. A thin pick is a spring in series with the string; a thick one is
  nearly rigid **[S]** [UBC].
- **Contact:** the tip's edge, about 0.5–1 mm wide on the string **[M]** (the table in
  `research/guitars.md` §2.1). The string slides along the pick's face and leaves over its edge.
- **Release:**
  - Stiff pick: the string is pushed to a displacement of 2–5 mm **[E]**, then slips off the
    edge in 0.1–0.3 ms **[M]**. A narrow, fast release puts energy up to the mode where the
    contact width is a half wavelength: 1 mm → mode ≈ 650 → past every partial in the model
    **[E]** (Fletcher's rule: a plectrum of width b excites little above mode L/b **[T]** [FR §9.3]).
  - Flexible pick: the pick bends with the string, the force rises more slowly and the string
    leaves when the pick has bent enough to let it pass; the release is still quick, but the
    peak displacement is smaller, so it is softer and a little less bright **[S/M]**. A
    secondhand report of a Vienna study: thin picks brighter, thick ones mellower and louder,
    thin picks with less initial pitch drop on the open G **[S, unverified]** [IG].
  - The pick then *rings*: a thin pick released from the string vibrates at a few hundred Hz to
    a few kHz and clicks against the next string or the air **[M]**. This is the "pick click".
- **The pick's noises** (the "attack" a listener names "pick"):
  - the tick of the edge leaving the string: broadband, 2–8 kHz, under 1 ms **[M]**;
  - on wound strings the edge scrapes across a few turns of the wrap as the string slides along
    it: a short burst of pulses at (sliding speed / wrap pitch) **[E]**, the same mechanism as
    the slide noise in §10.4;
  - on a strum the pick touches every string in turn, so each string gets both a pluck and the
    pick's tick.
- **Angle:** a pick held flat to the strings gives the cleanest tone; angled (the edge leading)
  gives more scrape **[L]**. Picking position: 100–150 mm from the saddle for strumming over the
  soundhole (the soundhole's centre lies about 165 mm from the saddle on a dreadnought **[M]**);
  near the bridge (40–60 mm) for bright, thin tone **[L]**.
- **Model** **[E]**: a new pluck kind in `Guitar::pluck` beside flesh and nail:
  - force rising over 0.5–2 ms as the pick pushes (stiffer → faster); no "plant" damping of the
    string's existing motion beyond the contact itself (a pick passes, it does not stop the
    string unless it is a rest stroke);
  - contact width 0.8 mm, a slip of 0.1–0.3 ms;
  - a click: a band-passed noise burst of 0.3–1 ms (centre 3–5 kHz) into the radiated sound,
    −30 to −20 dB under the note's peak, larger for a thin pick **[E]**; on wound strings, a
    burst of 3–8 wrap pulses at the pick's speed.
  - The direction: down pushes the string in −x along the top, and slightly into it; up the
    reverse.

### 4.2 Fingers on steel strings

- Finger-style steel players use the flesh, the nail, or thumb- and finger-picks
  (Travis picking: thumb on the bass strings alternating, fingers on the melody) **[M]**. The
  nylon model's flesh-and-nail pluck applies; the thumb-pick is a pick on the bass strings.
- The higher tension makes the same finger force move the string less; a steel-string
  finger-style note is quieter than a picked one and much darker than a picked steel note,
  but still brighter than nylon because the string keeps its highs **[E]**.

### 4.3 Strumming

- **Geometry:** string spacing at the bridge about 10.5–11 mm (54–55 mm E to E); at the
  soundhole, where the strum happens, a little less (~10 mm) **[M]**. A strum crosses five gaps:
  50–55 mm.
- **Hand speed and timing:** no measured strum timing was found (section 14). Estimates:
  - from the hand's swing: a forearm-and-wrist stroke at 1–3 m/s at the strings **[E]** gives
    3.5–10 ms per string, 18–50 ms for six;
  - the built nylon guitarist uses 1.4 m/s (7.5 ms a string) by the user's ear (`research/guitars.md`
    §13);
  - the user's MIDI files (§9): when a chord is spread, a step of 3.6–17 ms (median 8.3 ms)
    between notes **[T, measured in the files]**.
  - A pick strum is faster than a finger strum **[L]**; loud strums are faster than soft ones
    **[L]**, but the steps do not vanish.
- **Down against up:** the downstroke starts on the low E (all six or the lowest four or five in
  the chord), the upstroke catches the top three or four strings and often misses the bass **[L]**.
  A downstroke is louder and fuller, the upstroke lighter and brighter **[L]**. In 8th- and
  16th-note patterns the hand moves continuously (down on the beat, up on the off-beat) and simply
  misses the strings on rests ("ghost strokes") **[L]**: this is what the built guitarist's
  pendulum already does.
- **The pick in a strum:** the pick's angle to the strings makes it slide over each string rather
  than catch it; a strummed string is displaced less (1–2 mm) than a picked one, and released with
  more scrape **[E]**. The force per string falls along the stroke as the hand loses speed
  **[L]**.
- **What a strum does to strings already ringing:** a pick crossing a ringing string adds a new
  pluck on top of its motion; it does not stop it **[E]**. The fretting hand's release does the
  stopping (§5.3).
- **Muting strokes:** the "chuck" (the fretting hand loosens or the picking hand's palm slaps the
  strings) gives a percussive click without pitch, on the 2 and 4 of country and pop
  strumming **[L]**. GM files rarely write it (sometimes a very short chord).

### 4.4 Palm muting

- As on the electric (`research/guitars.md` §2.1): the side of the picking hand at the saddle.
  Less common on acoustic, used in rock and country rhythm **[L]**. The same lossy patch model.

## 5. The fretting hand and the noises

### 5.1 Fret buzz

- **Setup:** Martin recommends 12th-fret action (string bottom to fret top) of 0.055–0.065 in.
  (1.4–1.65 mm) on the treble side and 0.080–0.090 in. (2.0–2.3 mm) on the bass side, relief
  0.003–0.005 in. at the 5th fret, for its SC models **[S]** [Martin-SC]; a Martin gauge suggests
  3/32 in. (2.4 mm) bass and 2/32 in. (1.6 mm) treble **[S]** [Martin-gauge]. A classical guitar is
  set higher (3–4 mm bass) **[M]**.
- **When it buzzes:** a hard pick or strum with a peak displacement of 2–5 mm at the plucking
  point **[E]** against 1.5–2.4 mm of clearance at the 12th fret (less at the frets just above a
  fretted note) means the low strings touch the frets above the stopping fret on a hard attack
  **[E]**. Steel's tension (twice the nylon's) keeps the amplitude down per unit force, so a
  well-set steel guitar buzzes only when strummed hard **[E]**.
- **Sound:** a brief, bright rattle in the first 20–100 ms of a loud note, mostly on the low E and
  A; it shortens and sharpens the attack and adds energy above 2 kHz **[T, general]** [BT14,
  BRWD24]. Velocity above ~110 should start to produce it **[E]**.
- **Model:** a one-sided contact against the fret tops for the modes of the wound strings, as
  `research/guitars.md` proposes (the bass's fingerboard contact, the sitar's jawari). Cheap
  substitute: a short, velocity-gated, bandpassed burst correlated with the string's motion
  **[E]**.

### 5.2 String squeak

- **What:** a fingertip sliding along a wound string when the hand shifts position: a whistle
  or chirp whose pitch follows the sliding speed **[M]**. Very characteristic of steel-string
  recordings (and of nylon basses too, less loud) **[L]**.
- **Mechanism:** each turn of the wrap is a bump; the finger crosses them at v / p per second
  (p = the wrap wire's diameter) **[E]**, measured in kind for a slide tube by [PPV08] **[T]**,
  who model "a noise pulse train" whose rate is set by the sliding velocity, with a resonance on
  the first harmonic of the pulse rate, a waveshaper for its overtones and a fixed filter for
  the string's longitudinal modes (§10.4).
- **Numbers** **[E]**: wrap wire 0.15–0.4 mm (G string to low E); a shift of one hand position (5
  frets, 100–150 mm) in 0.1 s is 1–1.5 m/s: 3–10 kHz at the start of a fast shift, falling
  through 1–2 kHz as the finger slows. Level: 25–40 dB under the notes **[E]**; louder on a close
  microphone.
- **When:** only where the fretting hand changes position with a finger left on a wound string:
  in the guitarist model, a chord change that moves the hand more than 2 frets with a note on
  strings 4–6 at both ends **[E]**. Probability per such shift ~0.3–0.5 **[E]**; players reduce
  it with coated or flatter-wound strings.

### 5.3 Releasing a chord

- **Note-off:** the fretting fingers lift: the fretted strings are damped by the fingers in tens
  of milliseconds; open strings ring on unless the hand or palm touches them **[M]**. In GM files
  the chord's notes usually end together; the built guitarist's note-off damping (45 /s) is a
  start. Steel strings ring so long that what happens at note-off is heard much more than on
  nylon **[E]**.
- **Hammer-ons, pull-offs, slides, bends:** as `research/guitars.md` §2.2. On steel strings the
  bend is more natural (light gauges bend a whole tone on the B and G strings) and the
  hammer-on louder **[M]**.

## 6. What `guitar.cc` already has, and what changes

- **Already there, reused as is:** six strings, 48 modes in two directions, frets, the shared
  bridge on a modal body (`GuitarSpec::body`), ringing along, bends, damping, the guitarist's
  string choice, pinch and strum detector, pendulum strumming and the vibrato.
- **New in the spec:** `steelGuitar()` beside `classicalGuitar()` **[E]**:
  - scale 0.645–0.648 m, 20 frets;
  - strings from §2.1 (tensions, B with the cores), loss0 and loss1 from §2.2 (fitted), and
    without the nylon's heavy k^1.2 term: the steel loss rises much more slowly with frequency;
  - the body from §3.2/3.3 (98/1.1 kg/Q 35, 183/0.22/23, 350/0.6/40, 377 and 436 see-saw modes
    with small radiation, then ~25 modes from 450 Hz to 3.5 kHz with a mean admittance of
    −53 dB);
  - `alongDetune` as for nylon until measured.
- **New in the model:**
  - **48 modes is not enough above the low strings**: the high E's 48th partial is 15.8 kHz, but
    the low E's is 4 kHz and the A's 5.3 kHz. A steel string's partials ring at 6–10 kHz;
    64–96 modes on the wound strings, 48 on the plain **[E]**. The cost grows ~1.5×; the
    nylon was 0.7 % of a core.
  - **The pick** (§4.1) as a third pluck kind, and its click.
  - **Fret buzz** and **squeak** (§5.1, 5.2): optional, velocity- and shift-gated.
  - **String age** (§2.2).
- **New in the guitarist (`guitarinst.cc`):**
  - strum by default (§9 shows that most steel-guitar parts are rhythm chords), and pick-strum
    timing from §4.3;
  - voicings: complete six-string open and barre shapes (§9.3) when the file gives three or four
    notes, as a real player fills the chord;
  - a capo when the key asks for one (§9.3);
  - the picked single line: alternate down/up picking on runs.

## 7. What a model needs

Everything here is a proposal. Measured inputs are marked [T]; the rest is [E].

### 7.1 Strings

| | E2 | A2 | D3 | G3 | B3 | E4 |
|---|---|---|---|---|---|---|
| T (N) [T] | 111 | 129 | 133 | 134 | 104 | 104 |
| B | 1.1e-4 | 4.9e-5 | 2.1e-5 | 0.9e-5 | 6.1e-5 | 1.9e-5 |
| Modes | 96 | 80 | 64 | 64 | 48 | 48 |
| Fundamental T60 (s) | 10 | 9 | 8 | 7 | 5 | 4 |
| T60 at 3 kHz (s) | 2.5 | 2.5 | 2 | 2 | 3.5 | 3.5 |

The body's share of the losses comes from the coupling, not the table: the string's own losses
are what is left, Q ≈ 2 000–4 000 at the fundamental falling to 300–600 at 6 kHz on the wound
strings, 800–1 500 on the plain **[E]**.

### 7.2 Body

| Mode | f (Hz) | Q | Mass (kg) | rad | Note |
|---|---|---|---|---|---|
| Bending | 55 | 30 | 3 | 0 | below the strings; optional [T: f] |
| Breathing | 98 | 35 | 1.1 | 0.75 | [T: f, Q from CBWFP18; D-28 102] |
| Top (0,0) + air | 183 | 23 | 0.22 | 1.0 | [T: 175–188, Q 17–28] |
| Back (0,0) | 204 | 30 | 0.6 | 0.6 | D-28 [T: f] |
| Top transverse / see-saw | 350–377 | 40 | 0.6 | 0.25 | dipole [T: f, Q] |
| (0,1)-type | 436 | 40 | 0.8 | 0.2 | quadrupole [T: f] |
| Plates | 450–3 500 | 30–60 | 0.5–3 | falling | mean −53 dB re 1 m/(s N) |

Dreadnought by default; OM: all of the first four ×1.08, the masses ×0.85 **[E]**.

### 7.3 Pick and strum

- Pick: width 0.8 mm, slip 0.15 ms (thin 0.3 ms), force rise 1 ms, angle 0.3 rad, 120 mm from the
  saddle (bright 50 mm); the click (§4.1).
- Strum: 6–9 ms per string at mf, 4–6 at ff, 9–12 at pp; downstrokes all strings of the voicing,
  upstrokes the top 3–4; per-string force falling 10–20 % along the stroke; up 3–6 dB under down.
- Velocity: depth of push 1–4 mm, and a slightly faster release at high velocity (a firmer grip).

### 7.4 Calibration targets

From the literature there is no full steel-string pluck to fit against. `research/guitars.md`
§11.2 found unaccompanied acoustic guitar in *Fields of Gold* (120–148 s) in the user's
recordings, to be classified; Demucs' guitar stem of the strummed tracks among the user's
recordings is the other source. Fit: T60 per partial on single notes, the 100 Hz and 190 Hz body
peaks in the long-term spectrum of strumming, the strum's onset spread from the six string
onsets (where a single chord is exposed), and the pick click's level.

## 8. What a listener would notice if it were wrong

1. **Highs that die in half a second:** a nylon or a piano, not a steel-string. The 3–8 kHz
   partials must hang on for seconds after a strum.
2. **No shimmer of beating:** a strummed chord on steel strings beats slowly in its highs (stiff,
   slightly mistuned strings, two polarisations, sympathetic open strings). A dead-steady,
   exactly harmonic chord sounds like a sampler.
3. **Strums as one instant, or as a harp roll:** each string must be heard to enter, 5–10 ms
   apart, and fast enough that it is one gesture, not an arpeggio.
4. **Up and down strokes identical:** the off-beat must be lighter and higher.
5. **The boom in the wrong place:** a dreadnought's bass peaks on G2–A2 and is thin below E2;
   a low end that is flat, or peaks at 80 Hz, is the wrong instrument.
6. **No pick:** a soft finger-like attack on a strummed rock rhythm is wrong, and so is a pick
   click on every note of a finger-style ballad (most GM files don't say which, §9.4).
7. **Low E too clean:** its upper partials must be stretched (B ≈ 1e-4); the octave and a half
   above it is slightly "metallic".
8. **Chords that never stop, or stop like a gate:** the fretting hand damps fretted strings in
   ~50 ms at note-off; open strings ring through.
9. **Three-note chords on a guitar:** many GM files write triads; a real guitarist plays five or
   six strings (§9.3). A three-string strum sounds thin and keyboard-like.
10. **No squeak, ever, or squeak on every chord change.**
11. **Every strum the same:** a real hand varies 1–3 dB and a few ms; the pick's angle changes the
    brightness from stroke to stroke.

## 9. GM 26 in the user's files

Measured in the user's collections (`~/midifiles`, `~/Music/midi`: 6 080 files) with a small
parser in the scratchpad; all **[T, measured in the files]** unless marked. A "chord" is a group of
note-ons on one channel within 35 ms of the previous one.

### 9.1 How much, and what

- **1 118 distinct files** (by content; 1 199 with duplicates) use program 26, with **1.39 million
  notes**. It is the user's most common guitar by far: pop, rock, country and film themes (the
  largest parts: the Eagles' *Hotel California* and another Eagles file, the Beatles' *Mean Mr.
  Mustard*, Jethro Tull's *Aqualung*, ABBA's *Fernando*, 4 Non Blondes, Randy Travis, REM's *Man on
  the Moon*, Led Zeppelin's *Gallows Pole*, U2).
- **Track names:** most are "guitar", "acoustic guitar", "rhythm guitar", "steel guitar",
  "steel-str gt": the steel-*string* guitar. A few "steel guitar" tracks are pedal steel (§9.5).
- **Groups:** single notes 38 %, two notes 13 %, three 26 %, four 15 %, five 5 %, six 2.6 %,
  seven or more 0.1 %.
- **Range:** 1st–99th percentile 40–79 (E2–G5), median 60; 0.7 % below E2, 0.02 % above 88.
- **Velocity:** 10th / 50th / 90th percentile 60 / 91 / 121: loud, many files at fixed high
  velocities.
- **Durations:** 10th / 50th / 90th percentile 66 / 228 / 640 ms: rhythm chords.
- **Pitch bend:** in 127 of 1 118 files; where used, mostly full-range swings (bends or slides
  written as pitch bend).

### 9.2 Strums in the files

- **Two kinds of file:** of 808 files with more than 20 chords of three or more notes, 386 write
  almost every chord as one instant (under 5 % spread) and 350 spread most of them (over 50 %);
  only 72 are in between. The second kind was played on a MIDI guitar or keyboard, or humanised.
- **Spread chords:**

| Notes | Chords | Written as one instant | Spread, median / 75th / 90th percentile (ms) |
|---|---|---|---|
| 3 | 149 461 | 62 % | 15.6 / 22.3 / 28.8 |
| 4 | 85 903 | 48 % | 20.8 / 30.6 / 42.1 |
| 5 | 28 444 | 46 % | 22.1 / 34.0 / 46.2 |
| 6 | 14 788 | 51 % | 32.3 / 47.7 / 57.7 |

  - The step between successive notes: 10th / 25th / 50th / 75th / 90th percentile 3.5 / 5.2 /
    8.3 / 12.0 / 16.7 ms. A six-string strum of 32 ms (median) is 6.5 ms per string, matching the
    estimate in §4.3 and the 7.5 ms of the built nylon guitarist.
  - **Order:** of the spread chords, 53 % go strictly low to high (a downstroke), 15 % strictly
    high to low (an upstroke), 32 % mixed. (Notes that start together are sorted low first, which
    favours "low to high" a little; the mixed ones are mostly partly simultaneous chords.)
  - Velocity from the first to the last note of a spread chord: median 0, 10th/90th percentile
    −15 / +17: the files do not model the hand slowing.
- **For the guitarist** **[E]**: an exactly simultaneous chord in a file is a strum to be made
  (choose the direction from the clock, as the nylon guitarist does); a spread chord already
  carries its timing and direction, and the guitarist should keep the written order and spacing
  (perhaps lightly smoothed) rather than strum again on top of it. Detect per file: the share of
  spread chords is bimodal, so a threshold at 30 % separates them.

### 9.3 Voicings

- **The most common chord shapes** (intervals above the lowest note, counted over all files with
  duplicates): (0,4,7) 36 158; (0,5,9) 32 351; (0,3,8) 25 322; (0,7,12,16) 15 624; (0,3,7)
  13 426; (0,4,7,12) 13 152; (0,7,12) 10 500; (0,4,9) 10 218; (0,5,9,12) 7 586; (0,5,8) 6 505;
  (0,3,8,12) 6 013; (0,7,12,16,19,24) 4 714 (the full six-string E-shape barre);
  (0,7,12,16,19) 4 017.
- **Lowest note of a chord**, most common: A3 (57), G3 (55), E3 (52), D3 (50), F3 (53), B3 (59),
  G#3, F#3, then A2 (45) and C3 (48).
- **Reading** **[E]**: most chord parts are **close triads and their inversions in the D3–B3
  register**: the major triad in root position, second inversion (0,5,9) and first inversion
  (0,3,8). Those are exactly the shapes of a triad on the top three or four strings (G-B-E, D-G-B),
  but also of a keyboard player's right hand. The guitar-shaped voicings with open fifths and
  octaves ((0,7,12,16), (0,7,12,16,19,24)) are a quarter as common.
- **What the guitarist should do with a triad** (proposal, a question for the user, §12) **[E]**:
  - leave a triad on the top strings as written when it lies high (lowest note ≥ G3) or the part
    is fast;
  - otherwise "complete" it the way a strummer would: add the root (and fifth) below on the A or
    low E string, and the doublings of the open or barre shape that contains the written notes,
    preferring open shapes (E, A, D, G, C, Em, Am, Dm) in keys that allow them and barre shapes
    elsewhere. A capo is the player's answer to keys like B♭ or E♭ with open shapes.
  - The written notes must stay the top voices; added notes are quieter (the bass strings with
    less force in an up stroke, §4.3).

### 9.4 Picking and finger-style

- Single notes are 38 % of all groups. Runs of single notes in eighths or sixteenths with chord
  tones in turn are arpeggiated accompaniment (finger-picking or a pick across the strings,
  *Hotel California*'s intro style); single lines with larger steps are melodies or riffs.
  These were not separated in this count **[E]**: a detector (consecutive single notes that
  belong to one chord and overlap in time) would choose fingers or a pick.
- Notes that overlap (let ring) are the clue to arpeggios: the later note starts before the
  earlier one ends **[E]**.

### 9.5 Pedal steel in the files

- Searching track names for "steel" without "string" or "acoustic" finds the real steel parts,
  most of them country songs from one producer (TrackBust): *Beaches of Cheyenne*, *Cowboy Love*,
  *Big Ol' Truck*, *But I Will*, *Any Man of Mine*, *Don't Take the Girl*. 54 distinct tracks
  carry more than 50 pitch-bend events.
- **Program:** mostly **27 (jazz guitar)** and **94** (Pad 6 "metallic", used as a layer), a few
  26 and 28. Program 27 is GM2's home of the pedal-steel variation (§1.2) **[M]**.
- **Bend range:** "PBD=2" or "PBD=1" in the track name; 48 of the 54 at ±2 semitones (one at 4).
- **Bend gestures** (from the centre out to the extreme and back): extent 10th / 25th / 50th /
  75th / 90th percentile 50 / 100 / 194 / 200 / 200 cents. Half are a whole tone (a pedal or a
  bar slide of a tone), a quarter a semitone or less. The time to reach the extreme: 10th / 25th
  percentile 65 / 156 ms (the pedal's travel); longer values are held bends.
- **Notes:** mostly single (4 173 single onsets, 843 pairs, 216 triples): one bend moves the whole
  channel, so the sequencer writes the moving voice alone. A real pedal steel moves one voice
  inside a held chord.
- **Meaning for a model:** a pedal-steel instrument can be fed by these tracks unchanged: a held
  note bent up a tone in 65–150 ms is a pedal; a slower or partial one, the bar. Routing: a
  track named "steel" (without "string") on 26/27/94 with bends → the pedal steel, once built.
  Today these tracks play on the banjo (26) or on the jazz guitar's fallback, the koto (27).

## 10. Slide instruments: the lap steel, the Dobro and the pedal steel

### 10.1 The family

| Instrument | Strings | Scale | Tuning (low → high) | Body / pickup | Source |
|---|---|---|---|---|---|
| Lap steel (electric: Rickenbacker "frying pan", Fender) | 6 (or 8) | 22.5–24.75 in. | C6: C3 E3 G3 A3 C4 E4; also A6, E7, open G, open D | solid; one single-coil pickup | [M] |
| Dobro / resonator (square neck) | 6 | ~25 in. | open G: G2 B2 D3 G3 B3 D4 (98, 123.5, 146.8, 196, 246.9, 293.7 Hz) [T, RS19] | wooden or metal body with a spun aluminium cone | [T, RS19; Pol23] |
| Pedal steel (single neck) | 10 | 24 in. (24¼ on some) | E9 (§10.5) | solid; one or two pickups | [S, WikiPS, WikiCop] |
| Pedal steel (double neck) | 10 + 10 | 24 in. | E9 (far neck) and C6 (near neck) | | [S, WikiCop] |
| Universal | 12 | 24 in. | E9/B6 | | [S, WikiCop] |

- **Played flat**, strings up, the left hand holding the bar, the right hand with a thumb-pick and
  two or three metal finger-picks **[M]**.
- **GM:** none of these has its own GM1 program. In the user's files the pedal-steel lines are
  on GM 27 (the jazz guitar) and 94 (a metallic pad as a layer), a few on 26 and 28, named
  "steel" and full of pitch bend (§9.5) **[T, measured in the files]**.

### 10.2 The bar

- **Lap and pedal steel:** a solid chrome-plated steel bar, round ends, about 7/8–1 in. (22–25 mm)
  in diameter and 3 in. (76 mm) long, 150–250 g **[M]**. The Dobro's "Stevens"-type bar has a
  groove for the fingers and a bullet nose, ~5/8 in. thick **[M]**. "The left hand lightly
  presses the steel (a polished steel bar) on a perpendicular angle across the strings" **[S]**
  [WikiPS].
- **The contact:** the bar touches the string along a line, pressed only lightly (the strings are
  ~12 mm above the frets on a Dobro, raised by the nut; a lap steel has no frets, only markers)
  **[M]**. The string's speaking length is from the bar's contact line to the saddle.
- **Behind the bar:** the string between the bar and the nut is a second vibrating segment. It
  is excited through the bar (a compliant, light termination) and would sound at its own pitch;
  players damp it by trailing the ring and little fingers on the strings behind the bar
  **[L]**. Leaving it undamped gives a ghostly second pitch and a "clangy" tone **[E]**.
- **Termination losses:** a hard bar on a steel string is a better reflector than a finger on a
  fret; the speaking-length modes lose a little through the bar's mass into the hand **[E]**.
  Model it as the fretted end with a small loss (the `end` factor of `guitar.cc`, 1.05–1.1) plus
  coupling to the behind-bar segment that the trailing fingers damp at 100–300 /s **[E]**.
- **Pitch:** continuous, f = f₀ L / (L − x). Intonation is entirely the player's ear: every
  note is a little off and moving **[E]**.
- **Slants:** the bar angled across the strings gives different intervals on different strings:
  forward and reverse slants **[L]**. Rare in GM files; a voicing solver could use them.

### 10.3 Glides and vibrato

- **Glides** into a note from a semitone or more below are the signature; a glide of a third or
  a fifth into a chord is the "Hawaiian" cliché **[L]**. Durations 50–300 ms **[E]**.
- **Vibrato:** one pedal-steel player measured his own with a spectrum analyser: "a vibrato whose
  rate is about 6 Hz and whose extent is from 30 to 70 cents. The extent varies with the length
  of time that the note is being held. It builds from small to large" **[L]** [SGF-vib]. A
  settings guide gives 3–5 Hz and 40–70 cents for "pedal steel emulation" **[S]**. Lap steel and
  Dobro vibrato is often wider and slower, the bar rocked over ±2–5 mm **[L/E]**.
  - The vibrato is symmetric about the note (the bar moves both ways), unlike a fretted bend
    vibrato that only goes sharp **[E]**: contrast with `guitar.cc`'s vibrato.
  - It starts 150–400 ms into a held note and grows **[L]**.
- **MIDI:** pitch bend for the glides, CC1 or an automatic rule for the vibrato; a legato note
  (overlapping the previous one on the same string) as a slide to the new length rather than a
  new pluck **[E]**.

### 10.4 The bar's noise

- **Pakarinen, Puputti and Välimäki's virtual slide guitar** modelled the contact sound from
  measurements of a glass and other tubes on wound and plain strings **[T]** [PPV08]:
  - on wound strings, "when the tube slides over a single winding, it generates a short,
    exponentially decaying noise burst"; "the time interval between the noise pulses is controlled
    by the sliding velocity; a fast slide results in a temporally dense pulse train, while a slow
    slide makes the pulses appear further apart";
  - "these squeaky sounds consist mainly of lowpass-type noise with both static and time-varying
    harmonic components. The lowpass-cutoff frequency, frequencies of the time-varying harmonics,
    and the overall magnitude of the contact noise are controlled by the sliding velocity";
  - the model: a pulse train (rate = slide velocity / winding spacing), a resonator on its first
    harmonic, a tanh waveshaper for the higher ones, and a fixed 4th-order filter for "the
    static longitudinal string modes and the general spectral shape"; for plain strings, white
    noise in place of the pulses; the noise "is independent of the direction of the slide".
- **For the steels** **[E]**: pedal-steel and lap-steel players use plain steel up to .020–.022 and
  stainless or nickel windings below (§10.6), and press lightly, so the bar noise is mostly a soft
  hiss on the plain strings and a quiet buzz on the wound ones; a Dobro's phosphor-bronze strings
  and heavier bar make it plainly audible. Also: the bar's click as it lands on the strings, and
  "bar clack" against the frets on a Dobro played hard.

### 10.5 Tunings, pedals and knee levers (pedal steel)

- **E9, 10 strings** (string 1 farthest from the player) **[S]** [WikiCop, WikiPS]; octaves from
  D'Addario's E9 set's gauges and the standard layout **[S/M]**:

| String | Open | Gauge (D'Addario E9 set [S]) | Changes (Emmons-style, per [WikiCop]) |
|---|---|---|---|
| 1 | F#4 | .013 | RKL: +2 → G# (some set-ups +1 → G) |
| 2 | D#4 | .015 | RKL: +1 → E; RKR: −1 → D (half), −2 → C# (full) |
| 3 | G#4 | .011–.012 | P2 (B): +1 → A |
| 4 | E4 | .014–.015 | P3 (C): +2 → F#; LKL: +1 → F; LKR: −1 → D# |
| 5 | B3 | .017–.018 | P1 (A): +2 → C#; P3: +2 → C#; LKV: −1 → A# |
| 6 | G#3 | .020 | P2: +1 → A; RKL: −2 → F# |
| 7 | F#3 | .024w–.026w | — |
| 8 | E3 | .030w | LKL: +1 → F; LKR: −1 → D# |
| 9 | D3 | .034w | RKR: −1 → C# |
| 10 | B2 | .038w | P1: +2 → C#; LKV: −1 → A# |

  - Strings 3, 4, 5, 6, 8 and 10 make the E major chord; 1, 2, 7 and 9 add the 9th (F#), the 7th
    (D) and the major 7th (D#) **[S]** [WikiPS].
  - "Emmons" is the common pedal arrangement, "found on 9 out of 10 steel guitars" **[S]**
    [WikiCop snippet]; Day set-up swaps the strings of pedals A and B **[S]** [WikiPS].
  - String 3 (G#4, 415 Hz, on .011–.012 at 24 in.) carries 123–147 N, about 84 % of the wire's
    breaking load (147–175 N at 2.4 GPa) **[E]**: the highest-stressed string, and the B pedal
    raises it another 12 %. Hence the thinnest gauge on the instrument.
- **Typical moves** **[L/M]**: A+B pressed together gives the IV chord (E → A); A alone, from E,
  gives a sus/6 sound; B+C gives the ii minor; the famous sound is a pedal *pressed while the
  strings ring*, so one or two voices of the chord slide up a tone inside a held chord.
- **C6, 10 strings** (example copedent, string 1 first): G E C A G E C A F C **[S]** [WikiCop],
  i.e. G4 E4 C4 A3 G3 E3 C3 A2 F2 C2 **[M, octaves]**, with pedals 5–8 (P8, the "boo-wah" pedal,
  drops the low C a minor third to A) **[S]**.
- **Universal 12-string E9/B6:** E9 with two more low strings; a knee lever lowering the E's to
  D# turns it into B6 **[S]** [WikiPS].
- **Mechanism:** "Pedals connect to rods and pivot shafts … pull rods carry the motion to a
  changer finger for each string … each changer finger rotates when a pedal or lever is pressed,
  changing string tension … springs return the string to pitch on release" **[S]** [WikiPS].
  - The pitch follows the pedal's travel smoothly; a press takes 50–200 ms, slower for
    expressive moves **[E]**.
  - **Cabinet drop:** pressing a pedal pulls on the frame and lowers the other strings by a few
    cents **[L/M]**; small, but it is part of the "out of tune but sweet" sound. Model: −1 to −4
    cents on the unchanged strings per pedal pressed **[E]**.
  - A raised string's tension goes up 12 % per semitone, 26 % per tone; its B and its losses
    shift accordingly **[E]**.
- **Lap and Dobro tunings:** Dobro open G (§10.1) and open D (D2 A2 D3 F#3 A3 D4) **[M]**;
  lap steel C6 (C3 E3 G3 A3 C4 E4), A6, E13, B11 **[M]**.

### 10.6 Strings

- **Pedal steel:** plain steel .011–.020 for strings 1–6, nickel or stainless wound .024–.038 for
  7–10 **[S]** [DAD-E9, Pyramid]. Some sets use a wound .022 on string 6 to tame the B pedal
  **[S]** [SGF].
- **Lap steel (C6):** heavier than a guitar's, .015–.036 or more, so that the bar's pressure does
  not move the pitch **[M]**.
- **Dobro:** phosphor bronze .016–.056, open G **[M]**.

### 10.7 The Dobro's resonator

- **Construction:** "one or more spun metal cones embedded in the top plate, with strings driving
  the cone directly through a bridge"; single "biscuit" cone (~24 cm, bridge at the peak),
  single inverted cone (Dobro, with a spider bridge), or tricone **[T]** [RS19]. The cone
  measured by Rau and Smith is 0.35 mm aluminium; cones are "generally less than 0.5 mm" **[T]**.
- **Measured, a square-neck single inverted-cone guitar, open G** **[T]** [RS19]:
  - bridge admittance by hammer and vibrometer, fitted with 24 modes (19 linear);
  - five resonances soften with amplitude (a weakening spring): 83.2, 95.7, 128.8, 161.2,
    177.5 Hz, with slopes −8.3, −12.6, −21.4, −16.7, −0.6 Hz per (m/s) of the mode's velocity;
    no hysteresis;
  - "informal listening suggests that the linear and nonlinear models are quite similar, but do
    have a slight timbre difference, especially during the attack".
- **The cone as a speaker** **[T]** [Pol23]: the cone's lowest mode is a piston (a flexible
  bellows ring at the edge); a static stiffness of 9.8e4 N/m (bridge, biscuit and cone on a banjo
  rim, strings included) and an estimated moving mass of 0.040 kg give 249 Hz; a strong formant
  measured at 259 Hz on the cone in the banjo rim, slightly higher in the guitar; more resonances
  ("breakup") at 1 500–2 000 Hz. Aluminium is lightly damped: "more sustain and more prominent
  resonances"; "lots of power in sharply defined, long lived peaks at frequencies that are not in
  the strings' harmonic series". Most of what a listener hears comes through the body's air and
  soundholes, not the cone directly. The tailstrings (behind the bridge) add weak peaks of their
  own **[T]**.
- **Model** **[E]**: the `guitar.cc` body with a cone mode at ~260–300 Hz (mass 0.04 kg, Q
  10–20) in series with the bridge, a set of lightly damped (Q 100–300) metal modes at 1–4 kHz
  that are not harmonic, the wooden body's air mode near 100 Hz and Rau & Smith's softening on
  the low modes (optional). Then the slide bar (§10.2–10.4) and open G.

### 10.8 Pickups, amplifiers and the volume pedal

- **Pickups:** lap steels have single coils (Rickenbacker's horseshoe magnet around the strings,
  Fender's single coil) **[M]**; pedal steels have wide, high-output single coils or humbuckers
  under 10 strings, mounted near the bridge for a bright, clear sound **[M]**. The electric
  chain of `research/guitars.md` §4 (the clavinet's pickup: velocity at a point, the magnet's
  nonlinearity; L–C–R resonance) applies.
- **Amplifiers:** clean and loud with plenty of headroom: Fender Twin Reverb, Peavey Session 400
  (a 15 in. JBL, later the Peavey "Black Widow") and Nashville 400/112 are the classics **[L]**
  [SGF-amp]. Settings: treble low ("2–4" gives the warm sound), bass control a low shelf near 45 Hz
  **[L]**. Spring reverb, often a delay **[L]**.
- **Volume pedal:** "plugged in between the guitar and the amplifier … The player reduces volume
  during the attack and raises it as the note decays, extending sustain" **[S]** [WikiPS].
  Goodrich (potentiometer) and Hilton (electronic) pedals **[L]** [SGF-amp].
  - Two uses: (1) the swell, the pick's attack hidden and the note faded in over 100–400 ms
    (the "crying" steel); (2) slow compensation of the decay, holding a chord level for seconds.
  - A passive pot pedal loads the pickup more as it is turned down, losing treble **[M]**.
  - Model: a gain envelope driven by the expression controller (CC11) or by an automatic rule
    (swell on long notes, the attack pulled down by 10–20 dB) **[E]**.
- **The Dobro is acoustic** (mic'd), sometimes with a transducer under the spider **[M]**.

### 10.9 What a slide model needs (outline)

- A `Guitar` with:
  - open tunings (10 strings for the pedal steel);
  - the bar instead of frets: a continuous stopped length per string (all the strings the bar
    covers move together; slants optional), a glide time, a vibrato oscillator, and the
    behind-bar segment with its damping;
  - per-string tension changes from pedals and levers (a copedent table: which control changes
    which string by how many semitones), with travel time and cabinet drop;
  - the bar's contact noise (§10.4) driven by the bar's velocity;
  - thumb-pick and finger-picks (the pick of §4.1, a little wider);
  - the electric chain (pickup, volume pedal, clean amp, reverb) for lap and pedal steel; the cone
    body for the Dobro.
- **The player** is harder than the instrument: mapping a MIDI line to bar positions and pedal
  moves (which chord tones are reachable at one bar position with which pedals) is a search like
  the fingering solver, over a copedent instead of frets **[E]**. A first version can map any
  pitch change on a sounding string to a bar glide and leave the pedals for later.

### 10.10 What a listener would notice if it were wrong (slide)

1. Pitch steps instead of glides between notes on one string.
2. A vibrato that only goes sharp (a fretted bend) instead of both ways, or one that starts at
   full width at once.
3. No bar noise, or a hiss that does not follow the bar's speed.
4. Pedal moves as jumps; every voice of a chord moving together (the pedal moves one or two).
5. A plucked attack on every pedal-steel swell: the volume pedal hides it.
6. A Dobro without its nasal, metallic 250–300 Hz and 1.5–2 kHz formants and long inharmonic
   ring; or a Dobro with fretted (stepped) pitches.
7. Perfect intonation: a slide player is always a few cents off and correcting.

## 11. Order of work (suggestion)

1. `steelGuitar()` spec: strings, body, more modes on the wound strings. Check pitch, the T60s
   against §7.1, the 98 and 183 Hz peaks in a strum's long-term spectrum.
2. The pick and its click; strum timing for the pick.
3. The guitarist: strum by default, fuller voicings (§9.3), capo.
4. Squeak and buzz.
5. Fit against the user's recordings (§7.4).
6. Later: the slide family on the same `Guitar` with a bar, copedent and electric chain.

## 12. Open questions for the user

1. Which body: dreadnought (default here) or OM/000?
2. Pick by default for GM 26, with fingers only when the part looks finger-style (arpeggios,
   §9.4)?
3. Should the guitarist fill out three-note chords to full guitar voicings?
4. Are squeak and buzz wanted, and how audible?
5. Should "steel guitar"-named tracks with pitch bends (§9.5) be played as a pedal steel once one
   exists?

## 13. Not used here, but checked

- The [LEI] student paper cited in `research/guitars.md` §1.2 for "high E ~1.1 s, low E ~7.8 s"
  models a *solid-body electric* (an Ibanez RG570), not an acoustic **[T]**: its numbers should not
  be used for the acoustic guitars.

## 14. Not found, or uncertain

- **Measured decay times of a steel-string acoustic**, per partial: none found; §2.2 and §7.1 are
  estimates from the string's Q and the bridge mobility.
- **Phosphor bronze against 80/20**: no measurement of damping, mass or decay; only retailer and
  forum claims.
- **Core diameters** of bronze-wound strings (needed for B): not published by D'Addario.
- **An OM or 000 modal table.** Only lore for its air resonance.
- **In-plane bridge mobility** of a steel-string guitar.
- **Strum timing measured** on real players (time between strings, up against down). One
  motion-capture paper [Freire20] measured the wrist, not the strings.
- **Pick–string contact** measurements (force, release time, the pick's ring): none found for the
  guitar; the harpsichord plectrum literature [LeC13] is the nearest.
- **String squeak** measured on fretted guitars: only the slide-tube work [PPV08].
- **Slide vibrato** measured in recordings: one player's self-report only.
- **The GM2 specification** for the program-26 and -27 variations was not opened.
- **Pedal-steel pedal travel times** and cabinet drop: lore only.

## 15. References

- **[FR]** N. H. Fletcher, T. D. Rossing, *The Physics of Musical Instruments*, 2nd ed.,
  Springer 1998, ch. 9 (9.3–9.9; Table 9.1 and Figs. 9.9–9.21 from Rossing, Popov & Lehr 1985 and
  Popp & Rossing 1986). Read for these sections.
  https://csclub.uwaterloo.ca/~pbarfuss/The_Physics_of_Musical_Instruments.pdf
- **[CBWFP18]** S. Carcagno, R. Bucknall, J. Woodhouse, C. Fritz, C. J. Plack, "Effect of back wood
  choice on the perceived quality of steel-string acoustic guitars", JASA 144(6), 3533–3547
  (2018). Accepted manuscript read: Table I, Fig. 1, sections II–III.
  https://hal.sorbonne-universite.fr/hal-02025913/document ;
  https://eprints.lancs.ac.uk/id/eprint/129434
- **[FMSS14]** V. Fréour, H. Mansour, C. Saitis, G. Scavone, "Evaluation and classification of
  steel string guitars using bridge admittances", ISMA 2014, Le Mans.
  https://caml.music.mcgill.ca/lib/exe/fetch.php?media=publications:isma2014_freour.pdf
- **[W04a]** J. Woodhouse, "On the synthesis of guitar plucks", Acta Acustica united with
  Acustica 90, 928–944 (2004) (skimmed). https://euphonics.org/wp-content/uploads/2022/03/Guitar_I.pdf
- **[Euph]** J. Woodhouse, *Euphonics*, "7.2 Choosing strings" (read through a summarising fetch).
  https://euphonics.org/7-2-choosing-strings/
- **[W21]** "A necessary condition for double-decay envelopes in stringed instruments", JASA
  150(6), 4375 (2021) (snippet only).
  https://pubs.aip.org/asa/jasa/article/150/6/4375/994397/A-necessary-condition-for-double-decay-envelopes
- **[PLC14]** A. Paté, J.-L. Le Carrou, B. Fabre, "Predicting the decay time of solid body
  electric guitar tones", JASA 135(5), 3045–3055 (2014) (via `research/guitars.md`).
  https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A8_Pate_PredictingDecayTime.pdf
- **[C11]** Christian, "Quantifying multiple types of damping acting on bronze-wound guitar
  strings", MS thesis, Purdue (abstract only). https://docs.lib.purdue.edu/dissertations/AAI1502134
- **[BT14]**, **[BRWD24]**: fret and fretboard collisions; see `research/guitars.md` §12.
- **[LEI]** Davis et al., "Modelling the sustain of an electric guitar", J. Phys. Special Topics
  (Leicester), 2020. https://journals.le.ac.uk/index.php/pst/article/download/3678/3191/10415
- **[DAD]** D'Addario EJ16 product page with its tension chart (kg).
  https://integration.daddario.com/products/guitar/acoustic-guitar/phosphor-bronze/ej16-phosphor-bronze-acoustic-guitar-strings-light-12-53/item/daddario-ej16-phosphor-bronze-acoustic-guitar-strings-light-12-53-741 ;
  lb figures as copied by retailers: https://www.fiddlershop.com/products/daddario-phosphor-bronze-acoustic-guitar-strings-light ;
  D'Addario's tension catalogue (formula, 25.5 in. scale):
  https://www.daddario.com/globalassets/pdfs/accessories/tension_chart_13934.pdf
- **[DAD-EJ11]** EJ11 80/20 tensions (retailer copy): https://juststrings.com/dad_ej11.html
- **[WSH]** D'Addario phosphor-bronze unit weights converted to g/m, "Table 3 wound strings".
  https://wirestrungharp.com/material/strings/table_3_wound_strings/
- **[Martin-str]** Martin MA540 / MA140 set totals (retailer listings).
  https://nantelmusic.com/products/martin-12-54-80-20-bronze-authentic-acoustic-lifespan-2-0-guitar-strings-light-ma140t ;
  https://alamomusic.com/products/martin-ma540-light-12-54-phosphor-bronze-guitar-strings
- **[Martin-SC]** Martin SC-13E set-up and adjustment.
  https://www.martinguitar.com/on/demandware.static/-/Library-Sites-MartinSharedLibrary/default/pdf/sc13e-setup-adjustment.pdf
- **[Martin-gauge]** Martin 18TOOL27 string action gauge (retailer).
  https://www.alamomusic.com/collections/martin-guitars/products/martin-18tool27-string-action-gauge
- **[UMGF]**, **[Cream City]** phosphor bronze against 80/20, lore: https://umgf.com/pb-vs-80-20-t41847.html ;
  https://www.creamcitymusic.com/blog/phosphor-bronze-and-80-20-guitar-strings/
- **[PG]** Premier Guitar, "Acoustic Soundboard: The Shape of Things" (snippet).
  https://www.premierguitar.com/acoustic-soundboard-the-shape-of-things ;
  **[UMGF-air]** https://umgf.com/viewtopic.php?p=2452720 ; **[LF]** https://luthiersforum.com/forum/viewtopic.php?p=745331
- **[UBC]** UBC PHYS 341 student project on pick materials (snippet).
  https://wiki.ubc.ca/Courses:PHYS341/2021/Project1
- **[IG]** inside-guitar.com, secondhand report of a Vienna pick study (unverified).
  https://inside-guitar.com/?p=1198
- **[LeC13]** Le Carrou et al., harpsichord plectrum–string interaction, SMAC 2013.
  https://www.lam.jussieu.fr/Membres/Castellengo/publications/2013a_Le%20Carrou-SMAC_p.155-160.pdf
- **[Freire20]** Freire et al., strumming with an inertial sensor and a hexaphonic nylon guitar,
  Sensors 2020. https://pmc.ncbi.nlm.nih.gov/articles/PMC7583031/
- **[PPV08]** J. Pakarinen, T. Puputti, V. Välimäki, "Slide guitar synthesizer with gestural
  control", NIME 2008 (read); and "Virtual slide guitar", Computer Music Journal 32(3), 42–54
  (2008) (not read). https://nime.org/proceedings/2008/nime2008_049.pdf ;
  https://muse.jhu.edu/article/245515
- **[RS19]** M. Rau, J. O. Smith, "Measurement and modeling of a resonator guitar", ISMA 2019.
  https://ccrma.stanford.edu/~mrau/papers/ISMA2019_Rau_Smith.pdf
- **[Pol23]** D. Politzer, "An investigation of resonator guitar sound", Caltech (2023).
  https://www.its.caltech.edu/~politzer/resonator-guitar/resonator-guitar.pdf
- **[WikiPS]** Wikipedia, "Pedal steel guitar". https://en.wikipedia.org/wiki/Pedal_steel_guitar
- **[WikiCop]** Wikipedia, "Copedent". https://en.wikipedia.org/wiki/Copedent
- **[DAD-E9]**, **[Pyramid]** pedal-steel E9 string sets (retailer listings).
  https://www.visionguitar.com/products/new-daddario-eps490-pedal-steel-strings-e-9th ;
  https://www.thomann.co.uk/daddario_nyxl1238ps_e9_pedal_steel.htm ;
  https://www.thomann.co.uk/pyramid_e_9th_pedal_steel_set.htm
- **[SGF]** Steel Guitar Forum, B-pedal string 6 thread. https://be.steelguitarforum.com/viewtopic.php?p=3025081
- **[SGF-vib]** Steel Guitar Forum, vibrato rate and extent. https://be.steelguitarforum.com/viewtopic.php?p=25836
- **[SGF-amp]** Steel Guitar Forum threads on the Session 400, Nashville 400, volume pedals.
  https://be.steelguitarforum.com/viewtopic.php?p=3157719 ;
  https://be.steelguitarforum.com/viewtopic.php?p=2361913

---

# Part 3 (2026-11-20): what parts 1–2 left open, the playing culture, and the 12-string

Parts 1–2 above (sections 1–15) and `research/steel_guitar_build.md` (cited as **B** §n) stand as
they are. This part does not repeat them. It adds, section by section, what a second pass found:
the instrument's construction and woods (§16), more on strings: the medium set, measured
inharmonicity, what air drag alone does, measured whole-note decay, ageing (§17), an X-braced
modal map with its dead notes and the comparison with the classical guitar (§18), measured pluck
forces and the pluck's geometry (§19), the playing culture (§20), the words for good and bad
steel-string sound (§21), the 12-string as a second variant (§22), the model tables with every
number marked (§23), what is still not found (§24) and the new references (§25).

**Marks in this part.** The request for this pass asked for three marks: measured, derived,
guess. Parts 1–2 already use **[M]** for "general knowledge, not looked up", so the letters
would collide. Here they are spelled out:

- **[MEAS]** a measurement reported in a source that was read (the source is named; "snippet"
  is added where only a search snippet or a machine summary of the page was read, not the text).
- **[DER]** derived here by arithmetic from cited or measured values; the formula is given.
- **[GUESS]** not measured and not derivable: player or luthier lore, general knowledge from
  memory, or my estimate. "lore" is added where it is the common teaching of players or makers.

Where a number in parts 1–2 or in B is reused, its mark there is translated: [T] measured →
[MEAS], [E] → [DER] or [GUESS], [L] and [M] → [GUESS].

**Corrections and changes to parts 1–2 that this pass found:**

1. §14 says no measured decay times of a steel-string acoustic were found. One now is: the mean
   60 dB tone duration of the plucked notes on the four lower strings of an X-braced guitar
   (open to the 12th fret) is **8.9 s**, and 8.8 s on a sister guitar with a much denser,
   stiffer top **[MEAS]** [TUD20] (§17.4). It agrees with B §1.2's FluidR3 fundamentals (9–20 s
   on the wound strings).
2. §2.2 quotes Christian's finding that the decays were "primarily sensitive to the aerodynamic
   damping parameter". Read in full, that is a statement about his curve fit, not about the
   physics: he could not produce reliable coefficients **[MEAS, read]** [C11s]. Computed from
   Stokes' formula (§17.3), air drag accounts for about half the measured loss on the plain
   strings and only 10–35 % on the wound ones **[DER]**. On wound strings the wrap is the loss.
3. §2.1's wound-string B values are lower bounds, as it says. A measurement on hex-core wound
   steel strings found B from the partials "consistently greater" than B computed from the
   core **[MEAS]** [Mur22] (§17.2).
4. UNSW's guitar page gives the steel-string's air resonance as "A2 (55.0 Hz)" **[MEAS, read]**
   [UNSW]. A2 is 110 Hz; 55 Hz is A1. The note name fits parts 1–2 (96–102 Hz for a dreadnought,
   higher for small bodies), the number is a slip.
5. §3.4's soundhole diameter (100 mm, unmarked general knowledge) is supported in kind: an
   UNSW paper gives "about 95 mm" for the steel-string soundboards it measured **[MEAS]**
   [Inta].
6. B §6.2 and §8.3 put a fretting-stretch difference of 6 cents between the wound main string
   and its octave partner on courses 4–6 of the 12-string. Recomputed per course (§22.4), 4–5
   cents holds only for the low E course (main string sharper); on the A, D and G courses the
   two strings stretch nearly alike (0–2 cents, the *octave* string the sharper) **[DER, cores
   guessed]**.

---

## 16. The instrument

### 16.1 Bodies

| Body (Martin's names) | Length | Lower bout | Depth at tail | Scale | Air volume | Source |
|---|---|---|---|---|---|---|
| 0 ("parlour" size) | 485 mm (19.1 in.) | 344 mm (13.5) | 103 mm (4.06) | 24.9 in. | ~9–10 L | dims [MEAS, maker's figures via GW]; volume [GUESS] |
| 00 | 479 (18.9) | 364 (14.3) | 105 (4.13) | 24.9 | ~10–11 L | " |
| 000 / OM | 492 (19.3) | 381 (15.0) | 105 (4.1) | 000: 24.9 in. (632 mm); OM: 25.4 in. (645 mm) | ~11–13 L | " |
| Dreadnought (D-18, D-28) | 508 (20.0) | 397 (15.6) | 124 (4.8) | 25.4 | ~15–19 L | dims [MEAS via GW]; volume [DER] below |
| Jumbo (Martin J; Gibson J-200 is 17 in.) | 511 (20.1) | 406 (16.0); J-200 432 | 124 (4.8) | 25.4–25.5 | ~17–21 L | dims [MEAS via GW]; J-200 and volume [GUESS] |

- **Volume of a dreadnought, two ways** **[DER]**:
  - From the rigid-wall Helmholtz frequency of the D-28, 121 Hz **[MEAS]** [FR, §3.1 above]:
    V = S / L_eff × (c / 2πf)², with a 101.6 mm hole (S = 8.1e-3 m²), L_eff = 1.7 r + 3 mm =
    0.089 m: **V ≈ 18.5 L**. (The end correction of a hole in a compliant plate near a wall is
    uncertain by 20 %.)
  - From the outline: plan area ≈ 0.135 m² (0.508 × 0.33 m × a shape factor 0.8), mean depth
    0.111 m: **V ≈ 15 L**.
  - Use 16–17 L. The other bodies in the table are scaled by plan area × depth **[GUESS]**.
- **Shapes.** The dreadnought has a wide waist (about 273 mm against the 000's 235 mm
  **[GUESS]**): nearly a box, hence the largest air volume and plate area for its length. The
  000/OM and the parlour sizes are waisted like a classical guitar. The jumbo is the widest but
  strongly waisted. The slope-shoulder dreadnought (Gibson J-45, 24.75 in. scale) is §1.1's.
- **What the size does** (part 1 §3.2 has the lore numbers; nothing new measured):
  - smaller body → higher air mode (OM 100–115 Hz against 95–102 Hz), less output below
    120 Hz, relatively more 300–1 000 Hz: "balanced", "focused", the finger-style preference;
  - parlour: air mode around 110–125 Hz **[GUESS]**; the low E's fundamental and octave both
    fall below the body's strong range: "boxy", mid-heavy, quick;
  - jumbo: a larger plate with a tight waist: air mode 85–95 Hz **[GUESS, lore]**, loud and
    "wide" when strummed, less mid punch than a dreadnought.
- **Scale 25.4 against 24.9 in.** At the same pitch and gauge the tension goes as L²:
  24.9 in. carries 0.961 of 25.4's tension (−3.9 %); 24.75 in. (Gibson) 0.950 **[DER]**. The
  short scale bends more easily (§20.5), buzzes a little sooner, and B rises by about 8 %
  (B ∝ 1/(T L²) ∝ 1/L⁴ at fixed pitch) **[DER]**. A listener will not tell the scales apart in
  a mix; the player's feel differs.
- **Neck.** 14 frets clear of the body on dreadnoughts and OMs (12 on older and "parlour"
  designs, which moves the bridge towards the middle of the lower bout). Steel truss rod, nut
  width 43–44.5 mm (1 11/16–1 3/4 in.), fingerboard radius 16 in. (flat on a classical), 20
  frets **[GUESS, general]**; the 000's 1.75 in. nut is the finger-stylist's **[MEAS via GW]**.
  With L = 645 mm the 12th fret is 322 mm from the saddle, the 14th 287 mm, the 20th 203 mm
  **[DER]**: the fingerboard ends at the soundhole's upper edge.

### 16.2 The top, its bracing and woods

- **Thickness.** Two Adirondack-spruce OM tops from Santa Cruz Guitar Co.: 2.75 mm on the bass
  side to 2.95 mm on the treble side, and a uniform 2.9 mm; top and braces domed to a radius of
  10.06 m **[MEAS]** [RH19]. Factory tops run 2.5–3.2 mm **[GUESS]**; the built classical model
  assumes nothing about thickness, so this matters only through the mode table.
- **Variation on one instrument:** ±0.5 mm before control; ±0.05 mm measurable **[MEAS]**
  [Inta].
- **Top woods measured** (ten X-braced guitars of one design built from five grades of
  soundboard wood, a tonewood supplier's study) **[MEAS]** [TUD20 Table 1]:

| Label | ρ (kg/m³) | E along grain (GPa) | E across (GPa) | Shear G (GPa) | E_L/ρ (MPa m³/kg) |
|---|---|---|---|---|---|
| Aa (light, soft) | 363 | 12.1 | 1.42 | 0.51 | 33.3 |
| Ab | 415 | 11.4 | 1.69 | 0.68 | 27.6 |
| Bb (median) | 428 | 14.0 | 1.75 | 0.72 | 32.6 |
| Cb | 422 | 15.2 | 1.67 | 0.73 | 36.0 |
| Cc (dense, stiff) | 518 | 17.4 | 1.96 | 1.27 | 33.6 |

  - So spruce for tops spans 360–520 kg/m³ and 11–17 GPa along the grain, with the cross-grain
    modulus an eighth to a tenth of that. Reference values for Sitka: 425 kg/m³ and 11–12 GPa
    **[MEAS, snippet]** [Wiki-tw, Shepherd14].
  - **What the wood did to the sound:** listeners preferred the guitars with the light, soft
    tops; those tops put the coupled low resonances lower and gave "significantly more bass"
    below 100 Hz in strummed recordings. "No influence of the density variation of the guitar
    tops on the overall level … and the resulting overall loudness" was found **[MEAS]**
    [TUD20]. Above 1 kHz the strummed spectra "look very similar".
  - **Damping of the wood:** no number found in a source that could be read. Spruce's loss
    factor along the grain is of order 0.006–0.01 (Q 100–170) and three times that across
    **[GUESS, memory]**; cedar and redwood are said to be lower-damped than spruce **[GUESS,
    lore, snippet]**. The assembled body's Qs (17–58 in the first three peaks, part 1 §3.2) are
    what the model uses, and they include radiation.
- **Back and sides.** Rosewood (dense, ~800–900 kg/m³, low damping) or mahogany (~550, more
  damped) by tradition **[GUESS]**. The measured answer is part 1's: six guitars differing only
  in back wood were indistinguishable to players, and their low modes differed as much as the
  tops did **[MEAS]** [CBWFP18]. Model one back.
- **X-bracing.** Two braces of about 8 × 14 mm (5/16 in. wide) crossing 25–40 mm below the
  soundhole, two tone bars, finger braces, an upper transverse brace **[GUESS, general]**
  (structure in part 1 §3.1).
  - **Scalloped against straight.** Scalloping removes wood from the middle of the X arms and
    tone bars, leaving peaks. Measured on two tops through four carving stages: the modes
    "shift slightly lower in frequency as more wood is carved away", "in most cases the modal
    damping ratios decrease slightly", and the big change is the first rough carving; the free
    top shows "a few strong modes below 350 Hz, a slight lull around 400 Hz, a group of stronger
    amplitude modes around 800 Hz, and another lull around 1 kHz" **[MEAS]** [RH19].
  - So for the model: scalloped ("pre-war") = the low modes 3–6 % lower and a little lighter
    and less damped: more bass and bloom, a little less punch in the mids; straight ("1950s–
    70s", and heavier 1970s builds) = the opposite **[GUESS from RH19's direction and lore]**.
    "Forward-shifted" X (the crossing nearer the soundhole) frees more of the lower bout:
    the same direction as scalloping **[GUESS, lore]**.
- **Top, braces and bridge together weigh roughly 200 g** against a banjo head's 18 g; the
  steel-string's bridge admittance is in consequence "essentially flat" in trend and much lower
  than a banjo's **[MEAS, snippet]** [WPG21]. That is why a steel-string sustains and a banjo
  barks (`research/banjo_koto.md`).

### 16.3 Bridge, saddle, nut

- **Pin bridge.** A block of ebony or rosewood about 150 × 35 × 9 mm, 25–35 g **[GUESS]**, glued
  to the top over a hardwood bridge plate (maple or rosewood, about 2.5 mm thick, 15–20 g
  **[GUESS]**) inside. The strings pass down through the bridge, top and plate; their ball ends
  bear on the plate, held by tapered pins.
- **Against the classical's tie bridge:**
  - the string's pull is taken by the plate under the top, not by a loop over the back of the
    bridge; the bridge is glued on a wider footprint and is heavier (a classical bridge is
    15–25 g **[GUESS]**);
  - the saddle stands 2.5–4 mm above the bridge, the strings break over it at 20–35°, and the
    string's total tension of 700 N acts about 11–13 mm above the top: a torque of 8–9 N m that
    the X-brace resists **[DER from part 1's numbers]**;
  - the saddle is slanted (about 3 mm longer on the bass side) and often stepped for the B
    string: compensation for the stiff, high-tension strings. A classical saddle is straight.
- **Saddle and nut** are bone or a hard polymer; 2.4–3.2 mm thick saddle **[GUESS]**. To the
  string both ends are rigid on the scale that matters; what the model needs is the bridge
  admittance, not the saddle.
- **String spacing:** 54–57 mm E to E at the saddle (10.8–11.4 mm apart), 35–38 mm at the nut
  **[GUESS, general]**; part 1 §4.3 uses 10.5 mm at the strumming point.
- **Soundhole:** 4 in. (101.6 mm) on a Martin dreadnought **[GUESS, general]**; "about 95 mm"
  on the steel-string soundboards of [Inta] **[MEAS]**. Its centre lies about 150–165 mm from
  the saddle on a 14-fret dreadnought **[DER from the fret positions in §16.1]**: the lower edge
  at about 105–115 mm, the upper edge at about 205–215 mm.

## 17. Strings: additions

### 17.1 The medium set, and scale

D'Addario EJ17 (medium, .013–.056, phosphor bronze) at 25.5 in. **[DER]** from D'Addario's
formula T = UW (2 L f)² / 386.4 and its unit weights, which are quoted here **from memory, not
re-read** (the light-set weights from the same memory reproduce part 1's chart to 0.2 %); B as
in §2.1 with guessed cores:

| String | Item | UW (lb/in.) | T (lb) | T (N) | μ (g/m) | Z₀ (Ns/m) | B (core guessed) |
|---|---|---|---|---|---|---|---|
| E4 | PL013 | .00003744 | 27.4 | 121.8 | 0.67 | 0.285 | 2.3e-5 |
| B3 | PL017 | .00006402 | 26.3 | 116.9 | 1.14 | 0.366 | 6.9e-5 |
| G3 | PB026 | .00013640 | 35.3 | 156.9 | 2.44 | 0.618 | 0.9e-5 |
| D3 | PB035 | .00025365 | 36.8 | 163.7 | 4.53 | 0.861 | 2.3e-5 |
| A2 | PB045 | .00041751 | 34.0 | 151.3 | 7.46 | 1.062 | 5.3e-5 |
| E2 | PB056 | .00063477 | 29.0 | 129.1 | 11.34 | 1.210 | 1.2e-4 |

- Total **188.8 lb, 840 N**, 18 % above the light set (part 1 guessed 185 lb). Bluegrass
  flatpickers on dreadnoughts use mediums; most others use lights **[GUESS, lore]**.
- Mediums against lights for the model: tension and impedance +16–18 %, so at the same pick
  depth the string carries 17 % more force and energy (+1.4 dB), couples more strongly to the
  body, and buzzes later. B hardly changes.
- On a 25.4 in. scale multiply every tension by 0.992; on 24.9 in. by 0.953 **[DER]**.
- **Wound G.** Every acoustic set has a wound third string (.024–.026); a plain G (.018) is an
  electric-guitar habit. This fixes how far the G can be bent (§20.5) and makes the B the
  stiffest string of the set in B (6e-5), as §2.1 shows.
- **Alloys.** "80/20 bronze" is brass, 80 % copper and 20 % zinc; "phosphor bronze" is about
  92 % copper and 8 % tin with a trace of phosphorus **[GUESS, general; part 1 §2.1]**. Both
  wrap a hexagonal steel core. Coated strings (a polymer film over the wrap) start a little
  duller and age several times more slowly **[GUESS, lore]**.

### 17.2 Inharmonicity, measured

- **No measured B table for an acoustic bronze set was found.** The nearest:
  - An electric set, measured (Zollner 2007, quoted by a Munich study) **[MEAS]** [TUM]:

| | E2 (wound) | A2 (wound) | D3 (wound) | G3 (plain) | B3 (plain) | E4 (plain) |
|---|---|---|---|---|---|---|
| B | 1.1e-4 | 6.5e-5 | 4.5e-5 | 8.4e-5 | 2.9e-5 | 1.1e-5 |

    The low E agrees with part 1's computed 1.1e-4 for the .053. The A and D are 1.3–2× part 1's
    values; electric cores are relatively thicker, so this does not settle the acoustic set.
    "The inharmonicity of a solid (unwound) string grows with its diameter, whereas the
    inharmonicity of a wound string depends mainly on its core diameter" **[MEAS]** [TUM].
  - Plain steel: B measured from the partials agrees with B computed from diameter, tension and
    a measured Young's modulus to "one to two percentage points". Wound steel: none of the
    measured ranges overlapped the computed ones; "the line fit B being consistently greater
    than the material derived B", and less so for round cores than for hex cores (the corners
    of the hex bite into the wrap) **[MEAS]** [Mur22]. The size of the excess is only in a
    figure.
  - "For overwound strings, the effective value of EI or B cannot be reliably predicted, and
    must be measured" **[MEAS, snippet]** [WMSW12].
- **For the model:** keep the plain strings' B (E4 1.9e-5, B3 6.1e-5) **[DER, reliable]**. For
  the wound strings use part 1's values × 1.3 (E2 1.4e-4, A2 6.4e-5, D3 2.7e-5, G3 1.2e-5)
  **[GUESS: the direction is measured, the factor is not]**, and expose one knob.
- **Is it heard?** In listening tests on electric-guitar tones the slightly inharmonic sounds
  were "somewhat preferred, in particular by players"; beats from inharmonicity are "clearly
  audible" on the low wound strings **[MEAS]** [TUM]. So the low E's stretch is part of the
  sound, not a flaw to tune out.

### 17.3 What air drag alone would give

Stokes' drag on a cylinder (Fletcher & Rossing's form): τ = ρ_s / (2π ρ_a f) × 2M² / (2√2 M + 1),
M = (r/2) √(2πf/ν), ν = 1.5e-5 m²/s; T60 = 6.91 τ. For the wound strings ρ_s is the mean density
μ/(πr²) ≈ 6 800–7 100 kg/m³. All **[DER]**:

| String | T60 from air drag alone at f₀ | 1 kHz | 3 kHz | 6 kHz | 10 kHz | Measured on FluidR3 at 1 / 3 kHz (B §1.2) |
|---|---|---|---|---|---|---|
| E2 .053 w | 90 s | 29 s | 17 s | 12 s | 9.5 s | 4.6 / 1.9 s |
| A2 .042 w | 63 | 24 | 14 | 10 | 7.8 | 4.8 / 3.1 |
| D3 .032 w | 41 | 18 | 10.5 | 7.5 | 5.9 | 6.2 / 3.1 |
| G3 .024 w | 26 | 13 | 7.8 | 5.6 | 4.4 | 5.5 / 2.8 |
| B3 .016 plain | 16 | 9.0 | 5.5 | 4.0 | 3.2 | 4.9 / 2.5 |
| E4 .012 plain | 9.9 | 6.4 | 4.0 | 2.9 | 2.3 | 4.3 / 2.3 |

- **Reading.**
  - On the **plain** strings air drag is about half of the measured loss at 1–3 kHz (58 % on the
    high E at 3 kHz). The rest is the bridge (α ≈ 0.35 /s, T60 ≈ 20 s between body peaks, B
    §2.3), the steel's own loss and the finger or fret end: 1/T60 = 1/T60_air + 1/T60_bridge +
    f/(2.2 Q_material) **[DER]**.
  - As a *total* Q (= T60 × f / 2.2), FluidR3's high E has 830 at its fundamental (the body
    takes it), 1 950 at 1 kHz, 3 100 at 3 kHz and about 3 800 at 6 kHz **[DER from B §1.2]**:
    Woodhouse's "Q 3 000, typical of a steel guitar string" is this total at 2–6 kHz. The
    material's own Q must then be near 10⁴ at 3 kHz, higher than the 5 500 that Paté et al.
    fitted on an electric's string (part 1 §2.2); the two do not quite agree, and the air term
    is the less certain one (a string 20 mm from a fingerboard and next to five others is not
    a cylinder in free air).
  - On the **wound** strings air drag is 10–35 % of the measured loss. The wrap dominates:
    friction between its turns (a loss that in vacuum showed as a linear, Coulomb-type decay on
    the fundamental and first overtone only, and as plain exponential decay above
    **[MEAS]** [C11s]), and the dirt that collects there (§17.5).
  - The loss factor of "a typical steel guitar string" is "still falling" at 10 kHz, unlike
    nylon's, whose bending loss turns it up again **[MEAS, summary of the page]** [Euph]. In
    Valette's three-term form (air ∝ 1/ω, friction constant, bending ∝ stiffness × n²) the steel
    string has no visible third term below 10 kHz. That is the physical statement of "steel".
- **For the model:** `loss0 + loss1 k^1.2` cannot bend the right way for both kinds. The
  measured dB/s is close to **linear in frequency** on every string (B §1.2: a + b f, with
  b = 3.6–9.1 dB/s per kHz). A form `loss = a + b·f_k` (frequency, not mode number), with a
  per-string pair, is the simplest that fits; B §8.1's loss0/loss1 are that fit pushed through
  the existing form **[DER]**.

### 17.4 Measured whole-note decay, and where it is short

All **[MEAS]** [TUD20]; an X-braced guitar of fixed design (the report does not name it; the
group's earlier JASA paper used a Taylor 814ce, a "grand auditorium" between OM and dreadnought
— not verified for this report), strings plucked ("plugged") by hand in front of the soundhole,
microphone 2 m in front:

- **T15**, the 60 dB time extrapolated from the fall between −5 and −20 dB of the A-weighted
  level: about **10 s** for A♯2 (A string, 1st fret); **mean 8.9 s** over the notes of the four
  lower strings, open to the 12th fret (8.8 s on the guitar with the densest, stiffest top). The
  open D and G strings are the longest of all; open strings in general last markedly longer
  than fretted notes (the fretting finger is a loss).
- The B and high E strings "show much amplitude modulation" (beating of the two polarisations
  and against sympathetic strings), so much that T15 could not be fitted for the high E.
- **The level of each partial follows the body's transfer function** (force at the bridge →
  pressure in front) up to about 2 kHz; above that "the peaks and dips no longer seem to
  coincide". The A, D and G strings' partials fall off below 220–300 Hz.
- **Dead notes.** On the G string, the open G's fundamental (196 Hz) "lies below the T(1,1)₃
  coupled back resonance at 204 Hz and shows a long decay"; G♯3 (208 Hz) "coincides more with
  T(1,1)₃ and has, therefore, a dramatically short decay"; A, B♭, B, C lengthen again; C♯4–D♯4
  shorten towards the long-dipole resonance at ~320 Hz, which "lies directly between tone d♯
  (311 Hz) and e (330 Hz) and both are affected nearly equally". The same holds an octave up
  against the ~513 Hz resonance. The cross-dipole at 271 Hz, which hardly radiates, "does not
  seem to influence the decay".
- **Reading for the model.** These are the steel-string's wolf-like notes: no stutter as on a
  cello (the string is not driven), but notes whose fundamental dies in a fraction of a second
  while its overtones ring, a "thuddy" note next to a "singing" one. A model with the strings
  coupled to body modes of the right Q and mass produces them by itself; a model with a filter
  for a body does not. The check: semitone steps up the G string must show fundamental T60s
  varying by 3–5× with minima at the radiating body modes.

### 17.5 New strings, old strings

- **What ages:** the damping of the high partials, not the stiffness. "The sound of a quality
  guitar can be totally lost within a few hours of playing" **[MEAS, snippet]** [Allen76]; the
  loss factor rises with age, so the upper modes decay faster. Fletcher & Rossing: foreign
  material between the windings (part 1 §2.2). Corrosion of the bronze and loosening of the
  wrap on the core at the frets add to it **[GUESS, lore]**.
- **Only the wound strings go dead.** The plain B and E lose a little to corrosion and dirt
  but not much; an old set therefore sounds *unbalanced*: trebles still bright, basses "thuddy"
  **[GUESS, lore]**.
- **Time scale** (no measurement found; lore): the first glassy "zing" of a new set fades in
  1–3 hours of playing; 80/20 dulls in days of playing, phosphor bronze in one to three weeks;
  a set is "dead" after 1–3 months; coated strings last 3–5× longer **[GUESS, lore]**.
- **Model:** `age` in 0…1 acting on the wound strings only: b (the per-kHz loss slope) × (1 + 3
  age), a × (1 + 0.5 age) **[GUESS]**. At age 1 the low E's 3 kHz partial lasts 0.6 s instead of
  1.9 s: a "played-in" set that many recordings actually have. Default 0.15–0.25: strings a few
  days old, as on most studio recordings **[GUESS]**.

### 17.6 Polarisations, sympathetic strings, coupling at the bridge

- **Where double decays are expected on a steel-string** **[MEAS/model]** [Euph7.3]: comparing
  the loss a string suffers at the bridge with its loss to air viscosity, "strong double decays
  [are] expected near 98, 182, and 348 Hz" (the three body peaks of part 1's Fylde guitars);
  the lower strings score higher; "the top string barely produces double decays". So: two-stage
  decays on notes and partials near the body peaks, on the wound strings; plain exponential
  decay (with slow beating) elsewhere.
- **Pluck angle** on a real guitar (classical, but the mechanics are the same) **[MEAS]**
  [W04b]: plucks at 0°, 45° and 90° to the soundboard; the decay rates of the two
  polarisations differ, the dominant one changes with angle, and "the levels are extremely
  sensitive to angle near the parallel" pluck: a string plucked exactly along the top barely
  drives the body's normal motion and is quiet at first, then lasts. Unexplained split peaks of
  up to ~0.6 Hz appeared in the measured spectra where the model predicted 0.1 Hz. The beating
  between polarisations is therefore *larger* on real guitars than a clean model gives; B §8.1's
  `alongDetune` 0.0007 (0.07 %, 0.23 Hz at 330 Hz) is of the right order.
- **Sympathetic strings.** A fretted note whose partial matches a partial of an unfretted,
  undamped string sets it ringing through the bridge. With one string fingered to the pitch
  of an open one the two form a coupled pair, and plucking only one gives the "una corda"
  pattern: both the fast and the slow mode excited about equally, so the after-sound takes over
  sooner **[MEAS/model]** [Euph7.3].
  - Which notes do it in standard tuning **[DER]** (the open strings' first partials are E2: E2
    E3 B3 E4 G♯4 B4; A2: A2 A3 E4 A4 C♯5 E5; D3: D3 D4 A4 D5 F♯5; G3: G3 G4 D5 G5 B5; B3: B3 B4
    F♯5; E4: E4 E5 B5): a fretted **E** in any octave finds both E strings and the A string's
    3rd partial; an **A** the A string and the D string's 3rd; a **B** the B string and the low
    E's 3rd and 6th; a **D** the D string and the G string's 3rd; a **G** the G string. Sharps
    and flats (E♭, A♭, B♭) find almost nothing: keys like E♭ sound "drier" on a guitar than G,
    D, A, E or C, which is part of why players use a capo rather than barre chords (§20.8).
  - Level: the sympathetic string's partial reaches roughly 20–30 dB under the played one
    within 0.2–0.5 s and outlasts it after a damped or staccato note **[GUESS]**. `guitar.cc`
    has this for free (strings on one bridge); it is only heard if open strings are left
    undamped by the guitarist model.

## 18. Body: additions

### 18.1 An X-braced modal map with shapes

Roving-hammer modal analysis of one X-braced guitar (the light-top "Aa"), strings on and
damped **[MEAS]** [TUD20]. Names: T(m,n) with m across and n along the grain.

| f (Hz) | Shape | Radiation (2 m: front, 45° side, above) |
|---|---|---|
| ~100 (T(1,1)₁) | breathing: top and air | strong; weaker upward than forward: "not radiating as an omnidirectional monopole" below 200 Hz |
| 156 (T(1,1)₂) | top monopole, "strong movement of the outer edges (sides of the guitar)" | strong |
| 204 (T(1,1)₃) | back-coupled member of the triad | "comparable in all directions" |
| 271 (T(2,1)) | cross dipole | little to the front ("the average deflection of the soundboard is nearly zero"); good *upward*, towards the player's ear |
| 305–320 (T(1,2)) | long dipole | radiates well to the front (the bridge and soundhole make it asymmetric); cancels at 45° to the side |
| 353 | like the cross dipole, with more rib motion | |
| 501–513 (T(1,3)) | long tripole, little motion at the bridge | strong peak in the response, shortens notes near C5 |
| 547 (T(3,1)) | cross tripole | radiates (non-zero mean deflection) |
| 684, 736 (T(4,1)), 785, 906, 1 032 … | higher | "above 340 Hz, the directivity of the guitar increases" |

- This guitar's top monopole (156 Hz) is low against the D-28's 193 Hz and the Fylde's 175–188
  Hz: a light top on a body that was preferred for its bass. Across the ten guitars, a stiffer
  and denser top raised T(1,1)₁, T(1,1)₂ and T(1,2) but not T(1,1)₃ (the back's). The bridge-
  to-pressure curves of all ten nearly coincide below 240 Hz in shape and differ strongly
  above 640 Hz.
- **Order of the dipoles.** Here the cross dipole (271) lies below the long dipole (305); on
  the D-28 the "see-saw" is at 377 and the long-grain mode at 436 (part 1). Fletcher & Rossing's
  rule (cross-braced: (1,0) above (0,1)) is about which is which on the *free plate*, and
  authors' index orders differ; do not copy mode labels across sources, copy frequency, shape
  and radiation.
- **A maker's targets** **[GUESS, lore from a designer; snippet]** [Gore]: T(1,1)₁ ≈ 100 Hz,
  T(1,1)₂ ≈ 190 Hz, T(1,1)₃ about four semitones above T(1,1)₂, each placed *between* scale
  notes so that no note sits on a resonance; "monopole mobility" 1/√(K m) of the top between
  2 and 20 × 10⁻³ s/kg across guitars, with high values (light, loose tops) loud and
  responsive but prone to dead notes.
- **For the model:** B §2.2's table stands (98 / 183 / 204 / 350 / 436 from the Fylde and D-28
  data). The TUD map suggests two changes **[GUESS]**: give the long-dipole mode (B's #5) a
  larger frontal radiation weight than the cross dipole (0.5 against 0.15–0.2, not 0.15 for
  both), and add explicit radiating modes near 510 and 550 Hz (the first of the plate series,
  with masses that give short decays on C5–D5).

### 18.2 The three-oscillator model

- **The model** (Christensen & Vistisen's two masses, Christensen's and Richardson's third):
  the top's first mode (mass m_t, stiffness k_t, piston area A_t), the air plug in the
  soundhole (mass m_h = ρ S L_eff, area S) and the back's first mode (m_b, k_b, A_b), coupled
  through the cavity's air spring ρc²/V. "All parameters of this model can directly be derived
  from measurements of the frequencies of the two first resonances and of the cavity Helmholtz
  resonance" **[MEAS, snippet]** [CV80]. Sum rules: part 1 §3.2.
- **Numbers for a dreadnought.** No published table of m_t, A_t, m_b, A_b for a steel-string
  was reachable (the Cardiff theses that hold them refuse automated download; B §9). What can
  be said:
  - m_h = 1.2 × 8.1e-3 × 0.089 = **0.87 g**; cavity spring per unit area² ρc²/V = 1.4e5/0.0165
    = 8.6e6 Pa/m³ **[DER, §16.1's volume]**;
  - uncoupled frequencies on the D-28: Helmholtz 121 Hz, top (0,0) 163 Hz, back (0,0) 165 Hz
    (part 1 §3.1) **[MEAS]**; coupled 102 / 193 / 204 Hz **[MEAS]**;
  - the top's mass as the strings feel it at the bridge: 0.22 kg from the Fylde peak mobility
    (part 1 §3.3) **[DER]**. The piston-equivalent m_t that the three-oscillator model wants is
    smaller: 60–110 g with A_t ≈ 0.04–0.06 m² are the values usually quoted for classical
    guitars **[GUESS, memory of Christensen's and Wright's tables]**; a steel-string top is
    perhaps 1.3–1.6× heavier per area (thicker by 20–30 %, with heavier bracing and bridge)
    **[GUESS]**.
- **Why it matters little here:** `guitar.cc` uses modes at the bridge with masses from the
  measured mobility, which already contain the coupling. The three-oscillator model is useful
  for *variants*: a body volume or soundhole change moves 98 and 183 Hz in the way its two sum
  rules say (smaller V: both up; a soundhole cover: the 98 Hz peak disappears and the 183 Hz
  one falls towards 163 Hz).

### 18.3 Against the classical guitar

| | Classical (as built) | Steel-string | Mark |
|---|---|---|---|
| Total string tension | ~400 N | 714 N (light), 840 N (medium) | [MEAS] chart / [DER] |
| String impedance Z₀, low E / high E | 0.58 / 0.19 Ns/m | 1.04 / 0.24 | [DER] part 1 |
| String's own Q | hundreds, falling with f | 3 000–5 500, roughly flat | [MEAS] part 1 |
| Partials above 2 kHz last | 0.2–0.5 s | 2–3 s at 3 kHz | [MEAS on FluidR3] B §1.2 |
| Top | 2.0–2.5 mm, fan-braced, bridge 15–25 g | 2.75–2.95 mm, X-braced, bridge + plate 40–55 g | [GUESS] / [MEAS] RH19 / [GUESS] |
| First three peaks at the bridge | 100, 200, 236 Hz in `guitar.cc` | 96–102, 175–193, 204 (back) Hz | [MEAS] |
| Peak mobility at the top mode | −25 dB re 1 m/(s N) (the built table: 200 Hz, 0.30 kg, Q 22) | −21 dB | [DER] / [MEAS] part 1 §3.3 |
| Mobility 500 Hz–2 kHz | about −39 dB near 500 Hz, falling to −68 at 3 kHz in the built table | −53 ± 8 dB, flat to 2 kHz, −55…−58 at 2–5 kHz | [MEAS] CBWFP18; classical column [DER] B §2.2 |
| Excitation | flesh and nail, 1–3 ms release over 5–10 mm | pick, 0.1–0.3 ms over <1 mm | [GUESS] |
| Pluck stiffness at 130 mm from the saddle | 0.6 N/mm (T = 62 N) | 1.0–1.3 N/mm | [DER] §19.1 |

- **"Less bass fundamental per stroke?"** At equal *force* F a string stores F²/(2k) with
  k ∝ T: the steel string stores about half the energy of the nylon one (−2.5 to −3 dB)
  **[DER]**. At equal *displacement* it stores 1.7–2.1× more (+2.5 to +3 dB). A pick delivers
  2–3.5 N where a finger gives 0.5–1 N (§19.1), so in practice the steel string starts with
  5–10 dB more energy. Where it goes differs:
  - the energy in the fundamental is the same fraction of the total for the same pluck point
    and shape, but the pick's sharp corner puts relatively more into high partials than the
    finger's rounded one, and the steel string *keeps* them: the long-term spectrum tilts up;
  - the low E's fundamental (82 Hz) lies below the air mode on both instruments and is weak on
    both (B §1.3: −23 dB under its octave on a steel dreadnought). The dreadnought's bass is the
    98 Hz mode ringing on the G2–A2 region and the octaves of the lowest notes, not their
    fundamentals;
  - so: not less bass per stroke, but a smaller share of the sound, and a bass that is body
    boom plus string octave, not fundamental.
- **Brighter:** the strings (loss), the pick (release), and a bridge mobility that stays up at
  2–5 kHz. **Longer sustain:** the strings' Q, and a heavier bridge end. **Louder:** more
  tension × displacement (part 1 §3.4: 3–6 dB, unmeasured).

## 19. Excitation: additions

### 19.1 Force and displacement

- **Measured forces** (three experienced players on an electric guitar, by 240 frame/s video;
  a school-journal study, resolution 0.05 N — the only numbers found) **[MEAS, weak]**
  [Ang25]: "gentle finger plucks averaged 0.5–1.0 N, moderate pick attacks averaged 1.5–2.0 N,
  and aggressive pick strokes occasionally reached 3.2–3.5 N".
- **Displacement from force** **[DER]**: a string pulled aside at a from the saddle is a spring
  k = T (1/a + 1/(L − a)):

| Pluck point from saddle | k, plain strings (T 104 N) | k, wound (T 134 N) | Nylon (62 N) | First missing partial (open string) |
|---|---|---|---|---|
| 50 mm | 2.25 N/mm | 2.9 | 1.3 | 13th |
| 100 mm | 1.23 | 1.6 | 0.73 | 6th–7th |
| 130 mm | 1.00 | 1.3 | 0.60 | 5th |
| 160 mm | 0.86 | 1.1 | 0.51 | 4th |

  So 0.5–3.5 N is 0.4–3.5 mm at the usual 130 mm: B §8.2's depth law (0.1–2.7 mm) covers the
  measured range.
- **Longitudinal force.** On a classical guitar the transverse force at the bridge was 0–3 N for
  pluck displacements of 0–4 mm and the longitudinal (tension-modulation) force 7–10× smaller,
  growing faster than linearly **[MEAS, snippet of a thesis]** [NIU]. On steel the tension
  modulation is larger (EA/T is 140–330 against nylon's ~30–60 **[DER]**): the hard-picked
  note's pitch starts sharp and falls. For a 3 mm pluck at 130 mm the static strain is
  d²/(2a(L−a)) = 6.7e-5: a tension rise of 2.2 % on the low E (19 cents) and 0.9 % on the high
  E (8 cents) at the instant of release, about half that averaged over the first cycles, and
  falling with the square of the amplitude: **3–15 cents sharp at an ff attack**, 1–2 cents at
  mf, gone in 50–300 ms **[DER]**. Audible as the "twang" of a hard-hit low string; a
  secondhand report says thin picks give less of it (part 1 §4.1).

### 19.2 By dynamic

All **[GUESS]** except the force range **[MEAS, weak]** and what B §3.1 measured in kind
(Pluta: below a threshold depth the note is thin and noisy; above it the harmonic balance stays
and only the level grows).

| | pp | p | mf | f | ff |
|---|---|---|---|---|---|
| Force at release (N) | 0.3 | 0.6 | 1.2 | 2.2 | 3.5 |
| Depth at 130 mm (mm) | 0.3 | 0.6 | 1.1 | 2.0 | 3.0–3.5 |
| Level re mf (dB) | −12 | −6 | 0 | +5 | +9 |
| Pick grip / stiffness | loose: the pick yields, slow slip (0.3 ms), rounder corner | | medium | | firm: 0.1 ms slip |
| Brightness | thin, noisy, "brushed" | | full | same balance, louder | same + buzz + pitch sag |
| Pick click re note peak | −15 dB (relatively loud) | | −22 | | −26 |
| Fret buzz | none | | none | wound strings, first 30 ms | wound strings, 50–150 ms; slap on the low E |
| Attack pitch | — | | +1–3 cents | +3–8 | +8–15 on E2, A2 |

- A strummer's *ff* is as much hand speed and number of strings as depth per string; a
  fingerpicker's *ff* is a picker's *mf* (finger forces 0.5–1 N).
- **How dynamics change the pluck point** **[GUESS, lore]**: loud strumming drifts towards the
  bridge side of the soundhole (firmer strings, less buzz, more cut); soft playing drifts
  towards the fingerboard end ("sweeter", more fundamental). B §8.2 has 0.11–0.13 m falling to
  0.08 m on the hardest picked strokes.

### 19.3 Where, and at what angle

- **Positions on a 14-fret dreadnought** (from the saddle; soundhole from ~110 to ~210 mm,
  §16.3) **[DER/GUESS]**:

| Style | Distance | β = d/L (open) | Effect |
|---|---|---|---|
| Bridge-side lead, "bright" | 30–60 mm | 0.05–0.09 | thin, nasal, all partials to the 10th+, short |
| Flatpicked lead; palm on the bridge pins | 70–110 mm | 0.11–0.17 | the bluegrass sound: punchy, dry |
| Strumming, normal | 120–165 mm (over the hole's lower half) | 0.19–0.26 | full; 4th–5th partial weak |
| Fingerstyle | 110–150 mm | 0.17–0.23 | |
| Soft strum over the fingerboard end | 190–230 mm | 0.30–0.36 | hollow, 3rd partial weak, "harp-like" |

  - On a fretted note the string is shorter and β larger: at the 5th fret β × 1.33, at the 12th
    × 2. A chord's strings therefore each have their own comb; this is one reason a strummed
    chord has no single "formant" from the pluck point.
  - The comb: partial n ∝ sin(nπβ)/n in bridge force. Its notches are real but shallow in
    recordings: the pick's finite travel, the two polarisations and the body smear them
    **[GUESS]**. Do not make the pluck point exact to the millimetre from stroke to stroke:
    ±10 mm of scatter is what a hand does **[GUESS]**.
- **Angle to the top.** A flatpick moves nearly parallel to the top (0–20°), a little into the
  top on down strokes and out of it on up strokes (part 1 §2.3). A fingerstyle *free* stroke
  leaves at 20–45° away from the top; a thumb with a thumbpick pushes down and into the top
  (20–40°); a bluegrass *rest stroke* drives through the string into the next one, nearly
  parallel but deep **[GUESS]**.
  - More normal component → louder, punchier attack, faster first decay, more body "thump"
    (the 98 and 183 Hz modes are driven by the step in normal force) → the bass note of a
    Travis pattern. More parallel → quieter start, longer ring, more beating: the strummed
    chord's wash (the measured sensitivity near parallel is in §17.6).
- **The thump.** The release is a force step on the bridge; the body's two lowest modes ring
  at their own frequencies (98 and 183 Hz) for Q/(πf) ≈ 0.1 and 0.04 s whatever note is played.
  With the heel of the hand on the bridge or the forearm on the lower bout the player also
  knocks the top directly. On a close microphone the thump is 10–20 dB under the note at mf and
  nearly its equal on palm-muted bass notes **[GUESS]**. `guitar.cc` produces the first kind by
  itself; the second (hand on the top) is a separate small impulse into the body modes, wanted
  only for percussive styles.

### 19.4 Picks and fingers

- **The pick as a spring.** A flat-pick is a cantilever of the length that sticks out of the
  fingers (8–12 mm). Its tip stiffness goes as thickness³: taking celluloid or nylon at
  E ≈ 2–3 GPa, width 10 mm at the root, free length 10 mm: 0.5 mm → ~0.6–0.9 N/mm, 0.73 mm →
  2–3 N/mm, 1.0 mm → 5–8 N/mm, 1.5 mm → 17–25 N/mm **[DER, crude beam; the taper and the grip
  change it 2×]**. Against the string's own 1.0–1.3 N/mm (§19.1):
  - a 0.5 mm pick is *softer* than the string: it bends more than the string moves, the string
    is released with less displacement, and the pick slaps back: the papery "click-clack" of
    thin-pick strumming, bright and quiet (B §3.1: measured in kind by Carral & Paset);
  - at 0.73 mm the two are comparable; at 1 mm and above the pick is effectively rigid and the
    grip of the fingers is the compliance. Flatpickers use 1.0–1.5 mm, strummers 0.5–0.73 mm
    **[GUESS, lore]**.
- **Materials** (lore, B §3.1 has the one measurement series): celluloid (bright, a clear click),
  nylon (soft, warm, quiet click), acetal/"Tortex" (matte surface: more grip on the string,
  a slightly scratchy release), polycarbonate and casein or "tortoise" substitutes (hard,
  polished, loud and smooth) **[GUESS, lore]**. Bevelled, polished edges slide off; worn or
  rough edges scrape.
- **The pick on a wound string.** Held flat, the edge crosses the string at right angles and
  leaves in one event. Slanted 10–30° (as most players hold it, edge leading), the edge slides
  2–5 mm along the string while pushing it, across 5–25 turns of the wrap (wrap wire 0.17 mm on
  the G to 0.43 mm on the low E **[DER from §2.1's guessed cores]**): a zip of that many clicks
  in 1–3 ms, a burst at 3–10 kHz, the "scrape" or "chirp". It is strongest on slow, deliberate
  strokes with a slanted, rough pick, absent on plain strings (there the slide is a soft hiss)
  **[GUESS; mechanism as the measured squeak, §20.10]**.
- **Fingers.** Flesh: a pad 8–12 mm wide that damps the string while it loads it, release over
  1–3 ms: no partials above ~3 kHz at the start, then the string's own (still long-lived) highs
  from what was there: steel fingerstyle with bare flesh is warm but still rings. Nail: a ramp
  1–2 mm long, release 0.3–0.6 ms, closer to a soft pick. Steel wears nails; many players use
  acrylic nails or fingerpicks (metal: a hard, bright, clanky attack, a click as loud as a
  flatpick's) and a plastic thumbpick (B §4) **[GUESS, lore]**.
- **Buzz and slap at ff.** Part 1 §5.1 has the setup numbers (2.0–2.4 mm under the low E at the
  12th fret). Where the string touches **[DER, ideal string]**:
  - At release the string is a triangle with its corner at the pluck point, a from the saddle.
    Over the 12th–20th frets it is displaced by only 0.6–0.85 of the pluck's normal component,
    against 2.2–3 mm of clearance: no contact until the normal component exceeds ~3.5 mm.
  - Half a period later the triangle is mirrored: the corner, with the full displacement and
    the opposite sign, stands at a from the *nut* (or from the fretting finger): 130 mm from
    the nut is the 3rd–4th fret, where the clearance is only about 1 mm (and less just above a
    fretted note). So a string pulled *away* from the top by more than ~1–1.5 mm in the normal
    direction slaps the first frets beyond the stopping point 6 ms later (low E), and again on
    following cycles until the corner has rounded off: the buzz of 20–150 ms in §19.2.
  - A pluck parallel to the top never touches. Buzz therefore depends on the pluck's angle as
    much as on its size: a thumbpick or rest stroke with a large normal component slaps; a
    strum of the same loudness does not.

## 20. Playing culture

Measured strum timing (GuitarSet, six players) is in B §3.2 and the user's MIDI statistics in
part 1 §9; neither is repeated. Everything in this section is **[GUESS, lore]** — the common
teaching of players — unless marked.

### 20.1 Strumming

- **The pendulum.** The hand swings down on the beats (or on every eighth) and up between, and
  keeps swinging through rests; a pattern is which swings touch the strings. Common patterns in
  4/4, eighths: "D . D U . U D U" (the "folk" or "campfire" strum), "D D U U D U", all eighths
  with accents on 2 and 4; in sixteenths: "D . D U . U D U" at double speed (funk and pop).
  Measured: down strokes begin on a beat in 53–73 % of cases, up strokes in 2–29 % **[MEAS]**
  (B §3.2).
- **Accents.** Backbeat (2 and 4) louder by 3–6 dB; in country "boom-chick" the *boom* is a
  single bass note (root on 1, fifth on 3, on strings 4–6) and the *chick* a short down strum
  of the top three or four strings on 2 and 4 (the Carter style adds melody on the bass
  strings). A MIDI part that alternates one low note with a three- or four-note chord is this
  pattern and wants the bass note picked firmly and the chord strummed lightly and quickly.
- **Which strings.** Measured (B §3.2): a stroke takes 3.6–4.0 strings on average, downs start
  on the A string on average, ups sit half a string higher, full six-string strokes are rare.
  Accented downs are the full chord; unaccented ups are the top two to four strings; the
  constant-eighths "chug" between accents is often only the lowest three or four.
- **Time between strings:** 7.5 ms median (5.5 ms when all six are swept, 8.6 ms for three),
  independent of tempo, 2× between players **[MEAS]** (B §3.2). A deliberately slow "rake" or
  rolled chord (ballad endings, the first chord of a song) spreads 20–60 ms per string and is
  played with the thumb or a slow pick **[GUESS]**.
- **Mutes.** Palm mute: the heel of the hand on the saddle, damping every string to a T60 of
  0.1–0.4 s with the highs gone first; common in rock rhythm and as the "thump" of Travis
  bass. Left-hand mute ("chuck", "chick"): the fretting hand relaxes so the strings are stopped
  but not fretted, and the stroke gives a pitchless click-thud; on 2 and 4, or as sixteenth
  ghost strokes. Slap/percussive hits on the strings or top with the strumming hand on the
  backbeat (modern fingerstyle and pop). B §3.4 has a model for the chuck.

### 20.2 Flatpicking

- **Alternate picking:** down on the beat and the eighth, up on the off-sixteenths; fiddle
  tunes at 100–130 half-notes per minute are 6.7–8.7 notes per second in eighths (up to 10–12
  for the fastest players). Down strokes are a little louder and darker (the stroke goes into
  the top), up strokes lighter (B §3.3: no level measurement exists).
- **Rest strokes.** The bluegrass "power" stroke: the pick goes through the string and stops
  on the next one. It is louder (deeper), has more normal component, and it *damps the next
  string* if that was ringing. Used on bass runs and accented melody notes.
- **Crosspicking:** a rolling three-string pattern in eighths that imitates the banjo roll:
  down-down-up (the traditional way) or strict alternate picking, across strings 4-3-2 or
  3-2-1, the notes left ringing. In MIDI: steady eighths cycling over three pitches a third to
  a sixth apart with overlapping note lengths.
- **Runs and licks:** the "G run" (G2 A2 B2 D3 E3 G3 with a hammer-on), bass walks between
  chords, double stops in thirds and sixths, slides into notes from a fret below, open strings
  used as drones against fretted melody notes on the neighbouring string.
- **Where:** 70–110 mm from the saddle, the hand's heel on or behind the bridge pins; mediums,
  a heavy pick, a dreadnought: the dry, punchy sound of §19.3.

### 20.3 Fingerstyle

- Travis picking and the thumbpick: B §4. Other schools: classical-derived p-i-m-a arpeggios
  with nails ("folk fingerpicking", e.g. the 4/4 pattern thumb–index–thumb–middle), the
  clawhammer-derived "frailing" strum (the nail of the index or middle strikes down), the
  thumb-and-finger "pinch" on the beat, and modern percussive fingerstyle (tapping, body
  hits, slapped harmonics, altered tunings).
- The melody is on strings 1–3, usually louder than the accompaniment by 3–8 dB and played
  with the nail or fingerpick; the alternating bass is steady, softer, a little muted.

### 20.4 Hammer-ons, pull-offs, slides

- **Hammer-on:** the finger strikes the vibrating string onto the fret. No new pluck: the
  pitch steps up within about 5–10 ms, the level drops by 3–8 dB (energy in the cut-off part of
  the string is lost and the finger's impact adds a small thump and a click of the string on
  the fret), the highs are duller than a picked note's. A hammer-on "from nowhere" on a silent
  string gives a weak note, 15–25 dB under a picked one, with a clear fret click.
- **Pull-off:** the fretting finger plucks sideways as it leaves: a small pluck at the *fret*
  end of the new, longer string (β ≈ 0.05–0.1 from the nut side), flesh only: 4–10 dB under a
  picked note, soft attack. Pull-offs to open strings are the loudest (the open string rings
  best) and are the staple of flatpicking licks.
- **Slide** (glissando along the string with the finger down): the pitch steps fret by fret
  (each fret passes in 10–40 ms), not continuously as on a slide guitar; each fret crossing
  re-excites the string slightly and the finger rubs the wrap (squeak, §20.10). Grace slides
  from one or two frets below take 40–100 ms.
- `research/guitars.md` §2.2 has the mechanics as built for nylon; on steel all three are
  louder and clearer because the string has more energy left when the left hand acts.

### 20.5 Bends

- **How far, computed** **[DER]**: the push across the fingerboard that raises a string by a
  semitone or a tone, light gauges on 645 mm, ideal string with rigid ends (real ones need
  10–30 % more because the string slips through the nut and the neck gives):

| String | Semitone at the 12th fret | Whole tone at the 12th | Whole tone at the 5th | Force for the whole tone |
|---|---|---|---|---|
| E4 .012 plain | 13.5 mm | 19.6 mm | 17.0 mm | 16–18 N |
| B3 .016 plain | 10.1 | 14.7 | 12.8 | 12–14 |
| G3 .024 wound (core ~.0105) | 17.5 | 25.5 | 22.1 | 27–31 |
| G3 .018 plain (electric practice, for scale) | 8.0 | 11.6 | 10.1 | 7.5–8.6 |

- **Reading.** With strings 10.8–11.4 mm apart, a whole-tone bend on the B string crosses one
  and a third string spacings and needs about 13 N: possible, used by blues and country
  players, tiring. On the high E it is nearly two spacings; on the wound G it is impractical
  (2.3 spacings, 30 N). So on a steel-string acoustic:
  - **semitone bends are normal** on the B and high E, and "quarter-tone" blues inflections
    (20–50 cents) on any string;
  - **whole-tone bends** occur on the B and E strings above the 5th fret, by strong players,
    with lights; almost never on wound strings;
  - anything wider in a MIDI file (pitch-bend range set to ±2 with full swings, part 1 §9.1) was
    written for an electric guitar or is a *slide* to another fret. The guitarist model should
    render a bend of more than ~150 cents on strings 3–6, or more than ~250 cents anywhere, as
    a slide (fret steps) or by re-fretting **[GUESS]**.
- A bend takes 60–200 ms to reach pitch and is often released; pre-bends are rare on acoustic.
  Bending raises the tension by 12 % (semitone) to 26 % (tone): the bent note is slightly
  brighter and its B lower.

### 20.6 Vibrato

- No measurement of acoustic steel-string vibrato was found. Generic vibrato figures for bowed
  strings and voices (4–7 Hz, 10–60 cents) **[MEAS, snippet]** do not transfer.
- On a fretted steel string the player's vibrato is a small repeated *bend* (across the
  fingerboard), so it only raises the pitch: one-sided, 0 to +10–25 cents peak, 4–6 Hz, starting
  100–300 ms after the attack and usually only on held melody notes of a quarter-note or more
  **[GUESS, lore]**. The classical along-the-string vibrato (±5–10 cents on nylon) does almost
  nothing on steel at the low frets: the string is too stiff in tension **[GUESS]**.
- Much less than a violin (±25–50 cents continuous) and less than an electric guitar. Chords
  have none. A steel-string model with LFO vibrato of ±20 cents on every sustained note sounds
  like a synthesiser preset; the built guitarist's vibrato should be scaled to about a third of
  the nylon's depth and made one-sided **[GUESS]**.

### 20.7 Harmonics

- **Natural harmonics:** a finger touching the string lightly above the 12th, 7th or 5th fret
  (1/2, 1/3, 1/4 of the open string; also the 4th/9th fret region for 1/5, about fret 3.2 for
  1/6) while it is plucked, then lifted. Only the partials with a node there survive: the
  spectrum is the 2nd, 4th, 6th… (or 3rd, 6th…) of the open string, i.e. a note an octave
  (twelfth, two octaves) up with *the open string's long decay* and no fretting-finger loss.
  On steel they ring 5–10 s: the "chime" of intros and endings, of tuning, and of whole pieces
  in open tunings.
- Level: the 12th-fret harmonic is about as loud as a soft note; the 7th- and 5th-fret ones
  5–10 dB weaker. They are plucked near the bridge (so that the wanted partials are strong).
- **Artificial ("harp") harmonics:** the right index touches 12 frets above a fretted note
  while the thumb or another finger plucks; alternated with normal notes in "harp cascades"
  (Chet Atkins, Lenny Breau, Tommy Emmanuel). Slap harmonics: the finger slaps the strings at
  the 12th or 7th fret: a percussive chord of harmonics.
- **In the model:** a harmonic is a pluck followed within 10–30 ms by a narrow damper at the
  node (damping every mode without a node there) which then lifts. GM files do not mark
  harmonics; GM2/XG have separate "guitar harmonics" programs (32). Leave it unused unless a
  note lies exactly on a natural harmonic of an open string, is quiet, long, and above the
  12th-fret range of the part.

### 20.8 Capo, open strings, first-position voicings

- **Why the steel-string lives in first position.** Open strings ring longest (no finger at the
  far end: the open D and G are the longest notes on the instrument, §17.4), ring on after the
  hand moves, sound sympathetically, and allow hammer-ons and pull-offs to and from them. The
  "cowboy chords" put three to six open or low-fretted strings in every chord:

| Chord | Frets, strings 6→1 (x = not played, 0 = open) | Notes, low to high |
|---|---|---|
| E | 0 2 2 1 0 0 | E2 B2 E3 G♯3 B3 E4 |
| Em | 0 2 2 0 0 0 | E2 B2 E3 G3 B3 E4 |
| A | x 0 2 2 2 0 | A2 E3 A3 C♯4 E4 |
| Am | x 0 2 2 1 0 | A2 E3 A3 C4 E4 |
| D | x x 0 2 3 2 | D3 A3 D4 F♯4 |
| Dm | x x 0 2 3 1 | D3 A3 D4 F4 |
| G | 3 2 0 0 0 3 (or 3 2 0 0 3 3) | G2 B2 D3 G3 B3 G4 (or … D4 G4) |
| C | x 3 2 0 1 0 | C3 E3 G3 C4 E4 |
| F (small barre) | x x 3 2 1 1, or full barre 1 3 3 2 1 1 | F3 A3 C4 F4 |
| B7 | x 2 1 2 0 2 | B2 D♯3 A3 B3 F♯4 |
| A7, D7, E7, G7 | x02020, xx0212, 020100, 320001 | |
| Barre E-shape at fret n | n n+2 n+2 n+1 n n | intervals 0 7 12 16 19 24 (part 1 §9.3's sixth most common shape) |
| Barre A-shape at fret n | x n n+2 n+2 n+2 n | 0 7 12 16 19 |

  The intervals above the bass of these shapes ((0,7,12,16,19,24), (0,7,12,16,19),
  (0,7,12,16), (0,4,7,12,16) for C, (0,7,12,16) for D…) are how a file's chord is recognised as
  a guitar voicing rather than a keyboard triad (part 1 §9.3).
- **Capo.** A clamp across all strings at fret n makes fret n the nut. The player keeps the open
  shapes above and the music sounds n semitones higher: capo 2 with G shapes = A major; capo 3
  with C shapes = E♭; capo 5 with G shapes = C with a high, mandolin-like voicing; capo 7 with
  D shapes = A. Common positions 1–5 and 7.
  - What it does to the sound **[DER]**: the strings are shorter by 2^(−n/12): at capo 5,
    ×0.75; B rises ×2^(n/6) = 1.78; the bridge takes each partial 1.33× faster; the same
    picking position is a larger fraction of the string (β 0.20 → 0.27); the body's 98 Hz mode
    is no longer excited by fundamentals (lowest note A2 = 110 Hz at capo 5): a capoed guitar is
    brighter, thinner, "chimey", with shorter sustain and no boom.
  - "Open" strings under a capo are stopped by a fret and a rubber pad, not by a fingertip:
    they still ring nearly as long as true open strings **[GUESS]**.
  - Two guitars in a recording often play the same chords with and without a capo (G shapes
    open, and D shapes at capo 5 for C-major …) to get two registers: in MIDI, two program-26
    tracks with the same harmony an inversion apart.
  - **Inferring a capo from a file:** if the part's chords in key K are awkward barre shapes in
    first position (E♭, B♭, A♭, F♯, B, C♯m …) but become open shapes when transposed down by
    n ≤ 7 semitones, and no note in the part lies below E2 + n, assume a capo at n (part 1
    §9.3 raised this; this is the rule) **[GUESS]**.

### 20.9 Tunings

Tension changes from the light set **[DER]**: a string lowered a tone keeps 0.794 of its
tension, a semitone 0.891; B rises as 1/T.

| Tuning | Strings 6→1 | Changed strings | Total tension (light set) | Used for |
|---|---|---|---|---|
| Standard | E2 A2 D3 G3 B3 E4 | — | 714 N | |
| Drop D | **D2** A2 D3 G3 B3 E4 | 6th −2 | 691 | D-major and D-minor songs with a low D root; the low string is slack (88 N), buzzes sooner, B 1.4e-4 |
| Double drop D | **D2** A2 D3 G3 B3 **D4** | 6th, 1st −2 | 670 | Neil Young |
| DADGAD | **D2** A2 D3 G3 **A3 D4** | 6th, 2nd, 1st −2 | 648 | Celtic and modern fingerstyle: neither major nor minor, drones in D and G |
| Open G | **D2 G2** D3 G3 B3 **D4** | 6th, 5th, 1st −2 | 643 | slide and blues, Rolling Stones rhythm, slack-key; the 12-string and Dobro tuning (part 1 §10) |
| Open D | **D2** A2 D3 **F♯3 A3 D4** | 6th, 2nd, 1st −2; 3rd −1 | 634 | slide, blues, fingerstyle (Joni Mitchell) |
| Half-step down | E♭2 … E♭4 | all −1 | 636 | easier singing and bending; darker |
| Nashville ("high-strung") | E3 A3 D4 G4 B3 E4 | strings 6–3 replaced by a 12-string set's octave strings | ~520–540 | doubling a standard guitar: together they sound like a 12-string |

- **Signs in a MIDI file:** notes below E2 (40): D2 (38) present and nothing lower → drop D or
  a D tuning; C2 (36) → a C tuning or a keyboard-written part. Chords whose lowest three notes
  are D2 A2 D3 or D2 G2 D3 → DADGAD/open D or open G. Part 1 §9.1: 0.7 % of GM 26 notes lie
  below E2.
- For the model a tuning is only different open-string frequencies with tensions scaled by
  f² at fixed μ; the guitarist's fingering solver must know the tuning. Standard and drop D
  cover nearly all files **[GUESS]**.

### 20.10 Noises of the left hand

- **Squeak, measured in kind** **[MEAS, abstract summary]** [PPB07]: sliding a finger along a
  wound string gives a noise with "a harmonic structure due to the string's periodic texture"
  (the windings), and it also "excites the longitudinal modes, producing a set of static
  harmonics in the noise spectrum"; which longitudinal modes appear depends on where along the
  string the finger slides. The proportionality of the moving harmonic series to sliding speed
  could not be confirmed from the text reached; it follows from the mechanism (rate = speed ÷
  wrap pitch) and is what the slide-guitar model of the same group uses (part 1 §5.2).
- **The static part, computed** **[DER, cores guessed]**: longitudinal waves travel at
  √(E A_core / μ): about 1 940 m/s on the low E (.053), 2 020 on the A, 2 160 on the D, 2 320 on
  the G; the first longitudinal mode of the open length is 1.5, 1.6, 1.7 and 1.8 kHz (and
  proportionally higher for a shorter sounding length; on a plain steel string 5 050 m/s,
  3.9 kHz). So a squeak is a whistle sweeping down from several kHz as the finger slows,
  through fixed resonances near 1.5–1.8 kHz and their multiples: the two are what make it
  sound like a "squeak" rather than a "hiss".
- **When, and how loud** (no measurement found): on position shifts with fingers left on
  strings 3–6, on slides, and on barre-chord changes; 100–300 ms long; on a close-miked solo
  guitar it can be within 10–20 dB of the notes around it, in a band mix 30–40 dB under
  **[GUESS]** (part 1 §5.2 has the rate arithmetic and a trigger rule). New bronze strings
  squeak most; old, coated or polished ("ground-wound") strings much less.
- **Fretting and release clicks.** A finger landing on a string taps it onto the fret: a tick
  with the pitch of the two string segments, 30–45 dB under a picked note, heard in quiet
  fingerstyle. Lifting a finger off a sounding string gives a faint pull-off to the open
  string (the "ghost" open note between chords, often audible in real strumming: the open G,
  D and A sound briefly during chord changes) **[GUESS]**. The built guitarist could let the
  strings that are open in neither chord ring at a low level through a change: it is one of
  the things that make strummed parts sound human.
- **Note end.** B §3.5 has the proposal (fretted notes damped at 20 /s while strumming, 45 /s
  otherwise; open strings ring until touched). Add: the damping finger sits at the fret end,
  not in the middle, so the high partials die *last*, not first: a released fretted note
  leaves a faint, bright "zing" for 50–150 ms; and the palm, when it damps, sits at the bridge
  end and does the same from the other side **[GUESS]**.

### 20.11 From a MIDI chord to six strings

Part 1 §9.2–9.3 measured what the files contain and B §8.2 turned it into guitarist settings;
`research/guitars.md` §7.2 has the fingering solver. Rules that are specific to the steel-string
and not yet written down **[GUESS, design]**:

1. **A chord is a strum** when three or more notes start within 35 ms. If they start at one
   tick, the model supplies the spread (B §8.2: 7.5 ms a string, faster for more strings) and
   the direction from the beat (down on beats and accented notes, up between). If the file
   already spreads them by 3–17 ms a note, keep its order and timing.
2. **A spread of more than ~25 ms per note** is an arpeggio or a rolled chord: pick or finger
   each note separately, let them ring.
3. **String and fret for one note:** prefer, in this order, an open string; the lowest fret
   (0–4) on the string that the previous notes of the phrase used or its neighbour; otherwise
   the position nearest the current hand position. Unlike the jazz guitarist
   (`research/jazz_guitar.md`), who avoids open strings, the steel-string player seeks them.
   The same pitch on a lower string and higher fret is darker and shorter (thicker string,
   shorter length, larger β); use it only to keep a line on one string or when the open string
   is needed for the next note.
4. **A chord's strings:** search the open and barre shapes of §20.8 that contain the written
   notes as their top voices; if one fits with the written lowest note as its bass or with a
   root or fifth below it, play the whole shape (part 1 §9.3's "completion"), added strings at
   −4 to −8 dB. If the notes fit no shape within a four-fret span plus open strings, play them
   as written on adjacent strings.
5. **Never two notes on one string, never a stretch of more than four frets** (five above the
   7th fret), at most one barre; a note below the lowest open string means a tuning (§20.9).
6. **Capo and tuning** are decided once per track from its lowest notes and its chord shapes
   (§20.8, §20.9), not per chord.
7. **Dynamics → hand:** velocity sets pick depth per string; the chord's mean velocity sets the
   stroke speed (faster when louder) and how many strings an up stroke takes; high velocity
   moves the pick 20–30 mm towards the bridge.
8. **Note length → left hand:** notes shorter than ~80 ms in a chord part are a chuck or a
   muted stroke; notes that overlap the next onset on another string are let ring; a chord
   that ends before the next begins is damped by the fretting hand (open strings a little
   later, by the palm or not at all).

## 21. Words for the sound, and their causes

Player and reviewer vocabulary **[GUESS, lore]**, with the physical cause where one can be
given; causes are **[DER]** from this note's numbers unless marked.

| Word | What is heard | Cause in the model's terms |
|---|---|---|
| Shimmer, sparkle, "glassy" | energy at 3–10 kHz that lasts and moves | partials that ring 1–3 s at 3–6 kHz (§17.3), beating slowly: two polarisations, stretched partials of several strings a few Hz apart, sympathetic strings |
| Zing | the first seconds of a brand-new set | wound strings at age 0: wrap loss at its minimum (§17.5) |
| Chime, bell-like | long, clean highs on open strings and harmonics; a capoed or 12-string guitar | open-string decay without finger loss; inharmonic stretch small on the upper strings |
| "Piano-like" basses | wound-string notes with a clear, long, stretched overtone series and a firm attack | high Q wound strings (new), B ≈ 1e-4, a stiff top that does not swallow the fundamental quickly (no dead notes on the low strings), a hard pick or fingerpick |
| Boom, "bassy" | a dreadnought's low end, especially on G2–A2 | the 96–102 Hz breathing mode and the 175–195 Hz top mode, low and strong (light top: §16.2); soundhole towards the microphone |
| Punch, "bark", "cut" | a loud, short mid-range attack | high bridge mobility at 300–800 Hz with short decays there (strong radiating modes), pick near the bridge, mediums |
| Woody, warm, "dry" | mids with little sustained top | older strings, mahogany-type bodies (more damping), smaller bodies; fewer and shorter highs |
| Balanced | no register sticks out | OM-size body: air mode higher (100–115 Hz), less 100 Hz, more 300–1 000 Hz |
| Scooped | strong bass and treble, weak mids | rosewood dreadnought lore: low damping keeps both ends; in measurement the back wood did not matter (part 1), so this is mostly body size and bracing |
| Boxy, "honky", "cardboard" | a peak at 200–500 Hz that colours every note | body modes at 200–500 Hz too strong against the ends: small or heavily built body, close mic at the lower bout, dead strings; in a model: plate modes too light near 300–500 Hz or no radiation above 2 kHz |
| Boomy, muddy, "woofy" | 100–200 Hz hangs over | microphone at the soundhole; air and top modes with too high Q or mobility; all six strings struck on every stroke |
| Tinny, thin, brittle | highs without body | pick at the bridge, thin pick, small body, the low modes too weak; in a model: body radiation weights too small below 300 Hz, or strings radiating directly |
| Harsh, "ice-pick", clacky | 2–5 kHz attack too loud | pick click and scrape too loud, plain strings picked hard near the bridge, buzz |
| Dead, dull, thuddy | wound strings with no top, short notes | aged strings (§17.5); a note on a strongly coupled body resonance (§17.4) |
| Jangle | a bright, slightly out-of-tune, ringing strum | octave and unison pairs beating (12-string, §22), open strings, capo, bright pick |
| Sustain, "sings" | notes lasting 8–10 s | high string Q, heavy bridge end, no dead notes |
| "Quacky", plastic | the sound of an under-saddle pickup | piezo pickup: string force without the body's radiation and room; not a property of the guitar. A model must not sound like this: radiate through the body modes only |
| Compressed | loud strums do not get proportionally louder or brighter | Pluta's threshold behaviour (B §3.1), buzz limiting the amplitude, the hand |

## 22. The 12-string, as a second variant of the same model

B §6 has the first pass: the courses, D'Addario's light set EJ38 with tensions and B per
string, the fretting-stretch and inharmonicity arithmetic for the octave pairs, FluidR3's and
the GM2/GS/XG selection, and how `guitar.cc` would grow to twelve strings (B §8.3). This section
is complete enough to read alone but does not repeat B's derivations.

### 22.1 Courses, order, tuning

- **Six courses of two strings.** Standard tuning, low to high: E3·E2, A3·A2, D4·D3, G4·G3
  (octave pairs: the thin string an octave above the main one), B3·B3, E4·E4 (unisons)
  **[MEAS, read]** [Wiki-12]. Some players string the G course in unison; most keep the octave
  for its "bell-like quality".
- **The highest open string is the octave G (G4, 392 Hz)**, a minor third above the high E
  course. The tuning is therefore re-entrant: scales and chord voicings on the lower four
  courses sound simultaneously in two octaves, and a melody that moves from the G course to
  the B course drops by a seventh in its upper voice. This, as much as the beating, is the
  sound.
- **Order within a course.** Usual (Martin, Guild, Taylor, Gibson): the octave string is on the
  bass side of its main string, so a **down stroke meets the octave string first**, an up
  stroke the main string first. "Rickenbacker usually reverses this": main string first on a
  down stroke **[MEAS, read]** [Wiki-12]. Consequences **[DER/GUESS]**:
  - usual order: down strokes are brighter and more "jangly" (the pick takes the thin string
    cleanly, then is already moving when it meets the thick one), up strokes are darker and
    more fundamental; a thumb or thumbpick playing bass notes downward hits the octave string
    first and hardest: bass lines sound an octave up as much as down;
  - Rickenbacker order: the main string dominates on down strokes; the octave is a halo.
    (The Rickenbacker is an electric; for GM 26's acoustic 12-string use the usual order.)
- **Tuning down.** Before truss rods, 12-strings "were often tuned below standard pitch to
  reduce neck stress"; Lead Belly "often used low C" on 26.5–27 in. scales **[MEAS, read]**
  [Wiki-12]. Today: many players tune a semitone or a whole tone low and put a capo on the 1st
  or 2nd fret when they need concert pitch **[GUESS, lore; retailer guides]** [GC]. With the
  light set (B §6.1: 250.9 lb, 1 116 N at pitch) **[DER]**: a semitone down 994 N; a whole tone
  down 886 N: still 24 % above a light six-string. A forum figure for another maker's light
  12 set is 245.7 lb **[GUESS, snippet]**. Tuned down, the strings are slacker: more buzz,
  wider beating (§22.4), a "growly" sound; with the capo back at pitch the scale is 6 or 11 %
  shorter.

### 22.2 Strings

D'Addario EJ38, tensions **[MEAS: maker's chart via a retailer]** (B §6.1); μ = T/(2Lf)² and
Z₀ = √(Tμ) **[DER]**; B **[DER with guessed cores]** (B §6.1).

| Course | String | Gauge | Kind | f (Hz) | T (N) | μ (g/m) | Z₀ (Ns/m) | B |
|---|---|---|---|---|---|---|---|---|
| 1 | E4 | .010 | plain | 329.6 | 72.2 | 0.40 | 0.169 | 1.3e-5 |
| 1 | E4 | .010 | plain | 329.6 | 72.2 | 0.40 | 0.169 | 1.3e-5 |
| 2 | B3 | .014 | plain | 246.9 | 79.4 | 0.78 | 0.248 | 4.6e-5 |
| 2 | B3 | .014 | plain | 246.9 | 79.4 | 0.78 | 0.248 | 4.6e-5 |
| 3 | G4 (octave) | .008 | plain | 392.0 | 65.3 | 0.25 | 0.129 | 0.6e-5 |
| 3 | G3 | .023 | wound | 196.0 | 124.1 | 1.93 | 0.489 | 0.9e-5 |
| 4 | D4 (octave) | .012 | plain | 293.7 | 82.5 | 0.57 | 0.217 | 2.4e-5 |
| 4 | D3 | .030 | wound | 146.8 | 118.8 | 3.28 | 0.625 | 2.3e-5 |
| 5 | A3 (octave) | .018 | plain | 220.0 | 104.1 | 1.28 | 0.365 | 9.7e-5 |
| 5 | A2 | .039 | wound | 110.0 | 109.7 | 5.40 | 0.770 | 4.4e-5 |
| 6 | E3 (octave) | .027 | wound | 164.8 | 119.7 | 2.63 | 0.561 | 1.4e-5 |
| 6 | E2 | .047 | wound | 82.4 | 88.6 | 7.77 | 0.830 | 1.1e-4 |

- **Per course against the six-string** **[DER]**: the two strings together present Z₀ of
  0.34 (course 1) to 1.39 Ns/m (course 6), against 0.24 and 1.04: 1.3–1.4× the load on the
  bridge. At the same pick depth a course stores (T₁ + T₂)/T_six of the six-string's energy:
  ×1.39 on course 1 (+1.4 dB), ×1.88 on course 6 (+2.7 dB). A 12-string is 1.5–3 dB louder
  than the same body with six strings, not 6 dB; with the heavier top (§22.6) the gain is
  less. Players say "bigger, not louder" **[GUESS, lore]**.
- **The octave G** runs at 2.0 GPa (B §6.1), the plain .010 and .012 at 1.4 GPa: it breaks
  first and is the reason for tuning down.
- **Decay.** The octave strings are plain steel at the pitch of the six-string's upper strings:
  they keep 3–6 kHz partials for 2–3 s (§17.3's plain-string rows). The low E course therefore
  has, next to a dull wound .047, a wound .027 an octave up whose partials are twice as far
  apart and ring longer at the same frequency: the 12-string's basses are bright where the
  six-string's are thuddy.

### 22.3 Geometry and the stroke

- **Scale and neck.** Since about 1970 "most 12-strings were built to six-string dimensions and
  scale, though still with heavier bracing"; before, 26.5 in. and longer, often with a
  tailpiece **[MEAS, read]** [Wiki-12]. Scale 25.4–25.5 in. (Guild, Martin), some 24.9 in.
  **[GUESS]**. Nut 47–48 mm (1 7/8 in.), "similar in width to a classical guitar neck".
- **Spacing.** Within a course about 2.5 mm for the octave courses and 2 mm for the unisons at
  the nut; about 3 mm at the saddle "to allow for pin widths" (one builder's figures)
  **[GUESS, lore]** [LF12]. A 12-string electric saddle's total spread is 52.1 mm
  **[MEAS, a part's specification]** [SM12]; acoustic bridges 55–60 mm **[GUESS]**: courses
  10.5–11.5 mm centre to centre, so the gap between courses is 7.5–9 mm against 2.5–3 mm
  inside one. At the soundhole, where the stroke happens, the pair is 2.5–3 mm apart.
- **The second string's delay** **[DER]**: (spacing ÷ hand speed). At B §3.2's measured strum
  speeds (1.2–1.9 m/s, median 1.4): 3 mm → **1.6–2.5 ms** (2.1 ms median); for a deliberate
  single picked note the pick moves more slowly through the pair (0.5–1 m/s): 3–6 ms; for a
  fingertip that takes both strings at once, 0–1 ms. The courses follow each other at the
  six-string's 5.5–8.6 ms. So a down strum is twelve onsets in pairs: 2 ms inside a pair, 5–6 ms
  to the next pair.
  - 2 ms is below the ear's order threshold but not below its timbre: at 2 ms the two strings'
    common partials start with a relative phase of 2π f × 0.002, i.e. a comb in the *attack*
    with notches near 250, 750, 1 250 Hz for a unison pair (they fill in within tens of
    milliseconds as the strings drift). The attack of a 12-string course is softer and
    "wider" than a single string's. A model that plucks both strings at the same sample has a
    hard, doubled-sample attack.
- **Amplitudes within a course** **[GUESS; the mechanism is DER]**:
  - A pick pushed to the same depth gives each string force T (1/a + 1/(L−a)) × depth: the
    octave strings (65–120 N) get 0.5–1.35× the main string's force. In bridge-force terms the
    plain octave strings start 5.6 dB (G4), 3.2 dB (D4) and 0.5 dB (A3) under their mains, the
    wound E3 octave 2.6 dB *over* its main E2.
  - But the pick does not go equally deep: the first string of the pair gets the full depth,
    the second is caught on the way out or deeper, by ±30 %. The octave strings also sit lower
    in many nuts and saddles (their tops level with the main strings' tops, so their centres
    are lower), which makes a down stroke catch them lightly: a real, random 3–6 dB variation
    in octave-to-main balance from stroke to stroke. This variation is audible as "life".
  - Fingers: the flesh takes both strings; the nail often catches only the nearer one. Thumb
    (downward) favours the octave string, fingers (upward) the main string: in fingerstyle the
    bass has more octave, the treble pattern more main **[GUESS]**.
- **Bends and vibrato:** two strings under one fingertip go sharp in proportion to their EA/T
  (§22.4's list): on the low E course the main string rises 3.6× as fast as its octave, on the
  A and D courses the octave string 1.3–1.5× as fast as the main; only the G course and the
  unisons stay together. A bend therefore *de-tunes the pair*, and it takes twice the force:
  bends are avoided or kept under a quarter-tone; vibrato is rare. Hammer-ons and pull-offs
  work but are weaker.

### 22.4 Detuning, beats, and the bridge

- **No measurement of course detuning on 12-string guitars was found** (B §6.2 and §9). What is
  available is a worked model of exactly this case and general tuning knowledge.
- **The model** **[MEAS/model]** [Euph7.3]: "12-string guitar, second course": two steel
  strings 0.45 mm in diameter, 650 mm long, near 250 Hz, string Q 3 000, coupled at the bridge
  to one body mode of effective mass 200 g and Q 50, mistuned over ±8 cents, with the body
  mode placed below, on, and above the strings' pitch:
  - at exact unison the pair has a **symmetric** mode (strings together, force on the bridge
    doubled, loses energy to the body, short-lived) and an **antisymmetric** mode (no net
    force, keeps the string's Q of 3 000, nearly silent);
  - "the coupling of the two strings through the bridge means that there is a minimum
    separation of the two frequencies": the pair *cannot* be tuned to an exact unison; the
    separation is largest when a body resonance lies near the note; this veering "always comes
    with beats in the sound";
  - driving both strings (the normal 12-string stroke) mainly excites the fast, loud symmetric
    mode; driving one ("una corda") excites both about equally, 6 dB lower at the start, and
    the slow after-sound takes over sooner;
  - with mistuning, each mode becomes mostly one string; the two-stage decay turns into beats;
  - doubling the strings raises the loss to the body by 6 dB against the single string, so
    "12-string guitar: all curves raised by 6 dB": double decays are stronger than on the
    six-string near 98, 182 and 348 Hz, and the first stage is twice as fast.
  - For the piano (triple strings on a resistive soundboard) the same page finds pitch changes
    audible at about 2 cents and beating obvious only at 5 cents; on the guitar's reactive,
    resonant bridge beats appear sooner.
- **Detuning in practice** **[GUESS; arithmetic DER]**:
  - *By the tuner.* An electronic tuner leaves ±1–2 cents; by ear, unisons are tuned beat-free
    to within ~1 cent and octaves to within 1–3 cents. Some players deliberately leave one
    string of a unison 2–5 cents flat for more chorus (the studio trick of a 5–10 cent detuned
    double imitates this, B §6.2); others call that out of tune. A fair default: unisons
    ±1.5 cents (s.d.), octaves ±2.5 cents, fixed per performance.
  - *Drift.* The thin plain strings warm up and go flat faster than the wound ones under the
    hand, and the .008 stretches: a pair wanders by 2–5 cents within a song **[GUESS, lore]**.
  - *Fretting.* Pressing a string to the fret stretches it; the pitch rises by about
    865 × (EA/T) × strain cents, with a strain near 2e-5 at the first frets for 1 mm of action
    (B §6.2). EA/T with guessed cores **[DER]**: E2 370, E3 (wound .027) 100; A2 210, A3 320;
    D3 130, D4 180; G3 80, G4 100; unisons equal. So at the first frets: E2 +6.4 cents against
    E3 +1.8 (**4.6 cents apart, the main string sharp**); A2 +3.6 against A3 +5.5 (1.9, the
    octave sharp); D3 +2.3 against D4 +3.1 (0.8); G3 +1.4 against G4 +1.7 (0.3). This corrects
    B §6.2, which took the low E's figure for all octave courses. All shrink up the neck.
  - *The saddle* adds another 1–5 cents (§22.5).
  - *Beat rates* **[DER]**: a unison pair d cents apart beats at f × d × 5.78e-4 at partial
    frequency f: B3 course, 2 cents: 0.29 Hz at the fundamental, 1.4 Hz at its 5th partial
    (1.2 kHz), 2.9 Hz at the 10th. E4 course, 3 cents: 0.57 Hz at 330 Hz, 5.7 Hz at 3.3 kHz. An
    octave pair beats between the main string's even partials and the octave string's
    partials, at the same rule plus the inharmonicity term of B §6.2 (−1.7 cents at k = 5,
    −7 cents at k = 10 on the A course): slow at the bottom, 3–10 Hz at 2–4 kHz.
  - So the **shimmer is a set of amplitude modulations whose rate rises with frequency** —
    slow swells in the fundamentals, a flutter in the highs — different on every course and
    every fret, and never periodic as a chorus pedal's single LFO is.
- **Coupling in the model.** `guitar.cc`'s strings already meet at one bridge on shared body
  modes, so the veering, the symmetric/antisymmetric split, the double decay and the minimum
  separation come out of the existing equations once two strings of a course are present. What
  must be put in by hand is the mistuning and the stroke's delay and amplitude differences.
  The octave pair couples the same way wherever partials coincide (every partial of the octave
  string with an even partial of the main).

### 22.5 The octave string's intonation

- **Why it is off** **[DER]**: a string frets sharp by an amount that grows with its stiffness
  and the stretch of pressing it down, and the saddle is set back to compensate: about 4–6 mm
  for a wound low E, 1.5–2.5 mm for a plain .012 **[GUESS, luthier practice]**. In a course the
  wound main and its plain octave want different set-backs (by 1.5–3 mm) but on most
  12-strings share one straight or simply slanted saddle. With the pair tuned true on the open
  strings, a set-back error δ makes a fretted note off by 1731 δ (1/ℓ − 1/L) cents (ℓ the
  fretted length): for δ = 2 mm, **1 cent at the 3rd fret, 1.8 at the 5th, 5.4 at the 12th**.
  The octave string is the sharp one when the saddle is set for it and the main string flat
  when set for the main; makers split the difference.
- Add the fretting-stretch difference (§22.4: 4.6 cents on the low E course at the first
  frets, 0–2 cents on the others, shrinking up the neck). Net: octave pairs are 1–6 cents
  apart on fretted notes even when the open strings are perfect, most on the low E course near
  the nut and on every octave course high up the neck **[DER, order of magnitude]**. Modern
  instruments with individually compensated 12-string saddles reduce the saddle part
  **[GUESS]**.
- The nut adds its own error on the first frets (octave-string slots too high) **[GUESS]**.
- **Model:** per octave course, the octave string's offset against its main in cents at a
  fret = c₀ + c₁ × (1/ℓ − 1/L) + c₂ × (1 − fret/12), with c₀ the open-string mistuning
  (§22.4), c₁ = 1731 × δ with δ drawn once per course from ±2 mm, and c₂ = −4.6 (E course),
  +1.9 (A), +0.8 (D), +0.3 (G) cents. Unison courses: c₁ = c₂ = 0.

### 22.6 Body

- **Build.** "The neck, body, and bracing are reinforced"; bridges "often had larger
  reinforcement plates" **[MEAS, read]** [Wiki-12]. Typically a dreadnought or jumbo body
  (Martin D12-28, Guild F-212/F-512, Taylor 150e/355, Gibson B-45-12) with a thicker top,
  taller or unscalloped X-braces, a wider bridge with two rows of six pins and a larger bridge
  plate, sometimes a 12-fret neck join **[GUESS, general]**.
- **No modal measurement of a 12-string was found** (B §9). Expectation **[GUESS]**: bridge and
  plate 15–25 g heavier, top 10–20 % stiffer and heavier: the first three peaks at about the
  same frequencies (a jumbo body: 5–8 % lower), mobility at the peaks 2 dB lower, and above
  500 Hz 2 dB lower. B §6.4 proposes masses × 1.25 at unchanged frequencies. Together with
  +1.5–2.7 dB of string energy per course (§22.2) the 12-string is about as loud per stroke as
  the six-string, with longer first-stage decays than a six-string strung the same.
- The string load on the bridge is the reason: 1 116 N at 12 mm above the top is a torque of
  13 N m against 8.6 N m (§16.3) **[DER]**.

### 22.7 The sound, and how it is played

- **Jangle and shimmer, causes in order of weight** **[GUESS, ranked by the arithmetic
  above]**: (1) the octave strings: every bass-course note has a bright plain string at 2f, so
  the spectrum of the lower four courses gains 3–8 dB at 1–5 kHz and the pitch is doubled;
  (2) frequency-dependent beating within every course (§22.4); (3) the 2 ms pair delay
  softening and widening each attack (§22.3); (4) twice as many sympathetic strings;
  (5) the re-entrant G4. Pete Seeger: "the clanging of bells" **[MEAS, read]** [Wiki-12].
- **Styles** **[MEAS, read]** [Wiki-12] and **[GUESS, general]**: "mostly used for
  accompaniment, because the extra strings make fretting, bending, and soloing harder":
  - strummed open chords in first position, often with a capo (folk, folk-rock, 1970s–80s
    rock ballad intros and acoustic layers under electric guitars);
  - ringing arpeggios across a held chord (the picked intros named in B §6.3);
  - blues and ragtime fingerstyle in low tunings with a thumbpick (Lead Belly, Blind Willie
    McTell), later Leo Kottke's fast fingerstyle and slide in open tunings, tuned 2–4 semitones
    low on heavy strings;
  - rarely single-note leads; rarely barre chords high up the neck (intonation, §22.5).
- **For the guitarist model:** the six-string's player with: strums favoured over picking;
  no bends over 50 cents and no vibrato; first-position and capo voicings always; pick position
  as the six-string; arpeggios let ring; a slightly slower strum (heavier feel: hand speed
  × 0.9) **[GUESS]**.

### 22.8 What a listener would notice if it were wrong (12-string)

1. **A chorus effect instead of a 12-string**: one beat rate for everything. Real beating
   differs per course and rises with frequency.
2. **Octaves on the top two courses**, or a unison G: shrill and organ-like; or all unisons:
   a mandolin-like guitar without the 12-string's doubled bass.
3. **Both strings of a pair starting on the same sample, at the same level, every time**: a
   hard, phasey, static attack. Needs ~2 ms between them, order by stroke direction, and
   3–6 dB of random balance.
4. **Up and down strokes sounding alike**: on a real one the down stroke is jangly (octave
   first) and the up stroke rounder.
5. **Perfectly tuned octaves on every fret**: too clean, "synth octaver". Fretted notes need
   their few cents of error; open strings are the cleanest.
6. **Octave strings that die as fast as wound ones**, or wound mains as bright as the octaves.
7. **Whole-tone bends and wide vibrato**: nobody does that on a 12-string.
8. **Twice the loudness**: it is 1–3 dB up, and denser, not 6 dB.
9. **Too much bass**: the 12-string's low end is *lighter* than a six-string dreadnought's
   (lighter low strings: 88.6 N on a .047 against 111 N on a .053, a stiffer top) **[GUESS]**.

## 23. What a model needs: tables

Marks as in this part: [MEAS] / [DER] / [GUESS]. "B §n" = `steel_guitar_build.md`.

### 23.1 Six-string, per string (light set .012–.053 on 645–648 mm, new strings)

| | E2 | A2 | D3 | G3 | B3 | E4 | Mark, source |
|---|---|---|---|---|---|---|---|
| Gauge (in.) | .053 | .042 | .032 | .024 | .016 | .012 | [MEAS] maker |
| Kind | wound PB/hex steel | wound | wound | wound | plain | plain | [MEAS] maker |
| Tension (N) | 111.0 | 128.7 | 133.1 | 133.7 | 103.7 | 103.9 | [MEAS] maker's chart, part 1 §2.1 |
| μ (g/m) | 9.74 | 6.34 | 3.68 | 2.07 | 1.01 | 0.57 | [DER] T/(2Lf)² |
| Z₀ (Ns/m) | 1.040 | 0.903 | 0.700 | 0.527 | 0.324 | 0.243 | [DER] |
| B, computed | 1.1e-4 | 4.9e-5 | 2.1e-5 | 0.9e-5 | 6.1e-5 | 1.9e-5 | [DER]; wound: guessed cores, lower bound |
| B, to use | 1.4e-4 | 6.4e-5 | 2.7e-5 | 1.2e-5 | 6.1e-5 | 1.9e-5 | wound × 1.3 [GUESS], §17.2 |
| T60 of the fundamental (s) | 20 | 8.6 | 12 | 10 | 10 | 5.5 | [MEAS on FluidR3's sampled guitar] B §1.2 |
| T60 at 1 kHz (s) | 4.6 | 4.8 | 6.2 | 5.5 | 4.9 | 4.3 | [MEAS on FluidR3] |
| T60 at 2 kHz | 2.7 | 3.7 | 4.1 | 3.8 | 3.3 | 3.0 | [MEAS on FluidR3]; E2 beyond its fit range (1.6 kHz) |
| T60 at 4 kHz | 1.5 | 2.6 | 2.5 | 2.3 | 2.0 | 1.9 | [MEAS on FluidR3]; E2, A2 extrapolated |
| T60 at 6 kHz | 1.0 | 2.0 | 1.8 | 1.6 | 1.4 | 1.4 | [DER] extrapolated from the fits |
| T60 from air drag alone, 3 kHz | 17 | 14 | 10.5 | 7.8 | 5.5 | 4.0 | [DER] §17.3 |
| Whole-note T60 (A-weighted, −5…−20 dB) | mean 8.9 s over E2–G3 strings, open to fret 12; open D, G longest | | | | modulated | not fittable | [MEAS] TUD20 |
| Old strings: slope b × | 4 | 4 | 4 | 4 | 1.2 | 1.2 | [GUESS] §17.5 |
| Modes | 96 | 96 | 80 | 64 | 48 | 48 | [GUESS] B §8.1 / part 1 §7.1 |
| Longitudinal mode 1, open (kHz) | 1.5 | 1.6 | 1.7 | 1.8 | 3.9 | 3.9 | [DER] §20.10; for squeak only |

Caveats: FluidR3's recordings are of an unknown guitar with strings of unknown age through an
unknown microphone; they are the only per-partial steel-string decays in hand. Medium set:
§17.1 (tensions × 1.16–1.18). Variants of scale: §16.1.

### 23.2 Twelve-string, per string (light set, standard pitch)

T, μ, Z₀, B: §22.2's table ([MEAS] tensions, the rest [DER]). The rest:

| | Octave strings (G4, D4, A3 plain; E3 wound) | Main strings | Unison courses | Mark |
|---|---|---|---|---|
| Position in the course | bass side (met first on a down stroke) | treble side | — | [MEAS, read] Wiki-12; Rickenbacker reversed |
| Delay of the second string in a strum | 1.6–2.5 ms (3 mm at 1.2–1.9 m/s) | | same | [DER] |
| … in a slow picked note | 3–6 ms | | | [DER] |
| … from a fingertip | 0–1 ms | | | [GUESS] |
| Depth relative to the stroke's nominal | first string 1.0, second 0.7–1.3 (random); octave strings a further × 0.8 on down strokes (they sit lower) | | second 0.8–1.2 | [GUESS] |
| Open-string mistuning (cents, fixed per performance) | s.d. 2.5, clip ±5 | 0 | s.d. 1.5, clip ±4 | [GUESS] |
| Slow drift during a piece | ±2 | | ±1 | [GUESS] |
| Fretting-stretch term, octave string against main | (−4.6, +1.9, +0.8, +0.3) × (1 − fret/12) cents on the E, A, D, G courses | | 0 | [DER, cores guessed] §22.4; replaces B §8.3's 6 cents |
| Saddle term | 1731 δ (1/ℓ − 1/L) cents, δ fixed per course in ±2 mm | | 0 | [DER] §22.5 |
| T60 at 3 kHz | 2–3 s (plain rows of §23.1); E3 wound 3 s | as §23.1's wound rows, × 0.9 | 2.3–2.5 s | [GUESS from §23.1] |
| Modes | 64 (E3: 80) | 96 / 96 / 80 / 64 | 48 | [GUESS] B §8.3 |
| Bend limit, vibrato | 50 cents, none | | 50 cents, none | [GUESS] |
| Tuned a semitone / tone low | all tensions × 0.891 / 0.794 (total 994 / 886 N); mistuning s.d. × 1.3 | | | [DER] / [GUESS] |

### 23.3 Body resonances (at the bridge; dreadnought, six-string)

| Mode | f (Hz) | Q | Peak mobility (dB re 1 m/(s N)) | Mass at bridge (kg) | Radiates | Mark, source |
|---|---|---|---|---|---|---|
| Whole-guitar bending | 55 | — | — | — | nothing | f [MEAS] FR; omit |
| Breathing T(1,1)₁ (air + top, back opposite) | 98 (96–102) | 35 (33–42) | −26 | 1.1 | all round; soundhole in phase with top | f, Q, level [MEAS] CBWFP18, FR; mass [DER] |
| Top T(1,1)₂ | 183 (156–193) | 23 (17–28) | −21 | 0.22 | all round, the loudest; soundhole out of phase | [MEAS] CBWFP18, FR, TUD20; mass [DER] |
| Back-coupled T(1,1)₃ | 204 | 30 | not a separate bridge peak on some guitars | ~1.0 | "comparable in all directions"; notes on it die fast | f [MEAS] FR, TUD20; Q, mass [GUESS] |
| Cross dipole T(2,1) | 271–377 | 40 (24–58) | −30 | 0.6 | little to the front, more upward | f, Q, level [MEAS]; mass [DER] |
| Long dipole T(1,2) | 305–436 | 40 | — | 0.8 | well to the front (TUD20); "quadrupole, little" on the D-28 (FR): sources disagree | f [MEAS]; rest [GUESS] |
| Long tripole, cross tripole | 500–550 | 40 | — | 1–2 | both radiate; short decays near C5–D5 | f [MEAS] TUD20; rest [GUESS] |
| Plates, 450 Hz–2 kHz | one per 30–60 Hz | 30–55 | mean −53 ± 8 | 3.0 at 500 Hz → 0.76 at 2 kHz | lobed above 1.5 kHz | level, density [MEAS, read from a figure] CBWFP18; masses [DER] B §2.2 |
| Plates, 2–5 kHz | statistical | 30–55 | −55 … −58 | 0.6–0.8 | weak 2.5–5 kHz, beamed at 7–8 kHz | [MEAS] CBWFP18, AAU via B |

Variants **[GUESS]** unless marked: OM/000: first five × 1.08 in f, × 0.85 in mass (lore: air
mode 100–115 Hz). Parlour (0/00): × 1.15–1.2, × 0.7. Jumbo: first three × 0.93, × 1.1. Scalloped
bracing: × 0.95–0.97 and Q × 1.1 against straight. Light, soft top against dense, stiff: the
triad moves by up to ±6 %, T(1,1)₃ stays ([MEAS, direction] TUD20). 12-string: masses × 1.25,
frequencies unchanged (B §6.4). Perceptual tolerance: 1–2 % in the low mode frequencies, 20 % in
Q ([MEAS] via part 1 §3.2).

### 23.4 Pick and fingers by dynamic

§19.2's table (force 0.3–3.5 N **[MEAS, weak]**; the rest **[GUESS]**). Fixed parameters:

| | Flatpick, strum | Flatpick, single notes | Thumbpick | Finger, flesh | Finger, nail / fingerpick | Mark |
|---|---|---|---|---|---|---|
| Contact width (mm) | 0.8 | 0.8 | 1.5 | 8–12 | 1–2 | [GUESS] |
| Rise of force (ms) | 0.6–1.2 | 0.6–1.2 | 1–2 | 5–15 | 2–5 | [GUESS] |
| Slip / release (ms) | 0.15 (thin pick 0.3) | 0.1–0.15 | 0.2 | 1–3 | 0.3–0.6 | [GUESS] |
| Force at mf / ff (N) | 0.8 / 2.0 per string | 1.2–1.8 / 3.5 | 1.5 / 3 | 0.5 / 1.0 | 0.7 / 1.5 | [MEAS, weak] Ang25 for pick and finger; split [GUESS] |
| Angle to the top | 0–15°, down: into the top | 10–20°; rest stroke deeper | 20–40° into the top | 20–45° away | 20–45° away | [GUESS] |
| Distance from the saddle (mm) | 120–165; ff 100–130; soft 190–230 | 70–110 | 120–140 | 110–150 | 110–150 | [GUESS], §19.3 |
| Scatter of the point | ±10 mm | ±5 | ±5 | ±5 | ±5 | [GUESS] |
| Click (noise burst 2–8 kHz, 0.3–1 ms) | −22 dB re note peak at mf; thin pick +4 dB | −24 | −26 | none | −30 (metal fingerpick −20) | [GUESS]; target B §1.3 [MEAS on FluidR3]: energy above 5 kHz in the first 5 ms at −7…−11 dB of the total |
| Scrape on wound strings | 5–25 wrap pulses in 1–3 ms when the pick is slanted | same, less | little | none | few | [GUESS], §19.4 |
| Damps the string before release | no | rest stroke: damps the *next* string | slightly | yes (40 /s during the rise) | slightly | [GUESS] |

### 23.5 Strum timing

| | Value | Mark, source |
|---|---|---|
| Time between adjacent strings | median 7.5 ms; 8.6 / 8.1 / 6.6 / 5.5 ms for 3 / 4 / 5 / 6 strings | [MEAS] GuitarSet, B §3.2 |
| Player-to-player factor | × 0.75–1.65 (medians 5.6–12.3 ms) | [MEAS] B §3.2 |
| Dependence on tempo | none, 60–140 bpm | [MEAS] B §3.2 |
| Step written in spread MIDI chords | median 8.3 ms (10th–90th percentile 3.5–16.7) | [MEAS in the user's files] part 1 §9.2 |
| Down / up share | 59 / 32 % (rest mixed); downs on the beat 53–73 %, ups 2–29 % | [MEAS] B §3.2 |
| Strings per stroke | 3.8 down, 3.6 up; ups start half a string higher | [MEAS] B §3.2 |
| Up-stroke level | −2 to −6 dB against down | [GUESS, lore]; not measured |
| Force along the stroke | falls 10–20 % from first to last string | [GUESS] |
| Stroke-to-stroke scatter | ±1–3 dB, ±3 ms, pluck point ±10 mm | [GUESS] |
| Rolled chord / rake | 20–60 ms a string | [GUESS] |
| 12-string | pairs: 2 ms inside, 5–6 ms between; hand speed × 0.9 | [DER] / [GUESS] §22.3 |

### 23.6 Note-off, velocity

| | Value | Mark |
|---|---|---|
| Fretted note released while strumming | amplitude loss 15–25 /s (gone in ~0.3 s), highs last ("zing" 50–150 ms) | [GUESS] B §3.5, §20.10 |
| Fretted note released, single lines | 45 /s (the built value) | [GUESS] |
| Open string at note-off | rings on until the next stroke or a palm (then 30–60 /s) | [GUESS] |
| Chuck / left-hand mute | stroke at ¼ depth, then 200–400 /s within 3–8 ms | [GUESS] B §3.4 |
| Palm mute | patch 12–18 mm at the saddle, T60 0.1–0.4 s, highs first | [GUESS] |
| Velocity → depth | (0.1 + 2.6 v^1.5) mm, i.e. 0.3–3.2 N at 130 mm | [DER] from B §8.2 and §19.1; range [MEAS, weak] |
| Velocity → level | about 16–21 dB from velocity 40 to 127 (FluidR3 under timidity: +16 dB for 40→120) | [MEAS on FluidR3] B §1.3; 21 dB [DER] from the force range |
| Velocity → brightness | little change from mf up; thin and noisy at the bottom; buzz and +3–15 cents attack pitch at the top | [MEAS in kind] Pluta via B §3.1; cents [DER] §19.1 |
| Velocity → pluck point | 20–30 mm towards the bridge from mf to ff | [GUESS] |
| Velocity → strum speed | × 0.8 (pp) … × 1.3 (ff) | [GUESS] |
| Vibrato | one-sided, 0…+10–25 cents, 4–6 Hz, after 100–300 ms, melody notes ≥ a quarter | [GUESS] §20.6 |
| Bend | ≤ 100 cents normal, ≤ 200 on strings 1–2 above the 5th fret; wider → slide | [DER] §20.5 |

### 23.7 What a listener would notice (additions to part 1 §8 and B §7)

1. **Every fretted note sustaining alike.** Real notes next to each other differ 3–5× in the
   life of their fundamental (§17.4); open strings outlast fretted ones.
2. **Wound and plain strings ageing alike, or no age at all.** A model at "age 0" is a guitar
   restrung an hour ago: correct, but not what most records sound like.
3. **A low E with a round fundamental**, or with a harmonic (unstretched) spectrum.
4. **Hard-picked notes that start exactly in tune**: the first 100 ms should be a few cents
   sharp on the low strings.
5. **Bends and vibrato like an electric guitar's** (whole-tone bends on the G, ±30 cents LFO).
6. **Chords in closed position everywhere**: no open strings ringing through the changes, no
   sympathetic strings, the same decay on every chord tone; and E♭ sounding as lush as G.
7. **A pluck point that never moves** (a fixed comb on every note of every chord), or a pick
   click of constant level regardless of dynamic (it should be relatively *louder* in soft
   playing).
8. **Hammer-ons and pull-offs as re-picked notes.**
9. **Squeaks without their two parts** (a pure sweep, or pure noise) or on plain strings.
10. **A parlour, a dreadnought and a jumbo that differ only in level.** They differ in where
    the two lowest peaks sit (±15 %).
11. The 12-string list: §22.8.

## 24. Not found, or uncertain (this part)

- **Per-partial decay times measured on a real steel-string acoustic with known strings.** Still
  none: the per-partial numbers are FluidR3's samples (B §1.2); [TUD20] gives only whole-note
  A-weighted durations, and its guitar model is not named in the report.
- **Measured B for a bronze acoustic set.** Only an electric set [TUM] and the qualitative
  result that wound hex-core strings exceed the core formula [Mur22]. The factor 1.3 in §23.1
  is a guess. Core diameters remain unpublished.
- **String damping in Valette's three parameters for steel and bronze-wound strings.** The
  pages and papers that hold them (Woodhouse et al. 2012; Woodhouse & Lynch-Aird 2019; the
  Euphonics side-pages on string damping) could not be read (bot walls, 403/404). Only "Q about
  3 000, loss factor still falling at 10 kHz" was obtained.
- **Ageing, quantitatively.** Allen (1976) was reached only as a search snippet; no curve of
  loss against hours of playing was found anywhere. §17.5's time scales are lore.
- **Phosphor bronze against 80/20, coated against uncoated:** no measurement.
- **Wood damping** (loss factors of spruce, rosewood, mahogany as used): from memory only.
- **Effective piston masses and areas for a steel-string's three-oscillator model**, the body
  volume of any named model (§16.1's are derived), bridge and bridge-plate masses: not found;
  the Cardiff theses were again unreachable.
- **OM, parlour and jumbo modal tables:** still lore only.
- **The 12-string:** no measurement of course mistuning, of beat rates, of the delay or level
  difference between the strings of a course, of octave-string intonation error, or of any
  12-string body. The spacing figures are one builder's forum post. Which side the octave
  string sits on is from Wikipedia; the amplitude consequences are mine.
- **Pluck force and displacement by dynamic on an acoustic guitar with a pick.** The only
  figures are from an electric guitar in a school-level journal [Ang25], by 240 fps video.
  Contact times, the pick's ring and the click's level remain unmeasured (B §9).
- **Pluck position in real playing** (distance from the saddle by style and dynamic): only
  estimation methods were found (Traube; Mohamad et al.), not their statistics for steel-string
  players. §19.3 is geometry plus lore.
- **Pluck angle** by technique: not measured anywhere found.
- **Vibrato and bend statistics** on acoustic steel-string guitars: none; §20.5 is computed
  for an ideal string, §20.6 is lore.
- **Squeak level and spectrum on a fretted acoustic:** the JASA Express Letter [PPB07] was
  reached only through an abstract summary.
- **Hammer-on, pull-off and harmonic levels;** fretting clicks; the level of sympathetic
  strings: estimates.
- **Radiation efficiency** (acoustic power out per string energy in) of a steel-string: not
  looked up successfully; nothing in this note depends on it.
- **D'Addario's unit weights** in §17.1 are from memory (cross-checked against part 1's chart
  for four strings); re-read the maker's chart or its tension calculator before using the
  medium set's tensions to better than 1 %.
- **Sources that disagree:** the radiation of the long dipole (strong to the front in [TUD20],
  a weak quadrupole on the D-28 in [FR]); the steel-string's air resonance (UNSW's "A2 (55.0
  Hz)" is internally inconsistent; 96–102 Hz measured elsewhere); the top monopole (156 Hz on
  the TUD guitar, 175–193 Hz on the Fylde and D-28): real spread between designs, not error.

## 25. References (this part)

- **[TUD20]** S. Merchel et al., "Guitar analysis using measurements – X-braced / E, ρ and G
  varying", project report, TU Dresden, Chair of Acoustics and Haptics, for a tonewood
  supplier, 30 Oct 2020, 30 pp. (read: Table 1, modal analysis, transfer functions, plucked
  notes, T15). https://cdn.shopify.com/s/files/1/0588/5480/1567/files/Guitar_Analysis_Using_X-Braced_Guitars.pdf
  The related journal paper (not read): S. Merchel, M. E. Altinsoy, D. Olson, "Perceptual
  evaluation of bracewood and soundboard wood variations on the preference of a steel-string
  acoustic guitar", JASA 146 (2019).
  https://tu-dresden.de/ing/elektrotechnik/ias/aha/ressourcen/dateien/professur/publikationen/Merchel2019jasa-Guitar_Quality.pdf
- **[RH19]** M. Rau, R. Hoover, "Measurements of acoustic guitar top plates during the voicing
  process", ICSV26, Montreal 2019 (read). https://ccrma.stanford.edu/~mrau/papers/ICSV2019_Rau_Hoover.pdf
- **[Euph7.3]** J. Woodhouse, *Euphonics*, section 7.3 "Multiple strings and double decays"
  (read through a summarising fetch: the 12-string case study, the per-instrument double-decay
  criterion). https://euphonics.org/7-3-multiple-strings-and-double-decays/ ; **[Euph]** section
  7.2, as in §15.
- **[W04b]** J. Woodhouse, "Plucked guitar transients: comparison of measurements and
  synthesis", Acta Acustica united with Acustica 90, 945–965 (2004) (read in part: pluck
  angles, split peaks). https://euphonics.org/wp-content/uploads/2022/03/Guitar_II.pdf
- **[WMSW12]** J. Woodhouse, E. K. Y. Manuel, L. A. Smith, A. J. C. Wheble, C. Fritz,
  "Perceptual thresholds for acoustical guitar models", Acta Acustica 98, 475–486 (2012)
  (search snippets only; the HAL copy is behind a bot wall). https://hal.science/hal-01461755
- **[WPG21]** J. Woodhouse, D. Politzer, H. Mansour, "Acoustics of the banjo: measurements and
  sound synthesis", Acta Acustica 5, 15 (2021) (snippet only; the publisher refused the fetch).
  https://acta-acustica.edpsciences.org/articles/aacus/abs/2021/01/aacus200052/aacus200052.html ;
  D. Politzer, "Pickers' guide to acoustics of the banjo" (read; qualitative),
  https://arxiv.org/pdf/2104.02480
- **[C11s]** J. Christian, "Quantifying multiple types of damping acting on bronze-wound guitar
  strings", Savart Journal 1(1) (2011) (read; 80/20 and phosphor-bronze .056 strings in air and
  vacuum; coefficients unreliable by the author's own account).
  https://www.savartjournal.org/articles/14/article.pdf
- **[Mur22]** C. J. Murray, "Musical string inharmonicity" (undergraduate research paper, read);
  journal version: C. J. Murray, S. B. Whitfield, "Inharmonicity in plucked guitar strings",
  Am. J. Phys. 90, 487–493 (2022) (abstract only).
  https://dornsife.usc.edu/sergey-lototsky/wp-content/uploads/sites/211/2023/06/Musical-string-inharmonicity-Chris-Murray.pdf
- **[TUM]** "Inharmonicity of sounds from electric guitars: physical flaw or musical asset?",
  TU München (read: Table 1, after Zollner 2007).
  https://www.ce.cit.tum.de/fileadmin/w00cgn/mmk/Lehre/Musikalische_Akustik/PDF/1048.pdf
- **[Allen76]** J. B. Allen, "On the aging of steel guitar strings", Catgut Acoust. Soc.
  Newsletter 26 (1976) (search snippet only; the PDF link returned an error page).
  https://jontalle.web.engr.illinois.edu/Public/Allen/AllenCatgut76.pdf
- **[PPB07]** J. Pakarinen, H. Penttinen, B. Bank, "Analysis of handling noises on wound
  strings", JASA 122(6), EL197–EL202 (2007) (abstract summary only).
  https://research.aalto.fi/en/publications/analysis-of-handling-noises-on-wound-strings/
- **[Ang25]** R. Angra, "Effect of plucking dynamics and tonewood on electric guitar tone",
  Journal of High School Science 9(4) (2025) (read; pluck forces by 240 fps video; a school
  journal: treat as weak). https://jhss.scholasticahq.com/article/146009.pdf
- **[NIU]** thesis on the force of a plucked string at the bridge of a classical guitar
  (snippet only). https://huskiecommons.lib.niu.edu/allgraduate-thesesdissertations/6519
- **[Inta]** R. Inta, J. Smith, J. Wolfe, a Hall-probe device for measuring top-plate
  thickness, WESPAC (read). https://www.phys.unsw.edu.au/jw/reprints/WESPACInta.pdf ;
  R. Inta, "The acoustics of the steel string guitar", PhD thesis, UNSW 2007 (not found online).
- **[UNSW]** UNSW Music Acoustics, "Acoustically important construction features" (read).
  https://newt.phys.unsw.edu.au/music/guitaracoustics/construction.html
- **[CV80]** O. Christensen, B. B. Vistisen, "Simple model for low-frequency guitar function",
  JASA 68, 758–766 (1980) (snippet only).
  https://typeset.io/pdf/simple-model-for-low-frequency-guitar-function-plccqpv99x.pdf
- **[Gore]** T. Gore, "Wood for guitars", Proc. Meetings on Acoustics 12 (2011), and lecture
  material (search snippets only). https://es.scribd.com/document/391667030/POMA-paper-pdf
- **[GW]** Guitar World, "A guide to Martin acoustic guitar body sizes" (read through a
  summarising fetch; the maker's dimensions).
  https://www.guitarworld.com/features/martin-acoustic-body-sizes
- **[Wiki-tw]**, **[Shepherd14]** Sitka reference values (snippets): Wikipedia "Tonewood";
  Shepherd et al., JASA 2014 via https://physics.byu.edu/docs/publication/7735
- **[Wiki-12]** Wikipedia, "Twelve-string guitar" (read through a summarising fetch).
  https://en.wikipedia.org/wiki/Twelve-string_guitar
- **[LF12]** Luthiers Forum, 12-string spacing thread (snippet).
  https://www.luthiersforum.com/forum/viewtopic.php?p=659535 ; **[SM12]** a 12-string bridge's
  string spread (snippet), https://www.stewmac.com ; Premier Guitar, "12-string guitar setup",
  https://www.premierguitar.com/diy/guitar-bass-mods/12-string-guitar-setup
- **[GC]** tuning a 12-string down, retailer and maker guides (snippets).
  https://guitarcenter.com/faq/tuning-a-12-string-guitar ;
  https://www.daddario.com/blogs/guitar/how-to-tune-a-12-string-guitar
- **Estimating the plucking point** (methods only, not used for numbers): C. Traube, J. O. Smith,
  DAFx 2001, https://www.ee.columbia.edu/~dpwe/papers/TraubS01-guitar.pdf ; Z. Mohamad, S. Dixon,
  C. Harte, JASA 2017, https://webspace.eecs.qmul.ac.uk/s.e.dixon/pub/2017/MohamadDixonHarte-JASA2017.pdf
- **Arithmetic** for §16–§23 (air drag, medium-set tensions, 12-string μ and Z₀, bends, pluck
  stiffness): a scratch script, not in the repository.
