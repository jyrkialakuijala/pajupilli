# The General MIDI programs' levels against the SoundFonts

tools/gmcompare.py levels. Each melodic program plays five keys over the middle of its GM range at velocities 40, 64, 96, 127 (held 2 s), after a piano's C3, C4, C5 at velocity 96 in the same file. Measure: loudest 400 ms, K-weighted (BS.1770), each synth's note against its own piano, averaged over the keys. pajupilli less the mean of FluidR3, TimGM6mb, Jnsgm2, SGM, dB: positive = pajupilli's instrument is louder against its piano than the fonts'. sd: the spread of the four fonts (their standard deviation) at velocity 96. The dynamic range is v127 less v40.

## Summary

| | v40 | v64 | v96 | v127 |
|---|---|---|---|---|
| mean | +1.9 | +0.3 | -0.5 | -1.1 |
| mean abs | 2.2 | 0.7 | 0.9 | 1.7 |
| within 3 dB | 75 % | 96 % | 95 % | 83 % |

The fonts' own spread at v96: median sd 3.0 dB. Dynamic range v40-v127: pajupilli median 15.7 dB, fonts 17.5 dB.

## Piano

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Acoustic Grand Piano | grand-piano | +3.0 | +4.5 | +3.7 | +2.6 | +1.4 | 0.5 | 14.2 / 17.3 | 1.4 / -1.2 |
| 2 | Bright Acoustic Piano | bright-piano | +0.1 | +1.5 | +0.5 | -0.5 | -1.2 | 0.7 | 14.2 / 17.0 | -1.0 / -0.4 |
| 3 | Electric Grand Piano | electric-grand | -0.1 | -2.5 | -0.4 | +0.9 | +1.4 | 2.2 | 21.6 / 17.8 | 1.4 / 0.4 |
| 4 | Honky-tonk Piano | honky-tonk | +0.1 | +4.0 | +1.2 | -1.4 | -3.3 | 1.9 | 10.2 / 17.5 | 0.0 / 1.4 |
| 5 | Electric Piano 1 | electric-piano-1 | -0.0 | +1.9 | +0.1 | -0.3 | -1.8 | 2.0 | 13.9 / 17.6 | 3.2 / 3.6 |
| 6 | Electric Piano 2 | electric-piano-2 | +0.5 | +8.0 | +1.4 | -3.0 | -4.4 | 2.3 | 5.2 / 17.6 | -2.3 / 0.7 |
| 7 | Harpsichord | harpsichord | +0.5 | +11.7 | +3.2 | -5.2 | -7.6 | 1.4 | -1.8 / 17.5 | -4.5 / 0.7 |
| 8 | clavi | clavi | +0.1 | +3.4 | +0.2 | -0.7 | -2.6 | 2.6 | 11.5 / 17.5 | -0.8 / -0.1 |

## Chromatic percussion

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 9 | Celesta | celesta | +0.3 | +2.8 | +0.4 | -1.1 | -1.1 | 2.5 | 13.7 / 17.6 | -0.7 / 0.5 |
| 10 | Glockenspiel | glockenspiel | +0.2 | +3.1 | +0.7 | -1.1 | -2.0 | 1.7 | 12.8 / 17.9 | -0.2 / 0.9 |
| 11 | Music Box | music-box | +0.3 | +6.4 | +1.3 | -2.2 | -4.2 | 3.7 | 6.9 / 17.5 | -2.5 / -0.3 |
| 12 | Vibraphone | vibraphone | -0.2 | -0.9 | -0.2 | +0.7 | -0.4 | 4.1 | 18.2 / 17.7 | 2.2 / 1.5 |
| 13 | Marimba | marimba | +0.1 | +2.0 | +0.8 | -1.1 | -1.1 | 3.2 | 14.4 / 17.5 | -1.9 / -0.9 |
| 14 | Xylophone | xylophone | +0.2 | +3.0 | +1.0 | -1.3 | -1.9 | 3.5 | 12.7 / 17.7 | -7.7 / -6.4 |
| 15 | Tubular Bells | tubular-bells | +0.2 | +2.2 | +0.2 | -0.7 | -0.9 | 3.4 | 14.5 / 17.5 | 1.6 / 2.3 |
| 16 | Dulcimer | dulcimer | +0.2 | +2.4 | +0.2 | -0.7 | -1.2 | 3.3 | 14.0 / 17.6 | -4.2 / -3.5 |

## Organ

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 17 | Drawbar Organ | drawbar-organ | -2.4 | +6.9 | -0.6 | -6.6 | -9.5 | 2.7 | 1.5 / 17.9 | -1.8 / 4.7 |
| 18 | Percussive Organ | percussive-organ | -1.3 | +7.7 | +0.6 | -5.4 | -7.9 | 2.1 | 1.9 / 17.5 | -3.8 / 1.6 |
| 19 | Rock Organ | rock-organ | +5.7 | +14.8 | +7.3 | +1.5 | -1.0 | 3.5 | 1.7 / 17.5 | 3.1 / 1.6 |
| 20 | Church Organ | organ-plan | +0.2 | +10.0 | +2.6 | -3.5 | -8.3 | 2.0 | -0.8 / 17.5 | 0.2 / 3.7 |
| 21 | Reed Organ | reed-organ | +0.2 | +3.2 | -0.1 | -1.2 | -1.0 | 2.4 | 13.3 / 17.5 | 4.1 / 5.3 |
| 22 | Accordion | accordion | +0.2 | +3.7 | +0.1 | -1.2 | -1.7 | 1.9 | 12.2 / 17.5 | 2.8 / 4.0 |
| 23 | Harmonica | harmonica | +0.4 | +3.9 | -0.2 | -1.1 | -1.0 | 2.4 | 12.7 / 17.6 | 1.6 / 2.7 |
| 24 | Tango Accordion | bandoneon | +0.3 | +3.3 | -0.3 | -0.9 | -0.8 | 1.6 | 13.5 / 17.5 | 3.9 / 4.8 |

## Guitar

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 25 | Nylon Guitar | nylon-guitar | +0.1 | -1.0 | -2.3 | +1.6 | +2.0 | 3.3 | 20.6 / 17.5 | 2.9 / 1.2 |
| 26 | Steel Guitar | steel-guitar | +0.3 | +2.6 | +0.4 | -0.9 | -0.9 | 3.9 | 13.9 / 17.5 | -0.8 / 0.1 |
| 27 | Jazz Guitar | jazz-guitar | -0.3 | -0.8 | +0.6 | -0.0 | -1.0 | 3.2 | 18.2 / 18.4 | 3.8 / 3.9 |
| 28 | Clean Guitar | clean-guitar | -0.1 | +0.8 | +0.2 | -0.3 | -0.9 | 2.6 | 15.8 / 17.6 | 0.7 / 1.0 |
| 29 | Muted Guitar | muted-guitar | -0.3 | -0.8 | +1.4 | -0.4 | -1.5 | 5.0 | 16.9 / 17.5 | -4.0 / -3.6 |
| 30 | Overdriven Guitar | overdriven-guitar | -0.5 | -1.2 | +1.0 | +0.8 | -2.5 | 3.5 | 16.2 / 17.5 | 4.4 / 3.6 |
| 31 | Distortion Guitar | distortion-guitar | +0.2 | +5.5 | -0.4 | -0.1 | -4.4 | 5.6 | 7.7 / 17.5 | 6.8 / 6.8 |
| 32 | Guitar Harmonics | guitar-harmonics | -0.2 | -0.0 | +0.5 | -0.4 | -0.9 | 2.8 | 16.7 / 17.5 | 3.5 / 3.9 |

## Bass

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 33 | Acoustic Bass | double-bass | -0.2 | +2.3 | +1.0 | -0.4 | -3.7 | 2.5 | 11.5 / 17.5 | 1.9 / 2.4 |
| 34 | Finger Bass | electric-bass | -0.4 | +0.5 | +1.2 | -0.1 | -3.0 | 2.5 | 14.0 / 17.5 | 1.7 / 1.8 |
| 35 | Pick Bass | pick-bass | -0.1 | +0.9 | +0.1 | -0.1 | -1.2 | 4.0 | 15.4 / 17.5 | 0.5 / 0.6 |
| 36 | Fretless Bass | fretless-bass | -0.5 | +2.0 | +0.7 | -0.7 | -3.9 | 2.8 | 11.5 / 17.5 | 1.9 / 2.6 |
| 37 | Slap Bass 1 | slap-bass-1 | -0.3 | -0.1 | +1.7 | -0.5 | -2.1 | 5.7 | 15.5 / 17.5 | 0.2 / 0.7 |
| 38 | Slap Bass 2 | slap-bass-2 | +0.1 | +2.5 | +0.6 | -1.1 | -1.5 | 4.5 | 13.5 / 17.5 | -1.4 / -0.4 |
| 39 | Synth Bass 1 | synth-bass-1 | +0.0 | +3.4 | +1.1 | -1.3 | -3.1 | 3.2 | 11.0 / 17.5 | 1.4 / 2.7 |
| 40 | Synth Bass 2 | synth-bass-2 | -0.0 | +2.7 | +0.8 | -1.1 | -2.7 | 1.7 | 12.2 / 17.5 | 3.8 / 4.9 |

## Strings

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 41 | Violin | violin | -0.2 | -0.8 | +0.0 | -0.0 | +0.2 | 3.5 | 18.5 / 17.5 | 5.4 / 5.5 |
| 42 | Viola | viola | +0.1 | +1.0 | -0.2 | -0.2 | -0.4 | 3.1 | 16.2 / 17.6 | 4.5 / 4.7 |
| 43 | Cello | cello | +0.1 | -0.3 | -0.7 | +0.1 | +1.2 | 2.6 | 19.0 / 17.5 | 5.9 / 5.8 |
| 44 | Contrabass | contrabass | -0.1 | -0.0 | -0.6 | -0.0 | +0.2 | 3.9 | 17.8 / 17.5 | 3.7 / 3.8 |
| 45 | Tremolo Strings | tremolo-strings | +0.1 | +0.1 | +0.1 | +0.0 | -0.1 | 3.5 | 17.3 / 17.5 | 3.6 / 3.5 |
| 46 | Pizzicato Strings | pizzicato-strings | +0.0 | -0.5 | +0.2 | +0.2 | +0.1 | 3.4 | 18.1 / 17.5 | -0.8 / -1.0 |
| 47 | Orchestral Harp | harp | +0.2 | +3.3 | +0.0 | -0.9 | -1.5 | 3.7 | 12.7 / 17.5 | 1.6 / 2.5 |
| 48 | Timpani | timpani | +0.3 | -3.2 | -3.7 | +2.8 | +5.3 | 4.0 | 26.2 / 17.6 | 1.0 / -1.8 |

## Ensemble

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 49 | String Ensemble 1 | strings | -0.0 | -0.6 | +0.2 | +0.5 | -0.2 | 3.7 | 18.0 / 17.5 | 3.7 / 3.2 |
| 50 | String Ensemble 2 | slow-strings | +0.0 | +0.4 | -0.2 | +0.2 | -0.4 | 4.3 | 16.7 / 17.5 | 3.0 / 2.8 |
| 51 | Synth Strings 1 | synth-strings-1 | -0.0 | +0.0 | -0.4 | +0.1 | +0.3 | 2.2 | 17.8 / 17.5 | 3.0 / 3.0 |
| 52 | Synth Strings 2 | synth-strings-2 | -0.1 | +0.2 | -0.1 | -0.1 | -0.4 | 2.2 | 16.8 / 17.4 | 4.9 / 5.0 |
| 53 | Choir Aahs | choir | +0.5 | +1.5 | -2.1 | -0.7 | +3.4 | 2.5 | 19.5 / 17.5 | 4.0 / 4.7 |
| 54 | Voice Oohs | voice-oohs | +0.5 | +2.9 | -0.8 | -1.2 | +1.2 | 2.4 | 15.7 / 17.5 | 1.5 / 2.6 |
| 55 | Synth Voice | synth-voice | -0.0 | -2.8 | -1.5 | +1.2 | +2.9 | 2.9 | 23.2 / 17.5 | 4.6 / 3.4 |
| 56 | Orchestra Hit | orchestra-hit | +0.3 | +1.9 | +0.3 | -0.7 | -0.4 | 2.0 | 15.3 / 17.5 | 2.0 / 2.7 |

## Brass

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 57 | Trumpet | trumpet | +0.5 | +3.7 | -0.2 | -1.8 | +0.4 | 3.7 | 14.2 / 17.6 | -1.6 / 0.2 |
| 58 | Trombone | trombone | +0.1 | +1.5 | +0.4 | -0.8 | -0.7 | 2.2 | 15.2 / 17.4 | 2.5 / 3.2 |
| 59 | Tuba | tuba | +0.0 | -2.5 | -1.3 | +1.2 | +2.7 | 2.5 | 22.7 / 17.5 | 5.8 / 4.6 |
| 60 | Muted Trumpet | muted-trumpet | +0.1 | +4.5 | +1.4 | -2.1 | -3.3 | 2.2 | 10.7 / 18.5 | -2.0 / 0.1 |
| 61 | French Horn | horn | +0.2 | +5.5 | +0.6 | -1.3 | -4.2 | 2.5 | 7.8 / 17.5 | 5.8 / 7.2 |
| 62 | Brass Section | brass-section | +7.9 | +14.2 | +7.4 | +5.3 | +4.9 | 2.0 | 8.2 / 17.6 | 7.0 / 1.8 |
| 63 | Synth Brass 1 | synth-brass-1 | -0.0 | +0.9 | +0.6 | -0.6 | -1.1 | 3.2 | 15.5 / 17.5 | 2.1 / 2.6 |
| 64 | Synth Brass 2 | synth-brass-2 | -0.0 | +0.5 | +0.1 | +0.0 | -0.7 | 3.0 | 16.3 / 17.5 | 3.0 / 3.0 |

## Reed

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 65 | Soprano Sax | soprano-sax | +0.1 | +0.8 | -0.1 | +0.0 | -0.4 | 2.3 | 16.4 / 17.6 | 5.0 / 4.9 |
| 66 | Alto Sax | alto-sax | +0.1 | -0.5 | -0.2 | +0.4 | +0.6 | 3.3 | 18.7 / 17.5 | 5.0 / 4.6 |
| 67 | Tenor Sax | tenor-sax | +0.1 | +0.4 | -0.0 | -0.1 | +0.1 | 4.0 | 17.2 / 17.5 | 3.1 / 3.2 |
| 68 | Baritone Sax | baritone-sax | +0.1 | -1.1 | -0.5 | +0.4 | +1.8 | 2.3 | 20.5 / 17.5 | 3.5 / 3.1 |
| 69 | Oboe | oboe | +0.1 | -0.5 | -0.3 | +0.1 | +1.0 | 2.3 | 19.0 / 17.5 | 6.5 / 6.4 |
| 70 | English Horn | english-horn | -0.0 | -1.8 | -0.8 | +0.8 | +1.6 | 2.9 | 20.9 / 17.5 | 6.9 / 6.0 |
| 71 | Bassoon | bassoon | +0.0 | +0.9 | +0.0 | -0.1 | -0.6 | 2.9 | 16.0 / 17.5 | 3.7 / 3.9 |
| 72 | Clarinet | clarinet | +0.0 | +1.9 | -0.3 | -0.2 | -1.3 | 3.9 | 14.3 / 17.5 | 5.3 / 5.6 |

## Pipe

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 73 | Piccolo | piccolo | +0.1 | +1.2 | -0.0 | -0.6 | -0.2 | 3.0 | 16.1 / 17.5 | 5.0 / 5.6 |
| 74 | Flute | flute | -0.1 | +0.3 | +0.2 | -0.4 | -0.4 | 2.2 | 16.8 / 17.5 | 6.6 / 7.0 |
| 75 | Recorder | recorder | +0.0 | -0.1 | -0.8 | +0.1 | +1.0 | 1.6 | 18.6 / 17.4 | 7.3 / 7.2 |
| 76 | Pan Flute | panflute | -0.1 | +0.2 | -0.2 | -0.3 | -0.2 | 2.0 | 17.1 / 17.5 | 4.8 / 5.1 |
| 77 | Blown Bottle | bottle | +0.0 | +0.2 | -0.2 | -0.4 | +0.4 | 2.7 | 17.7 / 17.5 | 3.2 / 3.6 |
| 78 | Shakuhachi | shakuhachi | -0.0 | +0.2 | +0.1 | -0.5 | +0.2 | 1.9 | 17.5 / 17.5 | 4.8 / 5.3 |
| 79 | Whistle | whistle | +0.2 | +2.7 | +0.2 | -1.4 | -0.8 | 1.6 | 14.1 / 17.5 | 8.4 / 9.8 |
| 80 | Ocarina | ocarina | +0.2 | +1.6 | +0.0 | -1.1 | +0.5 | 3.2 | 16.4 / 17.5 | 7.7 / 8.8 |

## Synth lead

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 81 | Lead 1 (square) | lead-square | -0.0 | +1.1 | +0.2 | -0.4 | -1.0 | 2.7 | 15.5 / 17.5 | 6.4 / 6.8 |
| 82 | Lead 2 (sawtooth) | lead-saw | -0.0 | +0.8 | -0.0 | -0.3 | -0.7 | 2.9 | 16.1 / 17.5 | 3.8 / 4.1 |
| 83 | Lead 3 (calliope) | lead-calliope | +0.0 | +0.9 | +0.1 | -0.6 | -0.4 | 4.0 | 16.5 / 17.9 | 4.0 / 4.5 |
| 84 | Lead 4 (chiff) | lead-chiff | -0.1 | +0.8 | +0.3 | -0.5 | -1.0 | 3.7 | 16.2 / 18.0 | 3.5 / 4.0 |
| 85 | Lead 5 (charang) | lead-charang | +0.0 | +0.0 | -0.2 | -0.0 | +0.3 | 4.0 | 17.8 / 17.6 | 3.0 / 3.0 |
| 86 | Lead 6 (voice) | lead-voice | -0.1 | +1.6 | +0.8 | -0.9 | -1.7 | 2.9 | 14.3 / 17.5 | 4.8 / 5.7 |
| 87 | Lead 7 (fifths) | lead-fifths | +0.0 | +0.6 | +0.0 | -0.1 | -0.4 | 3.3 | 16.5 / 17.5 | 3.7 / 3.8 |
| 88 | Lead 8 (bass + lead) | lead-bass+lead | -0.0 | +0.9 | +0.1 | -0.4 | -0.8 | 2.8 | 15.8 / 17.5 | 6.1 / 6.5 |

## Synth pad

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 89 | Pad 1 (new age) | pad-new-age | +0.0 | +0.9 | -0.2 | -0.2 | -0.4 | 3.4 | 16.2 / 17.5 | 1.1 / 1.3 |
| 90 | Pad 2 (warm) | pad-warm | -0.0 | +0.2 | -0.5 | +0.1 | +0.0 | 3.0 | 17.3 / 17.5 | 4.2 / 4.1 |
| 91 | Pad 3 (polysynth) | pad-polysynth | -0.0 | +0.3 | -0.5 | +0.1 | -0.1 | 2.9 | 17.2 / 17.5 | 4.8 / 4.7 |
| 92 | Pad 4 (choir) | pad-choir | +0.0 | +0.6 | +0.3 | -0.6 | -0.2 | 3.0 | 16.7 / 17.5 | 2.9 / 3.4 |
| 93 | Pad 5 (bowed) | pad-bowed | -0.0 | +0.5 | +0.1 | -0.2 | -0.4 | 3.0 | 16.7 / 17.5 | 1.1 / 1.3 |
| 94 | Pad 6 (metallic) | pad-metallic | -0.4 | -0.3 | -0.2 | +1.4 | -2.6 | 3.9 | 15.3 / 17.5 | 4.2 / 2.8 |
| 95 | Pad 7 (halo) | pad-halo | +0.1 | +3.5 | +0.8 | -1.2 | -2.8 | 3.5 | 11.2 / 17.5 | 2.3 / 3.5 |
| 96 | Pad 8 (sweep) | pad-sweep | +0.2 | +1.9 | -0.2 | -0.8 | -0.1 | 4.6 | 15.7 / 17.7 | 1.2 / 2.0 |

## Synth effects

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 97 | FX 1 (rain) | fx-rain | +0.1 | +0.1 | +1.1 | -1.8 | +1.2 | 3.4 | 18.2 / 17.1 | -0.5 / 1.4 |
| 98 | FX 2 (soundtrack) | fx-soundtrack | -0.1 | +1.1 | -0.3 | +0.6 | -1.9 | 2.7 | 14.6 / 17.6 | 4.2 / 3.7 |
| 99 | FX 3 (crystal) | fx-crystal | +0.3 | +0.7 | +0.1 | -0.9 | +1.0 | 4.8 | 17.9 / 17.6 | 0.2 / 1.0 |
| 100 | FX 4 (atmosphere) | fx-atmosphere | +0.0 | +1.4 | +0.3 | -0.5 | -1.0 | 4.0 | 15.3 / 17.7 | 2.3 / 2.8 |
| 101 | FX 5 (brightness) | fx-brightness | -0.0 | +0.7 | +0.6 | -0.5 | -1.0 | 3.2 | 15.8 / 17.5 | 2.6 / 3.0 |
| 102 | FX 6 (goblins) | fx-goblins | +0.1 | +4.0 | +0.8 | -1.5 | -2.8 | 3.0 | 10.5 / 17.3 | 3.4 / 4.8 |
| 103 | FX 7 (echoes) | fx-echoes | -0.0 | +1.7 | +0.5 | -0.9 | -1.4 | 2.9 | 14.8 / 17.9 | 3.7 / 4.5 |
| 104 | FX 8 (sci-fi) | fx-sci-fi | +0.1 | +6.1 | +1.0 | -1.6 | -4.9 | 3.3 | 6.9 / 17.9 | 1.6 / 3.3 |

## Ethnic

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 105 | Sitar | sitar | -0.2 | -0.9 | +0.7 | +0.5 | -1.1 | 2.5 | 17.4 / 17.6 | 2.4 / 1.9 |
| 106 | Banjo | banjo | -0.1 | +0.1 | +0.8 | -0.3 | -1.2 | 3.6 | 16.3 / 17.5 | 0.6 / 0.9 |
| 107 | Shamisen | shamisen | +0.0 | +0.7 | +1.0 | -0.4 | -1.3 | 2.8 | 16.3 / 18.2 | -3.7 / -3.4 |
| 108 | Koto | koto | -0.0 | +0.5 | +0.3 | -0.2 | -0.6 | 3.6 | 16.4 / 17.6 | 0.7 / 1.0 |
| 109 | Kalimba | kalimba | +0.0 | +0.8 | +0.3 | -0.1 | -0.8 | 4.4 | 15.9 / 17.5 | -2.8 / -2.7 |
| 110 | Bagpipe | bagpipe | +0.1 | +1.5 | +0.6 | -0.7 | -1.2 | 3.3 | 16.3 / 19.1 | 2.5 / 3.1 |
| 111 | Fiddle | fiddle | +0.0 | +1.7 | +0.3 | -0.6 | -1.4 | 3.7 | 14.4 / 17.5 | 4.8 / 5.3 |
| 112 | Shanai | shehnai | +0.1 | +1.7 | +0.1 | -0.7 | -0.6 | 2.4 | 17.4 / 19.7 | 4.1 / 4.9 |

## Percussive

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 113 | Tinkle Bell | tinkle-bell | -0.1 | +0.8 | +0.1 | +0.0 | -1.6 | 2.6 | 15.1 / 17.5 | 0.1 / 0.0 |
| 114 | Agogo | agogo | -0.1 | +0.5 | +0.1 | -0.1 | -0.7 | 3.0 | 16.3 / 17.5 | -4.5 / -4.3 |
| 115 | Steel Drums | steel-drums | +0.1 | +0.5 | -0.0 | -0.4 | +0.2 | 2.4 | 17.3 / 17.5 | 0.6 / 1.1 |
| 116 | Woodblock | woodblock | +0.1 | +0.7 | +0.4 | -0.2 | -0.6 | 2.9 | 16.2 / 17.5 | -6.3 / -6.0 |
| 117 | Taiko Drum | taiko | -0.1 | -0.1 | +0.0 | +0.4 | -0.5 | 2.3 | 17.1 / 17.5 | 1.5 / 1.1 |
| 118 | Melodic Tom | melodic-tom | +0.1 | +1.1 | -0.1 | -0.4 | -0.3 | 1.8 | 16.0 / 17.5 | -1.3 / -1.0 |
| 119 | Synth Drum | synth-drum | +0.0 | +1.7 | +0.4 | -0.7 | -1.4 | 3.3 | 14.4 / 17.5 | 2.4 / 3.0 |
| 120 | Reverse Cymbal | reverse-cymbal | +0.3 | +4.5 | -0.2 | -0.4 | -2.7 | 4.2 | 10.3 / 17.5 | -1.6 / -1.1 |

## Sound effects

| GM | program | pajupilli | mean | v40 | v64 | v96 | v127 | sd | range pj / fonts | re piano v96 pj / fonts |
|---|---|---|---|---|---|---|---|---|---|---|
| 121 | Guitar Fret Noise | fret-noise | +0.3 | +7.0 | +0.2 | -1.0 | -5.2 | 3.2 | 5.3 / 17.5 | -5.0 / -4.0 |
| 122 | Breath Noise | breath-noise | +0.2 | +8.2 | +2.3 | -2.9 | -6.7 | 8.3 | 2.7 / 17.6 | -13.0 / -10.2 |
| 123 | Seashore | seashore | +0.0 | +4.3 | +0.2 | -0.4 | -4.0 | 3.0 | 9.2 / 17.5 | 1.6 / 2.0 |
| 124 | Bird Tweet | bird-tweet | -0.2 | +0.1 | +0.6 | +0.3 | -1.9 | 2.6 | 15.6 / 17.5 | 3.1 / 2.8 |
| 125 | Telephone Ring | telephone | +0.1 | +5.5 | -0.8 | -0.3 | -3.9 | 2.7 | 8.1 / 17.5 | 3.3 / 3.6 |
| 126 | Helicopter | helicopter | +0.1 | +2.0 | +0.4 | +0.3 | -2.3 | 3.7 | 13.2 / 17.5 | -4.4 / -4.7 |
| 127 | Applause | applause | +0.0 | +1.2 | +0.3 | -0.4 | -1.0 | 3.0 | 15.1 / 17.3 | 0.3 / 0.7 |
| 128 | Gunshot | gunshot | -0.2 | +0.9 | +0.5 | +0.1 | -2.2 | 2.3 | 14.5 / 17.5 | 0.6 / 0.5 |

