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
