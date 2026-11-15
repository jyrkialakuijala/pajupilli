# Piano variants and keyboard idiophones: research and model plans

Status: 2026-11-15. Research only. Nothing here is built. This file extends `research/piano.md`
(the concert grand, `src/piano.h/.cc`, `-inst grand-piano`) and uses the modal engine in
`src/modal.h` (`ModalSet`: modes with Hertz mallets, plucks, dampers and a small body). It does
not repeat what those cover.

Targets: GM 2 Bright Acoustic Piano, GM 4 Honky-tonk Piano (and the plain upright behind it),
tack and prepared piano (brief), toy piano, GM 9 Celesta and GM 11 Music Box. GM 3 Electric Grand
(the Yamaha CP-70/80) comes up under GM 2 because studios use the two together.

Tags, as in piano.md: **[web]** from a source in section 9; **[calc]** computed here (the script
is plain beam and contact formulas, E = 200 GPa and ρ = 7850 kg/m³ for steel unless stated);
**[memory]** remembered from the general literature, not looked up, to be checked before relying
on it. Values in the "plan" lines are starting choices for the model, not claims about instruments.

## 1. Short answers

- **Bright Acoustic Piano is the same grand, voiced and miked differently.** Use harder felt
  (K × 2–4 from the middle up, or a steeper hardening exponent), the lid open and microphones
  close over the hammers. A Yamaha C7 (2.27 m) is the studio favourite and has been "among the
  brightest sounding" since the 1970s **[web]**. The grand model has all the knobs already:
  `setVoicing`, `PJ_PIANO_COLOUR`, `PJ_PIANO_MICX/MICZ/MICBASS/MICTREBLE`. This needs no new
  physics, only a second voicing table and a mic preset. Cost is the grand's: 8 % of a core for
  one note, 29 % with the pedal down.
- **Honky-tonk is an upright with unisons spread by about 4–10 cents.** Tuners who build the sound
  on purpose set the centre string true and the outer strings ±1 Hz at A4, i.e. ±3.9 c; others
  plan 2 Hz (7.9 c); one account gives 8–10 c **[web]**. A neglected piano adds random strings up
  to a quarter tone flat **[web]**, and its hammers are hard and grooved. The grand's
  `PJ_PIANO_MISTUNE` scales a spread of 0.03–0.08 c. Honky-tonk is that spread × about 100, with a
  pattern (centre true, outer strings ±) and per-key randomness. The coupling through the shared
  bridge is already modelled: beyond about 0.3 Hz the strings beat instead of locking **[web,
  Weinreich]**, so the double decay goes away by itself.
- **A real upright needs a second scale and a second board, not a second model.** Shorter bass:
  the lowest string is 1.19 m on a Yamaha U1 and 1.21 m on a U3 **[web]**, against about 2.0 m on
  a concert grand. That gives about 3–10 times the inharmonicity in the bass and low tenor. B
  reaches 10⁻³ in an upright's bass, against about 10⁻⁴ on a large grand **[web, Dalmont 2021]**.
  The board is 1.38–1.53 m² (U1, U3 **[web]**) against the model grand's 1.98 m². It stands
  vertically, so the hammer swings horizontally. The one measured upright board in piano.md
  (1.39 × 0.91 m, 9 kg, a mode every 19.5 Hz, loss factor 2 %) **[web, Ege & Boutillon]** is the
  validation target. The work is to make `Piano` take a design (scale table, board data, mics)
  instead of building only the concert grand.
- **The moderator (practice felt) is a felt strip between hammer and strings.** Model it as a
  soft spring in series with the hammer felt. The tone loses "almost all its brilliance" **[web]**.
- **Tack and mandolin-rail pianos put metal between the hammer and the string.** The contact
  becomes a few tens of µs, governed by the string's impedance against a tack of a fraction of a
  gram **[calc]**. The result is a near-impulse with a flat spectrum up to 10 kHz, plus rattles. Add
  a small second mass to the hammer. Prepared piano (bolts, screws, rubber between strings) is a
  contact or mass point on a string. The modal engine's contact patches do it, but it is a project
  of its own.
- **The toy piano's rods are cantilevers. The note heard is probably not their fundamental.** A
  2.5 mm steel rod 215 mm long has modes at 38, 239, **670**, 1313 Hz **[calc]**. Its documented
  pitch is E5 = 659 Hz **[web, Logos]**, so the third bending mode (17.55 × f1) is the note. The
  fourth sits 1.96 × above it: an octave 35 c flat **[calc]**. The fundamental is "far too low to
  be radiated by the tiny sounding board" **[web]**. Use `ModalSet` and `cantileverRatio()`, as
  for the kalimba. Cost is under 1 % of a core.
- **The celesta's bars are free-free steel bars over tuned wooden boxes.** The modes are 1 : 2.757
  : 5.404 : 8.933 **[calc]** (2.76 and 5.40 **[web]**). The bars are 20–30 mm wide and 2–2.5 mm thick,
  supported at the nodes 22.4 % from each end **[web]**. The hammers are felt, with a damper per
  bar and a sustain pedal. It sounds an octave above the written note, C3/C4 to C8 (F8 on 5½-octave
  models) **[web]**. `ModalSet` covers it: one vibrator per sounding bar, `strike` with felt
  constants, `damp` for the dampers. The box can be a raised radiation of the fundamental and a
  little extra loss. About 1–2 % of a core.
- **The music box is plucked cantilever teeth with tip weights on the bass teeth.** The ratio
  f2/f1 is about 6.27 unweighted and up to about 7 with tip mass (the kalimba FE table). It is
  `ModalSet` with `pluck`, as the kalimba, with a fast slip, long decays and a small box. Under 1 %.

## 2. GM 2 Bright Acoustic Piano (and the CP-70)

**What it means physically.** GM names a timbre, not an instrument. In hardware GM sets it is a
brighter recording of the grand. Studios get that sound from:

- **Harder hammers.** Hardness affects "the loudness, the brightness, and the overall tone"
  **[web]**. Voicing (needling to soften; sanding, lacquer or a hardener to harden) has little
  measurable effect on the low partials of bass notes but affects every partial in the treble
  **[web, Conklin]**. A C7 owner asking for "a tad more punch in the treble" is a typical
  hardening job **[web]**.
- **Mics close over the hammers, lid open or off.** This is common pop practice at about 15–30 cm
  over the hammer line **[memory]**. The direct sound of the strings near the strike point and the
  hammer's knock dominate the board's radiation.
- **The instrument.** Yamaha C7 (7'6" = 2.29 m **[web]**) is "heavily recorded on pop records"
  **[web]**. In 1977 Yamaha was "considered to be among the brightest sounding" for studio use
  **[web, RE/P]**.

**How far.** Three recorded concert grands already spread by 10–20 dB in the 2–4 kHz share of a
middle-loud note: −22 to −31 dB (Beethoven) against −38 to −42 dB (Debussy, Schubert), with the
model at −25 to −35 (piano.md, 11). A "bright" preset should sit above the brightest of these.
**Plan:** −15 to −20 dB at keys 49–60, to be checked against a pop recording.

**Model plan.**
- Reuse `Piano` unchanged. Add a second voicing table: K from key 30 up × 2.5 (the fitted
  1.3e9 / 1.7e9 / 2.5e10 at keys 30 / 49 / 64 become about 3e9 / 4e9 / 6e10). Leave
  `PJ_PIANO_COLOUR` at 0.4, so soft blows still darken.
- Microphones: `MICZ` 0.65 → 0.25 m, `MICX` towards the hammer line. The microphones hear the
  board in 48 patches, so for a close mic add a direct pickup: the bridge velocity of the nearest
  keys, as the double bass's pickup does. This is the one new part (a few lines).
- Optional: a mid-air shelf (the lid's reflection) as a fixed EQ.
- Cost: as the grand.
- Validate with `tools/pianofit.py` on a pop solo-piano track: the 2–4 kHz and 4–8 kHz shares by
  register and loudness third. The decays must stay the grand's, since only the felt and the mic
  change.

**Yamaha CP-70/80 (for GM 3, and the "bright" pop piano of the 1970s–80s).**
- It is a real piano action and real strings, with a piezo pickup per string under the bridge and
  no soundboard to speak of **[web]**.
- The bass strings are about 67.9 cm long (26¾"). The lowest 15 keys have one string each and the
  rest two **[web]**.
- With the same core and tension as a grand bass string, B scales as L⁻²: (2.01 / 0.679)² = 8.8.
  A0's B is then about 7e-4 instead of 8e-5 **[calc]**. This is the clangy CP bass.
- Model: the grand's strings with the CP scale. Replace the board with a stiff, nearly rigid
  bridge: a few body modes, with high impedance so the strings lose little and the sustain is long.
  The output is the sum of bridge forces through a piezo high-pass (about 30–80 Hz **[memory]**)
  and a preamp. The cost is smaller than the grand's: about 160 strings and no 352-mode board.

## 3. GM 4 Honky-tonk, and the upright under it

### 3.1 The upright piano

| Quantity | Concert grand (model / literature) | Upright | Source |
|---|---|---|---|
| Lowest string, speaking length | 2.01 m (model), wound | 1.189 m (U1, 121 cm tall), 1.207 m (U3, 131 cm) | [web, Yamaha via dealers] |
| Lowest plain string | 1.90 m (model fit) | about 0.9–1.1 m; the bass bridge carries keys 1 to about 28 | [memory] |
| Middle C and up | 0.62–0.70 m at C4; the treble scale is nearly the same on all pianos | same within about 10 % from C5 up | [memory] |
| B, lowest strings | about 1e-4 (8e-5 model A0) | up to 1e-3 | [web, Dalmont 2021] |
| B, bass of a Yamaha C109 upright | | 4.5e-4 falling to 3.2e-4 across the bass | [web, secondary summary of Dalmont; unverified] |
| B, lowest plain string | 7.5e-5 (key 24, fitted) | about 8 × that if 1.0 m and 1.05 mm wire: about 6e-4 | [calc, B ∝ d² / (L⁴ f²)] |
| B, wound A0 at 1.19 m | 8e-5 | 2.3e-4 (same core and tension, B ∝ L⁻²) | [calc] |
| Soundboard | 1.98 m² (model), 2 m² (literature) | 1.38 m² (U1), 1.53 m² (U3); the measured board was 1.39 × 0.91 m, 9 kg | [web] |
| Board modes | one per 6.7 Hz (model), 256 to 2.3 kHz | one per 19.5 Hz measured to 1.1 kHz (plate behaviour); rib bays trap waves above 1.1–1.2 kHz | [web, Ege & Boutillon] |
| Bridge impedance | 1000–2000 kg/s | 800 falling to 230 kg/s at 2.5 kHz | [web, Ege & Boutillon] |
| Hammer mass | 11 g bass to 3.5 g treble | similar or slightly lighter; the hammer swings horizontally, so its weight matters less to the touch | [web, Conklin; web, Yamaha] [memory for the upright masses] |

- **Inharmonicity is the main audible difference.** The upright's tenor and bass partials run
  sharp sooner, the octaves stretch more, and the bass/tenor break (wound to plain) is a step in
  timbre. Dalmont calls the effect "particularly marked on upright piano with strings around 1 m
  length" **[web]**.
- **The board** is about 2/3 of the grand's area and 9 kg against 12.8 kg. With the same 2.5 cm
  grid, `tools/pianoboard.py` would give about one mode per 9–10 Hz (6.7 Hz × 1.98 / 1.38)
  **[calc]**, so 256 modes reach about 2.5 kHz. That is denser than the measured 19.5 Hz. The
  grand model came out three times denser than Ege's estimate too, and should be checked against
  the measured upright here before more is built on it.
- **The board stands upright, open to a wall.** It radiates from both faces. The rear face is
  0.1–0.3 m from a wall, which gives a comb of reflections. Microphones are usually above the open
  top or behind the board **[memory]**.
- **Action.** The upright action has no repetition lever (double escapement), and a spring, not
  gravity, returns the hammer. Fast repeats are less reliable **[memory]**. The dampers sit above
  the hammers on spring-loaded levers **[memory]**.
- **Pedals.**
  - Left (soft): moves the hammer rail closer to the strings, so the hammers travel less and hit
    slower. There is no una corda **[memory]**.
  - Middle: often the moderator (3.2). On older uprights it may be a sostenuto or a bass sustain
    **[web, Bechstein; memory]**.

### 3.2 The moderator / practice felt / "celeste"

- A felt strip drops between the hammers and the strings. The sound is "quieter and duller"; the
  felt "almost completely robs the piano sound of its brilliance" **[web]**. Players have to hit
  harder **[web]**. In the high treble, or with the pedal down, the felt can make neighbouring
  undamped strings sound **[web, PTG]**.
- No measured spectra were found. Pianoteq makes the pedal continuous with an adjustable felt
  thickness **[web]**, i.e. a modelled series spring.
- **Plan:** a second, soft spring in series with the hammer felt. Strip thickness is about 2–5 mm
  **[memory]**; stiffness about 1/10–1/30 of the middle hammer's K at the same compression, p about
  2, more hysteresis. The strip is wider than the hammer, so `hamW` goes from 10 to 20–30 mm, which
  rounds off the high partials. In the implicit hammer solve, two springs in series are one scalar
  equation for the shared compression, solved by Newton as now.

### 3.3 Honky-tonk numbers

| How | Spread | Source |
|---|---|---|
| Mrs Mills' Abbey Road piano, as recreated by a tuner | A4 strings at 439 / 440 / 441 Hz: ±3.9 c; plain-wire unisons only, wound bichords kept clean; a mandolin rail added | [web, PTG] |
| Another tuner's plan | 438 / 439 / 440 (2 Hz: 7.9 c), on old strings | [web, PTG] |
| A musician's request | two strings pure, the third about 1 beat/s off (about 4 c at A4) | [web, PTG] |
| Technician's honky-tonk tuning (one report) | unison strings 8–10 c apart | [web, Reddit, from memory] |
| Rag-piano FAQ recipe | centre true; in bass and tenor detune the second string, in the treble one side string, "just a little", up or down | [web, ragpiano] |
| A long-untuned piano | whole piano 1/4–1/2 tone flat after 20 years, a whole tone after 40–50; random strings about 1/4 tone off; the mid-treble (keys about 52–70) most out | [web, MMD] |
| A good tuning, for contrast | about 1.5 c between unison strings | [web, Physics Today] |
| The model grand | 0.03–0.08 c (fitted) | piano.md 11 |

Beat rates **[calc]** (between centre and outer string; partial n beats n times faster):

| Spread | C2 (65 Hz) | C4 (262 Hz) | C6 (1047 Hz) |
|---|---|---|---|
| 4 c | 0.15 Hz | 0.61 Hz | 2.4 Hz |
| 8 c | 0.30 Hz | 1.2 Hz | 4.8 Hz |
| 15 c | 0.57 Hz | 2.3 Hz | 9.1 Hz |

- A constant spread in **cents** makes the treble beat fast and turn sour. A constant spread in
  **Hz** (as "1 beat per second") makes the bass unclear and leaves the treble nearly clean.
  Tuners describe the treble as the worst on a neglected piano, so cents, with a gentler fall
  towards the top, is the better default.
- Above about 0.3 Hz the strings no longer lock through the bridge **[web, Weinreich]**. From
  about 4 c in the middle, every unison beats and the aftersound goes away. The model does this by
  itself, since the strings meet at the bridge.
- **Worn hammers.** The strings cut grooves into the felt, which "flattens the striking surface and
  hardens the felt" **[web, Yamaha]**. Played hammers harden more steeply (p 2.2–3.5 against
  1.5–2.8 new **[web, Hall & Askenfelt via piano.md]**). Plan: K × 2–3, p + 0.3, contact width
  12–15 mm (the flat), and a small random per-key K scatter (σ about 30 %) for uneven voicing.
- **Old strings.** Rust and corrosion **[web, anecdotal]**. Wound strings gain inharmonicity over
  years from mass redistribution **[web, Hamburg ISMA 2019, citing Houtsma]**. Old wire and loose
  bridge pins give "false beats": a single string's partial splits into two a few tenths of a Hz to
  a few Hz apart **[memory]**. Plan: higher wire loss (`PJ_PIANO_LOSS2` × 1.5–2), and the y/z
  split per mode raised and randomised so single strings beat.
- **Mandolin rail** (often fitted with honky-tonk tunings): metal tabs on felt fingers hang between
  hammer and strings **[web, ragpiano]**. It is the tack piano of section 4, switchable.

### 3.4 Model plan: upright and honky-tonk

1. **Make `Piano` take a design.** At present `scaleOf()`, `stiffness()`, `bassLength()`, the
   hammer tables, the mistuning law and `pianoboard_data.h` are fixed for the concert grand. Put
   them in a `PianoDesign` struct: the per-key `Scale`, a board namespace with its modes, shapes
   and patches, the mic geometry, the mistuning law (spread and pattern) and the damper range. The
   grand becomes one design. The `Piano` engine (strings, bridge exchange, hammer solve, dampers,
   board, mics) stays as it is.
2. **Upright scale.** 121 cm class (U1): wound strings for keys 1 to about 28 (1.19 m to about
   0.95 m), plain strings from about 1.0 m down to the grand's treble lengths by C5. The single
   and bichord breaks are as in the grand. B follows from wire and length, as the plain strings
   already do. Expected B: about 2e-4 at A0, about 6e-4 at the lowest plain string, the grand's
   from about C5 up **[calc]**.
3. **Upright board.** `tools/pianoboard.py` with a rectangular outline of 1.39 × 0.91 m, ribs
   12.8 cm apart, 8–9 mm spruce, 9 kg, a bass bridge and a long bridge. Check against Ege &
   Boutillon: modes per Hz to 1.1 kHz, loss factor 2 %, impedance 800 → 230 kg/s by 2.5 kHz.
   Same 256 + 96 resonances. Radiation from both faces, plus a wall reflection 0.2 m behind as a
   delayed, mirrored set of patch taps (the patch ring buffer already exists).
4. **Honky-tonk design = upright design plus:**
   - `mistune` pattern: centre true, outer strings at +s and −s c (bichords: one string +s).
     s = 6 c at A4, 4 c in the bass, 8 c in the treble; per-key random ×(0.6–1.4). With
     probability 0.05 per string, add a 15–30 c "screamer". Plain-wire unisons only by default (the
     Abbey Road recipe).
   - The whole piano 10–30 c flat, as an option. A440 is fine for GM.
   - Worn-hammer voicing (3.3), old-string losses and false beats.
   - The mandolin-rail tack switch (section 4) as an option.
5. **Cost.** The upright has about as many strings as the grand. Its stiffer bass needs fewer
   modes below the same frequency limit. Expect the grand's 8 % of a core for one note and up to
   about 29 % with the pedal down, or a little less. Only the active design is built.
6. **Validation.**
   - Upright recordings: `tools/pianofit.py` already measures B per register, the tuning stretch
     and partial decays. Targets: B 2–6e-4 in the bass and tenor, falling onto the grand's law
     above C5.
   - Honky-tonk: the same tool, plus a beat-rate estimate per partial. Fit the envelope
     modulation of partials 1–4 over the first 2 s. The spread s (c) per register is the fitted
     quantity. Expect 0.5–2 Hz beats on the fundamentals of the middle register.
   - Listening: a ragtime bass line (octaves in the left hand) should beat without losing pitch;
     past 10–15 c it turns into a chorus effect.

## 4. Tack piano, mandolin rail, prepared piano (brief)

- **Tack piano.** Tacks or nails are pushed into the hammer felt, so the tone is "brighter and
  more percussive" **[web]**. Technicians discourage it: the felt splits and tacks fall into the
  action **[web]**. Alternatives used: lacquer-hardened hammers (1 part lacquer to 5 thinner, 3
  coats, lighter on the bass), or a mandolin rail **[web, ragpiano]**.
- **The contact, in numbers [calc]:**
  - Steel on steel, Hertz: K = (4/3) E* √R, about 2.5–4.6 × 10⁹ N/m^1.5 for an effective radius of
    0.3–1 mm.
  - A 0.3 g tack against a rigid string would be in contact for about 16–21 µs at 1–4 m/s.
  - But the string is not rigid. Three strings at 5–10 kg/s each present 15–30 kg/s, and a tack of
    0.1–0.3 g gives up its momentum in m / Z = 3–20 µs.
  - Either way the contact is under one sample at 48 kHz: an impulse with a spectrum flat to
    beyond 10 kHz. Compare 0.5–4 ms for felt (piano.md).
- **What the hammer becomes.** The tack is a small mass on the felt spring. The felt still decides
  how much of the hammer's 4–11 g follows behind, so loud blows give a felt "thump" after the
  metal "tick". The tack may bounce and touch twice, which gives the jangle.
- **Plan.** In `Piano::hammer`, an optional second contact: tack mass m_t (0.1–0.3 g), a Hertz
  steel contact (K about 3e9, α = 1.5) with the strings, and the felt law between the tack and the
  hammer body. Two coupled implicit equations per key, which is still cheap. Because the contact
  lasts a fraction of a sample, either sub-step the tack (×8 oversampling during contact only) or
  replace it with a momentum-exchange impulse plus a felt follow-through. The second is cheaper
  and probably enough.
  - Mandolin rail: the same, with tab mass 0.3–0.5 g on a compliant canvas finger. It rattles: add
    a light restitution so it rebounds once or twice.
  - Validate the attack spectrum against a tack-piano recording (a flat to rising 2–8 kHz attack
    within 5 ms) and the ratio of tick to thump against velocity.
- **Prepared piano (Cage, *Bacchanale* 1940).** Screws, bolts, nuts, rubber and plastic go
  between or on the strings **[web]**. A composer's guide advises soft metals (brass, copper,
  aluminium) and steel only on plain strings **[web]**.
  - Physics: a point mass, or a point of friction or contact on one string. A mass at x shifts
    every mode by its shape there, and splits the series into two inharmonic sets (the sub-lengths
    either side). A rubber wedge is a damped spring between two strings of a unison. A loose bolt
    is a rattling contact.
  - Model: `ModalSet`'s contact patches (`cShape`, `cHeight`, `cK`) give the buzz and rattle. A
    mass is best done by recomputing the string's modes with the mass, a small eigenproblem per
    prepared string, offline.
  - Out of scope for GM. It belongs to an "effects" patch later.

## 5. Toy piano

**The object.**
- Round steel rods clamped at one end into a cast-iron bar, struck by small hammers through a
  celesta-like key mechanism **[web, Logos; Wikipedia]**. Metal replaced the original glass bars
  (Schoenhut) **[web]**. Alice Bennett patented the metal-rod design in 1930 **[web]**.
- Ranges: Schoenhut 37-key F3–F6 (written), 30-key C4–F6. Typically 1–3 octaves, rarely near
  A440 **[web]**.
- One restored toy piano: notes 72–107. Original rods 2.5 mm hardened steel (brittle). The square
  clamp end is tapered to 1.5–1.6 mm. Note 76 (E5, 659 Hz) is 215–227 mm long, and f·L² = 33.96
  Hz·m² **[web, Logos]**. Another replacement rod: 3.2 mm × 124 mm **[web, forum]**.
- A brass rod of the same size sounds "about a fifth lower" **[web]**. Bending speed in brass
  against steel, √(E/ρ), is 0.68: −6.7 semitones **[calc]**. This confirms that the rods sing in
  bending.

**Modes [calc].** Clamped-free: 1 : 6.267 : 17.55 : 34.39 : 56.84 (also **[web, Logos]**).

| Rod | f1 | f2 | f3 | f4 | f5 (Hz) |
|---|---|---|---|---|---|
| 2.5 mm, 215 mm | 38 | 239 | **670** | 1313 | 2171 |
| 2.5 mm, 227 mm | 34 | 215 | 601 | 1178 | 1947 |
| 3.2 mm, 124 mm | 147 | 921 | 2579 | 5053 | 8354 |

- E5 = 659 Hz matches f3 of the 215 mm rod (−1.8 %, as a tapered root would lower it). So **on
  that instrument the note is the third bending mode** **[calc]**. Relative to it, the other
  modes sit at 0.057, 0.357, **1**, 1.96, 3.24:
  - a "sub" 17.8 semitones below,
  - an octave 35 c flat,
  - a partial at 3.24 × (an octave plus 834 c: between a minor and a major sixth above the octave).

  The 3.2 × 124 mm rod puts f2 at A#5 and f3 at E7. Which mode carries the pitch may differ
  between makers. **To be settled on a recording:** look for a partial at 0.357× or 2.80× the
  nominal.
- The board is tiny, so the fundamental is "far too low to be successfully radiated", and "mainly
  certain upper overtones are heard" **[web, Ruiz & Tewari]**. "Each individual set of rods has its
  own inimitable potpourri of overtones" **[web, Tan]**.
- Mounting: an elastically mounted board sounded worse than one clamped at two points **[web,
  Logos]**. The clamp's compliance shifts the ratios: the kalimba FE table (kalimba_fiddle.md, A2)
  gives the size of the effect.

**Model plan.**
- `ModalSet`, one vibrator per rod (pool of up to 32; a toy piano has 25–37 keys).
  `cantileverRatio()` gives the modes. Tuning sets the chosen mode (3 by default) to the note.
  Per-rod jitter: ratios ±2 %, tuning ±15 c.
- Modes: 5–6 per rod, both directions with a small split (round rods have degenerate pairs, so
  the split comes from the clamp and makes a slow beat). Decay: steel rod modes ring for seconds
  **[memory]**. Plan T60 is 3–6 s for the note mode and shorter for higher modes; the fundamental's
  radiation is near zero (`rad` small) but it is kept, because it loads the board.
- Excitation: `strike` with a small, hard hammer: wood or plastic, 1–3 g, K about 1e8–1e9,
  α = 1.5–2.5. The robot used 10 mm wooden balls **[web]**. Strike near the free end.
- Body: 4–8 board and box modes (300 Hz–3 kHz, Q 10–30) at the bridge (`setBody`), plus a clack
  `knock` for the key and hammer noise. No dampers: toy pianos have none **[memory]**. The note
  stops when the next blow lands or decays naturally.
- Cost: about 2 × 37 × 6 = 440 oscillators: 0.7 % of a core at 0.33 ns per oscillator-sample
  (piano.md 7a) **[calc]**.
- Validate against a toy-piano recording: the ratio set per note (fit the 0.357 / 1.96 / 3.24
  family), decay per mode, and attack noise.

## 6. GM 9 Celesta

**The instrument.**
- Mustel, Paris, 1886: 49 bars, four octaves at first; now 4, 5 or 5½ octaves (C4–C8, C3–C8,
  C3–F8 sounding). It is written an octave below sounding and tuned at 442–446 Hz **[web, VSL,
  UBC]**. Makers today: Schiedmayer, Yamaha **[web]**.
- Bars: steel alloy, 20–30 mm wide, about 2 mm thick in the treble and 2.5 mm in the bass. They
  are supported at the nodes, 22.4 % of the length from each end. The low bars carry end weights
  **[web, UBC from Yamaha]**.
- Partials of a bar struck at the centre: 2.76 and 5.40 × the fundamental. The bars are not
  undercut to tune the overtones, unlike marimba bars (about 1 : 4 : 10 **[web, ISMA 2019]**). The
  resonators boost the fundamental, and larger bars have more inharmonic content **[web, VSL]**.
- Resonators: one wooden box per bar, tuned to the bar's fundamental. The low ones are in two
  rows **[web, VSL]**.
- Hammers: felt on wood, larger and thicker-felted towards the bass **[web, VSL]**.
- Dampers: one felt damper per bar, lifted by its key. One pedal lifts them all **[web, UBC]**.
- Decay: "a little shorter" than a piano in the same register; lower notes ring longer **[web,
  VSL]**. No measured T60 was found.

**Bars, in numbers [calc].** Free-free Euler-Bernoulli: 1 : 2.757 : 5.404 : 8.933 : 13.34. The
nodes of mode 1 are at 0.224 L, which matches the 22.4 %. Uniform bars, 25 mm wide:

| Note (sounding) | h (mm) | L (mm) | Mass (g) | First torsion (Hz) | Torsion / f1 |
|---|---|---|---|---|---|
| C3 131 Hz | 2.6 | 321 (shorter with end weights) | 164 | 1027 | 7.9 |
| C4 262 Hz | 2.5 | 223 | 109 | 1425 | 5.45 |
| C5 523 Hz | 2.35 | 153 | 70 | 1954 | 3.73 |
| C6 1047 Hz | 2.2 | 104 | 45 | 2673 | 2.55 |
| C7 2093 Hz | 2.1 | 72 | 30 | 3693 | 1.76 |
| C8 4186 Hz | 2.0 | 50 | 20 | 5097 | 1.22 |

- Torsion (thin strip, c_t ≈ 2 (h/w) √(G/ρ)) is not excited by a strike on the centre line. An
  off-centre hammer excites it, and in the top octave it falls close to the fundamental. Include
  one torsion mode with a small, per-bar random excitation.
- End weights on the bass bars lower f1 more than f2 and f3, so they spread the ratios upwards. The
  kalimba FE table shows the same effect for tip mass.

**The resonator.**
- A tuned box under a bar couples to the fundamental. It radiates it better: the bar's tiny
  dipole becomes a monopole at the box mouth. It also takes energy, shortening the fundamental's
  decay. A box mistuned by a few per cent from the bar gives a slow beat in the decay **[memory, as
  for marimba and vibraphone resonators]**.
- Plan, cheap version: per bar, `rad[0]` × 10–30 against the higher modes (the box), and the
  fundamental's T60 shortened by the box's radiation loss. Fitted version: the box as a second
  oscillator coupled to mode 1 (Helmholtz, Q 20–40 **[memory]**). One more mode per bar; in
  `ModalSet` it can be vibrator mode 0 with a `Quad` set to zero and a linear coupling added.
  This is the one engine change.

**Model plan.**
- `ModalSet` × 2 (32 vibrators each, 54–66 bars). Or one set with 32 voices and bar reassignment,
  as the tinkle bell pool does. Per bar: modes 1, 2.76, 5.40, 8.93, 13.3 (cut at 18 kHz), plus one
  torsion mode and the box: about 6–7 modes, one direction (`one = true`, flat bars).
- Hammer: `strike(mass, speed, K, alpha)`. Mass 3–8 g (bass to treble) **[memory]**, felt K about
  1e8–1e9 with α 2.3–2.8 (piano treble felt, softer) **[memory]**. Strike at the centre:
  exc ∝ mode shape at 0.5 L, so even modes are absent at the centre for a free-free bar. Only
  modes 1, 5.40 and 13.3 are excited by a perfectly central blow. Offset the hammer by 3–8 % of L
  so 2.76 appears weakly, as the measured 2.76 partial **[web]** says it does.
- Decay (plan, until measured): T60 of mode 1 at 3–6 s at C4 falling to 0.5–1 s at C8; higher
  modes faster (steel loss plus support loss at the cords, rising with frequency); the box shortens
  mode 1.
- Dampers: `damp(i, rate)` with rate 20–60 /s (a 0.1–0.3 s release) when the key rises without
  the pedal. The pedal is `setHold`, as in `PianoPlayer`.
- Key and action noise: a soft `knock` per note, into a 4–6 mode case (`setBody`). The case also
  carries the sympathetic sound of other bars when the pedal is down, but weakly. The bars do not
  share a bridge.
- Cost: 66 bars × 7 modes = 460 oscillators: about 0.8–1.5 % of a core with the engine's
  per-vibrator overhead **[calc]**.
- Validate against celesta recordings (orchestral, e.g. the Sugar Plum Fairy, if present in
  `~/Music`):
  - partial ratios per note: 2.76 and 5.40, how strong 2.76 is,
  - T60 of the fundamental and of 5.40 per octave,
  - the box beat if visible,
  - release time,
  - hammer brightness against velocity: the 5.40 partial's level against loudness.

## 7. GM 11 Music Box

**The object.**
- A steel comb, each tooth a cantilever plucked at its tip by a pin on a cylinder or disc.
  Pitch by length. A tooth's lowest mode is 3.52 / (2π L²) √(EI / m) **[web, kaistale]**.
- Bass teeth carry lead weights at the tips, which lower the pitch; trimming the lead raises it.
  Unleaded teeth are tuned by thinning near the tip (pitch up) or at the base (pitch down) **[web,
  MMD]**.
- Dampers: wire or spring dampers on the bass teeth stop a pin meeting a ringing tooth (the
  "squeak") **[web, MMD; memory]**.
- Neighbouring teeth couple through the comb's base. A ringing tooth can cancel a neighbour **[web,
  MMD; kaistale]**.
- The 18-note Sankyo movement is the standard small one. Its comb is tuned per tune, with repeated
  notes rather than a chromatic scale **[web]**.
- Combs are tuned "very wildly stretched" **[web, MMD]**. Several teeth per note, tuned slightly
  apart, give a chorus **[memory]**.

**Modes [calc, via the kalimba table].**
- Uniform tooth: 1 : 6.27 : 17.55.
- With a tip mass of 10–20 % of the tooth: 1 : 6.5–7.0 : 18.7–20.5.
- Root compliance (the comb's base) pulls them down.
- Small comb teeth for the C5–C7 octaves are about 10–25 mm long and about 0.5–1 mm thick
  **[memory]**. With 815 h / L² (the kalimba formula): h = 0.7 mm, L = 14 mm gives about 2.9 kHz
  **[calc]**. The upper teeth sit where f2 is above 15 kHz, so they are nearly pure sine tones.

**Model plan.**
- Reuse the kalimba patch on `ModalSet`: one vibrator per tooth in use (18–36), 3–4 modes plus a
  y/z split of a few tenths of a Hz (shimmer).
- `pluck(force, rise, hold, slip)`: rise 1–2 ms (the pin deflects the tooth), hold 0, slip
  0.05–0.2 ms (the pin slides off the tip), so the attack is a bright click. Force so that the
  tip moves 0.1–0.3 mm.
- Decay: steel teeth T60 of 2–8 s for f1 (long for the bass with lead, shorter in the treble)
  **[memory]**. f2 decays several times faster, as kalimbas do **[web, Chapman via
  kalimba_fiddle.md]**.
- Bedplate and box: 4–8 body modes (400 Hz–3 kHz, Q 15–40). Little radiation below about 300 Hz.
  The bass relies on its f2 and on the box.
- Damper: when a key repeats, `damp(i, 50 /s)` for 20–30 ms before the new pluck. Optional
  squeak: a short `knock` if the tooth's energy is still above a threshold when the pin lands.
- GM keyboard mapping: one tooth per key over the comb's range, stretched tuning (+5 to +15 c per
  octave above C6) and a duplicate tooth per note at ±2–4 c, for the chorus.
- Cost: 36 teeth × 4 modes × 2 directions ≈ 290 oscillators: under 0.5 % of a core **[calc]**.
- Validate against a music box recording: f2/f1 per note (6.3–7.0), f1 decays, the attack click
  spectrum, and the beat between duplicate teeth.

## 8. Summary: reuse, change, cost

| Instrument | Engine | New code | Main parameters | Cost (one core) |
|---|---|---|---|---|
| Bright piano (GM 2) | `Piano` (grand design) | voicing table, close-mic bridge pickup | K × 2–4 from key 30 up; mics 0.25 m over the hammers | 8 % one note, 29 % pedal |
| CP-70 (GM 3) | `Piano` | rigid-bridge "board", piezo output | CP scale (bass 0.68 m, 1–2 strings), B about 7e-4 at A0 | about 5–20 % |
| Upright | `Piano` with a `PianoDesign` | design struct; upright board from `pianoboard.py` | lengths 1.19 m down; B 2–6e-4 in the bass; 1.38 m² board; wall reflection | about as the grand |
| Honky-tonk (GM 4) | upright design | mistuning pattern, worn felt, false beats | ±4–8 c outer strings, random screamers; K × 2–3; loss × 1.5–2 | as the upright |
| Moderator | `Piano::hammer` | series felt spring | strip K about 1/10–1/30, hamW 20–30 mm | none |
| Tack / mandolin rail | `Piano::hammer` | tack mass and Hertz steel contact | m_t 0.1–0.5 g, K about 3e9, α 1.5 | none |
| Toy piano | `ModalSet` | patch only | clamped-free ratios, note on mode 3 (or 2), hard small hammer | under 1 % |
| Celesta (GM 9) | `ModalSet` × 2 | box-resonator coupling (optional) | free-free 1 : 2.76 : 5.40 : 8.93, torsion, felt hammer, dampers, pedal | 1–2 % |
| Music box (GM 11) | `ModalSet` | patch only | cantilever 1 : 6.3–7 : 18–20, fast pluck, duplicate teeth | under 0.5 % |

Order of work: (1) `PianoDesign` refactor with the grand as the first design, bit-exact against
the present output; (2) bright preset; (3) upright board and scale, checked against Ege &
Boutillon; (4) honky-tonk tuning and hammer wear; (5) celesta, toy piano, music box as
`ModalSet` patches, in parallel; (6) moderator and tack hammer.

Main gaps (most important first):
- Measured upright B per register. Dalmont 2021 and the UBC thesis on two uprights have it, but
  could not be read here.
- Celesta decay times and the box's coupling.
- Which bending mode carries the pitch on common toy pianos.
- Measured honky-tonk spreads from recordings, not from tuners' recipes.
- Music box tooth dimensions.

## 9. References

- J.-P. Dalmont, S. Maugeais, "Piano bass strings with reduced inharmonicity: theory and
  experiments", Acta Acustica 5, 9 (2021). https://acta-acustica.edpsciences.org/10.1051/aacus/2021002
  (abstract only; the C109 figures came via a secondary summary)
- K. Ege, X. Boutillon, "Synthetic description of the piano soundboard mechanical mobility", ISMA
  2010 (the upright board). https://arxiv.org/pdf/1210.5688
- Yamaha U-series specifications (U1, U3 string lengths and boards), via dealers:
  https://usa.yamaha.com/products/musical_instruments/pianos/upright_pianos/u_series/specs.html ;
  https://www.thepianoshopbath.co.uk/?p=14868 ; https://muex.com/blog/yamaha-u1-vs-yamaha-u3/
- G. Weinreich, "Coupled piano strings", KTH five lectures.
  https://www.speech.kth.se/music/5_lectures/weinreic/mistuned.html
- H. A. Conklin, "The hammers", KTH five lectures.
  https://www.speech.kth.se/music/5_lectures/conklin/thehammers.html
- "Evaluating musical instruments" (1.5 c unisons), Physics Today.
  https://physicstoday.aip.org/features/evaluating-musical-instruments
- PTG discussion, "Winifred Atwell's piano formula for tuning honky tonk".
  https://my.ptg.org/discussion/winifred-atwells-piano-formula-for-tuning-honky-tonk
- Rag piano FAQ, "What gives some pianos a Honky Tonk sound?" https://ragpiano.com/help/faq9.htm
- Mechanical Music Digest, 2011.09.07 (untuned pianos, honky-tonk).
  https://www.mmdigest.com/Archives/Digests/201109/2011.09.07.05.html
- r/piano, "I had a piano technician detune my piano…" (8–10 c, anecdotal).
  https://redlib.belloworld.it/r/piano/comments/1db4uot/i_had_a_piano_technician_detune_my_piano_to_sound
- Bechstein piano guide, moderator. https://www.bechstein.com/en/service/piano-guide/moderator-silent-system/
- PTG, "upright felt mute rail adjacent notes". https://my.ptg.org/discussion/upright-felt-mute-rail-adjacent-notes
- Pianoteq 7.4 felt pianos and continuous celeste pedal.
  https://www.kvraudio.com/news/modartt-updates-pianoteq-to-v7-4---adds-twelve-felt-pianos-52179
- Yamaha, "6000 parts, 230 strings…" (hammer grooves).
  https://au.yamaha.com/en/education/greatstart/articles/for_everyone/pianos-up-to-the-task-of-music-education.html
- ISMA 2019, piano ageing study (Hamburg). https://pub.dega-akustik.de/ISMA2019/data/articles/000005.pdf
- Yamaha CP-70. https://en.wikipedia.org/wiki/Yamaha_CP-70
- Yamaha C7 in studios: https://www.electricalaudio.com/instruments-1/p/yamaha-c7 ;
  RE/P 1977 via https://www.prosoundweb.com/?p=39655 ;
  https://markgoodwinpianos.co.uk/blog/add-more-punch-piano-voicing
- Tack piano and prepared piano: https://en.wikipedia.org/wiki/Prepared_piano ;
  https://cavac.at/cavacopedia/Tack_piano ; https://newmusicusa.org/?p=327739
- Toy piano: https://en.wikipedia.org/wiki/Toy_piano ; Logos Foundation robot toy piano
  https://logosfoundation.org/instrum_gwr/toypi.html ; M. Ruiz i Carulla, S. Tewari,
  "Odontophones" (2020) https://researchcatalogue.net/view/932703/939567 ;
  M. L. Tan, https://www.matafestival.org/news/800 ;
  replacement rod https://groups.google.com/g/toypianotime/c/62ZUrgw9eoo
- Celesta: Vienna Symphonic Library https://www.vsl.co.at/academy/percussion/celesta ;
  UBC PHYS 341 wiki (from Yamaha and Rossing, Springer Handbook of Acoustics 2007, pp. 649–650)
  https://wiki.ubc.ca/Course:Phys341_2020/Celeste ;
  Yamaha https://www.yamaha.com/en/musical_instrument_guide/celesta/mechanism/mechanism005.html
- Marimba bar tuning (1 : 4 : 10): ISMA 2019. https://pub.dega-akustik.de/ISMA2019/data/articles/000040.pdf
- Music box: MMD 2014.02.01 (lead weights, tuning) https://www.mmdigest.com/Archives/Digests/201402/2014.02.01.01.html ;
  kaistale blog (comb simulation) https://blog.kaistale.com/?p=1568 ;
  UT demo https://lecturedemos.ph.utexas.edu/demo/309 ;
  Sankyo 18-note https://www.mmdigest.com/Archives/Digests/201410/2014.10.03.01.html
- Within the project: `research/piano.md`, `research/kalimba_fiddle.md` (cantilever FE table),
  `research/bells_blocks.md` (voice pool), `src/piano.h`, `src/modal.h`.
