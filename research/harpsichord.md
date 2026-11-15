# A large harpsichord with 16', 8', 8', 4' and a pedal instrument: research and plan

Status: 2026-11-15, research only; nothing is built. SI units. Pitches by MIDI-style names (C4 =
middle C); A4 = 415 Hz unless stated.

This file adds the registers, the 16' choir, the pedal instrument and the way onto the grand's
code to `research/harpsichord_clavinet.md`, which already holds the physics of one harpsichord
string: the scale of the 8' and 4' choirs, the plectrum, the release, the board, the published
models and their sources. What is there is not repeated here; "(hc §2.4)" points into it.

Tags: **[web]** read in a source of section 8; **[calc]** worked out here; **[memory]** from the
general literature as remembered, not checked for this file; **[guess]** a number chosen here to
start from, to be fitted or heard.

## 1. Short answer

- **The instrument:** a North German double of the Hass kind, the largest historical type and the
  only one with a 16' as a rule: 61 keys F1–F6, four choirs of strings (16', 8', 8', 4'), on two
  manuals. Under it a pedal instrument of 30 keys C2–F4 with its own strings (16', 8', 4') in its
  own box, as Bach's circle had them **[web, Adlung via baroquemusic; Neupert]**.
- **Registers as switches:** each choir on or off; a buff (harp) stop on the 16' and on one 8'; a
  lute (nasal) row on one 8'; the coupler. Section 5 gives them MIDI controls and names.
- **All of it on the grand's model:** each choir's string is one "key" of `Piano`
  (`src/piano.h`): its modes in two directions, its damper, its place on a bridge of a
  soundboard made by `tools/pianoboard.py`. A plectrum takes the hammer's place. The 16' has its
  own bridge on its own piece of soundboard, as Hass built it **[web]**. The pedal instrument is a
  second `Piano`.
- **Cost [calc]:** about 17 000 string modes per direction for the manuals and 4500 for the
  pedals, against the grand's 10 500. Only the plucked strings and those ringing along are
  awake (every string has a damper, there is no sustain pedal), so playing costs less than the
  grand: an estimated 5–15 % of a core.

## 2. The registers

### 2.1 What a register is

- A **choir** is a set of strings, one per key. A **register** is a row of jacks, one per key,
  plucking one choir at one place. A slide moves the row a millimetre or two sideways so its
  plectra miss the strings: off **[web, Cambridge Companion; memory]**.
- The foot names are the organ's: 8' is at pitch, 4' an octave higher, 16' an octave lower, 2' two
  octaves higher.
- Two registers may share a choir (the usual 8' and the lute row pluck the same strings at
  different places); they are then not used together **[web, hpschd.nu]**.
- With more registers on, each key lifts more jacks. On a real instrument they are staggered to
  pluck a few milliseconds apart, so the touch does not grow too heavy **[memory]**. Model: 3–8 ms
  between the rows, by key speed **[guess]**.

### 2.2 The Hass instruments

| Instrument | Choirs | Notes | Source |
|---|---|---|---|
| H. A. Hass 1723 | 8' 8' 8' 4' | | **[web, Wikipedia]** |
| H. A. Hass 1734 | 16' 8' 8' 4' | 2 manuals, nine feet long. From the bentside inwards: the 16' bridge on a separate, slightly raised piece of soundboard, the 8' hitch-pin rail, the 8' bridge, the 4' bridge | **[web, Wikipedia]** |
| H. A. Hass 1740 | 16' 8' 8' 4' 2' | 3 manuals with couplers, six rows of jacks, lute stop, harp (buff) stop to the 16' | **[web, Wikipedia]** |
| J. A. Hass about 1760 (Yale) | up to 5 choirs | 273.7 × 100 cm; c'' = 34.0 cm; the 16' bridge on its own soundboard, to the right of the 8' hitch-pin rail; buff stops to the 16' and the upper 8' | **[web, Yale]** |
| Neupert "Hass" (after 1734) | 16' 8' 8' 4' | lower manual 8', 4', 16'; upper 8'; coupler; lute (buff) to the 16' and to the upper 8'; 255 × 94 cm | **[web, Neupert]** |

- No source found gives the 16' string lengths. They are in Hubbard, *Three Centuries of
  Harpsichord Making*, and in the museums' drawings, not read here.
- The 2' of 1740 covers only part of the keyboard **[web]**. It is left out here: at F6 it would
  sound at 5.3 kHz with two modes below 12 kHz.

### 2.3 Other ways to change the sound

- **Buff (harp) stop:** leather or cloth pads against one choir beside the nut: a short, dry,
  lute-like note (hc §2.1) **[web]**.
- **Lute (nasal) row:** jacks plucking very near the nut: thin and nasal **[web, Britannica]**.
- **Peau de buffle:** a row with soft leather plectra (Taskin): rounder, softer, a little
  dynamic **[web, hpschd.nu Taskin; memory]**.
- **Coupler:** the lower manual's keys also work the upper manual's jacks **[web]**.
- **How the player reaches them:** hand stops on old instruments; knee levers on late French ones
  (Taskin: six); the English machine stop, one pedal taking off the front 8' and the 4' at once;
  a lid or Venetian swell on a pedal **[web, hpschd.nu]**. The revival instruments put every
  register on a pedal: Pleyel's Landowska model has 16', 8', 8', 4', a nasal, a buff and the
  coupler on seven pedals **[web, Basel museum; harpsichord.com]**.

### 2.4 The pedal instrument

- A second harpsichord lying under the first, played from an organ pedalboard. Adlung (a pupil's
  account from Bach's circle): two choirs of plain 8' strings and one of overspun 16' strings in
  its own case, with a door in the lid that could be opened to make it louder **[web, Adlung via
  baroquemusic]**.
- A reconstruction after Hildebrandt: manuals 16', 3 × 8', 4'; pedals 2 × 16', 3 × 8' **[web]**.
- Neupert's pedal harpsichord: C–f' (30 keys, C2–F4 here), 8', 4', 16' on three foot stops, a
  lute stop to the 16', a Venetian swell; 300 × 123 cm, 135 kg; leather plectra on the 8' and 16'
  **[web, Neupert]**.
- **Chosen (the user: "closest to what Bach would have had in his day, but perhaps with more
  registers if that is an option with modern times"):**
  - the historical core, on by default: Adlung's pedal instrument, one overspun 16' and two
    plain 8' choirs, 30 keys C2–F4, the door in its lid; under a Hass-kind double (16', 8', 8',
    4'), the largest instrument of Bach's Germany;
  - the modern additions, off by default: a 4' on the pedals and a buff to their 16' (Neupert),
    a lute row and buff stops on the manuals, every register on a switch or under the foot
    (Pleyel) instead of hand stops.
  - Plectra of quill and tuning at A = 415 in an unequal temperament for the historical
    setting; leather on the 16' and A = 440 as options.

## 3. The strings

### 3.1 The 8' and 4' choirs

As hc §2.2, with the scale shortened to Hass's c'' = 340 mm (Taskin 357 mm). At A = 415 that puts
iron at 885 MPa at C5 **[calc]**: still near its limit, so iron from about C3 up and brass below
stays. The two 8' rows pluck about 15 mm apart (hc §2.3).

### 3.2 The 16' choir

- **Not twice as long.** Twice the 8' would be 3.5 m at F1. The case is 2.74 m **[web, Yale]**,
  so the 16' is at most about 2.3 m in the bass **[guess]**, and its bridge has to stand nearer
  the tail and the bentside than the 8' bridge: Hass's separate board **[web]**.
- **A start for the scale [guess, calc]:** sounding F0 (20.6 Hz) to F5 (659 Hz).

| Key | sounds | f0 (Hz) | L (mm) | wire | T (N) | B | Z (kg/s) | modes < 8 kHz |
|---|---|---|---|---|---|---|---|---|
| F1 | F0 | 20.6 | 2300 | brass 1.0 mm | 60 | 1.5e-4 | 0.63 | 168 |
| C3 | C2 | 61.7 | 1900 | brass 0.60 mm | 132 | 1.3e-5 | 0.56 | 119 |
| C4 | C3 | 123.4 | 1150 | brass 0.45 mm | 109 | 1.4e-5 | 0.38 | 63 |
| C5 | C4 | 246.8 | 620 | iron 0.35 mm | 71 | 5.1e-5 | 0.23 | 31 |
| F6 | F5 | 659 | 250 | iron 0.28 mm | 52 | 1.7e-4 | 0.16 | 11 |

  (σ = ρ(2Lf)², T = σA, B = π³Ed⁴/(64TL²), Z = √(Tμ); brass 8500 kg/m³ and 100 GPa, iron 7850
  and 190 GPa.) The bass is slack (76 MPa at F1): thick plain brass there is stiff, B 1.5e-4, and
  sounds tubby. That is why Adlung's 16' was overspun **[web]** and why the revival makers overspun
  theirs **[web, hpschd.nu]**.
- **Plain or overspun** is a design switch for the lowest two octaves. Overspun (a 0.4 mm brass
  core under a copper wrap of the same mass) brings B at F1 down to about 2e-5 **[calc]**. The
  grand's wound-bass table is the same thing.
- **Its board:** a strip about 0.35 m wide along the bentside, raised, with its own bridge. It
  is lighter than the main board's bridge region and carries heavier strings, so the 16' strings
  lose to it faster: a 16' note is full at the start and shorter than its length suggests
  **[calc, to be checked on the model]**. A buff stop on it gives the pizzicato bass.

### 3.3 The pedal instrument's strings

16' sounding C1–F3 (30.9–165 Hz), overspun through at least the lowest octave, up to about 2.6 m
in a 3 m case; 8' as the manual's C2–F4; 4' as the manual's 4' for those keys **[guess]**.

### 3.4 Counts [calc]

| Choir | strings | modes per direction |
|---|---|---|
| 8' × 2 (to 12 kHz) | 122 | 8600 |
| 4' (to 12 kHz) | 61 | 2300 |
| 16' (to 8 kHz) | 61 | about 4000 |
| after-lengths behind the bridges | 244 | about 2500 |
| Pedals: 16' (8 kHz), 8', 4' (12 kHz) | 90 | about 4500 |

The grand has 10 500 per direction.

## 4. The model

### 4.1 What the grand's code already is

`Piano` takes a `PianoDesign`: per key the strings (length, tension, mass, stiffness, loss), the
exciter's place, a damper; a board's resonances with each key's share of them; the microphones.
A string's modes exchange momentum with the board's resonances every sample, the damped strings
load the board, sleeping strings cost nothing. `-inst electric-grand` and `-inst upright-piano`
are already built this way (research/piano_design.md).

### 4.2 What the harpsichord needs from it

1. **More keys.** A choir's string is its own "key" (its own length, wire, plucking point,
   bridge point, damper). 4 × 61 = 244 keys: `PianoDesign::kMaxKeys` 88 → 256. The pedal
   instrument (90) is a second `Piano`.
2. **A plectrum as a second exciter** beside the hammer (`PianoKey::exciter`): hc §2.4 and §4.
   - A spring of 1000–1500 N/m on a jack rising at 0.02–0.1 m/s; it catches the string, bends,
     and lets go when the string has slid to its tip (a force limit of 1–2 N in the treble, more
     in the bass; the limit is the voicing).
   - The let-go is the sound. The string leaves in both directions: up, and sideways by the
     plectrum's slope (about 1 mm sideways to 3 mm up **[web, Perng]**): the sideways swing the
     grand already has, here set off at full strength.
   - Key speed changes the string's level by about ±2 dB and the knock of the key on its bed by
     about 10 dB a dynamic step (hc §2.4).
   - Quill, Delrin, leather: stiffness, the limit, and how gradually the tip lets go (leather
     over 1–2 ms: rounder; quill at once: brighter) **[memory]**.
3. **The jack's way back** on key-up: the plectrum's back brushes the string (a tick), then the
   cloth damper lands: the grand's damper, engaging over 20–50 ms.
4. **A fixed pad** for the buff stops: the damper's per-mode loss, at 1–2 % of the length from
   the nut, switched on for a whole choir.
5. **The lengths behind the bridge** as short strings on the same bridge point, never damped
   (hc §2.2, §4): the board's ring of some seconds between 350 Hz and 1 kHz **[web, Välimäki]**.
   On the grand these were left out; here they come first. They are more "keys" of the same
   kind with no exciter, so `kMaxKeys` grows again or they get an array of their own.
6. **Registers off:** a choir whose register is off keeps its dampers down and is not plucked,
   but rings along a little through the bridge and loads the board, as on the instrument.

### 4.3 Soundboards

`tools/pianoboard.py` with other numbers (as `tools/uprightboard.py` does for the upright):
- **Main board:** spruce 2.4–5 mm, outline 2.6 × 0.95 m less the 16' strip, the 8' bridge, the 4'
  bridge, the 4' hitch-pin rail under the board, a cut-off bar, a few ribs beyond it (hc §2.6).
  Targets: point stiffness 7.8e4 N/m at the c' bridge point, first resonance near 110 Hz, about
  one resonance per 17 Hz **[web, Sankey; Savage]**.
- **16' board:** its own plate, clamped all round, its own bridge. In the data it is a second
  set of resonances that the 8' and 4' keys have no share in, so `Piano` needs nothing new.
  The two boards meet only through the air in the case.
- **The air in the case:** the box is closed below, so the board works against the enclosed
  air: a Helmholtz resonance at 37–100 Hz and the wedge's air modes **[web]** (hc §2.6). A few
  resonators driven by the boards' volume flow and pressing back on them. New, small.
- **The pedal instrument:** its own board and box. Its lid door (the swell) is a low-pass on its
  sound and a change of the box's air resonance **[guess]**.
- **Microphones:** the grand's crossed pair over the main board; the pedal instrument sounds
  from below and behind, with the floor's reflection **[guess]**.

### 4.4 What is new in code, and whose

- `src/piano.h/.cc` (this session's): `kMaxKeys`, the plectrum, the jack's return, the buff pad,
  the after-lengths, the air resonators. The grand must stay bit-identical.
- `src/harpsichord.h/.cc`: `harpsichordDesign()`, `pedalHarpsichordDesign()`.
- `tools/harpsiboard.py`, `src/harpsiboard_data.h`.
- `src/harpsiinst.h/.cc`: the model (two `Piano`s) and the player with the registers.

## 5. Playing it

### 5.1 The registers and their controls

| Register | on at start | proposed control |
|---|---|---|
| Lower manual 8' (back row) | yes | pad 1 (toggle) |
| Upper manual 8' (front row) | no | pad 2 |
| 4' | no | pad 3 |
| 16' | no | pad 4 |
| Lute (nasal) row on the upper 8' choir | no | pad 5 |
| Buff to the upper 8' | no | pad 6 |
| Buff to the 16' | no | pad 7 |
| Pedal instrument on the low keys | no | pad 8 |

- Pads on the MPK send notes on channel 10 (`src/live.cc`); here each press flips a register,
  and the change takes hold at the next pluck, as a slide does.
- The same switches as options for rendering (`-stops 8,8,4,16`, `-buff`, `-lute`) and as MIDI
  controllers (CC 80–87, on at 64 and over) for a sequencer.
- General MIDI's program 7 is `-inst harpsichord` with 8' + 8'. Named registrations as further
  instruments: `harpsichord-8` (one 8'), `harpsichord-full` (16' 8' 8' 4'), `harpsichord-lute`,
  `harpsichord-buff`.

### 5.2 Pedals

A harpsichord has no sustain pedal. Two uses for the foot, both real:
- **The sustain pedal (CC 64) as a register pedal**, as on a Pleyel or with the English machine
  stop: pressed, it adds the 16' and the 4' to whatever is drawn (the "grand jeu" under the
  foot), released it takes them off. Which registers it moves is a setting.
- **The pedal instrument for the bass:**
  - from a second keyboard or a sequencer: MIDI channel 2 plays the pedalboard;
  - from the MPK's 25 keys alone: with "pedal" on (pad 8), the lowest key held is also played on
    the pedal instrument at 16' + 8', as an organist's left foot would double the bass
    **[guess: to be tried]**; or the keyboard's lowest octave is the pedalboard.
  - Its swell door on the joystick or a knob.

### 5.3 The knobs

Voicing (the plectra's limit: softer and rounder to harder and brighter) on the brightness knob;
the stagger between rows; the tuning's temperament (Vallotti or equal; the program has 7-limit
tuning already); A = 415 or 440 (Neupert's transposing keyboard **[web]**).

## 6. Order of work

1. One 8' choir on the main board with the plectrum, the jack's return and damper: the tone of a
   single 8', against hc's checks (pluck nulls, Perng's release table, T60, ±2 dB of dynamics).
2. The second 8', the 4', the after-lengths, the air in the case; the buff and the lute row.
3. The 16' on its board, plain and overspun to compare.
4. The registers in `live` and the renderer; listening files of each registration.
5. The pedal instrument and the pedal controls.
6. Fit to recordings as the piano was (`tools/pianofit.py`): `~/Music` has no harpsichord track
   now; one recording each of a single 8', of 8' + 8' + 4' and of an instrument with a 16' would
   settle the string losses, the board and the voicing.

## 7. Open questions

- The 16' lengths and wire of a Hass (Hubbard; museum drawings). Section 3.2's table is a guess.
- Whether the 16' was plain or overspun on the Hass manuals (overspun on Adlung's pedals).
- The 16' board's thickness and barring.
- Plectrum numbers for leather and quill (only Delrin is measured, hc §2.4).
- How the pedal instrument's jacks were driven and how fast (rods or cords from the pedals
  **[web]**): its touch is heavier and slower than a manual's.
- What "pedaling for the bass strings" should mean at the MPK (section 5.2 offers both readings).

## 8. Sources

Besides those of `research/harpsichord_clavinet.md` §7:
- Wikipedia, "Hieronymus Albrecht Hass". https://en.wikipedia.org/wiki/Hieronymus_Albrecht_Hass
- Yale Collection of Musical Instruments, harpsichord by J. A. Hass, no. 4879.1960.
  https://music.yale.edu/browse-collection/harpsichord-48791960
- J. C. Neupert, harpsichord "Hass" and pedal harpsichord (specifications).
  https://www.jc-neupert.de/en/component/virtuemart/neue-instrumente/cembali/cembali-zweimanualig/neupert-harpsichord-hass-detail ;
  https://www.jc-neupert.de/en/component/virtuemart/neue-instrumente/pedalcembalo-1/neupert-pedal-harpsichord-incl-bench-detail
- "The Pedal-Harpsichord in Baroque Germany" (Adlung's description; the Hildebrandt
  reconstruction), read through search summaries only. https://baroquemusic.org/pedalharpsichord.html
- Historisches Museum Basel, Pleyel "Landowska" model 1930 (seven pedals).
  https://www.hmb.ch/en/museums/eguides/a/e/harpsichord-model-landowska-by-pleyel-lyon-et-cie-1930
- harpsichord.com, Pleyel listings (16' 8' 8' 4', nasal, buff, coupler on seven pedals).
  https://harpsichord.com/products/pleyel-gaveu-double-landowska-1962
- hpschd.nu technical library: stringing materials, overspun 16' strings, Taskin's knee levers,
  the machine stop. https://hpschd.nu/tech/str/id.html ; https://www.hpschd.nu/pdf/tech/taskin.pdf
- The Cambridge Companion to the Harpsichord, "History and construction of the harpsichord"
  (registers, stops; through search summaries).
  https://www.cambridge.org/core/books/cambridge-companion-to-the-harpsichord/history-and-construction-of-the-harpsichord/3A52A2F5D80CF9382932EF6E1024EA02
