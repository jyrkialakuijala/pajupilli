# Orchestra configuration: one stage, one hall

How `-play` could seat its players on one stage in one hall, the way an unamplified concert
sounds, instead of mixing channels that each have their own room.

## Why

Today every channel of `-play` is its own `Ensemble` with its own `Room`, and its player stands in
the middle of it. The parts' stereo outputs are then added like faders on a mixing desk
(`src/midifile.cc`, the render loop). That has three consequences:

- **No places:** the flutes, the oboes and the strings all stand in the middle. Only the choir's
  sections (`setChoirRole`) and a section's own players (the brass section, the strings) are
  spread.
- **Each part's own reverb:** an instrument's late field comes from its own room, fed its
  model's physical pressure (`Ensemble::render`, `gs`). How wet it sounds depends on how loud
  its model is, not on where it sits. In an oratorio-like test the flutes were the loudest parts
  yet had slightly less reverb than the oboes, while the strings had much more. The Baroque flute
  needed its own send (`PJ_BAROQUE_FLUTE_REVERB`) to stop sounding close-miked.
- **No hall between the players:** in a real hall the oboe hears the flute through the room, and
  every player's sound reaches the listener by the same walls. Here each part's room is private.

A Baroque concert has no microphones and no mixing. The balance is where the players sit, how
loud they play and how they radiate. The rest of this document is how to model that.

## The stage as coordinates

The stage is a field with x from −1 (the audience's left) to +1 (right), and y from 0 (the stage
lip, at the podium) to 1 (the back wall). A real stage is about 16–20 m wide and 10–14 m deep;
a chamber stage about 8 × 5 m. Four physical constraints decide where each family is best:

- **Directivity and reflection:** bells and lids need a clear path to the hall or to a shell.
- **Propagation delay:** sound travels 343 m/s, so 15 m across the stage is ~44 ms. Rhythmically
  locked players should stay within ~15–20 ms (5–7 m) of each other.
- **Masking:** brass and percussion reach 100–115 dB SPL on axis and play from risers, over the
  seated players.
- **Sightlines:** within ±45° of the centre for the baton.

### The modern layouts

| Layout | Front arc, left to right | Basses |
|---|---|---|
| American (Stokowski) | Violin I, Violin II, Viola, Cello | far right, behind the cellos |
| European (antiphonal, German) | Violin I, Cello, Viola, Violin II | far left, behind Violin I |
| Central-low (Furtwängler) | as the American (Violin II optionally at the right: Violin I, Viola, Cello, Violin II) | centre-rear, raised, behind the woodwinds and brass |

- **American:** the upper strings blend; the bass is heavy on the right; the violins' dialogue
  loses its stereo.
- **European:** the violins on opposite sides give true antiphony (Beethoven, Brahms), and the
  Violin II f-holes face the hall. The violins are far apart (the latency risk).
- **Central-low:** the bass radiates from the middle and weighs evenly on the stage. It needs
  deep risers, and the basses are far from the front desks.

For `-play`: the European layout for Classical and Romantic readings (the violins' antiphony
is written into that music), the American one when only one violin part is present.

### Zones

| Zone (x, y) | Best | Possible | Avoid |
|---|---|---|---|
| Downstage left (−0.7, 0.2) | Violin I, a solo grand, string soloists | Violin II (European) | brass, timpani, choir, heavy percussion |
| Downstage right (+0.7, 0.2) | cello (American), Violin II (European), a continuo chamber organ | viola (European), harp | horns, heavy percussion |
| Centre, mid riser (0, 0.5) | the woodwinds: flutes and oboes in front, clarinets and bassoons behind | a chamber organ as Baroque continuo | unshielded high brass, double basses |
| Upstage centre (0, 0.85) | timpani; the choir, raised behind them; a choir's organ | double basses (central-low) | harp, celesta, solo singers |
| Upstage left (−0.6, 0.75) | horns; harp and celesta a little in front of them; an orchestral piano | double basses (European), auxiliary percussion | trumpets and trombones (they spoil the horns' reflection off the shell) |
| Upstage right (+0.6, 0.75) | trumpets, trombones, tuba; double basses (American) | the percussion battery | harp, celesta, the soft woodwinds |

Risers: the woodwinds 15–20 cm, the brass 30–45 cm. In the model, a riser is a source height
(the room's image sources already take one): the brass's direct sound clears the seated
players, and their bells see the shell.

## Placing an arbitrary ensemble

Place in decreasing order of immovability and footprint:

1. **Fixed anchors:**
   - **The choir:** upstage centre, on steep risers, over the brass.
   - **The chamber organ:** placed by its function and its wind chest. For sacred or choral works
     it stands upstage centre among the choir's risers, so every section takes its pitch from the
     pipes at once. As orchestral continuo it stands downstage right or mid-centre, beside the
     cellos and basses, so the organist hears the bass line without delay.
   - **The grand piano:**
     - a concerto: downstage left, curving round the podium, its lid open to the hall;
     - inside the orchestra: upstage left, behind the violins or beside the harps, the lid low or
       off.
2. **The timpani:** upstage centre or a little left, in the conductor's central sightline: the
   orchestra's clock. The mallet percussion, bass drum and cymbals flank them along the back
   riser, keeping the metal's harsh transients away from the players' ears.
3. **The winds on risers:** woodwinds in the centre behind the strings; horns upstage left
   (their bells point right and back, at the shell); trumpets, trombones and tuba upstage right.
4. **Harp and celesta:** mid-stage left, behind the violins, out of the brass's cones.
5. **The strings, last:** they fill the front arc round the podium.

Then four rules adjust the result:

- **Coupling springs:** rhythmically locked pairs are pulled together until their delay is under
  15–20 ms. These pairs are the double basses with the cellos; the timpani with the brass and
  the basses; the continuo organ with the cellos and bassoons; and the harpsichord with the
  continuo cello.
- **Elevation for the loud and narrow:** trumpets and trombones on the highest risers, their
  cones over the others' heads.
- **Sightlines:** nothing tall (bass scrolls, the harp's column, an open lid) between the
  conductor and the choir's centre.
- **Fusion or separation:**
  - Close together for blend: horns with cellos and violas, bassoons with cellos.
  - Apart across the stage for contrapuntal clarity: two lines in dialogue (two violins, two
    flutes in canon, the choir's divided parts).

## Baroque layouts

The Baroque ensemble is smaller, has no conductor's podium in the modern sense, and is built
round the continuo. Ideas for `-play` when the director reads a piece as Baroque:

- **The continuo in the middle:** the harpsichord (or the positive organ) at the centre, its
  player directing; the continuo cello and bass beside it, sharing its music. The violone
  behind.
- **Violins on both sides of the continuo:** Violin I at the left, Violin II at the right,
  facing each other; the violas inside them.
- **The obbligato winds in front:** in an aria with an obbligato flute or oboe, the soloist
  stands at the front, by the singer: the two lines in dialogue, near enough to hear each other.
- **Concertino and ripieno** (a concerto grosso): the soloists in front of the continuo, the
  tutti behind and to the sides.
- **The church:** a cantata or a Passion in a church gallery (the Leipzig churches). The choir
  and the orchestra stand together in the organ loft, the organ behind them, all at one height
  and close together. The hall is the church (`church`), and the listener is far below and away.
- **A double choir:** the two choirs, each with its own instruments, on opposite sides
  (`PJ_CHOIR_SECTIONS` would seat choir I left and choir II right).

## The plan for `-play`

1. **One room for the whole song:**
   - Each part's `Ensemble` renders dry: its voices' pressure, as the room is fed now.
   - One `Room` takes every voice of every part as its own source, at its seat.
   - It renders the early reflections and the late field once, for everyone.
   - The parts still render in parallel. The room follows each block: one more pass, already
     vectorised over 16 lines.
2. **The room heard at each player:** the late field's sound at each player's seat (`atSource`)
   goes back to that player's model as now (the oboe hearing the hall).
3. **Every level correction as playing louder:** the room is sent each instrument's pressure
   after its level trim (`levelTrim`), the program's level and the director's dynamics. Each
   then means "this player plays louder", not "a microphone moves closer". The room's feedback
   to each player is divided by the same factor, so every loop through the hall keeps the gain
   it has today (the old tuba oscillation stays away).
4. **The seats:**
   - Every part has a family and a role: melody, inner line, bass, continuo, soloist, choir
     section.
   - The zone table, the order of placement and the four rules give each part an (x, y, height).
   - A part's several voices sit round its seat (a desk, a section).
   - Printed with the parts: "flute 1 at (−0.2, 0.45)".
5. **The layout by style:** the director's reading picks the layout. Baroque: the continuo
   centre, violins either side, obbligato winds in front. Classical and Romantic: European.
   `-layout american|european|central|baroque|church` overrides.
6. **What it replaces:** the per-part rooms; the Baroque flute's extra send; the choir's own
   section placement, which becomes one case of this.

## Seating as a balance of forces

A stage manager starts from the tradition's chairs, then moves them until everyone has room and
the result makes musical sense. That is an iterative balance of attraction and repulsion: a
force-directed layout over the chairs, with the rules above as its forces.

### The chairs

- **One chair per player:**
  - For a part's section, a voice pool's players: two to a desk for the strings, one each for
    the winds.
  - A large body is its own object with its own footprint: the harpsichord, the grand, the
    organ, the timpani and the harp.
- **The footprint** is a radius (or a rectangle for the keyboards) that nobody else may
  enter: its player, the instrument, the bow's or the slide's reach, and a music stand.

  | Instrument | Footprint |
  |---|---|
  | singer | ~0.5 m |
  | flute, oboe, clarinet, violin, viola (a desk) | ~0.9 m |
  | bassoon | ~1.0 m |
  | horn (its bell behind the player) | ~1.1 m |
  | trombone (the slide forward) | ~1.3 m |
  | cello | ~1.3 m |
  | double bass (standing) | ~1.4 m |
  | harp | ~1.6 m |
  | the timpani's set of four (a ring) | ~2.5 m |
  | grand piano | 2.8 × 1.6 m |
  | harpsichord | 2.4 × 1.0 m |
  | positive organ | 1.5 × 1.0 m |

- **The start:** each chair begins at its family's candidate place in the layout the style
  chose: the zone table, or the Baroque layout's continuo centre with violins either side. These
  are the tradition's chairs before anyone moves.

### The forces

Each step adds up the forces on every chair and moves it a little.

- **Repel, room to play:** two chairs closer than the sum of their footprints push apart,
  strongly. This is the only hard constraint.
- **Attract, the tradition's place:** each family is pulled towards its zone (the heat map as a
  potential: its best zones low, its avoided ones high). The pull is weak, so the music can
  move a chair off it.
- **Attract, the section:** a section's chairs pull together into a block: desk partners at
  ~0.8 m, desks one behind another in rows.
- **Attract, the coupling springs:** a spring between rhythmically locked parts, slack until
  their distance would give more than 15–20 ms of delay (5–7 m), then stiff. The pairs:
  - the double basses with the cellos;
  - the timpani with the brass and the basses;
  - the continuo (harpsichord or organ) with its cello, bassoon and violone;
  - an obbligato soloist with the singer it accompanies.
- **Order:** an ordering potential keeps the tradition's sequence across the arc (Violin I left
  of Violin II, the American cellos right of the violas). It costs nothing when the order holds
  and pushes back when two sections would cross.
- **Rows and risers:** y is drawn towards the rows: the strings' arcs, the woodwinds' riser, the
  brass's higher riser, the choir's tiers. The height follows the row it settles on.
- **Masking:** a loud, narrow-beamed source (trumpet, trombone) in front of or level with soft
  players is pushed back and up, until its cone clears them.
- **Sightlines:** the corridor from the podium to the choir's centre, and a ±45° fan for every
  player to the conductor (the harpsichordist, in a Baroque layout). Tall objects (bass scrolls,
  the harp's column, an open lid) are pushed out of the corridor, and players are pushed back
  into the fan.
- **Fusion or separation, from the music:**
  - Parts that double each other, or share a line most of the time (horns with violas or cellos,
    bassoons with cellos, a flute doubling the first violins), attract a little: warm blend.
  - Parts in dialogue (two violin parts trading phrases, two flutes in canon, a double choir)
    repel across the stage: contrapuntal clarity.
  - The director already reads the parts; how much two parts share or answer each other can be
    measured from the notes (the share of beats both play; the lag at which their lines
    correlate).
- **The walls:** the stage's edges and the shell push inwards. The front lip is the strongest,
  since nobody sits past the podium.

### The iteration

1. Place each chair at its candidate place, with a small seeded scatter (the same file always
   gives the same stage).
2. Repeat ~500 steps:
   - add up the forces on every chair;
   - move each chair by its force times a step size;
   - cool the step size (simulated annealing: early steps may jump past a crossing, late ones
     only settle).
3. Snap each chair to its row or riser and resolve any overlap that remains. The hard repulsion
   has the last word.
4. From a few starts (the layouts the style allows), keep the lowest energy. The energy is the
   sum of the potentials: the overlaps, the distance from tradition, the slack springs, the
   crossed orders, the blocked sightlines.

About a hundred chairs and five hundred steps take well under a millisecond. The layout is
computed once per song and printed:

```
the stage (Baroque, after 500 steps): harpsichord (0.0, 0.45); cello (0.12, 0.48); violin I (-0.35, 0.25) ...
```

`-stage FILE` could write it as a picture: the chairs, their footprints and the springs.

### Tuning by ear

Every force has a weight, set by knobs such as `PJ_STAGE_TRADITION`, `PJ_STAGE_SPRING` and
`PJ_STAGE_FUSION`.
- **Tradition high:** the textbook layout.
- **Music high:** the parts move to where the score wants them (the dialogue apart, the doubling
  together).

The balance is where it sounds like players who have arranged themselves to hear each other.

## Checks

- **Dry to wet:** the same note on each wind from the same seat should give the same direct to
  reverberant ratio within ~2 dB. From a seat 5 m further back, the reverb should be ~3–5 dB
  higher relative to the note.
- **Places:** the onsets' interaural level and time differences put each part where it sits
  (the choir's sections already have a test for this).
- **Speed:** the one room costs about what one part's room costs now, a saving on a large
  score.
- **By ear:** the oratorio with the flutes and oboes in front, the choir behind, the continuo in
  the middle.
