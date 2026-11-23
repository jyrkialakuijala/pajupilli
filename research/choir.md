# Choirs (GM 53 "Choir Aahs", GM 54 "Voice Oohs"): research

Status: research only (2026-11-19). Nothing here is implemented. In `src/midifile.cc` (around line
198) General MIDI program 53 (counted from 1) is `{"choir", "pad-choir"}` and 54 is
`{"voice-oohs", "synth-voice"}`. Neither "choir" nor "voice-oohs" exists yet, so both fall back.
The building block exists: `src/singer.h/.cc`, a physical singer after the sister project
`~/puhuja`. Today it is used three to a key by the synth pad `pad-choir` (`src/lead.cc`, patch
"pad-choir", GM 92). This note covers four things:
- what a choir is acoustically: the voice, the ensemble, the room;
- what the existing singer does and does not do, measured;
- what to take from puhuja;
- a concrete plan, at the end (section 11).

How each claim is marked:
- **[web, KEY]** read in the source KEY (section 13). **(snip)** after it means it was read only
  through a search snippet or a machine summary, not the text itself.
- **[calc]** my arithmetic or estimate from cited values.
- **[meas]** measured for this note on this machine, with a scratch program (no repository code
  changed).
- **[code]** read in this repository's or puhuja's source.
- **[memory]** general knowledge, not looked up: check before relying on it.

Pitch names are scientific (A4 = 440 Hz, C4 = MIDI 60). 1 cm H2O = 98 Pa.

**Short answer.**
- **The ensemble.** A choir part is a crowd of nearly identical voices, each a few cents off:
  - the F0 dispersion inside a section is 16–30 c (the bass largest) **[web, Cuesta18]**;
  - listeners prefer 0–5 c of steady scatter between the singers' mean pitches and tolerate up to
    ~14 c **[web, Tern93 (snip), Chandna20]**;
  - onsets are spread by ~30–60 ms between singers **[web, DAmario18]**;
  - vibratos run free at 5–5.5 Hz, slightly slower and narrower than the same singers' solo
    vibrato **[web, Mann14]**;
  - the singers are 3–10 dB apart in level **[web, TM17]**.

  The "chorus effect" is these scatters summed and heard through a reverberant room. Nothing has
  to be chorused: 4–12 independent singers per part give it.
- **The choral voice** has more fundamental and less singer's formant than the same singer's solo
  voice **[web, Rossing86 (snip)]**. Opera chorus professionals are the exception **[web, Reid07
  (snip)]**.
- **Sopranos above ~C5 (523 Hz)** tune the first resonance R1 to f0 (R1:f0 tuning) **[web, HSW11,
  JSW04]**. A choir model must do this, or its high soprano notes will lose 10–20 dB of the
  fundamental's support **[calc]**.
- **The existing Singer is cheap enough for a real choir.** It costs 0.67 % of a core per singer
  here **[meas]**: 24 singers (a chamber choir) take 16 % of a core, 48 take 32 %.
- **Its vowels need refitting for singing.** Measured on its own tract, Singer's /a/ has
  F1 920–1110 Hz and F2 1540–1860 Hz, an [æ]. Its /u/ has F2 1300–1600 Hz, a fronted [ʉ]
  **[meas]**. Sung [ɑ] and [u] lie at R1/R2 ≈ 700–830 / 1160–1340 Hz and
  ≈ 300–400 / 700–1100 Hz **[web, HSW11]**, **[memory]**.
- **Plan:**
  - "choir" = 4 parts × 6 singers (S A T B, divisi as in `stringsect.cc`), per-singer fixed tract
    length and formant scatter, pitch scatter, vibrato and timing, in the church or the hall;
  - "voice-oohs" = the same choir on [u]/[ʊ] with soft "d"-less onsets, plus a hum variant that
    needs a nasal branch (puhuja's).

---

## 1. The existing singer, and what it is used for

### 1.1 Singer (src/singer.h, src/singer.cc) [code]

- **Tract:** a 1-D FDTD transmission line, 44 sections, 3 substeps per 48 kHz sample (staggered
  leapfrog). Losses are a constant 260 /s plus a viscous term 0.035/A; there are no wall
  impedances. The lips radiate through Flanagan's piston load (R ∥ L); the output is the lips'
  dU/dt.
- **Shapes:** puhuja's parametric area function. The vowel axis is 0 u, 1 o, 2 a, 3 e, 4 i, using
  puhuja's older fitted rows (`kVowels[5]`, lengths 17.4–17.5 cm). The tract moves between shapes
  in log area with τ = 50 ms, and its length changes with τ = 150 ms.
- **Length:** set by the *note*, not by a voice part: 18.0 cm at MIDI 45 falling linearly to
  14.5 cm at MIDI 76, × (1 ± 0.03) per singer.
- **Glottis:** a prescribed area pulse (raised cosine opening, cosine closing) at f0. Its open
  quotient falls from 0.68 to 0.50 with loudness and its closing quotient from 0.42 to 0.25. A
  posterior chink of 0.015 cm² + 0.09 × breath cm². The flow obeys
  Lg dU/dt = Δp − ρU|U|/(2Cd²a²), Cd = 0.92, solved implicitly against the first tract section,
  so the tract loads the source: there is source–tract interaction, but no fold mechanics.
- **Turbulence:** a pressure noise ∝ (Re² − Rc²)/Rc², Rc = 600, band ~300 Hz–6 kHz. A
  constriction jet (for consonants) also exists.
- **Variation per singer:**
  - pitch offset uniform ±6 c (singer 0 has none);
  - drift: a random walk of 3 c·√s relaxing in 1.5 s;
  - jitter uniform ±0.3 % per period, shimmer ±3 % per period;
  - vibrato rate × (1 ± 0.08), random phase; depth and rate are common.
- **Lung pressure:** `lead.cc` sends 0–1100 Pa (≈ 0–11 cm H2O) from the envelope × level, with a
  5 ms follower in the singer.

### 1.2 pad-choir [code]

Three singers per key, `ampA` 0.25 s, release × 2.5, keys 41–84. The brightness knob is the vowel
(u .. i) and the air knob is breathiness. It is a pad: every key gets the same three singers,
whatever its register. Singer 0 has no pitch offset; with three singers the ensemble sounds like a
trio, not a section.

### 1.3 What the Singer's tract actually sounds: formants measured [meas]

Method: a scratch program called `Singer::control` with f0 = 0.01 Hz (the folds still: closed but
for the chink) and no lung pressure. It then kicked the first section and read the formant peaks
off the FFT of the radiated response, divided by f to undo the radiation's +6 dB/oct. MIDI notes
43/55/67/79 set the length (18.0 → 14.5 cm).

| vowel | note 43 (≈18 cm) | 55 | 67 | 79 (≈14.5 cm) |
|---|---|---|---|---|
| u | 333 / 1315 / 2052 | 353 / 1401 / 2182 | 380 / 1521 / 2363 | 403 / 1625 / 2520 |
| o | 665 / 1083 / 2139 | 701 / 1147 / 2277 | 751 / 1236 / 2468 | 793 / 1313 / 2635 |
| a | 920 / 1537 / 2469 | 974 / 1628 / 2625 | 1048 / 1754 / 2838 | 1111 / 1863 / 3025 |

(F1 / F2 / F3 in Hz.)

Against sung targets (sections 2.1, 2.2):
- **/a/ is too open and too front for a choir "aah".** F1 920–1110 against R1 700–830, and F2
  1540–1860 against 1160–1340: it sounds like [æ]/[a], the Finnish a it was fitted to. The choral
  "aah" is a dark [ɑ], close to puhuja's newer ɑ row (`LINE_V14_*`, targets 722/1338/2461) **[code]**.
- **/u/ has F2 ~400–600 Hz too high:** [ʉ], not [u].
- **/o/ has F1 ~150–200 Hz too high:** nearer [ɔ] than [o].
- **The length scaling (18.0/14.5 = 1.24) is right in size** for male against female formants
  (puhuja's female ×1.12/1.15/1.06 for F1/F2/F3, sections 2.1 and 9). It is wrong in kind: a
  soprano singing G3 keeps a soprano's tract, and a bass singing D4 keeps a bass's.

The peaks above are the closed-glottis tract. With the glottis open part of each period, F1 rises
and broadens a little more **[memory]**.

---

## 2. The choral voice

### 2.1 Formants of the sung vowels, per voice type

Henrich, Smith & Wolfe measured R1 and R2 by broadband excitation at the lips. The singers were
22 classically trained sopranos, altos, tenors and baritones, singing four Australian-English
vowels ("hard" [ɐː], "heard" [ɜː], "hoard" [oː], "who'd" [ʉː]) over their range **[web, HSW11]**.
Their speech values (Hz, mean ± sd):

| | hard R1 | hard R2 | hoard R1 | hoard R2 | who'd R1 | who'd R2 |
|---|---|---|---|---|---|---|
| soprano | 825 ± 120 | 1340 ± 115 | 585 ± 45 | 1065 ± 90 | 410 ± 80 | 1340 ± 475 |
| alto | 830 ± 95 | 1160 ± 60 | 515 ± 40 | 825 ± 85 | 350 ± 30 | 1510 ± 280 |
| tenor | 700 ± 85 | 1175 ± 85 | 545 ± 100 | 900 ± 115 | 350 ± 30 | 1145 ± 245 |
| baritone | 785 ± 120 | 1235 ± 50 | 495 ± 85 | 880 ± 175 | 385 ± 85 | 1205 ± 335 |

- **The parts barely differ.** Voice classification did not correlate with R1 or R2 in speech
  (tenors against baritones: −25 ± 50 Hz in R1, 15 ± 90 Hz in R2 when singing) **[web, HSW11]**.
  The female–male difference in a choir is mostly at the *pitch* end (resonance tuning), less a
  constant formant scale.
- **Australian "who'd" is fronted (R2 1145–1510 Hz).** A choral [u] (German/Latin "u") is back:
  for men about 300/750–900 Hz, for women 350–400/800–1000 Hz **[memory]**. Choirs are trained to
  "round and darken" it.
- **Spread between singers at the same pitch:** in singing, the sd of R1 is 105/90/75/65 Hz for
  male a/er/o/u, and of R2 115/95/120/195 Hz **[web, HSW11 Table IV]**. For women with f0 above
  R1, the sd of R1 shrinks to 25–40 Hz (they all tune to f0). These numbers set the per-singer
  formant scatter in the model: about ±8–12 % on R1 and ±5–10 % on R2 at low pitch **[calc]**.
- **Repeatability within one singer:** R1 sd 5.5–6.8 %, R2 4.6–5.0 % below B4. Above B4 the R1 sd
  for women halves to 3.4 % **[web, HSW11 Table III]**. So each singer's vowel can also wander by
  ~3–5 % from note to note.

### 2.2 Resonance tuning at high pitch (what sopranos do)

- **R1:f0 tuning (the soprano strategy).** Sopranos raise R1 to approximately f0 over
  500 < f0 < 1000 Hz (B4/C5 to C6) **[web, JSW04, HSW11]**. Mainly by opening the jaw and mouth
  **[web, Garnier10 (snip)]**. JSW04: R1 averaged ~420 Hz around C4; once f0 passed the speech
  value of R1, R1 followed f0. The tuning line R1 = f0 lay within the data's sd from about 600 to
  1000 Hz. In "who'd" R1 could not follow up to 1 kHz: lips rounded **[web, JSW04]**. One soprano
  tuned R1 ~10 Hz *above* f0 and R2 ~11 Hz above 2f0 for almost all notes **[web, HSW11]**.
- **Altos** use R1:f0 for the vowels whose speech R1 is low, and may switch to R1:2f0 low in
  their range **[web, HSW11]**.
- **Tenors and baritones** mostly use R1:2f0 and R1:3f0 over part of their range, and various
  tunings to higher harmonics at low pitch. Individual strategies differ a lot **[web, HSW11]**.
- **Why it matters:** above ~C5 the harmonics are 500+ Hz apart. An untuned /u/ (R1 ~400 Hz) at
  f0 = 700 Hz leaves the fundamental well above R1 and above all resonances. The level of H1 then
  falls by roughly 12 dB/oct above R1 (one pole's skirt) relative to a tuned tract: 6–15 dB less
  output and a thin, "hooty" sound **[calc]**.
- **The vowel cost:** at f0 = 880 Hz every vowel has R1 ≈ 880 Hz. The vowels converge (intelligibility
  of soprano vowels above ~C5 is poor) **[memory]**. A GM "aah" sung by sopranos on A5 is
  therefore an "ah"-like open vowel whatever was asked: fine for this instrument.

### 2.3 The singer's formant, and its absence in choirs

- **The singer's formant** is a clustering of F3, F4, F5 near 2.5–3 kHz in male (and alto)
  classical singing. Sundberg proposes calling it present when the level L3 near 3 kHz is ≥ 6 dB
  above what F1 and F2 predict; a trained singer had ~4 dB **[web, Sund95]**. A 10 dB rise in
  overall level raised L3 by 15 dB in one singer (Gauffin & Sundberg 1989): the 3 kHz region grows
  1.5 dB per dB **[web, Sund95]**.
- **Choral mode:**
  - LTAS of singers in choral mode against solo mode: more energy at the fundamental, less in the
    singer's formant region **[web, Rossing86 (snip), TM17]**;
  - singers matched their neighbours' level in choral mode **[web, TM17]**;
  - in Reid et al. (2007), professional opera chorus members sang with equal or more singer's
    formant in chorus than solo, and no difference in vibrato **[web, Reid07 (snip)]**.

  So an amateur or church choir has little or no singer's formant, and an opera chorus has some.
- **Model:** the default choir (a chamber/church choir) without a singer's formant: L3 at what
  F1–F2 predict (ΔL ≈ 0 dB). An "opera chorus" variant with +4–6 dB for the men and altos
  **[calc]**. In a tube model the singer's formant comes from a narrow epilarynx tube
  (area ratio to the pharynx ≲ 1/6) **[memory]**. puhuja has this as `lHypo/aHypo` and
  `aLarynx` **[code]**. Setting `aLarynx` 0.3–0.5 cm² against `aPharynx` ≥ 3 cm² is the knob.

### 2.4 Spectral slope and breathiness

- **Slope:**
  - the voice source falls at ~−12 dB/oct for normal phonation (the model value in [web, Sund95]);
  - from pp to ff the slope flattens: +15 dB at 3 kHz for +10 dB overall **[web, Sund95]**;
  - puhuja's measured speech LTAS tilt was −12.3 dB/oct against −9.4 for the commercial
    references, which it fixed with a +8 dB shelf above 1875 Hz on the source **[code]**.

  Choral singing is mf on average. The model's LTAS should fall ~−10 to −12 dB/oct above
  ~600 Hz at mf **[calc]**.
- **Breathiness:** choral mode favours a stronger fundamental (H1). That suggests a larger open
  quotient and a softer closure than solo mode **[web, Rossing86 (snip)]**, **[calc]**. Female
  choristers in particular keep a posterior chink: aspiration noise sits ~20–30 dB under the
  harmonics at 2–5 kHz **[memory]**. Default "breath" 0.3–0.4 for women and 0.15–0.25 for men on
  Singer's scale **[calc]**.

### 2.5 Vowel modification in choirs

Choral directors unify vowels: a taller, rounder, darker [ɑ] for "ah" (lower F2), [u] lip-rounded,
and few diphthongs **[memory]**. Acoustically:
- F2 of /a/ falls toward 1100–1300 Hz;
- the spread between singers shrinks (that is what "blend" means acoustically);
- at the top of each part the vowel opens: R1 rises to f0 or 2f0 (section 2.2).

### 2.6 Vibrato

| measure | solo | choral | source |
|---|---|---|---|
| rate (30 female undergraduates, same excerpts) | 5.41 Hz | 5.13 Hz | [web, Mann14] |
| extent (cents, peak-to-peak as reported) | 226 ± 63 | 203 ± 51 | [web, Mann14] |
| share of the note with vibrato | 78 % | 62 % | [web, Mann14] |
| opera chorus, chorus vs solo mode | no difference in rate or extent | | [web, Reid07 (snip)] |
| famous sopranos' extent | ±34 to ±123 c, mean ±71 c | | cited in [web, Mann14] |

- **Rates:** classic vibrato is 5–7 Hz with an extent of about ±1 semitone (Seashore). Slower
  rates (~4 Hz) are heard as a wobble; opera singers reach ~8 Hz **[web, AcostaDaffern]**. The
  vibrato found in a unison choir dataset was near 5 Hz, most often in the slowest piece
  **[web, Cuesta18 (snip)]**.
- **Synchrony:** in a quartet, singers adjusted their vibrato for blend, with occasional
  synchrony between parts. The soprano used more vibrato than the bass **[web, Daffern16 (snip),
  AcostaDaffern]**. Within a section of many singers the vibratos are effectively independent:
  random phases, rates spread by a few tenths of a hertz **[calc]**. Their sum is not a vibrato
  but a smear (section 3.3).
- **Traditions:**
  - English cathedral/early-music choirs and Scandinavian chamber choirs: near straight tone, with
    extents ±10–30 c and vibrato in under half of the note **[memory]**;
  - romantic/symphonic choruses and opera choruses: ±40–80 c at 5–6 Hz **[memory]**.

  [Mann14] counts vibrato over 62 % of each note in choral mode.
- **Onset:** a singer's vibrato starts 0.2–0.5 s into a note, after the pitch has settled
  **[memory]**.
- **Model defaults:**
  - "choir": extent ±35 c (sd ±12 c between singers), rate 5.3 Hz (sd 0.35 Hz), onset delay
    0.3 s ± 0.1 s;
  - basses at 0.7 × the extent, sopranos at 1.2 × **[calc from the above]**;
  - a "straight" variant at ±12 c.

---

## 3. The choir effect: many voices on one part

### 3.1 How many singers

- **Chamber choir:** 16–32 singers, 4–8 a part **[memory]**. Ternström's SOR study choir had 25
  **[web, Fischinger15]**.
- **Church/community choir:** 20–40 **[memory]**.
- **Symphonic chorus:** 60–150, 15–40 a part **[memory]**. Titze & Maxfield model sections of 16
  and 128 singers. The Mormon Tabernacle Choir (360) is ~9 dB louder than a 45-voice choir
  **[web, TM17]**.
- **tape-replay keyboard "8 Choir":** 4 men + 4 women, recorded in two adjacent studios **[web, Wiki-Mtron]**
  (section 6.3).
- **Level:** +3 dB per doubling of singers (incoherent sum) **[web, TM17]**.

### 3.2 Pitch scatter

- **F0 dispersion** within a part (the sd of the singers' F0s in short windows, vibrato included):
  16–30 c, largest in the basses **[web, Cuesta18 (snip)]**. The same dataset (16 singers, 4 a
  part, semi-professional SATB) gave inter-singer deviations of 0–50 c, mean ~20 c
  **[web, Chandna20]**. Jers & Ternström's multitrack recordings found 25–30 c
  **[web, Cuesta18 (snip)]**. Ternström's overview reports dispersion "2 to 60 cents, typically
  20–30" **[web, Tern02 (snip)]**.
- **Pitch scatter,** the sd between singers' *mean* F0 per tone (not instantaneous): listeners
  preferred 0–5 c and tolerated 5–14 c before calling it out of tune **[web, Tern93 (snip),
  Chandna20]**. The dispersion (20–30 c) is mostly vibrato and flutter. The constant offsets are
  smaller.
- **Intonation correlation** between singers of a part: r ≈ 0.32 on average **[web, Cuesta18
  (snip)]**. They follow each other only loosely.
- **Reaction:** choir singers respond to an external pitch change after ~206–227 ms
  **[web, Fischinger15]**.
- **Model:** per singer, a fixed offset with sd 6 c (truncated at ±15 c). A slow drift with
  sd 4 c and τ ≈ 1.5 s (Singer has 3 c·√s with 1.5 s relaxation: keep it). Per note, a fresh
  offset with sd 4 c. Vibrato as in 2.6. Instantaneous dispersion then comes to
  √(6² + 4² + 4² + (35/√2)²) ≈ 27 c, matching [Cuesta18] **[calc]**.

### 3.3 What the "chorus effect" is

N voices at slightly different F0 make, at harmonic k, N components spread over k·Δf.
- Two voices 10 c apart at 220 Hz beat at 1.27 Hz in the fundamental and at 12.7 Hz in the
  10th harmonic (2.2 kHz) **[calc]**. The upper harmonics flutter fast and the low ones swell
  slowly.
- With vibratos (±35 c at 5 Hz, random phases) the instantaneous spread at harmonic k is ~k × 2 %
  of f0. Above ~10 harmonics the partials of different singers overlap into a band: the choir's
  upper spectrum is noise-like, its lower spectrum harmonic **[calc]**. Ternström calls this pitch
  dispersion a "bandwidth" of the partials. It relates to "flutter", F0 variations too fast to be
  heard as pitch changes **[web, Chandna20 on Tern91]**.
- The formant scatter (each singer's vowel ±5–10 %) makes the summed envelope smoother, with wider
  formant peaks. The singers' timbres do not fuse into one larger voice **[calc]**.
- The room adds its own decorrelation: every singer stands elsewhere, so each reaches each ear by
  other paths. Ternström found the room "had a large effect on the shape of the LTAS", and choirs
  adapt their level to the room **[web, Fischinger15 on Tern93]**.
- **Consequence for a model:** the effect needs ≥ 4 truly independent voices per part. Each needs
  its own pitch process, vibrato, formants and position. A delay-line chorus on fewer voices gives
  a periodic, comb-filtered imitation **[calc]**.

### 3.4 Timing scatter

- **Duos:** absolute onset asynchrony of 59 ± 11 ms mean over all notes. At note onsets it was
  52 ms with visual contact and 83 ms without **[web, DAmario18]**.
- **Small ensembles in general:** 30–50 ms **[web, DAmario18b]**. String quartets: sd 24–28 ms at
  157 bpm **[web, DAmario18 citing Wing14]**.
- **Unison choir (4 a part):** all singers' voiced/unvoiced transitions together span ~0.1 s on
  average (0.124 ± 0.021 s for the basses) **[web, Chandna20]**. Note that this includes
  consonants. Chandna et al. found synthetic timing scatter above 40 ms (sd) degraded quality in
  their system.
- **Model:**
  - each singer's onset delay after the key: 25 ms + N(0, 20 ms), truncated at 0–80 ms;
  - each singer also has a habit of ±10 ms;
  - each part has its own common offset per note change, N(0, 8 ms);
  - releases are scattered 1.5× as much as onsets (singers end by breath, not together).

  This follows `stringsect.cc`'s 20 ± 15 ms **[code]**.

### 3.5 Level scatter

- **Individual singers:**
  - choral singers' individual dynamic ranges span 11–33 dB **[web, TM17 citing Coleman94]**;
  - one gifted singer in five adds 5–15 dB to loud notes **[web, TM17]**;
  - non-homogeneous sections widen the section's dynamic range by 4–9 dB **[web, TM17]**.
- **Whole choirs, pp to ff:** 12 dB (boys), 16 dB (youth), 20 dB (adult) **[web, TM17 citing
  Tern93]**. Professional soloists have 30–40 dB **[web, TM17]**.
- **Self-to-other ratio:** preferred values range −1 to +15 dB between singers, each held within
  ±2 dB. Live, ~+4 dB in a 25-voice chamber choir; ~+6 dB in the lab **[web, TM17,
  Fischinger15]**.
- **Model:** per singer, a fixed level offset of sd 2.5 dB (the lung pressure × 10^(±2.5/20·…)) and
  a "strong voice" flag on 1 in 6 singers: +4 dB at f and ff only **[calc]**.

---

## 4. Ranges, tessitura and the keyboard split

### 4.1 Ranges

| part | choral range | comfortable tessitura | Henrich's singers (comfortable, [web, HSW11]) |
|---|---|---|---|
| soprano | C4–A5 (B♭5, C6 for strong choirs) | E4–E5 | G3/B3/C4 – A5…D6 (one to G6) |
| alto | F3/G3–D5 (E5) | A3–C5 | D3…G3 – C5…G5 |
| tenor | C3–G4 (A4, B♭4) | E3–F4 | F2…C3 – G4…D5 |
| bass (baritone) | E2/F2–D4 (E4) | A2–C4 | F2…A2 – D4…A4 (baritones) |
| basso profondo (Russian) | down to C2, A1, G1 | | [memory] |

(Choral ranges and tessituras **[memory]**: standard choral-writing figures. Henrich's "comfortable
range" is for trained soloists.)

- **The voice parts overlap by about an octave** **[web, UQ-SATB (snip)]**.
- **Unauditioned choirs** should avoid the top two notes of each part's high tessitura,
  particularly sopranos and tenors **[web, Hutchings (snip)]**.
- **tape-replay keyboard:** its keyboard is G2–F5 (35 keys). The men dropped the highest notes an octave
  because they were out of range **[web, Wiki-Mtron]**.

### 4.2 How GM keys should be shared (a proposal, following stringsect.cc)

`stringsect.cc` gives each section a key range, a maximum divisi and a preference (highest,
lowest, middle) **[code]**. The choir follows the same pattern:

| part | singers | keys (sounding, MIDI) | max divisi | prefers |
|---|---|---|---|---|
| soprano | 6 | 60–84 (C4–C6) | 3 | highest |
| alto | 6 | 53–77 (F3–F5) | 3 | middle |
| tenor | 6 | 48–69 (C3–A4) | 3 | middle |
| bass | 6 | 36–62 (C2–D4) | 3 | lowest |

- **One key alone:** the parts whose ranges contain it sing it in unison (C4: S, A, T, B all
  can). A GM melody played on Choir Aahs is expected to sound like a choir in unison. Mixed
  unisons should be limited to the parts within their *comfortable* tessitura at that key, plus
  the part that would sing it if alone **[calc]**.
  - **Unison by register:** below E3 basses only; E3–B3 tenors and basses; C4–F4 altos and tenors
    (sopranos too if the key is held alone); above C5 sopranos and altos.
  - **Example:** A4 alone goes to sopranos and altos; tenors join only in a "full" mode.
- **Chords:** spread as in `stringsect.cc`, bass on the lowest key, soprano on the highest,
  altos and tenors on the middle keys. A part divides when it has more keys than the parts below
  it can take.
- **Notes outside every range:**
  - below C2 the basses sing an octave higher. Real choirs never go below ~A1 (Russian
    oktavists), and a GM file asking for C1 means a low effect, not a pitch;
  - above C6 the sopranos sing an octave lower, or the top part whistles; GM samples simply
    transpose their top multisample **[memory]**;
  - near the edges the parts thin out (fewer singers can reach) and the vowel opens (R1:f0).

  Keep the singers who *can* reach: of 6 sopranos, 2 sing B♭5–C6 and the rest drop out (a section
  thinning at its top is part of the sound) **[calc]**.
- **Voice-specific tract length,** not note-specific:
  - soprano 14.5 cm, alto 15.5, tenor 16.8, bass 17.8 (puhuja male 17.5);
  - each × (1 ± 0.04), from the spread of adult vocal tract lengths, sd ~5 % **[memory]**;
  - the female/male formant ratio ~1.12–1.18 then follows from the lengths **[calc]**.

---

## 5. Onsets, releases, legato and dynamics

### 5.1 Onsets of a consonant-less "ah"

- **Three onset types** **[web, DeJonckere25 (snip)]**: soft (coordinated, "simultaneous"), hard
  (glottal: the folds closed first, then released into voicing) and breathy (aspirated: airflow
  before the folds close).
- **The physics of the start:** fold oscillation grows gradually before the first contact at the
  midline. Onset latency (adduction to sound) is shortest for modal voice and longest for pressed
  voice **[web, DeJonckere25 (snip)]**.
- **Times (my estimates, [memory]):**
  - glottal onset: full amplitude in 10–30 ms, a click-like first period;
  - soft onset: 50–150 ms;
  - breathy onset: 30–100 ms of aspiration noise, then voicing growing over 50–100 ms.

  Choirs are trained to the soft onset for "ah"; untrained or emphatic entries use the glottal
  onset.
- **Pitch at the onset:** amateur singers start up to 50–100 c flat and scoop up over 100–200 ms.
  Trained singers start within ~20 c and settle in < 100 ms **[memory]**.
- **Model:**
  - per singer, a lung-pressure rise with τ 30–80 ms (velocity-dependent: soft keys slow);
  - aspiration first for a breathy onset: the chink opened (breath 0.6 → 0.3 over 60 ms);
  - a start pitch of −15 c ± 15 c gliding in with τ 60 ms;
  - vibrato starting after 0.3 s;
  - the singer-to-singer onset scatter from 3.4.

  For "Choir Aahs" a moderate attack (the ensemble reaches full level in 150–300 ms); for "Voice
  Oohs" a softer one (250–400 ms) **[calc]**. (Built so, τ 0.19 s, it measured an attack of 774 ms
  against the SoundFonts' 40–80 (research/gm_compare.md, 54 Voice Oohs) and dragged in rhythmic
  parts; it is now τ ≈ 75 ms at mf, a soft onset's 50–150 ms, full in ~0.2 s.) one maker's Voice Oohs is described as a "dooh" set
  **[web, SynthMania]**, so a short soft /d/-like start can be imitated by a constriction release
  (Singer has `setConstriction`) **[code]**.

### 5.2 Legato and releases

- **Legato:** singers move between notes in ~50–150 ms, often with a slight overshoot or
  portamento on large leaps **[memory]**. Each singer of a part slurs at its own moment (3.4).
  Between parts the timings are independent.
- **Release:** the pressure falls over 50–150 ms. The folds abduct and the last ~30–80 ms are
  breathier, often with an audible exhalation (aspiration at the open glottis) **[memory]**. In a
  reverberant church the release is mostly heard as the room's tail.
- **Breathing:** singers renew breath every 5–15 s; in a choir they stagger it ("chorus
  breathing") so long notes never stop as a whole **[memory]**. A GM pad note held 30 s should
  have each singer drop out for 0.5–1 s at its own time, every 8–15 s, unless velocity is low
  **[calc]**. This is a real-choir cue that sample loops lack.

### 5.3 Dynamics: pressure, level, tilt, formant clarity

- **Subglottal pressure:**
  - it is the main control of loudness, and it must also rise with pitch **[web, Sund92]**;
  - doubling it raises SPL by ~9 dB **[web, Sund92 citing Fant82, Titze89]**, or 8–9 dB per
    doubling of the excess over the threshold pressure in neutral phonation
    **[web, STS91 (snip)]**;
  - +1 cm H2O raises f0 by ~4 Hz if the singer does not compensate **[web, Sund92]**: singers do
    compensate.
- **Values [memory]:** classical male singers use ~5–10 cm H2O for pp at mid pitch and 20–40 cm
  H2O for ff high notes. Sopranos use ~10 cm H2O low-soft up to 40–60 cm H2O high-loud. The
  threshold pressure is ~3–5 cm H2O, rising with f0.
  - Singer's 1.1 kPa ceiling (11 cm H2O) is a *speech* mf.
  - A choir instrument should map velocity/CC to 0.5–4 kPa for men, scaled up with pitch
    (×~1.5 per octave above the part's middle) **[calc]**.
- **Tilt:** from pp to ff the source spectrum flattens. 3 kHz grows 1.5 dB per dB of overall
  level **[web, Sund95]**, and the open quotient falls (Singer: 0.68 → 0.50, closing quotient
  0.42 → 0.25) **[code]**. So the 20 dB adult choir range (3.5) appears as ~30 dB at 3 kHz and
  ~15 dB at the fundamental **[calc]**.
- **Formant clarity:** at pp the formant peaks are weaker relative to the noise floor and the
  higher harmonics. Breathiness (the chink) also widens bandwidths: the vowel is less distinct.
  At ff the peaks sharpen and the higher formants emerge **[memory]**. The singer model gets this
  from physics if the chink and the pulse shape follow the pressure.
- **Scale of the dynamics:**
  - a choir's levels: modelled six levels pp–ff, 3–6 dB apart, over two octaves **[web, TM17]**;
  - crescendos average 13 dB, decrescendos 12 dB **[web, TM17 citing Geringer92]**;
  - male singers' average range > 25 dB, female > 20 dB, overall 70–113 dB (at 30 cm)
    **[web, TM17]**.

---

## 6. "Oohs", hums, and what GM files expect

### 6.1 Ooh against aah

- **[u] has a low F1 (300–400 Hz) and a low F2 (700–1000 Hz), with rounded, protruded lips.** In
  the ideal-formant picture L3 falls with F2, so the level near 3 kHz is "much lower" in [u] than
  in [i]. Relative L3 runs from about −45 dB in [u] to +20 dB in [i] (Sundberg's model with a
  −12 dB/oct source) **[web, Sund95]**. An "ooh" choir therefore has almost no energy above
  ~1.5 kHz: dark, soft, flute-like.
- **It is also quieter:** the radiation load of a small lip opening is smaller, and F1 at
  300–400 Hz supports only the lowest harmonics. Expect −5 to −10 dB against "aah" at the same
  pressure **[calc]**.
- **At high pitch** (sopranos above ~D5) an [u] with lips rounded cannot raise R1 to f0 near 1 kHz
  **[web, JSW04]**. Real sopranos open toward [o]/[ɔ]. The model should open the lips a little
  above ~E5 (the vowel axis toward o) **[calc]**.

### 6.2 The hum ("mmm"; GM2 "Humming")

- **Production:** lips closed, velum lowered. The sound leaves through the nose (nasal murmur) and,
  weakly, through the cheeks and walls (the "voice bar").
- **Spectrum:** puhuja's cascade used a nasal pole at 270 Hz and zeros near 1000 Hz for m, 2200
  for n and 2500 for ŋ. With full zero depth, m and n became "a dull low hum that sounds like a
  buzz" **[code, puhuja.cc kNasalPoleHz]**. puhuja's line nasal tract: 40 sections, 11 cm,
  nostrils 1.54 cm². The n came out level with the vowel at 1.2 cm²; the human readers had their
  n 9–14 dB under the vowel **[code, line.cc]**.
- **Model expectation:** a hum 10–15 dB under "aah", most energy below 500 Hz, little above
  1.5 kHz **[calc from the above]**.
- **What it needs:** the Singer has no nasal branch and no wall radiation. With the lips closed it
  radiates nothing (the R ∥ L load fades to a rigid end), so a hum needs a nasal branch
  (section 9).

### 6.3 What sampled choirs contain (what a GM file expects)

- **one maker Sound Canvas (the GM reference):**
  - 53 "Choir Aahs" is a "classic early-'90s sampled choir"; 54 "Voice Oohs" is "the ubiquitous
    one maker 'dooh' sample set" **[web, SynthMania]**;
  - GS also has "Choir Aahs 2" (variation 32 or 41) **[web, SynthMania (snip)]**;
  - GM2 adds "Humming" as a variation of 54 **[memory]**.
- **Typical content [memory]:**
  - a mixed (or, for "Oohs", female/boy) choir of ~8–20 voices on one vowel;
  - multisampled every few semitones, so men on low keys, women on high keys (the timbre changes
    at a split);
  - looped, slow attack (100–300 ms), long release (0.5–1 s with sample reverb included);
  - no consonants and no vibrato modulation by the synth (only the recorded vibrato);
  - a soft "d" or breath at the start of the oohs.

  GM files use Choir Aahs as a **pad**: long chords, often held for bars, mixed under strings,
  sometimes doubled with String Ensemble or Synth Voice.
- **tape-replay keyboard choirs:**
  - the "8 Choir" tapes: 4 men + 4 women recorded together in two adjacent studios. The men from
    the Ted Taylor Choir dropped their highest notes an octave **[web, Wiki-Mtron]**;
  - all tape-replay keyboard tapes play ≤ 8 s **[web, Hollowsun]**, with tape wow and flutter making each
    note differ **[web, Wiki-Mtron]**;
  - its character **[memory]**: a breathy, sustained "aah" with a slight constant pitch wander, a
    strong mid-range and little bass. It is heard on 1970s progressive rock and on Orchestral
    Manoeuvres in the Dark's *Architecture & Morality* (1981).

  A "tape-choir" variant (8 singers, 4+4, the men an octave down on their top notes, an
  8-s tape envelope, ±0.2 % wow at 0.5–1 Hz) is cheap once the choir exists **[calc]**.
- **Meeting the GM expectation:**
  - the choir must sound immediately as a pad: no consonant, no "words";
  - a fairly slow but not mushy attack;
  - level balanced against strings (the instruments' level tables `src/levels_data.h` and
    `velgain_data.h` handle that **[code]**);
  - for "Voice Oohs", softer, darker, more female.

  Mixed choirs are expected on 53. 54 may lean female/children. Keep men in, below ~C4.

---

## 7. The room and the stage

- **Rooms:**
  - churches: RT 2.5–6 s (stone churches 4–5 s, cathedrals to ~10 s);
  - concert halls: 1.8–2.2 s **[memory]**;
  - Fischinger et al. simulated the Concertgebouw at 1.77 s and a Spanish cathedral at 4.79 s for
    a choir **[web, Fischinger15]**;
  - the project has "studio" 0.5 s, "hall" 1.8 s (2500 m³), "concert" 2.0 s (18000 m³) and
    "church" 4.0 s (15000 m³) **[code, src/room.cc]**.

  Default for "choir": the church when nothing else sets the room (as `midifile.cc` does for the
  organ), else the file's room **[calc]**.
- **Reflections:**
  - singers prefer early reflections at 17–35 ms, with high-frequency content
    **[web, TM17 citing Marshall78]**;
  - ensemble singers prefer strong early reflections; the reverberant sound matters more when the
    nearest reflector is beyond ~7 m **[web, Fischinger15 citing Marshall85]**.
- **Stage formation [memory]:**
  - a choir stands on 3–4 risers, rows ~0.9–1.0 m apart, each row ~0.2–0.3 m higher;
  - lateral spacing ~0.5–0.7 m; Daugherty: ~60 % of singers preferred ~24 in. (0.6 m) of space
    around them **[web, TM17]**;
  - 24 singers in 3 rows of 8 → 4–5 m wide, 2–3 m deep; 100 singers in 4 rows of 25 → 15 m wide.
  - Sectional formations: S A T B left to right (sopranos audience-left), or S T B A,
    or men in the middle back rows. Mixed (quartet) formation also exists.

  Default: sectional, S A from left to right in front, T B behind (a common church layout)
  **[calc]**.
- **Singer directivity:** 15 trained singers in an anechoic room, 0° against 180°
  **[web, Monson12]**:

  | band | 125 Hz | 250 | 500 | 1 k | 2 k | 4 k | 8 k |
  |---|---|---|---|---|---|---|---|
  | singing, front − back (dB) | 4.4 | 6.0 | 6.3 | 6.0 | 13.4 | 18.6 | 26.8 |

  - The directivity index is 2.1 dB for both singing and speech (male 1.9, female 2.3), and
    rises with loudness. HF energy is down ~3 dB at 45° **[web, Monson12]**.
  - The singer's formant region is more directional than the lows **[web, Cabrera11 (snip)]**.
  - Singers on the back rows are partly shadowed by the front row's heads above ~1 kHz
    **[memory]**.

  Model: give each singer a frontal source with this directivity (a back-radiated share falling
  above 1 kHz), as the room's per-source paths allow.

---

## 8. Computational cost

### 8.1 Measured [meas]

- **Singer:** a scratch benchmark of 16 Singers, each with `control()` every 16 samples, over
  10 s, built with the Makefile's flags (`-O3 -march=native -mprefer-vector-width=512
  -ffast-math`) on the Ryzen 9 9950X3D: **0.67–0.68 % of one core per singer** (≈ 140 ns a
  sample).
- **pad-choir, whole instrument:** `./pajupilli -inst pad-choir -note 57 -secs 30` took 0.62–0.67
  s user for 30 s (3 singers plus the lead framework and pitch analysis), about 2.2 % of a core.
- **puhuja, whole synthesizer:** `./puhuja -en` on a 4.8 s sentence took 0.44 s, 9 % of a core
  for one voice (64-section oral line, 40-section nasal, 32-section trachea, 4 substeps, string
  folds, walls, planner).

### 8.2 What a choir costs

| choir | singers | cost (cores) | [calc] |
|---|---|---|---|
| tape-replay keyboard 4 + 4 | 8 | 0.054 | |
| chamber, 4 a part | 16 | 0.11 | |
| chamber, 6 a part (proposed) | 24 | 0.16 | |
| large, 12 a part | 48 | 0.32 | |
| symphonic, 24 a part | 96 | 0.64 (spread over threads, `src/parallel.h`) | |

The room comes on top. For comparison, `stringsect.cc`'s 24 bowed players run "about real time on
a 16-core machine" **[code, README]**. A singer is ~40× cheaper than a bowed viola (28 % of a core
each) **[calc]**. The choir needs no shortcut at chamber size. All singers run all the time, as
in the string section (a singer not singing costs the same if it is not skipped). A silent singer
(lungs at 0 and no flow) can be skipped after its tail has decayed (> 100 ms below −90 dB).

### 8.3 Cheaper alternatives, and what each loses

1. **Fewer singers + decorrelation** (e.g. 2 per part + all-pass/delay chorus):
   - costs 1/3;
   - loses the independent vibratos, the formant scatter and the spatial spread;
   - the chorus is periodic and comb-filtered. A GM sample of a real choir already contains its
     choir, but a synthetic one cannot be faked with 2 voices (3.3).
2. **Shared tracts** (one tract per part per key, driven by the sum of 6 glottal sources):
   - costs ~1/4 to 1/3. The sources are cheap: the glottis is ~5 % of the Singer's work
     **[calc]**;
   - keeps pitch and vibrato scatter and timing;
   - loses formant scatter (all singers share one vowel) and the source–tract interaction per
     singer: the tract sees 6× the flow; it can be scaled and the nonlinearity is small at mf;
   - with 2 tracts per part (long/short) much of the formant scatter is back **[calc]**.
3. **Formant filter bank per singer** (5–6 resonators + a nasal pole/zero, like puhuja's
   cascade):
   - ~25 multiply-adds a sample, ~0.02–0.04 % of a core (~20× cheaper) **[calc]**;
   - loses the physical source–tract coupling (the R1:f0 tuning has to be imposed by rule), the
     natural bandwidth changes with the glottis's opening, the physically right levels of the
     vowels, and puhuja's tuned shapes;
   - keeps everything ensemble-related;
   - good enough for a 100-voice symphonic chorus's back rows **[calc]**.
4. **SIMD across singers:** 16 Singers in AVX-512 lanes (one float per singer per section):
   - the leapfrog over sections is the same for every singer (only the coefficients differ);
   - a 4–8× speedup is plausible, since the per-singer loop over 44 sections already vectorises
     only partly **[calc]**;
   - this is the route to a 96-singer chorus with full physics.

Recommendation: chamber choir 24 singers with full Singers now. Lane-parallel Singers (4) later for
the large chorus.

---

## 9. What to take from puhuja

puhuja (`~/puhuja`, C++17, 48 kHz) is a speech synthesizer. It is a transmission-line tract with
oral, nasal and tracheal lines, vocal-fold models, noise sources, and a planner for formants,
prosody and phonemes. Its documents record weeks of tuning for naturalness. Singer.cc is a
deliberately reduced port of its line. This section lists what exists there and what each part
would give a choir.

### 9.1 How singer.cc relates to puhuja [code]

| part | puhuja | singer.cc |
|---|---|---|
| oral line | 64 sections, 4 substeps, walls (mass–damper–spring, radiating: voice bar), viscous at 1 kHz + an HF loss, Borda–Carnot head loss at area steps, piriform sinus option | 44 sections, 3 substeps, constant loss 260/s + 0.035/A, no walls |
| nasal line | 40 sections, 11 cm, velar port at 55 % of the length, two side sinuses (~464 Hz, ~1.4 kHz), nostrils 1.54 cm² | none |
| trachea | 32 sections, 12 cm, lungs at its end (matched: no subglottal resonances, REVIEW F8) | none: lung pressure straight at the glottis |
| area function | `TractShape` (tract.cc): larynx tube, pharynx, velum blend, cos² tongue hump, lips, optional hypopharynx and front space | the same function, without hypopharynx and front space |
| vowel rows | `kLineVowels` (15 rows: a e i o u æ ə ɪ ɛ ʊ ɔ ʌ ɑ ɒ ɜ), refitted 2026-11-04..09 | 5 rows, the older a e i o u |
| source | string folds (2 layers × 16 lanes, default), two-mass IF72, 4×8 grid, block, drawn pulses | a drawn area pulse with Bernoulli flow (≈ puhuja's `PulseLarynx` idea) |
| micro-variation | tension/rest/pressure noise low-passed at 8 Hz, per-cycle jitter 0.6 %, shimmer 3 % | uniform per-period jitter ±0.3 %, shimmer ±3 %, drift 3 c·√s |
| noise | glottal (Re-based) and constriction jets, Strouhal bands (St 0.1, band to 10 fc) | Re-based, fixed bands |

### 9.2 Parts to reuse or port, per item

1. **The newer vowel rows, refitted for singing.** *Reuse, cost 0.*
   - Take ɑ (row 14: 722/1338/2461 target) for "aah", u (row 4, refit 2026-11-04: rendered
     328/846 for "Who.") and ʊ (row 11: 462/1039) for "ooh", and ɔ (row 12: 488/903) for the
     opened ooh of high sopranos **[code, puhuja.cc]**.
   - These targets sit near HSW11's sung "hard" (male 700–785/1175–1235) and a choral [u]
     **[calc]**.
   - Then use puhuja's `linefit` approach to refit each row on Singer's own line, which lacks
     walls and has 44 sections, to the targets of 2.1. The measured offsets in 1.3 show the rows
     do not transfer one-to-one: F1 of a is +100–200 Hz on Singer's line.
   - Fit per part: a male row at 17.5 cm, scaled by length, and a female row at 15 cm, because
     female F2/F1 ratios differ (×1.12/1.15/1.06, below).
2. **Female voice settings.** *Reuse, cost 0.*
   - puhuja's female voice: formants ×1.12 (F1), ×1.15 (F2), ×1.06 (F3) from measured FLEURS/CSS10
     ratios; bandwidths ×1.15/1.10/1.05; F4 at 4.0 kHz; the oral HF loss `LINE_HF_BW_F` 198.6
     **[code]**.
   - Base f0 134/166 Hz with breathiness 1.1 and jitter 0.8 %. Against goog it gave HNR ~18 dB and
     H1−H2 1.7 dB **[code, kFemaleVoice comment]**.
   - For altos and sopranos, the formant ratios set the shape differences beyond length. The
     breathiness and HNR targets set the default chink.
3. **Micro-variation lessons.** *Reuse, cost 0.*
   - The slow tension noise was moved from 30 Hz to 8 Hz: at 30 Hz the harmonics above 1.5 kHz
     broke into noise (HNR 13 dB against 19 in the reference; shimmer 0.20 dB against 0.11). At
     8 Hz with gain 3: 23 dB, 0.06 dB, 7 c wobble **[code, puhuja.cc]**.
   - "Micro 5 matched goog's jitter but as an 8 Hz wobble of ±24 cents: an old woman's tremor"
     **[code]**.
   - Per-cycle jitter "costs HNR": puhuja runs 0.19–0.27 % frame-to-frame against 0.30–0.57 % in
     neural references **[code, larynx.cc]**.

   For trained choral voices: per-period jitter sd ≈ 0.2–0.3 %, slow micro-variation ≲ 5 c at
   ≤ 8 Hz. Singer's uniform ±0.3 % (sd 0.17 %) is fine. Its shimmer ±3 % (sd 1.7 %, ~0.15 dB) is
   at the upper edge: halve it for the choir **[calc]**.
4. **The nasal branch,** for "mmm" (GM2 Humming) and a nasal tinge in "ooh". *Port.*
   - puhuja's `VocalTractLine` couples a 40-section nasal line at the velum with a port area, two
     side sinuses and lossier walls (`LINE_NASAL_LOSS` 0.9935) **[code, line.cc]**.
   - For a hum: lips closed (the lip load fades to a rigid end), velum open ~0.5–1.5 cm².
   - Cost: 40 more sections ≈ +90 % per singer at 4 substeps, or +60 % at Singer's 3 with ~24
     sections (11 cm at Singer's dx ≈ 0.4 cm needs ~27 sections) **[calc]**.
   - Only the humming variant needs it. "Oohs" can stay oral.
   - Cheaper hum: Singer with closed lips + puhuja's cascade nasal pole (270 Hz) and zero (~1 kHz,
     partly compensated, kNasalZeroComp) as a filter on the glottal flow **[code]**.
5. **Walls and the voice bar.** *Port optionally.* Wall impedance raises F1 of closed or nearly
   closed tracts (u, hum) by tens of hertz. Wall radiation gives the closed-mouth hum its "voice
   bar" **[code, TRACT_MODEL.md]**. REVIEW_physics F5 warns that puhuja's wall damping is 6.4×
   the literature value, compensated by WALL_RADIATE = 10 **[code]**: port the physics, not those
   knob values. Cost ~+30 % (one more state per section) **[calc]**.
6. **The trachea and subglottal resonances.** *Skip for now.* puhuja's trachea end is matched, so
   it has no subglottal resonances (REVIEW F8) **[code]**. It adds nothing a choir would miss.
7. **Vocal folds (folds.cc two-mass, string.cc 2 × 16 lanes, grid.cc 4 × 8).** *Port with care.*
   - **Gains:** a natural onset (oscillation grows from the threshold pressure; soft against hard
     onset is the adduction), f0 rising with pressure (~4 Hz per cm H2O, [web, Sund92]), registers
     (falsetto/M2 for tenors' top and sopranos' head voice, with a lower contact quotient), and
     real source–tract interaction (the inertive load when R1 is just above f0, the case of R1:f0
     tuning) **[web, HSW11 citing Titze]**.
   - **puhuja's regime table** gives the continuous parametrisation (DESIGN_folds_physics §5)
     **[code]**:
     - modal a_m 0.02–0.05 cm², ps 5–8 cm H2O;
     - belting ps 12–25 cm H2O, CQ > 0.5;
     - falsetto: thin, near-sinusoidal, often no closure;
     - breathy: chink 0.1–0.3 cm²;
     - transitions with adduction 15–40 ms, ps 50–200 ms, larynx height 100–300 ms.
   - **The risk is pitch accuracy.** puhuja's folds needed a per-cycle tension controller
     (`PITCH_CONTROL` 0.3 per cycle), and the realised pitch had a per-vowel error sd of
     0.27 semitones (27 c) after a fix; before it, 0.81 st **[code, larynx.cc]**.
     - A choir tolerates ≤ 14 c of *constant* scatter (3.2), so the folds would need a much
       tighter loop (cents, over a few periods).
     - Equal temperament against just intonation is then a choice, not an accident.
   - **Cost:** the two-mass model is a few dozen flops at 4× oversampling, ~5–10 % of a Singer
     **[calc]**. The string folds are 2 × 16 lanes in AVX-512 registers with collision, ~20–40 % of
     a Singer **[calc]**.
   - **Recommendation:** keep Singer's drawn area pulse (exact pitch, already interactive through
     the Bernoulli flow). Take from puhuja the *shape* (its fitted pulses, `kFemalePulse`, LF-fitted
     folds as reference) and the regime parameters. Try the two-mass folds as an option
     (`PJ_CHOIR_FOLDS=1`) for onsets and the soprano top, with a tight pitch loop.
8. **Noise.** *Port the Strouhal band.* puhuja's jets use a band from fc = St·U/d (St 0.1) up to
   10 fc **[code, TRACT_MODEL.md]**; the narrow fixed bands rang ("zipping, zonking")
   **[code, LEARNINGS.md]**. REVIEW F2: "the glottal noise source fires at every closure because
   of mismatched areas and a circular Reynolds number for a slit". Check Singer's turbulence
   (Re from u_[0] with a fixed fold width) for the same closure bursts before raising breathiness
   for the women **[code]**.
9. **The source shelf.** puhuja added +8 dB above 1875 Hz on the folds' flow: a sharper closure,
   the mucosal wave's cut-off. Its voiced 4–8 kHz had been 12–18 dB under the references, with a
   tilt of −12.3 against −9.4 dB/oct **[code, larynx.cc]**. Check Singer's LTAS against choir
   recordings first (10.2). A choir wants *less* top than speech references, so this may not be
   needed.
10. **Planning and movement.** puhuja's lesson "movement beats sources" **[code, LEARNINGS.md]**:
    how formants move into and out of targets mattered more than source refinements. Its formant
    followers are critically damped (16/12/9 ms for F1/F2/F3) **[code, DESIGN_formant_planning]**.
    For a choir this means the vowel and pitch transitions at onsets and slurs (5.1, 5.2) deserve
    the care, more than the glottal detail.
    - DESIGN_prosody_planner (accent and contrast tunes) has no direct use. Its idea of planned
      pitch contours fits a slur's scoop and overshoot.
11. **Method.**
    - Compare against a human recording with simple physics: band energies in 10 ms frames and an
      LPC formant track, asking "what does the reference do in these 50 ms that we don't?"
    - Every idea a knob, off by default; A/B listening files for every decision.

    **[code, LEARNINGS.md]**. These carry over as the validation method (10).

### 9.3 Per voice part: what to use

| part | tract (cm) | formant ratios vs male row | source | extras |
|---|---|---|---|---|
| soprano | 14.5 ± 0.6 | ×1.12/1.15/1.06 | drawn pulse, breath 0.35, OQ +0.05 | R1:f0 tuning above ~C5; R2:2f0 optional; lips open toward o above E5 for u |
| alto | 15.5 ± 0.6 | ×1.12/1.15/1.06 | breath 0.3 | R1:f0 for u/o above ~A4, else R1:2f0 |
| tenor | 16.8 ± 0.7 | ×1.0 | breath 0.2 | R1:2f0 near the top; falsetto above A4 (folds option) |
| bass | 17.8 ± 0.7 | ×1.0 | breath 0.15 | narrower epilarynx (opera variant: singer's formant) |

([calc], from sections 2 and 4; the lengths from [memory] on adult vocal tract lengths.)

---

## 10. Validation targets

### 10.1 Per singer

- **Formants:** R1/R2 of the sung vowels at each part's mid pitch within the sd of HSW11
  Table II. Above C5 the soprano R1 within 50 Hz of f0 (R1:f0); Henrich used |Δ| < 25 Hz as
  "tuned" **[web, HSW11]**.
- **Pitch:** mean within ±1 c of the target before scatter. Jitter 0.2–0.4 %, shimmer ≤ 0.2 dB,
  HNR 18–25 dB at mf for men, 15–22 dB for women **[calc from puhuja's values]**.
- **Dynamics:** SPL range pp–ff ≥ 20 dB per singer at mid pitch. L(3 kHz) − L(overall) rising
  ~0.5 dB per dB **[web, Sund95]**.

### 10.2 Ensemble

- **F0 dispersion** in a part (sd of the singers' F0 over 50 ms windows) 20–30 c. The sd of mean
  F0 per note 5–10 c **[web, Cuesta18, Tern93]**. Measure from the model's per-singer F0 directly.
- **Onsets:** the spread between first and last singer of a part ~60–120 ms; mean absolute
  asynchrony ~30–60 ms **[web, DAmario18, Chandna20]**.
- **Dynamic range** of the whole choir pp–ff ~20 dB **[web, TM17]**.
- **LTAS:**
  - Rossing/Ternström's choral-mode LTAS shape: a strong fundamental region, no 3 kHz peak;
  - compare against recordings: the Choral Singing Dataset (16 singers of a semi-professional
    SATB choir, individual close microphones, three songs, Zenodo) **[web, Cuesta18 (snip)]**.
    It allows per-singer as well as summed comparisons.
- **References for the GM expectation:** a Sound Canvas (or its emulation) rendering of 53 and 54
  as a timbre and attack reference; band energies in 10 ms frames as in puhuja's method
  **[code, LEARNINGS.md]**. A tape-replay keyboard "8 Choir" recording for the variant.

### 10.3 Listening tests (the project's way)

A/B against the current pad-choir:
- **Unison:** one key held 8 s at C3, C4, C5, A5.
- **A four-part chord progression** (the `-ensemble -polydemo` chorale) **[code, README]**.
- **A GM file using program 53 as a pad under strings.**

Questions: does it sound like *a choir* (not a chorus pedal), are the high soprano notes thin, is
the attack GM-like, is the release natural in the church?

---

## 11. The model plan

### 11.1 Instruments

| name | GM | content |
|---|---|---|
| `choir` | 53 Choir Aahs | SATB, 6 per part, "aah" [ɑ], moderate attack, church |
| `voice-oohs` | 54 Voice Oohs | the same choir on [u]/[ʊ], softer and slower, a short soft "d"-like release of a constriction at onsets (velocity-dependent), women lead (men only below C4) |
| `choir-hum` (GM2 54 var.) | Humming | closed lips, nasal branch |
| `tape-choir` (later) | — | 4+4, 8-s tape envelope, wow |
| `pad-choir` | 92 | unchanged (or 3 singers of the new choir) |

### 11.2 Structure (after src/stringsect.cc)

- **An `Ensemble::initChoir`** with a `ChoirSection` table: part, singers, lo, hi, maxDiv, prefer
  (as `StringSection`), plus tract length mean/sd, formant ratios, breath, vibrato scale. Values
  in 4.2 and 9.3.
- **Divisi and unison:**
  - keys are shared out as in strings: as few singers move as will do;
  - a moving singer slurs (glides pitch over 80 ms, vowel continuous);
  - a lone key in several parts' tessitura gets the unison of those parts (4.2).
- **Per singer, fixed for the seat:**
  - tract length (part mean × (1 + 0.04 N));
  - formant scatter: the vowel row's hump place/area and lip area perturbed so that R1 ± 8 % and
    R2 ± 6 % (sd);
  - pitch offset N(0, 6 c) (truncated ±15 c);
  - vibrato rate N(5.3, 0.35) Hz, extent N(35, 12) c × part scale (B 0.7, T 0.9, A 1.0, S 1.2),
    onset delay N(0.3, 0.1) s;
  - level offset N(0, 2.5) dB; one in six a strong voice (+4 dB at f/ff);
  - onset habit N(0, 10) ms; breath (chink) per part ± 0.1;
  - position on the risers (7).
- **Per note:**
  - onset delay 25 ms + N(0, 20) (0–80 ms) + the part's common N(0, 8);
  - start pitch N(−15, 15) c gliding in with τ 60 ms;
  - fresh pitch offset N(0, 4 c);
  - vowel jitter ± 3 % on R1;
  - releases scattered 1.5× the onsets;
  - in long notes, staggered breaths every 8–15 s (5.2).
- **Pressure:**
  - velocity and CC11 map to lung pressure 0.4–3.5 kPa for the men at mid range, × (1.4 per
    octave above the part's middle), sopranos up to ~5 kPa at A5 ff (5.3);
  - the pressure rises with τ 30–80 ms (soft → slow);
  - Singer's loudness law (`loud = lung/1000`) has to be rescaled to the new range.
- **Resonance tuning,** a rule on top of the vowel:
  - for sopranos and altos, when f0 > R1(vowel) − 30 Hz, open the jaw (lip and mouth areas
    × k) until R1 ≈ f0 + 10..30 Hz, from a precomputed table R1(k, vowel, length);
  - for tenors and basses, nudge R1 to the nearest 2f0 when within 60 Hz (cheap: one table per
    row).
- **Room:**
  - each singer a source at its riser position with the singing directivity (7), into the church
    by default;
  - sopranos audience-left front, altos right front, tenors left back, basses right back.
- **Cost:** 24 Singers ≈ 16 % of a core plus the room. Above 48 singers, lane-parallel Singers
  (8.3, item 4).

### 11.3 Changes needed in Singer (src/singer.cc)

1. The length from the part (an `init` or `control` argument), not from the note.
2. The vowel rows: ɑ, ɔ, u, ʊ (and keep e, i), refitted on Singer's line to sung targets (9.2,
   item 1). A per-row female variant or a per-formant scale.
3. A jaw/opening parameter for R1:f0 tuning (scales `aLip` and `aOr`).
4. The lung range raised (to ~5 kPa) with the open-quotient and closing-quotient laws remapped to
   it.
5. Seat parameters exposed: pitch offset (now uniform ±6 c, singer 0 none), vibrato extent scale
   and onset delay, formant perturbation, shimmer halved.
6. An optional nasal branch (hum) and optional walls (9.2, items 4–5).
7. An optional two-mass fold source with a tight pitch loop (9.2, item 7).

### 11.4 Parameters with values (first build)

| parameter | value | source |
|---|---|---|
| singers per part | 6 (`PJ_CHOIR_N`) | 3.1, 8.2 |
| tract length S/A/T/B (cm) | 14.5 / 15.5 / 16.8 / 17.8, sd 4 % | 4.2, 9.3 |
| pitch offset per seat | sd 6 c | 3.2 |
| pitch offset per note | sd 4 c | 3.2 |
| drift | 3 c·√s, τ 1.5 s (as now) | code |
| vibrato rate | 5.3 ± 0.35 Hz | 2.6 |
| vibrato extent | ±35 c ± 12, part × 0.7/0.9/1.0/1.2 | 2.6 |
| vibrato onset delay | 0.3 ± 0.1 s | 2.6 |
| onset delay | 25 + N(0, 20) ms, part N(0, 8) | 3.4 |
| level per seat | sd 2.5 dB; 1 in 6 +4 dB at f | 3.5 |
| lung pressure | 0.4–3.5 kPa (men, mid), ×1.4/oct | 5.3 |
| breath (chink) S/A/T/B | 0.35 / 0.3 / 0.2 / 0.15 | 2.4 |
| jitter / shimmer | ±0.3 % / ±1.5 % per period | 9.2, item 3 |
| formant scatter | R1 sd 8 %, R2 sd 6 % | 2.1 |
| attack (ensemble) | 150–300 ms aahs, 250–400 oohs | 5.1, 6.3 |
| release | 150–400 ms + room | 5.2 |
| room | church (4.0 s) unless set | 7 |

### 11.5 Order of work

1. Measure and refit the vowel rows on Singer's line (no ensemble yet); unit test R1/R2.
2. `choir` with the ensemble structure (copying `stringsect.cc`'s allocator), pressures and
   scatters; A/B against pad-choir.
3. Resonance tuning.
4. `voice-oohs`.
5. Hum (nasal branch).
6. Folds option.
7. Lane-parallel Singers.
8. The tape-replay keyboard variant.

---

## 12. Open questions

1. **Constant pitch scatter:** 6 c sd per seat is a guess between the preferred 0–5 c and the
   tolerated 14 c. It should be tuned by ear. Is equal temperament right, or should the choir
   drift toward just intonation in held chords? Choirs tend to just thirds and fifths
   **[memory]**.
2. **Sung formant targets for an English/Latin choral [u] and [ɑ]:** HSW11's are
   Australian-English speech vowels sung. A measured table of choral vowels per part (e.g.
   Ternström & Sundberg 1989, "Formant frequencies in choir singers", JASA 86:517–522) was not
   read.
3. **Choir LTAS numbers** (dB/oct, the 3 kHz level relative to the 500 Hz region) from Ternström
   (1989/1993) were not obtained. Only the qualitative result (strong fundamental, room-dependent
   shape) was.
4. **Does a 6-per-part choir sound "big" enough for GM files** that expect a large sampled
   choir? Or do 8–12 per part matter audibly? There is no perceptual data; listen at 4, 6, 8, 12.
5. **Onset rise times for sung vowel onsets** (soft, breathy, glottal) were not found in a
   primary source. The values in 5.1 are estimates.
6. **Two-mass folds against the drawn pulse:** can the folds hold a choir's pitch accuracy
   (≤ 5 c per note) with a fast tension loop, and is the gain in onsets audible in a 24-voice
   ensemble?
7. **Singer's /a/ formants are 100–200 Hz above puhuja's targets on the same row (1.3).** Is the
   cause the missing walls, the 44 sections or the glottal end? The refit will hide it; it should
   be understood (a check against puhuja's line with walls off).
8. **Children's and boys' choirs** (a "Voice Oohs" colour): vocal tract 11–13 cm, f0 range G3–A5,
   little vibrato. Not researched here.
9. **The GM2 "Humming" variant:** is a nasal branch worth +60–90 % per singer, or does the filter
   approximation (closed lips + nasal pole/zero) suffice?

---

## 13. References

- **[HSW11]** N. Henrich, J. Smith, J. Wolfe, "Vocal tract resonances in singing: Strategies used
  by sopranos, altos, tenors, and baritones", JASA 129(2):1024–1035, 2011 (text read).
  https://www.phys.unsw.edu.au/jw/reprints/SATB.pdf
- **[JSW04]** E. Joliveau, J. Smith, J. Wolfe, "Vocal tract resonances in singing: The soprano
  voice", JASA 116(4):2434–2439, 2004 (text read). https://www.phys.unsw.edu.au/jw/reprints/Joliveauetal.pdf
  ; also Nature 427:116 (2004). https://www.nature.com/articles/427116a.pdf
- **[Garnier10]** M. Garnier et al., "Vocal tract adjustments in the high soprano range", JASA
  127(6), 2010 (snippet). https://hal.archives-ouvertes.fr/hal-00480078/file/GARNIER-JASA-2010.pdf
- **[Sund95]** J. Sundberg, "The singer's formant revisited", STL-QPSR 36(2-3):83–96, 1995 (text
  read). https://www.speech.kth.se/qpsr/1995/1995_36_2-3_083-096.pdf
- **[Sund92]** J. Sundberg, "Breathing behavior during singing", STL-QPSR 33(1):49–64, 1992 (text
  read). https://www.speech.kth.se/qpsr/1992/1992_33_1_049-064.pdf
- **[STS91]** J. Sundberg, I. Titze, R. Scherer, "Phonatory control in male singing", J. Voice,
  1993 (abstract). https://iro.uiowa.edu/esploro/outputs/journalArticle/Phonatory-control-in-male-singing-A/9984719747402771
- **[TM17]** I. Titze, L. Maxfield, "Acoustic factors affecting the dynamic range of a choir", JASA
  142(4):2464–2468, 2017 (read via a summary of the full text). https://pmc.ncbi.nlm.nih.gov/articles/PMC5662467
- **[Tern02]** S. Ternström, "Choir acoustics: an overview of scientific research published to
  date", TMH-QPSR 43:1–8, 2002 / IJRCS 1(1):3–11, 2003 (snippets).
  https://typeset.io/pdf/choir-acoustics-an-overview-of-scientific-research-published-2em0kyqmrt.pdf
- **[Tern93]** S. Ternström, "Perceptual evaluations of voice scatter in unison choir sounds", J.
  Voice 7(2):129–135, 1993; STL-QPSR 32:41–49, 1991 (via Chandna20).
- **Ternström's choir publications** (list, incl. SOR 1999, 2005, LTAS 1989/1993, choir synthesis
  1988): https://www.speech.kth.se/~sten/publications.html
- **[Cuesta18]** H. Cuesta, E. Gómez, A. Martorell, F. Loáiciga, "Analysis of intonation in unison
  choir singing", ICMPC 2018 (abstract and snippets).
  https://repositori.upf.edu/items/fc7337cd-8275-4878-aad3-118a47f036be ; Choral Singing Dataset:
  https://zenodo.org/record/1286485
- **[Chandna20]** P. Chandna, H. Cuesta, E. Gómez, "A deep learning based analysis-synthesis
  framework for unison singing", ISMIR 2020 (text read). https://arxiv.org/pdf/2009.09875
- **[DAmario18]** S. D'Amario, H. Daffern, F. Bailes, "Synchronization in singing duo
  performances: the roles of visual contact and leadership instruction", Front. Psychol. 9:1208,
  2018 (text read). https://eprints.whiterose.ac.uk/134187/1/fpsyg-09-01208.pdf
- **[DAmario18b]** S. D'Amario, H. Daffern, F. Bailes, "A longitudinal study investigating
  synchronization in a singing quintet", J. Voice, 2018 (text, introduction).
  https://eprints.whiterose.ac.uk/134306/14/1-s2.0-S0892199718301747-main.pdf
- **[Mann14]** L. M. Mann, "Effects of solo and choral singing modes on vibrato rate, extent, and
  duration exhibited by undergraduate female singers", IJRCS 5(1):26–38, 2014 (text read).
  https://acda-publications.s3.us-east-2.amazonaws.com/IJRCS/IJRCS5-1mann.pdf
- **[AcostaDaffern]** G. Acosta Martínez, H. Daffern, "The role of vibrato in group singing: a
  systematic review", J. Voice, 2022/2025 (text read).
  https://eprints.whiterose.ac.uk/id/eprint/228375/1/1-s2.0-S0892199722003551-main.pdf
- **[Daffern16]** H. Daffern, "Blend in singing ensemble performance: vibrato production in a
  vocal quartet", J. Voice, 2016 (abstract). https://eprints.whiterose.ac.uk/108367
- **[Reid07]** K. Reid et al., "The acoustic characteristics of professional opera singers
  performing in chorus versus solo mode", J. Voice 21:35–45, 2007 (abstract).
  https://pesquisa.bvsalud.org/portal/resource/e/mdl-16427767
- **[Rossing86]** T. Rossing, J. Sundberg, S. Ternström, "Acoustic comparison of voice use in solo
  and choir singing", JASA 79(6):1975–1981, 1986; and "… soprano solo and choir singing", JASA
  82(3):830–836, 1987 (via TM17, Mann14 and snippets).
- **[Fischinger15]** T. Fischinger, K. Frieler, J. Louhivuori, "Influence of virtual room acoustics
  on choir singing", Psychomusicology 25(3):208–218, 2015 (text read; reviews Ternström 1993, 1994,
  1999, Marshall & Meyer 1985, Jers & Ternström 2005, Grell et al. 2009).
  https://jyx.jyu.fi/bitstreams/19083343-3b37-4b3f-973f-ab7c3fe2d974/download
- **[Monson12]** B. Monson, E. Hunter, B. Story, "Horizontal directivity of low- and
  high-frequency energy in speech and singing", JASA 132(1):433–441, 2012 (read via a summary of
  the full text). https://pmc.ncbi.nlm.nih.gov/articles/PMC3407162
- **[Cabrera11]** D. Cabrera, P. Davis, A. Connolly, "Long-term horizontal vocal directivity of
  opera singers", J. Voice, 2011 (snippet).
  https://www.geidai.ac.jp/~marui/course_enshu_2024/cabrera_davis_connolly_2011.pdf
- **[DeJonckere25]** P. DeJonckere, J. Lebacq, "Biophysics of voice onset: a comprehensive
  overview", Bioengineering 12(2):155, 2025 (abstract). https://www.ncbi.nlm.nih.gov/pmc/articles/PMC11851932/
- **[Wiki-Mtron]** "Mellotron", Wikipedia (read via a summary). https://en.wikipedia.org/wiki/Mellotron
- **[Hollowsun]** Hollow Sun, Mellotron history. https://hollowsun.com/nostalgia/vintage/mellotron/index.html
- **[SynthMania]** SC-50 / SC-55 Sound Canvas patch descriptions (snippet).
  https://synthmania.com/sc-50_sound_canvas.htm
- **[UQ-SATB]** "Four-part vocal style (SATB)", University of Queensland music theory text
  (snippet). https://uq.pressbooks.pub/music-theory/?p=1092
- **[Hutchings]** Hutchings, SATB writing guide (snippet, via the search on choral ranges;
  thresholds garbled in the extract).
- **puhuja** (sister project, local): `~/puhuja/TRACT_MODEL.md`, `DESIGN_folds_physics.md`,
  `DESIGN_larynx.md`, `DESIGN_formant_planning.md`, `DESIGN_prosody_planner.md`,
  `LEARNINGS.md`, `REVIEW_physics.md`, `puhuja.cc` (kLineVowels, kMaleVoice, kFemaleVoice,
  kNasalPoleHz), `larynx.cc`, `line.cc`, `string.h`, `folds.h`, `grid.h`, `tract.h`.
- **This repository:** `src/singer.h/.cc`, `src/lead.cc` (pad-choir), `src/stringsect.cc`,
  `src/room.cc`, `src/midifile.cc`, README.md.
