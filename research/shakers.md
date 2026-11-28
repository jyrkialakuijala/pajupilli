# Shakers and jingles: maracas, cabasa, tambourine, shaker, jingle bell

Status: 2026-11-15. Research, a plan (§6) and what was built (§7). For General MIDI's drum keys
70 (Maracas), 69 (Cabasa), 54 (Tambourine), and GM2's 82 (Shaker) and 83 (Jingle Bell). In the
user's MIDI files maracas have 3400 notes, cabasa 1500 and tambourine 80, in bossa and Latin jazz
tunes (`stroke_placement.md`'s counts). Until now these keys were silent.

Marks: **[paper]**, **[web]** a source below; **[code]** a published implementation's constants;
**[calc]** worked out here; **[memory]** general knowledge, to be checked; **[design]** a choice of
this plan.

## 1. Short answer

- **All five are the same kind of instrument:** many small hard things (seeds, beads, discs, balls)
  thrown about inside or against a resonant body by a gesture of the hand. Each collision is a click
  that rings the body's few broad resonances. What is heard is the statistics of the collisions:
  how many, how hard, and when.
- **Perry Cook's PhISEM** (Physically Informed Stochastic Event Modelling) found from offline
  particle simulations of a maraca that the chance of a sound-making collision stays roughly
  constant until the beans are nearly at rest **[paper, C97]**. So a shaker reduces to:
  - a system energy that decays;
  - a random collision process whose rate follows the number of objects;
  - each collision a decaying noise burst, filtered by one to a few resonances.
  It costs two random numbers, two multiplies and a biquad per sample **[web, McGill]**.
- **This project wants more than the statistics**, for two of the user's standing wishes:
  - **The gesture.** The "chick" of a maraca is the seeds hitting the far wall when the hand stops,
    so its timing and spread come from the motion, and two maracas alternate hands.
  - **Repeated strokes meet the state already there.** Seeds still rattling from the last stroke are
    thrown again with the rest.
  So the plan keeps PhISEM's resonances and its cheapness, but drives them from a small
  simulation of the objects themselves: 1-D particles, tens of them, a few operations each per
  sample (§6).

## 2. PhISEM and its published constants [code, STK]

The Synthesis ToolKit's `Shakers` class (Cook and Scavone) gives each shaker as a handful of
constants **[code, STK]**:
- a system decay per sample (the shaking energy's);
- a sound decay per sample (each collision burst's);
- an object count (the collision chance per sample is count/1024);
- the resonances (frequency, pole radius, gain).

| Type | Objects | Resonances (Hz) | Pole radius | System decay | Sound decay |
|---|---|---|---|---|---|
| Maraca | 25 beans | 3200 | 0.96 | 0.999 | 0.95 |
| Cabasa | 512 beads | 3000 | 0.70 | 0.997 | 0.96 |
| Sekere | 64 | 5500 | 0.60 | 0.999 | 0.96 |
| Tambourine | 32 jingles | 2300 (frame), 5600, 8100 (jingles, varied per event) | 0.96, 0.99, 0.99 | 0.9985 | 0.95 |
| Sleigh bells | 32 | 2500, 5300, 6500, 8300, 9800 | 0.99 | 0.9994 | 0.97 |

- At 44.1 kHz [calc]:
  - **Pole radius 0.96 is a bandwidth of (1 − r) fs/π ≈ 560 Hz.** The maraca's gourd is a broad
    resonance, Q ≈ 6. The cabasa's 0.7 is almost no resonance at all (~4 kHz wide): its sound is
    the beads' clicks.
  - **The jingles' 0.99 is ≈ 140 Hz wide, Q 40–60.** Each jingle burst rings ~16 ms (T60): many
    short rings, not one long one.
  - **The maraca's system decay 0.999 is τ ≈ 23 ms**, so a stroke's rattle lasts ~100 ms. The
    cabasa's 0.997 (τ ≈ 7.5 ms) is shorter still.
  - **Collision rates:** maraca 25/1024 a sample, ~1000 collisions a second while the energy is up;
    cabasa 512/1024, ~22 000 a second (a continuous scrape).
- These are the published targets for the bodies' colour and the density of events. The model
  should land near them without copying the algorithm.

## 3. The instruments

### 3.1 Maracas [memory unless marked]

- **Body:** a gourd, or a turned leather, wood or plastic shell, about 7–10 cm across, on a handle.
- **Filling:** 50–200 seeds, beans or plastic and lead shot (0.05–0.3 g each). Heavy fillings
  thud; small hard ones sizzle **[web, Soundcy]**.
- **Played in pairs, one per hand, tuned a little apart.** The hands alternate or move together.
- **The stroke:**
  - The hand moves the maraca 5–15 cm at up to 1–2 m/s and stops it sharply.
  - While it accelerates, the seeds lie against the back wall.
  - When it stops, they fly on and hit the front wall together: the "chick".
  - They bounce back with little restitution (seed on gourd, e ≈ 0.3–0.5) and scatter off each
    other: a short rattle (~50–150 ms).
- **The flight is the delay** [calc]: across 5–6 cm of free space at 1–2 m/s, 25–60 ms from the
  stop to the chick. Players start the motion early so the chick lands on the beat. A MIDI note is
  the beat, so the model starts with the flight already under way (§6.2).
- **The spread of arrival** (seeds at different depths in the pile, at slightly different speeds)
  makes the chick a burst of a few ms, not a click. A soft stroke spreads more and sounds softer,
  because fewer seeds leave the wall at all.

### 3.2 Shaker (GM2 82) [memory]

- A tube (aluminium, wood or plastic), 15–25 cm long, with sand, seeds or beads.
- The same physics as a maraca, with a longer flight, more and lighter particles, and a higher,
  narrower tube resonance.
- It is played in continuous eighths or sixteenths, the push and the pull sounding differently. A
  MIDI note is one push or one pull: they alternate, with the push accented.

### 3.3 Cabasa (69)

- **A scraper, not a shaker:**
  - Loops of steel ball chain (beads ~3–4 mm) are wrapped round a corrugated steel cylinder on a
    handle.
  - One hand holds the beads against the cylinder while the other twists the handle.
  - The beads ride over the corrugations: dense fine clicks, a bright "tsch" lasting as long as the
    twist **[memory]**.
- **Its resonances are less pronounced than the maraca's** **[web, Wadi/Juan Reyes]**. STK gives
  one very broad 3 kHz pole and 512 objects **[code]**.
- **Rates** [calc]:
  - ridges ~2 mm apart, a surface speed of 0.2–1 m/s;
  - so 100–500 ridge crossings a second per bead;
  - with 50–100 beads touching, 10–50 thousand clicks a second.
  - A stroke's twist lasts 60–150 ms.

### 3.4 Tambourine (54)

- **A wooden or plastic frame**, often with no head, with 4–8 slots. In each, a pair of jingles
  (zils), thin slightly cupped brass or steel discs, 4–6 cm across, rides loose on a pin
  **[memory]**.
- **The jingles ring by hitting each other and the slot.** How freely they turn on the pin decides
  how well they ring: a patent argues that slots which let a jingle roll freely about a tangential
  axis ring better than tight ones **[web, US7157637]**.
- **The head (when there is one) is seldom the main radiator.** A study of the tambourine's
  directivity and of its frame's and jingles' modes (finite elements) notes how little the
  literature says beyond the membrane **[paper, TambFEM]**.
- **Timing:** a recorded stroke shows a small early jingle and then the larger one on the beat,
  their peaks about 100 ms apart at any tempo **[web, Kaufman]**. That is the lift before the hit;
  the hit is what a GM key is.
- **The disc's ring modes** [calc]:
  - A flat free brass disc, 5 cm across and 0.6 mm thick (E 100 GPa, ρ 8500), has its first modes
    at about 0.84 kHz (2 diameters) and 1.5 kHz (one circle).
  - The cupping stiffens them (as a cymbal's dome does), and the hole and the rough edges split
    them. The heard jingle sits at 4–10 kHz, as STK's 5.6 and 8.1 kHz.
  - So: three modes per disc between about 3 and 10 kHz, each disc a few % off its neighbours
    [design, to be heard].

### 3.5 Jingle bell (GM2 83)

- Sleigh bells: small hollow spheres with a slit, each with a loose steel ball, many on a strap.
- Each ball rattles in its bell; each bell rings its few modes (STK: 2.5, 5.3, 6.5, 8.3, 9.8 kHz,
  Q high) **[code]**. The tambourine's rattler with a ball for the disc and a bell for the body.

## 4. What the hand does, and what a MIDI note means [design]

- **A key is a stroke:** a sudden stop of the body after a motion. The velocity is the hand's
  speed: 0.3 m/s (ghost) to ~2.5 m/s (accent), roughly speed ∝ velocity^1.2. That is tuned so
  the level spans about 30 dB from 16 to 127, General MIDI's curve (`velcurve.py`).
- **Note-off is ignored**, as GM players do for these keys. One exception, the cabasa: a key held
  long is a slow, long twist, a held "shhh". The twist's length follows the note's, 60 ms to 0.5 s.
- **Alternation:**
  - maracas: left and right hand in turn, slightly different shells, panned a little apart;
  - shaker: push and pull in turn, the pull weaker;
  - tambourine: hits.
- **The state carries over:** objects still moving when the next stroke comes are thrown again
  with the rest. Fast strokes overlap into a continuous rattle, as real ones do.

## 5. What would sound wrong (a listening checklist)

| Fault | Cause | Fix |
|---|---|---|
| Every stroke the same | a stored burst | random arrival spread, per-object speeds and restitution, per-collision body weights |
| A "pshh" with no attack | collisions spread evenly | the stop throws the pile together: a dense first burst, then the scatter |
| Metallic ring on maracas | a narrow resonance | the gourd is broad (Q ≈ 6) |
| A soft stroke as a quiet loud one | only gain follows velocity | fewer objects leave the wall, a wider spread, slower hits: darker and thinner |
| Tambourine as hiss | jingles as noise | discs ringing their own modes after each knock, each pair its own pitches |

## 6. Plan (2026-11-15)

### 6.1 One engine, three exciters

`src/shaker.h/.cc`, one object per instrument for the whole song (as the kick):
- **Bodies:** a few two-pole resonators each (complex rotation, the clavi's), with gain and Q
  per instrument.
- **Rattle** (maracas, shaker): N particles (40–80) in 1-D along the shake axis, in a cavity of
  length L, in the body's frame.
  - At a stroke the body stops: each particle gets the body's speed (±10 %) towards the front
    wall, starting a random 0–10 mm from it (the flight already under way).
  - Wall hits are impulses J = m(1 + e)|u| into the body's modes, each with a random weight (where
    on the wall it lands), through a short contact (one-pole, ~40 µs).
  - Particle-particle scattering is statistical: per sample, with a probability set by the number
    still moving, two moving particles exchange velocities with a loss, and a faint click.
  - A particle under 3 cm/s stops.
- **Rattlers** (tambourine, jingle bell): K units, each a small mass (a jingle disc; a bell's ball)
  in a gap. The frame stops; the masses fly on, hit the stop and the other disc, and bounce
  (e ≈ 0.6). Each hit rings the unit's own modes (3 per disc, a few % apart from disc to disc);
  the frame's knock (3 short wooden modes) is added at the stroke.
- **Scraper** (cabasa): a twist with a raised-cosine speed profile. Clicks are a Poisson process,
  rate = beads × speed / ridge pitch, amplitude ∝ speed × a random factor. They go into one broad
  metal resonance plus the clicks' own high-passed sound. At the twist's end the chains swing:
  a short maraca-like rattle of a dozen beads.

### 6.2 The instruments

| Name | Key | Body modes (Hz, Q) | Objects | Notes |
|---|---|---|---|---|
| `maracas` | 70 | L: 3100 (6), 5200 (4); R: 3400 (6), 5600 (4) | 60 seeds, 0.15 g, e 0.4, cavity 6 cm | two hands alternate, ±30 % side |
| `shaker` | 82 | 4400 (5), 7200 (4) | 80 grains, 0.05 g, e 0.35, cavity 15 cm | push and pull alternate (the pull at 0.7) |
| `cabasa` | 69 | 3000 (1.5), 6500 (2) | 80 beads touching, 2 mm ridges | twist 60–150 ms by velocity, longer for a held key |
| `tambourine` | 54 | frame 250, 520, 1100 (Q 4); discs 3.2, 5.6, 8.1 kHz (Q 50–80) ± 6 % | 6 pairs = 12 discs, 6 g, gap 2 mm, e 0.6 | a hit |
| `jingle-bell` | 83 | bells 2.5, 5.3, 6.5, 8.3, 9.8 kHz (Q 80–120) ± 5 % | 10 bells, ball 2 g, gap 3 mm, e 0.5 | a shake |

### 6.3 Checks

- Envelope: the maraca chick peaks within 10 ms and is down 30 dB by 150 ms; the tambourine's
  jingles ring longer (−30 dB at 200–400 ms).
- Spectral centroid: maracas 3–6 kHz, cabasa above 5 kHz, tambourine 5–8 kHz.
- No two alike: the correlation between two strokes at the same velocity stays under 0.5.
- Velocity: 25–35 dB from 16 to 127.
- No blow-up in 10 s of random playing; cost under 1 % of a core each.

## 7. Built (2026-11-16)

- **Files:** `src/shaker.h/.cc` (`Shaker`, `ShakerModel`, `ShakerPlayer`); instruments `maracas`,
  `shaker`, `cabasa`, `tambourine`, `jingle-bell` (kPlucked rows, family "drum", so the main
  room); a test, `tools/test.py` `shakers()`.
- **As planned in §6, with these changes found by measuring:**
  - **The body's sound has a zero at DC.** A small closed body radiates no steady push. Without
    it, the chick's one-way hits passed their mean through the broad resonances: the shaker's
    centroid read 47 Hz.
  - **Each hit drives the modes by their shapes where it lands.** The main mode is driven the
    same way everywhere, the others with random signs. Driven coherently, 60 seeds landing
    together peaked +6 dBFS above a normal loudness.
  - **The contact is softer:** 80 µs, seed on gourd.
  - **The free path is the shell less the pile:** 2.5 cm in a maraca, 7 cm in the shaker.
  - **The arrivals trail off exponentially.** A hard-edged arrival window left a 30 ms hole
    between the chick and the rattle.
  - **The seeds settle on the pile** (drag τ = 70 ms). Without it they bounced on for seconds.
  - **The jingles and bells leave their rest only on a hard enough stroke**, as the seeds do.
  - **The hand's speed law differs by instrument:** maracas v^1.4, shaker v^1.35, cabasa
    v^1.2, tambourine and bells v^1.4, all on General MIDI's ~30 dB.
- **Measured** (`-seq`, dry, velocity 100 unless noted):

| | Maracas | Shaker | Cabasa | Tambourine | Jingle bell |
|---|---|---|---|---|---|
| envelope peak (ms) | 1.5 | 0.6 | 39 (the twist) | 5 | 16 |
| −30 dB after the peak (ms) | 11 (the rattle under it: −27 dB at 20–70 ms) | 13 | 71 (a short twist) | 101 | 115 |
| spectral centre (Hz) | 3500–3700 | 4900 | 9700 | 6100–6600 | 6200–6400 |
| LAFmax 16 → 127 (dB) | 28.6–30.0 | 29.5–31.0 | 31.1–31.4 | 27.1–27.3 | 29.1–31.1 |
| two strokes' correlation | 0.02–0.16 | 0.08–0.18 | 0.01 | 0.14 | 0.44 |
| level with the trim (LUFS) | −34.0 | −32.8 | −28.2 | −28.2 | −28.2 |
| cost (ns/sample) | 62–83 | 62 | 21–42 | 42–83 | 21–83 |

  - The maracas and shaker are trimmed by their peaks, not their loudness: a velocity-127 stroke
    peaks at about 1.0. Their chick's crest is ~30 dB, so at the common −28.2 LUFS a hard stroke
    would peak at +6 dBFS.
  - The cabasa held 0.6 s: 7.7 dB under its peak in the hold, −76 dB 0.15 s after the release.
  - Fuzz, 10 s of random playing each: no resets.
- **Not done:**
  - the tambourine's head (most are headless) and its early jingle before the beat (the lift,
    §3.4);
  - shaking rolls on long notes;
  - room pressure on the bodies (negligible);
  - fits to recordings: none of these instruments is in the separated stems.

- **2026-11-27:** the tambourine's spectrum revised against recordings (a 4th disc mode, less
  3 kHz, a contact that grows brighter with speed): `research/tambourine.md` §8.4.

## 9. Keys for `drumFor` (src/drumkit.cc)

    case 54: return {"tambourine", 54};
    case 69: return {"cabasa", 69};
    case 70: return {"maracas", 70};
    case 82: return {"shaker", 82};      // GM2
    case 83: return {"jingle-bell", 83}; // GM2

## 8. Sources

- **[C97]** P. R. Cook, "Physically Informed Sonic Modeling (PhISM): Synthesis of Percussive
  Sounds", Computer Music Journal 21(3), 1997. Summaries:
  https://caml.music.mcgill.ca/~gary/618/week12/phism.html and
  https://caml.music.mcgill.ca/~gary/307/week11/node12.html (the collision statistics; the cost).
- **[STK]** The Synthesis ToolKit, `Shakers.cpp` (P. Cook, G. Scavone):
  https://github.com/thestk/stk/blob/master/src/Shakers.cpp (the constants in §2).
- Cook and Lakatos, WASPAA 2003: https://www.cs.princeton.edu/courses/archive/spr05/cos598E/bib/CookWASPAAFinal03.pdf
- **[Wadi]** J. Reyes, a maraca model: https://ccrma.stanford.edu/~juanig/articles/wadi-icmc/Model_Maracas.html
  (the cabasa's resonances are less pronounced than the maraca's).
- **[Kaufman]** J. Kaufman, "What Noise Does a Tambourine Make?", 2020:
  https://www.jefftk.com/p/what-noise-does-a-tambourine-make (the early and the on-beat jingle,
  ~100 ms apart).
- **[US7157637]** Tambourine patent: https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/7157637
  (the jingles' free rotation and their ring).
- **[TambFEM]** A tambourine study (directivity, the spectrum by playing style, frame and jingle
  modes by finite elements): https://synapsesocial.com/papers/W4400287776 (abstract only read).
- **[Soundcy]** https://soundcy.com/article/what-do-maracas-sound-like (heavy fillings thud,
  small hard ones sizzle: qualitative).
