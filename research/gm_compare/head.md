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
