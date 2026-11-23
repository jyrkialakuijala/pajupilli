# The picked electric bass (GM 35, `-inst pick-bass`): research

Status: research 2026-11-20; **built the same day** (`-inst pick-bass`; see §12, "As built", for
what was built and where it departs from §9). Before the build GM 35 "Electric Bass (pick)" fell
back to `double-bass` (`src/midifile.cc` kGm: `{"pick-bass", "double-bass"}`, which now finds the
pick bass first), and the "bass-stack" amp preset was unused.

This document adds to three earlier ones and does not repeat them:
- `research/bass_section.md`: strings, inharmonicity, losses, neck dead spots, P and J pickup
  positions and coils (§2–5), and the first short notes on the pick (§6.2).
- `research/bass_amps.md`: the fliptop, bass stack, DI, cabinets and compression, and the "bass-stack" preset for
  GM 35 (§10.2) with its checks (§10.6).
- `research/electric_guitars.md` §7–8 and `research/steel_guitar.md` §4.1: the pick and the palm
  on the guitar.

What is new here: the instruments a pick bass usually is, the pick against the bass string
*measured in the project's own string model* (finger against pick, pick place, contact time,
stroke direction, palm mute, flats), the three SoundFonts' GM 35 read from the files and
measured from TiMidity renders, the user's MIDI files, the players, and a build plan.

How each fact is marked:
- **[T]**: tested or measured here, for this document: SF2 data read with a small parser, notes
  rendered by TiMidity and measured, the project's `Guitar` string model run in a scratch harness
  (below), the user's MIDI files scanned.
- **[web, source]**: read on a web page (interview, maker's or retailer's page, data list, forum).
  The source key is listed in §11. Forum and fan-site claims are labelled as such.
- **[paper]**: from a paper (the key is in §11).
- **[memory]**: general knowledge, not looked up for this document. Check before relying on it.
- **[E]**: my estimate or arithmetic from marked numbers (a fifth mark, needed for the build
  numbers; it is never a measurement).

**The scratch harness** (`pickbass_harness.cc` in the session scratchpad, not in the tree): the
project's `src/guitar.cc` as it is, with `jazzBass()`'s string numbers copied, and either the J
pickups of `src/ebass.cc` or a P split coil (E/A half at 149 mm, D/G half at 124 mm; 6.4 H,
10.5 kΩ). Its output goes through `ElectricBassModel`'s DI biquad (coils, pots, 700 pF cable into
1 MΩ). There is no amp or cabinet, so these are DI signals. The finger is `pluckPad` as
`ElectricBassPlayer` calls it at velocity 0.8 (3.7 mm deep, 0.42 rad into the body). The pick is
the existing `Guitar::pick` (0.8 mm edge, 0.15 ms slip, rise clamped to 0.3–4 ms). "hf" is the
energy above 2 kHz in the first 100 ms against all of it. "Centroid" is the first 100 ms.

---

## Short answer

1. **Which bass.** The GM "pick bass" is an American amp **Precision** with **roundwounds**, picked over
   or near the pickup, into an amp: a bright, mid-forward note with a defined attack **[E, from
   §5 and §6]**. FluidR3's GM 35 is a real bass sampled string by string and fret by fret. Its
   open E has a dip at the 6th harmonic, which is where the P's E/A half-coil has its comb notch
   **[T]**. Variants are the 1960s session sound (P, flatwounds, a felt mute, a hard pick:
   Carol Kaye) and the through-neck bass growl (Squire, Lemmy). These belong behind knobs, not in the
   default.
2. **Pick against finger, measured in the project's string model [T].**
   - At the same depth and angle, the existing `Guitar::pick` gives **+25 dB more energy above
     2 kHz** than the finger (hf −27 dB against −52 dB), and the 8th–16th harmonics come up
     10–20 dB.
   - It is **4.6 dB quieter**, because the model's pick is a force pulse of about 1 ms, shorter
     than a quarter of E1's 24 ms period. The low modes get an impulse, not a displacement.
   - With a 4 ms rise the same pick is as dark as the finger (centroid 160 Hz against 167 Hz).
   - So **in this model, contact time is the brightness knob, and the real pick's physics is
     not there yet.** A real pick displaces the string quasi-statically (milliseconds) and
     releases it fast (0.1–0.3 ms). It brightens without losing the fundamental.
   - **The new code a pick bass needs is a displacement-controlled pick contact:** a stiff
     spring moving with the hand, then a slip.
3. **What the SoundFonts do [T].**
   - In all three, velocity changes the level only: +10 dB from v40 to v80 and +6 dB from v80
     to v120 in TiMidity, with the spectrum identical to the dB.
   - FluidR3: 9 samples (3.4–5.8 s, 22.05 kHz) with a flat 3–4 s hold, so the decay is the
     recording's own, 2–5.5 dB/s.
   - Jnsgm2: 13 short looped samples under an envelope that decays 4–13 dB/s.
   - TimGM6mb: three single-cycle loops with a filter envelope that TiMidity does not play.
   - Against their own finger basses, FluidR3's pick is 15–25 dB stronger at 2–4 kHz in the
     attack and has a 1.3–1.9× higher centroid. Jnsgm2's is 1.5–2× brighter on the upper notes.
     There is no separate click: the >2 kHz energy lasts through the first 100 ms instead of
     spiking.
4. **The user's MIDI files [T].** 397 files: only one uses GM 35, for six notes
   (`~/Music/midi/bushgrafts/desire.mid`, channel 2, an intro on A1–E2 before it switches to the
   fretless). The finger bass (4195 notes) stands in for how these lines are written:
   - median note length 0.21 s (p10 0.10, p90 0.61);
   - median onset interval 0.32 s;
   - keys G1–G#3 (1–99 %), median G2;
   - median velocity 90.

   Short, detached, mostly on the E, A and D strings.
5. **Palm mute [T].** The existing `Guitar::mute` on the bass string gives the muted pick sound:
   - depth 5 at 12 mm: 45 dB/s, the centroid down from 383 to 101 Hz, only the fundamental left
     at 1 s;
   - depth 10: 220 dB/s;
   - depth 2: 20 dB/s, which is the felt-under-the-bridge-cover range.

   No new mute code is needed; the work is in the numbers and the player.
6. **The build** (§9): a `precisionBass()` spec, a `PickBassPlayer` (down/up strokes, place,
   pick, mute), a new `Guitar::pickContact` (displacement-controlled push, then a slip), and the
   "bass-stack" amp preset as it is. The tests are in §9.6.

---

## 1. What the project has (the parts a pick bass reuses)

| Part | Where | State for GM 35 |
|---|---|---|
| String model, 4 strings, neck modes, two polarisations, P/J pickups as taps with width, magnet nonlinearity | `src/guitar.h/.cc`, `jazzBass()` in `src/ebass.cc` | reuse as is; add a P spec |
| Pick | `Guitar::pick(s, depth, from, angle, rise)`: a force rising over `rise` (clamped 0.3–4 ms), then a 0.15 ms slip off a 0.8 mm edge; `hold` 40/s; does not stop the ringing string | works, but is a force pulse; see §4.2 |
| Finger | `Guitar::pluckPad` | the GM 34 player |
| Palm | `Guitar::mute(s, depth, at)`: each mode loses `depth`·(its shape² under a 12 mm hand)/(the fundamental's), saturating at `palmSat` = 30× | reuse; the numbers are in §4.5 |
| Player | `ElectricBassPlayer`: the fingering solver, one-voice lines, note-off damping, bends | copy its solver; replace the fingers with a pick |
| DI | `ElectricBassModel`'s biquad (coil + pots + cable into 1 MΩ) | reuse with P values |
| Amp | `GuitarAmp::bassStack()` (`src/guitaramp.cc` 204–245): bass stack preamp and James stack, mid +3 dB at 800 Hz, 6550 power with sag, 8×10 + kick-drum microphone, DI 35 %, FET compressor 4:1 0.8 ms/150 ms; knobs `PJ_BAMP_*` | reuse; built for exactly this |

---

## 2. The instruments

### 2.1 split-coil bass (the default)

- **What sets its sound** is the split coil. The E/A half sits about 148–152 mm from the saddle
  (0.172 L) and the D/G half about 121–125 mm (0.142 L). Each coil is 6.4–7 H and 10.5–12 kΩ;
  one volume and one tone (250 k, 0.047 µF) **[bass_section.md §3, web: pickup-position-db,
  American amp]**.
- **Combs** (from bass_section.md §3.1's table): the E/A half has its notch at the open string's
  6th harmonic (−19 dB), the D/G half at the 7th (−37 dB) and the 14th.
- **The loaded resonance** is near 2.0–2.4 kHz with 700 pF. That is less top than a J (4.4 kHz
  both up) **[E, bass_section.md §3.3]**.
- **Strings and the pick** [T, harness]:
  - With a pick at 10 cm, the P's open E gives centroid 379 Hz, against 360 Hz for the J both
    up at the same place.
  - The P's 2nd–5th harmonics are within 6 dB of each other, and its 6th is down to −19 dB:
    the comb notch, as predicted.
  - The J both up has its notch at the 8th–9th harmonic (−33, −37 dB).
- **In FluidR3's GM 35** the open E (first 100 ms) reads, harmonics 1–8:
  −7 / −2 / 0 / −2 / −8 / −16 / −21 / −22 dB. A dip at 6–7 fits a P's E/A coil better than a
  J **[T]**. The evidence is moderate: the 7th is also low, and an amp and a mic sit between.

### 2.2 through-neck bass 4001 / 4003

- **Build** **[memory]**:
  - neck-through maple body, 33¼ in. (845 mm) scale, 20 frets;
  - two single coils: the neck "toaster" at the end of the fingerboard, and the bridge pickup
    close to the saddle under a metal "ashtray" cover (on older ones);
  - two volumes and two tones; the "Rick-O-Sound" stereo jack sends each pickup out on its own.
  - The 4003 (1979–) has a sturdier truss rod and later a different bridge pickup.
- **Pickup DC resistance** is "about 8–8.5 kΩ" by owners; one rewound bridge pickup read 15.4 kΩ
  against a neck 11.9 kΩ **[web, RickResource forum; not makers' figures]**. No inductance or
  position measurements were found.
- **Its sound** **[memory]**: the bridge pickup very near the saddle gives a thin, nasal, "clank"
  register, with a fundamental 15–20 dB under the 5th–10th harmonics, like the J bridge '70s
  column of bass_section.md §3.1. Roundwounds, a pick and amp grind give the growl.
- **For the model:** a `throughNeckBass()` spec (845 mm, neck tap ~180 mm and bridge tap ~40 mm from
  the saddle **[E, unmeasured]**). It is a variant, not GM 35's default.

### 2.3 active single-humbucker bass

- **Pickup and preamp:**
  - one large humbucker (two coils, wired in parallel) set "well back toward the bridge", closer
    than a P's **[web, Wikipedia StingRay; Seymour Duncan]**;
  - the 1976 original has a 2-band active preamp on 9 V; later ones have 3 bands, some on 18 V
    **[web, Wikipedia; Music Man spec pages]**.
- **No measured pickup position was found.** About 95–100 mm from the saddle to the centre, with
  the two coils ~20 mm apart, is lore-level **[memory]**.
- **Its sound** **[memory]**: a strong, scooped, bright bass with an active treble boost. It is
  used with fingers, slap and pick, the pick mostly in modern rock.
- **For the model:** a humbucker tap pair (`Pickup::apart` ≈ 0.02 m), at about 0.1 m. The
  active preamp is a low-impedance output: no coil resonance, so no 2 kHz bump, and a flat top
  to 10 kHz.

### 2.4 Flatwounds against roundwounds

- **What they are** **[memory]**:
  - Flats have a flat ribbon as the outer wrap. The surface is smooth (no finger squeak), the
    tone thumpy and short.
  - Rounds are the EXL170 kind. Flats usually come heavier: .043–.105 or .050–.105, at higher
    tension.
- **Claims:**
  - "Roundwounds tend to have more sustain and a brighter, ringing sound"; flats sound "mellower,
    similar to an upright" **[web, Ernie Ball]**;
  - rounds give "more harmonic overtones … at the same length and gauge" **[web, Sweetwater]**.
- **No controlled measurement** of their spectra or decays was found **[web search, §10]**.
- **Mechanism** **[E]**:
  - the flat wrap's turns press against each other, which adds friction loss growing with
    frequency (loss1 up);
  - the wrap adds bending stiffness, so B is up a little;
  - the surface is smoother, so a pick slides off with less scrape.
- **In the harness [T]**, flats as `loss1` × 2.5 against rounds:
  - pick on the open E: the decay from 0.3 to 1.5 s went from 3.6 to 6.6 dB/s;
  - at 1 s the 5th–8th harmonics are 2–3 dB lower;
  - the attack's spectrum is unchanged (the first 100 ms is the pluck, not the losses).

  So flats are mostly a decay shape, not an attack shape. The ×2.5 is a guess to fit against
  recordings.
- **Which GM 35 is:** the SoundFont pick basses are bright and ring 2–5.5 dB/s (FluidR3), so
  they are rounds **[T, §5]**. The 1960s pick-on-flats session sound is the variant (§3.1).

---

## 3. Players and styles, and what each implies for the model

| Player / style | Bass, strings | Pick technique | Chain | For the model |
|---|---|---|---|---|
| **Carol Kaye** (LA sessions 1963–70s; Beach Boys, Motown's LA sessions, film) | American amp P; flatwounds (Thomastik JF344 heavy is reported) **[web, Reverb; GuitarWorld]** | a **hard teardrop pick**; "pick at the end of the neck, not anywhere near the bridge"; a doubled piece of felt taped over the strings where they leave the bridge "cuts the overtones and the undertones" **[web, Reverb, GuitarWorld, Premier Guitar]**; "98 % of the things cut in Hollywood in the '60s were done with a pick on flatwound strings" **[web, GuitarWorld]** | DI and/or a small amp (fliptop) **[memory]** | pick at **22–27 cm** from the saddle (the neck's end on a 20-fret P: 0.864·2^(−20/12) = 27 cm **[E]**); a **light, permanent mute** near the saddle (`mute` depth ~1.5–2.5 at 8–20 mm, §4.5); flats; stiff pick, so a fast release |
| **Paul McCartney** | short-scale violin bass (short scale, flats) to 1965; then through-neck bass 4001S with roundwounds (Rotosound) from about *Rubber Soul* / *Paperback Writer* **[memory]** | pick, melodic, often with the pick near the neck pickup **[memory]** | Vox, American amp tweed 4x10 amp, DI at Abbey Road **[memory]** | the Rick spec; neck pickup; a moderate amp |
| **Chris Squire** (Yes) | through-neck bass 4001, Rotosound Swing Bass roundwounds, standard gauge **[web, GuitarWorld]** | heavy pick, hard attack, "between the pickups" (forum) **[web, Harmony Central]** | stereo: neck clean for the low end, bridge to fuzz/a British stack guitar amp **[web, GuitarWorld]** | the Rick spec, two outputs: a clean low path plus a driven bridge path, blended; bright rounds |
| **Lemmy** (Hawkwind, Motörhead) | through-neck bass 4001/4004LK, rounds **[memory]** | heavy pick, near the bridge, chords and drones (open A/E under fretted D/G notes) **[web, GuitarWorld; guides]** | British stack Super Bass 100 W: "bass off, treble off, middle full on, presence at about three o'clock, volume at two o'clock, no effects" **[web, GuitarWorld interview]** | double stops and drones (the player must let strings ring together); a guitar-type British stack preset with mids full and heavy power-amp drive; 4×12 or 4×15 cabinet |
| **Punk** (Dee Dee Ramone, Paul Simonon, J.J. Burnel) | P basses, rounds **[memory]** | **downstrokes only** on fast eighths (Ramones), palm on or off **[memory]** | bass stack or British amp with grind **[memory]** | down-only at IOI ≥ 0.15 s; the same stroke every time (the variance from §4.4 only); grind on |
| **1970s rock pick, mid-forward** (John Deacon, Roger Glover, Geddy Lee early) | P/Rick, rounds **[memory]** | pick over the pickup, alternate | bass stack, miked 8×10 + DI **[bass_amps.md §2.2]** | the default: P, rounds, alternate strokes, ~10–12 cm, "bass-stack" |
| **Modern rock** (Mike Dirnt, Duff McKagan, Chris Wolstenholme, Justin Chancellor) | P / PJ / active single-humbucker bass / Wal, rounds **[memory]** | pick near the bridge, hard, palm-muted eighths in verses, open in choruses **[memory]** | DI + tube-emulation DI-type drive + amp, heavy compression, a high-pass on the drive path **[memory; bass_amps.md §5.3]** | mute on low velocities or by a CC; a parallel drive (exists in guitaramp as the "tube-emulation drive") |

Common ground: a stiff pick (≥ 0.88 mm), roundwounds except for the 1960s session sound, the
pick between the neck's end and the bridge depending on style, alternate picking except punk,
and a palm or felt mute as a style switch.

---

## 4. The pick on a bass string

### 4.1 Picks

- **Thickness:** guitar sizes are 0.38–0.6 mm thin, 0.6–0.8 medium, 0.8–1.2 heavy and 1.5+ jazz
  **[memory; steel_guitar.md §4.1]**. Bassists mostly use 0.88–1.5 mm **[memory]**; Kaye uses "a
  hard pick" **[web]**.
- **Materials:** celluloid, Delrin/Tortex (acetal), nylon (flexible, grippy), polycarbonate.
  Felt picks exist for a soft, finger-like attack (McCartney is said to have tried them)
  **[memory]**.
- **What thickness does** **[E]**:
  - A pick is a cantilever. The string slips off when its restoring force F_s at the contact
    exceeds what the bent pick can hold at its tip angle.
  - A stiff pick holds until the string is far aside. So it gives a large release displacement,
    a loud note, a fast slip and a bright note.
  - A thin pick bends first. The string leaves at a smaller displacement, and the pick then rings
    on its own (a few hundred Hz to a few kHz; acoustic, not heard by the pickup
    **[electric_guitars.md §7]**).
  - On a bass string (155–216 N, against 60–75 N on a guitar) the restoring force at a given
    displacement is 2–3× higher. So the same pick bends more on a bass, and a thin pick is
    "flappy" there. This is why bassists choose heavy ones **[E]**.
- **A secondhand Vienna study** (thin picks brighter, thick picks louder and mellower) is
  **unverified** **[web, via steel_guitar.md]**.
- **The static force** for a displacement d at x_p from the saddle on a string of length L is
  F = T·d·L / (x_p (L − x_p)) (as in `Guitar::pick`). On the E1 (155 N, L 0.864) at x_p = 0.10 m,
  d = 2 mm gives **F ≈ 7 N**. At 3 mm, **10.5 N** **[E]**.
  - A 1 mm Delrin pick sticking out about 8 mm bends ~1–2 mm under 5–10 N **[E, beam
    arithmetic: E ≈ 3 GPa, width ~8 mm]**.
  - So on a bass, the pick's own spring is part of the contact: model it, don't treat the pick
    as rigid.

### 4.2 Contact time, release, the attack and the "click"

- **How a plectrum releases:** harpsichord plectrum–string interaction, filmed, splits into
  sticking and slipping. The slip is **26–36 % of the whole contact** (normal against fast tempo)
  **[paper, Le Carrou SMAC 2013]**. The harp has the same three phases (stick, slip, free)
  **[paper, Chadefaux 2012]**. No absolute contact times for a guitar or bass pick were found
  (§10).
- **Hand speed and the push** **[E]**:
  - A picking stroke on eighths at 120–180 bpm moves the hand 1–2 cm in 80–125 ms, a mean of
    0.1–0.25 m/s, with peaks maybe 0.5–1 m/s.
  - Pushing the string 1–3 mm aside then takes **2–20 ms**, and the slip **0.1–0.5 ms**.
  - The push is slow against E1's 24 ms period only at its slow end. The string is close to its
    static triangle shape at release only if the push is longer than about a quarter period
    (6 ms on E1, 2.5 ms on G2).
- **What the existing model does [T, harness]:** `Guitar::pick` is a force ramp of `rise` (≤ 4 ms)
  and then a 0.15 ms slip, with no hold. P bass, open E, at 10 cm:

| Excitation | RMS first 100 ms | Centroid | hf > 2 kHz | Harmonics 8–12 against the strongest |
|---|---|---|---|---|
| finger (`pluckPad`, 3.7 mm, 0.42 rad, rise 2.2 ms, slip 0.7 ms) | −22.0 dB | 167 Hz | −52 dB | −14 … −26 |
| pick, 3.7 mm, 0.42 rad, rise 1 ms | −26.6 | 373 | −27 | −8 … −18 |
| pick, 2.5 mm, 0.25 rad, rise 0.3 ms | −38.9 | 727 | −17 | −8 … −17 |
| pick, 2.5 mm, 0.25 rad, rise 1 ms | −33.7 | 379 | −26 | −8 … −18 |
| pick, 2.5 mm, 0.25 rad, rise 3 ms | −28.6 | 177 | −31 | −12 … −26 |
| pick, 2.5 mm, 0.25 rad, rise 4 ms | −27.5 | 160 | −32 | −15 … −29 |

  - A shorter rise makes the note quieter *and* brighter: the low modes see an impulse F·t
    instead of a displacement F/k, a loss of about ω₁·t (−12 dB at 1 ms for E1).
  - A real pick does the opposite: it is loud *and* bright, because the push is slow and the
    slip is fast.
  - **In the model, brightness and level are tied through `rise`; in the real thing they are
    separate (push time against slip time).** That is the gap a pick contact model must close
    (§9.2).
- **The attack transient.** Through a magnetic pickup the "click" is not the pick's own sound;
  the pickup does not hear the pick **[electric_guitars.md §7, E]**. It is:
  - the string's high partials from the narrow, fast release, up to mode L/b (1 mm contact:
    mode ~860, beyond every modelled partial) **[paper, Fletcher & Rossing §9.3, via
    steel_guitar.md]**, low-passed by the slip time (0.15 ms: roughly −3 dB at 3 kHz **[E]**);
  - the first half-period's sharp corner (the Helmholtz corner leaving the pick);
  - on roundwounds, a scrape when the pick's edge crosses the wrap at an angle (pulses at slide
    speed / wrap pitch, 0.5–0.7 mm: at 0.3 m/s, 0.4–0.6 kHz with harmonics) **[E]**. It reaches
    the pickup only through the string's transverse motion, so it is weak.
  - Measured in the harness: the pick's >2 kHz energy does not spike in the first 10 ms against
    10–100 ms (ratio 0–2 dB). The finger's does (+11…+18 dB: its only highs are the onset).
    The pick's highs are *in the note*, decaying with the string's own losses.
  - The SoundFonts agree: FluidR3's ratio is −13…−30 dB (nothing extra at the onset, §5) **[T]**.
  - So **no added noise burst** is needed for the DI sound. The acoustic pick click (−20…−30 dB,
    2–8 kHz, < 1 ms) belongs only to a miked-room variant, if ever.
- **Stroke direction [T, harness]:** the upstroke (angle π + 0.12, the string lifted slightly
  from the body) against the downstroke (+0.25, into the body), same depth: **−4.7 dB**, the
  centroid +3 % (390 against 379 Hz). That is about the sin(angle) ratio of what the pickup
  hears (0.12/0.25 = −6.4 dB) **[E]**.
  - Players report upstrokes as slightly weaker and different **[memory]**. Our 4.7 dB is
    probably too much for a skilled player, who compensates.
  - Take the up angle at π + 0.18 and +1.5 mm depth so the up/down difference is 1.5–3 dB
    **[E]**. A test pins the gap (§9.6).

### 4.3 Pick position

- **Harness, P bass, open E, pick 2.5 mm, rise 1 ms [T]:**

| From saddle | RMS | Centroid | Harmonics 1–8 |
|---|---|---|---|
| 6 cm (near the bridge) | −29.7 | 402 | −17 / −7 / −3 / −2 / −6 / −18 / −3 / 0 |
| 10 cm (over/behind the pickup) | −33.7 | 379 | −13 / −3 / 0 / 0 / −6 / −19 / −8 / −12 |
| 15 cm (over the E/A coil) | −35.2 | 389 | −11 / −2 / 0 / −3 / −16 / −35 / −9 / −2 |

  - The pluck's own comb (notches at k = L/x_p: 14.4, 8.6, 5.8) and the pickup's comb (k ≈ 5.8
    on the E/A coil) multiply. At 15 cm they coincide (both at ~5.8), which gives the deep
    6th-harmonic hole (−35 dB): a hollow, "over the pickup" tone.
  - Near the bridge (6 cm) the fundamental drops by 4–6 dB against the 3rd–8th: thin and
    aggressive.
  - (The 6 cm level is higher here only because the same depth needs a larger force near the
    end, and the model's force pulse then carries more low-mode impulse: a side effect of §4.2.)
- **By style** **[memory, web §3]**:
  - punk and modern rock: 4–8 cm;
  - 1970s rock: 8–13 cm;
  - Kaye: 22–27 cm with felt;
  - McCartney: around the neck pickup.

  Default **0.10 m ± 0.006 per note**, the `-brightness` knob moving it (as `ElectricBassPlayer`
  does).

### 4.4 Downstrokes only, alternate picking, the player's variance

- **Alternate (down on the beat, up between):** the norm for eighths and sixteenths **[memory]**.
  - A note's stroke follows from its place in the beat if tempo is known. Otherwise from timing:
    a note within ~0.3 s of the last is the opposite stroke, a later one a downstroke **[E]**.
  - In the user's lines (§6) the median onset interval is 0.32 s, so about half the notes are
    alternate pairs.
- **Downstrokes only (punk, "chugging" eighths):** every stroke the same direction. Even level,
  harder attack **[memory]**. A player knob (`PJ_PICK_DOWN=1`).
- **Variance per note** (a human's, [E]):
  - depth ±7 %;
  - place ±6 mm;
  - angle ±0.05 rad;
  - push time ±20 %.

  Repeated notes then differ, as `ElectricBassPlayer` already does for fingers.
- **A pick passes; a finger stops.** A pick stroke does not stop what the string still sounds;
  it adds to it (`Guitar::pick`'s hold 40/s).
  - Repeated notes on one string therefore start from a moving string: a phase-dependent attack.
  - This is part of why a picked repeated note sounds "driving" and not identical **[E]**.
  - Keep it. Don't call `damp` before a pick on the same string. The previous note's note-off
    already damped it, if it was released.

### 4.5 Palm muting and the felt mute

- **What it is:** the side of the picking hand on the strings 0–20 mm in front of the saddle; or
  a foam/felt strip under the bridge cover or taped on (Kaye) **[web, Reverb/Premier Guitar;
  electric_guitars.md §8]**. The IDMT bass dataset's "muted" style is thumb plucking with the
  inner hand damping, not pick **[paper, Abeßer DAFx-13]**.
- **Harness [T]:** `Guitar::mute(s, depth, at)` on a P, pick at 9 cm, 2.5 mm.

| Mute | Decay 0.3–1.5 s | t to −30 dB | Centroid | Harmonics at 1 s (h1 / h2 / h3, dB to the attack's strongest) |
|---|---|---|---|---|
| none, open E | −2.9 dB/s | > 2.5 s | 383 Hz | −10 / 0 / +5 |
| depth 2 at 12 mm | −20 | (overall) ~1.5 s | 152 | −22 / −56 / −100 |
| depth 5 at 12 mm | −45 | 0.21 s after the peak | 101 | −42 / −107 / −123 |
| depth 5 at 25 mm | −45 | 0.21 | 110 | same |
| depth 10 at 12 mm | −220 | 0.15 | 78 | −77 / … |
| depth 5, A1 open | −45 | 0.29 | 134 | |
| depth 5, A2 (G string 2nd fret) | −54 | 0.31 | 248 | |

  - The overtones die in tens of ms and the fundamental carries the "thud". That is the
    recorded behaviour of FluidR3's palm-muted guitar (fundamental to −30 dB in 0.31–0.51 s,
    the 4th harmonic in 26–274 ms; electric_guitars.md §8) **[T]**.
  - **Bass targets** **[E]**:
    - the punk/modern palm mute: depth 6–8 at 10–15 mm, so the fundamental reaches −30 dB in
      0.25–0.5 s;
    - Kaye's felt: depth 1.5–2.5 at 8–20 mm, permanently on (−15…−25 dB/s: a short, round
      note that still has pitch).
  - **The palm's place hardly matters** between 12 and 25 mm at the same depth (the depth is
    normalised to the fundamental); only the overtone balance moves a little.
  - **What velocity does under a palm:** players lift on accents **[electric_guitars.md §8]**.
    Mute depth × (1 − 0.4·(v/127)²).

### 4.6 How the pick differs from fingerstyle, measurably

| Measure | Finger | Pick | Source |
|---|---|---|---|
| contact width | 8–12 mm pad | 0.5–1 mm edge | [memory; bass_section.md §6] |
| slip (release) | 0.7–2 ms (calloused ~1 ms) | 0.1–0.3 ms (stiff), up to 0.5 (thin) | [memory; E] |
| push | finger pulls 2–5 mm, ~5 ms | pick pushes 1–3 mm, 2–20 ms | [E] |
| angle into the body | rest stroke ~0.35–0.5 rad (into) | down ~0.25, up away from the body | [ebass.cc; guitar.h] |
| what it does to the ringing string | stops it (the finger lands) | passes (adds) | [guitar.cc] |
| >2 kHz in the first 100 ms (DI, same depth and angle) | −52 dB | −27 dB | [T, harness] |
| attack centroid, open E (DI) | 167 Hz | 373 Hz | [T, harness] |
| FluidR3, 2–4 kHz band at v120 against the strongest band | −54 … −26 dB (E1 … A3) | −32 … −15 | [T, §5] |
| FluidR3, centroid of the first 100 ms | 111 … 418 Hz | 153 … 555 Hz | [T] |
| Jnsgm2, centroid of the first 100 ms | 117 … 246 Hz | 206 … 610 Hz | [T] |

---

## 5. How the classic sets do GM 35

### 5.1 Read from the SF2 files [T]

SF2 times are 2^(tc/1200) s; filter frequencies are 8.176·2^(cents/1200) Hz.

**FluidR3_GM, preset 0:34 "Picked Bass".** One instrument, 9 key zones, one sample each, no
velocity layers, reverb send 2 %.

| Keys | Sample (name = string + fret) | Root | Length |
|---|---|---|---|
| 0–28 | Picked Bass E0 | 28 (E1) | 5.75 s |
| 29–30 | E2 | 30 | 4.41 |
| 31–32 | E4 | 32 | 3.43 |
| 33–34 | A1 | 34 | 4.37 |
| 35–37 | A3 | 36 | 4.21 |
| 38–39 | D1 | 39 | 4.23 |
| 40–42 | D3 | 41 | 3.98 |
| 43–47 | G0 | 43 | 4.45 |
| 48–84 | G5 | 48 | 3.38 |

- All samples are at 22.05 kHz (nothing above 11 kHz), looped over the last ~20–30 ms.
- **Volume envelope:**
  - hold 2^(1902/1200) = 3.0 s (4.0 s on the lowest zone);
  - then decay towards −100 dB over 2^(6386/1200) = 40 s (2.5 dB/s) at E1, falling to 24 s
    (4.2 dB/s) at the top;
  - release 2^(−2786/1200) = 0.20 s.
  - So in the first 3 s, the decay heard is the recording's own.
- No filter, no velocity modulation besides SF2's default velocity-to-attenuation.
- The sample names say this is **a real 4-string bass recorded string by string and fret by
  fret** (open E, E 2nd and 4th, A 1st and 3rd, D 1st and 3rd, open G, G 5th).

**TimGM6mb, preset 0:34 "Picked Bass".** Ten key zones sharing three tiny looped samples:
- "Pick Bass D1", 62 ms (keys 0–67); "D4", 20 ms (68–84); "C6", 3 ms (85–108). These are
  single-cycle-like wavetables.
- **Filter:** Fc 6786 cents = 412 Hz, with the mod envelope adding +4050 cents (×10.4, about
  4.3 kHz at the peak). It decays in 2^(−1106/1200) = 0.53 s at the bottom (0.31 s at the top)
  to a sustain 58 % down.
- **Volume:** hold 0.14 s, decay 19.8 s to a sustain 29.6 dB down (1.5 dB/s; 4–5 s at the top
  keys), release 0.12 s, attenuation 4.5 dB.
- A synthetic "subtractive" pick bass. **TiMidity does not apply its filter envelope:** in the
  render, the harmonics at 0.1 s and at 1 s are identical to 1 dB (§5.3). This matches the
  fx_sounds.md note on TiMidity.

**Jnsgm2, preset 0:34 "Pick Bass".** 13 key zones, one sample each ("ebass1"…"ebass13",
0.28–0.68 s at 44.1 kHz, looped), roots every 2–4 semitones from B1 to A4.
- **Volume:** decay 2^(5405/1200) = 22.7 s at the bottom (4.4 dB/s) to 2^(4094/1200) = 10.6 s at
  the top (9.4 dB/s), towards −100 dB.
- Release 0.40 s, attenuation 8.2 dB, reverb send 7 %.
- A preset-level mod LFO with no destination (does nothing). No filter, no velocity layers.

### 5.2 one maker early GS module/88 and XG (documentation)

- **GS:** program 35 "Picked Bs." on the early GS module **[memory]**; variations come by bank select
  (CC0 before the program change) **[web, Terratec GS doc]**. Whether the later GS module has a "Mute
  PickBs." variation was **not confirmed** **[web search, §10]**.
- **XG:** "PickBass" at bank 0 and "MutePkBa" / "MutePickBass" at **bank (LSB) 28**, both
  program 35, **one element each** **[web, Yamaha QY100, QY700 and CLP-585 data lists]**.
  - one maker's own GM 35 has a muted twin. The palm/felt mute is a recognised second voice of the
    same program.
  - The project could offer it on bank 28 (§9.5).
- **Measurements:** none (no hardware or ROM here). bass_amps.md §7.2 has the Soundhouse
  snippet on one maker velocity driving a filter for slap. Nothing similar was found for picked.

### 5.3 Rendered and measured [T]

- **Method.** TiMidity, a config mapping every program to the one SF2, reverb and chorus off,
  mono, 44.1 kHz.
  - Notes E1, A1, E2, A2, E3, A3 at velocities 40, 80 and 120, held 2 s.
  - Onsets from the render, decay as the envelope's slope from 0.3 to 1.5 s.
  - Bands are the first 100 ms in dB under the strongest octave band.
  - "Click" is the >2 kHz energy in the first 10 ms against 10–100 ms.
- **Velocity:** in all three, **level only**: +10.0 dB from v40 to v80 and +5.8–6.0 dB from v80
  to v120. Bands and centroid unchanged to the dB (one exception: Jnsgm2's v40 rows read
  slightly brighter, from the noise floor).

| Set | Key | Peak v120 (dBFS) | Envelope peak | Decay (dB/s) | Centroid 0–100 ms / 1.0–1.2 s | Bands 20-60/60-120/120-250/250-500/.5-1k/1-2k/2-4k/4-8k | Click |
|---|---|---|---|---|---|---|---|
| FluidR3 | E1 | −12.6 | 65 ms | 4.0 | 153 / 172 | −8 −2 0 −14 −12 −20 −28 −41 | −21 |
| | A1 | −12.5 | 80 | 2.0 | 169 / 165 | −10 −2 0 −9 −14 −22 −32 −48 | −17 |
| | E2 | −12.5 | 85 | 1.9 | 227 / 226 | −20 −7 0 −4 −16 −19 −28 −43 | −20 |
| | A2 | −12.6 | 35 | 2.3 | 373 / 350 | −14 −9 −3 0 −8 −12 −19 −28 | −25 |
| | E3 | −12.6 | 25 | 4.6 | 415 / 368 | −26 −29 −7 0 −9 −15 −21 −33 | −30 |
| | A3 | −12.6 | 15 | 5.4 | 555 / 468 | −30 −25 −4 0 0 −15 −15 −25 | −19 |
| TimGM6mb | E1 | −14.7 | 135 | 1.6 | 187 / 166 | −7 −3 0 −10 −9 −18 −27 −39 | −17 |
| | A2 | −14.6 | 30 | 2.0 | 456 / 445 | −43 −6 −3 0 −4 −9 −14 −26 | +1 |
| | A3 | −14.7 | 25 | 2.6 | 874 / 873 | −55 −59 −6 −3 0 −4 −9 −16 | +1 |
| Jnsgm2 | E1 | −15.6 | 25 | 7.8 | 213 / 147 | −12 −4 0 −6 −9 −23 −38 −70 | +19 |
| | A1 | −15.9 | 55 | 4.2 | 206 / 167 | −13 −4 0 −4 −15 −22 −40 −68 | +16 |
| | E2 | −15.8 | 10 | 10.1 | 376 / 247 | −40 −12 −3 0 −7 −14 −34 −67 | +13 |
| | A2 | −15.7 | 15 | 8.9 | 482 / 328 | −43 −11 −5 0 −1 −17 −14 −51 | +5 |
| | E3 | −15.7 | 5 | 13.3 | 610 / 463 | −43 −41 −8 0 0 −14 −12 −38 | +6 |
| | A3 | −15.8 | 5 | 10.9 | 449 / 256 | −46 −45 −3 0 −3 −18 −24 −42 | +13 |

- **FluidR3** is the most instrument-like: a real recorded attack (the envelope peaks at 15–85 ms,
  i.e. after several periods on the low strings) and a 2–5.5 dB/s decay.
  - Harmonics of the open E at 1 s: −7 / −13 / −8 / 0 / −6 / −32 / −39 / −44. The 2nd dies
    early (a dead-spot-like behaviour, or the amp).
  - The overtones above the 5th fall 20–40 dB in a second.
- **Jnsgm2** darkens fast: the centroid falls 25–45 % from the attack to 1 s on the upper notes.
  Its decay of 4–13 dB/s is short for a bass and comes from the envelope, not the recording.
  - It is the only set with a real onset transient in the >2 kHz band (+5…+19 dB).
- **TimGM6mb** is static (a looped single cycle; the filter envelope is not played). Not a
  reference for the attack.
- **Against their GM 34** (same method, v120) **[T]**:
  - FluidR3's finger has 2–4 kHz bands at −54 … −26 against the pick's −32 … −15, centroids
    111–418 Hz against 153–555, and its "click" −3…−9 against −17…−30.
  - The pick's highs are sustained, not a separate spike.
  - Jnsgm2's finger centroids are 117–246 Hz against the pick's 206–610.
  - **Peaks are equal** (FluidR3 finger −12.5, pick −12.6 dBFS), so in these renders the pick is
    not louder. bass_amps.md §7.1's "+4.7 dB" came from Debian's FluidR3 config, which carries
    per-program `amp=` trims **[T, by comparison]**. A modest +2…+4 dB for pick over finger at
    the same velocity is a matter of taste.

---

## 6. How GM files use program 35 [T]

- **Scan:** 397 MIDI files in `~/midi`, `~/Music`, `~/Downloads` (`~/Musici` is empty; some files
  appear in two places). Notes on channels whose current program is 34 (0-based), drums
  excluded.
- **Files using each bass program** (counted once per file):

| GM program | Files |
|---|---|
| 33 Acoustic | 125 |
| 34 Finger | 7 |
| 35 Pick | 1 |
| 36 Fretless | 42 |
| 37 Slap 1 | 2 |
| 38 Slap 2 | 1 |

  The collection is jazz-heavy.
- **The one GM 35 use:** `~/Music/midi/bushgrafts/desire.mid`, channel 2.
  - Program 35 at 0 s, then 36 (fretless) from 6 s on, re-sent every 8 s.
  - Six notes: A1–E2 (33–40), lengths 0.13–0.31 s (median 0.25), onset interval 0.25 s,
    velocities 29–112 (median 71), legato ratio 1.0. A short picked intro figure.
- **As a proxy for how pick-bass lines are written,** the finger-bass channels (4195 notes):

| Measure | p10 | p25 | p50 | p75 | p90 |
|---|---|---|---|---|---|
| note length (s) | 0.10 | 0.13 | 0.21 | 0.40 | 0.61 |
| onset interval (s) | 0.20 | 0.21 | 0.32 | 0.43 | 0.66 |
| length / next onset | 0.43 | | 0.79 | | 1.10 |
| key | 37 (C#2) | | 43 (G2) | | 50 (D3) |
| velocity | 63 | | 90 | | 109 |

  - 1st–99th percentile key: 31–56 (G1–G#3); below E1: 0.05 %.
  - The most used keys: E2 (40, 960 notes), A2 (45, 728), G2 (43, 375), D2 (38, 361).
- **What this implies:**
  - notes are mostly short and detached (median 0.21 s, 79 % of the slot), so **the attack and
    the first 200 ms are most of what is heard**, and note-off damping matters;
  - lines sit on the E/A/D strings at frets 0–7;
  - a pick bass's decay beyond 1 s is heard mostly on the last notes of phrases and long notes
    (the 10 % over 0.6 s).

---

## 7. Amp chains for pick bass

- **The preset exists:** "bass-stack" (§1) is bass_amps.md §10.2 built.
  - bass stack preamp; the James stack with bass 0.6 and treble 0.55; mid +3 dB at 800 Hz (the
    "mid-forward" voicing);
  - 6550 power with sag; 8×10 cabinet with a kick-drum microphone;
  - 35 % DI, aligned; a FET compressor at 4:1, 0.8 ms attack, 150 ms release.
  - Its checks (bass_amps.md §10.6 items 5, 8, 9 and 10): the hardest pick attacks at 3–5 % THD
    for 30–100 ms; mf notes under 1 %; v40→v120 at 10–12 dB after the compressor; 2–4 kHz
    within 25–35 dB of the strongest band on E1 v120.
  - These can only be run once a P with a pick plays into it.
- **Why the grind matters for a pick** **[E]**:
  - The pick's attack has 20–25 dB more energy above 2 kHz than a finger's (§4.6) and a sharper
    first peak. So it is the pick note, not the finger note, that reaches the 6550s' knee for
    30–100 ms.
  - The amp compresses the attack and adds odd harmonics in the 1–3 kHz region: the "growl".
  - The 8×10 cuts above 3–5 kHz (−20 dB at 5 kHz), so what remains of the click is 1–3 kHz.
- **DI + amp blend:**
  - The DI carries the pick's top (to 5–8 kHz) and the clean fundamental; the amp the grind and
    the mids.
  - 35/65 is the rock blend; 60/40 for the Kaye/session variant ("bass-stack-flat", bass_amps.md
    §10.2).
  - The DI must be lined up with the mic (exists: `diAlign`).
- **Compression:** the FET compressor at 4:1 with a 0.8 ms attack **[bass_amps.md §6]** catches the pick's
  peak, so the note is more even than a finger line. Keep it.
- **Variants:**
  - **Lemmy:** a British stack Super Bass with mids full, bass and treble off, presence 3 o'clock,
    volume 2 o'clock, no effects **[web, GuitarWorld]**: a guitar-type British stack preset (British stack
    tone stack, EL34 power, strong drive) into a 4×12 or 4×15. A `guitaramp` preset "superbass"
    beside "crunchStack" **[E]**.
  - **Squire:** two outputs; the neck pickup into the bass amp clean, the bridge into a driven
    guitar amp, blended **[web, GuitarWorld]**. That needs `Guitar` to output two pickups
    separately (it sums two selected pickups today) **[E]**.
  - **Modern:** the DI with the tube-emulation drive (exists) in parallel, a high-pass at
    ~150–250 Hz on the drive path, heavier compression **[memory; bass_amps.md §5.3]**.

---

## 8. What a listener would notice if it were wrong

1. **A pick bass that is just a brighter finger bass:** no mid-forward bark at 0.6–2 kHz, no
   growl on hard notes. That means the amp chain is missing or the release is not fast.
2. **A pick bass that is thin and quiet:** the model's short force pulse (§4.2). The fundamental
   must be as strong as with a finger at the same depth.
3. **Every note identical:** a pick passes over a moving string, and up/down strokes differ.
   Without the phase-dependent restrike and the stroke alternation, eighths sound like a
   sequencer.
4. **A clicking noise burst on every note:** through a pickup there is no separate click; the
   highs belong to the string and decay with it (§4.2).
5. **A palm mute that removes the pitch:** the fundamental must survive 0.25–0.5 s while the
   overtones go in tens of ms.
6. **Notes that ring under the next:** as for the finger bass, each note is stopped by the next
   on another string, or by its note-off.
7. **The P's comb wrong:** a P played over its pickup has a hollow 6th harmonic on the E and A
   strings. A J's scoop instead (8th–9th) reads as "jazz bass", not "P".

---

## 9. Build plan

### 9.1 What to reuse

- `Guitar` (`src/guitar.h/.cc`), unchanged except for §9.2: the string modes, two
  polarisations, neck modes, the pickups and their widths, `fret`, `bend`, `damp`, `mute`.
- From `src/ebass.cc`:
  - the string numbers (factor `jazzBass()` into a shared `bassStrings(GuitarSpec&)` that both
    specs call);
  - the DI biquad and the amp hook-up (`ElectricBassModel` taking a spec and a preset; or a
    sibling `PickBassModel` with 20 lines);
  - the fingering solver from `ElectricBassPlayer::play` (string choice, hand position, one-voice
    lines, double stops, note-off damping, the bend). Factor it out (`BassFingering`) rather than
    copy it.
- `GuitarAmp::bassStack()` as it is, with `connect()` told the P coil.

### 9.2 What to add

**(a) `precisionBass()` spec** (numbers from bass_section.md §3 and §4) [web, E]:

| Field | Value |
|---|---|
| scale, frets | 0.864 m, 20 |
| strings | as `jazzBass()` (EXL170 rounds) |
| pickup | one, "split": at {0.149, 0.149, 0.124, 0.124} m; gaps {3.5, 3.4, 3.3, 3.2} mm; width 7 mm; apart 0 (each string under one half) |
| coil | L 6.4 H, R 10.5 kΩ, C 30 pF (unloaded resonance ~11.6 kHz) |
| pots | one 250 k volume ‖ one 250 k tone: 125 kΩ; the tone cap 0.047 µF (tone open by default; a `PJ_PBASS_TONE` knob 0..1 adds the R–C branch) |
| cable | 700 pF |
| loaded resonance | ~2.0–2.4 kHz (bass_amps.md §10.6 item 1: 1.9–2.0 kHz into 10 MΩ) |
| neck modes | as the J (147 Hz dead spot, 290 Hz) |
| `steel`, `magnetR` | as the J |

**Flatwound variant** (`PJ_PBASS_FLATS=1`): `loss1` × 2.5 (fit to recordings; §2.4), B × 1.2,
and tensions +10 % (.050–.105 heavy flats) **[E]**. Expected **[T, harness with loss1 × 2.5]**:
the decay on the open E from ~3.6 to ~6.6 dB/s; the attack spectrum unchanged.

**(b) The pick contact, `Guitar::pickContact(s, from, angle, depth, push, slip, stiffness)`.**
Replaces the force pulse with a displacement-controlled push. Per sample while active:
- the pick tip's target displacement y_p(t) rises from 0 to `depth` over `push` (a raised
  cosine; 2–20 ms);
- the contact force is F = k_p·(y_p − y_s(x_p)) with y_s the string's displacement at the
  contact (the modes' sum over the 0.8 mm shape, as `shapeAt` already gives), and F ≥ 0 (a pick
  only pushes);
- k_p is the pick's tip stiffness: **2–20 kN/m** **[E]** (0.6 mm: ~2; 1.0 mm: ~6; 1.5 mm: ~20,
  from a cantilever 8 mm long, 8 mm wide, E = 3 GPa). A soft pick lags the hand: the string
  reaches less than `depth`;
- the slip starts when F exceeds the release force F_r: a stiff pick's F_r is set by `depth`
  (the string is released at the planned depth); a thin pick's F_r = k_p·δ_max, with δ_max
  ~1.5 mm the bend at which the tip lets the string pass **[E]**. The thin pick therefore
  releases early and softer;
- the slip: F falls to 0 over `slip` (0.1–0.3 ms; 0.15 ms for a 1 mm pick, 0.3 ms for 0.6 mm)
  **[memory; E]**;
- the pick passes: no damping of the existing motion beyond the contact (as `pick` today: hold
  ~40/s only while touching).

Stability: k_p·(the modes' admittance at the contact) at 48 kHz is the same class of problem as
`Bass::kBoard`. The penalty spring's own frequency √(k_p/m_eff), with m_eff the string's local
mass over ~1 cm (0.3 g on E1), is 1.3–4 kHz: fine at fs = 48 kHz with the existing semi-implicit
mode update **[E]**. Test (§9.6).

Expected effect **[E, from the §4.2 numbers]**: the level of a 4 ms "rise" (the fundamental
fully driven) together with the brightness of a 0.15 ms release, i.e. about −24…−27 dB RMS and a
centroid of 350–400 Hz at 2.5 mm. The harness can check this once the function exists.

Cheaper alternative if the contact solve proves fragile: keep the force model but add a **hold**
phase (the force held at its peak for `push` before the slip, as `pluckPad` holds the flesh).
This gets the low modes to static deflection without a contact solve; it cannot do the thin
pick's early release.

**(c) `PickBassPlayer`** (on the fingering solver):
- **Velocity → depth:** 0.6 mm + 2.6 mm·(v/127)^1.3 **[E]** (a pick bassist's range is shallower
  than a finger's 0.4–5 mm because the pick holds the string less). Also → push time
  (12 ms at pp to 3 ms at ff: harder strokes are faster) and → mute lift (§4.5).
- **Place:** 0.10 m·2^(1 − brightness) ± 6 mm (the `-brightness` knob as the finger bass has it).
- **Strokes:** down at angle +0.25 rad, up at π + 0.18 with depth × 1.1. Alternate if the onset
  interval < 0.3 s, else down. `PJ_PICK_DOWN=1` for downstrokes only.
- **Pick:** `PJ_PICK_MM` (default 1.0): k_p and slip from thickness (§9.2b).
- **Mute:** `PJ_PICK_MUTE` 0..1 → `mute` depth 0..8 at 12 mm on the played string while the note
  sounds; `PJ_PICK_FELT=1` → depth 2 at 10 mm on all strings, always. XG bank 28 → mute 0.8
  (§5.2).
- **Variance:** depth ±7 %, place ±6 mm, angle ±0.05 rad, push ±20 %.
- **Note-off and lines:** as `ElectricBassPlayer` (damp 40/s on release; a new note on another
  string stops the last; double stops allowed). A pick does not damp the string it strikes.
- **Drones and chords** (Lemmy): when two keys overlap by more than 50 ms, let both ring (the
  existing double-stop rule with a longer window, `PJ_PICK_CHORDS=1`).

**(d) Registration:** `-inst pick-bass`; kGm already names "pick-bass" first for GM 35, so it
plays once the instrument exists.

### 9.3 Parameters with numbers (the default "P, rounds, pick, svt")

| Parameter | Default | Range | Source |
|---|---|---|---|
| pick place from saddle | 0.10 m | 0.04–0.27 | §4.3 |
| depth | 0.6 + 2.6·(v/127)^1.3 mm | | [E] |
| push time | 12 → 3 ms with velocity | 2–20 | §4.2 [E] |
| slip | 0.15 ms (1.0 mm pick) | 0.1–0.5 | [memory, E] |
| pick stiffness k_p | 6 kN/m | 2–20 | [E] |
| contact width | 0.8 mm | 0.5–1 | [steel_guitar.md] |
| down / up angle | +0.25 / π + 0.18 | | [guitar.h, E] |
| up/down level gap | 1.5–3 dB | | [E; §4.2] |
| palm mute | off; on: depth 6–8 at 12 mm | | [T, §4.5] |
| felt (Kaye) | depth 2 at 10 mm | 1.5–2.5 | [T, §4.5] |
| strings | EXL170 rounds; flats `loss1` × 2.5 | | [T, E] |
| pickup | P split, 149/124 mm, 6.4 H, 10.5 kΩ | | [web, bass_section.md] |
| amp | "bass-stack" (DI 35 %, FET compressor 4:1) | "bass-stack-flat" for felt+flats | [bass_amps.md] |

### 9.4 Order

1. `precisionBass()` and the shared string table; the finger player on it as a sanity check (it
   should sound P-like at once).
2. `Guitar::pickContact` (or the hold-phase fallback), with its unit tests (§9.6 items 1–3)
   before any player work.
3. `PickBassPlayer` with strokes, place, variance, mute, felt; `-inst pick-bass`; hook to "bass-stack".
4. Run bass_amps.md §10.6 items 5, 8, 9, 10 against it; adjust depth and push to land them.
5. Variants by knob: flats, felt, downstrokes, chords; later a through-neck bass spec and the
   British stack "superbass" preset; the active single-humbucker bass last.

### 9.5 Optional: XG bank 28

If `src/midifile.cc` passes bank select to instruments, GM 35 on LSB 28 can set `PJ_PICK_MUTE`
≈ 0.8: one maker's MutePkBa **[web, Yamaha data lists]**.

### 9.6 Tests (for `tools/test.py`, as `electric_bass()`)

1. **In tune:** E1, A1, D2, G2 open and five fretted notes within ±8 cents (as GM 34).
2. **Pick contact, unit level** (harness or `-seq`, DI only, `PJ_EBASS_DI`-like):
   - (a) at the same depth and angle, the pick note's fundamental (35–48 Hz band, first 200 ms)
     is within 2 dB of the finger note's. This is the §4.2 gap closed; today −4.6 dB in RMS,
     more in the fundamental;
   - (b) its energy above 2 kHz in the first 100 ms is 18–30 dB above the finger's (today 25 dB);
   - (c) halving the slip (0.3 → 0.15 ms) raises the >2 kHz energy by ≥ 3 dB and changes the
     level of 40–100 Hz by < 1 dB (brightness and level separate).
3. **Stability:** 1000 random picks (place 4–27 cm, depth 0.3–4 mm, k_p 2–20 kN/m) over ringing
   strings: no NaN, no growth, the peak bounded.
4. **P comb:** a pick at 0.149 m on the open E gives a 6th harmonic ≥ 15 dB under the mean of
   the 3rd–5th in the first 200 ms (the comb notch, §4.3).
5. **Stroke alternation:** at 0.2 s intervals, up strokes 1.5–3 dB under down strokes (median
   over 16 notes), and the onset waveforms of consecutive notes differ (correlation of the first
   20 ms < 0.95).
6. **Restrike passes:** a repeated note on one string without note-off between keeps the energy
   of the old note (the level 1 ms before and 5 ms after the new pick differ by the new pick
   only); with note-off, the string is damped first.
7. **Palm mute:** with `PJ_PICK_MUTE=1`, open E at v100: the fundamental to −30 dB in 0.25–0.5 s;
   the 4th harmonic to −30 dB in < 0.15 s; the centroid of the first 100 ms ≤ 0.4 × unmuted's.
8. **Felt:** `PJ_PICK_FELT=1`: decay 0.3–1.5 s of −15…−25 dB/s on the open E.
9. **Velocity:** v40 → v120 at the DI: 16–20 dB, and the attack centroid rising ≥ 20 % (the
   physical pick should, unlike every SoundFont §5.3).
10. **Against FluidR3 (through "bass-stack"):** E1 v120 first 100 ms, 2–4 kHz band within 25–35 dB
    under the strongest (bass_amps.md §10.6 item 10; FluidR3 −28 dB); A2 centroid 300–600 Hz
    (FluidR3 373 Hz); decay 0.3–1.5 s of 1.5–6 dB/s on E1–A3 (FluidR3 1.9–5.4).
11. **Amp:** the hardest pick attack (v127, E1) at 3–5 % THD for ≤ 100 ms in the amp path,
    under 1 % after (bass_amps.md §10.6 item 5).
12. **Real time:** ≤ the finger bass's cost + 10 % (the contact adds work only during the push).
13. **Lines:** the desire.mid intro (channel 2, 0–6 s) plays without overlapping tails; a note-off
    stops its note within 0.15 s to −30 dB.

---

## 10. Not found, or uncertain

- **Absolute pick–string contact and slip times for guitar or bass picks.** Only harpsichord
  proportions (slip 26–36 % of the contact) **[paper]**. A robotic-plectrum study notes the
  release threshold varies with sub-millimetre mounting shifts (CV 14–60 % on shallow plucks)
  **[paper, Jasiński FA2026]**. That says the release point is sensitive, not where it is.
- **A controlled flat-against-round measurement** (spectra, partial decays): none found. The
  flats' `loss1` × 2.5 is a guess.
- **Pickup positions** on the through-neck bass 4001/4003 and the active single-humbucker bass, and the Rick's inductance:
  not found. Forum DC resistances only.
- **early GS module/88 GM 35 details** (layers, velocity filter, any "Mute PickBs." variation): not
  confirmed. The XG MutePkBa on bank 28 is confirmed by three data lists.
- **The IDMT-SMT-BASS picked (PK) notes** (3 basses, 3 pickup settings, E1–G3) would give real
  pick-against-finger numbers on the same instruments. The download (~1 GB) was not attempted
  under the disk quota. It is the best next measurement.
- **Player details** in §3 marked [memory] (McCartney's picking place, punk rigs, modern
  players' chains) were not looked up.
- **TimGM6mb's filter envelope** is not played by TiMidity, so its intended pick sound (a
  412 Hz → 4.3 kHz sweep closing in ~0.5 s) was not heard. A FluidSynth render would.

## 11. Sources

- **[bass_section.md]**, **[bass_amps.md]**, **[electric_guitars.md]**, **[steel_guitar.md]**,
  **[fx_sounds.md]**: this project's research documents.
- **[Abeßer DAFx-13]** J. Abeßer, P. Kramer, C. Dittmar, G. Schuller, "Parametric audio coding of
  bass guitar recordings using a tuned physical modeling algorithm", DAFx-13 (the plucking styles
  FS, PK, MU, ST, SP; MU = thumb with the hand damping).
  https://dafx.de/paper-archive/2013/papers/24.dafx2013_submission_9.pdf
- **[IDMT-SMT-BASS]** dataset page: https://idmt.fraunhofer.de/en/publications/datasets/bass.html ;
  Zenodo: https://zenodo.org/record/7188892
- **[Le Carrou SMAC 2013]** harpsichord plectrum–string interaction (stick/slip proportions).
  https://www.lam.jussieu.fr/Membres/Castellengo/publications/2013a_Le%20Carrou-SMAC_p.155-160.pdf
- **[Chadefaux 2012]** "Experimentally based description of harp plucking".
  https://www.institut-langevin.espci.fr/IMG/pdf/2012_experimentally_based_description_of_harp_plucking.pdf
- **[Jasiński FA2026]** J. Jasiński, "Limits of geometric pluck calibration for repeatable robotic
  excitation of plucked string instruments", Forum Acusticum 2026 (preprint).
  https://dael.euracoustics.org/confs/fa2026/data/articles/000508.pdf
- **[Fletcher & Rossing]** *The Physics of Musical Instruments*, §9.3 (plectrum width), via
  steel_guitar.md.
- **[Reverb]** "Spotlight On: Legendary Bassist Carol Kaye".
  https://reverb.com/blog/spotlight-on-carol-kaye-bassist-and-music-educator
- **[GuitarWorld Kaye]** "98 % of bass parts cut in Hollywood in the '60s were done with a pick on
  flatwound strings".
  https://guitarworld.com/news/bass-legend-carol-kaye-about-98-of-bass-parts-cut-in-hollywood-in-the-60s-were-done-with-a-pick-on-flatwound-strings ;
  https://guitarworld.com/features/10-things-you-didnt-know-about-carol-kaye
- **[Premier Guitar]** "Forgotten Heroes: Carol Kaye".
  https://www.premierguitar.com/artists/forgotten-heroes-carol-kaye
- **[GuitarWorld Squire]** "How Chris Squire got his iconic bass tone".
  https://guitarworld.com/features/chris-squire-bass-tone
- **[Harmony Central]** forum, "The Chris Squire Rickenbacker tone enigma".
  https://www.harmonycentral.com/forums/topic/239037-the-chris-squire-rickenbacker-tone-enigma/
- **[GuitarWorld Lemmy]** "How Lemmy Kilmister found his terrifying bass tones";
  "Lemmy – the final interview".
  https://www.guitarworld.com/features/lemmy-bass-tones ;
  https://guitarworld.com/features/lemmy-the-final-interview-i-was-born-to-play-the-bass ;
  https://www.guitarworld.com/features/how-to-get-lemmys-overdriven-bass-tone
- **[RickResource]** forum, Rickenbacker pickup resistances.
  https://www.rickresource.com/forum/viewtopic.php?p=231299 ;
  https://www.rickresource.com/forum/viewtopic.php?p=795756
- **[Wikipedia StingRay]** https://en.wikipedia.org/wiki/Music_Man_StingRay ;
  **[Seymour Duncan]** "The History of the Music Man StingRay Bass Pickup".
  https://www.seymourduncan.com/?p=76418 ; **[Music Man]**
  https://www.music-man.com/instruments/basses/stingray
- **[Ernie Ball]** "Bass strings: flat vs round". https://blog.ernieball.com/strings/bass-strings-flat-vs-round ;
  **[Sweetwater]** https://www.sweetwater.com/insync/flatwound-vs-roundwound-bass-strings/
- **[Yamaha data lists]** QY100 (https://uk.yamaha.com/files/download/other_assets/0/318130/QY100E2.pdf),
  QY700 (https://de.yamaha.com/files/download/other_assets/1/317031/QY700F2.pdf), CLP-585
  (https://br.yamaha.com/files/download/other_assets/9/331289/clp585_en_de_fr_es_ja_dl_a0.pdf):
  PickBass bank 0, MutePkBa bank 28, program 35, one element. Read as search snippets.
- **[Terratec GS doc]** GS variation selection (CC0, then program change).
  https://terratec.de/treiber/Audio/EWS/64L/SoundSets/94SBK08.DOC
- **[pickup-position-db]** https://agschaid.github.io/pickup-position-db (via bass_section.md).
- **[T] files:** `/usr/share/sounds/sf2/FluidR3_GM.sf2`, `/usr/share/sounds/sf2/TimGM6mb.sf2`,
  `~/soundfonts/Jnsgm2.sf2`; TiMidity++; `src/guitar.cc` as of 2026-11-20 in the harness; the
  MIDI files under `~/midi`, `~/Music`, `~/Downloads`.

---

## 12. As built (2026-11-20)

**Where:** `src/bassspec.cc` (`bassStrings()`, shared by `jazzBass()` and the new
`precisionBass()`), `src/ebass.h/.cc` (`BassLinePlayer`: the fretting hand and the line's rules
factored out of the finger bass; `ElectricBassPlayer` on it, byte-identical to before;
`PickBassPlayer`), `Guitar::pickContact` in `src/guitar.h/.cc` (the guitars byte-identical),
the "bass-stack" preset's FET compressor threshold in `src/guitaramp.cc`, `tools/pickcontact.cc` and
`tools/test.py` `pick_bass()` (§9.6's items), the trims in `src/levels_data.h` (+10.8 dB) and
`src/velgain_data.h`.

**As planned (§9.2-9.3):** the P split coil (149/124 mm, 6.4 H, 10.5 kΩ, 30 pF, 125 kΩ, a tone
knob), flats as a knob (`loss1` × 2.5, B × 1.2, T × 1.1), the contact (a raised-cosine push, a tip
spring of 6 kN/m for a 1 mm pick, pushing only, a slip of 0.15 ms, the early release of a thin
pick at 1.5 mm of bend), the place (0.10 m ± 6 mm, the brightness knob), alternate strokes under
0.3 s, `PJ_PICK_DOWN`, the palm (depth 8 × `PJ_PICK_MUTE` × (1 − 0.4 v²) at 12 mm) and the felt
(depth 2 at 10 mm), `PJ_PICK_CHORDS`, push 12 → 3 ms with velocity, variance on depth, place,
angle and push. The contact needed no fallback: 1000 random picks are stable.

**Departures, and why [T unless marked]:**
- **Velocity → depth** is 0.2 + 3.2 mm·(v/127)^1.8, not 0.6 + 2.6·v^1.3: the planned law spans
  only ~9 dB from v40 to v120 while item 9 asks 16-20 at the DI. Now 16.2 dB.
- **The slip follows the hand's speed** (0.15 ms × √(0.3 m/s / speed): ~0.3 ms at pp, < 0.1 at ff)
  **[E]**, so a hard stroke is also a sharper release.
- **Up strokes** are 30 % deeper (not 10 %): with π + 0.18 rad and × 1.1 the gap was 3.8 dB; at
  × 1.3 it is 2.4 dB (item 5's 1.5-3).
- **The P's field width** is 5 mm (the J keeps 7): 4.8 mm poles 3.4 mm under the string give ~12 mm
  at half height **[E]**; the top 4-8 kHz comes up 5-10 dB. Modes above 5 kHz add nothing
  (tried 8 and 11 kHz): the coil's 2.3 kHz resonance into 700 pF and the 8x10 set the top.
- **The FET compressor's threshold** in "bass-stack" is 0.04 V, not 0.06: with the P's EMF the compressor did
  nothing at 0.06 (0 dB on a mf line, 0.7 on ff). Now a ff line ~2.5 dB, and E1 v40 → v120 is
  11 dB after it (bass_amps.md §10.6 item 9: 10-12).
- **What §4.2 predicted about brightness was wrong in one point.** The held pick is as loud as a
  slow finger pull at the same depth (+0.3 dB), but its spectrum below ~2 kHz is the static
  triangle's, not the old force pulse's: the old pick's "brightness" was its missing fundamental.
  The DI centroid of the open E is ~140 Hz (not 350-400). The pick's brightness is above 2 kHz
  (+19 dB over GM 34's finger) and through the bass stack's 1-4 kHz.
- **Brightness with velocity** is modest: the release is the same triangle at every depth.
  E1 brightens 24 % (its push at ff is shorter than a quarter period); A2 6 %; A3 not at all (the
  fast push's own transient reshapes its 2nd-7th harmonics). Real hard picking also has fret clank,
  which the model has not.
- **The comb notch is shallower:** picked at 149 mm, the 6th harmonic is 21 dB under the strongest
  and 12 under the 3rd-5th (the old force pulse: 31 and 25). While the pick holds the string, the
  149 mm between it and the saddle rings at f0·L/x = 5.8 f0, beside the 6th; at release it feeds
  the notch. Item 4 is tested as ≥ 10 dB under the 3rd-5th and ≥ 15 under the strongest.
- **Item 2a** is tested against a slow finger pull at the same depth (+0.3 dB). Against GM 34's
  own finger the pick is +7.7 dB: that finger's 2.2 ms rise gives E1 less than a static pull.
- **Item 7's centroid** (≤ 0.4 × open) assumed the old bright open note; the held pick's open E is
  already fundamental-heavy (140 Hz). Tested instead: the energy above 150 Hz in the first 100 ms
  falls 8.5 dB under the palm (≥ 6 asked).
- **Item 11** reads as bass_amps.md §10.6 item 5 words it: v127 E1 at 4.1 % in its first 100 ms,
  1.9 % after (the preamp triodes' curvature follows the level; the power stage adds nothing); a
  mf note (v80) 1.0 % then 0.7 %.
- **Item 10's decays** are tested as 1-6 dB/s, not 1.5-6: the open A1 falls 1.3 dB/s through the
  FET compressor (which slows a held note's fall over its threshold) and 1.2-1.6 at the bare DI: the shared
  strings' own (FluidR3's A1 2.0).
- **XG bank 28** (§9.5) is not done: `-play` does not pass bank select to instruments.

**Measured against §9.6** (all in `tools/test.py` `pick_bass()`, passing): 1. in tune ±3 c;
2a +0.3 dB, 2b +18.7 dB, 2c +3.9 dB above 2 kHz and −0.1 dB at 40-100 Hz; 3. no NaN, the peaks per
250 picks 2.0 / 3.7 / 2.2 / 2.3 V (3.9 × the hardest single stroke: four strings, picks adding to
moving strings); 4. as above; 5. 2.4 dB, onsets correlating ≤ 0.25 (downstrokes only: 0.1 dB);
6. 1 ms before a restrike +0.0 dB against the note alone (−34 dB if released 0.1 s before), the old
note's sound continuing (correlation 0.75); 7. the fundamental to −30 dB in 0.46 s, the 4th
harmonic in 0.11 s; 8. the felt 20 dB/s (open 4.5, flats alone 6.0); 9. 16.2 dB, centroid
126 → 156 Hz; 10. E1 v120 2-4 kHz −30 dB, A2's centroid 344 Hz, decays 1.3-4.1 dB/s on E1-A3 (the open A1 the
longest, also 1.2-1.6 at the bare DI: tested as 1-6, not 1.5-6);
11. as above; 12. the strings and DI 157-268 ns a sample, under the finger bass's (the contact costs
only during the push); 2.4-3.8 % of a core with the bass stack at 4x; 13. desire.mid's intro: 0.15 s after
each release −38 / −58 / −49 dB.

**Against FluidR3 (through "bass-stack"):** E1 v120 octave bands 20-60 Hz … 4-8 kHz: ours −19 −2 0 −10 −19
−21 −30 −66, FluidR3 −9 −4 0 −14 −12 −21 −29 −42. Alike from 120 Hz to 4 kHz; above 4 kHz ours is
darker by 20+ dB, and FluidR3 has more 20-60 Hz (its pre-attack and room). FluidR3's DI-like
waveform (a flat line between two sharp kinks a period) is what our DI gives; FluidR3's notes
each start after ~20 ms of a slow, silent push, ours after 3-12 ms. Through `-play` the velocity
sweep follows FluidR3's level curve within a few dB; ours brightens with velocity (E1 98 → 123 Hz,
A2 307 → 409 Hz), FluidR3's does not move.

**Next:** fret clank on hard strokes (the string against the frets near the pick's push), a
"bass-stack-flat" session chain for felt and flats, the through-neck bass and the British stack "superbass", a
active single-humbucker bass; IDMT-SMT-BASS's picked notes (§10) to fit the slip and the push against real ones.
