# The brass section (GM 62 "Brass Section"): research

Status: built (2026-11-20): `-inst brass-section`, `src/brasssect.{h,cc}`; section 14 says what
was built and where it departs from this plan. (Before it, in `src/midifile.cc`'s kGm table GM
program 62 was `{"brass-section", "horn", "trumpet"}` with no "brass-section" built: one horn.) The single
brass voices exist and are physical models with players: `horn`, `trumpet`, `trombone`, `tuba`,
`muted-trumpet` (`src/brass.cc`, `src/horn.cc`, `src/hornplayer.cc`). Sections built from single
players exist too: the string ensembles (`src/stringsect.cc`), the choir (`src/choir.cc`) and the
orchestra hit (`src/orchhit.cc`), which already has two horns, two trumpets, two trombones and a tuba
in it. This note asks what makes a real brass section sound like one, in numbers, and what a
section of the existing players needs.

How each number is marked (as in `research/timpani.md` and `research/bass_section.md`):
- **[T]** read in the primary text, or measured here from a primary artefact (the SoundFont file,
  the Roland manual's tone list, the project's own binary). Measurements are said in words.
- **[S]** from a secondary source: a paper citing another, an interview, a magazine, a search
  snippet. The primary text was not read.
- **[L]** player or arranger lore: teaching texts, interviews. Not a measurement.
- **[E]** my estimate, fit or arithmetic, including statistics computed here over a local MIDI
  collection and simulations. Where it comes from running code, this is said.
- **[M]** general knowledge, not looked up. Check it before relying on it.

References are short keys in brackets. The list with URLs is section 13. What could not be found
is in section 12. Scripts and renders for the measurements are in the session scratchpad (section
13, "Artefacts"). They are not in the repository.

**Short answer.**
- **What GM means by it.** On the Sound Canvas, "Brass 1" is one sampled section of trumpets and
  trombones. The SC-88Pro adds variations: Brass ff, Brass 2, Brass 3, Brass sfz, Brass Fall,
  Octave Brass, Brass + Reed **[T, SC88Pro]**. XG's list names a "Tp&TbSec" variation **[S, XG]**.
  The installed FluidR3_GM "Brass Section" is a single mono multisample: 10 notes, each a looped
  unison section recording, panned hard left and hard right through different filters. Velocity
  only moves a low-pass filter, by up to two octaves. There is no octave doubling and no saxophone
  in it **[T, measured]**.
- **What the parts look like.** 556 unique GM-62 parts were found in a local collection of 6086
  MIDI files **[E, measured]**:
  - 52 % of onsets are single notes, 27 % two notes (60 % of those are octaves), 15 % three notes
    and 5 % four or more;
  - the median key is G#4 (68), and 90 % of keys lie between D3 and A5;
  - the median note lasts 0.17 s, and 44 % of notes are shorter than 0.15 s;
  - the median velocity is 106;
  - so it is mostly a pop or funk section playing stabs and unison or octave lines, with three- or
    four-note close chords.
- **What makes a section a section** (as opposed to one trumpet made louder):
  - **Beating among the players.** Each harmonic's amplitude fluctuates by 3.5–6.5 dB (sd) at
    1–2 Hz in the GM sample, against 0.2–1 dB in one project trumpet **[T, measured]**. That needs
    at least 3 players a note, a few cents apart (2–4 c sd) **[E, simulation]**.
  - **Onsets spread by milliseconds, not tens of milliseconds.** Measured between-player SDs are
    24–73 ms for classical trios and quartets **[S, Rasch; Wing]**. Listeners hear an asynchrony sd
    of about 4–8 ms in tight material **[S, Wing14]**. A pop section's stabs need about 5–10 ms sd.
  - **Little or no vibrato in the section.**
  - **Matched dynamics with the lead slightly on top.**
- **Cost is no obstacle.** One brass voice costs 65–100 ns a sample on one thread **[T, measured]**,
  so a 13-player big band brass section is about 6 % of a core. Do not fake players with detune.

---

## 1. What the project has, and what can be reused

### 1.1 One brass voice
(`src/horn.h`, `src/hornplayer.cc`, `src/performer.{h,cc}`, `src/brass.cc`.)
- **The physical model.** A lip-valve (outward-striking, one mass) drives a waveguide bore with valve
  loops. The trombone has a slide. Nonlinear propagation ("brassiness": Burgers steepening, a
  shock) runs on the outgoing wave (`horn.h` line 109, `horn.cc` line 304) **[T, code]**.
- **Ranges** (sounding MIDI, `kBrass[]`):

  | Instrument | Range | Notes |
  |---|---|---|
  | horn | 48–84 | |
  | trumpet | 54–84 | |
  | trombone | 36–77 | F attachment |
  | tuba | 28–65 | |
  | muted-trumpet | 54–84 | Harmon mute |

  Held notes are steady (the test checks this) from horn 45, trumpet 55, trombone 41 and tuba 37
  up to 78, 83, 78 and 66 (`tools/test.py` line 310).
- **Articulation** (`Performer::articulate`): slur, glide (a "rip": the lips slide through the
  partials, triggered by a very soft overlapping key), tongued legato, du, tu, breath and accent.
  The accent is an attack push of breath (×1.5 the pressure, decaying in 80 ms, `kAtkP_`/`kAtkT_`).
  A short note (`released()` before `kStaccato_`) is stopped by the tongue **[T, code]**.
- **Expression.**
  - Vibrato (rate 5 Hz, its depth from the director's cue).
  - Flutter tongue or growl (`flutter_`, 27–31 Hz).
  - Lip roll (`setLipRoll`, −1..1: the pitch bend with the lips).
  - A lip "ear" that pulls the pitch onto the note over 0.4 s (`PJ_LIPEAR`, `kEarTau_`).
  - `seed(s)` already gives each player its own vibrato rate (±8 %), depth (±15 %), phase and
    flutter rate **[T, code]**.
- **Missing.**
  - There is no fall or doit gesture: a pitch drop or rise at the note's end through the partials.
    The glide is the nearest thing, but it moves between two keys.
  - There is no shake (a lip trill on one note).
  - `HornPlayer` has no `vary(seat)` and `Horn` has no `Tube::vary`: both are the defaults that do
    nothing. A section of trumpets today would differ only by `seed`.
- **Velocity.** Measured on the trumpet's G4, with `-room dry -transpose 0` and one `-seq` key held
  2.5 s **[T, measured]**:

  | Velocity | Level | Spectral centroid |
  |---|---|---|
  | 40 | −30.4 dB | 0.80 kHz |
  | 100 | −27.6 dB | 0.79 kHz |
  | 127 | −21.3 dB | 1.09 kHz |

  The 4 kHz octave band sits 23–24 dB under the 1 kHz band at velocities 40–100 and 13 dB under it
  at 127. So outside `-play`'s velocity gain, velocity moves the trumpet little until the very top.
  The trombone's G3 behaves alike: −32.9, −29.7 and −23.9 dB. For comparison, FluidR3's section
  sample at ff has its 4 kHz band 4–9 dB under the peak band (section 2.3).

### 1.2 Sections already built from players

- **`src/stringsect.cc`** (GM 49–50) is the model to follow. It has:
  - **The seats.** A table of sections (`StringSection`: instrument, players, register, most keys a
    section divides into, which keys it prefers). There are 24 `SectPlayer` seats.
  - **The dealing.** `sectAssign()` deals the held keys (`sheld_[]`) to each section's players,
    evenly. The odd players go to the section's side (the violins' top notes, the basses' low
    ones). A player whose key is still held stays on it.
  - **The timing.** Entries come 20 ± 15 ms (sd) after the key, plus the seat's habit (4 ms a desk
    and ±5 ms) and the section's own moment (±8 ms). Moves are scattered over 30 ms and releases
    over 60 ms.
  - **The players.** `Performer::vary(seat)` and `Tube::vary(seat)` make each player and instrument
    its own. `BowPlayer::vary` gives seat cents ±6 (uniform), vibrato rate ±12 % and depth ±30 %,
    a vibrato delay of 0–150 ms, bow speed and pressure, plus habits per section.
  - **Velocity and gain.** Velocity is jittered ±6 per player. The section gain is
    `PJ_STRINGS_GAIN` 2.0, "measured on a chord".
- **`src/orchhit.cc`** (GM 56) has `Section` kids, each an `Ensemble` of one instrument's players
  with octave offsets folded into a range. The brass in it: horn 2 players (0, −12), trumpet 2
  (+12, 0), trombone 2 (0, −12), tuba 1 (−24). It shows that the brass players already run as
  ensemble members in a room.
- **`src/choir.cc`** deals voice parts and has per-singer `vary`. Its research note
  (`research/choir.md` §3.2–3.5) holds the perceptual numbers on unison scatter. Listeners prefer
  0–5 c between the singers' mean pitches and tolerate up to 5–14 c **[S, Ternström via choir.md]**.
- **`Ensemble::retune()`** moves sounding notes a few cents towards just intervals. Brass
  professionals lean towards equal temperament (section 6.3).
- **The director** (`src/director.h`) cues accent, length, legato, phrase end, vibrato depth, delay
  and rate, swell, level and mute per note. Each player takes what it can. There is no cue for a
  fall, a doit or a shake.

---

## 2. What "Brass Section" is in GM practice

### 2.1 Roland Sound Canvas
From the SC-88Pro owner's manual tone list. Its columns are the SC-88Pro, SC-88 and SC-55 maps
(a number after a name is its partial count) **[T, SC88Pro]**:

| PC 62 variation | SC-88Pro | SC-88 map | SC-55 map |
|---|---|---|---|
| 000 | Brass 1 (2) | Brass 1 (1) | Brass 1 (1) |
| 001 | Brass ff (1) | – | – |
| 008 | Brass 2 (2) | Brass 2 (2) | Brass 2 (2) |
| 009 | Brass 3 (2) | – | – |
| 010 | Brass sfz (2) | – | – |
| 016 | Brass Fall (1) | Brass Fall (1) | – |
| 024 | Octave Brass (2) | – | – |
| 025 | Brass + Reed (2) | – | – |

- The SC-55's GM "Brass 1" is one partial: one sampled section, no layering.
- The articulations a GM file cannot ask for (ff, sfz, fall) became separate variation patches,
  so a GM-level file plays them only through bank select.
- "Octave Brass" and "Brass + Reed" exist as variations. So the plain patch is neither
  octave-doubled nor mixed with saxophones.
- The MT-32 map in the same manual has "Brass 1" and "Brass 2" at its programs 59–62 **[T, SC88Pro]**.

### 2.2 Yamaha XG
Program 62 is "BrssSect" (bank LSB 0, 1 element). Its variations, by bank LSB **[S, XG list
snippet; QY100 data list]**:
- 3 "StBrsSec" (stereo, 2 elements);
- 35 "Tp&TbSec" (trumpet and trombone section, 2 elements);
- 40 "BrssSec2";
- 41 "Hi Brass";
- 42 "MelloBrs" (mellow).

XG names the instruments outright: trumpets and trombones.

### 2.3 FluidR3_GM.sf2 (installed), dissected **[T, measured]**
Parsed from the SoundFont's pdta chunks (bank 0, preset 61 0-based).
- **Structure.** 16 preset zones: 8 velocity layers, each a "Brass Left" and a "Brass Right"
  instrument. Both play the *same* 10 mono samples ("Brass Section F3 … C7", 32 kHz, 2.3–5.3 s,
  looped from 0.5–3.5 s on). One is panned −50 % and the other +50 % (SF2 pan ±500).
  - The two sides differ only by their instrument filter: 10 kHz on one, 7 kHz on the other.
  - Release is 0.5 s (1.0 s in the 73–80 layer). Attenuation is 3 dB, plus 5 dB on the top
    samples.
  - The sample names are an octave off their root keys: "F3" is rooted on MIDI 41 (F2), "C5" on
    60 (C4).
- **Velocity.** The layers are identical except for the filter cutoff offset:

  | Velocity | Cutoff offset |
  |---|---|
  | ≤ 72 | −24.4 st (−2.0 oct) |
  | 73–80 | −17.4 st |
  | 81–88 | −12.4 st |
  | 89–96 | −8.5 st |
  | 97–104 | −5.4 st |
  | 105–120 | −2.7 st |
  | 121–127 | 0 |

  So velocity is brightness, from about 2.4 kHz at velocity 64 up to 10 kHz.
- **The samples, analysed directly** (steady part from ~1/3 of the sample, 1 s Hann window, harmonic
  peaks):

| Sample (key) | f0 | 10→50 % rise | Centroid | Centroid / f0 | Energy at f0/2 | Harmonic AM sd (h1…h8) |
|---|---|---|---|---|---|---|
| F3 (41) | 87.5 Hz (+4 c) | 70 ms | 622 Hz | 7.1 | −69 dB | |
| C4 (48) | 130.5 (−4 c) | 60 ms | 961 | 7.4 | −54 dB | 4.7–6.5 dB |
| A#4 (58) | 233.5 (+3 c) | 40 ms | 1188 | 5.1 | −58 dB | |
| C5 (60) | 262.1 (+3 c) | 170 ms | 1293 | 4.9 | −72 dB | 3.5–6.2 dB |
| F5 (65) | 350.2 (+5 c) | 40 ms | 1405 | 4.0 | −68 dB | 2.4–5.5 dB |
| A5 (69) | 440.9 (+3 c) | 15 ms | 1989 | 4.5 | −78 dB | 3.5–5.0 dB |
| C6 (72) | 524.0 (+2 c) | 20 ms | 1676 | 3.2 | −58 dB | |
| C7 (84) | 1047.5 (+2 c) | 35 ms | 1759 | 1.7 | −67 dB | |

- **No octave below.** The energy at f0/2 is 54–78 dB under f0: there is no trombone an octave
  down. Each sample is a **unison** (an octave above cannot be told apart from the harmonics).
- **Attack.** In 10 ms steps from −40 dB, every sample is within 6–9 dB of its steady level after
  20–50 ms. It then creeps up another 3–6 dB over 0.2–0.3 s: the later players and the breath
  settling.
- **The section signature.** The 10 ms envelope moves 1–3 dB from step to step, and each harmonic's
  amplitude (band-passed ±0.4 f0, Hilbert envelope, over 2 s) has an sd of 3.5–6.5 dB. Its strongest
  modulation is at 1–2 Hz (up to 6–9 Hz on high harmonics).
  - The project's own trumpet (A4, dry) has 0.2–0.7 dB on h1–h6, and its trombone (C3) 0.8–1.6 dB.
  - A simulation of N players with random phases and a σ-cent spread (each with 1 c of slow wander)
    gives a harmonic AM sd of 7.5–8 dB for N = 2, 3.9–4.7 dB for N = 3 and 4.6–5.5 dB for N = 4–8,
    nearly independent of σ from 1.5 to 6 c **[E, simulation]**. The *rate* is what σ sets: two
    players Δc apart beat at h·f0·Δc/1731 Hz.
  - The measured 1–1.5 Hz at h2–h3 of 262–440 Hz therefore means pairwise differences of about
    3 c, so **σ ≈ 2–4 c** **[E]**.
  - The sample is several (≥ 3) players a few cents apart, not a chorus effect.
- **Pitch.** The samples sit 2–5 c sharp (one is −4 c).
- **Spectrum.** Octave bands relative to the loudest, TiMidity render, velocity 112:

  | Key | 125 Hz | 250 | 500 | 1 kHz | 2 kHz | 4 kHz | 8 kHz |
  |---|---|---|---|---|---|---|---|
  | C3 (48) | −17 | −7 | −8 | 0 | −2 | −15 | −31 |
  | C4 (60) | | −8 | −9 | 0 | −5 | −9 | −21 |
  | C5 (72) | | | −8 | −1 | 0 | −4 | −13 |

  The peak is at 1–2 kHz whatever the note: a formant-like brass envelope, as `research/trumpet.md`
  describes for one trumpet ("maximum near 1000 Hz").

**TiMidity render** (`timidity -c /etc/timidity/fluidr3_gm.cfg -Ow -s 48000`, reverb and chorus
sends at 0) **[T, measured]**:
- **Level against velocity** (C4): −46.9, −40.1, −37.1, −34.6, −32.4 and −30.6 dBFS at velocities
  40, 64, 80, 96, 112 and 127. That is 16 dB over 40→127, about 33·log10(v/127). This is
  TiMidity's own curve: it hardly applies the SF2's velocity filter steps. The centroid does step
  up between velocity 64 and 80 (C4: 1.21 → 1.37 kHz; C5: 1.30 → 1.73 kHz), and the 4 kHz band
  rises by 9–19 dB.
- **Stereo.** The L/R correlation in the steady part is 0.36–0.52: pseudo-stereo from the two
  filters and the loop.
- **Against the single brass** (velocity 100, same key, steady RMS):

  | Key | Brass Section vs Trumpet | vs Trombone | vs Horn |
  |---|---|---|---|
  | G3 (55) | +6.1 dB | +4.6 dB | +1.2 dB |
  | G4 (67) | −0.7 dB | −1.0 dB | −2.6 dB |
  | C5 (72) | +1.4 dB | +0.3 dB | −3.8 dB |

  The module balances the section at about one trumpet's level, not the +6 to +9 dB of 4–8 real
  players. A GM file's mix assumes that.
- **A chord** of C4 E4 G4 C5 at velocity 100 is 9 dB louder than C4 alone (four incoherent notes
  would add 6 dB; the C5 sample is 2 dB hotter).
- **A 150 ms stab** at velocity 110 sounds for 345 ms above −40 dB: the 0.5 s release.

### 2.4 What this says for the model
The GM patch is one timbre: an open brass unison of trumpets and trombones, played ff and darkened
by a filter for lower velocities. Its stereo is fake, and its level is matched to a solo trumpet.
A section of players can do better on every count: a real dynamic range, real chords (each note its
own players) and real spread. But it should keep the patch's register (no added octave below by
default), its level and its brightness range.

---

## 3. The parts: what a GM "Brass Section" channel contains **[E, measured on a local corpus]**

A scan of 6086 local MIDI files found 634 channels playing program 61 (0-based) with eight or more
notes. That is 556 parts after removing duplicates. Durations use each file's first tempo only.

- **Simultaneous notes** (onsets within 30 ms grouped):

  | Notes at once | Share of onsets |
  |---|---|
  | 1 | 52 % |
  | 2 | 27 % |
  | 3 | 15 % |
  | 4 | 4 % |
  | 5+ | 1 % |

  By largest chord, the parts divide as: monophonic 115, at most 2 notes 133, at most 3 125, at most
  4 122, 5 or more 61. 31 % of parts are over 90 % single notes. The median part has 2 % of onsets
  with 3+ notes; the 75th percentile part has 36 %, the 90th 78 %.
- **Dyads:** 60 % octaves, then major third 9 %, minor third 9 %, fourth 6 %, fifth 6 %, sixths 6 %.
  In 32 % of the parts that have dyads, most dyads are octaves. The files already write the octave
  doubling when they want it.
- **Triads** (lower and upper interval, in semitones): (4,3) 16 %, (5,4) 12 %, (3,4) 11 %, (7,5)
  11 %, (3,5) 10 %, (12,12) 5 %. These are close-position triads and their inversions, plus the
  open fifth-and-octave power chord. 89 % of chords span an octave or less; 5 % span more than 19
  semitones.
- **Register:**
  - keys at the 5/25/50/75/95th percentiles: 50, 62, 68, 74, 81 (D3, D4, G#4, D5, A5);
  - chord tops: median D5 (74), 95 % at or below C6 (84); chord bottoms: median D#4 (63),
    5 % at or below A2 (45);
  - 3.8 % of notes are below C3 and 1.6 % above C6.

  That is the trumpets' and trombones' common octave and a half: the lead around C5–A5, the bottom
  around C4.
- **Durations:**
  - 10/25/50/75/90th percentiles: 0.067, 0.098, 0.174, 0.325, 0.756 s;
  - 44 % of notes are under 0.15 s, 66 % under 0.25 s, 8 % over 1 s;
  - per part, the median note is under 0.2 s in 53 % of parts and over 0.5 s in 11 %.

  The parts are mostly **stabs and short riffs**, with a minority of pads.
- **Velocity:** 10/50/90th percentiles 72, 106, 127. A quarter of the parts (139) use a single
  velocity. Short and long notes have the same mean velocity (101.6 and 101.5), so "short and loud"
  is not encoded by velocity.
- **Controllers:**
  - CC11 in 196 parts, CC1 in 117, CC7 rides (more than 5 events) in 61;
  - pitch bend in 79 parts (14 %).

  The first bends: 424 go down while a note is held (falls), 127 up while held, and 408 start
  after a release (resets or preparations). A held-note bend starts a median 0.28 s after the note
  on (10–90 %: 0.005–0.54 s).
- **Other programs in the same file:** strings 43 %, fingered bass 33 %, trumpet 20 %, alto sax
  19 %, trombone 17 %, tenor sax 14 %, horn 14 %, synth brass 13 %, baritone sax 6 %, tuba 5 %.
  Where saxes are wanted, files usually give them their own channels. The brass section need not
  contain saxophones.

---

## 4. Real sections, and which one a GM part stands for

### 4.1 Line-ups
- **Pop, soul and funk horn sections** **[S, SOS15; Owsinski; SOS-other]**:
  - Jerry Hey: "usually … 2 trumpets, one trombone and one sax". His Seawind Horns were two
    trumpets, trombone, tenor and baritone sax. The "Uptown Funk" break uses the same five.
  - Earth, Wind & Fire's Phenix Horns were most often a four-piece.
  - Tower of Power is five: two trumpets, two tenors and a baritone, with one trumpet doubling
    trombone.
  - Chicago: trumpet, trombone, sax **[M]**.
  - Other recorded sections: three trumpets, tenor and trombone; four trumpets, tenor and trombone.
  - A studio rarely uses only 3–4 horns: "we overdub an extra trumpet". The sound is trumpet-heavy
    **[S, Rogers]**.
- **Big band** **[S, SOS15; Absil; M]**: 4 trumpets, 4 trombones (the fourth a bass trombone),
  5 saxes (2 alto, 2 tenor, baritone), rhythm.
- **Orchestral brass** **[M]**: 4 horns, 2–3 trumpets, 2 tenor trombones and a bass trombone, and a
  tuba (Romantic). Classical-period scores use 2 horns and 2 trumpets; Baroque, 2–3 natural
  trumpets.

### 4.2 Which one the GM patch is
Trumpets and trombones in unison (section 2). The parts (section 3) are stabs, unison and octave
lines, and close triads around C4–A5. That is the pop section's music played by an all-brass
sample. A minority of files (Classical or Romantic arrangements, film scores) use it for orchestral
brass chorales: long notes, wide chords, a low bass.

### 4.3 Deciding from the part **[E]**
Read the channel once (the director already reads the whole file):

| Signal | Pop / funk section | Big band | Orchestral |
|---|---|---|---|
| Median note length | < 0.25 s | 0.15–0.4 s, swung | > 0.4 s |
| Share of onsets with 3+ notes | < 30 % | > 40 %, 4–5 notes | any |
| Chord span | ≤ 12 | ≤ 12 (close), drop-2 up to 17 | often > 12 |
| Lowest notes (2nd percentile) | ≥ 50 | ≥ 40 | < 45 |
| Director's style | kUnknown | kUnknown | Baroque, Classical, Romantic |
| Other channels | bass, drums, sax | saxes, piano, walking bass | strings, timpani |

The default is **pop**. Orchestral needs the director's style or a long-note, wide-chord part.
Big band needs 4–5-note close chords in more than 40 % of onsets.

---

## 5. Voicing: who plays which note

### 5.1 How arrangers voice it **[L]**
- **Unison and octaves most of the time.** Use "unison and/or octaves 90 % of the time … so when
  chords do feature, they have a lot of impact". The trombone doubles the lead trumpet an octave
  lower, and the saxes take one of the two or an inner note. Middle C to the C above is the
  section's "sweet spot" for a unison **[S, Rogers; SOS15]**. A scream trumpet an octave up over a
  unison in two octaves gives "the basis of the EW&F sound" **[S, SOS-other]**.
- **Chords.**
  - Three-part close harmony "does the job most of the time". For seventh chords the horns take
    the third, fifth and seventh and leave the root to the bass.
  - Pop uses close-voiced block chords; chorale spacing suits slow music. Drop-2 spreads a
    three-horn triad for power **[S, SOS15; Rogers]**.
- **Big band.** Four-way close in the trumpets, with the lead doubled an octave down by the fourth
  trumpet or the trombones; drop-2 and drop-2-4 for spread voicings; the bass trombone on the root
  **[M]**.
- **Low interval limits** (closer intervals turn to mud below these) **[L, Brainscape; M; sources
  disagree on the octave]**: minor second E3, major second Eb3, minor third C3, major third Bb2,
  perfect fifth Bb1.
- **The lead is on top and loudest.** The section matches the lead's phrasing, length and
  dynamics. Wayne Jackson: "Don't step on the singer" **[S, SOS15]**.

### 5.2 What a section should do with the keys **[E]**
A GM part already is the voicing: its keys are the sounding notes. The section must not re-harmonise
it. It decides only who plays each key, and whether a key gets more than one player.

| Keys at once | Pop (2 tp + tb, or 3 tp + 2 tb) | Orchestral (4 hn, 2 tp, 3 tb, tuba) |
|---|---|---|
| 1 | All who reach it, in unison. The trumpets above about E4; below it trombones (and horns). The tuba never in unison above about F3. | Those whose comfortable range holds it. A chorale line goes to horns or trumpets, not everyone. |
| 2 (an octave) | Trumpets the top, trombones the bottom | Trumpets or horns the top; trombones and horns the bottom |
| 2 (other) | Trumpet 1 the top. Trumpet 2 and the trombone the lower note (the trombone if below about A3). | Divide by register |
| 3 | tp1 the top, tp2 the middle, tb the bottom. With 5 players, the spares double the top and bottom. | Horns the inner notes, trombones the bottom two |
| 4+ | One player a key from the top down. Extras double the lead (and the bottom). Too many keys: drop inner duplicates, keep top and bottom. | Like the string section: each section divides its register |

- **No added octaves by default.** The GM sample has none, and the files write octaves where they
  want them (60 % of dyads). The 8vb trombone doubling of a single-line part could be a knob
  ("Octave Brass" is a separate SC-88Pro variation).
- **Notes outside a player's comfort move to another player, never to another octave.** Below
  trumpet 55 is trombone; above trombone 74 is trumpet. Keep the `prefer` idea of
  `StringSection`: trumpets take the highest keys, trombones the lowest.
- **A held key keeps its players.** As in `sectAssign`, as few players move as will do. A slur
  stays on the same player, with its own lip legato.

---

## 6. Timing, tuning, vibrato, dynamics

### 6.1 Onset asynchrony
- **Classical ensembles** **[S, Wing14, citing Rasch 1979/1998]**: between-player asynchrony SD of
  49 ms in a string trio at 79 bpm and 32 ms in a wind trio (oboe, clarinet, bassoon) at 88 bpm;
  24–73 ms overall, smaller at faster tempi. Two string quartets: 24 and 28 ms at 157 bpm. The
  principal instruments run 30–50 ms ahead of the others in trios **[S, Goebl10 citing Rasch79]**.
- **Brass and strings** in a German orchestra: mean asynchronies around −12.6 ms against a pacing
  signal at inter-onset intervals of 300–1000 ms **[S, abstract only]**.
- **What a listener hears.** Listeners judged an ensemble "more synchronised" at an asynchrony
  variance threshold of 64 ms² (sd 8 ms) in one condition and 18 ms² (sd 4.3 ms) in another, at
  162 bpm **[S, Wing14]**. A section's stab spread of more than about 10 ms sd is heard as ragged,
  and less than 4 ms as machine-tight **[E]**.
- **Pop and big-band sections** are tighter than classical trios **[L]**. Hey's players wear one
  headphone so that "we can hear everyone in the room, which helps keep the time and phrasing the
  same" **[S, Owsinski]**. No measured section numbers were found.
- **The numbers to use** **[E]**:

  | Onset kind | sd | Mean lag | Lead trumpet |
  |---|---|---|---|
  | Stab or tongued note | 5–8 ms | 0–5 ms | first |
  | Long-note entry | 10–20 ms | 5–15 ms | |
  | Release | 1.5× the onset scatter (singers: `research/choir.md` §3.4) | | |

  On stabs the release is the tongue cut-off, which is tighter.

  The string section's 20 ± 15 ms is too loose for brass stabs. It suits orchestral brass entries
  at soft dynamics.

### 6.2 Pitch scatter
- **The GM sample:** σ ≈ 2–4 c between players, from the beat rate (2.3) **[E]**.
- **Trumpet intonation:** student trumpeters' scale tones deviate by 3–12 c on average, with sd
  about 7 c **[S, Geringer & Madsen]**. Professionals adapt towards equal temperament more than to
  just intonation **[S, Kopiez03]**.
- **Perception of unison scatter:** listeners prefer 0–5 c and tolerate up to 5–14 c of steady
  scatter **[S, Ternström via research/choir.md]**.
- **Numbers to use** **[E]**:
  - each seat's mean offset: uniform ±4 c (sd 2.3 c), or ±6 c for an orchestral section at soft
    dynamics;
  - the existing breath wander (`wander_`) supplies the slow drift;
  - the lip ear must pull towards *note + seat cents*, not towards the note. Otherwise the scatter
    heals in 0.4 s and the beating stops.
- **Just intonation:** keep `retune()` but weaken it for brass, to about half (Kopiez). Big-band
  lore lowers the major thirds of held chords somewhat **[L]**.

### 6.3 Vibrato **[L, S]**
- **One brass player:** jaw vibrato at 5–6.5 Hz and ±10–25 c (`research/trumpet.md`). The tuba
  uses little or none; orchestral horns and trombones none.
- **In a section:** pop and funk sections play straight. Big-band brass plays straight, with the
  lead's vibrato at most a lip shimmer at the ends of long notes; the saxes are another matter
  **[L]**.
- **The rule to build** **[E]**: no vibrato under 0.6 s. On longer notes the lead gets the
  director's depth ×0.5 after 0.3–0.5 s; the others ×0.2, starting later; the trombones and tuba
  0. Per-seat rate ±8 % (already in `seed`).

### 6.4 Dynamics and balance **[E, L]**
- **Within the section:** per-player velocity jitter ±4 (smaller than the strings' ±6), about
  ±0.7 dB. The lead trumpet +1.5 dB over the others: it "plays on top", and the section listens
  to it.
- **The "fat" unison** is the beating (6.2) plus small timbral differences between players. Give
  each player a slightly different lip (resonance ratio ±3 %, Q ±10 %) and attack push (±15 %),
  through `HornPlayer::vary`. Give each instrument a slightly different bore: mouthpiece cup volume
  ±5 % and bell flare scale ±2 %, through `Horn::vary`. The spectral differences should be ±1–2 dB
  per third-octave, as between instruments of one model **[E]**.
- **Level for N players:** N equal incoherent players add 10·log10 N dB: +3 dB for 2, +4.8 for 3,
  +6 for 4, +9 for 8 **[M]**. The GM mix expects a unison key at about one trumpet's level (2.3).
  Set the section's gain so that **a unison key at a given velocity sounds +1 to +3 dB over a
  single trumpet**. Chords then add up naturally (FluidR3: +9 dB for four keys).

---

## 7. Articulation in sections

### 7.1 What sections play **[S, SOS15; L]**
- staccato (dots are "essential" to soul and funk phrasing);
- 16th-note pushes (accents just before the beat);
- off-beat 8ths with a short fall;
- stabbed falls at the end of a phrase;
- doits (a rise at the note's end);
- shakes (a lip trill to the next partial, on trumpet);
- scoops and rips (into a note);
- sfz-crescendo (fp <);
- the "shout chorus".

About 90 % of section playing is open horns. The Harmon mute is the buzz.

### 7.2 How GM files carry them **[E, measured]**
- **Usually not at all.** The SC-88Pro gives them as separate patches (Brass sfz, Brass Fall,
  Brass ff), which a GM file cannot reach.
- **Pitch bend:** 14 % of parts carry it, mostly as falls (downward bends during a held note,
  starting a median 0.28 s in). The 127 upward bends during held notes are doits or scoops.
- **Swells:** CC11 or CC7 rides in about a third of parts carry swells, sfz-crescendos and fades.
- **Stabs:** short notes. Nearly half the notes are under 0.15 s.

### 7.3 What a MIDI-driven section should infer **[E]**
- **Pitch bend down during a note → a fall.** Each player bends with the lips (`setLipRoll`) for the
  first semitone or two. Beyond that the lips step down through the partials (a glide through the
  harmonic series, as the rip does), the air easing.
  - Each player starts the fall 0–30 ms apart and falls at its own speed (±15 %): a section fall
    is a smear, not a parallel slide.
  - An upward bend at the end of a note is a doit, played alike, upwards.
- **No bend: invent nothing by default.** A director option (pop style only, PJ_BRASS_FALLS) could
  add a short fall to an accented note that ends a phrase, at most one in eight such notes. Not on
  by default.
- **A short loud note (< 0.15 s, velocity ≥ 90) → a stab.** Accent attack (the push), tongue-stopped
  release, tight onsets (6.1), no vibrato. This is the existing accent and staccato path; the
  section only has to keep the onsets tight.
- **A long note (> 1 s) with a CC11 dip after the attack → fp<.** Follow the curve. The director's
  `kCueSwell` covers a written crescendo.
- **Shakes:** not inferable. Possibly a trill-like alternation of keys a partial apart (skip).

---

## 8. Placement and recording

- **Seating:**
  - big band: saxes in front, trombones in the middle (tiered higher), trumpets at the back,
    standing or on risers **[S, Absil; Rogers]**;
  - orchestra (US seating): horns centre-left at the back; trumpets centre-back; trombones and tuba
    centre-right at the back **[M]**;
  - pop studio: a semicircle. Hey puts the trumpets on one mic and the trombone and sax on one each,
    Royer ribbons, about 2 ft (0.6 m) from the bell (1.5–4 ft; 6 ft for Earth, Wind & Fire's
    livelier sound) **[S, Owsinski]**. Another engineer suggests 4–5 ft back for trumpets in a
    section **[S, Mix]**.
- **Stereo:**
  - FluidR3's section is pseudo-stereo, L/R correlation 0.36–0.52 (2.3);
  - a pop section is usually a modest spread: trumpets left of centre, trombone and sax right of
    centre, about ±20–30° **[E]**;
  - big band: trumpets across the back and trombones across the middle, each row about ±30°;
  - orchestral brass sits right of centre and far back.
  - The project's room gives each player its own position. Use positions, not pan laws.
- **Close against room:** pop horns are recorded close and dry, then compressed and reverbed as a
  group. Orchestral brass is mostly room. Use the main room with the players placed close to the
  listening position for pop, and further back for orchestral **[E]**.
- **Level:** see 6.4.

---

## 9. Cost

**Measured** (the existing `./pajupilli`, `PJ_NOCAL=1 PJ_THREADS=1`, one note held for 20 s, user
CPU) **[T, measured]**:

| Instrument | Cost per sample | Share of a core at 48 kHz | Calibration at load (wall) |
|---|---|---|---|
| trumpet | 101 ns | 0.5 % | 1.1 s |
| trombone | 80 ns | 0.4 % | 1.4 s |
| horn | 83 ns | 0.4 % | 1.4 s |
| tuba | 68 ns | 0.3 % | 0.7 s |

- `tools/test.py` allows 800 ns per brass voice (line 347). The string players are allowed
  5200–8000 ns each.
- **Pop section** (3 tp + 2 tb): about 0.5 µs a sample, 2.5 % of a core.
- **Big band brass** (4 + 4): about 0.75 µs, 3.6 %.
- **Orchestral** (4 hn + 2 tp + 3 tb + tuba): about 0.9 µs, 4.3 %. The room adds what it adds for
  any ensemble.
- **The 24-player string section** costs 50–100 times this.

So **no voice-saving tricks are needed**. Fewer voices with per-note detune or timing cannot stand
in anyway:
- one waveguide plays one pitch, so two players need two bores;
- a detuned copy of one voice's output is a chorus, which beats regularly and identically on every
  harmonic;
- the section's beating comes from independent players with their own wander.

The only saving worth making is calibration: calibrate each instrument kind once (as
`initStrings` does with `kidCals_`) and share it among its seats.

---

## 10. What a listener would notice if it were wrong **[E, L]**

1. **One trumpet, louder.** No beating (harmonic AM under 1 dB), no width: "a solo with reverb".
   This is the present fallback.
2. **A chorus effect.** Regular, periodic, equal-depth beating on all harmonics: a synth "brass".
3. **Ragged stabs.** Onsets spread over 30+ ms sound like an amateur section. Spread under 3 ms,
   with identical attacks, sounds like a sampler's layering (the flam disappears, but so does the
   section).
4. **Vibrato.** Section players all wobbling is "school band". Synchronised vibrato is a pitch LFO.
5. **Out of tune.** Steady scatter over about 10 c sour on held chords, especially at ff where the
   upper harmonics beat fast.
6. **Wrong register or doubling.** Trombones added an octave below a part that has none muddy the
   mix and contradict the arrangement. Trumpets forced below about E4 sound weak.
7. **Velocity without brightness.** Brass loudness is mostly spectrum. A section that only gets
   louder with velocity, but no brighter (no brassiness at ff), sounds like a filtered pad. The GM
   patch at least brightens over two octaves of cutoff.
8. **Release.** Every player stopping at the same sample, or long ringing tails after short stabs.
   Stabs end with the tongue; long notes end a little scattered.
9. **Falls in parallel.** A fall played as one pitch bend of every player is a synth gesture.
10. **Balance.** Brass three times louder than the GM mix expects drowns the song (6.4).

---

## 11. What to build

### 11.1 Make-up per context (one `BrassSection` table, like `kStringSections`)

| Context (4.3) | Players (sounding ranges in use) | Default |
|---|---|---|
| **Pop / funk** | trumpet ×3 (58–84; lead 65–84), trombone ×2 (40–74) | yes |
| **Big band** | trumpet ×4 (58–86), trombone ×4 (40–74; the 4th as bass trombone, 36–65) | 4–5-note close chords in > 40 % of onsets |
| **Orchestral** | horn ×4 (45–77), trumpet ×2 (58–82), trombone ×3 (40–72), tuba ×1 (28–55) | director style, or long notes and wide chords |

- Saxes are not in the GM patch (2.1–2.3), and GM files give saxes their own channels (3).
  "Brass + Reed" could be a later variation using `tenor-sax` and `baritone-sax`.
- The muted trumpet could replace the trumpets when the director's mute cue is on (pop: Harmon).

### 11.2 Voicing (dealing the held keys)
- **Reuse.** Use `sectAssign()`'s machinery (held keys, `want`, `prefer`, the fewest moves) with a
  brass table. Trumpets prefer the highest keys (+1), trombones and tuba the lowest (−1), horns the
  middle (0).
- **The rules** (5.2):
  - every key gets at least one player when there are enough;
  - spare players double the lead first, then the bottom;
  - no added octaves (`PJ_BRASS_OCTAVE` 0; 1 adds trombones 8vb on single-line parts);
  - a key outside a section's range goes to another section, never to another octave;
  - at most 1 key per player.
- **The lead.** Seat 1 of the trumpets is the lead and always takes the top key.

### 11.3 Per-seat variation (new `HornPlayer::vary(seat)`, `Horn::vary(seat)`)

| What | Value | Mark |
|---|---|---|
| Seat cents (mean offset; the lip ear's target moves with it) | uniform ±4 c (orchestral soft: ±6) | E (GM sample: σ 2–4 c) |
| Onset after the key: stabs and tongued | 3 ± 6 ms (sd), the lead at 0 | E (Wing14 thresholds) |
| Onset: long-note entry | 10 ± 15 ms | E (Rasch range) |
| Section moment (all of one kind) | ± 4 ms | E |
| Seat habit | ± 3 ms | E |
| Release scatter | stabs ± 5 ms, long notes ± 25 ms | E |
| Velocity jitter | ± 4, lead +1.5 dB | E, L |
| Vibrato | none under 0.6 s; lead ×0.5 after 0.3–0.5 s, others ×0.2, low brass 0; rate ±8 % (seed) | L, E |
| Lip resonance ratio | ±3 % | E |
| Lip Q | ±10 % | E |
| Attack push (`kAtkP_`) | ±15 %, decay ±20 % | E |
| Breath wander | own seed (exists) | T, code |
| Bore (`Horn::vary`): cup volume, bell flare | ±5 %, ±2 % | E |

### 11.4 Hooks
- **`Ensemble::initBrass` / `brassAssign` / `brassOn` / `brassOff` / `brassRender`** in a new
  `src/brasssect.cc`, mirroring `src/stringsect.cc`. It reuses `drive()`, `SectPlayer`, `sheld_[]`,
  `kids_[0]` and `kidCals_`. No pizzicato wait is needed (`noteLead` 0), unless the section's
  onsets need a lead of a few ms.
- **`midifile.cc` kGm:** `{"brass-section", "horn", "trumpet"}` becomes a real "brass-section"
  instrument (`Kind::kModal` with a dummy design, as the string ensembles and orchestra hit have).
- **The director:**
  - its cues go to every player (`Ensemble::direct`): accent, length, legato, phrase end, swell;
  - vibrato depth is scaled per seat (6.3);
  - the context of 4.3 needs the director's style and per-channel note statistics (median length,
    chord sizes, lowest notes), which it has from its single pass;
  - **New:** pitch bend on a brass-section channel is read as falls and doits for the section to
    play per player (7.3), not as one bend for all.
- **`Ensemble::retune()`:** about half strength for brass (Kopiez).
- **Gain:** `PJ_BRASS_GAIN` set so that a unison key is +1 to +3 dB over the solo trumpet at the
  same velocity.

### 11.5 Checks (for `tools/test.py`)
1. **The section beats.** A held unison key (C5, velocity 100, 3 s, dry): the harmonic-band
   amplitude sd over 0.8–2.8 s is ≥ 3.5 dB for h2–h6, with the strongest modulation at 0.5–3 Hz.
   Reference: GM sample 3.5–6.5 dB at 1–2 Hz; single trumpet 0.2–0.7 dB.
2. **In tune.** The unison's mean pitch is within ±3 c of the key, and the seat scatter sd is
   2–5 c. Render each seat alone (as `PJ_CHOIR_ONLY` does).
3. **Tight stabs.** A 150 ms key at velocity 110: per-player onsets span 10–30 ms first to last,
   sd 4–10 ms, with the lead first. The stab sounds no longer than 0.35 s above −40 dB (GM: 0.345 s).
4. **Dealing.**
   - C4 E4 G4 C5: trumpet 1 plays C5 and the lowest trombone plays C4;
   - every key is sounded; no added octaves (the energy at f0/2 of a single key is 40 dB or more
     under f0);
   - a moving top line keeps the lead on it.
5. **Velocity is brightness.** The 4 kHz octave band, relative to the 1 kHz band, rises at least
   8 dB from velocity 64 to 112 (GM: 10–19 dB). The level rises 10–16 dB over 40 → 127 in `-play`.
6. **Balance.** A unison key is +1 to +3 dB over `trumpet` alone. Four keys are +5 to +9 dB over
   one.
7. **Falls.** A bend down of 7 semitones over 0.3 s on a held key: each player's pitch reaches at
   least −5 st, and the players' half-way times differ by 10–40 ms.
8. **Real time.** The pop section (5 players) is under 1500 ns/sample and the orchestral one (10)
   under 3000 ns/sample (measured cost about 1 µs; leave headroom for the room).
9. **Fuzz and NaN** as for the other ensembles.

---

## 12. Not found, or uncertain

- **Measured onset asynchrony and pitch scatter inside a pop, big-band or orchestral brass section.**
  Nothing was found. The numbers in 6.1–6.2 extrapolate from classical trios and quartets (Rasch,
  Wing), listener thresholds (Wing14), choir scatter (Ternström) and the beat rate in one GM
  sample. Rasch 1979's own text (a Dutch PDF) was not read.
- **The Roland SC-55 "Brass 1" sample itself** (how many players, which instruments) was not
  measured: no SC-55 here. XG's list was read only as a search snippet.
- **Meyer's "Acoustics and the Performance of Music"** (the choral effect, orchestral brass
  directivity) could not be downloaded.
- **Kopiez 2003's cents figures** (trumpeters' intonation of intervals) were not read: only the
  secondary summary "closer to equal temperament".
- **Section vibrato practice** is lore only. No measurements of big-band or pop sections.
- **Low interval limits:** the sources disagree on octave numbering.
- **Corpus caveats.** Durations use each file's first tempo only. Duplicates were removed by their
  first 200 keys. The collection is a personal one, not a representative sample of GM files.
- **TiMidity** largely ignores FluidR3's per-velocity filter offsets, so its velocity response
  (2.3) is TiMidity's, not the SoundFont author's intent. The sample analysis (2.3, the table) does
  not depend on the renderer.
- **What the project's trumpet lacks at ff** compared with the GM sample's brightness (4 kHz band
  −13 dB against −4 to −9 dB) is not settled. It may be the nonlinear propagation's strength, the
  close-mic recording of the sample, or both. Compare at equal measured sound level first.

---

## 13. References

- [SC88Pro] Roland SC-88Pro Owner's Manual, tone list with SC-88 and SC-55 maps (read here).
  https://dosdays.co.uk/media/midi/SC-88PRO_OM.pdf
- [XG] Yamaha XG voice lists (QY100 Data List; MU100/MU128 lists), program 62 variations; seen as
  search snippets. https://uk.yamaha.com/files/download/other_assets/0/318130/QY100E2.pdf
- [FluidR3] FluidR3_GM.sf2 (Frank Wen, 2008), `/usr/share/sounds/sf2/FluidR3_GM.sf2`, preset 0:61
  "Brass Section", parsed and rendered here.
- [Wing14] Wing, Endo, Bradbury, Vorberg, "Optimal feedback correction in string quartet
  synchronization" and the companion perception study, Frontiers in Psychology 2014, citing Rasch
  1979/1998. https://www.frontiersin.org/journals/psychology/articles/10.3389/fpsyg.2014.01115/full
- [Rasch79] R. A. Rasch, "Synchronization in performed ensemble music", Acustica 43 (1979) 121–131
  (not read; via Wing14 and Goebl10). Dutch version:
  https://publications.tno.nl/publication/34637177/fvRTQS/rasch1979synchronisatie.pdf
- [Goebl10] Goebl, Flossmann, Widmer, "Investigations of between-hand synchronization in Magaloff's
  Chopin", Computer Music Journal 2010 (cites Rasch's trio leads).
  https://www.researchgate.net/publication/298292271
- [BrassStr13] "Timing and synchronisation of professional musicians: a comparison between
  orchestral brass and string players", conference abstract, Frontiers 2013.
  https://www.frontiersin.org/articles/10.3389/conf.fnhum.2013.214.00021
- [Kopiez03] R. Kopiez, "Intonation of harmonic intervals: adaptability of expert musicians to equal
  temperament and just intonation", Music Perception 20 (2003) 383–410 (via secondary summary).
- [Geringer] Geringer, Madsen and colleagues, trumpet intonation studies (via search summary).
  https://core.ac.uk/download/345078758.pdf
- [Ternström] via `research/choir.md` §3.2 (unison scatter preferences and tolerance).
- [SOS15] D. Stewart, "Top Brass, Part 4", Sound On Sound, August 2015.
  https://www.soundonsound.com/techniques/top-brass-part-4
- [SOS-other] Sound On Sound arranging series (EW&F unison, Tower of Power line-up), via search
  summary. https://www.soundonsound.com/techniques/top-brass-part-3
- [Rogers] E. Rogers, "Writing Horns for Pop Songs" and "Getting Started" (big band arranging blog).
  https://evanrogersmusic.com/blog-contents/big-band-arranging/writing-horns-for-pop-songs ;
  https://www.evanrogersmusic.com/blog-contents/big-band-arranging/getting-started
- [Owsinski] B. Owsinski, "Miking Horns With Trumpet Legend Jerry Hey".
  https://bobbyowsinskiblog.com/?p=4668
- [Mix] Mix Online on miking trumpets in a section. https://www.mixonline.com/?p=87807
- [Absil] F. Absil, "Studio orchestra seating". https://www.fransabsil.nl/archpdf/orchseat.pdf
- [Brainscape] low interval limit flashcards (lore).
  https://www.brainscape.com/flashcards/3-5-writing-for-small-and-large-ensemble-9779777/packs/14049301
- [Sweetwater] "Low interval limit". https://www.sweetwater.com/insync/low-interval-limit/
- Project: `src/stringsect.cc`, `src/orchhit.cc`, `src/ensemble.h`, `src/director.h`,
  `src/hornplayer.cc`, `src/performer.{h,cc}`, `src/brass.cc`, `src/bassinst.cc` (BowPlayer::vary),
  `tools/test.py`, `research/trumpet.md`, `research/trombone.md`, `research/tuba.md`,
  `research/horns.md`, `research/choir.md`.

**Artefacts** (session scratchpad, not kept in the repository):
`/tmp/claude-1000/-home-jyrki-pajupilli/f3f9b7e6-c8bc-4bf3-a7a3-c720f3e9b3d1/scratchpad/`.
It holds:
- `tools/sf2dump.py` (the SF2 zone dump);
- `tools/sfx.py` (sample extraction);
- `tools/ana.py`, `raw.py`, `rend.py`, `am.py` (analysis);
- `tools/sim.py` (the N-player beating simulation);
- `tools/corpus.py`, `cstats.py`, `bends.py` (MIDI corpus statistics);
- `mid/`, `wav/`, `raw/` (renders and extracted samples).

---

## 14. As built (2026-11-20)

`-inst brass-section` (GM 62): `src/brasssect.h`, `src/brasssect.cc`; hooks in `src/ensemble.*`
(`initBrass`, `brassAssign`, `brassOn/Off`, `brassRender`), `src/hornplayer.cc`
(`HornPlayer::vary`), `src/horn.cc` (`Horn::vary`), `src/midifile.cc` (the plan, the hints, CC 11),
`src/voice.cc` (`BrassSectionModel` for `-note`, `-seq`, `-fuzz`), `src/main.cc` (`-brassvoice`).
README "Brass section" has the rules and worked examples.

**What follows the plan.**
- 11.1: the three make-ups as tabled (pop 3 tp + 2 tb, big band 4 + 4 with a bass trombone,
  orchestral 2 tp, 4 hn, 3 tb, tuba), chosen by `planBrassSection` from the table of 4.3 (median
  note length, the share of 3+ and 4–5-key onsets, chord span, the 2nd-percentile key, the
  director's style, the other channels' programs) plus swing (off-beats on the triplet's third).
- 11.2: one key per seat at most; every written key sounded; spares to the lead, then the bottom;
  no key moved by an octave except one no seat reaches; the lead seat on the top key; the least
  motion from the last voicing. A monotone dynamic programme over the seats (top down), so no
  voice crosses, with a cap of players a key.
- 11.3: per-seat cents (±4 c, stratified: sd 2.4 c), the lips aiming there (feedforward, slope
  600 c a unit of log, as the ear's own) and the ear's target moved with them; attack push ±15 %,
  its decay ±20 %; cup volume ±5 %, lip Q ±10 %, brassiness ±12 % (`Horn::vary`). Onset and
  release scatter, velocity jitter ±4, lead +1.5 dB, vibrato rules as tabled.
- 11.4: one ensemble of the seats (`kids_[0]`), dealt by the section; the director's cues to every
  player, its vibrato depth scaled per seat; pitch bend read per player (falls, doits); the
  intonation at half strength (`PJ_BRASS_JI` 0.5); the gain set on a unison key.

**Where it departs, and why.**
- **Voicing beyond dealing** (5.2 said "must not re-harmonise"). Asked for "beautiful and practical
  voicing", the section adds notes in two cases only. (a) A unison line at G4 or above is doubled
  an octave down by the low brass (the trombones; the horns too in the orchestra), unless the part
  writes its own octaves (15 % or more of its onsets). Arrangers do this (5.1); the GM sample does
  not, which is why it stops at G4 (the octave stays at G3 or above, clear of the bass) and why it
  goes when the file has octaves of its own. (b) A sparse part is harmonised where the song's other
  channels show a clear chord under 60 % of it and 75 % of its notes fit that chord (chord tones,
  9, 11 on minor, 13): pop voices only held notes (0.3 s up), big band every chord note (4-way
  close, drop-2, planing passing notes); orchestral parts never. A blues line's flat third over a
  major chord, a riff over an unclear harmony, a part with its own chords: as written. In the
  local corpus's sense (3) this leaves most stab riffs and lines alone.
- **The big band's trombones** double a close written chord (2–4 keys within an octave) an octave
  down where no interval falls under its low interval limit: the section's own doubling.
- **Speaking times** (not in the plan). The players speak at different speeds (measured: trumpet
  20–33 ms, trombone 27–50, horn up to 85, tuba ~40), so the trombones landed 20–40 ms after the
  trumpets. Played from a file the keys come 90 ms early (`noteLead`) and each player starts its
  own speaking time ahead; live, they act on the key (the trumpets first).
- **Velocity and breath.** The trumpet alone hardly brightens below velocity 127 (1.1); a section
  player's breath spans 1.6× a soloist's (`PJ_BRASS_VELDYN`), which took 11.5's check 5 from
  +7.6 dB to +17 dB; the farther breath pulls the trumpets 5–15 c flat, so a section player's
  ear reaches 2 % of the lips' tension (`PJ_BRASS_EAR`; a soloist keeps 0.8 %).
- **Lip resonance ratio ±3 %** (11.3) was not used: the lips' tension sets the pitch (1 % ≈ 6 c),
  so it would be a ±18 c scatter; the seat cents do that job. **Bell flare ±2 %** was not built
  (the bore is fixed at construction); the cup, the lip Q and the brassiness vary instead.
- **Falls** reach the GM bend's ±2 semitones (the file player does not read RPN 0); past a
  semitone each player glides its valves or slide a semitone at a time, the lips bending the rest
  (their roll reaches only ~30 c). The players' half-way times lie ~50 ms apart (11.5's 10–40 ms;
  the glides' own physics adds to the set 3–20 ms delays).
- **Balance.** The key weight in the mix is k^0.7 (+ 0.3 a voiced note): four keys 8.6 dB over
  one, the GM section +7 to +9. `levels_data.h` has the measured trim (+4.8 dB against the median,
  `levels.py` `measure()`), and `PJ_BRASS_GAIN` 0.72 puts a unison key 2 dB over a trumpet in
  `-play` (the trim's `-note` measure plays a soft default velocity, where the wider breath makes
  the section quieter). Velocity row: `velcurve` `render()` / `gain_curve()` on C4.
- **Check 2** (in tune): the unison follows the lone trumpet, which itself is +5 c at F4 and +7 c
  at A4 (a property of the trumpet model, not the section); the check asks for the unison within
  3 c of the trumpet and 6 c of the key.

**Measured** (`tools/test.py` `brass_section`; dry; one thread for cost):

| Check (11.5) | Result | Target |
|---|---|---|
| 1 beating, F4 unison of 5 | harmonic AM sd 4.9–5.7 dB (h1–h6), 0.5–2.3 Hz; one trumpet 0.3–0.8 dB | ≥ 3.5 dB, 0.5–3 Hz |
| 2 tuning | unison +4.4 c (trumpet alone +5.0); seats −3.0 … +5.7, sd 3.1 c | 2–5 c |
| 3 stabs (`-play`, six C5 stabs) | seats within 14 ms, sd 5.2 ms, lead never behind; 0.33 s above −40 dB | 10–30, 4–10 ms; ≤ 0.35 s |
| 4 dealing | C4 E4 G4 C5: tp1 C5 … tb2 C4; E4 alone: f0/2 74 dB under f0 | |
| 5 velocity | 4 kHz re 1 kHz −28.8 → −12.0 dB (64 → 112); level 13 dB over 40 → 127 | ≥ 8 dB; 10–16 dB |
| 6 balance | unison +2.0 dB over a trumpet; four keys +8.6 dB over one | +1…+3; +5…+9 |
| 7 falls | each trumpet −215 c; half-way 49 ms apart | ≤ −150 c (GM range); 10–60 ms |
| 8 real time | pop 0.66 µs/sample (3.2 %), orchestral 1.2 µs (5.9 %) | < 1.5, < 3 µs |
| 9 fuzz | 10 s, no NaN, peak 4.6 | |

**Against FluidR3's Brass Section** (TiMidity, explicit `%font` config, same MIDI, dry; held F4,
velocity 100): FluidR3 octave bands 250 Hz–8 kHz −5 −6 0 −2 −8 −18 dB (centroid 1.5 kHz), ours
−3 −5 0 −14 −27 −37 (0.84 kHz): ours is much darker above 2 kHz at mf, the trumpet model's gap
(12) at velocities under ~110. Four keys over one: FluidR3 +6.9 dB, ours +9.0. Attack: FluidR3's
looped sample is at once, ours speaks in ~20 ms. A 0.15 s stab rings 0.18 s above −40 dB on
FluidR3, 0.33 s here (the players' tongue stops, the bells' tails). Stereo: FluidR3 is
pseudo-stereo (L/R correlation 0.39); ours is placed players in the room (−0.11 in the hall, mono
dry).

**Not done.** Shakes (not inferable, 7.3); "Brass + Reed" and the muted section; RPN bend ranges;
seats thinned in dense passages (not needed: 10 players cost 6 % of a core); the orchestral
section's softer ±6 c spread (11.3) — one spread for all.
