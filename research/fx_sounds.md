# The General MIDI "FX" sounds (programs 97–104): research and design

Status: all eight are built. FX 1 (rain) researched and built (`-inst fx-rain`, `src/rain.*`,
2026-11-20); FX 2 (soundtrack) likewise (`-inst fx-soundtrack`, `src/soundtrack.*`,
2026-11-20); FX 4 (atmosphere) likewise (`-inst fx-atmosphere`, `src/atmosphere.*`,
2026-11-20); FX 5 (brightness) likewise (`-inst fx-brightness`, `src/brightness.*`,
2026-11-20); FX 6 (goblins) likewise (`-inst fx-goblins`, `src/goblins.*`, 2026-11-20); FX 7
(echoes) likewise (`-inst fx-echoes`, `src/echoes.*`, 2026-11-20). FX 3 (crystal,
`src/lead.cc`) and FX 8 (sci-fi, `src/scifi.*`) were built before this file. The pads in
`src/midifile.cc` (~line 211) remain each program's fallback. One section per program follows.

How each fact is marked:
- **[web, source]** read on the web (the source named; the list is at the end).
- **[paper]** from the cited paper (as known from its abstract or standard citations; not
  re-read in full for this note).
- **[memory]** general knowledge, not looked up: check before relying on it.
- **[T]** tested here: measured on a SoundFont (parsed with a small SF2 reader) or rendered with
  TiMidity++ 2.14 / pajupilli.

## Survey: what the classic sets do for 97–104

Roland's GS names (SC-55, SC-88) and Yamaha's XG names are [memory] except where marked; the
three SoundFonts' preset names and samples are [T] (read from the files' `phdr`/`inst`/`shdr`).

| GM | GM name | Roland GS (SC-55/88) | Yamaha XG | FluidR3_GM: preset, layers | TimGM6mb | Jnsgm2 | pajupilli |
|---|---|---|---|---|---|---|---|
| 97 | FX 1 (rain) | Ice Rain [web, soundhouse] | Rain | Ice Rain: decaying filtered saw + Halo Pad + "Iceloop" (a 31 ms looped glass ping) | IceRain: one 1.7 s looped sample of a tinkling figure on C4, C5, G5 | Ice Rain: synth strings + a 30 ms looped xylophone ping | `fx-rain`: rain on tuned glass bowls (this file) |
| 98 | FX 2 (soundtrack) | Soundtrack (from the D-50's "Soundtrack") [web, synthmania; wikipedia] | Sound Track | Soundtrack: "Alien Strings" twice, on the note and a fifth up | synth strings twice, the second a fifth up | synth strings twice, the first a fifth up | `fx-soundtrack`: a keyed aeolian harp (this file) |
| 99 | FX 3 (crystal) | Crystal | Crystal | Crystal: celesta + square wave | celesta + "Spectrum" | celesta variants + "Spectrum" | `fx-crystal`: bowed/struck glass plate (`src/lead.cc`) |
| 100 | FX 4 (atmosphere) | Atmosphere (from the D-50's "Nylon Atmosphere") [web, wikipedia; synthmania] | Atmosphere (variations "Nylon Harp", "Warm Atmos", ...) [web, Yamaha lists] | Atmosphere: nylon guitar + Alien Strings, both decaying | nylon/steel guitar (a filtered short onset) + synth strings | nylon guitar + synth strings | `fx-atmosphere`: a keyed psaltery with magnetic resonators (this file) |
| 101 | FX 5 (brightness) | Brightness ("a Fairlight type of sound") [web, synthmania SC-50] | Brightness | Brightness: "Bright" (a chorused C6 cluster, looped) + synth strings | solo vox + synth strings | solo vox + synth strings an octave up | `fx-brightness`: a keyboard of singing arcs (this file) |
| 102 | FX 6 (goblins) | Goblin | Goblins | Goblin: synth vox + vox + wind | syn vox (aahs at the top) + sitar | the same | `fx-goblins`: a pyrophone, singing flames in glass pipes (this file) |
| 103 | FX 7 (echoes) | Echo Drops (one voice; "sweeping tones, drops at the release") [web, soundhouse] | Echoes | Echo Drops: "Ahh Choir" + "SynthVz" (a pair panned ±50 %), a 5 s release | aahs + syn vox + synth strings | syn vox + synth strings | `fx-echoes`: a dulcitone (tuning forks) into a tape echo (this file) |
| 104 | FX 8 (sci-fi) | Star Theme | Sci-Fi | Star Theme: clean electric guitar + "Sweeper" | clavinet + Strat + synth strings | electric guitar + synth strings | `fx-sci-fi` (`src/scifi.cc`) |

A pattern [T]: every FX program in the SoundFonts is a layer of a pad (synth strings, a halo
pad, a choir) with something struck or plucked (celesta, xylophone, guitar, sitar). GM itself
only names the programs; the sounds are each maker's.

## FX 1 (rain), GM 97

### What it is in the classic sets

- **Roland (SC-55 onward), "Ice Rain".** GM2 names program 97 "Rain"; Roland has called it "Ice
  Rain" since 1991, evoking hailstones [web, soundhouse]. A Japanese column describes it as a
  synth-brass/strings-like pad with a hard percussive attack from a looped xylophone-like sound,
  glassy, celesta-like, and traces it to the Roland D-50 (1987) [web, soundhouse; not checked
  against a D-50 preset list]. Later Roland libraries add an "Ice Rain Dly" variation [web,
  roland]. That the SC-55's droplets are a short tuned ping retriggered by a looped sample, not
  random notes, is [memory] and matches the SoundFonts below.
- **Yamaha XG, "Rain"** (variations "Harmo Rain", "African Wind", "Carib" on banks > 0) [memory]:
  a pad with a glittering high layer; not inspected.
- **FluidR3_GM, "Ice Rain"** [T], three layers:
  1. looped single-cycle sawtooths (`saw-fb-110` .. `-880`, one per octave), low-passed
     (fc ~ 9525 cents ≈ 4.3 kHz, Q 3 dB, the mod envelope opening it), with a volume envelope
     that **decays** to silence over ~15 s (decay 4757 tc, sustain 100 dB) and releases in ~3 s;
  2. "Halo Pad (L)", a 1 s looped pad sample over the whole keyboard;
  3. **"Iceloop"**: a 1358-sample loop (31 ms at 44.1 kHz) holding one decaying glass ping
     (2.34 kHz with a 13.6 kHz partial; the loop's envelope falls to 0.08 by its end). Played
     back looped it is a ping retriggered at the loop rate. Its tuning is scaleTuning 20 c/key,
     coarse -26 semitones, root 82, and a mod envelope adds +11.9 semitones that decays over
     ~6 s (3203 tc). So the "droplets" are a **periodic** ping train (~14–24 Hz measured on the
     render), the ping and the train both gliding down an octave over the first seconds, and
     they barely follow the key: C2 → ~1.0 kHz, C4 → ~1.3 kHz, C6 → ~1.75 kHz at 4–6 s (20 c per
     key, as the file says) [T]. Its volume envelope: instant, decays to -22.6 dB in 0.34 s;
     5.4 Hz pitch LFO of ±1 semitone.
  Rendered through TiMidity (C4, velocity 100): the level falls steadily, ~30 dB in 7 s; the
  partials are the note's harmonics 1–7 (the saw and pad) plus the ping's inharmonic cluster;
  above 8 kHz -28 dB re the whole [T].
- **TimGM6mb, "IceRain"** [T]: a single 1.74 s sample (17857 Hz) looped over 1.47 s, root C4,
  following the key normally. Its spectrum is C4, C5 and G5 (harmonics 1, 2, 3 of the note):
  a recorded tinkling figure on the note, its octave and twelfth, uneven in level (the loop's
  envelope swings 0.4–1.0), i.e. a stepped, tuned sparkle.
- **Jnsgm2, "Ice Rain"** [T]: synth strings (per-octave samples) and "Xylo Looped E4", a 30 ms
  looped xylophone ping with the same envelope values as FluidR3's Iceloop (mod env to pitch
  1191 c, decay 3203 tc, LFO 103 c at -725 tc): the same lineage, but with normal key tracking,
  so here the pings follow the key.

So the classic patch is a **pad plus a retriggered, glassy ping**: periodic in the
SoundFonts (a loop), on the note's harmonics in TimGM6mb and Jnsgm2, nearly fixed in pitch in
FluidR3. None has random-pitched droplets; the "rain" is the retriggering and the chime.

### How it is used in GM files

Sustained chords and atmosphere: intros, new-age and ambient beds, game music, under a melody
[memory]. None of the 44 local files in `~/midi` uses program 97 [T], so there was nothing to
measure; the design aims at held chords (2–8 s) at moderate velocities, and at a slow melody.

### The physics of rain

- **Drop sizes: Marshall and Palmer (1948)** [paper]: N(D) = N0 exp(-Λ D), N0 = 8000 m⁻³ mm⁻¹,
  Λ = 4.1 R^-0.21 mm⁻¹ (R the rain rate, mm/h). Light rain (1 mm/h): Λ ≈ 4.1, few drops over
  1 mm; heavy (50 mm/h): Λ ≈ 1.8. The exponential over-counts large drops: breakup cuts the tail
  at ~5–6 mm, and in light rain the largest drops are far smaller [memory].
- **Fall speeds: Gunn and Kinzer (1949), fitted by Atlas et al. (1973)** [paper]:
  v(D) ≈ 9.65 − 10.3 exp(−0.6 D) m/s (D in mm): 1 mm ≈ 4 m/s, 2 mm ≈ 6.5 m/s, 5 mm ≈ 9.1 m/s.
  The number of drops landing per area and time is ∫ N(D) v(D) dD, of order 10⁴ m⁻² s⁻¹ at a few
  mm/h [arithmetic]; it grows only as ~R^0.2 — heavier rain is mostly *larger* drops.
- **The sound of a drop into water**, by size (Medwin, Nystuen et al.; NASA's summary) [web, nasa]:
  < 0.8 mm: silent; 0.8–1.2 mm ("small"): a bubble entrained at *every* splash, loud, ringing at
  13–25 kHz, the impact very quiet; 1.2–2.0 mm: impact only, weak, broadband 1–30 kHz, no
  bubble; 2–3.5 mm and larger: a loud impact (1–35/50 kHz) and bubbles of varied size,
  1–10 kHz. Under-water rain noise has its peak near 14–15 kHz from the small drops' bubbles
  (~0.2 mm bubble radius) [web, arxiv 2512.09336].
- **Entrainment: Pumphrey and Elmore (1990)** [paper]: the regular entrainment band of drop
  sizes and impact speeds that traps a bubble at the bottom of the crater; outside it, irregular
  entrainment. Drops under ~1 mm did not entrain regularly in their tank [web, search snippet].
- **Minnaert (1933)** [paper]: a bubble of radius R rings at f = (1/2πR) √(3γ p0/ρ) ≈ 3.26/R Hz
  (R in m): 1 mm → 3.3 kHz, 0.2 mm → 16 kHz.
- **van den Doel (2005), "Physically based models for liquid sounds"** [paper]: each bubble an
  exponentially damped sine, d = 0.043 f + 0.0014 f^{3/2} s⁻¹ (viscous, thermal and radiation
  losses), its frequency rising as the bubble moves up to the surface,
  f(t) = f0 (1 + σ d t), σ ≈ 0.1; a stream or rain is many such bubbles with sizes from a power
  law. Already used in `src/sfx.cc` (seashore) [T].
- **The impact on a solid** (leaf, glass, metal) [memory]: the drop delivers its momentum m v
  over a contact time of about D / v (0.2–0.5 ms), so the force pulse's spectrum is flat up to
  ~1–2 kHz and falls above: large drops on a hard surface are "plinks" of the surface's modes,
  small ones a hiss. The radiated tick is roughly the force's derivative.
- **Rates** [arithmetic]: on a 20 cm bowl (0.03 m²) a few-mm/h rain lands ~100–300 drops/s,
  only a few per second over 2 mm.

### Design: rain on tuned glass bowls

What the patch must do: play chords in tune (pads), sound like droplets (a glassy ping
re-struck at a quick, uneven rate), and sit as a soft bed when held. Three physical
candidates:

1. **Bubbles tuned to the notes** (drawing bubble radii so Minnaert's frequency lands on the held
   notes' harmonics). Rejected: a bubble rings for a handful of cycles (Q ~ 10–30 at 1–10 kHz),
   so it is a "bloop" with little pitch, and real rain does not choose its bubble sizes; nothing
   would hold the pad.
2. **Drops on leaves or a metal roof.** Not tuned; a roof's modes are dense.
3. **Drops on tuned glass bowls (chosen).** A struck bowl rings for seconds on its own pitch, as a
   glass harmonica's or a singing bowl's. Rain on it is a Poisson train of strikes of random
   size and place: the large drops are the audible plinks, the many small ones keep the bowl
   ringing between them — the pad bed arises from the same physics. It is the classic patch's
   structure (a glass ping re-struck over a sustained tone) made by the rain itself, random
   where the SoundFonts' loops are periodic. Bowls on the note's harmonics give TimGM6mb's
   tuned figure (note, octave, twelfth); drops into the water add the real rain's bubbles and
   ticks, untuned, quietly.

The model (`src/rain.h` has the summary):
- **The bowls:** each key sets out bowls on 1×, 2×, 3× and 4× its note (K4: 1–5). A thin
  hemispherical bowl's bending modes n = 2..5 lie at n(n²−1)/√(n²+1): 1, 2.83, 5.42, 8.77
  [paper: Rayleigh; memory]. The 2.83 mode would lie a semitone under the 3× bowl beside it, so
  the wall is "thinned" to put n = 3 on the twelfth (3.0), as handbell founders tune their
  bells' partials [memory]. Each mode is a doublet (cos nθ, sin nθ) split 0.05–0.25 % by the
  glass's flaws: slow beats. T60 ~4 s at middle C (falling up the keyboard; K3 ×0.25..4), higher
  modes shorter. A smaller bowl catches fewer drops (rate ∝ 1/k) but, lighter, rings a
  little louder under each (√k: between swinging more, ∝ k, and moving less air).
- **The rain:** the key's velocity is the rain rate R = 0.3 … 90 mm/h; diameters from
  Marshall–Palmer with Λ(R), flux-weighted by v(D), truncated at d_max = 1.5 R^0.25 mm
  (~1.2 mm in a drizzle, ~4.6 mm in a cloudburst); a Poisson arrival at K1's density (140 drops/s
  on the note's bowl by default) times a gust factor (an Ornstein–Uhlenbeck walk, 3 s, ±35 %,
  shared by all keys: the wind). 60 % of the drops hit glass, 40 % the water inside.
- **A drop on glass:** momentum D³v into each mode, weighted by a sin² pulse's spectrum of length
  D/v at the mode's frequency, cos/sin nθ at a random angle θ and (height)^(n−1) at a random
  height on the wall. Its tick: one cycle of a sine over the contact time (the force's
  derivative, no DC), ∝ momentum / contact time.
- **A drop into the water:** 12 % of the push reaches the bowl; a 0.8–1.2 mm drop always
  entrains a bubble (13–21 kHz by size); a drop over 2 mm, half the time, a bubble of 1–10 kHz;
  each a van den Doel bubble (damped, chirping up).
- **The key:** a first drop (a drip from the eaves, 1.2 + 3.8·v mm, larger than any raindrop at
  that rate) strikes the note's bowl near its rim: the attack, monotone in velocity. Release:
  the rain on those bowls stops and a hand damps them (T60 ≤ 1.5 s). Pedal holds; bend ±2.
- **Stereo:** the note's bowl panned by the key, the others ±0.25 around it, bubbles near
  their bowl (`takeSide`).

### Measurements [T]

- In tune: the note's bowl's doublet, power-weighted, −1.2 … +4.2 c from C2 to C7; 88–96 % of
  the energy on the note's harmonics 1–6 (`tools/test.py` `fx_rain`).
- Texture (C-major chord at 90, 4–12 s): ~13 onsets/s, 100 ms level s.d. 2.8 dB, the 11th–12th
  seconds within 3 dB of the 5th–6th (the rain goes on; the first drip has rung down by ~4 s,
  10 dB above the bed), silent 3 s after release. No step at the keys (largest sample step
  0.12 of the peak), no DC.
- Velocity: chord RMS −48, −38, −27, −21 dB at 20, 60, 100, 127. Velocity row fitted with
  `velcurve.py`'s `render()`/`gain_curve()` on the mean of five keys (C3 G3 C4 E4 C5).
- Level: −29.3 LUFS untrimmed by `levels.py`'s measure(); trim +1.1 dB to the median −28.2.
- Cost: eight notes (4 bowls × 4 modes × 2 each) ~0.6 µs/sample, 3 % of a core.

### Compared with the SoundFonts (`tools/synthcompare.py profile`, program 97) [T]

pajupilli against FluidR3_GM, with TimGM6mb and Jnsgm2 as references (52 notes each):

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 |
|---|---|---|---|---|
| decay rate of a held note (dB/s) | 5.6 | 6 | 0.5 | 7 |
| level after 2 s held, re peak (dB) | −17 | −15 | −7 | −20 |
| release, 30 dB after note-off (ms) | ~580 | 654 | 1841 | 520 |
| odd / even partials (dB) | −0.7 | −4.4 | −0.7 | −2.7 |
| fundamental share (tristimulus 1) | 0.86 | 0.75 | 0.47 | 0.82 |
| brightness (centroid, ERB-number at vel 96) | 9.8 | 8.0 | 14 | 9.0 |
| noisiness (spectral flatness, dB) | −27 | −38 | −45 | −37 |
| onset sharpness (dB/ms) | 44 | 3.3 | 6.5 | 3.9 |
| key-to-key level scatter (dB) | 3.9 | 0.54 | 0.25 | 0.19 |
| brightening velocity 40 → 127 (ERB) | −0.6 | 0 | 0 | 0 |
| pitch scatter key to key (c) | 1.5 | 8 | 3.3 | 3.3 |

What differs: (1) the droplets — ours random and on the key's harmonics, FluidR3's a periodic
~14–24 Hz ping train nearly fixed in pitch; (2) noisier above 6 kHz (the small drops' bubbles;
FluidR3's >8 kHz share is −28 dB of the whole, ours similar at velocity 90 but flatter); (3) a
far sharper onset (a drip on glass, against sampled pads' soft fronts); (4) each note's level
varies by its own rain; (5) harder notes slightly darker (a large drop's longer contact rolls
off the highs), where the samples do not change. Envelope and release are alike. An earlier
version had bowls 1–3× only and the bowl's untuned 2.83 mode: odd/even +4.5 dB (hollow) and
partials 2.83/3 a semitone apart; four bowls, the n = 3 mode tuned to 3.0, and the smaller bowls
a little louder per drop brought it to −0.7 dB.

## FX 2 (soundtrack), GM 98

### What it is in the classic sets

- **Roland's origin: the D-50's "Soundtrack" (1987).** Wikipedia's D-50 article lists
  "Soundtrack" with "Fantasia" and "Atmosphere" among the D-50 presets that live on in every
  General MIDI device [web, wikipedia]. Synthmania analyses it as factory preset 37
  [web, synthmania]:
  - Dual key mode (split C4) in stereo through the reverb.
  - Upper tone "MelloTones", lower tone "BriteFifth". Both use structure 1: synthesized square
    partials, no PCM. The lower tone's two partials are **a fifth apart**.
  - It has chorus, slight pitch modulation and per-tone detuning.
  - Its partial-1 envelopes "change dramatically by velocity [and] release".
  - The author calls it "the perfect pad".
- **Roland GS (SC-55, SC-88), "Soundtrack":** the GS/GM2 name for program 98 [web, polyphone /
  GM2 list]. That the SC-55's version is a dark, slow fifth pad in the D-50's line is [memory]:
  not heard or inspected here.
- **Yamaha XG, "Sound Track":** [memory], not inspected.
- **FluidR3_GM, "Soundtrack"** [T]: two layers of one instrument (both "Alien Strings", a stereo
  pair panned ±50 %).
  - **The sample:** "Alien Strings" (1.16 s at 44.1 kHz, looped from 0.83 s) is a **detuned
    cluster of nearly pure tones**. Its peaks span 1028–1072 Hz around C6 (±~35 c), matching
    its overriding root key 84. The 2nd harmonic is −12 to −16 dB, and it rises ~7 dB in its
    first 100 ms. So the ensemble and the darkness are in the sample.
  - **Layer A, on the note:** attenuation 26 dB, +10 c. The preset zeroes the hold and decay
    and lifts the sustain, so it holds level after an instant attack.
  - **Layer B, a fifth up:** coarse tune +7, −10 c, attenuation 16 dB. It holds 0.4 s, then
    decays 14.9 s to −11 dB, with a 4.1 Hz, 0.8 dB tremolo from the preset's global zone.
  - **Filter:** the instrument's filter (fc 11108 c ≈ 5.0 kHz, Q 5 dB, mod envelope +10000 c,
    attack 0.2 s, decay 3 s to 42 %) is pushed past 20 kHz by the preset's offsets (+5294 and
    +5561 c), so it is open. The filter sweep the GM sound is often described with is absent
    here.
  - **Release:** 1200 tc + the global 1902 tc → 6 s nominal; TiMidity renders ~22 dB/s.
  - **Rendered** (C4, velocity 100, TiMidity): no swell, an even level beating ±2.5 dB, centroid
    ~250 Hz. Partials relative to the note: fifth −10, 2f −7, 3f −14, 4f −23, 4.5f −25, 5f −26,
    6f −26 dB.
- **TimGM6mb, "Soundtrack"** [T]: two instruments on short "SynthStrings" loops (G2, G3, G4,
  D6; 22 kHz, 0.12–0.26 s).
  - **"Soundtrack0", a fifth up** (by its root keys, e.g. G3 at root 71 where the other uses
    root 78): a resonant low-pass (fc 5138 c ≈ 159 Hz, Q 5 dB). Its mod envelope (+4612..4837 c)
    opens it to ~2.3 kHz in 0.23 s and closes it over 3.5 s to ~390 Hz. So the filter sweep is
    **bright then darker**, not the reverse. Volume: attack 0.19 s, hold 0.5 s, a ~20 s decay to
    −0.2 dB, release 5.6 s.
  - **"Soundtrack1", on the note:** a gentle fixed low-pass, key-tracked by zone (fc 814 Hz in
    the bass to 3.2 kHz at the top), attack 0.12 s, release 5 s.
  - **Rendered:** steady, centroid ~560 Hz. Fifth −4, 2f −11, 3f −9, 4f −13 dB.
- **Jnsgm2, "Soundtrack"** [T]: the same patch as TimGM6mb (same envelopes and filters to a cent)
  on 44.1 kHz samples. Its filtered layer is put a fifth up with an explicit coarse tune of +7.
  It renders like TimGM6mb.

So the common core [T, web]: **a sustained string-like pad of the note with a second voice a
fifth above** (4–10 dB under the note), detuned for a chorus, darker than a sawtooth. The D-50
also made it dynamic: velocity and release reshape it. A slow swell and a dark-to-bright filter
are not in the SoundFonts: they start within 0.1–0.25 s, and TimGM6mb's filter closes rather
than opens.

### How it is used in GM files

None of the 44 files in `~/midi`, nor any of 353 MIDI files under `~/Music`, `~/Downloads` and
`~/Musici`, selects program 98 [T]. As for 97, the design aims at held chords and slow
progressions (2–10 s) at moderate velocities: film-like beds under a melody [memory].

### The physics: aeolian tones and vortex-induced vibration

- **Aeolian tones: Strouhal (1878)** [paper]: a wire in wind sings at f = St U / D, St ≈ 0.2
  (from Re ~ 10² to 10⁵). The sound is the vortex street's alternating lift.
- **The aeolian harp** [memory]: a box with gut strings, usually tuned to one pitch but of
  different gauges, set in a window. The wind locks each string onto whichever harmonic the
  shedding lies near, so the harp sounds swelling, shifting chords of harmonics: Kircher
  described it in the 17th century; Coleridge's "The Eolian Harp" (1795).
- **Lock-in** [paper: Williamson and Govardhan 2004]: near a structural mode the shedding
  frequency is captured by the body's motion. For a heavy, lightly damped body (a string in air,
  mass ratio μ ~ 10³) the capture band is narrow, and the frequency is the body's own.
- **The wake oscillator: Facchinetti, de Langre and Biolley (2004)** [paper]:
  - The wake is q'' + ε Ω (q² − 1) q' + Ω² q = A ÿ/D, with ε = 0.3 and A = 12 (acceleration
    coupling: the best of the three couplings they tested against experiments).
  - The structure is ÿ + (2ζω + γΩ/μ) ẏ + ω² y = M Ω² q, with M = C_L0 / (16π² St² μ),
    C_L0 = 0.3 and γ = C_D / (4π St) ≈ 0.8.
  - Nylon in air: μ ≈ 750 and M ≈ 6.4·10⁻⁵.
  - The air's own damping, γΩ/(2μ), is comparable to a string's (ζ ~ 10⁻³).
- **Along-wind:** the drag fluctuates at twice the shedding frequency [paper, the same
  literature]. And a string's tension rises with the square of its swing, which reaches the
  bridge at twice the frequency: the octave grows as the amplitude squared [memory; the
  mechanism of the piano's "phantom partials"].

### Design: a keyed aeolian harp

What the patch must do: play chords in tune, a dark chorused pad of the note and its fifth, that
swells and opens (the prompt's brief) and holds. Three physical candidates were weighed:

1. **An analog circuit pad** (oscillators into a component-level OTA or ladder filter opened by
   an RC envelope). This is the most literal reading of the patch: the D-50's TVF and the
   SoundFonts' filter envelopes. Rejected because it is what `src/lead.cc`'s pads already are,
   and `pad-warm` is the fallback this replaces. A second one would add a parameter set, not a
   sound.
2. **A bowed body that blooms** (a bowed glass or plate, a bowed string with a slow bow).
   Rejected:
   - `fx-crystal` is already a bowed glass plate.
   - Stick–slip speaks within a few periods and is bright (Helmholtz motion: a sawtooth). It
     would need its darkness filtered in.
3. **Self-oscillating resonators that bloom: an aeolian harp (chosen).**
   - **Dark and pure:** a string driven by its wake is nearly sinusoidal (the lift of a vortex
     street is), the same nearly pure tone as "Alien Strings".
   - **A slow bloom:** it grows over its own Q (~1/(ζω), 0.3–1 s), not a programmed envelope.
   - **Chorus:** several strings a few cents apart beat, the patch's chorus.
   - **The fifth:** a string on the fifth is the fifth layer (the D-50's "BriteFifth").
   - **Brightness from the wind:** wind across a wire is how real harmonics appear (the harp
     locking higher modes), so brightness can come from where the wind blows, not from a filter.
   - **Keys:** a "keyed" harp, a shutter per course, makes it playable: chords, any key.

The model (`src/soundtrack.h` has the summary):
- **The course:** three strings on the note (±1.5–4 c, random per note, plus one near the
  middle) at exposures 1, 0.8 and 0.62, and one on a just fifth (×1.5) at 0.55 (K3 0–2×). Each
  has up to six modes, slightly stiff (B = 10⁻⁵). T60 = 3.5 s × (f/262)^−0.45, clamped to
  1–5 s, the modes j^−½ shorter (K4 ×0.25–4).
- **The wind:** one blower, its speed's log an Ornstein–Uhlenbeck walk (2.5 s, ±0.3 %; K1 up to
  2.4 %).
- **The shutter:** a key slides it to 0.03 + 0.97 v^1.5 of its travel, at (0.5 + 0.5 v) per
  0.8 s (K2 ×0.25–4), with 50 ms of inertia. It uncovers:
  - the main stretch, two wake cells at 0.29 and 0.61 of the length, ¼ of the span each, at the
    first mode's lock speed, over the whole travel;
  - side slots at 2, 3, 4 and 5 times the speed, at their modes' antinodes (¼, ½, ⅜, 0.3),
    spans 0.05, 0.025, 0.008 and 0.004, opening over 0.35–0.65, 0.55–0.8, 0.7–0.9 and 0.8–1 of
    the travel. The fifth's string has the slots up to 3.
- **Each cell:** Facchinetti's wake at the cell's Ω, its acceleration input the string's at
  that point (Σ φ_j ÿ_j). Its lift 2 · span · M Ω² · exposure · q goes onto each mode through
  sin(jπx). The air's damping is (j·γΩ/(2μ)) × the uncovered span.
- **The bridge:** (1 + κ Σ j² y_j²/4) · Σ j y_j (the tension times the slope) + κ Σ j² y_j²
  (the tension's swing along it), with κ = 10 (the octave ~−10 dB at full swing). A DC blocker
  at 8 Hz takes the tension's mean.
- **Voicing:** a key gain of (f/262)^−0.55, since the swing in diameters grows up the keyboard
  with Q. This stands in for gauges and box radiation that are not modelled.
- **Hiss:** the slots' turbulence, white noise band-passed at 1.5–6 kHz, ∝ the exposure, ~45 dB
  under the strings.
- **Release:** the shutter closes in 0.25 s, and its felt edge sets T60 ≤ 2.5 s × (f/262)^−0.2.
- **Stereo:** the course panned by the key, its strings ±0.3 around it (`takeSide`).

Numerics that mattered [T]:
- **The damping term.** The van der Pol damping was first taken explicitly (symplectic Euler).
  At Ωh = 0.1–0.17 that raised the wake's frequency by 1.5–2.6 %, outside the narrow capture
  band, and the 3×–5× slots never locked (or locked only when a gust happened to pass).
  Integrating the damping by the trapezoid fixed it.
- **The Strouhal correction.** Ω is raised by 1/(1 − ε²/16) so the limit cycle runs at the
  Strouhal frequency.
- **Sub-steps.** Above ~C7 the wake blew up at Ωh ≥ 1; it now takes sub-steps of ≤ 0.4 rad.
- **The lock, measured:** after 3 s at velocity 127 every cell's wake (zero-crossings over 1 s)
  sat at its mode within the count's resolution (0.9996–1.0005).
- **The capture band is narrow** (a fixed wind of ±2 % off the lock speed leaves C4 silent and
  off pitch). That is why the wind is steadied and K1 is the gusts, not the speed.

### Measurements [T]

- **Tuning:** the power-weighted pitch is −1.0 to −0.2 c from C2 to C7 (the three strings'
  centroid). The fifth's string is 6–7 dB under the note (`tools/test.py` `fx_soundtrack`).
- **Touch:** a C-major chord, 1–6 s, at velocities 20, 60, 100 and 127 measures −41.0, −25.6,
  −18.5 and −15.9 dB. A hard key is 19.5 dB brighter (the share above 1.75 f0) than a soft one
  (40).
- **Swell and opening** (C4, velocity 100):
  - 0.1–0.4 s is 25.9 dB under 2.5–4 s.
  - The share above 1.75 f0 goes from −26 dB (0.2–0.5 s) to −5 dB at ~1 s, then settles to
    −9 dB by 2.5–4 s: +15 dB.
  - It holds within 0.1 dB, and is 56 dB down 2 s after release.
- **Clicks and DC:** the largest step at a key is 0.05 of the peak; no DC.
- **Spectrum at velocity 100**, partials relative to the note:

  | partial | fifth | 2f | 3f | 4f | 4.5f | 5f |
  |---|---|---|---|---|---|---|
  | dB | −6 | −6 | −10 | −33 | −20 | −56 |

  At 127 the 4f is −20 and the 5f −25; at 64 everything above 2f is under −27.
- **Keyboard:** A0–C8 within ±1.5 dB at velocity 100 after the voicing.
- **Level:** −30.9 LUFS untrimmed by `levels.py`'s measure() (it blooms through most of the
  1.5 s note); trim +2.7 dB to the median −28.2.
- **Velocity row:** `velcurve.py`'s `render()`/`gain_curve()` on the mean of C3 G3 C4 E4 C5.
  Its physical curve is already near GM's: −5.6 dB at velocity 8, within ±1.3 dB from 24 up.
- **Cost:** eight notes (4 strings × ≤ 6 modes, ≤ 22 wake cells a note) take ~2.9 µs/sample,
  14 % of a core.

### Compared with the SoundFonts (`tools/synthcompare.py profile`, program 98) [T]

pajupilli against FluidR3_GM, with TimGM6mb and Jnsgm2 as references (52 notes each, 2 s held):

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 |
|---|---|---|---|---|
| onset delay (ms) | 778 | 238 | 107 | 113 |
| attack time, −20 to −1 dB (ms) | 880 | 565 | 350 | 625 |
| level after 2 s held, re peak (dB) | −5.5 | −9.1 | −6.9 | −7.5 |
| release, 30 dB after note-off (ms) | 852 | 1170 | 1069 | 1071 |
| loudness range, velocity 40 → 127 (dB) | 7.6 | 16 | 17 | 17 |
| brightness (centroid, ERB-number, vel 96) | 8.6 | 9.1 | 10 | 10 |
| odd / even partials (dB) | −1.4 | −2.7 | −0.6 | −0.9 |
| noisiness (spectral flatness, dB) | −32 | −37 | −38 | −33 |
| Plomp–Levelt dissonance of a note (log2) | −5.4 | −2.2 | −1.2 | −1.3 |
| partial envelopes alike (correlation) | 0.83 | 0.06 | −0.06 | −0.05 |
| key-to-key level scatter (dB) | 2.9 | 0.24 | 0.4 | 0.23 |
| pitch scatter key to key (c) | 4.7 | 7 | 2.7 | 3 |
| stereo coherence | 0.45 | 0.52 | 0.92 | 0.92 |

The mean spectrum relative to FluidR3, level removed: 40–150 Hz +3, 150–400 Hz +1,
400–1000 Hz −3, 1–2.5 kHz −4, 2.5–6 kHz −5, 6–16 kHz +8 dB (the hiss).

What differs:
1. **It speaks late.** The strings bloom over ~0.8 s, where the samples sound at once. In a
   2 s note it has not reached full strength, so its measured velocity range is half theirs
   (held 6 s it spans 25 dB from velocity 20 to 127).
2. **It is purer.** It is a few locked sines with their beats, less rough than a detuned sample
   cluster.
3. **Its partials move together.** A string's modes grow as one, and the beating moves them
   alike.
4. **It brightens with the touch**, where the samples do not. It also opens over the first
   second; TimGM6mb's filter does the opposite.
5. **Each note beats in its own way**, so the key-to-key level scatter is ~3 dB.

What is alike: the fifth, the overall brightness, the partials up to 4.5f at velocity ~100, the
held level and the release. The first version had no felt and a 9 s bass T60: notes overlapped
in the profile and the release was 30 % longer. The second version's 0.65 s release was too
short, which led to the 2.5 s felt.

### Limitations

- **The onset is slow** (~0.8 s): fast passages on program 98 will sound late and soft. K2
  speeds it (×0.25).
- **The capture band is narrow:** a wind ±2 % off at the key leaves a note silent. The steady
  blower prevents that, and K1 (gusts) is subtle: once locked, a wake holds through 2.4 % gusts.
- **The key gain stands in for physics:** the voicing (f^−0.55) is a gain, not gauges or a
  soundbox.
- **The bridge coupling is chosen:** κ is set for the octave's level, not derived.

## FX 3 (crystal), GM 99

Built: `src/lead.cc` (`fx-crystal`), a bowed and struck glass plate; README "Synth leads".

## FX 4 (atmosphere), GM 100

### What it is in the classic sets

- **Roland's origin: the D-50's "Nylon Atmosphere" (1987).** Wikipedia's D-50 article lists
  "Atmosphere" and "Nylon Atmosphere" among the D-50 presets that "live on in every machine that
  conforms to the General MIDI specification" [web, wikipedia]. Synthmania lists "Nylon
  Atmosphere" as factory preset 27: "Typical L.A. synthesis sound. A beautiful and classic 1980s
  pad patch" [web, synthmania]. That it layers a sampled (PCM) nylon-guitar attack with
  synthesized pad partials, as L.A. synthesis does, is [memory]: its partials were not found
  on the web.
- **Roland GS (SC-55, SC-88), "Atmosphere":** the GS name for program 100 [web, polyphone / GM2
  list]. That the SC-55's tone is a nylon-like pluck over a soft pad in the D-50's line is
  [memory]: not heard or inspected here.
- **Yamaha XG, "Atmosphere"** (MSB 0, LSB 0). Its variations [web, Yamaha QY100 and P-515 data
  lists]: Warm Atmos (LSB 18), Hollow Release (19), Nylon EP (40), Nylon Harp (64), Harp Vox
  (65), Atmos Pad (66), Planet (67). The names say what the voice is built from: a plucked
  nylon or harp attack with a pad. The voice itself was not inspected.
- **FluidR3_GM, "Atmosphere"** [T]. The preset: attenuation 5 dB, reverb 20 %, chorus 10 %. One
  instrument, two layers.
  - **"Alien Strings" (L/R), panned ±25 %:** the detuned cluster of near-pure tones used in
    "Soundtrack" (section FX 2), root 84, following the key. Volume: no hold, a decay of 4752 tc
    (15.6 s to −100 dB: ~6.4 dB/s) to a sustain of 100 dB, so the pad **decays to silence**.
    Release 3073 tc (5.9 s). The filter is fixed at 11108 c (~5 kHz). LFO rates are set but
    nothing routes them.
  - **Nylon guitar:** nine samples (Nylon E3 .. E6 and an "Extension" over C6), looped at their
    ends. Attenuation 7 dB (the global zone). Hold 0 tc (1 s) in the lower zones, then a decay of
    6590 tc (~45 s to −100 dB) in the bass to 3102 tc (~6 s) at the top. Release 1902 tc (3 s).
  - **Rendered** (C4, velocity 100, TiMidity):
    - The level rises to its peak at 0.3 s (the pad's sample swells), then falls steadily: −9 dB
      at 1 s, −16 at 2 s, −26 at 4 s, −34 at 5 s.
    - In the first 0.1 s (the pluck), relative to the fundamental: 2f −16, 3f −19, 4f −6 dB;
      above 4 kHz −21 dB; off the harmonics −8 dB.
    - At 2–3 s: 2f −13, 3f −10, 4f −17, 5f −22 dB.
    - The L/R correlation falls from 1.0 to ~0.2 as the pad takes over.
- **TimGM6mb, "Atmosphere"** [T]: two instruments.
  - **"Atmosphere0", synth strings:** looped G2/G3/G4/D6 samples at 22 kHz, attenuation 12 dB.
    Attack 33 ms, then a 19.8 s decay to −18 dB. The low-pass (412 Hz, its mod envelope +3487 c)
    opens at once to ~3.1 kHz and closes over 0.86 s to ~0.96 kHz. Release 3.4 s.
  - **"Atmosphere1", nylon guitar:** "N Guitar" samples of only ~50 ms (a pluck's onset), each
    looped over one or two cycles; the top keys use "Steel AcGtr C6". Attenuation 6 dB. A
    1.1 kHz low-pass opened to ~10.7 kHz by the mod envelope and closed again within 0.75 s. So
    the pluck's decay is **a filter sweep on a looped cycle**. Hold 0.14 s, decay 19.8 s to
    −30 dB, release 1.2 s.
  - **Rendered:** an instant attack; −6 dB at 0.2–0.5 s; −2 dB at 1 s (the layers beat); then it
    holds at ~−9 dB. Centroid 430–650 Hz.
- **Jnsgm2, "Atmosphere"** [T]: the same synth strings as TimGM6mb (same envelopes and filter
  to a cent, 44.1 kHz samples). Over them, its "Nylon Guitar" instrument (preset attenuation
  15 dB): 0.06–0.5 s samples, looped; a 729 Hz low-pass opened ×8 and closed over 0.45 s;
  volume decaying over ~14 s; release 0.7 s. Rendered: it holds at −3 to −8 dB re the peak.

So the common core [T, web]: **a plucked nylon string with a string pad behind it**, speaking at
once. FluidR3's pad is the louder layer and both of its layers decay (~7 dB/s). TimGM6mb and
Jnsgm2 build the pluck from a short onset and a closing filter, and their pad holds ~8–9 dB
under the peak for as long as the key is down. None has a slow swell.

### How it is used in GM files

None of 397 MIDI files under `~/midi`, `~/Music`, `~/Downloads` and `~/Musici` selects program
100 [T]. So the design aims at what the name and the patches suggest [memory]: held chords and
arpeggios under a melody (new-age, film and game beds), at moderate velocities, and slow melodies.

### The physics: a string kept singing by feedback

- **The EBow** (Heet, US patent 4,075,921, 1978) [memory]: a hand-held pickup coil, an
  amplifier and a driver coil over one steel string. The pickup senses the string's motion. The
  amplified current drives a magnet that pulls the string in step with it, so the loop
  sustains the string indefinitely. Its amplifier clips, which limits the level. Reversing the
  driver's polarity ("harmonic mode") damps the fundamental and lets a higher mode take over.
- **The Magnetic Resonator Piano** (McPherson 2010) [paper]: an electromagnet over each note of
  a grand piano, driven from a pickup through a filter tuned to the note (or to a chosen
  harmonic). It gives swelling, held, pad-like tones, played from the keyboard, the touch
  setting the intensity. Berdahl, Backer and Smith's electromagnetically prepared piano
  (2005) is the same idea [paper]. The Moog Guitar (2008) sustains or damps each string this
  way [memory].
- **The loop as a negative damping** [memory, textbook]: a force in phase with a mode's
  velocity, F = G v, changes its decay rate from σ to σ − G φ_p φ_d / 2 (φ_p and φ_d are the
  mode's shape at the pickup and the driver). Past σ, the mode grows. A saturating amplifier
  limits it: with the current clipped at I, its fundamental is (4/π) I, and the mode settles
  where (4/π) I φ_d = 2 σ ω A. Through one saturating amplifier the strongest mode starves the
  others (mode competition).
- **A magnet on steel** [memory]: the force goes as the flux squared, so a permanent bias b₀
  makes it 2 b₀ i + i²: the current plus a little of its octave.
- **String losses** (Chaigne and Askenfelt 1994) [paper]: σ(ω) = b₁ + b₃ ω². The air and the
  support set the bottom; the wire's internal friction sets the top. A plucked string's upper
  partials die within a second, and a drive at the fundamental can hold only the low ones.

### Design: a keyed psaltery with magnetic resonators

What the patch must do: speak at once with a pluck, carry a held, soft, chorused tone behind it
for as long as the key is held, play chords, and follow the keys. Four physical candidates were
weighed:

1. **A plucked string and a pad, side by side.** This is the most literal reading. Rejected:
   it is two instruments that do not interact, and the pad half would be one of
   `src/lead.cc`'s pads (`pad-halo` is already the fallback).
2. **A string blooming after the pluck** (a tanpura's jawari, sympathetic strings).
   Rejected: it decays and cannot hold while the key is held, nor stop when it is released.
3. **An acoustic guitar in feedback with a loudspeaker through the room.** This is the
   literal "atmosphere", but it was rejected: the loop locks onto whichever partial the room's
   phase favours, and in a chord the strings cross-feed through the shared body.
4. **A plucked string sustained by a magnet (chosen).**
   - **One system, both layers:** the pluck is the attack, and the same string held by the loop
     is the pad. As the plucked string's upper partials die, the timbre turns from a pluck into a
     held, pure tone.
   - **Prompt:** the pluck sounds within milliseconds. The loop only has to hold what the pluck
     gave, so nothing has to bloom (unlike the aeolian harp's 0.8 s).
   - **Playable:** a magnet per string (as in the MRP) plays chords; its band-pass follows the
     key.
   - **Chorus:** a course of three strings a few cents apart beats.
   - **A real control:** the amplifier's limit is the held level (the touch, K1). K1 at 0 gives
     a plain plucked string, which is FluidR3's decaying patch.
   - **Not a duplicate:** the aeolian harp (`src/soundtrack.cc`) is a self-oscillation driven
     by wind (a wake oscillator). The overdriven guitar's feedback bloom (`src/guitarinst.cc`)
     is a held mode given back what it loses, not a loop. The bowed glass plate and the glass
     bowls are struck or rubbed glass.

The model (`src/atmosphere.h` has the summary):
- **The course:** three steel strings, B = 4·10⁻⁵, up to 28 modes each (below 12 kHz). The
  outer strings are tuned ±(0.5–1.5) c × K4 (random per note); the middle one is within 0.3 of
  that of the note. Losses: σ = 6.9 / T₁ + 3·10⁻⁸ ω², with T₁ = 9 s × (f/262)^−0.35, clamped to
  3–16 s.
- **The plectrum:** at 0.27 of the length (±0.01), its leather 0.015 wide (each mode's shape
  averaged over it).
  - It presses with a raised-cosine force over (8 − 5v) ms. The force is set so that the string
    is pressed aside by v at the plectrum: its static shape summed over the modes.
  - It slips off over (0.4 − 0.3 v) ms × 3^(1−2·K3).
  - It meets the strings 0, 1.5–3.5 and 4–7 ms apart, at shares 0.85, 1 and 0.75.
- **The loop, per string:**
  - The pickup (at 0.11, 0.02 wide) senses the velocity.
  - Two band-passes (Q 6) at the note and at its second mode, each with gain κ = 4 times its
    mode's loss (2κσ / (φ_p φ_d)).
  - Each channel saturates on its own (tanh). The note's limit holds the fundamental at
    1.2 · K1 × the pluck's fundamental (default 0.6×; × (f/262)^0.3, at least 0.5, in the bass).
    The octave's limit holds the second mode at 0.25 · K2 of that.
  - The coil: a one-pole low-pass at 3 kHz (its inductance).
  - The force: i (1 + 0.3 i / I), onto each mode through its shape at the magnet (0.16).
  - The magnet's strength is 1 over the middle string and 0.35 and 0.3 over the side strings.
- **Output:** the slope at the bridge, Σ j y_j, through a 20 Hz high-pass. The top octave is
  lifted by (f/2093)^1.8 (its few modes). The strings are panned ±0.15 around the key's place
  (`takeSide`).
- **Release:** the magnets ramp off in 20 ms. A felt damper adds a decay of
  6.9 / (2.4 s × (f/262)^−0.25) × (1 + 0.15 j).

Tuning that mattered [T]:
- **One amplifier for two bands failed.** The octave never held: the saturating fundamental
  lowered the amplifier's small-signal gain for everything else (mode competition). Two
  channels summed at the coil hold both.
- **The chorus.** With three equal magnets and ±1.5–4.5 c, the held note (nearly a pure
  fundamental) beat as a whole by 10–15 dB. Weaker side magnets and ±0.5–1.5 c make it breathe
  by 1–2 dB over seconds.
- **The plectrum's place.** At 0.17 of the string the pluck was mid-heavy (2f and 3f above the
  fundamental; tristimulus 1 = 0.33 against the sets' 0.46–0.85). At 0.27 it is round
  (0.54).
- **What the loop cannot hold.** κ from 4 to 10 and B from 4·10⁻⁵ to 10⁻⁵ hardly changed the
  held spectrum (4f and 5f stay at ~−30 dB). A drive at one place pays for the upper modes'
  ω² losses only weakly. A magnetically held string is purer than a sampled string pad.

### Measurements [T]

- **Tuning:** the power-weighted pitch (1.5–5 s) is −0.7 to +0.3 c from C2 to C7.
- **Touch:** a C-major chord, 0–6 s, at velocities 20, 60, 100 and 127 measures −38.9, −29.2,
  −24.8 and −22.1 dB.
- **The pluck and the pad** (E4, velocity 100, the course in unison):
  - The first 20 ms are 0.8 dB under the peak, which comes at 10 ms.
  - The share above 3.5 f0 falls by 24 dB from the pluck (0–0.1 s) to 2.5–4 s.
  - 4–8 s is within 0.2 dB of 2.5–4 s, and 7.2 dB under the peak.
  - With K1 at 0 it is 34 dB lower at 4–6 s.
  - K2 lifts the octave from −24 dB (K2 at 0) to −8 dB (K2 at 1) re the fundamental.
  - 1.5 s after the release it is 59 dB down.
- **The chorus:** the 250 ms level's spread over 2–10 s is 1.3, 1.7 and 0.8 dB (G3, E4, C5).
- **Clicks and DC:** the largest step at a key is 0.19 of the peak; no DC.
- **Spectrum** (C4, velocity 100), partials relative to the fundamental:

  | time | 2f | 3f | 4f | 5f | 6f |
  |---|---|---|---|---|---|
  | 0–0.1 s (the pluck) | +4 | −4 | −12 | −4 | −6 |
  | 2–3 s | −6 | −9 | −32 | −32 | −53 |
  | 4.5–5.5 s | −13 | −14 | −30 | −36 | −64 |

- **Keyboard:** the first second is within ±2.5 dB from A0 to C8 (after the top octave's
  lift). The held note (1.5–3 s) is within ~±3.5 dB; the bass is louder, since it rings by
  itself.
- **Level:** −22.5 LUFS untrimmed by `levels.py`'s measure(); trim −5.7 dB to the median −28.2.
- **Velocity row:** `velcurve.py`'s `render()`/`gain_curve()` on the mean of C3 G3 C4 E4 C5.
  Its physical curve spans 25 dB from velocity 8 to 127 (TiMidity's 40). The row runs from
  −12.6 dB at velocity 8 to +1.6 at 127.
- **Cost:** eight notes take ~2.9–3.3 µs/sample, 14–16 % of a core.

### Compared with the SoundFonts (`tools/synthcompare.py profile`, program 100) [T]

pajupilli against FluidR3_GM, with TimGM6mb and Jnsgm2 as references (keys a few semitones
apart, velocities 40/64/96/127, 2 s held):

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 |
|---|---|---|---|---|
| onset delay (ms) | 15 | 23 | 8 | 12 |
| attack time, −20 to −1 dB (ms) | 37 | 213 | 165 | 28 |
| onset sharpness (dB/ms) | 17 | 8.4 | 11 | 6.5 |
| attack brighter than the body (ERB) | −1.2 | 2.0 | −0.3 | 0.2 |
| level after 2 s held, re peak (dB) | −14 | −17 | −11 | −10 |
| decay rate of a held note | 3.5 | 7.9 | 1.4 | 1.7 |
| release, 30 dB after note-off (ms) | 780 | 1041 | 631 | 734 |
| loudness range, velocity 40 → 127 (dB) | 14 | 17 | 17 | 17 |
| brightness (centroid, ERB-number, vel 96) | 8.4 | 9.2 | 11 | 10 |
| fundamental share (tristimulus 1) | 0.54 | 0.85 | 0.46 | 0.79 |
| partials 2–4 share (tristimulus 2) | 0.26 | 0.15 | 0.47 | 0.17 |
| noisiness (spectral flatness, dB) | −61 | −50 | −45 | −36 |
| partial-to-partial jaggedness (dB) | 5.2 | 1.4 | 3.1 | 1.9 |
| key-to-key level scatter (dB) | 3.1 | 0.6 | 1.2 | 0.8 |
| pitch wobble (c p-p) | 4.2 | 29 | 14 | 30 |
| stereo coherence | 0.29 | 1 | 1 | 1 |

The mean spectrum relative to FluidR3, level removed: 40–150 Hz +5, 150–400 Hz +3,
400–1000 Hz +5, 1–2.5 kHz +6, 2.5–6 kHz −4, 6–16 kHz −16 dB.

What differs:
1. **It is purer and darker on top.** The held note is a few locked partials, and the pluck's
   top dies with the string's ω² losses. Little lies above 6 kHz (FluidR3's nylon sample keeps
   a bright attack). It is less noisy and more jagged.
2. **Its attack is sharper**, and duller than its body: the plectrum's slip against a sample's
   recorded onset, under a pad sample that swells (FluidR3's attack time is 213 ms because its
   pad is the louder layer).
3. **Each key beats in its own way** (key-to-key level scatter ~3 dB), and the note has no
   vibrato: the samples have a 14–30 c LFO wobble.
4. **It is wider in stereo.** This comes mostly from pajupilli's room (the soundtrack measured
   0.45 the same way), plus the course's ±0.15 spread.
5. **It holds.** FluidR3 decays at ~7 dB/s; TimGM6mb and Jnsgm2 hold, as this does. K1 at 0 is
   FluidR3's behaviour.

What is alike: a prompt onset, the level after 2 s, the release, the velocity curve within
~2 dB, the overall brightness and the fundamental's share (between TimGM6mb's and FluidR3's).

### Limitations

- **The held note is nearly a pure tone.** A drive at one place cannot hold the upper modes
  against their ω² losses, so the pad is thinner than the sets' string samples. K2 adds the
  octave.
- **The plectrum's and magnets' constants are chosen,** not taken from a built instrument: the
  limit (0.6× the pluck's fundamental), κ = 4, the bias 0.3, and the side magnets 0.35/0.3.
- **The voicing is a gain:** the top octave's lift and the bass's weaker magnets stand in for
  scaling and a soundboard, which are not modelled (the output is the bridge force).
- **K3 and K4 act on new notes only** (they set the pluck and the course's tuning); K1 and K2
  act at once.
- **The chorus's level wanders slowly** (beat periods of seconds), so two plays of one key
  differ by up to ~3 dB.

## FX 5 (brightness), GM 101

### What it is in the classic sets

- **Roland GS (SC-55 onward), "Brightness"** (program 101, bank 0) [web, polyphone / GM2 list;
  dtm wiki SC-55 map]. Synthmania's SC-50 page calls it "Fairlight type of sound" [web,
  synthmania SC-50]. A Japanese column lists "101 000 Brightness (2 voices), range C2–C7", glosses
  the name as radiance or clarity, and hears "a metallic, string-like quality" [web, soundhouse
  GM SFX column; read through a search summary, the page itself timed out]. **D-50 heritage: not
  found.** The pages that trace "Fantasia", "Soundtrack", "Atmosphere" and "Ice Rain" to the
  D-50 do not mention "Brightness", and no D-50 preset of that name turned up; that it is a D-50
  patch is not supported. The sound itself was not heard or inspected.
- **Yamaha XG, "Brightness"** (MSB 0, LSB 0) [memory]. The XG voice lists (MU80, QY100) could not
  be fetched (Yamaha's PDFs returned 404/403), so its elements and variations are not known here.
- **FluidR3_GM, "Brightness"** [T]. The preset: attenuation 5 dB, chorus 20 %, reverb 20 %. One
  instrument, a global release of 2786 tc (5 s), two layers, no volume decay (it **holds**):
  - **"Synth Strings 1" (L/R)**, panned ±50 %: 2.15 s samples looped over their last 0.59 s,
    overriding root 84 (the sample is a C6 cluster: key 60 plays it two octaves down). Its
    partials fall 0, −6, −10, −15, −18 dB; it is a detuned cluster (much energy between the
    harmonics).
  - **"Bright"**: a 0.72 s sample at 44.1 kHz, looped over its whole length (8..31819), root 84,
    attenuation 5 dB, chorus 75 %, release 5186 tc (20 s). It is one note at C6 (1046 Hz), a
    chorused cluster (power-weighted spread 32–43 c), its partials 0, −5..−7, −11, −15, −16, −19,
    −22, −21, −23, −26, −28 (11th), −36 (12th) dB: **the octave nearly as strong as the note,
    then ~2 dB down a partial**. Its level falls 29 dB from the loop's start to its end, so
    looped it re-attacks every 0.72 s at C6 (2.9 s at C4).
  - **Rendered** (TiMidity, velocity 100): C4 holds, swelling and sagging by ~8 dB on the Bright
    loop's 2.9 s period; partials at 0.1–0.4 s 0, −1, −7, −14, −15, −19, −21; at 2–4 s 0, −8,
    −8, −16, −18 dB; centroid 1.5–1.8 × f0; above 4 kHz −36 to −43 dB; L/R correlation 0.03.
    C6: 0, −2, −10, −10, −14, −16; C7: 0, −1, −8, −13, −15, −23; C8: 0, −8, −13, −18, −22, with
    −21 dB of the sound past 20 kHz. What lies off the harmonics is −4 to −7 dB of the whole (the
    clusters).
- **TimGM6mb, "Brightness"** [T]: two instruments.
  - **"Brightness0", Solo Vox:** "Solo Vox C4" (0.25 s, looped over it; it holds a cluster near
    D6, root 86) for keys 0–94, "Solo Vox C7" above. A low-pass at 11564 c (6.5 kHz), Q 1.3 dB,
    its mod envelope +562 c falling away in 0.29 s. The volume: decay 3747 tc (8.7 s to
    −100 dB) to silence, so it **decays** ~11 dB/s; release 2316 tc (3.8 s).
  - **"Brightness1", synth strings:** G2/G3/G4/D6 at 22 kHz, 14 dB down, a 102 Hz low-pass
    opened by its mod envelope to ~4.4 kHz, attack 22 ms, hold 0.14 s, the same decay.
  - **Rendered:** C4 peaks at once and falls −9, −13, −21, −23 dB over 2 s; partials 0, −4,
    −13, −15, −29, −19, −29, −18, −29: the even partials 6, 8, 10 stand ~10 dB over the odd ones
    around them (the vox's formant); centroid 1.7–2.1 × f0; above 4 kHz −21 to −24 dB.
- **Jnsgm2, "Brightness"** [T]: one instrument, the same Solo Vox zones (C3, G3, C4, C5, C7;
  0.03–0.16 s loops) with TimGM6mb's filter and envelopes to the cent, and its synth strings
  (G2..D6) **an octave up** (coarse tune +12), 15.7 dB down. Rendered: C4 has its octave 3–6 dB
  over the note, the third +1 dB, then −7 to −20 dB; centroid 2.5–2.7 × f0 (1.8 at C6); it
  decays as TimGM6mb does.

So the common core [T]: **a bright, held tone whose octave is as strong as the note and whose
partials fall steadily**, speaking at once, its brightness from the octave and a long series
rather than from a filter sweep. It is not a struck bell or a glassy ping. FluidR3's holds;
TimGM6mb's and Jnsgm2's die in ~3 s. The samples are detuned clusters (a chorus); none of the
sets brightens with velocity.

### How it is used in GM files

Of 397 MIDI files under `~/midi`, `~/Music`, `~/Downloads` and `~/Musici`, one has a program
change to 101: "Love Is Here to Stay" (`~/Music/midi/bushgrafts`), channel 3 ("Flute then
Vibes"), and it is replaced by 12 (vibraphone) one tick later: nothing is played on it [T]. So
the design aims at what the patch suggests [memory]: held chords and pads, slow melodies over
them, at moderate velocities.

### The physics: the singing arc

- **Duddell's singing arc** (Duddell 1900) [paper, memory]: a DC carbon arc, fed through a
  ballast resistance (and a choke), with a coil and a capacitor in series across it. The arc's
  falling characteristic (its voltage drops as its current rises) is a negative resistance that
  feeds the tuned circuit, and it oscillates at about the circuit's frequency; the arc itself
  sounds the note. Duddell played tunes on it from a keyboard switching the capacitors [memory].
  If the swing is small the current never stops (the "musical arc", type I); if it is large
  the arc goes out each cycle and relights (type II), a harsh relaxation. The Poulsen arc
  (1903) is the same circuit at radio frequency [memory].
- **The arc's characteristic** (Ayrton 1902) [memory]: V = a + b l + (c + d l) / I: the electrode
  falls a, a column growing with its length l, and the column's falling part. A longer arc, a
  steeper fall.
- **The dynamic arc** (Mayr 1943) [paper]: the arc's conductance g follows its energy balance
  with a thermal time constant τ: dg/dt = (g/τ)(v i / P(i) − 1) = (i² / P(i) − g)/τ. Slower than
  τ the arc follows its static characteristic (a negative resistance); faster, it is a
  resistance. So it can drive a circuit only at frequencies up to ~1/τ, and its lag is a
  reactance in the loop (it pulls the pitch).
- **How it sounds** [memory; Arnold and Crandall 1917, paper]: the arc's column is hot gas; its
  power swings, the column swells and shrinks, and the air outside moves as the rate of change
  of the power (the thermophone's monopole: p ∝ dP/dt). Power is current times voltage: with the
  current swinging about its mean, the power holds the note (the mean current times the swing)
  and its octave (the swing squared), so a strongly driven arc has an octave as strong as the
  note: the classic patch's defining trait, from the physics. Simon's "speaking arc" (1898)
  played speech the same way [memory].

### Design: a keyboard of singing arcs

What the patch must do: speak at once, hold while the key is held, carry the octave as strongly
as the note with partials falling steadily, play chords, follow the keys, and not be harsh at the
top. The candidates:

1. **A pad and a bright layer, side by side** (the literal reading). Rejected: two instruments,
   and the pad half is `src/lead.cc`'s (`pad-metallic` is the fallback).
2. **Rubbed or bowed glass** (a glass harmonica, the Cristal Baschet). Rejected: the bowed glass
   plate (`fx-crystal`) and the glass bowls (`fx-rain`) are glass already, and a rubbed rim is
   slow to speak.
3. **A wheel bowing strings from keys** (Haiden's Geigenwerk). Rejected: the bowed-string family
   already models stick-slip strings.
4. **A string held by a magnet.** Rejected: that is `fx-atmosphere`.
5. **A thermoacoustic tube** (a Rijke tube, a singing flame). Rejected: it blooms over tenths of a
   second, its pitch is the tube's, and a flame a key is hard to key.
6. **Duddell's singing arc (chosen):**
   - **Its sound is the patch:** the octave comes from power = v i, the brightness from the
     limit cycle, not from a filter; the partials are exact harmonics (the arc's swing is
     periodic).
   - **Prompt:** the tank, switched across a burning arc, is struck by the arc's voltage and
     rings at once; nothing has to bloom (but the softest treble notes, ~20 ms).
   - **Holds** while the key is held (a self-oscillation), and stops when the current is run
     down.
   - **Playable:** an arc and a tank a key (Duddell's keyboard switched one arc's capacitors: a
     bank of arcs is the polyphonic form).
   - **A real touch:** velocity is the arc's current and its length (a longer arc, a fiercer,
     brighter limit cycle).
   - **Not a duplicate:** no instrument here is a gas discharge or a negative-resistance
     oscillator; `fx-sci-fi`'s "discharge" is a wavefolder.

The model (`src/brightness.h` has the summary), in units of the arc's operating point (its
current and voltage) and of the tank's period (L C = 1):
- **The circuit:** a supply E = 21 through a ballast of 20 (nearly a constant current); the tank's
  impedance √(L/C) = 1 and its Q 30; the arc's τ = 0.5 / the tank's angular frequency.
- **The arc:** Mayr's, its loss P = φ (1 + flicker) + (1 − φ)|i|, φ the column's share (the
  arc's length), plus a residual conductance 0.02 (the hot electrodes relight it).
- **The pressure:** dP/dt per radian of the tank (each arc's power scaled with its period, so all
  keys are alike).
- **Tuning:** the arc sings at ρ ≈ 0.61–0.67 of its tank (by φ; the trapezoid's own warping
  too), from a table made at start: 21 values of φ (0.27–0.67) by 8 step sizes (24–256 steps a
  period), each run 30 periods to settle and 40 counted (zero crossings of the tank's current).
  Above 256 steps the integrator's first-order error is carried on to none. ~15 ms at start.
- **The key:** the arc burns silent at its operating point (found by bisection). The key's
  slide-wire hands it to the tank (a share s of the arc's voltage, 0 → 1 as a raised cosine over
  1.0–1.5 ms, at least 1.5 radians of the tank), and its rheostat brings the current up (a
  raised cosine over 3–7 ms). φ = 0.30 + 0.10 v ± 0.15 (K1); the current 0.04 + 0.96 v^1.6.
- **Two arcs a key:** the large one on the note, the small one (0.35 of its sound) 0–12 c off
  (K2; 3.6–6 c by default), on the other side (±0.3 about the key's place, sides alternating with
  the key).
- **The flicker** (K3): white noise low-passed at 3 kHz, 0–4 % RMS of the column's loss (1 % by
  default).
- **Release:** the current runs down (−60 dB in 0.5–8 s, K4; 2 s by default), the arc keeping its
  shape.
- **Integration:** 4× oversampled; the tank by the trapezoid with the arc's conductance held over
  the step, the conductance then relaxed exactly; the guitar amp's 8th-order elliptic low-pass
  (`oversample_data.h`) back down.

Tuning that mattered [T]:
- **A current source made a type II arc.** Fed from an ideal current source, the arc went out
  each cycle and relit: bursts at ~12 Hz, partials flat to the 20th. A voltage supply through a
  ballast (the arc in parallel with it, seen from the tank) gives the musical arc's steady limit
  cycle.
- **The circuit's constants.** A sweep (ballast 2–20, tank impedance 0.25–1, τω 0.3–0.7, φ
  0.25–0.7) found a tank impedance of 1 and a ballast of 20 gave the classic's shape (octave ≈
  note, partials falling, centroid 1.5–2.5 × f0) varying smoothly with φ. A tank impedance of
  0.25 left the partials flat up to the 8th (harsh); τω = 0.5 has the lowest threshold (φ ≈ 0.27).
- **Which is the note.** The power's strongest partial can be the octave of the tank's current,
  so the first prototype took the octave for the note. The pitch is counted on the tank's
  current.
- **The integrator.** Implicit Euler damped the tank numerically at high notes (a Q of ~9 at 56
  steps a period), and above C6 the arc fell into period doubling (a subharmonic at f0/2, −16
  dB). The trapezoid keeps the spectrum over f0 the same at every key and what lies off the
  harmonics under −65 dB.
- **The start.** Switching the tank onto the arc at once made the tank's current turn on with a
  step in its slope, a click (a step of 0.47 of the peak). A contact resistance in the tank made
  the trapezoid ring. The slide-wire (a share of the voltage, rising smoothly) starts it cleanly
  (0.06). Its travel trades sharpness against delay (2.5 ms: 7 dB/ms but ~30 ms to speak in the
  treble; 1 ms plus the rheostat: 11 dB/ms, 18 ms).
- **The release.** Running the supply down moved the arc's operating point and pulled the pitch
  down 6 semitones. A damping resistor switched into the tank collapsed the limit cycle in
  ~50 ms and left the arc's flicker hissing. Running the current down with the arc keeping its
  shape holds the pitch.
- **The chorus.** Two equal arcs beat deeply and pulled the mean pitch 3 c flat (they were
  ±d/2). A large arc on the note and a small one off it breathe by ~1 dB.

### Measurements [T]

- **Tuning:** the power-weighted pitch (1–3 s): +0.8, +1.2, −1.2, −0.3, −0.7, −0.6, −0.7,
  −1.0 c at A0, C2, C3, C4, C5, C6, C7, C8.
- **Touch:** a C-major chord, 0–6 s, at velocities 20, 60, 100 and 127: −40.2, −28.8, −22.3 and
  −19.1 dB.
- **The arc** (C4, velocity 100, one arc's pitch, no flicker):
  - the first 20 ms are 1.3 dB under the loudest 20 ms; 4–8 s is within 0.0 dB of 1–3 s;
  - partials 2–6: −1, −10, −13, −12, −17 dB; 8–12 at most −21 dB; 2.5–3 s after the release
    −82 dB;
  - the centroid is 1.88, 1.88 and 1.90 × f0 at C3, C4 and C6; K1 from 0 to 1 moves it from
    1.58 to 2.42 × f0.
- **The top** (velocity 127, one pitch, no flicker), C6, C7, F#7, C8: what lies off the
  harmonics −93, −93, −78, −66 dB; past 20 kHz −73, −62, −32, −30 dB; the partials from the
  fifth up at most −11, −10, −10, −27 dB re the note. (FluidR3 at C7: 0, −1, −8, −13, −15, −23;
  at C8 −21 dB past 20 kHz.)
- **The chorus:** the 250 ms level's spread over 2–10 s is 1.2, 1.0 and 0.8 dB (G3, E4, C5).
- **Clicks and DC:** the largest step at a key is 0.06 of the peak; no DC.
- **Level:** −28.3 LUFS untrimmed by `levels.py`'s measure(); trim +0.1 dB to the median −28.2.
- **Velocity row:** `velcurve.py`'s `render()`/`gain_curve()` on the mean of C3 G3 C4 E4 C5. Its
  physical curve spans ~27 dB from velocity 8 to 127 (GM's 40). The row runs from −13.1 dB at
  velocity 8 to −0.1 at 127.
- **Cost:** eight notes take ~1.3–1.5 µs/sample, 6–7 % of a core.

### Compared with the SoundFonts (`tools/synthcompare.py profile`, program 100) [T]

pajupilli against FluidR3_GM, with TimGM6mb and Jnsgm2 as references (keys a few semitones
apart, velocities 40/64/96/127, 2 s held):

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 |
|---|---|---|---|---|
| onset delay (ms) | 18 | 4 | 8 | 5 |
| attack time, −20 to −1 dB (ms) | 46 | 159 | 53 | 18 |
| onset sharpness (dB/ms) | 11 | 5.2 | 7.7 | 7.1 |
| attack brighter than the body (ERB) | −0.3 | 1.5 | −0.8 | −0.1 |
| level after 2 s held, re peak (dB) | −5.7 | −8.4 | −28 | −27 |
| release, 30 dB after note-off (ms) | 882 | 1110 | 791 | 828 |
| loudness range, velocity 40 → 127 (dB) | 16 | 16 | 17 | 17 |
| brightness (centroid, ERB-number, vel 96) | 10 | 10 | 10 | 11 |
| brightening, velocity 40 → 127 (ERB) | 0.7 | 0.0 | 0.0 | 0.0 |
| fundamental share (tristimulus 1) | 0.48 | 0.52 | 0.75 | 0.19 |
| partials 2–4 share (tristimulus 2) | 0.39 | 0.44 | 0.23 | 0.80 |
| partials 5+ share (tristimulus 3) | 0.06 | 0.03 | 0.02 | 0.02 |
| odd / even partials (dB) | −5.8 | −7.1 | −12 | −8.8 |
| noisiness (spectral flatness, dB) | −40 | −40 | −32 | −26 |
| Plomp-Levelt dissonance (log2) | −4.3 | −1.8 | −2.6 | −2.8 |
| key-to-key level scatter (dB) | 1.7 | 0.5 | 0.3 | 0.4 |
| key-to-key brightness scatter (ERB) | 1.2 | 0.5 | 0.7 | 0.6 |
| pitch wobble (c p-p) | 4.8 | 54 | 29 | 25 |
| stereo coherence | 0.38 | 0.13 | 0.99 | 1 |

The mean spectrum relative to FluidR3, level removed: 40–150 Hz +1, 150–400 Hz +2,
400–1000 Hz +2, 1–2.5 kHz +2, 2.5–6 kHz +1, 6–16 kHz −4 dB. The velocity curves agree within
0.5 dB.

What differs:
1. **It is smoother.** Its partials are exact harmonics of one limit cycle (plus a faint
   flicker); the samples are detuned clusters with a 25–54 c LFO wobble. Plomp-Levelt roughness
   −4.3 against −1.8 to −2.8.
2. **Its onset is sharper and a little later.** The tank is struck at once (11 dB/ms against
   5–8), but soft treble notes bloom over ~20 ms (the arc near its threshold); 18 ms against
   4–8 ms (pajupilli's figure includes its room).
3. **It brightens with the touch** (0.7 ERB), the samples not at all.
4. **Keys scatter** by ~1.7 dB and ~1.2 ERB: the two arcs beat slowly, each key in its own phase
   over a 2 s note.
5. **A little darker above 6 kHz** (−4 dB), and a little richer in partials 5 and up (0.06
   against 0.02–0.03).

What is alike: the overall brightness, the fundamental's and the octave's shares, the odd/even
balance (between the sets), the hold (as FluidR3's), the release, the loudness range and the
velocity curve, the noisiness, the stereo width (between FluidR3's and the others').

### Limitations

- **The arc is idealised.** Mayr's model with a constant column loss and a residual conductance;
  no electrode burn-back, no cathode spot wandering, no choke (the ballast is a resistor), no
  arc lamp's slow flicker; the circuit's constants (ballast 20, impedance 1, Q 30, τω 0.5) were
  chosen by a sweep for the classic's spectrum, not from a built arc.
- **Self-similar scaling** (each key's arc sized to its period) makes the keyboard even, which a
  bank of real arcs would not be.
- **The release is a gain:** the current is run down with the arc keeping its shape (a real
  arc's operating point would move, and its pitch with it).
- **No vibrato.** The samples' 25–54 c LFO is not modelled; the arc only flickers.
- **The tuning table** is made at start (~15 ms); K1 and K2 act on new notes only.
- **The treble's softest notes bloom** (~20 ms), the price of an arc near its threshold.

## FX 6 (goblins), GM 102

### What it is in the classic sets

SF2 times below are converted from timecents (2^(tc/1200) s) and frequencies from absolute
cents (8.176 · 2^(c/1200) Hz).

- **Roland GS (SC-55 onward), "Goblin"** (program 102, bank 0).
  - A Japanese column lists it with two voices and a C2–C7 range, and describes it as heavily
    sweeping synth voices; the name is the fantasy creature [web, soundhouse GM SFX column; read
    through a search summary, the page itself timed out].
  - **D-50 heritage: not found.** The pages that trace "Fantasia", "Soundtrack" and "Atmosphere"
    to the D-50 do not mention a goblin, and Sound House ties another FX program ("Crystal") to
    the D-50 but not this one. Nothing supports a D-50 origin. The sound itself was not heard or
    inspected.
- **Yamaha XG, "Goblins"** (MSB 0, LSB 0, program 102; two elements) [web, Yamaha lists; QY100
  and QY700 data lists, read through a search summary]. The same summary lists bank variations
  near it ("GobSynth", "Creeper", "Ritual", "Night"); their placement was not checked against the
  PDF. The elements' make-up is not known here.
- **FluidR3_GM, "Goblin"** [T].
  - The preset: attenuation −10 dB (a boost), reverb 14 %, chorus 10 %.
  - The instrument's global zone applies to all three layers:
    - a low-pass at 5535 c (~200 Hz), opened by the modulation envelope by +5000 c (× 18, to
      ~3.6 kHz);
    - the modulation envelope's attack is 8.9 s (then it decays in 47 ms to its sustain, which is
      full) and its release 3.0 s;
    - the volume's attack is 0.77 s, its sustain full, its release 5.0 s.
  - **"SynthVz C 7(L)":** 0.97 s, looped over its last 0.48 s, root 84 (C6), 10 dB down. Its note
    and octave are equal (+1 to +3 dB), then −24 to −30 dB. At C4 it plays two octaves down, so
    its loop comes round every ~1.9 s.
  - **"Wind":** a 0.25 s noise loop, root 76, its own low-pass at 7935 c (~800 Hz), no envelope
    on the filter, a volume attack of 1 s.
  - **"Vox C4":** 1.5 s, root 72, 51 dB down (511 cB). Its filter has 3 dB of Q. A modulation LFO
    at 2396 c (33 Hz) moves the pitch +100 c, and the vibrato LFO (the default 8.2 Hz) −100 c: a
    growl. Its partials are 0, +4, 0, +1, −10, −11 dB (a bright "aah").
- **TimGM6mb, "Goblin"** [T]: two instruments.
  - **"Goblin0", the syn vox:** "Syn Vox C4" (12 kHz, a 0.52 s loop, root 80) for keys 0–93, and
    "Aahs C7" above.
    - At its root the vox is nearly a sine: h2 −9 to −13 dB, h3 −31.
    - A low-pass at 5491 c (~195 Hz) with 4 dB of Q, opened by +5737 c (to ~5.4 kHz) by the
      modulation envelope, which waits 0.28 s and then rises over 8.9 s.
    - The volume: attack 0.77 s, release 5.0 s.
  - **"Goblin1", the sitar:** "Sitar Eb3", "C5" and "Eb6" loops (a few ms to 0.1 s), 60 dB down
    (600 cB).
    - A low-pass at 4484 c (~100 Hz) with **19 dB of Q**, opened by +4950 c (to ~1.75 kHz) over
      the same 8.9 s attack, held 2.4 s, then falling over 28 s.
    - A modulation LFO at 10.5 Hz swings that filter by −2756 c.
    - **A vibrato of ±609 c (six semitones) at 8.5 Hz.**
    - The volume: attack 1.84 s, decay over 19.8 s to −16 dB, release 4.1 s.
- **Jnsgm2, "Goblin"** [T]: TimGM6mb's two layers in one instrument at 44.1 kHz ("Syn Vox C3",
  "F#3" and "C4", "Aahs C6"; "Sitar Eb3", "Eb4" and "Eb6"), with the same filters, envelopes and
  LFOs to within a few units.
- **Rendered** (TiMidity++ 2.14, velocity 100, C4 held 14 s) [T].
  - TiMidity++ does **not** sound the modulation envelope's filter sweep. With or without
    `--mod-envelope` the centroid stays put for 14 s in FluidR3. In TimGM6mb, with it on, only the
    sitar's resonant peak is seen to climb: h4 strongest at 3 s, h6 at 5 s, h7 at 7 s.
  - **FluidR3:**
    - it reaches −4 dB of its peak in 0.5–1 s, then swings ±1.5 dB on the SynthVz loop's ~2 s
      period (its centroid alternating 1.6–2.0 × f0);
    - partials 0, 0, −20, −20, −26, −19 dB, with what lies off the harmonics (the wind) at −9 to
      −14 dB;
    - its upper partials move at 5.3 Hz, and those above the eighth at 31 Hz (the growl);
    - L/R correlation 0.16.
  - **TimGM6mb:** centroid 1.05–1.15 × f0 (the dark syn vox); h2 −8 to −14 dB, the rest −30 to
    −50; the upper partials move at 8.5 Hz (the sitar's vibrato); the level swings ±3 dB at
    ~0.6 Hz.
  - **Jnsgm2:** steady; centroid 1.23–1.25 × f0; h2 −7, h3 −26, h4 −22 dB; off the harmonics −16
    dB.

So the common core, checked against the data rather than assumed [T]:
- **a dark voice:** a "syn vox", near a sine at its root, or with its octave as strong;
- **a slow attack:** 0.77 s (the sitar's 1.84 s);
- **a long opening:** a low-pass from ~100–200 Hz, opened by a modulation envelope with an 8.9 s
  attack. This is in all three; in TimGM6mb and Jnsgm2 the sitar's filter is strongly resonant,
  so a peak walks up the partials. So the swell differs from the attack: it is a sweep in timbre
  more than in level;
- **a fast warble in the upper partials:** at 5–9 Hz (the sitar's six-semitone vibrato through a
  resonant filter is the bubbling), or a 33 Hz growl in FluidR3;
- **some wind** in FluidR3.

There is no pitch glide beyond the LFOs. None of the sets brightens with velocity.

### How it is used in GM files

Of 397 MIDI files under `~/midi`, `~/Music`, `~/Downloads` and `~/Musici`, one has a program
change to 102: "Louisiana" (`~/Music/midi/bushgrafts`), channel 6. It is replaced by program 29
at the same tick, so nothing is played on it [T]. So the design aims at what the patch suggests
[memory]: held chords and slow lines, an eerie texture, moderate velocities.

### The physics: a flame that sings in a pipe

- **Higgins' singing flame (1777), the Rijke tube (1859), Sondhauss** [memory]: a flame, or a hot
  gauze, in a vertical pipe can drive the pipe's air column.
- **Rayleigh's criterion** (1878) [paper, memory]: heat added in phase with the pressure swing
  drives the oscillation. A flame's heat release follows the air's velocity past it with a lag
  (the time the flow takes to carry a disturbance through the flame). In the lower half of an
  open pipe, where the velocity leads the pressure by a quarter period, a lag near a quarter
  period makes the heat swell with the pressure.
- **The nonlinearity** (Heckl 1990) [paper]: the heat transfer from a hot gauze goes as the root
  of the flow's speed (King's law). With a mean flow u0 and an acoustic swing u, it goes as
  √|u0/3 + u|: the heat stops following the flow once the flow reverses, and that limits the
  oscillation (a limit cycle; in a Rijke tube the swing saturates near u0/3).
- **The flame's response** (Ducruix et al. 2003; Schuller et al. 2003) [paper, memory]: a
  laminar flame's heat release answers velocity swings as a low-pass in a Strouhal number (the
  flame's length over the flow speed, times the frequency), with a lag growing with it. A shorter
  flame in a faster flow passes higher frequencies.
- **The pyrophone** (Frédéric Kastner, 1873; "Les flammes chantantes", 1876) [memory]:
  - glass pipes a key, each with gas flames inside, which the keys moved apart (singing) or
    together (silent);
  - its tone was compared with the human voice;
  - it was slow to speak, and its pipes drifted as they warmed.
- **Flame puffing** (Cetegen and Ahmed 1993) [paper]: a buoyant flame flickers at about
  1.5/√D Hz (D the burner's diameter in m): a few Hz for burners of a few cm. It is a
  self-excited instability of the buoyant plume (toroidal vortices shed at the flame's base).
- **The draft:** warm gas in a vertical pipe draws air through it (a chimney), the flow growing as
  the root of the temperature rise [memory]. As the pipe warms over seconds, the flow past the
  flame grows: the lag shrinks and the flame shortens.

### Design: a keyboard of singing flames

What the patch must do:
- swell in over most of a second;
- start dark and open over several seconds, as a timbre more than a level;
- warble at a few Hz, mostly in the upper partials;
- hold while the key is held, play chords and follow the keys;
- have its slow evolution come from its own dynamics.

The candidates:

1. **A resonant filter swept over a voice** (the literal reading). Rejected: that is the analog
   pads' mechanism (`src/lead.cc`; `pad-sweep` is the fallback), and the sweep would be an
   envelope.
2. **The singer's tract with a moving formant.** Rejected: the choir/singer tract is taken, and
   the motion would be scripted.
3. **Bubbles** (a gurgle). Rejected: a bubble's note (Minnaert) is fixed by its size and dies in
   tens of ms; a held, keyed note would need a stream of bubbles, which is rain's territory
   (`fx-rain`), and the pitch would not hold.
4. **A friction drum** (a lion's roar, a cuíca). Rejected: stick-slip, as the bowed family.
5. **A jaw harp.** Rejected: its formant moves with the player's mouth (the tract again), and it
   is plucked.
6. **The pyrophone (chosen):**
   - **It swells in by itself:** a thermoacoustic oscillation grows from the flame's noise (and
     the lighting's thump) at a rate set by the gain over the losses: ~0.5–1.5 s.
   - **The slow sweep is the pipe warming:** the draft grows, so the flame's lag moves to the
     quarter period, the flame shortens (its response opens) and the cold walls stop quenching
     it (its gain grows). The partials come up in turn over seconds: the classic's 9 s filter
     opening, from heat rather than an envelope.
   - **The bubbling is the flame's own puffing:** a hydrodynamic oscillator a few Hz, which swings
     the heat and the lag. The partials' phases through the lag move n times as much as the
     note's, so the upper partials warble most, as in the classic.
   - **A voice-like pipe tone** (the pyrophone's reputation), dark at first.
   - **Playable:** a pipe a key, as Kastner's; velocity is the gas valve.
   - **Not a duplicate:** no instrument here is thermoacoustic; the organ's pipes are blown, and
     the aeolian harp's vortex shedding drives a string.

The model (`src/goblins.h` has the summary):
- **The pipe:** open at both ends; its modes are its harmonics up to 6 kHz (32 at most), each a
  complex one-pole resonator (rotated and decayed exactly each sample).
  - The fundamental's Q is 60 at C4, as √f over the keyboard (15–300).
  - The modes' damping is σ1 (0.8 √n + 0.1 n²): the walls and the radiation.
- **The flame:** at 0.23 of the pipe's length. Each mode is driven by the rate of the heat
  release times sin(nπ·0.23)/ω_n. The flame sees the velocity there (Σ −cos(nπ·0.23) Im z_n) a
  lag ago, through a fractional delay line.
  - The heat release is √(√((1/3 + u/u0)² + 0.03²)): Heckl's law, its corner rounded.
  - It is low-passed at the flame's corner, and scaled so that the loop's linear gain at the
    fundamental is the flame's gain × the fundamental's damping.
  - The pure delay is the lag less the low-pass's phase at the note, so the loop's phase at the
    note is the lag's.
- **The warming:** w → (the valve's opening) with a time constant of 2–16 s (K1; 5.7 s by
  default), and:
  - the lag is ¼ period × √((1 + c²)/(w + c²)), with c² = 1/(1.25² − 1), so the cold pipe's lag
    is 1.25 × the hot one's;
  - the flow is u0 = size^0.75 √((w + c²)/(1 + c²));
  - the gain is cold + (hot − cold) w: 1.3–1.4 cold and 1.8–3.2 hot (by velocity);
  - the flame's corner is f0 (1 + (k − 1) w²), with k 2.5–7 by velocity.
- **The puffing:** a Van der Pol oscillator (μ 0.3) at 6.5 Hz × (f0/C4)^0.1 ± 6 % per key, kicked
  by noise. It multiplies the gain by (1 + 0.6 · K2 · x) and the lag by (1 + 0.24 · K2 · x),
  with x its state over its limit amplitude (~±1).
- **The roar:** white noise low-passed at 700 Hz, 0.06 K3² √size of the heat's scale, added to the
  heat release.
- **The lighting:** the heat rises by 0.3 √size over 4 ms (a raised cosine) as the key goes down.
- **The valve** opens over 0.13–0.25 s (a raised cosine; a harder key, faster). The size is
  0.15 + 0.85 v^1.5. The release closes it exponentially (0.1–3 s, K4; 0.55 s by default).
- **Tuning:** the lag past the quarter period pulls the pipe flat by cot(phase)/2Q. Each pipe is
  cut so that it is in tune at w = 0.4.
- **The sound:** the top end's volume velocity's rate, Σ (−1)^(n+1) n Re z_n, panned by key
  (±0.5).

Tuning that mattered [T]:
- **The loop's scale.** With the forcing a factor ω1 too strong (an early slip), the pipe hopped
  between modes chaotically: h2 10 dB over h1, everything off the harmonics. Normalising the loop
  so that its linear gain is a multiple of the mode's damping gives one clean limit cycle at every
  key (what lies off the harmonics −16 to −49 dB, the puffing's sidebands among it).
- **Gain growth alone was a poor sweep.** With only the quenching (the gain growing with the
  warmth), the centroid went from 1.43 to 2.05 × f0 at C4, but the level swelled 11 dB, and
  starting the pipe nearer its threshold (to start darker) delayed it by seconds. The flame's
  response opening with the flow, with its phase taken out of the delay and its gain out of the
  loop, starts the note dark (1.16–1.2 × f0) without slowing it, and leaves a 6 dB swell.
- **The radiation's share of the damping.** At 0.3 (of σ1 n²) the upper modes were too damped
  for the flame's moving comb to matter; at 0.1 the upper partials come and go with it.
- **The seed.** With the roar as the only seed, soft bass notes took more than 2 s to speak (C2
  at velocity 20 had not reached within 12 dB of its held level in 2 s). The lighting's thump
  rings the pipe at once: now 1.4 s for C2 at velocity 20, 1.2 s at 96.
- **The roar.** At C3–C4 the band 2.5–6 kHz was all roar. Its default is a quarter of the first
  one's, and it scales as √size: a small flame is quieter. The smaller flame being quieter also
  stopped soft notes coming out brighter than loud ones once warm.
- **Where the pipes are cut.** In tune at full warmth, the first seconds were 4–10 c flat (the
  bass the most). Cut for 40 % warmth, a held note goes from −2 to −8 c at 0.5–1.5 s to −0.9 to
  +2.3 c once warm.
- **The room.** The comparison's default hall cancels the fundamental of a near-sine at some keys
  in the mono sum (C4 at velocity 96: h2 10 dB over h1, against −9 dB dry), so the profile was
  made both with the room and dry.

### Measurements [T]

- **Tuning, warm** (the power-weighted pitch over 6–10 s, velocity 100): +2.3, +1.1, +0.5, +0.2,
  −0.2, −0.5, −0.8, −0.9 c at A0, C2, C3, C4, C5, C6, C7, C8. At 0.5–1.5 s: −3 to −8 c in the
  bass, −1.5 to −2.3 c from C4 up.
- **Touch:** a C-major chord, 0–6 s, at velocities 20, 60, 100 and 127: −28.1, −21.9, −17.1 and
  −14.4 dB.
- **The swell** (C4, velocity 100):
  - the first 100 ms are 55 dB under the warm pipe (6–10 s);
  - it is within 12 dB of the warm level by 0.8 s (at velocity 96: C6 0.5 s, C2 1.2 s, A0
    1.5 s);
  - the warm pipe is 6.0 dB over 1–2 s.
- **The opening:** the centroid is 1.16 × f0 at 0.5–1.5 s and 1.70 × f0 at 6–10 s. The partials
  2–8 go from −11, −13, −36, −36, −30, −37, −46 dB at 0.5–1.5 s to −1, −6, −26, −19, −33, −21, −34
  dB at 6–10 s. With K1 at 0 the centroid at 2–4 s is 1.71 × f0, with K1 at 1 it is 1.20. At
  velocity 20 the warm pipe is darker (1.44 × f0).
- **The bubbling:** 98 % of the upper partials' (3–8) level modulation (0.3–40 Hz) lies in
  3–15 Hz, the strongest at 6.7 Hz (C3 6.2, C5 7.2, C6 7.7 Hz); with K2 at 0, 31 % (40–53 % at
  other keys and settings). The fundamental's pitch wobbles ~7 c peak to peak.
- **Release:** at C4, 2.5–3 s after the key −123 dB; 30 dB down in ~0.9 s.
- **Clicks and DC:** the largest step at a key is 0.06–0.08 of the peak; no DC.
- **Level:** −29.8 LUFS untrimmed by `levels.py`'s measure(); trim +1.6 dB to the median −28.2.
- **Velocity row:** `velcurve.py`'s `render()`/`gain_curve()` on the mean of C3 G3 C4 E4 C5. The
  row runs from −13.9 dB at velocity 8 to 0.0 at 127. Soft notes at velocity 8–16 have not fully
  grown within velcurve's 1 s notes.
- **Cost:** eight notes take ~0.8 µs/sample, 4 % of a core.

### Compared with the SoundFonts (`tools/synthcompare.py profile`, program 101) [T]

pajupilli (with the default room, and dry) against FluidR3_GM, with TimGM6mb and Jnsgm2 as
references (keys four semitones apart, velocities 40/64/96/127, 2 s held). The SoundFonts'
renders lack their filter sweep (TiMidity++ does not sound it), and 2 s shows only the start of
pajupilli's.

| measure | pajupilli (room) | pajupilli (dry) | FluidR3 | TimGM6mb | Jnsgm2 |
|---|---|---|---|---|---|
| onset delay (ms) | 938 | 874 | 547 | 538 | 608 |
| attack time, −20 to −1 dB (ms) | 482 | 478 | 728 | 758 | 790 |
| onset sharpness (dB/ms) | 7.0 | 2.3 | 8.9 | 8.9 | 9.3 |
| attack brighter than the body (ERB) | −4.5 | −1.7 | 1.1 | −0.1 | 0.1 |
| level after 2 s held, re peak (dB) | −2.5 | −2.0 | −7.5 | −9.2 | −9.0 |
| release, 30 dB after note-off (ms) | 926 | 909 | 880 | 610 | 789 |
| loudness range, velocity 40 → 127 (dB) | 17 | 16 | 17 | 17 | 17 |
| brightness (centroid, ERB-number, vel 96) | 13 | 11 | 9.4 | 7.5 | 7.4 |
| brightening, velocity 40 → 127 (ERB) | −1.4 | −1.3 | 0.0 | 0.0 | 0.0 |
| brightness over the keyboard (ERB/oct) | 2.0 | 2.1 | 3.1 | 3.5 | 3.3 |
| fundamental share (tristimulus 1) | 0.87 | 0.94 | 0.37 | 0.93 | 0.94 |
| partials 2–4 share (tristimulus 2) | 0.06 | 0.03 | 0.63 | 0.07 | 0.06 |
| partials 5+ share (tristimulus 3) | 0.06 | 0.02 | 0.00 | 0.00 | 0.00 |
| odd / even partials (dB) | −1.0 | −0.6 | −23 | −22 | −17 |
| noisiness (spectral flatness, dB) | −25 | −25 | −34 | −49 | −34 |
| Plomp-Levelt dissonance (log2) | −1.2 | −1.8 | −2.7 | −3.9 | −3.3 |
| key-to-key level scatter (dB) | 3.8 | 3.8 | 0.5 | 1.2 | 1.6 |
| key-to-key brightness scatter (ERB) | 2.5 | 1.8 | 0.6 | 0.2 | 0.3 |
| pitch re A440, median (c) | −4.2 | −4.3 | −1.2 | 2.1 | −2.3 |
| pitch wobble (c p-p) | 16 | 14 | 21 | 40 | 34 |
| stereo coherence | 0.52 | 1 | 0.22 | 1 | 1 |

The mean spectrum relative to FluidR3, level removed (room / dry): 40–150 Hz +0 / +3, 150–400 Hz
−5 / −2, 400–1000 Hz −1 / −1, 1–2.5 kHz 0 / −1, 2.5–6 kHz +10 / +9, 6–16 kHz −7 / −8 dB. The
velocity curves agree within 1 dB (re velocity 96: −12.5, −6.0, +4.4 against −12.6, −5.9, +4.0).

What differs:
1. **It speaks later and swells instead of sagging.** The oscillation grows for ~0.9 s before it
   is heard (0.54–0.61 s for the samples' 0.77 s ramp). Its −20 to −1 dB attack is shorter (0.48
   s against 0.73–0.79), and after 2 s it is still near its peak (−2 dB against −7.5 to −9).
2. **Its attack is duller than its body** (−1.7 to −4.5 ERB): it opens over seconds, where the
   renders do not (they lack the filter envelope the SF2 data ask for).
3. **It is brighter at 2.5–6 kHz** (+9–10 dB re FluidR3): the pipe's upper partials (partials 5
   and up 0.02–0.06 against 0). The centroid is 11–13 ERB against 7.4–9.4.
4. **Its odd and even partials are about even** (−1 dB), where the vox samples favour the even
   ones (−17 to −23 dB): a pipe with the flame near a quarter of its length favours odd modes.
5. **It is noisier and rougher** (flatness −25 dB, Plomp-Levelt −1.2 to −1.8): the puffing's
   sidebands and the roar.
6. **Keys scatter** by ~3.8 dB and 1.8–2.5 ERB: each flame puffs in its own phase, and the bass
   is still growing at 2 s.
7. **Soft notes are brighter in the first 2 s** (−1.3 ERB from 40 to 127). The lighting and the
   roar weigh more while a soft pipe grows; warm, soft notes are darker (above).
8. **A few cents flat at 2 s** (−4 c median): the pipes rise into tune as they warm.

What is alike: the velocity curve and the loudness range, the fundamental's share (as TimGM6mb's
and Jnsgm2's), the release, the pitch wobble (14–16 c against 21–40 c), the pitch scatter
(2 c), the mid-band spectrum (150 Hz–2.5 kHz within 5 dB).

### Limitations

- **The flame is idealised.** It uses Heckl's gauze law for a flame, and a first-order low-pass
  for its transfer function. Its gain growth (the walls' quenching) and its corner's growth with
  the warmth are chosen for the classic's sweep, not taken from a built pyrophone.
- **The pipe is one-dimensional and uniform:** no hot plume above the flame (which would detune
  the upper modes), no end corrections. The draft is a single warmth variable per pipe; pipes do
  not warm one another.
- **Self-similar scaling** (each pipe's lag, flame and Q sized to its note) evens the keyboard,
  which real pipes would not be. The bass still speaks slowly (A0 ~1.5 s to within 12 dB).
- **The tuning is cut for one warmth** (40 %): a note glides up a few cents as it warms, up to
  ~8 c in the bass. A note re-struck on a warm pipe starts cold again: each key's pipe starts
  cold on every key-down.
- **Odd partials dominate**, unlike the vox samples; the radiation is the top end's alone.
- **No vibrato wider than the puffing's ~8 c:** the sitar layer's six-semitone vibrato is not
  imitated.

## FX 7 (echoes), GM 103

### What it is in the classic sets

SF2 times below are converted from timecents (2^(tc/1200) s), frequencies from absolute cents
(8.176 · 2^(c/1200) Hz) and levels from centibels.

- **Roland GS (SC-55 onward), "Echo Drops"** (program 103, bank 0), with bank variations "Echo
  Bell" and "Echo Pan" [web, dtm wiki SC-55 map; read through a search summary].
  - Sound House's GM column lists it with **one voice** and a C2–C7 range. It says that despite
    the name it does not behave like a delay effect: it is two chorus-like tones that sweep, and a
    bursting sound comes at the key-up, which suggests water drops (hence "Drops") [web,
    soundhouse; read through a search summary, the page itself timed out].
  - **D-50 heritage: not found.** The pages that trace "Fantasia", "Soundtrack" and "Atmosphere"
    to the D-50 do not mention it, and no D-50 preset list searched has an "Echo Drops". The
    sound itself was not heard.
- **Yamaha XG, "Echoes"** (MSB 0, LSB 0, program 103) [web, Yamaha lists]. The variations are
  "Echoes 2" (LSB 8), "Echo Pan" (14), "EchoBell(s)" (64), "Big Pan" (65) and "SynPiano" (66),
  with "Creation", "StarDust" and "Reso&Pan" in some lists (QY100, CLP-785 and P-515 data lists,
  read through a search summary; the PDFs answered 403 to a direct read). The element count and
  make-up are not known here. The pan variations suggest a moving stereo image [memory].
- **FluidR3_GM, "Echo Drops"** [T].
  - The preset: 17 dB down, reverb 15 %, chorus 25 %.
  - The instrument's global zone, for both layers:
    - a low-pass at 11108 c (~5.0 kHz, 0.7 dB of Q), **pulled down** by the modulation envelope by
      −3000 c (to ~890 Hz) at its peak 0.16 s in, and opened again over its 2.9 s decay (its
      sustain is zero): a dull "drop" that clears over three seconds;
    - the volume: held 0.23 s, then decaying over 19.7 s to −3.4 dB (a held pad); **release
      5.0 s**.
  - **"Ahh Choir"**, D#3 to A6 (15 zones, three keys each), 32 kHz, 1.7–3.1 s, each looped over
    its last 0.8–2.2 s.
  - **"SynthVz"**, C4 to F#7 in six zones, a stereo pair **panned ±50 %**, its loops 0.35–0.96 s.
- **TimGM6mb, "Echo Drops"** [T]: two instruments.
  - **"Echo Drops0", the choir:** "Aahs" samples (Db3, F3, C4, F4, C7; 0.04–0.3 s loops) with
    FluidR3's envelopes to within a few units: a low-pass at ~10900–11100 c (~4.4–5.0 kHz)
    pulled down by −2250 to −3150 c at 0.16 s and reopened over 2.9 s; the volume held 0.23 s,
    decaying over 19.8 s to −3.4 dB; release 3.8 s.
  - **"Echo Drops1", the syn vox:** "Syn Vox" F#3 and C4 and "SynthStrings D6", 11 dB down,
    through a low-pass at 6609–6962 c (~370–470 Hz, 1.5 dB of Q) **opened by +4162 c (× 11) over
    a 4.9 s attack**; the volume decays over 19.8 s to −2.3 dB; release 0.39 s.
- **Jnsgm2, "Echo Drops"** [T]: TimGM6mb's syn vox alone at 44.1 kHz ("Syn Vox C3", "F#3", "C4",
  "Synth Strings D6"), the same filter and envelopes to within a unit or two.
- **Rendered** (TiMidity++ 2.14, C4 held 6 s and struck for 0.25 s at velocity 100; C2, C3, C5,
  C6 held 5 s) [T]. TiMidity++ again does not sound the modulation envelopes' filter moves; with
  its reverb and chorus on or off the renders were identical.
  - **There is no delay in any of them.** What repeats is the samples' loops: the level swells
    and dips at the loop's period, which follows the key because the sample is transposed.
    - **Jnsgm2:** regular pulses ~14 dB deep every 0.50 s at C4 (the "Syn Vox C3" loop, 0.25 s,
      played an octave down), 0.98 s at C3, 0.82 s at C5; the centroid alternates 1.3 and 2.0 ×
      f0 every quarter second; mono; a 0.25 s note is gone in 0.3 s.
    - **TimGM6mb:** swells every 0.68 s at C4 (the "Aahs Db3" loop, 0.29 s, 15 semitones down),
      0.58 s at C5; centroid 1.5–3.5 × f0; mono.
    - **FluidR3:** irregular ±3–5 dB swells, the strongest period 1.22 s at C4 (both layers'
      loops are ~1.2 s there), 0.96 s at C5; centroid 1.2–2.5 × f0; **L/R correlation
      0.6–0.8** (0.22 at C2), the synth voice's pair panned apart. A 0.25 s note fades over its
      5 s release with the loops' swells in it (bumps at ~0.8, 1.0 and 1.2 s).
  - So the "echoes" are a pad's level pulsing at 0.5–1.2 s, an interval set by the sample and
    the key, not a tempo; nothing decays repeat by repeat or darkens, and only FluidR3 is
    stereo. Roland's description (sweeping, chorus-like tones; drops at the release) fits
    FluidR3's long release over a sweeping pair.

So the common core [T]:
- **a near-sine voice** (the syn vox; at its root h2 −7 to −13 dB) or a choir "aah", held as a
  pad (−2 to −3.4 dB over ~20 s);
- **a pulsing at 0.5–1.2 s**, the loops', different from key to key;
- **a dull start that clears** (FluidR3's and TimGM6mb's choir: the low-pass dipped to ~0.9 kHz
  at 0.16 s and reopened over 2.9 s), or a slow opening (the syn vox: × 11 over 4.9 s);
- **a long release** in FluidR3 (5 s) and TimGM6mb's choir (3.8 s), a short one in Jnsgm2;
- **a stereo spread** in FluidR3 only.

### How it is used in GM files

Of 397 MIDI files under `~/midi`, `~/Music`, `~/Downloads` and `~/Musici`, none selects program
103 (no program change to it at all) [T]. So the design aims at what the name and the patch
suggest [memory]: struck notes and chords that come back, slow lines, moderate velocities.

### The physics: a tape echo

- **Magnetic recording with AC bias** [memory; Mee and Daniel, Bertram]: the record head's field
  plus a strong supersonic bias leaves the tape magnetised on its **anhysteretic** curve, which
  is linear when quiet and saturates softly. For non-interacting particles it is a Langevin
  function, L(u) = coth u − 1/u.
- **The playback head** reads the tape's field through the head-to-tape spacing d. The field it
  sees is the recorded one smoothed along the tape by a Poisson kernel of width d: in frequency,
  **Wallace's spacing loss** exp(−2π d f / v), 54.6 dB per wavelength of spacing [paper,
  Wallace 1951]. A slower tape (a shorter wavelength for the same frequency) or a worn one
  (larger d) is darker. Gap loss (the sinc of the gap over the wavelength) and thickness
  loss come on top; at these speeds the gap's first null lies far above the audio band.
- **The tape echo** (Echoplex, 1961; Roland RE-201 Space Echo, 1974) [memory]: a loop of tape
  runs past an erase head, a record head and one or more playback heads. A head at distance D
  hears what was recorded D / v ago; its output fed back to the record head ("intensity") makes
  the repeats. Each pass is quieter by the feedback, darker by the head losses and, when loud,
  more saturated. Past unity loop gain the machine **runs away** into its own oscillation, held
  by the tape's saturation.
- **Wow and flutter** [memory]: the capstan's and the pinch roller's eccentricity modulate the
  speed once per turn (a capstan of ~1 cm turns ~6 times a second at 18 cm/s: flutter); the
  loop's splice and tension once per loop (wow, a fraction of a Hz); scrape flutter is the tape's
  stick-slip on the heads (noise). A repeat's pitch moves with the difference between the speeds
  at recording and at playback. The motor's inertia makes a change of speed a glide: the echoes
  already on the tape slide in pitch.
- **The tuning fork** (Rayleigh) [memory]: each prong is a clamped-free bar; its bending modes
  lie at 1, 6.267 and 17.55 × the first. The second (the "clang") is struck strongly by a hard
  blow and dies in a fraction of a second; the first rings for many seconds. The stem moves at
  the fork's frequency (which is how a fork on a table sounds it), and a loud fork's stem also
  gives the octave. A box tuned to the fork (König's resonators) radiates the note.
- **The dulcitone** (Thomas Machell, Glasgow, 1880s) [memory]: a keyboard of tuning forks
  struck by felt hammers; soft, pure and bell-like.
- **A felt hammer** [paper, Hall; Stulov]: the felt's force grows faster than its compression
  (F ~ δ^p, p ≈ 2–3.5), so a faster blow is a shorter contact and excites higher modes.

### Design: a dulcitone into a tape echo

What the patch must do:
- come back: a struck note repeats at a fixed interval, quieter each time, and the release
  leaves its repeats ringing (the "drops" after the key-up);
- give a near-sine voice that sweeps like a chorus when held;
- spread in stereo;
- follow the keys and play chords, with the echoes following.

The candidates:
1. **The patch as it is** (a pad whose level pulses at a loop's period). Rejected: the pulsing
   is a sample artefact, and a pad is the analog pads' (`src/lead.cc`; `pad-halo`, the
   fallback, is one).
2. **A flutter echo between parallel walls** (a listener between two walls 50 m apart hears a
   repeat every 0.3 s, alternating sides). Rejected as the main mechanism: it is linear
   propagation with a little air absorption and gives the instrument nothing but a delay; a
   building this size is also the room's job.
3. **A long tube or a long wire** (a speaking tube; a guy wire's dispersive "pew"). A tube a key
   is a delay with wall losses, again linear; a long stiff wire disperses each repeat into a
   chirp, which is a spring tank's sound (`src/guitaramp.cc` has a dispersive spring reverb).
   Rejected.
4. **A spring reverb tank.** Rejected: it exists (`src/guitaramp.cc`), and its repeats come every
   30–80 ms, a flutter, not tempo-like echoes.
5. **A tape echo after a struck source (chosen):**
   - **real repeats at a fixed, settable interval:** the head spacing over the tape speed, the
     same for every key and chord;
   - **its own dynamics:** the feedback loop (decaying repeats, runaway past unity), the tape's
     saturation (loud chords and piled-up repeats compress), the spacing loss (each pass darker,
     a slower or worn tape darker still), wow and flutter (the repeats' pitch wanders; a held
     note beating against its own echo sweeps like a chorus: Roland's "sweeping tones"), the
     motor's inertia (turning the rate glides the echoes);
   - **stereo from the machine:** the two heads to the two sides make the repeats alternate;
   - **not a duplicate:** `pad-halo` has an abstract bucket-brigade echo (a 340 ms line, a fixed
     low-pass and a tanh); nothing here records on tape. The source is new too: no instrument
     here is a tuning fork (the Rhodes' tine and tone bar form an asymmetric fork, but it is
     heard through a magnetic pickup, and the kalimba's tines are free).
   - **The fork as the source:** the nearest a struck thing comes to the classic's near-sine
     voice; its clang under a hard blow is the "drop"; its long ring holds like a pad; and a
     pure tone makes the tape's wow audible as the classic's sweeping.

The model (`src/echoes.h` has the summary):
- **The fork:** three modes per key (1, 6.267, 17.55 × the note, those above 0.45 fs left
  out), each a complex one-pole resonator (rotated and decayed exactly each sample).
  - T60: 20 s × (f/C4)^−0.5 for the note (3–40 s), 0.8 s × (f2/1640 Hz)^−0.5 for the clang,
    0.35 s for the third mode.
  - **The hammer:** mass 0.25 of the prong's modal mass, speed 0.02 + 3 v^1.4 m/s (v the
    velocity over 127). Its felt is linear-equivalent to a contact of 1.2 ms × (C4/f)^0.7
    (0.1–5 ms) at 1 m/s, stiffening as δ^2.5: a harder blow is a shorter contact. The contact
    force is integrated against the modes sample by sample until the hammer bounces off.
  - **The sound at the microphone:** the stem's velocity through the box (a band-pass at the
    fork's note, Q 25, each box cut up to ±0.8 % off), plus the prongs' own radiation (0.06 ×
    each mode's velocity × its frequency over the note's: weak for the note, strong for the
    clang), plus the stem's octave (0.35 × the note's displacement × its velocity, scaled by
    ω). Panned by key (±0.2).
  - **The damper** lands over 15 ms at the key-up: T60 0.1–0.4 s for the note, 0.08 s for the
    others.
- **The tape:**
  - **Speed:** 8 × 5^K1 cm/s (8–40; 18 cm/s by default), reached by the motor with a 0.35 s time
    constant. The speed is wobbled by the capstan (1.2 cm across; 0.12 % at the default wear),
    the pinch roller (3 cm; 0.06 %), the loop (1.6 m; 0.2 %) and scrape flutter (noise below
    30 Hz; 0.04 % rms), all × (0.2 + 3.2 K4²).
  - **The record head:** the microphone's signal plus the second head's × 1.2 K2^1.5, written as
    3 S L(x / S) (slope 1 when quiet, S = 3: a forte note is nearly linear, a forte chord bends),
    with the tape's position at that sample.
  - **The playback heads,** 6 and 12 cm downstream (0.335 and 0.67 s by default): each finds
    where the tape now under it was written (a search on the stored positions, so a changing
    speed shifts the pitch as it would), and reads it through a Poisson kernel of width
    γ = d / v (in samples) + 0.5 (the electronics), d = 3 + 8 K4 µm (7 µm by default), over ±24
    samples (the loss's cusp at low frequencies lives in the kernel's 1/t² tails; at ±8 the
    octave of C5 lost only 0.5 dB a pass instead of 1.4). The kernel also interpolates between
    samples. Then a 60 Hz coupling high-pass.
  - **Out:** the forks plus 1.2 K3 × the heads, head 1 to the left and head 2 to the right, each
    half the way (L = dry + e (0.75 h1 + 0.25 h2)).
  - **Sleep:** the model stops once no note is left and nothing above 10⁻⁶ has reached the
    record head or head 1 for the tape's whole memory (2.7 s); with K2 near its top it keeps
    oscillating and never sleeps.

Tuning that mattered [T]:
- **The contact time.** At 1.6 ms × (C4/f)^0.35 the clang was gone above C4 and the treble fell
  15–20 dB (the contact outlasted the treble forks' half period). At 1.2 ms × (C4/f)^0.7 the
  level is even from A0 to C8 (−24 to −27 dB over the first 0.5 s at velocity 100) and the clang
  is −19 dB at C4 at velocity 100, −17 at 127, −44 at 40.
- **The hammer's slowest speed.** With 0.2 m/s at velocity 1, velocities 1–24 sounded alike
  (the velocity table's remap floors at velocity 1); at 0.02 m/s the raw curve is within ~4 dB
  of General MIDI's and the table is small.
- **The spacing kernel's length** (above).
- **The echo's width.** With the heads fully apart (L = dry + 0.8 h1, R = dry + 0.8 h2) a held
  chord's echoes, beating against one another, left the hall's two channels anticorrelated
  (−0.39). Half the way and the echo volume 0.6 bring the A/B phrase to −0.17 in the hall (the
  hall alone gives this instrument ~0) and +0.67 dry.
- **The fork's ring.** At 8–12 s T60 at C4 the profile decayed 4.8 dB/s against the samples'
  0.4; at 20 s, 3 dB/s.

### Measurements [T]

- **Tuning** (power-weighted over 0.3–2 s, the fork and its echoes, velocity 100): −0.1, +1.4,
  +0.1, −0.1, −0.3, +0.1, +0.4, 0.0 c at A0, C2, C3, C4, C5, C6, C7, C8. The repeats wander by
  +0.5 to +5 c (C5; the wow).
- **Touch:** a C-major triad, its first 2 s, at velocities 20, 60, 100, 127: −43.4, −29.7, −23.5
  and −20.6 dB. The clang at C4 is −44, −19 and −17 dB re the note at velocities 40, 100, 127,
  the octave −25, −14, −12 dB.
- **The echoes** (C5 struck 0.12 s at velocity 100, mono):
  - repeats every 0.31–0.36 s (median 0.335 s: 6 cm at 17.9 cm/s); K1 at 1: 0.165 s, at 0:
    0.755 s;
  - the first two repeats −11 dB re the blow, then each pair 8.2 dB under the one before: 6
    repeats within 30 dB (K2 at 0.8: 19; at 0: only the two heads' repeats; at ~0.9 a repeat
    loses ~0.3 dB a second and at 1 the machine runs away, growing ~1 dB/s until the tape's
    saturation holds it, after ~20 s, ~6 dB over the struck note's first second at C4);
  - **darker each pass:** at velocity 127 the octave falls 1.5 dB a pass against the note and
    the clang (3.3 kHz) 7.7 dB (K4 at 0: 0.8 and 4.1 dB; at 1: 2.3 and 11.4 dB);
  - **stereo** (`-play`, dry): the first repeat 14 dB louder on the left, the second 14 dB
    louder on the right.
- **Held:** the note decays 3 dB/s (C4: T60 20 s); a held note beating against its echoes in
  the room swells and dips slowly as the wow moves its phase.
- **Release:** the damper stops the fork in ~0.2 s; the repeats go on, 30 dB down ~1.3 s after
  the key-up in the profile.
- **Clicks and DC:** the largest step at a key 0.04 of the peak; no DC.
- **Fuzz:** 10 s of random keys, peak 0.86; with all knobs at 1 (running away) 1.69; at 0 0.84.
- **Level:** −24.8 LUFS untrimmed by `levels.py`'s measure(); trim −3.4 dB to the median −28.2.
- **Velocity row:** `velcurve.py`'s `render()`/`gain_curve()` on the mean of C3 G3 C4 E4 C5:
  −4.1 dB at velocity 8 to +0.8 at 127.
- **Cost:** eight notes ~0.35 µs/sample, 1.7 % of a core (most of it the two heads' 48-tap
  kernels).

### Compared with the SoundFonts (`tools/synthcompare.py profile`, program 102) [T]

pajupilli (with the default room, and dry) against FluidR3_GM, with TimGM6mb and Jnsgm2 as
references (keys four semitones apart, velocities 40/64/96/127, 2 s held). The SoundFonts'
renders lack their filter envelopes.

| measure | pajupilli (room) | pajupilli (dry) | FluidR3 | TimGM6mb | Jnsgm2 |
|---|---|---|---|---|---|
| onset delay (ms) | 48 | 21 | 85 | 58 | 6 |
| attack time, −20 to −1 dB (ms) | 132 | 78 | 245 | 208 | 543 |
| attack brighter than the body (ERB) | 0.23 | 0.13 | 0.59 | 1.15 | 0.29 |
| decay of a held note (dB/s) | 3.0 | 3.2 | 0.4 | 0.5 | −0.4 |
| level after 2 s held, re peak (dB) | −10 | −8.6 | −6.1 | −9.3 | −9.7 |
| release, 30 dB after note-off (ms) | 1208 | 1380 | 917 | 689 | 93 |
| loudness range, velocity 40 → 127 (dB) | 16 | 15 | 17 | 17 | 17 |
| brightness (centroid, ERB-number, vel 96) | 8.1 | 7.0 | 8.5 | 11 | 7.4 |
| brightening, velocity 40 → 127 (ERB) | 0.61 | 0.15 | 0 | 0 | 0 |
| brightness over the keyboard (ERB/oct) | 3.6 | 3.5 | 4.2 | 3.5 | 3.4 |
| fundamental share (tristimulus 1) | 0.99 | 0.99 | 0.83 | 0.76 | 0.94 |
| partials 2–4 share (tristimulus 2) | 0.009 | 0.008 | 0.17 | 0.24 | 0.06 |
| odd / even partials (dB) | −46 | −54 | −10 | 2.5 | −17 |
| noisiness (spectral flatness, dB) | −81 | −82 | −29 | −36 | −38 |
| Plomp-Levelt dissonance (log2) | −6.6 | −9.0 | −2.8 | −2.1 | −3.6 |
| key-to-key level scatter (dB) | 3.9 | 0.5 | 0.7 | 0.5 | 1.2 |
| pitch re A440, median (c) | 0.2 | 0.4 | −4.1 | −1.3 | 0.2 |
| pitch wobble (c p-p) | 5.8 | 3.6 | 16 | 24 | 31 |
| stereo coherence | 0.42 | 0.96 | 0.77 | 1 | 1 |

The mean spectrum relative to FluidR3, level removed (room / dry): 40–150 Hz +19 / +24,
150–400 Hz +15 / +20, 400–1000 Hz +13 / +16, 1–2.5 kHz +7 / +4, 2.5–6 kHz −11 / −16, 6–16 kHz
−25 / −23 dB. The velocity curves agree within 1 dB (re velocity 96: −12.4, −5.7, +3.3 against
−12.6, −5.8, +4.1).

What differs:
1. **It is a sine where they are voices.** The fundamental's share is 0.99 against 0.76–0.94,
   the spectral flatness −81 dB against −29 to −38 (no breath), Plomp-Levelt −6.6 to −9 against
   −2 to −3.6, odd/even −46 to −54 dB (the fork has no third harmonic; the octave is its only
   overtone once the clang has died). The mean spectrum is dark: −11 to −16 dB at 2.5–6 kHz.
2. **It decays where they hold:** 3 dB/s against ~0.4: a struck fork, not a pad.
3. **It speaks sooner** (onset 21–48 ms, −20 to −1 dB in 78–132 ms, against 0.2–0.5 s): it is
   struck. Its highs lead its lows (the clang).
4. **Its release is a train of repeats** (1.2–1.4 s for 30 dB, against 0.09–0.92 s), where
   FluidR3's is a 5 s fade.
5. **It brightens a little with the touch** (0.15–0.6 ERB; the samples not at all).
6. **Keys scatter by 3.9 dB in the hall** (0.5 dry): a near-sine and its echoes cancel or add in
   the hall's mono sum, key by key, as the goblins' pipes did.
7. **Its pitch wobble is smaller** (4–6 c against 16–31): the tape's wow, not a sample's LFO.
8. **Stereo:** dry 0.96 (the repeats come after the 2 s notes the profile measures); in the
   room 0.42 against FluidR3's 0.77.

What is alike: the velocity curve and the loudness range, the brightness over the keyboard
(3.5–3.6 ERB/oct against 3.4–4.2), the centroid at velocity 96 (7–8 ERB against 7.4–11), the
pitch (within 0.5 c), the level after 2 s (−9 to −10 dB against −6 to −10).

What the profile cannot see: the echoes themselves (the profile's notes are held 2 s, and the
SoundFonts have none); their interval, decay and darkening are in the measurements above.

### Limitations

- **The tape is idealised:** an anhysteretic Langevin curve without hysteresis or the bias's own
  level; spacing loss only (no gap or thickness loss, no head bump, no pre-emphasis); no tape
  hiss or modulation noise (a real Space Echo hisses audibly). The head spacing, the wow and
  flutter depths and the capstan's size are chosen, not measured on a machine.
- **Two playback heads**, the second fed back; the RE-201 has three and a mode switch. The
  repeats alternate sides because the heads are panned, which a mono Space Echo does not do.
- **The fork is three modes and a box,** the box a band-pass without back-reaction on the fork
  (no beating between fork and box); the octave is a quadratic term, not the stem's geometry.
- **It decays,** where the classic patch holds as a pad (3 dB/s at C4). Re-struck, a key's fork
  starts afresh: a new note replaces the old one rather than adding to its motion.
- **Purer than the classic:** no breath, no choir; the "drop" is the clang and the octave.
- **In the hall,** the echoes of held chords leave the two channels slightly anticorrelated
  (−0.17 for the A/B phrase); a near-sine and its repeats make the hall's key-to-key level
  scatter large (3.9 dB).
- **Runaway** (K2 near 1) settles at the tape's saturation, ~6 dB over a struck C4, and keeps
  the model awake until the knob comes down.

## FX 8 (sci-fi), GM 104

Built: `src/scifi.*` (`fx-sci-fi`), a folded discharge and an FM'd filter ping.

## Sources

- [web, soundhouse] Sound House column on GM sounds, "Ice Rain":
  https://www.soundhouse.co.jp/en/contents/column/index?post=2893 (Japanese; read through a
  search summary).
- [web, roland] Roland support sound list with "Ice Rain Dly":
  https://support.roland.com/hc/en-us/articles/15289085607579
- [web, nasa] NASA Earth Observatory, "Listening to raindrops":
  https://science.nasa.gov/earth/earth-observatory/listening-to-raindrops
- [web, arxiv 2512.09336] "Sound emission from oscillating bubbles trapped by the collapse of
  drop-impact craters": https://arxiv.org/abs/2512.09336
- [paper] Pumphrey, H. C., Elmore, P. A. (1990). The entrainment of bubbles by drop impacts.
  J. Fluid Mech. 220, 539–567.
- [paper] Minnaert, M. (1933). On musical air-bubbles and the sounds of running water.
  Phil. Mag. 16, 235–248.
- [paper] van den Doel, K. (2005). Physically based models for liquid sounds. ACM TAP 2(4).
- [paper] Marshall, J. S., Palmer, W. McK. (1948). The distribution of raindrops with size.
  J. Meteor. 5, 165–166.
- [paper] Atlas, D., Srivastava, R. C., Sekhon, R. S. (1973). Doppler radar characteristics of
  precipitation at vertical incidence. Rev. Geophys. 11, 1–35 (the v(D) fit to Gunn and Kinzer).
- [paper] Gunn, R., Kinzer, G. D. (1949). The terminal velocity of fall for water droplets in
  stagnant air. J. Meteor. 6, 243–248.
- [web, wikipedia] "Roland D-50", Wikipedia: https://en.wikipedia.org/wiki/Roland_D-50 (the D-50
  presets kept by General MIDI: "Fantasia", "Soundtrack", "Atmosphere", ...).
- [web, synthmania] Synthmania, "Roland D-50": https://synthmania.com/d-50.htm (preset 37
  "Soundtrack": Dual, "MelloTones" / "BriteFifth", square partials a fifth apart).
- [web, polyphone / GM2 list] Polyphone's MIDI standards annex:
  https://www.polyphone.io/en/documentation/manual/annexes/midi-standards (program 98
  "Soundtrack").
- [paper] Strouhal, V. (1878). Über eine besondere Art der Tonerregung. Ann. Phys. 241, 216–251.
- [paper] Facchinetti, M. L., de Langre, E., Biolley, F. (2004). Coupling of structure and wake
  oscillators in vortex-induced vibrations. J. Fluids Struct. 19, 123–140.
- [paper] Williamson, C. H. K., Govardhan, R. (2004). Vortex-induced vibrations. Annu. Rev. Fluid
  Mech. 36, 413–455.
- [web, synthmania] (FX 4) the same page: preset 27 "Nylon Atmosphere", "Typical L.A. synthesis
  sound. A beautiful and classic 1980s pad patch".
- [web, Yamaha lists] Yamaha QY100 and P-515 voice/data lists (program 100's XG variations):
  https://uk.yamaha.com/files/download/other_assets/0/318130/QY100E2.pdf,
  https://nl.yamaha.com/files/download/other_assets/7/1204367/p515_en_de_fr_es_pt_ja_dl_a0.pdf
  (read through a search summary).
- [memory] Heet, G. S. (1978). String instrument vibration initiator and sustainer. US patent
  4,075,921 (the EBow).
- [paper] McPherson, A. (2010). The Magnetic Resonator Piano: electronic augmentation of an
  acoustic grand piano. J. New Music Res. 39(3), 189–202.
- [paper] Berdahl, E., Backer, S., Smith, J. O. (2005). If I had a hammer: design and theory of
  an electromagnetically-prepared piano. Proc. ICMC.
- [paper] Chaigne, A., Askenfelt, A. (1994). Numerical simulations of piano strings. I. J.
  Acoust. Soc. Am. 95(2), 1112–1118 (σ = b₁ + b₃ω²).
- [web, synthmania SC-50] Synthmania, "SC-50 Sound Canvas": https://synthmania.com/sc-50_sound_canvas.htm
  (FX 5: "101_Brightness: Fairlight type of sound").
- [web, soundhouse GM SFX column] Sound House column on the GM SFX sounds:
  https://www.soundhouse.co.jp/contents/column/index?post=2908 (Japanese; read through a search
  summary: "101 000 Brightness (2 voices), range C2-C7").
- [web, dtm wiki SC-55 map] https://dtm.noyu.me/wiki/SC-55 (program 100, bank 0, Brightness;
  from a search summary).
- [paper, memory] Duddell, W. (1900). On rapid variations in the current through the
  direct-current arc. J. Inst. Electr. Eng. 30, 232–283 (the singing arc).
- [paper] Mayr, O. (1943). Beiträge zur Theorie des statischen und des dynamischen Lichtbogens.
  Archiv für Elektrotechnik 37, 588–608.
- [memory] Ayrton, H. (1902). The Electric Arc. London: The Electrician.
- [paper] Arnold, H. D., Crandall, I. B. (1917). The thermophone as a precision source of sound.
  Phys. Rev. 10, 22–38.
- [web, Yamaha lists] (FX 6) Yamaha QY100 and QY700 data lists: "102 Goblins", two elements, and
  bank variations near it (read through a search summary):
  https://uk.yamaha.com/files/download/other_assets/0/318130/QY100E2.pdf,
  https://de.yamaha.com/files/download/other_assets/1/317031/QY700F2.pdf
- [web, soundhouse GM SFX column] (FX 6) the same column: Goblin, two voices, sweeping synth
  voices (through a search summary).
- [paper] Rayleigh, J. W. S. (1878). The explanation of certain acoustical phenomena. Nature 18,
  319–321 (the criterion).
- [paper] Heckl, M. A. (1990). Non-linear acoustic effects in the Rijke tube. Acustica 72, 63–71.
- [paper] Ducruix, S., Schuller, T., Durox, D., Candel, S. (2003). Combustion dynamics and
  instabilities: elementary coupling and driving mechanisms. J. Propulsion Power 19(5), 722–734.
- [paper] Schuller, T., Durox, D., Candel, S. (2003). A unified model for the prediction of
  laminar flame transfer functions. Combust. Flame 134, 21–34.
- [paper] Cetegen, B. M., Ahmed, T. A. (1993). Experiments on the periodic instability of buoyant
  plumes and pool fires. Combust. Flame 93, 157–184 (f ≈ 1.5 D^−1/2).
- [memory] Kastner, F. (1876). Les flammes chantantes. Paris (the pyrophone, patented 1873).
- [memory] Rijke, P. L. (1859). Notiz über eine neue Art, die in einer an beiden Enden offenen
  Röhre enthaltene Luft in Schwingungen zu versetzen. Ann. Phys. 183, 339–343.
- [web, dtm wiki SC-55 map] (FX 7) https://dtm.noyu.me/wiki/SC-55: program 103 "Echo Drops", with
  "Echo Bell" and "Echo Pan" (through a search summary).
- [web, soundhouse] (FX 7) the Sound House GM column: Echo Drops, one voice, C2–C7, "not a delay
  effect": sweeping chorus-like tones and a burst at the key-up suggesting drops (through a
  search summary): https://www.soundhouse.co.jp/en/contents/column/index?post=2893
- [web, Yamaha lists] (FX 7) Yamaha CLP-785, QY100 and P-515 data lists: program 103 "Echoes",
  "Echoes 2", "Echo Pan", "EchoBell(s)", "Big Pan", "SynPiano" (through a search summary):
  https://it.yamaha.com/files/download/other_assets/2/1342332/clp785_en_de_fr_es_pt_ja_dl_a0.pdf,
  https://uk.yamaha.com/files/download/other_assets/0/318130/QY100E2.pdf
- [paper] Wallace, R. L. (1951). The reproduction of magnetically recorded signals. Bell Syst.
  Tech. J. 30, 1145–1173 (the spacing loss, 54.6 d/λ dB).
- [memory] Mee, C. D., Daniel, E. D. (eds.) (1996). Magnetic Recording Technology. McGraw-Hill;
  Bertram, H. N. (1994). Theory of Magnetic Recording. Cambridge (AC bias, the anhysteretic
  curve, the playback losses).
- [memory] Roland RE-201 Space Echo (1974): a tape loop, three playback heads, "repeat rate",
  "intensity"; the Echoplex (Mike Battle, 1961).
- [memory] Rayleigh, J. W. S. (1877/1894). The Theory of Sound (the tuning fork; its clang tone
  and its octave); Machell, T. (1880s), the dulcitone.
- [paper] Hall, D. E. (1986). Piano string excitation in the case of small hammer mass. J.
  Acoust. Soc. Am. 79, 141–147; Stulov, A. (1995). Hysteretic model of the grand piano hammer
  felt. J. Acoust. Soc. Am. 97, 2577–2585 (F ~ δ^p).
- [T] `/usr/share/sounds/sf2/FluidR3_GM.sf2`, `/usr/share/sounds/sf2/TimGM6mb.sf2`,
  `~/soundfonts/Jnsgm2.sf2`, TiMidity++ with `/etc/timidity/fluidr3_gm.cfg`.
