# The Leslie rotating speaker (122 / 147) as a physical model

Status: 2026-11-16. Research and plan; nothing is built. This is the speaker for the Hammond
B-3 of `research/hammond_b3.md` (GM 17-19). That paper ends at the organ's console output, and
this one starts there (§9). Nothing in `src/` models a rotating speaker yet. The room is
`src/room.*`. Stereo goes out through `Tube::takeSide` (`src/tube.h`; see `src/accordion.cc`).

Marks: **[paper]** a paper or primary document in §14; **[web]** a weaker web source in §14
(makers' manuals, emulators, forums, restorers); **[calc]** worked out here (scripts in the session
scratchpad, `hammond/calcs.py`, `leslie_calc.py`); **[memory]** general knowledge, to be checked;
**[design]** a modelling choice; **[rec]** measured in the user's recordings (none has a Leslie:
§11).

## 1. Short answer

- **What it is.** The 122 and 147 are the same box.
  - A **40 W tube amplifier** (two 6550s, push-pull; a 12AU7 driver).
  - An **800 Hz, 12 dB/oct, 16 Ω passive crossover**.
  - A **15-inch woofer** firing down into a **rotating wooden drum with a scoop**.
  - A **compression driver** (3/4-inch throat) firing up into a **rotating twin horn**, one bell
    live and one a dummy counterweight, with a diffuser cone at the mouth.
  - Each rotor has a fast and a slow AC motor, switched by a relay. A belt drives the rotor.
  - Everything sits in a louvred three-compartment cabinet 743 × 524 × 1057 mm **[paper,
    Henricksen; web, Hammond specs]**.
  - The 122 has a **balanced** input and is speed-switched by a DC voltage on the audio pair. The
    147 has an unbalanced input and a separate relay line **[web, Uneeda]**.
- **Speeds** (one 147, measured): horn **400 rpm (6.67 Hz) fast, 48 rpm (0.80 Hz) slow**; drum
  **342 rpm (5.70 Hz) and 40 rpm (0.67 Hz)** **[web, diyaudio/HammondWiki]**. Other units and
  emulators give 396-423 rpm and 31-49 rpm (§4). The drum turns a little slower than the horn, so
  the two beat.
- **Spin-up and spin-down: no primary measurement found.** Values in use:
  - Hammond-Suzuki's own Leslie 2101mk2 (modelled on the 122): horn rise 1.8-4.2 s and fall
    2.4-3.4 s; drum rise 12-13 s and fall 5.5-12 s **[web, L2101]**.
  - setBfree's first-order time constants: horn 0.16 s up and 0.32 s down; drum 4.1 s up and
    1.4 s down **[web, SBF]**.
  - Forums: horn ~1-3 s, drum ~5-9 s **[web]**.

  The drum is the slow one, by roughly 5-10× **[paper, Henricksen]**.
- **Doppler.** A source on radius r turning at ω has peak radial speed rω, so the deviation is
  **±rω/c**. With r ≈ 0.15-0.19 m: **±32-41 c fast, ±4-5 c slow** **[calc]**. The peak is the same
  at any mic distance. Close mics make the FM less sinusoidal (2nd harmonic of the FM 22 % at
  0.42 m) and add distance AM (8.5 dB at 0.42 m, 3.3 dB at 1 m), before directivity **[calc]**.
- **The drum is mostly AM**, "only for the upper two octaves or so of the bass section (200 to
  800 Hz)". Below 200 Hz the scoop is too small to matter **[paper, Henricksen]**.
- **Directivity.** Without its diffuser the horn is "beamy", with strong AM. With it the horn is
  "almost omnidirectional; note, however, the 'lobing'", and the diffuser keeps the apparent source
  at the mouth (more FM) **[paper, Henricksen]**. Measured (Leslie 600 horn, 15° steps): below
  ~12 Bark (~1.7 kHz) the angle mainly lowers the level as the horn turns away, the treble falling
  faster; an angle-independent "base leakage" arrives first **[paper, SSAB02]**.
- **A faithful model is:**
  - for every acoustic path (direct, early reflections, the louvres) from each rotating source
    (horn mouth, drum scoop) to each of two microphones, a **delay line with a moving read tap**
    whose delay is the path length / c;
  - an **angle-dependent filter** per path (directivity);
  - a small **cabinet reverberator**;
  - the **rotor mechanics** (motor torque, belt, inertia, drag);
  - the **tube amplifier** in front, with its overdrive;
  - stereo from **two mics**.

  Cost: well under 1 µs per sample (§12.4).

## 2. The cabinet

| Item | 122 / 147 | Source |
|---|---|---|
| Amplifier | 40 W rms, tube, 2 × 6550 push-pull, 12AU7 driver | **[paper, Henricksen; web, Hammond, Uneeda]** (Uneeda: "about 60 W") |
| Crossover | passive, 800 Hz, 12 dB/oct, 16 Ω | **[paper, Henricksen]** |
| Treble | Jensen compression driver, 3/4-inch throat, into a rotating twin horn (one live, one dummy), bakelite, diffuser cone | **[paper, Henricksen]** |
| Bass | 15-inch woofer, facing down, into a rotating wooden drum with a scoop, drum covered in black scrim cloth | **[paper, Henricksen; web, Hammond]** |
| Motors | 4 shaded-pole AC motors (fast and slow per rotor), relay-switched; belt; horn motor with 3 selectable pulleys; tension spring | **[paper, Henricksen; web, Uneeda]** |
| Cabinet | 29¼ × 20⅝ × 41⅝ in (743 × 524 × 1057 mm), 149 lb; three compartments | **[web, Hammond specs]** |
| Inputs | 122: balanced, ~10 kΩ bridging, DC speed control on the audio pair; 147: unbalanced, "16 Ω / 8 Ω / open" load switch | **[web, Uneeda; paper, Henricksen]** |
| Siblings | 145 (smaller, "allegedly" less bass), 22/45/47 (single-speed predecessors), 122RV (reverb), 142 | **[paper, Henricksen]** |

- **Three compartments** **[paper, Henricksen]**:
  - "the top compartment houses the rotating high-frequency horn;
  - the middle 'box' the high- and low-frequency drivers and crossover (and which also acts as a
    vented box for the low-frequency driver);
  - the bottom compartment the low-frequency rotor and amplifier.
  - Louvres located in the top and bottom compartments let out treble and bass sound respectively."
- **Players modify them.** "Many Leslies played in clubs are faced backwards, with the top and
  bottom rear panels and deflectors removed, because the sound 'cuts' better" **[paper,
  Henricksen]**. That is an option for the model (§12.2).
- **Never connect a Hammond to a 147.** "Only connect a Hammond Organ to a Model 122, 22, or 122RV;
  never connect a Hammond via a 'Leslie Cord' to a Model 147 or 145 … It will basically blow up"
  **[paper, Henricksen]**. The 122 is the B-3's speaker **[design: model the 122]**.

## 3. The rotors

### 3.1 The horn

- **The bells.** "A twin-bell, molded black bakelite horn, which starts vertically but flares
  horizontally, sits on this bearing/tube" over the driver. "Only one is operable as a horn. The
  other side, a 'dummy', acts as a counterweight or dynamic balancer, providing symmetric air drag
  at high speed" **[paper, Henricksen]**. "The Model 44W horn is identical to that of the Model
  600, and evidently standard across all Leslie models" **[paper, SSAB02]**.
- **The rotation radius** (axis to effective source) was **not found in a primary source**.
  setBfree uses 19.2 cm **[web, SBF]**. **[design]** r_h = 0.17 m ± 0.02 (the horn's total span is
  ~0.4-0.5 m tip to tip, from photos; to measure).
  - With the diffuser in, "more of the entire range of the horn will appear to come from the
    mouth". Without it, "higher and higher frequencies will appear to come from progressively
    further down the throat. They therefore appear to be rotating at a smaller radius, which
    results in less frequency modulation" **[paper, Henricksen]**.
  - So r_h(f) is a frequency-dependent radius: constant with the diffuser, falling at high f
    without it.
- **The mouth size** was not found **[to measure]**. The throat is 3/4 inch **[paper, Henricksen]**.
- **The response.** "±5 dB from 400 Hz to 10 kHz"; the 800 Hz-1.5 kHz part "isn't affected by
  removing the reflector". Without the deflectors there is "extra sensitivity above 1.5 kHz"
  on-axis **[paper, Henricksen]**. That is with a University ID-40 driver, measured anechoically.

### 3.2 The drum (bass rotor)

- **The drum.** "A rotating wooden drum mounted on a vertical shaft, and covered with a black scrim
  cloth, which provides lower aerodynamic drag on the drum at fast speed. At the drum's center is a
  cylinder fitted with a 'scoop' which, as in the treble unit, starts vertically (the bass driver
  faces downward into its entrance) and projects sound horizontally" **[paper, Henricksen]**.
- **Its effect.** "The bass rotor works just as an AM device, and only for the upper two octaves or
  so of the bass section (200 to 800 Hz). Frequencies lower than 200 Hz are probably unaffected by
  a scoop of this size, since 200 Hz has a wavelength of approximately 5.5 feet. There may be some
  frequency-modulation effects near the 800 Hz crossover point" **[paper, Henricksen]**.
- **The radius:** setBfree uses 22 cm **[web, SBF]**. **[design]** r_d = 0.2 m for the scoop
  opening (the drum fills most of the 524 mm cabinet depth; to measure).
- **Players disconnect it.** Some organists "find that the bass rotor's slower response to speed
  changes is distracting, and will actually disconnect the bass rotor drive, especially when
  playing a bass line" **[paper, Henricksen]**. That is an option: drum stopped **[design]**.

## 4. Speeds

| Source | Horn fast | Horn slow | Drum fast | Drum slow | Type |
|---|---|---|---|---|---|
| Leslie 147, one owner (belt middle pulley, normal tension) | 400 rpm = 6.67 Hz | 48 rpm = 0.80 Hz | 342 rpm = 5.70 Hz | 40 rpm = 0.67 Hz | **[web, diyaudio citing HammondWiki]** measured |
| Another owner | 409 rpm = 6.82 Hz | 48-49 rpm | 396 rpm = 6.60 Hz | 48-49 rpm | **[web]** measured |
| Forum summary | 5.7-6.8 Hz | ~0.8 Hz | | | **[web, musicplayer]** |
| Leslie 2101mk2 preset 1 (Hammond-Suzuki, "122") | 400 rpm | 46 rpm | 400 rpm | 46 rpm | **[web, L2101]** design |
| 2101mk2 preset 2 | 400 rpm | 46 rpm | 376 rpm | 31 rpm | **[web, L2101]** design |
| 2101mk2 ranges | 375-450 rpm | 24-318 rpm | 372-431 rpm | 24-48 rpm | **[web, L2101]** |
| setBfree | 423.36 rpm = 7.06 Hz | 40.32 rpm = 0.67 Hz | 357.3 rpm = 5.96 Hz | 36.0 rpm = 0.60 Hz | **[web, SBF]** design |
| Magazine | ~7.5 Hz | ~0.5 Hz | | | **[web, muzines]** |

- **[design] Defaults:** horn 6.7 / 0.80 Hz, drum 5.9 / 0.67 Hz. The drum's fast speed varies most
  between units (342-396 rpm). The horn speed depends on the pulley step (three choices) and belt
  tension **[paper, Henricksen; web]**.
- **The speeds are only roughly mains-locked.** Shaded-pole motors run at a load-dependent slip
  below synchronous speed **[memory]**. So temperature, belt and bearing state move them a few
  percent. **[design]**: a per-cabinet random ±3 % on each speed, plus a tiny slow wander.

## 5. Acceleration and braking

### 5.1 What is known

| Source | Horn up | Horn down | Drum up | Drum down | Meaning |
|---|---|---|---|---|---|
| Leslie 2101mk2 preset 1 | 4.2 s | 3.4 s | 12 s | 12 s (brake 12) | "transition time", seconds **[web, L2101]** |
| 2101mk2 preset 2 / Japanese edition | 1.8-3.0 s | 2.4-3.0 s | 12-13 s | 5.5-12 s | same **[web, L2101]** |
| setBfree | τ = 0.161 s | τ = 0.321 s | τ = 4.127 s | τ = 1.371 s | first-order time constants **[web, SBF]**; 95 % at 3τ = 0.48 / 0.96 / 12.4 / 4.1 s **[calc]** |
| x42-whirl | | | | | its constants are "the time to reach half the speed" **[web, zynthian]** |
| Forums / blogs | ~1-1.5 s | | ~5 s | | horn "1-3 s", drum "5-9 s" **[web, musicplayer; Monroe]** |
| Herrera et al. 2009 | measured | measured | measured | measured | "acceleration and deceleration dynamics … carefully measured and implemented using separate filters" (numbers not retrieved) **[paper, HHA09 via JAES review]** |

- **Agreement:** the drum's ~12 s to full fast speed (2101 and setBfree's 3τ agree).
- **Disagreement:** the horn, 0.5 s (setBfree) against 2-4 s (2101). Belts age: "Belts slip with
  age, which makes the transition time noticeable", and times "can change dramatically with age
  and/or lack of service" **[web, Yamaha; Low-Pro]**. The **D.C. brake**: "Direct current is
  sometimes applied to the 'fast' motor so that it will slow down more quickly" **[paper,
  Henricksen]**.
- **Asymmetry.** Spin-up and spin-down differ, and the horn and drum differ in opposite ways in
  setBfree:
  - the horn speeds up faster than it slows down (a light rotor with a strong motor);
  - the drum slows faster than it speeds up (heavy, with a weak motor and drag helping it stop).

  This is what a torque-inertia-drag model gives (§5.2).

### 5.2 A physical rotor model [design; calc]

For each rotor, with angle φ and speed ω:

J·dω/dt = τ_motor(ω) − b·ω − d·ω·|ω| − τ_brake,  dφ/dt = ω

- **The motor through the belt:** τ_motor = k·(ω_sync·ρ − ω) for the energised motor (the linear
  part of an induction motor's torque-slip curve). ρ is the pulley ratio. Belt slip limits it:
  |τ_motor| ≤ τ_slip, a Coulomb limit. That gives a **linear ramp** when the belt slips and an
  **exponential** approach otherwise, which is what players describe.
- **The idle motor:** its free rotor's inertia adds to J (reflected by ρ²).
- **Drag:** d is aerodynamic (the horn has two bells; the drum has the scrim to cut it); b is the
  bearings and belt.
- **Fitting:**
  - The steady state on each motor fixes k against b, d.
  - The fast→slow time fixes J/(b + d·ω).
  - The slow→fast time fixes J/k (or τ_slip).
  - With the 2101's 4.2 s / 3.4 s horn, 12 s / 12 s drum, and a 400/46 rpm horn target, three
    parameters per rotor are determined.
  - **[design] Default:** a horn time constant ~0.5 s up / 0.8 s down and a drum ~3.5 s up / 2.5 s
    down. These are between setBfree and the 2101, with a "belt age" knob that lowers τ_slip.
- **Brake (stop):** the drum coasts to a stop, which the 2101 calls "Brake Time" (10-14 s for the
  drum, 3-3.6 s for the horn) **[web, L2101]**. With a stop, the rotors halt at random angles.
  Some players want the horn to stop facing front; the 2101 and setBfree offer a "brake position"
  **[web, SBF]**.

## 6. Acoustics

### 6.1 Doppler [paper, SSAB02; calc]

- **The far-field formula.** "In the far field, a rotating horn causes an approximately sinusoidal
  multiplicative frequency shift, with the amplitude given by horn length r_s times horn angular
  velocity ω_m divided by sound speed": ω_l ≈ ω_s[1 − (r_s ω_m / c)·sin(ω_m t)] **[paper,
  SSAB02]**.
- **The peak is the same near the cabinet.** It is the tangential speed. Near the cabinet the FM
  waveform is no longer a sinusoid, and the AM from distance alone is large **[calc]**:

| r (m) | Mic distance from axis | Peak FM, fast 6.7 Hz | Peak FM, slow 0.8 Hz | FM 2nd harmonic / 1st | Distance AM alone | Delay swing |
|---|---|---|---|---|---|---|
| 0.15 | 0.42 m | +31.6 / −32.2 c | ±3.8 c | 0.18 | 6.5 dB | 0.875 ms |
| 0.19 | 0.30 m | +39.9 / −40.8 c | ±4.8 c | 0.30 | 13.0 dB | 1.108 ms |
| 0.19 | 0.42 m | +39.9 / −40.8 c | ±4.8 c | 0.22 | 8.5 dB | 1.108 ms |
| 0.19 | 1.0 m | +39.9 / −40.8 c | ±4.8 c | 0.09 | 3.3 dB | 1.108 ms |
| 0.19 | 3.0 m | +39.9 / −40.8 c | ±4.8 c | 0.03 | 1.1 dB | 1.108 ms |

- **The drum at r = 0.2 m** would have the same cents at 5.9 Hz (~±36 c). But its low frequencies
  radiate from the whole drum and cabinet (wavelength 0.4-1.7 m against a 0.2 m radius), so only
  its rotating fraction carries FM. That fraction rises with frequency (§6.3) **[calc; design]**.
- **The cabinet's reflections each have their own Doppler shift.** "The listener hears multiple
  reflections at different Doppler shifts, giving a kind of chorus effect" **[paper, SSAB02]**.

### 6.2 The horn's directivity

- **Henricksen [paper, Henricksen]:**
  - Polar plots with and without the diffuser: "without … typical of a 'beamy' straight horn";
    "with … almost omnidirectional; note, however, the 'lobing'. As the horn revolves, the sound
    will actually rise and fall a number of times".
  - The coverage angle narrows with frequency without the deflector.
  - "Leave the deflectors in place and the result will be maximum frequency modulation, and a
    relatively lower amplitude modulation … Take the deflectors out though, and the FM will be
    lost, but you obtain very strong AM, especially at higher frequencies".
- **Smith, Serafin, Abel, Berners [paper, SSAB02]:**
  - The Leslie 600 horn was measured in free field at −180° to 180° in 15° steps (Golay codes, 256
    taps used), modelled with 5 principal components.
  - "Below 12 Barks or so, the angle-dependence is primarily to decrease amplitude as the horn
    points away from the listener, with high frequencies decreasing somewhat faster with angle than
    low frequencies".
  - "The beginning of each impulse response contains a fixed portion which does not depend
    significantly on the angle", thought to be "'leakage' from the base of the horn", arriving
    first.
  - The dB-vs-angle curves are in their figures only (not tabulated).
- **Herrera, Hanson, Abel [paper, HHA09 via JAES]:**
  - A Leslie 44W measured in its cabinet with a spaced pair of B&K 4006 at 0.9° horn steps and
    2.7° baffle steps.
  - A dense tail of reflections in each response.
  - About **20 fixed filters**, crossfaded by angle, were enough. Interpolating irregular
    responses gave a "hashy" sound above 6 Hz (spatial aliasing), cured by spatially band-limiting
    the filters.
- **[design]** D(ψ, f) for the horn, ψ the angle between its axis and the path:
  - **Low band (< 800 Hz):** 0.7 + 0.3·cos ψ (≈ −4.4 dB at the back).
  - **Mid (800 Hz-2 kHz):** a cardioid (≈ −10 to −15 dB at the back).
  - **High (> 2 kHz):** cardioid^1.5, plus diffuser lobing a·cos(4ψ) with a ≈ 0.15 (±1.3 dB).
  - **Without the diffuser:** high band cos^4(ψ/2)-like (beamy), and r_h falling with f.
  - **Base leakage:** a fixed, angle-independent path at −12 dB (a guess; Smith's figure shows a
    clear early component).
  - Fit all of it to any measured Leslie response the user can supply.

### 6.3 The drum's directivity [paper, Henricksen; design]

- **Split by frequency:** the drum's output is a fixed part plus a rotating part, with weight
  w(f) = 0 below 150 Hz, rising to ~1 by 800 Hz (a first-order crossfade centred near 300 Hz).
- **The rotating part:** a scoop beam of roughly cardioid shape (front/back 10-15 dB at 600 Hz).
- **The fixed part:** leakage through the cabinet and the middle compartment's vent, omni.
- **This gives** Henricksen's 200-800 Hz "throb", nearly pure AM, with the small FM he hears near
  800 Hz.

### 6.4 Cabinet reflections, louvres and resonances

- **The louvres.** The horn sits in the top compartment (743 mm wide × 524 mm deep inside, less
  the walls). Its sound leaves through the louvres (and any open back). So what a microphone hears
  is the sum of the louvre openings, each excited mostly when the horn points at it, plus
  reflections **[paper, Henricksen; calc]**.
- **Delays [calc]:**
  - At the closest approach the mouth (r ≈ 0.17-0.19 m) is ~0.05-0.2 m from the side walls and
    louvres.
  - So the first reflections are **0.3-3 ms** behind the direct sound.
  - The compartment's axial modes are near **231 Hz (width) and 327 Hz (depth)**, and the height
    mode (~0.3 m tall) is ~570 Hz. These are damped by the louvres and the open back.
- **Smith et al.:** "the first few early reflections are simply handled as additional sources …
  a 'Leslie cabinet' reverberator for handling later reflections more statistically" **[paper,
  SSAB02]**.
- **Herrera et al.** measured the reflections directly, in the IRs **[paper, HHA09]**.
- **The bass compartment:** a box with louvres. The middle compartment is the woofer's vented
  enclosure **[paper, Henricksen]**. Its port tuning was not found **[to measure]**.

## 7. The amplifier (122)

- **Tubes and circuit:**
  - Two **6550** beam tetrodes, push-pull, **cathode-biased**.
  - One **12AU7** as push-pull driver. In the 147 it is a long-tailed-pair phase splitter
    **[web, Uneeda]**.
  - Half of a 12AU7 also works the speed relay in the 122 **[web, Uneeda]**.
  - "Cathode feedback from the output transformer (sometimes)" **[web, Uneeda]**.
- **The input.** Balanced, ~10 kΩ bridging. "Input impedance varies with the volume control
  setting". The organ's DC control voltage sits common-mode on both audio wires; the balanced
  input rejects it **[web, Uneeda]**.
- **The cable.** Pins 1 and 6 balanced input, 2 ground, 3 and 4 120 VAC, 5 the DC source
  (~295-310 V through 10 kΩ) **[web, Uneeda]**.
- **Power.** 40 W **[paper, Henricksen; web, Hammond]**; Uneeda says "about 60 W". Plate supply
  "over 400 VDC" **[paper, Henricksen]**.
- **The 147's drive level.** "Any source capable of delivering over 6 V into a load of 2.5 kΩ … will
  be able to drive the Leslie amp to full power" **[paper, Henricksen]**.
- **The character.** "When you really lean on it, it sounds even lovelier, with a warm rich
  distortion like nothing you've heard. The show organist at Circus-Circus, Las Vegas … runs his
  Hammond B3 on 10, and just keeps the hammer down" **[paper, Henricksen]**.
- **The schematic.** The Leslie 122 amplifier schematic is on elektrotanya behind a download gate
  ("leslie_122_142_amplifier_sch.pdf"; also 122R, 122XB) **[web, ELT]**. It was **not read**, so
  no component values are given here.
- **[design] The model:**
  - Driver: a grid-current soft clip on the 12AU7 (asymmetric).
  - Output: a push-pull 6550 pair as a symmetric soft saturator, with crossover rounding near zero
    and a **cathode-bias shift**. The cathode capacitor charges under heavy drive, shifting the
    bias toward class B and compressing. Time constant ~0.1-0.5 s **[memory; to check against the
    schematic]**.
  - **Supply sag:** the rectifier and reservoir, τ ~50-200 ms.
  - **Output-transformer** low-frequency saturation on big pedal notes ("supply it with 40 watts of
    a 30 Hz pedal tone and it shakes a lot" **[paper, Henricksen]**).
  - **The crossover is passive**, after the amp: 12 dB/oct at 800 Hz, loaded by 16 Ω drivers.
    Distortion products of bass notes above 800 Hz therefore go to the **horn**. That is part of
    why an overdriven Leslie "spins" its distortion.

## 8. Microphones and stereo

- **Practice [paper, Henricksen; web, Strymon]:**
  - "An RCA 77DX ribbon at back of the top rotor cabinet, about 8 inches from the treble horn."
  - Or "a pair of tube U67s, located about 5 feet away from the Leslie, aimed midway between top
    and bottom rotors, and spaced 10 feet apart".
  - Typical modern rock/jazz miking: "a pair of mics at the top of the cabinet at the horn and a
    single mic at the bottom to pick up the drum" **[web, Strymon]**.
- **setBfree** uses a 42 cm mic distance and a horn mic angle of 180° **[web, SBF]**. Smith et al.
  model "two fixed listeners [that] correspond to two ears or two studio microphones" **[paper,
  SSAB02]**.
- **[design] Default stereo:**
  - Two horn mics at ±60° from the cabinet's front axis, 0.4 m from the horn axis, panned L/R.
  - One drum mic at the front, 0.3 m, centre.
  - An alternative "room pair": two mics at 1.5 m, 3 m apart (the U67 setup).
  - Output: mid = (L + R)/2 to the model's main signal, side = (L − R)/2 through `takeSide`, then
    `src/room.*`.
- **Phase between the two horn mics.** Each mic hears the Doppler with its own phase, offset by the
  angle between the mics, so the stereo image swirls. With a 120° separation the two FM curves are
  a third of a cycle apart **[calc]**.

## 9. The interface to the organ

- **Signal.** The organ's AO-28 output transformer T3 feeds the 122 a balanced line-level signal
  (`hammond_b3.md` §10.1). The model passes the console signal (volts) into the Leslie amp's input
  stage, where the 122's volume control sets the drive **[design]**.
- **Speed switching** **[design; web, L2101]**:
  - **Slow/fast:** a Hammond half-moon switch (or the older separate switch) puts or removes the DC
    on the audio pair. In MIDI: **CC1 ≥ 64 → fast, < 64 → slow**, and **CC64 → fast while
    pressed** (the 2101's default slow/fast control code is 64, latching). The last one received
    wins.
  - **Stop (brake):** optional. The 122 has two speeds; a "stop" needs a third switch position
    (common modification). Map it to CC1 = 0 held for 1 s (the 2101 stops the rotors on a foot
    switch held > 1 s).
  - **Speed-change curves** come from §5.2, never instant.
- **One Leslie per organ instance**, shared by all notes. Per-note processing is not physical.

## 10. The literature

| Work | What it gives | Used here |
|---|---|---|
| Henricksen, "Unearthing the mysteries of the Leslie cabinet", *Recording Engineer/Producer*, Apr. 1981 | construction, crossover, amp, horn polar effects, drum = AM 200-800 Hz, mic setups | throughout |
| Smith, Serafin, Abel, Berners, "Doppler simulation and the Leslie", DAFx-02 | the moving-source/listener delay-line method; Leslie 600 horn measurements, PCA, base leakage | §6.1, §6.2, §12 |
| Herrera, Hanson, Abel, "Discrete time emulation of the Leslie speaker", AES 127th Conv., paper 7925 (2009) | 44W IRs at 0.9°/2.7° steps with a spaced mic pair; ~20 fixed filters; measured spin-up/down; spatial aliasing above 6 Hz | §5, §6.2 (via the JAES review; full paper not read) |
| Pekonen, Pihlajamäki, Välimäki, "Computationally efficient Hammond organ synthesis", DAFx-11 | the Leslie as time-varying spectral delay filters | an alternative to delay lines |
| Werner, Dunkel, Germain, DAFx-16 | the organ's vibrato line (not the Leslie) | `hammond_b3.md` §9 |
| Dobbins, "A new approach to simulating the Leslie speaker" (IoA) | a cardioid-type rotating beam from a fixed array | a directivity sanity check (abstract only) |
| setBfree / x42-whirl (F. Kilander, R. Gareus) | an open, widely used model: speeds, time constants, radii, filters (drum 812 Hz, horn 4.5 kHz/300 Hz) | §4, §5 |
| Leslie 2101mk2 manual (Hammond-Suzuki) | the maker's own digital Leslie: speeds, rise/fall/brake times, 0.7/0.8/1.0 kHz crossover, "122" cabinet character | §4, §5, §9 |

- **Not found:** a free-field or in-cabinet directivity table in numbers, a published horn
  rotation radius, the drum's dimensions, and the 122 amp's component values (schematic gated).
  "Werner" and "Holters" on the Leslie itself: no Leslie paper by them was found. Werner's Hammond
  work is the vibrato line.

## 11. The user's recordings [rec]

- **No Leslie found.** 26 tracks of the user's music likeliest to hold a Hammond/Leslie were split
  with Demucs `htdemucs_6s`. They were screened for envelope lines at 0.5-1.2 Hz and 4.5-8 Hz that
  are not tempo harmonics and that differ between below and above 800 Hz (drum ≠ horn).
- **The one candidate failed.** St Germain's *Sure Thing* (5.60 Hz) is a rhythmic loop: 7 × the bar
  rate, identical in both bands, steady for six minutes.
- **Details:** `hammond_b3.md` §12.2. Speeds, ramp times and AM/FM depths therefore come from §4-§6
  only.

## 12. Model plan [design]

### 12.1 Components

1. **Input and amp:**
   - the 122 input (balanced; volume = drive);
   - the 12AU7 driver clip;
   - the 6550 push-pull saturator with cathode-bias shift;
   - supply sag;
   - the output transformer (LF saturation, a 40 Hz-15 kHz band);
   - the 800 Hz 12 dB/oct passive crossover;
   - driver responses: horn 400 Hz-10 kHz ±5 dB; woofer to ~40 Hz.
2. **Rotor mechanics** (×2): §5.2, integrated per block (1 ms is enough), giving φ_h(t) and
   φ_d(t), then interpolated per sample.
3. **Horn acoustics.** For each path p ∈ {direct to the louvre/mic, 3-4 early reflections (side
   walls, top, back), base leakage} and each mic m:
   - a delay line read at τ_pm(t) = |x_m − x_s,p(φ_h)|/c, with cubic (4-tap Lagrange) interpolation;
   - a directivity filter D(ψ_pm(t), f): a 3-band crossfade (low/mid/high gains by angle, as
     one-pole splits);
   - the image sources of the reflections mirror x_s across each wall, so they rotate too, with
     their own Doppler.
4. **Drum acoustics:**
   - the fixed part (a fixed delay and gain);
   - the rotating part, crossfaded in by w(f), with 1-2 paths (direct and one reflection) to each
     mic and a cardioid scoop beam.
5. **The cabinet tail:** a 4-line FDN per compartment, T60 ~30-60 ms, with modes near
   230/330/570 Hz. Fed by the horn and drum outputs, mixed into both mics.
6. **Mics:** two horn mics and one drum mic (§8), summed to L/R. Mid to the main output, side via
   `takeSide`, then `src/room.*`.

- **Moving source vs moving tap.** Smith et al. write the source side with de-interpolation for a
  moving source. A read-side moving tap is exact for a moving listener and differs for a moving
  source only in second order: M² ≈ 5 × 10⁻⁴ at 8 m/s **[calc; paper, SSAB02]**. So read taps
  are good enough **[design]**.

### 12.2 Parameters

| Group | Parameter | Default | Range / option |
|---|---|---|---|
| Speeds | horn fast / slow | 6.67 / 0.80 Hz | ±3 % per cabinet; pulley step 3 choices (±8 %) |
| | drum fast / slow | 5.90 / 0.67 Hz | 5.6-6.6 / 0.5-0.8 Hz |
| Mechanics | horn τ up / down | 0.5 / 0.8 s | 0.16-4 s ("belt age") |
| | drum τ up / down | 3.5 / 2.5 s | 1.4-12 s |
| | brake | off | horn 3 s, drum 12 s to stop; stop angle |
| Geometry | horn radius r_h | 0.17 m | 0.15-0.20 |
| | drum scoop radius r_d | 0.20 m | 0.15-0.22 |
| | compartments | 743 × 524 × ~300 mm (top) | |
| Directivity | horn low/mid/high back-loss | −4 / −12 / −18 dB | diffuser out: high −25 dB, lobing off, r_h(f) falling |
| | base leakage | −12 dB | |
| | drum rotating fraction w(f) | 0 below 150 Hz, 1 above 800 Hz | |
| Amp | drive (organ expression + 122 volume) | clean at mf, crunch at ff | "Circus-Circus" setting |
| | cathode-bias shift τ, sag τ | 0.3 s, 0.1 s | |
| Crossover | frequency, slope | 800 Hz, 12 dB/oct | 700 / 1000 Hz (as the 2101) |
| Mics | horn pair | ±60°, 0.4 m | room pair at 1.5 m; reversed cabinet with the back off |

### 12.3 MIDI

- **CC1:** ≥ 64 fast, < 64 slow. **CC64:** fast while held (latch option).
- **Stop:** optional (CC1 = 0 held 1 s).
- **CC11:** the organ's expression (which drives the amp). It belongs to the organ, not here.
- **Defaults per program** (from `hammond_b3.md` §13.3): GM 17 slow, GM 18 slow, GM 19 fast with
  high drive.
- **No per-note control:** the Leslie is one shared mechanism.

### 12.4 Costs [calc]

- **Horn:**
  - 2 mics × 5 paths = 10 moving taps × (4-tap interpolation + a 3-band split and gains) ≈
    10 × 15 = 150 FLOP;
  - the angle-dependent gains are recomputed per 32-sample block from φ_h (tables).
- **Drum:** 2 mics × 2 paths = 4 taps, plus the fixed path, ≈ 60 FLOP.
- **FDN:** 2 × 4 lines ≈ 40 FLOP.
- **Amp:** 2 waveshapers, 4 biquads, sag ≈ 40 FLOP. Oversample the amp ×2 for aliasing, another
  ~40.
- **Total** ≈ 350 FLOP per sample ≈ **0.1-0.2 µs per sample** (a few ms of CPU per second).
- **Memory:** delay lines of a few ms; a 4096-sample ring suffices.

### 12.5 Build order

1. Rotor mechanics, alone, with a test that logs φ and ω through slow→fast→slow→stop.
2. The horn with one direct path per mic, Doppler and AM: check §6.1's table.
3. The directivity bands and base leakage.
4. The reflections and FDN.
5. The drum.
6. The amp and crossover.
7. The mics and stereo.
8. The interface to the B-3 (`hammond_b3.md` §13).

### 12.6 Tests

- **Speeds:** steady horn and drum rates within 0.5 % of the set values. They differ, so a
  tremolo spectrum shows two AM rates: the horn's above 800 Hz and the drum's below.
- **Ramps:** after a step, ω reaches 63 % at τ (±10 %). Up and down are asymmetric as set, and the
  drum is slower than the horn on the way up.
- **Doppler:** a 1 kHz sine at fast speed, through the direct path only, shows FM of ±rω/c
  (±40 c at r = 0.19, 6.7 Hz) at the horn rate. At 0.42 m the FM 2nd harmonic is ~22 % and the
  distance AM 8.5 dB (§6.1).
- **Crossover:** at 800 Hz both rotors contribute −3 dB (or −6 dB, as a 12 dB/oct passive pair).
- **Drum:** a 100 Hz sine shows < 1 dB AM. A 500 Hz sine shows ≥ 6 dB AM at the drum rate.
- **Stereo:** the side signal is non-zero and swirls at the horn rate; mono (L + R) keeps the FM.
- **Amp:** THD vs drive: clean (< 1 %) at nominal, ~10 % at +10 dB drive. Recovery after a burst
  shows the bias-shift time constant.
- **Aliasing:** fast rotation with a 10 kHz sine shows no "hashy" components (cf. HHA09's spatial
  aliasing above 6 Hz).
- **CPU:** under 0.3 µs per sample.

## 13. Open questions for the user

1. **A Leslie recording:** any track or clip with a clearly audible Leslie (ideally a speed change
   from chorale to tremolo, with the organ alone) would let us fit the rates, ramp times, AM/FM
   depths and the drum/horn balance. None of your music has one (§11).
2. **Horn ramp:** fast (setBfree, ~0.5 s) or slow (Hammond-Suzuki 2101, 2-4 s)? A "belt age" knob
   covers both. Which default?
3. **Diffuser in or out by default?** In is the stock 122 (more FM, gentler AM). Out with the back
   removed is the rock-club sound.
4. **Mic setup:** a close horn pair plus a drum mic (studio), or a room pair (the audience)?
5. **Stop/brake:** wanted, and how to trigger it from MIDI (CC1 = 0 held, or a separate CC)?
6. **Drive:** should GM 19 drive the 122's amp hard (the Jon Lord sound), or add a separate guitar
   amp (Marshall) in front, as Lord did? That would be a separate model.
7. **147 or 122 only?** For the B-3 only the 122 matters. A 147 would let other instruments (GM 21
   reed organ, guitar) use the Leslie. Should it be a general effect?

## 14. Sources

- **[Henricksen]** C. A. Henricksen, "Unearthing the mysteries of the Leslie cabinet", *Recording
  Engineer/Producer*, April 1981 (read in full).
  https://dannychesnut.com/Music/Leslie/LeslieMystery.html (originally at
  theatreorgans.com/hammond/faq/mystery/mystery.html)
- **[SSAB02]** J. O. Smith, S. Serafin, J. Abel, D. Berners, "Doppler simulation and the Leslie",
  DAFx-02, Hamburg (read in full). https://ccrma.stanford.edu/~jos/doppler/dafx02.pdf ; the CCRMA
  pages https://ccrma.stanford.edu/~jos/pasp05/Leslie_Free_Field_Horn_Measurements.html and
  https://ccrma.stanford.edu/realsimple/DelayVar/The_Leslie.html
- **[HHA09]** J. Herrera, C. Hanson, J. S. Abel, "Discrete time emulation of the Leslie speaker",
  AES 127th Convention, paper 7925 (2009) (abstract and the JAES review only).
  https://ccrma.stanford.edu/papers/view/2568 ; review: F. Rumsey, "Digital audio effects and
  simulations", J. Audio Eng. Soc. 58(5):420 (2010).
  https://freeverb3-vst.sourceforge.io/doc/JAES_V58_5_PG420.pdf
- **[PPV11]** J. Pekonen, T. Pihlajamäki, V. Välimäki, "Computationally efficient Hammond organ
  synthesis", DAFx-11 (abstract). https://dafx.de/paper-archive/details/Va-Hsjj4g396c-1d8ofv2w
- **[WDG16]** K. J. Werner, W. R. Dunkel, F. Germain, "A computational model of the Hammond organ
  vibrato/chorus using wave digital filters", DAFx-16.
  https://dafx16.vutbr.cz/dafxpapers/38-DAFx-16_paper_54-PN.pdf
- **[Dobbins]** P. Dobbins, "A new approach to simulating the Leslie speaker", Institute of
  Acoustics (abstract via search; the PDF did not download).
  https://www.ioa.org.uk/system/files/publications/PF%20DOBBINS%20A%20NEW%20APPROACH%20TO%20SIMULATING%20THE%20LESLIE%20SPEAKER.pdf
- **[L2101]** Hammond-Suzuki, *Leslie 2101mk2 Owner's Manual* (rotor speeds, rise/fall/brake
  times, crossover 0.7/0.8/1.0 kHz, MIDI slow/fast code 64).
  https://im.static-thomann.de/pics/atg/atgdata/document/manual/leslie_2101mk2.pdf
- **[SBF]** setBfree man page (whirl.* defaults: rpm, acceleration/deceleration, radii, mic
  distance, filters). https://mankier.com/1/setBfree
- **[zynthian]** x42-whirl engine page (time constants = time to half speed).
  https://www.zynthian.org/engines/_engine-list/engine-x42-whirl
- **[Uneeda]** Uneeda Audio, "Leslie Speaker Interface" (122 vs 147 amplifiers, inputs, pinouts,
  relay). https://uneeda-audio.com/leslie
- **[Hammond specs]** Hammond, "Traditional Leslie Specifications" (147A/122A: 40 W, 15" woofer,
  cabinet size). https://hammondorganco.com/trad-leslie-specifications
- **[diyaudio]** "Leslie speaker spin speed" (147 measured: 400/48 rpm horn, 342/40 rpm drum, citing
  HammondWiki; a second owner 409/396 rpm). https://www.diyaudio.com/community/threads/leslie-speaker-spin-speed.277275/
- **[musicplayer]** "Leslie rotor speeds" and "Leslie speaker spin up/down times" (forum; summaries
  via search, pages blocked). https://forums.musicplayer.com/topic/84960-leslie-rotor-speeds/ ;
  https://forums.musicplayer.com/topic/115116-leslie-speaker-spin-updown-times/
- **[Monroe]** A. Monroe, "Simulating a Leslie speaker" (blog: horn 1-3 s, drum 5-9 s).
  https://adammonroemusic.com/blog/simulating_a_leslie_speaker.html
- **[muzines]** *Keyboard Talk* magazine archive (fast ~7.5 Hz, chorale ~0.5 Hz).
  https://www.muzines.co.uk/articles/keyboard-talk/6155
- **[Strymon]** Strymon, "Rotary speaker technology white paper" (mic practice; qualitative).
  https://www.strymon.net/rotary-speaker-technology-white-paper/
- **[Yamaha]** Yamaha, "Rotary speaker effect" (belt slip with age).
  https://yamahasynth.com/learn/2010s/rotary-speaker-effect-xf/
- **[Low-Pro]** Leslie-style Low-Pro manual note (times change with age and service), via search.
  https://cf3.zzounds.com/media/lp-157e-361577612f60198be3373f22d3538eaf.pdf
- **[ELT]** elektrotanya.com, Leslie 122/142 amplifier schematic (gated; not read).
  https://elektrotanya.com/leslie_122_142_amplifier_sch.pdf/download.html ; 122R:
  https://elektrotanya.com/leslie_122r_tube_amplifier_sch.pdf/download.html

## 15. Built (2026-11-17)

`src/leslie.h`, `src/leslie.cc`, owned by the Hammond (`hammond_b3.md` §16).

**Simplified from §12 [design]:**
- **The amp:** an asymmetric tanh, its drive eased by a 0.1 s sag.
- **The crossover:** 2nd-order Butterworth at 800 Hz.
- **The horn:**
  - three bands (one-pole splits at 1.5 and 4 kHz), each a delay line;
  - to each mic, the direct path and the two side walls' images (mirrored, turning the other
    way), each read with 4-point Lagrange at its moving distance;
  - each weighted by the mouth's back-loss −4 / −12 / −18 dB in (1 − cos ψ) / 2;
  - plus the base's leakage, a fixed 1.25 ms path at −12 dB.
- **The drum:** below 200 Hz a fixed path; above, the scoop's cardioid (0.3 + 0.7 (1 + cos ψ) / 2)
  with its Doppler at r 0.20 m.
- **The cabinet:** a 4-line Householder FDN, T60 40 ms.
- **The mics:** a pair ±60° off the front at 0.4 m: left and right; the mid to the main output, the
  side via `takeSide`.
- **The rotors:** first-order toward the set speed, up and down with their own time constants
  (horn 0.5 / 0.8 s, drum 3.5 / 2.5 s), and a brake (`setBrake`, horn 1 s, drum 4 s, not yet on a
  MIDI control).

**Measured:** the fast horn's AM at 6.77 Hz, 18 dB p-p in a 1-1.2 kHz band at the mics.

**Not yet:** the per-cabinet louvres and top reflections; a drum mic; the room fed from directional
points; the 122's transformer saturation; the half-moon's stop and the brake on MIDI; motor and
belt noise.

**The cabinet's imperfections (2026-11-17, the user's request) [design]:**

| Flaw | Physical cause | Model | Size (age 1) |
|---|---|---|---|
| Uneven rotation within a turn | off-centre pulley, unbalanced rotor | speed × (1 + e cos(φ + a)) | horn e ~3 %, drum ~1.2 % |
| Belt wobble | belt seam and stretch | speed × (1 + b cos φ_belt), the belt turning 2.6 (horn) / 2.3 (drum) × the rotor | 1 % / 0.6 % |
| Hunting | induction motors' slip under load and mains | Ornstein-Uhlenbeck on the target speed, τ 0.5 / 0.8 s | fast 1 %, slow 3 % |
| The dummy bell | the twin horn's sealed bell leaks | a source at the rotor's other end, facing out, with its own Doppler | −16 dB (`PJ_LESLIE_DUMMY`) |
| The horn's wind | turbulence off the spinning horn | 1.5-5 kHz noise × (ω/ω_fast)², sent through the horn | +9 dB idle, fast vs slow |
| A loose panel | a louvre or back panel touching | a resonance (150-220 Hz, Q 15-25) driven by the bass under 200 Hz; what passes its clearance, high-passed, radiated | ~33 dB under the chord's highs at a loud rock bass |
| Machinery | motor hum and rumble, amp hum, the speed relay | 120 / 28.75 / 60 Hz tones; a 4 ms 2.3 kHz click on a switch | relay +35 dB over the idle floor |
| Crossover | aged capacitors | ±8 % of 800 Hz | |
| Microphones | placement | ±4°, ±3 cm | |

The rotor's within-turn wobble could not be read back from the audio: the horn's 18 dB beam swing
breaks a pitch tracker's lock in the dips. It is simple enough to check by reading the code.

**Into the room (2026-11-17, the user: "the Leslie is not connected to the room, like mic'ed in a
dead room from close proximity") [design]:**
- **Before,** the room got the close microphones' mid as an ordinary point source. Its late field
  pulsed with the mics' tremolo, and the mics' side signal was added dry.
- **Now** (using the room's own-early hooks, `Tube::ownEarly` / `takeEarly`), the cabinet sends
  six rays (0, ±60, ±120, 180°), each the horn seen from far along that direction: its Doppler
  from the distance along it, its beam from the angle. The listener's pair (2 m apart, 4 m off)
  hears them by image paths:
  - direct and off the floor;
  - off the side walls 4 m out;
  - off the back wall 1.5 m behind the cabinet and its corners.

  The late field is fed the rays' mean.
- **The level balance.** The ensemble's level trim is the listener's alone (the room is fed
  without it), so a quiet model with a big trim sounds drier than the rest. The Hammond's output
  was raised 14 dB and its trims lowered 10.9 dB (the other 3 dB make up the 4 m distance).
- **Measured** (a held chord, fast, in the hall), room vs close mics: tail −18 vs −21 dB,
  left-right correlation 0.60; the horn band's level swing ~21 dB (the direct sound's beam).

**Inside the horn compartment (2026-11-17, the user's observations of a 122; the sources) [web,
design]:**
- **The sources.** Hammond's own description: the louvres "reflect the resonance of inside sounds,
  while simultaneously spreading the sounds outside" [web, Hammond]. A Leslie "vents the sound from
  within the box after the sound has bounced around inside" [web]. Smith and Serafin: the cabinet
  is a "multiply-reflecting enclosure", its comb filtering by delayed reflections "critical to a
  realistic simulation" of the chorus and phasing [paper, SSAB02]. Herrera, Hanson and Abel
  modelled time-varying first-order reflections off the inside walls [paper, HHA09]. The outside
  size is 41 × 29 × 21 in, height first (b3world.com).
- **The user's look.** Louvres on all four sides: of their sound ~25 % out, ~25 % turned up and
  down by the slats, ~50 % back in. The four corners are solid, thick wood. The long sides are
  narrowed in their middle (six openings a turn). Both rotors sit off-centre. The slots are ~5 cm
  apart one over another.
- **Built:**
  - each path's way out by where it meets the wall: amplitude √0.25 at a louvre; none at a corner
    post (6 cm along each wall) or the long sides' middle (±5 cm); 1 cm soft edges;
  - images off the four walls (√0.5, turning the other way) and the lid and the motor board (√0.25
    each, 0.12 m above and below, the same way);
  - the horn's axis 3 cm back and 4 cm left of the middle;
  - the highs (above 4 kHz) on the close microphones through six slots 5 cm apart, at the mics'
    elevations (+12°, −8°);
  - the close microphones moved to the sides (±80°, 0.65 m).
- **Measured** (a full-drawbar chord, fast, close microphones), against the horn alone with no
  inside, posts, offset or slots:
  - the partials' level changes 2.0 vs 1.5 dB per 10 ms, 33 % more restless;
  - left vs right above 6 kHz 1.8 vs 1.3 dB.
- **The posts.** At the microphones the posts' own fourfold and sixfold pattern is lost under the
  reflections: what a post blocks comes round off the walls. That is as the user describes the box.
- **Not found:** the rotors' exact offsets, the corner posts' sizes, the louvres' slot count and
  depth. The numbers above are the user's estimates and defaults.

