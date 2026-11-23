# The church organs

Working notes for reworking the organ. The model's background is in
`research/church_organ.md` and `research/organ_room_coupling.md`. The code is in
`src/organ.cc`, `organ.h`, `organpipe.h` (flues), `organreed.h` (reeds),
`organroom.h` (each pipe's early paths), `organnave.h` (late field),
`organwind.h` and `organplan.cc` (registration in `-play`).

## Two organs

`-organ large` is the default. `-organ janakkala` selects the second one. The tuning
is separate: `-tuning NAME` works on either organ. `-play` prints the organ, its
tuning and its size.

| Organ | Stops | Ranks | Modelled pipes | Divisions |
|---|---|---|---|---|
| large | 53 | 69 | 3762 | Hauptwerk, Schwellwerk, Positiv, Pedal |
| after Janakkala | 40 | 57 | 3068 | Rückpositiv, Hauptwerk, Brustwerk, Pedal |

Both live in the one `kStops` table. Janakkala's tokens start with `j_`, and
`ofThisOrgan()` selects a rank by that prefix. Its `-play` registrations are the
`kJan*` ladders in `organplan.cc` (eight levels, ppp to fff). The organ-crescendo
instrument always uses the large organ's stops, because its roller names them.

## Janakkala: what is sourced and what is assumed

Source: [Suomen urut, Janakkalan kirkko](https://suomenurut.fi/janakkala/janakkalan-kirkko/).
Martti Porthan, opus 11, 1993, 40/III, disposition by Martti and Olli Porthan,
North German baroque with Stade St. Cosmae as the model. The church's site calls it
fully mechanical. Replacement pipes from 2000 allow meantone on some stops.

| I RP | II HW | III BW< | Pedal |
|---|---|---|---|
| Principal 8' | Quintadena 16' | Gedackt 8' | Principal 16' |
| Gedackt 8' | Principal 8' | Querflöt 8' | Sub-Baß 16' |
| Quintadena 8' | Rohrflöt 8' | Gedacktflöt 4' | Octav 8' |
| Octav 4' | Octav 4' | Octav 2' | Octav 4' |
| Rohrflöt 4' | Flöt 4' | Sedetz 1' | Nachthorn 2' |
| Waltflöt 2' | Nassat 3' | Tertia 2x | Mixtur 4x |
| Sieflöt 1 1/3' | Octav 2' | Scharff 3x | Posaun 16' |
| Sesquialter 2x | Mixtur 5x | Krumphorn 8' | Dulcian 16' |
| Scharff 4x | Cimbel 3x | Schalmey 4' | Trommet 8' |
| Dulcian 16' | Trommet 16' | | |
| Trechter Regal 8' | Trommet 8' | | |

The page also lists couplers RP/HW, BW/HW, RP/Ped, HW/Ped, BW/Ped, a tremolo for
the whole organ, and a Nachtigall. None of these are specific to this model yet.

**Assumed, because the source does not say:**
- Mixture, Scharff, Cimbel, Sesquialter and Tertia compositions are typical ones.
- Compass, wind pressure, pitch, temperament, total pipes and the church's
  dimensions are unknown. The model keeps the large organ's compass and its church.
- "BW<" is treated as enclosed, so the Brustwerk sits in the swell boxes. Their
  position above the main case is not a true Brustwerk position.
- Querflöt "C-h0 komb. Ged." is a stopped rank below c1 and its own flute above.
  The real organ shares those pipes, so the model's count includes 24 extra.
- The RP Principal's "C-H komb I Pr. 8'" is not modelled. It is a full rank.
- Quintadenas use the stopped-wood voicing, without their strong twelfth.
- Trechter Regal and Schalmey use the oboe family.
- Regulation copies each stop's nearest large-organ stop and needs doing by ear.
  Soloed at one key, these came out weak: Pedal Dulcian 16' (-46 dB, against about
  -26 for Posaun), Tertia (-47), Krumphorn (-44), Gedacktflöt 4' (-44).

Not done: couplers, the Nachtigall, a Janakkala-sized room.

### Temperaments

`-tuning NAME` sets the tuning on either organ, with A kept at 440 Hz. The two
`jyrki-*` tunings are adaptive: equal temperament with held notes drawn towards just
intervals. Every other tuning turns that off. Mutations stay pure against their key.
`temperamentCents()` holds the definitions.

| NAME | Definition (fifths of the circle from C) |
|---|---|
| `jyrki-spectral` | adaptive, by where the sounding pipes' partials meet (default) |
| `jyrki-heuristic` | adaptive, by a table of how much each interval (3rds, 6ths, 7ths ...) matters |
| `tet12` | plain 12-tone equal temperament, nothing retuned |
| `meantone` | quarter-comma, chain Eb to G#, wolf G#-Eb |
| `kirnberger3` | C-G-D-A-E each 1/4 syntonic comma narrow, F#-C# narrow by the schisma, the rest pure |
| `neidhardt` | 1724 "für eine große Stadt": C-G, G-D, D-A 1/6 Pythagorean comma; A-E, B-F#, F#-C#, C#-G#, Eb-Bb, Bb-F 1/12; E-B, G#-Eb, F-C pure |
| `lehman` | Bradley Lehman's reading of the WTC title page: F-C-G-D-A-E 1/6 narrow; E-B-F#-C# pure; C#-G#-D#-A# 1/12 narrow; A#-F 1/12 wide |

- Only meantone is an option on the real organ, through replacement pipes for *some*
  stops. The source does not say which, so the model retunes all of them.
- Pieces that need Ab or D# meet the meantone wolf. `PJ_ORGAN_MEANTONE_LOW=-4` gives
  Ab in place of G#, which suits C minor such as BWV 549.
- Lehman's reading is a contested hypothesis (Early Music, 2005, with replies in 2006).
- Neidhardt published several schemes and renamed them in 1732. This is the 1724 one.

Sources: [CBH, Neidhardt große Stadt](https://www.hpschd.nu/tech/tmp/neidhardt-gs.html),
[Wikipedia, Kirnberger temperament](https://en.wikipedia.org/wiki/Kirnberger_temperament),
[Amiot on Bach's temperament](https://www.mtosmt.org/issues/mto.09.15.2/mto.09.15.2.amiot.php).

## The church's shape

The nave is 50 m long, 18.3 m wide and 21.5 m high (19 673 m^3), the user's choice. It
replaced 50 x 20 x 20 m, whose equal width and height stacked modes and made reflections
coincide. The volume is nearly the same, so the reverberation times are unchanged.

**Side aisles** (`PJ_ORGAN_AISLES`, 1; 0 for flat walls). The side walls are arcades before
aisles about 6 m deep. The piers return part of a side reflection at once. The rest of a
first-order reflection comes back through the arches as two later, darker arrivals
(about 25-40 ms later), different for each source and microphone. Second-order paths
only lose a little at the piers.

**The rear wall.** The early paths have no east wall, so there is no discrete slap-back
from it (about 290 ms round trip). That energy is part of the diffuse late field.

**Not built:** a coupled chamber behind the altar, for a double-slope decay. The organ
should keep speaking directly into the nave, with the chamber catching and returning
the late sound through a wide opening. A deep apse should be faceted to avoid focusing.

### Janakkala's real church (not modelled yet)

Notes the user supplied, not independently checked: St. Lawrence, a cruciform stone
church of about 750 m^2 seating about 650. The medieval nave (1510-1520) is about 29 x
10 m inside. Transepts were added in 1848-1849, when the brick vaults were replaced by a
wooden barrel vault. The organ is in the west arm, the altar in the east, and a
half-octagon sacristy stands outside the east wall. The height and volume are not given.
Both organs still play in the one nave above. A real Janakkala room needs a room per
organ, a cruciform shape, a wooden vault, and the case re-laid to fit, since the
current case reaches 15.8 m.

## Listening and rendering pitfalls

- **`-seq` is dry.** It skips the church's early paths and the nave, so never judge
  room effects from it. Render through `-play` instead.
- **Solo a stop in the church** with `PJ_ORGAN_ONLY=<stop> pajupilli -play f.mid -o x.wav`.
  It overrides every registration the plan draws.
- Rendering is numerically chaotic. A small code change alters the waveform while
  the spectrum stays the same, so compare by bands and not sample by sample.
- `make test` runs `python3 -I`, which breaks `organ_stops` (`No module named
  'levels'`). Use `python3 tools/test.py organ organ_stops`.

## Open bug: the pedal's octave in `-play`

Plan-driven `-play` sends a file's pedal keys unchanged, while the Pedal's pipes
start at key 36. BWV 549's pedal track spans keys 24 to 48, so from the code 89 of
its 250 notes have no pipe and stay silent. Raising it an octave fits them all but
moves the opening from 32.7 Hz to 65 Hz on the 16' stops. The keyboard-zone mode
(`organ-88`) already adds that octave. Undecided.

## Attack puffs

Each flue pipe's puff has its own aimed source at its own mouth
(`PJ_ORGAN_PUFFPATH`, `PJ_ORGAN_PUFFLOBE`), yawed up to 55 degrees and pitched up to
20 degrees by a hash of stop and key. It also feeds the nave by enclosure.
`PJ_PEDAL_PUFF_DB` (-3) lowers the pedal flues' puffs.

## The Posaune's attack

A reed has no puff generator. Its "puff" is the tongue starting to speak. It is about
20 dB louder than the Principalbass at peak. These apply to `posaune16` and
`j_p_posaun16`, over roughly the first 200 ms of each note, and leave the settled
tone unchanged:

| Knob | Default | Meaning |
|---|---|---|
| `PJ_POSAUNE_ATTACK_DB` | -2 | level of the attack |
| `PJ_POSAUNE_DARK_DB`, `_DARK_HZ` | -8, 500 | shelf on the attack's highs |
| `PJ_POSAUNE_COLOR_DB` | 2.5 | each pipe's own peak or dip and tilt |
| `PJ_POSAUNE_ATTACK_MS`, `_ATTACK_FADE` | 170, 60 | hold and fade of the shaping |
| `PJ_POSAUNE_OPEN` | 0.6 | share of the attack leaving from the open tower front |

Why `_OPEN` exists: from inside a pedal tower the case and the gallery's edge
diffract the wall and floor reflections to 15-29 dB under the direct sound. Each
image path also passes a 300 Hz high-pass per reflection order, which removes much
of a pedal attack's energy from them. That filter is unchanged.

### The release puff

After key-up the Posaune's 4-12 kHz band swelled 8-12 dB over the held note for about
100 ms. The cause was the reed's turbulence term: it stayed a fixed share of the flow
while the tongue stopped beating and the gap stood open. Without the noise the band
falls 24-40 dB instead, and more Newton iterations changed nothing, so it is not a
solver artefact.

The release now has hysteresis between order and turbulence. While the wind dies the
flow stays organised: the turbulence follows (foot / chest pressure) to the power
`PJ_POSAUNE_RELNOISE` (4) downward and cannot rise again during the release. A new
press rebuilds it over about 20 ms. `PJ_POSAUNE_RELDAMP` (200, chosen by ear together with the hysteresis) damps the tongue as the
pallet closes, which removes a late swell on keys 55 and up and the low band's bump.
Janakkala's Posaun has its own knob, `PJ_JPOSAUN_RELDAMP` (also 200). Against 60 it lowers the
band under 250 Hz by about 2 dB at 20-40 ms after key-up with the same decay time.
From 600 up the overdamped tongue stays open: the note takes up to 100 ms longer to
fall 40 dB and the 250-1000 Hz band rings longer.
`PJ_POSAUNE_CLOSE` (0.5) halves the airflow's closing time. Below about 0.35 the low
band stops improving. `RELNOISE=0 RELDAMP=0 CLOSE=1` restores the old release. Other
reeds still have the old behaviour.

## Valve overshoot (trial, off by default)

`ReedPipe::setGroove`: the air slug through the pallet and toe (inertance) fills the
groove and foot (compliance), and the pipe bleeds them. The pallet's opening sets a
turbulent loss. The overshoot is therefore a response to the valve and is not imposed.
The drive is raised by the steady losses so that the settled pressure is the chest's.

Knobs: `PJ_POSAUNE_TOE` (toe resistance; below 0 is off), `PJ_POSAUNE_PALLET` (pallet
time multiplier), `PJ_POSAUNE_GROOVE_HZ` (45), `PJ_POSAUNE_TURB` (0.3),
`PJ_POSAUNE_BLEED` (0.25).

The organ's pedal pallets take about 34 ms (a first-order time constant), too slow
to overshoot at 45 Hz. Overshoot needs a snappier pallet:

| `_PALLET` | `_TOE` | Overshoot at a 34 ms pallet |
|---|---|---|
| 0.5 | 0.6 | 1.6 % |
| 0.4 | 0.45 | 4.9 % |
| 0.3 | 0.45 | 7.7 % |
| 0.25 | 0.3 | 12.7 % |
| 0.2 | 0.2 | 18.4 % |

Two earlier attempts were discarded: scaling the pressure during the valve's rise
never exceeded the chest pressure, and a prescribed underdamped step made every
note start far too fast.

Physics notes the user supplied (not independently checked): overshoot about 0 % for
closed-toe or slow keys, 10-25 % for open-toe tracker attacks, 25-40 % for very fast
valves, ringing at 20-100 Hz. The pallet box dips meanwhile. Jet speed follows the
square root of foot pressure, so an overshoot can excite upper partials before the
fundamental locks, which voicers shape into chiff. If adopted, flues should get this
too, and `organwind.h` should supply the pallet box's dip.

## Adaptive tuning and short notes

A key used to count as "young" for 150 ms and was re-solved freely at each neighbour's
arrival, by up to its whole range, so a pipe already speaking could bend audibly (BWV
549 near 4:55). Now a key takes its place in its first 40 ms, before it speaks, and
keeps it for its first `PJ_JI_SHORT_MS` (600). Only a key held longer moves, at most
3 cents a solve. Short notes are hard to place by pitch, but a bend in them is audible.

**`jyrki-heuristic` looks ahead in `-play`.** As a key lands, it is tuned against what the
score has sounding `PJ_JI_AHEAD_MS` (150) later. Keys about to be released no longer
pull it, and keys about to arrive do, as partners only. Live playing cannot look ahead.

**Later:** record each note's ideal tuning over its life and start it at that timed
average, so that it need not move at all.

## Ideas

- German Protestant churches tend to be drier and clearer (the user's note gives an
  occupied Thomaskirche at about 1.6 s), French ones longer and darker. A Janakkala
  room smaller than the current 50 x 18.3 x 21.5 m nave would suit its articulate voicing.
