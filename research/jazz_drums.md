# A jazz drum kit: cymbals, hi-hat, snare and brushes as physical models — research and model plan

Status: 2026-11-15. Research only; nothing is built. Scope: the General MIDI drum channel (10, keys 35–81)
as a jazz drummer uses it, with the GM2/GS Jazz (33) and Brush (41) kits. Most depth goes to the
cymbals, the hi-hat and the snare with sticks and brushes. SI units.

This file extends `research/drums.md` (goals, the fitting method, the brush strokes as playing
technique) and `research/bells_blocks.md` §4 (the reverse cymbal: crash modes, a damping law, the Hertz
contact table). It does not repeat them.

Where each number comes from is marked:
- **[paper]**: a paper read for this document (full text), listed in §11.
- **[web]**: a web page, a manual, a patent, or a paper seen only as an abstract or excerpt.
- **[calc]**: computed here from those numbers. The formulas are given inline.
- **[memory]**: general literature or practice as remembered, not looked up for this document. Check it
  before relying on it.

## 1. Short answer

- **Cymbals are the hard part, and the strike level decides how hard.** A cymbal is a thin, tapered
  bronze shell, 0.4–1.5 mm thick **[paper, Bao19]**. A 20–22" ride has about **10–15 Hz between
  modes** and **1300–1900 modes below 20 kHz [calc]**.
  - **Struck softly, it is linear.** A plain mode bank does it, and the project's engine already
    runs one (the reverse cymbal, 192 modes).
  - **Struck hard, the motion is a few times the thickness and the plate becomes nonlinear.**
    Energy cascades from the low modes to the high ones (wave turbulence). This is the crash's
    bloom and the gong's swell.
  - **How fast the cascade runs depends on the edge.** On a thin-edged crash it is "immediate";
    on a gong it takes up to 0.5 s **[paper, Bao19]**.
  - **A jazz ride is mostly in the weakly nonlinear regime.** That is why it keeps its ping: a
    stick-tip stroke at jazz dynamics moves the bow by less than its thickness. The wash is mostly
    the long linear decay of many dense modes, built up over many strokes. Nonlinearity adds the
    slight glide and the spread at accents.
- **Full von Kármán models are not real-time at full bandwidth.**
  - *Modal von Kármán.* The cost per step is about **N_Ψ · N_Φ²**: in-plane modes times the
    square of the transverse modes **[paper, DT/DAFx15; calc]**. 100 modes to 9 kHz ran at 8×
    real time in MATLAB; 800 modes to 19 kHz ran at f_s = 100 kHz offline **[paper, Bao19]**.
  - *Finite-difference plate.* Bilbao et al. ran a flat steel gong plate in real time on one core,
    at about 0.5 s of CPU per second for 0.06 m². A 20" bronze ride would be about **1–2 cores
    [calc]**: too much for a kit with five cymbals.
  - **Proposed for this project: a hybrid on the existing modal engine.** A nonlinear core of the
    lowest ~100–200 modes (to ~2–3 kHz) uses the analytic von Kármán coupling of a free circular
    plate, with its selection rules (sparse) plus a quadratic term for the dome. The core runs at
    a decimated rate (24 kHz is enough for modes below 3 kHz). It feeds a linear bank of ~1000–1500
    high modes at 48 kHz through an energy-conserving "cascade sink". Cost: about **0.1–0.5 core
    per cymbal while nonlinear, ~0.05 core once linear [calc]**.
- **The hi-hat is two cymbals with a contact ring.**
  - **Closed:** pre-loaded contact. The rims are pressed together, so the pair is much more
    strongly damped, especially at low frequency, and stiffer.
  - **Half-open:** intermittent rim contact, the sizzle.
  - **Foot "chick":** the plates clap shut. Squeezing the air out of the gap gives a rim jet of
    **10–90 m/s [calc]**, which is part of the chick's breathy noise.
  - **The only published physical model** (Sekiguchi & Samejima 2023: finite differences on two
    spherical shells at 480 kHz) found that a pseudo-inelastic contact sounds more like a real
    hi-hat than a stiff spring contact **[paper]**.
  - **In this project's terms:** two modal plates (~400–700 modes each) and 8–16 contact patches
    around the rim, using the engine's existing contact solver, with a pedal state.
- **The snare is two membranes coupled through the air, with wires rattling on the bottom head.**
  - **Batter head:** measured **(0,1) 224 Hz, (1,1) 280–322 Hz, (2,1) 403–457 Hz**, with 1–5 %
    damping **[paper, Skrodzka06, with Rossing's values]**. That fits T ≈ 3000 N/m on a 14" head
    **[calc]**.
  - **Published models:** finite-difference models in 3D air (Bilbao 2012; Torin, Hamilton and
    Bilbao 2014: energy-conserving collisions, Newton–Raphson) and a modal SAV scheme, explicit
    and fast (Ducceschi, Hamilton and Russo 2023) **[paper]**.
  - **In this project's terms:** two modal heads, the air as a linear coupling of the low modes,
    and 8–20 wires as small modal strings with contact patches on the snare head. All of this is
    within what `ModalSet` already does (contacts, linear coupling), only scaled up.
- **Brushes: no published physical model or acoustic measurement was found.**
  - Practice (a trade article) and one patent agree on what is heard **[web]**. A tap is many
    small wire impacts spread in time. A sweep is friction noise from the wires dragged over the
    coated head. Its colour changes once and twice per revolution.
  - **A model that follows from that:** a "scraping" exciter. Each of a few virtual bristle
    groups reads a texture profile at the contact speed, and its force enters the head's modes at
    the moving contact point. The head's own mode shapes then move the colour with the hand.
- **The kit switch (the user's choice): a program change on channel 10.**
  - **What the kits change:** GM2/GS **Jazz (program 33, sent as 32)** changes only the kicks:
    35/36 become Jazz BD 2/1. **Brush (41, sent as 40)** changes the same kicks, plus **38 Brush
    Tap, 39 Brush Slap, 40 Brush Swirl**. Every other key, including all cymbals and hi-hats,
    keeps the Standard kit's sound **[web]**.
  - **Which kit a file gets:** a GM1 file with no program change gets the Standard kit.
  - **The plan:** track the program per drum channel and map program families (33–40 Jazz,
    41–48 Brush). Hold the swirl from note-on to note-off, and shape it with pressure. With the
    Brush kit, the cymbals are played by the brush too (§2.4).
- **Order of building:**
  1. Ride, linear first, then the nonlinear core.
  2. Hi-hat (two plates, contact, pedal).
  3. Snare with sticks, then brushes.
  4. Kick and toms (membranes, which the snare work provides).
  5. Crash/splash/china (the same cymbal code, thinner, more nonlinear).
  6. The rest of the GM map, mostly on existing instruments.

## 2. The GM drum map, the Jazz and Brush kits, and kit switching

### 2.1 GM keys 35–81: jazz use, what the project has, what each needs

Today `src/midifile.cc` `drumFor()` maps: 35/36 → taiko; 37, 75–77 → woodblock; 38–40 → synth-drum
(lead.cc); toms 41–50 → melodic-tom (lead.cc); 56, 67, 68 → agogo; 60–66 → melodic-tom. **Everything
else is silent**, including all cymbals and hi-hats. GM names are **[memory]**, cross-checked against the
Standard kit list in **[web, voidaudio]** for 41–59.

| Key | GM name | Jazz use | Project today | Needs |
|---|---|---|---|---|
| 35 | Acoustic Bass Drum | feathered quarters, "bombs" | taiko | membrane kick: 18"/20" (§6) |
| 36 | Bass Drum 1 | same | taiko | same object as 35, other beater spot/velocity |
| 37 | Side Stick | cross-stick on ballads and bossa | woodblock | snare + stick on rim (§5.5) |
| 38 | Acoustic Snare | comping, accents | synth-drum | snare (§5) |
| 39 | Hand Clap | rare | synth-drum | many-hand clap noise (cheap; existing sfx/applause style) |
| 40 | Electric Snare | (2nd snare) | synth-drum | snare, rim-ward strike or tighter tuning |
| 41, 43 | Low/High Floor Tom | fills | melodic-tom | membrane tom (§6) |
| 42 | Closed Hi-Hat | stick on closed hats | — | hi-hat (§4) |
| 44 | Pedal Hi-Hat | **2 and 4 with the foot** | — | hi-hat, foot chick |
| 45, 47, 48, 50 | Low … High Tom | fills | melodic-tom | membrane toms |
| 46 | Open Hi-Hat | accents, swish | — | hi-hat, open |
| 49, 57 | Crash Cymbal 1/2 | accents, endings | — | thin crash plates (§3.7) |
| 51 | Ride Cymbal 1 | **the time: spang-a-lang** | — | ride (§3) |
| 52 | Chinese Cymbal | colour | — | china (§3.7) |
| 53 | Ride Bell | accents, Latin | — | **the same cymbal as 51**, struck at the bell with the shoulder |
| 54 | Tambourine | rare | — | jingle rattle (contact model, like the rivets) |
| 55 | Splash Cymbal | colour | — | small thin plate |
| 56 | Cowbell | Latin | agogo | cowbell modes (agogo code, new mode list) |
| 58 | Vibraslap | rare | — | rattle box (sfx) |
| 59 | Ride Cymbal 2 | second ride (sizzle or flat) | — | second ride plate, rivets |
| 60–66 | Bongos, congas, timbales | Latin jazz | melodic-tom | hand membranes (drums.md Latin kit) |
| 67, 68 | High/Low Agogo | Latin | agogo | fine |
| 69, 70 | Cabasa, Maracas | Latin, bossa | — | granular shaker (many-bead rattle) |
| 71, 72 | Short/Long Whistle | samba | — | whistle instrument exists (pipes) |
| 73, 74 | Short/Long Guiro | Latin | — | scraper: the brush's scraping exciter on a gourd's modes |
| 75 | Claves | clave | woodblock | fine (rosewood modes) |
| 76, 77 | Hi/Low Wood Block | | woodblock | fine |
| 78, 79 | Mute/Open Cuica | samba | — | friction drum: the bowed-membrane path (later) |
| 80, 81 | Mute/Open Triangle | | — | bar modes; mute = damping (tinkle-bell code) |

**Shared objects.** One physical object answers several keys, and its state carries over between them:
- 51/53 are one ride: the bell stroke excites the same modes, so the wash builds across both.
- 42/44/46 are one hi-hat, with a pedal state.
- 35/36 are one kick. 38/40/37 are one snare; 39 is not.
- 49/57 are two crashes, but the wash of each one persists.

GM exclusive classes **[memory]**: 42/44/46 choke each other, and so do 71/72, 73/74, 78/79 and 80/81.
On a physical hi-hat the choke follows from the pedal (§4.5). It needs no rule.

### 2.2 The Jazz and Brush kits, key by key

GM2 drum sets (Bank Select MSB 120, LSB 0, then a program change; 1-based numbers): 1 Standard, 9 Room,
17 Power, 25 Electronic, 26 TR-808, **33 Jazz**, **41 Brush**, 49 Orchestra, 57 SFX. GM2 percussion
adds keys 27–34 and 82–87 to GM1's 35–81 (27 High Q, 28 Slap, 29/30 Scratch Push/Pull, 31 Sticks,
32 Square Click, 33/34 Metronome Click/Bell; 82 Shaker, 83 Jingle Bell, 84 Belltree, 85 Castanets,
86/87 Mute/Open Surdo) **[web, Wikipedia GM2]**.

The Roland GS / GM2 key map, compared with the Standard kit **[web, voidaudio; PG Music]**:

| Key | Standard (1) | Jazz (33) | Brush (41) |
|---|---|---|---|
| 35 | Kick Drum 2 | **Jazz BD 2** | **Jazz BD 2** |
| 36 | Kick Drum 1 | **Jazz BD 1** | **Jazz BD 1** |
| 37 | Side Stick | Side Stick | Side Stick (a stick sound, kept) |
| 38 | Snare Drum 1 | Snare Drum 1 | **Brush Tap** |
| 39 | Hand Clap | Hand Clap | **Brush Slap** |
| 40 | Snare Drum 2 | Snare Drum 2 | **Brush Swirl** |
| 27–34, 41–87 | Standard | unchanged | unchanged |

- **Cymbals and hi-hats (42–59) are unchanged in both kits.** The tables list no change from 41 up,
  in either kit **[web, voidaudio]**.
  - A sampled module therefore plays stick cymbals under brush snare notes. That is a sample-set
    economy, not what a drummer does. With brushes in hand, the ride, the hi-hat strokes and the
    toms are played with the brush. The pedal hi-hat (44) and the kick are feet and do not change.
- **The Jazz kit is "nearly identical to Standard"**, with softer kick and snare sounds **[web, PG
  Music; Kurzweil]**. In the GS key map only the kicks differ **[web, voidaudio]**. So "jazz snare"
  is a voicing (tuning, heads), not a different key.
- **Program numbering:** the bytes on the wire are 0-based, so Jazz = 32 and Brush = 40 **[web,
  Kurzweil manual via search]**.
- Later Roland modules (SC-88 and later) add variants next to each set (e.g. a "Jazz 2"), so programs
  33–40 belong to the Jazz family and 41–48 to the Brush family **[memory]**. Yamaha XG and Korg maps
  differ **[web, search snippets]**: this plan follows GS/GM2.

### 2.3 How the player should handle the kit switch

- **Channel 10 is always the drums in GM1 and GM2.** A program change there selects the kit. It does
  not select a melodic program **[web]**.
  - GM2 defines the rhythm bank as **Bank Select MSB 120 (78h), LSB 0**. Channel 10 is in that bank
    by default, so a bare program change suffices.
  - Sending MSB 120 on **channel 11** turns it into a second rhythm channel; MSB 121 returns it to
    melodic **[web, MIDI.org forum, Wikipedia]**.
  - GS needs no bank select for drum sets on channel 10 **[memory]**.
- **A file with no program change on channel 10 (any GM1 file) gets the Standard kit.**
  - For this project "Standard" can still be the jazz-voiced kit when the user asks. A command-line
    option (`-kit jazz|brush|standard`) would override what the file selects. That is useful
    because most jazz MIDI files never send program 33 or 41 **[memory: common practice]**.
- **Mapping programs to kits (proposal):** use the family of the 1-based program p, (p − 1) / 8:
  1–8 Standard, 9–16 Room, 17–24 Power, 25–32 Electronic/808, 33–40 Jazz, 41–48 Brush, 49–56 Orchestra,
  57+ SFX. Map unbuilt families to Standard.
- **Mid-song kit changes:**
  - A switch takes effect for **new notes**. Ringing cymbals and the snare's decay continue: it is
    the same physical kit, and the drummer swapped implements.
  - Only the *implement* (stick or brush) and the voicing parameters change. The objects (plates,
    heads) stay, so their state is continuous.
  - Switching Jazz ↔ Brush is a change of implement, not of drums.

### 2.4 The brush swirl (key 40 in the Brush kit) as a sustained note

- **What modules do.** GS modules mostly ignore note-off on drum keys: each drum instrument has an
  `Rx.NoteOff` switch, off by default. The swirl is a fixed-length sample **[web, Roland manual
  snippet; memory]**. GM1's clarifications single out Long Whistle and Long Guiro as answering note-off
  **[web, GM1 clarification index]**. No GM2 text on the swirl's note-off was found.
- **Proposal for this project (physical, playable):**
  - **Note-on starts the stroke.** The brush lands and starts circling at the head's centre radius
    (~8–12 cm).
    - Velocity sets the pressure, i.e. the bristles' normal force (and so the noise level, ~20·log₁₀
      of the force).
    - Velocity also gives a small landing tap (a light Brush Tap at about 0.2× the velocity).
  - **While held:**
    - **Channel pressure (D0h)** or **polyphonic key pressure (A0h, key 40)** sets the pressure live
      (the brush pressed harder).
    - **The circle rate follows the tempo.** The MIDI file's tempo map is known: one revolution per
      beat above ~100 bpm, one per two beats below **[memory: practice; drums.md]**.
    - A small random wobble in radius and rate, ±10 %, keeps it alive.
  - **Note-off lifts the brush:** pressure to zero over 30–60 ms, with a small "flick" (the tail of
    the stroke) **[memory]**.
  - **A note shorter than ~80 ms** comes from a trigger-style file written for a sample module. It
    plays a one-beat stroke (about a half circle), not an 80 ms stroke. Notes from 80 ms up to the
    held length are followed exactly.
  - **Cap:** a held swirl with no note-off ends after 30 s (a lost note-off).
- **The other brush keys:**
  - **38 Brush Tap:** a vertical stroke, the fan tilted. Some wires land first; the spread is 2–5 ms.
  - **39 Brush Slap:** the fan lands flat. More wires and a larger footprint: louder and darker.
  - **Other strokes** (sweep-and-tap, rolls; drums.md §4.4) can go on keys the Brush kit leaves free,
    27–34 or 82–87. A GM2 file would only play them by accident, and none of them sound in a GM1 file.

### 2.5 What `src/midifile.cc` needs

1. **Track the kit.**
   - `prog[9]` is already recorded on program change (0xC0) but ignored by `partOf()` for channel 9.
     `drumFor(key)` becomes `drumFor(key, kit)` with `kit = family(prog[9])`.
   - Also honour Bank Select MSB 120/121 (CC0) on channel 11 (index 10), to support GM2's second
     rhythm channel (optional).
2. **One part for the kit.** Today every drum key goes to its own part (one `Ensemble` per instrument).
   The kit should be **one instrument** with per-key routing inside. Only then do 42/44/46 share a
   pedal, 51/53 share a ride, and the snare hear the kick (§5.3).
3. **Forward the controls the kit uses:**
   - **CC4** (foot controller): hi-hat pedal position, the usual e-drum practice **[memory]**.
   - **Poly aftertouch (A0h):** cymbal choke by the hand on 49/52/55/57, and swirl pressure on key 40.
   - **Channel pressure (D0h):** swirl pressure. Today it goes to `setBreath` as 0.5 · value.
4. **Pass note-offs to the drum part.** That already happens, through `noteOwner`/`heldKey`, and only
   the swirl uses them.
5. **Keep the tempo map** (now converted to seconds while reading) available to the kit, for the swirl's
   circle rate. A per-event "seconds per beat" field is enough.
6. **Live (MPK pads, channel 10):** the same routing. The pads' pressure gives the swirl's pressure.

## 3. Cymbals

### 3.1 Geometry and alloys

| Quantity | Value | Source |
|---|---|---|
| Alloys | B20 (CuSn20 bronze) for most professional and jazz cymbals; B8 for bright, cheaper lines; brass for the cheapest | [memory] |
| Bronze, as modelled in a hi-hat simulation | ρ = 8860 kg/m³, E = 110 GPa, ν = 0.33 | [paper, SS23] |
| Edge thickness h_e, crashes | 0.9–0.5 mm; mean 0.65–0.7 mm; thinnest: Paiste Fast Crash 14", Sabian AA Thin crash, Paiste 602 "paperthin" (used as hi-hats) | [paper, Bao19] |
| Centre thickness h_c | 0.9–1.4 mm (crashes); up to 1.5 mm (rides) | [paper, Bao19] |
| Rides, edge | 1.4–0.8 mm | [paper, Bao19] |
| Splashes, edge | 0.55–0.65 mm; minimum 0.4 mm (Sabian AAX 8") | [paper, Bao19] |
| Dome height of a 16" crash (Zildjian A Custom Rock-type), from the surface profile | H_c = 3.4 cm (bell + bow profile) | [paper, Bao19] |
| Curvature radius (one-curvature spherical model of a 14" hi-hat) | R_s = 0.8 m | [paper, SS23] |

Typical masses and mean thicknesses. The masses are typical catalogue weights **[memory]**. The mean
thickness is h̄ = m / (ρ π R²) with ρ = 8600 kg/m³, ignoring the dome **[calc]**:

| Cymbal | Mass (g) | h̄ (mm) |
|---|---|---|
| 22" thin jazz ride (K Constantinople-type, hammered, lathed) | 2300–2600 | 1.1–1.2 |
| 20" medium ride | 2300–2600 | 1.3–1.5 |
| 20" flat or sizzle ride | 1900–2200 | 1.1–1.25 |
| 18" thin crash | 1300–1450 | 0.9–1.0 |
| 16" thin crash | 950–1150 | 0.85–1.0 |
| 10" splash | 200–300 | 0.5–0.65 |
| 18" china | 1150–1400 | 0.8–0.95 |
| 14" hi-hat, top | 950–1150 | 1.1–1.35 |
| 14" hi-hat, bottom (heavier) | 1250–1500 | 1.45–1.75 |

These agree with the calliper ranges above (h̄ lies between h_e and h_c).

The **dark, dry jazz ride** (K Constantinople, Bosphorus, Istanbul, Paiste Dark Energy-type)
**[memory]**:
- It is thinner and more hammered than a rock ride, and often unlathed or partly lathed.
- Hammering leaves local stresses and thickness variations. They split the degenerate mode pairs and
  scatter the modes, which gives more beating and a "complex", trashier wash with less pitch.
- Thinness lowers the modes, so it sounds darker, and makes the cymbal easier to drive nonlinear
  (more wash).
- A drier cymbal is heavily hammered and has a small bell: the highs decay faster and the wash
  builds less.
- Paiste's own glossary: "ping" is the initial sound right after the stick tip strikes, *including the
  stick's wood sound*. A dry ride is "focused, controllable"; a wetter one builds a wash **[web, Paiste
  glossary]**.

### 3.2 Modal density

The asymptotic modal density of a thin plate is n(f) = A√3 / (h c_L) per Hz, with c_L = √(E/(ρ(1−ν²)))
= 3790 m/s for bronze. It counts both members of each degenerate pair. Areas use the full disc,
h = h̄ **[calc]**:

| Cymbal | Spacing (Hz) | Modes < 5 kHz | < 10 kHz | < 20 kHz |
|---|---|---|---|---|
| 22" thin ride | 10.4 | 480 | 970 | 1930 |
| 20" medium ride | 14.9 | 340 | 670 | 1350 |
| 20" sizzle/flat ride | 12.4 | 400 | 810 | 1620 |
| 18" thin crash | 12.7 | 390 | 790 | 1570 |
| 16" thin crash | 15.9 | 320 | 630 | 1260 |
| 10" splash | 24.8 | 200 | 400 | 810 |
| 18" china | 11.8 | 420 | 850 | 1700 |
| 14" hi-hat top | 27.1 | 190 | 370 | 740 |
| 14" hi-hat bottom | 34.8 | 140 | 290 | 580 |

- Bao & Touzé's 0.2 m-radius plate with h₀ = 1 mm (steel): first mode 32 Hz, **mode 800 at 18 861 Hz**
  **[paper, Bao19]**. The formula gives 830 modes there for steel **[calc]**, so it is a good count.
- **Low modes of real cymbals** (Rossing, via bells_blocks.md) **[web]**:
  - 18" crash: (3,0) 64 Hz, (4,0) 99 Hz, (5,0) 145 Hz, (6,0) 188 Hz.
  - Modes of the (m,0) type (m nodal diameters) dominate below ~1 kHz. Above that the shapes mix.
- **The dome stiffens the low modes**, which lifts them and limits the pitch glide **[paper, Bao19]**.

### 3.3 Damping

| Law | Values | Source |
|---|---|---|
| Thin lab plate, h = 0.5 mm (Humbert) | ξ = 8·10⁻³ ω^−0.4: T60 18 s at 100 Hz, 4.5 s at 1 kHz, 1.7 s at 5 kHz | [paper, DAFx15]; T60s from bells_blocks.md [calc] |
| Bao & Touzé's cymbal law | μ_k = 0.007 ω_k^0.6 + 2 | [paper, Bao19] |
| Bao's law read as an amplitude decay rate in s⁻¹ (the paper's convention is not restated here) | T60 = 6.91/μ: 3.0 s at 100 Hz, 2.1 s at 1 kHz, 1.5 s at 3 kHz, 0.95 s at 10 kHz | [calc] |
| Bilbao's gong scheme, as parameters | T60 = 20 s at 0 Hz, 10 s at 1 kHz | [paper, BWWD23] |
| A crash on a stand (targets) | ~4–8 s at 100–500 Hz, ~3 s at 1 kHz, 1.5–2.5 s at 3–5 kHz, < 1 s above 10 kHz | bells_blocks.md [memory] |

- **A ride rings longer than a crash at low and mid frequencies:** it is heavier and the stand's felt
  takes a smaller share **[memory]**. Its band T60s are to be measured (§9).
- **The felt and sleeve at the centre** add damping, strongest for the low (m,0) modes, whose motion
  reaches the centre. The stand rocks (tilting, rigid-body motion) **[paper, SS23 cites Samejima 2021
  on washers]**.

### 3.4 The strike: tip, shoulder, bell, bow, edge; mallets and brushes

Hertz contact time t_c = 2.87 (m²/(R E*² v))^{1/5}, for a sphere on a half-space; a thin shell is
softer, so real contacts are longer. Tip radius 5 mm, effective mass at the tip 12 g (a pivoting
stick of 40–50 g) **[calc; masses memory]**:

| Tip | v = 0.3 m/s | 1 m/s | 3 m/s | 8 m/s |
|---|---|---|---|---|
| Hickory (E ≈ 12 GPa along the grain) | 0.17 ms | 0.13 ms | 0.11 ms | 0.09 ms |
| Nylon (E ≈ 2 GPa) | 0.33 ms | 0.26 ms | 0.21 ms | 0.17 ms |

- **The force spectrum is flat to about 1/(2t_c).** That is 3–5 kHz for hickory and 1.5–3 kHz for
  nylon; above it the force falls quickly.
  - So a nylon tip is *not* brighter in the force. It is perceived brighter because it does not bruise
    or dampen, and because of its click **[memory]**. The "wood sound" in the ping is the stick's own
    bending modes **[web, Paiste]**.
  - Free–free hickory stick, 40 cm × 13.6 mm, E 15 GPa, ρ 820 kg/m³: **324, 892, 1749 Hz [calc]**.
- **Simulations use longer contacts:** 1 ms raised cosines to mimic a strong stick hit; 2–4 ms for
  softer strikes and gongs **[paper, Bao19, DAFx15, BWWD23]**. A cymbal edge is locally compliant, and
  crashes are struck with the shoulder, so 0.5–2 ms is plausible for crashes (bells_blocks.md
  **[memory]**).
- **Bell (r ≈ 4 cm):**
  - A thick, stiff region. The strike stays nearly linear even at 90 N on a shaped and tapered model
    **[paper, Bao19]**, so the bell's tone is clear and pitched.
  - The bell excites the modes that move the bell: a sparser, higher set with less low (m,0)
    content.
- **Bow:** the ride area, struck with the tip. It excites a broad band of modes with moderate
  displacement: ping plus wash.
- **Edge:** struck with the shoulder for a crash. The thin edge moves most (2–4 mm at 90–150 N on the
  tapered model) and the cascade starts at once **[paper, Bao19]**.
- **Mallets (yarn, felt):** t_c ≈ 2–5 ms. They roll off the high frequencies; the cymbal swell is
  built from the cascade (bells_blocks.md §4) **[memory]**.
- **Brush on a cymbal:**
  - A brush tap is many soft wire impacts. Little ping, a soft broadband "tss" whose wash builds with
    repetition.
  - A brush sweep on the ride is friction noise into the plate (§5.6).
  - Brush ride playing is quiet and is common on ballads **[memory]**.

### 3.5 The nonlinearity: regimes, cascade, glide

- **Three regimes, set by the displacement w against the thickness h** **[paper, DAFx15]**:
  - **w ≪ h: linear.**
  - **w ≈ h: weakly nonlinear.** The frequency depends on the amplitude (heard as a pitch bend), and
    modes exchange energy at internal resonances. For a spherical shell these are 1:1:2, from the
    quadratic terms **[paper, SS23 cites Thomas, Touzé & Chaigne 2005]**.
  - **w > h: the nonlinearity dominates.** Energy cascades from large to small wavelengths: wave
    turbulence, heard as a crash or shimmer.
- **Numbers from simulations:**
  - **100 N, 1 ms on a 21×18 cm, 1 mm steel plate:** weakly nonlinear (pitch bends).
  - **300 N, 0.8 ms:** w reaches 2h, the modes blur into a broadband "turbulent" spectrum, then become
    separate again as they decay **[paper, DAFx15]**.
  - **Tapered 0.4 m disc, 90 N at the edge:** the maximum displacement is 2 mm for a uniform 1 mm
    plate, 3 mm with h_e = 0.7 mm, 4 mm with h_e = 0.4 mm. With h_e = 0.4 mm, **frequencies up to
    15 kHz appear within the first milliseconds** **[paper, Bao19]**.
  - **The characteristic frequency f_c** (the energy-weighted mean of the velocity spectrum) for a
    150 N strike doubles from h_e 1 → 0.7 mm, then multiplies by 2.3 from 0.7 → 0.4 mm **[paper,
    Bao19]**.
- **How fast the cascade runs:**
  - **Gongs:** up to ~0.5 s to reach the highest frequencies, the audible "blow-up" after the strike
    **[paper, Bao19]**.
  - **A large plate struck for 4 ms:** "a slow migration of energy to the high frequency range over
    the first several hundred milliseconds" **[paper, BWWD23]**.
  - **A recorded 16" Sabian HHX Evolution crash:** new high frequencies emerge 100–300 ms after the
    hit **[paper, SA19]**.
  - **Crash level and spectrum (Rossing & Shepherd, via bells_blocks.md):** peak level 0.15–0.2 s
    after the strike; energy reaches 3–5 kHz within ~100 ms.
- **Shape matters as much as taper** **[paper, Bao19]**:
  - **Dome alone** (uniform 1 mm, H_c 3.4 cm): adds quadratic nonlinearity and brightens the
    700–800 Hz region, but it is **too stiff**: linear even at 150 N, no crash.
  - **Taper alone:** too compliant, with excessive pitch glide ("more resemble that of a membrane"),
    and the bell cannot ring cleanly.
  - **Both** (H_c 3.4 cm, h_e 0.4 mm): a clean bell at the centre, a rapid broadband build-up at the
    edge, little glide.
  - **Half the dome height** (1.7 cm) builds energy faster than 3.4 cm: the best compromise between
    the quadratic coupling and the stiffening.
- **Pitch glide:** downward after a hard hit, as the amplitude decays (the backbone curve). It is
  modest on real cymbals and large on thin flat plates **[paper, Bao19]**. Bilbao's gong simulations:
  the glides are *not* uniform across partials, so a simple tension-modulation (Berger) model that
  glides all modes alike is wrong for plates **[paper, BWWD23]**. The project's `Def::glide` is that
  uniform kind: fine for a membrane, not for a cymbal.
- **Theory:** weak-turbulence spectra for plates (Düring, Josserand, Rica 2006). Experiments and
  energy-conserving simulations of the transition to turbulence and of the spectrum's dynamics
  (Touzé, Bilbao, Cadot 2012; Ducceschi, Cadot, Touzé, Bilbao 2014) **[memory]**. Useful only as the
  shape of the cascade: a broad spectrum whose upper end moves upward over time, then retreats as the
  energy falls.

### 3.6 The jazz ride: ping against wash

What the model has to reproduce (mechanism **[memory]** unless marked):
- **Ping:** the first ~30–80 ms after a tip stroke.
  - The contact click (the stick's bending modes, the 0.1 ms contact: broadband to ~5 kHz).
  - The coherent onset of the modes near the strike point. The modal sum is spatially focused at
    first, and the strike-point weighting φ_k(x_s) is coherent.
  - Heard as a short, bright, slightly pitched "tick" in the 2–6 kHz range.
- **Wash:** the incoherent sum of hundreds of modes ringing with long T60s.
  - The strike-point coherence is lost within tens of milliseconds, because modes 10–15 Hz apart
    dephase in ~70–100 ms **[calc: 1/Δf]**.
  - It **accumulates over strokes.** At spang-a-lang tempo (quarter = 120–300 bpm: 4–10 strokes/s),
    each stroke adds energy to modes that live 2–10 s, so the wash level settles where the input power
    equals the decay. A linear model gives this for free, *if the ride is one object across all ride
    keys*.
- **Keeping the ping clear:**
  - (a) A heavier or more domed ride stays in the linear regime at jazz dynamics, so the stroke's
    energy does not smear into the cascade.
  - (b) The tip's short contact gives a strong high-frequency onset over the wash.
  - (c) The wash's spectrum is darker than the ping: high modes die faster (§3.3), so between strokes
    the 3–6 kHz band falls while the 300 Hz–2 kHz band holds. The ping lands in a gap the damping
    opened.
  - (d) Drummers play the bow with the tip, softly, and accent on the bell.
  - A "dry" ride has more damping in the mid-highs (hammering, small bell); a "wet" one has less.
- **What to measure on recordings (§9):**
  - The ping/wash ratio: band energy in the 0–60 ms window after an onset, against the level just
    before it.
  - The wash's band levels at steady state, and its T60s once the drummer stops.
- **Sizzle rivets** add a sustained rattle (§3.8).

### 3.7 Crash, splash, china

- **Thin crash (16–18", h_e 0.5–0.7 mm):** an edge strike with the shoulder drives it into turbulence at
  moderate force. The broadband build-up is "immediate" (no audible delay) on very thin edges
  **[paper, Bao19]**. Peak level after 0.15–0.2 s; brightest band 3–8 kHz for an 18" (bells_blocks.md).
- **Splash (8–12", h_e ~0.4–0.6 mm):** small and very thin. Fast, broadband, short (T60 ≈ 1–2 s)
  **[memory]**.
- **China (16–20"):** an upturned flange at the edge and a cylindrical bell; often unlathed, thin.
  - The flange is a stiff curved ring that couples strongly to the bow. Its bending gives strong
    quadratic terms, so the cascade starts early and the sound is "trashy": noise-like, little pitch,
    fast decay **[memory]**.
  - For a model it is the crash code with a stronger quadratic term (a larger effective curvature) and
    more damping above 5 kHz.
- **Choke:** a hand grabbing the edge damps all modes in ~50–100 ms **[memory]**. With MIDI: poly
  aftertouch on the cymbal key, or a short note-off when the user enables that.

### 3.8 Rivets and sizzle (a rattle)

- **What it is.** A sizzle ride has 6–8 loose rivets (copper, ~0.2–0.5 g each) in holes near the edge
  **[memory]**. Each rivet sits under gravity. It lifts off when the plate under it accelerates
  downward faster than g, then lands again: a series of tiny impacts.
- **Threshold [calc]:** a displacement amplitude of g/(2πf)²: 6.2 µm at 200 Hz, 1.0 µm at 500 Hz,
  0.25 µm at 1 kHz, 0.03 µm at 3 kHz.
  - A cymbal ringing at any audible level exceeds this in its kHz modes. So **the rivets sizzle
    after even soft strokes, and keep sizzling until late in the decay.**
  - Then they **stop rather abruptly** once the acceleration at the rivet falls below g: the
    characteristic sizzle cut-off.
- **The impacts are re-injected into the cymbal.** A rivet hitting the plate is a broadband impulse
  at the hole. That gives the dense, bright, steady sizzle and the rivets' longer apparent sustain.
- **Model (cheap, in the engine's terms):**
  - Each rivet is a point mass on a one-sided Hertz spring at its hole, plus gravity. The plate's
    height there comes from Σ_k φ_k(x_r) q_k; the contact force goes back into the modes at the same
    point.
  - This is exactly the `ModalSet` contact patch, with a moving obstacle (the rivet mass) instead of a
    fixed one. Cost: 2N MACs per rivet per sample (N ≈ 1000 → 8 rivets ≈ 16 k MAC/sample ≈ 0.8 GMAC/s)
    **[calc]**.
  - Cheaper: the rivets see only the modes below ~6 kHz.
- **Other rattles:** the same model serves a "sizzler" chain on the ride (a beaded chain on the
  cymbal: many masses), the tambourine's jingles (pairs of discs), and loosely the half-open hi-hat
  (§4).

### 3.9 Published cymbal and plate models, and what each costs

| Model | Method | Size | Cost reported | Real-time? |
|---|---|---|---|---|
| Ducceschi & Touzé (DAFx 2015; JSV 2015) | Modal von Kármán: N_Φ transverse modes, N_Ψ in-plane modes, coupling tensors computed offline, Störmer–Verlet | N_Φ = 100 (to 9 kHz), N_Ψ = 50; rectangular 21×18 cm × 1 mm plate | MATLAB, i7 2.4 GHz: **8× real time** for strikes, at k = k̂/2 (twice the linear stability rate) | No |
| Ducceschi (SMAC 2013): modal against finite differences on a gong plate | Both methods compared | modal Nw = 150, NF = 484; FD ~10 000 points | modal: "few hundred modes"; Γ tensor 0.5 h to compute; ~10 min per s at 20 kHz. FD at 100 kHz: ~2 h per s | No |
| Bao (Nguyen) & Touzé (JASA 2019) | Rayleigh–Ritz modal, variable thickness and shape | **N_U = 800 to 18.9 kHz, N_Ψ = 60 non-vanishing** | f_s = 100 kHz (stability needs > 59 kHz) | No (offline) |
| Bilbao, Webb, Wang, Ducceschi, "Real-time gong synthesis" (DAFx 2023) | FD von Kármán plate, quadratised energy (IEQ/SAV-like), explicit, custom biharmonic solver, AVX | steel, 0.5 mm, 0.03–0.06 m² (grids 13×27 to 25×31) | **0.15–0.99 s CPU per s of sound** at 44.1 kHz, one thread; the solver takes ~75 % | **Yes, for small plates** |
| Diaz & Sandler (DAFx 2025), JAX | Modal vK on a GPU | 10–100 modes | ~2× faster than real time at 100 modes, 44.1 kHz (RTX 3090); C++/Eigen and MATLAB slower at 100 modes | Barely |
| Skare & Abel (DAFx 2019) | Linear modal filterbank on a GPU (phasor filters), modes fitted to recordings; velocity layers; ad-hoc "coupling" that moves energy into high-velocity-only modes above an energy threshold | **> 2000 modes per cymbal**; 10 instruments × 2000 | A CPU core ran ~3000 phasor filters (2013 CPU) | Yes (GPU) |
| Bilbao (IEEE TASLP 2010) | FD shallow spherical shell (cymbal-like) | — | offline | No |
| Sekiguchi & Samejima (2023), hi-hat | FD of two spherical shells + rim contact | 14" bronze, R_s 0.8 m, **f_s = 480 kHz** | — | No |
| Samejima (2021) | FD cymbal including the washer (stand) and stick or mallet dynamics | — | — | No |
| Ducceschi, Hamilton, Russo (FA 2023) | Modal SAV: explicit, energy-stable; collision of a wire and a membrane | — | "compute times on par with Störmer–Verlet"; SAV enabled real-time vK plates, nonlinear strings, networks | Yes (as a method) |

Sources: [paper] for all except Bilbao 2010 and Samejima 2021, which are **[web: cited in SS23]**, and
Skare & Abel, **[paper]** read for the numbers.

**Cost structure of the modal von Kármán term [paper, DAFx15; calc].**
- **The equations:** q̈_s + 2ξ_s ω_s q̇_s + ω_s² q_s = −Σ_{p,q,r} Γ^s_{pqr} q_p q_q q_r + forcing,
  with Γ = Σ_k H^k E^k summed over the in-plane modes.
- **Computed in factored form:**
  1. v_k = H^k q, one matrix–vector product per in-plane mode: N_Φ² each.
  2. η_k = q · v_k.
  3. The force is nl = Σ_k η_k E^k q, the same cost again, or about N_Ψ N_Φ² when H and E are
     related.
- **Per step: ≈ N_Ψ N_Φ² MACs.** The tensor is N_Ψ N_Φ² floats.
- **For the free circular plate, an in-plane mode with n nodal diameters couples only pairs whose
  angular orders satisfy k_p ± k_q = ±n**, and coefficients that vanish are known analytically
  **[paper, DAFx15; Bao19]**. That makes the tensor sparse: ~5–15 % non-zero **[calc, rough]**.

| N_Φ | N_Ψ | f_s | Density | MAC/step | GMAC/s | Tensor |
|---|---|---|---|---|---|---|
| 100 | 50 | 96 kHz | dense | 5.0·10⁵ | 48 | 2 MB |
| 128 | 40 | 24 kHz | dense | 6.6·10⁵ | 16 | 2.6 MB |
| 128 | 40 | 24 kHz | 10 % | 6.6·10⁴ | 1.6 | 0.3 MB |
| 256 | 60 | 24 kHz | 10 % | 3.9·10⁵ | 9.4 | 1.6 MB |
| 800 | 60 | 100 kHz | dense | 3.8·10⁷ | 3800 | 150 MB |

All values **[calc]**.

- **Practical throughput.** One AVX2 core reaches ~20–30 GMAC/s on dense L2-resident data, and much
  less on gathers **[memory]**.
- **What is real time:**
  - A full-band modal von Kármán cymbal (the last row) is ~100 cores: out.
  - **A 128-mode nonlinear core (to ~2–3 kHz on a ride) at 24 kHz is 0.1–0.6 core**, depending on the
    sparsity achieved.
- **Why 24 kHz is enough.** Störmer–Verlet is stable for f_s > π f_max: 9.4 kHz for a 3 kHz top
  mode **[calc]**. In the modal form the nonlinear force only reaches the retained modes, so the
  products above the core's band cannot alias; they are simply not represented. That is why the
  cascade has to be continued separately (§8.2).

### 3.10 The reverse cymbal now, and what it shows

- **How it is built.** `ModalModel::kRevCymbal` (src/modalinst.cc) renders a 16" crash *once* at
  program build. It uses 3 vibrators × 64 modes = **192 linear modes**, log-spread 81 Hz–14 kHz,
  T60 = 4.5 s · (1 kHz/f)^0.6, modal mass 0.05 kg, struck by a 50 g, 4 m/s Hertz mallet
  (K = 2·10⁹). It renders 3 s into a buffer and plays it backwards.
- **There is no cascade in it,** although the comment in `modalinst.h` says "with its cascade". The
  "energy moves up" is faked by a radiation weight rad ∝ (f/1 kHz)^0.8.
- **What it shows.** The engine handles a cymbal's linear tail with a few hundred modes, at modest
  cost. It also hits two limits:
  - The 64-mode vibrator cap: a ride needs 1000–2000 modes.
  - There is no plate-type coupling. `Def::quad` has 6 terms; `Def::Lin` has 2.
- A real cymbal needs its own mode container (§8.1).

## 4. The hi-hat

### 4.1 The mechanism

- **Construction.** Two cymbals face each other: typically 13–15" (14" is the norm), the bottom
  heavier (§3.1).
  - The bottom one rests on a felt on the stand's cup, often tilted slightly by a tilter screw, so
    the rims meet first at one side. That makes the closed sound crisper and stops the air cushion
    from sealing **[memory]**.
  - The top one is held in a clutch (between felts) on a rod pulled down by the pedal against a
    spring.
- **The pedal** **[memory]**:
  - **Travel:** the open gap is commonly 10–30 mm, set by the clutch height. Half-open is ~1–5 mm.
  - **Closed force:** the spring plus the player's foot. Of order 20–100 N at the rims, against the
    top cymbal's own weight of ~10 N.
- **Sound states:**
  1. **Closed + stick** (42): a short "tick" with a damped, mostly high-frequency body.
  2. **Open + stick** (46): close to a single thin crash, with the bottom cymbal ringing by air
     coupling. Long.
  3. **Half-open + stick** (no GM key): the rims chatter. Sizzling, sustained while the gap stays
     small.
  4. **Foot chick** (44): the top closes onto the bottom. Air, a clap of the rims, a short ring.
  5. **Foot splash:** close and immediately reopen, so the two plates ring after the clap.
  6. **Open → closed while ringing:** the choke.
- **What the physics is** **[memory]**:
  - **Closed:** the rims are pressed together with a static pre-load. The pair moves as one stiffened
    structure with a friction-and-contact interface along the rim. Energy is lost at that interface:
    micro-slip friction and impact losses. That is the strong damping, especially of the low modes,
    whose rim motion is large.
  - **Half-open:** the rims touch intermittently. Every touch is a broadband impulse to both plates
    (like the rivets, but distributed along the rim) and an energy loss.

### 4.2 The chick and the air

- **Air pumping [calc].** Two discs of radius R closing at speed u across a gap d push air out at the
  rim at u_r ≈ R u / (2d), from mass conservation:

| u (closing speed) | d (gap) | u_r at the rim |
|---|---|---|
| 0.5 m/s | 5 mm | 9 m/s |
| 1 m/s | 2 mm | 45 m/s |
| 1 m/s | 1 mm | 90 m/s |

- So in the last millimetres before contact a fast annular jet forms. Its noise ("chhh") precedes and
  overlaps the rims' clap.
  - The squeezed air film also cushions the impact (squeeze-film damping), which softens the clap.
  - The tilter (rims meet at one side first) lets the air escape and makes the closure a fast rolling
    contact around the rim, not a single slap **[memory]**.
- **Duration.** A closing foot moves the top cymbal 10–20 mm in ~15–40 ms **[memory]**. The air noise
  lasts the last few ms. The clap's ring is short because the plates are then pressed together.
- **Spectrum:** not found in the literature. Measure it: centroid, the duration of the burst, and the
  share of noise against modal peaks (§9).

### 4.3 The published model (Sekiguchi & Samejima 2023) [paper]

- **Plates:** two thin spherical shells (14", R = 0.1778 m, H = 1 mm, R_s = 0.8 m, bronze 8860 kg/m³,
  110 GPa, ν 0.33). Clamped at an inner circle (no washer dynamics) with free rims. Finite differences
  in polar coordinates at **f_s = 480 kHz**.
  - Losses: σ₀ = 1.34 kg/(m²s), σ₁ = 9.6·10⁻⁵ kg/s.
- **Stick:** a 10 g point mass at 10 m/s on a Hertz-like spring (K_st = 3·10⁹, exponent 3).
- **Contact between the rims (model 1):** nonlinear springs at the rim nodes, K = 10⁹, exponent 3.
  - Aurally it had **too little rattle**, and the highs above 16 kHz decayed faster than in a real
    hi-hat.
- **Contact between the rims (model 2):** at contact, exchange the two velocities (an elastic
  collision of equal masses) with a pseudo-loss p_s = 0.05.
  - **Closer to the measured closed hi-hat:** many noise-like components between the modal lines,
    as in the recording.
- **Gap:**
  - Smaller gaps decay faster overall, and the extra attenuation is **larger at low frequencies**.
  - With d = 5 µm the model rang on late, unlike a real open-ish hat, because once the motion falls
    below d the contact stops.
  - A real closed hat has a **negative gap** (static pre-load and deformation). The model could not
    represent that; it diverged.
- **Limits they name:** no tilting of the top cymbal, no washers or felts, no true Hertz contact
  between the shells.

### 4.4 Lessons for this project

- **Use a pre-loaded contact.**
  - When closed, each rim patch carries a static force: the pedal force divided among the patches.
    The contact springs are always engaged, and the pair's linearised modes are stiffened and coupled.
  - Losses at the interface go in as a contact damping (Hunt–Crossley, damping ∝ penetration^α ·
    velocity) and a friction term, which is what model 2's pseudo-loss imitates.
- **Half-open is where the contact model matters most.** The stick moves the top rim by more than the
  gap, so the rims chatter, sustaining a rattle while the plates ring.
- **Model the tilt:** the bottom cymbal's contact height varies around the rim, e.g. (1 − cos θ)
  times a tilt of ~1 mm. The rims then meet at one side first, which avoids an unphysical perfect
  seal.
- **Air:**
  - Model the chick's air jet as noise filtered at the rim jet speed's characteristic frequency, with
    amplitude ∝ u_r³ (jet noise power ~ u⁸ is too steep at these sizes; to be fitted). The intent is
    the noise *gated by the closing*, not a free noise burst.
  - Optionally, couple the two plates' low modes through the trapped air (a linear coupling that
    grows as 1/d).

### 4.5 The pedal state machine (keys 42, 44, 46 and CC4)

- **Pedal position p:** 0 = closed (pre-load F_c), 1 = fully open (gap d_open ≈ 15 mm).
  - CC4 sets p continuously when present; otherwise the keys set it.
  - The rod moves the top cymbal with first-order lag dynamics (the foot): τ ≈ 10–20 ms closing,
    30–60 ms opening **[memory, to fit]**.
- **42 (closed + stick):** if p > 0, close first. That produces a soft chick about 20 ms before (for
  files, start the closing at the note time and strike at once; the overlap is natural). Then strike
  the top at the edge or bow with the tip.
- **46 (open + stick):** if closed, lift the pedal (no sound but the release). Strike.
- **44 (pedal):** if open, close at velocity → foot speed: the chick. If already closed, do a quick
  lift of ~5–10 mm and close, the 2-and-4 foot motion of a drummer whose foot rests on the pedal.
- **Choke.** A 42 or 44 after a 46 closes the hat on the ringing plates. The contact damps them.
  That is the GM exclusive class done physically.
- **Half-open (no GM key):** CC4 between ~0.05 and 0.3, or a velocity zone of 46 (e.g. a soft 46 =
  half-open), as an option.

## 5. The snare drum

### 5.1 Geometry, heads, tension

- A 14" × 5–6.5" snare; for jazz, often a wooden shell with a coated batter tuned high, and a thin
  snare-side head (~3 mil) **[memory]**.
- **Measured batter-head modes:** a 36 cm steel snare drum with a Remo coated Powerstroke 3, from
  352-point modal analysis **[paper, Skrodzka06]**. Rossing et al.'s values for another drum are given
  for comparison:

| Mode | Skrodzka f (Hz) | f/f₁ | Damping (% critical) | Rossing et al. f (Hz) | f/f₁ |
|---|---|---|---|---|---|
| (0,1) | 224 | 1 | 5.07 | 224 | 1 |
| (1,1) | 322 | 1.44 | 1.10 | 280 | 1.25 |
| (2,1) | 457 | 2.24 | 1.07 | 403 | 1.80 |
| (0,2) | 566 | 2.32 | 1.16 | 445 | 1.99 |
| (3,2) | 616 | 2.75 | 1.24 | 801 | 3.58 |
| (3,1) | 734 | 3.28 | 0.77 | 512 | 2.29 |

- The (0,1) mode is strongly damped: 5 %, i.e. T60 ≈ 0.03 s **[calc: T60 = 2.2/(ξ f)]**. It radiates
  and pumps the air in the cavity. The others are at ~1 %, T60 ≈ 0.07–0.1 s at 300–700 Hz.
- The **(0,1) mode is the strongest radiator only for a central strike**. Off-centre, the spectral
  peaks differ a lot from the modal frequencies, and the shell and snares contribute as much as the
  head **[paper, Skrodzka06]**.
- **Coupled pairs.** Rossing's six lowest resonances include **two pairs based on (0,1) and (1,1)**
  (batter and snare head coupled through the air and the shell). A simple two-mass model describes
  each pair well **[web, Rossing in Physics Today]**.
- **Tension from the (0,1) mode [calc]:** f₀₁ = 2.405/(2πR) · √(T/ρ_s). With R = 0.178 m and
  ρ_s ≈ 0.26 kg/m² (a coated 10 mil Mylar head), 224–231 Hz gives **T ≈ 3000 N/m**.
  - That matches the batter tension used in a snare/collision study, 3000 N/m **[paper, TN]**.
  - Another simulation uses R = 0.15 m, ρ = 0.2 kg/m², T = 2000 N/m **[paper, FA23]**.
  - The snare-side head is usually tuned higher than the batter **[memory]**.

### 5.2 Snare wires

- **What they are** **[memory]**: a set of 16–42 strands, typically 20, of coiled steel wire
  (~0.6–1 mm coil). They are stretched across the bottom head and held by the strainer at both ends,
  with a small tension (the strainer's knob). The coil makes each strand a soft spring with internal
  rattling.
- **Model parameters in the literature:**
  - **Torin & Newton:** test snares were a piano string and a guitar D string, radius 0.18–0.29 mm,
    tension 0.2–2 N, E = 2·10¹¹ **[paper, TN]**.
    - A tighter wire bounces about **twice as often** as a slack one.
    - The bounce amplitudes were ~1 mm (D string) and 2–3 mm (wound string). The wire alternates
      between bouncing on the head and moving with it.
  - **Ducceschi, Hamilton, Russo:** wire L = 0.2 m, ϱ = 1.6·10⁻³ kg/m, T = 30 N, sitting 1 mm above
    the membrane. Collision B = 10¹⁰, α = 1.1, at f_s = 5 × 44.1 kHz **[paper, FA23]**.
    - The wire's first mode is then 342 Hz **[calc]**.
- **How the rattle sounds.** It begins with the bottom head's arrival, delayed by the cavity air. "The
  movement is at first coherent; it is rapidly randomized through multiple collisions" **[paper,
  BTC14]**.
  - The rattle needs the head's motion to exceed the wires' rest gap or pre-load. A soft stroke gives
    a short, thin buzz; a hard one a long, loud rattle.
  - **The wires are excited by anything that moves the bottom head:** a kick drum, a tom, a bass note.
    That is the sympathetic buzz of a real kit **[memory]**.
- **Cost in the FD models.** "When a realistic number of snares is included … the dominant part of
  the code … is the solution of the nonlinear system [the collisions], and not … the 3D field"
  (Newton–Raphson on a dense system) **[paper, THB14]**.
  - The modal SAV approach makes it explicit and cheap **[paper, FA23]**.
  - In this project, the existing contact solver (penalty springs, two Gauss–Seidel passes) is the
    same idea in a cheaper form (§8.4).

### 5.3 Air and shell

- **The cavity** couples the batter and snare heads, mainly in their axisymmetric low modes. It
  delays the bottom head's response by the air's transit; the snapshots show this **[paper, BTC14]**.
- **In modal terms:** linear couplings between the two heads' (0,1), (1,1) and (0,2) modes, plus the
  cavity's own first modes. A 14" × 5.5" cylinder: axial (0,0,1) at c/(2·0.14) ≈ 1225 Hz, the first
  radial–azimuthal (1,0,0) at 1.84·c/(2πR) ≈ 565 Hz **[calc]**.
- **The shell** (wood or metal) has its own ring modes: a steel shell rings, wood is damped. Shell
  modes matter for rim shots and cross-sticks **[memory]**.
- **Radiation:** the batter's (0,1) mode dominates for centre strikes. The two heads radiate in
  antiphase for some coupled modes **[memory]**.

### 5.4 The stick on the head

- **The contact** is longer than on a cymbal: the membrane is compliant. A 28 g mallet at 5 m/s was
  used in the FD snare **[paper, THB14]**.
  - A hickory tip on a coated head at 1–5 m/s: ~0.5–1.5 ms **[memory, to fit]**.
- **Strike position:** centre for a fat sound; off-centre for more ring and overtones (the
  higher-angular-order modes are excited).
- **Velocity:**
  - **Ghost notes** in jazz comping: 0.2–0.5 m/s.
  - **Accents:** 3–6 m/s.
  - **Rimshots:** up to ~8 m/s **[memory]**.
- **Tension modulation:** a hard hit raises the membrane's tension for a few ms, so its pitch drops
  as the stroke decays. Avanzini & Marogna model it efficiently with an energy estimate, and
  `ModalSet` has it (`glide`) **[memory; web, DAFx25 ref list]**.

### 5.5 Rim shot and cross-stick (key 37)

- **Rim shot:** the stick hits the head and the rim at once. The rim (a steel or die-cast hoop) and
  shell are struck with a large force, and the head gets a hard, off-centre hit. A loud "crack" with
  hoop ring (~1–3 kHz) **[memory]**. Model: two contacts in the same stroke, head and hoop; the hoop is
  a ring with modes.
- **Cross-stick / side stick:** the stick lies across the head. The butt end strikes the rim while the
  palm holds the tip end on the head.
  - The sound is the stick's own bending modes (free–free hickory 40 cm × 13.6 mm: **324, 892,
    1749 Hz [calc]**; the hand and the head damp it) plus the hoop and shell knock. The head is
    excited through the stick's contact.
  - Model: a "woodblock-like" knock, but the modes come from the stick and the rim. Velocity barely
    changes the colour.
  - The existing woodblock mapping is a fair placeholder.

### 5.6 Brushes: tap, slap, sweep

- **The tool** **[memory]**: a fan of steel wires, about 100–200 of them, ~0.3 mm in diameter. The
  exposed length is ~10–12 cm, set by retracting them into the handle, and the fan spreads ~8–12 cm.
  The wire tips are what touch the head; the fan is springy.
- **What is heard** **[web, Digital Drummer]**: "a combination of multiple simultaneous micro-hits by
  the individual strands … and the sweeping sounds generated by the friction of the strands across the
  head". Electronic kits can detect a sweep's start but not its duration.
- **An analog patent** (US 4,181,059, a circuit imitating a brush rotated on a snare head)
  **[web, patent]**: the sound is basically unpitched, but **the changing angle between the brush and
  the head gives cyclic variations in timbre, one per revolution and one per half revolution**.
- **No published physical model or measurement of brush sweeps was found** (two searches, including
  an extended one).
- **A model from the physics (proposal) [calc/memory]:**
  - **Tap.**
    - N_b bristle groups (8–16 virtual groups stand for ~100 wires) land with arrival times spread
      over 2–5 ms (the fan's tilt × the hand's speed). Their positions are scattered within the fan's
      footprint (~4×8 cm).
    - Each landing is a soft Hertz impact. The wire tip's effective mass is ~0.05–0.2 g; its
      stiffness is set by the wire's bending.
    - **Slap (39):** the fan lands flat. More groups, a 1–2 ms spread, a larger footprint. Averaging
      over the area suppresses the high modes (the footprint filter), so it sounds darker and louder.
    - Taps also excite the snare wires through the head, as sticks do.
  - **Sweep / swirl.**
    - **The bristles drag over the coated head.** The coating is a rough sprayed layer. Each tip
      rides the texture, so its height follows a profile s(x) along its path. Through the tip's
      contact stiffness, that is a force f = k_c (s(x(t)) − z_head(t)) under a normal pre-load.
    - **The texture rate per tip is v/λ:** speed over grain spacing. With v = 0.3–1.5 m/s and
      λ ~ 50–200 µm, that is ~2–30 kHz per tip, so broadband.
    - **Circle speeds [calc]:** radius 10 cm, one turn per beat at 120 bpm: 1.26 m/s; one turn per two
      beats at 60 bpm: 0.31 m/s.
    - **Many tips sum incoherently**, so the force is Gaussian-like noise:
      - **Level:** ∝ normal force × a power of speed.
      - **Spectrum:** set by the texture and the tip's contact stiffness (a low-pass at k_c/m_tip).
    - **The force enters the head's modes at the moving contact point.** Each mode is weighted by
      its shape there: J_m(k r) cos(m θ) for a membrane. As the hand circles, the weights change.
      That gives the colour's motion with position, the 1/rev variation.
    - **The fan's orientation relative to the motion** (dragging the tips trailing, or pushing them
      leading) changes the friction's "bite". It flips twice per revolution, which is likely the
      patent's 2/rev **[memory: interpretation]**.
    - Friction also damps the modes slightly where the brush rests: a moving damper, cf. drums.md: "a
      tap where the stirring brush lies is damped by it a little".
  - **Precedents for the exciter:** scraping and rolling contact models that read a surface profile
    at the contact speed into a modal resonator (van den Doel, Kry & Pai, "FoleyAutomatic", SIGGRAPH
    2001; Rath & Rocchesso, Sounding Objects; Avanzini, Serafin & Rocchesso on friction) **[memory]**.
- **Brushes on the ride and hi-hat:** the same tap and sweep exciters on the plate's modes.

## 6. The rest of the kit

| Drum | Jazz voicing | Physics / numbers | Model | Status |
|---|---|---|---|---|
| Kick | **18" bebop** (or 20"), both heads, **no port**, tuned high (open, tonal, "boom" with pitch); felt beater; **feathered**: soft quarters, felt more than heard | Ideal (0,1) for 18", ρ_s 0.35 kg/m², T 600 N/m: **69 Hz**; 20": 62 Hz **[calc]**. Air loading lowers large heads' low modes by tens of % **[memory]**. Two heads coupled by the air (as taiko, steelpan_taiko.md) | Two modal membranes + air coupling (the snare's code, without wires); beater: Hunt–Crossley felt, ~3–6 ms contact **[memory]**; the beater may rest on the head ("buried") or bounce off | taiko placeholder |
| Toms | 12" rack, 14" floor, tuned high and open | 12", T 1500 N/m: **178 Hz**; 14", T 1200 N/m: **136 Hz [calc]** (ideal, both heads) | As the kick, smaller; snare buzz by sympathy | melodic-tom (a circuit lead) |
| Cowbell (56) | Latin jazz | Thin rectangular iron shell | The agogo code with a cowbell mode list (Rossing) | agogo |
| Tambourine (54) | rare | Jingles as pairs of discs rattling on pins | The rivet rattle model (§3.8) on a frame-drum membrane | — |
| Shakers (69, 70) | bossa, Latin | Many beads impacting a shell | Granular (PhISEM-like: stochastic collisions into a few resonances) **[memory]** | — |
| Triangle (80, 81) | | Bent steel bar, long T60; mute = hand damping | Tinkle-bell code, bar ratios | — |
| Guiro (73, 74) | | A scraper on ridges | The brush's scraping exciter with a periodic profile | — |
| Claves, blocks | | | woodblock | built |
| Hand clap (39, Standard and Jazz kits) | | | Several noisy bursts, 5–15 ms apart (the existing applause sfx is a source) | synth-drum |

## 7. Jazz drumming practice the model must support

- **Ride pattern ("spang-a-lang"):**
  - The tip on the bow, quarter notes with the swung "and" of 2 and 4. Accents on 2 and 4, or
    feathered evenly.
  - Swing ratio ~2:1 at medium tempo, approaching 1:1 at fast tempos **[memory]**.
  - The model's job: a clear ping per stroke, a continuous wash that the strokes maintain, and small
    timbre differences between strokes as the strike point wanders. Add ±5 mm per stroke of random
    strike position, and ±1 dB of velocity jitter for live play.
- **Hi-hat:**
  - The foot on **2 and 4** (44), a crisp chick.
  - Sometimes "hat splashes" on 2 and 4 in Latin and modern styles.
  - The stick on closed or half-open hats in bebop comping is rare in jazz; it is a rock and funk
    idiom.
- **Bass drum feathering:** quarter notes at very low level (ghost level). The beater may stay on the
  head between strokes, which damps it. Accents ("bombs") come from comping.
- **Snare comping:** ghost notes, accents, rimshots, cross-stick on ballads and bossa.
- **Brushes on ballads:**
  - The left hand circles, the right taps 2 and 4 or plays the ride rhythm on the snare. Very quiet.
  - The snare is often left on. The swirl must stay audible but soft at a whisper level.
- **Dynamics:**
  - Jazz dynamics sit low: a brush ballad is ~40–60 dB SPL at the player, a swinging ride ~70–90 dB
    **[memory]**.
  - **Velocity → speed (proposal):** v = v_min (v_max/v_min)^{vel/127} with **v_min = 0.2 m/s,
    v_max = 8 m/s** for sticks, 0.1–2 m/s for brushes. That spans 32 dB of linear-regime level
    (20 log₁₀ 40) **[calc]**.
  - The nonlinearity adds brightness at the top on its own: the crash blooms at high velocity only.
  - For brushes, velocity → hand speed and pressure.
- **Stick height and speed.** The stroke speed is the physical control. The model needs no separate
  "height". Contact time changes little with speed (∝ v^−1/5, §3.4), so **the brightness change
  with velocity comes from the cymbal's nonlinearity and the membrane's tension modulation, not from
  the stick**.
- **Choosing sticks or brushes:** by the kit program (§2), the user's decision. Within the Brush kit
  the cymbals, toms and hi-hat strokes default to brushes. A per-kit option keeps stick cymbals for
  compatibility with sample modules.

## 8. Model plan

### 8.1 Architecture

- **A new instrument `-inst jazz-kit`** (and `brush-kit` as the same instrument with the brush
  implement), registered for channel 10. It owns:
  - **Plates** (a new `Plate` class, `src/plate.h/.cc`), one per cymbal: ride, ride 2, crash 1 and 2,
    splash, china, and the hi-hat pair.
    - Up to 2048 modes, SoA arrays, the same exact two-by-two step matrices as `ModalSet` (`coefs`).
    - A nonlinear core (§8.2).
    - Contact points (rivets, rims).
    - A per-mode spatial shape table for strike positions (bell, bow, edge) and for the outputs.
  - **Heads** (snare batter and snare side, kick front and back, two toms): `ModalSet` vibrators. The
    64-mode cap is enough for a head below ~3 kHz. Linear couplings stand for the air, so `kLin`
    grows from 2 to ~8. Contact patches serve the snare wires; `kContacts` grows from 4 to ~32, or
    there is a dedicated wire set.
  - **The implements:** a stick (Hertz mass, tip or shoulder) and a brush (tap group and scraping
    exciter), a felt beater, the foot (the hi-hat pedal).
  - **A key router** (§2.5): key → object + implement + position + action.
- **Output:**
  - Each object radiates through its own modal radiation weights (the plates' (m,0) modes radiate
    poorly; the edge modes radiate well).
  - Two or more pickup weightings give "overheads" (stereo). The kit pans by object position (ride
    right, hi-hat left from the drummer's view, or the audience's view as an option).
  - The project's room/reverb follows as usual.

### 8.2 The cymbal: linear bank + nonlinear core + cascade sink

1. **Linear bank:**
   - **Modes:** N_lin modes from the modal-density table (§3.2), to 16–20 kHz. The frequencies are
     drawn from the free-plate (m, n) series, stretched by a dome factor, with random splitting of the
     pairs (hammering), and fitted to recordings where available.
   - **Damping:** a per-mode law (Bao's form, §3.3, fitted).
   - **Shapes at strike points:** from Bessel-type radial profiles × cos/sin(mθ). Those are known for
     the free plate; they can be refined later by a shell FEM offline (tools/).
   - **Cost:** ~1–2 ns per mode-sample scalar, or ~0.2–0.4 ns with AVX2 **[memory]**. 1500 modes at
     48 kHz ≈ 7·10⁷ mode-updates/s ≈ **2–5 % of a core [calc]**.
   - **Sleeping:** modes go to sleep below −100 dB; the engine already has `awake`/`quiet`.
2. **Nonlinear core:**
   - **Size:** the lowest N_Φ ≈ 96–192 modes (to ~2–3 kHz on a ride, ~4 kHz on a crash), stepped at
     24 kHz (a polyphase decimation/interpolation boundary to the 48 kHz bank).
   - **The cubic von Kármán term** comes from the circular-plate coupling tensors with the selection
     rules (sparse). The tensors are computed offline by a tool (`tools/vkplate.py`), from the
     analytic free-plate modes as in Ducceschi/Touzé and Bao, then scaled by thickness and stored in
     a data header.
   - **The quadratic term** of the dome (Bao: shape adds quadratic coupling that speeds the build-up
     and brightens 700–800 Hz) is a second sparse tensor G^s_{pq}.
   - **Time step:** Störmer–Verlet first. If high-amplitude instability shows, use the SAV form, which
     is explicit, energy-stable and about as cheap **[paper, FA23, BWWD23]**.
   - **The core is the same set of modes as the bottom of the linear bank.** They are not duplicated:
     the low modes simply get the nonlinear force added.
   - **Switching off.** When the core's energy falls below the weakly-nonlinear threshold (amplitude
     < ~0.1 h), the cubic term is negligible (∝ (w/h)²). Stop computing it: most of a ride's life,
     the core is off.
3. **Cascade sink (the closure that continues the cascade into the linear bank):**
   - **The sink:** the top band of core modes (the last ~10–20 %) gets an extra damping proportional
     to their own energy, an "eddy viscosity" in the spirit of turbulence closures.
   - **The injection:** the power this damping removes is injected as random forcing into the linear
     bank's modes above the core. It is distributed with a spectral weight ~f^−β (β fitted, cf. the
     weak-turbulence spectrum) and the modes' shapes at a few random points per block.
   - **What that gives:** energy is conserved across the boundary, the high band fills only when the
     core is nonlinear (the crash's bloom, delayed by the core's own dynamics), and the high modes'
     own damping then shapes the decay.
   - **This is a model choice, not a published method [calc/design].** Skare & Abel's coupling, which
     moves energy into high-velocity-only modes above an energy threshold, is the nearest published
     relative **[paper, SA19]**.
4. **Strikes:**
   - The contact is a Hertz mass on the plate: the shape vector at the strike point, the plate's
     compliance there, and the force solved per sample as in `ModalSet::strike`.
   - **Bell (53):** the strike point is at r ≈ 3–5 cm, with the stick's shoulder (larger R, longer
     contact).
   - **Bow (51):** r ≈ 0.6–0.75 R, with the tip.
   - **Edge/crash:** r ≈ 0.95 R, with the shoulder.
   - **The stick's own bending modes** (§3.4: 3 modes, damped by the hand, T60 ~20–50 ms) are excited
     by the reaction force: the "wood" in the ping.
5. **Rivets (59, a sizzle ride):** 6–8 contact masses near the edge (§3.8), seeing only the modes
   below ~6 kHz.

### 8.3 The hi-hat

- **Plates:** two `Plate`s (top ~700 modes, bottom ~550 to 20 kHz; §3.2), each with a small
  nonlinear core (64–96 modes).
- **Contact ring:** 12–16 patches at the rims.
  - Penalty force K (h_gap(θ) − (w_top − w_bot))^α with Hunt–Crossley damping, h_gap(θ) = d(p) +
    tilt (1 − cos θ). The pre-load is applied when d < 0 (closed).
  - Solved per sample with two Gauss–Seidel passes, as the engine does now. The compliance of each
    patch is precomputed from the modes.
  - **Cost:** 16 patches × 2 plates × ~400 modes (below 8 kHz) × 2 (read, apply) ≈ 26 k MAC/sample
    ≈ **1.2 GMAC/s ≈ 0.05–0.1 core [calc]**. It is only needed while the plates are near contact,
    which they always are when closed: the closed hat is cheap anyway, because it decays fast.
- **Air:** the chick's jet noise (§4.4) and an optional 1/d coupling of the low modes.
- **Pedal:** §4.5.

### 8.4 The snare

- **Heads:** two `ModalSet` vibrators (64 modes each, to ~2.5–3 kHz), ideal membrane ratios
  stretched to the measured table (§5.1).
  - The batter's (0,1) mode has 5 % damping; the others ~1 %; frequency-dependent above.
  - Tension glide is on the batter.
- **Air:** linear couplings of the axisymmetric and (1,1) modes of the two heads (needs `kLin` ≈ 6),
  fitted to Rossing's split pairs.
- **Wires:** 8–20 strands, each a short `one` (y-only) string vibrator with 8–12 modes.
  - Contact patches against the snare head: 3–4 per wire, so 32–64 contacts.
  - **Cost:** 64 contacts × (64 head modes + 10 wire modes) × 2 ≈ 9.5 k MAC/sample ≈ 0.5 GMAC/s
    **[calc]**.
  - The wires' high-frequency rattle is broadband impulses. Their high-frequency content above the
    head's 64 modes is not represented by the modes, so add each contact force's derivative,
    high-passed, straight to the output as the wires' own radiation. The wires radiate the snap
    directly.
- **Sympathetic buzz:** the kick and toms push the snare's bottom head through a small linear
  coupling (the room air). That is a few extra coupling terms.
- **Rim and hoop:** a ring vibrator, ~16 modes, for rimshots and cross-stick. The stick has 3 modes.
- **Brush exciter:** §5.6.
  - **Tap:** N_b small Hertz impacts.
  - **Sweep:** per bristle group, a texture table (white noise high-passed, 4096 entries, read at the
    speed), a normal force, and a moving point.
  - **The moving point:** the shape vector at the point is recomputed every 16 samples, the engine's
    control rate: 64 modes × N_b points. Bessel profiles are stored as a radial table (64 modes ×
    64 radii) × cos/sin(mθ).

### 8.5 Order of building

1. **The analysis tool first** (drums.md §5 item 2, `tools/drums.py`). Measure on the jazz stems (§9):
   ride band T60s and steady wash spectra; crash build-up; foot chick spectra; snare modes; brush
   spectra and modulation. The targets come before the models.
2. **`Plate`, linear:** a jazz ride (22" thin) with bow, bell and edge strikes, and rivets as an
   option. A/B against recordings. Wire 51/53/59 into `drumFor` through the kit router.
3. **The nonlinear core + cascade sink**, tested on a crash (49/57). Check the bloom delay and the
   f_c(t) curves against Bao's figures for the same plate parameters, using `tools/vkplate.py`
   offline as the reference.
4. **Hi-hat** (42/44/46, CC4): two plates, the contact ring, the pedal, air.
5. **Snare with sticks** (38/40/37): heads, air, wires, rim, cross-stick.
6. **Brushes** (Brush kit 38/39/40; brushes on ride and hats): tap groups, the scraping sweep, the
   swirl's note handling.
7. **Kick and toms** (35/36, 41–50): the snare's head code without wires.
8. **Splash, china, the rest** (§6), and the kit switch in `midifile.cc` (§2.5).
9. **Tests** in `tools/test.py`:
   - modes and decays against the stored fits;
   - energy conservation of the core (lossless test plate);
   - the contact's stability at high velocity;
   - NaN recovery;
   - real time with a busy pattern (ride + hat + snare + kick + a crash every bar).

### 8.6 CPU budget (48 kHz, one core, a busy jazz pattern) [calc]

| Part | Modes and contacts | Cost (share of one core) |
|---|---|---|
| Ride (1900 modes linear; core 128 at 24 kHz, active only on accents) | 1900 + core | 0.05 linear; +0.1–0.3 while the core runs |
| Ride 2 / crash 1 / crash 2 / splash / china, ringing (linear) | ~1300 each | 0.03–0.05 each while awake |
| A crash being struck (core active ~0.5–1 s) | core 128–192 | +0.2–0.5 for that time |
| Hi-hat (two plates + contact ring) | 1250 + 16 patches | 0.05–0.15 |
| Snare (two heads, wires, rim) | 128 + 160 + 64 contacts | 0.03–0.06 |
| Brush sweep exciter | 16 groups × 64 modes per 16 samples | < 0.01 |
| Kick + 2 toms | 6 heads × 64 | 0.02 |
| **Typical total** | | **~0.3–0.5 core; peaks ~1 core on simultaneous crash accents** |

- **Limiting the peaks:**
  - Cap the number of simultaneously active cores at 2; the oldest falls back to linear.
  - Lower the core's mode count when two are active.
- **Throughput figures are estimates.** The project's actual per-mode cost should be measured on the
  reverse cymbal (192 modes) before committing (one `-bench` run).
- **More cores:** the machine has 32 hardware threads. The kit's objects are independent within a
  block except for the snare–kick coupling, so a per-object thread split is possible if one core is not
  enough. That is not needed in the plan above.

### 8.7 Validation

- **Recordings** (~/Music/wavefield; drum stems separated with Demucs, drums.md §7, kept outside the
  repo):
  - **Brushes:** Love Ballads (*Some Cats Know* 1:01–1:10 brush strokes; *Laura*; *Here's to Life*;
    *I Thought About You*; *A Lovely Way to Spend an Evening*), Ella Fitzgerald *Body and Soul*.
  - **Ride and hats:** Cyrus Chestnut *My Favorite Things*, Jamie Cullum (*London Skies*, *I'd Probably
    Do It Again*), Till Brönner (three tracks), Gerry Mulligan *Georgia*, Louis Armstrong (two),
    *Autumn Leaves*, *Fly Me to the Moon*.
  - Whether each track has ride, brushes or both is to be checked by listening. Only measurements leave
    the scratchpad.
- **Measures:**
  1. **Decay per band** (1/3-octave, 100 Hz–16 kHz): Schroeder integration of isolated hits (endings,
     breaks). Target: within 10–20 % of the measured T60 per band.
  2. **Spectral centroid against time** (Bao's f_c) for crashes: the rise time to the peak, the peak
     value, and the decay. Also for ride strokes, which should show a small rise or none.
  3. **Crash build-up:** the 3–8 kHz band level's time of maximum, 0.1–0.3 s.
  4. **Ride ping/wash:** the onset energy ratio (0–60 ms against the pre-onset level) per band. The
     steady wash spectrum at a given tempo and level.
  5. **Hi-hat chick:** duration (−20 dB), centroid, the noise share (spectral flatness) in 2–12 kHz,
     the open hat's T60 per band, and the choke time.
  6. **Snare:** the head's mode frequencies (ESPRIT on hits), wire rattle duration against hit level,
     and the brightness against level.
  7. **Brush swirl:** the long-term band spectrum, the modulation spectrum of band envelopes (look for
     peaks at the revolution rate and twice it, per the patent), and the tap's spread (onset rise time).
- **Model-side checks:**
  - Energy conservation of the nonlinear core (lossless test).
  - Reproduce Bao's f_c(t) trends for h_e 1, 0.7 and 0.4 mm, a shape on or off (§3.5), with the
    offline reference tool.
  - Reproduce DAFx15's regimes (100 N weakly nonlinear, 300 N turbulent) on their rectangular plate.
- **Listening:** A/B with the stems, and against the project's mix of a jazz MIDI file. The kit
  switches Standard → Jazz → Brush mid-file.

## 9. Measurements to make first (from ~/Music)

1. Ride: the steady wash spectrum and band T60s after the last stroke of a phrase (endings).
2. Crash at a phrase ending: band levels against time for 2 s.
3. The foot chick on 2 and 4 under ride time: averaged chick spectra (aligned on onsets, ride bleed
   subtracted by averaging).
4. Brush swirl: a held swirl passage (still to be found: drums.md §7 found taps only in *Some Cats
   Know*).
5. Snare ghost notes against accents: the rattle duration and the spectral tilt.

## 10. Open questions

- **The cascade sink's spectral law and rate:** fit to crashes. Is a single energy-proportional sink
  enough for both the ride's mild spreading and the crash's bloom?
- **Coupling tensors for a domed, tapered cymbal**, which are not a flat plate's. Bao's Rayleigh–Ritz
  route is offline, so build `tools/vkplate.py` on it. The flat-plate tensors scaled by local thickness
  may be enough for sound.
- **Hi-hat:** the closed pre-load's effect on the modes (a stiffened pair). Is the contact ring with
  16 patches enough, or does the closed state need a linearised coupled mode set computed for each
  pedal force? The chick's spectrum, still to be measured.
- **Brush texture parameters:** grain spacing, tip stiffness, the number of active wires. None
  published, so fit to recordings.
- **GM2 brush swirl note-off:** no GM2 statement found. Device behaviour varies (§2.4).
- **Later Roland Brush sets** may add brush keys (e.g. rolls) at 25–34 **[memory, unverified]**.
- **Cymbal masses and the ride T60s** in §3.1 and §3.3 are from memory or a lab plate. Measure or find
  maker data.

## 11. Sources

Papers read (full text) for this document:
- [DAFx15] M. Ducceschi, C. Touzé, "Simulations of nonlinear plate dynamics: an accurate and efficient
  modal algorithm", Proc. DAFx-15, Trondheim (2015).
  https://www.ntnu.edu/documents/1001201110/1266017954/DAFx-15_submission_22.pdf
- [Bao19] Q. B. Nguyen, C. Touzé, "Nonlinear vibrations of thin plates with variable thickness:
  application to sound synthesis of cymbals", J. Acoust. Soc. Am. 145(2), 977–988 (2019),
  doi:10.1121/1.5091013. https://perso.ensta-paris.fr/~touze/PDF/2019_BaoCT_TaperedCymbals_JASA.pdf
- [SMAC13] M. Ducceschi, C. Touzé, S. Bilbao, "Sound synthesis of gongs obtained from nonlinear thin
  plates vibrations: comparison between a modal approach and a finite difference scheme", SMAC 2013. https://perso.ensta-paris.fr/~touze/PDF/ducceschi_smac2013.pdf
- [BWWD23] S. Bilbao, C. Webb, Z. Wang, M. Ducceschi, "Real-time gong synthesis", Proc. DAFx23,
  Copenhagen (2023). https://www.dafx.de/paper-archive/2023/DAFx23_paper_19.pdf
- [DS25] R. Diaz, M. Sandler, "Fast differentiable modal simulation of non-linear strings, membranes,
  and plates", Proc. DAFx25 (2025). https://dafx25.dii.univpm.it/wp-content/uploads/2025/09/DAFx25_paper_35.pdf
- [SA19] T. Skare, J. Abel, "Real-time modal synthesis of crash cymbals with nonlinear approximations,
  using a GPU", Proc. DAFx-19, Birmingham (2019). https://dafx.de/paper-archive/2019/DAFx2019_paper_48.pdf
- [SS23] S. Sekiguchi, T. Samejima, "Physical modeling and sound synthesis of the hi-hat", Acoust. Sci.
  & Tech. 44(5), 352–359 (2023), doi:10.1250/ast.44.352.
  https://www.jstage.jst.go.jp/article/ast/44/5/44_E2293/_pdf/-char/en
- [THB14] A. Torin, B. Hamilton, S. Bilbao, "An energy conserving finite difference scheme for the
  simulation of collisions in snare drums", Proc. DAFx-14, Erlangen (2014).
  https://www.pure.ed.ac.uk/ws/files/17485699/dafx14_submission_56_3.pdf
- [TN] A. Torin, M. Newton, "Collisions in drum membranes: a preliminary study on a simplified system"
  (ISMA/SMAC 2014, NESS). https://www.ness.music.ed.ac.uk/wp-content/uploads/2014/05/Snare_final.pdf
- [BTC14] S. Bilbao, A. Torin, V. Chatziioannou, "Numerical modeling of collisions in musical
  instruments", Acta Acustica u. A. 101 (2015); arXiv:1405.2589. https://arxiv.org/pdf/1405.2589
- [FA23] M. Ducceschi, M. Hamilton, R. Russo, "Simulation of the snare-membrane collision in modal form
  using the scalar auxiliary variable (SAV) method", Forum Acusticum 2023, Turin.
  https://dael.euracoustics.org/confs/fa2023/data/articles/000995.pdf
- [Skrodzka06] E. B. Skrodzka, E. Hojan, R. Proksza, "Vibroacoustic investigation of a batter head of a
  snare drum", Archives of Acoustics 31(3), 289–297 (2006) (includes Rossing et al.'s values).
  https://acoustics.ippt.pan.pl/index.php/aa/article/download/674/592/1079

Web pages, abstracts and excerpts:
- Wikipedia, "General MIDI Level 2" (drum sets, keys 27–34/82–87, Bank Select MSB 120/LSB 0, channels
  10/11). https://en.wikipedia.org/wiki/General_MIDI_Level_2
- Void Audio, "GM/GM2/GS percussion map" (Standard/Jazz/Brush key differences).
  https://www.voidaudio.net/percussion.html
- PG Music, "General MIDI instrument and drum kit lists" (Brush kit: 38 tap, 39 slap, 40 swirl; Jazz kit
  description). https://pgmusic.com/tutorial_gm.htm
- MIDI.org forum, "General MIDI Level 2 percussion questions"; "GM2 ch 11 percussion".
  https://midi.org/community/midi-specifications/general-midi-level-2-percussion-questions
- Kurzweil PC88 manual, Appendix C (Jazz kit = program change 32), via search snippet.
  https://www.manualslib.com/manual/888154/Kurzweil-Pc88.html?page=79
- Roland MIDI implementation (drum parts: note-off received when Rx.NOTE OFF = ON), via search snippet.
  https://manualmachine.com/roland/at900c/14865586-instruction
- Paiste, "Cymbal sound classification" (ping, dry/wet).
  https://www.paiste.com/en/about/everything-cymbals/cymbal-sound-classification-system
- Digital Drummer, "Brushing up on e-brushes" (micro-hits and friction; sweeps are hard to sense).
  https://digitaldrummermag.com/2023/12/24/brushing-up-on-e-brushes/
- US patent 4,181,059, "Circuit for simulating sound of wire brush rotated around head of snare drum"
  (cyclic timbre per revolution and half-revolution).
  https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/4181059
- Rossing, "Acoustics of drums", Physics Today (snare: two mode pairs from (0,1) and (1,1); two-mass
  model). https://physicstoday.aip.org/features/acoustics-of-drums
- Wikipedia, "Hi-hat" (construction, chick). https://en.wikipedia.org/wiki/Hi-hat
- Cited from the papers' reference lists, not read: S. Bilbao, "Percussion synthesis based on models of
  nonlinear shell vibration", IEEE TASLP 18, 872–880 (2010); T. Samejima, "Nonlinear physical modeling
  sound synthesis of cymbals involving dynamics of washers and sticks/mallets", Acoust. Sci. & Tech. 42,
  314–325 (2021); O. Thomas, C. Touzé, A. Chaigne, "Non-linear vibrations of free-edge thin spherical
  shells: modal interaction rules and 1:1:2 internal resonance", Int. J. Solids Struct. 42 (2005);
  A. Chaigne, C. Touzé, O. Thomas, "Nonlinear vibrations and chaos in gongs and cymbals", Acoust. Sci. &
  Tech. 26, 403–409 (2005); M. Ducceschi, C. Touzé, JSV 344, 313–331 (2015); F. Avanzini, R. Marogna,
  IEEE TASLP 18(4), 891–902 (2010) (modular membrane percussion); F. Avanzini, R. Marogna, B. Bank,
  JASA 131(1), 897–906 (2012) (tension modulation by energy estimation); S. Bilbao, "Time domain
  simulation and sound synthesis for the snare drum", JASA 131, 914–925 (2012); Bilbao, Russo, Webb,
  Ducceschi, "Real-time guitar synthesis", DAFx24.

In this repository: `research/drums.md` (goals, the brush strokes, Demucs progress),
`research/bells_blocks.md` §0 and §4 (Hertz table, crash modes and damping, reverse cymbal methods),
`research/steelpan_taiko.md` (two heads coupled by air), `src/modal.h/.cc` (the engine),
`src/modalinst.cc` (the reverse cymbal), `src/midifile.cc` (`drumFor`, event handling).
