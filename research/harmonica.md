# The harmonica (GM 23): a chromatic harmonica as a physical model

Status: 2026-11-16. Research and plan; nothing is built. This deepens `research/free_reeds.md` §5, §7, §8
and §12 (the free reed, the accordion and the bandoneón, built in `src/freereed.h` and
`src/accordion.cc`), and does not repeat them. The program table in `src/midifile.cc` still sends
`harmonica` to `lead-square`.

The user has decided (2026-11-16):
- a **chromatic** harmonica with as much range as a real one has;
- the **slide** modelled as a physical travel, not an instant switch;
- **chords** modelled physically (one mouth and one vocal tract feeding several reed chambers), with
  one breath direction at a time; when a chord asks for both, the direction is chosen by velocity;
- the **hands** as a resonating cavity whose opening the phrasing drives, coupled to the room;
- **hand tremolo** as a feature.

§10 is shaped around these decisions.

Marks: **[paper]** a paper below; **[web]** a weaker web source below (makers, dealers, forums);
**[calc]** worked out here; **[memory]** general knowledge, to be checked; **[design]** a modelling
choice; **[file]** measured in the user's MIDI files; **[rec]** measured in the user's recordings
`~/Music/harmonica.wav` and `~/Music/harmonica2.wav` (numbers only, no audio copied; scripts in the
session scratchpad).

## 1. Short answer

- **The physics is a series loop:** lungs, the vocal tract, the mouth, the lips' opening, the comb's
  chamber, the reed, the outside (the cupped hands), the room. A blown-closed reed in series with a
  resonator sounds *below* the resonator's frequency **[paper, C11, Fletcher via C11]**. Bending is
  the player tuning a tract resonance just above the wanted pitch. A single valved reed can be pulled
  down "a semitone or so" (Johnston's model) **[paper, BAB98 citing J87]**. On a diatonic both reeds
  of a chamber take part, so it bends further, up to three semitones, and overblows **[paper, BAB98]**.
- **For the engine** (`FreeReed`), where the tract sits does not matter. For a blow note the tract is
  upstream of the reed; for a draw note it is downstream. Either way it is in series with the reed in
  one loop, so the reed sees Δp = p_supply − Z·u in both cases **[calc]**. Feed `FreeReed::sample`
  with ±(p_mouth − p_hands) and keep its cell and duct as the comb's chamber. The tract and the hands
  are two new lumped resonators around it.
- **The recordings [rec]:**
  - `harmonica.wav` is a **diatonic in C played in 2nd position (G blues)**. It has draw chords G4 + B4
    (+ D5) on holes 2-3-4 and bends of 2-3 semitones released over 200-280 ms. Its vibrato is from
    the throat: 6.0-7.4 Hz, 12-36 c peak-to-peak in pitch and 2.4-3.2 dB in level.
  - `harmonica2.wav` is a **chromatic in C**. It ends on the draw chord D4 F4 A4 B4 D5 (holes 1-5),
    which a diatonic cannot play. Its slide grace notes cross-fade in ≤ 30-40 ms, with both pitches
    sounding together and a 1-6 dB dip. It has scoops into most notes (−50 to −185 c, settling in
    30-80 ms) and a 6.5 Hz tremolo on the final chord (7 dB level, centroid ±130 Hz in phase, little
    pitch).
  - Breath reversals leave 15-37 dB dips 60-120 ms wide; note changes within one breath leave ≤ 10 dB
    and ≤ 15 ms.
  - Brightness rises steeply with level. A soft G4 tail has its centroid at 1.3 × f0; the loud note,
    8 dB louder, has it at 4 × f0.
  - **No speech was found in harmonica2.wav.** Its last 1.7 s (8.9-10.58 s) is the sustained draw
    chord: steady partials, no voice-band fundamental, no moving formants. It is all used.
- **The widest real layout** is the 16-hole C chromatic, one maker 64 Chromonica 280/64 or Super 64:
  **C3-D7, MIDI 48-98, 64 reeds** **[web, HPG11; dealer]**. Its exact note map is §2.1.
- **Valves (windsavers)** are plastic flaps on the slots of the low and middle holes. The top
  octave or so is unvalved. They send all the air to the speaking reed and stop the two reeds of a
  chamber "operating in tandem", which is why a chromatic bends less **[web, HPG11; WCH]**.

## 2. The chromatic harmonica's build

### 2.1 Layout and range

A chromatic is "two diatonic harmonicas joined together, tuned a semitone apart … By pressing in the
slide button … raising the pitch of each note by one semitone". Its layout is the middle octave of the
Richter harp, repeated in every octave ("solo tuning") **[web, HPG11]**. Per four holes the blow notes
are C E G C and the draw notes D F A B. Solo tuning "eliminates the G on the draw and doubles the Cs
that are not on the ends" **[web, WCH]**.

**The 16-hole C chromatic (one maker 280/64, Super 64, Super 64X; one maker and another maker 16-holes the same),
C3-D7.** The 12-hole (one maker 270, Toots Mellow Tone and Hard Bopper) is holes 5-16 of this table,
C4-D7 **[web, HPG11, WCH; calc]**:

| Hole | Blow | Draw | Blow + slide | Draw + slide |
|---|---|---|---|---|
| 1 | C3 48 | D3 50 | C#3 49 | D#3 51 |
| 2 | E3 52 | F3 53 | F3 53 | F#3 54 |
| 3 | G3 55 | A3 57 | G#3 56 | A#3 58 |
| 4 | C4 60 | B3 59 | C#4 61 | C4 60 |
| 5 | C4 60 | D4 62 | C#4 61 | D#4 63 |
| 6 | E4 64 | F4 65 | F4 65 | F#4 66 |
| 7 | G4 67 | A4 69 | G#4 68 | A#4 70 |
| 8 | C5 72 | B4 71 | C#5 73 | C5 72 |
| 9 | C5 72 | D5 74 | C#5 73 | D#5 75 |
| 10 | E5 76 | F5 77 | F5 77 | F#5 78 |
| 11 | G5 79 | A5 81 | G#5 80 | A#5 82 |
| 12 | C6 84 | B5 83 | C#6 85 | C6 84 |
| 13 | C6 84 | D6 86 | C#6 85 | D#6 87 |
| 14 | E6 88 | F6 89 | F6 89 | F#6 90 |
| 15 | G6 91 | A6 93 | G#6 92 | A#6 94 |
| 16 | C7 96 | B6 95 | C#7 97 | **D7 98** |

- **Every MIDI note 48-98 is present** **[calc]**. The top hole's slide-in draw is D7, not a third
  C7. One maker's chart shows it, and Wikipedia calls it "common, though by no means present in all
  chromatic harmonicas" **[web, HPG11, WCH]**.
- **Several pitches have more than one place [calc]:**
  - C (60, 72, 84) is blow on two holes (4/5, 8/9, 12/13) and also draw with the slide in (holes 4, 8,
    12);
  - C# (61, 73, 85) is blow + slide on two holes;
  - F (53, 65, 77, 89) is draw on hole 2/6/10/14 and blow + slide on the same hole;
  - C7 (96) is only the hole-16 blow.
  So C and F can be played in either breath direction, and players use this to stay on one breath
  (§10.3).
- **Other real ranges:** the one maker Meisterklasse is 3½ octaves, G3-C7. The 10-hole (260) is 2½
  octaves, the 12-hole 3 **[web, HPG11]**. Bass and chord harmonicas exist but are not solo
  instruments. **No solo chromatic wider than C3-D7 was found.**
- **What the user's files need:** MIDI 50-88 **[file]**, inside the 16-hole and also inside the
  12-hole except for 50-59. Notes outside 48-98 are folded by octaves (§10.3).

### 2.2 The slide

- **How it works:** a button-driven bar between the mouthpiece and the comb redirects the air. In a
  straight-tuned design it chooses "the upper reed-plate (button out) or the lower reed-plate (button
  in)". Cross-tuned designs stagger the plates by hole **[web, WCH]**. Each mouthpiece hole sits over
  two chambers, one per row, and the slide's openings uncover one row and cover the other **[memory]**.
  The 280/64 has a zigzag slide **[web, dealer]**.
- **Assembly:** mouthpiece, slide cage (the U-channel), slide, backing plate **[web, BHC]**. The spring
  is "very stiff wire with several turns … wound around a post inside the comb, with one arm reaching
  through the slide" **[web, search summary of a BH guide]**. Plastic bumpers stop the slide clanking
  on the screws. The slide is a thin plate, about 0.8-1.0 mm thick in a patent's overblow slide
  **[web, US5739446 summary]**. Leakage at the slide is the chromatic's main air loss **[web, BHC]**.
- **Travel and stroke:** no maker publishes a stroke in mm. One maker's Toots models and the 270/280 have
  a "short slide action" **[web, HPG11]**, and a another maker model's slide action is "2 mm shorter"
  **[web, Thomann listing]**. **Estimate [calc, memory]:** a hole pitch of ~7-8 mm (16 holes in a
  193 mm body) with zigzag openings gives a travel of half a pitch, **~3-4 mm**, and a button stroke
  of about the same, 3-5 mm. The spring force is a few newtons **[memory, unverified]**.
- **Timing [rec, harmonica2.wav]:** two slide grace notes (G5→G#5→G5 at 1.33-1.47 s; G4→G#4→G4 at
  7.62-7.75 s), tracked in 43 ms windows every 10 ms:
  - the new note's partials rise as the old ones fall over **~40-50 ms of analysis time**, so the true
    travel is **≤ 30-40 ms**;
  - mid-way **both pitches sound together**, within 3-15 dB of each other;
  - the **total level dips only 1.5-2 dB when the slide goes in** and **5-6 dB when the spring returns
    it**;
  - the held grace note lasts 60-100 ms.
  There is no breath gap: the reeds of both rows are fed through a half-open slide.
- **Physically [design]:** while the slide moves, the natural row's opening area falls as the sharp
  row's rises. If the openings overlap, both chambers are partly open for a moment. That is a
  cross-fade through two lip-to-chamber resistances in parallel, fed by one mouth pressure, and the
  measured dip says they overlap. Slide trills ("shakes") run at ~5-10 Hz **[memory]**.

### 2.3 Windsavers (valves)

- **Material:** thin one-way flaps "of plastic, knit paper, leather, or Teflon" glued to the plate,
  "slightly larger than the air channel" **[web, search summary of WB, BH]**. Modern harmonicas use
  plastic film, often two-layer **[memory]**.
- **Where:**
  - each reed's slot is covered on the side away from the reed: the blow reed's valve outside, the
    draw reed's inside **[web, HPG11; memory]**;
  - "On most chromatics, the first two octaves (or, on 16-hole chromatics, the first three) are
    valved" **[web, search summary]**;
  - windsavers sit on "the low and mid-range holes", and "unless the windsavers are removed, chromatic
    harmonicas cannot 'overblow' except on the upper four holes" **[web, WCH]**.
  - So the top ~4 holes are unvalved.
- **What they do:**
  - **Leakage:** "The valves close off the draw reed while exhaling and the blow reed while inhaling,
    thus preventing air loss, but also preventing the two reeds from operating in tandem as on the
    Richter harp" **[web, HPG11]**. They direct all the air to the reed being played **[web, BH]**.
  - **Bending:** single-reed only, so shallower and less free than a diatonic's two-reed bend; on a
    fully valved chromatic, draw bends of about a semitone are usual **[web, forums; BAB98 for the
    single-reed limit]**.
  - **Response:** valves stick and buzz when wet or cold, and lift late on soft attacks. That is a
    quiet note's slow start and occasional flap noise **[memory]**.
- **The unvalved top holes** behave like a diatonic chamber: the other reed of the pair moves a
  little ("the secondary reed always oscillates" **[paper, C11]**, at ~1/20 of the primary's amplitude
  **[paper, BAB98]**) and leaks air.

### 2.4 Reed plates and reeds

- **Plates:**
  - brass, 1.05 mm on the 270, 280/64 and Mellow Tone; 1.2 mm on the 270 Deluxe, Hard Bopper,
    Meisterklasse and Amadeus **[web, HPG11]**;
  - the Super 64X has "double thickness reed plates in the bottom two octaves" for a "uniquely
    powerful sound" **[web, HPG11]**;
  - thicker plates make a longer slot: more inertance in the slot, fuller and louder, slower to speak
    **[web, HPG11 claims; memory]**;
  - MB07's diatonic plate is 0.9 mm **[paper, MB07]**.
- **Reed material:** brass (one maker), phosphor bronze (another maker), stainless steel (another maker: the "first
  mass-produced chromatic harmonica with stainless steel reeds") **[web, search; memory]**.
  √(E/ρ) is ~3430 m/s for brass, ~3600 for phosphor bronze and ~5000 for stainless **[calc, memory
  moduli]**. Steel is stiffer and less damped (a higher Q, a longer life, a slightly brighter and
  quicker start) **[memory]**.
- **Sizes across the range:** no published table was found.
  - MB07's diatonic reed at 444 Hz is 12.95 × 2.1 × 0.110 mm, set 0.53 mm, gap 50 µm **[paper,
    MB07]**.
  - The accordion scaling already in `freereed.h` (length 13 mm × (444/f)^0.42) gives ~22 mm at C3
    and ~6.5 mm at D7; width 2.1 mm × (444/f)^0.2 gives 2.7 and 1.5 mm. That is plausible for
    harmonica reeds **[calc; memory: low harmonica reeds ~20-25 mm, the top ~6-7 mm]**. Low
    chromatic reeds are thicker or carry a little tip weight **[memory]**.
  - The chamber is ~4-5 mm wide, so the width stays under ~3 mm **[calc]**.
- **Gapping (the set):** a few tenths of a mm at the top to ~1 mm in the bass. A lower set speaks
  quicker but chokes on hard playing; a higher set takes more air **[memory]**. MB07's 0.53 mm on a
  13 mm reed (4 %) is the model's default (`set = 0.04 L`).

### 2.5 Comb, chambers, mouthpiece, covers

- **Comb:** pearwood (270, Toots models), plastic (280/64, Super 64) or acrylic or aluminium (Amadeus,
  Meisterklasse) **[web, HPG11]**.
  - Each chamber is "a cell containing both blow and draw reeds" **[web, WCH]**: on a chromatic both
    reeds of a chamber are on the same plate, side by side, the blow reed inside and the draw reed
    outside **[memory]**. On a diatonic they are on opposite plates.
  - Two rows of chambers, one per plate: natural and sharp.
- **Chamber volume [calc, memory]:** ~25 mm long, ~4 mm wide, ~3.5 mm deep (half of the comb's two
  tiers): **~0.3-0.4 cm³**. Its quarter-wave and Helmholtz resonances are in the kHz. Cottingham: for
  high reeds "a resonance of the reed chamber can be close to the vibration frequency of the reed
  tongue … inhibiting or possibly preventing the sounding of the reed"; builders change the chamber,
  and in the harmonica "skilled players may be able to overcome this … by appropriate changes in vocal
  tract resonances" **[paper, C17]**.
- **Mouthpiece:** nickel- or chrome-plated brass, round or rectangular holes about 3-4 mm across on a
  ~7-8 mm pitch **[web, HPG11 "round holes"; memory sizes]**.
- **Covers:** stainless steel or chrome-plated brass. They open towards the back, away from the
  player: the sound leaves from there and from the ends, into the cupped hands **[memory]**. The
  Silver Concerto's "clear, brilliant sound" is a maker's claim about the covers **[web, HPG11]**.
- **The Toots models [web, HPG11]:**
  - **Mellow Tone** (7538): 12 holes, 48 reeds, 1.05 mm chrome-plated plates, short slot reeds,
    pearwood comb, short slide action; "full, warm tone for ballads and classical pieces".
  - **Hard Bopper** (7539): 12 holes, 1.2 mm plates, long slot reeds; "bright, powerful sound".

## 3. The diatonic (Richter) harmonica, briefly

- **Layout (C):** 10 holes, blow C4 E4 G4 C5 E5 G5 C6 E6 G6 C7 and draw D4 G4 B4 D5 F5 A5 B5 D6 F6 A6.
  Blowing three neighbours gives the tonic chord, drawing holes 1-4 the dominant **[paper, BAB98]**.
  The blow reeds sit inside the top plate and the draw reeds outside the bottom plate, opposite each
  other over one chamber, **unvalved** **[paper, BAB98]**.
- **Bends:** the higher note of a hole bends down towards the lower one: draw bends in holes 1-6, blow
  bends in 7-10. Hole 3 draw goes down 3 semitones, holes 2 and 10 two, the others one.
  - The higher reed bends down and the lower reed bends up, and the sound passes from one to the
    other. In a 3-draw bend: B♭ from the draw reed, A from both, A♭ from the blow reed **[paper,
    BAB98]**.
  - With the partner blocked, a closing reed alone bends only "a semitone or so" (Johnston), and the
    partner's presence sets the lower limit **[paper, BAB98]**.
- **Overblows and overdraws:** a third mode. The *other* reed speaks as an opening reed, a semitone or
  so above its own pitch; the pitch "pops" in rather than gliding. A dissonant overblow has the two
  reeds at different frequencies (518 and 358 Hz in hole 3) **[paper, BAB98]**.
- **Which reed sounds:** the closing reed on straight notes. The partner moves at ≤ 1/20 of its
  amplitude, in parallel on draw notes, which is counter-intuitive **[paper, BAB98]**.
- **The tract:** the anterior oral volume falls as the bent pitch rises (water-volume measurements,
  holes 1 and 4) **[paper, BAB98]**.
  - The tongue humps into anterior and posterior chambers. "The size … of the Anterior Chamber is what
    controls the depth of a bend"; deeper bends move the constriction back **[web, Barrett/MRI
    BHM]**.
  - 3D-printed MRI tracts reproduce a player's bends **[paper, VT15]**.
- **For pajupilli later:** the same parts as the chromatic (§10) with two unvalved reeds per chamber
  and no slide. Bends and overblows then come from the two-reed coupling, which `FreeReed` can run
  (two reeds on one cell; the opening reed's sign reversed).

## 4. The vocal tract coupling

### 4.1 What the literature gives

- **Johnston (1987)**, "Pitch control in harmonica playing", Acoustics Australia 15:69-75 (the title
  given in the brief, "Accounting for the harmonica bending phenomena", was not found). It used
  Fletcher's linear reed admittance and a variable-length cylinder for the tract. Results:
  - the bendable notes are those whose primary reed is the higher one;
  - they bend down into the interval;
  - a single closing reed can be flatted about a semitone.
  Read through **[paper, C11, BAB98]**; the article itself was not found.
- **Fletcher's model (via Cottingham):** self-sustained oscillation needs Re Y_reed < 0. A
  blown-closed free reed coupled to a pipe sounds slightly below the pipe's resonance; a blown-open
  (Asian) reed sounds above it **[paper, C11, N98]**. Tonon's pitch-bending accordion uses a cavity
  tuned near the reed and the key pressure to bend "about one semitone" **[paper, C11]**.
- **Accordion bending is a different mechanism:** a pressure loss at the half-shut pallet that lowers
  the effective stiffness, −2.5 vs −5.1 Hz/kPa **[paper, A26]**. The harmonica's is the tract's
  resonance **[paper, A26, C11]**. `accordion.cc` uses the pallet; the harmonica must use the tract.
- **Tract impedance during playing:** measured in clarinettists' and saxophonists' mouths (the
  closest data; none exists for the harmonica):
  - peaks of **20-60 MPa s m⁻³**;
  - the resonance set **~600-1500 Hz** for bends;
  - bends reach ~190 c **[paper, CSW09]**.
- **Tract bandwidths through the lips** (phonation, glottis closed): 50-90 Hz for the first three
  resonances (Q ≈ 6-20 at 0.5-1.5 kHz); opening the glottis widens them **[paper, HSW16]**. A
  harmonica is played with the glottis open, so **Q ≈ 4-10** **[calc, from HSW16]**.
- **Bahnson:** "a wide variety of air flow rates, vocal cavity volumes and vocal configurations are
  compatible with the production of natural tones", but bent notes require specific ones **[paper,
  BAB98]**. Constricting the throat with "a simultaneous increase in pressure from the lungs" (Sonny
  Terry via Palmer) **[paper, BAB98]**.
- **Chromatic bending specifically:** no paper was found. Practitioners say a fully valved chromatic
  bends single reeds (draw bends mostly, about a semitone), that half-valving limits draw bends, and
  that an unvalved chromatic leaks and loses most of its bending **[web, forums]**.
- **[rec] on the chromatic (harmonica2.wav):** most notes enter from below: −47 to −185 c, median
  ~−70 c, settling to ±15 c in **28-80 ms** (median ~40 ms). This is in both blow and draw notes,
  so it is a tract or breath scoop (and the reed's onset), not a two-reed bend. There are no long
  held bends. The user's MIDI asks down to −2 st, median −0.5 **[file]**.

### 4.2 Numbers for a lumped tract [calc]

- **The tract seen from the harmonica:** model it as a Helmholtz resonator. The oral cavity is a
  compliance C_t = V_m/(ρc²); the tongue's constriction plus the pharynx is an inertance M_t; R_t
  holds the losses. From the lips, Z_t(ω) = 1/(jωC_t + 1/(jωM_t + R_t)) is a **parallel resonance**:
  a peak of Q·Z₀, with Z₀ = √(M_t/C_t), at f_t = 1/(2π√(M_t C_t)).
- **Typical values:** V_m = 30-60 cm³ (BAB98's water volumes are the anterior part; the whole is
  larger) and f_t = 600 Hz give C_t ≈ 3.5e-10 m³/Pa, M_t ≈ 200 kg/m⁴ and Z₀ ≈ 0.75 MPa s m⁻³. With
  Q 8 the peak is ~6 MPa s m⁻³.
- **The reed's own scale:** ~600 Pa / 300 mL/s ≈ 2 MPa s m⁻³, and the engine's duct inertance of 960
  kg/m⁴ at 444 Hz is 2.7 MPa s m⁻³. The tract's peak is comparable to or above the reed's, enough to
  pull it, as CSW09's tens of MPa s m⁻³ do on the clarinet.
- **Moving f_t:** the tongue's arch narrows the constriction, which raises M_t and lowers f_t. A
  larger anterior volume raises C_t, which also lowers f_t. Both match BAB98 and the MRI description.
- **Neutral (unbent) playing:** keep f_t well away from the reed, above it by ≥ 1.5-2× for low notes
  (the tract then looks like a small mass, as the engine's duct already does) and below it for high
  notes. A neutral open-throat f_t of ~300-700 Hz with Q ~3 is safe **[design]**.

### 4.3 How far a valved chromatic reed bends, and how the sound changes

- **Depth:** ~1 semitone of steady bend; deeper only briefly or with a choked, falling level
  **[paper, BAB98/J87; web; rec scoops to −1.9 st lasting < 80 ms]**.
- **Sound as it bends:**
  - level falls and harmonicity is kept (the accordion's 10-35 c **[paper, EMS21]**);
  - timbre follows the tract's resonance, which sits near the fundamental, so the bent note is darker
    and "fatter", its fundamental relatively stronger **[paper, CSW09 for the clarinet; memory for the
    harmonica]**.
- **[rec] harmonica.wav (diatonic):** bend releases of 2-3 semitones over 200-280 ms (~1000 c/s); a
  4-draw half-step bend held at −80 to −110 c. The level of bent notes did not differ measurably from
  straight ones in this short take.

### 4.4 A model a synthesizer can run [design]

- **One tract state per instrument** (one mouth), with the lungs as a pressure source p_L behind the
  tract's inertance:

      M_t du_t/dt = p_L − p_m − R_t u_t                (flow through the tract into the mouth)
      C_t dp_m/dt = u_t − Σ_i s_i u_i                  (the mouth; s_i = +1 blow, −1 draw)

  Take f_t and Q_t from the bend, and C_t from a fixed V_m: M_t = 1/((2πf_t)² C_t),
  R_t = √(M_t/C_t)/Q_t.
- **Bend b** (semitones, ≤ 0; the files' pitch bend at ±2 st):
  - f_t → f_target·(1 + δ), with f_target = f_note·2^(b/12) and δ ≈ +3-6 % (the blown-closed reed
    sounds below the resonator);
  - Q_t rises from ~3 (neutral) to ~8-12 (a deep bend);
  - p_L rises ~×(1 + 0.3|b|) (the "simultaneous increase in pressure").
  - Fit δ and Q per register so the reed's pitch follows b to about −1 st, then saturates with a
    falling level. The user's deeper bends are followed as far as the reed goes. The test is that the
    deviation from the target stays within a few cents down to −1 st (§10.6).
- **Scoop on attack:** f_t starts lower (~−1 st) and moves to neutral over ~40 ms on a fraction of
  notes, set by velocity, more on draw notes; this matches the [rec] scoops.
- **Why one resonator is enough:** only the resonance near the reed matters; Cottingham's chamber
  remark and the clarinet data deal with one tract peak at a time. A two-mode tract (anterior and
  posterior chambers) is the upgrade if the timbre needs it.

## 5. The hands

### 5.1 Acoustics of cupping

- **No measurements of the hand cavity were found.** Players report a "hand resonance" in which a
  certain opening makes a note louder, sometimes a larger opening for higher notes. Wah is described
  as a band-pass sweeping with the hand **[web, forums]**. A cupped-hand clap is a Helmholtz resonator
  **[web, PTclap]**. A hand in a horn bell adds acoustic mass and shifts the intonation **[web,
  Watts09]**. BAB98 explicitly left cupping out.
- **Helmholtz estimate [calc]:** cavity V_h = 50-100 cm³ of air (two cupped hands round a 15-19 cm
  harmonica, minus its own volume: estimate). The opening between the hands is an equivalent circular
  hole of radius a; for a thin aperture L_eff ≈ 1.7a, so f_h = (c/2π)√(πa/(1.7 V_h)):

  | a (mm) | 2 | 5 | 10 | 20 |
  |---|---|---|---|---|
  | f_h at V_h = 100 cm³ (Hz) | 330 | 525 | 740 | 1050 |
  | f_h at V_h = 50 cm³ (Hz) | 470 | 740 | 1050 | 1490 |

  Above a ≈ 20-30 mm (ka → 1 at 1-2 kHz) the cavity stops being lumped: the hands are open.
- **Q of the hand cavity [calc]:** at a = 10 mm and 50 cm³, M = ρL/A ≈ 65 kg/m⁴ and the flanged
  radiation resistance R ≈ ρω²/(2πc) ≈ 24 kPa s m⁻³, so Q_rad ≈ 18. Soft, lossy hands lower it,
  **Q ≈ 4-10** **[calc, memory]**. Z₀ = √(M/C) ≈ 0.4 MPa s m⁻³, so the peak is ~2-4 MPa s m⁻³.
- **The pitch:** the hand cavity is in series in the same loop, on the outside of the reed. For a blow
  note it is downstream, for a draw note upstream. Its peak is about the reed's scale, so cupping
  can pull the pitch a few cents when f_h comes near the reed's frequency and it helps or chokes the
  level **[calc]**. This is the "hand resonance" players hear.
- **Radiation and projection:** cupped, the source is the small opening, and below f_h the radiated
  pressure follows the flow (−12 dB/octave above f_h): **dark, quieter, the highs held in**. Opening,
  f_h rises past the partials (the wah) and the full slot sources radiate: **brighter and louder**.
  - Directivity: cupped, the sound leaves through a small, often downward or sideways gap between
    the fingers; open, the harmonica faces forward through the covers' back openings. That forward
    projection and the high-frequency power going into the room are what "more of the room" means
    **[memory, design]**.
- **[rec]:** both recordings have a resonance at **~0.8-2 kHz** that lifts one partial 10-25 dB above
  the fundamental (H3 at 1.18 kHz on G4, H3 at 1.77 kHz on D5, H2 at 1.40 kHz on F5, H3 at 1.31 kHz on
  A4) and moves with time, independent of the note. It fits a hand cavity (or the tract) at f_h ≈
  1-1.8 kHz: a = 10-20 mm in the table.

### 5.2 Hand tremolo and wah

- **Rates [memory, web]:** hand tremolo (fluttering hands) runs at ~3-8 Hz. Wah is a single slow
  opening ("doo-wah") over ~100-300 ms at phrase ends and on long notes **[memory]**.
- **[rec] harmonica2.wav, final chord (8.6-10.4 s):**
  - **6.5 Hz** (mean period 154 ms, CV 0.12: fairly regular, wandering ±12 %);
  - **level 7.2 dB peak-to-peak**;
  - **centroid ±130 Hz (264 Hz p-p) in phase with the level** (correlation +0.77): louder is
    brighter;
  - pitch: the D5 partial moves 6 c rms (the lower partials more, partly the amplitude sidebands).
  - That is the hand-tremolo signature: level and brightness together, little pitch.
- **[rec] harmonica.wav (diatonic), throat vibrato on long G4s (5 notes):**
  - 6.0-7.4 Hz, pitch 12-36 c p-p, level 2.4-3.2 dB p-p (one 7.3 dB), centroid 200-720 Hz p-p;
  - level anti-correlated with the centroid (−0.88 to −0.99) and with the pitch (−0.76 to −0.93):
    louder when flatter and darker;
  - that is the tract (a bend-like pull) pulsing, not the hands.
- **Combination:** players use throat or diaphragm vibrato, the hands, or both **[memory, web]**. Jazz
  chromatic playing (Toots) leans on a light vibrato and lets the hands colour the end of phrases
  **[memory, to check]**.
- **Hand timing in phrasing [memory, design]:**
  - closed while playing;
  - opening on the last note's tail ("doo-wah") over ~100-250 ms;
  - opening during a long note, 40-70 % into it, sometimes;
  - opening in rests longer than ~0.25 s;
  - closing again 50-100 ms before the next onset.

## 6. Chords

### 6.1 The mouth over several holes

- **Pucker (lip pursing):** the lips cover 1 hole for single notes, 2-4 for chords. **Tongue
  blocking:** the lips cover ~3-4 holes and the tongue blocks those that should not sound. Single
  notes come out of the right side of the mouth; octave and split chords (hole i with i+3 or i+4)
  have the tongue in the middle **[paper, BAB98; memory]**. Toots is described as a pucker player by
  one observer and tongue-blocking by a Liège poster **[web, BHE, ULg]**: both are in jazz use.
- **Slap and pull-off (tongue blocking):**
  - a slap: the tongue lands on the left holes just as the note starts, so a short chord flashes
    first;
  - a pull-off: the tongue lifts, then returns, so a chord sounds briefly between single notes
    **[memory]**;
  - the tongue's open/shut on the holes takes ~5-20 ms **[memory, design]**.
- **Leakage:** a partly covered neighbouring hole sounds 10-25 dB down **[design, to fit]**. With a
  pucker that is not perfect, the neighbours colour single notes.
- **The air:** one mouth pressure p_m drives every open chamber. The chambers are in parallel, each
  through its own lip opening; their flows add into one tract flow. A chord **takes more air** (each
  reed passes hundreds of mL/s **[paper, C11]**), so the tract's loss and inertance drop p_m a little
  and each reed gets less. The player pushes harder: the chromatic "commonly takes more air" **[web,
  BHC]**.

### 6.2 One tract, all reeds

- **A bend or tract resonance acts on every note of the chord at once**, by how near each reed is to
  f_t: the reed nearest the resonance is pulled most, and the others are pulled less or choked
  **[calc, from §4]**. So a "bent chord" bends unevenly: the note nearest f_t moves most. That is
  physical, and the test should allow it (§10.6).
- **Interaction between reeds sharing a cavity:**
  - **Same chamber (blow and draw reed of one hole):** strong coupling. Parallel motion, a smooth
    shift of primacy in bends, and locking in a consonant overblow (both at 523 Hz), but different
    frequencies in a dissonant one **[paper, BAB98]**.
  - **Different chambers, one mouth (a chord):** weak coupling through p_m. Tremolo harmonicas sound
    two reeds tuned a few Hz apart through one mouth and beat instead of locking, and the accordion
    musette does the same through one bellows **[memory; free_reeds §3.1]**. So chord notes do not
    lock. Near-unisons beat, and a resonance between them can lower the level. No measured study of
    locking between chambers was found.
- **The level falling:** each added reed lowers p_m by the extra flow times the tract and airway's
  resistance, a few percent per reed at moderate levels **[calc: R_airway·u ≈ 0.1-0.3 MPa s m⁻³ ×
  300 mL/s ≈ 30-90 Pa against ~1 kPa]**. The player compensates in part.

### 6.3 Non-adjacent MIDI chords

A real chord is adjacent holes under one mouth (≤ 4), all in one direction and one slide state.
Otherwise the player plays the nearest adjacent set that holds most of the notes (often an inversion
or a subset), or a tongue-blocked split (holes i and i+3/i+4), and leaves the rest out **[memory,
design]**. The algorithm is §10.3.

## 7. The sound

- **Spectrum [rec]** (8192-point FFT, levels relative to the strongest harmonic):

  | Note (file, t) | Level re loudest | H1 | H2 | H3 | H4 | H5 | H6 | Centroid / f0 |
  |---|---|---|---|---|---|---|---|---|
  | D4 294 Hz (1, 7.75 s) | −20 dB | 0 | −28 | −8 | −14 | −12 | −18 | 5.5 |
  | G4 393 (1, 13.5) | −13 | −2 | −19 | 0 | −10 | −15 | −24 | 3.8 |
  | G4 389, soft tail (1, 19.25) | −22.5 | 0 | −30 | −27 | −20 | −29 | −29 | **1.3** |
  | G4 389, same note loud (1, 18.45) | −14.6 | 0 | −22 | −12 | −14 | −17 | −24 | **4.0** |
  | D5 591 (1, 10.65) | −17 | −19 | −4 | 0 | −19 | −10 | −19 | 3.3 |
  | F5 699 (1, 10.9) | −6 | −24 | 0 | −4 | −23 | −9 | −9 | 2.9 |
  | D4 294 (2, 6.4) | −6 | −2 | 0 | −16 | −21 | −7 | −8 | 3.7 |
  | G4 395 softer (2, 7.05) | −7 | 0 | 0 | −19 | −12 | −30 | −13 | 2.3 |
  | G4 395 louder (2, 7.5) | −4 | −1 | 0 | −11 | −13 | −19 | −5 | 3.0 |
  | A4 436 (2, 3.45) | −2 | −12 | −21 | 0 | −21 | −43 | −32 | 2.9 |
  | G5 790 (2, 1.2) | −3 | −1 | 0 | −5 | −16 | −10 | −26 | 2.0 |

  - The free reed's odd/even richness is there: the near-square puffs **[paper, C11]**.
  - A **resonance at 0.8-2 kHz** lifts whichever partial falls there (§5.1).
  - When the two puffs per cycle are equal (the tip swinging symmetrically through a thin plate), H2
    is weak. In harmonica.wav H2 is often 20-30 dB down **[rec; calc: the puff asymmetry sets the even
    harmonics]**.
  - Energy above 4 kHz is 7-25 dB below the total.
- **Brightness against level [rec]:** on one G4, 8 dB softer takes the centroid from 4.0 to 1.3 ×
  f0. In harmonica2.wav, 3 dB louder takes it from 2.3 to 3.0 × f0. Physically, at low amplitude the
  tip barely leaves the slot and the flow is near-sinusoidal; at high amplitude the puffs sharpen
  **[paper, MB07; free_reeds §12]**. The model gives this by itself if the supply pressure follows the
  dynamics.
- **Attack:**
  - **[rec] breath attacks from silence**, 10-90 % rise: 19, 21, 61, 73 ms (harmonica.wav) and 35 ms
    (harmonica2.wav). The −3 dB point comes 17-36 ms after onset.
  - **Tongued attacks** (a "t" or "d": the tongue releases the hole with the pressure already built)
    are faster, ~5-15 ms **[memory]**; many of harmonica2.wav's note-to-note onsets are of this kind.
  - Free reeds start slowly when large and low (a few tens of ms at C3), the torsional and second
    modes helping early **[paper, C17]**.
- **Release [rec]:** 90-10 % falls into silence of **120, 168, 209 ms** (breath dying, harmonica.wav)
  and 57 ms (harmonica2.wav, a tongue stop).
- **Noise [rec]:** between the harmonics (500-5000 Hz, excluding ±70 Hz round each partial), the floor
  is **−40 to −64 dB per 5.9 Hz bin** below the strongest harmonic on clean notes. Integrated, the
  breath noise is **~−25 to −35 dB** below the tone **[calc]**. It rises to −12 to −30 dB in
  transitions and chords. Recording noise floor: harmonica.wav is digitally silent between takes;
  harmonica2.wav sits at −44 dB re peak.
- **Vibrato:** §5.2. The throat (tract) gives pitch with level and timbre, 6-7.4 Hz; the hands give
  level with timbre, ~6.5 Hz; the diaphragm gives level with a little pitch, slower (~3-5 Hz)
  **[rec; memory]**.
- **Loudness:** no measurement found. Close-miked playing is quiet. A Liège poster on Toots: "not so
  much a powerful breath as a subtle one" **[web, ULg]**. A loud harmonica is roughly an accordion's
  level (mf ~70 dB(A) at 50 cm **[paper, EMS21]**) **[memory, to check]**.
- **Radiation:** the reed's outer slot side through the back of the covers and the ends, then the
  hands. A monopole at low frequency, more forward and directional when the hands are open **[memory,
  design]**.

## 8. Players and playing

- **Blowing pressure:** the reed's over-pressure in play is ~570-800 Pa in MB07 **[paper]**; the
  engine starts at 100-150 Pa and swings ±1.8-3.6 mm from 300 Pa to 2 kPa **[calc, free_reeds §12]**.
  Mouth pressures of ~0.3-3 kPa cover pp-ff **[memory, to check]**. The lips' and slide's resistance
  sits between the mouth and the chamber.
- **Embouchure:** pucker or tongue block (§6.1). Jazz chromatic players often pucker, with a tilted
  harmonica **[web, BHE]**.
- **Articulation:** a breath attack (the diaphragm, slow: 20-70 ms [rec]); a tongue attack (fast,
  5-15 ms); a slide articulation (a grace note through the slide, 30-40 ms cross-fade [rec]); tongue
  stops for staccato (*soon*'s 0.09-beat notes **[file]**); throat stops ("k") **[memory]**.
- **Changing between blow and draw within a phrase:** the mouth pressure must pass through zero. Every
  reed falls silent; the reverse reed starts from rest. **[rec] harmonica2.wav (chromatic, mapped by
  §2.1):**
  - blow↔draw changes give dips of **15-37 dB (median ~25)**, **60-120 ms** wide at −6 dB (F5 draw →
    G5 blow: 36 dB, 98 ms; G5 → F5: 27 dB, 120 ms; C5 blow → D5 draw: 37 dB, 108 ms);
  - same-breath hole changes give **~7-10 dB, ≤ 15 ms** (F5 draw → D5 draw: 9 dB, 13 ms), or none
    (D5 → F5 draw at 0.28 s);
  - same-direction re-attacks with the tongue give 13-23 dB.
  - No click is visible. The gap is the breath's zero crossing plus the new reed's onset.
- **Toots' jazz technique [memory, web; to check]:** 12-hole one maker (his Mellow Tone / Hard Bopper
  later); long legato lines with chromatic passing tones (the slide used as articulation), a light
  vibrato, scoops into notes, a soft, breathy, close-miked tone. "Waltz for Debby" in the user's
  files has 319 small downward bends **[file]**, which matches.

## 9. Recordings [rec]

Method: YIN pitch (4-5 ms hop, with harmonic-order checks against the spectrum, since H1 is often
10-25 dB below H3); 8192-point spectra at chosen notes; 5 ms RMS envelopes; band-limited modulation
analysis. Both files are 48 kHz stereo, analysed as mono.

| | harmonica.wav (21.2 s; sound 6.0-19.6 s) | harmonica2.wav (10.6 s; all sound) |
|---|---|---|
| Instrument | **Diatonic in C, 2nd position (G)** | **Chromatic in C** |
| Evidence | draw chords G4+B4(+D5) = holes 2-3-4; 2-3 st bend glides; 4-draw bend to C#5 | draw chord D4 F4 A4 B4 D5 (holes 1-5), A4+B4 together; slide cross-fades; G#4, G#5 |
| Range | D4-F5 (62-77), bent notes between | C4-G#5 (60-80) |
| Note time (s) by pitch | G4 3.5, D5 0.7, F#4 (bent G) 0.6, E4 0.4, A#4 (bent B) 0.3, D4 0.3, F5 0.25, F4 0.2, C#5 (bent D5) 0.2 | G4 1.7, G5 1.3, D5 1.0, D4 0.9, F5 0.6, F4 0.5, E5 0.3, A4 0.25, C5 0.2, G#4 0.1, C4 0.1 |
| Tuning | A ≈ 440-442 | ~+15 c: A ≈ 444 |
| Vibrato | throat, 6.0-7.4 Hz, 12-36 c, 2.4-3.2 dB, centroid ±100-360 Hz | tremolo on the last chord: 6.5 Hz, 7.2 dB, centroid ±130 Hz, little pitch; no vibrato on single notes (3-4 Hz wobbles of 8-65 c are expressive) |
| Bends | 2-3 st releases in 200-280 ms; held half-step bends | scoops −47 to −185 c into most notes, settling in 28-80 ms |
| Attack 10-90 % | 19-73 ms (breath) | 35 ms; mostly tongued note-to-note |
| Release | 120-210 ms | 57 ms |
| Gaps | 10-30 dB dips, 20-180 ms (many tongued) | blow/draw 15-37 dB, 60-120 ms; same breath ≤ 10 dB, ≤ 15 ms |
| Slide | – | in 1.5-2 dB dip, out 5-6 dB dip, ≤ 30-40 ms, both pitches present |

- **"Speech at the end of harmonica2.wav":** not found. From 8.9 s to the end the spectrum is the
  draw chord's partials (299, 352, 439-445, 492-498, 598, 891, 1488 Hz), steady and tremolo-modulated.
  There is no voice fundamental in 80-270 Hz (that band stays 20-35 dB below the total) and no moving
  formants. If the user hears speech, its time should be given; nothing was excluded.
- Demucs on five candidate tracks (Putumayo R&B 02, Ray Charles *Lonely Avenue*, Jeanie Bryson *Some
  Cats Know*, Joe Williams *Here's to Life*, Gerry Mulligan *Georgia*) found no harmonica. The "other"
  solos there are saxophone or brass.

## 10. Model plan [design]

### 10.1 Components and parameters

One `HarmonicaModel` (a `Tube` for the room hookup, as `AccordionModel`), at 96 kHz like the
accordion's reeds, with a half-band filter to 48 kHz.

1. **Lungs and breath:** p_L(t) (Pa, signed: + blow, − draw). Its target comes from the velocity and
   breath controllers. It rises over 20-70 ms (breath attack) or is held behind a closed tongue
   (tongued attack). It passes zero at a breath reversal, with a 60-120 ms gap.
2. **Tract (§4.4):** a Helmholtz mode. States u_t and p_m. Parameters: V_m 40 cm³; f_t and Q_t from
   the bend, scoop and throat vibrato; neutral f_t 300-700 Hz, Q_t 3. Bent: f_t = f_target(1 + δ),
   δ 3-6 %, Q_t 8-12. Throat vibrato: f_t modulated ±(1-3) % at 5.5-7.5 Hz together with p_L ±10-20 %.
3. **Lips, tongue and slide:** per hole h, an opening area A_h(t) =
   lipOpen_h(t) × (natural row: 1 − slideShut(x_s); sharp row: slideOpen(x_s)).
   - Resistance R_h = ρ|u|/(2 Cd² A_h²), linearised per block as `FreeReed`'s `pallet`, plus a small
     inertance folded into its duct.
   - lipOpen: 1 for the holes under the mouth, 0 for the others (or 0.05-0.3 leak for partly covered
     neighbours); tongue-blocked holes 0.
   - The slide position x_s ∈ [0, 1]: pushed in over τ_in 15-30 ms and returned by the spring over
     τ_out 10-25 ms, with s-shaped motion (finger on the way in, spring with a little bounce at the
     stop on the way out).
   - slideShut and slideOpen are ramps over an overlap window w ≈ 0.55-0.65 of the travel, so both
     rows are partly open in the middle (the [rec] dips of 1.5-6 dB).
4. **Chambers and reeds:** per hole and row, one chamber cell (0.3-0.4 cm³) with its blow reed and
   draw reed: `FreeReed` with `cell` the chamber, `duct` the slot and channel inertance, `pallet` R_h.
   - Reed builds from `accordionReed(hz)` with harmonica overrides: plate 1.05 mm (1.2 mm as the
     "Hard Bopper" option, or doubled in the bottom two octaves); width ≤ 2.8 mm; set 4 % of L; Q from
     the material (brass ~×1; stainless ×1.3).
   - The blow reed is driven by +(p_m − p_h), the draw reed by −(p_m − p_h) = (p_h − p_m).
   - Valves: on valved holes the non-speaking reed is not run (its slot is shut). On the top 4
     (unvalved) holes it is run as an opening reed in the same cell, which gives the slight leak and
     the secondary motion (BAB98, C11).
5. **Hands (§5):** one cavity, state p_h (C_h = V_h/ρc²) and u_o (the flow out of the opening,
   inertance M_o = ρ·1.7a/(πa²)). The opening radius a(t) runs from closed (3-6 mm) to open (≥ 30 mm,
   where the cavity is bypassed):

       C_h dp_h/dt = Σ_i s_i u_i − u_o
       M_o du_o/dt = p_h − R_o(a, f) u_o − p_room

   R_o is the radiation resistance, frequency-dependent (a one-pole fit of ρω²/(2πc) as in
   `bore.h`'s radiation loads), plus a hand-loss resistance setting Q 4-10. p_room is the room's
   pressure at the instrument (`Tube::setRoom`).
6. **Radiation:**
   - the source at 1 m is p = ρ/(4π) · du_o/dt, plus a direct leak through the fingers when cupped
     (~−20 dB) **[design]**;
   - open hands send more high-frequency power and project forward. In the present `Room` API
     (`Room::process`: one signal per source) this comes through the louder, brighter signal by
     itself;
   - a directivity term (the direct sound vs the reverberant send) needs `Room` to take a second
     per-source send. That is shared code, so §11 asks first.
7. **Hand phrasing and tremolo:** a(t) follows a target from the phrase logic:
   - closed while playing;
   - open on a phrase's last note (≥ 0.3 s to the next note): the opening starts 0-150 ms after the
     note-off or in its last 40 % and takes 100-250 ms;
   - on notes longer than 0.8 s, sometimes (p ≈ 0.3), open at 40-70 % of the note;
   - in rests > 0.25 s, open and close again 50-100 ms before the next onset;
   - every time randomised ±30 %.
   Tremolo is a(t) = ā + d·ā·sin(2πf_tr t + φ(t)): f_tr 5-7 Hz, its period wandering ±10-15 %
   (the measured CV 0.12), d from CC1 plus a habit (long notes > 0.7 s get d ≈ 0.3-0.5 after 0.3 s,
   in ballads and at slow tempi).

### 10.2 Equations, per sample at 96 kHz

    for each active reed i (one direction only, see the invariant):
        supply_i = s_i · (p_m − p_h)               # s_i = +1 blow reed, −1 draw reed
        y_i      = reed_i.sample(supply_i, R_h(i))   # FreeReed: cell, duct, tongue, gap (implicit)
        U       += s_i · reed_i.flow()               # flow out of the mouth (blow +, draw −)
    tract:  u_t += dt/M_t (p_L − p_m − R_t u_t);   p_m += dt/C_t (u_t − U)     (semi-implicit)
    hands:  p_h += dt/C_h (U − u_o);               u_o += dt/M_o (p_h − R_o u_o − p_room)
    out = ρ/(4π) · (u_o − u_o_prev)·fs

- The reeds use the previous p_m and p_h; their own cell and pallet already absorb the stiff part.
- The tract's time constants (~0.1-1 ms) and the hands' (> 0.1 ms) are long against 10 µs, so an
  explicit coupling is stable **[calc]**. Check it with a 4-reed chord at ff.
- When the hands are fully open (a ≥ 30 mm), the hand state is skipped: p_h = p_room and
  out = ρ/(4π)·dU/dt.

### 10.3 From MIDI to playing

- **Range:** fold notes below 48 up and notes above 98 down by octaves (what a player does). The
  user's files never need it (50-88) **[file]**. Option `PJ_HARP_HOLES=12` gives the 12-hole Toots
  model (60-98, folding below 60).
- **Note options:** each pitch maps to its list of (hole, direction, slide) from §2.1. C and F
  appear in both directions; C# and the doubled Cs appear on two holes.
- **Invariant: one breath direction at a time.** p_L has one sign, and only reeds of that direction
  are driven. A blow reed and a draw reed are never driven at the same time. The only exception is
  physical: on an unvalved hole the partner reed in the same chamber moves as an opening reed by
  leakage (§2.3), driven by the same Δp, never as a separate note.
- **Single line (one note held):** pick the option that minimises the cost
  `breath change (big) + slide change (medium) + |hole − current hole| (small)`, looking one note
  ahead. For example, C5 stays on draw as hole 8 + slide if the line is in draw, and goes to hole 8/9
  blow otherwise.
- **Chord algorithm** (whenever the held set changes):
  1. Fold every held note into range. For each, list its options.
  2. **Split** the held notes into B = those with a blow option (either slide state) and D = those
     with a draw option. Notes with both (C, F) go into both.
  3. **Direction:** the one whose notes have the higher **average velocity**. On a tie, keep the
     current direction. On a further tie, use the newest note's direction. The other side's notes do
     not sound (the invariant above).
  4. **Slide:** among the chosen direction's notes, the state (out or in) under which most of them
     are playable, then the larger summed velocity, then the current state (no move). Notes that need
     the other slide state do not sound. A held chord never mixes slide states: the slide is one bar.
  5. **Holes:** give each remaining note a hole (where it has two, e.g. C, F or C#, try both) so
     that the span max − min is smallest, preferring the window nearest the current mouth position.
  6. **Mouth window:** if the span is ≤ 4 holes, the lips cover it.
     - Interior holes without a note must be tongue-blocked (tongue-block mode for this chord). With
       the pucker mode chosen, they sound too, as a real pucker would.
     - If the span is > 4, keep the highest-velocity subset that fits 4 adjacent holes, or a split:
       the two outermost notes 3-4 holes apart with the middle blocked (octaves on holes i and i+3
       are a standard split).
     - Dropped notes stay "held" in the MIDI sense but silent.
  7. **A new note joining a held chord:** rerun the algorithm. If the direction changes, all
     sounding reeds stop through the breath reversal (60-120 ms gap) and the new set starts. If only
     the slide changes, every sounding hole moves through the slide's travel. If only holes change,
     the lips move (one hole: ~10-20 ms of a partly covered transition **[design]**).
  8. **Hysteresis:** a note that fits the current state is never moved; only a conflict re-decides.
     Releasing notes never forces a direction change while any held note still sounds.
- **Velocity:** the target p_L (e.g. 250 Pa at v = 1 to 3 kPa at v = 127 on GM's curve, as
  `midifile.cc` maps loudness) and the attack type: v > 90 tongued (5-10 ms), otherwise a breath
  attack 20-70 ms, shorter as v rises; the scoop probability and depth.
- **Pitch bend:** downward → the tract (§4.4). Upward: not physical on a chromatic, so ignored (or
  ≤ +20 c from more pressure; to ask).
- **CC1 (mod wheel):** hand-tremolo depth (§10.1.7). "Waltse For Dave" sends 61 CC1 events
  **[file]**.
- **CC2 / CC11 / channel pressure:** breath (p_L) as a held note's dynamics. **CC7:** volume.
- **Short notes** (*soon*'s 0.09 beats **[file]**): tongue articulation in one breath.
- **Phrases:** a new phrase after a rest > 0.3 s; the hands open at its end (§10.1.7).

### 10.4 Costs [calc]

- A `FreeReed` costs ~20-40 ns per sample (free_reeds §6). At 96 kHz that is 2-4 ms per second of
  sound, **~0.3 % of a core per reed**.
- Typical: 1 reed (melody), up to 4-5 (chords), +4 more during a slide move, +1 partner on unvalved
  top holes: **≤ 10 reeds, ≤ 3-4 % of a core**, usually ~0.5 %.
- Tract and hands: ~30 flops a sample. The half-band filter is as in the accordion.

### 10.5 Build order

1. The layout table, the single-line mapper and the breath invariant.
2. The tract and lips with one reed; bends.
3. The slide travel.
4. Chords: the algorithm and N reeds on one tract.
5. The hands: cavity, phrasing, tremolo.
6. Fit to the recordings: harmonica2.wav for the chromatic (scoops, gaps, slide, tremolo, spectrum);
   harmonica.wav for the throat vibrato and its spectrum.
7. Route GM 23 in `midifile.cc`, then play the four bushgrafts files.

### 10.6 Tests

- **Layout:** every MIDI 48-98 has ≥ 1 option; C and F have options in both directions; hole 16 draw
  + slide = D7; notes outside are folded.
- **Invariant:** fuzz random MIDI (chords mixing blow and draw notes, fast changes, slide moves). At
  every sample, no blow reed and draw reed are both driven, and p_L's sign matches the driven reeds.
  The partner reed on an unvalved hole is exempt but must stay ≥ 20 dB below its primary.
- **Chord algorithm:** C-E-G-C → blow holes 5-8 (or 1-4 a register down), slide out. D-F-A-B → draw,
  slide out. C(v 100) + D(v 40) → blow (and C needs no slide: blow hole 5). A chord spanning 6 holes →
  the 4-hole subset with the highest velocities. A new draw note joining a soft blow chord at higher
  velocity → a breath change with a 60-120 ms gap.
- **Chord loading:** 1 vs 4 reeds at the same p_L: total flow up roughly ×3-4; p_m down; each reed's
  Δp and amplitude down a little (a few %).
- **Bend moves every chord note:** with b = −1 st on a 3-note chord, each note goes down. The note
  nearest f_t moves most; none goes up.
- **Bend depth vs pitch-bend value:** single notes C4, G4, D5, G5. Deviation from the target ≤ 10 c
  down to −1 st; beyond, monotone with a level drop ≥ 3 dB at −2 st; the centroid falls with the bend.
- **Slide travel:** a slide press during G4 → G#4: the transition (10-90 % of the new note's partials)
  in 15-40 ms; both pitches present mid-way; the total level dip ≤ 3 dB going in and ≤ 6 dB coming
  back.
- **Breath reversal:** blow → draw between legato notes: a dip ≥ 15 dB lasting 50-120 ms. Same-breath
  hole changes ≤ 10 dB and ≤ 20 ms.
- **Hands:**
  - closed vs open on one note: open is ≥ 3 dB louder, its centroid ≥ 1.5× higher;
  - the transfer function from the reed flow to the output has its Helmholtz peak at the §5.1 table's
    f_h (±10 %), moving up with a;
  - in a phrase with rests, the hand opening's events fall at phrase ends and rests and never during
    a fast passage.
- **Tremolo:** CC1 at 64 on a held note. The level and the spectral centroid are modulated at the
  tremolo's rate (spectral peak at f_tr ± 0.3 Hz), in phase, the level by ≥ 3 dB p-p, the pitch by
  ≤ 5 c.
- **Throat vibrato:** pitch modulated 10-40 c p-p at 5.5-7.5 Hz with the level 2-4 dB p-p.
- **Spectrum vs [rec]:** a soft note's centroid ≤ 2 × f0 and a loud note's ≥ 3 × f0 (§7); harmonics to
  ~10 kHz at ff.
- **Real time:** a 4-note chord with a slide move and the hands under 4 % of a core; no NaN under
  fuzz (`resetAir` on NaN, as `Tube` does).

## 11. Open questions for the user

1. **Range:** 16-hole C3-D7 (recommended, all of the files' 50-88) with the top D7, or a C7 top?
   And should the 12-hole Toots model (C4-D7) be offered as an option?
2. **Tuning:** A = 440, or one maker's 442/443? harmonica2.wav sits at ~444.
3. **The room:** may `src/room.*` get a per-source reverberant send, so the hands' opening can change
   the direct-to-room balance (directivity), not only the level and tilt? It is shared code.
4. **Pitch bend up:** ignore it, or allow a few cents from pressure?
5. **Pucker or tongue blocking by default?** It decides whether interior holes of a chord window
   sound or are blocked, and whether slaps and pull-offs exist.
6. **Chord direction:** the average velocity as decided. Should a near-tie prefer the side with more
   notes?
7. **Diatonic later:** harmonica.wav is a C diatonic in 2nd position. Should a Richter mode (two
   unvalved reeds a chamber, deeper bends, overblows) follow as an option?
8. **Habits:** how much automatic hand opening and tremolo by default (none / light / as measured)?

## 12. Sources

- **[BAB98]** H. T. Bahnson, J. F. Antaki, Q. C. Beery, "Acoustical and physical dynamics of the
  diatonic harmonica", JASA 103(4):2134-2144 (1998).
  https://www.cs.bu.edu/fac/snyder/TablatureWeb/Bahnson%20JASA%201998.pdf (read in full)
- **[J87]** R. B. Johnston, "Pitch control in harmonica playing", Acoustics Australia 15(3):69-75 (1987).
  Not found online; read through BAB98 and C11.
- **[C11]** J. P. Cottingham, "Acoustics of free-reed instruments", Physics Today 64(3):44-48 (2011).
  https://aip.brightspotcdn.com/PTO.v64.i3.44_1.online.pdf (read in full)
- **[C17]** J. P. Cottingham, "Reed chamber resonances and reed tongue vibrational modes in free reed
  excitation", ISMA 2017 (abstract). https://isma2017.cirmmt.mcgill.ca/proceedings/pdf/ISMA_2017_paper_52.pdf
- **[N98]** J. P. Cottingham, "Theoretical and experimental investigation of the air-driven
  free reed" (Fletcher's admittance; opening vs closing behaviour). https://www.cs.bu.edu/fac/snyder/TablatureWeb/ICA.PDF
- **[MB07]** L. Millot, C. Baumann, "A proposal for a minimal model of free reeds", Acta Acustica united
  with Acustica 93:122 (2007); arXiv:2401.01606. https://arxiv.org/html/2401.01606v1
- **[CSW09]** J.-M. Chen, J. Smith, J. Wolfe, "Pitch bending and glissandi on the clarinet: roles of the
  vocal tract and partial tone hole closure", JASA 126(3):1511 (2009).
  https://www.phys.unsw.edu.au/jw/reprints/ChenetalJASA09.pdf
- **[HSW16]** N. Hanna, J. Smith, J. Wolfe, "Frequencies, bandwidths and magnitudes of vocal tract and
  surrounding tissue resonances, measured through the lips during phonation", JASA 139:2924 (2016).
  https://www.phys.unsw.edu.au/jw/reprints/HannaBandwidth.pdf
- **[A26]** S. Adachi et al., "Aerodynamic mechanism of accordion pitch bending: simulation and theoretical
  analysis", Forum Acusticum 2026. https://dael.euracoustics.org/confs/fa2026/data/articles/000418.pdf
- **[EMS21]** accordion pitch bending (see free_reeds.md).
  https://www.euracoustics.org/documents/14/AiP_2021_02_Accordion_acoustics.pdf
- **[VT15]** J. Granzow, T. Rossing, P. Egbert, "Bionic pitch benders for free reed instruments", Vienna
  Talk 2015 (abstract). https://viennatalk2015.mdw.ac.at/abstracts/ViennaTalk2015_submission_84.pdf
- **[MRI]** P. R. Egbert, L. K. Shin, D. Barrett, T. D. Rossing, A. Holbrook, "Real-time magnetic
  resonance imaging of the upper airways during harmonica pitch bends", ICA 2013 / JASA (PMID 23655957;
  abstract not read). https://ccrma405080.stanford.edu/events/peter-r-egbert-real-time-magnetic-resonance-imaging-of-vocal-tract-during-harmonica-pitch-ben
- **[BHM]** D. Barrett, "MRI bending process part 1: anterior and posterior chambers".
  https://www.bluesharmonica.com/mri_bending_process_part_1_anterior_and_posterior_chambers
- **[HPG11]** Hohner, *Pocket Guide Harmonicas* 2011 (models, plates, layout chart, the valves' effect).
  https://harmonica.ru/media/sites/2/posts/2021/11/Hohner-Pocket-Guide-Harmonicas-2011.pdf
- **[WCH]** Wikipedia, "Chromatic harmonica". https://en.wikipedia.org/wiki/Chromatic_harmonica
- **[BHC] [BH] [BHE]** bluesharmonica.com: "Chromatic Air" (W. Yerxa on the slide assembly, leakage),
  "Valves and chromatic", "Optimum embouchure for chromatic players".
  https://bluesharmonica.com/chromatic_air ; https://www.bluesharmonica.com/valves_and_chromatic ;
  https://www.bluesharmonica.com/optimum_embouchure_chromatic_harmonica_players
- **[WB]** Wikibooks, "Harmonica/Anatomy of a Harmonica". https://en.wikibooks.org/wiki/Harmonica/Anatomy_of_a_Harmonica
- **[ULg]** University of Liège poster on Toots Thielemans' chromatic technique.
  https://orbi.uliege.be/bitstream/2268/318588/1/Poster%20MIM%20-%20final.pdf
- **[T05]** T. Tonon, "Reed cavity design and resonance", PICA 2 (2005). https://concertina.org/?p=529
- **[forums]** harp-l and Modern Blues Harmonica on valved chromatic bending:
  https://harp-l.org/pipermail/harp-l/2015-May/msg00341.html ;
  https://www.modernbluesharmonica.com/board/board_topic/5560960/5442030.htm ;
  https://harp-l.org/pipermail/harp-l/2007-August/msg00650.html
- **[dealer]** Hohner 280/64 listings (C3-D7, 1.05 mm plates, zigzag slide):
  https://woodbrass.com/en-gb/chromatics-harmonicas-hohner-chromatic-280-64-chromonica-16-holes-c-do-p18953.html ;
  Toots models: https://www.eaglemusicshop.com/hohner-7539-48-toots-hard-bopper-harmonica ;
  Seydel slide action: https://thomann.de/ro/c_a._seydel_soehne_symphony_grand_chromatic_alu_c.htm
- **[PTclap] [Watts09]** cupped-hand Helmholtz clap: https://physicstoday.aip.org/backscatter/modeling-hand-clapping ;
  hand in a horn bell: https://courses.physics.illinois.edu/phys406/sp2017/Student_Projects/Spring09/Adam_Watts/A_Watts_Phys498POM_Spring09_Final_Report.pdf
- **[US5739446]** H. T. Bahnson, harmonica patent (slide thickness note via search).
  https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/5739446

## 13. Built (2026-11-16)

`src/harmonica.cc`, `src/harmonica.h`: §10's plan, with the user's answers to §11.
- **Answers:** A = 440 (as the other GM instruments). No second room send: the hands change only the
  level, the brightness and a forward projection. Upward bend: a few cents from a softer breath.
  Tongue blocking, the user's own technique. A near-tie goes to the side with more notes, and the
  lungs' fill leans the breath. No diatonic for now. All hand moves are automatic, scaled by the
  other parts' activity in C3-D7 in a MIDI file.
- **Changes from §10 found while fitting:**
  - The tract at rest is set low and broad (500 Hz, Q 1.2), and the cupped hands lossy (Q 3). At Q 3
    and Q 5 the reed nearest the resonance choked: C5 under the tract, E5 under a 6 mm hand opening.
  - A fixed bend tuning (f_t = 1.04 x the bent pitch, open loop) overshot by 1-2 semitones. The
    player now bends by ear: the lead reed's pitch moves the tract's tuning (a 30 ms loop), so bends
    land within a few cents down to -2 st at C4-G5.
  - The breath reversal's pause and the scoops scale with the time between notes. The recordings'
    60-120 ms gaps belong to a slow line; at 60 ms a note, blow/draw alternation must speak on
    every note, and does.
  - The slide's rows overlap over the whole travel (both open half-way): -3 dB into G#4.
  - The cupped opening is 12 mm (f_h ~1.1 kHz), not 6. A tighter cup made the top octave 10-15 dB
    quieter.
- **Not yet:** the partner reed's leakage on the unvalved top holes; slaps and tongue-block
  splits; the diatonic; throat vibrato habits (CC-driven vibrato only through the player's
  vibrato setting); fitting the spectrum in detail to `harmonica2.wav`.

