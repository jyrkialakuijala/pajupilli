# Comparing two MIDI synthesizers on the same files: research and design

Status: research and design only (2026-11-19). Nothing here is built. It proposes
`tools/synthcompare.py`, which renders (or records) one MIDI file on two synthesizers, lines the
two up against the MIDI, measures what a listener would notice, ranks the differences in units of
"just noticeable", and writes a report with one score per instrument and one overall. The first
pair is pajupilli `-play` against TiMidity++ 2.14.0 with a SoundFont (Jnsgm2, FluidR3_GM or
TimGM6mb); later, other software synths and hardware recorded through an audio interface. A proof
of concept was run on 19 s of `~/midi/Anighting.mid` (piano and bass): appendix A.

How each number is marked (as in `timpani.md` and `viola.md`):
- **[T]** read in the primary text (downloaded and read for this document). **(wf)** after it:
  read through a summarising web fetch, not the raw text.
- **[S]** from a secondary source, an abstract, a search snippet or a machine summary.
- **[E]** my estimate, measurement in the proof of concept, or arithmetic from cited values; not a
  published value.
- **[M]** from general knowledge, not looked up: check before relying on it.

References are short keys in brackets; the list with URLs is section 12. What could not be found
is section 11. Several primary texts were read by a research helper agent that fetched them in
full; they are marked [T] as it reported.

**Short answer.**
1. Render each MIDI channel alone on both synths (the same filtered MIDI file to both: note events
   of the other channels removed, all controllers, programs and tempo kept) plus the full mix.
   pajupilli's mix was the exact sum of its solos (residual −151 dB, `-director off`); TiMidity's
   within −27 dB **[E]**. Stems make every difference attributable to a channel, a key range and a
   velocity range without source separation.
2. The MIDI file is the ground truth for *where* every note is. Each synth is aligned to the MIDI
   (a global offset by cross-correlating onset strength, then per-note onsets found in a window
   around each MIDI onset), never one synth to the other. DTW is only needed for a hardware
   recording that drifts, or for a synth that plays the score freely (pajupilli's director).
3. Per note and per channel, measure a small set of features with known just-noticeable
   differences (JNDs): level (dB), spectral envelope on 1-ERB bands (dB per band), spectral
   centroid (cents), onset delay (ms), attack time, decay (T60), f0 (cents), vibrato, note length,
   stereo position and width, noise floor and clicks. Divide each difference by its JND, weight by
   how audible the passage is (its loudness and how much of it is masked by the other channels in
   the mix) and how long it lasts, and rank.
4. Neither synth is right. Report differences as differences and, where there is a third opinion,
   say which side is nearer it: several SoundFonts give a spread (a "reference cloud"), and a
   difference smaller than the spread between SoundFonts is not worth acting on. A recording of
   the real instrument, where one exists, is the only true reference.
5. Off-the-shelf quality metrics (PEAQ, ViSQOL, PEMO-Q, HAAQI) assume a clean reference and a
   degraded copy of the *same* signal: two synths are two different performances of the same
   score, and these metrics report only "very different". They are useful at most as a single
   sanity number per stem. Distribution metrics (FAD on CLAP/VGGish/PANNs embeddings) compare
   *sets* of sounds and need minutes of audio; they give a coarse "how far apart" per instrument,
   not what to change. The core of the tool is therefore its own feature set, in JND units.

---

## 1. What is being compared

Two renders of one MIDI file differ in three kinds of way:

- **Mastering**: overall gain, peak normalisation, limiters, master reverb and chorus, the
  sample rate. Not the instrument's sound: remove before comparing (section 2.3).
- **Interpretation**: which GM program a channel gets, the velocity → level curve, how CC7, CC11,
  CC64 and pitch bend are followed, voice stealing, and (in pajupilli) the director's reading of
  timing and dynamics. These are real differences but must be reported as such, not as timbre.
- **The instrument's sound**: per note: spectrum, attack, decay, pitch, noise, stereo image;
  per instrument: evenness across keys and velocities, balance against the others.

A difference is worth fixing in pajupilli if (a) a listener would hear it in the mix, (b) it is
larger than the variation between other plausible renderings (several SoundFonts, or recordings),
and (c) pajupilli is the outlier. (b) and (c) need a third opinion; without one the tool can only
rank by audibility.

## 2. Rendering

### 2.1 Per-channel stems from one MIDI file

Neither synth needs a solo switch: write a filtered copy of the MIDI file per channel that keeps
every event but the note on, note off and polyphonic aftertouch of the other channels **[E]**
(done in 40 lines of Python, appendix A). Keeping the other channels' controllers, programs,
pitch bend and the tempo map means each solo channel sees exactly the state it sees in the mix.
The same file goes to both synths and to hardware, so this is also how a hardware synth is
recorded one part at a time.

TiMidity can mute channels directly (`-Q n[,...]`, "--mute") **[T]** (`timidity -h`), but the
filtered file is the portable route.

Two checks the tool makes on every run:
- **Additivity:** mix − Σ stems, in dB re the mix. pajupilli with `-director off`: −151 dB, i.e.
  bit-exact up to float rounding (each part renders on its own thread and mixes in a fixed order,
  README "-play") **[E]**. TiMidity 2.14 with Jnsgm2/FluidR3: −27 dB, with the same total energy
  within 0.02 dB; unchanged by `-p 256` or `--no-volume-compensation` **[E]**. Probably TiMidity's
  envelope and control updates are quantised to blocks of `control_ratio` samples whose phase
  depends on which events exist **[M]**. −27 dB is fine for feature comparison, not for sample
  subtraction.
- **Interpretation does not depend on the other parts.** pajupilli's director gives "one reading
  for all parts" and the organ planner reads all of an organ's channels (README "-play") **[T]**:
  with them on, a solo render is not the same performance as the part inside the mix. Use
  `-director off` for stems, or render stems with the director's decisions frozen (a future
  `-play` option that writes the director's timing and dynamics into the MIDI, `PJ_PLAY_EXPORT` or
  similar, so both synths play the same reading). Otherwise compare timing on the mix only.

### 2.2 TiMidity at 48 kHz with a chosen SoundFont

    timidity -c sf.cfg -s 48000 -OwS2 -EFreverb=d -EFchorus=d -o out.wav in.mid

`-s 48000` sample rate, `-Ow` RIFF WAVE, `S` stereo, `2` 24-bit, `-EFreverb=d` and
`-EFchorus=d` disable the MIDI-controlled effects **[T]** (`timidity -h`). TiMidity renders the
whole file (no "until" option); 300 s of piano and bass took 4–5 s **[E]**.

**Choosing the SoundFont is not `-x 'soundfont …'` or `-c` with a `soundfont` line.** On this
machine `/etc/timidity/timidity.cfg` is always read and sources `fluidr3_gm.cfg`, which maps
every program explicitly (`0 %font "FluidR3_GM.sf2" 0 0 amp=29 pan=0`, 2826 lines). An explicit
mapping wins over a later `soundfont` directive: renders with `-c` naming Jnsgm2, FluidR3 or
TimGM6mb were bit-identical (same md5) **[E]**. What works is a config that maps every program
and drum key again, which overrides the default ones:

    bank 0
    0 %font "/home/jyrki/soundfonts/Jnsgm2.sf2" 0 0
    ...                                        (all 128 programs)
    drumset 0
    35 %font "/home/jyrki/soundfonts/Jnsgm2.sf2" 128 0 35
    ...                                        (keys 27-87)

With it, Jnsgm2, FluidR3 and TimGM6mb render differently (three md5s) **[E]**. The tool generates
this file per SoundFont (and the other GM2 banks and drum kits if a file uses bank select).

**Debian's `amp=` trims are mastering, not the SoundFont.** `fluidr3_gm.cfg` gives each program
its own amplification (piano `amp=29`, acoustic bass 32 `amp=125`, the standard kick `amp=206`)
**[T]** (the file). In the proof of concept they move the piano–bass balance of FluidR3 from −2.4
to −9.4 LU **[E]**. Generate configs without `amp=` (or with all 100) so the SoundFont's own
balance is what is compared, unless the point is to compare against "TiMidity as Debian ships it".

`/home/jyrki/soundfonts/SGM-V2.01.sf3` (GM bank 0 plus GS variation banks and drum kits) is an
SF3: its samples are Ogg Vorbis. TiMidity++ 2.14 plays it as noise (spectral flatness 0.84 against
0.2–0.3 for the SF2s, tested by the coordinator of this note) **[E]**. It needs converting to SF2
(decode each sample) or a player that reads SF3 (FluidSynth ≥ 2.0 **[M]**, not installed).

The SoundFonts also differ in overall level: Jnsgm2 renders about 0.5× the rms of FluidR3 on a
string chord **[E]** (coordinator) and 1.6 dB quieter on the proof-of-concept mix **[E]**. Overall
level is normalised away (2.3); what remains is each SoundFont's internal balance.

TimGM6mb (6 MB) and FluidR3 (142 MB) are older GM sets; Jnsgm2 (33 MB; JNSGM v2.0, 128 GM programs
plus 8 drum kits) is a GM/GM2 set. The three
disagree with each other by 2–5 dB rms in spectral envelope per stem and by up to 6 dB in
balance (appendix A) **[E]**: that spread is the yardstick in section 4.4.

### 2.3 Keeping mastering out

- **pajupilli** scales a whole file to its peak when the peak passes 0.99 and prints
  `(peak X: the whole song N dB down)` **[T]** (`src/midifile.cc` near line 1449). The tool parses
  that line and undoes it, or (better) normalises every stem itself, so the factor does not
  matter. A stem rendered alone may have a different peak than in the mix, so mix and stems get
  different factors: parse each. It also scales a file's velocities when their median is over 96
  (`velocities: median … scaled by …`; `PJ_PLAY_VELMEDIAN`) **[T]**: that is interpretation, not
  mastering, and is reported.
- **pajupilli's room** is part of its instruments (each instrument sounds in its own room; the
  organ in its nave). TiMidity's reverb is a separate effect. Compare dry TiMidity (`-EFreverb=d`)
  with pajupilli as it is, and report the reverb features (DRR, RT60, section 3.7) as their own
  line rather than letting a room difference leak into "timbre". For the per-note timbre features,
  use the first 150–300 ms of the note, where direct sound dominates **[E]**.
- **Gain.** Level differences are measured in two layers: the overall level of the mix is
  normalised away (it says nothing: a volume knob); everything else is measured *relative*: each
  stem against the mix, each note against its stem's median, each band against the stem's total.
- **Dither, clipping:** render at 24 bit or float; check for clipped samples (|x| ≥ 0.999 FS) and
  report them as an artefact.

## 3. Alignment

### 3.1 Two clocks, one score

Both synths are aligned to the MIDI, not to each other. The MIDI gives for every note: channel,
key, velocity, on and off time (after the tempo map), and the controllers in force. Each render
has an unknown constant offset (file lead-in, latency), and a hardware recording also a clock
drift and an unknown start.

1. **Global offset.** Onset-strength function (positive log-spectral flux, 5 ms hop) of the render
   cross-correlated with an onset-strength function synthesised from the MIDI (a pulse at each
   note-on, weight ∝ velocity); search ±2 s **[E]**. In the proof of concept, render against
   render gave TiMidity 5–10 ms earlier than pajupilli on the mix **[E]**.
2. **Drift** (hardware only). Repeat the cross-correlation in 30 s windows; fit offset(t) as a
   line; the slope is the clock-ratio error. A difference of 50 ppm (a cheap interface against the
   computer's clock is of this order: consumer cards 50–100 ppm measured **[S]** [baudline]) is 15 ms after 5 minutes: above the timing JND, so
   it must be removed. Resample the recording by the fitted ratio (scipy `resample_poly` with a
   rational approximation, or a polyphase fractional resampler).
3. **Per-note onsets.** For each MIDI note, look in a window −20 … +150 ms around its MIDI onset
   in the solo stem. Detectors in order of robustness:
   - monophonic stem, isolated notes: maximum slope of the 1 ms log-energy envelope (used in the
     proof of concept: ~85 % of isolated bass notes found **[E]**);
   - polyphonic stem: energy in narrow bands at the note's first few harmonics (a "harmonic comb"
     on a 1024-point STFT, 2.5 ms hop), whose rise marks the note even inside a chord;
   - chords: the onset of the chord (all notes within 30 ms in the MIDI) is one event.

   The note's **delay** is the detected onset minus the MIDI onset. Its median per stem is that
   synth's latency for that instrument; the spread around the median is its timing behaviour.
4. **DTW** only when the synth plays a different timing on purpose (pajupilli's director, a
   human playing a hardware synth, a groove quantiser). Use DTW on chroma + onset features
   (score-informed alignment in the style of Ewert & Müller, section 10) between the render and
   a MIDI-synthesised feature sequence, with a band constraint of ±300 ms. Then timing itself
   becomes a feature (the DTW path minus the diagonal, per beat).

### 3.2 Attack time, latency and release tails

- A slow attack moves the "max slope" point later; a perceptual onset is about where the
  envelope reaches −15 … −10 dB of its first maximum **[M]**. Report both: **delay** (MIDI to
  perceptual onset) and **rise time** (10 % to 90 % of the first maximum's amplitude, or log
  attack time, section 3.4).
- Tails: compare a note over [onset, MIDI off + 1.5 s] but stop at the next note on the same key.
  For timbre use the steady part; for decay fit the tail.

### 3.3 What the stems buy

A difference found in a stem is attributed directly to: the channel (instrument), the key range
(MIDI note numbers: group in octaves or in 6-key bins), the velocity range (bins of 16), the
articulation (note length class: < 150 ms staccato, legato if the next note starts before this
one ends, held > 1 s), the position in the bar (from the tempo and time-signature meta-events).

## 4. Features and how to measure them

Each feature: definition, how to measure it in numpy/scipy, its unit, its JND (section 5.1).

### 4.1 Loudness and balance

- **Stem loudness:** ITU-R BS.1770 K-weighted integrated loudness, gated (400 ms blocks with 75 %
  overlap, absolute gate −70 LUFS, relative gate −10 LU) **[S]** [R128] (BS.1770 itself not read). `tools/levels.py`
  already has the K-weighting; the gating is 10 lines (appendix A). Unit: LU.
- **Balance:** stem LUFS minus mix LUFS, per stem (removes the global gain). The difference of
  that between the synths is the balance error, in dB.
- **Time-varying loudness** where masking matters: Moore–Glasberg short-term loudness (ISO 532-2
  family; Glasberg & Moore 2002) or Zwicker (ISO 532-1). Not in scipy; implement a simplified
  one: 1-ERB bands from an STFT (excitation ≈ band power spread by a level-dependent
  spreading function), specific loudness N′ = C·((E/E0)^0.2 − 1)-like compression, summed **[M]**.
  The `mosqito` package implements ISO 532-1 in Python **[T]** [MOSQITO] (not installed; pure
  numpy/scipy, vendorable).
- **Velocity → level curve:** for each isolated note, K-weighted level over its first 160 ms;
  fit level = a + b·log10(v/127) per stem. Report the slope b (dB per decade of velocity) and the
  range v16 → v127 (b × 0.90). TiMidity with FluidR3 is the curve pajupilli already matches for
  winds (README: "33 log10(v/127) dB") **[T]**.
- **Dynamic range of the passage:** EBU Tech 3342 loudness range (LRA: 95th minus 10th percentile of
  3 s short-term loudness, −20 LU relative gate) **[S]** [T3342].

### 4.2 Timbre

- **Spectral envelope on ERB bands.** Long-term average spectrum (LTAS) of the stem, or of each
  note's first 300 ms, in 1-ERB-wide bands from 40 Hz to 16 kHz (37 bands; ERB-number
  21.4·log10(1 + 0.00437 f) **[S]** [ERB]), each normalised to its own total. The difference per
  band is in dB. For each note it should be taken on the harmonics (partial levels re the
  fundamental) when the note is long enough, since an ERB band between two partials of a low note
  measures leakage and reverb.
- **Spectral centroid** (Hz, ratio in cents), and **brightness** as the fraction of energy above
  1.5 kHz (or the spectral slope in dB/octave from a line fitted to the ERB-band levels).
- **MFCC:** 20 coefficients on a mel filterbank; Euclidean distance of the means per note
  ("MFCC distance"). Compact but not in perceptual units: use for clustering notes ("these notes
  sound alike in A but not in B"), not for scoring.
- **Roughness:** Daniel & Weber or Vassilakis' model on partial pairs **[M]** (mosqito has
  Daniel & Weber, validated **[T]** [MOSQITO]). Relevant for a chorus or detuned unisons (organ, tonewheel organ, pads).
- **Inharmonicity:** fit fn = n·f0·√(1 + B·n²) to the measured partials of a note (piano, bass,
  bells); compare B. The partial frequencies come from parabolic interpolation on a zero-padded
  FFT (appendix A's f0 code).
- **Spectral flux / temporal envelope of the partials:** the partial levels at 50, 150, 500 ms;
  "the 5th partial dies twice as fast in A" is the actionable output.

### 4.3 Attack

- **Delay** (section 3.2), **log attack time** log10(t90 − t10 of the amplitude envelope, s), one
  of the strongest timbre dimensions **[S]** [McA95].
- **Attack noise:** energy of the non-harmonic residual (spectrum minus a harmonic comb) in the
  first 50 ms, re the note's level; hammer, bow, breath and chiff noise. Unit dB.
- **Attack spectrum:** the ERB-band envelope of the first 30 ms against that of 150–300 ms (how
  much brighter the attack is).

### 4.4 Decay and release

- **Decay rate** of a held note: slope (dB/s) of the log-envelope from −5 dB to −35 dB below its
  peak, or T60 = 60 / rate; per partial for piano and plucked strings (each partial its own T60);
  piano's double decay (fast then slow) as two slopes.
- **Release:** time from MIDI note-off to −40 dB re the level at note-off; the release's spectrum
  (damper noise, key-off).
- **Tail into the room:** separate from the note's own decay by measuring the stem's level
  between notes (section 4.7).

### 4.5 Pitch and intonation

- **f0 per note** in cents from 12-TET A440 (or from the MIDI's tuning, with pitch bend applied):
  harmonic-sum search ±60 c around the written pitch, refined by parabolic interpolation of the
  strongest harmonic on a zero-padded FFT of 120–600 ms of the note (appendix A) **[E]**. For
  vibrato, the same on a sliding 80 ms window, giving f0(t).
- **Vibrato:** rate (Hz) and depth (cents peak-to-peak) from f0(t)'s strongest component in
  3–9 Hz; also onset delay of the vibrato.
- **Pitch stability:** spread of f0 between notes of one key (cents), and the glide over a note.
- **Beating:** amplitude modulation 0.5–15 Hz on a partial (detuned unisons, piano trichords,
  chorus): its rate and depth.
- **Tuning system:** for a chord, the deviation of each interval from ET and from just; pajupilli
  tunes some organs and others by spectra (README), so this is a feature, not an error.

### 4.6 Articulation and timing

- **Onset delay** per note and its spread (section 3.1).
- **Sounding length:** perceptual offset (level falls 20 dB under the note's peak, or the next
  note's onset) minus the onset, against the MIDI's length. Ratio and ms.
- **Legato overlap**, **staccato shortening**, **sustain pedal** following (a note after pedal-off
  must stop; a note under pedal must ring).
- **Timing against the beat** (with the director or a human): mean and rms deviation per beat
  position.

### 4.7 Stereo image and room

- **Position:** per stem, the interchannel level difference ILD = 10·log10(PL/PR) (dB) and the
  phase-weighted interchannel time difference from the cross-correlation peak within ±1 ms.
- **Width / envelopment:** interchannel coherence (maximum of the normalised cross-correlation
  within ±1 ms) over 80 ms frames, per band; ICC in [0, 1] (the room-acoustics IACC with the
  stereo channels in place of the ears) **[M]**.
- **Direct-to-reverberant ratio:** per note, energy in the first 2.5–5 ms after onset (in practice
  for music: the first 50 ms, C50-like) against the energy after 50 ms that persists after the
  note's MIDI end: C80-like clarity is easier to define on music than DRR **[E]**.
- **RT60 of the room:** from the decay after the last note of the piece and after general pauses
  (Schroeder-style backward integration of the band-filtered tail, T20 or T30 fitted between −5
  and −25/−35 dB) **[M]**.

### 4.8 Noise and artefacts

- **Noise floor:** stem level in gaps where no note of that channel sounds and the previous note
  ended > 2 s earlier (after the room's tail), per ERB band. A physical model's self-noise
  (breath, bow) or a sampler's loop noise.
- **Clicks:** a high-passed (> 4 kHz) envelope's peaks more than 20 dB above its 50 ms median,
  not at a MIDI onset; also voice-steal clicks (at note-ons when polyphony is full). Report
  count and time.
- **Aliasing:** for a single high note, energy at non-harmonic frequencies that fold
  (fs − n·f0) above −80 dB re the fundamental; best with a diagnostic MIDI (a slow chromatic run
  on each instrument, section 7.4).
- **Hum and DC:** peaks at 50/60 Hz and harmonics (hardware only), DC offset.
- **Loop artefacts** (samplers): periodic amplitude modulation at the loop length in a held note.
- **Clipping**, NaN, silence where the MIDI has a note (a dropped note: level < −40 dB re its
  neighbours of the same velocity).

### 4.9 Evenness within an instrument

From the per-note tables: level, centroid, delay, T60 and f0 as functions of key and velocity.
Fit a smooth model (level ~ a + b·log v + c·key, or a 2-D spline over key and velocity) and report
the residual's standard deviation and its outliers: "D♯4 is 6 dB louder than its neighbours" is
the actionable output. The proof of concept found pajupilli's piano scatter 3.1 dB against 2.0–
2.3 dB for the three SoundFonts (appendix A) **[E]**.

### 4.10 A wide panel of similarity measures

No single number has been shown to track how alike two clearly different sounds are (section
5.5). So the tool computes many simple measures, each tied to something a listener hears, and
reports each one separately against the references' spread (5.4). They are taken per note
(onset to the next onset, or to 1.5 s), per chord (the notes that sound together), and per stem.
Several of them agreeing is the evidence that a difference is real. Most have no published JND
for this use, so their scale comes from the references' spread **[E]**.

*Spectral similarity*
- **ERB-spectrum correlation and log-spectral distance:** the Pearson correlation of the two
  notes' ERB-band levels (dB), and the rms of their dB difference after removing the mean
  (level) and, separately, the slope (tilt). Shape, level and tilt get three separate numbers.
- **Spectral earth mover's distance** on the ERB axis: how far (in ERB) the energy has to move
  to turn one spectrum into the other. Unlike a band-by-band difference, it treats a formant
  moved by 1 ERB as a small change rather than two big ones **[M]**.
- **Harmonic-profile distance:** the partial levels re the total (partials 1–16, or up to
  8 kHz), compared as dB differences weighted by audibility (a partial masked by its neighbours,
  by Moore's excitation pattern, counts less) **[E]**. Also derived from it: **tristimulus**
  (Pollard & Jansson: the energy in partial 1, in 2–4, and in 5 and up), **odd/even ratio**
  (clarinet-likeness), and **spectral irregularity** (Krimphoff: how jagged the envelope is
  from one partial to the next) **[S]**.
- **Spectral flatness / harmonic-to-noise ratio:** how tonal the note is; a bow, breath or
  hammer noise level, separate from the attack noise of 4.3.
- **Sharpness** (DIN 45692 or Aures, in acum) as the perceptual form of brightness; mosqito
  computes it **[T]** [MOSQITO].

*Sensory dissonance and roughness*
- **Plomp–Levelt sensory dissonance** (Sethares' parameterisation): sum over all pairs of
  partials of the measured spectrum, each pair's dissonance by its frequency difference
  relative to the critical bandwidth at their mean frequency, weighted by the smaller amplitude
  (or the product). Computed per chord and per held note. A synth whose partials are
  inharmonic, detuned (chorus) or tuned differently (spectral tuning, just intervals) shows up
  here even when the levels match **[M]**.
- **Roughness** (Daniel & Weber, 4.2) as the time-domain counterpart: it sees beats from the
  waveform's envelope, where the Plomp–Levelt sum sees partials.
- **Dissonance curve of a single note:** the same sum taken between the note's spectrum and
  itself transposed by 0–1200 c. Its minima show which intervals the timbre makes consonant.
  A good check for the spectral tuning, and for bells and pianos.
- **Fluctuation strength** (vacil, modulation around 4 Hz): slow beats, vibrato and tremolo,
  where roughness covers 20–300 Hz modulation.

*Transients and time structure*
- **Transient coherence:** at each onset, the onset time in each ERB band (the steepest rise of
  that band's envelope). The spread of those times across bands, weighted by energy, is how
  "together" the attack is: a sharp hammer or pluck is coherent, a bowed or blown start rises
  band by band. Measured as a spread in ms, and as an order (low first or high first) **[E]**.
- **Phase coherence at the attack:** the group-delay spread of the first 20 ms over the
  harmonics; a click with all partials in phase sounds sharper than the same spectrum smeared.
- **Temporal centroid** of each note's envelope, and **onset sharpness** (the largest rise in
  dB/ms), next to the log attack time of 4.3.
- **Partial-envelope correlation:** how alike the envelopes of the first 8 partials are over a
  note (high: one scaled shape, sample-like; lower: the partials living their own lives, as in
  a real string). A physical model and a looped sample differ here.
- **Modulation spectrum distance:** per ERB band, the spectrum of the envelope's fluctuations at
  0.5–64 Hz; the distance between the two synths' modulation spectra catches vibrato, tremolo,
  beating, bow noise and the rotary speaker in one measure **[M]**.
- **Spectral flux:** frame-to-frame change of the ERB spectrum; its mean over the sustain is how
  much the sound lives once it has started.

*Pitch and tonality*
- **Pitch salience / harmonicity:** the strength of the harmonic-sum peak (4.5) against the
  spectrum's total: how clearly the note has a pitch (low bass notes and bells differ most).
- **Chroma similarity** per chord: the correlation of the two 12-bin pitch-class profiles. A
  check that the right notes sound at the right strength; a missing chord note or a doubled
  octave shows up here.
- **Virtual pitch agreement:** whether the perceived pitch (Terhardt's model, or simply the
  harmonic sum) lands on the written note in both synths; octave errors in low notes.

*Stem and mix*
- **Loudness-weighted spectral difference** of the whole stem (Moore–Glasberg specific loudness
  in sones per ERB band), as an overall timbre-balance check.
- **Mix sensory dissonance** over time: Plomp–Levelt on the full mix in 200 ms frames. Where one
  synth is much rougher or smoother than the other at the same moment, the passage is worth
  listening to.

*Overall*
- **Zimtohrli distance** (per note and per stem), included with a modest weight (5.5). It
  catches differences the hand-made measures above may miss, but it was tuned only near the
  threshold of hearing, so a large value is not read as a size.

All of these are a few lines each on top of the stage that already finds the notes, the ERB
spectra and the partials (7.2); none needs a trained model.

## 5. Psychoacoustic significance

### 5.1 JNDs

The JND is the unit of every difference. Values for isolated stimuli in quiet are lower bounds:
in music and in a mix, differences must be 2–3× larger to be noticed **[E]**, which the masking
weight (5.2) partly covers. The table gives the value the tool uses and its source.

| Feature | JND used | Source and conditions |
|---|---|---|
| Level of a note or stem | 1 dB | Pure tones: ~1.5 dB near threshold to ~0.5 dB at 80 dB SL (unverified); intensity discrimination is a "near miss" to Weber's law, slope 0.91 over 40–90 dB SL at 1 kHz **[S]** [Rab76 via JASA79]; one level function across 200–8000 Hz **[S]** [JWG77]. 1 dB is the usual music value **[M]**. |
| Balance between stems | 1 dB | same; in a mix, 1–2 dB **[E]** |
| Partial or band level (spectral shape) | 2 dB per ERB band, ~1 dB for a broad tilt | Profile analysis: thresholds within ±2 dB of theory, little change with the number of components **[S]** [Gre92]; one component's increment needs ~+6 dB in 0 dB-signal units **[S]** [bioRxiv23]. |
| Spectral centroid / brightness | ~4 % (≈70 c) **[E]** | No clean percentage found (section 11); McDermott et al. 2010 measured brightness thresholds by envelope shift **[S]** [McD10]. Placeholder to be checked. |
| f0, held note | 5 c (musician: 2 c) | Pure tone 1 kHz, 200 ms: 1.8 Hz = 3.5 c **[S]** [Moo74]; complex tones 0.1 % F0 = 1.7 c, 0.2 % for one partial = 3.4 c **[S]** [Goc07]; Beauchamp suggests 2 c as a synthesis target **[S]** [Bea09]; musicians 2–6× better than non-musicians **[S]** [Mic06]. Short notes (< 100 ms): more, ~10–20 c **[M]**. |
| Onset timing within a line | 6 ms for IOI < ~250 ms; 2.5 % of the IOI above | Isochronous sequence, Friberg & Sundberg 1995; training made no difference **[S]** [FS95]. |
| Onset asynchrony between parts | 20 ms (order), 2–3 ms (any asynchrony) | Hirsh 1959 order judgement 17–20 ms **[S]** [Hir59]; 2–3 ms snippet unclear **[S]**. Ensembles play 30–50 ms apart routinely **[M]**: use 20 ms. |
| Rise (attack) time | 25 % of the rise time | Weber fraction ~0.25, lowest near 80 ms, higher below 20 ms; studies range 6 % to > 100 % **[S]** [vHvdB79]. |
| Note length | 10 % (filled, 15–20 % under 400 ms) | Duration discrimination **[S]** [Abe72] |
| Decay / RT | 5 % | ISO 3382 limen for EDT, T20, T30 **[T]** [Odeon13]; from Seraphim's 4 % **[S]** [Sal]; studio music 0.042 s for RT < 0.6 s **[S]** [ND02]; other studies 3–26 % **[S]**. EDT's 5 % never validated **[S]** [PSU-EDT]. |
| Clarity C80, C50 | 1 dB (ISO); ~3 dB for music | **[T]** [Odeon13]; ~3 dB for concert-hall music **[S]** [PSU-C80] |
| DRR | 2–3 dB near 0 and +10 dB, much more at −10 and +20 dB | Larsen et al. 2008; the cue is mainly spectral **[S]** [Lar08]; Zahorik 2002: 5–6 dB **[S]** |
| Interchannel coherence | 0.075 (cited, untraced) | IACC limen 0.075 is widely cited but not traced to a primary source **[S]** [DAFx24]; ~10× smaller near ρ = 1 than near 0 **[S]** [Old] |
| AM / beating depth | 2–5 % modulation depth | detection within a critical band, rate up to 100 Hz **[S]** [arXiv25] |
| Roughness | 0.07 asper (absolute threshold); a 10 % change **[E]** | 1 asper = 1 kHz 60 dB tone, 100 % AM at 70 Hz **[S]** [Sim] |
| Vibrato rate, depth | ~10 % rate, ~20 % depth **[E]** | No numbers found; McAdams & Rodet 1988 measured width discrimination **[S]** [McAR88] |
| Velocity-curve slope | 1 dB at the ends of the range | the level JND applied to v16 and v127 **[E]** |
| Noise floor, clicks | audible = above threshold and unmasked | not a JND; use the masked threshold (5.2) **[E]** |

Pitch and timing JNDs also scale with the passage. A fast run hides 10 ms, and a sustained chord
shows 3 c beats. Where the JND depends on the IOI, frequency or level, the tool uses the value
for that note's context.

### 5.2 Masking: does the difference reach the listener in the mix?

A difference in one stem is heard only where that stem is audible in the mix. Per stem, per
200 ms frame, per ERB band, compute the stem's excitation and that of the rest of the mix (mix −
stem). The **partial loudness** of the stem in the presence of the rest (Glasberg & Moore 2005) is the right quantity **[S]** [GM05]. A cheaper proxy that is enough
for ranking **[E]**: the stem's band-to-rest ratio SMR(b, t) = E_stem − E_rest in dB; a
difference in band b at time t counts with weight

    w_mask(b, t) = 1 / (1 + 10^(−(SMR(b,t) + 6) / 10))

(≈1 where the stem is 6 dB or more above the rest, ≈0 when 10 dB under; a smooth version of
"under the masked threshold" with a −6 dB offset for a tonal masker **[E]**), multiplied by
whether the band is above the hearing threshold at a nominal playback level (map the mix's
integrated loudness to 80 dB SPL; ISO 226 threshold) **[E]**.

Use the *reference* synth's rest-of-mix as the masker, or the minimum of the two synths' masking,
so a difference is not hidden only because one synth's other instruments are louder **[E]**.

### 5.3 From a difference to a ranked score

For each feature f, for each unit u (a note, a band of a stem over a passage):

    z(u, f) = |Δ(u, f)| / JND(f, context)

with the JND's context (level, frequency, duration). Then weight:

    s(u, f) = z(u, f)^p · w_mask(u) · w_loud(u) · dur(u)

- w_loud: the passage's short-term loudness in sones, normalised to the piece's median (a
  difference in a loud passage is noticed more; a pp passage in a dark room is also listened to
  closely, so use loudness^0.5) **[E]**;
- dur: the time over which the difference persists (one note, or a stem's whole duration);
- p = 2 with Minkowski pooling over time and bands (√Σ s), as in PEAQ- and PEMO-Q-style models
  that pool with exponents of 2–4 **[M]**; differences under 1 JND are clipped toward zero
  (z ← max(0, z − 0.5)) so a sea of sub-threshold noise does not add up **[E]**.

A **difference item** in the report groups contiguous units with the same sign of Δ in the same
feature, stem, and key/velocity bin; its score is Σ s; the ranking sorts items by score.

### 5.4 Score per instrument and overall

Per stem: S_stem = √(Σ_items s²) mapped to a 0–100 "similarity" 100·exp(−S_stem / S0), with S0
calibrated so that two SoundFonts' renders of a typical piece score about 50 **[E]**. Overall:
the loudness-weighted mean of the stem scores, plus a separate mix-only term (balance, room).
The calibration against the SoundFont spread makes the number interpretable: "pajupilli's bass is
as far from Jnsgm2 as FluidR3 is", or "three times as far".

With a third opinion (several SoundFonts, or recordings) each item also reports **who is the
outlier**: for each feature, the median of the references and their spread (MAD); pajupilli's
value as z_ref = (x_pj − median_ref) / max(MAD_ref, JND). An item with |z_ref| < 1 is "within the
references' disagreement" and goes to the bottom of the list.

### 5.5 Existing perceptual metrics and their fit

| Metric | What it computes | Needs | Fit here |
|---|---|---|---|
| **PEAQ** (ITU-R BS.1387) | ear model (FFT; Advanced: also a filterbank) → model output variables (MOVs) → small neural net → ODG from 0 (imperceptible) to −4 (very annoying) **[S]** [PEAQ-W]; Basic has 11 MOVs **[S]** [arXiv22] | a time-aligned reference and a coded copy of the same signal; open implementations are not conformant **[S]** | Poor. Built for codec artefacts at small differences. Two synths sit at the bottom of its scale. |
| **PEMO-Q** (Huber & Kollmeier 2006) | Dau model internal representation; PSM is a cross-correlation; PSMt is the 5th percentile over time **[S]** [HK06] | full reference, aligned | Poor: one of the worst on music in a 2022 comparison **[S]** [arXiv22] |
| **ViSQOL v3** (Chinen et al. 2020) | gammatone spectrogram (ERB bands; 80 ms windows, 20 ms hop), 0.5 s patches aligned globally and per patch, NSIM, then SVR → MOS 1–5 **[T]** [ViSQOL] | 48 kHz; "presumes a clean reference" of equal or higher quality; 3–10 s clips; single scores not meaningful, aggregate them; authors advise against regression use without retraining **[T]** [ViSQOL, ViSQOL-gh] | A per-stem similarity number with its own alignment. Asymmetric (swap A and B, the number changes). Says nothing about *what* differs. |
| **HAAQI** (Kates & Arehart 2016) | auditory periphery with hearing loss, fitted to 100 hearing-aid conditions **[S]** [HAAQI] | full reference | Built for hearing-aid distortion of the same music. Not for two instruments. |
| **Zimtohrli** (Google) | 128-band complex gammatone filterbank 24 Hz–19.7 kHz, log loudness with threshold bias, **DTW alignment**, modified NSIM over 8 frames × 5 channels; distance 0 … 1 **[T]** [Zimt] | float 48 kHz; Apache-2.0; Python and Go wrappers in the repository, no PyPI package **[T]** | Symmetric, aligns itself, perceptual bands. But it was tuned only to tell very small differences between nearly equal signals (just-noticeable differences, codec range); how its distance relates to the similarity of clearly different sounds (two synths' pianos) is not known (the author's note). So it goes into the panel (4.10) as one measure with a modest weight, not as the verdict, and is only a weak check that the feature set misses nothing big: a large distance with no large feature item *may* mean a missing feature **[E]**. |
| **FAD** (Kilgour et al. 2019) | Fréchet distance between Gaussians fitted to two *sets* of embeddings; correlation with human ratings 0.52 (SDR 0.39) **[S]** [FAD] | no reference; many seconds of audio per set; biased by sample size | Per instrument across a corpus ("pajupilli's pianos vs Jnsgm2's"), not per note. |
| **FAD, better embeddings** (Gui et al. 2024) | CLAP / LAION-CLAP best overall, EnCodec for acoustic quality, MERT layer 4 for musical quality, **VGGish near chance**; extrapolation to infinite N; per-song FAD to find outliers **[T]** [Gui24] | `fadtk` (MIT) with clap, encodec, MERT, vggish, dac, …; `--indiv` per song **[T]** [fadtk] | Same. If used, CLAP or EnCodec, never VGGish. Needs torch (not installed). |
| **CLAP / OpenL3 / PANNs distances** | cosine distance of clip embeddings; LAION-CLAP 48 kHz, clips < 10 s **[T]** [CLAP]; OpenL3 MIT code, CC BY weights **[T]** [OpenL3]; PANNs AudioSet-trained, 0.439 mAP **[S]** [PANNs] | no reference | Semantic ("is it a piano?") more than fine timbre. A sanity check for a wrong program choice, not for voicing. |

**When neither is ground truth.** Every full-reference metric scores the second signal as a
degraded copy of the first. With two synths, two things go wrong. The number is asymmetric:
ViSQOL's silence thresholds and voice-activity gates apply to the reference **[T]**. And the
number cannot say who is wrong. Used symmetrically (both orders, or a symmetric distance such as
Zimtohrli's), it measures "how far apart" (though none of them is validated for large differences; Zimtohrli was tuned only near the threshold) and is calibrated by the spread among the references
(5.4). The only way to say "closer to right" is a third opinion, as in 5.4. Set metrics (FAD)
need no reference but compare distributions, so a perfect FAD between two synths would still
allow every note to differ.

**What the tool takes from them.** The ERB/gammatone front end and NSIM-style local similarity
(ViSQOL, Zimtohrli), Minkowski pooling over time (PEAQ, PEMO-Q), and percentile pooling (PSMt's 5th
percentile: the worst moments matter). Zimtohrli per stem and per note enters the panel of 4.10 as one more measure, with a modest weight: like the others it is scaled by the references' spread, and it counts as one voice among many rather than as the verdict, since its distance is validated only near the threshold. The ranking rests on the panel as a whole: many measures, each with a known meaning, each scaled by the references' spread.

## 6. The report

### 6.1 Ranked differences

A table with the top 20 items, e.g.:

| # | Where | What | Δ | JND | z | Score | Nearer the references |
|---|---|---|---|---|---|---|---|
| 1 | ch 1 piano, all keys, all v | spectral envelope 2.5–10 kHz | −25…−40 dB | ~2 dB | >10 | … | SoundFonts agree within 6 dB: pajupilli outlier |
| 2 | ch 2 bass, v 1–127 | velocity range v16→127 | 8.3 vs 4.1–4.9 dB | 1 dB | 3.5 | … | pajupilli outlier |

(numbers from appendix A). Each row links to its evidence: a plot and two short audio excerpts
(the passage on each synth, loudness-matched, 3–5 s) so the person fixing it can listen.

### 6.2 Per-instrument pages

For each stem: score; LUFS and balance; velocity curve (level vs velocity, both synths and the
references, one point per note); level, centroid, delay, T60 and f0 vs key (evenness plots);
ERB-band envelope difference as a heat map over key (rows: octave bins, columns: ERB bands,
colour: dB difference); the attack envelope overlay of a typical note per octave; a spectrogram
pair (log frequency, 100 dB range, same colour scale) of the worst passage.

### 6.3 Format

A single HTML file with embedded PNGs (matplotlib, 3.10 installed **[E]**) and `<audio>` elements
pointing at the excerpts, plus a JSON file with all the per-note tables and items (for scripted
regressions: "did my change reduce item 3?"). Markdown plus PNGs as a fallback. The JSON is the
interface for a loop that tries a parameter change, re-renders, and checks the score.

## 7. `tools/synthcompare.py`: design

### 7.1 CLI

    tools/synthcompare.py song.mid                              # pajupilli vs TiMidity+Jnsgm2
    tools/synthcompare.py song.mid --b timidity:FluidR3_GM.sf2  # another SoundFont
    tools/synthcompare.py song.mid --ref timidity:Jnsgm2.sf2,timidity:FluidR3_GM.sf2,timidity:TimGM6mb.sf2
    tools/synthcompare.py song.mid --b wav:recording.wav        # hardware or anything recorded
    tools/synthcompare.py song.mid --until 60 --channels 1,2 --out report/
    tools/synthcompare.py --diag piano                          # diagnostic MIDI (7.4)

`--a`/`--b` name a renderer: `pajupilli[:ARGS]` (default `-director off`), `timidity:SF2`, `wav:FILE`
(already rendered or recorded: mix only, or `wav:DIR` with `mix.wav` and `chN.wav`), `cmd:TEMPLATE`
(any command with `{mid}` and `{wav}`: FluidSynth later, `fluidsynth -ni -F {wav} -r 48000 sf2
{mid}` **[M]**). `--ref` adds third opinions. `--until S` passes `PJ_PLAY_UNTIL` and crops the
others.

### 7.2 Stages and data

1. **Parse MIDI** → `Note(ch, key, vel, on_s, off_s, program, bar, beat, pedal)` list; tempo map;
   controllers per channel. (Self-contained SMF reader; no `mido` installed **[E]**.)
2. **Write filtered MIDI** per channel with notes; skip channels with no notes in the range.
3. **Render** in parallel: each renderer × (mix + stems) → float32 48 kHz stereo; cache by hash
   of (renderer, args, MIDI bytes) in `~/.cache/synthcompare/`.
4. **Normalise** gains (2.3), parse pajupilli's printed scalings, verify additivity.
5. **Align** each render to the MIDI (3.1): global offset, drift, per-note onsets.
6. **Features**: per note (a `NoteFeatures` row: delay, level, rise, attack-noise, ERB envelope
   at 0–30 ms and 30–300 ms, centroid, f0, vibrato, T60, length, ILD, ICC) and per stem (LUFS,
   balance, LTAS, noise floor, clicks, LRA, RT60).
7. **Masking weights** per stem frame and band from the mix (5.2).
8. **Differences** in JND units, pooled into items (5.3); reference cloud if present.
9. **Report**: HTML + PNG + audio excerpts + JSON.

### 7.3 Run time

Measured in the proof of concept **[E]**: pajupilli rendered 20 s of the piano+bass mix in 18 s,
the piano stem in 15 s, the bass stem in 5 s (one machine, threads as default). TiMidity renders
the full 5-min file per stem in 1–5 s. Analysis of 19 s × 5 renders × 3 stems in numpy took a
few seconds. A 5-minute piece with 8 channels: pajupilli is the cost (mix + 8 stems ≈ 9 renders,
roughly 2–5 min each at 1–3× real time: tens of minutes on one machine; stems in parallel),
analysis ≈ 1 min. Hence the cache and `--until`.

### 7.4 Diagnostic MIDI

Real songs confound everything. A generated file per GM program gives clean per-note data:
every third key across the instrument's range at velocities 32, 64, 96, 127, each 1.5 s on and
1.5 s silence (plus a legato run, a staccato run, a held note with pedal, a fast repeated note).
It is what `tools/levels.py` and `tools/velcurve.py` already do for pajupilli alone; the same file
on TiMidity gives the comparison curves directly.

## 8. Hardware synthesizers

- **Path:** MIDI out (ALSA sequencer, `aplaymidi -p PORT file.mid` **[M]**) to the synth, its
  audio out to the interface, recorded with `arecord -f S24_LE -r 48000 -c 2` or through
  PipeWire **[M]**. Play the filtered MIDI per channel and the mix, one after another, each with a
  lead-in click.
- **Latency calibration:** put a marker in every file: two seconds of silence, a single loud short
  note on a dedicated channel (a percussive program, or a sysex-free "test" key) at t = 0, then
  the song after 2 s. The marker's onset in the recording gives the offset (≈1 ms accuracy for a
  click-like sound); a second marker at the end gives the drift. Better still: record the
  computer's own output (a click sent at the same MIDI time, through a loopback cable on the
  interface's second input) to measure the MIDI path's jitter (USB MIDI: of order 1 ms **[M]**).
- **Drift:** interface and synth clocks differ by tens of ppm (consumer ±100 ppm, professional ≤ 10 ppm **[S]** [comp.dsp, GlobalSpec]); measured by the two markers
  or by the windowed cross-correlation of section 3.1, removed by resampling.
- **Noise:** record 10 s of silence before; its spectrum is the floor to subtract from "noise"
  features and its hum lines are excluded.
- **Program and bank:** many hardware synths' GM mode differs from their native banks; send a GM
  reset (F0 7E 7F 09 01 F7) first.
- **Repeatability:** some synths are not deterministic (random sample start, LFO phase): render
  twice and use the twice-rendered difference as the floor ("noise of the measurement") per
  feature. TiMidity was bit-exact on re-render **[E]**.

## 9. Pitfalls

- **Different GM instruments.** pajupilli maps unbuilt programs to the nearest built instrument
  (README: guitars to koto and banjo, string sections to synth strings, choirs to pads, cymbals
  silent) **[T]**. The report must say per channel "A plays program 25 as banjo": a timbre
  difference on a substitute is not a defect. The tool reads pajupilli's `channel N: instrument`
  lines.
- **Velocity curves differ** (section 4.1): TiMidity's depends on the SoundFont's modulators and
  TiMidity's own tables. Compare level differences per velocity bin, and report the curve once
  rather than letting it appear as hundreds of "note too loud" items.
- **Reverb differs** (2.3): keep it its own line.
- **Release and sustain pedal handling** differ: TiMidity's `-EFdelay` etc. off; check CC64
  handling via "note after pedal-off rings" items.
- **Polyphony limits and voice stealing:** TiMidity default polyphony and its auto-reduce;
  pajupilli's pools ("8 at once, one voice"). A stolen note is a "dropped note" item.
- **Drums:** channel 10 is many instruments: split the stem by key (each drum key is its own
  "instrument") **[E]**.
- **Bank select / GM2:** Jnsgm2 is GM2; a file's bank select may pick variations pajupilli ignores.
- **Tuning:** pajupilli's just or spectral tuning in organs is intentional; compare to the score's
  12-TET and report, but do not count against.
- **The director and humanisation** change timing and dynamics on purpose. Compare with it off,
  and separately report what it does as interpretation.
- **Loudness of physical models vs samples:** the start of a physical-model note can take tens of
  ms to speak (bass +53 ms median in appendix A, a Jnsgm2 bass +64 ms too) **[E]**: delay is an
  attribute of the instrument and of the SoundFont's samples, not an alignment error, so the
  global offset must come from all notes of all stems, not from one instrument.

## 10. Prior art

No study was found that compares GM or SoundFont synthesizers to each other by measurement
(section 11). The nearest work:

- **DDSP** (Engel et al. 2020) evaluates resynthesis with three numbers **[T]** [DDSP]. Loudness
  L1 is taken on the A-weighted log power spectrum, standardised by the dataset. F0 L1 is in MIDI
  units (1 = a semitone), counted only where CREPE's confidence ≥ 0.85. F0 outliers are the frames
  under that confidence. Supervised DDSP scored 0.07 / 0.02 / 0.003 against WaveRNN's 0.10 / 1.00 /
  0.07 (column mapping to be checked). Its multi-scale spectral loss is L1 on linear and log
  magnitudes at FFT sizes 2048 … 64 with 75 % overlap **[T]**. These are the ancestors of this
  tool's loudness and f0 features, with frame-level rather than note-level pooling and no JND
  scaling.
- **NSynth** (Engel et al. 2017): a note dataset and a WaveNet autoencoder judged better than a
  spectral one **[S]** [NSynth]. **MIDI-DDSP** (Wu et al. 2022): quantitative experiments plus
  listening tests **[S]** [MIDIDDSP]. Neither has a per-feature perceptual comparison.
- **Wave2Midi2Wave / MAESTRO** (Hawthorne et al. 2019): note/audio alignment ~3 ms **[S]**. Its
  listening test played 20 s clips, asked "sounds more like a real piano" on a Likert scale, and
  collected 640 ratings. Real recordings and WaveNet renders were not significantly different
  after correction **[T]** [W2M2W]. This is the listening-test design to use for validating the
  tool's score on a few pieces.
- **Score-to-audio alignment** (Ewert, Müller & Grosche 2009) used chroma plus onset (DLNCO)
  features and DTW at 20 ms resolution, with MIDI distorted by ±30 % **[T]** [EMG09]. Piano onset
  error fell from 44 ms (chroma) to 19 ms (chroma + DLNCO); a string quartet gave 79–82 ms. This is
  the DTW of section 3.1 for free-timed renders. Note-level work then refines within that window.
- **MIREX score following**: a note counts as misaligned beyond 300 ms; precision is 1 − miss −
  error **[S]** [MIREX]. Far coarser than needed here.
- **Slakh2100**: 2100 Lakh MIDI songs rendered with professional sample libraries (Kontakt),
  145 h **[T]** [Slakh]. A related source chose SoundFonts by MFCC similarity to the original stem
  and notes that many SoundFonts do not map note to pitch exactly **[S]** [arXiv1809]. A f0 check
  per note (4.5) is therefore needed even on the references.
- **A listening study of MIDI devices**: 4 excerpts × 16 devices (sound cards, 3 hardware and 5
  software synths) **[S]** [MJCS]; results not seen.
- **Sound matching**: InverSynth predicts synth parameters from spectrograms with CNNs
  **[S]** [InverSynth]. Like DDSP's spectral loss it is an objective for an optimiser, a possible
  later use of this tool's score.
- **Timbre perception**: McAdams et al. 1995 found a 3-D timbre space whose axes correlate with log
  attack time, spectral centroid and (weakly) spectral flux **[S]** [McA95]. With the three varied
  independently, Caclin et al. found 2-D enough: flux is the weakest **[S]** [Cac05]. Hence attack
  and centroid are first-class features here.
- **Timbre Toolbox** (Peeters et al. 2011): descriptors from the STFT, harmonic, auditory-model and
  energy-envelope representations, summarised by median and IQR. About 10 independent groups,
  found on 6000+ McGill samples **[S]** [TT11]. Its definitions (log attack time, centroid,
  spectral slope, harmonic energy ratio, noisiness) are the ones to copy into section 4.
- **Loudness models**: EBU R128 and LRA (Tech 3342: 95th minus 10th percentile of 3 s loudness,
  −20 LU relative gate) **[S]** [R128, T3342]. ISO 532-1 (Zwicker, stationary and time-varying)
  and ISO 532-2 (Moore–Glasberg, stationary) can disagree **[S]** [ISO532]. The time-varying
  Moore–Glasberg–Schlittenlacher model is now ISO 532-3:2023 **[S]**. Glasberg & Moore 2005's
  partial loudness in a background is the masking model of 5.2 **[S]** [GM05]. In Python,
  `pyloudnorm` covers BS.1770-4 and an LRA attempt (MIT) **[T]** [pyln], and `mosqito` has
  validated ISO 532-1, DIN 45692 sharpness and Daniel & Weber roughness, with Moore–Glasberg
  "to do" (Apache-2.0) **[T]** [MOSQITO]. Neither is installed; both are numpy/scipy only.

## 11. Not found

- A measured comparison of GM/SoundFont synthesizers with each other, or of a SoundFont instrument
  with recordings of the real instrument, with numbers per feature.
- A numeric JND for spectral centroid or brightness in percent (Kendall & Carterette not found).
  The 4 % used in 5.1 is a placeholder.
- Vibrato rate and depth JNDs in Hz and cents.
- The ISO 3382-1 Annex A table itself. Seen: EDT, T20, T30 5 %; C50, C80 1 dB; D50 0.05; Ts 10 ms.
  The IACC 0.075 is untraced.
- The exact intensity JNDs of Jesteadt, Wier & Green 1977 and the frequency DLs of Wier et al. 1977
  in Hz.
- Glasberg & Moore 2002's short- and long-term loudness time constants (in ISO 532-3).
- PEAQ's MOV counts in the primary text (Kabal 2002's report would settle it).
- A paper on measuring the latency of a MIDI-out to audio-in loop, and a peer-reviewed survey of
  audio-interface clock accuracy. Forum and vendor pages give ±100 ppm for consumer cards,
  measured 5–100 ppm, and ≤ 10 ppm for professional gear **[S]** [comp.dsp, baudline, GlobalSpec].
  At 100 ppm a 5-minute piece drifts 30 ms **[E]**.
- TiMidity++'s velocity → amplitude curve for SF2 instruments and the cause of its −27 dB
  non-additivity (not looked up in its source).
- Zimtohrli on PyPI.

## 12. References

Local: `README.md` ("-play", "Offline"), `src/midifile.cc` (peak scaling, `PJ_PLAY_*`),
`/etc/timidity/timidity.cfg`, `/etc/timidity/fluidr3_gm.cfg`, `timidity -h` (TiMidity++ 2.14.0),
`tools/levels.py`.

- [Abe72] Abel 1972, duration discrimination, via https://link.springer.com/content/pdf/10.3758/BF03199358.pdf
- [arXiv1809] SoundFont selection by MFCC: https://arxiv.org/pdf/1809.00381
- [arXiv22] Evaluation of PEAQ, PEMO-Q and others on music (2022): https://arxiv.org/pdf/2212.01467
- [arXiv25] AM detection: https://arxiv.org/pdf/2510.14159
- [baudline] Sample-rate measurement: https://baudline.com/solutions/sample_rate/index.html
- [Bea09] Beauchamp, AUDITORY list 2009: https://auditory.org/postings/2009/74.html
- [bioRxiv23] Profile analysis: https://www.biorxiv.org/content/10.1101/2023.11.30.569471.full.pdf
- [BS1770] ITU-R BS.1770: https://www.itu.int/rec/R-REC-BS.1770 (not read; gating as in [R128])
- [Cac05] Caclin et al.: https://pmc.ncbi.nlm.nih.gov/articles/PMC3107380
- [CLAP] LAION-CLAP: https://github.com/LAION-AI/CLAP
- [comp.dsp] https://www.dsprelated.com/showthread/comp.dsp/32386-1.php
- [DAFx24] IACC JND discussion: https://dafx.de/paper-archive/2024/papers/DAFx24_paper_38.pdf
- [DDSP] Engel et al. 2020: https://arxiv.org/pdf/2001.04643
- [EMG09] Ewert, Müller, Grosche 2009: https://www.audiolabs-erlangen.com/content/resources/MIR/SyncRWC60/2009_EwertMuellerGrosche_HighResAudioSync_ICASSP.pdf
- [ERB] https://en.wikipedia.org/wiki/Equivalent_rectangular_bandwidth ; [Bark] https://en.wikipedia.org/wiki/Bark_scale
- [FAD] Kilgour et al. 2019: https://arxiv.org/abs/1812.08466
- [fadtk] https://github.com/microsoft/fadtk
- [FS95] Friberg & Sundberg 1995: https://typeset.io/pdf/time-discrimination-in-a-monotonic-isochronous-sequence-ynslrz5yhj.pdf
- [GlobalSpec] https://www.globalspec.com/reference/32697/203279/chapter-6-practical-audio-interfacing
- [GM05] Glasberg & Moore 2005: https://www.aes.org/e-lib/browse.cfm?elib=13391 ; 2002 model via https://www.frontiersin.org/articles/10.3389/fnins.2016.00183/full
- [Goc07] Gockel et al. 2007, via [Bea09]
- [Gre92] Green 1992: https://auditory.org/asamtgs/asa92nwo/1pPP/1pPP2.html
- [Gui24] Gui et al. 2024: https://arxiv.org/html/2311.01616
- [HAAQI] Kates & Arehart 2016: https://pmc.ncbi.nlm.nih.gov/articles/PMC4849486
- [Hir59] Hirsh 1959, via https://www.thieme-connect.de/products/ejournals/abstract/10.1055/s-2004-832856
- [HK06] Huber & Kollmeier 2006 (PEMO-Q): https://data.zhenzhi.com/details/1/901928382
- [InverSynth] https://arxiv.org/abs/1812.06349
- [ISO532] ISO 532-1:2017: https://standards.iteh.ai/catalog/standards/iso/4b1ae786-7030-41f2-a9c4-da3f3db639be/iso-532-1-2017 ; ISO 532-3:2023: https://iteh.es/catalog/standards/iso/9071eb5a-01ef-4f5c-8690-3c4d8c837568/iso-532-3-2023
- [JWG77] Jesteadt, Wier, Green 1977: https://scilit.com/publications/0f2a2362b3ed36eac542644f3b2826c5 ; Florentine 1983: https://scilit.com/publications/a727947581e2b39e3f5cea4da5f3522f
- [Lar08] Larsen et al. 2008 (DRR): https://pmc.ncbi.nlm.nih.gov/articles/PMC2677334
- [McA95] McAdams et al. 1995: https://three-mode.leidenuniv.nl/pdf/m/mcadams_etal1995pr.pdf
- [McAR88] McAdams & Rodet 1988: https://www.mcgill.ca/mpcl/files/mpcl/mcadams_1988_basicissueshearing.pdf
- [McD10] McDermott et al. 2010: https://mcdermottlab.mit.edu/papers/McDermott_etal_2010_intervals.pdf
- [Mic06] Micheyl et al. 2006, via https://link.springer.com/article/10.1007/s10162-015-0548-2
- [MIDIDDSP] https://arxiv.org/abs/2112.09312
- [MIREX] score-following evaluation: https://arxiv.org/pdf/2205.03247
- [MJCS] MIDI device listening study: https://adum.um.edu.my/index.php/MJCS/article/download/5784/3513/12834
- [Moo74] Moore 1974, via https://ccrma.stanford.edu/~malcolm/correlograms/text/33%20Frequency%20Difference%20Limen%20Or%20Jnd.html
- [MOSQITO] https://mosqito.readthedocs.io/en/latest/source/user_guide/scope.html
- [ND02] Niaounakis & Davies 2002: https://www.ioa.org.uk/system/files/proceedings/tj_ninounakis_wj_davies_the_perception_of_small_changes_in_reverberation_time_within_recording_studio_rooms.pdf
- [NSynth] https://arxiv.org/abs/1704.01279
- [Odeon13] ISO 3382 parameters (ISRA 2013), Table 1: https://odeon.dk/pdf/ISRA2013_Paper_The_ISO_3382_parameters_Can_we_simulate_them_Can_we_measure_them.pdf
- [Old] IACC JND vs reference: https://oops.uni-oldenburg.de/id/eprint/3278
- [OpenL3] https://github.com/marl/openl3
- [PANNs] https://arxiv.org/abs/1912.10211
- [PEAQ-W] https://en.wikipedia.org/wiki/Perceptual_Evaluation_of_Audio_Quality
- [PSU-C80] https://pure.psu.edu/en/publications/the-effects-of-different-test-methods-on-the-just-noticeable-diff/
- [PSU-EDT] https://pure.psu.edu/en/publications/a-study-of-the-just-noticeable-difference-of-early-decay-time-edt/
- [pyln] pyloudnorm: https://github.com/csteinmetz1/pyloudnorm
- [R128] https://en.wikipedia.org/wiki/EBU_R_128 ; [T3342] https://tech.ebu.ch/docs/tech/tech3342.pdf
- [Rab76 via JASA79] https://people.bu.edu/teich/pdfs/JASA-66-1738-1979.pdf
- [Sal] Salford RT JND: https://salford-repository.worktribe.com/OutputFile/1491017
- [Sim] Roughness test signals: https://www.ioa.org.uk/system/files/proceedings/d_simmons_test_signals_for_validating_sound_quality_measurement_instrumentation.pdf
- [Slakh] https://github.com/ethman/slakh-utils
- [TT11] Peeters et al. 2011: https://www.mcgill.ca/mpcl/files/mpcl/peeters_2011_jasa.pdf
- [vHvdB79] rise-time JND, via https://scholarlypublications.universiteitleiden.nl/access/item%3A2723023/view
- [ViSQOL] Chinen et al. 2020: https://arxiv.org/pdf/2004.09584 ; [ViSQOL-gh] https://github.com/google/visqol
- [W2M2W] Hawthorne et al. 2019: https://arxiv.org/pdf/1810.12247v3
- [Zimt] https://github.com/google/zimtohrli

---

## Appendix A. Proof of concept (scratchpad, not in the repository)

All numbers in this appendix are **[E]**: measured here, once, on one excerpt. Scripts in
`/tmp/claude-1000/-home-jyrki-pajupilli/ebe9e41e-83f7-48d8-b6c1-58d4fa6c7e29/scratchpad/poc/`
(`smf.py` MIDI reader and filter, `ana_lib.py` + `ana2.py` features, `f0.py` pitch).

**Material.** `~/midi/Anighting.mid` ("A Nightingale Sang in Berkeley Square"): channel 1 piano
(program 0; pajupilli: `grand-piano`, "8 at once, one voice"), channel 2 bass (programs 32 then 35;
pajupilli: `double-bass`). First 19 s: 138 piano notes (v 15–99), 37 bass notes (v 1–127). Only
CC64 and bank select; no CC7/CC11.

**Renders.** Mix and two stems (filtered MIDI, section 2.1) on:
- `pj`: `PJ_PLAY_UNTIL=20 ./pajupilli -director off -play X.mid -o X.wav` (float32, 48 kHz; no peak
  scaling printed, so no gain to undo);
- `jns`, `fl`, `tg`: TiMidity, `-s 48000 -OwS2 -EFreverb=d -EFchorus=d`, a full per-program
  config (section 2.2) without `amp=` for Jnsgm2, FluidR3_GM, TimGM6mb;
- `fl_deb`: TiMidity with Debian's default config (FluidR3 with its `amp=` trims).

**Additivity** (mix − piano − bass, re mix): pj −151 dB; TiMidity −27 dB.

**Alignment.** Onset-strength cross-correlation, 5 ms hop: TiMidity renders lie 5 (jns) to 10 ms
(the others) earlier than pajupilli. Per-note delays were measured against the MIDI directly.

**Loudness** (gated BS.1770, 0–19 s, LUFS):

| | mix | piano | bass | piano − bass |
|---|---|---|---|---|
| pj | −19.0 | −20.7 | −23.0 | +2.3 LU |
| jns | −25.3 | −27.1 | −30.2 | +3.1 |
| fl | −23.7 | −28.0 | −25.6 | −2.4 |
| fl_deb | −24.0 | −33.7 | −24.3 | −9.4 |
| tg | −27.2 | −31.8 | −28.9 | −2.9 |

The mix levels differ by 8 dB (mastering: normalised away). The balance is the comparison: pj
+2.3 LU lies inside the three SoundFonts' range (−2.9 … +3.1). **Not a finding.** Debian's
`amp=` trims alone move FluidR3 by 7 LU: had the default config been the reference, pajupilli's
balance would have looked 11.7 LU wrong.

**Spectral envelope** (LTAS in 1-ERB bands, each stem normalised to its own total; dB):

| piano, band centre | 162 | 312 | 518 | 803 | 1196 | 1739 | 2488 | 3524 | 4953 | 6928 | 9654 Hz |
|---|---|---|---|---|---|---|---|---|---|---|---|
| pj | −21 | −6 | −15 | −15 | −27 | −28 | −40 | −56 | −66 | −75 | −81 |
| jns | −19 | −11 | −10 | −13 | −16 | −21 | −23 | −26 | −31 | −37 | −45 |
| fl | −13 | −8 | −15 | −23 | −18 | −19 | −19 | −24 | −27 | −32 | −40 |
| tg | −17 | −11 | −10 | −15 | −14 | −19 | −19 | −31 | −45 | −74 | −82 |

Energy-weighted rms distance between stems (dB) and centroid ratio (cents):

| pair | piano | bass | mix |
|---|---|---|---|
| pj − jns | 5.8 dB, −501 c | 4.4 dB, −565 c | 4.2 dB |
| pj − fl | 8.2, −429 | 3.6, −264 | 4.5 |
| pj − tg | 5.8, −638 | 6.4, −1298 | 4.1 |
| jns − fl | 3.8, +72 | 2.2, +301 | 3.8 |
| jns − tg | 2.2, −138 | 3.4, −733 | 2.4 |
| fl − tg | 4.8, −209 | 3.4, −1034 | 3.6 |
| fl − fl_deb | 0.2, −15 | 0.0, 0 | 0.6 |

**The largest difference found:** pajupilli's piano is 15–35 dB darker than all three SoundFonts
from 1.2 to 5 kHz, and its centroid is 4.3–6.4 semitones lower, while the SoundFonts agree
within ~6 dB there (TimGM6mb's own roll-off starts only above 3.5 kHz). A single C4 at velocity 80
through `-play` showed the same (pj −50 dB at the 2.2 kHz band and −58 at 3.5 kHz re the total,
Jnsgm2 −22 and −28). With an ERB-band JND of ~1–2 dB **[M]** this is > 10 JND over a third of the
audible bands, in a stem that is never masked in this piece: rank 1. Whether pajupilli or the
samples are right needs a recorded grand piano (the SoundFonts' pianos are recordings, but close-
miked and possibly brightened); the measured direction is clear.

**Velocity → level** (isolated notes, K-weighted level of the first 160 ms, fitted to log v):

| | piano slope (v16→127) | piano scatter | bass slope (v16→127) | bass scatter |
|---|---|---|---|---|
| pj | 13.2 dB | 3.1 dB | 8.3 dB | 3.9 (3.1 after key trend) |
| jns | 14.7 | 2.1 | 4.9 | 3.7 |
| fl | 15.9 | 2.1 | 4.1 | 4.7 |
| tg | 17.9 | 2.3 | 4.2 | 4.2 |

Second finding: the bass's dynamic range across velocity is about twice the SoundFonts' (8.3 vs
4.1–4.9 dB from v16 to v127; ~3–4 JND at the extremes). Third: the piano's note-to-note scatter is
1 dB more than any SoundFont's (3.1 vs 2.0–2.3 dB): an evenness item; the per-note table shows
which keys.

**Onset delay re MIDI** (isolated notes; median and interquartile range, ms):

| | piano | bass |
|---|---|---|
| pj | +12 [−9, +15] (13 of 34 found) | +53 [+24, +96] (30/37) |
| jns | +22 [+16, +102] | +64 [+12, +96] |
| fl | +59 [+3, +90] | +6 [0, +29] |
| tg | +4 [0, +59] | +14 [0, +64] |

The pajupilli bass speaks ~50 ms after the MIDI onset; so does the Jnsgm2 bass, while FluidR3's
speaks at once: the references disagree, so it is not ranked. The detector (maximum slope of the
1 ms log envelope) is too crude for the piano (13 of 34 found for pj, wide IQRs from the pedal's
ringing): the harmonic-comb detector of section 3.1 is needed.

**Bass f0** (21 notes, 120–600 ms of each, cents from 12-TET A440): pj median +0.4 c, IQR −4.3 …
+1.8, 33 % of notes more than 5 c from the median; jns −3.5 c (IQR ±0.5), fl −0.2 (±0.35), tg
+1.7 (±0.5), none of the SoundFonts' notes off by 5 c. pajupilli's bass notes scatter by a few
cents (a plucked physical string's tension pitch glide, or its tuning), at about the pitch JND:
a low-rank item, to check by ear on held notes.

**What the PoC shows about the method.**
- The reference cloud is essential: two of the five raw differences (balance, bass delay) vanish
  once three SoundFonts are compared, and Debian's `amp=` alone would have made the balance the
  top item.
- Stems by filtered MIDI work for both synths, exactly additive for pajupilli.
- A ranked list from these numbers, with JNDs as in section 5.1: (1) piano high-frequency
  envelope, (2) bass velocity range, (3) piano evenness, (4) bass pitch scatter; not ranked:
  balance, bass delay (inside the references' spread).
