# Guitars (GM 25–32) and electric basses (GM 34–38) as physical models — research and plan

Status: 2026-11-16. The classical guitar (GM 25) is built (section 13); the rest is research.
SI units.

Marks:
- **[paper]**: read in the source (or its abstract), listed at the end.
- **[web]**: a manufacturer's page, a forum or a lab write-up.
- **[calc]**: worked out here; the inputs are given.
- **[memory]**: general knowledge or a classic source not re-read for this note; to be checked.
- **[design]**: a choice of this plan.
- **[files]**: measured in the user's MIDI files.

## 0. Short answer

- **Base the guitar on the plucked double bass (`src/bass.h`).** It already has the mechanisms a guitar
  needs: modes in two polarisations per string, a shared bridge (open strings ring along), tension
  modulation, a fingerboard contact (buzz, slap), stopped strings, slides and hammer-ons (`setLength`),
  harmonics (`touch`) and finger plucks. What is new:
  - six fretted strings (a fret is a hard metal stop, not a fingertip);
  - a guitar body: a top plate and a Helmholtz cavity, or a solid body for electrics;
  - nail and pick plucks;
  - palm muting;
  - for electrics, magnetic pickups and an amp. The pickups are the clavinet's (`src/clavinet.cc`); the
    amp's tone stack is an exact filter as in `tools/clavtone.py`, and its tubes are circuits in the
    style of `src/lead.cc`.
- **Cost is not the problem.** Six strings × 2 × 48–64 modes ≈ the double bass (four strings)
  plus 50 %. That is about 3–6 % of a core per guitar [calc, from the bass and the clavinet's 5.5 % for 60
  strings]. A finite-difference guitar with fret collisions runs at 11–18 % of a core *per string*
  without a body [paper, BRWD24]: the modal route is ~10× cheaper.
- **Most of the realism is in the player, not the string.** Every guitar part in the user's files writes its
  chords as exactly simultaneous notes (§7) [files]. The model must:
  - choose strings and frets;
  - strum or pinch the chord with its own timing;
  - damp strings when their keys are released;
  - fold notes outside the guitar's range.
- **Priority** (the user's files):
  1. **nylon (GM 25):** 5 800 notes in 5 files: bossa comping, dyads with the pedal;
  2. **muted electric (GM 29):** 5 800 in 2: comping chords, and one bass line below the guitar's range;
  3. **jazz archtop (GM 27):** 1 700 in 1: chords up to 7 notes, range 39–85;
  4. the rest (26, 28, 30–32) are not used in the files.
  - **Basses:** GM 33 (acoustic, 62 000 notes) and GM 36 (fretless, 25 000) already play the double bass.
    In these jazz files the fretless *is* an upright stand-in (the user's own piano-and-bass file sets it),
    so keep it there. Finger, pick and slap electric basses (GM 34–38: 2 500 notes) are a small job on the
    same base (§10).

## 1. The strings

### 1.1 Geometry and tension [calc unless marked]

The tension follows from T = μ (2 L f₀)², and the inharmonicity from B = π³ E d⁴ / (64 T L²). For wound
strings, d is the core's diameter; the tension is the set maker's figure [memory] and μ is backed out from
it.

| Guitar (scale) | String | Kind, gauge | μ (g/m) | T (N) | B |
|---|---|---|---|---|---|
| Classical (650 mm) | E4 | nylon .028" (ρ 1140, E 4.5 GPa) | 0.45 | 83 | 1.6e-5 |
| | B3 | nylon .032" | 0.60 | 62 | 3.7e-5 |
| | G3 | nylon .040" | 0.94 | 61 | 9.3e-5 |
| | D3, A2, E2 | silver-plated copper on a nylon multifilament core | — | 60–75 [memory] | ~1e-5 (a soft core) |
| Steel acoustic (645 mm, light 12–53) | E4 | plain .012 | 0.57 | 104 | 1.9e-5 |
| | B3 | plain .016 | 1.02 | 103 | 6.1e-5 |
| | G3 | wound .024 | 2.1 | 133 | 1.1e-5 |
| | D3 | wound .032 | 3.8 | 136 | 2.0e-5 |
| | A2 | wound .042 | 6.9 | 139 | 4.6e-5 |
| | E2 | wound .053 | 10.1 | 114 | 1.4e-4 |
| Electric (648 mm, 10–46) | E4 | plain .010 | 0.40 | 73 | 1.3e-5 |
| | B3 | plain .013 | 0.67 | 69 | 4.0e-5 |
| | G3 | plain .017 | 1.15 | 74 | 1.1e-4 |
| | D3 | wound .026 | 2.3 | 82 | 1.7e-5 |
| | A2 | wound .036 | 4.3 | 87 | 4.3e-5 |
| | E2 | wound .046 | 6.8 | 78 | 1.0e-4 |
| Jazz archtop (628 mm, flatwound 12–52) | E4 | plain .012 | 0.57 | 98 | 2.2e-5 |
| | B3 | plain .016 | 1.02 | 98 | 6.8e-5 |
| | G3 | wound .024 | 2.1 | 125 | 1.7e-5 |
| | D3 | wound .032 | 4.1 | 138 | 3.8e-5 |
| | A2 | wound .042 | 7.7 | 147 | 7.3e-5 |
| | E2 | wound .052 | 12.5 | 133 | 1.5e-4 |

- **A check against a measurement:** a measured G3 (.017", 0.629 m, an electric) has μ = 1.1 g/m and
  T = 69.4 N [paper, PLC14 Table I]. That is the table's row within 5 %.
- **Totals:** classical about 400 N; steel acoustic about 730 N; electric about 460 N; flatwound archtop
  about 740 N.
  - The steel acoustic's tension is nearly twice the classical's.
  - Nylon's lower impedance √(Tμ) gives a softer attack and a stronger coupling to the top. That is why a
    classical guitar is louder per string energy and dies sooner.
- **Fret positions:** x_q = L (1 − 2^(−q/12)) [paper, BRWD24]. The classical neck has 19 frets, electrics
  21–24.

### 1.2 Losses

- **Three loss mechanisms in the string itself:** air (viscous and radiation), internal friction of the
  material, and friction between the winding's turns. For a plain steel string under 1 kHz, air
  friction dominates [paper, PLC14].
  - PLC14's fit for the G3 above: δ_VE/TE = 1e-3 (published values range 1e-4…1e-3), Q_disl = 5500,
    ρ_air 1.2, η_air 1.8e-5 kg/(m s).
  - Bronze-wound strings: the friction between turns dominates the winding's loss. Material damping grows
    with frequency and acts as a viscous term [paper, C11].
- **The ends.** Energy also leaves through the bridge into the body, and through the fret and the neck.
  - On a solid-body electric, the *neck* end matters most. The out-of-plane conductance at the neck reaches
    0.1 m/(s N), while at the bridge it stays under 1.8e-3. Where a neck resonance meets a partial, the note
    dies early: a "dead spot" [paper, PLC14].
  - The pickup adds no damping [paper, PLC14].
- **Decay times:** a body-coupling model gives a sustain (2τ) of about 1.1 s for the high E and 7.8 s for
  the low E [paper, LEI].
- **Classical guitar:**
  - fundamentals: T60 of about 3–8 s on the bass strings and 1–3 s on the trebles;
  - partials above 2 kHz: 0.2–0.5 s;
  - nylon's internal loss is far higher than steel's, so a nylon string is darker after the attack
    [memory].
- **In the model:** each mode's own decay, of the form loss0 + loss1 k^1.2, as `BassString` has. The
  body's share goes through the bridge coupling, as in the bass. Fit per string to recordings (§9).

### 1.3 Two polarisations

- **Different admittances.** The string's two directions of swing meet different admittances at the
  bridge:
  - normal to the top it is high (the top moves easily);
  - along the top it is low.
- **Consequences** [memory; Woodhouse 2004]:
  - a "double decay": the part normal to the top dies fast, and what was set along the top rings on;
  - beating between the two slightly different frequencies.
- **On the electric guitar:** the pickup is "far more sensitive to the out-of-plane polarization" [paper,
  PLC14]. So the release angle of the pluck sets the electric's attack-to-sustain balance.
- **Already in `Bass`:** two polarisations with split frequencies (up to 0.3 %) and a slowly turning plane.
  For a guitar, the release angle becomes a playing parameter: a nail or pick sends it mostly along the
  top, a downward rest stroke (apoyando) more normal to it.

## 2. The pluck and the left hand

### 2.1 Plucking

| | Flesh | Nail | Pick |
|---|---|---|---|
| Contact width | 8–12 mm | 1–2 mm (the edge) | 0.5–1 mm |
| Slip-off time | ~2–5 ms (soft) | ~0.5–1 ms | ~0.1–0.5 ms |
| Spectrum | dark | bright | brightest; with a "click" |
| Source | [memory; Pavlidou 1997] | [memory] | [memory] |

- **Pluck position.** The partials whose node is at the plucking point are missing: a comb with notches at
  multiples of L/p [calc].
  - At 12 cm on a 650 mm scale, the notches fall at the 5.4th harmonic and its multiples.
  - Sul ponticello, near the bridge: bright and thin. Sul tasto, near the neck: round.
  - PLC14 plucked at 12 cm with a pick.
- **The stroke's phases.** A pluck has a sticking phase and a slipping phase, measured on the harp
  [paper, CLF12]. A guitar's finger/string interaction is described in Pavlidou's thesis [memory]. Motion
  capture separates contact, pressure and release, and nail from flesh [paper, PC19].
- **Classical strokes:**
  - **rest stroke (apoyando):** the finger ends on the next string, more normal to the top: louder,
    rounder;
  - **free stroke (tirando):** more along the top;
  - **the thumb (p):** plays the bass strings;
  - **i, m, a:** the trebles.
  - In bossa, the thumb plays the bass on 1 and 3 while i-m-a pinch the chord, nearly together (0–10 ms)
    [memory].
- **Strumming.** A pick or fingers cross the strings in about 5–30 ms per stroke [memory]: down from the
  low string, up from the high. A downstroke on the beat and an upstroke on the "and".
- **Palm muting.** The edge of the picking hand lies on the strings 0–2 cm from the saddle [web, a
  description of the technique].
  - It damps every mode, more the higher the mode: the strings move little there, but the hand's flesh is a
    lossy load on the near-node motion.
  - Most of the attack's highs survive, the ring is short (~50–150 ms), the pitch stays clear.
  - The model: a lossy contact patch near the bridge on each string (`ModalSet::damp`-like, weighted by
    each mode's slope at the hand) [design].
- **Harmonics:** a light finger at L/2, L/3, L/4 for a moment after the pluck. This is `Bass::touch(s, n)`.
  GM 32, "guitar harmonics", is a patch of such notes.

### 2.2 Left hand

- **The fret.** A fretted note ends on a fret wire, metal on metal: little loss, unlike the double bass's
  fingertip. The finger behind it adds a little loss and holds the string down.
  - Releasing the note while the string still rings: the finger lifts, and the string either falls back to
    the open string (the open note sounds briefly) or is damped by the lifting finger [memory].
  - In MIDI this is the note-off. It should mostly *damp*: the finger lifts while still lying on the
    string.
- **Fret buzz.** Low action or a hard pluck makes the string hit the frets above the stopping fret, a
  rattle. Modelled as a contact against the fret tops, as the bass's fingerboard contact [paper, BT14;
  BRWD24].
- **Hammer-on, pull-off, slide:** `Bass::setLength(s, len, glide)`.
  - A hammer-on adds a small excitation, the finger striking the string onto the fret.
  - A pull-off is a small pluck by the left-hand finger.
- **Bend and vibrato.** A bend stretches the string: Δf/f ≈ ½ ΔT/T for small bends. A semitone needs
  +12 % tension [calc]. In MIDI, pitch bend, a ±2 semitone range, mapped onto the string's tension.
- **Damping an open string** is the hand lying lightly on it (left or right).

## 3. The body

### 3.1 The acoustic guitar (classical, steel)

- **The lowest resonances:**

| Mode | Frequency | What moves | Source |
|---|---|---|---|
| A0 / T(1,1)₁ | 90–110 Hz | the air through the soundhole with the top, in phase (Helmholtz) | [paper, DAGA21; RJJP] |
| T(1,1)₂ | 180–230 Hz | the top's own (1,1), out of phase with the air | [paper, R; DAGA21] |
| T(1,1)₃ | 250–300 Hz | the back's | [paper, POMA] |
| (2,1), (1,2) top | 300–500 Hz | | [memory] |
| A1 | 350–450 Hz | the second air mode | [paper, DAGA21] |

  - One measured classical guitar: 96, 196 and 294 Hz [paper, POMA].
  - The Helmholtz resonance splits the top's single resonance into the two peaks, about 100 and 230 Hz
    [paper, RJJP].
- **The two-oscillator model.** The top, a piston, coupled to the cavity's air through the soundhole
  [memory; Christensen & Vistisen 1980]. Its parameters:
  - effective top mass ~0.05–0.1 kg, stiffness from T(1,1) uncoupled ~150 Hz;
  - cavity volume ~13 L (classical), ~15–20 L (dreadnought);
  - soundhole 85–100 mm in diameter.
- **Bridge mobility (the admittance the strings see):**
  - peaks of about 0.05–0.2 m/(s N) at 100–250 Hz;
  - falling to about 0.01–0.03 above 1 kHz, with dense modes [memory; ranges from Woodhouse & French's
    and Elejabarrieta's measurements, to be checked].
  - The project's bass body (`src/bassbody.h`) is the same structure: modes, each a mass at the bridge, a
    radiation weight and a side. A guitar body is a new table.
- **Radiation:** below ~400 Hz the body is a monopole: the top and the soundhole's air together. Above,
  the top's modes radiate as patterns, so a near microphone hears a strongly position-dependent
  spectrum [memory].
- **Classical vs steel:** a steel-string body is larger, X-braced, and stiffer at the bridge (a heavier top
  for the 730 N). Its main modes sit about 10–20 % higher and its mobility is lower: a brighter, longer
  note [memory].

### 3.2 The archtop (GM 27, jazz)

- **Construction:** a carved, arched top with f-holes, a floating bridge (held only by the strings' down
  force) and a tailpiece, like a violin's. Usually a floating or mounted neck humbucker.
- **Sound:** the floating bridge rocks, and the f-holes let the top flex [web, forum]. Acoustically the
  archtop is punchier and shorter-sustaining than a flat-top. The jazz sound is mostly *through the
  pickup*: flatwounds, the neck pickup, the tone rolled off, a clean amp [memory].
- **No published mode table found.** A modal analysis of free archtop top plates exists [paper, SGAS21],
  but those are free plates, not assembled guitars. Guess the A0 at ~110–130 Hz and the top's main mode at
  ~200–280 Hz [memory].
- **The model:** the electric chain (§4) on a lightly coupled acoustic body, mostly through the pickup,
  with a little acoustic body mixed in.

### 3.3 Solid bodies (GM 28–31)

- **Little body:** the bridge is nearly rigid (conductance < 1.8e-3 m/(s N)). The neck's resonances at
  some notes cause the dead spots [paper, PLC14]. So the strings ring long (§1.2), and the sound is the
  pickup's (§4).
- **The model:** no body radiation. A few neck modes at the nut and fret end, as the coupling that makes
  dead spots (two or three modes, 150–400 Hz) [design].

## 4. The electric chain

### 4.1 Pickups

- **What a pickup is:** a coil (inductance L, a few kΩ of resistance) on a magnet. The string's motion
  over the pole changes the flux.
- **The output** is close to the string's *velocity* at the pickup's position, weighted by the out-of-plane
  polarisation [paper, PLC14]. It is nonlinear in the string's distance from the pole, which adds even
  harmonics on large swings [memory; Horton & Moore 2009].
- **Position.** The pickup hears only the modes that move at its place: a comb with notches at multiples
  of L/x [calc]:
  - bridge pickup ~4 cm from the saddle: notches at the 16th harmonic, bright and thin;
  - neck pickup ~16 cm: notches at the 4th, round.
- **Electrically, a resonant low-pass.** The pickup's L against the winding's (~100 pF) and the cable's
  capacitance (~300–1000 pF), damped by the pots (250 k for single coils, 500 k for humbuckers)
  [web, Wikipedia talk; ozvalveamps; UIUC]:

| Pickup | L (H) | With 600 pF | With 1 100 pF |
|---|---|---|---|
| Single coil | 2–3 | 4.3 kHz | 3.2 kHz |
| Humbucker (series) | 4–8 | 3.1 kHz (4.5 H) / 2.3 kHz (8 H) | |

  [calc, f0 = 1/(2π√(LC))]
  - Turning the volume pot down damps the resonance. The tone pot (~0.022–0.047 µF) rolls off the highs.
  - 95 % of a P-90 single coil's output energy is under 2.5 kHz [paper, PLC14].
- **Already built:** the clavinet's pickups (`src/clavinet.cc`), velocity at a point with the magnet's
  nonlinearity, selector in or out of phase. The guitar reuses that, plus the L–C–R resonance as a biquad.

### 4.2 Amp

- **Tone stack.** The '59 Bassman tone stack (Fender, Marshall and Vox are the same circuit with other
  values) has closed-form digital coefficients as functions of the three knobs [paper, YS06]. That fits
  the way `tools/clavtone.py` turns the clavinet's switches into exact SOS filters.
- **Clean (GM 27, 28, 29):** two 12AX7 gain stages with a mild asymmetric soft clip, a tone stack, a
  push-pull power stage with its output transformer, and a 12" speaker [memory; Pakarinen & Yeh 2009]. The
  speaker in its cabinet: a resonance near 80–120 Hz, a roll-off above ~4–6 kHz, and cone breakup peaks at
  2–4 kHz [memory].
- **Overdrive and distortion (GM 30, 31):** more preamp gain, so the clipping is hard and asymmetric
  (2nd-harmonic rich), plus the power stage's sag. The cabinet's low-pass makes it musical [memory].
  - The project's analog circuits (`src/lead.cc`) already simulate diode and transistor stages; a triode
    stage is a new element there.
  - The speaker cabinet is a fixed IR or a few resonances.
- **Effects in jazz:** a clean amp with little reverb. The room already supplies the space.

## 5. Coupling and sympathy

- **Strings on a shared bridge.** All six strings sit on one bridge. A plucked note drives the open strings
  near its harmonics, so they ring quietly: sympathy. `Bass` does this already (PJ_BASS_SYMP).
- **The player stops most of it.** Strings that are not part of the chord are muted by the hands, so
  sympathy is mostly from open strings in the chord's region. The fingering solver (§7) knows which strings
  are open and undamped [design].
- **On an electric:** the bridge is stiff, so sympathy is weak, but the pickup hears every moving string.

## 6. Published synthesis models and their cost

| Approach | What it gives | Cost per voice (48 kHz) |
|---|---|---|
| Karplus–Strong / waveguide with loop filter | string decay, pluck position; calibrated from recordings | very low (a delay line and two filters per string) [paper, KVT98; LEVK01] |
| Commuted waveguide | body folded into the excitation (a stored body impulse response); the lowest body modes as resonators | low; the body is a table read [paper, Smith; VT] |
| Modal string + modal body (this project) | two polarisations, bridge coupling, sympathy, contacts, tension modulation | ~64–128 modes per string: about 0.5–1 % of a core per string [calc, from the bass/clavinet] |
| Finite-difference with fret/fretboard/finger collisions | rattles, taps, pitch glides, finger buzz, energy-stable | 0.11–0.18 s per 1 s of a 44.1 kHz string, one polarisation, no body (11–18 % of a core per string, M2/Xeon); two polarisations would "roughly double" it [paper, BRWD24] |
| Frequency-domain modal synthesis (Woodhouse) | the most accurate in his comparison; latency makes it unsuitable live | offline [paper, MWS16 citing Woodhouse 2004] |

- **Recommendation: modal** [design], as the project's other strings are.
  - Frets, the fretboard and the finger are contacts, as the bass's fingerboard and the sitar's jawari
    already are.
  - FD is the reference for checking the buzz [paper, BT14; BRWD24].

## 7. From MIDI to a guitarist

### 7.1 What the files contain [files]

| File | Channel / GM | Notes | Range | Chords | Duration (beats, median) | Other |
|---|---|---|---|---|---|---|
| goodbait (×3 copies) | ch3 / 25 nylon | 1274 | 48–60 | all dyads (637 × 2), simultaneous | 0.29 | 718 sustain-pedal events |
| Baby it's cold outside | ch3 / 25 | 974 | 52–67 | 4-note chords, **rolled ~0.025 beat** (~15 ms) | 0.55 | — |
| shinystockings | ch3 / 25 | 1011 | 52–77 | 3–5 notes, simultaneous | 0.41 | — |
| desire | ch4 / 29 muted | 3362 | 45–76 | 3–6 notes, simultaneous | 0.25 | 1 bend (a reset) |
| louisiana | ch5 / 29 | 1705 | 51–**97** | 3–7 notes, simultaneous | 0.33 | above the guitar's top (88) |
| louisiana | ch6 / 29 | 723 | **29**–54 | single notes: a bass line | 0.27 | below the guitar's E2 (40) |
| louisiana | ch7 / 27 jazz | 1705 | **39**–85 | 3–7 notes | 0.33 | one note under E2 |

- Chords are block chords, played at once, except one file that rolls them.
- Note lengths are short: comping.
- No pitch bends of note, and the sustain pedal is used on the nylon in one file.
- Some parts go outside the guitar's range: above 88, below 40, and a 7-note chord is more than six
  strings.

### 7.2 The fingering solver [design; paper, Sayegh 1989; Burlet & Fujinaga]

- **Strings:** standard tuning, open strings E2 A2 D3 G3 B3 E4 = 40, 45, 50, 55, 59, 64 [paper, MIDI-to-Tab].
- **Rules:**
  - one note per string;
  - a new note on a string that is sounding cuts it;
  - a chord's notes spread over strings, with the fretted span ≤ 4–5 frets (5 on a classical, at most 6);
  - the lowest position preferred, open strings preferred on a nylon;
  - continuity with the previous hand position (Sayegh's shortest path; Viterbi over the file, or greedy
    live).
- **Folding outside the range:**
  - notes above the top fret (E6 = 88 on 24 frets, C#6/D6 on 19–21): down an octave;
  - notes below E2: up an octave, or the low E dropped to D (drop-D, 38) if the part sits there;
  - chords over 6 notes: drop the doubled notes first.
  - A part that is a bass line (louisiana ch6: single notes, 29–54) is better sent to the electric bass or
    the double bass than folded. Detect it: monophonic and below 48 [design].
- **In `-play` the whole file is known:** the solver can run over the part beforehand (a Viterbi over
  chord positions), as the drums' song context does.

### 7.3 How a chord is played [design]

- **Simultaneous chord notes** (within 30 ms) become one gesture.
  - **Nylon:** the thumb on the bass note, i-m-a pinching the rest. Spread 0–8 ms, the thumb first.
  - **Electric and archtop:** a strum.
    - Down on the beat, up on offbeats (from the song clock, or alternating live).
    - 4–10 ms per string; faster when louder.
    - An upstroke may skip the lowest strings.
- **Velocity:** the pluck's depth and speed, a brighter release on harder notes, and the nail or pick share.
- **Note-off:** the fretting finger lifts and damps; open strings are damped by the hand.
  - The sustain pedal (CC64) on a guitar means let ring: no damping until the pedal lifts.
  - Muted electric (GM 29): the palm mute on every stroke; note-off barely matters.
- **Pitch bend:** a bend on the sounding strings (tension), or a slide (`setLength`) if larger than a whole
  tone.
- **Live (MPK):** the same, plus:
  - a pad for palm mute;
  - the joystick for bends and vibrato;
  - a knob for the pluck position.

## 8. What the project already has

- **`src/bass.h` (`Bass`):** the guitar's base.
  - Modes in two polarisations with split frequencies and a turning plane.
  - Sympathy through a shared bridge, and tension modulation (the pitch starts sharp on a hard pluck).
  - The fingerboard contact (buzz and slap), stopped length with fingertip loss.
  - `setLength` (slides, hammer-ons), `touch` (harmonics), fingers (pads that pull, hold and slip), thumb.
  - The body (`bassbody.h`: modes as masses at the bridge).
  - **What changes:** 6 strings; the fret as a hard stop (metal: less loss, plus a contact for the buzz); a
    guitar body table; nail and pick plucks (narrow, fast slip); palm mute (a lossy patch near the bridge);
    strings without a tailpiece and afterlength (a classical's tie block; a steel guitar's pins; the
    archtop's tailpiece, which does have afterlength).
- **`src/clavinet.cc`:** modal strings plus *pickups* (velocity at a point, the magnet's nonlinearity, the
  selector) and tone networks as exact SOS filters from a circuit (`tools/clavtone.py`). Reuse it for the
  electric guitars' pickups and for the tone pot and tone stack.
- **`src/lead.cc`:** the analog circuits: diodes, transistors, a shared board. Add a triode stage for the
  amps (GM 30, 31).
- **`src/modal.h` (`ModalSet`):** the generic engine. The sitar's jawari contact (`research/sitar_shamisen.md`)
  is the model for fret buzz and for a fretless bass's "mwah".
- **`research/banjo_koto.md`:** the plucked-string-on-a-body recipe on the `Bass` pattern, including
  per-string decay and a pick. **`research/double_bass.md`:** the fitting procedure from recordings (§5
  there).

## 9. Model plan, in order

### 9.1 Nylon (GM 25), first

1. **Strings:** the classical table (§1.1).
   - Losses: bass strings T60 ~3–8 s (fundamental) and trebles ~1–3 s, both falling as k^1.2 above.
   - To be fitted to a recording.
2. **Body:** A0 100 Hz, T(1,1)₂ 200 Hz, T₃ 290 Hz, A1 ~400 Hz, plus ~20 higher modes 400–3000 Hz
   (density ~1 per 50–100 Hz). Mobility peaks ~0.1 m/(s N). A bridge on the top with no afterlength.
3. **Pluck:**
   - flesh (wide, slow slip) to nail (narrow, fast), 12 cm from the bridge by default;
   - apoyando or tirando from the velocity and the role (the thumb on the bass);
   - the release angle (normal or along the top).
4. **Player:**
   - the solver (§7.2), the pinch (§7.3);
   - note-off damping, the pedal as let-ring.
5. **Checks:**
   - pitch (the tension solved per string, as the toms do);
   - T60 against the recording;
   - the A0 peak in the spectrum of a soft strum;
   - stability on the `-fuzz` test;
   - cost under 6 % of a core.

### 9.2 Muted electric (GM 29), then the clean electric (28)

- **The instrument:** the electric strings (10–46, 648 mm), a solid body with two neck modes (dead spots),
  a pick.
- **Pickups:** the bridge plus neck pair, selector in the middle; L 2.3 H against 700 pF, a 250 k pot.
- **Amp:** a clean amp (tone stack plus a mild soft clip plus the cabinet).
- **Palm mute:** on every stroke for GM 29. A downstroke strum, 4–8 ms per string.
- **Checks:**
  - the palm-muted note's ring: 60–150 ms to −30 dB;
  - the pickup resonance (~4 kHz) in the spectrum;
  - the comb's notch at the 16th harmonic on the bridge pickup.

### 9.3 Jazz archtop (GM 27)

- **The instrument:** the flatwound table, a 628 mm scale, the neck humbucker (L 4.5–8 H), the tone pot
  half rolled (a corner of ~1.5–2 kHz), a clean amp.
- **Body:** an acoustic body share (A0 ~120 Hz, top ~220–260 Hz) mixed under the pickup.
- **Flatwounds:** a lower internal loss in the highs, but a darker string (a smooth wrap, less friction
  noise). The notes ring 2–4 s.
- **Strokes:** a pick, the comping strum (Freddie Green's short quarter chords: note-off damping matters).

### 9.4 The rest (no notes in the files; cheap once the above exist)

- **GM 26, steel acoustic:** the steel table, a dreadnought body (A0 ~100, top ~170–200 Hz, stiffer,
  brighter), a pick.
- **GM 30 and 31, overdriven and distortion:** GM 28 plus preamp gain into triode clipping, and the
  cabinet.
- **GM 32, harmonics:** each note as a natural harmonic on the nearest string node (`touch`).

### 9.5 Measurements to fit against

- **Recordings:** the user has none of solo guitar in `~/Music` that we know of. Candidates among the
  wavefield tracks:
  - *Fields of Gold* (Sting): an acoustic guitar;
  - *Are You Gonna Go My Way* (Lenny Kravitz): overdriven electric;
  - the Arbeau's School tracks: possibly lute or renaissance guitar;
  - jazz trio tracks with a guitar, to be found by listening.
- **Separation:** Demucs' six-stem model (htdemucs_6s) separates *guitar* and *piano* from "other".
  Measure from the stems: T60 per partial (the bass's §5 method), the pluck spectrum, the strum spread, and
  the palm-mute ring.
- **The stems are copyrighted:** they stay in the scratchpad (research/drums.md's rule).

## 10. Electric basses (GM 34–38)

- **What the double bass already covers** (`src/bass.*`, research/double_bass.md):
  - modes and two polarisations;
  - finger plucks (index, middle, jazz pull, thumb);
  - the fingerboard contact up to a full slap at velocity 127;
  - slides, harmonics, sympathy and the body.
- **The electric basses (34 finger, 35 pick, 37/38 slap):**
  - **Strings:** steel roundwound, 864 mm (34") scale, 45–105 gauge [memory]. Their tension of 150–200 N
    each is a little lower than the double bass's 250–300 N.
  - **Frets:** a hard stop.
  - **Body:** solid (PLC14's dead-spot mechanism, a neck conductance peak).
  - **Pickups:** P (split coil, ~1/4 of the scale from the bridge) and J (two single coils), as §4.1.
  - **Slap (37/38):**
    - the thumb strikes the string near the neck's end, which drives it into the frets: a snap;
    - the "pop" is the index pulling a string out so it snaps back against the frets;
    - the bass's slap mechanism with frets as the contact.
  - **Pick (35):** a pick's narrow, fast slip.
- **Fretless (36):** on a real fretless electric, the string vibrating against the wooden fingerboard gives
  the "mwah", a sitar-like buzz along the board.
  - **But in the user's jazz files, GM 36 stands in for the upright:** 25 000 notes in 38 files; the user's
    own piano-and-bass file sets it.
  - Keep 36 on the double bass for jazz files. A real fretless electric would only be for other files
    [design; question for the user].
- **The low B** (5-string, B0 = 23): GM basses reach 28 (E1) normally. A part below 28 folds up an octave
  or uses a B string [design].

## 11. Open questions for the user

1. **Recordings:** is there a guitar recording to fit (any style)? Otherwise, should we run Demucs 6-stem on
   *Fields of Gold* or other wavefield tracks?
2. **GM 29 "muted":** is it always palm-muted, or do the files (desire, louisiana) want an ordinary clean
   electric?
3. **Bass lines on guitar programs** (louisiana ch6: GM 29 below the range): route to the electric bass, or
   fold up an octave?
4. **Nylon voice:** classical (Ramírez-like, flesh-and-nail), or a bossa guitar (flesh only, softer)?
5. **Strum feel:** how pronounced? A fixed spread, or one that follows the velocity and the song's tempo?
6. **GM 36 fretless:** confirm it stays on the double bass for jazz files.

### 11.1 The user's answers (2026-11-16)

1. Recordings: find guitars in the user's own files (separation with Demucs' 6-stem model; §11.2).
2. GM 29 is **always palm-muted**.
3. Bass lines on guitar programs **go to a bass** (the electric bass, once built; until then the
   double bass).
4. Nylon: **classical** (flesh and nail).
5. Strum feel: the user does not know yet; to be tried by ear (start: a spread that follows velocity
   and tempo, small).
6. Fretless (36) **stays on the double bass**: the double bass is fretless.

### 11.2 Guitars in the user's recordings (2026-11-16)

Demucs `htdemucs_6s` (its guitar stem, `--two-stems guitar`) on 18 likely tracks of `~/Music/wavefield`.
The stems are copyrighted recordings: they stay in the scratchpad, never in the repo (only numbers here).
"Alone": 2 s windows (hop 1 s) where the guitar stem holds over 70 % of the energy (and the track is not
silent), the material to fit against.

| Track | Guitar share | Alone | Where (s) | Likely |
|---|---|---|---|---|
| Putumayo Bossa Nova, track 05 | 47 % | 31 % | 0-24 (the intro) | nylon, bossa |
| Putumayo Café Latino, *Cogelados en el tiempo* | 34 % | 28 % | 72-90, 144 | nylon / Spanish |
| *Fields of Gold* | 36 % | 24 % | 120-148 | acoustic (nylon or steel: to tell from the decay and B) |
| Putumayo Bossa Nova, track 06 | 27 % | 14 % | 0-12 | nylon, bossa |
| Putumayo Cuba, *A Buena Vista* | 9 % | 5 % | 0-12 | Cuban guitar / tres |
| Putumayo French Café, track 10 | 38 % | 5 % | 0, 8, 32, 48 | accompaniment |
| *Are You Gonna Go My Way* | 14 % | 4 % | 8, 128-140 | distorted electric (flatness 0.34) |
| St Germain, *Sure Thing* | 9 % | 3 % | 0-12 | clean electric |
| Brazilian Lounge *Há dias*, R&B track 02, Tango *Escualo*, Paul Simon *She Moves On* / *Obvious Child* | 10-40 % | ~0 | - | guitar in the mix only |

- **Nylon (GM 25):** the Bossa Nova intros and Café Latino give unaccompanied nylon guitar; the user's voice
  is classical (flesh and nail), the bossa is flesh only: use them for the strings, the body and the decays,
  not the pluck's brightness.
- **Steel / clean electric:** *Fields of Gold* (to be classified), *Sure Thing*.
- **Distortion:** *Are You Gonna Go My Way*.
- **Not found:** a palm-muted electric, a jazz archtop. Their fits stay with the literature (§3, §5).
- Next: per-string measurements in those passages (partials and B, the two-stage decays per partial,
  the body's peaks under the open strings, the attack's spectrum) - the step-0 targets of §10.

## 12. Sources

- **[PLC14]** A. Paté, J.-L. Le Carrou, B. Fabre, "Predicting the decay time of solid body electric guitar
  tones", JASA 135(5), 3045–3055, 2014.
  - The G3 string data; damping dominated by air friction; δ_VE/TE 1e-3; Q_disl 5500.
  - Neck conductance up to 0.1 m/(s N) against 1.8e-3 at the bridge; the pickup adds no damping and
    favours the out-of-plane polarisation; 95 % of the energy under 2.5 kHz.
  - https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A8_Pate_PredictingDecayTime.pdf
- **[BRWD24]** S. Bilbao, R. Russo, C. Webb, M. Ducceschi, "Real-Time Guitar Synthesis", DAFx24.
  - String, fretboard, frets and finger collisions via IEQ/SAV.
  - 0.11–0.18 s per 44 100 steps per string (M2 Pro, M1, Xeon E5); fret positions L(1 − 2^(−q/12)).
  - https://dafx.de/paper-archive/2024/papers/DAFx24_paper_50.pdf
- **[BT14]** S. Bilbao, A. Torin, "Numerical Simulation of String/Barrier Collisions: The Fretboard", DAFx14.
  https://dafx.de/paper-archive/details/VW3ckMHQUdwpOen-KsadwA
- **[YS06]** D. T. Yeh, J. O. Smith, "Discretization of the '59 Fender Bassman Tone Stack", DAFx06.
  https://dafx.de/paper-archive/details/bbvGpyU59H2nHvV3Iamezg ; https://ccrma.stanford.edu/~dtyeh/tonestack
- **[DAGA21]** a DAGA 2021 paper on guitar body modes (A0 ~100 Hz; one or two plate modes below 400 Hz).
  https://pub.dega-akustik.de/DAGA_2021/data/articles/000093.pdf
- **[RJJP]** B. E. Richardson, H. R. Johnson, A. D. Joslin, I. A. Perry, "Mode placement in guitars
  revisited". https://www.ioa.org.uk/system/files/proceedings/be_richardson_hr_johnson_ad_joslin_ia_perry_mode_placement_in_guitars_revisited.pdf
- **[R]** B. E. Richardson, M. Brooke, "Modes of vibration and radiation fields of guitars"; B. E.
  Richardson, C. A. Taylor, "Resonance studies of the guitar" (IoA).
  https://www.ioa.org.uk/system/files/publications/BE%20RICHARDSON%20M%20BROOKE%20MODES%20OF%20VIBRATION%20AND%20RADIATION%20FIELDS%20OF%20GUITARS.pdf
- **[POMA]** a POMA paper on the three T(1,1) modes; the measured 96, 196 and 294 Hz.
  https://es.scribd.com/document/391667030/POMA-paper-pdf
- **[SGAS21]** Salvi, Gonzalez, Antonacci, Sarti, "Modal analysis of free archtop guitar top plates", JASA
  150(2):1505, 2021. https://www.pubmed.gov/34470294/
- **[C11]** Christian, "Quantifying multiple types of damping acting on bronze-wound guitar strings"
  (Purdue; Savart Journal). https://docs.lib.purdue.edu/dissertations/AAI1502134
- **[LEI]** a University of Leicester student paper on body-driven string decay (high E ~1.1 s, low E
  ~7.8 s). https://journals.le.ac.uk/index.php/pst/article/download/3678/3191/10415
- **[CLF12]** D. Chadefaux, J.-L. Le Carrou, B. Fabre, "A model of harp plucking" (the sticking and slipping
  phases). https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A6_Chadefaux_HarpPluckingModeling.pdf
- **[PC19]** A. Perez-Carrillo, "Finger-string interaction analysis in guitar playing with optical motion
  capture", Frontiers in Computer Science, 2019.
  https://www.frontiersin.org/journals/computer-science/articles/10.3389/fcomp.2019.00008/pdf
- **[MWS16]** H. Mansour, J. Woodhouse, G. P. Scavone, "Enhanced wave-based modelling of musical strings.
  Part 1: Plucked strings", Acta Acustica 102(6), 2016; it cites J. Woodhouse, "On the synthesis of guitar
  plucks" and "Plucked guitar transients: comparison of measurements and synthesis", Acta Acustica 90,
  928–944 and 945–965, 2004. https://www.repository.cam.ac.uk/handle/1810/261228
- **[KVT98]** M. Karjalainen, V. Välimäki, T. Tolonen, "Plucked-string models: from the Karplus-Strong
  algorithm to digital waveguides and beyond", CMJ 22(3), 1998 [memory].
  **[LEVK01]** M. Laurson, C. Erkut, V. Välimäki, M. Kuuskankare, "Methods for modeling realistic playing in
  acoustic guitar synthesis", CMJ 25(3), 2001.
  https://vbn.aau.dk/en/publications/methods-for-modeling-realistic-playing-in-plucked-string-synthesi
- **[Smith]** J. O. Smith, *Physical Audio Signal Processing*, "Commuted Synthesis".
  https://www.dsprelated.com/freebooks/pasp/Commuted_Synthesis.html
- **Pickups:**
  - https://ozvalveamps.org/pickups.html
  - https://courses.physics.illinois.edu/phys406/sp2017/406emi_guitar_pickup_results.html
  - https://wikipedia.com/wiki/Talk:Pickup_(music_technology)
  - [memory] N. G. Horton, T. R. Moore, "Modeling the magnetic pickup of an electric guitar", Am. J. Phys.
    77, 2009.
  - [memory] R. C. D. Paiva, J. Pakarinen, V. Välimäki, "Acoustics and modeling of pickups", JAES 60(10),
    2012.
- **Amps:** [memory] J. Pakarinen, D. T. Yeh, "A review of digital techniques for modeling vacuum-tube
  guitar amplifiers", CMJ 33(2), 2009.
- **Fingering:**
  - D. Sayegh, "Fingering for string instruments with the optimum path paradigm", CMJ 13(3), 1989
    [memory].
  - Burlet & Fujinaga (A*), and Viterbi tablature work, as summarised in
    https://arxiv.org/html/2408.05024v1
  - https://smcnetwork.org/smc2024/papers/SMC2024_paper_id55.pdf
- **Plucking:** [memory] E. Pavlidou, "A physical model of the string-finger interaction on the classical
  guitar", PhD thesis, Cardiff, 1997.
- **Body model:** [memory] O. Christensen, B. B. Vistisen, "Simple model for low-frequency guitar
  function", JASA 68, 1980. https://typeset.io/pdf/simple-model-for-low-frequency-guitar-function-plccqpv99x.pdf

## 13. As built (2026-11-16): the classical guitar

`src/guitar.h/.cc` (the model), `src/guitarinst.h/.cc` (the instrument and the guitarist);
`-inst nylon-guitar`, GM 25 in `-play`. Not on `src/bass.*` as §0 proposed, but a new, smaller
class with the same parts, so the bass stays as it is.

- **Strings:** §1.1's classical table; 48 modes in each of two directions; amplitude decay
  loss0 + loss1 k^1.2 with loss0 1.1–2.0 /s and loss1 0.10–0.36 [est, to be fitted]; along the top
  a string is 0.15 % sharp of towards it [est].
- **Bridge and body:** the strings' pull on the bridge drives 28 body resonances (100, 200, 236,
  290, 400 Hz, then 23 from 455 Hz to 3.2 kHz), each a mass under the bridge (0.3 kg for the top's
  first, mobility peak about 0.06 m/(s N)) with its own share of the sound and its own mix of the
  two string directions [est]. The bridge's acceleration drives every string's modes.
- **Pluck:** a force at the plucking point (12 cm from the bridge by default) rising over 5 ms
  while the finger damps the string, then falling in two stages: off the flesh (9 mm wide, about
  2 ms) and off the nail (1.5 mm, 0.5 ms).
- **The guitarist:** §7.2 as a greedy choice per note (free string, near the hand, low, open
  preferred, strings in the notes' order); §7.3's pinch: 3–9 ms between strings, the thumb first;
  note-off damps (45 /s), the pedal lets ring.
- **Measured on it:** in tune within 1 cent; a released note 23 dB down after 0.6 s; after an A4
  the open strings carry 34 dB more at 440 Hz than after a B-flat (they ring along); first decay
  about 12 dB/s on the low E, 25–45 dB/s on the trebles (fastest near the top's resonance), then
  slower; 0.7 % of a core in random playing.
- **By the user's ear, the same day:**
  - *Vibrato:* a held note with nothing else happening gets a left-hand vibrato, the string pushed
    up along the fret (pitch rises and returns, never flat), about 14 cents × the string's
    suitability (B and G 1, high E 0.6, D 0.7, A 0.15, low E none) × the place on the neck (full
    from the 7th fret, little under the 5th), 4.5–5.8 Hz. It enters 0.6 s after the note if nothing
    was played since 150 ms after its start; 0.45 s when the recent gaps between strokes are short,
    0.9 s and slower when long. A note that comes alone after a pause is fingered for it (B or G
    string, frets 5–10, never open) and always gets it. In `-play` other parts' note-ons within
    two octaves count as "something played", with certainty within an octave and linearly less to
    two octaves (`SongClock::lastOn`).
  - *Mending the tuning:* under the same calm, notes are pulled up (never down, never open
    strings) by chord: the chord's pitch classes are those held on the guitar plus those other
    parts began within 2 s. A minor chord is a root r with r+3 present and r+4 absent, where r is
    not the major third (r−4 present) or the fifth (r−7 and r−3 present) of a major chord
    sounding. In it: the minor third +13 cents, the minor seventh +17, the major seventh +11,
    the major thirteenth +18 (the user's numbers). In a major chord (r and r+4): the ninth +4. Measured: D minor 7's
    seventh +18 and third +11; the major seventh of a minor chord +10 to +13; C major 9's ninth
    +4; the seventh of D7 +1 and the fifth of C major +3 (left alone).
  - *The same note repeated:* each stroke differs by the velocity change and the spacing (rest
    stroke nearer the bridge on a rise of more than 8; nearer the fingerboard on more flesh on a
    fall; otherwise alternating nail share and a wandering plucking point, scaled by the gap),
    with the depth compensated so even repeats stay within about 3 dB.
  - *Strumming (the user's design):* a detector: the third and later of strings sounded within
    60 ms of each other in one direction, or 5–6 notes at once, or a chord of 3 or more within
    1.5 s of the last strum. A strummed string gets a passing hand, not a planted finger (rise
    1.2 ms, little damping), 17 cm from the bridge, the force along the top in the hand's
    direction; velocity sets the depth only; the hand's speed across the strings is constant
    (1.4 m/s, 10.5 mm string spacing: 7.5 ms a string). Down: nail share 0.85–0.95, the first
    strings 25 % deeper. Up: nail 0.4–0.55, 25 % shallower, less again on the strings reached
    last. Measured: six strings at once take 52 ms from the first string to the loudest moment at
    velocities 45, 80 and 115 alike, at −13, −8 and −4 dB; up strokes 3–8 dB under down strokes; a
    strum's spectral centre about 300–350 Hz against a three-note pinch's 233 Hz.
  - *Picking's colour by velocity (the user):* flesh alone when soft (nail share 0 up to velocity
    40), rising smoothly to 0.8 (0.5 for the thumb) by velocity 105; above 107 the nail's knock
    (a 0.6 ms force pulse against the pull, through the nail's narrow contact). Measured on E4,
    share of the first 100 ms between 0.8 and 2 kHz: −42 dB at velocity 30, −21 at 70, −18 at 127;
    between 2 and 5 kHz: −59, −43, −36.
  - *Strums are soft (the user):* broad flesh-and-nail contact (11 mm and 3 mm going down, 16 and
    6 mm coming up), slower release, nail share 0.4–0.5 down and 0.15–0.25 up whatever the
    velocity, and never the knock: a nail's rounded back flexes where its hollow side and edge
    are stiff. Measured, a strummed chord at velocity 60 and at 127: 0.8–2 kHz share −33 dB both,
    2–5 kHz −61 dB both, only the level differs (−16 and −9 dB).
  - *Strum direction (the user, with seven common patterns as D / U / rest charts):* the hand
    is a pendulum: its swing is the shortest gap between recent strums (learned from gaps of one
    swing), a strum n swings after the last goes the other way for odd n and the same way for
    even n; after 1.6 s without one: down; 5 % of strums go against it. With the song clock, a
    strum within 0.12 beat of a beat is down with probability 0.85, of the bar's first beat 0.97.
    With the clock, the last bar's pattern (its strums' ways by sixteenth; "clear" if it had three
    or more) pulls: where this bar's reading differs from it at the same place, the last bar's
    way is taken with probability 0.3. Checked on eight bars of one pattern with 30 % of strums
    made to go against the swing: 28 of 42 strums agree with the bar before without the pull, 34
    with it, 42 at 0.8; as set (5 % against the swing) all 42.
    Checked without the clock: eighths give D U D U D U D U; "D - D U - - D -" and
    "D - D U - U D U" come out exactly.
  - *One stroke, one depth (the user):* a strum's depth comes from the chord's mean velocity,
    compressed (0.3 + 0.5 × the mean, in velocity's 0–1), the same for every string but for a
    little more on the bass strings going down (up to 15 %), a little more on the middle strings
    coming up (12 %) and less on the highest (× 0.82). Measured: all notes at velocity 20, 50, 80,
    110, 127 give −19.6, −17.3, −15.3, −13.6, −12.7 dB (7 dB in all); six unequal velocities
    sound exactly as their mean.
  - *What a stroke takes from a ringing string (the user):* picking stops it (the finger plants);
    a down strum hardly touches it; an up strum's pad eats some. Measured, a nearly silent strum
    over a loud ringing chord: down −0.4 dB, up −4.8 dB.
  - *The nail's two sides (the user):* picking, the string lies in the nail's hollow side, held
    sharply at two places about 3 mm apart; strumming, it meets the rounded back. Modelled as two
    narrow contacts 3 mm apart for the picked nail; within the 48 modes this changes the highest
    partials by under 1 dB.
  - *Ring:* the strings' losses × 0.8, the 2nd and 3rd partials' × 0.55 again, the 4th and 5th's
    × 0.78 (asked for next: "slightly longer, but not as much"): those partials now
    lose about half as many dB per second (E5: 32 → 17 and 34 → 15).
- **Not built:** the fit to the separated recordings of §11.2; fret buzz; tension modulation;
  slides, hammer-ons, bends, harmonics; the whole-file fingering (Viterbi); live controls.
