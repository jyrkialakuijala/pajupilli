# The Hammond B-3 tonewheel organ (GM 17, 18, 19) as a physical model

Status: 2026-11-16. Research and plan; nothing is built. GM 17 (Drawbar Organ), 18 (Percussive
Organ) and 19 (Rock Organ) still go to `lead-calliope` in `src/midifile.cc`'s program table. The
church organ (`research/church_organ.md`, `src/organ.cc`) is a different instrument and is not
repeated here. **The Leslie speaker is a separate paper, `research/leslie.md`**; this one ends at
the organ's console output (the AO-28 preamp's output transformer), which is what a Leslie 122
receives (§10.6, and `leslie.md` §9 for the interface).

The user asked for:
- **a faithful tonewheel generator**: the wheels' teeth, the magnetic pickups and coils, the filters;
- **the wheels' irregularities** (tooth-to-tooth variation, eccentricity, wobble), modelled because
  they make **sidebands and subharmonic lines** tied to each wheel's rotation rate (§4).

Marks: **[paper]** a paper or a primary document in §15 (the Hammond service manual counts as
primary, and its Figure 24 schematic was read directly from the scan); **[web]** a weaker web
source in §15 (restorers, forums, emulator manuals); **[calc]** worked out here (scripts in the
session scratchpad, `hammond/wheels.py`, `drawbars.py`, `calcs.py`); **[memory]** general
knowledge, to be checked; **[design]** a modelling choice; **[file]** measured in the user's MIDI
files; **[rec]** measured in the user's recordings (none has a Hammond: §12.2).

## 1. Short answer

- **The generator is one synchronous motor, one shaft, 12 gear ratios and 91 wheels.** The motor
  runs at 1200 rpm on 60 Hz (1500 rpm on 50 Hz, with its own gear set) and turns a 20 rev/s drive
  shaft **[paper, HSM]**. 24 driving gears in 12 sizes turn 48 two-wheel assemblies at 12 speeds,
  one per pitch class. A wheel's frequency is 20 Hz × ratio × teeth. The teeth set the octave
  (2, 4, 8 … 128, plus seven 192-tooth wheels at the top) and the ratio sets the note **[paper,
  HSM; web, ED, GV]**. The tuning is within 0.71 c of equal temperament (A440) for wheels 1-84, and
  the top seven wheels are 1.25-1.98 c sharp **[calc]** (§3.1).
- **Every wheel turns 16.35-30.86 times a second**, whatever its octave: 20 × the ratio of its
  pitch class **[calc]**. Anything that repeats once per revolution (eccentricity, an uneven tooth,
  a dent, a magnetised spot) puts lines at **f ± k·f_rot** with f_rot = f / teeth:
  - **2-tooth wheels** (the complex pedal wheels, 32.7-61.7 Hz): f_rot = f/2, so the first
    sideband pair is the **sub-octave f/2 and the fifth above, 3f/2**;
  - **4-tooth wheels** (65-123 Hz): lines at f/4, f/2, 3f/4, 5f/4 …;
  - **128- and 192-tooth wheels** (2.1-5.9 kHz): lines ±16-31 Hz around the carrier, heard as a
    roughness or slow beat, not as a separate pitch.

  The wheels on one shaft share f_rot. All wheels also share the drive shaft's 20 Hz and the
  motor's 120 Hz (2 × mains) pulsation, which springs and couplings absorb **[paper, HSM]** (§4).
- **Hammond itself documents the irregularities.** The service manual: "The wheels are cut so to
  give as nearly a sine wave as possible, but the generated voltage seldom reaches that ideal
  condition, since even a change in the air gap will change the wave form". Copper rings on low
  coils reduce "harmonics which may be produced by irregularities in the tone wheels". Oxidation
  of the teeth "would introduce irregular frequencies" **[paper, HSM]**. No measured sideband
  level was found. §4.4 estimates them from plausible tolerances: AM sidebands at −25 to −40 dB on
  the low wheels and phase-modulation sidebands at −20 to −40 dB on the 128/192-tooth wheels
  **[calc]**.
- **The filters:** wheels 1-43 have only a resistance-wire shunt across the coil, 44-48 a reactor
  (inductor) only, and 49-91 a series capacitor into a tapped transformer, resonant at the wheel's
  own frequency (0.255 µF for 49-54, 0.105 µF for 55-91) **[paper, HSM]**. The wheels feed 9 key
  contacts per key and 9 busbars per manual. The drawbars pick taps on a **matching transformer**
  **[paper, HSM]**.
- **The electrical circuit is the AO-28 preamp** (Figure 24 of the service manual, read here, §10).
  It has two 6AU6 input channels (vibrato and non-vibrato), a 12BH7 driving the vibrato line, a
  12AX7 mixer and post-expression stage, a 12BH7 output into a balanced transformer, and a
  percussion section (6C4 amplifier, 6C4 diode keyer, 12AU7 push-pull control tube). The
  **vibrato line** is 18 LC sections (L = 500 mH, C = 0.004 µF, terminated in 15 kΩ). Its total
  delay is ~0.8 ms, its passband edge ~7 kHz, and it is scanned by a capacitive scanner at
  412 rpm = **6.87 Hz** **[paper, HSM, WDG16; calc]**.
- **Percussion** borrows the 4' or 2⅔' drawbar signal of the upper manual's B preset. A capacitor
  C31 = 0.33 µF discharges through 4.7 MΩ (slow, τ = 1.55 s) or 4.7 MΩ ∥ 1.5 MΩ (fast,
  τ = 0.375 s). It re-arms only after every upper key is released (single trigger) and takes
  over the 1' drawbar's wire as its keying line **[paper, HSM; calc]**.
- **The user's files:** in `~/Music/midi`, one file (*Good Bait*) has two GM 17 tracks (2646
  notes, 41-81, chords) and no CC1, CC64 or speed switching **[file]**. The large pop collection in
  `~/midifiles` has 269/218/434 files on GM 17/18/19. Their CC1 is a continuous wheel, not a
  switch **[file]**. **No Hammond recording** was found in the user's music **[rec]** (§12.2).

## 2. The family, and what this paper models

| Model | Years | Manuals / pedals | Generator | Preamp / output | Notes |
|---|---|---|---|---|---|
| **B-3** | 1955-1974 | 2 × 61 (C-C, 5 oct.) + 25 pedals | 91 wheels, 12 complex 2-tooth wheels, 5 blanks | AO-28; external tone cabinet or Leslie | percussion (the "-3"), scanner vibrato/chorus **[paper, HSM]** |
| **C-3** | 1955-1974 | same | same | same | "Same as Model C-2 but with Hammond Percussion"; the C cabinet is the B-3 in a closed "church" case **[paper, HSM; web, goffprof]** |
| **A-100** | 1959-1965 | same | same | AO-28 + built-in power amp, speakers, reverb | same schematic (Fig. 24: "Models B3, C3, RT3, A-100, D-100") **[paper, HSM]** |
| **M, M-2, M-3** | 1948-1964 | 2 × 44 + 12/13 pedals | **85 wheels** | built-in amp | spinet; 9 upper, 8 lower drawbars **[paper, M100]** |
| **M-100** | 1961-1968 | 2 × 44 + 13 pedals | **87 wheels** | AO-29 built-in amp | percussion; different line box **[paper, M100; web, SV-AO29]** |

- **What differs in sound [memory unless marked]:**
  - The A-100 sounds like a B-3 through a small built-in amplifier and speakers, plus reverb.
  - The M-series has a shorter compass (44 keys) and fewer wheels.
  - Its own power amp and small speakers give the "gritty spinet" sound when overdriven.
  - Its pedals are the lowest 12-13 generator frequencies, decaying, with no drawbars **[paper, M100]**.
- **This paper models the B-3/C-3** (and so the A-100 without its speakers). The M-series is a
  parameter set: fewer keys, 85/87 wheels, its own amp **[design]**.

## 3. The tonewheel generator

### 3.1 Drive, gears and tuning

- **Motors [paper, HSM]:**
  - A shaded-pole **start motor**, engaged while START is held.
  - A **synchronous run motor**: "2-pole field with 6-pole armature, 1,200 rpm at 60 cycles". The
    50 Hz version has a "4-pole armature, 1,500 rpm".
  - A resistor (250 Ω; 1000 Ω for 234 V; R87 in Fig. 24) goes in series with the start motor when
    RUN is engaged, so the system can pull into synchronism.
  - The generator must be changed for a different mains frequency ("115 volt 25 or 50 or 60 cycle
    and 230 volt 50 or 60 cycle generators").
  - So 50 Hz organs have a different gear set at the same pitch **[paper, HSM]**. A forum states
    that "all twelve gear ratios are different also in European TWG's" **[web]**. The 50 Hz gear
    table was not found.
- **Couplings [paper, HSM]:** "The synchronous motor does not deliver absolutely steady power, but
  rather operates with a series of pulsations, one with each half cycle. If the tone wheels were
  rigidly coupled to the motor, this slight irregularity would carry extra frequencies into each
  tone wheel". Two spring couplings on the motor shaft, flexible couplings between the drive-shaft
  sections and a **spring coupling on each tone wheel** absorb them. "Hunting is suppressed by the
  resilient couplings and inertia members". So 120 Hz (60 Hz mains) or 100 Hz (50 Hz) speed
  ripple, filtered mechanically, is a known residual.
- **The 12 gear pairs**, confirmed by two independent tables **[web, ED, GV]** and reproduced here
  **[calc]**. A wheel's rotation rate is 20 × ratio. A 2-tooth wheel gives twice that.

| Note | Driving/driven | Ratio | Rotation f_rot (Hz) | 2-tooth f (Hz) | Cents vs ET (A440) |
|---|---|---|---|---|---|
| C | 85/104 | 0.817308 | 16.3462 | 32.692 | −0.58 |
| C# | 71/82 | 0.865854 | 17.3171 | 34.634 | −0.68 |
| D | 67/73 | 0.917808 | 18.3562 | 36.712 | +0.20 |
| D# | 105/108 | 0.972222 | 19.4444 | 38.889 | −0.09 |
| E | 103/100 | 1.030000 | 20.6000 | 41.200 | −0.15 |
| F | 84/77 | 1.090909 | 21.8182 | 43.636 | −0.68 |
| F# | 74/64 | 1.156250 | 23.1250 | 46.250 | +0.03 |
| G | 98/80 | 1.225000 | 24.5000 | 49.000 | +0.02 |
| G# | 96/74 | 1.297297 | 25.9459 | 51.892 | −0.71 |
| A | 88/64 | 1.375000 | 27.5000 | 55.000 | 0.00 |
| A# | 67/46 | 1.456522 | 29.1304 | 58.261 | −0.29 |
| B | 108/70 | 1.542857 | 30.8571 | 61.714 | −0.59 |

- **The tuning:**
  - A is exact (20 × 88/64 × 16 = 440 Hz) **[web, ED]**.
  - G# is the flattest, at −0.71 c **[calc; web, ED: 0.69 c]**.
  - The pattern repeats in every octave up to wheel 84. These errors are far below audibility
    (a few cents) **[calc]**.
  - The fifths come out 1.34-2.66 c narrower than just (ET is 1.96 c narrow). The **major third
    (the 1⅗' drawbar) is 13.0-16.2 c sharp of the 5th harmonic**, as in ET **[calc]**. That
    beats against any 5th harmonic from distortion, for example 10.7 Hz at middle C (§7.3).
- **Mains frequency:**
  - Pitch follows the grid: a 0.02 Hz grid drift is 0.58 c, and 0.1 Hz is 2.9 c **[calc]**.
  - Every wheel, the scanner (§9) and the Leslie motors (shaded-pole, not synchronous, so only
    roughly) move together.
  - A slow global pitch wander of under 1 c is authentic **[design]**.

### 3.2 Wheels, teeth and assemblies

- **The wheels [paper, HSM]:**
  - Steel discs "about 2 inches in diameter" **[web, B3W]**, 91 working plus 5 blanks.
  - "Twelve wheels with two teeth, one to operate at each of twelve speeds, and similarly twelve
    have four teeth, twelve have eight teeth, twelve have sixteen, twelve have thirty-two, twelve
    have sixty-four, twelve have one hundred and twenty-eight and seven have one hundred
    ninety-two".
  - 48 rotating assemblies of two wheels each **[paper, HSM]**: 2-tooth with 32-tooth (four
    octaves apart), 4 with 64, 8 with 128.
  - "The twelve sixteen tooth wheels are mounted with seven one hundred ninety-two tooth wheels and
    the five blank wheels. In this last group the high frequency is not four octaves above, but is
    four octaves less five semi-tones above the lower."
  - "Each speed is isolated in a magnetically shielded compartment" **[web, B3W]**, and "five
    16-tooth wheels use blanks to keep the rotating unit balanced".
- **The 192-tooth wheels ride on the shafts a fourth above.** 256 teeth could not be cut
  **[web, ED]**. So wheel 85 (C8) sits on the F shaft: 192 × 21.818 = 4189.1 Hz, against
  4186.0 Hz for ET. These seven are 1.25-1.98 c sharp, and their rotation rates are those of F to
  B **[calc]**.
- **Frequency numbers.** Wheel n = 1 … 91 is MIDI 23 + n, i.e. wheel 1 = C1 (32.7 Hz) and wheel
  85 = C8 **[calc]**.

| Wheels | Octave | Teeth | f (Hz) | f / f_rot | f_rot (Hz) | Shaft partner | Filter |
|---|---|---|---|---|---|---|---|
| 1-12 | C1-B1 | 2 (**complex**) | 32.7-61.7 | 2 | 16.3-30.9 | 49-60 (32 teeth) | resistance-wire shunt |
| 13-24 | C2-B2 | 4 | 65.4-123.4 | 4 | same | 61-72 (64) | shunt |
| 25-36 | C3-B3 | 8 | 130.8-246.9 | 8 | same | 73-84 (128) | shunt |
| 37-48 | C4-B4 | 16 | 261.5-493.7 | 16 | same | 85-91 (192) on F-B; blanks on C-E | 37-43 shunt; 44-48 reactor only |
| 49-60 | C5-B5 | 32 | 523-987 | 32 | same | 1-12 | 0.255 µF (49-54) / 0.105 µF (55-60) + transformer |
| 61-72 | C6-B6 | 64 | 1046-1975 | 64 | same | 13-24 | 0.105 µF + transformer |
| 73-84 | C7-B7 | 128 | 2092-3950 | 128 | same | 25-36 | 0.105 µF + transformer |
| 85-91 | C8-F#8 | 192 | 4189-5925 | 192 | 21.8-30.9 (F-B shafts) | 42-48 | 0.105 µF + transformer |

### 3.3 Tooth profile, pickup and waveform

- **The pickup [paper, HSM]:**
  - "A magnetized rod about 1/4 of an inch in diameter and 4 inches in length, with a coil of wire
    wound near one end. The tip of the magnet at the coil end is ground to a sharp edge and mounted
    near the edge of the tone wheel."
  - "Small coils are used on the higher frequency magnets and larger coils on the lower
    frequencies … large pole pieces are needed on the low frequency magnets to give good frequency
    output, but it is necessary to use smaller ones on the high frequencies to prevent excessive
    iron losses."
  - Some low coils carry **copper rings**: "the eddy current loss in such a ring is small for the
    fundamental frequency of that coil, but high for its harmonics" **[paper, HSM]**. That is a
    shorted-turn low-pass on the coil.
  - "The tip of each magnet, as well as the edge of each tone wheel, is coated with lacquer to
    prevent corrosion. For, should oxidation set in, the change in tooth shape would introduce
    irregular frequencies."
- **The waveform:**
  - The rim is cut so that the gap varies sinusoidally ("as nearly a sine wave as possible")
    **[paper, HSM]**.
  - The voltage is the time derivative of the flux, and the flux is a nonlinear function of the
    gap. So even a sinusoidal rim gives harmonics, and they grow as the gap closes. "Even a change
    in the air gap will change the wave form" **[paper, HSM]**.
  - Restorers show the output going "from 0 mV to 50 mV or more with small movements" of the rod
    **[web, SV-caps]**.
  - One analysis: the pickup tip's finite width averages the field over part of a tooth. That
    smooths the high wheels, where a tooth pitch (0.83 mm on a 192-tooth 2-inch wheel **[calc]**)
    is comparable to the tip **[web, JT30, NI]**.
- **The air gap.** No figure was found in the service manual (Hammond sets it at the factory "while
  observing an output meter" and advises technicians not to touch it) **[paper, HSM]**. 0.2-0.5 mm
  is assumed below **[design; to check]**.
- **Measured harmonic content of a single wheel: not found.**
  - Reid (Sound on Sound) attributes a ~2.5 % (−32 dB) 5th harmonic, in a synth imitation of an
    888000000 patch, to "tonewheel geometry impurities and valve distortion" **[web, SOS1]**.
  - **[design]** Start with 2nd/3rd harmonics at −30 to −40 dB below the fundamental for the
    unfiltered wheels 13-43 (shunted only). Use −45 to −60 dB after the tuned filters (49-91). Fit
    by ear and to any recording the user can supply.
- **The complex wheels (1-12).** From the B-2/C-2 onward, "the original two-tooth wheels in the
  generator have been replaced with twelve two-tooth complex tone wheels, which supply a
  fundamental tone, that is enriched with the odd-number harmonics" **[paper, HSM]**. The tooth is
  shaped towards a square ("closer to a square wave, with some 3rd and 5th harmonics"
  **[web, ED]**). No harmonic levels were found. **[design]**: 3rd −10 to −15 dB, 5th −20 to
  −25 dB, then the pedal filter (§6.4). They feed the pedals (and the 16' of the lowest manual
  octave on early models only; the B-3 manuals fold back, §7.2).

### 3.4 Filters, resonances and output level by wheel

| Wheels | Circuit [paper, HSM, Fig. 24] | Inductance for resonance [calc] | Effect |
|---|---|---|---|
| 1-43 | no filter; "a length of resistance wire shunts each generator" | n/a | coil L / R low-pass; some coils with copper rings |
| 44-48 | reactor (inductor) only, "more turns" | n/a | mild low-pass |
| 49-54 | 0.255 µF + tapped transformer, resonant at the wheel's f | 363 mH at 523 Hz | band-pass on f; harmonics suppressed |
| 55-91 | 0.105 µF + tapped transformer | 247 mH at 987 Hz … 6.9 mH at 5925 Hz | same |

- **The transformer:** "a single tapped winding, and this tap is grounded, so one side, which is
  connected to the corresponding magnet coil through a condenser, forms a resonant circuit for the
  fundamental frequency of that coil". "The transformers are all different. Each transformer is
  matched to its condenser" **[paper, HSM]**. Q not given. **[design]** Q = 3-6, to be fitted.
- **Ageing:** the 0.105 µF wax capacitors drift up to "0.300 µF or more over several decades". One
  organ measured 0.22 µF at best (2.1×) and 0.40 µF at worst (~4×) **[web, SV-caps]**. That moves
  the resonance down by √(C ratio): 0.69× (−6.4 semitones) to 0.5× (an octave) **[calc]**. The
  wheel then sits on the filter's upper skirt, its level drops and its phase rotates. Some aged
  B-3s sound uneven in the top two octaves for this reason. This is an "age" parameter
  **[design]**.
- **Output level by wheel.** The generator gives at most ~30 mV p-p **[web, SV-caps]**. On an
  M-series organ a restorer recalibrated the filtered wheels to "a straight line from 12 mV
  peak-to-peak (#44) to 21 mV peak-to-peak (#91)", within 1 dB **[web, SV-caps]**, i.e. +4.9 dB
  across wheels 44-91 **[calc]**. The factory curve for 1-43 was not found. **[design]**: flat
  12 mV p-p for 1-43, then the 12→21 mV line.
- **Pedal filter.** The pedal busbars go "through a filter reactor and resistor network to the
  pedal drawbars", to "filter higher harmonics and balance pedal tones" (L20, R80-R86 in Fig. 24:
  10, 10, 47, 470, 5, 5, 20 Ω) **[paper, HSM]**.

### 3.5 Phases: one machine, locked

- **Every wheel is geared to one shaft, so all relative phases are fixed** once the organ is
  running. Wheels of one pitch class turn at exactly the same speed. Between pitch classes, the
  frequency ratios are rational (products of the gear ratios), so the phase pattern repeats
  exactly, however slowly **[calc]**.
- **No phase reset at key-down.** The key connects a wheel that is already turning. So a note
  starts at whatever phase its wheels are at, and the start is a step at a random point of a
  running sine (the root of the key click, §6.2) **[calc]**.
- **Drawbar partials of one key are not phase-locked harmonics.** The 2⅔' wheel is a tempered
  twelfth, so it drifts against the 8' at 3f_8' − f_2⅔', for example −0.62 Hz at C4 (§7.3)
  **[calc]**. The composite waveform slowly changes shape. It is inaudible on its own, but
  distortion (preamp, Leslie amp) and the Leslie turn it into slow beating. That is part of the
  "alive" quality **[calc; design]**.
- **The spring coupling on each wheel** lets the wheel lag or lead its shaft slightly under
  torque changes. Oil damps it. No resonance figure was found **[paper, HSM; to check]**.

## 4. Irregularities: sidebands, subharmonics and their sizes

### 4.1 What the sources say

- **Hammond (service manual, primary) [paper, HSM]:**
  - harmonics "produced by irregularities in the tone wheels" (hence the copper rings);
  - "even a change in the air gap will change the wave form";
  - oxidation "would introduce irregular frequencies";
  - motor pulsations "would carry extra frequencies into each tone wheel" without the couplings.
- **Emulators** model a "small amplitude modulation simulating tone wheel construction
  inaccuracies" **[paper, PPV11]**.
- **Forums:** "one of the subtleties of tonewheel sound is the frequency and amplitude variations
  caused by the wheels being out of center" **[web, DSPR]**. The AM rate "will be at a much lower
  frequency" for many teeth and "an octave or two down" for 2-4 teeth **[web, NI2]**, which is the
  user's point.
- **Not found:** any measured spectrum of a single real wheel showing the sidebands, or any
  machining tolerance for the wheels. §4.4 therefore estimates, and §13.6 asks for a recording to
  fit against.

### 4.2 Mechanisms and where their lines fall

f is the wheel frequency, T its teeth, f_rot = f/T, and f_sh = 20 Hz the drive shaft
**[calc unless marked]**.

| Mechanism | Modulates | Lines | Comment |
|---|---|---|---|
| Wheel **eccentricity** (centre offset e) | gap (AM) **and** tooth timing (PM, β = T·e/R) | f ± k·f_rot, mostly k = 1 | AM dominates on low wheels, PM on high (§4.4) |
| **Ovality / runout** of the rim (2nd harmonic of the radius) | gap | f ± 2f_rot | |
| **Tooth-to-tooth variation** (height, width, spacing) | AM and PM, a fixed pattern per revolution | f ± k·f_rot, all k, spread over f ± f/2 | a fixed random "fingerprint" per wheel |
| **Magnetised spot / dent / rust patch** | one-off per revolution | f ± k·f_rot, broad | an "aged" wheel |
| **Mean-flux change** with eccentricity (coil sees the DC field move) | baseband emf | k·f_rot itself (16-31 Hz and harmonics) | survives on wheels 1-43 (no tuned filter); then the preamp's coupling caps cut it |
| **Driven-gear runout** (bakelite gear, once per its revolution) | speed of both wheels of the assembly (FM) | f ± f_rot | the same rate as eccentricity, correlated across the assembly's two wheels |
| **Driving-gear runout** (on the 20 Hz shaft) | speed (FM) of all assemblies of that ratio | **f ± 20 Hz** | the same 20 Hz on every wheel |
| **Gear mesh** (driving teeth a at 20 rev/s) | speed ripple | f ± 20·a Hz (1.3-2.2 kHz) | tiny |
| **Motor pulsation** (one per half mains cycle) | speed of everything (FM), after the spring filter | f ± 120 Hz (or 100 Hz) | the manual names it |
| **Hunting** of the synchronous motor | speed, all wheels | f ± 1-3 Hz (to check) | "suppressed" |
| **Mains drift** | speed, all wheels | slow, < 0.1 Hz | ~0.6 c typical (§3.1) |

### 4.3 Where the once-per-revolution lines fall, octave by octave

For the note A (f_rot = 27.5 Hz) and the note C (f_rot = 16.346 Hz) **[calc]**:

| Wheel | Teeth | f | First lower / upper sidebands (k = ±1) | k = ±2 | Character |
|---|---|---|---|---|---|
| A1 (10) | 2 | 55.0 | **27.5** / 82.5 | 0 (DC) / 110 | a **sub-octave** and the 3rd harmonic of it: a different pitch |
| A2 (22) | 4 | 110.0 | **82.5** / 137.5 | **55** / 165 | lines at f/4, f/2, 3f/4: the sub-octave and sub-two-octave |
| A3 (34) | 8 | 220.0 | 192.5 / 247.5 | 165 / 275 | lines spaced f/8: a "chord" of near partials |
| A4 (46) | 16 | 440.0 | 412.5 / 467.5 | 385 / 495 | ±1 semitone-ish (±107 c), a rough cluster |
| A5 (58) | 32 | 880.0 | 852.5 / 907.5 | | ±55 c: roughness at 27.5 Hz |
| A6 (70) | 64 | 1760 | 1732.5 / 1787.5 | | ±27 c: roughness/AM |
| A7 (82) | 128 | 3520 | 3492.5 / 3547.5 | | ±13.5 c: a fast flutter |
| C1 (1) | 2 complex | 32.69 | **16.35** / 49.04 | | the sub-octave falls below hearing |
| C8 (85) | 192 on F shaft | 4189.1 | 4167.3 / 4210.9 | | f_rot = 21.82 Hz, not C's 16.35 Hz |

- **On the low wheels the "irregularity" is musical:** the lines are subharmonics f/2 and f/4 and
  their odd multiples. A slightly eccentric 2- or 4-tooth wheel adds a faint 16' or 32' tone and a
  5⅓'-like partial under the note **[calc]**. That is what the user expects to hear.
- **On the high wheels it is a roughness:** sidebands ±16-31 Hz (±7 to ±110 c), a fast AM/FM
  shimmer. Every note of one pitch class has the same rate. Different pitch classes differ, so a
  chord has several shimmer rates **[calc]**.
- **The 20 Hz drive-shaft line is common to all wheels.** If present, every note carries ±20 Hz
  sidebands of the same phase, so chords get a coherent 20 Hz "buzz" **[calc]**. **The 120 Hz
  motor line** is similar but heavily filtered by the springs **[paper, HSM]**.

### 4.4 Plausible magnitudes [calc]

The assumptions are marked **[design]**: wheel radius R = 25.4 mm (a 2-inch wheel), gap g =
0.25-0.5 mm, and pickup sensitivity ∝ g^(−1.5) near the operating point (so the AM index is
m ≈ 1.5·e/g). Eccentricity e is 5, 15 or 40 µm (good / ordinary / worn or bent). Sideband level
relative to the carrier is m/2 for AM and β/2 for PM (small index).

| e | AM index m (g = 0.25 / 0.5 mm) | AM sidebands | PM β = T·e/R: T = 2 / 16 / 128 / 192 | PM sidebands |
|---|---|---|---|---|
| 5 µm | 0.030 / 0.015 | −36 / −42 dB | 0.0004 / 0.003 / 0.025 / 0.038 | −74 / −56 / −38 / −35 dB |
| 15 µm | 0.090 / 0.045 | −27 / −33 dB | 0.001 / 0.009 / 0.076 / 0.113 | −65 / −46 / −28 / −25 dB |
| 40 µm | 0.24 / 0.12 | −18 / −24 dB | 0.003 / 0.025 / 0.20 / 0.30 | −56 / −38 / −20 / −16 dB |

- **Tooth spacing jitter** (σ_x is the rms tooth-position error) spreads power σ_φ² over all
  T lines, with σ_φ = 2π σ_x / pitch:
  - T = 128 (pitch 1.25 mm), σ_x = 5 µm: total −32 dB, −53 dB per line;
  - σ_x = 20 µm: total −20 dB, −41 dB per line;
  - T = 16: −50 to −38 dB total.

  This mostly matters for the top two octaves **[calc]**.
- **What survives the filters:**
  - The tuned filters of wheels 49-91 pass the near sidebands (within ±3 % of f for T ≥ 32) almost
    unchanged.
  - They suppress the harmonics of distortion.
  - Wheels 1-43 have no tuned filter, so their sub-octave lines (T = 2, 4) pass. Only the copper
    rings and the coil's L/R low-pass act on the upper ones **[calc from HSM]**.
- **[design] Defaults:**
  - e drawn per wheel from a half-normal with median 12 µm (AM sidebands ~−30 dB on low wheels,
    PM ~−30 dB on the 128/192-tooth wheels);
  - σ_x = 5 µm;
  - a 20 Hz shaft FM of 0.02 % (≈ 0.35 c peak, sidebands ~−75 dB at 1 kHz; inaudible by itself);
  - 120 Hz FM 10× smaller.

  The defaults are meant to be clearly audible only when the user turns an "age/irregularity"
  knob up. That knob scales e and σ_x together up to the 40 µm row.

### 4.5 Ageing

| Effect | Mechanism | Model handle [design] |
|---|---|---|
| Dried / drifted wax capacitors | resonance falls 0.5-0.7× (§3.4); level and phase of wheels 49-91 change | per-wheel C factor 1-4 |
| Gummed or dry oil | sluggish start; more torsional jitter of wheels on their springs **[paper, HSM]** | wheel-speed noise (low-passed, ~0.01-0.05 %) |
| Lacquer worn, rust on teeth | "irregular frequencies" **[paper, HSM]** | per-tooth amplitude noise, larger σ |
| Magnet drift / knocked rods | level and harmonic content per wheel | per-wheel gain ±2 dB, harmonic level |
| Dirty busbars / contacts | crackle, intermittent harmonics (§6.2) | contact noise |
| Line-box caps drift (+69 % average in one box) **[web, SV-caps]** | vibrato line slower, duller (§9) | line C factor |

### 4.6 A model of the irregularity (parameters) [design]

For each wheel w (teeth T, rotation rate f_rot; the revolution angle θ(t) = 2π∫f_rot·s(t)dt, where
s(t) is the shared speed factor below):

- **The rim gap:** g(θ) = g0·[1 + ε1 cos(θ − φ1) + ε2 cos(2θ − φ2)]
  + Σ_i h_i·bump(Tθ − 2πi − δ_i). Here ε1 = e/g0, ε2 is the ovality, and h_i, δ_i are tooth i's
  height and position errors (Gaussian, σ_h, σ_δ = 2πσ_x/pitch). For the complex wheels the bump
  is the square-ish profile.
- **The flux:** Φ = Φ0·(g/g0)^(−n) with n ≈ 1.5. The voltage is u = N·dΦ/dt, which gives the
  harmonics, the AM and the baseband f_rot line in one formula.
- **The coil and filter:** a linear filter per wheel range (§3.4), with Q and an age factor on C.
- **The shared speed factor:** s(t) = 1 + a20·sin(2π·20t + φ_shaft[ratio]) + a120·sin(2π·120t)
  + mains drift + per-assembly f_rot runout.
- **Seeding:** all random draws come from one seed, "the organ's serial number", so an organ keeps
  its fingerprint from run to run.

## 5. Leakage and crosstalk

- **Where it comes from [web, EVB3, ED, SBF]:**
  1. **The shaft partner** (the wheel four octaves away on the same assembly: 1-12 ↔ 49-60, etc.,
     and 42-48 ↔ 85-91): magnetic pickup of the neighbour wheel. "The signal of the lower wheel
     contains a small amount of audible crosstalk, induced by the higher wheel, and vice versa"
     **[web, EVB3]**.
  2. **The compartment:** "each speed is isolated in a magnetically shielded compartment"
     **[web, B3W]**, but not perfectly.
  3. **Filter transformers** on top of the generator, "mounted at an angle, thus minimizing
     interference between them" **[paper, HSM]**: so some mutual inductance remains.
  4. **The terminal strip, the manual cable and the key-contact wiring**: all 91 frequencies are
     live on every key's contacts at all times ("All key contacts are live at all times"
     **[paper, HSM]**), so busbars pick them up capacitively even with no key down.
- **The level: not measured in any source found.**
  - "The B3 tonewheel generators aren't completely quiet, even if all drawbars are at their
    minimum positions" **[web, EVB3]**.
  - setBfree's defaults are compartment crosstalk 0.01, terminal-strip 0.01, wiring 0.01 and
    transformer 0 (i.e. **−40 dB per path**) **[web, SBF]**.
  - A patent on Hammond-inspired effects tries −24, −18 and −12 dB as settings **[web, US10199024]**,
    which is too loud for a healthy organ **[memory]**.
- **[design] Defaults:**
  - The shaft partner at −45 dB into its neighbour's output.
  - The other wheels of the same compartment at −55 dB.
  - The "background whine" with no key pressed (all 91 wheels into the busbars through wiring
    capacitance): each wheel at −70 dB relative to a full drawbar. That gives a broadband whine
    ~−55 dB in sum.
  - Leakage scales with the expression pedal (it enters before the preamp). Expose a "leakage"
    knob (0 to +15 dB).
  - Measure it if any recording is supplied (§13.6).
- **Through keyed notes,** the busbar carries the key's 9 frequencies plus a little of neighbouring
  contacts' frequencies: per key a few extra wheels at −50 to −60 dB **[design]**.

## 6. The manuals: keys, contacts, busbars and the key click

### 6.1 Wiring

- **Per key, 9 contact springs** connected by resistance wires to the 9 generator terminals of that
  key's footages. "The resistance wires avoid overloading of the generators" so one wheel can feed
  many keys **[paper, HSM]**.
- **Per manual, 9 busbars.** Pressing the key moves its 9 springs onto the 9 busbars. Each busbar
  carries one footage of every pressed key.
- **The preset keys.** The busbars go through a **preset key** (C# to A: fixed presets on the preset
  panel; A# and B: the two drawbar sets "A" and "B"; C: cancel) to the drawbars **[paper, HSM]**.
- **The drawbars.** Each drawbar slides over 9 bars "tapped into the mixing transformer and
  correspond to different sound intensities" (0-8) **[paper, HSM]**.
- **The matching transformer** (T1: lower and pedals; T2: upper) matches "the low impedance of the
  generator" to the preamp's high-impedance input **[paper, HSM]**.
- **The pedals:** 25 pedals with 8 contacts each and 4 busbars, through the L20 / R80-86 filter to
  the two pedal drawbars **[paper, HSM]**.

### 6.2 Key click

- **The cause [calc from HSM].** Each of the 9 contacts closes onto a busbar carrying a running
  sinusoid, so each closure is a step from 0 to the instantaneous value of that wheel. The 9
  contacts of a key close at slightly different times (spring height tolerances, a tilted key),
  and each closure bounces. The click is the sum of these 9 × (bounces) steps:
  - broadband;
  - strongest for high footages (a step in a fast sine);
  - different every time (the phase of each wheel at closure is effectively random, §3.5).
- **Spread of closing times [calc; to measure].** Key dip ~10 mm. The contacts close in the last
  part of it. Spring-height scatter of 0.1-0.3 mm at a key speed of 0.1-1 m/s gives **0.1-3 ms**
  between first and last contact. Bounce ~0.05-1 ms.
  - setBfree's click model uses random attenuation on closing connections, level 0.5, with a
    length between 0.125 and 0.625 "units" (ms in its source, to check) **[web, SBF]**.
  - Pekonen et al. imitate the click with an enveloped 6th harmonic **[paper, PPV11]**, which is a
    perceptual shortcut.
- **The level:** no measurement found. **[design]** Simulate it physically: per contact a closure
  time (key-speed dependent), 0-3 bounces with exponentially shrinking gaps, and contact resistance
  that falls from a few kΩ to ~0 over 0.1-0.5 ms (a dirty contact stays noisy). The click level
  then follows from the drawbar setting with no free parameter.
  - Release clicks happen the same way, quieter (setBfree 0.25 vs 0.5 attack) **[web, SBF]**.
  - Tune contact scatter and bounce until a full-drawbar note's click is ~10-20 dB above the steady
    tone in the first 2 ms above 5 kHz **[design; to check against a recording]**.
- **No velocity.** A Hammond has no velocity sensitivity. Key speed only changes the click's
  spread **[memory]**. That is the one honest use of MIDI velocity (§13.3).

## 7. Drawbars, foldback and tapering

### 7.1 Footages and steps

| Drawbar | 16' | 5⅓' | 8' | 4' | 2⅔' | 2' | 1⅗' | 1⅓' | 1' |
|---|---|---|---|---|---|---|---|---|---|
| Harmonic of 8' | ½ | 1½ | 1 | 2 | 3 | 4 | 5 | 6 | 8 |
| Wheel offset from the 8' wheel | −12 | +7 | 0 | +12 | +19 | +24 | +28 | +31 | +36 |
| Colour | brown | brown | white | white | black | white | black | black | white |

- The drawbar footages and their harmonics are from **[web, SOS1, SV-draw]**; the colours are
  **[memory]**.
- **Levels per step.** The manual says only that the 9 drawbar bars are taps of the matching
  transformer **[paper, HSM]**. Two figures:
  - the folklore and most emulators use 3 dB per step (8 = 0 dB … 1 = −21 dB) **[web, SV-draw;
    Cherry]**;
  - a reader's measurement of "a typical matching transformer" gives
    **8: 0, 7: −3.0, 6: −6.0, 5: −10.5, 4: −15.2, 3: −20.0, 2: −26.1, 1: −32.1 dB** **[web,
    SV-draw comment, L. Hamre]**.
  - **[design]** Use Hamre's taps (a real transformer), keep 3 dB as a switch, and ask the user
    (§14).

### 7.2 Foldback [calc; web, ED]

With the 8' wheel of key k (1-61) = k + 12, each drawbar's wheel is k + 12 + offset, folded by
octaves into 13…91:

| Drawbar | Wheels used | Folded keys | Note |
|---|---|---|---|
| 16' | 13-61 | keys 1-12 fold **up** an octave (to 13-24) | the lowest manual octave's 16' sounds 8' |
| 5⅓' | 20-80 | none | |
| 8' | 13-73 | none | |
| 4' | 25-85 | none | |
| 2⅔' | 32-91 | key 61 | |
| 2' | 37-91 | keys 56-61 | |
| 1⅗' | 41-91 | keys 52-61 | |
| 1⅓' | 44-91 | keys 49-61 | |
| 1' | 49-91 | keys 44-61 (the top 18 keys) | |

- This matches the ranges Electric Druid gives **[web, ED]**: fundamental 13-73, 2nd 25-85, 3rd
  32-91, 4th 37-91, 5th 41-91, 6th 44-91, 8th 49-91.
- So a full registration on a high key has repeated wheels. Two drawbars then share one wheel, and
  their levels add on one frequency. **"Top C uses tones 80 and 85, three times over"**
  **[web, ED]**.
- Manual wheels never go below 13. The complex wheels 1-12 serve only the pedals **[calc]**.

### 7.3 Detuning of the drawbar "harmonics" against the 8' [calc]

| Drawbar | Cents vs exact harmonic (unfolded keys) | Beat against h·f_8' at C2 / C4 / C6 |
|---|---|---|
| 5⅓' | −1.34 to −2.66 | −0.08 / −0.31 / −1.23 Hz |
| 2⅔' | −2.66 to 0 | −0.15 / −0.62 / −2.46 Hz |
| 1⅗' | **+13.0 to +16.2** | **+2.7 / +10.7 / +49 Hz** |
| 1⅓' | −2.66 to 0 | −0.31 / −1.23 Hz |
| 4', 2', 1' | 0 to +2.66 (192-tooth wheels) | 0 (or +4.5 Hz at the top) |

### 7.4 Tapering and loading

- **Tapering.** The key resistance wires are not all equal ("manual tapering"). Hammond graded the
  level of each harmonic along the keyboard. In particular the upper footages are reduced on high
  keys **[web, SV-draw]**. The B-3 values were not found. setBfree carries a per-key, per-bus,
  per-wheel taper table **[web, SBF]**. **[design]** Begin with a 0 to −6 dB linear roll-off over
  the top two octaves for 2' to 1'. Expose it.
- **Loading.** Many keys sharing a wheel load it through their resistance wires, and a dense chord
  slightly compresses. When two keys use the same wheel, the combination is not +6 dB
  **[web, SV-draw]**. **[design]** Model each wheel as a Thévenin source (u_w, R_w) and each
  contact as a resistance. Solving the 9 busbars per manual as a small resistive network gives the
  loading for free (cost §13.4).

## 8. Percussion

### 8.1 How it works [paper, HSM]

- **Availability.** Upper manual only, and only with the **B** preset key down. Four tablets:
  - On/Off;
  - Volume Soft/Normal;
  - Decay Fast/Slow;
  - Harmonic Second/Third (the 4' or the 2⅔' drawbar's signal).
- **The borrowed signal.** "The percussion tones are produced by borrowing the 2nd or 3rd harmonic
  signal from the corresponding drawbar (of the upper manual B adjust key drawbar group),
  amplifying it, returning part of it to same drawbar, and conducting the balance through
  push-pull control tubes, which when keyed cause the signal to fade away". A third winding on T5
  feeds the borrowed signal back to the drawbar through R50, so the 4' or 2⅔' drawbar still plays
  normally.
- **The 1' drawbar is "stolen".** Percussion On "disconnects the 8th harmonic drawbar from its
  signal wire. This wire (connected through generator filters to ground when any key is pressed)
  is connected to terminal K. The 8th harmonic signal is not available on the upper manual as long
  as percussion tablet is on". The 1' busbar is the **keying sensor**: any upper key grounds it.
- **Single trigger.** When a key grounds K, V6 (a 6C4 used as a diode) stops conducting, "the grid
  then drifts from approximately +25 volts to about +15 volts, at a rate determined by the time
  required for C31 to discharge through R57 and R58". "No further percussion effects occur until
  all keys of the upper manual are released and control grids can again rise to +25 volts. The
  rate of this rise is fixed by the time required to charge C31 to +25 volts through R55 and R56."
- **Balance.** Percussion On inserts R1 in series with the upper manual's matching transformer
  secondary, which lowers the upper manual "so that lower manual will musically balance". Soft
  shunts R46 across the percussion output transformer **and** shorts R1, restoring the organ level.
  Fast decay reduces the control-tube bias, so it is "about 50 %" (+3.5 dB) louder at the start, to
  "preserve the same effective loudness".
- **Routing.** The percussion signal joins "after the vibrato but before the expression control":
  it never gets vibrato.

### 8.2 Component values and time constants (Fig. 24, read here) [paper, HSM; calc]

| Part | Value | Role |
|---|---|---|
| C31 | 0.33 µF | decay capacitor |
| R58 | 4.7 MΩ | slow decay: τ = 1.55 s |
| R57 | 1.5 MΩ | ∥ R58 for fast: 1.137 MΩ, τ = 0.375 s |
| R55 + R56 | 82 kΩ + 22 kΩ | re-arm: τ = 34 ms |
| R59 | 0.12 MΩ | control-tube bias (removed in fast) |
| R60 | 30 kΩ pot | percussion cut-off adjustment |
| R46 | 12 kΩ | soft shunt |
| R61 | 33 kΩ (selected) | balance |
| V5 6C4, T4, T5 | | percussion amplifier (R47 47 kΩ, R48 0.47 MΩ, C28 0.047 µF, C29 100 pF, C30 25 µF, R49 2.2 kΩ) |
| V7 12AU7 | | push-pull control tube; T6 out; C32 390 pF, R62 0.1 MΩ, C33 3.3 nF, C34 1 nF, C35 150 pF, R63 0.39 MΩ (high-pass and shaping into V4) |

- **The audible decay [calc].** The grid falls exponentially from +25 V toward a lower asymptote,
  and the signal is gone at ~+15 V (cut-off, set by R60).
  - If the asymptote is ~0 V, the time to cut-off is τ·ln(25/15): **slow 0.79 s, fast 0.19 s**.
  - The amplitude curve is the control tube's gain against grid voltage, falling to zero at
    cut-off. It is closer to an exponential that ends early than to a clean exponential.
  - **Cross-check:** these match the folklore "about 0.2 s fast, about 1 s slow" **[web, weak]**.
    setBfree's 1 s / 4 s (its decay constants) have the same 1:4 ratio as this RC (4.1:1)
    **[web, SBF]**.
- **Levels.** setBfree's soft is 0.5012 (−6 dB) of normal **[web, SBF]**. The R46 value suggests
  a −3 to −6 dB drop **[calc, rough]**. The fast/slow +50 % is from the manual.

## 9. The vibrato and chorus scanner

- **The line [paper, HSM Fig. 24; paper, WDG16, Table 1]:**
  - 18 series inductors L1-L18 of 500 mH and 17 shunt capacitors of 0.004 µF, plus a last one of
    0.001 µF, terminated in Rt = 15 kΩ.
  - The first six taps go through resistor dividers (R+ / R−: 27k/68k, 56k/150k, 39k/150k,
    33k/180k, 18k/180k, 12k/180k) that set tap levels.
  - The chorus resistor Rc = 22 kΩ (R44) is shorted for vibrato. **[calc]**:
    - √(LC) = 44.7 µs per section;
    - Z0 = √(L/C) = 11.2 kΩ (Rt = 15 kΩ is near it);
    - the passband edge 1/(π√(LC)) ≈ 7.1 kHz (SPICE: 7075 Hz **[paper, WDG16]**);
    - total delay ≈ 0.78-0.85 ms.
  - The line low-passes and smears as it delays: "the impulse is delayed and 'smeared'
    progressively" **[paper, WDG16]**.
- **The scanner [paper, HSM]:**
  - "Driven at 412 revolutions per minute", mounted on the generator's synchronous motor. That is
    **6.867 Hz** at 60 Hz, mains-locked **[calc]**.
  - 16 sets of stationary plates and one rotor; 9 line taps feed them (1→9→1 per revolution). A
    full-line sweep takes "about 1/14 second".
  - Plate capacitance is ~24.5 pF by one measurement **[web, DJB]**.
  - The scanner output is a capacitive crossfade between the two overlapping stacks
    **[paper, WDG16]**.
- **Depth [paper, HSM; calc]:**
  - "No. 3 scans the whole line (wide vibrato), No. 2 scans about half, and No. 1 scans about
    one-third". The full scan "causes about 1-1/2 percent decrease in frequency".
  - With a 0.78-1.07 ms line swept in 1/(2·6.87) = 72.8 ms, the frequency deviation is
    ±1.1-1.5 % = **±18-26 c (V3), ±9-13 c (V2), ±6-9 c (V1)**.
  - The sweep is near-linear in time (triangular delay), so the FM is near-**square** (two pitch
    levels with smoothed switching), not sinusoidal **[calc]**. That is part of its character.
- **Chorus:** "Mixing the vibrato signal with unvibrato signal produces a chorus effect" (C1-C3
  depths as V1-V3) **[paper, HSM]**. The vibrato channel has one extra gain stage to make up the
  line's loss **[paper, HSM]**.
- **The switch.** It has no Off position. The Vibrato Swell and Vibrato Great tablets put each
  manual on the vibrato or the dry channel **[paper, HSM]**.
- **setBfree** uses 7.25 Hz and modulation depths 3, 6, 9 (its units) for V1-V3 **[web, SBF]**.
  The service manual's 412 rpm is preferred **[design]**.

## 10. The electrical circuit (AO-28, Fig. 24 of the service manual)

### 10.1 The signal path [paper, HSM, Fig. 24, read from the scan]

1. **Wheel → coil → filter** (§3.4) → the generator terminal strip (91 terminals plus 3 grounds).
2. **Terminal → resistance wire → key contact → busbar → preset/adjust key → drawbar tap → matching
   transformer** (T1 lower and pedals, T2 upper). The secondary loading on T2 is R2 = 1 MΩ and
   C1 = 4.7 nF; R1 = 3.9 MΩ is the percussion series resistor.
3. **Vibrato routing.** "Vibrato Swell / Great" tablets route each manual to the **vibrato channel
   input (B)** or the **non-vibrato input (A)**.
4. **Non-vibrato channel:** R4 1.8 MΩ ∥ C2 39 pF, R5 10 MΩ, R6 47 kΩ and C3 100 pF into **V1
   6AU6** (plate ~130 V, screen 60 V via R7 2.2 MΩ / C4 0.33 µF, R8 0.47 MΩ plate load). Then
   C5 0.01 µF, R9 0.82 MΩ and **R10 3.9 MΩ ("selected at factory to balance channels")**.
5. **Vibrato channel:** R16 47 kΩ, R12 0.27 MΩ and C8 100 pF into **V2 6AU6** (R17 1.2 kΩ ∥ C9
   30 µF cathode; R19 0.47 MΩ plate load; R20 2.2 MΩ / C10 0.33 µF screen). Then C12 4.7 nF into
   **one half of V3 12BH7**, a cathode-coupled driver (R21 1 MΩ; R22 820 Ω; R23 and R24 22 kΩ;
   C13 1 µF), which feeds the **line** at its input (D). The scanner's output comes back through
   R13 4.7 MΩ ∥ C7 24 pF, R14 10 MΩ, R15 2.7 MΩ, R18 10 MΩ and C11 4.7 nF.
6. **Mixer: V4 12AX7 (first half)** (C6 300 pF, R27 1.8 MΩ, R28 0.27 MΩ, C15 51 pF feedback;
   plate ~145 V with R29 0.33 MΩ). The **percussion signal** joins here.
7. **Expression control:** C16 0.033 µF, then a **compensating network** (C18 and C20 220 pF; R30
   and R32 1.8 MΩ; R31 and R33 68 kΩ with C19 2.2 nF and C21 1 nF to ground; R34 15 MΩ) and the
   **expression control**, a variable capacitor whose rotor "meshes with either" a LOUD stator fed
   directly or a SOFT stator fed through the compensating network **[paper, HSM 5-43, 5-45]**.
   Then R35 3.3 MΩ to ground. Radio/phono input via C17 680 pF. C22 is a 24-90 pF trimmer "set at
   factory".
8. **V4 12AX7 (second half)**, plate ~140 V, R36 0.33 MΩ. Then C23 0.033 µF, C24 82 pF, R37 and
   R38 (2.7/3.9 MΩ class; the scan is unclear) and R39 3.3 MΩ, and the **Soft/Normal "Volume"
   tablet**.
9. **Tone control:** R40 3 MΩ pot, R41 1 MΩ and C25 1 nF (a treble cut). This is the "tone control
   … whereby the relative intensity of the high and low frequencies may be changed to suit
   acoustical conditions" **[paper, HSM]**.
10. **Output: V3 12BH7 (second half)**, plate ~285 V, cathode 12 V (R42 1.2 kΩ ∥ C26 30 µF), into
    **T3**, a balanced output transformer (G-BN-G). It goes to the "power amplifier and speakers
    (in tone cabinets)", i.e. a Hammond tone cabinet or a **Leslie 122** (`leslie.md` §9).
11. **Power supply:** T7 (340-0-340 V rms), **V8 6X4** rectifier (375 V → R91 900 Ω → 335 V), and
    a decoupling chain to +290, +280, +205, +37 and +33 V (R94-R101; C56-C60 30-50 µF).

### 10.2 Tubes

| Tube | Type | Role |
|---|---|---|
| V1 | 6AU6 | non-vibrato channel preamp |
| V2 | 6AU6 | vibrato channel preamp |
| V3 | 12BH7 | ½ line driver; ½ output stage into T3 |
| V4 | 12AX7 | ½ mixer (percussion joins); ½ post-expression amplifier |
| V5 | 6C4 | percussion amplifier |
| V6 | 6C4 | percussion keying diode |
| V7 | 12AU7 | percussion control, push-pull |
| V8 | 6X4 | rectifier |

### 10.3 What the circuit means for the sound [calc; design]

- **The expression is not a plain volume.** At the soft end the signal comes through the
  compensating network: R-C sections that pass bass and cut the mids less than the treble. So
  closing the pedal makes the organ thinner **and** relatively bassier (a loudness compensation).
  The pedal's dB range was not found. **[design]** Model it as the variable-capacitor divider
  (C_LOUD and C_SOFT varying linearly with travel, into R35 = 3.3 MΩ) with the network as drawn.
  That gives the taper and the tonal tilt from the schematic. Sanity-check that the range is
  about 30-40 dB **[to check]**.
- **Where it distorts.** The 6AU6 and 12AX7 stages run at small signals (generator mV), so they are
  clean. The 12BH7 output into T3 can be driven harder when many drawbars and keys sum, with the
  expression open and the Normal volume. The famous "growl" comes mostly from the **Leslie 122's
  power amp** (`leslie.md` §7) and from players adding external preamps. **[design]**: a mild
  tube waveshaper on V3's output half (asymmetric, soft) and on V4b. Most of the drive goes in the
  Leslie amp.
- **Where it is noisy.** The 6AU6 inputs see mV-level signals through megohm resistors. The B-3 has
  a hiss and hum floor, partly the leakage of §5 **[memory]**. **[design]**: thermal noise of R5,
  R14 (10 MΩ class) and a −70 dB 60/120 Hz hum, with a knob.

### 10.4 Schematic sources, and what could not be read

- **Read:** Fig. 24, "Schematic diagram of Hammond organ Model B-3 or C-3", service-manual page
  3-12 (PDF page 91 of the archive.org scan, 72 dpi). The values above were read from it. Where a
  digit was unclear it is marked.
- **Not read:**
  - The separate "exact circuit of preamplifier" (amplifier section) and the line-box drawings
    (vibrato section) of the same manual, which the figure refers to.
  - The AO-28 schematic on elektrotanya (a gated DjVu) **[web, ELT]**.
  - The Leslie 122 amplifier schematic (gated), discussed in `leslie.md` §7.

## 11. Sound and use

### 11.1 Registrations and their static spectra [calc]

Partials at key C4 (8' = wheel 37, 261.54 Hz), with drawbar levels from Hamre's taps (§7.1).
Frequencies come from §3.1.

| Registration | Partials (Hz : dB) | Character |
|---|---|---|
| **888000000** | 130.77 : 0, 261.54 : 0, 392.00 : 0 | the jazz "standard": sub-octave, 8', and a slightly narrow twelfth below the octave |
| **888800000** | + 523.08 : 0 | brighter; Jon Lord and many gospel/rock settings **[memory]** |
| **888000000 + perc. 3rd fast** | as 888000000, plus 784.00 : 0 dB decaying (fast, ~0.2 s), minus the 1' (already 0) | **Jimmy Smith** **[memory; web, MPF]** |
| **800000008** (the user's "80 0000 008") | 130.77 : 0, 2092.3 : 0 | hollow, whistly; a jazz ballad and gospel colour |
| **888888888** | all 9 at 0 dB; the 1⅗' at 1318.4 Hz, 13 c above 5 × 261.54 = 1307.7 Hz | "full organ"; on the top 18 keys several fold back and add |
| **838000000** | 130.77 : 0, 261.54 : 0, 392.00 : −20 | softer bluesy comping |

- Add to every line:
  - each wheel's own harmonics (§3.3) and its irregularity sidebands (§4);
  - the leakage floor (§5);
  - the key click at onset (§6.2);
  - vibrato/chorus (§9) and the Leslie (`leslie.md`).

### 11.2 Character

- **Phase-locked purity:** a sum of near-sines with no attack or decay. The organ is "on" the
  instant the contacts close, and off at release. There is no velocity.
- **Life comes from outside the tone:**
  - the click;
  - the percussion;
  - the scanner's square-ish ±20 c at 6.9 Hz (C3 adds the dry signal, giving a shimmering chorus);
  - the Leslie;
  - the overdrive (sum and difference tones of the 1.5f/3f twelfths and the 13 c sharp third make
    rough beating);
  - a faint shimmer at the wheels' rotation rates **[calc; design]**.
- **Jazz (Jimmy Smith, Jimmy McGriff, Larry Young) [memory]:**
  - upper 888000000 + percussion 3rd, fast decay, often soft;
  - vibrato off or C3;
  - lower 838000000 or 008530000 for comping;
  - pedals or left-hand bass;
  - the Leslie mostly on chorale, switched to fast for climaxes.
- **Rock (Jon Lord, Keith Emerson, Gregg Allman) [memory]:**
  - full drawbars or 888800000;
  - the Leslie fast, and the 122 amp and often an external guitar amp driven hard;
  - percussion often off;
  - glissandi and palm smears.
- **Gospel:** 888888888 or 888000000 with C3, the Leslie ramped often **[memory]**.

## 12. The user's files and recordings

### 12.1 MIDI [file]

| Set | Programs (1-based) | Files / tracks | Notes | Range (5-50-95 %) | Chords | CCs on organ channels |
|---|---|---|---|---|---|---|
| `~/Music/midi` (bushgrafts etc., 309 files) | GM 17 only | 1 file (*goodbaitGM*, Tadd Dameron's *Good Bait*, sequenced by Doug McKenzie), 2 tracks | 702 (melody, 52-81) + 1944 (comping, 41-75, 66 % chords, up to 7 voices) | 53-62-70 | 42 % median | CC7 82-101, CC10, CC11 = 127 once, CC91 = 26, RPN 0 (bend range); **no CC1, CC64**; 1 pitch bend |
| `~/midifiles` (5682 pop/rock files) | GM 17 | 269 files, 308 tracks | 125 k | 38-68-89 | 77 % median | CC11 in 90 tracks, CC1 in 63, CC64 in 45 |
| same | GM 18 | 218 files, 240 tracks | 94 k | 48-72-84 | 74 % | CC11 81, CC1 53, CC64 28 |
| same | GM 19 | 434 files, 499 tracks | 181 k | 51-74-88 | 61 % | CC11 184, CC1 114, CC64 70 |

- *Good Bait* also sets GM 18 on channel 2 and GM 17 on channel 3, with no notes there. Its bank
  select (CC0 = 0, CC32 = 126) and a "Kontakt 1" port name show it was made for a sampler.
  Velocities 1-127 (median 49-60) are used as dynamics. A Hammond would ignore them **[file]**.
- **CC1 on organ tracks is a continuous wheel.** On GM 19, for example: 2974 values in 1-63, 1447
  in 64-126, 286 at 0 and 44 at 127, so it is not a 0/127 switch. CC64 takes 2-6 distinct values
  (a switch). CC11 is the most used, as expression **[file]**.
- Notes outside the 61-key manual (MIDI 36-96) occur in the pop set (5 % below 38). They fold by
  octaves, or go to the pedals below 36 (§13.3) **[design]**.

### 12.2 Recordings [rec]

- No filename in `~/Music`, `~/Musici`, `~/Downloads` or `~/Videos` mentions Hammond, Leslie,
  Jimmy Smith, Booker T, Deep Purple, Procol Harum, McGriff, DeFrancesco, Larry Young or Lonnie
  Smith. The only "organ" files are the church-organ recordings used by `church_organ.md`
  **[rec]**.
- **The screen.** 26 likely tracks were split with Demucs `htdemucs_6s` (R&B, St Germain, Till
  Brönner, Jamie Cullum, Ray Charles, Love Ballads, Paul Simon, Lenny Kravitz and others). They
  were screened for:
  - organ-like sustained partials;
  - envelope modulation at 0.5-1.2 Hz and 4.5-8 Hz, at different rates below and above 800 Hz
    (horn ≠ drum);
  - partial FM.
- **Nothing passed [rec]:**
  - The 4.5-8 Hz envelope lines were tempo harmonics of the drum stem.
  - St Germain's *Sure Thing* has a 5.60 Hz line (5-8× the median, 20-365 s). But it is exactly
    7× the bar rate, the same in both bands to 0.01 Hz, and steady for six minutes: a rhythmic
    loop, not a Leslie.
  - Stems had 0-1 stable partials per 3 s window.
  - A partial-FM tracker, verified on a synthetic 6.7 Hz ±20 c tone (read 19.1 c), found only
    scattered vibrato and artefacts.
  - The screen is coarse: "not detected", not "absent".
- **So no rotation-rate, click, registration or sideband measurement was possible.** The numbers in
  §3-§9 are from the documents and calculations. §13.6 lists what a recording would let us fit.

## 13. Model plan [design]

### 13.1 Components

1. **Generator.** All 91 wheels run continuously and phase-locked from one master clock: angle
   θ_w = 2π·(20·a_w/b_w)·t·s(t), with 64-bit integer phase accumulation, so nothing drifts.
   - **Per wheel:** a **once-per-revolution table** of its pickup voltage (after the coil and
     filter). It is built at start-up from §4.6's rim-gap model: T teeth, the shape (sine; complex
     for 1-12), eccentricity, ovality, per-tooth errors, the flux law, and the filter.
   - **The filter is applied in the table** (exactly, as a Fourier-series multiply), since the
     wheel's signal is periodic in θ.
   - **Runtime:** read the table at θ_w with linear or cubic interpolation.
2. **Shared drive:** s(t) = 1 + 20 Hz shaft term + per-ratio driving-gear phase + 120 Hz motor term
   + slow mains wander. Wheels of one assembly share their driven-gear runout.
3. **Leakage:** a fixed mix matrix. The shaft partner and compartment go into each wheel's output.
   The whole-generator background goes into the busbars of each manual, before expression.
4. **Manuals:**
   - Per key, 9 contacts, each with a closure time, bounce and contact resistance (§6.2).
   - Busbars: per manual, 9 sums (the contact-gated wheels through resistance wires, with tapering
     and optional loading).
   - Presets: the B (and A) drawbar sets; fixed presets optional.
   - Drawbars: matching-transformer taps (§7.1).
5. **Pedals:** 25 keys (MIDI 24-48), complex wheels 1-12 and the next octave(s), 2 drawbars (16',
   8'), the L20/R network filter.
6. **Percussion:** the circuit of §8, as an envelope on the 4'/2⅔' busbar signal:
   - the C31 RC with slow/fast;
   - control-tube transfer (gain vs grid, with cut-off);
   - single trigger on the "any upper key" sense;
   - the 1' steal;
   - R1 level drop; soft/normal.
7. **Vibrato/chorus:** the 18-section LC line (WDF as in WDG16, or a cheaper chain of 18 first-order
   all-pass/low-pass sections fitted to its tap responses), a 9-tap capacitive crossfade at
   6.867 Hz, V1-3 / C1-3, the switch per manual.
8. **AO-28 preamp:** the linear filters as drawn (channel balance, expression network, tone
   control), the expression capacitor, and soft tube nonlinearity in V4b/V3. Output = the balanced
   console signal, in volts, to the Leslie (`leslie.md`) or a plain "tone cabinet" (a fixed
   speaker plus a room).
9. **Leslie:** a separate object (`leslie.md`), owned by the organ instance, one per channel
   carrying GM 17-19. The Leslie's mid/side output goes through `Tube::takeSide` as in
   `src/accordion.cc` and `src/organ.h`. Then the room (`src/room.*`).

### 13.2 Parameters (defaults)

| Group | Parameter | Default | Range |
|---|---|---|---|
| Generator | mains | 60 Hz (gear60) | 50 (gear50 table unknown: use gear60 at 1500 rpm × 0.8) |
| | wheel eccentricity e | half-normal, median 12 µm | 0-60 µm ("age") |
| | tooth position σ_x | 5 µm | 0-25 µm |
| | gap g0, flux exponent n | 0.35 mm, 1.5 | 0.2-0.5 mm, 1-2.5 |
| | wheel harmonics (13-43) | 2nd −35, 3rd −38 dB | |
| | complex wheels 1-12 | 3rd −12, 5th −22 dB | |
| | filter Q (49-91) | 4 | 2-8 |
| | cap age factor | 1.0 | 1-4 |
| | output level line | 12 → 21 mV p-p over 44-91 | |
| | shaft FM 20 Hz / motor 120 Hz | 0.02 % / 0.002 % | |
| Leakage | shaft partner / compartment / background | −45 / −55 / −70 dB per wheel | knob 0 to +15 dB |
| Contacts | closure scatter, bounces | 0.1-3 ms (by key speed), 0-3 | dirty-contact noise |
| Drawbars | taps | Hamre (0 … −32.1 dB) | or 3 dB/step |
| Percussion | τ slow / fast, re-arm | 1.55 / 0.375 s, 34 ms | soft −6 dB; fast +3.5 dB |
| Scanner | rate, line delay, V1/V2/V3 | 6.867 Hz, 0.8 ms, ⅓ / ½ / 1 line | C1-3 = V + dry |
| Preamp | drive, tone, expression range | mild, flat, ~35 dB | |

### 13.3 MIDI mapping

- **Notes:** MIDI 36-96 → upper manual keys 1-61. Below 36, the pedals (24-48 overlap; on GM
  tracks, notes below 36 go to the pedal division with 16'+8' at 8, 6). Above 96, fold down by
  octave. The **lower manual** is used by a second instrument instance or a split (open, §14)
  **[design]**.
- **Velocity: no effect on level** (faithful). It sets key speed, so the contact scatter and click
  (§6.2), and nothing else. A "velocity to level" option for GM files written with dynamics,
  *Good Bait* for example, is off by default (open, §14).
- **CC11 → expression pedal** (the capacitor's travel). **CC7 →** output gain after the preamp.
- **Leslie speed** (`leslie.md` §9): **CC1 ≥ 64 → fast, < 64 → slow** (a continuous wheel crossing
  the middle, as Hammond's own XK-series and the Leslie 2101 receive "Modulation"). **CC64** as an
  alternative switch: the Leslie 2101's factory default for its slow/fast control code is 64, the
  damper **[web, L2101]**. Use both, the last one wins.
- **CC12-20 or NRPN** for the drawbars (optional). **CC80-83** for percussion on, 2nd/3rd,
  fast/slow, soft/normal (optional) **[design]**.
- **Pitch bend: ignored** (a Hammond cannot bend). Players imitate bends by turning the motor off,
  which slows all wheels; an "organ stop/start" effect is optional.

| GM | Registration [design] | Percussion | Vibrato | Leslie default | Drive |
|---|---|---|---|---|---|
| 17 Drawbar Organ | upper 888000000 | off | C3 | slow (chorale) | mild |
| 18 Percussive Organ | upper 888000000 (1' stolen) | on, 3rd, fast, normal | off | slow | mild |
| 19 Rock Organ | upper 888800000 | off | off | fast | strong (Leslie amp driven hard) |

### 13.4 Costs [calc]

- **The generator:** 91 table reads per sample. One phase add, two loads and a lerp is ~1-2 ns
  each, so **~0.15 µs per sample = 7 ms of CPU per second at 48 kHz** for all wheels, always
  running.
- **Table memory:** N = 4096 points per revolution is enough for the 192-tooth wheel's first ~10
  harmonics up to 20 kHz (192 × 3 = 576 cycles per revolution needs N ≥ 1152). 91 × 4096 × 4 B =
  1.5 MB **[calc]**. For the low wheels a shorter table suffices.
- **Busbars:** per pressed key 9 contacts × (gain + add). A 10-note chord is 90 multiply-adds, plus
  the contact transients.
- **The network solve for loading:** a 9 × 9 per manual when the key set changes, or a cheap
  first-order approximation per wheel.
- **Leakage:** a fixed background signal (a constant mix of the 91 wheels, recomputed only when
  the drawbars or expression change), plus the partner term per used wheel.
- **Vibrato line:** 18 sections of 2nd-order WDF ≈ 36 states → ~0.1 µs per sample per channel.
  Two channels (vibrato and dry; the dry one is just a gain).
- **Preamp:** a few biquads and two waveshapers, under 0.05 µs.
- **Total organ without the Leslie: well under 1 % of one core.** The Leslie's cost is in its paper.

### 13.5 Build order

1. Generator tables plus the busbar/drawbar mix: get 888000000 right and in tune (§3.1 cents).
2. Contacts and click.
3. Percussion.
4. Scanner.
5. Preamp and expression.
6. Leakage and irregularities (the age knob).
7. The Leslie (`leslie.md`).
8. Pedals and the lower manual.
9. MIDI program table: GM 17/18/19 off `lead-calliope`.

### 13.6 Tests

- **Tuning:**
  - every wheel within 0.01 c of 20 × ratio × teeth, and its ET offset as in §3.1;
  - the top seven 1.25-1.98 c sharp;
  - mains-drift switch.
- **Phase lock:** two renders of one chord, started at different sample offsets, give identical
  steady-state waveforms after alignment to the master clock. Notes do not reset phase.
- **Sidebands:** with e = 40 µm on a 2-tooth wheel, a line at f/2 at the predicted level (§4.4,
  ±2 dB). With T = 128, PM sidebands at ±f_rot at the predicted level. The 20 Hz shaft line on all
  wheels at its set level.
- **Foldback:** the wheel per key and drawbar matches §7.2 (all 61 × 9).
- **Drawbar taps:** 0, −3, −6, −10.5, −15.2, −20, −26.1, −32.1 dB on a single wheel.
- **Percussion:**
  - slow and fast times to cut-off 0.79 and 0.19 s (±15 %);
  - no re-trigger while any upper key is held; re-arm in ~34 ms × 3 after the last release;
  - the 1' is silent with percussion on;
  - the upper manual drops (R1) unless soft.
- **Vibrato:** FM rate 6.867 Hz; V3 deviation ±18-26 c; near-square FM (check the 3rd harmonic of
  the FM); C3 shows the dry carrier plus the modulated copy; line passband ~7 kHz.
- **Click:** onset energy above 5 kHz, its spread scaling with velocity (key speed); a release click
  about 6 dB weaker.
- **Leakage:** with all drawbars in, the output is the background at its set level; a key on one
  wheel raises the partner wheel at −45 dB.
- **CPU:** 91 wheels plus a 10-note chord under 1 % of a core at 48 kHz.
- **Fit when a recording appears:**
  - the sideband levels around sustained notes (a long FFT, 0.1 Hz resolution, at f ± f_rot);
  - the leakage floor between notes;
  - the click's first 5 ms;
  - the percussion decay curve;
  - the scanner rate and depth (with the Leslie off).

## 14. Open questions for the user

1. **A recording**: can you supply (or point to) a dry Hammond (Leslie off or a direct line out) and
   one through a Leslie? Even a few held notes and chords would let us fit the sideband levels,
   leakage, click and percussion. None of your music has one (§12.2).
2. **Irregularity default**: a "new organ" (sidebands ~−35 dB, barely audible) or a "well-used B-3"
   (~−25 dB, the sub-octave lines of the low wheels audible)? Should "age" be a single knob?
3. **Drawbar taps**: Hamre's measured transformer steps or the folklore 3 dB?
4. **Velocity**: ignore it for level, as on a real Hammond (velocity only changes the click), or
   allow an option for files like *Good Bait* that use velocity as dynamics?
5. **Lower manual and pedals**: one MIDI channel = upper manual only, or a split (e.g. below MIDI 48
   to the lower manual with its own registration, below 36 to the pedals)?
6. **Leslie speed control**: CC1 ≥ 64 = fast and CC64 as a switch, as proposed (§13.3)?
7. **GM 17/18/19 registrations**: accept §13.3's table, or name settings (e.g. Jimmy Smith,
   Booker T's *Green Onions*, Jon Lord)?
8. **50 Hz organs**: worth modelling (a different gear table, not found), or 60 Hz only?
9. **The M-100 or A-100** as variants later, or only the B-3?

## 15. Sources

- **[HSM]** Hammond Organ Company, *Hammond Organ Service Manual: Models A, A-100, AB, BC, BCV, BV,
  B2, B3, C, CV, C2, C2-G, C3, D, DV, D100, E, G, GV, RT, RT2, RT3* (the generator, manual, vibrato,
  percussion and amplification chapters; Figure 24 "Schematic diagram of Hammond organ Model B-3 or
  C-3", p. 3-12). Read: the OCR text in full where relevant, and the Fig. 24 image directly.
  https://archive.org/details/HSM4952016 (text) and
  https://archive.org/details/HammondOrganServiceManual-EarlyModelsabcSeriesB3A-100RtDG (scan,
  PDF page 91 = p. 3-12)
- **[M100]** Hammond Organ Company, *Service Manual, Models M, M-2, M-3, M-100* (85/87 wheels,
  44-key manuals, drawbars, percussion). https://archive.org/details/HammondOrganServiceManual-ModelsMM2M3M-100
- **[B3W]** Keyboard Exchange International, "Hammond Organ Technical Information" (service-manual
  text: generator, vibrato, manuals, preamp). https://b3world.com/hammond-technical-information.html
  and pages -02 … -04
- **[WDG16]** K. J. Werner, W. R. Dunkel, F. Germain, "A Computational Model of the Hammond Organ
  Vibrato/Chorus using Wave Digital Filters", DAFx-16, Brno (Table 1: line values; delay 0.85 ms;
  passband edge 7075 Hz; scanner model). https://dafx16.vutbr.cz/dafxpapers/38-DAFx-16_paper_54-PN.pdf
- **[PPV11]** J. Pekonen, T. Pihlajamäki, V. Välimäki, "Computationally Efficient Hammond Organ
  Synthesis", DAFx-11, Paris (abstract read; full text not retrieved).
  https://dafx.de/paper-archive/details/Va-Hsjj4g396c-1d8ofv2w
- **[ED]** Electric Druid, "Hammond B3 technical" (gear ratios, teeth, foldback ranges, tuning).
  https://electricdruid.net/?p=316
- **[GV]** goodeveca.net, "Tone Wheel Spec" (the 91-wheel table, gearing, rotation rates).
  https://goodeveca.net/RotorOrgan/ToneWheelSpec.html
- **[SV-caps]** S. Vorkoetter, "Hammond Organ Tonewheel Generator Capacitor Replacement and
  Calibration" (cap values, drift 2.1-4×, 12-21 mV p-p calibration line).
  https://www.stefanv.com/electronics/hammond_tonewheel_capacitors.html
- **[SV-draw]** S. Vorkoetter, "The Science of Hammond Organ Drawbar Registration", with L. Hamre's
  comment (measured matching-transformer taps). https://www.stefanv.com/electronics/hammond_drawbar_science.html
- **[SV-AO29]** S. Vorkoetter, "Overhauling the AO-29 Amplifier in the Hammond M-100 Series"
  (listed for the M-series contrast; not read in full). https://www.stefanv.com/electronics/hammond_ao29_overhaul.html
- **[SBF]** setBfree man page (defaults: gear60/gear50 temperaments, crosstalk 0.01, percussion
  1 s / 4 s and 0.5012, click levels, scanner 7.25 Hz, whirl). https://mankier.com/1/setBfree
- **[EVB3]** Apple, Logic Pro "Using the EVB3 Model Parameters" (leakage, shaft crosstalk).
  https://help.apple.com/logicpro/mac/9.1.6/en/logicpro/instruments/chapter_7_section_9.html
- **[SOS1]** G. Reid, "Synthesizing Tonewheel Organs, Part 1", Sound on Sound (footages, 2.5 % 5th
  harmonic). https://www.soundonsound.com/techniques/synthesizing-tonewheel-organs-part-1
- **[JT30]** Usenet Q&A, "Hammond Tonewheel Organs" (pickup and tooth shape). https://jt30.com/usenet-articles/qanda/hammond-tonewheel-organs.html
- **[NI]** Native Instruments forum, modelling tonewheels (pickup smoothing, AM rate vs teeth).
  https://community.native-instruments.com/discussion/comment/9736 and
  **[NI2]** https://community.native-instruments.com/discussion/comment/177129
- **[DSPR]** DSP Robotics forum (wheels "out of center": FM and AM).
  https://dsprobotics.com/support/viewtopic.php?p=336373
- **[DJB]** D. J. Brown, "Hammond Scanner Measurements" (plate capacitance ~24.5 pF).
  https://mail.modularsynthesis.com/modules/DJB-scanner/Hammond%20Scanner%20Measurements.pdf
- **[US10199024]** "Modal processor effects inspired by Hammond tonewheel organs" (crosstalk
  settings). https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/10199024
- **[L2101]** Hammond-Suzuki, *Leslie 2101mk2 Owner's Manual* (CC64 as the default slow/fast code;
  rotor speeds and times are used in `leslie.md`). https://im.static-thomann.de/pics/atg/atgdata/document/manual/leslie_2101mk2.pdf
- **[ELT]** elektrotanya.com, "Hammond B-3 C-3 RT-3 AO-28 preamp" (DjVu, behind a download gate;
  not read). https://elektrotanya.com/hammond_b-3_c-3_rt-3_ao-28_preamp.djvu/download.html
- **[goffprof]** Goff Professional, B-3/C-3 technical service manual listing (B-3 = C-3 electrically).
  https://www.goffprof.com/collections/hammond-organ-parts/products/hammond-b-3-c-3-technical-service-manual
- **[Cherry]** Cherry Audio, "Drawbar Mixer" (3 dB per step convention). https://store.cherryaudio.com/modules/drawbar-mixer-black
- **[MPF]** Music Player forum, "Jimmy Smith organ settings" (registration lore; title only, not
  read). https://forums.musicplayer.com/topic/143631-jimmy-smith-organ-settings/page/2

## 16. Built (2026-11-17)

`src/hammond.cc`, `src/hammond.h` (the generator, manuals, percussion, scanner, preamp),
`src/hammondplan.cc` (the plan of a MIDI part), the Leslie in `src/leslie.cc` (`leslie.md` §15). §14
answered with the defaults:
- a used organ (sidebands ~−30 dB) with one age knob (`PJ_HAMMOND_AGE`);
- 3 dB per drawbar step;
- velocity only as the key's speed (the contacts' spread; the expression rider in `-play`);
- CC1 / CC64 for the Leslie;
- the GM 17/18/19 registrations of §13.3;
- the lower manual and pedals in one instrument, fed by the plan;
- 60 Hz gearing only.

**Simplifications from §13 [design]:**
- The wheel tables are built directly from the flaws' parameters (AM from eccentricity and
  ovality, PM and AM per tooth, a rotation-rate line, the harmonics), not from a rim-gap and flux
  model.
- The tuned filters scale the harmonics, not the sidebands (within ~3 % for T ≥ 32).
- The scanner is one interpolated delay line read along a triangle, not an 18-section LC line.
- The drawbar taps are 3 dB steps; there is no busbar loading.
- The preamp is a level, a shelf and a tanh.

**Measured:**
- A4 440.00 Hz, C6 −0.58 c;
- the A4 wheel's sidebands at ±27.5 Hz −30 dB; the 4-tooth C2 wheel's f/2 line −48 dB;
- a chord begun at two moments identical to −240 dB (phase-locked);
- the click 10-14 dB over the tone above 5 kHz, its waveform correlation between presses ≤ 0.44;
- percussion (3rd, fast) −20 dB in 171 ms, a legato note's ping 22 dB under a fresh one.

**Not yet:** the separate preamp and line-box values (Fig. 24 only); dirty contacts; 50 Hz
gearing; the A/B preset flip and drawbar CCs; slow-press for the percussion; machinery noises
(motor, belt, relay).

**Added 2026-11-17 (the user's requests):**
- Three tracks are an organist's three staves: the lowest the pedals (if its mean pitch is under
  A3 and it holds under 1.6 notes per onset), the other two the manuals.
- Every channel on one Hammond program is one organ.
- The Leslie:
  - the file's CC1 / CC64 / CC82 if present (CC82 0-42 stop, 43-85 slow, 86-127 fast, after the
    Leslie 2101's default; CC1 kept as two-way, ≥ 64 fast, since pop files use the wheel
    continuously and a centre "stop" would brake them);
  - otherwise planned: about 1 in 4 pieces (1 in 2 on the rock organ) fast in their strongest
    phrase or two, the rest slow; seeded by a hash of the notes.
- At the mics:
  - the Doppler −39..+44 c, the tremolo 15-16 dB (fast, C6);
  - the reflections' comb filtering: a chord's partials swinging 9-22 dB, each its own way
    (correlation 0.30).

**The organic imperfections (2026-11-17, the user's request: "more earthy and organic than a
perfect simulation") [design, from §3.1's couplings and §4's causes]:**

| Flaw | Physical cause | Model | Size |
|---|---|---|---|
| Torsional flutter | each wheel's spring coupling, a resonance excited by the motor's pulses and its own bearing | a 2nd-order resonator per wheel (3-8 Hz, Q 2-4), 0.6 common + 0.8 own noise, as a phase offset | σ 1.5e-4 rev: ~0.4 c rms, the same in cents on every wheel (a speed wobble), each wheel independent |
| Mains drift | the synchronous motor and the scanner follow the grid | an Ornstein-Uhlenbeck speed, τ 20 s | ±0.5 c |
| Breathing colour | the flux bends more where the gap is small | harmonic j's AM x (1 + 0.8 (j − 1)) of the eccentricity's | colour, not only level, once a revolution |
| Loading | the generator's source impedance against the busbars | a wheel / (1 + 0.03 (contacts − 1)), one sample late | a few % in big chords |
| Hum, ripple | the supply's mains harmonics in the preamp; B+ ripple | 60/120/180 Hz at 3e-4 of a wheel; 0.2 % AM at 120 Hz | idle floor ~−53 dB under a note |
| Dirty contacts | oxidised busbar contacts | a held contact breaks 0.1-2 ms at random | ~0.1 crackles a second under a held chord |

All but the crackles depend only on absolute time, so the phase lock holds (a chord begun at two
moments differs by −187 dB).

