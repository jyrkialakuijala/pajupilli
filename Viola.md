# Viola: review of the physical model

A review of `-inst viola` (GM 42) as a computational scientist and physicist would make it, with
the experiments behind each finding. Written 2026-11-19 against the working tree of that day
(uncommitted; the viola was still being worked on in another session, so numbers may have moved).
Background on the instrument is in `research/viola.md`; the user-facing description is in
`README.md` (search for `-inst viola`).

How each statement was obtained: **[measured]** a render or impulse response made for this
review; **[code]** read from the source; **[estimate]** my own calculation from the code's
constants, not checked by experiment.

## Verdict

A sound low-frequency physical model, with an empirically tuned bow, and a body that stops being
physical above roughly 1 kHz. The three main weaknesses:

1. the bowed string is not converged at 48 kHz;
2. playing louder does not make the tone brighter;
3. the body has almost no resonances above 2 kHz.

## The model in brief [code]

- **Strings** (`src/bass.cc`): four strings, each 64 modes in two directions, advanced by an
  exact rotation per sample with forces applied as velocity steps. C3 G3 D4 A4 on 37 cm, pure
  fifths from A = 440 Hz, tensions 55, 52, 58, 76 N. All end on one bridge (sympathetic ringing);
  a short length behind the bridge (6 modes) goes to an 18 g tailpiece.
- **Bow:** friction 0.9 sticking, falling to 0.35 slipping, solved each sample against the
  string's own response with hysteresis (Friedlander). Hair 10 mm wide, the string's twist as a
  lumped spring, a little noise in the grip. Bow force between limits measured on the model.
- **Bridge:** a 1.5 g mass on springs, rocking at 2.1 kHz, up and down at 4 kHz.
- **Body** (`src/bassbody.cc`, designed by `tools/bassbody.py -viola`): top and back as 16 x 8
  grids of masses (410 mm outline), a rigid sound post, the air as 16 cells along the body in two
  halves side by side with one f-hole each. Three time steps per sample.
- **What is heard:** the displaced volume's acceleration (far sound), three directional moments
  (into the room's reflections), an f-hole microphone, the top by the bridge, a bridge pickup.

## What holds up

- **The body's time stepping is converged [measured].** A private build with the viola at
  96 kHz gives the same body resonances as 48 kHz up to 2.4 kHz, within 3 Hz and 0.3 dB
  (223, 345, 403, 488, 573, 725, 1017, 1115, 1554, 1899, 2411 Hz).
- **The string scheme [code]:** exact modal rotation with the bridge motion as a boundary term
  is standard and stable; the friction solve uses the same one-step response the scheme applies,
  so it is consistent.
- **Signature resonances [measured]:** air resonance 224 Hz (Q 19), main wood resonance 345 Hz
  (Q 26), then 403, 488, 573 Hz: in a real viola's range.
- **Tuning [measured]:** most notes within 7 cents; clarity 0.90 or better.
- **Stability and cost [measured]:** 10 s fuzz, no resets; about 22 % of a core.
- **Pizzicato decay [measured]:** 30 to 35 dB in the first second.

## Findings, most important first

### 1. The bowed string is not converged in sample rate [measured]

At 96 kHz, with the bow's bandwidth held at the same frequency (`PJ_BOW_WMAX=0.45`):

| Note | 48 kHz | 96 kHz | Level 96 against 48 |
|---|---|---|---|
| C3 | +0 cents | +7 cents | +1.0 dB |
| A3 | +1 | +9 | +0.8 dB |
| E4 | -2 | +6 | -4.4 dB |
| A4 (open) | -2 | +1 | +1.5 dB |
| E5 | -1 | +3 | +1.3 dB |

Single harmonics differ by up to +-10 dB (E4: most harmonics 5 to 8 dB stronger relative to the
fundamental at 96 kHz). So the tuning and the per-note balance at 48 kHz are partly properties of
the step size. Some of the harmonic scatter may be the stick-slip motion's own sensitivity to
small changes; the pitch shift is systematic.

### 2. Louder does not get brighter [measured]

Bowed C4 by key velocity:

| Velocity | Level | Centroid | Share above 2 kHz | above 4 kHz |
|---|---|---|---|---|
| 25 | -48.0 dB | 1601 Hz | -5.6 dB | -25.8 dB |
| 50 | -41.6 | 1537 | -6.5 | -25.5 |
| 75 | -36.7 | 1459 | -7.8 | -23.4 |
| 100 | -32.3 | 1448 | -7.9 | -22.5 |
| 125 | -28.8 | 1459 | -8.0 | -22.0 |

The level rises 19 dB and the centroid falls. On a real bowed string more bow force sharpens
the travelling corner and adds high harmonics; a louder note is also played nearer the bridge.
Here the top of the spectrum is fixed by the cut-off of finding 3.

### 3. The bow's bandwidth is a number in radians per sample [code]

`BassSpec::bowWmax` (0.9 for the viola): the bow drives string modes that turn up to about that
much per sample, 6.9 kHz at 48 kHz, and falls back to 0.5 (3.8 kHz) for fundamentals of 400 to
800 Hz and above, because driven further those notes played flat. It stands in for what rounds
the corner on a real string (bending stiffness, the hair's width, the bow force), and it would
move with the sample rate. It is also why the top of the A string is dull **[measured]**: on A5
the harmonics above the seventh fall to -34, -44, -51, -64 dB.

### 4. The string looks too heavy to the bow [estimate]

The friction solve uses the string's one-sample response, `dt x (2 / (mu L)) x sum of shape^2`.
With the modes the bow drives, that comes to roughly 0.3 to 0.6 of a real string's wave
admittance 1 / (2 sqrt(T mu)). The bow-force limits in `Bass::bowForce` were measured on the
model, which makes up for it, but bow forces cannot be compared with measured ones.

### 5. The plate grid is too coarse above about 1 kHz [estimate]

Grid points are 26 mm apart along the grain and 30 mm across. For a 3 mm spruce plate (flat,
without the arching's help), six points per bending wavelength hold only up to a few hundred Hz
across the grain and about 1 kHz along it.

### 6. The body is almost featureless above 2 kHz [measured]

Peaks in the body's response to a force on the bridge foot:

| Band | Peaks | One per |
|---|---|---|
| 200 - 1000 Hz | 8 | 100 Hz |
| 1000 - 2000 Hz | 4 | 250 Hz |
| 2000 - 3000 Hz | 1 | 1000 Hz |
| 3000 - 5000 Hz | 1 | 2000 Hz |
| 5000 - 8000 Hz | 0 | |

A real viola has dense resonances through several kHz. Two causes: the coarse grid has too few
modes, and the wood's damping grows with the frequency squared (rate 45 /s plus 6e-6 s x the
stiffness force's rate of change), which by my estimate gives Q about 30 at 345 Hz, 13 at 2 kHz
and 5 at 5 kHz, where real wood's loss factor stays roughly constant.

### 7. One air cell carries both f-holes' flow [measured, code]

The f-hole plugs open from the single 26 mm slice of air at the bridge. That slice bounced on
them at 4.93 kHz with Q over 1000: a metallic shine on every note with an overtone there (D#4,
E4, F4, B4, F5; reported by Jyrki). Damped on 2026-11-19 with air losses that grow with
frequency (`BodyData::airHf`, `PJ_AIR_HF`; Q 15 at 4.9 kHz, A0 and the main resonance
unchanged; the overtone 12 to 24 dB weaker). The cause remains: real f-holes span about three
cells' length.

### 8. The parts are coupled with a one-sample lag [code]

Strings, bridge and body are advanced in turn, each using the others' previous values. Stable in
the fuzz runs, but not guaranteed to conserve energy at high frequencies (at 5 kHz one sample is
0.65 rad of phase). Not tested directly.

### 9. Radiation is a mix of microphones, not a field [code, measured]

A large share of the viola's output is the top's motion next to the bridge (its level x 3.5
against the cello's), sent out alike in all directions. The directional parts added on
2026-11-19 (`BassBody::dipole`, `Room::setAxes`) apply only to the far-field share, so in the
hall they change only a few overtones per note by up to about 10 dB left against right. The
moments describe the radiation properly only up to where the body is as large as the wavelength
(about 700 Hz); above, they are held at constant size as a stand-in for the real lobes.

### 10. Borrowed and assumed constants [code]

- string losses: the cello's strings of the same rank;
- velocity curve: the cello's; level: set against the cello;
- string angles over the bridge, fingerboard contact spring: the double bass's;
- stretch stiffness: assumed 100 x the tension.

### 11. Evenness [measured]

K-weighted loudness per note against the range's median: standard deviation 2.8 dB over C3 to
E6. Outliers: open A4 -5 dB, C#5 to E5 -4 to -6 dB, D#3 and E3 +6 to +7 dB. The bow-evening
table (`src/violabowlevels_data.h`) looked out of date against the current body. C5 and C#5
played 12 and 18 cents flat.

**The sound post [measured]:** moving it further behind the treble foot raises the A string
(Jyrki's suggestion). Loudness against the median:

| Post behind the foot | A4 (open) | G4 | A#4 | Note |
|---|---|---|---|---|
| 7 mm (0.26 of a node, the design) | -5.2 dB | -0.3 dB | -3.1 dB | |
| 12 mm (0.45) | -3.9 | 0.0 | -2.1 | |
| 17 mm (0.65) | -2.4 | -0.1 | -0.3 | A0 220 Hz, main 349 Hz, same Q |
| 20 mm (0.8) | -1.8 | -1.1 | +0.6 | F5 +6 dB |

About 17 mm is the best of these. It is further than a real viola's 9 to 12 mm; my reading is
that on a 26 mm grid a post 7 mm from the foot pins the foot almost rigidly. Not made the
default; `PJ_POST_BEHIND` tries it. The rest of the open A's weakness is the body: 440 Hz lies
in the dip between the resonances at 403 and 488 Hz.

## What to fix first

1. **Brightness with dynamics (findings 2, 3):** replace the fixed cut-off with a corner rounding
   that depends on bow force and string stiffness, defined in Hz. The most audible gain.
2. **Convergence (finding 1):** run string and bow at 96 kHz internally, or correct the pitch
   offset explicitly, so that tuning does not depend on the step size.
3. **The body above 1 kHz (findings 5, 6):** keep the grid for the low resonances and add a
   statistical bank of modes with constant Q for the upper range, rather than refining the grid.
4. **F-holes over three cells (finding 7).**
5. **The post at about 17 mm and a fresh bow-evening table (finding 11).**

## Fixed on 2026-11-19, after the review

The five items of "What to fix first", all viola-only through new fields of `BassSpec` and
`BodyData` whose defaults leave the bass, the cello and the violin as they were (their A3 renders
are unchanged to four digits). Measured, not judged by ear.

1. **Louder is brighter (findings 2, 3).** `BassSpec::bowBright` (0.75): the bow drives the
   string's modes up to `bowWmax` x (the bow's force against a forte stroke's, 40 cm/s at the
   same place) to that power, and `bowWmax` is now a frequency (what it was at 48 kHz) whatever
   the step. On middle C the bow reaches 2.1 kHz at pp, 3.3 at p, 5.2 at mf, 7.5 kHz from f on.

   | Velocity | Level | 2 - 4 kHz share | 4 - 8 kHz share |
   |---|---|---|---|
   | 25 | -44.3 dB | -19.2 dB | -61.9 dB |
   | 50 | -38.1 | -5.5 | -50.6 |
   | 75 | -34.6 | -8.7 | -25.5 |
   | 100 | -30.2 | -8.9 | -24.0 |
   | 125 | -26.6 | -10.3 | -24.1 |

   From f to ff nothing more is gained: the reach stops at 0.5 rad a step (7.6 kHz at two steps
   a sample). The centroid still falls a little from p to ff (1565 to 1269 Hz): the band below
   2 kHz grows faster than the one above. `PJ_VIOLA_BRIGHT`.
2. **Convergence (finding 1).** `BassSpec::oversample` (2): the strings, the bow, the tailpiece
   and the bridge take two steps a sample, the body one with the feet's mean force
   (`Bass::stepStrings`). Tuning of five notes at 1, 2 and 4 steps: C3 +1 / +5 / +8 cents, A3
   +4 / +7 / +9, E4 -5 / +4 / +5, A4 -3 / 0 / +2, E5 0 / +2 / +4: two steps are within 3 cents
   of four. The high notes' bow no longer falls back to 3.8 kHz. Cost: 29 % of a core (22 %
   before). `PJ_VIOLA_OS`.
3. **The body above 1 kHz (findings 5, 6).** `src/bodybank.h`, `BassSpec::bankGain` (14): a bank
   of about 100 resonances from 900 Hz to 9 kHz, one per 80 Hz, each a little off its place, all
   of Q 35, driven by the bridge feet's sum and difference in its own shares, each with its own
   share of the sound heard all round and of the three directional parts. Its level against the
   far sound: about -10 dB at 2 kHz falling to -24 dB at 7 kHz, re the main resonance. A
   stand-in, not physics: fixed resonances, and the bridge does not feel them. In the full mix
   the count of separate peaks rose only from 2 to 5 between 2 and 5 kHz: the resonances overlap
   (their width is about their spacing, as on a real body), and the top-by-the-bridge share of
   the sound (finding 9) lies over them. `PJ_VIOLA_BANK`.
4. **F-holes over three cells (finding 7).** `BodyData::holeCells` (3): each f-hole takes its air
   from the bridge's cell and its two neighbours (a quarter, a half, a quarter) and feels their
   pressures in the same shares. The 4.9 kHz resonance is gone without the extra air loss (F5's
   share there -23.6 dB with `PJ_AIR_HF=0`, -10.4 dB with one cell), so that loss was lowered to
   0.7e-6 s. The air resonance moved from 224 to 225-229 Hz. `PJ_VIOLA_HOLES`.
5. **The post and the bow-evening table (finding 11).** The post 17 mm behind the treble foot
   (0.65 of a node, set in `kViolaBodyData`, not in the generated header), and
   `src/violabowlevels_data.h` regenerated. Loudness against the range's median: standard
   deviation 1.8 dB (2.8 before); open A4 -1.3 dB, G4 -0.6, A#4 -1.3 (A4 was -5.2). Main wood
   resonance now 350 Hz (Q 26), air resonance 225 Hz (Q 20).

Also checked after the fixes: 10 s fuzz with two seeds, no resets; D#4 to F#4 keep their
fundamental (-5.1 dB against the second harmonic at worst; the test's limit is -15); a pizzicato
falls 30 dB in 0.8 s; the earlier shine notes' share at 4.85 to 4.99 kHz is -43 dB or less (F5
-27 dB).

**A correction to finding 11:** the notes more than 10 cents off (D#3 +19, A#3 +16, F4 +15,
C#5 -14) are not errors. They are the player's ring intonation (`PJ_RING_TUNE`, `stopLength` in
`src/bassinst.cc`): a stop moved by up to 15 cents so that the note rings with an open string.
With it off they play +8, +2, +1 and 0 cents.

**Still open:** findings 4 (the string's response to the bow), 8 (one-sample lag between the
parts), 9 (the close parts of the sound go out alike all round), 10 (borrowed constants); the
brightness from f to ff (needs three steps a sample or a different limit); none of it heard or
compared with a recording.

## Changed on 2026-11-19 (not part of the review, but they affect its numbers)

- **Two f-holes:** the air in two halves side by side, each with its own plug, and cross flow
  (`PJ_AIR_SIDES=0`: the single column of before). Main resonances unchanged.
- **Directional radiation:** three moments of the displaced volume into each early reflection
  and the reverberant field (`PJ_DIPOLE=0`: off).
- **Air losses growing with frequency** (finding 7).
- **Pizzicato softer than the bow:** the pull is now a share of the sounding length, rising with
  velocity squared, smaller on strings stopped high up and on the higher strings (`PJ_PIZZ_PP`,
  `PJ_PIZZ_FF`, `PJ_PIZZ_UP`). Measured 5 to 9 dB below the same key bowed on viola and cello
  (cello A string about 13 dB below); before, up to 20 dB above. Calibrated, not derived from
  measurements of players.

## How the experiments were made

- **Convergence:** a copy of `src/` with the viola's rate set to 96000 in the instrument table,
  built with the same flags; `-inst viola -note N -secs 3` at both rates with `PJ_NOCAL=1`;
  harmonic levels from a windowed FFT of 1.0 to 2.6 s.
- **Body response:** `PJ_BASS_SYMP=0 PJ_IMPOUT=1 PJ_IMPDUMP=file ./pajupilli -inst viola -imp 60`
  (a force on the bridge's bass foot, bypassing the bridge), peaks of the zero-padded spectrum.
- **Loudness per note:** `-note N -secs 2`, K-weighted (`tools/levels.py: k_weight`) over
  0.8 to 1.6 s.
- **Brightness against velocity:** `-seq "on 60 V wait 2.5 off 60 wait 0.3"`, spectrum of
  1.0 to 2.3 s.
- **Pizzicato against bow:** `-seq` with the key held 0.05 s (plucked) or 1.5 s (bowed); the
  loudest 100 ms window, K-weighted.
- None of it was judged by ear or compared with a recording.
