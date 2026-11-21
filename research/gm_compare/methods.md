## Methodology and caveats

### What was played

- **The probe** is `tools/synthcompare.py`'s profile probe, unchanged: per GM program, keys a few
  semitones apart over the program's GM range (at most 16 keys; `gm_range` in synthcompare), each
  at velocities 40, 64, 96 and 127, held 2 s in a 4 s slot. The drum kit plays every key 35–81 the
  same way on channel 10.
- **The phrase** (new in `tools/gmcompare.py`, melodic programs only): six notes up a major scale
  at velocity 80, 0.4 s apart, each held 20 ms into the next (legato), then 1 s of rest and four
  100 ms staccato notes 0.4 s apart on the same key. Measured: onset re the note-on (the steepest
  rise of the 1 ms envelope), its jitter, the level evenness of the six legato notes, the dip at
  each legato change, and how long a 100 ms note sounds (to 20 dB below its peak, at most 400 ms).
- **pajupilli**: `-play` with `-director off`, `PJ_PLAY_VELMEDIAN=1000` (no velocity rescaling),
  `PJ_THREADS=1`; its whole-file peak scaling (the "N dB down" line) undone. The binary was copied
  to the scratchpad first (other sessions rebuild `./pajupilli`); its commit and uncommitted diff
  are recorded below and in `research/gm_compare/data/_build.json`.
- **The four references** (the "reference cloud"):
  - **FluidR3_GM** (`/usr/share/sounds/sf2/FluidR3_GM.sf2`, "Fluid R3 GM", Frank Wen / Toby Smithe,
    2008-02-24, SF 2.1, 148 MB; Debian/Ubuntu package fluid-soundfont-gm 3.1-5.3) through TiMidity;
  - **TimGM6mb** (`/usr/share/sounds/sf2/TimGM6mb.sf2`, "TimGM6mb1", SF 2.1, 6 MB; package
    timgm6mb-soundfont 1.3-5) through TiMidity;
  - **Jnsgm2** (`~/soundfonts/Jnsgm2.sf2`, "JNSGM v2.0 (32Mb)", Jordi Navarro Subirana, 1999,
    SF 2.1) through TiMidity;
  - **SGM-V2.01** (`~/soundfonts/SGM-V2.01.sf3`, David Shan 2002–2007, SF 3.1 with Ogg Vorbis
    samples, 28 MB) through **FluidSynth**: TiMidity++ 2.14 plays SF3 samples as noise
    (research/synth_compare.md 2.2).
- **TiMidity++ 2.14.0** (Ubuntu 2.14.0-8.3) at 48 kHz, 24-bit stereo, `-EFreverb=d -EFchorus=d`,
  with one generated cfg per SoundFont that maps all 128 programs and drum keys 27–87 again with
  `%font` lines and no `amp=` trims (`synthcompare.timidity_cfg`). A bare `soundfont` line would
  leave Debian's explicit FluidR3 mapping in force. Check that the mapping took: the three
  TiMidity fonts give different numbers for every instrument in this run (e.g. violin centroid
  16.5 / 15.7 / 19.9 ERB, flatness −28 / −67 / −29 dB).
- **FluidSynth 2.4.8** (libfluidsynth3 2.4.8+dfsg-1) through ctypes, 48 kHz, reverb and chorus
  off, polyphony 256, MIDI events applied sample-exactly between blocks.
- **The TiMidity check**: FluidR3, TimGM6mb and Jnsgm2 are also rendered through FluidSynth.
  Those renders are not in the cloud. Each instrument's section lists, under "TiMidity misreads",
  the fonts whose FluidSynth render differs from their TiMidity render by at least 0.5 ERB in
  brightness or attack brightness, or 4 dB rms in envelope. That flags the instruments where
  TiMidity's missing modulation-envelope filter sweeps, velocity-to-filter modulators or
  attenuation curves misrepresent what the font's author intended.
- **No loudness matching of the whole set to one level.** Every measure is relative: the level
  against the other instruments is each synth's velocity-96 level minus that synth's median over
  all melodic programs, so TiMidity's and FluidSynth's different output gains don't count. The
  spectrum is compared with level removed. Zimtohrli compares each velocity-96 note against the
  same note, both scaled to the same rms.

### How a difference is scored

For each measure (synthcompare's `SUMMARY_MEASURES` with their JNDs and weights, plus the phrase
measures), pajupilli's value is compared with the four references:

- `z = |pajupilli − median(refs)| / max(JND, half the refs' range)`; attack time, decay rate,
  release time and staccato length are compared as log ratios;
- **outside** means pajupilli lies more than half a JND beyond the refs' range;
- the score is `min(z, 8) × weight`, halved when pajupilli is inside the range.

The note-for-note measures (body spectrum shape, attack spectrum shape, envelope, spectrogram,
overtone levels, Zimtohrli) compare pajupilli's distance to the fonts with the fonts' distances to
each other: `z = median distance to the fonts / median distance between fonts`; **outside** when
even the nearest font is 1.3 times further from pajupilli than the fonts are from each other.

An instrument's **distance** is the mean of its eight largest scores. **Fonts disagree** is the
median over the measures of half the refs' range in JNDs.

### What TiMidity and the references can't tell

- **TiMidity ignores the SF2 modulation envelope's pitch and filter routing and the default
  velocity-to-filter modulator** (TiMidity applies only a static filter; even `--mod-envelope`
  does not give FluidSynth's filter sweep: research/slap_bass_2.md 1.4). TimGM6mb and Jnsgm2 use
  filter envelopes on basses, synths and some plucks, so their TiMidity renders are brighter and
  have no "wah" where the author intended one. The FluidSynth check quantifies it per instrument.
- **The SoundFonts have no room.** pajupilli plays every instrument in its own room
  (README "-room"); the fonts were rendered dry. This inflates pajupilli's release time, its
  staccato length, its noise/room floor and its stereo width, and lowers its stereo coherence,
  for nearly every instrument. Those four measures are real differences in the output but are a
  deliberate design choice, not a fault of the instrument model. The per-note timbre measures use
  the first 600 ms, where the direct sound dominates.
- **Sample-based fonts loop.** A held SoundFont note is a sample loop with an amplitude envelope,
  so its "decay" and "level after 2 s" are the author's envelope settings, sometimes a plain
  hold. A physical model that decays (plucks, struck bars) differs there by nature.
- **Velocity curves.** TiMidity's velocity-to-volume curve applies the same attenuation law to
  every font. In this run the four fonts gave nearly the same velocity curve for most melodic
  instruments (−13/−6/0/+4 dB at 40/64/96/127 under TiMidity). Their "loudness range" is mostly
  the player's law, not the font, so a narrower or wider range than the fonts is a difference
  from TiMidity's convention, not necessarily from a real instrument. Only velocity layers and
  velocity-to-filter routing are the fonts' own.
- **One key range per program.** The keys come from synthcompare's coarse GM ranges (e.g. 55–96
  for violin, 36–84 for the synth and effect programs). Where pajupilli's instrument is narrower,
  notes outside its range are silent or folded and are listed under "silent probe notes".
- **Measures see the steady part and the onset, not the playing.** Bowing changes, breath
  articulation, slides, the director's phrasing (off here) and the Leslie's motion are not
  measured, except coarsely by pitch wobble and the phrase onsets. Effects programs (97–104,
  121–128) and pitched percussion with inharmonic spectra make the pitch and overtone measures
  meaningless: there the spectrum, envelope and Zimtohrli rows are the useful ones.
- **Neither side is right.** The references span a range; the distance says how far pajupilli
  is from that range, not from the real instrument. Where pajupilli was tuned against a recording
  or the literature (research notes), that wins over the fonts.
- **Drum kit.** The drum kit is scored as one instrument over keys 35–81 (level, brightness and
  envelope per key); the per-key table under its section is the more useful view.
