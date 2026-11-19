# Director: one musical reading of a piece for every part of `-play`

Status (2026-11-18): a first version is built (`src/director.{h,cc}`, glue in `src/midifile.cc`,
`-director on|off|auto`). The rest of this file is still the plan; "Built so far" below says what
of it exists and what does not.

## Built so far

- **Detection** (quantized share, flat velocities, few tempo events; per-channel "ridden" by CC 11,
  1, 2 or pressure) and the printed summary.
- **Style** from the rhythm only (motor rhythm → Baroque, long melodic notes → Romantic, else
  Classical). No harmony analysis, no cadence strengths, no ritornello detection.
- **Phrases** from general rests and the melody's long notes, split at eight bars. Melody = the
  channel with the highest mean pitch, bass = the lowest. One meter (the first time signature).
- **Dynamics:** terraces by texture and echo (Baroque) or hairpins to each phrase's climax;
  melody slightly over the others; swells on long notes. Sent as a level curve per channel,
  applied each block through `Performer::setBreath`; for velocity-only instruments folded into the
  key velocity.
- **Accents** (metric weight, syncopation, after a rest, climax), **articulation** (Baroque
  detaching, a breath at phrase ends, by moving the note-offs), **vibrato** depth, delay and rate.
- **Timing:** the tempo map is rewritten (final ritardando, last chord held, fermata = a long
  chord everyone begins together followed by a general rest, a breath before the piece's climax)
  and every event's time is recomputed from its tick.
- **Mutes** (not in the original plan): on for the thin or echoing phrases of a slow piece. The
  strings get it in the cue (a mass on the bridge, `Bass::setMute`); trumpet channels get program
  changes to the muted trumpet.
- **The cue:** `struct Cue`, 12 numbers per note (`director.h`), handed to the part's players
  just before the key through `Ensemble::direct` → `Performer::direct`. The base player turns it
  into attack hardness and vibrato; the bow player also scales its own vibrato, its delay, and
  sets the mute.
- **Organs:** `organ-plan` and Hammond parts take only the shared time; their own plans still do
  their articulation and balance. The migration below is not started.

Not built: the meter map, anacrusis, harmony and cadences, sections, appoggiaturas, sforzando
from file velocities, the fermata written as a tempo drop, agogic catch-up, phrase shaping,
drums following the level, tests in `tools/test.py`, the README section. Checked by hand on
`~/midi/bwv530-1.mid` (summary only) and on a synthetic three-part piece (fermata found, 56.3 s
→ 60.0 s, climaxes 5–7 dB over the undirected render).

## The problem

`-play` renders a MIDI file through physical models. A file that a human played carries its own
performance in note timing, velocities and controller rides (CC11, CC1, CC64). Many classical
files, however, are typed into a notation program or a sequencer. They have these traits:

- every onset exactly on the grid;
- one or two velocity values (often 64, 80 or 100);
- no CC11 or CC1;
- a single tempo or a handful of tempo changes;
- fermatas written as plain long notes, or as a sudden drop in the tempo map.

Played literally, such a file sounds mechanical. Each instrument makes some of its own decisions
today, but each one guesses alone:

- `src/organplan.cc` finds the style (trio, cantus firmus, meditative, grand chœur, pleno), the
  strong beats and the phrase breaths. The articulation block in `src/midifile.cc` (around lines
  670–900) also handles the Baroque *toucher ordinaire* (the normal, slightly detached touch), the
  silence before a strong beat, live trills and the pedal played ahead.
- `src/hammondplan.cc` finds the style, climaxes, pauses, the final chord, the expression-pedal
  shape (`swell`), the Leslie arc and the glissandos.
- `noteLead()` (`src/instrument.h:216`, applied in `src/midifile.cc` around line 464) plays slow
  instruments ahead of the beat.
- The wind players' `Vibrato` (`src/performer.h`) gives every player its own rate and phase. Depth
  and onset are fixed per instrument; nothing makes them depend on style or phrase.
- No player does crescendos, diminuendos, emphasis or fermatas.

In a real orchestra the conductor (with the concertmaster and the continuo player) decides these
things once, and every section follows. The **Director** is that role. It analyses the whole song
once, before any part plays, and gives every part the same reading:

- the beat;
- the phrases;
- the dynamics arc;
- the accents;
- the vibrato style;
- the ritardandos and fermatas.

## When it acts

It acts only on a file that has no performance of its own. Detection, per song (with the knob
values to tune):

- **Quantized:** at least 90 % of note-ons fall within 3 ms (or 2 % of a beat) of a grid of
  1/16, 1/12 (triplets) or 1/32 in ticks. Use ticks, not seconds, so a tempo map doesn't fool it.
- **Flat touch:** at most 3 distinct velocities per channel cover at least 90 % of its notes, or
  the spread of velocities in each channel is small.
- **Few tempo events:** fewer than ~1 per 4 bars, not counting a final ritardando or a fermata.
- **No rides:** no CC11, CC1 or CC2 rides on a channel. Decide this **per channel**: a channel with
  its own CC11 or CC1 keeps it, and the Director leaves that channel's dynamics and vibrato alone.
  It still gets the shared timing.

Turn it on by hand with `-director on|off|auto` (default `auto`), and set its strength with
`PJ_DIRECTOR` (0..2, default 1, scaling every deviation it makes). Print one line saying whether it
took over and why, in the style of the organ and Hammond plan summaries.

## What it decides, once per piece

The Director works on ticks. It converts to seconds through the tempo map, which it may warp
itself (see Timing). Its output is a `Direction`: data on a timeline plus a per-note table, with
no audio code.

### 1. Meter and beat weights

- Track every time signature event, not just the first. `readSong` keeps only the first (`Song::barTicks`,
  `src/midifile.cc:120`). Add a meter map.
- Give each beat a metric weight: the downbeat strongest, then the secondary strong beat (beat 3
  in 4/4, beat 4 in 6/8), then the weak beats, then the off-beat subdivisions.
- An anacrusis: if the first bar is incomplete, or the material clearly starts on an upbeat,
  shift the bar lines.

### 2. Phrases, cadences, sections

- **Phrase ends:**
  - all parts rest together, or at least the melody does;
  - a long note after short ones;
  - the bass falls a fifth or rises a fourth onto the tonic or the dominant, on a strong beat.
- **Cadences, by strength:** perfect authentic > half cadence > imperfect > deceptive. The tonic
  is `Song::tonic` (`SongClock::get().tonic`). Harmony per beat from the pitch classes sounding
  (a simple template match is enough).
- **Sections:** repetition (ritornello) detection from the melody's interval sequence; changes of
  texture (tutti vs. solo, density, register); key changes.
- **Climax per section:** the highest melody note, combined with the densest texture and the
  most parts sounding. Prefer one climax per section, at about two-thirds of its length when
  ambiguous.

### 3. Style (era)

This sets the defaults for everything below. Reuse and generalise the organ's and the Hammond's
detection:

- **Baroque:** a motor rhythm (an even sixteenth or eighth pulse throughout), a continuo-like
  bass line, sequences, terraced texture changes, few long notes. The Bach files in `~/midi`
  are the reference: the Hammond plan currently calls them "jazz".
- **Classical:** an Alberti or repeated-note accompaniment, periodic 4- and 8-bar phrases, clear
  cadences.
- **Romantic:** wide range, long melodic notes, chromatic harmony, tempo changes in the file.
- **Unknown:** the mildest defaults.

### 4. Dynamics

There are two layers, both as a level per part on the timeline (0 pp .. 1 ff, centred on the
level the file's velocity suggests).

- **The arc (global):**
  - *Baroque:* terraced. The tutti ritornello is forte. Episodes and solo passages are piano. A
    repeated phrase is an echo, softer the second time. No long hairpins, except a small swell
    into a cadence.
  - *Classical and Romantic:* hairpins. Crescendo over the bars leading to each climax and
    diminuendo after it. A rising sequence grows; a falling one fades. A crescendo starts at a
    phrase start, never mid-note.
- **Within notes and phrases (per part):**
  - *Messa di voce* (a swell then a fade within one note) on long held notes: at least 1.5 beats
    and at least 0.8 s, in the winds and the strings. Small in the Baroque, larger in the
    Romantic.
  - The last note of a phrase diminishes, and a feminine ending (strong–weak) has its weak note
    softer.
  - An appoggiatura or a suspension: the dissonance on the strong beat is louder and longer, its
    resolution softer.
  - Melody over accompaniment: the melodic line ~3 dB above the inner parts, and the bass line
    steady. Detect the melody as the top, most active non-accompaniment line.
  - A held note that is accompaniment (a pedal point, a horn's long note under a melody) stays
    flat or decays gently. It does not swell.

### 5. Emphasis

Each note gets an accent value. Each instrument turns it into whatever it can do (see "How each
instrument carries it out").

- From the metric weight: small, about ±1 dB in the Baroque and less in the Classical.
- A syncopation (a note starting on a weak position, held across a strong one) is accented.
- A dissonance on a strong beat (appoggiatura) is accented.
- The first note after a rest, if it lands on a strong beat.
- A sforzando: a note whose velocity in the file jumps ≥ 20 above its neighbours, when the file
  does have some velocity variation.
- The climax note gets the largest accent of its section, plus an agogic stretch (see below).
- A repeated-note figure: the first of the group is stronger and the rest lighter.

### 6. Articulation

This is the common articulation that today lives only in the organ block of `src/midifile.cc`,
moved so that all instruments doubling a line agree.

- *Baroque (toucher ordinaire):* notes slightly detached, more before a strong beat. Quavers in a
  motor rhythm are separated and semiquavers legato in steps. Leaps are detached, and slurred
  pairs (an appoggiatura and its resolution) are joined.
- *Classical and Romantic:* legato within phrases, with a breath (a gap of 60–150 ms, scaled by
  the tempo) at each phrase end.
- Every part gets the same breath at the same phrase end.
- Keep the per-instrument physical constraints after this: the organ's pedal speech rule (a note
  sounds at least speech + 0.1 s), and the repeated-key halving.

### 7. Vibrato

For the instruments that have it: winds through `Performer::setVibrato` and `setVibratoRate`,
strings through their bow-arm or left-hand hooks when they exist, and the organ's tremulant.

- *Baroque:* off on short notes. On long notes (≥ 1 beat) and on peak or cadential notes, a late
  (≥ 0.3 s), shallow vibrato used as an ornament. Never on fast passagework.
- *Classical:* light. On long and expressive notes, starting after ~0.2 s.
- *Romantic:* continuous on all notes ≥ ~0.25 s. The depth grows with the dynamic level and the
  note's length, and is widest at the climax.
- Each part's depth follows its dynamic curve: in a crescendo the vibrato widens a little; in a
  diminuendo it narrows, and dies out on the last fading note.
- Keep each player's individual rate and phase. `Vibrato::seed` already makes a section not pulse
  together; the Director scales only the depth and the onset delay, and nudges the rate slightly.
- Organ: the tremulant only in a slow, quiet, meditative movement. The organ plan already knows
  "meditative" and the céleste; tie the tremulant to it.
- Hammond: the vibrato tabs are its own (`src/hammondplan.cc`); the Director only informs the
  counterpoint vs. ballad decision.

### 8. Timing: ritardando, fermata, agogics

All timing changes are made by **warping the song's tempo map once** (or by moving whole events
between note-on and note-off times consistently for every part). No individual part changes its
own time, so the ensemble stays together. This also keeps `SongClock` (`src/songclock.h`) correct
for the drummer and the others who read it.

- **The final ritardando:** over the last 1–3 bars (scaled by the tempo), slowing ~10–25 % in a
  smooth curve, slowest into the last chord.
- **A fermata:**
  - *Detected:*
    - the final chord;
    - a chord that all parts begin together and hold longer than the surrounding pulse, followed
      by a general pause or a new phrase;
    - a sudden tempo drop in the file over one beat (a sequencer's way of writing a fermata).
  - *Performed:*
    - a short ritardando over the 1–2 beats before;
    - the chord held ~1.5–2× its written length (Baroque ~1.5, Romantic up to 2.5);
    - all parts released together;
    - a breath (~0.3–0.6 s) before the next phrase.
  - If the file already slows there, scale its slowing rather than adding to it.
- **Agogic accent:** at the climax note and at the strongest cadences, stretch the beat ~5–10 %
  (a "breath before the downbeat"), then catch up in the next beat or two so the average tempo
  holds.
- **Phrase shaping (Romantic only, mild):** a slight push forward in the middle of a phrase, and
  relax at its end.
- **Not in the Director:** each instrument's own play-ahead (`noteLead`, the pedal pipes' speech
  lead). Those stay physical and per-instrument, applied after the Director's warp.

## Output: what the parts receive

`struct Direction` (in `src/director.h`):

- `meter`: bar lines and metric weight per beat.
- `tempoWarp`: a new tempo map (replacing `Song::tempo`) for the ritardandos, the fermatas and the
  agogics.
- `phrases`: (start, end, cadence strength) and `sections`: (start, end, climax tick, kind:
  tutti, solo, echo).
- `style`: Baroque, Classical, Romantic or unknown.
- per part (channel × track): a `level` curve (piecewise linear, ticks → 0..1), and a `vibrato`
  curve (depth multiplier, onset-delay multiplier).
- per note (channel, key, on-tick): `accent` (−1..+1), `lengthScale` or an absolute release tick,
  and `swell` (the messa di voce depth, and where its peak falls).
- `fermatas`: (tick, hold factor) for printing and testing.
- `summary`: a short human-readable text, for example:

```
director: Baroque, terraced dynamics; 4/4, 64 bars; 11 phrases, 3 sections (ritornello, episode, ritornello)
  bars 1-16 forte (tutti), bars 17-40 piano (episode; echo at 25), bars 41-64 forte
  climax bar 52; fermata bar 40 (x1.6, rit. over 2 beats); final rit. bars 63-64 (-18 %)
  vibrato: ornamental, long notes only; articulation: toucher ordinaire
```

## How each instrument carries it out

The Director decides the *what*; each part's player decides the *how*, according to its physics.

| Instrument | Dynamics level | Accent | Vibrato | Articulation |
|---|---|---|---|---|
| Winds: reeds, flutes, horns (`Performer`) | `setDynamics` continuously, within notes too | `setHardness` and a small breath step at the attack | `setVibrato` and `setVibratoRate` | note lengths |
| Bowed strings | bow pressure and speed (needs a hook) | bow weight at the start | the left hand (needs a hook) | lengths, bow changes at the breaths |
| Piano, harpsichord, plucked | velocity per note | velocity; on the harpsichord, which has no dynamics, timing and length | — | lengths |
| Church organ (`organ-plan`) | registration terraces per section (already planned there); the swell box only on an enclosed division | the gap before the note and the note's length (already there) | the tremulant (meditative only) | the existing articulation block, reading the Director's phrases |
| Hammond | the expression-pedal shape (`HammondPlan::swell`) | velocity and the percussion's detaches | its own tabs | its own |
| Drums | velocity follows the level | the metric weight | — | — |

The wind path is the cheapest and most audible first target: today `-play` drives
`setBreath` only from CC11 and CC2 (`src/midifile.cc` around lines 1151–1175).

## Where it sits in `-play`

In `playMidiFile` (`src/midifile.cc`):

1. `readSong`, transpose, the velocity normalisation for a file played hot.
2. **New: `Direction dir = direct(song, partsInfo)`.** It runs on the raw song, before the leads
   and the plans. Apply `dir.tempoWarp` to the events' `t` (and to `Song::tempo`) here.
3. `noteLead` and the other per-instrument leads (unchanged).
4. The organ plan and the Hammond plan, now given `dir` to read the phrases, sections, climax,
   style and final chord instead of guessing their own.
5. During the render: at each block, for every part, look up its level and vibrato curves at
   `now` and set them on the players (the same clock loop that applies the Hammond's
   `swell`, around line 980). Per-note accent, length and swell are applied at the note-on and
   the note-off.

Live play (`src/live.cc`) does not use the Director.

## Migration of existing logic

After the Director exists and is tested, in this order:

1. The organ articulation block: its strong beats, phrase-end breaths and Baroque detach read
   `dir.meter` and `dir.phrases`. The physical rules (pedal speech, repeated keys, trills) stay.
2. `planOrgan`: its style detection is fed or replaced by `dir.style` and `dir.sections`.
3. `planHammond`: climaxes, the final chord (`endAt`), the pauses, and the counterpoint vs. ballad
   decision from `dir`. The Hammond-idiomatic choices (Leslie, percussion, glissandos, drawbars)
   stay in its own plan.

Each step must keep the existing tests in `tools/test.py` passing, or update them deliberately.

## Build order

1. **Analysis only.** `src/director.{h,cc}`: detection (does it act), the meter map, phrases,
   cadences, sections, climaxes, style, fermatas, and the summary. Print it under `-play`. No
   audible change yet. Tests on synthetic MIDI files made in `tools/test.py` (the Hammond tests
   show how to write small files):
   - a quantized 4/4 piece with a clear 8-bar phrase structure and a final cadence;
   - a fermata written as a long chord + general pause, and one written as a tempo drop;
   - a Baroque motor-rhythm trio (BWV 530-like) and a Romantic melody with long notes;
   - a humanly played file (jittered onsets, varied velocities, CC11): the Director must stay out.
2. **Timing.** The tempo warp: the final rit., the fermatas, the agogic accents. Test: the
   rendered duration and the onset times of the fermata chord and of the note after it; all parts
   released together.
3. **Dynamics and accents for the winds and for velocity-only instruments.** Test: an RMS
   envelope rises over a crescendo region and falls at phrase ends; a messa di voce shows a peak
   inside a long note; the terraced levels differ between tutti and episode.
4. **Vibrato.** Test: the depth is near zero on short Baroque notes and present on a long
   Romantic one; it starts later in the Baroque.
5. **Strings.** Hooks for bow pressure, speed and vibrato in the bowed models. **Coordinate with
   whoever owns the bowed-string and cello work** (`src/bass*`, `src/cellobody_data.h`,
   `research/cello.md`; uncommitted work exists there). Don't edit those files without that
   owner.
6. **The migration** of the organ and Hammond logic (above).

## Knobs (environment, in the existing `PJ_` style)

- `PJ_DIRECTOR` (1): overall strength; 0 disables every deviation but keeps the summary.
- `PJ_DIRECTOR_DYN` (1), `PJ_DIRECTOR_ACCENT` (1), `PJ_DIRECTOR_VIB` (1), `PJ_DIRECTOR_RIT` (1),
  `PJ_DIRECTOR_FERMATA` (1.6, the hold factor): each feature's own scale.
- `PJ_DIRECTOR_QUANT` (0.9): the quantized share needed to act.

## Principles and constraints

- **Deterministic.** Like the Hammond plan: any randomness comes from hashing the note's (tick,
  key, channel), never from a clock or a global RNG. The same file renders the same way.
- **Subtle by default.** A good conductor's reading is felt rather than noticed. The default
  deviations should sound natural at strength 1, and caricatured only at 2.
- **The file wins where it has information.** Its own CC11, CC1, velocities, tempo changes and
  fermatas are scaled or kept, never overwritten.
- **One timeline for everyone.** Timing is changed only through the shared tempo warp.
- **Physics stays in the players.** The Director never knows about pipes, reeds or bows. It
  gives musical intentions; each player realises them within what its instrument can do.
- **Explainable.** Every decision appears in the printed summary, so a listener can check it
  against the score.
- **Docs.** Each step updates `README.md` (the `-play` section) as the organ and Hammond work did.
- **Repository rules.** Commits are made in the user's name only, and only when asked. Do not
  touch another session's in-progress files: `src/bore.*`, `src/horn.*`, `src/hornplayer.cc`,
  `tools/brass.py`, and the bowed-string and cello work above.

## Open questions for refinement

- **Harmony analysis:** how far to go. A pitch-class template per beat is probably enough for
  cadences and appoggiaturas; a real Roman-numeral analysis is likely overkill.
- **Melody detection** in files where all the strings are on one channel and one track (common
  for "Strings" GM programs): split by register or by voice leading?
- **Ritornello detection** robustness for concerto movements: test pieces are needed. Which
  files in `~/midi` are concertos, trio sonatas, chorales?
- **Should the Director also choose the instrumentation balance?** For example, the continuo:
  harpsichord vs. organ. Probably not at first.
- **Dynamics scale:** how should the level curve map onto each instrument's own dynamic range, so
  that "forte" is the same intention in the oboe and in the strings? Probably through the
  existing velocity-gain calibration (`src/velgain_data.h`, `src/levels_data.h`).
- **Fermata as a tempo drop:** distinguish a fermata from a real tempo change (a new movement, a
  *meno mosso*). Return to the original tempo after it, or stay at the new tempo?
