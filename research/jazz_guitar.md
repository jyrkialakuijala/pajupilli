# Jazz guitar (GM 27, "Electric Guitar (jazz)"): research

Status: research 2026-11-19; built 2026-11-20 (section 12, as built). This goes deeper than `research/electric_guitars.md`
§2.1, §4.1, §5, §6, §12–13 (the sketch) and does not repeat it. It builds on what is in the code:
`src/guitar.h/.cc` (`GuitarSpec`, `electricGuitar()`: the Strat and the Les Paul, two-coil
humbuckers, the pick, the palm, the pickups' comb, aperture and magnet nonlinearity),
`src/guitaramp.h/.cc` (the amp module: the pickup's L–R–C input, compressor, triode stages, the
Yeh–Smith stack, the power stage, cabinets as biquads; presets twin, twinComp, jcm800, slo) and
`src/guitarinst.h/.cc` (the guitarist).

How each number is marked (as in `research/electric_guitars.md`):
- **[T]** read in the primary text (downloaded and read for this document); "measured" is said in
  words where the text reports a measurement.
- **[S]** secondary: an abstract, a search snippet, a retailer's or forum's copy of a number.
- **[L]** player or maker lore.
- **[E]** my estimate or arithmetic (the scripts are described where used).
- **[M]** general knowledge, not looked up.
- **[F]** measured here from files: the user's MIDI file, or FluidR3 rendered with TiMidity.

References are short keys in brackets; the list is section 11.

**Short answer.** The jazz guitar is the electric guitar already built with five things changed.
(1) **Strings:** flatwounds 12–52 at 95–142 N each, ~680 N in all at 628 mm. That is 1.6 times the
Strat set's tension. The wound strings are as dense as a solid steel rod of their diameter (fill
0.9–1.0, against 0.8 for roundwounds), and the G is wound. The plain-G set's most inharmonic
string becomes one of the least inharmonic: B drops from ~1.2e-4 to ~1e-5 **[E from T/S
tensions]**. (2) **Body:** a hollow archtop with a floating bridge. Measured on a hollow body with
a floating bridge and flatwounds through the neck pickup, the open low E's fundamental took 5.5 s
to fall 15 dB, but the 3rd–5th harmonics took 0.9–1.1 s (T60 ≈ 22 s against 3.4–4.5 s). The neck's
three strong modes are at 85, 175 and 332 Hz **[T, measured, Rau17]**. That is the jazz note: a
long, round fundamental and a short upper spectrum. (3) **Pickup:** the neck humbucker, 160–180 mm
from the saddle, so the comb's first notch falls at the 3.5th–4th harmonic of an open string.
(4) **Tone control:** with the usual 500 kΩ audio-taper pot and 22 nF, "3 of 10" puts ~15 kΩ in
series with the capacitor. That gives a plain low-pass, −3 dB at ~0.7 kHz and −12 dB at 2 kHz,
with no resonant peak. "4–5 of 10" (~30–45 kΩ) gives −3 dB at 1.2–1.9 kHz **[E, computed]**. The
"woman tone" peak exists only at 0. (5) **Amp:** clean, mids up, treble down; a closed 1×15 or an
open 1×12. On top of that, a small acoustic share (−15 to −25 dB) and the top's motion heard by the
top-mounted pickup itself (microphonic). The guitarist is the larger change. In the user's one file
(louisiana ch7), 90 % of the chords are directly playable. A reduction that never drops the top
note (doubled pitch classes first, then the fifth, then the bass under a wide gap, then inner
notes) makes all the rest playable while dropping 5 % of the notes **[F]**. The chords are short
(median 0.33 beat) and soft (median velocity 45); 10 % of the notes have zero length. They want a
fast down-rake, a left hand that chokes within ~30 ms, and a ghost stroke for the zero-length
notes. FluidR3's "Jazz Guitar" gives the targets: a 20–21 dB velocity range, a centroid of 0.6–0.8
kHz soft and 1.4–1.5 kHz loud in the middle register, and mid and high notes falling 13–28 dB/s over
the first half second **[F]**.

---

## 1. What is built, and what GM 27 still lacks

| Need | Built | Missing |
|---|---|---|
| Steel strings, modes, two polarisations, tilt | `GuitarString`, `GuitarSpec` | a flatwound table (section 3); `loss1·k^1.2` may want a steeper exponent (3.5) |
| A body under the bridge that radiates | `GuitarSpec::body` (the classical's 28 modes; the electric's two neck modes, `radiate = 0`) | an archtop table (2.3); **an electric that also radiates**: `Guitar::sample()` returns only the EMF when `electric()` and discards `out` |
| Pickups: position, aperture, magnet, two coils | `GuitarSpec::Pickup` (`apart` = 0.018 for humbuckers) | the pickup **moving with the top** (2.5) |
| The coil and its load | `GuitarAmp::Source{L, R, C, pots}` → a 2nd-order input biquad | **the tone pot and capacitor**: a resistor plus a capacitor across the pickup makes the input 3rd order (4.3) |
| An amp | `twin()`; cabinets 0 (2×12 open), 1, 2 | a jazz preset; a 1×15 closed or 1×12 open cabinet (5) |
| The pick | `Guitar::pick(s, depth, at, angle, rise)` | a heavy, round-edged pick (rise ~1.5–2 ms, a wider contact) |
| The thumb | `Guitar::pluckPad(...)` (the bass player's finger) | used as Wes's thumb (6.6) |
| Chords | greedy string choice; `asked_.resize(6)` keeps the **lowest** six | a voicing reducer that never drops the top note (6.3); no open strings in comping; a fast choke on release |

---

## 2. The archtop

### 2.1 The three guitars

| | Gibson ES-175 | Gibson L-5 CES / Super 400 CES | Gibson ES-335 (for comparison) |
|---|---|---|---|
| Body | 16" lower bout, ~3 3/8" (86 mm) deep, **laminated** maple top, back and sides; pressed arch **[M]** | 17" / 18", **carved** solid spruce top, carved maple back **[M]** | 16", ~1 5/8" thin, laminated maple, **solid maple centre block** under the bridge and pickups **[M]** |
| Scale | 24.75" (628 mm), 20 frets **[M]** | 25.5" (648 mm) **[M]** | 24.75", 22 frets **[M]** |
| Bridge | floating: a rosewood base on two feet, held by the strings' down-force (later with a tune-o-matic on top) **[M]** | floating ebony **[M]** | tune-o-matic on posts in the block, stopbar tailpiece **[M]** |
| Tailpiece | trapeze, to the end pin **[M]** | hinged trapeze **[M]** | stud (stopbar) **[M]** |
| Pickups | P-90 (1949), PAF humbuckers from 1957 **[M]**, mounted in the top | two humbuckers in the top (Charlie Christian bar pickup on early L-5 CES) **[M]** | two humbuckers |
| What it does to the string | the bridge sits on a top that gives: the string loses energy into it quickly | the same, a stiffer and lighter top that radiates more | like a solid body: the bridge is on the block |

GM 27's usual reading is the ES-175 **[L; electric_guitars.md §2.1]**. It has the laminated top,
which radiates less than a carved one and feeds back less. Its 24.75" scale also puts the
flatwounds 6 % slacker than on the L-5's 25.5".

### 2.2 What is known about the archtop's modes

- **No measured mode table of an assembled archtop was found** (section 10). What exists:
  - **A hollow-body electric with a floating bridge and flatwound strings, measured (laser
    vibrometer, impact hammer) [T, Rau17; the same in Rau's thesis, RauPhD §5.3]:**
    - Three modes of high amplitude at **85, 175 and 332 Hz**, seen at the bridge and along the
      neck, "likely caused by the neck bending modes". The 175 and 332 Hz modes weaken towards
      the bridge end of the neck.
    - The neck's losses are comparable to the bridge's at some frequencies: "losses at the neck
      should not be neglected".
    - The open low E through the neck pickup (tone and volume full up), the time for each of the
      first five harmonics to fall 15 dB: **5.45, 4.49, 0.86, 0.87, 1.12 s** (measured). Without
      body coupling, reflectances predicted 13.3, 0.68, 0.24, 0.33, 0.33 s; the authors conclude
      the boundaries are not stateless reflectances.
    - The low E string's characteristic impedance (from D'Addario data): 0.87 N·s/m.
    - The guitar's make is not named.
  - **Free archtop top plates (FEM from a 3-D scan of a real archtop) [S, SGAS21 abstract]:**
    the mode shapes are compared with the violin's and the classical guitar's; the study covers
    the carving of the inner surface and the f-holes. The full text is paywalled: no frequencies.
  - **A Gibson L-5-type archtop, a parametric FEM from a laser scan [S, LGDNA25 abstract]:** the
    top's thickness changes eigenfrequencies and radiation more than its bracing does (symmetric
    or asymmetric). No numbers seen.
  - **The air resonance:** luthiers report the archtop's "main air" at about A#2–C#3 (116–138 Hz),
    against F#2–G#2 (92–104 Hz) for flat-tops; one tap tone at 138 Hz; free carved tops at 180–350
    Hz **[S, luthiers' forums (MIMF, jazzguitar.be), via search snippets]**.
  - **Flat-tops for scale:** the lowest assembled mode is a "beam" mode at 70–75 Hz **[S,
    Richardson]**; the air and top modes are near 100 and 200 Hz **[S, Tufts thesis]**.
- **Plate tuning:** Benedetto's book describes how the tap tone changes during building but gives
  no target **[S, forum quote]**. The violin makers' Hutchins method (tuning the free plate's
  modes 2 and 5 an octave apart) has a computational echo for violin plates in the same Milan group
  (a mode 5/mode 2 ratio as the Cremonese makers had it) **[S]**. None of this reaches a
  frequency for an archtop guitar.

### 2.3 A mode table to use [E]

From the above: Rau's neck modes, the luthiers' air resonance, a top mode where a flat-top's sits
but a little higher (the arch makes the top stiffer for its mass **[M]**), and the classical spec's
pattern for the rest. The masses are heavier than the classical's: a laminated 3-ply maple top is
denser, and the archtop's bridge takes 120–180 N of down-force **[E: ~680 N over a ~10–15° break
angle]**.

| Mode | f (Hz) | Q | mass (kg) | rad | along | Note |
|---|---|---|---|---|---|---|
| neck 1 | 85 | 25 | 0.25 | 0 | 0.1 | [T, Rau17] frequency |
| A0 (air through the f-holes) | 125 | 20 | 1.2 | 0.6 | 0.05 | [S] 116–138 |
| neck 2 | 175 | 28 | 0.35 | 0 | 0.1 | [T] |
| T(1,1) top | 240 | 20 | 0.45 | 1.0 | 0.3 | [E]: the arch; laminated tops lower Q |
| neck 3 / back | 332 | 30 | 0.5 | 0.2 | −0.2 | [T] |
| top, cross-dipole | 420 | 25 | 0.8 | 0.35 | 0.6 | [E]: the bridge rocking on its two feet |
| plates | 500–2500, ~15 modes, ×1.11 | 25–45 | 1–3 | ±(0.3→0.05) | ±0.3 | the classical spec's pattern, lighter in the sound |

- **The bridge rocks:** a floating bridge on two feet turns the strings' motion along the top into
  a rocking of the bridge, as on a violin **[M]**. So the archtop's `along` shares are larger than
  a flat-top's, notably for the cross-dipole mode **[E]**.
- **The pickup hears the vertical motion, which the top drains:** the string's swing towards the
  top loses energy into the floating bridge faster than its swing along it. What the pickup hears
  is a two-stage decay: the attack and its "punch" first, then the along-swing coupling back
  through the tilt **[E; the mechanism in guitar.h]**. Set `towardsLoss` 1.5–2 for the archtop,
  against the Strat's 1.15 **[E]**.
- **"Punchy and short":** archtops are known for a strong initial attack that decays quickly,
  against a flat-top's sustain **[S, patent US 9905212's background]**.
- **ES-175 against L-5:** for the laminated top, all `rad` ×0.5 and Q ×0.8 (laminate damping)
  **[E]**. For the ES-335, the solid spec with the Les Paul's losses, two weak wing modes (~220,
  ~380 Hz, rad 0.1) and an acoustic share of −30 dB **[E]**.

### 2.4 The tailpiece and the afterlength

- The trapeze tailpiece leaves 80–120 mm of string between the bridge and the tailpiece **[E]**.
  Those short lengths ring at f_open × L/afterlength: the low E's at ~520 Hz, the high E's at ~2
  kHz (at 100 mm) **[E]**. Struck through the bridge, they add a faint bell-like ring to a
  comping chord. They are far from the pickup (which does not hear them) and audible only
  acoustically **[E, L]**. They are a refinement: six 1-mode resonators on the bridge, radiation
  only.
- The tailpiece itself has low rocking modes (a violin's at 100–200 Hz **[M]**). None measured
  for a guitar.

### 2.5 How much of the acoustic is in the sound

- **No measurement** of the acoustic-to-pickup level ratio was found.
- **Unplugged,** an ES-175 is about as loud as a quiet flat-top; a carved L-5 is louder (it was an
  acoustic orchestra guitar first) **[L]**.
- **Amplified on a gig,** the amp is 15–25 dB above the guitar's own sound at the player's and the
  microphone's position **[E]**. Recordings often mic the amp alone, sometimes the guitar too
  **[L]**.
- **Through the pickup:** a pickup mounted in the top (ES-175, L-5 CES) moves with the top. The
  EMF follows the gap between pole and string, so the top's motion at the pickup changes the gap
  for all six strings, sounding or not: the pickup becomes a body sensor **[E: physics of
  guitar.cc's `field(gap − y)`]**.
  - Size: a string at the pickup moves at ~0.1–1 m/s after a pluck **[E: 0.2–2 mm at 80–330
    Hz]**. The top at the pickup, at a resonance, moves at ~0.003–0.03 m/s **[E: bridge
    admittance 0.01–0.05 m/(N·s) at a resonance, a ~1 N string force, the pickup at 0.3–0.6 of
    the bridge's motion]**. That puts the body 30–40 dB under the string, but at its resonances
    and with every string's steel contributing.
  - This is the "air" an archtop has through the amp that a solid body lacks **[L for the
    sound]**.
  - A **floating** pickup (DeArmond, Johnny Smith, Benedetto: hung from the fingerboard or the
    guard, off the top) does not hear the top this way **[M]**: a purer, more electric sound.
  - Unpotted vintage coils are microphonic themselves (a tap on the cover is heard) **[M]**.
- **Mix to use [E]:** the radiated sound (`out`, with `radiate` as for the classical guitar) at −20
  dB under the amp's output for the ES-175 (−14 dB for an L-5). Through the pickup, a `mount`
  share of 0.4 of the bridge's displacement for the top-mounted humbucker.
- **Feedback:** a hollow archtop feeds back at its lowest modes (100–300 Hz) **[L; electric_guitars.md
  §5]**. Jazz players keep the bass and the volume moderate. GM 27 does not need it.

---

## 3. Flatwound strings

### 3.1 Tensions

**Thomastik-Infeld Jazz Swing** (nickel flat ribbon on a round core; "a true flatwound, not a
ground wound or compressed wound") **[T, TI catalogue 2013]**. The scale is not stated; the plain
strings agree with steel at 648 mm (fill 0.97, below) **[E]**.

| Set | E4 | B3 | G3 (wound) | D3 | A2 | E2 | Total |
|---|---|---|---|---|---|---|---|
| JS110 | .010 / 7.8 kp | .014 / 8.3 | .018 / 8.6 | .023 / 8.2 | .033 / 8.2 | .044 / 8.5 | 49.6 kp |
| JS111 | .011 / 9.0 | .015 / 9.3 | .019 / 9.8 | .025 / 9.7 | .035 / 9.2 | .047 / 9.6 | 56.6 |
| **JS112** | .012 / 10.3 | .016 / 10.3 | .020 / 11.2 | .027 / 10.9 | .037 / 10.3 | .050 / 10.6 | 63.6 |
| JS113 | .013 / 11.7 | .017 / 11.4 | .021 / 12.4 | .028 / 12.4 | .039 / 11.3 | .053 / 11.8 | 71.0 |

(1 kp = 9.81 N.) Thomastik's sets are deliberately even in tension across the strings
**[T, the table]**. The **George Benson** sets (same catalogue: GB112 flat, GR112 round) are 12–53
or 14–55 with the heavier strings at 11.8–14.2 kp. The **Bebop** sets (BB) are roundwound on the
same construction, "somewhat brighter than a flatwound … a bit more of an edge"; they use a
**plain G** (.019–.020) in the 11–12 sets and a wound .021–.022 in the 13–14 sets **[T]**.

**D'Addario Chromes** (stainless steel flat ribbon) **[S, official chart via search snippet; at
25.5"]**:

| Set | E4 | B3 | G3 (w) | D3 | A2 | E2 |
|---|---|---|---|---|---|---|
| ECG24 11–50 | .011 / 19.6 lb | .015 / 20.5 | .022 / 25.3 | .030 / 24.6 | .040 / 25.8 | .050 / 21.5 | (retailer's table) |
| ECG25 12–52 | .012 / 23.4 | .016 / 23.3 | .024 / **34.0** | .032 / 30.2 | .042 / 28.5 | .052 / 23.7 |
| ECG26 13–56 | .013 / 27.4 | .017 / 26.3 | .026 / 39.8 | .035 / 35.9 | .045 / 33.3 | .056 / 27.6 |

Retailers list ECG25's G as 30.0 lb and its D/A/E as 27.9/28.0/22.7, and ECG26's wound strings as
35.1/35.1/32.9 **[S]**. The official G looks high; the source of the disagreement is unknown.

**Converted, and what they imply [E; `str.py` in the scratchpad]:** linear density μ = T/(2Lf)².
"Fill" is μ over a solid steel rod of the nominal diameter (7850 kg/m³). Tensions at 628 mm are
×(628/647.7)² = 0.94. B is computed from the core alone (an assumed core of 0.42 × the diameter);
it is a lower bound for flats (below).

| | E4 | B3 | G3 | D3 | A2 | E2 | Total |
|---|---|---|---|---|---|---|---|
| ECG25 at 628 mm (N) | 98 | 98 | 142 | 127 | 119 | 99 | 683 |
| TI JS112 at 628 mm (N) | 96 | 95 | 103 | 101 | 96 | 98 | 588 |
| EXL110 round 10–46 at 628 (N), for scale | 68 | 65 | 70 (plain) | 77 | 80 | 71 | 429 |
| Fill, ECG25 / JS112 / EXL110 | 1.00 / 0.97 / 0.99 | 0.99 / 0.97 / 1.00 | 1.02 / 1.07 / 1.00 (plain) | 0.91 / 1.02 / 0.84 | 0.89 / 0.92 / 0.81 | 0.86 / 0.91 / 0.78 |
| B, JS112 (core only) | 2.2e-5 | 7.0e-5 | **4.9e-6** | 1.7e-5 | 6.2e-5 | 2.0e-4 |
| B, EXL110 | 1.5e-5 | 4.5e-5 | **1.2e-4** | 1.9e-5 | 6.7e-5 | 2.0e-4 |

- **Flatwounds are about 10–20 % denser for their diameter than roundwounds:** the flat ribbon
  leaves no gaps, and a wound G or D is nearly a solid rod (fill ~1) **[E]**.
- **The wound G against the plain G:** the plain .017 G at 70 N has B ≈ 1.2e-4. Its 10th partial
  is ~10 cents sharp, the most of any string. A wound .020–.024 G is under 1e-5 even if the flat
  wrap doubles the bending stiffness: ~1 cent **[E]**. Jazz sets use a wound G for intonation and
  for a G that matches the D and B in colour **[L]**. A plain G as heavy as .020 would have B ≈
  1.6e-4 **[E]**.
- **The bending stiffness of a flatwound** is above the core's: the ribbon's turns touch, and part
  of the wrap carries bending **[M]**. Use B = 1.5–2 × the core-only value for the wound flats
  **[E]**; not measured (section 10).
- **Total tension** ~590–680 N on a 628 mm ES-175 (12–52), 1.4–1.6 times a 10–46 set **[E]**.
  Heavier strings need a higher action; 1.5–3 mm picks drive them **[L]**.

### 3.2 Decay and spectrum

- **No controlled measurement** comparing flatwounds with roundwounds was found. Players and
  makers agree that flats give fewer high overtones and "thump". They disagree on sustain: shorter
  and thumpy (bass guides), or not faster at all **[S/L, several]**.
- **The one measurement:** Rau17's hollow body with flatwounds (2.2 above): the fundamental and
  2nd harmonic of the open low E have T60 ≈ 22 and 18 s; the 3rd–5th have 3.4–4.5 s
  **[T, ×4 from the 15 dB times]**. Loss rates (6.91/T60): **0.32, 0.38, 2.0, 2.0, 1.5 /s**.
  - The 4th harmonic (330 Hz) sits on the 332 Hz neck mode, so part of the step is the neck's
    **[E]**. The 3rd (247 Hz) and 5th (412 Hz) are not on a measured mode, yet decay as fast. That
    points to the string itself: friction between the touching turns of the flat ribbon, growing
    steeply with frequency **[E; M for the mechanism]**.
  - The Strat spec's `loss0 + loss1·k^1.2` (0.49 + 0.06·k^1.2 on the low E) gives 0.72 and 0.90 /s
    for k = 3 and 5. The flatwound is 2–3 times lossier there and has a longer fundamental.
- **Fit to use [E]:** wound flats `loss0` ≈ 0.30 /s (low E) to 0.6 (G), and a steeper exponent:
  `loss1·k^2` with loss1 ≈ 0.08 (low E: k=2: 0.62, k=3: 1.0, k=5: 2.3 /s). Plain strings: loss0
  as the Strat's, loss1 ×1.5 (heavier gauge, the archtop's bridge). Check against Rau's five
  numbers with the archtop's body modes in place.
- **Handling noise:** flats hardly squeak under a sliding finger **[L]**. The existing squeak, if
  any is added (steel_guitar.md §5), should be ×0.1 for flats **[E]**.
- **Magnetism:** the Chromes ribbon is stainless steel; TI's is nickel **[T/S]** (pure nickel is
  ferromagnetic, but weaker than steel **[M]**). The filled section brings more metal into the
  field: `steel[]` for the wound flats ≈ 0.9 of the diameter's area (the Strat spec uses 0.6 for
  roundwounds) **[E]**.

---

## 4. The pickup and the controls

### 4.1 Which pickup

| | L (H) | R (kΩ) | Note |
|---|---|---|---|
| Gibson PAF / '57 Classic (neck) | 4.2–4.6 | 7.2–8 | 490R measured 4.2 H **[T, Lemme13]**; PAFs 2.8–4.6 H **[S]**; '57 Classic 8 k (7.2 k underwound) **[S, Gibson]**; a Seymour Duncan 59/15-type neck 5.35 H at 100 Hz **[S]** |
| P-90 (the 1949 ES-175's) | 4.9–6.8 | 7–9 | **[S, planetz/Mojotone]**; a wide coil: MDH17 model it 40 mm wide **[T]** |
| DeArmond Rhythm Chief 1000 (floating) | 5.66 | 17.0 | measured at 1 kHz, ACR 27.9 kΩ **[S, forum, measured]** |
| DeArmond Rhythm Chief 1100 (floating) | ? | 6.3–6.9 (reissue); ~15 (vintage) | rubber magnet, 85 × 25 mm **[S, Guild, forum]** |
| Johnny Smith mini humbucker (floating) | not found | — | — |

- The ES-175 with a humbucker is the reference. Use **L = 4.4 H, R = 7.8 kΩ, C = 120 pF**, two
  coils 18 mm apart (`apart` = 0.018), volts as the Les Paul's neck pickup **[E]**.
- **Position on a 628 mm scale:** the ES-175's fingerboard ends near the 20th fret, 198 mm from the
  saddle; the humbucker's cover is ~38 mm along the strings **[M]**. Its centre is ~165–175 mm
  from the saddle, the two pole rows at ~160 and ~178 mm **[E]**. A floating pickup at the
  fingerboard's end sits at ~185–195 mm **[E]**. (The Les Paul's neck humbucker in the code is at
  155 mm.)
  - The comb: the first notch at harmonic L/x = 3.5–3.9 on open strings, so the **4th harmonic is
    nearly cancelled** and the 7th–8th again. On a note fretted at the 5th fret (L = 470 mm) the
    notch moves to the 2.8th: the 3rd harmonic is weak **[E]**. With the tone rolled off this is
    the hollow, woody middle of the jazz sound.
  - The humbucker's two coils comb only at ~k = 35 (cos(kπ·18 mm/(2L))): a gentle low-pass, no
    audible notch **[E]**.
- **Height:** jazz players set the neck pickup low (3–4 mm) under heavy strings; a close pickup on
  a heavy flatwound adds magnetic pull and warble **[L]**. Use gaps 3.5–4 mm **[E]**.

### 4.2 Pots and cable

Gibson: 500 kΩ audio-taper volume and tone pots, 22 nF ("0.022 µF") tone capacitor **[M]**. Modern
wiring puts the tone pot (as a variable resistor) in series with the capacitor, across the pickup.
A 3–6 m cable is 300–600 pF **[M]**. The amp's input is 1 MΩ.

### 4.3 The tone control at "0.3", computed

The network [E; `tone.py`, exact complex arithmetic]: EMF → R + sL → node; at the node C_coil
(120 pF) + cable (600 pF), 500 kΩ (volume full up), 1 MΩ (amp), and the tone branch R_t + 1/(sC_t).
Relative to 200 Hz:

| Tone knob | R_t (audio taper: R·(10^(2x)−1)/99) | Peak | −3 dB | 1 kHz | 2 kHz | 3 kHz | 5 kHz |
|---|---|---|---|---|---|---|---|
| 1.0 | 500 k | 2.7 kHz, +6.6 dB | 4.3 kHz | +0.9 | +4.2 | +5.8 | −6.8 |
| 0.7 | 122 k | 2.2 kHz, +1.6 | 3.8 kHz | +0.5 | +1.5 | +0.3 | −8.0 |
| 0.5 | 45 k | none | **1.9 kHz** | −1.1 | −3.3 | −6.1 | −11.7 |
| **0.3** | **15 k** | none | **0.71 kHz** | −5.6 | −11.7 | −15.3 | −20.0 |
| 0.2 | 7.6 k | none | 0.66 kHz | −8.6 | −17.0 | −21.1 | −25.9 |
| 0.0 | 0 | **462 Hz, +3.8 dB** | 0.71 kHz | −10.9 | −24.4 | −31.7 | −40.7 |

- With a **linear** pot, 0.3 is 150 k: still a 2.4 kHz peak of +2.4 dB, −3 dB at 4.0 kHz. A
  "rolled back" tone is therefore a different sound on different guitars **[E]**.
- At **0** the capacitor and the coil resonate: 1/(2π√(4.4 H · 22.7 nF)) ≈ 500 Hz (the
  computed peak is at 462 Hz, +3.8 dB). That is the "woman tone" **[E; L for the name]**. At 0.2
  and above, R_t damps it away.
- **Other values:** R_t = 25 k → −3 dB at 0.96 kHz; 70 k → 3.0 kHz. A 47 nF capacitor at R_t = 45 k
  is barely different from 22 nF (−3 dB at 2.0 kHz, against 1.9) **[E]**.
- **The pickup alone (tone full up):** 300 / 600 / 1000 pF of cable put the peak at 3.6 / 2.8 /
  2.2 kHz (+8.6 / +9.7 / +10.2 dB with the tone branch removed). With 4.4 H against Lemme's 4.2 H
  measured neck humbucker, the expected peak is 2.6 kHz at 830 pF (electric_guitars.md §6.3)
  **[E]**.
- **Inductance matters less once the tone is rolled off:** at knob 0.3, L = 2.5 / 4.4 / 5.7 / 8 H
  give −3 dB at 0.99 / 0.71 / 0.61 / 0.50 kHz **[E]**.
- **What a jazz player means by "the tone at about 3":** the dark end. The sources only say "rolled
  back to ~a third" **[L]**. The defensible default is **R_t ≈ 25–45 kΩ (−3 dB at 1–1.9 kHz)**.
  Expose R_t as the knob (PJ_JAZZ_TONE in kΩ), not a 0..1 position whose meaning depends on the
  taper **[E]**.
- **For the code:** `GuitarAmp::Source` needs the tone branch, `{Rt, Ct}`. With it, the input is
  3rd order: Z(s) = 1/(s·C + G + 1/(R_t + 1/(s·C_t))). As a cascade, a biquad for the L–C
  resonance and a one-pole for the tone branch is close but not exact. The exact 3rd-order
  bilinear transform is `setTone`'s pattern in `guitaramp.cc` **[E]**.

### 4.4 The volume pot

Turned down, the volume pot puts its resistance in series. A jazz player rides it at 7–10, which
also softens the attack **[L]**. At 7 on a 500 k audio pot (~120 k in series, ~380 k to ground
**[E]**), the residual peak goes away. It is not needed for GM 27: leave it full up.

---

## 5. Jazz amps

| Amp | What it is | Jazz settings / why | Cabinet |
|---|---|---|---|
| Fender Twin Reverb / Deluxe Reverb (blackface) | tube, the AB763 stack (in the code) | bass 6–7, mid 5–7, treble 3–4, little reverb **[L]** | 2×12 / 1×12 open back |
| Polytone Mini-Brute (I–V) | solid state; Joe Pass's and Jim Hall's amp **[L]**; small, closed | warm, flat, "the" jazz sound of the 1970s–90s **[L]** | 1×12 or 1×15 closed (IV and V: 15") **[S, retail]**; III ~110 W **[S]**; the tone circuit was **not found** |
| Roland JC-120 | solid state, 2×12, BRI/treble/middle/bass per channel, chorus **[S, Roland]**; at least 8 circuit versions **[S]** | clean, flat, a little sterile; jazz players turn the chorus off **[L]** | 2×12 open back |
| Henriksen JazzAmp (110, Ten, 112) | solid state, 80–120 W, 5-band graphic EQ, reverb **[S]** | the modern jazz club amp **[L]** | 1×10 (Eminence Beta 10) + a defeatable tweeter; closed back (110) or ported (310) **[S, retail]** |

- **The blackface stack at jazz settings [E; `stack.py`, the code's own coefficients]:**

| Treble / mid / bass | 60 Hz | 100 | 200 | 400 | 800 | 1.6 k | 3.2 k | 6.4 k (dB) |
|---|---|---|---|---|---|---|---|---|
| 0.6 / 0.5 / 0.4 (twin() default) | −12.9 | −13.9 | −17.2 | −20.7 | −15.4 | −9.8 | −6.2 | −4.7 |
| 0.35 / 0.5 / 0.65 | −9.6 | −11.7 | −16.5 | −22.1 | −19.5 | −13.9 | −10.3 | −8.8 |
| 0.3 / 0.6 / 0.6 | −10.1 | −11.9 | −16.1 | −21.0 | −19.7 | −14.6 | −11.2 | −9.6 |

  Even at jazz settings, the blackface stack keeps its ~400 Hz scoop: 10–12 dB under 60 Hz and
  under 3 kHz. A Polytone or JC (active, Baxandall-like EQ, flat at noon **[M]**) has no scoop.
  That is the Polytone's "fat middle" against the Fender's "sparkle" **[L]; [E] the cause**.
- **A "polytone" preset [E]:** no tube stages driven (gain as the twin's but volume 0.1: clean
  even with a humbucker 2.6× hotter). Skip the Fender stack and use an active EQ: a low shelf +3
  dB below 150 Hz, a peak +2 dB at 500 Hz (Q 0.7), a high shelf −4 dB above 2.5 kHz. Then the
  1×15 closed cabinet: closed-box resonance at 85 Hz (Q 1.0, 12 dB/oct below), flat to 1.5 kHz,
  −3 dB at 2.5 kHz, 24 dB/oct above 3.5 kHz, no break-up peaks (a 15" jazz speaker is stiff and
  clean) **[E from general 15" speaker curves]**. Clip/power stage at 0 drive (solid state, clean
  headroom).
- **A "twin-jazz" preset [E]:** `twin()` with treble 0.3, mid 0.65, bass 0.6, volume 0.12 (the
  humbucker's level). Cabinet 0 (open back) with `PJ_AMP_CAB` ~0.8 (less of the cone's 2.4/3.7
  kHz lift).
- **Reverb:** a little (a spring at 0.05–0.15 wet) **[L]**; the module has none yet.
- **No chorus, no compressor** for jazz **[L]**. (FluidR3's preset sends 14 % chorus **[T,
  electric_guitars.md §2.1]**; not to copy.)

---

## 6. The jazz guitarist

### 6.1 What the user's file asks for [F]

louisiana.mid, channel 7 (index 6), program 26 (GM 27), 133 bpm, 1693 notes, 535 chords
(`an.py`, `play.py`, `reduce.py`, `topv.py` in the scratchpad's `jg/`):

| | |
|---|---|
| Chord sizes | 1: 45, 2: 70, **3: 238, 4: 140**, 5: 21, 6: 18, 7: 3 |
| Within a chord | all notes on the same tick (never rolled); velocity spread median 15; the top note +2.6 above the rest on average (no melody emphasis); in 225 of 420 chords of 3+ notes the notes end at different times (fingers lifting one by one) |
| Duration (beats) | deciles 0.31, 0.33 (×5), 0.67, 1.0, 1.33; **166 notes (10 %) have zero length** (on and off on the same tick), velocity median 31 against 46 for the long ones |
| Velocity | deciles 22, 27, 31, 38, 45, 50, 57, 62, 72: **soft**; max 109 |
| Inter-onset | 1.0 beat (306), 1.67 (70), 1.33 (59), 2.33 (27), 0.67 (18): comping in a swung feel; the onsets are humanised (no fixed grid position) |
| Range | 39–85: one note under E2, **16 chords topping C#6**, over the ES-175's 20th fret (C6 = 84) |
| Intervals between adjacent chord notes | 4 (303), 3 (210), 5 (154), 1 (137), 7 (82), 6 (68), 2 (52), 10 (45), 16 (33), 14 (22): close, piano-like voicings with seconds, plus a bass note a tenth or more under the rest |
| Doubled pitch classes | 51 chords |
| Playable as written (distinct strings, fretted span ≤ 5 frets, 20 frets) | **87 %** before range folding; 90 % after (482/535) |

- **After range folding** (above 84 an octave down; under 40 an octave up), a reducer that keeps
  the top note fixes all of the remaining 53 [F]:
  - the fifth above the bass dropped: 13 chords;
  - the lowest inner note dropped: 10;
  - doubled pitch classes, then the bass (when ≥ a fifth under the next note), then an inner
    note: 9;
  - the bass alone: 6;
  - the bass and an inner note: 6;
  - the fifth and an inner note: 3;
  - doubled pitch classes and the fifth: 3;
  - doubled pitch classes alone: 3.

  83 of 1687 notes are dropped (4.9 %). Moving an inner note by an octave never helped (the
  close voicings are the problem, not the range).
- **The current code** (`guitarinst.cc`, `play()`) sorts the notes and `resize`s to six. That
  drops the **highest** notes, which carry the melody and the colour tones in a jazz voicing. Its
  per-note greedy string choice can also fail on close voicings that a proper grip handles (two
  notes a semitone apart on adjacent strings, 4–5 frets apart).

### 6.2 Comping, as players describe it

- **Freddie Green (Count Basie) four-to-the-bar [S/L, freddiegreen.org (Pettersen and
  others)]:**
  - An **unamplified** archtop, heavy bronze strings (low E .056 or more), very high action
    (three dimes under the 6th string at the 12th fret), a pick of 1.5 mm or more.
  - One down-pulse per beat from the shoulder and elbow with a wrist flick; alternate strokes hit
    different parts of the strings, so beats 1/3 and 2/4 differ.
  - Pettersen's reading: Green struck all six strings but **deadened all except the 4th**, so
    most chords sounded **one note** (its range F3–D4 on the 4th string, frets 3–12), moving as
    an inner line. The left hand **released the 4th string slightly** after each strike to choke
    it. Others (Berens) teach three-note voicings on strings 6, 4, 3, the 5th muted by the
    fingertip.
  - General swing advice in the same source: four strokes a bar. Beats 1 and 3 ring longer
    ("choo"), 2 and 4 are short ("chit") and slightly accented, the left hand releasing
    "almost immediately". Every unfretted string is muted. No barre chords ("muddy"). The guitar
    sits "barely quieter than the drums".
  - Other players compared there: Allan Reuss (three-note, widely spaced, accents on 2 and 4),
    Teddy Wilson-style (bass on the 6th, top on the 3rd a minor tenth up, a 6th/7th on the 4th),
    Barry Galbraith (miked, so struck more lightly).
- **Small-group comping** (bebop and after): irregular, short, syncopated chords (anticipations,
  Charleston rhythm), mostly 3–4 notes on the middle four strings. The guitarist leaves the root
  to the bass and plays the 3rd and 7th plus one or two extensions (9, 13, altered 5/9). That is
  what louisiana ch7 is **[L; S, Wikipedia "Comping (jazz)"]**.
- **Voicing families [S, lesson sites; L]:**
  - **Shell:** root + 3rd + 7th (or root + 7th + 3rd), the fifth omitted. On strings 6-4-3 or
    5-4-3: the skipped string is muted by the fretting finger.
  - **Guide tones:** 3rd and 7th alone (with a bassist), on adjacent strings.
  - **Drop 2:** a close four-note chord with its second-highest voice dropped an octave
    (root–5th–7th–3rd from the bottom). On strings 4-3-2-1, 5-4-3-2, 6-5-4-3.
  - **Drop 3:** the third-highest dropped (root–7th–3rd–5th). On strings 6-4-3-2 and 5-3-2-1, with
    one string skipped.
  - With a bassist, the root or the fifth is replaced by the 9th or the 13th **[S/L]**.
- **Bossa nova on an archtop:** the thumb on the bass (root–fifth alternation, on 1 and 3), the
  fingers plucking 3–4-note chords together on the syncopated rhythm. Soft, no pick, flesh
  **[L]**.
- **Chord-melody (Joe Pass, Ted Greene):** the melody on top of a drop-2/drop-3 grip, the melody
  note louder. Often fingers or a pick-and-fingers hybrid **[L]**.

### 6.3 Voicing rules for a MIDI-driven guitarist [E, from 6.1–6.2]

1. **Range:** a note above the top fret goes an octave down; below E2, an octave up if that does
   not cross the next chord note, else it is dropped (the bassist has it).
2. **Playable as written?** One string per note; frets within a span of 4 (5 above the 7th fret,
   where the frets are narrower); a string between two used ones may be skipped (muted). No open
   strings in comping: they cannot be choked by the left hand's release. Open costs +2 for GM 27,
   where the nylon prefers them.
3. **Otherwise reduce,** stopping at the first playable result, and **never drop the top note**:
   - (a) doubled pitch classes (keep the top and the bottom instance);
   - (b) the perfect fifth above the bass (unless it is the top note);
   - (c) the bass note, when it lies ≥ 7 semitones under the next note: the bassist plays it. If
     the file has no bass part, keep it and drop an inner note instead;
   - (d) the root doubled in the upper voices;
   - (e) inner notes from the lowest up, sparing the 3rd and 7th (intervals 3/4 and 10/11 above the
     bass, when the bass reads as the root).
   - On louisiana ch7 this drops 4.9 % of the notes, never a top note, and leaves every chord
     playable **[F]**.
4. **Prefer known grips:** when two fingerings tie, the one whose strings form a shell (6-4-3,
   5-4-3), a drop 2 (four adjacent strings) or a drop 3 (6-4-3-2, 5-3-2-1) wins. So does the one
   nearest the last chord's position (voice leading: a hand that moves 0–2 frets between chords).
   Frets 3–10 are the comping zone **[L]**.
5. **More than 6 notes,** or 5–6 notes that will not fit: reduce as in 3 to four notes. A jazz
   guitarist seldom sounds more than four in comping **[L]**.

### 6.4 How a comping chord is struck and let go [E, from 6.2 and the file]

- **The stroke:** a pick rake, **down** for chords on the beat and most anticipations. Up strokes
  come on fast repeated off-beats (pendulum, as the existing strum logic has it). It is faster
  than a rock strum: **2–4 ms per string** at mf, 5–6 ms when soft. A skipped (muted) string
  under the rake gives a dead click: the pick over a string the left hand damps. The pick crosses
  at ~150–180 mm from the saddle, over the neck pickup and the end of the fingerboard, where jazz
  players pick **[L]**.
- **Velocity:** the file's median 45 is a normal comping stroke, not a soft one. Map velocity 20 →
  pick depth ~0.3 mm, 45 → ~0.7 mm, 100 → ~1.6 mm on the heavier strings, and the rise from 2.5
  ms to 1.2 ms **[E]**. Within a chord, the per-note velocities spread ~15: honour them as each
  string's depth.
- **Short chords** (the median 0.33 beat ≈ 150 ms at 133 bpm): on note-off the left hand
  **releases pressure without lifting**. The string leaves the fret and rests on the finger, so it
  dies in 20–40 ms: a damping of ~150–300 /s, against the code's `kRelease_` = 25 /s. Add a quiet
  "chk": the string lifting off the fret, the finger's flesh **[E; L "chit"]**. Strings whose notes
  end at different times in the file release one by one (the file does this in 225 of 420
  chords).
- **Zero-length notes** (10 % in the file, softer, velocity median 31) are **ghost strokes**: the
  rake falls on strings the left hand touches but does not press. A pitched thud of 30–60 ms, its
  highs gone at once. Model it as a pick stroke followed by `damp(s, ~200)` after 15–25 ms; at the
  lowest velocities, the dead string alone: `mute()` on the whole string **[E]**.
- **Four-to-the-bar detection** (Freddie Green): eight or more chords in a row, about one beat
  apart, each under 0.6 beat, through a swing tune. Then all down strokes and a 6-4-3 shell: the
  string-6 and string-3 notes are damped harder than the 4th, so the inner line is what rings.
  Beats 2 and 4 are a touch louder and shorter **[E from S/L]**.
- **Long chords** (≥ 1 beat, soft: ballads, the end of a phrase): the fingers, not the pick. All
  notes plucked together with flesh (`pluck(..., nail 0)` or `pluckPad`), no rake **[E]**.

### 6.5 Single lines (bebop)

- **Picking [S/L, Premier Guitar's Parker and Coltrane lessons; forums]:** not strictly alternate.
  - The common practice: pick the off-beat ("and") and **slur** onto the beat (hammer-on,
    pull-off or slide). Coltrane-style lines do this rigorously; Parker's phrasing slurs in pairs,
    loosely.
  - Slurs only where the fingering keeps the two notes on one string.
  - Down strokes dominate on the beats; sweep/economy picking when crossing strings in the
    direction of travel. Benson's and the gypsy players' rest-stroke picking "has the articulation
    built in".
  - **For the player [E]:** consecutive notes ≤ 2 semitones apart on one string, the second on a
    beat, the gap under 0.3 s: play the second as a hammer/pull (`fret()` with a small attack and
    no pick), about half the time. Otherwise pick: down on beats, up off-beat, economy across
    strings.
- **Ghost notes:** a note in a line markedly softer than its neighbours (≤ 0.6 of their velocity)
  is half-fretted and picked lightly: muffled, pitch barely there **[L; E the rule]**.
- **Vibrato:** little and late in bebop **[L]**. The existing left-hand vibrato (0.6 s onset) is
  about right, at half depth.
- **Pitch bends:** rare; a quarter-tone blues inflection at most **[L]**.

### 6.6 Wes Montgomery's thumb and octaves

- **The thumb [S, Wikipedia, PG, Guitar World]:**
  - He plucked with the fleshy pad of the thumb: down strokes for single notes, down and up for
    chords and octaves. Upstrokes are hard to articulate (he had an unusually flexible thumb).
  - The tone is dark: he asked Gibson for guitars modified to be brighter.
  - He played thumb-style because it was quiet to practise with, and kept it because he liked the
    tone better.
- **Octaves:** two notes an octave apart, fretted by the index (on the 3rd or 4th string) and the
  pinkie (two strings higher), with the string between them muted by the index's underside. Down
  strokes sweep all three strings **[S, Guitar World]**.
- **Model [E]:** `pluckPad(s, depth, at ~0.17 m, angle 0.4, width 0.014, rise 0.004, slip 0.0018)`.
  It is broad and slow, with no edge: few partials above ~1.5 kHz before the tone control even
  acts. A MIDI octave pair (two notes 12 apart, alone or in a monophonic line) is played on
  strings s and s+2, the middle string muted and swept.
- **When to use it [E]:** GM files do not say. Use the thumb for octave pairs, and for soft single
  lines (velocity < 50) in a phrase of octaves. Use the pick otherwise. A knob (PJ_JAZZ_THUMB: 0
  never, 1 octaves, 2 all single lines) leaves the choice to the user.

---

## 7. FluidR3's "Jazz Guitar" as a target [F]

TiMidity 2.x with FluidR3_GM (`-c /etc/timidity/fluidr3_gm.cfg`, 44.1 kHz), CC91 and CC93 at 0.
Each note was held 4 s (8 beats at 120 bpm), mono mix, RMS over 10 ms (`mk.py`, `ana.py`,
`ana2.py`). The sample set itself is described in electric_guitars.md §2.1: 12 samples E3–G6,
22.05 kHz, looped.

| Note | Peak (dBFS) vel 30 / 60 / 90 / 120 | Centroid 0–50 ms (Hz) vel ≤60 / ≥90 | Decay 0.05–0.5 s (dB/s) | Decay 1–3.5 s (dB/s) | Content −40 dB above |
|---|---|---|---|---|---|
| E2 (40) | −46.4 / −36.4 / −29.7 / −25.7 | 275 / 500 | −5 | −2 | ~2.5 kHz |
| A2 (45) | −42.8 / −32.8 / −25.4 / −21.4 | 280 / 650 | −3 / −5 | −2 | ~3 kHz |
| D3 (50) | −44.4 / −34.4 / −26.8 / −22.7 | 315 / 790 | −4 / −6 | −4 | ~3.5 kHz |
| G3 (55) | −46.9 / −36.8 / −25.9 / −22.0 | 615 / 1545 | −14 / −24 | −5 | ~5 kHz |
| B3 (59) | −46.3 / −36.3 / −24.9 / −20.9 | 690 / 1490 | −13 / −14 | −5 | ~5 kHz |
| E4 (64) | −45.4 / −35.3 / −25.1 / −21.1 | 770 / 1430 | −15 | −4.5 | ~5.5 kHz |
| A4 (69) | −45.4 / −35.4 / −25.3 / −21.2 | 810 / 1375 | −18 / −23 | −4 | ~4.5–5 kHz |
| E5 (76) | −37.4 / −27.4 / −20.9 / −16.8 | 665 / 890 | −28 | −0.3 / −3.7 | ~3.5 kHz |
| C6 (84) | −43.6 / −33.6 / −21.0 / −16.9 | 1050 / 1350 | −50 to −55 | −3 | ~5 kHz |

- **Velocity:** ~**21 dB** from 30 to 120 (+10, +7, +4 dB per step). Two timbres: everything
  ≤ 60 shares one spectrum, everything ≥ 90 another, twice as bright (the harmonics' ratios are
  identical within each pair). The file has nine velocity layers on the same samples
  (electric_guitars.md §2.1), so this is the SF2 velocity-to-filter modulator as TiMidity applies
  it, in two steps **[F; E the cause]**.
- **The two-stage decay:** from G3 up, the first half second falls 13–55 dB/s, then 3–5 dB/s.
  That is the archtop's punch and quick fall. The low notes fall only 2–6 dB/s: their samples are
  long and looped.
- **Brightness:** at mf the centroid sits at 600–800 Hz through the middle register. Nothing above
  ~6 kHz anywhere: the 22 kHz samples, and a rolled-off tone.
- **Release:** a G3 held 0.17 s (its peak at ~30 ms) is 40 dB down 0.17–0.20 s after the peak,
  i.e. within ~30–60 ms of note-off.
- FluidR3 is a target for level, velocity range and brightness, not for the low notes' sustain:
  they are a loop, while the real low E's fundamental rings ~20 s (Rau17).

---

## 8. What a listener would notice if it were wrong

1. **Too bright:** any 3–5 kHz presence (a Strat's or a rock humbucker's with the tone up) and the
   jazz guitar becomes a "clean electric". The rolled-off tone (−12 dB at 2 kHz) is the first
   thing heard.
2. **Upper partials that sustain:** flats on an archtop lose everything above the 2nd–3rd harmonic
   in about a second, while the fundamental hums on. A note whose 4th–8th harmonics ring for 3+ s
   sounds like roundwounds on a solid body.
3. **The fundamental dying with the rest:** the opposite error, a plucked-acoustic decay. The jazz
   note is round and long underneath and short on top.
4. **A plain-G chord's beating:** with a plain G (B ~1e-4), major thirds against the B string
   beat audibly. Jazz sets have a wound G.
5. **Comping chords that ring on:** short chords must be choked within ~30 ms. A chord that rings
   through the rest of the beat sounds like folk strumming. Open strings ringing under a moving
   voicing are the same fault.
6. **Wide, slow strums:** a jazz rake is quick (10–20 ms for the chord); a 6–8 ms/string rock
   strum smears the voicing.
7. **Voicings with the melody missing,** or with the 3rd or 7th dropped and the fifth kept: the
   chord quality disappears. Bad voicing is heard as "wrong notes" even when every note sounding
   is in the file.
8. **Too many notes:** 5–7-note chords on a guitar sound like a piano. Players use 3–4.
9. **No body at all:** a hollow body has a faint woody "air" (the top heard by the pickup and the
   room). Without it the tone is a little clinical, like a solid body with flats. This is subtle.
10. **Too much body:** a boomy, resonant 100–250 Hz, a flat-top's "acoustic" sound. The jazz
    guitar is mostly the pickup.
11. **The same stroke every time:** jazz comping varies in accent, rake speed and length chord to
    chord; ghost strokes are part of the groove.
12. **A pick attack with a sharp click:** a heavy round pick on flats gives a soft, thick attack.
    A thin pick's "tick" is a rock sound.
13. **Chorus and compression:** FluidR3's 14 % chorus is not the jazz sound. Nor is a compressor's
    flattened attack.

---

## 9. What to build

### 9.1 The guitar: `electricGuitar("jazz-guitar")` → an ES-175 [E unless marked]

- **Scale and frets:** 0.628 m, 20 frets (top note C6 = 84).
- **Strings** (TI JS112 / D'Addario ECG25 at 628 mm). Tension from section 3.1; f0 standard.

  | | E2 | A2 | D3 | G3 (wound) | B3 | E4 |
  |---|---|---|---|---|---|---|
  | T (N) | 98 | 96 | 101 | 103 | 95 | 96 |
  | B | 3.5e-4 | 1.1e-4 | 3e-5 | 1e-5 | 7.0e-5 | 2.2e-5 |
  | loss0 (/s) | 0.30 | 0.35 | 0.45 | 0.55 | 0.70 | 0.90 |
  | loss1 (k^2) | 0.08 | 0.09 | 0.10 | 0.12 | — | — |

  - B for the wound strings is the core-only value ×1.75; the plain strings are exact.
  - The plain strings use k^1.2 with loss1 = 1.5 × the Strat's.
  - This needs a per-spec exponent in `GuitarString`: today's `loss1·k^1.2` → `loss1·k^p`.
- **Steel:** the wound flats 0.9 × d² (roundwounds 0.6), then the same ^0.8.
- **Modes:** 64 per string; `topHz` 6000 (nothing above survives the tone control and the 15"
  speaker).
- **Body:** the table in 2.3 (neck 85/175/332 [T], A0 125, top 240, cross-dipole 420, ~15 plate
  modes 500–2500 Hz). The ES-175 has the laminate factors (rad ×0.5, Q ×0.8). Also:
  - `towardsLoss` 1.7;
  - `tilt` 0.35 (as the Strat);
  - `radiate` as the classical's, then mixed at −20 dB (9.3).
- **Pickup:**
  - the neck humbucker, two pole rows at 0.160 and 0.178 m (`at` = 0.169, `apart` = 0.018);
  - gaps 3.8 mm (low E) to 3.4 mm (high E), `width` 5 mm;
  - L 4.4 H, R 7.8 kΩ, C 120 pF;
  - volts as the Les Paul's neck pickup (0.8 × 2.6 × 0.15).
  - One pickup selected.
- **Microphonic:** a new `GuitarSpec::mount` (0.4). Each sample the pickup's displacement z =
  mount × Σ(body mode x × its shape at the pickup: for now the same `rad` sign pattern) is added
  to every string's `y` in `field(gap − y − z)`, for all strings including silent ones. Cost:
  one add per string.
- **Afterlength (optional):** six resonators at f_open × 0.628/0.10, Q 200, driven by the
  bridge's acceleration, radiated only.
- **Controls:** volume full; the tone branch R_t = 35 kΩ (PJ_JAZZ_TONE, kΩ), C_t = 22 nF; cable
  600 pF; amp input 1 MΩ.

### 9.2 The amp

- `GuitarAmp::Source` gains `{Rt, Ct}`. `connect()` builds the exact 3rd-order input (4.3).
- **Preset "jazz" (default for GM 27):** the Polytone-like path of section 5:
  - clean stages;
  - an active EQ (+3 dB low shelf under 150 Hz, +2 dB at 500 Hz, −4 dB high shelf above 2.5
    kHz) in place of the Fender stack;
  - new cabinet 3: a 1×15 closed (85 Hz Q 1 high-pass, −3 dB at 2.5 kHz, 24 dB/oct above 3.5
    kHz).
- **Preset "twin-jazz"** (alternative): `twin()` with treble 0.3, mid 0.65, bass 0.6, volume 0.12;
  cabinet 0 with `PJ_AMP_CAB` 0.8.
- No compressor, no chorus, no oversampling.
- **Acoustic mix:** `Guitar` exposes `acoustic()` (its `out` before the electric override). The
  model's output is amp + 10^(−20/20) × kAcoustic × acoustic, after the amp's level calibration
  (`kAmpOut_`).

### 9.3 The guitarist (GM 27)

1. **Voicing:** the reducer of 6.3 replaces `asked_.resize(6)` for GM 27 (and is right for 28 too).
   Open strings cost +2 in comping. Grips (shell, drop 2, drop 3) win ties. The hand moves ≤ 2
   frets between chords when it can.
2. **Chord stroke:**
   - a pick rake (`pick`, rise 1.5–2.5 ms, contact as a heavy rounded pick) at 0.165 m from the
     saddle, 2–4 ms per string;
   - **down** on the beat and on anticipations, up on fast repeated off-beats (the existing
     pendulum and beat logic);
   - skipped strings in the grip get a dead click (a muted pick stroke).
3. **Velocity → depth:** 20 → 0.3 mm, 45 → 0.7 mm, 100 → 1.6 mm, per note.
4. **Release:** note-off → `damp(s, 200)` after 5 ms, plus a quiet fret "chk". The strings of a
   chord release individually as the file says. Sustain pedal: let ring.
5. **Zero-length notes:** a ghost: pick, then `damp(s, 200)` after 20 ms. Under velocity 25: a
   fully muted stroke.
6. **Four-to-the-bar** (8+ chords ~1 beat apart, each < 0.6 beat): all down, 2 and 4 +3 velocity
   and 30 % shorter, the outer notes of a 6-4-3 shell damped (`damp` 80 /s) so the 4th-string
   line sings.
7. **Long soft chords** (≥ 1 beat, velocity < 50): fingers together, flesh, no rake.
8. **Single lines:**
   - down on beats, up off-beats, economy across strings;
   - hammer/pull for ≤ 2-semitone steps onto a beat on the same string, about half the time;
   - ghost notes (≤ 0.6 × the neighbours' velocity) half-fretted;
   - vibrato at half the existing depth.
9. **Octaves:** a pair 12 apart in a monophonic context → strings s, s+2, the middle muted. The
   thumb (`pluckPad`, 14 mm, 4 ms rise, 1.8 ms slip) per PJ_JAZZ_THUMB.

### 9.4 Checks (measurable)

| Check | Target |
|---|---|
| Open low E through the pickup, time to −15 dB for harmonics 1–5 | 5.5, 4.5, 0.9, 0.9, 1.1 s (±30 %) **[T, Rau17]** |
| The input network at R_t = 35 kΩ | −3 dB at 1.4 ± 0.2 kHz, no peak; at R_t = 0: peak ~460 Hz **[E, computed]** |
| Spectral centroid, first 50 ms, G3–A4 | 600–800 Hz at velocity ≤ 60; 1.3–1.6 kHz at 100–120 **[F]** |
| Energy above 3 kHz, first 0.5 s, mid register, mf | ≥ 25 dB under the strongest third-octave band **[F]** |
| Velocity 30 → 120 | 20 ± 3 dB **[F]** |
| Mid and high notes (G3 and up), the first 0.5 s | −13 to −30 dB/s, then −3 to −6 dB/s **[F]**; low notes: a fundamental ringing ≥ 10 s |
| A 0.33-beat chord at 133 bpm | −40 dB within 40 ms of note-off |
| louisiana ch7 | every chord playable; 0 top notes dropped; ≤ 5 % of notes dropped; no open strings in chords of 3+ notes; the hand's mean move between chords ≤ 2 frets |
| Wound-G inharmonicity | partial 10 within 2 cents of 10·f1 |
| The acoustic share | −20 dB (±5) under the amp, A-weighted, on a chord |
| Cost | ≤ 1.5 % of a core (64 modes, ~25 body modes, no oversampling) |

### 9.5 Not found, or uncertain

- **Assembled archtop modes** (ES-175, L-5): none published that could be read. The table in 2.3
  is Rau's neck modes [T] plus estimates.
- **The archtop's bridge admittance:** Rau measured it on the hollow body, but only as plots (not
  digitised here). Digitising Rau17's Figure 2 (the bridge, vertical) would fix the masses of
  2.3.
- **Salvi et al. 2021 and Longo et al. 2025** (archtop plates, L-5 FEM): paywalled; abstracts only.
- **The acoustic-to-pickup level** of a hollow body: no measurement; −20 dB is an estimate. So is
  the pickup's motion with the top (the `mount` share).
- **Flatwound vs roundwound:** no controlled measurement of decay or spectrum. Rau's one string is
  the only measured flatwound decay. The flat ribbon's added bending stiffness (B ×1.5–2) is an
  estimate.
- **D'Addario's official chart PDF** did not download (an HTML stub); the Chromes numbers are from
  the product page via a search snippet, and retailers disagree on the wound strings by up to 4
  lb. Thomastik's catalogue was read in full [T], but does not state its scale.
- **Pickup inductance:** no measured value for the '57 Classic, the Johnny Smith or the DeArmond
  1100. The PAF/'57 at 4.4 H is interpolated from Lemme's 490R (4.2 H) and maker ranges.
- **The ES-175 neck pickup's distance from the saddle:** estimated from the geometry (165–175 mm),
  not measured.
- **The Polytone Mini-Brute's tone circuit:** the schematic is behind a download wall; its
  speaker per model is from retail listings. The "Polytone" EQ above is a design, not the circuit.
- **The JC-120's EQ:** not published; at least 8 circuit versions.
- **Which tone setting players actually use:** "rolled back to about a third" is lore. With an
  audio taper that is very dark (0.7 kHz); R_t is left as a knob.
- **Comping articulation in numbers** (rake speed, the choke's time constant, ghost-note levels):
  no measurement. The values in 6.4 are estimates to be set by ear.
- **Freddie Green's voicings:** the one-note-chord thesis (Pettersen) is disputed; both readings
  are given.
- **FluidR3's velocity timbre switch** is as TiMidity renders it. FluidSynth may apply the SF2
  velocity-to-filter modulator continuously.

---

## 10. Build order

1. **Strings and body:** the `GuitarSpec` for "jazz-guitar" (9.1) and the per-spec loss exponent.
   Check the low E's five decays against Rau17.
2. **The radiated share and the microphonic pickup:** `acoustic()`, `mount`.
3. **The tone branch in `GuitarAmp::Source`;** the "jazz" preset and the 1×15 cabinet. Check the
   input network and the FluidR3 centroid and velocity targets.
4. **The voicing reducer** (shared with GM 28), then the comping stroke, release, ghost and
   four-to-the-bar rules. Check on louisiana ch7.
5. **Single lines, octaves, the thumb.**
6. **Optional:** the afterlength resonators, feedback, reverb in the module.

Point `src/midifile.cc`'s kGm "jazz-guitar" at the new instrument once 1–4 are done; until then it
goes to the koto (electric_guitars.md, status).

---

## 11. References

- **[Rau17]** M. Rau, E. Maestre, J. O. Smith, G. Scavone, "An Exploration of Guitar Neck Admittance
  Measurements Taken at Different String Stopping Locations", ISMA 2017, 73–76. Read in full (a
  hollow-body electric with a floating bridge and flatwounds: neck modes, Table 1's decays).
  https://isma2017.cirmmt.mcgill.ca/proceedings/pdf/ISMA_2017_paper_71.pdf
- **[RauPhD]** M. G. Rau, "Vibration and acoustic measurements of guitars with applications to
  luthiery, synthesis, and audio effects", PhD thesis, Stanford, 2023 (§5.3 is Rau17; ch. 4
  electric-to-acoustic processing). Downloaded and searched. https://purl.stanford.edu/kx813bh3027
- **[SGAS21]** D. Salvi, S. Gonzalez, F. Antonacci, A. Sarti, "Modal analysis of free archtop guitar
  top plates", JASA 150(2), 2021, doi 10.1121/10.0005937 (abstract, via Crossref).
- **[LGDNA25]** Longo, Gonzalez, Dalisay, Nania, Antonacci, "Influence of thickness profile and bracing
  pattern in the radiation patterns of archtop guitars", JASA 157(2), 1141–1150, 2025 (abstract via
  search). https://re.public.polimi.it/handle/11311/1298758
- **[TI]** Thomastik-Infeld, "Jazz guitar strings" catalogue, 2013 (George Benson, Jazz Swing,
  Jazz Bebop tables). Read in full. https://medias.audiofanzine.com/files/ti-jazz-guitar-471994.pdf
- **[D'Addario]** ECG24/25/26 product pages and tension charts (via search snippets; the pages
  returned 429). https://daddario.com/products/guitar/electric-guitar/xl-chromes/ecg25-chromes-flat-wound-light-12-52/ ;
  https://daddario.com/products/guitar/electric-guitar/xl-chromes/ecg26-chromes-flat-wound-medium-13-56/
- **[FG]** freddiegreen.org: M. Pettersen's technique article and "A Comparative Study of Rhythm
  Guitar Styles" (read). https://www.freddiegreen.org/technique/pettersen.html ;
  https://freddiegreen.org/technique/mp_compare.html
- **[PG-Parker]** Premier Guitar, "Charlie Parker for Guitar" (via search).
  https://www.premierguitar.com/lessons/jazz/charlie-parker-for-guitar
- **[Wes]** Wikipedia "Wes Montgomery"; Guitar World "Ax Museum: Wes Montgomery"; Premier Guitar on
  Wes's octaves (via search). https://en.wikipedia.org/wiki/Wes_Montgomery ;
  https://guitarworld.com/blogs/ax-museum-wes-montgomery ; https://www.premierguitar.com/wes-montgomery-octaves
- **[Comping]** Wikipedia "Comping (jazz)"; voicing lessons (jazzguitar.be, guitarplayer.com) via
  search. https://en.wikipedia.org/wiki/Comping_(jazz)
- **[DeArmond]** Guild's 1100 page; music-electronics-forum measurements of a Rhythm Chief 1000
  (read). https://guildguitars.com/store/product/98-041-3270 ;
  https://music-electronics-forum.com/forum/instrumentation/pickup-makers/12262-measurements-from-my-dearmond-rhythm-chief-1100
- **[Gibson]** '57 Classic listings (DC resistance). https://www.gibson.com/en-US/p/Pickup/PU57-plus-DB2/PU57DB2
- **[Lemme13], [MDH17], [planetz, Mojotone], [ozvalveamps], [YS06]:** as in
  `research/electric_guitars.md` §19.
- **[Amps]** Roland JC-120 product page; Henriksen and Polytone retail listings and reviews
  (audiofanzine, djangobooks, libertybellows), via search. https://www.roland.com/us/products/JC-120/ ;
  https://henriksenamplifiers.com/the-jazz-amp/ ; https://en.audiofanzine.com/solid-state-combo-guitar-amp/polytone/mini-brute-iv/
- **[Forums]** MIMF and jazzguitar.be threads on archtop air resonance and tap tones (snippets
  only; the pages returned 403). https://www.mimf.com/phpbb/viewtopic.php?p=58950 ;
  https://www.jazzguitar.be/forum/builders-bench/98291-tuning-top-dangelico-new-yorker-copy.html
- **[Richardson]** B. E. Richardson, "Investigations of mode coupling in the guitar", Proc. IoA (via
  search). https://www.ioa.org.uk/system/files/publications/BE%20RICHARDSON%20INVESTIGATIONS%20OF%20MODE%20COUPLING%20IN%20THE%20GUITAR.pdf
- **[Patent]** US 9905212, "Bridge with pickup for hybrid arched top guitar" (background; via search).
- **[FluidR3]** FluidR3_GM.sf2 rendered with TiMidity (`/etc/timidity/fluidr3_gm.cfg`).
- **[louisiana]** `~/Music/midi/bushgrafts/louisiana.mid`, channel 7 (the user's file).
- Within this repository: `research/electric_guitars.md` (§2.1, 4–6, 12–19), `research/guitars.md`
  (§3.2, §7, §9.3), `src/guitar.*`, `src/guitaramp.*`, `src/guitarinst.*`.

---

## 12. As built (2026-11-20)

`src/guitar.cc` `jazzGuitar()`, `src/jazzguitar.h/.cc` (the guitarist), `src/guitaramp.cc` `jazz()`
(the amp and the speaking strip), `src/guitarinst.cc` (the model's wiring), `tools/test.py`
`jazz_guitar()`. GM 27 now plays it (`src/midifile.cc`'s map already named it first).

### 12.1 The touch speaks

The request: the player's touch shapes the tone like a vowel, through pick position, pick strength,
pick width and softness, with formants and an envelope that follow the playing. All of it is
physical, in the string and the pick:

- **The pick** is `Guitar::pickContact` (a place carried by a hand, a tip of finite stiffness, a
  slip), given a contact width (default 0.8 mm: every older caller unchanged). The jazz pick: 1.5
  mm (17 kN/m), a rounded edge (its slip 0.25 ms against a sharp pick's 0.15), 2.5 mm of contact.
  The contact's width hardly matters for a pick (its sinc's first zero lies past the 400th
  harmonic); the slip is what softens: a raised-cosine release over T rolls off above ~1/T.
  Wes's thumb: `pluckPad`, 14 mm, a 4 ms rise, a 1.8 ms slip.
- **Strength:** velocity v sets the depth (0.1 + 2.0 v^1.25 mm) and the push (10 − 7.5 v ms); the
  slip follows the hand's speed, edge × √(0.3 m/s ÷ speed), 0.5 ms at v30, 0.2 ms at v120.
- **Place:** 0.165 m × (1.15 − 0.54 I) from the saddle, I the phrase's intensity (its running mean
  velocity) and this note's accent against it: 0.19 m on soft, even comping (the pluck's comb puts
  a notch near the 3rd harmonic: round), 0.10–0.13 m on accents and climaxes (the notch past the
  5th: nasal). Checked: picked at L/3, harmonics 3 and 6 drop 9 and 23 dB against their
  neighbours; at L/4, 4 and 8 drop 5 and 22 dB (bridge pickup; on the neck pickup its own notch at
  3.7 hides 3 and 4).
- **The angle:** 0.22 + 0.35 v rad: a harder stroke digs into the top, the swing the floating
  bridge drains twice as fast: the punch.
- **In time:** the wound flats' k² loss: what a hard stroke adds above 1.5 kHz dies in 50–300 ms
  (G3 v120: the 2–5 kHz share −26 dB from the first 50 ms to 200–400 ms).
- **Measured** (the first 50 ms, v30/60/90/120): G3 centroid 236/250/259/275 Hz, 2–5 kHz share
  −63/−47/−40/−31 dB; D4 315/339/363/396, −51/−35/−28/−23; A4 469/508/556/628, −47/−31/−27/−20.
  Every step up, the share by 27–32 dB over the range. The 5–10 kHz share at 127: −90 to −52 dB.

### 12.2 Deviations from section 9

- **The body's masses** ×8 (box) and ×100 (the neck's three modes), **towardsLoss 2.0** (not 1.7),
  **loss0 ×0.6** and **loss1 ×0.75** (wound: 0.036–0.054 k²; plain 1.1× the Strat's). With the
  table's masses the 85 Hz neck mode beside E2 passed the fundamental into the along-swing, which
  the pickup hears little: 15 dB down in 0.4 s against Rau's 5.45. As fitted: harmonics 1–5 of the
  open low E 15 dB down in 5.7, 3.4, 0.5, 1.2, 1.0 s (Rau17 5.45, 4.49, 0.86, 0.87, 1.12; within
  40 %; the 3rd sits on the top's 240 Hz mode). The larger towardsLoss on smaller losses gives the
  two stages (a quicker first half second, a longer tail).
- **The tone at 45 kΩ** (−3 dB at 1.9 kHz), the top of the research's 25–45 kΩ, and the amp's
  treble shelf −2 dB (not −4): with all of the tone pot, the shelf and the 15" a soft note was dull.
- **The amp's output** is brought up ×74 in the model (37 dB: the jazz preset's level against the
  Twin's), past the strip; the level trim is then 0.0 dB.
- **The guitarist:** open strings are forbidden in chords of 3+ (cost +2 left 12 in the user's
  file); the fretted span is 5 frets from the 5th fret (not the 7th: with it 5.2 % of the notes
  were dropped). A chord over C6 goes down an octave as a whole where its bass allows (folding
  only its top note made it collide with a lower one: 8 top notes lost). Keys within 8 ms are
  gathered as one gesture. No look-ahead: the chord's length is not known at its key, so "long
  soft chords" with the fingers are those soft ones after a rest of 1.5 beats.
- **Muting:** the strings outside a chord's grip are damped by the fretting hand (200/s), those
  beside a single line lightly (40/s): left free they rang along through the body for half a second
  at −36 to −40 dB.
- **Not built:** the afterlength resonators, feedback, reverb.

### 12.3 The speaking strip (the "compressor kludge")

The user's idea: a mild 3–4 kHz boost with a compressor arranged so the touch shows in timbre
more than in loudness. As built, after the speaker (an engineer's channel on the miked amp):

1. **Presence, static:** a peak of +2 dB at 3.4 kHz, Q 0.8 (x + (g − 1)·bp, bp a constant-peak
   band-pass: an exact peaking filter, no dip).
2. **Presence, dynamic (an upward expander on the band alone):** the band's envelope (1 ms up, 80
   ms down) against a reference, a firm mf attack's (−64 dB of the output): g = 2 + 0.8 (L − ref)
   dB, within −4 … +8 dB, smoothed over 2 ms. A hard attack opens it, a soft one leaves it shut,
   and as the note's highs die (12.1) it closes again: the boost follows the vowel the string
   already makes, and exaggerates it.
3. **The compressor:** 2:1 above −56 dB rms (two 10 ms poles), a 10 dB soft knee, 10 ms attack
   (the pick's first transient passes: no flattened attack, section 8 item 13), 250 ms release.
   Its ear is the signal *before* the presence lift: it hears the body, so the presence it is
   letting through does not make it duck, and a hard stroke comes out held back in level but open
   in colour. Soft playing (under the knee) passes untouched.

Why not a plain compressor: compression alone *raises* a soft note's attack against its body and
narrows the timbre range (measured: alone it brightened a soft phrase by 2 dB). Why not a plain
boost: a static 3–4 kHz lift makes every note the "clean electric" of section 8 item 1. Keyed on
the band's own level, the boost is there only when the player puts it there.

Measured on one phrase (a 12-note line and a four-note chord) at velocity 35, 70, 110 (the whole
phrase, its tails included):

| | 2–5 kHz share (dB) | centroid (Hz) | loudness (LUFS, as trimmed) |
|---|---|---|---|
| strip on, v35 / 70 / 110 | −29.8 / −26.3 / −22.0 | 293 / 309 / 363 | −36.8 / −30.5 / −26.4 |
| strip off | −28.9 / −30.4 / −30.4 | 289 / 279 / 293 | −36.4 / −27.6 / −20.6 |
| soft → hard, on / off | +7.7 / −1.5 dB | ×1.24 / ×1.01 | +10.4 / +15.8 dB |

(Over a whole phrase without the strip the hard one is no brighter: its louder, long fundamentals
outweigh the brighter attacks. Per attack, the guitar alone opens by 25–30 dB (12.1); the strip
makes that heard over the phrase.) In the tuning runs, before the unused strings were muted, the
same phrase gave ×1.58 / ×1.29, +11.9 / +3.8 dB, +9.7 / +14.8 dB.

The single notes' level from v30 to v120 is then 15–18 dB (the guitar alone ~20; the research's
target 20 ± 3 was for the instrument without a compressor). In `-play` the velocity row in
`src/velgain_data.h` puts the loudness on General MIDI's curve as for every instrument; the colour
stays the touch's.

### 12.4 Checks (tools/test.py `jazz_guitar()`)

| Check | Result |
|---|---|
| In tune, E2 … B5 | within 2 c |
| Tone network, 35 kΩ / 0 | −3 dB at 1378 Hz, no peak / +3.8 dB at 467 Hz |
| Rau17, harmonics 1–5 to −15 dB | 5.66, 3.37, 0.54, 1.15, 1.04 s (within 40 %) |
| Wound G, partial 10 | +0.6 c |
| Velocity opens the spectrum (G3, D4, A4) | monotonic, above |
| 5–10 kHz at 127 | −90 to −52 dB |
| Brightness dies into the body | G3 v120 −26 dB by 200–400 ms |
| Pick place, comb | as 12.1 |
| Strip: colour up, loudness down | as 12.3 |
| A short chord choked | −40 dB 18–41 ms after its keys (three chords; the body's own sound the last of it; FluidR3 30–60 ms; the test's bound 60) |
| Acoustic share | −20.1 dB, A-weighted |
| louisiana ch7 | 492 chords, 4.9 % of 1660 notes dropped, 0 top notes, 0 unplayable, 0 open strings, 0.99 frets |
| Fuzz 10 s; cost | ok; 2.7 % of a core on a five-note chord |

Not met, or met loosely:
- **FluidR3's brightness** (section 7: centroid 600–800 Hz soft, 1.3–1.6 kHz loud in G3–A4):
  here 236–628 Hz. On louisiana ch7 (43 s): centroid 348 Hz against FluidR3's 901, 0.8–2 kHz −17
  dB against −4, 2–5 kHz −27 against −12, short-term loudness spread 17 against 27 dB. With the
  tone full up, 419 Hz: the neck humbucker's comb on a string picked near it, and the closed 15",
  leave a strong fundamental. A warm archtop by choice; FluidR3's sample is a brighter guitar.
- **The first half second's fall** of the mid and high notes: 6–12 dB/s (FluidR3 13–30), the tail
  7–12 dB/s on the high ones (FluidR3 3–6).
- The velocity range 15–18 dB with the strip (12.3).
