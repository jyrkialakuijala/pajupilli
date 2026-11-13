# Drum kits from recordings: physical models fitted to what is heard

Status: requirements and research plan only (2026-11-13). Nothing here is implemented yet.

The project's drums should be built the way its winds and synths are: as physical systems whose
sound comes from their dynamics, not from stored samples. What is new is where the parameters come
from. The music in `~/Music` is measured, and the models are fitted to it: each drum's overtones,
how fast they decay, how a hard hit bends the pitch, the snare's rattle, a cymbal's wash. The
recordings are only measured. No audio from them goes into the instruments or the repository.

## 1. Goals

1. **Physical models.** Each drum and cymbal is a vibrating structure, struck: a membrane, a
   shell, a plate. Its sound follows from its parameters (size, tension, damping, material,
   where and how hard it is hit), so velocity, strike position and damping by the hand or pedal
   work as on the real instrument.
2. **Kits fitted to recordings.** A few kits, each fitted to the drums of particular tracks: a rock
   kit, an R&B kit with programmed drums, a jazz kit with brushes, Latin percussion, and timpani.
3. **Playable with ordinary MIDI.** The drums sit on the usual General MIDI keys (35/36 kick, 38
   snare, 42 closed hi-hat, 46 open hi-hat, 49 crash, 51 ride, ...), so a standard drum track
   plays them. The brush kit follows GM2's Brush Set: 38 brush tap, 39 brush slap, 40 brush swirl
   (sounding for as long as the key is held). The drum channel is 10 (General MIDI); channel 16, a
   common older practice, can be chosen instead (an option). The MPK's pads (channel 10) play
   them live; the ensemble, room and live loop work as for the other instruments.
4. **Real time.** A full kit (several drums ringing at once, cymbals included) on one core.

**Priority: the jazz kit with brushes**, to sound really nice and organic. It comes first; the rock
kit's separation is done and kept for later (section 7).

## 2. Non-goals

- Sample playback, or resynthesis of the recorded hits (additive or otherwise).
- Reproducing a record's whole production. Its compression, EQ and reverb are measured as part
  of the sound and noted, but the kit is a model of the drums, played in this project's room.

## 3. Sources in `~/Music` (folder `wavefield`, the same 132 tracks as m1–m3, p1–p3)

| Kit | Tracks | Expected |
|---|---|---|
| Rock | *Are You Gonna Go My Way* (Lenny Kravitz) | A dry, close-miked kit: kick, snare, hi-hat, toms, crash |
| R&B, programmed | *Say My Name* (Destiny's Child) | Drum-machine sounds: a reference for the electronic kit |
| Electronic, house-jazz | St Germain: *So Flute*, *Latin Note*, *Sure Thing*, *Land of…* | Programmed kick and hats, acoustic percussion |
| Jazz, brushes | Jamie Cullum, Ella Fitzgerald, the Love Ballads tracks | Brushes on the snare, ride cymbal, soft kick |
| Latin and Brazilian | Putumayo Salsa, Cuba, Bossa Nova, Brazilian Lounge; *The Obvious Child* (Paul Simon) | Congas, timbales, cowbell, surdo, shaker, claves |
| Timpani | *O Fortuna* (Orff); the Mozart symphonies | Tuned kettle drums, rolls |

## 4. Method

### 4.1 Separate the drums

- A source-separation model (Demucs, htdemucs: open source, runs on the CPU with torch) splits each
  track into drums, bass, vocals and the rest. It needs `pip install demucs` and a one-time
  download of its weights (~80 MB). It has to run slowly on this shared machine: one track at a
  time, few threads.
- Classical tracks have no drum stem to separate: timpani are found directly in the orchestral
  mix, at the moments when they play alone or loudest.
- The separated stems are copyrighted audio: they stay in the scratchpad, never in the repo.

### 4.2 Find and sort the hits

- Onsets on the drum stem (spectral flux, with sub-band envelopes for overlapping hits).
- Each hit described by its spectrum, decay, pitch track and noise share, then grouped into
  kick, snare, toms, closed and open hi-hat, ride, crash (clustering, checked by listening).
- Per group, the cleanest hits (alone, not masked by others) aligned and averaged. That cancels
  most of the bleed from the other instruments, and leaves velocity layers when hits are sorted by
  loudness.

### 4.3 Measure each instrument

- **Modes:** the frequencies, decay times and amplitudes of the partials, from high-resolution
  methods (ESPRIT or matrix pencil) over each averaged hit.
- **Pitch glide:** a membrane hit hard starts sharp and falls as its tension relaxes. That is
  measured as the fundamental's pitch against time at each velocity.
- **Noise:** the snare wires' rattle, brushes, the cymbals' wash: its spectrum and envelope.
- **Strike:** the attack's spectrum gives the contact time (a soft mallet or brush, a hard stick).

### 4.4 The models

- **Membranes (kick, snare, toms, congas, timpani):** a circular membrane under tension. Its modes
  are Bessel-function ratios, moved by the air it pushes and by the shell (Rossing, *Science of
  Percussion Instruments*; Fletcher & Rossing, *The Physics of Musical Instruments*).
  - Real time: a modal bank, a few dozen damped resonators, struck by a contact force from a
    mallet model (a nonlinear spring, Hunt–Crossley), excited at the strike position.
  - Tension modulation: the pitch rises with the amplitude (Avanzini & Rocchesso; Bilbao,
    *Numerical Sound Synthesis*), which gives the hard hit's downward glide.
  - Two heads coupled through the air inside the shell (batter and resonant head), which gives
    the kick and toms their body.
  - Timpani: the kettle's air shifts the modes to near-harmonic ratios. That is why they have a
    clear pitch.
- **Snare wires:** a set of wires against the resonant head, a collision model. They rattle when
  the head moves enough, so a soft hit sounds different from a hard one.
- **Cymbals and hi-hats:** a thin plate with a nonlinear coupling of its modes (von Kármán). A hard
  strike carries the energy upwards into a bright wash, the shimmer that comes after the hit
  (Chaigne, Touzé & Thomas; Bilbao). A modal or finite-difference plate, as the cost allows.
  - The hi-hat: two plates pressed together by the pedal, from closed (a short chick) to open.
- **Brushes (the first kit):** a brush is a fan of thin steel wires (~0.3 mm, a hundred or more).
  What makes it organic is that nothing in it is a single event:
  - **Tap:** the wires land over a few milliseconds, not at once, each where it lands on the head
    (a spread attack, a "splat" instead of a click); the fan's springiness lets them bounce.
  - **Sweep (swirl):** the wire tips are dragged over the coated head's rough texture: each tip
    excites the head with micro-impacts from the coating's grain (stick-slip friction), at a rate
    set by the speed, a force set by the pressure. The hand moves in circles: the contact point
    travels over the head, and each mode is excited by its own shape at that point (Bessel
    functions in r and θ), so the colour of the swish moves with the hand.
  - **The snare wires** answer both, rattling more the more the head moves.
  - **Two brushes, "stirring the stew":** the usual brush playing is two independent hands on one
    snare. One stirs continuous circles (often one turn per beat or two), the other taps and
    slaps the accents (often on 2 and 4). Both excite the same head and snare wires at once. A
    tap where the stirring brush lies is damped by it a little. The circle's tempo follows the
    taps' spacing, or MIDI clock when a sequencer sends it.
  - **The strokes**, each a path of a brush over the head (where, how fast, how hard), so they
    can overlap and blend:
    - *circles:* one brush stirring, or both, often counter-rotating: two contact points moving
      opposite ways give a fuller, more even rush, with less of a single circle's swell;
    - *sweep:* a straight horizontal drag across the head, legato (the length of the key);
    - *sweep and tap:* a sweep that ends by lifting into a vertical tap: the swish then the accent;
    - *tap, slap:* vertical strokes (a slap: the fan flat, more wires at once, louder and darker);
    - *ghost notes:* very light taps, more texture than notes;
    - *rolls:* fast alternating taps and buzzes (the fan bouncing), as a sustained texture.
    Legato and staccato are made from these: sweeps that join, or short taps with silence.
  - **On the keys:** GM2's Brush Set has 38 tap, 39 slap and 40 swirl; the other strokes go on
    nearby free keys (to be fixed with the first kit), so a GM drum track still plays.
  - **The stroke's shape:** speed and pressure through a circle, in time with the music: a swirl
    held on a key goes on circling (one turn per beat or two), the key's velocity and aftertouch
    its pressure.
  - Measured from the recordings: the swish's spectrum and its modulation over a circle, the
    taps' spread and spectrum, the head's modes under the brush (the head itself, as with sticks).
- **Hand percussion:** a membrane struck by a hand (slap, open, muted); cowbell and claves as
  short metal and wood modal sets.
- **Electronic kit (the drum machine):** the analog circuits, as with the synths. A bridged-T
  resonator pinged for the kick, noise through filters for the snare and hats.

### 4.5 Fitting

- Analysis by synthesis: each model's parameters are fitted so that its sound matches the averaged
  hit. The loss compares spectrograms on a perceptual frequency scale, plus the decay times and
  the pitch glide.
- Parameters stay physical (size, tension, damping, the mallet's hardness), so the fit can be
  checked against what a real drum of that kind would be. Velocity layers fit the same model at
  several strike strengths: only the strike may change between them.

### 4.6 Checks

- The fitted model against the averaged hits (spectrogram distance, decay times within 10 %,
  pitch glide within 10 c), and by listening, side by side.
- A test per kit in `tools/test.py`: each drum's modes and decays against the stored fit
  parameters (not the audio), NaN recovery, fuzz, real time.

## 5. Deliverables, in order

1. This document, extended with the measurements per kit.
2. `tools/drums.py`: separation, hit finding and sorting, measurement, fitting.
3. The first kit, rock (kick, snare, hi-hat, a tom, a crash), as an instrument with the GM map.
4. The jazz, Latin, electronic kits and the timpani.

## 6. Open questions

- How much of each record's compression and reverb can be removed from the measurements? A
  close-miked dry kit (the Kravitz track) is the easiest start.
- Cymbal models are costly: a plate with enough modes to shimmer may not fit in real time. A
  modal approximation with a fitted noise wash may be needed.
- How clean are separated brushes and hand percussion? They may need hand-picked moments where
  they play alone.

## 7. Progress

- 2026-11-13: Demucs (htdemucs) installed in `~/.venvs/demucs` (outside the repo). The rock track
  separated in 78 s on 4 threads. The jazz tracks are being separated next (16 tracks: Cullum,
  Ella Fitzgerald, Cyrus Chestnut, Gerry Mulligan, Louis Armstrong, Ray Charles, the Love Ballads,
  *Autumn Leaves*, *Fly Me to the Moon*) to find where brushes play.
