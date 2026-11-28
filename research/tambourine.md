# The tambourine as a player: gestures, spectrum, listening to the band, following the director

Research of 2026-11-27. The instrument itself (jingles as rattlers, the frame's knock) was built on
2026-11-16 (`src/shaker.cc`, `research/shakers.md` §3.4, §7). This file is about everything around
it: how a tambourinist plays, what a stroke does to the spectrum, how they fit into a band and how
they follow the arrangement and a director. Tags: **[web]** a source says it (§10), **[meas]** measured
here, **[calc]**, **[design]** proposals for this project, still to be heard.

## 1. Short answer

- **Today the tambourine has one gesture:** a hit, whose hand speed is set by the velocity. There
  is no shake, no roll, no choke, no head, no knee, no pre-jingle, no beat or band awareness.
- **It is too dark** **[meas]**. Measured against two free recordings of real headless tambourines
  (§3.1), its centroid is 6.5 kHz against their 8.9–9.8 kHz. Its 2.8–3.6 kHz band holds −11 dB of
  the energy against their −30 to −35 dB. Its loud hits fade a little fast.
- **Players have six techniques chosen by tempo × dynamic, plus two kinds of roll** **[web, Stiers]**. Velocity
  chooses the hand and the place on the head, not just the gain.
- **The holding angle is the main control of ring length** **[web, VSL, Stiers, Instrumentalist]**.
  Held flat, the jingles lie on each other: short and dry. Held vertical, they hang free: longer
  ring and more head. Players change the angle per gesture: 45° for hits, upright for a roll, flat
  for a choke.
- **Every shaken note is two sounds** **[web, Kaufman]**: a small jingle as the wrist starts to
  move, then the main one on the beat, about 100 ms later.
- **Taste comes mostly from restraint and from choosing where to play** **[web]**:
  - one backbeat rather than both, picked where the vocal is quiet;
  - a ladder over the form: verse beat 4 → chorus 2 and 4 → 16ths → lay out in the bridge;
  - one shake into a chorus, at most once or twice in a song;
  - nothing on the hi-hat's own grid at equal weight;
  - softer than the snare, and pulled back under crashes and an active ride.
- **In the user's files key 54 is mostly the transcriber's guess** **[meas]**. About 1150 notes in
  68 files, every one at velocity 100 and off the beat grid. Some are plainly false: an organ
  Buxtehude has 149, most at the same moment as cabasa notes. So the player has to treat written
  notes as permission and a hint of density, not as a score (§7).

## 1.1 The principle: a note means "something should happen with the tambourine"

The owner, 2026-11-27: real players, as seen on YouTube, play with much variation and expression.
A key 54 is therefore not "play the tambourine sound". It means "something should happen with the
tambourine here". What happens is decided by the director and the musician together. The musician
listens to the beat, the other rhythm instruments and the rest of the band, and decides what fits.

**Who says what** **[design]**:

| Whose call | What they decide | What they read |
|---|---|---|
| **Director** (once per piece, `src/director.cc`) | **Intent:** how present the tambourine should be in this section (tacet … background … featured), the level arc, where the climaxes and lead-ins are, the style (orchestral or song), and articulation (detached or legato) | the form, the phrases, the level curve, style detection |
| **Musician** (a plan before the song, then per note) | **Means:** which gesture (§2), its strength, down or up, tilt, place on the head, lift, timing, whether to keep, skip, merge into a roll, or add a lead-in | the beat and its swing, the hi-hat/ride grid, the snare's backbeat, the kick and bass, crashes, the vocal's density, how long the band's notes are, the written tambourine notes and their density |
| **The instrument** (`Shaker`) | **Physics:** the result of that gesture, never the same twice | the state left by the last stroke |

- **The written note supplies three things:** where (the time), roughly how much (the velocity,
  where it means anything) and the density around it. Its velocity is a hint, not the gain.
- **The musician's answer to each note is one of:**
  - a struck stroke (finger / fingers / fist / palm / knee / the other hand's heel), on the rim or
    nearer the centre;
  - a shake (down or up), with its lift;
  - part of a roll (shake or thumb), with an attack and an ending;
  - a choke;
  - nothing (the note is skipped);
  - or an added gesture the file did not write: a lead-in shake, a ringing last note.
- **Variation comes from three layers,** none of them random noise on a fixed sound:
  - the gesture chosen from the context;
  - the hand's own inexactness: tilt, place and speed scatter a little around the intent;
  - the physics: the jingles' state, the order in which they collide.
- **A fixed pattern is a failure.** Two neighbouring notes may sound alike only when the music
  asks for it, as in a steady 16th groove. Even there the down and up strokes differ, and accents
  follow the band's.

## 1.2 What the written note is, and what the player chooses **[design]**

The owner, 2026-11-27: the note is a hint that such a tambourine hit could appear at about that time
and about that velocity. Where the velocities carry intention, the player takes it into account. It
still decides which variation it plays, how loud, and with what emphasis.

### 1.2.1 The hint

| Hint | How the player reads it |
|---|---|
| **time** | "about here". The player places the gesture on its own feel within a window: about ±⅛ beat when the file is a transcription; a few ms (push or lay back) when the file is on the grid. |
| **velocity** | Read first as information. In a flat file (one or two values) it says nothing. In a varied file (does it vary with the meter, with the phrases, or against the other drums?) it is an intent of roughly this much. The player's strength lands in a range around it, set by the context. |
| **density** | The neighbouring notes say "groove", "a single accent" or "a roll". |

### 1.2.2 The choices that make one gesture

Each stroke is a point in this space; no axis is a fixed sample.

1. **What makes the jingles move:**
   - **(a) A hit against something.** The other hand's palm or heel (the Motown backbeat), the thigh
     or knee, the hip. The frame *stops* at the impact. That gives one hard, clean throw of every
     pair and a short tail; the thigh or knee also adds a dull pop.
   - **(b) A change of direction in the air.** A linear shake, reversing: the pairs fly on and slam
     the far side of their slots. It is softer, a little spread out in time, with the lift before
     it.
   - **(c) A quick rotation of the wrist** (a twist about the forearm). The pairs are thrown
     tangentially, the far ones (larger radius) hardest. The result is a fast "ksh" with a short
     sweep across the pairs, in the order around the ring.
   - **(d) A stroke on the head (membrane):**
     - **Striker:** a flat palm, the heel of the hand, a fist or knuckles, bunched fingers, 1–3
       fingertips, the thumb.
     - **Where on the head:** at the rim over the jingles, partway in, or at the centre.
     - **What it does to the sound:** a light striker at the rim gives a precise, quiet sound from
       the nearest pairs only. Heavier strikers nearer the centre add the head's pop and shake every
       pair evenly. A headless tambourine has the wooden rim or frame instead: a knock.
   - **(e) Friction:** a thumb or finger rolled along the head.
2. **Where it is held, and so its projection:**
   - **High, in the air:** the arm raised and the instrument turned to the listener. It is clear of
     the body, so it is free to ring, and it is heard brighter and louder.
   - **At the chest:** the normal position.
   - **Low, against the thigh:** the body shadows it and damps it. It is darker by a few dB above
     5 kHz and the tail is shorter. Leaning on the leg is also what fast soft playing uses.
   - **For the model:** a level, a gentle high shelf, and how long the frame's motion goes on after
     the stroke (free, or stopped by the leg).
3. **Tilt:** flat … 45° … upright (the ring's length, §3.3).
4. **Strength:** the speed of the hand or frame.
5. **Emphasis, apart from strength:**
   - the lift's size (a big preparation versus none);
   - an earlier, harder attack, heard as on top;
   - the choice of (a) over (b), which gives a stopped, punchy hit.
6. **Follow-through:** let it ring, hold it still, or choke it (flat, or the hand on the head).

### 1.2.3 The musician's choice from the context (examples)

| Situation (from listening and the director) | A likely answer |
|---|---|
| backbeat in a soul chorus, band loud | (a) against the other hand's heel, held high, 45°, strong, a small lift, let ring |
| the 16ths in between, same chorus | (b) down/up reversals, at the chest, upright, the up strokes ~3 dB under the down strokes |
| a quiet verse, the director's presence low | (d) two fingertips at the rim, low, flat, soft; or skipped |
| a single written accent before a crash | (c) a wrist twist into it, then skip the note under the crash |
| a fast loud passage, orchestral | (a) fist and knee alternating, low |
| the last chord, a tight band | (d) the palm at the centre, then choked |
| a fermata, the band sustaining | a shake roll from upright, ending on an attack at 45° |
| the vocal busy around beat 4 | play beat 2 only; at 4 a (b) reversal at most, quiet |

- **Not two the same.** Within a groove the choices stay put; the hand's small scatter in tilt,
  place and speed, and the jingles' state, keep each stroke different. Between sections or
  phrases the choices move with the director's intent.

## 2. The gesture vocabulary

### 2.1 Struck strokes, by dynamic **[web, Stiers, VSL, Instrumentalist, William James]**

| Dynamic | Hand | Where | Sound |
|---|---|---|---|
| pp–mp | 1, then 2–3 fingertips; the heel of the hand rests on the head's centre | the rim, at the top of the instrument | jingles only, the nearest pairs most: "precise" |
| mf–f | three fingers bunched, falling into the head | the centre | jingles + the head's pop |
| f–ff | loose fist, the flat of the knuckles ("knocking on a door") | the centre | punch, the head louder |
| accents only | flat open hand | the centre | "a little goes a long way" |

- **Loudness comes from fingers, then wrist, then arm** **[web, William James]**.
- **The holding hand damps the head**, keeping that damping the same at every dynamic **[web]**:
  thumb on top, two fingers under it, or the forearm on the head.
- **Single strokes** **[web, VSL]**: the middle finger braced by the thumb, 3–4 cm from the edge.

### 2.2 Fast strokes

- **Fast and loud: fist and knee** **[web, Stiers]**. A foot on a chair, the tambourine 8–15 cm above the
  knee. The fist plays the downbeats; on the upbeats the tambourine is brought down onto the knee.
  The motion comes from the elbow. Sources disagree on which beats go to the knee **[web, Band
  Directors Talk Shop]**.
- **The knee adds a big, dull "pop"** that players try to match to the hand's sound **[web, William James]**.
- **Fast and soft:** the instrument lies on the thigh or on a padded table. The heels of both hands
  pin it, and the fingertips of both hands play the rim over the jingles.
- **Pop and worship playing:** the head is tapped against the hip or thigh, which gives cleaner
  jingles **[web, x8drums, Starline]**.
- **Motown (Jack Ashford):** the tambourine in the right hand, struck against the heel of the left
  **[web, Hudson]**. This is a *stopped* hit: the frame meets the hand and stops, so the hit is loud
  and short. Kaufman's samples are the same two kinds: a "free" jingle and a "louder, stopped"
  one **[web]**.

### 2.3 Shaking

- **Pop 16ths** **[web, Henzie-Skogen]**:
  - The downbeats are played going down and the upbeats coming back up.
  - The arm keeps moving through rests; a note is left out by making the motion smaller, not by
    stopping.
  - The backbeat is the heel of the free hand, at the bottom of a down stroke.
  - Almost every phrase begins or ends on an "and".
- **A rhythmic shake is distinct from a roll** **[web, Stiers]**. A shake back and forth from the
  elbow sounds rhythmic ("shooka-shooka"). A roll turns the wrist and shakes at once (three
  dimensions) so that it sounds as a sustained sound, not a rhythm.
- **The two slot sizes on a concert tambourine help** **[web, Stiers]**: their pairs fall out of
  step, which smooths the roll.
- **How a shake roll starts and ends** **[web, Stiers, Instrumentalist]**:
  - It starts with an attack: the instrument pushed upright.
  - It ends with an attack and a return to 45°, so the jingles stop.
  - It plays from about mp upwards, for as long as needed.
  - The beginning and end are not accented unless the music asks for it.
- **At faster tempos the shake gets smaller** **[web, riq: bonedo]**.

### 2.4 Friction rolls

- **Thumb and finger rolls** **[web, Stiers, VSL, Instrumentalist]**: the moistened or waxed thumb
  (or middle finger) runs about 1 cm in from the rim "in small jerks" (stick-slip). Each slip
  kicks the head, and the head shakes the jingles.
- **Range:** the finger roll is the most delicate (pp–mp). The thumb roll is a little louder and is
  the one for crescendos.
- **Length:** about 3 s at forte and about 8 s at piano, because the way around the rim runs out.
  A figure-8 path makes it endless.
- **Releases:** a soft one drops the thumb onto the head; an accented one drops the heel of the hand.
- **Practice in the repertoire** **[web, Ito]**: rolls of an 8th or a quarter are thumb rolls, longer
  ones are shake rolls (*Petrushka*). Tchaikovsky's Danse Arabe has 32nds "with the finger along the
  skin" **[web, Grover]**.

### 2.5 Stopping the sound

There are three ways **[web, VSL, Instrumentalist]**:
- tilt to flat: the jingles fall onto each other;
- the hand flat on the head (secco): the frame and the head are stopped;
- an ending stroke followed by a return to 45°.

### 2.6 The lift (the pre-jingle) **[web, Kaufman]**

- **The note is two sounds:** a small jingle when the wrist starts, and the large one when the
  motion completes, on the beat.
- **They are about 100 ms apart at any tempo.** Without the first, "it doesn't sound like a tambourine".
- **At fast tempos the lift merges** into the tail of the previous note.
- **[design]** A player that knows the song sends the gesture about 100 ms early (`noteLead()`, as the
  maracas' chest stroke already does) and the frame's acceleration starts then. Live play has no
  time before the note: there the lift is absent or very small.

### 2.7 Relatives worth borrowing from

- **Pandeiro** **[web, Acoustics Today 2007, Dende]**:
  - The basic stroke cycle is thumb–fingertips–heel–fingertips on 1-e-&-a, swung.
  - The holding hand adds its own shake exactly when the fingers strike: two sources of excitation
    per note.
  - A palm in the centre cuts off the thumb's low tone.
  - The jingles are cupped: drier and shorter.
- **Riq:** the holding thumb rests on one pair of jingles, damping it alone **[web]**.
- **Italian frame drums:** pizzica has triplet rolls of the hand with a thumb slap on each beat
  **[web, Turrisi]**.

## 3. Spectrum and envelope

### 3.1 Measured: the model against two recordings **[meas]**

- **The recordings:** a headless tambourine (Wikimedia, CC0, 35 events) and a half-moon (public
  domain, 38 events), both at 44.1 kHz.
- **Where the files are:** copied, with the analysis scripts, to
  `research/gm_compare/work/tambourine/`. The script is `cent.py`: onsets, centroid over 0–50 ms,
  the 2.8–3.6 kHz band, and the time to −30 dB.
- **Ours:** `-inst tambourine`, velocities 40/100/127.

| | ours | headless | half moon |
|---|---|---|---|
| centroid, first 50 ms (kHz) | 6.5 (6.3–6.9) | 8.9 (7.7–10.4) | 9.8 (8.3–10.6) |
| 2.8–3.6 kHz band, share of energy | −11 dB | −30 dB | −35 dB |
| −30 dB after the peak, hard hits (ms) | 109–114 | 130–210 | 100–140 (soft: 90–125) |

- **The research agent's band levels** (whole files):
  - 6–10 kHz: within 0–5 dB of the total.
  - 10–16 kHz: −2 to −6 dB.
  - 3–6 kHz: −16 dB.
  - below 1 kHz: −40 dB.
- **Peaks in the long-term spectrum:**
  - headless: 6.6, 8.0, 8.5 kHz, then 14.9 and 16.0 kHz;
  - half moon: 6.9–7.3, 9.8 and 11.2–12.4 kHz.
- **The tail is collisions, not free ring:** 25 or so distinct clicks in the first 100–150 ms
  (150–300 a second), and every band decays at the same rate.
- **Louder means brighter, and longer** (half moon): +17 dB of level gives about +1.0–1.3 kHz of
  centroid and 30–40 ms more before −30 dB.
- Microphones and distances are unknown, so these are indicative, not a fit.

### 3.2 What follows for the instrument **[design]**

- **Disc modes.** Our 5.6 and 8.1 kHz (±6 %) are Cook's PhISEM numbers **[web, STK]**.
  - The 3.2 kHz mode is ours, not Cook's (his weak shell resonance is at 2.3 kHz, gain 0.1).
    Lower it by 15–20 dB, or drop it.
  - Add a fourth mode at 11–15 kHz.
  - Then aim for a centroid of 8.5–10 kHz.
- **Brightness with velocity.** About +1 kHz per 15–20 dB, from the harder kick: more pairs leave
  their rest, the impacts are harder, and the contact is shorter, so it excites higher.
  - Today the contact low-pass is fixed (~40 µs).
  - Make it shorter at high impact speed (Hunt–Crossley: stiffer at a faster approach)
    **[web, Avanzini]**.
- **Ring with velocity.** A loud hit at about 150 ms to −30 dB, a soft one at about 100 ms.
- **The frame:** keep its modes about 40 dB under the jingles when there is no head.

### 3.3 What the player changes, and the model's parameter for each **[design]**

| Player's control | Physical effect | Parameter in `Shaker` |
|---|---|---|
| Tilt: flat … 45° … upright | gravity along the pins (`gEff_`); flat: the discs lie together, rubbing, preloaded | `gEff_`, an extra friction between the discs of a pair, `gapS` |
| Where it is struck: rim … centre | the rim kicks the near pairs most; the centre adds the head and kicks all evenly | the `rock` amplitude and phase (already exists, random); the head's share |
| Hand: finger, bunched fingers, fist, palm, knee, the other hand's heel | contact time and mass: the finger is light and short, the knee is a big soft mass that stops the frame | the frame's kick shape; a "stopped" flag: the frame stops after the impact, so the jingles get one clean throw and less re-excitation |
| Damping hand on the head | the head's Q, and how much the frame moves on | the head mode's damping |
| Thumb on a pair (riq) | that pair is silenced | per-pair damping |
| Choke | flat, plus the hand on the head | `gEff_` up, friction up, frame stopped |
| Jingle alloy | **[web, Grover, Weiss, Pearl]** steel/chrome > German silver > copper/beryllium copper > brass > phosphor bronze > heat-treated: from brightest and longest to darkest and driest | the mode frequencies × 0.85–1.15, Q × 0.5–1.5 (an instrument variant, not per stroke) |

### 3.4 The head (when there is one)

- **Estimated modes** **[calc from a 12″ frame drum, Illinois REU]**: a 10″ tambourine's (0,1) mode
  would sit at about 180–300 Hz, heavily damped by the holding hand.
- **The orchestral tambourine has a head; the rock tambourine usually has none** **[web, Stiers]**.
- **[design]** Have a headed variant with a pop that grows with velocity and with closeness to the
  centre. The fist and the knee bring it out.

## 4. Choosing a gesture for a note

- **Ben Stiers' grid of tempo × dynamic** is the frame **[web]**:

| | soft | loud |
|---|---|---|
| slow | fingertips at the rim | fingers, then fist, then palm, in the centre |
| fast | on the thigh, the fingertips of both hands | fist and knee |
| sustained | finger roll, then thumb roll | shake roll |

- **[design] The thresholds, to be heard:**
  - **"Fast":** notes closer than about 110 ms (16ths above ~135 bpm, 8ths above ~270 bpm). Below
    that a single hand can still repeat cleanly.
  - **"Sustained":** a key held longer than about ¾ of a beat (the maraca's roll rule), or the
    sixth repeat in a row closer than about 70 ms (a written roll as 32nds).
  - **Which roll:** a thumb roll if the roll is under about 1.5 beats or at mp and below; else a
    shake roll.
- **Pop grids** **[design, from §2.3]**:
  - Continuous down/up motion at the 8th-note rate.
  - Notes on 1 and & are down strokes, notes on e and a are up strokes, about 3 dB weaker.
  - A note on 2 or 4 that is accented in the file, or the loudest in its bar, becomes the stopped
    hit against the other hand.
  - The lift is the up stroke before it.

## 5. Listening to the band

These are all **[design]** rules drawn from the sources in §10. Each says what is to be read from the song.

- **The hi-hat and ride grid.**
  - From the drum channel's notes 42/44/46/51/59, find the hi-hat's subdivision per bar.
  - Hats on 8ths: the tambourine takes backbeats or 16ths, never the same 8ths at equal weight.
  - Hats on 16ths: the tambourine drops to backbeats.
  - The swing ratio is the hats' or ride's own.
- **The backbeat.**
  - Hit together with the snare (38/40), but usually on only one of 2 and 4 **[web, F1]**.
  - Choose the one with fewer vocal (melody-channel) onsets within ±⅛ bar.
  - Both in the chorus or the loudest section.
- **The vocal.** Thin out (drop the in-between notes) where the melody's onset density is high.
- **Cymbals.**
  - Pull back about 6 dB, or skip the stroke, at a crash (49/57/55/52).
  - No 16ths over a busy ride: the two share 6–10 kHz **[web, mixing guides]**.
- **Loudness.**
  - Follow the band's level (the director's level curve, or the drum channel's velocities over a
    bar), about 4–8 dB under the snare.
  - As the band gets louder, the gesture moves up the ladder of §2.1 (more head, brighter), not
    only the gain.
- **Length of the band's notes.**
  - Staccato comping and a short bass: strokes at 45° and choked.
  - Legato or a ballad: soft rolls and single strokes, no 16ths.
- **Microtiming** **[web, Acoustics Today, Danielsen, Câmara]**:
  - Accented notes slightly early, by 5–30 ms, locally rather than as a constant offset.
  - Hard attacks are heard as earlier, soft ones later.
  - Samba's 3rd and 4th 16ths early.
  - Measure against the hats in the file rather than invent.
- **The section's identity.** Two parts on 8ths or 16ths (shaker and tambourine) are panned apart
  **[web, Sweetwater]**.

## 6. Following the form and the director

- **The ladder** **[web, Hudson, jvworship, BMI, zZounds]**:
  - verse 1: tacet or beat 4;
  - verse 2: 2 or 4;
  - pre-chorus: a little louder;
  - chorus: 2 and 4, or 8ths or 16ths;
  - solo: 16ths;
  - bridge: lay out or change instrument;
  - last chorus: all of it.
  - Keep the pattern steady within a section, because changes every bar draw attention.
- **Lead-ins** **[design]**:
  - A crescendo shake of half a bar to a bar into the downbeat of a louder section, ending in an
    accented stroke.
  - At most once or twice in a song **[web, F21]**.
  - Orchestral: a slight crescendo at the end of a passage, "like a baton passing to the next
    section" (Dvořák *Carnival*) **[web, Ito]**.
- **Pacing** **[web, Ito]**: through a long ff, save the most for the end.
- **Endings:** a struck last note, choked within 100–300 ms. It rings only if the band holds a
  fermata or a cymbal swell.
- **The director's cue** (`struct Cue`, `src/director.h`):

| Cue | The tambourine's answer |
|---|---|
| level curve | gesture choice, §2.1 |
| accent | stopped hit, from closer to the centre |
| articulation (detached) | tilt toward flat, choke |
| phrase climax | the full ladder rung; a lead-in shake before it |
| fermata / last chord | roll or let ring; else choke at the cut-off |
| style | Baroque/Classical: orchestral technique (headed, fingers, thumb rolls); a song: pop technique (headless, shakes, the other hand's heel) |

- **What the director cannot yet give:**
  - sections (verse or chorus) and section-level arcs (`Director.md`: "Not built: sections").
  - The tambourine's ladder needs them. Until then, a section can be inferred from the drum
    channel: a crash after a fill, a change of hi-hat pattern, and the loudness bar by bar
    **[design]**.
- **Genre defaults** **[web, §10; design]**:
  - **Jazz:** off by default, because the ride and hats already hold that band; in soul-jazz or
    gospel, backbeats only.
  - **Samba/bossa:** pandeiro-like 16ths, accents on the 1st and 4th 16th, soft in bossa.
  - **Reggae:** light offbeat shakes, never on 1.
  - **Motown/soul:** a busy figure between backbeats plus the backbeat, forward.
  - **Rock:** backbeat or 8ths in choruses, hard.
  - **Ballad:** tacet, or rare soft rolls.

## 7. What the written notes mean in the user's files **[meas]**

- **The count:** `tamb.py` (`research/gm_compare/work/tambourine/`) over `~/midi` and `~/Music`.
  - 68 files have key 54 (not counting the `-breath` copies), about 1150 notes.
  - 15 files have 20 or more.
  - None is in `~/midi`.
- **Nearly all are transcriptions:**
  - every note at velocity 100;
  - placed uniformly over the beat (the files' grid is not the music's);
  - frequent coincidences with other "shaker-like" keys (69, 70, 82).
- **Some are surely false:** Buxtehude organ, 149 notes, 100 of them together with 69; Body and
  Soul, 2 notes.
- **The one hand-made file is different:** `Spain3.mid` has real velocities (10–100) and most notes
  a 16th before the beat.
- **[design] What the player does with written notes:**
  - **Isolated notes** (fewer than about 4 in 8 bars): played only if they land on a strong beat
    with the band loud; else skipped, or turned to a soft single stroke.
  - **Runs:** quantized in feel to the hats' grid, given the accent shape of §4 (velocity-100 runs
    otherwise sound like a machine), and the lift added.
  - **Notes coinciding with a cabasa or maraca note within 20 ms in a transcription:** a weak
    "tambourine" is likely a misheard shaker, so they are skipped or played at −10 dB.
  - **A switch, `PJ_TAMB_FREE`, to play exactly what is written.**

## 8. Model plan [design]

### 8.1 The instrument (`Shaker`, `Type::kTambourine`)

1. **Spectrum.**
   - The 3.2 kHz mode at −18 dB.
   - A fourth disc mode at 12 kHz (±8 %).
   - The contact shorter with impact speed.
   - Check: centroid 8.5–10 kHz, the 3 kHz band at −25 to −35 dB, −30 dB at 100 (soft) to
     150 ms (hard).
2. **The stroke as a gesture,** in place of the single speed: a `struct TambGesture` with one
   field per axis of §1.2.2:
   - `drive`: against the hand / thigh / knee / hip, a reversal in the air, a wrist twist, the
     head, friction;
   - `striker`: palm, heel, fist, fingers, 1–3 fingertips, thumb;
   - `place`: 0 = rim … 1 = centre, setting the `rock` and the head's share;
   - `height`: low … chest … high, setting the level, the shelf and the damping;
   - `tilt`: 0 = flat … 1 = upright, setting `gEff_`, the pair friction and `gapS`;
   - `speed`;
   - `lift`: its size and lead;
   - `follow`: ring, hold or choke.

   `Shaker::tambourine(const TambGesture&)` plays it. The twist kicks the pairs by their radius, in
   order around the ring.
3. **Shake:** a frame acceleration profile (down/up), each reversal kicking the pairs, plus the
   lift.
4. **Rolls:**
   - Shake roll: a jittered reversal train, about 8–14 a second, with a random phase per axis.
   - Thumb roll: a stick-slip train of weak head kicks, at a rate set by speed and pressure
     (30–80 a second), with its length capped (3–8 s).
   - Both with an attack and an ending.
5. **Choke** (a call, as the cabasa's `release()`).
6. **Head** (the orchestral variant): one damped mode at ~220 Hz plus one at ~500 Hz.
7. **Housekeeping:** `Shaker::ur()` is an inline xorshift. Move it to `Rng` (`src/rng.h`), as the
   project's rule asks.

### 8.2 The player (`ShakerPlayer`, the tambourine branch) and the director

- **The director's part.** Add a tambourine intent per section to the director's output: presence
  0 (tacet) … 1 (featured), the level arc, the lead-in points, the style and the articulation.
  - The director now gives per-note `Cue`s only to non-drum channels. The tambourine needs a few
    per-section numbers in addition, read by the plan.
  - Without sections in the director, the plan infers them from the drums (§6).
- **The musician's plan before the song,** like `planCymbalSwirls` (`src/drumkit.cc`).
  - **Input:** the `DrumPlanNote`s (every drum and non-drum onset with its velocity), the tempo
    map, the director's intent, and the melody channel's onsets.
  - **First it listens.** Per bar it measures:
    - the hats' grid and swing;
    - the snare's backbeat;
    - the crashes;
    - the kick and bass density;
    - the vocal density around 2 and 4;
    - the band's loudness;
    - how long the band's notes are.
  - **Then it answers each written note** with one of the gestures of §1.1: keep, skip, merge into
    a roll, or add a lead-in. For each it fixes:
    - the gesture (§4);
    - down or up;
    - lead;
    - strength (§5);
    - tilt and place;
    - the section's rung (§6).
  - **It prints its reading,** as the swirls are listed: per section, its rung and the gestures
    used. Then the owner can hear what it decided and why.
- **Live, without a song:** velocity chooses the hand and the place by §2.1, a held key is a roll,
  and a fast repeat changes to the fast techniques.

### 8.3 Checks

- **The spectrum numbers of 8.1.1,** against `headless.wav` and `tamb.wav` with `cent.py`.
- **Two same-velocity strokes:** correlation below 0.3.
- **A 16th run at 100 bpm:** the down and up strokes differ by 2–4 dB, and the lift's peak is
  80–120 ms before each main one.
- **A choke:** −40 dB within 60 ms.
- **The listening list:**
  - single strokes, pp to ff;
  - a Motown 16th figure;
  - a shake roll into a downbeat;
  - a thumb roll crescendo;
  - `Can't Run But` and `Say My Name` (the files with the most runs), base against new.

## 8.4 Built: the sound (2026-11-27)

- **The owner:** "fix the sound, but it is not bad as is: a lot of brightness depends on how far it
  is mic'ed". So the aim is not the close-miked recordings' 9–10 kHz.
- **`src/shaker.cc`:**
  - the 3.2 kHz mode ×0.3 (`PJ_TAMB_LOW`);
  - a fourth disc mode at 12.5 kHz ±8 %, gain 0.6, T60 0.1 s (`PJ_TAMB_HIGH`, 0 drops it);
  - the jingles' contact reaching up to `PJ_TAMB_HARDHZ` 9000 × (meeting speed in m/s)^`PJ_TAMB_HARD`
    0.7, as a 4th-order roll-off normalized to each disc's main mode. A soft meeting is darker,
    not quieter.
  - The old sound: `PJ_TAMB_LOW=1 PJ_TAMB_HIGH=0 PJ_TAMB_HARD=0` (bit-exact).
- **Measured** (`-seq`, dry, 6 strokes per velocity, the first 50 ms; `sweep.py` in the scratchpad,
  `cent.py` here):

| | v30 | v60 | v90 | v110 | v127 |
|---|---|---|---|---|---|
| centroid before (kHz) | 6.9 | 6.9 | 6.8 | 6.4 | 6.4 |
| centroid now (kHz) | 6.3 | 7.3 | 7.6 | 7.5 | 7.6 |
| 2.8–3.6 kHz share before / now (dB) | −11 / −14 | −13 / −20 | −12 / −20 | −10 / −22 | −10 / −22 |

  - Before, it grew *darker* with velocity; now it is brighter, as the recorded half-moon is.
  - `tools/test.py shakers`: all pass. The test's centroid is 6644 → 8136 Hz. Velocity 16→127 is
    35.5 → 36.0 dB. The fuzz peak is 2.02 → 2.36.
  - **Not changed:** the length of loud hits (still about 100 ms to −30 dB, against 130–200 ms
    recorded).
- **Listening:** `research/gm_compare/work/tambourine/tamb-groove-{base,new,new2}.wav` (a pop
  16th figure, then single strokes from 20 to 127) and `saymyname-{base,new,new2}.wav`
  (from 1:00). new2 = `PJ_TAMB_LOW=0.2 PJ_TAMB_HARDHZ=11000`, brighter still.
- **The owner, 2026-11-28:** "tambourine-new is great". It stays the default.

## 8.5 Built: gestures and the player's plan (2026-11-28)

- **The instrument** (`src/shaker.cc`, `Shaker::tambourine(const TambGesture&)`;
  `src/tambgesture.h`):
  - **One throw of the jingles,** `throwRattlers`: which pairs, how hard, spread over how long, in
    order round the ring for a twist.
  - **Drives:**
    - against the hand or leg: all pairs within 1.5 ms, the leg's pop through the head;
    - in the air: 10 ms spread, the up stroke at 0.65 of the down stroke's 0.85;
    - twist;
    - head: per striker a kick factor and a head share; the place sets the rock (the rim kicks the
      near pairs) and the head share;
    - shake roll: 9–14 reversals a second, irregular, with smaller shakes between;
    - thumb roll: 35–70 weak kicks a second, of the head and the near pairs;
    - choke: the discs at rest, ~60 dB in 30 ms.
  - **Tilt:** gravity along the pins (9.81 × (0.15 + 0.85 cos(tilt·90°))), and the bounce
    × (0.75 + 0.5 tilt).
  - **Height:** a shelf above 4 kHz (−5 … +1.5 dB) and a level (−3 … +2 dB), gliding over 20 ms.
  - **The head:** two damped modes, 230 and 480 Hz.
  - **A small timed queue** for the lift, the stroke after the lead, the choke and the rolls.
  - `Shaker::ur()` now draws from `Rng` (`src/rng.h`). This changes every shaker's random draws,
    not their statistics.
- **The player** (`ShakerPlayer::tambourineOn`): in a song it plays the planned gesture
  (`plannedTambourine`, by the song's time); live it chooses from the velocity, the repeat and
  the held key (MANUAL).
  - **The hand's speed** is v^1.0, not v^1.4: the striker ladder carries ~11 dB of the dynamic.
  - **The gestures' level** is trimmed −7.5 dB to match the single stroke's. Measured at 20…127,
    the two are within ±2 dB.
- **The plan** (`src/tambplan.cc`, called in `src/midifile.cc` after the director, before the
  leads): every rule of §1.2.3, §4–§7 that the file and the director can feed. It prints its
  reading:

| Song | Notes | Read as | Played | What it chose |
|---|---|---|---|---|
| Say My Name | 113 | transcription, rock/funk | 110 | 53 down, 49 up, 2 backbeats against the hand, 6 on the head |
| Can't Run But | 50 | transcription, rock/funk | 50 | 23 down, 25 up, 2 backbeats; 13 softer with a shaker's note |
| Jeepers Creepers | 162 | transcription, jazz | 28 | 66 left to the shaker, 14 in-between left to the ride; 3 shake rolls, 20 down, 6 on the head |
| Buxtehude (organ) | 149 | transcription | 14 | 88 left to the shaker; 2 shake rolls |
| Spain3 | 29 | velocities meant | 29 | 18 on the head, 8 down, 1 backbeat, 1 twist; the last choked |

- **Tests** (`tools/test.py shakers`): a held key is a roll until released; soft strokes are
  darker than hard (6.5 against 8.6 kHz); velocity 16→127 is 30 dB; all pass.
- **Not built:**
  - per-section intent from the director (it has no sections yet);
  - moving a transcription's notes in time;
  - the band's note lengths (staccato or legato), which `DrumPlanNote` does not carry;
  - the riq's thumb on one pair.

## 9. Open questions for the owner

- Should written notes be filtered (§7) by default, or only when the file looks like a
  transcription (flat velocity 100, off grid)?
- Headed (orchestral) or headless (pop) as the default? Or chosen by the director's style?
- How far should the player go in *adding* notes? The owner's principle (§1.1) lets the musician
  decide what is appropriate, so adding is allowed. How bold should it be? Proposed: only lead-ins
  and the choke or ring of a last note, never a new groove where the file has none.

## 10. Sources

- **[Stiers]** B. Stiers, "Tambourine Essentials" (Grover Pro):
  https://groverpro.com/docs/Tech-Talk/Tambourine_Essentials_Ben_Stiers.pdf (text in
  `research/gm_compare/work/tambourine/stiers_tambourine_essentials.txt`)
- **[VSL]** https://www.vsl.co.at/academy/percussion/tambourine
- **[Instrumentalist]** https://theinstrumentalist.com/october-november-2020/tambourine-technique/
- **[Band Directors Talk Shop]** https://banddirectorstalkshop.com/tambourine-101/
- **[William James]** https://williamjamespercussion.com/tag/tambourine-roll/
- **[Hudson]** J. Brinkworth on tambourine, Jack Ashford: https://hudsonmusic.com/?p=8102
- **[Henzie-Skogen]** https://www.musicgurus.com/courses/youngblood-brass-band-lessons/lessons/percussion-accessories-tambourine
- **[Kaufman]** https://www.jefftk.com/p/what-noise-does-a-tambourine-make
- **[Ito]** an IU doctoral document on orchestral percussion excerpts (*Carmen*, *Carnival*,
  *Petrushka*, *Nutcracker*, Britten):
  https://scholarworks.iu.edu/iuswrrest/api/core/bitstreams/278fc920-83d2-4233-ac0d-dd2d99ad8909/content
- **[Grover]** Danse Arabe notation:
  https://global.groverpro.com/have-we-been-playing-the-arabic-dance-wrong-all-these-years-judge-for-yourself
  and jingle alloys: https://groverpro.com/jingles
- **[Weiss]** https://www.steveweissmusic.com/product/choose-a-tambourine/articles
- **[Pearl]** https://pearldrum.com/products/percussion/tambourines/headless-tambourines
- **[F1]** https://percussionforworship.blogspot.com/2012/01/which-backbeat-to-play.html
- **[jvworship]** https://jvworship.blogspot.com/2009/02/worship-team-using-auxiliary-percussion.html
- **[BMI]** https://www.bmi.com/news/entry/percussion-primer
- **[zZounds]** https://blog.zzounds.com/?p=39622 (excerpt only)
- **[F21]** https://worship.calvin.edu/resources/articles/simple-percussion-instruments-worship
- **[Sweetwater]** https://www.sweetwater.com/insync/5-ways-to-improve-your-percussion-tracks/ (excerpt only)
- **[Acoustics Today 2007]** pandeiro and tamborim microtiming:
  https://acousticstoday.org/issues/2007AT/Jul2007/files/basic-html/page43.html
- **[Gouyon]** https://compmus.ime.usp.br/sbcm/2007/papers/sbcm-2007-17.pdf
- **[Danielsen]** Danielsen et al. 2023, *Popular Music*:
  https://www.cambridge.org/core/product/BBC410F9849DB982AEBFACEA14D38F32/core-reader
- **[Câmara]** https://duo.uio.no/bitstream/handle/10852/79965/MP3801_01_Camara.pdf?sequence=1
- **[Dende]** https://dendearts.com/blog/how-to-play-the-pandeiro-for-capoeira/
- **[Turrisi]** https://francescoturrisi.substack.com/p/a-brief-introduction-to-the-italian
- **[STK]** https://github.com/thestk/stk/blob/master/src/Shakers.cpp (tambourine 2300 / 5600 /
  8100 Hz, 32 objects, ±5 %)
- **[Avanzini]** https://avanzini.di.unimi.it/downloads/publications/papetti_dafx09.pdf
- **[Todd 2024]** Todd et al., the tambourine's directivity, spectra and modes, JASA 155(3)
  Suppl. A254, doi:10.1121/10.0027406 (abstract only, no numbers)
- **[Illinois REU]** a 12″ frame drum's modes:
  https://courses.physics.illinois.edu/phys406/sp2017/NSF_REU_Reports/2010_reu/Gregoire_Tronel/Gregoire_Tronel_Final_Report.pdf
- **Mixing guides:** https://soundshockaudio.com/how-to-eq-tambourine,
  https://johnnycopland.com/how-to-mix-live-tambourine/,
  https://www.izotope.com/en/learn/how-to-mix-hi-hats
- **Recordings:** https://commons.wikimedia.org/wiki/File:Headless_tambourine.wav (CC0),
  https://commons.wikimedia.org/wiki/File:Tambourine.ogg (public domain)
