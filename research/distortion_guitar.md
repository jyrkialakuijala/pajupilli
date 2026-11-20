# GM 31, Distortion Guitar: high gain on top of the built amp module

Status: research 2026-11-19; built the same day (README, "Electric guitars"): `-inst
distortion-guitar` with the `slo` preset (the stages as a list with shelves, dividers, the cold
clipper and the follower; 2× oversampling with first-order ADAA instead of 8×; the gate, with an
adaptive quick close the chugs needed; the noise floor; presence and resonance; the V30 cabinet,
fitted to FluidR3 rather than to Celestion's plot alone), the TS block (off by default), the GM-31
palm rule, power-chord octave and wide vibrato; all of section 11's checks pass in
`tools/test.py`, check 5 only as "more lines" (75 against 90, not 39 against 254). Not built: the
`rectifier`, `jcm800-boost` and pedal presets, drop tunings, pinch harmonics, the Greenback
refit, the option to drop thirds. It continues `research/electric_guitars.md`
(below "EG"), which covers the program list (EG §2.5), palm muting (EG §8), pinch harmonics (EG
§9.3), the amp module in general (EG §11), styles and presets (EG §12–13) and a listener's list
(EG §15). This document does not repeat those; it goes deeper where GM 31 needs more than GM 30's
`jcm800` preset. What is built and what this builds on: `src/guitar.*` (the Les Paul spec with the
hot 8 H bridge humbucker, the palm, the pick), `src/guitarinst.*` (the guitarist), `src/guitaramp.*`
(two triode stages, a tone stack after stage 1 or 2, a bright cap on the gain pot, a master, a
push-pull stage with sag, cabinet biquads, 4× oversampling with the elliptic filter of
`tools/oversample.py`, an optional compressor).

The marks are EG's:
- **[T]** read in the primary text (downloaded and read for this document); for the FluidR3
  measurements, "[T, sf2]" means measured here from the file itself.
- **[S]** secondary source, abstract, search snippet or a machine summary of a page.
- **[L]** player lore: players' or makers' descriptions, not measurements.
- **[E]** my estimate, fit, simulation or arithmetic from cited values.
- **[M]** general knowledge, not looked up.

References are short keys in brackets (section 13); EG's keys ([PY09], [YehThesis], [Mačák], [YS06],
[Koren96]) are listed in EG §19 and used here unchanged.

**Short answer.** GM 31 is the late-80s/90s high-gain rig: a hot bridge humbucker into a cascaded
preamp (four or five 12AX7 stages, one of them a "cold clipper", e.g. the Soldano SLO and its
descendants, the Mesa Dual Rectifier and the Peavey 5150) or a boosted Marshall, a master-volume
power stage that stays mostly clean, a closed 4×12 of Celestions, a close SM57, and a noise gate.
The Soldano's overdrive channel, read stage by stage: 36 dB, a gain pot (−20 dB at half), 32 dB, a
cold stage of only ~5 dB (39 kΩ unbypassed cathode, bias −4 V, 0.1 mA, a 1.6 kHz plate roll-off),
then a 37.8 dB stage that clips first, into a DC-coupled cathode follower; at half gain 3.4 mV peak
at the jack already overdrives the last stage, 0.34 mV at full gain **[T, Kuehnel/ampbooks]**, so a
humbucker's 0.5 V peak is 40–60 dB over the edge. "Tight" comes from what is kept out of the
clipping stages: small coupling capacitors (2.2 nF: 72 Hz into 1 MΩ, 154 Hz into 470 kΩ **[E]**),
small or partial cathode bypasses (bass 3–5 dB under treble below 90–170 Hz **[T, Kuehnel]**), and a
Tube Screamer in front at drive 0 / level high (its gain is 1 + R2/4.7 kΩ only above 720 Hz
**[T, YehThesis]**). Speakers: Celestion's own plots of the Vintage 30 and Greenback both show a
double peak at 2.3 and 3.1–3.2 kHz (+5 dB over 1 kHz), a dip between (2.6–2.7 kHz), and a fall of
~15 dB from 4 to 6.5 kHz **[T, Celestion plots]**; FluidR3's distortion-guitar samples show the same
fingerprint (a 3.2 kHz peak +6 dB over 1 kHz, −10 dB at 5 kHz, −26 dB at 8 kHz, −38 dB at 10 kHz)
**[T, sf2]**. Under distortion a power chord stays a handful of spectral lines (24 → 39 peaks below 1.5
kHz in a tanh simulation) while a major triad explodes into 254 and a minor one into 313 **[E,
simulation]**; listeners rated distorted power chords 6.06/10, major 5.61, minor 3.89 **[T, Herbst]**.
Aliasing: a hard clipper at gain 10 needs 12× oversampling for the aliasing that first-order
antiderivative antialiasing (ADAA) reaches at 4×, second order at 3× **[T, PZL16]**. Build: generalise
the two stages into a list of up to five, add the cold-clipper and cathode-follower forms, a TS
pedal, a noise gate keyed from the guitar, resonance and presence on the power stage, a V30 4×12 +
SM57 cabinet, 8× (or 4× + ADAA); the guitarist gets a GM 31 palm-mute rule and drop tuning.

---

## 1. What GM 31 stands for, and what the sound sets do

- **The rig the name points to** **[L]**: by 1991 (GM's year) "distortion" meant the
  preamp-distortion amps of the 80s: Soldano SLO-100 (1987), Mesa/Boogie Mark series and later the
  Dual Rectifier (1989–91), modded Marshalls (an extra gain stage in a JCM800), the Marshall JCM900
  (1990) with diode clipping, the Peavey 5150 (1992, after GM). Played through a closed 4×12 and
  close-miked. A distortion pedal (DS-1, RAT) into a clean-ish amp is the cheap version of the same.
  The Roland list (EG §2.5) has "Dist.Gt.", "Power Guitar" and "Feedback Gt."; the XG list has
  feedback variations: feedback belongs to the program **[T, EG]**.
- **The Roland and Yamaha sets:** their samples cannot be read here; the SC-88Pro's insertion
  effects include "Distortion" with four amp-simulator types (Small, BltIn, 2-Stk, 3-Stk) **[T,
  SC-88Pro manual, via EG]**: the vendor's own model of the rig is "a stack".
- **FluidR3's preset "Distortion Guitar"** (bank 0, program 30 counting from 0) read from the file
  **[T, sf2]**:
  - One instrument, nine velocity zones that differ only in release time (velocity 0–64: −1586
    timecents more release; 121–127: +316): velocity changes nothing but loudness and release.
  - Ten samples at 30 kHz, one per 3–6 semitones, roots E2 A2 D3 G3 B3 E4 A4 B4 E5 G5; the E2
    sample is 11.2 s, the rest 1.6–4.1 s, each looped only over its last few ms.
  - Two layers of the same samples, mixed: one through a low-pass at 8080 absolute cents (870 Hz)
    raised by the modulation envelope by 2468 cents to 10548 cents (**3.6 kHz**, no resonance); the
    other at 11108 + 2468 cents (20.8 kHz: effectively open) with Q 7 dB. (EG §2.5's "an 8 kHz
    low-pass" read the 8080-cent number as Hz; the dark layer is at 3.6 kHz.)
  - Volume envelope: hold 1586 timecents (**2.5 s**), then a decay towards −41.8 dB at 1.9 dB/s on
    the E2 zone to 6.8 dB/s on the top zone (decay times 6874 → 4665 timecents per 100 dB); release
    ~0.3 s at mid velocity. A 2.8 Hz LFO on volume, depth 0.7 dB. Chorus send 2 %, reverb 10 %.
- **What a TiMidity render sounds like** (reverb and chorus off, 48 kHz, E2…E5 at velocity 40, 80
  and 127, 4 s held) **[T, sf2 rendered]**:

| Note | Peak (dBFS) at v40 / v80 / v127 | Level at 1 s / 2 s / 3.5 s (re peak) | Centroid 0.1–1.1 s | Harmonic/noise (dB) |
|---|---|---|---|---|
| E2 | −31.6 / −21.6 / −14.9 | −1.8 / −1.6 / −5…−8 (peak at **1.9 s**) | 490 Hz | 6.6 |
| A2 | −33.6 / −23.6 / −17.0 | −2.4 / −3.6 / −7…−9 | 740 Hz | 6.9 |
| E3 | −37.1 / −25.7 / −19.0 | −2.5 / −3.4 / −6…−10 (peak at 1.6 s) | 990 Hz | 13.5 |
| A3 | −35.1 / −25.1 / −18.5 | −1.8 / −1.9 / −7…−9 | 1.8 kHz | 12.7 |
| E4 | −32.4 / −22.4 / −15.7 | −5.2 / −5.7 / −12…−14 | 2.1 kHz | 44 |
| A4 | −34.4 / −24.4 / −17.7 | −3.9 / −6.0 / −17…−19 | 1.8 kHz | 40 |
| E5 | −29.4 / −19.3 / −12.7 | −6.4 / −5.7 / −11…−13 | 3.0 kHz | 46 |

  - Velocity 40 → 127 is **16.7 dB** of level at every pitch and **no** change of spectrum (the
    centroid identical to 1 Hz). A real high-gain amp compresses most of those 17 dB away and
    changes the attack instead (section 9).
  - The held note does not decay for 2–2.5 s (the recording, plus the 2.5 s hold), then falls
    5–10 dB by 3.5 s; the low notes *grow*: E2 peaks at 1.9 s, E3 at 1.6 s.
  - The low notes are noisy between their harmonics (harmonic-to-rest 6.6–7 dB on E2, A2; 13 dB
    on E3–A3; 40–46 dB from E4 up): weak lines 8–15 Hz beside the low harmonics (other strings
    ringing along, or the two polarisations: **[E]**) plus broadband hiss.
- **The raw samples** (read straight from the file, without TiMidity's filters) **[T, sf2]**:
  - **Spectral envelope**, pooled over seven samples (E2–E4), each harmonic's level from 0.1 to
    1.5 s, normalised at 0.6–1.5 kHz, median per third octave:

| Hz | 200 | 315 | 500 | 800 | 1000 | 1250 | 1600 | 2000 | 2500 | 3150 | 4000 | 5000 | 6300 | 8000 | 10000 | 12500 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| dB | +10 | +12 | +10 | +5.5 | −0.5 | 0 | −2 | −4 | +3 | **+6** | −1 | −10 | −14 | −26 | −38 | −53 |

    A dip at 1.6–2 kHz, a presence peak at 3.2 kHz, and above 5 kHz a fall of about −30 to −40
    dB/octave (fitted slopes 3–12 kHz: −31 dB/oct on E2, −28 on A2, −7 to −24 on higher samples
    whose harmonics are sparse). This is the speaker's fingerprint (section 5). Jnsgm2's distortion
    guitar (22 kHz samples) has no 3.2 kHz peak and is darker (−10 at 2.5 kHz, −15 at 5 kHz, −30 at
    6.3 kHz, −47 at 8 kHz) **[T, sf2]**: the sets agree on the fall above 5 kHz, not on the presence.
  - **The E2 sample over 11 s:** the 2nd harmonic rises from −9 dB (0–0.5 s) to 0 dB at 2 s and
    stays strongest to 2.5 s, then sinks (−17 dB at 7.5 s, −27 dB at 10 s) while the fundamental
    climbs back to the top (−3 dB at 6.5–7.5 s); the overall level falls only 7 dB in 11 s. A held
    low note on a high-gain amp is not a decay but a slow exchange between its partials: the
    "bloom" (and, at stage volume, feedback: section 8.6). The A2 sample's fundamental swings
    between −2 and −19 dB on a 1–2 s period (beating) **[T, sf2]**.
  - Pitch is steady (±1–5 cents, no finger vibrato in any sample) **[T, sf2]**.
- **Chords in FluidR3** add as samples do: a power chord E2-B2-E3 is 3.8 dB louder than E2 alone, a
  4-note major chord 5.2 dB **[T, sf2 rendered]**, with no intermodulation at all (each note is its
  own recording). A real amp compresses a chord to about the level of a single note and adds the
  intermodulation products of section 8.1 **[E]**.

---

## 2. High-gain preamps: topologies and values

### 2.1 The Soldano SLO-100 overdrive channel (read stage by stage)

All values **[T, Kuehnel/ampbooks]** unless marked.

| Stage | Plate R / supply | Cathode | Bias, current | Gain (grid to grid) | Filters | Headroom (peak) |
|---|---|---|---|---|---|---|
| Input | 68 k grid stopper, 1 M grid leak | | | 0 dB | | |
| V1a | 220 k / 360 V | 1.8 k, 0.002 µF partial bypass (shelf at 169 Hz; bass 3.2 dB under treble) | −1.5 V, 0.83 mA | 36 dB (34.2 at 82 Hz, 37 at 10 kHz) | optional 1000 pF bright cap on the gain pot | 1.5 V |
| Gain pot | 470 k series, 500 k pot | | | at 82 Hz 0.52 (−5.6 dB) with 2000 pF across the 470 k (treble passes whole); at half gain ~−20 dB | | |
| Coupling | 0.02 µF | | | | 2 dB down at 10 Hz | |
| V1b | 100 k / 350 V | 1.8 k, partial bypass (bass 4.7 dB under treble) | −2 V, 1.1 mA | 32 dB (27.5 at 82 Hz, 32.3 at 10 kHz) | divider 470 k / 1 M: 0.68 (−3.3 dB) | 2 V |
| V2a, **cold clipper** | 100 k / 350 V | **39 k unbypassed** | **−4 V, 0.1 mA** | ~5 dB (5.4 at 82 Hz, −15 dB at 10 kHz) | 1000 pF from plate: corner 1.6 kHz; 220 k grid stopper, 330 k grid leak; output impedance 98 k | 4.2 V |
| V2b | like V1a (~37.8 dB) | | | 37.8 dB | | 1.5 V: **clips first** |
| Cathode follower | DC-coupled to V2b's plate | | | −0.1 dB, 615 Ω out | | |
| Send | 100 k / 1 k divider | | | −40 dB | | |
| V3 (return) | 220 k / 378 V | 2.2 k fully bypassed | −1.75 V | | stays clean (needs +4.2 dBV, has +1.9) | |
| Tone stack | Bassman-type, after the return's cathode follower, before the phase inverter | | | | values not given | |

- **Sensitivity:** with the gain at half, 3.4 mV peak (−52.5 dBV) at the jack overdrives V2b; at
  full gain 340 µV (−72.5 dBV) **[T]**. A hot humbucker's 0.5 V peak (EG; README: "a single note
  ~0.5 V, a hard power chord 1.3 V") is then **43 dB over** at half gain, 63 dB at full **[E]**.
- So the distortion is made mostly in **one** stage (V2b) plus the cathode follower it drives, after
  three stages that amplify, shape and pre-clip. The cold clipper clips its lower half at a few
  volts; V2b clips both ways. The DC-coupled follower adds its own soft compression when its grid
  is pulled above its cathode ("a pleasing compression effect", a large part of the Soldano sound
  **[S/L, Blais via Robinette]**).
- **Where it filters:** the bright caps and the 1.6 kHz plate capacitor of the cold stage cut the
  fizz *before* the last clipping stage; partial cathode bypasses give 3–5 dB less bass than treble
  in the first two stages **[T]**. The tone stack comes *after* all clipping: it shapes, it does not
  decide what clips **[T]**.

### 2.2 The others, less exactly

- **Marshall JCM800 2203/2204** (EG §11, the built `jcm800`): four triode sections: V1a, the gain
  pot with its 470 pF bright cap, V1b cold (10 k unbypassed), V2a, then the DC-coupled cathode
  follower into the Marshall stack; coupling capacitors 0.022 µF, 470 k grid leaks (faster recovery
  of the coupling capacitors) **[S, Robinette/Blais]**. A **modded / "hot-rodded" JCM800** adds a
  gain stage in front (a booster: the Soldano's V1a), so the cold stage then gets 30–40 dB more;
  Soldano's 39 k cold stage replaced Marshall's 10 k because 10 k was "a bit too much" once a stage
  was added **[S, Blais via Robinette]**. Builders' first-stage coupling cap for high gain: **2.2–4.7
  nF** **[S, Blais via Robinette]**; JCM800 lead: 0.0022 µF on one coupling **[S]**.
- **Mesa/Boogie Dual Rectifier:** widely described as the Soldano circuit with the 39 k cold stage,
  the effects loop moved after the tone stack, an AC-coupled cathode follower, **no global negative
  feedback** (the presence control replaced by a treble-cut) **[S, Robinette]**; channel modes
  "Vintage"/"Modern"; 6L6 power tubes, a switchable bias for EL34; a switch between valve and
  silicon rectifiers (the valve one sags more) **[S/L]**. One reported Rectifier-family stage: 200 k
  plate, 1.8 k cathode, 394 V **[S, forum]**. No verified stage table was found (section 12).
- **Peavey 5150 / 6505 lead channel:** players count six cascaded triode sections before the
  tone stack and two more before the power tubes **[S, forum]**; a 39 k cold stage **[S]**; 220 k
  plates; coupling caps mostly 0.022 µF with some small ones (a "C17" of 2.2 nF ∥ 470 k in the 5150,
  470 pF in the 5150 II **[S]**); a **plate-driven** tone stack (no cathode follower in front: more
  loss, a different curve) after an extra booster stage **[S, Robinette]**; a "resonance" and a
  "presence" on the power stage (section 6). Known among players as the tight one, the Rectifier as
  the loose, thick one **[L]**.
- **Marshall JCM900 (4100, 2100) and later DSL/TSL:** the JCM900's high-gain channel adds diode
  clipping (a pair of silicon diodes) after tube stages, in some versions only switched in at high
  gain **[S, forum]**; the JCM900 SL-X and later DSL/TSL turn to Soldano-style cascades **[S/L]**.
  No schematic read.
- **Peavey VTM (the switches that show the levers)** **[T, Kuehnel/ampbooks]**: a cathode resistor
  of 6.8 k (cold) or 6.8 k ∥ 4.7 k = 2.8 k; a "gain 2" switch that bypasses part of it at high
  frequencies only (1 k with 0.68 µF: treble gain up, bias unchanged); the first stage's bypass
  0.68 / 1.68 / 11.68 µF across 2.7 k (more capacitance: the bass shelf moves down, more bass into
  the stages): corners **87, 35, 5 Hz** **[E: 1/(2π·2.7k·C)]**; a "comp" pair of 24 V zeners at the
  cathode follower; a "mid" switch adding 1000 pF to the stack's 470 pF; presence switches adding
  0.1 µF steps; the power stage's gain from the phase inverter to the feedback tap 18.7 dB.

### 2.3 Tight and loose: what to keep out of the clipping stages

| Lever | Tight (modern metal: 5150, boosted Rectifier, Soldano with a TS) | Loose (classic, saggy: Rectifier "vintage", JCM800 without a boost) | Values |
|---|---|---|---|
| Coupling caps into clipping stages | small | large | 2.2 nF into 1 M → 72 Hz; into 470 k → 154 Hz; 1 nF into 1 M → 159 Hz; 22 nF into 1 M → 7.2 Hz **[E]** |
| Cathode bypass of the gain stages | small or partial (bass 3–5 dB under treble below 90–170 Hz) | large (25 µF: full bass gain to ~4 Hz) | 1.8 k ∥ 1 µF → 88 Hz; 1.8 k ∥ 0.68 µF → 130 Hz; 2.7 k ∥ 0.68 µF → 87 Hz **[E]** |
| A pedal in front | TS at drive 0, level high: below 720 Hz only unity gain | none | section 3 |
| Blocking | rarely: small caps recover fast | big caps charge on grid current, the bias slides cold, the note "farts out" after a hard attack | **[S, Robinette]**; EG §11.2 |
| Power rectifier | silicon: little sag | valve: sag, bloom | **[L]** |
| Bass restored after clipping | tone stack bass, "resonance" on the power stage, the closed 4×12 | | sections 5–6 |

- **Why it works** **[E, from the circuit]**: a stage that clips a signal with a lot of low E (82
  Hz) and its 2nd harmonic in it spends its headroom on the bass: a low note's waveform rides on a
  slow swing, the clipping point moves with it (the treble is gated on and off at 82 Hz: the
  "farty", flubby bass), and intermodulation between the bass and everything else fills the
  low-mids (the "mush"). Cut the bass before clipping and add it back after: the clipping becomes
  symmetric and steady, and the palm-muted low note becomes a short, defined "chug" whose bass comes
  from the linear filters after the clipper. Players' rule: "crank Treble, cut Bass [before the
  distortion] for a very tight aggressive sound; boosting lows later … will not muddy up the
  distortion" **[S/L, Blais via Robinette]**.
- **Unbypassed cold clipper:** "don't bypass a cold clipper: [it] will clip too early and too
  severely and sound fizzy" **[S/L, Blais via Robinette]**. The stage after a cold clipper is biased
  warm (820 Ω) to keep the other half clean **[S]**.

---

## 3. Pedals in front: the Tube Screamer as a boost, the DS-1 and RAT

### 3.1 Tube Screamer (TS808/TS9) into a high-gain amp

- The circuit and values are in EG §11.8 **[T, YehThesis]**: the clipping op amp's gain is 1 +
  (51 k + D·500 k)/4.7 k above **720 Hz** (the 4.7 k / 0.047 µF leg to ground: 1/(2π·4.7k·47n) =
  720 Hz), falling to unity below; 51 pF across the feedback gives a low-pass from 61 kHz (D = 0) to
  5.7 kHz (D = 1); two 1N914 antiparallel in the feedback; the output is the input plus the clipped
  difference; then a 2nd-order tone low-pass and a level control.
- **As a boost** **[L, consistent across Reverb.com, forum posts]**: drive at or near 0, level near
  max, tone at noon or a little above; the amp's own gain then lowered a little. At D = 0 the pedal
  adds +21.5 dB above 720 Hz to the clean path, so above ~720 Hz the amp sees up to ~12× more, below
  ~100 Hz about 1× (a first-order high-pass in the *added* part only: the clean part passes) **[E]**.
  The diodes clip only the excess, so at drive 0 it adds a soft edge, not much distortion.
- **What it does to a palm-muted chug** **[E]**: the chug's energy is mostly at f₁–2f₁ (82–165 Hz on
  the low E; EG §8: the palm takes the high partials first), so the TS leaves it at the amp's level
  while lifting the pick attack and the 1–3 kHz partials by ~15–20 dB into the clipping stages: the
  attack drives the amp to full distortion, the sustained bass under it does not swamp the stages,
  and the clipping recovers between chugs. Players' word: "tightens the low end", "adds focus".
- **An EQ instead:** some producers use a high-pass at ~100 Hz and a dip near 550 Hz before the amp
  instead of a TS **[S, one producer]**.

### 3.2 Distortion pedals instead of a high-gain amp

- **Boss DS-1:** values in EG §11.8 **[T, YehThesis]**. Its diodes go to ground after a 2.2 k /
  0.01 µF RC (7.2 kHz) and its tone control blends a 320 Hz low-pass with a 1.16 kHz high-pass:
  a scooped curve.
- **ProCo RAT** (electrosmash still unreachable: DNS failure on 2026-11-19): an LM308 op-amp
  stage whose gain is set by a 100 k pot against two legs to ground, **560 Ω + 4.7 µF** and **47 Ω
  + 2.2 µF** **[S, forum, search]**: corners 1/(2π·560·4.7µ) = **60 Hz** and 1/(2π·47·2.2µ) =
  **1.54 kHz** **[E]** (matching the "high-pass filters at 60 Hz and 1.5 kHz" in electrosmash's
  summary **[S]**): the low leg sets the gain below 1.5 kHz, both legs above: the RAT, too, adds more
  gain above ~1.5 kHz than below. Theoretical maximum gain ~67 dB, limited in practice by the
  LM308's slew rate (~0.3 V/µs **[M]**) and gain–bandwidth: at high gain the op amp cannot follow
  the highs: a built-in, level-dependent low-pass **[S]**; 100 pF across the feedback **[S]**; then
  1N914 (Rat 2: 1N4148) to ground (hard), a "filter" low-pass, a JFET output buffer **[S]**.
- **For GM 31:** a pedal into a clean-ish amp is a second voice (DS-1/RAT + JCM800 at low gain).
  The amp-made preset is the main one (section 10).

---

## 4. The noise gate

- **Why high gain needs one** **[E, arithmetic]**: at 40–60 dB over the clipping point (2.1), any
  input above ~1–3 mV is already distorted to full level, and anything above ~10 µV is clearly
  audible. Sources at the input: the hot humbucker's 13.5 kΩ winding gives thermal noise √(4kTRB) ≈
  2.1 µV rms over 20 kHz; the 68 k grid stopper 4.7 µV; a 12AX7's own ~1–2 µV **[M]**; hum picked
  up by the coils: a humbucker cancels most (−20 to −30 dB against a single coil **[M]**), but a
  few tens of µV remain at 50/60 Hz and its odd harmonics; the strings themselves: the open strings
  ringing along, the hand's noise, pick scrape. Amplified by 60–80 dB, ~5–10 µV becomes 5–100 mV
  at the stage that clips at 1.5 V: −20 to −50 dB under the full distorted level, and the distortion
  compresses it upwards. Without a gate, every rest is filled with hiss and hum, and every chug's
  tail rings into the next.
- **The pedals:**
  - **Boss NS-2:** threshold and decay knobs, a "reduction/mute" switch, an expander below the
    threshold; −20 dBu nominal input, 1 MΩ input, 1 kΩ output **[S, manual via search]**. Its
    "decay" sets how long the sound takes to fade after the input falls below the threshold **[S,
    Boss]**. No ranges in milliseconds were found.
  - **ISP Decimator / Decimator II "G-string":** the detector listens to the guitar directly (a
    separate input before any distortion), the gate acts on the amp's effects loop (after the
    preamp): it opens on the clean, fast pick attack and closes on the guitar's own level, not on
    the noise the preamp made **[L/M]**. Single threshold knob; the release is adaptive ("time vector
    processing") **[L]**.
- **Settings** (all **[L/S, metal producers' guides]**): attack 0.1–1 ms (a slow attack softens
  the pick); hold 5–20 ms for fast riffs, up to 50 ms; release 50–150 ms (too fast chops the note's
  decay, too slow lets noise through); range full (−80 dB or more) for tight rhythm; hysteresis (a
  closing threshold below the opening one) instead of a long hold to prevent chatter; threshold set
  by ear just above the floor, no standard number; one example −65 dB at 20:1 **[L]**. Too high a
  threshold "loses gain" (it eats the quiet end of the sustain) **[L]**.
- **Model** **[E]**: an envelope follower on the guitar's EMF (before the amp: clean, its dynamic
  range is the playing's), peak-held, opening at T_open, closing at T_open − 6 dB, attack 0.5 ms,
  hold 15 ms, release 80 ms (exponential, in dB), range −80 dB; applied after the preamp, before
  the power stage (the Decimator's loop position) so that the power stage's and cabinet's own
  tails decay naturally. T_open ≈ 2 mV EMF for a humbucker giving ~0.5 V peaks (−48 dB) **[E]**. The
  model also needs a noise floor to gate (section 10.6): otherwise the gate only shortens tails.

---

## 5. Speakers, cabinet and microphone

### 5.1 The speakers: Celestion's measured curves

Read by eye from Celestion's published response plots (on-axis SPL, 1 W, the maker's test baffle;
±1 dB reading error) **[T, Celestion plots]**:

| Hz | 50 | 100 | 200 | 350 | 500 | 1000 | 1300 | 1600 | 2300 | 2650 | 3150 | 4000 | 5000 | 6000 | 6500 | 8000 | 10000 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| Vintage 30 (dB SPL) | 87.5 | 94.5 | 98.5 | 98 | 100.5 | 101 | 98.5 | 101 | **106** | 102.5 | **105.5** | 101.5 | 95 | 86 | 83 | 78 | 80 |
| G12M Greenback | 87 | 95.5 | 97.5 | 95.5 | 98.5 | 98.5 | 97 | **95.5** | **105.5** | 100.5 | **107** | 100.5 | 95 | 86 | 78.5 | 79 | 80 |

- **Both:** a double presence peak at 2.3 and 3.1–3.2 kHz with a 3.5–5 dB dip at 2.6–2.7 kHz
  between, and a fall of 15–20 dB from 4 to 6.5 kHz (≈ −25 to −30 dB/octave), a floor of break-up
  peaks around 75–80 dB above 7 kHz. Below 100 Hz both fall ~12 dB/octave in the baffle.
- **Where they differ:** the Greenback has a dip at **1.6 kHz** (−3 dB under 1 kHz, 10 dB under its
  2.3 kHz peak) and a lower-mid dip at 350 Hz, its 3.2 kHz peak is 1.5 dB higher and its fall to 6.5
  kHz steeper: more "honk" and bite with a hollow in the upper mids. The V30's 500 Hz–1.6 kHz is
  flatter and 2 dB higher relative to its low end: the forward "mid" that players hear in it **[T
  plots; L the words]**. Specs: both resonate at **75 Hz**; V30 70 Hz–5 kHz, 100 dB; Greenback 75
  Hz–5 kHz, 98 dB **[T, Celestion]**.
- **FluidR3's samples** (1) have the 3.2 kHz peak and a dip at 1.6–2 kHz: either speaker would fit;
  the 1.6–2 kHz dip (−2 to −4 dB re 1 kHz) is closer to the Greenback's.

### 5.2 The closed 4×12

- **Closed back:** the box's air stiffens the cone: the resonance rises from the driver's 75 Hz to
  about 90–120 Hz with a bump, and below it the response falls at 12 dB/octave (a sealed box); no
  back wave, so none of the open-back's bass cancellation **[M/E]**. Players: "a tighter low end and,
  often, subjectively less treble" **[S, SOS]**. Thiele-Small parameters for the V30 beyond Fs (75 Hz)
  and Re (7.3 Ω) were not found, so the box's Q and bump cannot be computed here (section 12).
- **Four speakers:** in the near field one speaker dominates the close mic; the others add at a few
  ms of delay at lower level: a mild comb in the room mic, not in the close one **[M]**.
- **Cone break-up** dominates the sound above ~500 Hz and starts at "a few microvolts" of drive:
  it is linear and part of the speaker's fixed response **[S, SOS]**: a filter is right.

### 5.3 The SM57

- Its published response: flat-ish 200 Hz–2 kHz, a presence rise of ~+4 to +6 dB at 4–6 kHz, a
  fall above ~10 kHz, a bass roll-off below ~200 Hz that the proximity effect fills in at 2–5 cm
  (+6 to +10 dB near 100 Hz) **[M]**.
- **Position:** at 2.5 cm on the cone's centre: "sharp attack, emphasized bass"; at the edge: a
  different, darker high end **[S, Shure via search]**; brighter towards the centre, darker towards
  the edge **[S, Sweetwater, Thomann]**. No measured dB difference was found; EG §11.5 estimated
  5–10 dB above 2 kHz **[M]**. The usual metal placement: on-axis, just off the dust cap, close
  **[L]**; Herbst's recordings: SM57 "slightly off-centre, on-axis, close", Marshall 1960 4×12 with
  V30s **[T, Herbst]**.

### 5.4 Biquads for a "V30 closed 4×12, SM57 off-cap" [E]

Fitted by hand to 5.1 plus 5.2–5.3 (target: the V30 table, the closed-box bump and the 57's rise
cancelling some of the speaker's fall, then checked against FluidR3's pooled envelope in 1):

| # | Type | f (Hz) | Q | Gain (dB) | Why |
|---|---|---|---|---|---|
| 1 | high-pass | 85 | 0.9 | | closed box below its resonance (12 dB/oct) |
| 2 | peak | 110 | 1.2 | +3 | the box's bump, with proximity |
| 3 | peak | 350 | 1.5 | −1.5 | (V30: −0.5 dB; Greenback: −2.5) |
| 4 | peak | 1300 | 3 | −2.5 | the V30's small dip (Greenback: 1600 Hz, −4.5, Q 2) |
| 5 | peak | 2300 | 4 | +5 | presence peak 1 |
| 6 | peak | 3150 | 3.5 | +5 | presence peak 2 (the dip between follows) |
| 7 | low-pass | 4600 | 1.1 | | the fall: −10 dB at 5.5 kHz with #8 |
| 8 | low-pass | 6000 | 0.7 | | the steep part (together ≈ −24 dB/oct, −30 at 8 kHz) |
| 9 | peak | 9500 | 4 | +4 | a break-up floor (optional) |

The built Greenback cabinet (`cab_` for `cabinet == 1`: 72 Hz HP, +2 dB at 120, +4 at 1.8 kHz, +3
at 3.3 kHz, −7 at 4.2 kHz, low-passes at 4.8 and 6.2 kHz) has its presence at 1.8 kHz where
Celestion's plot has a dip at 1.6 kHz and only one peak at 3.3 kHz: worth re-fitting to the
table above while GM 31 is built (section 10.5).

---

## 6. The power amp at high gain

- **Master-volume amps** (all the high-gain amps of section 2) make their distortion in the preamp;
  the master sets how hard the power stage is driven. At home and in the studio it sits low: the
  power stage nearly clean, its contribution sag and soft compression on chord attacks; on stage
  higher: power-stage distortion and the speaker's own compression join in **[L]**. GM 31's preset:
  power stage at the edge, like `jcm800`'s 0.6 master, not saturated **[E]**.
- **EL34 vs 6L6** **[L]**: EL34 (Marshall, Soldano option): earlier, softer break-up with a mid
  bark; 6L6/5881 (Mesa Rectifier, 5150, Soldano standard): more headroom, bigger low end, "scooped"
  and tighter bass. As a model: the 6L6 stage clips later and more symmetrically; the EL34 earlier
  with a little more 3rd harmonic in the upper mids **[E]**. Koren's 6L6CG pentode constants are in
  EG §11.2 **[T, Koren96]**. JCM800 2204 numbers: EL34s at ~460 V plate, −40 V grid bias, 40 mA idle,
  3.2 kΩ plate-to-plate, ~60 V sag at full power, 64 W RMS before transformer loss, feedback 100 k
  from the 4 Ω tap ("about a factor of three" less feedback than the 47 k of the 1987) **[T,
  Kuehnel/ampbooks]**.
- **Negative feedback, presence, resonance:**
  - Global feedback from the speaker tap to the phase inverter flattens the power stage and lowers
    its output impedance; the amount varies: JTM45 about three times the Bassman's **[S, Robinette]**;
    the Peavey VTM's phase-inverter-to-tap gain 18.7 dB with presence at minimum **[T, Kuehnel]**;
    the Dual Rectifier has none **[S]**. Typical guitar amps: 6–12 dB of feedback **[M]**.
  - **Presence:** a capacitor (Marshall: 0.1 µF with a 5 k pot; Peavey: 0.1–0.3 µF) shunts the
    feedback divider at high frequencies, so above its corner the feedback falls and the gain rises,
    by up to the feedback amount: a shelf of +3 to +10 dB from ~2–5 kHz **[T values Kuehnel; E
    arithmetic: 1/(2π·4.7k·0.1µ) = 340 Hz for the cap's own corner, the shelf's audible part higher
    because it rides the speaker's rising impedance]**.
  - **Resonance / depth:** a capacitor in series with the feedback resistor (commonly a 1 M pot with
    **4.7 nF**, some 10–15 nF) removes the low frequencies from the feedback: the power stage's
    output then follows the speaker's impedance peak at the cabinet's resonance (90–120 Hz in a
    closed 4×12) **[S, forum]**: a bump of +3 to +8 dB at ~100 Hz, more low-frequency distortion in
    the power stage, a looser bass when overdone **[S/E]**. The 5150, Rectifier-era Mesas and most
    modern high-gain heads have it **[L]**.
  - **Without feedback** (Rectifier, Vox) the voltage follows the speaker's impedance everywhere:
    a bump at resonance and a rise above ~2 kHz (voice-coil inductance) **[M]**: as resonance and
    presence turned fully up.
- **Model** **[E]**: as built (`chain`: master, sag, push-pull soft clip) plus two shelves after
  it, inside the oversampled block or at the base rate: a low peak at 100 Hz (Q ~1.5, 0 to +8 dB:
  resonance) and a high shelf at 3 kHz (0 to +8 dB: presence), their depth scaled by how much
  feedback the preset has; with "no feedback" both at their maximum and the clip a little softer.
  A circuit-level feedback loop (EG §11.4, Mačák's one-unknown solve) is not needed for GM 31.

---

## 7. Aliasing at high gain

- **The listening numbers** are in EG §11.7 **[T, Mačák]**: for tanh at 20–60 dB gain on a solo
  guitar, mean oversampling needed 2.8–4.2, maximum 6–14.
- **ADAA** **[T, PZL16]**: replacing f(x) by (F(xₙ) − F(xₙ₋₁))/(xₙ − xₙ₋₁) (F the antiderivative of
  f; f((xₙ + xₙ₋₁)/2) when the difference is tiny: the 0/0 case) is the waveshaper applied to the
  linear interpolation of the signal and averaged over the sample: a rectangular kernel; second
  order uses a triangular kernel (F₁, the antiderivative of x·f(x), also needed). At low levels the
  first-order form is a half-sample delay by linear interpolation (group delay 0.5 sample; second
  order 1 sample), so it slightly low-passes (cos(πf/fs): −0.4 dB at 20 kHz in a 4× loop **[E]**)
  and its delay must be absorbed in a feedback loop. Their measurement, a hard clipper at gain 10,
  a sine sweep, for the same aliasing (SNR ~46.5 dB):

| Kernel | Sample rate | × 44.1 kHz |
|---|---|---|
| none | 529.2 kHz | 12 |
| rectangular (1st order) | 176.4 kHz | 4 |
| triangular (2nd order) | 132.3 kHz | 3 |

  Higher orders "improve the signal-to-noise ratio considerably over the first-order method" **[S,
  Bilbao et al. 2017 abstract]**. Cost: two evaluations of F (often tabulated) and a divide per
  sample, against the 4–8× multiple of every filter in the oversampled loop **[E]**.
- **A simulation here** **[E]**: a guitar-like tone (harmonics 1/k^1.5 up to 6 kHz) through
  tanh(g·x), aliasing measured as the energy off the harmonic grid below 20 kHz relative to the
  total, raw / after a 5 kHz 4th-order low-pass standing in for the cabinet:

| Note, gain | 1× | 2× | 4× | 8× | 16× | ADAA 1× | ADAA 2× |
|---|---|---|---|---|---|---|---|
| E4 (330 Hz), 40 dB | −27 / −34 | −35 / −41 | −44 / −50 | −57 / −62 | −81 / −87 | −39 / −59 | −54 / −71 |
| E4, 60 dB | −24 / −31 | −31 / −37 | −39 / −44 | −46 / −52 | −53 / −59 | −36 / −56 | −50 / −68 |
| A5 (880 Hz), 60 dB | −20 / −26 | −26 / −32 | −33 / −39 | −41 / −46 | −48 / −54 | −32 / −51 | −45 / −63 |

  Each doubling buys only ~7 dB at 60 dB of gain; first-order ADAA at the base rate behind a
  cabinet is about as good as 8×, ADAA at 2× better than 16× (the cabinet removes ADAA's
  remaining aliases, which lie high) **[E]**. (The resampler was scipy's polyphase filter, not the
  project's elliptic one.)
- **What this means for the amp module** **[E]**: the built 4× with soft knees is fine for GM 30.
  GM 31 cascades three or four clipping stages, each making new harmonics of what the previous
  one made; the last stage's input is already a square-ish wave. Either 8× (a second
  `tools/oversample.py` table: 8th-order elliptic at 384 kHz, ~2× the cost of the stages) or 4×
  with first-order ADAA on each stage's static curve (the triode curve's antiderivative tabulated
  once; the grid-current bias shift stays outside the ADAA as now). The ADAA route keeps the
  filters at 4× and costs ~1.3× the 4× chain **[E]**. Check by ear and by the off-grid energy test
  of section 11.

---

## 8. Playing GM 31

### 8.1 Power chords and why thirds turn to mush

- **Voicing** **[L]**: root–fifth (2 notes) or root–fifth–octave (3 notes) on adjacent low strings
  (6-5-4 or 5-4-3), one shape moved along the neck; the octave doubled on the next string; in drop
  D, a one-finger barre across the three low strings.
- **Intermodulation** **[S, Lilja via search and Herbst's review]**: distortion makes sum and difference tones between
  the notes; a fifth (3:2) gives a difference tone an octave below the root (A5: a strong component
  at 55 Hz) and products that all lie on the root's harmonic series: acoustically the distorted
  power chord is a single, richer note, "similar to a major chord" (its partials contain the major
  third as the 5th harmonic). A major third (5:4; equal-tempered 14 cents wide) or a minor third
  (6:5; 16 cents narrow) gives products two octaves down and between the harmonics, mistuned by the
  temperament: a dense cluster of beating lines.
- **The simulation here** **[E]**: E2-based chords (each note harmonics 1/k^1.5), equal temperament,
  through tanh at ~50 dB of gain; spectral peaks within 40 dB of the strongest, below 1.5 kHz:

| Chord | Clean | Distorted |
|---|---|---|
| E2 alone | 18 | 9 |
| E5 (E2 B2 E3) | 24 | 39 |
| E major (E2 G#2 B2 E3) | 38 | 254 |
| E minor (E2 G2 B2 E3) | 39 | 313 |
| E5 with a G#3 on top | 31 | 182 |

- **Listeners** **[T, Herbst]**: 171 listeners rated chords (C3 root; Laney, Marshall JCM2000,
  Mesa Triaxis, Orange, 5150 into a V30 4×12): clean, overdrive, distortion: power chord 7.40 /
  6.76 / **6.06**, major 7.52 / 6.64 / **5.61**, minor 6.96 / 5.05 / **3.89**, altered dominant
  6.06 / 4.40 / 3.35 (1–10). Going from overdrive to distortion mostly separated power from major;
  minor and complex chords had already fallen at overdrive. The "muddy major third" is mild; the
  minor third and complex chords are what high gain punishes.
- **For the guitarist model** **[E]**: play what the file writes (the composer chose the notes), but
  voice it as a guitarist would on GM 31: low, close, on adjacent strings, the octave doubled where
  the file has root and fifth; never spread a chord into an open-position six-string shape; a third
  written *above* a power chord on the top string where the file asks for it. An option (off by
  default) to drop written thirds from low (below ~E3) chords of four or more notes is the one
  liberty a GM-31 player routinely takes **[L]**.

### 8.2 Palm-muted chugs

- **Where and how heavy** **[L]**: on the low E/A (or the dropped D), eighth and sixteenth notes
  on one pitch or a power chord, the palm right at the saddle (0–10 mm: lighter than GM 29's funk
  mute 8–16 mm, since the amp must still hear a pitch), the open accents between them unmuted; at
  120–200 bpm, sixteenths at 8–13 strokes/s. Heavier and further from the saddle for the "djent"
  thud. Under high gain the amp's compression lifts the muted note so that it keeps a pitched low
  thump; the gate then cuts it clean.
- **As numbers** **[E, from EG §8 and the built palm]**: the built GM 29 palm (fundamental to −30
  dB in ~0.25 s, 2nd in 0.1–0.16 s, 4th in 50–60 ms) at about 0.6–0.8 × its depth and 3–8 mm from
  the saddle; the amp compresses that 25–30 dB decay to ~10 dB; the gate (80 ms release) ends it.
  The chug's tightness then comes from the gate and the pre-clip bass cut (2.3), not the palm alone.
- **When a MIDI-driven guitarist should mute on GM 31** **[E]** (no published rule found; what a
  player reading a score without "P.M." marks would do):
  1. Register: the note (or a chord's lowest note) below ~A3 (MIDI 57), playable on strings 6–4.
  2. Length: the note held less than ~60 % of the gap to the next onset, or shorter than ~0.25 s
     (a sixteenth at 60 bpm, an eighth at 120).
  3. Repetition: the same pitch (or power-chord root) struck ≥ 3 times within 0.4 s gaps: a pedal
     tone; mute it, except the first of a group after a rest, a note ≥ ~15 velocity steps louder
     than its neighbours (an accent), or a note on a downbeat followed by a longer note.
  4. Not: notes ≥ ~0.4 s, any note above C4 (a lead line), chords of ≥ 3 notes held ≥ one beat
     (open power chords ring), notes with pitch bend.
  5. Depth: stronger the shorter and more repeated; velocity lightens it as on GM 29.
  Guitar Pro and similar exports mark palm mutes only by shortened (sometimes quieter) notes
  **[L/M]**, which rules 2–3 recover.

### 8.3 Drop tunings

- Standard E (E2 = MIDI 40) is the GM default. Drop D (low string to D2, 38) is the common metal
  tuning; also D standard, drop C, and seven-string B1 (35) **[L]**.
- **Rule** **[E]**: before playing, scan the part: lowest note 38–39 → drop D (or drop C♯/Eb);
  36–37 → drop C or whole-step-down; below 36 → a seven-string (B1) or fold up an octave (as now).
  Retuned strings are slacker: lower tension, more inharmonicity, a darker, looser low note **[M]**:
  the string spec follows (tension ∝ f², B ∝ 1/T).

### 8.4 Pinch harmonics

- EG §9.3 covers the mechanism. On GM 31 they are a lead device: an accented, long note (≥ 0.4 s),
  high velocity, on the G, D or A string, often with wide vibrato after **[L]**. Rule **[E]**: with
  probability ~0.15 on lead notes that qualify (MIDI 50–76, velocity ≥ 110, ≥ 0.4 s, not in a
  chord), touch the string at a node of the stopped length for 5–15 ms just after the pick (n =
  3–6 chosen near the bridge pickup's position: the harmonic the pickup hears best). Default off
  until GM 32's touch is built.

### 8.5 Wide vibrato and bends

- **[L]**: rock/metal lead vibrato is wide and often slow: a quarter to a whole tone, 4–7 Hz, by
  bending the string up and back (never below pitch: EG §15 #11); held notes at the end of a phrase
  get it after a short delay. The built guitarist uses ~14 cents × string/neck factors after 0.45–0.9
  s **[README]**; for GM 31 leads (single notes above ~E3, held ≥ 0.6 s) scale to 40–100 cents,
  4.5–6 Hz, rising in over 0.3 s **[E]**. No measurement of guitarists' vibrato rates or depths was
  found (also EG §17).

### 8.6 Controlled feedback

- **Mechanism** **[M/E]**: at stage volume the cabinet's sound (110–120 dB at 1–2 m **[M]**) drives
  the guitar's body, neck and strings; a string mode whose frequency lies where the loop (string →
  pickup → amp (compressed, so the loop gain at small level is huge) → speaker → air delay →
  body → string) has gain > 1 and the right phase grows exponentially until the amp's compression
  limits it. The loop's phase depends on the distance (1 m: 2.9 ms of air), so which partial wins
  is chosen by where the player stands and how the guitar is turned; usually the fundamental of a
  fretted note or its 2nd or 3rd harmonic: the note "blooms" into an octave or a twelfth **[L]**.
  Sullivan's 1990 model: per string a Karplus–Strong loop, the distortion, and a second delay line
  (the amp-to-guitar distance) back into the strings: the feedback amount and the delay set the
  intensity and pitch **[S, via Costello/Nord Modular]**.
- **Timing** **[L/T]**: players expect a held note to start blooming after ~1–2 s and take over by
  2–4 s; the FluidR3 E2 sample's 2nd harmonic rises 9 dB over its first 2 s **[T, sf2]** (1), which is
  that behaviour or the amp's bloom.
- **Model** **[E]**: EG §11.1's "path back": the cabinet's output, delayed by d/343 s (d = 1–3 m),
  band-passed around 80 Hz–3 kHz (what the body and strings pick up), scaled, into the bridge (the
  shared bridge input that `Guitar` already has for the body). The loop gain set so that only
  sustained notes (≥ 1.5 s, held, not muted) cross 1 after the attack's compression has eased; a
  "feedback" amount knob, 0 by default except on the XG/SC "Feedback Gt" variations.

---

## 9. What a listener would notice if it were wrong

In addition to EG §15 (#6 fizzy distortion without the cabinet's low-pass, #7 aliasing, #8 thirds,
#9 palm mute as a short note, #15 feedback that never blooms):

1. **Fizz:** too much energy at 5–10 kHz after the distortion: the cabinet must fall ~25–40 dB per
   octave above 5 kHz (5.1; FluidR3: −26 dB at 8 kHz, −38 at 10 kHz re 1 kHz). The most common
   failure of synthetic high gain.
2. **No tightness:** a palm-muted low E that "farts" or blurs: too much bass into the clipping
   stages, the bias riding the 82 Hz swing; chugs that run into each other (no gate, no bass cut).
3. **Mush on chords:** a chord at the same gain as a single note but louder, or full open-position
   triads on GM 31; also no intermodulation at all (a layered-sample sound: the notes stay
   separate as in FluidR3).
4. **Missing gate:** hiss and hum in every rest, strings ringing between riffs; or the opposite: a
   gate that chatters on decaying notes, or chops sustained notes ("loses gain").
5. **Velocity as volume:** 17 dB between soft and hard (FluidR3) instead of the amp's few dB with a
   harder attack.
6. **A dead sustain:** notes decaying like a clean guitar's; a held distorted note should sit within
   ~2–3 dB for 2 s and change colour (partials exchanging, bloom) rather than fade.
7. **Static, too-perfect tone:** no hiss under the note, no pick scrape, no string noise; every
   repeated chug identical (the palm should vary as GM 29's does).
8. **Wrong presence:** a single 1.8 kHz bump instead of the 2.3/3.2 kHz pair (nasal, honky), or no
   presence at all (blanket-dark, as Jnsgm2).
9. **Boomy bottom:** a closed 4×12's bump at 100–120 Hz plus resonance plus the proximity effect
   stacking into a boom; or no low end at all (an open-back combo's).
10. **Aliasing on bends:** inharmonic whistles moving the wrong way during a bend or vibrato on high
    notes.

---

## 10. What to build

Mapped onto `src/guitaramp.h/.cc`. All values **[E]** unless cited.

### 10.1 Stages: a list instead of two

- `Preset::Stage stages[5]; int nStages;` replacing `s1, s2`; `stackAfter` becomes "after stage k"
  (0: before all, as a pedal's place; nStages: after all, the Soldano/Rectifier position);
  `volume`/`bright` stay the pot after stage 1. Each stage gains:
  - `shelfHz, shelfDb`: a partial cathode bypass as a first-order low shelf (bass `shelfDb` under
    treble below `shelfHz`); e.g. −3.2 dB at 169 Hz, −4.7 dB at ~130 Hz **[T values, Kuehnel]**.
  - `divider`: the interstage attenuation (0.68, 0.77 **[T]**), and an optional treble bypass
    (`divC`: the 2000 pF across the 470 k **[T]**).
  - `cold`: an unbypassed, cold-biased stage: low gain (~2.4×, 7.7 dB unloaded **[T]**), its bias
    near cut-off in the `triode()` scale (−2.2 of −2.8: it clips its lower half after ~0.6 V and
    its upper half only at grid conduction), its plate low-pass 1.6 kHz **[T]**.
  - `follower`: a DC-coupled cathode follower after the stage: a soft limit on the positive swing
    at ~+1.5–2 V in the next stage's scale (grid current into the follower), symmetric otherwise
    **[S/E]**.
- The grid-current bias shift as built, per stage; the coupling HP per stage (`hp`): for the tight
  presets 72–160 Hz into the clipping stages (2.2 nF), for loose 7–20 Hz.

### 10.2 Presets for GM 31

| Preset | Stage list (gain, bias, hp Hz, lp Hz, shelf) | Stack | Knobs | Power | Cabinet |
|---|---|---|---|---|---|
| `slo` (main GM 31) | V1a 60, 0, 10, —, −3 dB @ 170 → pot (gain 0.5 of a log pot, 2000 pF treble bypass, 470 k/500 k) → V1b 40, 0, 15, —, −4.5 dB @ 130; ×0.68 → V2a cold 2.4, −2.2, 30, 1600 ; ×0.77 → V2b 75, 0, 72, 9000 → follower | after all; Marshall values (Bassman-type not given) | T 0.6 M 0.5 B 0.6 | master 0.4; 6L6 (clip a little later and softer than EL34); resonance +4 dB, presence +3 dB | V30 4×12 + SM57 (5.4) |
| `slo-ts` | as `slo`, with a TS in front: drive 0, level such that above 720 Hz the amp sees +15 dB, tone 0.55 | | gain 0.35 (the TS adds the rest) | | |
| `rectifier` (variation) | as `slo` with larger coupling (20 Hz) and full cathode bypasses (no shelves), no feedback: resonance +7, presence +6, sag 0.25 s and deeper (valve rectifier) | | T 0.5 M 0.35 B 0.7 (scooped) | | V30 4×12 |
| `jcm800-boost` (variation) | the built `jcm800` with V1a's booster in front (a third stage, 36 dB) and the gain at 0.5 | Marshall | as `jcm800` | EL34 | Greenback 4×12 |
| `ds1` / `rat` (later) | a pedal clipper (EG §11.8; section 3.2) into `twin` or `jcm800` at low gain | | | | |

- Gain staging check **[E from T]**: with `slo` the humbucker's 0.5 V should put V2b ~40 dB over
  its knee on a single note (2.1), a soft (velocity 30) note ~25 dB over: still fully distorted,
  only the attack and the noise differ.
- Compressor: none on GM 31 (the amp compresses); the TS takes its place.

### 10.3 The Tube Screamer block

- Input HP 15.9 Hz; the clipping stage as a one-capacitor ODE or a static curve: the added path
  `v = clip( (R2/4.7k) · HP720(x) ) ` with `R2 = 51k + D·500k`, the 51 pF low-pass on it, two 1N914
  as a symmetric soft clip (a diode pair tabulated, or `asinh` with Is 2.5 nA, n 1.75 **[M]**), output
  `x + v`; tone: one-pole LP at 723 Hz…10 kHz (the Rs 1 kΩ / Cs 0.22 µF section and the pot
  **[T, YehThesis]**); level. Oversampled with the stages (it clips) **[T, YehThesis: 8× with a
  static curve captures "the general character"]**.

### 10.4 The noise gate and a noise floor

- `Gate`: detector on the input EMF (after `input_`, before the TS): peak follower (0.5 ms attack,
  10 ms release), open above `gateAt` (default 2 mV for the hot humbucker), close 6 dB lower, hold 15
  ms, gain ramp 0.5 ms open / 80 ms close in dB, range −80 dB; applied between the last preamp
  stage and the master (inside the oversampled loop: one multiply per sample). Knobs
  `PJ_AMP_GATE` (threshold, 0: off), `PJ_AMP_GATEREL`.
- **A noise floor** (all amps, only audible at high gain): thermal noise of the pickup's R and a grid
  stopper (2–5 µV rms white, injected at the first stage) and hum (60 or 50 Hz with odd harmonics
  to ~1 kHz; −25 dB for a humbucker against a single coil; ~10–30 µV for the humbucker) **[E/M]**.
  With these the gate has something to do and the rests sound like an amp.

### 10.5 Power stage and cabinet

- Two shelves after the power stage: resonance (peak, 100 Hz, Q 1.5, 0…+8 dB), presence (high shelf,
  3 kHz, 0…+8 dB) (6). A 6L6 / EL34 flag that moves the clip and its knee.
- Cabinet type 2: "V30 closed 4×12, SM57 off-cap": section 5.4's nine biquads. Refit type 1
  (Greenback) to Celestion's curve (the 1.6 kHz dip and the 2.3/3.2 kHz pair) and recheck GM 30
  by ear.

### 10.6 Oversampling

- `tools/oversample.py` gains an 8× table (`kSos8`; at 384 kHz, the same 19 kHz / 26 kHz / 80 dB
  spec), `Preset::oversample` 1, 4 or 8. Or (preferred if it passes the ear test) first-order ADAA
  on `triode()` at 4×: tabulate T(x) = ∫triode, use (T(u) − T(u₋₁))/(u − u₋₁) with the midpoint
  fallback when |u − u₋₁| < 1e-4 V, and absorb the half-sample delay (it is outside any feedback
  loop in this chain, so it is only a delay).

### 10.7 The guitar and the guitarist

- A new instrument name `distortion-guitar` (midifile.cc's kGm entry for GM 31 off `lead-charang`).
- Guitar: the GM 30 Les Paul spec; the bridge humbucker as built (8 H, 13.5 kΩ: "hot"); optional
  an active-pickup variant (EMG 81-type: a low-impedance source, no coil resonance, ~1 V peaks **[L]**:
  `Source{L = 0}` or a low R/L) later.
- Guitarist (`guitarinst.cc`, a `distorted_` flag beside `driven_`): pick over the bridge pickup
  (~4 cm: `where *= 0.55`), strict alternate picking on repeated notes (as built), the GM-31 palm
  rule of 8.2, drop tuning of 8.3, wide vibrato of 8.5, power-chord voicing preference of 8.1 in
  the fingering solver (adjacent low strings, octave on the next string), the fretting hand
  muting unplayed strings (already: "an electric player mutes what is not playing").
- Later: pinch harmonics (8.4, after GM 32's touch), feedback (8.6).

### 10.8 Cost

- 4–5 stages + TS + gate at 4× (or 8×): about 2–2.5× the `jcm800` chain's stage cost; the
  `jcm800` instrument costs 2.4–3.7 % of a core in total with the guitar **[README]**, so GM 31
  ≈ 3.5–5.5 % at 4× + ADAA, 5–8 % at 8× **[E]**. Against EG §14's target of ≤ 3 %, ADAA is the
  route that may meet it.

---

## 11. Checks (measurable targets for `tools/test.py`)

1. **Spectral envelope** of held notes (E2, A2, E3, A3, E4, 0.1–1.5 s, harmonic levels pooled per
   third octave, normalised at 0.6–1.5 kHz) within ±4 dB of FluidR3's table (1) from 200 Hz to 8
   kHz; a peak at 2.5–3.2 kHz ≥ +4 dB and a dip at 1.3–2 kHz ≤ −1 dB re 1 kHz; ≤ −8 dB at 5 kHz,
   ≤ −20 dB at 8 kHz, ≤ −30 dB at 10 kHz.
2. **Sustain:** a held E2 (velocity 100) within 3 dB of its peak from 0.2 to 2.0 s; ≥ −8 dB at 4 s
   (FluidR3: −1.6 dB at 2 s, raw sample −5 dB at 5 s, −7 dB at 11 s).
3. **Velocity:** level difference velocity 40 → 127 ≤ 6 dB on single notes (FluidR3: 16.7 dB, all
   from its velocity curve); the attack (first 30 ms) brighter at 127 than at 40 (centroid ≥ +10 %).
4. **Chord level:** E5 power chord (E2 B2 E3) ≤ +2.5 dB above E2 alone (FluidR3 +3.8).
5. **Intermodulation:** spectral-peak count below 1.5 kHz (within 40 dB of the maximum) of a held E
   major chord ≥ 3 × that of the E5 power chord (simulation: 254 vs 39).
6. **Chugs:** sixteenth-note palm-muted E2 at 160 bpm (0.094 s apart): between strokes the level
   falls ≥ 15 dB under the stroke peaks (gate on); centroid of each chug's first 80 ms ≤ 600 Hz.
7. **Gate:** after a note is released (or its string decays), the output ≤ −60 dB re the playing
   level within 200 ms; on a decaying held note no more than one close (no chatter: count gain
   transitions).
8. **Noise floor without the gate:** hiss + hum in a rest at −35 to −50 dB re a held note (an amp
   at high gain is noisy; silence would be wrong).
9. **Aliasing:** a held A5 and a bent E5 at the preset's gain: energy off the harmonic grid ≤ −45
   dB (re total, after the cabinet) at the chosen oversampling (8× or 4× + ADAA); compare 1× to hear
   it.
10. **Tightness:** palm-muted E2 chugs with and without the TS: the minima between chugs ≥ 5 dB lower
    with the TS; no blocking: a soft note 50 ms after a hard power chord within 3 dB of the same
    soft note alone.
11. **Cost:** ≤ 5 % of a core for a playing GM 31 (target ≤ 3 % with ADAA).

---

## 12. Not found, or uncertain

- **Verified stage tables for the Mesa Dual Rectifier, the Peavey 5150 and the JCM900/DSL:** only
  forum reports (values marked [S]); a DIY Rectifier clone's schematic image could not be
  downloaded. Only the Soldano was read stage by stage (Kuehnel's analysis), and even there the
  tone-stack values and the master were not given.
- **Electrosmash** (TS808, RAT, DS-1, Fuzz Face analyses): still unreachable (DNS failure). Geofex's
  "Technology of the Tube Screamer" reset the connection. The RAT numbers are search snippets and
  arithmetic.
- **Boss NS-2 and ISP Decimator time constants:** not published as numbers in what was found; the
  gate settings are producers' practice.
- **SM57 position effect in dB, measured 4×12 responses (closed box bump frequency and Q), V30
  Thiele–Small parameters** beyond Fs and Re: not found.
- **Presence and resonance curves** of real amps in dB: only component values; the dB figures are
  arithmetic and [M].
- **Feedback:** no measurement of feedback onset times, loop gains or the guitar body's pick-up of
  airborne sound; Sullivan (CMJ 1990) not read.
- **Palm-mute practice on GM 31** (how often, how heavy, how players decide): no study; the rule in
  8.2 is an estimate. Guitar Pro's MIDI export of palm mutes: from memory.
- **Vibrato widths and rates of rock/metal guitarists:** no measurement found.
- **The Roland and Yamaha distortion-guitar samples:** not accessible; only names (EG).
- **ADAA higher-order numbers** (Bilbao et al. 2017): the abstract only; PZL16's table read.
- **The simulations in 7 and 8.1** are idealised (one tanh, synthetic tones): they rank options,
  they do not predict the amp's numbers.

---

## 13. References

- **[Kuehnel/ampbooks]** R. Kuehnel (Amp Books), circuit analyses read as web pages: "Soldano Super
  Lead Overdrive Channel" https://www.ampbooks.com/mobile/classic-circuits/soldano-slo/ ; "Marshall
  JCM800 2204 Power Amp" https://www.ampbooks.com/mobile/classic-circuits/marshall-jcm800-2204-power-amp/ ;
  "Peavey VTM Response Modifications" https://www.ampbooks.com/mobile/classic-circuits/peavey/
  (machine-summarised pages; numbers as stated there).
- **[Robinette/Blais]** R. Robinette, "Tube Guitar Amp Overdrive" (quoting builder D. Blais),
  https://robrobinette.com/Tube_Guitar_Amp_Overdrive.htm ; "How the Soldano SLO-100 Works",
  https://robrobinette.com/How_the_Saldano_SLO-100_Works.htm (not read); "Amp Voicing",
  https://robrobinette.com/Voicing_an_Amp.htm (not read).
- **[PZL16]** J. D. Parker, V. Zavalishin, E. Le Bivic, "Reducing the Aliasing of Nonlinear Waveshaping
  Using Continuous-Time Convolution", DAFx-16. Read (method, Table 1).
  https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf
- **[BEPV17]** S. Bilbao, F. Esqueda, J. D. Parker, V. Välimäki, "Antiderivative Antialiasing for
  Memoryless Nonlinearities", IEEE SPL 24(7), 1049–1053, 2017 (abstract).
  https://aaltodoc.aalto.fi/items/470aab15-1702-4ccf-a148-24e6173079fb ; notes:
  https://ccrma.stanford.edu/~jatin/Notebooks/adaa.html
- **[Herbst]** J.-P. Herbst, "Distortion and Rock Guitar Harmony: The Influence of Distortion Level
  and Structural Complexity on Acoustic Features and Perceived Pleasantness of Guitar Chords",
  accepted manuscript (journal and year from memory: Music Perception, 2019 [M]). Read (method, Tables 1, 4, 5).
  https://pure.hud.ac.uk/ws/portalfiles/portal/15105454/Herbst_Distortion_and_Rock_Guitar_Harmony.pdf
- **[Lilja]** E. Lilja, "Theory and Analysis of Classic Heavy Metal Harmony", PhD, Helsinki 2009; "Dealing
  with the 3rd", 2015 (via Herbst and search). https://iipc.utu.fi/MHM/Lilja2.pdf
- **[Celestion plots]** Celestion product pages and response plots (Vintage 30: T3903; G12M
  Greenback: T1221), read by eye.
  https://celestion.com/product/vintage-30/ , https://celestion.com/wp-content/uploads/2019/09/T3903-Vintage-30-copy.jpg ;
  https://celestion.com/product/g12m-greenback/ , https://celestion.com/wp-content/uploads/2019/09/T1221-G12M-Greenback-copy.jpg
  (the Greenback plot's axis is mislabelled "Impedance"; it is SPL). Catalogue (no plots):
  https://celestion.com/wp-content/uploads/2020/03/Guitar_Speaker_Catalogue.pdf
- **[SOS]** Sound On Sound, "Understanding & Recording Guitar Speakers" (machine summary).
  https://www.soundonsound.com/techniques/understanding-recording-guitar-speakers
- **[Shure, Sweetwater, Thomann]** mic-position descriptions (search snippets):
  https://www.thomannmusic.com/onlineexpert_page_recording_e_guitars_microphone_positioning.html ;
  https://www.soundonsound.com/sos/aug07/articles/guitaramprecordingaudio.htm
- **[NS-2]** Boss NS-2 owner's manual and product page (search snippets).
  https://www.adorama.com/col/productManuals/BSNS2.pdf ; https://www.boss.info/us/products/ns-2/
- **[Gate guides]** Nail The Mix, "Noise Gate For Metal Guitars", https://nailthemix.com/noise-gate-for-metal-guitars ;
  Logic Pro manual (hysteresis), https://help.apple.com/logicpro-effects/mac/10.1/en.lproj/lgcef1bec259.html ;
  Fractal forum example, https://forum.fractalaudio.com/threads/need-inputs-on-my-noise-gate-settings-for-high-gain-presets.66057
- **[TS boost lore]** Reverb.com, "Using Pedals to Shape High-Gain Amp Tones",
  https://reverb.com/news/using-pedals-to-shape-high-gain-amp-tones ; rig-talk thread,
  https://www.rig-talk.com/forum/threads/pedals-that-tighten-but-dont-cut-low-end-do-they-exist.207961
- **[RAT]** cushychicken, "Simulating the ProCo RAT in LTSpice", https://cushychicken.github.io/ltspice-proco-rat (not read);
  electrosmash, https://electrosmash.com/proco-rat (unreachable); diystompboxes thread on the
  560 Ω/4.7 µF leg, https://diystompboxes.com/smfforum/index.php?msg=797495
- **[Forums: Rectifier, 5150, JCM900, resonance]** (values marked [S]):
  https://music-electronics-forum.com/forum/amplification/guitar-amps/maintenance-troubleshooting-repair/41574 ;
  https://www.rig-talk.com/forum/threads/some-questions-about-the-original-peavey-5150.89154/ ;
  https://www.audunmelbye.no/2012/10/10/peavey-5150-6505-series-mods/ ;
  https://marshallforum.com/threads/resonance-control-negative-feedback-loop-discussion.49874/ ;
  https://ampgarage.com/forum/viewtopic.php?t=17099 ;
  https://www.complang.tuwien.ac.at/kim/misc/erectoamp/erecto_main_english.htm
- **[Sullivan]** C. R. Sullivan, "Extending the Karplus-Strong Algorithm to Synthesize Electric
  Guitar Timbres with Distortion and Feedback", Computer Music Journal 14(3), 1990 (not read;
  described in https://cim.mcgill.ca/~clark/nordmodularbook/nm_physical.html).
- **[FluidR3]** /usr/share/sounds/sf2/FluidR3_GM.sf2, preset 30 "Distortion Guitar"; parsed and
  rendered with TiMidity (`-c /etc/timidity/fluidr3_gm.cfg -Ow -s 48000 --output-mono -EFreverb=0
  -EFchorus=0`) on 2026-11-19. **[Jnsgm2]** ~/soundfonts/Jnsgm2.sf2, preset 30 "Distorted Guitar".
- EG's references: [PY09], [YehThesis], [YS06], [Mačák], [Koren96], [SC-88Pro], [DB50XG]: see
  `research/electric_guitars.md` §19.
