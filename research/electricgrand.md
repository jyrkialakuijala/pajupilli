# The electric grand: research and model plan

Status: 2026-11-15. Research only; nothing is built. GM program 3, "Electric Grand Piano".
SI units. This file extends `research/piano.md` (strings as banks of modes, the hammer, dampers,
the bridge exchange) and does not repeat it.

Where each number comes from is marked: **[web]** from the sources in section 9 (a qualifier says
how firm: manual, patent, maker, forum, hobby code); **[calc]** computed here from those numbers
(the scripts are short and the formulas are given inline); **[memory]** from the general
literature as remembered, not looked up for this document, to be checked before it is relied on.

## 1. Short answer

- **Which instrument.** The electric grand is the **88-key (A0–C8)** model, 1978–1985. The 73-key model
  is the smaller electric grand, **E1–E7**. "73 keys E1–E7" is right for the smaller electric grand, wrong for the electric grand.
  The CP-60M is a 76-key upright. All of them have the same idea: a real grand action, hammers
  and steel strings on a small cast-iron harp, **no soundboard**, and a **piezo-electric element
  under every note** (88 on the electric grand) summed into a FET preamp with a 3-band tone control,
  a brilliance switch (electric grand), and a stereo antiphase tremolo.
- **Stringing.** electric grand: one wound string per key from A0 to F#2 (MIDI 21–42: 22 keys), two strings
  per key from G2 (MIDI 43) to C8: **154 strings [calc]**, against 243 on a concert grand. smaller electric grand:
  15 single + 58 double = **131 strings**, which is exactly the number one emulation maker gives
  **[web, GSi; calc]**. Longest (bass) string 67.9 cm on the smaller electric grand against about 2 m on a concert
  grand **[web, Wikipedia]**.
- **Why it sounds as it does** (section 4, mostly **[calc]**):
  1. The pickup reads the **force on the bridge**, not the motion of a radiating board. That force
     is the string's impedance times its velocity, so the line output is a dry, nearly flat
     spectrum of the strings, with no board resonances and no radiation roll-off in the bass.
  2. **Short, thick bass strings are 15–40 times stiffer** (B ≈ 1–2·10⁻³ against 5·10⁻⁵ on a concert
     grand). Partial 10 of a bass note is 80–160 cents sharp. This is the "clank" and the bell-like
     bass.
  3. **Two strings per key, beating freely.** The bridge sits on a piezo stack and the frame, with
     an impedance thousands of times a string's, not on a board of ~1000 kg/s. So the two strings
     hardly couple: they beat as two separate tones, with no locking and no aftersound. Each
     string also keeps its energy longer (bass notes ring for over a minute **[web, Hollow Sun]**).
  4. **Hard hammers.** The electric grand operating manual's spec sheet lists the hammers as
     "rubber (urethane) + artificial leather" (OCR of the scan; see 2.2): shorter contact and more
     high partials than felt.
  5. **Wide, drifting unisons** (a hobby fit to samples: 0.4–2 cents; a fitted concert grand:
     0.03–0.08 c) and the 1970s–80s habit of running it through a chorus. Together these give the
     shimmer heard on records.
- **The premise "fast decay" is not right as stated.** The physics says the strings keep their
  energy longer than on a grand. The impression of a short, "thin" note comes from five things:
  no board build-up or early-decay colour, no room radiation, only two strings, the upper
  partials dying at the wire's own rate (about the same as on a grand), and the tone stack.
  Section 4.6.
- **Model plan** (section 7): keep `Piano`'s strings, hammers, dampers and the exact bridge
  exchange. Replace the soundboard, its 352 resonances, the 48 radiating patches and the
  microphones with a stiff bridge bar on 88 piezo springs: a few dozen beam modes, or at first a
  per-mode bridge loss. The output is Σ (pickup gain × bridge force), then the preamp chain.
  About **4 200 modes per direction to 10 kHz (6 500 to 20 kHz) [calc]**, 40 % of the grand.
  No board, no air. Expect a third to a half of the grand's CPU.

## 2. The instrument

### 2.1 Family

| Model | Years | Keys | Notes |
|---|---|---|---|
| smaller electric grand | 1976– | 73, E1–E7 | first model; tuning problems; unbalanced ¼" outputs only **[web, Wikipedia; forum]** |
| smaller electric grand | 1978– | 73 | revised action and electronics, balanced XLR outputs, stable tuning **[web, Wikipedia]** |
| electric grand | 1978–1985 | 88, A0–C8 | balanced XLR + ¼" outputs, adds a 3-way Brilliance switch **[web, manual; forum]** |
| smaller electric grand / electric grand | early 1980s **[memory]** | 73 / 88 | 7-band graphic EQ instead of bass/mid/treble **[web, Wikipedia]** |
| smaller electric grand / electric grand | end of 1985 | 73 / 88 | MIDI out **[web, Wikipedia]** |
| CP-60M | mid-1980s **[memory]** | 76 | upright variant with MIDI **[web, Wikipedia]** |

- smaller electric grand price at launch: $4 000 **[web, Wikipedia]**.
- **smaller electric grand compass.** One collector page says "F1 to E7", but that is 72 keys, so it cannot be
  right. E1–E7 rests on four pieces of evidence: one maker calls the CP73's E-to-E layout "similar
  to the tine piano and smaller electric grand"; an encyclopaedia page gives E1–E7; an owner says "single string up to
  the second F#" (E1..F#2 = 15 keys); and a Swiss dealer says "one string on each of the first 15
  keys". 15 single + 58 double = 131, GSi's string count **[web; calc]**.
- **Note names.** the piano-modelling forum writes the electric grand's single-string limit as "F#1 (midi 42)". That is
  one maker's octave numbering (C3 = MIDI 60). In scientific pitch (C4 = 60, as in this project)
  it is **F#2**.

### 2.2 Action and hammers

- Real grand action ("GP action" in the electric grand spec), wooden keys, a damper pedal. The keyboard
  and action box is one half, the harp, strings and dampers the other **[web, manual; dealer]**.
- **Hammers: "Rubber (urethane) + artificial leather"** (electric grand operating manual, spec item 3, as
  read from the OCR text of the scan by two independent readers: this page and the electricgrand-model
  project) **[web, manual OCR]**. One piano technician mentions keeping a hammer voicing
  fluid (Escaine) for "this model's hammers" (a smaller electric grand) **[web, forum]**: not settled. Either way the
  hammers are harder and lighter than a concert grand's felt **[memory]**.
- Hammer masses and strike points are not published. A baby-grand action suggests 8–9 g in the
  bass to 4–5 g in the treble **[memory]**. The electricgrand-model project uses 4.6 g at C4 **[web, hobby
  code]**.

### 2.3 Strings and scale

- **Single/bichord:** see section 1. No trichords anywhere **[web, the piano-modelling forum, Hollow Sun, forum,
  dealer]**.
- **Bass string length:** "26¾ inches (67.9 cm)", against about 7 ft (213 cm) on an acoustic
  grand **[web, Wikipedia; it says CP-70]**. The electric grand also has to reach A0 and its harp is wider
  (1460 mm); its longest length is not published. 0.7–0.9 m is my guess **[calc from the
  dimensions]**.
- The strings are special: "each iteration used a different type of string, although some of
  the treble strings can be replaced with standard ones" **[web, Wikipedia]**. Two US makers
  supplied them as of 2019. The 15 singles of the smaller electric grand are "large round-wound strings"
  **[web, GSi]**. No published gauge table was found (the service manual and parts list exist as
  scans, see 5.2).
- **What a 68 cm bass string has to be [calc]** (μ = T/(2Lf)², copper winding at 0.8 fill,
  B = π³E d_core⁴/(64 T L²), longitudinal f_L = √(E A_core/μ)/(2L)):

| String | L (m) | T (N) | μ (kg/m) | Outer Ø (mm) | B | f_L (Hz) | Z = √(Tμ) (kg/s) |
|---|---|---|---|---|---|---|---|
| smaller electric grand E1, core 1.5 mm | 0.679 | 600 | 0.19 | 5.8 | 1.8·10⁻³ | 1000 (24 f0) | 10.7 |
| electric grand A0, L 0.68 | 0.68 | 500 | 0.36 | 8.0 | 2.1·10⁻³ | 730 (27 f0) | 13.4 |
| electric grand A0, L 0.80 | 0.80 | 600 | 0.31 | 7.4 | 1.3·10⁻³ | 670 (24 f0) | 13.6 |
| electric grand F#2 (last single) | 0.66 | 700 | 0.047 | 2.9 | 9·10⁻⁴ | 1800 (19 f0) | 5.7 |
| concert grand A0 | 2.0 | 1300 | 0.11 | 4.4 | 5·10⁻⁵ | 390 (14 f0) | 11.8 |

  The same impedance as a grand's bass string, at half the tension and three times the mass per
  length. An 8 mm A0 string is near the practical limit, hence the electric grand's probably longer A0.
  The hobby electricgrand-model fit to recorded samples lands at **B = 2.07·10⁻³** at its lowest anchor
  **[web, hobby code]**, which agrees.
- **The tenor is the awkward part [calc].** A plain G2 at 0.66 m and 1.15 mm carries only 136 N at
  131 MPa, a weak, floppy string with B ≈ 3·10⁻³. So the bichords just above F#2 are almost
  certainly wound too, up to somewhere around C3–C4. Not confirmed **[calc; open]**.
- **Treble:** plain steel at 50–70 % of breaking stress fixes f·L ≈ 150–180 m/s. From about C5 up the
  CP can and probably does use grand-like lengths (C5 0.30 m, C8 about 5 cm). There B is like a
  grand's: 1.6·10⁻³ at C5, 2–3.5·10⁻² at C8. Between C3 and C4 the shortened plain strings
  (C4 at 0.50–0.55 m against 0.70 m) have **2–3× a grand's B** (6–8·10⁻⁴ at C4 against
  2.7–2.9·10⁻⁴) **[calc]**.
- **Tuning stretch.** The electric grand manual prints a "Tuning curve of the electric grand", a Railsback-type
  stretch **[web, manual OCR; values not legible in the text]**. From B [calc]: an octave tuned
  4:2 over a bass note with B = 1·10⁻³ is 12 c wide; with 2·10⁻³, 23 c; tuned 6:3, 26 and 51 c. Expect a
  much bigger bass stretch than a grand's: tens of cents per octave in the lowest octave.
- **Unisons:** "measured 0.4–2.0 cent" split per bichord (electricgrand-model, from a commercial sample
  set) **[web, hobby code]**. The grands in piano.md fit to 0.03–0.08 c. Touring instruments went
  out of tune enough to earn a "honky-tonk" reputation **[web, Hollow Sun]**.

### 2.4 Frame, bridge, no soundboard

- A cast-iron harp, lighter than an upright's. No soundboard; the "belly" has the pickups
  instead **[web, collector page; forum]**. Wikipedia: "piezo-electric pickups mounted on the harp
  casting". Repair and forum posts: "individual piezos embedded in the bridge", "under each
  string" **[web, forum, Hollow Sun]**.
- Charles Helpinstill (who made pickups for pianos and fitted his own to one smaller electric grand, "with no real
  improvement"): "The fact that the smaller electric grand has no soundboard pretty much confines it to a unique
  sound" **[web, Hollow Sun]**.
- Unamplified it "sounds like a muffled traditional piano" **[web, forum]**. Some acoustic output
  comes from the bridge, harp and case; it is quiet.
- **Tuning stability:** the smaller electric grand and later are "very stable"; with no soundboard, humidity
  and transport hardly move the pitch. The first smaller electric grand had tuning problems **[web, Wikipedia]**.
- **Mass and size (electric grand manual):** width about 1460 mm; two weights printed, 76 kg and 66 kg,
  presumably the two halves: about 142 kg in all **[web, manual OCR; calc]**. Wikipedia: about
  140 kg for an electric grand; Hollow Sun: up to 313 lb (142 kg). smaller electric grand: about 75 kg
  (165 lb) **[web, collector page]**.
- **Dampers:** a normal grand damper system. On the smaller electric grand the top 8 keys (16 strings) have no
  dampers **[web, GSi]**. electric grand: not found. A grand leaves 18 keys undamped; the top 8–18 is a
  reasonable assumption **[memory]**.

## 3. Pickups and electronics

### 3.1 The pickups

- **One piezo element per note: "Piezo-electric elements 88 nos. Independent pickup system"**
  (electric grand owner's manual spec, quoted in a search result) **[web, manual]**. The electric grand block
  diagram reads "88P (SIGNAL OUTPUTS) → BUFFER AMPLIFIER" **[web, manual OCR]**. The bichord's two
  strings share one element.
- **All in parallel into a FET preamp:** "73 piezoelectric crystals in parallel which are fed into
  a discrete FET preamp" (smaller electric grand owner on a recording forum; seen only as a search snippet)
  **[web, forum]**. FETs appear in the smaller electric grand schematic **[web, repair blog]**. The pickups' signal
  is "about the same strength as a guitar pickup" **[web, repair blog comment]**.
- **What they sense [calc, from the construction].** A ceramic element in thickness mode under a
  string bearing reads the **vertical force** of the string(s) on that bearing: the string's slope
  times tension, T·∂y/∂x, plus the static downbearing load. It barely reads the horizontal
  polarisation. Longitudinal force (the tension change ΔT) reaches it through the downbearing
  angle (ΔT·sin α, with α about 1–2° **[memory]**) and through the bridge rocking.
- **Where.** One maker's own patents show two arrangements (section 5.1):
  - US 4,189,969 (1976): a sealed per-string resin block whose crest carries the string, set near
    the hitch pins, with the string "in pressure contact with a bridge and the crest". This is
    the form that matches "88 independent elements".
  - US 4,211,139 (1977): the bridge on a duralumin plate held by urethane strips, one element per
    5–6 notes, built to make the decay "very close to that of the natural piano".

  Which one each CP model uses is not stated anywhere I found. The electric grand's 88 elements point to
  the first.
- **Why piezo:** magnetic pickups sit at one point on the string and carry its harmonic notches
  (and do not work on wound copper). The bridge piezo sees all partials, and with no board it
  hardly feeds back at stage volume **[web, Wikipedia; Hollow Sun]**.
- **Electrical [calc + memory].** A piezo element is a charge source, q = d₃₃·F (d₃₃ of PZT
  ≈ 300 pC/N **[memory]**). Eighty-eight elements of ~1–2 nF in parallel make a 0.1–0.2 µF source:
  each note's charge divides over all of them, and the output is proportional to **Σ F_k**. Into a
  FET input of 1 MΩ or more, the low corner is under 2 Hz: flat over the audio band. A 1 N
  dynamic force gives about 2–3 mV. The element's own mechanical resonance (a 10 mm × 1 mm disc
  under a 20 g bridge segment) is near 80 kHz, far above audio.

### 3.2 The preamp and panel (electric grand operating manual)

| Item | Value | Tag |
|---|---|---|
| Line out | 2 × balanced XLR, −20 dBm max (78 mV), 600 Ω; not affected by Volume | [web, manual OCR] |
| Jack out | 2 × unbalanced ¼", −20 dBm, 600 Ω, after Volume | [web, manual OCR] |
| Patch out / in | mono insert, −20 dBm 600 Ω / −20 dBm 100 kΩ; "mixing with tremolo" (phaser, echo) | [web, manual OCR] |
| Tone | Bass, Middle, Treble; flat at the centre detent; "each affects the entire keyboard" | [web, manual OCR] |
| Brilliance | 3-way switch High / Medium / Low: "the level of the high frequency range" | [web, manual OCR] |
| Tremolo | on/off; speed about 0.8 to 9.5–10 Hz, continuously variable; depth 16 % to 40 % (spec sheet: "less than 15 % min", "more than 40 % max") | [web, manual OCR] |
| Stereo | with tremolo on, left and right "with 180 degrees phase difference" (an auto-pan) | [web, manual OCR] |
| Power | 5 W (120 V) | [web, manual OCR] |

- The **stereo is the tremolo**: without it the two outputs carry the same mono sum **[calc,
  from the description]**. No stereo split by register was found.
- The tone-stack and brilliance corner frequencies are on the circuit diagram (a scan) and not
  in any text I found. electricgrand-model's guesses: shelves at 150 Hz and 3 kHz, a peak at 800 Hz;
  brilliance corners 3.3 / 4.4 / 8.0 kHz, "not a transcription" **[web, hobby code: guesses]**.
- **The preamp colours the bass.** Owners who bypassed it (piezos straight to the output) report
  the bass "much clearer and more natural", "less bass boom", "less CP bass growl, more punch,
  brighter midrange" **[web, repair blog and comments]**. So the stock chain (tone stack at
  centre) adds low and low-mid weight.

### 3.3 The circuit (the electric grand overall circuit diagram, 1978, read from the user's scan)

Source: "electric grand OVERALL CIRCUIT DIAGRAM 006806", one sheet (the user's
~/Documents/yamaha_cp-80_overal_circuit.pdf). Values below are read off the scan **[schematic]**;
the corner frequencies are computed from them **[calc]**. The tone network's exact topology is
partly hard to read at the scan's resolution; where a reading is uncertain it says so.

**Pickup unit.** "88 PIEZO ELECTRIC PICKUPS", one per key, all in parallel on one shielded bus
(each element from the bus to ground). There is no per-key resistor or buffer: the bus voltage is
the sum of the elements' charges over the total capacitance, so every key is weighted alike
(a passive average), except:
- **The top six keys (83 G to 88 C)** sit on their own short bus, joined to the main one through a
  **0.022 µF "mixing capacitor"**. This is a capacitive divider: flat in frequency, a fixed loss for
  those keys. With an assumed ~2 nF per element **[guess: no element capacitance is printed]** it is
  about −3.5 dB; the true value depends on the element capacitance (fit it from recordings).
- No separate damping of the piezos: their source impedance is capacitive, loaded by 470 kΩ.

**Input and first stage.**
- C101 0.1 µF in series, R102 470 kΩ to ground: a high-pass at 1/(2π·470k·0.1µ) ≈ **3.4 Hz** (lower
  still with the piezo bank's capacitance in series) **[calc]**: subsonic, the bass passes whole.
- FET1, common source (drain R104 10 kΩ, source R103 1 kΩ): a gain of roughly ×5–10 **[calc,
  FET transconductance assumed]**, feeding the tone network from its drain.

**Tone network (passive, between FET1 and FET2).** Three panel pots, each fed by its own branch
from FET1's drain, their wipers mixed through the brilliance switch into C103 0.1 µF → FET2:
- **Treble** (VR4, B10k, linear): C102 2200 pF + R107 3.3 kΩ in series from the drain: a high-pass
  branch, corner 1/(2π·(3.3k+10k)·2.2n) ≈ **5.4 kHz** with the pot fully in, up to
  1/(2π·3.3k·2.2n) ≈ **22 kHz** **[calc]**. C128 470 pF at the pot.
- **Bass** (VR2, A50k, log): fed through R116 10 kΩ and C110 0.47 µF; R134 39 kΩ to ground at that
  node.
- **Middle** (VR3, A1k, log): C111 0.47 µF into the 1 kΩ pot: a corner near
  1/(2π·1k·0.47µ) ≈ **340 Hz** **[calc]** (the reading of which end of VR3 is grounded is
  uncertain).
- **Brilliance** (three-way: High / Medium / Low): switches a 47 kΩ / 4.7 kΩ divider into the
  treble branch (and re-routes the bass/middle returns): a fixed treble cut in steps.
- At FET2's gate: R108 5.6 kΩ, C129 100 pF, C130 330 pF to ground (radio-frequency filtering; the
  330 pF with 5.6 kΩ is at ~86 kHz, inaudible).

**Gain stages.**
- FET2 + FET3: a two-FET amplifier with local feedback (R111 39 k, R112 39 k, C104 0.047 µF,
  C106 2200 pF; C105 220 µF bypass). C106 with 39 k gives a roll-off near **1.9 kHz** in the
  feedback path **[calc, if it shunts R112]**: possibly the "low-mid weight" owners describe
  (2.2); to be checked.
- VR12 (B10k, preset "B.P.") after C107 1 µF: an internal level trim.
- FET4 + FET5: a second amplifier (R121 470 k, R126 5.6 k, R127 180 k, C113 0.1 µF), out through
  C114 1 µF.

**Tremolo: an opto attenuator per channel.**
- The signal splits: R135 22 kΩ to the left path, R136 22 kΩ to the right. At each node a shunt to
  ground: a light-dependent resistor (in an MCD527 LED-LDR coupler) in series with one half of the
  **Depth** pot (VR5, C20k×2). Gain = R_sh/(22k + R_sh), R_sh = R_LDR + R_depth (then the
  100 kΩ Volume pot loads the node). Dark LDR (MΩ): gain ≈ 1; lit: down to the depth pot's
  setting. The spec's 16–40 % depth means the lit R_sh runs ~115 kΩ .. ~33 kΩ **[calc]**.
- **Oscillator** IC102 (RC4558, half 1): a **Wien bridge**, C124 = C127 = 2.2 µF, R = 5.6 kΩ +
  the **Speed** pot (VR6, B100k×2, linear, ganged). f = 1/(2πRC): **12.9 Hz** (pot 0) down to
  **0.69 Hz** (pot full) **[calc]**, matching the spec's 0.8–10 Hz; the speed knob is linear in R,
  so the frequency goes as 1/(5.6k + 100k·x). Amplitude held by two Zener diodes (WZ-050, ~5 V)
  across the output: a sine, clipped a little at its peaks. VR7 (B22k) trims the bridge's gain.
- **Antiphase**: IC102 half 2 is an inverter (R157 = R156 = 39 kΩ): the second channel's drive is
  the first one's negated, hence "180 degrees phase difference".
- **Drivers**: each phase through C123/C125 10 µF and two trims (VR8/VR10, VR9/VR11, B100k) to a
  2SC458C transistor (emitter R151/R153 2.2 kΩ) that drives the coupler's LED. The transistor
  conducts only on the half-cycle that turns it on: the LED lights for about half of each period,
  so each channel dips **once per cycle**, the dip's shape set by the LDR's own response (fast to
  fall when lit, ~ms; slow to recover in the dark, tens of ms) **[memory: typical CdS LDR]**. Not a
  sine pan: a pulsed, asymmetric one.
- **Tremolo switch** (S1/S2): breaks both drivers (the LDRs go dark: gain ≈ 1, both channels the
  same).

**Outputs.**
- Volume (VR1, A100k×2) after the tremolo nodes; per channel two FET buffers: FET6/FET7 into
  transformers LT1/LT2 (GD90025) for the balanced XLR "LINE OUT (−20 dBm) BAL."; FET9/FET8 to
  the unbalanced "JACK OUT (−20 dBm) UNBAL." (C121/C122 1 µF, R150 330 Ω).
- Front panel: "PATCH" OUT J1 / IN J2 (−20 dBm), an insert between the tone stage and the
  tremolo: effects (a chorus) go in before the tremolo.
- Power: a 12 V AC adaptor (Tamradio GA-series), regulated +B for the preamp and a separate supply
  for the tremolo circuit.

**For the model (replaces the placeholders in 7.1 item 5):**
- the parallel sum, the top six keys a few dB down through the mixing capacitor;
- a 3.4 Hz high-pass;
- the tone network from the values above (a treble high-pass branch at 5.4–22 kHz, a middle
  branch at ~340 Hz, bass on a 50 k log pot), the brilliance as three fixed treble cuts; to be
  checked by simulating the network (ngspice) once its topology is read on a sharper scan
  (see "A second reading" below: in the model now as fixed-corner shelves);
- the patch insert (chorus) before the tremolo;
- the tremolo: a Wien-bridge sine 0.69–12.9 Hz (1/(5.6k + 100k·x)), soft-clipped; per channel a
  half-wave LED drive, an LDR with fast attack and slow release, R_sh = R_LDR + depth, gain
  R_sh/(22k + R_sh); the right channel driven by the negated sine.

**A second reading of the tone network (2026-11-15, the scan at 300 dpi, zoomed).** What is
legible: R116 10 k from FET1's drain to a node from which C110 0.47 µF feeds VR2's top ("B")
and C111 0.47 µF feeds VR3's top ("M", with R134 39 k to ground); VR3's wiper is drawn to
ground; VR2's wiper is tied to its own lower end (a rheostat); VR4's top ("T", pin 3) is fed
from the drain by C102 2200 pF + R107 3.3 k, its wiper (pin 1) carries C128 470 pF to ground.
Every pot terminal leaves on numbered wires (1–7) to the **brilliance board**: deck 1's wiper
(pin 4) goes to C103 → FET2, its HIGH contact to pin 1 (the treble wiper), MEDIUM to pin 2,
LOW to the junction of a 47 k (from pin 2) and a 4.7 k (to pin 3); deck 2 (wiper pin 5,
VR3's lower end) picks pin 6 or 7 in two positions and nothing in the third. **Not legible:**
whether the line from "M" runs on to VR2's lower end and VR4 (it crosses them on the drawing
with no clear dots), and so how the three branches are summed; with VR3's wiper grounded
the reading as drawn would mute the middle branch, so something here is misread. The
network's response therefore cannot be computed. **In the model** (`ElectricGrandCircuit`): the
controls as the manual gives them (flat at the centre detent), first order, at the corners
the legible parts give: bass a low shelf at ~100 Hz (assumed), middle a peak/dip at
1/(2π·1 k·0.47 µF) = 340 Hz, 1.5 octaves wide, treble a high shelf at
1/(2π(3.3 k + 5 k)·2200 pF) = 8.7 kHz, ±12 / ±10 / ±12 dB (assumed); brilliance high flat,
medium −4 dB and low −9 dB above 1/(2π·47 k·2200 pF) = 1.5 kHz (sizes assumed; the 47 k : 4.7 k
divider only bounds them).

### 3.4 Fitted to recordings (2026-11-15)

Six recordings of electric grands in the user's music folder (four dry, two with the tremolo on), about
120 separately measurable notes (`tools/pianofit.py`; the model's notes measured the same way):

- **Tremolo** (two recordings, the L/R balance folded over a cycle): 0.92 Hz, the channels in
  exact antiphase, each dipping ~8.5 dB (L/R swing 17.4 dB), nearly square: the balance sits at
  each side and flips in ~0.1 s, one edge with a slower tail (the unit's two LDRs differ).
  Model: the LED driver switching fully over 0.20–0.64 of the swing, the LDR 11.8 kΩ lit, 1 ms
  lighting, 43 ms darkening: swing 17.1 dB, 1.1 dB rms from the recording over the cycle.
- **Bass brightness** (share of the note's energy per band): A0–D2 nearly all below 1 kHz
  (MIDI 27–32: 1–2 kHz −16 dB, 2–4 kHz −27 dB; the grand's felt gave −8, −15). Fitted: the bass
  hammers ×0.03 in stiffness up to D1, back to the rest by D#2 (−17, −30). From D#2 up the
  model matched within 1–3 dB. a commercial piano model's note that its "restored" electric grand renewed wound strings
  "which lacked brilliance" fits.
- **Bass decay**: the wound strings' partials at 0.5–4 kHz die at 10–18 dB/s (2–4 kHz faster),
  twice the model's, their lowest ones as the model's. Fitted: the wire's loss rising with
  frequency, b2 ×3 up to G#2, back to the grand's by E3 (0.5–2 kHz −11…−18 dB/s). The bridge
  support (flat above 150 Hz, and the treble's too) would not fit.
- **Open**: in MIDI 57–80 the recordings' 2–4 kHz share is ~5 dB over the model's (two sets
  agree; above 4 kHz they disagree). No hammer stiffness (×6.7: +3 dB), strike point (×0.65:
  +3 dB) or wire loss reaches it: perhaps the recordings' EQ or panel settings, perhaps a
  resonance of the pickup's mount (the duralumin plate). Left unfitted.

## 4. The sound, physically

### 4.1 Force at the bridge, not a radiating board

- For an ideal string struck at x₀, mode n's displacement is a_n ∝ v_n/ω_n, and its force on the
  end is T·(nπ/L)·a_n. Since ω_n = nπc/L, **F_n = Z·v_n** (Z = √(Tμ)). The bridge force spectrum is
  the string's velocity spectrum: flat apart from the hammer's roll-off and the strike-point
  comb sin(nπx₀/L) **[calc]**.
- A grand's sound is the board's radiation: pressure ∝ board acceleration × radiation efficiency.
  Below ~100–200 Hz the board radiates poorly; above, its modes and its impedance (1000 kg/s
  falling to 200–300) colour every note (piano.md §2, §4). **The CP drops all of that.** Full-level
  fundamentals in the bass (the "boom/growl"), no formant from the board, no key-to-key
  unevenness from board modes. The pickups' own spread (sensitivity, contact) takes its place.
- The two strings of a key add *as forces at one point*, with no spatial spread, delay or
  directivity. The output is close to mono and dry (hence chorus, tremolo, amps and reverb in use).

### 4.2 No soundboard: sustain, coupling, double decay

- **Energy to the support.** A string ending on a resistance R ≫ Z loses about 2Z/R of its
  amplitude per round trip, so σ ≈ 2 Z f0 / R (1/s) **[calc]**. Grand, C4: Z ≈ 5, R ≈ 1000 →
  σ ≈ 2.6 /s, T60 ≈ 2.7 s, roughly the grand's early decay. CP, with a bridge on a piezo stack
  (k ≈ 5·10⁹ N/m, |Z| = k/ω ≈ 7·10⁵ kg/s at 1 kHz) on cast iron: σ falls by 100–1000×.
- **So the wire sets the decay.** With piano.md's fitted wire loss (σ = 3.5·10⁻⁴·f + 3·10⁻⁸·f²
  per second), T60 is 480 s at 41 Hz, 74 s at 262 Hz, 18 s at 1 kHz, 5 s at 3 kHz and 2 s at 6 kHz
  **[calc]**. Air drag and the support's real losses shorten the low end. That fits "over a
  minute" for recorded bass notes **[web, Hollow Sun]**. The upper partials die at the wire's own
  rate, as on a grand; only the low partials last far longer.
- one maker's 1977 patent states the problem from the other side: a pickup coupled straight to the
  bridge gives a damping envelope unlike a piano's and "does not allow sufficient mixing of
  vibrations from different strings". Hence their urethane-sprung plate, explicitly "a kind of
  spring and dash pot" that stands in for the soundboard **[web, US 4,211,139]**. So the real
  support is stiff but lossy, and its loss is the main thing to fit (section 7).
- **Unisons hardly couple.** Weinreich's coupled decay needs a bridge admittance comparable to
  the strings' (piano.md §3) **[memory]**. On a stiff bridge the two strings beat as independent
  tones: deep, regular beats at Δf, with no locking and no slow aftersound. Sympathetic resonance
  with the pedal down is still audible: the smaller electric grand emulation sells it as a feature **[web, GSi]**.
  It is much weaker than through a board.
- **The horizontal polarisation is nearly unheard**, since the piezo reads vertical force. A grand's
  aftersound is partly the slow horizontal swing reaching the board. On the CP that path is
  closed **[calc]**.

### 4.3 Inharmonicity: the clank

- Partial deviation in cents, 1731·½·ln(1+Bn²) **[calc]**:

| B | n = 2 | 4 | 6 | 8 | 10 | 16 |
|---|---|---|---|---|---|---|
| 5·10⁻⁵ (grand bass) | 0 | 1 | 2 | 3 | 4 | 11 |
| 3·10⁻⁴ (grand C4) | 1 | 4 | 9 | 16 | 26 | 64 |
| 1·10⁻³ (CP bass/tenor) | 3 | 14 | 31 | 54 | 83 | 197 |
| 2·10⁻³ (CP lowest) | 7 | 27 | 60 | 104 | 158 | 358 |

- From the 6th partial up, a CP bass note's partials lie between the harmonic series's slots:
  metallic, bell-like, "clunky" (forum: "the clunky bass is typical of upright and baby grand
  pianos … short strings are made thicker") **[web, forum]**.
- **Bichord beating grows with n.** A 5 % difference of B between the two strings of a key (wire,
  winding, bearing tolerance) at B = 1·10⁻³ puts partial 10 apart by n²ΔB/2·1731 = 4.3 c, partial
  16 by 11 c, even when the fundamentals agree. The upper partials then shimmer at a few Hz while
  the fundamental beats slowly **[calc]**. This is a chorus built into the strings.

### 4.4 Longitudinal motion and tension modulation

- Free longitudinal modes of the bass strings: f_L ≈ 0.7–1.0 kHz (22–27 × f0) on the CP, against
  0.4 kHz (14 × f0) on a grand A0 **[calc, table in 2.3]**. Plain tenor strings: 3.6–5 kHz.
- **Tension modulation per mm of amplitude** scales as EA/L²: about 9–12 times a grand's in the
  bass (smaller electric grand E1: ΔT ≈ 1.9 N per mm² of fundamental amplitude, against 0.16 N on a 2 m A0)
  **[calc]**. Hence stronger phantom partials (sums of transverse partials) and a larger initial
  pitch glide on loud low notes: ΔT_mean/(2T) ≈ 0.16 % ≈ 2.7 c per mm² on E1 **[calc]**.
- The piezo senses force directly at the bearing, where the longitudinal force acts. It hears
  this "ping" better than a board does, through the downbearing angle and the bridge's rocking.
  Worth stage 2 in the model.

### 4.5 Hammer

- Urethane/leather hammers (2.2): harder at small compression than felt, and probably a smaller
  exponent p (leather ≈ 1.5–2.5 against felt 2.2–3.5 **[memory]**). Short contact everywhere, so a
  strong high-partial attack that grows less with velocity than felt's: brightness at pp is high.
- The hammer meets 10–14 kg/s of string in the bass, as on a grand **[calc]**, so contact times
  are set by mass against string much as on a grand. electricgrand-model reports 5.0 ms at D#1, 3.5 ms at
  F#2 and 1.9 ms at C4, on its own fitted law **[web, hobby code]**.
- The attack dominates more on the CP than on a grand. The sum is direct: no board delay, no
  build-up of radiation in the hall. The attack transient comes through the flat pickup in full.

### 4.6 Why "bright, thin, strong attack, fast decay"

Separated **[calc, from 4.1–4.5]**:
- *Bright, strong attack:* hard hammers, a flat force readout, no board mass-law loss above
  2–3 kHz, and the Brilliance switch.
- *Thin:* two strings, not three; the strings lightly coupled, so no blooming aftersound;
  the output mono and dry; no board to colour the midrange; and the tenor's wound bichords and
  short plain strings (high B, low tension) give a less dense, more "electric" midrange.
- *"Fast decay":* not of the fundamental, which lasts longer than on a grand. The high partials
  die in 1–5 s as on any piano. With the low partials lasting, the timbre narrows quickly to a
  near-sine sustain, and players hear that as a short note. The real CP's support damping (the
  urethane springs, if present) shortens the low partials too: to be measured.
- *Chorus-like:* free beating of detuned bichords, n-dependent beating from B mismatch, and a
  chorus pedal on the records: Peter Gabriel ran his smaller electric grand through one **[web, forum]**.

## 5. What is published

### 5.1 Patents

| Patent | Assignee, date | What it says |
|---|---|---|
| US 4,189,969 | Nippon Gakki (one maker); Katayama & Murakami; priority 1976-07, granted 1980 | Pickup unit for an electric piano: piezo between electrodes, sealed in a hard epoxy block; the string rests on its crest; units at uniform spacing near the hitch pins, snapped into a board with soft urethane inserts. Prior art: a C-shaped metal channel with the crest as one electrode |
| US 4,211,139 | Nippon Gakki; Murakami; priority 1977-04, granted 1980 | Bridge (beech, 24 × 12 mm, or resin with metal crest) on a duralumin plate (3–4 mm thick, 80–100 mm wide in the bass, 50–60 mm treble) over a slot in the iron plate, held by urethane strips 3–4 mm thick "as a kind of spring and dash pot". A ring/bimorph piezo under the plate, **one per 5–6 notes**. Aim: decay like a natural piano, mixing between strings, no static strain on the piezo |
| US 4,084,473 | its maker; priority 1975 | its maker's electric piano (EP-308 family): bridge on small wood bars on rubber, metal weight to 7–10 kg "like a soundboard", 5 piezo pickups in all |
| US 3,519,721 / 3,685,384 | another maker; Martin & Stein; 1968–72 | Electropiano: one aluminium transducer rail near the anchor pins, 20–25 ceramic elements for 88 notes, rail on felt |
| US 4,044,644 | C. E. Mussulman; 1975 | 64-key soundboard-less piano, floating laminated bridge, magnetic pickups at L/8 (bass) to L/2 (top) |

No patent found that names the smaller electric grand/80 directly. The papers on pianos without boards are
spinet-era decay studies (an electrical spinet in a decay study cited by search results) and the
general soundboard literature. No measurement paper on the CP itself was found.

### 5.2 Manuals

- electric grand operating manual (12–13 pp.) and electric grand service manual (56 pp.); smaller electric grand operating manual
  (19 pp.; tuning, string replacement, voicing, regulation, overall circuit and block diagram);
  a combined parts list smaller electric grand/70B/80. All are on ManualsLib/Audiofanzine/elektrotanya as
  **image scans**. The specs quoted here come from ManualsLib's OCR text. **The string schedule
  ("designated key and string numbers"), the tuning curve and the tone-stack values are in those
  scans, not in any text found.** Reading the scans is the single best next step for exact
  numbers.

### 5.3 Emulations and models

| Product | Method | What it says about the instrument |
|---|---|---|
| a commercial piano model electric grand (the piano-modelling forum, 2007) | physical model | "Line output" (no mics), tremolo, amped, recording; a "Soundboarded" variant "with no real counterpart". An "original" and a "restored" version: the restored one renews the wound strings "which lacked brilliance". Users: "quite mellow, as was the original instrument we recorded" |
| GSi EG70 (2008, also in the Crumar Seven) | physical model | 131 strings, top 8 keys undamped, sympathetic resonance with the pedal, decay and release controls, 3-band EQ, tremolo/auto-pan; one voice per key |
| a software maker smaller electric grand V | 8-layer samples + "modelled EQ, tremolo and output stage" | an "age" detune control |
| a sample-library maker Keyscape, Hollow Sun smaller electric grand | samples from the DI | "every note recorded, over a minute at lower registers" (Hollow Sun) |
| electricgrand-model (GitHub, AGPL) | modal strings, nonlinear hammer, "piezo bridge-force readout", 2× oversampled | B = 2.07·10⁻³ at the bottom; unison split 0.4–2.0 c; frame resonators at 32, 38, 80, 170 Hz (fitted, not measured); tone stack and brilliance guessed; hammer felt law scaled 0.7 for "urethane/leather" |

All **[web]**. The electricgrand-model readme and specs were read as published; their numbers are fits to
samples, not measurements of an instrument. It is a useful second opinion, not a source.

## 6. What carries over from the grand model

| `Piano` part (src/piano.h, research/piano.md §10–11) | electric grand |
|---|---|
| Strings as banks of modes in registers of 16, two directions, `cs_/sn_` rotation, per-mode loss b2·f + b3·f² | **Keep.** Rebuild the scale table: 22 single + 66 double, short lengths, wound tenor bichords, B 1–2·10⁻³ in the bass |
| `kMaxStr = 3`, unison spread 0.03–0.08 c | 2 strings max; spread 0.4–2 c, plus a small per-string B mismatch (±3–5 %) |
| Horizontal direction (`x_[1]`, `tanH`) | The piezo reads it weakly: keep at a cross-sensitivity of 0.05–0.2, or drop for half the string cost |
| Hammer: mass on K·δ^p with a rate term, implicit solve | **Keep**, refit: harder at small δ, lower p (leather), lighter masses |
| Dampers, `engage`, top keys undamped (k ≥ 70) | Keep; undamped from about key 80 (smaller electric grand: top 8) |
| Tuning by partial matching (2:1 up, 6:3 down) | Keep the mechanism; the higher B gives the stretch by itself. Check against the manual's printed curve |
| Bridge exchange in two exact halves (`in_`, `out_`) | **Keep the mechanism**, change what is on the other side |
| Soundboard: 256 + 96 resonances, `phi_`, `loadR_/loadX_`, `tools/pianoboard.py` | **Remove** |
| Air: 48 patches, ring buffer, two microphones | **Remove.** The output is electrical |

## 7. Model plan

### 7.1 Structure

1. **Strings and hammers** as in `Piano`, with a CP scale (7.2). A new class or a `Piano` built
   from a scale and body choice; the hammer, damper and census code is shared.
2. **The bridge, stage 1 (rigid-ish):** each key's strings end on a bridge point of very high
   impedance. Fold the support into each mode as extra loss σ_b = 2 Z f0 Re(Y_b) and a small
   frequency shift from Im(Y_b): piano.md's damped-string-load trick turned round. The
   **pickup signal** for key k is the vertical end force, Σ_modes T·φ'_n(L)·y_n. That is one
   more weighted sum over the modes the bridge exchange already visits (`out_`). Unisons and
   sympathetic strings are uncoupled here.
3. **The bridge, stage 2 (coupled):** the bridge as a beam (or the two bridges as two beams) on an
   elastic foundation: 88 piezo/mount springs with loss (the urethane), and a few low modes of
   the harp. Computed offline as `tools/pianoboard.py` does, but 1-D: 32–64 modes. The exact
   two-half exchange stays as it is, with these modes in place of the board's. The pickup output
   for key k is k_mount,k × (bridge displacement at k). This gives bichord coupling, the
   pedal's sympathetic wash and the patent's "mixing". Keep every resonance below ~0.4 fs (the
   bare piezo stiffness puts one near 80 kHz). Use the *mount* stiffness, which is what gives
   the loss, and let the piezo be rigid.
4. **Longitudinal motion (stage 3):** a few longitudinal resonators per bass/tenor string,
   driven by the tension from the products of transverse modes (Bank's method, piano.md §3).
   They are read by the pickup through sin α ≈ 0.02–0.035. Stronger here than on the grand
   (4.4).
5. **Electronics, all linear and cheap:**
   - per-key pickup gain g_k (fixed random ±1.5 dB, a voicing table);
   - the parallel sum;
   - a DC block at about 5 Hz;
   - a preamp "colour": a low-mid lift of a few dB near 100–200 Hz, to match "boom/growl";
   - Bass/Middle/Treble and Brilliance: the passive network of 3.3 (read from the schematic);
   - tremolo: the opto attenuator of 3.3 (Wien sine 0.69–12.9 Hz, half-wave LED drive, LDR lag,
     antiphase), not a plain sine pan;
   - the patch loop: an optional chorus, using the BBD chorus already in `src/lead.cc`;
   - output scaled so that −20 dBm (78 mV) is the loudest chord;
   - then the project's room, or an amp-and-speaker colouring (the CP was sold with the
     A4115H: 100 W, horn plus bass reflex, 2 kHz crossover **[web, manual]**).

### 7.2 Parameters to start from

| Quantity | Bass (A0–F#2) | Tenor (G2–B3) | Middle (C4–B5) | Top (C6–C8) | Tag |
|---|---|---|---|---|---|
| Strings per key | 1 wound | 2, wound to ~C3–C4 (open) | 2 plain | 2 plain | [web] / [calc] |
| Speaking length | 0.80→0.66 m | 0.66→0.55 m | 0.55→0.16 m | 0.16→0.05 m | [calc, guess] |
| Tension per string | 500–700 N | 400–700 N | 400–650 N | 500–700 N | [calc, guess] |
| B | 2·10⁻³ → 6·10⁻⁴ | 1·10⁻³ → 5·10⁻⁴ | 6·10⁻⁴ → 2·10⁻³ | 4·10⁻³ → 3·10⁻² | [calc]; fit |
| Unison split | — | 0.4–2 c | 0.4–2 c | 0.4–2 c | [web, hobby fit] |
| B mismatch within a bichord | — | ±3–5 % | ±3–5 % | ±3–5 % | [calc, guess] |
| Hammer mass | 8–9 g | 7–6 g | 6–4.5 g | 4.5–3.5 g | [memory] |
| Strike point | 1/8 | 1/8 | 1/8–1/10 | 1/10–1/16 | [memory] |
| Bridge resistance Re(Z_b) | fit; start 2·10⁴ kg/s | 2·10⁴ | 3·10⁴ | 5·10⁴ | [calc, guess] |

- Modes per string to 10 kHz with these B: A0 95, E1 82, F#2 60, G2 53, C4 30, C5 17, C6 8, C7 4,
  C8 2. **Total 4 200 per direction to 10 kHz, 5 700 to 16 kHz, 6 500 to 20 kHz**; the grand
  is 10 300 / 14 300 / 16 600 under the same rule **[calc]**.

### 7.3 Cost

- The grand measured 8 % of a core for one note and 29 % for all 243 strings ringing (piano.md
  §11). Strings dominate; the board and air are a minority share.
- The CP: ~40 % of the string modes, one direction mostly, no board or air: **about 2–4 % for a
  note, 8–12 % with the pedal down [calc, scaled from piano.md]**. Stage 2's 32–64 bridge modes
  and the filters add little. The hammer solve is unchanged.

### 7.4 Validation

1. **Static checks:** 154 strings; compass A0–C8; single/double break at MIDI 42/43; undamped top
   keys; tuning curve against the manual's printed curve (once read).
2. **Against DI recordings.** The CP's line output is the target: no room, no mic.
   `tools/pianofit.py` already measures per-note B, partial levels, decays and tuning from
   isolated notes. Use it unchanged on smaller electric grand/80 tracks where the piano is exposed. John Cale's
   *Songs for Drella* (electric grand, mostly solo with guitar) **[web, Wikipedia]**; Genesis and Peter
   Gabriel tracks with the smaller electric grand **[web, forum; which tracks: memory]**. Use only recordings the
   user owns, measured for parameters only, as with the grand.
3. **Targets:**
   - B per key (expect 1–2·10⁻³ in the bass, versus 4–8·10⁻⁵ on the recorded grands: a factor
     the fit cannot miss);
   - beat rates per partial: a slow fundamental beat and faster upper-partial beats, growing
     with n;
   - early/late decay per partial: expect one slope per partial, long at low partials;
   - spectral share above 2 kHz against velocity (it should grow less with velocity than the
     grand's);
   - the low-end level: no radiation roll-off, so fundamentals at full level.
4. **A/B against the grand model** at the same keys, to hear the separate effects: rigid bridge
   only; then the CP scale; then hammers; then the electronics.

## 8. Open questions

- The electric grand string schedule (lengths, wire and core diameters, where the wound bichords end) and
  hammer data: in the service manual and parts list scans.
- The pickup mount: does the electric grand's bridge sit on 88 bare elements on the iron (stiff, long
  sustain), or on a sprung plate as in US 4,211,139 (lossy, grand-like decay)? The measured
  decay of low partials on a DI recording decides it.
- The tone-stack and brilliance frequencies, the preamp's own low-mid colour, and whether the
  preamp clips softly at high levels: in the smaller electric grand circuit diagrams.
- Felt or urethane/leather hammers on each model (the manual OCR and a technician disagree).
- How strongly the horizontal polarisation and the longitudinal force reach the piezo.

## 9. Sources

- Wikipedia, "Yamaha CP-70". https://en.wikipedia.org/wiki/Yamaha_CP-70
- Wikipedia, "Electric grand piano". https://en.wikipedia.org/wiki/Electric_grand_piano
- Yamaha CP-80 operating manual (scan with OCR), ManualsLib, pp. 7–11 (controls, outputs,
  specifications, block diagram). https://www.manualslib.com/manual/3398264/Yamaha-Cp-80.html ;
  also https://manualslib.com/manual/340066/Yamaha-Electric-Grand-Cp-80.html and the owner's
  manual at https://manualzz.com/doc/1417831/yamaha-cp-80-owner-s-manual
- Yamaha CP-70B operating manual (scan; contents only legible).
  https://www.manualslib.com/manual/340064/Yamaha-Electric-Grand-Cp-70b.html
- CP-80 service manual and block diagrams, listed at Audiofanzine.
  https://en.audiofanzine.com/electric-piano/yamaha/CP-80/ ; parts list CP-70/70B/80:
  https://elektrotanya.com/yamaha_cp-70_cp-70b_cp-80_parts_list.pdf/download.html
- Modartt, Pianoteq CP-80. https://www.modartt.com/cp-80 ; press release 2007-12-20,
  https://modartt.com/data/download/press/press_release_2007_12_20.pdf ; forum
  https://forum.modartt.com/viewtopic.php?id=166
- Hollow Sun, "Yamaha CP70" and the CP70 library page.
  http://hollowsun.com/nostalgia/vintage/cp70/index.html ;
  https://hollowsun.com/hs2/products/cp70/index.htm
- Genuine Soundware, Electric Grand EG70. https://www.genuinesoundware.com/?a=showproduct&b=31
- Arturia CP-70 V. https://arturia.com/products/software-instruments/cp-70-v/overview
- Spectrasonics Keyscape, Electric Grand CP-70.
  https://www.spectrasonics.net/products/keyscape/models/electric-grand-cp70/
- beneke7, cp80-model (README, spec_claude.md, spec_gpt.md). https://github.com/beneke7/cp80-model
- Vintage Synth Explorer forum, "Yamaha CP-60/70/80 series".
  https://forum.vintagesynth.com/viewtopic.php?t=75531
- SMEM (Swiss Museum for Electronic Music Instruments), Yamaha CP 70B.
  https://www.smemmusic.ch/index.php/fr/node/1473
- Curiocomp, "1975 Yamaha CP-70 Electric Grand Piano".
  https://curiocomp.com/musical-instruments/1975-yamaha-cp-70-electric-grand-piano
- R. Healey, "Yamaha CP70B repair" (preamp bypass; comments).
  https://shed.robhealey.com.au/fix/yamaha-cp70b-repair-2/
- Electric Piano Forum, "Yamaha CP-70 signal issue". https://ep-forum.com/smf/index.php?topic=9948.0
- Pro Recording Workshop forum, "Bypassing the preamp on a CP70" (seen as a search snippet only).
  https://www.tapatalk.com/groups/prorecordingworkshop/bypassing-the-preamp-on-a-cp70-t19406935.html
- Pianotech list, "Yamaha CP-70". https://groups.google.com/g/pianotech/c/q3fYMoGSiM4
- Yamaha Hub, "The rebirth of the stage piano" (CP73 E-to-E "like the Rhodes and CP-70").
  https://hub.yamaha.com/the-rebirth-of-the-stage-piano/
- Grokipedia, "Yamaha CP-70" (E1–E7; secondary). https://grokipedia.com/page/Yamaha_CP-70
- US 4,189,969 (Nippon Gakki, pickup unit). https://patents.google.com/patent/US4189969A/en
- US 4,211,139 (Nippon Gakki, pickup mechanism). https://patents.google.com/patent/US4211139A/en
- US 4,084,473 (Kawai, electric piano). https://patents.google.com/patent/US4084473A/en
- US 3,685,384 (Baldwin electropiano). https://patents.google.com/patent/US3685384A/en
- US 4,044,644 (Mussulman, piano). https://patents.google.com/patent/US4044644A/en
- K. Wogram, "The strings and the soundboard" (decay and soundboard impedance), KTH lectures.
  https://www.speech.kth.se/music/5_lectures/wogram/decay.html
- The grand model's own numbers: `research/piano.md` (§2–4, §10–11) in this repository.
