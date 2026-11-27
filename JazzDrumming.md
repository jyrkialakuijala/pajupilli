# Jazz drumming: breaks, endings, and what the drummer does here

Notes for the drummer of a played jazz file (`-play`), collected 2026-11-26 from the author's
sources on brush playing and on how a jazz piece ends. What is built says so; the rest is the
plan. The code is in `src/midifile.cc` (the swirls, the ending) and `src/drumkit.h`.

## 1. What is built

- **The brushes' cymbal swirl ends before the final note.** A swirl planned into the ending is
  cut two bars before the piece's final note; one left under a second long is not played. The
  final note is the last chord: three or more pitched notes begun together in the last three
  seconds (a pickup into it and a late grace note after it are not it). Without such a chord it
  is the first note begun within a second of the very last.
- **The snare's own brush circles stop two bars before the final note.** The brushed snare's left
  hand circles of its own accord from the first brush stroke on and used to stop only when the
  song was over, so a woosh ran under and past the last chord. From two bars before the final
  note the brushes are off the head: no circling, and a swirl the file holds (key 40) is ended
  too. Strokes still sound. This holds for any band and any kit, not only with the jazz drummer.
- **The ending is marked.** With brushes (a jazz band's file) the drummer strikes the snare, the
  bass drum and a crash together on the final note, unless the file's own drums already strike
  there. `PJ_SWIRL_END=0` gives the earlier behaviour.

- **A proper ending for the file's own drums.** A triplet on the snare sets up the last beat,
  the beat before the final note is left open, and none of the file's drum strokes are played
  after the final note (with brushes each was a sweep under the last chord).

- **The drummer listens.** A cymbal swirl is no louder than 0.7 of the velocity the pitched parts
  play around it (`PJ_SWIRL_UNDER`), and the ending's strokes are set against the band's last
  notes (the bass drum a little over them, the crash under them).

Not built: everything below.

## 2. Brush breaks

Brushes have no rebound, so a break lives on contrast and sideways motion, not on volume.

- **Wash against silence.** The groove is a continuous sideways sweep. Lifting a brush takes the
  friction bed away, and sparse taps, rim shots and bass drum accents stand out against it.
- **Sideways fills.** Fast single strokes are played by turning the forearm (a door knob) or
  wiping the tips back and forth, not by vertical wrist strokes.
- **Slaps and dig-ins.** The wire fan pressed flat is a choked slap; let rebound, a ringing
  tone. Alternating the two phrases like a horn.
- **Handle and rim.** The shaft on the rim, or the wires flopping onto the head after a rim
  hit, is the sharp punctuation of a loud break.

The players' vocabularies:

| Player | Way |
|---|---|
| Philly Joe Jones | rhythmic scrapes, sharp off-beat snare and bass drum, triplets, rim clicks |
| Papa Jo Jones | the left hand circling throughout, the right in light triplets over snare, hi-hat and ride, back into the sweep on beat 1 |
| Ed Thigpen | toms and snare as pitched voices, the hi-hat foot on 2 and 4 |
| Jeff Hamilton | the spread wires dragged under light pressure: a buzzing swell let go into a cymbal |

Trading fours:

- The feet keep the form: a quiet hi-hat on 2 and 4, or feathered quarters on the bass drum.
- Space is an accent: two beats with no sweep at all hold more tension than a flurry.
- The way out: the sweeping hand lands a little ahead of beat 1 (the "and" of 4), so the swish
  is already moving when the chorus starts.

## 3. Endings

An ending is agreed as it happens. The drummer signals it and matches how the last note dies.

| Ending | What happens | The drummer |
|---|---|---|
| The tag | the last four bars (iii-VI-ii-V) three times, then beat 1 | buoyant twice; the third time a setup fill in bar 3 or 4 |
| The Ellington ending | the rhythm section stops in the last bar, the piano plays a free flourish, then one chord | silent through the flourish; snare, bass drum and crash on the chord |
| The trash can | a long held last chord | a free swelling roll over cymbals, snare and toms, cut off with the leader |
| Vamp and fade | a two- or four-bar figure repeated | ride to hi-hat, sticks to brushes, down to nothing |

The last four bars:

1. Bars 1-2: steady time (an unbroken sweep with brushes).
2. Bar 3: opening up: a tom note, a snare accent on the "and" of 2 or 4, a cymbal swell.
3. Bar 4: no routine time. A setup figure on beats 1-3 (a triplet fill, two dotted quarters);
   beat 4 open, or a pickup on "4-and".

The last bar, by how the melody ends:

- **On beat 1:** snare, bass drum and a warm crash or the ride's bell with the bass's root. Let
  it ring if the band holds; choke it if the band cuts off.
- **On the "and" of 4 (the push):** caught with snare and crash, held over the last bar; perhaps
  a soft tap two beats later.
- **Slowing down (a ballad):** no steady time; a soft swell or sweep under each of the last
  notes.
- **Three beats of silence after beat 1:** a short comment on snare and toms, closed by one
  quiet stroke on beat 4.

With a singer: never cover the last consonant; stay under the voice; wait a moment for the
phrase to settle; place the last tap under the held note, not on top of it. In a vocal cadenza,
out of time.

With a horn: match the last note's envelope. A short cut-off: the cymbal choked. A held high
note: the cymbal left to ring.

The cymbal's decay: an unchoked crash ringing four seconds after a tight cut-off spoils it.

## 4. When the snare's brush swirl stops

The continuous swirl is a carpet of soft noise; lifting the brush pulls it away, and the ear
turns to whatever is left. The author (2026-11-26): `-play` "can take full control of this and
decide to stop it even if it is requested in the midi file".

| Moment | What the drummer does |
|---|---|
| A solo break (2 or 4 bars alone at a chorus's top) | the sweep lifted on beat 1; back with a tap or a fresh sweep when the band returns |
| A singer whispering, speaking, or out of tempo | sparse taps or silence on the snare, soft quarters on the hi-hat: the hiss masks consonants |
| Before a lift (to the ride, to sticks; A into the bridge; the head into the first solo) | the swirl stops a bar or half a bar before: the air makes the entrance land |
| Stops, hits, kicks, stop-time | the sweep stopped dead on the cut-off (a light tap, or the wires pressed into the head) |
| Between sections | no sweep in the bridge of an AABA tune (bass, hi-hat on 2 and 4, light taps): the return in the last A is warm again |
| A bass solo | a tiny slow circle at the head's edge, or only the lightest taps |

Leaving the swirl: not snatched up (a "zip"), but resolved into a soft tap on 2 or 4, or slowed
and lifted at a phrase's end.

Built of this: only the two bars without swirl before the final note (section 1). The rest is not
built yet.

## 5. Next steps, in order of use

1. A setup figure in the bar before the final note (so the hit is not a surprise).
2. The final crash choked when the band's last note is short, left to ring when it is held.
3. The push: detecting a final chord on the "and" of 4 and catching it there.
4. The drummer under the singer's last phrase: softer, and the last tap after the last consonant.
5. A slowing ending: the sweep given up, swells under the last notes.
6. Breaks: lifting the sweep for a soloist's pickup, and the way back in on the "and" of 4.
7. Section 4: the swirl dropped for breaks, soft singing, bass solos, stops and the bridge.
