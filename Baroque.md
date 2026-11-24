# Baroque: how the players could speak the music of 1600–1750

Ideas for the director (`Director.md`, `src/director.cc`) and the players, built and still to build.
The rule over all of them: the music is **rhetoric**, a sentence spoken to someone, not a pattern
played evenly. Quantz (*Versuch*, 1752), Hotteterre (*Principes*, 1707), C. P. E. Bach (*Versuch*,
1753), Leopold Mozart (*Violinschule*, 1756), Muffat (*Florilegium secundum*, 1698) and Mattheson
(*Der vollkommene Capellmeister*, 1739) are the sources. The ear decides the sizes.

## Built

- **Good and bad notes:** the note on the beat a little louder and tongued harder, the ones
  between softer and lighter (the winds' speech, `PJ_DIRECTOR_SPEECH`).
- **Each note relaxes once spoken**; long notes are a *messa di voce*: begun softly, swelled, faded.
- **The sigh** (*Seufzer*): a stressed note falling by a step onto a weaker one is leaned on and
  slurred; its resolution is softer and lifted.
- **Breaths** where a flutist would take them, after about 5 s of running line.
- **Terraced dynamics and echoes**, Baroque detaching of short notes, and vibrato only as an
  ornament on long notes.
- **The winds' own reading** when the file has its own timing: the phrase arc and the piece's
  drama, measured by how busy it is (`PJ_WIND_ARC`, `PJ_WIND_DRAMA`).
- **Flutes** (`PJ_WIND_LEAP`, `PJ_FLUTE_TAKA`, `PJ_FLUTE_REPEAT`):
  - **High notes:** the highest note is not blown the most. Notes above the part's middle note
    are held back; an accompanying part keeps only a third of the arc.
  - **Leaps up:** tongued softly ("dah") and grown into.
  - **Trills:** tongued ta-ka.
  - **Three repeated notes opening an accompanying passage:** shaped by where the beat falls.
    - Only the first on the beat: DA-ra-da.
    - Only the third on the beat: ri-ti-DA.
    - Only the second on the beat: da-DA-da.
    - None on a beat: ti-RI-ti.
- **Dissonance and resolution** (`PJ_DIRECTOR_HARMONY`): a note struck on a beat that clashes
  with the other parts and resolves down by a step is leaned on, the sharper the clash the more:
  a minor second or major seventh most, then a tritone, a major second or minor seventh, and a
  fourth over the bass. Its resolution is softer. All the parts' accents; the speaking winds'
  level too.
- **Compound metre** found from where the notes begin (6/8, 9/8, 12/8: the dotted beats weighted).
- **Dances** (`PJ_DIRECTOR_DANCE`, Baroque):
  - sarabande: the second beat leaned on;
  - minuet: felt in one;
  - gavotte and bourrée: the half bar strong;
  - gigue: the lilt of each dotted beat's first note.
- **Trills played alive** (`PJ_TRILL_QUICKEN`, 0.2): a wind's written-out trill begins slower and
  quickens; begun on the upper note, that note is leaned on.
- ***Notes inégales*** (`-style french`, or `PJ_INEGALES=1`; `PJ_INEGALES_RATIO` 0.6): running
  eighths by step, long-short, in every part.
- **Style forced:** `-style baroque` or `-style french`.
- **A one-keyed Baroque flute** (`-inst baroque-flute`). A file's flutes are put on it by
  themselves when the director reads the piece as Baroque, or when the file name carries a
  Baroque catalogue number (BWV, HWV, TWV, BuxWV, RV), and only when every flute note is in its
  compass. `PJ_BAROQUE_FLUTE=0` never does this, and `=2` does it wherever the notes fit.
  `-reinstrument flute=...` decides instead. The instrument itself: boxwood, a cylindrical head and a contracting conical
  body, six small holes and a D# key, D4–A6, at the ensemble's pitch (A = 440). Its fork-fingered
  notes (F, G#, Bb, C and their octaves) come out about 3 dB softer than their neighbours from
  the tube itself; the whole flute is 2–7 dB softer and darker than the Boehm flute.

## Ideas

### Speech and articulation

- ***Notes inégales*** (French music; Hotteterre, Couperin): pairs of the shortest running notes
  played long-short, about 55:45 to 2:1. Only by step, not in leaps, not in Italian music. It
  needs a guess at "French" (an overture's dotted rhythms, dance names in the file's track names).
- **Over-dotting** in a French overture: the dot held longer and the short note snapped late, all
  parts together.
- **Slur patterns from the figure:** two notes slurred and two tongued in running sixteenths; a
  slur over a step and a tongue on a leap.
- **The feminine ending:** the cadence's last note soft and short after a leaned appoggiatura,
  never punched.
- **Repeated notes in general** (not only flutes, not only at a start): the strings with light
  separate strokes, the organ with each repeat clearly lifted. None of them alike.
- **The bow's rule** (Muffat, Leopold Mozart): down-bow on the bar's first note, so strong beats
  get the heavier stroke. Two up-bows in a row at a triple-metre bar's end (the *craquer*).
- **Rests speak:** the note before a rest is released, not cut. A rest after a cadence gets a
  moment more.

### Harmony as dynamics (needs a chord reading)

- **Dissonance louder, resolution softer** (Quantz XVII.vi): a weight per harmony, from consonance
  to the diminished seventh, as "louder" and "more vibrato". The single biggest missing idea.
- **The cadence's weight:** a perfect cadence, a half cadence and a deceptive cadence each
  breathe differently; the deceptive one is a surprise, held a moment.
- **Hemiola** before cadences in triple metre: two bars of 3/4 felt as one bar of 3/2, and the
  stress moves with it.
- **Sequences:** each step of a sequence a little louder going up, softer going down, and never
  quite the same.
- **The bass speaks the harmony:** a harmony change in the bass gets a little weight; repeated
  bass notes under one harmony get less.

### Rhetorical figures (Mattheson, Burmeister)

- ***Exclamatio***: a leap up of a minor sixth (or any wide leap after a rest), the one place
  where a leap up *is* leaned on, unlike the flute's eased leap.
- ***Passus duriusculus***: a chromatic line falling by half steps, played legato and darkening.
- ***Suspiratio***: a rest breaking the line in mid-phrase, the voice taken away.
- ***Anabasis* and *catabasis***: a line rising grows; a line falling settles.
- **Pause and *abruptio***: a sudden general rest, held a little longer than written.

### Dances (the metre's own weights)

The dance gives the bar its weights; the director's single bar weighting could take one of these:
- **Sarabande:** the second beat leaned on, often longer.
- **Minuet:** in one, the bar's first beat light-footed, with hemiolas before the cadences.
- **Gigue:** lilting, the first of each triplet group.
- **Bourrée and gavotte:** starting on an upbeat; the half-bar pulse.
- **Courante:** shifting between 3/2 and 6/4.
- **Allemande:** a running upbeat, calm.

The dance could be detected from the metre, the tempo, the upbeat and the rhythm patterns, or
named in the file.

### Ornaments

- **Trills from the upper note, on the beat**, starting slow and quickening, and ending with a
  turn or a stop before the resolution. Cadential trills could be added where the file has none
  (`PJ_ADD_TRILLS`, off by default).
- **The long appoggiatura** taking half the main note (two-thirds before a dotted note), leaned
  on and resolved softly.
- **Mordents** on the strong notes of a bass line, and *port de voix* on rising steps.
- **A repeat's second time ornamented** or played as an echo.

### Time

- **Steady bass, free melody:** the melody moves slightly ahead or behind (*tempo rubato* in its
  old sense) while the bass keeps the time.
- **The final ritardando is short:** the last two beats, not the last two bars.
- **The *tactus***: the pulse a slow quarter or half note, with tempo choices from the time
  signature (C, ¢, 3/2, 3/8) as the period read them.

### The continuo and the ensemble

- **The harpsichord spreads its chords** (*arpeggiando*), longer chords slower, from the bass up.
  A dissonant chord gets an *acciaccatura*.
- **The organ continuo** lifts its chords before the bass's next note, and is lighter under a
  solo.
- **Balance:** treble and bass polarity. The outer voices lead and the inner parts are lighter,
  especially the second violins and violas in full textures.
- **Concertino against ripieno:** the solo group a step forward, the tutti a terrace louder.

### Sound

- **Pitch and temperament:** A = 415 and a well temperament per piece, as an option. The tuning
  per instrument exists (`-tuning`).
- **Gut strings:** less sustain and a faster bow decay than modern strings. A Baroque bow's
  natural *messa di voce* on every long note.
- **Natural horns and trumpets:** stopped notes darker; the harmonics' own tuning (the 11th and
  13th) left in where the player cannot lip them.
- **The Baroque oboe and flute:** fork-fingered notes veiled and a little softer (the
  cross-fingering's colour), from the fingering tables.

## Priorities

1. Dissonance and resolution (needs a light chord reading per beat).
2. Dance weights from the metre.
3. *Notes inégales* and over-dotting, when the piece is French.
4. The harpsichord's spread chords.
5. Trills from above, on the beat, quickening.
