# The Shigeru Kawai SK-7 as General MIDI 2 (Bright Acoustic Piano)

Status: 2026-11-20. Research first (sections 1-6), then the build plan (section 7) and what was
built (section 8). Adds to `research/piano.md` (the concert grand, the model's base),
`research/piano_variants.md` §2 (the bright piano as a voicing and miking of the grand) and
`research/piano_design.md` (the `PianoDesign` struct). Where those already say a thing it is not
repeated.

Tags: **[web, source]** looked up for this note (section 9); **[paper]** from the acoustics
literature as cited in piano.md; **[calc]** worked out here; **[memory]** from general knowledge,
not looked up, to be checked before it is relied on. Numbers in the plan are starting choices
for the model, not claims about the instrument.

## 1. Short answers

- **What it is.** Kawai's 7'6" semi-concert grand of the hand-built Shigeru line: 229 cm long,
  157 cm wide, 102 cm high, 400 kg **[web, Kawai SK-7 page]**. Built at Kawai's Ryuyo grand
  factory and finished (regulated, voiced) by its Master Piano Artisans **[web, Kawai]**. The
  size of a Yamaha C7 (227 cm) and a Steinway C (227 cm), between a Steinway B (211 cm) and a D
  (274 cm) **[web, Steinway; memory for the C7]**.
- **Why it fits "bright acoustic piano".** Kawai's own words are "brilliant treble, resounding
  bass" and "second only to the SK-EX in full-bodied tone and exceptional dynamic range"
  **[web, Kawai]**. A dealer who owns the 7' SK-6 says it "can sound as bright as a Yamaha or
  Bechstein" when wanted, its clarity "more even across the scale" than a Steinway B's, and
  "dynamically even throughout the range" where the B's "projection and sustain dip in the 4th
  and 5th octaves" **[web, Merriam Music]**. That is the timbre GM 2 names: a grand that is
  clear and singing in the treble, brighter than a concert Steinway, not the glassy edge of a
  worn or lacquered hammer.
- **Where the brightness comes from, physically.** (1) The hammers: Kawai presses its felt to a
  high compression and needles it down at the factory **[web, Kawai voicing bulletin 71-211]**:
  a firm core under a worked shoulder. Shiko Seion hammers use no heat and no chemical
  hardeners **[web, Kawai; Alamo Music]**: the firmness is in the felt itself, so it grows with
  the blow (a "pressed" bright, not a lacquered bright). (2) A shorter scale: a 2.29 m case has
  bass strings about 1.66 m long instead of 2.0 m **[calc, section 3]**; the wound strings are
  ~1.5x stiffer and the low tenor ~2-3x, so their upper partials stand sharper and stronger
  against the fundamental. (3) A smaller, stiffer board: higher lowest resonances, a little less
  low fundamental from the bass; the treble's share rises. (4) How it is recorded: close.
- **What it is not.** No measured decays, inharmonicity or spectra of an SK-7 were found (nor of
  any Kawai grand: the literature measures Steinways and Yamahas). Its tone is described only by
  the maker, dealers and players; the model's numbers are therefore derived from the concert
  grand's fitted ones by the physics of the differences.

## 2. The instrument as built (what is known)

| Part | SK-7 | Tag |
|---|---|---|
| Size | 229 x 157 x 102 cm, 400 kg | [web, Kawai SK-7 page] |
| Action | Millennium III ABS-Carbon: carbon-fibre-reinforced ABS parts in place of wooden ones (wippens, jacks, flanges); extended keysticks, lengthened to match those of the 9' SK-EX; concert-length key buttons; dual-pivot damper action for graded half-pedal | [web, Kawai technology page] |
| Hammers | Shiko Seion ("ultimate voicing"): New Zealand and Australian long-fibre wool, hand-pressed, no heat, no artificial hardeners; double-felted on mahogany moldings; voiced by hand by Master Piano Artisans | [web, Kawai; Alamo Music; Merriam Music] |
| Hammer pressing | "pressed for high compression of the felt", "premium high density felt", deep needling of the shoulders in two stages at the factory (the first by machine before hanging) | [web, Kawai voicing bulletin 71-211] |
| Soundboard | solid spruce, tapered (thicker in the middle, thinner at the rim), "Kigarashi" naturally aged (no kiln, no chemicals); SK-5/6/7 only. Ezo spruce on the SK-7 and SK-EX from a small remaining stock (one dealer); Sitka on the SK-EX (another dealer): sources disagree | [web, Kawai; Alamo Music; Merriam Music] |
| Ribs and crown | not published. Kawai crowns by rib shape on its K uprights (full-length ribs notched into the liner) | [web, Kawai K series]; [memory] that grands crown mostly by curved ribs |
| Rim | "Konsei Katagi": a blend of small-pore hardwoods (brightness, projection) and dense large-pore ones (warmth); the SK-5 page names rock maple and mahogany. The SK-7's species are not listed | [web, Kawai; Alamo Music] |
| Beams | four, aged spruce (Sitka) | [web, Kawai SK-7 page; Kawai technology] |
| Bridges | boxwood caps | [web, Kawai technology] |
| Agraffes | concert agraffes machined from solid billet brass, keys 1-54; a capo bar from key 55 (G4) up | [web, Kawai SK-7 page] |
| Duplex | front and aliquot (back) duplex scaling | [web, Kawai SK-7/SK-6 pages] |
| Bass strings | "Temaki": copper-wound by hand | [web, Kawai] |
| Treble wire | not published. Neither Röslau nor Mapes is named anywhere; Japanese makers commonly use Japanese wire (Suzuki) | [web: nothing found]; [memory] |
| Pedals | three: damper, sostenuto, soft | [web, Kawai] |

## 3. Scale and strings (derived)

Nothing of the SK-7's scale is published. Derived **[calc]** from the case lengths of grands whose
longest string is known: Steinway S (155 cm) 116 cm **[web, forum]**; the model's D (274 cm)
2.01 m. A line through them (L = 0.706 x case + 0.07 m) gives **1.68 m** for 229 cm, and
1.36 m for a 183 cm A1 (forum owners report 1.27-1.42 m). Taken: **A0 1.66 m**.

| | Model grand (D-like) | SK-7 (model) | Why |
|---|---|---|---|
| Wound strings | keys 1-20 (A0-E2) | keys 1-20 | a 7'6" grand's break is near the D's **[memory]** |
| A0 length | 2.01 m | 1.66 m | above |
| E2 (key 20) | 1.33 m | 1.16 m | the bass bridge shorter by the case |
| F2 (key 21), the lowest plain | 1.90 m | 1.44 m (x 0.76) | the long bridge's low end moves in most where the case is shorter |
| C3 / C4 / C5 | 1.31 / 0.695 / 0.368 m | 1.06 / 0.625 / 0.368 m | from C5 up all grands' scales nearly agree **[memory]** (piano_variants.md §3.1) |
| B, wound (same core and tension) | 8e-5 (A0), 4.5e-5 (key 13) | x (L_D / L)^2: 1.17e-4, 6.0e-5 | B ∝ d^2 / (L^4 f^2) and T ∝ L^2 [calc] |
| B, plain (the grand's gauge, 0.96 of it at F2 thinning less up to C5) | 5.8e-5 (F2), 3.0e-4 (C4) | 1.4e-4 (F2), 3.5e-4 (C4) | B ∝ d^2 / L^4 [calc] |
| Strike | 0.122 to C5, to 0.07 at C8 | the same | the strike line is a hammer-line on every grand; no SK-7 figure found |

- The duplex (front and aliquot) lengths are not modelled (as on the grand, piano.md §10).
- The agraffes to key 54 and the capo above it: the capo bar's V-shape gives a slightly harder
  termination than an agraffe **[memory]**; not modelled.
- Tuning: the shorter scale's higher B stretches the tuning by itself through `tuneByPartials`
  (2:1 octaves up, 6:3 down): **[calc, in the build]** the bass about 2-4 c lower and the top
  about 3-6 c higher than the model D.

## 4. Soundboard

- Kawai's board is tapered solid spruce **[web]**, as the model's (9 mm in the middle to 6 mm at
  the rim). Whether Ezo or Sitka on a given SK-7 is not settled **[web, sources disagree]**:
  Ezo is described as very reactive and long-sustaining ("next to Fazioli's red spruce, some of
  the longest sustain times") **[web, Merriam]**. The model keeps the grand's spruce constants.
- Size: the case 45 cm shorter, as wide. The model board: the grand's outline kept for the first
  1.5 m from the belly rail (the treble end of all grands is alike), the tail drawn in to end at
  1.70 m instead of 2.12: **1.77 m^2, 11.4 kg** (the grand's 1.98 m^2, 12.8 kg) **[calc,
  tools/sk7board.py]**. (First tried: the whole outline x 0.80, 1.58 m^2: the treble bridge then
  ran near the bent side and single keys of the top two octaves fell 10-15 dB into dead spots,
  4.5-5 dB rms off their neighbours against the grand's 2.3.)
- Resonances: lowest 78, 128, 174 Hz (the grand's 80, 126, 170); 256 resonances reach 2.42 kHz
  (the grand's 2.29). Bridge impedance on the long bridge (300 Hz-2.5 kHz): 2200-3800 kg/s; at A0
  below 300 Hz ~700 kg/s (the grand's 770-1060) **[calc]**.
- Rim: a hard blend with maple **[web]**: the model has no rim; a reflecting rim keeps energy in
  the board (longer sustain): the board's loss slightly lower (section 7).

## 5. Hammers and action

- **Hardness relative to a Steinway D.** No published hardness numbers for any maker. What is
  documented: Kawai presses for high compression and needles down **[web, Kawai bulletin]**;
  Steinway (New York) hangs soft hammers and brings them up with lacquer or hardener; Hamburg
  hammers are firmer, needled down **[memory, technicians' common knowledge]**. Kawai without
  hardeners means: hard at depth, worked soft at the surface (the shoulders needled): soft blows
  stay round, loud ones bloom brighter. In the model that is a stiffer felt `K` from the middle
  up with the same nap (the needled surface) and the same hardening exponent (`colour`).
- **Mass.** The Millennium III action replaces wooden parts with ABS-carbon but the hammers are
  felt on mahogany as on any grand: 11 g bass to ~4-5 g treble **[paper, KTH/Conklin; memory]**.
  The SK-7's keys are long (SK-EX length) for even leverage **[web, Kawai]**: no change to the
  hammer's mass; its speed per velocity as the grand's player gives.
- **Voicing target.** The bulletin's advice for a too-bright treble is fewer, shallower needles
  in the high treble **[web]**: Kawai's own treble is voiced to be brilliant and is softened by
  the technician to taste: the model's treble hammers stiffer by a larger factor than the bass.

## 6. The tone as described

| Quality | Said of the SK-7 / Shigeru line | Against | Source |
|---|---|---|---|
| Brilliance | "brilliant treble"; "can sound as bright as a Yamaha or Bechstein", but also "some of the darkest tones" (the SK-6: wide palette) | Yamaha C7: bright by reputation and "heavily recorded on pop records"; CFX "on the brighter end" | [web, Kawai; Merriam; piano_variants.md] |
| Clarity | "clean, clear tone" (credited to the billet agraffes); clarity even across the scale | Steinway B less even | [web, Kawai; Merriam] |
| Evenness | "dynamically even throughout the range"; the treble "consistent with the rest" | B dips in octaves 4-5; Mason & Hamlin's treble less consistent | [web, Merriam] |
| Sustain | "a sustain matched by only one or two other makes in the world" (Ezo board) | | [web, Merriam] (a dealer's claim) |
| Bass | "resounding bass" | | [web, Kawai] |
| Warmth | "very warm but also very colorful" (SK-EX); "not as clear and defined as a Bechstein or Bösendorfer" (SK-EX) | Hamburg D more blended; CFX brighter | [web, Merriam, CFX/D/SK-EX] |
| General | Kawai "warm, even tone"; Steinway "richer, more complex, more variable" | | [web, Mark Goodwin Pianos] |

Reading for the model: brighter and clearer than the concert grand from the tenor up, even from
key to key, a sustaining treble; a full but not boomy bass; not harsh (the upper partials grow
with the blow, the soft notes stay round).

**Measured numbers.** None found for the SK-7 (searches for Kawai inharmonicity, spectra, decay
and strike-line data gave only Steinway D and Yamaha measurements) **[web: nothing]**. The model
keeps the concert grand's fitted losses (piano.md §11) and changes only what the physical
differences change.

## 7. Build plan: PianoDesign, SK-7 against the concert grand

| Field | Grand | SK-7 | Reason |
|---|---|---|---|
| `key[].len` | 2.01 m (A0) .. 1.33 (E2); 1.90 (F2) .. 0.368 (C5) | 1.66 .. 1.16; 1.44 .. 0.368 | §3: a 229 cm case |
| `key[].B` | stiffness(key) | wound x (L_D/L)^2; plain from the wire (gauge x 0.96 at F2 to x 1 at C5) | §3 |
| `key[].tension, mu` | | wound: the grand's tension, mu for the pitch; plain: from the wire | |
| `key[].K` | fitted | x 1.0 (A0) rising to x `PJ_SK7_HARD` (2.0) at key 40 and x `PJ_SK7_HARDTOP` (2.5) from key 64 | §5: high-compression felt; the treble brilliant |
| `key[].nap` | 0.9 mm (C4) | x `PJ_SK7_NAP` (0.5; first planned 0.8) | a dense pressing, only the shoulders needled: a thinner loose layer. Measured to be the main lever (section 8) |
| `key[].colour, pivot, p, mh` | | the same | no hardener: the felt's law as the grand's |
| `key[].loss, b2, b3` | | the same | the wire is wire |
| `strike` | | the same | no data |
| `unisonCents` | 0.08 .. 0.03 c | the same | a Shigeru is tuned by its artisans as finely as a concert D |
| `tuneByPartials` | 2:1 up, 6:3 down | the same rule, on the new B: more stretch | §3 |
| `board` | pianoboard_data.h, 1.98 m^2 | sk7board_data.h, 1.58 m^2 | §4 (tools/sk7board.py) |
| `boardLoss` | 1.0 | `PJ_SK7_ETA` 0.7 (planned 0.85) | the long-sustaining board and the hard rim (§4, §6): a dealer's claim, a small step |
| `mic[]` | crossed figure-of-eights 0.65 m over the strings, 0.5 m from the belly rail | 0.30 m over the strings (`PJ_SK7_MICZ`; planned 0.35), 0.40 m from the belly rail (`PJ_SK7_MICX`), the same angle and pattern | a studio/pop grand miked close over the hammers' half of the board |
| reverb send (src/ensemble.cc) | +1 dB (`PJ_PIANO_REVERB`) | the others' (0 dB) | close mics: a drier picture |
| `seed` | 4242 | 7007 | its own unisons and tilts |

No EQ: the brightness is the hammers', the scale's and the board's, and the close microphones'.

## 8. As built (2026-11-20)

Files: `src/sk7.h/.cc` (`sk7Design()`), `tools/sk7board.py` -> `src/sk7board_data.h`,
`src/pianoinst.cc` (`-inst bright-piano` is a `PianoModel`), `src/bassinst.cc` (the table),
`src/midifile.cc` (GM 2 -> bright-piano, else grand-piano), `src/levels_data.h` (+4.6 dB),
`src/velgain_data.h`, `tools/test.py` (`bright_piano()`). The concert grand renders
bit-identically to before (a `-seq` of chords and a 20 s `-play`, compared byte for byte).

**What moved the brightness, and what did not** (above-2 kHz share of C3/C4/C5, first 0.5 s, at
velocities 64 and 96): the felt's stiffness K x10 gave +2..+5 dB; the hardening exponent +2
+1..+3; the hammer mass x0.5 +1..+3; the hammer's width, the pivot force under 1 dB. The nap
(the loose outer layer) was the lever: x0.5 thick gave +7..+12 dB at velocity 64, +1..+6 at 96,
and leaves the softest blows round. A stiffer nap (napK x3) made it darker, not brighter.
The close microphones gave little by themselves (the model hears only the board, not the
strings' own sound).

**Measured (velocity 80 unless said):**

| | grand-piano | bright-piano |
|---|---|---|
| Tuning (`-note`, cents; C1 by its partials) | -7 (C1) .. +3 (C7) | -15 (C1), +10, +1, -5, +2, +3, +10 (C2..C7) |
| B at C1 / C2 / C3 / C4 | 6.9e-5 / 5.0e-5 / 1.1e-4 / 3.0e-4 | 1.0e-4 / 9.2e-5 / 2.3e-4 / 4.3e-4 |
| Level range, middle C, velocity 16 -> 127 (the model's own) | 31.5 dB (A-weighted LAFmax) | 29.4 dB (LAFmax); 25.8 dB (rms, first 0.3 s) |
| Above 2 kHz, C3 / C4 / C5, velocity 64 | -45 / -62 / -37 dB | -30 / -37 / -25 dB |
| 4-8 kHz at velocity 120-127, C3 / C4 / C5 | -42 / -52 / -46 dB | -37 / -36 / -34 dB |
| Above 2 kHz at the softest (grand: velocity 20; bright: 16), C3 / C4 / C5 | -59 / -66 / -51 dB | -67 / -60 / -53 dB (round) |
| Fall to -20 / -40 / -60 dB: C1 | 4.1 / 14.6 / 20+ s | 3.4 / 11.9 / 20 s |
| C2 | 2.5 / 10.6 / 20+ | 1.6 / 6.2 / 16.9 |
| C3 | 2.0 / 8.6 / 19.3 | 2.3 / 8.0 / 19.8 |
| C4 | 1.2 / 5.7 / 17.8 | 2.4 / 9.5 / 15.9 |
| C5 | 1.2 / 4.7 / 11.8 | 1.2 / 3.6 / 8.5 |
| C6 | 1.0 / 1.7 / 6.4 | 0.9 / 1.9 / 5.5 |
| Evenness, rms off the neighbours (A0-E4 / F4-C#6 / D6-C8), velocity 100 | 0.9 / 0.9 / 2.3 dB | 0.8 / 1.0 / 2.5 dB |
| Level trim (`tools/levels.py` measure(), median -28.2 LUFS) | +9.8 | +4.6 (raw -32.8 LUFS) |
| Cost, one note / pedal down (a machine at load 60) | 3.1 / 11.7 us a sample | 3.4 / 12.1 us a sample |

**Against FluidR3 GM's Bright Acoustic Piano** (TiMidity++, the same MIDI notes, keys C2-C7):
FluidR3's GM 2 is its GM 1 sample 3 dB louder, with the same spectrum: about -15 dB above
2 kHz at C2-C4, -10 at C5, -5 at C6, at every velocity (one layer, a filter). Here the share
grows with the blow: C4 -63 / -53 / -35 / -28 / -24 dB at velocities 16 / 40 / 64 / 96 / 127; at
a hard blow the bright piano reaches FluidR3's middle-register brightness, and stays 6-14 dB under
it at 4-8 kHz (FluidR3 -21 .. -30 dB: its samples' hammer noise and close miking). The velocity
curves (with `src/velgain_data.h`'s row) agree within 1.5 dB from 16 to 127. FluidR3's treble
dies sooner (C5 -20 dB in 0.57 s, here 1.2 s).

**Still open:** nothing of the SK-7 is measured, so its scale, felt and board are derived (sections
3-5) and the brightness is set against the grand and FluidR3, not against a recording of an
SK-7. The duplex scale, the capo's termination, the strings' own radiation near the close
microphones and the hammers' knock are not modelled. C5 and C6 sustain a little less than the
grand's (the shorter board's bridge), against the makers' claims of a long-singing treble.

## 9. Sources

- Kawai America, SK-7 page: https://kawaius.com/shigeru/models/sk-7 (specification, agraffes 1-54,
  front and aliquot duplex, tapered solid spruce, four beams, tone claims).
- Kawai America, Shigeru technology: https://kawaius.com/shigeru/technology (Millennium III,
  extended keys, Shiko Seion, Kigarashi board, Konsei Katagi rim, boxwood bridge caps, billet
  agraffes, Temaki bass strings, SOLID design).
- Kawai America, Shigeru models: https://kawaius.com/shigeru/models; SK-5 and SK-6 pages at
  shigerukawai.com (tapered board; rock maple and mahogany rim on the SK-5).
- Kawai Service Bulletin 71-211, Voicing:
  https://kawaius.com/wp-content/uploads/2025/08/Voicing-Manual-Kawai.pdf (as quoted in search
  excerpts; the PDF itself returned 404 when fetched).
- Alamo Music, Shiko Seion hammers: https://alamomusic.com/pages/shiko-seion-hammers; Konsei Katagi
  rim: https://www.alamomusic.com/pages/konsei-katagi-rim.
- Merriam Music, "Our Shigeru Kawai SK-6": https://merriammusic.com/studio/the-piano (SK-6 against
  Steinway B, Yamaha C7, Fazioli, Mason & Hamlin; Ezo spruce).
- Merriam Music, "Yamaha CFX vs Hamburg Steinway D vs Kawai SK-EX":
  https://www.merriammusic.com/blog/yamaha-vs-kawai-vs-steinway/ (impressions of sample libraries).
- Merriam Music, "10 interesting facts about Shigeru Kawai": https://www.merriammusic.com/pianos/shigeru-kawai/
  (Ezo on SK-7 and SK-EX).
- Mark Goodwin Pianos, "Steinway vs Shigeru Kawai":
  https://markgoodwinpianos.co.uk/steinway/comparisons/steinway-vs-shigeru-kawai.
- PTG forum, Steinway S longest string 45 1/2": https://my.ptg.org/discussion/steinway-b-stringing
  (and an A1 owner's 1.27-1.42 m, PianoWorld).
- Jaatinen & Pätynen, JASA 2022 (inharmonicity of every string of a Steinway D): the literature's
  measurements are of Steinways; none of a Kawai was found.
