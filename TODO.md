# Open questions

## Brushes for a file that names a brush set in its text?

Some files select the Standard kit by number (program 1 on channel 10) while their own text says
"GenMidiBRUSH SET". The snare is chosen like this (`src/midifile.cc`, `drumFor()` in
`src/drumkit.cc`):

- a file that selects a Brush kit (programs 41–48): brushes;
- a file with a piano and an upright bass: the brushed snare (decided 2026-11-16: "usual MIDI
  file gets the standard snare, if the midi file has a piano and upright bass then Jazz brushed
  snare");
- every other file: the stick snare.

So a file that names a brush set only in its text gets brushes if it has a piano and an upright
bass, and sticks otherwise. Should the text count too?

## What should "pedaling for the bass strings" mean on the harpsichord?

The harpsichord (`-inst harpsichord`, `research/harpsichord.md` §5.2) has the sustain pedal as a
register pedal for now: down, it adds the 4' and the 16'. The other reading, a pedal instrument
with its own strings for the bass (as Bach's circle had), is planned but not built. Which is
wanted on the pedal, and should the pedal instrument be played from the low keys, from a second
MIDI channel, or both?

## Which accordion voice for General MIDI 22?

The accordion (`-inst accordion`, `research/free_reeds.md` §10, §12) plays a French-style
musette: three 8' sets, the outer two 3.5 beats a second either side of the middle one, the
middle set in the cassotto. `PJ_ACC_BEAT` changes the beating (0: one set, dry). Should GM 22
be the musette, a drier two-set ("Italian") tuning, or a single set? Should the cassotto be on?
GM 24 is the bandoneón (8' + 4'), as the research recommends.
