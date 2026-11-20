# Bass amps, cabinets, DI and compression for the electric basses (GM 34–38): research

Status: research 2026-11-19. Built (2026-11-19): the blocks of section 9 in `src/guitaramp.cc`
(the input's resistance, the James stack exact from its parts, fixed filters after the stages, a
per-stage swing, the output's corner, cabinets 3-5 and none, the DI with its filters, polarity, an
allpass lining it up with the mic, the mic's delay, a SansAmp-like drive, the studio compressor
with opto and FET modes) and the presets "b15-di" (GM 34, played by `-inst electric-bass`), "svt",
"slap-di" and "slap-squash" (prepared; their basses are not built). The checks of 10.6 that need no
P bass or slap are in `tools/bassamp.cc` and `tools/test.py` (`electric_bass_amp`); the 4.3
cabinets' low-passes are 5-6 % higher than written (the table's numbers are the analog filters').
Not built: the solid-state clip (6), the B-15's Ultra Low/Hi switches, the Bassman. This document covers what comes
after the bass's jack. The bass itself (strings, P and J pickups, the coil and pot values, the
techniques) is `research/bass_section.md`. The guitar amp module, its triode stages, the
Yeh–Smith tone stack, the power stage, oversampling and the pedals are `research/electric_guitars.md`
§11 and `src/guitaramp.h/.cc`. This document points to those and does not repeat them. The aim is
one amp/DI chain per GM program 34–38, built as presets and a few new blocks of `GuitarAmp` (or a
sibling `BassAmp` that shares its blocks). The GM numbers count from 1. GM 39/40 (synth basses) are
not covered.

How each number is marked (as in the two documents above):
- **[T]** read in the primary text (a manual, data sheet, schematic drawing or paper, downloaded
  and read for this document). "Measured" is said in words where the source reports a measurement.
- **[S]** from a secondary source, an abstract, a search snippet or a retailer's page: the primary
  text was not read.
- **[L]** players' and engineers' lore: a description, not a measurement.
- **[E]** my estimate, fit or arithmetic from cited values (including circuit solves run here).
- **[M]** from general knowledge, not looked up: check before relying on it.

References are short keys in brackets; the list with URLs is section 12. What could not be found
is in section 11.

**Short answer.**
- **How it is recorded.** On records the electric bass is mostly heard **direct (DI)**, or as a DI
  **blended** with a close-miked amp. Motown's sessions were almost all DI'd into a custom box
  **[S, Jamerson]**. Engineers' guides say the pros blend DI and mic **[T, White99]**, check the
  polarity between the two, and pick the setting with the most low end **[T, Owsinski13]**. Almost
  everyone compresses bass **[T, White99]**.
- **The amps.**
  - **Ampeg B-15 (1x15, about 30 W).** A 1960s studio amp: two 6SL7 gain stages, then a passive
    James/Baxandall bass/treble network. Its bass control gives ±10–11 dB at 40 Hz and its treble
    +18/−18…−30 dB at 5 kHz **[T, B15N-OM]**.
  - **Ampeg SVT (300 W, 6×6550).** The rock amp. Its James stack gives bass ±12 dB at 40 Hz. It has
    an LC "midrange" control at 220/450/800/1600/3000 Hz (+10/−20 dB) and two switches: Ultra Lo
    (+2…3 dB at 40 Hz, −10…−12 dB at 500 Hz) and Ultra Hi (+8…9 dB at 8 kHz). Total gain is 67 dB
    **[T, SVT-CL; SVP-PRO]**.
  - **Solid-state and class-D heads (GK 800RB, Markbass).** Their preamps are near-flat, with
    active EQ and voicing filters: a 500 Hz notch, a 200 Hz low cut and a >1 kHz shelf on the GK
    **[T, GK800-UA]**; a +35 Hz/−380 Hz/+10 kHz "VPF" scoop on the Markbass **[S, MB-TA503]**.
- **Headroom.** A bass amp is built to stay clean: several times a guitar amp's power, little
  preamp gain, unbypassed cathodes. When pushed, it distorts in the power stage on the attacks (the
  SVT "growl"). It is not a preamp fuzz on every note **[L; E]**.
- **The cabinet is a low-pass.**
  - SVT-810E: ±3 dB from 58 Hz to 5 kHz, −10 dB at 40 Hz **[T, 810E]**.
  - A 10" bass driver on a baffle is flat from 150 Hz to 2 kHz, rises 2–3 dB at 1–2.2 kHz, then
    falls by 30 dB by 6 kHz **[T, measured, BP102]**. A 15" falls by 30 dB by 4.5–5 kHz **[T,
    measured, CB15]**.
  - So the miked amp carries the body and the DI carries the click.
- **Loading the pickup.**
  - A 1 MΩ input and a 10 MΩ active DI differ by under 0.5 dB **[E]**, because the bass's own
    250 kΩ pots dominate the load.
  - A Jensen-transformer passive DI (140 kΩ **[T, JDI]**) takes the P's 2 kHz peak away (about
    −1.5…−2 dB at 2 kHz) **[E]**.
  - A 10 kΩ line input costs 6 dB at 1 kHz **[E]**.
- **The GM sound sets.** FluidR3's 34–38, rendered and measured here:
  - velocity changes only the level: 16 dB from velocity 40 to 120, with an identical spectrum
    **[E, measured]**;
  - "Fingered" is very dark (the 2–4 kHz band 54 dB under the strongest);
  - Pick is 5 dB louder and brighter than Finger;
  - Slap 2 is the brightest (centroid 0.4–2.1 kHz in the first 100 ms against Slap 1's 0.3–0.6).
- **What to build.** In `GuitarAmp`:
  - a DI branch with a blend and a configurable input resistance;
  - a James/Baxandall tone stack type;
  - a peaking "mid" and fixed voicing filters;
  - three bass cabinets (8×10 + D112, 1×15 + RE20, 4×10 + horn);
  - a studio compressor after the blend (LA-2A-like and 1176-like);
  - coupling corners ≤ 10 Hz for bass.

  Then one preset per GM program (section 10).

---

## 1. What the project has, and what is missing for bass

| What | Where | For bass |
|---|---|---|
| Pickup + cable + pots + amp input solved as one 2nd-order section (`connect`) | `guitaramp.cc` `connect()` | Reusable. But the amp's input is hard-coded to 1 MΩ: a DI preset needs it as a parameter (140 kΩ, 1 MΩ, 10 MΩ) |
| Compressor pedal (ratio, threshold, make-up; fixed 3 ms attack / 0.2 s release), before the preamp | `process()` | Good for a stomp compressor. Studio bass compression sits after the DI/mic blend and needs its own attack/release (section 6) |
| Triode stages (generic 12AX7 curve: knee at +1.4 V, cut-off at −2.8 V) with HP corners, Miller LP and cathode shelves | `stage()` | Usable. Bass stages need: coupling corners ≤ 10 Hz (presets now use 10–72 Hz); a per-stage swing scale (a 12AU7 or 6SL7 with an unbypassed cathode clips far later than a 12AX7) |
| Yeh–Smith FMV stack (Fender/Marshall) | `setTone()` | The Bassman's stack only. Ampeg uses a **James/Baxandall** passive network (section 3.1), a different circuit |
| Power stage: soft clip with knee 2 or 3, sag 0.1 s, resonance/presence | `chain()` | Usable. The SVT: 6550s, silicon rectifier (little sag). The B-15: 5AR4 tube rectifier (more sag) **[T, B15N-OM; M for the SVT]** |
| Output high-pass at 30 Hz (`hpOut_`) | `chain()` | Too high for bass: −3 dB at 30 Hz and about −1.6 dB at 41 Hz (E1) per pole. Use ≤ 10 Hz |
| Cabinets 0–2 (12" guitar speakers) | constructor | Bass needs its own (section 4) and a "none" (DI) |
| 4x oversampling, ADAA | | Needed only when a preset drives the stages (SVT growl, Slap 2 grit) |

---

## 2. How GM 34–38 are recorded in practice

### 2.1 Studio practice

- **DI, amp, or both.** "Many professional engineers" combine a DI with a miked amp. The DI
  supplies the bottom, so a smaller amp will do **[T, White99]**. Owsinski: DI and amp tracks are
  blended, "the balance changes the sound substantially". Using only one track is fine if it sounds
  best **[T, Owsinski13]**. A four-signal blend (DI, a low dynamic mic, a mid dynamic mic, a room
  condenser) is also described **[S, Reverb guide]**.
- **The DI box.**
  - A passive pickup wants ≥ 500 kΩ, "preferably higher than 1 MΩ" **[T, White99]**.
  - Active DIs capture the low fundamental better than most passive ones. A passive box with a
    large Jensen transformer (Radial) "performs well" **[T, Owsinski13]**.
  - A valve DI adds warmth "without obvious distortion" **[T, White99]**.
- **Polarity and time.** "ALWAYS check the phase relationship between the amp and DI". Flip the
  polarity of one and keep the setting with the most low end **[T, Owsinski13; T, White99]**. Or
  slide the mic track to line up with the DI **[S, SOS/Anderton]**.
- **Mic choice.** Large-diaphragm dynamics: AKG D112, Electro-Voice RE20, Shure Beta 52 **[T,
  Owsinski13]**. Avoid vocal dynamics with a low roll-off unless they touch the grille **[T,
  White99]**.
- **Position.** Pick the best-sounding speaker of the cabinet. Start a couple of inches from the
  cone, slightly off-centre. The centre is brighter, the edge warmer. Compare about 1 ft and 2 ft
  (more room) **[T, Owsinski13]**. White: 6–12 in. in front of the best speaker **[T, White99]**.
- **EQ in the mix.** An 80 Hz boost for weight. Character lives at 120–350 Hz. Roll off a lot of
  the top, which tames finger and string noise **[T, White99]**.

### 2.2 By style and by GM program **[L, except where marked]**

| GM | Typical record sound | Chain |
|---|---|---|
| 34 Fingered | 1960s–70s soul/pop: DI (Motown: "the vast majority of Motown recordings were DIed", into a custom tube DI **[S, Jamerson]**); 70s–90s pop/rock: DI + B-15 or SVT, blended | Jazz or Precision → DI ± B-15 → compressor (often an LA-2A or a Fairchild **[S, Jamerson: Fairchild + Pultec]**) |
| 35 Picked | Rock: a P with a pick into an SVT and an 8×10, miked, plus a DI | P → SVT → 8×10 + D112/RE20; DI blended; 1176 |
| 36 Fretless | 1970s–80s jazz-fusion and pop: a J fretless with the bridge pickup, DI-heavy, often chorus and reverb; Jaco's Acoustic 360/361 **[S, Wiki-Acoustic for the rig]** | In this project GM 36 stays on the double bass (`bass_section.md` §10) |
| 37 Slap 1 | Funk/fusion 1975–90: a J with both pickups, DI-dominant (the clank must survive), bright, scooped | J → active DI (often a bass with an active preamp) ± a solid-state head (GK) with a 4×10 + horn; 1176 |
| 38 Slap 2 | "More compressed, squashed" **[S, Soundhouse, via bass_section.md §6.4]** | as 37, heavier compression, brighter, a little SansAmp-style grit |

### 2.3 Live (for completeness)

- **The DI is taken before or after the amp's preamp** (the "pre/post EQ" DI out on most heads). A
  SansAmp Bass Driver DI is used as an "amp-in-a-box" straight into the desk **[T, BSDR]**.
- Compression live: lower ratios and less gain reduction than in the studio **[L]**.

---

## 3. Classic bass amps, with circuit values

### 3.1 The Ampeg B-15N Portaflex (flip-top), 1960s

- **Schematic** (B-15-N, 1968, drawn by J. Piazza **[T, Piazza-B15N68]**; channel one):
  - **Input:** "Bright" jack through 100 kΩ ‖ 5000 pF (0.005 µF), "Normal" through 47 kΩ; a grid
    leak of 5.6 MΩ.
  - **V1a (6SL7):** plate 470 kΩ, cathode 5.6 kΩ *unbypassed*. Ampeg's 470 kΩ plate resistor and
    higher voltages, against Fender's usual 100 kΩ, are what set it apart **[S, forum via search]**.
  - **Coupling and Ultra Low:** 0.1 µF from the plate, then 0.01 µF in series into the 1 MΩ volume
    pot. A shunt leg of 0.02 µF + 39 kΩ + 1 MΩ to ground; the **Ultra Low** switch shorts the 1 MΩ.
  - **Ultra Hi:** 500 pF across the volume pot (a bright cap).
  - **V1b (6SL7):** plate 220 kΩ, cathode 2.2 kΩ unbypassed. Then 0.1 µF into the tone network.
  - **Tone network (James / passive Baxandall):** bass pot 1 MΩ with 220 kΩ above and 22 kΩ below,
    0.001 µF and 0.01 µF across its two halves; treble pot 1 MΩ fed by 470 pF (0.00047 µF) and
    returned by 4700 pF (0.0047 µF); 120 kΩ from the bass wiper to the treble wiper (the output);
    270 kΩ from the bass wiper to ground; 270 kΩ mixing resistors from each channel into V3.
  - **Phase splitter and power stage:** V3 (6SL7GT), then two 6L6GC push-pull into an 8 Ω output
    transformer; a 5AR4 rectifier.
  - The Heritage reissue has three 12AX7s, two 6L6GCs and a 5AR4. Its "1964" channel is cathode
    bias, 25 W; its "1966" channel is fixed bias, 30 W **[T, B15N-OM]**.
- **Published control ranges** (Heritage B-15N) **[T, B15N-OM]**:
  - 1964 channel: bass ±10 dB at 40 Hz, treble ±18 dB at 5 kHz;
  - 1966 channel: bass ±11 dB at 40 Hz, treble +18/−30 dB at 5 kHz;
  - both: "flattest at the centre position".
- **The network solved here** (nodal solve of the values above; source 37 kΩ = 220 k ‖ the 6SL7's
  plate resistance; load ~500 kΩ; audio-taper pots, 9 % at noon) **[E]**:

| Setting | 40 Hz | 100 | 200 | 400 | 1 k | 2 k | 5 k | 12 k |
|---|---|---|---|---|---|---|---|---|
| both at noon (insertion) | −26.0 | −24.3 | −23.1 | −22.7 | −23.0 | −23.3 | −23.4 | −23.4 |
| bass full | −11.1 | −14.0 | −18.4 | −22.1 | −23.3 | | | |
| bass zero | −34.0 | −28.9 | −25.8 | −24.4 | −23.7 | | | |
| treble full | | | −21.0 | −17.3 | −11.9 | −7.8 | −4.6 | −3.7 |
| treble zero | | | −22.2 | −22.8 | −27.5 | −33.0 | −40.8 | −48.4 |

  - So the bass control gives +15/−8 dB at 40 Hz and the treble +18.8/−17.4 dB at 5 kHz, against
    the noon setting. That agrees with the manual's ±10–11 dB and +18 dB.
  - The stack costs 23 dB "flat", against the FMV stack's 10–20 dB (electric_guitars.md §11.3).
  - Unlike the FMV, the Baxandall is flat at noon: no built-in mid scoop. That is the main tonal
    difference from a Fender bass channel.
- **Ultra Low** (0.02 µF + 39 kΩ to ground when switched) **[E]**: above about 200 Hz the
  0.01 µF series cap is a short. The 39 kΩ then forms a divider with the plate's ~40 kΩ source:
  about −6 dB for everything above ~200 Hz, the bass untouched. A +6 dB low shelf, relative.
- **Where it distorts:** a 6SL7 with an unbypassed 5.6 kΩ cathode has volts of grid swing before
  it conducts. The 30 W power stage (with a tube rectifier) is where a B-15 breaks up when played
  loud **[E; L]**.
- **Cabinet:** a 1x15 in a closed, double-baffle Portaflex box. From 1965 a single baffle, which
  changed the response "slightly" **[S, TalkBass; Premier Guitar via search]**. No measured
  response was found (section 4.3).

### 3.2 The Ampeg SVT (1969–)

- **Specifications, SVT-CL** **[T, SVT-CL]**:
  - Bass ±12 dB at 40 Hz.
  - Midrange +10/−20 dB at 220 Hz, 450 Hz, 800 Hz, 1.6 kHz or 3 kHz (a five-position selector).
  - Treble +15/−20 dB at 4 kHz.
  - **Ultra Low:** +2 dB at 40 Hz, −10 dB at 500 Hz ("boosting the low frequencies and selectively
    cutting the mid frequencies").
  - **Ultra High:** +9 dB at 8 kHz.
  - Total system gain 67 dB at 1 kHz "with levels up and tones flat"; −3 dB at 40 Hz and 15 kHz.
  - 300 W RMS minimum at < 3 % THD into 2 or 4 Ω, with 0.4 V rms at the power amp input.
  - Tubes: 12AX7 (3), 12AU7 (2), 6550 (6). A −15 dB input for active basses.
- **The SVP-PRO/SVT-2PRO preamps** **[T, SVP-PRO; SVT2PRO]**: bass ±12 dB at 40 Hz; mid
  +12/−15 dB at the same five frequencies; treble ±12 dB at 4 kHz; Ultra Low +3 dB at 40 Hz and
  −12 dB at 500 Hz; Ultra High +8 dB at 8 kHz; Bright +7 dB at 2 kHz; a 9-band graphic (40, 90,
  180, 300, 500, 1k, 2k, 4k, 10 kHz, ±12 dB). Total gain 76 dB; −3 dB at 30 Hz and 15 kHz.
- **The 1972 SVT preamp** (Piazza's drawing **[T, Piazza-SVT72]**; channel one):
  - **Input:** "Bright" through 100 kΩ ‖ 470 pF, "Normal" through 47 kΩ; a 5.6 MΩ grid leak.
  - **V1 (12DW7):** one triode as a gain stage (plate 220 kΩ, cathode 3.3 kΩ unbypassed),
    DC-coupled into the other as a cathode follower (47 kΩ).
  - **Bass Cut / Ultra Low network:** two 150 kΩ in series with 0.002 µF to ground at their middle
    and 500 pF; 0.002 µF series; 820 kΩ / 68 kΩ. A three-position "Bass Select" switch: Ultra Low,
    normal, Bass Cut.
  - **Volume:** 1 MΩ linear, with the Ultra High switch adding a capacitor across it.
  - **V3a:** plate 47 kΩ, cathode 1.5 kΩ unbypassed. Then 0.1 µF into the **James stack:** bass
    1 MΩ with 220 kΩ / 22 kΩ, 0.001 µF / 0.01 µF; treble 1 MΩ with 47 pF / 470 pF; 120 kΩ to the
    output.
  - **The midrange:** V3b and V4 form a feedback amplifier. The 50 kΩ linear Mid-Range pot sits
    between their cathode circuits (each through 0.68 µF and 470 Ω or 620 Ω). Its wiper goes to a
    series LC to ground: a tapped inductor (part 320821-1) with 0.033 µF or 0.15 µF chosen by a
    three-position "Mid-Range Select".
  - **Driver:** V4's other half is a cathode follower; then 0.02 µF to a 6C4 cathode follower that
    drives the power amp.
  - The tube types per half (which half of the 12DW7 is the 12AX7-like one) are read from the
    standard pinout **[M]**.
- **The James stack solved here** (source ≈ 6 kΩ, a cathode-follower-like low impedance; load
  1 MΩ; audio-taper pots) **[E]**:

| Setting | 30 Hz | 100 | 200 | 400 | 1 k | 2 k | 4 k | 8 k | 12 k |
|---|---|---|---|---|---|---|---|---|---|
| noon | −22.6 | −22.4 | −22.2 | −22.0 | −21.8 | −21.7 | −21.6 | −21.5 | −21.5 |
| bass full | −4.5 | −7.6 | −11.8 | −16.9 | −21.9 | | | | |
| bass zero | −36.0 | −34.0 | −30.7 | −26.4 | −22.3 | | | | |
| treble full | | | | | −21.9 | −20.3 | −16.8 | −12.0 | −9.0 |
| treble zero | | | | | −21.2 | −20.6 | −21.4 | −24.7 | −27.5 |

  - So bass +18/−13 dB at 30–40 Hz, and treble +4.8 dB at 4 kHz, +9.5 at 8 kHz and +12.5 at 12 kHz.
    The 1972 treble is a higher shelf than the SVT-CL's "4 kHz" one, and the two circuits differ.
  - With **linear** pots at mid-rotation (the drawing marks only the volume "lin.") the same
    network would scoop 11 dB at 700 Hz. A builder must use the audio taper or the design ratio
    (9 % at noon) to get "flat at the centre".
  - The James design ratio is R1/R2 = C2/C1 = 10: 220k/22k and 0.01/0.001 µF **[E, from the
    values]**.
- **Midrange LC** **[E]**: no inductance value was found for 320821-1. With 0.15 µF at 220 Hz
  the tap would be about 3.5 H. The 1972 three-position switch probably gave 220 Hz, 800 Hz and
  3 kHz **[L]**; the later amps have five positions **[T, SVT-CL]**. A Q near 1 is a guess.
- **Ultra Low network** **[E, uncertain reading]**: two 150 kΩ in series, 0.002 µF to ground at
  the middle and 500 pF bridging. That is a bridged-T notch near 1/(2π·150k·√(2 nF·0.5 nF)) ≈ 1 kHz,
  which matches the later spec's "−10…−12 dB at 500 Hz" only roughly. Use the later spec as the
  target.
- **Where it distorts:**
  - Very little preamp gain before the volume: one 12AU7-like stage, ~13× **[E]**.
  - The six 6550s are where an SVT "growls" when the master is up **[L]**.
  - Total gain 67 dB **[T]**, against 300 W into 4 Ω = 34.6 V rms. Full power then needs about
    15 mV rms at the input with everything up **[E]**. Normal settings keep a passive bass's
    0.1–1 V peaks **[M]** well inside the clean range until the master is high.

### 3.3 The Fender Bassman, blackface/silverface (AB165, AA864…), bass channel

- **What is known:**
  - Two 12AX7 gain stages in the bass channel, sharing a 25 µF / 1.5 kΩ cathode network; a 12AT7
    phase inverter; 2×6L6GC, about 50 W; a closed 2×12 (or 2×15) cabinet **[S, fenderguru]**.
  - The bass channel is "much mellower and deeper" than the normal channel, and has a **Deep**
    switch **[S, fenderguru]**.
  - The stack has a fixed 6.8 kΩ resistor in the mid-pot position **[S, PG-Bassman]**. Its
    capacitors are disputed on forums: 0.1 µF, 0.047 µF or 0.25 µF **[S]**.
- **So:** the module's FMV stack with blackface-like values, a fixed mid (R3 = 6.8 kΩ, m = 1),
  larger bass caps (0.1 µF) and a Deep switch as a low shelf. This is [E] and not verified.
- The '59 Bassman (4×10) is in electric_guitars.md §11.3; it was a bass amp first.

### 3.4 Acoustic 360/361 (1968–)

- **What is known:**
  - The 360 is a preamp only. The 361 cabinet holds a 200 W RMS power amp and a rear-facing 18"
    speaker in a folded horn **[S, Wiki-Acoustic; bluebook]**.
  - The 2011 reissue: 400 W, 38 Hz–14 kHz, a horn above 1.4 kHz at 18 dB/oct **[S, Reverb listing]**.
  - Associated with Jaco Pastorius and John Paul Jones **[L]**.
- **Not found:** the 360's tone controls, their frequencies, and the "Variamp" and fuzz circuits.
  A preset would be guesswork; skip it (GM 36 stays on the double bass anyway).

### 3.5 Solid state: Gallien-Krueger 800RB (1980s–)

- **Controls** (from the licensed emulation's manual **[T, GK800-UA]**; the hardware's own manual
  was not read):
  - Input −10 dB pad; Volume (the first gain stage).
  - Three voicing switches: **Lo Cut** (a high-pass at 200 Hz), **Mid Contour** (a notch "at about
    500 Hz, giving a mellow round sound"), **Hi Boost** (a shelf above 1 kHz).
  - Four active bands: Bass ~60 Hz, Lo-Mid ~250 Hz, Hi-Mid ~1 kHz, Treble ~4 kHz.
  - A Boost (post-EQ gain into overdrive).
  - Bi-amp: a 300 W low amp and a 100 W high amp, with a variable crossover and separate masters.
- **Input** **[S, manual snippet]**: 1 MΩ; 2 mV rms sensitivity at normal gain; 1 V rms maximum.
- **Character:**
  - Clean, mid-forward, bright through a 4×10 with a horn: the slap and "punch" amp of the 1980s
    funk/rock **[L]**.
  - Its distortion is solid-state (op-amp and transistor clipping). It is harder-kneed than the
    triode curve **[M]**.

### 3.6 Modern class-D heads (Markbass and kin)

- **What they are:** a near-flat preamp and a 4-band EQ **[L]**, with two filters **[S,
  MB-TA503]**:
  - **VPF** ("Variable Pre-shape Filter"): one knob that boosts around 35 Hz and 10 kHz and cuts
    at 380 Hz, deeper as it turns.
  - **VLE** ("Vintage Loudspeaker Emulator"): a variable treble roll-off.
- **Usage:** players use the VPF at about noon for slap **[L, TalkBass]**.
- **Not found:** the slopes and depths.
- Clean to full power; no tube-like breakup **[M]**.

### 3.7 Bass amp against guitar amp: what to carry into the presets

| | Guitar amp (Twin, JCM800) | Bass amp (B-15, SVT, GK) |
|---|---|---|
| Preamp gain before the stack | 2 × 12AX7 at ~40× each; a cascade of four in high gain | One or two stages at ~13–37× with unbypassed cathodes; or op amps **[E, from the values]** |
| Stack | FMV: a built-in mid scoop | Baxandall (flat at noon) plus a separate tuned mid (SVT, GK); or the FMV with a fixed mid (Bassman) |
| Power | 20–100 W | 30 W (B-15) to 300 W (SVT) and more (solid state) |
| Low corners | coupling caps keep the bass *out* (a tight chug) | coupling corners of 1.6–16 Hz (0.1 µF/1 MΩ … 0.01 µF/1 MΩ) **[E, from the values]** |
| Where it distorts | preamp stages | power stage, on attacks; or a dedicated drive (SansAmp, Darkglass, the GK's boost) |
| Speaker | 12", passes to 5 kHz, presence peaks 2–3 kHz | 10" or 15", falls by 30 dB by 5–6 kHz (section 4) |

---

## 4. Cabinets and microphones

### 4.1 The 10" bass driver (as in 8×10 and 4×10)

- **Eminence Legend BP102 (8 Ω)** **[T, BP102 data sheet]**: Fs 35 Hz, Qts 0.43, Vas 91 l,
  Xmax 6.2 mm, 91.8 dB/1 W/1 m, "usable 40 Hz – 2 kHz". Its measured on-axis SPL curve, as read
  from the plot **[T, measured; E reading]**:
  - a rise of ~12 dB/oct to about 110 Hz;
  - a plateau at 92 dB from 150 to 300 Hz;
  - a shallow dip to 89 dB at 450–500 Hz;
  - a rise to 94 dB at 1–2 kHz with a peak of 95.5 dB at 2.2 kHz;
  - then a steep fall: 85 dB at 2.8 kHz, 78 dB at 3.5–4 kHz, ~60 dB at 6–7 kHz (−32 dB);
  - breakup peaks of ~66 dB at 8–12 kHz.
- **SVT-810E (8×10, sealed, two chambers)** **[T, 810E manual]**: ±3 dB from 58 Hz to 5 kHz;
  "usable low frequency (−10 dB) 40 Hz"; 4 Ω mono; 800 W; 2" voice coils; 100 dB sensitivity
  **[S]**. The Heritage version uses Eminence drivers **[S]**.
- **SVT-410HLF (4×10 + horn, ported)** **[S, retailer spec sheets]**: −3 dB from 48 Hz to 18 kHz;
  a 1" horn crossed over at 4 kHz, with an L-pad; 500 W, 4 Ω.

### 4.2 The 15" driver (as in the B-15)

- **Eminence Legend CB15 (a 15" bass driver, for scale)** **[T, CB15 data sheet]**: Fs 34 Hz,
  Qts 0.34, 98 dB, "usable 47 Hz – 3 kHz". Its curve **[T, measured; E reading]**:
  - flat at 98 dB from 150 to 400 Hz;
  - a small dip of −1.5 dB at ~600 Hz;
  - +2 dB at 1–1.5 kHz and a peak of +5.5 dB at ~2 kHz;
  - −10 dB at 3 kHz and a cliff of −30 dB at 4.5–5 kHz.
- **Ampeg's modern Portaflex 1x15s** **[S]**: PF-115HE 44 Hz–17 kHz (with a horn), 99 dB; PF-115LF
  57 Hz–3.7 kHz. The 1960s B-15's own speaker (CTS, Jensen, Eminence, Altec in various years
  **[S, Reverb listing]**): no data.

### 4.3 Proposed cabinet filters (base rate, RBJ biquads as in `guitaramp.cc`)

Each set below was checked by evaluating it at 48 kHz **[E]**. "dB" is relative to 200 Hz. The
targets come from 4.1 and 4.2, plus a close dynamic mic's proximity bump and presence (4.4).

| Cabinet | Biquads (type, f, Q, dB) | Result: 40 Hz / 58 / 100 / 450 / 2 k / 3 k / 4 k / 5 k / 6 k / 8 k |
|---|---|---|
| **3: SVT 8×10 sealed, D112 at ~5 cm off-centre** | HP 60/0.8; HP 40/0.6; peak 95/1.0/+3; peak 470/1.4/−3; peak 2000/2.5/+5; LP 2300/1.0; LP 2900/0.7; peak 4000/2.0/+3 (the mic's presence) | −11.4 / −3.3 / +1.9 / −3.1 / +4.9 / −4.7 / −11.7 / −20.4 / −27.1 / −35.7 |
| **4: B-15 1×15 ported, RE20 at ~15 cm** | HP 44/1.0; HP 38/0.7; peak 600/1.5/−2; peak 1900/3.0/+7; LP 2200/1.1; LP 2800/0.7 | −3.8 / +0.3 / +0.5 / −1.0 / +6.8 / −6.2 / −15.4 / −22.9 / −28.8 / −38.1 |
| **5: 4×10 + horn ported, mic between cone and horn** | HP 48/1.0; HP 35/0.6; peak 500/1.4/−2; peak 1800/2.0/+2.5; LP 2600/0.8 (the woofers); high shelf 3800/0.9/+12 (the horn, L-pad mid); LP 14000/0.7 | −5.5 / −0.4 / +0.4 / −1.7 / +1.6 / −0.4 / −0.7 / −1.6 / −3.5 / −8.6 |
| **−1: none (DI only)** | — | flat |

- The cone-breakup floor at 8–12 kHz (the BP102's ~−26 dB peaks) is left out. At −35 dB and below
  it would matter only for a pick's click through a miked amp. A designed 30–50 ms minimum-phase
  FIR could add it later (electric_guitars.md §11.5).
- **The low end of the miked path is the cabinet, not the amp:** the 810's −10 dB at 40 Hz means
  the miked SVT has little E1 fundamental. On record the low fundamental comes from the DI. This is
  why blends are standard.

### 4.4 Microphones

| Mic | Published | Use on bass cabinet |
|---|---|---|
| Electro-Voice RE20 | 45 Hz–18 kHz; **Variable-D: proximity effect eliminated** **[T, RE20]** | the flat choice; little change with distance **[E]** |
| AKG D112 | 20 Hz–20 kHz ±10 dB, 40 Hz–17 kHz ±3 dB **[S, D112 spec sheet]**; a presence lift near 4 kHz and a low-mid bump **[M]** | close, off-centre |
| Shure Beta 52A | 20 Hz–10 kHz, supercardioid **[S, Beta52]**; a presence peak near 4 kHz and a strong proximity bump at a few cm **[M]** | close |

- **Proximity effect** of a cardioid dynamic at 2–5 cm: +6…+12 dB at 100 Hz relative to 1 kHz
  **[M]**. It partly undoes the sealed box's low cut. Moving the mic away trades it for room.
- **Centre against edge:** the centre is brighter, the edge warmer **[T, White99; Owsinski13]**.
  For a guitar speaker the difference is 5–10 dB above 2 kHz **[M, electric_guitars.md §11.5]**.
  Here it acts on a region already 5–20 dB down.
- **Time:** sound takes 0.15 ms to travel 5 cm and 0.87 ms to travel 30 cm. A blend with an
  unaligned 30 cm mic has its first comb notch at 1/(2·0.87 ms) = 574 Hz **[E]**. Engineers align
  the two or move the mic (2.1).

---

## 5. The DI path and the pickup's load

### 5.1 Devices

| Device | Input impedance | Response | Notes |
|---|---|---|---|
| Radial JDI (passive, Jensen JT-DB-EPC) | 140 kΩ unbalanced **[T, JDI]** | 20 Hz–20 kHz ±0.2 dB; phase 3° at 20 Hz | 150 Ω balanced out; THD 0.05 %; max +21 dB **[T, JDI]** |
| Jensen JT-DB-E (the transformer) | 140 kΩ **[T, JT-DB-E via search]**; 200k:1.5k in Jensen's selector guide **[S]** | Bessel-tailored HF **[S]** | 12:1 step-down |
| Active DI (Countryman, Radial J48, Avalon…) | ~1–10 MΩ **[M]** | flat | |
| Radial PZ-DI | switchable 220 k / 1 M / 10 MΩ **[S, PZDI-bonedo]** | | a bonedo test found 220 k and 1 M thinner in the bass, more upper mid and top, and 10 MΩ "noticeably more balanced" on a passive bass; little difference on an active bass **[S]** |
| Tech 21 SansAmp Bass Driver DI | 1 MΩ **[T, BSDR]** | | see 5.3 |
| Bass amps' inputs | 1 MΩ (GK **[S]**; tube amps' 1–5.6 MΩ grid leaks **[T, the drawings]**) | | |
| A mixer's line or mic input | 1.5–10 kΩ **[M]** | | White: "an impedance mismatch and poor results" **[T, White99]** |

### 5.2 The pickup under each load (solved here)

- **Method** **[E]**: the bass_section.md §3.2/3.3 values. P: 6.4 H, 10.5 kΩ, 30 pF, pots
  ≈125 kΩ. J both: 1.65 H, 3.7 kΩ, 90 pF, pots ≈83 kΩ. The coil's L in series with its R, loaded
  by its C + the cable's C, the pots, and the input. Eddy-current losses are ignored (they lower
  the peaks further).
- **Results:** dB relative to 200 Hz.

| Pickup, cable | Load | Peak | Level at 2 kHz | 4 kHz | 8 kHz |
|---|---|---|---|---|---|
| P, 700 pF | 1 MΩ amp | 1.86 kHz, +1.8 dB | +1.7 | −7.1 | −20.2 |
| P, 700 pF | 10 MΩ active DI | 1.96 kHz, +2.4 dB | +2.3 | −6.9 | −20.3 |
| P, 700 pF | 140 kΩ JDI | none | −1.5 | −8.8 | −20.2 |
| P, 700 pF | 10 kΩ line in | none (−3 dB at 570 Hz) | −11.7 | −17.7 | −24.0 |
| P, 300 pF | 1 MΩ | 1.59 kHz, +0.2 dB | +0.1 | −3.0 | −13.6 |
| J both, 700 pF | 1 MΩ | 3.99 kHz, +4.1 dB | +1.3 | +4.1 | −7.8 |
| J both, 700 pF | 10 MΩ | 4.05 kHz, +4.5 dB | +1.4 | +4.5 | −7.7 |
| J both, 700 pF | 140 kΩ JDI | 3.45 kHz, +1.7 dB | +0.9 | +1.5 | −8.4 |
| J both, 700 pF | 10 kΩ line in | none (−3 dB at 1.3 kHz) | −5.4 | −10.4 | −16.6 |

- **1 MΩ against 10 MΩ:** under 0.5 dB anywhere. The bass's own pots (≈80–125 kΩ in parallel)
  set the damping. The PZ-DI review's audible difference is more likely a level and loading
  difference with that bass, or an active bass (see 5.1) **[E]**.
- **A JDI on a P:** the resonance is gone and 2 kHz is −1.5…−3 dB, a slightly "rounder" P. On a J
  the peak drops from +4 to +1.7 dB.
- **Into a line input:** −6 dB at 1 kHz on the P. The classic mistake, and a recognisably dull and
  weak sound.
- **For the model:** `connect()` takes the input resistance as a parameter. Presets use 10 MΩ
  (active DI), 1 MΩ (amp, SansAmp) or 140 kΩ (JDI). With an active DI *and* an amp on a splitter,
  the pickup sees both in parallel (1 MΩ ‖ 10 MΩ ≈ 0.9 MΩ) **[E]**.

### 5.3 The SansAmp Bass Driver DI (1994–), the common "amp in a box"

- **Controls and input** (v1 **[T, BSDR]**):
  - 1 MΩ input.
  - **Bass and Treble:** active, ±12 dB from unity at 12 o'clock. Boosting both gives a "mid-cut
    at 750 Hz"; cutting both gives a mid-boost there.
  - **Presence:** upper harmonics and attack.
  - **Drive:** "gain and overdrive, similar to when the output section of a tube amp is being
    pushed".
  - **Blend:** dry against the emulation; at minimum the EQ and level stay active.
  - **Outputs:** the XLR out has a −20 dB pad; the 1/4" out a +10 dB switch.
- **v2** **[T, BSDRv2]**: adds Mid (±12 dB, shift 500 Hz / 1000 Hz). The bass shifts 40 Hz /
  80 Hz.
- **Not found:** the treble's frequency, and the shape of its clipper.
- **In this project:** a SansAmp is a drive block, i.e. a pre-EQ, a soft clipper and a speaker-like
  post-EQ, with a dry/wet blend **[E]**. Build it from the existing blocks:
  - one triode-like or diode stage with a high-pass before it (so the lows stay cleaner);
  - a post low-pass ~5 kHz;
  - the blend.

### 5.4 Overdrive pedals, briefly

- **Darkglass B3K/B7K:**
  - A blend of a clean path with a clipped path. A "Grunt" switch sets how much low end goes into
    the clipper, an "Attack" switch the same for treble **[S, PG review; retailers]**.
  - The B7K Ultra has a 4-band EQ, ±12 dB, with three selectable frequencies for each of the two
    mids (500 Hz and 3 kHz quoted) **[S]**.
- **The principle for bass drive:**
  - distort the mids and highs, keep the fundamental clean (parallel or crossover), so that the low
    end does not fart out in intermodulation **[L; E]**;
  - the GM programs 34–38 do not want this, except a touch on Slap 2 and on rock picks.

---

## 6. Compression

- **What engineers do:** "most engineers compress bass", to control the peaks of slaps and pulls
  and raise the average level **[T, White99]**:
  - attack "several tens of milliseconds" to let the transient through; shorten it for less note
    emphasis;
  - release "around a quarter second";
  - a limiter after, fast, with its threshold a few dB under overload;
  - "valve amps may need little or no extra compression" **[T, White99]**.
- **The two classic studio units:**

| Unit | Attack | Release | Ratio | Use on bass |
|---|---|---|---|---|
| UREI/UA 1176LN (FET) | 20–800 µs | 50 ms–1.1 s | 4, 8, 12, 20:1 **[S, UA manual via search]** | 4:1, slow attack (the 800 µs end), fast release **[S, MF hub]**; 8:1 driven hard for a passive bass **[S]**; 1176 then LA-2A in series "the most common" **[S, forum]** |
| Teletronix LA-2A (optical) | ~10 ms | ~60 ms to 50 %, then 0.5–5 s (program-dependent; "1–15 s" elsewhere in the original manual) **[S, UA; VintageKing]** | ~3:1 in Compress **[S]** | smoothing, 3–6 dB |
| General starting point | medium-fast | medium | 2:1–5:1 **[S, MF hub]**; "usually 3:1" **[S]** | |

- **Physics that limits the settings** **[E]**:
  - E1's period is 24 ms and B0's (5-string) 32 ms.
  - A gain that moves within one period modulates the waveform, which is audible distortion of
    the low note.
  - So the release should be ≳ 50 ms (the 1176's fastest), and the detector must not follow the
    waveform: an RMS or a smoothed peak detector, with ≥ 10 ms of smoothing.
  - The fast 1176 attack is safe because it acts on the attack and then holds.
- **The existing pedal compressor** (3 ms attack, 0.2 s release, before the preamp) is an OTA
  stomp box. It suits Slap 2's "squashed" sound in front of the amp. The studio compressor belongs
  after the DI/amp blend.

---

## 7. What the GM sets do for 34–38

### 7.1 FluidR3_GM, measured here

- **Method** **[E, measured]**: TiMidity with `fluidr3_gm.cfg`, reverb and chorus off, mono,
  44.1 kHz. Notes E1, A1, E2, A2, E3 at velocities 40, 80 and 120, held 2 s. Script and renders in
  the scratchpad (`gm/`, `ana.py`). "Bands" are the energy in octave bands of the first 100 ms, in
  dB under the strongest band; at v120 on E1 and A2.

| Program | Peak at v120 (dBFS) | v40→v80→v120 | Decay 0.5–1.5 s | Centroid, first 100 ms (E1…E3) | Bands E1: 20–60 / 60–120 / 120–250 / 250–500 / 0.5–1k / 1–2k / 2–4k / 4–8k |
|---|---|---|---|---|---|
| 34 Fingered | −13.9 | −29.8 / −19.8 / −13.9 | 3.4–6.5 dB/s | 111–310 Hz | −4 / −1 / 0 / −12 / −28 / −36 / −54 / −64 |
| 35 Picked | −9.2 | −25.0 / −15.0 / −9.2 | 2.7–5.7 | 153–415 | −9 / −4 / 0 / −14 / −12 / −21 / −29 / −42 |
| 36 Fretless | −12.8 | −28.4 / −18.6 / −12.8 | 3.8–7.1 | 88–191 | 0 / −11 / −6 / −9 / −25 / −39 / −51 / −60 |
| 37 Slap 1 | −6.2 | −22.0 / −12.0 / −6.2 | 3.4–5.7 | 307–560 | 0 / −6 / −4 / −12 / −11 / −7 / −14 / −22 |
| 38 Slap 2 | −5.5 | −21.4 / −11.4 / −5.5 | 3.4–5.6 | 406–2140 | 0 / −4 / −5 / −8 / −9 / −6 / −11 / −18 |

- **Velocity is level only:** +10 dB from 40 to 80 and +6 dB from 80 to 120 in every program.
  The band shape and centroid are identical, to the dB, at every velocity: no velocity-to-filter
  mapping in this set **[E, measured]**.
- **Finger is dark:** almost nothing above 1 kHz (−36 dB at 1–2 kHz). That is a P- or flatwound-
  and-amp sound, or a DI with the tone rolled off, not a J DI'd with roundwounds.
- **Pick** has 15–25 dB more at 2–4 kHz than Finger, and is 4.7 dB louder.
- **The slaps** are 7.7–8.4 dB louder than Finger at the same velocity. They are bright in the
  attack (the 1–2 kHz band only 6–7 dB under the strongest) and fall to a dark tail: at 1 s the
  centroid is 100–300 Hz (Slap 1) and 145–540 Hz (Slap 2). The tail is compressed-sounding in both.
- Each program decays 3–7 dB/s through the 2 s hold; no note-to-note decay structure (sample
  splits only).

### 7.2 SC-55/88 and XG

- **Not measured** (no hardware or ROM here).
- **Names:** SC-55: "Fingered Bs.", "Picked Bs.", "Fretless Bs.", "Slap Bass 1", "Slap Bass 2"
  **[M]**. GS adds variations under program 37 (counting from 0): "Slap Pop", "Reso Slap",
  "Unison Slap" **[S, via bass_section.md]**.
- **How Slap 2 differs:** "more compressed, squashed", with velocity driving the filter **[S,
  Soundhouse snippet]**. That is the only primary-ish description found. So on Roland sets velocity
  may open a filter, unlike FluidR3.

---

## 8. What a listener would notice if the chain were wrong

1. **Too much top in the miked path.** A 10" or 15" cabinet is −20 dB at 5 kHz (4.3). A bass amp
   without its cabinet, or with a 12" guitar cabinet, sounds fizzy and buzzy on every note. The
   pick click and fret noise turn harsh.
2. **No click at all in a slap.** If everything goes through a B-15 or 8×10 and nothing direct,
   the 4–5 kHz fret clatter (bass_section.md §6.3) is gone and the slap turns into a thud. Slap
   needs the DI, or a horn.
3. **Thin bottom.** Stacked high-pass corners: the guitar presets' 18–35 Hz couplings plus the
   30 Hz output high-pass cost the E1 fundamental 3–6 dB. A sealed 8×10 without its DI partner is
   −10 dB at 40 Hz. The low notes then sound an octave up and weak.
4. **Boom.** Proximity, the cabinet's bump, the bass knob and an 80 Hz EQ all at once give 100 Hz
   mud that masks the kick drum, and a note-to-note unevenness where a note lands on a cabinet
   resonance.
5. **A hollow, phasey blend.** A misaligned DI+mic blend has comb notches starting at 0.5–1 kHz
   (a 30 cm mic). The wrong polarity cancels the fundamental, 6–20 dB at 40–100 Hz when the two are
   near equal **[E]**.
6. **Distortion in the wrong place.** Preamp clipping on every note (a guitar amp's behaviour)
   puts intermodulation on the fundamental: "farting" lows, dirty double stops. A bass amp should
   be clean at moderate levels and growl only on hard attacks, from the power stage (SVT) or a
   dedicated drive with the lows kept clean.
7. **Pumping or a warbling low note.** A compressor release under ~50 ms, or a detector that
   follows the waveform, modulates a 41 Hz note. Too slow a release on a staccato line ducks the
   next note.
8. **Every note at the same loudness and timbre.** Real amps and compressors change both with
   level. The opposite fault: no compression at all, so the slap pops are 10–15 dB over the thumbs
   and the line jumps out.
9. **The wrong bass for the amp.** A P's 2 kHz peak, or a J's 4 kHz, flattened by a 10 kΩ input
   (−6…−12 dB at 1–2 kHz) sounds dull and weak. A pickup simulated without its pots and cable
   rings unnaturally at 5–10 kHz.
10. **Hum and noise.** A single J pickup alone hums (mains and harmonics); both up cancels
    (bass_section.md §3.1). An SVT at volume hisses. Their absence is not noticed; a wrong presence
    is.

---

## 9. Blocks to add to the amp module

1. **Input resistance as a parameter** (`Preset::inputR`): 1 MΩ, 10 MΩ, 140 kΩ, used in
   `connect()`.
2. **A DI branch and a blend.**
   - The EMF goes through the pickup network, then splits. The DI path is optionally an active
     EQ (Baxandall shelves, a peaking mid, the VPF); the amp path is the existing chain and a
     cabinet.
   - `blend` (the DI's share), `diPolarity`, `micDelay` (samples; default 0 = aligned, as engineers
     do).
   - The DI path does not need oversampling.
3. **A James/Baxandall stack type.**
   - Values per preset: R1, Rb, C1, C2, R2, C3, Rt, C4, R3, Rwg; source and load resistance.
   - The network is fourth order. At `setTone()` time, either:
     - solve the 8-node admittance matrix at s on a grid and fit; or better,
     - derive the closed-form rational once (sympy) as `tools/clavtone.py` did for its stack, and
       emit it as code.
   - **A cheaper approximation** (the shelves' corners from 3.1 and 3.2's tables) **[E]**:
     - SVT: bass = low shelf, half its boost at ~230 Hz, +18/−13 dB at the extremes;
     - SVT: treble = high shelf, half at ~6 kHz, +12.5/−6 dB at 12 kHz;
     - B-15: treble half at ~1.5 kHz, +19/−25 dB at 12 kHz.
4. **Peaking mid and fixed voicings.**
   - SVT mid: 220/450/800/1600/3000 Hz, +10/−20 dB, Q ≈ 1 **[E]**.
   - SVT Ultra Lo: low shelf +2…3 dB at 40 Hz and a −10…−12 dB peak at 500 Hz, Q ≈ 0.7.
   - SVT Ultra Hi: high shelf, +8…9 dB at 8 kHz.
   - B-15 Ultra Low: +6 dB low shelf below ~200 Hz.
   - GK: Lo Cut (HP 200 Hz), Mid Contour (notch ~500 Hz), Hi Boost (shelf above 1 kHz), and four
     bands at 60/250/1k/4k Hz.
   - Markbass VPF: +35 Hz shelf, −380 Hz peak, +10 kHz shelf, scaled by one knob.
5. **A per-stage swing.** A stage's grid range scaled (12AU7 or 6SL7 with an unbypassed cathode:
   ×3–5 the 12AX7's), so bass presets clip where their tubes do **[E]**. And corners: `hp` ≤ 10 Hz,
   `hpOut_` ≤ 10 Hz for bass presets.
6. **A solid-state clip** (for the GK boost and the SansAmp-like drive): symmetric, harder knee
   (`knee` = 6 or a tanh) **[E]**.
7. **Cabinets 3, 4, 5 and −1** as in 4.3.
8. **The studio compressor**, after the blend:
   - feed-forward, RMS detector with 10 ms smoothing;
   - attack, release, ratio, threshold, soft knee (6 dB);
   - an "opto" mode: release 60 ms to half, then a slow tail of 0.5–2 s growing with the gain
     reduction held;
   - make-up gain.

   The pedal compressor stays as it is.

---

## 10. What to build: one preset per GM program

Common to all:
- the bass model's EMF and its `Source`: P 6.4 H / 10.5 kΩ / 30 pF / 125 kΩ; J both 1.65 H /
  3.7 kΩ / 90 pF / 83 kΩ (bass_section.md §3);
- cable 700 pF;
- output corner 8 Hz;
- 1× oversampling unless the amp path is driven.

Levels are written as the existing presets do: stage gains, the volume pot's share, the master,
the clip voltage. All numbers are **[E]** unless marked.

### 10.1 GM 34 Electric Bass (finger): "b15-di"

- **Bass:** J, both pickups (neck a little stronger), fingerstyle (bass_section.md §10).
- **Splitter:** an active DI (10 MΩ) and the B-15's input in parallel: `inputR` 0.9 MΩ.
- **DI path:** flat; a gentle high shelf −2 dB above 5 kHz (the engineers' "roll off the top"
  **[T, White99]**).
- **Amp path: the B-15N 1966 channel.**
  - Stage 1: 6SL7, gain 30, hp 3 Hz, swing ×4.
  - Volume share 0.15.
  - Stage 2: gain 30, hp 16 Hz, swing ×4.
  - The James stack after stage 2 (3.1's values; bass knob 0.6, treble 0.45); the phase splitter
    as a third stage of gain 10.
  - Power: 6L6 knee 2, sag 0.25 of swing (5AR4) over 0.1 s, clip set so that a v127 open E
    reaches the knee.
  - Cabinet 4 (1×15 + RE20).
- **Blend:** DI 50 %, amp 50 %; aligned; polarity matched.
- **Compressor:** opto, 3:1, threshold so that mf gives 3 dB of gain reduction and ff 6 dB;
  attack 10 ms; release 60 ms to half, 1 s tail.
- **Expected:** much brighter than FluidR3's Fingered (7.1). If the user wants the FluidR3/SC
  darkness, set the treble to 0.25 and the blend to 35 % DI.

### 10.2 GM 35 Electric Bass (pick): "svt"

- **Bass:** P, pick, roundwounds, tone open.
- **Input:** the SVT's 1 MΩ, plus a DI (10 MΩ): 0.9 MΩ.
- **Amp path: SVT, Normal input.**
  - Stage 1: gain 13 (12AU7-like, swing ×3), hp 2 Hz.
  - Ultra Lo off.
  - Volume share 0.35.
  - Stage 2: gain 18, hp 16 Hz.
  - James stack (3.2's values; bass 0.6, treble 0.55).
  - Mid peak 800 Hz +3 dB, Q 1.
  - Driver stage: gain 8, follower.
  - Master 0.5.
  - Power: 6550 knee 2.5, sag 0.08 (silicon rectifier), clip set so that the hardest pick
    attacks reach 3–5 % THD for 30–100 ms, mf notes < 1 %.
  - 4× oversampling (the growl).
  - Cabinet 3 (8×10 + D112).
- **Blend:** DI 35 %, amp 65 %; aligned.
- **Compressor:** 4:1, attack 0.8 ms, release 150 ms, 4–6 dB of gain reduction at f.
- **Variant "svt-flat"** (a cleaner 1970s P-with-pick-and-mute session sound): master 0.3, blend
  DI 60 %.

### 10.3 GM 36 Fretless Bass: stays on the double bass

- This follows the user's decision (bass_section.md §10).
- **If a fretless J is ever built, "fretless-di":**
  - J, bridge pickup favoured (bridge 1.0, neck 0.6), tone 0.7;
  - an active DI 10 MΩ, DI path only (cabinet −1);
  - a mid peak +2 dB at 800 Hz;
  - opto compressor 3:1 with 2–3 dB of gain reduction;
  - the room's reverb and an optional chorus.

### 10.4 GM 37 Slap Bass 1: "slap-di"

- **Bass:** J, both pickups full; thumb and pop (bass_section.md §6.4).
- **Splitter:** an active DI (10 MΩ) and a solid-state amp input 1 MΩ: 0.9 MΩ.
- **DI path:** the VPF at noon: +4 dB low shelf at 35 Hz, −6 dB peak at 380 Hz (Q 0.7), +4 dB high
  shelf at 10 kHz.
- **Amp path: a GK-like clean head.**
  - Op-amp stages: linear; a solid-state clip far above the playing level.
  - EQ: Bass +3 dB at 60 Hz, Lo-Mid −3 dB at 250 Hz, Hi-Mid +2 dB at 1 kHz, Treble +3 dB at
    4 kHz; Mid Contour on (−8 dB at 500 Hz).
  - Cabinet 5 (4×10 + horn).
- **Blend:** DI 75 %, amp 25 %.
- **Compressor:** 4:1, attack 0.8 ms, release 150 ms. Threshold so that thumbs get 3–4 dB and pops
  6–8 dB, which brings the pops to within ~4 dB of the thumbs.

### 10.5 GM 38 Slap Bass 2: "slap-squash"

- As 37, with the differences below.
- **The pedal compressor in front** (the existing OTA one, 4:1, 3 ms / 0.2 s), about 6 dB of gain
  reduction (bass_section.md §6.4).
- **A SansAmp-like drive on the DI path, blended 25 %:**
  - high-pass 150 Hz before the clipper;
  - soft clip at 6–10 dB over a mf note;
  - low-pass 5 kHz after.
- **Studio compressor:** 8:1, attack 0.2 ms, release 80 ms, 8–10 dB of gain reduction on pops.
- **Brightness:** treble +3 dB more on the DI (high shelf 4 kHz), so that the pops are brighter.
- **To be judged by ear** against 37 (as bass_section.md §6.4 says).

### 10.6 The checks

1. **Pickup network:** P into 10 MΩ peaks at 1.9–2.0 kHz, +2.4 dB (700 pF); J both at 4.0 kHz,
   +4.5 dB. 1 MΩ against 10 MΩ within 0.6 dB everywhere. 140 kΩ: no P peak, −1.5 dB at 2 kHz
   (5.2).
2. **Low end through the amp path, cabinet off:** with the stack at noon, −1 dB or better at
   41 Hz relative to 200 Hz. No corner above 10 Hz except the cabinet's.
3. **James stacks** (with the nodal solve or the closed form):
   - SVT at noon: flat ±1 dB from 30 Hz to 12 kHz, insertion −22 dB;
   - SVT bass full: +18 dB at 30 Hz, +14.8 dB at 100 Hz; treble full +9.5 dB at 8 kHz, +12.5 dB
     at 12 kHz;
   - B-15: bass full +15 dB at 40 Hz; treble full +18.8 dB at 5 kHz, treble zero −17.4 dB at
     5 kHz (3.1, 3.2).
4. **Cabinets** (relative to 200 Hz, ±1 dB):
   - 8×10: −11 dB at 40 Hz, −3 dB at 58 Hz, −20 dB at 5 kHz;
   - 1×15: −4 dB at 40 Hz, −23 dB at 5 kHz;
   - 4×10 + horn: −5.5 dB at 40 Hz, −1.6 dB at 5 kHz (4.3).
5. **Clean headroom:**
   - 34 and 37: E1 at v100: amp-path THD < 1 % (−40 dB);
   - 35: v127 attacks 3–5 % THD for ≤ 100 ms, falling under 1 %;
   - no preamp-stage clipping in 34, 35 or 37 at any velocity.
6. **Blend polarity:** DI + amp at 40–200 Hz no lower than the louder of the two alone. No notch
   deeper than 3 dB under 2 kHz in the blend.
7. **Compressor:**
   - a held E1 at v100: gain reduction settles at the preset's dB within 0.3 s;
   - the compressor adds < 0.5 % THD on a held E1 (the release and detector check);
   - slap: pops against thumbs ≤ 4 dB after 37's compressor, ≤ 2 dB after 38's.
8. **Velocity:**
   - v40 → v120 at the DI: 16–20 dB (FluidR3: 16 dB);
   - after each preset's compressor: 10–12 dB (34, 35), 8–10 dB (37), 5–7 dB (38);
   - the first 100 ms's centroid rising with velocity (FluidR3's does not move; the physical bass
     should).
9. **Relative levels between programs at v120**, to match the GM set's balance (7.1): pick
   +4.7 dB over finger; slaps +7.7…8.4 dB over finger; fretless ≈ finger.
10. **Spectral envelope sanity**, E1 at v120, first 100 ms, octave bands (7.1): 34 brighter than
    FluidR3's but its 2–4 kHz band still ≥ 25 dB under its strongest; 35's 2–4 kHz within 25–35 dB;
    37/38's 1–4 kHz within 15 dB of the strongest.
11. **Cost:** a DI-heavy clean preset ≤ 0.3 % of a core; "svt" with 4× oversampling ≤ 1 %
    (electric_guitars.md §11.6's estimate).

---

## 11. Not found, or uncertain

- **The SVT midrange inductor** (320821-1): its inductance and taps, hence the 1972 mid's exact
  frequencies and Q. The 1972 three-position frequencies (220 Hz / 800 Hz / 3 kHz) are lore.
- **The SVT Ultra Low network:** read from a 800×600 drawing. The topology (a bridged-T) is my
  reading and gives ~1 kHz, not the later spec's 500 Hz. The original service manual was not read.
- **Which half of the 12DW7** is the gain stage in V1/V3: from the standard pinout [M], not
  checked against the drawing's pin numbers in a data sheet.
- **The Bassman AB165 bass channel's stack values** (forum reports conflict) and its Deep switch.
- **The Acoustic 360's tone controls** and its "Variamp"/fuzz circuits: nothing primary found.
- **GK 800RB:** the hardware manual (control ranges in dB, the crossover range, the boost
  circuit). Only an emulation's manual was read.
- **Markbass VPF/VLE:** curves and depths. Only a description (35 Hz, 380 Hz, 10 kHz).
- **SansAmp Bass Driver:** the treble's and presence's frequencies, the clipper.
- **Measured responses of whole bass cabinets** (8×10, 4×10, B-15) with a microphone. Only the
  manufacturers' ±3 dB figures and bare-driver curves were found. The biquad sets of 4.3 are fits
  to those, not to cabinet measurements.
- **The original B-15 speaker's response**, and the double baffle's effect.
- **Microphone response curves** (D112, Beta 52A): only the headline ranges were read. The
  presence peaks and proximity numbers are [M].
- **Measured DI-loading differences on a passive bass:** only a review's listening impression.
- **SC-55/88 and XG renderings** of 34–38: not measured. Whether their velocity opens a filter is
  a snippet.
- **Blend ratios and compressor settings on famous records:** only lore. No engineer's documented
  settings for a specific GM-style record were found.

---

## 12. References

- **[SVT-CL]** Ampeg, *Owner's Guide for the SVT-CL Bass Amplifier* (front panel and technical
  specifications).
  https://ampeg.com/data/6/0a020a401692c654ea0c8b1d99/application/pdf/
- **[SVP-PRO]** Ampeg, *SVP-PRO User's Guide* (specifications). https://ampeg.com/pdf/SVP-PRO.pdf
- **[SVT2PRO]** Ampeg, *SVT-2PRO Owner's Guide*. https://ampeg.com/pdf/SVT-2PRO.PDF
- **[SVT-VR]** Plugin Alliance / Brainworx, *Ampeg SVT-VR Classic Plugin Manual* (Rec chains:
  810E with a Neumann CMV 563; 410HLF + 15E).
  https://files.plugin-alliance.com/products/ampeg_svtvr_classic/ampeg_svtvr_classic_manual.pdf
- **[B15N-OM]** Ampeg, *Heritage B-15N Owner's Manual*. https://ampeg.com/pdf/manuals/Heritage_B_15N_OM.pdf
- **[Piazza-SVT72]** J. Piazza (drawing, 1996), *Ampeg SVT pre-amp section 1972*.
  https://drtube.com/schematics/ampeg/svtpre-jp.gif ; index https://drtube.com/ampeg
- **[Piazza-B15N68]** J. Piazza (drawing, 1996), *Ampeg Portaflex B-15-N (1968)*.
  https://drtube.com/schematics/ampeg/b15n68-jp.gif
- **[810E]** Ampeg SVT-810E owner's manual (frequency response ±3 dB 58 Hz–5 kHz; −10 dB 40 Hz),
  e.g. https://ampeg.com/data/6/0a000419003669dfa82b7df50/application/pdf/ ; retailer
  specs: https://www.zzounds.com/item--AMPSVT810E
- **[410HLF]** SVT-410HLF spec (crossover 4 kHz; 48 Hz–18 kHz).
  https://www.zzounds.com/item--AMPSVT410HLF ; https://www.sweetwater.com/store/detail/SVT410HLF
- **[BP102]** Eminence, *Legend BP102* data sheet (T/S parameters, measured SPL curve).
  https://en.toutlehautparleur.com/media/catalog/product/datasheet/eminence/LEGEND_BP102-8.pdf
- **[CB15]** Eminence, *Legend CB15* data sheet.
  https://en.toutlehautparleur.com/media/catalog/product/datasheet/eminence/LEGEND_CB15-8.pdf
- **[JDI]** Radial Engineering, *JDI specifications*. https://www.radialeng.com/product/jdi/specifications
- **[JT-DB-E]** Jensen, *JT-DB-E* data sheet and direct-box transformer selector guide.
  https://www.jensen-transformers.com/wp-content/uploads/2014/08/jt-db-e.pdf ;
  https://www.jensen-transformers.com/?p=1269
- **[PZDI-bonedo]** bonedo.de, *Radial PZ-D.I. Test*. https://www.bonedo.de/artikel/radial-pz-di-test/
- **[BSDR]** Tech 21, *SansAmp Bass Driver DI owner's manual* (v1).
  https://www.tech21nyc.com/wp-content/uploads/2019/05/BassDriver-OM1.pdf
- **[BSDRv2]** Tech 21, *SansAmp Bass Driver DI v2 owner's manual*.
  https://www.tech21nyc.com/wp-content/uploads/2020/09/BSDR_v2_OM3.pdf
- **[GK800-UA]** Brainworx / Universal Audio, *Gallien Krueger 800RB Plug-in Manual*.
  https://media.uaudio.com/support/manuals/dd/Gallien%20Krueger%20800RB%20Manual.pdf ; hardware
  manual (not read): https://manualmachine.com/gallienkrueger/800rb/2576188-user-manual
- **[MB-TA503]** Markbass TA 503 manual (VPF, VLE).
  https://www.manualsdir.com/manuals/803780/markbass-ta-503.html?page=13 ; usage:
  https://www.talkbass.com/threads/markbass-vle-and-vpf-filters.763785/
- **[fenderguru]** Fenderguru, *BF/SF Bassman*. https://fenderguru.com/amp-models/bassman/
- **[PG-Bassman]** Premier Guitar, *Ask Amp Man: Bringing a Blackface Bassman in from the Cold*.
  https://www.premierguitar.com/gear/ask-amp-man-bringing-a-blackface-bassman-in-from-the-cold
- **[Wiki-Acoustic]** Wikipedia, *Acoustic Control Corporation*.
  https://en.wikipedia.org/wiki/Acoustic_Control_Corporation ; reissue listing:
  https://reverb.com/item/17823529-acoustic-usa-360-361m-bass-stack
- **[White99]** P. White, "Recording Bass Guitar", *Sound On Sound*, March 1999.
  https://www.soundonsound.com/techniques/recording-bass-guitar
- **[Owsinski13]** B. Owsinski, "In The Studio: Recording The Bass Amp", ProSoundWeb, 2013.
  https://www.prosoundweb.com/in-the-studio-recording-the-bass-amp/
- **[SOS/Anderton]** Sound On Sound, DI/mic alignment and mixing DI bass:
  https://www.soundonsound.com/techniques/mixing-di-bass-guitar
- **[Reverb guide]** "Essential Tips for Recording Bass Guitar".
  https://reverb.com/news/essential-tips-recording-bass-guitar
- **[Jamerson]** Reverb, "A Tribute to James Jamerson…" and "How to Sound Like Motown's James
  Jamerson"; MusicRadar; Guitar World (Fairchild and Pultec).
  https://reverb.com/news/a-tribute-to-james-jamerson-and-his-stolen-funk-machine ;
  https://reverb.com/news/video-how-to-sound-like-motowns-james-jamerson ;
  https://musicradar.com/news/james-jamerson-bassist ; https://guitarworld.com/features/ampeg-b15-portaflex
- **[1176]** Universal Audio, 1176LN manual and help pages.
  https://c3.zzounds.com/media/1176LN_manual-1a90c9dff91ec30316551923e877c4d0.pdf ;
  https://help.uaudio.com/hc/en-us/articles/34530260482324-1176-Classic-FET-Compressor-Manual
- **[LA2A]** Universal Audio, *Teletronix LA-2A*.
  https://help.uaudio.com/hc/en-us/articles/206356233-Teletronix-LA-2A-Classic-Leveling-Amplifier ;
  **[VintageKing]** https://vintageking.com/blogs/fun-stuff/teletronix-la-2a-history-behind-the-hardware
- **[MF hub]** Musician's Friend, "Compression settings for recording bass guitar".
  https://www.musiciansfriend.com/thehub/compression-settings-for-recording-bass-guitar
- **[RE20]** Electro-Voice, RE20 product page and data sheet. https://products.electrovoice.com/na/en/re20
- **[D112]** AKG D112 spec sheet. https://www.barryrudolph.com/recall/manuals/akgd112.pdf ;
  MkII: https://support.harmanaudio.com/on/demandware.static/-/Sites-masterCatalog_Harman/default/dwef0475a5/pdfs/AKG_d112_mkII_cutsheet.pdf
- **[Beta52]** Shure Beta 52A spec sheet. https://www.la-bs.com/Objets/BETA52_dt.pdf
- **[PG Darkglass]** Premier Guitar, *Darkglass Microtubes B7K Ultra review*.
  https://www.premierguitar.com/darkglass-microtubes-b7k-ultra-review
- **FluidR3_GM** (installed `/etc/timidity/fluidr3_gm.cfg`), rendered with TiMidity and measured
  here (7.1).
- Project documents: `research/bass_section.md` (the basses, pickups, techniques, GM mapping),
  `research/electric_guitars.md` §11 (the amp module), `src/guitaramp.h/.cc`.
