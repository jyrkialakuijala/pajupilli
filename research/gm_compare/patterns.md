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
