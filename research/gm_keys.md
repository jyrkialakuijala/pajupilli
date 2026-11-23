# Each key's level against the SoundFonts

Median over the programs: shape rms 1.8 dB, scatter 1.8 dB.

tools/gmcompare.py keys. Every key of each program's GM range at velocities 64, 100 (held 1.2 s), after a piano's C3, C4, C5 at velocity 96 in the same file; loudest 400 ms, K-weighted, against the synth's own piano. **Shape**: pajupilli's level per key less the fonts' mean, each less its own mean over the keys (the instrument's level is research/gm_levels.md's): its rms over the keys, and the **scatter** left after a smooth trend (a quadratic) is taken off. **Fonts sd**: the four fonts' spread per key, its median.

| GM | program | pajupilli | shape rms (dB) | scatter (dB) | fonts sd (dB) | keys |
|---|---|---|---|---|---|---|
| 1 | Acoustic Grand Piano | grand-piano | 4.0 | 3.5 | 1.4 | 88 |
| 2 | Bright Acoustic Piano | bright-piano | 2.1 | 2.1 | 1.4 | 88 |
| 3 | Electric Grand Piano | electric-grand | 1.8 | 1.8 | 2.0 | 88 |
| 4 | Honky-tonk Piano | honky-tonk | 2.2 | 2.2 | 2.0 | 88 |
| 5 | Electric Piano 1 | electric-piano-1 | 2.7 | 2.4 | 2.7 | 88 |
| 6 | Electric Piano 2 | electric-piano-2 | 2.6 | 2.6 | 2.1 | 88 |
| 7 | Harpsichord | harpsichord | 5.1 | 4.7 | 1.7 | 88 |
| 8 | clavi | clavi | 1.6 | 1.5 | 2.7 | 88 |
| 9 | Celesta | celesta | 2.6 | 2.6 | 2.7 | 49 |
| 10 | Glockenspiel | glockenspiel | 2.1 | 1.8 | 2.1 | 49 |
| 11 | Music Box | music-box | 3.1 | 2.8 | 3.9 | 49 |
| 12 | Vibraphone | vibraphone | 2.5 | 2.5 | 3.9 | 49 |
| 13 | Marimba | marimba | 1.8 | 1.8 | 3.2 | 49 |
| 14 | Xylophone | xylophone | 1.5 | 1.2 | 3.6 | 49 |
| 15 | Tubular Bells | tubular-bells | 1.9 | 1.9 | 3.5 | 49 |
| 16 | Dulcimer | dulcimer | 2.7 | 2.6 | 3.5 | 49 |
| 17 | Drawbar Organ | drawbar-organ | 3.3 | 2.1 | 2.9 | 61 |
| 18 | Percussive Organ | percussive-organ | 2.4 | 1.8 | 1.8 | 61 |
| 19 | Rock Organ | rock-organ | 3.4 | 2.2 | 3.5 | 61 |
| 20 | Church Organ | organ-plan | 1.8 | 1.8 | 2.4 | 61 |
| 21 | Reed Organ | reed-organ | 1.5 | 1.5 | 2.3 | 61 |
| 22 | Accordion | accordion | 1.1 | 1.0 | 2.3 | 61 |
| 23 | Harmonica | harmonica | 2.8 | 2.8 | 2.5 | 61 |
| 24 | Tango Accordion | bandoneon | 0.9 | 0.8 | 1.4 | 61 |
| 25 | Nylon Guitar | nylon-guitar | 4.0 | 3.9 | 3.7 | 45 |
| 26 | Steel Guitar | steel-guitar | 1.7 | 1.6 | 3.9 | 45 |
| 27 | Jazz Guitar | jazz-guitar | 2.4 | 2.4 | 3.1 | 45 |
| 28 | Clean Guitar | clean-guitar | 1.5 | 1.5 | 2.6 | 45 |
| 29 | Muted Guitar | muted-guitar | 0.8 | 0.7 | 5.3 | 45 |
| 30 | Overdriven Guitar | overdriven-guitar | 1.7 | 1.7 | 3.2 | 45 |
| 31 | Distortion Guitar | distortion-guitar | 1.4 | 1.4 | 5.2 | 45 |
| 32 | Guitar Harmonics | guitar-harmonics | 2.0 | 2.0 | 3.0 | 45 |
| 33 | Acoustic Bass | double-bass | 2.6 | 2.6 | 2.1 | 33 |
| 34 | Finger Bass | electric-bass | 1.1 | 1.1 | 2.7 | 33 |
| 35 | Pick Bass | pick-bass | 1.3 | 1.3 | 3.8 | 33 |
| 36 | Fretless Bass | fretless-bass | 2.6 | 2.6 | 2.8 | 33 |
| 37 | Slap Bass 1 | slap-bass-1 | 1.2 | 1.2 | 5.3 | 33 |
| 38 | Slap Bass 2 | slap-bass-2 | 1.1 | 1.1 | 4.2 | 33 |
| 39 | Synth Bass 1 | synth-bass-1 | 0.8 | 0.8 | 3.1 | 33 |
| 40 | Synth Bass 2 | synth-bass-2 | 0.8 | 0.8 | 2.0 | 33 |
| 41 | Violin | violin | 1.3 | 1.3 | 3.5 | 42 |
| 42 | Viola | viola | 1.2 | 1.2 | 3.2 | 37 |
| 43 | Cello | cello | 2.2 | 2.2 | 2.4 | 41 |
| 44 | Contrabass | contrabass | 1.9 | 1.9 | 3.9 | 33 |
| 45 | Tremolo Strings | tremolo-strings | 1.8 | 1.5 | 4.6 | 61 |
| 46 | Pizzicato Strings | pizzicato-strings | 3.1 | 2.0 | 3.5 | 61 |
| 47 | Orchestral Harp | harp | 2.5 | 2.5 | 3.8 | 61 |
| 48 | Timpani | timpani | 0.7 | 0.7 | 3.6 | 61 |
| 49 | String Ensemble 1 | strings | 2.2 | 1.8 | 3.7 | 61 |
| 50 | String Ensemble 2 | slow-strings | 2.8 | 2.0 | 4.1 | 61 |
| 51 | Synth Strings 1 | synth-strings-1 | 1.6 | 1.6 | 2.2 | 61 |
| 52 | Synth Strings 2 | synth-strings-2 | 1.3 | 1.3 | 2.3 | 61 |
| 53 | Choir Aahs | choir | 1.1 | 0.8 | 2.6 | 32 |
| 54 | Voice Oohs | voice-oohs | 5.4 | 2.2 | 2.6 | 32 |
| 55 | Synth Voice | synth-voice | 1.0 | 1.0 | 3.0 | 32 |
| 56 | Orchestra Hit | orchestra-hit | 1.7 | 1.2 | 2.1 | 32 |
| 57 | Trumpet | trumpet | 3.0 | 2.5 | 3.6 | 45 |
| 58 | Trombone | trombone | 5.9 | 2.6 | 2.2 | 45 |
| 59 | Tuba | tuba | 2.0 | 1.9 | 2.3 | 45 |
| 60 | Muted Trumpet | muted-trumpet | 1.8 | 1.8 | 2.4 | 45 |
| 61 | French Horn | horn | 2.9 | 2.4 | 2.7 | 45 |
| 62 | Brass Section | brass-section | 7.9 | 1.9 | 1.8 | 45 |
| 63 | Synth Brass 1 | synth-brass-1 | 1.4 | 1.4 | 3.4 | 45 |
| 64 | Synth Brass 2 | synth-brass-2 | 1.9 | 1.9 | 3.1 | 45 |
| 65 | Soprano Sax | soprano-sax | 2.1 | 2.1 | 2.5 | 37 |
| 66 | Alto Sax | alto-sax | 2.9 | 2.9 | 3.6 | 37 |
| 67 | Tenor Sax | tenor-sax | 2.3 | 2.3 | 3.8 | 37 |
| 68 | Baritone Sax | baritone-sax | 2.4 | 2.4 | 2.2 | 37 |
| 69 | Oboe | oboe | 2.4 | 2.4 | 2.5 | 37 |
| 70 | English Horn | english-horn | 2.0 | 2.0 | 2.7 | 37 |
| 71 | Bassoon | bassoon | 1.2 | 1.2 | 3.2 | 37 |
| 72 | Clarinet | clarinet | 3.8 | 3.8 | 3.9 | 37 |
| 73 | Piccolo | piccolo | 2.4 | 2.4 | 3.1 | 37 |
| 74 | Flute | flute | 2.6 | 2.4 | 2.4 | 37 |
| 75 | Recorder | recorder | 3.4 | 3.4 | 2.1 | 37 |
| 76 | Pan Flute | panflute | 1.5 | 1.5 | 2.5 | 37 |
| 77 | Blown Bottle | bottle | 2.3 | 2.3 | 2.8 | 37 |
| 78 | Shakuhachi | shakuhachi | 5.9 | 5.9 | 1.7 | 37 |
| 79 | Whistle | whistle | 3.0 | 3.0 | 1.6 | 37 |
| 80 | Ocarina | ocarina | 3.2 | 3.0 | 3.2 | 37 |
| 81 | Lead 1 (square) | lead-square | 1.6 | 1.6 | 2.6 | 49 |
| 82 | Lead 2 (sawtooth) | lead-saw | 1.2 | 1.2 | 2.9 | 49 |
| 83 | Lead 3 (calliope) | lead-calliope | 2.3 | 2.3 | 4.3 | 49 |
| 84 | Lead 4 (chiff) | lead-chiff | 1.8 | 1.8 | 3.7 | 49 |
| 85 | Lead 5 (charang) | lead-charang | 1.5 | 1.5 | 3.7 | 49 |
| 86 | Lead 6 (voice) | lead-voice | 0.8 | 0.8 | 2.8 | 49 |
| 87 | Lead 7 (fifths) | lead-fifths | 0.9 | 0.9 | 3.2 | 49 |
| 88 | Lead 8 (bass + lead) | lead-bass+lead | 1.5 | 1.5 | 2.8 | 49 |
| 89 | Pad 1 (new age) | pad-new-age | 1.6 | 1.6 | 3.4 | 49 |
| 90 | Pad 2 (warm) | pad-warm | 1.7 | 1.7 | 3.0 | 49 |
| 91 | Pad 3 (polysynth) | pad-polysynth | 1.3 | 1.3 | 2.8 | 49 |
| 92 | Pad 4 (choir) | pad-choir | 1.3 | 1.2 | 2.9 | 49 |
| 93 | Pad 5 (bowed) | pad-bowed | 1.7 | 1.7 | 2.9 | 49 |
| 94 | Pad 6 (metallic) | pad-metallic | 1.5 | 1.5 | 4.1 | 49 |
| 95 | Pad 7 (halo) | pad-halo | 1.3 | 1.3 | 3.2 | 49 |
| 96 | Pad 8 (sweep) | pad-sweep | 2.3 | 2.3 | 4.0 | 49 |
| 97 | FX 1 (rain) | fx-rain | 2.8 | 2.8 | 3.6 | 49 |
| 98 | FX 2 (soundtrack) | fx-soundtrack | 1.7 | 1.7 | 2.6 | 49 |
| 99 | FX 3 (crystal) | fx-crystal | 1.3 | 1.3 | 4.8 | 49 |
| 100 | FX 4 (atmosphere) | fx-atmosphere | 2.1 | 2.1 | 4.0 | 49 |
| 101 | FX 5 (brightness) | fx-brightness | 1.2 | 1.2 | 3.2 | 49 |
| 102 | FX 6 (goblins) | fx-goblins | 2.1 | 2.1 | 2.8 | 49 |
| 103 | FX 7 (echoes) | fx-echoes | 2.7 | 2.7 | 3.0 | 49 |
| 104 | FX 8 (sci-fi) | fx-sci-fi | 1.5 | 1.3 | 3.3 | 49 |
| 105 | Sitar | sitar | 2.0 | 2.0 | 2.6 | 49 |
| 106 | Banjo | banjo | 1.7 | 1.6 | 3.7 | 49 |
| 107 | Shamisen | shamisen | 1.5 | 1.5 | 2.7 | 49 |
| 108 | Koto | koto | 1.4 | 1.4 | 3.8 | 49 |
| 109 | Kalimba | kalimba | 1.9 | 1.9 | 5.0 | 49 |
| 110 | Bagpipe | bagpipe | 4.0 | 4.0 | 3.6 | 49 |
| 111 | Fiddle | fiddle | 1.8 | 1.8 | 3.4 | 49 |
| 112 | Shanai | shehnai | 7.6 | 7.6 | 2.6 | 49 |
| 113 | Tinkle Bell | tinkle-bell | 1.4 | 1.4 | 2.6 | 49 |
| 114 | Agogo | agogo | 1.1 | 0.9 | 3.2 | 49 |
| 115 | Steel Drums | steel-drums | 1.1 | 1.1 | 2.9 | 33 |
| 116 | Woodblock | woodblock | 0.8 | 0.8 | 2.9 | 49 |
| 117 | Taiko Drum | taiko | 2.2 | 2.1 | 2.3 | 49 |
| 118 | Melodic Tom | melodic-tom | 0.4 | 0.4 | 1.8 | 49 |
| 119 | Synth Drum | synth-drum | 0.3 | 0.3 | 3.4 | 49 |
| 120 | Reverse Cymbal | reverse-cymbal | 3.9 | 1.6 | 8.2 | 49 |
| 121 | Guitar Fret Noise | fret-noise | 1.1 | 0.6 | 3.2 | 49 |
| 122 | Breath Noise | breath-noise | 1.6 | 1.0 | 8.7 | 49 |
| 123 | Seashore | seashore | 2.5 | 1.6 | 5.1 | 49 |
| 124 | Bird Tweet | bird-tweet | 0.7 | 0.7 | 2.6 | 49 |
| 125 | Telephone Ring | telephone | 0.6 | 0.6 | 3.0 | 49 |
| 126 | Helicopter | helicopter | 0.5 | 0.5 | 3.2 | 49 |
| 127 | Applause | applause | 1.3 | 1.3 | 3.3 | 49 |
| 128 | Gunshot | gunshot | 0.1 | 0.1 | 2.5 | 49 |
