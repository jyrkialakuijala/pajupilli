# The drum keys below 35: GS/GM2 extras and XG's map

Status: 2026-11-15. Research, plan (§6) and what was built (§7). Marks: **[files]** measured in the
user's MIDI files (`~/Music/midi`); **[web]** a source in §8; **[calc]** worked out here; **[memory]**
general knowledge, to be checked; **[design]** a choice of this plan.

## 1. Short answer

- **Two maps disagree below 35.**
  - GS (and GM2, which took GS's): 25 Snare Roll, 26 Finger Snap, 27 High Q, 28 Slap,
    29/30 Scratch Push/Pull, 31 Sticks, 32 Square Click, 33 Metronome Click, 34 Metronome Bell
    **[web, GS]**.
  - XG (MU modules, PSR/Tyros keyboards) uses the same range for kit voices **[web, PSR]**:
    - 13/14 Surdo mute/open, 15 Hi Q, 16 Whip Slap, 17/18 Scratch Push/Pull, 19 Finger Snap;
    - 20 Click Noise, 21 Metronome Click, 22 Metronome Bell, 23/24 Seq Click L/H;
    - **25 Brush Tap, 26 Brush Swirl L, 27 Brush Slap, 28 Brush Swirl H, 29 Snare Roll**;
    - 30 Castanet, 31 Snare L, 32 Sticks, 33 Bass Drum L, 34 Open Rim Shot.
- **The user's files are XG.** All of the files that use these keys are Doug McKenzie's
  sequences (bushgrafts). Their texts say a keyboard model and "Set to GenMidiBRUSH SET", several
  have the XG system-on SysEx, and channel 10 uses bank 127 (XG drums). Only 2 of the 52 files
  with keys below 35 carry a GS reset **[files]**.
- **The use fits XG, not GS** **[files]**:
  - 28 is played on every beat with the hi-hat foot (44): the brush swirl's pulse, not a "slap".
  - 34 is played on triplet skips with the hats (desire.mid: 171 notes): an open rim shot, not
    a metronome bell.
  - 31 (245 notes, 20 files) sits in the groove: XG's Snare L.
  - 32 is played on the triplet "a" before each beat with the closed hat (desire.mid): XG's
    Sticks, a stick-on-stick skip.
  - 25/26/27: XG's brush tap, swirl and slap, in the brush passages.
  Played as GS, these files put metronome bells and scratches into ballads.
- **So:** keys below 35 follow XG unless the file has a GS reset (or GM2 system-on) **[design]**.
  - XG's brush, snare and kick keys go to the instruments already built: the brushed snare and
    the jazz kick.
  - The rest go to one new instrument (§7) holding the small sounds of both maps, by GS/GM2's key
    numbers, with castanets on GM2's 85.

## 2. How the keys are used in the user's files [files]

The table counts notes; "in" means mid-song, after other parts have started.

| Key | Notes | Files | Where | Velocity (median) | Rhythm (example) |
|---|---|---|---|---|---|
| 24 | 16 | 4 | 9 before the band, 7 in | 34 | — |
| 25 | 87 | 5 | in | 52 | — |
| 26 | 46 | 4 | in | 54 | with 25 and 27 (Inasent) |
| 27 | 148 | 8 | in | 51 | triplet "a" (How Deep: 41 of 52) |
| 28 | 986 | 8 | in | 45 | on every beat with 44 (Stormy Weather: 144 of 153) |
| 29 | 55 | 15 | in | 44 | — |
| 30 | 8 | 6 | in | 38 | — |
| 31 | 245 | 20 | in | 48 | triplet "a" and between the ride's notes |
| 32 | 144 | 19 | in | 32 | the triplet "a" with the closed hat (desire: every beat) |
| 33 | 27 | 20 | in | 50 | — |
| 34 | 804 | 32 | in | 46 | triplets with the hats (desire: 171) |

- They are **not** count-ins: only key 24 comes before the band.
- So nothing here should be silenced as "a metronome"; read as XG, all of it is kit playing.

## 3. What each sound is, and its physics

### 3.1 Sticks (GS 31, XG 32): two hickory sticks clicked together

- **A stick:** 5A, 406 mm long, 14.4 mm diameter; hickory with E ≈ 15 GPa and ρ ≈ 830 kg/m³
  **[memory]**.
- **Its modes:** a free-free beam, f_n = (β_n L)² / (2π L²) · √(E I / ρ A), where
  √(E I / ρ A) = √(E/ρ) · d/4 ≈ 15 m²/s **[calc]**:

| n | β_n L | f (Hz) |
|---|---|---|
| 1 | 4.730 | 330 |
| 2 | 7.853 | 910 |
| 3 | 10.996 | 1780 |
| 4 | 14.137 | 2950 |
| 5 | 17.279 | 4400 |
| 6 | 20.420 | 6140 |

  These agree with jazz_drums.md §5.5 (324, 892, 1749 Hz for a 13.6 mm stick).
- **Damping:** hickory's own loss is about Q 60–100 **[memory]**. The hands grip each stick about
  a third from its butt end and damp the modes that move there. That gives T60 of 20–60 ms for the
  low modes; the high ones live 30–80 ms **[memory, to fit]**.
- **The click:** the shafts cross near their tips, wood on wood. The contact is short, 0.1–0.2 ms,
  shortening a little at higher speed (Hertz) **[calc]**. It reaches every mode up to ~5 kHz,
  weighted by the modes' shapes at the contact (about 6 cm from the tip on one stick, 12 cm on the
  other).
- **Radiation:** a thin rod is acoustically small, so the pressure it radiates goes as the
  acceleration (∝ ω² of the displacement) **[calc]**. Each modal force's own jolt, high-passed, is
  the dry "crack" above the modes.

### 3.2 Whip / slapstick (GS 28 Slap, XG 16 Whip Slap)

- **The orchestral whip:** two hinged hardwood boards, about 50 × 6 × 1.2 cm, slapped together
  **[memory]**.
- **The sound:**
  - the air squeezed out between the boards as they meet: a sharp broadband pop, the "crack";
  - the boards' bending modes: free-free beams, √(E/ρ) · h/√12 ≈ 14 m²/s with maple → 200, 560,
    1100, 1820, 2720 Hz **[calc]**, about Q 40, damped by the hands.

### 3.3 Finger snap (GS 26, XG 19)

- **Mechanics:** the snap lasts about 7 ms. The middle finger, released by the thumb's friction,
  strikes the palm with the highest rotational acceleration the body makes **[web, B21]**.
- **The sound:** the finger's impact on the thenar pad, soft tissue (a contact of about 0.5–1 ms),
  and the cupped hand's air pocket, which rings briefly. In clapping, the cavity between the
  palms sets the pitch, and the soft tissue makes it short **[web, clap study]**. Taken here as two
  damped resonances, ~1.4 and ~2.8 kHz with Q 2–3, under a soft click **[design, to fit by ear]**.

### 3.4 Castanets (XG 30, GM2 85)

- **The instrument:** two cupped hardwood shells (about 5 cm), clapped by the fingers. The sound
  is the air between the shells resonating as they close, plus the shells' own modes. High-speed
  optical visualization has caught the resonance between the shells **[web, castanets 2020]**.
- **Model:** the closing's pop through a resonance of ~3 kHz (Q ~5), plus shell modes at
  2–7 kHz, Q ~30 **[memory, to fit]**.

### 3.5 The electronic ones

| Key | Sound | Model |
|---|---|---|
| GS 27, XG 15 | High Q | A short resonant "zap": a sine whose pitch falls quickly from ~3 kHz to ~700 Hz in 25 ms, ~30 ms long **[memory]**. |
| GS 32 | Square Click | One or two cycles of a square wave around 1–2 kHz **[memory]**. |
| XG 20 | Click Noise | A 2 ms noise burst **[memory]**. |
| GS 33, XG 21 | Metronome Click | The tick of a woodblock-like case: two damped resonances, 1.8 and 3.4 kHz, ~10 ms **[memory]**. |
| GS 34, XG 22 | Metronome Bell | The mechanical metronome's small bell: three partials (about 1.0 : 2.7 : 5.2 × 1.6 kHz), 0.4 s **[memory]**. |
| XG 23/24 | Seq Click L/H | 5 ms sine ticks, 1 and 2 kHz. |
| GS 29/30, XG 17/18 | Scratch Push/Pull | A turntable scratch. The record (a vocal "ah": a sawtooth through three formants) is read at a speed swept forward for a push (0 → +3 → 0 in 120 ms) and backward for a pull (0 → −2.5 → 0 in 150 ms). Pitch and formants scale with the speed, loudness with |speed| **[design]**. |

### 3.6 Not taken here

- **XG 13/14 Surdo:** a large Brazilian bass drum, for the toms and kick work.
- **XG 25–29, 31, 33, 34:**
  - brush tap, swirls and slap, the snare roll, Snare L and the open rim shot are the brushed
    snare's (the other session);
  - Bass Drum L is the jazz kick's.
  Mapping in §6.

## 4. Velocity

- Every sound here is struck: stroke speed ∝ (v/127)^1.25, level ∝ speed, about 30 dB from 16 to
  127. That matches the struck instruments (README's MIDI section) and General MIDI's curve.
- The electronic ones (High Q, the clicks, the bell) scale amplitude as (v/127)^(5/3), as a sample
  player does.

## 5. Cost

All of it is a few dozen resonators while a sound rings, a few hundred ns per sample at most
**[calc]**.

## 6. Plan (2026-11-15)

1. One instrument, `-inst perc-extras` (`src/percextras.h/.cc`), keyed by the GS/GM2 numbers.
   - 26 snap, 27 High Q, 28 whip, 29/30 scratches, 31 sticks, 32 square click, 33 metronome
     click, 34 metronome bell;
   - the XG-only small sounds on unused keys: 20 click noise, 23/24 seq clicks, 85 castanets
     (GM2's own number).
   - A pool of 8 sounding events, each a small modal bank or a synth voice; sticks, whip, snap and
     castanets physical, the rest simple.
2. Registration: a kPlucked row, voice.cc, a level trim, a test in tools/test.py, a README
   paragraph.
3. Requests for `src/drumkit.cc` and `src/midifile.cc` (the other session's):
   - drumFor to cover keys below 35 and 82–87, with two maps;
   - the file's map: XG unless a GS reset or GM2 system-on is in the file.
   The tables:

| Key | GS/GM2 map | XG map |
|---|---|---|
| 13 | — | (surdo mute: toms/kick work; until then silent) |
| 14 | — | (surdo open: as 13) |
| 15 | — | perc-extras 27 (Hi Q) |
| 16 | — | perc-extras 28 (whip slap) |
| 17 | — | perc-extras 29 (scratch push) |
| 18 | — | perc-extras 30 (scratch pull) |
| 19 | — | perc-extras 26 (finger snap) |
| 20 | — | perc-extras 20 (click noise) |
| 21 | — | perc-extras 33 (metronome click) |
| 22 | — | perc-extras 34 (metronome bell) |
| 23 | — | perc-extras 23 (seq click L) |
| 24 | — | perc-extras 24 (seq click H) |
| 25 | brush-snare (snare roll: none yet → 38) | brush-snare 38 (brush tap) |
| 26 | perc-extras 26 (finger snap) | brush-snare 40 (brush swirl L) |
| 27 | perc-extras 27 (High Q) | brush-snare 39 (brush slap) |
| 28 | perc-extras 28 (slap) | brush-snare 40 (brush swirl H: the swirl, lighter) |
| 29 | perc-extras 29 | brush-snare (snare roll: none yet → 38) |
| 30 | perc-extras 30 | perc-extras 85 (castanet) |
| 31 | perc-extras 31 (sticks) | brush-snare 38 (Snare L) |
| 32 | perc-extras 32 (square click) | perc-extras 31 (sticks) |
| 33 | perc-extras 33 (metronome click) | jazz-kick 35 (Bass Drum L) |
| 34 | perc-extras 34 (metronome bell) | brush-snare 37 (open rim shot: a rim shot wanted; the cross-stick until then) |
| 85 | perc-extras 85 (castanets) | perc-extras 85 |

## 7. Built (2026-11-16)

- **`-inst perc-extras`** (`src/percextras.h/.cc`): one instrument holding every sound, keyed as
  §6 says (GS/GM2 numbers; XG's own small sounds on 20, 21, 23, 24). It is a pool of up to 8
  sounding events, the oldest giving way.
  - **Physical:** sticks, whip, finger snap, castanets and the metronome's click and bell are small
    modal banks (up to 16 modes) under a half-sine contact force. The impulse is proportional to
    the stroke's speed; the contact is shorter for a faster blow (Hertz, ∝ speed^−0.2), so harder
    is brighter, not only louder. The contact's own jolt, high-passed above 2 kHz, is added for the
    crack.
  - **Synthesis:** High Q, square click, click noise and seq clicks; the scratches through three
    time-varying formants.
- **Measured** (`-seq`, velocity 100 unless noted):

| Quantity | Value |
|---|---|
| sticks' spectral peaks | 331, 926, 1782, 2958, 4411, 6156 Hz (and the second stick's: 1815, 4478, 6249) against the beam's 330, 910, 1780, 2950, 4400, 6140 |
| spectral centroid | click noise 14 kHz; sticks 3.3 kHz; castanets 2.7 kHz; whip 1.4 kHz; snap 1.1 kHz; metronome click 2.1 kHz; scratches about 1 kHz |
| balance | all 13 sounds equal in LAFmax (within 0.1 dB at velocity 100) |
| velocity 16 to 127 (LAFmax) | 30.0–33.1 dB; the struck ones 31–33 (speed ∝ v^1.6, brighter when hard), the electronic ones 30.0 (v^(5/3)) |
| level | −28.2 LUFS untrimmed would need +22.1 dB, but the short clicks then peak up to 4.8 at velocity 127. The trim is +10.1 dB: 12 dB under the common level, peaks ≤ 1.2 at velocity 127 (click noise 1.2, castanets 0.78, snap 0.81, sticks 0.41) |
| stability | fuzz 20 s: peak 1.32, no resets |
| cost | 31 ns/sample (0.1 % of a core) under random playing |

- **Not done:**
  - the XG/GS map switch and the key tables of §6: they belong to `src/drumkit.cc` /
    `src/midifile.cc` (the other session's);
  - XG 13/14 (surdo);
  - a rim shot and a snare roll on the brushed snare (XG 34 and 29, GS 25);
  - nothing is fitted to recordings; the snap, castanets and whip are by the physics and the ear's
    rough sense.

## 8. Sources

- **[GS]** Roland GS drum map, notes 25–34 (Snare Roll … Metronome Bell):
  https://en.wikipedia.org/wiki/Roland_GS ; Roland SC-8820 drum set list:
  https://www.manualslib.com/manual/314961/Roland-Sc-8820.html?page=127
- **[PSR]** Yamaha PSR-530 drum kit list, notes 13–40 (the XG layout):
  https://babycare.manualsonline.com/manuals/mfg/yamaha/psr530.html?p=106
- **[PG]** PG Music, General MIDI kit lists: on some Yamaha synths 25 is a brush tap, 26 a brush
  swirl, 27 a brush slap. https://www.pgmusic.com/tutorial_gm.htm
- **[B21]** R. Acharya et al. (S. Bhamla's group), "The ultrafast snap of a finger is mediated by
  skin friction", J. R. Soc. Interface 18: 20210672 (2021); press summary:
  https://sciencenews.org/article/finger-snap-physics-new-high-speed-video-thanos
- **[clap study]** Cornell hand-clapping study (the cavity between the palms sets the pitch):
  https://quantumzeitgeist.com/how-hand-clapping-creates-sound-new-study-reveals-physics-behind-this-everyday-action-cornell-university-research/
- **[castanets 2020]** "Seeing the sound of castanets: acoustic resonances between shells
  captured by high-speed optical visualization with 1-mm resolution", JASA Express Letters
  (2020), doi:10.1121/10.0002446. Only the title and DOI were seen; the numbers used here are
  not from it.
