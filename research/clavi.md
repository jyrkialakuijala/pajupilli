# The clavi (D6; also C, E7, Duo): research and model plan

Status: 2026-11-15. Research only; nothing is built. GM program 8 ("Clavi"). SI units. Pitches
by MIDI-style names (C4 = middle C, A4 = 440 Hz). Keys are numbered 1..60 from F1 (MIDI 29) to
E6 (MIDI 88).

This file builds on `research/harpsichord_clavi.md` §1, §3, §5 (which already summarises
Gabrielli 2013 and Remaggi 2012: the moving-end excitation, the 23 wound strings, Gabrielli's B
table, the T60 ripple and beating, the pickup polynomial, the release drop of three semitones,
the waveguide model's cost). That material is not repeated here except where it is corrected or
extended. New here: the D6, E7 and C circuits read from the schematics and the tone network
worked out from them (§7), which pickup is upper and which lower (§6.1), an estimated string
scale (§4), the physics of the strike, release and pressure bend (§3, §5), the family (§2),
playing practice and effects (§9), and a model plan against `src/piano.h` (§10).

Tags: **[web]** read in a source in §12 (the key follows in brackets); **[paper]** in a
peer-reviewed paper; **[schematic]** read off the schematic images; **[calc]** worked out here
(formula or script given); **[memory]** general knowledge not checked for this file.

## 1. Short answer

- **Compass:** 60 keys F1–E6, one steel string each, the 23 lowest wound (F1–D#3) **[paper,
  Gabrielli 2013; web, one maker 1973 catalogue "F1–e³"; Wikipedia]**. Verified.
- **Strike:** the key's rubber tip (the tangent) pushes the string onto a metal anvil and holds it
  there. The string's end at the anvil is thereby moved by the string-to-anvil gap h (about
  1 mm **[calc]**) in τ = h/v, a fraction of a millisecond to ~2 ms **[calc]**. Each mode gets
  2h/(nπ) of displacement below the corner 1/(πτ) and a share growing with the tangent's speed
  above it. So **velocity mostly changes brightness**; the low partials' level is set by h.
- **Release:** two things happen at once **[calc]**:
  - the speaking and the yarn-wrapped part join, so the note drops three semitones;
  - the string, held bent around the anvil by h, is let go: a pluck at 0.16 of the joined
    string, right next to the yarn.
  - Together they make the "thunk": low partials at the dropped pitch ringing ~0.1–0.3 s while
    the highs die in tens of ms (estimate; nothing is published).
- **Pickups:** two single-coil bars, each six coils over ten strings **[schematic; paper]**.
  - The **bridge pickup is the upper one** (above the strings, 4 cm from the tailpiece end,
    tilted 30°): bright, thin bass.
  - The **centre ("rhythm") pickup is the lower one**: 18.5 cm (bass) to 6.5 cm (treble) from
    the end, warm. Gabrielli 2013 and the clavi.com FAQ agree; Remaggi 2012 has it the other
    way round.
  - Switches: C/D sets one pickup or both; A/B then picks centre/bridge, or in/out of phase.
- **Electronics (D6, from the schematic):**
  - a Beyer TR 145 transformer, 1:15 step-up;
  - a BC550C stage (collector 51 kΩ, 3.9 MΩ ∥ 8.2 pF collector-base feedback);
  - the **tone network**: four elements **in series** from the signal node to ground, each
    shorted by its switch until pressed. Soft is 30 kΩ ∥ 0.1 µF, Medium 10 kΩ ∥ 15 nF, Treble
    2 H ∥ 4.7 nF, Brilliant 0.6 H. With nothing pressed the node is grounded: silence.
  - then 30 kΩ + 10 nF into a BC148B stage (gain ≈ 11) and a 100 kΩ volume.
  - Worked out (§7.3): Soft is a dark low-pass peaking near 170 Hz; Medium a broad hump near
    650 Hz; **Treble a resonant peak at 1.6 kHz**; Brilliant +6 dB/oct up to ~6 kHz. Pressed
    together, the elements resonate in series and cut notches (Soft+Brilliant near 650 Hz,
    Treble+Brilliant near 3.4 kHz). The settings differ by up to 20 dB in level.
  - Gabrielli models the switches as a cascade of the four impedances. The circuit is a series
    sum, so the combined settings come out wrong in that model.
- **What makes the sound** (§9): an end-driven sawtooth-like source, read by a velocity pickup
  near the end, which makes partials 2–3 louder than 1; the comb; the 1.6 kHz Treble peak; long
  sustain cut by a fast, pitch-dropping release; the key's knock. Then, almost always, a wah or
  envelope filter and an amp.
- **Model** (§10): a new `clavi` on the piano's modal-string machinery.
  - Per key two mode banks: speaking (held) and joined (released), with a fixed projection
    between them.
  - A moving-end exciter instead of the hammer; the yarn as a damper band on the joined bank.
  - Two weighted velocity taps per string; the switch matrix; the exact tone network as one IIR
    per switch setting; the two stages with a soft clip. Optionally a wah.
  - About 1–3 % of a core for ten voices **[calc]**. Validate with `tools/pianofit.py`-style
    fits. The pickup-comb nulls also give each string's length (§10.6).

## 2. The family

| Model | Years | Differences that matter to a model | Source |
|---|---|---|---|
| clavi I | 1964 | Built-in battery amp and speaker; two tablet switches for pickups; aimed at Baroque music | [web, Wikipedia] |
| clavi II | after I | Preamp instead of the speaker; first with rocker tone switches | [web, Wikipedia] |
| early clavi | 1968 | **Two magnets per pickup** ("unique tone"); **no transformer, no tone network**; 2× AC151 germanium PNP; a 3 nF shunt on the output (corner ≈ 14 kHz from 3.9 kΩ **[calc]**); 9 V. Used on "Superstition" (several overdubs) | [web, clavinet.com ident; schematic; Wikipedia; FAQ] |
| clavi L | 1968 | Home model, trapezoid case, built-in amp, reversed key colours | [web] |
| **clavi** | 1971–1982 | Four tone switches, AB/CD pickup switches, **mute slider**; unshielded; two preamp board layouts (on the switches, or on the floor) | [web, Wikipedia; FAQ §12] |
| late clavi | 1977/1979 (sources differ) | D6 controls plus slide volume, power switch; better shielding and a noise filter; 8.2 V zener regulator (BC238); different stage values (§7.4): "drier" | [web, clavinet.com; Wikipedia; groupdiy] |
| clavi and reed piano Duo | 1978 | E7 clavi + reed electric piano T (reeds plucked by sticky pads) in one case; foot switch clavi and reed piano/mix, keyboard split, stereo out. Its pickups have a centre tap so each half can be switched off | [web, Wikipedia; FAQ §11.1] |
| D6-N | — | D6 in the E7-style case | [web, Wikipedia] |

- About 38 000 made in all **[web, Wikipedia]**. D6 size about 119 × 38 × 15 cm, 31 kg with
  legs **[web, Hohner 1973 catalogue via hohner-pianet blog]**.
- one maker's own claim: "tone production is individually determinable through the touch"
  **[web, 1973 catalogue]**.
- **The reed electric piano T** (in the Duo) is a separate instrument (plucked reeds, passive pickups). It is
  out of scope here and would fit the project's tine/reed families (`src/modal.cc`), not this
  string model.

## 3. The mechanism

### 3.1 Parts

- The key is a lever with the fulcrum at the rear and a return spring. A metal holder under the
  key carries the rubber tip (the "hammer tip", the tangent) **[web, Wikipedia; FAQ §15]**.
  Gabrielli calls it a class-2 lever **[paper]**.
- The strings run diagonally under the keys: tuning pin at the front (player's side), tailpiece
  at the back. **The yarn is woven through the strings at the tuning-pin end** **[paper,
  Gabrielli 2013 fig. 1]**. (Wikipedia says "far end"; Gabrielli's labelled drawing wins.)
- The anvil is a stud on the harp under the string, near the tuning-pin end. The string runs
  between the tangent and the anvil. The tangent traps it for as long as the key is down
  **[paper]**.
- Key dip: about 6 mm intended, 8 mm seen on a worn instrument **[web, musicplayer forum]**. The
  tangent sits between the fulcrum and the finger, so it travels less than the key front
  **[calc]**.
- Original tips are orange rubber. They go sticky with age, which gives an "after-thunk" or a
  "pop" on release (the string sticks to the tip briefly) **[web, FAQ §14]**. Logic's EVD6
  models this as a "key-off click" **[web, Apple]**.

### 3.2 The strike as a moving end [calc]

- Let h be the string's height over the anvil top at rest, and v_t the tangent's speed. The
  string is light against the key (impedance √(Tμ) ≈ 0.1–0.2 kg/s, §4: 0.1–0.6 N at 1–4 m/s), so
  the tip drives the string's end as a velocity source. The end moves by h in τ = h/v_t and then
  stops on the anvil.
- With the speaking string written as u(t)(1 − x/L_s) plus modes, mode n is driven by
  −(2/(nπ)) ü(t). An ideal ramp has ü = v_t[δ(t) − δ(t − τ)], so mode n's swing is
  - (2h/(nπ)) for ω_n τ ≪ 1 (a step: the sawtooth spectrum, independent of key speed);
  - about (2/(nπ)) · √2 · v_t / ω_n for ω_n τ ≫ 1, with nulls at f = k/τ: **proportional to
    v_t**.
- The corner is f_c ≈ 1/(πτ). For h = 1 mm and v_t = 0.5–2 m/s, τ = 0.5–2 ms and f_c ≈
  160–640 Hz.
  - Above f_c the spectrum gains 6 dB per doubling of key speed; below it nothing changes.
  - So a clavi's dynamics are mostly a change of brightness and of the 1–4 kHz energy the
    ear reads as loudness.
  - The bass fundamentals barely change with velocity. To check on recordings (§10.6).
- The rubber rounds both corners of the ramp. Contact on the string starts softly (the tip
  compresses over ~0.1–0.3 ms) and the stop on the anvil is cushioned the same way (k ≈
  10⁴–10⁵ N/m with an effective key mass of ~10–20 g gives a contact time of ~1 ms **[memory for
  k and m; calc]**). That is a low-pass of a few kHz on the excitation. Fresh hard tips are
  brighter; worn soft tips duller **[calc]**.
- **Gabrielli's own numbers** **[paper]**: key speed 1–4 m/s mapped linearly from MIDI
  velocity; pulse length N = f_s d/v, with d the tangent–anvil distance (not published). His
  pulse is the pickup's output (velocity) for this end motion.
- **Amplitude check:** the string's largest swing is 1 mm peak-to-peak **[paper, Remaggi]**. The
  displacement field after a step is h(1 − x/L_s) around the new line. Near mid-string that gives
  about h p-p, so h ≈ 1 mm is consistent **[calc]**.
- **Rebound:** on a hard blow the key can bounce. The microphone recordings were dominated by
  "the mechanical noise generated by the key, its rebound, and the tangent hitting the anvil"
  **[paper]**. A bounce that briefly frees the string and re-clamps it gives a short buzz.
  Players hear a "fret buzz" on release **[web, modartt forum]**. Optional in a model.

### 3.3 The knock

- The tangent hitting the anvil reaches the pickups through the string. It is audible on high
  notes and its energy lies below 1.2 kHz **[paper, Gabrielli]**. The keybed and harp are a
  mechanical support, not a soundboard **[paper]**.
- Model it as a few frame resonances (a guess: 4–6 modes at 150–1200 Hz, Q 5–20) struck by the
  tip's stop force. They move the strings' ends, so the knock enters each sounding string as an
  end drive and is heard pitched through that string's modes **[calc, plan]**.

### 3.4 Key pressure: a pitch bend?

- **Claims:**
  - Up to a quarter-tone (Stiller, *Handbook of Instrumentation*, 1985, quoted on a piano-modelling
    forum), "ineffective in chords".
  - A D6 owner on the same forum says pressure does nothing.
  - Wikipedia: "aftertouch from the string being stretched further".
  - Logic's model has a "Pitch Pressure" control; a a sample library script offers aftertouch bend
    in cents and its author finds small values more natural **[web, all]**.
- **Physics [calc]:**
  - Holding the string down onto the anvil stretches it by ΔL ≈ (h²/2)(1/a + 1/L_s), with a =
    the yarn-side length (0.19 L_s).
  - For E3 (L_s = 0.678 m, a = 0.13 m, .011" plain, T = 24 N) and h = 1 mm: +0.29 % tension, +2.5
    cents. This is permanent while the key is down, so tuning (done with keys down) removes it.
  - Pressing harder only adds what the stack yields. 0.5 mm more yield gives about +3 cents
    (ΔL ∝ h²).
  - If the rubber grips the string and the tip slides 0.1 mm along it, the speaking part's
    strain changes by 1.5·10⁻⁴: about +60 cents on E3. So a worn or soft tip can bend a lot, a
    hard one hardly at all.
  - Both reports may be right for different instruments.
- **Decision:** map channel or poly aftertouch to 0..+X cents per key, default X ≈ 10 (range
  0–50). Not on by default in chords (Stiller), or simply leave it to the player.

### 3.5 A pitch glide on the strike? [calc]

- Kirchhoff–Carrier tension modulation on these slack strings: for E3 with h = 1 mm, the mean
  stretch from the excited modes gives about +3 cents at pp (τ = 1 ms) and about +10 cents at ff
  (τ = 0.25 ms). It decays with the high partials in tens of ms.
- Gabrielli measured f0 stable to 1–2 cents over attack and sustain **[paper]**. Logic offers a
  "Tension Mod" (a small upward bend after the strike) **[web, Apple]**.
- So include it as an optional term (a few cents) and keep it small.

## 4. Strings and scale

### 4.1 What is known

| Item | Value | Source |
|---|---|---|
| Strings | 60, one per key, steel; 23 wound (F1–D#3), 37 plain (E3–E6) | [paper, Gabrielli] |
| Gauges | wound .032" ×10, .028" ×5, .022" ×9; plain .011" ×3, .009" ×35 (chart garbled: 24 wound); .008" at the very top on one restring | [web, Vintage Vibe; clavinet.com] |
| One length | 67.8 cm at 161 Hz (≈ E3) | [paper, Remaggi] |
| B | F1 5e-4, A#1 2e-4, D3 9e-5, D#3 1e-4, F#5 9e-5, E6 8e-5; a jump at keys 23/24 | [paper, Gabrielli table 1, fig. 6] |
| Tuning | "strings relatively slack", tune with the key down, by machine heads at the front | [web, D6 manual; Wikipedia] |
| Size | case 119 × 38 cm, so strings run diagonally; the longest can't exceed ~0.9 m | [web; calc] |

### 4.2 An estimated scale [calc]

- **Plain strings:** solve B = π²Ed²/(64ρL⁴f²) (E = 200 GPa, ρ = 7850) at the measured B.
- **Bass:** the comb in Gabrielli's A#2 spectrum has nulls every 5th partial. With the centre
  pickup at ≈ 15 cm there (linear between 18.5 and 6.5 cm), L ≈ 0.75 m.
- **Wound strings:** B at F1 (5e-4) with a .032" outside needs a core of ≈ 0.5 mm at L ≈
  0.8 m. Tension is only ~17–19 N, the "slack" strings.

| Key | Note | f0 (Hz) | L (m) | Wire | T (N) | Stress (GPa) | B | Modes to 12 kHz |
|---|---|---|---|---|---|---|---|---|
| 1 | F1 | 43.7 | ≈0.82 | wound .032", core ≈0.5 mm | ≈17–19 | ≈0.1 core | 5e-4 [paper] | 106 |
| 18 | A#2 | 116.5 | ≈0.73–0.75 (comb) | wound .022" | — | — | ≈1.2e-4 | ~75 |
| 24 | E3 | 164.8 | 0.678 [paper] | plain .011" | 24 | 0.39 | 5.3e-5 calc (≈1e-4 measured nearby) | 61 |
| 36 | E4 | 329.6 | ≈0.43 | .009" | ≈24 | ≈0.6 | ~9e-5 | 34 |
| 41 | A4 | 440 | ≈0.36 | .009" | ≈32 | ≈0.8 | ~9e-5 | ~25 |
| 50 | F#5 | 740 | 0.254 (from B) | .009" | 46 | 1.1 | 9e-5 [paper] | ~15 |
| 60 | E6 | 1318.5 | 0.19 (from B; .008": 0.185) | .009"/.008" | 86 / 61 | 2.1 / 1.9 | 8e-5 [paper] | 9 |

- The top string near 2 GPa is close to breaking for 0.2 mm music wire (2.5–2.8 GPa **[memory]**).
  Either the top is a little shorter than B suggests (B from ~9 partials is soft), or it is
  .008". Use 0.17–0.19 m.
- The E3 discrepancy (B 5e-5 calculated against ~1e-4 measured nearby) means either a thicker
  first plain string (.013–.015") or noise in the measured B. Fit per key from recordings.
- **Mode count:** 3168 per direction to 12 kHz over all 60 strings, 3889 to 16 kHz (B
  interpolated log-linearly from Gabrielli's table). F1 106/124, E3 61/76, E6 9/12. The earlier
  file's "F1 needs ~275 modes" ignored B.
- Impedance √(Tμ): 0.08–0.17 kg/s, the harpsichord's range. The string hardly loads the tangent
  or the anvil.

### 4.3 Losses while the key is down

- **Published:** T60 ≥ 20 s in the low and middle range; the lowest 2–4 partials far longer than
  the rest; T60 ripple with a period of 2–3 partials; beating below E4 at 0.5–2 Hz, up to 15 dB
  p-p **[paper]**. Only the ends take energy: the metal tailpiece bar and the rubber tip
  **[paper]**.
- **Reading [calc]:** a lossy rubber end gives a loss rising with frequency. The ripple of 2–3
  partials suggests a second, compliant contact about L/2–L/3 from an end. The tangent's rubber
  pad has a width (a few mm), so the termination is smeared.
- **Start values for the model:** T60(f) = 20 s at 100 Hz → 8 s at 1 kHz → 1.5 s at 5 kHz.
  Fit them per key.

## 5. Release, the yarn and the mute

### 5.1 What happens [calc, on the published facts]

- Key up: the tip leaves, and the string becomes one string of L_t = L_s + a with a = 0.19 L_s
  (the three-semitone drop for every key **[paper]**: the anvils sit at a constant fraction of
  the length).
- **Its initial state is the sum of three things:**
  1. the speaking part's vibration at that instant;
  2. the static bend: zero at the pin, −h at the anvil point, back to zero at the tailpiece.
     Released, this is **a pluck of height h at 0.16 L_t**;
  3. whatever the sticky tip adds as it lets go (a lift, then a snap: the "pop" **[web, FAQ]**).
- **The yarn** covers the end part (up to a = 0.16 L_t). As a distributed resistance r (kg/(m·s))
  it damps mode m of the joined string at a rate σ_m = r·w_m/(2μ), where
  w_m = a/L_t − sin(2πm a/L_t)/(2πm).
  - That is 0.026 for m = 1 and → 0.16 for high m. The fundamental of the dropped note is damped
    about **6 times more slowly** than the highs.
  - If the highs die in ~30 ms (T60), the dropped fundamental rings ~0.2 s. That is the short
    downward "bwow" in the release.
- **Nothing measured is published** for these times. Gabrielli shows a spectrogram (release at
  0.5 s) and says the drop is "of short duration" with an "effective yarn damper" in a mint
  instrument **[paper]**. Worn yarn rings on. Fit the times to recordings.
- **Release speed:** a slow release lifts the tip slowly. The static bend is then let go over a
  longer time, which gives a softer thunk (the same corner law as the strike). Release velocity,
  where a keyboard sends it, maps here (Logic uses it for the click **[web, Apple]**).
- **Re-strike while the joined string rings:** the tangent clamps it again. Project the joined
  string's state back onto the speaking bank (the part on the yarn side is lost in the yarn).

### 5.2 The mute slider (D6, E7, Duo)

- A bar with pads that drops onto all strings, worked by a slider on the right **[web,
  Wikipedia; FAQ §15.2]**.
- The manual says use only the end positions (all muted or none), so in between it mutes some
  strings and not others **[web, D6 manual]**. A bar out of adjustment mutes some strings and kills
  sustain **[web, FAQ]**.
- **Position:** near the far (pickup) end in Gabrielli's fig. 1 (part I) **[paper, figure
  only]**. Neither pad position nor material is published.
- **Effect:** a palm-mute: the decay drops to ~0.1–0.5 s and the upper partials drop
  **[memory]**, and it is used for staccato bass lines.
- **Model:** a per-mode damper band near the tailpiece end at x ≈ 0.03–0.08 m from it (between
  the bridge pickup and the end). Its rate follows the slider (0..1). Nonlinear engagement,
  uneven across strings, at mid positions.

## 6. Pickups

### 6.1 Which is which (resolved)

| | Bridge pickup | Centre pickup |
|---|---|---|
| Side | **above** the strings ("upper") | **below** ("lower") |
| Distance from the tailpiece end | 4 cm, all strings | 18.5 cm (F1) → 6.5 cm (E6) |
| Orientation | tilted ≈ 30° | square to the strings |
| Bars | 3.3 cm × 0.5 cm | 3.7 cm × 0.5 cm |
| Switch name (D6/E7) | B "bridge/lead" | A "rhythm" |
| Sound | bright, brash; thin bass | warm; more bass, smoother attack |

- **Sources:** Gabrielli 2013 says the bridge pickup is above and the centre below. The
  clavi.com FAQ says the "upper" pickup is "bright brash" and the "lower" one "warm" **[web]**.
  Chicago Electric Piano says B = bridge **[web, schematic notes]**. Remaggi 2012 has the reverse
  and is the odd one out.
- Each pickup is six coils in series (the schematic draws six per pickup), each coil over ten
  strings **[schematic; paper, Remaggi]**. Gabrielli 2013 says "10 bars × 6 strings", which
  contradicts the drawing; take six × ten.
- **early clavi:** each pickup has two magnets **[web]**. The schematic draws two cores per pickup.

### 6.2 Switch matrix

| D6/E7 left (C/D) | right (A/B) | Result |
|---|---|---|
| C | A | centre (rhythm) alone: warm |
| C | B | bridge alone: bright |
| D | A | both in phase: full |
| D | B | both out of phase: thin, "the fundamental cancels somewhat" |

- **[web, till.com; FAQ; Chicago Electric Piano; schematic]**. The till.com table describes
  the same four states by switch up/down positions.
- **The C's letters are different [schematic]:** A = both pickups, B = one. Then A+C out of
  phase, A+D in phase, B+C bridge, B+D rhythm.
- **Pickups in parallel:** "parallel" in the schematic notes. Into the 1:15 transformer, both
  in parallel average the two EMFs (each coil's source impedance loads the other), so the sum
  is about the mean, not the total **[calc]**. Level differences between settings are a few dB.

### 6.3 Comb and phase per key [calc]

- Mode n's share at a pickup is ∝ sin(nπ d/L) (d from the tailpiece end), with the §4.2 lengths.
  The table gives dB for partials 1, 2, 3.
- The last column is the out-of-phase (C − B) sum's level relative to the in-phase sum, for the
  same partials, at equal pickup gains.

| Key | d_C/L | Centre p1/p2/p3 | Bridge p1/p2/p3 | (C−B)/(C+B) p1/p2/p3 | First null C, B (partial) |
|---|---|---|---|---|---|
| F1 | 0.23 | −3.7/−0.1/−1.4 | −16.3/−10.4/−7.1 | −4.2/−5.5/−10.1 | 4.4, 20 |
| A#2 | 0.21 | −4.4/−0.3/−0.6 | −15.3/−9.4/−6.1 | −5.1/−6.4/−10.3 | 4.8, 18 |
| E3 | 0.20 | −4.5/−0.4/−0.5 | −14.7/−8.8/−5.6 | −5.5/−6.9/−11.0 | 4.9, 17 |
| E4 | 0.26 | −2.6/0.0/−4.3 | −10.8/−5.2/−2.3 | −7.1/−10.8/−18.7 | 3.8, 11 |
| A4 | 0.29 | −2.0/−0.3/−8.1 | −9.2/−3.8/−1.2 | −8.1/−14.1/−8.5 | 3.4, 9 |
| E5 | 0.33 | −1.4/−1.0/−23.7 | −7.1/−2.0/−0.2 | −10.0/−25.1/−1.2 | 3.1, 7 |
| E6 | 0.34 | −1.1/−1.5/−21.7 | −4.2/−0.3/−0.8 | −15.0/−22.7/+1.6 | 2.9, 5 |

- **Reading:**
  - The bridge pickup has the fundamental 10–16 dB down in the bass: the "brash" sound.
  - Out of phase loses 4–15 dB of the lowest partials, more in the treble, and keeps the highs:
    the thin, nasal, quacky setting.
  - The centre pickup's comb (nulls every 3–5 partials) is the clavi's formant pattern.
  - The pickups' heights (set by screws, not published) change their relative gains by several
    dB. Make the gains parameters.
- **Velocity sensing:** the coil reads the string's velocity (ω × displacement). With the 1/n
  source of §3.2, the pickup sees a flat-to-rising spectrum up to the first comb null. That is
  why partial 2 is always ≥ 3 dB over partial 1 **[paper]**.
- **Nonlinearity:** Gabrielli's 4th-order polynomial in displacement (in the earlier file). At
  1 mm p-p it adds a few % of 2nd and 3rd harmonic distortion **[paper, Remaggi: their fig.]**.
  Keep it, applied to each pickup's displacement sum.
- **Horizontal polarisation:** 25 dB below vertical **[paper]**.
- **Aperture:** a bar 5 mm wide (5.8 mm along the string at 30°) averages over the string. That
  is −1 dB at 12 kHz on a 218 m/s string **[calc]**. Ignore.

## 7. Electronics (read from the schematics)

### 7.1 D6 signal chain [schematic, Chicago Electric Piano 2015 redraw of Hohner's]

1. **Pickups** (two × six coils), the switch matrix, then the primary of a **Beyer TR 145
   (BV 35 569) transformer, 1:15**. The FAQ calls it "the heart of the clavi sound", a
   studio microphone transformer **[web, FAQ §12.2]**. A 1:15 step-up is ×15 in voltage
   (+23.5 dB) and ×225 in impedance **[calc]**.
2. **Stage 1:** 10 nF → base of **BC550C** (BC413C on some boards **[web, clavi.com
   upgrade]**). The emitter is grounded; collector 51 kΩ to +9 V; 3.9 MΩ ∥ 8.2 pF from
   collector to base; 100 kΩ from base to ground.
   - It is a shunt-feedback (transimpedance) stage with high gain.
   - The 8.2 pF rolls the feedback off above 1/(2π · 3.9 MΩ · 8.2 pF) ≈ 5 kHz.
   - Note: 3.9 MΩ against 100 kΩ cannot bias the base on its own. The redraw may misplace the
     100 kΩ. The E7 has no such resistor.
3. **Tone node X:** collector → 0.22 µF → X.
   - From X to ground, a **series string**: [30 kΩ ∥ 0.1 µF] (Soft), [10 kΩ ∥ 15 nF] (Medium),
     [2 H ∥ 4.7 nF] (Treble), [0.6 H] (Brilliant).
   - Each switch is wired across its element. A pressed (active) switch is open: Gabrielli's
     "all switches active (open switches)" **[paper]**. With none pressed, X is shorted to
     ground and the instrument is silent, as the repairers say **[web, Chicago Electric Piano;
     FAQ §8]**.
   - The two inductors are the "two chokes" **[web, FAQ §12.3]**. The 4.7 nF disc sits between
     them and the 15 nF is a film cap **[web, clavinet.com upgrade]**.
4. **Stage 2:** X → 30 kΩ → 10 nF → base of **BC148B**. Bias 120 kΩ/20 kΩ; collector 10 kΩ;
   emitter 820 Ω + 1 kΩ (the 1 kΩ bypassed by 6.8 µF).
   - Gain ≈ 10 kΩ/(820 + r_e) ≈ 11 (21 dB) **[calc]**.
   - Input ≈ 120k ∥ 20k ∥ β·0.9k ≈ 16 kΩ **[calc]**.
   - Then 0.22 µF → 100 kΩ lin volume → jack. 9 V battery (6F22) or adapter; 100 µF across
     the supply.
5. **Coupling corners [calc]:**
   - 10 nF into the 30 kΩ + 16 kΩ stage-2 input: a high-pass near 300 Hz (exact value with the
     network's own impedance in §7.3).
   - 10 nF into stage 1: depends on the transformer's secondary impedance.
   - Gabrielli's simulation without the tone stack: −3 dB low shelf at 130 Hz, +3 dB high shelf
     at 4 kHz **[paper]**.
6. **Nonlinearity:** 1 % THD at 400 mV in (1 kHz), 3.6 % on rare ff chord peaks **[paper]**.
   Stage 2's swing on 9 V is ±3–4 V **[calc]**.

### 7.2 Why the series stack matters

- Gabrielli 2013 Table 3 gives each element's impedance Z_i correctly. But it treats the
  selected elements as a parallel network and implements it as a cascade H_1·H_2·… of
  per-element filters.
- In the circuit the selected Z_i **add** (they are in series), and the node voltage is a divider
  between the stage-1 source (≈ 51 kΩ ∥ load) and Σ Z_i ∥ (stage-2 input).
- Single switches come out about the same either way. Combinations do not: a series sum has
  **series resonances** (notches) where one element's capacitance meets another's inductance.
  A cascade cannot make those.

### 7.3 The tone network's response [calc]

- **Method:** V_X/V_src = Z_p/(R_s + Z_p), with Z_p = (ΣZ_i) ∥ (30 kΩ + 1/(sC_10n) + 16 kΩ), times
  the 16 kΩ share into the base.
- **Assumptions:**
  - R_s = 51 kΩ (the collector resistor). Shunt feedback lowers it; the 25 kΩ case moves
    everything up by ~5 dB, with the same shapes.
  - The inductors' DC resistances (60 Ω, 20 Ω) are guesses.
- Script: `scratchpad/tone.py` (not kept in the repo). dB relative to the source; only
  differences matter.

| Setting | 100 Hz | 400 Hz | 1 kHz | 1.64 kHz | 3 kHz | 5 kHz | 12 kHz | Shape |
|---|---|---|---|---|---|---|---|---|
| Soft | −33 | −34 | −40 | −44 | −49 | −53 | −61 | low-pass, peak ≈170 Hz: dark, quiet |
| Medium | −36 | −29 | −28.5 | −30 | −33 | −37 | −44 | broad hump ≈650 Hz |
| Treble | −52 | −31 | −19 | **−15.7** | −21 | −26 | −34 | resonant peak 1.57–1.64 kHz (2 H ∥ 4.7 nF) |
| Brilliant | −63 | −42 | −32 | −28 | −23 | −20 | −17 | +6 dB/oct high-pass, flattening above ~6 kHz (R/2πL) |
| Soft+Treble | −34 | −42 | −20 | −15.7 | −21 | −26 | −34 | Treble peak plus Soft's lows; dip ≈400 Hz |
| Soft+Brilliant | −33.5 | −38 | −37 | −29 | −23 | −20 | −17 | **notch ≈650–700 Hz** (0.1 µF with 0.6 H) |
| Treble+Brilliant | −50 | −29 | −18 | −15.7 | −30 | −23 | −17 | peak 1.6 kHz, **notch ≈3.4 kHz**, highs back |
| Medium+Treble | −36 | −29 | −22 | −15.7 | −20 | −24.5 | −32 | Treble peak on a fuller low-mid |
| Med+Treb+Brill | −36 | −28 | −20.5 | −15.7 | −26 | −25 | −17 | peak 1.6 kHz, dip ≈3 kHz, bright top |
| All four | −31 | −29 | −21 | −15.7 | −26 | −25 | −17 | the fullest; same peak and dip |

- **Treble's 1.6 kHz resonance** is in every setting that includes it, and is the "twang". With
  Brilliant added, the top comes back above a notch near 3 kHz: the nasal, quacky clavi.
- **Levels** span ~20 dB between settings (Soft alone is the quietest). The real instrument
  needs the volume reset when switching. A model should keep that (it is part of the
  instrument) but offer a trim.
- **E7 at R_s ≈ 12 kΩ** (§7.4): the same shapes, ~15–20 dB more level at the node, and the
  network shapes the sound a little less (a lower source impedance against the same Z). This
  fits the "drier" E7 **[web, forum]**.
- Implementation: for each of the 15 settings, a 4th–6th-order IIR fitted (bilinear transform of
  the exact rational function) at build time. Switching cross-fades over ~5 ms (a real rocker
  click, with a pop, is optional).

### 7.4 E7 and C [schematic]

- **E7:** the same transformer and tone network.
  - Stage 1: BC550C, collector **12 kΩ**, feedback **1 MΩ** ∥ 8.2 pF.
  - Stage 2: BC550C, bias 620 kΩ/100 kΩ, emitter **2.2 kΩ** + 1 kΩ (bypassed), collector 10 kΩ:
    gain ≈ 4.5. Volume 100 kΩ log.
  - Supply regulated to ≈ 7.6 V (BC238 + 8.2 V zener + 1.8 kΩ).
  - Lower gain and lower stage-1 impedance than the D6. The D6 "has more gain, deeper tone"
    **[web, Chicago Electric Piano]**.
- **C:** no transformer, no tone switches.
  - The pickups (two magnets each) feed T1 (AC151 germanium PNP, base 39 kΩ/6.8 kΩ, emitter
    1.5 kΩ) and T2 (collector 3.9 kΩ, emitter 620 Ω ∥ 25 µF). Output via 2 µF, a 3 nF shunt to
    ground, and 100 kΩ volume.
  - The germanium stages are blamed for the C's "gritty" sound **[web, Chicago Electric Piano]**.
  - For a model: a flat, slightly soft-clipping two-stage amp with a ~14 kHz roll-off.
  - The "Superstition" clavi **[web, FAQ; Wikipedia]**.

### 7.5 Noise

- Unshielded single coils on the D6 pick up hum from lights, transformers and dimmers. The FAQ
  advises a battery and keeping away from fluorescent lights **[web, FAQ §10]**.
- Model: hum at 50/60 Hz with harmonics up to ~1 kHz, about −60 dB relative to a mf note, plus
  white hiss shaped by the tone network. Default off or very low.

## 8. Published models and emulations

- **Gabrielli et al. 2013** (EURASIP), **ICMC 2010**, **Remaggi et al. 2012** (DAFx): covered
  in the earlier file §3.6.
  - Corrections here: the tone network is a series sum (§7.2); "10 bars × 6 strings" is six
    coils × ten strings; the bridge pickup is above.
  - Gabrielli left the dispersion out of the pickup comb to save 25 % of the cost. A modal model
    gets it exactly for free (the mode shapes).
  - Their real-time patch and samples were "to be shared" **[paper]**; not found.
- **Commercial:**
  - Logic/MainStage "Vintage Clav" (Emagic EVD6, physically modelled). Controls include pitch
    fall on release, tension modulation, pitch pressure, damping, excite shape (rubber hardness)
    and brilliance, key-off click with velocity/release-velocity, and pickup positions
    **[web, Apple]**.
  - a software maker clavi V (modelled mute and yarn damping) **[web]**.
  - the piano-modelling forum a commercial piano model CL1 **[paper ref. 25]**.
  - Samples: stage keyboard, sample libraries, a sample library. GSi Ticky Clav **[web, Wikipedia]**.
  - These name the features a player expects: pickup and tone switches, mute, release thunk and
    pitch fall, key-off click, a wah.
- **Related physics:** the clavichord's tangent (Jiolat et al., "Toward a physical model of the
  clavichord", JASA 2021: intermittent contact plus a Kirchhoff–Carrier string) **[web, search
  result only]**. It is the nearest published treatment of a tangent held on a string.

## 9. What makes it sound like a clavi, and how it is played

- **Physics (in order of weight) [calc, from §3–7]:**
  1. A sawtooth-like source at the very end of the string, sensed as velocity near the end:
     partials 2–3 over 1, a bright, buzzy attack whose top grows with key speed.
  2. The Treble network's 1.6 kHz peak and the Brilliant high-pass: the "twang".
  3. The pickup comb (every 3–5 partials on the centre pickup) and the out-of-phase setting's
     loss of the fundamental: nasal, hollow, "quack".
  4. Long sustain while held, killed fast on release with a three-semitone drop and a thunk.
     Short staccato notes are the idiom.
  5. The key knock and rebound, audible on high notes.
- **Playing:** percussive, staccato, interlocking two-hand riffs (like a funk rhythm guitar).
  The mute slider gives a palm-muted bass. Pressure vibrato on single lines **[web, Stiller via
  forum]**. Glissandi and hammer-on-like trills are idiomatic **[memory]**.
- **Effects** (the FAQ: "probably the most common effect is a wah-wah pedal. An envelope
  follower is also good" **[web, FAQ]**):
  - **Wah:** an inductor band-pass swept by the pedal. Cry Baby GCB-95 peak ≈ 450 Hz–1.6 kHz
    (750 Hz at mid travel; a dealer's chart says 350 Hz–2.2 kHz); inductor ≈ 0.5 H
    **[web, ElectroSmash and Sweetwater via search snippets]**. Q ≈ 2–6 **[memory]**.
    "Up on Cripple Creek" (1969) is the first wah'd clavi **[web, FAQ]**.
  - **Envelope filter:** Mu-Tron III, used on "Higher Ground" **[web, Wikipedia]**: a resonant
    filter (LP/BP/HP modes, Peak = Q, Range switch, up/down sweep) driven by the input's
    envelope **[web, effectsdatabase; filter type: memory]**.
  - Also phaser, chorus, compression **[web, FAQ; Wikipedia]**.
  - Amp: a tube amp, the clean 2x12 combo most cited **[web, FAQ]**.
- **Typical settings (not documented by players; [memory] unless tagged):**
  - Funk rhythm: Treble (+ Brilliant), with the bridge pickup alone (C/B) or both out of phase
    (D/B) for the thinnest. One maker's classic Clav samples use "treble" and the release thump
    **[web, Kurzweil manual via search]**.
  - Lead or warm comping: Medium (+ Treble), centre pickup or both in phase.
  - Soft alone: a muffled, bass-guitar-like tone, often with the mute.

## 10. Model plan (`-inst clavi`)

### 10.1 What is reused from the piano (`src/piano.h`)

| Piano part | clavi use |
|---|---|
| `PianoKey` len, tension, mu, B, f1, loss/b2/b3 | as is, per key, one string (`strings = 1`) |
| Modal banks x/v with decay folded into the rotation, SIMD loops | as is: the speaking bank |
| `damp_` per mode, damper place/length/rate | the yarn (on the joined bank) and the mute bar |
| `Bridge::kRigid`, `supportR` | the tailpiece end (and the rubber end as a per-mode loss) |
| `Output::kPickup` per-key gains | replaced: the pickup is no longer the bridge force but two velocity taps along the string |
| sleep/wake, active-key lists | as is (strings are uncoupled: no board, only sounding keys computed) |
| `tuneByPartials` | not needed (single strings, equal temperament on f1) |
| the plectrum exciter (being added in another session) | shares the "exciter drives modes through a row of weights" interface; the tangent is a third kind |

The `Piano` class is built around a bridge exchange and a board. The clavi needs none of
that and does need two banks per key. So it is cleaner as a sibling class (`src/clavi.h/.cc`)
that reuses the mode-bank helpers, not a `PianoDesign` variant. If the piano's bank code is not
factored out, factor it out first; the electric grand plan wants the same thing.

### 10.2 What is new

1. **Two banks per key.**
   - Speaking modes n = 1..N_s on L_s, and joined modes m = 1..N_f on L_t = 1.19 L_s (tuned
     2^(−3/12) lower, B scaled by (L_s/L_t)²). N_f ≈ 1.2 N_s.
   - A fixed projection P (N_f × N_s) per key, computed at build time:
     P_mn = (2/L_t) ∫_a^{L_t} sin(nπ(x−a)/L_s) sin(mπx/L_t) dx; P' for the way back.
   - Plus the static-bend vector s_m: the projection of the triangle (0 at 0, −h at a, 0 at L_t).
     F1 at 12 kHz: 106 × 126 ≈ 13 k multiply-adds once per release, nothing per sample.
2. **The tangent exciter.** A per-key end trajectory u(t):
   - a raised-cosine-smoothed ramp of height h over τ = h/v_t;
   - edge softening t_r ≈ 0.1–0.3 ms set by tip hardness;
   - an optional rebound (u dips by 0.1–0.3 h for 1–2 ms at ff).
   - Each sample: Δv_n = −(2/(nπ)) Δu̇ into the speaking bank (one row of weights per key, like
     the hammer's `hk_`). Velocity → v_t (0.3–1.5 m/s at the tip; Gabrielli's 1–4 m/s key speed
     × the lever ratio). h per key ≈ 1 mm (voicing).
3. **Release.** On key-off:
   - joined state = P·(speaking state) + s·h (displacement) and P·(velocities);
   - the speaking bank is cleared;
   - the joined bank runs with the yarn damper (rate per mode r·w_m/(2μ), w_m as in §5.1).
   - The release speed sets how fast the static bend is let go: the s·h term enters through a
     ramp over τ_r = h/v_release instead of at once.
   - A "sticky tip" option adds a lift-and-snap (a pluck at a of height 0.1–0.5 mm, random per
     note, scaled by a wear knob).
4. **Knock:** 4–6 frame resonators (150–1200 Hz) struck by the tip's stop. Their sum moves the
   string ends of the sounding keys (an end drive, as in item 2, with a small weight).
5. **Pickups:**
   - Per key and pickup, a row of weights w_n = sin(nπ d/L) (from the tailpiece end; d_C(key),
     d_B = 4 cm) for each bank.
   - Each sample: vertical displacement y_P = Σ w_n x_n/ω_n (with x = displacement·ω as in the
     piano), and velocity likewise.
   - Pickup EMF = g_P · p′(y_P) · ẏ_P, with p the Gabrielli polynomial scaled to a gap
     parameter. This is the chain rule, so there is no separate integrate and differentiate.
   - The horizontal direction enters at −25 dB.
   - Then the switch matrix (C, B, (C+B)/2, (C−B)/2, with gains g_C, g_B).
6. **Electronics:** ×15 (transformer, with a gentle low and high roll-off, say 40 Hz and 15 kHz);
   stage 1 (gain plus 5 kHz feedback roll-off); the tone network IIR per setting (§7.3); stage 2
   (×11) with a soft clip tuned to 1 % THD at 400 mV in; the volume.
   - Model variants: D6 (default), E7 (its values), C (no transformer or tone network, the
     germanium soft clip, 3 nF).
7. **Mute bar:** a damper band near the tailpiece end on both banks, rate × slider position,
   uneven per key at mid positions.
8. **Pressure:** aftertouch → per-key tension factor → every mode's turn rescaled at control rate
   (every 32 samples: cs/sn recomputed from ω(1+ε) for the active modes; cheap).
   Default range 10 cents.
9. **Tension modulation (optional):** ε(t) ∝ Σ n² q_n² per key, applied the same way. A few
   cents at ff on the attack.
10. **Effects:** a wah (an SVF band-pass, 350 Hz–2.2 kHz, Q 2–6, on a pedal CC) and an envelope
    filter (Mu-Tron-like: an envelope follower driving the SVF; LP/BP/HP; up/down). Neither
    exists in `src/` (`lead.cc` has a phaser only), and both are small. Then the project's
    amp/room as for the other keyboards.

### 10.3 Parameters to start from

| Parameter | Start | Range / note |
|---|---|---|
| L_s per key | §4.2 table, interpolated log-linearly | fit from comb nulls (§10.6) |
| B per key | Gabrielli table 1, interpolated | fit per key |
| Wound / plain | keys 1–23 / 24–60 | a level and loss step at the change |
| h (string-anvil gap) | 1.0 mm | 0.5–2 mm; sets the low-partial level |
| v_t (tip speed) | 0.3 (pp) – 1.5 (ff) m/s | sets brightness |
| Tip edge t_r | 0.2 ms | 0.05 (hard, new) – 0.5 ms (worn) |
| a / L_s | 0.19 | fixed: −3 semitones |
| T60 held | 20 s @100 Hz, 8 s @1 kHz, 1.5 s @5 kHz | ripple ±20 % over 2–3 partials |
| Yarn | highs T60 30 ms; dropped fundamental ≈0.2 s | 10–100 ms; "worn yarn" knob |
| Pickup distances | C: 18.5→6.5 cm; B: 4 cm | heights/gains g_C, g_B ±6 dB |
| Pickup nonlinearity | Gabrielli p0..p4 | gap scale |
| Horizontal / vertical | −25 dB | |
| Beating | from the two polarisations, tuned 0.5–2 Hz apart below E4, a random share of keys | 15 dB p-p max |
| Knock | 4–6 modes 150–1200 Hz | level by velocity |
| Mute | band at 3–8 cm from the tailpiece end | 0.1–0.5 s decay at full |
| Aftertouch | +10 cents at full | 0–50 |
| Model | D6 | D6, E7, C |

### 10.4 Cost [calc]

- Ten held voices × ~60 modes × 2 directions ≈ 1200 oscillators. At the piano's measured
  0.33 ns per oscillator-sample at 48 kHz that is ≈ 19 µs per ms of audio: **≈ 2 % of a core**.
- Released strings add briefly (their joined banks live ~0.3 s).
- Pickup rows: two more multiply-adds per mode per sample, which can be fused into the update
  loop: +30–50 %.
- The electronics per voice-sum (not per voice): ~30 multiplies per sample.
- Total **1–3 % of a core** for typical playing. All 60 keys held at 16 kHz: 7800 oscillators,
  about 12 %.

### 10.5 What not to do

- Don't use Gabrielli's cascade for the tone switches (§7.2), or a fixed knock sample.
- Don't build ad-hoc beating equalizers before trying two polarisations with slightly different
  terminations (the earlier file's suggestion stands).

### 10.6 Validation

1. **Static:** 60 keys F1–E6; joined pitch exactly 3 semitones below on every key; total modes
   ≈ 3200/direction to 12 kHz.
2. **Spectra of single notes** (DI recordings, known switch setting):
   - partial 2 ≥ partial 1 + 3 dB;
   - comb nulls at the right partials per pickup. **The null index gives L/d, and d is known
     (§6.1), so this measures each string's length**: a scale fit with no instrument to hand;
   - out of phase: fundamental 4–15 dB down by register (§6.3 table).
3. **Velocity:** pp against ff on one key. Expect low partials within a few dB and the 1–4 kHz
   band +10–15 dB. If the fundamentals also grow a lot, h is not the whole story; then make the
   end displacement grow with v_t.
4. **Decays:** T60 per partial with `tools/pianofit.py` (it already fits per-partial decay, B and
   tuning from isolated notes). Targets: ≥ 20 s low partials in the bass and middle, the ripple,
   beating below E4.
5. **Release:** f0 of the release tail = 2^(−3/12) f0; its T60 per partial (to fit the yarn);
   the thunk's level relative to the sustain at release time.
6. **Tone switches:** the 15 settings' long-term spectra on a chord against the §7.3 table
   (differences only).
7. **Recordings** (only ones the user owns, measured for parameters): exposed clavi in
   "Superstition" (C, overdubs), "Higher Ground" (Mu-Tron), Herbie Hancock *Head Hunters*,
   Led Zeppelin "Trampled Under Foot", Bob Marley "Could You Be Loved" **[web, Wikipedia]**.
   Most are processed (wah, filter), so they serve for listening and attack and release timing
   more than for spectra. Isolated DI notes (an owner, or a sample library the user owns) are
   what the fit needs.
8. **A/B listening:** stage by stage (string and pickup; plus tone; plus amp; plus wah), as for
   the electric grand.

## 11. Open questions

- Real string lengths and gauges per key: measure on an instrument or fit from comb nulls
  (§10.6). The first plain string's gauge (E3: B says thicker than .011").
- h, the tangent travel, the tip rubber's stiffness: one owner with calipers would settle them.
- The yarn's decay times, and the mute bar's position and pads.
- The size of the pressure bend on a well-kept instrument (a few cents by the geometry; a
  quarter-tone claimed).
- Stage 1's real output impedance (the D6 bias puzzle in §7.1) and the transformer's secondary
  impedance and bandwidth. A SPICE run of the schematic would pin §7.3's absolute levels; the
  shapes are robust.
- The early clavi's two-magnet pickups: their effect on the comb and the nonlinearity.

## 12. Sources

- L. Gabrielli, V. Välimäki, H. Penttinen, S. Squartini, S. Bilbao, "A digital waveguide-based
  approach for Clavinet modeling and synthesis", EURASIP J. Adv. Signal Process. 2013:103.
  https://www.ness.music.ed.ac.uk/wp-content/uploads/2013/06/Gabrielli.pdf (read in full: §2,
  §3, tables 1–3).
- L. Remaggi, L. Gabrielli, R. C. D. de Paiva, V. Välimäki, S. Squartini, "A pickup model for
  the Clavinet", DAFx-12. https://dafx.de/paper-archive/2012/papers/dafx12_submission_62.pdf
- L. Gabrielli, V. Välimäki, S. Bilbao, "Real-time emulation of the Clavinet", ICMC 2010/2011
  (not retrievable: repository blocked). https://www.research.ed.ac.uk/en/publications/real-time-emulation-of-the-clavinet-an-electromechanical-keyboard/
- Hohner Clavinet D6, E7 and C schematics, translated and annotated by The Chicago Electric
  Piano Co. (2015):
  https://cdn.shopify.com/s/files/1/1002/4874/8398/files/Clavinet-D6-Schematic.jpg ,
  https://cdn.shopify.com/s/files/1/1002/4874/8398/files/Clavinet-E7-Schematic.jpg ,
  https://cdn.shopify.com/s/files/1/1002/4874/8398/files/Clavinet-C-Schematic.jpg ; pages
  https://chicagoelectricpiano.com/blogs/news/clavinet-schematic-wiring ,
  https://chicagoelectricpiano.com/blogs/news/hohner-clavinet-schematic-tone-controls ,
  https://chicagoelectricpiano.com/blogs/news/clavinet-controls-and-settings
- A. Kipness, D. Berg, "The Hohner Clavinet FAQ" v1.6, clavinet.com, 2002.
  https://www.clavinet.com/clavfaq.pdf
- Clavinet.com, identification guide https://clavinet.com/ident.php ; D6 preamp upgrade
  https://clavinet.com/d6upgrade.php
- D. Tillman, Clavinet notes (pickup switches). https://till.com/articles/Clavinet/
- GroupDIY, "Clavinet E7" thread (regulator, transistors, supply). https://groupdiy.com/threads/clavinet-e7.38507
- Vintage Vibe transistor pinout chart (D6 BC550C/BC148B; E7 BC550C/BC238).
  https://cdn.shopify.com/s/files/1/0666/2821/files/Transistor_Pinout.pdf (via search snippet)
- Clavinet, Wikipedia. https://en.wikipedia.org/wiki/Clavinet
- Hohner 1973 catalogue (D6 size, weight, range), via https://hohner-pianet.blogspot.com/2021/06/clavinet-d6.html
- Clavinet D6 owner's manual (tuning, mute slider). https://www.manualslib.com/manual/742842/Clavinet-D6.html
- Modartt forum, "Clavinet aftertouch" (Stiller quotation; owner's report). https://forum.modartt.com/viewtopic.php?id=5538
- Sonic Couture, last-note pitch bend script. https://www.soniccouture.com/blog/?p=94
- Apple, Logic Pro "D6 Clavinet mechanical details" and EVD6 parameters.
  https://help.apple.com/logicpro/mac/10.4/en.lproj/lgsifbfa41d5.html ;
  https://help.apple.com/logicpro/mac/9.1.6/en/logicpro/instruments/chapter_8_section_3.html
- Musicplayer forum, "D6 key dip" (6–8 mm). https://forums.musicplayer.com/topic/164568-d6-keydip-query
- Wah: ElectroSmash Cry Baby GCB-95 analysis (via search snippet; site unreachable)
  https://www.electrosmash.com/crybaby-gcb-95 ; Sweetwater wah guide https://www.sweetwater.com/feature/dunlop/wah-pedals/
- Mu-Tron III. https://en.wikipedia.org/wiki/Mu-Tron_III ; https://www.effectsdatabase.com/model/musitronics/mutron3
- Arturia Clavinet V. https://www.arturia.com/products/software-instruments/clavinet-v/overview
- Jiolat et al., "Toward a physical model of the clavichord", JASA 2021 (via search; not read).
  https://hal.univ-lorraine.fr/IJLRDA-LAM/hal-03369924v1
- This repository: `research/harpsichord_clavi.md` §1, §3, §5; `research/electricgrand.md`;
  `src/piano.h`.

## 13. Fitted to recordings (2026-11-15)

Four recordings of a D6 (`~/Music/clavi*.wav`, mostly mono, riffs), measured with
`tools/pianofit.py`: 21 notes found clean enough, most between E2 and D#4.

- **Stiffness:** 3.5–8e-5 from E2 to D#4 (D#3, six takes: median 4.7e-5; E2–G2 6.5–7.9e-5;
  A#3 5.4e-5). That is half of Gabrielli's table there, and what §4.2's plain-wire calculation
  gave for E3 (5.3e-5). The model now uses these values (`src/clavi.cc` kStiff); bass and top
  keep Gabrielli's (no recorded notes there).
- **Setting:** against the six D#3 takes (partials 1–16), the model's default (both pickups in
  phase, Medium+Treble) is the nearest of 32 pickup/tone settings, 8.6 dB rms. Partials 1–4 agree
  within 4 dB. The recorded comb's first gap is at partials 5–7, the model's at 7–8, and the
  recording has no second gap near partial 11. Moving the centre pickup alone (×0.8–1.45) makes
  it worse: the comb needs more keys, or a known setting, to fit the lengths and distances.
- **Decays:** not fitted. The notes are short (0.3–0.4 s to the next) and the three longer ones
  disagree (some partials grow: overlap, an effect or a compressor).
- **Tuning:** about −3 c overall (the recorded instrument's).

