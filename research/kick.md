# The jazz bass drum: research and the model built (-inst jazz-kick)

Status: 2026-11-15. Built: `src/kick.h/.cc` on the shared head model (`src/membrane.*`: Membrane,
DrumAir). Marks: **[paper]**, **[web]** a source below; **[calc]** worked out here; **[memory]**
general knowledge, to be checked; **[design]** a choice; **[meas]** measured on the model.

## 1. The drum

- **The bebop kick:** 18" × 14" (some players 20"), both heads on, **no port**, little or no muffling,
  tuned up for a pitched, open "boom". A felt beater **[memory]**.
- **Heads:** single-ply, unported resonant heads (7.5–10 mil), sold for jazz and bop
  **[web, retail listings]**. A coated single-ply batter (10 mil: σ ≈ 0.26 kg/m², as the snare's
  **[jd]**).
- **Feathering:** quarter notes played very softly, felt more than heard. In the bushgrafts files the
  kick's velocity has a median of 44 (10–90 %: 22–71) [meas, this project].

## 2. Modes: two heads on a large cavity [calc, paper]

- **Coupling:** two-headed drums couple through the enclosed air or the shell, which gives mode
  pairs **[paper, Fletcher & Rossing; Rossing PT]**.
- **The bass drum of Fletcher and Bassett:** a 0.90 m drum, heads 0.41 m apart, both tuned near
  40 Hz. At the lowest frequencies the heads move in phase **[paper, FB]**.
- **The cavity is a stiff spring for a loose, large head [calc].** Take an 18" × 14" kick (V = 0.058 m³)
  with T = 690 N/m.
  - The cavity's stiffness on the (0,1)'s volume: k_air = ρc² A_eff²/V ≈ 12 kN/m, with
    A_eff = ∫J₀ dA = 0.071 m².
  - The head's own (0,1) stiffness is only ≈ 3.4 kN/m (M ω²; M = σπR² × 0.27 = 0.0115 kg).
  - So the out-of-phase (0,1), whose heads squeeze the air, sits far above, about 3× the in-phase
    one. The in-phase one, both heads carrying the air with the outside air's mass on them, is the
    drum's note and sits below each head's own (0,1).
  - On the model [meas]: the note is 70 Hz; the out-of-phase member 248 Hz; between them,
    diameter modes (157 Hz, −16 dB).
- **Tuning (18", 690 N/m on both heads):**
  - each head alone, unloaded: (0,1) = 86 Hz;
  - the drum's note: 70.2 Hz (C♯2) [meas].
  - The 20" at 640 N/m: 59.6 Hz [meas].
  - Jazz kicks sit roughly at 55–80 Hz **[memory]**. `PJ_KICK_TENSION` (batter) and `PJ_KICK_RESO`
    (the resonant head's tension, as a ratio) retune it.

## 3. The beater [calc, design]

- **The felt:** a soft, steep contact (Hunt–Crossley), F = K pen^2.3 (1 + 0.6 s/m × closing speed),
  with K = 1.5·10⁸ N/m^2.3. At 2 mm in, that is 62 N and 71 kN/m: a 2–3 ms contact on a rigid head,
  longer on the yielding one.
  - Contact time drives loudness; the best contact is about half a period of the mode to be driven
    **[web, Liljencrants]**. The felt's softness keeps the clicks above ~1 kHz down.
- **Mass at the felt:** 0.07 kg (the ball, the shaft and the pedal, referred to the felt). It lands
  0.12 R below the middle (key 36), or 0.22 R (key 35: more of the diameter modes, more ring).
  - The felt's patch, 2.5 cm across, averages the shapes, so the highest modes are taken down.
- **One key, one stroke.** The beater is thrown from just over the head **where the head is now**.
  After the contact, the pedal's spring (6 Hz) takes it back, and it never comes down again by
  itself.
  - A stroke onto a ringing drum meets its motion. A feathered stroke into a ringing note lands from
    2.8 dB under to 2.1 dB over the two strokes' summed energy, depending on the phase [meas].
- **Speed from velocity:** 4.5 m/s × (v/127)^1.25 × (0.75 + 0.5 × dynamics).
  - Feathered keys (20–70) give 0.27–2 m/s; a bomb (127) 4.5 m/s.
  - The level from velocity 16 to 127 is 29.6–32.6 dB [meas], on General MIDI's curve (TiMidity's
    median: 30 dB; `tools/velcurve.py`).
- **Buried** (the heel down, the beater left on the head): the hold (the sustain pedal, CC 64 on the
  drum channel; `PJ_KICK_BURY=1`: every stroke).
  - The foot leans in with 12 N, and the leg's damping (60 N s/m) keeps the beater from bouncing on
    its felt.
  - The pinned head sounds higher (88 Hz) and dies fast: 14 dB under the open stroke at 0.2 s, its
    peak 9.5 dB lower [meas].

## 4. Glide, decay, radiation

- **Glide:** Berger's mean tension (the film's E h/(1 − ν²) = 1.1 MN/m on the batter, 0.8 on the
  resonant head), as `toms.md` §2.3 [calc] [meas]:

| | 18" | 20" |
|---|---|---|
| velocity 127 | +79–81 c, falling within ~0.3 s | +155 c |
| velocity 90 | +19 c | |
| feathered | 0 c | |

- **Decay:** T60 of the note is 0.83 s (18") and 0.97 s (20"). Loss shares: 0.018 critical on the
  (0,1)s, 0.012 on the others, rising above 1.5 kHz. Open jazz kicks ring about this long
  **[memory, to be fitted]**.
- **Radiation:** two microphones 0.6 m in front of the resonant head (one on its axis, one 0.25 m to
  the side).
  - Each head's modes are heard by their shapes: patches over the head, each with its distance and
    delay.
  - The resonant head is baffled by the drum (ρ/2π). The batter is heard round the shell, as half
    that, from 0.36 m further.
  - So the in-phase note partly cancels at low frequency (a dipole) and the out-of-phase one adds
    (a monopole), as on a real kick.
  - The felt's own thump above the batter's modes (1.4 kHz) is added: the contact force on the film,
    high-passed.
- **Cost:** 430 ns per sample under random playing, 2.1 % of a core [meas]. The batter has modes to
  1.4 kHz, the resonant head to 700 Hz (only its m = 0 modes are driven, by the air).

## 5. Level

- **Trim:** −4.3 dB (18"), −5.4 dB (20"). That is the project's common −28.2 LUFS at velocity 90
  (`tools/levels.py`), in `src/levels_data.h`.
- **Checked against a GM player.** Five bushgrafts files, kick alone against the band (all non-drum
  channels), LAeq, A-weighted:
  - TiMidity with FluidR3: −36.8 / −21.6 / −31.2 / −30.5 / −25.5 dB;
  - the taiko placeholder: −42.7 / −32.5 / −40.5 / −29.3 / −24.0 dB.
  - On the same notes (the first 90 s of each), the trimmed jazz kick is +1.1 to +6.5 dB over the
    taiko. That puts it on average 1.4 dB from TiMidity's level (−5 to +8 dB per file) [meas].

## 6. The kinds, and which one a piece gets (built 2026-11-28)

The user: "have all jazz kicks and choose the most suitable for every setup; develop rules which
decide which jazz kick you keep". `src/kick.cc` `kKicks`, one instrument each; keys 35 and 36 play
the piece's kick (`chosenKick()`, `src/drumkit.cc`). All share the 18"'s calibration (one key map,
one gain); their own levels are `src/levels_data.h`'s trims, set to the 18"'s loudness (K-weighted,
loudest 400 ms, v64/96/120).

| Kind | Size | Note [meas] | -30 dB [meas] | Damping, heads, beater |
|---|---|---|---|---|
| `jazz-kick-16` | 16" x 14" | 86 Hz | 0.18 s | open; the resonant head 5 % up |
| `jazz-kick` | 18" x 14" | 70 Hz | 0.30 s | open (as before) |
| `jazz-kick-20` | 20" x 14" | 60 Hz | 0.40 s | open (as before) |
| `jazz-kick-22` | 22" x 14" | 52 Hz | 0.30 s | a felt strip on each head (zeta x 1.6–1.7) |
| `studio-kick` | 22" x 16" | 48 Hz | 0.11 s | ported (vent 40 Hz), a pillow (zeta x 5–8), hard felt at the middle, the mic at the port, click x 2 |

The rule (`chooseKick`, printed as "the bass drum: ..."), from the director's reading and the file:

- **Not jazz:**
  - an orchestra (the classical reading, orchestral programs at least half the parts, no electric
    bass, under 0.3 kick strokes a beat): the 22";
  - pop, rock, funk, disco (an electric bass, an amped band, or a hard four on the floor): the studio kick;
  - otherwise, acoustic (bossa, folk, a singer's ballad): the 20".
- **Jazz:**
  - a big band (a brass section, or 4+ horns): the 22", but the 20" at 240 a minute or faster;
  - an electric bass, or an organ trio: the 20";
  - a trio or smaller with brushes, or a ballad: the 16";
  - two or more horns at under 200 a minute: the 20";
  - otherwise: the 18" (bebop).
- `PJ_KICK_KIND=<name>` forces one.

On the collection [meas]:
- Disco Boogie, A Hard Day's Night, Brazil and Beguine get the studio kick.
- Autumn Leaves, Caravan and Blue Moon get the 18".
- Take Five (a big-band arrangement: 13 horn channels) gets the 22".
- Desafinado, El Cumbanchero and Are You Lonesome Tonight get the 20".

The `PJ_KICK_*` size and tuning knobs are now factors on each kind's own values. `PJ_KICK_ZETA*`
became one factor, `PJ_KICK_DAMP`.

## 7. Open

- Fit the note's T60, the pair and the glide to recordings of a jazz kick (the separated stems:
  `drums.md`). The kick is felt more than heard there, so it needs passages with little bass.
  `research/jazz_kick_sound.md` holds the targets, and the places where it finds the model off: the
  decay, the 18"'s note at the bottom of its range, the out-of-phase pair, the strike positions.
- Sympathy: the kick ringing the snare's wires and the toms (the kit's room).
- The kinds by ear: none is tuned against a recording yet.

## Sources

- **[FB]** N. H. Fletcher, I. G. Bassett, "Vibration and sound radiation of the bass drum",
  physics.byu.edu: https://physics.byu.edu/docs/publication/1492
- **[Rossing PT]** T. D. Rossing, "Acoustics of percussion instruments", Physics Today:
  https://aip.brightspotcdn.com/PTO.v45.i3.40_1.online.pdf
- **[Liljencrants]** J. Liljencrants on beater contact time, Mechanical Music Digest, 2002:
  https://www.mmdigest.com/archives/Digests/200201/2002.01.06.11.html
- **[web, retail]** unported single-ply 18" resonant heads for jazz and bop, e.g.
  https://www.columbuspercussion.com/store/evans-18-resonant-black-bass-drumhead.html
- **[jd]** `research/jazz_drums.md`; `research/toms.md` §2.3 (the glide).
