# The snare with sticks: strokes, the open rim shot, the buzz roll

Status: 2026-11-16. Research, plan and what was built (§3). It extends `jazz_drums.md` §5 (the
snare: heads, wires, air, the stick on the head, rim shot and cross-stick) and is built on the
other session's brushed snare (`src/snare.*`), wrapped, not copied.

Marks: **[paper]**, **[web]** a source below; **[jd]** from jazz_drums.md; **[calc]** worked out
here; **[memory]** general knowledge, to be checked; **[design]** a choice of this plan.

## 1. Research

### 1.1 Why it is needed

- In the user's MIDI files the drum channel selects the Standard kit (program 1, 166 files), the
  Jazz kit (33, 66 files) or the Brush kit (41, 48 files). In the first two the snare is played with
  sticks; until now every snare key went to the brushed snare.
- The XG open rim shot (key 34) is played 804 times, in triplets with the hats, and fell back to the
  cross-stick. The XG / GS snare roll (29 / 25) fell back to a single tap. (`gm2_extras.md`.)

### 1.2 The stick on the batter

- **Contact:**
  - a hickory tip on a coated head, 1–5 m/s: about 0.5–1.5 ms **[jd §5.4, memory]**;
  - the stick–membrane interaction is little measured. A KTH thesis instrumented a stick for
    force, tip acceleration and contact time, and calls it "a nearly white spot" **[paper, W06]**.
- **Tip mass:**
  - what of the stick is behind the blow: 10–20 g at the tip, more with the wrist locked **[memory]**;
  - the toms here use 15 g and a Hertz contact of 2e8 N/m^1.5 on the coated film (`toms.md` §9).
- **The stick's own ring:** a 40 cm hickory stick free at both ends, 324, 892, 1749 Hz **[jd §5.5]**.
  - The hand damps it within 20–50 ms **[jd]**.
  - Heard in a stroke as a small woody "tok", much louder when the shaft hits the hoop.
- **Strike position:**
  - the centre gives a fat, short sound;
  - off centre brings in the diameter modes and the ring;
  - near the edge is thin and quiet **[jd §5.4; stroke_placement.md §2]**.
  - Measured snare strokes show spectral peaks well away from the head's modal frequencies, the shell
    and the wires counting as much as the head **[paper, SHP06]**.
- **Ghost notes and accents** in jazz comping: ghosts 0.2–0.5 m/s, accents 3–6 m/s, rim shots up
  to ~8 m/s **[jd §5.4]**.
- **The wires** follow the snare (bottom) head. They ride it while it moves gently and slap back when
  it drops away faster than they can follow. A ghost note gives a short thin buzz, an accent a long
  rattle **[jd §5.2]**. The other session's `Snare` models this in full (10 wires × 8 contact
  points).

### 1.3 The open rim shot

- The stick hits the head and the hoop at once: the tip a little inside the rim, the shaft on the
  hoop **[jd §5.5]**.
  - The hoop (steel or die-cast) and the shell get a large force.
  - The head gets a hard, off-centre blow.
  - A loud "crack" with the hoop's ring, about 1–3 kHz **[jd]**.
- A patent on electronic drums describes the rim shot's sound as the longest-sustained of the
  drum's, with "stretched harmonics" **[web, patent]**. That is descriptive, not measured.
- **The open rim shot against the cross-stick:**
  - the cross-stick (37) is the butt on the hoop with the tip end lying on the head, a woody knock,
    the head damped;
  - the open rim shot is a full stroke that also catches the rim, with the head ringing free and
    loud.
- **Hoop modes:** a steel ring bending out of its plane, n (n² − 1)/√(n² + 1) × 137 Hz for n = 2…6:
  368, 1040, 1995, 3226, 4730 Hz, damped by the lugs and the head in 25–60 ms **[jd; snare.cc]**.

### 1.4 The roll

- **The buzz (press) roll:**
  - each hand presses the stick into the head, and it bounces several times before the next
    hand's stroke;
  - the hands alternate.
- **Rates:**
  - skilled drummers exceed 30 strokes a second by letting the stick bounce at least twice per
    hand stroke;
  - the grip's stiffness sets the bounce rate;
  - a robot reproducing this used bounce intervals of 40–160 ms **[paper, roll robots]**.
- **Quality:** a good roll is even in its bounce intervals and amplitudes **[paper, roll robots
  citing Brown et al.]**.
- **The bounces:** each bounce comes back weaker (the restitution, the grip's loss) and sooner, a
  geometric series in time **[paper, O22]**.
- **The buzz:** a press roll at 6–9 hand strokes a second per hand, 3–5 bounces each, gives 40–80
  impacts a second: the continuous "zzz" **[calc]**.

## 2. Plan (2026-11-16) [design]

1. **Wrap `Snare`** (heads, air, wires, microphones, cross-stick knock), at its default jazz tuning.
   Its two brushes stay up.
2. **Two sticks**, the drummer's hands. Each is a mass on a stiff Hertz contact, thrown from just over
   the batter where it is now (a stroke meets the ringing). The contact is solved for the end of
   each sample against the head's give, as `hihat.cc` and `tom.cc` do. It pushes into
   `batter().force()` before `Snare::sample()`.
3. **The stick's own ring:** three modes (324, 892, 1749 Hz, 40–30 ms), driven by the contact force.
4. **The click above the modes:** the contact force high-passed above the batter's top, as a force
   on a light film moves the air at once (as the toms').
5. **The open rim shot:** a heavier stroke (more of the stick behind it) near the rim, and the hoop's
   five modes with the stick's three struck by the shaft at once.
6. **The cross-stick:** the `Snare`'s own.
7. **The buzz roll:** a held key.
   - The hands alternate at a rate that rises with velocity (6–9 a second each).
   - Each hand stroke is followed by 3–5 bounces, each about 0.75× sooner and 0.55× weaker.
   - Jitter of a few percent.
   - It stops at the key's release.
8. **Keys on the instrument** (36–44, so that `tools/levels.py`'s three probes land on strokes):

| Key | Stroke |
|---|---|
| 36 | the buzz roll, held |
| 37 | the cross-stick |
| 38 | a stroke, middle |
| 39, 41 | the open rim shot |
| 40 | a stroke, off centre |
| 42 | a stroke, near the edge |
| 43, 44 | strokes |

9. **Measure:**
   - wire buzz present and following the head (ghost vs accent);
   - rim-shot crack level and spectrum against a normal stroke;
   - roll rate and evenness;
   - velocity range 16 → 127;
   - cost and stability.

## 3. Built (2026-11-16)

- **Files:** `src/sticksnare.h/.cc`: `StickSnare` wraps the other session's `Snare` (src/snare.h,
  unchanged), with `StickSnareModel` and `StickSnarePlayer`; `-inst stick-snare`, keys 36–44.
- **The stick:**
  - the implicit Hertz contact of `tom.cc` and `hihat.cc` (K 2e8 N/m^1.5), pushing into
    `Snare::batter()` before `Snare::sample()`;
  - two sticks, the hands, each stroke thrown from just over the head where it is now.
- **What was changed from the plan, and why:**
  - **Tip mass 9 g** (PJ_SSNARE_MASS 0.6 × 15 g). The contact time is set by the head's give under
    the stick, not by the contact's stiffness: K from 2e8 to 1e9 changes nothing. It goes as √mass:
    15 g 5 ms, 9 g 3.8–4 ms, 6 g 3.3 ms. Longer than the 0.5–1.5 ms of §1.2 **[memory]**: a real
    stick also pivots in the fingers and leaves sooner. Left as the model gives it, to judge by ear.
  - **Rim shot:**
    - the hoop's modes extended to n = 10 (6.5, 8.6, 10.9, 13.5 kHz; 20–10 ms), its strike
      10 × the stroke speed (PJ_SSNARE_HOOP);
    - each mode starts at a random phase: the shaft lands along the hoop, so the modes don't all
      start together;
    - started in phase, the crack had a needle of a peak: +11 dB over a stroke and 6.1 in random
      playing. With random phases it is +5.8 dB, the same loudness.
  - **Velocity:** ~v^1.75 (from 1.5: 26 dB was short of 30).
  - **Level:** 2 dB under the common −28.2 LUFS (trim −1.0 dB; −30.2 LUFS on strokes at
    velocity 90). That keeps an accent's peak near 1.5, not 1.8: `-play` scales a whole song by its
    peak.
- **Measured:**

| Quantity | Result |
|---|---|
| Contact, velocity 30 / 90 / 127 | 4.0 / 3.9 / 3.8 ms; peak 3 / 20 / 39 N |
| Velocity 16 → 127 (LAFmax A) | 31.3 dB (16 → 64: 20.3 dB) |
| The wires (2–8 kHz, 30–200 ms, wires on vs off) | +67.7 dB (ghost, v30), +64.0 dB (v110): the high band is theirs |
| The rattle's length to −40 dB | 78 ms (ghost) against 100 ms (v110): it follows the head |
| Open rim shot vs stroke, v100 | +6.1 dB LAFmax, peak +5.8 dB, 1–3 kHz +2.4 dB, centroid 391 → 589 Hz |
| Cross-stick vs stroke, v100 | −11.3 dB |
| Buzz roll, v50 / v100 | 67 / 87 impacts a second; 20 ms level p10..p90 8.4 / 6.9 dB; −112 dB 0.4 s after the release |
| Peaks, v127 | stroke 1.46, rim shot 2.70, roll 1.27; random playing 3.3 |
| Cost | 1500 ns/sample (7.2 % of a core), mostly the `Snare`'s wires |
| Fuzz | 10 s, no resets |

- **Not done:**
  - the stick's place doesn't drift with the phrase (stroke_placement.md §7.2); it scatters;
  - no stick-on-snare buzz from the other drums (that needs the shared room);
  - no open double-stroke roll, only the buzz roll;
  - nothing fitted to recordings of a stick snare (the jazz stems' soft passages are mostly
    brushes).

## Sources

- **[W06]** A. Wagner, master's thesis, KTH 2006 (an instrumented drumstick: force, tip
  acceleration, contact time). https://www.speech.kth.se/publications/masterprojects/2006/AndreasWagner.pdf
- **[SHP06]** E. Skrodzka, E. Hojan, R. Proksza, "Vibroacoustic investigation of a batter head of a
  snare drum", Archives of Acoustics 31(3), 2006. https://acoustics-old.ippt.pan.pl/index.php/aa/article/view/674
- **[patent]** Electronic drum, US 7473840 (rim shot sustain). https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/7473840
- **[roll robots]** "Multi-bounce Drum Roll with Optimized Active Tricks to Leverage Soft
  Embodiment", arXiv 2609.08490 (rolls over 30 Hz; bounce intervals 40–160 ms; uniformity, after
  Brown et al.). https://arxiv.org/pdf/2609.08490
- **[O22]** S. Oss, "drumroll" (a rigid-body model with restitution and time of flight), Phys.
  Educ. 57, 065021 (2022). https://iris.unitn.it/retrieve/8e75e667-bae9-4b03-9889-bb8919eaf733/Oss_2022_Phys._Educ._57_065021.pdf
- **[jd]** `research/jazz_drums.md` §5.
