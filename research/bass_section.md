# The missing basses: American amp Jazz and Precision (finger, pick, slap), the classic monosynth bass, the FM synth "E.BASS 1": research

Status: research 2026-11-19. Built: GM 34 (electric-bass, `src/ebass.cc`: the two-pickup bass fingerstyle, its DI blended with an fliptop bass amp and an optical compressor after: `research/bass_amps.md` 10.1, the amp module's "fliptop-di"; README "Electric bass"); the rest is not. In `src/midifile.cc` (the kGm
table, line 191) GM programs 34 Electric Bass (finger), 35 (pick), 36 Fretless, 37 Slap Bass 1 and
38 Slap Bass 2 (counting from 1) all fall back to `double-bass`. GM 39 and 40 exist: `synth-bass-1`
and `synth-bass-2` are patches of the analog lead circuit in `src/lead.cc` (lines 176–182), the
first a ramp plus its octave below through the Huovilainen ladder with a snapping filter envelope,
the second a narrow pulse ring-modulated by its octave. There is no FM engine in the project.

How each number is marked (as in `research/timpani.md`):
- **[T]** read in the primary text (downloaded and read for this document). "Measured" is said in
  words where the text reports a measurement and not a calculation.
- **[S]** from a secondary source, an abstract, a search snippet, a retailer's or maker's product
  page, or a machine summary of a page: the primary text was not read.
- **[L]** player lore: a teaching text or a player's description. Not a measurement.
- **[E]** my estimate, fit or arithmetic from cited values; not a published value. Where it comes
  from running code (the FM synth voice through a re-implementation of Dexed's engine), this is said.
- **[M]** from general knowledge, not looked up: check before relying on it.

References are short keys in brackets; the list with URLs is section 13. What could not be found
is in section 12.

**Short answer.**
- **Electric basses (GM 34, 35, 37, 38).** A 34 in. (864 mm) scale, four steel roundwound strings
  E1–G2 at 155–215 N each (standard EXL170: E 34.7 lb, A 40.1, D 48.4, G 42.6 lb) **[S]**, on a
  solid body whose *neck*, not bridge, takes the strings' energy (neck conductance 0.3 ms/kg at a
  live spot, over 2 and up to 20 ms/kg at a dead spot; bridge ≤ 0.3 ms/kg) **[T, Fleischer]**.
  - Losses: the string's own loss factor is 0.0004–0.001, so the open E's fundamental lasts tens
    of seconds unless the neck takes it. A measured G string (core 0.43 mm, 191.6 N) has
    B = 3.5e-5 **[T, Issanchou]**; the low strings perhaps 1.5–2.5e-4 open **[E]**, four times
    that at the 12th fret.
  - What makes a P differ from a J is *where the pickups listen* (P: one split coil 124/149 mm
    from the bridge, 6.4–7 H; J: 155 and 55–64 mm, 3.2–3.35 H each) and how the coil's
    inductance meets the cable (a resonance near 2.4 kHz for the P and 3.3–4.7 kHz for the J)
    **[S, E]**.
  - The techniques are excitations: a soft wide fingertip, a narrow fast pick, a thumb striking
    the string down onto the last frets (a velocity impulse), and a finger pulling it up and
    letting it snap onto the frets. In both slaps the string hits the frets during its first one
    or two periods (about 0–90 ms), which gives the metallic burst near 4–5 kHz **[T, Rank97;
    Issanchou18]**.
- **classic monosynth bass (GM 39).** Three free-running VCOs (saw, triangle-saw, square, two pulses), a
  mixer that can be overdriven into the 24 dB/oct transistor ladder (10 Hz–20 kHz, self-oscillates
  to a sine), two contours with attack 1 ms–10 s and decay 4 ms–35 s, filter contour up to
  4 octaves **[T, Moog manual]**. The project's ladder is already Huovilainen's; the classic monosynth bass
  needs the three oscillators beating, the mixer drive and single triggering, not a new filter.
- **FM synth "E.BASS 1".** ROM3A voice 12, parameter for parameter the same as ROM1A voice 15 "BASS 1"
  (checked byte by byte) **[T, sysex parsed here]**.
  - Algorithm 16: one carrier, op1 at 0.5 × the key, transposed down an octave, so a key sounds
    two octaves low. op1 is modulated by op2 (1:1, index ≈ 2.4 rad, slowly fading: the body), by
    op3 ← op4 (op4 at 10 × f0) and by op5 ← op6 (op6 at 18 × f0 with feedback 7).
  - Velocity changes only the timbre: the carrier has velocity sensitivity 0, the modulators 3–7.
  - The "slap" is a *harmonic* burst from op4→op3, centred at 13–19 × f0 in the first 10 ms at
    velocity 100–127 and fallen to about 2 × f0 by 0.3 s. Its keyboard level scaling makes the
    feedback pair op6→op5 nearly silent in the bass range **[E, from running the patch through a
    port of Dexed's engine]**.

---

## 1. What the project has, and what can be reused

- **GM mapping today** (`src/midifile.cc:191–192`): `{"electric-bass", "double-bass"},
  {"pick-bass", "double-bass"}, {"fretless-bass", "double-bass"}, {"slap-bass-1", "double-bass"},
  {"slap-bass-2", "double-bass"}, {"synth-bass-1"}, {"synth-bass-2"}`.
- **Earlier decisions to keep** (`research/guitars.md` §10–11.1, the user's answers of
  2026-11-16): GM 36 Fretless **stays on the double bass** (in the user's jazz files it stands in
  for the upright: 25 000 notes in 38 files). Bass lines written on guitar programs go to the
  electric bass once it exists. The files hold about 2 500 notes on GM 34–38 **[guitars.md]**.
- **`src/guitar.h/.cc` (`Guitar`)**: the nearest template. Six strings, each 48 modes in two
  directions (towards and along the top), the fret as a shorter string, a pluck that plants,
  pulls and lets go off the flesh then the nail, damping by a lying hand, a bridge on body modes,
  and the guitarist's fingering choice. An electric bass is the same class with four strings,
  more modes on the low strings (see §9.1), no acoustic body, and pickups instead of a microphone.
- **`src/bass.h/.cc` (`Bass`)**: the fingerboard contact: `kBoard` = 5 points along the board,
  each a one-sided penalty spring (`kBoardK` = 3e4 N/m, `kBoardC` = 1.5 N s/m) acting through each
  mode's shape at that point (`bass.cc` 316–325, 769–790). This is the mechanism for fret clank
  and slap, with the frets as the contact points and much stiffer contacts. Also there: tension
  modulation (pitch starts sharp), `setLength` slides, `touch` harmonics, the thumb (finger 3), the
  calloused skin knob, the two polarisations with a turning plane.
- **`src/clavi.cc`**: magnetic pickups reading string velocity at a point, with the magnet's
  nonlinearity, selector in or out of phase, and a passive tone network turned into an exact
  filter by `tools/clavtone.py`. The P and J pickups, pots, tone cap and cable are the same
  problem with other values (§3).
- **`src/lead.cc`**: the expo converter and ramp core, the Huovilainen ladder (2× oversampled
  inside), the OTA, RC envelopes, per-voice part tolerances and drift, the shared board. The
  classic monosynth needs three cores per voice, a mixer stage that can saturate, and classic monosynth contour
  and trigger behaviour (§7).
- **No FM engine.** `research/electric_pianos.md` §6 parsed ROM1A "E.PIANO 1" and decided to play
  GM 6 on the tine piano model; nothing implements operators. The FM synth voice (§8) needs one.

## 2. The electric bass: what the split-coil bass and the Jazz share

### 2.1 Scale, frets, strings

- **Scale 34 in. = 863.6 mm**, nut to saddle (the saddles move a few mm for intonation) **[M]**.
  Fret q lies at L(1 − 2^(−q/12)) from the nut. Measured: 0.863 m on the bass of [Issanchou18]
  **[T]**; 0.873 m on a one bass maker 5-string [Kemp20] **[T]**.
- **Frets:** 20 on the classic P and J, 21 on many later ones **[M]**. With 20 frets the G string
  reaches D#4 (MIDI 63). Where the neck meets the body (the last frets) the string stands about
  L·2^(−20/12) ≈ 27 cm from the saddle **[E]**; this is where the thumb slaps.
- **Tuning:** E1 41.2, A1 55.0, D2 73.4, G2 98.0 Hz **[T, Fleischer II, Tab. IV]**.
- **A standard set, standard EXL170 (45-65-80-100) long scale:**

| String | Gauge (in.) | Tension (lb) **[S]** | Tension (N) **[E]** | μ (g/m) from T = μ(2Lf)² **[E]** | Z = √(Tμ) (kg/s) **[E]** |
|---|---|---|---|---|---|
| G2 | .045 | 42.6 | 189.6 | 6.6 | 1.12 |
| D2 | .065 | 48.4 | 215.6 | 13.4 | 1.70 |
| A1 | .080 | 40.1 | 178.6 | 19.8 | 1.88 |
| E1 | .100 | 34.7 | 154.8 | 30.6 | 2.18 |

  - The E figure (34.7 lb, 15.75 kg) is from standard's own product pages **[S]**; the other
    three are a retailer's copy of standard's chart for the same singles (kg, converted) **[S]**.
  - **Check against a measured string:** the electric bass G string of [Issanchou18] has
    d = 1.14 mm (.045 in.), core 0.43 mm, T = 191.6 N, μ = 6.69 g/m, L = 0.863 m **[T, Tab. IV]**,
    within 1 % of the table's G row.
  - **Fleischer's "typical" strings** **[T, Tab. III, V]:** radii 1.2, 1.0, 0.7, 0.5 mm (E, A, D,
    G); speaking-length masses 23, 15.3, 6.8, 3.9 g; Z = 1.9, 1.7, 1.0, 0.76 kg/s. That makes μ
    26.7, 17.8, 7.9, 4.5 g/m: lighter than the EXL170 on the top two strings (older or lighter
    sets).
  - **Flatwounds** (the Motown P bass: heavy flats, e.g. .052–.110) carry more tension and die
    sooner and darker **[L]**. No tension chart was read.

### 2.2 Inharmonicity

- **Formula:** f_p = p f₀ √(1 + B p²), B = π² E S κ² / (L² T) for a wound string, where E S κ² is
  the core's bending stiffness. For a hex core with points-to-points d₁, S = (3√3/2)(d₁/2)² and
  κ = √(5/6)·d₁/4; the stiffness is about 7 % above the core's alone because of the windings
  **[T, Kemp20]**.
- **Measured / fitted:**
  - G2 (.045, core 0.43 mm, 191.6 N, 0.863 m): **B = 3.5e-5**, fitted to measured partials up to
    3.4 kHz **[T, Issanchou18, Tab. V]**. For the core alone B would be 2.4e-5 **[E]**: the
    winding adds about 45 %.
  - B0 (.132, hex core .032 in. at the points, 0.873 m): B ≈ 2.5e-4 open **[E, from Kemp's
    Eq. 4 and his Tab. 3]**. Measured: "around 100 cents at the 10th partial" stopped at the 12th
    fret, "around 100 cents at the 6th partial" at the 21st **[T, Kemp20]**.
- **Estimates for the EXL170** (cores not published; an E .100 with a hex core about .029 in. at
  the points) **[E]**:

| | Open | 12th fret (×4) | Cents at the 10th partial, open / 12th |
|---|---|---|---|
| E1 | 1.5–2.5e-4 | 6–10e-4 | 13–21 / 50–80 |
| A1 | ~1e-4 | ~4e-4 | 9 / 35 |
| D2 | ~6e-5 | ~2.4e-4 | 5 / 21 |
| G2 | 3.5e-5 **[T]** | 1.4e-4 | 3 / 12 |

  - B grows as 1/L² when stopped (×4 at the octave, ×12 at the 21st fret), so the stiffness
    shows most in high notes on low strings. This is why bassists favour thinner strings up the
    neck **[L]**.
- **Pitch glide (measured, Kemp's B string):** 6.3 cents/s open, 15.8 at the 12th fret, 10.5 at
  the 21st for a standard string. Kemp puts it down to inharmonic upper partials dying first,
  together with tension modulation **[T, Kemp20]**.

### 2.3 Losses and decay

- **Three mechanisms** **[T, Fleischer II §7]**, each a time constant τ (amplitude, 1/e) per
  partial, with T30 = 3.45 τ:
  - **Air:** τ_air ≈ 364 (R/mm)/√(f/Hz) s. Above 10 s for every string of a 4-string bass, above
    60 s for the B: negligible.
  - **Internal (string material and core–winding friction):** τ_int = 1/(π η f). Fitted η: E1
    0.0009–0.001, A1 0.0005–0.0006, D2 0.00035–0.0004, G2 0.00045–0.00047 **[T]** (one figure
    caption prints 0.0045 for the G; the text says 0.00045). So **an open E1's fundamental alone
    would have τ ≈ 7.7 s (T60 ≈ 53 s)**; the partials decay ∝ k (each octave twice as fast).
  - **Support (energy into the neck at the fret or nut):** τ_sup = 1 / (4 m_open f_open G f),
    with m_open the speaking mass and G the conductance (s/kg). G = 1 ms/kg gives 6 s open and
    3 s at the 12th fret. G = 10 ms/kg, "not uncommon for bass necks", gives fractions of a second
    **[T]**.
- **Fleischer's functional model** (measured on two basses: open and fretted decays at the output
  jack) **[T, Fleischer II §8]**:
  - the total signal's decay follows the partial with the longest time constant;
  - at a *live* spot that is the fundamental, internally damped;
  - at a *dead* spot the neck's conductance is high at the fundamental's frequency, which
    "switches off" that partial, and the decay follows the 2nd partial (twice as fast), or even
    the 3rd. Example: the A1 string's 5th fret of the Action Bass (an American amp P copy) has
    G > 2 ms/kg at 73.5 Hz and an extreme value at 147 Hz, so the third partial carries the note.
    The live 12th fret has G = 0.3 ms/kg at 110 Hz.
- **Neck modes** (Action bass in playing position) **[T, Fleischer I §8]**: rigid-body motion at
  very low frequency; the first bending mode near 40 Hz (maximum at about the 7th fret); the
  second near 150 Hz (minimum near the 12th, maximum at the low frets); the third just under
  300 Hz; the fourth above 500 Hz. Torsion of the neck and head makes the conductance differ
  between strings at the same fret. Bridge conductance is about 0.3 ms/kg or less. A
  well-known American amp dead spot, the G string around the 5th–7th fret (C3–D3, 131–147 Hz), sits on
  the second bending mode **[L for the lore; T for the mode]**.
- **A measured high-frequency loss** **[T, Kemp20]**: on the one bass maker B string, T60 is 0.4 s at
  8 kHz at every fret, 13 s at 600 Hz (open and 12th fret), 10 s at 600 Hz at the 21st. Between
  600 Hz and 8 kHz the decay rate grows about as f^1.35 **[E]**: the `loss0 + loss1·k^1.2` form of
  `BassString` fits.
- **The magnetic pickup adds no measurable damping** (shown for the guitar) **[S, PLC14 via
  guitars.md]**; Kemp mentions magnetic drag as a cause of the two polarisations differing **[T]**.
- **Not found:** measured T60 tables for an American amp P or J. Fleischer's figures 50–67 hold T30 per
  fret for two basses, but only as plots, which pdftotext cannot read.

### 2.4 Two polarisations

- The string swings both perpendicular to the body (towards the pickup and the frets) and
  parallel to it. A magnetic pickup is far more sensitive to the perpendicular motion **[S, PLC14
  via guitars.md]**. So the direction of the pluck (a rest stroke pushes the string towards the
  body; a pop pulls it away) decides how loud and how long a note sounds through the pickup, and
  whether it reaches the frets.
- At the bridge the two directions see different admittances, so each partial has two slightly
  different frequencies and decays (as `Bass` already does) **[M]**.

## 3. Pickups and electronics

### 3.1 Where the pickups listen

Measured on real instruments in a players' database (from the middle of the 12th fret to the
middle of the pole piece, cm, 34 in. basses) **[S, pickup-position-db]**; from the saddle =
43.2 cm − value **[E]**:

| Bass | Coil | From 12th fret (cm) | From saddle (mm) **[E]** | As a share of L |
|---|---|---|---|---|
| Precision ('72, Road Worn 2011, Special) | E/A half | 28.0–28.4 | 148–152 | 0.172 |
| | D/G half | 30.7–31.1 | 121–125 | 0.142 |
| Jazz (USA Standard, CIJ '62, Fretless '98) | neck | 27.5–27.7 | 155–157 | 0.180 |
| | bridge, '60s spacing | 36.5–36.8 | 64–67 | 0.075 |
| | bridge, '70s spacing | 37.7 | 55 | 0.064 |

- Bartolini's maker's rule: the J bridge pickup centre at L/16 = 54 mm from the saddle; the neck
  pickup about 152 mm from the G saddle **[S]**. This agrees with the table's '70s spacing.
- **What a position does** (a velocity pickup at a point hears mode k as sin(kπx/L); dB under the
  strongest of the first 20 harmonics of an open string, before the string's own spectrum and the
  coil's width) **[E]**:

| Coil | k = 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| P, E/A half | −6 | −1 | 0 | −2 | −8 | **−19** | −4 | −1 | 0 | −2 | −10 | −13 | −3 | 0 | 0 | −3 |
| P, D/G half | −7 | −2 | 0 | 0 | −2 | −7 | **−37** | −7 | −2 | 0 | 0 | −2 | −8 | **−31** | −7 | −2 |
| J neck | −5 | −1 | 0 | −2 | −10 | −12 | −3 | 0 | −1 | −4 | **−22** | −7 | −1 | 0 | −2 | −8 |
| J bridge '60s | −13 | −7 | −4 | −2 | −1 | 0 | 0 | 0 | −1 | −3 | −5 | −9 | **−19** | **−19** | −9 | −5 |
| J bridge '70s | −14 | −8 | −5 | −3 | −1 | −1 | 0 | 0 | 0 | −1 | −2 | −3 | −6 | −9 | −17 | **−25** |
| J both ('60s, in phase) | −8 | −3 | −2 | −2 | −4 | −8 | −17 | **−39** | −29 | −24 | −12 | −8 | −6 | −7 | −12 | −22 |

  - The J with both pickups full has a deep notch around the 7th–10th harmonic of the open
    string (on the E: 290–410 Hz): the scooped "J both up" sound that slap players use **[E; the
    lore is L]**.
  - On a stopped string the notches move: x/L grows as L shrinks.
- **Aperture:** a pickup does not read one point. Each string passes two pole pieces, and the
  field spreads over about the coil's width. Rank & Kubin found a one-point pickup gave
  "unrealistic high frequency components" and used a Gaussian-weighted tap around the pickup
  position **[T, Rank97]**. A width of 10–20 mm suits the J's and P's pole pairs **[E]**: it
  removes what a point pickup would hear above about c/(2·width), i.e. 5–10 kHz on the E.
- **Pole layout:** the P's two half-coils each sit under two strings with two poles per string;
  the halves are wound and magnetised oppositely, in series, so hum cancels (a split humbucker).
  Each J pickup has eight poles; the neck and bridge J are reverse-wound and reverse-polarity, so
  hum cancels only with both up **[M]**. Bass pickups have two poles per string "because of the
  larger string excursion" **[S, HdM slides]**.
- **Distance to the string:** typically 2–3 mm from the pole to the fretted-out string **[M]**.
  A loud E1 swings several mm, so the pickup's response to distance is far from linear: even
  harmonics and the "bark" of a hard pluck **[M; Horton & Moore 2009, via guitars.md]**.

### 3.2 Coils

| Pickup | DC resistance | Inductance | Source |
|---|---|---|---|
| American amp '62 P (Original / Pure Vintage '62 reissue) | 10.5 kΩ | 6.4 H | **[S, Fender part page]** |
| American amp Pure Vintage '63 P | 12.0 kΩ | 7.0 H | **[S]** |
| American amp Cobalt Chrome P | 12.4 kΩ | 8.5 H | **[S]** |
| American amp Original Jazz, bridge | 7.5 kΩ | 3.35 H | **[S]** |
| American amp Original Jazz, neck | 7.25 kΩ | 3.2 H | **[S]** |

- Unloaded self-resonances in makers' charts: "vintage split-coil bass 11.6 kHz, J about 9 kHz", which
  implies coil capacitances of about 30 pF (P) and 90 pF (J) **[S, forum; E]**. Treat 50–150 pF
  as the range **[E]**.

### 3.3 Pots, tone, cable

- **Controls:** P: one volume, one tone. J: two volumes (one per pickup, wired as a blend into a
  parallel sum) and one master tone. 250 kΩ pots and a 0.047 µF tone capacitor in the reissue
  harnesses of both **[S, StewMac, Lollar]**. Some 1960s-style J harnesses use a 500 kΩ tone pot
  **[S]**. A 0.1 µF cap on early P basses is often claimed but no source was found **[M]**.
- **Cable:** 300–1000 pF for 3–6 m of instrument cable **[M; guitars.md §4.1]**. Take 700 pF.
- **The loaded resonance** f = 1/(2π√(LC)) and the load R (pots in parallel, amp 1 MΩ) **[E]**:

| Configuration | L | C | f | R load | Q ≈ R√(C/L) |
|---|---|---|---|---|---|
| P, tone open | 6.4 H | 0.75 nF | 2.3 kHz | 250k‖250k‖1M = 111 kΩ | 1.2 (a gentle bump, damped further by the 10.5 kΩ coil) |
| J, one pickup (other volume at 0) | 3.3 H | 0.75 nF | 3.2 kHz | 3 × 250k‖1M ≈ 77 kΩ | 1.2 |
| J, both up (parallel) | ~1.65 H | 0.8 nF | 4.4 kHz | 77 kΩ | 1.7 |
| P, tone fully off (0.047 µF across the coil) | 6.4 H | 47 nF | 290 Hz | | the "thump": the Motown P with flats and the tone off **[L]** |

  - Eddy currents in the poles and covers lower Q further than these numbers show **[M;
    Zollner]**.
  - The J's volume pots interact: turning one pickup down loads the other (the blend is not
    independent) **[M]**.
- **After the jack:** basses are usually recorded through a DI and/or an amp (an bass amp fliptop or
  bass stack and a 1×15 / 8×10 cabinet) **[M]**. A first model can stop at the DI: pickup + pots + cable
  + a preamp's 1 MΩ input.

## 4. The split-coil bass

- **What it is:** 1951 (single coil, slab body); from 1957 the split-coil humbucker at 121–152 mm
  from the bridge, one volume, one tone **[M]**; 20 frets, 34 in. scale.
- **Its sound, from the numbers above:** each string pair heard at one place only (E/A at 0.172 L,
  D/G at 0.142 L), so the open E has a notch near its 6th harmonic and a strong 2nd–4th. A
  6.4–7 H coil gives a resonance near 2.3 kHz on a normal cable, so there is less top than on a J
  and a strong low mid: the "P punch" **[E; the description is L]**.
- **Typical uses** **[L]**:
  - **fingerstyle**: plucked over or just neck-side of the pickup, often with flatwounds and the
    tone rolled back (Jamerson);
  - **pick**: near the bridge, often with a foam mute under the bridge cover (Carol Kaye's
    session sound), or roundwounds and a pick for rock;
  - **slap** is less typical than on the J.
- **Model:** strings §2.1–2.3; the split coil as two velocity taps (E/A strings at 149 mm, D/G at
  124 mm), each with an aperture; the coil's L, R and C; a tone network (250 k pot + 0.047 µF);
  the cable. The variant for GM 35 (pick bass) should be the P.

## 5. The two-pickup bass

- **What it is:** 1960. An offset body and a narrower nut (1.5 in.). Two single coils at about
  155 mm and 55–67 mm from the bridge ('60s vs '70s spacing; 1960–62 had stacked
  volume/tone knobs, then vol/vol/tone) **[S for positions; M for history]**.
- **Its sound** **[E; descriptions L]**:
  - **neck alone:** close to a P, rounder: notch near the 11th harmonic, resonance about 3.2 kHz;
  - **bridge alone:** thin and nasal. The fundamental is 13 dB under the strongest harmonic, the
    5th–9th are strong (the "Jaco" bridge sound when the player digs in near it);
  - **both full:** the scoop at the 7th–10th harmonic, a resonance near 4.4 kHz. This is the
    slap bass's classic setting **[L]**.
- **Typical uses:** fingerstyle between or over the pickups (Jaco over the bridge pickup); slap
  (Marcus Miller's 1977 J, both pickups up; Larry Graham) **[L]**.
- **Model:** as the P, with two taps on every string, two coils summed in parallel through their
  volume pots, and the shared tone. GM 34 (finger), 37 and 38 (slap) should be the J.

## 6. The techniques

### 6.1 Fingerstyle (GM 34)

- **Who plucks:** index and middle alternately (the IDMT taxonomy's "finger-style") **[T,
  Abeßer13]**. Usually a *rest stroke*: the finger ends on the next string. The string is pushed
  towards the body as well as along it, which the pickup favours **[L]**.
- **Where:** J players between or over the pickups, about 6–16 cm from the bridge; P players over
  the pickup or over the end of the neck, about 12–25 cm **[L]**. The double bass model plucks at
  0.2 of the string **[bass.h]**.
- **Contact:** the fleshy pad, 8–12 mm wide, slipping off in about 2–5 ms (the guitar's table)
  **[M]**; calloused bass fingers slip off faster, about 1 ms (as `Bass` already assumes)
  **[bass.h; L]**. Nails are rare on bass; a nail adds a click and highs (as the guitar's nail)
  **[L]**.
- **Depth:** a few mm. A hard pluck reaches the frets (fret growl), more so at low action **[L]**.
- **A finger landing on a ringing string stops it** (as in `Guitar` and `Bass`). The plucking
  hand's thumb rests on a pickup or on the E string and mutes the lower strings ("floating thumb")
  **[L]**.

### 6.2 Pick (GM 35)

- **Picks:** 0.6–1.5 mm Delrin, celluloid or nylon; bassists mostly 0.88–1.5 mm **[L]**.
- **Contact:** 0.5–1 mm wide, a slip of about 0.1–0.5 ms **[M; guitars.md §2.1]**. A thin
  flexible pick bends and lets go later and softer; a stiff one gives the narrowest, quickest
  release.
- **The click:** the pick's edge strikes the string and scrapes the winding as it slips. A
  broadband transient a few ms long, strongest about 2–6 kHz **[M]**, plus the brighter string
  spectrum from the narrow release.
- **Down- and upstrokes** alternate on eighth notes and differ in level and direction **[L]**.
- **Palm or foam mute**, common with a pick on a P: a lossy pad 0–2 cm from the saddle. The
  guitar's palm mute applies (guitars.md §2.1) **[design]**.

### 6.3 Slap: the thumb and the pop (GM 37, 38)

- **Rank & Kubin's account** (Vienna 1997, recordings and a waveguide model) **[T, Rank97]**:
  - "Two different slapping techniques: the string is either struck (slapped) with the knuckle of
    the thumb or it is pulled strongly away from the guitar body with the index or middle finger.
    In both cases, the string hits the frets during the first fundamental periods of the tone."
  - The thumb is "very similar to striking a piano string with a hammer" and modelled as an
    initial *velocity* pulse at the striking point; the pop as a large triangular initial
    *displacement*. Both have rounded edges.
  - The fret contact is a one-sided limit on the string's displacement along the fingerboard
    (a conditional reflection where the string would pass below y_fret(x)). The limit must act
    "at least during the first period at the last fret at the body end"; the clearance shrinks
    towards the first fret but not to zero, or the string is damped within a few periods.
  - Heard in the signal: in the first period, extra fast reflections, one from the last fret
    (its delay is the time from the body-end of the string to the end of the fingerboard) and two
    from the string's recoil off the thumb. In the first two periods, a broadband noise pulse from
    contacts at the other frets ("fret noise"). After about the second period the waveform is
    nearly periodic and decays.
  - Their model ran a C on a 486DX2-66 at twice real time for E1, with about 800 delay units
    along the fingerboard at 44.1 kHz and the contact points thinned to one per 8 sections.
  - **What is missing in it:** the thumb as a second contact that comes back. After striking, the
    thumb stays near the string and the string recoils off it, "another conditional reflection
    point" (the piano hammer's case); not modelled.
- **Issanchou et al.'s measured electric bass** (G string of Tab. IV, plucked 64 cm from the nut,
  i.e. about 22 cm from the bridge, initial amplitude 3.6 mm, string against the real measured
  neck profile with 20 frets) **[T, Issanchou18]**:
  - contacts occur in **0–0.09 s**;
  - "substantial high frequency content from the very first collisions";
  - a reinforced spectral zone **around 4.5 kHz**;
  - measured and simulated agree once the linear string is fitted up to 3.4 kHz (B 3.5e-5,
    η_ve 0.01, thermoelastic Q⁻¹ 6e-6, the measured nut admittance in the losses).
  - Their converged reference needs 863 modes and 1–8 MHz; the restitution coefficient hardly
    matters with a few point contacts. A continuous contact (a fretless board) is different:
    sticking can occur along an interval.
- **Where and how hard** **[L]**: the thumb strikes over the end of the neck (the last frets,
  about 22–27 cm from the bridge), bouncing off (a rebound stroke) or following through to rest on
  the next string. The pop hooks under a D or G string and pulls it a few mm up and away from the
  body. Pops usually take the octave above a thumbed note. Left-hand "ghost" notes (dead notes,
  the string touched but not pressed) fill the rhythm.
- **The IDMT dataset** separates slap-thumb (ST) and slap-pluck (SP), finger (FS), pick (PK) and
  muted (MU) on one bass, with all strings and frets 0–12 **[T, Abeßer13]**. In the 17 recorded
  bass lines ST is 12 % and SP 6 % of the notes. In listening tests, "changing the plucking style
  ... is most prominent for the bass lines using either the slap-techniques ... or the picked
  technique"; slap lines suffered most from a 3.5 kHz low-pass; and tuning the loss filters and
  inharmonicity "can be considered as less important" than the pluck **[T]**. The dataset is free
  to download (IDMT-SMT-BASS, IDMT-SMT-BASS-SINGLE-TRACKS) and is the obvious fitting material.

### 6.4 Slap Bass 1 against Slap Bass 2 in GM practice

- The GM Level 1 list gives only the names **[M]**. No primary description of how the two should
  differ was found.
- A Japanese GM-module column describes Slap Bass 2 as "the same slap bass technique, but this
  variation has a more compressed, squashed tone", with velocity driving the filter **[S,
  Soundhouse; snippet only]**.
- GS's variation banks add "Slap Pop", "Reso Slap" and "Unison Slap" under program 37 (from 0)
  **[S, battleofthebits]**: makers treated thumb and pop as related sounds.
- **Proposal [E]:**
  - **Slap Bass 1:** the J with both pickups, thumb on notes that start a figure or lie low, pop
    on notes an octave or more above the last thumbed note or above about D3, ghost notes at low
    velocity. Velocity sets how hard: soft notes barely reach the frets.
  - **Slap Bass 2:** the same player through a compressor (fast attack, 4:1, about 6 dB of gain
    reduction) and a brighter pop. Or a thumb-only variant. The user should choose by ear.

### 6.5 The left hand, fret and finger noise

- **Fretted note:** the string ends on a metal fret (little loss); the finger just behind adds a
  little. Note-off: the finger eases off but stays on the string, a fast damp (about 50–150 ms
  to −30 dB) **[L; E]**. Open strings are damped by the right hand or another left finger.
- **Fret buzz** at low action or hard plucks: the same fret contact as slap, weaker.
- **Hammer-on / pull-off / slide:** `Bass::setLength` (the fret-by-fret steps of a slide are the
  fretted string's length jumping a semitone at a time). A hammer adds a small strike, a pull-off
  a small pluck **[M]**.
- **Finger squeak on roundwounds:** a finger sliding along the outer winding excites it at
  (speed)/(winding pitch). With an outer wrap of 0.5–0.7 mm and shifts of 0.3–1 m/s that is
  0.4–2 kHz, with harmonics, a few tens of ms long **[E]**. Flatwounds hardly squeak **[L]**.
- **Dead notes** (expression "DN"): "a percussively sounding note with almost no harmonic
  components" **[T, Abeßer13]**: the string plucked under a lightly touching left finger.

### 6.6 The fretless "mwah" (GM 36, if ever built)

- **Mechanism:** the stopping fingertip presses the string onto a hard wooden (or epoxy) board.
  Near the finger, the vibrating string grazes the board along a short stretch, a distributed
  one-sided contact like a sitar's jawari: "really just accurately controlled buzz", brighter on
  harder boards **[L, TalkBass]**. Issanchou et al. note that continuous contact regions ("e.g., a
  fretless bass") behave unlike point contacts: sticking may occur along an interval **[T]**. No
  paper measuring the mwah was found.
- **Also different from fretted:** the string ends on a soft fingertip, not metal (more loss,
  more so high up: the double bass's stopped string). Vibrato and glides are continuous. Players
  favour the bridge pickup and roll off the tone **[L]**.
- **In this project it is the double bass's board contact** (`Bass`, `kBoard`), which
  double_bass.md already credits with the steel-string "mwah". GM 36 stays there (§1).

## 7. The classic monosynth bass (GM 39, Synth Bass 1)

### 7.1 The instrument (classic monosynth; the 2016 reissue's manual) **[T, Moog]**

- **Oscillators:** three, 0.1 Hz–20 kHz in six overlapping ranges (LO, 32', 16', 8', 4', 2'),
  short-term stability "> 0.25 %" as printed. Waveforms: triangle, triangle-sawtooth (osc 1 and
  2), reverse sawtooth (osc 3), sawtooth, rectangle (square), wide pulse, narrow pulse. Osc 2 and
  3 are detuned against osc 1 by their knobs; osc 3 can leave keyboard control (a modulator).
- **Mixer:** five sources (3 oscillators, white/pink noise, external input). With nothing in the
  external input jack the main output, slightly attenuated, is fed back into the mixer. "By
  increasing the External Input volume far enough, the Mixer can overload, introducing varying
  levels of overdrive or distortion"; fully up "only one sound is heard and different pitches do
  not sound". Keep the main output at or below 6 for "the most musical range of overdrive".
- **Filter:** "a traditional classic ladder Ladder Filter", 24 dB/oct low-pass, cutoff 10 Hz–20 kHz (the
  text also says 10 Hz–32 kHz), Emphasis feeds the output back to the input; turned up with a low
  cutoff "the Filter can be coaxed into a self-oscillating state, acting as a sine-wave oscillator"
  playable from the keyboard. Keyboard tracking switches 1/3 and 2/3, both on = one octave per
  octave.
- **Contours** (filter and loudness, each attack, decay, sustain): attack 1 ms–10 s, decay
  4 ms–35 s, sustain 0–100 %. With the DECAY switch on, the decay time is also the release.
  Filter contour amount up to 4 octaves; loudness 80 dB range.
- **Keyboard:** 44 keys F0–C4, glide 1 ms–10 s per octave, bend ±7 semitones (reissue). Note
  priority selectable in the reissue; the original is low-note priority **[M]**. The original's
  contours do not retrigger on legato notes (single triggering) **[M]**.

### 7.2 The ladder

- **Circuit** (the 1969 transistor ladder patent; [Huovilainen04] §2) **[T]**: four stages, each two transistors and
  a capacitor, driven by the stage below. The bottom is a differential pair carrying the input,
  biased by the control current I_c (exponential in the cutoff voltage). The output is taken
  differentially off the top stage, and resonance is "a portion of the output ... routed to the
  other side of the input differential amplifier".
- **Huovilainen's model** **[T]**:
  - each stage dV/dt = (I_ctl/C)(tanh(V_in/2V_T) − tanh(V/2V_T));
  - Euler-discretised, it is a cascade of one-pole sections, each with a tanh;
  - oversampling is needed (the paper uses 2×), with a half-sample delay compensation in the
    feedback to keep the tuning and the resonance level right.
  - The "warmth" comes from the input level and needs no extra knob.
  - Stilson & Smith 1996 is the linear predecessor (the preserved tuning and resonance problems
    of a digitised ladder) **[S]**.
- **Already in `src/lead.cc`:** this model, its gain under cutoff 1/(1 + 4r) (−14 dB at
  self-oscillation; research/synth_leads.md §"Loudness under the cutoff"), and per-voice pair
  offsets. **The ladder needs no new work.** What matters for bass:
  - the **passband loss with emphasis**: a resonant classic ladder bass thins out, so players keep the
    emphasis moderate for bass **[M]**;
  - the **input drive**: the mixer level sets how hard the input pair is driven (tanh at 2V_T ≈
    52 mV) **[T for the tanh]**.

### 7.3 A typical bass patch **[L]**

No measured patch could be read: the manual's patch sheets ("Air Bass", "Sawyer = Bass", "Kraft
Bass", "More Bounce Bass", "Sub Bass", "Soft Rez Bass") are drawings. The common recipe:
- osc 1 sawtooth at 32' or 16', osc 2 sawtooth or square an octave up or at unison detuned
  3–10 cents, osc 3 sometimes a fifth or an octave, sometimes off;
- mixer 6–10 (into the ladder's drive);
- cutoff low (100–400 Hz), emphasis 2–5 of 10, keyboard control 1 (1/3) on;
- filter contour: attack 0, decay 150–600 ms, sustain 20–40 %, amount 2–3 octaves;
- loudness: attack 1–5 ms, decay 0.5–2 s, sustain 60–100 %, DECAY switch on for a short release;
- glide off or short; played legato, so held notes do not retrigger.

### 7.4 What differs from the present synth-bass-1

`synth-bass-1` (lead.cc:176): one core, its ramp and the octave below from a flip-flop (sub 0.7),
cutoff −12 semitones, filter envelope 1.6 × with 2 ms attack, 0.12 s decay, sustain 0.05,
resonance ×1.2, amp 2 ms / 0.6 s / 0.7, release ×0.4. Against a classic monosynth:
- **The octave below is phase-locked** (a divider) where the classic monosynth's oscillators are free and
  beat. The slow phase walk of three VCOs, each with its own drift, is the most recognisable
  thing about a classic monosynth bass **[M]**.
- **No mixer stage:** the summed oscillators should meet a saturating stage (the mixer's own
  overload, plus the optional output-to-input loop) before the ladder's input pair.
- **Triggering:** legato notes should not restart the contours; glide between them.
- **Contour shapes:** RC charges, as now. Times as §7.3; the decay switch is the release.
- **Keyboard tracking** of the cutoff, 1/3 by default.

## 8. The FM synth "E.BASS 1"

### 8.1 Where the data come from

- Factory banks downloaded from yamahablackboxes.com and parsed here with the documented 32-voice
  packed layout (128 bytes per voice, op6 first) **[T]**. The parser reproduces electric_pianos.md's
  table for ROM1A "E.PIANO 1" exactly, which checks the field decoding.
- "E.BASS 1" is **ROM3A voice 12**. Its 118 parameter bytes are **identical to ROM1A voice 15
  "BASS 1"** **[T]**: ROM3 repackages ROM1's voices by category. Also present: ROM1A 16 "BASS 2",
  ROM1B 31–32 "BASS 3/4", ROM2B 15–16 "SYN-BASS 1/2", ROM3A 18 "FRETLESS 1" (the same as
  "BASS 2" but for op2/op1 rates), ROM3B 30 "E.BASS 2", ROM3B 32 "PLUCK BASS".

### 8.2 The patch

- **Global:** algorithm **16**, feedback **7** (maximum), oscillator key sync **on** (every note
  starts its operators at phase 0, so every attack is the same), transpose value 12 (**down one
  octave**), pitch EG flat (all levels 50), LFO speed 35, delay 0, PMD 0, AMD 0, triangle, PMS 3
  (no vibrato unless the mod wheel adds it). All detunes 0, all ratios (no fixed frequencies).
- **Algorithm 16** (from Dexed's table, checked against the flags) **[T]**:

```
        op4          op6 ⟲ (feedback)
         |            |
  op2   op3          op5
     \   |   /
       op1  →  out
```

| Op | Role | Ratio (× key) | × f₀ heard | Out level | Vel. sens | Rate scal. | EG rates R1–R4 | EG levels L1–L4 | Level scaling |
|---|---|---|---|---|---|---|---|---|---|
| 1 | carrier | 0.50 | 1 | 99 | **0** | 7 | 95 62 17 58 | 99 95 32 0 | BP 36, L 57 +LIN, R 14 −LIN |
| 2 | mod of 1 | 0.50 | 1 | 80 | 0 | 7 | 99 20 0 0 | 99 0 0 0 | (BP 41, depths 0) |
| 3 | mod of 1 | 0.50 | 1 | 99 | 3 | 6 | 88 96 32 30 | 79 65 0 0 | none |
| 4 | mod of 3 | 5.00 | **10** | 93 | 5 | 5 | 90 42 7 55 | 90 30 0 0 | none |
| 5 | mod of 1 | 0.50 | 1 | 62 | 3 | 7 | 99 0 0 0 | 99 0 0 0 | **BP 52, L 75 −LIN** |
| 6 | mod of 5, feedback | 9.00 | **18** | 85 | **7** | 1 | 94 56 24 55 | 93 28 0 0 | none |

- **Pitch:** the carrier at 0.5 × the key and the octave transpose: **a key sounds two octaves
  below its name**. To sound GM note n, play key n + 24 **[E; the transpose convention is M]**.
- **Every ratio is a multiple of 0.5**, so every component lands on a harmonic of f₀: the
  spectrum is strictly harmonic at every moment, however bright **[E]**.

### 8.3 What it does, in numbers

The voice was run through a float re-implementation of Dexed's envelope, scaling and operator
code (`msfa/env.cc`, `fmsynthnote.cc`, `fm_core.cc`; 44.1 kHz; transpose applied to the scaling note)
**[E]**. Operator output g (1 = full scale) is a modulation index of 2πg rad.

- **Decay rates** at the E1 key (scaling note 40; dB/s, from the EG formula):
  - op1 (carrier): 99 → 95 at 519 dB/s, then to L3 = 32 (−50 dB) at 4 dB/s, release 389 dB/s
    (−60 dB in 0.15 s);
  - op2: 99 → 0 at 5 dB/s (a slow fade of the body's brightness);
  - op3: 79 → 65 at once, then to 0 at 16 dB/s;
  - op4: 90 → 30 at 41 dB/s;
  - op6: 93 → 28 at 113 dB/s;
  - op5: holds (rate 0).
  - Rate scaling makes all of these about 1.6× faster an octave up.
- **Peak indices** at the E1 key:

| Velocity | op2 → op1 | op3 → op1 | op4 → op3 | op6 → op5 | op5 → op1 | op6 feedback β |
|---|---|---|---|---|---|---|
| 127 | 2.4 rad | 2.9 | 5.8 | 4.1 | 0.003 | ~2.0 rad |
| 100 | 2.4 | 2.3 | 3.9 | 2.3 | 0.003 | 1.16 |
| 64 | 2.4 | 1.3 | 1.6 | 0.7 | 0.003 | 0.33 |
| 32 | 2.4 | 0.7 | 0.5 | 0.1 | 0.000 | 0.07 |

- **op5 is scaled out in the bass:** its −LIN left depth 75 below breakpoint 52 leaves an index of
  0.003 rad at the E1 key, 0.19 at E3 and 0.53 (its maximum) from about A3 up (scaling notes
  40 / 64 / 69). **The feedback pair matters only in the top of the voice's range** **[E]**. If
  the FM synth scales from the untransposed key, op5 is still under 0.03 rad at E1 **[E]**.
- **The resulting sound** (E1, 41.2 Hz; spectral centroid and the share above 1 kHz):

| Window | vel 127 | vel 100 | vel 64 | vel 32 |
|---|---|---|---|---|
| 0–10 ms | 768 Hz (18.6 f₀), −5 dB | 518 Hz (12.6 f₀), −8 dB | 120 Hz (2.9 f₀), −29 dB | 100 Hz, −37 dB |
| 10–40 ms | 564 Hz (13.7 f₀), −6 dB | 304 Hz (7.4 f₀), −10 dB | 125 Hz, −29 dB | 109 Hz, −55 dB |
| 40–100 ms | 488 Hz (11.8 f₀), −6 dB | 248 Hz (6.0 f₀), −14 dB | 96 Hz, −36 dB | 85 Hz, −63 dB |
| 100–300 ms | 218 Hz (5.3 f₀), −17 dB | 130 Hz (3.2 f₀), −26 dB | 88 Hz, −50 dB | 87 Hz, −68 dB |
| 300–600 ms | 99 Hz (2.4 f₀), −43 dB | 92 Hz (2.2 f₀), −53 dB | 90 Hz, −67 dB | 91 Hz, −68 dB |

  - The level hardly changes with velocity (the first 10 ms within 1 dB from velocity 32 to 127):
    velocity is brightness only.
  - The sustained tone is the op2 1:1 pair (index 2.4 rad, a sawtooth-like series whose upper
    harmonics fade at 5 dB/s) on a carrier that falls slowly to −50 dB.
- **Where the "slap" comes from:** op4 at 10 f₀ modulates op3 (1 f₀), so op3's output has
  sidebands at (1 ± 10n) f₀ with Bessel weights J_n(I₄). That output phase-modulates the carrier,
  giving a cluster of harmonics around 9–21 f₀ and beyond (Carson bandwidth (I₄ + 1)·10 f₀, about
  2.8 kHz at velocity 127 on E1). It dies at 41 dB/s, the index halving about every 0.15 s
  **[E]**. A real slap's attack is the string clattering on frets: broadband and partly
  inharmonic (§6.3). The DX bass imitates it with a harmonic, velocity-scaled brightness burst
  that is exactly the same on every note (key sync).
- **How feedback FM makes an attack, in general:** an operator fed its own output, y = sin(ωt +
  βy), has the series y = Σ (2/(nβ)) J_n(nβ) sin(nωt) **[M; Tomisawa's patent]**. Near β ≈ 1.5 this
  is close to a sawtooth; beyond about 2 the waveform turns chaotic and noise-like **[M]**. A
  feedback operator at a high ratio with a fast decay therefore makes a short burst of bright,
  rough partials. In E.BASS 1, op6 (18 f₀, β up to ~2 at velocity 127, −22 dB by 0.2 s) would be
  that click, but only where op5 lets it through: on the high notes. Dexed implements feedback
  as half the mean of the last two outputs shifted by (8 − feedback) bits **[T, Dexed
  `fm_op_kernel.cc`]**.
- **The FM synth hardware** around the patch: the YM21280 operator chip computes in the log domain (a
  quarter-wave log-sine ROM, then an exponential ROM), and its algorithm is a fixed routing of 6
  operators per voice **[S, Shirriff]**. The sample rate is about 49.1 kHz, and the DAC (12 bits
  with a gain stage) adds noise and grit to quiet passages **[M]**.

### 8.4 What a model of it is

The FM synth is a digital circuit, so "physical" here means an exact model of its arithmetic: six
sine operators, the algorithm table, the envelope generator with its level and rate tables,
keyboard rate and level scaling, the velocity table, feedback, and ideally the chip's log-sine
and exponential quantisation and the DAC. Dexed's `msfa` code (Apache 2.0) holds all of these
tables. It costs a few hundred operations per voice-sample; the bass is monophonic.

## 9. What a model needs

### 9.1 The electric bass (one class, P and J as specs)

1. **Strings:** four, from §2.1 (EXL170 by default, flats as a variant), B from §2.2, losses
   `loss0 + loss1·k^1.2` with loss0 from Fleischer's η (E1 ~0.001 → δ₁ ≈ 0.13 /s open,
   before the neck) and loss1 fitted so that T60 at 600 Hz is ~10–15 s and at 8 kHz ~0.4 s
   (Kemp). Fit against the IDMT single notes.
2. **Enough modes:** the slap's fret clatter reaches 4–5 kHz and beyond, so carry modes to
   ~8–10 kHz on every string: about 200–240 on an open E1, 100 on G2, fewer on stopped strings
   **[E]**. Finger and pick notes could stop at 5 kHz (120 on E1).
3. **Two polarisations,** the perpendicular one heard by the pickup; the pluck's direction sets
   the mix; a slow turn of the plane (as `Bass`).
4. **Frets as contacts:** 20 one-sided stiff contacts at the fret positions (crowns ~1 mm over
   the board), the string's height over each from the action (bass side ~2.5 mm at the 12th fret
   **[M]**) and the neck relief. Active only while the string is near them (the first ~90 ms of
   a slap; the loudest plucks). Penalty springs as `Bass::kBoard`, but much stiffer, and tested
   for stability. A stopped string's speaking part ends on its fret; the part behind the
   stopping finger is damped.
5. **The neck:** 3–4 modes (≈ 40, 150, 290, 520 Hz) coupled at the stopping fret (or nut), with a
   conductance landscape that makes 1–3 dead spots per string. Bridge: nearly rigid
   (≤ 0.3 ms/kg). A real instrument's dead spots are its own: vary per instance.
6. **Pickups:** velocity taps at the positions of §3.1, each with a Gaussian aperture (10–20 mm),
   the perpendicular polarisation, and the magnet's distance nonlinearity (clavi code). Then
   per coil the L–R–C of §3.2, the pots, tone network and cable of §3.3 as an exact passive
   filter (`tools/clavtone.py` method). P: two half-coils in series, each under two strings. J:
   two coils, each its volume pot, summed in parallel.
7. **The player:** a fingering solver as the guitar's (4 strings; open strings preferred low; the
   hand position; slides between near notes). Techniques:
   - finger (two fingers alternating, rest stroke, 6–25 cm);
   - pick (narrow, fast, the click, alternate strokes, optional mute pad);
   - thumb (a velocity pulse ~2–3 cm wide at ~22–27 cm, with the thumb as a returning contact);
   - pop (a large outward pull of a D or G string at ~20 cm, released);
   - ghost notes; left-hand note-off damping; squeak on shifts (roundwounds).
   Velocity → depth, and for slap → how far the string goes into the frets.
8. **Output:** the DI signal (pickups to a 1 MΩ input). An amp/cabinet stage is optional; a
   compressor for Slap Bass 2 (§6.4).

### 9.2 The classic monosynth bass

- Three VCO cores per voice from `lead.cc` (each its own parts, drift and temperature), each with
  the classic monosynth waveforms (triangle-saw hybrid, saw, square, wide and narrow pulse) and range
  switches.
- A mixer stage that saturates (and the optional output → external input loop), then the
  existing ladder with keyboard tracking 1/3.
- Contours with the classic monosynth ranges, single triggering and low-note priority, glide.
- A patch per §7.3.

### 9.3 The FM synth E.BASS 1

- An FM voice: 6 operators, the 32 algorithms (Dexed's flag table), feedback, the EG and scaling
  tables, velocity table, oscillator key sync, transpose. Load the 155 voice parameters from the
  sysex (the instrument could then play any FM synth voice; E.PIANO 1 for GM 6 would come free).
- Optional fidelity: the log-sine/exp quantisation (Shirriff), 49.096 kHz internal rate and its
  resampling, the DAC.
- Check against Dexed's output for the same patch, note and velocity (spectra within a dB).

### 9.4 What a listener would notice if it were wrong

1. **Notes that all decay alike.** Real basses have dead spots: a few notes (often G string, 5th–7th
   fret) die in a second while their neighbours sing.
2. **The fundamental decaying as fast as the overtones,** or the tone not darkening as it
   decays (§2.3: each octave of partial decays twice as fast).
3. **High notes on the E string sounding as pure as on the G:** they should be audibly stiff
   (50–80 cents at the 10th partial at the 12th fret).
4. **A P that sounds like a J,** or a J with both pickups that is not scooped: the pickup
   positions are the instrument's identity.
5. **Too much top:** a bass through a 2.3–4.4 kHz pickup resonance has little above 5 kHz except
   the slap's and pick's transients. Too little: a dull pick with no click.
6. **A slap that is just a bright pluck:** without the fret contact in the first ~90 ms there is
   no clank, no 4–5 kHz burst, and the pop doesn't snap.
7. **Every slap the same:** the thumb's rebound, the hand's force and the fret clatter vary note
   to note; a DX slap does not (but that is the FM synth's character, and it must stay identical).
8. **Notes not stopped:** a bassist damps nearly every note at its end; a ringing tail under the
   next note sounds like a synth.
9. **A classic monosynth bass with an octave locked in phase:** no beating, no motion. Or a filter whose
   bass thins with emphasis left at a high setting.
10. **Legato lines whose envelope retriggers** on every note (the classic monosynth doesn't).
11. **The DX bass an octave off** (the patch sounds two octaves below its key), or louder with
    velocity (it shouldn't be: only brighter).

## 10. GM mapping

| GM (from 1) | Name | Proposed instrument |
|---|---|---|
| 34 | Electric Bass (finger) | two-pickup bass, fingerstyle, both pickups (neck a little stronger), roundwounds |
| 35 | Electric Bass (pick) | split-coil bass, pick, roundwounds, tone open |
| 36 | Fretless Bass | **stays on the double bass** (the user's decision); a fretless J only on request |
| 37 | Slap Bass 1 | two-pickup bass, both pickups full, thumb and pop by register |
| 38 | Slap Bass 2 | as 37 with a compressor and brighter pops (§6.4; to be judged by ear) |
| 39 | Synth Bass 1 | classic monosynth-style: today's `synth-bass-1` upgraded (§7.4) |
| 40 | Synth Bass 2 | keep today's patch; the FM synth E.BASS 1 as its own `-inst` (or as GM 40 if the user prefers) |

## 11. Proposed build order

1. **The electric bass core and GM 34 (J, fingerstyle).** A new class on `Guitar`'s pattern, the
   string table of §2.1, losses fitted to the IDMT finger notes, the J pickups and passive
   network (clavi code). The most notes in the user's files; no contact solver needed for
   moderate plucks. Also takes the bass lines on guitar programs.
2. **GM 35 (P, pick).** A new spec (split coil), the pick excitation and click, the foam/palm mute.
   Small once step 1 exists.
3. **The FM synth E.BASS 1.** Self-contained, exactly specified, and checkable against Dexed. A day or
   two, and the FM engine then also offers GM 6 (E.PIANO 1). It can be done in parallel with
   step 1.
4. **The classic monosynth upgrade of `synth-bass-1`.** Three free cores, the mixer drive, single
   triggering and glide, inside `lead.cc`.
5. **Slap (GM 37, 38).** The fret contacts, more modes, the thumb as a returning contact, the pop,
   the player's thumb/pop choice. The hardest and costliest part: test stability and cost (20
   contacts × ~200 modes while contacts are active), fit to the IDMT ST and SP notes, judge
   Slap 1 against Slap 2 with the user.
6. **Neck dead spots and the fretless J** (only if wanted).

## 12. Not found, or uncertain

- **Measured T60 tables for a real American amp P or J** (per string and fret). Fleischer has T30 per fret
  for two basses, but only as plots. The IDMT single notes would let us measure it.
- **Core diameters** of the EXL170 strings, hence measured B for E, A, D (only G is measured).
- **Measured loaded responses of P and J pickups** (with a real cable and pots); the resonances
  in §3.3 are computed from makers' L and an assumed C. Zollner's *Physik der Elektrogitarre*
  probably has measurements but was not opened.
- **A primary description of GM Slap Bass 1 against Slap Bass 2.** Only one snippet (Soundhouse:
  Slap 2 "more compressed, squashed").
- **Any measurement of the fretless "mwah"**, or of finger squeak and fret noise on a bass.
- **The classic monosynth manual's bass patch charts** (drawings only), and measured classic monosynth contour
  curves, oscillator waveshapes and mixer overload transfer curves. The original (1970s) classic monosynth
  service data were not read; the reissue's specifications were.
- **Pluck positions and depths** for fingerstyle, pick and slap: only lore (§6). The motion
  capture literature found is for the guitar.
- **Kramer et al., ICASSP 2012** (the 11-technique waveguide bass) was not opened. Its follow-up
  [Abeßer13] was.
- **Whether the FM synth applies its transpose before keyboard rate and level scaling.** The run above
  assumes it does. Either way op5 stays near silent on bass notes.

## 13. References

- **[Fleischer I]** H. Fleischer, *Dead Spots of Electric Basses. I. Structural Vibrations*, 2nd
  ed., Beiträge zur Vibro- und Psychoakustik 2/99, UniBw München.
  https://www.unibw.de/lrt4/veroeffentlichungen/1999_2_basses_i-vibrations.pdf/@@download/file/1999_2_Basses_I.%20Vibrations.pdf
- **[Fleischer II]** H. Fleischer, *Dead Spots of Electric Basses. II. Diagnosis*, Beiträge zur
  Vibro- und Psychoakustik 1/2000 (damping theory §7, string radii and masses Tab. III–V, decay
  measurements §8).
  https://www.unibw.de/lrt4/veroeffentlichungen/2000_1_basses-_ii-diagnosis.pdf/@@download/file/2000_1_Basses._II.%20Diagnosis.pdf
- **[Kemp20]** J. A. Kemp, "On inharmonicity in bass guitar strings with application to tapered
  and lumped constructions", SN Applied Sciences 2:636 (2020).
  https://research-repository.st-andrews.ac.uk/bitstream/handle/10023/19673/Kemp_2020_SNAS_Inharmonicity_CC.pdf
- **[Issanchou18]** C. Issanchou, V. Acary, F. Pérignon, C. Touzé, J.-L. Le Carrou, "Nonsmooth
  contact dynamics for the numerical simulation of collisions in musical string instruments",
  JASA 143(5), 3195 (2018): the electric bass G string (Tab. IV, V), 20 frets.
  https://perso.ensta-paris.fr/~touze/PDF/2018_NonSmoothString_JASA.pdf ; abstract of the
  companion ISMA 2017 paper: https://perso.ensta.fr/~touze/PDF/ISMA_2017_paper_15.pdf
- **[Rank97]** E. Rank, G. Kubin, "A waveguide model for slapbass synthesis", ICASSP 1997.
  https://www2.spsc.tugraz.at/people/franklyn/ICASSP97/pdf/author/ic970443.pdf
- **[Abeßer13]** J. Abeßer, P. Kramer, C. Dittmar, G. Schuller, "Parametric audio coding of bass
  guitar recordings using a tuned physical modeling algorithm", DAFx-13.
  https://dafx.de/paper-archive/2013/papers/24.dafx2013_submission_9.pdf ; datasets:
  https://idmt.fraunhofer.de/en/publications/datasets/bass.html
- **[Kramer12]** P. Kramer, J. Abeßer, C. Dittmar, G. Schuller, "A digital waveguide model of the
  electric bass guitar including different playing techniques", ICASSP 2012 (not opened).
  https://publica.fraunhofer.de/handle/publica/378203
- **[pickup-position-db]** Players' database of pickup positions (cm from the 12th fret).
  https://agschaid.github.io/pickup-position-db
- **[Bartolini]** "If I am building my own instrument how do I position the pickups?"
  https://contact.bartolini.net/support/solutions/articles/5000552591
- **Fender pickup specifications** (retail and Fender part pages, snippets):
  https://fender.com/en-US/parts/pickups-preamps/original-precision-bass-pickups/0992046000.html ;
  https://fender.com/it/accessori/pickup/pure-vintage-63-precision-bass-pickup/0992241000.html ;
  https://www.zzounds.com/item--FEN0992375 (Cobalt Chrome) ;
  https://fender.com/encessories/pickups/yosemite-pj-pickup-set/0992281000.html
- **Unloaded resonances (forum, snippet):** https://music-electronics-forum.com/forum/instrumentation/pickup-makers/31970-4-conductor-hook-up-cable-capacitance-effects-on-pickup-resonant-frequency
- **Wiring kits:** https://www.stewmac.com/electronics/components-and-parts/premium-wiring-kit-for-jazz-bass ;
  https://www.stewmac.com/electronics/components-and-parts/premium-wiring-kit-for-p-bass ;
  https://www.lollarguitars.com/j-bass-pre-wired-kit
- **[HdM slides]** Tonabnehmer (two poles per string on bass). https://curdt.home.hdm-stuttgart.de/PDF/Tonabnehmer.pdf
- **String tension:** D'Addario EXL170 product page (E 34.7 lb),
  https://www.daddario.com/en-eu/products/exl170-xl-nickel-wound-bass-guitar-strings-light-45-100-long-scale ;
  retailer's copy of the chart (kg),
  https://nantelmusic.com/products/daddario-6-string-32-130-long-scale-bass-strings-light-exl170-6
- **Fretless (lore):** https://www.talkbass.com/threads/the-physics-of-mwah.1349750/
- **GM slap (snippet):** https://www.soundhouse.co.jp/en/contents/column/index?post=2827 ;
  GS variations: https://battleofthebits.com/lyceum/View/Specification+of+General+MIDI+and+Roland+MT-32+patches
- **[Moog]** Moog Music, *Minimoog Model D* manual (2016 reissue), mixer, filter, contours,
  specifications pp. 26–31, 80–81.
  https://cdn.inmusicbrands.com/Moog/Model%20D/Minimoog_Model_D_Manual.pdf
- **[Huovilainen04]** A. Huovilainen, "Non-linear digital implementation of the Moog ladder
  filter", DAFx-04. https://dafx.de/paper-archive/2004/P_061.PDF
- **[StilsonSmith96]** T. Stilson, J. O. Smith, "Analyzing the Moog VCF with considerations for
  digital implementation", ICMC 1996 (not opened). https://ccrma.stanford.edu/~stilti/papers/Welcome.html
- **[Moog patent]** R. A. Moog, US 3,475,623, "Electronic high-pass and low-pass filters employing
  the base to emitter diode resistance of bipolar transistors" (1969) **[M]**.
- **[DX7 ROM]** Yamaha DX7 factory banks: https://yamahablackboxes.com/patches/dx7/factory/rom1a.syx ,
  …/rom3a.syx (and rom1b–rom4b). E.BASS 1 = rom3a voice 12 = rom1a voice 15.
- **[Dexed]** Dexed / msfa source (envelope, scaling, algorithms, feedback):
  https://github.com/asb2m10/dexed/tree/master/source/msfa (`env.cc`, `dx7note.cc`, `fm_core.cc`,
  `fm_op_kernel.cc`).
- **[Shirriff]** K. Shirriff, the DX7 YM21280 chip series: https://www.righto.com/2021/11/reverse-engineering-yamaha-dx7.html ;
  https://www.righto.com/2021/12/yamaha-dx7-reverse-engineering-part-iii.html (log-sine ROM);
  https://www.righto.com/2021/11/reverse-engineering-yamaha-dx7_28.html (exponential);
  https://www.righto.com/2021/12/yamaha-dx7-chip-reverse-engineering.html (algorithms).
- **[guitars.md]** `research/guitars.md` (PLC14, pickups, pluck table, the user's answers of
  2026-11-16); **[double_bass.md]**; **[electric_pianos.md]** §6 (the E.PIANO 1 table);
  **[synth_leads.md]**.
