# Where the strokes land: placement that follows the music

Status: 2026-11-15. A design; nothing is built. It is for the physical drums (`toms.md`,
`jazz_drums.md`, `cymbal_plan.md`), whose models take a stroke's place on the head or the cymbal.
The present drums (the circuit snare and toms) have no place to give.

Marks: **[user]** the user's account of the playing; **[paper]** a source below; **[calc]** worked out
here; **[design]** a choice of this plan; **[memory]** general knowledge, to be checked.

## 1. The practice [user]

- When brushing the snare, the spot the brush lands on **travels back and forth, about every 8 or 16
  strokes.**
- It is the drummer leaning back and forth, moving both hands' positions with the body.
- It keeps a better conceptual structure of the rhythm: the pattern of places is a map of where one
  is in the bar and the phrase.
- In a song it may be linked to other things going on in the music.

What the listener gets from it: the sound changes in a way that is **structured, not random**. The
colour comes back to where it was after each turn, and the turns fall on the music's own
boundaries. A model that scatters each stroke at random (the usual "humanise") gives the variety but
not the structure.

## 2. What the place does to the sound [calc]

A stroke at radius r drives each membrane mode by its shape there, J_m(j_mn r/R) (times cos m θ).
The energy each mode gets, per unit modal mass, in dB against its own mean over the head:

| Mode | r = 0 | 0.2 R | 0.4 R | 0.5 R | 0.6 R | 0.8 R |
|---|---|---|---|---|---|---|
| (0,1) | +0.7 | +0.2 | −1.4 | −2.8 | −4.6 | −10.7 |
| (1,1) | — | −3.0 | +0.9 | +1.2 | +0.6 | −4.1 |
| (2,1) | — | −11.0 | −1.3 | +0.6 | +1.2 | −2.1 |
| (0,2) | +4.4 | +1.5 | −15.1 | −11.1 | −4.8 | −5.0 |
| (3,1) | — | −19.6 | −4.4 | −0.8 | +1.0 | −0.7 |
| (1,2) | — | +3.2 | +0.7 | −9.0 | −8.5 | −1.1 |
| (0,1)'s share of the six | 0.30 | 0.20 | 0.17 | 0.13 | 0.08 | 0.03 |

- **The centre** gives a fat, short sound: (0,1) and (0,2) only.
- **From 0.4 to 0.6 R** the diameter modes come up 10–20 dB: more ring and overtones.
- **Near the edge** the fundamental is gone (−11 dB), and the sound is thin and bright. On a snare it
  is also quieter.
- Measured snare strokes agree in kind. Off centre the spectral peaks move away from the modal
  frequencies, and the shell and wires count as much as the head **[jd §5.1]**.
- Players control this deliberately. Snare datasets of centre, near-centre, halfway, near-edge and
  edge strokes were recorded, and the positions told apart by a classifier from the sound alone
  **[paper, T04; P13]**.
- **The angle** matters too. The diameter modes are split into close pairs by the uneven tension
  (`toms.md` §2.4), and θ sets the balance between the two members, so the slow beating changes
  with the angle. On a snare, the place relative to the wires underneath matters as well.
- **Brushes:** the footprint (4 × 8 cm) averages the shapes. Moving it changes the colour in the same
  direction, more gently.
- **On the other surfaces:**
  - ride: bow, nearer the bell, or the bell;
  - hi-hat: the top against the edge;
  - toms: centre against off-centre (`toms.md` §3).

## 3. The pattern [design]

### 3.1 One hand's place over time

- **A slow path between two places**, e.g. the near-centre (0.25 R) and the off-centre (0.55 R),
  through a smooth turn: a cosine, a lean is not a zigzag.
  - The hand also moves a little sideways with the body (θ ± 15°).
- **The period:** 8 or 16 strokes, or better, **in the music's time**: one way per 2 or 4 bars. The
  measurements (§7.1) show phrase-held states rather than a regular swing; §7.2 updates this.
  Stroke density changes (a fill, a sparse ballad), and counting strokes would then drift against
  the phrase. With a file, the turns are snapped to bar lines (§4). Live, it counts strokes (§5).
- **Scatter on top:** each stroke lands within a few mm of the path (σ ≈ 5–10 mm; more with
  brushes) **[memory]**.
- **Both hands lean together.** When the body leans, the tapping hand and the sweeping hand move the
  same way, each from its own home: the left hand's taps on 2 and 4 near one side, the right hand's
  circle on the other.

### 3.2 The parameters

| Parameter | Default | Range |
|---|---|---|
| inner place (r/R) | 0.25 | 0.1–0.4 |
| outer place (r/R) | 0.55 | 0.4–0.75 |
| sideways (θ) | ±15° | 0–30° |
| one way | 2 bars (8 quarter strokes) | 1–8 bars, or 4–32 strokes |
| shape | cosine | cosine, triangle |
| scatter | 6 mm | 0–15 mm |

## 4. From a MIDI file: linking the lean to the music

The player reads the whole file before it plays (the first pass in `src/midifile.cc`), so it can look
ahead. A drummer anticipates a section; the lean can begin before the boundary it leads to.

### 4.1 The grid

- **Tempo and meter:**
  - the tempo map is read already;
  - the time signature (meta FF 58) is not yet: it is needed for the bar lines. Without one, 4/4.
- **Bars and beats** as times, and each stroke's place in them.

### 4.2 The music's structure, found in the file [design]

| Cue | How to find it | What it does to the lean |
|---|---|---|
| phrase (4 or 8 bars) | default: every 4 bars, refined by the cues below | a turn at each phrase end |
| harmonic rhythm | the bass part's (or the lowest notes') changes of pitch class per bar | turns on the bars where it changes; a long held chord, a long way |
| repeated sections (AABA, 12-bar blues) | bars compared by their note patterns (pitch classes and onsets); a repeat starts where the similarity jumps | at a new section the hand goes to the other side, or a third place (the bridge played nearer the edge) |
| a part entering or leaving | a channel's first and last notes in a passage | a section boundary |
| crashes and fills | drum keys 49/57 (crash), a burst of tom keys | a section boundary; after a crash, a fresh start from home |
| the music's loudness | the velocity and the number of notes per bar, smoothed over a bar or two | louder: the place moves in (fuller); softer: out (thinner, quieter) |
| the soloist's breaths | the melody's (top channel's) rests longer than a beat | the lean turns in the gap |

- The lean is then a function of the song's time: its phase is set by the phrase boundaries, its
  centre and width by the section and the loudness.
- **What each stroke gets:** (r, θ) for its surface, given to the model with the stroke.

### 4.3 What it can be wrong about

- The cues are guesses at the form. If they disagree (a phrase of 5 bars, a pickup), the lean must
  not jump: a turn moves at most halfway in a bar.
- With no cues (a drum-only file), it falls back to counting bars.

## 5. Live

- No score. **Counting strokes:** turn every N strokes (8 or 16, a setting).
- **A rest** (no stroke for more than about two beats at the present tempo, the tempo from the recent
  strokes' spacing) ends the phrase: the next stroke starts from home.
- **With `-pads drums` beside an instrument,** the other part's own phrasing can drive it the same way
  as a file's melody: its rests turn the lean.

## 6. Building it

- **A small "drummer" layer in the kit** (`src/drumkit.*`, after the other session's commit), per
  surface a lean state:
  - `place(surface, time, key, velocity) → (r, θ)`;
  - for a file, a context built in the first pass: the bar grid, phrase and section boundaries, the
    loudness curve;
  - live, the stroke counter.
- **It needs the models that take a place:** the tom plan's Bessel table and shape vector at (r, θ)
  (`toms.md` §5.3), the snare's, and the cymbal plan's strike points. It is built with or after the
  physical snare.
- **The MIDI reader:** read the time signature (FF 58); keep the bars.

## 7. Checking it

- **In the recordings.** A stroke's place can be read from its sound: the diameter modes' energy
  against the (0,1)'s, the feature the snare classifiers use **[paper, T04]**.
  - On the separated jazz drum stems (`drums.md`), measure it per snare stroke and plot it against
    the bar number.
  - A drift with a period of 2–4 bars, turning on the phrase ends, is the pattern of §1. Its depth,
    period and link to the form are then fitted from real players, not set by hand.
  - Brush passages (*Some Cats Know*, 1:01–1:10 **[drums.md]**) first.
- **By ear:**
  - with the lean against fixed placement and against random scatter alone, on a ballad and a
    medium swing from the bushgrafts files;
  - the turns should be heard as the phrase, not as a wobble.

### 7.1 Measured (2026-11-15, `tools/placementdrift.py`)

- **The passages:** seven passages of the separated jazz drum stems where the treble falls nearly
  silent between strokes, so no ride is playing: 35–57 dB from peak to floor above 4 kHz, against
  10–25 dB under a ride. Brushes, or a quiet snare with sticks: the stems do not tell which, so this
  is "soft snare playing", mostly brushes.
  - Cullum, *I'd Probably Do It Again*, 0–75 s;
  - Cullum, *London Skies*, 15–90 and 135–195 s;
  - *Fly Me to the Moon*, 0–60 s;
  - Bönner, *Ku'damm*, 0–60 s;
  - Ray Charles, *Lonely Avenue*, 0–155 s (likely sticks);
  - *Some Cats Know*, 14–35 s (brushes).
  65–533 strokes each.
- **The feature:** the head's upper modes (500–2000 Hz) against its (0,1) band (120–500 Hz), per
  stroke, over 100 ms. Several things are removed from it before testing:
  - the stroke's loudness;
  - the music's own five bands at that moment (the separation's leakage, which has the same phrase
    structure as the drums and would fake a drift);
  - the bar's own pattern: each stroke group's mean (beat in the bar × on the beat or the swung
    eighth, from a fitted beat grid).
  Each test is against 200–500 shuffles of the strokes within their groups.

| Passage | Strokes | 8-beat running mean | p | 2-bar blocks | 4-bar blocks | 8-bar blocks | 4-bar blocks, lag-2 r |
|---|---|---|---|---|---|---|---|
| I'd Probably | 239 | 2.7 vs 2.0 dB | 0.003 | 3.2 vs 2.5 (p 0.07) | 2.5 vs 2.2 (0.27) | 3.2 vs 2.2 (0.16) | −0.38 |
| London Skies 1 | 256 | 2.8 vs 2.0 | 0.000 | 2.9 vs 2.2 (0.005) | 3.1 vs 1.8 (0.01) | 3.3 vs 1.6 (0.005) | +0.78 (a slide) |
| London Skies 2 | 170 | 3.3 vs 2.1 | 0.000 | 3.4 vs 2.3 (0.000) | 3.9 vs 2.0 (0.015) | 5.5 vs 1.9 (0.000) | +0.19 |
| Fly Me | 154 | 3.9 vs 2.0 | 0.000 | 5.1 vs 2.4 (0.000) | 6.2 vs 2.0 (0.000) | 6.8 vs 2.1 (0.000) | −0.61 |
| Ku'damm | 223 | 2.7 vs 1.9 | 0.000 | 3.0 vs 2.2 (0.000) | 2.4 vs 1.7 (0.05) | 2.0 vs 1.6 (0.22) | −0.51 |
| Lonely Avenue | 533 | 3.1 vs 2.7 | 0.013 | 3.4 vs 2.9 (0.005) | 2.8 vs 2.2 (0.005) | 2.8 vs 1.8 (0.015) | −0.18 |
| Some Cats Know | 65 | 4.4 vs 2.9 | 0.000 | 5.1 vs 3.6 (0.03) | 5.2 vs 3.1 (0.015) | 5.8 vs 2.7 (0.01) | (n 5) |

  The block columns are the sd of block means against the shuffles' median, best over the offsets
  for both.

- **What it shows:**
  1. **The strokes' colour moves slowly, beyond chance, in all seven passages.** The slow part (over
     8 beats) is 2.7–4.4 dB, about 1.2–2× what shuffled strokes give, on top of 6–8 dB of
     stroke-to-stroke scatter. It shows in bar-aligned blocks of 2 to 8 bars too, in 5–6 of the 7.
  2. **No regular pendulum.** No fixed period stands out: the best periods have p 0.07–0.88. Nothing
     clocks back and forth every N strokes.
  3. **It holds for a bar or two, then turns.**
     - Consecutive 1–2-bar blocks are alike (lag-1 r +0.2 to +0.6 in most).
     - Between 4-bar blocks two apart the correlation is negative in 5 of 7 passages (−0.2 to −0.6),
       so the colour returns over about 8–16 bars.
     - One passage (*London Skies* 15–90 s) slides one way all along instead.
  4. **Size.** About 3 dB in this feature between blocks. By the table of §2 that is a travel of about
     0.25 R to 0.5 R (the (0,1)'s share halving), a few cm on a 14" head. Modest, as a lean would
     be.
- **What it cannot show:** that the cause is the place.
  - Anything that changes the strokes' colour slowly reads the same: the brush's angle or pressure,
    more sweep against taps in a section, the kick's feathering, a dynamic arc beyond the linear
    level term, a slow change of leakage between solos.
  - The stems do not separate the two hands, nor brushes from sticks.
  - The 4-bar lag correlations rest on 7–34 blocks each, so they are indications only.

### 7.2 What it means for the design

- **The lean is real enough to model**, and of the size of §3.2's defaults: about 0.25–0.5 R.
- **Not a pendulum on a stroke count, but phrase-held states:** the lean holds for 1–2 bars (or a
  phrase), moves at the phrase boundaries, and over 2–4 such moves tends to come back. One
  passage in seven drifted one way throughout.
- So §3.1 becomes [design]:
  - the place is a smoothed sequence of targets, one per phrase (default 2 bars), changing at the
    boundaries;
  - each new target is on the other side of home with probability ~0.6–0.7 (back and forth), else
    on the same side (a slide). Its distance is random within the range;
  - the move takes about a beat (a lean, not a jump);
  - §4's cues still set where the boundaries fall and how wide the range is.
- **To pin it down further:** stems with brushes alone (the *Some Cats Know* passage is only 21 s), a
  hand-labelled check of a few passages by listening, and the two hands told apart by their beats
  (taps on 2 and 4, the sweep's accents).

## 8. Sources

- **[T04]** A. Tindale, A. Kapur, G. Tzanetakis, I. Fujinaga, "Retrieval of percussion gestures using
  timbre classification techniques", ISMIR 2004. https://archives.ismir.net/ismir2004/paper/000235.pdf
  - Snare strokes at centre, near-centre, halfway, near-edge, edge, a rimshot and a brush stroke.
  - The positions were told apart from the sound.
- **[P13]** Expressive snare and drum-kit samples varied by intensity, stroke height and strike
  position, ISMIR 2013. https://archives.ismir.net/ismir2013/paper/000242.pdf
- **[D15]** A. Danielsen et al. (with H. G. Sundt), "Effects of instructed timing
  and tempo on snare drum sound in drum kit performance", JASA 138(4), 2015.
  - Laid-back, on-beat and pushed playing differ in intensity and timbre as well as in time.
  - Timbre carries the rhythm's intent, which is the same idea at the stroke level.
- **[jd]** `research/jazz_drums.md`.
