# pajupilli against four SoundFonts, every General MIDI program

Status: measurement and report only (2026-11-20). No synth source was changed. pajupilli's
`-play` was compared with TiMidity++ playing FluidR3_GM, TimGM6mb and Jnsgm2, and with FluidSynth
playing SGM-V2.01. All 128 GM programs were compared, plus the drum kit. The driver is
`tools/gmcompare.py`. It renders each instrument on its own and saves its numbers to
`research/gm_compare/data/<key>.json`, where key `p000`–`p127` is the 0-based program. It deletes
the renders as it goes, then writes this file. The method and caveats are at the end of this file.

**The build measured.** `./pajupilli` was copied to the scratchpad at the start (md5
40f1eb3b86d6028fe6407018615743d3). It was built from commit e872b48 ("Jazz guitar, brass section,
fretless and slap basses; bowed bodies as modes") plus uncommitted changes. `git diff --stat` at
the time:

    Makefile          |  5 ++--
    src/bass.cc       | 78 +++++++++++++++++++++++++++++++++++++++++--------------
    src/calibrate.cc  |  8 +++++-
    src/ebass.cc      | 15 ++++++++++-
    src/ebass.h       |  2 ++
    src/fretless.cc   | 15 ++++++++++-
    src/fretless.h    |  2 ++
    src/guitar.cc     |  8 +++---
    src/guitar.h      |  5 ++++
    src/guitarinst.cc | 19 +++++++++++---
    src/guitarinst.h  |  2 ++
    src/main.cc       |  5 ++--
    src/organ.cc      |  3 ++-
    src/pianoinst.cc  |  4 +++
    src/voice.cc      | 13 ++++++++--
    src/voice.h       |  4 +++
    tools/test.py     | 78 ++++++++++++++++++++++++++++++++++++++++++++++++++-----
    17 files changed, 223 insertions(+), 43 deletions(-)

**Coverage.**
- All 128 GM programs have a built pajupilli instrument; `-inst` lists no fallbacks. All 128 were
  compared, plus the drum kit (keys 35–81) as one instrument.
- References:
  - FluidR3_GM, TimGM6mb and Jnsgm2 through TiMidity++ 2.14;
  - SGM-V2.01 through FluidSynth 2.4.8 (it is an SF3, which TiMidity can't play).
- As a check, FluidR3, TimGM6mb and Jnsgm2 were also rendered through FluidSynth. That shows where
  TiMidity misrepresents a font.
- Per instrument there are 52–64 probe notes per synth (keys × velocities 40/64/96/127, held 2 s),
  and a short phrase for the melodic programs.

**Re-running one instrument** (e.g. after a fix, with another build):

    cp pajupilli /tmp/pj_copy   # (a copy: other sessions rebuild ./pajupilli)
    tools/gmcompare.py run --bin /tmp/pj_copy --programs 45 --force    # GM 46 (0-based 45), or a name:
    tools/gmcompare.py run --bin /tmp/pj_copy --programs pizzicato-strings,trumpet,drums --force
    tools/gmcompare.py report --bin /tmp/pj_copy                        # rebuild this file

- `run` without `--force` does only the instruments that have no data yet.
- `--jobs` sets the number of parallel workers (default 8). Each worker renders one instrument
  after the other on all eight synths, single-threaded. An instrument takes about 2–3 minutes; the
  drum kit takes longer.
- `report` merges the hand-written notes in `research/gm_compare/notes.md` (one `## pNNN` section
  per instrument), the flags in `flags.json` and the texts `head.md`, `patterns.md` and
  `methods.md`, all in `research/gm_compare/`.
- The hand notes refer to this build. After a change, re-read them against the new numbers.

## Faults found and verified while writing this report

The measurements raised two faults that are not matters of taste. Both were confirmed directly on
the build measured.

1. **Under `-play`, the transposing winds and brass sound at the wrong pitch.**
   - `src/midifile.cc` (the note-on at about line 1362, and the held-key bookkeeping at 1327 and
     1333) plays `pk + inst->transpose`. `pk` is the GM key, which is already concert pitch.
   - `Instrument::transpose` is "sounding minus written". Adding it once more moves the sound by
     the transposition.
   - Measured: alto sax key 60 sounds at 155.7 Hz (D#3, −9 semitones); clarinet key 62 sounds C4
     (−2); tenor −14, baritone −21, english horn −7, horn −7, piccolo +12.
   - Affected programs:
     - 57 trumpet and 60 muted trumpet (−2);
     - 61 horn (−7);
     - 65–68 saxophones (−2/−9/−14/−21);
     - 70 english horn (−7);
     - 72 clarinet (−2);
     - 73 piccolo (+12).
   - For these programs the pitch, overtone and register-dependent numbers below compare the wrong
     notes. Re-run them after the fix: the trumpet's and the horn's places in the top 15 may be
     partly this.
2. **GM 46 Pizzicato Strings bows instead of plucking.** A held probe note is +3.6 dB louder at
   2 s than at its start, and its level after 2 s is −7 dB against the fonts' −92 … −127.
   - `BowPlayer::setPizzicatoOnly(true)` (src/bassinst.h:151) only sets `wait_ = 0`.
   - With `wait_ <= 0`, `BowPlayer::noteOn` calls `strike()`, which is a bow stroke
     (src/bassinst.cc about line 673).
   - The pluck happens only in `due()`, for keys let go while they wait.
   - This reading of the code is from the annotation pass; the sustained level is measured.

## Summary table

One row per instrument. **Distance**: the mean of the eight largest weighted differences (each in units of the larger of its JND and half the fonts' own range, capped at 8, halved when pajupilli lies inside the fonts' range), leaving out the four measures pajupilli's room moves (release, staccato length, floor, stereo coherence); about 1 is "like another SoundFont", 3 and more stands out. **Fonts disagree**: the median, over the measures, of half the four references' range in JNDs (high: the fonts themselves are far apart, so a difference there is less likely a fault). **Rank** marks the 15 furthest (the first to look at). The differences are pajupilli against the fonts' median.

| GM | instrument | pajupilli | distance | rank | fonts disagree | biggest differences |
|---|---|---|---|---|---|---|
| 1 | Acoustic Grand Piano | grand-piano | 1.77 | 113 | low (1.1) | brightens more with velocity (1.21 vs 0 ERB); less even from key to key (4.95 vs 1.77 dB); a sharper onset (28.2 vs 9.7 dB/ms) |
| 2 | Bright Acoustic Piano | bright-piano | 1.97 | 105 | low (1.1) | brightens more with velocity (1.62 vs 0 ERB); brighter (11.2 vs 10.6 ERB); a sharper onset (28.3 vs 9.55 dB/ms) |
| 3 | Electric Grand Piano | electric-grand | 2.17 | 98 | medium (1.5) | brightens more with velocity (2.16 vs 0 ERB); a sharper onset (44.6 vs 10.9 dB/ms); less even in tone from key to key (1.92 vs 0.925 ERB) |
| 4 | Honky-tonk Piano | honky-tonk | 2.66 | 75 | low (1.2) | brighter (14.3 vs 10.9 ERB); a narrower dynamic range (10.3 vs 16.7 dB); a sharper onset (37.3 vs 9.55 dB/ms) |
| 5 | Electric Piano 1 | electric-piano-1 | 2.84 | 60 | low (1.2) | brightens more with velocity (2.17 vs 0 ERB); a slower attack (29.0 vs 5 ms); less even from key to key (3.17 vs 1.23 dB) |
| 6 | Electric Piano 2 | electric-piano-2 | 3.86 | 23 | medium (1.5) | a narrower dynamic range (1.6 vs 16.6 dB); brightens more with velocity (4.2 vs 0 ERB); less even from key to key (3.38 vs 1.04 dB) |
| 7 | Harpsichord | harpsichord | 3.99 | 21 | low (1.2) | a narrower dynamic range (-2.2 vs 16.7 dB); less even from key to key (5.03 vs 0.88 dB); quieter (-6.9 vs -1.8 dB) |
| 8 | clavi | clavi | 3.73 | 28 | low (1.2) | brightens more with velocity (3.03 vs 0 ERB); less even from key to key (4.01 vs 0.945 dB); a slower attack (45.0 vs 1 ms) |
| 9 | Celesta | celesta | 2.01 | 104 | low (1.0) | less even in tone from key to key (1.11 vs 0.202 ERB); brightens more with velocity (0.84 vs 0 ERB); a narrower dynamic range (13.1 vs 16.7 dB) |
| 10 | Glockenspiel | glockenspiel | 2.30 | 90 | medium (1.6) | a slower attack (18.0 vs 1.5 ms); louder (-0.2 vs -4.2 dB); a narrower dynamic range (12.2 vs 16.6 dB) |
| 11 | Music Box | music-box | 2.67 | 70 | medium (1.9) | a narrower dynamic range (6 vs 16.6 dB); less even from key to key (3.42 vs 0.53 dB); brightens slower up the keyboard (1.77 vs 3.34 ERB/oct) |
| 12 | Vibraphone | vibraphone | 2.15 | 99 | medium (1.8) | more even partials (-34.4 vs -17.8 dB); less even from key to key (4.26 vs 0.7 dB); less even in tone from key to key (1.5 vs 0.206 ERB) |
| 13 | Marimba | marimba | 2.12 | 101 | low (0.8) | a sharper onset (59.5 vs 2.4 dB/ms); rougher (-2.42 vs -7.16 log2); a narrower dynamic range (13.1 vs 16.6 dB) |
| 14 | Xylophone | xylophone | 4.01 | 20 | low (0.5) | brighter (15.7 vs 11.0 ERB); a narrower dynamic range (9.8 vs 17.4 dB); brightens slower up the keyboard (0.2 vs 3.45 ERB/oct) |
| 15 | Tubular Bells | tubular-bells | 3.39 | 38 | medium (2.0) | darker (16.2 vs 19.8 ERB); brightens more with velocity (2.19 vs 0.005 ERB); weaker low partials (0.171 vs 0.997) |
| 16 | Dulcimer | dulcimer | 2.22 | 93 | low (1.2) | less even from key to key (2.62 vs 0.73 dB); decays slower (11.6 vs 19.9 dB/s); sustains more (-31.8 vs -46.4 dB) |
| 17 | Drawbar Organ | drawbar-organ | 2.28 | 91 | low (1.1) | a narrower dynamic range (0.7 vs 18.4 dB); louder toward the bottom (-2.98 vs -0.2 dB/oct); envelope shape (3.2x fonts' spread) |
| 18 | Percussive Organ | percussive-organ | 2.21 | 94 | medium (1.9) | a narrower dynamic range (0.8 vs 16.6 dB); speaks later in a phrase (30.0 vs -1.5 ms); envelope shape (2.7x fonts' spread) |
| 19 | Rock Organ | rock-organ | 2.87 | 57 | low (1.1) | a narrower dynamic range (0.7 vs 16.6 dB); speaks later (44.0 vs 7 ms); speaks later in a phrase (38.0 vs -0.5 ms) |
| 20 | Church Organ | organ-plan | 3.41 | 37 | low (1.4) | a narrower dynamic range (0.4 vs 16.6 dB); quieter (-6.5 vs -0.1 dB); speaks later (222 vs 76.5 ms) |
| 21 | Reed Organ | reed-organ | 2.95 | 53 | low (0.8) | a duller attack (-3.54 vs 0 ERB); brightens more with velocity (1.48 vs 0 ERB); a slower attack (232 vs 53.2 ms) |
| 22 | Accordion | accordion | 2.65 | 77 | medium (1.6) | a duller attack (-6.82 vs -0.52 ERB); speaks later (107 vs 24.0 ms); a narrower dynamic range (11.9 vs 16.6 dB) |
| 23 | Harmonica | harmonica | 4.38 | **12** | low (0.9) | a slower attack (508 vs 48.2 ms); speaks later (106 vs 31.0 ms); less even from key to key (3.43 vs 0.91 dB) |
| 24 | Tango Accordion | bandoneon | 4.53 | **9** | low (1.2) | a slower attack (262 vs 74.5 ms); quieter (-3 vs 2.1 dB); speaks later (100 vs 30.5 ms) |
| 25 | Nylon Guitar | nylon-guitar | 3.85 | 24 | low (1.1) | less even from key to key (4.43 vs 0.86 dB); brighter (10.9 vs 9.09 ERB); brightens more with velocity (1.75 vs 0 ERB) |
| 26 | Steel Guitar | steel-guitar | 2.68 | 69 | low (1.2) | brightens more with velocity (1.74 vs 0 ERB); brighter (14.3 vs 10.0 ERB); a sharper onset (52.7 vs 11.7 dB/ms) |
| 27 | Jazz Guitar | jazz-guitar | 2.20 | 96 | low (1.1) | a slower attack (25.5 vs 3.5 ms); decays slower (5.5 vs 10.8 dB/s); less even from key to key (2.62 vs 1.67 dB) |
| 28 | Clean Guitar | clean-guitar | 1.88 | 109 | low (1.1) | brightens more with velocity (1.34 vs 0 ERB); brightens slower up the keyboard (0.17 vs 4.47 ERB/oct); a slower attack (19.5 vs 4 ms) |
| 29 | Muted Guitar | muted-guitar | 1.93 | 107 | medium (2.5) | brighter (13.1 vs 8.03 ERB); a sharper onset (65.0 vs 11.5 dB/ms); rougher (-0.27 vs -5.36 log2) |
| 30 | Overdriven Guitar | overdriven-guitar | 1.64 | 117 | low (1.1) | a narrower dynamic range (12.8 vs 16.6 dB); less even from key to key (2.3 vs 1.08 dB); a slower attack (264 vs 40.5 ms) |
| 31 | Distortion Guitar | distortion-guitar | 4.15 | 16 | low (1.2) | a narrower dynamic range (3.4 vs 16.6 dB); brightens more with velocity (4.5 vs 0.065 ERB); speaks later (106 vs 9.5 ms) |
| 32 | Guitar Harmonics | guitar-harmonics | 4.13 | 17 | medium (1.6) | a faster attack (2 vs 31.5 ms); darker (12.5 vs 18.4 ERB); more fundamental (0.597 vs 0) |
| 33 | Acoustic Bass | double-bass | 2.21 | 95 | medium (1.9) | less even from key to key (3.23 vs 0.655 dB); speaks later in a phrase (95.0 vs 4 ms); speaks later (47.0 vs 16.0 ms) |
| 34 | Finger Bass | electric-bass | 1.55 | 122 | low (1.3) | brightens more with velocity (1.29 vs 0 ERB); a narrower dynamic range (12.3 vs 16.6 dB); purer (-85.5 vs -63.5 dB) |
| 35 | Pick Bass | pick-bass | 1.47 | 126 | low (1.4) | purer (-88.0 vs -61.1 dB); less even in tone from key to key (1.57 vs 0.66 ERB); louder toward the bottom (-0.96 vs 1.3 dB/oct) |
| 36 | Fretless Bass | fretless-bass | 2.04 | 102 | low (1.2) | less even from key to key (2.88 vs 0.655 dB); brightens more with velocity (1.04 vs -0.005 ERB); a narrower dynamic range (13.2 vs 16.7 dB) |
| 37 | Slap Bass 1 | slap-bass-1 | 2.55 | 81 | medium (1.6) | brightens more with velocity (2.95 vs 0 ERB); less even from key to key (2.33 vs 0.45 dB); a slower attack (15.0 vs 1.5 ms) |
| 38 | Slap Bass 2 | slap-bass-2 | 2.67 | 73 | low (1.2) | brightens more with velocity (5.16 vs 0 ERB); a narrower dynamic range (11.9 vs 16.7 dB); less even in tone from key to key (2.33 vs 0.543 ERB) |
| 39 | Synth Bass 1 | synth-bass-1 | 2.87 | 58 | low (1.2) | decays slower (1.2 vs 7.2 dB/s); a narrower dynamic range (10.8 vs 16.6 dB); a slower attack (28.0 vs 5.5 ms) |
| 40 | Synth Bass 2 | synth-bass-2 | 1.46 | 127 | medium (1.7) | a narrower dynamic range (11.9 vs 16.7 dB); brightens more with velocity (0.77 vs 0 ERB); decays slower (1.5 vs 2.9 dB/s) |
| 41 | Violin | violin | 1.85 | 111 | low (1.2) | speaks later in a phrase (70.0 vs 14.5 ms); less fundamental (0.317 vs 0.617); spectrogram over time (3.5x fonts' spread) |
| 42 | Viola | viola | 2.37 | 88 | low (1.2) | brightens less with velocity (-1.55 vs 0 ERB); speaks later in a phrase (76.0 vs 6.5 ms); envelope shape (4.3x fonts' spread) |
| 43 | Cello | cello | 1.61 | 118 | low (1.5) | brighter (11.1 vs 8.37 ERB); spectrogram over time (3.7x fonts' spread); envelope shape (3.6x fonts' spread) |
| 44 | Contrabass | contrabass | 2.20 | 97 | medium (1.7) | speaks later in a phrase (98.0 vs 20.0 ms); brightens less with velocity (-1.19 vs 0 ERB); brighter (6.47 vs 5.04 ERB) |
| 45 | Tremolo Strings | tremolo-strings | 3.03 | 47 | low (1.2) | brighter (15.2 vs 12.5 ERB); brightens less with velocity (-1.98 vs -0.015 ERB); speaks later in a phrase (85.0 vs 1 ms) |
| 46 | Pizzicato Strings | pizzicato-strings | 5.28 | **2** | medium (1.8) | **bows instead of plucking (pizzicato bug)**; brighter (16.6 vs 9.23 ERB); speaks later (176 vs 12.0 ms); brightens less with velocity (-1.68 vs 0.005 ERB) |
| 47 | Orchestral Harp | harp | 2.39 | 86 | low (1.1) | brighter (10.6 vs 9.32 ERB); purer (-81.3 vs -68.1 dB); a narrower dynamic range (13.6 vs 16.7 dB) |
| 48 | Timpani | timpani | 1.93 | 106 | high (3.3) | a wider dynamic range (24.6 vs 16.9 dB); brightens slower up the keyboard (1.63 vs 3.95 ERB/oct); speaks later (25.0 vs 2.5 ms) |
| 49 | String Ensemble 1 | strings | 3.01 | 49 | low (1.2) | brighter (17.3 vs 12.8 ERB); brightens less with velocity (-1.52 vs -0.015 ERB); less even in tone from key to key (2.3 vs 0.987 ERB) |
| 50 | String Ensemble 2 | slow-strings | 3.53 | 32 | low (1.2) | brightens less with velocity (-2.09 vs -0.015 ERB); brighter (17.1 vs 12.9 ERB); less even from key to key (3.86 vs 1.35 dB) |
| 51 | Synth Strings 1 | synth-strings-1 | 1.69 | 115 | low (1.4) | speaks later in a phrase (92.0 vs 1.5 ms); less even from key to key (2.55 vs 0.73 dB); speaks later (301 vs 132 ms) |
| 52 | Synth Strings 2 | synth-strings-2 | 1.30 | 129 | low (1.0) | brightens more with velocity (1.06 vs 0 ERB); brighter (13.0 vs 11.8 ERB); less evenly tuned (9.9 vs 5.2 c) |
| 53 | Choir Aahs | choir | 2.70 | 65 | low (1.2) | brightens more with velocity (2.21 vs 0 ERB); smoother (-2.78 vs -1.04 log2); brighter (13.7 vs 12.7 ERB) |
| 54 | Voice Oohs | voice-oohs | 5.15 | **3** | low (0.9) | a slower attack (760 vs 40.0 ms); quieter (-7 vs 1.2 dB); speaks later (326 vs 23.5 ms) |
| 55 | Synth Voice | synth-voice | 2.97 | 52 | low (1.1) | less even in tone from key to key (1.62 vs 0.345 ERB); brightens less with velocity (-1.11 vs 0 ERB); a duller attack (-2.04 vs 0.075 ERB) |
| 56 | Orchestra Hit | orchestra-hit | 3.61 | 29 | low (1.3) | brightens more with velocity (2.1 vs 0.02 ERB); wobbles less (17.4 vs 64.5 c); smoother (-2.04 vs -0.265 log2) |
| 57 | Trumpet | trumpet | 4.23 | **15** | low (1.0) | **plays 2 semitones low under -play (transpose bug)**; darker (12.3 vs 17.9 ERB); less even from key to key (3.46 vs 0.555 dB); a brighter attack (3.21 vs -2.13 ERB) |
| 58 | Trombone | trombone | 4.50 | **10** | low (1.1) | darker (9.34 vs 13.8 ERB); less even from key to key (3.58 vs 0.855 dB); a brighter attack (4.81 vs -1.24 ERB) |
| 59 | Tuba | tuba | 2.64 | 78 | low (1.2) | speaks later (74.0 vs 14.5 ms); less even from key to key (2.6 vs 0.46 dB); a wider dynamic range (22.2 vs 16.6 dB) |
| 60 | Muted Trumpet | muted-trumpet | 2.41 | 85 | low (1.3) | **plays 2 semitones low under -play (transpose bug)**; a narrower dynamic range (8.9 vs 17.0 dB); less even from key to key (3.07 vs 1.01 dB); speaks later in a phrase (54.0 vs 6 ms) |
| 61 | French Horn | horn | 3.51 | 33 | low (1.2) | **plays a fifth low under -play (transpose bug)**; a narrower dynamic range (7 vs 16.6 dB); a brighter attack (5.88 vs -0.65 ERB); speaks later (146 vs 73.0 ms) |
| 62 | Brass Section | brass-section | 3.48 | 35 | low (1.4) | a narrower dynamic range (4.8 vs 16.8 dB); louder toward the top (7.6 vs 0.295 dB/oct); speaks later in a phrase (70.0 vs 5 ms) |
| 63 | Synth Brass 1 | synth-brass-1 | 1.92 | 108 | low (1.2) | brightens more with velocity (1.88 vs 0 ERB); less even from key to key (1.91 vs 0.43 dB); envelope shape (3.0x fonts' spread) |
| 64 | Synth Brass 2 | synth-brass-2 | 2.53 | 82 | low (1.2) | brightens more with velocity (1.49 vs -0.005 ERB); less even from key to key (2.31 vs 0.21 dB); smoother (-6.51 vs -4.11 log2) |
| 65 | Soprano Sax | soprano-sax | 2.90 | 56 | low (1.4) | **plays 2 semitones low under -play (transpose bug)**; speaks later in a phrase (85.0 vs 0 ms); speaks later (99.0 vs 31.0 ms); envelope shape (5.5x fonts' spread) |
| 66 | Alto Sax | alto-sax | 2.98 | 51 | low (1.1) | **plays a major sixth low under -play (transpose bug)**; less even from key to key (4.65 vs 0.955 dB); speaks later (110 vs 22.5 ms); envelope shape (5.0x fonts' spread) |
| 67 | Tenor Sax | tenor-sax | 3.74 | 27 | low (1.1) | **plays a ninth low under -play (transpose bug)**; speaks later (134 vs 24.5 ms); a brighter attack (8.54 vs 0.045 ERB); speaks later in a phrase (94.0 vs 1 ms) |
| 68 | Baritone Sax | baritone-sax | 4.09 | 18 | low (1.3) | **plays an octave and a sixth low under -play (transpose bug)**; speaks later (161 vs 15.0 ms); a brighter attack (10.3 vs 0.085 ERB); speaks later in a phrase (88.0 vs 2.5 ms) |
| 69 | Oboe | oboe | 4.57 | **8** | low (0.9) | brighter (21.5 vs 15.7 ERB); a brighter attack (8.54 vs -0.805 ERB); speaks later in a phrase (88.0 vs -1 ms) |
| 70 | English Horn | english-horn | 3.84 | 25 | low (1.4) | **plays a fifth low under -play (transpose bug)**; brighter (20.0 vs 13.8 ERB); brightens more with velocity (1.33 vs 0 ERB); speaks later in a phrase (66.0 vs 0 ms) |
| 71 | Bassoon | bassoon | 3.34 | 39 | low (1.2) | brighter (17.3 vs 12.8 ERB); speaks later in a phrase (81.0 vs 2 ms); a duller attack (-2.38 vs -0.41 ERB) |
| 72 | Clarinet | clarinet | 4.38 | **11** | low (1.1) | **plays 2 semitones low under -play (transpose bug)**; less even from key to key (4.83 vs 0.69 dB); brightens more with velocity (4.58 vs -0.055 ERB); speaks later (131 vs 27.5 ms) |
| 73 | Piccolo | piccolo | 4.33 | **14** | low (1.2) | **plays an octave high under -play (transpose bug)**; brighter (20.8 vs 13.9 ERB); less even in tone from key to key (4.01 vs 0.348 ERB); less fundamental (0.158 vs 0.972) |
| 74 | Flute | flute | 2.91 | 55 | low (1.2) | less even from key to key (3.63 vs 0.88 dB); louder toward the top (9.98 vs 3.19 dB/oct); brighter (18.1 vs 14.6 ERB) |
| 75 | Recorder | recorder | 2.64 | 79 | low (1.0) | less even in tone from key to key (2.13 vs 0.294 ERB); less even from key to key (3.38 vs 0.975 dB); less evenly tuned (15.3 vs 0.95 c) |
| 76 | Pan Flute | panflute | 2.66 | 74 | low (1.0) | louder toward the bottom (-7.61 vs 0.625 dB/oct); less even in tone from key to key (1.79 vs 0.315 ERB); less even from key to key (2.9 vs 0.53 dB) |
| 77 | Blown Bottle | bottle | 2.81 | 62 | low (1.2) | louder toward the top (9.44 vs -0.255 dB/oct); less even from key to key (3.08 vs 0.34 dB); less even in tone from key to key (1.45 vs 0.272 ERB) |
| 78 | Shakuhachi | shakuhachi | 3.55 | 31 | medium (1.6) | less even from key to key (5.74 vs 0.285 dB); speaks later (109 vs 36.5 ms); a brighter attack (11.7 vs 0.545 ERB) |
| 79 | Whistle | whistle | 3.18 | 43 | low (1.3) | less even in tone from key to key (2.97 vs 0.189 ERB); less evenly tuned (18.9 vs 0.4 c); noisier (-29.6 vs -67.1 dB) |
| 80 | Ocarina | ocarina | 4.37 | **13** | low (0.5) | brighter (14.6 vs 13.0 ERB); less even in tone from key to key (2.3 vs 0.218 ERB); less evenly tuned (21.7 vs 1.55 c) |
| 81 | Lead 1 (square) | lead-square | 3.77 | 26 | low (0.8) | a slower attack (44.5 vs 1 ms); spectrogram over time (9.1x fonts' spread); less even from key to key (2.73 vs 0.485 dB) |
| 82 | Lead 2 (sawtooth) | lead-saw | 2.37 | 87 | low (0.7) | brightens more with velocity (2.1 vs 0 ERB); speaks later in a phrase (44.0 vs -1 ms); spectrogram over time (3.9x fonts' spread) |
| 83 | Lead 3 (calliope) | lead-calliope | 2.80 | 63 | low (1.3) | less even from key to key (3.74 vs 0.785 dB); smoother (-6.8 vs -3.01 log2); a more jagged overtone series (11.2 vs 2.1 dB) |
| 84 | Lead 4 (chiff) | lead-chiff | 1.59 | 119 | low (1.3) | less even from key to key (2.57 vs 0.65 dB); more fundamental (0.952 vs 0.736); a faster attack (25.0 vs 64.5 ms) |
| 85 | Lead 5 (charang) | lead-charang | 2.67 | 71 | low (1.2) | brightens more with velocity (1.7 vs 0 ERB); darker (11.5 vs 16.8 ERB); speaks later in a phrase (44.0 vs -1 ms) |
| 86 | Lead 6 (voice) | lead-voice | 2.35 | 89 | low (1.4) | brightens more with velocity (1.63 vs 0 ERB); speaks later in a phrase (65.0 vs 2.5 ms); brightens slower up the keyboard (2.2 vs 3.55 ERB/oct) |
| 87 | Lead 7 (fifths) | lead-fifths | 2.50 | 83 | low (0.8) | brightens more with velocity (1.94 vs 0 ERB); speaks later in a phrase (56.0 vs -1.5 ms); brighter (11.0 vs 8.84 ERB) |
| 88 | Lead 8 (bass + lead) | lead-bass+lead | 3.05 | 45 | low (0.8) | darker (6.95 vs 9.32 ERB); a slower attack (96.5 vs 5 ms); noisier (-46.5 vs -60.9 dB) |
| 89 | Pad 1 (new age) | pad-new-age | 1.88 | 110 | low (1.4) | less even from key to key (1.83 vs 0.445 dB); speaks later (162 vs 33.5 ms); more fundamental (0.979 vs 0.758) |
| 90 | Pad 2 (warm) | pad-warm | 1.64 | 116 | low (1.0) | spectrogram over time (4.8x fonts' spread); brightens more with velocity (0.81 vs 0 ERB); less even from key to key (2.1 vs 0.925 dB) |
| 91 | Pad 3 (polysynth) | pad-polysynth | 1.55 | 123 | low (1.4) | brightens more with velocity (1.42 vs 0 ERB); speaks later in a phrase (66.0 vs 5 ms); brightens slower up the keyboard (2.54 vs 3.48 ERB/oct) |
| 92 | Pad 4 (choir) | pad-choir | 4.69 | **7** | low (1.1) | quieter (-5.2 vs 1.5 dB); a slower attack (518 vs 128 ms); brighter (18.6 vs 10.2 ERB) |
| 93 | Pad 5 (bowed) | pad-bowed | 2.55 | 80 | low (1.2) | speaks later (270 vs 93.0 ms); less even from key to key (2.59 vs 0.665 dB); a slower attack (544 vs 206 ms) |
| 94 | Pad 6 (metallic) | pad-metallic | 2.65 | 76 | low (1.2) | decays slower (0.6 vs 5.3 dB/s); brightens more with velocity (1.17 vs 0 ERB); less even from key to key (2.94 vs 1.4 dB) |
| 95 | Pad 7 (halo) | pad-halo | 3.97 | 22 | low (0.9) | brighter (12.2 vs 7.97 ERB); less fundamental (0.257 vs 0.905); less even in tone from key to key (1.81 vs 0.464 ERB) |
| 96 | Pad 8 (sweep) | pad-sweep | 1.80 | 112 | medium (1.8) | less even from key to key (4.81 vs 0.87 dB); brightens more with velocity (0.78 vs 0 ERB); quieter (-8.1 vs -3.8 dB) |
| 97 | FX 1 (rain) | fx-rain | 1.77 | 114 | medium (1.9) | less even from key to key (3.27 vs 0.325 dB); brightens less with velocity (-1.06 vs 0 ERB); noisier (-26.0 vs -41.2 dB) |
| 98 | FX 2 (soundtrack) | fx-soundtrack | 5.44 | **1** | low (1.0) | quieter (-19.6 vs 0.65 dB); a narrower dynamic range (5.3 vs 16.6 dB); speaks later (804 vs 114 ms) |
| 99 | FX 3 (crystal) | fx-crystal | 3.21 | 41 | medium (1.8) | brightens more with velocity (2.19 vs 0 ERB); less fundamental (0.374 vs 0.973); less even from key to key (2.36 vs 0.095 dB) |
| 100 | FX 4 (atmosphere) | fx-atmosphere | 2.14 | 100 | low (1.2) | less even from key to key (2.7 vs 1 dB); less even in tone from key to key (1.53 vs 0.694 ERB); quieter (-2.8 vs 1.9 dB) |
| 101 | FX 5 (brightness) | fx-brightness | 1.45 | 128 | medium (2.4) | smoother (-4.29 vs -2.48 log2); speaks later in a phrase (41.0 vs 2.5 ms); brightens more with velocity (0.65 vs 0 ERB) |
| 102 | FX 6 (goblins) | fx-goblins | 4.79 | **6** | medium (1.9) | quieter (-27.6 vs -8.8 dB); brighter (13.3 vs 7.46 ERB); less even in tone from key to key (2.23 vs 0.444 ERB) |
| 103 | FX 7 (echoes) | fx-echoes | 2.68 | 68 | low (1.3) | speaks later in a phrase (100 vs 6 ms); less even from key to key (3.73 vs 0.96 dB); smoother (-6.51 vs -2.69 log2) |
| 104 | FX 8 (sci-fi) | fx-sci-fi | 3.30 | 40 | low (1.5) | sustains less (-51.1 vs -11.3 dB); a narrower dynamic range (6.5 vs 16.6 dB); brightens more with velocity (1.89 vs 0 ERB) |
| 105 | Sitar | sitar | 2.01 | 103 | low (1.2) | less even from key to key (2.67 vs 0.485 dB); brightens more with velocity (1.19 vs 0 ERB); more uneven onsets (46.1 vs 4.6 ms) |
| 106 | Banjo | banjo | 2.45 | 84 | low (1.2) | rougher (-0.04 vs -4.05 log2); a sharper onset (49.1 vs 12.8 dB/ms); stronger high partials (0.638 vs 0.198) |
| 107 | Shamisen | shamisen | 3.51 | 34 | low (0.8) | darker (10.3 vs 14.2 ERB); spectrogram over time (7.5x fonts' spread); stronger high partials (0.534 vs 0.09) |
| 108 | Koto | koto | 1.56 | 120 | low (0.9) | rougher (-1.08 vs -3.49 log2); darker (10.4 vs 14.9 ERB); more uneven onsets (38.5 vs 0.95 ms) |
| 109 | Kalimba | kalimba | 2.78 | 64 | low (0.5) | a brighter attack (5.54 vs 0.56 ERB); brighter (8.06 vs 7 ERB); less even in tone from key to key (1.33 vs 0.217 ERB) |
| 110 | Bagpipe | bagpipe | 4.98 | **4** | low (1.1) | darker (8.33 vs 17.2 ERB); less even in tone from key to key (2.72 vs 0.632 ERB); speaks later (205 vs 12.5 ms) |
| 111 | Fiddle | fiddle | 2.95 | 54 | medium (1.6) | brighter (23.3 vs 12.2 ERB); stronger high partials (0.941 vs 0.147); brightens slower up the keyboard (-0.04 vs 3.9 ERB/oct) |
| 112 | Shanai | shehnai | 4.86 | **5** | low (1.0) | less even from key to key (6.48 vs 0.445 dB); speaks later (104 vs 12.0 ms); brighter (21.7 vs 18.7 ERB) |
| 113 | Tinkle Bell | tinkle-bell | 2.67 | 72 | low (1.5) | less even from key to key (2.24 vs 0.33 dB); less even in tone from key to key (1.08 vs 0.261 ERB); brightens more with velocity (0.95 vs 0 ERB) |
| 114 | Agogo | agogo | 3.44 | 36 | medium (1.6) | less even in tone from key to key (1.96 vs 0.322 ERB); more fundamental (0.474 vs 0); noisier (-11.6 vs -46.3 dB) |
| 115 | Steel Drums | steel-drums | 2.86 | 59 | medium (1.9) | **silent on keys 52-59 (no note areas below C4)**; brightens more with velocity (1.59 vs 0.015 ERB); brightens slower up the keyboard (1.36 vs 3.83 ERB/oct); a less even scale (47.9 vs 0.4 dB) |
| 116 | Woodblock | woodblock | 1.56 | 121 | medium (1.8) | sustains more (-83.1 vs -116 dB); brightens more with velocity (0.64 vs 0.005 ERB); a faster attack (0.5 vs 2 ms) |
| 117 | Taiko Drum | taiko | 4.09 | 19 | low (1.1) | brighter (7.25 vs 3.01 ERB); less even from key to key (3.6 vs 0.28 dB); less stretched partials (-5.94 vs -0.33 c/partial) |
| 118 | Melodic Tom | melodic-tom | 2.82 | 61 | low (1.2) | brighter (7.26 vs 4.62 ERB); more odd partials (hollower) (22.3 vs -5 dB); a slower attack (43.5 vs 7 ms) |
| 119 | Synth Drum | synth-drum | 3.03 | 46 | low (1.3) | brighter (7.14 vs 4.98 ERB); wobbles less (35.6 vs 90.3 c); sustains more (-64.7 vs -120 dB) |
| 120 | Reverse Cymbal | reverse-cymbal | 3.59 | 30 | low (1.4) | a narrower dynamic range (7.1 vs 16.7 dB); darker (12.8 vs 25.9 ERB); weaker high partials (0.459 vs 0.968) |
| 121 | Guitar Fret Noise | fret-noise | 2.99 | 50 | high (3.2) | a narrower dynamic range (6.3 vs 16.6 dB); sustains more (-50.0 vs -119 dB); brighter (22.8 vs 19.1 ERB) |
| 122 | Breath Noise | breath-noise | 3.02 | 48 | medium (2.8) | a narrower dynamic range (0.4 vs 16.8 dB); sustains more (-3.5 vs -112 dB); partials living more their own lives (0.008 vs 0.994) |
| 123 | Seashore | seashore | 2.69 | 66 | medium (2.6) | brightens less with velocity (-2.75 vs 0.23 ERB); less even in tone from key to key (2.69 vs 0.167 ERB); a brighter attack (3.33 vs -0.685 ERB) |
| 124 | Bird Tweet | bird-tweet | 1.54 | 124 | high (3.1) | starts sharper (54.3 vs 0 c); speaks later in a phrase (33.0 vs -1 ms); less even from key to key (1.7 vs 0.67 dB) |
| 125 | Telephone Ring | telephone | 2.23 | 92 | medium (2.7) | a narrower dynamic range (7.9 vs 16.7 dB); speaks later (73.0 vs 4 ms); speaks later in a phrase (38.0 vs 1.5 ms) |
| 126 | Helicopter | helicopter | 3.19 | 42 | medium (2.2) | louder (-10.1 vs -22.0 dB); a narrower dynamic range (9.8 vs 16.8 dB); brightens less with velocity (-0.68 vs 0 ERB) |
| 127 | Applause | applause | 2.69 | 67 | medium (1.7) | speaks sooner (40.0 vs 444 ms); darker (15.9 vs 21.8 ERB); a faster attack (432 vs 760 ms) |
| 128 | Gunshot | gunshot | 1.52 | 125 | medium (2.0) | a sharper onset (63.1 vs 8.65 dB/ms); sharper (26.4 vs -18.4 c); smoother (0.59 vs 1.98 log2) |
| - | Drum kit (channel 10) | drum kit | 3.07 | 44 | low (1.3) | louder (-3.2 vs -7.85 dB); sustains more (-76.2 vs -121 dB); darker (12.4 vs 16.5 ERB) |

### The 15 furthest

1. **98 FX 2 (soundtrack)** (fx-soundtrack, distance 5.44): quieter (-19.6 vs 0.65 dB); a narrower dynamic range (5.3 vs 16.6 dB); speaks later (804 vs 114 ms).
2. **46 Pizzicato Strings** (pizzicato-strings, distance 5.28): **bows instead of plucking (pizzicato bug)**; brighter (16.6 vs 9.23 ERB); speaks later (176 vs 12.0 ms); brightens less with velocity (-1.68 vs 0.005 ERB).
3. **54 Voice Oohs** (voice-oohs, distance 5.15): a slower attack (760 vs 40.0 ms); quieter (-7 vs 1.2 dB); speaks later (326 vs 23.5 ms).
4. **110 Bagpipe** (bagpipe, distance 4.98): darker (8.33 vs 17.2 ERB); less even in tone from key to key (2.72 vs 0.632 ERB); speaks later (205 vs 12.5 ms).
5. **112 Shanai** (shehnai, distance 4.86): less even from key to key (6.48 vs 0.445 dB); speaks later (104 vs 12.0 ms); brighter (21.7 vs 18.7 ERB).
6. **102 FX 6 (goblins)** (fx-goblins, distance 4.79): quieter (-27.6 vs -8.8 dB); brighter (13.3 vs 7.46 ERB); less even in tone from key to key (2.23 vs 0.444 ERB).
7. **92 Pad 4 (choir)** (pad-choir, distance 4.69): quieter (-5.2 vs 1.5 dB); a slower attack (518 vs 128 ms); brighter (18.6 vs 10.2 ERB).
8. **69 Oboe** (oboe, distance 4.57): brighter (21.5 vs 15.7 ERB); a brighter attack (8.54 vs -0.805 ERB); speaks later in a phrase (88.0 vs -1 ms).
9. **24 Tango Accordion** (bandoneon, distance 4.53): a slower attack (262 vs 74.5 ms); quieter (-3 vs 2.1 dB); speaks later (100 vs 30.5 ms).
10. **58 Trombone** (trombone, distance 4.50): darker (9.34 vs 13.8 ERB); less even from key to key (3.58 vs 0.855 dB); a brighter attack (4.81 vs -1.24 ERB).
11. **72 Clarinet** (clarinet, distance 4.38): **plays 2 semitones low under -play (transpose bug)**; less even from key to key (4.83 vs 0.69 dB); brightens more with velocity (4.58 vs -0.055 ERB); speaks later (131 vs 27.5 ms).
12. **23 Harmonica** (harmonica, distance 4.38): a slower attack (508 vs 48.2 ms); speaks later (106 vs 31.0 ms); less even from key to key (3.43 vs 0.91 dB).
13. **80 Ocarina** (ocarina, distance 4.37): brighter (14.6 vs 13.0 ERB); less even in tone from key to key (2.3 vs 0.218 ERB); less evenly tuned (21.7 vs 1.55 c).
14. **73 Piccolo** (piccolo, distance 4.33): **plays an octave high under -play (transpose bug)**; brighter (20.8 vs 13.9 ERB); less even in tone from key to key (4.01 vs 0.348 ERB); less fundamental (0.158 vs 0.972).
15. **57 Trumpet** (trumpet, distance 4.23): **plays 2 semitones low under -play (transpose bug)**; darker (12.3 vs 17.9 ERB); less even from key to key (3.46 vs 0.555 dB); a brighter attack (3.21 vs -2.13 ERB).

## Patterns across instruments

The counts are over the 128 melodic programs. "Outside" means pajupilli lies beyond all four fonts
by more than half a JND. Medians are over the instruments.

1. **Key-to-key unevenness is the most common real difference.**
   - Level scatter across the keys, after the velocity and a smooth key trend are removed: median
     2.5 dB against 0.55–1.2 dB for the fonts. pajupilli is outside, and less even, in 97 of 128.
   - Brightness scatter is more uneven in 62 (none more even).
   - The legato scale is less even in 40 (more even in 1).
   - Part of it is deliberate: tine piano voicing, the guitarist's choice of string, choir sections
     covering parts of the range. Part of it is the room: near-sines and their reflections add or
     cancel key by key, 3.9 dB in the hall against 0.5 dB dry in the fx-echoes note.
   - Part is the probe: keys outside an instrument's compass are folded by octaves.
   - It is still too consistent to be all of that. Worst: bandoneón 8.3 dB, ocarina 8.0,
     harmonica 7.8, shehnai 5.9, accordion 5.9, vibraphone 5.3. The free reeds and the vessel and
     flute family stand out.
   - The choir already has an evening table (`PJ_CHOIR_EVEN`) and the bowed basses one
     (`tools/bowlevels.py`). Instruments without one are the uneven ones.
2. **pajupilli speaks later.**
   - Onset (−6 dB of the first peak) after the note-on: median 34 ms against 12–19 ms; later and
     outside in 74.
   - Attack time from −20 to −1 dB: median 99 ms against 32–45.
   - In the phrase, the onset is 30 ms against 0.5–4 ms; later in 86 of 128 (the fonts start at
     once, a sample).
   - Some of this is physics: blown and bowed notes build up, and the plucked models have a pick
     or finger travel. But the reeds (saxes and clarinet 90–180 ms), voice oohs (444 ms), the
     choir, the pads and fx-soundtrack/goblins (0.8–0.9 s) are slow enough to drag in a GM file.
   - The leads and pads are 30–66 ms late in the phrase, where the fonts take about 0.
3. **A narrower loudness range under TiMidity's convention, but brighter with velocity.**
   - The fonts under TiMidity all give about −13/−6/0/+4 dB at velocities 40/64/96/127. That is
     TiMidity's law, the same for every font. Through FluidSynth the same fonts span about 20 dB.
   - pajupilli's median range is 15.6 dB; it is narrower and outside in 69 programs.
   - It is nearly flat on purpose for the organs (17–21) and the harpsichord, which have no
     velocity. On the overdriven and distortion guitars the amplifier's clipping compresses it,
     as expected. It is narrow without a stated reason on the brass section (5 dB) and fx-sci-fi
     (6.7 dB). On the harmonica it is reversed: velocity 127 is 24 dB softer than 96.
   - pajupilli brightens with velocity: median +0.44 ERB from 40 to 127, more in 67 programs. The
     fonts don't, not even through FluidSynth (median −0.2 … 0). This is deliberate: physical
     models get brighter when played harder.
   - The bowed strings are the exception: they get darker when louder. research/viola.md §2 fixed
     this for the viola only.
4. **Darker above 6 kHz, fuller in the middle.**
   - Against the fonts, with level removed, the median spectrum of pajupilli is:
     - −0.7 dB below 150 Hz;
     - +0.9 dB at 150–400 Hz;
     - +1.9 dB at 400 Hz–1 kHz;
     - +1.5 dB at 1–2.5 kHz;
     - +0.2 dB at 2.5–6 kHz;
     - **−5.8 dB above 6 kHz**.
   - 52 % of the programs are more than 3 dB darker above 6 kHz; 32 % are brighter.
   - The overall centroid shows no bias (median +0.1 ERB). The difference is the top octave, not
     the balance. Some is deliberate (the jazz guitar), and the fonts' samples
     carry recording hiss and room air in their top octave.
   - The exceptions go the other way and are large:
     - brighter: piccolo (+10 ERB, with the octave bug), oboe/english horn/bassoon (4–6 ERB,
       noisier by 20–40 dB in flatness), the bowed strings and string sections (+4–6 ERB),
       pad-choir and pad-halo, the bandoneón;
     - darker: trumpet (−6.7 ERB), trombone (−4.5), bagpipe (−8.9), the jazz guitar (deliberate).
5. **Brightness grows more slowly up the keyboard.** It is outside and slower in 64 programs, faster
   in 8. The fonts' multisamples get brighter per zone; pajupilli's models keep a more constant
   spectrum shape in harmonics. That is often closer to the real instrument, but it makes the bass
   relatively bright and the top relatively dull next to a SoundFont.
6. **The room is in everything.**
   - pajupilli plays every instrument in its own room; the fonts were rendered dry. Its effects:
     - the noise/room floor 3.6–4 s after the note-on is higher in 91 programs;
     - the release is longer in 84;
     - stereo coherence is 0.60 against 0.97–1.0 (lower in 80);
     - a 100 ms note sounds longer in 64.
   - These four measures are left out of the distance and the summary table. They are a
     deliberate design, not instrument faults.
   - Where pajupilli plays dry (`-room dry`), a re-run would separate them. The room also adds to
     the key-to-key scatter (1.).
7. **The families.** The mean distance by GM family:
   - reeds (65–72) 4.0, brass (57–64) 3.9, pipes (73–80) 3.8, ethnic (105–112) 3.4 and organs
     (17–24) 3.4 are furthest;
   - basses (33–40) 2.3, sound effects (121–128) 2.6, strings (41–48) 2.7 and pads (89–96) 2.8
     are nearest.
   - The reed and brass figures are inflated by the transposition bug.
8. **TiMidity misreads the fonts often.** In 152 of 384 font × instrument pairs, the FluidSynth
   render differs from TiMidity's by at least 0.5 ERB in brightness, attack brightness or
   brightening with velocity. Each instrument section lists those pairs. For synth basses, leads,
   pads and some plucks the TiMidity numbers are therefore not what the font's author intended.
9. **Relative level (velocity 96, against each synth's own median).** Struck and plucked programs
   come out 4–10 dB above the fonts: marimba, xylophone, nylon guitar, timpani. The slow-speaking
   ones come out below: church and reed organ, bandoneón, choir, oohs, pads, fx-soundtrack,
   fx-goblins.
   - For the slow ones, part of this is the 0.5 s level window. In a GM file, short notes on those
     programs will still be too quiet.

## Per instrument

### 1 Acoustic Grand Piano: grand-piano

Distance 1.77 (rank 113 of 129); the fonts disagree 1.1 JND. Keys 21,27,33,39,45,51,57,63,69,75,81,87,93,99,105, velocities 40/64/96/127.

- **Staccato note dies at once (32 ms to −20 dB vs 131…208)**: a 100 ms note falls 20 dB within 32 ms. Together with the sharp onset (28 vs 8…14 dB/ms), this means the first few ms peak about 20 dB above what follows: a hammer/contact spike over the body. The bright piano gives 88 ms and the electric grand 217 ms on the same engine. Likely FAULT: look at the first 30 ms of a v80 note.
- **Key-to-key level scatter (5.0 vs 1.3…3.2 dB)**: the probe includes A0 and A7, where the grand drops. FluidR3 is 3.2 too. Worth checking, but not clearly wrong.
- **Brightens with velocity (+1.07 vs 0 ERB)**: DELIBERATE (README "Concert grand": the felt hardens). This is mostly a TiMidity ARTIFACT: Jnsgm2 through FluidSynth brightens +1.4 ERB.
- **Floor −90 dB, release 328 ms**: room ARTIFACT.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **sounding length of a 100 ms note [room]**: shorter staccato (32.0 ms | 138 | 131 | 208 | 167), outside the fonts, 6.7 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (1.21 ERB | 0 | -0 | -0 | -0.25), outside the fonts, 4.0 x.
3. **key-to-key level scatter**: less even from key to key (4.95 dB | 3.18 | 1.93 | 1.26 | 1.62), outside the fonts, 3.3 x.
4. **onset sharpness**: a sharper onset (28.2 dB/ms | 9.4 | 10.0 | 8.3 | 14.4), outside the fonts, 6.1 x.
5. **loudness range, velocity 40 to 127**: a narrower dynamic range (14.1 dB | 16.7 | 16.6 | 16.7 | 19.1), outside the fonts, 1.7 x.
6. **level against the other instruments (velocity 96)**: louder (-0.8 dB | -5.2 | -5.8 | -0.1 | -3.9), inside the fonts' range, 1.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -14 | +7 | +6 | +3 | -8 | -1 |
| re TimGM6mb | -17 | +0 | -4 | -3 | -1 | +24 |
| re Jnsgm2 | -10 | +4 | -0 | +1 | -4 | +6 |
| re SGM | -18 | -4 | -7 | -4 | +3 | +32 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -14 -6 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack -0.0 ERB, velocity brightening +0.0 ERB, loudness range +3 dB, envelope 1.7 dB rms; TimGM6mb: centroid -0.1 ERB, attack -0.0 ERB, velocity brightening +0.0 ERB, loudness range +4 dB, envelope 0.5 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.5 ERB, velocity brightening +1.4 ERB, loudness range +5 dB, envelope 0.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.02 | 0.57 | -0.01 | 0.82 | 0.29 | 1.0 |  |
| attack time (-20 to -1 dB) (ms) | 13.0 | 22.0 | 12.0 | 20.0 | 13.0 | 0.7 |  |
| spectral shape of the attack, note for note | 10.7 (nearest 9.14) | fonts among themselves 11.5 | | | | 0.9 |  |
| brightness over the keyboard (ERB/oct) | 2.09 | 3.01 | 2.5 | 2.07 | 1.1 | 0.2 |  |
| key-to-key brightness scatter (ERB) | 1.11 | 1.38 | 1.02 | 1.3 | 2.87 | 0.2 |  |
| brightening from velocity 40 to 127 (ERB) | 1.21 | 0 | -0 | -0 | -0.25 | 4.0 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 10.7 | 10.3 | 10.8 | 10.7 | 10.0 | 0.6 |  |
| decay rate of a held note (dB/s) | 12.4 | 12.0 | 9.9 | 11.7 | 8.1 | 0.7 |  |
| onset delay (ms) | 14.0 | 6 | 6 | 8 | 9 | 0.7 |  |
| Plomp-Levelt dissonance of a note (log2) | -3.92 | -3.9 | -3.67 | -4.76 | -2.83 | 0.1 |  |
| envelope curve, note for note (dB rms) | 8.69 (nearest 8.12) | fonts among themselves 5.91 | | | | 1.5 | yes |
| pitch re 12-TET A440 (median) (c) | -1.2 | -0.1 | 0.9 | -1.2 | -0.3 | 0.2 |  |
| pitch scatter from key to key (c) | 5 | 5.5 | 1.5 | 2.8 | 5.5 | 0.3 |  |
| noisiness (spectral flatness) (dB) | -68.4 | -39.7 | -67.1 | -47.0 | -57.7 | 1.2 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -91.0 | -120 | -118 | -123 | -105 | 3.0 | yes |
| pitch at the start re settled (c) | 0.2 | 1.5 | 0.8 | 0.8 | 0.8 | 0.1 |  |
| stereo coherence | 0.679 | 0.574 | 1 | 1 | 0.607 | 0.6 |  |
| partial-to-partial jaggedness (dB) | 5.75 | 3.9 | 4.2 | 4.4 | 3.6 | 1.7 | yes |
| dip between legato notes (dB) | 11.8 | 9.3 | 8 | 11.6 | 5.7 | 1.1 |  |
| level evenness of a legato scale (dB) | 1.9 | 1.4 | 1.3 | 0.8 | 1.2 | 0.6 |  |
| level over the keyboard (dB/oct) | -0.37 | -2.07 | -0.23 | -0.55 | -1.57 | 0.7 |  |
| level against the other instruments (velocity 96) (dB) | -0.8 | -5.2 | -5.8 | -0.1 | -3.9 | 1.3 |  |
| key-to-key level scatter (dB) | 4.95 | 3.18 | 1.93 | 1.26 | 1.62 | 3.3 | yes |
| odd / even partials (dB) | -3.7 | -7 | -7.7 | -4.2 | -7.9 | 1.8 |  |
| onset jitter in a phrase (ms) | 40.3 | 21.2 | 17.8 | 3.5 | 39.7 | 1.1 |  |
| onset in a phrase (re the note-on) (ms) | 11.0 | 0 | -1 | 0 | 1 | 1.1 | yes |
| onset sharpness (dB/ms) | 28.2 | 9.4 | 10.0 | 8.3 | 14.4 | 6.1 | yes |
| overtone levels, note for note (dB rms) | 11.4 (nearest 10.2) | fonts among themselves 9.27 | | | | 1.2 |  |
| panning across the keys (dB/oct) | -0.63 | -0.82 | 0 | 0 | 0.07 | 0.6 |  |
| partial envelopes alike | 0.91 | 0.877 | 0.871 | 0.873 | 0.512 | 0.2 |  |
| release time (30 dB after note-off) (ms) | 324 | 240 | 276 | 330 | 226 | 1.2 |  |
| spectrogram (1 - correlation), note for note | 0.201 (nearest 0.186) | fonts among themselves 0.215 | | | | 0.9 |  |
| spectral shape of the body, note for note | 11.8 (nearest 10) | fonts among themselves 11.3 | | | | 1.0 |  |
| sounding length of a 100 ms note (ms) | 32.0 | 138 | 131 | 208 | 167 | 6.7 | yes |
| partial stretch (inharmonicity) (c/partial) | 1.44 | 1.75 | 1.65 | 1.75 | 2.22 | 0.6 |  |
| level after 2 s held, re peak (dB) | -27.7 | -31.3 | -28.9 | -26.2 | -24.8 | 0.0 |  |
| high bands lag low bands by (ms) | 0 | -2.8 | -1.3 | -3 | -2.5 | 0.5 |  |
| transient coherence: attack spread over bands (ms) | 0.6 | 2.1 | 0.9 | 2.6 | 1.4 | 0.2 |  |
| fundamental share (tristimulus 1) | 0.625 | 0.734 | 0.481 | 0.79 | 0.569 | 0.2 |  |
| partials 2-4 share (tristimulus 2) | 0.1 | 0.15 | 0.337 | 0.146 | 0.112 | 0.4 |  |
| partials 5+ share (tristimulus 3) | 0.002 | 0.039 | 0.052 | 0.031 | 0.013 | 0.5 |  |
| loudness range, velocity 40 to 127 (dB) | 14.1 | 16.7 | 16.6 | 16.7 | 19.1 | 1.7 | yes |
| pitch wobble / vibrato (p-p) (c) | 3.7 | 4 | 5.6 | 3.7 | 7.1 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0156 (nearest 0.00979) | fonts among themselves 0.0157 | | | | 1.0 |  |

</details>

### 2 Bright Acoustic Piano: bright-piano

Distance 1.97 (rank 105 of 129); the fonts disagree 1.1 JND. Keys 21,27,33,39,45,51,57,63,69,75,81,87,93,99,105, velocities 40/64/96/127.

- **Key-to-key level scatter (5.1 vs 1.3…3.2 dB)**: the same engine as the grand and the same picture. research/bright_grand.md measured 0.8–2.5 dB rms off the neighbours at v100. The extra here comes from the probe's extremes and its velocities: check before calling it a fault.
- **Sharper onset (27.6 vs 8…15 dB/ms)**: the same hammer spike as p000, but the staccato length (88 ms) is less extreme. Possible FAULT shared with the grand.
- **Brighter (11.3 vs 10.3…10.8 ERB), brightens with velocity (+1.16 ERB)**: DELIBERATE. This agrees with the earlier FluidR3 comparison (bright_grand.md, "Against FluidR3"): FluidR3's GM 2 is its grand sample with a fixed brightness, and here the brightness grows with the blow. The >6 kHz excess re SGM (+33 dB) is the dark SGM sample.
- **Release 408 ms**: room ARTIFACT.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (1.62 ERB | 0 | -0 | -0 | -0.94), outside the fonts, 3.4 x.
2. **brightness (spectral centroid, velocity 96)**: brighter (11.2 ERB | 10.3 | 10.8 | 10.7 | 10.5), outside the fonts, 2.3 x.
3. **onset sharpness**: a sharper onset (28.3 dB/ms | 9.4 | 9.7 | 8.3 | 14.6), outside the fonts, 6.0 x.
4. **key-to-key level scatter**: less even from key to key (4.02 dB | 3.18 | 1.66 | 1.26 | 1.65), outside the fonts, 2.5 x.
5. **release time (30 dB after note-off) [room]**: rings longer after the key (385 ms | 240 | 272 | 330 | 226), outside the fonts, 2.2 x.
6. **pitch scatter from key to key**: less evenly tuned (9.8 c | 5.5 | 1.5 | 2.8 | 5.5), outside the fonts, 1.9 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -14 | +2 | +5 | +2 | -6 | +4 |
| re TimGM6mb | -14 | -2 | -3 | -0 | -0 | +15 |
| re Jnsgm2 | -10 | -0 | -1 | +0 | -2 | +8 |
| re SGM | -16 | -7 | -8 | -6 | +2 | +34 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -14 -6 +0 +4
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening +0.0 ERB, loudness range +4 dB, envelope 1.8 dB rms; TimGM6mb: centroid -0.0 ERB, attack -0.0 ERB, velocity brightening +0.0 ERB, loudness range +4 dB, envelope 0.3 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.3 ERB, velocity brightening +0.5 ERB, loudness range +4 dB, envelope 0.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.04 | 0.57 | 0 | 0.82 | 0.34 | 1.2 |  |
| attack time (-20 to -1 dB) (ms) | 21.0 | 22.0 | 12.0 | 20.0 | 13.5 | 0.8 |  |
| spectral shape of the attack, note for note | 9.78 (nearest 8.79) | fonts among themselves 11.3 | | | | 0.9 |  |
| brightness over the keyboard (ERB/oct) | 2.08 | 3.01 | 2.52 | 2.07 | 1.3 | 0.3 |  |
| key-to-key brightness scatter (ERB) | 1.41 | 1.38 | 0.981 | 1.3 | 2.75 | 0.1 |  |
| brightening from velocity 40 to 127 (ERB) | 1.62 | 0 | -0 | -0 | -0.94 | 3.4 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 11.2 | 10.3 | 10.8 | 10.7 | 10.5 | 2.3 | yes |
| decay rate of a held note (dB/s) | 11.7 | 12.0 | 9.8 | 11.7 | 7.9 | 0.4 |  |
| onset delay (ms) | 14.0 | 6 | 6 | 8 | 9 | 0.7 |  |
| Plomp-Levelt dissonance of a note (log2) | -2.73 | -3.9 | -3.59 | -4.76 | -2.85 | 1.1 |  |
| envelope curve, note for note (dB rms) | 9.57 (nearest 8.45) | fonts among themselves 5.91 | | | | 1.6 | yes |
| pitch re 12-TET A440 (median) (c) | 1.2 | -0.1 | 0.9 | -1.2 | -0.3 | 0.3 |  |
| pitch scatter from key to key (c) | 9.8 | 5.5 | 1.5 | 2.8 | 5.5 | 1.9 | yes |
| noisiness (spectral flatness) (dB) | -58.8 | -39.7 | -66.8 | -47.0 | -56.5 | 0.5 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -89.5 | -122 | -118 | -123 | -106 | 3.4 | yes |
| pitch at the start re settled (c) | 0.4 | 1.5 | 0.8 | 0.8 | 0.8 | 0.1 |  |
| stereo coherence | 0.593 | 0.574 | 1 | 1 | 0.589 | 0.9 |  |
| partial-to-partial jaggedness (dB) | 5.95 | 3.9 | 4.3 | 4.4 | 3.6 | 1.9 | yes |
| dip between legato notes (dB) | 10.0 | 9.3 | 8.7 | 11.6 | 5.4 | 0.3 |  |
| level evenness of a legato scale (dB) | 1.9 | 1.4 | 1.4 | 0.8 | 1.2 | 0.6 |  |
| level over the keyboard (dB/oct) | 0.49 | -2.07 | -0.16 | -0.55 | -1.25 | 1.4 | yes |
| level against the other instruments (velocity 96) (dB) | -5.5 | -2.9 | -5.9 | -0.1 | -3.4 | 0.8 |  |
| key-to-key level scatter (dB) | 4.02 | 3.18 | 1.66 | 1.26 | 1.65 | 2.5 | yes |
| odd / even partials (dB) | -2.9 | -7 | -8.1 | -4.2 | -6.5 | 1.9 | yes |
| onset jitter in a phrase (ms) | 39.1 | 21.2 | 30.0 | 3.5 | 39.7 | 0.7 |  |
| onset in a phrase (re the note-on) (ms) | 11.0 | 0 | -1 | 0 | 1 | 1.1 | yes |
| onset sharpness (dB/ms) | 28.3 | 9.4 | 9.7 | 8.3 | 14.6 | 6.0 | yes |
| overtone levels, note for note (dB rms) | 10.5 (nearest 9.75) | fonts among themselves 8.75 | | | | 1.2 |  |
| panning across the keys (dB/oct) | 0.09 | -0.82 | 0 | 0 | 0.06 | 0.1 |  |
| partial envelopes alike | 0.917 | 0.877 | 0.872 | 0.873 | 0.512 | 0.2 |  |
| release time (30 dB after note-off) (ms) | 385 | 240 | 272 | 330 | 226 | 2.2 | yes |
| spectrogram (1 - correlation), note for note | 0.211 (nearest 0.193) | fonts among themselves 0.203 | | | | 1.0 |  |
| spectral shape of the body, note for note | 12 (nearest 10.5) | fonts among themselves 11.2 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 88.0 | 138 | 131 | 208 | 167 | 2.4 | yes |
| partial stretch (inharmonicity) (c/partial) | 2.74 | 1.75 | 1.72 | 1.75 | 2.44 | 2.0 | yes |
| level after 2 s held, re peak (dB) | -26.0 | -31.3 | -28.8 | -26.2 | -24.8 | 0.5 |  |
| high bands lag low bands by (ms) | 0 | -2.8 | -1 | -3 | -2 | 0.5 |  |
| transient coherence: attack spread over bands (ms) | 1.4 | 2.1 | 0.9 | 2.6 | 1.4 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.279 | 0.734 | 0.484 | 0.79 | 0.55 | 2.4 | yes |
| partials 2-4 share (tristimulus 2) | 0.325 | 0.15 | 0.348 | 0.146 | 0.129 | 1.6 |  |
| partials 5+ share (tristimulus 3) | 0.014 | 0.039 | 0.046 | 0.031 | 0.025 | 0.3 |  |
| loudness range, velocity 40 to 127 (dB) | 14.9 | 16.6 | 16.6 | 16.7 | 18.5 | 1.2 | yes |
| pitch wobble / vibrato (p-p) (c) | 4.4 | 4 | 5.7 | 3.7 | 6.8 | 0.1 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0153 (nearest 0.0139) | fonts among themselves 0.0153 | | | | 1.0 |  |

</details>

### 3 Electric Grand Piano: electric-grand

Distance 2.17 (rank 98 of 129); the fonts disagree 1.5 JND. Keys 21,27,33,39,45,51,57,63,69,75,81,87,93,99,105, velocities 40/64/96/127.

- **Sharp onset (37.8 vs 5.5…13.8 dB/ms) but slow attack (39.5 vs 2…12 ms)**: the onset is steep and the peak comes late. This pattern repeats across plucked and struck programs (p004, p007, p014, p024, p025) and is most likely the hall's early reflections and reverb build-up adding to the first tens of ms. Probably a room ARTIFACT; a `-room dry` render would settle it.
- **Key-to-key scatter (3.6 vs 0.75…2.8 dB)**: SGM is 2.8. Partly DELIBERATE: research/electricgrand.md models per-pickup spread.
- **Brightens with velocity (+0.72 ERB)**: DELIBERATE (README "Electric grand"). Jnsgm2 through FluidSynth gives +1.7 ERB, so this is a TiMidity ARTIFACT.
- Overall the nearest to the fonts of all programs (rank 67).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (2.16 ERB | -0.01 | -0 | 0 | 0.48), outside the fonts, 7.2 x.
2. **onset sharpness**: a sharper onset (44.6 dB/ms | 12.5 | 5.5 | 13.8 | 9.4), outside the fonts, 8.1 x.
3. **key-to-key brightness scatter**: less even in tone from key to key (1.92 ERB | 0.602 | 1.25 | 0.748 | 1.1), outside the fonts, 3.1 x.
4. **release time (30 dB after note-off) [room]**: rings longer after the key (393 ms | 224 | 221 | 327 | 262), outside the fonts, 2.5 x.
5. **pitch scatter from key to key**: less evenly tuned (7.7 c | 5.5 | 1.3 | 3 | 1.5), outside the fonts, 1.8 x.
6. **attack time (-20 to -1 dB)**: a slower attack (33.5 ms | 2 | 10.0 | 7 | 12.0), outside the fonts, 1.5 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -6 | -2 | +3 | +0 | -6 | +13 |
| re TimGM6mb | -0 | +6 | +5 | +2 | -6 | -5 |
| re Jnsgm2 | -2 | +5 | +6 | +3 | -6 | -6 |
| re SGM | -7 | +1 | +0 | -1 | -4 | +14 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -16 -7 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -16 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack -0.7 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 1.0 dB rms; TimGM6mb: centroid -0.0 ERB, attack +0.2 ERB, velocity brightening +0.0 ERB, loudness range +3 dB, envelope 1.3 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.0 ERB, velocity brightening +1.7 ERB, loudness range +4 dB, envelope 0.4 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.35 | 2.88 | -0.17 | 0.51 | 0.45 | 0.1 |  |
| attack time (-20 to -1 dB) (ms) | 33.5 | 2 | 10.0 | 7 | 12.0 | 1.5 | yes |
| spectral shape of the attack, note for note | 10 (nearest 7.94) | fonts among themselves 10.2 | | | | 1.0 |  |
| brightness over the keyboard (ERB/oct) | 2.02 | 2.82 | 2.89 | 3.63 | 2.65 | 1.7 | yes |
| key-to-key brightness scatter (ERB) | 1.92 | 0.602 | 1.25 | 0.748 | 1.1 | 3.1 | yes |
| brightening from velocity 40 to 127 (ERB) | 2.16 | -0.01 | -0 | 0 | 0.48 | 7.2 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 12.0 | 8.46 | 13.0 | 12.2 | 10.9 | 0.2 |  |
| decay rate of a held note (dB/s) | 11.6 | 14.0 | 9.3 | 19.8 | 6 | 0.0 |  |
| onset delay (ms) | 12.0 | 2 | 2 | 6 | 4 | 0.9 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.24 | -6.51 | -3.1 | -5.44 | -3.84 | 0.8 |  |
| envelope curve, note for note (dB rms) | 12.9 (nearest 11.5) | fonts among themselves 11 | | | | 1.2 |  |
| pitch re 12-TET A440 (median) (c) | -0.5 | 2.7 | 1.7 | 2.7 | 1 | 0.7 |  |
| pitch scatter from key to key (c) | 7.7 | 5.5 | 1.3 | 3 | 1.5 | 1.8 | yes |
| noisiness (spectral flatness) (dB) | -51.2 | -47.7 | -54.4 | -52.8 | -52.1 | 0.4 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -91.9 | -124 | -119 | -122 | -106 | 3.2 | yes |
| pitch at the start re settled (c) | -0.4 | 0.9 | -1.2 | 3.1 | 1.2 | 0.3 |  |
| stereo coherence | 0.697 | 1 | 0.988 | 1 | 0.422 | 1.0 |  |
| partial-to-partial jaggedness (dB) | 5.35 | 4.8 | 3 | 4.1 | 4.7 | 0.9 | yes |
| dip between legato notes (dB) | 12.4 | 11.2 | 4.3 | 9.1 | 7 | 1.3 | yes |
| level evenness of a legato scale (dB) | 1.1 | 1.2 | 1.5 | 1.2 | 1.1 | 0.1 |  |
| level over the keyboard (dB/oct) | -0.55 | -0.4 | -0.13 | -1.07 | -1.65 | 0.2 |  |
| level against the other instruments (velocity 96) (dB) | 0.2 | 0.1 | -3 | -3.2 | -4.8 | 1.3 |  |
| key-to-key level scatter (dB) | 2.76 | 1.13 | 1.25 | 0.75 | 2.8 | 1.5 |  |
| odd / even partials (dB) | -6.8 | -4.3 | -1.9 | -2.9 | -8.3 | 1.0 |  |
| onset jitter in a phrase (ms) | 26.5 | 48.7 | 28.6 | 1.5 | 5.8 | 0.4 |  |
| onset in a phrase (re the note-on) (ms) | 7 | -4 | -2 | 0 | 2 | 0.8 |  |
| onset sharpness (dB/ms) | 44.6 | 12.5 | 5.5 | 13.8 | 9.4 | 8.1 | yes |
| overtone levels, note for note (dB rms) | 11.8 (nearest 7.07) | fonts among themselves 10.7 | | | | 1.1 |  |
| panning across the keys (dB/oct) | -0.25 | 0 | 0 | 0 | -0.71 | 0.2 |  |
| partial envelopes alike | 0.643 | 0.919 | 0.816 | 0.873 | 0.831 | 2.1 | yes |
| release time (30 dB after note-off) (ms) | 393 | 224 | 221 | 327 | 262 | 2.5 | yes |
| spectrogram (1 - correlation), note for note | 0.228 (nearest 0.171) | fonts among themselves 0.175 | | | | 1.3 |  |
| spectral shape of the body, note for note | 10.6 (nearest 9.23) | fonts among themselves 8.88 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 147 | 139 | 192 | 176 | 209 | 1.1 |  |
| partial stretch (inharmonicity) (c/partial) | -0.03 | 1.74 | 0.03 | 4.49 | 2.37 | 0.9 |  |
| level after 2 s held, re peak (dB) | -26.6 | -41.3 | -23.7 | -52.2 | -28.9 | 0.6 |  |
| high bands lag low bands by (ms) | 1 | 0 | 0 | -0.5 | -1.1 | 0.2 |  |
| transient coherence: attack spread over bands (ms) | 1.1 | 0 | 0 | 0.9 | 1 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.752 | 0.877 | 0.622 | 0.504 | 0.747 | 0.4 |  |
| partials 2-4 share (tristimulus 2) | 0.238 | 0.116 | 0.239 | 0.361 | 0.248 | 0.0 |  |
| partials 5+ share (tristimulus 3) | 0.004 | 0.014 | 0.196 | 0.119 | 0.025 | 0.7 |  |
| loudness range, velocity 40 to 127 (dB) | 19.5 | 16.6 | 16.7 | 16.7 | 20.6 | 1.4 |  |
| pitch wobble / vibrato (p-p) (c) | 6.3 | 3 | 2.2 | 3.3 | 4.7 | 0.6 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.013 (nearest 0.0105) | fonts among themselves 0.0124 | | | | 1.1 |  |

</details>

### 4 Honky-tonk Piano: honky-tonk

Distance 2.66 (rank 75 of 129); the fonts disagree 1.2 JND. Keys 21,27,33,39,45,51,57,63,69,75,81,87,93,99,105, velocities 40/64/96/127.

- **Bright, with little bass (centroid 14.6 vs 10.3…11.9 ERB; −15…−20 dB under 150 Hz re all four; fundamental share 0.22 vs 0.43…0.89)**: probable FAULT. README "Honky-tonk piano" says the closed case adds "no overall tilt". The upright's smaller board (piano_variants.md §3.1) explains a weaker bass, but not 15–20 dB against every font. FluidR3 through FluidSynth is only +1.0 ERB brighter. Check A0–A2 at v96.
- **Sharp onset (30.5 vs 8…13 dB/ms)**: the grand's hammer spike (notes_a p000) plus the tacks' knock (README). Shared possible FAULT.
- **Pitch scatter 12.5 vs 4.6…8.3 c; slower decay (6.4 vs 9.6…11.4 dB/s)**: DELIBERATE. These come from the detuned unisons and bad strings, which no longer lock, so the prompt decay goes (README; piano_variants.md §3.3).
- **Range 10.3 vs 16.7 dB**: mostly TiMidity's law (the grand gives 14.1). **Release 482 ms**: room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (14.3 ERB | 10.3 | 11.1 | 10.8 | 11.9), outside the fonts, 4.2 x.
2. **loudness range, velocity 40 to 127**: a narrower dynamic range (10.3 dB | 16.7 | 16.7 | 16.7 | 20.1), outside the fonts, 3.8 x.
3. **onset sharpness**: a sharper onset (37.3 dB/ms | 9.3 | 9.8 | 8.2 | 13.2), outside the fonts, 11.1 x.
4. **decay rate of a held note**: decays slower (6.6 dB/s | 10.9 | 10.3 | 11.4 | 9.6), outside the fonts, 2.7 x.
5. **release time (30 dB after note-off) [room]**: rings longer after the key (452 ms | 234 | 262 | 319 | 206), outside the fonts, 2.8 x.
6. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-85.1 dB | -124 | -122 | -125 | -114), outside the fonts, 6.2 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -20 | -3 | +5 | +3 | +0 | +3 |
| re TimGM6mb | -20 | -6 | -3 | -0 | +5 | +13 |
| re Jnsgm2 | -15 | -4 | -1 | +1 | +4 | +6 |
| re SGM | -20 | -8 | -5 | -2 | +6 | +17 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -8 -4 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +1.0 ERB, attack -0.3 ERB, velocity brightening -0.0 ERB, loudness range +3 dB, envelope 4.9 dB rms; TimGM6mb: centroid +0.7 ERB, attack +0.1 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 2.9 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening +0.5 ERB, loudness range +4 dB, envelope 2.7 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.25 | 0.45 | -0.14 | -0.34 | 0.27 | 0.8 |  |
| attack time (-20 to -1 dB) (ms) | 8 | 20.0 | 12.0 | 12.0 | 13.0 | 1.7 | yes |
| spectral shape of the attack, note for note | 9.2 (nearest 8.04) | fonts among themselves 9.17 | | | | 1.0 |  |
| brightness over the keyboard (ERB/oct) | 1.34 | 3.02 | 2.55 | 1.99 | 1.19 | 1.0 |  |
| key-to-key brightness scatter (ERB) | 1.36 | 1.08 | 1.15 | 1.51 | 2.79 | 0.0 |  |
| brightening from velocity 40 to 127 (ERB) | 0.04 | -0 | -0 | 0 | -0.05 | 0.1 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 14.3 | 10.3 | 11.1 | 10.8 | 11.9 | 4.2 | yes |
| decay rate of a held note (dB/s) | 6.6 | 10.9 | 10.3 | 11.4 | 9.6 | 2.7 | yes |
| onset delay (ms) | 13.0 | 6 | 5 | 6 | 9 | 0.7 |  |
| Plomp-Levelt dissonance of a note (log2) | -1.7 | -3.86 | -3.11 | -2.99 | -2.63 | 2.2 | yes |
| envelope curve, note for note (dB rms) | 12.5 (nearest 10.8) | fonts among themselves 6.12 | | | | 2.0 | yes |
| pitch re 12-TET A440 (median) (c) | -2.8 | -1.3 | 2.6 | 1.8 | 5.3 | 1.2 |  |
| pitch scatter from key to key (c) | 11.3 | 6.7 | 4.6 | 5.6 | 8.3 | 1.7 | yes |
| noisiness (spectral flatness) (dB) | -43.0 | -40.1 | -62.7 | -44.8 | -44.7 | 0.2 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -85.1 | -124 | -122 | -125 | -114 | 6.2 | yes |
| pitch at the start re settled (c) | 1.6 | -0.2 | -0.7 | -4.7 | -1 | 0.5 |  |
| stereo coherence | 0.592 | 0.444 | 0.979 | 1 | 1 | 1.4 |  |
| partial-to-partial jaggedness (dB) | 6 | 3.6 | 4.3 | 4.5 | 4.1 | 1.8 | yes |
| dip between legato notes (dB) | 7.7 | 8.2 | 11.1 | 5.1 | 7.9 | 0.1 |  |
| level evenness of a legato scale (dB) | 2.1 | 1.4 | 1.1 | 1 | 3.4 | 0.7 |  |
| level over the keyboard (dB/oct) | 0.12 | -2.03 | -0.34 | -0.76 | -0.89 | 0.9 |  |
| level against the other instruments (velocity 96) (dB) | -4.1 | -1.9 | -4 | -0.1 | 3.2 | 0.9 |  |
| key-to-key level scatter (dB) | 2.44 | 3.07 | 1.74 | 1.24 | 1.02 | 0.9 |  |
| odd / even partials (dB) | -0.2 | -7.2 | -8.1 | -4.3 | -5.9 | 3.2 | yes |
| onset jitter in a phrase (ms) | 17.6 | 30.5 | 27.9 | 6 | 5.2 | 0.1 |  |
| onset in a phrase (re the note-on) (ms) | 9 | 1 | 0 | 1 | 1 | 0.8 | yes |
| onset sharpness (dB/ms) | 37.3 | 9.3 | 9.8 | 8.2 | 13.2 | 11.1 | yes |
| overtone levels, note for note (dB rms) | 10.3 (nearest 9.69) | fonts among themselves 8.21 | | | | 1.3 |  |
| panning across the keys (dB/oct) | -0.06 | -0.93 | 0 | 0 | 0.01 | 0.1 |  |
| partial envelopes alike | 0.296 | 0.679 | 0.333 | 0.455 | 0.437 | 0.9 |  |
| release time (30 dB after note-off) (ms) | 452 | 234 | 262 | 319 | 206 | 2.8 | yes |
| spectrogram (1 - correlation), note for note | 0.233 (nearest 0.193) | fonts among themselves 0.187 | | | | 1.2 |  |
| spectral shape of the body, note for note | 12.3 (nearest 10.4) | fonts among themselves 10.3 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 171 | 132 | 72.0 | 126 | 164 | 0.7 |  |
| partial stretch (inharmonicity) (c/partial) | 1.19 | 1.77 | 1.75 | 1.95 | 3.28 | 0.9 | yes |
| level after 2 s held, re peak (dB) | -20.4 | -32.3 | -35.8 | -28.7 | -25.5 | 2.0 | yes |
| high bands lag low bands by (ms) | 1.5 | -2.6 | -1.3 | -3.3 | -3.3 | 0.9 | yes |
| transient coherence: attack spread over bands (ms) | 1.1 | 1.8 | 1 | 3.2 | 1.8 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.232 | 0.804 | 0.712 | 0.887 | 0.427 | 2.3 | yes |
| partials 2-4 share (tristimulus 2) | 0.213 | 0.16 | 0.149 | 0.088 | 0.193 | 0.8 |  |
| partials 5+ share (tristimulus 3) | 0.06 | 0.036 | 0.036 | 0.018 | 0.036 | 0.3 |  |
| loudness range, velocity 40 to 127 (dB) | 10.3 | 16.7 | 16.7 | 16.7 | 20.1 | 3.8 | yes |
| pitch wobble / vibrato (p-p) (c) | 12.1 | 5.9 | 9.7 | 14.2 | 10.5 | 0.4 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0184 (nearest 0.0162) | fonts among themselves 0.0172 | | | | 1.1 |  |

</details>

### 5 Electric Piano 1: electric-piano-1

Distance 2.84 (rank 60 of 129); the fonts disagree 1.2 JND. Keys 21,27,33,39,45,51,57,63,69,75,81,87,93,99,105, velocities 40/64/96/127.

- **Brightens strongly with velocity (+2.63 vs 0 ERB), brighter (12.1 vs 8.0…9.9 ERB)**: DELIBERATE. The pickup's bark grows with the swing (README "Electric piano 1"; research/tinepiano.md §4.6 quotes Muenster & Pfeifle: the timbre changes more than the volume). The fonts' TiMidity renders here do not move under FluidSynth either.
- **Key-to-key level (4.6 vs 0.9…1.4 dB) and +2.8 dB/oct toward the top**: DELIBERATE uneven voicing. tinepiano.md §5 gives ±3–6 dB between neighbours on a real harp. Still the outlier among the fonts: consider halving `PJ_TINEPIANO_WEAR`'s spread if it sounds lumpy.
- **Slow attack (36 vs 3…7 ms) with a sharp onset**: the room pattern from p002, perhaps with the Suitcase vibrato (on by default). Likely ARTIFACT.
- **Release 346 ms**: room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (2.17 ERB | -0 | 0 | -0 | -0.36), outside the fonts, 7.2 x.
2. **attack time (-20 to -1 dB)**: a slower attack (29.0 ms | 3 | 4 | 7 | 6), outside the fonts, 4.2 x.
3. **key-to-key level scatter**: less even from key to key (3.17 dB | 1.32 | 1.15 | 0.87 | 1.36), outside the fonts, 3.9 x.
4. **onset sharpness**: a sharper onset (43.8 dB/ms | 2.7 | 4.5 | 7.7 | 15.4), outside the fonts, 5.9 x.
5. **loudness range, velocity 40 to 127**: a narrower dynamic range (13.4 dB | 16.7 | 16.7 | 16.6 | 20.1), outside the fonts, 1.9 x.
6. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-83.1 dB | -121 | -121 | -123 | -110), outside the fonts, 6.0 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -3 | -2 | -1 | +0 | +7 | - |
| re TimGM6mb | -2 | +6 | +14 | +12 | -5 | -32 |
| re Jnsgm2 | -4 | -4 | -1 | +3 | +4 | - |
| re SGM | -5 | +0 | +1 | +1 | +1 | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -4 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.1 ERB, attack +0.0 ERB, velocity brightening +0.1 ERB, loudness range +3 dB, envelope 1.9 dB rms; TimGM6mb: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 0.7 dB rms; Jnsgm2: centroid -0.0 ERB, attack +0.0 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 0.4 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.16 | 0.35 | 0.05 | 0.15 | 0.02 | 0.1 |  |
| attack time (-20 to -1 dB) (ms) | 29.0 | 3 | 4 | 7 | 6 | 4.2 | yes |
| spectral shape of the attack, note for note | 11.3 (nearest 10.7) | fonts among themselves 11.4 | | | | 1.0 |  |
| brightness over the keyboard (ERB/oct) | 2.72 | 2.59 | 3.03 | 2.72 | 2.88 | 0.3 |  |
| key-to-key brightness scatter (ERB) | 1.33 | 0.619 | 0.476 | 0.624 | 1.06 | 2.4 | yes |
| brightening from velocity 40 to 127 (ERB) | 2.17 | -0 | 0 | -0 | -0.36 | 7.2 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 10.5 | 9.01 | 8.03 | 9.47 | 9.91 | 1.3 | yes |
| decay rate of a held note (dB/s) | 6.7 | 4.9 | 6.6 | 7.8 | 6.2 | 0.2 |  |
| onset delay (ms) | 11.0 | 0 | 1 | 4 | 2 | 0.9 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.04 | -2.54 | -5.21 | -4.92 | -4.92 | 1.4 |  |
| envelope curve, note for note (dB rms) | 13.8 (nearest 12.4) | fonts among themselves 6.68 | | | | 2.1 | yes |
| pitch re 12-TET A440 (median) (c) | -2.3 | 1.3 | 2.7 | 3.8 | -0.7 | 1.1 |  |
| pitch scatter from key to key (c) | 6.5 | 2.5 | 0.9 | 3.1 | 4.2 | 1.2 | yes |
| noisiness (spectral flatness) (dB) | -71.3 | -77.4 | -67.3 | -68.5 | -61.5 | 0.4 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -83.1 | -121 | -121 | -123 | -110 | 6.0 | yes |
| pitch at the start re settled (c) | 0.2 | 0 | -0.1 | -0.2 | -0.6 | 0.1 |  |
| stereo coherence | 0.786 | 0.975 | 0.997 | 1 | 0.409 | 0.7 |  |
| partial-to-partial jaggedness (dB) | 3.3 | 5.2 | 4.4 | 4.2 | 6.1 | 1.5 | yes |
| dip between legato notes (dB) | 12.6 | 8.3 | 14.9 | 11.5 | 12.6 | 0.2 |  |
| level evenness of a legato scale (dB) | 2.2 | 2.5 | 1.8 | 1.6 | 2.1 | 0.2 |  |
| level over the keyboard (dB/oct) | 1.4 | 1.52 | 0.63 | 0.37 | -0.88 | 0.8 |  |
| level against the other instruments (velocity 96) (dB) | 2 | -1.6 | 1.2 | 3.1 | 0.1 | 0.6 |  |
| key-to-key level scatter (dB) | 3.17 | 1.32 | 1.15 | 0.87 | 1.36 | 3.9 | yes |
| odd / even partials (dB) | -8.4 | -7.4 | -3.7 | -10.2 | -3.4 | 0.8 |  |
| onset jitter in a phrase (ms) | 39.6 | 23.9 | 20.8 | 13.4 | 39.0 | 1.3 |  |
| onset in a phrase (re the note-on) (ms) | 18.0 | -2 | 4 | 6 | 1 | 1.6 | yes |
| onset sharpness (dB/ms) | 43.8 | 2.7 | 4.5 | 7.7 | 15.4 | 5.9 | yes |
| overtone levels, note for note (dB rms) | 10.3 (nearest 8.55) | fonts among themselves 11 | | | | 0.9 |  |
| panning across the keys (dB/oct) | 0.14 | -5.33 | 0.11 | 0 | 0.18 | 0.0 |  |
| partial envelopes alike | 0.905 | 0.465 | 0.903 | 0.944 | 0.583 | 0.7 |  |
| release time (30 dB after note-off) (ms) | 348 | 60.0 | 69.0 | 191 | 202 | 1.8 | yes |
| spectrogram (1 - correlation), note for note | 0.249 (nearest 0.214) | fonts among themselves 0.154 | | | | 1.6 | yes |
| spectral shape of the body, note for note | 11.1 (nearest 10.6) | fonts among themselves 10.4 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 86.0 | 117 | 132 | 188 | 150 | 2.1 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.01 | 0.01 | 0.06 | 0 | 0.02 | 0.0 |  |
| level after 2 s held, re peak (dB) | -18.6 | -15.0 | -17.9 | -19.6 | -19.3 | 0.0 |  |
| high bands lag low bands by (ms) | -2.3 | 0 | 0 | 0 | 0 | 0.5 |  |
| transient coherence: attack spread over bands (ms) | 2.6 | 0 | 0 | 0 | 0 | 0.5 | yes |
| fundamental share (tristimulus 1) | 0.935 | 0.832 | 0.991 | 0.654 | 0.564 | 0.9 |  |
| partials 2-4 share (tristimulus 2) | 0.065 | 0.167 | 0.008 | 0.346 | 0.408 | 1.0 |  |
| partials 5+ share (tristimulus 3) | 0 | 0.001 | 0.001 | 0 | 0.001 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 13.4 | 16.7 | 16.7 | 16.6 | 20.1 | 1.9 | yes |
| pitch wobble / vibrato (p-p) (c) | 4 | 0.3 | 0.3 | 0.9 | 10.4 | 0.7 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.00752 (nearest 0.0049) | fonts among themselves 0.00819 | | | | 0.9 |  |

</details>

### 6 Electric Piano 2: electric-piano-2

Distance 3.86 (rank 23 of 129); the fonts disagree 1.5 JND. Keys 21,27,33,39,45,51,57,63,69,75,81,87,93,99,105, velocities 40/64/96/127.

- **Velocity barely changes level (2.7 vs 16.6 dB) but brightens hugely (+4.3 ERB)**: DELIBERATE. This is the FM synth's own velocity law (README "Electric piano 2"). The fonts' range is TiMidity's law.
- **Speaks late (onset 44 vs 1…9 ms; attack 148 vs 5…53 ms; phrase onset 26 ms)** and **key scatter 3.8 vs 0.6…1.3 dB**: likely FAULT. A plausible common cause: each key is three whole FM synth voices 1/7 Hz apart with free-running phases. Their sum depends on the starting phases, partly cancelling at the start of some keys and swelling over a 7 s beat. Test with `PJ_DX_ONE=128` (a single voice).
- **Louder (+3.1 vs −4.5…−0.2 dB)**: minor; a trim.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (1.6 dB | 16.7 | 16.6 | 16.6 | 20.2), outside the fonts, 8.4 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (4.2 ERB | 0 | 0 | -0 | -0.02), outside the fonts, 14.0 x.
3. **key-to-key level scatter**: less even from key to key (3.38 dB | 0.86 | 0.6 | 1.22 | 1.34), outside the fonts, 4.7 x.
4. **onset delay**: speaks later (43.0 ms | 2 | 1 | 1 | 9), outside the fonts, 4.2 x.
5. **onset in a phrase (re the note-on)**: speaks later in a phrase (40.0 ms | -0 | -1 | 0 | 4), outside the fonts, 4.0 x.
6. **attack time (-20 to -1 dB)**: a slower attack (142 ms | 5 | 17.0 | 7 | 53.0), outside the fonts, 2.2 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -12 | -6 | -5 | -2 | +3 | +12 |
| re TimGM6mb | -4 | -5 | +2 | +1 | -4 | +6 |
| re Jnsgm2 | -5 | +1 | +2 | +4 | -2 | -3 |
| re SGM | -6 | -6 | -4 | -4 | -2 | +13 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -1 -1 +0 +1; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 3.0 dB rms; TimGM6mb: centroid -0.2 ERB, attack -0.6 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 0.7 dB rms; Jnsgm2: centroid -0.9 ERB, attack -0.1 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 2.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.18 | 0.08 | 2.43 | 1.05 | 3.19 | 1.0 |  |
| attack time (-20 to -1 dB) (ms) | 142 | 5 | 17.0 | 7 | 53.0 | 2.2 | yes |
| spectral shape of the attack, note for note | 16 (nearest 14.6) | fonts among themselves 15.9 | | | | 1.0 |  |
| brightness over the keyboard (ERB/oct) | 3.22 | 3.22 | 3.66 | 3.44 | 3.01 | 0.3 |  |
| key-to-key brightness scatter (ERB) | 1.71 | 0.739 | 1.27 | 1.2 | 1.57 | 1.1 | yes |
| brightening from velocity 40 to 127 (ERB) | 4.2 | 0 | 0 | -0 | -0.02 | 14.0 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 12.7 | 11.7 | 8.82 | 10.6 | 14.4 | 0.5 |  |
| decay rate of a held note (dB/s) | 6.4 | 5.6 | 9 | 11.4 | 9.6 | 1.1 |  |
| onset delay (ms) | 43.0 | 2 | 1 | 1 | 9 | 4.2 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.58 | -6.27 | -4.47 | -4.06 | -4.51 | 0.8 | yes |
| envelope curve, note for note (dB rms) | 15.3 (nearest 14.2) | fonts among themselves 7.11 | | | | 2.2 | yes |
| pitch re 12-TET A440 (median) (c) | -3.2 | -4 | 2.2 | -1.9 | 2.7 | 0.8 |  |
| pitch scatter from key to key (c) | 5 | 4 | 1.2 | 6.4 | 1.3 | 0.8 |  |
| noisiness (spectral flatness) (dB) | -57.5 | -75.5 | -65.5 | -58.3 | -65.8 | 0.9 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -76.7 | -122 | -119 | -122 | -105 | 5.0 | yes |
| pitch at the start re settled (c) | 0.2 | -0.2 | -0.3 | -10.8 | 1.8 | 0.1 |  |
| stereo coherence | 0.458 | 0.936 | 1 | 1 | 0.158 | 1.2 |  |
| partial-to-partial jaggedness (dB) | 5 | 5.5 | 8.3 | 8.5 | 2.7 | 0.7 |  |
| dip between legato notes (dB) | 10.3 | 6.5 | 7 | 14.1 | 10.1 | 0.5 |  |
| level evenness of a legato scale (dB) | 1.3 | 0.9 | 1.3 | 1.2 | 0.7 | 0.2 |  |
| level over the keyboard (dB/oct) | -0.34 | -0.93 | -0.37 | -1.32 | 0.3 | 0.3 |  |
| level against the other instruments (velocity 96) (dB) | -2.4 | -0.9 | -2.6 | -0.1 | -3.3 | 0.4 |  |
| key-to-key level scatter (dB) | 3.38 | 0.86 | 0.6 | 1.22 | 1.34 | 4.7 | yes |
| odd / even partials (dB) | -7.8 | -6.6 | 0.6 | -4.4 | -5.3 | 0.8 | yes |
| onset jitter in a phrase (ms) | 43.8 | 37.5 | 49.0 | 1 | 43.0 | 0.1 |  |
| onset in a phrase (re the note-on) (ms) | 40.0 | -0 | -1 | 0 | 4 | 4.0 | yes |
| onset sharpness (dB/ms) | 34.0 | 23.4 | 2.1 | 21.3 | 18.9 | 1.3 | yes |
| overtone levels, note for note (dB rms) | 17.5 (nearest 15.5) | fonts among themselves 9.33 | | | | 1.9 | yes |
| panning across the keys (dB/oct) | -0.43 | -0.11 | 0 | 0 | -0.06 | 0.4 |  |
| partial envelopes alike | 0.425 | 0.504 | 1 | 0.337 | 0.441 | 0.1 |  |
| release time (30 dB after note-off) (ms) | 600 | 223 | 108 | 313 | 187 | 2.0 | yes |
| spectrogram (1 - correlation), note for note | 0.322 (nearest 0.235) | fonts among themselves 0.261 | | | | 1.2 |  |
| spectral shape of the body, note for note | 13.3 (nearest 12.6) | fonts among themselves 12.3 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 226 | 41.0 | 142 | 184 | 183 | 0.5 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.58 | -0.11 | 0 | 0.01 | -0.36 | 1.3 | yes |
| level after 2 s held, re peak (dB) | -14.8 | -19.4 | -23.3 | -28.4 | -22.8 | 1.8 | yes |
| high bands lag low bands by (ms) | -14.3 | 0 | 0 | 0 | -1.1 | 2.9 | yes |
| transient coherence: attack spread over bands (ms) | 10.6 | 0 | 0 | 0.8 | 14.9 | 1.4 |  |
| fundamental share (tristimulus 1) | 0.806 | 0.737 | 0.423 | 0.839 | 0.582 | 0.7 |  |
| partials 2-4 share (tristimulus 2) | 0.145 | 0.25 | 0.512 | 0.129 | 0.324 | 0.7 |  |
| partials 5+ share (tristimulus 3) | 0.002 | 0.002 | 0.032 | 0.01 | 0.013 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 1.6 | 16.7 | 16.6 | 16.6 | 20.2 | 8.4 | yes |
| pitch wobble / vibrato (p-p) (c) | 10.9 | 11.4 | 0 | 5.8 | 10.4 | 0.5 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0156 (nearest 0.0127) | fonts among themselves 0.0163 | | | | 1.0 |  |

</details>

### 7 Harpsichord: harpsichord

Distance 3.99 (rank 21 of 129); the fonts disagree 1.2 JND. Keys 21,27,33,39,45,51,57,63,69,75,81,87,93,99,105, velocities 40/64/96/127.

- **No dynamics (−0.6 vs 16.7 dB)**: DELIBERATE, as on the instrument (README "Harpsichord": the plectrum lets go at a fixed force). The fonts' range is TiMidity's law.
- **Key scatter (4.3 vs 0.4…1.7 dB) and uneven legato scale (3.5 vs 0.3…0.8 dB)**: likely FAULT. Candidates: the 0.8 % split between the two swing directions, which beats at ~3.5 Hz at A4, deep inside a 0.5 s window, and the rows plucking one after another.
- **Darker above 2.5 kHz (−3…−9 dB re all four), brightens slower up the keys (2.0 vs 2.8…3.4 ERB/oct)**: likely FAULT, known. README: the board is still the grand's, and the treble plucks at a third of the string. TimGM6mb and Jnsgm2 are 1.3–1.4 ERB darker under FluidSynth, which narrows the gap.
- **Quieter (−5.9 vs −4.7…−1.2 dB)**: a trim.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (-2.2 dB | 16.6 | 16.7 | 16.7 | 20.1), outside the fonts, 10.8 x.
2. **key-to-key level scatter**: less even from key to key (5.03 dB | 0.99 | 0.77 | 0.42 | 1.66), outside the fonts, 6.7 x.
3. **level against the other instruments (velocity 96)**: quieter (-6.9 dB | -1.3 | -4.5 | -1.1 | -2.3), outside the fonts, 3.0 x.
4. **brightness (spectral centroid, velocity 96)**: darker (9.99 ERB | 12.9 | 13.5 | 13.7 | 11.5), outside the fonts, 2.9 x.
5. **attack time (-20 to -1 dB)**: a faster attack (10.5 ms | 21.0 | 39.0 | 49.0 | 37.0), outside the fonts, 3.0 x.
6. **brightness over the keyboard**: brightens slower up the keyboard (1.48 ERB/oct | 2.84 | 3.39 | 3.14 | 2.9), outside the fonts, 5.1 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +6 | +6 | +4 | +5 | -5 | -9 |
| re TimGM6mb | +3 | +5 | +5 | +6 | -5 | -9 |
| re Jnsgm2 | +3 | +1 | +2 | +6 | -4 | -5 |
| re SGM | +4 | +4 | +4 | +5 | -3 | -8 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli +3 +1 +0 +1; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.2 ERB, attack -0.8 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 0.6 dB rms; TimGM6mb: centroid -1.4 ERB, attack +0.0 ERB, velocity brightening -0.0 ERB, loudness range +3 dB, envelope 0.9 dB rms; Jnsgm2: centroid -1.3 ERB, attack -4.1 ERB, velocity brightening +1.7 ERB, loudness range +4 dB, envelope 1.0 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.09 | 1.55 | 1.3 | 5.65 | 2.25 | 0.9 | yes |
| attack time (-20 to -1 dB) (ms) | 10.5 | 21.0 | 39.0 | 49.0 | 37.0 | 3.0 | yes |
| spectral shape of the attack, note for note | 15.9 (nearest 13) | fonts among themselves 10.9 | | | | 1.5 |  |
| brightness over the keyboard (ERB/oct) | 1.48 | 2.84 | 3.39 | 3.14 | 2.9 | 5.1 | yes |
| key-to-key brightness scatter (ERB) | 2.4 | 2.11 | 1.25 | 0.859 | 1.43 | 1.7 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.54 | -0 | -0 | 0 | -0.24 | 1.8 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 9.99 | 12.9 | 13.5 | 13.7 | 11.5 | 2.9 | yes |
| decay rate of a held note (dB/s) | 10.6 | 12.4 | 11.1 | 7.9 | 9.4 | 0.2 |  |
| onset delay (ms) | 26.0 | 7 | 11.0 | 9 | 8 | 1.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.4 | -3.24 | -3.34 | -3.41 | -4.01 | 2.7 | yes |
| envelope curve, note for note (dB rms) | 9.53 (nearest 8.96) | fonts among themselves 8.33 | | | | 1.1 |  |
| pitch re 12-TET A440 (median) (c) | -1.4 | -1.2 | 2.7 | 10.5 | -4 | 0.3 |  |
| pitch scatter from key to key (c) | 4.7 | 2.5 | 1 | 1.3 | 3.4 | 0.9 |  |
| noisiness (spectral flatness) (dB) | -60.2 | -35.8 | -52.0 | -48.0 | -40.6 | 2.0 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -112 | -122 | -117 | -119 | -104 | 0.7 |  |
| pitch at the start re settled (c) | -0.5 | 5.8 | 0 | 1.3 | 3.1 | 0.5 |  |
| stereo coherence | 0.97 | 1 | 1 | 1 | 1 | 0.4 |  |
| partial-to-partial jaggedness (dB) | 5.7 | 4.1 | 6.5 | 4.7 | 3.9 | 1.0 |  |
| dip between legato notes (dB) | 6.2 | 7 | 6.2 | 3.8 | 7.7 | 0.2 |  |
| level evenness of a legato scale (dB) | 3.2 | 0.3 | 0.8 | 0.8 | 0.8 | 2.4 | yes |
| level over the keyboard (dB/oct) | -0.38 | -0.34 | -0.18 | 0.26 | 0.81 | 0.4 |  |
| level against the other instruments (velocity 96) (dB) | -6.9 | -1.3 | -4.5 | -1.1 | -2.3 | 3.0 | yes |
| key-to-key level scatter (dB) | 5.03 | 0.99 | 0.77 | 0.42 | 1.66 | 6.7 | yes |
| odd / even partials (dB) | -6.2 | -0.4 | 0.2 | -4.9 | -3.6 | 1.6 | yes |
| onset jitter in a phrase (ms) | 6.5 | 52.2 | 3.2 | 23.7 | 12.5 | 0.5 |  |
| onset in a phrase (re the note-on) (ms) | 25.0 | -2 | 1 | 0 | 0 | 2.5 | yes |
| onset sharpness (dB/ms) | 10.6 | 4.6 | 18.1 | 7.5 | 5.9 | 0.6 |  |
| overtone levels, note for note (dB rms) | 14.1 (nearest 12.6) | fonts among themselves 10.1 | | | | 1.4 |  |
| panning across the keys (dB/oct) | -0.07 | 0 | 0.01 | 0 | 0.01 | 0.1 |  |
| partial envelopes alike | 0.815 | 0.735 | 1 | 0.458 | 0.839 | 0.1 |  |
| release time (30 dB after note-off) (ms) | 213 | 52.0 | 60.0 | 276 | 121 | 1.1 |  |
| spectrogram (1 - correlation), note for note | 0.218 (nearest 0.185) | fonts among themselves 0.174 | | | | 1.3 |  |
| spectral shape of the body, note for note | 11.5 (nearest 10.6) | fonts among themselves 9.06 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 171 | 130 | 130 | 201 | 169 | 0.7 |  |
| partial stretch (inharmonicity) (c/partial) | 0.41 | 0.85 | 0 | 0.69 | 0.31 | 0.2 |  |
| level after 2 s held, re peak (dB) | -27.3 | -31.1 | -23.9 | -18.4 | -21.2 | 0.7 |  |
| high bands lag low bands by (ms) | 0 | -2.5 | -8 | -8 | -7.5 | 1.6 |  |
| transient coherence: attack spread over bands (ms) | 1.2 | 5.1 | 7.4 | 6.1 | 4.3 | 0.9 | yes |
| fundamental share (tristimulus 1) | 0.482 | 0.266 | 0.402 | 0.438 | 0.576 | 0.4 |  |
| partials 2-4 share (tristimulus 2) | 0.207 | 0.541 | 0.477 | 0.405 | 0.402 | 3.3 | yes |
| partials 5+ share (tristimulus 3) | 0.027 | 0.135 | 0.1 | 0.106 | 0.082 | 1.1 | yes |
| loudness range, velocity 40 to 127 (dB) | -2.2 | 16.6 | 16.7 | 16.7 | 20.1 | 10.8 | yes |
| pitch wobble / vibrato (p-p) (c) | 4.3 | 4.8 | 0 | 4.2 | 3 | 0.1 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0174 (nearest 0.0148) | fonts among themselves 0.0118 | | | | 1.5 |  |

</details>

### 8 clavi: clavi

Distance 3.73 (rank 28 of 129); the fonts disagree 1.2 JND. Keys 21,27,33,39,45,51,57,63,69,75,81,87,93,99,105, velocities 40/64/96/127.

- **Odd/even partials (−13.4 vs −1.6…−4.1 dB)**: possible FAULT. The default pickup pair's comb is unverified. README "clavi" says the recorded D#3's first comb gap lies two partials lower than the model's.
- **Release 708 ms vs 5…44**: the room accounts for ~300–400 ms (cf. p002, p004). The rest is the thunk and the dropped string, which research/clavi.md says the yarn kills in 0.1–0.3 s. Probably too long: FAULT worth checking.
- **Brightens with velocity (+3.7 ERB)**: DELIBERATE (README: "the clavi's dynamics are mostly its brightness"). Jnsgm2 through FluidSynth gives +3.0 ERB, so this is a TiMidity ARTIFACT.
- **Slow attack (85 vs 1…4 ms)**: likely the beating of the two swing directions (up to 15 dB deep, 0.5–2 Hz) moving the peak, plus the room. Mostly ARTIFACT of the measure.
- **−20…−30 dB above 6 kHz**: DELIBERATE (stage 1 rolls off from 5 kHz).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (3.03 ERB | 0 | 0 | 0 | -0.05), outside the fonts, 10.1 x.
2. **key-to-key level scatter**: less even from key to key (4.01 dB | 1.05 | 0.84 | 0.7 | 1.65), outside the fonts, 6.1 x.
3. **attack time (-20 to -1 dB)**: a slower attack (45.0 ms | 1 | 1 | 1 | 4), outside the fonts, 5.5 x.
4. **loudness range, velocity 40 to 127**: a narrower dynamic range (10.6 dB | 16.6 | 16.6 | 16.7 | 20.1), outside the fonts, 3.5 x.
5. **release time (30 dB after note-off) [room]**: rings longer after the key (656 ms | 44.0 | 6 | 5 | 5), outside the fonts, 4.4 x.
6. **odd / even partials**: more even partials (-14.3 dB | -4.1 | -2.8 | -3 | -1.6), outside the fonts, 5.7 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -7 | +3 | +8 | +8 | -3 | -21 |
| re TimGM6mb | +7 | +12 | +18 | +14 | -6 | -29 |
| re Jnsgm2 | +8 | +12 | +18 | +14 | -6 | -30 |
| re SGM | +5 | +5 | +14 | +9 | -2 | -23 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -10 -3 +0 +1; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening +0.2 ERB, loudness range +4 dB, envelope 0.2 dB rms; TimGM6mb: centroid -0.6 ERB, attack +0.4 ERB, velocity brightening +0.4 ERB, loudness range +4 dB, envelope 2.7 dB rms; Jnsgm2: centroid -1.4 ERB, attack +0.6 ERB, velocity brightening +3.0 ERB, loudness range +6 dB, envelope 6.0 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.09 | 0.61 | 0.02 | 0.05 | 2.83 | 0.2 |  |
| attack time (-20 to -1 dB) (ms) | 45.0 | 1 | 1 | 1 | 4 | 5.5 | yes |
| spectral shape of the attack, note for note | 14.5 (nearest 11.2) | fonts among themselves 8.46 | | | | 1.7 | yes |
| brightness over the keyboard (ERB/oct) | 0.92 | 2.21 | 1.75 | 1.8 | 2.05 | 3.3 | yes |
| key-to-key brightness scatter (ERB) | 1.99 | 1.19 | 1.5 | 1.32 | 1.02 | 3.1 | yes |
| brightening from velocity 40 to 127 (ERB) | 3.03 | 0 | 0 | 0 | -0.05 | 10.1 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 14.1 | 13.2 | 15.9 | 15.9 | 14.7 | 0.9 |  |
| decay rate of a held note (dB/s) | 6.9 | 7.2 | 11.1 | 11.1 | 9.4 | 1.8 |  |
| onset delay (ms) | 14.0 | 2 | 2 | 2 | 1 | 1.2 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.22 | -4.69 | -2.59 | -3.06 | -4.41 | 0.5 |  |
| envelope curve, note for note (dB rms) | 18.8 (nearest 18.1) | fonts among themselves 9.53 | | | | 2.0 | yes |
| pitch re 12-TET A440 (median) (c) | 0 | 0.4 | 2.9 | 3 | -1.6 | 0.4 |  |
| pitch scatter from key to key (c) | 0.4 | 1.5 | 0.3 | 0.6 | 2.4 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -53.8 | -48.6 | -53.2 | -52.4 | -47.0 | 1.1 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -73.3 | -124 | -118 | -118 | -107 | 5.1 | yes |
| pitch at the start re settled (c) | 0.6 | -0.5 | 0 | 0 | -0.6 | 0.2 |  |
| stereo coherence | 0.407 | 1 | 1 | 1 | 0.62 | 3.1 | yes |
| partial-to-partial jaggedness (dB) | 4.85 | 2.8 | 3 | 3.1 | 2.9 | 1.9 | yes |
| dip between legato notes (dB) | 10.1 | 4.7 | 11.5 | 11.7 | 14.9 | 0.3 |  |
| level evenness of a legato scale (dB) | 1.1 | 1.9 | 2.3 | 1.5 | 0.8 | 0.6 |  |
| level over the keyboard (dB/oct) | 0.21 | 0.26 | 0.7 | 0.31 | 0.28 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | -2.5 | -3.1 | -2.6 | -3.2 | -6.4 | 0.3 |  |
| key-to-key level scatter (dB) | 4.01 | 1.05 | 0.84 | 0.7 | 1.65 | 6.1 | yes |
| odd / even partials (dB) | -14.3 | -4.1 | -2.8 | -3 | -1.6 | 5.7 | yes |
| onset jitter in a phrase (ms) | 36.2 | 1 | 51.9 | 22.6 | 0.9 | 1.0 |  |
| onset in a phrase (re the note-on) (ms) | 16.0 | -2 | -3 | -3 | -1 | 1.9 | yes |
| onset sharpness (dB/ms) | 24.9 | 17.1 | 11.4 | 11.2 | 4.7 | 2.2 | yes |
| overtone levels, note for note (dB rms) | 18.7 (nearest 10.6) | fonts among themselves 9.73 | | | | 1.9 |  |
| panning across the keys (dB/oct) | -0.25 | 0 | 0 | 0 | -0.32 | 0.2 |  |
| partial envelopes alike | 0.677 | 0.997 | 1 | 1 | 0.906 | 3.2 | yes |
| release time (30 dB after note-off) (ms) | 656 | 44.0 | 6 | 5 | 5 | 4.4 | yes |
| spectrogram (1 - correlation), note for note | 0.577 (nearest 0.389) | fonts among themselves 0.24 | | | | 2.4 | yes |
| spectral shape of the body, note for note | 14.8 (nearest 12.3) | fonts among themselves 10.4 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 186 | 115 | 11.0 | 11.0 | 102 | 1.5 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.81 | 0.01 | 0 | 0 | 0.01 | 1.6 | yes |
| level after 2 s held, re peak (dB) | -17.9 | -24.0 | -24.5 | -25.6 | -35.4 | 1.3 | yes |
| high bands lag low bands by (ms) | -1.3 | 0 | 0 | 0 | 0 | 0.3 |  |
| transient coherence: attack spread over bands (ms) | 6 | 0 | 0 | 0 | 0 | 1.2 | yes |
| fundamental share (tristimulus 1) | 0.248 | 0.29 | 0.109 | 0.114 | 0.198 | 1.0 |  |
| partials 2-4 share (tristimulus 2) | 0.244 | 0.439 | 0.527 | 0.532 | 0.296 | 2.0 | yes |
| partials 5+ share (tristimulus 3) | 0.008 | 0.023 | 0.22 | 0.219 | 0.196 | 2.0 |  |
| loudness range, velocity 40 to 127 (dB) | 10.6 | 16.6 | 16.6 | 16.7 | 20.1 | 3.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 3.3 | 0.7 | 0 | 0 | 5.2 | 0.6 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0225 (nearest 0.0197) | fonts among themselves 0.0127 | | | | 1.8 | yes |

</details>

### 9 Celesta: celesta

Distance 2.01 (rank 104 of 129); the fonts disagree 1.0 JND. Keys 48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Stops sooner after the key (205 vs 658…841 ms)**: DELIBERATE. The celesta has dampers (README "Celesta"); the fonts let the sample ring.
- **Decays slower while held (7 vs 11…24 dB/s)**: DELIBERATE. The box ring times are fitted to a recording (research/celesta.md: ~11 s at middle C).
- **Uneven in tone and level from key to key (1.1 vs 0.2…0.55 ERB; 3.4 vs 0.1…2.7 dB) and −2.3 dB/oct toward the top**: mostly the fitted model. SGM is also uneven (2.7 dB). Low priority; listen to the top octave's level.
- **Louder (+7.6 dB)**: the fonts span −8…+4, so not a clear fault.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: stops sooner after the key (205 ms | 841 | 759 | 776 | 658), outside the fonts, 7.6 x.
2. **key-to-key brightness scatter**: less even in tone from key to key (1.11 ERB | 0.189 | 0.212 | 0.192 | 0.554), outside the fonts, 4.5 x.
3. **brightening from velocity 40 to 127**: brightens more with velocity (0.84 ERB | -0 | 0 | -0 | -0.3), outside the fonts, 2.8 x.
4. **loudness range, velocity 40 to 127**: a narrower dynamic range (13.1 dB | 16.7 | 16.7 | 16.6 | 20.2), outside the fonts, 2.0 x.
5. **decay rate of a held note**: decays slower (7.1 dB/s | 22.4 | 15.1 | 11.3 | 23.6), outside the fonts, 2.6 x.
6. **sounding length of a 100 ms note [room]**: shorter staccato (214 ms | 326 | 400 | 400 | 294), outside the fonts, 3.0 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +1 | +1 | +8 | +1 | -5 | -9 |
| re TimGM6mb | +9 | +6 | +7 | +5 | -10 | -17 |
| re Jnsgm2 | +0 | +1 | +4 | -4 | -1 | +18 |
| re SGM | +16 | +10 | +12 | +4 | -6 | -16 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -10 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 0.8 dB rms; TimGM6mb: centroid -0.2 ERB, attack -0.3 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 1.4 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.1 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 1.0 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.08 | 1.03 | 5.67 | 0.32 | 1.83 | 0.5 | yes |
| attack time (-20 to -1 dB) (ms) | 20.0 | 6 | 4 | 20.0 | 3 | 1.5 |  |
| spectral shape of the attack, note for note | 15.1 (nearest 12.8) | fonts among themselves 12.5 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 2.73 | 3.55 | 2.97 | 3.54 | 3.17 | 2.1 | yes |
| key-to-key brightness scatter (ERB) | 1.11 | 0.189 | 0.212 | 0.192 | 0.554 | 4.5 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.84 | -0 | 0 | -0 | -0.3 | 2.8 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 11.0 | 11.0 | 12.0 | 11.0 | 11.4 | 0.3 |  |
| decay rate of a held note (dB/s) | 7.1 | 22.4 | 15.1 | 11.3 | 23.6 | 2.6 | yes |
| onset delay (ms) | 10.0 | 4 | 1 | 4 | 1 | 0.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | -8.97 | -9.47 | -6.8 | -9.97 | -4.21 | 0.3 |  |
| envelope curve, note for note (dB rms) | 16 (nearest 10.6) | fonts among themselves 17.3 | | | | 0.9 |  |
| pitch re 12-TET A440 (median) (c) | 0 | 0.2 | 3.7 | 13.9 | 1.1 | 0.4 |  |
| pitch scatter from key to key (c) | 0 | 0.6 | 0.6 | 1.5 | 0.7 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -78.9 | -68.5 | -56.9 | -71.7 | -44.4 | 1.2 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -103 | -123 | -117 | -123 | -99.5 | 1.4 |  |
| pitch at the start re settled (c) | 0.1 | -0.4 | 0.6 | -0.5 | 0.4 | 0.0 |  |
| stereo coherence | 0.991 | 0.992 | 1 | 1 | 0.921 | 0.1 |  |
| partial-to-partial jaggedness (dB) | 9.2 | 5.8 | 9.3 | 10.3 | 4.4 | 0.6 |  |
| dip between legato notes (dB) | 9.5 | 4.3 | 4.4 | 9.2 | 6.4 | 1.7 |  |
| level evenness of a legato scale (dB) | 1 | 0.7 | 0.9 | 0.8 | 1 | 0.1 |  |
| level over the keyboard (dB/oct) | -1.6 | 0.38 | 0.54 | 0.32 | -0.78 | 2.0 | yes |
| level against the other instruments (velocity 96) (dB) | 1.2 | 0.2 | -3.5 | 3.9 | -7 | 0.5 |  |
| key-to-key level scatter (dB) | 2.26 | 0.5 | 0.25 | 0.08 | 2.67 | 1.5 |  |
| odd / even partials (dB) | 29.9 | -4.7 | -5.3 | 9.2 | -13.5 | 3.1 | yes |
| onset jitter in a phrase (ms) | 20.4 | 1.3 | 68.0 | 15.8 | 0.9 | 0.4 |  |
| onset in a phrase (re the note-on) (ms) | 5 | -0 | 46.0 | 4 | 1 | 0.1 |  |
| onset sharpness (dB/ms) | 53.4 | 25.2 | 3.2 | 9.5 | 5.5 | 4.2 | yes |
| overtone levels, note for note (dB rms) | 12.8 (nearest 9.99) | fonts among themselves 14.8 | | | | 0.9 |  |
| panning across the keys (dB/oct) | -0.22 | 0.17 | -0 | -0 | -0.44 | 0.2 |  |
| partial envelopes alike | 0.979 | 0.966 | 0.958 | 0.845 | 0.984 | 0.2 |  |
| release time (30 dB after note-off) (ms) | 205 | 841 | 759 | 776 | 658 | 7.6 | yes |
| spectrogram (1 - correlation), note for note | 0.284 (nearest 0.187) | fonts among themselves 0.317 | | | | 0.9 |  |
| spectral shape of the body, note for note | 10.1 (nearest 8.87) | fonts among themselves 11.9 | | | | 0.9 |  |
| sounding length of a 100 ms note (ms) | 214 | 326 | 400 | 400 | 294 | 3.0 | yes |
| level after 2 s held, re peak (dB) | -19.4 | -48.9 | -28.8 | -23.4 | -56.5 | 1.2 | yes |
| high bands lag low bands by (ms) | -2 | -5.3 | -4 | -0.7 | 0 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 2.2 | 2.8 | 4.2 | 0.5 | 0 | 0.1 |  |
| fundamental share (tristimulus 1) | 1 | 1 | 1 | 1 | 0.988 | 0.0 |  |
| partials 2-4 share (tristimulus 2) | 0 | 0 | 0 | 0 | 0.011 | 0.0 |  |
| partials 5+ share (tristimulus 3) | 0 | 0 | 0 | 0 | 0.001 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 13.1 | 16.7 | 16.7 | 16.6 | 20.2 | 2.0 | yes |
| pitch wobble / vibrato (p-p) (c) | 0.4 | 0.9 | 2.1 | 0.6 | 2.8 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0137 (nearest 0.00868) | fonts among themselves 0.0139 | | | | 1.0 |  |

</details>

### 10 Glockenspiel: glockenspiel

Distance 2.30 (rank 90 of 129); the fonts disagree 1.6 JND. Keys 48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Spectrum: −24…−39 dB below 400 Hz, spectrogram far from the fonts (0.92 vs 0.37)**: ARTIFACT of the compass. The glockenspiel sounds G5–C8 (README "Chromatic percussion"), so probe keys 48–76 fold up by octaves, while the fonts play at key pitch. This was chosen DELIBERATELY.
- **Slow attack (18 vs 1…3 ms)**: hard mallets on bare steel should peak at once. Probably the room build-up pattern (cf. p002). Check dry.
- **Louder (+3.7 vs −6.9…−2.8 dB), noisier**: the noise may be the room at high pitch. The level is a trim; research/chromatic_percussion.md §9 says levels were set only "roughly even".

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **attack time (-20 to -1 dB)**: a slower attack (18.0 ms | 1 | 2 | 1 | 3), outside the fonts, 4.6 x.
2. **level against the other instruments (velocity 96)**: louder (-0.2 dB | -3.3 | -5.1 | -2.6 | -5.7), outside the fonts, 2.6 x.
3. **loudness range, velocity 40 to 127**: a narrower dynamic range (12.2 dB | 16.6 | 16.6 | 16.7 | 20.1), outside the fonts, 2.5 x.
4. **noisiness (spectral flatness)**: noisier (-37.8 dB | -59.6 | -55.0 | -53.0 | -61.9), outside the fonts, 4.4 x.
5. **Plomp-Levelt dissonance of a note**: rougher (-2.98 log2 | -4.59 | -4.41 | -4.9 | -5.88), outside the fonts, 2.4 x.
6. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.926, nearest 0.888; the fonts among themselves 0.367), outside.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -27 | -24 | -4 | +15 | +8 | +2 |
| re TimGM6mb | -30 | -25 | -5 | +14 | +17 | -5 |
| re Jnsgm2 | -39 | -36 | -14 | +13 | +13 | +21 |
| re SGM | -36 | -26 | -4 | +18 | +6 | -0 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -10 -5 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack -0.1 ERB, velocity brightening +1.4 ERB, loudness range +5 dB, envelope 0.4 dB rms; TimGM6mb: centroid -0.9 ERB, attack +0.3 ERB, velocity brightening +0.6 ERB, loudness range +4 dB, envelope 2.0 dB rms; Jnsgm2: centroid -0.7 ERB, attack -1.3 ERB, velocity brightening +1.4 ERB, loudness range +7 dB, envelope 1.0 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 4.02 | 0.8 | 2.16 | 5.26 | -0.62 | 0.9 |  |
| attack time (-20 to -1 dB) (ms) | 18.0 | 1 | 2 | 1 | 3 | 4.6 | yes |
| spectral shape of the attack, note for note | 19 (nearest 15.8) | fonts among themselves 13.9 | | | | 1.4 |  |
| brightness over the keyboard (ERB/oct) | -0.34 | 5.53 | 3.24 | 2.22 | 6.26 | 2.3 | yes |
| key-to-key brightness scatter (ERB) | 0.986 | 1.93 | 1.69 | 0.721 | 1.56 | 1.1 |  |
| brightening from velocity 40 to 127 (ERB) | 0.8 | 0.94 | 0.06 | -0 | 0.7 | 0.9 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 20.7 | 21.2 | 21.1 | 18.2 | 27.2 | 0.1 |  |
| decay rate of a held note (dB/s) | 12.6 | 18.4 | 17.2 | 19.8 | 13.4 | 1.8 |  |
| onset delay (ms) | 9 | 9 | 0 | 13.0 | 1 | 0.4 |  |
| Plomp-Levelt dissonance of a note (log2) | -2.98 | -4.59 | -4.41 | -4.9 | -5.88 | 2.4 | yes |
| envelope curve, note for note (dB rms) | 13 (nearest 10.4) | fonts among themselves 11.7 | | | | 1.1 |  |
| pitch re 12-TET A440 (median) (c) | 0 | -11.0 | 9.6 | 2.6 | 0 | 0.1 |  |
| pitch scatter from key to key (c) | 0 | 0.7 | 1.9 | 1.9 | 0.1 | 0.4 |  |
| noisiness (spectral flatness) (dB) | -37.8 | -59.6 | -55.0 | -53.0 | -61.9 | 4.4 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -56.2 | -90.4 | -70.7 | -92.1 | -93.8 | 3.0 | yes |
| pitch at the start re settled (c) | -0.1 | -0.3 | -26.1 | 0 | 3.1 | 0.0 |  |
| stereo coherence | 0.843 | 1 | 1 | 1 | 1 | 2.1 | yes |
| partial-to-partial jaggedness (dB) | 6.6 | 14.1 | 12.9 | 9.2 | 15.0 | 2.4 | yes |
| dip between legato notes (dB) | 4.8 | 4.5 | 4.9 | 5.9 | 2.4 | 0.0 |  |
| level evenness of a legato scale (dB) | 1.3 | 0.2 | 0.2 | 0.2 | 1.2 | 1.1 |  |
| level over the keyboard (dB/oct) | -1.09 | -2.74 | -2.29 | -1.82 | -1.28 | 1.0 |  |
| level against the other instruments (velocity 96) (dB) | -0.2 | -3.3 | -5.1 | -2.6 | -5.7 | 2.6 | yes |
| key-to-key level scatter (dB) | 1.43 | 1.16 | 0.55 | 0.39 | 1.41 | 1.1 |  |
| odd / even partials (dB) | -23.1 | -0.6 | 33.2 | 17.1 | 23.0 | 2.6 | yes |
| onset jitter in a phrase (ms) | 1.3 | 1.7 | 1.1 | 3.3 | 0.5 | 0.0 |  |
| onset in a phrase (re the note-on) (ms) | 6 | 10.0 | -2 | 15.0 | 0 | 0.1 |  |
| onset sharpness (dB/ms) | 28.6 | 12.5 | 2 | 29.2 | 5.6 | 1.4 |  |
| overtone levels, note for note (dB rms) | 30 (nearest 24) | fonts among themselves 21.1 | | | | 1.4 |  |
| panning across the keys (dB/oct) | -0.1 | -0 | 0.07 | -0 | -0.03 | 0.1 |  |
| partial envelopes alike | 0.972 | 0.65 | 0.953 | 0.935 | 0.81 | 0.7 |  |
| release time (30 dB after note-off) (ms) | 2000 | 1058 | 2000 | 1060 | 792 | 1.4 |  |
| spectrogram (1 - correlation), note for note | 0.926 (nearest 0.888) | fonts among themselves 0.367 | | | | 2.5 | yes |
| spectral shape of the body, note for note | 21.5 (nearest 20.8) | fonts among themselves 14.9 | | | | 1.4 | yes |
| sounding length of a 100 ms note (ms) | 400 | 338 | 362 | 333 | 400 | 0.8 |  |
| level after 2 s held, re peak (dB) | -32.0 | -40.5 | -49.1 | -44.9 | -35.1 | 1.5 | yes |
| high bands lag low bands by (ms) | 0.3 | 0.2 | 0 | -0.7 | 0 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 0.8 | 0.7 | 0 | 0.4 | 0 | 0.1 |  |
| fundamental share (tristimulus 1) | 0 | 0.235 | 0.657 | 0.973 | 0.99 | 2.2 | yes |
| partials 2-4 share (tristimulus 2) | 0.001 | 0 | 0 | 0 | 0 | 0.0 |  |
| partials 5+ share (tristimulus 3) | 0.001 | 0.764 | 0.343 | 0.027 | 0.01 | 0.5 |  |
| loudness range, velocity 40 to 127 (dB) | 12.2 | 16.6 | 16.6 | 16.7 | 20.1 | 2.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 1.7 | 1.8 | 2.1 | 0.7 | 0.7 | 0.1 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0257 (nearest 0.0222) | fonts among themselves 0.0184 | | | | 1.4 |  |

</details>

### 11 Music Box: music-box

Distance 2.67 (rank 70 of 129); the fonts disagree 1.9 JND. Keys 48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Narrow dynamics (6.9 vs 16.6 dB)**: plausible as DELIBERATE: a pin plucks a tooth with a fixed bend (research/chromatic_percussion.md §6). TiMidity's law sets the fonts' range.
- **Louder (+6.3 vs −5.7…−1.3 dB)**: a trim FAULT, low risk.
- **Key scatter 4.1 vs 0.1…1.3 dB; brightens slower up the keys (1.8 vs 3.2…3.9 ERB/oct)**: a first form, unchecked (§9 "What is owed"). Possible FAULT: the box's fixed rise to 300 Hz and fall above 7 kHz shape the top teeth too much.
- **−30 dB below 150 Hz**: ARTIFACT (the C4–C8 compass folds key 48).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (6 dB | 16.6 | 16.6 | 16.7 | 20.1), outside the fonts, 6.1 x.
2. **key-to-key level scatter**: less even from key to key (3.42 dB | 1.31 | 0.07 | 0.16 | 0.9), outside the fonts, 4.7 x.
3. **brightness over the keyboard**: brightens slower up the keyboard (1.77 ERB/oct | 3.18 | 3.94 | 3.4 | 3.29), outside the fonts, 4.1 x.
4. **brightness (spectral centroid, velocity 96)**: darker (11.0 ERB | 12.0 | 13.3 | 12.6 | 14.6), outside the fonts, 1.5 x.
5. **onset sharpness**: a sharper onset (50.2 dB/ms | 21.0 | 9.1 | 4 | 14.2), outside the fonts, 4.5 x.
6. **attack time (-20 to -1 dB)**: a faster attack (4 ms | 55.0 | 12.0 | 23.0 | 12.0), outside the fonts, 1.9 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -37 | -5 | +22 | +17 | -19 | -20 |
| re TimGM6mb | -29 | +1 | +20 | +14 | -18 | -12 |
| re Jnsgm2 | -36 | -3 | +16 | +14 | -22 | -13 |
| re SGM | -31 | +3 | +21 | +14 | -14 | -15 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -4 -2 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 0.3 dB rms; TimGM6mb: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 4.8 dB rms; Jnsgm2: centroid -0.1 ERB, attack -0.5 ERB, velocity brightening +0.8 ERB, loudness range +7 dB, envelope 2.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0 | 0.23 | 0.5 | 1.18 | -0.09 | 0.6 |  |
| attack time (-20 to -1 dB) (ms) | 4 | 55.0 | 12.0 | 23.0 | 12.0 | 1.9 | yes |
| spectral shape of the attack, note for note | 13.6 (nearest 12.2) | fonts among themselves 12.5 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | 1.77 | 3.18 | 3.94 | 3.4 | 3.29 | 4.1 | yes |
| key-to-key brightness scatter (ERB) | 0.809 | 0.265 | 0.21 | 0.122 | 0.706 | 2.0 | yes |
| brightening from velocity 40 to 127 (ERB) | -0 | 0 | 0.01 | 0 | -0.1 | 0.0 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 11.0 | 12.0 | 13.3 | 12.6 | 14.6 | 1.5 | yes |
| decay rate of a held note (dB/s) | 23.0 | 32.5 | 0.5 | 24.1 | 10.3 | 0.2 |  |
| onset delay (ms) | 10.0 | 14.0 | 3 | 13.0 | 2 | 0.2 |  |
| Plomp-Levelt dissonance of a note (log2) | -7.38 | -9.97 | -4.01 | -5.8 | -5.51 | 0.6 |  |
| envelope curve, note for note (dB rms) | 15.4 (nearest 13.1) | fonts among themselves 18.7 | | | | 0.8 |  |
| pitch re 12-TET A440 (median) (c) | 0 | 1.2 | 1.5 | 0.3 | -0.5 | 0.2 |  |
| pitch scatter from key to key (c) | 0 | 0.5 | 0.3 | 4.6 | 1 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -64.9 | -59.8 | -54.9 | -61.3 | -46.6 | 1.0 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -94.0 | -121 | -37.7 | -121 | -58.9 | 0.1 |  |
| pitch at the start re settled (c) | -0.1 | 0.4 | 1.3 | 2.4 | -1.9 | 0.2 |  |
| stereo coherence | 0.998 | 1 | 1 | 1 | 0.942 | 0.0 |  |
| partial-to-partial jaggedness (dB) | 4.8 | 10.6 | 5.5 | 11.1 | 10.7 | 2.1 | yes |
| dip between legato notes (dB) | 14.3 | 2.5 | 6.2 | 9.6 | 6.4 | 2.3 | yes |
| level evenness of a legato scale (dB) | 0.7 | 1.2 | 1.3 | 0.6 | 1.3 | 0.6 |  |
| level over the keyboard (dB/oct) | 0.86 | 1.02 | -1.5 | -0.93 | -3.33 | 1.0 |  |
| level against the other instruments (velocity 96) (dB) | -0.7 | -2.6 | -1.1 | -5.1 | -4.5 | 1.4 |  |
| key-to-key level scatter (dB) | 3.42 | 1.31 | 0.07 | 0.16 | 0.9 | 4.7 | yes |
| odd / even partials (dB) | -8.7 | -32.0 | -18.0 | -44.2 | -23.0 | 1.4 | yes |
| onset jitter in a phrase (ms) | 40.5 | 6.6 | 19.0 | 67.6 | 1.2 | 0.8 |  |
| onset in a phrase (re the note-on) (ms) | 8 | 0 | 2 | 28.0 | -0 | 0.5 |  |
| onset sharpness (dB/ms) | 50.2 | 21.0 | 9.1 | 4 | 14.2 | 4.5 | yes |
| overtone levels, note for note (dB rms) | 28.4 (nearest 22.3) | fonts among themselves 22.2 | | | | 1.3 |  |
| panning across the keys (dB/oct) | -0.13 | -0 | -0 | -0 | -0.17 | 0.1 |  |
| partial envelopes alike | 0.983 | 0.993 | 0.618 | 0.978 | 0.829 | 0.4 |  |
| release time (30 dB after note-off) (ms) | 1885 | 160 | 1574 | 952 | 1385 | 0.4 | yes |
| spectrogram (1 - correlation), note for note | 0.479 (nearest 0.431) | fonts among themselves 0.405 | | | | 1.2 |  |
| spectral shape of the body, note for note | 13.8 (nearest 12.9) | fonts among themselves 13.5 | | | | 1.0 |  |
| sounding length of a 100 ms note (ms) | 400 | 190 | 308 | 300 | 372 | 0.8 |  |
| level after 2 s held, re peak (dB) | -53.2 | -63.7 | -8.4 | -56.9 | -27.1 | 0.4 |  |
| high bands lag low bands by (ms) | -0.7 | 29.3 | 0.7 | 3.8 | 3 | 0.3 |  |
| transient coherence: attack spread over bands (ms) | 0.4 | 20.8 | 1.2 | 6.6 | 7 | 0.7 |  |
| fundamental share (tristimulus 1) | 1 | 0.956 | 0.7 | 0.695 | 0.847 | 1.7 | yes |
| partials 2-4 share (tristimulus 2) | 0 | 0.043 | 0.299 | 0.305 | 0.106 | 1.5 | yes |
| partials 5+ share (tristimulus 3) | 0 | 0 | 0.001 | 0 | 0.044 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 6 | 16.6 | 16.6 | 16.7 | 20.1 | 6.1 | yes |
| pitch wobble / vibrato (p-p) (c) | 1 | 1.1 | 5.9 | 6.1 | 5.2 | 0.9 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0171 (nearest 0.00947) | fonts among themselves 0.0217 | | | | 0.8 |  |

</details>

### 12 Vibraphone: vibraphone

Distance 2.15 (rank 99 of 129); the fonts disagree 1.8 JND. Keys 48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Key scatter (5.3 vs 0.2…2.6 dB) and tone scatter (1.5 vs 0.2…1.0 ERB)**: possible FAULT. Each bar's resonator coupling and the mallet straying give per-key differences; a real M55 is voiced even. Look at which keys stand out.
- **Rougher (−4.3 vs −6.5…−9 log2), Zimtohrli 2.4× the fonts' spread**: worth a listen. Odd/even (−34 dB) is an ARTIFACT of the measure: 1:4:10 bars have no harmonic series.
- **<150 Hz −38…−53 dB**: ARTIFACT (F3–F6 compass; key 48 folded).
- **Brightens with velocity (+0.6 ERB)**: DELIBERATE (README "Vibraphone": the mallet hardens with velocity).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **odd / even partials**: more even partials (-34.4 dB | -15.9 | -21.1 | -18.6 | -17.0), outside the fonts, 6.4 x.
2. **key-to-key level scatter**: less even from key to key (4.26 dB | 2.56 | 0.22 | 0.36 | 1.04), outside the fonts, 3.0 x.
3. **key-to-key brightness scatter**: less even in tone from key to key (1.5 ERB | 0.19 | 0.195 | 0.217 | 1), outside the fonts, 3.2 x.
4. **partial stretch (inharmonicity)**: more stretched partials (3.5 c/partial | 0.02 | - | -1.38 | -2.2), outside the fonts, 4.4 x.
5. **onset delay**: speaks later (32.0 ms | 2 | 4 | 0 | 1), outside the fonts, 3.0 x.
6. **brightening from velocity 40 to 127**: brightens more with velocity (0.58 ERB | -0.06 | 0 | -0 | -0.06), outside the fonts, 2.0 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -40 | -6 | +10 | +10 | +2 | -20 |
| re TimGM6mb | -53 | -5 | +8 | +7 | +7 | -32 |
| re Jnsgm2 | -55 | -7 | +8 | +1 | +7 | - |
| re SGM | -50 | -5 | +6 | +0 | +6 | -3 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -14 -7 +0 +1; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.2 ERB, attack -2.0 ERB, velocity brightening -0.1 ERB, loudness range +5 dB, envelope 3.9 dB rms; TimGM6mb: centroid +0.0 ERB, attack -0.1 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 2.0 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.1 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 1.2 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.66 | 2.23 | 0.27 | 0.27 | 2.85 | 0.5 |  |
| attack time (-20 to -1 dB) (ms) | 110 | 1 | 14.0 | 31.0 | 29.0 | 1.0 | yes |
| spectral shape of the attack, note for note | 15.1 (nearest 14.3) | fonts among themselves 10.9 | | | | 1.4 | yes |
| brightness over the keyboard (ERB/oct) | 2.56 | 3.65 | 3.51 | 3.13 | 1.87 | 0.9 |  |
| key-to-key brightness scatter (ERB) | 1.5 | 0.19 | 0.195 | 0.217 | 1 | 3.2 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.58 | -0.06 | 0 | -0 | -0.06 | 2.0 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 11.0 | 11.2 | 11.0 | 11.0 | 12.9 | 0.1 |  |
| decay rate of a held note (dB/s) | 11.3 | 21.6 | 13.1 | 16.9 | 26.7 | 1.5 | yes |
| onset delay (ms) | 32.0 | 2 | 4 | 0 | 1 | 3.0 | yes |
| Plomp-Levelt dissonance of a note (log2) | -5.16 | -8.97 | -7.3 | -8.97 | -6.51 | 2.4 | yes |
| envelope curve, note for note (dB rms) | 14 (nearest 10.2) | fonts among themselves 10.8 | | | | 1.3 |  |
| pitch re 12-TET A440 (median) (c) | -0.3 | 0.3 | 2.9 | 14.9 | -0.3 | 0.2 |  |
| pitch scatter from key to key (c) | 0.8 | 2.1 | 1.6 | 0.6 | 0.4 | 0.1 |  |
| noisiness (spectral flatness) (dB) | -66.2 | -62.9 | -65.2 | -65.2 | -52.5 | 0.3 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -91.2 | -119 | -123 | -121 | -104 | 3.1 | yes |
| pitch at the start re settled (c) | -0.2 | 1.6 | 4.4 | -0.7 | -1 | 0.1 |  |
| stereo coherence | 0.294 | 0.782 | 1 | 1 | 0.83 | 5.7 | yes |
| partial-to-partial jaggedness (dB) | 10.6 | 7.1 | 7.5 | 5.45 | 9 | 1.8 | yes |
| dip between legato notes (dB) | 9.2 | 3.6 | 5.6 | 3.6 | 3.5 | 2.8 | yes |
| level evenness of a legato scale (dB) | 3.1 | 0.9 | 1 | 1.5 | 0.7 | 2.2 | yes |
| level over the keyboard (dB/oct) | -0.89 | -2.97 | -0.6 | 1.68 | -1.13 | 0.0 |  |
| level against the other instruments (velocity 96) (dB) | -2.2 | -3.8 | 2.7 | 2.6 | -1.3 | 0.9 |  |
| key-to-key level scatter (dB) | 4.26 | 2.56 | 0.22 | 0.36 | 1.04 | 3.0 | yes |
| odd / even partials (dB) | -34.4 | -15.9 | -21.1 | -18.6 | -17.0 | 6.4 | yes |
| onset jitter in a phrase (ms) | 26.3 | 7.3 | 9.6 | 16.3 | 39.9 | 0.8 |  |
| onset in a phrase (re the note-on) (ms) | 23.0 | 0 | 6 | 8 | -1 | 2.0 | yes |
| onset sharpness (dB/ms) | 41.7 | 31.3 | 2 | 1.9 | 3.3 | 2.7 | yes |
| overtone levels, note for note (dB rms) | 17.2 (nearest 13.5) | fonts among themselves 10.9 | | | | 1.6 |  |
| panning across the keys (dB/oct) | 1.34 | -3.29 | -0 | -0 | 1.22 | 0.6 |  |
| partial envelopes alike | 0.917 | 0.946 | 0.954 | 0.954 | 0.928 | 0.3 |  |
| release time (30 dB after note-off) (ms) | 443 | 260 | 332 | 311 | 531 | 0.9 |  |
| spectrogram (1 - correlation), note for note | 0.329 (nearest 0.256) | fonts among themselves 0.204 | | | | 1.6 |  |
| spectral shape of the body, note for note | 12 (nearest 11.2) | fonts among themselves 8.73 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 400 | 162 | 310 | 346 | 382 | 0.5 |  |
| partial stretch (inharmonicity) (c/partial) | 3.5 | 0.02 | - | -1.38 | -2.2 | 4.4 | yes |
| level after 2 s held, re peak (dB) | -22.1 | -46.8 | -27.2 | -33.2 | -42.9 | 1.6 | yes |
| high bands lag low bands by (ms) | -24.0 | -1.6 | -3.1 | 0 | -1 | 4.5 | yes |
| transient coherence: attack spread over bands (ms) | 11.4 | 1.9 | 0.8 | 0 | 1 | 2.1 | yes |
| fundamental share (tristimulus 1) | 0.993 | 0.998 | 0.995 | 1 | 0.974 | 0.0 |  |
| partials 2-4 share (tristimulus 2) | 0.004 | 0.001 | 0.005 | 0 | 0.026 | 0.0 |  |
| partials 5+ share (tristimulus 3) | 0 | 0 | 0 | 0 | 0 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 15.2 | 17.3 | 16.7 | 16.7 | 20.1 | 1.1 | yes |
| pitch wobble / vibrato (p-p) (c) | 3.3 | 1.5 | 3.3 | 0.4 | 1.9 | 0.3 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0197 (nearest 0.0131) | fonts among themselves 0.00772 | | | | 2.6 | yes |

</details>

### 13 Marimba: marimba

Distance 2.12 (rank 101 of 129); the fonts disagree 0.8 JND. Keys 48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Louder (+8.5 vs −7.2…−0.7 dB)**: likely trim FAULT. Struck programs sit consistently above the fonts here (p008–p015).
- **Rougher (−2.5 vs −7…−9 log2), Zimtohrli ~2× the fonts' spread, key scatter 2.9 vs 0.5…1.4 dB**: possible FAULT. research/chromatic_percussion.md §3 admits uniform-bar shapes and estimated ratios (1:3.92:9.24). The 3.92 partial, 24 c off the 4th harmonic, may be what reads as rough.
- **Odd/even (−21 dB) and "sustains more after 2 s"**: ARTIFACTS. Bars have no harmonic series; −86 dB at 2 s is the room floor.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **onset sharpness**: a sharper onset (59.5 dB/ms | 6.9 | 1.5 | 1.5 | 3.3), outside the fonts, 21.1 x.
2. **Plomp-Levelt dissonance of a note**: rougher (-2.42 log2 | -8.97 | -7.16 | -7.06 | -7.16), outside the fonts, 5.0 x.
3. **loudness range, velocity 40 to 127**: a narrower dynamic range (13.1 dB | 16.7 | 16.6 | 16.6 | 20.0), outside the fonts, 2.1 x.
4. **level after 2 s held, re peak**: sustains more (-86.3 dB | -123 | -110 | -108 | -108), outside the fonts, 2.9 x.
5. **odd / even partials**: more even partials (-21.6 dB | -8.5 | -5.7 | -5.7 | 0.6), outside the fonts, 3.5 x.
6. **key-to-key level scatter**: less even from key to key (2.07 dB | 0.46 | 1.13 | 1.44 | 1.08), outside the fonts, 1.9 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -6 | -5 | +3 | +2 | +1 | -12 |
| re TimGM6mb | -13 | +1 | +8 | +9 | -7 | -16 |
| re Jnsgm2 | -13 | +1 | +8 | +9 | -7 | -16 |
| re SGM | -23 | -2 | +2 | +3 | +9 | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -4 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 0.1 dB rms; TimGM6mb: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 0.6 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.7 ERB, velocity brightening -0.3 ERB, loudness range +6 dB, envelope 1.4 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.1 | 0.12 | 0.92 | 0.89 | 0.02 | 0.9 |  |
| attack time (-20 to -1 dB) (ms) | 4 | 23.0 | 3 | 3 | 6 | 0.1 |  |
| spectral shape of the attack, note for note | 11.3 (nearest 11.1) | fonts among themselves 7.96 | | | | 1.4 | yes |
| brightness over the keyboard (ERB/oct) | 3.6 | 3.53 | 3.53 | 3.53 | 3.19 | 0.2 |  |
| key-to-key brightness scatter (ERB) | 0.202 | 0.194 | 0.194 | 0.192 | 0.526 | 0.0 |  |
| brightening from velocity 40 to 127 (ERB) | 0.05 | 0 | 0 | 0 | -0.31 | 0.2 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 11.1 | 11.0 | 11.0 | 11.0 | 11.0 | 0.2 |  |
| decay rate of a held note (dB/s) | 32.7 | 80.3 | 31.7 | 31.2 | 116 | 0.7 |  |
| onset delay (ms) | 10.0 | 3 | 0 | 0 | 1 | 0.9 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.42 | -8.97 | -7.16 | -7.06 | -7.16 | 5.0 | yes |
| envelope curve, note for note (dB rms) | 15.7 (nearest 13.9) | fonts among themselves 16.2 | | | | 1.0 |  |
| pitch re 12-TET A440 (median) (c) | -0.1 | -1.5 | 2.8 | 2.8 | -4.5 | 0.2 |  |
| pitch scatter from key to key (c) | 5.5 | 0.1 | 0.9 | 0.9 | 5.6 | 1.5 |  |
| noisiness (spectral flatness) (dB) | -53.7 | -71.2 | -60.9 | -61.9 | -62.9 | 1.7 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -125 | -123 | -118 | -118 | -108 | 0.8 |  |
| pitch at the start re settled (c) | 0.1 | 0.1 | -2.6 | -2.7 | 3.1 | 0.3 |  |
| stereo coherence | 0.902 | 0.992 | 1 | 1 | 1 | 1.3 | yes |
| partial-to-partial jaggedness (dB) | 7.45 | 4.2 | 4.6 | 4.6 | 5.9 | 2.9 | yes |
| dip between legato notes (dB) | 5.7 | 6.1 | 3.3 | 3.3 | 8 | 0.4 |  |
| level evenness of a legato scale (dB) | 1.3 | 0.3 | 0.2 | 0.2 | 0.4 | 1.1 | yes |
| level over the keyboard (dB/oct) | 0.53 | 0.35 | 0.95 | 0.99 | 1.85 | 0.4 |  |
| level against the other instruments (velocity 96) (dB) | -1.8 | -0.2 | -7 | -6.1 | -1.7 | 0.6 |  |
| key-to-key level scatter (dB) | 2.07 | 0.46 | 1.13 | 1.44 | 1.08 | 1.9 | yes |
| odd / even partials (dB) | -21.6 | -8.5 | -5.7 | -5.7 | 0.6 | 3.5 | yes |
| onset jitter in a phrase (ms) | 1.2 | 4.3 | 1.2 | 1.2 | 1.1 | 0.0 |  |
| onset in a phrase (re the note-on) (ms) | 8 | -2 | -4 | -4 | -2 | 1.1 | yes |
| onset sharpness (dB/ms) | 59.5 | 6.9 | 1.5 | 1.5 | 3.3 | 21.1 | yes |
| overtone levels, note for note (dB rms) | 13.2 (nearest 10.7) | fonts among themselves 9.64 | | | | 1.4 |  |
| panning across the keys (dB/oct) | 0.37 | 0.23 | 0.05 | 0.05 | -0.03 | 0.3 |  |
| partial envelopes alike | 0.962 | 0.99 | 0.998 | 0.998 | 0.996 | 0.4 |  |
| spectrogram (1 - correlation), note for note | 0.25 (nearest 0.223) | fonts among themselves 0.181 | | | | 1.4 |  |
| spectral shape of the body, note for note | 13.1 (nearest 11.5) | fonts among themselves 6.27 | | | | 2.1 | yes |
| sounding length of a 100 ms note (ms) | 353 | 270 | 228 | 269 | 400 | 1.0 |  |
| level after 2 s held, re peak (dB) | -86.3 | -123 | -110 | -108 | -108 | 2.9 | yes |
| high bands lag low bands by (ms) | -1.3 | -1.1 | -1.5 | -2.7 | 0 | 0.0 |  |
| transient coherence: attack spread over bands (ms) | 0.7 | 0.8 | 1 | 2 | 0 | 0.0 |  |
| fundamental share (tristimulus 1) | 1 | 0.998 | 0.998 | 0.998 | 1 | 0.0 |  |
| partials 2-4 share (tristimulus 2) | 0 | 0.002 | 0.002 | 0.002 | 0 | 0.0 |  |
| partials 5+ share (tristimulus 3) | 0 | 0 | 0 | 0 | 0 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 13.1 | 16.7 | 16.6 | 16.6 | 20.0 | 2.1 | yes |
| pitch wobble / vibrato (p-p) (c) | 30.1 | 41.2 | 2 | 1.6 | 86.9 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0103 (nearest 0.00466) | fonts among themselves 0.00529 | | | | 1.9 |  |

</details>

### 14 Xylophone: xylophone

Distance 4.01 (rank 20 of 129); the fonts disagree 0.5 JND. Keys 48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Bright and flat over the keys (15.5 vs 11.0 ERB; 0.23 vs 3.3…3.5 ERB/oct; −32…−60 dB under 400 Hz)**: mostly a MEASUREMENT ARTIFACT. The compass is F4–C8 sounding (README "Chromatic percussion"), so probe keys 48–64 fold up an octave. The fonts play a near-pure fundamental at the key (fundamental share 1.0). The 1:3:6 bar (chromatic_percussion.md §4) is DELIBERATE.
- **Louder (+1.4 vs −8.4…−11.9 dB re the other instruments)**: FAULT, a trim of ~10 dB.
- **Onset 64 vs 0.3…1.4 dB/ms, noisier (−26 vs −49…−60 dB)**: DELIBERATE hard mallets. The fonts' samples start softly.
- **Pitch scatter 8.3 vs 0.3…2.1 c; tone scatter 1.1 vs 0.2…0.5 ERB**: possible FAULT. README calls the model a first form, "not yet checked". The fold boundary accounts for part of it.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (15.7 ERB | 11.0 | 11.0 | 11.0 | 11.1), outside the fonts, 15.7 x.
2. **loudness range, velocity 40 to 127**: a narrower dynamic range (9.8 dB | 18.0 | 16.7 | 16.6 | 20.1), outside the fonts, 4.3 x.
3. **brightness over the keyboard**: brightens slower up the keyboard (0.2 ERB/oct | 3.35 | 3.54 | 3.54 | 3.25), outside the fonts, 10.8 x.
4. **onset sharpness**: a sharper onset (59.8 dB/ms | 0.9 | 1.1 | 0.3 | 1.4), outside the fonts, 29.4 x.
5. **key-to-key brightness scatter**: less even in tone from key to key (1.1 ERB | 0.189 | 0.193 | 0.192 | 0.542), outside the fonts, 4.5 x.
6. **noisiness (spectral flatness)**: noisier (-26.1 dB | -52.7 | -59.7 | -57.5 | -48.5), outside the fonts, 5.2 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -54 | -38 | -3 | +3 | +22 | +17 |
| re TimGM6mb | -58 | -38 | -0 | +7 | +21 | +8 |
| re Jnsgm2 | -60 | -41 | -3 | +5 | +25 | +13 |
| re SGM | -39 | -32 | +4 | +10 | +14 | +7 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -8 -4 +0 +2; FluidR3 -14 -7 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.9 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 0.7 dB rms; TimGM6mb: centroid +0.0 ERB, attack -0.3 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 1.4 dB rms; Jnsgm2: centroid +0.0 ERB, attack -2.3 ERB, velocity brightening -0.3 ERB, loudness range +7 dB, envelope 4.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.04 | 2.23 | 2.76 | 2.59 | 7.1 | 1.1 | yes |
| attack time (-20 to -1 dB) (ms) | 1 | 1 | 1 | 1 | 1 | 0.0 |  |
| spectral shape of the attack, note for note | 13.3 (nearest 11.1) | fonts among themselves 11.3 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 0.2 | 3.35 | 3.54 | 3.54 | 3.25 | 10.8 | yes |
| key-to-key brightness scatter (ERB) | 1.1 | 0.189 | 0.193 | 0.192 | 0.542 | 4.5 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.09 | 0.02 | 0 | 0 | -0.31 | 0.3 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 15.7 | 11.0 | 11.0 | 11.0 | 11.1 | 15.7 | yes |
| decay rate of a held note (dB/s) | 33.3 | 79.2 | 67.3 | 65.1 | 31.5 | 1.5 |  |
| onset delay (ms) | 9 | 0 | 0 | 0 | 0 | 0.9 | yes |
| Plomp-Levelt dissonance of a note (log2) | -0.19 | -5.72 | -7.64 | -7.51 | -4.24 | 3.8 | yes |
| envelope curve, note for note (dB rms) | 19.1 (nearest 11.4) | fonts among themselves 12.1 | | | | 1.6 |  |
| pitch re 12-TET A440 (median) (c) | -0.3 | -0.2 | 3.4 | 3.4 | 1.2 | 0.6 |  |
| pitch scatter from key to key (c) | 8.3 | 0.3 | 0.4 | 0.9 | 2.1 | 2.6 | yes |
| noisiness (spectral flatness) (dB) | -26.1 | -52.7 | -59.7 | -57.5 | -48.5 | 5.2 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -121 | -121 | -122 | -118 | -106 | 0.2 |  |
| pitch at the start re settled (c) | -2.2 | -0.4 | 1.9 | 2.1 | -1.8 | 0.6 |  |
| stereo coherence | 0.579 | 0.778 | 0.998 | 1 | 1 | 3.8 | yes |
| partial-to-partial jaggedness (dB) | 6.7 | 7.6 | 3.4 | 4.1 | 4.8 | 1.1 |  |
| dip between legato notes (dB) | 6.4 | 5.9 | 5.4 | 5.6 | 4.9 | 0.5 |  |
| level evenness of a legato scale (dB) | 0.7 | 0.5 | 0.4 | 0.4 | 1.3 | 0.2 |  |
| level over the keyboard (dB/oct) | -1.22 | -2.48 | -2.53 | -2.24 | -2.34 | 1.2 | yes |
| level against the other instruments (velocity 96) (dB) | -8.5 | -11.2 | -8.3 | -11.8 | -8.5 | 0.8 |  |
| key-to-key level scatter (dB) | 1.54 | 0.76 | 0.81 | 0.16 | 1.21 | 1.4 | yes |
| odd / even partials (dB) | 12.4 | 4.2 | -6.4 | -3.3 | -0.3 | 2.7 | yes |
| onset jitter in a phrase (ms) | 1.2 | 7.4 | 45.8 | 27.1 | 0.9 | 0.7 |  |
| onset in a phrase (re the note-on) (ms) | 4 | -4 | -4 | -3 | -2 | 0.8 | yes |
| onset sharpness (dB/ms) | 59.8 | 0.9 | 1.1 | 0.3 | 1.4 | 29.4 | yes |
| overtone levels, note for note (dB rms) | 26.5 (nearest 18.3) | fonts among themselves 11.7 | | | | 2.3 | yes |
| panning across the keys (dB/oct) | -0.22 | -0.28 | -0.01 | -0 | -0.03 | 0.2 |  |
| partial envelopes alike | 0.966 | 0.976 | 0.994 | 0.995 | 0.984 | 0.2 |  |
| spectrogram (1 - correlation), note for note | 0.45 (nearest 0.289) | fonts among themselves 0.228 | | | | 2.0 |  |
| spectral shape of the body, note for note | 13 (nearest 11.2) | fonts among themselves 7.3 | | | | 1.8 | yes |
| sounding length of a 100 ms note (ms) | 127 | 121 | 90.0 | 140 | 200 | 0.1 |  |
| level after 2 s held, re peak (dB) | -78.0 | -119 | -122 | -118 | -69.6 | 1.5 |  |
| high bands lag low bands by (ms) | -1.3 | 0 | 0 | 0 | -3.3 | 0.3 |  |
| transient coherence: attack spread over bands (ms) | 1.2 | 0 | 0 | 0 | 2.8 | 0.2 |  |
| fundamental share (tristimulus 1) | 0.767 | 1 | 1 | 1 | 0.999 | 3.3 | yes |
| partials 2-4 share (tristimulus 2) | 0.155 | 0 | 0 | 0 | 0 | 2.2 | yes |
| partials 5+ share (tristimulus 3) | 0.007 | 0 | 0 | 0 | 0.001 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 9.8 | 18.0 | 16.7 | 16.6 | 20.1 | 4.3 | yes |
| pitch wobble / vibrato (p-p) (c) | 37.9 | 75.0 | 78.5 | 76.6 | 1.5 | 1.0 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0102 (nearest 0.00925) | fonts among themselves 0.00349 | | | | 2.9 | yes |

</details>

### 15 Tubular Bells: tubular-bells

Distance 3.39 (rank 38 of 129); the fonts disagree 2.0 JND. Keys 48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Darker (15.3 vs 19.4…20.7 ERB; −10 dB at 2.5–6 kHz, −28…−36 dB above 6 kHz)**: partly ARTIFACT: the compass is C4–F5, so 8 of the 13 probe keys fold down. Mostly a FAULT, though: only nine modes (up to 8.9× the note) and radiation that grows with frequency (research/chromatic_percussion.md §5) cut the metallic shimmer real chimes have. All four fonts agree.
- **Weaker low partials (tristimulus 0.17 vs 1.0)**: ARTIFACT. Bells have no harmonic series.
- **Slow attack (57 vs 2…12 ms)**: a rawhide strike should be instant. Room and modal bloom; check dry.
- **Brightens with velocity (+2.35 ERB), louder (+9.9 dB; fonts −11…−0.3)**: the hammer hardness is plausible. The level is a trim, though the fonts disagree widely.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: darker (16.2 ERB | 20.7 | 19.9 | 19.8 | 19.4), outside the fonts, 5.7 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (2.19 ERB | 0 | 0.01 | 0 | 0.52), outside the fonts, 7.3 x.
3. **partials 2-4 share (tristimulus 2)**: weaker low partials (0.171 | 0.995 | 0.999 | 0.995 | 0.999), outside the fonts, 11.8 x.
4. **loudness range, velocity 40 to 127**: a narrower dynamic range (12.1 dB | 16.6 | 16.6 | 16.6 | 20.1), outside the fonts, 2.6 x.
5. **attack time (-20 to -1 dB)**: a slower attack (57.0 ms | 12.0 | 4 | 2 | 4), outside the fonts, 3.0 x.
6. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-27.1 dB | -59.8 | -53.6 | -59.8 | -57.6), outside the fonts, 5.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +11 | +11 | +9 | +9 | -10 | -36 |
| re TimGM6mb | +16 | +10 | +11 | +9 | -7 | -27 |
| re Jnsgm2 | +18 | +13 | +7 | +3 | -10 | -27 |
| re SGM | +19 | +12 | +9 | +4 | -9 | -28 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -9 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 1.4 dB rms; TimGM6mb: centroid -0.1 ERB, attack -0.0 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 1.2 dB rms; Jnsgm2: centroid -0.0 ERB, attack +0.0 ERB, velocity brightening +1.4 ERB, loudness range +6 dB, envelope 0.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.78 | 1.64 | 2.01 | 2.23 | 0.94 | 1.6 |  |
| attack time (-20 to -1 dB) (ms) | 57.0 | 12.0 | 4 | 2 | 4 | 3.0 | yes |
| spectral shape of the attack, note for note | 15.8 (nearest 14.9) | fonts among themselves 11.2 | | | | 1.4 | yes |
| brightness over the keyboard (ERB/oct) | 0.02 | 5 | 4.43 | 3.32 | 2.19 | 2.7 | yes |
| key-to-key brightness scatter (ERB) | 1.02 | 0.33 | 0.24 | 0.485 | 1.45 | 1.0 |  |
| brightening from velocity 40 to 127 (ERB) | 2.19 | 0 | 0.01 | 0 | 0.52 | 7.3 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 16.2 | 20.7 | 19.9 | 19.8 | 19.4 | 5.7 | yes |
| decay rate of a held note (dB/s) | 7 | 18.4 | 13.0 | 15.7 | 4.7 | 1.0 |  |
| onset delay (ms) | 11.0 | 1 | 1 | 5 | 2 | 0.9 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.01 | -3.54 | -4.72 | -4.72 | -4.54 | 1.1 |  |
| envelope curve, note for note (dB rms) | 19 (nearest 9.38) | fonts among themselves 10.2 | | | | 1.9 |  |
| pitch re 12-TET A440 (median) (c) | -3.5 | 7.7 | -1.7 | 17.5 | -1.1 | 0.7 |  |
| pitch scatter from key to key (c) | 5.5 | 0.1 | 2.4 | 13.3 | 41.1 | 0.1 |  |
| noisiness (spectral flatness) (dB) | -49.5 | -46.5 | -54.7 | -49.3 | -46.3 | 0.4 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -27.1 | -59.8 | -53.6 | -59.8 | -57.6 | 5.3 | yes |
| pitch at the start re settled (c) | 1.5 | 0.1 | 0.5 | -0.4 | 1.2 | 0.2 |  |
| stereo coherence | 0.323 | 1 | 1 | 1 | 0.184 | 1.7 |  |
| partial-to-partial jaggedness (dB) | 11.1 | 8.35 | 5.6 | 9.4 | 10.7 | 0.9 |  |
| dip between legato notes (dB) | 8.5 | 9.9 | 8.6 | 6.1 | 6.3 | 0.5 |  |
| level evenness of a legato scale (dB) | 1.4 | 0.8 | 0.3 | 0.2 | 2 | 0.8 |  |
| level over the keyboard (dB/oct) | -0.98 | -1.11 | -1.74 | -1.19 | 0.56 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | -0.8 | -0.1 | -0.1 | -1.9 | -9.8 | 0.0 |  |
| key-to-key level scatter (dB) | 1.42 | 0.08 | 0.06 | 0.25 | 2.14 | 1.2 |  |
| odd / even partials (dB) | -6.3 | -1 | 0 | -21.1 | -8.2 | 0.2 |  |
| onset jitter in a phrase (ms) | 54.1 | 63.6 | 1.5 | 0.5 | 0.8 | 1.7 |  |
| onset in a phrase (re the note-on) (ms) | 10.0 | 0 | -0 | 3 | 1 | 0.9 | yes |
| onset sharpness (dB/ms) | 12.9 | 3.8 | 2.8 | 25.2 | 8.9 | 0.6 |  |
| overtone levels, note for note (dB rms) | 33.8 (nearest 32.5) | fonts among themselves 15.2 | | | | 2.2 | yes |
| panning across the keys (dB/oct) | 0.13 | -0.1 | 0 | -0 | 0.02 | 0.1 |  |
| partial envelopes alike | 0.903 | 0.899 | 0.787 | 0.839 | 0.531 | 0.5 |  |
| release time (30 dB after note-off) (ms) | 2000 | 2000 | 1956 | 1768 | 1081 | 0.2 |  |
| spectrogram (1 - correlation), note for note | 0.491 (nearest 0.356) | fonts among themselves 0.261 | | | | 1.9 | yes |
| spectral shape of the body, note for note | 20.2 (nearest 19.1) | fonts among themselves 12.1 | | | | 1.7 | yes |
| sounding length of a 100 ms note (ms) | 400 | 128 | 254 | 305 | 346 | 0.7 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.71 | 6.65 | 3.6 | 4.55 | 2.33 | 1.6 | yes |
| level after 2 s held, re peak (dB) | -16.7 | -43.6 | -35.0 | -34.2 | -17.0 | 1.3 |  |
| high bands lag low bands by (ms) | 1.3 | 0 | 0 | 0 | 0 | 0.3 |  |
| transient coherence: attack spread over bands (ms) | 4.9 | 0.4 | 0.4 | 0 | 0.5 | 0.9 | yes |
| fundamental share (tristimulus 1) | 0 | 0 | 0 | 0 | 0 | 0.0 |  |
| partials 2-4 share (tristimulus 2) | 0.171 | 0.995 | 0.999 | 0.995 | 0.999 | 11.8 | yes |
| partials 5+ share (tristimulus 3) | 0.019 | 0.005 | 0.001 | 0.005 | 0.001 | 0.2 |  |
| loudness range, velocity 40 to 127 (dB) | 12.1 | 16.6 | 16.6 | 16.6 | 20.1 | 2.6 | yes |
| pitch wobble / vibrato (p-p) (c) | 3.2 | 59.0 | 50.6 | 36.7 | 26.5 | 2.5 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0225 (nearest 0.02) | fonts among themselves 0.0123 | | | | 1.8 | yes |

</details>

### 16 Dulcimer: dulcimer

Distance 2.22 (rank 93 of 129); the fonts disagree 1.2 JND. Keys 48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Key scatter (5.0 vs 0.5…1.6 dB), tone scatter 1.8 ERB, uneven legato (3.0 dB)**: KNOWN FAULT. research/chromatic_percussion.md §7: "the soundboard is still the grand's… the largest known fault"; decays are uneven note to note.
- **Decays slower (11.6 vs 17…23 dB/s), −31.5 vs −42…−51 dB after 2 s**: DELIBERATE, partly. There are no dampers, and the ring was set by ear at the user's request (§7). The grand's board likely adds sustain.
- **Louder toward the top (+0.8 vs −2.3…−0.5 dB/oct), louder overall (−3.1 vs −9.3…−5.1 dB)**: levels set only "roughly even" (§9). A FAULT, minor.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (2.62 dB | 0.7 | 0.51 | 0.76 | 1.55), outside the fonts, 3.6 x.
2. **decay rate of a held note**: decays slower (11.6 dB/s | 21.5 | 23.2 | 17.3 | 18.4), outside the fonts, 3.1 x.
3. **level after 2 s held, re peak**: sustains more (-31.8 dB | -49.2 | -50.6 | -41.8 | -43.6), outside the fonts, 3.3 x.
4. **key-to-key brightness scatter**: less even in tone from key to key (1.79 ERB | 0.917 | 0.41 | 0.828 | 0.973), outside the fonts, 3.3 x.
5. **loudness range, velocity 40 to 127**: a narrower dynamic range (13.3 dB | 16.6 | 16.7 | 16.7 | 20.2), outside the fonts, 1.9 x.
6. **noisiness (spectral flatness)**: purer (-59.7 dB | -32.6 | -46.8 | -39.1 | -31.9), outside the fonts, 3.2 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +1 | +14 | -0 | -5 | -2 | -2 |
| re TimGM6mb | -10 | +6 | +5 | -1 | -2 | -1 |
| re Jnsgm2 | +1 | +5 | +2 | -3 | -2 | +0 |
| re SGM | +2 | +23 | +0 | -5 | -6 | -5 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -10 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.9 ERB, attack -0.8 ERB, velocity brightening +0.2 ERB, loudness range +4 dB, envelope 0.9 dB rms; TimGM6mb: centroid -0.6 ERB, attack -0.8 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 1.0 dB rms; Jnsgm2: centroid -0.1 ERB, attack -1.2 ERB, velocity brightening +1.8 ERB, loudness range +5 dB, envelope 0.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.17 | 2.39 | 0.74 | 1.42 | 3.46 | 1.5 | yes |
| attack time (-20 to -1 dB) (ms) | 5 | 4 | 14.0 | 6 | 4 | 0.0 |  |
| spectral shape of the attack, note for note | 13.2 (nearest 11.4) | fonts among themselves 10.7 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 2.63 | 1.78 | 2.67 | 2.26 | 2.39 | 0.7 |  |
| key-to-key brightness scatter (ERB) | 1.79 | 0.917 | 0.41 | 0.828 | 0.973 | 3.3 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.08 | 0 | 0 | -0 | 0.55 | 0.3 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 11.7 | 16.1 | 12.9 | 13.5 | 18.1 | 1.2 | yes |
| decay rate of a held note (dB/s) | 11.6 | 21.5 | 23.2 | 17.3 | 18.4 | 3.1 | yes |
| onset delay (ms) | 13.0 | 1 | 2 | 1 | 1 | 1.2 | yes |
| Plomp-Levelt dissonance of a note (log2) | -5.32 | -3.24 | -5.11 | -3.84 | -2.3 | 1.3 | yes |
| envelope curve, note for note (dB rms) | 16.9 (nearest 14.4) | fonts among themselves 6.22 | | | | 2.7 | yes |
| pitch re 12-TET A440 (median) (c) | 1 | 0 | 4.2 | -0.2 | 0.2 | 0.2 |  |
| pitch scatter from key to key (c) | 1.6 | 3.4 | 0.5 | 2.8 | 1.9 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -59.7 | -32.6 | -46.8 | -39.1 | -31.9 | 3.2 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -40.5 | -120 | -121 | -118 | -78.0 | 3.6 | yes |
| pitch at the start re settled (c) | -0.9 | -3.1 | 3.1 | -0.4 | -0.9 | 0.1 |  |
| stereo coherence | 0.879 | 0.969 | 1 | 1 | 1 | 1.6 | yes |
| partial-to-partial jaggedness (dB) | 6.15 | 3.7 | 5.1 | 4.9 | 2.7 | 1.5 | yes |
| dip between legato notes (dB) | 7.8 | 4.3 | 7.5 | 4 | 6.4 | 1.2 |  |
| level evenness of a legato scale (dB) | 1.5 | 0.5 | 1.8 | 0.9 | 1.6 | 0.2 |  |
| level over the keyboard (dB/oct) | -1.08 | -2.09 | -1.32 | -0.51 | -2.27 | 0.6 |  |
| level against the other instruments (velocity 96) (dB) | -5.2 | -7.8 | -4.9 | -6.9 | -8.1 | 1.3 |  |
| key-to-key level scatter (dB) | 2.62 | 0.7 | 0.51 | 0.76 | 1.55 | 3.6 | yes |
| odd / even partials (dB) | -5 | -5.6 | -7.9 | -1.3 | -4.1 | 0.0 |  |
| onset jitter in a phrase (ms) | 28.3 | 1.3 | 1.4 | 36.3 | 1.3 | 1.5 |  |
| onset in a phrase (re the note-on) (ms) | 12.0 | -2 | -3 | -1 | -1 | 1.4 | yes |
| onset sharpness (dB/ms) | 20.7 | 3.7 | 3.3 | 2.8 | 12.6 | 3.5 | yes |
| overtone levels, note for note (dB rms) | 14.6 (nearest 13.4) | fonts among themselves 11.4 | | | | 1.3 |  |
| panning across the keys (dB/oct) | 0.34 | 0.1 | -0 | -0 | -0.03 | 0.3 |  |
| partial envelopes alike | 0.571 | 0.796 | 0.974 | 0.827 | 0.798 | 2.4 | yes |
| release time (30 dB after note-off) (ms) | 2000 | 468 | 290 | 568 | 1362 | 1.8 | yes |
| spectrogram (1 - correlation), note for note | 0.373 (nearest 0.257) | fonts among themselves 0.201 | | | | 1.9 |  |
| spectral shape of the body, note for note | 11.6 (nearest 11.3) | fonts among themselves 11 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 400 | 111 | 240 | 225 | 256 | 1.3 | yes |
| partial stretch (inharmonicity) (c/partial) | 3.59 | 1.81 | 0.01 | 2.37 | 2.14 | 1.4 | yes |
| level after 2 s held, re peak (dB) | -31.8 | -49.2 | -50.6 | -41.8 | -43.6 | 3.3 | yes |
| high bands lag low bands by (ms) | 0.5 | 0 | -0.7 | 0 | 0.5 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 0.7 | 0 | 0.4 | 0 | 0.9 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.88 | 0.497 | 0.851 | 0.621 | 0.282 | 1.1 |  |
| partials 2-4 share (tristimulus 2) | 0.11 | 0.486 | 0.136 | 0.271 | 0.654 | 1.0 |  |
| partials 5+ share (tristimulus 3) | 0.004 | 0.019 | 0.032 | 0.074 | 0.045 | 0.5 |  |
| loudness range, velocity 40 to 127 (dB) | 13.3 | 16.6 | 16.7 | 16.7 | 20.2 | 1.9 | yes |
| pitch wobble / vibrato (p-p) (c) | 4.1 | 10.2 | 4.5 | 3.2 | 11.4 | 0.7 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0197 (nearest 0.0172) | fonts among themselves 0.0167 | | | | 1.2 |  |

</details>

### 17 Drawbar Organ: drawbar-organ

Distance 2.28 (rank 91 of 129); the fonts disagree 1.1 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **No velocity response (0.7 vs 16.6…22 dB)**: DELIBERATE: a tonewheel organ has none (README "tonewheel organ": "Velocity has no other effect" than the click).
- **Floor −57 vs −112…−125 dB**: DELIBERATE: hum, leakage and the generator's whisper (README: "the idle floor is ~−53 dB under a note").
- **Release 390 vs 60…80 ms**: room plus the rotary speaker at 4 m: ARTIFACT.
- **Wobble 15.9 vs 0.1…6.9 c**: DELIBERATE (rotary speaker Doppler).
- **Louder toward the bottom (−3.0 vs −0.5…+0.1 dB/oct)**: possible FAULT; also in p018 and p019. Check the generator taper or the rotary speaker drum's share. The level against a band agreed with TiMidity/FluidR3 to a median −0.3 dB (research/tonewheel_organ.md "Level against TiMidity").

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (0.7 dB | 22.2 | 16.6 | 16.6 | 20.1), outside the fonts, 6.3 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (390 ms | 74.0 | 60.0 | 80.0 | 70.0), outside the fonts, 9.8 x.
3. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-56.6 dB | -120 | -121 | -125 | -112), outside the fonts, 9.9 x.
4. **level over the keyboard**: louder toward the bottom (-2.98 dB/oct | -0.26 | -0.14 | -0.53 | 0.14), outside the fonts, 2.8 x.
5. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 16.4, nearest 16; the fonts among themselves 5.06), outside.
6. **pitch wobble / vibrato (p-p)**: wobbles more (15.9 c | 2.8 | 0.1 | 0.8 | 6.9), outside the fonts, 2.8 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -4 | -7 | +2 | -2 | +11 | -8 |
| re TimGM6mb | -1 | +3 | +5 | +8 | +7 | -21 |
| re Jnsgm2 | +4 | -1 | +6 | -3 | +3 | -7 |
| re SGM | +10 | -7 | -1 | -9 | +9 | +5 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -0 -0 +0 +1; FluidR3 -14 -7 +0 +9; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.6 ERB, attack +0.0 ERB, velocity brightening +0.5 ERB, loudness range +2 dB, envelope 8.2 dB rms; TimGM6mb: centroid -0.0 ERB, attack +0.0 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 1.2 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 3.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.09 | 0.52 | -0.01 | -0.04 | 0.55 | 0.4 |  |
| attack time (-20 to -1 dB) (ms) | 148 | 24.0 | 2.5 | 12.0 | 329 | 0.9 |  |
| spectral shape of the attack, note for note | 13.9 (nearest 11.6) | fonts among themselves 16.2 | | | | 0.9 |  |
| brightness over the keyboard (ERB/oct) | 3.77 | 1.87 | 3.26 | 4.45 | 2.14 | 0.8 |  |
| key-to-key brightness scatter (ERB) | 0.995 | 0.983 | 0.215 | 1.1 | 2.29 | 0.0 |  |
| brightening from velocity 40 to 127 (ERB) | 0.34 | -0.66 | 0 | 0 | -0.01 | 1.0 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 7.13 | 6.12 | 9.31 | 12.0 | 11.2 | 1.1 |  |
| onset delay (ms) | 26.0 | 9 | 0 | 5 | 16.0 | 1.9 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.93 | -4.84 | -3.8 | -5.8 | -3.91 | 1.4 | yes |
| envelope curve, note for note (dB rms) | 16.4 (nearest 16) | fonts among themselves 5.06 | | | | 3.2 | yes |
| pitch re 12-TET A440 (median) (c) | -0.1 | 0.5 | 2.4 | 6.3 | -1.6 | 0.4 |  |
| pitch scatter from key to key (c) | 1.5 | 1.6 | 0.5 | 0.4 | 1.5 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -61.5 | -76.1 | -64.8 | -54.8 | -51.4 | 0.1 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -56.6 | -120 | -121 | -125 | -112 | 9.9 | yes |
| pitch at the start re settled (c) | -0.7 | 0.2 | 0 | -0.3 | 1.1 | 0.2 |  |
| stereo coherence | 0.748 | 0.388 | 1 | 1 | 0.366 | 0.2 |  |
| partial-to-partial jaggedness (dB) | 5.45 | 7 | 10.7 | 3.8 | 6.7 | 0.4 |  |
| dip between legato notes (dB) | 13.2 | 13.1 | 9 | 9.3 | 7.6 | 1.5 |  |
| level evenness of a legato scale (dB) | 1.8 | 1.5 | 2 | 1.5 | 2.1 | 0.1 |  |
| level over the keyboard (dB/oct) | -2.98 | -0.26 | -0.14 | -0.53 | 0.14 | 2.8 | yes |
| level against the other instruments (velocity 96) (dB) | 0.4 | -2.1 | 1.2 | 5.4 | 2.2 | 0.3 |  |
| key-to-key level scatter (dB) | 1.66 | 1.28 | 0.4 | 0.47 | 3.15 | 0.6 |  |
| odd / even partials (dB) | -17.2 | -38.8 | -36.0 | 0.4 | 0.5 | 0.0 |  |
| onset jitter in a phrase (ms) | 64.1 | 28.0 | 31.7 | 18.2 | 29.7 | 5.2 | yes |
| onset in a phrase (re the note-on) (ms) | 18.0 | -0 | -2 | 0 | 0 | 1.8 | yes |
| onset sharpness (dB/ms) | 16.0 | 10.3 | 3 | 10.9 | 11.4 | 1.3 | yes |
| overtone levels, note for note (dB rms) | 19.4 (nearest 15.2) | fonts among themselves 21.4 | | | | 0.9 |  |
| panning across the keys (dB/oct) | 0.38 | 0.04 | 0 | 0 | -0.24 | 0.4 |  |
| partial envelopes alike | 0.101 | -0.048 | 0.988 | 0.135 | 0.001 | 0.1 |  |
| release time (30 dB after note-off) (ms) | 390 | 74.0 | 60.0 | 80.0 | 70.0 | 9.8 | yes |
| spectrogram (1 - correlation), note for note | 0.356 (nearest 0.238) | fonts among themselves 0.388 | | | | 0.9 |  |
| spectral shape of the body, note for note | 15.5 (nearest 14) | fonts among themselves 17 | | | | 0.9 |  |
| sounding length of a 100 ms note (ms) | 141 | 106 | 111 | 145 | 129 | 0.9 |  |
| partial stretch (inharmonicity) (c/partial) | -0.18 | 0.2 | 0 | -0.23 | 0.56 | 0.6 |  |
| level after 2 s held, re peak (dB) | -7.4 | -11.4 | -7 | -4.6 | -7.5 | 0.0 |  |
| high bands lag low bands by (ms) | 0.2 | -0.3 | 0 | 0 | -0.7 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 11.1 | 0.2 | 0 | 0 | 4.2 | 2.2 | yes |
| fundamental share (tristimulus 1) | 0.786 | 0.773 | 0.58 | 0.364 | 0.253 | 1.2 |  |
| partials 2-4 share (tristimulus 2) | 0.214 | 0.227 | 0.347 | 0.635 | 0.184 | 0.3 |  |
| partials 5+ share (tristimulus 3) | 0 | 0 | 0.072 | 0.001 | 0.058 | 0.4 |  |
| loudness range, velocity 40 to 127 (dB) | 0.7 | 22.2 | 16.6 | 16.6 | 20.1 | 6.3 | yes |
| pitch wobble / vibrato (p-p) (c) | 15.9 | 2.8 | 0.1 | 0.8 | 6.9 | 2.8 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.023 (nearest 0.0216) | fonts among themselves 0.0208 | | | | 1.1 |  |

</details>

### 18 Percussive Organ: percussive-organ

Distance 2.21 (rank 94 of 129); the fonts disagree 1.9 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **No velocity response, floor −53 dB, release 306 ms**: DELIBERATE or room, as in p016.
- **Speaks later in a phrase (30 vs −4…5 ms)**: DELIBERATE. The percussion is single-trigger (README "tonewheel organ"), so legato notes lack the ping and their steepest rise comes later.
- **Brightens slightly with velocity (+0.37 ERB)**: DELIBERATE (touch-sensitive percussion).
- Otherwise close (rank 49).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (0.8 dB | 16.6 | 16.6 | 16.7 | 20.1), outside the fonts, 9.1 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (306 ms | 42.0 | 54.0 | 86.0 | 56.0), outside the fonts, 4.8 x.
3. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-53.0 dB | -125 | -122 | -122 | -107), outside the fonts, 7.7 x.
4. **onset in a phrase (re the note-on)**: speaks later in a phrase (30.0 ms | 5 | -4 | -2 | -1), outside the fonts, 3.1 x.
5. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 15.5, nearest 14.9; the fonts among themselves 5.8), outside.
6. **onset sharpness**: a sharper onset (15.9 dB/ms | 7.7 | 2.2 | 5.1 | 8.4), outside the fonts, 3.1 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +4 | -4 | +3 | -4 | +4 | -3 |
| re TimGM6mb | -3 | -5 | -4 | -1 | +11 | -2 |
| re Jnsgm2 | -7 | -8 | -5 | -8 | +20 | +3 |
| re SGM | +1 | -11 | -8 | -8 | +14 | +17 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -0 -0 +0 +1; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 0.2 dB rms; TimGM6mb: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 0.4 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.4 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 0.4 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 1.12 | 0.46 | 1.85 | 5.77 | 1.55 | 0.2 |  |
| attack time (-20 to -1 dB) (ms) | 100 | 63.0 | 1 | 32.0 | 147 | 0.3 |  |
| spectral shape of the attack, note for note | 11.5 (nearest 8.92) | fonts among themselves 12 | | | | 1.0 |  |
| brightness over the keyboard (ERB/oct) | 4.06 | 3.8 | 2.78 | 2.66 | 2.53 | 2.1 | yes |
| key-to-key brightness scatter (ERB) | 1.34 | 2.02 | 0.778 | 0.815 | 1.31 | 0.5 |  |
| brightening from velocity 40 to 127 (ERB) | 0.37 | 0 | -0 | -0 | -0.19 | 1.2 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 7.82 | 10.4 | 9.06 | 6.12 | 8.6 | 0.5 |  |
| onset delay (ms) | 19.0 | 25.0 | 1 | 6 | 18.0 | 0.6 |  |
| Plomp-Levelt dissonance of a note (log2) | -3.59 | -4.25 | -5.54 | -3.33 | -3.39 | 0.2 |  |
| envelope curve, note for note (dB rms) | 15.5 (nearest 14.9) | fonts among themselves 5.8 | | | | 2.7 | yes |
| pitch re 12-TET A440 (median) (c) | 0.2 | 1.4 | 2.5 | 1.4 | -0.1 | 0.3 |  |
| pitch scatter from key to key (c) | 1.6 | 0.9 | 0.7 | 2.4 | 0.7 | 0.3 |  |
| noisiness (spectral flatness) (dB) | -55.7 | -57.8 | -77.5 | -51.6 | -56.0 | 0.1 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -53.0 | -125 | -122 | -122 | -107 | 7.7 | yes |
| pitch at the start re settled (c) | -2.4 | -0.7 | -0.3 | 2.8 | -0.9 | 0.4 |  |
| stereo coherence | 0.825 | 1 | 1 | 1 | 0.688 | 1.1 |  |
| partial-to-partial jaggedness (dB) | 6.6 | 7.65 | 8.1 | 5.8 | 4.2 | 0.1 |  |
| dip between legato notes (dB) | 15.0 | 10.1 | 4.7 | 13.7 | 7.1 | 1.4 | yes |
| level evenness of a legato scale (dB) | 2.3 | 0.6 | 0.9 | 1 | 1.2 | 1.3 | yes |
| level over the keyboard (dB/oct) | -2.2 | 0.18 | -0.04 | -2.18 | -0.36 | 1.7 |  |
| level against the other instruments (velocity 96) (dB) | -1.7 | 2.2 | -1.6 | 0.6 | -0.7 | 0.9 |  |
| key-to-key level scatter (dB) | 1.37 | 0.79 | 0.35 | 1.13 | 1.01 | 0.9 |  |
| odd / even partials (dB) | -8.7 | 0.7 | -38.9 | -2.7 | -11.6 | 0.1 |  |
| onset jitter in a phrase (ms) | 48.6 | 16.0 | 2.3 | 66.6 | 8.6 | 1.1 |  |
| onset in a phrase (re the note-on) (ms) | 30.0 | 5 | -4 | -2 | -1 | 3.1 | yes |
| onset sharpness (dB/ms) | 15.9 | 7.7 | 2.2 | 5.1 | 8.4 | 3.1 | yes |
| overtone levels, note for note (dB rms) | 16.5 (nearest 15.9) | fonts among themselves 17.9 | | | | 0.9 |  |
| panning across the keys (dB/oct) | 0.31 | 0 | 0 | 0 | -0.28 | 0.3 |  |
| partial envelopes alike | 0.179 | 0.087 | 0.617 | 0.081 | 0.101 | 0.3 |  |
| release time (30 dB after note-off) (ms) | 306 | 42.0 | 54.0 | 86.0 | 56.0 | 4.8 | yes |
| spectrogram (1 - correlation), note for note | 0.232 (nearest 0.17) | fonts among themselves 0.217 | | | | 1.1 |  |
| spectral shape of the body, note for note | 11.3 (nearest 11.2) | fonts among themselves 13.2 | | | | 0.9 |  |
| sounding length of a 100 ms note (ms) | 130 | 121 | 112 | 121 | 112 | 0.6 |  |
| partial stretch (inharmonicity) (c/partial) | 0.02 | 0.27 | 0 | -1.8 | 0.75 | 0.1 |  |
| level after 2 s held, re peak (dB) | -7.2 | -9.1 | -6.5 | -9.9 | -8.3 | 0.5 |  |
| high bands lag low bands by (ms) | -1.3 | 8.4 | 0 | -7.2 | -13.0 | 0.2 |  |
| transient coherence: attack spread over bands (ms) | 2.9 | 14.6 | 0 | 5.6 | 8.2 | 0.5 |  |
| fundamental share (tristimulus 1) | 0.645 | 0.182 | 0.216 | 0.998 | 0.561 | 0.6 |  |
| partials 2-4 share (tristimulus 2) | 0.355 | 0.818 | 0.784 | 0.003 | 0.36 | 0.5 |  |
| partials 5+ share (tristimulus 3) | 0 | 0 | 0 | 0 | 0 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 0.8 | 16.6 | 16.6 | 16.7 | 20.1 | 9.1 | yes |
| pitch wobble / vibrato (p-p) (c) | 6.8 | 8.8 | 0 | 23.0 | 15.7 | 0.5 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0197 (nearest 0.0171) | fonts among themselves 0.0166 | | | | 1.2 |  |

</details>

### 19 Rock Organ: rock-organ

Distance 2.87 (rank 57 of 129); the fonts disagree 1.1 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **No velocity response, floor −53 dB, release 462 ms**: DELIBERATE or room, as in p016.
- **Speaks late (onset 44 vs 2…15 ms; phrase 38 vs ~0 ms)**: the rotary speaker heard at 4 m accounts for ~12 ms. The remaining ~25–30 ms is unexplained; possible FAULT in the driven 122 path (the amp's envelope?). Worth a look, since it is audible against a band.
- **Louder toward the bottom (−3.4 vs −1.3…0 dB/oct)**: as in p016.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (0.7 dB | 16.6 | 16.6 | 16.6 | 20.1), outside the fonts, 9.1 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (462 ms | 39.0 | 40.0 | 73.0 | 37.0), outside the fonts, 7.2 x.
3. **onset delay**: speaks later (44.0 ms | 9 | 2 | 15.0 | 5), outside the fonts, 3.7 x.
4. **onset in a phrase (re the note-on)**: speaks later in a phrase (38.0 ms | 0 | -1 | -1 | 0), outside the fonts, 3.9 x.
5. **level over the keyboard**: louder toward the bottom (-3.42 dB/oct | -0.59 | 0.03 | -0.88 | -1.31), outside the fonts, 2.7 x.
6. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-53.0 dB | -125 | -121 | -123 | -102), outside the fonts, 6.0 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +3 | +0 | +4 | -2 | +5 | -18 |
| re TimGM6mb | -1 | -4 | -2 | -5 | +13 | -4 |
| re Jnsgm2 | +0 | -1 | +5 | -3 | +3 | -12 |
| re SGM | +4 | -1 | -0 | -7 | +8 | -0 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli +0 -0 +0 +1; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 0.3 dB rms; TimGM6mb: centroid -0.2 ERB, attack +0.0 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 1.4 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 0.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.15 | -0.4 | 0.33 | 0.65 | -0.02 | 0.6 |  |
| attack time (-20 to -1 dB) (ms) | 150 | 160 | 250 | 32.0 | 11.0 | 0.5 |  |
| spectral shape of the attack, note for note | 11.8 (nearest 10.1) | fonts among themselves 9.61 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 4.16 | 4 | 3.62 | 3.62 | 3.42 | 1.8 | yes |
| key-to-key brightness scatter (ERB) | 0.819 | 1.43 | 0.347 | 1.68 | 1.65 | 1.1 |  |
| brightening from velocity 40 to 127 (ERB) | 0.28 | 0 | -0 | 0 | -0.05 | 0.9 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 8.9 | 12.9 | 10.5 | 8.93 | 14.9 | 0.9 |  |
| onset delay (ms) | 44.0 | 9 | 2 | 15.0 | 5 | 3.7 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.4 | -3.31 | -3.86 | -1.49 | 0.27 | 0.0 |  |
| envelope curve, note for note (dB rms) | 18.1 (nearest 17.3) | fonts among themselves 5.47 | | | | 3.3 | yes |
| pitch re 12-TET A440 (median) (c) | -0.3 | 0.2 | 3.5 | 1.4 | -0.8 | 0.3 |  |
| pitch scatter from key to key (c) | 1.6 | 0.8 | 1.1 | 8.3 | 5.8 | 0.5 |  |
| noisiness (spectral flatness) (dB) | -57.4 | -45.2 | -59.9 | -41.8 | -35.1 | 1.1 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -53.0 | -125 | -121 | -123 | -102 | 6.0 | yes |
| pitch at the start re settled (c) | 1.7 | 1.9 | -1.9 | 3.6 | 6.3 | 0.2 |  |
| stereo coherence | 0.55 | 1 | 0.999 | 1 | 0.184 | 1.1 |  |
| partial-to-partial jaggedness (dB) | 5.9 | 6.6 | 4.3 | 6.6 | 4.4 | 0.3 |  |
| dip between legato notes (dB) | 12.0 | 8.4 | 11.4 | 6 | 7.7 | 1.5 |  |
| level evenness of a legato scale (dB) | 1.5 | 1.9 | 2.2 | 0.9 | 2.7 | 0.5 |  |
| level over the keyboard (dB/oct) | -3.42 | -0.59 | 0.03 | -0.88 | -1.31 | 2.7 | yes |
| level against the other instruments (velocity 96) (dB) | 2.3 | 1.7 | -0.8 | 1.1 | -7.7 | 0.5 | yes |
| key-to-key level scatter (dB) | 1.88 | 1.01 | 1 | 0.96 | 1.55 | 1.8 | yes |
| odd / even partials (dB) | -26.6 | -2.7 | -14.6 | -1.5 | -4.4 | 3.5 | yes |
| onset jitter in a phrase (ms) | 47.6 | 53.5 | 42.6 | 43.0 | 48.6 | 0.3 |  |
| onset in a phrase (re the note-on) (ms) | 38.0 | 0 | -1 | -1 | 0 | 3.9 | yes |
| onset sharpness (dB/ms) | 13.9 | 11.1 | 5.9 | 7.4 | 8 | 2.4 | yes |
| overtone levels, note for note (dB rms) | 23.1 (nearest 12.5) | fonts among themselves 13.9 | | | | 1.7 |  |
| panning across the keys (dB/oct) | 0.29 | 0 | 0.02 | 0 | -0.24 | 0.3 |  |
| partial envelopes alike | 0.057 | -0.018 | 0.04 | 0.108 | 0.038 | 0.2 |  |
| release time (30 dB after note-off) (ms) | 462 | 39.0 | 40.0 | 73.0 | 37.0 | 7.2 | yes |
| spectrogram (1 - correlation), note for note | 0.325 (nearest 0.277) | fonts among themselves 0.132 | | | | 2.5 | yes |
| spectral shape of the body, note for note | 12.5 (nearest 9.56) | fonts among themselves 10.3 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 114 | 118 | 110 | 127 | 109 | 0.0 |  |
| partial stretch (inharmonicity) (c/partial) | -0.31 | -0.07 | -0.21 | 0.01 | -0.11 | 0.4 |  |
| level after 2 s held, re peak (dB) | -7.5 | -7.8 | -7.3 | -7.5 | -8.6 | 0.1 |  |
| high bands lag low bands by (ms) | -12.0 | -0.8 | 0 | -2.8 | -0.3 | 2.3 | yes |
| transient coherence: attack spread over bands (ms) | 14.6 | 1.9 | 0 | 6.5 | 2.5 | 2.5 | yes |
| fundamental share (tristimulus 1) | 0.67 | 0.236 | 0.903 | 0.665 | 0.345 | 0.5 |  |
| partials 2-4 share (tristimulus 2) | 0.33 | 0.486 | 0.098 | 0.257 | 0.44 | 0.1 |  |
| partials 5+ share (tristimulus 3) | 0 | 0.044 | 0.001 | 0.123 | 0.105 | 1.1 |  |
| loudness range, velocity 40 to 127 (dB) | 0.7 | 16.6 | 16.6 | 16.6 | 20.1 | 9.1 | yes |
| pitch wobble / vibrato (p-p) (c) | 15.9 | 8.7 | 12.6 | 25.1 | 42.9 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0225 (nearest 0.0198) | fonts among themselves 0.0156 | | | | 1.4 |  |

</details>

### 20 Church Organ: organ-plan

Distance 3.41 (rank 37 of 129); the fonts disagree 1.4 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **No velocity response (0.3 vs 16.6 dB)**: DELIBERATE (pipe organ).
- **Release 1838 ms, floor −26 dB, onset 222 vs 34…86 ms**: DELIBERATE. These are the church and the pipes' speech: README "Church organ" gives ~230 ms at 8' C and ~70 at middle C, plus 24 ms pallets.
- **Quieter (−7.9 vs −1.9…+0.4 dB)**: partly ARTIFACT: the slow speech and the room's build-up eat into the 0.5 s level window. Likely also a trim FAULT. Check the level against a band, as was done for the tonewheel organ (tonewheel_organ.md).
- **Uneven legato scale (3.1 vs 0.3…1.6 dB)**: probably pallet variation plus room. Minor.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (0.4 dB | 16.6 | 16.6 | 16.7 | 20.1), outside the fonts, 9.3 x.
2. **level against the other instruments (velocity 96)**: quieter (-6.5 dB | -0.3 | -1.7 | 0.1 | 1.6), outside the fonts, 3.9 x.
3. **release time (30 dB after note-off) [room]**: rings longer after the key (1838 ms | 358 | 176 | 337 | 287), outside the fonts, 5.0 x.
4. **onset delay**: speaks later (222 ms | 68.0 | 34.0 | 86.0 | 85.0), outside the fonts, 5.6 x.
5. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-26.0 dB | -117 | -119 | -122 | -108), outside the fonts, 13.5 x.
6. **stereo coherence [room]**: wider (0.175 | 1 | 0.998 | 1 | 0.977), outside the fonts, 11.0 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +2 | -2 | -0 | -3 | -8 | +10 |
| re TimGM6mb | -2 | +1 | +2 | +3 | -7 | +2 |
| re Jnsgm2 | -6 | -0 | -2 | -1 | -1 | +5 |
| re SGM | +4 | +3 | +1 | +1 | -3 | -2 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -0 +0 +0 +0; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack -0.2 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 0.5 dB rms; TimGM6mb: centroid -0.1 ERB, attack +0.3 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 1.1 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 0.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.58 | 1.97 | 0.05 | 7.54 | 1.38 | 0.6 | yes |
| attack time (-20 to -1 dB) (ms) | 516 | 559 | 142 | 260 | 314 | 0.9 |  |
| spectral shape of the attack, note for note | 13.8 (nearest 12.9) | fonts among themselves 10.1 | | | | 1.4 |  |
| brightness over the keyboard (ERB/oct) | 3.27 | 3.92 | 4.25 | 3.6 | 3.1 | 0.9 |  |
| key-to-key brightness scatter (ERB) | 1.15 | 0.767 | 1.15 | 1.35 | 1.2 | 0.1 |  |
| brightening from velocity 40 to 127 (ERB) | -0.03 | 0 | -0.01 | 0 | 0.12 | 0.1 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 13.9 | 16.2 | 16.1 | 12.6 | 15.1 | 0.9 |  |
| onset delay (ms) | 222 | 68.0 | 34.0 | 86.0 | 85.0 | 5.6 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.93 | -3.98 | -5.97 | -4.31 | -2.77 | 0.1 |  |
| envelope curve, note for note (dB rms) | 23 (nearest 21.5) | fonts among themselves 7.52 | | | | 3.1 | yes |
| pitch re 12-TET A440 (median) (c) | 0.1 | 7.2 | 3.9 | -7.2 | -0.1 | 0.2 |  |
| pitch scatter from key to key (c) | 1.3 | 3.2 | 2.3 | 1.4 | 1.9 | 0.3 |  |
| noisiness (spectral flatness) (dB) | -47.5 | -38.4 | -66.3 | -36.0 | -46.0 | 0.3 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -26.0 | -117 | -119 | -122 | -108 | 13.5 | yes |
| pitch at the start re settled (c) | -15.2 | 1.5 | -0.1 | -1.9 | -2.1 | 2.8 | yes |
| stereo coherence | 0.175 | 1 | 0.998 | 1 | 0.977 | 11.0 | yes |
| partial-to-partial jaggedness (dB) | 11.7 | 10.1 | 8.75 | 9.5 | 5.4 | 1.1 | yes |
| dip between legato notes (dB) | 8.9 | 6.2 | 6.6 | 8 | 6.5 | 1.2 |  |
| level evenness of a legato scale (dB) | 3.1 | 1.6 | 1 | 0.3 | 1.7 | 1.8 | yes |
| level over the keyboard (dB/oct) | 0.15 | 0.1 | -0.45 | 0.11 | -1.04 | 0.3 |  |
| level against the other instruments (velocity 96) (dB) | -6.5 | -0.3 | -1.7 | 0.1 | 1.6 | 3.9 | yes |
| key-to-key level scatter (dB) | 1.45 | 1.08 | 0.49 | 0.66 | 0.94 | 1.3 | yes |
| odd / even partials (dB) | -12.2 | -8.7 | -15.2 | -9.7 | -5.2 | 0.6 |  |
| onset jitter in a phrase (ms) | 47.7 | 32.1 | 42.9 | 58.7 | 45.5 | 0.3 |  |
| onset in a phrase (re the note-on) (ms) | 68.0 | 14.0 | 14.0 | 36.0 | 38.0 | 3.6 | yes |
| onset sharpness (dB/ms) | 5.7 | 4.8 | 4.4 | 4.6 | 4.5 | 0.6 |  |
| overtone levels, note for note (dB rms) | 10.4 (nearest 8.05) | fonts among themselves 9.72 | | | | 1.1 |  |
| panning across the keys (dB/oct) | -0.65 | 0 | 0.04 | 0 | 0.01 | 0.7 | yes |
| partial envelopes alike | 0.241 | 0.001 | -0.046 | 0.018 | 0.021 | 2.3 | yes |
| release time (30 dB after note-off) (ms) | 1838 | 358 | 176 | 337 | 287 | 5.0 | yes |
| spectrogram (1 - correlation), note for note | 0.367 (nearest 0.16) | fonts among themselves 0.184 | | | | 2.0 |  |
| spectral shape of the body, note for note | 13.3 (nearest 9.93) | fonts among themselves 11.2 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 390 | 327 | 198 | 266 | 243 | 1.7 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.01 | -0.16 | 0.01 | 0.03 | 0.03 | 0.0 |  |
| level after 2 s held, re peak (dB) | -6.2 | -6.2 | -3.8 | -7.2 | -6.3 | 0.0 |  |
| high bands lag low bands by (ms) | -6.8 | -23.0 | -6.7 | -52.6 | 4.2 | 0.3 |  |
| transient coherence: attack spread over bands (ms) | 25.4 | 25.1 | 12.8 | 28.7 | 12.7 | 0.8 |  |
| fundamental share (tristimulus 1) | 0.543 | 0.131 | 0.147 | 0.273 | 0.333 | 3.3 | yes |
| partials 2-4 share (tristimulus 2) | 0.386 | 0.717 | 0.75 | 0.633 | 0.466 | 2.0 | yes |
| partials 5+ share (tristimulus 3) | 0.06 | 0.112 | 0.098 | 0.091 | 0.133 | 0.6 |  |
| loudness range, velocity 40 to 127 (dB) | 0.4 | 16.6 | 16.6 | 16.7 | 20.1 | 9.3 | yes |
| pitch wobble / vibrato (p-p) (c) | 6.5 | 6.2 | 0.9 | 7.1 | 7.5 | 0.0 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0229 (nearest 0.0165) | fonts among themselves 0.0188 | | | | 1.2 |  |

</details>

### 21 Reed Organ: reed-organ

Distance 2.95 (rank 53 of 129); the fonts disagree 0.8 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Quieter (−4.9 vs +0.4…+3.8 dB)**: partly the slow speech in the 0.5 s window. Likely a trim FAULT as well: all four fonts sit above the median.
- **Slow attack (285 vs 49…102 ms), onset 86 vs 28…34 ms, duller attack (−3.6 vs −0.3…+0.4 ERB)**: DELIBERATE in kind. A pressure harmonium's reeds swell, start nearly sinusoidal and brighten (README "Reed organ"). But 285 ms with the Percussion on (README: C3 speaks in ~75 ms) looks slow; check.
- **Brightens with velocity (+1.4 ERB)**: DELIBERATE (Expression: velocity is wind pressure).
- **Release 380 ms**: room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **attack brighter than the body by**: a duller attack (-3.54 ERB | 0.38 | -0.27 | -0.2 | 0.2), outside the fonts, 8.8 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (1.48 ERB | 0 | 0 | 0 | 0.11), outside the fonts, 4.9 x.
3. **attack time (-20 to -1 dB)**: a slower attack (232 ms | 57.5 | 48.5 | 102 | 49.0), outside the fonts, 4.0 x.
4. **onset delay**: speaks later (82.0 ms | 28.0 | 31.0 | 33.0 | 33.0), outside the fonts, 5.0 x.
5. **release time (30 dB after note-off) [room]**: rings longer after the key (384 ms | 111 | 80.0 | 137 | 147), outside the fonts, 3.7 x.
6. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-78.6 dB | -123 | -122 | -123 | -111), outside the fonts, 7.1 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +0 | +2 | +9 | +10 | +4 | -19 |
| re TimGM6mb | -2 | -2 | -2 | -1 | +4 | -0 |
| re Jnsgm2 | +2 | +2 | +1 | +2 | +4 | -8 |
| re SGM | -1 | -2 | -1 | +0 | +6 | -4 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -10 -5 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.5 ERB, attack -0.2 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 2.1 dB rms; TimGM6mb: centroid +0.1 ERB, attack -0.2 ERB, velocity brightening +0.0 ERB, loudness range +4 dB, envelope 1.1 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.0 ERB, loudness range +3 dB, envelope 1.3 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -3.54 | 0.38 | -0.27 | -0.2 | 0.2 | 8.8 | yes |
| attack time (-20 to -1 dB) (ms) | 232 | 57.5 | 48.5 | 102 | 49.0 | 4.0 | yes |
| spectral shape of the attack, note for note | 11.9 (nearest 8.95) | fonts among themselves 7.78 | | | | 1.5 |  |
| brightness over the keyboard (ERB/oct) | 1.41 | 4.22 | 2 | 2.16 | 2.15 | 0.7 | yes |
| key-to-key brightness scatter (ERB) | 1.4 | 0.569 | 1.49 | 1.38 | 1.54 | 0.1 |  |
| brightening from velocity 40 to 127 (ERB) | 1.48 | 0 | 0 | 0 | 0.11 | 4.9 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 14.9 | 13.7 | 14.7 | 14.7 | 14.9 | 0.3 |  |
| onset delay (ms) | 82.0 | 28.0 | 31.0 | 33.0 | 33.0 | 5.0 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.19 | -5.06 | -2.96 | -4.84 | -4.92 | 0.7 |  |
| envelope curve, note for note (dB rms) | 12.6 (nearest 12.2) | fonts among themselves 4.89 | | | | 2.6 | yes |
| pitch re 12-TET A440 (median) (c) | -1.4 | -1.2 | 2.3 | 1.8 | 3 | 0.9 |  |
| pitch scatter from key to key (c) | 3.1 | 1.9 | 0.5 | 0.7 | 0.4 | 0.8 |  |
| noisiness (spectral flatness) (dB) | -73.8 | -69.9 | -64.0 | -63.2 | -71.7 | 1.6 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -78.6 | -123 | -122 | -123 | -111 | 7.1 | yes |
| pitch at the start re settled (c) | -9.4 | 0.1 | -1.1 | -0.6 | -0.1 | 1.8 | yes |
| stereo coherence | 0.935 | 0.924 | 1 | 1 | 1 | 0.9 |  |
| partial-to-partial jaggedness (dB) | 4.8 | 5.6 | 4.45 | 4.95 | 4.1 | 0.1 |  |
| dip between legato notes (dB) | 9.9 | 7.8 | 8.2 | 8.2 | 7 | 1.0 | yes |
| level evenness of a legato scale (dB) | 1.9 | 0.6 | 0.8 | 0.5 | 1.2 | 1.2 | yes |
| level over the keyboard (dB/oct) | 0.55 | -0.21 | 1.29 | 1.07 | 1.22 | 0.6 |  |
| level against the other instruments (velocity 96) (dB) | 2.8 | 0.9 | 2.4 | 3.4 | 5 | 0.0 |  |
| key-to-key level scatter (dB) | 1.75 | 0.7 | 0.77 | 0.63 | 1.02 | 2.0 | yes |
| odd / even partials (dB) | 0.5 | -7.2 | -3.6 | -5.3 | -3.9 | 2.5 | yes |
| onset jitter in a phrase (ms) | 36.8 | 20.6 | 31.9 | 20.7 | 10.2 | 1.5 | yes |
| onset in a phrase (re the note-on) (ms) | 41.0 | 8 | 14.0 | 8 | -1 | 3.3 | yes |
| onset sharpness (dB/ms) | 3.4 | 4.2 | 3.4 | 4.2 | 2.7 | 0.2 |  |
| overtone levels, note for note (dB rms) | 8.75 (nearest 8.48) | fonts among themselves 6.12 | | | | 1.4 | yes |
| panning across the keys (dB/oct) | -0.28 | 0.02 | 0 | 0 | -0.01 | 0.3 |  |
| partial envelopes alike | 0.021 | 0.593 | 0.122 | -0.01 | -0.026 | 0.1 |  |
| release time (30 dB after note-off) (ms) | 384 | 111 | 80.0 | 137 | 147 | 3.7 | yes |
| spectrogram (1 - correlation), note for note | 0.193 (nearest 0.157) | fonts among themselves 0.0873 | | | | 2.2 | yes |
| spectral shape of the body, note for note | 10.7 (nearest 9.81) | fonts among themselves 9.04 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 162 | 147 | 129 | 168 | 169 | 0.2 |  |
| partial stretch (inharmonicity) (c/partial) | 0 | 0 | -0.01 | 0.07 | 0 | 0.0 |  |
| level after 2 s held, re peak (dB) | -4.3 | -6.7 | -5.4 | -5.3 | -3 | 0.3 |  |
| high bands lag low bands by (ms) | 22.0 | -1.2 | -3.5 | 5.1 | 0 | 4.5 | yes |
| transient coherence: attack spread over bands (ms) | 16.7 | 2.3 | 3 | 5.5 | 1.3 | 2.8 | yes |
| fundamental share (tristimulus 1) | 0.278 | 0.141 | 0.225 | 0.108 | 0.16 | 1.8 | yes |
| partials 2-4 share (tristimulus 2) | 0.372 | 0.744 | 0.354 | 0.402 | 0.376 | 0.1 |  |
| partials 5+ share (tristimulus 3) | 0.089 | 0.102 | 0.05 | 0.069 | 0.06 | 0.3 |  |
| loudness range, velocity 40 to 127 (dB) | 14.0 | 16.6 | 16.6 | 16.7 | 20.1 | 1.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 2.3 | 0 | 2.7 | 2.5 | 0.1 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0128 (nearest 0.0117) | fonts among themselves 0.00969 | | | | 1.3 |  |

</details>

### 22 Accordion: accordion

Distance 2.65 (rank 77 of 129); the fonts disagree 1.6 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Key scatter (5.9 vs 0.5…1.1 dB)**: likely FAULT, or a probe interaction. Candidates: the musette's 3.5 beats/s within the 0.5 s window, and a bellows reversal (every 2.5–5 s, ~30 ms gap; README "Accordion and bandoneón") landing in some notes. Check by holding the bellows (one direction) and with `PJ_ACC_BEAT=0`.
- **Duller attack (−6.4 vs −1.4…+0.2 ERB), speaks later (104 vs 19…50 ms)**: DELIBERATE in kind (the duct inertia makes the reed speak; research/free_reeds.md). The size of the difference is large, though: the reed may start too sinusoidally.
- **Narrower dynamics (11.1 vs 16.6 dB)**: plausible for free reeds. TiMidity sets the fonts' range.
- **Stereo coherence 0.35, release 616 ms**: DELIBERATE (bass left, treble right) and room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **attack brighter than the body by**: a duller attack (-6.82 ERB | -1.35 | 0.17 | -0.3 | -0.74), outside the fonts, 8.3 x.
2. **onset delay**: speaks later (107 ms | 50.0 | 25.0 | 23.0 | 18.0), outside the fonts, 5.2 x.
3. **release time (30 dB after note-off) [room]**: rings longer after the key (622 ms | 106 | 62.0 | 143 | 92.0), outside the fonts, 4.4 x.
4. **loudness range, velocity 40 to 127**: a narrower dynamic range (11.9 dB | 16.6 | 16.7 | 16.6 | 20.1), outside the fonts, 2.7 x.
5. **key-to-key level scatter**: less even from key to key (2.07 dB | 1.08 | 0.52 | 0.58 | 0.56), outside the fonts, 3.0 x.
6. **sounding length of a 100 ms note [room]**: longer staccato (318 ms | 136 | 131 | 196 | 145), outside the fonts, 4.1 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -6 | -3 | +1 | +5 | +2 | -3 |
| re TimGM6mb | -4 | -3 | +0 | +2 | +1 | +0 |
| re Jnsgm2 | -3 | -2 | +1 | +1 | -0 | +2 |
| re SGM | +5 | +2 | +6 | +5 | -0 | -11 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -10 -4 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.1 ERB, attack +0.0 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 0.5 dB rms; TimGM6mb: centroid -0.1 ERB, attack +0.0 ERB, velocity brightening +0.0 ERB, loudness range +3 dB, envelope 1.7 dB rms; Jnsgm2: centroid -0.1 ERB, attack -0.1 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 1.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -6.82 | -1.35 | 0.17 | -0.3 | -0.74 | 8.3 | yes |
| attack time (-20 to -1 dB) (ms) | 278 | 189 | 60.0 | 84.0 | 238 | 1.1 | yes |
| spectral shape of the attack, note for note | 12 (nearest 11.4) | fonts among themselves 10.5 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | 2.39 | 3.26 | 2.5 | 2.74 | 3.51 | 1.2 |  |
| key-to-key brightness scatter (ERB) | 0.919 | 1.45 | 1.05 | 0.711 | 1.18 | 0.5 |  |
| brightening from velocity 40 to 127 (ERB) | 0.11 | 0 | 0 | 0 | 0.26 | 0.4 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 16.6 | 13.2 | 14.2 | 15.1 | 18.5 | 0.8 |  |
| onset delay (ms) | 107 | 50.0 | 25.0 | 23.0 | 18.0 | 5.2 | yes |
| Plomp-Levelt dissonance of a note (log2) | -1.59 | -2.57 | -2.3 | -4.35 | -2.3 | 0.8 | yes |
| envelope curve, note for note (dB rms) | 18.1 (nearest 16.7) | fonts among themselves 6.11 | | | | 3.0 | yes |
| pitch re 12-TET A440 (median) (c) | -7.8 | 1 | -14.9 | -1.7 | 4 | 0.8 |  |
| pitch scatter from key to key (c) | 14.1 | 5.6 | 20.2 | 1 | 0.3 | 1.1 |  |
| noisiness (spectral flatness) (dB) | -44.5 | -44.1 | -58.3 | -54.9 | -42.3 | 0.6 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -69.4 | -124 | -122 | -123 | -106 | 5.9 | yes |
| pitch at the start re settled (c) | -1.6 | -5.8 | -0.1 | -3 | 0.3 | 0.0 |  |
| stereo coherence | 0.353 | 0.895 | 0.989 | 1 | 1 | 8.6 | yes |
| partial-to-partial jaggedness (dB) | 5.75 | 3 | 4.75 | 4.9 | 2.35 | 1.5 | yes |
| dip between legato notes (dB) | 8.9 | 9.9 | 9.3 | 8.4 | 3.6 | 0.0 |  |
| level evenness of a legato scale (dB) | 1.4 | 1.7 | 0.8 | 1.7 | 2 | 0.3 |  |
| level over the keyboard (dB/oct) | 1.3 | -0.56 | 1.63 | 1.01 | 0.09 | 0.7 |  |
| level against the other instruments (velocity 96) (dB) | -0.9 | 0.4 | 2.1 | 3.9 | -1.2 | 0.8 |  |
| key-to-key level scatter (dB) | 2.07 | 1.08 | 0.52 | 0.58 | 0.56 | 3.0 | yes |
| odd / even partials (dB) | -1.1 | 3.5 | -3.3 | -8 | -3.8 | 0.4 |  |
| onset jitter in a phrase (ms) | 41.1 | 51.0 | 31.3 | 17.1 | 44.7 | 0.2 |  |
| onset in a phrase (re the note-on) (ms) | 42.0 | 20.0 | 8 | 6 | 0 | 3.5 | yes |
| onset sharpness (dB/ms) | 3.8 | 3 | 5.7 | 6.3 | 5.9 | 1.0 |  |
| overtone levels, note for note (dB rms) | 9.52 (nearest 8.73) | fonts among themselves 7.13 | | | | 1.3 |  |
| panning across the keys (dB/oct) | -0.18 | -0.02 | -0.04 | 0 | -0.01 | 0.2 |  |
| partial envelopes alike | 0.023 | -0.003 | 0.721 | 0.013 | 0.002 | 0.0 |  |
| release time (30 dB after note-off) (ms) | 622 | 106 | 62.0 | 143 | 92.0 | 4.4 | yes |
| spectrogram (1 - correlation), note for note | 0.286 (nearest 0.235) | fonts among themselves 0.131 | | | | 2.2 | yes |
| spectral shape of the body, note for note | 11.6 (nearest 10.6) | fonts among themselves 11.7 | | | | 1.0 |  |
| sounding length of a 100 ms note (ms) | 318 | 136 | 131 | 196 | 145 | 4.1 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.36 | 0.38 | 0.18 | -0.19 | -0.04 | 0.6 |  |
| level after 2 s held, re peak (dB) | -5.9 | -10.1 | -6.7 | -3.7 | -3.8 | 0.2 |  |
| high bands lag low bands by (ms) | -3 | 19.0 | -0.2 | 8.3 | -1.3 | 0.7 |  |
| transient coherence: attack spread over bands (ms) | 19.8 | 9.9 | 3.7 | 6 | 14.3 | 2.2 | yes |
| fundamental share (tristimulus 1) | 0.064 | 0.262 | 0.133 | 0.092 | 0.286 | 1.4 |  |
| partials 2-4 share (tristimulus 2) | 0.52 | 0.55 | 0.452 | 0.645 | 0.411 | 0.2 |  |
| partials 5+ share (tristimulus 3) | 0.257 | 0.102 | 0.08 | 0.26 | 0.331 | 0.6 |  |
| loudness range, velocity 40 to 127 (dB) | 11.9 | 16.6 | 16.7 | 16.6 | 20.1 | 2.7 | yes |
| pitch wobble / vibrato (p-p) (c) | 19.3 | 9.7 | 1 | 2.9 | 6.5 | 2.9 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0206 (nearest 0.0197) | fonts among themselves 0.0159 | | | | 1.3 |  |

</details>

### 23 Harmonica: harmonica

Distance 4.38 (rank 12 of 129); the fonts disagree 0.9 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Velocity 127 is 24 dB softer than 96 (range −10.3 vs +16.6…20.1 dB; darkens −1.45 ERB)**: FAULT, the largest. At the top of the breath map (~3 kPa at v127, harmonica.md §10.3) the reeds appear to choke. Check a held C5 at v127.
- **Key-to-key level 7.8 vs 0.8…1.0 dB**: probable FAULT. It may be the same choke on some reeds, or a blow/draw imbalance. Keys 36–44 fold into C3–D7.
- **Slow (attack 92 vs 44…61 ms; onset 64 vs 28…32)**: mostly DELIBERATE: breath attacks take 20–70 ms (README "Chromatic harmonica").
- **Mid-heavy (+8…+13 dB at 400 Hz–1 kHz, −11…−16 dB at 2.5–6 kHz)**: DELIBERATE: the cupped hands' ~1.1 kHz resonance (README).
- **Release 486 ms**: room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **attack time (-20 to -1 dB)**: a slower attack (508 ms | 44.0 | 50.5 | 61.0 | 46.0), outside the fonts, 13.6 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (456 ms | 49.0 | 49.0 | 50.0 | 50.0), outside the fonts, 12.8 x.
3. **onset delay**: speaks later (106 ms | 32.0 | 28.0 | 31.0 | 31.0), outside the fonts, 7.5 x.
4. **key-to-key level scatter**: less even from key to key (3.43 dB | 0.93 | 1.02 | 0.76 | 0.89), outside the fonts, 5.0 x.
5. **brightening from velocity 40 to 127**: brightens more with velocity (1.33 ERB | 0 | -0 | -0 | 0.19), outside the fonts, 4.4 x.
6. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 18.8, nearest 18.6; the fonts among themselves 2.69), outside.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -28 | -1 | +13 | +5 | -9 | +4 |
| re TimGM6mb | -31 | -2 | +9 | +3 | -12 | +12 |
| re Jnsgm2 | -28 | +0 | +12 | +6 | -12 | +5 |
| re SGM | -26 | +1 | +15 | +7 | -8 | -1 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -7 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.6 ERB, attack -0.1 ERB, velocity brightening -0.0 ERB, loudness range +3 dB, envelope 1.4 dB rms; TimGM6mb: centroid -0.1 ERB, attack +0.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 0.7 dB rms; Jnsgm2: centroid -0.2 ERB, attack -0.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 0.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -4.42 | -1.47 | -2.28 | -2.29 | -1.58 | 6.1 | yes |
| attack time (-20 to -1 dB) (ms) | 508 | 44.0 | 50.5 | 61.0 | 46.0 | 13.6 | yes |
| spectral shape of the attack, note for note | 16.5 (nearest 15) | fonts among themselves 9.1 | | | | 1.8 | yes |
| brightness over the keyboard (ERB/oct) | 0.95 | 4.62 | 3.93 | 4.04 | 4.86 | 7.3 | yes |
| key-to-key brightness scatter (ERB) | 1.13 | 1.33 | 0.684 | 0.429 | 1.1 | 0.5 |  |
| brightening from velocity 40 to 127 (ERB) | 1.33 | 0 | -0 | -0 | 0.19 | 4.4 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 13.6 | 13.4 | 15.3 | 15.8 | 15.2 | 1.4 |  |
| onset delay (ms) | 106 | 32.0 | 28.0 | 31.0 | 31.0 | 7.5 | yes |
| Plomp-Levelt dissonance of a note (log2) | -5.21 | -4.21 | -3.39 | -3.89 | -3.72 | 3.4 | yes |
| envelope curve, note for note (dB rms) | 18.8 (nearest 18.6) | fonts among themselves 2.69 | | | | 7.0 | yes |
| pitch re 12-TET A440 (median) (c) | -0.2 | 0.4 | 2.1 | 2.7 | 0.7 | 0.4 |  |
| pitch scatter from key to key (c) | 1.7 | 0.7 | 1.6 | 0.8 | 0.3 | 0.3 |  |
| noisiness (spectral flatness) (dB) | -63.9 | -53.8 | -62.6 | -62.7 | -51.2 | 1.0 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -72.9 | -123 | -120 | -120 | -105 | 5.4 | yes |
| pitch at the start re settled (c) | -11.6 | -8.4 | -3.4 | -9.1 | -9.3 | 0.6 |  |
| stereo coherence | 0.851 | 0.958 | 1 | 1 | 1 | 2.0 | yes |
| partial-to-partial jaggedness (dB) | 5.5 | 5.1 | 5.7 | 5.85 | 5.3 | 0.0 |  |
| dip between legato notes (dB) | 14.5 | 6 | 9.4 | 9.3 | 6.1 | 3.4 | yes |
| level evenness of a legato scale (dB) | 0.8 | 1.2 | 0.5 | 0.3 | 2.1 | 0.0 |  |
| level over the keyboard (dB/oct) | 0.49 | 0 | 1.18 | 1.31 | 0.92 | 0.6 |  |
| level against the other instruments (velocity 96) (dB) | -1.8 | 0.2 | 0 | 1.4 | -0.4 | 1.9 | yes |
| key-to-key level scatter (dB) | 3.43 | 0.93 | 1.02 | 0.76 | 0.89 | 5.0 | yes |
| odd / even partials (dB) | -6.5 | -1.5 | 5.2 | 5.7 | -1.6 | 2.3 | yes |
| onset jitter in a phrase (ms) | 41.4 | 21.5 | 20.2 | 21.5 | 21.1 | 4.0 | yes |
| onset in a phrase (re the note-on) (ms) | 28.0 | 4 | 34.0 | 32.0 | -0 | 0.6 |  |
| onset sharpness (dB/ms) | 5.1 | 3.9 | 6.5 | 6.2 | 4.7 | 0.2 |  |
| overtone levels, note for note (dB rms) | 18.1 (nearest 16.2) | fonts among themselves 9.95 | | | | 1.8 | yes |
| panning across the keys (dB/oct) | -0.61 | 0 | -0.08 | 0.04 | -0.01 | 0.6 | yes |
| partial envelopes alike | 0.121 | 0.769 | 0.997 | 0.998 | 0.97 | 7.5 | yes |
| release time (30 dB after note-off) (ms) | 456 | 49.0 | 49.0 | 50.0 | 50.0 | 12.8 | yes |
| spectrogram (1 - correlation), note for note | 0.437 (nearest 0.389) | fonts among themselves 0.0861 | | | | 5.1 | yes |
| spectral shape of the body, note for note | 14.1 (nearest 11.6) | fonts among themselves 9.81 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 172 | 122 | 126 | 126 | 125 | 1.8 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.01 | 0 | 0 | 0 | 0 | 0.0 |  |
| level after 2 s held, re peak (dB) | -5 | -10.2 | -2.7 | -2.8 | -4.2 | 0.4 |  |
| high bands lag low bands by (ms) | 16.0 | 17.0 | 11.3 | 9.6 | 7.5 | 1.1 |  |
| transient coherence: attack spread over bands (ms) | 7.4 | 10.6 | 11.6 | 12.8 | 11.0 | 0.8 | yes |
| fundamental share (tristimulus 1) | 0.218 | 0.516 | 0.386 | 0.328 | 0.489 | 2.3 | yes |
| partials 2-4 share (tristimulus 2) | 0.24 | 0.254 | 0.413 | 0.543 | 0.249 | 0.6 |  |
| partials 5+ share (tristimulus 3) | 0.013 | 0.217 | 0.189 | 0.153 | 0.233 | 2.7 | yes |
| loudness range, velocity 40 to 127 (dB) | 16.0 | 16.7 | 16.6 | 16.6 | 20.1 | 0.4 |  |
| pitch wobble / vibrato (p-p) (c) | 7.5 | 1.1 | 0 | 0 | 1 | 1.4 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0208 (nearest 0.0182) | fonts among themselves 0.0101 | | | | 2.1 | yes |

</details>

### 24 Tango Accordion: bandoneon

Distance 4.53 (rank 9 of 129); the fonts disagree 1.2 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Brighter (19.7 vs 14.3…15.6 ERB; −8…−17 dB below 400 Hz)**: likely FAULT. research/free_reeds.md §4 describes zinc-plate bandoneóns as "a large, deep sound", "sweet and somewhat dull". The model's brighter grille, thicker plates and the 4' octave overshoot that, and nothing is fitted to a recording yet (§10; *Escualo* is the candidate).
- **Key scatter (8.3 vs 0.5…1.1 dB)**: the worst of the set. It shares p021's suspects: bellows reversals and the probe window.
- **Slow attack (252 vs 58…84 ms), onset 85 ms**: the tango marcato wants a quicker attack. Possible FAULT.
- **Quieter (−3.4 dB), release 719 ms**: a trim; the release is room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **attack time (-20 to -1 dB)**: a slower attack (262 ms | 70.0 | 58.5 | 84.0 | 79.0), outside the fonts, 7.0 x.
2. **level against the other instruments (velocity 96)**: quieter (-3 dB | 1.4 | 1.4 | 3.9 | 2.8), outside the fonts, 4.1 x.
3. **onset delay**: speaks later (100 ms | 34.0 | 27.0 | 23.0 | 36.0), outside the fonts, 7.0 x.
4. **brightening from velocity 40 to 127**: brightens more with velocity (1.44 ERB | 0 | 0 | 0 | -0.08), outside the fonts, 4.8 x.
5. **attack brighter than the body by**: a duller attack (-7.16 ERB | -2.01 | -0.72 | -0.3 | -1.56), outside the fonts, 7.0 x.
6. **onset in a phrase (re the note-on)**: speaks later in a phrase (68.0 ms | 8 | 2 | 6 | -1), outside the fonts, 6.4 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -6 | +2 | +7 | +6 | +2 | -11 |
| re TimGM6mb | -5 | +0 | +3 | +4 | +1 | -6 |
| re Jnsgm2 | -3 | +2 | +4 | +4 | +0 | -6 |
| re SGM | -9 | +4 | +8 | +5 | +2 | -10 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -9 -4 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.5 ERB, attack -0.4 ERB, velocity brightening +0.0 ERB, loudness range +4 dB, envelope 1.7 dB rms; TimGM6mb: centroid -0.2 ERB, attack +0.1 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 1.8 dB rms; Jnsgm2: centroid -0.1 ERB, attack -0.1 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 1.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -7.16 | -2.01 | -0.72 | -0.3 | -1.56 | 7.0 | yes |
| attack time (-20 to -1 dB) (ms) | 262 | 70.0 | 58.5 | 84.0 | 79.0 | 7.0 | yes |
| spectral shape of the attack, note for note | 17.3 (nearest 15.2) | fonts among themselves 9.8 | | | | 1.8 | yes |
| brightness over the keyboard (ERB/oct) | 2.62 | 4.08 | 2.44 | 2.74 | 4.66 | 0.7 |  |
| key-to-key brightness scatter (ERB) | 1.67 | 0.741 | 1.52 | 0.711 | 1.33 | 1.6 | yes |
| brightening from velocity 40 to 127 (ERB) | 1.44 | 0 | 0 | 0 | -0.08 | 4.8 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 15.4 | 14.8 | 14.3 | 15.1 | 15.6 | 0.8 |  |
| onset delay (ms) | 100 | 34.0 | 27.0 | 23.0 | 36.0 | 7.0 | yes |
| Plomp-Levelt dissonance of a note (log2) | -5.38 | -2.93 | -2.65 | -4.35 | -3.31 | 2.7 | yes |
| envelope curve, note for note (dB rms) | 19.9 (nearest 18.1) | fonts among themselves 5.81 | | | | 3.4 | yes |
| pitch re 12-TET A440 (median) (c) | 0.2 | 0.3 | -16.2 | -1.7 | -3.1 | 0.3 |  |
| pitch scatter from key to key (c) | 2.1 | 0.9 | 17.3 | 1 | 1.3 | 0.1 |  |
| noisiness (spectral flatness) (dB) | -57.2 | -45.1 | -57.8 | -54.9 | -52.6 | 0.5 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -67.3 | -124 | -122 | -123 | -110 | 7.9 | yes |
| pitch at the start re settled (c) | -8.8 | -8.8 | -3.6 | -3 | -3.3 | 1.1 |  |
| stereo coherence | 0.36 | 0.946 | 0.961 | 1 | 1 | 8.3 | yes |
| partial-to-partial jaggedness (dB) | 5.85 | 4.3 | 3.7 | 4.9 | 5.2 | 1.2 | yes |
| dip between legato notes (dB) | 6.5 | 7.4 | 7.6 | 8.4 | 6.9 | 0.5 |  |
| level evenness of a legato scale (dB) | 1.1 | 2 | 0.7 | 1.7 | 0.6 | 0.1 |  |
| level over the keyboard (dB/oct) | -0.78 | -0.42 | 0.96 | 1.01 | 0.09 | 1.3 |  |
| level against the other instruments (velocity 96) (dB) | -3 | 1.4 | 1.4 | 3.9 | 2.8 | 4.1 | yes |
| key-to-key level scatter (dB) | 2.55 | 1.06 | 0.54 | 0.58 | 0.74 | 3.8 | yes |
| odd / even partials (dB) | -8.5 | 3.4 | -2.2 | -8 | 0.4 | 1.3 |  |
| onset jitter in a phrase (ms) | 37.2 | 43.0 | 36.1 | 17.1 | 15.5 | 0.8 |  |
| onset in a phrase (re the note-on) (ms) | 68.0 | 8 | 2 | 6 | -1 | 6.4 | yes |
| onset sharpness (dB/ms) | 3.4 | 3.5 | 4.8 | 6.3 | 4.2 | 0.6 |  |
| overtone levels, note for note (dB rms) | 10.2 (nearest 8.91) | fonts among themselves 7.42 | | | | 1.4 |  |
| panning across the keys (dB/oct) | 0.03 | 0.01 | -0.73 | 0 | -0.01 | 0.0 |  |
| partial envelopes alike | 0.015 | 0.06 | 0.383 | 0.013 | 0.055 | 0.2 |  |
| release time (30 dB after note-off) (ms) | 657 | 90.0 | 44.0 | 143 | 48.0 | 3.9 | yes |
| spectrogram (1 - correlation), note for note | 0.328 (nearest 0.279) | fonts among themselves 0.118 | | | | 2.8 | yes |
| spectral shape of the body, note for note | 10.3 (nearest 9.1) | fonts among themselves 10.5 | | | | 1.0 |  |
| sounding length of a 100 ms note (ms) | 244 | 121 | 116 | 196 | 122 | 2.7 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.01 | -0.1 | 0.34 | -0.19 | 0.01 | 0.1 |  |
| level after 2 s held, re peak (dB) | -4.2 | -8.1 | -6.1 | -3.7 | -8.3 | 1.0 |  |
| high bands lag low bands by (ms) | 15.7 | 8.8 | 5.5 | 8.3 | 10.7 | 1.4 | yes |
| transient coherence: attack spread over bands (ms) | 16.4 | 18.6 | 8.4 | 6 | 15.9 | 0.7 |  |
| fundamental share (tristimulus 1) | 0.046 | 0.328 | 0.101 | 0.092 | 0.426 | 1.0 | yes |
| partials 2-4 share (tristimulus 2) | 0.663 | 0.458 | 0.418 | 0.645 | 0.309 | 1.3 |  |
| partials 5+ share (tristimulus 3) | 0.1 | 0.197 | 0.06 | 0.26 | 0.226 | 1.1 |  |
| loudness range, velocity 40 to 127 (dB) | 11.3 | 16.6 | 16.6 | 16.6 | 20.1 | 3.0 | yes |
| pitch wobble / vibrato (p-p) (c) | 2.2 | 10.2 | 4 | 2.9 | 6.4 | 0.6 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0191 (nearest 0.0148) | fonts among themselves 0.0123 | | | | 1.5 |  |

</details>

### 25 Nylon Guitar: nylon-guitar

Distance 3.85 (rank 24 of 129); the fonts disagree 1.1 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Louder (+5.9 vs −2…+1 dB) together with a velocity-curve kink (−13 / −10 / 0 / +5 vs −13 / −6 / 0 / +4)**: likely FAULT in the velocity law. v64 sits 4 dB under the fonts' shape and v96 is 10 dB above it, so at v96 the guitar is too loud and at v64 about right. The nail coming in between 40 and 105 (research/guitars.md, "Picking's colour by velocity") may add a level step on top of the colour.
- **Brightens with velocity (+2.1 vs 0 ERB)**: DELIBERATE, the user's design (flesh to nail; README "Classical guitar").
- **"Brighter" (10.9 vs 8.7…9.4 ERB)**: the band table says the opposite (+12…+19 dB at 150 Hz–1 kHz, −13…−27 dB above 2.5 kHz). This is a MEASUREMENT conflict: the guitar is fuller in the body and darker on top.
- **Key scatter (4.2 vs 0.8…1.6 dB), decays faster (16.6 vs 9…12 dB/s)**: the scatter is partly the guitarist's string and fret choice. The decay is within a real nylon guitar's range and was set by ear (`PJ_GUITAR_RING`).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (4.43 dB | 0.84 | 0.87 | 0.85 | 1.63), outside the fonts, 7.1 x.
2. **brightness (spectral centroid, velocity 96)**: brighter (10.9 ERB | 9.07 | 9.42 | 8.71 | 9.12), outside the fonts, 5.1 x.
3. **brightening from velocity 40 to 127**: brightens more with velocity (1.75 ERB | 0.03 | -0 | 0 | -0.12), outside the fonts, 5.8 x.
4. **attack time (-20 to -1 dB)**: a slower attack (18.0 ms | 11.0 | 6 | 6 | 6), outside the fonts, 3.6 x.
5. **level against the other instruments (velocity 96)**: louder (3.3 dB | -1.6 | 0.2 | -1.9 | 2.2), outside the fonts, 2.0 x.
6. **level after 2 s held, re peak**: sustains less (-40.8 dB | -30.6 | -24.6 | -26.7 | -32.2), outside the fonts, 3.2 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +13 | +11 | +12 | -3 | -13 | -18 |
| re TimGM6mb | +2 | +11 | +10 | -0 | -10 | -14 |
| re Jnsgm2 | +3 | +17 | +18 | +3 | -10 | -26 |
| re SGM | -8 | +6 | +2 | +3 | -8 | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -14 -8 +0 +4; FluidR3 -12 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack -0.0 ERB, velocity brightening +0.4 ERB, loudness range +4 dB, envelope 0.4 dB rms; TimGM6mb: centroid -0.8 ERB, attack +0.3 ERB, velocity brightening -0.0 ERB, loudness range +3 dB, envelope 0.7 dB rms; Jnsgm2: centroid -0.9 ERB, attack +0.6 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 0.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.84 | 1.1 | 0.08 | -0.22 | -0.2 | 1.2 | yes |
| attack time (-20 to -1 dB) (ms) | 18.0 | 11.0 | 6 | 6 | 6 | 3.6 | yes |
| spectral shape of the attack, note for note | 14.1 (nearest 12) | fonts among themselves 9.64 | | | | 1.5 |  |
| brightness over the keyboard (ERB/oct) | 2.78 | 4.02 | 3.39 | 3.43 | 2.95 | 1.2 | yes |
| key-to-key brightness scatter (ERB) | 1.4 | 1.43 | 0.472 | 0.527 | 1.03 | 1.3 |  |
| brightening from velocity 40 to 127 (ERB) | 1.75 | 0.03 | -0 | 0 | -0.12 | 5.8 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 10.9 | 9.07 | 9.42 | 8.71 | 9.12 | 5.1 | yes |
| decay rate of a held note (dB/s) | 16.8 | 11.5 | 9 | 9.4 | 11.8 | 2.8 | yes |
| onset delay (ms) | 16.0 | 9 | 7 | 6 | 7 | 0.9 | yes |
| Plomp-Levelt dissonance of a note (log2) | -6.11 | -7.51 | -5.38 | -4.04 | -5.64 | 0.3 |  |
| envelope curve, note for note (dB rms) | 10.9 (nearest 9.14) | fonts among themselves 8.06 | | | | 1.4 |  |
| pitch re 12-TET A440 (median) (c) | 0.7 | -0.5 | 1.7 | -5.4 | -1.5 | 0.4 |  |
| pitch scatter from key to key (c) | 0.7 | 1.3 | 0.6 | 5.3 | 2.2 | 0.4 |  |
| noisiness (spectral flatness) (dB) | -87.2 | -70.4 | -66.2 | -58.8 | -63.8 | 3.8 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -108 | -125 | -123 | -122 | -113 | 2.4 | yes |
| pitch at the start re settled (c) | -1.4 | 0.7 | 0.4 | 0.2 | 0.8 | 0.4 |  |
| stereo coherence | 0.996 | 1 | 1 | 1 | 1 | 0.1 |  |
| partial-to-partial jaggedness (dB) | 6.9 | 3.65 | 4.2 | 6.6 | 5 | 1.6 |  |
| dip between legato notes (dB) | 11.1 | 8.1 | 8.3 | 7.8 | 7 | 1.6 | yes |
| level evenness of a legato scale (dB) | 2.2 | 1.4 | 1.2 | 2 | 3.9 | 0.4 |  |
| level over the keyboard (dB/oct) | -0.42 | -3.39 | -1.22 | -1.49 | -0.7 | 0.7 |  |
| level against the other instruments (velocity 96) (dB) | 3.3 | -1.6 | 0.2 | -1.9 | 2.2 | 2.0 | yes |
| key-to-key level scatter (dB) | 4.43 | 0.84 | 0.87 | 0.85 | 1.63 | 7.1 | yes |
| odd / even partials (dB) | -5.6 | -6.1 | -3.8 | -3.6 | -7.9 | 0.3 |  |
| onset jitter in a phrase (ms) | 12.2 | 34.6 | 31.4 | 25.5 | 22.6 | 2.7 | yes |
| onset in a phrase (re the note-on) (ms) | 14.0 | 4 | -0 | 8 | 0 | 1.2 | yes |
| onset sharpness (dB/ms) | 21.5 | 15.4 | 10.7 | 7.3 | 13.4 | 2.3 | yes |
| overtone levels, note for note (dB rms) | 16.4 (nearest 14.9) | fonts among themselves 13.2 | | | | 1.2 |  |
| panning across the keys (dB/oct) | 0.1 | -0 | -0 | -0 | 0.02 | 0.1 |  |
| partial envelopes alike | 0.921 | 0.928 | 1 | 1 | 0.966 | 0.6 |  |
| release time (30 dB after note-off) (ms) | 155 | 240 | 144 | 177 | 178 | 0.5 |  |
| spectrogram (1 - correlation), note for note | 0.278 (nearest 0.254) | fonts among themselves 0.24 | | | | 1.2 |  |
| spectral shape of the body, note for note | 12.2 (nearest 12.1) | fonts among themselves 11 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 177 | 251 | 131 | 118 | 114 | 0.9 |  |
| partial stretch (inharmonicity) (c/partial) | 0.12 | 0.43 | 0 | 0 | 0.87 | 0.2 |  |
| level after 2 s held, re peak (dB) | -40.8 | -30.6 | -24.6 | -26.7 | -32.2 | 3.2 | yes |
| high bands lag low bands by (ms) | 2 | -0.1 | -2 | 0 | 0 | 0.4 |  |
| transient coherence: attack spread over bands (ms) | 1.9 | 1.4 | 0.8 | 0.4 | 0.7 | 0.2 |  |
| fundamental share (tristimulus 1) | 0.274 | 0.225 | 0.599 | 0.474 | 0.918 | 0.8 |  |
| partials 2-4 share (tristimulus 2) | 0.722 | 0.764 | 0.393 | 0.512 | 0.071 | 0.8 |  |
| partials 5+ share (tristimulus 3) | 0 | 0.013 | 0.006 | 0.036 | 0.001 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 18.1 | 16.5 | 16.7 | 16.6 | 20.1 | 0.8 |  |
| pitch wobble / vibrato (p-p) (c) | 5.3 | 1.4 | 0 | 0.1 | 3.4 | 0.9 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0154 (nearest 0.0112) | fonts among themselves 0.0125 | | | | 1.2 |  |

</details>

### 26 Steel Guitar: steel-guitar

Distance 2.68 (rank 69 of 129); the fonts disagree 1.2 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Louder (+7 vs −4.9…+1.1 dB)** at every velocity: a trim FAULT.
- **Sharper onset (52.9 vs 10…15 dB/ms), brighter at 1–6 kHz (+7…+13 dB)**: DELIBERATE pick click (README steel guitar, `PJ_STEEL_CLICK`), targeted at FluidR3's first-5 ms excess above 5 kHz (research/steel_guitar_build.md §1.3). The ~−17 dB above 6 kHz re the TiMidity fonts suggests the click lacks top end rather than having too much.
- **Brightens with velocity (+2.0 ERB)**: DELIBERATE: a hard stroke is nearer the bridge. steel_guitar_build.md §1.3 found FluidR3 has no timbre change under TiMidity.
- **Partial envelopes alike (0.52 vs 0.92…1.0)**: possible FAULT. Even FluidR3's recorded guitar keeps 0.96; the two-polarisation beating or the sympathetic strings may be overdone.
- **Decays slower (7.8 vs 10.6…18.4 dB/s)**: DELIBERATE. It is fitted to the in-recording decays of FluidR3's samples (§1.2), not to the fonts' loop envelopes.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (1.74 ERB | 0.08 | -0 | -0 | -0.16), outside the fonts, 5.8 x.
2. **brightness (spectral centroid, velocity 96)**: brighter (14.3 ERB | 10.9 | 9.14 | 11.2 | 8.47), outside the fonts, 3.1 x.
3. **onset sharpness**: a sharper onset (52.7 dB/ms | 12.8 | 10.5 | 15.4 | 9.9), outside the fonts, 14.9 x.
4. **attack time (-20 to -1 dB)**: a slower attack (26.5 ms | 15.0 | 8 | 7 | 7), outside the fonts, 3.3 x.
5. **loudness range, velocity 40 to 127**: a narrower dynamic range (14.1 dB | 16.6 | 16.7 | 16.7 | 20.1), outside the fonts, 1.5 x.
6. **partial envelopes alike**: partials living more their own lives (0.561 | 0.96 | 1 | 0.969 | 0.919), outside the fonts, 4.0 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -7 | +1 | +1 | +11 | +7 | -16 |
| re TimGM6mb | -28 | +6 | +10 | +14 | +8 | -19 |
| re Jnsgm2 | -23 | +7 | +8 | +10 | +10 | -18 |
| re SGM | -36 | -8 | -2 | +9 | +8 | +9 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening +0.2 ERB, loudness range +3 dB, envelope 0.5 dB rms; TimGM6mb: centroid -0.9 ERB, attack +0.3 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 0.8 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 0.4 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -1.13 | 1.87 | -0.25 | -0.47 | -1.24 | 0.5 |  |
| attack time (-20 to -1 dB) (ms) | 26.5 | 15.0 | 8 | 7 | 7 | 3.3 | yes |
| spectral shape of the attack, note for note | 11.3 (nearest 9.93) | fonts among themselves 11.1 | | | | 1.0 |  |
| brightness over the keyboard (ERB/oct) | 1.25 | 2.9 | 4.48 | 4.85 | 3.23 | 2.7 | yes |
| key-to-key brightness scatter (ERB) | 1.92 | 1.03 | 1.62 | 2.61 | 0.809 | 0.7 |  |
| brightening from velocity 40 to 127 (ERB) | 1.74 | 0.08 | -0 | -0 | -0.16 | 5.8 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 14.3 | 10.9 | 9.14 | 11.2 | 8.47 | 3.1 | yes |
| decay rate of a held note (dB/s) | 7.9 | 10.6 | 11.1 | 18.4 | 11.2 | 1.2 | yes |
| onset delay (ms) | 12.0 | 10.0 | 6 | 9 | 7 | 0.4 |  |
| Plomp-Levelt dissonance of a note (log2) | -4.37 | -3.98 | -4.35 | -4.68 | -6.38 | 0.1 |  |
| envelope curve, note for note (dB rms) | 14.9 (nearest 11.4) | fonts among themselves 12.7 | | | | 1.2 |  |
| pitch re 12-TET A440 (median) (c) | 1 | -0.7 | 1.7 | 2.4 | -0.4 | 0.1 |  |
| pitch scatter from key to key (c) | 1 | 1 | 0.4 | 1.2 | 3 | 0.0 |  |
| noisiness (spectral flatness) (dB) | -61.4 | -61.5 | -56.3 | -52.3 | -66.7 | 0.3 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -104 | -59.3 | -121 | -121 | -113 | 0.4 |  |
| pitch at the start re settled (c) | -0.3 | 0 | 0 | 1.8 | 1.3 | 0.2 |  |
| stereo coherence | 0.993 | 1 | 1 | 1 | 1 | 0.1 |  |
| partial-to-partial jaggedness (dB) | 6.25 | 4.7 | 4.7 | 4.2 | 5.2 | 1.5 | yes |
| dip between legato notes (dB) | 5.2 | 8.1 | 6.1 | 7 | 10.7 | 1.0 |  |
| level evenness of a legato scale (dB) | 1.9 | 1.4 | 1.1 | 0.8 | 3.7 | 0.4 |  |
| level over the keyboard (dB/oct) | -0.4 | -2.09 | -0.19 | -1.41 | -0.46 | 0.5 |  |
| level against the other instruments (velocity 96) (dB) | -0.6 | -1.4 | -2.6 | -4.8 | 2.3 | 0.4 |  |
| key-to-key level scatter (dB) | 1.71 | 1.1 | 0.85 | 1.1 | 1.12 | 1.2 | yes |
| odd / even partials (dB) | -1.2 | -5.2 | -4.9 | -4.1 | -9.3 | 1.5 | yes |
| onset jitter in a phrase (ms) | 11.3 | 37.7 | 2.5 | 12.7 | 23.9 | 0.4 |  |
| onset in a phrase (re the note-on) (ms) | 6 | 4 | -0 | 2 | 0 | 0.5 |  |
| onset sharpness (dB/ms) | 52.7 | 12.8 | 10.5 | 15.4 | 9.9 | 14.9 | yes |
| overtone levels, note for note (dB rms) | 15 (nearest 13.7) | fonts among themselves 11.5 | | | | 1.3 |  |
| panning across the keys (dB/oct) | 0.03 | -0 | -0 | -0 | 0.02 | 0.0 |  |
| partial envelopes alike | 0.561 | 0.96 | 1 | 0.969 | 0.919 | 4.0 | yes |
| release time (30 dB after note-off) (ms) | 144 | 706 | 68.0 | 170 | 83.0 | 0.2 |  |
| spectrogram (1 - correlation), note for note | 0.265 (nearest 0.249) | fonts among themselves 0.231 | | | | 1.1 |  |
| spectral shape of the body, note for note | 12.4 (nearest 12) | fonts among themselves 11.7 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 154 | 326 | 70.0 | 120 | 53.0 | 0.6 |  |
| partial stretch (inharmonicity) (c/partial) | 0.34 | 0.57 | 0 | 0.71 | 0.8 | 0.6 |  |
| level after 2 s held, re peak (dB) | -17.9 | -27.3 | -28.4 | -44.9 | -32.2 | 1.4 | yes |
| high bands lag low bands by (ms) | 0.5 | -2.5 | 0 | -2.5 | 0 | 0.3 |  |
| transient coherence: attack spread over bands (ms) | 5 | 1.7 | 0 | 1.1 | 0.9 | 0.8 | yes |
| fundamental share (tristimulus 1) | 0.08 | 0.208 | 0.754 | 0.56 | 0.83 | 1.9 | yes |
| partials 2-4 share (tristimulus 2) | 0.378 | 0.708 | 0.233 | 0.374 | 0.157 | 0.3 |  |
| partials 5+ share (tristimulus 3) | 0.22 | 0.038 | 0.066 | 0.034 | 0.004 | 2.6 | yes |
| loudness range, velocity 40 to 127 (dB) | 14.1 | 16.6 | 16.7 | 16.7 | 20.1 | 1.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 5 | 1.6 | 0.1 | 2.6 | 2.1 | 0.6 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0169 (nearest 0.0162) | fonts among themselves 0.0148 | | | | 1.1 |  |

</details>

### 27 Jazz Guitar: jazz-guitar

Distance 2.20 (rank 96 of 129); the fonts disagree 1.1 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Darker above 1 kHz than three fonts (−3 to −12 dB at 1–2.5 kHz; +6 to +16 dB below 1 kHz)**: deliberate. The warm archtop (the neck humbucker's comb, tone at "3", closed 15") was chosen over FluidR3's brighter sample (research/jazz_guitar.md §12.4, "Not met": centroid 348 Hz against FluidR3's 901). The centroid is inside the fonts' range (9.2 vs 8.7–15.8 ERB), and SGM is darker than ours above 2.5 kHz (−15 dB).
- **Speaks late: onset 22–24 ms vs 0–6 ms, attack 20 ms vs 1–5**: a small fault. The 8 ms gesture wait (README "Jazz guitar") plus the hand's push (up to 10 ms soft) add up. A noteLead of about 15 ms would put the comping back on the beat.
- **Wobblier and rougher held note (7 c p-p vs ≤1.7; dissonance −4.0 vs −5.0…−6.7)**: mostly deliberate. The late shallow vibrato and the floating bridge's two swing directions beat (README). Worth a listen on long chords.
- **Quieter (−2.7 dB vs +0.7 median)**, long release, high floor: the room's release and floor are an artifact; the level difference is minor.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **attack time (-20 to -1 dB)**: a slower attack (25.5 ms | 3 | 4 | 5 | 3), outside the fonts, 7.8 x.
2. **decay rate of a held note**: decays slower (5.5 dB/s | 6.4 | 11.0 | 11.1 | 10.6), outside the fonts, 2.5 x.
3. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-83.4 dB | -126 | -124 | -124 | -114), outside the fonts, 6.6 x.
4. **release time (30 dB after note-off) [room]**: rings longer after the key (504 ms | 15.0 | 81.0 | 65.0 | 91.0), outside the fonts, 2.2 x.
5. **key-to-key level scatter**: less even from key to key (2.62 dB | 2.11 | 1.32 | 1.4 | 1.94), outside the fonts, 1.9 x.
6. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 14.4, nearest 13.7; the fonts among themselves 5.28), outside.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +12 | +10 | +4 | -11 | -4 | - |
| re TimGM6mb | +14 | +7 | +8 | -3 | -4 | -36 |
| re Jnsgm2 | +11 | +4 | +4 | -4 | -2 | -30 |
| re SGM | -4 | -3 | -7 | -1 | +18 | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -15 -7 +0 +3; FluidR3 -16 -9 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.3 ERB, attack -0.1 ERB, velocity brightening +0.2 ERB, loudness range +5 dB, envelope 0.4 dB rms; TimGM6mb: centroid -0.1 ERB, attack +0.0 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 0.9 dB rms; Jnsgm2: centroid -0.2 ERB, attack -0.3 ERB, velocity brightening +0.5 ERB, loudness range +5 dB, envelope 1.2 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.06 | 0.73 | 0.5 | 1.09 | 0.25 | 1.3 |  |
| attack time (-20 to -1 dB) (ms) | 25.5 | 3 | 4 | 5 | 3 | 7.8 | yes |
| spectral shape of the attack, note for note | 16.4 (nearest 10.5) | fonts among themselves 11.5 | | | | 1.4 |  |
| brightness over the keyboard (ERB/oct) | 3.3 | 3.75 | 4.08 | 3.73 | 3.14 | 0.9 |  |
| key-to-key brightness scatter (ERB) | 1.1 | 1.75 | 0.606 | 0.375 | 0.992 | 0.4 |  |
| brightening from velocity 40 to 127 (ERB) | 0.9 | 2.86 | -0 | -0 | -0.29 | 0.6 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 8.7 | 15.8 | 9.52 | 8.66 | 9.7 | 0.3 |  |
| decay rate of a held note (dB/s) | 5.5 | 6.4 | 11.0 | 11.1 | 10.6 | 2.5 | yes |
| onset delay (ms) | 24.0 | 6 | 5 | 5 | 4 | 1.9 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.25 | -6.06 | -6.38 | -6.65 | -5.01 | 2.4 | yes |
| envelope curve, note for note (dB rms) | 14.4 (nearest 13.7) | fonts among themselves 5.28 | | | | 2.7 | yes |
| pitch re 12-TET A440 (median) (c) | -0.7 | -1.6 | 1.9 | 1.8 | -1.1 | 0.3 |  |
| pitch scatter from key to key (c) | 0.9 | 2 | 0.3 | 1 | 0.7 | 0.0 |  |
| noisiness (spectral flatness) (dB) | -81.3 | -71.5 | -64.4 | -64.0 | -75.9 | 2.2 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -83.4 | -126 | -124 | -124 | -114 | 6.6 | yes |
| pitch at the start re settled (c) | 1.1 | 0.3 | -0.2 | -1.5 | 0.1 | 0.2 |  |
| stereo coherence | 0.708 | 1 | 1 | 1 | 1 | 3.9 | yes |
| partial-to-partial jaggedness (dB) | 5.3 | 4.45 | 4.6 | 4.65 | 4.6 | 0.7 | yes |
| dip between legato notes (dB) | 11.8 | 9 | 8.2 | 9.1 | 10.8 | 1.4 |  |
| level evenness of a legato scale (dB) | 3.2 | 1 | 1.3 | 1.1 | 2.6 | 2.0 | yes |
| level over the keyboard (dB/oct) | -1.16 | 0.75 | -0.93 | -1.64 | 0.59 | 0.8 |  |
| level against the other instruments (velocity 96) (dB) | 2.8 | 2.6 | 1.5 | -0.4 | 3.8 | 0.4 |  |
| key-to-key level scatter (dB) | 2.62 | 2.11 | 1.32 | 1.4 | 1.94 | 1.9 | yes |
| odd / even partials (dB) | -12.4 | -4.5 | -6 | -6.7 | -11.0 | 1.9 | yes |
| onset jitter in a phrase (ms) | 21.2 | 8.6 | 8.7 | 22.9 | 32.9 | 0.4 |  |
| onset in a phrase (re the note-on) (ms) | 22.0 | 2 | -2 | 0 | 0 | 2.2 | yes |
| onset sharpness (dB/ms) | 13.0 | 14.2 | 14.3 | 19.3 | 16.1 | 0.9 | yes |
| overtone levels, note for note (dB rms) | 16.2 (nearest 12) | fonts among themselves 15.2 | | | | 1.1 |  |
| panning across the keys (dB/oct) | 0.11 | 0 | -0 | -0 | 0.02 | 0.1 |  |
| partial envelopes alike | 0.76 | 0.933 | 1 | 0.998 | 0.913 | 2.1 | yes |
| release time (30 dB after note-off) (ms) | 504 | 15.0 | 81.0 | 65.0 | 91.0 | 2.2 | yes |
| spectrogram (1 - correlation), note for note | 0.393 (nearest 0.206) | fonts among themselves 0.22 | | | | 1.8 |  |
| spectral shape of the body, note for note | 15.6 (nearest 9.85) | fonts among themselves 11.9 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 78.0 | 105 | 123 | 64.0 | 21.0 | 0.1 |  |
| partial stretch (inharmonicity) (c/partial) | 0.55 | 0.75 | 0 | 0.01 | 0.54 | 0.6 |  |
| level after 2 s held, re peak (dB) | -18.6 | -19.9 | -24.9 | -26.9 | -27.1 | 2.0 |  |
| high bands lag low bands by (ms) | 1 | 0 | 0 | 0 | 0 | 0.2 |  |
| transient coherence: attack spread over bands (ms) | 1 | 0.7 | 0 | 0 | 0 | 0.2 |  |
| fundamental share (tristimulus 1) | 0.865 | 0.444 | 0.624 | 0.785 | 0.865 | 0.8 |  |
| partials 2-4 share (tristimulus 2) | 0.134 | 0.415 | 0.322 | 0.212 | 0.134 | 0.9 |  |
| partials 5+ share (tristimulus 3) | 0.001 | 0.03 | 0.062 | 0.022 | 0 | 0.4 |  |
| loudness range, velocity 40 to 127 (dB) | 17.8 | 19.7 | 16.6 | 16.6 | 20.1 | 0.2 |  |
| pitch wobble / vibrato (p-p) (c) | 6.7 | 1.6 | 0 | 1.4 | 1.7 | 1.0 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0147 (nearest 0.00631) | fonts among themselves 0.0107 | | | | 1.4 |  |

</details>

### 28 Clean Guitar: clean-guitar

Distance 1.88 (rank 109 of 129); the fonts disagree 1.1 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Brightness hardly rises up the neck (0.47 ERB/oct vs 3.2–4.8)**: probable fault. All four fonts agree. Either the low strings are too bright (modes to 8 kHz through the bridge and middle pickups) or the treble strings are too dull. The centroid at velocity 96 is inside the fonts' range (16.0 vs 12.8–18.4), so the problem is the tilt over the keyboard, not the overall level.
- **Soft attack: 20 ms vs 1–8 ms, darker than the body (−0.5 vs +1.1…+5.0 ERB)**: possible fault. The pick has no click on the three-pickup solid-body (the steel guitar has `PJ_STEEL_CLICK`), and the two-polarisation bloom delays the peak.
- **Decays faster (14.1 vs 8.4–11.8 dB/s)**: fault-ish. README "Electric guitars" says the string losses are "not yet fitted to recordings".
- **Brightens with velocity (+2.1 ERB vs 0)**: deliberate (pick physics). FluidR3's velocity layers change only the release (research/electric_guitars.md §2.7), and TiMidity drops filter modulators.
- **Release 508 ms vs 124–140 ms**: room artifact (the fretting hand mutes at once).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (378 ms | 140 | 124 | 132 | 130), outside the fonts, 6.1 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (1.34 ERB | -0.25 | -0 | -0 | 0.03), outside the fonts, 4.5 x.
3. **brightness over the keyboard**: brightens slower up the keyboard (0.17 ERB/oct | 4.84 | 4.82 | 3.2 | 4.12), outside the fonts, 5.2 x.
4. **attack time (-20 to -1 dB)**: a slower attack (19.5 ms | 6 | 2 | 1 | 7), outside the fonts, 1.8 x.
5. **decay rate of a held note**: decays faster (14.2 dB/s | 8.4 | 11.1 | 9.9 | 11.8), outside the fonts, 1.8 x.
6. **loudness range, velocity 40 to 127**: a narrower dynamic range (14.4 dB | 16.7 | 16.7 | 16.6 | 20.2), outside the fonts, 1.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +15 | -5 | -4 | +0 | -2 | +15 |
| re TimGM6mb | +6 | +12 | +10 | +15 | +5 | -33 |
| re Jnsgm2 | +4 | +2 | +3 | +0 | +1 | -12 |
| re SGM | -11 | -7 | -3 | +0 | +5 | +23 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.4 ERB, attack +0.1 ERB, velocity brightening +0.5 ERB, loudness range +3 dB, envelope 0.4 dB rms; TimGM6mb: centroid -4.5 ERB, attack +3.3 ERB, velocity brightening +0.2 ERB, loudness range +3 dB, envelope 2.7 dB rms; Jnsgm2: centroid -0.2 ERB, attack -0.4 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 0.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.6 | 1.07 | 4.99 | 2.42 | 3.56 | 1.8 | yes |
| attack time (-20 to -1 dB) (ms) | 19.5 | 6 | 2 | 1 | 7 | 1.8 | yes |
| spectral shape of the attack, note for note | 13.1 (nearest 10.8) | fonts among themselves 13.7 | | | | 1.0 |  |
| brightness over the keyboard (ERB/oct) | 0.17 | 4.84 | 4.82 | 3.2 | 4.12 | 5.2 | yes |
| key-to-key brightness scatter (ERB) | 2.21 | 2 | 0.533 | 1.53 | 1.14 | 1.2 | yes |
| brightening from velocity 40 to 127 (ERB) | 1.34 | -0.25 | -0 | -0 | 0.03 | 4.5 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 16.3 | 18.3 | 17.8 | 18.4 | 12.8 | 0.6 |  |
| decay rate of a held note (dB/s) | 14.2 | 8.4 | 11.1 | 9.9 | 11.8 | 1.8 | yes |
| onset delay (ms) | 10.0 | 9 | 0 | 10.0 | 19.0 | 0.1 |  |
| Plomp-Levelt dissonance of a note (log2) | -4.49 | -4.76 | -3.31 | -3.08 | -4.37 | 0.8 |  |
| envelope curve, note for note (dB rms) | 8.76 (nearest 7.5) | fonts among themselves 6.51 | | | | 1.3 |  |
| pitch re 12-TET A440 (median) (c) | 0.5 | -1.4 | 2.5 | 4.8 | 2.5 | 0.5 |  |
| pitch scatter from key to key (c) | 0.4 | 1.5 | 0.4 | 1 | 1 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -54.8 | -57.5 | -53.1 | -55.2 | -56.5 | 0.5 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -89.2 | -124 | -119 | -118 | -110 | 4.0 | yes |
| pitch at the start re settled (c) | -0.2 | 1.3 | 0.1 | 2.1 | 4.3 | 0.4 |  |
| stereo coherence | 0.952 | 1 | 1 | 1 | 1 | 0.6 | yes |
| partial-to-partial jaggedness (dB) | 6.05 | 4.3 | 4 | 2.9 | 2.4 | 2.6 | yes |
| dip between legato notes (dB) | 6.2 | 5.5 | 6.2 | 6.2 | 6.7 | 0.0 |  |
| level evenness of a legato scale (dB) | 1.3 | 1.4 | 0.9 | 0.6 | 1.1 | 0.3 |  |
| level over the keyboard (dB/oct) | 0.58 | -2.78 | -0.84 | 1.18 | -0.07 | 0.5 |  |
| level against the other instruments (velocity 96) (dB) | 0.6 | -0.4 | -6 | -3.9 | 0.5 | 0.8 |  |
| key-to-key level scatter (dB) | 1.42 | 1.38 | 0.29 | 0.75 | 1.28 | 0.7 |  |
| odd / even partials (dB) | -4 | 0.5 | 0.1 | 0.4 | -2.1 | 2.1 | yes |
| onset jitter in a phrase (ms) | 8.1 | 38.4 | 2.1 | 36.8 | 29.5 | 1.4 |  |
| onset in a phrase (re the note-on) (ms) | 5 | 5 | -4 | 9 | 34.0 | 0.1 |  |
| onset sharpness (dB/ms) | 44.0 | 19.1 | 2.7 | 15.8 | 10.2 | 3.8 | yes |
| overtone levels, note for note (dB rms) | 14.8 (nearest 12.8) | fonts among themselves 10.8 | | | | 1.4 |  |
| panning across the keys (dB/oct) | -0.04 | -0 | -0 | -0 | 0.02 | 0.0 |  |
| partial envelopes alike | 0.853 | 0.968 | 1 | 0.958 | 0.951 | 1.1 | yes |
| release time (30 dB after note-off) (ms) | 378 | 140 | 124 | 132 | 130 | 6.1 | yes |
| spectrogram (1 - correlation), note for note | 0.275 (nearest 0.218) | fonts among themselves 0.201 | | | | 1.4 |  |
| spectral shape of the body, note for note | 13.5 (nearest 13.1) | fonts among themselves 13.5 | | | | 1.0 |  |
| sounding length of a 100 ms note (ms) | 166 | 164 | 115 | 17.0 | 75.0 | 0.5 |  |
| partial stretch (inharmonicity) (c/partial) | 0.56 | 0.64 | 0 | 0.67 | 1.04 | 0.2 |  |
| level after 2 s held, re peak (dB) | -30.8 | -23.7 | -30.1 | -25.5 | -30.7 | 0.9 |  |
| high bands lag low bands by (ms) | 1.3 | -0.8 | 0 | -0.7 | 0 | 0.3 |  |
| transient coherence: attack spread over bands (ms) | 2 | 1 | 0 | 0.9 | 0.9 | 0.2 |  |
| fundamental share (tristimulus 1) | 0.149 | 0.017 | 0.114 | 0.051 | 0.35 | 0.4 |  |
| partials 2-4 share (tristimulus 2) | 0.422 | 0.825 | 0.443 | 0.66 | 0.549 | 1.0 |  |
| partials 5+ share (tristimulus 3) | 0.105 | 0.117 | 0.417 | 0.283 | 0.069 | 0.5 |  |
| loudness range, velocity 40 to 127 (dB) | 14.4 | 16.7 | 16.7 | 16.6 | 20.2 | 1.3 | yes |
| pitch wobble / vibrato (p-p) (c) | 4.5 | 1.3 | 0.1 | 3.6 | 3.8 | 0.4 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0148 (nearest 0.014) | fonts among themselves 0.0134 | | | | 1.1 |  |

</details>

### 29 Muted Guitar: muted-guitar

Distance 1.93 (rank 107 of 129); the fonts disagree 2.5 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Brighter (12.7 vs 7.8…11.2 ERB; +5…+23 dB at 1–6 kHz re three fonts)**: probable FAULT. The palm was fitted to FluidR3's decay times (README "Electric guitars", "The palm"; electric_guitars.md §7), but not to its tone. That note measured a centroid of 220–360 Hz below E4. Ours uses bridge+middle pickups with the pick ~6 cm from the saddle. Only TimGM6mb (11.2) comes near.
- **Rough (Plomp-Levelt −0.2 vs −3.4…−5.7)**: possibly the pick click over a dark note. Listen first.
- **Onset 62 vs 4…13 dB/ms, attack 1 ms**: DELIBERATE: the pick slips off in 0.15 ms.
- **Release 430 vs 58…69 ms**: room ARTIFACT. The palm-muted release itself is 25/s.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (13.1 ERB | 7.91 | 11.2 | 7.82 | 8.14), outside the fonts, 3.0 x.
2. **onset sharpness**: a sharper onset (65.0 dB/ms | 13.1 | 4 | 9.9 | 13.6), outside the fonts, 11.1 x.
3. **Plomp-Levelt dissonance of a note**: rougher (-0.27 log2 | -3.43 | -5.64 | -5.72 | -5.07), outside the fonts, 4.4 x.
4. **loudness range, velocity 40 to 127**: a narrower dynamic range (13.8 dB | 16.6 | 16.6 | 16.6 | 20.1), outside the fonts, 1.6 x.
5. **attack time (-20 to -1 dB)**: a faster attack (1 ms | 4 | 3 | 1.5 | 5), outside the fonts, 2.1 x.
6. **noisiness (spectral flatness)**: noisier (-26.4 dB | -33.3 | -69.3 | -45.0 | -64.4), outside the fonts, 1.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +2 | +9 | +8 | +8 | +11 | -27 |
| re TimGM6mb | +12 | +1 | +2 | +6 | +5 | -22 |
| re Jnsgm2 | -7 | -5 | -1 | +24 | +17 | -28 |
| re SGM | -10 | -15 | -10 | +1 | +19 | +25 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -4 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack +0.1 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 3.7 dB rms; TimGM6mb: centroid -0.0 ERB, attack +0.9 ERB, velocity brightening +0.0 ERB, loudness range +4 dB, envelope 2.2 dB rms; Jnsgm2: centroid +0.5 ERB, attack -0.5 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 6.7 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 1.52 | 2.3 | 1.03 | 3.35 | 4.14 | 0.8 |  |
| attack time (-20 to -1 dB) (ms) | 1 | 4 | 3 | 1.5 | 5 | 2.1 | yes |
| spectral shape of the attack, note for note | 9.75 (nearest 8.75) | fonts among themselves 10.6 | | | | 0.9 |  |
| brightness over the keyboard (ERB/oct) | 3.5 | 5.28 | 3.96 | 2.49 | 2.95 | 0.0 |  |
| key-to-key brightness scatter (ERB) | 1.54 | 1.31 | 0.971 | 0.483 | 1.25 | 1.1 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.09 | 0.07 | 0 | 0 | -0.1 | 0.3 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 13.1 | 7.91 | 11.2 | 7.82 | 8.14 | 3.0 | yes |
| decay rate of a held note (dB/s) | 33.3 | 38.5 | 11.0 | - | 7.9 | 1.4 |  |
| onset delay (ms) | 10.0 | 9 | 1 | 2 | 32.0 | 0.3 |  |
| Plomp-Levelt dissonance of a note (log2) | -0.27 | -3.43 | -5.64 | -5.72 | -5.07 | 4.4 | yes |
| envelope curve, note for note (dB rms) | 23.8 (nearest 14.6) | fonts among themselves 31.4 | | | | 0.8 |  |
| pitch re 12-TET A440 (median) (c) | 0.1 | -1 | 2.8 | -61.3 | -3.7 | 0.1 |  |
| pitch scatter from key to key (c) | 4.6 | 7.4 | 1 | 2 | 4.6 | 0.4 |  |
| noisiness (spectral flatness) (dB) | -26.4 | -33.3 | -69.3 | -45.0 | -64.4 | 1.6 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -130 | -125 | -123 | -123 | -110 | 0.9 | yes |
| pitch at the start re settled (c) | -1.9 | -0.7 | 4.2 | 69.0 | 4.5 | 0.2 |  |
| stereo coherence | 0.897 | 1 | 1 | 1 | 1 | 1.4 | yes |
| partial-to-partial jaggedness (dB) | 2.35 | 2.8 | 1.8 | 1.75 | 2.9 | 0.1 |  |
| dip between legato notes (dB) | 10.2 | 12.5 | 13.8 | 0 | 9.4 | 0.1 |  |
| level evenness of a legato scale (dB) | 1.2 | 0.4 | 0.7 | 0.2 | 2.3 | 0.6 |  |
| level over the keyboard (dB/oct) | -0.44 | -3.71 | -1.1 | -1.83 | 1.33 | 0.4 |  |
| level against the other instruments (velocity 96) (dB) | -4.2 | -7.8 | -2.6 | -9.1 | -3 | 0.4 |  |
| key-to-key level scatter (dB) | 1.63 | 1.24 | 1.01 | 0.43 | 2.08 | 0.6 |  |
| odd / even partials (dB) | -4.3 | -7.4 | -2.7 | -2.5 | -15.1 | 0.1 |  |
| onset jitter in a phrase (ms) | 1.4 | 4.4 | 32.4 | 5.5 | 15.7 | 0.7 | yes |
| onset in a phrase (re the note-on) (ms) | 5 | 2 | -3 | -2 | 11.0 | 0.5 |  |
| onset sharpness (dB/ms) | 65.0 | 13.1 | 4 | 9.9 | 13.6 | 11.1 | yes |
| overtone levels, note for note (dB rms) | 13.4 (nearest 11) | fonts among themselves 13.8 | | | | 1.0 |  |
| panning across the keys (dB/oct) | 0.06 | 0 | -0 | 0 | 0.02 | 0.1 |  |
| partial envelopes alike | 0.958 | 0.998 | 1 | 1 | 0.933 | 0.4 |  |
| spectrogram (1 - correlation), note for note | 0.465 (nearest 0.291) | fonts among themselves 0.545 | | | | 0.9 |  |
| spectral shape of the body, note for note | 13.4 (nearest 12.2) | fonts among themselves 11.7 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 26.0 | 25.0 | 81.0 | 5 | 124 | 0.3 |  |
| partial stretch (inharmonicity) (c/partial) | 0.15 | -0.33 | 0 | -1 | 0.28 | 0.5 |  |
| level after 2 s held, re peak (dB) | -88.1 | -120 | -29.4 | -123 | -29.0 | 0.3 |  |
| high bands lag low bands by (ms) | 0 | -2 | 0 | 0 | -0.7 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 0.1 | 1.3 | 0 | 0 | 0.9 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.522 | 0.438 | 0.5 | 0.712 | 0.837 | 0.4 |  |
| partials 2-4 share (tristimulus 2) | 0.407 | 0.537 | 0.479 | 0.286 | 0.163 | 0.1 |  |
| partials 5+ share (tristimulus 3) | 0.035 | 0.005 | 0.009 | 0.002 | 0 | 0.4 |  |
| loudness range, velocity 40 to 127 (dB) | 13.8 | 16.6 | 16.6 | 16.6 | 20.1 | 1.6 | yes |
| pitch wobble / vibrato (p-p) (c) | 24.5 | 76.9 | 0 | 61.0 | 3.7 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0115 (nearest 0.00908) | fonts among themselves 0.0088 | | | | 1.3 |  |

</details>

### 30 Overdriven Guitar: overdriven-guitar

Distance 1.64 (rank 117 of 129); the fonts disagree 1.1 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **A 100 ms staccato sounds 400 ms (the cap) vs 116–155; release 732 ms vs 30–122**: suspect. The room alone gives the clean guitar 170 ms. Here the compressor pedal (0.2 s release) and the crunch probably hold up the muted string's tail. Check how fast the fretting-hand mute falls through the drive.
- **Narrow velocity range (12.3 vs 16.6–20.1 dB; +0 dB from 96 to 127)**: deliberate. At crunch the pickup's 17 dB come out as 6 (README "The British stack"). The fonts' 16.6 dB is TiMidity's velocity law.
- **Slow swell (attack 256 ms; the fonts range from 32 to 193)**: deliberate. The compressor and the feedback hold make the note grow (README "Feedback").
- **Quieter (−2.9 vs −0.7…+2 dB)**, more low end (<150 Hz +1…+17 dB), stereo coherence 0.47: the stereo is the room (artifact); the rest is minor.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **sounding length of a 100 ms note [room]**: longer staccato (353 ms | 116 | 126 | 145 | 155), outside the fonts, 5.5 x.
2. **loudness range, velocity 40 to 127**: a narrower dynamic range (12.8 dB | 16.6 | 16.6 | 16.6 | 20.1), outside the fonts, 2.2 x.
3. **release time (30 dB after note-off) [room]**: rings longer after the key (720 ms | 30.0 | 68.0 | 99.0 | 125), outside the fonts, 3.0 x.
4. **key-to-key level scatter**: less even from key to key (2.3 dB | 1.05 | 0.65 | 1.24 | 1.12), outside the fonts, 2.4 x.
5. **stereo coherence [room]**: wider (0.47 | 1 | 1 | 1 | 1), outside the fonts, 7.1 x.
6. **attack time (-20 to -1 dB)**: a slower attack (264 ms | 193 | 32.0 | 36.0 | 45.0), outside the fonts, 2.1 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +9 | +0 | +3 | +5 | -4 | -10 |
| re TimGM6mb | +16 | +3 | -2 | +5 | -2 | -12 |
| re Jnsgm2 | +6 | -4 | -6 | +7 | -2 | -2 |
| re SGM | +1 | -3 | +1 | +5 | -1 | -6 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -4 +0 +0; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.4 ERB, attack +0.1 ERB, velocity brightening +0.6 ERB, loudness range +4 dB, envelope 1.8 dB rms; TimGM6mb: centroid -0.6 ERB, attack -0.4 ERB, velocity brightening +0.5 ERB, loudness range +4 dB, envelope 2.2 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.1 ERB, velocity brightening +0.6 ERB, loudness range +4 dB, envelope 2.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.98 | -2.49 | 0.31 | 2.54 | -0.61 | 0.3 |  |
| attack time (-20 to -1 dB) (ms) | 264 | 193 | 32.0 | 36.0 | 45.0 | 2.1 | yes |
| spectral shape of the attack, note for note | 10 (nearest 8.83) | fonts among themselves 8.52 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 2 | 3.36 | 1.55 | 1.42 | 3.15 | 0.4 |  |
| key-to-key brightness scatter (ERB) | 1.23 | 1.55 | 1.16 | 1.27 | 2.06 | 0.4 |  |
| brightening from velocity 40 to 127 (ERB) | -0.36 | -0.09 | -0 | 0 | 0.13 | 1.2 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 17.5 | 20.2 | 16.1 | 13.2 | 15.9 | 0.4 |  |
| onset delay (ms) | 16.0 | 29.0 | 11.0 | 7 | 21.0 | 0.0 |  |
| Plomp-Levelt dissonance of a note (log2) | -3.61 | -2.56 | -2.89 | -3.35 | -2.99 | 1.7 | yes |
| envelope curve, note for note (dB rms) | 17.6 (nearest 17.2) | fonts among themselves 5.72 | | | | 3.1 | yes |
| pitch re 12-TET A440 (median) (c) | 1.1 | -0.7 | 2.1 | 5.7 | -0.2 | 0.0 |  |
| pitch scatter from key to key (c) | 1 | 0.7 | 0.6 | 1.3 | 2.7 | 0.0 |  |
| noisiness (spectral flatness) (dB) | -46.2 | -55.0 | -64.0 | -35.0 | -38.6 | 0.0 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -72.4 | -124 | -119 | -119 | -110 | 6.3 | yes |
| pitch at the start re settled (c) | -0.1 | 0.4 | -1.2 | 4.5 | 2.6 | 0.3 |  |
| stereo coherence | 0.47 | 1 | 1 | 1 | 1 | 7.1 | yes |
| partial-to-partial jaggedness (dB) | 3.6 | 2.2 | 3.6 | 4.1 | 3 | 0.3 |  |
| dip between legato notes (dB) | 6.8 | 4.9 | 3.1 | 5.6 | 5.4 | 0.8 | yes |
| level evenness of a legato scale (dB) | 1.1 | 1.3 | 1 | 1.3 | 1.4 | 0.2 |  |
| level over the keyboard (dB/oct) | 1.81 | 0.08 | 1.13 | 0.4 | 0.64 | 1.3 | yes |
| level against the other instruments (velocity 96) (dB) | 0.1 | 1.8 | -1.3 | -0.6 | 3.2 | 0.2 |  |
| key-to-key level scatter (dB) | 2.3 | 1.05 | 0.65 | 1.24 | 1.12 | 2.4 | yes |
| odd / even partials (dB) | -0.1 | -0.9 | 0.6 | 1 | -0.9 | 0.0 |  |
| onset jitter in a phrase (ms) | 42.9 | 40.3 | 19.2 | 11.5 | 13.8 | 1.8 | yes |
| onset in a phrase (re the note-on) (ms) | 7 | -2 | -2 | 0 | 0 | 0.8 | yes |
| onset sharpness (dB/ms) | 36.8 | 8.3 | 4.7 | 8.5 | 23.8 | 3.0 | yes |
| overtone levels, note for note (dB rms) | 9.77 (nearest 8.68) | fonts among themselves 8.74 | | | | 1.1 |  |
| panning across the keys (dB/oct) | 0.32 | -0 | -0 | -0 | 0.02 | 0.3 |  |
| partial envelopes alike | 0.148 | 0.027 | 0.036 | 0.361 | -0.097 | 0.5 |  |
| release time (30 dB after note-off) (ms) | 720 | 30.0 | 68.0 | 99.0 | 125 | 3.0 | yes |
| spectrogram (1 - correlation), note for note | 0.339 (nearest 0.308) | fonts among themselves 0.121 | | | | 2.8 | yes |
| spectral shape of the body, note for note | 10.6 (nearest 9.64) | fonts among themselves 8.6 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 353 | 116 | 126 | 145 | 155 | 5.5 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.14 | 0.04 | 0 | 0.08 | 0.02 | 0.2 |  |
| level after 2 s held, re peak (dB) | -10.8 | -6.8 | -3.4 | -11.9 | -6.3 | 1.0 |  |
| high bands lag low bands by (ms) | 9.4 | 9.5 | -1.8 | 10.0 | 3.2 | 0.5 |  |
| transient coherence: attack spread over bands (ms) | 10.6 | 7.9 | 5.1 | 9.4 | 10.0 | 0.4 |  |
| fundamental share (tristimulus 1) | 0.041 | 0.03 | 0.066 | 0.289 | 0.309 | 1.0 |  |
| partials 2-4 share (tristimulus 2) | 0.393 | 0.372 | 0.477 | 0.488 | 0.392 | 0.6 |  |
| partials 5+ share (tristimulus 3) | 0.499 | 0.604 | 0.384 | 0.085 | 0.221 | 0.8 |  |
| loudness range, velocity 40 to 127 (dB) | 12.8 | 16.6 | 16.6 | 16.6 | 20.1 | 2.2 | yes |
| pitch wobble / vibrato (p-p) (c) | 4.8 | 3.7 | 0 | 3.4 | 7.6 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0178 (nearest 0.0142) | fonts among themselves 0.00937 | | | | 1.9 | yes |

</details>

### 31 Distortion Guitar: distortion-guitar

Distance 4.15 (rank 16 of 129); the fonts disagree 1.2 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Speaks late: onset 92 ms vs 1–13; 58 ms in a phrase vs −2…22**: likely fault, the most audible one here. Possible causes are the gate opening, the grid-current bias slide blocking the attack, or the GM-31 player's look-ahead. The high bands also lag the low ones by 17.5 ms (fonts 1–10).
- **Spectrum differs from the earlier fit: −8 to −15 dB at 400 Hz–1 kHz and +6 to +21 dB above 6 kHz against every font**: research/distortion_guitar.md §11 check 1 asks for ±4 dB of FluidR3. This average over all the probe keys does not show that, so check whether the >6 kHz excess is hiss, aliasing or the room.
- **A 100 ms note sounds 400 ms (vs 126–145)**: suspect. Gate check 7 says −60 dB within 200 ms; see p029.
- **Velocity range 4.2 dB (vs 16.6) and brightening +4.6 ERB**: deliberate (distortion_guitar.md §11 check 3; README "The high-gain amp").
- **Wobble 26 c**: deliberate, the rock player's wide vibrato from E3 up.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (3.4 dB | 16.6 | 16.7 | 16.6 | 20.1), outside the fonts, 7.6 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (4.5 ERB | 0.23 | 0 | 0 | 0.13), outside the fonts, 14.8 x.
3. **onset delay**: speaks later (106 ms | 12.0 | 1 | 7 | 18.0), outside the fonts, 9.7 x.
4. **release time (30 dB after note-off) [room]**: rings longer after the key (719 ms | 48.0 | 56.0 | 99.0 | 95.0), outside the fonts, 6.3 x.
5. **sounding length of a 100 ms note [room]**: longer staccato (400 ms | 126 | 127 | 145 | 124), outside the fonts, 6.6 x.
6. **onset sharpness**: a sharper onset (29.2 dB/ms | 6.5 | 4.1 | 8.5 | 10.3), outside the fonts, 7.0 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -1 | +0 | -7 | -5 | +4 | +10 |
| re TimGM6mb | +6 | -1 | -8 | -6 | +6 | +5 |
| re Jnsgm2 | +2 | -4 | -14 | -4 | +6 | +18 |
| re SGM | -8 | -7 | -8 | -2 | +3 | +20 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -3 -2 +0 +0; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.8 ERB, attack -0.5 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 2.1 dB rms; TimGM6mb: centroid -0.2 ERB, attack -0.3 ERB, velocity brightening +0.3 ERB, loudness range +3 dB, envelope 2.0 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.1 ERB, velocity brightening +0.6 ERB, loudness range +4 dB, envelope 2.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.74 | -0.88 | -0.05 | 2.54 | -2.05 | 0.1 |  |
| attack time (-20 to -1 dB) (ms) | 334 | 230 | 20.0 | 36.0 | 840 | 0.7 |  |
| spectral shape of the attack, note for note | 10.9 (nearest 9.14) | fonts among themselves 9.24 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 2.47 | 2.94 | 3.03 | 1.42 | 4.85 | 0.3 |  |
| key-to-key brightness scatter (ERB) | 2.41 | 1.11 | 0.993 | 1.27 | 2.16 | 2.1 | yes |
| brightening from velocity 40 to 127 (ERB) | 4.5 | 0.23 | 0 | 0 | 0.13 | 14.8 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 21.0 | 21.4 | 16.0 | 13.2 | 14.9 | 1.4 |  |
| onset delay (ms) | 106 | 12.0 | 1 | 7 | 18.0 | 9.7 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.38 | -3.06 | -3.47 | -3.35 | -3.4 | 3.3 | yes |
| envelope curve, note for note (dB rms) | 18.7 (nearest 18) | fonts among themselves 5.6 | | | | 3.3 | yes |
| pitch re 12-TET A440 (median) (c) | 1.2 | 0.6 | 2.6 | 5.7 | -4.7 | 0.1 |  |
| pitch scatter from key to key (c) | 1.3 | 2.1 | 0.7 | 1.3 | 5.5 | 0.1 |  |
| noisiness (spectral flatness) (dB) | -34.0 | -52.0 | -65.1 | -35.0 | -40.2 | 0.8 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -69.9 | -129 | -120 | -119 | -115 | 7.1 | yes |
| pitch at the start re settled (c) | 0.1 | 1.9 | -0.1 | 4.5 | 2.4 | 0.4 |  |
| stereo coherence | 0.312 | 1 | 1 | 1 | 1 | 9.2 | yes |
| partial-to-partial jaggedness (dB) | 4.25 | 3.2 | 3.5 | 4.1 | 3 | 0.9 |  |
| dip between legato notes (dB) | 6.2 | 5.5 | 4.3 | 5.6 | 7.3 | 0.3 |  |
| level evenness of a legato scale (dB) | 2.1 | 1.8 | 1.5 | 1.3 | 2.2 | 0.5 |  |
| level over the keyboard (dB/oct) | 1.48 | 0.75 | 0.99 | 0.4 | 0.34 | 0.9 |  |
| level against the other instruments (velocity 96) (dB) | 0.2 | 6.2 | 0.7 | -0.6 | 4.8 | 0.8 |  |
| key-to-key level scatter (dB) | 1.41 | 1.27 | 1.11 | 1.24 | 1.39 | 0.3 |  |
| odd / even partials (dB) | -4.2 | -1.2 | 3.1 | 1 | -1.2 | 1.9 | yes |
| onset jitter in a phrase (ms) | 47.9 | 57.5 | 11.3 | 11.5 | 14.8 | 1.5 |  |
| onset in a phrase (re the note-on) (ms) | 16.0 | 22.0 | 7 | 0 | -2 | 1.0 |  |
| onset sharpness (dB/ms) | 29.2 | 6.5 | 4.1 | 8.5 | 10.3 | 7.0 | yes |
| overtone levels, note for note (dB rms) | 12 (nearest 8.52) | fonts among themselves 10.7 | | | | 1.1 |  |
| panning across the keys (dB/oct) | 0.16 | -0 | -0 | -0 | 0.02 | 0.2 |  |
| partial envelopes alike | -0.014 | -0.068 | -0.099 | 0.361 | -0.049 | 0.2 |  |
| release time (30 dB after note-off) (ms) | 719 | 48.0 | 56.0 | 99.0 | 95.0 | 6.3 | yes |
| spectrogram (1 - correlation), note for note | 0.371 (nearest 0.308) | fonts among themselves 0.176 | | | | 2.1 | yes |
| spectral shape of the body, note for note | 10.4 (nearest 9.78) | fonts among themselves 9.68 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 400 | 126 | 127 | 145 | 124 | 6.6 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.03 | 0.09 | 0 | 0.08 | 0.04 | 0.1 |  |
| level after 2 s held, re peak (dB) | -8.9 | -8.6 | -2.2 | -11.9 | -4.8 | 0.5 |  |
| high bands lag low bands by (ms) | 15.5 | 3.4 | 1.3 | 10.0 | 6.3 | 2.1 | yes |
| transient coherence: attack spread over bands (ms) | 22.2 | 2.6 | 5.1 | 9.4 | 5 | 3.4 | yes |
| fundamental share (tristimulus 1) | 0.221 | 0.094 | 0.127 | 0.289 | 0.631 | 0.0 |  |
| partials 2-4 share (tristimulus 2) | 0.444 | 0.453 | 0.444 | 0.488 | 0.177 | 0.0 |  |
| partials 5+ share (tristimulus 3) | 0.346 | 0.447 | 0.341 | 0.085 | 0.156 | 0.5 |  |
| loudness range, velocity 40 to 127 (dB) | 3.4 | 16.6 | 16.7 | 16.6 | 20.1 | 7.6 | yes |
| pitch wobble / vibrato (p-p) (c) | 25.3 | 2.3 | 0.1 | 3.4 | 4.2 | 4.5 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0203 (nearest 0.02) | fonts among themselves 0.011 | | | | 1.8 | yes |

</details>

### 32 Guitar Harmonics: guitar-harmonics

Distance 4.13 (rank 17 of 129); the fonts disagree 1.6 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Darker, more fundamental (12.5 vs 17.1–19.0 ERB; tristimulus-1 0.60 vs 0)**: measurement artifact. Every font sounds two octaves above the key, so the measures don't find the fonts' fundamental at the key's pitch. Ours plays at the written pitch (README "Guitar harmonics"; electric_guitars.md §9.5). The same artifact explains the odd/even, partial-stretch and tristimulus rows.
- **Uneven from key to key (2.8 dB vs 0.2–0.8)**: a known residue. The player's evening fit leaves 1.5 dB rms and ±4 dB at worst (README). The fonts are flatter, and FluidR3 stays within ±0.8 dB.
- **Instant attack (2 ms vs 20–33)**: deliberate (picked). The fonts' samples fade in.
- **−34 dB after 2 s (vs −1…−14)**: deliberate. The string decays on its own, while FluidR3's samples are cut at 0.2–1.5 s and looped.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **attack time (-20 to -1 dB)**: a faster attack (2 ms | 30.0 | 27.0 | 33.0 | 33.0), outside the fonts, 15.9 x.
2. **brightness (spectral centroid, velocity 96)**: darker (12.5 ERB | 18.3 | 19.0 | 18.4 | 17.1), outside the fonts, 6.0 x.
3. **fundamental share (tristimulus 1)**: more fundamental (0.597 | 0 | 0 | 0 | 0), outside the fonts, 8.5 x.
4. **partials 2-4 share (tristimulus 2)**: weaker low partials (0.138 | 0.822 | 0.866 | 0.852 | 0.975), outside the fonts, 9.4 x.
5. **key-to-key level scatter**: less even from key to key (2.24 dB | 0.19 | 0.78 | 0.23 | 0.66), outside the fonts, 3.6 x.
6. **partial stretch (inharmonicity)**: more stretched partials (2.5 c/partial | 0 | -0.22 | 0 | -0.89), outside the fonts, 5.2 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +27 | +39 | +12 | +9 | -2 | -40 |
| re TimGM6mb | +1 | +23 | +8 | +6 | -1 | -35 |
| re Jnsgm2 | -11 | +6 | +8 | +7 | +2 | -23 |
| re SGM | +5 | +20 | +2 | +6 | -1 | -33 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack -0.0 ERB, velocity brightening +0.6 ERB, loudness range +4 dB, envelope 1.9 dB rms; TimGM6mb: centroid -0.1 ERB, attack -0.2 ERB, velocity brightening +0.4 ERB, loudness range +4 dB, envelope 2.0 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 1.9 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.15 | 0.15 | -3.35 | -0.07 | -4.63 | 0.8 |  |
| attack time (-20 to -1 dB) (ms) | 2 | 30.0 | 27.0 | 33.0 | 33.0 | 15.9 | yes |
| spectral shape of the attack, note for note | 11.7 (nearest 10.1) | fonts among themselves 9.72 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 1.98 | 3.73 | 3.57 | 5.17 | 5.26 | 2.9 | yes |
| key-to-key brightness scatter (ERB) | 3.02 | 1.08 | 0.976 | 2.2 | 0.426 | 2.2 | yes |
| brightening from velocity 40 to 127 (ERB) | -0.11 | -0 | -0 | -0 | 0.04 | 0.4 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 12.5 | 18.3 | 19.0 | 18.4 | 17.1 | 6.0 | yes |
| onset delay (ms) | 15.0 | 18.0 | 19.0 | 2 | 6 | 0.3 |  |
| Plomp-Levelt dissonance of a note (log2) | -5.51 | -4.97 | -1.92 | -2.75 | -4.29 | 1.3 | yes |
| envelope curve, note for note (dB rms) | 13.9 (nearest 13.7) | fonts among themselves 7.24 | | | | 1.9 | yes |
| pitch re 12-TET A440 (median) (c) | 0.6 | -3.4 | -2.5 | -6.2 | -0.2 | 0.9 |  |
| pitch scatter from key to key (c) | 1.2 | 1.9 | 2.7 | 9 | 0.1 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -58.1 | -58.7 | -55.2 | -49.8 | -64.0 | 0.2 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -89.9 | -123 | -120 | -122 | -109 | 4.5 | yes |
| pitch at the start re settled (c) | -0.7 | -0.1 | 0 | 16.1 | 0.1 | 0.1 |  |
| stereo coherence | 0.986 | 1 | 1 | 1 | 1 | 0.2 |  |
| partial-to-partial jaggedness (dB) | 8.85 | 17.0 | 9.7 | 17.4 | 7.7 | 0.9 |  |
| dip between legato notes (dB) | 7 | 8 | 12.3 | 4.7 | 8.4 | 0.3 |  |
| level evenness of a legato scale (dB) | 1.2 | 0.3 | 0.5 | 0.6 | 1 | 0.6 |  |
| level over the keyboard (dB/oct) | 0.52 | 0.51 | 1.54 | 0.39 | -1.67 | 0.0 |  |
| level against the other instruments (velocity 96) (dB) | 4.6 | 4.2 | -0.9 | 0.1 | -0.2 | 1.8 |  |
| key-to-key level scatter (dB) | 2.24 | 0.19 | 0.78 | 0.23 | 0.66 | 3.6 | yes |
| odd / even partials (dB) | 7.8 | -47.6 | -15.5 | -34.5 | -25.9 | 2.4 | yes |
| onset jitter in a phrase (ms) | 54.1 | 16.4 | 27.3 | 24.2 | 14.5 | 5.3 | yes |
| onset in a phrase (re the note-on) (ms) | 11.0 | 23.0 | 44.0 | 1 | -0 | 0.0 |  |
| onset sharpness (dB/ms) | 40.9 | 4.3 | 5 | 4 | 48.8 | 1.6 |  |
| overtone levels, note for note (dB rms) | 29.7 (nearest 25) | fonts among themselves 25.1 | | | | 1.2 |  |
| panning across the keys (dB/oct) | 0.07 | -0 | -0 | -0 | 0.02 | 0.1 |  |
| partial envelopes alike | 0.921 | 0.403 | 0.83 | 0.752 | 0.773 | 0.7 | yes |
| release time (30 dB after note-off) (ms) | 513 | 6 | 70.0 | 102 | 76.0 | 1.4 | yes |
| spectrogram (1 - correlation), note for note | 0.647 (nearest 0.624) | fonts among themselves 0.39 | | | | 1.7 | yes |
| spectral shape of the body, note for note | 18.8 (nearest 16.9) | fonts among themselves 14.9 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 400 | 104 | 58.0 | 143 | 138 | 2.7 | yes |
| partial stretch (inharmonicity) (c/partial) | 2.5 | 0 | -0.22 | 0 | -0.89 | 5.2 | yes |
| level after 2 s held, re peak (dB) | -33.7 | -0.9 | -14.3 | -14.2 | -11.5 | 3.1 | yes |
| high bands lag low bands by (ms) | 0 | 7 | 24.0 | 1 | 10.7 | 0.8 |  |
| transient coherence: attack spread over bands (ms) | 0.9 | 6.1 | 8.5 | 2.5 | 8.3 | 1.3 |  |
| fundamental share (tristimulus 1) | 0.597 | 0 | 0 | 0 | 0 | 8.5 | yes |
| partials 2-4 share (tristimulus 2) | 0.138 | 0.822 | 0.866 | 0.852 | 0.975 | 9.4 | yes |
| partials 5+ share (tristimulus 3) | 0.139 | 0.178 | 0.134 | 0.148 | 0.025 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 16.2 | 16.6 | 16.6 | 16.6 | 20.1 | 0.2 |  |
| pitch wobble / vibrato (p-p) (c) | 1.5 | 0.5 | 0 | 7.4 | 0 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0284 (nearest 0.0281) | fonts among themselves 0.0233 | | | | 1.2 |  |

</details>

### 33 Acoustic Bass: double-bass

Distance 2.21 (rank 95 of 129); the fonts disagree 1.9 JND. Keys 28,31,34,37,40,43,46,49,52,55,58, velocities 40/64/96/127.

- **Speaks late: onset 47 ms vs 15–19; 70 ms in a phrase vs 0–34**: fault worth fixing. Double-bass has no noteLead, so a walking line drags behind the beat. A lead of about 40 ms, as the slap and fretless basses have, would fix it.
- **Uneven from key to key (3.9 dB vs 0.03–1.4)**: deliberate in kind (notes near A0/T1 loud and short: research/double_bass.md §2), but larger than any font. Unlike the contrabass, nothing evens the plucked notes.
- **Darker above 1 kHz (−5 to −10 dB)**: consistent with the gut, the old-record targets (double_bass.md §7d).
- **Narrow velocity range (13 vs 16.6 dB)**: TiMidity convention. **Purer (−82 vs −60…−68 dB flatness)**: the samples' noise, an artifact. The floor and the release come from the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (3.23 dB | 0.52 | 0.03 | 0.79 | 1.43), outside the fonts, 3.7 x.
2. **onset in a phrase (re the note-on)**: speaks later in a phrase (95.0 ms | -0 | 34.0 | 6 | 2), outside the fonts, 5.4 x.
3. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-69.7 dB | -128 | -127 | -126 | -116), outside the fonts, 9.5 x.
4. **onset delay**: speaks later (47.0 ms | 16.0 | 15.0 | 16.0 | 17.0), outside the fonts, 3.1 x.
5. **noisiness (spectral flatness)**: purer (-82.3 dB | -59.6 | -62.8 | -65.3 | -68.3), outside the fonts, 4.2 x.
6. **brightening from velocity 40 to 127**: brightens more with velocity (0.59 ERB | 0 | 0 | 0 | -0.38), outside the fonts, 2.0 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -5 | -4 | +4 | +2 | - | - |
| re TimGM6mb | +1 | -2 | -2 | +3 | - | - |
| re Jnsgm2 | -0 | +1 | +2 | -2 | -2 | - |
| re SGM | -6 | -6 | -1 | +7 | - | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -4 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack +0.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 2.4 dB rms; TimGM6mb: centroid +0.0 ERB, attack -0.1 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 6.8 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 2.4 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 1.16 | -0.06 | -1.9 | 0.03 | -0.17 | 1.3 | yes |
| attack time (-20 to -1 dB) (ms) | 40.5 | 38.0 | 15.0 | 13.0 | 20.0 | 1.6 |  |
| spectral shape of the attack, note for note | 9.5 (nearest 9.11) | fonts among themselves 6.25 | | | | 1.5 | yes |
| brightness over the keyboard (ERB/oct) | 5.13 | 4.55 | 4.64 | 7.39 | 4.02 | 0.3 |  |
| key-to-key brightness scatter (ERB) | 1.01 | 0.268 | 0.177 | 0.501 | 0.893 | 1.7 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.59 | 0 | 0 | 0 | -0.38 | 2.0 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 4.68 | 3.37 | 5.03 | 3.49 | 4.14 | 1.0 |  |
| decay rate of a held note (dB/s) | 11.9 | 10.5 | 3.6 | 10.6 | 4.6 | 1.0 | yes |
| onset delay (ms) | 47.0 | 16.0 | 15.0 | 16.0 | 17.0 | 3.1 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.95 | -5.27 | -3.68 | -4.72 | -4.72 | 1.0 |  |
| envelope curve, note for note (dB rms) | 9.68 (nearest 8.9) | fonts among themselves 7 | | | | 1.4 |  |
| pitch re 12-TET A440 (median) (c) | 1.4 | -0.3 | 2.6 | 0.7 | -0.3 | 0.3 |  |
| pitch scatter from key to key (c) | 5 | 0.8 | 0.4 | 1.3 | 2.1 | 1.3 | yes |
| noisiness (spectral flatness) (dB) | -82.3 | -59.6 | -62.8 | -65.3 | -68.3 | 4.2 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -69.7 | -128 | -127 | -126 | -116 | 9.5 | yes |
| pitch at the start re settled (c) | -0.3 | -0.9 | 0 | 16.2 | -9.4 | 0.0 |  |
| stereo coherence | 0.8 | 1 | 1 | 1 | 1 | 2.7 | yes |
| partial-to-partial jaggedness (dB) | 5.7 | 3.9 | 2.9 | 4.9 | 3.5 | 2.0 | yes |
| dip between legato notes (dB) | 19.6 | 14.3 | 12.2 | 19.7 | 11.1 | 1.5 |  |
| level evenness of a legato scale (dB) | 1 | 0.8 | 0.8 | 1 | 3.6 | 0.1 |  |
| level over the keyboard (dB/oct) | -5.07 | -0.31 | -0.88 | -3.23 | -7.42 | 0.8 |  |
| level against the other instruments (velocity 96) (dB) | 1.8 | 0.9 | 0.4 | 2.6 | 4.8 | 0.0 |  |
| key-to-key level scatter (dB) | 3.23 | 0.52 | 0.03 | 0.79 | 1.43 | 3.7 | yes |
| odd / even partials (dB) | -11.3 | -9.3 | -10.3 | -3.2 | -5.1 | 1.2 |  |
| onset jitter in a phrase (ms) | 45.9 | 35.2 | 42.2 | 41.0 | 22.1 | 0.8 | yes |
| onset in a phrase (re the note-on) (ms) | 95.0 | -0 | 34.0 | 6 | 2 | 5.4 | yes |
| onset sharpness (dB/ms) | 7.7 | 20.3 | 8.6 | 8.5 | 10.7 | 0.3 |  |
| overtone levels, note for note (dB rms) | 13.4 (nearest 12.3) | fonts among themselves 8.45 | | | | 1.6 | yes |
| panning across the keys (dB/oct) | -0.86 | 0 | 0.13 | 0 | -0.04 | 0.9 | yes |
| partial envelopes alike | 0.946 | 0.929 | 0.716 | 0.919 | 0.643 | 0.9 |  |
| release time (30 dB after note-off) (ms) | 194 | 24.0 | 61.0 | 83.0 | 63.0 | 1.8 | yes |
| spectrogram (1 - correlation), note for note | 0.287 (nearest 0.212) | fonts among themselves 0.096 | | | | 3.0 | yes |
| spectral shape of the body, note for note | 11.2 (nearest 10.2) | fonts among themselves 8.13 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 166 | 105 | 52.0 | 44.0 | 117 | 1.7 | yes |
| partial stretch (inharmonicity) (c/partial) | 1.86 | 0.02 | 0.02 | 0.22 | 0.38 | 3.5 | yes |
| level after 2 s held, re peak (dB) | -33.0 | -34.3 | -22.3 | -27.2 | -23.0 | 1.3 |  |
| high bands lag low bands by (ms) | -11.2 | -7 | -4 | -0.5 | -6 | 1.2 | yes |
| transient coherence: attack spread over bands (ms) | 2.8 | 1.1 | 1.4 | 0.9 | 1.7 | 0.3 |  |
| fundamental share (tristimulus 1) | 0.85 | 0.891 | 0.508 | 0.76 | 0.909 | 0.1 |  |
| partials 2-4 share (tristimulus 2) | 0.141 | 0.098 | 0.439 | 0.16 | 0.081 | 0.1 |  |
| partials 5+ share (tristimulus 3) | 0.003 | 0.004 | 0.052 | 0.008 | 0.011 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 14.1 | 16.6 | 16.6 | 16.7 | 20.1 | 1.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 21.8 | 13.6 | 33.6 | 10.7 | 9.1 | 0.8 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.00804 (nearest 0.0072) | fonts among themselves 0.0086 | | | | 0.9 |  |

</details>

### 34 Finger Bass: electric-bass

Distance 1.55 (rank 122 of 129); the fonts disagree 1.3 JND. Keys 28,31,34,37,40,43,46,49,52,55,58, velocities 40/64/96/127.

- **Louder toward the top (+2.4 dB/oct vs −0.3…+0.9)**: probable fault, shared with every electric bass. The pickup hears the string's velocity, and the bassist does not even out the line.
- **Quieter against the others (−3.7 vs +0.8…+1.4 dB)**: worth retrimming in the level table.
- **Brightens with velocity (+1.3 ERB vs 0)**: deliberate (README "Electric bass": centroid 141/154/173 Hz at v40/80/120, "FluidR3's does not move"). Part of the fonts' 0 is TiMidity ignoring the filters.
- **Narrow range (12.6 dB; +1 dB from 96 to 127)**: deliberate. The optical compressor narrows it, and README says "-play's velocity gain makes up the difference", but the top step is still flat.
- **Purer**: sample-noise artifact. The spectrum agrees with FluidR3 within ±2 dB.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (1.29 ERB | 0 | 0 | 0 | -0.38), outside the fonts, 4.3 x.
2. **loudness range, velocity 40 to 127**: a narrower dynamic range (12.3 dB | 16.6 | 16.6 | 16.7 | 20.1), outside the fonts, 2.5 x.
3. **noisiness (spectral flatness)**: purer (-85.5 dB | -75.7 | -62.4 | -62.8 | -64.2), outside the fonts, 3.3 x.
4. **key-to-key brightness scatter**: less even in tone from key to key (0.938 ERB | 0.686 | 0.102 | 0.279 | 0.585), outside the fonts, 1.7 x.
5. **level against the other instruments (velocity 96)**: louder (2.8 dB | 1.8 | 1.1 | 0.9 | 2.6), inside the fonts' range, 1.3 x.
6. **brightness over the keyboard**: brightens slower up the keyboard (-0.59 ERB/oct | 1.7 | 4.96 | 0.74 | 5.34), outside the fonts, 1.7 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -0 | +2 | +1 | -2 | - | - |
| re TimGM6mb | +18 | +20 | +10 | +2 | -22 | -27 |
| re Jnsgm2 | -6 | -2 | -2 | +6 | - | - |
| re SGM | -20 | -5 | +8 | +5 | - | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -4 +0 +1; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 1.9 dB rms; TimGM6mb: centroid -0.0 ERB, attack +0.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 5.5 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 2.4 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.02 | 0.14 | -0.08 | 0.3 | -0.19 | 0.1 |  |
| attack time (-20 to -1 dB) (ms) | 11.0 | 11.0 | 9 | 13.0 | 13.0 | 0.5 |  |
| spectral shape of the attack, note for note | 7.98 (nearest 7.55) | fonts among themselves 8.92 | | | | 0.9 |  |
| brightness over the keyboard (ERB/oct) | -0.59 | 1.7 | 4.96 | 0.74 | 5.34 | 1.7 | yes |
| key-to-key brightness scatter (ERB) | 0.938 | 0.686 | 0.102 | 0.279 | 0.585 | 1.7 | yes |
| brightening from velocity 40 to 127 (ERB) | 1.29 | 0 | 0 | 0 | -0.38 | 4.3 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 6.63 | 6.33 | 7.77 | 5.63 | 3.15 | 0.3 |  |
| decay rate of a held note (dB/s) | 3 | 2.6 | 2.3 | 8.3 | 2.4 | 0.3 |  |
| onset delay (ms) | 11.0 | 13.0 | 3 | 6 | 14.0 | 0.1 |  |
| Plomp-Levelt dissonance of a note (log2) | -3.74 | -3.82 | -1.91 | -3.75 | -5.97 | 0.0 |  |
| envelope curve, note for note (dB rms) | 8.6 (nearest 5.48) | fonts among themselves 7.78 | | | | 1.1 |  |
| pitch re 12-TET A440 (median) (c) | 0.6 | 0.4 | -1.6 | -0.1 | -1.9 | 0.4 |  |
| pitch scatter from key to key (c) | 0.5 | 1.8 | 0.6 | 3.1 | 2.2 | 0.5 |  |
| noisiness (spectral flatness) (dB) | -85.5 | -75.7 | -62.4 | -62.8 | -64.2 | 3.3 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -129 | -127 | -125 | -127 | -114 | 0.5 |  |
| pitch at the start re settled (c) | -0.8 | -0.5 | 0 | -0.3 | -2.1 | 0.1 |  |
| stereo coherence | 0.871 | 1 | 1 | 1 | 1 | 1.7 | yes |
| partial-to-partial jaggedness (dB) | 3.8 | 4 | 2.5 | 2.95 | 2.7 | 1.0 |  |
| dip between legato notes (dB) | 14.1 | 17.1 | 16.0 | 21.4 | 16.1 | 0.9 | yes |
| level evenness of a legato scale (dB) | 1.9 | 0.4 | 1.4 | 0.8 | 3 | 0.6 |  |
| level over the keyboard (dB/oct) | 0.62 | -0.29 | -0.05 | 0.2 | 0.93 | 0.5 |  |
| level against the other instruments (velocity 96) (dB) | 2.8 | 1.8 | 1.1 | 0.9 | 2.6 | 1.3 |  |
| key-to-key level scatter (dB) | 1.56 | 1.44 | 0.05 | 0.55 | 2.49 | 0.5 |  |
| odd / even partials (dB) | -3 | -2.9 | -1.5 | -2.8 | -15.7 | 0.0 |  |
| onset jitter in a phrase (ms) | 18.2 | 47.3 | 29.9 | 25.9 | 36.7 | 1.4 | yes |
| onset in a phrase (re the note-on) (ms) | 5 | 46.0 | 0 | -1 | 0 | 0.2 |  |
| onset sharpness (dB/ms) | 15.6 | 10.9 | 13.2 | 16.1 | 7.6 | 0.8 |  |
| overtone levels, note for note (dB rms) | 12.6 (nearest 8.87) | fonts among themselves 15.6 | | | | 0.8 |  |
| panning across the keys (dB/oct) | -1.25 | 0 | 0 | 0 | -0.04 | 1.2 | yes |
| partial envelopes alike | 0.64 | 0.56 | 1 | 0.984 | 0.733 | 1.0 |  |
| release time (30 dB after note-off) (ms) | 96.0 | 54.0 | 41.0 | 100 | 71.0 | 1.0 |  |
| spectrogram (1 - correlation), note for note | 0.178 (nearest 0.121) | fonts among themselves 0.17 | | | | 1.0 |  |
| spectral shape of the body, note for note | 10 (nearest 9.56) | fonts among themselves 10.7 | | | | 0.9 |  |
| sounding length of a 100 ms note (ms) | 62.0 | 51.0 | 22.0 | 38.0 | 109 | 0.4 |  |
| partial stretch (inharmonicity) (c/partial) | 0.61 | 0.76 | 0.01 | 0.34 | 0.75 | 0.1 |  |
| level after 2 s held, re peak (dB) | -16.8 | -18.1 | -21.3 | -30.1 | -16.6 | 0.4 |  |
| high bands lag low bands by (ms) | -2 | 0.5 | 0 | 2.5 | -2 | 0.5 |  |
| transient coherence: attack spread over bands (ms) | 1.1 | 0.2 | 0.1 | 0.7 | 0.5 | 0.2 |  |
| fundamental share (tristimulus 1) | 0.143 | 0.243 | 0.207 | 0.325 | 0.936 | 0.4 | yes |
| partials 2-4 share (tristimulus 2) | 0.77 | 0.711 | 0.527 | 0.665 | 0.064 | 0.5 | yes |
| partials 5+ share (tristimulus 3) | 0.021 | 0.01 | 0.266 | 0.025 | 0 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 12.3 | 16.6 | 16.6 | 16.7 | 20.1 | 2.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 1.2 | 3.3 | 0.2 | 1.6 | 4.1 | 0.3 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.00688 (nearest 0.00546) | fonts among themselves 0.00809 | | | | 0.9 |  |

</details>

### 35 Pick Bass: pick-bass

Distance 1.47 (rank 126 of 129); the fonts disagree 1.4 JND. Keys 28,31,34,37,40,43,46,49,52,55,58, velocities 40/64/96/127.

- **Louder toward the top (+4.4 dB/oct vs 0…+2.0)**: probable fault, the same pattern as p033 but larger: about 11 dB over E1–A#3 against ≤5.
- **Purer (−88 vs −58…−67 dB flatness)**: artifact. The fonts' samples carry noise. Fret clank is missing ("Not yet" in README "Pick bass").
- **Quieter (−4.5 vs −0.2 median)**: small level trim.
- **Brightens with velocity (+0.55 ERB)**, partials less alike (0.44 vs 0.78–1.0): deliberate. README "Pick bass" says "FluidR3's samples change only in level with velocity; ours brightens". The spectrum agrees with the earlier FluidR3 band check from 120 Hz to 4 kHz; ±12 dB against the other fonts, which disagree with one another.
- Close overall (see its rank above).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **noisiness (spectral flatness)**: purer (-88.0 dB | -67.2 | -59.5 | -62.8 | -58.3), outside the fonts, 6.0 x.
2. **key-to-key brightness scatter**: less even in tone from key to key (1.57 ERB | 0.72 | 0.098 | 0.791 | 0.6), outside the fonts, 2.6 x.
3. **level over the keyboard**: louder toward the bottom (-0.96 dB/oct | 1.97 | -0.01 | 1.95 | 0.66), outside the fonts, 2.3 x.
4. **partial envelopes alike**: partials living more their own lives (0.459 | 0.778 | 1 | 0.988 | 0.776), outside the fonts, 3.8 x.
5. **brightening from velocity 40 to 127**: brightens more with velocity (0.46 ERB | 0 | 0 | 0 | -0.38), outside the fonts, 1.5 x.
6. **loudness range, velocity 40 to 127**: a narrower dynamic range (15.1 dB | 16.7 | 16.7 | 16.6 | 20.1), outside the fonts, 0.9 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +3 | +0 | -6 | +8 | -8 | - |
| re TimGM6mb | +15 | +11 | +2 | +12 | -15 | -33 |
| re Jnsgm2 | -2 | -8 | -13 | +12 | +12 | - |
| re SGM | -23 | -16 | +6 | +12 | +10 | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening +0.0 ERB, loudness range +3 dB, envelope 1.9 dB rms; TimGM6mb: centroid -0.5 ERB, attack +0.5 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 6.8 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 2.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.22 | 0.22 | 0.27 | 1.26 | 0.19 | 0.9 | yes |
| attack time (-20 to -1 dB) (ms) | 19.0 | 9 | 2 | 8 | 17.0 | 0.8 | yes |
| spectral shape of the attack, note for note | 10.2 (nearest 8.1) | fonts among themselves 10.3 | | | | 1.0 |  |
| brightness over the keyboard (ERB/oct) | 0.4 | 2.76 | 5.01 | -2.31 | 4.81 | 0.9 |  |
| key-to-key brightness scatter (ERB) | 1.57 | 0.72 | 0.098 | 0.791 | 0.6 | 2.6 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.46 | 0 | 0 | 0 | -0.38 | 1.5 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 6.89 | 7.95 | 8.54 | 8.16 | 3.34 | 0.4 |  |
| decay rate of a held note (dB/s) | 3.2 | 3.1 | 2.1 | 6.6 | 1.5 | 0.3 |  |
| onset delay (ms) | 22.0 | 25.0 | 7 | 1 | 17.0 | 0.8 |  |
| Plomp-Levelt dissonance of a note (log2) | -3.35 | -2.79 | -1.57 | -2.37 | -5.38 | 0.4 |  |
| envelope curve, note for note (dB rms) | 8.89 (nearest 5.44) | fonts among themselves 8.3 | | | | 1.1 |  |
| pitch re 12-TET A440 (median) (c) | 0.1 | -1.3 | 1.7 | -0.1 | 0.1 | 0.0 |  |
| pitch scatter from key to key (c) | 1.7 | 0.4 | 0.4 | 5 | 3 | 0.0 |  |
| noisiness (spectral flatness) (dB) | -88.0 | -67.2 | -59.5 | -62.8 | -58.3 | 6.0 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -127 | -126 | -124 | -123 | -114 | 0.7 |  |
| pitch at the start re settled (c) | -0.9 | 0.5 | 1.2 | 0.3 | -1.6 | 0.3 |  |
| stereo coherence | 0.917 | 1 | 1 | 1 | 1 | 1.1 | yes |
| partial-to-partial jaggedness (dB) | 5.15 | 4 | 0.9 | 2.1 | 3.8 | 1.4 | yes |
| dip between legato notes (dB) | 13.0 | 18.2 | 14.3 | 14.9 | 14.2 | 0.8 | yes |
| level evenness of a legato scale (dB) | 2.3 | 0.9 | 1.8 | 0.9 | 2.3 | 0.9 |  |
| level over the keyboard (dB/oct) | -0.96 | 1.97 | -0.01 | 1.95 | 0.66 | 2.3 | yes |
| level against the other instruments (velocity 96) (dB) | 2.2 | 0.9 | -0.5 | -3.4 | 2.2 | 0.7 |  |
| key-to-key level scatter (dB) | 1.53 | 1.2 | 0.03 | 0.88 | 1.69 | 0.6 |  |
| odd / even partials (dB) | -5 | -1.1 | -1 | -0.6 | -17.2 | 0.5 |  |
| onset jitter in a phrase (ms) | 48.5 | 34.9 | 10.0 | 38.4 | 40.6 | 0.8 | yes |
| onset in a phrase (re the note-on) (ms) | 6 | 32.0 | 2 | 18.0 | 0 | 0.2 |  |
| onset sharpness (dB/ms) | 8.5 | 11.5 | 12.3 | 12.0 | 11.5 | 1.6 | yes |
| overtone levels, note for note (dB rms) | 13.1 (nearest 8.62) | fonts among themselves 15.4 | | | | 0.8 |  |
| panning across the keys (dB/oct) | -0.93 | 0 | 0.16 | 0 | -0.04 | 0.9 | yes |
| partial envelopes alike | 0.459 | 0.778 | 1 | 0.988 | 0.776 | 3.8 | yes |
| release time (30 dB after note-off) (ms) | 96.0 | 39.0 | 29.0 | 86.0 | 69.0 | 1.1 | yes |
| spectrogram (1 - correlation), note for note | 0.232 (nearest 0.152) | fonts among themselves 0.295 | | | | 0.8 |  |
| spectral shape of the body, note for note | 12.8 (nearest 10.8) | fonts among themselves 12.9 | | | | 1.0 |  |
| sounding length of a 100 ms note (ms) | 128 | 58.0 | 56.0 | 19.0 | 58.0 | 1.5 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.71 | 0.93 | 0 | 0.79 | 0.49 | 0.1 |  |
| level after 2 s held, re peak (dB) | -13.7 | -16.6 | -22.2 | -30.6 | -14.9 | 0.7 |  |
| high bands lag low bands by (ms) | -4.8 | -0.7 | 0 | 0 | -5 | 0.9 |  |
| transient coherence: attack spread over bands (ms) | 2.6 | 0.5 | 0 | 0 | 1.3 | 0.5 |  |
| fundamental share (tristimulus 1) | 0.228 | 0.11 | 0.119 | 0.072 | 0.854 | 0.3 |  |
| partials 2-4 share (tristimulus 2) | 0.676 | 0.795 | 0.651 | 0.772 | 0.146 | 0.1 |  |
| partials 5+ share (tristimulus 3) | 0.06 | 0.021 | 0.231 | 0.105 | 0 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 15.1 | 16.7 | 16.7 | 16.6 | 20.1 | 0.9 | yes |
| pitch wobble / vibrato (p-p) (c) | 1.3 | 2.2 | 0 | 2.6 | 4.4 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0194 (nearest 0.0144) | fonts among themselves 0.0098 | | | | 2.0 | yes |

</details>

### 36 Fretless Bass: fretless-bass

Distance 2.04 (rank 102 of 129); the fonts disagree 1.2 JND. Keys 28,31,34,37,40,43,46,49,52,55,58, velocities 40/64/96/127.

- **Pitch wobble 17.8 c vs 2.5–6.4**: deliberate. The landing error (sd 8 c), the scoops and the ±10 c vibrato are in README "Fretless bass". Audible, and the defining trait.
- **Uneven from key to key (2.5 dB vs 0.04–1.1)**: partly the neck's dead spot (D3 on the G string, README "Electric bass"). Minor.
- **Brightens with velocity (+1.6 ERB vs ≤0)**: deliberate. TiMidity ignores Jnsgm2's velocity-to-filter modulator (research/fretless_bass.md §4.2).
- **Brighter above 2.5 kHz (+6 to +26 dB) and hollower at 400 Hz–1 kHz (−7 to −12)**: the bridge pickup at full and the fliptop blend. The upper partials' bloom (FluidR3's recording) is still "Not yet". Possibly too bright: worth a listen.
- **Smoother legato (dip 13.6 vs 18–21 dB)**: deliberate (one voice). The release comes from the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (2.88 dB | 1.12 | 0.21 | 0.04 | 1.1), outside the fonts, 4.1 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (1.04 ERB | -0.01 | 0 | 0 | -0.38), outside the fonts, 3.5 x.
3. **loudness range, velocity 40 to 127**: a narrower dynamic range (13.2 dB | 16.6 | 16.7 | 16.7 | 20.1), outside the fonts, 2.0 x.
4. **release time (30 dB after note-off) [room]**: rings longer after the key (82.0 ms | 45.0 | 31.0 | 49.0 | 43.0), outside the fonts, 2.7 x.
5. **pitch wobble / vibrato (p-p)**: wobbles more (17.6 c | 2.5 | 2.5 | 3.9 | 6.4), outside the fonts, 2.9 x.
6. **level over the keyboard**: louder toward the top (5.07 dB/oct | -1.64 | -0.09 | -0.53 | -5.37), outside the fonts, 2.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +0 | +4 | -9 | +3 | +11 | - |
| re TimGM6mb | +6 | +8 | -7 | -2 | +5 | - |
| re Jnsgm2 | -6 | -5 | -12 | +11 | +26 | - |
| re SGM | -3 | +2 | -1 | -1 | +15 | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -5 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.2 ERB, attack +0.2 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 2.8 dB rms; TimGM6mb: centroid -0.0 ERB, attack -0.0 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 2.2 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 2.4 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.63 | 0.87 | -2.62 | -0.1 | -0.29 | 0.2 |  |
| attack time (-20 to -1 dB) (ms) | 16.0 | 47.0 | 9 | 5 | 13.0 | 0.3 |  |
| spectral shape of the attack, note for note | 9.08 (nearest 6.35) | fonts among themselves 7.63 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 1.54 | 5.62 | 4.99 | 3.24 | 5.37 | 3.1 | yes |
| key-to-key brightness scatter (ERB) | 0.745 | 0.466 | 0.417 | 0.148 | 0.685 | 1.1 |  |
| brightening from velocity 40 to 127 (ERB) | 1.04 | -0.01 | 0 | 0 | -0.38 | 3.5 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 5.44 | 4.03 | 6.92 | 5.18 | 3.7 | 0.5 |  |
| decay rate of a held note (dB/s) | 5.4 | 5.7 | 3 | 3.4 | 4.8 | 0.9 |  |
| onset delay (ms) | 20.0 | 11.0 | 2 | 5 | 13.0 | 1.2 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.56 | -4.04 | -2.95 | -4.18 | -4.29 | 0.7 | yes |
| envelope curve, note for note (dB rms) | 9.39 (nearest 6.1) | fonts among themselves 6.38 | | | | 1.5 |  |
| pitch re 12-TET A440 (median) (c) | -2.8 | -0.2 | 1.7 | -3.5 | -0.2 | 0.6 |  |
| pitch scatter from key to key (c) | 2.4 | 0.7 | 0.9 | 1 | 1.8 | 0.5 |  |
| noisiness (spectral flatness) (dB) | -77.3 | -56.3 | -66.7 | -72.9 | -57.7 | 1.8 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -130 | -127 | -125 | -126 | -115 | 0.8 | yes |
| pitch at the start re settled (c) | -0.7 | -1.1 | -2.1 | 0.4 | -2.5 | 0.2 |  |
| stereo coherence | 0.981 | 1 | 1 | 1 | 1 | 0.3 |  |
| partial-to-partial jaggedness (dB) | 4.15 | 1.2 | 1.1 | 0.9 | 1.7 | 3.0 | yes |
| dip between legato notes (dB) | 16.3 | 19.6 | 20.4 | 20.9 | 16.7 | 1.8 |  |
| level evenness of a legato scale (dB) | 3.1 | 1 | 1.8 | 1.4 | 3 | 1.5 |  |
| level over the keyboard (dB/oct) | 5.07 | -1.64 | -0.09 | -0.53 | -5.37 | 2.3 | yes |
| level against the other instruments (velocity 96) (dB) | 2.4 | 3.4 | 0.8 | 0.9 | 3.8 | 0.2 |  |
| key-to-key level scatter (dB) | 2.88 | 1.12 | 0.21 | 0.04 | 1.1 | 4.1 | yes |
| odd / even partials (dB) | -5.2 | -2.5 | -0.9 | -4.2 | -7.5 | 0.6 |  |
| onset jitter in a phrase (ms) | 36.1 | 15.2 | 28.5 | 55.8 | 41.5 | 0.1 |  |
| onset in a phrase (re the note-on) (ms) | 17.0 | 5 | -1 | 42.0 | 26.0 | 0.1 |  |
| onset sharpness (dB/ms) | 14.6 | 11.4 | 8 | 10.0 | 9 | 2.5 | yes |
| overtone levels, note for note (dB rms) | 15 (nearest 11.3) | fonts among themselves 10.3 | | | | 1.5 |  |
| panning across the keys (dB/oct) | -0.54 | 0 | 0 | 0 | -0.04 | 0.5 |  |
| partial envelopes alike | 0.829 | 0.893 | 0.966 | 0.932 | 0.851 | 0.8 |  |
| release time (30 dB after note-off) (ms) | 82.0 | 45.0 | 31.0 | 49.0 | 43.0 | 2.7 | yes |
| spectrogram (1 - correlation), note for note | 0.119 (nearest 0.107) | fonts among themselves 0.0922 | | | | 1.3 |  |
| spectral shape of the body, note for note | 10.7 (nearest 8.99) | fonts among themselves 8.22 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 56.0 | 111 | 85.0 | 14.0 | 47.0 | 0.1 |  |
| partial stretch (inharmonicity) (c/partial) | 0.98 | 0.17 | 0 | 0.09 | 0.1 | 1.8 | yes |
| level after 2 s held, re peak (dB) | -23.4 | -18.2 | -18.4 | -22.1 | -18.7 | 1.6 |  |
| high bands lag low bands by (ms) | -4 | 24.0 | 20.6 | 4 | 16.0 | 2.2 | yes |
| transient coherence: attack spread over bands (ms) | 1 | 5.3 | 9.9 | 1.8 | 2.9 | 0.6 |  |
| fundamental share (tristimulus 1) | 0.534 | 0.737 | 0.357 | 0.473 | 0.792 | 0.3 |  |
| partials 2-4 share (tristimulus 2) | 0.43 | 0.174 | 0.38 | 0.517 | 0.204 | 0.8 |  |
| partials 5+ share (tristimulus 3) | 0.002 | 0.057 | 0.262 | 0.01 | 0.004 | 0.2 |  |
| loudness range, velocity 40 to 127 (dB) | 13.2 | 16.6 | 16.7 | 16.7 | 20.1 | 2.0 | yes |
| pitch wobble / vibrato (p-p) (c) | 17.6 | 2.5 | 2.5 | 3.9 | 6.4 | 2.9 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0114 (nearest 0.00675) | fonts among themselves 0.00589 | | | | 1.9 |  |

</details>

### 37 Slap Bass 1: slap-bass-1

Distance 2.55 (rank 81 of 129); the fonts disagree 1.6 JND. Keys 28,31,34,37,40,43,46,49,52,55,58, velocities 40/64/96/127.

- **Brightens strongly with velocity (+3.0 ERB vs 0)**: mostly a TiMidity artifact. The thumb/pop and fret physics are deliberate (README "slap"; slap_bass_2.md §1.4, where under FluidSynth Jnsgm2 gives +24 dB above 2 kHz).
- **Attack 15 ms vs 1–4**: possible fault. FluidR3's thumb samples peak 4–7 ms after their start (research/slap_bass_1.md §1.3), so a thumb slap should be the fastest attack on the list.
- **Uneven from key to key (2.6 dB vs ≤1.2)**: thumb, ghost and late-pop rules plus the dead spot. Check that the probe's single notes don't flip technique.
- **Narrow range (13.7 vs 16.6 dB)**: TiMidity convention. The release comes from the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (2.95 ERB | 0 | 0 | 0 | -0.3), outside the fonts, 9.8 x.
2. **key-to-key level scatter**: less even from key to key (2.33 dB | 0.68 | 0.22 | 0.07 | 1.17), outside the fonts, 3.4 x.
3. **attack time (-20 to -1 dB)**: a slower attack (15.0 ms | 1 | 1 | 2 | 4), outside the fonts, 3.4 x.
4. **key-to-key brightness scatter**: less even in tone from key to key (1.54 ERB | 0.882 | 0.231 | 0.328 | 0.726), outside the fonts, 3.1 x.
5. **loudness range, velocity 40 to 127**: a narrower dynamic range (13.7 dB | 16.6 | 16.7 | 16.6 | 20.0), outside the fonts, 1.7 x.
6. **release time (30 dB after note-off) [room]**: rings longer after the key (84.0 ms | 46.0 | 16.0 | 6 | 20.0), outside the fonts, 1.5 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -1 | -3 | -1 | +4 | +2 | -12 |
| re TimGM6mb | +1 | +1 | +5 | +15 | +6 | -29 |
| re Jnsgm2 | +5 | +2 | -3 | +14 | +4 | -25 |
| re SGM | -7 | -10 | +0 | +4 | +5 | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -10 -4 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.1 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 2.4 dB rms; TimGM6mb: centroid -0.1 ERB, attack -0.3 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 4.0 dB rms; Jnsgm2: centroid -0.1 ERB, attack -0.4 ERB, velocity brightening +0.5 ERB, loudness range +4 dB, envelope 5.4 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.17 | 1.95 | 0.79 | 0.95 | 1.26 | 1.6 | yes |
| attack time (-20 to -1 dB) (ms) | 15.0 | 1 | 1 | 2 | 4 | 3.4 | yes |
| spectral shape of the attack, note for note | 11.7 (nearest 9.43) | fonts among themselves 8.87 | | | | 1.3 |  |
| brightness over the keyboard (ERB/oct) | 0.33 | 0.41 | 4.4 | 4.81 | 1.14 | 1.1 |  |
| key-to-key brightness scatter (ERB) | 1.54 | 0.882 | 0.231 | 0.328 | 0.726 | 3.1 | yes |
| brightening from velocity 40 to 127 (ERB) | 2.95 | 0 | 0 | 0 | -0.3 | 9.8 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 6.14 | 5.32 | 4.24 | 6.98 | 5.29 | 0.6 |  |
| decay rate of a held note (dB/s) | 5.7 | 3.6 | 1.6 | 1.3 | 5.8 | 1.2 |  |
| onset delay (ms) | 9 | 1 | 0 | 1 | 3 | 0.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.4 | -2.69 | -4.24 | -3 | -3.7 | 0.1 |  |
| envelope curve, note for note (dB rms) | 10.3 (nearest 9.73) | fonts among themselves 6.66 | | | | 1.5 | yes |
| pitch re 12-TET A440 (median) (c) | -0.4 | -1.8 | 2.6 | -1.1 | -1.6 | 0.2 |  |
| pitch scatter from key to key (c) | 2.1 | 0.7 | 0.4 | 0.9 | 1.3 | 0.4 |  |
| noisiness (spectral flatness) (dB) | -49.8 | -55.5 | -53.5 | -60.6 | -58.3 | 2.0 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -131 | -126 | -126 | -126 | -117 | 0.8 | yes |
| pitch at the start re settled (c) | -0.4 | 2.1 | 0.4 | 2.9 | 0.5 | 0.3 |  |
| stereo coherence | 0.884 | 1 | 1 | 1 | 1 | 1.5 | yes |
| partial-to-partial jaggedness (dB) | 4.65 | 4.2 | 2.4 | 1.9 | 5.3 | 0.8 |  |
| dip between legato notes (dB) | 10.6 | 10.7 | 11.7 | 11.8 | 11.2 | 0.4 |  |
| level evenness of a legato scale (dB) | 3.2 | 1.5 | 0.8 | 1.1 | 4 | 1.2 |  |
| level over the keyboard (dB/oct) | 1.36 | -1.45 | 0.02 | -0.5 | 6.1 | 0.4 |  |
| level against the other instruments (velocity 96) (dB) | 1.5 | -3.2 | 1.4 | -3.9 | 5.4 | 0.5 |  |
| key-to-key level scatter (dB) | 2.33 | 0.68 | 0.22 | 0.07 | 1.17 | 3.4 | yes |
| odd / even partials (dB) | -3.7 | -6.2 | -11.3 | -2.4 | -7.1 | 0.7 |  |
| onset jitter in a phrase (ms) | 50.4 | 2.4 | 47.5 | 45.0 | 27.4 | 0.6 | yes |
| onset in a phrase (re the note-on) (ms) | 3 | -2 | 6 | -1 | -0 | 0.3 |  |
| onset sharpness (dB/ms) | 16.8 | 17.2 | 1 | 4.4 | 49.8 | 0.2 |  |
| overtone levels, note for note (dB rms) | 11.9 (nearest 9.86) | fonts among themselves 12.3 | | | | 1.0 |  |
| panning across the keys (dB/oct) | -1.11 | 0 | 0 | 0 | -0.04 | 1.1 | yes |
| partial envelopes alike | 0.725 | 0.461 | 0.999 | 0.998 | 0.768 | 0.6 |  |
| release time (30 dB after note-off) (ms) | 84.0 | 46.0 | 16.0 | 6 | 20.0 | 1.5 | yes |
| spectrogram (1 - correlation), note for note | 0.224 (nearest 0.166) | fonts among themselves 0.231 | | | | 1.0 |  |
| spectral shape of the body, note for note | 11.2 (nearest 10.4) | fonts among themselves 11.1 | | | | 1.0 |  |
| sounding length of a 100 ms note (ms) | 58.0 | 32.0 | 27.0 | 79.0 | 16.0 | 0.9 |  |
| partial stretch (inharmonicity) (c/partial) | 0.72 | 1 | 0 | 0 | 2.09 | 0.2 |  |
| level after 2 s held, re peak (dB) | -23.8 | -23.5 | -13.7 | -19.4 | -22.1 | 0.6 |  |
| high bands lag low bands by (ms) | -0.7 | 0 | 0 | 1 | 4.3 | 0.2 |  |
| transient coherence: attack spread over bands (ms) | 1.1 | 0 | 0 | 0.5 | 0.9 | 0.2 |  |
| fundamental share (tristimulus 1) | 0.623 | 0.704 | 0.64 | 0.114 | 0.355 | 0.4 |  |
| partials 2-4 share (tristimulus 2) | 0.322 | 0.222 | 0.357 | 0.857 | 0.64 | 0.6 |  |
| partials 5+ share (tristimulus 3) | 0.041 | 0.037 | 0.003 | 0.029 | 0.003 | 0.4 |  |
| loudness range, velocity 40 to 127 (dB) | 13.7 | 16.6 | 16.7 | 16.6 | 20.0 | 1.7 | yes |
| pitch wobble / vibrato (p-p) (c) | 4.6 | 1.5 | 0.5 | 1.3 | 3.7 | 0.6 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0139 (nearest 0.0104) | fonts among themselves 0.0157 | | | | 0.9 |  |

</details>

### 38 Slap Bass 2: slap-bass-2

Distance 2.67 (rank 73 of 129); the fonts disagree 1.2 JND. Keys 28,31,34,37,40,43,46,49,52,55,58, velocities 40/64/96/127.

- **Louder toward the top (+10.6 dB/oct vs −4.1…+2.2)**: largely deliberate. On GM 38 everything from D2 up is popped (README), so the probe crosses from thumb to pop. The size of the step (≈+25 dB over the range) still looks too large: FluidR3's pop is +4…+9 dB above 2 kHz.
- **Brightening +5.2 ERB vs 0**: TiMidity artifact plus deliberate (slap_bass_2.md §1.4 and README "Against the references": E3 above 2 kHz +23 dB from v32 to v127, Jnsgm2 +24).
- **Attack 15 ms vs 0.5–4**: as p036, worth a look.
- **Narrow range (11.9 dB)**: GM gain curve after the frets' cap; TiMidity convention. The release comes from the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (5.16 ERB | 0 | 0 | 0 | -0.28), outside the fonts, 17.2 x.
2. **loudness range, velocity 40 to 127**: a narrower dynamic range (11.9 dB | 16.6 | 16.7 | 16.7 | 20.0), outside the fonts, 2.8 x.
3. **key-to-key brightness scatter**: less even in tone from key to key (2.33 ERB | 1.02 | 0.165 | 0.205 | 0.881), outside the fonts, 4.2 x.
4. **release time (30 dB after note-off) [room]**: rings longer after the key (84.0 ms | 57.0 | 38.0 | 40.0 | 44.0), outside the fonts, 3.4 x.
5. **partials 2-4 share (tristimulus 2)**: weaker low partials (0.283 | 0.694 | 0.628 | 0.692 | 0.541), outside the fonts, 4.9 x.
6. **attack time (-20 to -1 dB)**: a slower attack (15.0 ms | 4 | 0.5 | 0.5 | 2), outside the fonts, 2.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +4 | -0 | -7 | +3 | +3 | -2 |
| re TimGM6mb | +9 | +4 | +3 | +10 | +5 | -20 |
| re Jnsgm2 | -0 | -6 | -7 | +3 | +4 | +3 |
| re SGM | -9 | -14 | -5 | -1 | +7 | +17 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -9 -4 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack -0.3 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 2.3 dB rms; TimGM6mb: centroid -1.2 ERB, attack +0.8 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 8.4 dB rms; Jnsgm2: centroid -0.1 ERB, attack -0.8 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 5.3 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.95 | 1.4 | -0.28 | 0.69 | 0.98 | 0.1 |  |
| attack time (-20 to -1 dB) (ms) | 15.0 | 4 | 0.5 | 0.5 | 2 | 2.6 | yes |
| spectral shape of the attack, note for note | 8.81 (nearest 8.68) | fonts among themselves 8.04 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | -1.3 | 0.85 | 4.19 | 3.79 | 0.2 | 1.8 | yes |
| key-to-key brightness scatter (ERB) | 2.33 | 1.02 | 0.165 | 0.205 | 0.881 | 4.2 | yes |
| brightening from velocity 40 to 127 (ERB) | 5.16 | 0 | 0 | 0 | -0.28 | 17.2 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 8.37 | 9.51 | 7.76 | 6.93 | 5.7 | 0.5 |  |
| decay rate of a held note (dB/s) | 6.4 | 4.2 | 3 | 2.4 | 6 | 1.3 |  |
| onset delay (ms) | 10.0 | 6 | 0 | 0 | 8 | 0.7 |  |
| Plomp-Levelt dissonance of a note (log2) | -2.73 | -1.67 | -1.32 | -2.21 | -3.61 | 0.7 |  |
| envelope curve, note for note (dB rms) | 8 (nearest 6.78) | fonts among themselves 4.78 | | | | 1.7 | yes |
| pitch re 12-TET A440 (median) (c) | 0.4 | -2.1 | 3.7 | 3 | -3.2 | 0.0 |  |
| pitch scatter from key to key (c) | 2.1 | 1.2 | 0.7 | 1.6 | 5.3 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -43.6 | -45.7 | -49.2 | -46.0 | -54.5 | 0.9 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -130 | -126 | -126 | -126 | -116 | 0.7 | yes |
| pitch at the start re settled (c) | 2.8 | 1.4 | -0.6 | 0.7 | 1 | 0.4 |  |
| stereo coherence | 0.875 | 1 | 1 | 1 | 1 | 1.7 | yes |
| partial-to-partial jaggedness (dB) | 4.7 | 4.4 | 4 | 4.7 | 4.8 | 0.1 |  |
| dip between legato notes (dB) | 12.4 | 10.4 | 9.4 | 7.8 | 9.7 | 1.4 | yes |
| level evenness of a legato scale (dB) | 2.3 | 0.6 | 1.3 | 0.8 | 3.6 | 0.8 |  |
| level over the keyboard (dB/oct) | -0.28 | -4.06 | -0.97 | -1.13 | 2.17 | 0.2 |  |
| level against the other instruments (velocity 96) (dB) | 0.8 | -3.2 | -4.3 | -4.1 | 4 | 1.1 |  |
| key-to-key level scatter (dB) | 1.9 | 0.89 | 0.08 | 0.13 | 2.35 | 1.2 |  |
| odd / even partials (dB) | -4.1 | -0.7 | -0.1 | 0.1 | -5 | 1.5 |  |
| onset jitter in a phrase (ms) | 25.9 | 28.8 | 31.9 | 44.7 | 11.7 | 0.3 |  |
| onset in a phrase (re the note-on) (ms) | 8 | 1 | -2 | -2 | 0 | 0.9 | yes |
| onset sharpness (dB/ms) | 10.9 | 12.1 | -1.9 | -2 | 18.9 | 0.6 |  |
| overtone levels, note for note (dB rms) | 13.4 (nearest 11.7) | fonts among themselves 10.7 | | | | 1.3 |  |
| panning across the keys (dB/oct) | -1.23 | 0 | 0 | 0 | -0.04 | 1.2 | yes |
| partial envelopes alike | 0.909 | 0.904 | 1 | 0.883 | 0.767 | 0.1 |  |
| release time (30 dB after note-off) (ms) | 84.0 | 57.0 | 38.0 | 40.0 | 44.0 | 3.4 | yes |
| spectrogram (1 - correlation), note for note | 0.224 (nearest 0.195) | fonts among themselves 0.176 | | | | 1.3 |  |
| spectral shape of the body, note for note | 11.6 (nearest 10.7) | fonts among themselves 10.4 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 53.0 | 44.0 | 29.0 | 29.0 | 17.0 | 1.3 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.71 | 1 | 0 | 0.49 | 1.63 | 0.0 |  |
| level after 2 s held, re peak (dB) | -26.0 | -24.9 | -23.8 | -23.0 | -23.5 | 0.8 |  |
| high bands lag low bands by (ms) | 0.8 | 1 | 0 | 0 | 4.7 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 1.4 | 2.9 | 0 | 0 | 1.5 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.595 | 0.107 | 0.311 | 0.26 | 0.414 | 2.0 | yes |
| partials 2-4 share (tristimulus 2) | 0.283 | 0.694 | 0.628 | 0.692 | 0.541 | 4.9 | yes |
| partials 5+ share (tristimulus 3) | 0.069 | 0.199 | 0.061 | 0.048 | 0.018 | 0.2 |  |
| loudness range, velocity 40 to 127 (dB) | 11.9 | 16.6 | 16.7 | 16.7 | 20.0 | 2.8 | yes |
| pitch wobble / vibrato (p-p) (c) | 2.6 | 1.9 | 1.4 | 1.9 | 6.4 | 0.1 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0155 (nearest 0.0123) | fonts among themselves 0.0143 | | | | 1.1 |  |

</details>

### 39 Synth Bass 1: synth-bass-1

Distance 2.87 (rank 58 of 129); the fonts disagree 1.2 JND. Keys 28,31,34,37,40,43,46,49,52,55,58, velocities 40/64/96/127.

- **Speaks slowly (attack 28 vs 4…10 ms; onset 24 vs 2…5; 12 ms in a phrase)**: probable FAULT for a bass meant to snap. The patch's amp attack is 2 ms (src/lead.cc), so something else delays the peak, perhaps the filter pluck's resonance or the core's start (it also starts −7.5 c flat).
- **Holds (decay 1.2 vs 4.8…9.3 dB/s; −11 dB after 2 s vs −14…−22)**: DELIBERATE: sustain 0.7 (synth_leads.md, "synth families" table). TiMidity misreads the fonts' envelopes badly here (11.7–14.3 dB rms for FluidR3 and TimGM6mb), so this is partly an ARTIFACT.
- **Darker (2.9 vs 3.2…5.7 ERB)**: mostly a TiMidity ARTIFACT. FluidSynth makes the fonts 1.3–2.1 ERB darker, and SGM gives 3.2. **Range 10.8 dB**: TiMidity's law.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **decay rate of a held note**: decays slower (1.2 dB/s | 9.3 | 7.6 | 4.8 | 6.8), outside the fonts, 5.4 x.
2. **loudness range, velocity 40 to 127**: a narrower dynamic range (10.8 dB | 16.6 | 16.7 | 16.6 | 20.1), outside the fonts, 3.3 x.
3. **attack time (-20 to -1 dB)**: a slower attack (28.0 ms | 4 | 10.0 | 6 | 5), outside the fonts, 3.6 x.
4. **onset in a phrase (re the note-on)**: speaks later in a phrase (45.0 ms | -1 | -1 | -1 | -0), outside the fonts, 4.6 x.
5. **partial envelopes alike**: partials living more their own lives (0.416 | 1 | 1 | 1 | 0.953), outside the fonts, 5.8 x.
6. **brightness (spectral centroid, velocity 96)**: darker (2.92 ERB | 4.87 | 5.41 | 5.69 | 3.21), outside the fonts, 1.8 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +32 | +28 | +22 | +1 | -26 | -22 |
| re TimGM6mb | +31 | +27 | +18 | -5 | -27 | -18 |
| re Jnsgm2 | +33 | +29 | +20 | -4 | -27 | -17 |
| re SGM | -3 | -3 | +4 | +0 | - | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -8 -4 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -1.3 ERB, attack +1.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 12.0 dB rms; TimGM6mb: centroid -1.8 ERB, attack +1.8 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 14.6 dB rms; Jnsgm2: centroid -2.1 ERB, attack +1.1 ERB, velocity brightening +0.7 ERB, loudness range +8 dB, envelope 3.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.15 | 0.06 | 0.29 | 0.13 | 0.67 | 0.1 |  |
| attack time (-20 to -1 dB) (ms) | 28.0 | 4 | 10.0 | 6 | 5 | 3.6 | yes |
| spectral shape of the attack, note for note | 12.9 (nearest 11.6) | fonts among themselves 5.93 | | | | 2.2 | yes |
| brightness over the keyboard (ERB/oct) | 3.75 | 4.28 | 4.45 | 4.36 | 4.76 | 2.2 | yes |
| key-to-key brightness scatter (ERB) | 0.466 | 0.169 | 0.162 | 0.17 | 0.742 | 1.0 |  |
| brightening from velocity 40 to 127 (ERB) | 0.33 | 0 | 0 | 0 | -0.47 | 1.1 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 2.92 | 4.87 | 5.41 | 5.69 | 3.21 | 1.8 | yes |
| decay rate of a held note (dB/s) | 1.2 | 9.3 | 7.6 | 4.8 | 6.8 | 5.4 | yes |
| onset delay (ms) | 24.0 | 2 | 5 | 5 | 3 | 2.0 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.78 | -2.59 | -2.35 | -2.21 | -5.44 | 0.2 |  |
| envelope curve, note for note (dB rms) | 12.9 (nearest 11.8) | fonts among themselves 5.32 | | | | 2.4 | yes |
| pitch re 12-TET A440 (median) (c) | -0.3 | 0 | 1.7 | -1.2 | -0.6 | 0.0 |  |
| pitch scatter from key to key (c) | 0.7 | 1 | 0.4 | 0.7 | 0.6 | 0.0 |  |
| noisiness (spectral flatness) (dB) | -70.4 | -55.0 | -53.5 | -48.3 | -59.0 | 3.0 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -77.0 | -125 | -125 | -123 | -111 | 6.4 | yes |
| pitch at the start re settled (c) | -7.5 | 0 | 0 | 0 | 1.2 | 1.5 | yes |
| stereo coherence | 0.804 | 0.759 | 1 | 1 | 1 | 1.6 |  |
| partial-to-partial jaggedness (dB) | 3.6 | 1.1 | 0.1 | 0.1 | 1.4 | 3.0 | yes |
| dip between legato notes (dB) | 15.9 | 17.1 | 19.2 | 18.0 | 13.9 | 0.6 |  |
| level evenness of a legato scale (dB) | 2.1 | 1.3 | 1.6 | 1.5 | 0.9 | 0.7 |  |
| level over the keyboard (dB/oct) | 0.41 | -0.2 | -3.54 | -0.01 | 3.73 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | 4 | 3.4 | 3.7 | 2.2 | -1.4 | 0.5 |  |
| key-to-key level scatter (dB) | 1.38 | 0.06 | 0.47 | 0.03 | 1.67 | 1.4 |  |
| odd / even partials (dB) | -5.2 | -3.7 | -2.9 | -2.7 | -9.9 | 0.5 |  |
| onset jitter in a phrase (ms) | 50.5 | 57.1 | 39.3 | 44.2 | 41.2 | 0.9 |  |
| onset in a phrase (re the note-on) (ms) | 45.0 | -1 | -1 | -1 | -0 | 4.6 | yes |
| onset sharpness (dB/ms) | 11.1 | 5.4 | 8.5 | 12.5 | 47.8 | 0.0 |  |
| overtone levels, note for note (dB rms) | 16.1 (nearest 6.94) | fonts among themselves 8.89 | | | | 1.8 |  |
| panning across the keys (dB/oct) | -1.08 | 0.02 | 0 | 0 | -0.04 | 1.1 | yes |
| partial envelopes alike | 0.416 | 1 | 1 | 1 | 0.953 | 5.8 | yes |
| release time (30 dB after note-off) (ms) | 138 | 92.0 | 28.0 | 34.0 | 37.0 | 2.3 | yes |
| spectrogram (1 - correlation), note for note | 0.435 (nearest 0.179) | fonts among themselves 0.18 | | | | 2.4 |  |
| spectral shape of the body, note for note | 22.1 (nearest 13.2) | fonts among themselves 11.8 | | | | 1.9 |  |
| sounding length of a 100 ms note (ms) | 96.0 | 71.0 | 15.0 | 43.0 | 114 | 0.5 |  |
| partial stretch (inharmonicity) (c/partial) | -0.09 | 0 | 0 | 0 | 0.04 | 0.2 |  |
| level after 2 s held, re peak (dB) | -11.0 | -21.1 | -21.0 | -14.5 | -22.2 | 2.6 | yes |
| high bands lag low bands by (ms) | -6 | 0 | 0 | 0 | 0 | 1.2 | yes |
| transient coherence: attack spread over bands (ms) | 2.1 | 0 | 0 | 0 | 0 | 0.4 |  |
| fundamental share (tristimulus 1) | 0.839 | 0.698 | 0.62 | 0.629 | 0.962 | 1.0 |  |
| partials 2-4 share (tristimulus 2) | 0.16 | 0.265 | 0.279 | 0.267 | 0.036 | 0.9 |  |
| partials 5+ share (tristimulus 3) | 0.001 | 0.038 | 0.1 | 0.104 | 0.001 | 1.0 |  |
| loudness range, velocity 40 to 127 (dB) | 10.8 | 16.6 | 16.7 | 16.6 | 20.1 | 3.3 | yes |
| pitch wobble / vibrato (p-p) (c) | 5.3 | 0.4 | 0.5 | 0.4 | 8.9 | 1.0 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0275 (nearest 0.0248) | fonts among themselves 0.00809 | | | | 3.4 | yes |

</details>

### 40 Synth Bass 2: synth-bass-2

Distance 1.46 (rank 127 of 129); the fonts disagree 1.7 JND. Keys 28,31,34,37,40,43,46,49,52,55,58, velocities 40/64/96/127.

- **Quieter than the fonts (−0.4 vs +1.4…+6.2 dB)**: a small level fault. The fonts all make this patch loud.
- **Narrow velocity range (11.9 vs 16.6–20.1 dB; −9 at v40)**: a TiMidity convention, and also the patch's velocity-to-envelope routing. Jnsgm2 is misread by TiMidity here (+17 dB range, 11 dB envelope under FluidSynth), so treat the fonts' spread with care.
- **Brightens with velocity (+0.8 ERB)**: deliberate (synth_leads.md §"velocity to envelope depth and filter").
- **Release 164 ms vs 13–53, onset 21 ms vs 2–5**: the room, and `relMul` 0.4. Rank 64: close overall.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (11.9 dB | 16.7 | 16.6 | 16.7 | 20.1), outside the fonts, 2.7 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (0.77 ERB | 0 | 0 | 0 | -0.46), outside the fonts, 2.6 x.
3. **release time (30 dB after note-off) [room]**: rings longer after the key (164 ms | 13.0 | 47.0 | 53.0 | 18.0), outside the fonts, 2.5 x.
4. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-78.0 dB | -131 | -126 | -126 | -112), outside the fonts, 5.0 x.
5. **decay rate of a held note**: decays slower (1.5 dB/s | 1.9 | 2.6 | 4.8 | 3.2), outside the fonts, 1.4 x.
6. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 12.1, nearest 11; the fonts among themselves 5.35), outside.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -2 | -0 | +1 | -0 | - | - |
| re TimGM6mb | +20 | +19 | +14 | +0 | -21 | -32 |
| re Jnsgm2 | +4 | +3 | +2 | -4 | -7 | - |
| re SGM | -10 | -8 | +2 | +9 | - | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -9 -4 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 2.5 dB rms; TimGM6mb: centroid +0.1 ERB, attack -0.3 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 4.5 dB rms; Jnsgm2: centroid -1.9 ERB, attack +0.3 ERB, velocity brightening -0.2 ERB, loudness range +17 dB, envelope 11.2 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.21 | -0.26 | -1.83 | 0.12 | 0.34 | 0.3 |  |
| attack time (-20 to -1 dB) (ms) | 18.0 | 35.0 | 6 | 7 | 18.0 | 0.5 |  |
| spectral shape of the attack, note for note | 9.47 (nearest 8.81) | fonts among themselves 7.96 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 3.16 | 4.37 | 5.19 | 3.35 | 4.77 | 1.5 | yes |
| key-to-key brightness scatter (ERB) | 0.581 | 0.192 | 0.349 | 0.169 | 0.71 | 1.1 |  |
| brightening from velocity 40 to 127 (ERB) | 0.77 | 0 | 0 | 0 | -0.46 | 2.6 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 5.09 | 3.81 | 6.66 | 4.89 | 3.45 | 0.5 |  |
| decay rate of a held note (dB/s) | 1.5 | 1.9 | 2.6 | 4.8 | 3.2 | 1.4 | yes |
| onset delay (ms) | 21.0 | 4 | 2 | 4 | 5 | 1.7 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.18 | -5.8 | -2.91 | -3.34 | -5.27 | 1.5 | yes |
| envelope curve, note for note (dB rms) | 12.1 (nearest 11) | fonts among themselves 5.35 | | | | 2.3 | yes |
| pitch re 12-TET A440 (median) (c) | -0.2 | -4.7 | 2.5 | -1.2 | -0.2 | 0.1 |  |
| pitch scatter from key to key (c) | 0.3 | 6.5 | 0.4 | 0.7 | 0.3 | 0.1 |  |
| noisiness (spectral flatness) (dB) | -69.3 | -72.4 | -60.9 | -65.7 | -78.1 | 0.0 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -78.0 | -131 | -126 | -126 | -112 | 5.0 | yes |
| pitch at the start re settled (c) | -2.4 | -8.6 | -3.9 | 0 | -12.0 | 0.6 |  |
| stereo coherence | 0.731 | 1 | 1 | 1 | 1 | 3.6 | yes |
| partial-to-partial jaggedness (dB) | 3.85 | 4.1 | 3.1 | 0.1 | 4.2 | 0.1 |  |
| dip between legato notes (dB) | 14.0 | 15.5 | 15.6 | 16.4 | 19.6 | 1.0 | yes |
| level evenness of a legato scale (dB) | 1.5 | 0.2 | 1.7 | 1 | 1.7 | 0.1 |  |
| level over the keyboard (dB/oct) | -1.02 | -0.86 | 0.14 | -0.19 | 4.45 | 0.4 |  |
| level against the other instruments (velocity 96) (dB) | 6.2 | 6.7 | 1.6 | 5.1 | 5.9 | 0.3 |  |
| key-to-key level scatter (dB) | 1.36 | 0.06 | 0.26 | 0.03 | 1.44 | 1.7 |  |
| odd / even partials (dB) | -4.7 | -13.1 | -6.4 | -2.8 | -13.2 | 1.0 |  |
| onset jitter in a phrase (ms) | 44.2 | 30.8 | 39.8 | 58.6 | 58.3 | 0.3 |  |
| onset in a phrase (re the note-on) (ms) | 8 | 10.0 | -1 | -0 | 46.0 | 0.1 |  |
| onset sharpness (dB/ms) | 12.4 | 29.5 | 8.1 | 14.6 | 43.0 | 0.6 |  |
| overtone levels, note for note (dB rms) | 10.1 (nearest 9.07) | fonts among themselves 13.3 | | | | 0.8 |  |
| panning across the keys (dB/oct) | -2.73 | 0 | 0 | 0 | -0.04 | 2.7 | yes |
| partial envelopes alike | 0.439 | 0.559 | 0.233 | 1 | 0.977 | 0.9 |  |
| release time (30 dB after note-off) (ms) | 164 | 13.0 | 47.0 | 53.0 | 18.0 | 2.5 | yes |
| spectrogram (1 - correlation), note for note | 0.153 (nearest 0.117) | fonts among themselves 0.117 | | | | 1.3 |  |
| spectral shape of the body, note for note | 14.4 (nearest 10.4) | fonts among themselves 10.6 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 134 | 104 | 13.0 | 43.0 | 85.0 | 0.8 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.03 | 1.1 | 0.03 | 0 | -0.01 | 0.1 |  |
| level after 2 s held, re peak (dB) | -13.3 | -11.3 | -17.5 | -14.9 | -10.1 | 0.1 |  |
| high bands lag low bands by (ms) | -3.1 | 2.5 | -2 | 0 | 0 | 0.6 |  |
| transient coherence: attack spread over bands (ms) | 2.5 | 0.8 | 2 | 0 | 0 | 0.4 |  |
| fundamental share (tristimulus 1) | 0.73 | 0.803 | 0.405 | 0.627 | 0.851 | 0.1 |  |
| partials 2-4 share (tristimulus 2) | 0.258 | 0.188 | 0.413 | 0.287 | 0.148 | 0.2 |  |
| partials 5+ share (tristimulus 3) | 0.009 | 0.007 | 0.182 | 0.079 | 0.001 | 0.4 |  |
| loudness range, velocity 40 to 127 (dB) | 11.9 | 16.7 | 16.6 | 16.7 | 20.1 | 2.7 | yes |
| pitch wobble / vibrato (p-p) (c) | 2.8 | 14.7 | 3.1 | 0.4 | 7.9 | 0.4 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.00834 (nearest 0.00622) | fonts among themselves 0.00851 | | | | 1.0 |  |

</details>

### 41 Violin: violin

Distance 1.85 (rank 111 of 129); the fonts disagree 1.2 JND. Keys 55,58,61,64,67,70,73,76,79,82,85,88,91,94, velocities 40/64/96/127.

- **Loses level toward the top (−5.2 dB/oct vs −1.4…+0.8) and is quieter overall (−4.8 vs −0.6…+2.3 dB)**: probable fault. About 17 dB from G3 to A#6. The fonts' E strings are not weaker, so check the violin's bow-evening table.
- **Weak fundamental (tristimulus-1 0.32 vs 0.56–0.67)**: deliberate in kind. The body is fitted to 17 recorded violins (README, within 5 dB from 196 Hz to 1.4 kHz). The fonts' close samples have more fundamental. The centroid is inside the fonts' range (18.3 vs 15.7–19.9).
- **Speaks late in a phrase (70 ms vs 2–24)**: the bow's onset. The 100 ms pizzicato wait is compensated by noteLead. Real, but on the slow side.
- **Staccato 400 ms**: a 100 ms key is a pizzicato by design (README "Double bass": "a key let go within 100 ms is a pizzicato"). Release and floor come from the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (770 ms | 167 | 82.0 | 149 | 113), outside the fonts, 5.0 x.
2. **onset in a phrase (re the note-on)**: speaks later in a phrase (70.0 ms | 24.0 | 13.0 | 2 | 16.0), outside the fonts, 5.0 x.
3. **fundamental share (tristimulus 1)**: less fundamental (0.317 | 0.615 | 0.67 | 0.557 | 0.619), outside the fonts, 4.3 x.
4. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-66.0 dB | -122 | -121 | -120 | -109), outside the fonts, 8.1 x.
5. **sounding length of a 100 ms note [room]**: longer staccato (400 ms | 218 | 142 | 212 | 164), outside the fonts, 3.6 x.
6. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.39, nearest 0.38; the fonts among themselves 0.11), outside.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -13 | +3 | +3 | +6 | +10 | -12 |
| re TimGM6mb | - | -6 | +1 | +4 | +8 | -9 |
| re Jnsgm2 | -9 | -0 | +7 | +7 | +7 | -16 |
| re SGM | - | +4 | +4 | +5 | +8 | -15 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -1.4 ERB, attack +0.0 ERB, velocity brightening +0.2 ERB, loudness range +4 dB, envelope 2.1 dB rms; TimGM6mb: centroid -0.0 ERB, attack -0.2 ERB, velocity brightening +0.0 ERB, loudness range +3 dB, envelope 1.9 dB rms; Jnsgm2: centroid -0.1 ERB, attack -0.0 ERB, velocity brightening +0.5 ERB, loudness range +4 dB, envelope 1.6 dB rms
- silent probe notes: FluidR3/fs 4

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.17 | -0.42 | 0.34 | 1.91 | 0.13 | 0.3 |  |
| attack time (-20 to -1 dB) (ms) | 714 | 402 | 42.5 | 101 | 80.0 | 1.8 | yes |
| spectral shape of the attack, note for note | 11.5 (nearest 9.84) | fonts among themselves 9.9 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 0.4 | 3.65 | 3.82 | 5.68 | 3.77 | 3.3 | yes |
| key-to-key brightness scatter (ERB) | 2.68 | 1.74 | 0.693 | 2.65 | 2 | 0.8 |  |
| brightening from velocity 40 to 127 (ERB) | -0.56 | -0 | -0 | -0 | 0.39 | 1.9 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 18.3 | 16.4 | 15.7 | 19.9 | 17.9 | 0.5 |  |
| onset delay (ms) | 94.0 | 120 | 28.0 | 56.0 | 47.0 | 0.9 |  |
| Plomp-Levelt dissonance of a note (log2) | -2.91 | -1.51 | -4.76 | -0.79 | -2.4 | 0.5 |  |
| envelope curve, note for note (dB rms) | 19.1 (nearest 18) | fonts among themselves 7.03 | | | | 2.7 | yes |
| pitch re 12-TET A440 (median) (c) | -1.4 | 3.8 | 1.7 | 3.3 | 1.6 | 1.0 | yes |
| pitch scatter from key to key (c) | 6.1 | 11.9 | 1 | 10.6 | 2.2 | 0.1 |  |
| noisiness (spectral flatness) (dB) | -33.5 | -28.4 | -67.0 | -28.7 | -34.0 | 0.1 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -66.0 | -122 | -121 | -120 | -109 | 8.1 | yes |
| pitch at the start re settled (c) | -2.8 | 0.3 | -0.7 | 7.5 | 4 | 1.0 |  |
| stereo coherence | 0.516 | 0.325 | 1 | 1 | 1 | 1.4 |  |
| partial-to-partial jaggedness (dB) | 3.5 | 2.2 | 3 | 2.8 | 2.95 | 0.6 |  |
| dip between legato notes (dB) | 7.1 | 5.9 | 6 | 5.9 | 4.8 | 0.6 | yes |
| level evenness of a legato scale (dB) | 1.9 | 1.1 | 0.9 | 1.4 | 0.5 | 0.9 |  |
| level over the keyboard (dB/oct) | -1.09 | -1.35 | 0.75 | 0.38 | -0.14 | 1.2 |  |
| level against the other instruments (velocity 96) (dB) | 1.3 | -1.1 | 2.5 | -0.4 | 1.8 | 0.3 |  |
| key-to-key level scatter (dB) | 2.11 | 1.47 | 0.75 | 1.13 | 1.94 | 1.4 |  |
| odd / even partials (dB) | -4.5 | -7.9 | -5.2 | -4.2 | -3 | 0.1 |  |
| onset jitter in a phrase (ms) | 41.3 | 38.3 | 17.9 | 49.3 | 37.2 | 0.2 |  |
| onset in a phrase (re the note-on) (ms) | 70.0 | 24.0 | 13.0 | 2 | 16.0 | 5.0 | yes |
| onset sharpness (dB/ms) | 4 | 3.5 | 5.1 | 3.2 | 1.6 | 0.3 |  |
| overtone levels, note for note (dB rms) | 11.7 (nearest 11.1) | fonts among themselves 7.46 | | | | 1.6 | yes |
| panning across the keys (dB/oct) | 0.17 | 0.41 | 0.07 | 0 | 0.02 | 0.1 |  |
| partial envelopes alike | 0.019 | 0.104 | 0.999 | 0.391 | 0.083 | 0.5 | yes |
| release time (30 dB after note-off) (ms) | 770 | 167 | 82.0 | 149 | 113 | 5.0 | yes |
| spectrogram (1 - correlation), note for note | 0.39 (nearest 0.38) | fonts among themselves 0.11 | | | | 3.5 | yes |
| spectral shape of the body, note for note | 11.1 (nearest 10.2) | fonts among themselves 8.4 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 400 | 218 | 142 | 212 | 164 | 3.6 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.1 | 0.21 | 0 | -0.13 | 0.33 | 0.0 |  |
| level after 2 s held, re peak (dB) | -5.1 | -4.8 | -1.5 | -7.1 | -4.3 | 0.2 |  |
| high bands lag low bands by (ms) | 6.2 | 51.0 | 6.4 | -0.2 | 1.4 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 18.8 | 19.8 | 5 | 11.7 | 2.6 | 1.2 |  |
| fundamental share (tristimulus 1) | 0.317 | 0.615 | 0.67 | 0.557 | 0.619 | 4.3 | yes |
| partials 2-4 share (tristimulus 2) | 0.403 | 0.367 | 0.319 | 0.357 | 0.345 | 0.7 | yes |
| partials 5+ share (tristimulus 3) | 0.072 | 0.022 | 0.057 | 0.064 | 0.044 | 0.3 |  |
| loudness range, velocity 40 to 127 (dB) | 17.2 | 16.6 | 16.7 | 16.6 | 20.1 | 0.3 |  |
| pitch wobble / vibrato (p-p) (c) | 22.0 | 37.2 | 0 | 42.2 | 26.9 | 0.5 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0221 (nearest 0.0197) | fonts among themselves 0.0144 | | | | 1.5 | yes |

</details>

### 42 Viola: viola

Distance 2.37 (rank 88 of 129); the fonts disagree 1.2 JND. Keys 48,51,54,57,60,63,66,69,72,75,78,81,84, velocities 40/64/96/127.

- **Darkens with velocity (−1.55 ERB vs 0…+0.5)**: known fault. Viola.md §2 and "Fixed" item 1 note that the centroid still falls from p to ff after `bowBright`.
- **Brighter (15.7 vs 11.0–13.9 ERB; +4…+8 dB at 1–2.5 kHz)**: probable fault. The upper-body resonance bank is "a stand-in, not physics" (Viola.md fix 3), and the model has not been compared with a recording.
- **Speaks late in a phrase (68 ms vs 0–24)**: bow onset, as on the violin.
- **Staccato 354 ms, release 803 ms**: by design (a 100 ms key is a pizzicato; README) plus the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (803 ms | 126 | 103 | 142 | 138), outside the fonts, 10.4 x.
2. **brightening from velocity 40 to 127**: brightens less with velocity (-1.55 ERB | 0 | 0 | 0 | 0.51), outside the fonts, 5.2 x.
3. **onset in a phrase (re the note-on)**: speaks later in a phrase (76.0 ms | 24.0 | -0 | 12.0 | 1), outside the fonts, 5.8 x.
4. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-63.0 dB | -122 | -121 | -120 | -108), outside the fonts, 8.2 x.
5. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 19.3, nearest 18.6; the fonts among themselves 4.45), outside.
6. **sounding length of a 100 ms note [room]**: longer staccato (354 ms | 173 | 121 | 172 | 186), outside the fonts, 3.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +8 | -4 | +0 | +5 | +2 | -4 |
| re TimGM6mb | -19 | -8 | -3 | +6 | +4 | +3 |
| re Jnsgm2 | -13 | -2 | +4 | +8 | -1 | -6 |
| re SGM | +0 | -5 | -4 | +4 | +1 | +3 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack +0.2 ERB, velocity brightening +0.1 ERB, loudness range +3 dB, envelope 2.2 dB rms; TimGM6mb: centroid +0.0 ERB, attack +0.1 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 1.9 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.5 ERB, velocity brightening +0.3 ERB, loudness range +3 dB, envelope 1.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.39 | -0.82 | 1.37 | 2.75 | -0.57 | 0.4 |  |
| attack time (-20 to -1 dB) (ms) | 714 | 606 | 54.0 | 99.0 | 212 | 1.3 | yes |
| spectral shape of the attack, note for note | 10.8 (nearest 10.4) | fonts among themselves 9.01 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 1.39 | 3.71 | 3.96 | 4.28 | 2.52 | 2.8 | yes |
| key-to-key brightness scatter (ERB) | 2.32 | 1.6 | 0.526 | 1.93 | 1.39 | 1.2 | yes |
| brightening from velocity 40 to 127 (ERB) | -1.55 | 0 | 0 | 0 | 0.51 | 5.2 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 15.7 | 12.9 | 11.0 | 13.8 | 13.9 | 1.6 | yes |
| onset delay (ms) | 80.0 | 151 | 38.0 | 57.0 | 71.0 | 0.3 |  |
| Plomp-Levelt dissonance of a note (log2) | -2.71 | -2.51 | -4.72 | -2.09 | -1.78 | 0.3 |  |
| envelope curve, note for note (dB rms) | 19.3 (nearest 18.6) | fonts among themselves 4.45 | | | | 4.3 | yes |
| pitch re 12-TET A440 (median) (c) | 1.4 | -1.4 | 3 | 4.4 | -1 | 0.1 |  |
| pitch scatter from key to key (c) | 4.4 | 5.8 | 0.4 | 7.7 | 5.3 | 0.3 |  |
| noisiness (spectral flatness) (dB) | -39.0 | -33.0 | -67.2 | -27.4 | -36.5 | 0.2 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -63.0 | -122 | -121 | -120 | -108 | 8.2 | yes |
| pitch at the start re settled (c) | -12.7 | 1 | -1 | 3.6 | 3.1 | 3.0 | yes |
| stereo coherence | 0.359 | 0.099 | 1 | 1 | 1 | 1.4 |  |
| partial-to-partial jaggedness (dB) | 3.9 | 3 | 3 | 2.8 | 2.5 | 1.0 | yes |
| dip between legato notes (dB) | 7.1 | 10.6 | 11.0 | 9.5 | 8.8 | 1.5 | yes |
| level evenness of a legato scale (dB) | 1.5 | 0.9 | 0.5 | 1.5 | 0.9 | 0.6 |  |
| level over the keyboard (dB/oct) | 0.51 | -1.97 | 0.66 | 0.92 | 0.71 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | -0.2 | -2.6 | 2.7 | -0.8 | 2 | 0.3 |  |
| key-to-key level scatter (dB) | 1.9 | 1.12 | 0.54 | 1 | 0.81 | 2.0 | yes |
| odd / even partials (dB) | -2 | -3.8 | -4.1 | -4.2 | -2.7 | 1.0 |  |
| onset jitter in a phrase (ms) | 52.3 | 28.6 | 22.8 | 25.8 | 31.3 | 5.0 | yes |
| onset in a phrase (re the note-on) (ms) | 76.0 | 24.0 | -0 | 12.0 | 1 | 5.8 | yes |
| onset sharpness (dB/ms) | 4.5 | 5.4 | 7.3 | 3.9 | 7.3 | 0.9 |  |
| overtone levels, note for note (dB rms) | 9.15 (nearest 8.41) | fonts among themselves 6.34 | | | | 1.4 | yes |
| panning across the keys (dB/oct) | -0.46 | 0.45 | -0.09 | 0 | -0.05 | 0.4 |  |
| partial envelopes alike | 0.033 | 0.113 | 0.999 | 0.39 | 0.217 | 0.6 | yes |
| release time (30 dB after note-off) (ms) | 803 | 126 | 103 | 142 | 138 | 10.4 | yes |
| spectrogram (1 - correlation), note for note | 0.348 (nearest 0.267) | fonts among themselves 0.0995 | | | | 3.5 | yes |
| spectral shape of the body, note for note | 11.8 (nearest 9.75) | fonts among themselves 8.86 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 354 | 173 | 121 | 172 | 186 | 3.3 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.06 | -0.15 | 0 | 0.1 | -0.03 | 0.1 |  |
| level after 2 s held, re peak (dB) | -6.8 | -5.8 | -2.1 | -8 | -6.2 | 0.3 |  |
| high bands lag low bands by (ms) | 6 | 0.7 | -4 | -13.5 | 8.8 | 0.7 |  |
| transient coherence: attack spread over bands (ms) | 20.1 | 14.8 | 6.5 | 11.7 | 17.2 | 1.3 | yes |
| fundamental share (tristimulus 1) | 0.133 | 0.193 | 0.662 | 0.402 | 0.482 | 1.3 | yes |
| partials 2-4 share (tristimulus 2) | 0.371 | 0.683 | 0.262 | 0.467 | 0.408 | 0.3 |  |
| partials 5+ share (tristimulus 3) | 0.216 | 0.089 | 0.083 | 0.137 | 0.058 | 1.9 | yes |
| loudness range, velocity 40 to 127 (dB) | 16.5 | 16.7 | 16.7 | 16.7 | 20.2 | 0.1 |  |
| pitch wobble / vibrato (p-p) (c) | 26.6 | 27.8 | 0 | 31.3 | 32.6 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0209 (nearest 0.0186) | fonts among themselves 0.0144 | | | | 1.5 |  |

</details>

### 43 Cello: cello

Distance 1.61 (rank 118 of 129); the fonts disagree 1.5 JND. Keys 36,39,42,45,48,51,54,57,60,63,66,69,72,75, velocities 40/64/96/127.

- **Brighter and nasal (11.1 vs 6.7–9.5 ERB; +9…+13 dB at 400 Hz–1 kHz; tristimulus-1 0.19 vs 0.33–0.87)**: probable fault. All four fonts agree, and research/cello.md §10 says "no comparison with a recording".
- **Staccato 400 ms vs 132–184, release 683 ms**: by design (a 100 ms key is a pizzicato: README "Double bass (plucked)") plus the room.
- **Darkens slightly with velocity (−0.6 ERB)**: same bow-bandwidth issue as the viola (Viola.md §2). The `bowBright` fix was viola-only.
- Onset and attack are inside the fonts' range (FluidR3 and Jnsgm2 are just as slow).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (683 ms | 184 | 134 | 120 | 129), outside the fonts, 7.7 x.
2. **sounding length of a 100 ms note [room]**: longer staccato (400 ms | 184 | 154 | 164 | 126), outside the fonts, 4.9 x.
3. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-62.1 dB | -123 | -122 | -124 | -112), outside the fonts, 10.1 x.
4. **brightness (spectral centroid, velocity 96)**: brighter (11.1 ERB | 9.5 | 6.65 | 7.58 | 9.15), outside the fonts, 1.9 x.
5. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.351, nearest 0.325; the fonts among themselves 0.0951), outside.
6. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 18.7, nearest 17.1; the fonts among themselves 5.26), outside.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +4 | +6 | +11 | +6 | +3 | -19 |
| re TimGM6mb | -15 | +0 | +9 | +8 | +3 | -16 |
| re Jnsgm2 | +0 | +3 | +13 | +8 | -1 | -21 |
| re SGM | -5 | +4 | +12 | +6 | +2 | -17 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -14 -6 +0 +5; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.1 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 1.7 dB rms; TimGM6mb: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.4 ERB, loudness range +4 dB, envelope 1.9 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.2 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 1.8 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.8 | -0.32 | 0.03 | 0.08 | -0.23 | 2.2 | yes |
| attack time (-20 to -1 dB) (ms) | 554 | 489 | 21.5 | 495 | 411 | 0.1 | yes |
| spectral shape of the attack, note for note | 10.5 (nearest 9.92) | fonts among themselves 7.42 | | | | 1.4 | yes |
| brightness over the keyboard (ERB/oct) | 2.36 | 3.97 | 3.64 | 4.23 | 4.73 | 3.2 | yes |
| key-to-key brightness scatter (ERB) | 1.99 | 2.29 | 0.167 | 1.08 | 1.52 | 0.6 |  |
| brightening from velocity 40 to 127 (ERB) | -0.6 | -0 | -0 | -0 | 0.06 | 2.0 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 11.1 | 9.5 | 6.65 | 7.58 | 9.15 | 1.9 | yes |
| onset delay (ms) | 162 | 230 | 20.0 | 214 | 74.0 | 0.2 |  |
| Plomp-Levelt dissonance of a note (log2) | -2.93 | -2.99 | -5.09 | -3.88 | -3.58 | 0.8 |  |
| envelope curve, note for note (dB rms) | 18.7 (nearest 17.1) | fonts among themselves 5.26 | | | | 3.6 | yes |
| pitch re 12-TET A440 (median) (c) | -1.4 | 0.9 | 2.6 | 3.2 | 2.5 | 1.0 | yes |
| pitch scatter from key to key (c) | 6 | 4.4 | 0.7 | 4.9 | 6 | 0.4 |  |
| noisiness (spectral flatness) (dB) | -46.2 | -32.9 | -74.8 | -37.8 | -35.0 | 0.5 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -62.1 | -123 | -122 | -124 | -112 | 10.1 | yes |
| pitch at the start re settled (c) | -8.5 | -7.1 | 0 | -3.5 | -18.2 | 0.4 |  |
| stereo coherence | 0.422 | 0.416 | 1 | 1 | 1 | 2.0 |  |
| partial-to-partial jaggedness (dB) | 5 | 3.35 | 1.45 | 2.75 | 3.55 | 1.9 | yes |
| dip between legato notes (dB) | 11.3 | 6.1 | 10.6 | 16.7 | 12.0 | 0.0 |  |
| level evenness of a legato scale (dB) | 2.7 | 1.4 | 1.2 | 1.8 | 2.9 | 1.1 |  |
| level over the keyboard (dB/oct) | -0.26 | -0.79 | -0.18 | -1.78 | -1.1 | 0.7 |  |
| level against the other instruments (velocity 96) (dB) | 2.6 | -2.8 | 4.3 | 1.1 | 4.6 | 0.0 |  |
| key-to-key level scatter (dB) | 2.08 | 1.39 | 0.3 | 1.64 | 1.35 | 1.1 | yes |
| odd / even partials (dB) | -5.9 | -1.1 | -2.5 | -4.8 | -6.2 | 0.9 |  |
| onset jitter in a phrase (ms) | 43.9 | 54.8 | 24.1 | 54.6 | 46.1 | 0.4 |  |
| onset in a phrase (re the note-on) (ms) | 72.0 | 76.0 | 13.0 | 27.0 | 1 | 1.4 |  |
| onset sharpness (dB/ms) | 6.6 | 6.4 | 8 | 7.2 | 9.1 | 0.5 |  |
| overtone levels, note for note (dB rms) | 10 (nearest 9.64) | fonts among themselves 7.81 | | | | 1.3 |  |
| panning across the keys (dB/oct) | 0.03 | 0.49 | -0 | -0 | -0.01 | 0.0 |  |
| partial envelopes alike | 0.042 | 0.344 | 0.245 | 0.636 | 0.101 | 0.9 | yes |
| release time (30 dB after note-off) (ms) | 683 | 184 | 134 | 120 | 129 | 7.7 | yes |
| spectrogram (1 - correlation), note for note | 0.351 (nearest 0.325) | fonts among themselves 0.0951 | | | | 3.7 | yes |
| spectral shape of the body, note for note | 11.2 (nearest 10.6) | fonts among themselves 7.27 | | | | 1.5 | yes |
| sounding length of a 100 ms note (ms) | 400 | 184 | 154 | 164 | 126 | 4.9 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.03 | 0.06 | 0 | -0.04 | 0.01 | 0.0 |  |
| level after 2 s held, re peak (dB) | -7.9 | -7.1 | -3.5 | -6.3 | -7.4 | 0.4 |  |
| high bands lag low bands by (ms) | -10.3 | 13.3 | -5.3 | -2.7 | -10.5 | 0.5 |  |
| transient coherence: attack spread over bands (ms) | 17.4 | 16.6 | 2 | 10.4 | 9.3 | 1.0 |  |
| fundamental share (tristimulus 1) | 0.192 | 0.332 | 0.867 | 0.572 | 0.659 | 1.6 | yes |
| partials 2-4 share (tristimulus 2) | 0.405 | 0.356 | 0.096 | 0.395 | 0.311 | 0.5 |  |
| partials 5+ share (tristimulus 3) | 0.122 | 0.08 | 0.036 | 0.026 | 0.034 | 1.2 | yes |
| loudness range, velocity 40 to 127 (dB) | 19.0 | 16.6 | 16.6 | 16.6 | 20.1 | 1.4 |  |
| pitch wobble / vibrato (p-p) (c) | 27.7 | 21.6 | 0 | 22.5 | 33.0 | 0.3 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0228 (nearest 0.0205) | fonts among themselves 0.0135 | | | | 1.7 | yes |

</details>

### 44 Contrabass: contrabass

Distance 2.20 (rank 97 of 129); the fonts disagree 1.7 JND. Keys 28,31,34,37,40,43,46,49,52,55,58, velocities 40/64/96/127.

- **Uneven from key to key (4.0 dB vs 0.3–2.1)**: likely a regression. research/double_bass.md §7e reports 1.3 dB std after the bow-evening table. The body changed to modes on 2026-11-20 (§7f), so `tools/bowlevels.py` probably needs a re-run, as the viola's table did.
- **Darkens with velocity (−1.2 ERB vs ≤0)**: the bow-bandwidth fault (Viola.md §2, not applied to the bass).
- **Speaks late (onset 126 ms, 64 ms in a phrase, vs 37–108 and −1…24)**: bow onset (double_bass.md §7e: 40–180 ms). There is no noteLead for the contrabass.
- **Wobble 26 c vs 4–18**: the vibrato knob plus the finger's tremble. Possibly heavy for a bass.
- **Brighter (6.5 vs 3.9–5.5 ERB)**: the fonts disagree (+10…+19 dB at 150–400 Hz against FluidR3 and SGM, about 0 against the other two). Minor.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **onset in a phrase (re the note-on)**: speaks later in a phrase (98.0 ms | -1 | 24.0 | 24.0 | 16.0), outside the fonts, 6.2 x.
2. **brightening from velocity 40 to 127**: brightens less with velocity (-1.19 ERB | 0 | 0 | 0 | -0.32), outside the fonts, 4.0 x.
3. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-57.9 dB | -121 | -124 | -123 | -115), outside the fonts, 10.7 x.
4. **brightness (spectral centroid, velocity 96)**: brighter (6.47 ERB | 4.63 | 5.53 | 5.46 | 3.93), outside the fonts, 1.8 x.
5. **key-to-key level scatter**: less even from key to key (2.94 dB | 2.08 | 0.59 | 0.33 | 1.15), outside the fonts, 2.4 x.
6. **sounding length of a 100 ms note [room]**: longer staccato (200 ms | 114 | 116 | 116 | 133), outside the fonts, 3.1 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +10 | +19 | +10 | +5 | -5 | -23 |
| re TimGM6mb | -3 | +2 | +1 | +4 | -6 | - |
| re Jnsgm2 | -4 | +1 | +0 | +4 | -4 | - |
| re SGM | +2 | +10 | +8 | +7 | -8 | -28 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -15 -7 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack -0.0 ERB, velocity brightening -0.0 ERB, loudness range +3 dB, envelope 2.2 dB rms; TimGM6mb: centroid +0.0 ERB, attack -0.1 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 2.2 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.1 ERB, velocity brightening -0.4 ERB, loudness range +4 dB, envelope 2.2 dB rms
- silent probe notes: FluidR3/fs 4

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 1.24 | 0.4 | -0.92 | -0.84 | -0.12 | 2.6 | yes |
| attack time (-20 to -1 dB) (ms) | 351 | 317 | 79.0 | 91.0 | 735 | 0.7 |  |
| spectral shape of the attack, note for note | 9.76 (nearest 7.99) | fonts among themselves 6.34 | | | | 1.5 |  |
| brightness over the keyboard (ERB/oct) | 3.94 | 14.8 | 5.13 | 5.09 | 9.37 | 0.7 | yes |
| key-to-key brightness scatter (ERB) | 1.51 | 1.52 | 0.41 | 0.399 | 0.688 | 1.7 |  |
| brightening from velocity 40 to 127 (ERB) | -1.19 | 0 | 0 | 0 | -0.32 | 4.0 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 6.47 | 4.63 | 5.53 | 5.46 | 3.93 | 1.8 | yes |
| onset delay (ms) | 126 | 73.0 | 37.0 | 37.0 | 122 | 1.7 |  |
| Plomp-Levelt dissonance of a note (log2) | -2.93 | -3.32 | -4.41 | -4.44 | -4.11 | 2.4 | yes |
| envelope curve, note for note (dB rms) | 12.9 (nearest 12) | fonts among themselves 5.05 | | | | 2.6 | yes |
| pitch re 12-TET A440 (median) (c) | 2.2 | 0.2 | 2.3 | 4.5 | 0.2 | 0.2 |  |
| pitch scatter from key to key (c) | 8 | 0.9 | 0.9 | 1.5 | 11.3 | 1.3 |  |
| noisiness (spectral flatness) (dB) | -53.2 | -35.4 | -60.1 | -58.2 | -35.8 | 0.5 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -57.9 | -121 | -124 | -123 | -115 | 10.7 | yes |
| pitch at the start re settled (c) | -6.5 | -5.7 | -7.1 | -7.3 | -7.3 | 0.1 |  |
| stereo coherence | 0.587 | 0.577 | 1 | 1 | 1 | 2.0 |  |
| partial-to-partial jaggedness (dB) | 5.15 | 3.9 | 7.2 | 8.9 | 3.5 | 0.1 |  |
| dip between legato notes (dB) | 15.0 | 16.6 | 14.1 | 13.4 | 16.2 | 0.1 |  |
| level evenness of a legato scale (dB) | 2.4 | 1.3 | 0.9 | 0.7 | 2.5 | 1.3 |  |
| level over the keyboard (dB/oct) | -2.9 | -6.28 | -0.62 | -0.3 | -7.39 | 0.2 |  |
| level against the other instruments (velocity 96) (dB) | 3.8 | -2.3 | 1.9 | 1.9 | 4.6 | 0.6 |  |
| key-to-key level scatter (dB) | 2.94 | 2.08 | 0.59 | 0.33 | 1.15 | 2.4 | yes |
| odd / even partials (dB) | -7 | -1.9 | -16.6 | -16.4 | -5.7 | 0.6 |  |
| onset jitter in a phrase (ms) | 45.0 | 35.6 | 57.8 | 41.3 | 34.8 | 0.6 |  |
| onset in a phrase (re the note-on) (ms) | 98.0 | -1 | 24.0 | 24.0 | 16.0 | 6.2 | yes |
| onset sharpness (dB/ms) | 9.3 | 11.2 | 9.5 | 9.6 | 9.8 | 0.2 |  |
| overtone levels, note for note (dB rms) | 11.9 (nearest 11.6) | fonts among themselves 10.1 | | | | 1.2 |  |
| panning across the keys (dB/oct) | -1.55 | 0.27 | 0 | 0 | -0.04 | 1.6 | yes |
| partial envelopes alike | 0.114 | 0.445 | 0.177 | 0.372 | 0.562 | 1.5 | yes |
| release time (30 dB after note-off) (ms) | 334 | 63.0 | 135 | 157 | 114 | 2.2 | yes |
| spectrogram (1 - correlation), note for note | 0.216 (nearest 0.171) | fonts among themselves 0.09 | | | | 2.4 | yes |
| spectral shape of the body, note for note | 10.8 (nearest 9.25) | fonts among themselves 8.12 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 200 | 114 | 116 | 116 | 133 | 3.1 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.14 | 0.04 | 0.01 | 0.01 | -0.02 | 0.3 |  |
| level after 2 s held, re peak (dB) | -8.6 | -5.1 | -8 | -8.3 | -7.4 | 0.3 |  |
| high bands lag low bands by (ms) | -14.3 | 4.7 | -8.7 | -8.7 | -7 | 1.0 | yes |
| transient coherence: attack spread over bands (ms) | 14.6 | 12.5 | 7.5 | 7.5 | 4.4 | 1.4 |  |
| fundamental share (tristimulus 1) | 0.05 | 0.103 | 0.324 | 0.338 | 0.39 | 2.0 | yes |
| partials 2-4 share (tristimulus 2) | 0.722 | 0.255 | 0.636 | 0.624 | 0.512 | 0.8 | yes |
| partials 5+ share (tristimulus 3) | 0.068 | 0.118 | 0.041 | 0.038 | 0.014 | 0.4 |  |
| loudness range, velocity 40 to 127 (dB) | 18.6 | 16.7 | 16.6 | 16.6 | 20.1 | 1.1 |  |
| pitch wobble / vibrato (p-p) (c) | 25.9 | 8.5 | 4.1 | 4.2 | 18.1 | 2.8 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0189 (nearest 0.0159) | fonts among themselves 0.0118 | | | | 1.6 | yes |

</details>

### 45 Tremolo Strings: tremolo-strings

Distance 3.03 (rank 47 of 129); the fonts disagree 1.2 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Brighter with a weak fundamental (15.4 vs 11.9–12.9 ERB; tristimulus-1 0.16 vs 0.43–0.64)**: probable fault, the same as the bowed sections (p048). The fonts agree tightly.
- **Darkens with velocity (−2.1 ERB)**: the bow-bandwidth fault (Viola.md §2).
- **Speaks late in a phrase (72 ms vs 0–13)**: the seats' spread (20 ± 15 ms, the desks) plus the bow. Fonts' tremolo samples start at once. Partly deliberate (README string ensembles), but late.
- **Release 914 ms**: the room. Wobble and stereo are inside the fonts' range.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (15.2 ERB | 12.6 | 12.3 | 11.9 | 12.9), outside the fonts, 5.7 x.
2. **brightening from velocity 40 to 127**: brightens less with velocity (-1.98 ERB | -0.03 | -0 | 0 | -0.23), outside the fonts, 6.6 x.
3. **onset in a phrase (re the note-on)**: speaks later in a phrase (85.0 ms | 1 | 1 | -0 | 15.0), outside the fonts, 8.4 x.
4. **fundamental share (tristimulus 1)**: less fundamental (0.167 | 0.568 | 0.639 | 0.474 | 0.428), outside the fonts, 3.4 x.
5. **key-to-key brightness scatter**: less even in tone from key to key (1.69 ERB | 1.22 | 1.23 | 1.55 | 1.17), outside the fonts, 2.3 x.
6. **partials 5+ share (tristimulus 3)**: stronger high partials (0.213 | 0.016 | 0.029 | 0.045 | 0.06), outside the fonts, 2.5 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -6 | +5 | +4 | +8 | +6 | -14 |
| re TimGM6mb | -15 | +1 | +0 | +4 | -1 | +5 |
| re Jnsgm2 | -7 | +6 | +7 | +9 | +5 | -18 |
| re SGM | -11 | +3 | +4 | +6 | +3 | -10 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -14 -6 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 1.7 dB rms; TimGM6mb: centroid +0.5 ERB, attack +0.0 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 4.1 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.2 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 1.8 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.03 | 0.76 | 2.21 | 0.92 | 0.64 | 1.0 | yes |
| attack time (-20 to -1 dB) (ms) | 618 | 738 | 176 | 362 | 784 | 0.2 |  |
| spectral shape of the attack, note for note | 9.31 (nearest 8.77) | fonts among themselves 8.54 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | 3.48 | 3.5 | 4.12 | 3.9 | 4.06 | 1.6 |  |
| key-to-key brightness scatter (ERB) | 1.69 | 1.22 | 1.23 | 1.55 | 1.17 | 2.3 | yes |
| brightening from velocity 40 to 127 (ERB) | -1.98 | -0.03 | -0 | 0 | -0.23 | 6.6 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 15.2 | 12.6 | 12.3 | 11.9 | 12.9 | 5.7 | yes |
| onset delay (ms) | 131 | 163 | 78.0 | 34.0 | 392 | 0.1 |  |
| Plomp-Levelt dissonance of a note (log2) | -0.68 | -2.36 | -2.65 | -0.7 | -0.42 | 0.8 |  |
| envelope curve, note for note (dB rms) | 15 (nearest 7.67) | fonts among themselves 10.3 | | | | 1.5 |  |
| pitch re 12-TET A440 (median) (c) | -6.3 | 0.8 | -1.8 | -5.2 | 2.8 | 1.4 |  |
| pitch scatter from key to key (c) | 11.6 | 11.0 | 3.4 | 19.9 | 8.5 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -32.7 | -33.2 | -35.5 | -25.0 | -25.9 | 0.6 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -54.1 | -94.7 | -126 | -121 | -106 | 3.8 | yes |
| pitch at the start re settled (c) | -1.7 | -2.5 | -8.9 | -2.1 | 0 | 0.1 |  |
| stereo coherence | 0.081 | 0.22 | 0.995 | 1 | 0.127 | 1.2 | yes |
| partial-to-partial jaggedness (dB) | 2.75 | 2.3 | 2.2 | 1.8 | 1.9 | 0.7 |  |
| dip between legato notes (dB) | 5.9 | 11.1 | 9.7 | 9.2 | 9.8 | 1.9 | yes |
| level evenness of a legato scale (dB) | 1.4 | 0.8 | 0.9 | 1 | 3.1 | 0.4 |  |
| level over the keyboard (dB/oct) | -1.18 | 0.44 | -1.49 | -0.52 | -1.17 | 0.3 |  |
| level against the other instruments (velocity 96) (dB) | -0.2 | -1.9 | 3.7 | -0.1 | -9.5 | 0.1 |  |
| key-to-key level scatter (dB) | 2.17 | 1.43 | 2.07 | 1.75 | 1.78 | 0.8 |  |
| odd / even partials (dB) | -3.4 | -10.8 | -3.3 | -4.8 | -4.8 | 0.4 |  |
| onset jitter in a phrase (ms) | 49.2 | 54.1 | 28.3 | 38.0 | 41.6 | 0.7 |  |
| onset in a phrase (re the note-on) (ms) | 85.0 | 1 | 1 | -0 | 15.0 | 8.4 | yes |
| onset sharpness (dB/ms) | 4.7 | 5.6 | 4.8 | 6.7 | 6.2 | 0.6 |  |
| overtone levels, note for note (dB rms) | 8.63 (nearest 7.74) | fonts among themselves 6.98 | | | | 1.2 |  |
| panning across the keys (dB/oct) | 0.95 | 0.25 | -0.04 | 0 | -0.08 | 1.0 | yes |
| partial envelopes alike | 0.051 | 0.056 | 0.182 | 0.076 | 0.363 | 0.5 |  |
| release time (30 dB after note-off) (ms) | 914 | 618 | 226 | 337 | 417 | 1.8 | yes |
| spectrogram (1 - correlation), note for note | 0.215 (nearest 0.145) | fonts among themselves 0.149 | | | | 1.4 |  |
| spectral shape of the body, note for note | 9.42 (nearest 7.54) | fonts among themselves 6.86 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 292 | 330 | 225 | 184 | 223 | 0.9 |  |
| partial stretch (inharmonicity) (c/partial) | 0.47 | -0.05 | -0.35 | -0.28 | 0.24 | 1.3 |  |
| level after 2 s held, re peak (dB) | -7.4 | -7.8 | -15.0 | -8.1 | -7.9 | 0.2 |  |
| high bands lag low bands by (ms) | -7.4 | -3.6 | -18.3 | -17.6 | -8.3 | 0.8 |  |
| transient coherence: attack spread over bands (ms) | 22.4 | 15.1 | 20.8 | 17.0 | 20.0 | 0.8 |  |
| fundamental share (tristimulus 1) | 0.167 | 0.568 | 0.639 | 0.474 | 0.428 | 3.4 | yes |
| partials 2-4 share (tristimulus 2) | 0.446 | 0.4 | 0.328 | 0.491 | 0.491 | 0.0 |  |
| partials 5+ share (tristimulus 3) | 0.213 | 0.016 | 0.029 | 0.045 | 0.06 | 2.5 | yes |
| loudness range, velocity 40 to 127 (dB) | 17.2 | 16.7 | 16.6 | 16.6 | 20.1 | 0.3 |  |
| pitch wobble / vibrato (p-p) (c) | 46.9 | 24.4 | 23.6 | 58.6 | 47.4 | 0.6 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0212 (nearest 0.0199) | fonts among themselves 0.0195 | | | | 1.1 |  |

</details>

### 46 Pizzicato Strings: pizzicato-strings

Distance 5.28 (rank 2 of 129); the fonts disagree 1.8 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Bows instead of plucking: level after 2 s −7 dB vs −92…−127; attack 708 ms vs 14–56; tristimulus-1 0.23 vs 0.86–0.96; centroid 16.8 vs 9.1–9.6 ERB**: FAULT, a bug. `BowPlayer::setPizzicatoOnly(true)` (src/bassinst.h:151) only sets `wait_ = 0`, and with `wait_ <= 0` `BowPlayer::noteOn` calls `strike()`, a bow stroke (src/bassinst.cc:673). The pluck happens only in `due()` for keys released while waiting. So GM 46 is the bowed section 3 dB down. Every other row follows from this.
- **Onset 174 ms**: the same bug (a slow bow stroke).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (16.6 ERB | 9.14 | 9.56 | 9.33 | 9.07), outside the fonts, 24.6 x.
2. **onset delay**: speaks later (176 ms | 50.0 | 8 | 11.0 | 13.0), outside the fonts, 7.8 x.
3. **brightening from velocity 40 to 127**: brightens less with velocity (-1.68 ERB | 0.02 | 0.01 | -0 | -0.15), outside the fonts, 5.6 x.
4. **level after 2 s held, re peak**: sustains more (-7.2 dB | -127 | -91.8 | -124 | -108), outside the fonts, 6.2 x.
5. **fundamental share (tristimulus 1)**: less fundamental (0.225 | 0.952 | 0.924 | 0.86 | 0.96), outside the fonts, 10.2 x.
6. **onset in a phrase (re the note-on)**: speaks later in a phrase (72.0 ms | 1 | -1 | 0 | 0), outside the fonts, 7.2 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -21 | -3 | -3 | +3 | +10 | +0 |
| re TimGM6mb | -20 | +1 | +2 | +7 | +7 | -9 |
| re Jnsgm2 | -20 | -0 | +0 | +6 | +13 | -11 |
| re SGM | -14 | -1 | -1 | +8 | +11 | -11 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -15 -7 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.1 ERB, attack -0.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 3.1 dB rms; TimGM6mb: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 2.6 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 3.2 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.05 | 0.15 | 0.26 | -0.05 | 1.49 | 0.2 |  |
| attack time (-20 to -1 dB) (ms) | 664 | 56.0 | 14.0 | 13.5 | 37.0 | 4.7 | yes |
| spectral shape of the attack, note for note | 11.5 (nearest 10.5) | fonts among themselves 8.11 | | | | 1.4 |  |
| brightness over the keyboard (ERB/oct) | 3.69 | 3.41 | 3.76 | 3.8 | 2.61 | 0.2 |  |
| key-to-key brightness scatter (ERB) | 2.34 | 0.228 | 0.309 | 0.439 | 0.968 | 5.3 | yes |
| brightening from velocity 40 to 127 (ERB) | -1.68 | 0.02 | 0.01 | -0 | -0.15 | 5.6 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 16.6 | 9.14 | 9.56 | 9.33 | 9.07 | 24.6 | yes |
| onset delay (ms) | 176 | 50.0 | 8 | 11.0 | 13.0 | 7.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | -1.85 | -0.71 | -3.68 | -3.44 | -2.12 | 0.6 |  |
| envelope curve, note for note (dB rms) | 41.9 (nearest 40.6) | fonts among themselves 11.9 | | | | 3.5 | yes |
| pitch re 12-TET A440 (median) (c) | -2.3 | 5 | 3.4 | -20.2 | 1.3 | 0.4 |  |
| pitch scatter from key to key (c) | 5.8 | 15.4 | 0.6 | 8.7 | 14.8 | 0.8 |  |
| noisiness (spectral flatness) (dB) | -35.4 | -39.9 | -47.0 | -51.0 | -32.8 | 0.9 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -55.5 | -128 | -123 | -124 | -108 | 6.8 | yes |
| pitch at the start re settled (c) | -5.2 | 10.7 | 2.5 | 0.8 | 1.7 | 1.5 | yes |
| stereo coherence | 0.113 | 0.9 | 1 | 1 | 0.289 | 2.4 | yes |
| partial-to-partial jaggedness (dB) | 3.1 | 1.9 | 3.4 | 2.35 | 1.4 | 1.0 |  |
| dip between legato notes (dB) | 5.5 | 10.0 | 5.8 | 7.3 | 11.2 | 1.2 |  |
| level evenness of a legato scale (dB) | 1.5 | 0.6 | 0.6 | 0.6 | 2.9 | 0.8 |  |
| level over the keyboard (dB/oct) | -1.46 | -1.34 | -2.95 | -1.62 | -1.12 | 0.0 |  |
| level against the other instruments (velocity 96) (dB) | -4.5 | 0.2 | -3.4 | -5.4 | -7.3 | 0.0 |  |
| key-to-key level scatter (dB) | 2.31 | 0.91 | 0.42 | 0.12 | 2.89 | 1.2 |  |
| odd / even partials (dB) | -3.5 | -5.6 | -6.3 | -4.7 | -8.4 | 1.2 | yes |
| onset jitter in a phrase (ms) | 50.0 | 20.2 | 1.5 | 1.3 | 0.3 | 4.9 | yes |
| onset in a phrase (re the note-on) (ms) | 72.0 | 1 | -1 | 0 | 0 | 7.2 | yes |
| onset sharpness (dB/ms) | 5 | 8.7 | 4.1 | 7.5 | 40.8 | 0.2 |  |
| overtone levels, note for note (dB rms) | 16.4 (nearest 15.4) | fonts among themselves 7.26 | | | | 2.3 | yes |
| panning across the keys (dB/oct) | 0.84 | 0.27 | -0 | 0 | -0 | 0.8 | yes |
| partial envelopes alike | 0.051 | 0.956 | 0.995 | 0.996 | 0.999 | 9.4 | yes |
| spectrogram (1 - correlation), note for note | 0.886 (nearest 0.833) | fonts among themselves 0.258 | | | | 3.4 | yes |
| spectral shape of the body, note for note | 13.7 (nearest 12) | fonts among themselves 7.48 | | | | 1.8 | yes |
| sounding length of a 100 ms note (ms) | 400 | 239 | 166 | 57.0 | 166 | 1.2 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.14 | 3.46 | 0 | 0 | 2.89 | 0.9 |  |
| level after 2 s held, re peak (dB) | -7.2 | -127 | -91.8 | -124 | -108 | 6.2 | yes |
| high bands lag low bands by (ms) | -11.1 | -0.7 | -2.2 | -2.9 | -4.7 | 1.7 | yes |
| transient coherence: attack spread over bands (ms) | 18.2 | 1.9 | 1.9 | 1.8 | 7.2 | 3.3 | yes |
| fundamental share (tristimulus 1) | 0.225 | 0.952 | 0.924 | 0.86 | 0.96 | 10.2 | yes |
| partials 2-4 share (tristimulus 2) | 0.417 | 0.046 | 0.07 | 0.139 | 0.039 | 5.1 | yes |
| partials 5+ share (tristimulus 3) | 0.157 | 0.002 | 0.002 | 0.002 | 0 | 2.2 | yes |
| loudness range, velocity 40 to 127 (dB) | 18.7 | 16.6 | 16.7 | 16.6 | 20.1 | 1.2 |  |
| pitch wobble / vibrato (p-p) (c) | 28.1 | 86.0 | 18.0 | 52.5 | 76.2 | 1.1 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0265 (nearest 0.0232) | fonts among themselves 0.00651 | | | | 4.1 | yes |

</details>

### 47 Orchestral Harp: harp

Distance 2.39 (rank 86 of 129); the fonts disagree 1.1 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Staccato 78 ms vs 311–400**: deliberate. A note let go within 0.1 s outside a quick passage is étouffé (README "Concert harp").
- **Uneven from key to key (4.8 dB vs 0.07–2.5)**: deliberate and known. The board's T1/A0 strings D3–F3 sound up to 14 dB over the middle (README: FluidR3 is within 5). It may be stronger than a real harp.
- **Starts 16 c sharp (vs −1…+3)**: deliberate in kind (tension modulation: README), but more than "a few cents". Worth a check at velocity 96.
- **More even partials (−9.9 vs −3…+0.3 dB)**: the phantom partials (README). **Purer**: the samples' noise, an artifact.
- **Onset 29 ms vs 4–10**: the pad's draw and slip. Slightly late, minor.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (10.6 ERB | 9.57 | 9.07 | 9.32 | 9.31), outside the fonts, 4.4 x.
2. **sounding length of a 100 ms note [room]**: shorter staccato (78.0 ms | 400 | 361 | 400 | 304), outside the fonts, 9.1 x.
3. **noisiness (spectral flatness)**: purer (-81.3 dB | -67.8 | -67.9 | -68.7 | -68.3), outside the fonts, 6.6 x.
4. **loudness range, velocity 40 to 127**: a narrower dynamic range (13.6 dB | 16.7 | 16.7 | 16.6 | 20.1), outside the fonts, 1.8 x.
5. **odd / even partials**: more even partials (-9.5 dB | -2.3 | 0.3 | -1.5 | -3.2), outside the fonts, 3.8 x.
6. **key-to-key brightness scatter**: less even in tone from key to key (1.66 ERB | 1.3 | 0.809 | 0.778 | 0.613), outside the fonts, 2.5 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +16 | +5 | +8 | +7 | -3 | -20 |
| re TimGM6mb | +2 | +5 | +11 | +4 | +5 | -24 |
| re Jnsgm2 | -10 | -4 | +1 | -5 | +8 | +25 |
| re SGM | +5 | -1 | -0 | -4 | +4 | -2 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -10 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening +0.3 ERB, loudness range +3 dB, envelope 5.5 dB rms; TimGM6mb: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 2.0 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 1.9 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.94 | -0.08 | -0.01 | 0.04 | 0.14 | 2.4 | yes |
| attack time (-20 to -1 dB) (ms) | 19.0 | 5.5 | 13.5 | 10.0 | 12.5 | 1.2 | yes |
| spectral shape of the attack, note for note | 16.9 (nearest 15.8) | fonts among themselves 8.65 | | | | 2.0 | yes |
| brightness over the keyboard (ERB/oct) | 2.34 | 2.39 | 2.52 | 2.75 | 2.13 | 0.4 |  |
| key-to-key brightness scatter (ERB) | 1.66 | 1.3 | 0.809 | 0.778 | 0.613 | 2.5 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.06 | 0 | 0 | 0 | 0.01 | 0.2 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 10.6 | 9.57 | 9.07 | 9.32 | 9.31 | 4.4 | yes |
| decay rate of a held note (dB/s) | 14.5 | 15.7 | 15.3 | 10.3 | 10.6 | 0.6 |  |
| onset delay (ms) | 29.0 | 4 | 9 | 10.0 | 6 | 2.1 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.34 | -5.51 | -5.14 | -7.38 | -6.64 | 2.4 | yes |
| envelope curve, note for note (dB rms) | 9.63 (nearest 7.6) | fonts among themselves 9.42 | | | | 1.0 |  |
| pitch re 12-TET A440 (median) (c) | 0.5 | 3.6 | 2.4 | 0.4 | -0.4 | 0.2 |  |
| pitch scatter from key to key (c) | 2.7 | 2.9 | 1 | 2.3 | 0.2 | 0.4 |  |
| noisiness (spectral flatness) (dB) | -81.3 | -67.8 | -67.9 | -68.7 | -68.3 | 6.6 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -83.2 | -67.8 | -126 | -122 | -74.2 | 0.5 |  |
| pitch at the start re settled (c) | 16.3 | -0.1 | 1.8 | 2.7 | -0.7 | 3.1 | yes |
| stereo coherence | 0.881 | 0.564 | 1 | 1 | 1 | 0.5 |  |
| partial-to-partial jaggedness (dB) | 6.7 | 6.2 | 4.4 | 4.85 | 5.2 | 1.7 |  |
| dip between legato notes (dB) | 12.1 | 9.2 | 4.4 | 10.2 | 7.4 | 1.3 | yes |
| level evenness of a legato scale (dB) | 1.8 | 0.8 | 0.8 | 1 | 2.9 | 0.9 |  |
| level over the keyboard (dB/oct) | -1.15 | -2.64 | -0.01 | 0.12 | -1.51 | 0.3 |  |
| level against the other instruments (velocity 96) (dB) | -0.2 | -2.4 | 3.1 | 1.9 | 1.6 | 0.7 |  |
| key-to-key level scatter (dB) | 3.33 | 2.48 | 0.07 | 0.39 | 1.63 | 1.9 | yes |
| odd / even partials (dB) | -9.5 | -2.3 | 0.3 | -1.5 | -3.2 | 3.8 | yes |
| onset jitter in a phrase (ms) | 18.3 | 27.1 | 2.3 | 28.3 | 1.7 | 0.3 |  |
| onset in a phrase (re the note-on) (ms) | 24.0 | 1 | 7 | 8 | 4 | 1.9 | yes |
| onset sharpness (dB/ms) | 8.6 | 15.2 | 17.7 | 13.8 | 20.6 | 2.3 | yes |
| overtone levels, note for note (dB rms) | 12.6 (nearest 11.9) | fonts among themselves 14.2 | | | | 0.9 |  |
| panning across the keys (dB/oct) | 0.07 | 0 | -0 | 0 | -0.01 | 0.1 |  |
| partial envelopes alike | 0.96 | 0.878 | 1 | 0.741 | 0.957 | 0.3 |  |
| release time (30 dB after note-off) (ms) | 406 | 1878 | 649 | 665 | 1088 | 1.4 | yes |
| spectrogram (1 - correlation), note for note | 0.282 (nearest 0.259) | fonts among themselves 0.203 | | | | 1.4 |  |
| spectral shape of the body, note for note | 13.1 (nearest 12.3) | fonts among themselves 10.5 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 78.0 | 400 | 361 | 400 | 304 | 9.1 | yes |
| partial stretch (inharmonicity) (c/partial) | 1.62 | 1.57 | 0 | 1.99 | 0.07 | 0.8 |  |
| level after 2 s held, re peak (dB) | -35.0 | -40.8 | -33.5 | -23.0 | -32.8 | 0.2 |  |
| high bands lag low bands by (ms) | -1 | 0 | 0 | -1.1 | 0 | 0.2 |  |
| transient coherence: attack spread over bands (ms) | 1.6 | 0.1 | 0.3 | 1.1 | 1 | 0.2 |  |
| fundamental share (tristimulus 1) | 0.914 | 0.764 | 0.991 | 0.982 | 0.977 | 0.6 |  |
| partials 2-4 share (tristimulus 2) | 0.082 | 0.237 | 0.006 | 0.013 | 0.023 | 0.6 |  |
| partials 5+ share (tristimulus 3) | 0.001 | 0 | 0.003 | 0.004 | 0 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 13.6 | 16.7 | 16.7 | 16.6 | 20.1 | 1.8 | yes |
| pitch wobble / vibrato (p-p) (c) | 10.2 | 2 | 0 | 2.2 | 4.8 | 1.6 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0161 (nearest 0.013) | fonts among themselves 0.00886 | | | | 1.8 | yes |

</details>

### 48 Timpani: timpani

Distance 1.93 (rank 106 of 129); the fonts disagree 3.3 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Wider dynamics (26.4 vs 16.6…20.1 dB; −20/+7 dB at v40/127)**: DELIBERATE: the felt mallet's F = K x^2.54 (README timpani). The fonts' range is TiMidity's law.
- **Louder (+7.7 vs −12.4…+1.2 dB re the other instruments)**: a trim of ~5 dB. The fonts disagree widely.
- **Flat over the keys (1.67 vs 3.7…4.1 ERB/oct)**: MEASUREMENT ARTIFACT. Probe keys 68–96 lie above the five drums' C2–E4 and go to the outer drums (README).
- **Speaks 25 ms late (21 ms in a phrase vs 0)**: possible FAULT. It is likely the thrown mallet's flight. Timpanists play ahead of the beat, but GM files don't, so consider starting the throw earlier.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a wider dynamic range (24.6 dB | 17.2 | 16.6 | 16.7 | 20.1), outside the fonts, 4.4 x.
2. **brightness over the keyboard**: brightens slower up the keyboard (1.63 ERB/oct | 3.81 | 4.1 | 4.09 | 3.73), outside the fonts, 7.7 x.
3. **onset delay**: speaks later (25.0 ms | 3 | 2 | 0 | 6), outside the fonts, 2.2 x.
4. **onset in a phrase (re the note-on)**: speaks later in a phrase (21.0 ms | 0 | 0 | -2 | 0), outside the fonts, 2.1 x.
5. **onset sharpness**: a sharper onset (55.9 dB/ms | 14.5 | 24.2 | 1.9 | 26.0), outside the fonts, 3.0 x.
6. **brightness (spectral centroid, velocity 96)**: darker (7.16 ERB | 12.7 | 11.8 | 12.2 | 6.68), inside the fonts' range, 1.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +28 | +28 | +20 | +1 | -14 | -28 |
| re TimGM6mb | +26 | +28 | +19 | +3 | -10 | -30 |
| re Jnsgm2 | +21 | +22 | +15 | -5 | -15 | -24 |
| re SGM | +4 | +5 | -0 | -2 | -5 | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -19 -13 +0 +5; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.3 ERB, attack -0.6 ERB, velocity brightening +2.2 ERB, loudness range +5 dB, envelope 4.0 dB rms; TimGM6mb: centroid -0.3 ERB, attack -0.0 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 2.4 dB rms; Jnsgm2: centroid -1.4 ERB, attack -0.3 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 1.7 dB rms
- silent probe notes: SGM 23, Jnsgm2/fs 7

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.34 | 1.83 | -0.47 | -0.21 | -0.3 | 0.1 |  |
| attack time (-20 to -1 dB) (ms) | 7.5 | 6 | 3 | 6.5 | 5 | 0.8 | yes |
| spectral shape of the attack, note for note | 22.5 (nearest 12.3) | fonts among themselves 9.68 | | | | 2.3 |  |
| brightness over the keyboard (ERB/oct) | 1.63 | 3.81 | 4.1 | 4.09 | 3.73 | 7.7 | yes |
| key-to-key brightness scatter (ERB) | 0.715 | 0.629 | 0.164 | 0.19 | 0.549 | 1.5 |  |
| brightening from velocity 40 to 127 (ERB) | 0.29 | 0.53 | -0 | -0 | -0.04 | 1.0 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 7.16 | 12.7 | 11.8 | 12.2 | 6.68 | 1.6 |  |
| decay rate of a held note (dB/s) | 28.1 | 59.0 | 19.5 | 13.5 | 15.8 | 0.6 |  |
| onset delay (ms) | 25.0 | 3 | 2 | 0 | 6 | 2.2 | yes |
| Plomp-Levelt dissonance of a note (log2) | -0.89 | -0.33 | -3.36 | -3.12 | -2.72 | 1.3 |  |
| envelope curve, note for note (dB rms) | 13.9 (nearest 9.2) | fonts among themselves 16 | | | | 0.9 |  |
| pitch re 12-TET A440 (median) (c) | 2.3 | 36.3 | -14.7 | -4 | -24.6 | 0.4 |  |
| pitch scatter from key to key (c) | 12.6 | 19.9 | 1 | 9.6 | 5.6 | 0.5 |  |
| noisiness (spectral flatness) (dB) | -62.0 | -22.8 | -48.0 | -45.2 | -50.2 | 1.1 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -117 | -117 | -120 | -93.0 | -82.6 | 0.6 |  |
| pitch at the start re settled (c) | -1.6 | -3.1 | 8.2 | 33.1 | 10.8 | 0.6 |  |
| stereo coherence | 0.445 | 0.453 | 1 | 1 | 0.857 | 1.8 |  |
| partial-to-partial jaggedness (dB) | 4.45 | 2 | 3.6 | 3.6 | 3 | 1.2 | yes |
| dip between legato notes (dB) | 11.7 | 7.7 | 6 | 9.9 | 13.9 | 0.7 |  |
| level evenness of a legato scale (dB) | 1 | 0.5 | 0.6 | 1 | 2.1 | 0.2 |  |
| level over the keyboard (dB/oct) | -1.42 | -4.37 | -2.2 | 0.11 | -7.83 | 0.5 |  |
| level against the other instruments (velocity 96) (dB) | 0.9 | -11.9 | -4.8 | 1.4 | -1.7 | 0.6 |  |
| key-to-key level scatter (dB) | 2.16 | 1.01 | 0.19 | 0.25 | 6.29 | 0.5 |  |
| odd / even partials (dB) | -14.2 | -5.5 | 1.5 | -12.5 | -12.1 | 0.8 | yes |
| onset jitter in a phrase (ms) | 19.2 | 0.8 | 1 | 50.1 | 1.3 | 0.7 |  |
| onset in a phrase (re the note-on) (ms) | 21.0 | 0 | 0 | -2 | 0 | 2.1 | yes |
| onset sharpness (dB/ms) | 55.9 | 14.5 | 24.2 | 1.9 | 26.0 | 3.0 | yes |
| overtone levels, note for note (dB rms) | 20.9 (nearest 13.5) | fonts among themselves 11.1 | | | | 1.9 |  |
| panning across the keys (dB/oct) | 0.2 | -0.75 | 0 | 0 | -0.25 | 0.3 |  |
| partial envelopes alike | 0.917 | 0.997 | 0.994 | 0.879 | 0.86 | 0.2 |  |
| release time (30 dB after note-off) (ms) | 610 | 566 | 716 | 1143 | 675 | 0.4 |  |
| spectrogram (1 - correlation), note for note | 0.642 (nearest 0.28) | fonts among themselves 0.286 | | | | 2.2 |  |
| spectral shape of the body, note for note | 19.7 (nearest 12.3) | fonts among themselves 10.6 | | | | 1.9 |  |
| sounding length of a 100 ms note (ms) | 44.0 | 105 | 71.0 | 126 | 15.0 | 0.6 |  |
| partial stretch (inharmonicity) (c/partial) | -1.79 | 0.85 | 0.23 | 1.4 | -3.76 | 0.9 |  |
| level after 2 s held, re peak (dB) | -64.6 | -110 | -52.5 | -31.1 | -47.7 | 0.4 |  |
| high bands lag low bands by (ms) | -1 | 0 | -0.8 | 0.2 | 0 | 0.2 |  |
| transient coherence: attack spread over bands (ms) | 0.5 | 0.1 | 1.1 | 3.2 | 1 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.98 | 0.816 | 0.715 | 0.018 | 0.851 | 0.5 | yes |
| partials 2-4 share (tristimulus 2) | 0.019 | 0.152 | 0.19 | 0.978 | 0.107 | 0.3 | yes |
| partials 5+ share (tristimulus 3) | 0 | 0.021 | 0.096 | 0.005 | 0.002 | 0.2 |  |
| loudness range, velocity 40 to 127 (dB) | 24.6 | 17.2 | 16.6 | 16.7 | 20.1 | 4.4 | yes |
| pitch wobble / vibrato (p-p) (c) | 59.5 | 78.4 | 3.6 | 22.1 | 31.4 | 0.9 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0255 (nearest 0.0107) | fonts among themselves 0.0137 | | | | 1.9 |  |

</details>

### 49 String Ensemble 1: strings

Distance 3.01 (rank 49 of 129); the fonts disagree 1.2 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Brighter with a weak fundamental (17.6 vs 11.8–13.5 ERB; +4…+9 dB at 1–2.5 kHz; tristimulus-1 0.22 vs 0.55–0.67)**: probable fault. Recorded sections heard from the hall are darker, and the fonts agree tightly. "Bow nearer the bridge" (README string ensembles) is chosen, but 4–5 ERB is too much.
- **Louder (+4.9 vs −5.2…−0.6 dB), 8 dB over Ensemble 2**: a fault for single notes. A lone key goes to every section whose register covers it, but the `PJ_STRINGS_GAIN` trim was set on a chord.
- **Uneven (2.7 dB; −2.5 dB/oct)**: the number of sections on a key changes across the register. Deliberate dealing, but it is audible as steps.
- **Darkens with velocity (−1.6 ERB)**: the bow-bandwidth fault (Viola.md §2).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (17.3 ERB | 12.6 | 11.8 | 13.1 | 13.6), outside the fonts, 5.1 x.
2. **brightening from velocity 40 to 127**: brightens less with velocity (-1.52 ERB | -0.03 | 0 | 0 | -0.21), outside the fonts, 5.0 x.
3. **key-to-key brightness scatter**: less even in tone from key to key (2.3 ERB | 1.22 | 0.753 | 0.964 | 1.01), outside the fonts, 5.6 x.
4. **fundamental share (tristimulus 1)**: less fundamental (0.217 | 0.569 | 0.672 | 0.549 | 0.597), outside the fonts, 5.2 x.
5. **key-to-key level scatter**: less even from key to key (2.57 dB | 1.42 | 0.99 | 1.08 | 1.76), outside the fonts, 2.6 x.
6. **release time (30 dB after note-off) [room]**: rings longer after the key (890 ms | 442 | 648 | 358 | 372), outside the fonts, 2.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -4 | +2 | +4 | +8 | +5 | -15 |
| re TimGM6mb | -10 | -1 | +1 | +4 | +2 | -5 |
| re Jnsgm2 | -5 | +4 | +5 | +9 | +4 | -18 |
| re SGM | -5 | +4 | +4 | +8 | +4 | -14 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -14 -7 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 1.6 dB rms; TimGM6mb: centroid -0.6 ERB, attack +0.4 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 3.8 dB rms; Jnsgm2: centroid -0.0 ERB, attack +0.1 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 1.7 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.32 | 0.78 | -0.08 | -0.59 | 0.29 | 0.3 |  |
| attack time (-20 to -1 dB) (ms) | 728 | 637 | 408 | 132 | 406 | 0.7 | yes |
| spectral shape of the attack, note for note | 9.98 (nearest 9.41) | fonts among themselves 7.73 | | | | 1.3 |  |
| brightness over the keyboard (ERB/oct) | 3.66 | 3.5 | 3.71 | 3.93 | 3.45 | 0.2 |  |
| key-to-key brightness scatter (ERB) | 2.3 | 1.22 | 0.753 | 0.964 | 1.01 | 5.6 | yes |
| brightening from velocity 40 to 127 (ERB) | -1.52 | -0.03 | 0 | 0 | -0.21 | 5.0 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 17.3 | 12.6 | 11.8 | 13.1 | 13.6 | 5.1 | yes |
| decay rate of a held note (dB/s) | 0.3 | 1.3 | 0.6 | 0.1 | 0.8 | 0.7 |  |
| onset delay (ms) | 187 | 112 | 130 | 55.0 | 79.0 | 2.4 | yes |
| Plomp-Levelt dissonance of a note (log2) | -1.97 | -2.35 | -1.98 | -1.71 | -2.27 | 0.5 |  |
| envelope curve, note for note (dB rms) | 14.2 (nearest 8.08) | fonts among themselves 6.55 | | | | 2.2 |  |
| pitch re 12-TET A440 (median) (c) | -2 | -1.4 | -4.8 | 5.9 | 9.7 | 0.6 |  |
| pitch scatter from key to key (c) | 6.2 | 13.6 | 6.7 | 14.7 | 9.6 | 1.4 |  |
| noisiness (spectral flatness) (dB) | -35.2 | -33.4 | -34.3 | -26.8 | -32.9 | 0.5 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -54.3 | -126 | -94.8 | -120 | -106 | 3.8 | yes |
| pitch at the start re settled (c) | -7 | -2.6 | 3.3 | 1.2 | -2 | 1.3 | yes |
| stereo coherence | 0.138 | 0.216 | 0.399 | 1 | 0.18 | 0.4 | yes |
| partial-to-partial jaggedness (dB) | 3.1 | 2.3 | 2.2 | 2 | 1.7 | 1.0 | yes |
| dip between legato notes (dB) | 7.7 | 11.1 | 6.5 | 8.1 | 10.7 | 0.7 |  |
| level evenness of a legato scale (dB) | 1.1 | 0.6 | 2.3 | 0.5 | 2.4 | 0.3 |  |
| level over the keyboard (dB/oct) | -1.3 | 0.45 | -0.06 | -0.91 | -0.92 | 0.8 |  |
| level against the other instruments (velocity 96) (dB) | -1 | -0.1 | -2.7 | -2.1 | -4 | 0.7 |  |
| key-to-key level scatter (dB) | 2.57 | 1.42 | 0.99 | 1.08 | 1.76 | 2.6 | yes |
| odd / even partials (dB) | -2.8 | -10.7 | -5.2 | -7.2 | -4.1 | 1.0 | yes |
| onset jitter in a phrase (ms) | 47.9 | 42.5 | 56.5 | 45.4 | 40.4 | 0.5 |  |
| onset in a phrase (re the note-on) (ms) | 60.0 | 1 | 49.0 | 1 | 57.0 | 1.2 |  |
| onset sharpness (dB/ms) | 5.2 | 5.5 | 5.3 | 5.6 | 5.2 | 0.1 |  |
| overtone levels, note for note (dB rms) | 9.48 (nearest 8.65) | fonts among themselves 6.51 | | | | 1.5 | yes |
| panning across the keys (dB/oct) | 0.87 | 0.24 | -0.53 | 0 | -0 | 0.9 | yes |
| partial envelopes alike | 0.05 | 0.036 | 0.071 | 0.003 | 0.058 | 0.0 |  |
| release time (30 dB after note-off) (ms) | 890 | 442 | 648 | 358 | 372 | 2.6 | yes |
| spectrogram (1 - correlation), note for note | 0.167 (nearest 0.152) | fonts among themselves 0.0927 | | | | 1.8 | yes |
| spectral shape of the body, note for note | 8.86 (nearest 8.55) | fonts among themselves 6.13 | | | | 1.4 | yes |
| sounding length of a 100 ms note (ms) | 341 | 152 | 381 | 192 | 337 | 0.6 |  |
| partial stretch (inharmonicity) (c/partial) | 0.07 | -0.05 | 0.26 | 0.04 | 0.14 | 0.0 |  |
| level after 2 s held, re peak (dB) | -6.8 | -8.3 | -10.7 | -8.2 | -8.1 | 0.5 |  |
| high bands lag low bands by (ms) | 0.4 | -3.6 | -13.5 | 2.1 | -4.2 | 0.6 |  |
| transient coherence: attack spread over bands (ms) | 19.9 | 15.1 | 19.3 | 10.2 | 12.6 | 1.2 |  |
| fundamental share (tristimulus 1) | 0.217 | 0.569 | 0.672 | 0.549 | 0.597 | 5.2 | yes |
| partials 2-4 share (tristimulus 2) | 0.415 | 0.399 | 0.299 | 0.409 | 0.213 | 0.7 |  |
| partials 5+ share (tristimulus 3) | 0.183 | 0.016 | 0.026 | 0.033 | 0.068 | 2.2 | yes |
| loudness range, velocity 40 to 127 (dB) | 17.3 | 16.8 | 16.6 | 16.6 | 20.1 | 0.3 |  |
| pitch wobble / vibrato (p-p) (c) | 29.6 | 24.4 | 27.2 | 24.9 | 22.7 | 1.0 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0206 (nearest 0.0179) | fonts among themselves 0.0172 | | | | 1.2 |  |

</details>

### 50 String Ensemble 2: slow-strings

Distance 3.53 (rank 32 of 129); the fonts disagree 1.2 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Darkens with velocity (−2.1 ERB) and brighter (17.3 vs 11.7–13.5 ERB)**: as p048, a fault. Even with brightness 0.7 (toward the fingerboard), it is as bright as Ensemble 1.
- **Uneven from key to key (3.8 dB vs 0.9–1.7)**: fault-ish, the section register overlap as in p048.
- **Late (onset 404 ms vs 202–270; 104 ms in a phrase vs 22–53)**: deliberate. The players come in one by one over 300 ms (README string ensembles), slower than the fonts' slow attack.
- Release and staccato are inside the fonts' range, the level too (−3.2 dB).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens less with velocity (-2.09 ERB | -0.03 | -0 | 0 | -0.21), outside the fonts, 6.9 x.
2. **brightness (spectral centroid, velocity 96)**: brighter (17.1 ERB | 12.7 | 11.7 | 13.1 | 13.5), outside the fonts, 4.6 x.
3. **key-to-key level scatter**: less even from key to key (3.86 dB | 1.67 | 1.04 | 0.87 | 1.75), outside the fonts, 5.0 x.
4. **onset delay**: speaks later (411 ms | 270 | 248 | 202 | 248), outside the fonts, 4.8 x.
5. **onset in a phrase (re the note-on)**: speaks later in a phrase (106 ms | 46.0 | 34.0 | 22.0 | 50.0), outside the fonts, 4.7 x.
6. **fundamental share (tristimulus 1)**: less fundamental (0.237 | 0.558 | 0.73 | 0.556 | 0.6), outside the fonts, 3.9 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -7 | +3 | +4 | +9 | +6 | -14 |
| re TimGM6mb | -13 | +0 | +1 | +6 | +4 | -8 |
| re Jnsgm2 | -9 | +6 | +5 | +10 | +5 | -18 |
| re SGM | -9 | +5 | +4 | +8 | +5 | -14 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -9 -8 +0 +6; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack -0.2 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 1.1 dB rms; TimGM6mb: centroid -0.7 ERB, attack -0.2 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 4.0 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 0.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -1.06 | 1 | -0.8 | -0.37 | 0.94 | 1.5 | yes |
| attack time (-20 to -1 dB) (ms) | 758 | 704 | 613 | 572 | 467 | 1.2 |  |
| spectral shape of the attack, note for note | 11.6 (nearest 11.2) | fonts among themselves 8.13 | | | | 1.4 | yes |
| brightness over the keyboard (ERB/oct) | 3.12 | 3.51 | 3.49 | 3.91 | 3.43 | 1.3 | yes |
| key-to-key brightness scatter (ERB) | 1.88 | 1.25 | 0.692 | 0.948 | 1.01 | 3.2 | yes |
| brightening from velocity 40 to 127 (ERB) | -2.09 | -0.03 | -0 | 0 | -0.21 | 6.9 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 17.1 | 12.7 | 11.7 | 13.1 | 13.5 | 4.6 | yes |
| onset delay (ms) | 411 | 270 | 248 | 202 | 248 | 4.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | -1.64 | -2.44 | -2.13 | -1.85 | -2.27 | 1.9 | yes |
| envelope curve, note for note (dB rms) | 7.31 (nearest 6.3) | fonts among themselves 6.48 | | | | 1.1 |  |
| pitch re 12-TET A440 (median) (c) | -2.6 | 0.7 | -2.5 | 2.3 | 12.6 | 0.5 |  |
| pitch scatter from key to key (c) | 6 | 11.6 | 5.9 | 10.9 | 8.3 | 1.2 |  |
| noisiness (spectral flatness) (dB) | -31.9 | -33.2 | -42.2 | -26.6 | -33.1 | 0.2 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -49.8 | -46.0 | -93.7 | -57.7 | -60.6 | 0.4 |  |
| pitch at the start re settled (c) | -5.5 | -0.4 | 3.6 | 2.2 | -2.1 | 1.3 | yes |
| stereo coherence | 0.081 | 0.248 | 0.377 | 1 | 0.148 | 0.5 | yes |
| partial-to-partial jaggedness (dB) | 3.2 | 2.3 | 2.15 | 2.05 | 1.7 | 1.1 | yes |
| dip between legato notes (dB) | 8.5 | 10.5 | 6.7 | 10.1 | 9.2 | 0.6 |  |
| level evenness of a legato scale (dB) | 4.7 | 0.4 | 2.2 | 2.2 | 2.5 | 2.4 | yes |
| level over the keyboard (dB/oct) | -1.09 | 0.27 | 0.47 | -0.32 | -0.63 | 1.1 |  |
| level against the other instruments (velocity 96) (dB) | -5.5 | -1.6 | -5.2 | -5.6 | -5.3 | 0.1 |  |
| key-to-key level scatter (dB) | 3.86 | 1.67 | 1.04 | 0.87 | 1.75 | 5.0 | yes |
| odd / even partials (dB) | -3.8 | -10.9 | -8.5 | -7 | -4.2 | 1.2 |  |
| onset jitter in a phrase (ms) | 56.3 | 60.9 | 54.7 | 54.5 | 51.1 | 0.3 |  |
| onset in a phrase (re the note-on) (ms) | 106 | 46.0 | 34.0 | 22.0 | 50.0 | 4.7 | yes |
| onset sharpness (dB/ms) | 4.8 | 5.5 | 5.8 | 4.5 | 3.6 | 0.1 |  |
| overtone levels, note for note (dB rms) | 10.5 (nearest 9.97) | fonts among themselves 6.67 | | | | 1.6 | yes |
| panning across the keys (dB/oct) | 0.96 | 0.29 | 0.03 | 0 | -0.09 | 0.9 | yes |
| partial envelopes alike | 0.325 | 0.272 | 0.028 | 0.281 | 0.181 | 0.8 |  |
| release time (30 dB after note-off) (ms) | 1070 | 1017 | 649 | 831 | 788 | 1.2 |  |
| spectrogram (1 - correlation), note for note | 0.155 (nearest 0.133) | fonts among themselves 0.0955 | | | | 1.6 | yes |
| spectral shape of the body, note for note | 9.7 (nearest 8.9) | fonts among themselves 6.88 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 400 | 398 | 350 | 212 | 400 | 0.2 |  |
| partial stretch (inharmonicity) (c/partial) | 0.43 | -0.07 | 0.15 | 0.02 | 0.23 | 0.7 |  |
| level after 2 s held, re peak (dB) | -6.8 | -7.5 | -9.4 | -7 | -8.1 | 0.3 |  |
| high bands lag low bands by (ms) | -8.1 | -1.1 | 14.8 | -6.2 | 4 | 0.9 |  |
| transient coherence: attack spread over bands (ms) | 19.9 | 17.0 | 20.3 | 20.9 | 22.4 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.237 | 0.558 | 0.73 | 0.556 | 0.6 | 3.9 | yes |
| partials 2-4 share (tristimulus 2) | 0.422 | 0.409 | 0.248 | 0.402 | 0.216 | 1.0 |  |
| partials 5+ share (tristimulus 3) | 0.138 | 0.016 | 0.014 | 0.035 | 0.068 | 1.6 | yes |
| loudness range, velocity 40 to 127 (dB) | 15.8 | 16.6 | 16.7 | 16.7 | 20.1 | 0.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 28.9 | 24.8 | 27.1 | 25.4 | 22.5 | 0.8 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0206 (nearest 0.0198) | fonts among themselves 0.0179 | | | | 1.2 |  |

</details>

### 51 Synth Strings 1: synth-strings-1

Distance 1.69 (rank 115 of 129); the fonts disagree 1.4 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Late in a phrase (106 ms vs 1–18) and a slow onset (301 ms vs 22–166)**: deliberate (`ampA` 0.28 s, "a slow bow": synth_leads.md table), but slower than every font, so legato lines go soft. Consider a shorter attack.
- **Dark top (−18…−23 dB above 6 kHz; +8…+15 dB below 2.5 kHz)**: `cutoff` 0 by choice. It loses the sheen. The centroid is inside the fonts' range (11.2 vs 10.2–13.7).
- **Uneven from key to key (2.6 dB vs 0.05–1.7)**: likely an artifact. The two cores 12 c apart beat slowly at low keys inside the measuring window.
- Rank 68: close overall. The release comes from the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **onset in a phrase (re the note-on)**: speaks later in a phrase (92.0 ms | 1 | 1 | 10.0 | 2), outside the fonts, 9.1 x.
2. **key-to-key level scatter**: less even from key to key (2.55 dB | 0.05 | 0.9 | 0.56 | 1.72), outside the fonts, 2.2 x.
3. **onset delay**: speaks later (301 ms | 22.0 | 166 | 148 | 116), outside the fonts, 2.3 x.
4. **brightening from velocity 40 to 127**: brightens more with velocity (0.52 ERB | -0 | 0 | -0 | -0.25), outside the fonts, 1.7 x.
5. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.211, nearest 0.164; the fonts among themselves 0.0877), outside.
6. **release time (30 dB after note-off) [room]**: rings longer after the key (970 ms | 730 | 882 | 418 | 430), outside the fonts, 1.5 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +8 | +10 | +10 | +10 | -2 | -22 |
| re TimGM6mb | +15 | +11 | +10 | +8 | -5 | -23 |
| re Jnsgm2 | +10 | +11 | +9 | +6 | -5 | -18 |
| re SGM | +9 | +11 | +11 | +7 | -3 | -20 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -14 -5 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.1 ERB, attack +0.1 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 3.9 dB rms; TimGM6mb: centroid -0.5 ERB, attack +0.7 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 3.5 dB rms; Jnsgm2: centroid -0.9 ERB, attack -0.9 ERB, velocity brightening +0.4 ERB, loudness range +4 dB, envelope 1.7 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.31 | -0.01 | -0.96 | 0.9 | 0.55 | 0.6 |  |
| attack time (-20 to -1 dB) (ms) | 670 | 34.0 | 418 | 378 | 280 | 0.6 | yes |
| spectral shape of the attack, note for note | 12.3 (nearest 10.9) | fonts among themselves 6.82 | | | | 1.8 | yes |
| brightness over the keyboard (ERB/oct) | 3.21 | 3.62 | 3.68 | 3.55 | 3.9 | 1.5 | yes |
| key-to-key brightness scatter (ERB) | 1.02 | 0.167 | 0.738 | 0.857 | 0.912 | 0.6 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.52 | -0 | 0 | -0 | -0.25 | 1.7 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 11.2 | 10.2 | 12.4 | 13.7 | 11.1 | 0.4 |  |
| onset delay (ms) | 301 | 22.0 | 166 | 148 | 116 | 2.3 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.82 | -6.8 | -2.43 | -1.7 | -3.63 | 0.7 |  |
| envelope curve, note for note (dB rms) | 12.6 (nearest 6.47) | fonts among themselves 12.6 | | | | 1.0 |  |
| pitch re 12-TET A440 (median) (c) | -5.7 | -0.3 | 0.7 | -1.9 | -7.4 | 1.1 |  |
| pitch scatter from key to key (c) | 3.6 | 6.4 | 3.9 | 2.1 | 7 | 0.5 |  |
| noisiness (spectral flatness) (dB) | -61.5 | -66.7 | -40.7 | -36.2 | -48.3 | 1.1 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -41.2 | -88.5 | -51.8 | -122 | -106 | 1.6 | yes |
| pitch at the start re settled (c) | 1.2 | -1.1 | 1.6 | 1.6 | 1.3 | 0.1 |  |
| stereo coherence | 0.786 | 1 | 0.117 | 1 | 0.259 | 0.4 |  |
| partial-to-partial jaggedness (dB) | 2.6 | 0.2 | 1.55 | 1.05 | 1.3 | 1.4 | yes |
| dip between legato notes (dB) | 12.7 | 10.5 | 9.7 | 8.3 | 11.6 | 1.3 | yes |
| level evenness of a legato scale (dB) | 1.4 | 1.9 | 1.7 | 0.4 | 1.3 | 0.1 |  |
| level over the keyboard (dB/oct) | 0.83 | -0.09 | 1.39 | -0.07 | -0.84 | 0.8 |  |
| level against the other instruments (velocity 96) (dB) | -4.2 | 3.1 | -5.9 | -0.9 | -1.3 | 0.7 |  |
| key-to-key level scatter (dB) | 2.55 | 0.05 | 0.9 | 0.56 | 1.72 | 2.2 | yes |
| odd / even partials (dB) | -6.2 | -4.8 | -5.2 | -3.7 | -5.5 | 0.6 |  |
| onset jitter in a phrase (ms) | 50.5 | 36.7 | 39.7 | 47.9 | 34.3 | 1.8 | yes |
| onset in a phrase (re the note-on) (ms) | 92.0 | 1 | 1 | 10.0 | 2 | 9.1 | yes |
| onset sharpness (dB/ms) | 6.4 | 5.2 | 6.3 | 4 | 4.7 | 0.7 |  |
| overtone levels, note for note (dB rms) | 11.5 (nearest 10.7) | fonts among themselves 4.56 | | | | 2.5 | yes |
| panning across the keys (dB/oct) | 0.19 | 0 | -0.1 | 0 | 0.52 | 0.2 |  |
| partial envelopes alike | 0.228 | 1 | 0.045 | -0.02 | 0.029 | 0.4 |  |
| release time (30 dB after note-off) (ms) | 970 | 730 | 882 | 418 | 430 | 1.5 | yes |
| spectrogram (1 - correlation), note for note | 0.211 (nearest 0.164) | fonts among themselves 0.0877 | | | | 2.4 | yes |
| spectral shape of the body, note for note | 10.8 (nearest 8.94) | fonts among themselves 8.64 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 400 | 375 | 400 | 377 | 328 | 0.4 |  |
| partial stretch (inharmonicity) (c/partial) | 0.04 | 0 | 0.04 | 0.07 | 0.01 | 0.0 |  |
| level after 2 s held, re peak (dB) | -7 | -2 | -6.9 | -6.7 | -10.1 | 0.0 |  |
| high bands lag low bands by (ms) | 17.0 | 0 | -28.0 | 4 | 7 | 0.9 | yes |
| transient coherence: attack spread over bands (ms) | 16.1 | 0 | 38.9 | 23.9 | 21.5 | 0.3 |  |
| fundamental share (tristimulus 1) | 0.919 | 0.813 | 0.528 | 0.804 | 0.845 | 0.7 | yes |
| partials 2-4 share (tristimulus 2) | 0.074 | 0.171 | 0.453 | 0.18 | 0.144 | 0.7 | yes |
| partials 5+ share (tristimulus 3) | 0.002 | 0.015 | 0.025 | 0.025 | 0.012 | 0.3 |  |
| loudness range, velocity 40 to 127 (dB) | 18.3 | 16.7 | 16.6 | 16.6 | 20.1 | 0.9 |  |
| pitch wobble / vibrato (p-p) (c) | 18.2 | 1.9 | 19.8 | 27.6 | 20.2 | 0.1 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0231 (nearest 0.0124) | fonts among themselves 0.0148 | | | | 1.6 |  |

</details>

### 52 Synth Strings 2: synth-strings-2

Distance 1.30 (rank 129 of 129); the fonts disagree 1.0 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96, velocities 40/64/96/127.

- **Brightens with velocity (+1.1 ERB vs 0)**: deliberate (velocity to filter). The fonts' 0 is TiMidity. Under FluidSynth Jnsgm2 moves +0.5.
- **Quieter (−3 vs −1.6…+1.5 dB)**: small level trim.
- **Pitch scatter 9.9 c vs 4–6**: deliberate. The cores are 20 c apart with the chorus (synth_leads.md table).
- **Slightly brighter (13.0 vs 10.8–12.5 ERB)** and a slower attack (460 vs 138–407 ms): minor. Rank 69: the closest of all programs.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (1.06 ERB | 0 | 0 | -0 | 0.09), outside the fonts, 3.5 x.
2. **brightness (spectral centroid, velocity 96)**: brighter (13.0 ERB | 10.8 | 12.3 | 12.5 | 11.2), outside the fonts, 1.4 x.
3. **pitch scatter from key to key**: less evenly tuned (9.9 c | 4 | 5.9 | 4.5 | 6.2), outside the fonts, 1.6 x.
4. **attack time (-20 to -1 dB)**: a slower attack (460 ms | 138 | 407 | 383 | 150), outside the fonts, 1.2 x.
5. **odd / even partials**: more even partials (-7.1 dB | -4.7 | -3 | -3.8 | -3.7), outside the fonts, 1.7 x.
6. **level over the keyboard**: louder toward the top (0.56 dB/oct | -0.98 | -0.18 | -0.36 | -0.88), outside the fonts, 1.2 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -0 | +4 | +7 | +5 | +2 | -13 |
| re TimGM6mb | -2 | +2 | +0 | +1 | -2 | +1 |
| re Jnsgm2 | +1 | +6 | +4 | +5 | -0 | -11 |
| re SGM | +0 | +5 | +7 | +3 | -2 | -9 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack -0.0 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 2.3 dB rms; TimGM6mb: centroid +1.1 ERB, attack +0.1 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 4.4 dB rms; Jnsgm2: centroid +0.6 ERB, attack +0.8 ERB, velocity brightening +0.5 ERB, loudness range +4 dB, envelope 5.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.21 | -0.15 | 0.02 | -0.07 | 0.67 | 0.5 |  |
| attack time (-20 to -1 dB) (ms) | 460 | 138 | 407 | 383 | 150 | 1.2 | yes |
| spectral shape of the attack, note for note | 10.8 (nearest 9.27) | fonts among themselves 11.3 | | | | 1.0 |  |
| brightness over the keyboard (ERB/oct) | 3.27 | 4.09 | 3.61 | 3.6 | 3.88 | 1.6 | yes |
| key-to-key brightness scatter (ERB) | 1.04 | 0.419 | 1.04 | 1.2 | 1.56 | 0.1 |  |
| brightening from velocity 40 to 127 (ERB) | 1.06 | 0 | 0 | -0 | 0.09 | 3.5 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 13.0 | 10.8 | 12.3 | 12.5 | 11.2 | 1.4 | yes |
| onset delay (ms) | 150 | 58.0 | 156 | 146 | 33.0 | 0.8 |  |
| Plomp-Levelt dissonance of a note (log2) | -2.68 | -4.01 | -2.06 | -2.14 | -3.32 | 0.1 |  |
| envelope curve, note for note (dB rms) | 6.51 (nearest 5.77) | fonts among themselves 4.99 | | | | 1.3 |  |
| pitch re 12-TET A440 (median) (c) | -4.2 | -1.9 | -3.1 | -1.5 | -1.5 | 0.6 |  |
| pitch scatter from key to key (c) | 9.9 | 4 | 5.9 | 4.5 | 6.2 | 1.6 | yes |
| noisiness (spectral flatness) (dB) | -50.3 | -61.0 | -37.4 | -37.2 | -49.1 | 0.6 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -54.8 | -118 | -75.1 | -74.8 | -62.2 | 0.7 | yes |
| pitch at the start re settled (c) | 0.9 | 0.3 | -0.8 | -0.6 | -2.5 | 0.3 |  |
| stereo coherence | 0.675 | 0.942 | 0.777 | 0.813 | 0.783 | 1.5 | yes |
| partial-to-partial jaggedness (dB) | 1.85 | 0.3 | 1.25 | 1 | 2.65 | 0.6 |  |
| dip between legato notes (dB) | 10.0 | 11.8 | 9.4 | 7.4 | 4.5 | 0.4 |  |
| level evenness of a legato scale (dB) | 1.6 | 1.4 | 1.4 | 1.3 | 3.2 | 0.2 |  |
| level over the keyboard (dB/oct) | 0.56 | -0.98 | -0.18 | -0.36 | -0.88 | 1.2 | yes |
| level against the other instruments (velocity 96) (dB) | 1 | 1.9 | -0.2 | 0.9 | -0.4 | 0.6 |  |
| key-to-key level scatter (dB) | 1.89 | 0.69 | 1.19 | 0.75 | 2.4 | 1.1 |  |
| odd / even partials (dB) | -7.1 | -4.7 | -3 | -3.8 | -3.7 | 1.7 | yes |
| onset jitter in a phrase (ms) | 49.4 | 41.0 | 59.6 | 48.9 | 46.0 | 0.2 |  |
| onset in a phrase (re the note-on) (ms) | 54.0 | 8 | 64.0 | 27.0 | 30.0 | 0.9 |  |
| onset sharpness (dB/ms) | 5 | 4.4 | 4.8 | 4.2 | 6 | 0.2 |  |
| overtone levels, note for note (dB rms) | 7.27 (nearest 6.17) | fonts among themselves 5.89 | | | | 1.2 |  |
| panning across the keys (dB/oct) | 0.47 | -0.2 | 0.44 | 0.5 | 0.14 | 0.2 |  |
| partial envelopes alike | 0.007 | 0.009 | 0.026 | -0.026 | 0.121 | 0.1 |  |
| release time (30 dB after note-off) (ms) | 790 | 740 | 604 | 571 | 693 | 1.2 |  |
| spectrogram (1 - correlation), note for note | 0.125 (nearest 0.114) | fonts among themselves 0.13 | | | | 1.0 |  |
| spectral shape of the body, note for note | 9.34 (nearest 7.15) | fonts among themselves 9.15 | | | | 1.0 |  |
| sounding length of a 100 ms note (ms) | 400 | 349 | 319 | 319 | 358 | 1.0 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.29 | 0 | 0.31 | 0.18 | -0.5 | 0.8 |  |
| level after 2 s held, re peak (dB) | -8.4 | -5 | -6.9 | -6.7 | -8.7 | 0.5 |  |
| high bands lag low bands by (ms) | 23.0 | 5.2 | -15.0 | -6.6 | 7.7 | 2.1 | yes |
| transient coherence: attack spread over bands (ms) | 20.2 | 4.2 | 33.1 | 29.9 | 6.8 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.728 | 0.802 | 0.809 | 0.817 | 0.852 | 1.2 | yes |
| partials 2-4 share (tristimulus 2) | 0.243 | 0.169 | 0.169 | 0.165 | 0.127 | 1.1 | yes |
| partials 5+ share (tristimulus 3) | 0.011 | 0.019 | 0.018 | 0.027 | 0.014 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 17.1 | 16.8 | 16.6 | 16.7 | 20.0 | 0.2 |  |
| pitch wobble / vibrato (p-p) (c) | 25.1 | 5.8 | 31.2 | 33.9 | 17.9 | 0.0 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0197 (nearest 0.0132) | fonts among themselves 0.0155 | | | | 1.3 |  |

</details>

### 53 Choir Aahs: choir

Distance 2.70 (rank 65 of 129); the fonts disagree 1.2 JND. Keys 48,51,54,57,60,63,66,69,72,75,78, velocities 40/64/96/127.

- **Quieter (-6.3 dB vs -0.4..-3 against the other instruments)**: FAULT, small and easy: the choir's level trim leaves it ~4 dB under the fonts' balance.
- **Speaks later (onset 314 ms vs 107-186; in a phrase 100 ms vs 0-80)**: borderline FAULT. choir.md 5.1 aims for full level in 150-300 ms, and README "Choir (Choir Aahs)" adds ~25 ms entry scatter per singer. Every font is quicker.
- **Brightens with velocity (2.23 ERB vs 0-0.33)**: DELIBERATE. The glottal pulse closes faster with pressure (README "Choir", "The lungs"; choir.md 5.3). The fonts' 0 is TiMidity's convention.
- **Brighter top (+10..+31 dB above 6 kHz)**: partly the church room and breath noise. Overall it is only 1 ERB brighter than the fonts. Watch it, but not a priority.
- **Pan rises across the keys (3.6 dB/oct), wider vibrato (52 vs 36-42 c)**: DELIBERATE (sopranos left, basses right; each singer's vibrato extent ±35 c).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (2.21 ERB | -0 | -0 | -0 | 0.33), outside the fonts, 7.4 x.
2. **Plomp-Levelt dissonance of a note**: smoother (-2.78 log2 | -1.36 | -1.1 | -0.97 | -0.79), outside the fonts, 5.8 x.
3. **brightness (spectral centroid, velocity 96)**: brighter (13.7 ERB | 12.9 | 12.6 | 12.3 | 12.9), outside the fonts, 2.8 x.
4. **loudness range, velocity 40 to 127**: a wider dynamic range (21.1 dB | 16.6 | 16.6 | 16.7 | 20.1), outside the fonts, 2.5 x.
5. **onset delay**: speaks later (269 ms | 186 | 124 | 168 | 104), outside the fonts, 3.0 x.
6. **pitch wobble / vibrato (p-p)**: wobbles more (48.5 c | 41.4 | 35.0 | 39.8 | 35.7), outside the fonts, 2.1 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -4 | -6 | -8 | -2 | +1 | +10 |
| re TimGM6mb | -28 | -13 | -11 | -7 | -1 | +31 |
| re Jnsgm2 | -21 | -8 | -8 | -2 | -1 | +16 |
| re SGM | -24 | -5 | -4 | -2 | +5 | +11 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -7 +0 +10; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.1 ERB, attack -0.1 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 5.1 dB rms; TimGM6mb: centroid -0.2 ERB, attack -0.3 ERB, velocity brightening +0.2 ERB, loudness range +4 dB, envelope 4.0 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.1 ERB, velocity brightening +0.3 ERB, loudness range +3 dB, envelope 1.8 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.59 | -0.73 | 0.15 | -0.54 | 0.55 | 0.6 |  |
| attack time (-20 to -1 dB) (ms) | 689 | 1029 | 356 | 562 | 573 | 0.4 |  |
| spectral shape of the attack, note for note | 9.75 (nearest 8.46) | fonts among themselves 7.94 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 1.24 | 1.29 | 1.03 | 1.6 | 1.41 | 0.4 |  |
| key-to-key brightness scatter (ERB) | 0.866 | 0.663 | 0.732 | 0.601 | 0.851 | 0.8 |  |
| brightening from velocity 40 to 127 (ERB) | 2.21 | -0 | -0 | -0 | 0.33 | 7.4 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 13.7 | 12.9 | 12.6 | 12.3 | 12.9 | 2.8 | yes |
| onset delay (ms) | 269 | 186 | 124 | 168 | 104 | 3.0 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.78 | -1.36 | -1.1 | -0.97 | -0.79 | 5.8 | yes |
| envelope curve, note for note (dB rms) | 12.9 (nearest 5.9) | fonts among themselves 7.81 | | | | 1.7 |  |
| pitch re 12-TET A440 (median) (c) | -0.2 | -16.6 | -4.3 | -11.1 | 0 | 0.9 |  |
| pitch scatter from key to key (c) | 7.1 | 7.4 | 10.7 | 19.6 | 23.3 | 1.0 |  |
| noisiness (spectral flatness) (dB) | -28.3 | -31.6 | -38.3 | -29.7 | -31.3 | 0.7 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -61.4 | -72.0 | -123 | -124 | -111 | 2.1 | yes |
| pitch at the start re settled (c) | -1.6 | 5.4 | 5.1 | -12.8 | 2.2 | 0.6 |  |
| stereo coherence | 0.131 | 0.268 | 0.651 | 1 | 0.118 | 0.7 |  |
| partial-to-partial jaggedness (dB) | 2.8 | 3.3 | 2.4 | 2.4 | 2.5 | 0.3 |  |
| dip between legato notes (dB) | 8 | 10.4 | 8.7 | 8.2 | 9 | 0.4 |  |
| level evenness of a legato scale (dB) | 1.9 | 2.8 | 2.1 | 1.7 | 1 | 0.0 |  |
| level over the keyboard (dB/oct) | 2.05 | -0.1 | 0.13 | 0.24 | 2.66 | 1.4 |  |
| level against the other instruments (velocity 96) (dB) | -1.8 | -2.6 | -1.1 | -0.2 | -0.2 | 1.0 |  |
| key-to-key level scatter (dB) | 1.41 | 1.82 | 1.31 | 1.54 | 1 | 0.0 |  |
| odd / even partials (dB) | -4.6 | -8 | -5.2 | -5.1 | -11.4 | 0.6 |  |
| onset jitter in a phrase (ms) | 54.9 | 49.6 | 44.8 | 46.4 | 42.5 | 1.9 | yes |
| onset in a phrase (re the note-on) (ms) | 26.0 | 80.0 | 0 | 1 | 3 | 0.6 |  |
| onset sharpness (dB/ms) | 6.1 | 6.3 | 6.2 | 8.6 | 6.6 | 0.2 |  |
| overtone levels, note for note (dB rms) | 9.19 (nearest 7.95) | fonts among themselves 6.86 | | | | 1.3 |  |
| panning across the keys (dB/oct) | 1.59 | 0.76 | 0.16 | 0 | -0.57 | 1.5 | yes |
| partial envelopes alike | 0.053 | 0.087 | -0.005 | 0.142 | 0.028 | 0.0 |  |
| release time (30 dB after note-off) (ms) | 653 | 667 | 276 | 346 | 303 | 1.6 |  |
| spectrogram (1 - correlation), note for note | 0.2 (nearest 0.132) | fonts among themselves 0.112 | | | | 1.8 |  |
| spectral shape of the body, note for note | 10.6 (nearest 10) | fonts among themselves 6.74 | | | | 1.6 | yes |
| sounding length of a 100 ms note (ms) | 319 | 400 | 185 | 229 | 221 | 0.9 |  |
| partial stretch (inharmonicity) (c/partial) | -0.09 | -0.35 | 0.37 | 0.26 | 0.36 | 0.8 |  |
| level after 2 s held, re peak (dB) | -8.2 | -10.6 | -9.1 | -7.6 | -9.6 | 0.4 |  |
| high bands lag low bands by (ms) | 16.8 | 15.3 | -14.0 | 14.7 | -12.7 | 1.1 |  |
| transient coherence: attack spread over bands (ms) | 20.9 | 17.1 | 42.6 | 18.9 | 14.1 | 0.2 |  |
| fundamental share (tristimulus 1) | 0.204 | 0.109 | 0.267 | 0.121 | 0.221 | 0.4 |  |
| partials 2-4 share (tristimulus 2) | 0.629 | 0.811 | 0.516 | 0.791 | 0.673 | 0.7 |  |
| partials 5+ share (tristimulus 3) | 0.069 | 0.01 | 0.007 | 0.012 | 0.021 | 0.8 | yes |
| loudness range, velocity 40 to 127 (dB) | 21.1 | 16.6 | 16.6 | 16.7 | 20.1 | 2.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 48.5 | 41.4 | 35.0 | 39.8 | 35.7 | 2.1 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0172 (nearest 0.0147) | fonts among themselves 0.0162 | | | | 1.1 |  |

</details>

### 54 Voice Oohs: voice-oohs

Distance 5.15 (rank 3 of 129); the fonts disagree 0.9 JND. Keys 48,51,54,57,60,63,66,69,72,75,78, velocities 40/64/96/127.

- **Very slow speech (attack 774 ms vs 39-80; onset 444 ms vs 22-39; 76 ms in a phrase vs 0-10)**: FAULT. README "Voice Oohs" asks for half level at ~110 ms and choir.md 5.1 for 250-400 ms. 774 ms is about twice the plan, so rhythmic "ooh" parts will drag.
- **Quieter (-7.1 dB vs -0.3..+2.8)**: FAULT. README says it is ~6 dB under the aahs before its trim. The trim does not make up for it.
- **Uneven over the keys (level scatter 3.2 dB vs 0.5-1.4; legato evenness 4.1 dB)**: probable FAULT. Above C4 the men drop out (a DELIBERATE split), but the level then jumps there.
- **Release 664 ms, stereo coherence 0.16**: MEASUREMENT ARTIFACT of the church room.
- **Vibrato 47 c vs 7-15; brightens with velocity**: DELIBERATE (singer physics). The fonts are flat loops.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **attack time (-20 to -1 dB)**: a slower attack (760 ms | 80.0 | 39.0 | 40.0 | 40.0), outside the fonts, 8.2 x.
2. **level against the other instruments (velocity 96)**: quieter (-7 dB | 3.2 | -0.1 | 0.6 | 1.8), outside the fonts, 5.0 x.
3. **onset delay**: speaks later (326 ms | 39.0 | 22.0 | 23.0 | 24.0), outside the fonts, 30.2 x.
4. **onset in a phrase (re the note-on)**: speaks later in a phrase (114 ms | 3 | 10.0 | 6 | -0), outside the fonts, 10.9 x.
5. **release time (30 dB after note-off) [room]**: rings longer after the key (642 ms | 304 | 218 | 241 | 231), outside the fonts, 5.8 x.
6. **brightness (spectral centroid, velocity 96)**: brighter (9.42 ERB | 8.42 | 8.51 | 8.19 | 8.26), outside the fonts, 3.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -11 | -6 | -2 | -5 | +5 | +6 |
| re TimGM6mb | -22 | -18 | -13 | -11 | +6 | +30 |
| re Jnsgm2 | -10 | -8 | -2 | -1 | -0 | +9 |
| re SGM | -13 | -9 | -4 | -3 | -2 | +16 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -8 -5 +0 +8; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.1 ERB, attack -0.1 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 4.0 dB rms; TimGM6mb: centroid -0.3 ERB, attack -0.0 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 4.5 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 2.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.16 | -0.19 | -0.11 | 0.02 | 0.03 | 0.5 |  |
| attack time (-20 to -1 dB) (ms) | 760 | 80.0 | 39.0 | 40.0 | 40.0 | 8.2 | yes |
| spectral shape of the attack, note for note | 14.7 (nearest 12.3) | fonts among themselves 10.5 | | | | 1.4 |  |
| brightness over the keyboard (ERB/oct) | 1.49 | 3.73 | 2.91 | 2.51 | 2.93 | 2.3 | yes |
| key-to-key brightness scatter (ERB) | 1.02 | 0.228 | 0.286 | 0.387 | 0.5 | 3.4 | yes |
| brightening from velocity 40 to 127 (ERB) | 1.38 | -0 | 0 | -0 | -0.33 | 4.6 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 9.42 | 8.42 | 8.51 | 8.19 | 8.26 | 3.6 | yes |
| onset delay (ms) | 326 | 39.0 | 22.0 | 23.0 | 24.0 | 30.2 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.7 | -4.44 | -4.29 | -4.97 | -4.68 | 0.4 |  |
| envelope curve, note for note (dB rms) | 17.2 (nearest 15.4) | fonts among themselves 4.61 | | | | 3.7 | yes |
| pitch re 12-TET A440 (median) (c) | -1.9 | -0.6 | 3.4 | 3.2 | 3.1 | 1.3 |  |
| pitch scatter from key to key (c) | 8.2 | 0.3 | 2.1 | 0.7 | 2.7 | 2.3 | yes |
| noisiness (spectral flatness) (dB) | -37.1 | -41.1 | -55.8 | -37.3 | -39.4 | 0.3 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -61.4 | -125 | -122 | -122 | -110 | 8.1 | yes |
| pitch at the start re settled (c) | -1.1 | -7.3 | -1.7 | -1.1 | -2 | 0.1 |  |
| stereo coherence | 0.267 | 0.968 | 1 | 1 | 1 | 9.8 | yes |
| partial-to-partial jaggedness (dB) | 3.75 | 5.8 | 3.8 | 5 | 4.7 | 1.1 |  |
| dip between legato notes (dB) | 11.5 | 16.4 | 4.3 | 7.3 | 6.2 | 0.8 |  |
| level evenness of a legato scale (dB) | 2.2 | 2.7 | 0.9 | 1 | 1.7 | 0.9 |  |
| level over the keyboard (dB/oct) | 3.52 | 2.85 | 1.5 | 0.16 | 0.95 | 1.7 | yes |
| level against the other instruments (velocity 96) (dB) | -7 | 3.2 | -0.1 | 0.6 | 1.8 | 5.0 | yes |
| key-to-key level scatter (dB) | 3.25 | 0.58 | 1.28 | 0.45 | 1.37 | 4.6 | yes |
| odd / even partials (dB) | -9.4 | 18.4 | -17.8 | -12.9 | -14.4 | 0.2 |  |
| onset jitter in a phrase (ms) | 49.2 | 24.5 | 42.2 | 17.9 | 43.8 | 1.2 | yes |
| onset in a phrase (re the note-on) (ms) | 114 | 3 | 10.0 | 6 | -0 | 10.9 | yes |
| onset sharpness (dB/ms) | 5.7 | 3.8 | 4.9 | 7.8 | 45.6 | 0.0 |  |
| overtone levels, note for note (dB rms) | 11.7 (nearest 11) | fonts among themselves 8.3 | | | | 1.4 | yes |
| panning across the keys (dB/oct) | 2.05 | 0.25 | 0 | 0 | -0.06 | 2.0 | yes |
| partial envelopes alike | 0.103 | 0.092 | -0.017 | 0.004 | 0.063 | 0.7 |  |
| release time (30 dB after note-off) (ms) | 642 | 304 | 218 | 241 | 231 | 5.8 | yes |
| spectrogram (1 - correlation), note for note | 0.389 (nearest 0.308) | fonts among themselves 0.179 | | | | 2.2 | yes |
| spectral shape of the body, note for note | 10.7 (nearest 9.4) | fonts among themselves 8.5 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 264 | 261 | 126 | 136 | 150 | 1.7 |  |
| partial stretch (inharmonicity) (c/partial) | 0.09 | 0.01 | 0.17 | 0.03 | 0.15 | 0.0 |  |
| level after 2 s held, re peak (dB) | -8.9 | -10.4 | -8.4 | -8.7 | -8.6 | 0.1 |  |
| high bands lag low bands by (ms) | 21.0 | 69.0 | -4.3 | -14.5 | -13.0 | 0.7 |  |
| transient coherence: attack spread over bands (ms) | 14.5 | 20.0 | 2 | 3.7 | 4.2 | 1.2 |  |
| fundamental share (tristimulus 1) | 0.851 | 0.968 | 0.97 | 0.965 | 0.958 | 1.6 | yes |
| partials 2-4 share (tristimulus 2) | 0.144 | 0.031 | 0.03 | 0.035 | 0.042 | 1.6 | yes |
| partials 5+ share (tristimulus 3) | 0.005 | 0 | 0.004 | 0.001 | 0.005 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 16.6 | 16.6 | 16.7 | 16.6 | 20.0 | 0.0 |  |
| pitch wobble / vibrato (p-p) (c) | 45.5 | 11.3 | 14.7 | 7.4 | 13.1 | 6.7 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0241 (nearest 0.023) | fonts among themselves 0.0174 | | | | 1.4 | yes |

</details>

### 55 Synth Voice: synth-voice

Distance 2.97 (rank 52 of 129); the fonts disagree 1.1 JND. Keys 48,51,54,57,60,63,66,69,72,75,78, velocities 40/64/96/127.

- **Uneven tone from key to key (brightness scatter 1.62 ERB vs 0.17-0.45)**: likely the per-key formant/cutoff spread (synth_leads.md 10, K1 "each key its own fixed cutoff offset"). DELIBERATE, but the default looks too wide for a pad-like voice.
- **Darker when played harder (-1.11 ERB vs 0..-0.3)**: probable FAULT. A synth voice should open up with velocity, or at least stay the same.
- **Mid-heavy spectrum (+10..+15 dB at 1-2.5 kHz, -11..-18 dB above 6 kHz; fundamental share 0.26 vs 0.64-0.99)**: the formant bank's choice of vowel. Debatable: the fonts disagree with each other by up to 30 dB above 6 kHz.
- **Faster attack (55 ms vs 167-480)**: odd, since synth_leads.md says "slow onset". Minor.
- **Quieter (-3.1 dB)**: small trim.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key brightness scatter**: less even in tone from key to key (1.62 ERB | 0.403 | 0.166 | 0.287 | 0.448), outside the fonts, 6.4 x.
2. **brightening from velocity 40 to 127**: brightens less with velocity (-1.11 ERB | -0 | -0 | -0 | -0.29), outside the fonts, 3.7 x.
3. **attack brighter than the body by**: a duller attack (-2.04 ERB | 0.57 | 0.04 | 0.1 | 0.05), outside the fonts, 5.3 x.
4. **attack time (-20 to -1 dB)**: a faster attack (54.5 ms | 167 | 233 | 480 | 478), outside the fonts, 3.4 x.
5. **loudness range, velocity 40 to 127**: a wider dynamic range (20.9 dB | 16.7 | 16.7 | 16.7 | 20.0), outside the fonts, 2.5 x.
6. **brightness (spectral centroid, velocity 96)**: brighter (9.98 ERB | 9.42 | 8.05 | 8.55 | 8.49), outside the fonts, 2.1 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +2 | +2 | +3 | +10 | +7 | -18 |
| re TimGM6mb | -6 | -17 | -13 | +3 | +10 | +30 |
| re Jnsgm2 | -2 | -3 | -0 | +15 | +1 | -15 |
| re SGM | -4 | -5 | -3 | +13 | +1 | -11 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -15 -6 +0 +6; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack +0.0 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 8.4 dB rms; TimGM6mb: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.5 ERB, loudness range +3 dB, envelope 1.6 dB rms; Jnsgm2: centroid -0.1 ERB, attack -0.1 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 1.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -2.04 | 0.57 | 0.04 | 0.1 | 0.05 | 5.3 | yes |
| attack time (-20 to -1 dB) (ms) | 54.5 | 167 | 233 | 480 | 478 | 3.4 | yes |
| spectral shape of the attack, note for note | 16.8 (nearest 13.1) | fonts among themselves 11 | | | | 1.5 |  |
| brightness over the keyboard (ERB/oct) | 2.92 | 4.86 | 3.58 | 2.91 | 2.72 | 0.3 |  |
| key-to-key brightness scatter (ERB) | 1.62 | 0.403 | 0.166 | 0.287 | 0.448 | 6.4 | yes |
| brightening from velocity 40 to 127 (ERB) | -1.11 | -0 | -0 | -0 | -0.29 | 3.7 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 9.98 | 9.42 | 8.05 | 8.55 | 8.49 | 2.1 | yes |
| onset delay (ms) | 36.0 | 63.0 | 79.0 | 43.0 | 41.0 | 0.9 |  |
| Plomp-Levelt dissonance of a note (log2) | -3.1 | -4.29 | -6.16 | -3.66 | -4.01 | 0.8 | yes |
| envelope curve, note for note (dB rms) | 10.3 (nearest 4.76) | fonts among themselves 8.63 | | | | 1.2 |  |
| pitch re 12-TET A440 (median) (c) | 0.3 | -3.7 | -2.1 | 0.3 | 1.2 | 0.3 |  |
| pitch scatter from key to key (c) | 0.8 | 1.6 | 3.7 | 1.5 | 1.9 | 0.3 |  |
| noisiness (spectral flatness) (dB) | -33.0 | -30.9 | -72.9 | -35.8 | -41.3 | 0.3 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -61.8 | -59.7 | -123 | -122 | -98.9 | 1.5 |  |
| pitch at the start re settled (c) | 2.7 | -0.2 | -3.5 | 10.6 | 10.6 | 0.4 |  |
| stereo coherence | 0.716 | 0.987 | 1 | 1 | 1 | 3.8 | yes |
| partial-to-partial jaggedness (dB) | 3.45 | 3.1 | 3.2 | 3.4 | 3.4 | 0.2 |  |
| dip between legato notes (dB) | 9.5 | 13.2 | 14.1 | 9.9 | 14.0 | 2.0 |  |
| level evenness of a legato scale (dB) | 2.1 | 1.1 | 0.7 | 2.3 | 2.6 | 0.4 |  |
| level over the keyboard (dB/oct) | -1.04 | -0.78 | -1.47 | 2.43 | 1.19 | 0.6 |  |
| level against the other instruments (velocity 96) (dB) | 3.4 | 1.2 | 0.6 | -1.4 | 2.4 | 1.3 | yes |
| key-to-key level scatter (dB) | 1.29 | 0.55 | 0.78 | 0.99 | 0.87 | 0.9 | yes |
| odd / even partials (dB) | -3 | -14.4 | -7.1 | -19.6 | -19.5 | 2.2 | yes |
| onset jitter in a phrase (ms) | 40.2 | 46.1 | 38.6 | 46.4 | 40.4 | 0.6 |  |
| onset in a phrase (re the note-on) (ms) | 38.0 | 31.0 | 18.0 | 13.0 | 8 | 2.0 | yes |
| onset sharpness (dB/ms) | 6.4 | 6 | 2.7 | 9.2 | 9.2 | 0.4 |  |
| overtone levels, note for note (dB rms) | 25.1 (nearest 15.5) | fonts among themselves 14.9 | | | | 1.7 |  |
| panning across the keys (dB/oct) | 0.05 | 0.29 | 0 | 0 | -0.06 | 0.1 |  |
| partial envelopes alike | -0.014 | 0.011 | -0.039 | -0.001 | 0.026 | 0.2 |  |
| release time (30 dB after note-off) (ms) | 859 | 673 | 379 | 421 | 362 | 2.5 | yes |
| spectrogram (1 - correlation), note for note | 0.48 (nearest 0.397) | fonts among themselves 0.253 | | | | 1.9 | yes |
| spectral shape of the body, note for note | 15.1 (nearest 12.5) | fonts among themselves 10.6 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 236 | 400 | 400 | 133 | 139 | 0.0 |  |
| partial stretch (inharmonicity) (c/partial) | 0.03 | 0.55 | - | -4.94 | -4.22 | 1.5 |  |
| level after 2 s held, re peak (dB) | -8.6 | -4.7 | -7.4 | -9.7 | -10.9 | 0.0 |  |
| high bands lag low bands by (ms) | 8.3 | -3 | -20.0 | 6.7 | 1 | 0.7 |  |
| transient coherence: attack spread over bands (ms) | 11.2 | 8.4 | 2.3 | 2.8 | 2.7 | 1.7 | yes |
| fundamental share (tristimulus 1) | 0.261 | 0.636 | 0.994 | 0.879 | 0.88 | 3.5 | yes |
| partials 2-4 share (tristimulus 2) | 0.551 | 0.362 | 0.006 | 0.121 | 0.12 | 2.4 | yes |
| partials 5+ share (tristimulus 3) | 0.058 | 0.002 | 0 | 0 | 0 | 0.8 | yes |
| loudness range, velocity 40 to 127 (dB) | 20.9 | 16.7 | 16.7 | 16.7 | 20.0 | 2.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 12.3 | 20.1 | 31.1 | 31.1 | 30.9 | 3.4 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0232 (nearest 0.0219) | fonts among themselves 0.0203 | | | | 1.1 |  |

</details>

### 56 Orchestra Hit: orchestra-hit

Distance 3.61 (rank 29 of 129); the fonts disagree 1.3 JND. Keys 48,51,54,57,60,63,66,69,72,75,78, velocities 40/64/96/127.

- **Holds as long as the key (decay 0.6 dB/s vs 18-37; level after 2 s -9 dB vs -40..-124)**: DELIBERATE. README "-inst orchestra-hit" and the src/orchhit.cc header say it is players, not a recording (`PJ_HIT_LEN` gives an eighth instead). The GM habit is a one-shot stab, though, so the eighth may be the better default for `-play`.
- **Slow for a "hit" (attack 374 ms vs 44-103; onset 72 ms vs 4-41)**: FAULT for this program. The piccolo and flute run-ups, the piano roll and the reeds' slow speech blunt the stab.
- **Brighter and thin in the bass (13.3 ERB vs 6.5-9.4; -11..-22 dB under 150 Hz)**: probable FAULT. The low sections fold into their own ranges and the reeds are bright.
- **Smoother, little "wobble" (Plomp-Levelt -2.4 vs -0.3; wobble 16 c vs 67-82)**: MEASUREMENT ARTIFACT plus DELIBERATE choice. The fonts' sample is the Firebird chord, ours the key in octaves (orchhit.cc). The wobble measure misreads a chord.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (2.1 ERB | -0.02 | 0.06 | 0.06 | -0.06), outside the fonts, 6.9 x.
2. **pitch wobble / vibrato (p-p)**: wobbles less (17.4 c | 73.6 | 61.3 | 60.9 | 67.8), outside the fonts, 7.4 x.
3. **release time (30 dB after note-off) [room]**: rings longer after the key (642 ms | 342 | 276 | 258 | 262), outside the fonts, 5.0 x.
4. **Plomp-Levelt dissonance of a note**: smoother (-2.04 log2 | -0.3 | -0.27 | -0.26 | 0.13), outside the fonts, 5.9 x.
5. **brightness (spectral centroid, velocity 96)**: brighter (11.6 ERB | 8.29 | 6.53 | 7.09 | 9.35), outside the fonts, 2.8 x.
6. **attack time (-20 to -1 dB)**: a slower attack (308 ms | 46.0 | 99.0 | 103 | 44.0), outside the fonts, 3.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -10 | -3 | -2 | -1 | +4 | +4 |
| re TimGM6mb | -21 | -10 | -10 | -8 | +4 | +24 |
| re Jnsgm2 | -14 | -3 | -3 | -3 | +3 | +9 |
| re SGM | -11 | -3 | -2 | -4 | +0 | +10 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -5 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.9 ERB, attack +0.0 ERB, velocity brightening +0.0 ERB, loudness range +4 dB, envelope 2.7 dB rms; TimGM6mb: centroid -0.6 ERB, attack +0.4 ERB, velocity brightening -0.4 ERB, loudness range +4 dB, envelope 2.6 dB rms; Jnsgm2: centroid -0.2 ERB, attack +0.0 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 2.2 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -2.86 | -1 | -2.38 | -2.48 | -1.91 | 1.0 | yes |
| attack time (-20 to -1 dB) (ms) | 308 | 46.0 | 99.0 | 103 | 44.0 | 3.6 | yes |
| spectral shape of the attack, note for note | 10.5 (nearest 8.76) | fonts among themselves 8.45 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 0.64 | 1.13 | 0.72 | 0.4 | 1.7 | 0.4 |  |
| key-to-key brightness scatter (ERB) | 1.25 | 0.698 | 0.414 | 0.39 | 0.536 | 3.9 | yes |
| brightening from velocity 40 to 127 (ERB) | 2.1 | -0.02 | 0.06 | 0.06 | -0.06 | 6.9 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 11.6 | 8.29 | 6.53 | 7.09 | 9.35 | 2.8 | yes |
| onset delay (ms) | 43.0 | 19.0 | 4 | 5 | 36.0 | 1.9 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.04 | -0.3 | -0.27 | -0.26 | 0.13 | 5.9 | yes |
| envelope curve, note for note (dB rms) | 26.5 (nearest 21) | fonts among themselves 16.9 | | | | 1.6 |  |
| pitch re 12-TET A440 (median) (c) | 0.6 | -3.4 | 18.1 | -3.2 | 28.7 | 0.4 |  |
| pitch scatter from key to key (c) | 2.1 | 5.5 | 9.2 | 8.5 | 1.9 | 1.3 |  |
| noisiness (spectral flatness) (dB) | -22.6 | -18.9 | -38.4 | -19.9 | -35.5 | 0.5 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -64.2 | -126 | -125 | -127 | -111 | 7.8 | yes |
| pitch at the start re settled (c) | -0.1 | 7.5 | -2 | -16.5 | -20.9 | 0.6 |  |
| stereo coherence | 0.263 | 0.815 | 1 | 1 | 0.923 | 7.6 | yes |
| partial-to-partial jaggedness (dB) | 4.55 | 3.2 | 2.3 | 2.9 | 3 | 1.6 | yes |
| dip between legato notes (dB) | 11.3 | 11.4 | 11.7 | 10.5 | 8.3 | 0.2 |  |
| level evenness of a legato scale (dB) | 1.3 | 0.6 | 0.9 | 0.9 | 1.7 | 0.4 |  |
| level over the keyboard (dB/oct) | 1.09 | -1.61 | -0.67 | -0.73 | -3.45 | 1.6 | yes |
| level against the other instruments (velocity 96) (dB) | -1.7 | 0.4 | 3.9 | 5.8 | 0.2 | 1.4 | yes |
| key-to-key level scatter (dB) | 1.1 | 0.3 | 0.06 | 0.07 | 0.63 | 1.8 | yes |
| odd / even partials (dB) | -8.2 | -6.1 | -2.3 | -0.9 | -5.7 | 1.6 | yes |
| onset jitter in a phrase (ms) | 47.9 | 41.5 | 39.1 | 38.9 | 36.1 | 1.8 | yes |
| onset in a phrase (re the note-on) (ms) | 30.0 | 68.0 | 4 | 62.0 | 21.0 | 0.4 |  |
| onset sharpness (dB/ms) | 6.5 | 6.1 | 8.2 | 8.3 | 41.1 | 0.1 |  |
| overtone levels, note for note (dB rms) | 6.73 (nearest 5.22) | fonts among themselves 5.19 | | | | 1.3 |  |
| panning across the keys (dB/oct) | -0.07 | 0 | 0 | 0 | -0.24 | 0.1 |  |
| partial envelopes alike | 0.001 | 0.995 | 0.874 | 0.855 | 0.954 | 9.1 | yes |
| release time (30 dB after note-off) (ms) | 642 | 342 | 276 | 258 | 262 | 5.0 | yes |
| spectrogram (1 - correlation), note for note | 0.506 (nearest 0.402) | fonts among themselves 0.184 | | | | 2.8 | yes |
| spectral shape of the body, note for note | 11.4 (nearest 10.5) | fonts among themselves 8.23 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 236 | 94.0 | 125 | 105 | 109 | 4.6 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.3 | 0.16 | -0.39 | -0.07 | -0.76 | 0.1 |  |
| level after 2 s held, re peak (dB) | -9.9 | -124 | -40.9 | -39.9 | -96.2 | 1.4 | yes |
| high bands lag low bands by (ms) | 14.2 | -6 | 11.0 | 28.0 | 22.6 | 0.2 |  |
| transient coherence: attack spread over bands (ms) | 27.2 | 5.9 | 5.7 | 9.2 | 16.6 | 3.6 | yes |
| fundamental share (tristimulus 1) | 0.299 | 0.653 | 0.416 | 0.476 | 0.482 | 1.5 | yes |
| partials 2-4 share (tristimulus 2) | 0.443 | 0.281 | 0.526 | 0.361 | 0.367 | 0.6 |  |
| partials 5+ share (tristimulus 3) | 0.186 | 0.062 | 0.082 | 0.163 | 0.148 | 1.0 |  |
| loudness range, velocity 40 to 127 (dB) | 14.6 | 16.6 | 16.6 | 16.7 | 20.1 | 1.2 | yes |
| pitch wobble / vibrato (p-p) (c) | 17.4 | 73.6 | 61.3 | 60.9 | 67.8 | 7.4 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0251 (nearest 0.0245) | fonts among themselves 0.0126 | | | | 2.0 | yes |

</details>

### 57 Trumpet: trumpet

Distance 4.23 (rank 15 of 129); the fonts disagree 1.0 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Sounds a tone low under -play**: FAULT (the transposition bug: src/midifile.cc adds `transpose` −2). It also voids the pitch rows (median +28 c, scatter 20 c, wobble 86 c, fundamental share ~0): the f0 search stays within ±5 % of the key (tools/gmcompare.py), and −200 c falls outside.
- **Dark (11.2 vs 17.8…19.3 ERB)**: known FAULT: brass_section.md §12 and §14, "Against FluidR3": ours is much darker above 2 kHz under v~110.
- **High bands lead the low (−7.5 vs +19…+59 ms); attack 4.6 ERB brighter than the body (fonts −1…−2)**: probable FAULT, shared with p058/p060. trumpet.md §7 says the higher harmonics build up later. Something in the onset (the tongue or the breath push) is too bright.
- **Low notes weak (+5.6 dB/oct; scatter 3.4 dB)**: known (README brass test table: low notes ~20 dB softer). **Release 722 ms**: room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: darker (12.3 ERB | 18.0 | 17.8 | 19.3 | 17.8), outside the fonts, 7.4 x.
2. **key-to-key level scatter**: less even from key to key (3.46 dB | 0.58 | 0.53 | 0.68 | 0.47), outside the fonts, 5.8 x.
3. **release time (30 dB after note-off) [room]**: rings longer after the key (770 ms | 78.0 | 140 | 98.0 | 70.0), outside the fonts, 6.3 x.
4. **attack brighter than the body by**: a brighter attack (3.21 ERB | -2.09 | -2.34 | -1.32 | -2.18), outside the fonts, 10.5 x.
5. **loudness range, velocity 40 to 127**: a narrower dynamic range (10.8 dB | 16.6 | 16.6 | 16.7 | 20.1), outside the fonts, 3.3 x.
6. **onset delay**: speaks later (58.0 ms | 16.0 | 13.0 | 21.0 | 14.0), outside the fonts, 4.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -27 | +7 | +4 | -2 | -1 | +2 |
| re TimGM6mb | -20 | +6 | +1 | -5 | -2 | +8 |
| re Jnsgm2 | -19 | +12 | +6 | +1 | -1 | -6 |
| re SGM | -28 | +7 | +3 | -2 | -0 | +2 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -8 -3 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.2 ERB, attack +0.1 ERB, velocity brightening +1.0 ERB, loudness range +5 dB, envelope 2.0 dB rms; TimGM6mb: centroid -0.0 ERB, attack -0.1 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 1.9 dB rms; Jnsgm2: centroid -0.1 ERB, attack +0.4 ERB, velocity brightening +1.7 ERB, loudness range +5 dB, envelope 1.9 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 3.21 | -2.09 | -2.34 | -1.32 | -2.18 | 10.5 | yes |
| attack time (-20 to -1 dB) (ms) | 83.5 | 22.0 | 176 | 97.0 | 29.0 | 0.4 |  |
| spectral shape of the attack, note for note | 15.3 (nearest 15) | fonts among themselves 6.69 | | | | 2.3 | yes |
| brightness over the keyboard (ERB/oct) | 2.26 | 3.13 | 3.06 | 3.03 | 3.33 | 2.8 | yes |
| key-to-key brightness scatter (ERB) | 1.36 | 0.701 | 0.411 | 0.699 | 0.602 | 3.6 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.66 | 0.59 | -0 | -0 | 0.13 | 2.0 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 12.3 | 18.0 | 17.8 | 19.3 | 17.8 | 7.4 | yes |
| onset delay (ms) | 58.0 | 16.0 | 13.0 | 21.0 | 14.0 | 4.3 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.2 | -3 | -2.33 | -2.24 | -2.85 | 1.0 |  |
| envelope curve, note for note (dB rms) | 21.5 (nearest 19.9) | fonts among themselves 5.5 | | | | 3.9 | yes |
| pitch re 12-TET A440 (median) (c) | 2.3 | -0.4 | -2.4 | 0.2 | 7.6 | 0.5 |  |
| pitch scatter from key to key (c) | 4.7 | 1.6 | 2.7 | 1.9 | 0.4 | 1.0 | yes |
| noisiness (spectral flatness) (dB) | -27.8 | -46.8 | -37.2 | -31.1 | -40.8 | 1.4 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -63.3 | -122 | -120 | -117 | -107 | 7.3 | yes |
| pitch at the start re settled (c) | -3 | -17.9 | -15.1 | -0.7 | -20.3 | 1.4 |  |
| stereo coherence | 0.236 | 1 | 1 | 1 | 1 | 10.2 | yes |
| partial-to-partial jaggedness (dB) | 3.55 | 1.3 | 1.4 | 0.7 | 1.4 | 2.2 | yes |
| dip between legato notes (dB) | 8 | 8.3 | 9.5 | 11.1 | 9.6 | 0.8 |  |
| level evenness of a legato scale (dB) | 2.1 | 0.5 | 1.6 | 1.6 | 0.6 | 1.0 |  |
| level over the keyboard (dB/oct) | 3.21 | 0.76 | 1.54 | 2.13 | 0.68 | 2.1 | yes |
| level against the other instruments (velocity 96) (dB) | -4.2 | -2.8 | -1.8 | -4.6 | -2.8 | 1.0 |  |
| key-to-key level scatter (dB) | 3.46 | 0.58 | 0.53 | 0.68 | 0.47 | 5.8 | yes |
| odd / even partials (dB) | -6 | -0.9 | 1 | -0.6 | -1 | 2.6 | yes |
| onset jitter in a phrase (ms) | 35.1 | 60.7 | 47.5 | 51.4 | 45.1 | 1.8 | yes |
| onset in a phrase (re the note-on) (ms) | 50.0 | 4 | 0 | 6 | 10.0 | 4.5 | yes |
| onset sharpness (dB/ms) | 5.6 | 12.1 | 11.7 | 11.7 | 11.5 | 3.0 | yes |
| overtone levels, note for note (dB rms) | 10 (nearest 9.65) | fonts among themselves 3.19 | | | | 3.1 | yes |
| panning across the keys (dB/oct) | 0.4 | -0 | -0 | -0 | 0.02 | 0.4 |  |
| partial envelopes alike | 0.003 | 0.512 | 0.004 | 0.158 | 0.858 | 0.8 |  |
| release time (30 dB after note-off) (ms) | 770 | 78.0 | 140 | 98.0 | 70.0 | 6.3 | yes |
| spectrogram (1 - correlation), note for note | 0.324 (nearest 0.282) | fonts among themselves 0.07 | | | | 4.6 | yes |
| spectral shape of the body, note for note | 12.6 (nearest 11.3) | fonts among themselves 7.29 | | | | 1.7 | yes |
| sounding length of a 100 ms note (ms) | 288 | 66.0 | 141 | 79.0 | 100 | 3.1 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.04 | 0.12 | 0.03 | 0.01 | 0 | 0.1 |  |
| level after 2 s held, re peak (dB) | -5.5 | -16.9 | -8.1 | -16.7 | -18.0 | 2.3 | yes |
| high bands lag low bands by (ms) | -8.8 | 19.0 | 59.0 | 22.3 | 25.7 | 1.6 | yes |
| transient coherence: attack spread over bands (ms) | 11.3 | 8 | 18.3 | 6.5 | 8.2 | 0.5 |  |
| fundamental share (tristimulus 1) | 0.249 | 0.111 | 0.048 | 0.031 | 0.099 | 2.5 | yes |
| partials 2-4 share (tristimulus 2) | 0.562 | 0.389 | 0.428 | 0.286 | 0.362 | 2.6 | yes |
| partials 5+ share (tristimulus 3) | 0.075 | 0.487 | 0.503 | 0.682 | 0.534 | 4.5 | yes |
| loudness range, velocity 40 to 127 (dB) | 10.8 | 16.6 | 16.6 | 16.7 | 20.1 | 3.3 | yes |
| pitch wobble / vibrato (p-p) (c) | 9 | 10.6 | 13.4 | 9.4 | 3.6 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.014 (nearest 0.013) | fonts among themselves 0.00435 | | | | 3.2 | yes |

</details>

### 58 Trombone: trombone

Distance 4.50 (rank 10 of 129); the fonts disagree 1.1 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Dark, fundamental-heavy (centroid 9.3 ERB vs 13.5-14.5, about 400 vs 800 Hz; fundamental share 0.54 vs 0.03-0.05)**: FAULT, the largest. A radiated trombone tone at mf has a weak fundamental and its peak at 500-1000 Hz. This is the same gap as the trumpet's darkness at mf in research/brass_section.md 14 ("against FluidR3": ours much darker above 2 kHz).
- **Much louder toward the top (12.2 dB/oct vs 1.0-1.8; key scatter 3.3 dB)**: FAULT. The f^1.45 breath law (README "Brass", the horn player) is uneven over 3.5 octaves. Keys 79 and 82 lie above F5 and fold, but that does not explain it.
- **Attack 4.8 ERB brighter than the body (fonts -0.5..-2.1)**: a brassy blip on a dull body. It follows from the darkness above.
- **Onset 74 ms vs 18-42; 64 ms in a phrase**: mildly slow (slide travel). Release 630 ms is the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: darker (9.34 ERB | 14.0 | 13.6 | 13.5 | 14.5), outside the fonts, 9.3 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (624 ms | 86.0 | 72.0 | 106 | 78.0), outside the fonts, 10.5 x.
3. **key-to-key level scatter**: less even from key to key (3.58 dB | 1.15 | 0.42 | 0.94 | 0.77), outside the fonts, 5.5 x.
4. **attack brighter than the body by**: a brighter attack (4.81 ERB | -2.12 | -0.93 | -0.5 | -1.54), outside the fonts, 7.5 x.
5. **fundamental share (tristimulus 1)**: more fundamental (0.539 | 0.054 | 0.043 | 0.029 | 0.051), outside the fonts, 7.0 x.
6. **brightening from velocity 40 to 127**: brightens more with velocity (1.15 ERB | -0.02 | -0 | -0 | 0.38), outside the fonts, 3.8 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +22 | +4 | -7 | -11 | -2 | +9 |
| re TimGM6mb | +14 | +7 | -3 | -5 | -4 | +1 |
| re Jnsgm2 | +15 | +9 | -4 | -5 | -3 | -1 |
| re SGM | +15 | +8 | -4 | -8 | -2 | +3 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -8 -4 +0 +4; FluidR3 -12 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack +0.1 ERB, velocity brightening +1.1 ERB, loudness range +4 dB, envelope 1.7 dB rms; TimGM6mb: centroid -0.1 ERB, attack +0.1 ERB, velocity brightening +0.2 ERB, loudness range +4 dB, envelope 1.8 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening +0.2 ERB, loudness range +3 dB, envelope 1.9 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 4.81 | -2.12 | -0.93 | -0.5 | -1.54 | 7.5 | yes |
| attack time (-20 to -1 dB) (ms) | 39.5 | 106 | 36.0 | 37.0 | 114 | 0.8 |  |
| spectral shape of the attack, note for note | 16.5 (nearest 15.3) | fonts among themselves 7.11 | | | | 2.3 | yes |
| brightness over the keyboard (ERB/oct) | 1.72 | 0.91 | 2.26 | 1.42 | 1.42 | 0.4 |  |
| key-to-key brightness scatter (ERB) | 1.6 | 0.894 | 1.09 | 1.29 | 1.11 | 2.5 | yes |
| brightening from velocity 40 to 127 (ERB) | 1.15 | -0.02 | -0 | -0 | 0.38 | 3.8 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 9.34 | 14.0 | 13.6 | 13.5 | 14.5 | 9.3 | yes |
| onset delay (ms) | 74.0 | 42.0 | 26.0 | 18.0 | 30.0 | 3.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.85 | -5.8 | -4.06 | -4.38 | -3.51 | 1.2 | yes |
| envelope curve, note for note (dB rms) | 17.7 (nearest 17.2) | fonts among themselves 4.11 | | | | 4.3 | yes |
| pitch re 12-TET A440 (median) (c) | 2 | -1.9 | 1.9 | 2.5 | -0.4 | 0.3 |  |
| pitch scatter from key to key (c) | 3.1 | 1 | 0.9 | 1 | 1 | 0.7 | yes |
| noisiness (spectral flatness) (dB) | -35.9 | -59.5 | -72.6 | -49.4 | -35.2 | 1.0 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -69.2 | -124 | -119 | -122 | -110 | 7.1 | yes |
| pitch at the start re settled (c) | -14.9 | -11.2 | -2.8 | -4.2 | -11.8 | 1.4 | yes |
| stereo coherence | 0.606 | 1 | 1 | 1 | 1 | 5.3 | yes |
| partial-to-partial jaggedness (dB) | 2.7 | 3.3 | 1.1 | 1.8 | 1.1 | 1.1 |  |
| dip between legato notes (dB) | 9.3 | 10.7 | 10.0 | 10.9 | 11.0 | 0.8 |  |
| level evenness of a legato scale (dB) | 1.2 | 0.3 | 0.4 | 1 | 1.1 | 0.5 |  |
| level over the keyboard (dB/oct) | 5.5 | 1.76 | 0.98 | 1.14 | 1.35 | 4.3 | yes |
| level against the other instruments (velocity 96) (dB) | -1.7 | 3.6 | -1.5 | 1.2 | 1.2 | 1.1 |  |
| key-to-key level scatter (dB) | 3.58 | 1.15 | 0.42 | 0.94 | 0.77 | 5.5 | yes |
| odd / even partials (dB) | -4.7 | -4.8 | 0.4 | -3.4 | -1.3 | 0.9 |  |
| onset jitter in a phrase (ms) | 33.3 | 24.0 | 37.0 | 42.4 | 64.0 | 0.3 |  |
| onset in a phrase (re the note-on) (ms) | 65.0 | 16.0 | 15.0 | 16.0 | -1 | 5.0 | yes |
| onset sharpness (dB/ms) | 5.9 | 6.5 | 11.7 | 10.7 | 11.4 | 2.0 |  |
| overtone levels, note for note (dB rms) | 11.9 (nearest 11.3) | fonts among themselves 4.84 | | | | 2.4 | yes |
| panning across the keys (dB/oct) | 0.71 | -0 | -0 | -0 | 0.02 | 0.7 | yes |
| partial envelopes alike | 0.028 | 0.012 | 0.465 | 0.125 | 0.886 | 0.6 |  |
| release time (30 dB after note-off) (ms) | 624 | 86.0 | 72.0 | 106 | 78.0 | 10.5 | yes |
| spectrogram (1 - correlation), note for note | 0.302 (nearest 0.261) | fonts among themselves 0.0581 | | | | 5.2 | yes |
| spectral shape of the body, note for note | 12 (nearest 11.1) | fonts among themselves 7.15 | | | | 1.7 | yes |
| sounding length of a 100 ms note (ms) | 225 | 141 | 129 | 136 | 109 | 3.1 | yes |
| partial stretch (inharmonicity) (c/partial) | 0 | 0 | 0 | 0 | -0.01 | 0.0 |  |
| level after 2 s held, re peak (dB) | -7.2 | -4.6 | -7.9 | -8.9 | -12.7 | 0.3 |  |
| high bands lag low bands by (ms) | 0 | 42.0 | 23.3 | 24.7 | 34.7 | 3.2 | yes |
| transient coherence: attack spread over bands (ms) | 4.2 | 18.4 | 5.2 | 6.6 | 8.9 | 0.5 |  |
| fundamental share (tristimulus 1) | 0.539 | 0.054 | 0.043 | 0.029 | 0.051 | 7.0 | yes |
| partials 2-4 share (tristimulus 2) | 0.435 | 0.665 | 0.579 | 0.565 | 0.542 | 2.0 | yes |
| partials 5+ share (tristimulus 3) | 0.012 | 0.078 | 0.168 | 0.163 | 0.247 | 1.8 | yes |
| loudness range, velocity 40 to 127 (dB) | 11.3 | 16.3 | 16.6 | 16.7 | 20.1 | 2.8 | yes |
| pitch wobble / vibrato (p-p) (c) | 6 | 2 | 0 | 1.9 | 7.6 | 0.8 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0124 (nearest 0.0113) | fonts among themselves 0.00439 | | | | 2.8 | yes |

</details>

### 59 Tuba: tuba

Distance 2.64 (rank 78 of 129); the fonts disagree 1.2 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Uneven and flat over the keys (+5.6 dB/oct vs 0.3…1.1; scatter 4.2 vs 0.4…0.6 dB; 0.9 vs 2.9…4.0 ERB/oct)**: mostly a MEASUREMENT ARTIFACT. The probe plays 40–82 against E1–F4 (README "Brass"), so 67–82 fold down and E1–D2 is never heard. Recheck within F2–F4.
- **Speaks late (onset 74 vs 13…28 ms; 56 ms in a phrase)**: DELIBERATE: tuba.md §7.2 gives 80–200 ms at low pitch and says "accept the physics".
- **Wide dynamics (24.1 vs 16.6…20.1 dB)**: DELIBERATE (README "Brass", Dynamics). The fonts' range is TiMidity's law.
- **High bands first (−6.3 vs +1.5…+27 ms), attack 2.9 ERB brighter**: the brass onset pattern of p056.
- **Release 504 ms**: room. Louder by ~3 dB: a small trim.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (504 ms | 90.0 | 71.0 | 111 | 80.0), outside the fonts, 8.0 x.
2. **onset delay**: speaks later (74.0 ms | 13.0 | 13.0 | 16.0 | 28.0), outside the fonts, 6.0 x.
3. **key-to-key level scatter**: less even from key to key (2.6 dB | 0.62 | 0.36 | 0.39 | 0.53), outside the fonts, 4.3 x.
4. **loudness range, velocity 40 to 127**: a wider dynamic range (22.2 dB | 16.6 | 16.6 | 16.7 | 20.1), outside the fonts, 3.2 x.
5. **brightness over the keyboard**: brightens slower up the keyboard (0.9 ERB/oct | 3.08 | 3.95 | 3.94 | 2.94), outside the fonts, 5.2 x.
6. **attack brighter than the body by**: a brighter attack (2.85 ERB | -0.03 | -0.92 | -0.81 | -2.28), outside the fonts, 3.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +3 | -3 | -9 | -0 | +9 | - |
| re TimGM6mb | +26 | +16 | +5 | -6 | -7 | -17 |
| re Jnsgm2 | +29 | +20 | +9 | -4 | -6 | -24 |
| re SGM | +24 | +13 | -1 | -9 | -6 | -9 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -19 -8 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.6 ERB, attack -0.3 ERB, velocity brightening +0.0 ERB, loudness range +4 dB, envelope 1.9 dB rms; TimGM6mb: centroid -0.1 ERB, attack +0.1 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 2.1 dB rms; Jnsgm2: centroid +0.1 ERB, attack -0.3 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 2.3 dB rms
- silent probe notes: FluidR3/fs 16

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 2.85 | -0.03 | -0.92 | -0.81 | -2.28 | 3.3 | yes |
| attack time (-20 to -1 dB) (ms) | 41.0 | 40.0 | 26.0 | 36.0 | 103 | 0.1 |  |
| spectral shape of the attack, note for note | 16.8 (nearest 15.2) | fonts among themselves 11.4 | | | | 1.5 | yes |
| brightness over the keyboard (ERB/oct) | 0.9 | 3.08 | 3.95 | 3.94 | 2.94 | 5.2 | yes |
| key-to-key brightness scatter (ERB) | 0.768 | 0.347 | 0.309 | 0.776 | 0.796 | 0.8 |  |
| brightening from velocity 40 to 127 (ERB) | -0.16 | -0 | -0 | 0 | 0.14 | 0.5 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 8.09 | 7.04 | 12.1 | 12.7 | 10.9 | 1.2 |  |
| onset delay (ms) | 74.0 | 13.0 | 13.0 | 16.0 | 28.0 | 6.0 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.63 | -6.97 | -5.11 | -4.32 | -5.21 | 1.9 | yes |
| envelope curve, note for note (dB rms) | 17.1 (nearest 16.3) | fonts among themselves 5.21 | | | | 3.3 | yes |
| pitch re 12-TET A440 (median) (c) | 3.3 | -1.1 | 2.5 | 1.2 | 0.5 | 0.6 |  |
| pitch scatter from key to key (c) | 2.9 | 0.7 | 0.6 | 0.3 | 0.6 | 0.8 | yes |
| noisiness (spectral flatness) (dB) | -43.3 | -83.0 | -72.4 | -58.8 | -55.0 | 1.6 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -71.0 | -127 | -122 | -122 | -109 | 5.8 | yes |
| pitch at the start re settled (c) | 0 | -2.1 | 0 | -1.7 | 5.6 | 0.2 |  |
| stereo coherence | 0.718 | 1 | 1 | 0.874 | 1 | 3.8 | yes |
| partial-to-partial jaggedness (dB) | 3.1 | 2.4 | 0.8 | 0.5 | 2.6 | 1.4 |  |
| dip between legato notes (dB) | 11.0 | 11.0 | 9.1 | 6.8 | 16.6 | 0.2 |  |
| level evenness of a legato scale (dB) | 2.1 | 0.6 | 1.1 | 0.8 | 1.5 | 1.1 | yes |
| level over the keyboard (dB/oct) | 2.5 | 1.12 | 0.47 | 0.66 | 0.33 | 1.9 | yes |
| level against the other instruments (velocity 96) (dB) | 4.2 | 6.1 | 0.9 | 1.4 | 3.3 | 0.7 |  |
| key-to-key level scatter (dB) | 2.6 | 0.62 | 0.36 | 0.39 | 0.53 | 4.3 | yes |
| odd / even partials (dB) | -6.5 | -14.5 | -2.2 | -1.6 | -11.1 | 0.0 |  |
| onset jitter in a phrase (ms) | 19.3 | 13.6 | 51.6 | 52.4 | 35.8 | 1.3 |  |
| onset in a phrase (re the note-on) (ms) | 52.0 | 9 | -1 | 34.0 | 2 | 2.7 | yes |
| onset sharpness (dB/ms) | 6.7 | 8.4 | 9 | 8.4 | 11.9 | 1.0 | yes |
| overtone levels, note for note (dB rms) | 15.2 (nearest 12.9) | fonts among themselves 16.9 | | | | 0.9 |  |
| panning across the keys (dB/oct) | -0.13 | -0 | -0 | -0 | 0.02 | 0.1 |  |
| partial envelopes alike | 0.013 | -0.036 | 1 | 0.814 | 0.614 | 1.4 |  |
| release time (30 dB after note-off) (ms) | 504 | 90.0 | 71.0 | 111 | 80.0 | 8.0 | yes |
| spectrogram (1 - correlation), note for note | 0.325 (nearest 0.292) | fonts among themselves 0.179 | | | | 1.8 | yes |
| spectral shape of the body, note for note | 14.1 (nearest 11.1) | fonts among themselves 11.2 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 132 | 131 | 109 | 103 | 121 | 0.8 |  |
| partial stretch (inharmonicity) (c/partial) | -0.02 | -0.04 | 0 | 0 | 0.01 | 0.0 |  |
| level after 2 s held, re peak (dB) | -9 | -4.7 | -12.5 | -11.6 | -6.7 | 0.0 |  |
| high bands lag low bands by (ms) | -6.3 | 1.5 | 2.7 | 6.7 | 28.7 | 0.8 | yes |
| transient coherence: attack spread over bands (ms) | 1.8 | 0.9 | 1.6 | 3.6 | 9 | 0.2 |  |
| fundamental share (tristimulus 1) | 0.782 | 0.986 | 0.235 | 0.224 | 0.084 | 1.2 |  |
| partials 2-4 share (tristimulus 2) | 0.212 | 0.014 | 0.673 | 0.66 | 0.898 | 1.0 |  |
| partials 5+ share (tristimulus 3) | 0.002 | 0 | 0.08 | 0.112 | 0.008 | 0.6 |  |
| loudness range, velocity 40 to 127 (dB) | 22.2 | 16.6 | 16.6 | 16.7 | 20.1 | 3.2 | yes |
| pitch wobble / vibrato (p-p) (c) | 12.2 | 0.1 | 0 | 1.2 | 2.1 | 2.3 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0161 (nearest 0.0147) | fonts among themselves 0.00666 | | | | 2.4 | yes |

</details>

### 60 Muted Trumpet: muted-trumpet

Distance 2.41 (rank 85 of 129); the fonts disagree 1.3 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Sounds a tone low**: FAULT (the transposition, as p056). The pitch +31.7 c, wobble 50 c, scatter 15.8 c and fundamental share 0.005 are ARTIFACTS of it.
- **Dark and full below 1 kHz (8.1 vs 10.6…22.3 ERB; +6…+17 dB at 150 Hz–1 kHz re the fonts)**: probable FAULT. A stem-out Harmon should have little below 1 kHz and a bump at 2 kHz (README "Brass": 16–25 dB less below 1 kHz; mutes.md §5 "What to validate"). Under FluidSynth the fonts are 1.5–2.5 ERB darker but still brighter than ours. Part of this is the open trumpet's darkness.
- **Little dynamics (7.8 vs 16.6…21 dB)**: known: soft notes stop near mf (README brass test table). The open trumpet gives 14.4.
- **Key scatter 4.2 dB, legato dip 12.9 vs 6.5…8.9 dB**: probable FAULT. The ~160 Hz parasite makes low notes wobbly (mutes.md §3).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (8.9 dB | 17.4 | 16.7 | 16.6 | 21.0), outside the fonts, 3.7 x.
2. **key-to-key level scatter**: less even from key to key (3.07 dB | 0.62 | 0.9 | 1.12 | 1.12), outside the fonts, 4.1 x.
3. **release time (30 dB after note-off) [room]**: rings longer after the key (378 ms | 75.0 | 55.0 | 110 | 85.0), outside the fonts, 4.5 x.
4. **onset in a phrase (re the note-on)**: speaks later in a phrase (54.0 ms | 2 | 24.0 | 10.0 | 0), outside the fonts, 4.0 x.
5. **onset delay**: speaks later (50.0 ms | 22.0 | 17.0 | 32.0 | 14.0), outside the fonts, 3.0 x.
6. **attack time (-20 to -1 dB)**: a faster attack (22.0 ms | 25.5 | 34.0 | 32.0 | 36.5), outside the fonts, 2.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -10 | +14 | +10 | +6 | -0 | -16 |
| re TimGM6mb | -10 | +11 | +9 | +2 | -4 | -10 |
| re Jnsgm2 | -9 | +14 | +17 | +10 | +3 | -28 |
| re SGM | -17 | +8 | +6 | +2 | -2 | -5 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -6 -3 +0 +2; FluidR3 -13 -7 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -16 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -1.5 ERB, attack +0.7 ERB, velocity brightening +0.8 ERB, loudness range +4 dB, envelope 1.7 dB rms; TimGM6mb: centroid -2.4 ERB, attack -0.1 ERB, velocity brightening +0.2 ERB, loudness range +3 dB, envelope 2.2 dB rms; Jnsgm2: centroid -2.5 ERB, attack +0.9 ERB, velocity brightening +8.5 ERB, loudness range +7 dB, envelope 1.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -1.51 | -2.12 | 1.64 | -3.76 | 0.49 | 0.3 |  |
| attack time (-20 to -1 dB) (ms) | 22.0 | 25.5 | 34.0 | 32.0 | 36.5 | 2.3 | yes |
| spectral shape of the attack, note for note | 15.8 (nearest 13.9) | fonts among themselves 9.19 | | | | 1.7 | yes |
| brightness over the keyboard (ERB/oct) | 3.44 | 3.79 | 4.45 | 8.65 | 2.68 | 0.2 |  |
| key-to-key brightness scatter (ERB) | 1.26 | 1.2 | 0.579 | 4.04 | 1.17 | 0.0 |  |
| brightening from velocity 40 to 127 (ERB) | 0.95 | 3.97 | -0 | -0 | 2.22 | 0.1 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 7.84 | 14.1 | 14.8 | 22.3 | 10.6 | 1.1 | yes |
| onset delay (ms) | 50.0 | 22.0 | 17.0 | 32.0 | 14.0 | 3.0 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.22 | -3.1 | -2.46 | -2.8 | -3.92 | 1.7 | yes |
| envelope curve, note for note (dB rms) | 14.8 (nearest 13.6) | fonts among themselves 5.07 | | | | 2.9 | yes |
| pitch re 12-TET A440 (median) (c) | 1.7 | -0.5 | 2.3 | 1.3 | -0.2 | 0.3 |  |
| pitch scatter from key to key (c) | 3 | 0.6 | 0.9 | 0.6 | 0.1 | 0.8 | yes |
| noisiness (spectral flatness) (dB) | -37.4 | -48.5 | -60.4 | -44.7 | -38.0 | 0.8 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -77.1 | -122 | -119 | -118 | -102 | 4.1 | yes |
| pitch at the start re settled (c) | -4 | -1.3 | -11.7 | 3.3 | 0.1 | 0.5 |  |
| stereo coherence | 0.89 | 1 | 1 | 1 | 0.962 | 1.5 | yes |
| partial-to-partial jaggedness (dB) | 3.9 | 3.4 | 2 | 3 | 3.7 | 0.7 |  |
| dip between legato notes (dB) | 5.8 | 8 | 7.8 | 6.5 | 8.8 | 1.1 |  |
| level evenness of a legato scale (dB) | 1.6 | 1.3 | 0.8 | 1.7 | 2.2 | 0.1 |  |
| level over the keyboard (dB/oct) | 0 | 0.6 | 0.53 | -0.21 | -1.63 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | -0.2 | 0.4 | -5.1 | -0.6 | -5.1 | 1.0 |  |
| key-to-key level scatter (dB) | 3.07 | 0.62 | 0.9 | 1.12 | 1.12 | 4.1 | yes |
| odd / even partials (dB) | -5.4 | -4.6 | -1 | 1.2 | -4.7 | 0.9 |  |
| onset jitter in a phrase (ms) | 29.8 | 48.0 | 36.9 | 55.7 | 17.9 | 0.7 |  |
| onset in a phrase (re the note-on) (ms) | 54.0 | 2 | 24.0 | 10.0 | 0 | 4.0 | yes |
| onset sharpness (dB/ms) | 6 | 7.6 | 8.2 | 5.3 | 38.8 | 0.1 |  |
| overtone levels, note for note (dB rms) | 12.9 (nearest 11.8) | fonts among themselves 10 | | | | 1.3 |  |
| panning across the keys (dB/oct) | -0.03 | -0 | -0 | -0 | -0.42 | 0.0 |  |
| partial envelopes alike | 0.101 | 0.313 | 1 | 0.525 | 0.039 | 0.7 |  |
| release time (30 dB after note-off) (ms) | 378 | 75.0 | 55.0 | 110 | 85.0 | 4.5 | yes |
| spectrogram (1 - correlation), note for note | 0.41 (nearest 0.299) | fonts among themselves 0.16 | | | | 2.6 | yes |
| spectral shape of the body, note for note | 16.5 (nearest 11) | fonts among themselves 12.7 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 178 | 126 | 110 | 133 | 136 | 1.8 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.03 | 0 | 0 | 0.03 | -0.01 | 0.1 |  |
| level after 2 s held, re peak (dB) | -7.8 | -5 | -7.8 | -5 | -7 | 0.6 |  |
| high bands lag low bands by (ms) | 7.6 | 18.4 | 18.0 | 21.5 | 19.0 | 2.2 | yes |
| transient coherence: attack spread over bands (ms) | 2.1 | 7.4 | 10.3 | 15.4 | 9.4 | 1.6 | yes |
| fundamental share (tristimulus 1) | 0.57 | 0.56 | 0.565 | 0.718 | 0.789 | 0.6 |  |
| partials 2-4 share (tristimulus 2) | 0.399 | 0.268 | 0.108 | 0.092 | 0.135 | 3.2 | yes |
| partials 5+ share (tristimulus 3) | 0.024 | 0.168 | 0.326 | 0.152 | 0.086 | 1.1 | yes |
| loudness range, velocity 40 to 127 (dB) | 8.9 | 17.4 | 16.7 | 16.6 | 21.0 | 3.7 | yes |
| pitch wobble / vibrato (p-p) (c) | 7.6 | 2.6 | 0 | 2.8 | 4.2 | 1.0 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0232 (nearest 0.0219) | fonts among themselves 0.00991 | | | | 2.3 | yes |

</details>

### 61 French Horn: horn

Distance 3.51 (rank 33 of 129); the fonts disagree 1.2 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Sounds a fifth low**: FAULT (the transposition, −7). The fundamental share 0, odd/even −24 dB and wobble 33 c are ARTIFACTS: the key's harmonics catch only every third partial.
- **Quiet and weak at the bottom (−7.2 vs +1.1…+5.2 dB; +7.7 vs 0.3…2.0 dB/oct)**: partly the fault, which drops the probe into the horn's weak low register. Recheck after the fix. A trim will likely remain.
- **Narrow range (8.8 vs 16.6 dB)**: known (README brass test table: "many notes hold only a narrow dynamic range").
- **Late (158 vs 64…88 ms)**: README gives 30–60 ms mid-register and ~100 ms at A2. Mostly the transposition.
- **High bands first (−23.5 vs +18…+42 ms), attack 6.2 ERB brighter**: the brass onset pattern (p056), strongest here.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (7 dB | 16.6 | 16.6 | 16.6 | 20.1), outside the fonts, 5.5 x.
2. **attack brighter than the body by**: a brighter attack (5.88 ERB | -0.52 | -0.81 | -0.78 | 0.29), outside the fonts, 11.9 x.
3. **onset delay**: speaks later (146 ms | 88.0 | 73.0 | 64.0 | 73.0), outside the fonts, 6.1 x.
4. **key-to-key level scatter**: less even from key to key (3.25 dB | 0.95 | 0.69 | 1.29 | 1.3), outside the fonts, 4.3 x.
5. **release time (30 dB after note-off) [room]**: rings longer after the key (695 ms | 153 | 285 | 181 | 170), outside the fonts, 4.4 x.
6. **onset in a phrase (re the note-on)**: speaks later in a phrase (62.0 ms | 6 | 12.0 | 0 | 26.0), outside the fonts, 4.1 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +9 | +4 | -0 | +0 | -6 | +0 |
| re TimGM6mb | +4 | +2 | +1 | -0 | -3 | -2 |
| re Jnsgm2 | +11 | +5 | -0 | -1 | -5 | -2 |
| re SGM | +6 | +1 | -0 | +4 | -3 | -6 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -7 -3 +0 +0; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.1 ERB, velocity brightening +0.3 ERB, loudness range +3 dB, envelope 4.5 dB rms; TimGM6mb: centroid -0.0 ERB, attack +0.0 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 2.8 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening +0.0 ERB, loudness range +3 dB, envelope 1.8 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 5.88 | -0.52 | -0.81 | -0.78 | 0.29 | 11.9 | yes |
| attack time (-20 to -1 dB) (ms) | 187 | 225 | 161 | 394 | 279 | 0.7 |  |
| spectral shape of the attack, note for note | 15.8 (nearest 15.4) | fonts among themselves 7.09 | | | | 2.2 | yes |
| brightness over the keyboard (ERB/oct) | 0.52 | 1.42 | 2.13 | 1.2 | 1.44 | 2.0 | yes |
| key-to-key brightness scatter (ERB) | 0.97 | 0.968 | 0.605 | 0.805 | 0.494 | 1.1 |  |
| brightening from velocity 40 to 127 (ERB) | 0.3 | 0.36 | -0 | -0 | 0.11 | 0.8 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 9.4 | 10.9 | 10.9 | 11.1 | 9.27 | 1.7 |  |
| onset delay (ms) | 146 | 88.0 | 73.0 | 64.0 | 73.0 | 6.1 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.07 | -4.4 | -3.49 | -3.56 | -5.57 | 1.8 | yes |
| envelope curve, note for note (dB rms) | 16.6 (nearest 13.7) | fonts among themselves 6.04 | | | | 2.7 | yes |
| pitch re 12-TET A440 (median) (c) | 1.1 | 3.6 | 1 | -2.7 | 1.5 | 0.0 |  |
| pitch scatter from key to key (c) | 3.5 | 2.5 | 4.2 | 3.3 | 6.8 | 0.1 |  |
| noisiness (spectral flatness) (dB) | -38.2 | -45.9 | -48.4 | -44.2 | -47.1 | 4.0 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -68.3 | -128 | -128 | -124 | -109 | 6.1 | yes |
| pitch at the start re settled (c) | 33.5 | -10.0 | 5.8 | -9.3 | 0.9 | 4.8 | yes |
| stereo coherence | 0.405 | 0.705 | 0.949 | 0.223 | 0.519 | 0.6 |  |
| partial-to-partial jaggedness (dB) | 2.75 | 1.9 | 1.8 | 2 | 2.3 | 0.8 |  |
| dip between legato notes (dB) | 10.4 | 15.5 | 12.6 | 9.4 | 10.0 | 0.3 |  |
| level evenness of a legato scale (dB) | 1.6 | 0.6 | 1.4 | 1.3 | 1.6 | 0.2 |  |
| level over the keyboard (dB/oct) | 2.97 | 0.58 | 2.04 | 0.33 | 0.54 | 2.4 | yes |
| level against the other instruments (velocity 96) (dB) | 0.8 | 2.9 | 5.3 | 1.9 | 1.4 | 0.8 | yes |
| key-to-key level scatter (dB) | 3.25 | 0.95 | 0.69 | 1.29 | 1.3 | 4.3 | yes |
| odd / even partials (dB) | -9.2 | -5 | -5.1 | -4 | -5.3 | 2.1 | yes |
| onset jitter in a phrase (ms) | 43.5 | 52.8 | 39.0 | 48.6 | 38.2 | 0.0 |  |
| onset in a phrase (re the note-on) (ms) | 62.0 | 6 | 12.0 | 0 | 26.0 | 4.1 | yes |
| onset sharpness (dB/ms) | 5.8 | 8.9 | 7.6 | 7.6 | 9.9 | 1.2 | yes |
| overtone levels, note for note (dB rms) | 7.47 (nearest 7.21) | fonts among themselves 5.49 | | | | 1.4 | yes |
| panning across the keys (dB/oct) | 0.37 | -0.64 | -0.95 | 1.42 | 0.01 | 0.6 |  |
| partial envelopes alike | 0.071 | -0.024 | 0.058 | -0.017 | 0.005 | 0.8 |  |
| release time (30 dB after note-off) (ms) | 695 | 153 | 285 | 181 | 170 | 4.4 | yes |
| spectrogram (1 - correlation), note for note | 0.185 (nearest 0.14) | fonts among themselves 0.0767 | | | | 2.4 | yes |
| spectral shape of the body, note for note | 7.54 (nearest 7.03) | fonts among themselves 6.13 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 152 | 130 | 132 | 152 | 211 | 0.3 |  |
| partial stretch (inharmonicity) (c/partial) | -0.17 | 0.2 | -0.2 | -0.59 | 0.23 | 0.3 |  |
| level after 2 s held, re peak (dB) | -8.1 | -9.6 | -9.9 | -8.8 | -7 | 0.4 |  |
| high bands lag low bands by (ms) | 2 | 42.3 | 17.7 | 22.0 | 26.0 | 1.8 | yes |
| transient coherence: attack spread over bands (ms) | 5 | 20.2 | 8.6 | 11.5 | 9.1 | 0.9 | yes |
| fundamental share (tristimulus 1) | 0.44 | 0.151 | 0.346 | 0.397 | 0.857 | 0.2 |  |
| partials 2-4 share (tristimulus 2) | 0.349 | 0.572 | 0.597 | 0.453 | 0.138 | 0.7 |  |
| partials 5+ share (tristimulus 3) | 0.009 | 0.009 | 0.013 | 0.005 | 0.005 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 7 | 16.6 | 16.6 | 16.6 | 20.1 | 5.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 29.8 | 15.2 | 12.1 | 15.6 | 14.0 | 3.0 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0135 (nearest 0.0135) | fonts among themselves 0.0104 | | | | 1.3 |  |

</details>

### 62 Brass Section: brass-section

Distance 3.48 (rank 35 of 129); the fonts disagree 1.4 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Almost no dynamics (velocity 40->127: 5 dB vs 17-20; curve -3/0/0/+2)**: FAULT. It contradicts brass_section.md 14, check 5 (13 dB over 40->127, `PJ_BRASS_VELDYN`). Something on the `-play` path (the plan or the section's gain) loses velocity. Check it first.
- **Much louder toward the top (7.6 dB/oct vs -0.1..0.8)**: partly DELIBERATE voicing (a low lone key gets only trombones; README "Brass section", "A lone key"). Combined with each player's f^1.45 breath it is too steep: probable FAULT.
- **Bass-heavy and dull (+14..+17 dB under 150 Hz, -8..-14 dB above 6 kHz; fundamental share 0.47 vs 0.14-0.27)**: FAULT, as in p057. It agrees with brass_section.md 14's FluidR3 comparison (centroid 0.84 vs 1.5 kHz).
- **Release 952 ms; envelope far from the fonts**: mostly the room and the bells' tails (ARTIFACT). The fonts' samples sound at once.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (4.8 dB | 16.9 | 16.6 | 16.7 | 20.1), outside the fonts, 6.9 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (920 ms | 142 | 70.0 | 103 | 76.0), outside the fonts, 6.6 x.
3. **level over the keyboard**: louder toward the top (7.6 dB/oct | 0.83 | 0.42 | -0.11 | 0.17), outside the fonts, 7.3 x.
4. **onset in a phrase (re the note-on)**: speaks later in a phrase (70.0 ms | 0 | 4 | 25.0 | 6), outside the fonts, 5.2 x.
5. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 23.5, nearest 22.3; the fonts among themselves 4.79), outside.
6. **level against the other instruments (velocity 96)**: louder (4.1 dB | 1.2 | -4.4 | 0.1 | -5.5), outside the fonts, 1.9 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +17 | +10 | +0 | -3 | -6 | -7 |
| re TimGM6mb | +14 | +6 | +0 | -3 | -5 | -2 |
| re Jnsgm2 | +2 | +8 | +6 | +4 | +1 | -12 |
| re SGM | +14 | +12 | +4 | +2 | -3 | -14 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -2 -0 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.5 ERB, attack -0.1 ERB, velocity brightening +1.1 ERB, loudness range +5 dB, envelope 1.9 dB rms; TimGM6mb: centroid -0.3 ERB, attack -0.0 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 1.7 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.2 ERB, velocity brightening +0.2 ERB, loudness range +3 dB, envelope 2.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 1.87 | -0.25 | 1.05 | 0.6 | -0.93 | 1.7 | yes |
| attack time (-20 to -1 dB) (ms) | 184 | 306 | 84.0 | 68.0 | 133 | 0.7 |  |
| spectral shape of the attack, note for note | 14.1 (nearest 11.9) | fonts among themselves 11.4 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 3.08 | 1.37 | 2.95 | 3.48 | 3.37 | 0.1 |  |
| key-to-key brightness scatter (ERB) | 1.37 | 1.34 | 1.14 | 0.521 | 0.733 | 1.1 |  |
| brightening from velocity 40 to 127 (ERB) | 1.83 | 1.43 | -0 | -0 | 0.2 | 2.4 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 10.7 | 16.5 | 15.7 | 11.2 | 14.3 | 1.6 | yes |
| onset delay (ms) | 90.0 | 60.0 | 25.0 | 40.0 | 46.0 | 2.7 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.49 | -2.69 | -2.28 | -2.51 | -2.39 | 0.1 |  |
| envelope curve, note for note (dB rms) | 23.5 (nearest 22.3) | fonts among themselves 4.79 | | | | 4.9 | yes |
| pitch re 12-TET A440 (median) (c) | 1.3 | 2.2 | 5.9 | -1 | 7.1 | 0.7 |  |
| pitch scatter from key to key (c) | 7.1 | 1.9 | 1.9 | 0.9 | 5.5 | 1.7 | yes |
| noisiness (spectral flatness) (dB) | -32.8 | -33.5 | -46.8 | -26.4 | -31.0 | 0.1 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -58.2 | -126 | -120 | -123 | -103 | 5.5 | yes |
| pitch at the start re settled (c) | 0 | -0.8 | -9 | -0.8 | -6.4 | 0.7 |  |
| stereo coherence | 0.238 | 0.959 | 1 | 1 | 0.225 | 1.9 |  |
| partial-to-partial jaggedness (dB) | 2.5 | 1.5 | 1.8 | 2.1 | 2.2 | 0.5 |  |
| dip between legato notes (dB) | 9.1 | 8.9 | 8.1 | 12.8 | 10.3 | 0.2 |  |
| level evenness of a legato scale (dB) | 2.3 | 0.9 | 0.8 | 0.5 | 2.8 | 1.3 |  |
| level over the keyboard (dB/oct) | 7.6 | 0.83 | 0.42 | -0.11 | 0.17 | 7.3 | yes |
| level against the other instruments (velocity 96) (dB) | 4.1 | 1.2 | -4.4 | 0.1 | -5.5 | 1.9 | yes |
| key-to-key level scatter (dB) | 2.53 | 1.31 | 0.6 | 0.1 | 1.54 | 2.2 | yes |
| odd / even partials (dB) | -3.5 | -0.3 | -1.9 | -7.8 | -0.2 | 0.6 |  |
| onset jitter in a phrase (ms) | 30.7 | 52.0 | 44.1 | 38.2 | 43.2 | 1.9 | yes |
| onset in a phrase (re the note-on) (ms) | 70.0 | 0 | 4 | 25.0 | 6 | 5.2 | yes |
| onset sharpness (dB/ms) | 7 | 8.7 | 9.5 | 9.4 | 6.9 | 1.0 |  |
| overtone levels, note for note (dB rms) | 8.51 (nearest 7.32) | fonts among themselves 6 | | | | 1.4 |  |
| panning across the keys (dB/oct) | 0.17 | 0.07 | -0 | -0 | -0.3 | 0.2 |  |
| partial envelopes alike | 0.007 | 0.001 | -0.036 | 0.043 | 0.028 | 0.1 |  |
| release time (30 dB after note-off) (ms) | 920 | 142 | 70.0 | 103 | 76.0 | 6.6 | yes |
| spectrogram (1 - correlation), note for note | 0.48 (nearest 0.42) | fonts among themselves 0.229 | | | | 2.1 | yes |
| spectral shape of the body, note for note | 13.1 (nearest 10.6) | fonts among themselves 12.1 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 207 | 148 | 123 | 103 | 123 | 2.9 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.01 | 0.47 | -0.01 | -0.14 | -0.09 | 0.1 |  |
| level after 2 s held, re peak (dB) | -8.6 | -8.6 | -11.2 | -9.2 | -8.1 | 0.1 |  |
| high bands lag low bands by (ms) | -21.6 | 15.3 | -0.7 | 11.3 | -5.5 | 2.6 | yes |
| transient coherence: attack spread over bands (ms) | 13.9 | 12.1 | 4.2 | 18.1 | 11.2 | 0.3 |  |
| fundamental share (tristimulus 1) | 0.454 | 0.137 | 0.266 | 0.249 | 0.199 | 3.3 | yes |
| partials 2-4 share (tristimulus 2) | 0.489 | 0.385 | 0.406 | 0.653 | 0.634 | 0.2 |  |
| partials 5+ share (tristimulus 3) | 0.032 | 0.186 | 0.319 | 0.098 | 0.183 | 1.4 | yes |
| loudness range, velocity 40 to 127 (dB) | 4.8 | 16.9 | 16.6 | 16.7 | 20.1 | 6.9 | yes |
| pitch wobble / vibrato (p-p) (c) | 17.4 | 8 | 7.4 | 8.7 | 9.7 | 1.8 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0207 (nearest 0.0188) | fonts among themselves 0.0145 | | | | 1.4 |  |

</details>

### 63 Synth Brass 1: synth-brass-1

Distance 1.92 (rank 108 of 129); the fonts disagree 1.2 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Brightens with velocity (1.88 ERB vs ~0, the same under FluidSynth)**: a DELIBERATE analog patch choice, since velocity opens the filter (synth_leads.md 10). The fonts have no velocity-to-filter here. Harmless.
- **Late speech (onset 32 ms, 42 ms in a phrase vs 0-12)**: mild FAULT for a synth. The circuit should speak at once; the filter opening late (60 ms) is DELIBERATE, the level's late start is not.
- **Uneven from key to key (2.2 dB vs 0.1-1.4)**: per-voice analog spread (README "Synth leads", "Each voice its own"). DELIBERATE, but on the large side.
- **Release 622 ms, staccato 239 ms**: room plus the RC release (ARTIFACT). Overall close to the fonts (rank 50).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (1.88 ERB | 0 | -0 | -0 | -0.05), outside the fonts, 6.3 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (622 ms | 136 | 142 | 91.0 | 154), outside the fonts, 5.7 x.
3. **key-to-key level scatter**: less even from key to key (1.91 dB | 0.29 | 0.57 | 0.11 | 1.42), outside the fonts, 2.3 x.
4. **sounding length of a 100 ms note [room]**: longer staccato (239 ms | 132 | 142 | 118 | 142), outside the fonts, 3.2 x.
5. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 18, nearest 16.6; the fonts among themselves 6.05), outside.
6. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-67.2 dB | -126 | -124 | -121 | -103), outside the fonts, 4.8 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +4 | -0 | +0 | +2 | +1 | -9 |
| re TimGM6mb | +12 | +6 | +8 | +6 | -4 | -19 |
| re Jnsgm2 | +3 | +9 | +11 | +10 | -0 | -22 |
| re SGM | +11 | +7 | +10 | +7 | -1 | -20 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.3 ERB, attack +0.0 ERB, velocity brightening +0.0 ERB, loudness range +4 dB, envelope 2.1 dB rms; TimGM6mb: centroid +0.0 ERB, attack -0.2 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 3.4 dB rms; Jnsgm2: centroid -1.0 ERB, attack -0.1 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 3.9 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.34 | -0.74 | -0.37 | 0.51 | -0.59 | 0.2 |  |
| attack time (-20 to -1 dB) (ms) | 293 | 7 | 76.0 | 398 | 82.0 | 0.6 |  |
| spectral shape of the attack, note for note | 13.3 (nearest 11.8) | fonts among themselves 10.2 | | | | 1.3 |  |
| brightness over the keyboard (ERB/oct) | 3.24 | 3.1 | 4.28 | 3.39 | 3.41 | 0.3 |  |
| key-to-key brightness scatter (ERB) | 1.34 | 0.541 | 0.86 | 0.217 | 1.04 | 1.6 | yes |
| brightening from velocity 40 to 127 (ERB) | 1.88 | 0 | -0 | -0 | -0.05 | 6.3 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 9.77 | 9.45 | 9.26 | 9.79 | 11.4 | 0.1 |  |
| onset delay (ms) | 32.0 | 5 | 6 | 4 | 29.0 | 2.1 |  |
| Plomp-Levelt dissonance of a note (log2) | -4.99 | -4.32 | -3.63 | -3.78 | -3.22 | 2.3 | yes |
| envelope curve, note for note (dB rms) | 18 (nearest 16.6) | fonts among themselves 6.05 | | | | 3.0 | yes |
| pitch re 12-TET A440 (median) (c) | -0.2 | -4.1 | 0.8 | -3.3 | 1.9 | 0.3 |  |
| pitch scatter from key to key (c) | 1 | 9.6 | 3 | 2.7 | 5.2 | 0.9 | yes |
| noisiness (spectral flatness) (dB) | -63.0 | -42.2 | -48.7 | -62.8 | -33.0 | 1.2 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -67.2 | -126 | -124 | -121 | -103 | 4.8 | yes |
| pitch at the start re settled (c) | 5.2 | 2.2 | -4.4 | -0.9 | -4.8 | 1.6 | yes |
| stereo coherence | 0.675 | 0.653 | 1 | 1 | 0.246 | 0.4 |  |
| partial-to-partial jaggedness (dB) | 2.2 | 2.1 | 1.5 | 0.2 | 0.9 | 1.0 |  |
| dip between legato notes (dB) | 11.3 | 8.1 | 12.3 | 9.5 | 9.7 | 0.8 |  |
| level evenness of a legato scale (dB) | 2.3 | 0.5 | 1.7 | 1.4 | 1.5 | 0.8 | yes |
| level over the keyboard (dB/oct) | -0.02 | -1.29 | -0.95 | 0.22 | 0.4 | 0.3 |  |
| level against the other instruments (velocity 96) (dB) | 1.3 | 0.8 | 3.1 | 0.9 | -3.8 | 0.1 |  |
| key-to-key level scatter (dB) | 1.91 | 0.29 | 0.57 | 0.11 | 1.42 | 2.3 | yes |
| odd / even partials (dB) | -6 | -4.3 | -3 | -2.7 | -4.6 | 1.2 | yes |
| onset jitter in a phrase (ms) | 44.1 | 26.6 | 39.4 | 42.1 | 43.9 | 0.4 |  |
| onset in a phrase (re the note-on) (ms) | 24.0 | -0 | 12.0 | -0 | 8 | 2.0 | yes |
| onset sharpness (dB/ms) | 6.2 | 8.8 | 7.4 | 8 | 7.3 | 0.8 | yes |
| overtone levels, note for note (dB rms) | 9.94 (nearest 9.18) | fonts among themselves 5.16 | | | | 1.9 | yes |
| panning across the keys (dB/oct) | 0.74 | 0.63 | -0 | -0 | 0.33 | 0.6 |  |
| partial envelopes alike | -0.002 | 0.044 | -0.025 | -0.014 | 0.115 | 0.2 |  |
| release time (30 dB after note-off) (ms) | 622 | 136 | 142 | 91.0 | 154 | 5.7 | yes |
| spectrogram (1 - correlation), note for note | 0.456 (nearest 0.312) | fonts among themselves 0.201 | | | | 2.3 | yes |
| spectral shape of the body, note for note | 12.4 (nearest 8.7) | fonts among themselves 10.3 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 239 | 132 | 142 | 118 | 142 | 3.2 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.01 | 0.06 | 0.01 | 0 | 0.06 | 0.1 |  |
| level after 2 s held, re peak (dB) | -6.6 | -14.2 | -8.9 | -6.1 | -10.6 | 0.8 |  |
| high bands lag low bands by (ms) | 26.6 | 10.8 | -1 | 0 | 9 | 3.7 | yes |
| transient coherence: attack spread over bands (ms) | 12.2 | 2.7 | 2.5 | 0 | 14.4 | 1.3 |  |
| fundamental share (tristimulus 1) | 0.79 | 0.672 | 0.715 | 0.64 | 0.499 | 1.2 | yes |
| partials 2-4 share (tristimulus 2) | 0.191 | 0.299 | 0.232 | 0.287 | 0.46 | 0.9 | yes |
| partials 5+ share (tristimulus 3) | 0.006 | 0.024 | 0.053 | 0.1 | 0.059 | 0.7 |  |
| loudness range, velocity 40 to 127 (dB) | 15.7 | 16.6 | 16.7 | 16.7 | 20.1 | 0.6 | yes |
| pitch wobble / vibrato (p-p) (c) | 5.5 | 16.3 | 12.2 | 1.9 | 13.7 | 1.0 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0214 (nearest 0.0205) | fonts among themselves 0.0133 | | | | 1.6 | yes |

</details>

### 64 Synth Brass 2: synth-brass-2

Distance 2.53 (rank 82 of 129); the fonts disagree 1.2 JND. Keys 40,43,46,49,52,55,58,61,64,67,70,73,76,79,82, velocities 40/64/96/127.

- **Uneven from key to key (3.0 dB vs 0.1-1.4)**: probable FAULT. Larger than the analog spread needs.
- **Brightens with velocity (1.49 ERB vs ~0)**: DELIBERATE, as in p062.
- **Late speech (onset 42 ms, 40 ms in a phrase vs 0-5); legato dips 13.8 dB vs 8.4-9.4**: mild FAULT. The legato retriggers and dips more than any font.
- **More even partials (-12 dB odd/even vs -3..-6), smoother**: DELIBERATE (ramp plus wide pulse, ladder offsets; synth_leads.md 10).
- **Release 756 ms**: the room (ARTIFACT).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (756 ms | 122 | 144 | 91.0 | 154), outside the fonts, 6.6 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (1.49 ERB | -0 | -0 | -0.01 | -0.08), outside the fonts, 5.0 x.
3. **key-to-key level scatter**: less even from key to key (2.31 dB | 0.22 | 0.2 | 0.12 | 1.37), outside the fonts, 3.4 x.
4. **Plomp-Levelt dissonance of a note**: smoother (-6.51 log2 | -4.51 | -3.37 | -4.38 | -3.84), outside the fonts, 4.2 x.
5. **onset in a phrase (re the note-on)**: speaks later in a phrase (42.0 ms | -0 | -0 | 5 | 0), outside the fonts, 4.2 x.
6. **odd / even partials**: more even partials (-12.1 dB | -3.9 | -2.7 | -2.9 | -5.7), outside the fonts, 4.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +4 | +2 | +0 | -1 | +2 | -8 |
| re TimGM6mb | +7 | +4 | +2 | -4 | -2 | -1 |
| re Jnsgm2 | -5 | +4 | +3 | -2 | +3 | -8 |
| re SGM | +3 | +2 | +2 | -4 | +0 | -0 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.2 ERB, attack -0.6 ERB, velocity brightening +0.0 ERB, loudness range +4 dB, envelope 3.0 dB rms; TimGM6mb: centroid -0.7 ERB, attack -0.1 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 2.9 dB rms; Jnsgm2: centroid -1.2 ERB, attack -0.1 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 4.0 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.21 | -0.33 | -1.07 | 0.39 | -0.52 | 0.3 |  |
| attack time (-20 to -1 dB) (ms) | 168 | 19.5 | 30.0 | 393 | 82.0 | 0.8 |  |
| spectral shape of the attack, note for note | 13.4 (nearest 12.7) | fonts among themselves 9.77 | | | | 1.4 |  |
| brightness over the keyboard (ERB/oct) | 3.54 | 2.97 | 3.61 | 2.97 | 3.14 | 1.5 |  |
| key-to-key brightness scatter (ERB) | 1.13 | 0.515 | 0.366 | 0.205 | 0.958 | 1.8 | yes |
| brightening from velocity 40 to 127 (ERB) | 1.49 | -0 | -0 | -0.01 | -0.08 | 5.0 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 8.59 | 9.45 | 10.2 | 9.13 | 10.8 | 1.5 | yes |
| onset delay (ms) | 42.0 | 10.0 | 9 | 5 | 29.0 | 2.7 | yes |
| Plomp-Levelt dissonance of a note (log2) | -6.51 | -4.51 | -3.37 | -4.38 | -3.84 | 4.2 | yes |
| envelope curve, note for note (dB rms) | 19.2 (nearest 18.4) | fonts among themselves 5.77 | | | | 3.3 | yes |
| pitch re 12-TET A440 (median) (c) | -0.1 | -2 | -0.8 | -3.3 | 1.9 | 0.3 |  |
| pitch scatter from key to key (c) | 0.4 | 6.8 | 8.3 | 2.7 | 5.2 | 1.9 | yes |
| noisiness (spectral flatness) (dB) | -65.8 | -44.5 | -55.0 | -70.2 | -42.2 | 1.1 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -63.9 | -126 | -124 | -121 | -104 | 5.3 | yes |
| pitch at the start re settled (c) | 7.3 | -1.2 | -9.5 | -0.9 | -4.8 | 2.1 | yes |
| stereo coherence | 0.613 | 0.604 | 1 | 1 | 0.258 | 0.5 |  |
| partial-to-partial jaggedness (dB) | 5.05 | 1.9 | 1.4 | 0.2 | 0.9 | 3.9 | yes |
| dip between legato notes (dB) | 14.0 | 8.5 | 8.4 | 9.2 | 10.0 | 2.6 | yes |
| level evenness of a legato scale (dB) | 2.8 | 0.5 | 1.7 | 1.6 | 1.1 | 1.4 | yes |
| level over the keyboard (dB/oct) | -0.1 | -1.24 | 0.16 | 0.15 | 0.67 | 0.3 |  |
| level against the other instruments (velocity 96) (dB) | 1.8 | 1.2 | 1.6 | 0.6 | -2.6 | 0.4 |  |
| key-to-key level scatter (dB) | 2.31 | 0.22 | 0.2 | 0.12 | 1.37 | 3.4 | yes |
| odd / even partials (dB) | -12.1 | -3.9 | -2.7 | -2.9 | -5.7 | 4.3 | yes |
| onset jitter in a phrase (ms) | 37.3 | 5.8 | 59.4 | 49.4 | 40.2 | 0.3 |  |
| onset in a phrase (re the note-on) (ms) | 42.0 | -0 | -0 | 5 | 0 | 4.2 | yes |
| onset sharpness (dB/ms) | 4.3 | 8.2 | 10.8 | 8.6 | 7.5 | 2.0 | yes |
| overtone levels, note for note (dB rms) | 14.3 (nearest 11.5) | fonts among themselves 5.1 | | | | 2.8 | yes |
| panning across the keys (dB/oct) | 0.47 | 0.41 | -0 | -0 | 0.32 | 0.3 |  |
| partial envelopes alike | 0.236 | 0.151 | -0.035 | -0.014 | 0.115 | 1.9 | yes |
| release time (30 dB after note-off) (ms) | 756 | 122 | 144 | 91.0 | 154 | 6.6 | yes |
| spectrogram (1 - correlation), note for note | 0.365 (nearest 0.307) | fonts among themselves 0.161 | | | | 2.3 | yes |
| spectral shape of the body, note for note | 11.6 (nearest 9) | fonts among themselves 9.2 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 194 | 127 | 114 | 118 | 137 | 2.7 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.01 | 0.24 | -0.11 | 0 | -0.19 | 0.1 |  |
| level after 2 s held, re peak (dB) | -5.3 | -13.9 | -7.4 | -6.2 | -10.6 | 1.0 |  |
| high bands lag low bands by (ms) | 31.5 | 11.0 | 5 | 0 | 6 | 4.7 | yes |
| transient coherence: attack spread over bands (ms) | 9.7 | 3.6 | 4.9 | 0 | 14.0 | 0.8 |  |
| fundamental share (tristimulus 1) | 0.877 | 0.771 | 0.697 | 0.674 | 0.55 | 1.7 | yes |
| partials 2-4 share (tristimulus 2) | 0.11 | 0.222 | 0.249 | 0.274 | 0.431 | 1.4 | yes |
| partials 5+ share (tristimulus 3) | 0.002 | 0.011 | 0.063 | 0.067 | 0.031 | 0.6 |  |
| loudness range, velocity 40 to 127 (dB) | 15.7 | 16.6 | 16.6 | 16.7 | 20.1 | 0.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 4.7 | 14.8 | 16.7 | 1.9 | 13.9 | 1.3 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0186 (nearest 0.017) | fonts among themselves 0.0146 | | | | 1.3 |  |

</details>

### 65 Soprano Sax: soprano-sax

Distance 2.90 (rank 56 of 129); the fonts disagree 1.4 JND. Keys 48,51,54,57,60,63,66,69,72,75,78,81,84, velocities 40/64/96/127.

- **Wrong pitch: sounds a tone low**: FAULT, the biggest for all transposing winds. Fundamental share 0 and high partials 0.999 against the key's harmonic series. src/midifile.cc plays `noteOn(pk + inst->transpose)`, so a GM concert-pitch key is taken as written (soprano-sax transpose -2).
- **Pitch +31.5 c, key scatter 27 c, wobble 103 c (fonts 0-16)**: re-measure after the transposition fix. The pitch tracker is reading a note a tone off. If any of it stays, it is a FAULT.
- **Slow speech (onset 88 ms vs 25-45; attack 226 ms vs 21-81)**: FAULT. Compare the bassoon's priming fix (README "Other woodwinds": rise 20 ms).
- **Noisy (flatness -26 dB vs -34..-57)**: breathier than every font.
- **Release 782 ms, staccato 400 ms**: the room (ARTIFACT).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (763 ms | 87.0 | 76.0 | 79.0 | 86.0), outside the fonts, 12.8 x.
2. **onset in a phrase (re the note-on)**: speaks later in a phrase (85.0 ms | 8 | 0 | -1 | 0), outside the fonts, 8.5 x.
3. **onset delay**: speaks later (99.0 ms | 28.0 | 25.0 | 34.0 | 45.0), outside the fonts, 6.8 x.
4. **sounding length of a 100 ms note [room]**: longer staccato (400 ms | 130 | 128 | 120 | 128), outside the fonts, 6.6 x.
5. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 20.9, nearest 20.8; the fonts among themselves 3.79), outside.
6. **key-to-key level scatter**: less even from key to key (2.31 dB | 1.07 | 1.1 | 0.94 | 0.96), outside the fonts, 2.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -55 | -22 | -13 | -8 | +7 | +29 |
| re TimGM6mb | -54 | -20 | -7 | -6 | -5 | +32 |
| re Jnsgm2 | -59 | -22 | -9 | +2 | +8 | +16 |
| re SGM | -58 | -24 | -13 | -2 | +9 | +24 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +4; FluidR3 -12 -5 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.1 ERB, attack -0.2 ERB, velocity brightening +2.3 ERB, loudness range +5 dB, envelope 2.9 dB rms; TimGM6mb: centroid +0.1 ERB, attack -0.0 ERB, velocity brightening +0.4 ERB, loudness range +3 dB, envelope 1.9 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.1 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 1.8 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.72 | 0.56 | -0.45 | -0.6 | -0.49 | 0.4 |  |
| attack time (-20 to -1 dB) (ms) | 188 | 29.0 | 21.0 | 81.0 | 56.0 | 2.3 | yes |
| spectral shape of the attack, note for note | 15.2 (nearest 12.8) | fonts among themselves 13.2 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | 0.02 | 2.29 | 3.72 | 3.96 | 3.37 | 4.2 | yes |
| key-to-key brightness scatter (ERB) | 1.69 | 1.18 | 1.22 | 0.863 | 0.558 | 2.0 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.08 | -0.78 | -0 | 0 | -0.07 | 0.3 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 19.1 | 15.4 | 20.0 | 12.0 | 12.8 | 1.2 |  |
| decay rate of a held note (dB/s) | 0.1 | 2.7 | 0.4 | 0.8 | 1.1 | 2.3 | yes |
| onset delay (ms) | 99.0 | 28.0 | 25.0 | 34.0 | 45.0 | 6.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.24 | -5.62 | -4.16 | -4.8 | -5.11 | 2.3 | yes |
| envelope curve, note for note (dB rms) | 20.9 (nearest 20.8) | fonts among themselves 3.79 | | | | 5.5 | yes |
| pitch re 12-TET A440 (median) (c) | -0.3 | 3.3 | 0 | 1.2 | -1.5 | 0.2 |  |
| pitch scatter from key to key (c) | 0.9 | 4.4 | 1.5 | 4 | 0.7 | 0.6 |  |
| noisiness (spectral flatness) (dB) | -26.1 | -57.0 | -40.9 | -34.3 | -46.4 | 1.5 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -64.5 | -126 | -120 | -122 | -109 | 6.5 | yes |
| pitch at the start re settled (c) | -0.8 | -10.7 | -1.3 | -19.6 | -1.8 | 0.6 |  |
| stereo coherence | 0.207 | 0.776 | 1 | 1 | 1 | 7.1 | yes |
| partial-to-partial jaggedness (dB) | 3.9 | 2.7 | 2 | 2.7 | 4.2 | 1.1 |  |
| dip between legato notes (dB) | 5.4 | 6.3 | 7.2 | 13.1 | 13.6 | 1.3 |  |
| level evenness of a legato scale (dB) | 3.2 | 0.7 | 1.1 | 1.2 | 1.6 | 2.1 | yes |
| level over the keyboard (dB/oct) | 1.39 | 0.79 | 0.75 | 0.89 | 2.09 | 0.5 |  |
| level against the other instruments (velocity 96) (dB) | -0.7 | 3.8 | -0.9 | 2.1 | 3.4 | 1.5 |  |
| key-to-key level scatter (dB) | 2.31 | 1.07 | 1.1 | 0.94 | 0.96 | 2.6 | yes |
| odd / even partials (dB) | -2.1 | -6.8 | -2.5 | -6.6 | -10.8 | 1.1 |  |
| onset jitter in a phrase (ms) | 37.6 | 25.5 | 33.7 | 19.0 | 20.3 | 2.0 | yes |
| onset in a phrase (re the note-on) (ms) | 85.0 | 8 | 0 | -1 | 0 | 8.5 | yes |
| onset sharpness (dB/ms) | 4.5 | 5.2 | 5.6 | 5 | 6.9 | 0.5 |  |
| overtone levels, note for note (dB rms) | 15.8 (nearest 11.8) | fonts among themselves 10.4 | | | | 1.5 |  |
| panning across the keys (dB/oct) | 0.69 | -0.03 | -0 | 0 | -0.05 | 0.7 | yes |
| partial envelopes alike | -0.008 | 0.546 | 0.238 | 0.303 | 0.775 | 1.6 | yes |
| release time (30 dB after note-off) (ms) | 763 | 87.0 | 76.0 | 79.0 | 86.0 | 12.8 | yes |
| spectrogram (1 - correlation), note for note | 0.486 (nearest 0.419) | fonts among themselves 0.151 | | | | 3.2 | yes |
| spectral shape of the body, note for note | 15 (nearest 10.6) | fonts among themselves 10.9 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 400 | 130 | 128 | 120 | 128 | 6.6 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.02 | 0 | 0 | -0.1 | 0 | 0.0 |  |
| level after 2 s held, re peak (dB) | -3.7 | -11.1 | -7.8 | -6.3 | -6.7 | 1.2 | yes |
| high bands lag low bands by (ms) | -15.4 | 1.5 | 4.2 | 6.3 | 6 | 4.1 | yes |
| transient coherence: attack spread over bands (ms) | 19.7 | 3.2 | 1.9 | 3.3 | 2.3 | 3.4 | yes |
| fundamental share (tristimulus 1) | 0.063 | 0.28 | 0.133 | 0.852 | 0.347 | 0.7 | yes |
| partials 2-4 share (tristimulus 2) | 0.291 | 0.415 | 0.276 | 0.127 | 0.625 | 0.2 |  |
| partials 5+ share (tristimulus 3) | 0.436 | 0.136 | 0.486 | 0.015 | 0.027 | 1.5 |  |
| loudness range, velocity 40 to 127 (dB) | 16.9 | 16.0 | 16.7 | 16.6 | 20.1 | 0.1 |  |
| pitch wobble / vibrato (p-p) (c) | 4.5 | 13.9 | 10.1 | 16.4 | 2.4 | 1.1 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0235 (nearest 0.0212) | fonts among themselves 0.0131 | | | | 1.8 | yes |

</details>

### 66 Alto Sax: alto-sax

Distance 2.98 (rank 51 of 129); the fonts disagree 1.1 JND. Keys 48,51,54,57,60,63,66,69,72,75,78,81,84, velocities 40/64/96/127.

- **Wrong pitch: sounds a major sixth low (alto-sax transpose -9)**: FAULT, the same `-play` transposition bug as p064 (fundamental share 0 vs 0.26-0.45).
- **Slow speech (onset 112 ms vs 15-24; attack 208 ms vs 19-50)**: FAULT, common to the reed player.
- **Attack 4 ERB brighter than the body; noisy (flatness -28 dB vs -34..-47)**: a breath or noise burst before the reed speaks. Probable FAULT.
- **Pitch -14 c (fonts -4..+2)**: re-measure after the fix. README Status says the alto plays 28/30 notes in tune.
- **Release 722 ms, staccato 400 ms**: the room (ARTIFACT).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (4.65 dB | 1.06 | 0.85 | 1.33 | 0.53), outside the fonts, 7.4 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (774 ms | 85.0 | 77.0 | 83.0 | 82.0), outside the fonts, 12.9 x.
3. **onset delay**: speaks later (110 ms | 23.0 | 24.0 | 15.0 | 22.0), outside the fonts, 8.8 x.
4. **sounding length of a 100 ms note [room]**: longer staccato (390 ms | 149 | 129 | 144 | 131), outside the fonts, 6.0 x.
5. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 20.6, nearest 20.4; the fonts among themselves 4.14), outside.
6. **stereo coherence [room]**: wider (0.141 | 0.876 | 1 | 1 | 1), outside the fonts, 11.5 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -33 | -10 | -5 | -11 | -0 | +22 |
| re TimGM6mb | -31 | -6 | -0 | -9 | -6 | +27 |
| re Jnsgm2 | -38 | -1 | +4 | -2 | -1 | +6 |
| re SGM | -38 | -4 | +3 | -2 | -5 | +10 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.1 ERB, velocity brightening +1.0 ERB, loudness range +4 dB, envelope 2.3 dB rms; TimGM6mb: centroid -0.0 ERB, attack -0.2 ERB, velocity brightening +0.5 ERB, loudness range +4 dB, envelope 1.7 dB rms; Jnsgm2: centroid -0.2 ERB, attack -0.3 ERB, velocity brightening +2.5 ERB, loudness range +5 dB, envelope 1.9 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 4.22 | -0.55 | -1.77 | 1.36 | 0.02 | 2.9 | yes |
| attack time (-20 to -1 dB) (ms) | 144 | 47.0 | 50.0 | 37.0 | 19.0 | 2.6 | yes |
| spectral shape of the attack, note for note | 16 (nearest 14) | fonts among themselves 11.4 | | | | 1.4 |  |
| brightness over the keyboard (ERB/oct) | -0.56 | 0.82 | 1.99 | 1.92 | 2.42 | 3.1 | yes |
| key-to-key brightness scatter (ERB) | 0.884 | 1.51 | 1.5 | 0.853 | 1.59 | 1.7 |  |
| brightening from velocity 40 to 127 (ERB) | 0.13 | 0.44 | -0 | 0 | 0.41 | 0.2 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 13.6 | 14.5 | 18.8 | 16.0 | 15.5 | 1.0 | yes |
| decay rate of a held note (dB/s) | 0.2 | 1.6 | 0.2 | 1 | 1.3 | 1.7 |  |
| onset delay (ms) | 110 | 23.0 | 24.0 | 15.0 | 22.0 | 8.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.85 | -3.19 | -3.12 | -2.34 | -3.34 | 1.4 | yes |
| envelope curve, note for note (dB rms) | 20.6 (nearest 20.4) | fonts among themselves 4.14 | | | | 5.0 | yes |
| pitch re 12-TET A440 (median) (c) | -0.4 | -3.9 | 1.5 | 1.9 | 0 | 0.3 |  |
| pitch scatter from key to key (c) | 0.8 | 2.8 | 1.8 | 2.4 | 0 | 0.4 |  |
| noisiness (spectral flatness) (dB) | -29.2 | -41.5 | -34.5 | -38.1 | -47.2 | 1.7 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -63.6 | -127 | -122 | -118 | -107 | 5.6 | yes |
| pitch at the start re settled (c) | -7 | -13.5 | 2.4 | -13.5 | -0.1 | 0.0 |  |
| stereo coherence | 0.141 | 0.876 | 1 | 1 | 1 | 11.5 | yes |
| partial-to-partial jaggedness (dB) | 3.6 | 3.05 | 2.6 | 2.4 | 2.8 | 0.9 | yes |
| dip between legato notes (dB) | 6.4 | 8.3 | 5.1 | 5.5 | 7.8 | 0.1 |  |
| level evenness of a legato scale (dB) | 1.4 | 1.2 | 1.1 | 2.2 | 0.6 | 0.2 |  |
| level over the keyboard (dB/oct) | 1.83 | -0.34 | 3.41 | 0.75 | 1.59 | 0.4 |  |
| level against the other instruments (velocity 96) (dB) | 0.4 | 5.4 | 2.4 | -1.6 | 1.4 | 0.4 |  |
| key-to-key level scatter (dB) | 4.65 | 1.06 | 0.85 | 1.33 | 0.53 | 7.4 | yes |
| odd / even partials (dB) | -4 | -1.1 | -0.4 | -2.3 | -1.6 | 1.3 | yes |
| onset jitter in a phrase (ms) | 51.0 | 29.4 | 21.7 | 44.8 | 14.7 | 1.7 | yes |
| onset in a phrase (re the note-on) (ms) | 23.0 | 13.0 | -0 | 5 | -0 | 2.0 | yes |
| onset sharpness (dB/ms) | 4.8 | 5.6 | 4.1 | 6.3 | 7.6 | 0.6 |  |
| overtone levels, note for note (dB rms) | 9.61 (nearest 9.06) | fonts among themselves 7.61 | | | | 1.3 |  |
| panning across the keys (dB/oct) | 0.31 | -0.45 | 0 | 0 | -0.05 | 0.3 |  |
| partial envelopes alike | -0.005 | 0.281 | 0.328 | 0.326 | 0.999 | 0.9 | yes |
| release time (30 dB after note-off) (ms) | 774 | 85.0 | 77.0 | 83.0 | 82.0 | 12.9 | yes |
| spectrogram (1 - correlation), note for note | 0.433 (nearest 0.417) | fonts among themselves 0.149 | | | | 2.9 | yes |
| spectral shape of the body, note for note | 12.2 (nearest 10.4) | fonts among themselves 9.93 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 390 | 149 | 129 | 144 | 131 | 6.0 | yes |
| partial stretch (inharmonicity) (c/partial) | 0 | -0.2 | -0.09 | -0.2 | 0 | 0.3 |  |
| level after 2 s held, re peak (dB) | -5.2 | -9.1 | -5.4 | -7 | -5.9 | 0.4 |  |
| high bands lag low bands by (ms) | -9.6 | 11.8 | 9.3 | 1 | 5.5 | 3.1 | yes |
| transient coherence: attack spread over bands (ms) | 19.8 | 5.8 | 5.6 | 5.2 | 2.2 | 2.9 | yes |
| fundamental share (tristimulus 1) | 0.293 | 0.445 | 0.258 | 0.415 | 0.33 | 0.9 |  |
| partials 2-4 share (tristimulus 2) | 0.498 | 0.323 | 0.498 | 0.338 | 0.265 | 1.4 |  |
| partials 5+ share (tristimulus 3) | 0.106 | 0.047 | 0.194 | 0.082 | 0.234 | 0.3 |  |
| loudness range, velocity 40 to 127 (dB) | 16.2 | 16.9 | 16.6 | 16.7 | 20.1 | 0.3 |  |
| pitch wobble / vibrato (p-p) (c) | 4.8 | 22.2 | 12.4 | 17.6 | 0.1 | 0.9 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0207 (nearest 0.0178) | fonts among themselves 0.0144 | | | | 1.4 |  |

</details>

### 67 Tenor Sax: tenor-sax

Distance 3.74 (rank 27 of 129); the fonts disagree 1.1 JND. Keys 48,51,54,57,60,63,66,69,72,75,78,81,84, velocities 40/64/96/127.

- **Wrong pitch: sounds a ninth low (transpose -14)**: FAULT, the `-play` transposition bug (fundamental share 0). The note an octave lower also makes it darker (11.3 vs 11.6-16 ERB).
- **Slow speech (onset 129 ms vs 17-39; attack 273 ms vs 17-38)**: FAULT.
- **Attack 14 ERB brighter than the body (fonts -0.4..+0.8)**: FAULT. A noise or key burst precedes the tone and is heard as a hiss.
- **Vibrato 47 c vs 1-7**: the jaw vibrato is DELIBERATE (README "Other woodwinds"), but too wide unless it is the tracker misreading the wrong pitch. Re-measure.
- **Release 750 ms**: the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (662 ms | 89.0 | 126 | 85.0 | 90.0), outside the fonts, 10.2 x.
2. **onset delay**: speaks later (134 ms | 25.0 | 24.0 | 17.0 | 40.0), outside the fonts, 9.5 x.
3. **attack brighter than the body by**: a brighter attack (8.54 ERB | 0.07 | -0.42 | 0.81 | 0.02), outside the fonts, 13.8 x.
4. **onset in a phrase (re the note-on)**: speaks later in a phrase (94.0 ms | 16.0 | 2 | -1 | 0), outside the fonts, 9.3 x.
5. **attack time (-20 to -1 dB)**: a slower attack (196 ms | 38.0 | 32.0 | 17.0 | 28.5), outside the fonts, 4.7 x.
6. **key-to-key level scatter**: less even from key to key (3.61 dB | 1.78 | 0.93 | 1.51 | 1.52), outside the fonts, 4.2 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -12 | +2 | +3 | -6 | -3 | +8 |
| re TimGM6mb | +3 | +3 | +4 | -1 | -5 | -1 |
| re Jnsgm2 | +14 | +7 | +7 | -0 | -4 | -7 |
| re SGM | -15 | -2 | +1 | -3 | -1 | +9 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -14 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -14 -5 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.2 ERB, attack -0.2 ERB, velocity brightening +1.4 ERB, loudness range +4 dB, envelope 2.1 dB rms; TimGM6mb: centroid -0.4 ERB, attack -1.1 ERB, velocity brightening +0.2 ERB, loudness range +4 dB, envelope 2.5 dB rms; Jnsgm2: centroid -0.2 ERB, attack -0.2 ERB, velocity brightening +4.0 ERB, loudness range +5 dB, envelope 1.8 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 8.54 | 0.07 | -0.42 | 0.81 | 0.02 | 13.8 | yes |
| attack time (-20 to -1 dB) (ms) | 196 | 38.0 | 32.0 | 17.0 | 28.5 | 4.7 | yes |
| spectral shape of the attack, note for note | 17.2 (nearest 14.9) | fonts among themselves 10.9 | | | | 1.6 | yes |
| brightness over the keyboard (ERB/oct) | -0.09 | 2.32 | 3.36 | 3.65 | 2.78 | 4.8 | yes |
| key-to-key brightness scatter (ERB) | 1.12 | 1.81 | 1.57 | 1.47 | 0.917 | 0.9 |  |
| brightening from velocity 40 to 127 (ERB) | -0.12 | 0.51 | -0 | 0 | 3.86 | 0.2 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 11.4 | 14.8 | 16.0 | 15.5 | 11.6 | 1.7 | yes |
| onset delay (ms) | 134 | 25.0 | 24.0 | 17.0 | 40.0 | 9.5 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.9 | -5.51 | -3.17 | -2.98 | -4.57 | 0.8 |  |
| envelope curve, note for note (dB rms) | 20.6 (nearest 19.5) | fonts among themselves 4.79 | | | | 4.3 | yes |
| pitch re 12-TET A440 (median) (c) | -0.1 | 2.7 | 2.6 | 0.9 | -3.7 | 0.5 |  |
| pitch scatter from key to key (c) | 0.7 | 1.4 | 0.7 | 1 | 2.7 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -31.2 | -41.3 | -61.5 | -46.7 | -34.5 | 0.9 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -64.0 | -125 | -120 | -119 | -105 | 5.5 | yes |
| pitch at the start re settled (c) | -7.2 | -2.4 | -1.3 | -1.2 | -10.1 | 1.1 |  |
| stereo coherence | 0.133 | 0.782 | 1 | 1 | 0.988 | 7.9 | yes |
| partial-to-partial jaggedness (dB) | 4 | 3.1 | 2.4 | 2.5 | 2.2 | 1.5 | yes |
| dip between legato notes (dB) | 8.7 | 7.6 | 8.1 | 6.8 | 15.4 | 0.2 |  |
| level evenness of a legato scale (dB) | 2.2 | 0.9 | 0.7 | 0.4 | 0.8 | 1.5 | yes |
| level over the keyboard (dB/oct) | 1.21 | 0.62 | 0.86 | 2.05 | 1.08 | 0.2 |  |
| level against the other instruments (velocity 96) (dB) | 1.2 | 3.6 | 0.6 | -1.1 | -2 | 0.5 |  |
| key-to-key level scatter (dB) | 3.61 | 1.78 | 0.93 | 1.51 | 1.52 | 4.2 | yes |
| odd / even partials (dB) | -2.7 | -1.4 | -1.6 | 0.9 | -3.8 | 0.5 |  |
| onset jitter in a phrase (ms) | 51.6 | 25.0 | 35.9 | 39.4 | 26.2 | 2.9 | yes |
| onset in a phrase (re the note-on) (ms) | 94.0 | 16.0 | 2 | -1 | 0 | 9.3 | yes |
| onset sharpness (dB/ms) | 5.5 | 5.8 | 6.1 | 5.8 | 7.3 | 0.2 |  |
| overtone levels, note for note (dB rms) | 10.3 (nearest 9.29) | fonts among themselves 8.37 | | | | 1.2 |  |
| panning across the keys (dB/oct) | 0.71 | -0.78 | 0.13 | 0 | -0.17 | 0.8 | yes |
| partial envelopes alike | 0.065 | 0.138 | 1 | 0.349 | 0.613 | 1.0 | yes |
| release time (30 dB after note-off) (ms) | 662 | 89.0 | 126 | 85.0 | 90.0 | 10.2 | yes |
| spectrogram (1 - correlation), note for note | 0.416 (nearest 0.364) | fonts among themselves 0.12 | | | | 3.5 | yes |
| spectral shape of the body, note for note | 13 (nearest 11.9) | fonts among themselves 10.5 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 270 | 136 | 138 | 143 | 139 | 3.9 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.02 | -0.01 | 0 | 0 | 0.01 | 0.0 |  |
| level after 2 s held, re peak (dB) | -4 | -7.8 | -4.9 | -6.6 | -7 | 0.9 |  |
| high bands lag low bands by (ms) | -4.4 | 10.3 | 6.2 | 3.7 | 4.7 | 2.0 | yes |
| transient coherence: attack spread over bands (ms) | 15.4 | 4.2 | 3.2 | 2 | 3.5 | 2.4 | yes |
| fundamental share (tristimulus 1) | 0.366 | 0.254 | 0.354 | 0.139 | 0.518 | 0.3 |  |
| partials 2-4 share (tristimulus 2) | 0.403 | 0.594 | 0.28 | 0.475 | 0.371 | 0.1 |  |
| partials 5+ share (tristimulus 3) | 0.073 | 0.073 | 0.268 | 0.384 | 0.042 | 0.6 |  |
| loudness range, velocity 40 to 127 (dB) | 17.9 | 16.8 | 16.6 | 16.7 | 18.7 | 0.8 |  |
| pitch wobble / vibrato (p-p) (c) | 5.5 | 7.1 | 7.3 | 0.9 | 6.1 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0184 (nearest 0.0174) | fonts among themselves 0.0104 | | | | 1.8 | yes |

</details>

### 68 Baritone Sax: baritone-sax

Distance 4.09 (rank 18 of 129); the fonts disagree 1.3 JND. Keys 48,51,54,57,60,63,66,69,72,75,78,81,84, velocities 40/64/96/127.

- **Wrong pitch: sounds an octave and a sixth low (transpose -21)**: FAULT, the `-play` transposition bug (fundamental share 0, partials 2-4 share 0.8). It explains much of "darker" (9.2 vs 12.4-18.1 ERB) and the spectrogram distance (0.81 vs 0.08 among the fonts).
- **Very slow speech (onset 182 ms vs 13-28; 94 ms in a phrase vs 0-6)**: FAULT.
- **Attack 10 ERB brighter than the body**: onset noise burst (FAULT, as on the tenor).
- **Vibrato 72 c vs 5-23; pitch scatter 10 c**: re-measure after the fix.
- **Release 612 ms**: the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (614 ms | 78.0 | 80.0 | 89.0 | 96.0), outside the fonts, 11.5 x.
2. **onset delay**: speaks later (161 ms | 14.0 | 16.0 | 13.0 | 29.0), outside the fonts, 14.6 x.
3. **attack brighter than the body by**: a brighter attack (10.3 ERB | -0.36 | 0.22 | 0.66 | -0.05), outside the fonts, 20.0 x.
4. **onset in a phrase (re the note-on)**: speaks later in a phrase (88.0 ms | 6 | -0 | 5 | -0), outside the fonts, 8.6 x.
5. **key-to-key level scatter**: less even from key to key (3.21 dB | 1.04 | 0.69 | 0.97 | 1.22), outside the fonts, 4.4 x.
6. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.537, nearest 0.494; the fonts among themselves 0.0802), outside.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -0 | +16 | +14 | +3 | -3 | -18 |
| re TimGM6mb | +4 | +11 | +12 | +0 | -7 | -12 |
| re Jnsgm2 | +3 | +14 | +11 | +1 | -3 | -14 |
| re SGM | +7 | +8 | +7 | +0 | -3 | -10 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -14 -7 +0 +5; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.8 ERB, attack -0.0 ERB, velocity brightening +1.3 ERB, loudness range +4 dB, envelope 2.1 dB rms; TimGM6mb: centroid -4.6 ERB, attack +0.1 ERB, velocity brightening +0.1 ERB, loudness range +3 dB, envelope 2.0 dB rms; Jnsgm2: centroid -0.1 ERB, attack +0.0 ERB, velocity brightening +2.8 ERB, loudness range +5 dB, envelope 2.0 dB rms
- silent probe notes: FluidR3/fs 16, TimGM6mb/fs 8

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 10.3 | -0.36 | 0.22 | 0.66 | -0.05 | 20.0 | yes |
| attack time (-20 to -1 dB) (ms) | 258 | 28.5 | 24.0 | 70.0 | 33.0 | 4.0 | yes |
| spectral shape of the attack, note for note | 16.5 (nearest 14.5) | fonts among themselves 8.99 | | | | 1.8 | yes |
| brightness over the keyboard (ERB/oct) | -0.12 | 3.64 | 4.08 | 2.65 | 1.65 | 2.7 | yes |
| key-to-key brightness scatter (ERB) | 1.37 | 2.39 | 1.63 | 1.76 | 0.921 | 0.4 |  |
| brightening from velocity 40 to 127 (ERB) | 0.47 | 0.2 | 0 | 0 | 0.35 | 1.2 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 9.45 | 16.3 | 18.1 | 16.0 | 12.4 | 2.3 | yes |
| onset delay (ms) | 161 | 14.0 | 16.0 | 13.0 | 29.0 | 14.6 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.56 | -4.14 | -3.84 | -2.71 | -5.97 | 0.3 |  |
| envelope curve, note for note (dB rms) | 20.4 (nearest 19.8) | fonts among themselves 3.13 | | | | 6.5 | yes |
| pitch re 12-TET A440 (median) (c) | 0.6 | -0.9 | 1.7 | -1.5 | -0.4 | 0.3 |  |
| pitch scatter from key to key (c) | 3.7 | 1.2 | 1.5 | 8.7 | 4.7 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -32.4 | -38.3 | -43.0 | -35.6 | -43.5 | 2.1 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -65.2 | -125 | -117 | -119 | -104 | 5.1 | yes |
| pitch at the start re settled (c) | -2.2 | -17.2 | -12.5 | -5.7 | -3.2 | 1.0 |  |
| stereo coherence | 0.426 | 0.779 | 1 | 1 | 0.861 | 4.6 | yes |
| partial-to-partial jaggedness (dB) | 3.75 | 3.4 | 5.6 | 3.5 | 4.9 | 0.4 |  |
| dip between legato notes (dB) | 10.1 | 7.4 | 6.6 | 6.8 | 10.2 | 1.5 |  |
| level evenness of a legato scale (dB) | 2.6 | 0.2 | 0.6 | 0.6 | 1.1 | 2.0 | yes |
| level over the keyboard (dB/oct) | -0.11 | 0.15 | -0.56 | 2 | 1.18 | 0.6 |  |
| level against the other instruments (velocity 96) (dB) | 0.5 | 1.9 | -2.8 | 0.1 | -1.6 | 0.5 |  |
| key-to-key level scatter (dB) | 3.21 | 1.04 | 0.69 | 0.97 | 1.22 | 4.4 | yes |
| odd / even partials (dB) | -4.8 | -1.8 | 3.5 | -1.1 | -5 | 0.8 |  |
| onset jitter in a phrase (ms) | 39.8 | 25.4 | 16.6 | 21.5 | 15.9 | 4.1 | yes |
| onset in a phrase (re the note-on) (ms) | 88.0 | 6 | -0 | 5 | -0 | 8.6 | yes |
| onset sharpness (dB/ms) | 7.1 | 5.9 | 6.8 | 9.1 | 3.4 | 0.3 |  |
| overtone levels, note for note (dB rms) | 11.5 (nearest 10.7) | fonts among themselves 8.25 | | | | 1.4 |  |
| panning across the keys (dB/oct) | 0.26 | -0.02 | 0 | 0 | 1.04 | 0.3 |  |
| partial envelopes alike | 0.162 | 0.266 | 0.717 | 0.284 | 0.081 | 0.4 |  |
| release time (30 dB after note-off) (ms) | 614 | 78.0 | 80.0 | 89.0 | 96.0 | 11.5 | yes |
| spectrogram (1 - correlation), note for note | 0.537 (nearest 0.494) | fonts among themselves 0.0802 | | | | 6.7 | yes |
| spectral shape of the body, note for note | 15.4 (nearest 15) | fonts among themselves 8.3 | | | | 1.9 | yes |
| sounding length of a 100 ms note (ms) | 172 | 142 | 135 | 147 | 155 | 1.0 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.04 | -0.01 | 0 | 0 | -0.04 | 0.1 |  |
| level after 2 s held, re peak (dB) | -4.2 | -8.3 | -5.2 | -5.7 | -5.3 | 0.4 |  |
| high bands lag low bands by (ms) | -7.5 | 4.8 | 0.8 | 1.3 | 4.2 | 2.0 | yes |
| transient coherence: attack spread over bands (ms) | 15.6 | 4.8 | 2.2 | 3.4 | 3.5 | 2.4 | yes |
| fundamental share (tristimulus 1) | 0.704 | 0.398 | 0.191 | 0.546 | 0.632 | 1.1 | yes |
| partials 2-4 share (tristimulus 2) | 0.262 | 0.335 | 0.45 | 0.289 | 0.221 | 0.4 |  |
| partials 5+ share (tristimulus 3) | 0.022 | 0.171 | 0.279 | 0.144 | 0.082 | 1.4 | yes |
| loudness range, velocity 40 to 127 (dB) | 19.5 | 16.7 | 16.7 | 16.6 | 20.1 | 1.6 |  |
| pitch wobble / vibrato (p-p) (c) | 9 | 8.4 | 5.1 | 22.5 | 6.6 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0244 (nearest 0.0234) | fonts among themselves 0.0113 | | | | 2.2 | yes |

</details>

### 69 Oboe: oboe

Distance 4.57 (rank 8 of 129); the fonts disagree 0.9 JND. Keys 48,51,54,57,60,63,66,69,72,75,78,81,84, velocities 40/64/96/127.

- **Much brighter (22.1 ERB, about 2.2 kHz, vs 15.6-16.8, about 1 kHz; partials 5+ share 0.44 vs 0.02-0.11; +28..+35 dB above 6 kHz)**: probable FAULT. All four fonts agree closely, and flatness of -27 vs -45..-51 dB points to reed noise or buzz above 6 kHz.
- **Attack 8.5 ERB brighter than the body**: an onset noise burst (FAULT, shared by the reeds).
- **Brightness over the keyboard -0.8 vs +3.6 ERB/oct**: MEASUREMENT ARTIFACT. Probe keys 48-57 lie below Bb3 and fold up an octave.
- **Onset in a phrase 91 ms vs -2..12**: slow slurs, FAULT. The single-note onset (114 ms) is inside the fonts (16-128).
- **Release 746 ms**: the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (21.5 ERB | 16.8 | 15.6 | 15.6 | 15.8), outside the fonts, 9.6 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (755 ms | 112 | 152 | 130 | 89.0), outside the fonts, 6.9 x.
3. **attack brighter than the body by**: a brighter attack (8.54 ERB | -1.14 | -0.8 | -0.81 | -0.31), outside the fonts, 22.5 x.
4. **onset in a phrase (re the note-on)**: speaks later in a phrase (88.0 ms | 12.0 | -1 | -1 | -2), outside the fonts, 8.9 x.
5. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.435, nearest 0.421; the fonts among themselves 0.0555), outside.
6. **brightness over the keyboard**: brightens slower up the keyboard (-0.78 ERB/oct | 3.58 | 3.59 | 3.59 | 3.19), outside the fonts, 14.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -58 | -32 | -13 | -7 | +15 | +27 |
| re TimGM6mb | -61 | -36 | -15 | -8 | +11 | +35 |
| re Jnsgm2 | -60 | -35 | -14 | -7 | +12 | +32 |
| re SGM | -54 | -31 | -12 | -3 | +9 | +27 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -15 -7 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.4 ERB, attack -0.1 ERB, velocity brightening +0.2 ERB, loudness range +3 dB, envelope 5.5 dB rms; TimGM6mb: centroid -0.0 ERB, attack -0.0 ERB, velocity brightening +0.0 ERB, loudness range +3 dB, envelope 1.9 dB rms; Jnsgm2: centroid -0.1 ERB, attack +0.0 ERB, velocity brightening +0.1 ERB, loudness range +3 dB, envelope 2.9 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 8.54 | -1.14 | -0.8 | -0.81 | -0.31 | 22.5 | yes |
| attack time (-20 to -1 dB) (ms) | 207 | 499 | 670 | 665 | 18.0 | 0.6 |  |
| spectral shape of the attack, note for note | 20.3 (nearest 19.9) | fonts among themselves 6.43 | | | | 3.2 | yes |
| brightness over the keyboard (ERB/oct) | -0.78 | 3.58 | 3.59 | 3.59 | 3.19 | 14.6 | yes |
| key-to-key brightness scatter (ERB) | 1.35 | 0.883 | 0.705 | 0.707 | 0.763 | 3.1 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.15 | -0 | 0 | 0 | 0.22 | 0.5 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 21.5 | 16.8 | 15.6 | 15.6 | 15.8 | 9.6 | yes |
| onset delay (ms) | 104 | 128 | 42.0 | 38.0 | 15.0 | 1.1 |  |
| Plomp-Levelt dissonance of a note (log2) | -2.46 | -5.51 | -6.27 | -6.38 | -4.21 | 3.2 | yes |
| envelope curve, note for note (dB rms) | 18.6 (nearest 17.8) | fonts among themselves 4.27 | | | | 4.4 | yes |
| pitch re 12-TET A440 (median) (c) | 0 | -0.6 | -3.8 | -4.1 | 0.1 | 0.5 |  |
| pitch scatter from key to key (c) | 0.4 | 1.8 | 1.8 | 1.9 | 0.6 | 0.5 |  |
| noisiness (spectral flatness) (dB) | -27.6 | -48.1 | -51.4 | -50.8 | -45.1 | 6.9 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -66.2 | -123 | -123 | -123 | -108 | 7.5 | yes |
| pitch at the start re settled (c) | -0.3 | 9.7 | 1 | 1 | 7.4 | 0.9 |  |
| stereo coherence | 0.308 | 0.144 | 1 | 1 | 1 | 1.6 |  |
| partial-to-partial jaggedness (dB) | 4.7 | 3.7 | 2.8 | 2.8 | 4.6 | 1.5 |  |
| dip between legato notes (dB) | 4.4 | 7.6 | 7.4 | 8.1 | 8.3 | 1.7 | yes |
| level evenness of a legato scale (dB) | 4 | 0.7 | 2.1 | 2.1 | 1.4 | 2.2 | yes |
| level over the keyboard (dB/oct) | 2.33 | 0.97 | 1.98 | 1.97 | 0.22 | 0.9 |  |
| level against the other instruments (velocity 96) (dB) | 1.9 | 0.6 | 0.6 | 1.1 | 3 | 0.9 |  |
| key-to-key level scatter (dB) | 2.2 | 1.24 | 0.95 | 0.95 | 0.59 | 2.5 | yes |
| odd / even partials (dB) | -7.9 | -1.3 | -5.5 | -5.5 | -1 | 2.0 | yes |
| onset jitter in a phrase (ms) | 38.2 | 17.5 | 18.8 | 21.1 | 28.6 | 3.3 | yes |
| onset in a phrase (re the note-on) (ms) | 88.0 | 12.0 | -1 | -1 | -2 | 8.9 | yes |
| onset sharpness (dB/ms) | 4.5 | 4.8 | 5.4 | 5.4 | 7.2 | 0.5 |  |
| overtone levels, note for note (dB rms) | 19.9 (nearest 17) | fonts among themselves 7.17 | | | | 2.8 | yes |
| panning across the keys (dB/oct) | 0.17 | 0.48 | 0 | 0 | -0.05 | 0.2 |  |
| partial envelopes alike | 0.009 | 0.243 | 0.736 | 0.718 | 0.929 | 2.1 | yes |
| release time (30 dB after note-off) (ms) | 755 | 112 | 152 | 130 | 89.0 | 6.9 | yes |
| spectrogram (1 - correlation), note for note | 0.435 (nearest 0.421) | fonts among themselves 0.0555 | | | | 7.8 | yes |
| spectral shape of the body, note for note | 15.3 (nearest 14.8) | fonts among themselves 7.39 | | | | 2.1 | yes |
| sounding length of a 100 ms note (ms) | 400 | 172 | 158 | 154 | 146 | 5.4 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.01 | 0.02 | -0.01 | -0.01 | 0 | 0.0 |  |
| level after 2 s held, re peak (dB) | -4.2 | -4.4 | -5 | -5.6 | -5 | 0.3 |  |
| high bands lag low bands by (ms) | -3 | 34.7 | 4.3 | 6 | 4 | 0.5 | yes |
| transient coherence: attack spread over bands (ms) | 15.2 | 10.4 | 2.9 | 2.9 | 1.6 | 2.5 | yes |
| fundamental share (tristimulus 1) | 0.026 | 0.086 | 0.122 | 0.122 | 0.097 | 1.2 | yes |
| partials 2-4 share (tristimulus 2) | 0.461 | 0.704 | 0.778 | 0.779 | 0.547 | 2.4 | yes |
| partials 5+ share (tristimulus 3) | 0.416 | 0.018 | 0.029 | 0.029 | 0.111 | 5.5 | yes |
| loudness range, velocity 40 to 127 (dB) | 18.7 | 16.7 | 16.7 | 16.7 | 20.1 | 1.2 |  |
| pitch wobble / vibrato (p-p) (c) | 3.9 | 12.7 | 6.4 | 6.3 | 1.9 | 0.5 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0235 (nearest 0.0228) | fonts among themselves 0.00893 | | | | 2.6 | yes |

</details>

### 70 English Horn: english-horn

Distance 3.84 (rank 25 of 129); the fonts disagree 1.4 JND. Keys 48,51,54,57,60,63,66,69,72,75,78,81,84, velocities 40/64/96/127.

- **Wrong pitch: sounds a fifth low (english-horn transpose -7)**: FAULT, the `-play` transposition bug (fundamental share 0).
- **Much brighter (20.4 ERB vs 13.3-14.4; partials 5+ share 0.43 vs ≤0.04; flatness -27 vs -44..-69 dB)**: probable FAULT, the same double-reed brightness and noise as p068.
- **Onset in a phrase 60 ms vs -2..8**: slow slurs.
- **Release 753 ms, staccato 398 ms**: the room (ARTIFACT).
- Not in README's woodwind table: undocumented.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (20.0 ERB | 14.4 | 14.1 | 13.3 | 13.5), outside the fonts, 11.6 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (753 ms | 90.0 | 95.0 | 114 | 102), outside the fonts, 11.7 x.
3. **brightening from velocity 40 to 127**: brightens more with velocity (1.33 ERB | 0 | -0 | -0 | 0.3), outside the fonts, 4.4 x.
4. **onset in a phrase (re the note-on)**: speaks later in a phrase (66.0 ms | 8 | 0 | -0 | -1), outside the fonts, 6.6 x.
5. **sounding length of a 100 ms note [room]**: longer staccato (400 ms | 140 | 131 | 131 | 112), outside the fonts, 6.4 x.
6. **partials 5+ share (tristimulus 3)**: stronger high partials (0.416 | 0.004 | 0.022 | 0.037 | 0.008), outside the fonts, 5.7 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -52 | -20 | -13 | -7 | +9 | +26 |
| re TimGM6mb | -46 | -16 | -13 | -4 | +8 | +20 |
| re Jnsgm2 | -52 | -19 | -12 | +0 | +11 | +15 |
| re SGM | -46 | -16 | -14 | -0 | +12 | +14 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -14 -8 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack +0.1 ERB, velocity brightening +0.4 ERB, loudness range +4 dB, envelope 4.9 dB rms; TimGM6mb: centroid -0.0 ERB, attack -0.0 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 1.8 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 1.7 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -2.06 | -1.99 | -0.17 | -0.03 | 0.01 | 2.0 |  |
| attack time (-20 to -1 dB) (ms) | 184 | 976 | 34.0 | 246 | 26.0 | 0.4 |  |
| spectral shape of the attack, note for note | 14 (nearest 13.8) | fonts among themselves 7.61 | | | | 1.8 | yes |
| brightness over the keyboard (ERB/oct) | -0.3 | 1.27 | 0.67 | 0.19 | 1.45 | 2.0 | yes |
| key-to-key brightness scatter (ERB) | 1.48 | 1.1 | 0.999 | 1.14 | 1.17 | 1.8 | yes |
| brightening from velocity 40 to 127 (ERB) | 1.33 | 0 | -0 | -0 | 0.3 | 4.4 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 20.0 | 14.4 | 14.1 | 13.3 | 13.5 | 11.6 | yes |
| onset delay (ms) | 77.0 | 414 | 22.0 | 39.0 | 18.0 | 0.2 |  |
| Plomp-Levelt dissonance of a note (log2) | -2.11 | -4.64 | -4.54 | -6.13 | -4.84 | 3.3 | yes |
| envelope curve, note for note (dB rms) | 19.6 (nearest 19) | fonts among themselves 3.53 | | | | 5.5 | yes |
| pitch re 12-TET A440 (median) (c) | -0.1 | -6.2 | 2.1 | 3.6 | 4.6 | 0.5 |  |
| pitch scatter from key to key (c) | 0.6 | 8.2 | 1 | 0.7 | 3 | 0.4 |  |
| noisiness (spectral flatness) (dB) | -27.5 | -45.0 | -68.9 | -44.3 | -49.3 | 1.6 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -65.8 | -123 | -121 | -122 | -113 | 9.3 | yes |
| pitch at the start re settled (c) | -0.5 | 15.6 | 1.8 | -0.4 | -8.5 | 0.1 |  |
| stereo coherence | 0.2 | 0.368 | 1 | 1 | 1 | 2.5 | yes |
| partial-to-partial jaggedness (dB) | 3.2 | 2.6 | 4.6 | 4.7 | 4 | 1.0 |  |
| dip between legato notes (dB) | 6 | 10.4 | 9.8 | 8.8 | 9.5 | 1.8 | yes |
| level evenness of a legato scale (dB) | 1.7 | 1.5 | 1.5 | 1.5 | 2.5 | 0.2 |  |
| level over the keyboard (dB/oct) | -0.12 | -2 | 1.24 | -1.81 | 1.88 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | 0.1 | -2.8 | 2.4 | 2.2 | 6 | 0.5 |  |
| key-to-key level scatter (dB) | 2.46 | 1.97 | 0.62 | 1.17 | 2.01 | 1.3 | yes |
| odd / even partials (dB) | -1.9 | -4.1 | 4.4 | -0.3 | -2.2 | 0.2 |  |
| onset jitter in a phrase (ms) | 38.0 | 35.1 | 27.4 | 55.4 | 27.7 | 0.5 |  |
| onset in a phrase (re the note-on) (ms) | 66.0 | 8 | 0 | -0 | -1 | 6.6 | yes |
| onset sharpness (dB/ms) | 5.7 | 5.5 | 3.7 | 3.3 | 10.0 | 0.3 |  |
| overtone levels, note for note (dB rms) | 15 (nearest 14.6) | fonts among themselves 8.43 | | | | 1.8 | yes |
| panning across the keys (dB/oct) | 0.56 | -1.11 | -0.12 | 0 | -0.05 | 0.6 | yes |
| partial envelopes alike | 0.008 | 0.68 | 1 | 0.313 | 0.377 | 1.5 | yes |
| release time (30 dB after note-off) (ms) | 753 | 90.0 | 95.0 | 114 | 102 | 11.7 | yes |
| spectrogram (1 - correlation), note for note | 0.42 (nearest 0.402) | fonts among themselves 0.0853 | | | | 4.9 | yes |
| spectral shape of the body, note for note | 12.6 (nearest 11.8) | fonts among themselves 7.65 | | | | 1.6 | yes |
| sounding length of a 100 ms note (ms) | 400 | 140 | 131 | 131 | 112 | 6.4 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.01 | -0.02 | 0 | 0.01 | -0.02 | 0.0 |  |
| level after 2 s held, re peak (dB) | -4.2 | -4.3 | -3.4 | -3.2 | -3.8 | 0.2 |  |
| high bands lag low bands by (ms) | 0 | 8.3 | 4 | 5 | 4.8 | 1.0 | yes |
| transient coherence: attack spread over bands (ms) | 20.7 | 7.6 | 2.3 | 4.3 | 1.7 | 3.5 | yes |
| fundamental share (tristimulus 1) | 0.07 | 0.337 | 0.244 | 0.455 | 0.177 | 1.6 | yes |
| partials 2-4 share (tristimulus 2) | 0.332 | 0.6 | 0.522 | 0.433 | 0.695 | 1.7 | yes |
| partials 5+ share (tristimulus 3) | 0.416 | 0.004 | 0.022 | 0.037 | 0.008 | 5.7 | yes |
| loudness range, velocity 40 to 127 (dB) | 18.5 | 16.6 | 16.6 | 16.6 | 20.1 | 1.1 |  |
| pitch wobble / vibrato (p-p) (c) | 4.2 | 20.1 | 0 | 5.4 | 6.1 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0203 (nearest 0.0188) | fonts among themselves 0.011 | | | | 1.8 | yes |

</details>

### 71 Bassoon: bassoon

Distance 3.34 (rank 39 of 129); the fonts disagree 1.2 JND. Keys 48,51,54,57,60,63,66,69,72,75,78,81,84, velocities 40/64/96/127.

- **Brighter (17.6 ERB vs 12.2-13.2; flatness -24 dB vs -47..-82)**: probable FAULT, reed noise. It holds even though 7 of 13 probe keys (66-84) lie above the bassoon's Eb4 and fold down an octave, which should make it darker.
- **Brightness and spectrogram against the keyboard**: largely a MEASUREMENT ARTIFACT of that folding. The fonts stretch bassoon samples up to C6.
- **Onset in a phrase 53 ms vs 2-5; staccato 318 ms**: slurs slow-ish. The staccato is the room.
- **Vibrato 24 c vs 0-14**: DELIBERATE breath vibrato (README "Other woodwinds", the reed player).
- **Release 654 ms**: the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (17.3 ERB | 12.2 | 13.0 | 13.2 | 12.7), outside the fonts, 9.1 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (648 ms | 120 | 66.0 | 90.0 | 64.0), outside the fonts, 6.8 x.
3. **onset in a phrase (re the note-on)**: speaks later in a phrase (81.0 ms | 2 | 2 | 2 | 5), outside the fonts, 7.9 x.
4. **sounding length of a 100 ms note [room]**: longer staccato (309 ms | 141 | 137 | 155 | 122), outside the fonts, 4.6 x.
5. **attack brighter than the body by**: a duller attack (-2.38 ERB | -0.68 | -0.08 | -0.14 | -0.97), outside the fonts, 4.4 x.
6. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.439, nearest 0.379; the fonts among themselves 0.0897), outside.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +3 | -3 | -1 | +9 | +13 | -16 |
| re TimGM6mb | -4 | -9 | -3 | +2 | +4 | +4 |
| re Jnsgm2 | -2 | -6 | -1 | +3 | +5 | -4 |
| re SGM | -1 | -9 | -8 | +1 | +9 | +7 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack +0.1 ERB, velocity brightening +0.1 ERB, loudness range +3 dB, envelope 5.5 dB rms; TimGM6mb: centroid -0.0 ERB, attack +0.1 ERB, velocity brightening +0.0 ERB, loudness range +3 dB, envelope 2.0 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.1 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 2.2 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -2.38 | -0.68 | -0.08 | -0.14 | -0.97 | 4.4 | yes |
| attack time (-20 to -1 dB) (ms) | 196 | 610 | 17.0 | 17.0 | 78.0 | 0.9 |  |
| spectral shape of the attack, note for note | 12.4 (nearest 11.8) | fonts among themselves 8.15 | | | | 1.5 | yes |
| brightness over the keyboard (ERB/oct) | 0.65 | 1.37 | 3 | 3.12 | 1.46 | 1.8 | yes |
| key-to-key brightness scatter (ERB) | 0.671 | 0.925 | 0.405 | 0.381 | 0.729 | 0.4 |  |
| brightening from velocity 40 to 127 (ERB) | -0.22 | 0 | -0 | -0 | 0.27 | 0.7 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 17.3 | 12.2 | 13.0 | 13.2 | 12.7 | 9.1 | yes |
| onset delay (ms) | 51.0 | 28.0 | 9 | 9 | 32.0 | 2.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | -1.81 | -6.32 | -4.57 | -3.82 | -5.72 | 2.7 | yes |
| envelope curve, note for note (dB rms) | 19.1 (nearest 17.7) | fonts among themselves 5.07 | | | | 3.8 | yes |
| pitch re 12-TET A440 (median) (c) | -0.7 | -1.8 | 2.7 | 2.7 | 0 | 0.5 |  |
| pitch scatter from key to key (c) | 1.3 | 2.1 | 0.6 | 0.6 | 0 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -25.0 | -46.8 | -70.2 | -81.9 | -59.0 | 2.3 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -67.8 | -123 | -122 | -120 | -109 | 7.4 | yes |
| pitch at the start re settled (c) | -13.6 | 5 | 0.2 | 0.3 | 2.5 | 3.0 | yes |
| stereo coherence | 0.43 | 0.401 | 1 | 1 | 1 | 1.9 |  |
| partial-to-partial jaggedness (dB) | 3.85 | 3.9 | 4 | 4.1 | 3.3 | 0.1 |  |
| dip between legato notes (dB) | 7.1 | 11.1 | 9.2 | 7.7 | 12.5 | 1.3 |  |
| level evenness of a legato scale (dB) | 2.3 | 1.6 | 1.2 | 1.3 | 0.6 | 1.0 | yes |
| level over the keyboard (dB/oct) | -0 | -0.93 | -0.41 | -0.71 | 0.73 | 0.6 |  |
| level against the other instruments (velocity 96) (dB) | 1.9 | -1.1 | 0.1 | -0.9 | 3.4 | 1.0 |  |
| key-to-key level scatter (dB) | 2.13 | 2.19 | 0.99 | 1.36 | 0.43 | 1.1 |  |
| odd / even partials (dB) | -3.1 | -16.4 | -6.4 | -6.1 | -7.9 | 0.8 | yes |
| onset jitter in a phrase (ms) | 42.0 | 36.3 | 10.8 | 6.8 | 14.9 | 2.0 | yes |
| onset in a phrase (re the note-on) (ms) | 81.0 | 2 | 2 | 2 | 5 | 7.9 | yes |
| onset sharpness (dB/ms) | 11.2 | 4.9 | 6.6 | 8.5 | 8.4 | 1.8 | yes |
| overtone levels, note for note (dB rms) | 13.3 (nearest 12.3) | fonts among themselves 7.88 | | | | 1.7 | yes |
| panning across the keys (dB/oct) | -0.19 | 0.85 | 0 | 0 | -0.05 | 0.2 |  |
| partial envelopes alike | 0.018 | 0.167 | 0.999 | 0.337 | 0.953 | 1.5 | yes |
| release time (30 dB after note-off) (ms) | 648 | 120 | 66.0 | 90.0 | 64.0 | 6.8 | yes |
| spectrogram (1 - correlation), note for note | 0.439 (nearest 0.379) | fonts among themselves 0.0897 | | | | 4.9 | yes |
| spectral shape of the body, note for note | 14.4 (nearest 14.1) | fonts among themselves 7.96 | | | | 1.8 | yes |
| sounding length of a 100 ms note (ms) | 309 | 141 | 137 | 155 | 122 | 4.6 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.03 | 0.17 | 0 | 0 | 0 | 0.1 |  |
| level after 2 s held, re peak (dB) | -5.8 | -6 | -7.4 | -6.1 | -5.2 | 0.1 |  |
| high bands lag low bands by (ms) | 19.1 | 16.7 | 7 | 6.3 | 9.7 | 2.1 |  |
| transient coherence: attack spread over bands (ms) | 14.7 | 4.5 | 2.6 | 2.4 | 4.8 | 2.2 | yes |
| fundamental share (tristimulus 1) | 0.041 | 0.583 | 0.342 | 0.31 | 0.165 | 1.4 | yes |
| partials 2-4 share (tristimulus 2) | 0.542 | 0.417 | 0.657 | 0.689 | 0.636 | 0.8 |  |
| partials 5+ share (tristimulus 3) | 0.166 | 0.003 | 0.001 | 0.001 | 0.003 | 2.3 | yes |
| loudness range, velocity 40 to 127 (dB) | 15.2 | 16.7 | 16.7 | 16.6 | 20.1 | 0.9 | yes |
| pitch wobble / vibrato (p-p) (c) | 8.8 | 13.8 | 0 | 0 | 1 | 1.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0245 (nearest 0.0225) | fonts among themselves 0.0109 | | | | 2.2 | yes |

</details>

### 72 Clarinet: clarinet

Distance 4.38 (rank 11 of 129); the fonts disagree 1.1 JND. Keys 48,51,54,57,60,63,66,69,72,75,78,81,84, velocities 40/64/96/127.

- **Wrong pitch: sounds a tone low (clarinet transpose -2)**: FAULT, the `-play` transposition bug (fundamental share 0, high partials 0.999). The pitch +29.6 c, scatter 37.7 c and wobble 24 c are likely the tracker misreading it. Re-measure.
- **Brightens strongly with velocity (4.9 ERB vs ~0; FluidR3 under FluidSynth +0.6)**: DELIBERATE physics (the beating reed). The fonts' 0 is TiMidity's convention.
- **Slow speech (onset 102 ms vs 19-46; 86 ms in a phrase)**: FAULT.
- **Attack 15.6 ERB brighter than the body; noisy (-30 vs -56..-67 dB)**: onset noise burst, FAULT.
- **Key level scatter 3.5 dB vs 0.6-0.8; loudness range 13 dB vs 16-20**: uneven and a little narrow.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (4.83 dB | 0.82 | 0.66 | 0.63 | 0.72), outside the fonts, 8.3 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (4.58 ERB | -0.22 | -0 | 0 | -0.11), outside the fonts, 15.4 x.
3. **onset delay**: speaks later (131 ms | 33.0 | 19.0 | 46.0 | 22.0), outside the fonts, 7.7 x.
4. **key-to-key brightness scatter**: less even in tone from key to key (3.11 ERB | 1.09 | 1.41 | 0.578 | 0.62), outside the fonts, 5.4 x.
5. **loudness range, velocity 40 to 127**: a narrower dynamic range (11.9 dB | 16.4 | 16.7 | 16.6 | 20.1), outside the fonts, 2.6 x.
6. **sounding length of a 100 ms note [room]**: longer staccato (358 ms | 104 | 152 | 139 | 143), outside the fonts, 4.9 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -41 | -10 | -7 | -17 | +0 | +30 |
| re TimGM6mb | -43 | -11 | -8 | -14 | +0 | +28 |
| re Jnsgm2 | -41 | -9 | -4 | -10 | +2 | +19 |
| re SGM | -43 | -12 | -9 | -8 | +2 | +23 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -10 -5 +0 +2; FluidR3 -12 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +1.0 ERB, attack -0.1 ERB, velocity brightening +0.8 ERB, loudness range +4 dB, envelope 3.3 dB rms; TimGM6mb: centroid -0.2 ERB, attack +0.0 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 1.6 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.1 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 2.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 13.7 | -6.13 | 0.15 | -2.51 | -0.91 | 4.9 | yes |
| attack time (-20 to -1 dB) (ms) | 176 | 55.0 | 20.0 | 38.0 | 43.0 | 2.9 | yes |
| spectral shape of the attack, note for note | 21.1 (nearest 18.5) | fonts among themselves 9.85 | | | | 2.1 | yes |
| brightness over the keyboard (ERB/oct) | -1.21 | 2.78 | 4.32 | 5 | 2.49 | 3.8 | yes |
| key-to-key brightness scatter (ERB) | 3.11 | 1.09 | 1.41 | 0.578 | 0.62 | 5.4 | yes |
| brightening from velocity 40 to 127 (ERB) | 4.58 | -0.22 | -0 | 0 | -0.11 | 15.4 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 15.3 | 17.0 | 12.8 | 12.1 | 11.2 | 1.0 |  |
| decay rate of a held note (dB/s) | 0.2 | 1.2 | 0.8 | 0.2 | 0.5 | 1.3 |  |
| onset delay (ms) | 131 | 33.0 | 19.0 | 46.0 | 22.0 | 7.7 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.8 | -3.95 | -4.61 | -5.64 | -6.53 | 1.0 | yes |
| envelope curve, note for note (dB rms) | 19.1 (nearest 18.6) | fonts among themselves 5.83 | | | | 3.3 | yes |
| pitch re 12-TET A440 (median) (c) | -0.2 | -0.9 | 2.2 | 0.1 | 1.2 | 0.2 |  |
| pitch scatter from key to key (c) | 3.2 | 0.6 | 1.2 | 1.9 | 1 | 0.7 |  |
| noisiness (spectral flatness) (dB) | -29.1 | -58.0 | -67.2 | -56.5 | -61.7 | 5.7 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -66.0 | -124 | -121 | -118 | -109 | 7.0 | yes |
| pitch at the start re settled (c) | 2.1 | 0.6 | -3.6 | 0.6 | 7.9 | 0.3 |  |
| stereo coherence | 0.423 | 0.843 | 1 | 1 | 1 | 7.4 | yes |
| partial-to-partial jaggedness (dB) | 6.5 | 7.9 | 6.9 | 5.7 | 7.3 | 0.5 |  |
| dip between legato notes (dB) | 4.2 | 10.6 | 6.8 | 7.3 | 9 | 2.0 | yes |
| level evenness of a legato scale (dB) | 4.7 | 0.7 | 1.2 | 0.9 | 1.6 | 3.7 | yes |
| level over the keyboard (dB/oct) | 3.11 | 1.42 | 1.43 | -0.5 | 0.57 | 2.1 | yes |
| level against the other instruments (velocity 96) (dB) | 0.8 | 3.4 | 3 | 0.1 | 5.2 | 0.9 |  |
| key-to-key level scatter (dB) | 4.83 | 0.82 | 0.66 | 0.63 | 0.72 | 8.3 | yes |
| odd / even partials (dB) | 5.9 | 10.8 | 9 | 8.7 | 10.0 | 1.8 | yes |
| onset jitter in a phrase (ms) | 40.9 | 22.2 | 17.4 | 16.7 | 13.8 | 4.8 | yes |
| onset in a phrase (re the note-on) (ms) | 28.0 | 8 | 19.0 | 14.0 | 0 | 1.7 | yes |
| onset sharpness (dB/ms) | 3.5 | 2.4 | 4.1 | 2.4 | 4.1 | 0.1 |  |
| overtone levels, note for note (dB rms) | 11.1 (nearest 10.3) | fonts among themselves 10.3 | | | | 1.1 |  |
| panning across the keys (dB/oct) | -0.06 | 0.62 | 0 | 0 | -0.05 | 0.1 |  |
| partial envelopes alike | 0.092 | 0.596 | 0.999 | 0.581 | 0.769 | 2.8 | yes |
| release time (30 dB after note-off) (ms) | 616 | 6 | 106 | 111 | 92.0 | 1.3 | yes |
| spectrogram (1 - correlation), note for note | 0.405 (nearest 0.367) | fonts among themselves 0.0959 | | | | 4.2 | yes |
| spectral shape of the body, note for note | 16 (nearest 14.2) | fonts among themselves 9.18 | | | | 1.7 | yes |
| sounding length of a 100 ms note (ms) | 358 | 104 | 152 | 139 | 143 | 4.9 | yes |
| partial stretch (inharmonicity) (c/partial) | 0 | 0.01 | 0 | 0 | 0 | 0.0 |  |
| level after 2 s held, re peak (dB) | -3.7 | -3.9 | -3.3 | -2.7 | -2.4 | 0.2 |  |
| high bands lag low bands by (ms) | -34.2 | 16.3 | 6.7 | 15.3 | 10.7 | 9.4 | yes |
| transient coherence: attack spread over bands (ms) | 13.6 | 7.2 | 2.7 | 6.2 | 5 | 1.6 | yes |
| fundamental share (tristimulus 1) | 0.66 | 0.239 | 0.632 | 0.652 | 0.765 | 0.1 |  |
| partials 2-4 share (tristimulus 2) | 0.145 | 0.297 | 0.329 | 0.225 | 0.184 | 1.6 | yes |
| partials 5+ share (tristimulus 3) | 0.137 | 0.415 | 0.119 | 0.047 | 0.06 | 0.3 |  |
| loudness range, velocity 40 to 127 (dB) | 11.9 | 16.4 | 16.7 | 16.6 | 20.1 | 2.6 | yes |
| pitch wobble / vibrato (p-p) (c) | 5.3 | 0.4 | 0 | 0.6 | 2.4 | 1.0 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0212 (nearest 0.0199) | fonts among themselves 0.0125 | | | | 1.7 | yes |

</details>

### 73 Piccolo: piccolo

Distance 4.33 (rank 14 of 129); the fonts disagree 1.2 JND. Keys 60,63,66,69,72,75,78,81,84,87,90,93,96, velocities 40/64/96/127.

- **Wrong octave: sounds an octave high (piccolo transpose +12)**: FAULT. It is the `-play` transposition bug: fundamental share 0, partials 2-4 share 0.91 (energy on the key's even harmonics), centroid 24.2 vs 13.3-15 ERB. GM piccolo sounds at the key. The `-ensemble` piccolo (README "The flute family") is right to transpose; `-play` is not.
- **Key-to-key scatter (brightness 3.4 ERB, level 3.4 dB)**: re-measure after the fix. Some of it is the top keys folding past B7.
- **Release 677 ms, staccato 318 ms, coherence 0.48**: the room (ARTIFACT).
- **Noisy (flatness -24 dB vs -37..-81)**: breath noise. Plausible for a piccolo; the fonts disagree widely.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (20.8 ERB | 13.3 | 13.3 | 14.5 | 15.0), outside the fonts, 8.1 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (751 ms | 159 | 100 | 121 | 119), outside the fonts, 7.9 x.
3. **key-to-key brightness scatter**: less even in tone from key to key (4.01 ERB | 0.185 | 0.214 | 0.482 | 0.759), outside the fonts, 12.8 x.
4. **fundamental share (tristimulus 1)**: less fundamental (0.158 | 0.986 | 0.964 | 0.891 | 0.979), outside the fonts, 11.6 x.
5. **level against the other instruments (velocity 96)**: quieter (-1.2 dB | 3.2 | 4 | 1.9 | 1.5), outside the fonts, 3.0 x.
6. **brightness over the keyboard**: brightens slower up the keyboard (-3.22 ERB/oct | 3.54 | 3.61 | 4.25 | 2.35), outside the fonts, 7.2 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | - | -54 | -18 | -2 | +17 | +19 |
| re TimGM6mb | - | -54 | -18 | -3 | +16 | +20 |
| re Jnsgm2 | - | -51 | -17 | -2 | +15 | +19 |
| re SGM | - | -51 | -18 | -5 | +17 | +21 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -5 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 5.9 dB rms; TimGM6mb: centroid -0.0 ERB, attack -0.3 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 1.7 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.1 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 1.7 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 1.92 | 0.02 | 1.13 | 0.25 | -0.33 | 2.4 | yes |
| attack time (-20 to -1 dB) (ms) | 104 | 329 | 28.0 | 60.0 | 31.0 | 0.7 |  |
| spectral shape of the attack, note for note | 12.7 (nearest 12.2) | fonts among themselves 9.17 | | | | 1.4 | yes |
| brightness over the keyboard (ERB/oct) | -3.22 | 3.54 | 3.61 | 4.25 | 2.35 | 7.2 | yes |
| key-to-key brightness scatter (ERB) | 4.01 | 0.185 | 0.214 | 0.482 | 0.759 | 12.8 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.26 | 0 | -0 | -0 | -0.07 | 0.9 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 20.8 | 13.3 | 13.3 | 14.5 | 15.0 | 8.1 | yes |
| onset delay (ms) | 44.0 | 72.0 | 50.0 | 54.0 | 28.0 | 0.4 |  |
| Plomp-Levelt dissonance of a note (log2) | -3.08 | -6.64 | -7.64 | -4.92 | -6.51 | 2.6 | yes |
| envelope curve, note for note (dB rms) | 18.2 (nearest 17.2) | fonts among themselves 4.3 | | | | 4.2 | yes |
| pitch re 12-TET A440 (median) (c) | -0.9 | 5.8 | 2.4 | 8.2 | 1.3 | 1.2 | yes |
| pitch scatter from key to key (c) | 1.7 | 2.2 | 1 | 6.4 | 1.6 | 0.1 |  |
| noisiness (spectral flatness) (dB) | -25.4 | -40.4 | -81.0 | -37.3 | -45.0 | 0.8 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -67.5 | -123 | -122 | -121 | -108 | 7.2 | yes |
| pitch at the start re settled (c) | 2.5 | -5.9 | 3.5 | 12.0 | -1.2 | 0.2 |  |
| stereo coherence | 0.384 | 0.751 | 1 | 1 | 1 | 4.9 | yes |
| partial-to-partial jaggedness (dB) | 3.55 | 2.3 | 4.4 | 3.2 | 4 | 0.0 |  |
| dip between legato notes (dB) | 7.4 | 13.6 | 15.6 | 12.2 | 5.7 | 1.1 |  |
| level evenness of a legato scale (dB) | 4.9 | 0.8 | 0.6 | 0.3 | 2.1 | 4.2 | yes |
| level over the keyboard (dB/oct) | -2.37 | 0.34 | -0.15 | 0.14 | 4.13 | 1.2 | yes |
| level against the other instruments (velocity 96) (dB) | -1.2 | 3.2 | 4 | 1.9 | 1.5 | 3.0 | yes |
| key-to-key level scatter (dB) | 2.43 | 1.54 | 0.4 | 0.56 | 1.11 | 2.8 | yes |
| odd / even partials (dB) | -10.8 | -0.5 | 3.1 | 1.4 | -3.8 | 3.3 | yes |
| onset jitter in a phrase (ms) | 41.2 | 25.5 | 22.7 | 23.9 | 13.0 | 2.9 | yes |
| onset in a phrase (re the note-on) (ms) | 27.0 | 20.0 | 19.0 | 18.0 | 8 | 0.8 | yes |
| onset sharpness (dB/ms) | 2.5 | 1.4 | 1.7 | 1.9 | 2.5 | 0.4 |  |
| overtone levels, note for note (dB rms) | 21.1 (nearest 16.1) | fonts among themselves 7.44 | | | | 2.8 | yes |
| panning across the keys (dB/oct) | 0.44 | -0.64 | 0 | 0 | 0.04 | 0.4 |  |
| partial envelopes alike | -0.006 | 0.279 | 0.335 | 0.474 | 0.502 | 3.7 | yes |
| release time (30 dB after note-off) (ms) | 751 | 159 | 100 | 121 | 119 | 7.9 | yes |
| spectrogram (1 - correlation), note for note | 0.278 (nearest 0.24) | fonts among themselves 0.101 | | | | 2.8 | yes |
| spectral shape of the body, note for note | 10.5 (nearest 10.1) | fonts among themselves 6.26 | | | | 1.7 | yes |
| sounding length of a 100 ms note (ms) | 400 | 193 | 166 | 182 | 150 | 4.8 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.02 | 0.07 | 0 | 0.5 | 0.01 | 0.1 |  |
| level after 2 s held, re peak (dB) | -3.5 | -3.7 | -0.6 | -5.4 | -3.6 | 0.1 |  |
| high bands lag low bands by (ms) | -10.8 | 5 | -19.5 | -22.0 | 2 | 0.2 |  |
| transient coherence: attack spread over bands (ms) | 9.1 | 4.1 | 2.4 | 5.1 | 1.2 | 1.2 | yes |
| fundamental share (tristimulus 1) | 0.158 | 0.986 | 0.964 | 0.891 | 0.979 | 11.6 | yes |
| partials 2-4 share (tristimulus 2) | 0.224 | 0.014 | 0.035 | 0.109 | 0.021 | 2.8 | yes |
| partials 5+ share (tristimulus 3) | 0.01 | 0 | 0 | 0 | 0 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 16.0 | 16.6 | 16.6 | 16.6 | 20.0 | 0.4 |  |
| pitch wobble / vibrato (p-p) (c) | 3.3 | 21.9 | 0 | 15.1 | 5.1 | 0.6 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0184 (nearest 0.018) | fonts among themselves 0.00966 | | | | 1.9 | yes |

</details>

### 74 Flute: flute

Distance 2.91 (rank 55 of 129); the fonts disagree 1.2 JND. Keys 60,63,66,69,72,75,78,81,84,87,90,93,96, velocities 40/64/96/127.

- **Far too much louder toward the top (16.9 dB/oct vs 1.7-3.5; about 50 dB over C4-C7)**: FAULT. Real flutes gain maybe 15-20 dB over the range. This needs a per-note level evening like the choir's `PJ_CHOIR_EVEN`.
- **Quieter overall (-3.1 dB vs +1.8..+3.9); key level scatter 3.5 dB; legato scale uneven 4.2 dB**: FAULT, the same root (low notes far too soft).
- **Brighter with pitch (5.1 ERB/oct vs 0.4-2.4)**: follows from the jet overblowing more at the top. Partly DELIBERATE physics (README Status, the jet).
- **Release 634 ms**: the room. Onset (55 ms) and pitch are inside the fonts.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (3.63 dB | 0.82 | 0.88 | 0.88 | 1.01), outside the fonts, 5.5 x.
2. **level over the keyboard**: louder toward the top (9.98 dB/oct | 1.69 | 3.05 | 3.34 | 3.47), outside the fonts, 6.8 x.
3. **brightness (spectral centroid, velocity 96)**: brighter (18.1 ERB | 15.8 | 13.4 | 13.4 | 15.7), outside the fonts, 2.9 x.
4. **key-to-key brightness scatter**: less even in tone from key to key (1.64 ERB | 0.778 | 0.489 | 0.401 | 0.773), outside the fonts, 5.0 x.
5. **release time (30 dB after note-off) [room]**: rings longer after the key (634 ms | 154 | 180 | 206 | 90.0), outside the fonts, 3.2 x.
6. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-66.2 dB | -124 | -123 | -123 | -107), outside the fonts, 7.0 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | - | -7 | -10 | -7 | +4 | +11 |
| re TimGM6mb | -32 | -11 | -8 | -3 | +5 | +21 |
| re Jnsgm2 | -28 | -8 | -4 | +2 | +6 | +9 |
| re SGM | - | -2 | -9 | -7 | +1 | +12 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.7 ERB, attack -0.0 ERB, velocity brightening +0.1 ERB, loudness range +3 dB, envelope 5.9 dB rms; TimGM6mb: centroid -0.0 ERB, attack +0.0 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 1.7 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.0 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 2.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 1.59 | -0.7 | -0.08 | 1.07 | -0.24 | 2.0 | yes |
| attack time (-20 to -1 dB) (ms) | 108 | 61.0 | 293 | 604 | 27.0 | 0.1 |  |
| spectral shape of the attack, note for note | 12.9 (nearest 9.41) | fonts among themselves 9.66 | | | | 1.3 |  |
| brightness over the keyboard (ERB/oct) | 5.14 | 2.37 | 0.76 | 0.44 | 1.44 | 4.2 | yes |
| key-to-key brightness scatter (ERB) | 1.64 | 0.778 | 0.489 | 0.401 | 0.773 | 5.0 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.01 | -0 | 0 | 0 | 0.11 | 0.0 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 18.1 | 15.8 | 13.4 | 13.4 | 15.7 | 2.9 | yes |
| onset delay (ms) | 55.0 | 55.0 | 47.0 | 70.0 | 35.0 | 0.2 |  |
| Plomp-Levelt dissonance of a note (log2) | -4.99 | -5.76 | -5.01 | -5.21 | -6.06 | 0.9 |  |
| envelope curve, note for note (dB rms) | 17.1 (nearest 16.5) | fonts among themselves 5.33 | | | | 3.2 | yes |
| pitch re 12-TET A440 (median) (c) | 0.6 | 2.7 | 1.4 | -1.9 | 0.1 | 0.0 |  |
| pitch scatter from key to key (c) | 2.4 | 3.4 | 2.8 | 4.7 | 0.1 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -30.1 | -39.6 | -36.5 | -37.0 | -44.4 | 2.1 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -66.2 | -124 | -123 | -123 | -107 | 7.0 | yes |
| pitch at the start re settled (c) | 4 | -3.3 | -6.5 | -8.2 | 0.3 | 1.8 | yes |
| stereo coherence | 0.446 | 0.439 | 1 | 1 | 1 | 2.0 |  |
| partial-to-partial jaggedness (dB) | 2.85 | 3.4 | 2.1 | 2.3 | 3.7 | 0.0 |  |
| dip between legato notes (dB) | 6.2 | 10.1 | 8.6 | 13.0 | 9.8 | 1.7 | yes |
| level evenness of a legato scale (dB) | 3.3 | 0.9 | 1.8 | 0.5 | 1.2 | 2.2 | yes |
| level over the keyboard (dB/oct) | 9.98 | 1.69 | 3.05 | 3.34 | 3.47 | 6.8 | yes |
| level against the other instruments (velocity 96) (dB) | 3.4 | 3.8 | 4.1 | 3.1 | 3 | 0.1 |  |
| key-to-key level scatter (dB) | 3.63 | 0.82 | 0.88 | 0.88 | 1.01 | 5.5 | yes |
| odd / even partials (dB) | -9.1 | -3.8 | -9.9 | -6.3 | -7.8 | 0.7 |  |
| onset jitter in a phrase (ms) | 23.6 | 25.1 | 26.4 | 28.1 | 19.2 | 0.4 |  |
| onset in a phrase (re the note-on) (ms) | 40.0 | 18.0 | 8 | 22.0 | 12.0 | 2.5 | yes |
| onset sharpness (dB/ms) | 3 | 1.2 | 2.3 | 2.3 | 2.4 | 0.4 |  |
| overtone levels, note for note (dB rms) | 8.71 (nearest 8.52) | fonts among themselves 7.47 | | | | 1.2 |  |
| panning across the keys (dB/oct) | -0.03 | -0.38 | 0 | 0 | 0.04 | 0.0 |  |
| partial envelopes alike | -0.022 | 0.058 | 0.645 | 0.352 | 0.092 | 0.8 | yes |
| release time (30 dB after note-off) (ms) | 634 | 154 | 180 | 206 | 90.0 | 3.2 | yes |
| spectrogram (1 - correlation), note for note | 0.222 (nearest 0.201) | fonts among themselves 0.114 | | | | 2.0 | yes |
| spectral shape of the body, note for note | 8.59 (nearest 7.85) | fonts among themselves 7.2 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 400 | 188 | 211 | 241 | 141 | 2.6 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.01 | 0.15 | 0.15 | 0.2 | 0 | 0.3 |  |
| level after 2 s held, re peak (dB) | -3.5 | -4.8 | -2.3 | -3.4 | -2 | 0.2 |  |
| high bands lag low bands by (ms) | -16.4 | 23.1 | 9 | -24.0 | 2 | 0.9 |  |
| transient coherence: attack spread over bands (ms) | 5.1 | 19.1 | 5.5 | 5.7 | 1.6 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.804 | 0.765 | 0.936 | 0.954 | 0.855 | 1.0 |  |
| partials 2-4 share (tristimulus 2) | 0.195 | 0.234 | 0.063 | 0.045 | 0.144 | 1.0 |  |
| partials 5+ share (tristimulus 3) | 0.001 | 0.001 | 0 | 0.001 | 0.001 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 15.2 | 16.7 | 16.6 | 16.6 | 20.1 | 0.8 | yes |
| pitch wobble / vibrato (p-p) (c) | 3.8 | 18.4 | 9.3 | 21.2 | 0.3 | 1.0 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0152 (nearest 0.0146) | fonts among themselves 0.0114 | | | | 1.3 |  |

</details>

### 75 Recorder: recorder

Distance 2.64 (rank 79 of 129); the fonts disagree 1.0 JND. Keys 60,63,66,69,72,75,78,81,84,87,90,93,96, velocities 40/64/96/127.

- **Uneven from key to key (level 5.1 dB vs 0.04-1.1; brightness 2.1 ERB; pitch scatter 15 c vs 0.1-7)**: partly MEASUREMENT ARTIFACT. 5 of 13 probe keys lie outside F4-E6 and fold by an octave. The rest is a known FAULT: README Status lists A#5, C#6 and D6 as off (second register barely leaving the first mode).
- **Louder toward the top (6.7 dB/oct vs 0.8-2.7)**: a duct's breath sets pitch and level together (README "Other woodwinds", "A duct"). Partly physics, but steeper than any font.
- **Breathier (flatness -29 vs -43..-66 dB; fundamental share 0.83 vs 0.94-0.99)**: plausible chiff. Minor.
- **Release 414 ms**: the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key brightness scatter**: less even in tone from key to key (2.13 ERB | 0.25 | 0.206 | 0.338 | 0.398), outside the fonts, 9.2 x.
2. **key-to-key level scatter**: less even from key to key (3.38 dB | 1.06 | 0.04 | 0.89 | 1.07), outside the fonts, 4.7 x.
3. **release time (30 dB after note-off) [room]**: rings longer after the key (414 ms | 53.0 | 42.0 | 91.0 | 94.0), outside the fonts, 4.4 x.
4. **pitch scatter from key to key**: less evenly tuned (15.3 c | 1.5 | 0.4 | 7.1 | 0.1), outside the fonts, 4.1 x.
5. **Plomp-Levelt dissonance of a note**: rougher (-4.53 log2 | -6.44 | -6.8 | -6.97 | -7.97), outside the fonts, 3.1 x.
6. **brightening from velocity 40 to 127**: brightens more with velocity (0.56 ERB | 0.07 | -0 | -0 | -0.21), outside the fonts, 1.9 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | - | -41 | -7 | -11 | +6 | +23 |
| re TimGM6mb | - | -35 | +0 | -2 | +6 | +9 |
| re Jnsgm2 | - | -40 | -6 | -9 | +4 | +22 |
| re SGM | - | -34 | +1 | -3 | +4 | +10 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -5 +0 +3; FluidR3 -12 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack -0.1 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 1.8 dB rms; TimGM6mb: centroid -0.0 ERB, attack +0.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 1.8 dB rms; Jnsgm2: centroid -0.0 ERB, attack +0.0 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 1.9 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.68 | 1.3 | 0.72 | 0.09 | 0.05 | 0.4 |  |
| attack time (-20 to -1 dB) (ms) | 52.0 | 74.0 | 38.0 | 745 | 166 | 0.5 |  |
| spectral shape of the attack, note for note | 15.3 (nearest 13.9) | fonts among themselves 11.1 | | | | 1.4 |  |
| brightness over the keyboard (ERB/oct) | 2.76 | 3.52 | 3.43 | 3.72 | 3.03 | 2.1 | yes |
| key-to-key brightness scatter (ERB) | 2.13 | 0.25 | 0.206 | 0.338 | 0.398 | 9.2 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.56 | 0.07 | -0 | -0 | -0.21 | 1.9 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 13.4 | 13.5 | 13.1 | 14.8 | 13.3 | 0.0 |  |
| onset delay (ms) | 40.0 | 36.0 | 42.0 | 16.0 | 25.0 | 0.7 |  |
| Plomp-Levelt dissonance of a note (log2) | -4.53 | -6.44 | -6.8 | -6.97 | -7.97 | 3.1 | yes |
| envelope curve, note for note (dB rms) | 17 (nearest 16.2) | fonts among themselves 5.47 | | | | 3.1 | yes |
| pitch re 12-TET A440 (median) (c) | 5.3 | 6.1 | 2.8 | 4 | -0.2 | 0.5 |  |
| pitch scatter from key to key (c) | 15.3 | 1.5 | 0.4 | 7.1 | 0.1 | 4.1 | yes |
| noisiness (spectral flatness) (dB) | -29.2 | -58.3 | -66.3 | -43.4 | -48.5 | 2.1 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -72.3 | -126 | -122 | -125 | -106 | 5.1 | yes |
| pitch at the start re settled (c) | 9.8 | -1.6 | 0.2 | -3 | -5.6 | 2.4 | yes |
| stereo coherence | 0.908 | 0.999 | 1 | 1 | 1 | 1.2 | yes |
| partial-to-partial jaggedness (dB) | 3.85 | 4.65 | 5.1 | 3.8 | 4.9 | 0.9 |  |
| dip between legato notes (dB) | 8.2 | 5.5 | 12.2 | 5.5 | 11.2 | 0.0 |  |
| level evenness of a legato scale (dB) | 2.7 | 1.1 | 0.3 | 1.3 | 1.8 | 1.5 | yes |
| level over the keyboard (dB/oct) | 1.19 | 1.94 | 0.77 | 1.01 | 2.69 | 0.3 |  |
| level against the other instruments (velocity 96) (dB) | 5.4 | 4.6 | 4.7 | 5.8 | 3.3 | 0.6 |  |
| key-to-key level scatter (dB) | 3.38 | 1.06 | 0.04 | 0.89 | 1.07 | 4.7 | yes |
| odd / even partials (dB) | -5.7 | -0.5 | 7.2 | -5.9 | 8.8 | 1.2 |  |
| onset jitter in a phrase (ms) | 22.5 | 13.1 | 19.9 | 16.3 | 13.9 | 1.5 | yes |
| onset in a phrase (re the note-on) (ms) | 34.0 | 9 | 14.0 | 8 | 6 | 2.5 | yes |
| onset sharpness (dB/ms) | 3 | 2.8 | 4.3 | 3.5 | 4 | 0.4 |  |
| overtone levels, note for note (dB rms) | 15.3 (nearest 11.1) | fonts among themselves 14.7 | | | | 1.0 |  |
| panning across the keys (dB/oct) | -0.35 | 0 | 0 | 0 | 0.04 | 0.3 |  |
| partial envelopes alike | -0.014 | 0.603 | 0.998 | 0.622 | 0.606 | 3.2 | yes |
| release time (30 dB after note-off) (ms) | 414 | 53.0 | 42.0 | 91.0 | 94.0 | 4.4 | yes |
| spectrogram (1 - correlation), note for note | 0.338 (nearest 0.318) | fonts among themselves 0.145 | | | | 2.3 | yes |
| spectral shape of the body, note for note | 12.1 (nearest 10.2) | fonts among themselves 9.59 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 216 | 127 | 128 | 147 | 165 | 2.6 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.01 | -0.12 | 0 | 0.01 | 0.01 | 0.0 |  |
| level after 2 s held, re peak (dB) | -3.2 | -4.8 | -2.5 | -3.1 | -1.4 | 0.1 |  |
| high bands lag low bands by (ms) | -5 | 0.5 | 0 | 3 | -0.7 | 1.1 | yes |
| transient coherence: attack spread over bands (ms) | 2.5 | 14.3 | 2 | 1.7 | 2.1 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.833 | 0.982 | 0.99 | 0.935 | 0.977 | 2.1 | yes |
| partials 2-4 share (tristimulus 2) | 0.154 | 0.018 | 0.006 | 0.065 | 0.015 | 2.0 | yes |
| partials 5+ share (tristimulus 3) | 0.003 | 0 | 0.004 | 0 | 0.005 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 15.5 | 16.4 | 16.6 | 16.6 | 20.1 | 0.6 | yes |
| pitch wobble / vibrato (p-p) (c) | 4.6 | 10.0 | 0 | 12.7 | 0.9 | 0.1 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0188 (nearest 0.0168) | fonts among themselves 0.00753 | | | | 2.5 | yes |

</details>

### 76 Pan Flute: panflute

Distance 2.66 (rank 74 of 129); the fonts disagree 1.0 JND. Keys 60,63,66,69,72,75,78,81,84,87,90,93,96, velocities 40/64/96/127.

- **Louder toward the bottom (-5.6 dB/oct vs -0.4..+1.8); quieter (-5.1 dB); legato scale uneven 4.7 dB**: FAULT. The high pipes are weak. Probe keys 60-66 fold up an octave, which hides part of it.
- **Key-to-key scatter (brightness 1.8 ERB, level 2.8 dB)**: FAULT, per-pipe levels not evened (one stopped pipe per note).
- **Brightens less up the keyboard (1.6 vs 2.8-3.4 ERB/oct)**: partly the folding (ARTIFACT).
- **Release 592 ms, staccato 394 ms**: the room. Pitch is fine (README Status: "all 30 in tune").

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **level over the keyboard**: louder toward the bottom (-7.61 dB/oct | 1.01 | 1.8 | 0.24 | -0.37), outside the fonts, 7.6 x.
2. **key-to-key brightness scatter**: less even in tone from key to key (1.79 ERB | 0.294 | 0.559 | 0.21 | 0.336), outside the fonts, 7.4 x.
3. **key-to-key level scatter**: less even from key to key (2.9 dB | 0.88 | 0.54 | 0.25 | 0.52), outside the fonts, 4.7 x.
4. **brightness over the keyboard**: brightens slower up the keyboard (1.6 ERB/oct | 3.36 | 2.82 | 3.43 | 3.17), outside the fonts, 5.5 x.
5. **release time (30 dB after note-off) [room]**: rings longer after the key (592 ms | 66.0 | 68.0 | 120 | 217), outside the fonts, 3.2 x.
6. **sounding length of a 100 ms note [room]**: longer staccato (400 ms | 137 | 144 | 186 | 255), outside the fonts, 2.9 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | - | -48 | -2 | +1 | +4 | +14 |
| re TimGM6mb | - | -45 | -1 | +1 | +3 | +12 |
| re Jnsgm2 | - | -54 | -6 | -1 | +7 | +18 |
| re SGM | - | -45 | +1 | +4 | +5 | +7 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack -0.0 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 4.5 dB rms; TimGM6mb: centroid +0.0 ERB, attack +0.1 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 2.0 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.1 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 4.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.97 | -0.18 | -0.08 | 1.59 | 7.03 | 0.1 |  |
| attack time (-20 to -1 dB) (ms) | 79.5 | 148 | 34.0 | 138 | 73.0 | 0.3 |  |
| spectral shape of the attack, note for note | 9.13 (nearest 6.44) | fonts among themselves 8.53 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | 1.6 | 3.36 | 2.82 | 3.43 | 3.17 | 5.5 | yes |
| key-to-key brightness scatter (ERB) | 1.79 | 0.294 | 0.559 | 0.21 | 0.336 | 7.4 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.11 | 0.53 | -0 | -0 | -0.17 | 0.3 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 14.5 | 13.9 | 15.3 | 13.1 | 13.8 | 0.6 |  |
| onset delay (ms) | 31.0 | 28.0 | 23.0 | 13.0 | 42.0 | 0.4 |  |
| Plomp-Levelt dissonance of a note (log2) | -4.61 | -5.7 | -3.1 | -5.8 | -3.72 | 0.1 |  |
| envelope curve, note for note (dB rms) | 18.2 (nearest 15.1) | fonts among themselves 6.61 | | | | 2.8 | yes |
| pitch re 12-TET A440 (median) (c) | -1.4 | -0.7 | 1.7 | -2 | 2 | 0.5 |  |
| pitch scatter from key to key (c) | 1.9 | 1 | 0.3 | 0.7 | 1 | 0.3 |  |
| noisiness (spectral flatness) (dB) | -25.2 | -41.4 | -39.0 | -47.8 | -24.5 | 1.3 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -69.2 | -124 | -119 | -124 | -106 | 5.7 | yes |
| pitch at the start re settled (c) | 4.4 | -0.7 | -0.3 | -2 | -0.8 | 1.0 | yes |
| stereo coherence | 0.775 | 0.938 | 1 | 1 | 1 | 3.0 | yes |
| partial-to-partial jaggedness (dB) | 5.6 | 8 | 7.4 | 10.8 | 8 | 1.4 | yes |
| dip between legato notes (dB) | 8.4 | 12.3 | 7.5 | 9.7 | 9.5 | 0.5 |  |
| level evenness of a legato scale (dB) | 2.7 | 0.7 | 1.5 | 1.2 | 0.4 | 1.8 | yes |
| level over the keyboard (dB/oct) | -7.61 | 1.01 | 1.8 | 0.24 | -0.37 | 7.6 | yes |
| level against the other instruments (velocity 96) (dB) | 4.9 | 3.7 | 3.2 | 5.6 | -0.1 | 0.5 |  |
| key-to-key level scatter (dB) | 2.9 | 0.88 | 0.54 | 0.25 | 0.52 | 4.7 | yes |
| odd / even partials (dB) | 13.6 | 17.3 | 14.2 | 18.2 | 17.1 | 1.8 |  |
| onset jitter in a phrase (ms) | 20.0 | 15.0 | 14.5 | 13.9 | 24.2 | 1.0 |  |
| onset in a phrase (re the note-on) (ms) | 24.0 | 12.0 | 12.0 | 4 | 9 | 1.4 | yes |
| onset sharpness (dB/ms) | 3.3 | 1.8 | 4.8 | 5 | 3.2 | 0.4 |  |
| overtone levels, note for note (dB rms) | 9.59 (nearest 7.93) | fonts among themselves 8.87 | | | | 1.1 |  |
| panning across the keys (dB/oct) | -1.2 | -0.27 | 0 | 0 | 0.04 | 1.2 | yes |
| partial envelopes alike | -0.029 | 0.861 | 0.562 | 0.176 | 0.45 | 1.6 | yes |
| release time (30 dB after note-off) (ms) | 592 | 66.0 | 68.0 | 120 | 217 | 3.2 | yes |
| spectrogram (1 - correlation), note for note | 0.204 (nearest 0.102) | fonts among themselves 0.12 | | | | 1.7 |  |
| spectral shape of the body, note for note | 10.6 (nearest 5.63) | fonts among themselves 7.33 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 400 | 137 | 144 | 186 | 255 | 2.9 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.02 | 0 | -0.01 | - | 1.19 | 0.0 |  |
| level after 2 s held, re peak (dB) | -2.8 | -3.2 | -1 | -5.1 | -5.2 | 0.5 |  |
| high bands lag low bands by (ms) | -8.2 | 6.5 | -13.7 | -6 | -28.8 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 2.1 | 6.3 | 5.6 | 2.8 | 11.9 | 0.8 |  |
| fundamental share (tristimulus 1) | 0.863 | 0.941 | 0.892 | 0.994 | 0.955 | 1.2 |  |
| partials 2-4 share (tristimulus 2) | 0.13 | 0.041 | 0.106 | 0.006 | 0.043 | 1.3 |  |
| partials 5+ share (tristimulus 3) | 0.004 | 0.017 | 0.002 | 0 | 0.002 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 16.6 | 17.0 | 16.6 | 16.7 | 20.1 | 0.1 |  |
| pitch wobble / vibrato (p-p) (c) | 3.7 | 5.2 | 2.6 | 3.9 | 5.4 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0155 (nearest 0.0108) | fonts among themselves 0.00923 | | | | 1.7 |  |

</details>

### 77 Blown Bottle: bottle

Distance 2.81 (rank 62 of 129); the fonts disagree 1.2 JND. Keys 60,63,66,69,72,75,78,81,84,87,90,93,96, velocities 40/64/96/127.

- **Much louder toward the top (18.2 dB/oct vs -1.1..+0.3; key level scatter 2.6 dB)**: FAULT. Keys 87-96 fold down into C4-C6, which should flatten the slope, so the small high bottles are genuinely much louder. Even out the per-bottle levels.
- **Legato separated and uneven (dip 12.6 dB vs 4-7.7; evenness 5.7 dB)**: the dip is DELIBERATE physics, since each note is another bottle and cannot be slurred. The unevenness is the level FAULT above.
- **Slightly darker (12.1 vs 13.0-15.3 ERB)**: small, inside the fonts' spread on the bands.
- **Release 398 ms**: the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **level over the keyboard**: louder toward the top (9.44 dB/oct | -0.01 | -0.5 | -1.11 | 0.32), outside the fonts, 9.7 x.
2. **key-to-key level scatter**: less even from key to key (3.08 dB | 0.32 | 0.28 | 0.36 | 1.13), outside the fonts, 5.5 x.
3. **key-to-key brightness scatter**: less even in tone from key to key (1.45 ERB | 0.191 | 0.537 | 0.204 | 0.34), outside the fonts, 5.9 x.
4. **brightness (spectral centroid, velocity 96)**: darker (12.1 ERB | 13.4 | 15.3 | 13.0 | 14.8), outside the fonts, 1.8 x.
5. **release time (30 dB after note-off) [room]**: rings longer after the key (398 ms | 204 | 68.0 | 122 | 111), outside the fonts, 2.2 x.
6. **dip between legato notes**: more separated legato (11.9 dB | 7.7 | 5.3 | 5 | 3.9), outside the fonts, 3.4 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | - | -2 | -2 | -12 | +10 | +5 |
| re TimGM6mb | - | +5 | +3 | -13 | +1 | +7 |
| re Jnsgm2 | - | -3 | -1 | -12 | +10 | +5 |
| re SGM | - | +4 | +5 | -9 | -0 | +3 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.1 ERB, attack -0.0 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 8.5 dB rms; TimGM6mb: centroid +0.6 ERB, attack +0.8 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 2.4 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.1 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 2.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.69 | 0.43 | 0.43 | 0.48 | 7.66 | 0.1 |  |
| attack time (-20 to -1 dB) (ms) | 30.0 | 105 | 34.0 | 21.0 | 63.0 | 0.5 |  |
| spectral shape of the attack, note for note | 13.9 (nearest 11.8) | fonts among themselves 11.2 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 4.91 | 3.59 | 3.11 | 3.36 | 2.45 | 2.9 | yes |
| key-to-key brightness scatter (ERB) | 1.45 | 0.191 | 0.537 | 0.204 | 0.34 | 5.9 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.03 | -0 | -0 | -0 | -0.01 | 0.1 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 12.1 | 13.4 | 15.3 | 13.0 | 14.8 | 1.8 | yes |
| onset delay (ms) | 40.0 | 29.0 | 25.0 | 22.0 | 25.0 | 1.5 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.18 | -2.86 | -3.2 | -3.9 | -2.66 | 1.9 | yes |
| envelope curve, note for note (dB rms) | 17.2 (nearest 16.7) | fonts among themselves 6.87 | | | | 2.5 | yes |
| pitch re 12-TET A440 (median) (c) | -0.6 | 21.8 | -31.4 | 2 | -1.5 | 0.0 |  |
| pitch scatter from key to key (c) | 1.6 | 0.6 | 1.8 | 0.1 | 3.7 | 0.1 |  |
| noisiness (spectral flatness) (dB) | -29.1 | -47.7 | -39.1 | -63.1 | -22.6 | 0.7 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -71.8 | -127 | -122 | -122 | -105 | 4.5 | yes |
| pitch at the start re settled (c) | 4.2 | -0.7 | 0.9 | 1.7 | 4.8 | 0.6 |  |
| stereo coherence | 0.734 | 0.851 | 1 | 1 | 1 | 3.5 | yes |
| partial-to-partial jaggedness (dB) | 4.05 | 10.7 | 7.3 | 7.3 | 11.2 | 2.5 | yes |
| dip between legato notes (dB) | 11.9 | 7.7 | 5.3 | 5 | 3.9 | 3.4 | yes |
| level evenness of a legato scale (dB) | 3 | 0.9 | 1 | 1.3 | 0.6 | 2.0 | yes |
| level over the keyboard (dB/oct) | 9.44 | -0.01 | -0.5 | -1.11 | 0.32 | 9.7 | yes |
| level against the other instruments (velocity 96) (dB) | 2 | 4.7 | 0.8 | 2.4 | -1.8 | 0.1 |  |
| key-to-key level scatter (dB) | 3.08 | 0.32 | 0.28 | 0.36 | 1.13 | 5.5 | yes |
| odd / even partials (dB) | -6.2 | -2.5 | 14.2 | -15.6 | 19.1 | 0.7 |  |
| onset jitter in a phrase (ms) | 32.1 | 28.1 | 19.7 | 13.1 | 24.5 | 1.3 | yes |
| onset in a phrase (re the note-on) (ms) | 28.0 | 10.0 | 14.0 | 8 | 4 | 1.9 | yes |
| onset sharpness (dB/ms) | 3.4 | 3.8 | 5.3 | 3.7 | 4 | 0.2 |  |
| overtone levels, note for note (dB rms) | 18.8 (nearest 14.9) | fonts among themselves 20.3 | | | | 0.9 |  |
| panning across the keys (dB/oct) | -1.68 | 0 | 0 | 0 | 0.04 | 1.7 | yes |
| partial envelopes alike | -0.044 | 0.005 | 0.525 | 0.04 | 0.152 | 0.5 |  |
| release time (30 dB after note-off) (ms) | 398 | 204 | 68.0 | 122 | 111 | 2.2 | yes |
| spectrogram (1 - correlation), note for note | 0.372 (nearest 0.276) | fonts among themselves 0.309 | | | | 1.2 |  |
| spectral shape of the body, note for note | 12.9 (nearest 9.73) | fonts among themselves 12.8 | | | | 1.0 |  |
| sounding length of a 100 ms note (ms) | 216 | 193 | 133 | 171 | 177 | 1.2 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.02 | 5.88 | 0 | - | 2.49 | 0.9 |  |
| level after 2 s held, re peak (dB) | -3.6 | -18.4 | -5.5 | -3.6 | -5.2 | 0.2 |  |
| high bands lag low bands by (ms) | -10.7 | -11.5 | -2.7 | -10.0 | -26.0 | 0.0 |  |
| transient coherence: attack spread over bands (ms) | 1.8 | 3.4 | 8.1 | 2.3 | 13.8 | 0.7 |  |
| fundamental share (tristimulus 1) | 0.96 | 0.999 | 0.888 | 1 | 0.978 | 0.4 |  |
| partials 2-4 share (tristimulus 2) | 0.019 | 0 | 0.109 | 0 | 0.016 | 0.2 |  |
| partials 5+ share (tristimulus 3) | 0.003 | 0 | 0.003 | 0 | 0.005 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 16.7 | 16.6 | 16.6 | 16.6 | 20.1 | 0.1 |  |
| pitch wobble / vibrato (p-p) (c) | 4.2 | 16.0 | 3.5 | 1.9 | 8.3 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0219 (nearest 0.0187) | fonts among themselves 0.0181 | | | | 1.2 |  |

</details>

### 78 Shakuhachi: shakuhachi

Distance 3.55 (rank 31 of 129); the fonts disagree 1.6 JND. Keys 60,63,66,69,72,75,78,81,84,87,90,93,96, velocities 40/64/96/127.

- **Key-to-key level scatter 6.3 dB vs 0.2-1.0**: known FAULT. The probe hits D#4 (63, -76 c) and D#5 (75, silent), both listed in README Status. Keys 87-96 also fold.
- **Onset 109 ms vs 33-38; attack 11.7 ERB brighter than the body**: a long breathy chiff before the tone. A breathy attack is DELIBERATE idiom (end-blown, README "Other woodwinds"), but this one is too long and too loud: probable FAULT.
- **Darker (11.3 vs 13.2-15.5 ERB); more partials 2-4 (0.26 vs 0.01-0.08)**: debatable. Real shakuhachi tone is breathy and hollow; the fonts disagree.
- **Stereo coherence 0.46, release 572 ms**: the room (ARTIFACT).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (5.74 dB | 0.18 | 0.39 | 0.17 | 1), outside the fonts, 10.9 x.
2. **onset delay**: speaks later (109 ms | 38.0 | 36.0 | 37.0 | 33.0), outside the fonts, 7.2 x.
3. **attack brighter than the body by**: a brighter attack (11.7 ERB | 0.77 | 0.63 | -0.54 | 0.46), outside the fonts, 17.1 x.
4. **key-to-key brightness scatter**: less even in tone from key to key (2.2 ERB | 0.201 | 0.23 | 0.405 | 1.02), outside the fonts, 4.6 x.
5. **brightness (spectral centroid, velocity 96)**: darker (11.3 ERB | 13.2 | 13.5 | 14.7 | 15.5), outside the fonts, 2.4 x.
6. **stereo coherence [room]**: wider (0.455 | 0.963 | 1 | 1 | 1), outside the fonts, 7.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -22 | -16 | +3 | -1 | +5 | +6 |
| re TimGM6mb | - | -18 | -1 | -4 | +2 | +13 |
| re Jnsgm2 | - | -12 | +4 | -1 | +2 | +1 |
| re SGM | - | -4 | +2 | -3 | +2 | +1 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.1 ERB, attack -0.1 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 2.5 dB rms; TimGM6mb: centroid -0.0 ERB, attack +0.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 2.0 dB rms; Jnsgm2: centroid -0.1 ERB, attack +0.1 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 4.0 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 11.7 | 0.77 | 0.63 | -0.54 | 0.46 | 17.1 | yes |
| attack time (-20 to -1 dB) (ms) | 140 | 58.0 | 41.0 | 197 | 40.0 | 1.3 |  |
| spectral shape of the attack, note for note | 13.2 (nearest 10.8) | fonts among themselves 11.6 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | 2.48 | 3.67 | 3.73 | 4.54 | -1.72 | 0.4 |  |
| key-to-key brightness scatter (ERB) | 2.2 | 0.201 | 0.23 | 0.405 | 1.02 | 4.6 | yes |
| brightening from velocity 40 to 127 (ERB) | -0.14 | 0 | -0 | 0 | 0.25 | 0.5 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 11.3 | 13.2 | 13.5 | 14.7 | 15.5 | 2.4 | yes |
| onset delay (ms) | 109 | 38.0 | 36.0 | 37.0 | 33.0 | 7.2 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.32 | -4.84 | -4.06 | -4.72 | -3.57 | 0.1 |  |
| envelope curve, note for note (dB rms) | 18.8 (nearest 15.2) | fonts among themselves 9.07 | | | | 2.1 | yes |
| pitch re 12-TET A440 (median) (c) | -0.4 | 0.7 | 0.7 | 12.7 | -0.3 | 0.2 |  |
| pitch scatter from key to key (c) | 4.5 | 1 | 7.7 | 1.2 | 5.6 | 0.3 |  |
| noisiness (spectral flatness) (dB) | -28.2 | -33.7 | -35.1 | -32.0 | -27.4 | 1.2 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -70.0 | -129 | -123 | -120 | -111 | 5.7 | yes |
| pitch at the start re settled (c) | 1.1 | -9.3 | 5.7 | -9.8 | -2.1 | 0.9 |  |
| stereo coherence | 0.455 | 0.963 | 1 | 1 | 1 | 7.3 | yes |
| partial-to-partial jaggedness (dB) | 3.35 | 2.8 | 3.3 | 5.1 | 4.6 | 0.5 |  |
| dip between legato notes (dB) | 7.7 | 4.3 | 10.5 | 10.9 | 10.9 | 0.9 |  |
| level evenness of a legato scale (dB) | 6.1 | 0.2 | 0.5 | 1.1 | 3.4 | 3.3 | yes |
| level over the keyboard (dB/oct) | -3.16 | -2.73 | -2.1 | 1.2 | 6.26 | 0.6 |  |
| level against the other instruments (velocity 96) (dB) | 5.1 | 5.1 | -0.1 | 2.9 | 2 | 1.0 |  |
| key-to-key level scatter (dB) | 5.74 | 0.18 | 0.39 | 0.17 | 1 | 10.9 | yes |
| odd / even partials (dB) | -8.9 | -1 | -0.4 | 7.1 | 5.4 | 2.8 | yes |
| onset jitter in a phrase (ms) | 52.4 | 14.6 | 44.4 | 16.2 | 33.4 | 1.9 | yes |
| onset in a phrase (re the note-on) (ms) | 68.0 | 11.0 | 61.0 | 10.0 | 24.0 | 2.0 | yes |
| onset sharpness (dB/ms) | 1.9 | 5.2 | 2.9 | 3.4 | 3.2 | 0.7 |  |
| overtone levels, note for note (dB rms) | 12.2 (nearest 10.7) | fonts among themselves 10.1 | | | | 1.2 |  |
| panning across the keys (dB/oct) | -1.71 | 0 | 0 | 0 | 0.04 | 1.7 | yes |
| partial envelopes alike | 0.07 | 0.094 | 0.257 | 0.071 | 0.172 | 0.6 |  |
| release time (30 dB after note-off) (ms) | 567 | 116 | 295 | 117 | 76.0 | 2.3 | yes |
| spectrogram (1 - correlation), note for note | 0.328 (nearest 0.251) | fonts among themselves 0.227 | | | | 1.4 |  |
| spectral shape of the body, note for note | 11.6 (nearest 8.48) | fonts among themselves 8.4 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 253 | 176 | 124 | 192 | 148 | 2.1 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.01 | 0.51 | 0.09 | -0.02 | 0.19 | 0.3 |  |
| level after 2 s held, re peak (dB) | -2.7 | -10.4 | -6.8 | -4.1 | -8.6 | 1.6 |  |
| high bands lag low bands by (ms) | -61.3 | -12.0 | 17.3 | 28.7 | -10.5 | 3.2 | yes |
| transient coherence: attack spread over bands (ms) | 14.2 | 4.5 | 6.8 | 27.8 | 11.8 | 0.4 |  |
| fundamental share (tristimulus 1) | 0.734 | 0.992 | 0.986 | 0.899 | 0.943 | 3.3 | yes |
| partials 2-4 share (tristimulus 2) | 0.26 | 0.007 | 0.012 | 0.084 | 0.057 | 3.2 | yes |
| partials 5+ share (tristimulus 3) | 0.002 | 0 | 0.002 | 0.006 | 0.006 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 16.0 | 16.6 | 16.6 | 16.7 | 20.1 | 0.4 |  |
| pitch wobble / vibrato (p-p) (c) | 3.4 | 14.5 | 19.7 | 7.8 | 28.4 | 1.3 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.019 (nearest 0.0185) | fonts among themselves 0.0128 | | | | 1.5 | yes |

</details>

### 79 Whistle: whistle

Distance 3.18 (rank 43 of 129); the fonts disagree 1.3 JND. Keys 60,63,66,69,72,75,78,81,84,87,90,93,96, velocities 40/64/96/127.

- **Uneven over the keys (level scatter 4.8 vs 0.05…1.4 dB; tone scatter 3.0 vs 0.1…0.4 ERB; −4.2 dB/oct; legato evenness 3.7 vs 0.3…2.3 dB)**: partly an ARTIFACT. The compass is D5–A6 (src/designs.cc), so keys 60–73 fold up and 96 down. The legato scale is uneven too, so this is a probable FAULT.
- **Quieter (0.1 vs 5.4…8.2 dB)**: FAULT, a trim of ~7 dB.
- **Noisy (flatness −30 vs −60…−72 dB; +13…+36 dB above 2.5 kHz)**: partly DELIBERATE: a fipple is breathy, and the fonts are near-sines. But nothing in README or research/ documents a target, and 30 dB is a lot.
- **Pitch scatter 18.9 vs 0…6.5 c**: tools/test.py has 19/20 notes in tune, so suspect the folds and the noise. Low confidence.
- **Release 388 ms**: room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key brightness scatter**: less even in tone from key to key (2.97 ERB | 0.167 | 0.117 | 0.21 | 0.422), outside the fonts, 13.9 x.
2. **pitch scatter from key to key**: less evenly tuned (18.9 c | 6.5 | 0.4 | 0.4 | 0), outside the fonts, 5.7 x.
3. **noisiness (spectral flatness)**: noisier (-29.6 dB | -59.9 | -71.5 | -67.2 | -67.0), outside the fonts, 6.5 x.
4. **key-to-key level scatter**: less even from key to key (2.58 dB | 0.42 | 0.12 | 0.05 | 1.41), outside the fonts, 3.4 x.
5. **brightness over the keyboard**: brightens slower up the keyboard (0.49 ERB/oct | 3.58 | 2.58 | 3.41 | 2.93), outside the fonts, 5.4 x.
6. **dip between legato notes**: smoother legato (4.3 dB | 13.2 | 14.1 | 14.4 | 13.4), outside the fonts, 4.7 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | - | -65 | -33 | -14 | +33 | +30 |
| re TimGM6mb | -44 | -56 | -20 | +6 | +23 | +36 |
| re Jnsgm2 | - | -60 | -24 | -8 | +13 | +32 |
| re SGM | -28 | -42 | -20 | -3 | +17 | +29 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 8.1 dB rms; TimGM6mb: centroid -2.9 ERB, attack -0.2 ERB, velocity brightening -0.7 ERB, loudness range +3 dB, envelope 1.7 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.1 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 1.8 dB rms
- silent probe notes: TimGM6mb/fs 24

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.28 | 0 | 0.01 | 0 | 0 | 0.7 | yes |
| attack time (-20 to -1 dB) (ms) | 106 | 139 | 128 | 136 | 45.0 | 0.4 |  |
| spectral shape of the attack, note for note | 16.3 (nearest 15.5) | fonts among themselves 12.8 | | | | 1.3 |  |
| brightness over the keyboard (ERB/oct) | 0.49 | 3.58 | 2.58 | 3.41 | 2.93 | 5.4 | yes |
| key-to-key brightness scatter (ERB) | 2.97 | 0.167 | 0.117 | 0.21 | 0.422 | 13.9 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.37 | -0 | 0 | 0 | -0.24 | 1.2 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 16.1 | 13.4 | 9.99 | 13.0 | 13.1 | 1.8 | yes |
| onset delay (ms) | 34.0 | 71.0 | 89.0 | 86.0 | 36.0 | 1.7 |  |
| Plomp-Levelt dissonance of a note (log2) | -5.32 | -3.06 | -1.13 | -6.16 | -6.38 | 0.3 |  |
| envelope curve, note for note (dB rms) | 15.1 (nearest 13.9) | fonts among themselves 9.21 | | | | 1.6 | yes |
| pitch re 12-TET A440 (median) (c) | 8.8 | 10.2 | 22.1 | -2.3 | 12.7 | 0.2 |  |
| pitch scatter from key to key (c) | 18.9 | 6.5 | 0.4 | 0.4 | 0 | 5.7 | yes |
| noisiness (spectral flatness) (dB) | -29.6 | -59.9 | -71.5 | -67.2 | -67.0 | 6.5 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -74.0 | -131 | -125 | -126 | -112 | 5.5 | yes |
| pitch at the start re settled (c) | 6.8 | 6.6 | 0.8 | 0 | 0.3 | 1.2 |  |
| stereo coherence | 0.93 | 0.997 | 1 | 1 | 1 | 0.9 | yes |
| partial-to-partial jaggedness (dB) | 7.2 | 3.3 | 1.1 | 4.5 | 2.9 | 2.4 | yes |
| dip between legato notes (dB) | 4.3 | 13.2 | 14.1 | 14.4 | 13.4 | 4.7 | yes |
| level evenness of a legato scale (dB) | 3.3 | 0.6 | 0.6 | 0.3 | 2.3 | 2.7 | yes |
| level over the keyboard (dB/oct) | -1.89 | -0.8 | -0.04 | 0.1 | 1.81 | 1.5 | yes |
| level against the other instruments (velocity 96) (dB) | 9.3 | 8.2 | 5.5 | 7.8 | 8.5 | 0.9 | yes |
| key-to-key level scatter (dB) | 2.58 | 0.42 | 0.12 | 0.05 | 1.41 | 3.4 | yes |
| odd / even partials (dB) | -9.4 | 1.2 | -9.5 | 12.4 | 1.7 | 1.0 |  |
| onset jitter in a phrase (ms) | 37.6 | 23.0 | 50.8 | 31.2 | 15.5 | 0.6 |  |
| onset in a phrase (re the note-on) (ms) | 25.0 | 2 | 0 | 30.0 | 4 | 1.5 |  |
| onset sharpness (dB/ms) | 3.8 | 1.2 | 5.5 | 0.8 | 2.4 | 0.9 |  |
| overtone levels, note for note (dB rms) | 24.6 (nearest 16.3) | fonts among themselves 15.6 | | | | 1.6 |  |
| panning across the keys (dB/oct) | 0.08 | 0 | 0 | 0 | 0.04 | 0.1 |  |
| partial envelopes alike | -0.021 | 0.117 | 0.388 | 0.766 | 0.864 | 1.6 | yes |
| release time (30 dB after note-off) (ms) | 381 | 177 | 112 | 147 | 64.0 | 2.1 | yes |
| spectrogram (1 - correlation), note for note | 0.722 (nearest 0.422) | fonts among themselves 0.407 | | | | 1.8 |  |
| spectral shape of the body, note for note | 17.2 (nearest 13.8) | fonts among themselves 14.4 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 296 | 250 | 153 | 197 | 142 | 1.9 | yes |
| level after 2 s held, re peak (dB) | -2.6 | -8.9 | -2.3 | -0.9 | -1.2 | 0.2 |  |
| high bands lag low bands by (ms) | -1 | - | -0.6 | 4 | -3 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 1.9 | - | 3.4 | 0.4 | 0.5 | 0.3 |  |
| fundamental share (tristimulus 1) | 0.792 | 1 | 0.974 | 0.999 | 1 | 3.0 | yes |
| partials 2-4 share (tristimulus 2) | 0.206 | 0 | 0.025 | 0.001 | 0 | 2.9 | yes |
| partials 5+ share (tristimulus 3) | 0.004 | 0 | 0 | 0 | 0 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 14.8 | 16.7 | 16.7 | 16.6 | 20.1 | 1.1 | yes |
| pitch wobble / vibrato (p-p) (c) | 4.2 | 31.4 | 16.5 | 0.5 | 12.8 | 0.7 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0233 (nearest 0.0208) | fonts among themselves 0.00949 | | | | 2.5 | yes |

</details>

### 80 Ocarina: ocarina

Distance 4.37 (rank 13 of 129); the fonts disagree 0.5 JND. Keys 60,63,66,69,72,75,78,81,84,87,90,93,96, velocities 40/64/96/127.

- **Key-to-key level scatter 8 dB vs 0.06-1.3; brightness scatter 1.8 ERB; pitch scatter 22 c vs 0.4-2.2**: partly MEASUREMENT ARTIFACT, since 6 of 13 probe keys lie outside A4-F6 and fold. The pitch scatter is a FAULT: the duct's "louder is sharper" is not held at velocity 96 (README Status only lists F5 -22 c).
- **Noisy and impure (flatness -18 dB vs -40..-80; fundamental share 0.61 vs ≥0.997)**: probable FAULT. An ocarina is close to a sine; ours is very breathy.
- **Quieter (-4.7 dB vs +2.8..+8.2)**: FAULT, the level trim.
- **Louder toward the bottom (-4.6 dB/oct)**: partly folding (ARTIFACT).
- **Release 279 ms**: the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (560 ms | 88.0 | 98.0 | 99.0 | 94.0), outside the fonts, 10.2 x.
2. **brightness (spectral centroid, velocity 96)**: brighter (14.6 ERB | 13.0 | 13.0 | 13.0 | 13.0), outside the fonts, 5.2 x.
3. **key-to-key brightness scatter**: less even in tone from key to key (2.3 ERB | 0.225 | 0.211 | 0.208 | 0.416), outside the fonts, 10.4 x.
4. **pitch scatter from key to key**: less evenly tuned (21.7 c | 2.2 | 1.2 | 0.4 | 1.9), outside the fonts, 6.7 x.
5. **key-to-key level scatter**: less even from key to key (3.79 dB | 0.68 | 0.06 | 0.06 | 1.29), outside the fonts, 5.6 x.
6. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 18.8, nearest 18.5; the fonts among themselves 2.82), outside.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | - | -49 | -2 | -9 | +9 | +19 |
| re TimGM6mb | - | -55 | -4 | -9 | +9 | +22 |
| re Jnsgm2 | - | -55 | -5 | -11 | +10 | +24 |
| re SGM | - | -58 | -8 | -15 | +9 | +32 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -10 -5 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 1.8 dB rms; TimGM6mb: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 2.2 dB rms; Jnsgm2: centroid -0.0 ERB, attack +0.0 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 2.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.51 | 0.36 | 0 | 0 | 0 | 1.3 |  |
| attack time (-20 to -1 dB) (ms) | 305 | 190 | 28.0 | 21.0 | 24.0 | 2.2 | yes |
| spectral shape of the attack, note for note | 14.8 (nearest 14.4) | fonts among themselves 6.88 | | | | 2.2 | yes |
| brightness over the keyboard (ERB/oct) | 1.65 | 2.66 | 3.4 | 3.4 | 2.88 | 4.0 | yes |
| key-to-key brightness scatter (ERB) | 2.3 | 0.225 | 0.211 | 0.208 | 0.416 | 10.4 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.43 | -0 | 0 | 0 | -0.23 | 1.4 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 14.6 | 13.0 | 13.0 | 13.0 | 13.0 | 5.2 | yes |
| onset delay (ms) | 63.0 | 11.0 | 15.0 | 15.0 | 19.0 | 4.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.92 | -4.21 | -6.16 | -6.16 | -8.97 | 0.9 | yes |
| envelope curve, note for note (dB rms) | 18.8 (nearest 18.5) | fonts among themselves 2.82 | | | | 6.6 | yes |
| pitch re 12-TET A440 (median) (c) | 9.4 | -0.3 | 1.5 | -2.3 | 1.2 | 2.2 | yes |
| pitch scatter from key to key (c) | 21.7 | 2.2 | 1.2 | 0.4 | 1.9 | 6.7 | yes |
| noisiness (spectral flatness) (dB) | -23.0 | -40.5 | -63.5 | -63.2 | -80.5 | 2.0 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -68.0 | -123 | -126 | -125 | -112 | 7.9 | yes |
| pitch at the start re settled (c) | 9.2 | 2.6 | -0.1 | 0 | 0.3 | 1.8 | yes |
| stereo coherence | 0.546 | 1 | 1 | 1 | 1 | 6.1 | yes |
| partial-to-partial jaggedness (dB) | 3.4 | 15.4 | 4.5 | 4.5 | 4.5 | 0.2 | yes |
| dip between legato notes (dB) | 4.2 | 10.7 | 11.0 | 11.7 | 11.8 | 3.6 | yes |
| level evenness of a legato scale (dB) | 4.3 | 2.1 | 1.8 | 1.7 | 3 | 2.3 | yes |
| level over the keyboard (dB/oct) | 0.08 | 3.36 | 0.43 | 0.41 | 3.54 | 1.2 |  |
| level against the other instruments (velocity 96) (dB) | 2.5 | 3.2 | 8.3 | 8.3 | 7.3 | 2.1 | yes |
| key-to-key level scatter (dB) | 3.79 | 0.68 | 0.06 | 0.06 | 1.29 | 5.6 | yes |
| odd / even partials (dB) | 1.7 | 23.5 | 12.3 | 12.3 | -5.2 | 0.7 |  |
| onset jitter in a phrase (ms) | 45.9 | 12.3 | 11.6 | 11.6 | 11.1 | 6.9 | yes |
| onset in a phrase (re the note-on) (ms) | 29.0 | 14.0 | 7 | 8 | 2 | 2.1 | yes |
| onset sharpness (dB/ms) | 4.1 | 7 | 3.6 | 3.9 | 5.7 | 0.4 |  |
| overtone levels, note for note (dB rms) | 24.4 (nearest 24) | fonts among themselves 7.99 | | | | 3.1 | yes |
| panning across the keys (dB/oct) | 0.36 | 0 | 0 | 0 | 0.04 | 0.4 |  |
| partial envelopes alike | -0.009 | 0.079 | 0.944 | 0.891 | 0.954 | 2.1 | yes |
| release time (30 dB after note-off) (ms) | 560 | 88.0 | 98.0 | 99.0 | 94.0 | 10.2 | yes |
| spectrogram (1 - correlation), note for note | 0.379 (nearest 0.364) | fonts among themselves 0.07 | | | | 5.4 | yes |
| spectral shape of the body, note for note | 13 (nearest 11.9) | fonts among themselves 6.47 | | | | 2.0 | yes |
| sounding length of a 100 ms note (ms) | 314 | 149 | 165 | 166 | 160 | 3.8 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.03 | -0.02 | - | - | 0.02 | 0.1 |  |
| level after 2 s held, re peak (dB) | -4.7 | -2.4 | -4 | -2.9 | -5.5 | 0.4 |  |
| high bands lag low bands by (ms) | -17.2 | 1.5 | 0.5 | 1 | 0 | 3.6 | yes |
| transient coherence: attack spread over bands (ms) | 6.5 | 2.7 | 0.4 | 0.2 | 0.1 | 1.2 | yes |
| fundamental share (tristimulus 1) | 0.802 | 0.998 | 0.999 | 0.999 | 0.997 | 2.8 | yes |
| partials 2-4 share (tristimulus 2) | 0.075 | 0.002 | 0.001 | 0.001 | 0.003 | 1.0 | yes |
| partials 5+ share (tristimulus 3) | 0.009 | 0 | 0 | 0 | 0 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 12.7 | 16.6 | 16.6 | 16.6 | 20.0 | 2.3 | yes |
| pitch wobble / vibrato (p-p) (c) | 10.4 | 12.5 | 0.5 | 0.5 | 1.4 | 1.6 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.018 (nearest 0.0168) | fonts among themselves 0.00676 | | | | 2.7 | yes |

</details>

### 81 Lead 1 (square): lead-square

Distance 3.77 (rank 26 of 129); the fonts disagree 0.8 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Not a square (odd/even −2 dB vs +50…+60; fundamental share 0.74 vs 0.86…0.92)**: DELIBERATE pulse-width sweep (README "Synth leads", pad table: threshold swept 35–65 % at 0.6 Hz), but averaged over the sweep the even partials are as strong as the odd ones. synth_leads.md §4 asked for "hollow, odd harmonics", so the default sweep is too wide or off-centre for GM 81. Possible FAULT.
- **Soft front (attack 44 ms, phrase onset 36 ms vs 1…3 / −3…8)**: a lead should snap. Partly the room's build-up (notes_a p002), but likely FAULT in the default "hardness".
- **Quieter (−0.4 vs +3.3…+5.3 dB), darker above 6 kHz (−11…−19 dB)**: a trim; the ladder's default cutoff.
- **Key scatter 2.8 vs 0.4…1.1 dB; release 588 ms**: room ARTIFACT (see p102).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (588 ms | 81.0 | 113 | 81.0 | 73.0), outside the fonts, 9.1 x.
2. **attack time (-20 to -1 dB)**: a slower attack (44.5 ms | 1 | 1 | 1 | 3), outside the fonts, 6.9 x.
3. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.671, nearest 0.582; the fonts among themselves 0.0736), outside.
4. **key-to-key level scatter**: less even from key to key (2.73 dB | 0.36 | 0.46 | 0.51 | 1.07), outside the fonts, 4.5 x.
5. **odd / even partials**: more even partials (-2.2 dB | 50.2 | 52.0 | 51.8 | 59.9), outside the fonts, 11.2 x.
6. **brightening from velocity 40 to 127**: brightens more with velocity (1.12 ERB | -0 | 0 | -0 | -0.31), outside the fonts, 3.7 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +6 | +8 | +7 | +7 | -3 | -15 |
| re TimGM6mb | +6 | +10 | +9 | +8 | -3 | -19 |
| re Jnsgm2 | +6 | +10 | +10 | +8 | -3 | -19 |
| re SGM | +3 | +6 | +5 | +5 | -3 | -11 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.5 ERB, attack +0.3 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 10.0 dB rms; TimGM6mb: centroid +1.5 ERB, attack +0.5 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 6.5 dB rms; Jnsgm2: centroid +2.2 ERB, attack +0.6 ERB, velocity brightening -0.5 ERB, loudness range +4 dB, envelope 6.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.48 | -1.12 | -0.58 | -0.59 | 0.09 | 0.2 |  |
| attack time (-20 to -1 dB) (ms) | 44.5 | 1 | 1 | 1 | 3 | 6.9 | yes |
| spectral shape of the attack, note for note | 19 (nearest 17) | fonts among themselves 5.05 | | | | 3.8 | yes |
| brightness over the keyboard (ERB/oct) | 3.5 | 3.6 | 4.07 | 4.15 | 3.68 | 1.2 |  |
| key-to-key brightness scatter (ERB) | 1.4 | 0.97 | 1.32 | 1.18 | 0.622 | 0.9 |  |
| brightening from velocity 40 to 127 (ERB) | 1.12 | -0 | 0 | -0 | -0.31 | 3.7 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 8.55 | 10.4 | 9.82 | 9.16 | 8.45 | 1.0 |  |
| onset delay (ms) | 17.0 | 0 | 0 | 0 | 2 | 1.7 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.97 | -3.64 | -4.76 | -4.47 | -4.8 | 1.1 |  |
| envelope curve, note for note (dB rms) | 18.6 (nearest 18.1) | fonts among themselves 9.54 | | | | 2.0 | yes |
| pitch re 12-TET A440 (median) (c) | 0.1 | 4.3 | 0.3 | 2.5 | 4.1 | 0.8 |  |
| pitch scatter from key to key (c) | 1.2 | 1 | 2.1 | 3 | 0.1 | 0.1 |  |
| noisiness (spectral flatness) (dB) | -57.5 | -69.1 | -67.4 | -64.8 | -62.3 | 2.5 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -68.6 | -123 | -124 | -123 | -110 | 7.5 | yes |
| pitch at the start re settled (c) | -0.1 | -0.1 | 0 | 0 | 0.2 | 0.0 |  |
| stereo coherence | 0.611 | 0.246 | 1 | 1 | 1 | 1.0 |  |
| partial-to-partial jaggedness (dB) | 3.3 | 32.7 | 34.6 | 34.8 | 40.6 | 7.9 | yes |
| dip between legato notes (dB) | 11.4 | 29.4 | 14.8 | 17.9 | 14.1 | 0.6 | yes |
| level evenness of a legato scale (dB) | 2.4 | 1.6 | 2.1 | 2 | 3.2 | 0.4 |  |
| level over the keyboard (dB/oct) | -0.01 | -0.54 | -0.55 | -0.98 | -0.92 | 0.7 | yes |
| level against the other instruments (velocity 96) (dB) | 3.8 | 3.4 | 5.5 | 4.9 | 5.5 | 1.3 |  |
| key-to-key level scatter (dB) | 2.73 | 0.36 | 0.46 | 0.51 | 1.07 | 4.5 | yes |
| odd / even partials (dB) | -2.2 | 50.2 | 52.0 | 51.8 | 59.9 | 11.2 | yes |
| onset jitter in a phrase (ms) | 50.7 | 44.8 | 28.9 | 30.8 | 24.5 | 2.1 | yes |
| onset in a phrase (re the note-on) (ms) | 30.0 | -1 | -2 | -3 | 8 | 3.1 | yes |
| onset sharpness (dB/ms) | 11.1 | 0.7 | 1.3 | 1.3 | 49.6 | 0.4 |  |
| overtone levels, note for note (dB rms) | 28.2 (nearest 27.7) | fonts among themselves 3.77 | | | | 7.5 | yes |
| panning across the keys (dB/oct) | -0.3 | -0.01 | -0.01 | -0.02 | -0.01 | 0.3 |  |
| partial envelopes alike | -0.027 | 0.107 | -0.002 | -0.038 | 0.09 | 0.7 |  |
| release time (30 dB after note-off) (ms) | 588 | 81.0 | 113 | 81.0 | 73.0 | 9.1 | yes |
| spectrogram (1 - correlation), note for note | 0.671 (nearest 0.582) | fonts among themselves 0.0736 | | | | 9.1 | yes |
| spectral shape of the body, note for note | 21.5 (nearest 18) | fonts among themselves 5.41 | | | | 4.0 | yes |
| sounding length of a 100 ms note (ms) | 400 | 145 | 167 | 176 | 139 | 5.4 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.03 | 0.03 | -0.15 | -0.02 | -0.16 | 0.1 |  |
| level after 2 s held, re peak (dB) | -6.5 | -2.2 | -7.1 | -5.8 | -5 | 0.4 |  |
| high bands lag low bands by (ms) | 2.5 | 0 | 0 | 0 | 2.5 | 0.5 |  |
| transient coherence: attack spread over bands (ms) | 3.7 | 0 | 0 | 0 | 1 | 0.7 | yes |
| fundamental share (tristimulus 1) | 0.74 | 0.856 | 0.917 | 0.904 | 0.888 | 2.2 | yes |
| partials 2-4 share (tristimulus 2) | 0.174 | 0.097 | 0.045 | 0.051 | 0.068 | 1.6 | yes |
| partials 5+ share (tristimulus 3) | 0.022 | 0.053 | 0.038 | 0.042 | 0.044 | 0.3 |  |
| loudness range, velocity 40 to 127 (dB) | 15.6 | 16.7 | 16.7 | 16.6 | 20.0 | 0.6 | yes |
| pitch wobble / vibrato (p-p) (c) | 4.6 | 15.9 | 10.7 | 10.3 | 12.3 | 1.4 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0298 (nearest 0.0277) | fonts among themselves 0.0121 | | | | 2.5 | yes |

</details>

### 82 Lead 2 (sawtooth): lead-saw

Distance 2.37 (rank 87 of 129); the fonts disagree 0.7 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Brightens with velocity (+2.1 vs 0 ERB)**: DELIBERATE: the dynamics knob drives level and cutoff (README "Synth leads", Controls). FluidSynth renders of the fonts don't brighten either, so this is a real difference, but the analog one.
- **Darker above 6 kHz (−22…−30 dB) with the same overall centroid (10.7 vs 10.6…12.0 ERB)**: the ladder. TimGM6mb and Jnsgm2 are ~2 ERB darker under FluidSynth (their filter envelopes), so the gap is overstated.
- **Phrase onset 30 ms vs −4…21**: the slur/glide logic or the room. Check before calling it a fault.
- **Key scatter 2.0 vs 0.3…0.6 dB, release 586 ms, floor −69 dB**: room ARTIFACT plus the per-voice board.
- Rank 73: close overall.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (2.1 ERB | 0 | -0 | -0 | -0.09), outside the fonts, 7.0 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (586 ms | 81.0 | 94.0 | 120 | 63.0), outside the fonts, 5.9 x.
3. **onset in a phrase (re the note-on)**: speaks later in a phrase (44.0 ms | 21.0 | -4 | -2 | 0), outside the fonts, 3.6 x.
4. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.532, nearest 0.501; the fonts among themselves 0.135), outside.
5. **key-to-key level scatter**: less even from key to key (1.6 dB | 0.34 | 0.41 | 0.43 | 0.63), outside the fonts, 2.4 x.
6. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-69.3 dB | -122 | -126 | -125 | -110), outside the fonts, 6.8 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +16 | +11 | +13 | +12 | -4 | -28 |
| re TimGM6mb | +8 | +9 | +9 | +9 | -6 | -22 |
| re Jnsgm2 | +11 | +12 | +12 | +12 | -3 | -27 |
| re SGM | +14 | +13 | +14 | +13 | -3 | -30 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.3 ERB, attack +0.0 ERB, velocity brightening +0.2 ERB, loudness range +3 dB, envelope 2.3 dB rms; TimGM6mb: centroid -2.2 ERB, attack +0.5 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 3.6 dB rms; Jnsgm2: centroid -1.8 ERB, attack +0.3 ERB, velocity brightening +0.4 ERB, loudness range +4 dB, envelope 3.2 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.09 | 2.83 | -1.15 | -1.03 | -0.62 | 0.4 |  |
| attack time (-20 to -1 dB) (ms) | 31.5 | 420 | 1 | 1 | 5 | 0.9 |  |
| spectral shape of the attack, note for note | 13.8 (nearest 12.7) | fonts among themselves 7.97 | | | | 1.7 | yes |
| brightness over the keyboard (ERB/oct) | 3.7 | 3.25 | 3.93 | 3.73 | 4.15 | 0.3 |  |
| key-to-key brightness scatter (ERB) | 1.43 | 0.518 | 0.724 | 0.896 | 0.995 | 2.6 | yes |
| brightening from velocity 40 to 127 (ERB) | 2.1 | 0 | -0 | -0 | -0.09 | 7.0 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 10.7 | 10.6 | 12.0 | 11.2 | 11.7 | 1.0 |  |
| onset delay (ms) | 14.0 | 6 | 0 | 0 | 5 | 1.1 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.27 | -3.41 | -3 | -2.84 | -2.17 | 2.2 | yes |
| envelope curve, note for note (dB rms) | 16.7 (nearest 15.9) | fonts among themselves 5 | | | | 3.3 | yes |
| pitch re 12-TET A440 (median) (c) | -0.1 | 4.1 | 0.4 | 1.1 | 5.5 | 0.7 |  |
| pitch scatter from key to key (c) | 0.3 | 2.2 | 2.4 | 3.3 | 2.5 | 0.7 | yes |
| noisiness (spectral flatness) (dB) | -61.5 | -56.7 | -55.9 | -59.2 | -39.9 | 0.5 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -69.3 | -122 | -126 | -125 | -110 | 6.8 | yes |
| pitch at the start re settled (c) | -0.9 | -0.2 | 0.3 | 0.2 | 1.1 | 0.2 |  |
| stereo coherence | 0.804 | 1 | 1 | 1 | 1 | 2.6 | yes |
| partial-to-partial jaggedness (dB) | 3.4 | 0.8 | 0.3 | 0.3 | 0.2 | 3.1 | yes |
| dip between legato notes (dB) | 10.4 | 7.9 | 7.4 | 6.2 | 8.2 | 1.4 | yes |
| level evenness of a legato scale (dB) | 0.5 | 1.4 | 0.7 | 0.9 | 2.1 | 0.6 |  |
| level over the keyboard (dB/oct) | -0.4 | 0.69 | -0.98 | -0.87 | -1.13 | 0.5 |  |
| level against the other instruments (velocity 96) (dB) | 3.4 | -0.1 | 3.1 | 3.8 | 0.2 | 0.9 |  |
| key-to-key level scatter (dB) | 1.6 | 0.34 | 0.41 | 0.43 | 0.63 | 2.4 | yes |
| odd / even partials (dB) | -3.5 | -2.1 | -2.8 | -2.9 | -2.2 | 0.5 |  |
| onset jitter in a phrase (ms) | 49.7 | 38.6 | 25.7 | 56.2 | 37.8 | 0.8 |  |
| onset in a phrase (re the note-on) (ms) | 44.0 | 21.0 | -4 | -2 | 0 | 3.6 | yes |
| onset sharpness (dB/ms) | 14.1 | 5.8 | 1 | 0.9 | 19.8 | 1.1 |  |
| overtone levels, note for note (dB rms) | 8.54 (nearest 7.8) | fonts among themselves 2.25 | | | | 3.8 | yes |
| panning across the keys (dB/oct) | -0.27 | 0 | 0 | 0 | -0.01 | 0.3 |  |
| partial envelopes alike | 0.102 | -0.004 | -0.055 | -0.063 | -0.023 | 1.4 | yes |
| release time (30 dB after note-off) (ms) | 586 | 81.0 | 94.0 | 120 | 63.0 | 5.9 | yes |
| spectrogram (1 - correlation), note for note | 0.532 (nearest 0.501) | fonts among themselves 0.135 | | | | 3.9 | yes |
| spectral shape of the body, note for note | 15.6 (nearest 12.2) | fonts among themselves 9.5 | | | | 1.6 |  |
| sounding length of a 100 ms note (ms) | 188 | 128 | 118 | 130 | 7 | 0.3 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.03 | 0.01 | -0.09 | 0.43 | -0.05 | 0.0 |  |
| level after 2 s held, re peak (dB) | -6.5 | -7.3 | -6.8 | -6.7 | -10.8 | 0.2 |  |
| high bands lag low bands by (ms) | 0.8 | -4 | 0 | 0 | 0 | 0.2 |  |
| transient coherence: attack spread over bands (ms) | 1 | 4.9 | 0 | 0 | 2.8 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.625 | 0.617 | 0.721 | 0.705 | 0.609 | 0.5 |  |
| partials 2-4 share (tristimulus 2) | 0.297 | 0.285 | 0.221 | 0.228 | 0.26 | 0.8 |  |
| partials 5+ share (tristimulus 3) | 0.015 | 0.104 | 0.06 | 0.067 | 0.131 | 1.0 | yes |
| loudness range, velocity 40 to 127 (dB) | 15.3 | 16.7 | 16.7 | 16.6 | 20.1 | 0.8 | yes |
| pitch wobble / vibrato (p-p) (c) | 2.2 | 3.4 | 0.7 | 0.4 | 1.3 | 0.2 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0228 (nearest 0.0216) | fonts among themselves 0.0117 | | | | 2.0 | yes |

</details>

### 83 Lead 3 (calliope): lead-calliope

Distance 2.80 (rank 63 of 129); the fonts disagree 1.3 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Purer and hollower (fundamental share 0.98 vs 0.69…0.84; odd/even +24 vs −4…+7 dB)**: DELIBERATE near-sine from a triangle into a differential pair (synth_leads.md §10 table).
- **Not breathy (flatness −63 vs −34…−47 dB)**: FAULT against our own spec. synth_leads.md §4 asks for "near-sine with noise", but the pitched breath is far below the fonts' chiff.
- **Key scatter (3.8 vs 0.6…1.4 dB)**: a near-sine in the hall cancels or adds key by key (README "FX 7": 3.9 dB in the hall, 0.5 dry). Room ARTIFACT.
- **Phrase onset 52 ms vs −2…14; staccato 296 ms**: possible slow speech, plus the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (3.74 dB | 1.44 | 0.62 | 0.88 | 0.69), outside the fonts, 5.9 x.
2. **Plomp-Levelt dissonance of a note**: smoother (-6.8 log2 | -3.34 | -2.83 | -2.68 | -3.19), outside the fonts, 11.5 x.
3. **partial-to-partial jaggedness**: a more jagged overtone series (11.2 dB | 1.1 | 2.5 | 1.7 | 3.5), outside the fonts, 7.6 x.
4. **sounding length of a 100 ms note [room]**: longer staccato (296 ms | 147 | 119 | 131 | 133), outside the fonts, 4.7 x.
5. **onset in a phrase (re the note-on)**: speaks later in a phrase (48.0 ms | 3 | 14.0 | -2 | 6), outside the fonts, 4.3 x.
6. **odd / even partials**: more odd partials (hollower) (24.1 dB | -3.9 | 7.2 | -1.3 | 5.8), outside the fonts, 3.9 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +8 | +4 | +5 | -1 | -6 | -21 |
| re TimGM6mb | +12 | +9 | +6 | -2 | -8 | -15 |
| re Jnsgm2 | +12 | +8 | +6 | -0 | -4 | -16 |
| re SGM | +11 | +8 | +8 | -2 | -6 | -17 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +4; FluidR3 -18 -12 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack -0.1 ERB, velocity brightening +0.7 ERB, loudness range +4 dB, envelope 7.1 dB rms; TimGM6mb: centroid -0.6 ERB, attack -0.2 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 3.0 dB rms; Jnsgm2: centroid -0.5 ERB, attack +0.2 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 3.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.2 | -0.07 | 0.88 | 1.08 | -0.97 | 0.6 |  |
| attack time (-20 to -1 dB) (ms) | 114 | 21.5 | 473 | 86.0 | 359 | 0.3 |  |
| spectral shape of the attack, note for note | 11.6 (nearest 9.1) | fonts among themselves 8.73 | | | | 1.3 |  |
| brightness over the keyboard (ERB/oct) | 3.5 | 3.54 | 3.01 | 2.95 | 3.53 | 0.8 |  |
| key-to-key brightness scatter (ERB) | 0.507 | 0.558 | 0.634 | 0.482 | 0.687 | 0.4 |  |
| brightening from velocity 40 to 127 (ERB) | -0.03 | 0.27 | -0 | 0 | -0.28 | 0.1 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 8.04 | 8 | 10.7 | 8.59 | 8.85 | 0.5 |  |
| onset delay (ms) | 34.0 | 18.0 | 34.0 | 27.0 | 13.0 | 1.1 |  |
| Plomp-Levelt dissonance of a note (log2) | -6.8 | -3.34 | -2.83 | -2.68 | -3.19 | 11.5 | yes |
| envelope curve, note for note (dB rms) | 17 (nearest 9.76) | fonts among themselves 11.7 | | | | 1.5 |  |
| pitch re 12-TET A440 (median) (c) | 0.8 | 15.6 | 1.5 | 2.1 | 4.9 | 0.4 |  |
| pitch scatter from key to key (c) | 2.3 | 11.0 | 2.1 | 2.4 | 3.9 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -63.3 | -47.3 | -46.4 | -37.8 | -34.4 | 3.3 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -68.0 | -64.4 | -126 | -125 | -112 | 1.6 |  |
| pitch at the start re settled (c) | -0.4 | -15.6 | -3.5 | -3.1 | -0.2 | 0.4 |  |
| stereo coherence | 0.677 | 0.819 | 1 | 1 | 1 | 3.6 | yes |
| partial-to-partial jaggedness (dB) | 11.2 | 1.1 | 2.5 | 1.7 | 3.5 | 7.6 | yes |
| dip between legato notes (dB) | 11.4 | 8.9 | 10.2 | 8.1 | 9.1 | 1.2 | yes |
| level evenness of a legato scale (dB) | 3.1 | 1.1 | 0.8 | 0.3 | 2.6 | 1.9 |  |
| level over the keyboard (dB/oct) | 0.23 | -1.95 | 1.35 | -0.07 | 0.64 | 0.0 |  |
| level against the other instruments (velocity 96) (dB) | 1 | -0.8 | 1.7 | 2.6 | 3.8 | 0.5 |  |
| key-to-key level scatter (dB) | 3.74 | 1.44 | 0.62 | 0.88 | 0.69 | 5.9 | yes |
| odd / even partials (dB) | 24.1 | -3.9 | 7.2 | -1.3 | 5.8 | 3.9 | yes |
| onset jitter in a phrase (ms) | 36.1 | 46.5 | 65.1 | 26.0 | 27.3 | 0.0 |  |
| onset in a phrase (re the note-on) (ms) | 48.0 | 3 | 14.0 | -2 | 6 | 4.3 | yes |
| onset sharpness (dB/ms) | 4.4 | 6 | 6 | 5.9 | 13.4 | 0.4 | yes |
| overtone levels, note for note (dB rms) | 22.1 (nearest 19.9) | fonts among themselves 5.94 | | | | 3.7 | yes |
| panning across the keys (dB/oct) | 0.01 | -2.53 | 0.05 | 0.05 | -0.01 | 0.0 |  |
| partial envelopes alike | 0.078 | 0.111 | 0.004 | 0.011 | -0.006 | 0.7 |  |
| release time (30 dB after note-off) (ms) | 556 | 640 | 65.0 | 80.0 | 151 | 1.4 |  |
| spectrogram (1 - correlation), note for note | 0.368 (nearest 0.193) | fonts among themselves 0.155 | | | | 2.4 |  |
| spectral shape of the body, note for note | 11 (nearest 9.98) | fonts among themselves 6.27 | | | | 1.7 | yes |
| sounding length of a 100 ms note (ms) | 296 | 147 | 119 | 131 | 133 | 4.7 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.03 | -2.79 | -0.09 | 0.18 | 0.34 | 0.0 |  |
| level after 2 s held, re peak (dB) | -5.8 | -11.8 | -5.6 | -10.5 | -7 | 1.0 |  |
| high bands lag low bands by (ms) | 1.5 | -4 | -2 | -0.7 | 11.3 | 0.4 |  |
| transient coherence: attack spread over bands (ms) | 2.1 | 1.2 | 4.3 | 3.5 | 10.4 | 0.4 |  |
| fundamental share (tristimulus 1) | 0.978 | 0.837 | 0.698 | 0.843 | 0.693 | 2.8 | yes |
| partials 2-4 share (tristimulus 2) | 0.019 | 0.162 | 0.272 | 0.135 | 0.283 | 2.7 | yes |
| partials 5+ share (tristimulus 3) | 0.001 | 0.003 | 0.035 | 0.01 | 0.033 | 0.3 |  |
| loudness range, velocity 40 to 127 (dB) | 16.8 | 22.5 | 16.7 | 16.6 | 20.0 | 0.5 |  |
| pitch wobble / vibrato (p-p) (c) | 8.4 | 36.7 | 17.1 | 32.9 | 13.4 | 1.4 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0198 (nearest 0.0138) | fonts among themselves 0.0149 | | | | 1.3 |  |

</details>

### 84 Lead 4 (chiff): lead-chiff

Distance 1.59 (rank 119 of 129); the fonts disagree 1.3 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Near-sine body (fundamental share 0.95 vs 0.73…0.79; −7…−9 dB at 1–2.5 kHz)**: DELIBERATE: "the sine with a little pulse" (synth_leads.md §10). It is mellower than every font, so maybe a bit more pulse.
- **Faster attack (25 vs 38…99 ms) and brighter attack (+1.1 ERB)**: the chiff resonator works as planned.
- **Key scatter (3.2 vs 0.2…1.5 dB)**: near-sine in the room (see p082). ARTIFACT.
- **Release 466 ms, floor −72 dB**: room ARTIFACT. FluidR3 brightens +2.0 ERB with velocity under FluidSynth, so our +0.36 isn't out of line.
- Rank 89: one of the closer leads.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (466 ms | 63.0 | 64.0 | 80.0 | 62.0), outside the fonts, 11.5 x.
2. **key-to-key level scatter**: less even from key to key (2.57 dB | 1.5 | 0.85 | 0.24 | 0.45), outside the fonts, 3.0 x.
3. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-71.6 dB | -129 | -124 | -124 | -112), outside the fonts, 6.4 x.
4. **fundamental share (tristimulus 1)**: more fundamental (0.952 | 0.726 | 0.733 | 0.787 | 0.74), outside the fonts, 3.1 x.
5. **attack time (-20 to -1 dB)**: a faster attack (25.0 ms | 38.5 | 99.0 | 72.0 | 57.0), outside the fonts, 2.0 x.
6. **partial envelopes alike**: partials moving more as one (0.434 | 0.168 | -0.031 | 0.034 | 0.107), outside the fonts, 3.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +2 | -1 | +2 | -9 | +3 | +11 |
| re TimGM6mb | +8 | +4 | +6 | -7 | -3 | -1 |
| re Jnsgm2 | +5 | +4 | +5 | -7 | +2 | -3 |
| re SGM | +3 | +1 | +1 | -9 | +4 | +8 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +4; FluidR3 -16 -10 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack -0.2 ERB, velocity brightening +2.0 ERB, loudness range +6 dB, envelope 5.1 dB rms; TimGM6mb: centroid +1.3 ERB, attack +0.1 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 2.3 dB rms; Jnsgm2: centroid -1.8 ERB, attack +0.5 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 6.2 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 1.09 | -0.12 | -0.51 | 2.42 | 0.33 | 0.7 |  |
| attack time (-20 to -1 dB) (ms) | 25.0 | 38.5 | 99.0 | 72.0 | 57.0 | 2.0 | yes |
| spectral shape of the attack, note for note | 10.1 (nearest 9.07) | fonts among themselves 9 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | 3.39 | 3.74 | 3.85 | 3.91 | 3.18 | 1.1 |  |
| key-to-key brightness scatter (ERB) | 0.559 | 0.668 | 1.12 | 0.511 | 0.562 | 0.2 |  |
| brightening from velocity 40 to 127 (ERB) | 0.36 | -0.68 | -0 | 0 | -0.21 | 1.4 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 8.22 | 7.51 | 8.75 | 10.1 | 9.76 | 0.8 |  |
| onset delay (ms) | 16.0 | 8 | 10.0 | 35.0 | 15.0 | 0.3 |  |
| Plomp-Levelt dissonance of a note (log2) | -3.89 | -4.24 | -3.28 | -3.66 | -4.06 | 0.1 |  |
| envelope curve, note for note (dB rms) | 17.3 (nearest 17) | fonts among themselves 6.68 | | | | 2.6 | yes |
| pitch re 12-TET A440 (median) (c) | 0.1 | 0.1 | 3.6 | -0.4 | 2.7 | 0.3 |  |
| pitch scatter from key to key (c) | 0.4 | 3.6 | 2.7 | 5 | 5.2 | 1.3 | yes |
| noisiness (spectral flatness) (dB) | -40.7 | -56.7 | -44.8 | -36.1 | -41.8 | 0.3 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -71.6 | -129 | -124 | -124 | -112 | 6.4 | yes |
| pitch at the start re settled (c) | -0.3 | -13.5 | -11.3 | 1.4 | -7.9 | 1.2 |  |
| stereo coherence | 0.762 | 0.532 | 1 | 1 | 1 | 1.0 |  |
| partial-to-partial jaggedness (dB) | 2.85 | 1.45 | 1.6 | 1.7 | 1.2 | 1.3 | yes |
| dip between legato notes (dB) | 11.9 | 6.1 | 9.8 | 12.8 | 7.7 | 0.9 |  |
| level evenness of a legato scale (dB) | 2.4 | 0.7 | 0.8 | 0.5 | 2.3 | 1.6 |  |
| level over the keyboard (dB/oct) | 0.01 | -0.52 | 0.5 | -1.34 | 0.33 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | 2.1 | 4.1 | 2.7 | -0.6 | 3.3 | 0.4 |  |
| key-to-key level scatter (dB) | 2.57 | 1.5 | 0.85 | 0.24 | 0.45 | 3.0 | yes |
| odd / even partials (dB) | -2.5 | -5.3 | 0.4 | 3 | -2.8 | 0.3 |  |
| onset jitter in a phrase (ms) | 24.3 | 57.9 | 42.9 | 40.9 | 39.8 | 1.9 | yes |
| onset in a phrase (re the note-on) (ms) | 26.0 | 8 | -1 | 11.0 | 4 | 2.0 | yes |
| onset sharpness (dB/ms) | 11.1 | 8.6 | 8.7 | 7.8 | 11.2 | 1.2 |  |
| overtone levels, note for note (dB rms) | 13.1 (nearest 11.2) | fonts among themselves 5.82 | | | | 2.3 | yes |
| panning across the keys (dB/oct) | -0.27 | 1.2 | 0 | 0 | -0.01 | 0.3 |  |
| partial envelopes alike | 0.434 | 0.168 | -0.031 | 0.034 | 0.107 | 3.6 | yes |
| release time (30 dB after note-off) (ms) | 466 | 63.0 | 64.0 | 80.0 | 62.0 | 11.5 | yes |
| spectrogram (1 - correlation), note for note | 0.324 (nearest 0.227) | fonts among themselves 0.167 | | | | 1.9 | yes |
| spectral shape of the body, note for note | 9.9 (nearest 9.11) | fonts among themselves 8.66 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 198 | 123 | 26.0 | 115 | 95.0 | 0.8 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.02 | 0.11 | -0.36 | -0.08 | 0.39 | 0.1 |  |
| level after 2 s held, re peak (dB) | -7.2 | -15.4 | -6.9 | -15.2 | -11.0 | 1.4 |  |
| high bands lag low bands by (ms) | -2.7 | 7.2 | 3.3 | -19.7 | 1 | 0.4 |  |
| transient coherence: attack spread over bands (ms) | 1.4 | 4.8 | 7 | 11.6 | 6.9 | 1.1 | yes |
| fundamental share (tristimulus 1) | 0.952 | 0.726 | 0.733 | 0.787 | 0.74 | 3.1 | yes |
| partials 2-4 share (tristimulus 2) | 0.042 | 0.263 | 0.174 | 0.207 | 0.17 | 2.1 | yes |
| partials 5+ share (tristimulus 3) | 0.002 | 0.013 | 0.055 | 0.019 | 0.038 | 0.4 |  |
| loudness range, velocity 40 to 127 (dB) | 16.2 | 20.6 | 16.7 | 16.7 | 20.1 | 1.1 |  |
| pitch wobble / vibrato (p-p) (c) | 5.3 | 27.8 | 15.5 | 11.0 | 16.8 | 1.3 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0145 (nearest 0.0098) | fonts among themselves 0.012 | | | | 1.2 |  |

</details>

### 85 Lead 5 (charang): lead-charang

Distance 2.67 (rank 71 of 129); the fonts disagree 1.2 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Lacks the fuzz (centroid 11.5 vs 14.4…18.3 ERB; −17…−25 dB above 6 kHz; partials 5+ share 0.01 vs 0.09…0.36)**: likely FAULT. An overdriven guitar-like lead (synth_leads.md §4) should be brighter than a saw, and this is darker. FluidR3 is 4 ERB darker under FluidSynth, but the other three are not. Raise the cutoff after the diode stage or weaken the 650 Hz scoop.
- **Brightens with velocity (+1.7 ERB)**: DELIBERATE (the dynamics knob).
- **Phrase onset 50 ms vs −2…1**: as on the other leads. Check the slur.
- **Release 619 ms, key scatter 2.4 dB**: room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (619 ms | 66.0 | 52.0 | 91.0 | 54.0), outside the fonts, 8.4 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (1.7 ERB | -0 | 0 | 0 | 0.18), outside the fonts, 5.7 x.
3. **brightness (spectral centroid, velocity 96)**: darker (11.5 ERB | 18.3 | 15.8 | 14.4 | 17.8), outside the fonts, 2.7 x.
4. **onset in a phrase (re the note-on)**: speaks later in a phrase (44.0 ms | -2 | 1 | -0 | -2), outside the fonts, 4.5 x.
5. **key-to-key level scatter**: less even from key to key (2.13 dB | 0.62 | 0.65 | 1.04 | 0.51), outside the fonts, 3.0 x.
6. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.631, nearest 0.518; the fonts among themselves 0.145), outside.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +5 | +11 | +11 | +12 | -11 | -17 |
| re TimGM6mb | +16 | +14 | +11 | +9 | -5 | -25 |
| re Jnsgm2 | +20 | +12 | +8 | +8 | -6 | -22 |
| re SGM | +13 | +13 | +9 | +4 | -6 | -18 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -4.0 ERB, attack +1.9 ERB, velocity brightening -0.0 ERB, loudness range +3 dB, envelope 5.4 dB rms; TimGM6mb: centroid +0.0 ERB, attack +0.1 ERB, velocity brightening +0.1 ERB, loudness range +3 dB, envelope 3.6 dB rms; Jnsgm2: centroid -0.3 ERB, attack -0.2 ERB, velocity brightening +0.2 ERB, loudness range +3 dB, envelope 2.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.09 | -1.12 | 0.34 | 1.82 | -0.14 | 0.0 |  |
| attack time (-20 to -1 dB) (ms) | 57.0 | 7 | 69.0 | 28.0 | 31.0 | 0.6 |  |
| spectral shape of the attack, note for note | 13.6 (nearest 12.4) | fonts among themselves 8.52 | | | | 1.6 | yes |
| brightness over the keyboard (ERB/oct) | 4.19 | 4.86 | 3.27 | 1.74 | 3.24 | 0.6 |  |
| key-to-key brightness scatter (ERB) | 1.25 | 1.69 | 0.941 | 1.47 | 0.872 | 0.1 |  |
| brightening from velocity 40 to 127 (ERB) | 1.7 | -0 | 0 | 0 | 0.18 | 5.7 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 11.5 | 18.3 | 15.8 | 14.4 | 17.8 | 2.7 | yes |
| decay rate of a held note (dB/s) | 0.9 | 0.7 | 0.7 | 4.9 | 2 | 0.3 |  |
| onset delay (ms) | 16.0 | 2 | 3 | 4 | 4 | 1.2 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.31 | -1.82 | -2.74 | -2.87 | -2.85 | 2.9 | yes |
| envelope curve, note for note (dB rms) | 18.7 (nearest 18.5) | fonts among themselves 5.06 | | | | 3.7 | yes |
| pitch re 12-TET A440 (median) (c) | 0.1 | 1 | 2.8 | 5.8 | 2.5 | 0.6 |  |
| pitch scatter from key to key (c) | 0.5 | 3.9 | 1 | 2.1 | 1.2 | 0.4 |  |
| noisiness (spectral flatness) (dB) | -56.8 | -53.0 | -50.5 | -34.4 | -56.9 | 0.4 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -67.8 | -124 | -121 | -118 | -108 | 6.6 | yes |
| pitch at the start re settled (c) | -0.6 | 0.1 | -2.5 | -3.8 | -0.6 | 0.2 |  |
| stereo coherence | 0.75 | 1 | 1 | 1 | 1 | 3.3 | yes |
| partial-to-partial jaggedness (dB) | 2.45 | 1.4 | 3.3 | 2.85 | 3 | 0.5 |  |
| dip between legato notes (dB) | 8.1 | 4.3 | 4.4 | 7.1 | 3.9 | 1.9 |  |
| level evenness of a legato scale (dB) | 1.8 | 1.5 | 1.5 | 0.5 | 1.3 | 0.4 |  |
| level over the keyboard (dB/oct) | 1.1 | 0.84 | 1.49 | 0.13 | 1.17 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | 1.4 | 1.8 | 0.5 | -1.9 | 0.5 | 0.5 |  |
| key-to-key level scatter (dB) | 2.13 | 0.62 | 0.65 | 1.04 | 0.51 | 3.0 | yes |
| odd / even partials (dB) | -6.3 | 1.8 | 1 | -2.1 | 2.9 | 3.1 | yes |
| onset jitter in a phrase (ms) | 26.8 | 27.3 | 35.9 | 34.9 | 7.4 | 0.3 |  |
| onset in a phrase (re the note-on) (ms) | 44.0 | -2 | 1 | -0 | -2 | 4.5 | yes |
| onset sharpness (dB/ms) | 12.7 | 10.1 | 5 | 11.8 | 47.8 | 0.1 |  |
| overtone levels, note for note (dB rms) | 11.1 (nearest 10.2) | fonts among themselves 8.03 | | | | 1.4 |  |
| panning across the keys (dB/oct) | -0.19 | 0 | 0 | 0 | -0.01 | 0.2 |  |
| partial envelopes alike | -0.052 | -0.035 | 0.068 | 0.426 | 0.52 | 1.1 |  |
| release time (30 dB after note-off) (ms) | 619 | 66.0 | 52.0 | 91.0 | 54.0 | 8.4 | yes |
| spectrogram (1 - correlation), note for note | 0.631 (nearest 0.518) | fonts among themselves 0.145 | | | | 4.4 | yes |
| spectral shape of the body, note for note | 17.8 (nearest 15.2) | fonts among themselves 7.71 | | | | 2.3 | yes |
| sounding length of a 100 ms note (ms) | 207 | 127 | 130 | 136 | 133 | 2.6 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.12 | 0.02 | -0.06 | -0.1 | -0.07 | 0.1 |  |
| level after 2 s held, re peak (dB) | -6.8 | -6.3 | -5.7 | -14.8 | -8.6 | 0.1 |  |
| high bands lag low bands by (ms) | -1.5 | 0 | -2.5 | 0 | -1.5 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 8.1 | 0 | 4.4 | 5.9 | 4.4 | 0.7 |  |
| fundamental share (tristimulus 1) | 0.369 | 0.493 | 0.075 | 0.127 | 0.074 | 1.3 |  |
| partials 2-4 share (tristimulus 2) | 0.562 | 0.156 | 0.566 | 0.511 | 0.578 | 0.1 |  |
| partials 5+ share (tristimulus 3) | 0.012 | 0.094 | 0.36 | 0.161 | 0.308 | 1.7 | yes |
| loudness range, velocity 40 to 127 (dB) | 17.4 | 16.7 | 16.7 | 16.7 | 20.1 | 0.4 |  |
| pitch wobble / vibrato (p-p) (c) | 3.8 | 1.8 | 1.9 | 5.1 | 1.3 | 0.4 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.027 (nearest 0.0258) | fonts among themselves 0.0102 | | | | 2.6 | yes |

</details>

### 86 Lead 6 (voice): lead-voice

Distance 2.35 (rank 89 of 129); the fonts disagree 1.4 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Brightens with velocity (+1.6 vs 0 ERB)**: DELIBERATE: "oo" on a soft key to "ah" on a hard one (synth_leads.md §10 table).
- **Brightness rises slower up the keys (2.2 vs 3.5…3.7 ERB/oct)**: DELIBERATE consequence of fixed formants. A voice does this too.
- **Late in a phrase (65 vs −1…10 ms)**: probable FAULT. The largest phrase delay among the leads, and this voice should be quick.
- **Quieter (−1.3 vs +1.9…+6.1 dB)**: a trim.
- **Release 772 ms, floor −63 dB, coherence 0.67**: the room plus the bucket-brigade chorus. ARTIFACT or by design.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (1.63 ERB | 0 | -0 | -0 | -0.23), outside the fonts, 5.4 x.
2. **onset in a phrase (re the note-on)**: speaks later in a phrase (65.0 ms | 10.0 | -1 | -0 | 5), outside the fonts, 6.2 x.
3. **release time (30 dB after note-off) [room]**: rings longer after the key (772 ms | 311 | 81.0 | 67.0 | 70.0), outside the fonts, 3.0 x.
4. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-62.6 dB | -126 | -126 | -125 | -114), outside the fonts, 10.4 x.
5. **brightness over the keyboard**: brightens slower up the keyboard (2.2 ERB/oct | 3.53 | 3.74 | 3.47 | 3.57), outside the fonts, 4.5 x.
6. **key-to-key level scatter**: less even from key to key (1.98 dB | 0.76 | 0.87 | 0.37 | 1.07), outside the fonts, 2.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -9 | -7 | +4 | +5 | +9 | -7 |
| re TimGM6mb | -3 | -4 | +4 | -1 | +3 | -2 |
| re Jnsgm2 | -3 | -7 | +3 | +1 | +6 | -3 |
| re SGM | -6 | -9 | +1 | -2 | +5 | +7 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -5 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 3.8 dB rms; TimGM6mb: centroid -3.2 ERB, attack +2.3 ERB, velocity brightening -0.4 ERB, loudness range +4 dB, envelope 3.9 dB rms; Jnsgm2: centroid -0.4 ERB, attack +0.1 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 3.2 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.34 | 0.07 | -1.61 | 0.46 | 0.52 | 0.6 |  |
| attack time (-20 to -1 dB) (ms) | 173 | 303 | 106 | 153 | 164 | 0.2 |  |
| spectral shape of the attack, note for note | 10.5 (nearest 9.95) | fonts among themselves 9.72 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | 2.2 | 3.53 | 3.74 | 3.47 | 3.57 | 4.5 | yes |
| key-to-key brightness scatter (ERB) | 1.36 | 0.209 | 0.877 | 0.654 | 0.872 | 1.8 | yes |
| brightening from velocity 40 to 127 (ERB) | 1.63 | 0 | -0 | -0 | -0.23 | 5.4 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 12.2 | 7.18 | 11.4 | 9.82 | 9.39 | 1.2 | yes |
| onset delay (ms) | 43.0 | 47.0 | 13.0 | 6 | 9 | 1.6 |  |
| Plomp-Levelt dissonance of a note (log2) | -4.06 | -3.84 | -2.11 | -2.46 | -2.45 | 1.9 | yes |
| envelope curve, note for note (dB rms) | 19.6 (nearest 13.7) | fonts among themselves 8.59 | | | | 2.3 | yes |
| pitch re 12-TET A440 (median) (c) | 0.5 | 2.4 | 1.1 | 1.7 | 2.1 | 0.3 |  |
| pitch scatter from key to key (c) | 1.6 | 9 | 1.9 | 0.4 | 1.2 | 0.0 |  |
| noisiness (spectral flatness) (dB) | -44.9 | -38.0 | -42.5 | -30.3 | -34.0 | 1.5 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -62.6 | -126 | -126 | -125 | -114 | 10.4 | yes |
| pitch at the start re settled (c) | 0.4 | 0.8 | -2.1 | 7.9 | 7.5 | 0.8 |  |
| stereo coherence | 0.665 | 0.81 | 1 | 1 | 1 | 3.5 | yes |
| partial-to-partial jaggedness (dB) | 4.25 | 4.1 | 8.7 | 6.4 | 6.9 | 1.0 |  |
| dip between legato notes (dB) | 11.8 | 17.5 | 12.1 | 12.8 | 12.5 | 0.3 |  |
| level evenness of a legato scale (dB) | 2.4 | 1.5 | 0.5 | 1.3 | 3.2 | 0.7 |  |
| level over the keyboard (dB/oct) | 0.13 | -0.23 | 0.75 | 0.27 | -0.18 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | 3.5 | 2.2 | 2.2 | 3.1 | 6.3 | 0.4 |  |
| key-to-key level scatter (dB) | 1.98 | 0.76 | 0.87 | 0.37 | 1.07 | 2.3 | yes |
| odd / even partials (dB) | -3.2 | -8.9 | 3.2 | -8 | -5.8 | 0.6 |  |
| onset jitter in a phrase (ms) | 49.8 | 27.5 | 18.6 | 56.3 | 56.0 | 0.4 |  |
| onset in a phrase (re the note-on) (ms) | 65.0 | 10.0 | -1 | -0 | 5 | 6.2 | yes |
| onset sharpness (dB/ms) | 4.8 | 2.7 | 11.3 | 10.6 | 12.0 | 1.3 |  |
| overtone levels, note for note (dB rms) | 16.6 (nearest 12.5) | fonts among themselves 11 | | | | 1.5 |  |
| panning across the keys (dB/oct) | -0.08 | 0.11 | 0 | 0 | -0.01 | 0.1 |  |
| partial envelopes alike | -0.019 | -0.01 | 0.007 | 0.013 | 0.175 | 0.3 |  |
| release time (30 dB after note-off) (ms) | 772 | 311 | 81.0 | 67.0 | 70.0 | 3.0 | yes |
| spectrogram (1 - correlation), note for note | 0.302 (nearest 0.237) | fonts among themselves 0.197 | | | | 1.5 |  |
| spectral shape of the body, note for note | 10.3 (nearest 9.43) | fonts among themselves 8.56 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 256 | 168 | 88.0 | 142 | 127 | 2.0 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.1 | 3.19 | -0.21 | -0.37 | 0.81 | 0.2 |  |
| level after 2 s held, re peak (dB) | -7.8 | -7.7 | -8.3 | -7.3 | -11.1 | 0.1 |  |
| high bands lag low bands by (ms) | 5.3 | -4.7 | 3.3 | 1.3 | 7.3 | 0.5 |  |
| transient coherence: attack spread over bands (ms) | 13.2 | 3.1 | 3.8 | 3.7 | 3.1 | 2.0 | yes |
| fundamental share (tristimulus 1) | 0.225 | 0.958 | 0.461 | 0.378 | 0.39 | 0.7 | yes |
| partials 2-4 share (tristimulus 2) | 0.533 | 0.042 | 0.51 | 0.616 | 0.607 | 0.1 |  |
| partials 5+ share (tristimulus 3) | 0.037 | 0 | 0.049 | 0.002 | 0.002 | 0.5 |  |
| loudness range, velocity 40 to 127 (dB) | 15.4 | 16.6 | 16.6 | 16.7 | 20.1 | 0.7 | yes |
| pitch wobble / vibrato (p-p) (c) | 15.2 | 31.3 | 27.6 | 35.0 | 30.6 | 3.2 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0219 (nearest 0.0205) | fonts among themselves 0.0147 | | | | 1.5 | yes |

</details>

### 87 Lead 7 (fifths): lead-fifths

Distance 2.50 (rank 83 of 129); the fonts disagree 0.8 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Brightens with velocity (+1.9 vs 0 ERB) and brighter (11.0 vs 8.6…10.2 ERB)**: DELIBERATE: the dynamics knob and the sine wavefolder.
- **Smoother (Plomp-Levelt −3.4 vs −1.0…−2.7)**: DELIBERATE. The fifth is locked at 3:2 within 1 c by soft sync (synth_leads.md §10), where the fonts are detuned stacks.
- **Phrase onset 56 vs −2…−1 ms**: the same lead-family delay as p081–p085.
- **Floor −69 dB, key scatter 1.5 dB**: room.
- Rank 82, and the fonts agree closely (0.6 JND): close.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (1.94 ERB | -0 | -0 | -0 | -0.15), outside the fonts, 6.5 x.
2. **onset in a phrase (re the note-on)**: speaks later in a phrase (56.0 ms | -1 | -2 | -2 | -1), outside the fonts, 5.8 x.
3. **brightness (spectral centroid, velocity 96)**: brighter (11.0 ERB | 10.2 | 8.71 | 8.97 | 8.61), outside the fonts, 2.7 x.
4. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-69.2 dB | -120 | -127 | -126 | -113), outside the fonts, 8.0 x.
5. **Plomp-Levelt dissonance of a note**: smoother (-3.37 log2 | -2.71 | -1.02 | -1.04 | -1.05), outside the fonts, 2.8 x.
6. **key-to-key level scatter**: less even from key to key (1.28 dB | 0.23 | 0.53 | 0.24 | 0.66), outside the fonts, 1.8 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +8 | +10 | +12 | +12 | -1 | -26 |
| re TimGM6mb | +0 | +1 | +2 | +4 | -5 | -4 |
| re Jnsgm2 | +9 | +9 | +10 | +12 | -0 | -26 |
| re SGM | +3 | +4 | +5 | +7 | -3 | -14 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -5 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack -0.4 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 2.7 dB rms; TimGM6mb: centroid -0.1 ERB, attack +0.0 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 2.8 dB rms; Jnsgm2: centroid -0.3 ERB, attack +0.5 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 3.0 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.29 | -0.64 | 0.43 | 0.15 | 0.47 | 1.0 |  |
| attack time (-20 to -1 dB) (ms) | 62.0 | 11.0 | 380 | 380 | 365 | 1.0 |  |
| spectral shape of the attack, note for note | 15.2 (nearest 13.4) | fonts among themselves 10.3 | | | | 1.5 |  |
| brightness over the keyboard (ERB/oct) | 3.49 | 3.99 | 3.38 | 3.63 | 3.38 | 0.0 |  |
| key-to-key brightness scatter (ERB) | 0.941 | 0.623 | 0.808 | 0.85 | 0.78 | 0.7 |  |
| brightening from velocity 40 to 127 (ERB) | 1.94 | -0 | -0 | -0 | -0.15 | 6.5 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 11.0 | 10.2 | 8.71 | 8.97 | 8.61 | 2.7 | yes |
| onset delay (ms) | 17.0 | 5 | 4 | 1 | 2 | 1.4 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.37 | -2.71 | -1.02 | -1.04 | -1.05 | 2.8 | yes |
| envelope curve, note for note (dB rms) | 12.4 (nearest 12.1) | fonts among themselves 6.87 | | | | 1.8 | yes |
| pitch re 12-TET A440 (median) (c) | 0 | 5.5 | -0.8 | -0.6 | -0.8 | 0.2 |  |
| pitch scatter from key to key (c) | 0.3 | 2.2 | 1.8 | 3.4 | 2.8 | 0.7 | yes |
| noisiness (spectral flatness) (dB) | -57.0 | -57.5 | -35.8 | -24.3 | -29.4 | 1.5 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -69.2 | -120 | -127 | -126 | -113 | 8.0 | yes |
| pitch at the start re settled (c) | -0.3 | -0.7 | 0 | 0 | 0 | 0.1 |  |
| stereo coherence | 0.639 | 0.079 | 1 | 1 | 1 | 0.8 |  |
| partial-to-partial jaggedness (dB) | 4.8 | 0.4 | 1.3 | 1.2 | 1.3 | 3.5 | yes |
| dip between legato notes (dB) | 11.8 | 8.2 | 10.8 | 11.9 | 9.4 | 0.8 |  |
| level evenness of a legato scale (dB) | 1.7 | 1.2 | 1.3 | 1.2 | 3.1 | 0.4 |  |
| level over the keyboard (dB/oct) | 0.34 | -0.43 | 0.36 | -0 | 0.59 | 0.2 |  |
| level against the other instruments (velocity 96) (dB) | 1.6 | -3.8 | 3 | 3.6 | 3.5 | 0.4 |  |
| key-to-key level scatter (dB) | 1.28 | 0.23 | 0.53 | 0.24 | 0.66 | 1.8 | yes |
| odd / even partials (dB) | 1.6 | -2.7 | -1.6 | -1 | -1.2 | 1.5 | yes |
| onset jitter in a phrase (ms) | 49.6 | 33.3 | 45.7 | 50.7 | 44.6 | 0.5 |  |
| onset in a phrase (re the note-on) (ms) | 56.0 | -1 | -2 | -2 | -1 | 5.8 | yes |
| onset sharpness (dB/ms) | 12.0 | 6.5 | 8 | 7.5 | 53.1 | 0.2 |  |
| overtone levels, note for note (dB rms) | 9.91 (nearest 9.48) | fonts among themselves 2.63 | | | | 3.8 | yes |
| panning across the keys (dB/oct) | -0.32 | 0.09 | 0 | 0 | -0.01 | 0.3 |  |
| partial envelopes alike | 0.145 | -0.123 | -0.045 | -0.064 | 0.226 | 1.1 |  |
| release time (30 dB after note-off) (ms) | 510 | 77.0 | 253 | 264 | 182 | 1.4 | yes |
| spectrogram (1 - correlation), note for note | 0.37 (nearest 0.254) | fonts among themselves 0.233 | | | | 1.6 |  |
| spectral shape of the body, note for note | 16.5 (nearest 14.4) | fonts among themselves 10.2 | | | | 1.6 | yes |
| sounding length of a 100 ms note (ms) | 188 | 59.0 | 151 | 151 | 152 | 0.5 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.04 | 0.01 | -0.18 | 0.13 | -0.18 | 0.1 |  |
| level after 2 s held, re peak (dB) | -7.9 | -9.7 | -9.7 | -8 | -10.8 | 0.6 |  |
| high bands lag low bands by (ms) | 1.1 | 0.5 | -1 | -2.2 | -0.5 | 0.4 |  |
| transient coherence: attack spread over bands (ms) | 3.5 | 0.9 | 2.8 | 2.8 | 0.9 | 0.3 |  |
| fundamental share (tristimulus 1) | 0.601 | 0.731 | 0.772 | 0.772 | 0.766 | 2.4 | yes |
| partials 2-4 share (tristimulus 2) | 0.391 | 0.201 | 0.18 | 0.188 | 0.194 | 2.9 | yes |
| partials 5+ share (tristimulus 3) | 0.009 | 0.094 | 0.035 | 0.042 | 0.037 | 0.4 |  |
| loudness range, velocity 40 to 127 (dB) | 15.5 | 16.7 | 16.7 | 16.7 | 20.1 | 0.7 | yes |
| pitch wobble / vibrato (p-p) (c) | 2.4 | 2.6 | 33.2 | 33.3 | 37.8 | 1.8 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.025 (nearest 0.0217) | fonts among themselves 0.0151 | | | | 1.7 | yes |

</details>

### 88 Lead 8 (bass + lead): lead-bass+lead

Distance 3.05 (rank 45 of 129); the fonts disagree 0.8 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Darker (6.95 vs 9.2…10.2 ERB; +4…+9 dB under 150 Hz)**: FAULT for GM 88. The ÷2 and ÷4 flip-flops (synth_leads.md §10) let the sub-octaves swamp the "bright lead" on top that §4 asks for.
- **Slow attack (97 vs 2…8 ms; phrase onset 44 vs −2…0)**: FAULT. It is the slowest lead, and too slow for a bass+lead stab. The room explains only part of it.
- **Quieter (+0.5 vs +3.5…+6.5 dB), noisier (flatness −47 vs −59…−64 dB)**: a trim; the noise is probably the ring modulator's hum and board noise.
- **Release 565 vs 5…31 ms**: room ARTIFACT.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: darker (6.95 ERB | 9.33 | 9.3 | 10.2 | 9.23), outside the fonts, 4.6 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (565 ms | 6 | 6 | 31.0 | 6), outside the fonts, 5.5 x.
3. **attack time (-20 to -1 dB)**: a slower attack (96.5 ms | 2 | 8 | 5 | 5), outside the fonts, 4.3 x.
4. **noisiness (spectral flatness)**: noisier (-46.5 dB | -59.1 | -59.6 | -64.0 | -62.2), outside the fonts, 5.9 x.
5. **key-to-key level scatter**: less even from key to key (2.09 dB | 0.24 | 0.8 | 0.33 | 1.17), outside the fonts, 3.0 x.
6. **onset in a phrase (re the note-on)**: speaks later in a phrase (44.0 ms | -2 | -1 | -2 | 0), outside the fonts, 4.5 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +9 | +6 | +6 | +4 | -0 | -14 |
| re TimGM6mb | +9 | +7 | +11 | +3 | -2 | -15 |
| re Jnsgm2 | +8 | +8 | +7 | +2 | -3 | -11 |
| re SGM | +4 | +2 | +6 | -2 | -3 | -3 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.5 ERB, attack -0.4 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 5.2 dB rms; TimGM6mb: centroid +0.3 ERB, attack +0.2 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 2.9 dB rms; Jnsgm2: centroid -0.1 ERB, attack +1.4 ERB, velocity brightening -0.4 ERB, loudness range +4 dB, envelope 3.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.2 | 0.86 | 0.39 | -0.66 | 0.58 | 0.9 |  |
| attack time (-20 to -1 dB) (ms) | 96.5 | 2 | 8 | 5 | 5 | 4.3 | yes |
| spectral shape of the attack, note for note | 17.7 (nearest 13.2) | fonts among themselves 6.97 | | | | 2.5 | yes |
| brightness over the keyboard (ERB/oct) | 2.71 | 3.46 | 4.5 | 4.12 | 4.05 | 2.6 | yes |
| key-to-key brightness scatter (ERB) | 1.12 | 0.434 | 1.12 | 1.17 | 1.1 | 0.0 |  |
| brightening from velocity 40 to 127 (ERB) | 0.77 | 0 | 0 | 0 | -0.26 | 2.6 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 6.95 | 9.33 | 9.3 | 10.2 | 9.23 | 4.6 | yes |
| onset delay (ms) | 18.0 | 1 | 2 | 4 | 4 | 1.5 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.16 | -4.44 | -3.83 | -3.24 | -4.64 | 2.8 | yes |
| envelope curve, note for note (dB rms) | 19.3 (nearest 18.9) | fonts among themselves 5.92 | | | | 3.3 | yes |
| pitch re 12-TET A440 (median) (c) | 0 | 3.4 | 2.3 | 0 | 2 | 0.5 |  |
| pitch scatter from key to key (c) | 0.4 | 1 | 0.9 | 1.8 | 0.6 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -46.5 | -59.1 | -59.6 | -64.0 | -62.2 | 5.9 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -69.2 | -123 | -127 | -126 | -114 | 8.9 | yes |
| pitch at the start re settled (c) | -0.3 | 0 | 0 | 0 | 0.2 | 0.1 |  |
| stereo coherence | 0.625 | 0.885 | 0.997 | 1 | 0.998 | 5.0 | yes |
| partial-to-partial jaggedness (dB) | 2.55 | 0.2 | 0.5 | 0.5 | 0.7 | 2.0 | yes |
| dip between legato notes (dB) | 11.7 | 11.2 | 10.4 | 8.2 | 9.6 | 0.8 |  |
| level evenness of a legato scale (dB) | 0.6 | 1.5 | 1.3 | 0.8 | 3.7 | 0.6 |  |
| level over the keyboard (dB/oct) | -0.79 | -0.32 | -1.04 | -0.72 | -0.53 | 0.2 |  |
| level against the other instruments (velocity 96) (dB) | 6.6 | 3.6 | 4.8 | 4.8 | 6.7 | 1.2 |  |
| key-to-key level scatter (dB) | 2.09 | 0.24 | 0.8 | 0.33 | 1.17 | 3.0 | yes |
| odd / even partials (dB) | -3.4 | -3.3 | -3.2 | -2.9 | -5.1 | 0.1 |  |
| onset jitter in a phrase (ms) | 53.9 | 16.7 | 17.4 | 5.1 | 20.4 | 4.8 | yes |
| onset in a phrase (re the note-on) (ms) | 44.0 | -2 | -1 | -2 | 0 | 4.5 | yes |
| onset sharpness (dB/ms) | 13.4 | 5.3 | 7.7 | 11.7 | 48.5 | 0.2 |  |
| overtone levels, note for note (dB rms) | 7.42 (nearest 6.2) | fonts among themselves 3.73 | | | | 2.0 | yes |
| panning across the keys (dB/oct) | 0.27 | -0.01 | -0 | 0 | 0 | 0.3 |  |
| partial envelopes alike | 0.167 | 0.865 | -0.076 | -0.065 | 0.194 | 0.2 |  |
| release time (30 dB after note-off) (ms) | 565 | 6 | 6 | 31.0 | 6 | 5.5 | yes |
| spectrogram (1 - correlation), note for note | 0.796 (nearest 0.478) | fonts among themselves 0.24 | | | | 3.3 | yes |
| spectral shape of the body, note for note | 23.1 (nearest 20.5) | fonts among themselves 5.65 | | | | 4.1 | yes |
| sounding length of a 100 ms note (ms) | 134 | 102 | 99.0 | 91.0 | 88.0 | 2.0 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.8 | 0.01 | 0.03 | -0.18 | 0.23 | 1.6 | yes |
| level after 2 s held, re peak (dB) | -8.4 | -7 | -7.4 | -8.8 | -15.9 | 0.1 |  |
| high bands lag low bands by (ms) | -1 | 0 | 0 | 0 | 0 | 0.2 |  |
| transient coherence: attack spread over bands (ms) | 3.2 | 0 | 0 | 0 | 0 | 0.6 | yes |
| fundamental share (tristimulus 1) | 0.607 | 0.673 | 0.767 | 0.681 | 0.772 | 1.7 | yes |
| partials 2-4 share (tristimulus 2) | 0.296 | 0.268 | 0.186 | 0.227 | 0.188 | 1.3 |  |
| partials 5+ share (tristimulus 3) | 0.062 | 0.058 | 0.067 | 0.074 | 0.055 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 15.4 | 16.7 | 16.7 | 16.6 | 20.1 | 0.7 | yes |
| pitch wobble / vibrato (p-p) (c) | 3 | 0 | 0.3 | 0 | 1.2 | 0.6 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0299 (nearest 0.0285) | fonts among themselves 0.0136 | | | | 2.2 | yes |

</details>

### 89 Pad 1 (new age): pad-new-age

Distance 1.88 (rank 110 of 129); the fonts disagree 1.4 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Speaks later (onset 162 vs 3…81 ms; attack 328 vs 7…763)**: borderline. The fonts disagree widely (SGM 763 ms). In a phrase it is quicker than every font (10 vs 35…62 ms), so it is not a fault.
- **Fundamental share 0.98 vs 0.72…0.78; odd/even +3.4 vs −5…−11 dB**: the bell is ring-modulated at 2.76×, which is inharmonic, so the harmonic measures miss it. A MEASUREMENT ARTIFACT, partly.
- **Brightens with velocity (+0.6 ERB), key scatter 1.4 dB**: small; the dynamics knob and the room.
- Rank 109: among the closest programs.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (1.83 dB | 0.29 | 0.46 | 0.43 | 0.6), outside the fonts, 2.8 x.
2. **onset delay**: speaks later (162 ms | 3 | 33.0 | 34.0 | 79.0), outside the fonts, 3.4 x.
3. **fundamental share (tristimulus 1)**: more fundamental (0.979 | 0.753 | 0.723 | 0.764 | 0.782), outside the fonts, 3.1 x.
4. **brightening from velocity 40 to 127**: brightens more with velocity (0.6 ERB | -0 | 0.01 | 0.01 | -0.28), outside the fonts, 2.0 x.
5. **brightness (spectral centroid, velocity 96)**: brighter (11.0 ERB | 8.88 | 9.85 | 9.67 | 10.5), outside the fonts, 1.5 x.
6. **odd / even partials**: more odd partials (hollower) (3.4 dB | -6 | -11.4 | -10.8 | -4.7), outside the fonts, 3.5 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +2 | +1 | +5 | -0 | -0 | -5 |
| re TimGM6mb | +6 | +4 | +10 | +4 | -6 | -9 |
| re Jnsgm2 | +6 | +4 | +9 | +5 | -5 | -11 |
| re SGM | +2 | +6 | +7 | +1 | +1 | -11 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.4 ERB, attack +0.9 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 3.0 dB rms; TimGM6mb: centroid -0.2 ERB, attack +0.1 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 2.4 dB rms; Jnsgm2: centroid -0.1 ERB, attack +0.1 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 2.0 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.06 | 0.49 | 1.02 | 1.3 | 1.41 | 2.4 | yes |
| attack time (-20 to -1 dB) (ms) | 328 | 7 | 96.0 | 100 | 759 | 0.5 |  |
| spectral shape of the attack, note for note | 15 (nearest 14.7) | fonts among themselves 13 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 4.28 | 3.52 | 4.3 | 4.37 | 4.03 | 0.3 |  |
| key-to-key brightness scatter (ERB) | 1.49 | 0.258 | 1.3 | 1.05 | 0.817 | 1.1 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.6 | -0 | 0.01 | 0.01 | -0.28 | 2.0 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 11.0 | 8.88 | 9.85 | 9.67 | 10.5 | 1.5 | yes |
| decay rate of a held note (dB/s) | 2.7 | 5.7 | 1.3 | 0.8 | 1.9 | 0.6 |  |
| onset delay (ms) | 162 | 3 | 33.0 | 34.0 | 79.0 | 3.4 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.12 | -3.92 | -1.75 | -1.81 | -3.72 | 1.2 | yes |
| envelope curve, note for note (dB rms) | 7.66 (nearest 6.43) | fonts among themselves 5.1 | | | | 1.5 |  |
| pitch re 12-TET A440 (median) (c) | 0.8 | -1.7 | 0.7 | -0.5 | -2 | 0.5 |  |
| pitch scatter from key to key (c) | 3.4 | 7.7 | 5.6 | 3.3 | 6.8 | 0.9 |  |
| noisiness (spectral flatness) (dB) | -53.5 | -46.2 | -41.2 | -42.6 | -48.0 | 2.7 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -49.5 | -46.5 | -63.1 | -63.5 | -58.3 | 1.3 |  |
| pitch at the start re settled (c) | 0.3 | -1.6 | -2.9 | -0.4 | 8.7 | 0.2 |  |
| stereo coherence | 0.702 | 1 | 1 | 1 | 0.536 | 1.3 |  |
| partial-to-partial jaggedness (dB) | 5.45 | 1.9 | 5.3 | 4.5 | 5.1 | 0.4 |  |
| dip between legato notes (dB) | 14.4 | 9.4 | 11.7 | 9.4 | 10.0 | 2.4 | yes |
| level evenness of a legato scale (dB) | 3.4 | 1.7 | 1.6 | 1.6 | 0.9 | 1.8 | yes |
| level over the keyboard (dB/oct) | 0.39 | -0.09 | -0.36 | -0.88 | 0.22 | 0.6 |  |
| level against the other instruments (velocity 96) (dB) | -3.1 | -1.8 | 1.4 | 0.4 | -3.6 | 1.0 |  |
| key-to-key level scatter (dB) | 1.83 | 0.29 | 0.46 | 0.43 | 0.6 | 2.8 | yes |
| odd / even partials (dB) | 3.4 | -6 | -11.4 | -10.8 | -4.7 | 3.5 | yes |
| onset jitter in a phrase (ms) | 52.7 | 33.6 | 28.9 | 20.4 | 41.3 | 2.1 | yes |
| onset in a phrase (re the note-on) (ms) | 10.0 | 62.0 | 35.0 | 40.0 | 48.0 | 2.5 | yes |
| onset sharpness (dB/ms) | 6.1 | 5.8 | 3 | 3.3 | 17.6 | 0.2 |  |
| overtone levels, note for note (dB rms) | 15.1 (nearest 12) | fonts among themselves 12.4 | | | | 1.2 |  |
| panning across the keys (dB/oct) | 0.87 | 0 | 0 | 0 | -0.25 | 0.9 | yes |
| partial envelopes alike | 0.072 | 0.659 | -0.091 | -0.092 | -0.04 | 0.4 |  |
| release time (30 dB after note-off) (ms) | 882 | 1213 | 666 | 634 | 745 | 0.7 |  |
| spectrogram (1 - correlation), note for note | 0.297 (nearest 0.264) | fonts among themselves 0.234 | | | | 1.3 |  |
| spectral shape of the body, note for note | 15.7 (nearest 15.1) | fonts among themselves 11.6 | | | | 1.4 | yes |
| sounding length of a 100 ms note (ms) | 372 | 70.0 | 400 | 400 | 363 | 0.0 |  |
| partial stretch (inharmonicity) (c/partial) | -0.49 | -0.5 | -0.43 | -1.13 | -1.14 | 0.6 |  |
| level after 2 s held, re peak (dB) | -9.4 | -16.9 | -8.5 | -8.6 | -8.3 | 0.2 |  |
| high bands lag low bands by (ms) | 8.2 | 1 | -8.6 | -8.7 | -11.0 | 2.8 | yes |
| transient coherence: attack spread over bands (ms) | 24.2 | 0.9 | 10.7 | 10.7 | 13.8 | 2.1 | yes |
| fundamental share (tristimulus 1) | 0.979 | 0.753 | 0.723 | 0.764 | 0.782 | 3.1 | yes |
| partials 2-4 share (tristimulus 2) | 0.021 | 0.237 | 0.121 | 0.059 | 0.208 | 1.6 | yes |
| partials 5+ share (tristimulus 3) | 0.001 | 0.02 | 0.129 | 0.114 | 0.005 | 0.9 |  |
| loudness range, velocity 40 to 127 (dB) | 17.4 | 16.6 | 16.6 | 16.7 | 20.1 | 0.4 |  |
| pitch wobble / vibrato (p-p) (c) | 14.7 | 4.7 | 31.3 | 31.2 | 12.5 | 0.5 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0354 (nearest 0.032) | fonts among themselves 0.0263 | | | | 1.3 |  |

</details>

### 90 Pad 2 (warm): pad-warm

Distance 1.64 (rank 116 of 129); the fonts disagree 1.0 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Release 866 vs 412…507 ms**: the fonts' pads already release in ~0.45 s, and the room adds ~0.3–0.4 s. ARTIFACT.
- **Darker above 6 kHz (−22…−27 dB re FluidR3 and SGM)**: DELIBERATE warmth: the saturating output transformer and the ladder (README pad table). TimGM6mb and Jnsgm2 have nothing up there, so the fonts disagree.
- **Brightens with velocity (+0.8 ERB), key scatter 2.2 vs 0.6…1.2 dB**: the dynamics knob, and the transformer compressing low notes; plus the room.
- Rank 106: close.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (866 ms | 436 | 434 | 412 | 523), outside the fonts, 4.0 x.
2. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.338, nearest 0.275; the fonts among themselves 0.0706), outside.
3. **brightening from velocity 40 to 127**: brightens more with velocity (0.81 ERB | 0 | 0 | -0 | -0.33), outside the fonts, 2.7 x.
4. **key-to-key level scatter**: less even from key to key (2.1 dB | 0.64 | 1.04 | 0.81 | 1.2), outside the fonts, 2.4 x.
5. **spectral shape of the body, note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 16.9, nearest 15.3; the fonts among themselves 5.15), outside.
6. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 12.5, nearest 12.4; the fonts among themselves 5.51), outside.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +10 | +10 | +11 | +10 | -6 | -27 |
| re TimGM6mb | +1 | +2 | +3 | -1 | -3 | - |
| re Jnsgm2 | +2 | +3 | +3 | -1 | -5 | - |
| re SGM | +7 | +9 | +10 | -0 | -11 | -22 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +5; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.3 ERB, attack -0.2 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 2.4 dB rms; TimGM6mb: centroid -0.2 ERB, attack -0.0 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 1.0 dB rms; Jnsgm2: centroid -0.2 ERB, attack -0.0 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 1.0 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.41 | 0.38 | 0.28 | 0.29 | -0.29 | 1.7 |  |
| attack time (-20 to -1 dB) (ms) | 689 | 549 | 597 | 620 | 671 | 0.7 |  |
| spectral shape of the attack, note for note | 14.3 (nearest 14.2) | fonts among themselves 6.22 | | | | 2.3 | yes |
| brightness over the keyboard (ERB/oct) | 2.86 | 3.17 | 3.25 | 3.25 | 3.74 | 1.3 | yes |
| key-to-key brightness scatter (ERB) | 0.778 | 0.308 | 0.594 | 0.61 | 0.522 | 1.1 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.81 | 0 | 0 | -0 | -0.33 | 2.7 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 7.95 | 8.29 | 7.7 | 7.81 | 7.59 | 0.6 |  |
| onset delay (ms) | 357 | 370 | 378 | 433 | 439 | 1.4 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.53 | -3.94 | -3.57 | -3.82 | -5.21 | 0.8 |  |
| envelope curve, note for note (dB rms) | 12.5 (nearest 12.4) | fonts among themselves 5.51 | | | | 2.3 | yes |
| pitch re 12-TET A440 (median) (c) | -1.9 | -2.1 | 0 | -1.3 | -3.4 | 0.0 |  |
| pitch scatter from key to key (c) | 4.7 | 8.5 | 5.6 | 4 | 6.4 | 0.4 |  |
| noisiness (spectral flatness) (dB) | -64.1 | -55.2 | -59.8 | -52.8 | -72.8 | 0.7 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -42.2 | -81.5 | -120 | -120 | -82.3 | 3.1 | yes |
| pitch at the start re settled (c) | 2.8 | -11.6 | -1.5 | -0.9 | -7.2 | 1.3 | yes |
| stereo coherence | 0.748 | 0.835 | 1 | 1 | 0.202 | 0.4 |  |
| partial-to-partial jaggedness (dB) | 2.6 | 1.4 | 1.1 | 1.05 | 1.8 | 1.4 | yes |
| dip between legato notes (dB) | 12.0 | 12.4 | 8.6 | 9 | 8.3 | 1.6 |  |
| level evenness of a legato scale (dB) | 3.5 | 3.3 | 2 | 1.9 | 1.9 | 1.6 |  |
| level over the keyboard (dB/oct) | -0.04 | 1.57 | -0.03 | -0.42 | -1.31 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | -4.4 | -3.4 | -5.5 | -6.6 | -3.9 | 0.2 |  |
| key-to-key level scatter (dB) | 2.1 | 0.64 | 1.04 | 0.81 | 1.2 | 2.4 | yes |
| odd / even partials (dB) | -6.1 | -4.3 | -6.2 | -5.6 | -6.2 | 0.1 |  |
| onset jitter in a phrase (ms) | 31.6 | 61.2 | 53.1 | 42.4 | 53.0 | 2.3 | yes |
| onset in a phrase (re the note-on) (ms) | 48.0 | 34.0 | 20.0 | 4 | 25.0 | 1.7 | yes |
| onset sharpness (dB/ms) | 9.7 | 9 | 6.6 | 6.7 | 5.5 | 1.5 |  |
| overtone levels, note for note (dB rms) | 8.24 (nearest 7.15) | fonts among themselves 4.94 | | | | 1.7 | yes |
| panning across the keys (dB/oct) | 0.81 | 1.46 | 0 | 0 | 1.04 | 0.3 |  |
| partial envelopes alike | 0.276 | 0.402 | 0.491 | 0.562 | 0.352 | 1.6 | yes |
| release time (30 dB after note-off) (ms) | 866 | 436 | 434 | 412 | 523 | 4.0 | yes |
| spectrogram (1 - correlation), note for note | 0.338 (nearest 0.275) | fonts among themselves 0.0706 | | | | 4.8 | yes |
| spectral shape of the body, note for note | 16.9 (nearest 15.3) | fonts among themselves 5.15 | | | | 3.3 | yes |
| sounding length of a 100 ms note (ms) | 198 | 255 | 294 | 269 | 203 | 1.5 |  |
| partial stretch (inharmonicity) (c/partial) | -0.11 | 0.06 | 0.7 | 0.55 | -0.64 | 0.6 |  |
| level after 2 s held, re peak (dB) | -8.2 | -8.8 | -6.7 | -7.4 | -9 | 0.0 |  |
| high bands lag low bands by (ms) | 3.5 | 5 | 22.1 | 18.0 | 10.0 | 1.2 |  |
| transient coherence: attack spread over bands (ms) | 17.8 | 10.7 | 16.0 | 16.3 | 16.6 | 0.3 |  |
| fundamental share (tristimulus 1) | 0.946 | 0.826 | 0.922 | 0.912 | 0.925 | 0.4 |  |
| partials 2-4 share (tristimulus 2) | 0.053 | 0.173 | 0.078 | 0.088 | 0.074 | 0.4 |  |
| partials 5+ share (tristimulus 3) | 0.001 | 0.002 | 0.001 | 0.001 | 0.001 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 18.1 | 16.6 | 16.6 | 16.7 | 20.1 | 0.8 |  |
| pitch wobble / vibrato (p-p) (c) | 18.3 | 28.1 | 31.2 | 30.0 | 18.9 | 1.7 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0159 (nearest 0.0139) | fonts among themselves 0.0106 | | | | 1.5 | yes |

</details>

### 91 Pad 3 (polysynth): pad-polysynth

Distance 1.55 (rank 123 of 129); the fonts disagree 1.4 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Thin bass, louder toward the top (+1.55 vs −0.3…+0.6 dB/oct; brightness rises 2.5 vs 3.0…3.8 ERB/oct)**: DELIBERATE consequence of the 140 Hz RC high-pass before the ladder (README pad table; synth_leads.md "The pads").
- **Phrase onset 66 vs −2…17 ms**: the lead family's slur delay again. Check it.
- **Brightens with velocity (+1.4 ERB)**: DELIBERATE (the dynamics knob).
- **Quieter (−1.2 vs +0.8…+3.6 dB)**: a trim.
- Rank 107: close.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (1.42 ERB | -0 | -0 | 0 | -0.08), outside the fonts, 4.7 x.
2. **onset in a phrase (re the note-on)**: speaks later in a phrase (66.0 ms | 17.0 | 10.0 | -0 | -1), outside the fonts, 6.1 x.
3. **brightness over the keyboard**: brightens slower up the keyboard (2.54 ERB/oct | 3.82 | 3.84 | 3.04 | 3.14), outside the fonts, 2.4 x.
4. **spectral shape of the body, note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 14.3, nearest 12.9; the fonts among themselves 9.74), outside.
5. **stereo coherence [room]**: wider (0.594 | 0.667 | 0.992 | 1 | 0.993), outside the fonts, 2.4 x.
6. **pitch wobble / vibrato (p-p)**: wobbles less (17.6 c | 31.9 | 31.7 | 19.7 | 31.4), inside the fonts' range, 2.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +1 | +4 | +7 | +3 | +3 | -12 |
| re TimGM6mb | +0 | +4 | +4 | -1 | -6 | +0 |
| re Jnsgm2 | +8 | +8 | +8 | +3 | -4 | -12 |
| re SGM | +3 | +1 | +3 | -2 | -2 | -0 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -14 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack -0.1 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 4.1 dB rms; TimGM6mb: centroid +0.7 ERB, attack -0.2 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 4.5 dB rms; Jnsgm2: centroid -0.7 ERB, attack +0.6 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 3.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.16 | 1.24 | 0.27 | -2.16 | 0.53 | 0.3 |  |
| attack time (-20 to -1 dB) (ms) | 120 | 237 | 196 | 5 | 334 | 0.3 |  |
| spectral shape of the attack, note for note | 14.2 (nearest 11.5) | fonts among themselves 10.4 | | | | 1.4 |  |
| brightness over the keyboard (ERB/oct) | 2.54 | 3.82 | 3.84 | 3.04 | 3.14 | 2.4 | yes |
| key-to-key brightness scatter (ERB) | 0.942 | 0.329 | 1.3 | 1.04 | 1.06 | 0.2 |  |
| brightening from velocity 40 to 127 (ERB) | 1.42 | -0 | -0 | 0 | -0.08 | 4.7 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 9 | 7.39 | 9.19 | 10.7 | 8.96 | 0.0 |  |
| onset delay (ms) | 30.0 | 49.0 | 7 | 4 | 12.0 | 0.9 |  |
| Plomp-Levelt dissonance of a note (log2) | -3.21 | -3.63 | -2.26 | -0.99 | -2.38 | 0.7 |  |
| envelope curve, note for note (dB rms) | 14.2 (nearest 6.51) | fonts among themselves 9.98 | | | | 1.4 |  |
| pitch re 12-TET A440 (median) (c) | 0.3 | -0.7 | -0.9 | 5.7 | 0.5 | 0.1 |  |
| pitch scatter from key to key (c) | 3.9 | 9 | 3.4 | 21.3 | 4.7 | 0.3 |  |
| noisiness (spectral flatness) (dB) | -53.2 | -44.6 | -38.5 | -55.0 | -40.5 | 1.3 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -61.2 | -69.3 | -123 | -123 | -110 | 2.1 | yes |
| pitch at the start re settled (c) | -0.6 | 1 | -2.2 | 3.6 | -3.6 | 0.0 |  |
| stereo coherence | 0.594 | 0.667 | 0.992 | 1 | 0.993 | 2.4 | yes |
| partial-to-partial jaggedness (dB) | 3 | 1.9 | 1.3 | 0.2 | 1.3 | 1.7 | yes |
| dip between legato notes (dB) | 12.0 | 18.0 | 11.6 | 5.3 | 8.9 | 0.3 |  |
| level evenness of a legato scale (dB) | 1.9 | 1.6 | 1.5 | 0.8 | 2.4 | 0.3 |  |
| level over the keyboard (dB/oct) | 0.74 | 0.04 | -0.17 | -0.26 | 0.58 | 0.8 |  |
| level against the other instruments (velocity 96) (dB) | 1.6 | 3.7 | 1.2 | 1.2 | 1 | 0.3 |  |
| key-to-key level scatter (dB) | 1.82 | 0.83 | 0.86 | 0.4 | 1.94 | 1.3 |  |
| odd / even partials (dB) | -5.3 | -8.3 | -4.4 | -2.7 | -4.2 | 0.4 |  |
| onset jitter in a phrase (ms) | 41.2 | 49.9 | 39.5 | 2.2 | 17.5 | 0.5 |  |
| onset in a phrase (re the note-on) (ms) | 66.0 | 17.0 | 10.0 | -0 | -1 | 6.1 | yes |
| onset sharpness (dB/ms) | 7.5 | 5.9 | 6.1 | 9.7 | 49.8 | 0.0 |  |
| overtone levels, note for note (dB rms) | 8.31 (nearest 7.23) | fonts among themselves 7.84 | | | | 1.1 |  |
| panning across the keys (dB/oct) | 0.17 | -0.74 | -0.08 | 0 | -0.18 | 0.3 |  |
| partial envelopes alike | 0.019 | -0.052 | -0.071 | 0.007 | -0.017 | 0.5 |  |
| release time (30 dB after note-off) (ms) | 612 | 579 | 292 | 190 | 304 | 1.3 |  |
| spectrogram (1 - correlation), note for note | 0.341 (nearest 0.295) | fonts among themselves 0.228 | | | | 1.5 |  |
| spectral shape of the body, note for note | 14.3 (nearest 12.9) | fonts among themselves 9.74 | | | | 1.5 | yes |
| sounding length of a 100 ms note (ms) | 224 | 228 | 199 | 133 | 158 | 0.9 |  |
| partial stretch (inharmonicity) (c/partial) | -0.2 | -0.99 | 0.16 | -0.92 | 0.42 | 0.3 |  |
| level after 2 s held, re peak (dB) | -10.0 | -11.5 | -8.6 | -8.2 | -8.1 | 0.5 |  |
| high bands lag low bands by (ms) | 5.2 | -3.3 | -1 | 4.2 | -3.3 | 1.5 |  |
| transient coherence: attack spread over bands (ms) | 8.5 | 8.6 | 19.9 | 1.1 | 4 | 0.2 |  |
| fundamental share (tristimulus 1) | 0.716 | 0.886 | 0.787 | 0.634 | 0.874 | 0.9 |  |
| partials 2-4 share (tristimulus 2) | 0.238 | 0.109 | 0.193 | 0.294 | 0.121 | 0.9 |  |
| partials 5+ share (tristimulus 3) | 0.01 | 0.005 | 0.028 | 0.088 | 0.009 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 18.0 | 16.7 | 16.7 | 16.7 | 20.1 | 0.8 |  |
| pitch wobble / vibrato (p-p) (c) | 17.6 | 31.9 | 31.7 | 19.7 | 31.4 | 2.3 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0259 (nearest 0.0214) | fonts among themselves 0.0189 | | | | 1.4 |  |

</details>

### 92 Pad 4 (choir): pad-choir

Distance 4.69 (rank 7 of 129); the fonts disagree 1.1 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Far too bright and thin (centroid 18.6 vs 7.3…10.8 ERB; −26…−34 dB under 150 Hz, +10…+23 dB above 6 kHz; fundamental share 0.03 vs 0.19…0.92; flatness −27 dB)**: FAULT. Puhuja's singers on a pad should not peak near 4–5 kHz. Suspect the breath (K3) default or the tract's radiation, and choir.md 1.3 already finds the /a/ too open and front. Listen first.
- **Slow (attack 518 vs 102…149 ms; onset 279 vs 18…35 ms; phrase 108 ms)**: borderline FAULT. choir.md 1.2 sets `ampA` 0.25 s, slower than every font's choir pad.
- **Quieter (−3.1 vs +0.1…+2.5 dB)**: a trim. The early level window also catches the slow attack.
- Rank 24.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **level against the other instruments (velocity 96)**: quieter (-5.2 dB | 1.2 | 1.8 | 0.2 | 2.7), outside the fonts, 5.4 x.
2. **attack time (-20 to -1 dB)**: a slower attack (518 ms | 149 | 102 | 133 | 122), outside the fonts, 7.4 x.
3. **brightness (spectral centroid, velocity 96)**: brighter (18.6 ERB | 7.26 | 10.2 | 10.8 | 10.2), outside the fonts, 4.8 x.
4. **onset delay**: speaks later (279 ms | 35.0 | 19.0 | 18.0 | 21.0), outside the fonts, 25.9 x.
5. **partials 5+ share (tristimulus 3)**: stronger high partials (0.646 | 0.001 | 0.002 | 0.002 | 0.002), outside the fonts, 9.2 x.
6. **pitch wobble / vibrato (p-p)**: wobbles less (5.5 c | 21.6 | 28.4 | 32.9 | 33.2), outside the fonts, 4.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -34 | -18 | -6 | +7 | +12 | +10 |
| re TimGM6mb | -31 | -13 | -4 | +4 | +6 | +12 |
| re Jnsgm2 | -26 | -14 | -5 | +5 | +7 | +10 |
| re SGM | -30 | -19 | -9 | +0 | +5 | +23 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -5 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack +0.1 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 0.7 dB rms; TimGM6mb: centroid -0.8 ERB, attack +0.8 ERB, velocity brightening -0.4 ERB, loudness range +4 dB, envelope 2.0 dB rms; Jnsgm2: centroid -0.5 ERB, attack +0.2 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 2.3 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -1.09 | 0.59 | -1.21 | -0.33 | -0.09 | 1.0 |  |
| attack time (-20 to -1 dB) (ms) | 518 | 149 | 102 | 133 | 122 | 7.4 | yes |
| spectral shape of the attack, note for note | 14.7 (nearest 14) | fonts among themselves 9.7 | | | | 1.5 | yes |
| brightness over the keyboard (ERB/oct) | 1.85 | 3.26 | 4.38 | 3.69 | 3.45 | 3.1 | yes |
| key-to-key brightness scatter (ERB) | 1.23 | 0.3 | 0.61 | 0.612 | 0.721 | 3.0 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.76 | 0 | 0 | 0 | 0.03 | 2.5 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 18.6 | 7.26 | 10.2 | 10.8 | 10.2 | 4.8 | yes |
| onset delay (ms) | 279 | 35.0 | 19.0 | 18.0 | 21.0 | 25.9 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.46 | -4.31 | -2.33 | -2.67 | -2.76 | 0.3 |  |
| envelope curve, note for note (dB rms) | 7.42 (nearest 6.87) | fonts among themselves 5.3 | | | | 1.4 |  |
| pitch re 12-TET A440 (median) (c) | 1.7 | 1 | 1.3 | 1.9 | 1.9 | 0.0 |  |
| pitch scatter from key to key (c) | 2.1 | 8 | 1.8 | 1.3 | 0.8 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -27.2 | -37.5 | -42.4 | -29.3 | -37.1 | 1.5 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -59.2 | -39.0 | -64.5 | -64.3 | -62.0 | 0.3 |  |
| pitch at the start re settled (c) | 2.6 | -2.4 | 1.2 | 4.3 | 4.6 | 0.0 |  |
| stereo coherence | 0.389 | 1 | 1 | 1 | 1 | 8.1 | yes |
| partial-to-partial jaggedness (dB) | 3.65 | 5.7 | 7.8 | 4.7 | 5.1 | 1.1 | yes |
| dip between legato notes (dB) | 6.7 | 14.8 | 9.6 | 9.5 | 13.0 | 1.7 | yes |
| level evenness of a legato scale (dB) | 2 | 1.6 | 0.9 | 0.8 | 2.9 | 0.7 |  |
| level over the keyboard (dB/oct) | 0.32 | 0.64 | -0.38 | 0.24 | 0.16 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | -5.2 | 1.2 | 1.8 | 0.2 | 2.7 | 5.4 | yes |
| key-to-key level scatter (dB) | 1.3 | 0.96 | 0.33 | 0.33 | 0.54 | 1.7 | yes |
| odd / even partials (dB) | 0.1 | -10.3 | -11.8 | -8.5 | -8.1 | 4.8 | yes |
| onset jitter in a phrase (ms) | 50.5 | 29.6 | 46.7 | 36.5 | 29.0 | 2.0 | yes |
| onset in a phrase (re the note-on) (ms) | 98.0 | 16.0 | 75.0 | 66.0 | 45.0 | 1.4 | yes |
| onset sharpness (dB/ms) | 4 | 5.1 | 9.4 | 7.1 | 7 | 1.4 | yes |
| overtone levels, note for note (dB rms) | 21.8 (nearest 20.7) | fonts among themselves 7.77 | | | | 2.8 | yes |
| panning across the keys (dB/oct) | -0.48 | -0.21 | 0 | 0 | -0.01 | 0.5 |  |
| partial envelopes alike | 0.243 | 0.037 | -0.067 | -0.032 | 0.143 | 2.3 | yes |
| release time (30 dB after note-off) (ms) | 966 | 838 | 614 | 609 | 576 | 2.4 | yes |
| spectrogram (1 - correlation), note for note | 0.591 (nearest 0.423) | fonts among themselves 0.171 | | | | 3.5 | yes |
| spectral shape of the body, note for note | 16.8 (nearest 15.6) | fonts among themselves 9.25 | | | | 1.8 | yes |
| sounding length of a 100 ms note (ms) | 400 | 400 | 66.0 | 168 | 163 | 1.0 |  |
| partial stretch (inharmonicity) (c/partial) | -0.06 | -1.06 | -1.14 | -1.1 | -0.64 | 2.0 | yes |
| level after 2 s held, re peak (dB) | -6.2 | -6.9 | -8.2 | -7.4 | -9.5 | 0.5 |  |
| high bands lag low bands by (ms) | 8.8 | -20.0 | 6.3 | 16.3 | 16.8 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 20.1 | 5.1 | 10.0 | 13.6 | 10.8 | 1.9 | yes |
| fundamental share (tristimulus 1) | 0.028 | 0.919 | 0.771 | 0.191 | 0.204 | 1.3 | yes |
| partials 2-4 share (tristimulus 2) | 0.308 | 0.081 | 0.227 | 0.807 | 0.794 | 0.6 |  |
| partials 5+ share (tristimulus 3) | 0.646 | 0.001 | 0.002 | 0.002 | 0.002 | 9.2 | yes |
| loudness range, velocity 40 to 127 (dB) | 15.7 | 16.6 | 16.6 | 16.6 | 20.1 | 0.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 5.5 | 21.6 | 28.4 | 32.9 | 33.2 | 4.3 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0263 (nearest 0.0258) | fonts among themselves 0.0201 | | | | 1.3 |  |

</details>

### 93 Pad 5 (bowed): pad-bowed

Distance 2.55 (rank 80 of 129); the fonts disagree 1.2 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Slow (onset 270 vs 48…96 ms; attack 544 vs 146…243 ms)**: probable FAULT. The README pad table doesn't ask for a slow bow, and this is twice the slowest font.
- **Uneven (key scatter 2.7 vs 0.4…0.7 dB; legato scale 2.9 vs 0.7…1.5 dB)**: probably the fixed formant bank at 280/460/1150/2700 Hz meeting each key's harmonics differently. Partly DELIBERATE (a body), partly the room.
- **Quieter (−6.1 vs −3.0…+2.3 dB)**: a trim, inflated by the slow attack in the early window.
- **No vibrato (1.6 vs 8.5…31 c), partials moving as one (0.96)**: DELIBERATE, one core through filters. But a bowed pad would gain from some motion.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **onset delay**: speaks later (270 ms | 48.0 | 96.0 | 96.0 | 90.0), outside the fonts, 7.4 x.
2. **key-to-key level scatter**: less even from key to key (2.59 dB | 0.41 | 0.74 | 0.69 | 0.64), outside the fonts, 3.8 x.
3. **attack time (-20 to -1 dB)**: a slower attack (544 ms | 146 | 206 | 243 | 207), outside the fonts, 3.8 x.
4. **partial envelopes alike**: partials moving more as one (0.956 | 0.318 | -0.046 | -0.057 | 0.215), outside the fonts, 4.6 x.
5. **level against the other instruments (velocity 96)**: quieter (-5 dB | 2.4 | -2.7 | -2.6 | -0.7), outside the fonts, 1.3 x.
6. **brightening from velocity 40 to 127**: brightens more with velocity (0.52 ERB | 0.01 | 0 | 0 | -0.16), outside the fonts, 1.7 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -2 | +3 | -2 | -1 | +2 | - |
| re TimGM6mb | +1 | +4 | +2 | +0 | -5 | - |
| re Jnsgm2 | +2 | +5 | +3 | +1 | -5 | -7 |
| re SGM | -3 | -1 | -2 | +2 | +0 | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.3 ERB, attack -0.2 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 2.5 dB rms; TimGM6mb: centroid -1.8 ERB, attack -0.4 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 2.8 dB rms; Jnsgm2: centroid -1.4 ERB, attack -0.6 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 2.8 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.14 | 5.2 | 2.19 | 2.7 | 1.96 | 1.6 | yes |
| attack time (-20 to -1 dB) (ms) | 544 | 146 | 206 | 243 | 207 | 3.8 | yes |
| spectral shape of the attack, note for note | 11.9 (nearest 11.3) | fonts among themselves 10 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 2.83 | 2.78 | 3.17 | 3.21 | 2.85 | 0.6 |  |
| key-to-key brightness scatter (ERB) | 0.95 | 0.672 | 1.1 | 1.02 | 0.517 | 0.4 |  |
| brightening from velocity 40 to 127 (ERB) | 0.52 | 0.01 | 0 | 0 | -0.16 | 1.7 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 8.06 | 9.27 | 9.67 | 9.16 | 7.95 | 1.3 |  |
| onset delay (ms) | 270 | 48.0 | 96.0 | 96.0 | 90.0 | 7.4 | yes |
| Plomp-Levelt dissonance of a note (log2) | -4.68 | -5.44 | -2.26 | -2.47 | -3.44 | 1.1 |  |
| envelope curve, note for note (dB rms) | 6.36 (nearest 4.91) | fonts among themselves 6 | | | | 1.1 |  |
| pitch re 12-TET A440 (median) (c) | 0.3 | 0.8 | -0.3 | -0.5 | -0.6 | 0.2 |  |
| pitch scatter from key to key (c) | 0.3 | 1.6 | 3.7 | 3.3 | 3 | 1.0 |  |
| noisiness (spectral flatness) (dB) | -57.0 | -59.2 | -56.8 | -49.3 | -51.6 | 0.6 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -54.3 | -44.5 | -118 | -58.5 | -61.2 | 0.2 |  |
| pitch at the start re settled (c) | 0.3 | 2.1 | 3.3 | 1.8 | -1.3 | 0.3 |  |
| stereo coherence | 0.899 | 1 | 1 | 1 | 1 | 1.3 | yes |
| partial-to-partial jaggedness (dB) | 3.9 | 7 | 4.7 | 4.6 | 3.1 | 0.4 |  |
| dip between legato notes (dB) | 11.4 | 9.6 | 10.3 | 8.5 | 10.0 | 0.8 | yes |
| level evenness of a legato scale (dB) | 3.1 | 0.8 | 0.8 | 0.7 | 1.4 | 2.3 | yes |
| level over the keyboard (dB/oct) | -0.86 | -0.51 | -1.89 | -2.21 | 0.13 | 0.3 |  |
| level against the other instruments (velocity 96) (dB) | -5 | 2.4 | -2.7 | -2.6 | -0.7 | 1.3 | yes |
| key-to-key level scatter (dB) | 2.59 | 0.41 | 0.74 | 0.69 | 0.64 | 3.8 | yes |
| odd / even partials (dB) | -1.6 | -6.1 | -17.7 | -16.8 | -11.4 | 2.2 | yes |
| onset jitter in a phrase (ms) | 52.5 | 42.0 | 44.0 | 50.0 | 46.1 | 1.5 |  |
| onset in a phrase (re the note-on) (ms) | 70.0 | 0 | 98.0 | 64.0 | 99.0 | 0.2 |  |
| onset sharpness (dB/ms) | 4.3 | 3.4 | 4.3 | 4.6 | 4.9 | 0.1 |  |
| overtone levels, note for note (dB rms) | 12.6 (nearest 11.6) | fonts among themselves 6.9 | | | | 1.8 | yes |
| panning across the keys (dB/oct) | -0.8 | 0 | 0 | 0 | -0.01 | 0.8 | yes |
| partial envelopes alike | 0.956 | 0.318 | -0.046 | -0.057 | 0.215 | 4.6 | yes |
| release time (30 dB after note-off) (ms) | 784 | 938 | 465 | 451 | 448 | 1.5 |  |
| spectrogram (1 - correlation), note for note | 0.172 (nearest 0.149) | fonts among themselves 0.097 | | | | 1.8 | yes |
| spectral shape of the body, note for note | 10.8 (nearest 9.16) | fonts among themselves 7.8 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 262 | 396 | 391 | 388 | 400 | 2.3 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.01 | -0.7 | -1.1 | -0.62 | 0.3 | 0.9 |  |
| level after 2 s held, re peak (dB) | -4.2 | -7.5 | -7.8 | -8 | -12.2 | 1.2 | yes |
| high bands lag low bands by (ms) | 12.9 | -13.7 | -27.0 | -19.0 | -24.0 | 5.2 | yes |
| transient coherence: attack spread over bands (ms) | 7.2 | 21.7 | 15.5 | 13.4 | 12.1 | 1.4 | yes |
| fundamental share (tristimulus 1) | 0.722 | 0.81 | 0.772 | 0.816 | 0.887 | 1.3 | yes |
| partials 2-4 share (tristimulus 2) | 0.213 | 0.081 | 0.228 | 0.182 | 0.111 | 0.9 |  |
| partials 5+ share (tristimulus 3) | 0.015 | 0.049 | 0 | 0 | 0 | 0.2 |  |
| loudness range, velocity 40 to 127 (dB) | 16.2 | 16.6 | 16.7 | 16.7 | 20.1 | 0.3 |  |
| pitch wobble / vibrato (p-p) (c) | 1.6 | 8.5 | 31.2 | 31.1 | 31.3 | 2.6 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.017 (nearest 0.0136) | fonts among themselves 0.0125 | | | | 1.4 |  |

</details>

### 94 Pad 6 (metallic): pad-metallic

Distance 2.65 (rank 76 of 129); the fonts disagree 1.2 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Holds (decay 0.6 vs 4.1…8.7 dB/s; −6 vs −11…−19 dB after 2 s)**: DELIBERATE. Two of the four pipes are bowed and swell in over ~1 s (README pad table).
- **Speaks sooner and sharper (onset 44 vs 216…356 ms; 16 vs 6…8 dB/ms) but the full attack is slower (762 vs 449…498 ms)**: DELIBERATE: felt hammers strike two pipes, then the bowed ones swell.
- **Key scatter 3.5 vs 1.2…1.6 dB**: DELIBERATE. Each key's highest pipe has its own ratio (synth_leads.md "Bowing, the room, the per-key pipe").
- **Darker above 6 kHz re the TiMidity fonts (−13…−19 dB), −2 re SGM**: the fonts disagree. Not an issue.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **decay rate of a held note**: decays slower (0.6 dB/s | 8.7 | 5.8 | 4.8 | 4.4), outside the fonts, 6.4 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (1.17 ERB | -0 | 0 | -0 | -0.23), outside the fonts, 3.9 x.
3. **key-to-key level scatter**: less even from key to key (2.94 dB | 1.39 | 1.21 | 1.62 | 1.41), outside the fonts, 3.1 x.
4. **attack time (-20 to -1 dB)**: a slower attack (762 ms | 496 | 449 | 498 | 452), outside the fonts, 2.7 x.
5. **onset delay**: speaks sooner (44.0 ms | 356 | 268 | 216 | 240), outside the fonts, 3.0 x.
6. **onset sharpness**: a sharper onset (15.8 dB/ms | 7.8 | 5.9 | 7.3 | 5.8), outside the fonts, 4.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +5 | +6 | +8 | +7 | +3 | -19 |
| re TimGM6mb | +8 | +7 | +9 | +8 | -1 | -18 |
| re Jnsgm2 | +6 | +3 | +6 | +4 | +1 | -13 |
| re SGM | -0 | -2 | +0 | +3 | -1 | -2 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack +0.2 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 0.8 dB rms; TimGM6mb: centroid -0.0 ERB, attack +0.2 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 2.8 dB rms; Jnsgm2: centroid -0.3 ERB, attack +0.1 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 1.4 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.56 | -1.06 | -0.61 | 0.29 | -0.45 | 0.0 |  |
| attack time (-20 to -1 dB) (ms) | 762 | 496 | 449 | 498 | 452 | 2.7 | yes |
| spectral shape of the attack, note for note | 14.6 (nearest 11.3) | fonts among themselves 10.1 | | | | 1.5 |  |
| brightness over the keyboard (ERB/oct) | 3.79 | 4.53 | 4 | 3.29 | 3.15 | 0.2 |  |
| key-to-key brightness scatter (ERB) | 1.56 | 0.636 | 1.25 | 1.52 | 0.717 | 1.3 |  |
| brightening from velocity 40 to 127 (ERB) | 1.17 | -0 | 0 | -0 | -0.23 | 3.9 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 10.1 | 10.4 | 9.67 | 11.5 | 9.57 | 0.0 |  |
| decay rate of a held note (dB/s) | 0.6 | 8.7 | 5.8 | 4.8 | 4.4 | 6.4 | yes |
| onset delay (ms) | 44.0 | 356 | 268 | 216 | 240 | 3.0 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.66 | -4.68 | -3.72 | -2.89 | -3.7 | 0.1 |  |
| envelope curve, note for note (dB rms) | 10.3 (nearest 8.53) | fonts among themselves 4.57 | | | | 2.2 | yes |
| pitch re 12-TET A440 (median) (c) | 0.2 | 3.7 | 1.7 | 1.3 | 1.5 | 0.4 |  |
| pitch scatter from key to key (c) | 1.4 | 4.3 | 2.1 | 4 | 3.3 | 0.8 |  |
| noisiness (spectral flatness) (dB) | -58.5 | -51.6 | -44.8 | -41.9 | -43.3 | 3.0 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -45.0 | -57.3 | -68.5 | -66.3 | -65.3 | 3.5 | yes |
| pitch at the start re settled (c) | 1.2 | -2.9 | 2.1 | -3.9 | 5.2 | 0.3 |  |
| stereo coherence | 0.768 | 0.931 | 1 | 1 | 0.971 | 2.9 | yes |
| partial-to-partial jaggedness (dB) | 3.3 | 4.7 | 3 | 2.5 | 3.1 | 0.2 |  |
| dip between legato notes (dB) | 10.9 | 7.6 | 5.2 | 11.1 | 9.8 | 0.7 |  |
| level evenness of a legato scale (dB) | 1.7 | 2.7 | 1.1 | 2.9 | 1.4 | 0.3 |  |
| level over the keyboard (dB/oct) | -1.15 | 0.66 | -0.17 | -0.68 | -0.66 | 0.7 |  |
| level against the other instruments (velocity 96) (dB) | -1.4 | 0.1 | -2.8 | -5.6 | -2.7 | 0.5 |  |
| key-to-key level scatter (dB) | 2.94 | 1.39 | 1.21 | 1.62 | 1.41 | 3.1 | yes |
| odd / even partials (dB) | -2.6 | -13.7 | -3.1 | -3.8 | -4.9 | 0.3 |  |
| onset jitter in a phrase (ms) | 42.5 | 52.3 | 58.1 | 43.2 | 45.6 | 0.9 |  |
| onset in a phrase (re the note-on) (ms) | 40.0 | 10.0 | 64.0 | 65.0 | 14.0 | 0.0 |  |
| onset sharpness (dB/ms) | 15.8 | 7.8 | 5.9 | 7.3 | 5.8 | 4.6 | yes |
| overtone levels, note for note (dB rms) | 8.02 (nearest 7.56) | fonts among themselves 6.76 | | | | 1.2 |  |
| panning across the keys (dB/oct) | 0.32 | 0.6 | 0 | 0 | 0.16 | 0.2 |  |
| partial envelopes alike | -0.009 | 0.275 | 0.249 | 0.085 | 0.218 | 2.4 | yes |
| release time (30 dB after note-off) (ms) | 880 | 888 | 641 | 666 | 664 | 1.6 |  |
| spectrogram (1 - correlation), note for note | 0.179 (nearest 0.137) | fonts among themselves 0.152 | | | | 1.2 |  |
| spectral shape of the body, note for note | 12.9 (nearest 11.3) | fonts among themselves 8.73 | | | | 1.5 |  |
| sounding length of a 100 ms note (ms) | 260 | 204 | 250 | 153 | 306 | 0.4 |  |
| partial stretch (inharmonicity) (c/partial) | 0 | 0.23 | 0.02 | 0.41 | 0.27 | 0.5 |  |
| level after 2 s held, re peak (dB) | -6 | -19.0 | -11.3 | -12.6 | -11.9 | 1.6 | yes |
| high bands lag low bands by (ms) | 37.5 | 41.0 | -12.7 | -2 | -11.3 | 1.6 |  |
| transient coherence: attack spread over bands (ms) | 15.7 | 20.9 | 21.1 | 19.5 | 21.1 | 1.1 | yes |
| fundamental share (tristimulus 1) | 0.527 | 0.403 | 0.743 | 0.716 | 0.772 | 1.1 |  |
| partials 2-4 share (tristimulus 2) | 0.386 | 0.591 | 0.181 | 0.236 | 0.187 | 0.9 |  |
| partials 5+ share (tristimulus 3) | 0.058 | 0.008 | 0.062 | 0.044 | 0.03 | 0.3 |  |
| loudness range, velocity 40 to 127 (dB) | 15.5 | 16.7 | 16.6 | 16.7 | 20.1 | 0.7 | yes |
| pitch wobble / vibrato (p-p) (c) | 8.6 | 10.4 | 22.0 | 29.2 | 29.8 | 1.8 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0207 (nearest 0.0181) | fonts among themselves 0.0186 | | | | 1.1 |  |

</details>

### 95 Pad 7 (halo): pad-halo

Distance 3.97 (rank 22 of 129); the fonts disagree 0.9 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **A formant voice where the fonts have a dark near-sine (centroid 13.6 vs 7.5…8.6 ERB; fundamental share 0.31 vs 0.88…0.93)**: DELIBERATE: the five-formant cascade, whose compressors lift F3–F5 (synth_leads.md "pad-halo"). It is the farthest program from the fonts (rank 1). Consider a darker default (lower formant ranges or less lift) if GM files use Halo as a soft bed.
- **Brightness scatter 1.8 vs 0.3…0.6 ERB**: DELIBERATE: per-key Halton formants.
- **Slow (onset 422 vs 16…104 ms; phrase 130 ms)**: FAULT for GM use. The 1.5 s compressor integrators are the likely cause.
- **Quieter (−7.3 vs −1.3…+2.1 dB)**: FAULT (a trim). Part of it is the early window over the slow onset.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (12.2 ERB | 7.47 | 7.63 | 8.64 | 8.32), outside the fonts, 7.3 x.
2. **fundamental share (tristimulus 1)**: less fundamental (0.257 | 0.914 | 0.932 | 0.895 | 0.877), outside the fonts, 9.2 x.
3. **key-to-key brightness scatter**: less even in tone from key to key (1.81 ERB | 0.53 | 0.398 | 0.31 | 0.602), outside the fonts, 6.7 x.
4. **key-to-key level scatter**: less even from key to key (2.66 dB | 0.63 | 1.44 | 0.84 | 0.65), outside the fonts, 3.8 x.
5. **loudness range, velocity 40 to 127**: a narrower dynamic range (12.1 dB | 16.5 | 16.6 | 16.6 | 20.1), outside the fonts, 2.5 x.
6. **pitch wobble / vibrato (p-p)**: wobbles less (11.2 c | 33.7 | 31.5 | 32.9 | 26.9), outside the fonts, 4.2 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -3 | -2 | +6 | +5 | +8 | -12 |
| re TimGM6mb | -9 | -7 | +1 | -2 | +1 | +14 |
| re Jnsgm2 | -1 | -0 | +7 | +5 | +5 | -13 |
| re SGM | -5 | -4 | +3 | +1 | +4 | -4 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -10 -4 +0 +2; FluidR3 -12 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.1 ERB, attack -0.0 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 1.1 dB rms; TimGM6mb: centroid +0.3 ERB, attack -0.3 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 4.0 dB rms; Jnsgm2: centroid -0.1 ERB, attack +0.1 ERB, velocity brightening -0.4 ERB, loudness range +4 dB, envelope 3.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.35 | 1.02 | 2.07 | 0.12 | 0.14 | 1.0 | yes |
| attack time (-20 to -1 dB) (ms) | 515 | 580 | 474 | 415 | 62.0 | 0.1 |  |
| spectral shape of the attack, note for note | 11.2 (nearest 10.5) | fonts among themselves 9.81 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | 2.19 | 3.56 | 3.76 | 3.43 | 3.39 | 4.4 | yes |
| key-to-key brightness scatter (ERB) | 1.81 | 0.53 | 0.398 | 0.31 | 0.602 | 6.7 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.78 | 0.03 | -0 | -0 | -0.25 | 2.6 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 12.2 | 7.47 | 7.63 | 8.64 | 8.32 | 7.3 | yes |
| onset delay (ms) | 63.0 | 25.0 | 104 | 16.0 | 18.0 | 0.9 |  |
| Plomp-Levelt dissonance of a note (log2) | -2.52 | -3.31 | -3.4 | -3.21 | -3.32 | 2.6 | yes |
| envelope curve, note for note (dB rms) | 21.5 (nearest 6.03) | fonts among themselves 11.8 | | | | 1.8 |  |
| pitch re 12-TET A440 (median) (c) | 0.5 | -1.5 | 1.2 | -1.9 | -1.9 | 0.6 |  |
| pitch scatter from key to key (c) | 1.3 | 5.2 | 2.4 | 5.6 | 5.2 | 1.3 |  |
| noisiness (spectral flatness) (dB) | -39.0 | -42.8 | -47.7 | -31.8 | -35.6 | 0.0 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -30.7 | -32.1 | -126 | -124 | -110 | 1.8 |  |
| pitch at the start re settled (c) | -0.6 | 0.8 | -9.5 | 1.2 | 2.3 | 0.3 |  |
| stereo coherence | 0.524 | 0.775 | 1 | 1 | 0.987 | 4.2 | yes |
| partial-to-partial jaggedness (dB) | 4.3 | 1.4 | 1.2 | 1.2 | 1.1 | 3.1 | yes |
| dip between legato notes (dB) | 9.7 | 13.5 | 11.8 | 11.0 | 8 | 0.6 |  |
| level evenness of a legato scale (dB) | 2.2 | 2.2 | 1.1 | 2 | 2.9 | 0.1 |  |
| level over the keyboard (dB/oct) | -0.23 | 1 | -0.51 | 0.13 | -0.19 | 0.2 |  |
| level against the other instruments (velocity 96) (dB) | 0.5 | -1.2 | 1.7 | 0.9 | 2.3 | 0.5 |  |
| key-to-key level scatter (dB) | 2.66 | 0.63 | 1.44 | 0.84 | 0.65 | 3.8 | yes |
| odd / even partials (dB) | -1.5 | -5.7 | -3.3 | -4 | -5.2 | 1.5 | yes |
| onset jitter in a phrase (ms) | 40.1 | 31.2 | 45.6 | 38.9 | 33.9 | 0.5 |  |
| onset in a phrase (re the note-on) (ms) | 54.0 | 64.0 | 4 | 65.0 | 56.0 | 0.2 |  |
| onset sharpness (dB/ms) | 4.5 | 6 | 5.8 | 5.7 | 6 | 0.7 | yes |
| overtone levels, note for note (dB rms) | 10.7 (nearest 10.3) | fonts among themselves 4.64 | | | | 2.3 | yes |
| panning across the keys (dB/oct) | 0.27 | 0.36 | -0.05 | -0.02 | 0.06 | 0.2 |  |
| partial envelopes alike | 0.035 | -0.038 | -0.047 | -0.032 | 0.069 | 0.7 |  |
| release time (30 dB after note-off) (ms) | 1541 | 1278 | 381 | 353 | 333 | 2.1 | yes |
| spectrogram (1 - correlation), note for note | 0.24 (nearest 0.213) | fonts among themselves 0.181 | | | | 1.3 |  |
| spectral shape of the body, note for note | 9.99 (nearest 9.28) | fonts among themselves 8.7 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 400 | 231 | 376 | 65.0 | 209 | 0.7 |  |
| partial stretch (inharmonicity) (c/partial) | 0.01 | -0.46 | -0.16 | -0.18 | 0.39 | 0.4 |  |
| level after 2 s held, re peak (dB) | -6.7 | -10.4 | -10.5 | -8.5 | -9.5 | 1.1 | yes |
| high bands lag low bands by (ms) | 2.6 | -2 | -11.3 | 0.8 | 1.3 | 0.5 |  |
| transient coherence: attack spread over bands (ms) | 12.2 | 5 | 14.9 | 2.3 | 1.9 | 1.3 |  |
| fundamental share (tristimulus 1) | 0.257 | 0.914 | 0.932 | 0.895 | 0.877 | 9.2 | yes |
| partials 2-4 share (tristimulus 2) | 0.181 | 0.083 | 0.056 | 0.086 | 0.123 | 1.4 | yes |
| partials 5+ share (tristimulus 3) | 0.131 | 0.004 | 0.007 | 0.014 | 0.008 | 1.8 | yes |
| loudness range, velocity 40 to 127 (dB) | 12.1 | 16.5 | 16.6 | 16.6 | 20.1 | 2.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 11.2 | 33.7 | 31.5 | 32.9 | 26.9 | 4.2 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0223 (nearest 0.0215) | fonts among themselves 0.0155 | | | | 1.4 | yes |

</details>

### 96 Pad 8 (sweep): pad-sweep

Distance 1.80 (rank 112 of 129); the fonts disagree 1.8 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Key scatter (4.4 vs 0.4…2.2 dB), brightness scatter 1.3 ERB**: DELIBERATE. The 0.12 Hz phaser runs free, so each key starts at another phase of the sweep. SGM shows the same at 2.2 dB.
- **Less vibrato (12.5 vs 17…31 c), smoother**: the default vibrato knob (~42/127; synth_leads.md §10). Minor.
- **Brightens with velocity (+0.8 ERB)**: the dynamics knob.
- Rank 113, and the fonts disagree (1.8 JND): about as close as a sweep pad gets.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (4.81 dB | 0.39 | 1.01 | 0.73 | 2.18), outside the fonts, 4.4 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (0.78 ERB | 0.04 | -0 | 0 | -0.16), outside the fonts, 2.6 x.
3. **level against the other instruments (velocity 96)**: quieter (-8.1 dB | -6.8 | -2.2 | -5.4 | -1.3), outside the fonts, 1.6 x.
4. **key-to-key brightness scatter**: less even in tone from key to key (1.29 ERB | 0.367 | 0.993 | 0.449 | 0.542), outside the fonts, 2.5 x.
5. **pitch wobble / vibrato (p-p)**: wobbles less (12.5 c | 28.3 | 31.0 | 27.0 | 17.1), outside the fonts, 2.2 x.
6. **Plomp-Levelt dissonance of a note**: smoother (-6.38 log2 | -3.01 | -2.35 | -3.86 | -5.92), outside the fonts, 1.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +1 | +11 | +5 | +1 | -13 | -7 |
| re TimGM6mb | +1 | +11 | +3 | +0 | -10 | -11 |
| re Jnsgm2 | -5 | +2 | -1 | +2 | -1 | - |
| re SGM | -9 | +2 | -2 | +2 | +3 | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -14 -5 +0 +5; FluidR3 -12 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -17 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.2 ERB, attack -0.3 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 2.8 dB rms; TimGM6mb: centroid -1.7 ERB, attack -0.1 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 4.8 dB rms; Jnsgm2: centroid -0.4 ERB, attack -0.3 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 5.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.54 | 1.45 | 0.12 | -0.05 | 0.07 | 0.8 | yes |
| attack time (-20 to -1 dB) (ms) | 700 | 1058 | 924 | 384 | 738 | 0.3 |  |
| spectral shape of the attack, note for note | 10.4 (nearest 8.22) | fonts among themselves 9.1 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | 3.17 | 4.67 | 3.45 | 3.06 | 3.23 | 0.2 |  |
| key-to-key brightness scatter (ERB) | 1.29 | 0.367 | 0.993 | 0.449 | 0.542 | 2.5 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.78 | 0.04 | -0 | 0 | -0.16 | 2.6 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 8.78 | 9.63 | 9.08 | 7.48 | 7.17 | 0.4 |  |
| onset delay (ms) | 382 | 345 | 173 | 116 | 286 | 1.3 | yes |
| Plomp-Levelt dissonance of a note (log2) | -6.38 | -3.01 | -2.35 | -3.86 | -5.92 | 1.6 | yes |
| envelope curve, note for note (dB rms) | 10.7 (nearest 7.29) | fonts among themselves 10.5 | | | | 1.0 |  |
| pitch re 12-TET A440 (median) (c) | 0.7 | -1.1 | -3.3 | -10.1 | -6.3 | 1.2 |  |
| pitch scatter from key to key (c) | 6.6 | 10.5 | 3.1 | 8.5 | 5.6 | 0.1 |  |
| noisiness (spectral flatness) (dB) | -71.4 | -46.2 | -42.7 | -72.9 | -71.0 | 0.8 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -50.5 | -35.0 | -123 | -117 | -98.8 | 1.3 |  |
| pitch at the start re settled (c) | 1.4 | -12.8 | -0.5 | 6.6 | -7.8 | 0.6 |  |
| stereo coherence | 0.843 | 0.102 | 0.983 | 1 | 1 | 0.3 |  |
| partial-to-partial jaggedness (dB) | 2.8 | 1.5 | 1.5 | 0.5 | 1 | 1.5 | yes |
| dip between legato notes (dB) | 10.0 | 12.1 | 9.5 | 17.4 | 11.7 | 0.5 |  |
| level evenness of a legato scale (dB) | 3.4 | 3 | 0.8 | 1.6 | 1.2 | 1.8 |  |
| level over the keyboard (dB/oct) | -0.66 | 3.43 | -0.11 | -2.77 | -5.12 | 0.2 |  |
| level against the other instruments (velocity 96) (dB) | -8.1 | -6.8 | -2.2 | -5.4 | -1.3 | 1.6 | yes |
| key-to-key level scatter (dB) | 4.81 | 0.39 | 1.01 | 0.73 | 2.18 | 4.4 | yes |
| odd / even partials (dB) | -10.8 | -2.6 | -4.7 | -7.5 | -8.3 | 1.6 | yes |
| onset jitter in a phrase (ms) | 51.1 | 48.6 | 46.0 | 42.2 | 55.7 | 0.6 |  |
| onset in a phrase (re the note-on) (ms) | 42.0 | 28.0 | 4 | 60.0 | 29.0 | 0.5 |  |
| onset sharpness (dB/ms) | 7.7 | 6.4 | 6.9 | 6.6 | 6.6 | 0.6 |  |
| overtone levels, note for note (dB rms) | 11.2 (nearest 7.65) | fonts among themselves 8.8 | | | | 1.3 |  |
| panning across the keys (dB/oct) | 0.1 | 0.11 | 0.06 | 0 | -0.03 | 0.1 |  |
| partial envelopes alike | 0.184 | 0.024 | -0.092 | -0.013 | 0.651 | 0.5 |  |
| release time (30 dB after note-off) (ms) | 866 | 999 | 426 | 639 | 470 | 1.1 |  |
| spectrogram (1 - correlation), note for note | 0.183 (nearest 0.141) | fonts among themselves 0.163 | | | | 1.1 |  |
| spectral shape of the body, note for note | 9.94 (nearest 7.16) | fonts among themselves 8.73 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 286 | 289 | 282 | 274 | 292 | 0.0 |  |
| partial stretch (inharmonicity) (c/partial) | -0.35 | -0.01 | 0.49 | -0.81 | -1.12 | 0.1 |  |
| level after 2 s held, re peak (dB) | -9.6 | -6.7 | -8.7 | -11.4 | -5.4 | 0.6 |  |
| high bands lag low bands by (ms) | 20.0 | -6.2 | 27.0 | -19.0 | -2.5 | 1.1 |  |
| transient coherence: attack spread over bands (ms) | 16.4 | 25.4 | 16.7 | 25.3 | 6 | 0.5 |  |
| fundamental share (tristimulus 1) | 0.707 | 0.755 | 0.859 | 0.874 | 0.969 | 1.5 | yes |
| partials 2-4 share (tristimulus 2) | 0.261 | 0.234 | 0.116 | 0.118 | 0.03 | 1.4 |  |
| partials 5+ share (tristimulus 3) | 0.001 | 0.015 | 0.018 | 0.001 | 0 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 18.5 | 16.3 | 16.7 | 16.7 | 21.7 | 0.7 |  |
| pitch wobble / vibrato (p-p) (c) | 12.5 | 28.3 | 31.0 | 27.0 | 17.1 | 2.2 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0158 (nearest 0.0127) | fonts among themselves 0.0169 | | | | 0.9 |  |

</details>

### 97 FX 1 (rain): fx-rain

Distance 1.77 (rank 114 of 129); the fonts disagree 1.9 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Key scatter (3.1 vs 0.2…0.5 dB), brightness scatter 1.9 ERB**: DELIBERATE: each note gets its own random rain (README "FX 1 (rain)"; fx_sounds.md FX 1 comparison).
- **Darker when harder (−1.0 vs 0 ERB)**: DELIBERATE: large drops have a longer contact.
- **Noisier (flatness −28 vs −37…−46 dB), wide (coherence 0.47)**: DELIBERATE: the bubbles above 6 kHz; random drops plus the room.
- **Sharp onset (40 dB/ms)**: a drop on glass. SGM is 49, the TiMidity fonts 3…7: the fonts disagree (1.9 JND).
- Rank 105: close. The decay and release match (fx_sounds.md).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (3.27 dB | 0.54 | 0.25 | 0.19 | 0.4), outside the fonts, 5.9 x.
2. **brightening from velocity 40 to 127**: brightens less with velocity (-1.06 ERB | -0 | 0.03 | 0 | -0.17), outside the fonts, 3.5 x.
3. **stereo coherence [room]**: wider (0.406 | 0.833 | 1 | 1 | 1), outside the fonts, 7.1 x.
4. **noisiness (spectral flatness)**: noisier (-26.0 dB | -37.5 | -45.0 | -36.8 | -45.8), outside the fonts, 3.4 x.
5. **attack time (-20 to -1 dB)**: a slower attack (34.0 ms | 13.0 | 15.0 | 29.0 | 25.0), outside the fonts, 1.4 x.
6. **key-to-key brightness scatter**: less even in tone from key to key (1.63 ERB | 0.362 | 1.51 | 0.896 | 0.624), outside the fonts, 1.5 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -1 | -3 | +2 | -1 | -1 | +2 |
| re TimGM6mb | +7 | +6 | +12 | +3 | -6 | -12 |
| re Jnsgm2 | +0 | -3 | +1 | -2 | -6 | +7 |
| re SGM | -7 | -10 | -4 | -1 | +4 | +9 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -5 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -12 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.3 ERB, attack -0.4 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 2.4 dB rms; TimGM6mb: centroid -0.4 ERB, attack +0.3 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 3.0 dB rms; Jnsgm2: centroid -1.4 ERB, attack -0.2 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 2.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.74 | 0.64 | -3.04 | 1.53 | 1.26 | 0.7 |  |
| attack time (-20 to -1 dB) (ms) | 34.0 | 13.0 | 15.0 | 29.0 | 25.0 | 1.4 | yes |
| spectral shape of the attack, note for note | 15.2 (nearest 14.1) | fonts among themselves 13 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 3.53 | 3.57 | 6.25 | 3.47 | 3.05 | 0.0 |  |
| key-to-key brightness scatter (ERB) | 1.63 | 0.362 | 1.51 | 0.896 | 0.624 | 1.5 | yes |
| brightening from velocity 40 to 127 (ERB) | -1.06 | -0 | 0.03 | 0 | -0.17 | 3.5 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 10.9 | 7.97 | 13.6 | 8.99 | 7.58 | 0.8 |  |
| decay rate of a held note (dB/s) | 4.5 | 6 | 0.5 | 7 | 7.1 | 0.3 |  |
| onset delay (ms) | 10.0 | 4 | 2 | 1 | 3 | 0.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.26 | -4.02 | -2.47 | -1.92 | -3.44 | 0.3 |  |
| envelope curve, note for note (dB rms) | 9.17 (nearest 8.5) | fonts among themselves 12.4 | | | | 0.7 |  |
| pitch re 12-TET A440 (median) (c) | -0.1 | -1.7 | 0.5 | -0.6 | -1.7 | 0.3 |  |
| pitch scatter from key to key (c) | 2.1 | 8 | 3.3 | 3.3 | 2.1 | 0.4 |  |
| noisiness (spectral flatness) (dB) | -26.0 | -37.5 | -45.0 | -36.8 | -45.8 | 3.4 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -77.0 | -123 | -26.6 | -120 | -92.8 | 0.6 |  |
| pitch at the start re settled (c) | 1.3 | -0.5 | 2.5 | 2.1 | -1.9 | 0.1 |  |
| stereo coherence | 0.406 | 0.833 | 1 | 1 | 1 | 7.1 | yes |
| partial-to-partial jaggedness (dB) | 7.95 | 0.5 | 18.5 | 1.3 | 1.2 | 0.7 |  |
| dip between legato notes (dB) | 8.9 | 12.9 | 8.1 | 10.4 | 10.4 | 0.6 |  |
| level evenness of a legato scale (dB) | 2.5 | 1.2 | 1.2 | 1 | 1.6 | 1.3 | yes |
| level over the keyboard (dB/oct) | -0.61 | 0.48 | -1.08 | -0.13 | -0.06 | 0.5 |  |
| level against the other instruments (velocity 96) (dB) | 0.8 | 1.1 | 0.3 | -2.1 | -0.1 | 0.4 |  |
| key-to-key level scatter (dB) | 3.27 | 0.54 | 0.25 | 0.19 | 0.4 | 5.9 | yes |
| odd / even partials (dB) | 0.1 | -4.4 | -0.7 | -2.7 | -6.3 | 1.3 |  |
| onset jitter in a phrase (ms) | 37.5 | 47.3 | 36.5 | 36.8 | 39.8 | 0.1 |  |
| onset in a phrase (re the note-on) (ms) | 23.0 | -1 | 0 | 77.0 | 48.0 | 0.0 |  |
| onset sharpness (dB/ms) | 41.0 | 3.3 | 6.5 | 3.9 | 48.8 | 1.6 |  |
| overtone levels, note for note (dB rms) | 17.7 (nearest 13.1) | fonts among themselves 17.6 | | | | 1.0 |  |
| panning across the keys (dB/oct) | 0.71 | 1.12 | 0 | 0 | -0.01 | 0.7 |  |
| partial envelopes alike | 0.048 | 0.743 | 0.065 | 0.336 | 0.545 | 1.2 |  |
| release time (30 dB after note-off) (ms) | 626 | 654 | 1841 | 520 | 394 | 0.1 |  |
| spectrogram (1 - correlation), note for note | 0.263 (nearest 0.236) | fonts among themselves 0.263 | | | | 1.0 |  |
| spectral shape of the body, note for note | 12 (nearest 10.4) | fonts among themselves 13.3 | | | | 0.9 |  |
| sounding length of a 100 ms note (ms) | 327 | 112 | 376 | 269 | 135 | 0.9 |  |
| partial stretch (inharmonicity) (c/partial) | 0.19 | 0 | 0.02 | 0.08 | 0.07 | 0.3 |  |
| level after 2 s held, re peak (dB) | -16.9 | -15.2 | -7 | -20.4 | -20.4 | 0.1 |  |
| high bands lag low bands by (ms) | 26.0 | 11.0 | 0 | 23.8 | 9.5 | 1.3 |  |
| transient coherence: attack spread over bands (ms) | 10.1 | 3.5 | 0 | 15.1 | 3.2 | 0.9 |  |
| fundamental share (tristimulus 1) | 0.802 | 0.75 | 0.47 | 0.818 | 0.935 | 0.1 |  |
| partials 2-4 share (tristimulus 2) | 0.198 | 0.24 | 0.281 | 0.173 | 0.064 | 0.1 |  |
| partials 5+ share (tristimulus 3) | 0.003 | 0.018 | 0.247 | 0.016 | 0.001 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 16.1 | 16.6 | 16.0 | 16.7 | 20.1 | 0.3 |  |
| pitch wobble / vibrato (p-p) (c) | 16.8 | 10.6 | 14.3 | 29.1 | 31.3 | 0.5 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0327 (nearest 0.0255) | fonts among themselves 0.0302 | | | | 1.1 |  |

</details>

### 98 FX 2 (soundtrack): fx-soundtrack

Distance 5.44 (rank 1 of 129); the fonts disagree 1.0 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Quieter (−23 vs −1.8…+2.6 dB) and late (onset 778 vs 107…238 ms)**: the level is read in the first ~0.3–0.5 s, before the strings bloom (~0.8 s), so the number is mostly a MEASUREMENT ARTIFACT. The slowness behind it is a known limitation (fx_sounds.md FX 2 "Limitations": fast passages sound late and soft; K2 speeds it). For GM files, a faster default swell is worth considering.
- **Narrow velocity range (7.6 vs 16…20 dB); uneven legato scale (7.5 vs 1.2…3 dB)**: the same cause. Held for 6 s it spans 25 dB (fx_sounds.md).
- **Smoother, partials moving as one (0.83 vs ~0); key scatter 2.9 dB**: DELIBERATE: locked strings that beat.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **level against the other instruments (velocity 96)**: quieter (-19.6 dB | -1.8 | 0.2 | 1.1 | 2.8), outside the fonts, 8.8 x.
2. **loudness range, velocity 40 to 127**: a narrower dynamic range (5.3 dB | 16.4 | 16.6 | 16.6 | 20.5), outside the fonts, 5.5 x.
3. **onset delay**: speaks later (804 ms | 238 | 107 | 113 | 115), outside the fonts, 10.5 x.
4. **key-to-key level scatter**: less even from key to key (3.29 dB | 0.24 | 0.4 | 0.23 | 0.92), outside the fonts, 5.9 x.
5. **Plomp-Levelt dissonance of a note**: smoother (-5.35 log2 | -2.17 | -1.22 | -1.27 | -1.88), outside the fonts, 7.9 x.
6. **brightness (spectral centroid, velocity 96)**: darker (8.11 ERB | 9.09 | 10.3 | 10.2 | 9.74), outside the fonts, 2.9 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +5 | +3 | -1 | -7 | -6 | +7 |
| re TimGM6mb | +0 | -2 | -7 | -16 | -10 | +28 |
| re Jnsgm2 | +6 | +3 | -2 | -11 | -6 | +12 |
| re SGM | -0 | -3 | -8 | -14 | -3 | +22 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -6 -5 +0 -1; FluidR3 -12 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -16 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.8 ERB, attack -0.4 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 2.4 dB rms; TimGM6mb: centroid -0.1 ERB, attack -0.7 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 3.0 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.6 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 2.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.37 | 1.43 | 0.07 | 0.15 | -0.67 | 0.5 |  |
| attack time (-20 to -1 dB) (ms) | 855 | 565 | 350 | 625 | 528 | 1.5 | yes |
| spectral shape of the attack, note for note | 15.7 (nearest 14.9) | fonts among themselves 7.62 | | | | 2.1 | yes |
| brightness over the keyboard (ERB/oct) | 3.47 | 3.64 | 3.67 | 3.63 | 3.35 | 0.5 |  |
| key-to-key brightness scatter (ERB) | 0.529 | 0.298 | 0.529 | 0.591 | 0.683 | 0.2 |  |
| brightening from velocity 40 to 127 (ERB) | 0.31 | 0.01 | 0 | 0 | 1.09 | 0.6 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 8.11 | 9.09 | 10.3 | 10.2 | 9.74 | 2.9 | yes |
| onset delay (ms) | 804 | 238 | 107 | 113 | 115 | 10.5 | yes |
| Plomp-Levelt dissonance of a note (log2) | -5.35 | -2.17 | -1.22 | -1.27 | -1.88 | 7.9 | yes |
| envelope curve, note for note (dB rms) | 11.3 (nearest 10.4) | fonts among themselves 3.29 | | | | 3.4 | yes |
| pitch re 12-TET A440 (median) (c) | -0.3 | 11.2 | -5.3 | -4.3 | -3.2 | 0.4 |  |
| pitch scatter from key to key (c) | 2.4 | 7 | 2.7 | 3 | 4.5 | 0.5 |  |
| noisiness (spectral flatness) (dB) | -33.1 | -37.3 | -37.8 | -32.7 | -41.2 | 1.0 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -45.6 | -38.1 | -40.0 | -40.5 | -43.8 | 0.9 |  |
| pitch at the start re settled (c) | 0.6 | -11.2 | 1.7 | 1.7 | -0.9 | 0.0 |  |
| stereo coherence | 0.48 | 0.521 | 0.923 | 0.922 | 0.953 | 2.0 | yes |
| partial-to-partial jaggedness (dB) | 3.6 | 2.1 | 2.9 | 2.7 | 3.4 | 0.8 |  |
| dip between legato notes (dB) | 18.3 | 11.1 | 10.5 | 11.8 | 9.8 | 3.8 | yes |
| level evenness of a legato scale (dB) | 8.3 | 2.5 | 1.2 | 1.2 | 3 | 6.5 | yes |
| level over the keyboard (dB/oct) | 1.64 | 2.48 | 0.28 | -0.16 | -0.03 | 1.1 |  |
| level against the other instruments (velocity 96) (dB) | -19.6 | -1.8 | 0.2 | 1.1 | 2.8 | 8.8 | yes |
| key-to-key level scatter (dB) | 3.29 | 0.24 | 0.4 | 0.23 | 0.92 | 5.9 | yes |
| odd / even partials (dB) | -2.2 | -2.7 | -0.6 | -0.9 | -1.1 | 0.6 |  |
| onset jitter in a phrase (ms) | 48.6 | 61.5 | 48.8 | 34.8 | 56.4 | 0.3 |  |
| onset in a phrase (re the note-on) (ms) | 82.0 | 67.0 | 65.0 | 71.0 | 66.0 | 1.6 | yes |
| onset sharpness (dB/ms) | 8.4 | 9.4 | 7.7 | 7.2 | 9.1 | 0.0 |  |
| overtone levels, note for note (dB rms) | 24.5 (nearest 21.2) | fonts among themselves 6.13 | | | | 4.0 | yes |
| panning across the keys (dB/oct) | -0.22 | -0.51 | 0.08 | -0.04 | 0 | 0.2 |  |
| partial envelopes alike | 0.871 | 0.059 | -0.06 | -0.051 | -0.032 | 9.1 | yes |
| release time (30 dB after note-off) (ms) | 868 | 1170 | 1069 | 1071 | 729 | 0.9 |  |
| spectrogram (1 - correlation), note for note | 0.405 (nearest 0.364) | fonts among themselves 0.0946 | | | | 4.3 | yes |
| spectral shape of the body, note for note | 16 (nearest 14.5) | fonts among themselves 6.84 | | | | 2.3 | yes |
| sounding length of a 100 ms note (ms) | 320 | 327 | 400 | 333 | 227 | 0.1 |  |
| partial stretch (inharmonicity) (c/partial) | -0.19 | -0.44 | 0.57 | 0.3 | 0.37 | 1.0 |  |
| level after 2 s held, re peak (dB) | -4.8 | -9.1 | -6.9 | -7.5 | -8.9 | 1.1 | yes |
| high bands lag low bands by (ms) | 7.3 | 3.5 | 22.0 | 12.7 | 48.5 | 0.4 |  |
| transient coherence: attack spread over bands (ms) | 12.8 | 18.1 | 17.6 | 20.1 | 22.5 | 1.3 | yes |
| fundamental share (tristimulus 1) | 0.989 | 0.843 | 0.732 | 0.753 | 0.829 | 2.8 | yes |
| partials 2-4 share (tristimulus 2) | 0.011 | 0.136 | 0.221 | 0.213 | 0.136 | 2.3 | yes |
| partials 5+ share (tristimulus 3) | 0 | 0.006 | 0.038 | 0.043 | 0.015 | 0.4 |  |
| loudness range, velocity 40 to 127 (dB) | 5.3 | 16.4 | 16.6 | 16.6 | 20.5 | 5.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 11.6 | 30.4 | 29.6 | 29.1 | 30.4 | 3.7 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0175 (nearest 0.0151) | fonts among themselves 0.0137 | | | | 1.3 |  |

</details>

### 99 FX 3 (crystal): fx-crystal

Distance 3.21 (rank 41 of 129); the fonts disagree 1.8 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Pitch wobble 32 vs 0…15 c**: MEASUREMENT ARTIFACT. The beating 1/1.022/1.164 cluster fools the tracker (synth_leads.md "FX 3").
- **Fundamental share 0.38 vs 0.83…1.0; −17…−26 dB under 150 Hz**: the plate's inharmonic modes. The harmonic measures are meaningless here, and the fonts' near-sine crystals are a different idea. DELIBERATE.
- **Brightens with velocity (+2.2 ERB), key scatter 2.4 vs 0.04…0.6 dB**: DELIBERATE: the felt hardens, and the strikes land at random places.
- **Sustains more (−20 vs −23…−33 dB after 2 s)**: DELIBERATE: the bow keeps it singing.
- The 1.164 mode carries 17 % of the energy, a cluster ~2.6 semitones up. Listen for whether the note reads clearly.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (2.19 ERB | 0 | 0 | 0 | -0.06), outside the fonts, 7.3 x.
2. **fundamental share (tristimulus 1)**: less fundamental (0.374 | 0.834 | 0.999 | 0.998 | 0.948), outside the fonts, 7.3 x.
3. **key-to-key level scatter**: less even from key to key (2.36 dB | 0.04 | 0.13 | 0.06 | 0.61), outside the fonts, 4.5 x.
4. **partials 2-4 share (tristimulus 2)**: stronger low partials (0.567 | 0.095 | 0 | 0 | 0), outside the fonts, 8.1 x.
5. **pitch wobble / vibrato (p-p)**: wobbles more (32.0 c | 0 | 2.6 | 0.6 | 15.4), outside the fonts, 3.9 x.
6. **onset in a phrase (re the note-on)**: speaks later in a phrase (72.0 ms | -1 | -0 | -4 | 40.0), outside the fonts, 3.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -26 | +2 | +3 | +4 | +5 | -6 |
| re TimGM6mb | -20 | +5 | +6 | +9 | -2 | -20 |
| re Jnsgm2 | -26 | +3 | +6 | +10 | +6 | -18 |
| re SGM | -17 | +13 | +16 | +14 | +1 | -25 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.9 ERB, attack +1.4 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 9.6 dB rms; TimGM6mb: centroid -0.8 ERB, attack -0.3 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 1.9 dB rms; Jnsgm2: centroid -0.0 ERB, attack +0.0 ERB, velocity brightening -0.5 ERB, loudness range +4 dB, envelope 1.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.99 | 0 | 3.91 | 0.19 | 0.48 | 0.3 |  |
| attack time (-20 to -1 dB) (ms) | 45.0 | 1 | 6 | 34.0 | 35.0 | 0.6 | yes |
| spectral shape of the attack, note for note | 16.8 (nearest 13.6) | fonts among themselves 16.1 | | | | 1.0 |  |
| brightness over the keyboard (ERB/oct) | 3.32 | 3.2 | 3.5 | 3.6 | 3.33 | 0.3 |  |
| key-to-key brightness scatter (ERB) | 1.42 | 0.189 | 0.3 | 0.228 | 1.55 | 1.7 |  |
| brightening from velocity 40 to 127 (ERB) | 2.19 | 0 | 0 | 0 | -0.06 | 7.3 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 9.95 | 8.47 | 10.8 | 7.57 | 14.2 | 0.1 |  |
| decay rate of a held note (dB/s) | 7.6 | 16.6 | 15.5 | 11.5 | 7.3 | 1.4 |  |
| onset delay (ms) | 20.0 | 0 | 1 | 5 | 29.0 | 1.2 |  |
| Plomp-Levelt dissonance of a note (log2) | -2.03 | -6.39 | -3.08 | -4.64 | -3.64 | 1.3 | yes |
| envelope curve, note for note (dB rms) | 11.8 (nearest 9.28) | fonts among themselves 6.65 | | | | 1.8 | yes |
| pitch re 12-TET A440 (median) (c) | -0.3 | 3.8 | 3.8 | 13.9 | 3 | 0.8 | yes |
| pitch scatter from key to key (c) | 4.7 | 0.9 | 0.9 | 0.4 | 4.6 | 1.3 |  |
| noisiness (spectral flatness) (dB) | -57.5 | -60.7 | -56.5 | -55.2 | -60.4 | 0.3 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -70.7 | -106 | -119 | -126 | -65.7 | 1.4 |  |
| pitch at the start re settled (c) | -0.5 | 0 | 1.8 | -2.5 | 3.9 | 0.3 |  |
| stereo coherence | 0.251 | 1 | 0.984 | 0.999 | 0.572 | 3.5 | yes |
| partial-to-partial jaggedness (dB) | 10.7 | 31.6 | 10.0 | 7.8 | 11.8 | 0.0 |  |
| dip between legato notes (dB) | 12.2 | 15.0 | 10.1 | 14.6 | 7.1 | 0.0 |  |
| level evenness of a legato scale (dB) | 3 | 2.1 | 0.4 | 1.4 | 0.8 | 1.9 | yes |
| level over the keyboard (dB/oct) | 0.14 | -0.02 | 1.1 | 0.46 | -0.82 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | -1.2 | -7.8 | 0 | 6.3 | -1.7 | 0.0 |  |
| key-to-key level scatter (dB) | 2.36 | 0.04 | 0.13 | 0.06 | 0.61 | 4.5 | yes |
| odd / even partials (dB) | -24.8 | 48.6 | 0.2 | 4.7 | 10.4 | 1.3 | yes |
| onset jitter in a phrase (ms) | 48.3 | 54.0 | 28.4 | 40.1 | 46.5 | 0.4 |  |
| onset in a phrase (re the note-on) (ms) | 72.0 | -1 | -0 | -4 | 40.0 | 3.3 | yes |
| onset sharpness (dB/ms) | 29.5 | 0.6 | 3.7 | 5.1 | 25.2 | 2.0 | yes |
| overtone levels, note for note (dB rms) | 25.5 (nearest 23.9) | fonts among themselves 22.2 | | | | 1.1 |  |
| panning across the keys (dB/oct) | -0.05 | 0 | -0.13 | -0 | -0.02 | 0.0 |  |
| partial envelopes alike | 0.784 | 0.946 | 0.836 | 0.828 | 0.565 | 0.3 |  |
| release time (30 dB after note-off) (ms) | 1100 | 1484 | 748 | 846 | 998 | 0.5 |  |
| spectrogram (1 - correlation), note for note | 0.518 (nearest 0.485) | fonts among themselves 0.356 | | | | 1.5 | yes |
| spectral shape of the body, note for note | 21.4 (nearest 19.6) | fonts among themselves 17.4 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 270 | 400 | 56.0 | 272 | 332 | 0.1 |  |
| partial stretch (inharmonicity) (c/partial) | 0.71 | 0.01 | 0.92 | 4.68 | -1.6 | 0.1 |  |
| level after 2 s held, re peak (dB) | -19.9 | -33.1 | -33.2 | -23.9 | -23.2 | 1.7 | yes |
| high bands lag low bands by (ms) | -1 | 0 | -1 | 2.3 | -2 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 8.7 | 0 | 1.8 | 2.8 | 7.6 | 1.3 |  |
| fundamental share (tristimulus 1) | 0.374 | 0.834 | 0.999 | 0.998 | 0.948 | 7.3 | yes |
| partials 2-4 share (tristimulus 2) | 0.567 | 0.095 | 0 | 0 | 0 | 8.1 | yes |
| partials 5+ share (tristimulus 3) | 0.002 | 0.062 | 0.001 | 0.002 | 0.052 | 0.4 |  |
| loudness range, velocity 40 to 127 (dB) | 15.3 | 16.7 | 16.7 | 16.6 | 20.1 | 0.8 | yes |
| pitch wobble / vibrato (p-p) (c) | 32.0 | 0 | 2.6 | 0.6 | 15.4 | 3.9 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0248 (nearest 0.0226) | fonts among themselves 0.0272 | | | | 0.9 |  |

</details>

### 100 FX 4 (atmosphere): fx-atmosphere

Distance 2.14 (rank 100 of 129); the fonts disagree 1.2 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Key scatter (3.1 vs 0.6…1.5 dB), brightness scatter 1.5 vs 0.6…0.9 ERB**: DELIBERATE. Each course beats its own way (README "FX 4 (atmosphere)").
- **Wide (coherence 0.29 vs 1)**: room ARTIFACT (README).
- **A little darker (8.4 vs 9.2…10.8 ERB) and purer (flatness −61 vs −37…−51 dB)**: DELIBERATE. Locked partials against the fonts' detuned pad clusters. The README's earlier run had it inside the range, so this is minor.
- **Phrase onset 37 vs 1…14 ms; velocity range 13.9 vs 16.7…21 dB**: minor. The plectrum's press (3–8 ms) plus the room.
- **No vibrato (4 vs 14…30 c)**: DELIBERATE.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (2.7 dB | 0.6 | 1.18 | 0.82 | 1.45), outside the fonts, 3.4 x.
2. **key-to-key brightness scatter**: less even in tone from key to key (1.53 ERB | 0.574 | 0.87 | 0.635 | 0.753), outside the fonts, 4.2 x.
3. **level against the other instruments (velocity 96)**: quieter (-2.8 dB | 1.6 | 2.2 | -1.4 | 3.3), outside the fonts, 2.0 x.
4. **stereo coherence [room]**: wider (0.29 | 1 | 1 | 1 | 1), outside the fonts, 9.5 x.
5. **brightness (spectral centroid, velocity 96)**: darker (8.61 ERB | 9.2 | 10.8 | 10.4 | 9.53), outside the fonts, 1.8 x.
6. **loudness range, velocity 40 to 127**: a narrower dynamic range (14.0 dB | 16.7 | 16.7 | 16.7 | 20.8), outside the fonts, 1.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +4 | +2 | +4 | +5 | -4 | -12 |
| re TimGM6mb | +4 | +2 | +5 | +2 | -6 | -10 |
| re Jnsgm2 | +8 | +7 | +8 | +6 | -6 | -16 |
| re SGM | -1 | -2 | +0 | +0 | +2 | -2 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -16 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.5 ERB, attack +0.2 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 2.4 dB rms; TimGM6mb: centroid -0.4 ERB, attack +0.4 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 7.9 dB rms; Jnsgm2: centroid -0.8 ERB, attack +0.3 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 6.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -1.23 | 1.98 | -0.33 | 0.22 | 0.67 | 1.5 | yes |
| attack time (-20 to -1 dB) (ms) | 37.0 | 213 | 165 | 28.5 | 8 | 0.4 |  |
| spectral shape of the attack, note for note | 11.7 (nearest 10.9) | fonts among themselves 7.56 | | | | 1.5 | yes |
| brightness over the keyboard (ERB/oct) | 3.23 | 3.11 | 3.63 | 3.41 | 3.28 | 0.4 |  |
| key-to-key brightness scatter (ERB) | 1.53 | 0.574 | 0.87 | 0.635 | 0.753 | 4.2 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.52 | 0 | -0 | 0 | -0.04 | 1.7 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 8.61 | 9.2 | 10.8 | 10.4 | 9.53 | 1.8 | yes |
| decay rate of a held note (dB/s) | 3.5 | 7.9 | 1.4 | 1.7 | 4.6 | 0.3 |  |
| onset delay (ms) | 15.0 | 23.0 | 8 | 12.0 | 8 | 0.5 |  |
| Plomp-Levelt dissonance of a note (log2) | -3.17 | -2.93 | -3.15 | -2.72 | -3.95 | 0.2 |  |
| envelope curve, note for note (dB rms) | 9.1 (nearest 7.99) | fonts among themselves 7.85 | | | | 1.2 |  |
| pitch re 12-TET A440 (median) (c) | 0 | 3.8 | 1.6 | -1 | 2.2 | 0.5 |  |
| pitch scatter from key to key (c) | 0.9 | 9.3 | 1.9 | 3.7 | 1.1 | 0.5 |  |
| noisiness (spectral flatness) (dB) | -61.0 | -49.6 | -44.9 | -36.5 | -51.4 | 1.8 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -70.8 | -47.1 | -68.7 | -56.8 | -73.2 | 0.6 |  |
| pitch at the start re settled (c) | 0.2 | -5.4 | -2.6 | -6.5 | 0.9 | 0.8 |  |
| stereo coherence | 0.29 | 1 | 1 | 1 | 1 | 9.5 | yes |
| partial-to-partial jaggedness (dB) | 5.15 | 1.4 | 3.1 | 1.9 | 2.6 | 2.9 | yes |
| dip between legato notes (dB) | 9.3 | 13.2 | 9.3 | 10.4 | 13.2 | 1.2 |  |
| level evenness of a legato scale (dB) | 2.8 | 2.2 | 1.2 | 1.4 | 2.8 | 1.0 |  |
| level over the keyboard (dB/oct) | -0.33 | 1.11 | -0.57 | -0.29 | -1.52 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | -2.8 | 1.6 | 2.2 | -1.4 | 3.3 | 2.0 | yes |
| key-to-key level scatter (dB) | 2.7 | 0.6 | 1.18 | 0.82 | 1.45 | 3.4 | yes |
| odd / even partials (dB) | -5.2 | -6.1 | -3.7 | -3.4 | -4.6 | 0.5 |  |
| onset jitter in a phrase (ms) | 40.2 | 47.2 | 43.4 | 7.6 | 44.6 | 0.2 |  |
| onset in a phrase (re the note-on) (ms) | 29.0 | 4 | 1 | 14.0 | 3 | 2.5 | yes |
| onset sharpness (dB/ms) | 16.8 | 8.4 | 10.8 | 6.5 | 8.8 | 3.8 | yes |
| overtone levels, note for note (dB rms) | 11.4 (nearest 9.83) | fonts among themselves 7.06 | | | | 1.6 | yes |
| panning across the keys (dB/oct) | 0.05 | -0.02 | 0 | 0 | -0.01 | 0.1 |  |
| partial envelopes alike | 0.298 | 0.176 | 0.04 | -0.024 | 0.573 | 0.6 |  |
| release time (30 dB after note-off) (ms) | 794 | 1041 | 631 | 734 | 458 | 0.4 |  |
| spectrogram (1 - correlation), note for note | 0.225 (nearest 0.133) | fonts among themselves 0.131 | | | | 1.7 |  |
| spectral shape of the body, note for note | 12.2 (nearest 9.8) | fonts among themselves 8.55 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 104 | 260 | 168 | 269 | 94.0 | 1.3 |  |
| partial stretch (inharmonicity) (c/partial) | 0.43 | 0.29 | 0 | -0.07 | 0.01 | 0.8 |  |
| level after 2 s held, re peak (dB) | -14.6 | -17.0 | -11.2 | -10.5 | -19.8 | 0.1 |  |
| high bands lag low bands by (ms) | 1.4 | -8 | 2 | 2 | 0.4 | 0.0 |  |
| transient coherence: attack spread over bands (ms) | 4 | 3.4 | 1.7 | 6 | 1.6 | 0.3 |  |
| fundamental share (tristimulus 1) | 0.544 | 0.847 | 0.459 | 0.794 | 0.532 | 0.6 |  |
| partials 2-4 share (tristimulus 2) | 0.254 | 0.147 | 0.467 | 0.174 | 0.465 | 0.4 |  |
| partials 5+ share (tristimulus 3) | 0.019 | 0.006 | 0.007 | 0.038 | 0.003 | 0.2 |  |
| loudness range, velocity 40 to 127 (dB) | 14.0 | 16.7 | 16.7 | 16.7 | 20.8 | 1.3 | yes |
| pitch wobble / vibrato (p-p) (c) | 4 | 28.6 | 14.4 | 29.9 | 18.2 | 2.5 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0167 (nearest 0.014) | fonts among themselves 0.0127 | | | | 1.3 |  |

</details>

### 101 FX 5 (brightness): fx-brightness

Distance 1.45 (rank 128 of 129); the fonts disagree 2.4 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Smoother (Plomp-Levelt −4.3 vs −1.8…−2.8), no vibrato (4.8 vs 22…51 c)**: DELIBERATE. Exact harmonics from the arc, where the samples are detuned clusters with an LFO (README "FX 5 (brightness)").
- **Key scatter 1.7 vs 0.3…0.9 dB**: DELIBERATE: the two arcs beat, each key in its own phase.
- **Phrase onset 38 vs −4…7 ms**: the room and the bloom of soft treble notes (README). Minor.
- **Holds (−5.7 dB after 2 s)**: like FluidR3 (−8.4); the others fall to −21…−28. The fonts disagree (2.4 JND).
- Rank 114: the closest of my programs.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **Plomp-Levelt dissonance of a note**: smoother (-4.29 log2 | -1.84 | -2.58 | -2.78 | -2.37), outside the fonts, 3.9 x.
2. **onset in a phrase (re the note-on)**: speaks later in a phrase (41.0 ms | -4 | 7 | 1 | 4), outside the fonts, 3.9 x.
3. **brightening from velocity 40 to 127**: brightens more with velocity (0.65 ERB | 0.03 | -0 | 0 | -0.3), outside the fonts, 2.2 x.
4. **key-to-key level scatter**: less even from key to key (1.33 dB | 0.48 | 0.26 | 0.37 | 0.87), outside the fonts, 1.8 x.
5. **level after 2 s held, re peak**: sustains more (-5.6 dB | -8.4 | -28.4 | -26.7 | -20.7), outside the fonts, 1.8 x.
6. **pitch wobble / vibrato (p-p)**: wobbles less (4.8 c | 51.4 | 28.9 | 25.3 | 22.0), outside the fonts, 1.5 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +1 | +2 | +2 | +2 | +1 | -5 |
| re TimGM6mb | -0 | -0 | +1 | +1 | +0 | -1 |
| re Jnsgm2 | +5 | -2 | +0 | +1 | +0 | -2 |
| re SGM | +0 | +3 | +3 | -0 | +0 | -4 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +3; FluidR3 -12 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack -0.1 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 1.7 dB rms; TimGM6mb: centroid -0.1 ERB, attack +0.1 ERB, velocity brightening -0.4 ERB, loudness range +3 dB, envelope 2.5 dB rms; Jnsgm2: centroid -0.1 ERB, attack -0.0 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 2.3 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -0.31 | 1.45 | -0.76 | -0.1 | -1.43 | 0.1 |  |
| attack time (-20 to -1 dB) (ms) | 48.5 | 159 | 53.0 | 18.0 | 77.0 | 0.3 |  |
| spectral shape of the attack, note for note | 10.9 (nearest 5.49) | fonts among themselves 9.67 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | 4.53 | 4.77 | 4.19 | 3.63 | 4.52 | 0.3 |  |
| key-to-key brightness scatter (ERB) | 1.22 | 0.483 | 0.654 | 0.623 | 1.27 | 1.5 |  |
| brightening from velocity 40 to 127 (ERB) | 0.65 | 0.03 | -0 | 0 | -0.3 | 2.2 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 10.7 | 10.4 | 9.99 | 10.7 | 12.4 | 0.1 |  |
| onset delay (ms) | 18.0 | 4 | 8 | 5 | 18.0 | 1.1 |  |
| Plomp-Levelt dissonance of a note (log2) | -4.29 | -1.84 | -2.58 | -2.78 | -2.37 | 3.9 | yes |
| envelope curve, note for note (dB rms) | 16.6 (nearest 7.53) | fonts among themselves 12 | | | | 1.4 |  |
| pitch re 12-TET A440 (median) (c) | 0 | 4.3 | 1.4 | 1.6 | -0.2 | 0.4 |  |
| pitch scatter from key to key (c) | 1 | 6.1 | 1.5 | 0.7 | 5.3 | 0.8 |  |
| noisiness (spectral flatness) (dB) | -39.8 | -40.0 | -32.1 | -26.1 | -34.9 | 0.9 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -57.1 | -40.0 | -119 | -117 | -83.4 | 1.1 |  |
| pitch at the start re settled (c) | -2.2 | 28.9 | 2.1 | -2.2 | 3.6 | 0.3 |  |
| stereo coherence | 0.385 | 0.128 | 0.991 | 1 | 0.426 | 0.7 |  |
| partial-to-partial jaggedness (dB) | 3.25 | 1.4 | 5.6 | 7.5 | 14.0 | 0.5 |  |
| dip between legato notes (dB) | 10.1 | 13.7 | 9.5 | 8.6 | 7.8 | 0.4 |  |
| level evenness of a legato scale (dB) | 1.8 | 1.4 | 0.8 | 1 | 2.4 | 0.6 |  |
| level over the keyboard (dB/oct) | -0.31 | -0.56 | -0.26 | 0 | -1.13 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | 0.5 | -2.8 | 2.6 | 1.4 | 0.2 | 0.1 |  |
| key-to-key level scatter (dB) | 1.33 | 0.48 | 0.26 | 0.37 | 0.87 | 1.8 | yes |
| odd / even partials (dB) | -5.7 | -7.1 | -12.3 | -8.8 | -17.1 | 1.0 | yes |
| onset jitter in a phrase (ms) | 37.9 | 29.4 | 35.7 | 37.1 | 53.8 | 0.1 |  |
| onset in a phrase (re the note-on) (ms) | 41.0 | -4 | 7 | 1 | 4 | 3.9 | yes |
| onset sharpness (dB/ms) | 10.4 | 5.2 | 7.7 | 7.1 | 38.9 | 0.2 |  |
| overtone levels, note for note (dB rms) | 9.58 (nearest 5.43) | fonts among themselves 8.19 | | | | 1.2 |  |
| panning across the keys (dB/oct) | -0.33 | -0.26 | -0.05 | 0 | 0.42 | 0.3 |  |
| partial envelopes alike | -0.017 | -0.018 | 0.683 | 0.704 | 0.238 | 1.3 |  |
| release time (30 dB after note-off) (ms) | 887 | 1110 | 791 | 828 | 700 | 0.4 |  |
| spectrogram (1 - correlation), note for note | 0.185 (nearest 0.114) | fonts among themselves 0.198 | | | | 0.9 |  |
| spectral shape of the body, note for note | 10 (nearest 6.66) | fonts among themselves 9.4 | | | | 1.1 |  |
| sounding length of a 100 ms note (ms) | 352 | 351 | 329 | 149 | 118 | 0.9 |  |
| partial stretch (inharmonicity) (c/partial) | -0.03 | -0.78 | -0.16 | 0.25 | -0.33 | 0.4 |  |
| level after 2 s held, re peak (dB) | -5.6 | -8.4 | -28.4 | -26.7 | -20.7 | 1.8 | yes |
| high bands lag low bands by (ms) | 4 | -32.0 | 3.3 | 7.2 | 0 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 2 | 16.7 | 2.2 | 3.4 | 12.3 | 0.8 |  |
| fundamental share (tristimulus 1) | 0.474 | 0.52 | 0.748 | 0.185 | 0.408 | 0.0 |  |
| partials 2-4 share (tristimulus 2) | 0.389 | 0.441 | 0.234 | 0.795 | 0.475 | 0.2 |  |
| partials 5+ share (tristimulus 3) | 0.058 | 0.028 | 0.016 | 0.016 | 0.059 | 0.5 |  |
| loudness range, velocity 40 to 127 (dB) | 15.8 | 16.5 | 16.7 | 16.6 | 20.0 | 0.5 |  |
| pitch wobble / vibrato (p-p) (c) | 4.8 | 51.4 | 28.9 | 25.3 | 22.0 | 1.5 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0207 (nearest 0.0171) | fonts among themselves 0.0222 | | | | 0.9 |  |

</details>

### 102 FX 6 (goblins): fx-goblins

Distance 4.79 (rank 6 of 129); the fonts disagree 1.9 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Quieter (−29 vs −8…−10 dB)**: the fonts swell too, but ours starts 20 dB lower. Its oscillation grows out of the lighting (onset 938 vs 538…690 ms), and the early level window misses it. Partly a MEASUREMENT ARTIFACT, but the effect is real: notes under ~1 s nearly vanish. The trim came from 1.5 s notes (fx_sounds.md FX 6 "Measurements"). FAULT for GM use.
- **Brighter (12.8 vs 7.0…9.4 ERB), brightness scatter 2.5 vs 0.2…0.6 ERB**: DELIBERATE: the pipe's upper partials, and each flame puffs in its own phase (README "FX 6 (goblins)").
- **Duller attack (−4.5 ERB), darker when harder (−1.4 ERB)**: DELIBERATE. It opens as it warms. TiMidity also drops the fonts' ~9 s filter sweep.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **level against the other instruments (velocity 96)**: quieter (-27.6 dB | -9.8 | -8.6 | -7.9 | -9), outside the fonts, 18.8 x.
2. **brightness (spectral centroid, velocity 96)**: brighter (13.3 ERB | 9.43 | 7.48 | 7.44 | 7.04), outside the fonts, 4.9 x.
3. **key-to-key brightness scatter**: less even in tone from key to key (2.23 ERB | 0.557 | 0.189 | 0.33 | 0.601), outside the fonts, 8.7 x.
4. **onset delay**: speaks later (1033 ms | 547 | 538 | 608 | 686), outside the fonts, 6.2 x.
5. **attack brighter than the body by**: a duller attack (-3.52 ERB | 1.06 | -0.12 | 0.06 | -0.04), outside the fonts, 6.0 x.
6. **transient coherence: attack spread over bands**: a less coherent attack (33.2 ms | 19.0 | 9 | 9.8 | 8.9), outside the fonts, 4.7 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +2 | -7 | -0 | +0 | +10 | -7 |
| re TimGM6mb | -16 | -14 | -5 | -0 | +19 | +8 |
| re Jnsgm2 | -5 | -4 | +5 | +8 | +9 | -14 |
| re SGM | -26 | -24 | -15 | +5 | +31 | +18 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -5 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -21 -12 +0 +7
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.5 ERB, attack +2.5 ERB, velocity brightening +0.2 ERB, loudness range +3 dB, envelope 2.4 dB rms; TimGM6mb: centroid -0.4 ERB, attack -0.1 ERB, velocity brightening -0.5 ERB, loudness range +3 dB, envelope 2.5 dB rms; Jnsgm2: centroid -0.3 ERB, attack -0.1 ERB, velocity brightening -0.5 ERB, loudness range +3 dB, envelope 2.3 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -3.52 | 1.06 | -0.12 | 0.06 | -0.04 | 6.0 | yes |
| attack time (-20 to -1 dB) (ms) | 462 | 728 | 758 | 790 | 670 | 2.7 | yes |
| spectral shape of the attack, note for note | 12.9 (nearest 10.4) | fonts among themselves 11.8 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | 1.98 | 3.08 | 3.49 | 3.29 | 3.29 | 4.4 | yes |
| key-to-key brightness scatter (ERB) | 2.23 | 0.557 | 0.189 | 0.33 | 0.601 | 8.7 | yes |
| brightening from velocity 40 to 127 (ERB) | -0.69 | 0.02 | 0.02 | 0.04 | -0.11 | 2.4 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 13.3 | 9.43 | 7.48 | 7.44 | 7.04 | 4.9 | yes |
| onset delay (ms) | 1033 | 547 | 538 | 608 | 686 | 6.2 | yes |
| Plomp-Levelt dissonance of a note (log2) | -1.3 | -2.7 | -3.91 | -3.32 | -4.31 | 2.9 | yes |
| envelope curve, note for note (dB rms) | 11 (nearest 10.3) | fonts among themselves 6.24 | | | | 1.8 | yes |
| pitch re 12-TET A440 (median) (c) | -4.4 | -1.2 | 2.1 | -2.3 | 5.2 | 1.2 | yes |
| pitch scatter from key to key (c) | 2.1 | 3.1 | 14.4 | 9.6 | 9.8 | 1.3 |  |
| noisiness (spectral flatness) (dB) | -24.3 | -34.5 | -49.0 | -34.5 | -62.8 | 1.2 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -60.4 | -41.7 | -41.1 | -42.4 | -49.0 | 3.1 | yes |
| pitch at the start re settled (c) | 4.6 | -2 | -1.6 | 6.2 | -5.7 | 1.1 |  |
| stereo coherence | 0.498 | 0.217 | 1 | 1 | 1 | 1.3 |  |
| partial-to-partial jaggedness (dB) | 3.2 | 3.7 | 3 | 3.4 | 3.45 | 0.2 |  |
| dip between legato notes (dB) | 10.8 | 16.8 | 13.6 | 12.3 | 4.3 | 0.3 |  |
| level evenness of a legato scale (dB) | 3.9 | 3 | 2.8 | 4.3 | 2.2 | 1.0 |  |
| level over the keyboard (dB/oct) | -0.32 | 1.53 | 0.08 | 1.94 | -2.69 | 0.5 |  |
| level against the other instruments (velocity 96) (dB) | -27.6 | -9.8 | -8.6 | -7.9 | -9 | 18.8 | yes |
| key-to-key level scatter (dB) | 2.91 | 0.53 | 1.16 | 1.64 | 4.05 | 0.9 |  |
| odd / even partials (dB) | -1.2 | -22.7 | -22.0 | -17.4 | -26.1 | 4.9 | yes |
| onset jitter in a phrase (ms) | 44.5 | 47.1 | 59.6 | 48.1 | 32.0 | 0.2 |  |
| onset in a phrase (re the note-on) (ms) | 38.0 | 41.0 | 68.0 | 44.0 | -8 | 0.1 |  |
| onset sharpness (dB/ms) | 5.9 | 8.9 | 8.9 | 9.3 | 7.5 | 1.5 | yes |
| overtone levels, note for note (dB rms) | 22.9 (nearest 12.7) | fonts among themselves 13.2 | | | | 1.7 |  |
| panning across the keys (dB/oct) | 0.67 | 4.03 | 0 | 0 | -0.01 | 0.3 |  |
| partial envelopes alike | 0.822 | 0.643 | 0.643 | 0.601 | 0.836 | 1.5 |  |
| release time (30 dB after note-off) (ms) | 935 | 880 | 610 | 789 | 594 | 1.5 |  |
| spectrogram (1 - correlation), note for note | 0.28 (nearest 0.187) | fonts among themselves 0.23 | | | | 1.2 |  |
| spectral shape of the body, note for note | 13.7 (nearest 9.19) | fonts among themselves 11.6 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 286 | 400 | 251 | 184 | 230 | 0.4 |  |
| partial stretch (inharmonicity) (c/partial) | 0.02 | 0.88 | 5.52 | -0.29 | -3.67 | 0.1 |  |
| level after 2 s held, re peak (dB) | -2.7 | -7.5 | -9.2 | -9 | -10.7 | 2.1 | yes |
| high bands lag low bands by (ms) | 39.1 | 0 | 32.0 | 37.6 | 0 | 1.2 |  |
| transient coherence: attack spread over bands (ms) | 33.2 | 19.0 | 9 | 9.8 | 8.9 | 4.7 | yes |
| fundamental share (tristimulus 1) | 0.889 | 0.37 | 0.933 | 0.941 | 0.991 | 0.2 |  |
| partials 2-4 share (tristimulus 2) | 0.038 | 0.626 | 0.067 | 0.059 | 0.009 | 0.1 |  |
| partials 5+ share (tristimulus 3) | 0.053 | 0.004 | 0 | 0 | 0 | 0.8 | yes |
| loudness range, velocity 40 to 127 (dB) | 15.1 | 16.6 | 16.6 | 16.6 | 27.6 | 0.3 | yes |
| pitch wobble / vibrato (p-p) (c) | 19.0 | 20.9 | 38.0 | 34.1 | 34.9 | 1.8 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0196 (nearest 0.0172) | fonts among themselves 0.0196 | | | | 1.0 |  |

</details>

### 103 FX 7 (echoes): fx-echoes

Distance 2.68 (rank 68 of 129); the fonts disagree 1.3 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **It echoes**: DELIBERATE. The fonts' "Echo Drops" don't echo at all (README "FX 7 (echoes)"; fx_sounds.md FX 7).
- **Key scatter 4.0 vs 0.5…1.6 dB**: room ARTIFACT. A near-sine and its repeats add or cancel in the hall (0.5 dB dry, README).
- **Phrase onset 104 vs 0…33 ms**: probably a MEASUREMENT ARTIFACT. The 0.335 s repeats of the previous note overlap the 0.4 s phrase steps, and a struck fork otherwise speaks quickly (attack 132 vs 208…543 ms).
- **Near-sine, no breath (fundamental share 0.99; flatness −81 vs −29…−54 dB; −10…−25 dB above 2.5 kHz); highs lead by 35 ms**: DELIBERATE: a fork with a fast-dying clang, against the fonts' choir/synth voices.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **onset in a phrase (re the note-on)**: speaks later in a phrase (100 ms | 12.0 | 0 | 0 | 20.0), outside the fonts, 9.4 x.
2. **key-to-key level scatter**: less even from key to key (3.73 dB | 0.73 | 0.51 | 1.19 | 1.59), outside the fonts, 5.1 x.
3. **Plomp-Levelt dissonance of a note**: smoother (-6.51 log2 | -2.76 | -2.08 | -3.63 | -2.62), outside the fonts, 4.9 x.
4. **odd / even partials**: more even partials (-46.1 dB | -10.5 | 2.5 | -16.9 | -6.4), outside the fonts, 3.9 x.
5. **partial envelopes alike**: partials moving more as one (0.491 | 0.004 | 0.021 | -0.003 | 0.133), outside the fonts, 4.8 x.
6. **high bands lag low bands by**: its highs start sooner (-32.5 ms | 0 | -5.3 | 0 | -3), outside the fonts, 6.2 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +18 | +15 | +12 | +7 | -10 | -24 |
| re TimGM6mb | +16 | +10 | +8 | +3 | -16 | -16 |
| re Jnsgm2 | +15 | +13 | +12 | +11 | -14 | -24 |
| re SGM | +5 | -2 | -6 | +2 | +19 | - |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -18 -8 +0 +6
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack +0.2 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 2.4 dB rms; TimGM6mb: centroid -0.6 ERB, attack +0.6 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 2.3 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.2 ERB, velocity brightening -0.5 ERB, loudness range +3 dB, envelope 2.3 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.21 | 0.59 | 1.15 | 0.29 | 0.48 | 0.8 |  |
| attack time (-20 to -1 dB) (ms) | 134 | 245 | 208 | 543 | 420 | 1.8 | yes |
| spectral shape of the attack, note for note | 14.2 (nearest 10.8) | fonts among themselves 10.9 | | | | 1.3 |  |
| brightness over the keyboard (ERB/oct) | 3.68 | 4.16 | 3.51 | 3.38 | 2.48 | 0.3 |  |
| key-to-key brightness scatter (ERB) | 0.449 | 0.547 | 0.605 | 0.357 | 0.647 | 0.6 |  |
| brightening from velocity 40 to 127 (ERB) | 0.58 | 0 | 0 | -0 | 1.33 | 0.9 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 8.05 | 8.49 | 10.7 | 7.42 | 9.25 | 0.5 |  |
| onset delay (ms) | 52.0 | 85.0 | 58.0 | 6 | 12.0 | 0.4 |  |
| Plomp-Levelt dissonance of a note (log2) | -6.51 | -2.76 | -2.08 | -3.63 | -2.62 | 4.9 | yes |
| envelope curve, note for note (dB rms) | 7.9 (nearest 5.16) | fonts among themselves 13 | | | | 0.6 |  |
| pitch re 12-TET A440 (median) (c) | 0.2 | -4.1 | -1.3 | 0.2 | -3.8 | 0.7 |  |
| pitch scatter from key to key (c) | 1.3 | 1.2 | 5.6 | 10.5 | 5.8 | 0.9 |  |
| noisiness (spectral flatness) (dB) | -80.4 | -29.2 | -35.6 | -38.3 | -54.1 | 3.5 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -38.0 | -42.2 | -57.5 | -125 | -56.4 | 0.5 | yes |
| pitch at the start re settled (c) | 0.4 | -0.4 | 1.4 | 5.8 | 10.8 | 0.6 |  |
| stereo coherence | 0.421 | 0.769 | 1 | 1 | 1 | 5.0 | yes |
| partial-to-partial jaggedness (dB) | 4.2 | 2.8 | 2.7 | 3.3 | 2.4 | 1.5 | yes |
| dip between legato notes (dB) | 12.9 | 13.4 | 11.4 | 8.6 | 11.0 | 0.7 |  |
| level evenness of a legato scale (dB) | 3.5 | 1.3 | 0.7 | 2 | 2.7 | 1.9 | yes |
| level over the keyboard (dB/oct) | 0.13 | -0.09 | 0.51 | 1.04 | -0.76 | 0.1 |  |
| level against the other instruments (velocity 96) (dB) | 1.1 | -0.1 | 3.7 | 1.9 | 1.8 | 0.4 |  |
| key-to-key level scatter (dB) | 3.73 | 0.73 | 0.51 | 1.19 | 1.59 | 5.1 | yes |
| odd / even partials (dB) | -46.1 | -10.5 | 2.5 | -16.9 | -6.4 | 3.9 | yes |
| onset jitter in a phrase (ms) | 43.0 | 50.3 | 56.2 | 16.4 | 38.8 | 0.1 |  |
| onset in a phrase (re the note-on) (ms) | 100 | 12.0 | 0 | 0 | 20.0 | 9.4 | yes |
| onset sharpness (dB/ms) | 10.0 | 6.8 | 6.7 | 9.4 | 10.6 | 1.0 |  |
| overtone levels, note for note (dB rms) | 25.8 (nearest 19.4) | fonts among themselves 12.7 | | | | 2.0 | yes |
| panning across the keys (dB/oct) | -0.09 | 0.54 | 0 | 0 | -0.01 | 0.1 |  |
| partial envelopes alike | 0.491 | 0.004 | 0.021 | -0.003 | 0.133 | 4.8 | yes |
| release time (30 dB after note-off) (ms) | 1208 | 917 | 689 | 93.0 | 697 | 0.5 | yes |
| spectrogram (1 - correlation), note for note | 0.416 (nearest 0.31) | fonts among themselves 0.271 | | | | 1.5 |  |
| spectral shape of the body, note for note | 13.1 (nearest 11.3) | fonts among themselves 11.3 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 316 | 214 | 215 | 72.0 | 210 | 0.7 | yes |
| level after 2 s held, re peak (dB) | -10.3 | -6.1 | -9.3 | -9.7 | -9.6 | 0.3 |  |
| high bands lag low bands by (ms) | -32.5 | 0 | -5.3 | 0 | -3 | 6.2 | yes |
| transient coherence: attack spread over bands (ms) | 7.8 | 6.7 | 9.4 | 0.2 | 3.5 | 0.5 |  |
| fundamental share (tristimulus 1) | 0.988 | 0.829 | 0.757 | 0.94 | 0.78 | 2.0 | yes |
| partials 2-4 share (tristimulus 2) | 0.011 | 0.169 | 0.24 | 0.06 | 0.205 | 2.0 | yes |
| partials 5+ share (tristimulus 3) | 0 | 0.003 | 0.014 | 0 | 0.001 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 16.0 | 16.7 | 16.7 | 16.7 | 23.2 | 0.2 |  |
| pitch wobble / vibrato (p-p) (c) | 5.8 | 15.7 | 23.5 | 30.6 | 32.2 | 2.6 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0199 (nearest 0.0167) | fonts among themselves 0.0196 | | | | 1.0 |  |

</details>

### 104 FX 8 (sci-fi): fx-sci-fi

Distance 3.30 (rank 40 of 129); the fonts disagree 1.5 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Dies away (−50 vs −8…−18 dB after 2 s; decay 25 vs 0.3…7 dB/s; staccato 148 vs 275…396 ms)**: DELIBERATE. A shot and a ping whose T60 is ~2.3 s at middle C (README "FX 8 (sci-fi)", K4), against the fonts' held pads. K4 lengthens it.
- **Narrow velocity range (6.7 vs 17…21 dB; −5 dB at v40)**: probable FAULT. Velocity drives the fold, the drop and the ping's index, but hardly the loudness. GM files expect quiet notes to be quiet.
- **Sharp onset (57 vs 4.5…11 dB/ms), brightens with velocity (+2.6 ERB)**: DELIBERATE: a discharge.
- Effects: the fonts disagree (1.5 JND), and the pitch measures mean little here.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **level after 2 s held, re peak**: sustains less (-51.1 dB | -17.7 | -7.8 | -8 | -14.6), outside the fonts, 8.0 x.
2. **loudness range, velocity 40 to 127**: a narrower dynamic range (6.5 dB | 16.6 | 16.6 | 16.6 | 21.2), outside the fonts, 4.4 x.
3. **sounding length of a 100 ms note [room]**: shorter staccato (108 ms | 358 | 396 | 275 | 356), outside the fonts, 6.6 x.
4. **brightening from velocity 40 to 127**: brightens more with velocity (1.89 ERB | 0 | 0 | 0 | 1.01), outside the fonts, 3.7 x.
5. **onset sharpness**: a sharper onset (57.5 dB/ms | 11.2 | 5 | 4.5 | 4.7), outside the fonts, 15.7 x.
6. **brightness (spectral centroid, velocity 96)**: darker (8.24 ERB | 10.0 | 10.6 | 11.0 | 8.93), outside the fonts, 2.0 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +4 | -2 | -1 | -3 | -2 | +6 |
| re TimGM6mb | +7 | +4 | +5 | +1 | -2 | -7 |
| re Jnsgm2 | +2 | +2 | +2 | -4 | -2 | +4 |
| re SGM | -3 | -6 | -6 | -4 | +2 | +13 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -5 -2 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -16 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.2 ERB, attack +0.0 ERB, velocity brightening +0.0 ERB, loudness range +3 dB, envelope 2.1 dB rms; TimGM6mb: centroid -1.3 ERB, attack +0.3 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 4.5 dB rms; Jnsgm2: centroid -0.6 ERB, attack -0.6 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 4.7 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 5 | 5.82 | 4.31 | 4.19 | 3.68 | 0.7 |  |
| attack time (-20 to -1 dB) (ms) | 81.0 | 124 | 308 | 96.0 | 136 | 0.8 | yes |
| spectral shape of the attack, note for note | 13 (nearest 11.9) | fonts among themselves 10.3 | | | | 1.3 |  |
| brightness over the keyboard (ERB/oct) | 3.7 | 3.04 | 3.67 | 3.54 | 2.77 | 0.9 |  |
| key-to-key brightness scatter (ERB) | 0.912 | 0.67 | 0.943 | 1.11 | 0.576 | 0.4 |  |
| brightening from velocity 40 to 127 (ERB) | 1.89 | 0 | 0 | 0 | 1.01 | 3.7 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 8.24 | 10.0 | 10.6 | 11.0 | 8.93 | 2.0 | yes |
| decay rate of a held note (dB/s) | 26.7 | 7.4 | 0.3 | 0.8 | 3.1 | 1.8 | yes |
| onset delay (ms) | 15.0 | 49.0 | 52.0 | 36.0 | 44.0 | 3.1 | yes |
| Plomp-Levelt dissonance of a note (log2) | -1.4 | -3.16 | -2.21 | -2.18 | -3.48 | 2.0 | yes |
| envelope curve, note for note (dB rms) | 24.6 (nearest 21.2) | fonts among themselves 7.75 | | | | 3.2 | yes |
| pitch re 12-TET A440 (median) (c) | 1.6 | -1.9 | 1.4 | 1.4 | 0.1 | 0.2 |  |
| pitch scatter from key to key (c) | 2.4 | 7.1 | 3.9 | 4.6 | 2.5 | 0.6 |  |
| noisiness (spectral flatness) (dB) | -37.6 | -48.7 | -41.7 | -40.1 | -47.6 | 1.6 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -122 | -47.1 | -65.8 | -66.2 | -79.8 | 3.4 | yes |
| pitch at the start re settled (c) | -0.2 | -0.4 | -3.6 | 2.8 | -4.9 | 0.4 |  |
| stereo coherence | 0.671 | 1 | 1 | 1 | 1 | 4.4 | yes |
| partial-to-partial jaggedness (dB) | 2 | 2.5 | 2.7 | 2.1 | 2.15 | 0.3 |  |
| dip between legato notes (dB) | 10.4 | 8.1 | 7.7 | 6.9 | 13.0 | 0.8 |  |
| level evenness of a legato scale (dB) | 1.3 | 2.5 | 1.5 | 1.6 | 2.4 | 0.7 |  |
| level over the keyboard (dB/oct) | -0.76 | 0.95 | 0.36 | 0.25 | -0.2 | 1.1 | yes |
| level against the other instruments (velocity 96) (dB) | 0.1 | -2.2 | 1.6 | 1.6 | 2.9 | 0.6 |  |
| key-to-key level scatter (dB) | 1.47 | 0.78 | 0.5 | 0.35 | 0.85 | 1.7 | yes |
| odd / even partials (dB) | -7.3 | 1.3 | -0.1 | 0.7 | -2.7 | 3.8 | yes |
| onset jitter in a phrase (ms) | 53.9 | 35.7 | 52.4 | 54.3 | 58.2 | 0.0 |  |
| onset in a phrase (re the note-on) (ms) | 48.0 | 41.0 | 4 | 4 | 10.0 | 2.2 | yes |
| onset sharpness (dB/ms) | 57.5 | 11.2 | 5 | 4.5 | 4.7 | 15.7 | yes |
| overtone levels, note for note (dB rms) | 13.3 (nearest 9.31) | fonts among themselves 7.48 | | | | 1.8 |  |
| panning across the keys (dB/oct) | 0.21 | -0.04 | 0 | 0 | -0.01 | 0.2 |  |
| partial envelopes alike | 0.931 | 0.283 | -0.051 | -0.004 | 0.457 | 3.1 | yes |
| release time (30 dB after note-off) (ms) | 349 | 1041 | 694 | 652 | 491 | 1.7 | yes |
| spectrogram (1 - correlation), note for note | 0.424 (nearest 0.263) | fonts among themselves 0.162 | | | | 2.6 | yes |
| spectral shape of the body, note for note | 13.5 (nearest 11.3) | fonts among themselves 8.12 | | | | 1.7 | yes |
| sounding length of a 100 ms note (ms) | 108 | 358 | 396 | 275 | 356 | 6.6 | yes |
| partial stretch (inharmonicity) (c/partial) | -1.04 | 0.58 | 0.04 | 0.68 | 0.2 | 2.9 | yes |
| level after 2 s held, re peak (dB) | -51.1 | -17.7 | -7.8 | -8 | -14.6 | 8.0 | yes |
| high bands lag low bands by (ms) | -22.0 | -40.1 | -31.0 | -15.3 | -19.6 | 0.3 |  |
| transient coherence: attack spread over bands (ms) | 19.2 | 18.8 | 17.9 | 7.2 | 10.2 | 0.9 |  |
| fundamental share (tristimulus 1) | 0.929 | 0.658 | 0.585 | 0.701 | 0.792 | 2.4 | yes |
| partials 2-4 share (tristimulus 2) | 0.071 | 0.243 | 0.293 | 0.276 | 0.203 | 2.7 | yes |
| partials 5+ share (tristimulus 3) | 0.001 | 0.091 | 0.112 | 0.052 | 0.006 | 1.0 |  |
| loudness range, velocity 40 to 127 (dB) | 6.5 | 16.6 | 16.6 | 16.6 | 21.2 | 4.4 | yes |
| pitch wobble / vibrato (p-p) (c) | 40.8 | 28.5 | 26.6 | 29.2 | 31.1 | 2.4 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0178 (nearest 0.0117) | fonts among themselves 0.0171 | | | | 1.0 |  |

</details>

### 105 Sitar: sitar

Distance 2.01 (rank 103 of 129); the fonts disagree 1.2 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Louder (+6.1 dB vs −3.6..+0.4 against the other instruments)**: FAULT, small. README "Ethnic and percussive (modes)", Levels, sets every modal instrument to one common loudness; the fonts keep the sitar lower in a mix.
- **Uneven onsets in a phrase (jitter 42.6 ms vs 1.6–8.4)**: probably a MEASUREMENT ARTIFACT. The phrase is at velocity 80, so no meend (that needs ≤ 45, src/modalinst.cc). The jawari buzz blooms ~0.3 s after the stroke (README), and the steepest-rise detector can lock onto the bloom. Listen before fixing.
- **Key-to-key level scatter (3.1 vs 0.06–0.62 dB), jagged overtones (6.8 vs 1.9–3.9 dB)**: mostly DELIBERATE: the jod/main-string split at F#3, the tarafs ringing on some keys, and the jawari spectrum (sitar_shamisen.md 1.2, 1.5). Probe keys 36–44 fold up an octave into the 48–84 range.
- **Brightens with velocity (+1.58 vs 0–0.39 ERB)**: DELIBERATE. A harder stroke, plus the chikari above velocity 90. The fonts' 0 is TiMidity's convention.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (2.67 dB | 0.44 | 0.53 | 0.06 | 0.62), outside the fonts, 4.4 x.
2. **brightening from velocity 40 to 127**: brightens more with velocity (1.19 ERB | 0 | -0 | -0 | 0.39), outside the fonts, 4.0 x.
3. **onset jitter in a phrase**: more uneven onsets (46.1 ms | 1.8 | 1.6 | 7.4 | 8.4), outside the fonts, 8.3 x.
4. **attack time (-20 to -1 dB)**: a slower attack (13.0 ms | 10.0 | 7 | 8 | 9), outside the fonts, 2.4 x.
5. **partial-to-partial jaggedness**: a more jagged overtone series (6.7 dB | 3.9 | 1.9 | 2.1 | 3.6), outside the fonts, 3.9 x.
6. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.476, nearest 0.384; the fonts among themselves 0.233), outside.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -10 | -4 | -1 | +8 | +7 | -6 |
| re TimGM6mb | -20 | +5 | +7 | +8 | +5 | -12 |
| re Jnsgm2 | -20 | +6 | +10 | +10 | +5 | -16 |
| re SGM | -10 | -2 | -1 | +7 | +7 | -7 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack -0.7 ERB, velocity brightening +0.2 ERB, loudness range +3 dB, envelope 2.4 dB rms; TimGM6mb: centroid -3.0 ERB, attack +0.4 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 3.2 dB rms; Jnsgm2: centroid -0.3 ERB, attack -0.2 ERB, velocity brightening +0.2 ERB, loudness range +3 dB, envelope 1.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -1.31 | 0.02 | -11.3 | -12.2 | -2.87 | 0.9 |  |
| attack time (-20 to -1 dB) (ms) | 13.0 | 10.0 | 7 | 8 | 9 | 2.4 | yes |
| spectral shape of the attack, note for note | 15.6 (nearest 13) | fonts among themselves 12.4 | | | | 1.3 |  |
| brightness over the keyboard (ERB/oct) | 0.04 | 2.11 | 2.54 | 5.14 | 1.33 | 1.2 | yes |
| key-to-key brightness scatter (ERB) | 2.55 | 2.59 | 1.75 | 0.39 | 1.67 | 0.8 |  |
| brightening from velocity 40 to 127 (ERB) | 1.19 | 0 | -0 | -0 | 0.39 | 4.0 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 19.1 | 16.0 | 22.6 | 26.9 | 16.2 | 0.1 |  |
| decay rate of a held note (dB/s) | 16.3 | 10.6 | 7.2 | 19.2 | 7.7 | 1.2 |  |
| onset delay (ms) | 14.0 | 11.0 | 4 | 10.0 | 13.0 | 0.3 |  |
| Plomp-Levelt dissonance of a note (log2) | -1.42 | -2.63 | -1.1 | -1.15 | -2.6 | 0.6 |  |
| envelope curve, note for note (dB rms) | 9.46 (nearest 8.08) | fonts among themselves 8.58 | | | | 1.1 |  |
| pitch re 12-TET A440 (median) (c) | 1.6 | -0.1 | 4.8 | 1.8 | 2.3 | 0.1 |  |
| pitch scatter from key to key (c) | 3.6 | 1 | 0.7 | 1.5 | 3.3 | 0.8 |  |
| noisiness (spectral flatness) (dB) | -34.4 | -41.8 | -47.4 | -44.6 | -36.0 | 1.5 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -54.2 | -63.6 | -66.2 | -114 | -70.0 | 0.5 | yes |
| pitch at the start re settled (c) | -2.1 | 3.4 | 3.6 | 6.8 | 3.6 | 1.1 | yes |
| stereo coherence | 0.318 | 0.396 | 1 | 1 | 1 | 2.3 | yes |
| partial-to-partial jaggedness (dB) | 6.7 | 3.9 | 1.9 | 2.1 | 3.6 | 3.9 | yes |
| dip between legato notes (dB) | 5.4 | 6.6 | 3.7 | 5.3 | 4.5 | 0.2 |  |
| level evenness of a legato scale (dB) | 1.3 | 0.8 | 0.7 | 0.6 | 1.7 | 0.6 |  |
| level over the keyboard (dB/oct) | -1.04 | -2.17 | -1.82 | -1.25 | -2.32 | 1.0 |  |
| level against the other instruments (velocity 96) (dB) | -3.6 | -3.6 | -1.7 | -2.4 | 0.4 | 0.8 |  |
| key-to-key level scatter (dB) | 2.67 | 0.44 | 0.53 | 0.06 | 0.62 | 4.4 | yes |
| odd / even partials (dB) | -3.5 | -2.9 | -6.1 | -4.5 | -2.5 | 0.1 |  |
| onset jitter in a phrase (ms) | 46.1 | 1.8 | 1.6 | 7.4 | 8.4 | 8.3 | yes |
| onset in a phrase (re the note-on) (ms) | 12.0 | 2 | 0 | 7 | 23.0 | 0.7 |  |
| onset sharpness (dB/ms) | 12.3 | 9.3 | 11.4 | 14.2 | 9.7 | 0.7 |  |
| overtone levels, note for note (dB rms) | 13.4 (nearest 12.7) | fonts among themselves 8.82 | | | | 1.5 | yes |
| panning across the keys (dB/oct) | 0.26 | 0 | 0 | 0 | -0.01 | 0.3 |  |
| partial envelopes alike | 0.81 | 0.208 | 0.982 | 0.919 | 0.469 | 0.3 |  |
| release time (30 dB after note-off) (ms) | 1262 | 1401 | 1152 | 976 | 846 | 0.7 |  |
| spectrogram (1 - correlation), note for note | 0.476 (nearest 0.384) | fonts among themselves 0.233 | | | | 2.0 | yes |
| spectral shape of the body, note for note | 18.4 (nearest 14.3) | fonts among themselves 11.9 | | | | 1.5 |  |
| sounding length of a 100 ms note (ms) | 400 | 387 | 400 | 400 | 316 | 0.1 |  |
| partial stretch (inharmonicity) (c/partial) | 0.33 | 0.1 | 0 | 0.11 | 0.02 | 0.5 |  |
| level after 2 s held, re peak (dB) | -33.5 | -31.0 | -20.8 | -44.6 | -21.4 | 0.6 |  |
| high bands lag low bands by (ms) | 17.0 | 21.0 | 29.0 | 37.0 | 11.5 | 0.6 |  |
| transient coherence: attack spread over bands (ms) | 5.5 | 16.8 | 19.6 | 20.2 | 5.6 | 1.7 |  |
| fundamental share (tristimulus 1) | 0.007 | 0.005 | 0.189 | 0.145 | 0.007 | 0.7 |  |
| partials 2-4 share (tristimulus 2) | 0.449 | 0.353 | 0.125 | 0.153 | 0.44 | 1.2 |  |
| partials 5+ share (tristimulus 3) | 0.434 | 0.296 | 0.678 | 0.696 | 0.424 | 0.6 |  |
| loudness range, velocity 40 to 127 (dB) | 16.2 | 16.7 | 16.6 | 16.7 | 20.2 | 0.3 |  |
| pitch wobble / vibrato (p-p) (c) | 10.1 | 2.7 | 1 | 6.4 | 9.8 | 1.1 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0286 (nearest 0.0243) | fonts among themselves 0.021 | | | | 1.4 |  |

</details>

### 106 Banjo: banjo

Distance 2.45 (rank 84 of 129); the fonts disagree 1.2 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Pitch scatter (11.6 c vs 0.7–4.3)**: probable FAULT. That is audible in chords, and README `make test` only checks three notes to ~7 c. Check the stopped-string retuning (`stopString`) against the light bridge and head.
- **Rough, bright, sharp onset (dissonance −0.04 vs −3.2..−4.5 log2; tristimulus 3 0.64 vs 0.10–0.26; onset 51 vs 11–18 dB/ms)**: DELIBERATE. The Mylar head's 500–800 Hz formant and fast death are what a banjo is (README "Banjo"; banjo_koto.md 2.2–2.3). The fonts are tamer.
- **Louder (+4.6 dB vs −6.6..+0.2)**: the common-loudness rule. A small trim would match GM balance.
- **Stereo coherence 0.43**: room ARTIFACT.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **Plomp-Levelt dissonance of a note**: rougher (-0.04 log2 | -3.17 | -3.7 | -4.51 | -4.41), outside the fonts, 6.0 x.
2. **onset sharpness**: a sharper onset (49.1 dB/ms | 10.9 | 12.8 | 12.8 | 17.2), outside the fonts, 11.5 x.
3. **partials 5+ share (tristimulus 3)**: stronger high partials (0.638 | 0.24 | 0.262 | 0.155 | 0.096), outside the fonts, 5.3 x.
4. **pitch scatter from key to key**: less evenly tuned (11.6 c | 4.3 | 0.7 | 1.2 | 0.9), outside the fonts, 3.5 x.
5. **stereo coherence [room]**: wider (0.428 | 1 | 1 | 1 | 1), outside the fonts, 7.6 x.
6. **decay rate of a held note**: decays faster (33.7 dB/s | 18.8 | 11.2 | 13.1 | 21.2), outside the fonts, 2.4 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -13 | +2 | +9 | +11 | +1 | -14 |
| re TimGM6mb | -1 | +1 | +8 | +7 | -2 | -10 |
| re Jnsgm2 | -4 | -2 | +3 | +9 | +0 | -7 |
| re SGM | -4 | -4 | +2 | +5 | -2 | -1 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.1 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 1.4 dB rms; TimGM6mb: centroid -0.5 ERB, attack +0.0 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 2.2 dB rms; Jnsgm2: centroid -0.2 ERB, attack -0.2 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 1.9 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.2 | 0.72 | -0.1 | 0.48 | 0.01 | 0.1 |  |
| attack time (-20 to -1 dB) (ms) | 5 | 16.0 | 7 | 7 | 8 | 1.0 | yes |
| spectral shape of the attack, note for note | 13.5 (nearest 11.8) | fonts among themselves 8.59 | | | | 1.6 | yes |
| brightness over the keyboard (ERB/oct) | 0.25 | 4.67 | 4.21 | 2.2 | 1.91 | 2.1 | yes |
| key-to-key brightness scatter (ERB) | 1.38 | 1.93 | 0.372 | 0.837 | 1.06 | 0.5 |  |
| brightening from velocity 40 to 127 (ERB) | -0.08 | 0.16 | -0 | 0 | 0.11 | 0.5 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 16.4 | 13.1 | 16.0 | 13.2 | 15.0 | 1.6 | yes |
| decay rate of a held note (dB/s) | 33.7 | 18.8 | 11.2 | 13.1 | 21.2 | 2.4 | yes |
| onset delay (ms) | 12.0 | 10.0 | 3 | 3 | 4 | 0.8 |  |
| Plomp-Levelt dissonance of a note (log2) | -0.04 | -3.17 | -3.7 | -4.51 | -4.41 | 6.0 | yes |
| envelope curve, note for note (dB rms) | 16.1 (nearest 10.6) | fonts among themselves 12.3 | | | | 1.3 |  |
| pitch re 12-TET A440 (median) (c) | 4.8 | -4 | 2.4 | 1.7 | 4 | 0.7 |  |
| pitch scatter from key to key (c) | 11.6 | 4.3 | 0.7 | 1.2 | 0.9 | 3.5 | yes |
| noisiness (spectral flatness) (dB) | -32.4 | -56.4 | -53.9 | -53.6 | -42.8 | 3.1 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -129 | -71.3 | -124 | -124 | -113 | 0.4 | yes |
| pitch at the start re settled (c) | -4.2 | 4 | 1.6 | 3.1 | 3.7 | 1.5 | yes |
| stereo coherence | 0.428 | 1 | 1 | 1 | 1 | 7.6 | yes |
| partial-to-partial jaggedness (dB) | 3.8 | 4.6 | 4.3 | 3.8 | 4.7 | 0.6 |  |
| dip between legato notes (dB) | 8.2 | 9.5 | 9 | 9.8 | 6.9 | 0.5 |  |
| level evenness of a legato scale (dB) | 1.6 | 0.6 | 0.9 | 0.9 | 2.1 | 0.7 |  |
| level over the keyboard (dB/oct) | -1.1 | -0.14 | -1.96 | -2.63 | -1.53 | 0.5 |  |
| level against the other instruments (velocity 96) (dB) | -2.7 | -6.6 | -0.7 | -1.2 | 0.2 | 0.5 |  |
| key-to-key level scatter (dB) | 1.85 | 2 | 0.28 | 0.24 | 1.01 | 1.4 |  |
| odd / even partials (dB) | -2.2 | -3.4 | 2 | 0.9 | -4.2 | 0.3 |  |
| onset jitter in a phrase (ms) | 2.3 | 6.7 | 0.4 | 0.5 | 1.4 | 0.3 |  |
| onset in a phrase (re the note-on) (ms) | 7 | 4 | 0 | 0 | 0 | 0.7 |  |
| onset sharpness (dB/ms) | 49.1 | 10.9 | 12.8 | 12.8 | 17.2 | 11.5 | yes |
| overtone levels, note for note (dB rms) | 11 (nearest 8.47) | fonts among themselves 8.74 | | | | 1.3 |  |
| panning across the keys (dB/oct) | -0.16 | -0 | 0 | 0 | -0.01 | 0.2 |  |
| partial envelopes alike | 0.948 | 0.938 | 0.997 | 0.919 | 0.99 | 0.2 |  |
| spectrogram (1 - correlation), note for note | 0.386 (nearest 0.202) | fonts among themselves 0.298 | | | | 1.3 |  |
| spectral shape of the body, note for note | 16.9 (nearest 13.5) | fonts among themselves 11.8 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 86.0 | 25.0 | 159 | 165 | 155 | 0.6 |  |
| partial stretch (inharmonicity) (c/partial) | 1.18 | 0.48 | 0 | 0.49 | 0.54 | 1.4 | yes |
| level after 2 s held, re peak (dB) | -79.5 | -50.1 | -30.4 | -36.8 | -110 | 0.9 |  |
| high bands lag low bands by (ms) | 1 | -0.2 | 0 | 0 | 1 | 0.2 |  |
| transient coherence: attack spread over bands (ms) | 0.8 | 2 | 0 | 0 | 0.8 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.027 | 0.275 | 0.004 | 0.004 | 0.045 | 0.0 |  |
| partials 2-4 share (tristimulus 2) | 0.276 | 0.47 | 0.734 | 0.716 | 0.445 | 2.2 | yes |
| partials 5+ share (tristimulus 3) | 0.638 | 0.24 | 0.262 | 0.155 | 0.096 | 5.3 | yes |
| loudness range, velocity 40 to 127 (dB) | 15.3 | 16.7 | 16.7 | 16.7 | 20.1 | 0.8 | yes |
| pitch wobble / vibrato (p-p) (c) | 56.0 | 15.3 | 0.2 | 3 | 53.0 | 1.8 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0168 (nearest 0.0142) | fonts among themselves 0.0135 | | | | 1.2 |  |

</details>

### 107 Shamisen: shamisen

Distance 3.51 (rank 34 of 129); the fonts disagree 0.8 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Louder (+4.4 dB vs −7.8..−4.8)**: FAULT. The fonts agree closely (they disagree by only 0.8 JND), so this is a trim of ~9 dB against the GM balance.
- **Darker (10.3 vs 14.2–14.4 ERB) with +13..+15 dB below 150 Hz, but strong high partials (tristimulus 3 0.46 vs 0.06–0.10)**: partly DELIBERATE. The skin's low modes and the bachi's knock on the skin (README "Shamisen"; sitar_shamisen.md 2.3–2.4) put weight at the bottom; the sawari buzz feeds the top. All four fonts sit ~4 ERB brighter, a thin plucked twang. Worth a listen: the boom may be too heavy.
- **Pitch wobble 21 c vs 0–1**: sawari contact and tension modulation. DELIBERATE, though large.
- **Stereo coherence 0.35**: room ARTIFACT.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: darker (10.3 ERB | 14.4 | 14.2 | 14.2 | 14.3), outside the fonts, 13.1 x.
2. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.443, nearest 0.43; the fonts among themselves 0.0587), outside.
3. **partials 5+ share (tristimulus 3)**: stronger high partials (0.534 | 0.062 | 0.09 | 0.09 | 0.099), outside the fonts, 6.3 x.
4. **pitch wobble / vibrato (p-p)**: wobbles more (22.7 c | 0.5 | 0 | 0.5 | 1), outside the fonts, 4.4 x.
5. **stereo coherence [room]**: wider (0.345 | 0.968 | 1 | 1 | 1), outside the fonts, 8.7 x.
6. **noisiness (spectral flatness)**: noisier (-33.0 dB | -60.7 | -60.8 | -62.8 | -73.1), outside the fonts, 4.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +15 | +4 | -9 | -5 | +2 | +7 |
| re TimGM6mb | +14 | +5 | -5 | -4 | -4 | +8 |
| re Jnsgm2 | +14 | +5 | -5 | -4 | -6 | +10 |
| re SGM | +13 | +4 | -6 | -5 | -3 | +12 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -5 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -17 -8 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.3 ERB, attack +0.0 ERB, velocity brightening +0.4 ERB, loudness range +3 dB, envelope 4.5 dB rms; TimGM6mb: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 2.1 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 2.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 1.5 | 2.36 | 1.49 | 1.5 | 0.96 | 0.0 |  |
| attack time (-20 to -1 dB) (ms) | 2.5 | 14.0 | 2 | 2 | 3 | 0.0 |  |
| spectral shape of the attack, note for note | 17.4 (nearest 15.2) | fonts among themselves 7.9 | | | | 2.2 | yes |
| brightness over the keyboard (ERB/oct) | 0.55 | 1.05 | 2.47 | 2.48 | 2.46 | 2.7 | yes |
| key-to-key brightness scatter (ERB) | 1.81 | 1.03 | 1.29 | 1.29 | 1.22 | 2.8 | yes |
| brightening from velocity 40 to 127 (ERB) | -0.7 | 0 | 0 | -0 | 0.93 | 1.5 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 10.3 | 14.4 | 14.2 | 14.2 | 14.3 | 13.1 | yes |
| decay rate of a held note (dB/s) | 30.5 | 17.1 | 23.3 | 23.3 | 23.2 | 1.6 | yes |
| onset delay (ms) | 10.0 | 15.0 | 0 | 0 | 3 | 0.8 |  |
| Plomp-Levelt dissonance of a note (log2) | -0.84 | -5.64 | -4.27 | -5.18 | -3.13 | 3.1 | yes |
| envelope curve, note for note (dB rms) | 11.8 (nearest 11.4) | fonts among themselves 3.5 | | | | 3.4 | yes |
| pitch re 12-TET A440 (median) (c) | 2.8 | -0.8 | 2.6 | 2.2 | 3.5 | 0.1 |  |
| pitch scatter from key to key (c) | 3.3 | 0.4 | 0.7 | 0.7 | 0.3 | 0.9 | yes |
| noisiness (spectral flatness) (dB) | -33.0 | -60.7 | -60.8 | -62.8 | -73.1 | 4.6 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -122 | -123 | -121 | -120 | -108 | 0.3 |  |
| pitch at the start re settled (c) | 0.8 | 7.6 | 0.7 | 1.8 | 1.3 | 0.1 |  |
| stereo coherence | 0.345 | 0.968 | 1 | 1 | 1 | 8.7 | yes |
| partial-to-partial jaggedness (dB) | 3.1 | 4.3 | 3.3 | 2.6 | 3.4 | 0.2 |  |
| dip between legato notes (dB) | 8.6 | 7.6 | 8 | 8 | 7.7 | 0.4 |  |
| level evenness of a legato scale (dB) | 1.2 | 0.3 | 0.6 | 0.7 | 2 | 0.6 |  |
| level over the keyboard (dB/oct) | -0.62 | -0.74 | -1.31 | -1.14 | -1.84 | 0.6 |  |
| level against the other instruments (velocity 96) (dB) | -5.8 | -6.8 | -7.2 | -7.8 | -4.8 | 0.8 |  |
| key-to-key level scatter (dB) | 1.89 | 0.36 | 1.22 | 1.51 | 2.08 | 0.6 |  |
| odd / even partials (dB) | -0.4 | -0.7 | -0.4 | -0.4 | 0.3 | 0.0 |  |
| onset jitter in a phrase (ms) | 0.4 | 5.6 | 38.3 | 38.3 | 0.7 | 1.1 |  |
| onset in a phrase (re the note-on) (ms) | 7 | 3 | -2 | -2 | -1 | 0.8 |  |
| onset sharpness (dB/ms) | 59.9 | 18.9 | 2.2 | 2.1 | 51.8 | 2.0 | yes |
| overtone levels, note for note (dB rms) | 11.6 (nearest 10.7) | fonts among themselves 3.63 | | | | 3.2 | yes |
| panning across the keys (dB/oct) | -0.09 | 0 | -0.1 | 0 | -0.01 | 0.1 |  |
| partial envelopes alike | 0.933 | 0.974 | 1 | 1 | 1 | 0.7 |  |
| release time (30 dB after note-off) (ms) | 268 | 324 | 220 | 228 | 243 | 0.7 |  |
| spectrogram (1 - correlation), note for note | 0.443 (nearest 0.43) | fonts among themselves 0.0587 | | | | 7.5 | yes |
| spectral shape of the body, note for note | 16.6 (nearest 16.1) | fonts among themselves 6.35 | | | | 2.6 | yes |
| sounding length of a 100 ms note (ms) | 136 | 115 | 53.0 | 53.0 | 37.0 | 1.7 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.35 | -0.01 | 0 | 0 | 0 | 0.7 | yes |
| level after 2 s held, re peak (dB) | -69.2 | -41.2 | -55.5 | -53.8 | -53.2 | 2.2 | yes |
| high bands lag low bands by (ms) | -0.4 | -1 | 0 | 0 | 0 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 1 | 0.9 | 0 | 0 | 0 | 0.2 |  |
| fundamental share (tristimulus 1) | 0.038 | 0.002 | 0.001 | 0.001 | 0.001 | 0.5 | yes |
| partials 2-4 share (tristimulus 2) | 0.288 | 0.242 | 0.909 | 0.909 | 0.797 | 1.7 |  |
| partials 5+ share (tristimulus 3) | 0.534 | 0.062 | 0.09 | 0.09 | 0.099 | 6.3 | yes |
| loudness range, velocity 40 to 127 (dB) | 14.5 | 16.7 | 16.6 | 16.6 | 22.1 | 0.8 | yes |
| pitch wobble / vibrato (p-p) (c) | 22.7 | 0.5 | 0 | 0.5 | 1 | 4.4 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0169 (nearest 0.0144) | fonts among themselves 0.00538 | | | | 3.1 | yes |

</details>

### 108 Koto: koto

Distance 1.56 (rank 120 of 129); the fonts disagree 0.9 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Darker and duller attack (10.4 vs 13.6–18.3 ERB; attack −1.4 vs +0.5..+3.5 ERB re body; −7..−17 dB above 6 kHz)**: probable FAULT. A koto played with ivory tsume should have a bright, twangy attack (banjo_koto.md 7.1). The pluck (`rise 0.002`) seems too soft.
- **Uneven onsets in a phrase (jitter 38.5 ms vs 0.7–1.5)**: check. The E-major phrase (keys 52–61) falls off hirajōshi: 4 of the 6 notes are pressed (oshide) or on a moved bridge, and two pairs share a string. A string still sounding is damped and replucked 20 ms into the next note.
- **Rings 1268 ms after the key (vs 562–812)**: DELIBERATE (the strings are not damped on key release) plus the room.
- **Louder (−0.4 vs −5.1..−2.1 dB)**: a small trim.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (1268 ms | 562 | 570 | 812 | 633), outside the fonts, 4.1 x.
2. **Plomp-Levelt dissonance of a note**: rougher (-1.08 log2 | -4.16 | -3.16 | -3.61 | -3.37), outside the fonts, 4.8 x.
3. **brightness (spectral centroid, velocity 96)**: darker (10.4 ERB | 13.6 | 18.3 | 16.1 | 13.8), outside the fonts, 1.9 x.
4. **onset jitter in a phrase**: more uneven onsets (38.5 ms | 0.7 | 1.5 | 1.2 | 0.7), outside the fonts, 7.5 x.
5. **attack brighter than the body by**: a duller attack (-1.41 ERB | 1.67 | 0.51 | 3.48 | 1.76), outside the fonts, 2.1 x.
6. **partial stretch (inharmonicity)**: more stretched partials (1.35 c/partial | 0 | 0 | 0.22 | 0.46), outside the fonts, 2.5 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +10 | +5 | +2 | +6 | -4 | -17 |
| re TimGM6mb | +30 | +12 | -2 | +3 | -11 | -17 |
| re Jnsgm2 | +28 | +9 | -4 | +1 | -7 | -14 |
| re SGM | +8 | +0 | -6 | +1 | +2 | -7 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 2.0 dB rms; TimGM6mb: centroid -0.3 ERB, attack +0.1 ERB, velocity brightening +0.2 ERB, loudness range +4 dB, envelope 1.9 dB rms; Jnsgm2: centroid -0.1 ERB, attack -0.1 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 1.7 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -1.41 | 1.67 | 0.51 | 3.48 | 1.76 | 2.1 | yes |
| attack time (-20 to -1 dB) (ms) | 3 | 3 | 5 | 5 | 5 | 2.0 |  |
| spectral shape of the attack, note for note | 16.3 (nearest 12.7) | fonts among themselves 10.1 | | | | 1.6 |  |
| brightness over the keyboard (ERB/oct) | 1.33 | 4.22 | 3.73 | 2.04 | 1.99 | 1.4 | yes |
| key-to-key brightness scatter (ERB) | 1.01 | 0.142 | 1.07 | 1.39 | 1.71 | 0.3 |  |
| brightening from velocity 40 to 127 (ERB) | 0 | -0 | -0 | 0 | 0.06 | 0.0 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 10.4 | 13.6 | 18.3 | 16.1 | 13.8 | 1.9 | yes |
| decay rate of a held note (dB/s) | 19.1 | 18.9 | 17.0 | 19.6 | 15.9 | 0.4 |  |
| onset delay (ms) | 10.0 | 2 | 7 | 7 | 3 | 0.5 |  |
| Plomp-Levelt dissonance of a note (log2) | -1.08 | -4.16 | -3.16 | -3.61 | -3.37 | 4.8 | yes |
| envelope curve, note for note (dB rms) | 7.73 (nearest 6.6) | fonts among themselves 7.68 | | | | 1.0 |  |
| pitch re 12-TET A440 (median) (c) | 1.6 | -1.9 | 3.4 | 4.9 | -1.9 | 0.2 |  |
| pitch scatter from key to key (c) | 1.9 | 4.6 | 0.9 | 1.8 | 3.4 | 0.2 |  |
| noisiness (spectral flatness) (dB) | -45.1 | -54.4 | -54.0 | -48.9 | -41.3 | 1.0 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -76.3 | -124 | -122 | -123 | -90.0 | 2.7 | yes |
| pitch at the start re settled (c) | -4.6 | 2.5 | 0.1 | -1.4 | -0.8 | 0.8 | yes |
| stereo coherence | 0.281 | 1 | 1 | 1 | 0.496 | 2.9 | yes |
| partial-to-partial jaggedness (dB) | 5.5 | 3.6 | 4.1 | 3.8 | 3.6 | 1.8 | yes |
| dip between legato notes (dB) | 10.7 | 10.7 | 8.5 | 9.7 | 7.6 | 0.8 |  |
| level evenness of a legato scale (dB) | 1.3 | 0.4 | 1 | 0.8 | 1.2 | 0.4 |  |
| level over the keyboard (dB/oct) | -1.13 | -1.53 | -0.71 | -1.78 | -2.12 | 0.5 |  |
| level against the other instruments (velocity 96) (dB) | -1.6 | -5.1 | -2.1 | -3.8 | -2.5 | 1.0 |  |
| key-to-key level scatter (dB) | 1.16 | 0.35 | 0.84 | 0.77 | 1.42 | 0.7 |  |
| odd / even partials (dB) | -1 | 4.3 | -2.1 | -3.8 | -5 | 0.4 |  |
| onset jitter in a phrase (ms) | 38.5 | 0.7 | 1.5 | 1.2 | 0.7 | 7.5 | yes |
| onset in a phrase (re the note-on) (ms) | 8 | 0 | 5 | 4 | -2 | 0.6 |  |
| onset sharpness (dB/ms) | 39.5 | 18.9 | 18.6 | 18.6 | 43.8 | 1.6 |  |
| overtone levels, note for note (dB rms) | 12.1 (nearest 10.9) | fonts among themselves 9.59 | | | | 1.3 |  |
| panning across the keys (dB/oct) | -0.48 | 0 | 0.1 | 0.1 | -0.7 | 0.5 |  |
| partial envelopes alike | 0.848 | 0.999 | 1 | 0.961 | 0.885 | 1.3 |  |
| release time (30 dB after note-off) (ms) | 1268 | 562 | 570 | 812 | 633 | 4.1 | yes |
| spectrogram (1 - correlation), note for note | 0.342 (nearest 0.298) | fonts among themselves 0.21 | | | | 1.6 | yes |
| spectral shape of the body, note for note | 19.2 (nearest 13.1) | fonts among themselves 10.3 | | | | 1.9 |  |
| sounding length of a 100 ms note (ms) | 147 | 170 | 179 | 179 | 239 | 1.1 | yes |
| partial stretch (inharmonicity) (c/partial) | 1.35 | 0 | 0 | 0.22 | 0.46 | 2.5 | yes |
| level after 2 s held, re peak (dB) | -47.0 | -49.8 | -41.5 | -51.2 | -39.2 | 0.2 |  |
| high bands lag low bands by (ms) | 1.8 | -0.5 | -4.3 | -3.7 | 0 | 0.8 |  |
| transient coherence: attack spread over bands (ms) | 0.7 | 0.1 | 1.7 | 1.7 | 0 | 0.0 |  |
| fundamental share (tristimulus 1) | 0.044 | 0.168 | 0.039 | 0.192 | 0.152 | 1.5 |  |
| partials 2-4 share (tristimulus 2) | 0.493 | 0.47 | 0.507 | 0.475 | 0.544 | 0.0 |  |
| partials 5+ share (tristimulus 3) | 0.387 | 0.28 | 0.414 | 0.315 | 0.298 | 1.2 |  |
| loudness range, velocity 40 to 127 (dB) | 15.5 | 16.6 | 16.6 | 16.6 | 20.1 | 0.6 | yes |
| pitch wobble / vibrato (p-p) (c) | 9.4 | 6.4 | 0 | 2.1 | 5 | 1.2 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0171 (nearest 0.0161) | fonts among themselves 0.0105 | | | | 1.6 | yes |

</details>

### 109 Kalimba: kalimba

Distance 2.78 (rank 64 of 129); the fonts disagree 0.5 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Speaks late (phrase onset 28 ms vs −3..0; onset delay 31 ms; jitter 29.7 ms)**: borderline FAULT. README "Kalimba" calls for a slow press; the press is the pluck's 20 ms `rise`. Real, but in a band it sounds behind the beat. Halving it would keep the character.
- **Brighter attack (+5.5 vs 0.3–1.6 ERB) and brighter overall (8.1 vs 7.0 ERB)**: DELIBERATE: the nail's quick release (kalimba_fiddle.md A6). The −30..−40 dB below 150 Hz is a fold ARTIFACT: probe keys 36–44 fold up into the 48–88 compass.
- **Rings on: 100 ms note sounds 400 ms (vs 45–204), decay 20.9 vs 30–47 dB/s**: DELIBERATE. A tine cannot be damped by letting the key go.
- **Brightness scatter 1.33 vs 0.21–0.60 ERB**: per-tine tuning and support. Minor.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **attack brighter than the body by**: a brighter attack (5.54 ERB | 1.6 | 0.3 | 0.3 | 0.82), outside the fonts, 7.7 x.
2. **brightness (spectral centroid, velocity 96)**: brighter (8.06 ERB | 7.02 | 6.99 | 6.99 | 7.01), outside the fonts, 3.5 x.
3. **key-to-key brightness scatter**: less even in tone from key to key (1.33 ERB | 0.214 | 0.217 | 0.217 | 0.598), outside the fonts, 5.6 x.
4. **decay rate of a held note**: decays slower (20.9 dB/s | 46.6 | 29.6 | 31.4 | 46.2), outside the fonts, 2.6 x.
5. **Plomp-Levelt dissonance of a note**: rougher (-3.16 log2 | -6.9 | -9.97 | -8.38 | -7.38), outside the fonts, 3.1 x.
6. **pitch wobble / vibrato (p-p)**: wobbles more (16.1 c | 2.6 | 0.3 | 0.8 | 0.8), outside the fonts, 3.1 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -30 | -4 | -2 | +6 | +11 | +6 |
| re TimGM6mb | -38 | -7 | -7 | +10 | +15 | +20 |
| re Jnsgm2 | -40 | -9 | -9 | +10 | +20 | +14 |
| re SGM | -32 | -4 | -3 | +10 | +9 | +0 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.2 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 2.8 dB rms; TimGM6mb: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.5 ERB, loudness range +4 dB, envelope 2.3 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.5 ERB, loudness range +3 dB, envelope 2.3 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 5.54 | 1.6 | 0.3 | 0.3 | 0.82 | 7.7 | yes |
| attack time (-20 to -1 dB) (ms) | 19.0 | 3 | 15.0 | 15.0 | 2 | 1.0 | yes |
| spectral shape of the attack, note for note | 13.9 (nearest 12.6) | fonts among themselves 9.59 | | | | 1.4 | yes |
| brightness over the keyboard (ERB/oct) | 3.02 | 3.47 | 3.49 | 3.49 | 3.37 | 1.5 | yes |
| key-to-key brightness scatter (ERB) | 1.33 | 0.214 | 0.217 | 0.217 | 0.598 | 5.6 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.39 | -0 | -0 | -0 | -0.36 | 1.3 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 8.06 | 7.02 | 6.99 | 6.99 | 7.01 | 3.5 | yes |
| decay rate of a held note (dB/s) | 20.9 | 46.6 | 29.6 | 31.4 | 46.2 | 2.6 | yes |
| onset delay (ms) | 31.0 | 4 | 9 | 9 | 2 | 2.5 | yes |
| Plomp-Levelt dissonance of a note (log2) | -3.16 | -6.9 | -9.97 | -8.38 | -7.38 | 3.1 | yes |
| envelope curve, note for note (dB rms) | 18.6 (nearest 15.9) | fonts among themselves 11.7 | | | | 1.6 | yes |
| pitch re 12-TET A440 (median) (c) | 0.1 | -0.7 | 2.4 | 2.2 | 0 | 0.2 |  |
| pitch scatter from key to key (c) | 2.1 | 0.9 | 0.4 | 0.7 | 0.1 | 0.5 |  |
| noisiness (spectral flatness) (dB) | -50.8 | -47.0 | -68.2 | -64.9 | -52.1 | 0.7 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -77.1 | -122 | -125 | -124 | -107 | 5.0 | yes |
| pitch at the start re settled (c) | 1.8 | 0.1 | 0 | 0.7 | 1 | 0.3 |  |
| stereo coherence | 0.601 | 1 | 1 | 1 | 1 | 5.3 | yes |
| partial-to-partial jaggedness (dB) | 5.6 | 4.7 | 3.3 | 4.15 | 6.1 | 0.8 |  |
| dip between legato notes (dB) | 9.9 | 9.9 | 6.8 | 7.8 | 9.8 | 0.5 |  |
| level evenness of a legato scale (dB) | 2.9 | 1 | 0.5 | 0.4 | 0.9 | 2.2 | yes |
| level over the keyboard (dB/oct) | -0.04 | 1.4 | -0.75 | -1.06 | 0.04 | 0.3 |  |
| level against the other instruments (velocity 96) (dB) | -3.9 | -11.1 | -0.9 | -1.9 | -6.3 | 0.0 |  |
| key-to-key level scatter (dB) | 2.38 | 2.47 | 0.23 | 0.37 | 2.35 | 0.9 |  |
| odd / even partials (dB) | -24.9 | -12.1 | -6.1 | -3.7 | -24.5 | 1.5 |  |
| onset jitter in a phrase (ms) | 6.2 | 0.8 | 1.3 | 1.1 | 1 | 1.0 | yes |
| onset in a phrase (re the note-on) (ms) | 28.0 | -2 | 0 | -0 | -3 | 2.9 | yes |
| onset sharpness (dB/ms) | 25.6 | 6.9 | 14.1 | 14.2 | 52.1 | 0.5 |  |
| overtone levels, note for note (dB rms) | 18.2 (nearest 18) | fonts among themselves 15.9 | | | | 1.1 |  |
| panning across the keys (dB/oct) | 0.14 | 0 | 0 | 0 | -0.01 | 0.1 |  |
| partial envelopes alike | 0.954 | 0.988 | 0.959 | 0.954 | 0.985 | 0.2 |  |
| spectrogram (1 - correlation), note for note | 0.316 (nearest 0.264) | fonts among themselves 0.183 | | | | 1.7 | yes |
| spectral shape of the body, note for note | 10.4 (nearest 10.1) | fonts among themselves 7.3 | | | | 1.4 | yes |
| sounding length of a 100 ms note (ms) | 400 | 55.0 | 204 | 195 | 45.0 | 1.8 | yes |
| level after 2 s held, re peak (dB) | -39.6 | -122 | -70.7 | -76.3 | -82.8 | 1.5 | yes |
| high bands lag low bands by (ms) | -0.7 | 0 | -2 | -2 | -0.5 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 1.1 | 0 | 1.3 | 1.2 | 0.4 | 0.1 |  |
| fundamental share (tristimulus 1) | 0.984 | 0.997 | 1 | 1 | 0.995 | 0.2 |  |
| partials 2-4 share (tristimulus 2) | 0 | 0.003 | 0 | 0 | 0.005 | 0.0 |  |
| partials 5+ share (tristimulus 3) | 0.009 | 0 | 0 | 0 | 0 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 16.7 | 16.6 | 16.6 | 16.7 | 20.1 | 0.0 |  |
| pitch wobble / vibrato (p-p) (c) | 16.1 | 2.6 | 0.3 | 0.8 | 0.8 | 3.1 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0139 (nearest 0.0113) | fonts among themselves 0.00364 | | | | 3.8 | yes |

</details>

### 110 Bagpipe: bagpipe

Distance 4.98 (rank 4 of 129); the fonts disagree 1.1 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Darker, hollow, fundamental-heavy (8.3 vs 16.2–18.3 ERB; odd/even +38 vs −2.8..+1.3 dB; tristimulus 1 0.68 vs 0.01–0.16)**: MEASUREMENT ARTIFACT. The chanter covers only G4–A5 (67–81, src/designs.cc). Probe keys 36–64 and the whole phrase (52–61) play the **drones**, which are cylindrical pipes with odd harmonics (README "Ethnic and percussive", drone paragraph). Transpose is 0, so there is no transposed-wind shift.
- **Brightness scatter 2.7 ERB, speaks late (205 vs 10–37 ms)**: the same drone/chanter split, and the drone reed's build-up.
- **Design question**: a GM bagpipe melody written below G4 becomes a drone at that pitch, not a chanter note. That is DELIBERATE, but the timbre jumps at G4.
- **Release 702 ms**: room ARTIFACT.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: darker (8.33 ERB | 16.2 | 18.3 | 17.6 | 16.8), outside the fonts, 8.3 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (702 ms | 132 | 111 | 153 | 146), outside the fonts, 9.4 x.
3. **key-to-key brightness scatter**: less even in tone from key to key (2.72 ERB | 0.611 | 0.747 | 0.653 | 0.604), outside the fonts, 10.4 x.
4. **onset delay**: speaks later (205 ms | 37.0 | 13.0 | 10.0 | 12.0), outside the fonts, 14.3 x.
5. **fundamental share (tristimulus 1)**: more fundamental (0.681 | 0.023 | 0.016 | 0.014 | 0.163), outside the fonts, 8.9 x.
6. **odd / even partials**: more odd partials (hollower) (38.0 dB | -2.3 | 1.3 | -1.1 | -2.8), outside the fonts, 19.4 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +5 | +3 | -6 | -7 | -5 | +11 |
| re TimGM6mb | +19 | +6 | -3 | -4 | -5 | -1 |
| re Jnsgm2 | +10 | +7 | -1 | -2 | -3 | -2 |
| re SGM | +6 | +2 | -5 | -6 | -6 | +10 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -18 -8 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +1.5 ERB, attack -0.1 ERB, velocity brightening +0.3 ERB, loudness range +3 dB, envelope 1.8 dB rms; TimGM6mb: centroid -1.8 ERB, attack +0.3 ERB, velocity brightening +0.2 ERB, loudness range +3 dB, envelope 2.9 dB rms; Jnsgm2: centroid -0.5 ERB, attack +0.0 ERB, velocity brightening +0.1 ERB, loudness range +3 dB, envelope 1.7 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -1.72 | 0.2 | -0.09 | 0.06 | 0.7 | 4.6 | yes |
| attack time (-20 to -1 dB) (ms) | 283 | 424 | 39.0 | 78.0 | 43.0 | 1.3 |  |
| spectral shape of the attack, note for note | 16.9 (nearest 15.8) | fonts among themselves 9.18 | | | | 1.8 | yes |
| brightness over the keyboard (ERB/oct) | 5.78 | 3.97 | 3.7 | 4.35 | 4 | 5.5 | yes |
| key-to-key brightness scatter (ERB) | 2.72 | 0.611 | 0.747 | 0.653 | 0.604 | 10.4 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.34 | 0 | 0 | 0 | 3.53 | 0.2 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 8.33 | 16.2 | 18.3 | 17.6 | 16.8 | 8.3 | yes |
| onset delay (ms) | 205 | 37.0 | 13.0 | 10.0 | 12.0 | 14.3 | yes |
| Plomp-Levelt dissonance of a note (log2) | -6.38 | -4.72 | -3.52 | -1.34 | -1.77 | 2.2 | yes |
| envelope curve, note for note (dB rms) | 19.2 (nearest 18.9) | fonts among themselves 3.63 | | | | 5.3 | yes |
| pitch re 12-TET A440 (median) (c) | -1.4 | -1.5 | 0 | -1.4 | -1.8 | 0.0 |  |
| pitch scatter from key to key (c) | 5.1 | 1.8 | 1.8 | 1.5 | 1.9 | 1.1 | yes |
| noisiness (spectral flatness) (dB) | -52.7 | -56.1 | -54.6 | -35.2 | -38.7 | 0.6 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -65.6 | -123 | -121 | -120 | -104 | 5.9 | yes |
| pitch at the start re settled (c) | -2.5 | -0.9 | -6.2 | -0.4 | 1.1 | 0.4 |  |
| stereo coherence | 0.341 | 1 | 1 | 1 | 1 | 8.8 | yes |
| partial-to-partial jaggedness (dB) | 27.8 | 4.1 | 2.9 | 3.5 | 3.5 | 24.2 | yes |
| dip between legato notes (dB) | 8.5 | 5.8 | 3.8 | 4.4 | 6.2 | 1.7 | yes |
| level evenness of a legato scale (dB) | 3.2 | 1 | 3.7 | 3.4 | 3.1 | 0.0 |  |
| level over the keyboard (dB/oct) | 3.19 | 1.09 | 1.42 | 1.55 | -0.55 | 1.8 | yes |
| level against the other instruments (velocity 96) (dB) | -8 | 0.1 | 1.5 | 0.9 | -4.8 | 2.7 | yes |
| key-to-key level scatter (dB) | 3.15 | 1.61 | 1.11 | 1.2 | 3.04 | 1.8 |  |
| odd / even partials (dB) | 38.0 | -2.3 | 1.3 | -1.1 | -2.8 | 19.4 | yes |
| onset jitter in a phrase (ms) | 55.0 | 44.1 | 23.5 | 22.9 | 22.6 | 3.0 | yes |
| onset in a phrase (re the note-on) (ms) | 56.0 | 5 | 0 | 0 | 2 | 5.5 | yes |
| onset sharpness (dB/ms) | 5.2 | 8.3 | 5.1 | 7.4 | 11.2 | 0.9 |  |
| overtone levels, note for note (dB rms) | 32.3 (nearest 30.8) | fonts among themselves 4.25 | | | | 7.6 | yes |
| panning across the keys (dB/oct) | 1.35 | 0 | 0 | 0 | -0.01 | 1.4 | yes |
| partial envelopes alike | 0.303 | 0 | 0.012 | -0.027 | -0.028 | 3.2 | yes |
| release time (30 dB after note-off) (ms) | 702 | 132 | 111 | 153 | 146 | 9.4 | yes |
| spectrogram (1 - correlation), note for note | 0.49 (nearest 0.44) | fonts among themselves 0.111 | | | | 4.4 | yes |
| spectral shape of the body, note for note | 15.5 (nearest 14.9) | fonts among themselves 10.1 | | | | 1.5 | yes |
| sounding length of a 100 ms note (ms) | 156 | 182 | 157 | 185 | 174 | 0.8 |  |
| partial stretch (inharmonicity) (c/partial) | 0.03 | -0.34 | 0.12 | -0.06 | 0.14 | 0.0 |  |
| level after 2 s held, re peak (dB) | -4 | -4.7 | -5.4 | -6.1 | -6.5 | 0.6 |  |
| high bands lag low bands by (ms) | 5.7 | -9 | -2.7 | 0 | -1 | 1.5 | yes |
| transient coherence: attack spread over bands (ms) | 7.7 | 20.8 | 4.5 | 5 | 4.2 | 0.4 |  |
| fundamental share (tristimulus 1) | 0.681 | 0.023 | 0.016 | 0.014 | 0.163 | 8.9 | yes |
| partials 2-4 share (tristimulus 2) | 0.148 | 0.61 | 0.452 | 0.515 | 0.534 | 4.8 | yes |
| partials 5+ share (tristimulus 3) | 0.074 | 0.265 | 0.533 | 0.443 | 0.317 | 2.3 | yes |
| loudness range, velocity 40 to 127 (dB) | 16.6 | 16.7 | 16.7 | 16.7 | 23.4 | 0.0 |  |
| pitch wobble / vibrato (p-p) (c) | 9.6 | 6.9 | 1.7 | 9.2 | 9 | 0.3 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0265 (nearest 0.0248) | fonts among themselves 0.0105 | | | | 2.5 | yes |

</details>

### 111 Fiddle: fiddle

Distance 2.95 (rank 54 of 129); the fonts disagree 1.6 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Missing low end, far too bright (centroid 23.3 vs 9.9–15.3 ERB; −30..−66 dB below 400 Hz; tristimulus 3 0.94 vs 0.04–0.19)**: FAULT, the largest of this group. The fundamental is almost absent. pajupilli's own violin sits inside the fonts (18.3 vs 15.7–19.9 ERB), so the fiddle's waveguide bridge-force-to-body output (README "Fiddle") loses the low string modes or over-weights the bridge hill.
- **Brightness flat over the keyboard (−0.04 vs 3.2–4.3 ERB/oct)**: the same fault. The fixed high band dominates every note.
- **Quieter (−3.9 vs −1.2..+2.8 dB)**: follows from the missing fundamental.
- **Phrase onset 41 ms, release 595 ms**: bow build-up (the fonts span −3..18 ms) and the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **release time (30 dB after note-off) [room]**: rings longer after the key (595 ms | 114 | 97.0 | 125 | 101), outside the fonts, 9.9 x.
2. **brightness (spectral centroid, velocity 96)**: brighter (23.3 ERB | 11.2 | 9.95 | 13.2 | 15.3), outside the fonts, 4.1 x.
3. **partials 5+ share (tristimulus 3)**: stronger high partials (0.941 | 0.043 | 0.134 | 0.16 | 0.188), outside the fonts, 11.0 x.
4. **brightness over the keyboard**: brightens slower up the keyboard (-0.04 ERB/oct | 3.96 | 3.83 | 4.3 | 3.22), outside the fonts, 7.3 x.
5. **spectrogram (1 - correlation), note for note**: pajupilli further from the fonts than they are from each other (median distance to the fonts 0.498, nearest 0.465; the fonts among themselves 0.0985), outside.
6. **onset in a phrase (re the note-on)**: speaks later in a phrase (41.0 ms | 6 | -1 | -3 | 18.0), outside the fonts, 3.7 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -49 | -34 | -10 | +14 | +19 | +15 |
| re TimGM6mb | -66 | -35 | -12 | +10 | +20 | +25 |
| re Jnsgm2 | -57 | -30 | -7 | +12 | +18 | +15 |
| re SGM | -44 | -32 | -12 | +8 | +21 | +18 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -5 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.1 ERB, velocity brightening -0.0 ERB, loudness range +4 dB, envelope 4.3 dB rms; TimGM6mb: centroid -0.0 ERB, attack +0.1 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 1.9 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.3 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 1.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.3 | 0.45 | 0.89 | 2.78 | 0.85 | 0.5 |  |
| attack time (-20 to -1 dB) (ms) | 184 | 422 | 58.0 | 99.0 | 688 | 0.1 |  |
| spectral shape of the attack, note for note | 19.5 (nearest 15.5) | fonts among themselves 9.51 | | | | 2.1 | yes |
| brightness over the keyboard (ERB/oct) | -0.04 | 3.96 | 3.83 | 4.3 | 3.22 | 7.3 | yes |
| key-to-key brightness scatter (ERB) | 0.493 | 1.41 | 0.484 | 1.48 | 1.69 | 1.6 |  |
| brightening from velocity 40 to 127 (ERB) | 0.11 | -0 | -0 | 0 | 0.01 | 0.4 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 23.3 | 11.2 | 9.95 | 13.2 | 15.3 | 4.1 | yes |
| decay rate of a held note (dB/s) | 0.1 | 0.7 | 0.2 | 0.8 | 0.1 | 1.3 |  |
| onset delay (ms) | 95.0 | 144 | 24.0 | 70.0 | 77.0 | 0.4 |  |
| Plomp-Levelt dissonance of a note (log2) | -3.73 | -3.13 | -4.51 | -1.78 | -2.45 | 0.7 |  |
| envelope curve, note for note (dB rms) | 16.8 (nearest 16.5) | fonts among themselves 4.5 | | | | 3.7 | yes |
| pitch re 12-TET A440 (median) (c) | -2.8 | 5.9 | 3.1 | 7.1 | 1.4 | 1.8 | yes |
| pitch scatter from key to key (c) | 1.9 | 5.8 | 0.9 | 5.6 | 1.3 | 0.5 |  |
| noisiness (spectral flatness) (dB) | -44.7 | -30.8 | -67.3 | -29.7 | -34.2 | 0.6 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -72.0 | -126 | -122 | -120 | -110 | 5.9 | yes |
| pitch at the start re settled (c) | 0.9 | -6.6 | 0 | 0 | 3.8 | 0.2 |  |
| stereo coherence | 0.431 | 0.758 | 1 | 1 | 1 | 4.7 | yes |
| partial-to-partial jaggedness (dB) | 5.55 | 3.2 | 3.1 | 2.9 | 3.6 | 2.4 | yes |
| dip between legato notes (dB) | 8.2 | 14.1 | 9.7 | 10.2 | 6.5 | 0.5 |  |
| level evenness of a legato scale (dB) | 2.4 | 0.7 | 0.7 | 1 | 2.2 | 1.5 |  |
| level over the keyboard (dB/oct) | 1.19 | 0.08 | 1.05 | 0.53 | 1.59 | 0.4 |  |
| level against the other instruments (velocity 96) (dB) | -1.7 | 2.8 | 2.1 | -1.2 | 0 | 1.4 |  |
| key-to-key level scatter (dB) | 1.88 | 0.91 | 0.46 | 0.86 | 1.19 | 2.0 | yes |
| odd / even partials (dB) | -9.1 | -7.7 | -2.2 | -4.6 | -2.5 | 2.0 | yes |
| onset jitter in a phrase (ms) | 52.1 | 33.2 | 29.4 | 17.7 | 22.9 | 3.3 | yes |
| onset in a phrase (re the note-on) (ms) | 41.0 | 6 | -1 | -3 | 18.0 | 3.7 | yes |
| onset sharpness (dB/ms) | 5 | 5.8 | 12.8 | 4.4 | 4.8 | 0.1 |  |
| overtone levels, note for note (dB rms) | 19.5 (nearest 17.1) | fonts among themselves 8.96 | | | | 2.2 | yes |
| panning across the keys (dB/oct) | -0.45 | 0.31 | 0.05 | 0 | -0.01 | 0.5 |  |
| partial envelopes alike | 0.009 | 0.115 | 1 | 0.385 | 0.447 | 0.9 | yes |
| release time (30 dB after note-off) (ms) | 595 | 114 | 97.0 | 125 | 101 | 9.9 | yes |
| spectrogram (1 - correlation), note for note | 0.498 (nearest 0.465) | fonts among themselves 0.0985 | | | | 5.1 | yes |
| spectral shape of the body, note for note | 15.2 (nearest 12.7) | fonts among themselves 7.9 | | | | 1.9 | yes |
| sounding length of a 100 ms note (ms) | 250 | 160 | 135 | 161 | 172 | 2.6 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.01 | 0.18 | 0 | -0.02 | -0.01 | 0.0 |  |
| level after 2 s held, re peak (dB) | -3.5 | -5 | -3.1 | -8.2 | -5 | 0.5 |  |
| high bands lag low bands by (ms) | -34.2 | 22.3 | -4 | -11.0 | -4.7 | 1.8 | yes |
| transient coherence: attack spread over bands (ms) | 15.0 | 16.8 | 9.9 | 21.8 | 2.9 | 0.2 |  |
| fundamental share (tristimulus 1) | 0 | 0.063 | 0.661 | 0.402 | 0.282 | 1.1 | yes |
| partials 2-4 share (tristimulus 2) | 0.059 | 0.889 | 0.207 | 0.491 | 0.494 | 1.3 | yes |
| partials 5+ share (tristimulus 3) | 0.941 | 0.043 | 0.134 | 0.16 | 0.188 | 11.0 | yes |
| loudness range, velocity 40 to 127 (dB) | 15.6 | 16.6 | 16.6 | 16.6 | 20.0 | 0.6 | yes |
| pitch wobble / vibrato (p-p) (c) | 2.1 | 28.5 | 0 | 31.7 | 20.7 | 1.4 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.024 (nearest 0.0218) | fonts among themselves 0.0139 | | | | 1.7 | yes |

</details>

### 112 Shanai: shehnai

Distance 4.86 (rank 5 of 129); the fonts disagree 1.0 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Slow to speak (attack 222 vs 15–40 ms; onset 104 vs 12–32 ms; phrase onset 64 ms vs 0; a 100 ms note sounds 400 vs 40–121 ms)**: FAULT. A shehnai is played with crisp tonguing; this one blurs fast passages.
- **Key-to-key level scatter 5.9 vs 0.3–1.8 dB, pitch scatter 14.2 vs 0–1.5 c (legato evenness 5.2 vs 0.5–2.3 dB)**: FAULT. The per-note design (kShehnaiNotes, C4–G5) has weak or out-of-tune fingerings. The folded probe keys reuse the same notes, so this is not a range artifact.
- **Attack 8.2 ERB brighter than the body (vs −0.6..+0.4)**: a squeal before the note settles. Likely the same reed set-up fault.
- Transpose is 0 (src/designs.cc). Release 686 ms: room ARTIFACT.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (6.48 dB | 0.51 | 0.38 | 0.27 | 1.78), outside the fonts, 8.0 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (704 ms | 67.0 | 58.0 | 58.0 | 58.0), outside the fonts, 14.4 x.
3. **onset delay**: speaks later (104 ms | 32.0 | 12.0 | 12.0 | 12.0), outside the fonts, 9.2 x.
4. **brightness (spectral centroid, velocity 96)**: brighter (21.7 ERB | 18.8 | 18.7 | 18.7 | 17.4), outside the fonts, 4.3 x.
5. **attack brighter than the body by**: a brighter attack (8.17 ERB | -0.64 | -0.11 | -0.14 | 0.44), outside the fonts, 15.4 x.
6. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 20.5, nearest 20.2; the fonts among themselves 2.54), outside.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -40 | -23 | -8 | -1 | +8 | +26 |
| re TimGM6mb | -43 | -19 | -1 | +5 | +7 | +15 |
| re Jnsgm2 | -43 | -19 | -1 | +6 | +7 | +15 |
| re SGM | -48 | -25 | -8 | -1 | +8 | +29 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -23 -11 +0 +6
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -1.0 ERB, attack +0.6 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 2.3 dB rms; TimGM6mb: centroid -0.4 ERB, attack +0.1 ERB, velocity brightening +0.1 ERB, loudness range +3 dB, envelope 2.1 dB rms; Jnsgm2: centroid -0.4 ERB, attack +0.1 ERB, velocity brightening +0.0 ERB, loudness range +4 dB, envelope 2.2 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 8.17 | -0.64 | -0.11 | -0.14 | 0.44 | 15.4 | yes |
| attack time (-20 to -1 dB) (ms) | 221 | 40.0 | 19.0 | 19.0 | 14.0 | 4.7 | yes |
| spectral shape of the attack, note for note | 18.6 (nearest 17.3) | fonts among themselves 8.55 | | | | 2.2 | yes |
| brightness over the keyboard (ERB/oct) | 0.41 | 3.1 | 4.1 | 4.16 | 2.73 | 4.5 | yes |
| key-to-key brightness scatter (ERB) | 1.88 | 0.744 | 0.593 | 0.606 | 1.11 | 4.7 | yes |
| brightening from velocity 40 to 127 (ERB) | 1.7 | 0 | -0 | 0 | 4.92 | 0.7 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 21.7 | 18.8 | 18.7 | 18.7 | 17.4 | 4.3 | yes |
| onset delay (ms) | 104 | 32.0 | 12.0 | 12.0 | 12.0 | 9.2 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.14 | -3.56 | -1.73 | -1.92 | -4.11 | 0.5 |  |
| envelope curve, note for note (dB rms) | 20.5 (nearest 20.2) | fonts among themselves 2.54 | | | | 8.1 | yes |
| pitch re 12-TET A440 (median) (c) | -1.6 | 1.6 | 2.9 | 3.1 | 4 | 1.1 | yes |
| pitch scatter from key to key (c) | 15.2 | 1.5 | 0.7 | 1 | 0 | 4.8 | yes |
| noisiness (spectral flatness) (dB) | -23.0 | -60.3 | -56.0 | -55.5 | -69.9 | 4.9 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -66.8 | -124 | -123 | -122 | -108 | 7.0 | yes |
| pitch at the start re settled (c) | -4 | -0.3 | 0 | 0 | -0.4 | 0.8 | yes |
| stereo coherence | 0.265 | 1 | 1 | 1 | 1 | 9.8 | yes |
| partial-to-partial jaggedness (dB) | 4.7 | 7.1 | 2.3 | 2.3 | 2.3 | 1.0 |  |
| dip between legato notes (dB) | 6.7 | 8.8 | 13.0 | 12.9 | 15.9 | 1.8 | yes |
| level evenness of a legato scale (dB) | 4 | 0.5 | 1.4 | 1.4 | 2.3 | 2.6 | yes |
| level over the keyboard (dB/oct) | -1.28 | 0.95 | 0.2 | -0.03 | -2.13 | 0.9 |  |
| level against the other instruments (velocity 96) (dB) | 0.9 | 2.2 | 2 | 1.9 | 2.7 | 1.2 | yes |
| key-to-key level scatter (dB) | 6.48 | 0.51 | 0.38 | 0.27 | 1.78 | 8.0 | yes |
| odd / even partials (dB) | -5.7 | 7.4 | -0.1 | 0 | 0.2 | 1.5 | yes |
| onset jitter in a phrase (ms) | 49.9 | 32.6 | 36.8 | 36.8 | 36.3 | 2.7 | yes |
| onset in a phrase (re the note-on) (ms) | 62.0 | -0 | -1 | -1 | 0 | 6.2 | yes |
| onset sharpness (dB/ms) | 4.5 | 8.9 | 11.8 | 11.8 | 10.5 | 3.3 | yes |
| overtone levels, note for note (dB rms) | 14.2 (nearest 12.3) | fonts among themselves 8.38 | | | | 1.7 | yes |
| panning across the keys (dB/oct) | 0.56 | 0 | -0.01 | -0.03 | -0.02 | 0.6 | yes |
| partial envelopes alike | -0.002 | 1 | 1 | 1 | 0.972 | 10.0 | yes |
| release time (30 dB after note-off) (ms) | 704 | 67.0 | 58.0 | 58.0 | 58.0 | 14.4 | yes |
| spectrogram (1 - correlation), note for note | 0.444 (nearest 0.389) | fonts among themselves 0.0674 | | | | 6.6 | yes |
| spectral shape of the body, note for note | 15 (nearest 13.1) | fonts among themselves 9.89 | | | | 1.5 | yes |
| sounding length of a 100 ms note (ms) | 397 | 121 | 50.0 | 50.0 | 39.0 | 3.7 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.01 | 0 | 0 | 0 | 0.01 | 0.0 |  |
| level after 2 s held, re peak (dB) | -4.2 | -6.8 | -9.9 | -9.9 | -10.2 | 1.9 | yes |
| high bands lag low bands by (ms) | -18.4 | 3.9 | 0.7 | 1 | 1.1 | 3.9 | yes |
| transient coherence: attack spread over bands (ms) | 22.1 | 4.5 | 1 | 1 | 0.9 | 4.2 | yes |
| fundamental share (tristimulus 1) | 0.021 | 0.005 | 0.04 | 0.04 | 0.079 | 0.3 |  |
| partials 2-4 share (tristimulus 2) | 0.313 | 0.222 | 0.139 | 0.139 | 0.289 | 1.8 |  |
| partials 5+ share (tristimulus 3) | 0.639 | 0.773 | 0.821 | 0.821 | 0.505 | 1.0 |  |
| loudness range, velocity 40 to 127 (dB) | 15.5 | 16.6 | 16.7 | 16.6 | 29.0 | 0.2 | yes |
| pitch wobble / vibrato (p-p) (c) | 8.5 | 0 | 0.1 | 0 | 0.3 | 1.7 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0285 (nearest 0.0268) | fonts among themselves 0.00494 | | | | 5.8 | yes |

</details>

### 113 Tinkle Bell: tinkle-bell

Distance 2.67 (rank 72 of 129); the fonts disagree 1.5 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Legato dip 17.9 vs 5–9 dB, phrase onset 22 ms, attack 24.5 vs 1–16 ms**: DELIBERATE, but debatable for GM. Below C5 the tinkle bell is a church bell struck by soft felt that "blooms" (README "Tinkle bell"), and the phrase (keys 52–61) lies entirely there. The GM Tinkle Bell is a small bell everywhere. Consider moving the split lower.
- **Level scatter 2.74 vs 0.2–0.57 dB, brightness scatter 1.13 vs 0.23–0.33 ERB, tristimulus 2 0.32 vs 0–0.03**: the same handbell/church-bell split at key 72.
- **Brightens with velocity (+1.1 vs ≈0 ERB)**: DELIBERATE: the Hertz contact.
- **Release 2000 ms**: the bell rings on (DELIBERATE), plus the room.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key level scatter**: less even from key to key (2.24 dB | 0.37 | 0.2 | 0.29 | 0.57), outside the fonts, 3.8 x.
2. **key-to-key brightness scatter**: less even in tone from key to key (1.08 ERB | 0.326 | 0.234 | 0.232 | 0.287), outside the fonts, 4.1 x.
3. **brightening from velocity 40 to 127**: brightens more with velocity (0.95 ERB | 0.03 | 0 | 0 | -0.13), outside the fonts, 3.2 x.
4. **dip between legato notes**: more separated legato (18.9 dB | 9.3 | 6.4 | 5.3 | 8.6), outside the fonts, 5.7 x.
5. **partials 2-4 share (tristimulus 2)**: stronger low partials (0.369 | 0.002 | 0 | 0 | 0.032), outside the fonts, 5.3 x.
6. **loudness range, velocity 40 to 127**: a narrower dynamic range (13.0 dB | 16.6 | 16.6 | 16.7 | 20.0), outside the fonts, 2.1 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -6 | +3 | -2 | -8 | -5 | +17 |
| re TimGM6mb | +15 | -1 | -4 | -13 | -4 | +17 |
| re Jnsgm2 | +12 | -5 | -7 | -12 | -4 | +22 |
| re SGM | +3 | +1 | -0 | -9 | +1 | +7 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +1; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.6 ERB, attack -0.2 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 2.9 dB rms; TimGM6mb: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 1.8 dB rms; Jnsgm2: centroid -0.1 ERB, attack -0.1 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 1.7 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 1.29 | 1.09 | 0.28 | 0.44 | -0.01 | 1.7 |  |
| attack time (-20 to -1 dB) (ms) | 25.0 | 4 | 2 | 1 | 16.0 | 1.6 | yes |
| spectral shape of the attack, note for note | 15.5 (nearest 14.1) | fonts among themselves 13.2 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 2.72 | 3.68 | 4.73 | 4.7 | 4.74 | 3.8 | yes |
| key-to-key brightness scatter (ERB) | 1.08 | 0.326 | 0.234 | 0.232 | 0.287 | 4.1 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.95 | 0.03 | 0 | 0 | -0.13 | 3.2 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 8.89 | 9.12 | 13.3 | 13.4 | 12.7 | 1.9 | yes |
| decay rate of a held note (dB/s) | 9.2 | 58.2 | 20.9 | 21.2 | 9.9 | 0.9 |  |
| onset delay (ms) | 10.0 | 5 | 1 | 1 | 3 | 0.8 |  |
| Plomp-Levelt dissonance of a note (log2) | -2.51 | -4.57 | -7.38 | -8.38 | -3.47 | 1.4 | yes |
| envelope curve, note for note (dB rms) | 21.8 (nearest 13.4) | fonts among themselves 17.6 | | | | 1.2 |  |
| pitch re 12-TET A440 (median) (c) | -0.2 | -4.5 | 28.5 | 28.5 | 0.1 | 0.9 |  |
| pitch scatter from key to key (c) | 0.3 | 2.1 | 1.6 | 2.5 | 0 | 0.5 |  |
| noisiness (spectral flatness) (dB) | -80.2 | -62.4 | -68.2 | -67.7 | -75.0 | 1.9 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -33.3 | -123 | -122 | -125 | -83.4 | 4.3 | yes |
| pitch at the start re settled (c) | -0.4 | 7 | 0.4 | 0.1 | 0 | 0.1 |  |
| stereo coherence | 0.654 | 1 | 1 | 1 | 0.972 | 4.6 | yes |
| partial-to-partial jaggedness (dB) | 6.15 | 4.9 | 6.65 | 8.9 | 14.5 | 0.3 |  |
| dip between legato notes (dB) | 18.9 | 9.3 | 6.4 | 5.3 | 8.6 | 5.7 | yes |
| level evenness of a legato scale (dB) | 1.6 | 0.9 | 0.4 | 0.6 | 1.3 | 0.9 |  |
| level over the keyboard (dB/oct) | 0.18 | -1.52 | -1.23 | -1.04 | -0.62 | 1.3 | yes |
| level against the other instruments (velocity 96) (dB) | -1.1 | -2.6 | -1.9 | -2.1 | -0.7 | 0.9 |  |
| key-to-key level scatter (dB) | 2.24 | 0.37 | 0.2 | 0.29 | 0.57 | 3.8 | yes |
| odd / even partials (dB) | -25.4 | -14.2 | 33.6 | 33.9 | 1 | 1.8 | yes |
| onset jitter in a phrase (ms) | 42.7 | 1 | 6.8 | 1.1 | 42.6 | 1.9 |  |
| onset in a phrase (re the note-on) (ms) | 16.0 | 3 | -2 | -2 | 2 | 1.6 | yes |
| onset sharpness (dB/ms) | 17.3 | 27.4 | 7.5 | 2.6 | 17.1 | 0.4 |  |
| overtone levels, note for note (dB rms) | 21.6 (nearest 16.4) | fonts among themselves 19.6 | | | | 1.1 |  |
| panning across the keys (dB/oct) | 0.3 | 0 | 0 | 0 | -0.06 | 0.3 |  |
| partial envelopes alike | 0.948 | 0.989 | 0.95 | 0.942 | 0.863 | 0.0 |  |
| release time (30 dB after note-off) (ms) | 2000 | 409 | 833 | 882 | 863 | 2.2 | yes |
| spectrogram (1 - correlation), note for note | 0.562 (nearest 0.524) | fonts among themselves 0.377 | | | | 1.5 | yes |
| spectral shape of the body, note for note | 16.7 (nearest 15) | fonts among themselves 14.4 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 330 | 158 | 400 | 314 | 333 | 0.0 |  |
| partial stretch (inharmonicity) (c/partial) | -0.02 | -5.35 | -0.77 | 2.84 | 0.15 | 0.1 |  |
| level after 2 s held, re peak (dB) | -24.4 | -112 | -46.5 | -50.8 | -24.4 | 0.6 |  |
| high bands lag low bands by (ms) | -2 | 0 | 0 | 0 | 0 | 0.4 |  |
| transient coherence: attack spread over bands (ms) | 1.5 | 0.9 | 0 | 0 | 0.3 | 0.3 |  |
| fundamental share (tristimulus 1) | 0.631 | 0.997 | 0.063 | 0.037 | 0.958 | 0.3 |  |
| partials 2-4 share (tristimulus 2) | 0.369 | 0.002 | 0 | 0 | 0.032 | 5.3 | yes |
| partials 5+ share (tristimulus 3) | 0 | 0 | 0.937 | 0.963 | 0.011 | 1.0 |  |
| loudness range, velocity 40 to 127 (dB) | 13.0 | 16.6 | 16.6 | 16.7 | 20.0 | 2.1 | yes |
| pitch wobble / vibrato (p-p) (c) | 3.3 | 16.3 | 1.2 | 1.2 | 0.7 | 0.3 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0154 (nearest 0.0128) | fonts among themselves 0.0106 | | | | 1.4 |  |

</details>

### 114 Agogo: agogo

Distance 3.44 (rank 36 of 129); the fonts disagree 1.6 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Click dominates the ring (a 100 ms note sounds 11 ms vs 66–150; flatness −11.8 vs −38..−48 dB)**: probable FAULT. The ring decays more slowly than the fonts' (34 vs 49–79 dB/s), so the strike transient must sit ~20 dB over it. The stick tip (`PJ_AGOGO_TIP`, K 3e9, ~0.05 ms contact) is too hard against the 0.08 kg bell mass. The agogo should clank and ring, not tick.
- **Louder toward the top (+3.3 vs −1.5..−3.3 dB/oct), brightness scatter 1.97 vs 0.26–0.49 ERB**: worth a check, but secondary.
- **Tristimulus 1/2 (0.47 vs 0; 0.02 vs 0.7–0.99)**: MEASUREMENT ARTIFACT: an inharmonic bell has no meaningful partial series.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **key-to-key brightness scatter**: less even in tone from key to key (1.96 ERB | 0.494 | 0.274 | 0.258 | 0.369), outside the fonts, 8.2 x.
2. **fundamental share (tristimulus 1)**: more fundamental (0.474 | 0 | 0 | 0 | 0), outside the fonts, 6.8 x.
3. **noisiness (spectral flatness)**: noisier (-11.6 dB | -47.8 | -44.9 | -48.3 | -38.3), outside the fonts, 6.9 x.
4. **sounding length of a 100 ms note [room]**: shorter staccato (11.0 ms | 117 | 104 | 66.0 | 150), outside the fonts, 5.6 x.
5. **partials 2-4 share (tristimulus 2)**: weaker low partials (0.02 | 0.99 | 0.705 | 0.938 | 0.944), outside the fonts, 6.5 x.
6. **decay rate of a held note**: decays slower (34.0 dB/s | 78.1 | 75.9 | 78.7 | 48.9), outside the fonts, 3.4 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -10 | -10 | -11 | -10 | +7 | +20 |
| re TimGM6mb | -1 | +0 | -4 | -5 | -0 | +8 |
| re Jnsgm2 | -5 | -6 | -6 | -8 | +4 | +13 |
| re SGM | -0 | -0 | -0 | -3 | +3 | +1 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack +0.3 ERB, velocity brightening +0.3 ERB, loudness range +3 dB, envelope 3.5 dB rms; TimGM6mb: centroid -0.2 ERB, attack -0.3 ERB, velocity brightening +0.1 ERB, loudness range +3 dB, envelope 3.4 dB rms; Jnsgm2: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening -0.1 ERB, loudness range +4 dB, envelope 4.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 4.42 | 8.33 | 2.86 | 10.7 | 3.73 | 0.4 |  |
| attack time (-20 to -1 dB) (ms) | 0.5 | 6 | 7 | 2 | 2 | 3.1 | yes |
| spectral shape of the attack, note for note | 17.9 (nearest 16.3) | fonts among themselves 12.8 | | | | 1.4 |  |
| brightness over the keyboard (ERB/oct) | 1.23 | 3.27 | 4.21 | 3.93 | 4.03 | 5.9 | yes |
| key-to-key brightness scatter (ERB) | 1.96 | 0.494 | 0.274 | 0.258 | 0.369 | 8.2 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.44 | -0.01 | -0 | 0.04 | 0.06 | 1.4 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 14.0 | 11.5 | 15.7 | 12.8 | 12.7 | 0.6 |  |
| decay rate of a held note (dB/s) | 34.0 | 78.1 | 75.9 | 78.7 | 48.9 | 3.4 | yes |
| onset delay (ms) | 9 | 8 | 1 | 6 | 3 | 0.5 |  |
| Plomp-Levelt dissonance of a note (log2) | -1.18 | -4.88 | -2.08 | -3.99 | -3.07 | 1.7 | yes |
| envelope curve, note for note (dB rms) | 14.9 (nearest 10.8) | fonts among themselves 10.2 | | | | 1.5 |  |
| pitch re 12-TET A440 (median) (c) | -1.5 | 22.5 | 11.6 | -9.8 | -0.2 | 0.4 |  |
| pitch scatter from key to key (c) | 7.9 | 0.6 | 1.9 | 0.4 | 0.1 | 2.5 | yes |
| noisiness (spectral flatness) (dB) | -11.6 | -47.8 | -44.9 | -48.3 | -38.3 | 6.9 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -128 | -120 | -121 | -122 | -100 | 0.6 | yes |
| pitch at the start re settled (c) | 3.4 | -0.9 | -33.5 | -0.2 | -1.6 | 0.3 | yes |
| stereo coherence | 0.415 | 0.953 | 1 | 1 | 0.045 | 1.2 |  |
| partial-to-partial jaggedness (dB) | 4.4 | 9.9 | 9 | 14.7 | 14.0 | 2.6 | yes |
| dip between legato notes (dB) | 7.3 | 6.6 | 10.6 | 10.4 | 7.8 | 0.9 |  |
| level evenness of a legato scale (dB) | 1 | 0.6 | 0.3 | 0.3 | 1.9 | 0.6 |  |
| level over the keyboard (dB/oct) | -0.52 | -1.49 | -2.12 | -2.54 | -3.31 | 1.8 | yes |
| level against the other instruments (velocity 96) (dB) | -6.7 | -9.1 | -6.7 | -8.4 | -14.5 | 0.5 |  |
| key-to-key level scatter (dB) | 1.6 | 0.46 | 0.08 | 0.09 | 1.01 | 2.7 | yes |
| odd / even partials (dB) | 2.2 | -28.7 | -15.0 | -11.9 | -32.5 | 2.3 | yes |
| onset jitter in a phrase (ms) | 0 | 2.7 | 1.4 | 1.5 | 0.7 | 0.3 |  |
| onset in a phrase (re the note-on) (ms) | 4 | 8 | -4 | 6 | -0 | 0.1 |  |
| onset sharpness (dB/ms) | 63.1 | 24.2 | 3.1 | 30.2 | 45.4 | 1.7 | yes |
| overtone levels, note for note (dB rms) | 30.4 (nearest 21.3) | fonts among themselves 17.7 | | | | 1.7 |  |
| panning across the keys (dB/oct) | -0.49 | 0.02 | -0 | -0 | -0.06 | 0.5 |  |
| partial envelopes alike | 0.944 | 0.995 | 0.989 | 0.996 | 0.992 | 0.5 |  |
| spectrogram (1 - correlation), note for note | 0.407 (nearest 0.389) | fonts among themselves 0.22 | | | | 1.9 | yes |
| spectral shape of the body, note for note | 16.1 (nearest 14.9) | fonts among themselves 13 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 11.0 | 117 | 104 | 66.0 | 150 | 5.6 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.27 | -1.57 | 0.58 | 3.63 | 3.44 | 0.7 |  |
| level after 2 s held, re peak (dB) | -84.9 | -120 | -121 | -122 | -97.3 | 3.0 | yes |
| high bands lag low bands by (ms) | -1.3 | 0 | 0 | 0 | 0 | 0.3 |  |
| transient coherence: attack spread over bands (ms) | 1.6 | 0.5 | 0 | 0.5 | 0 | 0.3 |  |
| fundamental share (tristimulus 1) | 0.474 | 0 | 0 | 0 | 0 | 6.8 | yes |
| partials 2-4 share (tristimulus 2) | 0.02 | 0.99 | 0.705 | 0.938 | 0.944 | 6.5 | yes |
| partials 5+ share (tristimulus 3) | 0.251 | 0.01 | 0.295 | 0.062 | 0.056 | 1.3 |  |
| loudness range, velocity 40 to 127 (dB) | 13.7 | 16.7 | 16.7 | 16.6 | 20.1 | 1.7 | yes |
| pitch wobble / vibrato (p-p) (c) | 81.8 | 73.8 | 67.4 | 66.1 | 61.4 | 2.4 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0131 (nearest 0.0126) | fonts among themselves 0.00554 | | | | 2.4 | yes |

</details>

### 115 Steel Drums: steel-drums

Distance 2.86 (rank 59 of 129); the fonts disagree 1.9 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **A scale that drops out (legato evenness 48.8 vs 0.2–0.8 dB; 16 silent probe notes)**: FAULT. `steel-drums` declares 52–90 (src/modalinst.cc), but note areas exist only for keys 60–90 (`vib = midi - 60; if (vib < 0) return`). Keys 52–59 are silent: probe keys 40, 44, 52 and 56 (folded) and five of the six phrase notes. Fix: lo = 60 (folding then works) or add note areas below C4.
- **Brightens with velocity (+1.62 vs 0–0.03 ERB)**: DELIBERATE. The stick contact, and the octave mode taking energy (Monteil; README "Steel drums").
- **Brightens slowly up the keys (1.36 vs 3.7–4.0 ERB/oct)**: partly the duplicated folded keys.
- **Stereo coherence 0.29**: room ARTIFACT.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens more with velocity (1.59 ERB | 0.03 | 0 | 0 | 0.03), outside the fonts, 5.3 x.
2. **brightness over the keyboard**: brightens slower up the keyboard (1.36 ERB/oct | 3.66 | 3.96 | 3.7 | 3.96), outside the fonts, 8.2 x.
3. **level evenness of a legato scale**: a less even scale (47.9 dB | 0.2 | 0.6 | 0.8 | 0.2), outside the fonts, 47.5 x.
4. **attack time (-20 to -1 dB)**: a faster attack (1 ms | 5 | 7 | 18.0 | 27.0), outside the fonts, 2.9 x.
5. **key-to-key level scatter**: less even from key to key (1.54 dB | 0.17 | 0.06 | 0.13 | 0.56), outside the fonts, 2.8 x.
6. **stereo coherence [room]**: wider (0.272 | 0.999 | 1 | 1 | 1), outside the fonts, 9.7 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -12 | +2 | +15 | +5 | -10 | -17 |
| re TimGM6mb | -21 | +0 | +14 | +9 | -10 | -12 |
| re Jnsgm2 | -12 | +4 | +15 | +7 | -11 | -18 |
| re SGM | -5 | +10 | +20 | +10 | -17 | -23 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -5 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.4 ERB, velocity brightening +0.0 ERB, loudness range +3 dB, envelope 3.8 dB rms; TimGM6mb: centroid -0.0 ERB, attack -0.3 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 2.3 dB rms; Jnsgm2: centroid -0.1 ERB, attack -0.1 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 1.4 dB rms
- silent probe notes: pajupilli 16

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -1.84 | -1.8 | 0.75 | -0.8 | -3.05 | 0.3 |  |
| attack time (-20 to -1 dB) (ms) | 1 | 5 | 7 | 18.0 | 27.0 | 2.9 | yes |
| spectral shape of the attack, note for note | 20.2 (nearest 19.5) | fonts among themselves 8.33 | | | | 2.4 | yes |
| brightness over the keyboard (ERB/oct) | 1.36 | 3.66 | 3.96 | 3.7 | 3.96 | 8.2 | yes |
| key-to-key brightness scatter (ERB) | 0.849 | 0.492 | 0.304 | 0.581 | 0.375 | 2.1 | yes |
| brightening from velocity 40 to 127 (ERB) | 1.59 | 0.03 | 0 | 0 | 0.03 | 5.3 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 12.4 | 12.5 | 8.7 | 11.7 | 15.6 | 0.1 |  |
| decay rate of a held note (dB/s) | 33.2 | 58.7 | 31.1 | 7 | 28.0 | 0.1 |  |
| onset delay (ms) | 9 | 4 | 0 | 2 | 4 | 0.6 |  |
| Plomp-Levelt dissonance of a note (log2) | -0.84 | -2.91 | -3.27 | -3.18 | -1.29 | 2.2 | yes |
| envelope curve, note for note (dB rms) | 15.4 (nearest 10.5) | fonts among themselves 18.2 | | | | 0.8 |  |
| pitch re 12-TET A440 (median) (c) | -2.2 | -2.5 | 16.4 | 0 | 5.8 | 0.5 |  |
| pitch scatter from key to key (c) | 3.2 | 2.7 | 2.7 | 1.2 | 1.6 | 0.3 |  |
| noisiness (spectral flatness) (dB) | -46.6 | -60.8 | -56.0 | -65.7 | -47.0 | 1.3 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -128 | -129 | -125 | -125 | -107 | 0.2 |  |
| pitch at the start re settled (c) | -0.9 | 1 | 2.1 | 4.6 | 0.8 | 0.5 |  |
| stereo coherence | 0.272 | 0.999 | 1 | 1 | 1 | 9.7 | yes |
| partial-to-partial jaggedness (dB) | 5.55 | 10.5 | 9.4 | 7.2 | 6.5 | 1.4 | yes |
| dip between legato notes (dB) | 0 | 7.9 | 10.8 | 7.4 | 10.3 | 4.6 | yes |
| level evenness of a legato scale (dB) | 47.9 | 0.2 | 0.6 | 0.8 | 0.2 | 47.5 | yes |
| level over the keyboard (dB/oct) | -1.01 | -2.74 | -2.02 | -1.88 | -2.55 | 1.3 | yes |
| level against the other instruments (velocity 96) (dB) | -0.6 | 3.8 | -2.5 | 1.4 | -4.5 | 0.0 |  |
| key-to-key level scatter (dB) | 1.54 | 0.17 | 0.06 | 0.13 | 0.56 | 2.8 | yes |
| odd / even partials (dB) | -31.2 | -22.3 | -29.7 | -22.9 | -12.7 | 1.0 | yes |
| onset jitter in a phrase (ms) | 50.2 | 40.1 | 13.3 | 40.9 | 1.5 | 1.2 | yes |
| onset in a phrase (re the note-on) (ms) | 2 | 24.0 | -5 | 7 | 0 | 0.1 |  |
| onset sharpness (dB/ms) | 63.1 | 23.6 | 2.4 | 5.4 | 44.3 | 2.3 | yes |
| overtone levels, note for note (dB rms) | 17.6 (nearest 15.7) | fonts among themselves 10.4 | | | | 1.7 | yes |
| panning across the keys (dB/oct) | -0.71 | -0 | 0 | 0 | -0.01 | 0.7 | yes |
| partial envelopes alike | 0.955 | 0.995 | 0.963 | 0.875 | 0.994 | 0.2 |  |
| spectrogram (1 - correlation), note for note | 0.48 (nearest 0.36) | fonts among themselves 0.293 | | | | 1.6 |  |
| spectral shape of the body, note for note | 18.2 (nearest 16.4) | fonts among themselves 9.37 | | | | 1.9 | yes |
| sounding length of a 100 ms note (ms) | 160 | 81.0 | 197 | 155 | 138 | 0.2 |  |
| partial stretch (inharmonicity) (c/partial) | -0.68 | 0.67 | -3.2 | -0.05 | -0.56 | 0.2 |  |
| level after 2 s held, re peak (dB) | -76.2 | -126 | -72.6 | -26.6 | -106 | 0.3 |  |
| high bands lag low bands by (ms) | 0.3 | 24.0 | 16.7 | 30.7 | 30.0 | 3.8 | yes |
| transient coherence: attack spread over bands (ms) | 0.8 | 9.8 | 10.3 | 22.9 | 15.7 | 1.9 | yes |
| fundamental share (tristimulus 1) | 0.201 | 0.1 | 0.505 | 0.227 | 0.064 | 0.2 |  |
| partials 2-4 share (tristimulus 2) | 0.567 | 0.86 | 0.471 | 0.77 | 0.673 | 0.8 |  |
| partials 5+ share (tristimulus 3) | 0 | 0.005 | 0.003 | 0.004 | 0.227 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 15.0 | 16.7 | 16.7 | 16.7 | 20.1 | 1.0 | yes |
| pitch wobble / vibrato (p-p) (c) | 24.4 | 60.2 | 6.3 | 5.3 | 72.5 | 0.3 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0121 (nearest 0.00971) | fonts among themselves 0.00729 | | | | 1.7 | yes |

</details>

### 116 Woodblock: woodblock

Distance 1.56 (rank 121 of 129); the fonts disagree 1.8 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Louder (+1.6 vs −11.8..−7.1 dB against the other instruments)**: FAULT for GM balance. The common-loudness rule (README, Levels) puts the woodblock 9–13 dB above where all four fonts agree it belongs.
- **Shorter (staccato 16 vs 27–43 ms)**: DELIBERATE: the block decays in tens of ms (README "Woodblock"; bells_blocks.md 3.2).
- **Sustains more after 2 s (−83 vs −111..−125 dB), stereo coherence 0.40**: room ARTIFACT.
- **Brightens with velocity (+0.68 ERB)**: DELIBERATE: the Hertz contact.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **level after 2 s held, re peak**: sustains more (-83.1 dB | -121 | -125 | -111 | -112), outside the fonts, 4.8 x.
2. **stereo coherence [room]**: wider (0.396 | 0.998 | 1 | 1 | 1), outside the fonts, 8.1 x.
3. **sounding length of a 100 ms note [room]**: shorter staccato (16.0 ms | 27.0 | 40.0 | 34.0 | 43.0), outside the fonts, 3.6 x.
4. **brightening from velocity 40 to 127**: brightens more with velocity (0.64 ERB | 0.01 | -0 | -0.04 | 0.12), outside the fonts, 2.1 x.
5. **attack time (-20 to -1 dB)**: a faster attack (0.5 ms | 1 | 4 | 1 | 3), outside the fonts, 1.8 x.
6. **key-to-key level scatter**: less even from key to key (0.87 dB | 0.02 | 0.03 | 0.03 | 0.41), outside the fonts, 1.7 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +6 | +28 | +2 | +7 | -7 | -19 |
| re TimGM6mb | +17 | +13 | -3 | +6 | -9 | -19 |
| re Jnsgm2 | -16 | -0 | +0 | +4 | +2 | +2 |
| re SGM | +1 | +8 | +4 | +9 | -4 | -12 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.2 ERB, velocity brightening +0.6 ERB, loudness range +3 dB, envelope 5.4 dB rms; TimGM6mb: centroid -0.1 ERB, attack +0.2 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 7.0 dB rms; Jnsgm2: centroid -0.1 ERB, attack +0.1 ERB, velocity brightening -0.4 ERB, loudness range +4 dB, envelope 2.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.77 | 1.23 | -0.7 | -0.33 | 0.88 | 0.5 |  |
| attack time (-20 to -1 dB) (ms) | 0.5 | 1 | 4 | 1 | 3 | 1.8 | yes |
| spectral shape of the attack, note for note | 13.7 (nearest 13) | fonts among themselves 18.6 | | | | 0.7 |  |
| brightness over the keyboard (ERB/oct) | 1.7 | 0.96 | 3.21 | 3.65 | 2.38 | 0.8 |  |
| key-to-key brightness scatter (ERB) | 0.626 | 0.191 | 0.17 | 0.428 | 0.294 | 1.9 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.64 | 0.01 | -0 | -0.04 | 0.12 | 2.1 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 10.7 | 14.5 | 12.0 | 8.86 | 11.9 | 0.4 |  |
| decay rate of a held note (dB/s) | 33.0 | 169 | 181 | 13.6 | 157 | 1.2 |  |
| onset delay (ms) | 9 | 2 | 1 | 0 | 2 | 0.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | -0.21 | -0.1 | -0.91 | -0.09 | -0.65 | 0.4 |  |
| envelope curve, note for note (dB rms) | 22.1 (nearest 15.6) | fonts among themselves 20.1 | | | | 1.1 |  |
| pitch re 12-TET A440 (median) (c) | -0.6 | 2.7 | -0.1 | -15.9 | -55.7 | 0.3 |  |
| pitch scatter from key to key (c) | 21.6 | 42.1 | 19.6 | 20.6 | 47.0 | 0.7 |  |
| noisiness (spectral flatness) (dB) | -27.1 | -17.1 | -25.9 | -53.6 | -25.7 | 0.1 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -128 | -121 | -125 | -119 | -112 | 1.2 |  |
| pitch at the start re settled (c) | 3 | -5.2 | 12.5 | 8.8 | -1.6 | 0.1 |  |
| stereo coherence | 0.396 | 0.998 | 1 | 1 | 1 | 8.1 | yes |
| partial-to-partial jaggedness (dB) | 2.9 | 3.7 | 4.2 | 1.9 | 5.4 | 0.6 |  |
| dip between legato notes (dB) | 8.6 | 21.2 | 0 | 10.0 | 9.8 | 0.1 |  |
| level evenness of a legato scale (dB) | 0.6 | 0.3 | 0.2 | 0.4 | 2 | 0.2 |  |
| level over the keyboard (dB/oct) | -0.79 | -0.6 | -1.44 | -3.04 | -1.69 | 0.6 |  |
| level against the other instruments (velocity 96) (dB) | -8.2 | -11.8 | -7.5 | -7.4 | -7.1 | 0.3 |  |
| key-to-key level scatter (dB) | 0.87 | 0.02 | 0.03 | 0.03 | 0.41 | 1.7 | yes |
| odd / even partials (dB) | -5.1 | -0.4 | 4.7 | -5.3 | -1.4 | 0.8 |  |
| onset jitter in a phrase (ms) | 1.5 | 0.6 | 0.6 | 17.4 | 0.7 | 0.1 |  |
| onset in a phrase (re the note-on) (ms) | 4 | -2 | -1 | -4 | 0 | 0.6 |  |
| onset sharpness (dB/ms) | 63.3 | 36.1 | 3.9 | 1 | 54.5 | 1.6 | yes |
| overtone levels, note for note (dB rms) | 16.7 (nearest 15.6) | fonts among themselves 21.7 | | | | 0.8 |  |
| panning across the keys (dB/oct) | -0.34 | -0 | 0 | -0 | -0.01 | 0.3 |  |
| partial envelopes alike | 0.956 | 0.999 | 0.998 | 0.74 | 0.999 | 0.3 |  |
| spectrogram (1 - correlation), note for note | 0.496 (nearest 0.477) | fonts among themselves 0.567 | | | | 0.9 |  |
| spectral shape of the body, note for note | 14.3 (nearest 12.3) | fonts among themselves 17.4 | | | | 0.8 |  |
| sounding length of a 100 ms note (ms) | 16.0 | 27.0 | 40.0 | 34.0 | 43.0 | 3.6 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.95 | 0.32 | -0.78 | -2.52 | 0.29 | 0.5 |  |
| level after 2 s held, re peak (dB) | -83.1 | -121 | -125 | -111 | -112 | 4.8 | yes |
| high bands lag low bands by (ms) | -0.7 | 0 | 0 | 0 | 0 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 0.9 | 0 | 0 | 0 | 0 | 0.2 |  |
| fundamental share (tristimulus 1) | 0.874 | 0.001 | 0 | 0.985 | 0 | 1.8 |  |
| partials 2-4 share (tristimulus 2) | 0.056 | 0.409 | 0.912 | 0.012 | 0.8 | 1.2 |  |
| partials 5+ share (tristimulus 3) | 0.07 | 0.562 | 0.088 | 0.002 | 0.2 | 0.3 |  |
| loudness range, velocity 40 to 127 (dB) | 15.2 | 16.7 | 16.6 | 16.6 | 20.0 | 0.9 | yes |
| pitch wobble / vibrato (p-p) (c) | 74.7 | 66.8 | 56.6 | 92.3 | 63.4 | 0.5 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0113 (nearest 0.0102) | fonts among themselves 0.00698 | | | | 1.6 | yes |

</details>

### 117 Taiko Drum: taiko

Distance 4.09 (rank 19 of 129); the fonts disagree 1.1 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Brighter, without the boom (7.3 vs 2.8–3.1 ERB; −19..−28 dB below 150 Hz)**: probable FAULT for GM use. The drum's f0 follows the key (`hz(midi)`, range 24–72), so the probe's 52–72 is a 165–520 Hz drum. The fonts play one big o-daiko whatever the key (their pitch scatter is 41–64 c). Consider limited key tracking around the 68–135 Hz sizes in steelpan_taiko.md 2.6.
- **Key-to-key level scatter 4.7 vs 0.03–0.74 dB**: the same key mapping, and probe keys 76–84 fold down.
- **Evenly tuned (3 c), less wobble (56 vs 87–102 c)**: DELIBERATE. The glide is ≤ a semitone (README "Taiko"). The fonts' pitch numbers are unreliable on a drum.
- **Stereo coherence 0.19**: room ARTIFACT.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (7.25 ERB | 3.08 | 3.01 | 3.01 | 2.8), outside the fonts, 14.1 x.
2. **key-to-key level scatter**: less even from key to key (3.6 dB | 0.03 | 0.28 | 0.28 | 0.74), outside the fonts, 6.6 x.
3. **partial stretch (inharmonicity)**: less stretched partials (-5.94 c/partial | 0.37 | -0.26 | -0.53 | -0.4), outside the fonts, 11.2 x.
4. **pitch scatter from key to key**: more evenly tuned (2.9 c | 42.7 | 41.0 | 63.8 | 42.8), outside the fonts, 3.5 x.
5. **level against the other instruments (velocity 96)**: quieter (-1.8 dB | 0.7 | 0.4 | 0.9 | 3.2), outside the fonts, 1.9 x.
6. **pitch wobble / vibrato (p-p)**: wobbles less (55.1 c | 87.7 | 86.6 | 89.2 | 102), outside the fonts, 4.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -28 | -12 | +4 | +1 | +13 | +14 |
| re TimGM6mb | -21 | -3 | +0 | -8 | +3 | +21 |
| re Jnsgm2 | -21 | -3 | +0 | -8 | +3 | +21 |
| re SGM | -18 | +2 | +5 | -11 | +2 | +17 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.1 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 3.1 dB rms; TimGM6mb: centroid +0.0 ERB, attack +0.1 ERB, velocity brightening -0.5 ERB, loudness range +3 dB, envelope 7.7 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.1 ERB, velocity brightening -0.5 ERB, loudness range +3 dB, envelope 7.7 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.03 | 0.75 | 1.77 | 1.77 | 0.38 | 1.8 | yes |
| attack time (-20 to -1 dB) (ms) | 2 | 3 | 11.0 | 11.0 | 38.0 | 1.3 | yes |
| spectral shape of the attack, note for note | 11.9 (nearest 11.6) | fonts among themselves 7.78 | | | | 1.5 | yes |
| brightness over the keyboard (ERB/oct) | 2.18 | 0.84 | 0.76 | 0.76 | 0.27 | 4.7 | yes |
| key-to-key brightness scatter (ERB) | 0.947 | 0.205 | 0.216 | 0.216 | 0.523 | 3.7 | yes |
| brightening from velocity 40 to 127 (ERB) | 0.21 | 0.02 | 0 | 0 | -0.29 | 0.7 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 7.25 | 3.08 | 3.01 | 3.01 | 2.8 | 14.1 | yes |
| decay rate of a held note (dB/s) | 30.4 | 60.5 | 26.9 | 26.9 | 33.3 | 0.0 |  |
| onset delay (ms) | 10.0 | 5 | 6 | 6 | 8 | 0.4 |  |
| Plomp-Levelt dissonance of a note (log2) | -1.85 | -3.17 | -3.76 | -3.76 | -2.24 | 2.1 | yes |
| envelope curve, note for note (dB rms) | 11.2 (nearest 11.1) | fonts among themselves 12.7 | | | | 0.9 |  |
| pitch re 12-TET A440 (median) (c) | -43.5 | -13.7 | 17.7 | 4.3 | -16.8 | 2.2 | yes |
| pitch scatter from key to key (c) | 2.9 | 42.7 | 41.0 | 63.8 | 42.8 | 3.5 | yes |
| noisiness (spectral flatness) (dB) | -58.1 | -57.2 | -45.6 | -45.2 | -34.3 | 1.1 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -108 | -127 | -123 | -123 | -114 | 2.4 | yes |
| pitch at the start re settled (c) | -0.2 | 0 | 0 | 0 | 0 | 0.0 |  |
| stereo coherence | 0.216 | 0.969 | 1 | 1 | 0.94 | 10.2 | yes |
| partial-to-partial jaggedness (dB) | 2.45 | 1.4 | 1.65 | 1.7 | 1.5 | 0.9 | yes |
| dip between legato notes (dB) | 16.3 | 15.5 | 12.6 | 12.6 | 14.0 | 1.5 |  |
| level evenness of a legato scale (dB) | 1.8 | 0.7 | 0.8 | 0.8 | 3.4 | 0.7 |  |
| level over the keyboard (dB/oct) | -1.84 | -1.31 | -0.62 | -0.62 | -1.05 | 1.0 | yes |
| level against the other instruments (velocity 96) (dB) | -1.8 | 0.7 | 0.4 | 0.9 | 3.2 | 1.9 | yes |
| key-to-key level scatter (dB) | 3.6 | 0.03 | 0.28 | 0.28 | 0.74 | 6.6 | yes |
| odd / even partials (dB) | -3.2 | -3.5 | -5 | -5.5 | -5.4 | 1.0 |  |
| onset jitter in a phrase (ms) | 17.5 | 1.2 | 53.2 | 53.2 | 51.8 | 1.3 |  |
| onset in a phrase (re the note-on) (ms) | 16.0 | 1 | 138 | 138 | 88.0 | 1.4 |  |
| onset sharpness (dB/ms) | 55.6 | 20.2 | 6.5 | 6.5 | 22.6 | 5.2 | yes |
| overtone levels, note for note (dB rms) | 25.8 (nearest 16.7) | fonts among themselves 6.23 | | | | 4.1 | yes |
| panning across the keys (dB/oct) | 0.25 | 0 | 0 | 0 | -0 | 0.2 |  |
| partial envelopes alike | 0.956 | 0.995 | 0.976 | 0.976 | 0.973 | 0.2 |  |
| release time (30 dB after note-off) (ms) | 1294 | - | 973 | 973 | 220 | 0.4 | yes |
| spectrogram (1 - correlation), note for note | 0.479 (nearest 0.461) | fonts among themselves 0.101 | | | | 4.7 | yes |
| spectral shape of the body, note for note | 13.8 (nearest 13.1) | fonts among themselves 7.72 | | | | 1.8 | yes |
| sounding length of a 100 ms note (ms) | 35.0 | 19.0 | 48.0 | 48.0 | 70.0 | 0.5 |  |
| partial stretch (inharmonicity) (c/partial) | -5.94 | 0.37 | -0.26 | -0.53 | -0.4 | 11.2 | yes |
| level after 2 s held, re peak (dB) | -67.1 | -127 | -59.7 | -59.7 | -110 | 0.5 |  |
| high bands lag low bands by (ms) | -1 | 0 | -0.7 | -0.7 | -2 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 0.3 | 0 | 0.5 | 0.5 | 1.8 | 0.0 |  |
| fundamental share (tristimulus 1) | 0.999 | 0.951 | 0.706 | 0.706 | 0.811 | 2.0 | yes |
| partials 2-4 share (tristimulus 2) | 0.001 | 0.048 | 0.281 | 0.281 | 0.153 | 1.9 | yes |
| partials 5+ share (tristimulus 3) | 0 | 0.005 | 0.025 | 0.025 | 0.024 | 0.3 |  |
| loudness range, velocity 40 to 127 (dB) | 14.0 | 16.7 | 16.7 | 16.7 | 20.1 | 1.6 | yes |
| pitch wobble / vibrato (p-p) (c) | 55.1 | 87.7 | 86.6 | 89.2 | 102 | 4.3 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0139 (nearest 0.00917) | fonts among themselves 0.00515 | | | | 2.7 | yes |

</details>

### 118 Melodic Tom: melodic-tom

Distance 2.82 (rank 61 of 129); the fonts disagree 1.2 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Slow attack (43.5 vs 4–14 ms)**: probable FAULT. A tom should peak at once. The ampA is 1 ms (src/lead.cc), so the peak probably comes late as the falling pitch (4 semitones, 120 ms) passes the filter. Look at the first 50 ms.
- **Brighter (7.3 vs 4.2–5.1 ERB), +8..+12 dB at 2.5–6 kHz**: the stick-noise chiff (0.6). Debatable.
- **A 100 ms note sounds 212 vs 92–122 ms; release 2000 ms**: relMul 0.5 plus the room. Partly ARTIFACT.
- **Odd/even +22.5 dB, jaggedness 8 dB**: MEASUREMENT ARTIFACT: a sine has almost no partials.
- **Evenly tuned (4 c vs 28–53)**: DELIBERATE.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (7.26 ERB | 5.05 | 5.03 | 4.15 | 4.22), outside the fonts, 5.9 x.
2. **odd / even partials**: more odd partials (hollower) (22.3 dB | -1.7 | -6.8 | -3.2 | -9.4), outside the fonts, 7.1 x.
3. **attack time (-20 to -1 dB)**: a slower attack (43.5 ms | 9 | 4 | 14.0 | 5), outside the fonts, 3.0 x.
4. **sounding length of a 100 ms note [room]**: longer staccato (211 ms | 105 | 122 | 103 | 92.0), outside the fonts, 4.1 x.
5. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-66.0 dB | -126 | -118 | -126 | -112), outside the fonts, 7.9 x.
6. **pitch scatter from key to key**: more evenly tuned (4.5 c | 38.5 | 42.4 | 27.9 | 52.5), outside the fonts, 2.9 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +3 | -10 | +7 | +1 | +0 | -4 |
| re TimGM6mb | -9 | -22 | +2 | +7 | +8 | +7 |
| re Jnsgm2 | -12 | -10 | -4 | +2 | +10 | +12 |
| re SGM | -21 | -21 | +0 | +10 | +12 | +3 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.1 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 3.9 dB rms; TimGM6mb: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.4 ERB, loudness range +4 dB, envelope 4.6 dB rms; Jnsgm2: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening -0.5 ERB, loudness range +3 dB, envelope 2.1 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 1.19 | 1.52 | 1.04 | 1.15 | 1.03 | 0.2 |  |
| attack time (-20 to -1 dB) (ms) | 43.5 | 9 | 4 | 14.0 | 5 | 3.0 | yes |
| spectral shape of the attack, note for note | 11.6 (nearest 10.4) | fonts among themselves 9.26 | | | | 1.3 |  |
| brightness over the keyboard (ERB/oct) | 3.57 | 1.31 | 1.32 | 2.22 | 1.05 | 3.9 | yes |
| key-to-key brightness scatter (ERB) | 0.221 | 0.211 | 0.242 | 0.241 | 0.53 | 0.1 |  |
| brightening from velocity 40 to 127 (ERB) | -0.01 | 0.02 | 0 | -0 | -0.28 | 0.0 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 7.26 | 5.05 | 5.03 | 4.15 | 4.22 | 5.9 | yes |
| decay rate of a held note (dB/s) | 23.6 | 86.5 | 18.2 | 27.6 | 52.8 | 0.6 |  |
| onset delay (ms) | 12.0 | 2 | 1 | 0 | 3 | 1.1 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.84 | -2.86 | -4.41 | -2.63 | -3.24 | 0.2 |  |
| envelope curve, note for note (dB rms) | 15 (nearest 9.42) | fonts among themselves 14.6 | | | | 1.0 |  |
| pitch re 12-TET A440 (median) (c) | 13.9 | -23.0 | -28.6 | 15.2 | 0 | 1.2 |  |
| pitch scatter from key to key (c) | 4.5 | 38.5 | 42.4 | 27.9 | 52.5 | 2.9 | yes |
| noisiness (spectral flatness) (dB) | -45.5 | -34.7 | -59.5 | -44.0 | -55.8 | 0.4 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -66.0 | -126 | -118 | -126 | -112 | 7.9 | yes |
| pitch at the start re settled (c) | 0 | 0 | 0 | 0 | 0 | 0.0 |  |
| stereo coherence | 0.959 | 1 | 1 | 1 | 1 | 0.5 | yes |
| partial-to-partial jaggedness (dB) | 7.75 | 2.3 | 2.05 | 1.8 | 1.4 | 5.8 | yes |
| dip between legato notes (dB) | 13.5 | 14.2 | 11.5 | 11.3 | 13.3 | 0.5 |  |
| level evenness of a legato scale (dB) | 1.7 | 0.5 | 0.2 | 0.5 | 2.4 | 1.1 |  |
| level over the keyboard (dB/oct) | -1.1 | -1.49 | -1.36 | 0.17 | -1.34 | 0.2 |  |
| level against the other instruments (velocity 96) (dB) | 0.8 | -4.4 | -7.1 | 2.4 | -0.3 | 0.7 |  |
| key-to-key level scatter (dB) | 1.04 | 0.02 | 0.03 | 0.05 | 0.38 | 2.0 | yes |
| odd / even partials (dB) | 22.3 | -1.7 | -6.8 | -3.2 | -9.4 | 7.1 | yes |
| onset jitter in a phrase (ms) | 17.3 | 1.6 | 40.7 | 6.8 | 0.9 | 0.7 |  |
| onset in a phrase (re the note-on) (ms) | 8 | 0 | -2 | -2 | -1 | 0.9 | yes |
| onset sharpness (dB/ms) | 25.4 | 33.7 | 4.3 | 2.4 | 50.5 | 0.3 |  |
| overtone levels, note for note (dB rms) | 20.9 (nearest 15.2) | fonts among themselves 12.4 | | | | 1.7 |  |
| panning across the keys (dB/oct) | 0.03 | 0 | 0 | 0 | -0.01 | 0.0 |  |
| partial envelopes alike | 0.937 | 0.997 | 0.971 | 0.946 | 0.992 | 0.4 |  |
| release time (30 dB after note-off) (ms) | 2000 | - | 702 | 157 | 33.0 | 1.7 | yes |
| spectrogram (1 - correlation), note for note | 0.573 (nearest 0.539) | fonts among themselves 0.332 | | | | 1.7 | yes |
| spectral shape of the body, note for note | 17.5 (nearest 13.9) | fonts among themselves 11.5 | | | | 1.5 |  |
| sounding length of a 100 ms note (ms) | 211 | 105 | 122 | 103 | 92.0 | 4.1 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.26 | 0.22 | -0.06 | 0.55 | 1.14 | 0.2 |  |
| level after 2 s held, re peak (dB) | -49.7 | -126 | -51.5 | -60.2 | -110 | 1.0 | yes |
| high bands lag low bands by (ms) | -1 | 0 | 0 | 0 | 0 | 0.2 |  |
| transient coherence: attack spread over bands (ms) | 2 | 0 | 0 | 0 | 0 | 0.4 |  |
| fundamental share (tristimulus 1) | 0.988 | 0.598 | 0.876 | 0.637 | 0.987 | 1.2 |  |
| partials 2-4 share (tristimulus 2) | 0.01 | 0.321 | 0.1 | 0.341 | 0.012 | 1.2 |  |
| partials 5+ share (tristimulus 3) | 0.001 | 0.053 | 0.008 | 0.02 | 0.001 | 0.2 |  |
| loudness range, velocity 40 to 127 (dB) | 15.9 | 16.7 | 16.6 | 16.7 | 20.0 | 0.5 |  |
| pitch wobble / vibrato (p-p) (c) | 56.8 | 96.2 | 27.3 | 61.9 | 87.8 | 0.5 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.014 (nearest 0.0131) | fonts among themselves 0.00519 | | | | 2.7 | yes |

</details>

### 119 Synth Drum: synth-drum

Distance 3.03 (rank 46 of 129); the fonts disagree 1.3 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Brighter (7.1 vs 4.0–5.0 ERB; +10..+19 dB at 400 Hz–1 kHz, −13..−23 dB at 150–400 Hz)**: DELIBERATE design (an analog drum through a resonant filter, src/lead.cc), but it tracks the key more steeply than the fonts (3.5 vs 0.9–1.4 ERB/oct). Debatable: the fonts' synth drum stays low and thuddy on every key.
- **Less pitch sweep (wobble 36 vs 87–98 c)**: the 12-semitone sweep over 45 ms reads as less than the fonts' longer drop. Consider a slower `pitchTau`.
- **Sustains more (−65 vs −108..−130 dB), release 2000 ms**: room ARTIFACT.
- **Evenly tuned (0.7 vs 23–50 c), loudness range 14 vs 17–20 dB**: DELIBERATE / minor.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightness (spectral centroid, velocity 96)**: brighter (7.14 ERB | 4.98 | 4.98 | 4.98 | 4), outside the fonts, 4.4 x.
2. **pitch wobble / vibrato (p-p)**: wobbles less (35.6 c | 98.0 | 88.4 | 92.3 | 87.4), outside the fonts, 10.3 x.
3. **level after 2 s held, re peak**: sustains more (-64.7 dB | -130 | -120 | -120 | -108), outside the fonts, 5.1 x.
4. **brightness over the keyboard**: brightens faster up the keyboard (3.52 ERB/oct | 1.35 | 1.43 | 1.38 | 0.89), outside the fonts, 7.2 x.
5. **pitch scatter from key to key**: more evenly tuned (0.7 c | 43.9 | 30.4 | 49.8 | 22.7), outside the fonts, 2.7 x.
6. **loudness range, velocity 40 to 127**: a narrower dynamic range (14.0 dB | 16.7 | 16.6 | 16.7 | 20.1), outside the fonts, 1.5 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -6 | -13 | +19 | -1 | -5 | +4 |
| re TimGM6mb | -10 | -23 | +11 | +3 | +4 | +8 |
| re Jnsgm2 | -10 | -18 | +10 | +2 | +3 | +8 |
| re SGM | -14 | -14 | +12 | +10 | -2 | -8 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -11 -5 +0 +3; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.0 ERB, attack +2.2 ERB, velocity brightening -0.2 ERB, loudness range +3 dB, envelope 5.7 dB rms; TimGM6mb: centroid +0.8 ERB, attack +1.3 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 3.6 dB rms; Jnsgm2: centroid -0.5 ERB, attack +1.3 ERB, velocity brightening -0.5 ERB, loudness range +3 dB, envelope 3.6 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 3.18 | 0.41 | 1.48 | 1.62 | 2.67 | 1.4 | yes |
| attack time (-20 to -1 dB) (ms) | 36.0 | 25.0 | 3 | 3 | 149 | 0.7 |  |
| spectral shape of the attack, note for note | 15.8 (nearest 15.4) | fonts among themselves 8.94 | | | | 1.8 | yes |
| brightness over the keyboard (ERB/oct) | 3.52 | 1.35 | 1.43 | 1.38 | 0.89 | 7.2 | yes |
| key-to-key brightness scatter (ERB) | 0.194 | 0.191 | 0.259 | 0.209 | 0.469 | 0.2 |  |
| brightening from velocity 40 to 127 (ERB) | 0.02 | 0.02 | -0.02 | 0.02 | -0.29 | 0.1 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 7.14 | 4.98 | 4.98 | 4.98 | 4 | 4.4 | yes |
| decay rate of a held note (dB/s) | 33.0 | 135 | 137 | 137 | 32.3 | 2.0 |  |
| onset delay (ms) | 12.0 | 1 | 2 | 2 | 2 | 1.0 | yes |
| Plomp-Levelt dissonance of a note (log2) | -2.65 | -5.32 | -8.97 | -8.97 | 0.01 | 1.0 |  |
| envelope curve, note for note (dB rms) | 20.2 (nearest 12.4) | fonts among themselves 11.1 | | | | 1.8 |  |
| pitch re 12-TET A440 (median) (c) | 0.9 | -13.9 | -2.2 | 11.9 | -17.5 | 0.6 |  |
| pitch scatter from key to key (c) | 0.7 | 43.9 | 30.4 | 49.8 | 22.7 | 2.7 | yes |
| noisiness (spectral flatness) (dB) | -38.3 | -53.3 | -53.3 | -52.0 | -40.3 | 2.2 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -66.7 | -130 | -120 | -120 | -112 | 5.8 | yes |
| pitch at the start re settled (c) | 0 | -1.8 | 0 | 0 | 0 | 0.0 |  |
| stereo coherence | 0.978 | 1 | 1 | 1 | 1 | 0.3 |  |
| partial-to-partial jaggedness (dB) | 3.25 | 2.4 | 3.3 | 3.2 | 3 | 0.1 |  |
| dip between legato notes (dB) | 12.2 | 16.9 | 15.6 | 14.9 | 21.1 | 1.3 | yes |
| level evenness of a legato scale (dB) | 1.7 | 1.1 | 0.2 | 0.2 | 2.3 | 1.0 |  |
| level over the keyboard (dB/oct) | -0.42 | 1.35 | -0.11 | -0.07 | -1.28 | 0.3 |  |
| level against the other instruments (velocity 96) (dB) | 4 | 4.9 | -1.1 | -0.9 | 3.7 | 0.9 |  |
| key-to-key level scatter (dB) | 0.79 | 1.29 | 0.1 | 0.03 | 0.89 | 0.5 |  |
| odd / even partials (dB) | 3.2 | -1.7 | -1.4 | -1.5 | -1 | 2.3 | yes |
| onset jitter in a phrase (ms) | 29.7 | 19.3 | 1.2 | 0.8 | 7.2 | 2.8 | yes |
| onset in a phrase (re the note-on) (ms) | 7 | -2 | -2 | -2 | -3 | 0.9 | yes |
| onset sharpness (dB/ms) | 26.6 | 5.5 | 7.3 | 7.5 | 53.8 | 0.8 |  |
| overtone levels, note for note (dB rms) | 18.9 (nearest 15.1) | fonts among themselves 9.08 | | | | 2.1 | yes |
| panning across the keys (dB/oct) | -0.02 | -0 | -0 | -0 | -0.01 | 0.0 |  |
| partial envelopes alike | 0.986 | 0.998 | 0.998 | 0.998 | 0.976 | 0.1 |  |
| spectrogram (1 - correlation), note for note | 0.501 (nearest 0.465) | fonts among themselves 0.229 | | | | 2.2 | yes |
| spectral shape of the body, note for note | 18.3 (nearest 17.7) | fonts among themselves 8.74 | | | | 2.1 | yes |
| sounding length of a 100 ms note (ms) | 124 | 41.0 | 171 | 157 | 312 | 0.3 |  |
| partial stretch (inharmonicity) (c/partial) | -0.06 | 0.46 | 0.09 | 0.58 | -0.68 | 0.5 |  |
| level after 2 s held, re peak (dB) | -64.7 | -130 | -120 | -120 | -108 | 5.1 | yes |
| high bands lag low bands by (ms) | -10.8 | 0 | 0 | 0 | 0 | 2.2 | yes |
| transient coherence: attack spread over bands (ms) | 10.3 | 0 | 0 | 0 | 0 | 2.1 | yes |
| fundamental share (tristimulus 1) | 0.995 | 0.819 | 0.113 | 0.412 | 0.988 | 0.9 |  |
| partials 2-4 share (tristimulus 2) | 0.003 | 0.044 | 0.177 | 0.124 | 0.003 | 0.9 |  |
| partials 5+ share (tristimulus 3) | 0.001 | 0.051 | 0.157 | 0.159 | 0.009 | 1.4 |  |
| loudness range, velocity 40 to 127 (dB) | 14.0 | 16.7 | 16.6 | 16.7 | 20.1 | 1.5 | yes |
| pitch wobble / vibrato (p-p) (c) | 35.6 | 98.0 | 88.4 | 92.3 | 87.4 | 10.3 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.00838 (nearest 0.00752) | fonts among themselves 0.00358 | | | | 2.3 | yes |

</details>

### 120 Reverse Cymbal: reverse-cymbal

Distance 3.59 (rank 30 of 129); the fonts disagree 1.4 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Dark and tonal, not a hiss (12.8 vs 22.6–27.9 ERB; flatness −34.5 vs −9.5..−13.5 dB; +15..+33 dB below 400 Hz)**: FAULT. The pre-rendered crash (src/modalinst.cc) uses 192 modes **log-spaced** from 81 Hz to 14 kHz, so above ~2 kHz they are 100+ Hz apart instead of the ~14 Hz the comment intends, and the highs come out sparse and tonal. Every font's swell is a bright wash. Space the modes evenly in frequency, or reuse the drum kit's crash (cymbals_built.md).
- **Narrow loudness range (10.3 vs 16.7–20 dB)**: the `revGain = 0.3 + amp` floor. Minor FAULT.
- **Release 555 ms**: room ARTIFACT (note-off stops the swell at once).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (7.1 dB | 16.7 | 16.7 | 16.7 | 20.0), outside the fonts, 5.8 x.
2. **brightness (spectral centroid, velocity 96)**: darker (12.8 ERB | 26.8 | 27.9 | 24.9 | 22.6), outside the fonts, 4.9 x.
3. **partials 5+ share (tristimulus 3)**: weaker high partials (0.459 | 0.954 | 0.982 | 0.99 | 0.848), outside the fonts, 7.2 x.
4. **noisiness (spectral flatness)**: purer (-34.5 dB | -9.5 | -13.5 | -13.2 | -11.4), outside the fonts, 11.1 x.
5. **level against the other instruments (velocity 96)**: quieter (-34.5 dB | -13.7 | -12.2 | -32.4 | -9), outside the fonts, 1.8 x.
6. **partials 2-4 share (tristimulus 2)**: stronger low partials (0.344 | 0.042 | 0.018 | 0.007 | 0.139), outside the fonts, 4.5 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +19 | +16 | +14 | +3 | -16 | -16 |
| re TimGM6mb | +33 | +21 | +10 | +2 | -17 | -18 |
| re Jnsgm2 | +15 | +15 | +9 | +3 | -8 | -16 |
| re SGM | +29 | +11 | +4 | +1 | -13 | -10 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -5 -0 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack -0.0 ERB, velocity brightening +1.4 ERB, loudness range +3 dB, envelope 0.3 dB rms; TimGM6mb: centroid -5.5 ERB, attack -6.2 ERB, velocity brightening +0.8 ERB, loudness range +3 dB, envelope 10.7 dB rms; Jnsgm2: centroid -0.1 ERB, attack +0.0 ERB, velocity brightening +0.2 ERB, loudness range +3 dB, envelope 0.7 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -1.23 | -1.32 | -3.66 | -0.4 | -1.29 | 0.0 |  |
| attack time (-20 to -1 dB) (ms) | 1394 | 1051 | 420 | 1122 | 634 | 1.1 | yes |
| spectral shape of the attack, note for note | 17.6 (nearest 15.9) | fonts among themselves 11.8 | | | | 1.5 | yes |
| brightness over the keyboard (ERB/oct) | -0.47 | 1.52 | 2.88 | 5.77 | 2.06 | 1.4 | yes |
| key-to-key brightness scatter (ERB) | 0.88 | 0.038 | 0.105 | 0.838 | 1.36 | 0.6 |  |
| brightening from velocity 40 to 127 (ERB) | -0 | -0 | -0.01 | 0.2 | 0.85 | 0.2 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 12.8 | 26.8 | 27.9 | 24.9 | 22.6 | 4.9 | yes |
| onset delay (ms) | 1158 | 857 | 160 | 734 | 323 | 1.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | 1.24 | 1.14 | 1.39 | 1.83 | 1.06 | 0.1 |  |
| envelope curve, note for note (dB rms) | 26.5 (nearest 24.9) | fonts among themselves 16.4 | | | | 1.6 | yes |
| pitch re 12-TET A440 (median) (c) | 18.7 | 17.8 | 22.9 | -24.7 | 4.3 | 0.3 |  |
| pitch scatter from key to key (c) | 38.8 | 55.4 | 35.0 | 0.9 | 48.3 | 0.1 |  |
| noisiness (spectral flatness) (dB) | -34.5 | -9.5 | -13.5 | -13.2 | -11.4 | 11.1 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -62.8 | -119 | -113 | -106 | -103 | 6.0 | yes |
| pitch at the start re settled (c) | -0.5 | -1.8 | 2 | -1 | 4.4 | 0.2 |  |
| stereo coherence | 0.63 | 1 | 1 | 1 | 1 | 4.9 | yes |
| partial-to-partial jaggedness (dB) | 2.3 | 4.9 | 3.6 | 4.7 | 4.7 | 2.4 | yes |
| dip between legato notes (dB) | 10.0 | 18.2 | 12.4 | 22.4 | 15.2 | 1.3 | yes |
| level evenness of a legato scale (dB) | 2.9 | 0.6 | 0.9 | 1.9 | 3 | 1.2 |  |
| level over the keyboard (dB/oct) | -2.93 | 1.04 | -2.11 | 7.12 | 0.18 | 0.8 | yes |
| level against the other instruments (velocity 96) (dB) | -34.5 | -13.7 | -12.2 | -32.4 | -9 | 1.8 | yes |
| key-to-key level scatter (dB) | 1.94 | 0.04 | 0.55 | 1.24 | 1.33 | 1.6 | yes |
| odd / even partials (dB) | -0.5 | 0.7 | 1.1 | -3.8 | -0.6 | 0.2 |  |
| onset jitter in a phrase (ms) | 6.1 | 44.9 | 11.1 | 28.1 | 45.1 | 1.8 | yes |
| onset in a phrase (re the note-on) (ms) | 7 | 2 | -1 | 4 | 20.0 | 0.4 |  |
| onset sharpness (dB/ms) | 6.7 | 3.8 | 4.7 | 3.4 | 3.8 | 1.5 | yes |
| overtone levels, note for note (dB rms) | 15.7 (nearest 14.3) | fonts among themselves 12.3 | | | | 1.3 |  |
| panning across the keys (dB/oct) | -0.08 | -0 | -0 | -0 | -0.01 | 0.1 |  |
| partial envelopes alike | 0.831 | 0.655 | 0.265 | 0.756 | 0.295 | 1.5 | yes |
| release time (30 dB after note-off) (ms) | 555 | 54.0 | - | 6 | 32.0 | 2.6 | yes |
| spectrogram (1 - correlation), note for note | 0.981 (nearest 0.81) | fonts among themselves 0.453 | | | | 2.2 | yes |
| spectral shape of the body, note for note | 18 (nearest 16.6) | fonts among themselves 11.4 | | | | 1.6 | yes |
| sounding length of a 100 ms note (ms) | 125 | 141 | 142 | 103 | 118 | 0.2 |  |
| partial stretch (inharmonicity) (c/partial) | -1.22 | -0.75 | 0.39 | -0.11 | -0.38 | 1.7 | yes |
| level after 2 s held, re peak (dB) | 7.2 | -41.8 | -113 | -6.4 | -11.8 | 0.6 | yes |
| high bands lag low bands by (ms) | 10.7 | 13.0 | 32.8 | 6 | 15.7 | 0.3 |  |
| transient coherence: attack spread over bands (ms) | 14.4 | 11.1 | 15.0 | 17.6 | 25.8 | 0.3 |  |
| fundamental share (tristimulus 1) | 0.074 | 0 | 0 | 0.001 | 0 | 1.1 | yes |
| partials 2-4 share (tristimulus 2) | 0.344 | 0.042 | 0.018 | 0.007 | 0.139 | 4.5 | yes |
| partials 5+ share (tristimulus 3) | 0.459 | 0.954 | 0.982 | 0.99 | 0.848 | 7.2 | yes |
| loudness range, velocity 40 to 127 (dB) | 7.1 | 16.7 | 16.7 | 16.7 | 20.0 | 5.8 | yes |
| pitch wobble / vibrato (p-p) (c) | 80.0 | 85.2 | 88.7 | 103 | 86.1 | 0.8 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0249 (nearest 0.0217) | fonts among themselves 0.0183 | | | | 1.4 |  |

</details>

### 121 Guitar Fret Noise: fret-noise

Distance 2.99 (rank 50 of 129); the fonts disagree 3.2 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Velocity hardly matters (loudness range 5.7 vs 16.6–20.1 dB; curve −6/−6/0/0)**: FAULT. In src/sfx.cc, velocity mainly shortens the slide, and the pluck amplitude is only `0.4 + vel`.
- **Speaks late in a phrase (54 ms vs −1..0), onset 86 ms**: DELIBERATE physics: the fingertip must slide before it squeaks (README "Sound effects"). For a GM effect it is late, though the fonts' onset delays range from 6 to 114 ms.
- **Sustains more (−50 vs −98..−123 dB), decays slower**: the string and guitar body ring after the squeak (DELIBERATE), plus the room.
- **Louder (−0.2 vs −11.7..−4.6 dB)**: trim. The fonts disagree widely (3.2 JND).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (6.3 dB | 16.6 | 16.6 | 16.7 | 20.1), outside the fonts, 5.9 x.
2. **level after 2 s held, re peak**: sustains more (-50.0 dB | -123 | -119 | -118 | -98.5), outside the fonts, 5.7 x.
3. **brightness (spectral centroid, velocity 96)**: brighter (22.8 ERB | 21.6 | 18.8 | 18.1 | 19.3), outside the fonts, 2.1 x.
4. **onset in a phrase (re the note-on)**: speaks later in a phrase (40.0 ms | -1 | -1 | -1 | 0), outside the fonts, 4.1 x.
5. **decay rate of a held note**: decays slower (26.8 dB/s | - | - | 40.6 | 43.9), outside the fonts, 2.6 x.
6. **key-to-key level scatter**: less even from key to key (1.24 dB | 0.03 | 0.03 | 0.44 | 0.84), outside the fonts, 2.0 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -8 | +1 | +7 | +6 | +13 | -21 |
| re TimGM6mb | -12 | +5 | +15 | +4 | +16 | -24 |
| re Jnsgm2 | -9 | +2 | +6 | +9 | +19 | -25 |
| re SGM | -15 | +3 | +12 | +15 | +18 | -32 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -5 -3 +0 +1; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.6 ERB, attack -0.9 ERB, velocity brightening +1.1 ERB, loudness range +4 dB, envelope 13.1 dB rms; TimGM6mb: centroid +0.1 ERB, attack +0.2 ERB, velocity brightening +0.9 ERB, loudness range +4 dB, envelope 5.9 dB rms; Jnsgm2: centroid -0.0 ERB, attack -0.7 ERB, velocity brightening +0.1 ERB, loudness range +3 dB, envelope 3.9 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -2.85 | -0.12 | 1.46 | -3.93 | 1.06 | 1.2 |  |
| attack time (-20 to -1 dB) (ms) | 164 | 146 | 52.0 | 113 | 80.0 | 1.1 | yes |
| spectral shape of the attack, note for note | 15 (nearest 12.2) | fonts among themselves 11.7 | | | | 1.3 |  |
| brightness over the keyboard (ERB/oct) | 1.58 | 0.54 | 2.95 | 3.3 | 4.78 | 0.7 |  |
| key-to-key brightness scatter (ERB) | 0.54 | 0.066 | 0.18 | 1.07 | 1.35 | 0.1 |  |
| brightening from velocity 40 to 127 (ERB) | -0.51 | 0.01 | -0 | -0 | 0.59 | 1.7 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 22.8 | 21.6 | 18.8 | 18.1 | 19.3 | 2.1 | yes |
| decay rate of a held note (dB/s) | 26.8 | - | - | 40.6 | 43.9 | 2.6 | yes |
| onset delay (ms) | 88.0 | 6 | 51.0 | 114 | 45.0 | 0.7 |  |
| Plomp-Levelt dissonance of a note (log2) | -0.03 | 2 | -0.51 | 1.08 | 1.67 | 1.1 |  |
| envelope curve, note for note (dB rms) | 29.9 (nearest 22.8) | fonts among themselves 26.4 | | | | 1.1 |  |
| pitch re 12-TET A440 (median) (c) | 20.0 | -9.6 | 0 | -59.3 | -20.5 | 1.2 | yes |
| pitch scatter from key to key (c) | 25.1 | 29.1 | 0 | 0.9 | 1.2 | 1.7 |  |
| noisiness (spectral flatness) (dB) | -24.1 | -11.0 | -17.8 | -14.2 | -12.7 | 3.1 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -92.8 | -123 | -119 | -118 | -102 | 2.5 | yes |
| pitch at the start re settled (c) | 2.5 | 0 | -8.4 | -5.8 | 25.1 | 0.3 |  |
| stereo coherence | 0.611 | 1 | 1 | 1 | 1 | 5.2 | yes |
| partial-to-partial jaggedness (dB) | 5.15 | 3.5 | 3.4 | 4.8 | 4 | 1.4 |  |
| dip between legato notes (dB) | 6.9 | 2.2 | 0 | 35.3 | 8.1 | 0.1 |  |
| level evenness of a legato scale (dB) | 1.5 | 0.1 | 0.3 | 0.3 | 1.5 | 1.2 |  |
| level over the keyboard (dB/oct) | -0.21 | -0.51 | -1.52 | -2.57 | -2.9 | 1.5 |  |
| level against the other instruments (velocity 96) (dB) | -8.4 | -4.6 | -9.3 | -8.1 | -11.7 | 0.1 |  |
| key-to-key level scatter (dB) | 1.24 | 0.03 | 0.03 | 0.44 | 0.84 | 2.0 | yes |
| odd / even partials (dB) | -2.2 | -0.6 | -0.2 | 9.8 | -3 | 0.3 |  |
| onset jitter in a phrase (ms) | 43.9 | 0.3 | 43.9 | 46.8 | 1.2 | 0.9 |  |
| onset in a phrase (re the note-on) (ms) | 40.0 | -1 | -1 | -1 | 0 | 4.1 | yes |
| onset sharpness (dB/ms) | 4.5 | 8.5 | 3.1 | 6.5 | 45.0 | 0.1 |  |
| overtone levels, note for note (dB rms) | 15 (nearest 13.5) | fonts among themselves 18.7 | | | | 0.8 |  |
| panning across the keys (dB/oct) | 0.11 | -0 | 0 | 0 | -0.01 | 0.1 |  |
| partial envelopes alike | 0.917 | 0.998 | 0.999 | 0.995 | 0.997 | 0.8 | yes |
| spectrogram (1 - correlation), note for note | 0.595 (nearest 0.458) | fonts among themselves 0.577 | | | | 1.0 |  |
| spectral shape of the body, note for note | 15.8 (nearest 12) | fonts among themselves 13.4 | | | | 1.2 |  |
| sounding length of a 100 ms note (ms) | 400 | 55.0 | 139 | 165 | 146 | 1.9 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.08 | -0.02 | 3.6 | -0.65 | -1.37 | 0.2 |  |
| level after 2 s held, re peak (dB) | -50.0 | -123 | -119 | -118 | -98.5 | 5.7 | yes |
| high bands lag low bands by (ms) | 10.5 | 10.2 | 28.8 | 44.0 | 14.5 | 0.7 |  |
| transient coherence: attack spread over bands (ms) | 26.8 | 33.0 | 4.8 | 27.3 | 25.9 | 0.0 |  |
| fundamental share (tristimulus 1) | 0 | 0.006 | 0 | 0 | 0.005 | 0.0 |  |
| partials 2-4 share (tristimulus 2) | 0.019 | 0.136 | 0 | 0.002 | 0.007 | 0.2 |  |
| partials 5+ share (tristimulus 3) | 0.98 | 0.829 | 1 | 0.998 | 0.986 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 6.3 | 16.6 | 16.6 | 16.7 | 20.1 | 5.9 | yes |
| pitch wobble / vibrato (p-p) (c) | 24.5 | 68.9 | 42.4 | 104 | 90.0 | 1.8 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0182 (nearest 0.0151) | fonts among themselves 0.00781 | | | | 2.3 | yes |

</details>

### 122 Breath Noise: breath-noise

Distance 3.02 (rank 48 of 129); the fonts disagree 2.8 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **No velocity response (loudness range 0.3 vs 16.7–20.1 dB)**: FAULT. The tract's pressure follows velocity (`300 + 900 * vel_`), but something (the whisper's level or the velgain table) cancels it. Velocity does nothing audible.
- **A held whisper, not a puff (−3.5 dB after 2 s vs −81..−120)**: DELIBERATE (README "Sound effects": the vocal tract whispering while the key is held). The fonts play short breaths.
- **Brighter (32.5 vs 7.3–26 ERB)**: the fonts disagree enormously (2.8 JND). Not a verdict.
- **Partial envelopes alike 0.008**: MEASUREMENT ARTIFACT (noise).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (0.4 dB | 16.7 | 16.8 | 16.8 | 20.1), outside the fonts, 9.6 x.
2. **level after 2 s held, re peak**: sustains more (-3.5 dB | -120 | -114 | -111 | -81.4), outside the fonts, 5.6 x.
3. **partial envelopes alike**: partials living more their own lives (0.008 | 0.999 | 0.995 | 0.991 | 0.994), outside the fonts, 9.9 x.
4. **brightness (spectral centroid, velocity 96)**: brighter (32.5 ERB | 21.9 | 9.45 | 7.34 | 26.0), outside the fonts, 1.8 x.
5. **envelope curve, note for note (dB rms)**: pajupilli further from the fonts than they are from each other (median distance to the fonts 41.9, nearest 39; the fonts among themselves 15.2), outside.
6. **dip between legato notes**: smoother legato (2.5 dB | 11.0 | 6.1 | 12.0 | 9.1), outside the fonts, 2.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -39 | -9 | -6 | -7 | -1 | +29 |
| re TimGM6mb | -53 | -35 | -20 | -6 | +13 | +46 |
| re Jnsgm2 | -52 | -33 | -17 | +2 | +18 | +31 |
| re SGM | -24 | -4 | -4 | -1 | +4 | +11 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -0 -0 +0 +0; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack +0.0 ERB, velocity brightening +1.0 ERB, loudness range +3 dB, envelope 3.3 dB rms; TimGM6mb: centroid -0.1 ERB, attack -0.3 ERB, velocity brightening -0.3 ERB, loudness range +3 dB, envelope 3.1 dB rms; Jnsgm2: centroid -0.2 ERB, attack +0.8 ERB, velocity brightening -0.5 ERB, loudness range +3 dB, envelope 3.4 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.17 | 1.17 | 6.26 | 8.75 | 0.94 | 0.9 | yes |
| attack time (-20 to -1 dB) (ms) | 196 | 88.0 | 58.0 | 58.0 | 5 | 0.8 | yes |
| spectral shape of the attack, note for note | 18.9 (nearest 13.5) | fonts among themselves 15.6 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | 0.39 | 1.41 | 3.45 | 3.35 | 5.09 | 1.6 | yes |
| key-to-key brightness scatter (ERB) | 0.974 | 0.068 | 0.492 | 0.283 | 1.03 | 1.2 |  |
| brightening from velocity 40 to 127 (ERB) | -0.05 | 0.01 | -0 | 0.02 | 0.65 | 0.2 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 32.5 | 21.9 | 9.45 | 7.34 | 26.0 | 1.8 | yes |
| onset delay (ms) | 78.0 | 61.0 | 49.0 | 45.0 | 4 | 1.1 | yes |
| Plomp-Levelt dissonance of a note (log2) | 1.94 | 2.41 | -3.2 | -4.06 | 0.78 | 1.0 |  |
| envelope curve, note for note (dB rms) | 41.9 (nearest 39) | fonts among themselves 15.2 | | | | 2.7 | yes |
| pitch re 12-TET A440 (median) (c) | -3.8 | -21.1 | 2.9 | -2.4 | 15.0 | 0.2 |  |
| pitch scatter from key to key (c) | 27.8 | 32.3 | 10.6 | 3.9 | 22.7 | 0.8 |  |
| noisiness (spectral flatness) (dB) | -7.7 | -15.0 | -41.5 | -34.1 | -11.2 | 1.1 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -75.3 | -120 | -114 | -111 | -86.8 | 2.2 | yes |
| pitch at the start re settled (c) | 4.5 | 33.4 | 6.6 | 5.6 | -9.2 | 0.1 |  |
| stereo coherence | 0.772 | 1 | 1 | 1 | 1 | 3.0 | yes |
| partial-to-partial jaggedness (dB) | 3.3 | 2.5 | 7.55 | 5.1 | 7.3 | 1.1 |  |
| dip between legato notes (dB) | 2.5 | 11.0 | 6.1 | 12.0 | 9.1 | 2.6 | yes |
| level evenness of a legato scale (dB) | 0.7 | 0.3 | 0.6 | 1.2 | 9.3 | 0.0 |  |
| level over the keyboard (dB/oct) | 0.11 | -0.89 | 0.95 | 2.52 | -2.76 | 0.0 |  |
| level against the other instruments (velocity 96) (dB) | -16.9 | -3.2 | -11.6 | -12.0 | -27.5 | 0.4 |  |
| key-to-key level scatter (dB) | 2.44 | 0.02 | 0.57 | 0.12 | 2.63 | 1.6 |  |
| odd / even partials (dB) | -1 | -0.7 | 20.4 | 13.5 | 7.6 | 1.1 |  |
| onset jitter in a phrase (ms) | 27.9 | 19.0 | 28.8 | 35.6 | 44.7 | 0.3 |  |
| onset in a phrase (re the note-on) (ms) | 30.0 | 45.0 | 12.0 | 0 | -1 | 1.0 |  |
| onset sharpness (dB/ms) | 2.3 | 4.4 | 5.4 | 5.7 | 37.7 | 0.2 | yes |
| overtone levels, note for note (dB rms) | 17.5 (nearest 10.7) | fonts among themselves 18.1 | | | | 1.0 |  |
| panning across the keys (dB/oct) | 0 | -0 | 0 | 0.03 | -0.01 | 0.0 |  |
| partial envelopes alike | 0.008 | 0.999 | 0.995 | 0.991 | 0.994 | 9.9 | yes |
| spectrogram (1 - correlation), note for note | 1.12 (nearest 0.899) | fonts among themselves 0.532 | | | | 2.1 | yes |
| spectral shape of the body, note for note | 22.7 (nearest 12.9) | fonts among themselves 16.7 | | | | 1.4 |  |
| sounding length of a 100 ms note (ms) | 400 | 279 | 232 | 245 | 170 | 2.1 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.17 | -0.55 | 0 | -0.56 | -0.86 | 0.8 |  |
| level after 2 s held, re peak (dB) | -3.5 | -120 | -114 | -111 | -81.4 | 5.6 | yes |
| high bands lag low bands by (ms) | -8.5 | -14.3 | -14.3 | -22.0 | -5.3 | 0.7 |  |
| transient coherence: attack spread over bands (ms) | 8.8 | 11.2 | 11.2 | 11.6 | 4.4 | 0.5 |  |
| fundamental share (tristimulus 1) | 0 | 0.007 | 0.814 | 0.967 | 0.002 | 0.9 |  |
| partials 2-4 share (tristimulus 2) | 0.024 | 0.037 | 0.169 | 0.031 | 0.007 | 0.1 |  |
| partials 5+ share (tristimulus 3) | 0.976 | 0.957 | 0.019 | 0.002 | 0.991 | 1.0 |  |
| loudness range, velocity 40 to 127 (dB) | 0.4 | 16.7 | 16.8 | 16.8 | 20.1 | 9.6 | yes |
| pitch wobble / vibrato (p-p) (c) | 93.4 | 109 | 62.6 | 84.2 | 67.1 | 0.8 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.03 (nearest 0.0298) | fonts among themselves 0.00948 | | | | 3.2 | yes |

</details>

### 123 Seashore: seashore

Distance 2.69 (rank 66 of 129); the fonts disagree 2.6 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Darker (13.1 vs 16.4–23.7 ERB), brighter attack (+3.9 vs −1.9..+0.2 ERB)**: DELIBERATE character: Minnaert bubbles and wash rather than the fonts' looped white-noise surf (README "Sound effects"). The fonts disagree widely (2.6 JND; TimGM6mb is 16.4).
- **Odd velocity curve (−9/−11/0/−1 dB; range 8.1 vs 9.3–20.1), level −5 dB/oct down the keys**: mostly a MEASUREMENT ARTIFACT. Wave timing is random, so a 2 s window catches crash or backwash by chance. Velocity sets the bubble rate, not a gain.
- **Louder (−0.6 vs −20.4..−5.1 dB)**: trim, though the fonts span 15 dB.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **brightening from velocity 40 to 127**: brightens less with velocity (-2.75 ERB | -0 | 0.25 | 0.21 | 0.73), outside the fonts, 8.2 x.
2. **key-to-key brightness scatter**: less even in tone from key to key (2.69 ERB | 0.083 | 0.238 | 0.096 | 1.19), outside the fonts, 4.6 x.
3. **attack brighter than the body by**: a brighter attack (3.33 ERB | -1.86 | -1.03 | 0.23 | -0.34), outside the fonts, 3.8 x.
4. **key-to-key level scatter**: less even from key to key (3.97 dB | 0.13 | 1.1 | 0.91 | 2.76), outside the fonts, 2.3 x.
5. **pitch re 12-TET A440 (median)**: flatter (-16.5 c | 16.3 | -5.6 | 22.4 | 17.5), outside the fonts, 2.4 x.
6. **brightness (spectral centroid, velocity 96)**: darker (14.9 ERB | 23.7 | 16.4 | 23.0 | 18.1), outside the fonts, 1.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +7 | +7 | +10 | +3 | -6 | -11 |
| re TimGM6mb | -9 | -2 | +7 | +4 | -1 | -3 |
| re Jnsgm2 | +10 | +8 | +9 | +0 | -8 | -8 |
| re SGM | +10 | +6 | +4 | -1 | -2 | -6 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -8 -6 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -12 -6 +0 +4; Jnsgm2 -5 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -4.1 ERB, attack +0.7 ERB, velocity brightening +0.8 ERB, loudness range +4 dB, envelope 6.8 dB rms; TimGM6mb: centroid -0.3 ERB, attack +0.1 ERB, velocity brightening +0.0 ERB, loudness range +3 dB, envelope 1.1 dB rms; Jnsgm2: centroid -1.9 ERB, attack -0.0 ERB, velocity brightening +0.9 ERB, loudness range +1 dB, envelope 2.9 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 3.33 | -1.86 | -1.03 | 0.23 | -0.34 | 3.8 | yes |
| attack time (-20 to -1 dB) (ms) | 582 | 229 | 301 | 1144 | 1236 | 0.0 |  |
| spectral shape of the attack, note for note | 8.23 (nearest 7.25) | fonts among themselves 12.3 | | | | 0.7 |  |
| brightness over the keyboard (ERB/oct) | 1.23 | 1.35 | 5.09 | 1.06 | 1.39 | 0.1 |  |
| key-to-key brightness scatter (ERB) | 2.69 | 0.083 | 0.238 | 0.096 | 1.19 | 4.6 | yes |
| brightening from velocity 40 to 127 (ERB) | -2.75 | -0 | 0.25 | 0.21 | 0.73 | 8.2 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 14.9 | 23.7 | 16.4 | 23.0 | 18.1 | 1.6 | yes |
| onset delay (ms) | 302 | 227 | 243 | 745 | 913 | 0.6 |  |
| Plomp-Levelt dissonance of a note (log2) | 1.77 | 1.9 | 2.16 | 2.54 | 2.79 | 1.3 |  |
| envelope curve, note for note (dB rms) | 10.5 (nearest 8.62) | fonts among themselves 12.4 | | | | 0.8 |  |
| pitch re 12-TET A440 (median) (c) | -16.5 | 16.3 | -5.6 | 22.4 | 17.5 | 2.4 | yes |
| pitch scatter from key to key (c) | 49.1 | 34.2 | 57.4 | 27.9 | 28.5 | 1.2 |  |
| noisiness (spectral flatness) (dB) | -12.9 | -3.4 | -37.7 | -4.9 | -7.3 | 0.4 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -23.1 | -47.4 | -36.0 | -23.9 | -95.1 | 0.5 |  |
| pitch at the start re settled (c) | -3 | -5 | 15.2 | 3.4 | -25.3 | 0.1 |  |
| stereo coherence | 0.289 | 0.009 | 1 | 0.95 | 1 | 1.4 |  |
| partial-to-partial jaggedness (dB) | 1.9 | 1 | 1 | 0.8 | 0.8 | 1.0 | yes |
| dip between legato notes (dB) | 8.4 | 4.6 | 9.4 | 4.3 | 5.3 | 1.4 |  |
| level evenness of a legato scale (dB) | 5.6 | 1.2 | 4.2 | 2.2 | 7.4 | 0.8 |  |
| level over the keyboard (dB/oct) | -0.48 | 0.3 | 2.55 | -1.21 | 0.6 | 0.5 |  |
| level against the other instruments (velocity 96) (dB) | -5.2 | -7.1 | -5.1 | -16.5 | -20.4 | 0.9 |  |
| key-to-key level scatter (dB) | 3.97 | 0.13 | 1.1 | 0.91 | 2.76 | 2.3 | yes |
| odd / even partials (dB) | -2.2 | -0.2 | -0.2 | -0.5 | -0.3 | 1.0 | yes |
| onset jitter in a phrase (ms) | 60.6 | 50.2 | 46.4 | 50.4 | 43.9 | 2.5 | yes |
| onset in a phrase (re the note-on) (ms) | 36.0 | 108 | 13.0 | 52.0 | 21.0 | 0.0 |  |
| onset sharpness (dB/ms) | 9.4 | 4.7 | 20.3 | 4.5 | 6.3 | 0.5 |  |
| overtone levels, note for note (dB rms) | 7.43 (nearest 6.08) | fonts among themselves 4.32 | | | | 1.7 | yes |
| panning across the keys (dB/oct) | -0.09 | -0.51 | 0 | -0.1 | -0.01 | 0.0 |  |
| partial envelopes alike | 0.303 | 0.581 | 0.485 | 0.75 | 0.679 | 2.5 | yes |
| release time (30 dB after note-off) (ms) | 2000 | 1330 | 1608 | 2000 | 533 | 0.5 |  |
| spectrogram (1 - correlation), note for note | 0.493 (nearest 0.472) | fonts among themselves 0.674 | | | | 0.7 |  |
| spectral shape of the body, note for note | 9.25 (nearest 6.93) | fonts among themselves 11.8 | | | | 0.8 |  |
| sounding length of a 100 ms note (ms) | 267 | 400 | 394 | 400 | 400 | 2.3 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.77 | -0.01 | -2 | -0.96 | -1.57 | 0.5 |  |
| level after 2 s held, re peak (dB) | -19.5 | -12.2 | -16.2 | -4.1 | -2.9 | 1.7 | yes |
| high bands lag low bands by (ms) | -18.5 | -4 | 12.5 | -8 | 17.0 | 1.8 | yes |
| transient coherence: attack spread over bands (ms) | 27.1 | 19.2 | 16.7 | 12.8 | 30.8 | 1.0 |  |
| fundamental share (tristimulus 1) | 0.007 | 0.018 | 0.046 | 0.005 | 0.006 | 0.1 |  |
| partials 2-4 share (tristimulus 2) | 0.494 | 0.165 | 0.375 | 0.103 | 0.391 | 1.6 | yes |
| partials 5+ share (tristimulus 3) | 0.261 | 0.831 | 0.564 | 0.893 | 0.603 | 2.8 | yes |
| loudness range, velocity 40 to 127 (dB) | 9.4 | 16.6 | 16.6 | 9.3 | 20.1 | 1.3 |  |
| pitch wobble / vibrato (p-p) (c) | 108 | 109 | 109 | 108 | 117 | 0.3 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0219 (nearest 0.0207) | fonts among themselves 0.0244 | | | | 0.9 |  |

</details>

### 124 Bird Tweet: bird-tweet

Distance 1.54 (rank 124 of 129); the fonts disagree 3.1 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Starts sharp (+44 c vs 0–11)**: DELIBERATE. Each syllable sweeps down (README "Sound effects", after Gardner et al.).
- **Speaks later in a phrase (48 vs −2..6 ms)**: the pressure gesture's rise. Minor; the fonts' attacks range from 107 to 601 ms anyway.
- **Rings 583 ms after the key**: the bird finishes its syllable, plus the room. ARTIFACT.
- **Louder (+4.1 vs −5.1..+0.8 dB)**: small trim. The fonts disagree widely (3.1 JND), so this is the closest of the effects (rank 113).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **pitch at the start re settled**: starts sharper (54.3 c | 0 | 0 | 11.2 | 0), outside the fonts, 9.7 x.
2. **release time (30 dB after note-off) [room]**: rings longer after the key (539 ms | - | - | 40.0 | 7), outside the fonts, 4.0 x.
3. **onset in a phrase (re the note-on)**: speaks later in a phrase (33.0 ms | 6 | -2 | -2 | 0), outside the fonts, 3.4 x.
4. **key-to-key level scatter**: less even from key to key (1.7 dB | 0.03 | 0.71 | 0.81 | 0.63), outside the fonts, 2.1 x.
5. **stereo coherence [room]**: wider (0.643 | 1 | 1 | 0.945 | 1), outside the fonts, 4.8 x.
6. **noisiness (spectral flatness)**: purer (-53.2 dB | -21.5 | -26.7 | -43.9 | -18.8), outside the fonts, 2.3 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | - | -32 | -17 | +20 | -1 | -3 |
| re TimGM6mb | -26 | -25 | -14 | +27 | +16 | -4 |
| re Jnsgm2 | - | - | -27 | +12 | +14 | -10 |
| re SGM | - | - | - | +4 | -2 | -2 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -14 -6 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack -0.0 ERB, velocity brightening +1.5 ERB, loudness range +4 dB, envelope 2.4 dB rms; TimGM6mb: centroid +0.0 ERB, attack +0.1 ERB, velocity brightening +2.0 ERB, loudness range +3 dB, envelope 3.5 dB rms; Jnsgm2: centroid +0.1 ERB, attack +0.2 ERB, velocity brightening +0.5 ERB, loudness range +4 dB, envelope 2.4 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 1.25 | -1.93 | 0.65 | 2.16 | 2.29 | 0.1 |  |
| attack time (-20 to -1 dB) (ms) | 141 | 322 | 601 | 107 | 247 | 0.8 |  |
| spectral shape of the attack, note for note | 18.3 (nearest 17.3) | fonts among themselves 16.1 | | | | 1.1 |  |
| brightness over the keyboard (ERB/oct) | 4 | 0.88 | 6.45 | 5.2 | 2.28 | 0.1 |  |
| key-to-key brightness scatter (ERB) | 1.57 | 0.041 | 0.837 | 0.178 | 1.7 | 1.3 |  |
| brightening from velocity 40 to 127 (ERB) | 0.05 | -0 | 0 | 0 | 1.05 | 0.1 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 24.2 | 26.2 | 27.3 | 23.6 | 23.3 | 0.4 |  |
| onset delay (ms) | 32.0 | 20.0 | 27.0 | 86.0 | 41.0 | 0.1 |  |
| Plomp-Levelt dissonance of a note (log2) | 3.24 | 4.13 | 3.48 | 3.62 | 2.98 | 0.5 |  |
| envelope curve, note for note (dB rms) | 25.1 (nearest 18.3) | fonts among themselves 25.9 | | | | 1.0 |  |
| pitch re 12-TET A440 (median) (c) | 22.9 | 7.1 | -74.0 | 56.4 | 16.6 | 0.2 |  |
| pitch scatter from key to key (c) | 39.1 | 36.5 | 17.0 | 2.5 | 55.2 | 0.5 |  |
| noisiness (spectral flatness) (dB) | -53.2 | -21.5 | -26.7 | -43.9 | -18.8 | 2.3 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -84.9 | -122 | -126 | -122 | -103 | 3.3 | yes |
| pitch at the start re settled (c) | 54.3 | 0 | 0 | 11.2 | 0 | 9.7 | yes |
| stereo coherence | 0.643 | 1 | 1 | 0.945 | 1 | 4.8 | yes |
| partial-to-partial jaggedness (dB) | 2.3 | 2.6 | 1.65 | 2.8 | 2.7 | 0.4 |  |
| dip between legato notes (dB) | 10.3 | 27.5 | 0 | 17.4 | 5.9 | 0.1 |  |
| level evenness of a legato scale (dB) | 0.9 | 0.2 | 0 | 0.2 | 0.9 | 0.7 |  |
| level over the keyboard (dB/oct) | -1.82 | -0.43 | -6.46 | -0.22 | -0.79 | 0.4 |  |
| level against the other instruments (velocity 96) (dB) | 0.8 | 0.2 | 0.8 | -1.4 | -5.2 | 0.5 |  |
| key-to-key level scatter (dB) | 1.7 | 0.03 | 0.71 | 0.81 | 0.63 | 2.1 | yes |
| odd / even partials (dB) | -0.9 | -0.7 | -0.4 | -0.7 | -3.3 | 0.1 |  |
| onset jitter in a phrase (ms) | 45.4 | 52.7 | 35.6 | 2.1 | 8.2 | 0.9 |  |
| onset in a phrase (re the note-on) (ms) | 33.0 | 6 | -2 | -2 | 0 | 3.4 | yes |
| onset sharpness (dB/ms) | 15.6 | 6.8 | 37.7 | 3.1 | 5.9 | 0.5 |  |
| overtone levels, note for note (dB rms) | 27.2 (nearest 13.6) | fonts among themselves 19.7 | | | | 1.4 |  |
| panning across the keys (dB/oct) | -0.07 | -0 | 0 | 0.05 | -0.01 | 0.1 |  |
| partial envelopes alike | 0.843 | 0.954 | 0.997 | 0.595 | 0.359 | 0.2 |  |
| release time (30 dB after note-off) (ms) | 539 | - | - | 40.0 | 7 | 4.0 | yes |
| spectrogram (1 - correlation), note for note | 0.883 (nearest 0.366) | fonts among themselves 0.858 | | | | 1.0 |  |
| spectral shape of the body, note for note | 17.8 (nearest 11.5) | fonts among themselves 17.3 | | | | 1.0 |  |
| sounding length of a 100 ms note (ms) | 328 | 47.0 | 68.0 | 330 | 105 | 1.4 |  |
| partial stretch (inharmonicity) (c/partial) | -9.38 | -0.67 | 1.64 | -5.57 | -3.08 | 2.1 | yes |
| level after 2 s held, re peak (dB) | -16.2 | -122 | -126 | -35.0 | -32.4 | 1.3 | yes |
| high bands lag low bands by (ms) | -39.0 | 9 | 17.3 | -56.5 | -23.0 | 0.9 |  |
| transient coherence: attack spread over bands (ms) | 7.1 | 10.7 | 21.4 | 31.6 | 17.3 | 1.2 | yes |
| fundamental share (tristimulus 1) | 0 | 0 | 0.166 | 0 | 0 | 0.0 |  |
| partials 2-4 share (tristimulus 2) | 0 | 0.008 | 0.172 | 0 | 0 | 0.0 |  |
| partials 5+ share (tristimulus 3) | 1 | 0.992 | 0.649 | 1 | 1 | 0.0 |  |
| loudness range, velocity 40 to 127 (dB) | 16.2 | 16.6 | 16.7 | 16.6 | 20.0 | 0.3 |  |
| pitch wobble / vibrato (p-p) (c) | 102 | 106 | 89.9 | 99.0 | 111 | 0.1 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0332 (nearest 0.0287) | fonts among themselves 0.0323 | | | | 1.0 |  |

</details>

### 125 Telephone Ring: telephone

Distance 2.23 (rank 92 of 129); the fonts disagree 2.7 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Narrow loudness range (7.9 vs 16.6–20.1 dB)**: FAULT, mild. A real bell's loudness hardly depends on anything, but GM expects velocity to scale the effect.
- **Speaks late (onset 73 vs 0–34 ms; phrase 40 ms)**: DELIBERATE physics. The 20 Hz ring current must swing the armature (~18 Hz) into a gong (README "Sound effects").
- **Release 692 ms, floor −60 dB, stereo coherence 0.53**: the gongs ring on, plus the room. ARTIFACT.
- Pitch measures are meaningless here. The fonts disagree widely (2.8 JND).

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **loudness range, velocity 40 to 127**: a narrower dynamic range (7.9 dB | 16.7 | 16.6 | 16.7 | 20.1), outside the fonts, 5.0 x.
2. **onset delay**: speaks later (73.0 ms | 34.0 | 6 | 0 | 2), outside the fonts, 4.1 x.
3. **onset in a phrase (re the note-on)**: speaks later in a phrase (38.0 ms | 4 | -3 | 6 | -1), outside the fonts, 3.6 x.
4. **stereo coherence [room]**: wider (0.491 | 1 | 1 | 1 | 1), outside the fonts, 6.8 x.
5. **release time (30 dB after note-off) [room]**: rings longer after the key (746 ms | 155 | - | 39.0 | 153), outside the fonts, 2.3 x.
6. **noise / room floor 3.6-4 s after on, re peak [room]**: a higher floor (-59.2 dB | -122 | -123 | -120 | -101), outside the fonts, 5.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +8 | -6 | +2 | +6 | +4 | -9 |
| re TimGM6mb | -10 | -11 | -3 | -1 | -1 | +14 |
| re Jnsgm2 | -6 | -9 | +1 | +4 | +3 | +1 |
| re SGM | +1 | -4 | +3 | +0 | +2 | -3 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -7 -6 +0 +1; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.1 ERB, attack +0.0 ERB, velocity brightening +1.9 ERB, loudness range +3 dB, envelope 1.7 dB rms; TimGM6mb: centroid +0.1 ERB, attack -0.9 ERB, velocity brightening +0.4 ERB, loudness range +4 dB, envelope 4.8 dB rms; Jnsgm2: centroid -0.6 ERB, attack +0.1 ERB, velocity brightening +0.4 ERB, loudness range +3 dB, envelope 1.9 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -2.61 | -0.84 | -2.61 | -0.65 | -0.41 | 1.7 |  |
| attack time (-20 to -1 dB) (ms) | 376 | 302 | 176 | 27.0 | 29.0 | 1.4 | yes |
| spectral shape of the attack, note for note | 12.6 (nearest 10.6) | fonts among themselves 17.3 | | | | 0.7 |  |
| brightness over the keyboard (ERB/oct) | 2.77 | 0.27 | 5.88 | 4.21 | 1.64 | 0.1 |  |
| key-to-key brightness scatter (ERB) | 1.21 | 0.101 | 0.226 | 1.12 | 2.18 | 0.5 |  |
| brightening from velocity 40 to 127 (ERB) | 0.17 | -0 | 0 | -0 | 1.34 | 0.3 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 22.7 | 30.8 | 23.2 | 24.8 | 26.6 | 0.8 | yes |
| onset delay (ms) | 73.0 | 34.0 | 6 | 0 | 2 | 4.1 | yes |
| Plomp-Levelt dissonance of a note (log2) | -0.16 | -1.38 | -2.13 | 0.04 | -1.13 | 1.0 |  |
| envelope curve, note for note (dB rms) | 22.5 (nearest 19.3) | fonts among themselves 27.5 | | | | 0.8 |  |
| pitch re 12-TET A440 (median) (c) | -1.2 | -16.2 | 43.2 | -41.3 | -22.5 | 0.4 |  |
| pitch scatter from key to key (c) | 13.3 | 34.1 | 6.1 | 0.4 | 43.0 | 0.3 |  |
| noisiness (spectral flatness) (dB) | -19.1 | -14.3 | -40.0 | -37.7 | -21.7 | 0.8 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -59.2 | -122 | -123 | -120 | -101 | 5.6 | yes |
| pitch at the start re settled (c) | -2.2 | 0.6 | 1.6 | -32.3 | 0.2 | 0.2 |  |
| stereo coherence | 0.491 | 1 | 1 | 1 | 1 | 6.8 | yes |
| partial-to-partial jaggedness (dB) | 7.7 | 4.2 | 9.95 | 6.2 | 5.8 | 0.6 |  |
| dip between legato notes (dB) | 5.2 | 7.7 | 1 | 3.6 | 2.1 | 0.7 |  |
| level evenness of a legato scale (dB) | 0.9 | 0.4 | 0.3 | 0.8 | 1 | 0.3 |  |
| level over the keyboard (dB/oct) | -1.24 | 0 | -3.02 | 0.06 | -0.08 | 0.8 |  |
| level against the other instruments (velocity 96) (dB) | -1.2 | -0.1 | -0.2 | 0.2 | -3.6 | 0.6 |  |
| key-to-key level scatter (dB) | 0.91 | 0.02 | 0.61 | 0.09 | 0.36 | 1.4 | yes |
| odd / even partials (dB) | -2.1 | -2.9 | -6.9 | -12.6 | -9.1 | 1.2 |  |
| onset jitter in a phrase (ms) | 28.8 | 17.7 | 0.9 | 35.5 | 0.9 | 1.1 |  |
| onset in a phrase (re the note-on) (ms) | 38.0 | 4 | -3 | 6 | -1 | 3.6 | yes |
| onset sharpness (dB/ms) | 26.4 | 5.4 | 6.8 | 3 | 48.5 | 0.9 |  |
| overtone levels, note for note (dB rms) | 15.8 (nearest 14.4) | fonts among themselves 15.5 | | | | 1.0 |  |
| panning across the keys (dB/oct) | 0.14 | 0 | -0 | 0 | -0.01 | 0.1 |  |
| partial envelopes alike | 0.035 | 0.756 | 0.998 | 0.224 | 0.096 | 1.0 | yes |
| release time (30 dB after note-off) (ms) | 746 | 155 | - | 39.0 | 153 | 2.3 | yes |
| spectrogram (1 - correlation), note for note | 0.577 (nearest 0.434) | fonts among themselves 0.81 | | | | 0.7 |  |
| spectral shape of the body, note for note | 13.6 (nearest 11.8) | fonts among themselves 16.5 | | | | 0.8 |  |
| sounding length of a 100 ms note (ms) | 400 | 210 | 103 | 119 | 191 | 2.7 | yes |
| partial stretch (inharmonicity) (c/partial) | 0.29 | -1.01 | 0.29 | 0.38 | 0.72 | 0.1 |  |
| level after 2 s held, re peak (dB) | -5.5 | -25.9 | -123 | -3 | -3 | 0.1 |  |
| high bands lag low bands by (ms) | 3.8 | 22.5 | 55.9 | -2.5 | -2 | 0.2 |  |
| transient coherence: attack spread over bands (ms) | 11.8 | 41.0 | 23.6 | 8.4 | 7.2 | 0.2 |  |
| fundamental share (tristimulus 1) | 0 | 0.002 | 0 | 0.002 | 0.002 | 0.0 |  |
| partials 2-4 share (tristimulus 2) | 0.349 | 0.053 | 0.002 | 0.215 | 0.045 | 2.8 | yes |
| partials 5+ share (tristimulus 3) | 0.651 | 0.943 | 0.998 | 0.783 | 0.955 | 2.8 | yes |
| loudness range, velocity 40 to 127 (dB) | 7.9 | 16.7 | 16.6 | 16.7 | 20.1 | 5.0 | yes |
| pitch wobble / vibrato (p-p) (c) | 34.4 | 78.1 | 81.0 | 44.6 | 67.9 | 2.1 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0349 (nearest 0.0274) | fonts among themselves 0.0288 | | | | 1.2 |  |

</details>

### 126 Helicopter: helicopter

Distance 3.19 (rank 42 of 129); the fonts disagree 2.2 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Far too loud (+5.9 vs −24.4..−21.0 dB against the other instruments)**: FAULT, the largest balance error in the set. All four fonts agree within 3 dB that the helicopter sits ~22 dB under the median instrument; here it is set to the common loudness, so in a GM mix it is ~28 dB too hot.
- **Release 2000 ms**: DELIBERATE: the rotor spins down (README "Sound effects").
- **Narrow loudness range (9.8 vs 16.7–20.1 dB), purer (flatness −28.8 vs −11.9..−18.7 dB)**: the blade slap is periodic. Plausible; minor.
- **Darker with velocity (−0.66 ERB)**: minor.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **level against the other instruments (velocity 96)**: louder (-10.1 dB | -21.8 | -24.4 | -22.1 | -21.0), outside the fonts, 7.0 x.
2. **loudness range, velocity 40 to 127**: a narrower dynamic range (9.8 dB | 16.7 | 16.7 | 16.9 | 20.1), outside the fonts, 4.1 x.
3. **release time (30 dB after note-off) [room]**: rings longer after the key (2000 ms | 639 | 386 | 391 | 326), outside the fonts, 4.9 x.
4. **brightening from velocity 40 to 127**: brightens less with velocity (-0.68 ERB | -0 | 0 | 0 | -0.13), outside the fonts, 2.3 x.
5. **key-to-key brightness scatter**: less even in tone from key to key (0.631 ERB | 0.068 | 0.01 | 0.123 | 0.226), outside the fonts, 2.7 x.
6. **noisiness (spectral flatness)**: purer (-28.8 dB | -16.4 | -11.9 | -16.1 | -18.7), outside the fonts, 3.7 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +16 | +10 | +3 | -20 | -3 | +7 |
| re TimGM6mb | +35 | +24 | +6 | -19 | -3 | -10 |
| re Jnsgm2 | +20 | +9 | +2 | -12 | +2 | -4 |
| re SGM | +10 | +6 | +3 | -15 | +1 | +4 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -8 -6 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.3 ERB, attack +0.1 ERB, velocity brightening +0.4 ERB, loudness range +3 dB, envelope 6.3 dB rms; TimGM6mb: centroid -0.4 ERB, attack -1.1 ERB, velocity brightening +0.6 ERB, loudness range +3 dB, envelope 6.5 dB rms; Jnsgm2: centroid -1.7 ERB, attack -0.1 ERB, velocity brightening -0.1 ERB, loudness range +3 dB, envelope 8.8 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 1.75 | -1.28 | 0.52 | 0.4 | 0.26 | 1.6 | yes |
| attack time (-20 to -1 dB) (ms) | 1196 | 1050 | 1171 | 1149 | 1139 | 0.3 |  |
| spectral shape of the attack, note for note | 11.7 (nearest 11.4) | fonts among themselves 9.72 | | | | 1.2 |  |
| brightness over the keyboard (ERB/oct) | -0.23 | 0.09 | 0.5 | 0.41 | 1.55 | 0.9 | yes |
| key-to-key brightness scatter (ERB) | 0.631 | 0.068 | 0.01 | 0.123 | 0.226 | 2.7 | yes |
| brightening from velocity 40 to 127 (ERB) | -0.68 | -0 | 0 | 0 | -0.13 | 2.3 | yes |
| brightness (spectral centroid, velocity 96) (ERB) | 5.91 | 10.4 | 17.2 | 9.69 | 7.5 | 0.9 | yes |
| onset delay (ms) | 462 | 658 | 777 | 605 | 831 | 2.3 | yes |
| Plomp-Levelt dissonance of a note (log2) | 1.18 | 1.08 | 2.59 | 2.37 | 1.92 | 1.3 |  |
| envelope curve, note for note (dB rms) | 26.2 (nearest 17.9) | fonts among themselves 10.4 | | | | 2.5 | yes |
| pitch re 12-TET A440 (median) (c) | -1.6 | 11.9 | 34.3 | -5.1 | -0.7 | 0.4 |  |
| pitch scatter from key to key (c) | 51.3 | 36.9 | 38.5 | 42.8 | 52.6 | 1.4 |  |
| noisiness (spectral flatness) (dB) | -28.8 | -16.4 | -11.9 | -16.1 | -18.7 | 3.7 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -19.0 | -69.8 | -114 | -115 | -107 | 4.0 | yes |
| pitch at the start re settled (c) | -2.5 | -9.1 | -37.5 | 0 | 17.2 | 0.1 |  |
| stereo coherence | 0.724 | 0.065 | 1 | 0.973 | 1 | 0.6 |  |
| partial-to-partial jaggedness (dB) | 2.65 | 1.9 | 1.5 | 1.4 | 1.2 | 1.2 | yes |
| dip between legato notes (dB) | 13.5 | 9.7 | 9.7 | 16.0 | 10.4 | 1.1 |  |
| level evenness of a legato scale (dB) | 9.4 | 3.2 | 1.4 | 1.1 | 5.2 | 3.5 | yes |
| level over the keyboard (dB/oct) | 0.49 | 0.82 | 0.03 | -0.07 | -0.51 | 0.5 |  |
| level against the other instruments (velocity 96) (dB) | -10.1 | -21.8 | -24.4 | -22.1 | -21.0 | 7.0 | yes |
| key-to-key level scatter (dB) | 2.45 | 0.45 | 0.16 | 0.37 | 2.65 | 1.6 |  |
| odd / even partials (dB) | -2 | -0.4 | -0.6 | -1.8 | -2.8 | 0.4 |  |
| onset jitter in a phrase (ms) | 39.3 | 39.8 | 50.7 | 22.2 | 53.1 | 0.4 |  |
| onset in a phrase (re the note-on) (ms) | 51.0 | 48.0 | 1 | 4 | 6 | 2.0 |  |
| onset sharpness (dB/ms) | 11.1 | 8.2 | 10.7 | 10.0 | 11.3 | 0.4 |  |
| overtone levels, note for note (dB rms) | 11.9 (nearest 9.89) | fonts among themselves 8.2 | | | | 1.5 |  |
| panning across the keys (dB/oct) | 0.24 | 0.03 | 0 | 0 | -0.01 | 0.2 |  |
| partial envelopes alike | 0.309 | 0.644 | 0.851 | 0.402 | 0.852 | 1.9 | yes |
| release time (30 dB after note-off) (ms) | 2000 | 639 | 386 | 391 | 326 | 4.9 | yes |
| spectrogram (1 - correlation), note for note | 0.531 (nearest 0.422) | fonts among themselves 0.377 | | | | 1.4 |  |
| spectral shape of the body, note for note | 13 (nearest 11.1) | fonts among themselves 9.96 | | | | 1.3 |  |
| sounding length of a 100 ms note (ms) | 87.0 | 143 | 119 | 78.0 | 162 | 1.1 |  |
| partial stretch (inharmonicity) (c/partial) | -0.77 | -0.77 | 1.45 | -1.31 | 0.28 | 0.4 |  |
| level after 2 s held, re peak (dB) | -9.4 | -14.0 | -14.6 | -17.5 | -18.0 | 2.2 | yes |
| high bands lag low bands by (ms) | -13.5 | 18.5 | 2.5 | -20.8 | -0.7 | 0.7 |  |
| transient coherence: attack spread over bands (ms) | 5.9 | 42.3 | 22.2 | 11.4 | 7.6 | 0.6 |  |
| fundamental share (tristimulus 1) | 0.532 | 0.431 | 0.009 | 0.609 | 0.754 | 0.0 |  |
| partials 2-4 share (tristimulus 2) | 0.256 | 0.381 | 0.307 | 0.303 | 0.196 | 0.5 |  |
| partials 5+ share (tristimulus 3) | 0.135 | 0.175 | 0.373 | 0.058 | 0.059 | 0.1 |  |
| loudness range, velocity 40 to 127 (dB) | 9.8 | 16.7 | 16.7 | 16.9 | 20.1 | 4.1 | yes |
| pitch wobble / vibrato (p-p) (c) | 104 | 108 | 92.5 | 106 | 103 | 0.0 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0398 (nearest 0.0373) | fonts among themselves 0.0507 | | | | 0.8 |  |

</details>

### 127 Applause: applause

Distance 2.69 (rank 67 of 129); the fonts disagree 1.7 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **Starts at once (onset 32 vs 412–498 ms; attack 435 vs 693–980 ms)**: DELIBERATE, but unlike GM. Every font fades the applause in over ~0.5 s; here the crowd claps from the key. Debatable: GM files expect the swell.
- **Darker (16.0 vs 18.9–23.2 ERB; −6..−15 dB at 2.5–6 kHz)**: the palm-cavity resonance (Peltola et al., README "Sound effects"). Plausible, perhaps slightly dull.
- **Louder (−0.4 vs −18.3..−5.8 dB)**: trim. The fonts span 12 dB.
- **Stereo coherence 0.35**: room ARTIFACT, and arguably right for a crowd.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **onset delay**: speaks sooner (40.0 ms | 412 | 431 | 456 | 498), outside the fonts, 9.4 x.
2. **brightness (spectral centroid, velocity 96)**: darker (15.9 ERB | 21.7 | 21.8 | 23.2 | 18.9), outside the fonts, 2.7 x.
3. **attack time (-20 to -1 dB)**: a faster attack (432 ms | 693 | 731 | 980 | 788), outside the fonts, 3.3 x.
4. **level against the other instruments (velocity 96)**: louder (-2.2 dB | -5.8 | -18.3 | -16.2 | -12.8), outside the fonts, 2.0 x.
5. **partials 5+ share (tristimulus 3)**: weaker high partials (0.326 | 0.939 | 0.919 | 0.808 | 0.701), outside the fonts, 4.5 x.
6. **stereo coherence [room]**: wider (0.355 | 1 | 0.923 | 1 | 0.964), outside the fonts, 8.4 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | +18 | +16 | +10 | -1 | -15 | -8 |
| re TimGM6mb | +12 | +5 | +0 | -9 | -16 | +13 |
| re Jnsgm2 | +10 | +7 | +9 | +7 | -11 | -11 |
| re SGM | +17 | +5 | +6 | +4 | -7 | -11 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -12 -6 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -4 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -1.1 ERB, attack -0.3 ERB, velocity brightening +1.1 ERB, loudness range +3 dB, envelope 1.7 dB rms; TimGM6mb: centroid -0.4 ERB, attack -0.6 ERB, velocity brightening +0.9 ERB, loudness range +3 dB, envelope 1.8 dB rms; Jnsgm2: centroid -1.7 ERB, attack -0.3 ERB, velocity brightening +0.4 ERB, loudness range -5 dB, envelope 1.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.32 | 0.73 | 0.36 | -0.81 | -0.05 | 0.2 |  |
| attack time (-20 to -1 dB) (ms) | 432 | 693 | 731 | 980 | 788 | 3.3 | yes |
| spectral shape of the attack, note for note | 11.2 (nearest 9.71) | fonts among themselves 12 | | | | 0.9 |  |
| brightness over the keyboard (ERB/oct) | -0.05 | 0.48 | 0.52 | 1.4 | 3.03 | 0.8 | yes |
| key-to-key brightness scatter (ERB) | 0.296 | 0.091 | 0.057 | 1.26 | 0.829 | 0.3 |  |
| brightening from velocity 40 to 127 (ERB) | -0.11 | 0.01 | 0 | 0.93 | 0.52 | 0.8 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 15.9 | 21.7 | 21.8 | 23.2 | 18.9 | 2.7 | yes |
| onset delay (ms) | 40.0 | 412 | 431 | 456 | 498 | 9.4 | yes |
| Plomp-Levelt dissonance of a note (log2) | 1.97 | 2.48 | 2.88 | 2.57 | 2.53 | 1.9 | yes |
| envelope curve, note for note (dB rms) | 14.3 (nearest 6.88) | fonts among themselves 17.4 | | | | 0.8 |  |
| pitch re 12-TET A440 (median) (c) | -19.6 | -10.3 | -4.2 | 14.8 | -10.5 | 1.0 | yes |
| pitch scatter from key to key (c) | 30.3 | 53.2 | 45.5 | 66.3 | 29.9 | 1.0 |  |
| noisiness (spectral flatness) (dB) | -18.1 | -7.1 | -27.4 | -5.9 | -8.2 | 1.0 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -36.2 | -41.2 | -115 | -23.4 | -102 | 0.8 |  |
| pitch at the start re settled (c) | -1.7 | 11.1 | -8.4 | -27.8 | 15.2 | 0.1 |  |
| stereo coherence | 0.355 | 1 | 0.923 | 1 | 0.964 | 8.4 | yes |
| partial-to-partial jaggedness (dB) | 1.7 | 1 | 1.4 | 1.1 | 1.1 | 0.6 |  |
| dip between legato notes (dB) | 9.4 | 5.4 | 8.7 | 7.8 | 9.2 | 0.6 |  |
| level evenness of a legato scale (dB) | 1.1 | 1.5 | 1.3 | 4.4 | 3.2 | 0.8 |  |
| level over the keyboard (dB/oct) | 0.5 | -0.06 | -0.26 | 2.06 | -0.02 | 0.5 |  |
| level against the other instruments (velocity 96) (dB) | -2.2 | -5.8 | -18.3 | -16.2 | -12.8 | 2.0 | yes |
| key-to-key level scatter (dB) | 0.6 | 0.08 | 0.25 | 2.55 | 1.6 | 0.3 |  |
| odd / even partials (dB) | -1.4 | -0.5 | 0.2 | -0.8 | -0.5 | 0.4 |  |
| onset jitter in a phrase (ms) | 21.9 | 40.3 | 35.4 | 49.8 | 52.0 | 2.8 | yes |
| onset in a phrase (re the note-on) (ms) | 6 | 34.0 | 8 | 52.0 | 28.0 | 1.1 |  |
| onset sharpness (dB/ms) | 13.4 | 5.5 | 9 | 6.5 | 7.6 | 3.2 | yes |
| overtone levels, note for note (dB rms) | 7.88 (nearest 6.27) | fonts among themselves 5.91 | | | | 1.3 |  |
| panning across the keys (dB/oct) | -0.01 | 0 | 0.5 | 0 | 0.08 | 0.1 |  |
| partial envelopes alike | 0.192 | 0.629 | 0.622 | 0.802 | 0.678 | 4.6 | yes |
| release time (30 dB after note-off) (ms) | 1735 | 1222 | 440 | 2000 | 476 | 1.1 |  |
| spectrogram (1 - correlation), note for note | 0.513 (nearest 0.251) | fonts among themselves 0.329 | | | | 1.6 |  |
| spectral shape of the body, note for note | 11.5 (nearest 9.26) | fonts among themselves 11.4 | | | | 1.0 |  |
| sounding length of a 100 ms note (ms) | 196 | 400 | 87.0 | 400 | 155 | 0.3 |  |
| partial stretch (inharmonicity) (c/partial) | -0.66 | -2.56 | -1.33 | -0.76 | -1.55 | 0.9 |  |
| level after 2 s held, re peak (dB) | -12.6 | -6 | -12.3 | -6.3 | -12.9 | 1.0 |  |
| high bands lag low bands by (ms) | 1.4 | -4 | 11.3 | -4.9 | -14.5 | 0.5 |  |
| transient coherence: attack spread over bands (ms) | 11.8 | 16.1 | 24.9 | 18.1 | 22.3 | 1.7 | yes |
| fundamental share (tristimulus 1) | 0.009 | 0.001 | 0.001 | 0.012 | 0.007 | 0.1 |  |
| partials 2-4 share (tristimulus 2) | 0.215 | 0.06 | 0.08 | 0.179 | 0.283 | 0.8 |  |
| partials 5+ share (tristimulus 3) | 0.326 | 0.939 | 0.919 | 0.808 | 0.701 | 4.5 | yes |
| loudness range, velocity 40 to 127 (dB) | 13.9 | 16.6 | 16.7 | 7.5 | 20.1 | 0.4 |  |
| pitch wobble / vibrato (p-p) (c) | 102 | 110 | 112 | 109 | 116 | 1.7 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0285 (nearest 0.0238) | fonts among themselves 0.0299 | | | | 1.0 |  |

</details>

### 128 Gunshot: gunshot

Distance 1.52 (rank 125 of 129); the fonts disagree 2.0 JND. Keys 36,40,44,48,52,56,60,64,68,72,76,80,84, velocities 40/64/96/127.

- **A crack, not a boom (a 100 ms note sounds 19 vs 92–189 ms; onset 64 vs 4.5–16 dB/ms)**: DELIBERATE physics: an N-wave and a Friedlander blast last milliseconds (README "Sound effects"). The fonts' gunshots carry a recorded tail and echo. In a GM mix this sounds thin and small; consider a longer ground-reflection or report tail.
- **Louder (+5.6 vs −6.2..+1.7 dB)**: trim. The fonts disagree widely.
- **Pitch +26 c, smoother, less wobble**: MEASUREMENT ARTIFACT. A gunshot has no pitch.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **sounding length of a 100 ms note [room]**: shorter staccato (19.0 ms | 185 | 110 | 189 | 92.0), outside the fonts, 5.6 x.
2. **onset sharpness**: a sharper onset (63.1 dB/ms | 16.2 | 7.7 | 4.5 | 9.6), outside the fonts, 9.3 x.
3. **pitch re 12-TET A440 (median)**: sharper (26.4 c | -24.5 | -25.4 | 7.7 | -12.3), outside the fonts, 2.7 x.
4. **Plomp-Levelt dissonance of a note**: smoother (0.59 log2 | 2.26 | 1.46 | 1.96 | 1.99), outside the fonts, 3.5 x.
5. **pitch wobble / vibrato (p-p)**: wobbles less (91.7 c | 116 | 112 | 107 | 104), outside the fonts, 3.1 x.
6. **loudness range, velocity 40 to 127**: a narrower dynamic range (14.6 dB | 16.6 | 16.6 | 16.7 | 20.1), outside the fonts, 1.2 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -1 | -8 | -10 | -9 | -7 | +25 |
| re TimGM6mb | -19 | -22 | -15 | -12 | +16 | +27 |
| re Jnsgm2 | +18 | +3 | +3 | -6 | -8 | +1 |
| re SGM | +9 | +5 | +0 | -3 | -4 | -1 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +2; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid -0.2 ERB, attack +0.3 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 1.9 dB rms; TimGM6mb: centroid -0.3 ERB, attack +0.4 ERB, velocity brightening -0.3 ERB, loudness range +4 dB, envelope 5.3 dB rms; Jnsgm2: centroid -0.3 ERB, attack -0.1 ERB, velocity brightening +0.5 ERB, loudness range +3 dB, envelope 4.5 dB rms

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | -2.23 | -3.03 | 0.72 | -2.18 | -3.65 | 0.2 |  |
| attack time (-20 to -1 dB) (ms) | 4 | 1 | 28.0 | 37.0 | 9 | 0.8 |  |
| spectral shape of the attack, note for note | 13.7 (nearest 4.76) | fonts among themselves 13.9 | | | | 1.0 |  |
| brightness over the keyboard (ERB/oct) | 1.52 | 1.06 | 1.26 | 1.74 | 2.86 | 0.0 |  |
| key-to-key brightness scatter (ERB) | 0.272 | 0.019 | 0.38 | 0.299 | 0.628 | 0.2 |  |
| brightening from velocity 40 to 127 (ERB) | 0 | -0 | -0.04 | 0.03 | 0.39 | 0.1 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 9.12 | 11.9 | 5.79 | 19.5 | 15.4 | 0.7 |  |
| decay rate of a held note (dB/s) | 32.2 | 27.9 | 16.2 | 25.6 | 98.6 | 0.2 |  |
| onset delay (ms) | 13.0 | 24.0 | 17.0 | 27.0 | 15.0 | 0.8 |  |
| Plomp-Levelt dissonance of a note (log2) | 0.59 | 2.26 | 1.46 | 1.96 | 1.99 | 3.5 | yes |
| envelope curve, note for note (dB rms) | 20.4 (nearest 11.6) | fonts among themselves 19.5 | | | | 1.0 |  |
| pitch re 12-TET A440 (median) (c) | 26.4 | -24.5 | -25.4 | 7.7 | -12.3 | 2.7 | yes |
| pitch scatter from key to key (c) | 38.5 | 36.6 | 37.7 | 40.2 | 39.6 | 0.1 |  |
| noisiness (spectral flatness) (dB) | -15.4 | -24.6 | -61.6 | -10.3 | -8.6 | 0.1 |  |
| noise / room floor 3.6-4 s after on, re peak (dB) | -138 | -126 | -124 | -122 | -110 | 2.0 | yes |
| pitch at the start re settled (c) | 0 | 0 | -11.6 | 0 | 8 | 0.0 |  |
| stereo coherence | 0.631 | 1 | 0.316 | 0.823 | 0.553 | 0.2 |  |
| partial-to-partial jaggedness (dB) | 2.1 | 1.3 | 4.6 | 1.1 | 1.1 | 0.5 |  |
| dip between legato notes (dB) | 10.2 | 13.1 | 10.9 | 6.8 | 12.5 | 0.5 |  |
| level evenness of a legato scale (dB) | 0.1 | 0.1 | 0.6 | 0.9 | 1.5 | 0.7 |  |
| level over the keyboard (dB/oct) | -1.44 | -0.59 | -0.93 | -0.68 | -1.78 | 0.6 |  |
| level against the other instruments (velocity 96) (dB) | 1.5 | 1.7 | -3.8 | -1.4 | -6.2 | 1.0 |  |
| key-to-key level scatter (dB) | 0.31 | 0.03 | 0.21 | 0.05 | 0.33 | 0.4 |  |
| odd / even partials (dB) | -0.3 | -3 | -2.8 | -0.6 | -0.4 | 0.7 |  |
| onset jitter in a phrase (ms) | 0.9 | 7 | 48.9 | 45.7 | 5.3 | 1.2 | yes |
| onset in a phrase (re the note-on) (ms) | 8 | 22.0 | 26.0 | 36.0 | 5 | 1.0 |  |
| onset sharpness (dB/ms) | 63.1 | 16.2 | 7.7 | 4.5 | 9.6 | 9.3 | yes |
| overtone levels, note for note (dB rms) | 5.04 (nearest 4.06) | fonts among themselves 13.5 | | | | 0.4 |  |
| panning across the keys (dB/oct) | -0.02 | -0 | 0.06 | 0.04 | -0.01 | 0.0 |  |
| partial envelopes alike | 0.961 | 0.956 | 0.803 | 0.75 | 0.999 | 0.7 |  |
| spectrogram (1 - correlation), note for note | 0.25 (nearest 0.175) | fonts among themselves 0.615 | | | | 0.4 |  |
| spectral shape of the body, note for note | 11.7 (nearest 6) | fonts among themselves 13.9 | | | | 0.8 |  |
| sounding length of a 100 ms note (ms) | 19.0 | 185 | 110 | 189 | 92.0 | 5.6 | yes |
| partial stretch (inharmonicity) (c/partial) | -0.13 | -1.72 | -6.22 | -0.57 | 0.32 | 0.3 |  |
| level after 2 s held, re peak (dB) | -86.9 | -126 | -48.2 | -112 | -110 | 0.6 |  |
| high bands lag low bands by (ms) | -2.5 | -2.7 | 23.0 | -12.0 | -1 | 0.0 |  |
| transient coherence: attack spread over bands (ms) | 1.3 | 2.2 | 14.7 | 9 | 1.6 | 0.7 |  |
| fundamental share (tristimulus 1) | 0.34 | 0.089 | 0.484 | 0.076 | 0.07 | 1.2 |  |
| partials 2-4 share (tristimulus 2) | 0.348 | 0.38 | 0.366 | 0.142 | 0.426 | 0.2 |  |
| partials 5+ share (tristimulus 3) | 0.287 | 0.222 | 0.018 | 0.807 | 0.511 | 0.2 |  |
| loudness range, velocity 40 to 127 (dB) | 14.6 | 16.6 | 16.6 | 16.7 | 20.1 | 1.2 | yes |
| pitch wobble / vibrato (p-p) (c) | 91.7 | 116 | 112 | 107 | 104 | 3.1 | yes |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0131 (nearest 0.00731) | fonts among themselves 0.0176 | | | | 0.7 |  |

</details>

###  Drum kit (channel 10): drum kit

Distance 3.07 (rank 44 of 129); the fonts disagree 1.3 JND. Keys 35-81, velocities 40/64/96/127.

The kit-level numbers mix very different pieces. The per-key table in this section is the useful
view. In it, the level is in dB against each synth's kit median and the brightness is the
centroid in ERB at velocity 96.

- **The splash cymbal (55) is far too dark.** 12.6 ERB, about 0.9 kHz, against 30–33 ERB (5–7 kHz)
  in all four fonts. FAULT, already known: research/cymbals_built.md §"The dome pile-up" finds
  that the splash's energy collects at 800–1600 Hz. The `PJ_CYM_DOME` workaround does not cure it.
  The crash cymbals are also darker (49: 22 against 27–32 ERB; 57: 26 against 28–34) and 3–9 dB
  quieter in the kit. The china (52) and the ride (51, 59) sit inside the fonts.
- **The guiro (73/74) is 7–9 dB louder in the kit and much darker** (8.5–8.8 against 19.5–30 ERB).
  research/scrapers_friction.md built it from silence. FAULT: the scrape lacks its high rasp. The
  vibraslap (58) is also darker (15 against 22–31).
- **Some quiet pieces:**
  - woodblocks 76/77 sit −12/−15 dB against −0 … −8 (FAULT, level trim);
  - side stick 37: −12 against −3 … −5;
  - hand clap 39: −10 against −5 … +2, and darker at 12.4 against 15–18 ERB
    (research/clap_claves_surdo.md);
  - cowbell 56: −6 against −1 … −2;
  - low conga 64: −4 against +4 … +6.
  These are level trims, and the most audible kit-balance faults in a groove.
- **The snares are darker:** 38 is 7.3 ERB against 8.8–15.9, and 40 is 7.0 against 8.6–18.6. The
  fonts disagree widely here, and research/stick_snare.md tunes against literature rather than
  samples. Probably DELIBERATE, worth an ear.
- **Kick and snare decay at about 32 dB/s against 54–193** in the fonts, and "sustain" ends 45 dB
  higher. MEASUREMENT ARTIFACT of the room: the decay is fitted over 2 s, which the room tail
  dominates.
- The kick (35/36), the toms (41–50) and the hi-hats (42/44/46) are within the fonts' spread in
  level and brightness. TimGM6mb and Jnsgm2 give no usable body for several short keys (marked
  "-"), so the cloud there has only two members.

Largest differences, by weighted score (pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM):

1. **level against the other instruments (velocity 96)**: louder (-3.2 dB | -7.1 | -8.8 | -8.6 | -6.8), outside the fonts, 4.6 x.
2. **level after 2 s held, re peak**: sustains more (-76.2 dB | -123 | -121 | -122 | -108), outside the fonts, 5.9 x.
3. **brightness (spectral centroid, velocity 96)**: darker (12.4 ERB | 16.0 | 17.0 | 15.9 | 18.3), outside the fonts, 3.4 x.
4. **decay rate of a held note**: decays slower (32.0 dB/s | 70.0 | 85.2 | 67.7 | 53.7), outside the fonts, 3.3 x.
5. **key-to-key brightness scatter**: more even in tone from key to key (8.93 ERB | 9.73 | 9.9 | 10.1 | 10.3), outside the fonts, 3.6 x.
6. **attack time (-20 to -1 dB)**: a slower attack (6 ms | 2 | 4 | 3 | 4), outside the fonts, 1.6 x.

Mean spectrum of pajupilli re each font, dB, level removed (velocity 96):

| | <150 Hz | 150-400 | 400-1k | 1-2.5k | 2.5-6k | >6k |
|---|---|---|---|---|---|---|
| re FluidR3 | -2 | +4 | +1 | -1 | -2 | +0 |
| re TimGM6mb | -2 | +2 | -0 | -1 | -3 | +3 |
| re Jnsgm2 | -3 | +2 | +2 | -2 | +1 | -1 |
| re SGM | -1 | +3 | +1 | +1 | -2 | -1 |

- velocity curve, dB re velocity 96 at 40/64/96/127: pajupilli -13 -6 +0 +4; FluidR3 -13 -6 +0 +4; TimGM6mb -13 -6 +0 +4; Jnsgm2 -13 -6 +0 +4; SGM -15 -7 +0 +5
- TiMidity misreads (FluidSynth re TiMidity, same font): FluidR3: centroid +0.0 ERB, attack +0.0 ERB, velocity brightening +0.3 ERB, loudness range +4 dB, envelope 3.7 dB rms; TimGM6mb: centroid +0.2 ERB, attack -0.2 ERB, velocity brightening -0.2 ERB, loudness range +4 dB, envelope 9.2 dB rms; Jnsgm2: centroid +0.6 ERB, attack -0.2 ERB, velocity brightening +0.1 ERB, loudness range +4 dB, envelope 4.5 dB rms

<details><summary>per drum key: level (dB re the kit median) and centroid (ERB), velocity 96</summary>

| key | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM |
|---|---|---|---|---|---|
| 35 | +1 / 3.6 | +2 / 3.0 | - | - | +6 / 2.9 |
| 36 | +2 / 4.1 | +2 / 2.9 | - | +9 / 3.1 | +7 / 2.9 |
| 37 | -8 / 11.6 | -3 / 21.8 | -4 / 8.7 | -5 / 11.8 | -4 / 18.3 |
| 38 | +2 / 7.4 | +3 / 12.3 | +5 / 15.9 | +6 / 8.8 | +3 / 13.9 |
| 39 | -4 / 13.7 | +2 / 16.9 | -1 / 17.4 | -2 / 15.3 | -5 / 18.3 |
| 40 | +0 / 7.1 | +0 / 11.3 | +3 / 18.3 | +3 / 18.6 | +5 / 8.6 |
| 41 | +8 / 3.0 | +7 / 2.0 | +6 / 3.0 | +10 / 3.0 | +9 / 2.3 |
| 42 | -8 / 25.7 | -6 / 29.8 | - | - | -8 / 25.3 |
| 43 | +4 / 3.8 | +8 / 3.0 | +6 / 3.0 | +10 / 3.0 | +9 / 3.2 |
| 44 | -8 / 31.3 | -6 / 23.2 | - | -6 / 5.3 | -10 / 16.8 |
| 45 | +5 / 4.0 | +4 / 3.0 | +6 / 3.4 | +7 / 3.4 | +14 / 4.0 |
| 46 | -2 / 21.9 | -3 / 25.5 | -9 / 22.7 | -0 / 30.3 | +0 / 33.4 |
| 47 | +4 / 5.1 | +4 / 3.0 | +5 / 4.0 | +7 / 4.0 | +10 / 4.1 |
| 48 | +3 / 5.8 | +3 / 3.1 | +5 / 4.4 | +8 / 4.2 | +1 / 5.3 |
| 49 | +4 / 20.8 | +4 / 29.8 | +2 / 26.8 | +6 / 32.5 | +6 / 30.7 |
| 50 | +4 / 7.8 | +2 / 4.0 | +4 / 5.0 | +7 / 5.1 | +2 / 6.7 |
| 51 | -8 / 30.6 | -5 / 27.1 | -8 / 33.0 | -4 / 25.9 | -8 / 19.9 |
| 52 | +0 / 21.1 | -0 / 27.4 | +1 / 20.6 | +3 / 22.1 | +0 / 24.6 |
| 53 | -8 / 20.1 | -5 / 23.8 | -10 / 20.7 | -2 / 24.0 | -7 / 23.7 |
| 54 | -6 / 30.1 | +1 / 34.5 | -1 / 31.8 | -1 / 34.9 | -1 / 34.8 |
| 55 | +1 / 12.7 | -2 / 30.5 | -0 / 29.9 | +5 / 31.7 | -1 / 32.7 |
| 56 | -6 / 12.4 | -2 / 11.5 | -1 / 16.8 | -2 / 9.5 | -1 / 13.7 |
| 57 | +3 / 25.6 | +3 / 28.8 | +2 / 27.5 | +5 / 34.0 | +6 / 29.6 |
| 58 | +1 / 15.1 | +3 / 27.3 | -6 / 31.0 | -2 / 21.7 | +1 / 30.6 |
| 59 | -7 / 25.9 | -6 / 26.5 | -9 / 33.5 | -4 / 26.1 | -4 / 24.1 |
| 60 | -3 / 11.2 | -1 / 15.6 | -4 / 17.3 | -0 / 16.4 | -3 / 13.9 |
| 61 | -0 / 9.6 | +1 / 12.7 | +0 / 10.8 | +2 / 8.2 | +1 / 9.5 |
| 62 | -8 / 8.2 | -4 / 12.2 | -4 / 11.8 | -3 / 6.5 | -8 / 10.6 |
| 63 | +2 / 9.1 | +2 / 8.2 | +2 / 7.5 | -0 / 7.7 | +3 / 7.2 |
| 64 | +3 / 6.6 | +4 / 7.0 | +5 / 6.0 | +5 / 6.0 | +6 / 5.0 |
| 65 | +1 / 9.1 | +2 / 12.2 | +3 / 12.9 | +0 / 12.3 | +2 / 13.4 |
| 66 | +2 / 8.0 | +4 / 13.6 | +5 / 7.3 | +0 / 8.6 | +6 / 7.9 |
| 67 | -3 / 19.3 | -4 / 19.0 | -2 / 22.2 | -4 / 20.1 | -3 / 29.3 |
| 68 | -2 / 17.0 | -4 / 16.0 | -1 / 19.9 | -3 / 17.8 | -4 / 26.9 |
| 69 | -5 / 29.6 | -0 / 29.1 | -2 / 30.9 | -6 / 35.5 | -6 / 33.9 |
| 70 | -9 / 25.4 | -4 / 6.7 | -6 / 32.5 | - | -11 / 33.7 |
| 71 | -0 / 25.1 | +1 / 24.9 | - | - | +5 / 23.0 |
| 72 | +3 / 25.4 | +8 / 23.9 | - | +9 / 21.7 | +10 / 23.0 |
| 73 | +0 / 8.6 | -2 / 19.5 | -2 / 28.0 | - | -7 / 26.4 |
| 74 | +0 / 8.7 | -1 / 24.9 | -3 / 29.9 | -4 / 27.7 | -5 / 27.6 |
| 75 | -5 / 22.7 | -0 / 19.3 | - | -2 / 22.0 | -4 / 22.9 |
| 76 | -5 / 13.9 | -1 / 14.5 | -1 / 16.5 | -8 / 21.5 | -3 / 15.9 |
| 77 | -5 / 12.2 | -0 / 12.8 | +0 / 13.6 | -4 / 16.6 | -2 / 14.5 |
| 78 | +5 / 11.9 | +8 / 12.0 | +4 / 13.7 | +8 / 13.6 | +8 / 12.9 |
| 79 | +5 / 10.4 | +13 / 8.1 | +3 / 8.4 | +5 / 10.4 | +8 / 6.9 |
| 80 | -9 / 30.8 | -3 / 33.5 | - | - | -8 / 30.3 |
| 81 | -5 / 30.4 | -1 / 30.3 | - | - | -4 / 31.2 |

</details>

<details><summary>every measure</summary>

| measure | pajupilli | FluidR3 | TimGM6mb | Jnsgm2 | SGM | x | outside |
|---|---|---|---|---|---|---|---|
| attack brighter than the body by (ERB) | 0.41 | 1 | 0.69 | 0.8 | 0.67 | 0.8 | yes |
| attack time (-20 to -1 dB) (ms) | 6 | 2 | 4 | 3 | 4 | 1.6 | yes |
| spectral shape of the attack, note for note | 12.3 (nearest 11.7) | fonts among themselves 9.02 | | | | 1.4 | yes |
| key-to-key brightness scatter (ERB) | 8.93 | 9.73 | 9.9 | 10.1 | 10.3 | 3.6 | yes |
| brightening from velocity 40 to 127 (ERB) | -0.24 | -0.33 | -0 | -0 | 0.19 | 0.8 |  |
| brightness (spectral centroid, velocity 96) (ERB) | 12.4 | 16.0 | 17.0 | 15.9 | 18.3 | 3.4 | yes |
| decay rate of a held note (dB/s) | 32.0 | 70.0 | 85.2 | 67.7 | 53.7 | 3.3 | yes |
| onset delay (ms) | 10.0 | 3 | 1 | 2 | 3 | 0.8 | yes |
| Plomp-Levelt dissonance of a note (log2) | -0.2 | -0.32 | -0.56 | -0.24 | 0.41 | 0.2 |  |
| envelope curve, note for note (dB rms) | 19.9 (nearest 16.1) | fonts among themselves 15.6 | | | | 1.3 |  |
| noisiness (spectral flatness) (dB) | -31.1 | -18.4 | -15.4 | -24.6 | -18.1 | 2.8 | yes |
| noise / room floor 3.6-4 s after on, re peak (dB) | -112 | -124 | -121 | -122 | -108 | 1.2 |  |
| stereo coherence | 0.645 | 0.976 | 1 | 1 | 1 | 4.7 | yes |
| level against the other instruments (velocity 96) (dB) | -3.2 | -7.1 | -8.8 | -8.6 | -6.8 | 4.6 | yes |
| key-to-key level scatter (dB) | 4.42 | 4.14 | 4.6 | 5.11 | 6.19 | 0.4 |  |
| onset sharpness (dB/ms) | 50.8 | 9.9 | 4.1 | 3.7 | 46.9 | 2.0 | yes |
| release time (30 dB after note-off) (ms) | 1617 | 1135 | - | - | 1243 | 1.8 | yes |
| spectrogram (1 - correlation), note for note | 0.36 (nearest 0.327) | fonts among themselves 0.212 | | | | 1.7 | yes |
| spectral shape of the body, note for note | 12.4 (nearest 12.3) | fonts among themselves 10.3 | | | | 1.2 |  |
| level after 2 s held, re peak (dB) | -76.2 | -123 | -121 | -122 | -108 | 5.9 | yes |
| high bands lag low bands by (ms) | -0.7 | 0 | 0 | 0 | 0 | 0.1 |  |
| transient coherence: attack spread over bands (ms) | 1 | 0 | 0 | 0 | 0 | 0.2 |  |
| loudness range, velocity 40 to 127 (dB) | 17.5 | 16.6 | 16.6 | 16.6 | 20.1 | 0.5 |  |
| Zimtohrli per note (velocity 96, loudness matched) | 0.0104 (nearest 0.00905) | fonts among themselves 0.00501 | | | | 2.1 | yes |

</details>

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

