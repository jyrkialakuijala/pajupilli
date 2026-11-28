# The jazz kick's sound, its playing and its timing: targets for -inst jazz-kick

Status: 2026-11-28. A research companion to `kick.md` (the model as built, `src/kick.cc`). It covers what
a good jazz kick sounds like, the physics and the playing behind that, and how its timing is perceived.
Nothing here is built yet. Marks: **[paper]** a published measurement; **[web, site]** a web source
(teachers, makers, retailers, forums: opinion, unless it says it measured something); **[forum]** one
person's post; **[calc]** worked out here; **[memory]** general knowledge, to be checked; **[code]** read in
this repo. The user's starting summary was treated as leads to check. §9 says what held up.

## 1. What a good jazz kick sounds like

- **A tom, not a thump.** In jazz the ride keeps the time, so the kick and snare are accent voices. The kick
  is "treated more like a large floor tom" and blends tonally with the toms, unported, with a single-ply
  head **[web, sweetwater recording-jazz-drums pt 2 (search summary; the page refused the fetch)]**. Its
  pitch is "high and open" **[web, Modern Drummer education team]**. It is meant to "sing"
  **[web, sweetwater how-to-tune]**.
- **It blends with the double bass.** The feathered pulse "blends with the acoustic bass"
  **[web, Fidyk, Modern Drummer 2012]**. It is felt more than heard, and never louder than the bass
  **[web, Jazz Queensland; The Instrumentalist; Marshall Univ. rhythm-section handout, via search]**. Avoid
  "too low and overpowering" **[web, onjazzdrumming]**.
- **Softer and less clicky than rock.** Live-sound engineers want a jazz kick "softer and less clicky"
  than a rock or funk one, and they reach for an RE20 or MD421 rather than a scooped "click" mic
  **[web, gearspace "Live Jazz – Kick Drum" (search summary; the page refused the fetch)]**.
- **Records put it lower in the mix than it is in the room.** An engineers' thread asks why the "HUGE fat"
  kick heard at live jazz gigs is rarely on records **[forum, homerecording.com]**. Fletcher and Bassett
  noted that a bass drum on tape never sounds like the drum, and that the sound changes with where the
  listener stands **[paper, FB]**. So stems give a lower bound on the kick's level, not its target.

## 2. The drum: sizes, heads, damping

### 2.1 Sizes, as played (who used what)

| Player / era | Kick | Source |
|---|---|---|
| Swing-era kits | 24–26" marching-band bass drums | [paper, Scott & Morrison, Acoustics Today 2022] |
| Max Roach, Roy Haynes (bebop) | moved down to **20"** | [paper, Scott & Morrison] |
| Elvin Jones | **20"** at first, then **18"** (the 20" would not fit in the car's trunk) | [web, cruiseshipdrummer quoting MD 1982] |
| Elvin Jones | an 18" tuned "very highly pitched", "basic to [his] sound": he uses the kick a great deal | [web, thesis snippet via search; unverified] |
| Philly Joe Jones | **22"**, with 13" and 16" toms; tuned higher than Blakey's drums | [web, cruiseshipdrummer key players; jazzprofiles] |
| Art Blakey | "a big drum sound" | [web, jazzprofiles] |
| 1960s Gretsch endorsers (Mel Lewis, Roach, Blakey, Elvin, Tony Williams) | mostly 20"; the 18" grew common later | [web, search summary of jazzprofiles; partly unverified] |
| Bebop kit, today's catalogue | 18" × 14" with 12" and 14" toms | [web, jazzfuel; drumeo] |
| Big band | 22" or larger | [web, drumeo] |

- So **"jazz = 18"" is a later convention.** The classic records are as often 20" (Roach, Haynes, Blakey,
  Tony Williams) or 22" (Philly Joe). 16" kits exist for higher tunings **[web, bax-shop; buyers' guides]**.
- **Depth:** 14" is the bop standard **[web, catalogues]**. The claim that "a shallow depth gives a higher,
  singing pitch" is only partly physical. The note is set by the heads and the air spring between them
  (§3). A shallower drum stiffens the air, which raises the out-of-phase partner much more than the note
  **[calc, §3]**.

### 2.2 Heads

- **Batter:** single-ply **coated**, about 10 mil, "warmth, openness and dynamic response". Coated heads
  absorb some of the highs, which gives a warmer tone with smoother overtones and a softer attack than a
  clear head **[web, D'Addario/Evans]**. Clear heads give a sharper attack and a longer ring
  **[web, sweetwater]**. Calftone and Fiberskyn are for a vintage feel; their kick sound is not documented
  **[web, D'Addario; search]**.
- **Resonant:** single-ply, **unported** (a hole controls sustain and helps miking; unported gives "a
  fuller, more resonant tone") **[web, D'Addario]**. "Jazz drummers generally do not opt for a hole or any
  dampening" **[web, jazzfuel]**.

### 2.3 Damping, as players do it

| Damping | Who | Source |
|---|---|---|
| none | the usual bop kick | [web, jazzfuel; drum-tuning.com; sweetwater] |
| a 3" felt strip under the batter | Joe Morello, Buddy Rich | [web, Fidyk MD 2012] |
| torn newspaper inside the shell | Dave Tough | [web, Fidyk] |
| a paper napkin taped at the batter's edge | Mel Lewis | [web, Fidyk] |
| a towel between the pedal and the batter | a teacher's advice "to control overtones" | [web, onjazzdrumming] |
| a small pillow lightly touching both heads | a tuning site's "small amount" | [web, drum-tuning.com] |

- **No source measured what these do in numbers.** Engineers say a pillow on the resonant head takes off
  some of the note and adds attack "without greatly reducing the sustain" **[web, Vintage King]**. A
  damped rock kick's note "sustains for 300 ms" **[web, Sonnox; an engineer's rule of thumb]**.

## 3. Pitch and tuning

### 3.1 Tuning guides (Hz)

| Source | 18" | 20" | 22" | Notes |
|---|---|---|---|---|
| drum-tuning.com, fundamental: low / medium / high | 60 / 70 / 80 | 55 / 65 / 75 | 50 / 60 / 70 | jazz: "high, minimal or no dampening": 75–80 Hz on an 18–20" |
| drum-tuning.com, batter / front head at "high" | 108 / 116 | 101 / 109 | | front 6–8 Hz above the batter ("3–15 Hz" in its text) |
| tune-bot, lug pitches for a "typical" kick (not jazz) | batter lug 71, reso lug 107 (note G1) | batter 64–68, reso 96–101 | batter 58–61 | reso lug a fifth (1.5×) above the batter as a starting point |

- **Bop players by note** (one forum, several posters): a "default bop" 18" has its batter at D and
  "fundamental F"; another poster's 18" is at **C♯2 (69 Hz)**; another treats the kick as an 18" tom, in
  even intervals 12"(C3)–14"(G2)–18"(D2, 73 Hz). Joey Baron often tunes an 18" very low
  **[forum, drumforum.org, via search; the pages are paywalled to bots]**. Read in octave 2, F is 87 Hz.
- **So a bop 18" sits at about 69–87 Hz (C♯2–F2), a 20" at about 62–75 Hz** **[web + forum; a range, not
  a measurement]**. That is a fourth to a sixth above a rock 22" (50–60 Hz).
- **Relative to the bass's notes:** no source states a rule (such as "tune away from the key"). The
  advice is to blend, to keep the kick quieter than the bass, and to tune high enough to "cut through"
  without overpowering **[web, several]**. A kick at 70–87 Hz lies inside the upright's walking range
  (E1 41 Hz to about G2 98 Hz). Its in-phase note is a clear pitch, so it **will** sometimes clash with the
  bass's notes. Players handle that by level (feathering under the bass) and by short notes, not by
  tuning **[calc + memory]**.

### 3.2 Batter against resonant head

- **Kit sources:** the resonant head **a whole tone above** the batter **[web, sweetwater]**; the same
  pitch up to a minor third **[web, sweetwater]**; the same pitch "to maximise sustain"
  **[web, Yamaha hub]**; match the batter, then half a turn more per lug **[web, Fidyk, MD]**; a few Hz
  above **[web, drum-tuning.com]**; lug pitch a fifth above **[web, tune-bot]**.
  - So the majority view for a jazz kick is **the resonant head slightly higher, by 0–2 semitones**.
- **Acoustics:**
  - Most concert drummers tune the **batter higher** than the carry head, by up to 75 % in tension (a
    fourth). Levine wants the carry head lower for solo and small-ensemble playing **[paper, PMI §18.9]**.
  - **Equal tension maximises the coupling and decays faster:** 6–11 dB/s against 3–9 dB/s with the carry
    head lower. With the carry head lower, the partials at 70–300 Hz are stronger at first and decay
    faster **[paper, PMI §18.9, after Rossing 1987]**.
- **Mode pair (82 cm concert drum, equal tension):** the (0,1) pair at **44 and 104 Hz**, a ratio of
  **2.36**; the (1,1) pair at 76 and 82 Hz. With the carry head lower: 39, 80, 121, 162, 204, 248 Hz, a
  nearly harmonic series. The fitted coupling frequency is f_c = 67 Hz; the piston model gives 89 Hz
  **[paper, PMI Table 18.5]**.
- **Fletcher–Bassett 36" drum:** 40.0 Hz, then 61.9, 84.4, 95.8, 110, 131 Hz, … (the (1,1) at 1.55× the
  note) **[paper, FB Table II]**.

### 3.3 The out-of-phase partner, scaled to a kick [calc]

- In the two-mass model f₋² = f₀² + 2 f_c². In the piston estimate f_c² ∝ ρc² / (σ d): the drum's depth
  and the film's mass per area set it, nearly independently of its diameter.
- For an 18" × 14" with σ = 0.26 kg/m², the piston f_c is about 139 Hz. Scaled by PMI's
  measured-to-piston ratio (67/89), that is about **105 Hz**.
- With the head alone at 86 Hz, **f₋ ≈ 170 Hz**, about **2.4×** the note. That is PMI's ratio.
- **The model's partner is at 248 Hz (3.5×; `kick.md` §2).** Its k_air comes out at 3.5× the head's own
  stiffness, which implies f_c ≈ 160 Hz. That is about 1.5× this estimate. PMI's own lesson is that the
  piston model overestimates f_c (89 against 67 Hz measured). The model's air coupling may be too stiff, or
  its effective area too large; see §9.

### 3.4 Pitch glide

- **A full blow on a concert drum:** about 6 mm of initial amplitude, **+10 % (nearly a whole tone)**,
  falling within the first 1–2 s **[paper, PMI; Cahoon 1970; FB]**.
- **Toms:** a 33 cm tom is +8 % for the first 0.2 s; a 32 cm one +160 c. The glide shrinks as the tension
  rises **[paper, PMI §18.11]**. A high jazz tuning therefore glides less than a rock kick does.
- **The model:** +80 c on an 18" bomb and +155 c on a 20", 0 when feathered. That lies within these
  bounds; for a bop 18" tuned higher it should be at the low end.

## 4. Decay (sustain)

### 4.1 Measured decay rates

| Drum | Mode / band | Decay | T60 | ζ = 1.1 / (f T60) | Source |
|---|---|---|---|---|---|
| 36" concert, hard centre blow | 40 Hz note | 4 dB/s | 15 s | 0.0018 | [paper, FB Table II] |
| same | 62 Hz (1,1) | 7 dB/s | 8.6 s | 0.0021 | [paper, FB] |
| same | 84–250 Hz | 10–64 dB/s | 1–6 s | | [paper, FB] |
| same | 300–850 Hz | 11–96 dB/s | 0.6–5 s | | [paper, FB] |
| 82 cm concert, carry head lower | low modes | 3–9 dB/s | 6.7–20 s | | [paper, PMI] |
| 82 cm, equal tension | low modes | 6–11 dB/s | 5.5–10 s | 0.0025–0.0045 at 44 Hz | [paper, PMI] |
| 32 cm two-headed tom, five head types | upper (0,1), ~190 Hz | | 0.8–1.05 s | 0.0055–0.0072 | [paper, PMI Table 18.8, Bork] |
| same | ⅓ octave at 500 Hz | | 1.1–2.7 s (1.1–1.4 for damped heads) | | [paper, PMI] |
| a tom on a different mounting arm | (0,1) | | 5.5 s down to 0.6 s | | [paper, PMI, Bork 1983] |
| a damped rock kick (an engineer's figure) | the note | | ~0.3 s "sustain" | | [web, Sonnox] |

- **Lab drums keep their low modes for 150–450 cycles [calc].** A jazz kick's 70 Hz note at that loss
  would have **T60 ≈ 2–6 s**. Taking the tom's per-cycle loss (similar heads, ~170 cycles): about 2.4 s.
- **The model's note:** T60 0.83 s on the 18" (ζ₀₁ = 0.018, about 58 cycles). That is **3–7× lossier** than
  any lab drum above. `kick.md` §4 marked its own figure "[memory, to be fitted]".
- **Fletcher and Bassett:** "decay rates seem to depend more on the characteristics of the drum than on
  how or where the drum is struck" **[paper, FB conclusion 4]**.
- **The room matters.** At these wavelengths decay times depend strongly on the acoustical surroundings
  **[paper, PMI]**. The drum on a carpet, the drummer's leg on the pedal and the felt resting near the head
  all take off energy.
- **Targets [calc + design, to be fitted by ear].**
  - An open, unported, unmuffled bop 18": the note's T60 **1.5–3 s**, so audible for about 1–2 s after a
    bomb. The 150–400 Hz partials' T60 0.5–1.5 s.
  - With a felt strip: about 1 s. With a pillow touching a head: 0.3–0.5 s. A ported, padded rock kick:
    about 0.3 s.
  - This matches the project's rule for cymbals (keep the long jazz tails; don't shorten to a textbook
    value): the open kick should ring longer than the model's 0.83 s.
- **Buried beater:** no measured figure was found. The model's 14 dB under the open stroke at 0.2 s is
  plausible. Engineers prefer the buried sound on records **[web, cruiseshipdrummer]**.

## 5. Spectrum: note, overtones, click

### 5.1 Relative peak levels

Fletcher and Bassett measured the peak level of each component in a 3 Hz band. Relative to the note:

| Band (× the note) | Hard blow at the centre | Hard blow near the edge |
|---|---|---|
| note (40 Hz) | 0 dB | 0 dB |
| 1.5–3.3× (62–131 Hz) | −4 to −14 dB | −4 to −16 dB |
| 4–6× (164–250 Hz) | −10 to −18 dB | −4 to −20 dB |
| 7–10× (270–400 Hz) | −20 to −30 dB | (not read) |
| 13–21× (540–850 Hz) | **−27 to −46 dB** | **−8 to −22 dB** |

- So **an edge strike adds about 15–20 dB in the 500–650 Hz partials** **[paper, FB Tables II–III]**.
  Off-centre is "ringier" (more partials), as the user's summary says. It is **not** "rounder". Rounder
  belongs to the centre strike, where the (0,n) modes dominate **[paper, PMI §18.12]**.
- **Ear weighting:** above 200 Hz the many inharmonic partials sound louder than their levels, because the
  ear discriminates against the lows. The low cluster (01, 11, 21, 31, 41) is nearly harmonic on a concert
  drum **[paper, PMI]**.

### 5.2 Click

- **The band:** a kick's click lies in about **1–4 kHz** **[web, Native Instruments EQ guide]**. It is
  "hundreds of high partials lasting only a few ms" **[web, Sound On Sound]**.
- **Beaters:** felt gives a "subdued attack with emphasis on the low end". Plastic and wood are bright,
  with more "top-end attack" **[web, Pearl]**. Harder and smaller beaters give more definition and punch
  **[web, Thomann]**. No measured spectra were found.
- **[calc]:** a felt contact of 2–6 ms is a half-sine force whose spectrum falls steeply above about
  1/(contact time), i.e. 170–500 Hz. A coated head and a felt beater therefore put the click 30–40 dB under
  the note at 2 kHz. A wood beater on a clear head gives 1 ms or less: the click rises by 10–20 dB.

### 5.3 Miking

- Jazz kicks are miked from outside the resonant head, often with one dynamic mic (RE20, MD421, M88) and
  no scooped EQ **[web, gearspace via search]**.
- The model's two mics 0.6 m in front of the resonant head fit this.

## 6. Dynamics

- **Feathering:** pp–mp **[web, Fidyk MD]**; "very softly" (Kenny Clarke, quoted) **[web, cruiseshipdrummer]**.
  Never "heard as a separate rhythm riding above the rest of the kit" **[web, search summary]**, and quieter
  than the bass **[web, Jazz Queensland; The Instrumentalist]**.
- **Bombs:** f–ff accents, often with the crash or a snare stroke **[web, Wikipedia "Dropping bombs"; MD]**.
  The concert drum's hard blows peaked at 112–121 dB SPL at 1 m **[paper, FB]**; a kit kick's bomb at
  1 m is likely 105–115 dB **[memory]**.
- **Range:** this is 30–40 dB from feathering to bombs **[calc from pp–ff]**. The model's 29.6–32.6 dB from
  velocity 16 to 127 (`kick.md` §3) is about right.
- **The bushgrafts files' velocities:** a median of 44, 10–90 % from 22 to 71 **[meas, kick.md]**, i.e.
  mostly feathering.
- **Target [design]:** a feathered stroke 8–15 dB (A-weighted) under the walking bass's notes at the
  listener, so that it is felt (it adds low weight to the bass's attack) but not heard as its own line.

## 7. Expression: what jazz drummers do with the kick

### 7.1 Technique

- **Heel down for feathering:** "a round tone and a consistent swing pulse", the foot resting on the
  pedal **[web, Fidyk MD]**. Feathering is usually heel down with the beater **buried** **[web, search
  summary of cruiseshipdrummer]**. Burying is contested, though many studio engineers prefer it on
  records.
- **Heel up for bombs [memory]:** the leg's weight; the beater thrown and let rebound, so that the drum
  rings.
- **So burying goes with soft playing and rebound with loud.** The model buries only on CC 64 or
  `PJ_KICK_BURY`. It never chooses by dynamics.

### 7.2 The patterns

- **Kenny Clarke** moved the time to the ride, played the kick "very softly", and used it for
  irregular, syncopated accents: the **bombs**. They were "clave-based" (Billy Hart)
  **[web, NPR; Britannica; Wikipedia; cruiseshipdrummer on Hart]**. As the string bass's role grew, he
  wanted the kick out of its way **[web, NPR 2014]**.
- **Feathering by tempo:**
  - Jeff Hamilton feathers all four beats in swing and drops to **1 and 3 at very fast tempos**. Max Roach
    audibly played 1 and 3 at his fastest **[web, McCaslin blog]**.
  - Vernel Fournier feathered "on every tune for a few choruses until it was locked in". Some teach 2 and
    4 on the kick at up-tempos **[web, McCaslin comments]**.
  - Todd Bishop rarely feathers a whole tune: 1 and 3 at slower tempos, or for weight at moments
    **[web, cruiseshipdrummer]**.
  - Billy Hart calls feathering "very important" for depth and mood, but says it needs "syncopation or
    clave" in it, not a steady pulse **[web, cruiseshipdrummer, Hart's autobiography]**.
- **Two-feel against four-feel:**
  - When the bass plays in four, the kick feathers four, felt not heard. When the bass is in two, the kick
    plays four **[web, The Instrumentalist via search]**: the kick fills the beats the bass leaves.
  - McCaslin: a two-beat kick on 1 and 3 with the hi-hat on 2 and 4 "can be made to feel like 4/4"
    **[web]**.
- **Accents by the bass:** feather the beats, and accent the bass's notes that fall on the "&"s
  **[web, search summary of cruiseshipdrummer]**.
- **Set-ups (Philly Joe Jones):**
  - Isolated kicks on the **"and" of 2** or the **"and" of 3**, resolving to 4 or to 1 with the cymbal.
    These are "easy spots to rush" **[web, cruiseshipdrummer, "Blue Roz", *Bags Meets Wes*]**.
  - His set-ups introduce sections. He used the kick sparingly and with weight, sometimes alternating bars
    of snare with bars of kick in solos **[web, cruiseshipdrummer]**. A bandmate said he could play big
    band lead-ins "without rushing" **[web, jazzprofiles]**.
- **Big-band figures [memory]:**
  - Short ensemble hits are caught with the kick (often with the crash) on the hit itself.
  - A set-up is a snare or kick stroke an eighth or a beat before the hit.
  - Long figures: the kick on the attack, the cymbal for the length.
- **Kick-snare comping [memory, web MD]:** the "third hand". The kick takes one note of a snare figure
  (snare–kick, or kick–snare on "and of 4 → 1"), as an answer to the soloist's phrase.
- **Latin and bossa [memory]:**
  - Bossa nova: the kick on **1 and 3, each with a pickup on the "and" before** (dotted quarter–eighth,
    from the surdo), soft and steady.
  - Afro-Cuban jazz: the kick on the "and" of 2 and on 4 (the tumbao's bombo and ponche), or on 1 and 3.
  - Samba: 1 and the "and" of 2, the second louder.
  - These are steady, so they are the exception to "sparse".
- **The form [memory, design]:**
  - **Head:** lighter; set up the melody's breaks.
  - **Solos:** comp, rising through each chorus.
  - **Bass solo:** brushes or a soft hat, the kick silent or barely there.
  - **Trading fours:** the kick is a solo voice.
  - **Out-chorus:** more bombs, kick and crash on the shout figures.
  - **Ending:** kick, snare and crash on the last chord (already in the code).
- **When not to play it:**
  - Under the bass solo. On ballads with brushes the feathering is optional or very soft, though some
    teach it as part of brush technique **[web, search]**.
  - Not four-on-the-floor through a whole tune **[web, Palen Music]**.
  - Not at a level that competes with the bass, and not as a rock-style crutch (Bishop) **[web]**.
  - Not on every downbeat the bass plays: Clarke's point.

## 8. Listening: whom the kick answers

- **The bassist first.**
  - Hart: the ride and kick relate to "the ensemble's bass line"; a drummer who doesn't grasp that is
    "driving a car out of alignment" **[web, Hart]**.
  - Christian McBride wants a drummer who listens; NPR describes the bass and drums as one instrument
    **[web, NPR]**.
  - Cues to read:
    - Walking (four) or a two-feel: feather in four, or fill to four.
    - The bass's syncopations (accent with them, or leave them alone).
    - A bass solo: drop out.
    - Its register: when the bass sits around the kick's pitch, play softer or shorter.
- **The soloist:**
  - Comping fills the soloist's gaps, never their notes, and catches their accents at phrase ends with a
    bomb or a kick–snare answer **[memory; Scott & Morrison on comping complementing the melody]**.
  - Cues: rests over half a bar in the lead line, the lead's accents, its peaks (the highest or loudest
    notes of a phrase), and the end of a chorus.
- **Piano or guitar:** comping is a conversation. The drummer avoids doubling every piano stab, and
  answers or fills around the comp's rhythm **[memory]**. Cue: the chords' onsets off the beat.
- **The horns' hits (big band):** catch and set up the ensemble figures (§7.2). Cue: notes struck together
  by several horn channels at once, especially short and loud ones, and off-beat ones.
- **The form:** the kick density and level by section (§7.2). Cues: chorus boundaries (32 or 12 bars), the
  director's reading of the song's sections, the density of the other parts.

### 8.1 What the code already reads [code]

- `src/director.cc` already finds whether the song is **jazz** (swing ratio, ride against hat, a walking
  bass, a band's programs), **which channel is the bass**, and whether it **walks** (≥ 50 % of steps a
  beat). It records the drum strokes too.
- `src/midifile.cc` (≈ l. 711) has a **bandVel(t0, t1)** "the drummer listens" helper: the pitched parts'
  mean velocity around a moment, used for the brush swirls. The jazz ending (≈ l. 774–816) adds a snare
  triplet set-up and kick + snare + crash on the final note.
- **The kick itself plays the file's notes as written:**
  - 35 is struck at 0.22 R and 36 at 0.12 R, ±0.015 R and ±2 % speed per stroke.
  - It buries only under CC 64 or `PJ_KICK_BURY`.
  - Its velocity goes through drumcal's map (`drumVelocity`).
  - Nothing yet feathers, drops, adds or re-levels kick notes by what the band does.

## 9. Timing and perception

### 9.1 Perceptual onset (P-centre)

**Danielsen et al. 2019** measured the P-centre of real sounds by click alignment and by tapping, N = 20.
P-centre means relative to the physical onset **[paper, Danielsen et al., Table A1]**:

| Sound | Rise / duration / centre frequency | Click alignment | Tapping |
|---|---|---|---|
| click | 1 ms | −0.5 ms | −21.6 ms |
| **kick drum** | 5 ms / 130 ms / 65 Hz | **+2.1 ms** | **−4.2 ms** |
| snare | 2 ms / 25 ms | +1.6 | −0.3 |
| dark piano (low, long) | 18 ms / 487 ms / 65 Hz | +9.0 | +8.1 |
| light piano (high, long) | 19 ms / 318 ms / 659 Hz | +3.9 | −1.2 |
| arco bass (slow, short, low) | 31 ms / 66 ms / 65 Hz | +12.0 | −0.5 |
| synth bass (slow, long, low) | 33 ms / 220 ms / 33 Hz | +23.4 | +16.9 |
| fiddle (slow, long, high) | 43 ms / 105 ms | +12.8 | +9.8 |

- **Slow attack, long duration and low frequency each make the P-centre later.** The frequency effect
  appears only for long sounds, where low is about **+5 ms** (pianos) to **+10 ms** (the synth bass
  against the fiddle).
- **A short, fast kick's P-centre is at its onset, within 1 ms of the snare's.**
- Low frequencies are slower in the cochlea **[paper, Danielsen, citing Wojtczak 2012]**.
- **Asymmetric tolerance:** simultaneity is judged more tolerantly when the high sound leads the low one
  than the reverse **[paper, Wojtczak, Mehta & Oxenham, PNAS 2017]**. **A late kick is forgiven more
  easily than an early one.**
- **Low tones carry timing:** the brain's timing-error response (MMN) is larger for a low tone 50 ms early
  than for a high one. Tappers move about 11 ms when a low tone comes 50 ms early, and less for a high one
  **[paper, Hove et al., PNAS 2014]**. Timing errors in the kick are **more** noticeable, not less.
- **Asynchrony is perceived as "one event" below about 20 ms.** Listeners preferred ensemble asynchronies
  under 19 ms in a jazz trio; discrepancies up to 20 ms were irrelevant to groove, over 30 ms harmful
  **[paper, Hofmann, Wesolowski & Goebl 2017]**.

### 9.2 What players actually do (measured)

- **Jazz trio (Hofmann et al. 2017):**
  - The ride against the bass: **+2.1 ms** on average, "almost perfect synchrony" (per drummer +5.3, +1.4,
    −0.1 ms).
  - The **hi-hat runs 2–26 ms ahead of the pulse**, and the bassists are laid back 7–26 ms against the
    hat. The bassists align to the cymbal.
  - The kick's own timing was not reported.
- **Swing duo (Kilchenmann & Senn 2015):** against the grid, the bass is **−17.5 ms** (ahead), the ride
  **−14 ms**, the snare 0 ms. No kick in their swing pattern.
- **Funk, same study:** the **kick at −12.5 ms**, ahead of the hat, snare and bass (about −3 to −4 ms), with
  an SD of 10.5 ms **[paper, Kilchenmann & Senn, PMC4542135]**. A drummer's foot sounding early is
  measured, if in funk.
- **Friberg & Sundström 2002:** soloists delayed on the downbeats against the drummer and in time on the
  upbeats, at medium tempi. Drummers' swing ratio is above 2, soloists' below **[paper]**. A secondary
  source puts their bass–ride discrepancy at under 20 ms **[web, Hurst & Peacock via search]**.
- **Keil 1987, Prögler 1995 ("participatory discrepancies")**, bass against ride: later controlled
  studies (Butterfield 2010; Frühauf 2013; Senn 2016) found no gain in groove from such discrepancies. The
  listeners preferred them *smaller* **[paper, as summarised by Hofmann et al.]**. Butterfield's
  "pushing"/"laying back" numbers and Collier & Collier's were not reachable here.
- **Rasch:** ensemble asynchronies in the tens of ms, falling with tempo. In a string trio the *high* voice
  led (violin 5–10 ms before cello) **[paper, via Hove et al.]**: no rule that low instruments play
  ahead.
- **Stage travel:** 2.9 ms per metre. The bassist 1.5–3 m from the kick hears it 4–9 ms late. The drummer
  hears his kick from about 0.8 m and his ride from about 0.6 m: under 1 ms apart **[calc]**. In the
  room, all parts arrive together if they are about equally far away.

### 9.3 Is a constant early kick justified? [calc + design]

- **No constant 10 ms.** A short, hard, clicky kick has its P-centre at its onset (+2 ms, like the
  snare). Moving it early would be heard as early, the low-lagging asymmetry being against it.
- **A small, attack-dependent lead is justified:**
  - A feathered stroke has a slow attack (soft felt, a long contact, no click) and a long, low ring: a
    "slow/long/low" sound whose P-centre is later.
  - Interpolating Danielsen's table between the kick (+2 ms) and the dark piano (+9 ms), the lead should
    be lead ≈ max(0, P(stroke) − P(snare)), where P is the stroke's own perceptual onset.
  - For the model, about **0 ms for bombs and 3–8 ms for feathered strokes**: growing as the contact gets
    longer and the level falls, and as the click (energy above 1 kHz) gets weaker.
- **Measure it, rather than guess.** Take the model's strokes at velocities 20…127. Find the time at which
  a P-centre estimator crosses its threshold: for example the 50 %-of-peak rise of a 20 ms-smoothed,
  loudness-weighted envelope (as in Danielsen's MIR measures), against the snare's. Plan each note that much
  early.
  - The precedent: `midifile.cc` (≈ l. 2133) already plays an organ's slow-speaking bass pipes ahead, by
    part of their speech time (≤ 60 ms).
- **The real drummer's habit:** the funk data show the foot ahead by about 10 ms. The jazz data show the
  cymbals and hat ahead, with the bass laid back against them. A feathered kick that locks with the bass
  therefore sits *with the bass* (not ahead of the ride). Locking to the bass's onsets matters more than
  any absolute offset.
- **Recommendation:** an onset compensation of 0–8 ms from the stroke's own attack, plus the existing
  per-stroke scatter, made small (SD ≈ 5 ms, under the 19 ms "one event" limit). A feathered kick is
  anchored to the bass's note onsets when they are within ±25 ms of the kick's written time.

## 10. Checking the user's summary

| Claim | Verdict |
|---|---|
| round, open, melodic; softer attack, longer sustain than rock | held up: coated head, felt beater, unported, no muffling **[web, several]** |
| 18"/20", sometimes 16" | held up, but the classic records are often 20" or 22" (§2.1) |
| shallow depths give a higher, singing pitch | mostly not: depth moves the out-of-phase partner far more than the note (§3.3) |
| both heads unmuffled or lightly damped | held up (§2.3) |
| tuned higher than rock, clear pitch | held up: ~69–87 Hz on an 18" against 50–60 Hz on a rock 22" (§3.1) |
| coated heads for warmth and a softer click | held up **[web, D'Addario]** |
| feathering and bombs | held up; feathering is optional and style-dependent (§7.2) |
| beater rebounds rather than buried, to keep the drum open | partly: bombs rebound, feathering is often buried (§7.1) |
| off-centre is rounder, ringier, less click | **ringier yes, rounder no**: an edge blow has +15–20 dB at 500–650 Hz (§5.1) |
| natural EQ; a dynamic mic plus a condenser further back | dynamic outside the reso head, yes; the condenser was not confirmed |

## 11. Sources

- **[FB]** H. Fletcher, I. G. Bassett, "Some experiments with the bass drum", JASA 64 (1978) 1570:
  https://physics.byu.edu/docs/publication/1492 (Tables II–III read here).
- **[PMI]** N. H. Fletcher, T. D. Rossing, *The Physics of Musical Instruments*, 2nd ed., §18.9 (bass
  drums, Table 18.5, after Rossing 1987), §18.11 (tom-toms, Bork 1983), §18.12 (Table 18.8, decay):
  https://csclub.uwaterloo.ca/~pbarfuss/The_Physics_of_Musical_Instruments.pdf
- **[Scott & Morrison]** E. K. E. Scott, A. Morrison, "The Acoustics of the Modern Jazz Drum Kit",
  Acoustics Today, Winter 2022: https://acousticstoday.org/wp-content/uploads/2022/11/AT-WINTER-2022-Final.pdf
- **[Danielsen]** A. Danielsen, K. Nymoen, E. Anderson, G. S. Câmara, M. T. Langerød, M. R. Thompson,
  J. London, "Where is the beat in that note? Effects of attack, duration, and frequency on the perceived
  timing of musical and quasi-musical sounds", JEP:HPP 45 (2019), accepted copy:
  https://jyx.jyu.fi/handle/123456789/63979
- **[Hofmann]** A. Hofmann, B. C. Wesolowski, W. Goebl, "The Tight-interlocked Rhythm Section", JNMR 2017:
  https://pub.mdw.ac.at/media/content_files/052f88157e7331f49053c1467aa41c3d08523c4b11f1ea33322be509dd9b4ca6.pdf
- **[Kilchenmann & Senn]** "Microtiming in Swing and Funk affects the body movement behavior of music
  expert listeners", Front. Psychol. 2015: https://pmc.ncbi.nlm.nih.gov/articles/PMC4542135/
- **[Hove]** M. J. Hove, C. Marie, I. C. Bruce, L. J. Trainor, PNAS 111 (2014) 10383 (via abstract and search):
  https://trainorlab.mcmaster.ca/publications/HoveEtAl2014
- **[Wojtczak]** M. Wojtczak, A. H. Mehta, A. J. Oxenham, PNAS 114 (2017) 1201 (abstract):
  https://pmc.ncbi.nlm.nih.gov/articles/PMC5293018
- **[Friberg & Sundström]** Music Perception 19 (2002) 333 (abstract and KTH page):
  https://www.speech.kth.se/music/performance/Texts/ensemble_swing.htm
- Butterfield, "Participatory Discrepancies and the Perception of Beats in Jazz", Music Perception 27
  (2010) 157 (not read; summarised by Hofmann).
- Tuning: https://drum-tuning.com/kick-drum-tuning-frequency/ ; https://tune-bot.com/tuning-guide/ ;
  https://www.sweetwater.com/insync/how-to-tune-drums ; https://hub.yamaha.com/drums/stage/part-2-the-bass-drum ;
  https://www.moderndrummer.com/2013/07/md-education-team-weighs-in-on-tuning/ ; drumforum threads
  "Tuning Bebop-Kit for Jazz with a DrumDial", "Jazz Tuning Questions", "Intervals for Jazz-Tuning" (seen
  only as search snippets).
- Heads and beaters: https://www.daddario.com/en-ca/blogs/percussion/how-to-choose-a-bass-drumhead ;
  https://pearldrum.com/en/products/options-accessories/bass-drum-pedal-beaters/bass-drum-pedal-beaters ;
  https://www.thomann.co.uk/onlineexpert_page_drumsticks_bass_drum_beater.html
- Playing: S. Fidyk, "Improve Your Bottom Line, Part 1", MD 2012:
  https://www.moderndrummer.com/2012/07/improve-your-bottom-line-part-1-bass-drum-comping-in-44/ ;
  T. Bishop: https://cruiseshipdrummer.com/?p=3985 , https://cruiseshipdrummer.com/2025/07/25/billy-hart-on-the-bass-drum/ ,
  https://cruiseshipdrummer.com/?p=1059 , https://cruiseshipdrummer.com/2024/01/18/philly-joe-set-ups/ ,
  https://cruiseshipdrummer.com/2018/10/15/a-minor-rant-elvins-18 ;
  J. McCaslin: https://jonmccaslinjazzdrummer.blogspot.com/2010/11/feathering-bass-drum-when-2-becomes-4.html ;
  NPR on Kenny Clarke: https://www.npr.org/sections/ablogsupreme/2014/01/08/260769892/the-drummer-who-invented-jazzs-basic-beat ;
  https://jazzprofiles.blogspot.com/search/label/Gretsch%20Drums ; https://www.onjazzdrumming.com/jazz-drumming/how-to-tune-your-drums-for-jazz-drumming.html ;
  https://jazzfuel.com/everything-you-need-to-know-about-bass-drums/ ; https://www.bax-shop.co.uk/blog/drums/how-to-sound-like-a-jazz-drummer/
- Engineering: https://www.soundonsound.com/techniques/synthesizing-drums-bass-drum ;
  https://sonnox.com/articles/drum-decay-resonance-vs-spill-balance ; https://vintageking.com/blog/2018/03/dampening-kick-drum ;
  https://www.native-instruments.com/blogs/blog/kick-drum-eq ; gearspace "Live Jazz – Kick Drum":
  https://gearspace.com/threads/live-jazz-kick-drum.940653/ (search summary only).
- Not reached: the user's reddit r/Jazz thread, rhythmnotes.net, sessionville EQ part 4, the medium.com
  comparison (403), and the YouTube videos (EcbOWf4QasA, X5BMlJrxIzI, geypsEram2U: not watchable here).

## 12. For the model

### 12.1 Testable targets

1. **Pitch:**
   - An 18" bop kick's note at **69–87 Hz** (default ~75–80: drum-tuning.com's "high", the forum's D–F).
     The model is at 70.2 Hz: the bottom of the range. Consider raising `PJ_KICK_TENSION` until the note
     is about 78 Hz.
   - A 20" at 62–75 Hz; the model is at 59.6, under it.
   - The resonant head 0–2 semitones above the batter, by head pitch.
2. **Mode pair:** the out-of-phase (0,1) at about **2.4×** the note (PMI; §3.3). The model has 3.5×
   (248 Hz). The (1,1) at about 1.6–1.9× the note.
3. **Decay, open 18", hard stroke, the room taken out:**
   - The note's T60 **1.5–3 s**; the 150–400 Hz partials' 0.5–1.5 s.
   - With a felt strip ~1 s; buried ≤ 0.3 s.
4. **Spectrum at the mics:**
   - At the centre: the partials at 4–6× the note −10 to −18 dB, those at 13–20× −27 to −46 dB (3 Hz-band
     peaks). An edge strike (r > 0.5 R) adds 15–20 dB at 13–16×.
   - Felt on coated: the click band (1–4 kHz) 30–40 dB under the note's peak.
5. **Glide:** a bomb under +10 % (+165 c), smaller at higher tension; feathered 0.
6. **Dynamics:** 30–40 dB from feathering to bomb; feathering 8–15 dB (A) under the bass's notes.
7. **Timing:** each stroke's P-centre (a 20 ms-smoothed envelope's 50 % rise) within ±3 ms of the snare's
   after compensation; the compensation 0 ms for a bomb and up to ~8 ms early for the softest feathering.

### 12.2 Where the model differs (to decide)

- **(a) The decay is probably too short.** ζ₀₁ = 0.018 gives a T60 of 0.83 s, 3–7× lossier than measured
  lab drums. It was a guess in `kick.md`. Fit `PJ_KICK_ZETA01` to 0.006–0.01 by ear, with the room on.
- **(b) The out-of-phase partner is probably too high** (3.5× against ~2.4×). The DrumAir coupling may
  be too stiff: check A_eff and V, or fit f₋ to about 170 Hz.
- **(c) The note sits low for a bop 18"** (C♯2), the 20" lower still. Fine for Elvin's or Roach's era,
  low for a modern bop kit.
- **(d) Strike positions:** 35 and 36 both land near the centre (0.22 R and 0.12 R). A ringier stroke
  wants r ≈ 0.4–0.6 R. The comment "35 … more ring" holds only weakly at 0.22 R.
- **(e) Burying is not tied to dynamics.** Real feathering is often heel-down and buried, and bombs
  rebound. The model buries only on CC 64 or the knob.
- **(f) No timing compensation.** The kick strikes at the written time; nothing aligns its perceptual
  onset. There is no kick lead or lag anywhere in `drumkit.cc`, `kick.cc`, `director.cc` or `midifile.cc`
  [code]. The only drum timing is the ending's planned triplet.
- **(g) No listening.** The kick plays the file's notes. The director already knows "jazz", the bass's
  channel and "walks". It could:
  - thin feathering to 1 and 3 above about 260 bpm;
  - drop the kick under a bass solo (the bass channel has the highest and loudest line, no lead playing);
  - lower feathered strokes so they stay 8–15 dB under the bass;
  - lift bombs to the band's accents (bandVel), and add set-ups on the "and" before ensemble hits (several
    horns struck together, short and loud);
  - feather more softly under ballads and brushes.
- **(h) The RNG.** `KickPlayer::ur()` is a home-made LCG (`rng_ * 1664525u + 1013904223u`). The project's
  rule is to use `Rng` from `src/rng.h`.
- **(i) Code comment against the data.** `kickSpec` says the resonant head is "a little higher". It is,
  but only through its lighter film (σ 0.20 against 0.26: +14 %, about two semitones in head pitch); the
  tensions are equal. That matches the kit sources. It is the opposite of the concert practice PMI
  reports (batter higher), and PMI's finding that equal coupling decays faster suggests that fitting (a)
  should also try `PJ_KICK_RESO` < 1.
