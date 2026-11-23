# The fretless bass (GM 36, `-inst fretless-bass`): research and build plan

Status: research 2026-11-20. Nothing is built. Today GM 36 falls back to `double-bass`
(`src/midifile.cc:191`).

This document adds to earlier work and does not repeat it:
- `research/bass_section.md`: the two-pickup bass (GM 34), its strings, pickups and losses. §6.6 is a
  short note on the fretless "mwah".
- `research/bass_amps.md`: the amps. §3.4 covers the Acoustic 360, §10.3 sketches a
  "fretless-di" preset.
- `research/double_bass.md`: the upright bass, which has a board contact and the fingertip
  termination.

How each fact is marked:
- **[paper, key]**: from a paper's text. Where only the abstract or a snippet was read, it says so.
- **[web, source]**: from a web page. Most were read as search snippets or machine summaries, so
  treat them as second-hand.
- **[memory]**: general knowledge, not looked up. Check it before relying on it.
- **[T]**: tested or measured for this document. The scripts and renders are in the session
  scratchpad (prefix `fretless_`): an SF2 zone dump, a TiMidity renderer and analyser, a MIDI
  scanner, a numerical prototype of the board contact, and a spectral-pull calculator.
- **[E]**: my estimate or arithmetic from cited values.

---

## 0. Short answer

- **What the mwah is.** On a fretted bass, a stopped string ends on a fret crown. On a fretless
  bass it ends under a soft fingertip pressing it onto a hard board.
  - Between the finger and the bridge the string rises from the board at a small angle, about
    θ ≈ action / (finger-to-board-end distance) ≈ 4–20 mrad.
  - A plucked string's slope at its ends swings up to roughly depth / (pluck distance) ≈ 10–40 mrad.
    That is larger than θ, so every period the string lies down on the board for a few centimetres
    next to the finger: a distributed, jawari-like contact.
  - My modal prototype reproduces what the recordings show **[T]**:
    - partials 5–9 grow late, peaking at 110–300 ms instead of at the attack;
    - the pluck-position notches fill in (a missing 6th harmonic goes from −50 dB to −5 dB);
    - the low partials lose energy faster;
    - the contact ends 0.3–0.5 s after the pluck, or 0.5–0.85 s with a lower action.
  - It converges at 48 kHz with 4–8 contact points. It is the double bass's board contact
    (`Bass::kBoard`) moved to the finger's end of the string. That fits the real-time budget,
    whereas the published reference schemes run at 1–2 MHz.
- **Nobody has published a measurement of the fretless mwah** (§2.4). The physics papers treat
  frets (Issanchou; Bilbao & Torin) or the sitar and tanpura (Siddiq; van Walstijn). The
  mechanism, the delayed growth of high partials through dispersion and grazing contact, is the
  tanpura's.
- **The GM sets.** FluidR3's Fretless is a real recording **[T]**:
  - the attack peaks late (50–150 ms, against 10–27 ms for its Fingered);
  - on C3 and G3, partials 3–11 peak at 200–460 ms, and on G3 the spectral centroid rises from
    236 to 365 Hz by 0.2 s.

  Jnsgm2's sample has the same bloom in partials 4–8 (40–350 ms). TimGM6mb fakes it with a filter
  envelope that TiMidity++ does not play. No set changes the timbre with velocity, and none has
  vibrato.
- **The user's files.** 26 600 notes on GM 36 in about 38 distinct files, nearly all jazz-trio
  files from the "bushgrafts" collection, where GM 36 **stands in for the upright bass**
  **[T]**. This matches `midifile.cc`'s jazz-band rule and the earlier decision to keep GM 36 on
  the double bass.
  - 29 % of successive notes overlap (median 60 ms): keyboard legato, not slides.
  - Pitch bends are rare. One file (Recardo) has a sequenced "fretless scoop" on almost every
    note: from about −34 c, through 0 at 55 ms, overshooting +15 c at 80 ms, settling by 290 ms.
- **Recommendation.**
  - Build `-inst fretless-bass` as a spec of the electric bass: `jazzBass()` defretted, the bridge
    pickup favoured.
  - Add four things to `Guitar`: a continuous stopping length; a few board-contact points behind
    the finger; the fingertip termination's losses; and the scoop, slide and vibrato player.
  - Leave GM 36 in jazz-band files on the double bass. Map it to the fretless elsewhere.
- **Intonation (§7).**
  - The fretless player tunes its sounding note within ±15 c by the partials of what the other
    channels sound. It uses the choir's spectral rule, with a fixed spectrum template for each
    instrument family.
  - For that, `-play` needs a small shared registry of sounding notes, written by the event loop
    of `midifile.cc`. The `heldNote` map already exists there, and the harmonica's
    `setAccompaniment` already uses it.
  - The bass's pull is weak against a full chord that contains its root (+0.1 c over C–E–G) and
    strong against a bare third (+13.7 c) or a shell voicing **[T]**. That is how the bass should
    behave.

---

## 1. The instruments

### 1.1 Players and basses

**Jaco Pastorius, the "Bass of Doom"**
- The bass: a 1962 two-pickup bass **[web, Wikipedia]**.
- The board:
  - He said he pulled the frets with a butter knife. Other accounts say a previous owner did it.
  - The slots were filled with Plastic Wood or wood putty.
  - The board was brushed with several coats of Pettit "Poly-Poxy" boat epoxy, as he told
    Milkowski **[web, reverb/mixdown/Suchow snippets]**.
  - Kevin Kaufman's later restoration used one thicker cast coat **[web, snippet]**.
- Strings: Rotosound Swing Bass 66, stainless roundwounds, which wear a bare board
  **[web, Wikipedia]**.
- Right hand: the thumb anchored on the bridge pickup for E and A. He muted the E with the thumb
  when playing higher strings **[web, Wikipedia]**.
- Pickups: the bridge pickup soloed or favoured. This is the well-known growl **[memory]**.
- Rig:
  - Two Acoustic 360 preamps with the Variamp boosting the mids. He often used the 360's built-in
    fuzz. Later he used bass amp aluminium-cone cabinets **[web, Wikipedia]**.
  - A rack one pedal maker digital delay fed a second rig while the first stayed dry. Set short, it gave a
    "shimmering stereo doubling", a chorus effect **[web, Wikipedia]**. No settings in ms were
    found; about 10–30 ms is often quoted **[memory]**.
- Harmonics: "Portrait of Tracy" is played almost entirely in natural harmonics, and artificial
  harmonics appear elsewhere, for example the "Birdland" melody **[web, Wikipedia]**.

**American amp fretless Precision.** Introduced in 1970 (some sources say 1971): a standard P with the
frets and markers removed **[web, 12fret, Premier Guitar]**. The often-quoted 1966 date belongs to
the bass amp's AUB-1 (1965 or 1966 depending on the source) **[web, Wikipedia "Fretless bass"]**.

**Pino Palladino.**
- A fretless 1979 active single-humbucker bass **[web, Reverb]**.
- An early rig with a one pedal maker octave pedal **[web, Guitar World snippet]**.
- Compressor and chorus are documented on Gary Numan's *I, Assassin* (1982) **[web, bonedo
  snippet]**.
- No source ties a chorus to "Wherever I Lay My Hat" (1983).
- The active single-humbucker bass's single humbucker sits close to the bridge, with an active 2- or 3-band preamp
  **[memory]**. It is the same "bridge-heavy" listening position as Jaco's.

**Percy Jones** (Brand X): a fretless P, then a Wal. His Wal is described as more mid-focused and
the American amp as "woodier" **[web, Wikipedia, SOS, basschat]**.

**Mick Karn** (Japan): a Travis Bean (aluminium neck) up to *Gentlemen Take Polaroids*, a Wal on
*Tin Drum*, later a Klein headless **[web]**.
- His sliding, wide-vibrato style is famous, but no technical description was found.
- From records: long glissandi of several semitones on one string, and wide slow vibrato
  **[memory]**.

**John Giblin**: a fretless Wal **[web, SOS]**.

**Gary Willis** (one maker): a GWB1 with an ebony board, a GWB105 with Ebanol (phenolic) and a
GWB1005 with gloss-finished rosewood **[web, retailer]**.

### 1.2 Lined against unlined

- **Lined:** fret-position lines of inlay (wood veneer or plastic) set in the slots, flush with
  the board. They are a visual guide only and make no acoustic difference, since they are flush
  **[memory]**.
- **Unlined:** side dots only. Players say unlined boards make them listen more **[memory]**.
- **For the model:** nothing. A lined board might be a reason to give the player a smaller
  intonation error (§5.1), but there are no data.

### 1.3 Board materials

| Board | Janka (lbf) [memory] | Lore |
|---|---|---|
| Macassar ebony | ≈ 3 220 | hard, bright, clear mwah |
| Gaboon ebony | ≈ 3 080 | as above |
| East Indian rosewood | ≈ 2 440 | warmer, "less pronounced muah"; roundwounds "eat" it **[web, basschat]** |
| Pau ferro | ≈ 1 960 | between |
| Epoxy over rosewood (Jaco) | n/a: a glassy coating | "edgier brightness", the most mwah **[web, basschat/talkbass snippets]** |
| Phenolic (Richlite, Ebanol) | n/a | ebony-like, stable |

- **For the model [E]:** the board's hardness enters as the contact's stiffness and damping
  (`K`, `C` in §6.3).
  - The prototype's stiffness from 3e3 to 3e5 N/m moves partials 6–10 by a few dB and the contact's
    end by 0.1 s (§2.5).
  - A loss term C (N s/m) for rosewood that is two to three times the epoxy's is the natural
    knob.
  - No measured value exists.

### 1.4 Flatwounds against roundwounds

- **Roundwounds** are brighter, give more mwah, squeak on slides and wear the board. **Flatwounds**
  are mellower, gentler on the board and quieter on slides **[web, forum lore]**.
- **Physics [E]:**
  - A flatwound's smooth outer wrap means less damping in the windings? Measured flatwound decays
    are actually shorter: flats are stiffer and lossier at high frequencies **[memory]**.
  - Flats also give less finger noise on slides. A roundwound slide excites the winding at
    speed / pitch (bass_section.md §6.5: 0.4–2 kHz).
- **Model:** a `flat` knob raises the high-mode loss (loss1 × 2–3), lowers the slide squeak to
  near zero, and raises B a little (×1.3) **[E]**.

---

## 2. The mwah: physics

### 2.1 The geometry, and why it grazes

- The board lies under the string from the nut to the end of the neck. On a 34" two-pickup bass the
  board's end is near the 20th "fret" position, 0.864 × 2^(−20/12) ≈ 0.27 m from the saddle **[E]**.
- A stopped string of sounding length Ls lies on the board at the finger. It rises to the action
  height h_end over the board's end, plus relief (the neck's concave bow).
- So the clearance at a distance d behind the finger is c(d) ≈ θ d + ½ κ d², where:
  - θ = h_end / (Ls − 0.27);
  - κ is the relief curvature.

  With h_end ≈ 2.5 mm:
  - θ ≈ 7.6 mrad at Ls = 0.6 m (about the 6th position);
  - θ ≈ 19 mrad at Ls = 0.4 m **[E]**.
- Lore setups **[web, harmonycentral/basschat snippets]**:
  - action 2.0–2.5 mm at the 12th position;
  - relief near zero ("very very little"; one player had 0.24 mm relief and 0.35 mm at the nut);
  - some prefer the action slightly higher than on a fretted bass for the "fretless tone".
- **When it touches.** Near its end a string's displacement is s(t)·d, where s is its slope there.
  - For a pluck of depth A at distance x_p from the bridge, the end slope swings up to about
    A / x_p. That is 10–40 mrad for A = 1–4 mm at 10 cm **[E]**.
  - When s > θ, the whole stretch next to the finger presses into the board at once: both the
    displacement and the clearance are linear in d.
  - The string then lies down on the board and lifts off progressively. The effective end of the
    string runs away from the finger and back each period. This is the tanpura's jawari with a
    flat bridge, laid along the neck **[E]**.
  - Relief makes the clearance grow faster than linearly, which bounds the contact length at
    d* ≈ 2 (s − θ) / κ.
- **The threshold** is A_crit ≈ θ · x_p ≈ 0.8 mm at the 6th position, plucked at 10 cm **[E]**.
  - Soft notes barely touch; hard notes graze for hundreds of ms.
  - High on the neck, θ grows, so the same pluck grazes less. Plucking further from the bridge
    lowers A/x_p, so it also grazes less. That contradicts the lore "pluck nearer the neck helps",
    unless "nearer the neck" means a fuller, less tense pluck. Unresolved.
- **Direction.** Only the swing towards the board touches. A fingerstyle rest stroke throws the
  string mostly along the board and a little into it (`ebass.cc`: an angle of 0.35–0.5 rad). The
  into-board share sets how much mwah there is. The two directions' mode axes are tilted
  (`GuitarSpec::tilt`), so the along-board swing leaks into the contact over time **[E]**.
- **Open strings have no mwah.** They end on the nut, and the board beyond it lies under the
  string's far end, which a stopped string does not reach **[E]**. Fretless players know open
  strings sound "fretted" **[memory]**.

### 2.2 Why the high partials grow late

- **Tanpura** (van Walstijn, Bridges & Mehes, DAFx-16) **[paper, full text]**:
  - The string's stiffness sends a high-frequency precursor back from the bridge contact before
    the low frequencies arrive.
  - Each contact moves energy upward, so "the sound grows in amplitude over the initial 500 ms".
  - With EI = 0 the precursor vanishes.
- **Siddiq 2012** **[paper, full text]**: the flat sitar bridge in waveguide and finite
  differences. "Descending formants and complex partial envelopes". "Neglecting dispersion … is
  an oversimplification". No growth times.
- **Mandal & Wahi** (JSV 2015, and Nonlinear Dynamics 2024) **[paper, abstract only]**:
  - A smooth curved obstacle gives a quadratic nonlinearity through wrapping and unwrapping.
  - The mode-locked state tends toward equal energy among modes as more modes are included.
- **Issanchou et al., JSV 2017** **[paper, full text]**: a point obstacle under a guitar string.
  - The contact couples all modes: none is missing, so the pluck-position notches fill.
  - A "spectral resurgence zone" near 8 kHz appears from dispersion and disappears without
    stiffness.
  - Nothing on fretless basses or contact over an interval.
  - The ISMA 2017 abstract and the 2018 Applied Acoustics paper (frets, slap and pop) agree with
    experiments but treat frets **[paper, abstracts]**.
- **Bilbao & Torin, DAFx-14** **[paper, full text]**:
  - Finite differences on a guitar string against the board and 12 frets, with a lumped stopping
    finger. Barrier K = 1e15, α = 2.3, at 88.2 kHz with Newton iterations.
  - "Well out of real time" (NESS).
  - The other polarisation would need a stick-slip finger, which they did not model.

### 2.3 Finger pressure, pluck strength, action and relief

| Variable | Effect | Source |
|---|---|---|
| Action (θ) | lower: more and longer grazing (prototype θ 7.6→4 mrad: contact to 0.53 s instead of 0.29, partials 6–9 to 0.27 s) | [T], lore agrees |
| Relief | near zero is preferred; relief raises the clearance far from the finger, so it shortens the grazing length | lore [web]; [E] |
| Pluck depth | below A_crit ≈ θ x_p no contact; 1 mm at 6th pos.: sparse contact, the notches fill; 2.5–4 mm: partials 5–9 bloom at 110–300 ms | [T] |
| Pluck angle | only the into-board share touches | [E] |
| Finger pressure | firm: the string fully down, the termination stiff and clear. Light: a buzzy, lossy, "dead" end (towards a harmonic touch) | lore [memory]; no data |
| Finger loss | with no fingertip loss the bloom lasts to 0.3–0.7 s and the contact to 1.3 s; with the double bass's fingertip loss, to 0.3 s | [T] |
| Board hardness | K from 3e3 to 3e5: details of partials 6–10 change by a few dB; contact end 0.37–0.46 s | [T] |

### 2.4 Decay against a fretted bass

- The fingertip termination is lossy where a fret is metal. The double bass model uses an extra
  loss of 0.3 + 0.12 k + 15 (f / 4 kHz)² per second on a stopped string (`bass.cc` 288–289).
  A fretted string end gets × 1.15 (`guitar.cc:196`).
- **No measured fingertip loss values exist** **[web search]**.
  - Schelleng and Woodhouse model the finger as a dashpot; no values were seen **[web, snippet]**.
  - My prototype with the double bass's finger loss: the open-E-class string at the 6th position
    loses its fundamental at 6–8 dB/s and partials 8–10 at 15–30 dB/s **[T]**.
  - The grazing itself takes energy from partials 1–2 (at 1.2 s the 2nd harmonic is −15.7 dB with
    contact against −4.7 dB without) **[T]**.
- **FluidR3's renders** do not settle the comparison: two different recorded instruments
  (§4.2) **[T]**.
  - Fretless fundamental: −4 to −7 dB/s. Fingered: −11 to −12 dB/s on E1 and D2, −2.5 on C3.
  - Level over 0–3 s: E1 −11.8 (fretless) against −17 (fingered); D2 −14 against −6.7; C3 −14
    against −8.4.
- **Plan:** the fretted J's losses, plus the fingertip termination on stopped notes, scaled to
  half the double bass's values to start. A bass string is steel and heavier, so its end impedance
  is higher against the finger's. Then judge by ear against the recordings **[E]**.

### 2.5 The prototype

- **The model:**
  - One polarisation (towards the board), modal.
  - The E string's numbers from `jazzBass()`, stopped at Ls = 0.6 m (f1 = 59.3 Hz, B scaled by
    (L/Ls)²), modes to 5 kHz (84 of them).
  - Losses: the J's, plus the double bass's fingertip loss.
  - A triangle pluck at 10 cm; the J bridge pickup's velocity at 55 mm.
  - Eight penalty points at 2, 4, 7, 11, 16, 23, 32 and 45 mm behind the finger, with clearance
    θ d.
  - The penalty is `Bass`'s: K = 3e4 N/m, C = 1.5 N s/m, with its step limiter.
- **Results [T]** (harmonics in dB against the fundamental's peak; "tmax" is when each peaks):

| Case | Contact ends | 6th harmonic at 0.1 s | Partials peaking late |
|---|---|---|---|
| No contact, 2.5 mm | – | −62 (pluck-point notch) | none (all at 33 ms) |
| 1 mm | 0.29 s (0.7 % of samples) | −2.5 | 5–6, 9–11 at 60–130 ms |
| 2.5 mm | 0.29 s (2.6 %) | −8.3 | 6–9 at 110–280 ms |
| 4 mm | 0.44 s (4.3 %) | −11.5 | 6–9 at 140–270 ms |
| 2.5 mm, low action θ = 4 mrad | 0.53 s | −14.5 | 6–9 at 160–270 ms |
| 2.5 mm, high action θ = 12 mrad | 0.29 s | −5.2 | 4–5 at 220–230 ms |
| 2.5 mm, Ls = 0.4 m, θ = 19.5 mrad (high on the neck) | 0.32 s | −14.2 | 3–4, 6 at 150–190 ms |
| 2.5 mm, no fingertip loss | 1.30 s | −7.6 | 5–10 at 140–730 ms |

- **Convergence [T]:**
  - 48 kHz and 96 kHz give the same numbers to 0.1 dB.
  - 4, 8 and 24 contact points give the same picture: notches fill, partials 5–9 late. The
    details differ by a few dB and the late peaks move by up to 100 ms.
  - The published hard-contact references need 1–2 MHz (Issanchou 2017, stiff contact K = 1e13)
    **[paper]**. A soft penalty as in `Bass` does not, and the mwah is a soft-ish contact
    (wood or epoxy, string windings).
- **What it lacks:** the second polarisation, the pickup's magnetic nonlinearity, string
  stretching (tension modulation), and friction along the board.

---

## 3. Playing

### 3.1 Intonation (the player's own error)

- No fretless-bass intonation study was found. String players tend to play sharp rather than
  flat (Geringer 1978; Geringer & Madsen 1987) **[web, snippet]**. String players also tune about
  3 c lower when playing with vibrato (Geringer) **[web, snippet]**.
- Evidence on ensembles and harmony: §7.1.
- **Plan [E]** (the numbers are the choir's and the section strings' in this project, scaled for
  a bassist):
  - a fixed per-player bias of 0 ± 3 c (§7.1: string players sharp, low voices flat);
  - a fresh per-note landing error with sd 6 c (a good player) to 12 c (a casual one);
  - a correction towards the target with a 60–120 ms time constant (§3.2);
  - longer notes corrected more completely (a residue of sd 3 c after 300 ms).

### 3.2 The landing and the scoop

- **Recardo** (bushgrafts), channel 1 on GM 36, 592 bend gestures on 707 notes. A typical one
**[T]**:

```
t (ms)    0    9   18   27   36   45   55   64   73   82  109  136  164  209  291
bend  -1408 -1216 -1088 -768 -512 -192   0  448  544  608  544  352  224   96    0
```

  - With the default ±2 semitone range, that is −34 c at the onset, crossing 0 at 55 ms,
    overshooting to +15 c at 80 ms, and settling by about 290 ms. It is a damped approach: the
    finger lands flat, slides up, overshoots and settles.
  - 72 % of the gestures start downward. They are 55–230 ms long (median 190 ms) **[T]**.
  - Whoever sequenced it meant it as the fretless sound.
- **Plan [E]:** each stopped note starts at the landing error e₀ and moves to the target. The
  movement is a critically damped to slightly underdamped second-order glide: ζ ≈ 0.7,
  t_settle ≈ 150–300 ms, an overshoot of 10–40 % of e₀.
  - e₀ = the human error (sd 6–12 c), plus a deliberate scoop of −(15–35) c on a share of notes:
    - 30 % of long notes (> 0.4 s) at velocity ≥ 80;
    - 0 % in fast lines (inter-onset interval < 0.2 s).
  - The scoop is the Recardo shape scaled.

### 3.3 Slides and glissandi

- **Data:** cello portamento in recordings 1930–2012 (arXiv 2604.22037) **[paper, HTML]**:
  - a clean shift is under 50 ms;
  - a slide takes 45 ms at the fastest (≈ 4 440 Hz/s) to 330 ms at the slowest (≈ 600 Hz/s);
  - era means 1 980–3 065 Hz/s.
- No fretless statistics were found.
- **In bass terms [E]:** at 100 Hz a semitone is 6 Hz. 2 000 Hz/s on a cello's 300–600 Hz
  register is roughly 6–12 semitones per second. A bassist's slide of 2–5 semitones therefore
  takes 100–400 ms. Long Karn-style glissandi go at 5–15 semitones per second.
- **In the user's files [T]:**
  - 29 % of successions overlap: p10 13 ms, median 60 ms, p90 0.54 s.
  - The most common overlapped intervals: −5, +12, +7, −12, −2, −1, −7, −3, +1, −4. Octave and
    fifth leaps are not slides.
  - These files are keyboard-played jazz bass lines, so overlap here means keyboard legato.
- **Mapping from MIDI [E]:**
  - A new note while the previous key is still down, on the same string, at most 4 semitones
    away, with an overlap of at least 40 ms: a **slide**.
    - The finger glides from the old length to the new one in t = 60 ms + 35 ms × |Δ semitones|,
      at most 300 ms.
    - The shape is an S-curve (raised cosine), starting at the new note-on.
    - No new pluck. Above about 0.3 s since the last pluck, add a light "re-pluck" only if
      velocity > 90.
  - An overlap under 40 ms or a larger interval: a normal new pluck. The slide from the old note
    is cut short by the plucking finger.
  - Probability gate: in a jazz-band file (where GM 36 is the upright) never. Elsewhere,
    overlapping steps of 1–2 semitones slide with probability 0.6 and 3–4 semitones with 0.3. A
    seeded RNG keeps renders deterministic.
  - **Pitch bend** moves the stopping finger continuously, on stopped notes only: `bend(s, cents)`
    becomes a change of length. With bends present, the player prefers stopped positions to open
    strings for that note.
  - **The energy in a glide:** keep each mode's action, its energy over its frequency (an
    adiabatic invariant). The displacement amplitude then scales as ω^−½ when the length changes
    (`Guitar::tune` today keeps q and v as they are) **[E]**.
  - **Squeak:** a roundwound slide adds a short band-limited noise at speed / winding pitch,
    −30 dB under the note (bass_section.md §6.5) **[E]**.

### 3.4 Vibrato

- **Data:**
  - String vibrato extent 20–35 c peak to peak, rate 5.5–8 Hz (Seashore, Sundberg, Meyer)
    **[paper, mmm-35]**.
  - Violin about 30 c, range 10–60 (Fletcher & Sanders 1967) **[web]**.
  - Cello recordings average 6.5 Hz (sd 0.4). Double bass (Mick 2024): width and rate rise with
    register **[paper, arXiv 2512.18162]**.
  - No onset-delay data. 100–300 ms is commonly cited **[memory]**.
- **Plan [E]:**
  - Rate 4.5–5.5 Hz (40 university double bassists: mean 5.17 Hz, 19 c wide [web, scite]; same §7.1). A bassist's vibrato is slower; `BassSpec::vibRate` already scales the
    double bass's.
  - Depth ±8–15 c (16–30 c peak to peak), wider high on the neck.
  - Starts 200–350 ms after the onset, ramping in over 200 ms, on notes longer than 0.5 s, on a
    stopped string only.
  - In MIDI: CC1 adds depth (0–127 to 0–±25 c). The director's cue (`Performer::direct`) sets the
    depth and rate as for other players.
  - In the user's files CC1 is dense on several GM 36 channels (857 events in "Gone with the
    wind"), all inside the jazz-upright files **[T]**.
  - Vibrato is a finger rocking, a length change: the same `bend` path as the slide.

### 3.5 Harmonics

- Natural harmonics at 1/n of an open string (n = 2–6: the 12th, 7th, 5th, 4th and 3rd
  positions) and artificial harmonics (stopped at one point, touched an octave up) **[memory]**.
- `Guitar::touch(s, n, secs, rate)` already does it.
- **Mapping [E]:** notes above the fingerboard's top (G2 + 20 = D#4, MIDI 63) are played as
  harmonics when they fit a natural harmonic of an open string within ±20 c. Otherwise they are
  played an octave down (as `ebass.cc` does). A knob `PJ_FRETLESS_HARMONICS=1` instead plays every
  note from MIDI 60 up as a harmonic when it fits: the "Portrait of Tracy" mode.

---

## 4. How the GM sets do program 36

### 4.1 The sets' data

**FluidR3_GM** (preset 0:35 "Fretless Bass") **[T, sf2 dump]**:
- Seven 48 kHz sample zones from E1 to A3, 1.3–3.1 s each, looped only in the last 20–60 ms.
- Volume envelope: hold 3 s; decay 5769–6204 tc (that is 28–36 s).
- A layered "Sine Wave E4": a single-cycle loop at −9 dB, through a 405 Hz low-pass with 4.8 dB
  of Q, a vibrato LFO at 5.6 Hz with no depth set, and velocity-to-filter switched off.
- Reverb send 2 %, no chorus send.

**TimGM6mb** **[T, sf2 dump]**:
- Six zones of single-cycle or very short loops (0.02–0.13 s).
- A filter at 710 Hz, opened by the modulation envelope +3437 c to 5.2 kHz at the onset. It decays
  back over 9.5 s (key 0–38), 5.3 s, 3.8 s, 2.1 s or 2 s up the keyboard.
- TiMidity++ does not play the modulation envelope's filter move (fx_sounds.md). So the renders
  below show TiMidity's static filter, not the SF2's intent: a bright onset darkening over
  seconds.
- Reverb send 3.1 %.

**Jnsgm2** **[T, sf2 dump]**:
- One sample (0.6 s at 14 989 Hz, looped at 0.56–0.6 s) over the whole keyboard, reverb 7 %.
- Preset-level modulators from CC21–24 (filter, envelope, LFO) and velocity to filter cutoff.
  TiMidity ignores modulators.

**one maker** (SC-88Pro owner's manual) **[paper, read]**:

| Variation | SC-88Pro map | later GS module map | early GS module map |
|---|---|---|---|
| 000 | Fretless Bs. | FretlessBs. | Fretless Bs |
| 001 | Fretless Bs2 | FretlessBs2 | none |
| 002 | Fretless Bs3 | FretlessBs3 | none |
| 003 | Fretless Bs4 | FretlessBs4 | none |
| 004 | Syn Fretless | SynFretless | none |

- Voices: the base tone 1 voice, the variations 2.
- The MT-32 map has "Fretless" (126/028) and "Fretless 1/2" (127/071–072).
- The SC-88Pro's built-in "JP FRETLESS" uses Fretless Bs2 with insertion effect 53, "Bass Multi".
- GS part defaults: reverb send (CC91) 40, chorus send (CC93) 0. So **the GM-mode fretless has no
  chorus** on a Sound Canvas unless the file sends CC93.

**XG** (MSB 0, program 36) **[web, Yamaha lists]**:
- LSB 0 Fretless, 32 Fretless2, 33 Fretless3, 34 Fretless4, 96 SynFretless, 97 SmthFretless.
- Element counts 1, 2, 2, 2, 2, 2.
- No sound descriptions were found for either maker.

### 4.2 Renders

**Method:** TiMidity++ 2.14, mono, 44.1 kHz, reverb and chorus off. Each set was forced by an
explicit `%font` line. With a bare `soundfont` line, TiMidity silently kept FluidR3: a trap.
- Notes E1, A1, D2, G2, C3, G3 at velocities 40, 80 and 120, held 4 s; GM 34 at velocity 100 for
  comparison.
- Harmonic tracks from a 4-period window. "tmax" is when each harmonic peaks.

| Set, note | Attack (10 % → peak) | Partials peaking late (ms) | Centroid 0 → 0.2 → 1 s (Hz) | Fund. decay dB/s |
|---|---|---|---|---|
| FluidR3 E1 | 145 ms | none (all ~50) | 132 → 92 → 104 | −6.7 |
| FluidR3 A1 | 95 | h7–h12: 150–610 | 153 → 101 → 92 | −6.3 |
| FluidR3 D2 | 68 | h8–h10: 160–200 | 197 → 130 → 86 | −4.1 |
| FluidR3 G2 | 41 | h6–h12: 150–350 | 224 → 157 → 106 | −3.4 |
| FluidR3 C3 | 48 | **h3–h11: 200–460** | 163 → 171 → 188 | −6.4 |
| FluidR3 G3 | 156 | **h2–h10: 165–385** | **236 → 365** → 232 | −4.0 |
| FluidR3 GM 34 E1 / D2 / C3 | 27 / 18 / 10 | none | 88 → 115 → 112 (E1) | −11 / −12 / −2.5 |
| Jnsgm2 E1 | 11 | h4–h9: 110–350 | 70 → 81 → 70 | −3.5 |
| Jnsgm2 C3 | 3 | h4–h8: 55–130 | 235 → 254 → 154 | −4.3 |
| TimGM6mb E1 | 133 | h3, h7–h10: 300–330 | 68 → 75 → 81 | −2.4 (all equal: a loop) |
| TimGM6mb C3 | 58 | h2–h9: 115–210 | 199 → 386 → 387 | −3.0 |

- **Velocity is level only in all three [T]:**
  - FluidR3 rises 16 dB from velocity 40 to 120, with the same spectrum to 0.1 dB.
  - TimGM6mb and Jnsgm2 likewise in TiMidity. Jnsgm2's velocity-to-filter modulator is ignored.
- **Pitch [T]:** no vibrato in any set.
  - FluidR3's recording wanders 1–4 c (sd), 3–10 c peak to peak, over 0.1–3 s.
  - Offsets: FluidR3 +4 to −1 c, Jnsgm2 −5 to −7 c, TimGM6mb +1 to +4 c.
- **The FluidR3 and Jnsgm2 recordings are the only real fretless data here.** In both, the upper
  partials bloom late in the middle register: h3–h11 at 0.2–0.46 s on C3 and G3. That matches the
  prototype's 0.1–0.3 s. On E1 and D2, FluidR3 shows no bloom, possibly an open-string or low-hand
  recording **[E]**.

---

## 5. Usage in the user's files

- **The scan [T]:** 774 MIDI files in `~/midi`, `~/Music`, `~/Downloads`, `~/Musici` and MIDI zips.
  - 42 channels on program 35 (from 0) with 4 or more notes: about 38 distinct files after
    duplicates (`Anighting.mid` and `georgia.mid` appear in two or three folders).
  - 26 636 notes, almost all in `~/Music/midi/bushgrafts/` jazz-trio files: Autumn Leaves, Spain,
    All The Things You Are, Blue Bossa, Desafinado, and others.
- **What the lines are [T]:**
  - Pitch p10–p90: MIDI 33–50 (A1–D3), median E2.
  - Velocity 40–105, median 67.
  - Duration median 0.30 s (p10 47 ms, p90 0.81 s). Inter-onset interval median 0.32 s: walking
    quarter notes.
  - Intervals: ±1 (5 755), 0 (2 085), −2, −5, +7, −3, −7, ±12. Walking bass.
  - 264 near-simultaneous pairs (double stops or doubled keys).
- **Overlaps:** 29 % of successions, median 60 ms (§3.3).
- **Pitch bend:** 22 channels have some.
  - **Recardo:** 6 202 bend messages, a scoop on almost every note (§3.2).
  - **Desafinado:** 130 upward pushes of about +26 c, 0.11 s long, about 0.17 s after the onset.
  - **The rest:** 3–13 gestures per file, mostly upward, 0.1–0.25 s, peaks 2 000–6 800 (0.5–1.7
    semitones at ±2 st): falls and scoops of a walking bass.
  - No RPN 0 bend-range messages were found in the checked files.
- **CC1:** 3 776 events, continuous, values 0–127 (median 13–66 per file): a mod wheel in use.
- **So:** in these files GM 36 is the upright, as `midifile.cc`'s jazz-band rule says. The
  fretless model must not take them over. When `-inst fretless-bass` is asked for explicitly, the
  same lines should still play well: walking-bass legato without false slides (the 40 ms and
  4-semitone gate), and scoops from the bend wheel.

---

## 6. Amp and effects chain

- **Jaco** (§1.1): the bridge pickup, an Acoustic 360 with the mids pushed and sometimes its fuzz,
  two rigs with a short delay for doubling.
  - No circuit for the 360 was found (bass_amps.md §3.4). Its controls are known: volume, bass,
    treble, fuzz, and a 5-position Variamp **[web, Fretboard Journal]**.
  - The 361 cabinet: about 200 W into a rear-firing 18" folded horn **[web]**.
- **Records 1975–90** (bass_amps.md §2.2): a J or P fretless, bridge pickup favoured, DI-heavy,
  often chorus and reverb.
- **Chorus:**
  - one pedal maker CE-2 (MN3007 bucket brigade, 5–51 ms chip range; a triangle LFO; rate and depth only)
    **[web]**.
  - Typical settings: rate 0.3–1 Hz, centre delay 5–8 ms, depth ±1–3 ms **[memory]**.
  - The CE-2B keeps the lows dry **[memory]**.
- **Compression:** opto, 3:1, 2–4 dB of gain reduction (bass_amps.md §10.3) **[E]**.
- **Proposed preset "fretless-di"** (bass_amps.md §10.3, made concrete) **[E]**:
  1. Pickups: bridge 1.0, neck 0.5 (`PJ_FRETLESS_PICKUP`); the J network as in `ebass.cc`.
  2. An active DI into 1 MΩ.
  3. A mid peak of +3 dB at 800 Hz, Q 0.8: the Variamp's push.
  4. Optional: the fliptop path blended at 30 % for warmth.
  5. A chorus, off for GM 36 by default, as on the later GS module. `PJ_FRETLESS_CHORUS` 0–1 adds a BBD-like
     modulated delay: 7 ms ± 2 ms at 0.6 Hz, mix 35 %, the lows below 150 Hz kept dry, stereo by
     inverting the LFO on the right.
  6. optical compressor at 3:1.
- The chorus is the only new block. It is a modulated fractional delay, about 100 lines of code.
  The amp module has none yet.

---

## 7. Intonation against the band: the spectral solver

The user asked for this (2026-11-20): the fretless player should tune its note by ear to what
else is sounding, within ±15 c of equal temperament.

### 7.1 Evidence on how bassists tune against harmony

- **No measured study of fretless or upright-bass intonation against the harmony was found.**
  That includes whether bassists lock to the root or shade thirds and sevenths.
- **SATB quartets** (Dai & Dixon, JASA 2019) **[paper, full text]**:
  - Adjacent parts differ by about 8 c: sopranos sharp, tenors and basses flat. Harmonic
    intervals are stretched and melodic ones compressed.
  - Pitch is unstable for about 120 ms at the start and end of each note.
  - The ensemble drifts flat over time.
  - Basses had the lowest mean absolute pitch error (Dai & Dixon 2017, cited there).
  - Most intervals measured in vocal ensembles lie within 1 sd of equal temperament (Devaney et
    al. 2012). Preferred major thirds lie closer to ET than to pure (Nordmark & Ternström 1996).
    Barbershop intervals fit neither just nor Pythagorean (Hagerman & Sundberg 1980). All cited
    there.
- **Violinists** (Weng, PhD, Lund/RCM 2025) **[paper, summary chapters]**:
  - Concertmasters' performed leading-tone semitones averaged 78–89 c; 60 c was "not at all
    rare". Under 80 c they were rare on notes longer than 1 s, and tighter on short and off-beat
    notes.
  - Melody pulls towards Pythagorean (high sharps), harmony towards just (Leedy & Haynes 2001).
    So soloists come closer to Pythagorean and quartets closer to just.
  - Nickerson 1949 (24 quartet players): closest to Pythagorean, consistent with no single
    system **[web, snippet]**.
  - Kopiez 2003 (trumpets): nearer ET than just, a "burn-in" from ET practice **[web, snippet]**.
- **Double bass:**
  - Borém & Lage 2019: solo pizzicato note-finding. Overshoots under 0.7 % (about 12 c) are
    treated as imperceptible **[web, snippet]**.
  - 40 university bassists' vibrato: 5.17 Hz, 19 c **[web, scite snippet]**.
  - Priester (IU thesis): a normative model, no data **[web, abstract]**.
- **Lore [memory]:** bassists tune the root and fifth pure (against open strings or harmonics)
  and leave the colour of thirds and sevenths to the upper voices.
- **Consequences for the model:**
  - The bass is the steadiest voice: a small error, no systematic stretch, and its root held near
    the band's.
  - The spectral rule gives exactly this. Over a chord with its root above, it barely moves
    (§7.3); under a bare third or a shell it shades.
  - A "low voice slightly flat" of about −3 c can be the per-player bias's sign for the bass,
    instead of the +2 c that string players in general show. The two findings conflict; take
    0 ± 3 c per player.
  - Tight semitones (sharp leading tones, 80–90 c) belong to melodic, solo lines. They are a
    melodic rule for a fretless solo (notes above C3 in a stepwise line): a leading tone resolving
    up by a semitone is placed +8 c. It is not part of the bass's harmonic solve.
  - Melody and harmony cannot both be applied without a conflict. The melodic +8 c is added after
    the solve, then the ±15 c clamp applies.

### 7.2 The pajupilli intonation as it is

All three users solve a small least-squares problem over the sounding notes.

**The flutes**: `Ensemble::retune()`, `PJ_JI`; the rules are in `ensemble.h` lines 25–40.
- Every pair is pulled to its nearest simple ratio.
- Notes younger than 150 ms are still being placed. If the young notes are the minority they fit
  the old ones; if they are the majority they are placed together with their mean at 0 c.
- The solve runs when the set changes and every 100 ms otherwise.
- The offsets heal towards 0 c with a 3 s time constant.

**The organ**: `OrganModel::justSolve()`, `organ.cc:1812`.
- The same structure, over the held keys.
- Each key's spectrum is read from its pipes' modelled outputs, and pairs of partials pull by the
  weaker partial's level.
- Window 36 c with a Gaussian of width 20 c.
- Limits: ±30 c; a pedal key ±15 (`PJ_JI_RANGE_PEDAL`: "the bass's beats slow, its pitch the
  harmony's floor"); the top key ±10.
- A sounding key moves at most 3 c per solve.
- `justGlide()`: no glide quicker than 25 ms, none faster than 1 c/ms.

**The choir**: `Ensemble::choirTune()`, `choir.cc:528`.
- The spectra are a fixed template: harmonics up to 5 kHz at 1/k, with the formant band near
  700 Hz raised ×3.
- The weight of a pair of partials: (weaker level) × exp(−(δ/20)²) / (higher harmonic number)².
- Limit ±25 c, at most 3 c per change, each singer following in about 80 ms.

### 7.3 Anchor or follower?

- **The organ's answer:** the pedal is a narrow-limit participant (±15 c). It is part of the
  solve, but other notes move more easily.
- **Real players do both [memory; see 7.1]:**
  - they lock the root to the kick and the piano's root (an equal-tempered piano can't follow
    them);
  - they shade thirds and sevenths when the bass plays them;
  - they bend into notes.
- **Recommendation:** the bass is a **follower** of the fixed-pitch instruments and an **equal
  participant** with the adaptive ones.
  - Against a piano, guitar, tine piano, vibes or any keyboard (fixed in equal temperament), only the
    bass can move. Its root against the piano's root is pulled to 0 by strong octave
    coincidences, and it shades only when it plays a chord tone other than the root over a sparse
    voicing.
  - Against the adaptive families (flutes, organ, choir, strings), the bass's note enters their
    solve as a narrow-limit key (±15 c, like the organ's pedal). Its weight is high, so the others
    tune to it more than it tunes to them. The bass is the anchor among the adaptive players.
- **What the template rule gives [T, `fretless_ji.py`]** (the choir's weighting, the fretless's
  spectrum from FluidR3, a 1/k template for the others, the others fixed at 0 c):

| Bass, against | The bass wants |
|---|---|
| C2, against E3 alone | +13.7 c (its 5th partial meets E3's 2nd) |
| C2, against C3 E3 G3 | +0.1 c (the octaves hold it) |
| C2, against E3 G3 B♭3 (rootless C7) | +0.6 c |
| E2 (the third in the bass), against C3 E3 G3 | −0.1 c |
| E2, against C3 G3 (no E above) | −4.6 c |
| G1, against B2 F3 (G7 shell) | +14.5 c |
| A1, against C3 E3 G3 (Am7) | −1.5 c |
| C2, against an already just E3 (−13.7) | 0.0 c |

- **What the solver does by itself:**
  - With the root doubled above, the bass stays put.
  - Under shell voicings and dyads it shades by up to 15 c.
  - Where an adaptive player above has already tuned justly, the bass has nothing to correct.
  - The weights differ by 20× between these cases. The total weight (sw) can therefore also set
    how firmly the finger seeks: a weak pull is a slow, partial correction.

### 7.4 The missing piece: what one channel knows of the others

- **Today:**
  - `Performer` knows only its own `note_`.
  - `midifile.cc`'s player loop keeps `heldNote` (channel × 128 + key → times down, lines
    1109–1274). The harmonica's `setAccompaniment` reads it each block (lines 1388–1393): a scalar
    of register busyness.
  - Nothing exposes pitches with spectra, notes ringing under the sustain pedal, or other parts'
    tuning offsets.
- **Design A: a registry of sounding notes with templates (recommended).**
  - A struct `Sounding { int ch; int key; double cents; double level; const Template* spec; }`,
    kept by the event loop of `midifile.cc`.
  - **On:** note-on adds an entry, with level from velocity.
  - **Off:** note-off marks it released. With CC64 down, or for a family that rings (piano,
    guitar, vibes, harp), it stays with a level decaying at the family's rate: piano about
    −8 dB/s, tine piano −6, vibes with the motor off −3. It is dropped below −40 dB. An organ or choir
    note drops at once.
  - **Templates per family** (a 24-entry level table each, built once): piano 1/k with stretch
    (B from the piano model); organ from `OrganModel`'s cached spectra if present, else 1/k;
    choir as in `choirTune`; flute strong 1–3; strings 1/k; brass 1/√k to 2 kHz; guitar and bass
    from the pluck position. A refinement can come later: each `Performer` may report its own via
    a new virtual `partials()`.
  - **Cents:** an adaptive part (flutes, organ, choir) writes back its current offset per key
    through a hook, `Performer::tuning(key)`, default 0. A fixed-pitch part's cents are 0, or its
    `-transpose` and tuning knobs.
  - **The hook:** a pointer `const SoundingSet* band_` handed to every `Performer` through a new
    `setBand(const SoundingSet*)`. It is read at the start of each block. The event loop updates
    the set between blocks, before the pool's threads render, so there is no locking: render
    reads, the event loop writes, and they never overlap.
  - **Latency:** none beyond the block (32 samples, 0.67 ms). The bass knows a chord at its
    note-on, as a player who reads the chart does.
  - **Cost:** a solve per change, plus one every 50–100 ms. The pair loop is (bass partials 16) ×
    (others' partials 24) × (notes ≤ 32): about 12 k multiply-adds, roughly 10 µs. Negligible.
  - **Other users later:**
    - the organ, choir and flutes could take the band's fixed notes as fixed entries in their own
      solves (an organ tuning to a piano);
    - `setAccompaniment` becomes a sum over the registry;
    - a director can read harmony (chord quality) from it;
    - a sitar's drone could tune its sympathetic strings to it.
- **Design B: analysing the other channels' audio.** Each part's previous blocks would go
  through a pitch-partial tracker.
  - It is more "honest", but resolving partials 36 c apart at 100 Hz needs windows of about 0.3–
    0.5 s. That is 300–500 ms of latency, a full-rate FFT per part, and octave errors.
  - It also sees what the room and reverb add.
  - Rejected for now. It could later replace the templates by measuring each part's real spectrum
    offline (per program and register) into the template tables.
- **Hybrid for live (`src/live.cc`):** only keyboard input exists there, so the registry is just
  the held keys. The same code applies.

### 7.5 The fretless solver

- **Who solves:** the fretless's `Performer` itself, each control block (16 samples), reading
  `band_`. It needs no global solver: it is one moving note against a set of others.
  - Its own pitch c_b is the only unknown. Every other note is fixed: its `cents` from the
    registry.
  - For adaptive parts, the bass's sounding note is in turn written to the registry, so they treat
    it as fixed with a narrow range. The two never solve each other at the same time: the bass
    reads the state of the last block. This is a Gauss–Seidel iteration that settles in a few
    blocks, since each side moves at most 3 c per change.
- **The target:**
  - target = clamp(Σ w·δ / Σ w, −15, +15) c, using the choir's pair weights.
  - It heals to 0 c with time constant 3 s while nothing pulls (sw < a floor), as the flutes do.
  - With no other channel sounding (or `PJ_FRETLESS_JI=0`), the target is 0.
- **The finger's motion** (in cents of the stopping length):
  1. **Landing:** at the pluck the finger is at target + e₀. e₀ is the human error, sd 6–12 c
     (§3.1), and on scoop notes −(15–35) c (§3.2).
  2. **Correction:** a second-order approach to the target, ζ = 0.7, natural period 150–250 ms.
     That gives about 80–120 ms to 90 %, with the Recardo-like overshoot when e₀ is a scoop.
  3. **Following:** while the note sounds and the target moves (a chord changes above a held
     bass), the finger moves at most 1 c/ms and no quicker than 25 ms per change (the organ's
     `justGlide` limits). A change under 2 c is ignored: a player does not chase a 1 c beat on a
     bass note.
  4. **Residual:** the correction is incomplete. The finger stops within ±(2 + 0.3 · sd) c of the
     target, a fresh random residue per note, with the per-player bias (0 ± 3 c).
  5. **Vibrato** (§3.4) is added after the onset delay, around the corrected centre. Real
     players' vibrato is roughly centred on the target **[memory]**. While vibrato is on, the
     error term (step 4) is halved, because vibrato masks it. The 3 c-lower-with-vibrato finding
     can be a −2 c centre shift.
  6. **Slides** (§3.3) go to the new note's target, not to its equal-tempered pitch.
- **Limits:** the sum of the solved offset and the residue stays within ±15 c. Scoops and vibrato
  are excursions on top: the scoop transient up to −35 c, vibrato ±25 c.
- **All of it is one quantity:** the cents of the stopping finger, applied as the stopped length
  (§8.2). One code path drives intonation, scoop, slide, vibrato and pitch bend.

### 7.6 The winds by ear (brass and reeds; built 2026-11-21)

The user (2026-11-20): "all horns and reeds should use spectral tuning". Every brass part (trumpet,
muted trumpet, trombone, horn, tuba, the brass section) and every blown reed (the saxes, oboe,
English horn, bassoon, clarinet; duduk, shawm, shehnai) now tunes by ear to the registry (§7.4).
Not the free reeds (harmonica, accordion, bandoneón: fixed reeds) and not synth brass.
`src/windear.cc`, `Ensemble::windEar`; `PJ_WIND_EAR=0` gives the old behaviour exactly (checked:
byte-identical renders of a brass-section file and a four-wind file).

- **The rule is the fretless's (§7.5)**, factored into `bandPull()` (`src/band.cc`), which the
  fretless now calls too (its renders byte-identical before and after):
  - each note's own partials against every other note's template; pairs within 36 c pulling by
    min(levels) · exp(−(δ/20)²) / k², followed by the weight w as w/(w + 0.0005), within ±15 c;
  - the winds' own templates: brass 1/√k to 2 kHz (then 0.5/k); the reeds 1/k; the clarinet its odd
    partials 1/k, its even ones 0.1/k below 1.5 kHz (the tone-hole lattice's cutoff, Benade: about
    −20 dB in the chalumeau) and 0.6/k above. The clarinet is now its own family in the registry,
    so the others hear it so too.
- **The lips are slower than the fretless's finger:** a held note is retuned over 0.1 s or more, at
  most 0.2 c/ms (the finger: 25 ms, 1 c/ms), and a change under 3 c is let be (the finger: 2). In a
  busy texture (a Bach chorus's running strings) passing notes pull a few cents either way within
  100–200 ms; the finger's limits let a held trumpet note follow them as a 2–3 c flutter.
- **A section's own intervals and the band, solved together:** in a part playing chords
  (`Ensemble::retune`, the pajupilli intonation) each voice's band pull b_i takes the place of
  equal temperament in its anchor: a new note's a_i = b_i, a sounding one anchored where it was
  plus what its pull has moved since the last solve, the young notes' mean and the top note pulled
  to their b's instead of 0, the healing towards b. The pairs still ask for their just intervals.
  A pull moved by 1 c asks for a new solve (not printed). A single voice plays b. The brass
  section's lean to equal temperament (×0.5) scales the whole.
- **Reaching the target:** the reeds' own ear (`ReedPlayer`, ±120 c) plays the target to within
  0.5 c. A brass player's ear trims at most 0.8 % of the lips' tension with a 5 c dead band
  (`HornPlayer`): it reaches ~5 c. A lone brass player therefore sets its lips for the pull as for
  the pitch wheel (the lip roll: the lips' resonance × e^(0.06·roll), 600 c per unit of its log,
  36 c a roll unit), the ear trimming the rest. The section's seats set their lips by their own
  feed-forward (`section_`).
- **Measured** (each wind holding a note 2.5 s over a piano's or a string pad's chord, the backing
  heard but not rendered; the logged target, then the played pitch from the spectrum, and in
  brackets the same render with `PJ_WIND_EAR=0`; the brass in its octave):

| | E4 over C3 G3 | G4 over C3 E3 | C4 over C3 E3 G3 |
|---|---|---|---|
| target (all) | −13.3 c | +1.9 | −0.1 |
| alto sax, oboe, clarinet, bassoon | −12.7 to −13.6 (−0.2) | +1.7 to +1.8 (−0.3 to +0.1) | −0.4 to +0.4 |
| horn, tuba | −13.2, −12.7 (−2.6, +7.5) | +3.4, +6.6 (+1.8, +4.6) | −3.1, −2.8 |
| trumpet, trombone | −7.4, −7.7 (+4.6, +5.1) | +6.4, +6.8 (+6.4, +5.0) | +6.3, +4.4 |
| brass section (×0.5) | −3.9 (+3.8) | +2.5 (+1.2) | +1.1 |

  The same over C2 C3 G3 and over strings instead of the piano. The reeds land within 0.5 c. The
  brass move by −12 to −20 c on the third; what is left is the player's own error on that note
  (the trumpet's and trombone's +5 to +6 c without the band too, their ear's dead band), not the
  band's.
- **No loops (`src/band.h`):** a note of a part tuning by ear (a wind, the fretless) is heard only
  by the notes begun after it (more than 35 ms later; struck together, the lower by the higher, a
  unison by channel). The newcomer fits in, as the young notes do within a section. Who hears whom
  is a fixed order: two winds never chase each other (A to B's −14 c, B to A's, ...).
- **No wander:** a chain of winds can still carry an offset on through held common tones: the
  comma pump (C Am Dm G C, every interval just, 21.5 c a round). With the order alone, four winds
  (trumpet, clarinet, alto sax, trombone, no fixed pitch) settled with their C chord's mean at
  −10.5 c, held there by the ±15 c clamp. Hence the winds' common healing (`SoundingSet::drift`):
  every held wind note moves at once (their intervals kept) by −(Σ level · share · cents)/(Σ
  level)/1 s, summed over the winds and the fixed-pitch notes, `share` being how much of the
  note's pull comes from other parts tuning by ear (what a piano asks for is the piano's to keep;
  a note hearing nothing: all). Then the C chord's mean stays at −2.9 c round after round (its
  just form, 0 −14 +2 0, has a mean of −3), the fastest change within a note 0.45 c/ms.
- **A real file:** `bwv0248a.mid`'s first 30 s (two trumpets, two oboes, flutes, strings, choir,
  harpsichord, continuo; under the user's remaps alto sax, clarinet, trombone, baritone sax,
  bassoon): 309 wind notes landed, their pulls' mean −0.4 c, sd 3.0 c, 5 % beyond ±5 c, the
  extremes −14.8 and +14.2 c. The band's part of a held note moves at most 0.2 c/ms; the faster
  changes (to 0.8 c/ms) and the largest offsets (to 24.5 c) are the trumpet chord part's own
  retune (8 notes at once), the same with `PJ_WIND_EAR=0`. No wandering: the winds' landings
  have no trend over the 30 s.
- **Cost:** a hearing per note at its onset, on each change of the registry and every 50 ms:
  (16 own partials) × (24) × (the sounding notes), ~10–100 µs. The 30 s of `bwv0248a.mid` took
  797 s of CPU with it and 806 s without (a loaded machine: the same within its noise).

---

## 8. Build plan

### 8.1 What to reuse

| From | What |
|---|---|
| `ebass.h/.cc` (`jazzBass`) | the strings, losses, neck dead spots, pickups (bridge favoured), the DI biquad, the player's string choice, the fingers alternating, the pad pluck (`pluckPad`), damping at note-off |
| `guitar.h/.cc` (`Guitar`) | the modal string (to 128 modes, two tilted directions), `bend` (retune), `touch` (harmonics), the pickups' ears, `mute`, the asleep logic |
| `bass.h/.cc` (`Bass`) | the board-contact loop (penalty, step limiter, `boardOn` clock), the stopped-string fingertip loss, `setLength`'s glide, `kBoardEnd`, `setAction` |
| `guitaramp.h/.cc` | the DI, fliptop and optical compressor; a new "fretless-di" preset (§6) |
| `organ.cc` / `choir.cc` | the spectral pair weights, the 3 c-per-change and glide limits (§7) |

### 8.2 Changes to `Guitar` (they are general; the fretted bass loses nothing)

1. **A continuous stopping length.**
   - `Guitar::stop(s, cents)` sets the stopped length to scale × 2^(−(fret + cents/100)/12).
   - It recomputes the frequencies and losses each control block while moving, and the pickup
     ears and board shapes every 4th block during a slide (place = true). Cost: a sin per mode per
     ear, about 2 × 100 per 64 samples.
   - Each mode's amplitude is rescaled by √(ω_old/ω_new) to keep its action (§3.3).
   - `fret()` and `bend()` become cases of it.
2. **The board behind the finger** (fretless spec only, `GuitarSpec::board` = true).
   - Five points, towards-the-top direction only, at 3, 7, 13, 22 and 36 mm behind the finger.
   - Clearance θ·d + ½κd², with θ = action / (len − boardEnd) and κ from `relief`.
   - Penalty K = 3e4 N/m, C = 1.5 N s/m (`Bass`'s, with its step limiter `hold`). Knobs
     `PJ_FRETLESS_BOARDK`, `_BOARDC`.
   - Active only while the string is stopped and loud, and for 1.5 s after a pluck (`Bass`'s
     `boardClock`).
   - Cost while active: 5 × (2 dot products + 1 axpy) × ~100 modes ≈ 1.5 k flops per sample per
     string. With one or two strings sounding, ~150 MFLOP/s, which SIMD makes small. The guitar's
     own step is ~1 k flops per string.
   - Points closer than 1.5 mode wavelengths to the finger see no low modes. That is right: the
     contact talks to the high modes.
3. **The fingertip termination** (stopped strings, fretless only).
   - Extra loss 0.5 × (0.3 + 0.12 k + 15 (f/4 kHz)²) /s (`PJ_FRETLESS_FINGER`, 0.5 to start).
   - The ×1.15 fret factor is dropped.
   - A slight end softness: the high partials' frequency lowered by 0–1 c. Not needed at first.
4. **Open strings:** the nut. No board contact, no finger loss. The player avoids open strings
   for notes that want vibrato, scoops or slides.

### 8.3 The player: `FretlessBassPlayer` (`src/fretless.cc`, or a mode of `ElectricBassPlayer`)

- String choice as in `ebass.cc`, with a cost for open strings on notes longer than 0.4 s.
- `stopCents_` per string: landing (§7.5.1), correction (§7.5.2–4), slide (§3.3), vibrato
  (§3.4), pitch bend (`roll_`, ±2 st).
- The slide gate: overlap ≥ 40 ms and |Δ| ≤ 4 semitones on the same string. In a jazz-band file,
  or with `PJ_FRETLESS_SLIDES=0`, no slides.
- Harmonics above the stopped range (§3.5).
- The pluck: `pluckPad` with the angle raised to 0.5–0.7 rad (more into the board), from about
  9 cm (between the pickups, nearer the bridge).

### 8.4 Mapping

- `instruments.cc`: `fretless-bass`, family "bass".
- `midifile.cc:191`: `{"fretless-bass", "double-bass"}` stays the fallback.
- GM 36 plays the fretless **unless** the jazz-band rule holds (piano plus GM 33 or 36). Then it
  stays the double bass, as now. This keeps the user's 2026-11-16 decision for the bushgrafts
  files.
- `-inst fretless-bass` forces the fretless anywhere.

### 8.5 The registry (§7.4)

- `src/band.h`: `SoundingSet` and the family templates.
- In `midifile.cc`: update it at note-on, note-off and CC64, and decay the ringing entries per
  block.
- In `Performer`: `setBand()` and a virtual `tuning(key)`.
- The fretless reads it; so do the brass and the reeds (§7.6). The organ, choir and flutes can
  follow later.

### 8.6 Tests (`tools/test.py`, the existing style)

1. **Mwah exists:** a stopped A1 (5th position on E) plucked hard against the same note on the
   fretted J: in the fretless, h6–h9 peak 80–400 ms after the onset, against < 60 ms fretted.
   The pluck-position notch is filled: the harmonic at x_p / Ls is within 15 dB of its
   neighbours.
2. **Soft is clean:** at velocity 30, the board contact is touched in under 0.5 % of samples
   (a debug counter), and the late peak is absent.
3. **Open string:** open E has no contact at any velocity.
4. **Action:** `PJ_FRETLESS_ACTION` 0.6 against 1.4: the contact lasts at least 1.5× longer at
   0.6.
5. **Decay:** a stopped note's partials above 1 kHz decay at least 1.5× faster than the fretted
   J's at the same fret. The fundamental's decay is within ±30 %.
6. **Convergence:** the same note at 48 and 96 kHz: harmonic levels at 0.1, 0.3 and 0.6 s agree
   within 1.5 dB.
7. **Slide:** C2 then D2 overlapping by 100 ms on the A string: pitch moves monotonically from
   C2 to D2 in 120–160 ms, with no new attack transient (peak level change < 1 dB). With 20 ms of
   overlap: a new pluck.
8. **Glide energy:** a 5-semitone slide up keeps the amplitude change within the adiabatic
   prediction ±1 dB.
9. **No false slides in walking lines:** render `Autumn Leaves` with `-inst fretless-bass`: fewer
   than 5 % of notes slide (most overlaps are octaves, fifths or fourths).
10. **Scoop and landing:** with `PJ_FRETLESS_ERR=0` and a forced scoop, the pitch track starts
    25–35 c flat, crosses the target at 40–80 ms, overshoots 5–15 c and is within 2 c by 300 ms.
11. **Vibrato:** a 2 s note: no pitch modulation before 200 ms; then 4.5–5.5 Hz, ±8–15 c.
12. **Intonation, the band:**
    - (a) Bass C2 under a held E3 on a piano channel: the bass settles +10 to +15 c within
      300 ms. Measure the bass's 5th partial against the piano's 2nd: the beat rate falls below
      0.5 Hz.
    - (b) Under C3 E3 G3: within ±2 c of 0.
    - (c) G1 under B2 F3: never beyond +15 c (the limit holds; the unclamped wish is +14.5, so
      also test a case that wants > 15 and check the clamp).
    - (d) No other channel: with `PJ_FRETLESS_ERR=0` exactly 0 c. With the human error on, the
      mean over 200 notes is within ±1 c of the player's bias and the sd is 6–12 c.
    - (e) A chord change above a held bass note: the finger moves ≤ 1 c/ms and ignores changes
      under 2 c.
13. **Registry:** a piano note released under CC64 stays in the set and decays. Released without
    the pedal, it leaves. An organ note leaves at once.
14. **Jazz-band rule:** a file with piano and GM 36 still plays the double bass on GM 36. A file
    without piano plays the fretless.
15. **Realtime:** `-play` of a busy fretless line with chorus within the CPU budget of
    `electric-bass` + 30 %.

### 8.7 Order

1. The continuous stop and the fingertip loss in `Guitar`; the fretless spec; the player without
   intonation; tests 3, 5, 7 and 8.
2. The board contact; tests 1, 2, 4 and 6. Listen against FluidR3 C3 and G3.
3. Landing, scoop, vibrato and harmonics; tests 10 and 11.
4. The registry and the solver; tests 12 and 13.
5. The amp preset and the chorus; the mapping; tests 9, 14 and 15.

---

## 9. Not found, or uncertain

- **Any measurement of the fretless mwah**, of fingertip termination losses, or of fretless
  intonation and slide statistics.
- Issanchou's thesis; Mandal & Wahi's full text.
- Acoustic 360 circuit values; CE-2 sweep and LFO ranges; any one maker or one maker GM 36 sound
  description.
- Jaco's delay settings in ms.
- Whether "pluck nearer the neck gives more mwah" (lore) contradicts the A/x_p argument (§2.1).
- Board-material contact parameters (K, C): no measured values.

## 10. Sources

- [Issanchou2017] C. Issanchou, S. Bilbao, J.-L. Le Carrou, C. Touzé, O. Doaré, "A modal-based
  approach to the nonlinear vibration of strings against a unilateral obstacle", JSV 393 (2017)
  229–251. https://perso.ensta.fr/~doare/files/issanchou_jsv2017.pdf
- [Issanchou2017ISMA] "String/fretboard interaction in electric basses", ISMA 2017 (abstract).
  https://perso.ensta.fr/~touze/PDF/ISMA_2017_paper_15.pdf
- [Issanchou2018AA] "String/frets contacts in the electric bass sound", Applied Acoustics 129
  (2018) (abstract). https://hal-ensta.archives-ouvertes.fr/hal-01582578
- [Issanchou2018JASA] nonsmooth contact dynamics (title only):
  https://perso.ensta-paris.fr/~touze/PDF/2018_NonSmoothString_JASA.pdf ;
  https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A24_Issanchou_NonsmoothContactDynamics.pdf
- [BilbaoTorin2014] S. Bilbao, A. Torin, "Numerical simulation of string/barrier collisions: the
  fretboard", DAFx-14. https://www.dafx14.fau.de/papers/dafx14_stefan_bilbao_numerical_simulation_of_s.pdf ;
  Bilbao, Torin, Chatziioannou, collisions: https://arxiv.org/abs/1405.2589
- [vanWalstijn2016] M. van Walstijn, J. Bridges, S. Mehes, "A real-time synthesis oriented
  tanpura model", DAFx-16. https://pureadmin.qub.ac.uk/ws/files/94023894/25_DAFx_16_paper_08_PN.pdf ;
  ISMA 2014: https://pureadmin.qub.ac.uk/ws/files/10528686/isma14a.pdf ; Vienna Talk 2015:
  https://viennatalk2015.mdw.ac.at/proceedings/ViennaTalk2015_submission_72.pdf
- [Siddiq2012] S. Siddiq, "A physical model of the nonlinear sitar string", Archives of Acoustics
  37(1) 73–79. https://journals.pan.pl//Content/101574/PDF/10_paper.pdf
- [MandalWahi] JSV 338 (2015) 42–59 (citation only); Nonlinear Dynamics 2024 (abstract):
  https://link.springer.com/10.1007/s11071-023-09046-w
- [Fleischer] dead spots: https://acoustics.org/pressroom/httpdocs/137th/fleischer.html
- [Woodhouse] https://www.ioa.org.uk/system/files/publications/J%20WOODHOUSE%20MEASURING%20THE%20PLAYING%20PROPERTIES%20OF%20VIOLINS.pdf
- [TalkBass] https://www.talkbass.com/threads/the-physics-of-mwah.1349750/ (snippets)
- Boards and setup: https://www.basschat.co.uk/topic/176229-fretless-help-epoxied-rosewood-vs-ebony ;
  https://www.talkbass.com/threads/ebony-or-epoxy.236331/ ;
  https://hub.yamaha.com/guitars/bass/guitars-a-guide-to-bass-fingerboards/ ;
  https://www.harmonycentral.com/forums/topic/1928026-setup-specs-for-a-fretless-bass/ ;
  https://basschat.co.uk/topic/80635-fretless-set-up-couple-of-questions-on-action-neck-relief
- Jaco: https://en.wikipedia.org/wiki/Jaco_Pastorius ;
  https://reverb.com/news/remembering-jaco-pastorius-a-tribute-to-his-favorite-gear ;
  https://ricksuchow.com/rebuilding-the-bass-of-doom-added-2-21 ;
  https://en.wikipedia.org/wiki/Portrait_of_Tracy
- Acoustic 360: https://www.fretboardjournal.com/columns/catch-day-circa-1969-acoustic-360361-bass-amplifier/ ;
  https://www.notreble.com/buzz/2011/07/22/acoustic-usa-launches-with-revamped-360361-bass-rig/
- Fretless history: https://en.wikipedia.org/wiki/Fretless_bass ;
  https://www.premierguitar.com/bass-bench-tracing-the-origins-of-the-fretless-electric-bass ;
  https://www.12fret.com/2013/06/10/1970-fender-fretless-precision-bass/
- Pino: https://reverb.com/news/living-in-the-pocket-a-pino-palladino-bass-retrospective ;
  https://www.guitarworld.com/artists/bassists/pino-palladino-ernie-ball-music-man-signature-stingray ;
  https://www.bonedo.de/artikel/play-alike-21-bass-edition
- Karn, Jones, Wal: https://en.wikipedia.org/wiki/Mick_Karn ;
  https://en.wikipedia.org/wiki/Percy_Jones_(musician) ;
  https://www.soundonsound.com/people/why-love-fretless-bass ;
  https://walbasshistory.blogspot.com/2013/02/interview-with-percy-jones.html
- Willis: https://www.guitar-list.com/node/3910 ;
  https://www.sweetwater.com/store/detail/GWB35BKF--ibanez-gwb35-black-flat
- Vibrato: https://www.mcg.uva.nl/mmm-2003/papers/mmm-35/mmm-35.html ;
  https://arxiv.org/html/2512.18162v1 ;
  https://experts.illinois.edu/en/publications/a-pilot-study-of-relationships-between-pitch-register-and-dynamic/
- Portamento: https://arxiv.org/html/2604.22037v1
- Ensemble intonation:
  - [DaiDixon2019] J. Dai, S. Dixon, JASA 2019:
    https://webspace.eecs.qmul.ac.uk/s.e.dixon/pub/2019/DaiDixon-JASA-Quartet-AcceptedVersion.pdf
  - [Weng2025] String intonation, Lund/RCM PhD:
    https://lup.lub.lu.se/search/files/216926330/Omslag-sammanfogad.pdf
  - Sheffield vocal quintet: https://eprints.whiterose.ac.uk/157113/
  - Papiotis/Marchini: https://repositori.upf.edu/items/a9a88ea4-3936-49d8-a208-dbd7ed19a157
  - Tavani 2021 (not read): https://drum.lib.umd.edu/bitstream/1903/27715/2/Tavani_umd_0117E_21692.pdf
  - Priester: https://scholarworks.iu.edu/dspace/items/bc92c78c-46cc-41c1-84ab-baa7af3cb162
  - Borém & Lage 2019: https://ojbr.com/10-3.asp
  - Bass vibrato: https://scite.ai/reports/in-tune-9OKvpGN
- Boss CE-2: https://support.roland.com/hc/en-us/articles/201927549-CE-2-Technical-Specifications
- [SC-88Pro OM] Roland SC-88Pro owner's manual: https://dosdays.co.uk/media/midi/SC-88PRO_OM.pdf
- XG lists: https://it.yamaha.com/files/download/other_assets/2/1342332/clp785_en_de_fr_es_pt_ja_dl_a0.pdf ;
  https://uk.yamaha.com/files/download/other_assets/0/318130/QY100E2.pdf
- Local: `/usr/share/sounds/sf2/FluidR3_GM.sf2`, `/usr/share/sounds/sf2/TimGM6mb.sf2`,
  `~/soundfonts/Jnsgm2.sf2`, TiMidity++ 2.14; the MIDI files in `~/Music/midi/bushgrafts/`,
  `~/midi`, `~/Downloads`.
- Project: `research/bass_section.md`, `research/bass_amps.md`, `research/double_bass.md`,
  `src/ebass.cc`, `src/guitar.cc`, `src/bass.cc` (board contact 769–790, finger loss 288–289),
  `src/organ.cc` 1812–1980, `src/choir.cc` 528–610, `src/ensemble.h` 25–40, `src/midifile.cc`
  1109–1393, README ~421.
