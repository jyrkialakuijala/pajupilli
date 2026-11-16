# The reed organ (GM 21) as a pressure harmonium: a Schiedmayer Kunstharmonium

Status: 2026-11-16. Research and plan; nothing is built. This builds on `research/free_reeds.md` (§2
the free reed, §12 what is built in `src/freereed.h` and `src/accordion.cc`) and on
`research/harmonica.md` §10 (reeds sharing an upstream resonator), and does not repeat them. The
program table in `src/midifile.cc` still sends GM 21 to the `lead-square` synth.

The user wants: a high-end **pressure** harmonium (positive wind, reeds blown, the French system), a
Schiedmayer of Stuttgart in the Mustel lineage (a *Kunstharmonium*, *harmonium d'art*); **not** the
American suction reed organ.

Marks: **[paper]** a source below (book, paper, primary document, museum record, maker's own
literature quoted in one); **[web]** a weaker web source below (dealers, forums, encyclopaedias);
**[calc]** worked out here; **[memory]** general knowledge, to be checked; **[design]** a modelling
choice; **[file]** measured in the user's MIDI files; **[rec]** measured in the user's recordings
(numbers only, no audio copied; scripts in the session scratchpad).

Pitch names: scientific (C4 = middle C = MIDI 60). The harmonium literature uses others: the French
split "mi 3 / fa 3" and the German "e¹ / f¹" are both **E4/F4 = MIDI 64/65** here; German "C" (großes
C) is C2 (MIDI 36), "c⁴" is C7 (MIDI 96), "A1" (Kontra-A) is A1 (MIDI 33).

## 1. Short answer

- **The wind is the instrument.** Two feeders (pumped by the feet) blow into an air space; normally a
  spring-loaded reservoir steadies it (an organ-like, "expressionless" tone); the **Expression** stop
  closes the reservoir off, so the pressure at the reeds *is* the feet's push: "the pressure on the
  reeds is now the same as the pressure on the treadles" **[paper, T05]**. Mustel's **double
  expression** (1853/54) splits the wind at E4/F4 into a bass and a treble chamber, each held by a
  self-regulating valve at a pianissimo pressure until the player's knee releases it to the feet
  **[paper, T05]**. A model needs a wind system with these states, not a per-note envelope.
- **No harmonium wind pressure in Pa or mm of water was found in any source**, primary or secondary.
  The bracket from what was found: harmonium reed chambers 0.2-1.5 kPa (a hand-pumped Indian
  harmonium) **[paper, FA23]**, American suction organs 2¼-3½ inches of water = 560-870 Pa **[web,
  ROW]**, Cottingham's laboratory reed-organ reeds 0.5-3.5 kPa **[paper, CRB]**, and a reservoir
  spring load of 2-3 springs of 10-16 lb **[web, LH]**, which gives **~0.7-1.5 kPa** on a plausible
  reservoir **[calc, §2.3]**. Plan: reservoir ~800 Pa; expression 150 Pa (pp) to ~2.2 kPa (ff).
- **The Kunstharmonium disposition is standard** (Mustel 1889, Titz, Schiedmayer): bass (keys C2-E4)
  1 Cor anglais 8', 2 Bourdon 16', 3 Clairon 4', 4 Basson 8', 5 Harpe éolienne 2'; treble (F4-C7)
  1 Flûte 8', 2 Clarinette 16', 3 Fifre 4', 4 Hautbois 8', 5 Musette 16', 6 Voix céleste 16',
  7 Baryton 32', 8 Harpe éolienne 8'; with Percussion (on stop 1), Forte fixe, Forte expressif,
  Métaphone, Grand Jeu and Prolongement **[paper, MF; T05; LEM]**. Stops 1 and 2 are the "front"
  rows, round and dark; 3, 4 and the higher numbers the "back" rows, brighter and string-like, under
  the forte lids and the métaphone shutters **[paper, K00]**. Schiedmayer's *Meisterharmonium
  "Dominator"* (Strauss, *Ariadne auf Naxos*, 1912) adds Violoncello 8', Undamaris 8', Vox angelica 16',
  a compass A1-F7, a two-depth key touch and a five-octave celesta **[paper, K00]**.
- **In a pressure harmonium the reed cell is downstream of the reed.** The chest's wind passes the
  reed (screwed under the cell), fills the cell (*Kanzelle*), and leaves through the key's pallet on
  top of the cell **[paper, T05]**. `FreeReed` has its cell *upstream* (the accordion's); the
  harmonium needs a variant with a downstream cell vented through the pallet: a Helmholtz resonator
  at ~1.2-2.9 kHz **[calc, §4.4]**, which is where Cottingham measured the cell's boost of the
  harmonics (1-4 kHz) **[paper, AK4; C97]**.
- **Attacks are slow and pressure-dependent**, bass reeds slowest ("the lower notes on the Bourdon
  are slow of speech" **[paper, K00, Malkin]**); the onset passes through stages with the second
  transverse and the first torsional modes early **[paper, CW15; C17]**. The **Percussion** is a
  lead-weighted felt hammer thrown up by the key against the tongue of stop 1: alone a dry
  pizzicato, sustained by any wind **[paper, K00 quoting Karg-Elert]**.
- **No harmonium recording was found** in `~/Music`; `franck_track01.cdda.wav` is almost certainly a
  pipe organ in a church **[rec, §7]**. No GM 21 occurs in the 301 MIDI files **[file]**.

## 2. The instrument

### 2.1 The lineage

| Year | Event | Mark |
|---|---|---|
| 1840 or 1842 | A.-F. Debain patents the "Harmonium" in Paris: pressure wind, free reeds (sources disagree on the year) | [web, HC; FRW; DEW] |
| 1853/1854 | Victor Mustel's **double expression** ("the most ingenious invention harmonium building has known", Titz) | [paper, T05; web, FRW] |
| 1853 | **J. & P. Schiedmayer** (Julius and Paul, sons of J. L. Schiedmayer) found the first German harmonium factory in Stuttgart; one account has Julius trained with Debain in Paris; the firm later builds pianos as *Schiedmayer Pianofortefabrik*, makes harmoniums into the 1950s | [web, SCH; DEW] |
| ~1880 | Mustel's Kunstharmonium essentially complete; later additions only (the treble Harpe éolienne, a bass Contrebasse) | [paper, K00] |
| 1886-1889 | Auguste Mustel's celesta (1886); Victor Mustel's *Orgue-Célesta* patented 1888, the third one built 1889 (8 reed ranks, A = 455 Hz) | [web, SCH; paper, MF] |
| 1904 | Gustav Fritsche (an organ builder) with Kaim & Sohn, Kirchheim, show a "Deutsches Meisterharmonium" (Paul Schmidt): pressure wind, prolongement, a **five-octave** celesta, double expression on a new system (patented 2 Oct 1903), a two-depth key touch, the bass 16' and 2' switchable for the bottom octave; the line passes to Schiedmayer, Fritsche its chief constructor | [paper, K00] |
| 1905 | Titz (Berlin, for the publisher Carl Simon) copies the Mustel Kunstharmonium exactly | [paper, K00; T05] |
| 1912 | Strauss's *Ariadne auf Naxos* asks for an instrument "from the Schiedmayer factory, Stuttgart": the *Meisterharmonium "Dominator"*, played by Paul Schmidt at the premiere | [paper, K00] |
| 1995- | Schiedmayer Celesta GmbH (Wendlingen) still builds celestas | [web, SCH] |

Price, for scale: a Kunstharmonium cost ~2 500 Mark against 1 580 for a large plain pressure
harmonium (Simon's "reform" model C) **[paper, K00]**. Schiedmayer also built plain pressure
*Orchester-Harmoniums* ("Ideal", 2-4 rows, percussion optional), with mutes (*Sourdinen*) instead of
the expensive double expression **[paper, K00]**.

### 2.2 The wind system, from the Titz description [paper, T05]

1. **Feeders** (*Schöpfbälge*): two, one under each treadle, filled from the room on the up stroke,
   blowing on the down stroke into a common channel and **air space** (*Luftraum*). The two feeders
   join *before* the chambers: the right foot does not feed the right half.
2. **Reservoir** (*Magazinbalg*): a bellows loaded by two large spiral springs (Titz); a German
   maker's account gives two flat springs of ~10-16 lb, a third of 10-12 lb on large reservoirs
   (16 + 16 + 10 = 42 lb) **[web, LH]**. It stores wind at the springs' pressure: "the
   unmodifiable, even, expressionless, organ-like tone, if one plays without expression".
3. **Expression stop:** closes the **expression valve** between the air space and the reservoir, so
   the feeders blow straight to the reeds: "the tone proportional to the strength of the treading".
4. **Double expression** (Mustel; Titz's version): the wind enters two large chambers (bass, treble,
   divided at E4/F4) each through an **inlet valve**. The valve is balanced against a small bellows
   of the same size by a beam, so the wind's force on it cancels; a large **regulating bellows**
   open to the room tends to shut it as the chamber's pressure rises; a **regulating spring** keeps
   it from shutting. The result: in the knee levers' neutral position each chamber regulates itself
   to "no more wind than the reeds need to speak in pianissimo", whatever the number of stops. A
   **knee lever** pressed outward (or both latched inward) holds that side's inlet valve wide open:
   that half then follows the feet. One half can stay pp while the other swells: a melody in the
   treble over a pp accompaniment.
5. **Distribution:** each **half-stop** has its own compartment (13 on the Titz: 5 bass, 8 treble),
   opened by its stop knob's inlet valve; the reeds of that half-stop sit in it.
6. **Reed, cell, pallet:** "on the underside of the cell (K) the reed frame (ZR) with the reed (Z) is
   screwed; the wind escaping pushes the tongue upward" into the cell; the key's **pallet** (V)
   rests on the upper rim of the cell, held shut by the key spring. The cell lies downstream of the
   reed (§4.4).
7. **Forte lids and the métaphone:** above the back rows, two **forte lids** (bass, treble) open by
   *Forte fixe* (mechanically) or by *Forte expressif* (pneumatically: two small bellows wired to the
   wind open the lids more as the feet push harder; "the lids sink, the reeds suffer a marked
   weakening"); below them, three **jalousie shutters** per half shut by the *Métaphone* knobs.
8. **Heel levers and knees:** the middle heel lever draws the **Grand Jeu** (the continuous stops 1-4
   with percussion and forte expressif); the left heel lever releases the **Prolongement**.

### 2.3 Pressures

No source gives a harmonium's wind pressure as a number. What there is:

| What | Pressure | Mark |
|---|---|---|
| Reed-chamber pressures in a real (Indian, hand-pumped) harmonium | 0.2-1.5 kPa | [paper, FA23] |
| Its reed's motion: sinusoidal at ~0.2 kPa, harmonics in the motion at ~1.4-1.6 kPa | | [paper, FA23] |
| American suction reed organs | 2¼-3½ in. water = **560-870 Pa** (blowers: 2⅜ in. for 3-5 ranks, 3⅛ in. for 10) | [web, ROW] |
| Cottingham's laboratory measurements of reed-organ reeds | 0.5-3.5 kPa range; 0.6 kPa a working point | [paper, CRB; C13] |
| Pipe organ flues, for comparison | 3-4 in. = 750-1000 Pa | [web, ROW] |
| **Reservoir, from the spring loads** | 2 springs × 10-16 lb = 89-142 N, 3 springs 187 N; a reservoir top ~50 × 35 cm hinged at one end with the springs at ~0.7 of its length: p ≈ 1.4 F / A = **0.7-1.1 kPa**, 1.5 kPa with three springs | [calc] |
| **Feet under expression** | a feeder plate ~0.07 m² at ~1:1 leverage, a foot pushing 10-150 N: **150 Pa-2.1 kPa** | [calc] |

- How fast the feet can change it: the air space and chambers, ~5 L, have a compliance V/ρc² =
  3.5e-8 m³/Pa; a net 3 L/s raises the pressure ~85 Pa/ms: **a pp-to-ff sforzando in ~10-30 ms**,
  limited by the treadle stroke, not the air **[calc]**. Hence Malkin: "sforzandos, nuances and even
  a vibrato can, with it, be produced as easily as on the violin" **[paper, K00]**.
- Air use: a good reservoir holds a single 8' note for 15-20 s, 30-40 s when tight; leaky ones only
  3-4 s **[web, LH]**. The model's untuned harmonium-sized reed draws ~0.3-1 L/s at 0.8-2 kPa
  **[calc, freereed.h]**; a 4-rank chord of 4 notes, ~5-15 L/s. Two feeders of ~3 L each at 1-2
  strokes per second per foot give ~6-12 L/s **[calc, memory for the feeder size]**: full chords at
  ff are near the feet's limit, as players report of grand jeu **[memory]**.
- Mustel's own method (*École de l'orgue expressif*) gives ~50 pages to "the art of blowing" **[web,
  unverified]**; Karg-Elert describes tremolo made by the feet with expression (KdR 1, 18) **[paper,
  K00]**.

### 2.4 Pressure against suction

- **Suction** (the American organ, the German *Saugwind*): exhausters draw air out; the room's air is
  drawn through the reeds; reeds mounted the other way round; keyboards usually start on F; no
  expression stop; dynamics by **swell shutters** over the reed cells (knee swell) **[web, DEW; GdO
  FAQ via search; CP]**. Split at B3/C4 (h/c) on suction harmoniums, E4/F4 on pressure ones **[paper,
  K00]**.
- **Why pressure is more expressive [paper + calc]:** with the reservoir bypassed the pressure is the
  feet's, so level *and* timbre follow the player within ~10-30 ms (§2.3); the suction organ's
  exhauster always works through its reservoir, and its dynamics are a filter (shutters), not the
  reeds' drive. Suction instruments are "less strident" because "the sound gets drawn into the organ"
  **[web, CP]**; Schiedmayer's own catalogue: "the suction system produces a gentler, milder tone"
  and their orchestral harmonium is built for pressure "with regard to its use" **[paper, K00]**;
  Malkin calls the pressure harmonium greatly superior "to the American organ for orchestral work"
  **[paper, K00]**.
- **Why brighter [design hypothesis, to test in the model]:** (1) the pressure instrument is played
  harder (up to ~2 kPa against ~0.6-0.9): the tongue swings wider and the puffs are sharper, more
  harmonics (FA23 sees the harmonics appear at 1.4-1.6 kPa); (2) its reeds are wider and heavier
  (Schiedmayer: "reeds of the largest scale" **[paper, K00]**), more flow per puff; (3) in the pressure
  instrument the room hears the **downstream** side through a short cell and an open pallet (the jet
  side, §4.4), in the suction organ the **upstream** side through the cell mouth and the shutters,
  with the downstream jet inside the exhauster box.

## 3. Stops and ranks

### 3.1 The disposition

Mustel *Orgue-Célesta* no. 484-37 (1889), as printed for its recording **[paper, MF]**, with the
sounding ranges for a 61-key C2-C7 keyboard split E4/F4 **[calc]**; Titz and Lemare's Mustel agree on
the numbering **[paper, T05; web, LEM]**:

| No. | Bass (keys C2-E4, MIDI 36-64) | Sounds MIDI | Treble (keys F4-C7, MIDI 65-96) | Sounds MIDI | Row |
|---|---|---|---|---|---|
| 1P | Percussion (on Cor anglais) | | Percussion (on Flûte) | | front |
| 1 | Cor anglais 8' | 36-64 | Flûte 8' | 65-96 | front |
| 2 | Bourdon 16' | 24-52 | Clarinette 16' | 53-84 | front |
| 3 | Clairon 4' | 48-76 | Fifre 4' | 77-108 | back |
| 4 | Basson 8' | 36-64 | Hautbois 8' | 65-96 | back |
| 5 | Harpe éolienne 2' (two reeds, beating) | 60-88 | Musette 16' | 53-84 | back |
| 6 | | | Voix céleste 16' (two reeds, beating) | 53-84 | front (beats with 1) |
| 7 | | | Baryton 32' | 41-72 | back |
| 8 | | | Harpe éolienne 8' (two reeds; 2' on some) | 65-96 | back |
| 0 | Forte expressif | | Forte expressif | | |
| M | Métaphone | | Métaphone | | |
| F | Forte fixe | | Forte fixe | | |
| E, G | Expression; Grand Jeu (heel) | | Coupler (to the celesta) | | |
| Prol. | Prolongement C2-B2 (heel) | | | | |

- **Classic "four and a half" (*Vierspiel*):** stops 1-4 continuous through both halves plus the
  treble Musette; the Kunstharmonium adds 0, 5-8, Prolongement and Métaphone **[web, FRW]**. Grand
  Jeu = 1-4 on both sides **[paper, K00]**. Titz: 13 half-stops (5 + 8) **[paper, T05]**; Lemare's
  Mustel: 6 reed sets bass, 10 treble (6a "Muted strings" 16', 6b "Violins" 8') **[web, LEM]**.
- **Counts:** the bass of the 1889 Mustel has 6 reeds per key (1, 2, 3, 4, 5 ×2), the treble 10
  (1-5, 6 ×2, 7, 8 ×2) **[calc]**.
- **Front and back:** the front rows (1, 2; 6 beats with 1) are "gentle and dark", the back rows "brighter
  and more string-like" **[paper, K00]**; the métaphone acts on Musette, Hautbois, Fifre, Baryton and
  Harpe éolienne **[web, BARS]**, i.e. the back rows.
- **Registration character [paper, K00: Malkin; Karg-Elert]:** (1) alone "not much use", strengthen
  with (4); (2) alone "quite full, distinct in quality from 1 and 4", played an octave up; (1)(3)(4)
  "bright, crisp", "sometimes too shrill"; (2)+(3) "slightly grotesque"; [1]+[4] "light, crisp";
  [2] "sustained accompaniments and horn passages"; [2]+[1] "pipe organ diapason"; Grand Jeu "heavy
  accompaniments, trumpet passages"; [3]+[5] in the bass "tremendous crescendo from finest
  whisper". The Musette is "more flexible, smoother and more expressive than [4] or [3], with less
  core"; "[5] stands to [2] as [4] to [1]"; the Bourdon/Clarinette [2] is "patzig" (blatant) in
  close chords. Karg-Elert counts ~500 mixtures on the four-and-a-half and 7 905 on the
  Kunstharmonium **[paper, K00]**.
- **The 16' convention:** harmonium music is often written for 16' as the base pitch; with 16' the
  player moves up an octave ("the iron rule") **[paper, K00]**. A GM file is at concert pitch:
  the model's default must be 8' rows only (§8.4).

### 3.2 Compass and split

- Compass **C2-C7** (5 octaves, 61 keys) at 8' on pressure harmoniums (Kotykiewicz "C bis c⁴", Titz
  "to c⁴"); with 16' C1-C6 **[paper, K00; T05]**. Schiedmayer *Dominator*: **A1-F7** (MIDI 33-101),
  3 more in the bass, 5 more in the treble **[paper, K00]**.
- Split **E4/F4** (MIDI 64/65): bass 29 keys, treble 32 **[paper, T05; K00; web, FRW "mi 3 / fa 3"]**.
  The prompt's "E3/F3" is the French octave numbering of the same keys.
- Pitch: A = 435 Hz until 1939 on most surviving instruments; Mustel 1889 at 455 Hz (English
  pitch); Lemare's at 440 **[paper, K00; MF; web, LEM]**. Model: 440 Hz [design].

### 3.3 How the ranks differ physically

- **Reed scale (width):** German makers' scales: narrow ~4 mm, normal ~5, broad ~6, half-broad 4.5
  **[web, LH]**. Reed frames 23-88 mm long, ~2.3 mm thick (German), 2.5-2.6 mm (American) **[web,
  LH]**. The Harpe éolienne's treble reeds are of narrow scale (laid side by side, where the bass
  ones face each other) **[paper, T05]**. A reed of the largest scale for an orchestral pressure
  instrument **[paper, K00]**. The front rows broad, the back rows (Hautbois, Musette, Voix céleste)
  narrow **[memory: consistent with "round and dark" vs "string-like", to check]**.
- **A measured harmonium reed:** B5 (990 Hz), brass, 16 × 2 × 0.4 mm, E 125 GPa, ρ 8490, tip set
  0.1 mm *into* the slot **[paper, FA23]**. Cantilever check: f = 0.1615 (t/L²) √(E/ρ) = 0.1615 ×
  0.4e-3 / 2.56e-4 × 3837 = **968 Hz** **[calc]**: the reed is close to a uniform beam.
- **Cells (*Kanzellen*):** each reed has its own; the cells of a half-stop open into their compartment
  from below through the reed, upward through the pallet **[paper, T05]**. Mustel made the reeds
  speak into resonating chambers, sacrificing size and cost **[web, CP]**. Their volumes are not
  published; §4.4 estimates them.
- **Voicing:** bending the tongue slightly into the frame reduces the highs, away from it increases
  them; a twisted tip corner changes the windway and tone; metal off the tip raises the pitch, off
  near the root lowers it; large reeds loaded with solder at the tip **[web, CP]**. Curving the tongue
  to engage the torsional mode helps the slow speech of large low reeds **[paper, C17]**.
- **Mutes:** shutters over the back rows (métaphone: duller), lids above them (forte: open, freer);
  Sourdine on stop 1 on some (Kotykiewicz) **[paper, T05; K00]**; felted shades over cells on suction
  organs ("dolce", about half as loud) **[web, CP; LH]**.

### 3.4 The mechanisms

| Mechanism | What it does | Mark |
|---|---|---|
| **Percussion (1P)** | each key's pusher throws "a small, lead-weighted, felt-covered hammer" up against the tongue of stop 1, which sounds at the same pitch as when blown; alone "dry, short, brittle, without carrying power; clearer toward the bass, needle-sharp in the treble", like pizzicato; held on by even minimal wind it becomes "musically valuable"; for prompt speech of the 8' (Malkin adds (1) to a moving bass because the Bourdon is slow) | [paper, K00 quoting KdR 1, 24 ff.; T05] |
| **Prolongement** | heel-lever: a catch bar moves forward; a key of the bottom octave C2-B2 pressed is caught and held; pressing another releases the first (one note at a time); a sideways heel push releases all | [paper, T05] |
| Prolongement doux / grand | two strengths on large instruments (Dominator) | [paper, K00] |
| **Grand Jeu** | heel lever: stops 1-4 (with percussion and forte expressif) on both halves | [paper, T05; K00] |
| **Expression (E)** | closes the reservoir off | [paper, T05] |
| **Double expression** | the knee levers, §2.2 (4) | [paper, T05; web, BARS] |
| **Forte fixe (F)** | opens the forte lids above the back rows | [paper, T05] |
| **Forte expressif (0)** | the lids lifted by small bellows on the wind: open more as the pressure rises | [paper, T05] |
| **Métaphone (M)** | closes three jalousie shutters per half under the lids: the back rows veiled | [paper, T05; web, BARS] |
| **Tremolo** | a sprung flap in one stop's compartment (usually the treble Clarinette 16'), fluttering in the wind and interrupting it; found on 19th-century French instruments; Zellner (1862): a toy | [paper, K00] |
| **Two-depth touch** (Schiedmayer *halber Tastenfall*) | a shallow key sounds the back rows only, a deep one front and back: single notes can be brought out | [paper, K00] |
| **Celesta** | Mustel to c0 only; Schiedmayer five octaves (harp imitation in the low register); with a split coupler | [paper, K00] |

### 3.5 Céleste tunings

- Voix céleste 16' (treble) is a beating rank with one of the front rows, "round and dark" **[paper,
  K00]**; its two reeds per key face each other **[paper, T05]**. Harpe éolienne 2' (bass), 8'
  (treble) also two reeds beating **[paper, MF; T05]**. Schiedmayer's Undamaris 8' continues the bass
  Äolsharfe, Vox angelica 16' is "very gently beating, string-like" **[paper, K00]**.
- **No harmonium beat rate was found.** A German maker's account: the second set tuned **2-3 Hz flat**
  **[web, LH]** (suction harmoniums); pipe-organ célestes ~1-2 beats/s **[web]**. The accordion here
  uses 3.5 beats/s at A4 (`PJ_ACC_BEAT`). Plan: Voix céleste one reed in tune with stop 1, the other
  +1.5 beats/s at A4 rising as √f (constant-beat-ish) **[design, ask]**; Harpe éolienne wider, ~3-5
  beats/s (it is a shimmer stop) **[design]**.

### 3.6 Schiedmayer's Dominator, as far as found

- A Kunstharmonium (the §3.1 disposition) plus **Violoncello 8'** ("cello-like"; with a "Horn" the
  same [4] voiced round and nasal), **Undamaris 8'** and **Vox angelica 16'**: three back rows, two
  beating; compass A1-F7; two-depth touch; the bass's lowest octave (to B) switchable separately for
  several stops; two prolongement strengths; split coupler to the five-octave celesta **[paper, K00,
  from Schiedmayer's catalogue reprinted in *Das mechanische Musikinstrument* 46, 1988]**.
- The full stop list exists only as an image in that catalogue; not seen here.
- Its sales prose promises an orchestra: "the mysteriously wavering tremolo of the violins … the
  mighty sforzato of the bass tubas and trombones" **[paper, K00]**: i.e. a wide range of pressure
  and colour, which is what the model should deliver.

## 4. The reed and its cell

### 4.1 Reed sizes (for the model)

| Pitch | Source | Tongue L × W × t | Mark |
|---|---|---|---|
| B5 990 Hz | an Indian harmonium | 16 × 2 × 0.4 mm, brass | [paper, FA23] |
| C2-G6 | German reed numbers 1-90; frames 23-88 mm | widths 4-6 mm by scale | [web, LH] |
| G4 444 Hz | harmonica (for contrast) | 12.95 × 2.1 × 0.11 mm | [paper, MB07] |

Plan **[design, to fit]**: L = 16 mm × (990/f)^0.47 (C2 58 mm, C4 30 mm, C6 16 mm, C1 80 mm with a
tip weight), W by scale (broad 5-6 mm, narrow 3.5-4 mm in the middle, × (990/f)^0.3 within limits),
the set 4 % of L, the frame 2.3 mm. Harmonium reeds are 2-3 times the accordion's (`accordionReed`
gives 29 mm at C2): more flow, more power per reed **[calc]**.

### 4.2 Pitch against pressure

- Free reeds fall roughly linearly in pitch as the pressure rises, with anomalies at very low pressure
  (a small rising region, measured on harmonium-type reeds from an American organ) and at very high
  pressure, and some hysteresis **[paper, CRB; CH]**. A ~420 Hz reed: 422 → 419 Hz from 0.05 to
  0.85 kPa, **~-12 c over the playing range** **[paper, C13]**. Reed-organ reeds in their organs:
  "frequency essentially constant over a very wide pressure range" **[paper, C95]**.
- So under expression a ppp-fff swell moves the pitch by ~10-15 c at most: the model's reed already
  does this (-4 c from 0.3 to 2 kPa at 444 Hz, free_reeds §12) **[calc]**. Céleste beats will drift a
  little with pressure, as on the real one.

### 4.3 Onset

- **Stages [paper, CW15]:** a static deflection of the tongue first ("an initial displacement of the
  equilibrium position") with a few irregular cycles; then the amplitude grows while the frequency
  settles; then growth to the steady amplitude. The second transverse mode and the **first torsional
  mode** appear early, the torsional one often first; curved tongues engage it and speak quicker
  **[paper, C17]**. The growth is exponential, its rate rising with the flow **[paper, SWB71]**.
- **Mode frequencies** of a reed-organ-type reed: 542 Hz, 2140 Hz (3.95×), 5175-5192 Hz (9.5×), others
  at 4653 and 5696 Hz (torsional?) **[paper, C13]** (a clamped-free beam: 6.27×, 17.5×; the tapered
  tongue's are lower) **[calc]**.
- **Bass reeds are slow:** "the lower notes on the Bourdon are slow of speech, and the 8 ft. will get
  much quicker response" **[paper, K00]**. No measured onset time of a harmonium reed was found.
- **The engine's onset [calc, `freereed.h` with the §4.1 harmonium build, pallet opening over 5 ms,
  t90 = 90 % of the steady tip amplitude]:**

| Reed | 250 Pa | 800 Pa | 2 kPa |
|---|---|---|---|
| C2 (L 57 mm) | barely speaks | 43 ms | 35 ms |
| C3 (41 mm) | 312 ms | 26 ms | 16 ms |
| C4 (30 mm) | 75 ms | 13 ms | 12 ms |
| C5 (22 mm) | 36 ms | 11 ms | 10 ms |
| C6 (16 mm) | 29 ms | 10 ms | 10 ms |

  Near the threshold the start is slow (hundreds of ms: a pp swell-in, which players use), above it
  a few cycles. The engine has the cell upstream and no torsional mode, so its bass onsets are likely
  too *fast* at mf: §8 fits them (a lower Q-independent growth for long tongues) **[design]**.
- **Percussion, numbers [calc]:** a C4 broad tongue 30 × 5 × 0.5 mm brass weighs 0.64 g, its first
  mode's effective mass ~0.16 g; a 1-3 g hammer at 0.3-1 m/s gives the tip ~1-2 m/s, an amplitude
  v/ω ≈ 0.6-1.2 mm, ~15-25 % of the steady swing at 800 Pa: the reed starts most of the way up its
  exponential growth (from 20 % to 90 % takes ~1/5 of the time from the noise floor): **onsets
  several times quicker**, and a felt "tick" as the attack.

### 4.4 The cell: downstream, a Helmholtz resonator

- **Topology [paper, T05]:** chest (stop compartment, pressure p_s) → reed (screwed under the cell)
  → cell (volume V_c) → pallet (opening A_p, lifted by the key) → under the forte lid / shutters → room.
  With the pallet shut the cell sits at p_s and the reed has no pressure difference; opening the
  pallet drains the cell and the difference builds within a few ms. The reed's **downstream** side
  is coupled to the room through the cell.
- **Its resonance [calc]:** f_H = (c/2π) √(A_p / (V_c L_eff)), L_eff the pallet hole's depth plus end
  corrections:

| Cell | V_c | A_p | L_eff | f_H |
|---|---|---|---|---|
| bass 8' (C2-C3) | 40 cm³ | 6 cm² | 30 mm | ~1.2 kHz |
| middle (C4) | 13.5 cm³ | 4 cm² | 25 mm | ~1.9 kHz |
| treble (C6) | 4 cm³ | 2 cm² | 18 mm | ~2.9 kHz |

  (cell sizes guessed from the reed frames' 23-88 mm length **[web, LH]**, × ~10-15 mm × 15 mm
  **[memory]**). A pallet a quarter open halves f_H **[calc]**.
- **Measured cell effects [paper]:** a reed cell lowers the frequency at the same pressure and raises
  the higher harmonics relative to the fundamental, longer cells anomalous; hysteresis **[C97]**; the
  harmonics in **1000-4000 Hz are strengthened** and the reed in a cell vibrates "a little slower"
  **[AK4, summarising Cottingham's American-organ work]**. A chamber resonance near the reed's own
  frequency can inhibit speech (accordion, harmonica top reeds) **[C17]**: in a harmonium only the top
  octave of the 4' and 2' rows (C7-C8, 2-4 kHz) comes near f_H **[calc]**: voice those cells larger.
- The cell is where "the 4 and the 1 differ" (§3.1) besides the tongue's scale: same pitch, a
  different cell and voicing; Cottingham: the reed's own spectrum largely sets the radiated sound,
  the environment modifies it **[paper, C95]**.

## 5. The sound

- **Spectrum:** the free reed's near-square pressure puffs, harmonics to ~10 kHz (accordion, EMS21);
  even and odd harmonics both strong on harmonica reeds **[web, CP]**; the harmonium reed's motion
  sinusoidal at low pressure, harmonics in the motion appearing at ~1.4-1.6 kPa; a model's spectrum
  shows formants at the tongue's higher modes (~6 and ~16 kHz for a 990 Hz reed) **[paper, FA23]**.
  The cell adds a broad lift at ~1-4 kHz (§4.4). No measured spectrum of a pressure harmonium was
  found.
- **Brightness with pressure:** louder is brighter (wider swing, sharper puffs) **[paper, FA23; C95
  "frequency and amplitude of the spectral components vs pressure"]**; plus, on the back rows, the
  **forte expressif** lids opening with the pressure, a second, mechanical brightening **[paper,
  T05]**. Target **[design]**: centroid ≈ 2-2.5 f0 at pp to ≥ 4-5 f0 at ff on 8' rows.
- **Attacks:** without percussion a swell from silence (tens to hundreds of ms, the pressure and the
  reed's growth); with percussion a click and a prompt pizzicato-like start (§3.4, §4.3). Releases:
  the pallet shuts, the cell's pressure equalises in ~ms and the reed rings down at its Q (~0.1-0.3 s
  at C4) **[calc]**, filtered by the cell.
- **Vibrato/tremolo:** the foot's tremolo under expression (pressure modulated at the treading's rate
  and faster), the tremolo flap (French), beating rows; Mahler (8th symphony) and Schoenberg
  (*Herzgewächse*) ask for a "Tremolo-Register", which Koenig cannot pin down to one of the four
  **[paper, K00]**.
- **Loudness:** none measured. Model estimate **[calc]**: a reed with a 0.3 L/s fundamental flow
  swing at C4 radiates p = ρ ω U / (4π r) ≈ 0.05 Pa at 1 m (~67 dB) in the fundamental alone; 4 rows ×
  4 notes at ff, ~85-90 dB at 1 m. Malkin: "the volume of tone … will generally be found louder than
  would be anticipated" **[paper, K00]**.
- **Radiation from the case [memory + paper, T05]:** the front rows' sound leaves under the front
  board above the keys (behind a cloth panel); the back rows' through their shutters and lids at the
  top back of the case. Model as two sources with their own filters (§8.1), the halves panned slightly
  apart.

## 6. Repertoire and playing

- **Karg-Elert** wrote more than 100 pieces for the Kunstharmonium 1903-1915 **[web]**: *Kompositionen
  für Kunstharmonium* op. 26 ("Doppel-Expression", Simon 1906-09), *Portraits* op. 101, *Die Kunst des
  Registrierens* op. 91 (1911, a handbook for every system), *Die Reform des modernen
  Druckwind-Harmoniums* (~1908, with a model disposition) **[web, IMSLP]**; he calls the expression
  the soul of the instrument **[web]**.
- **Franck** (*L'Organiste*, harmonium pieces; the *Prélude, fugue et variation* op. 18 first for
  harmonium and piano) **[memory]**; also Alain, Boëllmann, Liszt, Reger, Widor **[web, HC]**; Rossini's
  *Petite messe solennelle*, Dvořák's *Bagatelles* op. 47 **[memory]**. In the orchestra: Strauss
  (*Ariadne*, Schiedmayer), Mahler 8, Schoenberg op. 20, Schreker's *Kammersymphonie* (1916), the
  Viennese chamber arrangements **[paper, K00]**.
- **Playing:** legato fingering as on the organ (nothing sustains after the key except through the
  wind); phrasing "the same nuances and breathings as the vocalist" **[paper, K00, Malkin]**; the
  expression pedalled continuously with both feet alternating, each stroke a push; the double
  expression to lift a treble melody over a pp bass; the prolongement for a bass pedal point while
  both hands play in the treble **[paper, K00; T05]**.
- **What a GM 21 part is [memory]:** hymn-like chords and sustained pads, a church or parlour
  accompaniment, some melody; sound sets play a soft, slightly nasal 8' reed with slow attack. Velocity
  is often constant; CC11 or CC7 swells; CC64 sometimes (sustain). The user's files have none
  **[file]**.

## 7. Recordings [rec]

- Searched `~/Music` (names: harmonium, reed organ, Karg-Elert, Schiedmayer, Mustel, Franck, Debain,
  organ): **no harmonium**. Found: `franck_track01.cdda.wav` (wavefield, copies in m1-m3, p1-p3 and two
  excerpts in `orchestral_samples`), `organ-9.wav`, `organ_7.wav`, `organ_full_12.wav` (the church
  organ recordings of `research/church_organ.md`).
- `franck_track01.cdda.wav`: 15:07, 44.1 kHz. Long-term spectrum (re total): 20-60 Hz **-11 dB**,
  60-125 -5, 125-250 -6, 250-500 -7, 0.5-1 k -10, 1-2 k -12, 2-4 k -19, 4-8 k -33, 8-16 k **-56 dB**;
  the final chord stops at ~899 s and falls ~30 dB in ~2 s (T60 ~4 s) to a floor at -48 dB. Strong
  energy below 60 Hz (16'/32' pedal), a dark top and a reverberant church: **a pipe organ (one of
  Franck's organ works), not a harmonium**; not separated with Demucs, not used.
- No GM 21 in the 301 MIDI files under `~/Music/midi` (program-change scan) **[file]**.
- So the model is fitted to the papers and by ear; a recording would help (§9).

## 8. Model plan [design]

### 8.1 Components and parameters

One `HarmoniumModel` (a `Tube`, like `AccordionModel`), reeds at 96 kHz, a half-band filter to 48 kHz
(`HalfBand` in `freereed.h`).

1. **Feet and feeders:** a target line pressure p_F(t) from the player (§8.3), reached with a time
   constant 15-60 ms (shorter for an accent); a treading ripple: the two feet alternate, each stroke
   ~0.4-0.8 s, the line pressure modulated ±2-5 % at the stroke rate with a dip at each changeover
   **[design; amount to fit by ear]**. A flow limit Q_max 8-12 L/s **[calc §2.3]**: heavy registrations
   at ff sag.
2. **Air space and reservoir:** with Expression off: the reservoir holds p_R = p_0 (1 + κ (V/V_max
   - 0.5)), p_0 = 800 Pa default, κ 0.1 (the springs stiffer as it fills), V integrating feed - use;
   the feet keep it 30-90 % full; a safety valve at full. With Expression on: the reservoir is out,
   the air space (V_L ~5 L, C = V_L/ρc²) is driven straight by the feeders: C dp_L/dt = Q_feed - Q_use.
3. **Double expression, per half h ∈ {bass, treble}:** chamber pressure p_h; knee open (or both latched,
   the default): p_h follows p_L through a small resistance (~1 ms); neutral: a regulator,
   p_h → min(p_L, p_pp), p_pp 200-300 Pa, its valve's opening ∝ the half's consumption, response
   ~20 ms **[paper, T05; design numbers]**.
4. **Stop compartments:** one per half-stop, p_s = p_h when the stop is drawn (a ~2 ms valve), shut
   otherwise.
5. **Reed + downstream cell + pallet, per key and row:** a variant of `FreeReed` (§8.2): the tongue,
   the gap, the slot's duct inertance (as now), the Bernoulli jet into the **cell** (V_c by pitch,
   §4.4), the cell emptying through the **pallet** (opening 0-1 from the key, area A_p, inertance
   ρ L_eff / A_p, Bernoulli loss). Builds by row: scale (broad: rows 1, 2, Bourdon; narrow: 4, 5, 6,
   Musette), the set/curvature as a brightness voicing knob, Q by pitch as in `accordionReed`,
   bass tongues 2-3× the accordion's (§4.1). Tuning by a table as `TuneTable` does (at 600 Pa).
6. **Percussion:** on rows 1 (bass and treble), a key-down with stop 1P drawn adds a tongue velocity
   v_0 = 1.5 m/s × hammer(velocity) at the key's pallet-opening moment (a `kick(v)` method on the reed)
   plus a felt "tick" (a 1-2 ms raised-cosine of the hammer's own thump, -30 dB) **[calc §4.3; design]**.
7. **Mechanisms:** Prolongement (MIDI 36-47 latch, one key, released by the next key or the release
   control); Grand Jeu (stops 1-4 both halves); Forte fixe / Forte expressif (lid opening g ∈ [0, 1]:
   fixed 1, or expressif g = clamp((p_h - 300)/1200, 0, 1) **[design]**); Métaphone; Tremolo flap (a
   6-8 Hz chopper on the treble Clarinette's compartment pressure, depth 30-60 %) **[design]**.
8. **Radiation:** per half, two groups: **front** (1, 2, 6: a gentle low-pass, one pole ~5 kHz) and
   **back** (3, 4, 5, 7, 8: through the shutters and lids: closed lid a two-pole low-pass ~1.8 kHz and
   -6 dB, open lid flat; métaphone shut an extra pole at ~1 kHz and -4 dB) **[design, to fit]**; the
   source at 1 m ρ/(4π) du_pallet/dt; bass a little left, treble a little right; the case's own few
   broad modes (100-300 Hz) optional.

### 8.2 Equations, per sample at 96 kHz

    stop compartment s of half h:   p_s = p_h (drawn) or 0
    for each sounding reed i in compartment s:
        reed:   z'' + (w/Q) z' + w^2 z = mu (p_s - p_c,i)              (FreeReed's exact step)
        jet:    M (du/dt) = p_s - p_c,i - rho/(2 alpha^2 S(z)^2) u|u|     (implicit, closed form, as now)
                u_tot = u + S_r z'
        cell:   (V_c / rho c^2) dp_c,i/dt = u_tot - u_p                    (implicit with the pallet)
        pallet: M_p du_p/dt = p_c,i - rho/(2 A_p(k)^2) u_p|u_p|          (A_p(k): the key's opening)
        out_g  += du_p/dt                                                  (g: the reed's group)
    half h:  Q_use,h = sum u_p over its reeds;  chamber/regulator update (§8.1.3)
    line:    reservoir or expression update (§8.1.2), feeders (§8.1.1)

- The jet and the cell are stiff together when the pallet is wide open and the cell small: solve u,
  p_c and u_p together (a 2×2 linearised step per sample, or two Newton steps), or keep the existing
  closed-form jet with p_c from the last sample and the cell implicit; check stability on C7 4' at
  2 kPa **[calc to do]**.
- The chambers' time constants (ms) are long against 10 µs: explicit coupling is fine **[calc]**.
- Accordion unaffected: the harmonium reed is a new class (or a flag) in `freereed.h`, not a change to
  `FreeReed`'s behaviour.

### 8.3 From MIDI to playing

- **Program:** GM 21 (zero-based 20) → `reed-organ`, a Kunstharmonium.
- **Default registration (to ask, §9):** Expression on; both knee levers latched (single expression
  everywhere); stops **1 + 4** both halves (Cor anglais + Basson / Flûte + Hautbois: Malkin's "light,
  crisp" 8'); Percussion on (prompt speech for short GM notes); forte off; no 16', no céleste.
  Alternatives as `PJ_HARM_REG`: `flute` (1), `full8` (1+4+Musette treble), `celeste` (1+6),
  `grandjeu` (1-4, playing at 8' means the 16' sounds an octave down), `soft` (Musette / Harpe).
- **Velocity:** (a) the feet: a chord onset sets p_F's target, 150 Pa × 10^(1.15 v/127) (pp 150 Pa -
  ff ~2.1 kPa) on GM's loudness curve as in `AccordionPlayer::pressure`; the push's time constant
  shorter for harder notes (60 → 15 ms); one pressure per half (shared by the chord); (b) the
  percussion hammer's strength; (c) with the two-depth touch on (Schiedmayer option): v < 40 back rows
  only.
- **CC11 / CC2 / channel pressure:** the feet, overriding velocity once seen (a held chord swells).
  **CC7:** volume. **CC1:** the treading tremolo depth (the foot vibrato), or the tremolo flap.
- **Double expression automatic (option):** when the top voice moves above a held lower part, the
  treble half follows the top voice's velocity and the bass half is regulated softer **[design]**.
- **CC64:** sustain = keys held (pallets stay open, as weights or the prolongement on every key); or
  Prolongement only (bottom octave, one note) as the authentic choice (to ask).
- **Pitch bend:** none physical (fixed reeds); ignore, or allow ±20 c as the pressure's small effect
  (no).
- **Range:** keys MIDI 36-96 (Dominator 33-101); fold outside notes by octaves.

### 8.4 Costs [calc]

- `FreeReed` ~20-40 ns per sample; the cell and pallet add ~10 ns: ~50 ns at 96 kHz = **~0.5 % of a
  core per reed**.
- Default (2 rows) × a 5-note chord = 10 reeds, ~5 %; Grand Jeu (4 rows) × 6 notes = 24 reeds, ~12 %;
  the full treble (10 reeds a key) × 4 notes + bass 6 × 2 = 52 reeds, ~25 %. Cap at ~60 reeds;
  skip silent compartments and rung-down reeds (`quiet()`).
- Wind, regulators and filters: negligible. Tune tables: one per row type at start-up (as the
  accordion's, ~100 ms each).

### 8.5 Build order

1. The harmonium reed (downstream cell, pallet) and its tests alone (§8.6 a-d).
2. One half-stop row (1) on a fixed reservoir: tuning table, chords.
3. The wind: feeders, reservoir, expression, the regulators of the double expression.
4. All rows of §3.1, the split, the céleste rows, the radiation groups, forte and métaphone.
5. Percussion, prolongement, Grand Jeu, the tremolo; MIDI mapping; route GM 21 in `midifile.cc`.
6. Fit by ear (no recording) and against §4-5's numbers.

### 8.6 Tests

- a. **Topology:** pallet shut → cell pressure = chamber pressure, no flow, no sound; opening it →
  sound within 5 ms + the reed's growth.
- b. **Cell:** the transfer from reed flow to pallet flow peaks at the §4.4 f_H (±15 %); a quarter-open
  pallet lowers it ~2×; the cell lowers the reed's pitch a few cents at fixed pressure and raises the
  1-4 kHz harmonics (C97, AK4).
- c. **Pitch vs pressure:** 0.2 → 2 kPa moves each reed by -2 to -15 c, monotonic in the playing
  range (C13).
- d. **Onset:** t90 longer in the bass than the treble at the same pressure; near the threshold
  ≥ 100 ms; with percussion t90 at least 2× shorter and an audible click.
- e. **Tuning:** every row of §3.1 within ±2 c at 600 Pa; the 16', 4', 32', 2' rows at their octaves;
  the split exactly at 64/65 (a key's rows switch halves there).
- f. **Céleste:** the beat rate at A4 = the set value ±0.2 Hz, its rise with pitch as designed.
- g. **Reservoir vs expression:** Expression off: the level of a held chord stays within ±0.5 dB
  while velocity and CC11 vary (no expression); adding notes sags < 5 %. Expression on: CC11 0 → 127
  raises the level ≥ 25 dB and the centroid ≥ 2× **[design target]**; a sforzando (ff for 50 ms then
  p) shows in the envelope within 30 ms.
- h. **Double expression:** with one knee neutral, sweeping the feet changes that half's level
  < 1 dB while the other half follows.
- i. **Forte / métaphone:** opening the lids raises the back rows' 2-4 kHz band ≥ 6 dB; the métaphone
  lowers it ≥ 4 dB; front rows unaffected.
- j. **Prolongement:** a bottom-octave key latched; a second releases the first; keys above 47 not
  latched.
- k. **Wind limit:** Grand Jeu, 8-note ff chord: pressure sags (feeders saturated) and recovers.
- l. **Real time and safety:** 52 reeds under 30 % of a core; fuzz (random stops, keys, CC11, knee
  levers) with no NaN; `resetAir` on NaN as `Tube` does.

## 9. Open questions for the user

1. **Default registration for GM 21:** 1 + 4 (Cor anglais/Basson, Flûte/Hautbois, crisp), 1 alone
   (flute-like, soft), 1 + 6 (Voix céleste: the classic "reed organ" shimmer), or Musette-based
   (soft, nasal)? Percussion on by default?
2. **Expression by default** (pressure from velocity/CC11), or the reservoir's steady wind when a file
   sends no CC11 (an organ-like constant level)?
3. **CC64:** keys sustained (practical), or the authentic Prolongement (bottom octave, one note)?
4. **How Schiedmayer:** the plain Mustel/Titz Kunstharmonium (§3.1), or the Dominator's extras
   (Violoncello, Undamaris, Vox angelica, A1-F7, the two-depth touch)? The celesta is GM 9 already.
5. **Céleste beat rates:** ~1.5 beats/s at A4 for the Voix céleste, 3-5 for the Harpe éolienne?
6. **A recording?** Any harmonium (Karg-Elert or Franck on a Mustel, Schiedmayer or Titz) to fit the
   spectra, the attacks and the case's radiation; none is in `~/Music`.
7. **Pitch:** A = 440, or 435 (most period instruments) as an option? **Answered 2026-11-16: A = 440** (as the other GM instruments).

## 10. Sources

- **[T05]** J. Titz, *Titz-Kunst-Harmonium* (Berlin, c. 1905-1910; photographs and description of a
  Mustel-type Kunstharmonium: wind system, expression, double expression, prolongement, percussion,
  forte, métaphone); reprint U. Averesch, Wuppertal 2000.
  https://www.harmonium.gdo.de/fileadmin/ak_harmonium/veroeff/Titz-Kunst-Harmonium.pdf
- **[K00]** (Koenig), *Das Harmonium als Orchester- und Ensembleinstrument* (Vienna, c. 2001; GdO
  Arbeitskreis Harmonium): Schiedmayer's catalogues (H 104; the Dominator), E. Malkin *The Orchestral
  Organist and the Mustel Organ* (London, after 1921), Karg-Elert's *Die Kunst des Registrierens*
  (KdR), Kotykiewicz's dispositions, the Tremolo-Register, *Ariadne*, Schreker, Schoenberg.
  https://www.gdo.de/fileadmin/_migrated/content_uploads/Koenig_Harmonium-als-Orchester-und-Ensembleinstrument.pdf
- **[MF]** M. Fitze, *Moussorgski: Tableaux d'une exposition*, CD booklet: Mustel Orgue-Célesta no.
  484-37 (1889), stop list, 455 Hz. https://vdegallo.com/wp-content/uploads/2026/04/CD-1777-Booklet.pdf
- **[LEM]** Edwin H. Lemare's Mustel harmonium (stop list, French and English names).
  https://edwinlemare.com/mustel-harmonium/
- **[FA23]** "Clamped bar model for free reeds", Forum Acusticum 2023 (an Indian harmonium's B5 reed,
  16 × 2 × 0.4 mm; chamber pressures 0.2-1.5 kPa; LDV). https://dael.euracoustics.org/confs/fa2023/data/articles/000853.pdf
- **[CW15]** J. Cottingham, D. Wolff, "Experimental study of free reed initial transients", Vienna Talk
  2015. https://viennatalk2015.mdw.ac.at/abstracts/ViennaTalk2015_submission_39.pdf
- **[C17]** J. Cottingham, "Reed chamber resonances and reed tongue vibrational modes in free reed
  excitation", ISMA 2017. https://isma2017.cirmmt.mcgill.ca/proceedings/pdf/ISMA_2017_paper_52.pdf
- **[C13]** J. Cottingham, "Reed vibration and pitch bending in Western free reed instruments", CCRMA
  mini-course slides, 2013. https://ccrma.stanford.edu/courses/318/mini-courses/Winter2013/Cottingham-2.pdf
- **[C95]** J. Cottingham, B. Colson, S. Wilson, K. Quigley, "Sound spectra from air-driven American
  organ reeds", ASA 129 (1995). https://auditory.org/asamtgs/asa95wsh/5aMU/5aMU10.html
- **[C96]** J. Cottingham, "The reed organ in North America and Japan", ASA 132 (1996).
  https://auditory.org/asamtgs/asa96haw/4aMUb/4aMUb7.html
- **[C97]** J. Cottingham, "Effects of reed cell geometry on the vibration frequency and spectrum of a
  free reed", ASA 133 (1997) (= free_reeds.md's CRB link). https://auditory.org/asamtgs/asa97pen/4aMU/4aMU1.html
- **[CF97]** J. Cottingham, C. Fetzer, "Modeling free reed behavior using calculated reed admittance",
  ASA 134 (1997). https://auditory.org/asamtgs/asa97snd/2aMU/2aMU3.html
- **[CRB] [CH]** as in `research/free_reeds.md` (frequency vs pressure, 0.5-3.5 kPa; hysteresis).
  https://auditory.org/asamtgs/asa96haw/4aMUb/4aMUb8.html
- **[SWB71]** A. O. St Hilaire, T. A. Wilson, G. S. Beavers, "Aerodynamic excitation of the harmonium
  reed", J. Fluid Mech. 49 (1971) 803-816 (abstract only: exponential growth vs flow, unsteady
  potential flow). https://resolve.cambridge.org/core/journals/journal-of-fluid-mechanics/article/aerodynamic-excitation-of-the-harmonium-reed/3B5156625194BBF5549B08928B2F5495
- **[AK4]** *Arbeitskreis Harmonium* Heft 4 (GdO, 2002): a summary of Cottingham's "Free reed acoustics:
  experimental and theoretical studies of the American reed organ" (cells boost 1-4 kHz).
  https://harmonium.gdo.de/fileadmin/_migrated/content_uploads/AK-Harmonium-Heft4.PDF
- **[MB07] [EMS21]** as in `research/free_reeds.md`.
- **[web, LH]** Lindholm harmonium practice notes (springs, scales, frames, tone-hold, beating sets).
  http://home.snafu.de/raschdorf/lindholm/praxis.html
- **[web, CP]** C. Pykett, "The physics of free reeds". http://www.colinpykett.org.uk/physics-of-free-reed-wind-instruments.htm
- **[web, BARS]** Musée de l'harmonium de Bars: métaphone; double expression.
  https://sites.google.com/view/museedelharmoniumdebars/fonctionnement-de-lharmonium/métaphone ;
  https://sites.google.com/view/museedelharmoniumdebars/fonctionnement-de-lharmonium/double-expression
- **[web, FRW] [DEW]** Wikipedia, "Harmonium" (French; German). https://fr.wikipedia.org/wiki/Harmonium ;
  https://de.wikipedia.org/wiki/Harmonium
- **[web, HC]** harmoniums.com, history of the reed organ. https://www.harmoniums.com/hist_en.htm
- **[web, SCH]** Schiedmayer: https://en.wikipedia.org/wiki/Schiedmayer ;
  https://www.namm.org/video/orh/elianne-schiedmayer
- **[web, ROW]** reed-organ suction pressures and blowers: https://www.mmdigest.com/Archives/Digests/200303/2003.03.05.09.html ;
  https://organforum.com/forums/forum/reed-organs-and-accordions/pump-reed-organs/34227-bellows-vacumm-how-long-should-they-hold-it ;
  pipe organ pressures: https://www.thediapason.com/content/wind-72
- **[web, IMSLP]** Karg-Elert: https://www.imslp.org/wiki/Die_Reform_des_modernen_Druckwind-Harmoniums_(Karg-Elert,_Sigfrid) ;
  https://imslp.org/wiki/Kompositionen_f%C3%BCr_Kunstharmonium,_Op.26_(Karg-Elert,_Sigfrid) ;
  https://imslp.org/wiki/Die_Kunst_des_Registrierens,_Op.91_(Karg-Elert,_Sigfrid)
- **[web, GdO FAQ]** pressure or suction: https://harmonium.gdo.de/faq/ist-mein-harmonium-ein-druckwind-oder-ein-saugwind-harmonium-wie-kann-ich-das-feststellen
- Not reached: the Met Museum's Mustel Orgue-Célesta record (HTTP 429), Karg-Elert's *Reform* and
  KdR texts themselves, Mustel's *École*, Schiedmayer's catalogue images, any measured harmonium
  pressure, beat rate, spectrum or SPL.

## 11. Built (2026-11-16)

`src/harmonium.cc`, `src/harmonium.h`, and `FreeReed::sampleDown` / `kick` in `src/freereed.h`
(the accordion's and the harmonica's `sample()` unchanged). §9 answered with the defaults: stops
1 + 4 with Percussion; Expression always on (`PJ_HARM_EXPR=0` for the reservoir); CC64 holds the
keys; the plain Kunstharmonium; the céleste 1.5 beats/s at A4; A = 440.
- **The downstream cell speaks** (§4.4's topology): from ~150 Pa across C2-C7 with the
  accordion's voicing. Onsets are slower in the bass (t90 ~100 ms at C2, 7 ms at C7, 600 Pa).
  Percussion's kick cuts the bass's t90 ~3x and the treble's a little.
- **Fitted [design]:**
  - **The pallet's felt:** the cell's resonance undamped stood ~+5-8 dB over the fundamental at
    harmonics 11-13 of C4 (a centroid ~2-3 kHz). A linear loss on the pallet's air (Q 2) gives a
    fundamental-led tone (centroid ~0.6-0.9 kHz at C4). The top reeds have Q 6, or they would not
    speak.
  - **Voicing:** with the accordion's voicing the pitch fell 30-60 c from a soft to a hard wind,
    and up to 66 c flat at C2. Half the set and twice the stiffness (closing pressure 3 kPa) bring
    it to a few cents, as Cottingham's reed-organ reeds. The tongues at the ends are softer (x0.5),
    so the softest wind (~300 Pa) speaks there.
  - **Wind:** the feet's pressure is 300 x 7.3^(v/127) Pa. Velocity 30 -> 127 gives +14 dB, the
    centroid 0.64 -> 0.89 kHz.
- **Not yet:** the double expression's two regulated halves (both knee levers are latched, so
  the halves share one wind); the forte lids and the métaphone as controls (the lids stay
  closed); the Prolongement; the tremolo flap; the Musette, Baryton and Harpe éolienne rows; the
  Dominator; the hammer's own felt tick. No recording to fit to.

