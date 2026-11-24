<p align="center"><img src="pajupilli.svg" alt="Pajupilli, General MIDI instrument" width="480"></p>

# pajupilli

pajupilli is a real-time General MIDI instrument built on physical models. Instead of playing
samples, it simulates how each instrument makes sound: air in a bore, a hammer on a string, a bow,
a reed, the room around the players. Play it from a MIDI keyboard, or render a MIDI file to a WAV.

## Build

You need Linux, a C++17 compiler and the ALSA headers.

    sudo apt install g++ make libasound2-dev
    make

`make` tunes the program for your CPU and is fastest with AVX-512. `make scalar` builds a portable,
slower `pajupilli-scalar`.

## Quick start

Play a MIDI file through your speakers:

    ./pajupilli -play song.mid

Render it to a file:

    ./pajupilli -play song.mid -o song.wav

Play from a MIDI keyboard:

    ./pajupilli -live -inst grand-piano

No keyboard at hand? Try a short arpeggio, or write one note to a file:

    ./pajupilli -selftest
    ./pajupilli -inst grand-piano -seq "on 60 80, wait 2, off 60, wait 0.5" -o c4.wav

## Instruments

Choose one with `-inst NAME`. With `-play`, each MIDI channel uses its General MIDI program, and a
program that has no model of its own is played by the nearest one that does.

| Family | Examples |
|---|---|
| Piano and keys | `grand-piano`, `bright-piano`, `electric-grand`, `electric-piano-1`, `harpsichord`, `clavi` |
| Organ | `drawbar-organ`, `rock-organ`, `organ-plan`, `accordion`, `harmonica` |
| Tuned percussion | `celesta`, `vibraphone`, `marimba`, `xylophone`, `tubular-bells` |
| Guitar and bass | `nylon-guitar`, `steel-guitar`, `distortion-guitar`, `double-bass`, `fretless-bass` |
| Strings and voices | `violin`, `viola`, `cello`, `harp`, `strings`, `choir` |
| Brass | `trumpet`, `trombone`, `tuba`, `horn`, `brass-section` |
| Woodwinds | `flute`, `piccolo`, `clarinet`, `oboe`, `bassoon`, `alto-sax`, `shakuhachi` |
| Synths and effects | `lead-saw`, `pad-warm`, `fx-rain`, `seashore`, `applause` |
| Drums | `drums` |

To see the full list:

    ./pajupilli -inst nosuch

## Common options

| Option | What it does |
|---|---|
| `-o FILE.wav` | Writes the sound to a file (48 kHz) instead of playing it |
| `-volume DB` | Raises or lowers the whole mix of a MIDI file, for example `-volume -6` |
| `-start_time T` | Starts a MIDI file at `T`, in seconds or `m:ss` |
| `-transpose N` | Shifts notes by `N` semitones |
| `-room NAME` | Plays in `dry`, `studio`, `hall` (default), `concert` or `church` |
| `-reinstrument A=B` | Plays one instrument's part on another, for example `grand-piano=organ` |
| `-pianist` | Plans each piano part's sustain pedal from its notes |
| `-hammer_pos X` | Sets where the grand's hammers strike, as a share of the string (default 0.125) |
| `-tuning SPEC` | Sets the tuning, for example `"grand-piano;tet12,organ;meantone"` (keep the quotes) |
| `-midi NAME` | Connects to a named MIDI controller |
| `-dev PCM` | Uses another ALSA audio device |

`./pajupilli -help` lists every option.

## Tests

    make test              # all checks, about 25 minutes
    make test T=piano J=4  # one group, four jobs at a time
    python3 tools/test.py -l

The analysis tools and tests need Python 3 with NumPy and SciPy. Real-time checks can fail when the
machine is busy.

## More documentation

- [MANUAL.md](MANUAL.md): the full manual, with every instrument, control and modelling decision.
  Older notes that point to a "README" section mean this file.
- [REQUIREMENTS.md](REQUIREMENTS.md): what the model is meant to achieve.
- [Organ.md](Organ.md), [Viola.md](Viola.md), [Trombone.md](Trombone.md),
  [EnglishHorn.md](EnglishHorn.md): notes on individual instruments.
- [Director.md](Director.md): how a MIDI file gets a musical reading.
- [research/](research/): the physics, measurements and sources.
