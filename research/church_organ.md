# A church organ of ~35 stops, physically modelled — research and plan

Status: 2026-11-16. Research and plan only; nothing is built. The user is thinking of about 35
registers, "possibly the ones from Gloria 355 CC" (a four-division digital church organ). Its
disposition (pasted by the user) has about 60 speaking stops; §6 proposes a 35-stop selection.

Marks, as in `toms.md`: **[paper]**, **[web]** a source in §11; **[calc]** worked out here;
**[memory]** organ-building or acoustics knowledge, to be checked; **[design]** a choice of this
plan; **[meas]** measured here on the user's recordings (numbers only; audio stays out of the repo).

## 1. Short answer

- **An organ is many simple instruments sharing one wind.** Each pipe is either a **flue** (a jet
  across a mouth driving an air column: what `src/flute.cc` already models, on a fixed foot pressure
  and fixed "embouchure") or a **reed** (a beating brass tongue on a shallot driving a resonator:
  what `src/reed.cc` models). No player's lips: every pipe is voiced once, by the organ builder.
- **What makes it an organ rather than a sum of samples:**
  1. the **attack** of each pipe (the jet finds the pipe over 26–160 ms; mouth tones and the octave
     or twelfth speak before the fundamental; reeds snap on) **[paper, Castellengo 2004]**;
  2. the **shared wind** (§4a): a blower, reservoirs, trunks and chests; every pipe draws its flow
     and reads its foot pressure; a full tutti (~180-280 L/s, ~11-17 m³/min) makes a modest blower
     struggle, the pressure sags ~5-10 % and every flue droops ~3-6 c and speaks slower (the
     user's wish); a big chord dips and wobbles the wind; the tremulant modulates it
     **[memory; calc; web, Pykett]**;
  3. **many ranks a little apart**: mixture ranks and mutations fuse into one colour; the célestes
     beat; equal temperament against the pure fifths and thirds of the mutations **[memory]**;
  4. the **room**: a stone church (T60 6–7 s in two of the user's organ recordings, §9) and pipes
     spread over metres, low pipes at the sides **[meas; memory]**;
  5. the **swell box** for one division: shutters as a level and a low-pass **[memory]**.
- **Every pipe in the space (the user's requirement, §7a):** each pipe has its place in the case and
  is both a source in the room and a listener to it (flue: at its mouth; reed: at its resonator
  top). Close unison pipes then lock or beat (organ builders' "Mitnahme", measured by Abel et al.),
  a chord spreads over the C/C# sides, the swell box encloses its pipes. The project's room does
  this for a few players but costs O(n²) in sources; §7a.4 restructures it for hundreds of pipes.
- **Cost is the problem, not the physics.** A full plenum with couplers puts **300–700 pipes**
  speaking at once (§7). The flute model costs **145 ns/sample** per voice **[meas: make test]**, so
  a brute-force tutti is 45–100 µs/sample, 2–5 cores. The plan (§8) keeps the physics where it is
  heard (each pipe's attack and its pitch/level under the wind) and uses a cheaper steady-state form
  per rank once a pipe has spoken.
- **The user's MIDI files barely use organ programs**: only GM 17 (drawbar, i.e. a tonewheel organ) in one
  file (`goodbaitGM.mid`, 2646 notes, keys 41–81, mostly 2-note chords); **no GM 19, 20 or 21**
  **[meas]**. So the church organ is mainly a **live** instrument (25-key MIDI controller) and a `-play` voice for
  other files.

## 2. Flue pipes

### 2.1 The oscillator

- **Jet and labium:** wind at foot pressure p passes the flue (a slit, width h ≈ 0.2–1 mm), forms a
  jet of speed U = √(2p/ρ) that crosses the mouth (cut-up W, the mouth's height) to the upper lip.
  The pipe's air column steers the jet; the jet drives the column. Same physics as the flute's jet
  and edge, minus the player **[paper, Fletcher & Rossing ch. 16–17; memory]**.
  - p = 360 Pa (37 mm water column) for a small tin pipe (312 mm, Ø 27 mm, mouth 21 × 7 mm, flue
    0.2–0.3 mm, first mode 475 Hz); the pipe-tone threshold 125 Pa **[paper, Castellengo 2004]**.
  - Typical organ wind **[memory]**: classical/baroque 55–80 mm WC (540–785 Pa); romantic and
    symphonic 80–110 mm (785–1080 Pa); high-pressure reeds and chamades 150–300 mm (1.5–3 kPa).
  - Jet transit time vs period sets the regime: U/W ≈ 2–4 × f for the fundamental **[memory]**;
    voicing sets W (cut-up) so the pipe speaks its fundamental, not the octave.
- **Open vs stopped:**
  - open (Principal, Gamba, Flûte harmonique): all harmonics; length ≈ λ/2;
  - stopped (Gedackt, Bourdon, Subbass): length ≈ λ/4, odd harmonics dominate; a Stopped
    Diapason's fundamental is ~15 dB over its 3rd harmonic **[web, Pykett flutes]**;
  - half-stopped with a chimney (Rohrflöte, Rohrgedackt): a chimney tuned to bring up one
    harmonic (often the 5th or 6th) **[web, Rucz et al. via AT 2017; memory]**.
- **Scaling (diameter against length):** wide pipes are flutier (fewer, weaker upper harmonics),
  narrow ones stringier **[memory]**.
  - Töpfer's Normalmensur: 8′ C (2.4 m) Ø ≈ 155 mm, the diameter halving at the 17th semitone (a
    ratio 2^(−1/16) per semitone of length 2^(−1/12)) **[memory]**;
  - principals at about normal scale (±2 half-tones), flutes 4–8 half-tones wider, strings
    (Salicional, Gamba, Voix céleste) 6–12 narrower, conical Gemshorn/Spitzflöte tapered to 1/3–1/2
    at the top **[memory]**.
- **Harmonic flutes (Flûte harmonique, Flûte octaviante):** a double-length open pipe with a small
  hole at mid-length, overblown to its octave on higher pressure: round, strong, few harmonics; the
  4th harmonic absent within 60 dB in one measured example **[web, Pykett flutes]**.
- **Spectra (steady state, middle of the compass) [web, Pykett; memory]:**

| Family | Example | Harmonics | Shape |
|---|---|---|---|
| Principal | Principal 8′, Octave 4′ | 8–15 audible | 2nd −3…−8 dB, 3rd −6…−12, smooth fall |
| Stopped flute | Gedackt, Bourdon | 3–6 | odd dominant; 3rd ~−15 dB |
| Open flute | Hohlflöte, Claribel | 3–5 | 3rd −25…−30 dB |
| Harmonic flute | Flûte harmonique | 2–4 | nearly pure; 4th < −60 dB |
| String | Gamba, Salicional, Céleste | 15–30 | upper harmonics strong, fundamental weak |
| Conical | Gemshorn, Blockflöte | 4–8 | octave and twelfth emphasized |

- **Attack (the "speech") [paper, Castellengo 1999, 2004; web, Pykett violone]:**
  - pallet opening to steady foot pressure: 160 ms ("slow"), 45 ms ("short"), 26 ms ("impulsive");
    the pipe-tone threshold crossed at 40, 9 and 4 ms;
  - before the pipe tone locks, **mouth tones**: quasi-pure tones at the pipe's eigenfrequencies
    where the edge-tone curves cross them (often the octave or a higher mode first); a "chiff" of
    jet noise; string pipes (narrow, high harmonics) speak slowly, low pipes over up to ~1 s;
  - a quicker transient is not always heard as more "precise": early mouth tones blur it;
  - voicing aids (nicking of the languid: less chiff; ears, beards on strings: steadier speech)
    **[memory]**.
- **Mixtures and mutations [memory]:**
  - a mutation sounds a harmonic of the 8′: Quinte 2⅔′ (3rd), Nasard 2⅔′, Terz 1⅗′ (5th),
    Quinte 1⅓′ (6th), Sifflöte 1′ (8th); the Sesquialtera II = 2⅔′ + 1⅗′;
  - a mixture is several ranks of octaves and fifths per key (Mixtur V on 2′: 2, 1⅓, 1, ⅔, ½ at
    C), **breaking back** an octave (or a fifth) every octave or so so no pipe gets too small; the
    Cymbel (high, often with a third) breaks every few notes; Plein jeu IV and Scharf IV likewise;
  - the ranks are tuned to equal-tempered octaves but their fifths are pure (in a mixture's own
    pipes): they fuse into the chorus and are heard as brightness, not as notes.
- **The voix céleste:** a string rank tuned sharp (or flat) by 2–5 cents, ~1–3 beats/s at middle C
  against the Gamba **[memory]**.

### 2.2 Temperature and wind

- Flue pitch follows the speed of sound: **+3 cents per °C** (0.18 %/°C) **[calc]**; reeds hardly
  move (their tongue sets them) — a warm church puts flues sharp of reeds **[memory]**.
- More wind: flue pitch rises slightly (a few cents per 10 % pressure, and the level ~1.5–3 dB)
  **[memory]**. A sagging chest (§4) flattens and softens every flue a little.

## 3. Reed pipes

- **The tongue:** a curved brass tongue beats against a shallot (a brass tube with a slot) in a
  boot under foot pressure; the tuning wire sets its free length. Odd and even harmonics in the
  driving waveform; the resonator shapes them and matches impedance **[web, Pykett reeds]**.
- **Resonators [web, Pykett; memory]:**

| Stop | Resonator | Character | Measured |
|---|---|---|---|
| Trompete 8′, Trompette harm., Clairon 4′ | conical, ~λ/2 (double length when harmonic) | brilliant | 8′ F#3: ~11 harmonics |
| Posaune 16′, Contra Posaune 32′ | conical, often half-length in the bass | weak fundamental | 16′ C2: ≥ 75 harmonics to 5 kHz; fundamental 20 dB under the 2nd, inaudible if removed |
| Oboe 8′ | narrow conical, capped bell | keen, nasal | F#3: ~9 harmonics in 3 kHz |
| Cromorne 8′, Dulcian 16′, Fagott 16′, Klarine 4′ | cylindrical, ~half length | even harmonics suppressed | Krummhorn C3: odd strong |
| Vox humana 8′ | very short cylindrical (~1/8 of a clarinet's) | a formant: dominant harmonic ~1.85 kHz, fixed over 4 octaves (±18 %) | reed electric piano F#3 |
| Trompette en chamade 8′ | conical, horizontal, high pressure | very bright, direct | — |

- **The reed dominates the pitch**, the resonator the timbre; a half-length resonator has no
  resonance at the fundamental, which radiates poorly **[web, Pykett]**.
- **Attack:** reeds speak fast (a few periods), with a characteristic "blatt" or snap; bass reeds
  can be slow to settle **[memory]**. Angster's group measured tongue velocity, boot and shallot
  pressure and the sound on lingual pipes with a high-speed camera **[paper, Angster et al. JASA
  2016]**.
- The project's `src/reed.*` (a beating reed against a waveguide bore, with the bassoon's
  `prime`) is the model; an organ reed is the same with no lips, a fixed "embouchure" (the tongue's
  curvature and the wire), and a resonator that is a cone or a cylinder of given length.

## 4. The wind system

- **Chests:** a slider chest has one channel per key; every stop drawn on it gets wind from that
  channel when the pallet opens. Pipes of one key share the pallet's flow; all keys share the
  reservoir **[memory]**.
- **Wind sag and "breathing":** a full chord on a full registration draws more than the reservoir
  can replace at once: the pressure drops a few % and recovers in tens of ms; modern stiff wind
  hardly sags, historic "flexible" wind sags audibly (a slight pitch and level dip at big
  chords, recovering) **[memory]**. Model: a reservoir as a mass-spring-damper (Helmholtz-like,
  natural frequency 2–8 Hz) driven by the flow of all speaking pipes **[design]**.
- **Tremulant:** a valve or bellows that modulates the wind of a division at **~4–7 Hz** (≈ 3.8 Hz
  measured on one sample; "about 6 Hz" a fast one) **[web, jOrgan; BYU organ tutor]**. Depth: a few
  % of the pressure, heard as ±5–15 cents and ±1–3 dB on flues, mostly level on reeds **[memory]**;
  theatre organs deeper. The rate depends on the trem's mass and spring and on the wind drawn
  **[web, Pykett digital trems]** — so a big chord slows and deepens it (physics, not a rule).
- **The swell box (Schwellwerk):** shutters in front of an enclosed division: closed, roughly
  −15…−25 dB and the highs cut more than the lows **[memory]**; the box is also a small room (the
  pipes speak into it). Live: the swell shoe → CC (expression).
- **Couplers:** HW-Ped, SW-Ped, Pos-Ped, SW-HW, Pos-HW, SW-Pos; sub- and super-couplers (an octave
  down/up) add pipes. Couplers multiply the pipe count (§7).
- **Pressures per division [memory]:** Positiv ~60 mm, Hauptwerk ~75 mm, Schwellwerk ~90 mm,
  Pedal ~90 mm, chamade 150+ mm.

## 4a. The wind supply, modelled physically: blower, reservoirs, trunks, chests

The user: *"the compressor should also be modeled, I like the effect when the compressor struggles a
bit when all stops are pulled, and the tuning droops a bit in all stops + lots of notes."* So the
wind is not a constant: it is a small fluid network from the blower to every pipe's foot, and the
pipes' own flow draws it down.

### 4a.1 The chain

    blower (fan) -> main reservoir (regulator) -> wind trunks -> division regulators (or Schwimmer)
                 -> chest (pallet box, key channels) -> pipe feet -> mouths / shallots

- **The blower:** a centrifugal fan (the "compressor"; historically hand- or foot-pumped bellows).
  Its curve: the pressure falls as the flow rises, roughly p(Q) = p0 (1 - (Q / Qmax)^2), with the
  shut-off pressure p0 above the working pressure so the regulators have something to throttle
  **[memory]**. Laukhuff's Ventus blowers for the 120 mm WS (1200 Pa) range deliver about 7.5, 13,
  21, 28 m³/min (125-470 L/s) by size **[web, Laukhuff datasheet]**; flow is quoted with its
  pressure, a CFM figure alone means little **[web, The Diapason]**. Builders size a blower by rules
  of thumb on the number of ranks **[web, toff.org.uk]**.
- **Reservoirs and regulators:** a box with a hinged or parallel-rising top loaded by springs or
  weights sets the pressure F / A; an inlet valve (curtain or cone) opened by the falling top lets
  the blower's air in. Top ~1 × 1.2 m, mass ~20-40 kg, volume ~0.3-0.6 m³ **[memory]**. It is a
  mass on the air's spring with a valve in a feedback loop: it overshoots and wobbles ("shimmy")
  after a sudden demand; eveners reduce it **[web, organsupply; toff regulator]**. Chest-mounted
  Schwimmers (small, stiff, fast) are compact but feel less "breathing" **[web, Parkey]**.
  Resonances of reservoirs, valves and blower cause pipe flutter if badly matched **[paper, Taylor,
  Salford thesis]**.
- **Trunks:** wooden or metal ducts 3-10 m long, 0.05-0.1 m² section: an air mass (inertance ρl/S)
  and a compliance (V / ρc²), a lumped Helmholtz-like line; a quarter-wave resonance ~10-30 Hz for
  3-8 m **[calc]**.
- **Chest:** the pallet box under each key channel; a big chord draws on one pallet box at once;
  the channel's own volume makes each key's foot pressure rise over a few ms (the attack, §2.1).

### 4a.2 What the pipes draw [calc]

A flue's flow is the jet's: Q ≈ C · U · (mouth width × flue depth), U = √(2p/ρ), C ≈ 0.7 (the flue's
contraction), limited by the foot hole when it is smaller than the flue (toe-voicing) **[web,
pipe-scale calculator (unverified tool); IOA Woolley]**. At 80 mm WS (785 Pa) U ≈ 36 m/s:

| Pipe | Mouth width × flue | Flow |
|---|---|---|
| Principal 8′ C (Ø ~155 mm) | ~120 mm × 1 mm | ~3 L/s |
| Principal 8′ c¹ (Ø ~50 mm) | ~40 mm × 0.5 mm | ~0.5 L/s |
| a top pipe of a mixture (Ø ~8 mm) | ~6 mm × 0.2 mm | ~0.03 L/s |
| Principalbass 16′ C | ~200 mm × 2 mm | ~10 L/s |
| Untersatz 32′ C (stopped, wide) | ~250 mm × 3 mm | ~20 L/s |
| a reed (Trompete 8′, mid) | shallot opening | ~0.5-1 L/s |

- **The tutti with a big chord** (the user's case): Hauptwerk ~17 ranks (incl. Mixtur V, Cymbel III)
  × 4 notes × ~0.6 L/s ≈ 40 L/s; Schwellwerk ~14 ranks × 4 × 0.5 ≈ 28 L/s; Positiv ~6 ranks × 4 × 0.4
  ≈ 10 L/s; Pedal ~12 ranks (incl. Mixtur IV) × 2 notes × ~4 L/s ≈ 100 L/s (the large pipes
  dominate); couplers (manual to pedal, sub/super) +50-100 L/s → **~180-280 L/s ≈ 11-17 m³/min**.
  One stop with one note: ~0.5 L/s, a thousandth of that.
- So a blower of ~10-13 m³/min (the 13 m³/min Ventus class) carries any plenum but **struggles at
  the full tutti**: it runs near Qmax, its pressure falls toward the working pressure, the
  regulators' valves open fully and can no longer hold the set pressure: the user's effect.

### 4a.3 What the pipes do when the wind sags [memory; calc]

| Effect | Flue pipes | Reed pipes |
|---|---|---|
| **Pitch** | rises with pressure: ~+0.3-0.6 cent per +1 % of pressure (the jet faster against the column); a 5 % sag droops 2-3 c, 10 % 3-6 c | the tongue sets the pitch: ~0.1-0.3 c per 1 %, either way |
| **Level** | the jet's power ∝ U³ ∝ p^1.5: −0.7 dB per −10 % | louder with pressure, more than flues (~1-2 dB per 10 %) |
| **Timbre** | lower pressure: fewer upper harmonics (the jet slower against the period), a hollower tone; much lower: the pipe speaks late or not at all | lower pressure: duller, the reed closing less hard |
| **Attack** | short wind: a slower rise in the foot, a longer, weaker speech (Castellengo's 160 ms "slow" against 26 ms "impulsive", §2.1) | slower to start |

- **The transient:** a big chord struck at once dips the chest pressure by a few % for ~50-150 ms
  until the regulators open, then it recovers with an overshoot and a wobble at the reservoir's
  own frequency (~2-6 Hz) — the "breathing" wind **[memory]**.
- **All pipes droop together** (their division's chest), the big pipes in the bass first (their
  flow is largest); the mixtures' tiny pipes barely draw but droop with the rest.

### 4a.4 The tremulant in this system

- A valve (retained-wind: modulating the reservoir or the regulator's pan; spilled-wind: venting
  the trunk) opens and closes at 4-7 Hz; the reservoir's top and the trunk respond: the pressure
  swings by a few %, the pipes follow it in pitch and level (§4a.3) **[web, Pykett digital trems;
  toff regulator]**. With a Schwimmer there is no reservoir: the trem moves the regulator's pan.
- In the model it is just another flow element in the network: a periodically opened vent. Its
  depth and even its rate then depend on the demand (a big chord deepens and slows it) and on the
  reservoir's tuning, as in a real organ; no rule is written for it **[design]**.

### 4a.5 The model [design]

- **Elements (lumped, per sample or per 4-8 samples):**
  - blower: Q_b = Qmax √(1 − p_blower / p0) into a plenum volume;
  - main reservoir and one regulator per division: the top as a mass-spring-damper (its height
    sets the volume and the inlet valve's opening) on the air's compliance;
  - trunks: an inertance and a compliance each (or a short waveguide for their 10-30 Hz resonance);
  - chests: a volume each, the key channels optionally as small volumes behind the pallets;
  - each sounding pipe: its flow Q_i(p_i) (the jet's U × area, or the reed's opening), drawn from
    its chest, and its foot pressure p_i = the chest's (plus, optionally, the channel's).
- **Solve:** the chest pressures and the network each sample (a few dozen states for 4 divisions:
  under 1 µs per sample, nothing against the pipes' budget). Every pipe reads its chest pressure
  each sample: the jet's U (flue) or the reed's driving pressure follows the wind as it is. The
  pitch droop, the level and timbre change and the slower attack then come out of the pipe models
  themselves, not from a pitch or gain rule.
- **The user's choice (2026-11-16): modern wind that struggles clearly at the full organ** —
  "probably modern, and struggle clearly, but perhaps 8 %, 4 cents or something like that":
  - stiff, well-damped regulators: ordinary chords and plena barely sag (< ~2 %) or wobble;
  - a blower sized so that the full organ with couplers and a big chord (~230 L/s ≈ 14 m³/min,
    §4a.2) sags **~8 %** (flues ~4 c flat at ~0.5 c/%): the regulators hold their set pressure
    until the blower's pressure falls to it, at Q_s = Qmax √(1 − p_set/p0); above that the chest
    follows the blower, the pipes' demand ∝ √p. Solving for 8 % **[calc]**: with the shut-off
    pressure p0 = 1.25 × the working pressure (100 mm WS over 80 mm), **Qmax ≈ 26 m³/min**
    (Q_s ≈ 11.5 m³/min); with p0 = 1.5 ×, **Qmax ≈ 21 m³/min** — the 21-28 m³/min Ventus classes.
    The model's own measured tutti demand sets the final Qmax (it scales with it).
- **Knobs:** blower size (Qmax: as above, so a tutti struggles as the user likes), regulator
  stiffness (steady modern ↔ flexible historic), reservoir mass and springs (the wobble's rate),
  trunk lengths, the tremulant's rate and depth.
- **Checks:**
  - one stop, one note: the chest moves < 0.05 %, no audible change;
  - a hymn plenum chord: −1…−2 % at the attack, recovering in ~100 ms, ~1 c droop;
  - all 37 stops + couplers + a 10-note chord with a blower of ~10 m³/min: a sustained sag of
    ~5-10 %, flues ~3-6 c flat and ~0.5 dB softer, the attack slower; on release the pressure
    overshoots and wobbles ~2-5 Hz for ~0.5 s;
  - the tremulant's rate 4-7 Hz, its depth deepening with demand.

## 5. Tuning, layout, room

- **Temperament:** modern organs mostly equal; historic ones meantone or well-tempered (Werckmeister,
  Kellner, Vallotti). Equal is the safe default; a well-tempered option is cheap (a per-pitch-class
  offset), cf. the harpsichord's just option **[design]**.
- **Layout:** pipes stand on C and C# sides (alternate semitones left and right), basses at the
  ends, the Positiv in front (Rückpositiv), the swell box above or behind, the pedal towers at the
  sides **[memory]**. In the project's room model each division is a source group with its own
  stage place (the ensemble's x seats).
- **Room:** the project's `church` room (15 000 m³, RT60 4.0 s) **[src/room.cc]**; the user's organ
  recordings end with T60 ~6–7 s (§9) — a bigger room, or the church preset lengthened.
- **The 32′ stops:** Untersatz 32′ C = 16.35 Hz, felt more than heard; most speakers and the room
  model's low end may not carry it. Contra Posaune 32′ is audible through its harmonics (a reed's
  fundamental is weak anyway) **[calc; web, Pykett]**.

## 6. Choosing ~35 stops

The user's list has ~60 speaking stops (Pedal 12, Hauptwerk 13, Schwellwerk 16, Positiv 13). A
~35-stop selection (now 37) that keeps a complete organ: a principal chorus with mixture on each manual, a
flute and a string family, a French swell with céleste and reeds, a Positiv with a cornet colour and
a Cromorne, a pedal with flue and reed basses **[design]**:

| Division | # | Stops |
|---|---|---|
| **Pedal** | 9 | Untersatz 32′, Principalbass 16′, Subbass 16′, Violonbass 16′, Octavbass 8′, Choralbass 4′, Mixtur IV, Posaune 16′, Trompete 8′ |
| **Hauptwerk** | 11 | Principal 16′, Principal 8′, Flûte harmonique 8′, Rohrflöte 8′, Octave 4′, Quinte 2⅔′, Superoctave 2′, Mixtur V, Cymbel III, Trompete 16′, Trompete 8′ |
| **Schwellwerk** | 12 | Bourdon 16′, Gedackt 8′, Viola di Gamba 8′, Voix céleste 8′, Flûte octaviante 4′, Nasard 2⅔′, Waldflöte 2′, Terz 1⅗′, Fagott 16′, Trompette harmonique 8′, Oboe 8′, Clairon 4′ |
| **Positiv** | 5 | Rohrgedackt 8′, Octave 4′, Gedacktflöte 4′, Sesquialtera II, Cromorne 8′ (provisional: the user's Positiv list may follow) |
| Accessories | — | Tremulants (SW, Pos), all couplers incl. sub/super, swell shoe |

**The Pedal (confirmed by the user as 12 stops: Untersatz 32′, Principalbass 16′, Subbass 16′,
Violonbass 16′, Octavbass 8′, Gedacktbass 8′, Choralbass 4′, Mixtur IV, Contra Posaune 32′,
Posaune 16′, Trompete 8′, Klarine 4′).** Its character is a full independent pedal: a 32′
foundation, three different 16′ colours, its own principal chorus up to a mixture, and a reed
chorus. Kept, 9:
- **Untersatz 32′** (a stopped 32′, C = 16.35 Hz, sounding mostly by its 3rd harmonic and the
  floor's shake): the pedal's depth; the stop that makes it a large organ; one stopped rank is cheap
  (a stopped pipe's few odd harmonics) **[calc]**;
- **Principalbass 16′, Octavbass 8′, Choralbass 4′, Mixtur IV**: the pedal's own principal chorus
  (the bass of the hymn plenum without coupling the manuals, and the cantus firmus stop: Choralbass
  4′ for a chorale melody in the feet);
- **Subbass 16′** (stopped, soft): the accompaniment bass; **Violonbass 16′** (a narrow open string
  bass): the third 16′ colour, the bass for the strings and the French romantic tutti;
- **Posaune 16′, Trompete 8′**: the reed chorus for full organ.
Left out, 3: **Gedacktbass 8′** (the Subbass 16′ with the Octavbass 8′ covers its role; on many
organs it is the Subbass extended), **Contra Posaune 32′** (the costliest rank in the organ: long
reed resonators, ~45 harmonics each to 1.5 kHz at C1, and its role is taken by Posaune 16′ +
Untersatz 32′), **Klarine 4′** (the third reed of the chorus: a luxury once Trompete 8′ is there).
If the user wants all 12, the manuals lose three more (e.g. HW Quinte 2⅔′, SW Clairon 4′, Pos
Quinte 1⅓′).

**The Hauptwerk (confirmed by the user as 13 stops: Principal 16′, Principal 8′, Flûte harmonique
8′, Rohrflöte 8′, Salicional 8′, Octave 4′, Blockflöte 4′, Quinte 2⅔′, Superoctave 2′, Mixtur V,
Cymbel III, Trompete 16′, Trompete 8′).** Its character is a large 16′-based Hauptwerk: a principal
chorus from 16′ to a mixture and a cymbel, crowned by a 16′ + 8′ trumpet chorus, with two 8′
flutes of different kinds. Kept, 11:
- **Principal 16′**: the plenum's foundation, the gravity that makes it a Hauptwerk of a large
  organ rather than a Positiv (left out of the first draft; restored);
- **Principal 8′, Octave 4′, Quinte 2⅔′, Superoctave 2′, Mixtur V**: the chorus itself;
- **Cymbel III**: the plenum's crown, a high mixture breaking every few notes, the sparkle over the
  Mixtur V (left out of the first draft; restored: the two mixtures together are the north German
  plenum's character, and their high ranks are short pipes, cheap) **[memory; calc]**;
- **Flûte harmonique 8′** (the romantic solo flute) and **Rohrflöte 8′** (the classical chimney
  flute): two flutes of different centuries, both worth their pipes;
- **Trompete 16′ + Trompete 8′**: the trumpet chorus on the 16′ plenum.
Left out, 2: **Salicional 8′** (a soft narrow string; the Schwellwerk's Viola di Gamba and Voix
céleste give the strings, and enclosed, which is where they are wanted for accompaniment; on the
Hauptwerk it would mainly colour the 8′ fonds, which Principal 8′ + Rohrflöte 8′ already do),
**Blockflöte 4′** (a third 4′ beside Octave 4′; the flute 4′ colour is on the Schwellwerk and
Positiv).

**The trade for the two restored Hauptwerk stops** (first revision): the Schwellwerk dropped its
Vox humana 8′ and the Positiv its Quinte 1⅓′.

**The Schwellwerk (confirmed by the user as 16 stops: Bourdon 16′, Principal 8′, Gedackt 8′, Viola
di Gamba 8′, Voix céleste 8′, Octave 4′, Flûte octaviante 4′, Nasard 2⅔′, Waldflöte 2′, Terz 1⅗′,
Plein jeu IV, Fagott 16′, Trompette harmonique 8′, Oboe 8′, Vox humana 8′, Clairon 4′).** Its
character is the French romantic swell (Cavaillé-Coll's récit expressif): foundations, strings with
the céleste, a cornet of mutations, and the reed battery (anches) that builds the crescendo as the
box opens **[memory]**. The user does not want the **Vox humana 8′**: it is out for good, and its
place goes to the **Waldflöte 2′**, which completes the cornet. Kept, 12:
- **Bourdon 16′, Gedackt 8′**: the stopped foundations, and the cornet's base;
- **Viola di Gamba 8′, Voix céleste 8′**: the strings and their beating pair, the swell's signature
  soft sound;
- **Flûte octaviante 4′, Nasard 2⅔′, Waldflöte 2′, Terz 1⅗′**: with the Gedackt 8′ the full cornet
  décomposé (8 + 4 + 2⅔ + 2 + 1⅗: the 1st to 5th harmonics), the French solo colour and the
  mutations' fusion (§2.1); the Waldflöte 2′ also serves alone as a bright flute 2′;
- **Fagott 16′, Trompette harmonique 8′, Clairon 4′**: the reed battery 16-8-4, the core of the
  French tutti and of the box's crescendo;
- **Oboe 8′**: the classic swell solo reed (with the Positiv's Cromorne, the solo reeds now that the
  Vox humana is gone).
Left out, 4: **Principal 8′ and Octave 4′** (a French swell lives on flutes, strings and reeds; the
principal colour comes from the Hauptwerk through SW-HW), **Plein jeu IV** (the swell tutti's
brilliance comes from the reed battery, and the Hauptwerk's Mixtur V and Cymbel III are coupled
in), **Vox humana 8′** (not wanted).

**The trade for the Schwellwerk's 12:** the Positiv gives up two more (provisionally 5: Gemshorn 8′
and Octave 2′ out), and the total is **37** (Pedal 9, Hauptwerk 11, Schwellwerk 12, Positiv 5): two
over 35, to keep the cornet and the reed battery whole. For exactly 35, the next to go would be the
Schwellwerk's Clairon 4′ (the battery as 16 + 8) and the Positiv's Gedacktflöte 4′, or the
Hauptwerk's Trompete 16′ and the Pedal's Violonbass 16′. The Positiv's selection is provisional
until the user's list for it.

Left out of the Positiv so far: Principal 8′, Gemshorn 8′, Octave 2′, Quinte 1⅓′, Sifflöte 1′,
Scharf IV, Dulcian 16′ and the Trompette en chamade (a spectacular extra to add first if wanted).
The selection covers about 20 distinct pipe families (§8 stage 3).

### 6.1 Registrations (presets)

| Use | Registration |
|---|---|
| Soft accompaniment | SW Gedackt 8′ + Viola di Gamba 8′ (+ Voix céleste), box half closed; Ped Subbass 16′ |
| Hymn (plenum) | HW Principal 8, Octave 4, Superoctave 2, Mixtur V (+ Quinte 2⅔); SW-HW; Ped Principalbass 16, Octavbass 8, Choralbass 4, HW-Ped |
| Grand plenum (north German) | HW Principal 16, 8, Octave 4, Quinte 2⅔, Superoctave 2, Mixtur V, Cymbel III (+ Trompete 16, 8); Ped Untersatz 32, Principalbass 16, Octavbass 8, Choralbass 4, Mixtur IV, Posaune 16 |
| Full organ | plenum + HW Trompete 16/8, SW reeds, Pos-HW; Ped Posaune 16, Trompete 8 |
| Trio (Bach) | RH: Pos Rohrgedackt 8 + Gedacktflöte 4 + Sesquialtera; LH: SW Gedackt 8 + Flûte octaviante 4; Ped Subbass 16 + Octavbass 8 |
| French cornet solo | SW Gedackt 8 + Flûte octaviante 4 + Nasard 2⅔ + Waldflöte 2 + Terz 1⅗ (box open); accompaniment HW Rohrflöte 8; Ped Subbass 16 |
| French romantic | SW fonds 8 + Gamba + céleste, box closed, then the reed battery (Fagott 16, Trompette 8, Clairon 4) and the box opening (the crescendo), SW-HW; HW Flûte harm. 8 |
| Solo | Pos Cromorne 8 (+ Tremulant), or SW Oboe 8 or the SW cornet, over SW strings |

### 6.2 From a MIDI file

- **GM 20 (Church Organ)**: a default "hymn" or "principals 8+4" registration on the part; organs
  have no key velocity — velocity is ignored for the pipes **[memory]**, CC11/CC7 open and close the
  swell box (if the part is on the swell) **[design]**.
- **Texture rules [design]:** one line → a solo or flute registration; 3–4 voices in close chords →
  plenum; low notes under the left hand's range (below C3) → the pedal division (as the bagpipe's
  drones are routed by range); very long notes and pp → the soft accompaniment.
- **GM 21 (Reed Organ)** is a harmonium (free reeds) — not this instrument (see `free_reeds.md`).
- **GM 17–19** (drawbar, percussive, rock organ) are tonewheel organs — a different instrument
  (tonewheel organ: tonewheels, drawbars, rotary speaker), out of scope here; the user's one organ file uses GM 17.

## 7. Cost

- **Pipes speaking at once [calc]:**
  - hymn plenum, 4-note chords: HW 8+4+2⅔+2 = 4 ranks + Mixtur V = 9 ranks × 3 notes = 27 (the
    grand plenum with Principal 16′ and Cymbel III: 13 ranks × 3 = 39), SW
    coupled (4 ranks) × 3 = 12, Pedal 3 ranks × 1 = 3 → ~45 pipes (with release tails ×1.5: ~70);
  - full organ with sub/super couplers and Plein jeu: ~25 ranks × 4 notes × (1 + super + sub) →
    300–400; with 32′ and the chamade ~450; with release tails and chords of 6–8 notes up to ~700.
- **The project's costs [meas]:** a flute voice 145 ns/sample (0.7 % of a core), bassoon 108,
  pads 250–600, the grand piano and drums 400–1500.
- **Brute force:** 400 flute-model pipes = 58 µs/sample = 2.8 cores; 700 = 4.9 cores. Feasible on
  the user's 32-thread machine with the part-parallel renderer (`src/parallel.h`, up to 16 threads)
  if each division (or each rank group) is its own job, but not on a laptop, and not live with
  headroom **[calc]**.
- **Cheaper, still physical [design]:**
  - a pipe = a short waveguide (its own length) + a reduced jet (a delayed, saturated, low-passed
    feedback, as the classic flue-pipe waveguide models) or a reduced reed: ~20–40 ns/sample;
    700 pipes = 14–28 µs/sample, 0.7–1.3 cores;
  - or the full jet model only during the attack (the first ~0.2 s, where the physics is heard)
    and then the pipe's steady state continued by its own fitted modal resonator (a few modes,
    ~5–10 ns): a hybrid that keeps the speech and the wind's influence;
  - mixtures: their ranks per key share one wind input and one attack model with small detunings;
  - asleep when silent, as every instrument here.
- **With the user's budget (3 cores for the pipes, §7b.1)** the full model can run for every pipe
  up to ~350 sounding at once; the cheaper forms below are fallbacks for the largest tuttis.
- **What must stay physical (audible) [memory; design]:** each pipe's speech (§2.1), the wind's
  common sag and the tremulant acting on all pipes of a division, the célestes' beating, the
  mixture ranks' slightly imperfect tuning, the room. The steady spectrum of a principal is not
  the hard part; its onset and its life on the shared wind are.

## 7a. Every pipe in the room: positions and two-way coupling

The user's requirement: *"every one of the pipes should have its own position in the space and
connect to the space in a different way, every pipe is both excited by the vibrations in space as
well as excites the space itself."*

### 7a.1 Positions: the layout of the case [memory, to be set per design]

| Division | Where | Chest | Pipes |
|---|---|---|---|
| **Hauptwerk** | the main case, chest ~3-4 m above the gallery floor | one slider chest ~3-4 m wide; each key a channel front to back, the stops in rows across | **C side and C# side**: the pipes of whole tones on one half, the semitones on the other, the largest at the outer ends or at the centre (a "C/C#" or "mitred" layout); the Principal 8′/16′ basses in the **façade** (front, mouths ~2-3 m above the chest, pipe Ø up to 15-25 cm) |
| **Schwellwerk** | above or behind the Hauptwerk, in a **box** ~3 × 2 × 3 m | as above, inside the box | everything radiates into the box; the box opens to the church through **shutters** (front, louvres) |
| **Positiv** | a separate case at the gallery rail (Rückpositiv), ~1.5-2 m in front of the player, ~1 m lower than the HW chest | ~2 m wide | C/C# as above; its own small façade |
| **Pedal** | two **towers** at the sides of the main case, ~4-6 m apart | C side left, C# side right | 16′ C ~5 m tall (open), Untersatz 32′ stopped ~5 m, mouths ~0.5-1 m above their chests; reeds' resonators standing behind |

- **Spacing:** pipes of a rank stand 1-3 cm apart (diameter plus a gap); the ranks of one key stand
  in a row front to back, ~5-15 cm between ranks; mixture ranks of one key in a tight cluster
  **[memory]**.
- **Mouth heights:** a pipe's mouth sits at its foot's top, 10-40 cm above the toeboard for manual
  pipes (longer feet for the large ones), ~1 m for pedal flues **[memory]**.
- **Where each pipe radiates:**
  - **open flue:** two sources, the **mouth** and the **open top** (a length L ≈ λ/2 above it); for
    the fundamental and the odd harmonics they breathe out **in phase** (the pressure antinode in the
    middle pushes air out of both ends together), for the even harmonics in **opposite phase** (a
    dipole along the pipe's axis, cancelling broadside: a listener level with the pipe's middle hears
    less octave; one below the gallery, the top ~0.3 L farther than the mouth, hears it) **[calc:
    the open-open modes' velocity cos(kπx/L) at the two ends; corrected from an earlier draft that
    had the phases the other way round]**;
  - **stopped flue** (Gedackt, Bourdon, Subbass, Untersatz): the **mouth** only, a monopole;
  - **chimney flue** (Rohrflöte, Rohrgedackt): the mouth, plus the chimney's top weakly;
  - **reed:** the **resonator's top** only (the boot is closed); the en chamade horizontal, pointing
    into the church.
- So each pipe is one or two **point sources** at known (x, y, z): mouth at the chest height + foot,
  top at mouth + L. A typical 37-stop organ has ~2400 pipes **[calc: 33 manual stops × 61 + 5
  mixture/mutation stops × ~3.5 ranks × 61 + 9 pedal × 32 + Mixtur IV × 32 ≈ 2500]**.

### 7a.2 Into the room, and back

- **Excitation of the room:** each source's volume flow → pressure at 1 m (as `src/flute.cc` already
  computes for the flute's embouchure and open end) → the room's direct paths, early reflections and
  reverberant field.
- **The room acting on the pipe:**
  - **flue:** the acoustic **velocity across the mouth** deflects the jet (the jet is the sensitive
    element: a few % of the jet's own oscillation is enough to pull it), and the pressure at the
    open top drives the column; `src/flute.cc` already takes the room's pressure at its embouchure
    (Tube::setRoom: the room's pressure in the jet's feedback, `pRoom_`);
  - **reed:** the pressure at the resonator's top travels down to the tongue (weakly; the reed is
    stiff), as `src/reed.cc` takes it at the bell (Tube::stepBore's `pRoom_`).
- **The known physics: synchronization ("Mitnahme") [paper]:**
  - two flue pipes side by side, a little detuned, **lock to one frequency**; the locking range
    (the Arnold tongue) grows with the coupling and shrinks with the distance between them, not
    monotonically: a delay in the coupling (their distance / c) reshapes it **[Abel, Bergweiler,
    Gerhard-Multhaupt 2006; Sawicki, Abel, Schöll 2018]**;
  - pipes of equal pitch very close can **fall silent** ("oscillation death", Rayleigh's
    observation) or lock **in anti-phase** (destructive interference: weaker) **[Abel et al. 2006;
    DAGA 2011]**;
  - a loudspeaker drives a pipe into synchronization over ~3 decades of amplitude **[Abel et al.
    PRL 2009]**: the room's field at the mouth is a real force on the pipe;
  - both near-field (adjacent pipes) and far-field (the room) coupling matter; synchronization can
    stabilize the pitch but attenuate the sound, particularly in façade pipes **[arXiv 2025]**;
  - the physics is captured by delay-coupled self-sustained (van der Pol-like) oscillators, and in
    full by compressible Navier-Stokes simulations **[Fischer & Abel JASA 2013; Fischer & Bader
    2015]**. Organ builders know Mitnahme: pipes of close pitch are not placed side by side
    **[memory]**.
- **The shared wind chest** is a further coupling path: pipes on one key channel share the pallet's
  pressure, and all share the reservoir (§4) **[memory]**.

### 7a.3 The project's room now, and what it costs [read in src/room.*, src/ensemble.cc]

- `Room` (src/room.h/.cc): sources on a line across the stage; per sample it
  - writes each source into a ring;
  - reads the 16-line feedback delay network once, and for **every listener** (each source and the two
    microphones) a 16-tap projection of the reverberant field **plus the direct sound of every
    other source** (`dGain_` is ns × ns: an **O(ns²)** loop);
  - early reflections: 24 image sources per source and microphone (**48 taps per source**);
  - the sources into the network through 3 allpass diffusers each, and a 16-tap projection.
- Per source and sample, roughly 16 (field out) + 16 (field in) + 6 (diffusers) + 48 (early) + **ns**
  (direct from everyone) multiply-adds **[calc]**. For an ensemble of 8 that is ~100; for 500
  sounding pipes ~590 each, **~300 000 per sample** (≈ 14 G MAC/s): many cores. It does not scale
  as it is.
- `Ensemble` places each voice at a seat (1 m apart) and feeds `atSource` back into each voice's
  tube (`Tube::setRoom`) block by block (`Room::kBlock` = 32 samples, 0.67 ms). The organ needs the
  same, per pipe, at real 3-D positions.

### 7a.4 A structure that scales [design]

| Path | Exact for | Cost per pipe and sample |
|---|---|---|
| **Direct sound to the listeners** | every sounding pipe (each source: mouth, top) | 2 delayed taps per source (left, right mic): ~4 |
| **Near-field neighbour coupling** | the k nearest sounding pipes within ~1 m (same chest region: neighbours on the C/C# side, the other ranks of the same key) | k delayed taps (k ≈ 6-8): ~8 |
| **Early reflections** | per **chest region** (e.g. 8 regions: HW C, HW C#, Pos C, Pos C#, Ped left, Ped right, SW box, façade), not per pipe | the pipe adds into its region's sum: 1; each region 48 taps (shared) |
| **Late (reverberant) field** | shared by all: the FDN | each pipe in through its region's diffusers (shared) and out by a 4-of-16-line projection: ~4 |
| **The swell box** | its pipes into a small box (its own short FDN), out through the shutters (a low-pass and gain driven by the shoe) as one or two region sources into the church | ~1 per pipe |
| **Silent pipes** | not computed; a silent pipe whose region's field is strong near its own frequency could be woken as a passive resonator (a few modes) to ring in sympathy | ~6 when awake |

- **~20 multiply-adds per sounding pipe and sample** plus a fixed ~500 for the regions and the
  network **[calc]**: 500 pipes ≈ 10 500 per sample ≈ 0.5 G MAC/s, about a third of a core. With
  the pipes themselves (§7: 20-40 ns cheap, 145 ns physical attack), a full tutti stays within 1-3
  cores, split over divisions on `src/parallel.h`'s threads.
- The coupling is block-delayed (0.67 ms, as now); near-field pairs within 1 m have a true delay of
  ~3 ms, so the block delay is within it. For synchronization between adjacent pipes (delay ~0.1-0.3
  ms at 3-10 cm) a sample-exact path is needed: neighbours on one chest region are stepped together
  in one job and couple sample by sample (as the synth circuits share a board) **[design]**.

### 7a.5 What it gives that a simpler organ lacks (and what is audible)

- **Unison ranks locking or beating:** the Principal 8′ and the Rohrflöte 8′ of the same key, a few
  cm apart, pull into one pitch (Mitnahme) or beat slowly when outside the tongue; mixture ranks
  fuse more firmly with the chorus **[paper; design]**.
- **The céleste's beating** differs by key and position (a little different where the Gamba and the
  céleste pipes stand closer), never a uniform LFO.
- **The chord spread in space:** consecutive semitones alternate C side / C# side: a chromatic run
  moves left-right, a chord is spread over the case; the pedal towers at the sides; the Positiv in
  front, closer and earlier.
- **The swell's enclosure:** the box's own resonance and the shutters' filtering, the swell sounding
  "from behind" when closed.
- **Sympathy:** a sustained note's pipe drives silent neighbours tuned near its harmonics (the
  "aura" of a big organ), and the 32′ shaking the room.
- Audible for sure: the spatial spread and the swell; likely: locking/beating of close ranks, the
  céleste's variation; subtle: sympathy of silent pipes (computed only when strong).

## 7b. The budget: 3 cores for the pipes, 1 core for the church (the user's)

*"I don't mind if the organ takes more than one core, 3 cores is ok, and one core for credible
'church' acoustics also fine."* One core is 20.8 µs per sample at 48 kHz, so the pipes get
**62.5 µs/sample** and the room **20.8 µs/sample** **[calc]**.

### 7b.1 What the pipes can keep with 3 cores [calc; design]

| Registration | Pipes sounding (with release tails) | Full physics for all (145 ns each) | Verdict |
|---|---|---|---|
| Soft accompaniment (SW 8′ + 8′, Ped 16′) | ~15-25 | 2-4 µs | full physics, all the time |
| Hymn plenum (HW 8-4-2⅔-2-V + SW + Ped) | ~45-70 | 7-10 µs | full physics, all the time |
| Grand plenum (HW 16′ to Cymbel + Ped 32′ to Mixtur IV + reeds) | ~120-180 | 17-26 µs | full physics, all the time |
| Full organ, couplers (sub/super) | ~300-450 | 44-65 µs | full physics just fits; with the room paths (§7a.4) over |
| Extreme tutti, 8-note chords + tails | ~500-700 | 73-100 µs | hybrid needed |

- **So:** every pipe keeps the full jet (or reed) model **for its whole life** up to ~350 sounding
  pipes; above that the **hybrid** of §7 applies, and only to the pipes that sound longest and are
  least exposed (the steady middle of mixture ranks first; never a pipe's attack, never the pedal,
  never a solo stop). The cheap form is then a fallback, not the design.
- **More of the couplings exactly (§7a):**
  - **neighbours:** k = 16 (all pipes within ~1 m on the same chest region, both rows of a key and
    the C/C# neighbours) instead of 6-8;
  - **sympathetic silent pipes:** the ~2-4 silent pipes nearest each sounding pipe whose pitch is
    near one of its harmonics run as passive resonators (the pipe's own column with the jet off: a
    few modes, ~10 ns): ~300 extra passive pipes ≈ 3 µs;
  - **early reflections per pipe** rather than per region: that is the room's budget (§7b.2).
- **The split across 3 threads (src/parallel.h WorkPool) [design]:** one job per **chest region**,
  so the pipes that couple sample by sample (§7a.4: the same region, distances of cm) are stepped
  together on one thread, as the synth circuits share a board now:

| Job | Region | Typical share of a plenum |
|---|---|---|
| 1 | Hauptwerk, C side | 25 % |
| 2 | Hauptwerk, C# side | 25 % |
| 3 | Schwellwerk (its box, both sides) | 20 % |
| 4 | Positiv (both sides) | 10 % |
| 5 | Pedal, left tower (C) | 10 % |
| 6 | Pedal, right tower (C#) | 10 % |

  Six jobs on three threads (the pool's heaviest-first order puts the two HW halves on separate
  threads). Between regions the coupling goes through the room, block-delayed (0.67 ms, as now:
  their distances are metres, 3+ ms, so the block delay is inside the true one). The wind
  (reservoir per division, §4) is updated per block too (its dynamics are 2-8 Hz).

### 7b.2 The church with one core

**What a credible church needs [memory; meas]:**
- **The decay:** the user's recordings end with a broadband T60 of 6.4-7.0 s (§9). Per octave, from
  `organ-9.wav`'s final release (one release only; a first estimate): 63 Hz 6.3 s, 125 Hz 9.5 s,
  250 Hz 6.1 s, 500 Hz 5.6 s, 1 kHz 6.2 s, 2 kHz 4.1 s, 4 kHz 3.6 s, 8 kHz 1.6 s **[meas]** (the
  other file's release is masked by a fade; step 0 refines both). A stone nave: long in the low
  mids, falling steeply above 2 kHz (air absorption over long paths and the absorbing congregation)
  **[memory]**.
- **The size:** a nave of ~15 000-30 000 m³ (e.g. 50 × 20 × 20 m). Sabine A = 0.161 V / T60 ≈
  370-740 m² at 6.5 s; the critical distance r_c = 0.057 √(V/T60) ≈ 2.7-3.9 m **[calc]**: a
  listener 20-30 m down the nave hears the reverberant field 15-20 dB over the direct sound; at the
  console (2-3 m) about equal.
- **The gallery:** the organ high at the west end (chests 6-10 m up); early reflections off the
  west wall behind the case, the vault, the side walls and the floor; the first reflections arrive
  within 10-40 ms and the vault's at ~40-80 ms **[memory; calc]**.
- **The low end:** the Schroeder frequency f_S = 2000 √(T60/V) ≈ 30-40 Hz **[calc]**: below it the
  nave is a set of separate modes, not a diffuse field. The pedal's 16-32 Hz (Untersatz 32′ C = 16.35
  Hz, the 16′ C = 32.7 Hz) sits there: a few room modes (the long axis's first modes ~3.4 Hz × n
  at 50 m) set how each low note blooms, differently per note.
- **Diffuse late reverb** with frequency-dependent decay (per band), dense enough to sound smooth
  (no flutter, no metallic ring) over 6-7 s.

**What `src/room.*` does now [read]:** a shoebox (2 : 1.4 : 0.7), the `church` at 15 000 m³ and
RT60 **4.0 s**, a 16-line FDN with one low-pass per line (shorter T60 at high frequencies), early
reflections to 2nd order per source and microphone (24 each), the listener 6 m from a line of
players, the direct sound of every player to every other (O(n²)).

**What to add within one core [design]:**

| Addition | Cost (per sample) |
|---|---|
| A `cathedral`/`nave` spec: ~20 000 m³, nave proportions (long: 2.5 : 1 : 1), T60 per octave fitted to the recordings (§9) | — |
| The FDN to 32 lines, each with a 3-band decay filter (low / mid / high shelving: the measured band T60s), longer lines for the low end | ~32 × 10 = 320 MAC |
| Early reflections **per pipe** (the user's requirement: every pipe its own place): image sources to 2nd order in the nave geometry with the gallery, ~24 per pipe per microphone pair, shared delay lines per region | ~48 × 300-500 pipes = 15-24 k MAC (~7-12 µs) |
| The low-frequency modes: the nave's modes below ~40 Hz as ~20-30 modal resonators driven by the pedal pipes' sources and read at the listener and at each pedal pipe (their feedback) | ~30 × (pedal pipes + 2) ≈ 1-2 k MAC |
| Listener positions: the nave (20-30 m) and the console; the pipes' feedback reads the field at each pipe's place (16 taps of the FDN per pipe: §7a.4) | ~16 × 500 = 8 k MAC (~4 µs) |
| **Total** | **~25-35 k MAC ≈ 12-18 µs: within one core (20.8 µs)** |

- Per-pipe early reflections take most of it; if a tutti exceeds the core, the farthest pipes from
  the listener (the high mixture ranks, the box's pipes behind the shutters) fall back to their
  region's shared early reflections.
- The room runs as its own job on the fourth thread, a block behind the pipes as now.

## 7c. Arithmetic: float32 or int16 on AVX-512 (measured, 2026-11-16)

The user asked whether the pipes could run in int16 rather than float32 (AVX-512 on this machine's
Ryzen 9 9950X3D, Zen 5: full 512-bit units, AVX-512 BW and VNNI). A 512-bit register holds 16 float32
or 32 int16 lanes: twice the lanes, half the memory. Pipes speak at a nearly steady level (their own
wind noise lies ~40-60 dB under them), so dynamic range is not the question; precision in the
recursions is. Prototypes in the scratchpad (`int16/acc.cc`, `wg*.cc`, `bench*.cc`) **[meas]**:

- **A pipe resonance as a recursive two-pole resonator (the modal form), Q 300: int16 fails.**
  Coefficients are not the problem (rounded to 2^-15 they move the pitch by a few cents; split into
  hi + lo halves, mostly less). The 16-bit state is: a Q-300 resonance changes by ~1e-4 of its
  amplitude a sample, finer than the state holds, and the rounding piles up inside the resonance:
  noise 4-44 dB under the signal (worst in the bass), against 57-95 dB in float32. First-order error
  feedback did not rescue it.
- **A pipe as a waveguide (its round trip a delay line, a low-order loss filter, a fractional-delay
  allpass): int16 works.** The delay line only stores samples (exact in int16); rounding happens
  once a sample in a low-Q filter. Against double, steady drive:

| Pipe | 16 Hz | 65 Hz | 262 Hz | 1 kHz | 4 kHz | 8 kHz |
|---|---|---|---|---|---|---|
| noise between the harmonics | -79 dB | -73 | -73 | -79 | -89 | -92 |
| difference at the harmonics (level, phase) | -74 dB | -61 | -53 | -42 | -30 | -20 |
| pitch | within 0.03 cents everywhere | | | | | |

  The noise is far under a pipe's own wind noise. The difference at the harmonics is deterministic:
  the loss coefficient g (close to 1 for the long-ringing high pipes) rounded to 2^-15 changes the
  pipe's damping a little: a slightly different voicing, not noise; computing the coefficients with
  their rounding in mind (or g alone in 32 bit) removes it.
- **Speed, vectorised along time** (one pipe, B consecutive samples at once: valid while the pipe's
  round trip is longer than B; a symmetric 3-tap FIR as the loss, so nothing in the block depends on
  itself; the clip of the jet's nonlinearity free in int16's saturating adds; the mix in int32):
  512 pipes 16 Hz-1 kHz: **float32 0.44-0.46 ns, int16 0.18 ns per pipe-sample: x2.46** (more than
  the lanes' x2: half the memory traffic). With 16 Hz-8 kHz the short (high) pipes run scalar in this
  prototype (round trip < 16 or 32 samples: 93 and 146 of 512), and the whole is only x1.5: the short
  pipes need their own layout (lanes ACROSS pipes, e.g. 32 high pipes of one rank side by side, their
  short delays in registers or a small transposed ring), the next prototype.
- **What it means for the organ:** the arithmetic is not where the cost is decided. These toy pipes cost
  0.2-0.5 ns a sample; the project's full flute model 145 ns. The physics kept per pipe (the jet's own
  delay and its noise, the mouth's coupling to the room and the neighbours, the foot's wind) decides
  the cost; int16 then gives a further ~2-2.5x on what vectorises. Practical split:
  - **int16:** the pipes' delay lines and their low-order filters, the jet's nonlinearity (a table or
    a polynomial), the steady state of every rank; the mixing and the couplings accumulate in int32;
  - **float32 (or int32):** anything recursive with high Q (the room's modes below 40 Hz, the wind
    system's reservoirs and trunks: few, cheap), the per-pipe parameters as the wind sags (computed per
    block, quantised into the int16 coefficients), the room's late field.
  - Check before relying on it: the quiet first milliseconds of a pipe's speech (Castellengo's
    precursors at -30..-40 dB) still ~50 dB over the int16 floor; small pitch changes from the wind
    (0.1 cent) resolvable through the allpass coefficient (Q15: ~0.03 cent at worst measured).
- **Multirate: each resonance at its own octave's rate (the user's idea) makes int16 resonators work.**
  The trouble above is the per-sample change (a resonance decays by pi f / (Q fs) a sample: 1.4e-5 for
  65 Hz at Q 300 and 48 kHz, under the state's 3e-5 step). At the lowest of the rates 48000 / 2^k
  that gives a mode at least 6 samples a period, its change is 100-1000x larger, its rounding is
  amplified ~+25 dB instead of +45, and the coefficients' rounding no longer shows in cents
  (`int16/multirate.cc`, Q 300, against double at the same rate) **[meas]**:

| Mode | 16 Hz | 33 Hz | 65 Hz | 262 Hz | 1-8 kHz |
|---|---|---|---|---|---|
| its rate | 188 Hz | 375 Hz | 750 Hz | 3 kHz | 12-48 kHz |
| int16 noise, multirate | -67 dB | -64 | -66 | -69 | -62 .. -69 |
| int16 noise, all at 48 kHz | -15 dB | -17 | -34 | -57 | the same |

  Pitch exact to 0.004 cents. About 6 samples a period is the sweet spot (at 12 the bass falls back to
  -49 dB). **The user accepts ~-62 dB for a pipe** (2026-11-16): it is under the pipe's own wind noise.
  - It is per mode, not per pipe: a 16 Hz pipe's harmonics reach kHz, each in its own octave band.
  - Cost: a 65 Hz pipe with 32 harmonics ~7x cheaper than every mode at 48 kHz, before int16's 2x
    lanes; the higher pipes gain less.
  - It vectorises perfectly: every mode of one band, of any pipes, runs at the same rate and is
    independent: 32 modes a register, no delay lines, no gathers (the waveguide's short treble pipes
    are no problem here).
  - The bands are joined by one interpolation chain (x2 a stage, high quality, its delay irrelevant for
    an organ) per output or room-source group, not per pipe.
  - **The catch: the pipe's self-oscillation crosses the rates.** The jet's (reed's) nonlinearity runs at
    full rate on the summed mouth pressure; the resonances at lower rates are reached through
    decimation and interpolation, and that delay sits inside the loop. Flue pipes: the jet's own
    travel time (~half a period of the fundamental: 7.7 ms at 65 Hz) can absorb it if the in-loop filters
    are short (linear interpolation up, a short average down: ~0.25-0.33 period in the fundamental's
    band) and their delay is taken off the jet's; their images stay inside the loop (the output path
    keeps high-quality filters). Reed pipes have no such budget: their reed and lowest modes at full
    rate, the upper modes multirate; or multirate only for the steady state after the attack.
  - Next test: a self-oscillating flue pipe (jet + modes) with its loop across the rates against a
    full-rate double one: the attack, pitch, spectrum, and two pipes locking (Mitnahme).
- **Dynamic (adaptive) just tuning, wanted later by the user (2026-11-16).** Each sounding pipe retuned
  to pure intervals within the chord it sounds in, as the project's flute ensemble already does
  (`Ensemble::retune`, the just intonation among the sounding voices, healing back to equal
  temperament), rather than the harpsichord's fixed just scale on the song's tonic. What the design must
  keep possible:
  - **Retuning a pipe = rotating its modes' coefficients**, all of its modes by the same ratio (its
    slight inharmonicity kept), per block, smoothly (a glide of ~50-300 ms, or only at a new chord's
    onset, so a held chord does not audibly slide). The coupled (rotation) form keeps a ringing mode's
    amplitude as its angle changes: no click. In int16 at ~6 samples a period the coefficient's step is
    ~0.004 cent: fine enough. A retune of a few to ~15 cents never moves a mode out of its octave
    band (the bands' margins are a factor of 2).
  - **The pitch is the loop's**: the jet (reed) and the resonances together set it, so the retune acts
    through the resonances (and the jet's travel time with them), not by resampling the output.
  - **The solver works on keys and couplers, not pipes:** a chord's notes (after couplers and the
    pedal) get their just ratios; every rank's pipe of a note follows its note (mixtures and mutations
    keep their own pure ratios to it, which is what they are voiced for: a Quinte 3/2, a Terz 5/4
    above its octave).
  - **Interactions to check:** two pipes locking (Mitnahme) when one is retuned near the other; the
    céleste's beating (its detuning must survive the retune: retune both ranks together); the wind's
    droop on top (it adds a common offset, not a new interval); the comma drift of long progressions
    (anchored to equal temperament as the ensemble's healing does).
- **(The voice, puhuja.)** A singer's tract has a wide dynamic range, time-varying high-Q formants and a
  breathy source; that is the recursive, high-Q case that fails above. The organ differs because its
  resonators can be waveguides at a steady level. A waveguide (Kelly-Lochbaum) tract might fare
  better and could be tested the same way.

## 7d. Prototypes: the self-oscillating pipe across rates, and the reverb in two rates (2026-11-16)

Standalone C++ in the scratchpad (`int16/mr.cc`, `cost*.cc`, `reverb.cc`; analysis `ana.py`,
`lock.py`, `hq.py`); not the project's code. Ryzen 9 9950X3D (Zen 5, AVX-512), one thread.

### 7d.1 A flue pipe whose loop crosses the rates [meas]

The pipe [design]: a jet whose deflection is G x (the mouth signal tau ago) + a bias, its flow
tanh of it, driving 24 harmonic modes (upper partials a little flat; Q 60 sqrt(k) / (1 + (k/12)^2);
input weight k^-0.5); the mouth signal = the modes' sum; jet speed ~ sqrt(wind), so tau ~ 1/sqrt(wind)
and the drive ~ sqrt(wind); a little turbulence noise. tau = 0.7 of the period, G 8 (65 Hz) / 4
(262, 1047 Hz). It speaks on its fundamental at all three; harmonics 2-3 at -20..-24 dB (a mild
principal; its attack is slow, 0.2-1 s: not voiced yet).

Multirate: each mode at the lowest of 48000/2^k with >= 6 samples a period; the jet and its tanh at
48 kHz; the drive decimated to each band (boxcar, or triangle) and each band's output interpolated
back (linear, or cubic) inside the loop; the bands' delays equalised to the slowest and taken off the
jet's tau (it fits: the slowest band's loop delay is 96 samples at 65 Hz against tau 514; 24 against
128 at 262 Hz; 5.5 against 32 at 1047 Hz); each mode's input corrected for the filters' gain at its
frequency. int16 = Q15 coupled-form modes; "mrd" the same in double.

| | 65 Hz: ref | int16 MR | 262 Hz: ref | int16 MR | 1047 Hz: ref | int16 MR |
|---|---|---|---|---|---|---|
| pitch (vs nominal) | +8.70 c | +8.99 c | +6.02 c | +6.53 c | +5.59 c | +7.81 c |
| harmonics 2-6 vs ref | | 0, -0.3, -5.4, -1.3, -3.0 dB | | -0.5, -0.8, -1.3, -1.6, -2.2 | | -1.2, -1.5, -4.4, -4.1, -9.1 |
| attack (to 90 %) | 1084 ms | 1086 | 682 | 680 | 199 | 183 |
| wind -8 %: pitch | -5.26 c | -5.45 c | -2.61 c | -2.70 c | -2.54 c | -3.12 c |
| retune +15 c (modes rotated over 100 ms) | +12.0 c | +11.8 c | +14.4 c | +14.3 c | +14.4 c | +14.3 c |
| click at the retune (2nd-difference energy) | +15.3 dB | +16.2 | +3.2 | +0.9 | +1.3 | +0.7 |
| floor between harmonics, in-loop signal | -31.1 dB | -26.0 | -33.0 | -17.5 | -42.2 | -12.0 |
| floor, output through high-quality interpolation | | -31.9 | | -32.2 | | -36.0 |

- **The loop works across the rates**: pitch within 0.3-2.2 c, the attack, the wind's droop and the
  retuning alike. The retune falls short at 65 Hz (+12 of +15 c) in both: only the modes were turned;
  the jet's tau must turn with them (§7c's retuning: "the jet's travel time with them").
- **int16 changes nothing measurable against double** (mrd = mri within 0.1-0.5 c and ~0.3 dB).
- **The harmonics drift a little** (1-5 dB, the 1047 Hz pipe's 6th by 9 dB): the in-loop filters'
  images and gain errors near a band's top. Correctable by voicing (each mode's input weight).
- **The in-loop interpolation's images** raise the mouth signal's floor (to -12..-26 dB), but they
  stay in the loop: the sound taken through a high-quality interpolation outside it has the
  reference's floor (-32..-36 dB, set by the jet's turbulence). So: cheap filters inside the loop,
  good ones on the output (their delay does not matter, §7c).
- **Longer in-loop filters** (triangle down, cubic up) were no better (harmonics alike or worse at
  1047 Hz) and doubled the loop delay: the shortest are the choice.
- **Locking (Mitnahme)**, two pipes 3, 6, 10 c apart, each jet also hearing kappa x the other's mouth:

| 262 Hz | kappa 0.15 | 0.3 | 0.6 |
|---|---|---|---|
| reference | 3 c pulled to 1.1; 6, 10 free | 3, 6 locked; 10 free | all locked |
| int16 multirate | 3 locked; 6, 10 pulled a little | all locked | all locked |

  At 65 Hz both are irregular below kappa 0.6 (the low pipe's slow attack); at 0.6 the reference locks
  3 and 6 c, the multirate all three. The multirate pipe locks about twice as readily: its coupling
  strength has to be calibrated per form (against the published locking ranges, §7a.2).

### 7d.2 Cost [meas]

A rank of 256 pipes x 24 modes over five octaves, the jets included (16 lanes, a gather for each
pipe's tau, a rational tanh); lanes across pipes in both; the mouths' history stored [time][pipe]:

| rank from | float32, every mode at 48 kHz | int16 multirate, octave pyramid | ceiling (the mode work alone) |
|---|---|---|---|
| 16 Hz | 3.03 ns per pipe-sample | 2.55 (x1.2) | x6.6 |
| 33 Hz | 3.01 | 3.06 (x1.0) | x4.3 |
| 65 Hz | 3.09 (4.1 in another run) | 3.00-3.3 (x1.0-1.3) | x3.4 |
| 262 Hz | 3.73 | 3.69 (x1.0) | x2.4 |

- **The modal pipe itself is cheap**: ~3-4 ns a pipe-sample in plain float32, ~40x under the
  project's flute model (145 ns, §6). One core runs ~5000-7000 such pipes; the 3-core budget is no
  longer tight for the pipes themselves (per-pipe room coupling and early reflections come on top:
  §7a.4, §7b).
- **Multirate gains little as built** (x1.0-1.3): the jets and the pyramid run at 48 kHz for every
  pipe (30-45 % of the multirate time), the bands are padded to their busiest pipe (56 % empty
  slots), and a rank's high pipes keep their harmonics at 24-48 kHz. The first version, each band
  decimated and interpolated on its own from 48 kHz, was even slower (x0.6-0.7): the pyramid
  (each stage halving / doubling once) is required.
- The ceiling is x2.2 (a treble rank) to x6.6 (a 16' rank): reaching it needs dense packing (any
  pipes' modes side by side in the lanes, their inputs by permutes), and pays most in the bass.
- **So**: int16 multirate is accurate enough (accepted) and works in the loop, but for cost it is a
  later optimisation for the low ranks (16', 32'); the plain float32 modal pipe at 48 kHz is the
  first build.

### 7d.3 The reverb in two rates [meas; design]

The late field: a 16-line FDN, Hadamard mixing, each line's absorption an octave graphic EQ fitted
to the nave's T60s (organ-9.wav, §7b.2). (a) all at 48 kHz; (b) the band under 5 kHz (a 127-tap
low-pass, decimated x4) in the same FDN at 12 kHz (octaves 63 Hz-4 kHz), the band above it (the
input less its low part) in a 4-line FDN at 48 kHz (11-24 ms lines: the short, air-damped highs),
the low FDN interpolated x4 with the same filter.

| octave | 63 | 125 | 250 | 500 | 1 k | 2 k | 4 k | 8 k | 16 k |
|---|---|---|---|---|---|---|---|---|---|
| target T60 (s) | 6.3 | 9.5 | 6.1 | 5.6 | 6.2 | 4.1 | 3.6 | 1.6 | (0.8) |
| 48 kHz FDN | 6.10 | 8.58 | 5.72 | 5.33 | 5.67 | 4.02 | 3.48 | 1.85 | 2.32 |
| two rates | 6.65 | 8.60 | 6.09 | 5.64 | 5.99 | 4.41 | 4.06 | 1.84 | 2.31 |

- Both within ~10 % up to 8 kHz; the 12 kHz FDN runs long near its top (4 kHz +13 %: the EQ's last
  band near its Nyquist; refit there); 16 kHz too long in both (the octave EQ cannot reach it: a high
  shelf instead).
- Late echo density alike (0.98 / 0.99 of Gaussian at 0.3-1 s); the fine spectrum alike (sd 6.4 /
  6.7 dB over 200 Hz-4 kHz, the largest peak 17 dB in both: no new metallic ring). The early part
  (50-150 ms) is sparse in the 48 kHz FDN (0.17) and denser with the high FDN (0.51): both want
  diffusion (allpasses) in front.
- The high band came out ~6 dB too strong against 0.5-4 kHz (a gain to calibrate).
- Cost (scalar double, unoptimised): 147-173 ns a sample at 48 kHz; the 12 kHz FDN 38 ns + the high
  FDN 39 ns (its nine-octave EQ is overkill: two bands do) + the 127-tap filters (run naively here;
  as polyphase ~32 multiply-adds a sample each way, a few ns vectorised). Memory 43 500 delay samples
  against 10 900 + 3 400. So x2.5-3 on the late field, x3 less memory.
- **In absolute terms the late field is cheap anyway** (~0.15 µs a sample against the core's 20.8):
  the room's cost is the per-pipe early part (§7a.4, §7b.2). There the same split pays: **at 48 kHz**
  each pipe's direct sound and its first reflections (~20-50 ms: their timing and highs make the
  image); **at 12 kHz** the later reflections (50-200 ms, mostly under 4 kHz) and the late FDN, which
  lets the same budget hold 2-4x the lines (a smoother, denser low end); **at a few hundred Hz** the
  nave's modes under 40 Hz (20-30 resonators, nearly free); the highs above ~5 kHz as one short,
  fast-decaying field shared by all.

### 7d.4 Early reflections: discrete images to second order, lateral, band-shaped (the user's) [meas; design]

The user's preference: the direct sound (the real image), the first-order images (mirror images:
a side wall flips it left-right), the second-order images (real again), weighted to the side walls;
beyond that the diffuse field. Each reflection band-shaped, about **300 Hz to 8 kHz** (-3 dB edges),
best at 1-4 kHz, more so at second order; and **each image a discrete, delayed copy at its own
geometric time**, so that transients stay sharp (no FIRs, diffusers or allpasses on the images).

- **`src/room.*` now** [read]: a shoebox 2 : 1.4 : 0.7; images to second order off all six surfaces
  (24 + the direct, per source and microphone), 1/r, delayed r/c, each wall a frequency-independent
  sqrt(1 - alpha) (side walls 0.4x, ends and ceiling 0.8x, the floor 2.5x the mean); no air
  absorption; direction only through two spaced, angled microphones; `lateral()` reports the first
  side reflections' time and right-left level. The order is already the user's; missing: the
  band-shaping, the air, per-image direction, the church's geometry.
- **Surfaces by octave** [memory: typical published values, to be checked]: (absorption)
  stone / plaster 0.02 / 0.02 / 0.03 / 0.04 / 0.05 / 0.07 (125 Hz-4 kHz); windows 0.35 / 0.25 / 0.18 /
  0.12 / 0.07 / 0.04; wood panelling 0.30 / 0.25 / 0.20 / 0.17 / 0.15 / 0.10; occupied pews 0.40 /
  0.55 / 0.70 / 0.80 / 0.80 / 0.80; empty pews 0.10 / 0.15 / 0.20 / 0.25 / 0.30 / 0.35. The mirror
  image also loses what rough stone, piers and mouldings scatter into the diffuse field (s ~0.05 at
  125 Hz to 0.5 at 8 kHz), and in the bass it is less specular (a broken, finite wall; its Fresnel
  zone) [design]. A side wall of 70 % stone, 20 % windows, 10 % panelling then reflects (dB, 63 Hz-
  8 kHz) -3.7 -2.2 -1.3 -0.9 -1.3 -1.8 -2.5 -3.4 [calc]: milder than the user's band at the bottom
  (-0.4 dB at 250 Hz against their -3 dB at 300); the user's band is taken (the bass term is ours).
- **The image filters, chosen**: per reflection a one-pole high-pass at 300 Hz and a one-pole
  low-pass at 8 kHz (minimum-phase, 4 multiply-adds), once per reflection; each surface's broadband
  loss (at 1 kHz: side walls -1.3 dB, west -1.5, ceiling -1.2, floor (pews) -4.2) and the air of the
  path (ISO 9613-1, ~0.1 dB/m at 8 kHz) on top:

| octave (Hz) | 63 | 125 | 250 | 500 | 1 k | 2 k | 4 k | 8 k | 16 k |
|---|---|---|---|---|---|---|---|---|---|
| first order (dB) | -13.7 | -8.3 | -3.9 | -1.4 | -0.4 | -0.4 | -1.0 | -3.0 | -7.0 |
| second order (dB) | -27.5 | -16.6 | -7.8 | -2.7 | -0.9 | -0.7 | -2.0 | -6.0 | -14.0 |

**Revised (the user, 2026-11-16): steeper at the top.** Every real wall has decorations and
unevenness (mouldings, capitals, carvings, rough stone) that scatter the highest frequencies out of the
specular reflection into the diffuse field, and the air carries them poorly. So the image's own band
filter (before air absorption) is to fall to **-10 dB at 16 kHz in first order and -20 dB in second**,
not -7 / -14; air absorption is added on top per path (ISO 9613-1: roughly 0.2-0.5 dB/m at 16 kHz,
0.05-0.1 dB/m at 8 kHz, by humidity: another ~6-20 dB at 16 kHz over a 30-40 m path). The low-pass of
the image filter therefore becomes steeper than a one-pole (e.g. a one-pole at 8 kHz plus a high shelf
of about -4 dB above ~11 kHz, still minimum-phase and short: its spread stays well under 0.1 ms), and
what the specular image loses at the top goes into the late field's input (the scattered share), so the
energy is kept, only diffused. The per-image levels in the table below were computed with the -7 / -14
filter at 8 kHz and below they are unchanged; at 16 kHz they fall by a further ~3 / ~6 dB.

| octave (Hz) | 8 k | 16 k |
|---|---|---|
| first order, revised (dB) | -3.0 | **-10** |
| second order, revised (dB) | -6.0 | **-20** |

**The church's air (the user, 2026-11-16: relatively high humidity).** Historic Central European
churches: unheated 70-85 %+ RH, intermittently heated 45-75 %, climate-controlled 50-65 % (the user's
summary). Design condition, the user's choice: **58 % RH at 20 °C** (a heated, climate-controlled church;
at 58 % the temperature matters: 12 °C and 15 °C would give the air alone an 8 kHz T60 of 1.4 and 1.5 s,
under organ-9.wav's measured 1.6 s; at 20 °C the air allows 1.9 s). ISO 9613-1 pure-tone
absorption **[calc]**:

| condition | 1 k | 2 k | 4 k | 8 k | 16 k (dB/m) |
|---|---|---|---|---|---|
| 20 °C, 50 % (a heated hall) | 0.005 | 0.010 | 0.030 | 0.105 | 0.365 |
| **20 °C, 58 % (design)** | 0.005 | 0.009 | 0.026 | 0.092 | 0.326 |
| 18 °C, 58 % | 0.004 | 0.009 | 0.028 | 0.100 | 0.347 |
| 12 °C, 58 % | 0.004 | 0.011 | 0.036 | 0.129 | 0.392 |
| 12 °C, 75 % | 0.004 | 0.009 | 0.028 | 0.102 | 0.343 |
| 10 °C, 85 % | 0.004 | 0.008 | 0.027 | 0.099 | 0.333 |

- In this range humidity hardly matters: the cooler air absorbs a little less at the top and that cancels
  the extra moisture's effect (above ~30 % RH more moisture absorbs less). At the design condition, over a 35 m image path: ~3.2 dB at 8 kHz,
  ~11 dB at 16 kHz, on top of the image filter above.
- **The air alone caps the late field's T60** (Sabine's air term, any room; at the design condition):
  ~6.6 s at 4 kHz, **1.9 s at 8 kHz, ~0.5 s at 16 kHz**. organ-9.wav's 1.6 s at 8 kHz sits just under that: that church's top is set by
  its air, a check on the measurement. The FDN prototypes' 2.3 s at 16 kHz (§7d.3) is impossible: the late
  field's 16 kHz T60 must come out ≤ 0.5 s (the high shelf mentioned there).


- **The church and a pipe** [calc]: the nave 50 x 20 x 20 m (20 000 m^3); a pipe at the west gallery
  (2.5 m from the west wall, 3 m north of the axis, its mouth 9 m up); the listener in the nave (27 m
  from the west wall, 2 m south, ears 1.2 m), facing west. Azimuth + = to the listener's right
  (north). Levels re the direct sound, with the image filters, the walls and the air:

| image | order | delay (ms) | 125 Hz | 1 kHz | 8 kHz | azimuth | lateral | mirror |
|---|---|---|---|---|---|---|---|---|
| direct | 0 | 0 | 0 | 0 | 0 | +12 | | |
| floor | 1 | 2.4 | -12.8 | -4.9 | -7.6 | +12 | | |
| west | 1 | 13.8 | -11.2 | -3.4 | -6.4 | +10 | | |
| west-floor | 2 | 15.8 | -23.9 | -8.2 | -13.9 | +10 | | |
| north | 1 | 16.8 | -11.3 | -3.5 | -6.6 | +38 | yes | yes |
| north-floor | 2 | 18.8 | -24.0 | -8.3 | -14.1 | +38 | yes | yes |
| south | 1 | 20.4 | -11.7 | -3.8 | -7.1 | -41 | yes | yes |
| south-floor | 2 | 22.3 | -24.3 | -8.6 | -14.5 | -41 | yes | yes |
| west-north | 2 | 28.4 | -22.2 | -6.5 | -12.6 | +33 | yes | yes |
| west-south | 2 | 31.6 | -22.4 | -6.7 | -12.9 | -35 | yes | yes |
| ceiling | 1 | 37.0 | -12.9 | -5.1 | -8.9 | +12 | | |
| ceiling-floor | 2 | 42.5 | -25.9 | -10.2 | -16.7 | +12 | | |
| west-ceiling | 2 | 46.8 | -23.5 | -7.8 | -14.5 | +10 | | |
| north-ceiling | 2 | 49.0 | -23.4 | -7.8 | -14.5 | +38 | yes | yes |
| north-south | 2 | 50.3 | -23.6 | -8.0 | -14.7 | -55 | yes | (real) |
| south-ceiling | 2 | 51.7 | -23.6 | -8.0 | -14.8 | -41 | yes | yes |
| south-north | 2 | 74.7 | -25.1 | -9.5 | -17.1 | +61 | yes | (real) |
| floor-ceiling | 2 | 80.9 | -28.3 | -12.7 | -20.5 | +12 | | |
| (east wall and its pairs) | 1-2 | 131-288 | | -9 .. -14 (1 kHz, without the band) | | ~180 | | |

  The side walls give the lateral images at +38 / -41 degrees within 17-20 ms, their second-order
  pairs (with the floor, the west wall, the ceiling, each other) through 75 ms, the two side walls
  in sequence (north-south, south-north) real images far out at -55 / +61 degrees.
- **The blend into the late field** [calc]: at this distance (25 m; the critical distance is 3-4 m)
  the statistical diffuse energy per 10 ms is about the direct's (+0.9 to -0.0 dB re direct over
  0-100 ms at 1 kHz, Eyring, T60 6 s). The exact images fill 10-30 ms at that level (+0.8, -1.1 dB),
  thin out to -5 dB by 50-60 ms, and stop. The late field must therefore fade in under them:
  ~35 % of the diffuse energy at 20-30 ms, ~55-70 % at 30-60 ms, all of it from 60-70 ms. With that
  ramp (its onset envelope, a design parameter of the FDN's injection), the sum follows the
  statistical curve: no gap, no double hump. The images after ~80 ms (the east wall, 131-288 ms,
  ~9 dB under the diffuse field by then) are dropped: masked, and they would only cost.
- **Transients** [meas, `images` prototype]: a click through the stage (fractional delays by
  2-tap linear interpolation) shows each of the 18 images as its own sharp arrival at the computed
  time, all within 0.01 ms (one sample). The image filters spread an impulse by **0.02 ms** (first
  order) and **0.12 ms** (second order) for 90 % of its energy; their peaks lie at 0 ms (minimum
  phase). Diffusion only in the late field.
- **Cost** [meas, `taps3.cc`]: per source one delay line holding a few band-shaped copies (the
  direct; first and second order, each near and far for the air: 5 copies, 4-6 one-poles), and
  per image one tap (a gather and two multiply-adds, left and right): 512 sources x 18 images, the
  filters across sources and every tap a 16-lane gather: **~15 ns per source-sample** (memory-bound:
  5 x 4096 x 512 floats = 42 MB of rings; int16 rings or shorter rings would help). For 512 sounding
  pipes ~7.7 µs a sample, ~37 % of the church's core; the late field ~0.15 µs. Layouts that wrote the
  rings sample by sample per source were 4x slower (55-59 ns). Where the budget says so (an
  extreme tutti), the pipes of a chest region (within ~0.5 m: the images' times within ~1.5 ms)
  share one set of images.

## 7e. The design against a checklist of what makes an organ sound good (the user's, 2026-11-16)

The user pasted a general checklist (room acoustics, placement and projection, tonal design and voicing,
climate and maintenance). How the design stands against it, and what it adds **[design]**:

| The checklist | In this design | Added or to decide |
|---|---|---|
| Reverberation 2-4 s, notes blending into a "glowing wall of sound" | the nave fitted to organ-9.wav: 5.6-6.2 s at 500 Hz-1 kHz, 9.5 s at 125 Hz (§7b.2) | **decided (the user): match the recordings.** Churches range from ~2 to ~15 s, and "the" reverberation time depends on which band is meant (a single figure is usually the 500 Hz-1 kHz mean, the bass often far longer, the treble capped by the air): so the room is defined by its **T60 per octave**, fitted to a recording, never by one number; the default is organ-9.wav's nave, and other churches come from other recordings' per-octave decays (within the air's caps at 4-16 kHz, §7d.4) |
| Hard surfaces (stone, plaster, hard wood) reflect; carpets, cushioned pews and tapestries drain the sparkle | the image filters (300 Hz-8 kHz, best 1-4 kHz, -10/-20 dB at 16 kHz) and the air at 20 °C / 58 % (§7d.4) | a per-surface choice (stone nave, wooden ceiling, a carpeted or upholstered floor: its images' highs lower) as room parameters, not one absorption for all |
| Unobstructed projection; not in an alcove or behind arches (up to 30 % lost) | the west gallery, facing the nave (§7a.1) | an optional arch or alcove: a low-pass and a shadow on the direct sound and the first images from that side |
| Elevation: early ceiling reflections spread the sound evenly | the images to second order include the ceiling and ceiling-lateral pairs (§7d.4) | keep the pipes' height in the geometry (3-4 m chests, the pedal towers lower), so the ceiling images arrive as in a real gallery |
| A shallow wooden case blends the pipes before the sound leaves it | not yet | **the case as the first reflector:** its back and sides (wood, 0.5-1 m behind and beside the pipes) as images at 1-6 ms, wood's own colour (a little lower in the highs than stone), the case's resonances weak; per chest region, cheap (a few taps per source) |
| Voicing (scaling, cut-up, tongues) decides whether it sings or is harsh; the attack, volume and timbre per rank | the per-rank pipe design and the voicing solver (step 2-3 of the plan) | the attack's "speech" as a voicing parameter per rank (the precursors, the chiff), set by ear |
| Balance: a chorus, not every stop at once | the registrations (§6) | the presets themselves (hymn plenum, soft accompaniment, trio, French tutti) are the balance |
| Electronic organs: multi-channel, speaker placement, room mapping | the per-pipe positions and images (§7a, §7d.4) | stereo now; binaural (head-related filters for each image's direction) as an option |
| Temperature and humidity stability: wood swells, pipes drift out of tune | the design condition 20 °C, 58 % | **temperature as a parameter:** flue pipes rise ~3 cents per °C (the speed of sound, ~0.18 %/°C), reed pipes barely move (their tongues set the pitch): tuned at 20 °C, a church at 15 °C has its flues ~15 cents flat against its reeds. An optional "seasonal" detuning |
| Tuning once or twice a year | the ranks tuned exactly (or justly, §7c) | an optional small per-pipe mistuning (±1-2 cents, fixed per pipe, from a seed): an organ half a year after its tuning; it also enriches the unisons' beating |
| Dust alters speech, leathers age | out of scope | (could be a "neglected organ" preset: slower, breathier speech; leaks) |

## 8. Build plan

1. **Step 0 — targets [meas, §9]:** from the user's organ recordings, measure a few isolated
   notes (attack time, which partials first, steady spectra of a principal, a flute, a reed),
   the room's T60, tremulant rate if present. Also Castellengo's transient numbers (§2.1) as checks.
2. **One rank, physical:** a Principal 8′ on the flute engine (`src/flute.cc` geometry from a
   scale: diameter by Normalmensur, mouth width 1/4 of circumference, cut-up ~1/4 of mouth width,
   foot pressure 75 mm WC) — all 61 keys voiced by a solver (the calibration's role: each pipe's
   cut-up/flue so it speaks its fundamental in ~30–80 ms). Check: in tune, the steady spectrum of
   §2.1, the attack's mouth tones, cost per pipe.
3. **The families** from one parametrized pipe: open (principal, octave, gemshorn conical),
   stopped (Gedackt, Bourdon, Subbass), chimney (Rohrflöte), harmonic (Flûte harm./octaviante),
   string (Gamba, céleste detuned +3 c), mutations and mixtures (rank tables with breaks); reeds
   on `src/reed.*` with conical/cylindrical/short resonators (Trompete, Posaune, Oboe, Cromorne,
   (no Vox humana: not wanted)). Check each against §2.1/§3 and the recordings.
4. **The cheap form** (§7; with the 3-core budget only a fallback above ~350 sounding pipes, §7b.1)
   once a family sounds right: verify by A/B that the hybrid (physical
   attack, then modal) is indistinguishable at < 1/4 of the cost.
5. **The wind (§4a):** the blower's curve, the main reservoir and each division's regulator
   (mass-spring-damper tops with inlet valves), the trunks (inertance and compliance), the chests;
   every pipe's flow drawn from its chest and its foot pressure read back each sample; the
   tremulant as a vent valve in the network; the swell box as shutters (level + low-pass + a
   little room). Checks (§4a.5): one stop barely moves the wind; a hymn chord −1…−2 %, ~1 c,
   recovering in ~100 ms; ordinary plena below ~2 %; the full organ with couplers and a 10-note
   chord sags **8 % ± 2** and flues droop **4 ± 1 c** (the user's target), slower to speak, with
   at most a small wobble after the release (stiff, damped regulators); the tremulant 4-7 Hz,
   deepening with demand.
5b. **Every pipe in the room (§7a):** the case layout (§7a.1) as data per pipe (x, y, z of mouth and
   top, its region); the room restructured for hundreds of sources (§7a.4: direct and neighbour
   paths per pipe, early reflections per region, the shared field, the swell box), each pipe fed
   back its pressure (flue: at the mouth; reed: at the resonator top). Checks: two adjacent flue
   pipes 0-10 c apart lock (a measured Arnold tongue against their spacing, qualitatively as Abel
   et al. 2006 / Sawicki et al. 2018: wider when closer; anti-phase or silence when equal and very
   close); the céleste's beat rate varies by key; a chromatic run alternates sides; the cost of a
   500-pipe tutti under 1 core for the room.
6. **The console:** 35 stops, couplers, presets (§6.1), the 25-key MIDI controller mapping (§10), and `-play`
   (GM 20) with a registration and the swell on CC11.
6b. **The church (§7b.2), on its own core:** the nave spec (~20 000 m³, T60 per octave fitted to
   the recordings), a 32-line FDN with 3-band decay, early reflections per pipe in the nave with the
   west gallery, the low-frequency modes for the pedal, nave and console listener positions.
   Checks: band T60s within 10 % of the recordings' (step 0), the direct-to-reverberant ratio at
   20-30 m (-15…-20 dB), the room under one core with a 500-pipe tutti.
7. **Room and layout:** divisions placed on the stage (Positiv front, pedal sides, swell back),
   C/C# sides; the church room; the Untersatz 32′'s lowest octave (16-31 Hz) checked through the
   room model and the output (it may need the room's low end extended).
8. **Tests** in `tools/test.py`: tuning per rank, each family's spectrum bands, attack time
   windows, wind sag numbers, tremulant rate, real-time margin of a full plenum.

Cost of the work: steps 2–3 are the bulk (a pipe voicing solver and ~19 families); the wind and
console are smaller. Each step ends with something to hear.

## 8a. What is built (2026-11-16) [meas: the project's own renders; GrandOrgue's demo set, numbers only]

All 37 stops of §6, and then the rest of the user's disposition (53 in all: everything but the Vox
humana), `-inst organ` with `PJ_ORGAN_STOPS` (stops and named registrations: README).
References: GrandOrgue's demo sample set (GPL-2+ / CC-BY-SA; Bourdon 8′, Montre 8′, Gamba 8′,
Flûte harmonique 8′, Octavin 2′, Trompette 8′, Hautbois 8′; F#/G of each octave), decoded in the
scratchpad only, measured for harmonics, speech, onset and noise.

- **The flue pipe** (`src/organpipe.h`): the jet (its travel a quarter period, a tanh of its
  deflection by the mouth's flow, its turbulence) driving the column's modes; the sound from the
  mouth and, for an open pipe, its top (half a wavelength up; the odd modes in phase, the even ones
  opposite: a listener below the gallery hears the octave). Found on the way:
  - the jet's own flow radiated directly made a buzz of upper harmonics (-30 dB to the 9th):
    low-passed (4 poles at 2x the pitch) it gives the even harmonics without it;
  - an open pipe's octave needs an asymmetric jet: the languid's offset set against the jet's swing
    (a fixed offset vanishes against a saturated jet's swing); a voicer's octave weight on the even
    modes; strings centred (the duty near 1/2 keeps the 3rd), their bass bridged;
  - the jet hears the field ~ pressure / frequency (the acoustic velocity): else the treble's pipes
    hopped between regimes in their boxes;
  - the smallest pipes (above 2.5 kHz) oversampled 2-4x, their jets deflected by flow / mouth width;
  - the chiff: the first overtone driven harder at the attack (fading over 25-60 ms): the Montre's
    octave leads by -3..+10 dB, ours 0..+9; the Bourdon's twelfth +2..+6, ours +1..+4.
  - the small pipes' chiff (2026-11-17; our own renders, key by key, G5-F#6 and C6 struck six times):
    it was one setting from the 8' (fading from the key's fall over >= 25 ms, so mostly spent before a
    small pipe spoke; on the octave's steady weight, so an open flute's, its octave ~-30 dB, did
    nothing: its leads the same with the chiff off), the stopped 4' top x3 (its twelfth leading
    +14..+20 dB for ~50 ms, up to +50: overblown), each attack alike (a flute's within 0.1-0.3 dB).
    Not too loud otherwise: the octave's peak 15-27 dB under the held tone. (The attacks that are
    +7..+15 dB over their held level, a few swell flutes' keys, e.g. the Flöte 4' C6, are pipes whose
    held level sits 10-15 dB under their neighbours' in the box, not their chiff: open.) Now, from C6
    (fully from C7; the 8' and 4' middle and the strings untouched): its time ~0.03 sqrt(262 / f) s
    (~11 ms at 2 kHz, ~5 at 8), fading from when the wind is in the foot; on the first overtone in its
    own weight (a principal's octave: principals 2, flutes 2, stopped 5); each pipe x exp(+-0.25),
    each attack +-15 % (a quicker pallet up to 15 % more); a key struck again before its pallet
    closed ~ the square of how far it had. Leads (G5-F#6): Waldflöte 2' +7 -> +10, Superoctave 2' +4
    (as was), Gedacktflöte 4' +14 -> +4 dB; C6's presses' spread (Waldflöte) 0.3 -> 0.9 dB.
  - Matched (the recordings' F# octaves): Principal octave -8..-14 dB, 3rd/4th ~-20, speech 108 /
    74 / 44 / 25 / 19 ms (Montre 133 / 113 / 66 / 43 / 11); Gedackt twelfth -13..-21, the 5th-9th
    within the Bourdon's range, breath and puff within a few dB; Gamba from the middle up within a
    few dB.
  - the strings' middle (2026-11-17; our own renders): C3-C5 spoke late and unsure (the pipe alone
    ~200 ms to 90 % at C3, ~120 at middle C; in the swell box ~225 / ~160): the chiff's octave led
    ~40 ms while the fundamental stalled ~40 dB down, ~+100 c sharp, then crept up at the narrow
    pipe's Q (~40: its ring-up q1 / (pi f) ~100 ms at C3); gain, harmFeed, bias and the box's hearing
    changed little (once the jet saturates only the Q sets the ring-up). Now a frein harmonique
    (`freinHz`: its loss at the mouth adds ~5 Hz to every mode's bandwidth, Q ~16 at C3, ~23 at
    middle C, the treble's little changed; from F#2 in, full from C3): the octave's precursor braked
    (the chiff a quarter), a string's low cut-up (the jet's travel 0.22 of the period: less phase
    lost while the foot fills), an open toe (the foot filling in half the time; the cut-up's ~0.8 dB
    back). In the box C3-C5 to 90 % in ~105 ms on average (was ~155), to 30 % within 60 ms (was
    60-105: no hesitation), the pitch within 5 c ~90 ms (was ~110); level within 0.6 dB, H2/H3 as
    before. Left: a quick scoop from flat at C3-D3 (up to ~40 c above -10 dB for ~30 ms, the jet slow
    while its foot fills; ~15-20 c from the pipe alone, the box's reverberation adding to it).
- **The reed pipe** (`src/organreed.h`): a tongue beating on the shallot, the gap's flow and the
  resonator's modes solved together each sample (2x oversampled), a formant band at the top.
  Found: a reed must not hear its enclosure (its loud sound fed back howled on after the release);
  the gap's flow viscous near no pressure; a reed's pitch follows a retuning unevenly (a 16′'s
  bistable): calibrated by secant steps and a fine scan. Matched: the Trompette's and Hautbois's
  bass and middle (harmonics over the fundamental), the bass speech ~30-55 ms. The treble
  (2026-11-17; measured on our own renders): in tune (within 1 c) and steady (pitch sd < 0.1 c,
  ±20 % wind moves it ~3 c, no regime change) all along, but thin and loud: with the formant fixed
  at 1.3-1.5 kHz and the top's roll at 5-9 kHz, a pipe above the band kept only its fundamental (C7
  at 8': H2 -7..-15, H3 -17..-25 dB; the Clairon's C7 at 4 kHz H2 -21, H3 -51), and the model's
  flow, per unit of a pipe's scale, radiated as its rate of change, rose ~27 dB (K-weighted) from
  middle C to C7 (the flues: flat within 4 dB). Now: the band follows the pipe from half its
  frequency (a small top radiates higher: on the 2nd harmonic), the roll at >= 6 harmonics; each pipe
  from middle C regulated down 12 dB at C5 and 6 more per octave (the Oboe 4 and 5), the treble ~+1
  dB per octave over middle C; the Clairon breaks back at c''' (a reprise at 8' pitch, as most do;
  re-regulated +11 dB, its middle as before, its bass no longer a hole). C7 at 8': H2 -0..-8, H3
  -6..-14 dB; the top two octaves' line within 0.5-5 dB. Not taken: a flue reprise for the 8'
  reeds' top (they speak well to C7) and harmonic (double-length) treble resonators.
- **Calibration:** every pipe in tune where it stands (its enclosure's field acting on it, at its
  regulated level), its fundamental through two band-passes in double; cached on disk.
- **Placement:** divided chests (C side left, C# right) in every division; the swell's two boxes;
  the Positiv at the rail; the façade (the Principal 16′ / 8′ basses); the pedal towers; each
  enclosure a 4-line room; the microphones: below 300 Hz both alike, above it a few dB to the side.
- **The wind** (`src/organwind.h`): blower 570 L/s (~34 m³/min, for all 53 stops), a regulator per division (a fast
  valve in a ~4 Hz loop), a relief valve; the full organ with a 10-note chord sags ~8-11 %, a plenum
  ~1 %; the flues ~0.5 c per 1 %. Tremulants: a beater on the regulator's top (±8 L, 5.5 Hz):
  ~13 % p-p, ±3 c.
- **Regulation:** every stop's output set against the Principal 8′ (balanced-organ targets).
  The reeds re-regulated (2026-11-17): their treble line (above) had taken ~20 dB off the treble key of
  the three the regulation averaged, so the reeds sat 6-13 dB under their targets (Trompete 8′ -7,
  Trompette -12, chamade -4 dB against the Principal at C3-C5), and the crescendo roller's top added
  nothing (f -> ff +1.1, ff -> fff +0.0 dB K-weighted, three chords averaged; a single chord over 2 s
  read the fff step -0.8: the unisons' slow beats, ±0.5-1 dB by chance, which also made the Contra
  Posaune 32′ look as if it lowered the organ: averaged it adds ~0, coupled or not; the wind at fff, a
  4-note chord, sags 0.7 %, the reeds' field into the boxes ~0.2 dB). Now on target at C3-C5 (the 16′s
  C4-C5, the Pedal C2-D3), and the roller's fff takes the swell's reed battery whole (Fagott, Trompette,
  Clairon: the French crescendo's last step) with the 16′, 32′ and chamade; ff the Hauptwerk's and the
  Pedal's reed chorus and the high mixtures. Steps from mf: +1.4, +1.9, +1.6 dB (Zwicker-style loudness
  +3.5, +3.1, +1.7 phon); the planner's zones (organ-88) f..fff +0.6, +3.5, +1.8 (were +0.3, +0.8, +1.0).
- **Cost:** a flue ~30 ns a pipe-sample, a reed ~50; enclosures rendered by blocks on 3 threads
  (they meet only in the wind): the full organ (53 stops) with 8 keys ~1.4x faster than real time on
  a busy machine.
- **Not built:** the dedicated nave (§7b.2: the T60s of organ-9.wav, per-pipe early reflections,
  the low modes): `-play -room church` meanwhile (the project's 4 s church). The int16 / multirate
  pipes of §7c-d (the float32 model is fast enough so far). Couplers (all manuals sound on every key;
  the Pedal on the lowest key below G3). A principal's octave-first onset by the jet itself (the
  chiff stands in for it).

## 9. The user's organ recordings [meas]

Analysed in the scratchpad (`ffmpeg`, numpy), numbers only:

| File | Length | Spectral centre | Energy below 60 Hz | >4 kHz | End decay |
|---|---|---|---|---|---|
| `organ-9.wav` | 286 s | 464 Hz | −10 dB | −31 dB | T60 ≈ 7.0 s |
| `organ_7.wav` | 246 s | 470 Hz | −25 dB | −40 dB | T60 ≈ 1.9 s (faded?) |
| `organ_full_12.wav` | 415 s | 385 Hz | −5 dB | −25 dB | T60 ≈ 6.4 s |
| Bach BWV 593 (concerto after Vivaldi) | 805 s | 565 Hz | −22 dB | −30 dB | — |

- **T60 per octave** at the final release (a first estimate, one release each):
  `organ-9.wav` 63 Hz 6.3 s, 125 Hz 9.5, 250 Hz 6.1, 500 Hz 5.6, 1 kHz 6.2, 2 kHz 4.1, 4 kHz 3.6,
  8 kHz 1.6 s; `organ_full_12.wav`'s release is masked (a fade or a held note: 7.2 s at 2 kHz, 6.5
  s at 4 kHz, the others unreliable) — to be refined in step 0 with proper release detection.
- `organ_full_12` and `organ-9` carry strong low energy (pedal 16′/32′: −5 and −10 dB below 60 Hz)
  and a long room: fit the room's T60 (6–7 s) and the pedal's level from them.
- BWV 593 is a plenum-and-trio piece: its trio passages may give isolated manual flues; its ending
  chords give principal-chorus spectra.
- Next (step 0): find single held notes and chord releases in these files (onset detection; the
  final chords) for attack and spectral targets.

## 10. Live (25-key MIDI controller)

- 25 keys + octave shift: a manual; the pads: 8 registration presets (§6.1) and a bank switch for
  coupler/division; K1–K8 knobs: the swell shoe (K1), tremulant on/off & depth (K2), wind
  stability (K3: steady ↔ flexible), room size (K4); the joystick: none (an organ has no pitch
  bend) or the swell **[design]**.
- Pedal: the lowest octave of the keyboard as the pedal division when a "split" preset is on (cf.
  the drum kit's pads) **[design]**.

## 10a. The continuo: the organ under an orchestra (2026-11-24)

The owner, with a text on registering the continuo of Bach's Christmas Oratorio (BWV 248): "you
(the director) needs to make the decision if it is a chamber music piece with a positive or chamber
organ -- small continuo positive organ with Gedackt 8' + Flöte 4' for tuttis and choruses, dropping
the 4' for quiet arias and recitatives -- or if it is a church pipe organ, perhaps look at if the
stage is a concert hall, room, or one of the churches"; and: "even if it is a church, the organ
would need to be register used in a way that it doesn't block the whole orchestra -- perhaps it can
overdo a bit in the climax, but even in usual tutti it needs to be considerate".

Before: a General MIDI church organ in an orchestral file got the solo plan (src/organplan.cc): in
Part 1 of the oratorio it built up to fff, the Hauptwerk's mixtures and reeds and the pedal's
Posaune and Untersatz 32' under the choir. Now (planContinuo, src/organplan.cc; the others' notes
from src/midifile.cc's organCompany):

- **Is it a continuo?** The organ hardly ever plays alone: 85 % of its sounding time another line
  sounds (their articulation gaps bridged). PJ_ORGAN_CONTINUO=0: never (the solo plan), 1: always.
- **Which organ?** By the stage. In a room (-room hall, studio, dry) a positive: Gedackt 8' and
  Flöte 4' (the Positiv's Rohrgedackt 8' and Gedacktflöte 4'; Janakkala's Rückpositiv Gedackt 8',
  Rohrflöt 4'), no pedal. In a church or a concert hall the church organ, played from the Positiv.
  With no -room given, an organ still means the church, timpani the concert hall; new: chamber
  music (six other lines at most, no choir, brass or timpani) over a continuo organ goes to the
  hall, on the positive.
- **How much?** Two-bar windows; the others' lines sounding in each (the mean), the choir's lines,
  trumpets (two brass lines) and timpani. Levels: 0 the Gedackt 8' alone (under 4.5 lines: a quiet
  aria, a recitative); 1 the 8' and the 4' (an orchestral movement, a chorus); 2, church organ only,
  a flute 2' more (the swell's Waldflöte coupled, its box at 0.6; Janakkala's Waltflöt 2'): a
  festive tutti, trumpets or timpani with six lines or more; 3 a light Principal 8' more, for the
  last bars of a piece that ends festive ("it can overdo a bit in the climax").
- **The pedal.** The Subbass 16' (with the Gedacktbass 8' in a festive tutti) only under a full
  texture (eight lines, or three of the choir's) or a festive one, never on the positive;
  elsewhere the bass line plays on the manual at 8'.
- **When does it change?** At a movement's start (a general pause: everyone silent a beat). Inside a
  movement one registration, what its fuller passages ask (the 70th percentile of its windows; a
  movement with trumpets and timpani in a tenth of it is festive throughout); another only where
  it lasts 16 bars.

On the oratorio's files (-play, no -room): Part 1 in the church: the opening chorus level 2 with
the Subbass, the recitatives and the aria the Gedackt alone, the chorale (bar 648) 8' + 4' +
Subbass. Part 6 (timpani on a GM program: the concert hall): the opening chorus 2 and 1 by its
sections, the arias 0 and 1, the last chorale 8' + 4' + Subbass (not festive: in this file's
windows the trumpet mostly plays alone over a thin texture, and the climax's Principal never comes).

Not done: the positive is the church organ's Positiv stops in its own place and room coupling,
not a small instrument beside the cello; no tacet movements (the file decides where the organ
plays); recitative chords are not shortened. In these files the organ's channel also carries a
harpsichord's and a "Rock Organ" program (18) on other tracks, and the program in force flips
between them: only the notes that fall to the church organ are planned here.

Tests: tools/test.py organ_continuo.

## 11. Open questions for the user

1. The **~35 stops** (now 37): is the selection in §6 right (the Pedal keeps 9 of its 12, incl. the
   Untersatz 32′; the Hauptwerk 11 of its 13, incl. Principal 16′ and Cymbel III; the Schwellwerk
   12 of its 16, the cornet and the reed battery whole, no Vox humana; the Positiv 5, provisional),
   or should it be cut to exactly 35 (§6: Clairon 4′ and Gedacktflöte 4′ first), or are other
   stops wanted (all 12 pedal stops, the Salicional, the Plein jeu, the chamade)?
2. **Temperament:** equal, or a well-tempered option (Werckmeister III, Kellner)?
3. **Wind (§4a):** answered (2026-11-16): **modern**, steady regulators, and the blower
   **struggles clearly** at the full organ: ~8 % sag, ~4 c droop (§4a.5: Qmax ≈ 21-26 m³/min).
4. **Room:** the existing `church` (RT60 4 s) or a bigger one fitted to the recordings (6–7 s)?
5. **GM programs:** GM 20 to this organ; should GM 17–19 (tonewheel) get a tonewheel organ later, and GM 21
   go to the harmonium from the free-reed work?
6. **Live control:** registration presets on the pads as in §10, or another layout?
7. Is the "Gloria 355 CC" a specific instrument the user plays (its sound as the reference)? I
   could not find its specification online.
8. **The case layout (§7a.1):** a classical one (Hauptwerk centre, Schwellwerk above in its box,
   Positiv at the gallery rail, pedal towers at the sides, C/C# sides), or a specific church's?
   And where is the listener: in the nave (distant, the room dominant) or at the console (the
   Positiv close, the room around)?
9. **Sympathy of silent pipes:** with the 3-core budget, the 2-4 nearest silent pipes per sounding
   pipe are affordable (§7b.1); more, or none?
10. **The church:** one nave fitted to `organ-9.wav` (T60 ~6 s mid, 1.6 s at 8 kHz), or a choice of
    churches? Listener in the nave, at the console, or both as options?

## 12. Sources

- **[Castellengo 2004]** M. Castellengo, "Flute-like instrument transients: an analytical study of
  mouth-tone production versus pressure rise time", ISMA 2004 (Nara). Pipe 312 mm × Ø 27 mm, mouth
  21 × 7 mm, flue 0.2–0.3 mm, R1 475 Hz; steady foot pressure 360 Pa, threshold 125 Pa; transients
  160 / 45 / 26 ms. https://www.lam.jussieu.fr/Membres/Castellengo/publications/2004c-Transient_Pressure%20rise%20time-ISMA.pdf
- **[Castellengo 1999]** "Acoustical analysis of initial transients in flute-like instruments".
  https://www.lam.jussieu.fr/Membres/Castellengo/publications/1999a1-Flute%20transients%20Analysis.pdf
- **[Pykett flutes]** C. Pykett, "The Tonal Structure of Organ Flutes".
  https://colinpykett.org.uk/the_tonal_structure_of_organ_flutes.htm
- **[Pykett reeds]** C. Pykett, "The Tonal Structure of Organ Reeds"; "How the Reed Pipe Speaks".
  https://www.colinpykett.org.uk/reedpipetones.htm ; https://colinpykett.org.uk/reedpipe.htm
- **[Pykett violone]** C. Pykett, "A Second in the Life of a Violone" (a pipe's attack).
  https://www.colinpykett.org.uk/violone.htm
- **[Pykett trems]** C. Pykett, "Tremulant Simulation in Digital Organs". https://colinpykett.org.uk/digitaltrems.htm
- **[jOrgan]** "Analyzing, Understanding, and Replicating Pipe Organ Tremulants" (a 3.83 Hz example).
  https://jorgan.info/base/arch/ps/do/Analyzing,%20Understanding,%20and%20Replicating%20Pipe%20Organ%20Tremulants%20with%20Soundfonts%20and%20jOrgan.docx
- **[BYU]** BYU Organ Tutor, the tremulant. https://organ.byu.edu/organtutor/organ_registration/non-speaking_stops/tremulant/page2.htm
- **[AT 2017]** Acoustics Today, Spring 2017 (organ pipe research, Angster, Rucz).
  https://acousticstoday.org/issues/2017AT/2017Spring/files/basic-html/page18.html
- **[Angster]** J. Angster et al., measurements on lingual (reed) organ pipes, JASA 2016.
  https://independent.academia.edu/JuditAngster
- **[Hruška 2020]** V. Hruška, P. Dlask, transient descriptors of organ pipes, Archives of
  Acoustics 2020. https://czasopisma.pan.pl//Content/117148/PDF/aoa.2020.134054.pdf
- **[Abel 2006]** M. Abel, S. Bergweiler, R. Gerhard-Multhaupt, "Synchronization of organ pipes by
  means of air flow coupling: experimental observations and modeling", JASA 119(4), 2467-2475, 2006
  (preprint arXiv:physics/0506094). https://arxiv.org/abs/physics/0506094
- **[Abel 2009]** M. Abel, K. Ahnert, S. Bergweiler, "Synchronization of sound sources", PRL 103,
  114301 (2009). https://arxiv.org/pdf/0904.2449 ; https://www.pubmed.gov/19792375/
- **[Fischer 2013]** J. Fischer, M. Abel et al., "The mother tongue of organ pipes: synchronization,
  experiments, numerical simulations, and model", JASA 134, 4243 (2013).
  https://www.kulturwissenschaften.uni-hamburg.de/sm/personen/fischer/publikationen/2013fischer-jasa.html
- **[Fischer & Bader 2015]** Vienna Talk 2015 (Navier-Stokes simulations of pipe synchronization).
  https://viennatalk2015.mdw.ac.at/index.htmlpage_id=13002&pap=38.html
- **[Sawicki 2018]** J. Sawicki, M. Abel, E. Schöll, "Synchronization of organ pipes", EPJ B (2018);
  arXiv:1708.06571. https://arxiv.org/pdf/1708.06571 ; https://link.springer.com/article/10.1140/epjb/e2017-80485-8
- **[DAGA 2011]** anti-phase locking of two pipes. https://www.pub.dega-akustik.de/DAGA_2011/data/articles/000075.pdf
- **[arXiv 2025]** "Importance of near- and far-field coupling for the synchronization of organ
  pipes". https://arxiv.org/html/2504.00556v1
- **[Pykett blowing]** C. Pykett, "The physics of organ blowing". https://colinpykett.org.uk/physics-of-organ-blowing.htm
- **[Laukhuff]** Laukhuff Ventus blower datasheet (120 mm WS range: 7.5-28 m³/min).
  https://www.logosfoundation.org/instrum_gwr/roro/Datasheets/Laukhuff_6.pdf ; Meidinger:
  https://www.logosfoundation.org/instrum_gwr/roro/Datasheets/meidinger.pdf
- **[toff]** Theatre Organ Forum: wind supply, regulators. https://toff.org.uk/chambers/wind-supply/ ;
  https://toff.org.uk/chambers/regulator/ ; https://toff.org.uk/chambers/wind/
- **[Diapason]** "Wind" (flow figures need their pressure). https://www.thediapason.com/wind-11
- **[Parkey]** Winding systems (Schwimmers). https://www.parkeyorgans.com/?p=13251
- **[organsupply]** Reservoirs (eveners against "shimmy"). https://www.organsupply.com/assets/CatalogPDFs/Section-2-Reservoirs.pdf
- **[Taylor]** A. J. Taylor, PhD thesis, University of Salford: blowers, valves, reservoir resonance
  and pipe flutter. https://salford-repository.worktribe.com/OutputFile/1490319
- **[IOA Woolley]** A. G. Woolley, A. G. Apostoli, pressure build-up in the feet of flue pipes.
  https://www.ioa.org.uk/system/files/proceedings/ag_woolley_ag_apostoli_a_review_of_an_experiment_on_the_pressure_build-up_in_the_feet_of_flue_organ_pipes.pdf
- **[pipe-scale calculator]** (an unverified tool; pressures by division, flow from the flue).
  https://pipeorganservices.pages.dev/tools/pipe-scale-calculator.html
- **[Modartt]** Organteq's physical modelling (a commercial precedent).
  https://modartt.com/organteq_physical_modeling
- **[F&R]** N. H. Fletcher, T. D. Rossing, *The Physics of Musical Instruments*, ch. 16–17 (flue
  and reed organ pipes) **[memory]**.
- Project: `src/flute.cc` (jet/edge, 145 ns/sample), `src/reed.*`, `src/room.cc` (`church`
  15 000 m³, RT60 4 s), `src/parallel.h`, `research/jet_and_losses.md`.
