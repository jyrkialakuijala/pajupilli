# Electric guitars (GM 27–32) and a separate amp and effects module: research

Status: research 2026-11-19. Built: GM 28 (clean-guitar: the three-pickup solid-body, the pick, the pickups,
and the amp module's clean Twin), GM 29 (muted-guitar: the palm of section 8, the compressor) and
GM 30 (overdriven-guitar: a twin-humbucker solid-body's humbucker, a compressor, the British crunch stack at crunch, 4x
oversampled); README "Electric guitars (clean, muted, overdriven)". The rest is not built. GM
programs 27–32 counting from 1 (26–31 from 0): 27 Electric Guitar (jazz), 28 (clean), 29 (muted), 30
Overdriven Guitar, 31 Distortion Guitar, 32 Guitar Harmonics. Before this, `src/midifile.cc`'s kGm
table sent "jazz-guitar" and "clean-guitar" to the koto (clean-guitar now plays itself),
"muted-guitar" to the shamisen (then the koto), "overdriven-guitar" and "distortion-guitar" to
`lead-charang` (then `lead-saw`), and "guitar-harmonics" to the kalimba. The nylon guitar (GM 25) is
built (`src/guitar.cc`, `research/guitars.md` §13). Written beside `research/bass_section.md`
(electric basses, their pickups) and `research/steel_guitar.md` (the steel-string acoustic, pick,
palm mute, squeak): where this document needs what those cover, it points there and does not repeat
it.

How each number is marked:
- **[T]** read in the primary text (downloaded and read for this document). "Measured" is said in
  words where the text reports a measurement and not a calculation.
- **[S]** from a secondary source, an abstract, a search snippet or a machine summary of a page:
  the primary text was not read.
- **[L]** player lore: a players' or makers' description, not a measurement.
- **[E]** my estimate, fit or arithmetic from cited values; not a published value.
- **[M]** from general knowledge, not looked up: check before relying on it.

References are short keys in brackets; the list with URLs is section 19. What could not be found is
in section 17.

**Short answer.** The six programs are one instrument in three bodies, played three ways, into one
amp at three gains. A solid-body guitar with 10–46 roundwound steel strings on a 648 mm scale
(tensions 69–85 N, B from 1.3e-5 on the high E to 1.1e-4 on the plain G **[E]**), heard only through
magnetic pickups: a single coil of 2.2 H or a humbucker of 4–8 H **[T, Lemme]**, resonating with
the cable at 2–5 kHz (a 2.2 H three-pickup solid-body pickup with 470 pF and 200 kΩ peaks at 4.5 kHz **[T]**), placed
38–49 mm (bridge), ~100 mm (middle) and ~158 mm (neck) from the saddle **[T, measured, MDH17]**, so
that the bridge pickup's first comb notch sits at the 13th–16th harmonic and the neck's at the 4th.
GM 27 swaps the body for a hollow archtop with flatwounds, the neck humbucker and the tone rolled
off; GM 29 adds the palm on the strings at the saddle; GM 32 adds a finger at a node; GM 30 and 31
are GM 28's guitar with a humbucker through more gain. Everything after the jack is a separate
module that any instrument can use: triode stages (a Koren 12AX7 **[T]**), a classic three-knob/Vox
tone stack with exact closed-form coefficients **[T, YS06]**, a push-pull power stage with sag (a
linear output transformer is indistinguishable from a nonlinear one in an ABX test **[T, Mačák]**),
and a 12" speaker cabinet that passes 75 Hz–5 kHz **[S, Celestion]** (measured cabinets reach 7–8
kHz **[T, Mačák]**); pedals are a mid-hump overdrive (1N914 pair in an op-amp's feedback, 720 Hz
pre-emphasis, 21–41 dB of gain **[T values, E arithmetic]**), a DS-1 or RAT (diodes to ground), a
fuzz, compression, BBD chorus/flanger, phaser, tremolo, a dispersive spring reverb and a delay. The
nonlinear stages want 4–8× oversampling (4× satisfies most listeners, some hear aliasing until 14×
at 60 dB of gain **[T, Mačák]**). The amp's static-waveshaper-plus-filters form is what the the tonewheel organ's
and the rotary speaker's drive already are; the tone stack is `tools/clavtone.py`'s exact-SOS approach; the
pickups are the clavi's. Build the guitar first (GM 28 and 29 on a clean amp), then the amp
module (30, 31), then the archtop (27) and the harmonics (32).

---

## 1. What the project has, and what is reusable

| What | Where | Reuse for |
|---|---|---|
| Six strings, 48 modes in two polarisations each, a shared bridge on a body of resonances, frets as a length change, bends, damping; flesh and nail plucks; a strumming guitarist with a pendulum hand; note-off damping (45 /s); 0.7 % of a core | `src/guitar.h/.cc`, `src/guitarinst.*`, `research/guitars.md` §13 | the string set, the fret, the bend, the guitarist (fingering solver, strum detector, pendulum); a new `GuitarSpec` per electric |
| Harmonics by touch (`touch(s, n, secs, rate)`), slides and hammer-ons (`setLength`), fingerboard contact (buzz, slap), tension modulation, sympathy | `src/bass.h/.cc` | GM 32's touch; slides; fret buzz; the pitch drop of a hard pick |
| Pickups: EMF = (1 + k·y) · v at a point (the magnet's first-order nonlinearity), two pickups, the selector in and out of phase | `src/clavi.cc` (line ~417) | the guitar's pickups, with an aperture and the coil's L–C–R added |
| Exact analog filters from circuit values, by symbolic transfer function → bilinear (prewarped) → SOS | `tools/clavtone.py` → `src/clavtone_data.h` | the guitar's tone and volume pots, the tone stacks, the TS tone stage (all closed form) |
| Component-level netlists against ngspice | `tools/circuit.py` | checking the triode stage, the mid-hump overdrive clipper, the RAT against SPICE |
| A static waveshaper with sag: `tanh(x + 0.12)` asymmetric, the drive eased by a 0.1 s sag follower, the output compressed as drive^0.7 | `src/rotary.cc` ~line 235 (the rotary speaker's amp) | a first power-amp model; the sag mechanism |
| A triode's even harmonic as `v + k v²`, then `tanh(1.5 v)/1.5`, DC blocked at 8 Hz | `src/tonewheel.cc` ~line 891 | a first preamp stage |
| An overdrive: preamp gain into an asymmetric diode pair, then a mid notch | `src/lead.cc` ~line 1139 (85 Charang) | the pedal family's shape |
| The rotary speaker as a separate cabinet class | `src/rotary.*` | the pattern for an independent output-stage module |

- The decisive thing missing for electrics is not the string: it is (1) a pick, (2) the pickup's
  aperture, coil and load, (3) the palm, and (4) everything after the jack. Items 1 and 3 are shared
  with `research/steel_guitar.md` §4.1 and §4.4; item 2 with `research/bass_section.md` §3.

---

## 2. The six programs one by one

### 2.1 GM 27, Electric Guitar (jazz)

- **The instrument it stands for:** a hollow-body archtop with a humbucker at the neck: the humbucker guitar
  laminated archtop (16 in. laminated maple body, 24.75 in./628 mm scale; a P-90 from 1949, humbuckers from
  1957) or the carved-top L-5 CES / Super 400 CES **[M]**. Others: one maker Broadway, one drum maker,
  one maker George Benson models **[M]**.
- **Set-up:** flatwound strings, 12–52 or 13–56 **[L]**, much heavier than rock gauges; the neck
  pickup alone; the tone knob turned down (to about a third) **[L]**; a heavy pick (1.5 mm and up)
  or the thumb (Wes Montgomery) **[L]**.
- **Amp:** clean, little or no reverb, bass up and treble down: an American amp (clean 2x12 combo, Deluxe), a
  Polytone Mini-Brute, a one maker JC-120 **[L]**. No pedals.
- **Sound:** round, dark, the attack soft and the note short (flats and the archtop's top take the
  highs and the sustain); some acoustic top in the sound **[L]**.
- **How played in GM files:** comping chords, 3–7 notes, short (median 0.33 beat in the user's one
  file: louisiana ch7, 1705 notes, range 39–85 **[T, files, guitars.md §7.1]**); and single-line
  solos with octaves (Wes) and chord melody **[L]**. Freddie Green-style quarter-note chords in big
  band charts **[L]**.
- **Sound sets:**
  - early GS module: "Jazz Gt."; variation 27/8 "Hawaiian Gt" **[T, SC-88Pro manual's map table]**.
  - later GS module / SC-88Pro: "Jazz Gt.", 27/1 "Mellow Gt.", 27/8 "Pedal Steel" **[T]**.
  - XG (DB50XG): "Jazz Gtr", bank 18 "MelloGtr", bank 32 "JazzAmp" **[T, DB50XG manual]**.
  - FluidR3: "Jazz Guitar": one recorded set of 12 samples (E3 … G6, 0.84–2.63 s, 22.05 kHz),
    looped, panned twice (±50 %) as a stereo pair, chorus send 14 %; velocity layers (9 of them) all
    use the same samples **[T, read from the installed FluidR3_GM.sf2]**.

### 2.2 GM 28, Electric Guitar (clean)

- **The instrument:** a solid body with single coils, an American amp three-pickup solid-body (3 single coils, 648 mm)
  or two-pickup solid-body (2, the bridge one in a steel plate) **[M]**. The SC-88Pro's variations name them:
  "TC FrontPick", "TC Rear Pick", "TC Clean ff" (two-pickup solid-body), "JC Clean Gt." (the one maker JC-120)
  **[T]** (the abbreviations' meaning is **[E]**).
- **Set-up:** roundwound nickel-plated steel 9–42 or 10–46 **[M]**; the three-pickup solid-body's in-between
  positions (neck+middle, middle+bridge, in phase: the "quack") **[L]**; a medium pick.
- **Amp:** an American amp blackface clean (clean 2x12 combo: 6L6 push-pull, spring reverb) or the solid-state
  JC-120 with its stereo chorus **[L]**; often a compressor, chorus, spring reverb, a short delay.
- **How played:** strummed chords, arpeggios, funk 16ths, double stops; pop and soul rhythm parts
  **[L]**. Not used in the user's files **[T, files, guitars.md §7]**.
- **Sound sets:**
  - early GS module "Clean Gt.", 28/8 "Chorus Gt." **[T]**.
  - SC-88Pro adds "Clean Half", "Open Hard 1/2", "JC Clean Gt.", "JC Chorus Gt", "TC FrontPick",
    "TC Rear Pick", "TC Clean ff", "TC Clean 2" **[T]**.
  - XG "CleanGtr", bank 32 "ChorusGt" **[T]**.
  - FluidR3 "Clean Guitar": 10 samples (0.99–3.96 s), a 7.9 kHz low-pass (filterFc 7935 cents) on
    one of two layers, chorus send 25 % **[T, sf2]**.

### 2.3 GM 29, Electric Guitar (muted)

- **The instrument:** the clean guitar of 2.2 with the picking hand's palm on the strings at the
  saddle (palm muting), or with the fretting hand lightly lifted (the "scratch") **[L]**. FluidR3
  calls the preset "Palm Muted Guitar" **[T, sf2]**.
- **How played:** two different uses:
  1. **Funk and disco:** 16th-note single notes and chords, staccato, with dead strokes
     ("chicken scratch") **[L]**.
  2. **Rock and metal:** palm-muted chugs on the low strings and power chords, usually on a
     distorted amp (which in GM is often program 31 with short notes, or 29 under heavy reverb)
     **[L]**.
  - In the user's files it is comping: desire ch4, 3362 notes, 3–6-note chords, median 0.25 beat;
    louisiana ch5, 1705 notes up to MIDI 97 (above the guitar); louisiana ch6, a bass line 29–54
    **[T, files, guitars.md §7.1]**. These are the "muted" sound used as a short-chord comping
    guitar, not palm-muted rock.
- **Sound sets:**
  - early GS module "Muted Gt.", 29/8 "Funk Gt.", 29/16 "Funk Gt.2" **[T]**.
  - SC-88Pro adds "Muted Dis.Gt" (a muted distorted), "TC Muted Gt.", "Funk Pop" **[T]**.
  - XG "Mute.Gtr", 40 "FunkGtr1", 41 "MuteStlG", 43 "FunkGtr2", 45 "Jazz Man" **[T]**.
  - FluidR3 "Palm Muted Guitar": 10 samples of 0.27–0.90 s at 44.1 kHz; the 9 velocity layers
    differ only in release time (releaseVol from +316 to −1586 timecents: louder = longer release)
    **[T, sf2]**.

### 2.4 GM 30, Overdriven Guitar

- **The instrument:** a guitar into an amp at the edge of break-up or a little past it: crunch.
  Classic pairs: a humbucker guitar twin-humbucker solid-body's bridge humbucker into a British stack plexi/JCM800, or a
  three-pickup solid-body into an American amp with a mid-hump overdrive **[L]**. The SC-88Pro names "LP OverDrvGt" (Les
  Paul) **[T]**.
- **Sound:** the note still clear, chords still readable; dynamic (picking softer cleans it up)
  **[L]**; mostly 2nd and 3rd harmonic distortion with compression.
- **How played:** blues and rock leads with bends and vibrato (pitch bend ±2 semitones by default
  in GM; CC1 for vibrato), rhythm with dyads and open chords **[L]**. Not in the user's files.
- **Sound sets:**
  - early GS module "OverdriveGt" **[T]**; SC-88Pro adds "Overdrive 2/3", "More Drive", "LP OverDrvGt" **[T]**.
  - XG "Ovrdrive", bank 43 "Gt.Pinch" (a pinch harmonic, §9.3) **[T]**.
  - FluidR3 "Overdrive Guitar": 10 samples named "Distortion E3 … G6", 2.5–13 s long (so the
    sustain is recorded, not looped), an 8.3 kHz low-pass on one layer **[T, sf2]**.
  - The SC-88Pro's insertion effects include Overdrive and Distortion with an "amp simulator" of
    four types: Small, BltIn (single-unit combo), 2-Stk, 3-Stk **[T, SC-88Pro manual]**.

### 2.5 GM 31, Distortion Guitar

- **The instrument:** high gain: a humbucker (often a hot one; active EMG 81 in metal **[L]**)
  through a distortion pedal or a high-gain preamp (high-gain amp maker Rectifier, high-gain amp, a modded British stack)
  into a closed 4×12 with 60 W British speakers or 25 W British speakers **[L]**.
- **Sound:** compressed, sustaining, the chord reduced to root and fifth (power chords: a major third
  turns muddy under heavy distortion because the intermodulation products are inharmonic to the
  root **[M]**); palm mutes; feedback.
- **How played:** power chords (root, fifth, octave: 2–3 notes), riffs on the low strings, leads with
  bends and wide vibrato, sustained notes held for bars **[L]**. Not in the user's files.
- **Sound sets:**
  - early GS module "Dist.Gt." **[T]**; later GS module/88Pro "DistortionGt", "Dist. Gt2", "Dazed Guitar", "Feedback Gt2",
    "Power Guitar", "Power Gt.2", "5th Dist." (a fifth layered in), "Rock Rhythm", "Rock Rhythm2"
    **[T]** (rows 31/4–31/8 fell on a page break and were not read).
  - XG "Dist.Gtr", 40 "FeedbkGt", 41 "FeedbGt2" **[T]**.
  - FluidR3 "Distortion Guitar": 10 samples at 30 kHz, 1.6–11.2 s, two layers (one through an 8 kHz
    low-pass, one at 11 kHz with Q) **[T, sf2]**.

### 2.6 GM 32, Guitar Harmonics

- **What:** natural harmonics: a left-hand finger lightly on the string at a node (12th fret: the
  octave; 7th: the 12th; 5th: two octaves; ~4th: two octaves and a third) while picking; the finger
  lifts and the bell-like partial rings **[M]**. Artificial (tapped or touched 12 frets above the
  fretted note) and pinch harmonics extend it to any pitch (§9).
- **How played in GM files:** single bell-like notes and arpeggios, intros and fills; the pitch
  written is the pitch heard **[L]**. Not in the user's files.
- **Sound sets:**
  - early GS module "Gt.Harmonix", 32/8 "Gt.Feedback" **[T]**; SC-88Pro "Gt.Harmonics", "Gt.Feedback2",
    "Ac.Gt.Harmnx", "E.Bass Harm." **[T]**.
  - XG "GtrHarmo", 65 "GtFeedbk", 66 "GtrHrmo2" **[T]**.
  - FluidR3 "Guitar Harmonics": 11 short samples (0.23–1.45 s), the top sample (0.23 s) stretched
    over MIDI 60–85; velocity layers differ only in release **[T, sf2]**.
- **Note:** both one maker and one maker put guitar *feedback* under 31 and 32's variations: GM 32's bank
  is the "special guitar sounds" slot **[T, the lists; E the reading]**.

### 2.7 What the classic sets have in common

- Every set is one recorded guitar per program, a sample every 3–5 semitones, no velocity timbre
  (FluidR3's velocity layers change only the release or, for 31, a filter) **[T, sf2]**. One maker's
  variations are where the real distinctions are: two-pickup solid-body pickups, a JC-120, a twin-humbucker solid-body, a muted
  distortion, feedback **[T]**. A physical model gets these as switches (§13).
- Chorus is part of the clean presets (FluidR3: 14–25 % chorus send on 27 and 28; one maker and one maker
  both have a "Chorus Gt." variation) **[T]**.

---

## 3. The commonalities: one electric guitar with variations

| | 27 jazz | 28 clean | 29 muted | 30 overdriven | 31 distortion | 32 harmonics |
|---|---|---|---|---|---|---|
| Body | hollow archtop (some acoustic top) | solid | solid | solid | solid | solid |
| Scale | 628 mm | 648 | 648 | 628 (LP) or 648 | 628 or 648 | 648 |
| Strings | flatwound 12–52 | round 10–46 | round 10–46 | round 10–46 | round 10–46 (or 10–52 for drop tunings) | round 10–46 |
| Pickup | neck humbucker, tone ~0.3 | three-pickup solid-body neck+middle (pos. 4) or Tele bridge | bridge or middle single | bridge humbucker | bridge humbucker, hot | neck single (or middle) |
| Pick | heavy, near the neck pickup | medium, 100–140 mm | medium, near the bridge, palm on | medium, near the bridge | heavy, near the bridge | medium, at a node of the harmonic |
| Left hand | chord grips, short | strums, arpeggios | short | bends, vibrato | power chords, bends | a light touch at a node |
| Amp | clean, dark | clean, chorus, spring reverb | clean + compressor | crunch (British-stack-like) | high gain + TS boost | clean, reverb |
| Velocity | depth, pick share | depth | depth, mute depth | depth → amount of drive | depth (compressed away) | depth |

All of [L]/[E] except the scales [M]. The guitar is one class with a spec (strings, body, pickups,
controls); the playing (pick, palm, touch) is per program; the amp is a separate module (section
11) with a preset per program.

---

## 4. The strings

### 4.1 Gauges and tensions

standard EXL110 (10–46, nickel-plated steel roundwound), at 648 mm **[S, retailer's copy of the
standard chart: lb]**, converted **[E]**, with plain strings checked from the gauge (steel 7850
kg/m³) **[E]**:

| String | Gauge (in.) | Tension (lb) [S] | (N) | Plain-string check (N) [E] | B [E] |
|---|---|---|---|---|---|
| E4 | .010 | 16.2 | 72.1 | 72.6 | 1.3e-5 |
| B3 | .013 | 15.4 | 68.5 | 68.9 | 4.0e-5 |
| G3 | .017 | 16.6 | 73.8 | 74.2 | 1.1e-4 |
| D3 | .026w | 18.4 | 81.8 | — | ~2.4e-5 (core ~.012 assumed **[M]**) |
| A2 | .036w | 19.0 | 84.5 | — | ~5.8e-5 (core ~.015 **[M]**) |
| E2 | .046w | 16.9 | 75.2 | — | ~1.3e-4 (core ~.018 **[M]**) |

- B = π³ E d⁴ / (64 T L²), E = 200 GPa, d the plain wire or the wound string's core **[E]**. The plain
  G is the most inharmonic string of the set, more than the low E: it is the string players find
  hardest to intonate and why jazz sets use a wound G **[L]**.
- Total ≈ 456 N on the neck **[E]**; the classical's is ~400 N (guitar.cc's table).
- **On a 628 mm twin-humbucker solid-body scale** the same set is 6 % slacker (×(628/648)² = 0.94) and B 13 % higher
  **[E]**.
- **Flatwound 11–50 / 12–52 (jazz):** no tension chart was opened (section 17). Plain .011 at 628
  mm: 82.5 N, B 1.8e-5; .015: 86 N, B 6.0e-5 **[E]**. Flatwound wound strings are denser for their
  diameter (the flat ribbon fills the gaps) **[M]** and usually heavier gauges, so their tensions
  run 80–110 N **[E]**.

### 4.2 Losses and decay

- **Solid body:** the bridge is nearly rigid (conductance below 1.8e-3 m/(s N)), the string's own
  losses dominate (air friction most, then internal), and the neck's out-of-plane conductance (up
  to 0.1 m/(s N)) shortens particular notes: the dead spots **[from guitars.md §3.3, PLC14]**. The
  pickup adds no damping **[PLC14 via guitars.md]**; a pickup set very close does add magnetic drag
  and shortens sustain **[S, ozvalveamps]**.
- **Numbers to use until fitted:** a solid-body open low E rings 10–20 s to inaudibility, a high E
  3–6 s **[M]**; partials decay faster in proportion roughly to k^1–1.5 **[M]**. The structure in
  `GuitarString` (loss0 + loss1·k^1.2) is right; the values must be fitted (the bass's §5 method,
  `research/double_bass.md`).
- **Neck modes:** two or three neck resonances at 150–400 Hz under the fret end, as a small
  admittance in series with the fret stop **[design, guitars.md §3.3]**; a note whose fundamental
  sits on one rings 2–3 times shorter **[M, Fleischer]**.
- **Flatwound:** sources disagree whether flats decay faster or slower; all say fewer high
  overtones and a "thump" **[L; no measurement found, section 17]**. Model: loss1 × 2–3 on the wound
  strings and on the plain ones (jazz sets are older and heavier) **[E]**.
- **Archtop:** the floating bridge on a light top drains energy: notes are shorter than on a solid
  body, 2–4 s **[guitars.md §9.3, design]**.

### 4.3 Shared with the electric bass

- The electric bass (`research/bass_section.md` §2) is the same string class with heavier gauges
  and a 864 mm scale; the losses, polarisations, frets and pickup model are the same code with
  another table. The guitar's `GuitarSpec` and the bass's spec should be one type **[design]**.

---

## 5. The body and neck

- **Solid (three-pickup solid-body alder or ash, twin-humbucker solid-body mahogany with a maple cap) **[M]**:** the body's sound is the
  string's decay and the dead spots, not radiation. The direct acoustic sound of an unplugged solid
  body is 20–30 dB under the amp's and is ignored **[E]**.
- **Semi-hollow (ES-335: thin hollow wings on a solid maple centre block) **[M]**:** like a solid body
  for the strings, a little more top resonance in the sound, less feedback than a hollow body
  **[L]**.
- **Hollow archtop (GM 27):** a carved or laminated arched top on a floating bridge. No assembled
  archtop mode table was found; free top plates have been measured [SGAS21, via guitars.md]. Use A0
  ~110–130 Hz and the top's main mode ~200–280 Hz **[guitars.md §3.2, M]** as `GuitarSpec::body`, and
  mix the acoustic share in at −15 to −20 dB under the pickup **[E]**.
  - The bridge is not glued: the strings' down-force holds it, it rocks on two feet. Its
    admittance is higher than a flat-top's; that is why the archtop's note is punchy and short
    **[L]**.
- **Acoustic feedback (archtop; and "Feedback Gt." on 31/32):** the amp's sound drives the top or the
  strings, the pickup hears it, the loop gain passes 1 at a resonance. On an archtop it howls at the
  top's lowest modes (100–300 Hz), which is why jazz players keep the amp low and stand away
  **[L]**. On a distorted solid body it is controlled feedback: a string partial (often an octave or
  a fifth above the note) blooms after a second or two and sustains **[L]**. Model **[E]**: an
  acoustic path from the cabinet's output, delayed by distance (3 ms per metre), into the strings'
  modes through the bridge's acceleration (the archtop: into the body modes), with a gain set by
  the player's distance; the amp's compression sets the level the loop settles at. This is the only
  place where the amp module talks back to the guitar.

---

## 6. Pickups and the guitar's electronics

### 6.1 What a pickup hears

- **Velocity at a place, through a nonlinearity:** the flux through the coil follows the string's
  displacement through a static nonlinear function, and the voltage is its time derivative (a
  Hammerstein model). Measured on three Seymour Duncans (SSL-5 single coil, SH-2N humbucker,
  STHR-1B Hot Rails) with a shaker-driven string piece **[T, measured, NLSB18]**:
  - the static function does not depend on frequency (60–400 Hz) nor on the rest distance; it fits
    NLfnc(x) = A · [ (x + Leq) / ∛(req² + (x + Leq)²) − x / ∛(req² + x²) ], three parameters per
    pickup, req and Leq credible as an equivalent magnet's radius and length **[T]**;
  - the humbuckers give more level and more distortion than the single coil, much more at 3 mm from
    the string than at 5 mm (2 mm swing at 80 Hz) **[T]**.
  - The clavi's (1 + kNl·y/1 mm)·v is this function's first-order expansion; for a guitar's
    larger swings (2 mm at a hard pick on the low E against 2–4 mm of clearance) the full form
    matters **[E]**.
- **The polarisation:** the pickup hears the swing towards the pole far more than the one along it
  **[PLC14 via guitars.md]**; the clavi uses −25 dB for the sideways swing **[clavinet.cc]**.
- **Position (the comb):** measured on a Squier three-pickup solid-body **[T, measured, MDH17]**:

| String | L (mm) | Bridge pickup (mm) | Middle | Neck |
|---|---|---|---|---|
| E4 | 649 | 38 | 99 | 157 |
| B3 | 650 | 41 | 100 | 158 |
| G3 | 652 | 45 | 102 | 160 |
| D3 | 651 | 46 | 101 | 159 |
| A2 | 652 | 49 | 102 | 160 |
| E2 | 650 | 49 | 100 | 158 |

  - The bridge pickup is slanted: 38 mm under the high E, 49 mm under the low E **[T]**.
  - First notch (open string) at harmonic L/x: bridge 13–17, middle 6.5, neck 4.1 **[E]**. Fretting
    moves it down (x/L grows).
  - A two-pickup solid-body's two pickups are about 120 mm apart, the largest spacing among popular electrics
    **[T, MDH17]**. twin-humbucker solid-body: bridge ~40 mm, neck ~155 mm from the saddle **[M]**.
- **Aperture (sensing width):** MDH17 model a three-pickup solid-body single coil as 20 mm wide and a P-90 as 40 mm
  **[T]**; Paiva et al. model the "sensitivity width" as a low-pass in the waveguide **[S, PPV12]**;
  the bass document uses a Gaussian tap of 10–20 mm **[bass_section.md §3.1]**. A humbucker is two
  sensing points ~18 mm apart **[M]**: a further comb, cos(kπ·d/(2L)) as `Guitar::shapeAt` already
  has for the nail's two contacts **[E]**.

### 6.2 The coil

Measured inductances (LCR meter at 120 Hz; ±10 % between samples) **[T, Lemme13]**:

| Pickup | L (H) |
|---|---|
| American amp three-pickup solid-body | 2.2 |
| American amp two-pickup solid-body bridge | 3.0 |
| humbucker guitar 490R (neck humbucker) | 4.2 |
| humbucker guitar 498T, 500T (bridge humbuckers) | 8.0 |
| American amp offset solid-body | 3.0–4.5 (at ~8 kΩ DC) |
| split-coil bass / two-pickup bass | 6.0 / 3.6 |

- Average of 21 pickups (from Lemme): 4.1 H, 12 kΩ, 130 pF self-capacitance; a '70s three-pickup solid-body: 2.2 H,
  6.5 kΩ, 110 pF **[S, ozvalveamps citing Lemme]**.
- Ranges from makers: three-pickup solid-body 1.8–2.5 H, PAF 2.8–4.6 H, P-90 4.9–6.8 H **[S, planetz, Mojotone]**.
  humbucker guitar wound a fixed 5000 turns per coil on PAFs from 1961; magnet strength still varied
  **[T, Lemme13]**.
- **DC resistance says little** (the same type varies in L at the same R); inductance and the loaded
  response are what matter **[T, Lemme13]**.
- **Output level:** the strongest pickups give over ten times the voltage of the weakest (a Schaller
  "Hot Stuff" against a three-pickup solid-body); the 500T gives more than twice the 498T's voltage with the same
  frequency response **[T, Lemme13]**. This is what drives the amp into distortion (GM 30, 31).
- **Covers and eddy currents:** a brass cover damps the resonance strongly, nickel silver less; an
  open-topped cover restores it **[T, Lemme13]**. Pickups with steel pole pieces have a lower
  resonance peak than ones with magnets in the coil **[T]**.
- More turns: lower resonance, more output; thicker wire (42 AWG against 44): higher resonance and
  louder at the same turns **[S, BGMC24 abstract]**.

### 6.3 Loaded: pots, cable, amp input

- **Equivalent circuit:** EMF in series with L and R (the winding), C (winding) across the
  terminals, then the volume pot, tone pot and capacitor, the cable's capacitance, the amp's input
  resistance **[T, ozvalveamps]**.
- **The resonance** (a '70s three-pickup solid-body, 2.2 H, simulated) **[T, ozvalveamps]**:
  - naked: ~10 kHz with a 25 dB peak;
  - with a cable: 100 pF ~8 kHz, 200 pF ~6 kHz, 500 pF ~4 kHz, 1000 pF ~3 kHz;
  - the load sets the peak's height, not its frequency: 1 MΩ +15 dB, 470 k +10 dB, 220 k +4 dB,
    100 k −3 dB (no peak), 47 k −6 dB.
  - Measured: a 1972 three-pickup solid-body bridge pickup (2.2 H) loaded with 470 pF and 200 kΩ (a 250 k volume pot
    ∥ 1 MΩ amp, no tone pot) peaks at about 4.5 kHz, flat below 1 kHz **[T, measured, Lemme13]**.
- **f₀ = 1/(2π√(L·C)) [E] with C = cable + 110–130 pF:**

| Pickup | L | C total | f₀ |
|---|---|---|---|
| three-pickup solid-body single coil | 2.2 H | 580 pF | 4.4 kHz (Lemme measured 4.5) |
| three-pickup solid-body single coil | 2.2 H | 810 pF | 3.8 kHz |
| Tele bridge | 3.0 H | 810 pF | 3.2 kHz |
| PAF-type humbucker | 4.5 H | 830 pF | 2.6 kHz |
| Hot humbucker (498T) | 8.0 H | 830 pF | 1.95 kHz |
| P-90 | 5.5 H | 830 pF | 2.4 kHz |

  - Q ≈ R_load·√(C/L): with 250 k ∥ 250 k ∥ 1 MΩ = 111 kΩ for single coils, 500 k ∥ 500 k ∥ 1 M
    = 200 kΩ for humbuckers: Q 2–2.7 **[E]**, lower again from eddy currents **[M]**.
- **Volume pot:** turning it down puts its resistance in series and the resonance vanishes (the
  "darker when turned down"); a 470 pF treble-bleed cap keeps the peak but moves it up by ~1.4×; a
  resistor across the cap is the usual compromise **[T, Lemme13]**.
- **Tone pot:** the tone capacitor (0.022 µF on humbucker guitars, 0.047 µF on American amps **[M]**) in series with
  the tone pot across the pickup. Fully down, the capacitor and L resonate: 2.2 H with 47 nF at 490
  Hz, 4.5 H with 22 nF at 500 Hz **[E]**: the "woman tone" (Clapton: neck humbucker, tone at 0, a
  cranked British stack) is this resonance, a nasal mid peak, not merely a low-pass **[L; E the
  mechanism]**. The jazz setting (tone ~0.3) is a damped version of it **[E]**.
- **Selector:** three-pickup solid-body positions 2 and 4 put two pickups in parallel; one of the pair is reverse
  wound and reverse magnetised (the middle on later Strats), so they hum-cancel and add in phase
  **[M]**. Out-of-phase wiring (a mod) subtracts: thin, nasal **[L]**. Two pickups in parallel halve
  the inductance (L/2) and double the load: resonance up ~√2 **[E]**.
- **Into the module:** the pickup network must be solved together with the first stage's input
  (section 11.1): a 1 MΩ tube input lets it ring; a fuzz's ~5–10 kΩ input loads it flat and dark;
  a buffered pedal hides the cable **[T for the loads, ozvalveamps; S for the fuzz]**.
- **The bass:** the same model with the bass's coils (`research/bass_section.md` §3.2–3.3: P 6.4 H, J
  3.3 H, 0.047 µF, Q ~1.2) **[there]**. One `Pickup` class: position(s) per string, aperture,
  nonlinearity, L, R, C; one `GuitarControls` network: pots, caps, selector, cable.

---

## 7. The picking hand

- **The pick model** is `research/steel_guitar.md` §4.1 (pick thickness, the 0.5–1 mm contact, a
  0.1–0.3 ms slip, the click, the scrape on wound strings, the pick's bend). It is the same for the
  electric; only the strings are lighter, so the same hand moves the string farther and a hard pick
  reaches the frets **[E]**.
- **Where:** MDH17 picked at 30–170 mm from the bridge with a 0.88 mm pick in their recordings
  **[T]**; position estimates from audio are good to 3.5–11 mm **[T, MDH17; S, ICASSP17]**. Players:
  near the bridge (30–60 mm) for bite and metal, over the middle pickup (~100 mm) for rhythm,
  between the neck pickup and the fretboard (150–200 mm) for jazz and soft lines **[L]**.
- **Pick against fingers:** the pick is narrow and quick (bright, an attack click), the thumb (Wes
  Montgomery) is the flesh pluck of `guitar.cc` with no nail: wide, slow, dark **[E, from
  guitar.cc's model]**. Fingerstyle electric (Knopfler, country hybrid picking: pick plus middle and
  ring fingers, "chicken picking" with snapped strings) **[L]**.
- **Velocity:** the depth of the pull (1–4 mm at the pick on an electric **[E]**) and the release
  speed; on GM 30 and 31 velocity is mostly the amount of drive, because the amp compresses level
  **[E]**. The pitch drop of a hard attack (tension modulation, `bass.h`) is audible on the light
  strings: a hard pick starts a few cents sharp **[M]**.
- **Strumming:** the built pendulum (guitars.md §13) with a pick: faster per string (a pick strum
  3.5–10 ms a string **[steel_guitar.md §4.3, E]**), the down stroke on the beat.
- **The pick's click through the pickup:** the pick's own sound is acoustic; through a pickup only
  the string's response to it is heard (the pick itself is not magnetic) **[E]**. So on the
  electrics the "click" is the string's sharp onset and the contact's high partials, not an added
  noise burst; on GM 27 the archtop's top adds some of the acoustic click **[E]**.

---

## 8. Palm muting (GM 29, and chugs on 30/31)

- **What:** the fleshy side of the picking hand lies on the strings just in front of the saddle,
  0–20 mm from it, while the pick strikes **[L]**. The closer to the saddle, the more the note
  rings; further in, the more it thuds **[L]**.
- **What it does** **[E]**: a distributed, lossy, slightly massive contact near one end. For mode k
  the string's slope (and displacement) near the end goes as sin(kπx/L) ≈ kπx/L, so the energy the
  hand takes grows about as k² × the contact's resistance: the high partials die in milliseconds,
  the fundamental and the 2nd survive for 50–200 ms. The hand's mass loads the end a little: the
  pitch goes very slightly flat **[E]**. Under distortion the short, dark note becomes a tight "chug"
  because the amp's compression lifts what remains **[L]**.
- **Measured from FluidR3's "Palm Muted Guitar" (2026-11-19, rendered by TiMidity at A2, E3, A3,
  E4):** to −30 dB the fundamental 310–510 ms, the 2nd harmonic 105–420 ms, the 4th 26–274 ms,
  the 8th 31–117 ms; the spectral centroid of the first 150 ms 220–360 Hz below E4, 920 Hz at E4;
  the same at velocity 60 and 100 **[T, sf2 rendered]**. The 60–150 ms below was 3–5 times too
  short: the built palm follows these numbers instead.
- **Numbers:** a palm-muted note to −30 dB in 60–150 ms **[guitars.md §9.2, design]**; FluidR3's
  muted samples last 0.27–0.9 s including the room's tail **[T, sf2]**. No measured decay rates of
  palm-muted notes were found (section 17).
- **Model:** per string, a damping patch at x_p (5–20 mm from the saddle) of width 10–15 mm; each
  mode's extra loss = c·(shape of the patch)², the patch shape computed as `Guitar::shapeAt` does
  for a pluck **[E]**. Parameters: mute depth (the contact's loss, 0 to "dead"), position. GM 29's
  default: on every stroke, medium depth; velocity → a little less mute (the hand lifts on accents)
  **[E]**. Shared with `research/steel_guitar.md` §4.4, which points here and to guitars.md §2.1.
- **The scratch / dead note** (funk): the fretting hand rests on the strings without pressing: all
  strings damped along their length; the pick gives a pitchless click with a short, broadband
  thump **[L]**: a very high, uniform damping over the whole length plus the pick's contact.

---

## 9. Harmonics (GM 32)

### 9.1 Natural harmonics

- A finger touching lightly at L/n forces a node there: every mode not a multiple of n is damped
  out; modes n, 2n, 3n … ring on **[M; S, Wikipedia]**. The finger lifts after 20–100 ms and the
  harmonic rings as long as those modes do **[E]**.
- Nodes and pitches on the open string: n = 2 at the 12th fret (+12 semitones), n = 3 at the 7th
  (or the 19th) (+19), n = 4 at the 5th (+24), n = 5 near the 4th (3.86 frets: +28, a pure major
  third, 14 cents flat of equal temperament), n = 6 near the 3rd (3.16 frets: +31) **[E]**.
- Inharmonicity makes the sounded harmonic sharp of n·f₁: on the plain G (B 1.1e-4), the 4th mode
  is 1.0009× → +1.5 cents; on the low E (~1.3e-4), the 5th mode is +2.8 cents **[E]**.
- **The pick position matters:** picking at a node of the harmonic kills it; near the bridge is
  safe **[M]**.
- **Already built:** `Bass::touch(s, n, secs, rate)` lays a finger at the n-th node: the same in the
  guitar class **[bass.h]**.

### 9.2 Artificial harmonics

- **Touched (or "harp") harmonics:** the fretting hand stops a fret; a picking-hand finger touches 12
  frets higher (the new L/2) and the thumb or pick plucks **[M]**: any pitch is available an octave
  above the fretted note (n = 2), or a 12th (n = 3).
- **Tapped harmonics:** a finger slaps the string at the node over the fret 12 frets up **[M]**.

### 9.3 Pinch harmonics

- The thumb's edge brushes the string just after the pick, at a node of the fretted length: on an
  overdriven amp, a squeal **[S, Wikipedia]**. Which harmonic depends on where (and the guitarist
  moves the picking hand to choose it) **[L]**: near the bridge pickup, typically n = 3–6 of a
  fretted note on the G, D or A string, sounding 2–2.5 octaves up **[E]**.
- The XG's "Gt.Pinch" on 30 is this sound **[T, name; E meaning]**. The amp makes it: the harmonic is
  20–30 dB under a normal note, and the distortion's compression brings it up to level **[E]**.
- **Model:** a touch (as 9.1) with secs ~0.005–0.015 just after the pick, at the node of the stopped
  length, the pick within a few mm of it **[E]**.

### 9.4 GM 32: from a MIDI pitch to a harmonic

- For a note of pitch p, search (string, fret, n) with n ∈ {2, 3, 4, 5}: p = open + fret + 12·log₂(n)
  (n = 5 is 14 cents flat of p: allow it only for p ≥ 76 where nothing else works) **[E]**.
- Prefer natural (fret 0) harmonics at the 12th, 7th and 5th fret nodes, then n = 2 artificial
  harmonics anywhere (fret = p − open − 12), then n = 3 **[E]**. Natural harmonics cover MIDI 52–95
  (E2's octave to E4's 6th harmonic, 64 + 31); touched n = 2 harmonics reach the 22nd fret + 12 =
  98 **[E]**.
- Sound: clean, neck or middle pickup, a little reverb and chorus (as the sets do) **[E]**.

### 9.5 As built (`-inst guitar-harmonics`)

- **Searched, not found:** a measured time for the touching finger. Teachers say only that it
  lifts "right after" the pluck, the pluck first and the release at once **[L: tonebase,
  douglasniedt.com]**. Pakarinen et al. (a waveguide with a resistor at the node, static and
  time-varying) and Tanaka et al. (JASA 2023: a flageolet keeps sounding after the finger lifts)
  model the touch as a lossy point, which is what is built **[S]**. The duration stays **[E]**.
- **The touch:** `Guitar::touch(s, n, secs, rate, width, off)`, as `Bass::touch`: each mode loses
  rate × its shape squared at L/n, averaged over a 4 mm fingertip, ±0.7 mm off the node; 300/s
  for 30–60 ms after the pick, then lifted gradually (the hold halved every 6 ms). A sudden
  lift let the bridge's forcing of the touched modes ring free: the A string's fundamental came
  back at −30 dB; lifted gradually, −55 dB. Built on the palm's machinery: a loss in `tune()`.
- **The guitar:** the clean three-pickup solid-body and Twin (GM 28's). Pickup: the middle alone. The neck pickup
  at 158 mm is 0.244 L from the saddle, on the open strings' 4th-harmonic node (the 5th fret's
  harmonic nearly silent there); the middle at 100 mm has no node near harmonics 2–5. No reverb
  or chorus in the amp: the room gives the reverb.
- **The pick:** on the harmonic's antinode nearest the bridge, L/2n (also mode 2n's node), clamped
  to 3.5–16 cm. The guitar's pick (a ~1 ms rise and a 0.15 ms slip) puts about as much into mode
  3n as into mode n; for harmonics the stroke is rounder (2–3.5 ms), and n = 3's 3rd partial
  comes down from +8 to −1 dB against the harmonic **[E]**.
- **The map:** §9.4 without n = 5; touched n = 2 reaches the 21st fret + 12 (C#7, 97), n = 3 to
  G#7 (104), beyond an octave down; under E3 an octave up. Strings whose pickup lies near the
  harmonic's node are avoided. Tuning left to the string: 0–3 c sharp (stiffness; n = 3 a pure
  twelfth, +2 c); near the neck's dead-spot resonance (212 Hz) the A string's octave and the low
  E's twelfth +5 and +7 c (the neck pulls them).
- **Levels:** harmonics differ by 14 dB across the range as plucked alike (about 1 dB a fret, the
  pickup's and the pick's share of mode n); the player evens them out (a fit, 1.5 dB rms left).
- **FluidR3's** "Guitar Harmonics" sounds **two octaves over the key** (E3 → 660 Hz, every key
  +24.0 semitones); its notes 0.3–1.1 s to −20 dB; level within ±0.8 dB over the range; velocity
  40 dB from 8 to 127. Here: as written (`PJ_GHARM_SHIFT=24` for FluidR3's), 0.7–1.8 s to
  −20 dB (a string's own decay, let ring), 1.5 dB rms (±4 at worst), 24 dB (the velocity curve
  brings it to GM's).
  Against the pitch each plays, the brightness is alike (centroid ~1.8× the pitch on E3).

---

## 10. The left hand: frets, slides, bends, vibrato, the tremolo bar

- **Frets:** `guitar.cc`'s length change with the fret's loss (1.15× at a fretted end) **[built]**. A
  fret buzz (a hard pick on the low strings against 1.5–2.5 mm of clearance) as in
  `research/steel_guitar.md` §5.1; electrics are set lower (1.2–2 mm at the 12th **[M]**), so they
  buzz more acoustically but it hardly reaches the pickup **[E]**.
- **Slides:** on a fretted guitar a slide moves over the frets: the pitch steps a semitone at each
  fret, with a short glide between **[E]**. `Bass::setLength` glides continuously (fretless); a
  guitar's slide is the same with the length quantised to the frets, a tiny "tick" on each **[E]**.
  The string squeak on wound strings: `research/steel_guitar.md` §5.2 (a pulse train at
  v/wrap-pitch, after [PPB07]); flatwounds barely squeak (the point of them) **[L]**.
- **Bends:** `Guitar::bend` (cents) **[built]**. Light strings bend 1–3 semitones on the B and G
  **[L]**; the bend is not a perfect pitch-bend: the bent string stretches, its inharmonicity and
  losses change a little, and its pitch overshoots and settles **[M]**. GM's default pitch-bend range
  is ±2 semitones **[M]**; a bend in a GM file applies to all sounding notes of the channel; for a
  guitar, apply it to the most recent (highest) note's string, and leave the others (a held
  double-stop bend is a lead player's move) **[E]**.
- **Vibrato:** finger vibrato on an electric is a pitch rise only (the string is pushed sideways):
  4–7 Hz, 10–50 cents wide, wider in rock **[L/M]**. GM files carry it as CC1 or as pitch-bend
  wiggles **[M]**.
- **Tremolo bar (three-pickup solid-body's synchronised tremolo):** the bridge tilts and all strings drop (or rise)
  together, but not by the same interval: each string's pitch change depends on its tension and
  stiffness; chords go out of tune under deep dives **[M]**. Return-to-pitch errors of a few cents
  **[M]**. Model: one control lowering every string's tension by a common length change at the
  bridge (Δf/f = ½·ΔT/T with ΔT = k_s·Δx, k_s per string from its gauge) **[E]**. Rare in GM files;
  pitch bends below zero on a lead line could be read as bar dips **[E]**.
- **Hammer-ons, pull-offs, taps:** `setLength` without a pluck **[bass.h]**; on a distorted amp
  they are loud enough to sound like picked notes **[L]**: legato lines in GM 30/31 leads.

---

## 11. The amp and effects module (independent of the guitar)

### 11.1 The interface

- **One signal chain object** that takes an instrument's output and returns the speaker's (or the
  microphone's) sound, with a preset per GM program and per style (section 12). It does not know
  which instrument is upstream: guitars, the electric bass (bass amps are the same blocks with
  another tone stack and cabinet), the tine piano and clavi (often played through guitar amps and
  pedals **[L]**), the organ (its rotary speaker is already its own module).
- **What crosses the boundary:** not just a voltage. The first stage's input impedance loads the
  pickup (6.3): a fuzz face or a 1 MΩ tube grid see the same EMF very differently. So the instrument
  passes its **EMF** and its **source network** (L, R, C, pots, cable); the chain's first block
  solves pickup + first input together, then the rest is a one-directional cascade with
  buffered interfaces **[E]**. Instruments without a passive pickup (a DI'd keyboard) pass a
  low-impedance source.
- **The path back:** only for feedback (5): the cabinet's output, delayed and scaled, into the
  instrument's bridge **[E]**.
- **Sample rate:** the chain runs at the engine's rate; each nonlinear block oversamples itself
  (11.7).

### 11.2 The preamp: triode stages

- **The circuit:** the common-cathode 12AX7 stage, cascaded 2–4 times: plate resistor Rp, cathode
  resistor Rk (the bias) with or without its bypass capacitor Ck, grid resistor, coupling
  capacitor to the next stage; the Miller capacitance across grid and plate makes it a low-pass
  **[T, PY09; YehThesis]**. Yeh's simulated stage: Rg 70 kΩ, Rk 1.5 kΩ, Rp 100 kΩ, Ri 1 MΩ, Ci
  0.047 µF, Cf 2.5 pF, Ck 25 µF **[T]**. Supply 250–300 V **[M]**.
- **Why it is not a static curve** **[T, PY09]**:
  - the cathode bypass capacitor remembers the bias and moves it with the signal's history;
  - large transients drive the grid positive: grid current charges the coupling capacitor, shifts
    the bias, and in the worst case cuts the stage off ("blocking distortion");
  - Miller capacitance low-passes the input.
  - So a played chord distorts differently in its attack and its sustain: the "sag" and "bloom"
    players hear **[L]**.
- **Tube models:**
  - **Koren:** E₁ = (E_P/k_P)·ln(1 + exp(k_P (1/µ + E_G/√(k_VB + E_P²)))), I_P = (E₁^X/k_G1)·(1 +
    sgn E₁). 12AX7: µ 100, X 1.4, k_G1 1060, k_P 600, k_VB 300, R_GI 2 kΩ; C_CG 2.3 pF, C_PG 2.4 pF,
    C_CP 0.9 pF **[T, Koren96]**. Pentode form with arctan(E_P/k_VB) and a screen current; 6L6CG: µ
    8.7, X 1.35, k_G1 1460, k_G2 4500, k_P 48, k_VB 12 **[T]**. Another 12AX7 fit: µ 100.8, k_G1
    1890, k_P 828, k_VB 72, X 1.4979, V_ct 0.612 **[S, a DAFx-07 paper via search]**.
  - **Child–Langmuir** (Yeh's proof of concept): I_P = K·(µV_gk + V_pk)^{3/2}, µ 83.5, K 1.73e-6
    A/V^{3/2}; unphysical where guitar distortion operates **[T, YehThesis]**.
  - Grid current: a diode from grid to cathode with R_GI ~2 kΩ **[T, Koren R_GI]**.
- **How to run it** (11.6): a WDF or nodal-DK state space per stage, the tube as a 2-D table; or a
  per-stage static curve computed from the Koren equations with the stage's load line, plus
  filters, plus a slow bias-shift follower (the Kuroki–Ito / Karjalainen form) **[T, PY09]**. The
  latter is the the tonewheel organ's `v + k v²; tanh` grown into a measured curve **[E]**.

### 11.3 The tone stack

- **The late-1950s tweed / British stack / blackface / British top-boost:** a passive third-order RC bridge after
  the preamp (buffered by a cathode follower: ~1 kΩ source, 1 MΩ load, which leave the response
  unchanged) **[T, YS06/YehThesis]**. Component values **[T, YehThesis Figs. 2.2–2.4]**:

| Stack | C1 | C2 | C3 | R1 (treble) | R2 (bass) | R3 (mid) | R4 |
|---|---|---|---|---|---|---|---|
| late-1950s tweed | 250 pF | 20 nF | 20 nF | 250 k | 1 M (log) | 25 k | 45 k |
| British crunch stack | 470 pF | 22 nF | 22 nF | 220 k | 1 M | 22 k | 33 k |
| British top-boost combo (2 knobs) | 50 pF | 22 nF | 22 nF | 1 M | 1 M | 10 k (fixed) | 100 k |
| American amp blackface (Twin) | 250 pF | 100 nF | 47 nF | 250 k | 250 k | 10 k | 100 k |

- **The transfer function** is exact and closed form: H(s) = (b₁s + b₂s² + b₃s³)/(1 + a₁s + a₂s² +
  a₃s³), the coefficients polynomials in t, m, l (treble, mid, bass; the bass pot logarithmic),
  checked against SPICE "exactly"; three real poles and a zero at DC; bilinear transform at run
  time, the knob-free products precomputed **[T, YS06]**. This is `tools/clavtone.py`'s method;
  either emit tables or compute the 3rd-order coefficients live (cheap) **[E]**.
- **Character:** the mid knob is a notch; every setting is a V-shaped scoop around 400–800 Hz
  that compensates the pickup's resonance **[T, PY09; M the frequency]**. Most stacks lose ~10–20 dB
  even "flat" **[M]**: the gain stage after it makes up.
- **Where:** classic American and British amps: after the first or second stage, before the phase inverter
  **[T, YehThesis]**; high-gain amps often put a stack between gain stages, which changes what
  distorts **[M]**.

### 11.4 The power amp: phase inverter, push-pull, output transformer, sag

- **Push-pull class AB** pentodes (EL34 British stack, 6L6 clean 2x12 combo, 6V6 American amp Deluxe, EL84 Vox
  British top-boost combo **[M]**): one tube pushes, one pulls; in class AB each idles near cut-off for its half,
  which gives crossover distortion at the switch-over; symmetric halves cancel even harmonics
  **[T, PY09; M the cancellation]**. Single-ended class A (a Champ) keeps even harmonics **[T, PY09;
  M]**.
- **Sag:** a class AB stage draws supply current with the signal; the supply (a tube rectifier in
  vintage amps) sags under a loud passage and compresses it **[T, PY09; Aiken via PY09]**. Time
  constants: tens to hundreds of ms **[M]**; the rotary speaker already uses 0.1 s **[leslie.cc]**.
- **Negative feedback** from the speaker winding to the phase inverter, with a "presence" control
  shaping its high end; a delayed (one-sample) feedback path changes the response only at high
  frequencies, still audible; solving the non-delayed feedback needs one unknown (the feedback
  current) per sample **[T, Mačák]**. British top-boost combos have none **[M]**.
- **Output transformer:** it adds low-frequency distortion and hysteresis **[T, PY09]**; but in an
  ABX test of a modelled American amp NSC041318 transformer, linear vs nonlinear: 32 of 60 correct, and
  hysteresis vs none: 31 of 60 (36 needed to reject "no difference"): use the linear model **[T,
  measured, Mačák]**. Its bandwidth (a band-pass of tens of Hz to ~10 kHz **[M]**) and its
  leakage resonance remain.
- **The speaker as the load:** the tube stage sees the speaker's impedance, which rises at the
  speaker's resonance (~75–100 Hz) and with the voice coil's inductance at high frequencies; with
  little feedback (a Vox) the amp's response follows it **[M]**. Model as a frequency-dependent load
  only if the power stage is a circuit model; otherwise fold it into the EQ **[E]**.
- **Simplest acceptable model** **[E]**: phase-split, two Koren pentode curves (or two asymmetric
  tanh), subtract; a slow supply follower dividing the drive (the the rotary speaker's sag); a linear
  transformer band-pass; presence/resonance shelves. All in one oversampled block.

### 11.5 The speaker and the cabinet

- **Speakers:** British speaker G12M 25 W British speaker: resonance 75 Hz, usable 75 Hz–5 kHz; 60 W British speaker: 75 Hz,
  70 Hz–5 kHz, 100 dB sensitivity **[S, Celestion data sheets via search]**. A Jensen P12 or C12 in
  American amp combos **[M]**.
- **Cabinets:** two measured loudspeaker cabinets passed up to 7–8 kHz **[T, measured, Mačák]**;
  above, the cone breaks up into peaks and dips and falls steeply **[M]**. An open-back combo (American amp)
  has less low bass and a more diffuse sound; a closed 4×12 a low-end bump (80–120 Hz) and tighter
  palm mutes **[M/L]**.
- **Microphone:** a dynamic (dynamic microphone) at a few cm, on the cone's axis (brighter) or at its edge
  (darker); a room microphone adds the room **[L]**. The direction to the cone's centre changes the
  response above 2 kHz by 5–10 dB **[M]**.
- **Model:** the cabinet is linear: a measured or designed impulse response (minimum phase, 10–50
  ms) by convolution, or 6–10 biquads (the resonance, a 1–3 kHz presence region with a dip, the
  5 kHz fall, cone-breakup peaks) **[T, Karjalainen et al. via PY09: a minimum-phase FIR]**. The
  project's rule (no samples) favours the filter design from published response curves, with a
  short designed FIR for the breakup detail **[E]**. Then into the room (`src/room.*`) as any
  source **[E]**.

### 11.6 Real-time circuit models and their cost

| Method | What it captures | Cost | Notes |
|---|---|---|---|
| Static waveshaper between filters (prefilter → curve → postfilter) | the gross curve and EQ | a table lookup per oversampled sample | Yeh: DS-1 and mid-hump overdrive with a tabulated diode curve at 8× oversampling capture "the general character"; sounds slightly brighter than the pedal (a static curve filters less than the real one), misses even-order products < −40 dB from the BJT stages **[T, YehThesis]** |
| Dynamic waveshaper (bias follows envelope; curve chosen by level and direction) | bias shift, blocking, hysteresis | a few multiplies more | Kuroki–Ito (one maker), Karjalainen et al., Gustafsson (Softube) **[T, PY09]** |
| WDF | any series/parallel circuit; one nonlinearity per tree root | diode clipper: 4 multiplies + 8 adds + the nonlinearity **[T, YehThesis]** | bridge circuits (the tone stack) and multiple nonlinearities need special adaptors **[T, PY09]** |
| Nodal DK / K-method (nonlinear state space) | any netlist; the nonlinearity's dimension = number of nonlinear ports | diode clipper: 13 multiplies + 12 adds **[T, YehThesis]**; with tables for the nonlinear part, no iteration at run time **[T, YehThesis]** | Holters & Zölzer: variable parts (a wah's pot) recomputed efficiently **[S, HZ11]**; Phase 90 by DK **[S, Eichas14]** |
| Newton on the full circuit (MNA) | everything | an American-type preamp (N = 15 unknowns): 5.48e3 operations per iteration, 2.79 iterations on average (1.53e4 per sample), 100 at worst (5.48e5); split into three blocks: 2.97e3 per iteration, 6.89e3 average, 1.39e4 worst **[T, measured, Mačák]** | at 8× oversampling the block form is ~2.6e9 operations/s **[E]**: too much for a voice in this project |
| Diode-clipper ODE (implicit, semi-implicit) | the one-capacitor clipper exactly | ≤ 8 Newton iterations per sample, ≤ 1.8 averaged over 32-sample frames, at 8× **[T, measured, YehThesis]** | semi-implicit methods: constant cost, stable **[T]** |
| Neural (LSTM, WaveNet) | a measured amp, black box | "relatively small" on a desktop; 3 minutes of audio to train; listeners could not tell the two largest from a high-gain amp maker 5:50+ **[S, WDJV20]** | not a circuit model: outside the project's rule; useful as a reference to check against **[E]** |

- **For this project** **[E]**: the pedals' clippers and each triode stage as a one- or
  two-capacitor DK/WDF block with a tabulated nonlinearity (cost like a biquad plus a lookup, at
  4–8×); the tone stacks and all linear parts as exact closed-form filters at the base rate; the
  power stage as 11.4's simple form. A clean amp then costs a few biquads plus two oversampled
  stages; a high-gain chain 4–6 stages: roughly 0.5–2 % of a core per guitar **[E, from the
  guitar's 0.7 % and the clavi's tone filters]**.
- **Checking:** `tools/circuit.py` already drives ngspice: the triode stage (with a Koren .subckt),
  the TS clipper and the RAT can be checked against SPICE as the synth leads' VCO was **[E]**.

### 11.7 Oversampling and aliasing

- Strong distortion of a guitar makes harmonics above Nyquist that fold back inharmonically
  **[T, PY09]**. One maker's patent ran the nonlinearity 8× oversampled (31.2 kHz base, 249.6 kHz)
  **[T, PY09]**. Yeh argues 8× suffices for the worst case (a comparator on a 20 kHz input) and that
  less is often masked **[T, YehThesis]**.
- **Measured audibility** (tanh after 20–60 dB of gain, a guitar solo, reference 64×): average
  oversampling factor needed 2.8–4.2, maximum 6–14; 4× satisfies most, two listeners heard
  aliasing until 14× on the most distorted signals; 8× "for most musicians" **[T, measured,
  Mačák]**.
- **Use** **[E]**: 4× for overdrive (30), 8× for distortion and fuzz (31); antiderivative
  antialiasing (ADAA) of the static parts would let these drop by half **[M]**. The pickup's and
  cabinet's low-passes before and after help: the guitar has little above 5 kHz going in.

### 11.8 Pedals

- **Overdrive: the mid-hump overdrive (mid-hump overdrive)** **[T, YehThesis, values; E arithmetic]**:
  - input high-passes at 15.9 and 15.6 Hz;
  - the clipping stage: a non-inverting op amp (JRC4558/4580) whose feedback holds two 1N914 diodes
    in antiparallel, R2 = 51 k + D·500 k (the Drive pot) with 51 pF across it; to ground 4.7 kΩ in
    series with 0.047 µF: the gain is 1 + R2/4.7 k above 720 Hz (21.5 dB at D = 0, 41.4 dB at D =
    1) and falls to unity towards DC: the mid-hump; the 51 pF puts a low-pass at 61 kHz (D = 0) to
    5.7 kHz (D = 1) **[E]**;
  - the output is the input plus the clipped difference (Vo = Vi + V): the clean signal passes
    through, only the excess is clipped: soft, asymmetric only by mismatch **[T]**;
  - the tone stage: a 2nd-order low-pass with a variable corner (Rs 1 kΩ, Cs 0.22 µF, a 20 k tone
    pot, 220 Ω + 0.22 µF) **[T]**;
  - modelled at 8× with a tabulated static clipper; misses even-order products below −50 dB and
    sounds a little brighter than the pedal **[T]**.
  - Use: as a boost into a driven amp (the bass cut tightens palm mutes in metal; SRV's blues)
    **[L]**.
- **Distortion: hard-clipping distortion pedal** **[T, YehThesis]**: a BJT input stage (an even-order source); an op-amp
  gain stage with Rt = D·100 k, Rb = (1 − D)·100 k + 4.7 k, Cz 1 µF, Cc 250 pF, which itself hard
  clips at the rails (asymmetric, slow to recover); then an RC low-pass (2.2 kΩ, 0.01 µF: 7.2 kHz
  **[E]**) with 1N914 diodes to ground: a low-pass whose pole moves with the voltage; then a tone
  control that blends a 320 Hz low-pass and a 1.16 kHz high-pass (a V-shaped "loudness" EQ)
  **[T]**.
- **Distortion: hard-clipping distortion** **[M]**: an LM308 op amp with gain to ~67 dB, its slow slew rate (~0.3
  V/µs) a part of the sound; hard clipping by 1N914s to ground; a variable "filter" low-pass. Not read
  (electrosmash was unreachable, section 17).
- **Fuzz: germanium fuzz** **[S/M]**: two transistors (germanium AC128/NKT275 or silicon BC108/BC109)
  with shunt feedback; its input impedance is low (~5–10 kΩ; the input high-pass near 14 Hz with a
  ~5 kΩ input **[S]**), so it loads the pickup: the pickup's resonance disappears, and turning the
  guitar's volume down cleans the fuzz up (the classic Hendrix trick) **[L]**. Germanium leakage
  and gain vary with temperature, so the bias drifts **[S]**. This is why 11.1 passes the source
  network.
- **Compressor** (one pedal maker Dyna Comp, Ross: an OTA, CA3080, in a feedback loop) **[M]**: fast attack
  (~1–5 ms), slow release (100–500 ms), heavy ratio; squashes the pick attack and lengthens the
  sustain: funk and country clean **[L]**. A digital feed-forward/feedback compressor with these
  ballistics suffices **[E]** (a published design tutorial exists, not read **[M, Giannoulis et al.
  2012]**).
- **Chorus and flanger (BBD):** a bucket-brigade delay (one pedal maker CE-2: an MN3207 **[M]**) of 5–15 ms
  (chorus) or 1–10 ms with feedback (flanger), swept by an LFO at 0.1–5 Hz, mixed with the dry
  signal; the BBD's clock-dependent sampling, its anti-aliasing filters and its companding noise
  are part of the sound **[M]**. Models: Raffel & Smith's BBD **[S, RS10]**; Holters & Parker's
  variable-rate model that uses the device's own input/output filters instead of interpolation
  **[S, HP18]**. The JC-120's chorus is the same in stereo (dry left, chorus right) **[M]**.
- **Phaser** (one pedal maker Phase 90: four JFET first-order all-pass stages swept by an LFO, mixed with the
  dry for notches; Univibe: four stages with a lamp and LDRs, a lopsided sweep) **[M]**; the Phase 90
  is published as a DK model **[S, Eichas14]**; Huovilainen's enhanced models of analog modulation
  effects **[S, Huo05]**.
- **Tremolo** (amplitude): American amp blackface: an opto-coupler (a lamp or LED and an LDR) on the
  signal; brownface: the power tubes' bias wobbled ("harmonic tremolo" on some splits the bands)
  **[M]**. 2–10 Hz, depth to near full; the LDR's slow, asymmetric response rounds the wave
  **[M]**. A guitarist's "vibrato" channel on American amps is this tremolo **[M]**.
- **Spring reverb** (in the amp: clean 2x12 combo, small American combo; an Accutronics/Tonewheel tank with 2–3
  springs **[M]**): the spring is dispersive: each echo becomes a chirp; the main chirp series exists
  only below a "transition frequency" fC (rarely above 5 kHz) where the group delay peaks; a second,
  wide-band series is ≥ 30 dB weaker **[T, Parker11]**. Model: two parallel feedback delay lines
  with long chains of all-pass "spectral delay" filters (100 for the low series, 200 for the high
  in Välimäki et al.); Parker's changes (stretched all-passes, the low series at fs/4, a crossover)
  cut the cost to a third; dropping the weak series entirely is "acceptable" **[T, Parker11; S,
  VPA10]**. Dripping surf reverb is the tank driven hard (Dick Dale, an American amp Reverb unit) **[L]**.
- **Delay:** tape echo (tape echo, one maker tape echo: wow, flutter, a darkening and saturating loop),
  analog BBD (one pedal maker DM-2: 20–300 ms, dark repeats), digital (clean) **[M]**. Country slapback: one
  repeat at 80–150 ms **[L]**; rock leads: 300–500 ms, 2–4 repeats, often dotted eighths (the Edge)
  **[L]**.
- **Wah:** an inductor band-pass (Crybaby: roughly 400 Hz–2.2 kHz with Q ~2–5 **[M]**) swept by the
  foot; modelled by DK with the pot as a variable part **[S, HZ11]**. Funk and Hendrix **[L]**.
- **Order:** compressor → wah → overdrive/distortion/fuzz (fuzz first, because of its input) →
  amp preamp → (effects loop: chorus, delay, reverb) → power amp → cabinet **[L]**. Modulation and
  delay before the amp sound different (smeared by the distortion) from after it **[L]**.

---

## 12. Styles: which guitar, pickup, pedals and amp

All [L] (players' practice), with the module's settings as **[E]**.

| Style | Guitar, pickup | Pedals | Amp and settings | Playing |
|---|---|---|---|---|
| Jazz (bebop, mainstream) | archtop (laminated archtop, L-5), neck humbucker, tone ~0.3, flats 12–52 | none | clean 2x12 combo/Deluxe, Polytone, JC-120: clean, bass 0.6–0.7, treble 0.3–0.4, light reverb | heavy pick or thumb, comping, single lines |
| Clean funk, R&B | three-pickup solid-body (pos. 4 or neck+middle), Tele | compressor, wah, sometimes chorus | Twin or JC-120 clean, bright, little reverb | 16th scratches, muted strokes, single-note riffs |
| Country | two-pickup solid-body bridge pickup | compressor, slapback delay (~100 ms, one repeat), B-bender | clean 2x12 combo/Deluxe at the edge of break-up, spring reverb | hybrid picking, chicken picking, double stops, bends |
| Blues | three-pickup solid-body neck or middle (SRV), ES-335 / twin-humbucker solid-body neck (B.B. King, Clapton) | mid-hump overdrive as a boost; or none | American amp tweed 4x10 amp/Deluxe cranked, or a British stack low gain; spring reverb | bends, vibrato, call-and-response |
| Classic rock | twin-humbucker solid-body bridge humbucker, or three-pickup solid-body bridge | none, or fuzz (60s), wah, Univibe (Hendrix) | British stack plexi/JCM800 cranked (power-amp distortion), 4×12 25 W British speakers | power chords, pentatonic leads |
| Hard rock / metal | superstrat or LP-type, hot or active bridge humbucker, often drop tunings | TS as a tight boost, noise gate | high-gain preamp (high-gain, 5150), mids scooped (or not), 4×12 60 W British, close-miked | palm-muted chugs, fast alternate picking, pinch harmonics, dive bombs |
| Pop ballad / 80s clean | three-pickup solid-body pos. 2/4 | chorus (CE-2), delay | JC-120 stereo | arpeggios |
| Surf | three-pickup solid-body/offset solid-body bridge | — | American amp Reverb unit + large American combo, reverb drenched | tremolo picking |

---

## 13. One preset per GM program (the sum)

| GM | Guitar spec | Pickup, controls | Playing | Chain preset |
|---|---|---|---|---|
| 27 | archtop: 628 mm, flats 12–52, A0 120 Hz / top 240 Hz body mixed at −18 dB | neck humbucker 4.5 H; tone 0.3 (0.022 µF); 700 pF cable | pick 1.5 mm at ~170 mm, or thumb for soft velocity | clean blackface (Twin), Bass 0.65 Mid 0.5 Treble 0.35; spring reverb 0.1; 1×12 open-back |
| 28 | solid: 648 mm, 10–46 round | three-pickup solid-body pos. 4 (neck+middle) single coils 2.2 H; tone 1 | medium pick ~110 mm; strums | clean blackface, reverb 0.2, chorus light (the sets' chorus send) |
| 29 | as 28 | bridge+middle (or Tele bridge 3.0 H) | palm mute every stroke (5–15 mm from the saddle), pick ~60 mm | clean + compressor; reverb 0.1 |
| 30 | solid: 628 mm (LP), 10–46 | bridge humbucker 8 H (hot) | pick ~50 mm; bends, vibrato from CC1 / pitch bend | British crunch stack stack at crunch: two triode stages driven, power stage at the edge; 4×12; 4× OS |
| 31 | as 30 | bridge humbucker, high output | pick ~40 mm; palm mute on short low notes (< 0.2 beat) | TS boost → four driven triode stages → British stack → 4×12 60 W British; noise gate; 8× OS |
| 32 | as 28 | neck single coil | natural harmonics (9.4), pick near the bridge | clean, reverb 0.3, chorus |

All [E]. GM 29 played as a bass line below E2 (louisiana ch6) should go to the electric bass, as
guitars.md §7.2 already says.

---

## 14. What a model needs

**The guitar (an extension of `Guitar`)**
1. `GuitarSpec` for electrics: steel strings (tension, B, losses from 4.1–4.2; flat or round), scale,
   22 frets, a body (none / semi / archtop table), neck dead-spot modes, an acoustic share.
2. A pick pluck (narrow contact, fast slip, pick stiffness), shared with the steel-string acoustic.
3. Palm mute: a per-string damping patch near the saddle (position, depth); the fretting hand's
   damp-all for scratches.
4. Touch for harmonics (`Bass::touch`), including the short touch after the pick (pinch).
5. Slides quantised to frets, bends, finger vibrato (rise only), a tremolo bar (optional).
6. Pickups: positions per string (three-pickup solid-body table, twin-humbucker solid-body, Tele, archtop), aperture (Gaussian 15–20
   mm or two humbucker coils), the Novak/Paiva nonlinearity, polarisation weighting; summed per the
   selector.
7. The guitar's network: L, R, C of each coil, volume and tone pots and caps, selector wiring;
   passed with the EMF to the chain.
8. The guitarist: the existing solver and strum, plus per-program choices (pick position, mute,
   harmonic search, which string carries a bend).

**The chain (a new module, independent)**
9. A first block that solves the source network against its input (1 MΩ, a pedal's buffer, a fuzz).
10. Pedals: compressor, wah (DK, a variable part), TS-type and diode-to-ground clippers (one
    capacitor + table, oversampled), a fuzz (two transistors: DK), BBD chorus/flanger, phaser,
    tremolo, delay.
11. Preamp: N triode stages (Koren tables; bias follower; Miller low-pass; coupling high-pass); a
    bright cap where the amp has one.
12. Tone stacks: exact 3rd-order coefficients for tweed 4x10 amp, British stack, blackface, Vox.
13. Power amp: phase split, push-pull pentode curves, sag, presence and negative feedback, a linear
    output transformer.
14. Cabinet: a filter set per speaker and cabinet, a microphone position; then the room.
15. Spring reverb (two dispersive chirp series, or only the strong one).
16. Presets per GM program and style; live knobs (drive, tone, reverb, the mute).

**Checks**
- The loaded pickup's response against Lemme's measured three-pickup solid-body (peak ~4.5 kHz at 470 pF/200 kΩ).
- The comb: the bridge pickup's notch at harmonic ~15 on open strings.
- The tone stack against SPICE (YS06's check), the TS and DS-1 against Yeh's spectra (220 Hz,
  100 mV in).
- Aliasing at 4× and 8× on a 60 dB-gain lead, by ear (Mačák's test).
- Palm-mute decays (60–150 ms to −30 dB), harmonics' pitch (cents against n·f₁).
- Cost: a clean electric ≤ 1.5 % of a core, a high-gain one ≤ 3 % **[E target]**.

---

## 15. What a listener would notice if it were wrong

1. **No pickup comb:** a bridge pickup that sounds like the neck one, or a neck pickup with a bright
   top; the three-pickup solid-body's in-between "quack" missing.
2. **The resonance wrong:** a single coil without its 3–5 kHz presence sounds like a keyboard
   through an amp; a humbucker with it sounds thin.
3. **A one-point pickup:** glassy, hi-fi highs no guitar has (the aperture's low-pass).
4. **Strings that decay as an acoustic's:** electrics sustain for 10+ s; short notes give it away at
   once on GM 28 and 30. Conversely, flats and an archtop that sustain like a three-pickup solid-body are not jazz.
5. **The same timbre at every velocity on GM 30:** a crunch amp cleans up when picked softly; the
   sets' samples cannot do this, a circuit model must.
6. **Distortion without compression or without the cabinet's low-pass:** fizzy, buzzing "digital"
   distortion, the most common failure of synthetic guitar.
7. **Aliasing:** a metallic, inharmonic haze on high notes and bends (worst on GM 31 leads).
8. **Major thirds in distorted chords:** if the model plays full triads on GM 31 they sound muddy
   (as they do on real amps); a guitarist would play fifths, so the voicing (the solver) matters as
   much as the amp.
9. **Palm mute as a short note:** a note simply cut is not a mute; a mute keeps a low, pitched thud
   and loses the highs first.
10. **Harmonics with a fundamental:** a GM 32 note that still has its fundamental is a plucked note
    an octave up, not a harmonic; harmonics have a soft attack and a pure, glassy tone.
11. **Bends that behave as pitch-bend:** all strings of a chord gliding together; vibrato that goes
    below pitch.
12. **Slides that glide smoothly on a fretted guitar.**
13. **Strums with simultaneous strings** (as every GM file writes them): the built strum fixes this;
    it must stay on for electrics.
14. **Chorus missing on the clean presets:** listeners of GM files expect the shimmer (every set has
    it).
15. **Feedback on GM 31 holds that never blooms:** a sustained distorted note on a real amp grows a
    harmonic after 1–2 s at stage volume.

---

## 16. Where the electric guitar shares with the electric bass and the steel-string acoustic

- **Strings:** one string class and spec type for the nylon, steel, electric guitar and electric
  bass; only the tables differ (`research/bass_section.md` §2, `research/steel_guitar.md` §2).
- **Pickups and electronics:** one `Pickup` (positions, aperture, nonlinearity, coil) and one
  controls network; the bass's P and J positions and coils are in `research/bass_section.md` §3, the
  guitar's in 6.1–6.3 here.
- **Pick, strum, palm mute, squeak, fret buzz:** `research/steel_guitar.md` §4–5.
- **Amp and effects:** the bass uses the same module (a bass amp's tone stack and a 15" or 8×10
  cabinet: other values, the same blocks).

---

## 17. Not found, or uncertain

- **Palm muting:** no measurement of a palm-muted note's decay, the hand's contact force or
  position, or a published model; the model in section 8 is an estimate.
- **Flatwound vs roundwound:** no measured decay or spectrum comparison; sources contradict each
  other on which decays faster.
- **Flatwound tensions** (standard Chromes): the tension chart PDF did not download; plain-string
  values are computed, wound ones estimated.
- **Lemme's 21-pickup table** itself, and his measured humbucker, P-90 and Tele resonance curves:
  his site's certificate had expired; only the average and the three-pickup solid-body values (via ozvalveamps) and
  the inductance list (his Sustain article) were read.
- **GuitarFreak measurements**: not found.
- **The fitted NLfnc parameters** (A, Leq, req per pickup) are in a figure of NLSB18 and were not
  extracted.
- **Paiva, Pakarinen & Välimäki (JAES 2012), Horton & Moore (2009):** paywalled; only abstracts.
- **Electrosmash's analyses** (mid-hump overdrive, RAT, germanium fuzz, DS-1) were unreachable (DNS); the RAT and Fuzz
  Face numbers are [M]/[S]. The TS and DS-1 numbers come from Yeh's thesis instead [T].
- **Assembled archtop modes** (laminated archtop, L-5): none found.
- **The one maker early GS module/88 samples' sources** and their effect settings (beyond the SC-88Pro's
  insertion-effect list): not published. FluidR3's guitar sample provenance is undocumented; what
  is said here is read from the sf2 file itself.
- **Spring reverb tank numbers** (delay times of the Accutronics types, decay): [M] only; the JAES
  spring paper was not read (its EURASIP follow-up was).
- **Strumming speed of a pick strum, vibrato rates of guitarists:** no measurements read.
- **Wah, compressor, BBD and phaser papers:** abstracts only.
- **SC-88Pro rows 31/4–31/8** fell on a page break.

---

## 18. Proposed build order

1. **The electric guitar on its own, clean, through a minimal output** (pickup network → 1 MΩ →
   a fixed clean "amp" EQ and a speaker filter): `GuitarSpec` for the three-pickup solid-body (strings 4.1, no body,
   two neck modes), the pick pluck (with the steel-string acoustic), the pickups with positions,
   aperture, nonlinearity and the loaded L–C–R, the selector. **GM 28** works at the end of this
   step (with the existing strummer). Check against Lemme's three-pickup solid-body and the comb.
2. **GM 29:** the palm-mute patch and the scratch; the guitarist mutes every stroke. This is the
   program the user's files use most (5800 notes **[guitars.md §0]**).
3. **The amp module, linear part:** the source/input block, the tone stacks (closed form), the
   cabinet filters, spring reverb, chorus. Move GM 28 and 29 onto it (Twin/JC presets).
4. **The amp module, nonlinear part:** triode stages (Koren tables in a one-capacitor DK block),
   the power stage with sag, oversampling. **GM 30** (twin-humbucker solid-body spec + British stack crunch). Then the
   mid-hump overdrive and a high-gain preset: **GM 31** (power-chord voicings in the solver; mute on
   short low notes).
5. **GM 27:** the archtop spec (628 mm, flats, the body's acoustic share), the neck humbucker with
   the tone rolled off, the clean blackface preset. Used in the user's files (1705 notes).
6. **GM 32:** the harmonic search (9.4) with `touch`; then pinch harmonics for GM 30/31 leads and the
   XG "Gt.Pinch".
7. **Then:** feedback (the path back), the remaining pedals (fuzz with its loading, wah, phaser,
   tremolo, delay), the tremolo bar, the electric basses on the same chain (with the bass agent's
   specs), and the tine piano/clavi through the chain as options.

---

## 19. References

- **[PY09]** J. Pakarinen, D. T. Yeh, "A Review of Digital Techniques for Modeling Vacuum-Tube Guitar
  Amplifiers", Computer Music Journal 33(2), 85–100, 2009. Read in full.
  https://www.effectrode.com/wp-content/uploads/2018/08/a_review_of_digital_techniques_for_modeling_guitar_amplifiers.pdf
- **[YehThesis]** D. T. Yeh, "Digital Implementation of Musical Distortion Circuits by Analysis and
  Simulation", PhD thesis, Stanford (CCRMA), 2009. Chapters 1–3 and 5.4 read: tone stack, DS-1, TS-9,
  diode clipper costs, triode stage, conclusions.
  https://ccrma.stanford.edu/~dtyeh/papers/DavidYehThesisdoublesided.pdf
- **[YS06]** D. T. Yeh, J. O. Smith, "Discretization of the '59 Fender Bassman Tone Stack", DAFx-06
  (read through the thesis' chapter 2). https://ccrma.stanford.edu/~dtyeh/tonestack
- **[YAS07]** D. T. Yeh, J. S. Abel, J. O. Smith, "Simplified, Physically-Informed Models of
  Distortion and Overdrive Guitar Effects Pedals", DAFx-07 (read through the thesis' chapter 2).
  https://dafx.de/paper-archive/details/mIq7vlQKu6pUlF467vvhgw
- **[Yeh10]** D. T. Yeh, J. S. Abel, J. O. Smith, "Automated Physical Modeling of Nonlinear Audio
  Circuits for Real-Time Audio Effects—Part I", IEEE TASLP 18(4), 2010 (downloaded, not used beyond
  the thesis). https://ccrma.stanford.edu/~dtyeh/papers/yeh10_taslp.pdf
- **[Mačák]** J. Mačák, "Real-Time Digital Simulation of Guitar Amplifiers as Audio Effects", PhD
  thesis, Brno University of Technology, 2012. Read: computational complexity (§5.2.1), cabinets,
  the transformer and aliasing listening tests (§6.4–6.5).
  https://ccrma.stanford.edu/~jingjiez/portfolio/gtr-amp-sim/pdfs/Real-time%20Digital%20Simulation%20of%20Guitar%20Amplifiers%20as%20Audio%20Effects.pdf
- **[Koren96]** N. Koren, "Improved vacuum tube models for SPICE simulations", Glass Audio 8(5),
  1996. https://www.normankoren.com/Audio/Tubemodspice_article.html
- **[NLSB18]** A. Novak, B. Lihoreau, P. Lotton, E. Brasseur, L. Simon, "Experimental Study of
  Guitar Pickup Nonlinearity", DAFx-18. https://www.dafx.de/paper-archive/2018/papers/DAFx2018_paper_39.pdf
- **[MDH17]** Z. Mohamad, S. Dixon, C. Harte, "Pickup position and plucking point estimation on an
  electric guitar via autocorrelation", JASA 142(6), 2017 (Table I: the measured Strat).
  https://webspace.eecs.qmul.ac.uk/s.e.dixon/pub/2017/MohamadDixonHarte-JASA2017.pdf ;
  ICASSP 2017 version: https://webspace.eecs.qmul.ac.uk/s.e.dixon/pub/2017/MohamadHarteDixon-ICASSP2017.pdf ;
  DAFx 2017 (with effects): https://webspace.eecs.qmul.ac.uk/s.e.dixon/pub/2017/MohamadHarteDixon-DAFx2017.pdf
- **[Lemme13]** H. Lemme, "Pickup Measuring Techniques", Sustain magazine #3, 2013.
  https://www.planetz.com/wp-content/uploads/2026/01/Pickup_Measuring_Techniques.pdf ; his site:
  https://www.gitarrenelektronik.de
- **[ozvalveamps]** "Pickups: a look at the guitar pickup as a transducer and signal source"
  (PC-ECAP simulations with Lemme's values). https://ozvalveamps.org/pickups.html
- **[planetz, Mojotone]** pickup inductance ranges. https://www.planetz.com/pickup-inductance/ ;
  https://www.mojotone.com/blog/introduction-to-guitar-pickups-part-4-spec-check
- **[BGMC24]** C. Batchelor et al., "Guitar Pickups I: Analysis of the Effect of Winding and Wire
  Gauge on Single Coil Electric Guitar Pickups", arXiv 2409.19782 (abstract used).
  https://arxiv.org/pdf/2409.19782
- **[PPV12]** R. C. D. Paiva, J. Pakarinen, V. Välimäki, "Acoustics and Modeling of Pickups", JAES
  60(10), 768–782, 2012 (abstract). https://aes2.org/publications/elibrary-page/?id=16551
- **[PLC14]** A. Paté, J.-L. Le Carrou, B. Fabre, "Predicting the decay time of solid body electric
  guitar tones", JASA 135(5), 2014 (via `research/guitars.md`).
  https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A8_Pate_PredictingDecayTime.pdf
- **[PPB07]** J. Pakarinen, H. Penttinen, B. Bank, "Analysis of handling noises on wound strings",
  JASA 2007 (abstract). https://research.aalto.fi/en/publications/analysis-of-handling-noises-on-wound-strings/
- **[Parker11]** J. Parker, "Efficient Dispersion Generation Structures for Spring Reverb
  Emulation", EURASIP J. Adv. Signal Process. 2011:646134. Read.
  https://acris.aalto.fi/ws/portalfiles/portal/13004391/art_10.1155_2011_646134.pdf
- **[VPA10]** V. Välimäki, J. Parker, J. S. Abel, "Parametric Spring Reverberation Effect", JAES
  58(7/8), 547–562, 2010 (abstract). https://aes2.org/publications/elibrary-page/?id=15511
- **[HZ11]** M. Holters, U. Zölzer, "Physical Modelling of a Wah-wah Effect Pedal as a Case Study for
  Application of the Nodal DK Method to Circuits with Variable Parts", DAFx-11 (abstract).
  https://www.dafx.de/paper-archive/details/hR2kI9NQ8b1kjRgzxNk0pw
- **[Eichas14]** F. Eichas, M. Fink, M. Holters, U. Zölzer, "Physical Modeling of the MXR Phase 90
  Guitar Effect Pedal", DAFx-14 (abstract).
  https://dafx14.fau.de/papers/dafx14_felix_eichas_physical_modeling_of_the_.pdf
- **[MHS12]** J. Mačák, M. Holters, J. Schimmel, "Simulation of Fender Type Guitar Preamp using
  Approximation and State-Space Model", DAFx-12 (title only).
  https://www.hsu-hh.de/ant/wp-content/uploads/sites/699/2017/10/Mačák-Holters-Schimmel-2012-Simulation-of-Fender-Type-Guitar-Preamp-using-Approximation-and-State-Space-Model.pdf
- **[HP18]** M. Holters, J. Parker, "A Combined Model for a Bucket Brigade Device and its Input and
  Output Filters", DAFx-18 (abstract). https://dafx.de/paper-archive/details/KbFTgvcTMmHQ2bHZvOUekw
- **[RS10]** C. Raffel, J. O. Smith, "Practical Modeling of Bucket-Brigade Device Circuits", DAFx-10
  (abstract). https://dafx.de/paper-archive/details/JhVfAOFXD1lAtctkMTUODg
- **[Huo05]** A. Huovilainen, "Enhanced Digital Models for Analog Modulation Effects", DAFx-05
  (abstract). https://dafx.de/paper-archive/details/jPSmVsZq6Xn0nTNK6sygIQ
- **[WDJV20]** A. Wright, E.-P. Damskägg, L. Juvela, V. Välimäki, "Real-Time Guitar Amplifier
  Emulation with Deep Learning", Applied Sciences 10(3):766, 2020 (abstract).
  https://acris.aalto.fi/ws/portalfiles/portal/41964332/Real_time_guitar_amplifier_emulation.pdf
- **[Overton]** W. E. Overton, digital circuit-level emulation of transistor distortion including
  the Fuzz Face, Georgia Tech thesis (title only). https://core.ac.uk/download/4682834.pdf
- **[Celestion]** G12M Greenback and Vintage 30 data sheets (via search).
  https://celestion.com/productpdf.php?id=899 ; https://cdn.shopify.com/s/files/1/0543/2003/8061/files/Celestion_Vintage_30.pdf
- **[D'Addario]** EXL110 tensions (a retailer's copy of the chart).
  https://stingray-seadragon-lafs.squarespace.com/the-music-shop/p/daddario-xl-10-46-regular-light-electric-guitar-strings-exl110 ;
  chart: https://www.daddario.com/tension-chart
- **[SC-88Pro]** Roland SC-88Pro Owner's Manual (tone list with the SC-88 and SC-55 maps; insertion
  effects). https://dosdays.co.uk/media/midi/SC-88PRO_OM.pdf
- **[DB50XG]** Yamaha DB50XG Owner's Manual (XG voice list). https://dosdays.co.uk/media/midi/DB50XGE.pdf
- **[FluidR3]** FluidR3_GM.sf2 (Frank Wen, MIT licence), as installed at
  /usr/share/sounds/sf2/FluidR3_GM.sf2; presets, instruments and samples read with a small parser.
- **[Wikipedia]** "String harmonic" (natural and pinch harmonics). https://en.wikipedia.org/wiki/String_harmonic
- Within this repository: `research/guitars.md` (§3.2–3.3, §4, §7, §9, §13), `research/steel_guitar.md`
  (§4–5), `research/bass_section.md` (§2–3), `src/guitar.*`, `src/bass.h`, `src/clavi.cc`,
  `src/rotary.cc`, `src/tonewheel.cc`, `src/lead.cc`, `tools/clavtone.py`, `tools/circuit.py`.
