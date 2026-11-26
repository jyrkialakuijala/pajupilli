# Training: what the players practise, and what it taught us

The wind, brass and bowed players do not play a slur, a start or a trill by one fixed rule.
They practise: a tool plays each thing on the model many ways, listens, and writes down the way
that worked. The players read the result at run time. This note records what was learned while
building that (2026-11-25). The method itself is in `research/slurs.md` (section 13); the tools
are `tools/slurs.py` and `tools/colours.py`.

Everything below is from the practice's own measurements unless it says otherwise. None of it
has yet been judged by ear, and the ear outranks the numbers.

## 1. What is practised

| Thing | Per | What is searched | Who |
|---|---|---|---|
| Slur | interval, start note, dynamics, speed | fingers' speed, lip lead, breath push, lips, tongue touch, lip opening, firmness, vowel | brass, reeds, Baroque flute |
| Slur (bowed) | the same | left hand's speed, bow speed push, bow pressure pulse, bow place, the crossing's overlap and pick-up | double bass, cello, viola, violin |
| Start (attack) | note, dynamics | tongue release, breath rise and push, lips' first position, firmness, lip opening, vowel | brass, reeds |
| Trill | lower note, interval, kind | one gesture for both directions | brass, reeds |
| Colour (measured, not searched) | note, dynamics, syllable | which vowels and consonants can be heard | brass, reeds |

A cost in ms scores each try: the time until the new note is there and in tune, plus penalties
for overshoot, a wrong partial on the way, a hole or a bump in the level, and effort. Lower is
cleaner. A slur no gesture can make work is marked as failing, and the player tongues it lightly.

## 2. What the practice taught us about playing

### A slur out of a settled note is not a slur in a run

The first practice held every note 0.6 s before slurring. In a run of sixteenths the note left
has only just spoken, and the gestures of consecutive slurs pile up. Practised separately, the
quick slurs chose different gestures (a smaller lip lead, mostly) and improved 30 to 50 % on the
trumpet over using the settled gesture. Every slur is now practised at both speeds, and the
player blends between them by how long the note it leaves has sounded (0.1 to 0.4 s).

### More of the embouchure fixes slurs that breath and lips alone could not

Adding the lip opening, the lips' firmness and a mouth vowel to the gesture:

- Muted trumpet G3 to E3 landed about 190 cents flat with any breath-and-lip gesture. A
  narrower lip opening fixed it.
- Bassoon E2 to B1 settled in 229 ms. Firmer lips brought it to 121 ms; a vowel tuned about an
  octave and a half above the note, to 131 ms.
- Trumpet A#5 to F#5: looser lips, 54 ms to 36 ms.
- A focused breath (less turbulence) alone changed almost nothing.

One step at a time does not find what only helps together (a vowel and where it is tuned), so
the hard slurs are also tried from several ready-made techniques before the search.

### Some failures are not the player's

No embouchure setting moved the saxophones' slurs that land an octave off (soprano G#3 to F4),
or the alto's C#5 up into the high register. Those are in the fingering model. The bassoon's
and tuba's slurs down into their lowest notes, and the trombone's D4 to D#4 played softly, also
resist every gesture. Practice cannot fix a note the instrument does not have.

### Starts

A practised start roughly halved the cost on every instrument (trumpet 310 to 116, horn 515 to
259, alto sax 206 to 85, bassoon 353 to 205). The common finding on brass: firmer lips with a
slightly smaller breath push than the player's fixed rule, so less bark. Some notes never
settled within the half-second test note on any occasion and got no practised start (horn D3 to
E3, muted trumpet E3 and F3, soprano sax F#5, oboe C4 soft, tuba D2 and D#2 soft); for the low
horn and tuba the test note may simply be too short.

### A trill is not a row of slurs

Rehearsed as trills (the embouchure held, the fingers moving), the gesture differs from a
slur's: the fastest fingers, little lip movement. On the trumpet's whole-tone trill the cost fell
from 260 to 101. Each kind is rehearsed apart (steady, quickening, growing, dying away, an
emphasis half-way or three quarters in, linked on into the next note), and the director now
chooses a kind for each long trill by its context.

### The trombone's slide: mostly faster, sometimes slower

The slide's speed was almost outside the practice's reach (a gesture could slow it 1.6 times at
most). Given a real handle on it (to 3.3 times slower), the practice chose a faster slide for
1342 of 1559 slurs and a slower one for 79. Slower wins on some downward slurs (C5 to F4) and
loses on small upward ones. A caveat: the score mostly rewards arriving soon, which favours a
quick slide. If "not stable" to the ear means a wobble after arrival, the score should weigh
that more.

### Tuning belongs in the practice

The players had only practised at equal temperament with their ear switched off, which is how
the calibration plays them. In an ensemble the ear is on and notes sit several cents away. A
probe showed the gap: the baritone sax's D3 to G3 costs 75 at equal temperament and 207 to 260
when the two notes are asked 12 cents either side. The practice now includes occasions with the
interval narrowed and widened by 24 cents, scored also on landing the pitch asked.

### A start must be heard before it is played

Starts were first practised only at the instrument's own pitch. Asked 12 cents either side by
ear, the brass could not get there within a note: a soloist's ear begins 0.2 s in and reaches
under 5 cents, so a trumpet's A4 asked 12 cents low still sat 18 cents off after half a second.
The reeds' ear is quick and landed within 1 to 3 cents. A brass soloist now sets the lips for
the asked tuning from the note's first moment (as a section's players already did), and every
start is practised on eight occasions, four of them with the note asked 12 cents high or low,
plain and re-tongued, scored also on landing the pitch asked. With the lips pre-set the trumpet,
trombone and tuba follow the full 12 cents; the horn breaks on a few notes (G2, A2, C#4, F4,
G4), which is what the practice is there to find a start for.

A side finding: with the ear off the brass sit sharp (trumpet, trombone and tuba about 5 cents
in the median, up to 10 to 15 on some notes).

### Bowed strings

The double bass's slurs were the worst measured anywhere: a median of 207 ms to settle, up to
220 ms on a wrong pitch at string crossings, level jumps of 12 dB. A pulse of bow pressure at
the change fixed the worst crossing outright (C#2 to E2: 222 ms of wrong pitch to none). Some
same-string slurs (E2 to F2) answer to no gesture: that is the note's own slow build-up.

### Which vocal colours carry (`tools/colours.py`)

A player can say a syllable into the instrument: the consonant is the tongue (d or t), the
vowel a mouth resonance held through the note. Measured on every third note:

- Clarinet: vowels carry everywhere (1 to 4 dB of change in the harmonics, up to 6 to 8 dB);
  u and o in the low and middle, e and i at the top.
- Alto sax: the low and middle registers, 1.5 to 2.5 dB; a vowel breaks some low and high notes.
- Trombone: the low register played softly; patchy elsewhere.
- Trumpet: barely (about 1 dB, low and loud). Oboe: almost nowhere.
- On the sax and clarinet u, o and a darken the tone 2 to 5 %, e and i brighten it 3 to 4 %.
- d against t is audible on most notes of every instrument: the t speaks 15 to 30 ms sooner.

Nothing in the music chooses syllables yet.

## 3. What building it taught us

- **The workers must be told where the binary is.** The tool copied the binary so another
  session's build could not swap it mid-practice, but its worker processes do not inherit a
  Python variable. They played the shared binary; a build took it away and four practices died.
  The path now goes through the environment.
- **Trial settings must not look like a new instrument.** Each attack trial's knobs changed the
  calibration cache's key, so every render calibrated the whole instrument again and the load
  reached 300. The calibration now ignores the practice's knobs and always plays plain starts.
- **Compare like with like.** The first two-speed practice flagged 758 of the trumpet's 2460
  slurs as needing their own gesture, because it compared a worst-of-three-occasions cost
  against a one-occasion typical cost. The typical cost is now the median on the same occasions.
- **Two tools writing one file lose data.** A trill test read an instrument's results, ran for
  two minutes and wrote its stale copy back over a finished practice. Results are now read
  again just before writing. One instrument at a time per file is still the rule.
- **A probe is not the search.** Tuned occasions worked in a hand probe and crashed in the
  search (a sort over keys mixing "no tuning" and numbers). New occasions need a quick run of
  the real search before a long one is started.
- **A slur test is only as good as its occasions.** Each addition (the note left short, the
  player's natural unsteadiness, the notes tuned off equal temperament) made slurs fail that
  had passed. The trumpet's first survey showed 4 failures of 378; with both speeds, three
  occasions and its own gestures the practice covers 2460 and leaves none.
- **Cost of a full round.** About 200,000 to 300,000 renders an instrument, 10 to 20 minutes on
  24 workers when the machine is otherwise free; a thorough round (every interval, every slur a
  little worse than typical) about 800,000.

## 4. State on 2026-11-25

- Slurs at two speeds: all 14 winds and the four bowed strings practised. The trombone and the
  baritone sax had a thorough round (every interval 1 to 12); only the baritone sax's includes
  the tuned occasions. Everything else was practised at equal temperament.
- Starts: all 13 brass and reeds, on eight occasions, four of them asked 12 cents high or low.
- Trills: rehearsed for the 13 brass and reeds.
- All of it is built in (`src/slur_data.h`). Nothing is committed.

Checking the built-in tables found three faults, all fixed:

- **A slur's own row was used at every dynamics.** A slur with its own gesture at forte only was
  played the forte's way at piano too (the trumpet's F3 to A#3 ended 47 cents flat). Its own row
  now counts only at its own dynamics; the general way stands in at the others.
- **A practised start leaked into the next slur.** A start's slower breath rise stayed on for
  the rest of the breath: the trumpet's soft E3, begun 4.8 times slower, slurred to B3 on that
  slow breath and landed 47 cents flat. The rise is now the start's alone.
- **Lips pre-set for a tuning break unpractised slurs.** A soft slur into the trumpet's G3 asked
  12 cents low, the lips pre-set, sagged to 68 cents flat. Only tongued notes are pre-set until
  the slurs are practised with it (`PJ_BRASS_PREHEAR_SLUR=1`).

The trumpet's slurs in the check, worst of five occasions (two of them tuned): 196 failing of
2460 before the fixes, 50 after; the horn's 551, then 137. At equal temperament alone: trumpet
21, horn 101, oboe 84, bassoon 296 (the practice had left 0, 81, 58 and 93; the bassoon's gap is
not explained yet). Starts in the check, before and after practice: trumpet 2 failing and 414 mean
cost to 0 and 170; trombone 8 and 505 to 0 and 237; horn 24 and 577 to 15 and 358.

## 5. Not done

- Releases (how a note ends) for brass and reeds.
- Brass slurs indexed by partial instead of by interval.
- Alternate fingerings for the slurs that still fail.
- The director slowing a trill written faster than the instrument can play it (the limits are
  measured; nothing uses them).
- Starts and trills for the bowed strings and the flutes.
- A second round of slurs with the tuned occasions for every instrument but the baritone sax
  (the check shows what it would fix: most of the failures left are on the tuned occasions).
- Why the bassoon's slurs fail three times as often in the check as in its practice.
- A score that weighs steadiness after arrival, if the ear asks for it.
- Syllables chosen in the music.

## 6. Running it

    python3 -I tools/slurs.py recalibrate INST     practice, starts, trills, rebuild, verify
    python3 -I tools/slurs.py audition INST -o F   what changed most, before and after, one WAV
    python3 -I tools/colours.py exercise INST      which syllables carry where
    python3 -I tools/colours.py line INST KEY      "da ta da te ti da" on one note

Switches for comparing by ear: `PJ_SLURS=0`, `PJ_SLURSPEED=0`, `PJ_ATTACKS=0`, `PJ_TRILLWAYS=0`.
